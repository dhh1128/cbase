/* HEADER */

      
/**
 * @file MJobCheck.c
 *
 * Contains: Job Check functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 *
 *
 * @param J (I) job
 * @param RQ (I) req
 * @param N (I) node
 * @param StartTime (I) job start time
 * @param TCAvail (O) [optional] tasks allowed at starttime 
 * @param MinSpeed (I)
 * @param RIndex (O) [optional] selection failure index
 * @param Affinity (O) [optional] affinity of allocation
 * @param ATime (O) [optional] availability duration
 * @param BMsg (O) [optional,minsize=MMAX_NAME] block reason
 */

int MJobCheckNStartTime(

  const mjob_t       *J,
  const mreq_t       *RQ,
  const mnode_t      *N,
  long                StartTime,
  int                *TCAvail,
  double              MinSpeed,
  enum MAllocRejEnum *RIndex,
  char               *Affinity,
  long               *ATime,
  char               *BMsg)

  {
  long AvailableTime = 0;

  enum MRsvTypeEnum Type;

  long AdjustedWCLimit;

  mbool_t           SeekLong;

  /* check reservations */

  char TString[MMAX_LINE];
  const char *FName = "MJobCheckNStartTime";

  MULToTString(StartTime - MSched.Time,TString);

  MDB(5,fSCHED) MLog("%s(%s,RQ,%s,%s,TasksAllowed,%f,RIndex,%s,ATime,BMsg)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (N != NULL) ? N->Name : "NULL",
    TString,
    MinSpeed,
    (Affinity != NULL) ? "Affinity" : "NULL");

  if (TCAvail != NULL)
    *TCAvail = 0;

  if (RIndex != NULL)
    *RIndex = marNONE;

  if (ATime != NULL)
    *ATime = 0;

  if (Affinity != NULL)
    *Affinity = (char)mnmNONE;

  if (BMsg != NULL)
    BMsg[0] = '\0';

  if ((J == NULL) || (RQ == NULL) || (N == NULL))
    {
    return(FAILURE);
    }

  AdjustedWCLimit = (long)((double)J->SpecWCLimit[0] / MinSpeed);

  /* check reservation time */

  if (!bmisset(&J->Flags,mjfIgnState))
    {
    /* for rsvmap jobs, go ahead and call MJobGetNRange(), even for IgnIdle, IgnRsv, IgnJob, ACLOverlap, and ExcludeAll */

    SeekLong = MSched.SeekLong;

    if (MJobGetNRange(
          J,
          RQ,
          N,
          StartTime,
          TCAvail,
          &AvailableTime,
          Affinity,
          &Type,
          SeekLong,
          BMsg) == FAILURE)
      {
      /* cannot locate feasible time */

      if (RIndex != NULL)
        {
        *RIndex = marTime;

        if (AvailableTime == 0)
          {
          if ((MJOBREQUIRESVMS(J) == TRUE) && (N->VMList == NULL))
            *RIndex = marVM;
          }
        }

      return(FAILURE);
      }
    }  
  else
    {
    int TC;

    /* do not care about the state of the system or resources consumed--everything is available (thus the CRes for Available parameter) */

    TC = MNodeGetTC(N,&N->CRes,&N->CRes,NULL,&RQ->DRes,StartTime,1,NULL);

    if (TC == 0)
      {
      if (RIndex != NULL)
        {
        *RIndex = marTime;
        }

      return(FAILURE);
      }

    if (TCAvail != NULL)
      *TCAvail = TC;

    AvailableTime = StartTime;

    if (ATime != NULL)
      *ATime = MMAX_TIME;

    if (Affinity != NULL)
      *Affinity = (char)mnmNeutralAffinity;
    }  /* END else if (bmisset(&J->Flags,mjfIgnState)) */

  if (AvailableTime < MIN(AdjustedWCLimit,8640000)) 
    {
    /* NOTE:  ignore time based violations over 100 days out */

    if (RIndex != NULL)
      {
      *RIndex = marTime;
      }

    return(FAILURE);
    }

  if (ATime != NULL)
    *ATime = AvailableTime;

  if (TCAvail != NULL)
    {
    if ((bmisset(&J->IFlags,mjifExactTPN) || bmisset(&J->IFlags,mjifMaxTPN)) &&
        (RQ->TasksPerNode > 0))
      {
      *TCAvail = MIN(RQ->TasksPerNode,*TCAvail);
      }

    if (*TCAvail < RQ->TasksPerNode)
      *TCAvail = 0;
    }

  return(SUCCESS);
  }  /* END MJobCheckNStartTime() */




/**
 *
 * @see MPolicyCheckLimit() - child
 *
 * @param J (I)
 * @param Q (I)
 * @param AdjustJob (I)
 * @param Buffer (O) [optional]
 * @param BufSize (I) [optional]
 */

int MJobCheckQOSJLimits(

  mjob_t   *J,         /* I */
  mqos_t   *Q,         /* I */
  mbool_t   AdjustJob, /* I */
  char     *Buffer,    /* O human-readable rejection reason (optional) */
  int       BufSize)   /* I (optional) */

  {
  mpc_t JUsage;

  mpu_t *OP;  /* object limits */
  mpu_t *DP;
  mpu_t *QP;

  int  pindex;
  long  tmpL;

  long tmpWCLimit;
  int  tmpNode;
  int  tmpProc;

  mqos_t *DefQ;

  mjob_t *JL;

  enum MActivePolicyTypeEnum PList[]  = {
    mptMaxProc,
    mptMaxNode,
    mptMaxPS,
    mptMaxPE,
    mptMaxWC,
    mptMinProc,
    mptNONE };

  const char *FName = "MJobCheckQOSJLimits";

  MDB(7,fSCHED) MLog("%s(%s,C,%d,Buffer,BufSize)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    AdjustJob);

  if (Buffer != NULL)
    Buffer[0] = '\0';

  if ((J == NULL) || (Q == NULL))
    {
    return(SUCCESS);
    }

  /* initialize values */

  tmpWCLimit = 0;
  tmpNode    = 0;
  tmpProc    = 0;

  OP = Q->L.JobPolicy;
  QP = Q->L.JobPolicy;

  DefQ = MSched.DefaultQ;

  if (DefQ != NULL)
    DP = DefQ->L.JobPolicy;
  else
    DP = NULL;

  if (Q->L.JobMaximums[0] != NULL)
    {
    /* check max limits */

    JL = Q->L.JobMaximums[0];

    tmpWCLimit = JL->SpecWCLimit[0];
    tmpNode    = JL->Request.NC;
    tmpProc    = JL->Request.TC;
    }  /* END if (C->L.JMax != NULL) */

  if ((DefQ != NULL) && (DefQ->L.JobMaximums[0] != NULL))
    {
    JL = DefQ->L.JobMaximums[0];

    if (tmpWCLimit == 0)
      tmpWCLimit = JL->SpecWCLimit[0];

    if (tmpNode == 0)
      tmpNode = JL->Request.NC;

    if (tmpProc == 0)
      tmpProc = JL->Request.TC;
    }

  if (tmpWCLimit > 0)
    {
    /* check max WC limit */

    if (((long)J->SpecWCLimit[1] > tmpWCLimit) || ((long)J->SpecWCLimit[1] == -1))
      {
      if (AdjustJob == TRUE)
        {
        MJobSetAttr(J,mjaReqAWDuration,(void **)&tmpWCLimit,mdfLong,mSet);
        }
      else
        {
        if (Buffer != NULL)
          {
          sprintf(Buffer,"wclimit too high (%ld > %ld)",
            J->SpecWCLimit[1],
            tmpWCLimit);
          }

        MDB(7,fSCHED) MLog("INFO:     job %s cannot access qos %s - wclimit too high (%ld > %ld)",
          J->Name,
          Q->Name,
          J->SpecWCLimit[1],
          tmpWCLimit);

        return(FAILURE);
        }
      }
    }    /* END if (tmpWCLimit > 0) */

  /* enforce policies */

  memset(&JUsage,0,sizeof(JUsage));

  MSNLInit(&JUsage.GRes);

  MPCFromJob(J,&JUsage);

  for (pindex = 0; PList[pindex] != mptNONE; pindex++)
    {
    if (MPolicyCheckLimit(
          &JUsage,
          FALSE,
          PList[pindex],
          mptHard,
          0, /* NOTE:  change to P->Index for per partition limits */
          OP,
          DP,
          QP,
          &tmpL,
          NULL,
          NULL) == FAILURE)
      {
      int AdjustVal;
  
      switch (PList[pindex])
        {
        case mptMaxJob:  AdjustVal = JUsage.Job;  break;
        case mptMaxNode: AdjustVal = JUsage.Node; break;
        case mptMaxPE:   AdjustVal = JUsage.PE;   break;
        case mptMaxProc: AdjustVal = JUsage.Proc; break;
        case mptMaxPS:   AdjustVal = JUsage.PS;   break;
        case mptMaxWC:   AdjustVal = JUsage.WC;   break;
        case mptMaxMem:  AdjustVal = JUsage.Mem;  break;
        case mptMinProc: AdjustVal = JUsage.Proc; break;
        case mptMaxGRes: AdjustVal = MSNLGetIndexCount(&JUsage.GRes,0); break;
        default: AdjustVal = 0; break;
        }

      if (Buffer != NULL)
        {
        sprintf(Buffer,"job '%s' violates %s policy of %ld (R: %d, U: %lu)",
          J->Name,
          MPolicyType[PList[pindex]],
          tmpL,
          AdjustVal,
          OP->Usage[PList[pindex]][0]);
        }

      MDB(7,fSCHED) MLog("INFO:     job %s cannot access qos %s - violates %s policy of %ld (R: %d, U: %d)\n",
        J->Name,
        Q->Name,
        MPolicyType[PList[pindex]],
        tmpL,
        AdjustVal,
        (int)OP->Usage[PList[pindex]][0]);
  
      MSNLFree(&JUsage.GRes);

      return(FAILURE);
      }  /* END MPolicyCheckLimit() */
    }    /* END for (pindex) */

  MSNLFree(&JUsage.GRes);

  return(SUCCESS);
  }  /* END MJobCheckQOSJLimits() */



/**
 * Report SUCCESS if job satisfies account min/max job template constraints.
 *
 * @param J (I)
 * @param A (I)
 * @param AdjustJob (I)
 * @param Buffer (O) [optional]
 * @param BufSize (I) [optional]
 */

int MJobCheckAccountJLimits(

  mjob_t   *J,         /* I */
  mgcred_t *A,         /* I */
  mbool_t   AdjustJob, /* I */
  char     *Buffer,    /* O (optional) */
  int       BufSize)   /* I (optional) */

  {
  mjob_t *JDefault;
  mgcred_t *DefA;

  const char *FName = "MJobCheckAccountJLimits";

  MDB(7,fSCHED) MLog("%s(%s,C,%d,Buffer,BufSize)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    AdjustJob);

  if (Buffer != NULL)
    Buffer[0] = '\0';

  if ((J == NULL) || (A == NULL))
    {
    return(SUCCESS);
    }

  /* if AdjustJob == TRUE, fix job violations */
  /* if AdjustJob == FALSE, return failure on violation */

  /* NOTE:  temporarily map 'proc' to 'task' for class limits */

  DefA = MSched.DefaultA;

  /* check max limits */

  JDefault = NULL;

  if ((DefA != NULL) && (DefA->L.JobMaximums[0] != NULL))
    {
    JDefault = DefA->L.JobMaximums[0];
    }

  if (MJobCheckJMaxLimits(
        J,
        mxoAcct,
        A->Name,
        A->L.JobMaximums[0],
        JDefault,
        AdjustJob,
        Buffer,
        BufSize) == FAILURE)
    {
    return(FAILURE);
    }

  /* check min limits */

  JDefault = NULL;

  if ((DefA != NULL) && (DefA->L.JobMinimums[0] != NULL))
    {
    JDefault = DefA->L.JobMinimums[0];
    }

  if (MJobCheckJMinLimits(
        J,
        mxoAcct,
        A->Name,
        A->L.JobMinimums[0],
        JDefault,
        AdjustJob,
        Buffer,
        BufSize) == FAILURE)
    {
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MJobCheckAccountJLimits() */



/**
 *
 *
 * @param J
 */

int MJobCheckDeadLine(

  mjob_t *J)
  
  {
  if (J == NULL)
    {
    return(FAILURE);
    }

  /* NYI */

  /* if deadline scheduling is enabled
      remove all deadline/priority reservations
      create deadline reservations at latest possible time
      for J in priority order
        {
        if J->R is deadline
          slide forward if possible
        if J->R is priority
          create reservation
        }
  */

  /* NOTE:  this should be handled within MQueueScheduleRJobs() */
 
  return(SUCCESS);
  }  /* END MJobCheckDeadLine() */




/**
 * Checks to make sure a job can run in the given partition.
 *
 * @see MJobEvaluateParAccess() - child
 *
 * NOTE:  check the following:
 *   J->PAL
 *   MigrationPath to Partition/RM
 *   RM class, account, group authorization 
 *   partition Max limits
 *   partition Min limits 
 *   MJobEvaluateParAccess()
 *   check job node match policy
 *   ...
 *
 * @see MJobEval() - parent
 *
 * @param J (I)
 * @param P (O)
 * @param IsPartial (I) not used - job is partial, do not verify resource quantity
 * @param RIndexP (O) optional
 * @param EMsg (O) optional,minsize=MMAX_LINE
 */

int MJobCheckParAccess(

  mjob_t             *J,         /* I */
  const mpar_t       *P,         /* O */
  mbool_t             IsPartial, /* I (not used - job is partial, do not verify resource quantity) */
  enum MAllocRejEnum *RIndexP,   /* O (optional) */
  char               *EMsg)      /* O (optional,minsize=MMAX_LINE) */

  {
  const char *FName = "MJobCheckParAccess";

  mrm_t      *R;

  mreq_t     *RQ;

  MDB(4,fCKPT) MLog("%s(%s,%s,%s,RIndexP,EMsg)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (P != NULL) ? P->Name : "NULL",
    MBool[IsPartial]);

  if (EMsg != NULL)
    {
    EMsg[0] = '\0';
    }

  if (RIndexP != NULL)
    *RIndexP = marNONE;

  if ((J == NULL) || (P == NULL))
    {
    MDB(6,fCKPT) MLog("INFO:     invalid parameters passed to %s\n",
      FName);

    if (RIndexP != NULL)
      *RIndexP = marCorruption;

    if (EMsg != NULL)
      strcpy(EMsg,"internal error");

    return(FAILURE);
    }

  /* deny access if partition doesn't meet resource requirements */
  /* NOTE: be aware of init/delayed race conditions with resource info */
  /* NYI */

  /* grant access if job consumes no resources */

  if (bmisset(&J->Flags,mjfNoResources))
    {
    return(SUCCESS);
    }

  /* deny access if partition not in job PMask */

  if (!bmisset(&J->PAL,P->Index))
    {
    if (EMsg != NULL)
      {
      sprintf(EMsg,"partition %s not in job partition mask",
        P->Name);
      }

    MDB(6,fCKPT) MLog("INFO:     job %s cannot access partition %s - not in partition mask\n",
      J->Name,
      P->Name);

    if (RIndexP != NULL)
      *RIndexP = marPartition;

    return(FAILURE);
    }

  R = P->RM;

  /* deny access if partitions do not share RM and 
     there is no migration path from job source rm to partition */

  if (bmisset(&J->Flags,mjfNoRMStart))
    {
    /* NO-OP */
    }
  else if ((R != NULL) && (J->SubmitRM != NULL))
    {
    if (R != J->SubmitRM)
      {
      if (R->Type != mrmtMoab)
        {
        /* check native migration path */

        /* NOTE:  J->Cred.U->OID can be -1 based on OSCREDLOOKUP parameter */

        if ((MSched.UID != 0) && 
            (J->Credential.U != NULL) && 
            (J->Credential.U->OID != MSched.UID) &&
            ((J->Credential.U->OID != -1) && (getenv("MFORCESUBMIT") == NULL)) &&
            ((R->Type != mrmtWiki) && (R->SubType != mrmstSLURM)) && /* can run as slurm user */
            ((R->Type != mrmtPBS) && (!bmisset(&R->Flags,mrmfProxyJobSubmission))) &&
            (!bmisset(&J->IFlags,mjifDataOnly)))
          {
          /* No SUID, Moab can only migrate jobs as execution user */

          if (EMsg != NULL)
            {
            sprintf(EMsg,"no migration path - cannot submit job as target user");
            }

          MDB(6,fCKPT) MLog("INFO:     job %s cannot access partition %s - no migration path\n",
            J->Name,
            P->Name);

          if (RIndexP != NULL)
            *RIndexP = marRM;

          return(FAILURE);
          }

        if ((J->Credential.U == NULL) ||
           ((J->Credential.U->OID < 0) &&
            (!bmisset(&R->Flags,mrmfDCred)) &&
            (!bmisset(&R->Flags,mrmfUserSpaceIsSeparate)) &&
            (!bmisset(&R->Flags,mrmfProxyJobSubmission))))
          { 
          /* credentials do not exist locally to allow direct submission to RM */

          if (EMsg != NULL)
            {
            sprintf(EMsg,"user %s does not exist on '%s'",
              (J->Credential.U != NULL) ? J->Credential.U->Name : "???",
              MSched.ServerHost);
            }

          MDB(6,fCKPT) MLog("INFO:     job %s cannot access partition %s - user %s does not exist locally\n",
            J->Name,
            P->Name,
            J->Credential.U->Name);

          if (RIndexP != NULL)
            *RIndexP = marUser;

          return(FAILURE);
          }

        /* NOTE:  check if user root allowed to submit jobs was here but was moved to
                  MRMJobPostLoad() after job templates were loaded to allow template
                  based EUser attribute support.
        */

        if ((J->Credential.G == NULL) ||
           ((J->Credential.G->OID < 0) &&
            (!bmisset(&R->Flags,mrmfDCred)) &&
            (!bmisset(&R->Flags,mrmfUserSpaceIsSeparate)) &&
            (!bmisset(&R->Flags,mrmfProxyJobSubmission)) &&
            (MCredGroupIsVariable(J) == FALSE)))
          {
          /* credentials do not exist locally to allow direct submission to RM */

          if (EMsg != NULL)
            {
            sprintf(EMsg,"group %s does not exist on '%s'",
              (J->Credential.G != NULL) ? J->Credential.G->Name : "???",
              MSched.ServerHost);
            }

          MDB(6,fCKPT) MLog("INFO:     job %s cannot access partition %s - group %s does not exist locally\n",
            J->Name,
            P->Name,
            (J->Credential.G != NULL) ? J->Credential.G->Name : "NULL");

          if (RIndexP != NULL)
            *RIndexP = marGroup;

          return(FAILURE);
          }

        if ((J->Credential.G->OID == 0) &&
            !bmisset(&J->Flags,mjfSystemJob) &&
            (MSched.AllowRootJobs == FALSE))
          {
          /* credentials do not exist locally to allow direct submission to RM */

          if (EMsg != NULL)
            {
            sprintf(EMsg,"group root cannot submit jobs");
            }

          MDB(6,fCKPT) MLog("INFO:     job %s cannot access partition %s - group root cannot submit jobs\n",
            J->Name,
            P->Name);

          if (RIndexP != NULL)
            *RIndexP = marGroup;

          return(FAILURE);
          }
        }
      }      /* END if (R != J->SRM) */

    if ((J->ReqRMType != mrmtNONE) && (MJobLangIsSupported(&R->Languages,J->ReqRMType) == FALSE))
      {
      if (EMsg != NULL)
        {
        sprintf(EMsg,"required RM type '%s' not supported",
          MRMType[J->ReqRMType]);
        }

      if (RIndexP != NULL)
        *RIndexP = marRM;
 
      return(FAILURE);
      }

    if ((R->Type == mrmtMoab) &&
        (J->Grid != NULL) &&
        (R->MaxJobHop > 0) &&
        (J->Grid->HopCount >= R->MaxJobHop))
      {
      /* hop count violated */
      
      if (EMsg != NULL)
        {
        sprintf(EMsg,"hop count violation '%d'",
          J->Grid->HopCount);
        }

      MDB(6,fSCHED) MLog("INFO:     partition %s cannot support job %s (violates hop count '%d')\n",
        P->Name,
        J->Name,
        R->MaxJobHop);

      if (RIndexP != NULL)
        *RIndexP = marLocality;

      return(FAILURE);
      }
    }  /* END if ((R != NULL) && (J->SrcRM != NULL)) */
  else if (R == NULL)
    {
    if ((J->Credential.U == NULL) ||
        (J->Credential.U->OID <= 0))
      { 
      /* credentials do not exist locally to allow direct submission to RM */

      if (EMsg != NULL)
        {
        sprintf(EMsg,"user %s does not exist on '%s'",
          (J->Credential.U != NULL) ? J->Credential.U->Name : "???",
          MSched.ServerHost);
        }

       MDB(6,fCKPT) MLog("INFO:     job %s cannot access partition %s - user %s does not exist locally\n",
         J->Name,
         P->Name,
         J->Credential.U->Name);

      if (RIndexP != NULL)
        *RIndexP = marUser;

      return(FAILURE);
      }

    if ((J->Credential.G == NULL) ||
        (J->Credential.G->OID <= 0))
      {
      /* credentials do not exist locally to allow direct submission to RM */

      if (EMsg != NULL)
        {
        sprintf(EMsg,"group %s does not exist on '%s'",
          (J->Credential.G != NULL) ? J->Credential.G->Name : "???",
          MSched.ServerHost);
        }

      MDB(6,fCKPT) MLog("INFO:     job %s cannot access partition %s - group %s does not exist locally\n",
        J->Name,
        P->Name,
        (J->Credential.G != NULL) ? J->Credential.G->Name : "NULL");

      if (RIndexP != NULL)
        *RIndexP = marGroup;

      return(FAILURE);
      }
    }  /* END else if ((R == NULL) && ...) */

  if (R != NULL)
    {
    if ((R->AuthA != NULL) && 
       ((J->Credential.A == NULL) || (MUCheckList(J->Credential.A->Name,R->AuthA,NULL) == FAILURE)))
      {
      if (EMsg != NULL)
        {
        sprintf(EMsg,"account %s not allowed to access RM '%s'",
          (J->Credential.A != NULL) ? J->Credential.A->Name : "???",
          R->Name);
        }

      MDB(6,fCKPT) MLog("INFO:     account %s not allowed to access RM '%s'",
        (J->Credential.A != NULL) ? J->Credential.A->Name : "???",
        R->Name);

      if (RIndexP != NULL)
        *RIndexP = marAccount;

      return(FAILURE);
      }  /* END if ((R->AuthA != NULL) && ...) */

    if ((R->AuthC != NULL) && 
       ((J->Credential.C == NULL) || (MUCheckList(J->Credential.C->Name,R->AuthC,NULL) == FAILURE)))
      {
      if (EMsg != NULL)
        {
        sprintf(EMsg,"class %s not allowed to access RM '%s'",
          (J->Credential.C != NULL) ? J->Credential.C->Name : "???",
          R->Name);
        }

      MDB(6,fCKPT) MLog("INFO:     class %s not allowed to access RM '%s'",
        (J->Credential.C != NULL) ? J->Credential.C->Name : "???",
        R->Name);

      if (RIndexP != NULL)
        *RIndexP = marClass;

      return(FAILURE);
      }  /* END if ((R->AuthC != NULL) && ...) */

    if ((R->AuthG != NULL) && 
       ((J->Credential.G == NULL) || (MUCheckList(J->Credential.G->Name,R->AuthG,NULL) == FAILURE)))
      {
      if (EMsg != NULL)
        {
        sprintf(EMsg,"group %s not allowed to access RM '%s'",
          (J->Credential.G != NULL) ? J->Credential.G->Name : "???",
          R->Name);
        }

      MDB(6,fCKPT) MLog("INFO:     group %s not allowed to access RM '%s'",
        (J->Credential.G != NULL) ? J->Credential.G->Name : "???",
        R->Name);

      if (RIndexP != NULL)
        *RIndexP = marGroup;

      return(FAILURE);
      }  /* END if ((R->AuthG != NULL) && ...) */

    if ((R->AuthQ != NULL) && 
       ((R->AuthQ == NULL) || (MUCheckList(J->Credential.Q->Name,R->AuthQ,NULL) == FAILURE)))
      {
      if (EMsg != NULL)
        {
        sprintf(EMsg,"qos %s not allowed to access RM '%s'",
          (J->Credential.Q != NULL) ? J->Credential.Q->Name : "???",
          R->Name);
        }

      MDB(6,fCKPT) MLog("INFO:     qos %s not allowed to access RM '%s'",
        (J->Credential.Q != NULL) ? J->Credential.Q->Name : "???",
        R->Name);

      if (RIndexP != NULL)
        *RIndexP = marQOS;

      return(FAILURE);
      }  /* END if ((R->AuthQ != NULL) && ...) */

    if ((R->AuthU != NULL) && 
       ((J->Credential.U == NULL) || (MUCheckList(J->Credential.U->Name,R->AuthU,NULL) == FAILURE)))
      {
      if (EMsg != NULL)
        {
        sprintf(EMsg,"user %s not allowed to access RM '%s'",
          (J->Credential.U != NULL) ? J->Credential.U->Name : "???",
          R->Name);
        }

      MDB(6,fCKPT) MLog("INFO:     user %s not allowed to access RM '%s'",
        (J->Credential.U != NULL) ? J->Credential.U->Name : "???",
        R->Name);

      if (RIndexP != NULL)
        *RIndexP = marUser;

      return(FAILURE);
      }  /* END if ((R->AuthU != NULL) && ...) */
    }    /* END if (R != NULL) */

  if (J->Credential.C != NULL)
    {
    if (bmisset(&J->Credential.C->Flags,mcfRemote))
      {
      /* NO-OP */
      }
    else if (!bmisset(&P->Classes,J->Credential.C->Index))
      {
      MDB(4,fSTRUCT) MLog("INFO:     partition %s cannot support job %s (class '%s')\n",
        P->Name,
        J->Name,
        J->Credential.C->Name);

      if (EMsg != NULL)
        {
        sprintf(EMsg,"partition %s does not support requested class %s",
          P->Name,
          J->Credential.C->Name);
        }

      if (RIndexP != NULL)
        *RIndexP = marClass;

      return(FAILURE);
      }
    else if ((J->Request.NC != 0) &&
             (J->Request.NC > J->Credential.C->ConfiguredNodes))
      {
      MDB(4,fSTRUCT) MLog("INFO:     Class %s cannot support job %s (Nodes R:%d,C:%d)\n",
        J->Credential.C->Name,
        J->Name,
        J->Request.NC,
        J->Credential.C->ConfiguredNodes);

      if (EMsg != NULL)
        {
        if (J->Credential.C->ConfiguredNodes == 0)
          {
          sprintf(EMsg,"Class %s has no nodes configured",
            J->Credential.C->Name);
          }
        else 
          {
          sprintf(EMsg,"Class %s has insufficient nodes configured (%d < %d)",
            J->Credential.C->Name,
            J->Credential.C->ConfiguredNodes,
            J->Request.NC);
          }
        }

      if (RIndexP != NULL)
        *RIndexP = marNodeCount;

      return(FAILURE);
      }
    }    /* END if (J->Cred.C != NULL) */

  if (MJobCheckParLimits(J,P,RIndexP,EMsg) == FAILURE)
    {
    return(FAILURE);
    }

  RQ = J->Req[0];

  if (RQ != NULL)
    {
    if (RQ->TasksPerNode * RQ->DRes.Procs > P->MaxPPN)
      {
      if (EMsg != NULL)
        sprintf(EMsg,"partition ppn exceeded");

      if (RIndexP != NULL)
        *RIndexP = marTime;

      return(FAILURE);
      }
    }

  if (MJobEvaluateParAccess(J,P,RIndexP,EMsg) == FAILURE)
    {
    return(FAILURE);
    }

  if ((!bmisclear(&J->NodeMatchBM)) && 
      (!bmisclear(&P->JobNodeMatchBM)) &&
      ((bmisset(&J->NodeMatchBM,mnmpExactNodeMatch) &&
       !bmisset(&P->JobNodeMatchBM,mnmpExactNodeMatch)) ||
       (bmisset(&J->NodeMatchBM,mnmpExactProcMatch) &&
       !bmisset(&P->JobNodeMatchBM,mnmpExactProcMatch))))
    {
    if (EMsg != NULL)
      strcpy(EMsg,"partition JOBNODEMATCHPOLICY violated");
 
    return(FAILURE);
    }

  /* added by Doug (per Dave's request), but LLNL and others thought it was a bad fix

  if ((MSched.MapFeatureToPartition == TRUE) &&
      (!bmisclear(&J->Req[0]->ReqFBM)) &&
      (MAttrSubset(P->FBM,J->Req[0]->ReqFBM,sizeof(P->FBM),J->Req[0]->ReqFMode) != SUCCESS))
    {
    if (EMsg != NULL)
      strcpy(EMsg,"partition does not have required features");

    if (RIndexP != NULL)
      *RIndexP = marFeatures;

    return(FAILURE);
    }
  */

  return(SUCCESS);
  }  /* END MJobCheckParAccess() */




/**
 *
 *
 * @param J (I)
 * @param StartTime (I)
 * @param EndTime (I)
 */

int MJobCheckExpansion(

  mjob_t    *J,            /* I */
  mulong     StartTime,    /* I */
  mulong     EndTime)      /* I */

  {
  int nindex;
  int rqindex;

  mnode_t *N;
  mreq_t  *RQ;

  mnl_t *NL;

  mrange_t RRange;
  mrange_t ARange[2];

  if (J == NULL)
    {
    return(FAILURE);
    }  

  for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
    {
    RQ = J->Req[rqindex];

    NL = &RQ->NodeList;

    for (nindex = 0;MNLGetNodeAtIndex(NL,nindex,&N) == SUCCESS;nindex++)
      {
      RRange.STime = StartTime;
      RRange.ETime = EndTime;
      RRange.TC    = MNLGetTCAtIndex(NL,nindex);
      RRange.NC    = 1;

      if (MJobGetSNRange(
            J,
            J->Req[0],
            N,
            &RRange,
            1,
            NULL,
            NULL,
            ARange,
            NULL,
            MSched.SeekLong,
            NULL) == FAILURE)
        {
        return(FAILURE);
        }
      }   /* END for (nindex) */
    }     /* END for (rqindex) */

  return(SUCCESS);
  }  /* END MNLCheckJExpansion() */






/* __MJobCheckFSTreeAccess
 *  Inner recursive function which is initially called by 
 *  MJobCheckFSTreeAccess.
 *
 */

int __MJobCheckFSTreeAccess(

  mxml_t *Tree,      /* I */
  mbool_t *AFound,   /* I */
  mbool_t *CFound,   /* I */
  mbool_t *UFound,   /* I */
  mbool_t *GFound,   /* I */
  mbool_t *QFound,   /* I */
  mcred_t *FSCreds)  /* I */

  {
  mbool_t OAccount;
  mbool_t OClass;
  mbool_t OUser;
  mbool_t OGroup;
  mbool_t OQOS;

  mxml_t *CE;
  int CTok;
  mfst_t *TData;

  if ((Tree == NULL) ||
      (AFound == NULL) ||
      (CFound == NULL) ||
      (UFound == NULL) ||
      (GFound == NULL) ||
      (QFound == NULL) ||
      (FSCreds == NULL))
    {
    return(FAILURE);
    }

  /* Save off passed in parameters */ 
  OAccount = *AFound;
  OClass   = *CFound;
  OUser    = *UFound;
  OGroup   = *GFound;
  OQOS     = *QFound;

  TData = Tree->TData;

  switch (TData->OType)
    {
    case mxoUser:

      if (FSCreds->U == TData->O)
        *UFound = TRUE;
      else if (*UFound == MBNOTSET)
        *UFound = FALSE;

      break;

    case mxoAcct:                                                           

      if (FSCreds->A == TData->O)                                           
        *AFound = TRUE;
      else if (*AFound == MBNOTSET)                                         
        *AFound = FALSE;                                                    

      break;

    case mxoClass:                                                          

      if (FSCreds->C == TData->O)                                           
        *CFound = TRUE;
      else if (*CFound == MBNOTSET)                                         
        *CFound = FALSE;                                                    

      break;

    case mxoGroup:                                                          

      if (FSCreds->G == TData->O)                                           
        *GFound = TRUE;
      else if (*GFound == MBNOTSET)                                         
        *GFound = FALSE;                                                    

      break;

    case mxoQOS:                                                          

      if (FSCreds->Q == TData->O)                                           
        *QFound = TRUE;
      else if (*QFound == MBNOTSET)                                         
        *QFound = FALSE;                                                    

      break;

    default:

      /* NO-OP */

      break;
    }

  CTok = -1;

  if ((Tree->CCount == 0) && (TData->OType != mxoNONE))
    {
    /* Check if this leaf matches passed in creds */

    if ((*AFound != FALSE) &&
        (*GFound != FALSE) &&
        (*UFound != FALSE) &&
        (*CFound != FALSE) &&
        (*QFound != FALSE) &&
        ((MSched.FSTreeUserIsRequired == FALSE) ||
         (*UFound == TRUE)))
      {
      /* Not setting anything, just saying that given creds are 
           valid in the fairshare tree */

      return(SUCCESS);
      }
    }

  while (MXMLGetChild(Tree,NULL,&CTok,&CE) == SUCCESS)
    {
    if (__MJobCheckFSTreeAccess(CE,AFound,CFound,UFound,GFound,QFound,FSCreds) == SUCCESS)
      {
      return(SUCCESS);
      }
    }

  *AFound = OAccount;
  *CFound = OClass;
  *UFound = OUser;
  *GFound = OGroup;
  *QFound = OQOS;

  return(FAILURE);
  }  /* END __MJobCheckFSTreeAccess */




/**
 *  MJobCheckFSTreeAccess
 *
 *   Checks if the specified job has access to the fairshare tree in the given
 *    partition.  If Account, Class, User, Group, and/or QOS are specified,
 *    access will be checked with those credentials in place of the job's 
 *    current credentials.  Each credential may be left as NULL to use the
 *    job's credential.
 *
 * @param J         (I) Job to check access for
 * @param P         (I) Partition to check FSTree access in
 * @param Account   (I) Optional
 * @param Class     (I) Optional
 * @param User      (I) Optional
 * @param Group     (I) Optional
 * @param QOS       (I) Optional
 */

int MJobCheckFSTreeAccess(
  mjob_t   *J,        /* I */
  mpar_t   *P,        /* I */
  mgcred_t *Account,  /* I (Optional) */
  mclass_t *Class,    /* I (Optional) */
  mgcred_t *User,     /* I (Optional) */
  mgcred_t *Group,    /* I (Optional) */
  mqos_t   *QOS)      /* I (Optional) */

  {
  int ValidCreds;

  mcred_t FSCreds;

  mbool_t AccountFound;
  mbool_t ClassFound;
  mbool_t UserFound;
  mbool_t GroupFound;
  mbool_t QOSFound;

  if ((J == NULL) || (P == NULL))
    {
    return(FAILURE);
    }

  if (P->FSC.ShareTree == NULL)
    {
    /* No fairshare tree for this partition, return success */

    return(SUCCESS);
    }

  /* Set found variables to not set */

  AccountFound = MBNOTSET;
  ClassFound   = MBNOTSET;
  UserFound    = MBNOTSET;
  GroupFound   = MBNOTSET;
  QOSFound     = MBNOTSET;

  /* Set up account, class, etc. */

  if (Account != NULL)
    FSCreds.A = Account;
  else
    FSCreds.A = J->Credential.A;

  if (Class != NULL)
    FSCreds.C = Class;
  else
    FSCreds.C = J->Credential.C;

  if (User != NULL)
    FSCreds.U = User;
  else
    FSCreds.U = J->Credential.U;

  if (Group != NULL)
    FSCreds.G = Group;
  else
    FSCreds.G = J->Credential.G;

  if (QOS != NULL)
    FSCreds.Q = QOS;
  else
    FSCreds.Q = J->Credential.Q;

  ValidCreds =  __MJobCheckFSTreeAccess(
                    P->FSC.ShareTree,
                    &AccountFound,
                    &ClassFound,
                    &UserFound,
                    &GroupFound,
                    &QOSFound,
                    &FSCreds);

  return(ValidCreds);
  }  /* END MJobCheckFSTreeAccess */




/**
 * Determine whether partition has enough configured resources to satisfy job.
 *
 * @param J (I)
 * @param P (I)
 */

int MJobCheckParRes(

  mjob_t *J,  /* I */
  mpar_t *P)  /* I */

  {
  int rqindex;
  mreq_t *RQ;

  if ((J == NULL) ||
      (P == NULL))
    {
    return(FAILURE);
    }

  for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
    {
    RQ = J->Req[rqindex];

    /* only check reqs that want non-global resources */

    if (MReqIsGlobalOnly(RQ) == TRUE)
      continue;

    if (MParGetTC(P,&P->ARes,&P->CRes,&P->DRes,&RQ->DRes,MMAX_TIME) <= 0)
      {
      return(FAILURE);
      }
    } /* END if(MParGetTC() <= 0) */

  return(SUCCESS);
  }  /* END MJobCheckParRes() */





/**
 * Verify job can run in target partition.
 *
 * @param J (I)
 * @param P (I) 
 * @param CheckClassResources (I) 
 * @param EMsg (O) [optional,minsize=MMAX_LINE] 
 */

int MJobCheckParFeasibility(

  mjob_t *J,     /* I */
  mpar_t *P,     /* I */
  mbool_t CheckClassResources, /* I */
  char   *EMsg)  /* O (optional,minsize=MMAX_LINE) */

  {
  if (EMsg != NULL)
    EMsg[0] = '\0';

  if ((J == NULL) ||
      (P == NULL))
    {
    return(FAILURE);
    }

  if ((J->Credential.C != NULL) && (J->Credential.C->FType != mqftRouting))
    {
    mclass_t *tmpClass = J->Credential.C;

    /*
    if (P->RM != NULL)
      {
      char tmpCName[MMAX_NAME];

      if (MOMap(P->RM->OMap,mxoClass,tmpClass->Name,FALSE,FALSE,tmpCName) == SUCCESS)
        {
        tmpClass = NULL;
        }
      }
    */

    if (tmpClass != NULL)
      {
      if (!bmisset(&P->Classes,tmpClass->Index))
        {
        /* requested class does not exist in specified partition */

        if (EMsg != NULL)
          sprintf(EMsg,"class %s has no configured resources",(tmpClass->Name == NULL) ? "" : tmpClass->Name);

        if (CheckClassResources == FALSE)
          {
          /* don't return a failure if the class exists but has no nodes. */

          MMBAdd(&J->MessageBuffer,EMsg,NULL,mmbtNONE,0,0,NULL);
          }
        else
          {
          /* requested class does not exist in specified partition */
  
          return(FAILURE);
          }
        }
      }
    }

  if (bmisset(&J->IFlags,mjifHostList) && 
      !MNLIsEmpty(&J->ReqHList))
    {
    mnode_t *N;

    MNLGetNodeAtIndex(&J->ReqHList,0,&N);

    /* simple check, determine partition of first node in hostlist */

    if (N->PtIndex != P->Index)
      {
      /* requested hostlist does not match specified partition */

      if (!bmisset(&J->Flags,mjfSystemJob) &&
          !bmisset(&J->Flags,mjfFragment))
        {
        if (EMsg != NULL)
          strcpy(EMsg,"missing hostlist nodes");

        return(FAILURE);
        }
      }
    }  /* END if (bmisset(&J->Flags,mjfHostList) && (J->ReqHList != NULL)) */

  if ((!bmisclear(&J->Req[0]->ReqFBM)) &&
      (MAttrSubset(&P->FBM,&J->Req[0]->ReqFBM,J->Req[0]->ReqFMode) != SUCCESS))
    {
    mbool_t MissingFeature = TRUE;

    /* requested feature does not exist within partition */

    /* check to see if we can map the feature to a partition, in the case we have not loaded the partition yet */

    if ((MSched.MapFeatureToPartition == TRUE) &&
        (P->ConfigNodes == 0))
      {
      char tmpLine[MMAX_LINE];
      char *ptr;
      char *TokPtr;

      /* we have not had a successful workload query for this partition yet so we may not
       * have setup the partition features. If the requested feature is the same as the 
       * partition name then assume that the feature matches the partition so we are not 
       * missing the feature after all. */

      MUNodeFeaturesToString(',',&J->Req[0]->ReqFBM,tmpLine);

      ptr = MUStrTok(tmpLine,",",&TokPtr);

      while (ptr != NULL)
        {
        /* see if the feature matches the partition name */

        if (!strcmp(ptr,P->Name))
          {
          MissingFeature = FALSE;
          break;
          }
        ptr = MUStrTok(NULL,",",&TokPtr);
        }
      }

    if (MissingFeature == TRUE)
      {
      if (EMsg != NULL)
        strcpy(EMsg,"missing feature");

      return(FAILURE);
      }
    }

  /* put FSTree check here? */
 
  return(SUCCESS);
  }  /* END MJobCheckParFeasibility() */





/**
 * According to policy, determine whether job is eligible for
 * queuetime during the coming UI cycle 
 *
 * @param J (I)
 * @param P (I)
 */

int MJobCheckPriorityAccrual(

  mjob_t *J,
  mpar_t *P)

  {
  mbitmap_t BM;
  mbitmap_t *JobPrioExceptions = NULL;

  /* NOTE:  if a job SHOULD accrue queuetime then set the NoViolation flag
            and the job will be updated the next iteration.  If neither the job
            does not have the NoViolation flag or the UIQ flag then the job
            will not accrue queuetime */

  if (J == NULL)
    {
    return(FAILURE);
    }

  if ((J->Credential.Q != NULL) && (!bmisclear(&J->Credential.Q->JobPrioExceptions)))
    JobPrioExceptions = &J->Credential.Q->JobPrioExceptions;
  else
    JobPrioExceptions = &MPar[0].JobPrioExceptions;

  /* check all policies, if we get to the end without exiting early then set the
     NoViolation flag */

  if (bmisset(&J->IFlags,mjifUIQ))
    {
    return(SUCCESS);
    }

  if (bmisset(&J->Hold,mhUser) &&
      !bmisset(JobPrioExceptions,mjpeUserhold))
    {
    return(FAILURE);
    }

  if (bmisset(&J->Hold,mhBatch) &&
      !bmisset(JobPrioExceptions,mjpeBatchhold))
    {
    return(FAILURE);
    }

  if (bmisset(&J->Hold,mhSystem) &&
      !bmisset(JobPrioExceptions,mjpeSystemhold))
    {
    return(FAILURE);
    }

  if (bmisset(&J->Hold,mhDefer) &&
      !bmisset(JobPrioExceptions,mjpeDefer))
    {
    return(FAILURE);
    }

  if (!bmisset(JobPrioExceptions,mjpeDepends)   &&
      (MJobCheckDependency(J,NULL,FALSE,NULL,NULL) == FAILURE))
    {
    return(FAILURE);
    }

  /* FIXME: how do we check per partitions limits like FSTree? */

  bmset(&BM,mlActive);

  if (!bmisset(JobPrioExceptions,mjpeSoftPolicy) &&
      (MJobCheckLimits(
         J,
         mptSoft,
         &MPar[0],
         &BM,
         NULL,
         NULL) == FAILURE))
    {
    return(FAILURE);
    }

  if (!bmisset(JobPrioExceptions,mjpeHardPolicy) &&
      (MJobCheckLimits(
         J,
         mptHard,
         &MPar[0],
         &BM,
         NULL,
         NULL) == FAILURE))
    {
    return(FAILURE);
    }

  bmclear(&BM);
  bmset(&BM,mlIdle);

  if (!bmisset(JobPrioExceptions,mjpeIdlePolicy) &&
      (MJobCheckLimits(
         J,
         mptHard,
         &MPar[0],
         &BM,
         NULL,
         NULL) == FAILURE))
    {
    return(FAILURE);
    }

  bmset(&J->IFlags,mjifNoViolation);

  return(SUCCESS);
  }  /* END MJobCheckPriorityAccrual() */



/**
 *
 * @see MJobEvaluateParAccess - peer
 *
 * @param J 
 * @param P 
 * @param RIndexP 
 * @param EMsg 
 */ 

int MJobCheckParLimits(

  const mjob_t       *J,
  const mpar_t       *P,
  enum MAllocRejEnum *RIndexP,
  char               *EMsg)

  {
  if (bmisset(&J->SpecFlags,mjfRsvMap))
    {
    /* psuedo-rsv jobs require no partition checks */
    return(SUCCESS);
    }

  if (!bmisset(&J->Flags,mjfIgnPolicies))
    {
    /* enforce partition limits */

    if (P->L.JobMaximums[0] != NULL)
      {
      mbool_t Rejected = FALSE;

      mjob_t *JMax;

      JMax = P->L.JobMaximums[0];

      if ((JMax->CPULimit > 0) && (J->CPULimit > JMax->CPULimit))
        {
        if (EMsg != NULL)
          sprintf(EMsg,"partition maxcpu limit %ld exceeded",
            JMax->CPULimit);

        if (RIndexP != NULL)
          *RIndexP = marCPU;

        Rejected = TRUE;
        }

      if ((JMax->PSDedicated > 0) && (J->Request.TC * J->SpecWCLimit[0] > JMax->PSDedicated))
        {
        if (EMsg != NULL)
          sprintf(EMsg,"partition maxps limit %f exceeded",
            JMax->PSDedicated);

        if (RIndexP != NULL)
          *RIndexP = marCPU;
  
        Rejected = TRUE;
        }
  
      if ((JMax->Request.TC > 0) && (MAX(J->TotalTaskCount,J->Request.TC) > JMax->Request.TC))
        {
        if (EMsg != NULL)
          sprintf(EMsg,"partition maxproc limit %d exceeded",
            JMax->Request.TC);
  
        if (RIndexP != NULL)
          *RIndexP = marCPU;
  
        Rejected = TRUE;
        }
 
      if (JMax->Request.NC > 0)
        {
        if (J->Request.NC > 0)
          {
          if (J->Request.NC > JMax->Request.NC)
            {
            if (EMsg != NULL)
              sprintf(EMsg,"partition maxnode limit %d exceeded",
                JMax->Request.NC);
   
            if (RIndexP != NULL)
              *RIndexP = marNodeCount;
  
            Rejected = TRUE;
            }
          }
        else
          {
          int tmpNC = (J->Request.TC / MAX(1,P->MaxPPN)) + ((J->Request.TC % MAX(1,P->MaxPPN) > 0) ? 1 : 0);
  
          if (tmpNC > JMax->Request.NC)
            {
            if (EMsg != NULL)
              sprintf(EMsg,"partition maxnode limit %d exceeded",
                JMax->Request.NC);
   
            if (RIndexP != NULL)
              *RIndexP = marNodeCount;
  
            Rejected = TRUE;
            }
          }
        }    /* END if (JMax->Request.NC > 0) */
 
      if ((JMax->SpecWCLimit[0] > 0) && (J->SpecWCLimit[0] > JMax->SpecWCLimit[0]))
        {
        if (EMsg != NULL)
          {
          char TString[MMAX_LINE];

          MULToTString(JMax->SpecWCLimit[0],TString);

          sprintf(EMsg,"partition maxwc limit %s exceeded",
            TString);
          }
  
        if (RIndexP != NULL)
          *RIndexP = marTime;
  
        Rejected = TRUE;
        }

      if (Rejected == TRUE)
        {
        if (!bmisset(&J->IFlags,mjifSubmitEval))
          {
          return(FAILURE);
          }
        else if (bmisset(MJobGetRejectPolicy(J),mjrpIgnore))
          {
          return(FAILURE);
          }
        else if (bmisset(MJobGetRejectPolicy(J),mjrpRetry))
          {
          /* NO-OP */
          }
        else if (bmisset(MJobGetRejectPolicy(J),mjrpCancel))
          {
          return(FAILURE);
          }
        }
      }    /* END if (P->JMax != NULL) */
  
    if (P->L.JobMinimums[0] != NULL)
      {
      mbool_t Rejected = FALSE;

      mjob_t *tJ;
  
      tJ = P->L.JobMinimums[0];
  
      if (J->Request.TC < tJ->Request.TC)
        {
        if (EMsg != NULL)
          strcpy(EMsg,"partition minproc limit violated");
  
        if (RIndexP != NULL)
          *RIndexP = marCPU;
  
        Rejected = TRUE;
        }

      if (tJ->Request.NC > 0)
        {
        if (J->Request.NC > 0)
          {
          if (J->Request.NC < tJ->Request.NC)
            {
            if (EMsg != NULL)
              strcpy(EMsg,"partition minnode limit violated");
  
            if (RIndexP != NULL)
              *RIndexP = marNodeCount;
  
            Rejected = TRUE;
            }
          }
        else
          {
          int tmpNC = (J->Request.TC / MAX(1,P->MaxPPN)) + ((J->Request.TC % MAX(1,P->MaxPPN) > 0) ? 1 : 0);
  
          if (tmpNC < tJ->Request.NC)
            {
            if (EMsg != NULL)
              strcpy(EMsg,"partition minnode limit violated");
   
            if (RIndexP != NULL)
              *RIndexP = marNodeCount;
  
            Rejected = TRUE;
            }
          }
        }
  
      if (J->SpecWCLimit[0] < tJ->SpecWCLimit[0])
        {
        if (EMsg != NULL)
          strcpy(EMsg,"partition minwc limit violated");
  
        if (RIndexP != NULL)
          *RIndexP = marTime;

        Rejected = TRUE;
        }

      if (Rejected == TRUE)
        {
        if (!bmisset(&J->IFlags,mjifSubmitEval))
          {
          return(FAILURE);
          }
        else if (bmisset(MJobGetRejectPolicy(J),mjrpIgnore))
          {
          return(FAILURE);
          }
        else if (bmisset(MJobGetRejectPolicy(J),mjrpRetry))
          {
          /* NO-OP */
          }
        else if (bmisset(MJobGetRejectPolicy(J),mjrpCancel))
          {
          return(FAILURE);
          }
        }
      }    /* END if (P->JMin != NULL) */
    }      /* END if (!bmisset(&J->Flags,mjfIgnPolicies)) */
 
  return(SUCCESS);
  }  /* END MJobCheckParLimits() */





/**
 *
 * @param J 
 * @param JC 
 */ 

int MJobCheckConstraintMatch(

  mjob_t           *J,   /* I */
  mjobconstraint_t *JC)  /* I */

  {
  int aindex;

  mbool_t ValMatch;

  int vindex;

  ValMatch = TRUE;
      
  if ((JC == NULL) || (J == NULL))
    {  
    return(FAILURE);
    }
 
  for (aindex = 0;JC[aindex].AType != mjaNONE;aindex++)
    {
    ValMatch = FALSE;
 
    switch (JC[aindex].AType)
      {
      case mjaJobID:
 
        for (vindex = 0;vindex < MMAX_JOBCVAL;vindex++)
          {
          if (JC[aindex].AVal[vindex][0] == '\0')
            break;
 
          if (!strcmp(JC[aindex].AVal[vindex],J->Name))
            ValMatch = TRUE;
          }  /* END for (vindex) */
 
        break;
 
      case mjaClass:
 
        for (vindex = 0;vindex < MMAX_JOBCVAL;vindex++)
          {
          if (JC[aindex].AVal[vindex][0] == '\0')
            break;
 
          if (strstr(J->Credential.C->Name,JC[aindex].AVal[vindex]))
            ValMatch = TRUE;
          }  /* END for (vindex) */
 
        break;
 
      case mjaGroup:
 
        for (vindex = 0;vindex < MMAX_JOBCVAL;vindex++)
          {
          if (JC[aindex].AVal[vindex][0] == '\0')
            break;
 
          if (!strcmp(JC[aindex].AVal[vindex],J->Credential.G->Name))
            ValMatch = TRUE;
          }  /* END for (vindex) */
 
        break;
 
      case mjaQOS:
 
        for (vindex = 0;vindex < MMAX_JOBCVAL;vindex++)
          {
          if (JC[aindex].AVal[vindex][0] == '\0')
            break;
 
          if (!strcmp(JC[aindex].AVal[vindex],J->Credential.Q->Name))
            ValMatch = TRUE;
          }
 
        break;
 
      case mjaState:
 
        for (vindex = 0;vindex < MMAX_JOBCVAL;vindex++)
          {
          if (JC[aindex].AVal[vindex][0] == '\0')
            break;
 
          if (!strcmp(JC[aindex].AVal[vindex],MJobState[J->State]))
            ValMatch = TRUE;
          }
         
        break;
 
      case mjaUser:
 
        for (vindex = 0;vindex < MMAX_JOBCVAL;vindex++)
          {
          if (JC[aindex].AVal[vindex][0] == '\0')
            break;
 
          if (!strcmp(JC[aindex].AVal[vindex],J->Credential.U->Name))
            ValMatch = TRUE;
          }  /* END for (vindex) */
 
        break;
 
      case mjaAllocNodeList:
 
        {
        /* FIXME: not supported */
#ifdef MNOT
        char AllocHList[1];
        char *ptr;
 
        int   tmpLength;

        AllocHList[0] = '\0';
 
        for (vindex = 0;vindex < MMAX_JOBCVAL;vindex++)
          {
          if (JC[aindex].AVal[vindex][0] == '\0')
            break;
 
          if ((ptr = strstr(AllocHList,JC[aindex].AVal[vindex])) == NULL)
            continue;
 
          /* Potential Match -- Check for beginning, termination, or comma */
 
          tmpLength = strlen(JC[aindex].AVal[vindex]);
 
          if ((ptr > AllocHList) && (*(ptr - 1) != ','))
            continue;
 
          if ((ptr[tmpLength] != '\0') && (ptr[tmpLength] != ','))
            continue;
 
          ValMatch = TRUE;
 
          break;
          }     /* END for (vindex) */
#endif
        }       /* END BLOCK */
 
        break;
 
      case mjaAccount:
 
        for (vindex = 0;vindex < MMAX_JOBCVAL;vindex++)
          {
          if (JC[aindex].AVal[vindex][0] == '\0')
            break;
 
          if (!strcmp(JC[aindex].AVal[vindex],J->Credential.A->Name))
            ValMatch = TRUE;
          }
 
        break;
 
      case mjaRMXString:
 
        for (vindex = 0;vindex < MMAX_JOBCVAL;vindex++)
          {
          if (JC[aindex].AVal[vindex][0] == '\0')
            break;
 
          /* (NYI) */
 
          if (strstr(J->RMXString,JC[aindex].AVal[vindex]))
            ValMatch = TRUE;
          }
 
        break;
 
      case mjaReqAWDuration:
 
        /* NOTE:  also require arch, featurelist (req attrs) */
 
        /* NYI */
 
        break;
 
      case mjaStartTime:
         
        {
        mulong tmpL;

        for (vindex = 0;vindex < MMAX_JOBCVAL;vindex++)
          {
          tmpL = JC[aindex].ALong[vindex];
 
          if (tmpL <= 0)
            break;
 
          switch (JC[aindex].ACmp)
            {
            case mcmpLE:
            default:
 
              if (J->StartTime <= tmpL)
                ValMatch = TRUE;
 
              break;
 
            case mcmpGE:
 
               if (J->StartTime >= tmpL)
                ValMatch = TRUE;
 
              break;
            } /* END switch (JC[aindex].ACmp) */
          }   /* END for (vindex) */
        }
 
        break;
 
      case mjaCompletionTime:
         
        {
        mulong tmpL;

        for (vindex = 0;vindex < MMAX_JOBCVAL;vindex++)
          {
          tmpL = JC[aindex].ALong[vindex];
 
          if (tmpL <= 0)
            break;
 
          switch (JC[aindex].ACmp)
            {
            case mcmpLE:
            default:
 
              if (J->CompletionTime <= tmpL)
                ValMatch = TRUE;
 
              break;
 
            case mcmpGE:
 
               if (J->CompletionTime >= tmpL)
                ValMatch = TRUE;
 
              break;
            } /* END switch (JC[aindex].ACmp) */
          }   /* END for (vindex) */
        }
 
        break;
 
      default:
 
        /* constraint ignored */
 
        break;
      } /* END switch (JC[aindex].AType) */

    if (ValMatch == FALSE)
      {
      return(FAILURE);
      }
    }   /* END for(aindex) */

  return(SUCCESS);
  }  /* END MJobCheckConstraintMatch() */


/* END MJobCheck.c */
