/**
 * @file MJobClass.c
 *
 * Contains job related functions that deal with classes.
 */

#include "moab.h"
#include "moab-proto.h"
#include "moab-global.h"  
#include "moab-const.h"  

/**
 * Gets the job's default class.
 *
 * Current precendence is user, default user, account, rm.
 *
 * @see MJobGetClass - parent
 *
 * @param J
 */

int __MJobGetDefaultClass(

  mjob_t *J)

  {
  MASSERT(J != NULL,"got a null job pointer when trying to get job's default class.");

  if (J == NULL)
    return(FAILURE);

  if (J->Credential.C != NULL) /* Already have default class */
    return(SUCCESS);

  /* precedence is user, account, default user, rm */

  if ((J->Credential.U != NULL) && (J->Credential.U->F.CDef != NULL))
    {
    J->Credential.C = J->Credential.U->F.CDef;
    }
  else if ((J->Credential.A != NULL) && (J->Credential.A->F.CDef != NULL))
    {
    J->Credential.C = J->Credential.A->F.CDef;
    }
  else if ((MSched.DefaultU != NULL) && (MSched.DefaultU->F.CDef != NULL))
    {
    J->Credential.C = MSched.DefaultU->F.CDef;
    }
  else if ((MSched.DefaultA != NULL) && (MSched.DefaultA->F.CDef != NULL))
    {
    J->Credential.C = MSched.DefaultA->F.CDef;
    }
  else if ((J->SubmitRM != NULL) && (J->SubmitRM->DefaultC != NULL))
    {
    J->Credential.C = J->SubmitRM->DefaultC;
    }

  return(SUCCESS);
  } /* END __MJobGetDefaultClass() */



/**
 * Assigns a default class to the job and remaps the job to a class if necessary.
 *
 * @param J (I)
 */

int MJobGetClass(

  mjob_t *J)

  {
  MASSERT(J != NULL,"got a null job pointer when trying to get job's class.");

  if (J == NULL)
    return(FAILURE);

  if (!bmisset(&J->IFlags,mjifClassLocked))
    {
    if (J->Credential.C == NULL)
      {
      __MJobGetDefaultClass(J);
      }    /* END if (J->Cred.C == NULL) */
    else
      {
      bmset(&J->IFlags,mjifClassSpecified);
      }

    /* remap class if set */

    if ((MSched.RemapC != NULL) &&
        (J->Credential.C != NULL) &&
        (!strcmp(J->Credential.C->Name,MSched.RemapC->Name)))
      {
      /* NOTE:  class mapping may depend on features, procs, nodes, memory, etc */

      MJobSelectClass(J,TRUE,TRUE,NULL,NULL);
      }
    }    /* END if (!bmisset(&J->IFlags,mjifClassLocked)) */

  if (J->Credential.C != NULL)
    {
    if (J->Credential.C->FType == mqftRouting)
      {
      if ((J->SubmitRM != NULL) && 
          (J->SubmitRM->Type != mrmtMoab) && 
          (J->SubmitRM->Type != mrmtS3))
        {
        MJobSetState(J,mjsNotQueued);
        }
      }
    }

  return(SUCCESS);
  } /* END MJobGetClass() */



/**
 * Select class based on user, requested duration, nodes, processors, features, and flags.
 *
 * @parent MRMJobPostLoad
 *
 * @param J (I)
 * @param ModifyJob (I)
 * @param ModifyRM (I)
 * @param CP (O) [optional]
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 */

int MJobSelectClass(

  mjob_t    *J,         /* I (modified) */
  mbool_t    ModifyJob, /* I */
  mbool_t    ModifyRM,  /* I */
  mclass_t **CP,        /* O (optional) */
  char      *EMsg)      /* O (optional,minsize=MMAX_LINE) */

  {
  int GPUIndex;
  int cindex;

  int MinPC;
  int MaxPC;
  int MinGPU = -1;

  int MinNC;
  int MaxNC;
  int MaxGPU;

  mulong MinWC;
  mulong MaxWC;

  int JGPU;
  int JPC;
  int JNC;
  mulong JWC;

  mbool_t ClassFound;

  mclass_t *C = NULL;

  macl_t   *UList;

  mreq_t   *RQ;

  mrm_t    *RM;

  mjob_t   *JMax;
  mjob_t   *JMin;

  const char *FName = "MJobSelectClass";

  MDB(1,fSTRUCT) MLog("%s(%s,%s,%s,CP,EMsg)\n",
      FName,
      (J != NULL) ? J->Name : "NULL",
      MBool[ModifyJob],
      MBool[ModifyRM]);

  if (CP != NULL)
    *CP = NULL;

  if (J == NULL)
    {
    return(FAILURE);
    }

  GPUIndex = MUMAGetIndex(meGRes,MCONST_GPUS,mAdd);

  JPC = J->TotalProcCount;
  JNC = J->Request.NC;
  JWC = J->SpecWCLimit[0];
  JGPU = MSNLGetIndexCount(&J->Req[0]->DRes.GenericRes,GPUIndex);

  ClassFound = MBNOTSET;

  /* is rm default queue specified and is the job acceptable in this queue? */

  RM = (J->SubmitRM != NULL) ? J->SubmitRM : &MRM[0];

  if ((J->Credential.C == NULL) && /* only try DefaultC if no class specified. */
      (RM != NULL) && 
      (RM->DefaultC != NULL) &&
      (MSched.RemapC != NULL) &&
      (RM->DefaultC->FType != mqftRouting) &&
      (RM->DefaultC != MSched.RemapC))
    {
    C = RM->DefaultC;

    JMin = C->L.JobMinimums[0];

    MinGPU = -1;

    if (JMin != NULL)
      {
      MinPC = JMin->Request.TC;  
      MinNC = JMin->Request.NC;
      MinWC = JMin->SpecWCLimit[0];

      if (bmisset(&JMin->IFlags,mjifPBSGPUSSpecified))
        MinGPU = MSNLGetIndexCount(&JMin->Req[0]->DRes.GenericRes,GPUIndex);
 
      UList = JMin->RequiredCredList;
      }
    else
      {
      MinPC = 0;
      MinNC = 0;
      MinWC = 0;

      UList = NULL;
      }

    JMax = C->L.JobMaximums[0];

    MaxGPU = -1;

    if (JMax != NULL)
      {
      MaxPC = JMax->Request.TC;
      MaxNC = JMax->Request.NC;
      MaxWC = JMax->SpecWCLimit[0];

      if (bmisset(&JMax->IFlags,mjifPBSGPUSSpecified))
        MaxGPU = MSNLGetIndexCount(&JMax->Req[0]->DRes.GenericRes,GPUIndex);
      }
    else
      {
      MaxPC = 0;
      MaxNC = 0;
      MaxWC = 0;
      }

    if (((MinPC == 0) || (JPC >= MinPC)) &&
        ((MaxPC == 0) || (JPC <= MaxPC)) &&
        ((MinNC == 0) || (JNC == 0) || (JNC >= MinNC)) &&
        ((MaxNC == 0) || (JNC <= MaxNC)) &&
        ((MinWC == 0) || (JWC >= MinWC)) &&
        ((MinGPU == -1) || (JGPU >= MinGPU)) &&
        ((MaxGPU == -1) || (JGPU <= MaxGPU)) &&
        ((MaxWC == 0) || (JWC <= MaxWC)))
      {
      /* no violations detected */

      /* NO-OP */
      }
    else
      {
      ClassFound = FALSE;
      }

    if (ClassFound != FALSE)
      {
      /* check required features */

      if ((JMin != NULL) && 
          (JMin->Req[0] != NULL) &&
          (!bmisclear(&JMin->Req[0]->ReqFBM)))
        {
        /* verify all required job features are requested  */

        RQ = J->Req[0];

        /* SUCCESS if JMin features is subset of J features */
        /* if JMin ReqFMode == OR, SUCCESS if any JMin features found */

        if (MAttrSubset(&RQ->ReqFBM,&JMin->Req[0]->ReqFBM,JMin->Req[0]->ReqFMode) == FAILURE)
          {
          ClassFound = FALSE;
          }
        else
          {
          /* job requests required feature */

          MDB(4,fSCHED) MLog("INFO:     default rm class %s acceptable as remap class for job %s (feature match)\n",
            C->Name,
            J->Name);
          }
        }
      }    /* END if (ClassFound != FALSE) */

    if (ClassFound != FALSE)
      {
      /* check exclusion features */

      if ((JMax != NULL) &&
          (!bmisclear(&JMax->Req[0]->ReqFBM)))
        {
        /* verify all required job features are requested  */

        RQ = J->Req[0];

        /* SUCCESS if JMax features is subset of J features */
        /* if JMax ReqFMode == OR, SUCCESS if any JMax features found */

        if (MAttrSubset(&RQ->ReqFBM,&JMax->Req[0]->ReqFBM,JMax->Req[0]->ReqFMode) == FAILURE)
          {
          MDB(4,fSCHED) MLog("INFO:     default rm class %s acceptable as remap class for job %s (feature match)\n",
            C->Name,
            J->Name);
          }
        else
          {
          /* job requests exclusion features */

          ClassFound = FALSE;
          }
        }
      }    /* END if (ClassFound != FALSE) */
  
    if (ClassFound != FALSE)
      {
      if ((J->Credential.U != NULL) && (UList != NULL))
        {
        macl_t *tACL;

        mbool_t UserIsRequired = FALSE;

        /* check if J->Cred.U is in UList */

        for (tACL = UList;tACL != NULL;tACL = tACL->Next)
          {
          if (tACL->Type != maUser)
            continue;

          UserIsRequired = TRUE;

          if (!strcmp(tACL->Name,J->Credential.U->Name))
            {
            /* requesting user located */

            break;
            }
          }  /* END (tACL) */

        if ((UserIsRequired == TRUE) &&
            (tACL == NULL))
          {
          /* requesting user not listed in class's required user list */

          MDB(7,fSCHED) MLog("INFO:     class %s rejected for job %s remap (not in user list)\n",
            C->Name,
            J->Name);

          ClassFound = FALSE;
          }
        }    /* END if ((J->Cred.U != NULL) && (UList != NULL)) */
      }      /* END if (ClassFound != FALSE) */

    if (ClassFound != FALSE)
      {
      if (bmisset(&J->Flags,mjfInteractive))
        {
        if ((JMax != NULL) && bmisset(&JMax->Flags,mjfInteractive))
          {
          /* job possesses exclusive flag */

          MDB(7,fSCHED) MLog("INFO:     class %s rejected for job %s remap (job requests exclusive flag)\n",
            C->Name,
            J->Name);

          ClassFound = FALSE;
          }
        else if ((JMin != NULL) && !bmisset(&JMin->Flags,mjfInteractive))
          {
          /* job does not possess required flag */

          MDB(7,fSCHED) MLog("INFO:     class %s rejected for job %s remap (job does not have required flag)\n",
            C->Name,
            J->Name);

          ClassFound = FALSE;
          }
        }
      }    /* END if (ClassFound != FALSE) */

    if (ClassFound == MBNOTSET)
      { 
      /* job meets all class constraints */

      ClassFound = TRUE;

      MDB(4,fSCHED) MLog("INFO:     default rm class %s acceptable as remap class for job %s (no features)\n",
        C->Name,
        J->Name);
      }
    }  /* END if ((J->SRM != NULL) && (J->SRM->DefaultC != NULL)) */

  /* classes:  empty, all, none, default, <class1>, ... */

  if (ClassFound != TRUE)  /* If default class not acceptable, continue search */
    {
    /* Search either MClass[] or MSched.RemapCList[] and start at the first real class index. */
        
    if (MSched.RemapCList[0] == NULL) /* If searching MClass*/
      cindex = MCLASS_FIRST_INDEX;
    else
      cindex = 0;
      
    for (;cindex < MMAX_CLASS;cindex++)
      {
      if (MSched.RemapCList[0] == NULL)  /* If searching MClass*/
        {
        C = &MClass[cindex];
        }
      else
        {
        /* class remap list specified */

        C = MSched.RemapCList[cindex];

        if (C == NULL)  /* If No Remap class found, break out of the search. */
          break;
        }

      if ((C->Name == NULL) || (C->Name[0] == '\0'))
        break;

      if ((MSched.RemapC != NULL) && 
          !strcmp(C->Name,MSched.RemapC->Name))
        {
        /* ignore remap class */

        continue;
        }

      MDB(7,fSCHED) MLog("INFO:     checking class %s as remap for job %s\n",
        C->Name,
        J->Name);

      JMin = C->L.JobMinimums[0];

      MinGPU = -1;

      if (JMin != NULL)
        {
        MinPC = JMin->Request.TC;
        MinNC = JMin->Request.NC;
        MinWC = JMin->SpecWCLimit[0];

        UList = JMin->RequiredCredList;

        if (bmisset(&JMin->IFlags,mjifPBSGPUSSpecified))
          MinGPU = MSNLGetIndexCount(&JMin->Req[0]->DRes.GenericRes,GPUIndex);
        }
      else
        {
        MinPC = 0;
        MinNC = 0;
        MinWC = 0;

        UList = NULL;
        }

      JMax = C->L.JobMaximums[0];

      MaxGPU = -1;

      if (JMax != NULL)
        {
        MaxPC = JMax->Request.TC;
        MaxNC = JMax->Request.NC;
        MaxWC = JMax->SpecWCLimit[0];

        if (bmisset(&JMax->IFlags,mjifPBSGPUSSpecified))
          MaxGPU = MSNLGetIndexCount(&JMax->Req[0]->DRes.GenericRes,GPUIndex);
        }
      else
        {
        MaxPC = 0;
        MaxNC = 0;
        MaxWC = 0;
        }

      if ((MinGPU != -1) && (JGPU < MinGPU)) 
        {
        /* job is too small */

        MDB(7,fSCHED) MLog("INFO:     class %s rejected for job %s remap (too few gpus - %d < %d)\n",
          C->Name,
          J->Name,
          JGPU,
          MinGPU);

        continue;
        }

      if ((MinPC > 0) && (JPC < MinPC)) 
        {
        /* job is too small */

        MDB(7,fSCHED) MLog("INFO:     class %s rejected for job %s remap (too few procs - %d < %d)\n",
          C->Name,
          J->Name,
          JPC,
          MinPC);

        continue;
        }

      if ((MinNC > 0) && (JNC > 0) && (JNC < MinNC))
        {
        /* job is too small */

        MDB(7,fSCHED) MLog("INFO:     class %s rejected for job %s remap (too few nodes - %d < %d)\n",
          C->Name,
          J->Name,
          JNC,
          MinNC);

        continue;
        }

      if ((MaxGPU != -1) && (JGPU > MaxGPU)) 
        {
        /* job is too big */

        MDB(7,fSCHED) MLog("INFO:     class %s rejected for job %s remap (too many gpus - %d > %d)\n",
          C->Name,
          J->Name,
          JGPU,
          MaxGPU);

        continue;
        }



      if ((MaxPC > 0) && (JPC > MaxPC)) 
        {
        /* job is too big */

        MDB(7,fSCHED) MLog("INFO:     class %s rejected for job %s remap (too many procs - %d > %d)\n",
          C->Name,
          J->Name,
          JPC,
          MaxPC);

        continue;
        }

      if ((MaxNC > 0) && (JNC > MaxNC))
        {
        /* job is too big */

        MDB(7,fSCHED) MLog("INFO:     class %s rejected for job %s remap (too many nodes - %d > %d)\n",
          C->Name,
          J->Name,
          JNC,
          MaxNC);

        continue;
        }

      if ((MaxWC > 0) && (JWC > MaxWC))
        {
        /* job is too long */

        MDB(7,fSCHED) MLog("INFO:     class %s rejected for job %s remap (job too long - %ld > %ld)\n",
          C->Name,
          J->Name,
          JWC,
          MaxWC);

        continue;
        }

      if ((MinWC > 0) && (JWC < MinWC))
        {
        /* job is too short */

        MDB(7,fSCHED) MLog("INFO:     class %s rejected for job %s remap (job too short - %ld < %ld)\n",
          C->Name,
          J->Name,
          JWC,
          MinWC);

        continue;
        }

      if ((JMin != NULL) && (!bmisclear(&JMin->Req[0]->ReqFBM)))
        {
        /* verify all min job features are requested  */

        RQ = J->Req[0];

        /* SUCCESS if JMin features is subset of J features */
        /* if JMin ReqFMode == OR, SUCCESS if any JMin features found */

        if (MAttrSubset(&RQ->ReqFBM,&JMin->Req[0]->ReqFBM,JMin->Req[0]->ReqFMode) == FAILURE)
          {
          /* job is missing required feature */

          MDB(7,fSCHED) MLog("INFO:     class %s rejected for job %s remap (missing features)\n",
            C->Name,
            J->Name);

          continue;
          } 
        }    /* END ((JMin != NULL) && ...) */

      if ((JMax != NULL) && (!bmisclear(&JMax->Req[0]->ReqFBM)))
        {
        /* verify no exclusive job features are requested  */

        RQ = J->Req[0];

        /* SUCCESS if JMax features is not subset of J features */
        /* if JMax ReqFMode == OR, SUCCESS if all JMax features found */

        if (MAttrSubset(&RQ->ReqFBM,&JMax->Req[0]->ReqFBM,JMax->Req[0]->ReqFMode) == SUCCESS)
          {
          /* job requests exclusive feature */

          MDB(7,fSCHED) MLog("INFO:     class %s rejected for job %s remap (possesses excluded features)\n",
            C->Name,
            J->Name);

          continue;
          }
        }    /* END ((JMax != NULL) && ...) */

      if ((J->Credential.U != NULL) && (UList != NULL))
        {
        macl_t *tACL;

        mbool_t UserIsRequired = FALSE;

        /* check if J->Cred.U is in UList */

        for (tACL = UList;tACL != NULL;tACL = tACL->Next)
          {
          if (tACL->Type != maUser)
            continue;

          UserIsRequired = TRUE;

          if (!strcmp(tACL->Name,J->Credential.U->Name))
            {
            /* requesting user located */

            break;
            }
          }  /* END for (aindex) */

        if ((UserIsRequired == TRUE) &&
            (tACL == NULL))
          {
          /* requesting user not listed in class required user list */

          MDB(7,fSCHED) MLog("INFO:     class %s rejected for job %s remap (not in user list)\n",
            C->Name,
            J->Name);

          continue;
          }
        }    /* END if ((J->Cred.U != NULL) && (UList != NULL)) */

      if (bmisset(&J->Flags,mjfInteractive))
        {
        if ((JMax != NULL) && bmisset(&JMax->Flags,mjfInteractive))
          {
          /* job possesses exclusive flag */

          MDB(7,fSCHED) MLog("INFO:     class %s rejected for job %s remap (job requests exclusive flag)\n",
            C->Name,
            J->Name);

          continue;
          }
        }    /* END if (bmisset(&J->Flags,mfjInteractive)) */
      else
        {
        if ((JMin != NULL) && bmisset(&JMin->Flags,mjfInteractive))
          {
          /* job does not possess required flag */

          MDB(7,fSCHED) MLog("INFO:     class %s rejected for job %s remap (job does not have required flag)\n"
,
            C->Name,
            J->Name);

          continue;
          }
        }

      /* matching class located */

      ClassFound = TRUE;

      break;
      }  /* END for (cindex) */
    }    /* END if (ClassFound == FALSE) */

  if (ClassFound != TRUE)
    {
    MDB(4,fSCHED) MLog("INFO:     no matching remap class located for job %s\n",
      J->Name);

    return(FAILURE);
    }

  /* matching class located */

  if (CP != NULL)
    *CP = C;

  MDB(4,fSCHED) MLog("INFO:     matching remap class '%s' located for job %s\n",
    C->Name,
    J->Name);

  if (ModifyJob == TRUE)
    {
    if (MJobSetClass(J,C,ModifyRM,EMsg) == FAILURE)
      {
      return(FAILURE);
      }
    }

  return(SUCCESS);
  }  /* END MJobSelectClass() */



/**
 * Will set the class credential of a job. It may also
 * alert the job's RM of the queue/class change.
 *
 * NOTE:  does not adjust job attributes inherited from class.
 * 
 * @see MJobApplyClassDefaults();
 *
 * @param J (I) The job which will have its class modified. [modified]
 * @param C (I) The class that should be applied to the job.
 * @param ModifyRM (I) Whether or not the underlying RM should be notified of the change.
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 */

int MJobSetClass(

  mjob_t   *J,         /* I - modified */
  mclass_t *C,         /* I */
  mbool_t   ModifyRM,  /* I */
  char     *EMsg)      /* O (optional,minsize=MMAX_LINE) */

  {
  char    tEMsg[MMAX_LINE];

  const char *FName = "MJobSetClass";

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if ((J == NULL) || (C == NULL))
    {
    if (EMsg != NULL)
      strcpy(EMsg,"invalid parameters - no class");

    return(FAILURE);
    }

  MDB(7,fSCHED) MLog("%s(%s,%s,%s,EMsg)\n",
    FName,
    J->Name,
    C->Name,
    MBool[ModifyRM]);

  /* NOTE:  MRMJobModify() may alter J->Cred.C/PSlot */

  if (ModifyRM == TRUE)
    {
    /* modify job within resource manager */

    if (MRMJobModify(
         J,
         "queue",
         NULL,
         C->Name,
         mSet,
         NULL,
         tEMsg,
         NULL) == FAILURE)
      {
      MDB(2,fSCHED) MLog("INFO:     cannot remap class for rm job %s - '%s'\n",
        J->Name,
        tEMsg);

      if (EMsg != NULL)
        strcpy(EMsg,tEMsg);

      return(FAILURE);
      }
    }    /* END if (ModifyRM == TRUE) */

  J->Credential.C = C;

  return(SUCCESS);
  }  /* END MJobSetClass() */


/**
 * Apply class task and node count defaults to job.
 *
 * If job's class (ClassDefJ) doesn't have a task or node count specified then the
 * job will get default scheduler class's (SchedDefJ) counts if they exist.
 *
 * @param J (I)
 * @param ClassDefJ (I)
 * @param SchedDefJ (I)
 */

int __ApplyDefaultTaskCounts(

  mjob_t *J,
  mjob_t *ClassDefJ,
  mjob_t *SchedDefJ)

  {
  if (J == NULL)
    return(FAILURE);

  if ((ClassDefJ == NULL) && (SchedDefJ == NULL))
    return(SUCCESS);

  if ((J->SubmitRM != NULL) && 
       bmisset(&J->SubmitRM->IFlags,mrmifLocalQueue) && 
      (J->DestinationRM == NULL) &&
      (!bmisset(&J->IFlags,mjifTasksSpecified)))
    {
    int defaultTC = 0;
    int defaultNC = 0;

    /* set default procs and nodes */

    if ((ClassDefJ != NULL) && (ClassDefJ->Request.TC > 0))
      defaultTC = ClassDefJ->Request.TC;
    else if ((SchedDefJ != NULL) && (SchedDefJ->Request.TC > 0))
      defaultTC = SchedDefJ->Request.TC;

    if ((ClassDefJ != NULL) && (ClassDefJ->Request.NC > 0))
      defaultNC = ClassDefJ->Request.NC;
    else if ((SchedDefJ != NULL) && (SchedDefJ->Request.NC > 0))
      defaultNC = SchedDefJ->Request.NC;

    if (defaultTC > 0)
      { 
      MReqSetAttr(
        J,
        J->Req[0],
        mrqaTCReqMin,
        (void **)&defaultTC,
        mdfInt,
        mSet);
      }

    if (defaultNC > 0)
      { 
      MReqSetAttr(
        J,
        J->Req[0],
        mrqaNCReqMin,
        (void **)&defaultNC,
        mdfInt,
        mSet);
      }
    }

  return(SUCCESS);
  } /* END void __ApplyDefaultTaskCounts() */



/**
 * Apply Class's JDef and JSet attributes to job.
 *
 * @param J (I) [modified]
 */

int MJobApplyClassDefaults(

  mjob_t   *J)

  {
  mclass_t *C = NULL;

  mjob_t *DefJ;
  mjob_t *ClassDefJ = NULL;
  mjob_t *SchedDefJ = NULL;

  mjob_t *SetJ;

  mreq_t *DefRQ = NULL;
  mreq_t *RQ;

  int     rqindex;

  marray_t *Prolog;
  marray_t *Epilog;

  const char *FName = "MJobApplyClassDefaults";

  MDB(5,fSTRUCT) MLog("%s(%s)\n",
    FName,
    (J != NULL) ? J->Name : "NULL");

  if (J == NULL)
    {
    return(FAILURE);
    }

  if (J->Credential.C != NULL)
    {
    C = J->Credential.C;
    }
  else
    {
    C = MSched.DefaultC;
    }

  if (C == NULL)
    {
    return(FAILURE);
    }

  if (C->NAIsSharedOnSerialJob == TRUE)
    {
    if (J->Request.TC == 1)
      {
      J->Req[0]->NAccessPolicy = mnacShared;

      MDB(7,fCONFIG) MLog("INFO:     job %s node access policy set to %s from class default\n",
        J->Name,
        MNAccessPolicy[J->Req[0]->NAccessPolicy]);
      }
    }

  if (J->LogLevel != 0)   
    J->LogLevel = C->LogLevel;

  /* store separate JDefs to allow the job get the scheduler default if the
   * class's JDef doesn't have a default set. */
  if ((J->Credential.C != NULL) && (J->Credential.C->L.JobDefaults != NULL))
    ClassDefJ = C->L.JobDefaults;
  
  if ((MSched.DefaultC != NULL) && (MSched.DefaultC->L.JobDefaults != NULL))
    SchedDefJ = MSched.DefaultC->L.JobDefaults;

  if (C->L.JobDefaults != NULL)
    {
    DefJ = C->L.JobDefaults;
    }
  else 
    DefJ = NULL;

  if (DefJ != NULL)
    {
    DefRQ = DefJ->Req[0];

    /* apply job attribute defaults */

    bmor(&J->AttrBM,&DefJ->AttrBM);

    if (DefJ->SystemPrio != 0)
      J->SystemPrio = DefJ->SystemPrio;

    if (DefJ->RMXString != NULL)
      {
      MUStrDup(&J->RMXString,DefJ->RMXString);

      MJobProcessExtensionString(J,J->RMXString,mxaNONE,NULL,NULL);
      }
 
    if (DefJ->Triggers != NULL)
      {
      MTrigApplyTemplate(&J->Triggers,DefJ->Triggers,(void *)J,mxoJob,J->Name);
      }      /* END if (DefJ->T != NULL) */

    Prolog = NULL;

    if (DefJ->PrologTList != NULL)
      {
      Prolog = DefJ->PrologTList;
      }
    else if ((MSched.DefaultC != NULL) && (MSched.DefaultC->L.JobDefaults != NULL))
      {
      Prolog = MSched.DefaultC->L.JobDefaults->PrologTList;
      }

    if (Prolog != NULL)
      {
      mtrig_t *T;

      int tindex;

      MTListCopy(Prolog,&J->PrologTList,FALSE);

      bmset(&J->IFlags,mjifProlog);

      for (tindex = 0; tindex < J->PrologTList->NumItems;tindex++)
        {
        T = (mtrig_t *)MUArrayListGetPtr(J->PrologTList,tindex);

        if (MTrigIsValid(T) == FALSE)
          continue;

        /* update trigger OID with new RID */

        T->O = (void *)J;
        T->OType = mxoJob;

        T->ETime = MSched.Time;

        MUStrDup(&T->OID,J->Name);

        MUFree(&T->TName);

        MUStrDup(&T->TName,J->Name);

        /* do not call MTrigInitialize() as it will add the trigger to the 
           job's regular trigger array */

        bmunset(&T->InternalFlags,mtifIsTemplate);
        }    /* END for (tindex) */
      }

    Epilog = NULL;
    
    if (DefJ->EpilogTList != NULL)
      {
      Epilog = DefJ->EpilogTList;
      }
    else if ((MSched.DefaultC != NULL) && (MSched.DefaultC->L.JobDefaults != NULL))
      {
      Epilog = MSched.DefaultC->L.JobDefaults->EpilogTList;
      }
  
    if (Epilog != NULL)
      {
      mtrig_t *T;

      int tindex;

      MTListCopy(Epilog,&J->EpilogTList,FALSE);

      for (tindex = 0; tindex < J->EpilogTList->NumItems;tindex++)
        {
        T = (mtrig_t *)MUArrayListGetPtr(J->EpilogTList,tindex);

        if (MTrigIsValid(T) == FALSE)
          continue;

        /* update trigger OID with new RID */

        T->O = (void *)J;
        T->OType = mxoJob;

        MUStrDup(&T->OID,J->Name);

        MUFree(&T->TName);

        MUStrDup(&T->TName,J->Name);

        /* do not call MTrigInitialize() as it will add the trigger to the 
           job's regular trigger array */

        bmunset(&T->InternalFlags,mtifIsTemplate);
        }    /* END for (tindex) */
      }
    }        /* END if (DefJ != NULL) */

  SetJ = C->L.JobSet;

  if (SetJ != NULL)
    {
    RQ = SetJ->Req[0];

    /* apply forced class job attribute settings */

    if (RQ != NULL)
      {
      if (RQ->NAccessPolicy != mnacNONE)
        {
        J->Req[0]->NAccessPolicy = RQ->NAccessPolicy;

        MDB(7,fCONFIG) MLog("INFO:     job %s node access policy set to %s from class %s force settings\n",
          J->Name,
          MNAccessPolicy[J->Req[0]->NAccessPolicy],
          C->Name);
        }

      if ((RQ->OptReq != NULL) &&
          (RQ->OptReq->ReqImage != NULL))
        {
        MReqSetAttr(
          J,
          J->Req[0],
          mrqaReqImage,
          (void **)RQ->OptReq->ReqImage,
          mdfString,
          mSet);
        }
      }    /* END if (RQ != NULL) */
    }      /* END if (SetJ != NULL) */

  /* loop through all job reqs */
  
  for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
    {
    RQ = J->Req[rqindex];

    /* insert default node features */

    /* FIXME:  should this only set ReqFBM if not ReqFBM has been already set? */

    if ((bmisclear(&RQ->ReqFBM)) &&
        (RQ->DRes.Procs != 0))
      {
      bmor(&RQ->ReqFBM,&C->DefFBM);
      }

    if (DefRQ != NULL)
      {
      if ((RQ->SetType == 0) && (DefRQ->SetType != 0))
        {
        /* copy node sets */

        MNSCopy(RQ,DefRQ);
        }

      if ((RQ->DRes.Procs > 0) &&
          (RQ->Opsys == 0) &&
          (DefRQ->Opsys != 0))
        {
        RQ->Opsys = DefRQ->Opsys;
        }
      }

    /* only apply queue defaults to local queue jobs */

    __ApplyDefaultTaskCounts(J,ClassDefJ,SchedDefJ);

    /* set default wclimit */

    /* handled earlier */
    }  /* END for (rqindex) */

  if ((DefJ != NULL) && (DefJ->Req[0] != NULL))
    {
    /* Only set GRes if class has default GRes and job has no set GRes */

    if ((!MSNLIsEmpty(&DefJ->Req[0]->DRes.GenericRes)) &&
        (MSNLIsEmpty(&J->Req[0]->DRes.GenericRes)))
      {
      int gindex;
      char tmpLine[MMAX_LINE];

      /* add class default generic resource requirements to job */

      for (gindex = 1;gindex <= MSched.GResCount;gindex++)
        {
        if (MSNLGetIndexCount(&DefJ->Req[0]->DRes.GenericRes,gindex) <= 0)
          continue;

        if (MJobAddRequiredGRes(
              J,
              MGRes.Name[gindex],
              MSNLGetIndexCount(&DefJ->Req[0]->DRes.GenericRes,gindex),
              mxaGRes,
              FALSE,
              FALSE) == FAILURE)
          {
          snprintf(tmpLine,sizeof(tmpLine),"cannot add required resource '%s'",
            MGRes.Name[gindex]);

          MJobSetHold(J,mhBatch,0,mhrNoResources,tmpLine);

          break;
          }
        }    /* END for (gindex) */
      }      /* END if (DefJ->Req[0]->GRes[0].Count > 0) */
    }        /* END if ((DefJ != NULL) && ...) */

  if (C->Partition != NULL)
    {
    char     ParLine[MMAX_LINE];

    /* set SysPAL and SpecPAL */

    if (strcasecmp(C->Partition,"ALL") &&
       (strstr(C->Partition,MPar[0].Name) == NULL))
      {
      mbitmap_t tmpI;

      /* non-global pmask specified */

      MPALFromString(C->Partition,mAdd,&tmpI);

      bmcopy(&J->SpecPAL,&tmpI);

      bmsetall(&J->SysPAL,MMAX_PAR);

      MJobGetPAL(J,&J->SpecPAL,&J->SysPAL,&J->PAL);
      }    /* END if (strstr() == NULL) */
    else
      {
      /* set global pmask */

      MDB(3,fCONFIG) MLog("INFO:     job %s setting SpecPAL to ALL\n",
        J->Name);

      bmsetall(&J->SpecPAL,MMAX_PAR);
      bmsetall(&J->SysPAL,MMAX_PAR);

      MJobGetPAL(J,&J->SpecPAL,&J->SysPAL,&J->PAL);
      }

    MDB(3,fCONFIG) MLog("INFO:     job %s SpecPAL (pmask) set to %s in apply class defaults\n",
      J->Name,
      MPALToString(&J->SpecPAL,NULL,ParLine));
    }

  return(SUCCESS);
  }  /* END MJobApplyClassDefaults() */



/**
 * Report SUCCESS if job satisfies class min/max job template constraints.
 *
 * @param J (I)
 * @param C (I)
 * @param AdjustJob (I)
 * @param CheckPIndex (I)  The index of the partition that should be checked.
 * @param Buffer (O) [optional]
 * @param BufSize (I) [optional]
 */

int MJobCheckClassJLimits(

  mjob_t   *J,       
  mclass_t *C,        
  mbool_t   AdjustJob, 
  int       CheckPIndex,
  char     *Buffer, 
  int       BufSize)

  {
  mjob_t   *JDefault;
  mclass_t *DefC;

  const char *FName = "MJobCheckClassJLimits";

  MDB(6,fSCHED) MLog("%s(%s,C,%s,Buffer,BufSize)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    MBool[AdjustJob]);

  if (Buffer != NULL)
    Buffer[0] = '\0';

  if ((J == NULL) || (C == NULL))
    {
    return(SUCCESS);
    }

  if (bmisset(&J->Flags,mjfIgnPolicies))
    {
    /* do not enforce class limits */

    return(SUCCESS);
    }

  /* if AdjustJob == TRUE, fix job violations */
  /* if AdjustJob == FALSE, return failure on violation */

  /* NOTE:  temporarily map 'proc' to 'task' for class limits */

  DefC = MSched.DefaultC;

  /* check max limits */

  JDefault = NULL;

  /* NOTE:  This might need to be moved into the for loop below at some point
   *        for checking defaults on a per-partition basis */

  if ((DefC != NULL) && (DefC->L.JobMaximums[0] != NULL))
    {
    JDefault = DefC->L.JobMaximums[0];
    }

  if (MJobCheckJMaxLimits(
        J,
        mxoClass,
        C->Name,
        C->L.JobMaximums[CheckPIndex],
        JDefault,
        AdjustJob,
        Buffer,    /* O */
        BufSize) == FAILURE)
    {
    return(FAILURE);
    }

  /* check min limits */

  JDefault = NULL;

  if ((DefC != NULL) && (DefC->L.JobMinimums[0] != NULL))
    {
    JDefault = DefC->L.JobMinimums[0];
    }

  if (MJobCheckJMinLimits(
        J,
        mxoClass,
        C->Name,
        C->L.JobMinimums[CheckPIndex],
        JDefault,
        AdjustJob,
        Buffer,
        BufSize) == FAILURE)
    {
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MJobCheckClassJLimits() */



/* END MJobClass.c */

/* vim: set ts=2 sw=2 expandtab nocindent filetype=c: */
