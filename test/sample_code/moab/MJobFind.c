/* HEADER */


      
/**
 * @file MJobFind.c
 *
 * Contains JOB FIND Functions of various types
 *
 */


#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 *
 * Build a job name to search for the job based on the RM type and search mode.
 *
 * @param SpecJobName (I) job name
 * @param QJobID (I/O)
 * @param QJobIDSize (I) [optional]
 * @param Mode (I) [completed,basic,extended,jobname]
 */

void MJobFindBuildQJobID(

  const char           *SpecJobName, /* I   job name */
  char                 *QJobID,      /* O   generated job name buffer */
  long                  QJobIDSize,  /* I   generated job name buffer size */
  enum MJobSearchEnum   Mode)        /* I   search mode (completed,basic,extended,jobname) */

  {
  char JobName[MMAX_NAME];

# if defined(__MLL)
  int count;

  char *split;

# endif /* __MLL */

  if (Mode != mjsmInternal)
    {
    MJobGetName(
      NULL,
      SpecJobName,
      &MRM[0],
      JobName,
      sizeof(JobName),
      mjnShortName);
    }
  else
    {
    MUStrCpy(JobName,SpecJobName,sizeof(JobName));
    }

  /* change to (Mode == 1) to restore */

  if (Mode == mjsmExtended)
    {
#   if defined(__MLL)

    {
    char *ptr;

    /* FORMAT:  <SUBMITHOST>[.<DOMAIN>].<JOBID>.<STEPID> */

    count = 0;

    for (ptr = JobName + strlen(JobName) - 1;*ptr != '\0';ptr--)
      {
      if (*ptr == '.')
        {
        count++;

        if (count == 2)
          split = ptr;
        }
      }    /* END for (ptr) */

    if (count > 2)
      {
      /* fully qualified submit hostname specified */

      MUStrCpy(QJobID,JobName,QJobIDSize);
      }
    else
      {
      /* non-fully qualified submit hostname specified */
      /* insert DefaultDomain                          */

      MUStrCpy(QJobID,JobName,MIN(QJobIDSize,split - JobName + 1));

      strcat(QJobID,MSched.DefaultDomain);

      strcat(QJobID,split);
      }
    }     /* END BLOCK */

#   elif defined(__MPBS)

    {
    char *ptr;

    /* try PBS format name */

    /* FORMAT:  <JOBID>[.<SUBMITHOST>.<DOMAIN>] */

    if ((ptr = strchr(JobName,'.')) != NULL)
      {
      MUStrCpy(QJobID,JobName,MIN(QJobIDSize,ptr - JobName + 1));
      }
    else
      {
      MUStrCpy(QJobID,JobName,QJobIDSize);
      }
    }    /* END BLOCK */

#   else /* __MPBS */

    MUStrCpy(QJobID,JobName,QJobIDSize);

#   endif /* else defined(__MLL) */
    }
  else
    {
    MUStrCpy(QJobID,JobName,QJobIDSize);
    }

  return;
  } /* END MJobFindBuildQJobID() */





/**
 *
 * Look for the specified job name in the corresponding job name hash table(s).
 *
 * There are "name" and "aname" hash tables for jobs in MJob[]. 
 * There are "name" and "aname" hash tables for jobs in MCJob[]. 
 *
 * @param JobName (I)
 * @param JP (O)
 * @param CompletedJob (I)
 */

int MJobFindHTLookup(

  const char           *JobName,
  mjob_t              **JP,
  mbool_t               CompletedJob)

  {
  mjob_t  *J = NULL;

  if (JobName == NULL)
    return(FAILURE);

  if (JP != NULL)
    *JP = NULL;

  if (CompletedJob == TRUE)
    {
    return (MCJobTableFind(JobName,JP));
    }

  /* Look in the Name and AName hash tables */

  if (MJobTableFind(JobName,&J) == FAILURE)
    {
    /* not found in the name table */

    MDB(8,fRM) MLog("INFO:     hash table lookup name (%s) %s failed \n",
      JobName,
      (CompletedJob == TRUE) ? "completed job" : "");

    if (MUHTGet(&MJobDRMJIDHT,JobName,(void **)&J,NULL) == FAILURE)
      {
      /* not found in the aname table either so we are done */

      MDB(8,fRM) MLog("INFO:     hash table lookup drmjid (%s) %s failed \n",
        JobName,
        (CompletedJob == TRUE) ? "completed job" : "");

      return(FAILURE);
      }
    else
      {
      /* found in the aname table */

      MDB(8,fRM) MLog("INFO:     hash table lookup drmjid (%s) succeeded\n",
        JobName);
      }
    }
  else
    {
    /* found in the name table */

    MDB(8,fRM) MLog("INFO:     hash table lookup name (%s) succeeded\n",
      JobName);
    }

  if (!MJobPtrIsValid(J))
    {
    MDB(8,fRM) MLog("INFO:     hash table lookup (%s) failed - empty job\n",
      JobName);

    return(FAILURE);
    }

  /* match located */

  if (JP != NULL)
    *JP = J;

  MDB(8,fRM) MLog("INFO:     hash table lookup (%s) found job (%s)\n",
    JobName,
    J->Name);

  return(SUCCESS);
  } /* END MJobFindHTLookup() */




/**
 * Find job by AName.
 *
 * @see MJobFind() - parent
 *
 * @param AName (I)
 * @param JP (O)
 */

int MJobFindAName(

  const char  *AName,
  mjob_t     **JP)

  {
  job_iter JTI;

  mjob_t *J;

  if ((AName == NULL) || (JP == NULL))
    {
    return(FAILURE);
    }

  MJobIterInit(&JTI);

  while (MJobTableIterate(&JTI,&J) == SUCCESS)
    {
    if (J->AName == NULL)
      continue;

    /* skip array sub jobs */

    if ((bmisset(&J->Flags,mjfArrayJob)) &&
        (!bmisset(&J->Flags,mjfArrayMaster)))
      continue;

    if (strcasecmp(AName,J->AName) == 0)
      {
      *JP = J;

      return(SUCCESS);
      }
    }

  return(FAILURE);
  }  /* END MJobFindAName() */
 
 
  
 
 
  
/**
 * Locate specified job using jobid, jobname, drmjid, and other attributes.
 * NOTE:  if Mode=mjsmJobName, *JP must be initialized to NULL before first call
 *
 * Use MJobCFind to find jobs in the completed job hash table.
 *
 * search order:
 * @li 1) basic - active jobs using J->Name
 * @li 2) jobname - active jobs using J->AName
 * @li 3) extended - search all of the above
 *
 * @see MJobCFind() - locate completed jobs in MCJob[] table
 * @see MJobFindSystemJID()
 *
 * @param SpecJobName (I) job name
 * @param JP (I/O) [optional,in for jobname only]
 * @param Mode (I) [basic,extended,jobname]
 */
  
int MJobFind(
  
  const char           *SpecJobName, /* I   job name */
  mjob_t              **JP,          /* I/O job pointer (optional,in for jobname only) */
  enum MJobSearchEnum   Mode)        /* I   search mode (basic,extended,jobname) */

  {
  mjob_t *J;
  char QJobID[MMAX_NAME];

  const char *FName = "MJobFind";

  MDB(8,fSTRUCT) MLog("%s('%s',JP,%s)\n",
    FName,
    (SpecJobName != NULL) ? SpecJobName : "NULL",
    MJobSearchMode[Mode]);

  if (JP != NULL)
    *JP = NULL;

  if ((SpecJobName == NULL) || (SpecJobName[0] == '\0'))
    {
    return(FAILURE);
    }

#if defined(__MLL)
  if (!strncmp(SpecJobName,MSched.Name,strlen(MSched.Name)))
    {
    char *ptr;
    char *TokPtr;

    char JobID[MMAX_NAME];

    MUStrCpy(JobID,SpecJobName,sizeof(JobID));

    ptr = MUStrTok(JobID,".",&TokPtr);

    ptr = MUStrTok(NULL,".",&TokPtr);

    if ((ptr == NULL) || (ptr[0] == '\0') || (isdigit(ptr[0])))
      Mode = mjsmInternal;
    }
#endif /* LL */

  /* search order
   *
   * 1) basic - active jobs using J->Name
   * 2) jobname - active jobs using J->AName
   * 3) extended - search all of the above
   */
 
  if ((Mode == mjsmBasic) || (Mode == mjsmExtended))
    {
    /* Build the qjobid to search for */
    /* hash table searches are fast so look for both the shortened name
       and the full name */

    MJobFindBuildQJobID(SpecJobName,QJobID,sizeof(QJobID),Mode);

    /* Look up the job in the hash table with the Q job id */

    if ((MJobFindHTLookup(SpecJobName,&J,FALSE) == SUCCESS) ||
        (MJobFindHTLookup(QJobID,&J,FALSE) == SUCCESS))
      {
      MDB(5,fSCHED) MLog("INFO:     found active job\n");

      /* match located */

      if (JP != NULL)
        *JP = J;

      return(SUCCESS);
      }  /* END (MJobFindHTLookup(QJobID,&J,FALSE) == SUCCESS) */
    }

  if ((Mode == mjsmJobName) || (Mode == mjsmExtended))
    {
    if (MJobFindAName(SpecJobName,&J) == SUCCESS)
      {
      MDB(5,fSCHED) MLog("INFO:     found job by aname\n");

      if (JP != NULL)
        *JP = J;

      return(SUCCESS);
      }
    }

  return(FAILURE);
  }  /* END MJobFind() */





/**
 * Searches through Moab's internal completed job queue (MCJob array) to
 * find a job matching the specified job name.
 *
 * @see MJobFind()
 *
 * @param JName (I) Name of the completed job to find.
 * @param JP (O) A pointer to the found job (set to NULL if the job is not found) [optional]
 * @param Mode (I) mjsmJobName, mjsmExtended, mjsmBasic are supported.
 */

int MJobCFind(

  char    *JName,  /* I */
  mjob_t **JP,     /* O (optional) */
  enum MJobSearchEnum Mode)  /* I */

  {
  job_iter JTI;

  mjob_t *J;

  if (JP != NULL)
    *JP = NULL;

  if (JName == NULL) 
    {
    return(FAILURE);
    }

  /* Look for job in completed job hash table */

  if (MJobFindHTLookup(JName,&J,TRUE) == SUCCESS)
    {
    if (JP != NULL)
      *JP = J;

    return(SUCCESS);
    }

  if ((Mode == mjsmExtended) || (Mode == mjsmJobName))
    {
    /* search DRMJID also */
    
    MCJobIterInit(&JTI);

    while (MCJobTableIterate(&JTI,&J) == SUCCESS)
      {
      if ((Mode != mjsmJobName) &&  /* don't search DRMJID on JobName search */
          (J->DRMJID != NULL) &&
          (!strncmp(J->DRMJID,JName,strlen(JName))))
        {
        if (JP != NULL)
          *JP = J;
        
        return(SUCCESS);        
        }

      if ((J->AName != NULL) && /* search AName on Extended and JobName */
          (!strncmp(J->AName,JName,strlen(JName))))
        {
        if (JP != NULL)
          *JP = J;
        
        return(SUCCESS);        
        }
      } /* END for (J) */
    }   /* END if (Mode == mjsmExtended) */
      
  return(FAILURE);
  }  /* END MJobCFind() */




/**
 *  Searches for a dependency in all possible locations.
 *
 *  JobOut stores NULL if the function returns FAILURE or if JobIdPtr 
 *    represented a valid but non-existent job name
 *
 * @see MJobFindDependent()
 *
 * @param JobIdPtr (I) 
 * @param JobOut   (O)
 * @param Msg      (O) [optional,minsize=MMAX_LINE]
 */

int MJobFindDepend(

  char const *JobIdPtr,  /* I */
  mjob_t    **JobOut,    /* O */
  char       *Msg)       /* O [optional,minsize=MMAX_LINE] */

  {
  if (JobOut != NULL)
    {
    *JobOut = NULL;
    }
  else
    {
    return(FAILURE);
    }

  if (JobIdPtr == NULL)
    {
    return(FAILURE);
    }

  if (!strcasecmp(JobIdPtr,"none"))
    {
    *JobOut = NULL;
    }
  else if ((MJobFind((char *)JobIdPtr,JobOut,mjsmExtended) != SUCCESS) &&
           (MJobCFind((char *)JobIdPtr,JobOut,mjsmExtended) != SUCCESS))
    {
    if (Msg != NULL)
      snprintf(Msg,MMAX_LINE,"ERROR:  specified job '%s' does not exist/is invalid",
        JobIdPtr);

    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MJobFindDepend() */




/**
 * Attempt to locate a job template in the job template array
 * using the job name as the search.
 *
 * @see MTJobAdd() for MTJob create.
 *
 * NOTE:  case insensitive in Moab 5.4 and higher
 *
 * @param TJName the template name to search on. (I)
 * @param TJP the job to populate with the template information if found.(O)
 */

int MTJobFind(

  const char  *TJName,
  mjob_t     **TJP)

  {
  int jindex;
  mjob_t *TJ;

  const char *FName = "MTJobFind";

  MDB(7,fSCHED) MLog("%s(%s,TJP)\n",
    FName,
    (TJName != NULL) ? TJName : "NULL");

  if (TJP != NULL)
    *TJP = NULL;

  if (TJName == NULL)
    {
    return(FAILURE);
    }

  if (TJName[0] == '^')
    TJName++;

  /* search job template table for match */
  
  TJ = NULL;
  
  for (jindex = 0;MUArrayListGet(&MTJob,jindex) != NULL;jindex++)
    {
    TJ = *(mjob_t **)(MUArrayListGet(&MTJob,jindex));

    if (TJ == NULL)
      break;

    if (!strcasecmp(TJ->Name,TJName))
      {
      break;
      }

    TJ = NULL;
    }

  if (TJ == NULL)
    {
    /* no match found */

    MDB(7,fSCHED) MLog("INFO:     job template '%s' not found\n",
      TJName);

    return(FAILURE);
    }
  
  if (TJP != NULL)
    *TJP = TJ;

  return(SUCCESS);  
  }  /* END int MTJobFind() */




/**
 * Locate jobs which depend upon specified job.
 *
 * NOTE:  search both normal and completed job hash tables.
 *
 * @see MJobReserve() - parent
 *
 * @param SJ   (I)
 * @param DJP  (O) [optional]
 * @param JTI  (I) [token indicating last location checked]
 * @param CJTI (I) [token indicating last location checked]
 */

int MJobFindDependent(

  mjob_t      *SJ,
  mjob_t     **DJP,
  job_iter    *JTI,
  job_iter    *CJTI)

  {
  mjob_t    *J;
  mdepend_t *D;

  char const *NameEnd;

  if ((SJ == NULL) || (JTI == NULL) || (CJTI == NULL))
    {
    return(FAILURE);
    }

  /* NOTE:  only check 'AND' dependencies */

  while (MJobTableIterate(JTI,&J) == SUCCESS)
    {
    if (J->Depend == NULL)
      continue;

    D = J->Depend;
 
    while (D != NULL)
      {
      if (D->Type != mdDAG)
        {
        /* value is job ID */

        if (!strcmp(D->Value,SJ->Name))
          {
          if (DJP != NULL)
            *DJP = J;

          return(SUCCESS);
          }
        }

      D = D->NextAnd;
      }    /* END while (D != NULL) */
    }    /* END if (SearchActive == TRUE) */

  /* sometimes a job name might be fully qualified, i.e., <jobid>.<hostname>.<domainname>, 
   * but the dependency id will simply be <jobId>  . We use NameEnd to truncate the 
   * job name to just <jobid> for comparison purposes
   */

  if ((SJ->DestinationRM == NULL) || 
      ((SJ->DestinationRM->Type != mrmtPBS) && (SJ->DestinationRM->Type != mrmtTORQUE)) || 
      ((NameEnd = strchr(SJ->Name,'.')) == NULL))
    {
    /* search the full length of the string. Add 1 extra to search beyond the
     * string if the second string is longer */

    NameEnd = SJ->Name + strlen(SJ->Name) + 1;
    }

  while (MCJobTableIterate(CJTI,&J) == SUCCESS)
    {
    if (J->Depend == NULL)
      continue;

    D = J->Depend;

    while (D != NULL)
      {
      if (D->Type != mdDAG)
        {
        /* value is job ID */

        if (!strncmp(D->Value,SJ->Name,NameEnd - SJ->Name))
          {
          if (DJP != NULL)
            *DJP = J;

          return(SUCCESS);
          }
        }

      D = D->NextAnd;
      }  /* END while (D != NULL) */
    }    /* END for (jindex) */

  return(FAILURE);
  }  /* END MJobFindDependent() */


 


/**
 * Locates a job in Moab's internal memory based on the SystemJID of the job.
 *
 * Supported enums for Mode are:
 * - mjsmBasic: search through all non-completed jobs
 * - mjsmCompleted: search through completed jobs
 *
 * @see MJobFind()
 *
 * @param SystemJID (I) The sysytem job ID to use for searching.
 * @param JP (O) Populated with the address of the found job. [optional]
 * @param Mode (I) The type of search to perform.
 * @return FAILURE if a job is not found with the given SystemJID.
 */

int MJobFindSystemJID(

  char    *SystemJID,        /* I */
  mjob_t **JP,               /* O (optional) */
  enum MJobSearchEnum Mode)  /* I */

  {
  mjob_t *J;

  job_iter JTI;

  if (SystemJID == NULL)
    {
    return(FAILURE);
    }

  switch (Mode)
    {
    case mjsmBasic:

      /* search through non-completed jobs */

      MJobIterInit(&JTI);

      while (MJobTableIterate(&JTI,&J) == SUCCESS)
        {
        if ((J->SystemJID != NULL) &&
            (!strcmp(SystemJID,J->SystemJID)))
          {
          if (JP != NULL)
            *JP = J;

          return(SUCCESS);
          }
        }  /* END for (J->Next) */

      break;

    case mjsmCompleted:

      /* search base name in MCJob[] array */

      MCJobIterInit(&JTI);
      
      while (MCJobTableIterate(&JTI,&J) == SUCCESS)
        { 
        if ((J->SystemJID != NULL) &&
            !strcmp(SystemJID,J->SystemJID))
          {
          if (JP != NULL)
            *JP = J;

          return(SUCCESS);
          }
        }    /* END for (jindex) */

      break;

    default:

      /* unsupported search mode */

      return(FAILURE);

      break;
    }

  return(FAILURE); 
  }  /* END MJobFindSystemJID() */



