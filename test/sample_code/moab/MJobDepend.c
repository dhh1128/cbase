/* HEADER */

      
/**
 * @file MJobDepend.c
 *
 * Contains job functions dealing with dependencies
 *
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 * Compute the actual name a job should have and return it.
 * The pointer will point to a portion of Name or of the value copied
 * into SJID.
 * 
 * @param J        (I) job with dependency 
 * @param Name     (I) dependency name
 * @param SJID     (O) [minsize=MMAX_LINE] Source Job ID will be copied into here
 * @param Type     (I) dependency type
 * @param OAttrOut (O) I don't know what this is. @see MJobSetDependency.
 */

char *MJobComputeDependencyName(

  mjob_t           *J,        /* I job with dependency  */
  char             *Name,     /* I dependency name */
  char             *SJID,     /* O [minsize=MMAX_LINE] Source Job ID will be copied into here */
  enum MDependEnum  Type,     /* I dependency type */
  int              *OAttrOut) /* O I don't know what this is. see @MJobSetDependency. */

  {
  char *OID;
  int OAttr = 0;

  if ((Type != mdJobSyncMaster) &&
      (Type != mdVarSet) &&
      (Type != mdVarNotSet) &&
      (Type != mdVarEQ) &&
      (Type != mdVarNE) &&
      (Type != mdVarGE) &&
      (Type != mdVarLE) &&
      (Type != mdVarLT) &&
      (Type != mdVarGT))
    {
    if (!strncmp(Name,"jobname.",strlen("jobname.")))
      {
      OID = Name + strlen("jobname.");

      OAttr = mjaJobName;
      }
    else if (!strncmp(Name,"jobid.",strlen("jobid.")))
      {
      OID = Name + strlen("jobid.");
      }
    else 
      {
      OID = Name;
      }

    if (OAttr == 0)
      MJobGetName(NULL,OID,J->SubmitRM,SJID,MMAX_LINE,mjnShortName);
    else
      MUStrCpy(SJID,OID,MMAX_LINE);
    }  /* END if ((Type != mdJobSyncMaster) && ...) */
  else
    {
    OID = Name;
    MUStrCpy(SJID,Name,MMAX_LINE);
    }

  *OAttrOut = OAttr;

  return(OID);
  }  /* END MJobComputeDependencyName() */





/**
 * Frees a dependency and alters its containing linked list accordingly.
 * Frees both the resources in ToDestroy and the mdepend_t struct itself.
 * Therefore, ToDestroy must be heap-allocated.
 * 
 * @param ToDestroy (I/O) a pointer to a heap-allocated dependency to be destroyed
 * @param Prev      (I) [optional/can be NULL] the previous link
 * @param ListHead  (I) [optional/can be NULL] a pointer to list head
 */

int MDependFree(

  mdepend_t  *ToDestroy, /* I/O a pointer to a heap-allocated dependency to be destroyed */
  mdepend_t  *Prev,      /* I [optional/can be NULL] the previous link */
  mdepend_t **ListHead)  /* I [optional/can be NULL] a pointer to list head */

  {
  if (Prev != NULL)
    Prev->NextAnd = ToDestroy->NextAnd;

  MUFree(&ToDestroy->Value);
  MUFree(&ToDestroy->SValue);

  if (ToDestroy->DepList != NULL)
    {
    /* dtor the mstring_t */
    delete ToDestroy->DepList;
    }

  if ((ListHead != NULL) && (*ListHead == ToDestroy))
    *ListHead = ToDestroy->NextAnd;

  MUFree((char **)&ToDestroy);

  return(SUCCESS);
  } /* END MDependFree() */





/**
 * Deletes a dependency from J. 
 *
 * @see MJobSetDependency()
 *
 * @param J (I) The job that will have dependency removed 
 * @param Type (I) The type of dependency which will be removed
 * @param Value (I) processed final dependency target
 * @param SValue (I) raw specified dependency target - optional
 * @param Flags (I) bitmap of enum MTrigVarAttrEnum
 */

int MJobRemoveDependency(

  mjob_t           *J,      /* I (modified) */
  enum MDependEnum  Type,   /* I (dependency type) */
  char             *Value,  /* I processed final dependency target (jobid or other, see below) */
  char             *SValue, /* I raw specified dependency target (optional) */
  mulong            Flags)  /* I (bitmap of enum MTrigVarAttrEnum) */

  {
  char SJID[MMAX_LINE+1]; /* job/object id specified within dependency */

  mdepend_t *D;
  mdepend_t *DPrev;

  int         OAttr;  /* dependency key attribute */
  char       *OID;

  const char *FName = "MJobRemoveDependency";

  MDB(5,fALL) MLog("%s(%s,%s,%s,%s,%ld)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    MDependType[Type],
    (Value != NULL) ? Value : "NULL",
    (SValue != NULL) ? SValue : "NULL",
    Flags);

  if (J == NULL)
    {
    return(FAILURE);
    }

  if (Value == NULL) 
    {
    return(FAILURE);
    }

  OAttr = 0;  /* set object key attr to object default */

  OID = MJobComputeDependencyName(J,Value,SJID,Type,&OAttr);

  /* search all existing dependencies looking for match */

  DPrev = NULL;

  for (D = J->Depend;D != NULL;D = D->NextAnd)
    {
    if ((D->Type == Type) && 
      (((Type == mdJobSyncMaster) && (D->Value != NULL) && !strcmp(D->Value,OID)) ||
       ((Type != mdJobSyncMaster) && (D->Value != NULL) && !strcmp(D->Value,SJID))))
      {
      /* match located */

      break;
      }

    DPrev = D;
    }    /* END for (D) */

  if (D == NULL)
    {
    return(FAILURE);
    }

  MDependFree(D,DPrev,&J->Depend);

  return(SUCCESS);
  }  /* END MJobRemoveDependency() */







/**
 * Synchronizes job dependencies reported by an RM with the job's moab actually has.
 *
 * @see designdoc JobDependenciesDoc
 *
 * This Should only be called at the first iteration.
 * All dependencies that cannot be found are removed.
 * All dependencies whose name varies from the job name used by Moab are 
 *   changed match Moab's job primary job name.
 *
 * @param J (I/O)
 */

int MJobSynchronizeDependencies(

  mjob_t *J) /* I/O */

  {
  mdepend_t *D = J->Depend;
  mdepend_t *Prev = NULL;

  while (D != NULL)
    {
    mjob_t *tmpJ;

    switch (D->Type)
      {
      case mdJobStart:                /* job must have previously started */
      case mdJobCompletion:           /* job must have previously completed */
      case mdJobSuccessfulCompletion: /* job must have previously completed successfully */
      case mdJobFailedCompletion:
      case mdJobBefore:               /* job cannot start until this job completes */
      case mdJobBeforeAny:            /* job cannot start until this job starts */
      case mdJobBeforeSuccessful:     /* job cannot start until this job completes successfully */
      case mdJobBeforeFailed:         /* job cannot start until this job fails */

        /* evaluate/update job id */

        /* NO-OP */

        break;

      default:

        /* dependency does not utilize job id */

        Prev = D;
  
        D = D->NextAnd;

        continue;

        break;
      }  /* END switch (D->Type) */

    if ((MJobFindDepend(D->Value,&tmpJ,NULL) == FAILURE) || (tmpJ == NULL))
      {
      mdepend_t *toDestroy = D;

      D = D->NextAnd;

      MDependFree(toDestroy,Prev,&J->Depend);
      }
    else 
      {
      if (strcmp(tmpJ->Name,D->Value))
        {
        /* depend job id does not match - update */

        MUFree(&D->Value);

        MUStrDup(&D->Value,tmpJ->Name);
        }
      
      Prev = D;

      D = D->NextAnd;
      }
    }    /* END while (D != NULL) */

  return(SUCCESS);
  }  /* END MJobSynchronizeDependencies() */





/**
 * find a dependency by type and/or name
 *
 * @param Depend     (I) linked list of dependencies, can be NULL
 * @param DependType (I) optional
 * @param DependName (I) optional
 * @param Out        (I) optional, set to NULL on FAILURE and a matching 
 *  dependency on SUCCESS
 *
 * @return SUCCESS if a dependency is found, FAILURE otherwise.
 */

int MDependFind(

  mdepend_t       *Depend,
  enum MDependEnum DependType,
  char const      *DependName,
  mdepend_t      **Out)

  {
  mdepend_t *Link;
  mbool_t UseDependType = ((DependType != mdNONE) && (DependType != mdLAST));
  mbool_t UseDependName = (DependName != NULL);
  mdepend_t *Answer = NULL;

  if (Out != NULL)
    *Out = NULL;

  if ((UseDependType == FALSE) && (UseDependName == FALSE))
    {
    return(FAILURE);
    }

  for (Link = Depend;Link != NULL;Link = Link->NextAnd)
    {
    if ((UseDependType == TRUE) && (Link->Type == DependType))
      {
      if (UseDependName == TRUE)
        {
        if (!strcmp(Link->Value,DependName))
          {
          Answer = Link;

          break;
          }
        }
      else
        {
        Answer = Link;

        break;
        }
      }
    else if ((UseDependName == TRUE) && !strcmp(Link->Value,DependName))
      {
      Answer = Link;

      break;
      }
    }

  if (Out != NULL)
    *Out = Answer;

  return((Answer == NULL) ? FAILURE : SUCCESS);
  }  /* END MJobGetDependency() */




/**
 * Expand the AName to the list of job ids for setting dependencies.
 *
 * @see MJobSetDependency() - child
 *
 * @param J           (I) The job that will have dependency applied 
 * @param AName       (I) AName to expand
 * @param Type        (I) The type of dependency which will be applied
 * @param Flags       (I) bitmap of enum MTrigVarAttrEnum 
 * @param CheckTarget (I) make sure dependency job exists 
 *
 */
int MJobSetDependencyByAName(

  mjob_t           *J,
  char             *AName,
  enum MDependEnum  Type,
  mbitmap_t        *Flags,
  mbool_t           CheckTarget)
  {
  mjob_t *tmpJ;
  mbool_t success = FAILURE;

  job_iter JTI;

  MJobIterInit(&JTI);

  while (MJobTableIterate(&JTI,&tmpJ) == SUCCESS)
    {
    if (tmpJ == J)
      continue;

    if ((tmpJ->AName != NULL) && !strcmp(AName,tmpJ->AName))
      {
      if (MJobSetDependency(
          J,
          Type,
          tmpJ->Name, /* use the job id */
          AName,      /* raw target (optional) */
          Flags,
          CheckTarget,
          NULL) == SUCCESS)
        {
        success = SUCCESS;
        }
      }
    } /* END for(jindex) */

  /* Check for completed jobs so that the job submission doesn't fail if 
   * the job is dependent on a completed job. */

  MCJobIterInit(&JTI);

  while (MCJobTableIterate(&JTI,&tmpJ) == SUCCESS)
    {
    if ((tmpJ->AName != NULL) && !strcmp(AName,tmpJ->AName))
      {
      if (MJobSetDependency(
          J,
          Type,
          tmpJ->Name, /* use the job id */
          AName,      /* raw target (optional) */
          Flags,
          CheckTarget,
          NULL) == SUCCESS)
        {
        success = SUCCESS;
        }
      }
    } /* END for(cJIndex) */

  return(success);
  } /* END MJobSetDependencyByAName() */




/**
 * This routine will set new job/variable dependencies associated with the
 * specified job.  
 *
 * No change is made to the specified job.
 * See also:  MJobCheckDependency(), MJobFindDependent(), 
 *            MJobDependDestroy(), MJobAddDependency()
 *
 * @see MJobComputeDependencyName() - child
 *
 * @param J (I) The job that will have dependency applied 
 * @param Type (I) The type of dependency which will be applied
 * @param Value (I) processed final dependency target
 * @param SValue (I) raw specified dependency target - optional
 * @param Flags (I) bitmap of enum MTrigVarAttrEnum 
 * @param CheckTarget (I) make sure dependency job exists 
 * @param EMsg (O) output message - optional
 *  
 */

int MJobSetDependency(

  mjob_t           *J,
  enum MDependEnum  Type,
  char             *Value,
  char             *SValue,
  mbitmap_t        *Flags,
  mbool_t           CheckTarget,
  char             *EMsg)
  {
  char SJID[MMAX_LINE]; /* job/object id specified within dependency */

  mdepend_t *D;
  mdepend_t *DPrev;

  mjob_t     *MJ;     /* master job */
  int         OAttr;  /* dependency key attribute */

  char       *OID;
  mjob_t     *tmpJ = NULL;

  const char *FName = "MJobSetDependency";

  MDB(5,fALL) MLog("%s(%s,%s,%s,%s)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    MDependType[Type],
    (Value != NULL) ? Value : "NULL",
    (SValue != NULL) ? SValue : "NULL");

  if (J == NULL)
    {
    return(FAILURE);
    }

  if (Value == NULL) 
    {
    return(FAILURE);
    }

  SJID[0] = '\0';
  OAttr = 0;  /* set object key attr to object default */

  OID = MJobComputeDependencyName(J,Value,SJID,Type,&OAttr);

  if ((CheckTarget == TRUE) && (MJobFindDepend(OID,&tmpJ,NULL) != SUCCESS))
    {
    MDB(4,fSTRUCT) MLog("WARNING:  unknown job name or job id in dependency '%s' specified for job %s\n",
      OID,
      J->Name);

    return(FAILURE);
    }

  /* if OID == tmpJ->AName, expand jobname to list of jobid */

  if ((tmpJ != NULL) && 
      (tmpJ->AName != NULL) &&
      (OID != NULL) && 
      !strcmp(OID,tmpJ->AName) &&
      strcmp(OID,tmpJ->Name)) /* last check avoids a recursive crash */
    {
    return(MJobSetDependencyByAName(J,OID,Type,Flags,CheckTarget));
    }

  /* detect circular dependencies */

  if (MJobCheckCircularDependencies(J,tmpJ) != SUCCESS)
    {
    if(EMsg != NULL)
      {
      snprintf(EMsg,MMAX_LINE,"ERROR:   job %s already has a dependency on job %s (circular dependency)",
        tmpJ->Name,
        J->Name);
      }

    MDB(4,fSTRUCT) MLog("WARNING:   circular dependency, job %s has a dependency on job %s\n",
      tmpJ->Name,
      J->Name);

    return(FAILURE);
    }

  /* search all existing dependencies looking for match */

  DPrev = NULL;

  for (D = J->Depend;D != NULL;D = D->NextAnd)
    {
    if ((D->Type == Type) && (OID != NULL) &&
      (((Type == mdJobSyncMaster) && (D->Value != NULL) && !strcmp(D->Value,OID)) ||
       ((Type != mdJobSyncMaster) && (D->Value != NULL) && !strcmp(D->Value,SJID))))
      {
      /* match located */

      return(SUCCESS);
      }

    /* D->Value could be jobname and OID could be jobid, so find the respective jobs and see
     * if they are the same. */

    if (D->Type == Type)
      {
      mjob_t *tmpDependJ;
      mjob_t *tmpJ;

      if (((MJobFind(OID,&tmpJ,mjsmExtended) == SUCCESS) ||
           (MJobCFind(OID,&tmpJ,mjsmExtended) == SUCCESS)) &&
          (MJobFind(D->Value,&tmpDependJ,mjsmJobName) == SUCCESS))
        {
        if (tmpJ == tmpDependJ)
          {
          return(SUCCESS);
          }
        }
      }

    DPrev = D;
    }    /* END for (D) */

  if ((D == NULL) &&
     ((D = (mdepend_t *)MUCalloc(1,sizeof(mdepend_t))) == NULL))
    {
    /* memory allocation failed */

    return(FAILURE);
    }

  /* D had to be NULL when allocated because of the for loop above,
      initialize Satisfied */

  D->Satisfied = MBNOTSET;

  if (DPrev == NULL)
    {
    J->Depend = D;
    }
  else
    {
    DPrev->NextAnd = D;
    }

  bmcopy(&D->BM,Flags);

  D->JAttr = OAttr;
 
  switch (Type)
    {
    case mdJobSyncMaster:

      D->ICount = (int)((OID != NULL) ? strtol(OID,NULL,10) : 0);

      D->Type = Type;

      MUStrDup(&D->SValue,SValue);
 
      /* if SValue is not set, the dependency is worthless */

      MUStrDup(&D->Value,OID);

      break;

    case mdJobSyncSlave:

      if (MJobFind(OID,&MJ,mjsmBasic) == FAILURE)
        {
        /* cannot locate dependency */

        MDependFree(D,DPrev,&J->Depend);

        return(FAILURE);
        }

      /* set local dependency first to avoid recursive loop */

      D->Type = Type;

      MUStrDup(&D->SValue,SValue);
 
      MUStrDup(&D->Value,SJID);

      /* add local dependency pointing to master */
      /* add job dependency to master pointing back to slave */

      if (MJobSetDependency(MJ,mdJobSyncSlave,J->Name,SValue,0,FALSE,NULL) == FAILURE)
        {
        /* most likely, dependency target cannot be located */

        MDependFree(D,DPrev,&J->Depend);

        return(FAILURE);
        }

      /* add local dependency pointing to master is added below */

      /* NOTE: slave jobs not directly scheduled, only started as side-affect 
               of launching master */

      break;

    case mdVarSet:
    case mdVarNotSet:
    case mdVarEQ:
    case mdVarNE:
    case mdVarGE:
    case mdVarLE:
    case mdVarLT:
    case mdVarGT: 

      {
      char *ptr;
      char *TokPtr = NULL;

      D->Type = Type;

      ptr = MUStrTok(OID,"| \n\t",&TokPtr);

      MUStrDup(&D->Value,ptr);

      if ((TokPtr != NULL) && (TokPtr[0] != '\0'))
        {
        MUStrDup(&D->SValue,TokPtr);
        }
      }  /* END case mdVarSet */

      break;

    default:

      D->Type = Type;

      MUStrDup(&D->SValue,SValue);
 
      /* if SValue is not set, the dependency is worthless */

      /* should we change this to SJID? */
      /* verify it won't break anything first */

      MUStrDup(&D->Value,SJID);

      break;
    }  /* END switch (Type) */

  return(SUCCESS);
  }  /* END MJobSetDependency() */





/**
 * Free/destroy job dependency
 *
 * @param J (I) [modified]
 *
 * @see MJobDestroy() - parent
 * @see MQueueUpdateCJobs() - parent
 * @see MDependFree() - child
 * @see MJobDependCopy() - peer
 * @see MJobSetDependency() - peer
 */

int MJobDependDestroy(

  mjob_t *J)  /* I (modified) */

  {
  mdepend_t *D;
  mdepend_t *DNext;

  if (J == NULL)
    {
    return(FAILURE);
    }

  DNext = NULL;

  for (D = J->Depend;D != NULL;D = DNext)
    {
    DNext = (mdepend_t *)D->NextAnd; 

    MDependFree(D,NULL,NULL);
    }  /* END for (D) */

  J->Depend = NULL;

  return(SUCCESS);
  }  /* END MJobDependDestroy() */





/**
 * Perform 'deep' copy of job dependency.
 *
 * @param SrcDepend (I) 
 * @param DstDependP (O) [alloc]
 *
 * @see MQueueAddCJob() - parent
 * @see MJobDependDestroy() - peer
 *
 * NOTE:  NextOr list not yet supported.
 */

int MJobDependCopy(

  mdepend_t  *SrcDepend,  /* I */
  mdepend_t **DstDependP) /* O (alloc) */

  {
  mdepend_t *SrcD;
  mdepend_t *DstD;
  mdepend_t *D;

  if ((SrcDepend == NULL) || (DstDependP == NULL))
    {
    return(FAILURE);
    }

  /* copy dependency */

  DstD = NULL;
  
  for (SrcD = SrcDepend;SrcD != NULL;SrcD = SrcD->NextAnd)
    {
    if ((D = (mdepend_t *)MUCalloc(1,sizeof(mdepend_t))) == NULL)
      {
      /* memory allocation failed */

      return(FAILURE);
      }

    D->Satisfied = MBNOTSET;

    if (DstD == NULL)
      *DstDependP = D;
    else
      DstD->NextAnd = D;

    DstD = D;

    memcpy(DstD,SrcD,sizeof(mdepend_t));

    DstD->NextAnd = NULL;
    DstD->NextOr = NULL;

    DstD->Value = NULL;
    DstD->SValue = NULL;

    MUStrDup(&DstD->Value,SrcD->Value);
    MUStrDup(&DstD->SValue,SrcD->SValue);

    if (SrcD->DepList != NULL)
      {
      /* Create a new Dest mstring_t */
      DstD->DepList = new mstring_t(MMAX_LINE);

      /* copy the info from src to the new dest */
      *DstD->DepList = *SrcD->DepList;
      }
    }  /* END for (SrcD) */

  return(SUCCESS);
  }  /* END MJobDependCopy() */






/**
 * This routine will check all job/variable dependencies associated with the
 * specified job.  Its output parameters will indicate the first dependency
 * which cannot be satisfied.
 *
 * @see MJobDiagnoseEligibility() - parent - report job's execution eligibility
 * @see MQueueSelectJobs() - parent
 * @see MJobSetDepend() - peer
 *
 * No change is made to the specified job.
 * See also:  MJobSetDependency(), MJobFindDependent(), MJobDependDestroy()
 *
 * @param SJ (I) The job that will be evaluated 
 * @param Type (O) The type of dependency which was not satisfied [optional]
 *   set to mdNONE on SUCCESS
 * @param DoVerbose (I) If FALSE, Value parameter contains the object id of the 
 *   first failed dependency, if TRUE, Value paramter contains list of all 
 *   dependencies and the dependency status
 * @param RemoveJob (O) [optional] dependency can never be satisfied - job should
 *   be removed
 * @param Value (O) dependency details. Must be MStringInit'ed prior to call.
 * 
 * @return SUCCESS if all job dependencies are satisfied.  If not all 
 *         dependencies are satisfied, return FAILURE and set Type and Value
 *         parameters.
 */

int MJobCheckDependency(
 
  mjob_t           *SJ,         /* I (specified job) */
  enum MDependEnum *Type,       /* O depend type which was not satisfied (optional) */
  mbool_t           DoVerbose,  /* I */
  mbool_t          *RemoveJob,  /* O (optional) */
  mstring_t        *Value)      /* O depend object which was not satisfied (optional,must be MStringInit'ed' prior) */
 
  {
  mjob_t *J = NULL;

  mdepend_t *D;
 
  mbool_t  DependSatisfied = FALSE;  /* initialized to avoid compiler warnings */

  int ReqSlaveJobCount;
  int NumSyncJobsFound;
  enum MDependEnum Unsatisfied = mdNONE;
  char const *UnsatisfiedName = "";

  int      VarFlags[16];

  const char *FName = "MJobCheckDependency";
 
  MDB(8,fSCHED) MLog("%s(%s,Type,%s,RemoveJob,%s)\n",
    FName,
    (SJ != NULL) ? SJ->Name : "NULL",
    MBool[DoVerbose],
    (Value != NULL) ? "Value" : "NULL");

  if (Type != NULL)
    *Type = mdNONE;

  if (SJ == NULL)
    {
    return(FAILURE);
    }

  if (RemoveJob != NULL)
    *RemoveJob = FALSE;

  memset(VarFlags,0,sizeof(VarFlags));

  D = SJ->Depend;

  /* NOTE:  handle only single job dependency per job for now */

  /* NOTE:  verbose format:  <TYPE>:<OBJECT>:<STATUS>[,<TYPE>:<OBJECT>:<STATUS>]... */
  /* NOTE:  assume Value of size MMAX_LINE if verbose requested */

  if ((D == NULL) || (D->Type == mdNONE) || (D->Value == NULL))
    {
    return(SUCCESS);
    }

  if ((DoVerbose == TRUE) && (Value == NULL))
    return(FAILURE);

  /* NOTE:  LL31 job steps no longer have default prereq dependency */

  ReqSlaveJobCount = 0;
  NumSyncJobsFound = 0;
 
  while (D != NULL)
    { 
    DependSatisfied = FALSE;

    /* job dependency specified */

    if (D->Satisfied == TRUE)
      {
      /* This dependency has already been fulfilled, go to next one */

      D = D->NextAnd;
      continue;
      }

    if (D->Satisfied == FALSE)
      {
      /* This dependency has already failed, no chance for this job to run */

      Unsatisfied = D->Type;
      UnsatisfiedName = D->Value;

      if (RemoveJob != NULL)
        *RemoveJob = TRUE;

      break;
      }

    if (D->Type != mdDAG)
      {
      int jindex;

      enum MJobSearchEnum SearchTypes[] = {
        mjsmExtended,
        mjsmJobName,
        mjsmCompleted,
        mjsmLAST};

      for (jindex = 0;SearchTypes[jindex] != mjsmLAST;jindex++)
        {
        if (MJobFind(D->Value,&J,SearchTypes[jindex]) == SUCCESS)
          {
          /* matching job located */

          break;
          }
        else if (MJobCFind(D->Value,&J,SearchTypes[jindex]) == SUCCESS)
          {
          /* matching job located */

          break;
          }
        }
      }

    switch (D->Type)
      {
      case mdJobStart:

        if (J == NULL)
          {
          /* cannot located requested job */

          MJobProcessMissingDependency(D->Type,
            SJ,
            DoVerbose,
            &DependSatisfied);

          break;
          }

        if ((MJOBISACTIVE(J) != TRUE) && 
            (MJOBISCOMPLETE(J) != TRUE)) 
          {
          /* job is not active or completed */

          break;
          }

        if (J->StartTime + D->Offset > MSched.Time)
          {
          break;
          }

        DependSatisfied = TRUE;
      
        break;
 
      case mdJobCompletion:

        if (J == NULL)
          {
          /* cannot locate requested job */

          MJobProcessMissingDependency(D->Type,
            SJ,
            DoVerbose,
            &DependSatisfied);
 
          break;
          }

        if (MJOBISCOMPLETE(J) != TRUE)
          {
          break;
          }

        if (J->CompletionTime + D->Offset > MSched.Time)
          {
          break;
          }

        DependSatisfied = TRUE;

        break;

      case mdJobSuccessfulCompletion:

        if (J == NULL)
          {
          MJobCFind(D->Value,&J,mjsmJobName);
          }

        if (J == NULL)
          {
          /* cannot locate requested job */

          MJobProcessMissingDependency(D->Type,
            SJ,
            DoVerbose,
            &DependSatisfied);

          break;
          }
        else if (MJOBISCOMPLETE(J) && (J->CompletionCode != 0))
          {
          /* job failed - dependency can never be satisfied */

          if (RemoveJob != NULL)
            *RemoveJob = TRUE;

          D->Satisfied = FALSE;

          break;
          }

        if (MJOBISSCOMPLETE(J) != TRUE)
          {
          break;
          }

        if (J->CompletionTime + D->Offset > MSched.Time)
          {
          break;
          }

        DependSatisfied = TRUE;

        break;

      case mdJobFailedCompletion:
 
        if (J == NULL)
          {
          MJobCFind(D->Value,&J,mjsmJobName);

          if (J == NULL)
            MJobCFind(D->Value,&J,mjsmExtended);
          }

        if (J == NULL)
          {
          /* cannot locate requested job */

          MJobProcessMissingDependency(D->Type,
            SJ,
            DoVerbose,
            &DependSatisfied);
          
          break;
          }
        else if (MJOBISCOMPLETE(J) && (J->CompletionCode == 0))
          {
          /* job succeeded - dependency can never be satisfied */

          if (RemoveJob != NULL)
            *RemoveJob = TRUE;

          D->Satisfied = FALSE;

          break;
          }

        if (MJOBISCOMPLETE(J) != TRUE)
          {
          break;
          }

        if (J->CompletionTime + D->Offset > MSched.Time)
          {
          break;
          }

        DependSatisfied = TRUE;

        break;

      case mdJobBefore:
      case mdJobBeforeAny:
      case mdJobBeforeSuccessful:
      case mdJobBeforeFailed:

        /* This job must run before another job, this one can run */
        /* Set the associated dependency on the other job */

        DependSatisfied = TRUE;

        break;

      case mdJobOnCount:

        if (D->Value != NULL)
          {
          int BeforeNeeded;
          int BeforeSatisfied = 0;
          job_iter JTI;
          job_iter CJTI;
          mjob_t *JChild = NULL;
          char *UsedJobs = NULL;  /* Job dependencies that have already been recorded */
          mbool_t FreeUsedJobs = TRUE;

          BeforeNeeded = strtol(D->Value,NULL,10);

          if (BeforeNeeded == 0) /* Error in strtol */
            {
            break;
            }

          if (D->DepList == NULL)
            {
            /* Use DepList to hold a comma separated list of job names.  Must keep 
               track of how many dependencies have been satisfied because a job 
               may finish and be purged before the next dependency is filled */

            D->DepList = new mstring_t(MMAX_LINE);

            D->ICount = BeforeNeeded;  /* No dependencies filled yet */
            }

          if (D->ICount <= 0)
            {
            DependSatisfied = TRUE;

            break;
            }

          MUStrDup(&UsedJobs,D->DepList->c_str()); /* Copy the names string over */

          if ((UsedJobs == NULL) || (UsedJobs[0] == '\0'))
            FreeUsedJobs = FALSE;

          /* Count satisfied before dependencies */

          MJobIterInit(&JTI);
          MCJobIterInit(&CJTI);

          while (MJobFindDependent(SJ,&JChild,&JTI,&CJTI) == SUCCESS)
            {
            mdepend_t *tmpD;
            mjob_t *tmpJobName;

            if ((FreeUsedJobs == TRUE) &&
                (MUStrRemoveFromList(UsedJobs,JChild->Name,',',FALSE) != FAILURE))
              {
              /* This job dependency has already been fulfilled and recorded */

              continue;
              }

            /* Check dependency on JChild */
            tmpD = JChild->Depend;

            while (tmpD != NULL)
              {
              if ((MJobFind(tmpD->Value,&tmpJobName,mjsmJobName) == SUCCESS) &&
                  (tmpJobName == SJ))
                {
                if (tmpD->Type == mdJobBefore)
                  {
                  if (!MJOBISCOMPLETE(JChild) &&
                      !MJOBISACTIVE(JChild))
                    {
                    tmpD = tmpD->NextAnd;

                    continue;
                    }

                  BeforeSatisfied++;
                  *D->DepList += ',';
                  *D->DepList += JChild->Name;

                  tmpD = NULL;
                  continue;
                  }

                /* Checking for before dependencies - if the job has not finished, the dependency is unfulfilled. */
                if (!MJOBISCOMPLETE(JChild))
                  {
                  tmpD = tmpD->NextAnd;

                  continue;
                  }

                if (tmpD->Type == mdJobBeforeAny)
                  {
                  BeforeSatisfied++;
                  *D->DepList += ',';
                  *D->DepList += JChild->Name;

                  tmpD = NULL;
                  continue;
                  }
                else if (tmpD->Type == mdJobBeforeSuccessful)
                  {
                  if (JChild->CompletionCode == 0)
                    {
                    BeforeSatisfied++;
                    *D->DepList += ',';
                    *D->DepList += JChild->Name;

                    tmpD = NULL;
                    continue;
                    }
                  }
                else if (tmpD->Type == mdJobBeforeFailed)
                  {
                  if (JChild->CompletionCode != 0)
                    {
                    BeforeSatisfied++;
                    *D->DepList += ',';
                    *D->DepList += tmpJobName->Name;

                    tmpD = NULL;
                    continue;
                    }
                  }
                }

              tmpD = tmpD->NextAnd;
              }
            }

          D->ICount -= BeforeSatisfied;

          if (D->ICount <= 0)
            {
            DependSatisfied = TRUE;
            }

          if ((UsedJobs != NULL) && (FreeUsedJobs == TRUE))
            MUFree(&UsedJobs);
          } /* END BLOCK mdJobOnCount */

        break;

      case mdJobSyncMaster:

        ReqSlaveJobCount = D->ICount;

        DependSatisfied = TRUE;

        UnsatisfiedName = D->Value;

        break;

      case mdJobSyncSlave:

        NumSyncJobsFound++;

        DependSatisfied = TRUE;

        if (ReqSlaveJobCount == 0)
          {
          /* this job is a slave job to a master job, and we need the name of
             the master job if we fail because the master job isn't running */

          UnsatisfiedName = D->Value;
          }

        if (Type != NULL)
          *Type = D->Type;

        if (DoVerbose == FALSE)
          { 
          if ((Value != NULL) && (D->Value != NULL))
            {
            MStringSet(Value,D->Value);
            }
          }

        break;

      case mdDAG:

        {
        /* check if DAG dependencies are satisfied */

        /* NYI */
        }  /* END BLOCK */

      case mdVarSet:
      case mdVarNotSet:
      case mdVarEQ:
      case mdVarNE:
      case mdVarGE:
      case mdVarLE:
      case mdVarLT:
      case mdVarGT:

        {
        mhash_t *tHTs[MMAX_NAME];
        int     tHTFlags[MMAX_NAME];

        mhash_t **HTs;
        int      *HTFlags;

        int HTIndex;

        HTs = tHTs;
        HTFlags = tHTFlags;

        memset(HTs,0,sizeof(tHTs));
        memset(HTFlags,0,sizeof(tHTFlags));

        /* NOTE:  populate var with all accessible variable arrays (NYI) */

        HTIndex = 0;

        if (SJ->Variables.Table != NULL)
          {
          HTs[HTIndex++] = &SJ->Variables;
          }

        if (SJ->JGroup != NULL)
          {
          J = NULL;

          if (MJobFind(SJ->JGroup->Name,&J,mjsmExtended) == SUCCESS)
            {
            HTFlags[HTIndex] = 1;
            HTs[HTIndex++] = &J->Variables;
            }
          }

        if (SJ->Depend != NULL)
          {
          if (MDAGCheckMultiWithHT(NULL,SJ->Depend,(mhash_t **)HTs,HTFlags,0,NULL,SJ->ParentVCs) == FAILURE)
            {
            if (Type != NULL)
              *Type = D->Type;

            return(FAILURE);
            }
          else
            {
            /* Need to set in case successful rc was not from trigger vars */

            DependSatisfied = TRUE;

            break;
            }
          }
       
        DependSatisfied = TRUE;
        }  /* END BLOCK (case mdVarSet) */

        break;

      default:

        /* NO-OP */

        break;
      }  /* END switch (D->Type) */

    if (DoVerbose == TRUE)
      {
      if (!Value->empty())
        MStringAppend(Value,",");

      MStringAppendF(Value,"%s:%s:%s",
        MDependType[D->Type],
        D->Value,
        (DependSatisfied == TRUE) ? "TRUE" : "FALSE");      
      }

    if (DependSatisfied == FALSE)
      {
      Unsatisfied = D->Type;
      UnsatisfiedName = D->Value;

      if (DoVerbose == FALSE)
        break;
      }  /* END if (DependSatisfied == FALSE) */

    if (DependSatisfied == TRUE)
      {
      D->Satisfied = TRUE;

      /* remove satisfied dependencies from the cached object */
      MJobTransition(SJ,FALSE,FALSE);
      }

    D = D->NextAnd;
    }  /* END while (D != NULL) */

  if ((ReqSlaveJobCount > 0) && 
      (NumSyncJobsFound < ReqSlaveJobCount))
    {
    /* master job does not yet have enough slave jobs */

    Unsatisfied = mdJobSyncMaster;
    }
  else if ((ReqSlaveJobCount == 0) && (NumSyncJobsFound > 0))
    {
    /* job is a slave and cannot run before its master */

    Unsatisfied = mdJobSyncSlave;
    }

  if (DependSatisfied == MBNOTSET)
    {
    return(FAILURE);
    }

  if (Unsatisfied != mdNONE)
    {
    /* dependency not satisfied */

    if (Type != NULL)
      *Type = Unsatisfied;

    if (DoVerbose == FALSE)
      { 
      if (Value != NULL)
        {
        MStringSet(Value,(char *)UnsatisfiedName);
        }

      return(FAILURE);
      }
    }

  /* all dependencies satisfied (or verbose output requested) */
 
  return(SUCCESS);
  }  /* END MJobCheckDependency() */





/**
 * perform the needed operations so that MasterJob and slave job run at the 
 *  same time The "synccount" dependency on MasterJob is incremented if it 
 *  already exists, otherwise it is set to 1.
 *  
 * @param MasterJob (I) 
 * @param SlaveJob  (I)
 */

int MJobSetSyncDependency(

  mjob_t *MasterJob,
  mjob_t *SlaveJob)

  {
  mdepend_t *SyncCount;

  MJobSetDependency(SlaveJob,mdJobSyncSlave,MasterJob->Name,NULL,0,FALSE,NULL);

  if (MDependFind(
        MasterJob->Depend,
        mdJobSyncMaster,
        NULL,
        &SyncCount) == FAILURE)
    {
    SyncCount = (mdepend_t *)MUCalloc(1,sizeof(SyncCount[0]));
    SyncCount->Satisfied = MBNOTSET;
    SyncCount->Type = mdJobSyncMaster;
    SyncCount->NextAnd = MasterJob->Depend;
    MUStrDup(&SyncCount->Value,"1");

    /* The synccount dependency must show up first in the list, other code 
     * depends on it */
    MasterJob->Depend = SyncCount;
    }
  else
    {
    long CurVal = strtol(SyncCount->Value,NULL,10);
    /* synccount dependency should always be first! */
    assert(SyncCount == MasterJob->Depend);

    assert(CurVal >= 0);

    CurVal += 1;

    MUFree(&SyncCount->Value);
    SyncCount->Value = MUStrFormat("%ld",CurVal);
    }

  return(SUCCESS);
  } /* END MJobSetSyncDependency() */



/**
 * Add data/job dependency to job.
 *
 * @see MJobProcessExtensionString() - parent
 *
 * @param J (I) [modified]
 * @param Depend (I) job dependency
 */

int MJobAddDependency(

  mjob_t *J,       /* I (modified) */
  char   *Depend)  /* I job dependency */

  {
  mbitmap_t Flags;

  char *TokPtr;

  enum MDependEnum DType;

  char *DTPtr;
  char *DValue;
  char *DNext;

  char *DPtr;  /* current full dependency line */
  char *DPTokPtr = NULL;

  char *tmpTok;

  mbool_t DoStrictCheck = FALSE;

  if ((J == NULL) || (Depend == NULL))
    {
    return(FAILURE);
    }

  /* FORMAT: [<TYPE>:]{<VALUE>|<INSTANCECOUNT>}[@<HOSTNAME>][:<VALUE>...][{,|&&}[<TYPE>:]{<VALUE>|<INSTANCECOUNT>}[@<HOSTNAME>]]... */

  /*  Here are a few examples of dependency submission formats */
  /*  "-l depend=290:291" */
  /*  "-l depend=afterok:290" */
  /*  "-l depend=afterok:290:291" */
  /*  "-l depend=host1.123:host1.456" */
  /*  "-l depend=testjob1:testjob2" */
  /*  "-l depend=afterany:testjob1:testjob2" */
  /*  "-l depend=afterany:1234:testjob1:testjob2" */
  /*  "-l depend=afterany:290:291\&afterok:292" */

  /*  If string contains an "&" then it has multiple dependency lists - split up the lists and process them separately */

  char *mutableString=NULL;
  MUStrDup(&mutableString,Depend);

  DPtr = MUStrTok(mutableString,"&,",&DPTokPtr);

  while (DPtr != NULL)
    {
    int index = 0;

    DType  = mdNONE;
    DValue = NULL;
    DNext  = NULL;

    bmclear(&Flags);

    /* NOTE:  strip off '@<HOSTNAME>' option - not used */

    DPtr = MUStrTok(DPtr,"@",&TokPtr);

    /* parse dependency type and dependency objects */

    if ((DTPtr = MUStrTok(DPtr,": \t\n",&TokPtr)) == NULL)
      {
      MDB(4,fSTRUCT) MLog("WARNING:  mal-formed dependency '%s' specified for job %s",
        DPtr,
        J->Name);

      MUFree(&mutableString);
      return(FAILURE);
      }

    /* NOTE:  strip off '@<HOSTNAME>' option - not used */

    DValue = MUStrTok(NULL,": @\t\n",&TokPtr);

    if ((DValue != NULL) && (!strcmp(DValue,J->Name)))
      {
      MDB(4,fSTRUCT) MLog("ERROR:    job '%s' cannot be dependent on itself",
        J->Name);
      
      MUFree(&mutableString);
      return(FAILURE);
      }

    if (strchr(DTPtr,'*') != NULL)
      {
      bmset(&Flags,mdbmImport);

      DTPtr = MUStrTok(DTPtr,"*",&tmpTok);
      }

    /* check if depend type is unsupported */

    while (MSched.UnsupportedDependencies[index] != NULL)
      {
      if ((strlen(DTPtr) == strlen(MSched.UnsupportedDependencies[index])) && 
          !strncmp(DTPtr,MSched.UnsupportedDependencies[index],strlen(DTPtr)))
        {
        /* DTPtr is an unsupported depend type */

        MDB(7,fSTRUCT) MLog("ERROR:    depend type '%s' is in the unsupported list in the config file",
            DTPtr);

        MUFree(&mutableString);
        return(FAILURE);
        }
      index++;
      }

    /* if DTPtr starts with a number then it must be a job id so don't bother
     * looking for a matching depend type */

    if (isalpha(*DTPtr))
      {
      MJobParseDependType(DTPtr,mrmtNONE,mdNONE,&DType);
      }

    /* If DType is mdNONE then the first value in the dependency string was
     * not a dependency type, so assume it is a jobid or a job name */

    if (DType == mdNONE)
      {
      /* allow format: <JOBID> */

      DType = mdJobCompletion;

      DNext  = DValue;

      DValue = DTPtr;
      }

    /* Note - if the dependency type was not recognized (e.g. frog:1234) then we are assuming that frog is a job id and not a dependency type.
              when we attempt to set the dependency using the job name of "frog" then job "frog" will not be found and there may be issues
              with error handling if we think the dependent job "frog" is completed or missing. */

    switch (DType)
      {
      case mdVarEQ:
      case mdVarNE:
      case mdVarLT:
      case mdVarLE:
      case mdVarGE:
      case mdVarGT:
      case mdVarSet:
      case mdVarNotSet:

        DoStrictCheck = FALSE;

        break;

      default:

        DoStrictCheck = (bmisset(&J->IFlags,mjifSubmitEval)) ? TRUE : FALSE;
        
        break;
      } /* END switch(DType) */

    if (MJobSetDependency(
          J,
          DType,
          DValue,
          NULL,
          &Flags,
          DoStrictCheck,
          NULL) != SUCCESS)
      {
      /* Fail submission of job. */

      MDB(4,fSTRUCT) MLog("INFO:     cannot create requested dependency '%s' for job %s",
        DPtr,
        J->Name);
        
      MUFree(&mutableString);
      return(FAILURE);
      }

    /* any tokenized entries after this should be job ids submitted with the same dependency type */

    if (DNext != NULL)
      DValue = DNext;
    else
      DValue = MUStrTok(NULL,": @\t\n",&TokPtr);

    while (DValue != NULL)
      {
      if (MJobSetDependency(J,DType,DValue,NULL,&Flags,DoStrictCheck,NULL) == FAILURE)
        {
        MDB(4,fSTRUCT) MLog("INFO:     cannot create requested dependency '%s' for job %s",
          DPtr,
          J->Name);
      
        MUFree(&mutableString);
        return(FAILURE);
        }

      DValue = MUStrTok(NULL,": @\t\n",&TokPtr);
      }  /* END while (DValue != NULL) */

    DPtr = MUStrTok(NULL,"&,",&DPTokPtr);
    }      /* END while (DPtr != NULL) */

  MUFree(&mutableString);
  return(SUCCESS);
  }  /* END MJobAddDependency() */





#ifdef MNOT
 /* BOC 11/24/09 - This function was created to ensure that job dependencies were pushed
  * down to the resource manager when they weren't specified in the SubmitString.
  * It was only used for torque in MPBSJobSubmit. The placement of the  function was added 
  * after the CommandBuffer was created, but should have been placed in the section of code 
  * that only deals with the scenario where a SubmitString wasn't available (ex. only XML 
  * was passed to msub). Since the creation of this function there was a simpler and more 
  * concise function which is being used in its place -- MDependToString. MDependToString 
  * by default doesn't check the validity of the dependency string, which 
  * MJobDependencyToString began to do. Not doing the validity check is fine because moab 
  * is putting the dependency string in -l and not -W. Torque doesn't care what is in -l
  * depend, but does for -W. Removing MJobDependencyToString and adding MDependToString to
  * only add the the dependency string only if the submitstring isn't available, should not
  * effect behavior since moab will just pass what was passed on from the client or what 
  * MDependString prints out. MPBSJobSubmit won't fail because of bad dependencies. Invalid 
  * dependencies should be handled when moab reads them in in MJobAddDependency. */

/**
 * Returns a dependency string for all of the job's dependencies.
 *
 * NOTE:  string will be properly formatted to be used as a RM extension
 *        'depend' request.
 *
 * NOTE:  DependStr should be UNINITIALIZED, as it will be initialized here
 *
 * @see MPBSJobSubmit() - parent
 *
 * @param J Job with dependencies.
 * @param RMType to mold dependency string as.
 * @param DependStr String to return the dependency string.
 * @param PrintSystemJobs specify whether to print system jobs as well
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 */

int MJobDependencyToString(

  mjob_t           *J,         /* I */
  enum MRMTypeEnum  RMType,    /* I */
  mstring_t        *DependStr, /* O */
  mbool_t           PrintSystemJobs, /* I */
  char             *EMsg)      /* O (optional,minsize=MMAX_LINE) */

  {
  char *jobId;

  mdepend_t *D;
  mjob_t    *tmpJ;

  mbool_t    FoundJob = FALSE;
  mbool_t    DependRequiresJob = FALSE;  /* dependency currently being evaluated requires job id */

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if ((J == NULL) ||
      (DependStr == NULL) ||
      (J->Depend == NULL))
    {
    if (EMsg != NULL)
      {
      strcpy(EMsg,"invalid dependency");
      }

    return(FAILURE);
    }

  /* Call constructor on already alloced memory */
  new (DependStr) mstring_t(MMAX_LINE);

  for (D = J->Depend;D != NULL;D = D->NextAnd)
    {
    tmpJ = NULL;

    DependRequiresJob = TRUE;

    switch (D->Type)
      {
      case mdJobSyncMaster:
      case mdJobOnCount:
      case mdVarEQ:

        /* these kinds of dependencies don't have a job ID
         * as their D->Value */

        FoundJob = TRUE;  /* keep from failing out of function */

        DependRequiresJob = FALSE;

        break;

      default:

        /*NO-OP*/

        break;
      }

    if (DependRequiresJob == TRUE)
      {
      if ((MJobFind(D->Value,&tmpJ,NULL,mjsmJobName) == FAILURE) &&
          (MJobCFind(D->Value,&tmpJ,mjsmJobName) == FAILURE))
        {
        /* NOTE:  if dependency is one of the completion types, will we only
                  get here if completion is satisfied and thus should we
                  not export the dependency when submitting the job? */

        if ((strstr(D->Value,"vmcreate") != NULL) || 
            (strstr(D->Value,"vmdestroy") != NULL) ||
            (strstr(D->Value,"provision") != NULL) ||
            (strstr(D->Value,"vmmigrate") != NULL))
          {
          FoundJob = TRUE;
          
          continue;
          }

        /* cannot locate needed job */

        /* NOTE:  should this be a failure? */

        continue;
        }

      if ((tmpJ->System != NULL) && (PrintSystemJobs == FALSE))
        {
        /* dependency is tied to internal system job not visible outside of Moab */

        /* do not display */

        FoundJob = TRUE;

        continue;
        } 
      }
 
    FoundJob = TRUE;

    MStringSet(DependStr,"depend=");
    
    /* PBS uses jobid's not job names */

    jobId = tmpJ->Name;

    switch (RMType)
      {
      case mrmtPBS:

        {
        switch (D->Type)
          {
          case mdJobStart:

            MStringAppendF(DependStr,"after:%s",
              jobId);

            break;

          case mdJobCompletion:

            MStringAppendF(DependStr,"afterany:%s",
              jobId);
                
            break;

          case mdJobSuccessfulCompletion:

            MStringAppendF(DependStr,"afterok:%s",
              jobId);
                
            break;

          case mdJobFailedCompletion:

            MStringAppendF(DependStr,"afternotok:%s",
              jobId);
                
            break;

          case mdJobSyncMaster:

            MStringAppendF(DependStr,"synccount:%s",
              D->Value);

            break;

          case mdJobSyncSlave:

            MStringAppendF(DependStr,"syncwith:%s",
              D->Value);

            break;

          case mdJobOnCount:

            MStringAppendF(DependStr,"on:%s",
              D->Value);

            break;

          case mdJobBefore:

            MStringAppendF(DependStr,"before:%s",
              D->Value);

            break;

          case mdJobBeforeAny:

            MStringAppendF(DependStr,"beforeany:%s",
              D->Value);

            break;

          case mdJobBeforeSuccessful:

            MStringAppendF(DependStr,"beforeok:%s",
              D->Value);
  
            break;

          case mdJobBeforeFailed:

            MStringAppendF(DependStr,"beforenotok:%s",
              D->Value);
  
            break;

          default:

            /* place an error string in DependStr and return failure */
            /* clear whatever is in the buffer to this point */

            if (EMsg != NULL)
              {
              sprintf(EMsg,"one or more of the dependency types is not supported: %s",
                MDependType[D->Type]);
              }

            /* Will segfault on job cancel if this is not freed now */

            DependStr->~mstring_t();

            return(FAILURE);

            /*NOTREACHED*/

            break;
          } /* END switch (D->Type) */
        }   /* END BLOCK (case mrmtPBS) */

        break;

      default:

        /* NYI */

        break;
      }  /* END switch (RMType) */

    if (D->NextAnd != NULL)
      MStringAppend(DependStr,",depend=");
    }  /* END for (D = J->Depend;D != NULL;D = D->NextAnd) */

  if (FoundJob == FALSE)
    {
    /* couldn't find matching jobs */

    if (EMsg != NULL)
      {
      snprintf(EMsg,MMAX_LINE,"could not locate %s dependency target when exporting to RM %s",
        (D != NULL) ? MDependType[D->Type] : "",
        MRMType[RMType]);
      }

    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MJobDependencyToString() */
#endif




/**
 * Check to make sure the source job is not found as a dependent job among the jobs 
 * that it depends on or any of the jobs which those jobs depend on.
 *
 * @param SrcJ     (I)
 * @param DependJ  (I)
 */

int MJobCheckCircularDependencies(

  mjob_t *SrcJ, 
  mjob_t *DependJ)

  {
  mjob_t *tmpJ = NULL;
  mdepend_t *D;

  /* if we have exhausted all dependency checks and we are done */

  if ((DependJ == NULL) || (SrcJ == NULL))
    return(SUCCESS);

  /* Make sure this dependency is not self referential. */
  if (SrcJ == DependJ)
    {
    return(FAILURE);
    }
  
  for (D = DependJ->Depend; D != NULL; D = D->NextAnd)
    {
    MJobFind(D->Value,&tmpJ,mjsmExtended);

    MDB(7,fPBS) MLog("INFO:     checking to see if %s depends on %s\n",
      tmpJ->Name,
      SrcJ->Name);

    if (tmpJ == NULL)
      continue;

    if ((tmpJ == SrcJ) ||   /* tmpJ depends on SrcJ */
        (tmpJ == DependJ))  /* tmpJ depends on itself???
                               Shouldn't happen, but if it does,
                               it would result in an endless loop. */
      {
      /* source job found as a dependent job */

      return(FAILURE);
      }

    if (MJobCheckCircularDependencies(SrcJ,tmpJ) != SUCCESS)
      {
      return(FAILURE);
      }
    } /* END For loop */

  return(SUCCESS);
  } /* END MJobCheckCircularDependencies() */





/**
 * Checks the action for a missing dependency, based on the dependency type
 * passed in, and marks the given boolean value as to whether the
 * dependency should be satisfied or not
 *
 * @param DependencyType (I)
 * @param DependentJob (I) (may be modified)
 * @param DoVerbose (I) (set if we're inside checkjob/showq)
 * @param DependencySatisfied (O)
 *
 * NOTE:  In the case that this is called during a scheduling iteration the
 *        dependent job may be canceled or held, depending on the current
 *        missing dependency action
 */

int MJobProcessMissingDependency(

  enum MDependEnum  DependencyType,       /* (I) The type of dependency on the job */
  mjob_t           *DependentJob,         /* (I) The dependent job */
  mbool_t           DoVerbose,            /* (I) */
  mbool_t          *DependencySatisfied)  /* (O) Whether we should mark the dependency as satisfied */

  {

  if (DependentJob == NULL)
    {
    return(FAILURE);
    }

  /* if the dependent job is already running, we'll assume Moab did the right
   * thing when it started it. */

  if (DependentJob->State == mjsRunning)
    {
    return(SUCCESS);
    }

  /* check the missing dependency action and take appropriate action */

  switch (MSched.MissingDependencyAction)
    {

    case mmdaAssumeSuccess:

      /* assume the job finished successfully, and set the dependency
       * satisified on the job appropriately as to the dependency type */

      MDB(3,fSCHED) MLog("INFO:     Cannot find dependency job. Assuming successful completion.\n");

      switch (DependencyType)
        {

        /* NOTE:  assuming that the job finished successfully means we don't
         *        want beforenotok and afternotok dependencies to run */

        case mdJobFailedCompletion:

          *DependencySatisfied = FALSE;

          break;

        default:

          /* NOTE:  I think we want anything else to run if we're assuming
           *        the dependency job finished successfully */

          *DependencySatisfied = TRUE;

          break;

        } /* END switch (DependencyType) */

      break;

    case mmdaCancel:

      *DependencySatisfied = FALSE;

      MDB(3,fSCHED) MLog("WARNING:  Cannot find dependency job. MissingDependencyAction is set to CANCEL.\n");

      if (!DoVerbose)
        {
        bmset(&DependentJob->IFlags, mjifCancel);
        }

      break;

    case mmdaHold:

      *DependencySatisfied = FALSE;

      MDB(3,fSCHED) MLog("WARNING:  Cannot find dependency job. MissingDependencyAction is set to HOLD.\n");

      if (!DoVerbose)
        {
        MJobSetHold(DependentJob,mhBatch,MMAX_TIME,mhrMissingDependency,"missing dependency job");
        }

      break;
  
    case mmdaRun:
  
      *DependencySatisfied = TRUE;
  
      MDB(3,fSCHED) MLog("WARNING:  Cannot find dependency job. MissingDependencyAction is set to RUN.\n");
  
      break;
    
    
    default:
      
      *DependencySatisfied = FALSE;

      MDB(3,fSCHED) MLog("WARNING:  Cannot find dependency job. MissingDependencyAction is unspecified.\n");
    
      break;
    
    } /* END switch(MSched.MMissingDependencyAction) */
  
  return(SUCCESS);
  }



/**
 * Runs jobs that are dependent on the current job finishing and need to run
 * immediately and use the same resource allocation
 *
 * @param J - [I] The job that has finished
 * @param DepList - [I] List of dependencies to launch
 */

int MJobRunImmediateDependents(

  mjob_t *J,       /* I */
  mln_t  *DepList) /* I */

  {
  mln_t  *tmpL;
  mjob_t *tmpJ = NULL;

  if (DepList == NULL)
    return(FAILURE);

  tmpL = DepList;

  while (tmpL != NULL)
    {
    MJobFind(tmpL->Name,&tmpJ,mjsmBasic);

    if (tmpL->Ptr != NULL)
      {
      /* Job has a specific dependency type */

      enum MDependEnum DType;

      DType = (enum MDependEnum)MUGetIndexCI((char *)tmpL->Ptr,MDependType,FALSE,0);

      /* NOTE: with the J->ImmediateDependent, any before dependencies are
          converted to after dependencies.   Instead of "before" on A, we
          put it on B and treat it as "after" */

      switch (DType)
        {
        case mdJobSuccessfulCompletion:
        case mdJobBeforeSuccessful:

          if (J->CompletionCode == 0)
            {
            /* job succeeded, run dependency */

            break;
            }

          tmpL = tmpL->Next;

          continue;

        case mdJobFailedCompletion:
        case mdJobBeforeFailed:

          if (J->CompletionCode != 0)
            {
            /* job failed, run dependency */

            break;
            }

          tmpL = tmpL->Next;

          continue;

        default:

          /* nothing specified, let it go */

          break;
        } /* END switch (DType) */

      if (DType != mdNONE)
        {
        MJobRemoveDependency(tmpJ,DType,J->Name,NULL,0);
        }
      } /* END if (tmpL->Ptr != NULL) */

    if ((tmpJ != NULL) && (!MJOBISACTIVE(tmpJ)))
      {
      MJobTransferAllocation(tmpJ,J);

      bmunset(&tmpJ->IFlags,mjifMaster);

      MJobRemoveDependency(tmpJ,mdJobSuccessfulCompletion,J->Name,NULL,0);

      MULLPrepend(&MJobsReadyToRun,tmpJ->Name,tmpJ);
      }
    else
      {
      MDB(5,fSCHED) MLog("INFO:    job '%s' completed - master job has already been destroyed\n",
        J->Name);
      }

    tmpL = tmpL->Next;
    } /* END while (tmpL != NULL) */

  return(SUCCESS);
  } /* END MJobRunImmediateDependents() */


