/* HEADER */

/**
 * @file MJobGet.c
 *
 * Contains Job Getter functions
 */



#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 * Query Accounting Manager to determine proper account for specified job.
 *
 * @see MJobGetGroup() - peer
 *
 * @param J (I)
 * @param AP (O)
 */

int MJobGetAccount(

  mjob_t    *J,  /* I */
  mgcred_t **AP) /* O */

  {
  const char *FName = "MJobGetAccount";

  MDB(7,fSTRUCT) MLog("%s(%s,AP)\n",
    FName,
    (J != NULL) ? J->Name : "NULL");

  if ((J == NULL) || (AP == NULL))
    {
    return(FAILURE);
    }

  *AP = NULL;

  /* look in cred defaults */

  /* look for user based defaults */

  if ((J->Credential.U != NULL) &&
      (J->Credential.U->F.ADef != NULL))
    {
    *AP = (mgcred_t *)J->Credential.U->F.ADef;

    return(SUCCESS);
    }

  /* look for group based defaults */

  if ((J->Credential.G != NULL) &&
      (J->Credential.G->F.ADef != NULL))
    {
    *AP = (mgcred_t *)J->Credential.G->F.ADef;
 
    return(SUCCESS);
    }

  /* look for QOS based defaults */

  if ((J->Credential.Q != NULL) &&
      (J->Credential.Q->F.ADef != NULL))
    {
    *AP = (mgcred_t *)J->Credential.Q->F.ADef;
 
    return(SUCCESS);
    }

  /* NOTE:  do not support default QOS based default class */

  /* look for class based defaults */

  if ((J->Credential.C != NULL) &&
      (J->Credential.C->F.ADef != NULL))
    {
    *AP = (mgcred_t *)J->Credential.C->F.ADef;

    return(SUCCESS);
    }

  if (MAM[0].Type != mamtNONE)
    {
    char tmpAccount[MMAX_NAME];

    /* get default account number */
 
    if (MAMAccountGetDefault(J->Credential.U->Name,tmpAccount,NULL) == SUCCESS)
      {
      if ((MAcctFind(tmpAccount,AP) == FAILURE) &&
          (MAcctAdd(tmpAccount,AP) == FAILURE))
        {
        /* cannot create account structure */

        MDB(2,fSTRUCT) MLog("ALERT:    cannot add account '%s' for job '%s'\n",
          tmpAccount,
          J->Name);

        return(FAILURE);
        }

      /* NOTE:  accounting manager user-to-account mapping is authoritative */
      /*        grant account access */

      MULLAdd(&J->Credential.U->F.AAL,tmpAccount,(void *)AP,NULL,NULL);  /* no free required */

      return(SUCCESS);
      }
    else
      {
      MDB(5,fSTRUCT) MLog("WARNING:  cannot get default account for job '%s' from AM\n",
        J->Name);
      }
    }    /* END if (MAM[0].Type != mamtNONE) */

  if ((MSched.DefaultU != NULL) &&
      (MSched.DefaultU->F.ADef != NULL))
    {
    *AP = (mgcred_t *)MSched.DefaultU->F.ADef;

    return(SUCCESS);
    }

  if ((MSched.DefaultG != NULL) &&
      (MSched.DefaultG->F.ADef != NULL))
    {
    *AP = (mgcred_t *)MSched.DefaultG->F.ADef;

    return(SUCCESS);
    }

  if ((MSched.DefaultC != NULL) &&
      (MSched.DefaultC->F.ADef != NULL))
    {
    *AP = (mgcred_t *)MSched.DefaultC->F.ADef;

    return(SUCCESS);
    }
 
  return(SUCCESS);
  }  /* END MJobGetAccount() */






/**
 * Determine group associated with job.
 *
 * @param J (I) [optional]
 * @param SU (I)
 * @param G (O)
 */

int MJobGetGroup(

  mjob_t    *J,  /* I (optional) */
  mgcred_t  *SU, /* I */
  mgcred_t **G)  /* O */

  {
  char tmpLine[MMAX_LINE];

  mgcred_t *U;

  /* examine user to group mapping */

  if ((J != NULL) && (J->Credential.U != NULL))
    {
    U = J->Credential.U;
    }
  else if (SU != NULL)
    {
    U = SU;
    }
  else
    {
    return(FAILURE);
    }
 
  if (U->F.GDef != NULL)
    {
    if (G != NULL)
      *G = (mgcred_t *)U->F.GDef;

    return(SUCCESS);
    }

  if (U->F.GAL != NULL)
    {
    /* step through group access list to determine feasibility (NYI) */

    MGroupAdd(U->F.GAL->Name,G);
  
    return(SUCCESS);
    }   /* END if (U->F.GAL != NULL) */

  /* no default/GAL specified, search OS group definitions */
 
  if ((J != NULL) && 
      (J->Credential.U != NULL) && 
      (MUGNameFromUName(J->Credential.U->Name,tmpLine) == SUCCESS))
    {
    MGroupAdd(tmpLine,G);

    return(SUCCESS);
    }

  return(FAILURE);
  }  /* END MJobGetGroup() */



/**
 * Populate Count with the number of VCs this job belons to which have a throttling policies set.
 *
 * @param J (I)
 * @param Count (O)
 */

int MJobGetVCThrottleCount(

  const mjob_t *J,
  int          *Count)

  {
  mln_t *tmpL;
  mvc_t *tmpVC;

  void *Ptr;

  if (Count != NULL)
    *Count = 0;

  if (!MJobPtrIsValid(J))
    {
    return(FAILURE);
    }

  for (tmpL = J->ParentVCs;tmpL != NULL;tmpL = tmpL->Next)
    {
    tmpVC = (mvc_t *)tmpL->Ptr;

    if (tmpVC == NULL)
      continue;

    Ptr = NULL;

    if (MVCGetDynamicAttr(tmpVC,mvcaThrottlePolicies,&Ptr,mdfOther) == SUCCESS)
      {
      if (Count != NULL)
        (*Count)++;
      }
    }  /* END for (tmpL) */

  return(SUCCESS);
  }  /* END MJobGetVCThrottleCount() */





/**
 * Get RM-specific, short, or long job name based on job and RM attributes.
 *
 * @param J (I) [optional]
 * @param SpecJobName (I) [optional]
 * @param SpecR (I)
 * @param Buffer (O)
 * @param BufLen (I)
 * @param NameType (I)
 */

const char *MJobGetName(

  mjob_t            *J,            /* I (optional) */
  const char        *SpecJobName,  /* I (optional) */
  mrm_t             *SpecR,        /* I */
  char              *Buffer,       /* O */
  int                BufLen,       /* I */
  enum MJobNameEnum  NameType)     /* I */

  {
  static char tmpBuffer[MMAX_BUFFER];

  char  BaseJName[MMAX_BUFFER];

  int   Len;
  int   RMType;

  char *Head;
  char *ptr;

  mrm_t *R;

  if (Buffer != NULL)
    Buffer[0] = '\0';

  if (SpecJobName != NULL)
    {
    MUStrCpy(BaseJName,SpecJobName,sizeof(BaseJName));
    }
  else if (MJobPtrIsValid(J))
    {
    strcpy(BaseJName,J->Name);
    }
  else
    {
    if (Buffer != NULL)
      strcpy(Buffer,"NULL");

    return("NULL");
    }

  R = (SpecR != NULL) ? SpecR : &MRM[0];

  switch (R->Type)
    {
    case mrmtNative:
   
      if ((R->SubType == mrmstXT3) || (R->SubType == mrmstXT4))
        {
        if ((NameType == mjnRMName) && (J->DRMJID != NULL))
          {
          if (Buffer != NULL)
            MUStrCpy(Buffer,J->DRMJID,BufLen);

          return(J->DRMJID);
          }

        RMType = mrmtPBS;
        }
      else
        {
        RMType = mrmtNative;
        }

      break;

    case mrmtNONE:

      RMType = MDEF_RMTYPE;

      break;

    default:

      RMType = R->Type;

      break;
    }  /* END switch (R->Type) */

  if ((RMType == mrmtPBS) &&
      ((SpecJobName != NULL) &&
       (isalpha(SpecJobName[0]))))
    {
    RMType = mrmtS3;
    }

  if (Buffer != NULL)
    {
    Head = Buffer;
    Len  = BufLen;
    }
  else
    {
    Head = tmpBuffer;
    Len  = sizeof(tmpBuffer);
    }

  Head[0] = '\0';

  if (R->DefaultQMDomain[0] == '\0')
    {
    /* obtain default domain name */

    MDB(5,fSTRUCT) MLog("INFO:     default QM domain not specified for RM '%s' (Type: %s) - attempting to determine domain\n",
      R->Name,
      MRMType[RMType]);

    switch (RMType)
      {
      case mrmtLL:

        {
        char *ptr;
        char *Head;

        char tmpName[MMAX_NAME];

        /* FORMAT:  <HOST>[.<DOMAIN>].<JOBID>.<STEPID> */

        MUStrCpy(tmpName,BaseJName,MMAX_NAME);

        /* ignore hostname */

        if ((ptr = strchr(tmpName,'.')) == NULL)
          break;

        Head = ptr + 1;

        /* remove job/step id */

        if ((ptr = strrchr(Head,'.')) == NULL)
          break;

        *ptr = '\0';

        if ((ptr = strrchr(Head,'.')) == NULL)
          break;

        *ptr = '\0';

        if (strlen(Head) <= 1) 
          break;

        MUStrCpy(R->DefaultQMDomain,Head,Len);
        }  /* END BLOCK (case mrmtLL) */
 
        break;
 
      case mrmtPBS:

        /* NO-OP */

        break;

      case mrmtNative:

        /* NO-OP */

        break;
 
      default:

        /* NO-OP */
 
        break;
      }  /* END switch (RMType) */

    if (R->DefaultQMDomain[0] == '\0')
      {
      /* domain name could not be determined */

      if (MSched.DefaultDomain[0] != '\0')
        {
        MDB(5,fSTRUCT) MLog("INFO:     setting default QM domain for RM '%s' to '%s' - using scheduler default domain\n",
          R->Name,
          MSched.DefaultDomain);

        MUStrCpy(R->DefaultQMDomain,&MSched.DefaultDomain[1],sizeof(R->DefaultQMDomain));
        }
      }    /* END if (R->DefaultQMDomain[0] == '\0') */
    }      /* END if (R->DefaultQMDomain[0] == '\0') */

  if (R->DefaultQMHost[0] == '\0')
    {
    MDB(5,fSTRUCT) MLog("INFO:     default QM host not specified for RM '%s' (Type: %s) - attempting to determine QM host\n",
      R->Name,
      MRMType[RMType]);

    if (MSched.DefaultQMHost[0] != '\0')
      {
      MDB(5,fSTRUCT) MLog("INFO:     setting default QM host for RM '%s' to '%s' - using scheduler default QM host\n",
        R->Name,
        MSched.DefaultQMHost);

      MUStrCpy(R->DefaultQMHost,MSched.DefaultQMHost,sizeof(R->DefaultQMHost));
      }
    else
      {
      switch (RMType)
        {
        case mrmtPBS:

          /* format:  <JOBID>.<QMHOST> */

          if ((ptr = strchr(BaseJName,'.')) != NULL)
            {
            MDB(5,fSTRUCT) MLog("INFO:     setting default QM host for RM '%s' to '%s' - using domain from job '%s'\n",
              R->Name,
              ptr + 1,
              BaseJName);

            MUStrCpy(R->DefaultQMHost,ptr + 1,Len);
            }
          else
            {
            MDB(5,fSTRUCT) MLog("INFO:     setting default QM host for RM '%s' to '%s' - using scheduler server host\n",
              R->Name,
              MSched.ServerHost);

            MUStrCpy(R->DefaultQMHost,MSched.ServerHost,sizeof(R->DefaultQMHost));
            }

          break;
  
        case mrmtLL:

          /* NO-OP */

          break;

        case mrmtNative:

          switch (R->SubType)
            {
            case mrmstXT4:

              /* NOTE:  TORQUE-NATIVE Hybrid */

              /* format:  <JOBID>.<QMHOST> */

              if ((ptr = strchr(BaseJName,'.')) != NULL)
                {
                MDB(5,fSTRUCT) MLog("INFO:     setting default QM host for RM '%s' to '%s' - using domain from job '%s'\n",
                  R->Name,
                  ptr + 1,
                  BaseJName);

                MUStrCpy(R->DefaultQMHost,ptr + 1,Len);
                }
              else
                {
                MDB(5,fSTRUCT) MLog("INFO:     setting default QM host for RM '%s' to '%s' - using scheduler server host\n",
                  R->Name,
                  MSched.ServerHost);

                MUStrCpy(R->DefaultQMHost,MSched.ServerHost,sizeof(R->DefaultQMHost));
                }

              break;
 
            default:

              /* NO-OP */

              break;
            }  /* END switch (R->SubType) */

          break;

        default:

          /* NO-OP */

          break;
        }  /* END switch (RMType) */
      }    /* END else */
    }      /* END if (R->DefaultQMHost[0] == '\0') */

  switch (NameType)
    {
    case mjnShortName: 

      switch (RMType)
        {
        case mrmtLL:

          {
          char *ptr;
          char *TokPtr;

          char  tmpName[MMAX_NAME];

          char *Host;
          char *JobID;
          char *StepID;

          MUStrCpy(tmpName,BaseJName,MMAX_NAME);
        
          ptr = MUStrTok(tmpName,".",&TokPtr);

          /* FORMAT:  <HOST>[.<DOMAIN>].<JOBID>.<STEPID> */

          Host = ptr;

          JobID = NULL;
          StepID = NULL;

          while (ptr != NULL)
            {
            JobID = StepID;

            StepID = ptr;

            ptr = MUStrTok(NULL,".",&TokPtr);
            }  /* END while(ptr != NULL) */
 
          if ((Host == NULL) || (JobID == NULL) || (StepID == NULL))
            {
            MUStrCpy(Head,BaseJName,Len);
            }
          else
            {
            sprintf(Head,"%s.%s.%s",
              Host,
              JobID,
              StepID);
            }
          }    /* END BLOCK */

          break;

        case mrmtPBS:

          /* FORMAT:  <JOBID>[.HOST[.<DOMAIN>]] */

          if ((ptr = strchr(BaseJName,'.')) != NULL)
            MUStrCpy(Head,BaseJName,MIN(ptr - BaseJName + 1,Len));
          else
            MUStrCpy(Head,BaseJName,Len);

          break;

        default:

          /* i.e., WIKI, etc */
 
          /* copy entire name */
 
          MUStrCpy(Head,BaseJName,Len);

          break;
        }  /* END switch (RMType) */
   
      break;

    case mjnSpecName:

      if (J != NULL)
        {
        if (J->AName != NULL)
          {
          MUStrCpy(Head,J->AName,Len);
          }  /* END if (J->AName != NULL) */
        else
          {
          MUStrCpy(Head,J->Name,Len);
          }
        }

      break;

    case mjnFullName: 
    case mjnRMName:
    default:

      if ((NameType == mjnRMName) && bmisset(&J->Flags,mjfCoAlloc))
        {
        int rqindex;
        mbool_t Found = FALSE;
    
        for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
          {
          if (J->Req[rqindex] == NULL)
            break;

          if ((R->Index == J->Req[rqindex]->RMIndex) && 
              (J->Req[rqindex]->ReqJID != NULL))
            {
            MUStrCpy(Head,J->Req[rqindex]->ReqJID,Len);

            Found = TRUE;

            break;
            }
          }    /* END for (rqindex) */

        if (Found == TRUE)
          break;
        }  /* END if ((NameType == mjnRMName) && bmisset(&J->Flags,mjfCoAlloc)) */

      if ((J != NULL) && (J->DRMJID != NULL))
        {
        MUStrCpy(Head,J->DRMJID,Len);

        break;
        }

      switch (RMType)
        {
        case mrmtLL:

          /* FORMAT:  <HOST>[.<DOMAIN>].<JOBID>.<STEPID> */

          if ((R->DefaultQMDomain[0] == '\0') || 
              (strstr(BaseJName,R->DefaultQMDomain) != NULL))
            {
            /* no QM domain or QM domain already in job name */

            MUStrCpy(Head,BaseJName,Len);
            }
          else
            {
            char *ptr;

            char *BPtr = NULL;
            int   BSpace = 0;

            if ((ptr = strchr(BaseJName,',')) == NULL)
              {
              MUStrCpy(Head,BaseJName,Len);

              break;
              }

            /* insert domain name */
 
            MUSNInit(&BPtr,&BSpace,Head,Len);
 
            MUStrCpy(Head,BaseJName,(ptr - BaseJName));
 
            BSpace -= strlen(Head);
            BPtr   += strlen(Head);
 
            MUSNPrintF(&BPtr,&BSpace,"%s%s",
              R->DefaultQMDomain,
              ptr);
            }  /* END else (R->DefaultQMDomain[0] == '\0') */

          break;

        case mrmtPBS:
        default:

          /* FORMAT:  <JOBID>.<QMHOST> */

          MUStrCpy(Head,BaseJName,Len);

          /* append domain */
 
          if ((R->DefaultQMHost[0] != '\0') &&
              (strchr(Head,'.') == NULL))
            {
            MUStrCat(Head,".",Len);
            MUStrCat(Head,R->DefaultQMHost,Len);
            }

          break;
        }  /* END switch (RMType) */

      break;
    }  /* END switch (NameType) */
 
  return(Head);
  }  /* END MJobGetName() */



/**
 * Calculate job 'run' priority for active/suspended job.
 *
 * @see MJobCalcStartPriority()
 * @see MJobCheckDynJobTargets() - child
 *
 * @param J (I)
 * @param PIndex (I)
 * @param Priority (O) [optional]
 * @param UsageFactor (O) [optional]
 * @param Buffer (O) [optional]
 */

int MJobGetRunPriority(

  mjob_t *J,           /* I */
  int     PIndex,      /* I */
  double *Priority,    /* O (optional) */
  double *UsageFactor, /* O (optional) */
  char   *Buffer)      /* O (optional) */

  {
  double PercentResUsage;

  long   Duration;
  long   WallTime;             

  long   ResourcesRequired;
  long   ResourcesConsumed;
  long   ResourcesRemaining;

  double tmpUFactor;

  long   UConsPSW;
  long   URemPSW;
  long   UPerCPSW;
  long   UExeTimePSW;

  mfsc_t *F;
  mfsc_t *GF;

  if (Priority != NULL)
    *Priority = 0.0;

  if (UsageFactor != NULL)
    *UsageFactor = 0.0;

  if (Buffer != NULL)
    *Buffer = '\0';

  if ((J == NULL) || 
     ((J->State != mjsStarting) && (J->State != mjsRunning)))
    {
    return(FAILURE);
    }

  GF = &MPar[0].FSC;
  F = &MPar[PIndex].FSC;

  /* NOTE:  allow TotalUsage to be determined by WallTime or CPUTime */
  /*        for now, calculate as dedicated PS                       */

  Duration          = J->WCLimit;
  WallTime          = J->AWallTime;
  ResourcesRequired = J->TotalProcCount;

  ResourcesConsumed  = ResourcesRequired * WallTime;
  ResourcesRemaining = ResourcesRequired * (Duration - WallTime);

  PercentResUsage    = WallTime * 100 / MAX(1,Duration - WallTime);

  UConsPSW  = (F->PSW[mpsUCons] != MCONST_PRIO_NOTSET) ? 
     F->PSW[mpsUCons] : GF->PSW[mpsUCons];
 
  URemPSW  = (F->PSW[mpsURem] != MCONST_PRIO_NOTSET) ? 
     F->PSW[mpsURem] : GF->PSW[mpsURem];
 
  UPerCPSW  = (F->PSW[mpsUPerC] != MCONST_PRIO_NOTSET) ? 
     F->PSW[mpsUPerC] : GF->PSW[mpsUPerC];
 
  UExeTimePSW  = (F->PSW[mpsUExeTime] != MCONST_PRIO_NOTSET) ? 
     F->PSW[mpsUExeTime] : GF->PSW[mpsUExeTime];
 
  tmpUFactor = 
     (UConsPSW    * ResourcesConsumed +
      URemPSW     * ResourcesRemaining +
      UPerCPSW    * PercentResUsage +
      UExeTimePSW * (MSched.Time - J->StartTime));

  if (UsageFactor != NULL)
    *UsageFactor = tmpUFactor;

  if (Priority != NULL)
    { 
    double  UsagePCW;

    UsagePCW  = (F->PCW[mpcUsage] != MCONST_PRIO_NOTSET) ? 
      F->PCW[mpcUsage] : GF->PCW[mpcUsage];

    *Priority = UsagePCW * tmpUFactor + 
                J->PStartPriority[PIndex] + 
                MSched.PreemptPrioMargin;
    }
 
  return(SUCCESS);
  }  /* END MJobGetRunPriority() */





/**
 * Determine job backfill priority.
 *
 * @param J (I)
 * @param MaxDuration (I)
 * @param PIndex (I)
 * @param BFPriority (O)
 * @param Buffer (O)
 */

int MJobGetBackfillPriority(

  mjob_t        *J,            /* I */
  unsigned long  MaxDuration,  /* I */
  int            PIndex,       /* I */
  double        *BFPriority,   /* O */
  char          *Buffer)       /* O */

  {
  double WCAccuracy = 0.0;
  long   Duration;

  if ((J == NULL) || (BFPriority == NULL))
    {
    return(FAILURE);
    }

  /* Need probability of success info */
  /* May need idle job priority       */

  Duration = J->WCLimit;

  switch (MPar[0].BFPriorityPolicy)
    { 
    case mbfpDuration:

      *BFPriority = MMAX_TIME - Duration;

      break;

    case mbfpHWDuration:

      MJobGetWCAccuracy(J,&WCAccuracy);

      *BFPriority = (double)MMAX_TIME - WCAccuracy * Duration;

      break;

    case mbfpRandom:
    default:
 
      *BFPriority = 1;
 
      break;
    }  /* END switch (MSched.BFPriorityPolicy) */

  return(SUCCESS);  
  }  /* END MJobGetBackfillPriority() */




/**
 * Report job wallclock accuracy.
 *
 * @param J (I)
 * @param Accuracy (O)
 */

double MJobGetWCAccuracy(

  mjob_t *J,         /* I */
  double *Accuracy)  /* O */

  {
  if ((J == NULL) || (Accuracy == NULL))
    {
    return(FAILURE);
    }

  if ((J->Credential.U != NULL) && (J->Credential.U->Stat.JC > 0))
    {
    *Accuracy = J->Credential.U->Stat.JobAcc / J->Credential.U->Stat.JC;
    }
  else if ((J->Credential.G != NULL) && (J->Credential.G->Stat.JC > 0))
    {
    *Accuracy = J->Credential.G->Stat.JobAcc / J->Credential.G->Stat.JC;
    }
  else if ((J->Credential.A != NULL) && (J->Credential.A->Stat.JC > 0))
    {
    *Accuracy = J->Credential.A->Stat.JobAcc / J->Credential.A->Stat.JC;
    }
  else if (MPar[0].S.JC > 0)
    {
    *Accuracy = MPar[0].S.JobAcc / MPar[0].S.JC;
    }
  else
    {
    *Accuracy = 1.00;
    }
  
  return(SUCCESS);  
  }  /* END MJobGetWCAccuracy() */





/**
 * Calculates a job's processor equivalent.
 *
 * @param J (I)
 * @param P (I)
 * @param PEptr (O)
 */

int MJobGetPE(
 
  const mjob_t  *J,
  const mpar_t  *P,
  double        *PEptr)
 
  {
  int     rqindex;
 
  mreq_t *RQ;

  double  PE; 
  double  TotalPE;
 
  TotalPE = 0.0;

  if ((J == NULL) || (P == NULL))
    {
    if (PEptr != NULL)
      *PEptr = 0.0;
 
    return(FAILURE);
    }
 
  if (P->CRes.Procs == 0)
    {
    if (PEptr != NULL) 
      *PEptr = 0.0;

    return(SUCCESS);
    }
 
  for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
    {
    RQ = J->Req[rqindex];

    if (RQ->DRes.Procs >= 0) 
      PE = (double)RQ->DRes.Procs / P->CRes.Procs;
    else
      PE = MAX(1,P->MaxPPN);
 
    if ((RQ->DRes.Mem > 0) && (P->CRes.Mem > 0))
      PE = MAX(PE,(double)RQ->DRes.Mem  / P->CRes.Mem);
 
    if ((RQ->DRes.Disk > 0) && (P->CRes.Disk > 0))
      PE = MAX(PE,(double)RQ->DRes.Disk / P->CRes.Disk);
 
    if ((RQ->DRes.Swap > 0) && (P->CRes.Swap > 0))
      PE = MAX(PE,(double)RQ->DRes.Swap / P->CRes.Swap);
 
    TotalPE += (RQ->TaskCount * PE);
    }  /* END for (rqindex) */
 
  TotalPE *= P->CRes.Procs;

  if (PEptr != NULL)
    *PEptr = TotalPE;
 
  return(SUCCESS);
  }  /* END MJobGetPE() */




/**
 *
 *
 * @param J (I)
 * @param NL (O)
 */

int MJobGetNL(

  mjob_t    *J,  /* I */
  mnl_t     *NL) /* O */
 
  {
  int rqindex;

  const char *FName = "MJobGetNL";
 
  MDB(5,fSTRUCT) MLog("%s(%s,NL)\n",
    FName,
    (J != NULL) ? J->Name : "NULL");

  if (J == NULL)
    {
    return(FAILURE);
    }
 
  for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
    {
    if (J->Req[rqindex] == NULL)
      continue; 
 
    MNLMerge(NL,NL,&J->Req[rqindex]->NodeList,NULL,NULL);
    }    /* END for (rqindex) */
 
  return(SUCCESS);
  }  /* END JobGetNL() */



    
/**
 * Report earliest/best possible job start time.
 *
 * @see MJobReserve() - parent - calls this routine to identify earliest time 
 *                      rsv can start
 * @see MJobGetRange() - child - locate eligible time/eligible resources for job 
 *
 * NextETime added 5/29/09 by DRW - to address failing case for multi-req jobs
 * this routine will make 2 calls to MJobGetRange() -- the first is to get the
 * best possible starttime, the second call will use that starttime to get a
 * nodelist.  This nodelist is then given to MJobDistributeMNL() to ensure that
 * multi-req jobs get exclusive resources (among reqs).  However, the first call
 * to MJobGetRange() is done on a per-req basis and there are cases where each
 * req will get an individual starttime of "now" but together they cannot start
 * "now".  The subsequent call to MJobGetRange() will not return enough hosts
 * (because they aren't available "now") and MJobDistributeMNL() will fail and
 * defer the job.  To address this we return NextETime, which is the next
 * event found from the first call to MJobGetRange().  If the multi-req job
 * fails to find a time "now" we will return the next possible event-time and 
 * this routine can be called again with a modified EStartTime (see MJobReserve)
 *
 * @param J             (I) job
 * @param PP            (I/O) constraining/best partition [optional]
 * @param NodeCount     (O) number of nodes located [optional]
 * @param TaskCount     (O) tasks located [optional]
 * @param MNodeList     (O) list of best nodes to execute job [optional]
 * @param EStartTime    (I/O) earliest start time possible [absolute]
 * @param NextETime     (O) next event time for this job discovered via MJobGetRange() [optional]
 * @param DoRemote      (I) consider remote partitions for earliest start time [SLAVEs not considered remote]
 * @param CheckParAvail (I) check partition availability
 * @param EMsg          (O) [optional,minsize=MMAX_LINE]
 */

int MJobGetEStartTime(
 
  mjob_t      *J,
  mpar_t     **PP,
  int         *NodeCount,
  int         *TaskCount,
  mnl_t      **MNodeList,
  long        *EStartTime,
  long        *NextETime,
  mbool_t      DoRemote,
  mbool_t      CheckParAvail,
  char        *EMsg)
 
  {
  int    rindex;
  int    pindex;
  int    nindex;
  int    rqindex;
 
  mulong JobStartTime;
  mulong JobEndTime;

  int    JobTC;    /* total tasks found for job within a single partition */

  int    ReqJobTC;
 
  int    TC;
 
  mbool_t DoAllocation = TRUE;

  mnl_t        *tmpMNodeList[MMAX_REQ_PER_JOB];
 
  int           tmpNodeCount;
  int           tmpTC;

  int RangeValue = 0;
  int BestRangeValue;

  mulong        MaxETime;
  mulong        ETime;
 
  mreq_t       *RQ;
  
  mrange_t      GRL[1][MMAX_RANGE];
  mrange_t      TRange[MMAX_RANGE];
  mrange_t      RRange[MMAX_REQ_PER_JOB];
 
  mbitmap_t     RTBM;
 
  char         *NodeMap = NULL;
  char         *tmpNodeMap = NULL;

  int           NIndex;
 
  int           BestTC;
  int           BestNC;
  mulong        BestStartTime;
  mulong        SpecStartTime;
  int           BestFeature;
  int           REIndex = 0;

  int           rc;

  mbool_t       BestIsPartial;
  mbool_t       NodeSetIsOptional;

  char          TString[MMAX_LINE]; /* Used for calls to MULToTString() */
 
  enum MResourceSetSelectionEnum NodeSetPolicy;

  char *DownRMID = NULL;

  mrange_t      BestRange[MMAX_RANGE];
 
  mcres_t       DRes;

  mpar_t       *P;
  mpar_t       *PS;
  mpar_t       *EP = NULL;    

  char         *BPtr;
  int           BSpace;

  mnl_t     *FNL = NULL; /* not used yet. Allows context-sensitive 
    resource constraints to be passed into MJobGetRange */

  enum MNodeAllocPolicyEnum NAPolicy;

  const char *FName = "MJobGetEStartTime";

  MDB(4,fSCHED) MLog("%s(%s,%s,NodeCount,TaskCount,MNodeList,%ld,EMsg)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    ((PP != NULL) && (*PP != NULL)) ? (*PP)->Name : "NULL",
    (EStartTime != NULL) ? *EStartTime : -1);

  if (EMsg != NULL)
    {
    MUSNInit(&BPtr,&BSpace,EMsg,MMAX_LINE);
    }

  if ((J == NULL) || (J->Req[0] == NULL) || (EStartTime == NULL))
    {
    if (EStartTime != NULL)
      {
      *EStartTime = MMAX_TIME;
      }

    return(FAILURE);
    }

  /* Step 1.0  Initialize */

  SpecStartTime = MAX((long)MSched.Time,*EStartTime);
  *EStartTime   = MMAX_TIME;

  RRange[0].STime = SpecStartTime;
  RRange[0].ETime = MMAX_TIME;
  RRange[1].ETime = 0;

  /* NOTE:  initialize local memory, BE SURE TO FREE */

  MCResInit(&DRes);

  MNLMultiInit(tmpMNodeList);

  if ((MNodeMapInit(&NodeMap) == FAILURE) ||
      (MNodeMapInit(&tmpNodeMap) == FAILURE))
    {
    return(FAILURE);
    }

  /* apply min starttime time constraints */
  /* apply partition/partition access space constraints */
  /* apply reservation time/space constraints */
  /* apply resource requirement space constraints */
  /* apply locality/nodeset space constraints */
  /* return best start time */
  /* return resource availability quantity */
  /* return feasible partition/node list */

  if ((PP != NULL) && (*PP != NULL) && ((*PP)->Index > 0))
    {
    PS = *PP;
    }
  else
    {
    PS = NULL;
    }
 
  BestTC = 0;
  BestStartTime = MMAX_TIME;
  BestIsPartial = FALSE;
  BestRangeValue = MMAX_TIME;
 
  memset(NodeMap,mnmUnavailable,sizeof(NodeMap));
 
  if (NodeCount != NULL)
    *NodeCount = 0;
 
  if (TaskCount != 0)
    *TaskCount = 0;

  RQ = J->Req[0];

  ReqJobTC = J->Request.TC;

  if (bmisset(&J->IFlags,mjifSpecNodeSetPolicy))
    NodeSetPolicy = J->Req[0]->SetSelection;
  else
    NodeSetPolicy = MPar[0].NodeSetPolicy;

  /* determine if NodeSets are optional */

  NodeSetIsOptional = TRUE;

  if (J->Req[0]->SetBlock != MBNOTSET)
    {
    NodeSetIsOptional = !J->Req[0]->SetBlock;
    }
  else if (MPar[0].NodeSetIsOptional == FALSE)
    {
    NodeSetIsOptional = FALSE;
    }

  if (NodeSetPolicy == mrssNONE)
    NodeSetIsOptional = TRUE;

  /* Step 2.0  Determine Best Partition and Earliest Start */
 
  /* Step 2.1  Evaluate all partitions */

  for (pindex = 1;pindex < MMAX_PAR;pindex++)
    {
    P = &MPar[pindex];

    if (P->Name[0] == '\0')
      break;

    if (P->Name[0] == '\1')
      continue;

    /* Step 2.2  Filter Invalid Partitions */

    if ((PS != NULL) && (PS != P))
      continue;

    if (P->ConfigNodes == 0)
      {
      if ((P->RM != NULL) && (P->RM->State == mrmsDown))
        DownRMID = P->RM->Name;

      continue;
      }

    MDBO(7,J,fSCHED) MLog("INFO:     searching for job (%s) starttime on partition '%s'\n",
      J->Name,
      P->Name);

    /* check partition access */

    if (bmisset(&J->PAL,P->Index) == FAILURE)
      {
      char ParLine[MMAX_LINE];

      MDB(7,fSCHED) MLog("INFO:     job %s not allowed to run in partition %s (allowed %s)\n", 
        J->Name,
        P->Name,
        MPALToString(&J->PAL,NULL,ParLine));

      continue;
      }

    if ((!bmisset(&J->IFlags,mjifIsTemplate)) &&
        (MJobEvaluateParAccess(J,P,NULL,NULL) == FAILURE))
      {
      MDB(7,fSCHED) MLog("INFO:     job %s not allowed to run in partition %s (FSTree constraints)\n", 
        J->Name,
        P->Name);

      continue;
      }

    if ((!bmisset(&J->IFlags,mjifIsTemplate)) &&
        (MJobCheckParLimits(J,P,NULL,NULL) == FAILURE))
      {
      MDB(7,fSCHED) MLog("INFO:     job %s not allowed to run in partition %s (partition limits)\n", 
        J->Name,
        P->Name);

      continue;
      }

    if ((DoRemote == FALSE) &&
        (P->RM != NULL) &&
        (P->RM->Type == mrmtMoab) &&
        !bmisset(&P->RM->Flags,mrmfSlavePeer))
      {
      /* SLAVE peers are not considered remote in this case */

      continue;
      }

    bmset(&RTBM,mrtcNONE);
 
    JobStartTime = SpecStartTime;
 
    /* bound range of events that will be processed */

    ETime    = JobStartTime;
    MaxETime = MMAX_TIME;

    if (J->SysSMinTime != NULL)
      JobStartTime = MAX(JobStartTime,J->SysSMinTime[P->Index]);
 
    JobEndTime   = MMAX_TIME;
  
    if (NextETime != NULL)
      *NextETime = MMAX_TIME;

    JobTC        = 0;

    /* partition is acceptable - feasible node list determined - time range bounded */

    while (ETime < MaxETime)
      {
      /* Step 2.2  Evaluate Feasible Resource Availability for all Job Reqs */
 
      for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
        {
        mpar_t *tmpP;

        RQ = J->Req[rqindex];
 
        if (RQ == NULL)
          break;

        tmpP = P;
 
        /* check resource availability */
 
        if (RQ->DRes.Procs > 0)
          {
          /* NOTE:  must support check for jobs which require both standard and
                    shared partition resources in single req (NYI) */
 
          TC = MParGetTC(
            tmpP,
            &tmpP->ARes,
            &tmpP->CRes,
            &DRes,
            &RQ->DRes,
            (CheckParAvail == TRUE) ? MSched.Time : MMAX_TIME);
 
          /* NOTE:  should compare against RQ->TaskCount in if statement below (NYI) */
 
          if (TC < J->Request.TC)
            {
            if ((TC == 0) || !bmisset(&J->SpecFlags,mjfBestEffort))
              {
              MDB(7,fSCHED) MLog("INFO:     tasks not available for job %s:%d in partition %s (%d of %d tasks)\n",
                J->Name,
                RQ->Index,
                P->Name,
                TC,
                J->Request.TC);
 
              ETime = MMAX_TIME;
  
              MaxETime = 0;

              /* partition is invalid for req - do not evaluate remaining reqs */

              break;
              }
            }
          }    /* END if (RQ->DRes.Procs > 0) */
 
        /* check class availability */
 
        if ((J->Credential.C != NULL) && !bmisset(&P->Classes,J->Credential.C->Index))
          {
          MDB(7,fSCHED) MLog("INFO:     class not available for job %s:%d in partition %s\n",
            J->Name,
            RQ->Index,
            P->Name);

          ETime = MMAX_TIME;

          MaxETime = 0;

          /* partition is invalid for req - do not evaluate remaining reqs */

          break;
          }
 
        /* Step 2.3.1 Determine Earliest Starttime for Req */
 
        if (MSched.NodeSetPlus == mnspDelay)
          {
          rc = MJobGetRangeWrapper(
                 J,
                 RQ,
                 FNL,
                 P,
                 JobStartTime,    /* I (earliest time job can start) */
                 GRL[0],          /* O */
                 NULL,                                                  
                 &tmpNodeCount,   /* O */
                 NodeMap,         /* O */
                 &RTBM,
                 MSched.SeekLong,
                 TRange,
                 &BestFeature,
                 NULL);

          /* set flag so job can run in MJobAllocMNL() */

          if (MSched.NodeSetPlus == mnspDelay)
            bmset(&J->IFlags,mjifNodeSetDelay);
          }
        else
          {
          rc = MJobGetRange(
                 J,
                 RQ,
                 FNL,
                 P,
                 JobStartTime,        /* I (earliest time job can start) */
                 FALSE,
                 GRL[0],              /* O */
                 NULL,
                 &tmpNodeCount,       /* O */
                 NodeMap,             /* O */
                 &RTBM,
                 MSched.SeekLong,
                 TRange,
                 NULL);
          }
 
        if (rc == FAILURE)
          {
          /* acceptable range could not be located */
 
          if (EMsg != NULL)
            {
            mnl_t     NodeList;
 
            int FNC;
            int FTC;
 
            MNLInit(&NodeList);

            /* populate EMsg with host specific rejection reasons */
 
            if (MReqGetFNL(
                  J,
                  RQ,
                  P,
                  &J->ReqHList,
                  &NodeList,    /* O (not used) */
                  &FNC,        /* O */
                  &FTC,        /* O */
                  MMAX_TIME,
                  0,
                  EMsg) == FAILURE)
              {
              /* EMsg already populated */
 
              /* NO-OP */
              }
            else
              {
              MUSNPrintF(&BPtr,&BSpace,"resources not available for specified timeframe");
              }

            MNLFree(&NodeList);
            }
 
          ETime = MMAX_TIME;
  
          MaxETime = 0;

          JobStartTime = MMAX_TIME;

          /* time range for req cannot be located w/in partition - 
             break out of Req loop - do not evaluate remaining reqs */

          break;
          }  /* END if (MJobGetRange() == FAILURE) */

        /* time range successfully located for Req */

        if (J->Req[1] != NULL)
          {
          if ((NextETime != NULL) && (GRL[0][1].ETime != 0))
            *NextETime = MIN((unsigned long)*NextETime,GRL[0][1].STime);

          /* collapse discovered ranges into joined availability range */
 
          MRLMergeTime(
            GRL[0],
            RRange,
            TRUE,
            -1,               /* merge all possible ranges */
            RQ->TaskCount);   /* no proc limit */
          }
 
        JobStartTime  = MAX(JobStartTime,GRL[0][0].STime);
        JobEndTime    = MIN(JobEndTime,GRL[0][0].ETime);
        JobTC        += GRL[0][0].TC;
 
        if (JobStartTime > JobEndTime)
          {
          /* located ranges do not overlap */

          MDB(7,fSCHED) MLog("INFO:     located ranges do not overlap for req %d (%ld > %ld)\n",
            rqindex,
            JobStartTime,
            JobEndTime);

          ETime = MMAX_TIME;

          MaxETime = 0;
 
          break;
          }
        }  /* END for (rqindex) */
 
      if (J->Req[rqindex] != NULL) 
        {
        /* ranges not found for all reqs */
 
        MDB(7,fSCHED) MLog("INFO:     cannot locate range for job %s:%d in partition %s %s\n",
          J->Name,
          rqindex,
          P->Name,
          FName);
 
        continue;
        }

      /* have located time range for given req within given partition which
         meets required time constraints */

      /* NOTE:  for multi-req jobs, job BestTime should be MAX of all per-req 
                BestTimes (which should each be earliest possible per-req 
                starttime) */

      MJobGetRangeValue(
        J,
        P,
        GRL[0],
        &RangeValue,   /* O */
        &MaxETime,     /* O */
        &REIndex,      /* O */
        &DoAllocation, /* O */
        tmpMNodeList,  /* O */
        &BestNC);      /* O */
 
      /* NOTE: change 12/4/08 from ">" to "<" as jobs were RangeValue is usually the earliest starttime
               and it makes sense that the earlier the better */

      /* NOTE: FIXME: 5/29/09 multi-req jobs need to take "the worst of the best" (change "<" to ">") */

      if (RangeValue < BestRangeValue)
        {
        BestRangeValue = RangeValue;
       
        if (REIndex > 0)
          BestStartTime = GRL[0][REIndex].STime;
        else
          BestStartTime = JobStartTime;

        EP             = P;
        BestTC         = JobTC;
 
        if (JobTC < J->Request.TC)
          BestIsPartial = TRUE;
 
        /* NOTE:  if job level or global level node sets are specified, should
                  verify each potential solution has an appropriate nodeset associated */
 
        /* NOTE:  should sort and order top 'N' ranges with their associated partition 
                  once all range info is collected, should walk through ranges, best 
                  range first, and impose nodeset constraints.  Search should continue
                  until acceptable range is found */
 
        /* preserve new earliest range */
 
        if (TRange[0].TC >= J->Request.TC)
          memcpy(BestRange,TRange,sizeof(BestRange)); /* FIXME: only works for single req jobs */
        else
          memcpy(BestRange,GRL[0],sizeof(BestRange));

        MULToTString(BestStartTime - MSched.Time,TString);
 
        MDB(5,fSCHED) MLog("INFO:     start time %s found for job %s in partition %s (%ld)\n",
          TString,
          J->Name,
          EP->Name,
          SpecStartTime);
        }
      }    /* END while (ETime < MMAX_TIME) */
    }      /* END for (pindex) */

  MCResFree(&DRes);

  /* determine if NodeSets are optional */

  NodeSetIsOptional = TRUE;

  if (J->Req[0]->SetBlock != MBNOTSET)
    {
    NodeSetIsOptional = !J->Req[0]->SetBlock;
    }
  else if (MPar[0].NodeSetIsOptional == FALSE)
    {
    NodeSetIsOptional = FALSE;
    }

  if (DoAllocation == FALSE)
    {
    if (MNodeList != NULL)
      {
      MNLCopy(MNodeList[0],tmpMNodeList[0]);

      tmpTC = 0;

      for (nindex = 0;MNLGetNodeAtIndex(MNodeList[0],nindex,NULL) == SUCCESS;nindex++)
        {
        tmpTC += MNLGetTCAtIndex(MNodeList[0],nindex);
        }

      if (NodeCount != NULL)
        *NodeCount = nindex;

      if (TaskCount != NULL)
        *TaskCount = tmpTC;
      }

    *EStartTime = BestStartTime;

    if (PP != NULL) 
      *PP = EP;

    MNLMultiFree(tmpMNodeList);

    MUFree(&NodeMap);
    MUFree(&tmpNodeMap);

    return(SUCCESS);
    }

  if (EP != NULL)
    {
    mbitmap_t BM;

    mnl_t     *AMNL[MMAX_REQ_PER_JOB];

    mbool_t ReqFailed;

    MNLMultiInit(AMNL);

    bmset(&BM,mrtcStartEarliest);

    for (rindex = 0;BestRange[rindex].ETime != 0;rindex++)
      {
      BestTC = 0;
      BestNC = 0;

      ReqFailed = FALSE;

      for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
        {
        /* NOTE:  NodeMap must be logically OR'd list of NodeMaps */

        RQ = J->Req[rqindex];

        if ((MSched.NodeSetPlus == mnspDelay) && 
            (BestFeature >= 0))
          {
          bmset(&RQ->ReqFBM,BestFeature); 

          if (MJobPTransform(J,P,BestFeature,TRUE) == FAILURE)
            {
            ReqFailed = TRUE;

            break;
            }
          }

        /* Second call: get host lists that can start within start ranges */

        /* NOTE:  BestFNL[] must be preserved - FIXME */

        if (MJobGetRange(
              J,
              RQ, 
              FNL,
              EP,
              BestRange[rindex].STime,
              FALSE,
              NULL,                       /* do not pass GRL (not used) */
              tmpMNodeList,               /* O */
              &tmpNodeCount,              /* O */
              tmpNodeMap,                 /* O */
              &BM,
              MSched.SeekLong,
              NULL,
              NULL) == FAILURE)
          {
          /* cannot obtain range nodelist */

          MDB(2,fSCHED) MLog("ALERT:    cannot obtain nodelist for job %s:%d in range %d\n",
            J->Name,
            rqindex,
            rindex);

          /* do not attempt any more reqs */

          ReqFailed = TRUE;

          if ((MSched.NodeSetPlus == mnspDelay) && 
              (BestFeature >= 0))
            {
            bmunset(&RQ->ReqFBM,BestFeature); 

            MJobPTransform(J,P,BestFeature,FALSE);
            }

          break;
          }

        /* NOTE:  If NodeList obtained at specified starttime, and NodeSet 
                  is required, validate NodeList.  If unsuccessful, continue 
                  loop through range */

        if (bmisset(&MSched.Flags,mschedfValidateFutureNodesets))
          {
          if (NodeSetIsOptional == FALSE)
            {
            enum MResourceSetSelectionEnum tmpRSS;

            tmpRSS = (RQ->SetSelection != mrssNONE) ?
              RQ->SetSelection :
              MPar[0].NodeSetPolicy;

            if (MJobSelectResourceSet(
                 J,
                 RQ,
                 BestRange[rindex].STime,
                 (RQ->SetType != mrstNONE) ? RQ->SetType : MPar[0].NodeSetAttribute,
                 (tmpRSS != mrssOneOf) ? tmpRSS : mrssFirstOf,
                 ((RQ->SetList != NULL) && (RQ->SetList[0] != NULL)) ? RQ->SetList : MPar[0].NodeSetList,
                 tmpMNodeList[rqindex],  /* I/O */
                 NodeMap,
                 -1,
                 NULL,
                 NULL,
                 NULL) == FAILURE)
              {
              /* selected range does not support NodeSet constraints */

              MULToTString(BestRange[rindex].STime - MSched.Time,TString);

              MDB(3,fSCHED) MLog("INFO:     nodeset constraints prevent use of range %d for job %s:%d at %s\n",
                rindex,
                J->Name,
                RQ->Index,
                TString);

              BestStartTime = BestRange[rindex].STime;

              ReqFailed = TRUE;

              if ((MSched.NodeSetPlus == mnspDelay) && 
                  (BestFeature >= 0))
                {
                bmunset(&RQ->ReqFBM,BestFeature); 

                MJobPTransform(J,P,BestFeature,FALSE);
                }

              break;
              }  /* END if (MJobSelectResourceSet() == FAILURE) */
            }    /* END if ((MPar[0].NodeSetIsOptional == FALSE) || (RQ->SetSelection != mrssNONE)) */
          }      /* END if (bmisset(&MSched.Flags,ValidateFutureNodeSets)) */
              
        BestNC += tmpNodeCount;

        if (rqindex == 0)
          {
          MNodeMapCopy(NodeMap,tmpNodeMap);

          for (nindex = 0;MNLGetNodeAtIndex(tmpMNodeList[rqindex],nindex,NULL) == SUCCESS;nindex++)
            {
            BestTC += MNLGetTCAtIndex(tmpMNodeList[rqindex],nindex);
            }
          }
        else
          { 
          mnode_t *tmpN;

          for (nindex = 0;MNLGetNodeAtIndex(tmpMNodeList[rqindex],nindex,&tmpN) == SUCCESS;nindex++)
            {
            NIndex = tmpN->Index;

            BestTC += MNLGetTCAtIndex(tmpMNodeList[rqindex],nindex);

            /* merge nodemaps into 'max affinity' nodemap */

            if (NodeMap[NIndex] == mnmUnavailable)
              NodeMap[NIndex] = tmpNodeMap[NIndex];
            else if (tmpNodeMap[NIndex] != mnmUnavailable)
              NodeMap[NIndex] = MIN(NodeMap[NIndex],tmpNodeMap[NIndex]);
            }
          }

        if ((MSched.NodeSetPlus == mnspDelay) && 
            (BestFeature >= 0))
          {
          bmunset(&RQ->ReqFBM,BestFeature); 

          MJobPTransform(J,P,BestFeature,FALSE);
          }
        }    /* END for (rqindex) */

      if (ReqFailed == TRUE)
        {
        /* selected range is not valid - attempt next range */

        continue;
        }

      MULToTString(BestStartTime - MSched.Time,TString);

      MDB(2,fSCHED) MLog("INFO:     located resources for %d tasks (%d) in best partition %s for job %s at time offset %s\n",
        J->Request.TC,
        BestTC,
        EP->Name,
        J->Name,
        TString);

      RQ = J->Req[0];

      /* per job NodeSets implemented 10/25/07 by BC and DRW */

      if (NodeSetIsOptional == FALSE)
        {
        /* impose resource set constraints */

        enum MResourceSetSelectionEnum tmpRSS;

        mnl_t     *NodeList; 

        for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
          { 
          RQ = J->Req[rqindex];

          if (RQ == NULL)
            break;

          NodeList = tmpMNodeList[rqindex];

          /* mandatory node set */

          tmpRSS = (RQ->SetSelection != mrssNONE) ?
            RQ->SetSelection :
            MPar[0].NodeSetPolicy;

          if (MJobSelectResourceSet(
               J,
               RQ,
               BestStartTime,
               (RQ->SetType != mrstNONE) ? RQ->SetType : MPar[0].NodeSetAttribute,
               (tmpRSS != mrssOneOf) ? tmpRSS : mrssFirstOf,
               ((RQ->SetList != NULL) && (RQ->SetList[0] != NULL)) ? RQ->SetList : MPar[0].NodeSetList,
               NodeList,  /* I/O */
               NodeMap,
               -1,
               NULL,
               NULL,
               NULL) == FAILURE)
            {
            MULToTString(BestRange[rindex].STime - MSched.Time,TString);

            MDB(0,fSCHED) MLog("WARNING:  nodeset constraints prevent use of tasks for job %s:%d at %s\n",
              J->Name,
              RQ->Index,
              TString);

            break;
            }
          }  /* END for (rqindex) */

        if ((rqindex < MMAX_REQ_PER_JOB) && (RQ != NULL))
          {
          /* node set constraints not satisfied */

          continue;
          }
        }    /* END if (MPar[0].NodeSetIsOptional == FALSE) */

      /* NOTE:  if best effort specified and inadequate tasks found, 
                req TC should be reduced */

      if (bmisset(&J->SpecFlags,mjfBestEffort))
        {
        if (BestTC < J->Request.TC)
          {
          /* if full req located, do not search out partial req */

          if ((BestStartTime < MMAX_TIME) && (BestIsPartial == FALSE))
            {
            MDB(0,fSCHED) MLog("WARNING:  ignoring partial range - full range previously located\n");

            continue;
            }

          J->Request.TC = BestTC;
          J->Req[0]->TaskCount = BestTC;
          }            
        }

      /* call MJobDistributeMNL */

      NAPolicy = (J->Req[0]->NAllocPolicy != mnalNONE) ?
        J->Req[0]->NAllocPolicy :
        EP->NAllocPolicy;

      if ((MJobDistributeMNL(
            J,
            tmpMNodeList,
            AMNL) == FAILURE) ||
          (MJobAllocMNL(
            J,
            AMNL,
            NodeMap,
            MNodeList,     /* O */
            NAPolicy,
            BestRange[rindex].STime,
            NULL,
            NULL) == FAILURE))
        {
        MULToTString(BestRange[rindex].STime - MSched.Time,TString);

        MDB(0,fSCHED) MLog("WARNING:  cannot allocate tasks for job %s at %s\n",
          J->Name,
          TString);

        if (ReqJobTC != J->Request.TC)
          {
          J->Request.TC = ReqJobTC;
          J->Req[0]->TaskCount = ReqJobTC;
          }

        continue; 
        }

      /* node list located */

      break;
      }    /* END for (rindex) */

    MNLMultiFree(AMNL);

    if (ReqJobTC != J->Request.TC)
      {
      J->Request.TC = ReqJobTC;
      J->Req[0]->TaskCount = ReqJobTC;
      }

    if (BestRange[rindex].ETime == 0)
      {
      MDB(0,fSCHED) MLog("ERROR:    cannot allocate tasks for job %s at any time\n",
        J->Name);

      if (EMsg != NULL)
        {
        MUSNPrintF(&BPtr,&BSpace,"resources not available at requested time");
        }

      MNLMultiFree(tmpMNodeList);
      MUFree(&NodeMap);
      MUFree(&tmpNodeMap);

      return(FAILURE);
      }

    BestStartTime = BestRange[rindex].STime;

    tmpTC = 0;

    if (MNodeList != NULL)
      {
      for (nindex = 0;MNLGetNodeAtIndex(MNodeList[0],nindex,NULL) == SUCCESS;nindex++)
        {
        tmpTC += MNLGetTCAtIndex(MNodeList[0],nindex);
        }

      if (NodeCount != NULL)
        *NodeCount = nindex;

      if (TaskCount != NULL)
        *TaskCount = tmpTC;
      }

    *EStartTime = BestStartTime;

    if (PP != NULL) 
      *PP = EP;

    MNLMultiFree(tmpMNodeList);
    MUFree(&NodeMap);
    MUFree(&tmpNodeMap);

    return(SUCCESS);
    }  /* END if if (EP != NULL) */

  MDB(2,fSCHED) MLog("ALERT:    job %s cannot run in any partition\n",
    J->Name); 

  if ((EMsg != NULL) && (EMsg[0] == '\0'))
    {
    if (DownRMID != NULL)
      MUSNPrintF(&BPtr,&BSpace,"resource manager %s is down",
        DownRMID);
    else
      MUSNPrintF(&BPtr,&BSpace,"resources not available in any partition");
    }

  MUFree(&NodeMap);
  MUFree(&tmpNodeMap);
  MNLMultiFree(tmpMNodeList);

  return(FAILURE);
  }  /* END MJobGetEStartTime() */



/**
 * Select feasible nodes for job.
 *
 * @param J (I) job
 * @param P (I) node partition
 * @param SrcNL (I) - NOTE:  hostlist constraints may not be imposed [optional]
 * @param DstNL (O) [minsize=MMAX_NODE]
 * @param NCP (O) [optional]
 * @param TCP (O) [optional]
 * @param StartTime (I) time job must start
 * @param ResMode (I) [not used]
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 */

int MJobGetFNL(

  mjob_t       *J,           /* I  job                        */
  mpar_t       *P,           /* I  node partition             */
  mnl_t        *SrcNL,       /* I  eligible nodes (optional) - NOTE:  hostlist constraints may not be imposed */
  mnl_t        *DstNL,       /* O  feasible nodes (minsize=MMAX_NODE) */
  int          *NCP,         /* O  number of nodes found (optional)  */
  int          *TCP,         /* O  number of tasks found (optional) */
  long          StartTime,   /* I  time job must start        */
  mbitmap_t    *ResMode,     /* I  1 == used by MBFFirstFit to optimized "start now" tests */
  char         *EMsg)        /* O (optional,minsize=MMAX_LINE) */

  {
  int rc;

  int rqindex;

  int RNC;
  int RTC;

  mnl_t      RDstNL = {0};

  mreq_t *RQ;

  const char *FName = "MJobGetFNL";

  MDB(4,fSCHED) MLog("%s(%s,%s,%s,DstNL,NC,TC,%ld,%ld,EMsg)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (P != NULL) ? P->Name : "NULL",
    (SrcNL != NULL) ? "SrcNL" : "NULL",
    StartTime,
    ResMode);

  if (EMsg != NULL)
    {
    EMsg[0] = '\0';
    }

  if (NCP != NULL)
    *NCP = 0;

  if (TCP != NULL)
    *TCP = 0;

  MNLClear(DstNL);

  if ((J == NULL) || (P == NULL))
    {
    return(FAILURE);
    }

  MNLInit(&RDstNL);

  for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
    {
    RQ = J->Req[rqindex];

    if (RQ == NULL)
      break;

    rc = MReqGetFNL(
          J,
          RQ,
          P,
          SrcNL,
          &RDstNL,
          &RNC,
          &RTC,
          StartTime,
          ResMode,
          EMsg);

    if (rc == FAILURE)
      {
      MNLFree(&RDstNL);

      return(FAILURE);
      }

    if (rqindex == 0)
      {
      MNLCopy(DstNL,&RDstNL);
      }
    else
      {
      MNLMerge(DstNL,DstNL,&RDstNL,TCP,NCP);
      }
    }  /* END for (rqindex) */

  MNLFree(&RDstNL);

  return(SUCCESS);
  }  /* END MJobGetFNL() */



/**
 * This function determines nodes that are available for immediate use at the specified starttime
 *
 * @param J         (I)
 * @param SrcMNL    (I) nodes already determined to be feasible
 * @param DstMNL    (O) nodes that are available at specified starttime
 * @param NodeMap   (O) state of nodes
 * @param NC        (O) number of nodes that are available 
 * @param TC        (O) number of procs that are available
 * @param StartTime (I) time at which job must start
 */


int MJobGetAMNodeList(

  mjob_t      *J,          /* I  job                             */
  mnl_t      **SrcMNL,     /* I  feasible nodes (optional)       */
  mnl_t      **DstMNL,     /* O  available nodes                 */
  char        *NodeMap,    /* O state of nodes           */
  int         *NC,         /* O  number of available nodes       */
  int         *TC,         /* O  number of available procs       */
  long         StartTime)  /* I  time at which job must start    */

  {
  /* determine nodes available for immediate use at starttime */

  mbool_t IgnoreAffinity = FALSE;

  int     nindex;
  int     rqindex;

  int     index;

  mnl_t       *MtmpNodeList[MMAX_REQ_PER_JOB];

  mnl_t       *NodeList;
  mnl_t       *Feasible;

  char         Affinity;

  enum MAllocRejEnum RIndex;

  int          AvailTC[MMAX_REQ_PER_JOB] = {0};
  int          AvailNC[MMAX_REQ_PER_JOB] = {0};

  mnode_t     *N;

  mreq_t      *RQ;

  double       MinSpeed;

  mbool_t      BestEffort;

  int          TasksAllowed;
  char         TString[MMAX_LINE];

  const char *FName = "MJobGetAMNodeList";

  MULToTString(StartTime - MSched.Time,TString);

  MDB(5,fSCHED) MLog("%s(%s,SrcMNL,DstMNL,NodeMap,NodeCount,TaskCount,%s)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    TString);

  if (J == NULL)
    {
    return(FAILURE);
    }

  /* check time constraints on all nodes */

  if (NC != NULL)
    *NC = 0;

  BestEffort = FALSE;

  if (TC != NULL)
    {
    if (*TC == -1)
      BestEffort = TRUE;

    *TC = 0;
    }

  MNLMultiInit(MtmpNodeList);

  if (NodeMap != NULL)
    memset(NodeMap,mnmUnavailable,sizeof(char) * MSched.M[mxoNode]);

  if ((bmisset(&J->Flags,mjfRsvMap)) && (MPar[0].RsvNAllocPolicy != mnalNONE))
    {
    /* if a RSVNODEALLOCATIONPOLICY has been specified then ignore affinity */

    IgnoreAffinity = TRUE;
    }

  for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
    {
    RQ = J->Req[rqindex];

    NodeList = MtmpNodeList[rqindex];
    Feasible = SrcMNL[rqindex];

    MUNLGetMinAVal(Feasible,mnaSpeed,NULL,(void **)&MinSpeed);

    nindex = 0;

    AvailTC[rqindex] = 0;
    AvailNC[rqindex] = 0;

    for (index = 0;MNLGetNodeAtIndex(Feasible,index,&N) == SUCCESS;index++)
      {
      MDB(5,fSCHED) MLog("INFO:     checking Feasible[%d][%03d]:  '%s'\n",
        rqindex,
        index,
        N->Name);

      if (MJobCheckNStartTime(
            J,
            RQ,
            N,
            StartTime,
            &TasksAllowed,
            MinSpeed,
            &RIndex,
            &Affinity,
            NULL,
            NULL) == FAILURE)
        {
        /* job cannot start at requested time */

        NodeMap[N->Index] = mnmUnavailable;

        continue;
        }

      if ((!MReqIsGlobalOnly(RQ)) &&
          (bmisset(&J->Flags,mjfAdvRsv)) && 
          (!bmisset(&J->IFlags,mjifNotAdvres)))
        {
        if ((Affinity == mnmPositiveAffinity) ||
            (Affinity == mnmNeutralAffinity) ||
            (Affinity == mnmNegativeAffinity) ||
            (Affinity == mnmRequired))
          {
          MDB(7,fSCHED) MLog("INFO:     node '%s' added (reserved)\n",
            N->Name);
          }
        else
          {
          MDB(7,fSCHED) MLog("INFO:     node '%s' rejected (not reserved)\n",
            N->Name);

          NodeMap[N->Index] = mnmUnavailable;

          continue;
          }
        }    /* END if (bmisset(&J->Flags,mjfAdvRsv)) */

      MNLSetNodeAtIndex(NodeList,nindex,N);
      MNLSetTCAtIndex(NodeList,nindex,TasksAllowed);

      if (IgnoreAffinity == TRUE)
        NodeMap[N->Index] = mnmPositiveAffinity;
      else
        NodeMap[N->Index] = Affinity;

      nindex++;

      AvailTC[rqindex] += TasksAllowed;
      AvailNC[rqindex] ++;

      MDB(5,fSCHED) MLog("INFO:     MNode[%03d] '%s' added (%d of %d)\n",
        index,
        N->Name,
        AvailTC[rqindex],
        RQ->TaskCount);
      }     /* END for (index) */

    /* terminate list */

    MNLTerminateAtIndex(NodeList,nindex);

    if (NC != NULL)
      (*NC) += nindex;

    if (TC != NULL)
      (*TC) += AvailTC[rqindex];

    if (AvailTC[rqindex] < RQ->TaskCount)
      {
      MULToTString(StartTime - MSched.Time,TString);

      MDB(3,fSCHED) MLog("INFO:     inadequate tasks found for job %s:%d (%d < %d) at %s\n",
        J->Name,
        rqindex,
        AvailTC[rqindex],
        RQ->TaskCount,
        TString);

      if ((BestEffort == TRUE) && (AvailTC[rqindex] > 0))
        {
        J->Request.TC -= (RQ->TaskCount - AvailTC[rqindex]);

        RQ->TaskCount = AvailTC[rqindex];
        }
      else
        {
        MNLMultiFree(MtmpNodeList);

        return(FAILURE);
        }
      }

    if ((RQ->NodeCount > 0) &&
        (AvailNC[rqindex] < RQ->NodeCount) &&
        (BestEffort != TRUE))
      {
      MDB(3,fSCHED) MLog("INFO:     inadequate nodes found for job %s:%d (%d < %d)\n",
        J->Name,
        rqindex,
        AvailNC[rqindex],
        RQ->NodeCount);

      MNLMultiFree(MtmpNodeList);

      return(FAILURE);
      }
    }      /* END for (rqindex) */

  MULToTString(StartTime - MSched.Time,TString);

  MDB(3,fSCHED) MLog("INFO:     adequate tasks found for all reqs (time %s)\n",
    TString);

  if (MJobDistributeMNL(J,MtmpNodeList,DstMNL) == FAILURE)
    {
    /* insufficient nodes were found */

    MDB(4,fSCHED)
      {
      for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
        {
        RQ = J->Req[rqindex];

        MDB(4,fSCHED) MLog("INFO:     tasks found for job %s:%d  %d of %d\n",
          J->Name,
          rqindex,
          AvailTC[rqindex],
          RQ->TaskCount);
        }
      }

    MNLMultiFree(MtmpNodeList);

    return(FAILURE);
    }

  MNLMultiFree(MtmpNodeList);

  MDB(4,fSCHED) MLog("INFO:     tasks found for job %s (tasks requested: %d)\n",
    J->Name,
    J->Request.TC);

  return(SUCCESS);
  }  /* END MJobGetAMNodeList() */



/**
 * Get resource availablility ranges for specified node.
 *
 * @see MJobGetRange() - parent
 * @see MJobGetSNRange() - child
 *
 * @param J (I) job
 * @param RQ (I) requirement checked
 * @param N (I) node
 * @param StartTime (I) time resources are requested
 * @param TasksAvailable (O) reservations available
 * @param WindowTime (O) length of time node is available
 * @param Affinity (O) reservation affinity
 * @param Type (O) type of reservation requested
 * @param SeekLong (I)
 * @param BRsvID (O) [optional,minsize=MMAX_NAME]
 */

int MJobGetNRange(

  const mjob_t      *J,
  const mreq_t      *RQ,
  const mnode_t     *N,
  long               StartTime,
  int               *TasksAvailable,
  long              *WindowTime,
  char              *Affinity,
  enum MRsvTypeEnum *Type,
  mbool_t            SeekLong,
  char              *BRsvID)

  {
  char  tmpAffinity;
  long  RangeTime;

  int   MinTasks;
  int   MaxTasks;
  int   TaskCount;

  int   MinTPN;

  mrange_t RRange[MMAX_RANGE];
  mrange_t ARange[MMAX_RANGE];

  int TI;
  int TP;

  int TC;

  int TPNCap;

  const char *FName = "MJobGetNRange";

  MDB(8,fSCHED) MLog("%s(%s,RQ,%s,%ld,TA,WT,Aff,Type,%s,BRsvID)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (N != NULL) ? N->Name : "NULL",
    StartTime,
    (SeekLong != TRUE) ? "TRUE" : "FALSE");

  if ((J == NULL) || (RQ == NULL) || (N == NULL))
    {
    return(FAILURE);
    }

  MinTasks = 0;

  /* find only ranges which start at StartTime */

  RRange[0].STime = StartTime;
  RRange[0].ETime = StartTime;

  RRange[1].ETime = 0;

  if (!bmisset(&J->Flags,mjfRsvMap) &&
      !bmisset(&J->Flags,mjfPreemptor))
    {
    TC = MNodeGetTC(N,&N->ARes,&N->CRes,&N->DRes,&RQ->DRes,StartTime,1,NULL);

    if (MSched.ResOvercommitSpecified[0] == TRUE)
      {
      int tmpTC;

      MNodeGetAvailTC(N,J,RQ,TRUE,&tmpTC);

      TC = MAX(TC,tmpTC);
      }
    }
  else
    {
    TC = MNodeGetTC(N,&N->CRes,&N->CRes,NULL,&RQ->DRes,StartTime,1,NULL);
    }

  MaxTasks = MAX(RQ->TaskCount,RQ->TasksPerNode);
  MaxTasks = MIN(MaxTasks,TC);

  if ((bmisset(&J->IFlags,mjifExactTPN) || bmisset(&J->IFlags,mjifMaxTPN)) && (RQ->TasksPerNode > 0))
    MaxTasks = MIN(MaxTasks,RQ->TasksPerNode);

  MinTPN = 0;

  if (RQ->TasksPerNode > 0)
    {
    MaxTasks -= MaxTasks % RQ->TasksPerNode;
    MinTPN    = RQ->TasksPerNode;
    }
  else
    {
    if ((N->RM != NULL) && (N->RM->Type == mrmtLL))
      {
      if (RQ->NodeCount > 0)
        {
        MinTPN = RQ->TaskCount / RQ->NodeCount;

        /* pessimistic */

/*
        if (RQ->TaskCount % RQ->NodeCount)
          MinTPN++;
*/
        }
      }
    }    /* END if (RQ->TasksPerNode > 0) */

  RangeTime   = 0;
  tmpAffinity = mnmNONE;

  if (Type != NULL)
    *Type = mrtNONE;

  if (RQ->TasksPerNode > 0)
    TPNCap = RQ->TasksPerNode - 1;
  else
    TPNCap = 0;

  MDB(8,fSCHED) MLog("INFO:     TC: %d  Maxtasks: %d  MinTasks: %d (TPNCap: %d)\n",
    TC,
    MaxTasks,
    MinTasks,
    TPNCap);

  while (MaxTasks > MAX(MinTasks,TPNCap))
    {
    if (MaxTasks < MinTPN)
      {
      MaxTasks = 0;

      break;
      }

    TaskCount = (MaxTasks + MinTasks + 1) >> 1;

    if (RQ->TasksPerNode > 0)
      {
      TI = MinTasks / RQ->TasksPerNode;

      TP = RQ->TasksPerNode * (TI + 1);

      if (TP > TaskCount)
        TaskCount = TP;
      else
        TaskCount -= (TaskCount % RQ->TasksPerNode);
      }

    RRange[0].TC = TaskCount;

    if (MJobGetSNRange(
          J,
          RQ,
          N,
          RRange,
          1,
          Affinity,
          Type,      /* O */
          ARange,
          NULL,
          SeekLong,
          BRsvID) == FAILURE)
      {
      MaxTasks = TaskCount - 1;

      if (RQ->TasksPerNode > 0)
        MaxTasks -= (MaxTasks % RQ->TasksPerNode);
      }
    else
      {
      MinTasks  = ARange[0].TC;

      RangeTime = MIN(MMAX_TIME,ARange[0].ETime - ARange[0].STime);

      if (Affinity != NULL)
        tmpAffinity = *Affinity;
      }
    }    /* END while(MaxTasks > MinTasks) */

  if (WindowTime != NULL)
    *WindowTime = RangeTime;

  if (Affinity != NULL)
    *Affinity = tmpAffinity;

  if (TasksAvailable != NULL)
    {
    if (RQ->TasksPerNode > 0)
      MinTasks -= (MinTasks % RQ->TasksPerNode);

    *TasksAvailable = MinTasks;
    }

  if ((MaxTasks <= 0) || (RangeTime <= 0))
    {
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MJobGetNRange() */





/**
 * Compute feasible resources for J on N.
 *
 * NOTE: This function is NOT MEANT to take into account existing workload, only
 *  existing permanent VM resources.
 *
 * @see MJobGetSNRange() - parent
 * @see VMSchedulingDoc
 *
 * @param J   (I) [optional]
 * @param SpecVMUsage (I) [optional]
 * @param N   (I)
 * @param Out (O)
 */

int MJobGetFeasibleNodeResources(

  const mjob_t            *J,            /* I (optional) */
  enum MVMUsagePolicyEnum  SpecVMUsage,  /* I (optional) */
  const mnode_t           *N,            /* I */
  mcres_t                 *Out)          /* O */

  {
  mcres_t tmpRes;
  mcres_t LowerBound;
  mln_t *Link;

  MCResInit(&tmpRes);
  MCResInit(&LowerBound);

  if (((J != NULL) && MJOBREQUIRESVMS(J) && FALSE) ||
       (SpecVMUsage == mvmupVMOnly)) 
    {
    /* static VM jobs only look at the CRes of permanent VMs on N
     * (no deleted or one-time use VMs) */

    for (Link = N->VMList;Link != NULL;Link = Link->Next)
      {
      mvm_t *VM = (mvm_t *)Link->Ptr;

      if (bmisset(&VM->Flags,mvmfOneTimeUse) || 
          bmisset(&VM->Flags,mvmfInitializing) ||
         (MVMIsDestroyed(VM) == TRUE))
        {
        continue;
        }

      MCResPlus(&tmpRes,&VM->CRes);
      }  /* END for (Link) */

    /* TODO: constrain resources to node CRes */
    }
  else if ((N->VMList != NULL) &&
           (MSched.VMTracking == FALSE) &&
           (!bmisset(&MSched.Flags,mschedfOutOfBandVMRsv)))
    {
    /* All other jobs must subtract out static VM resources,
        including those being destroyed */

    MCResCopy(&tmpRes,&N->CRes);

    for (Link = N->VMList;Link != NULL;Link = Link->Next)
      {
      mvm_t *VM = (mvm_t *)Link->Ptr;
      
      /* do not count VM resources that are always temporary or not
       * yet created */

      if (!bmisset(&VM->Flags,mvmfOneTimeUse) &&
          !bmisset(&VM->Flags,mvmfInitializing))
        {
        MCResMinus(&tmpRes,&VM->CRes);
        }
      }  /* END for (Link) */
    }    /* END else if (N->VMList != NULL) */
  else
    {
    /* if the node has no VMs, we can safely copy it's CRes 
       because we don't have to differentiate between 
       static and non-static VM's */

    MCResCopy(&tmpRes,&N->CRes);
    }

  MCResLowerBound(&tmpRes,&LowerBound);

#if 0
  /* doug says that this section of code is redundant as the handling of GRES happens
     in the lines above. */

  /* Most VM's will now require some sort of generic resource consumption due to storage, network, etc.
   * For this reason we are going to always allow the VM create job to see the GRES of the physical server. */
  /* MSched.VMGResMap == TRUE) [JMB]*/
  if (TRUE)
    {
    /* if GRes based vm-mapping is occuring, then requirevm jobs must see gres's of
       physical server in order to know that the needed vm is available (no longer limited
       to GRes based vm-mapping) */

    MGResPlus(&tmpRes.GRes,&N->CRes.GRes);
    }
  else if ((N->RM != NULL) && (bmisset(&N->RM->RTypes,mrmrtStorage)))
    {
    /* copy shared storage resources */

    MGResPlus(&tmpRes.GRes,&N->CRes.GRes);
    }
#endif

  /* We do not yet accurately track class/gres resources for VMs */

  MCResCopy(Out,&tmpRes);

  MCResFree(&tmpRes);
  MCResFree(&LowerBound);

  return(SUCCESS);
  }  /* END MJobGetFeasibleNodeResources() */



/* NOTE:  extract MRRSRange for compiler stack limits and for optimization */
/*        should change to dynamic allocation as many environments will not use */
/*        MJobGetMRRange() at all */

/* NOTE:  MJobGetMRRange() is not threadsafe due to global MRRSRange[][]! */

mrange_t MRRSRange[MMAX_REQ_PER_JOB][MMAX_RANGE];

/**
 * Determine multi-req resource availability ranges.
 *
 * @param J            (I) job description
 * @param OTime        (I) job req time offset list
 * @param P            (I) partition in which resources must reside
 * @param MinStartTime (I) earliest possible starttime
 * @param GRL          (O) [optional] array of time ranges located
 * @param MAvlNodeList (O) multi-req list of nodes found
 * @param NodeCount    (O) number of available nodes located
 * @param NodeMap
 * @param RTBM         (I) [bitmap]
 * @param SeekLong     (I)
 * @param TRange       (O)
 * @param EMsg         (O) [optional,minsize=MMAX_LINE]
 */

int MJobGetMRRange(

  mjob_t      *J,
  mulong      *OTime,
  mpar_t      *P,
  long         MinStartTime,
  mrange_t    *GRL,
  mnl_t      **MAvlNodeList,
  int         *NodeCount,
  char        *NodeMap,
  mbitmap_t   *RTBM,
  mbool_t      SeekLong,
  mrange_t    *TRange,
  char        *EMsg)

  {
  int      FTC[MMAX_REQ_PER_JOB];
  int      FNC[MMAX_REQ_PER_JOB];

  int     *ResIndex = NULL;
  mrange_t ARange[MMAX_RANGE];
  mrange_t NGRL[MMAX_RANGE];

  mnl_t     *RQNodeList[MMAX_REQ_PER_JOB];
  mnl_t      NodeList = {0};

  mrange_t RRange[MMAX_REQ_PER_JOB];           /* required range */

  mulong   ReqDuration;

  long     FoundStartTime;
  int      RCount;

  int      ATC;
  int      ANC;

  /* total per req TC (across all nodes) for first globally available range */

  int      ReqTC[MMAX_REQ_PER_JOB];

  /* NOTE:  require per req ATC tracking (NYI) */

  int      nindex;
  int      nindex2;
  int      nlindex;
  int      rqindex;
  int      NCount;

  mreq_t  *RQ;

  char     Affinity;

  mnode_t *N;
  mnode_t *N1;
  mnode_t *N2;

  char     TString[MMAX_LINE];
  const char *FName = "MJobGetMRRange";

  MULToTString(MinStartTime - MSched.Time,TString);

  MDB(5,fSCHED) MLog("%s(%s,OTime,%s,%s,GRL,%s,NodeMap,%s,%s,EMsg)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (P != NULL) ? P->Name : "NULL",
    TString,
    (MAvlNodeList == NULL) ? "NULL" : "MAvlNodeList",
    MBool[SeekLong],
    (TRange != NULL) ? "TRange" : "NULL");

  if (EMsg != NULL)
    EMsg[0] = '\0';

  /* NOTE:  GRL or MAvlNodeList must be specified */

  if ((J == NULL) || (OTime == NULL))
    {
    return(FAILURE);
    }

  /* initialize */

  memset(ResIndex,0,sizeof(ResIndex));

  memset(ARange,0,sizeof(ARange));

  if (NodeMap != NULL)
    memset(NodeMap,mnmUnavailable,sizeof(char) * MSched.M[mxoNode]);

  if (GRL != NULL)
    GRL[0].ETime = 0;

  ResIndex = (int *)MUCalloc(1,sizeof(int) * MSched.M[mxoNode]);

  MNLMultiInit(RQNodeList);

  /* load feasible resources for all reqs */

  for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
    {
    if (J->Req[rqindex] == NULL)
      break;
 
    RQ = J->Req[rqindex];

    if (MReqGetFNL(
          J,
          RQ,
          P,
          NULL,
          RQNodeList[rqindex], /* O */
          &FNC[rqindex],       /* O */
          &FTC[rqindex],       /* O */
          MMAX_TIME,
          0,
          EMsg) == FAILURE)
      {
      MDB(6,fSCHED) MLog("INFO:     cannot locate configured resources in %s()\n",
        FName);

      if ((EMsg != NULL) && (EMsg[0] == '\0'))
        snprintf(EMsg,MMAX_LINE,"cannot locate feasible resources");   

      MUFree((char **)&ResIndex);

      MNLMultiFree(RQNodeList);

      return(FAILURE);
      }

    /* NOTE:  MReqGetFNL() populates EMsg on SUCCESS and FAILURE */

    if (EMsg != NULL)
      EMsg[0] = '\0';

    if (rqindex > 0)
      {
      FNC[0] = MIN(FNC[0],FNC[rqindex]);
      FTC[0] = MIN(FTC[0],FTC[rqindex]);
      }
    }    /* END for (rqindex) */

  /* NOTE:  RRange is execution time range */

  RQ = J->Req[0];

  RRange[0].STime = MinStartTime;

  if ((J->CMaxDate > 0) && (J->CMaxDateIsActive != FALSE))
    {
    RRange[0].ETime = J->CMaxDate;
    }
  else
    {
    RRange[0].ETime = MMAX_TIME;
    }

  RRange[0].TC    = MAX(1,RQ->TasksPerNode);

  RRange[1].ETime = 0;

  nlindex = 0;

  ATC = 0;
  ANC = 0;

  memset(ReqTC,0,sizeof(ReqTC));

  N = NULL;

  /* locate nodes which satisfy all reqs */

  NCount = 0;

  MNLInit(&NodeList);

  for (nindex = 0;MNLGetNodeAtIndex(RQNodeList[0],nindex,&N1) == SUCCESS;nindex++)
    {
    for (rqindex = 1;rqindex < MMAX_REQ_PER_JOB;rqindex++)
      {
      if (J->Req[rqindex] == NULL)
        break;

      for (nindex2 = 0;MNLGetNodeAtIndex(RQNodeList[rqindex],nindex2,&N2) == SUCCESS;nindex2++)
        {
        if (N1 == N2)
          break;
        }

      if (N1 != N2)
        break;
      }  /* END for (rqindex = 1;rqindex < MMAX_REQ_PER_JOB;rqindex++) */

    if (J->Req[rqindex] == NULL)
      MNLSetNodeAtIndex(&NodeList,NCount++,N1);
    }  /* for (nindex = 0;RQNodeList[0][nindex].N != NULL;nindex++) */

  MNLTerminateAtIndex(&NodeList,NCount);

  rqindex = 0;

  for (nindex = 0;MNLGetNodeAtIndex(&NodeList,nindex,&N) == SUCCESS;nindex++)
    {
    /* initialize node structures */

    NGRL[0].ETime = 0;

    /* loop through all reqs */

    for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
      {
      if (J->Req[rqindex] == NULL)
        break;

      RQ = J->Req[rqindex];
 
      if (MJobGetSNRange(
            J,
            RQ,
            N,
            RRange,
            MMAX_RANGE,
            &Affinity,
            NULL,
            ARange,   /* O */
            NULL,
            SeekLong,
            NULL) == FAILURE)
        {
        MULToTString(MinStartTime - MSched.Time,TString);

        MDB(7,fSCHED) MLog("INFO:     no reservation time found for job %s:%d on node %s at %s\n",
          J->Name,
          RQ->Index,
          N->Name,
          TString);

        if (NodeMap != NULL)
          NodeMap[N->Index] = mnmUnavailable;

        /* break out of rqindex loop */

        break;
        }

      /* adjust reported range by negative offset */

      if (OTime[rqindex] != 0)
        MRLOffset(ARange,- OTime[rqindex]);

      if (MRangeApplyLocalDistributionConstraints(ARange,J,N) == FAILURE)
        {
        /* no valid ranges available */

        continue;
        }

      /* NOTE:  req duration is delta between offset OTime[rqindex] and OTime[rqindex + 1] (variable) */

      if (rqindex == 0)
        ReqDuration = J->SpecWCLimit[0];
      else if (rqindex == 1)
        ReqDuration = -OTime[1];
      else
        ReqDuration = OTime[rqindex - 1] - OTime[rqindex];
 
      if (MRLSFromA(ReqDuration,ARange,MRRSRange[rqindex]) == FAILURE)
        {
        /* no valid ranges available */

        continue;
        }

      /* AND current range w/NGRL */

      if (rqindex == 0)
        memcpy(NGRL,MRRSRange[rqindex],sizeof(MRRSRange[0]));
      else
        MRLAND(NGRL,NGRL,MRRSRange[rqindex],FALSE);

      /* if result is empty, break */

      if (NGRL[0].ETime == 0)
        break;
      }    /* END for (rqindex) */

    if (NGRL[0].ETime == 0)
      {
      /* node does not satisfy requirements, try next node */

      continue;
      }

    ANC++;

    ATC += NGRL[0].TC;

    /* NOTE:  current implementation is optimistic as final GRL may not have 
              sufficient tasks in corresponding timeframe to satisfy all job 
              tasks.  Correct solution may need to recalculate all MRRSRanges 
              and corresponding per req taskcounts once potential solution 
              timeframe is located.  This process may be iterative until
              per node and per req conditions are satisfied (NYI) */

    for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
      {
      if (J->Req[rqindex] == NULL)
        break;

      ReqTC[rqindex] += MRangeGetTC(MRRSRange[rqindex],NGRL[0].STime);
      }  /* END for (rqindex) */

    /* if exact match on starttime */

    /* behavior change 11/12/09 by DRW */
    /* if MJobGetSNRange() does not return failure then this node can run both jobs at least at 
       some time.  It's up to the calling routines to check whether that "sometime" is valid
       or not (VLP had some bugs in this area, they were fixed by this change) */

    if (MAvlNodeList != NULL)
      {
      /* NOTE:  resources are co-allocated, only one node range returned */
 
      MNLSetNodeAtIndex(MAvlNodeList[0],nlindex,N);
      MNLSetTCAtIndex(MAvlNodeList[0],nlindex,ARange[0].TC);

      nlindex++;

      MDB(7,fSCHED) MLog("INFO:     node %d '%sx%d' added to nodelist\n",
        ANC,
        N->Name,
        ARange[0].TC);
      }

    /* result is not empty, add node to MNodeList */

    if (NodeMap != NULL)
      NodeMap[N->Index] = Affinity;

    MRLMerge(GRL,NGRL,J->Request.TC,SeekLong,&FoundStartTime);

    if (Affinity == mnmRequired)
      {
      /* constrain feasible time frames */

      MRLAND(GRL,GRL,NGRL,FALSE);
      }

    if (bmisset(RTBM,mrtcStartEarliest))
      RRange[0].ETime = FoundStartTime;
    }    /* END for (nindex) */

  MUFree((char **)&ResIndex);
  MNLFree(&NodeList);
  MNLMultiFree(RQNodeList);

  MRangeApplyGlobalDistributionConstraints(GRL,J,&ATC);

  if (NodeCount != NULL)
    *NodeCount = ANC;

  if (MAvlNodeList != NULL)
    {
    MNLTerminateAtIndex(MAvlNodeList[0],nlindex);
    MNLClear(MAvlNodeList[1]);

    for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
      {
      RQ = J->Req[rqindex];

      if (RQ == NULL)
        break;

      if (RQ->TaskCount > ReqTC[rqindex])
        {
        if (!bmisset(&J->Flags,mjfBestEffort) || (ATC == 0))
          {
          MULToTString(MinStartTime - MSched.Time,TString);

          MDB(6,fSCHED) MLog("ALERT:    inadequate tasks located for req %s:%d at %s (%d < %d)\n",
            J->Name,
            RQ->Index,
            TString,
            ReqTC[rqindex],
            RQ->TaskCount);

          /* NOTE:  warn only, cannot determine failure until MRLAND is optimized */

          /* return(FAILURE); */
          }
        }

      if ((J->Request.NC > ANC) || (ANC == 0))
        {
        if (!bmisset(&J->Flags,mjfBestEffort) || (ANC == 0))
          {
          MULToTString(MinStartTime - MSched.Time,TString);

          MDB(6,fSCHED) MLog("ALERT:    inadequate nodes located for job %s at %s (%d < %d)\n",
            J->Name,
            TString,
            ANC,
            RQ->NodeCount);

          return(FAILURE);
          }
        }
      }    /* END for (rqindex) */

    /* NOTE:  GRL not populated if MNodeList specified */

    /* calendar constraints must be enforced w/GRL specification */

    return(SUCCESS);
    }  /* END if (MAvlNodeList != NULL) */

  /* get only feasible ranges */

  if (MPar[0].MaxMetaTasks > 0)
    {
    /* impose meta task limit */

    MRLLimitTC(
      GRL,
      MRange,
      NULL,
      MPar[0].MaxMetaTasks);
    }

  if ((TRange != NULL) && (GRL != NULL))
    memcpy(TRange,GRL,sizeof(mrange_t) * MMAX_RANGE);

  if (!bmisset(&J->Flags,mjfBestEffort))
    {
    /* NOTE:  should this apply on a per-req basis, ie, set arg 2 to RQ? (NYI) */

    MJobSelectFRL(J,NULL,GRL,RTBM,&RCount);
    }

  if ((GRL == NULL) || (GRL[0].ETime == 0))
    {
    MDB(6,fSCHED) MLog("INFO:     no ranges found\n");

    if (EMsg != NULL)
      snprintf(EMsg,MMAX_LINE,"no ranges found");

    return(FAILURE);
    }

  /* return feasible resource info with termination range */

  for (nindex = 0;nindex < MMAX_RANGE;nindex++)
    {
    if (GRL[nindex].ETime != 0)
      continue;

    GRL[nindex].NC = FNC[0];
    GRL[nindex].TC = FTC[0];

    break;
    }

  return(SUCCESS);
  }  /* END MJobGetMRRange() */


/**
 * check class and user defaults to set a default wall time for the job
 *
 * @param J           (I) [modified]
 * @param JobWalltime (O) [modified]
 */

int MJobGetDefaultWalltime(
    
  mjob_t *J,
  long *JobWalltime)

  {
  mjob_t *QJDef = NULL;
  mjob_t *CJDef = NULL;
  mjob_t *UJDef = NULL;
  mjob_t *CDJDef = NULL;

  mrsv_t *JRes;   /* pointer to job's reservation to check for end time */

  mulong JRsvEndtime;     /* end time of the job's reservation */

  MDB(4,fRM) MLog("INFO:     wallclock limit not specified for job %s\n",
    J->Name);

  if ((J == NULL) || (JobWalltime == NULL))
    {
    return(FAILURE);
    }

  bmset(&J->IFlags,mjifWCNotSpecified);

  *JobWalltime = 0;

  JRes = NULL;

  /* get credential defaults */

  if (J->Credential.Q != NULL)
    {
    QJDef = J->Credential.Q->L.JobDefaults;
    }

  if (J->Credential.C != NULL)
    {
    CJDef = J->Credential.C->L.JobDefaults;
    }

  if (J->Credential.U != NULL)
    {
    UJDef = J->Credential.U->L.JobDefaults;
    }

  if (MSched.DefaultC != NULL)
    {
    CDJDef = MSched.DefaultC->L.JobDefaults;
    }

  /* Check the job's reservation if the useRsvWalltime flag is set. If this
   * flag is set, this should take precendence over other defaults */


  if ((J->ReqRID != NULL) &&
      (bmisset(&MSched.Flags,mschedfJobsUseRsvWalltime)) &&
      ((MRsvFind(J->ReqRID,&JRes,mraNONE) != FAILURE) ||
       (MRsvFind(J->ReqRID,&JRes,mraRsvGroup) != FAILURE)))
    {
    /* get the job's reservation end time and the current time */

    JRsvEndtime = JRes->EndTime;

    /* get the current time in seconds and set JobWalltime to the
      * duration between now and the job's reservation end time */

    /* NOTE:  if we use the exact remaining time in the reservation,
      *        sometimes the job will not run; using a 5-second buffer
      *        prevents this. Bigger now time will give smaller walltime
      *        to give the buffer. */

    *JobWalltime = JRsvEndtime - (MSched.Time + 5);

    bmunset(&J->IFlags,mjifWCNotSpecified);
    } /* END if (MRsvFind... */

  else if ((UJDef != NULL) && (UJDef->SpecWCLimit[0] > 0))
    {
    /* USERCFG[XXX] DEFAULT.WCLIMIT has greater precedence than class, queue,
     * or partition */

    *JobWalltime = UJDef->SpecWCLimit[0];

    bmunset(&J->IFlags,mjifWCNotSpecified);
    }

  else if ((CJDef != NULL) && (CJDef->SpecWCLimit[0] > 0))
    {
    *JobWalltime = CJDef->SpecWCLimit[0];
    
    bmunset(&J->IFlags,mjifWCNotSpecified);
    }

  else if ((QJDef != NULL) && (QJDef->SpecWCLimit[0] > 0))
    {
    *JobWalltime = QJDef->SpecWCLimit[0];

    bmunset(&J->IFlags,mjifWCNotSpecified);
    }

  else if ((CDJDef != NULL) && (CDJDef->SpecWCLimit[0] > 0))
    {
    *JobWalltime = CDJDef->SpecWCLimit[0];

    bmunset(&J->IFlags,mjifWCNotSpecified);
    }

  else if (MPar[0].L.JobPolicy->HLimit[mptMaxWC][0] > 0)
    {
    /* apply partition defaults if any exist and we haven't gotten a
     * walltime yet */

    *JobWalltime = MPar[0].L.JobPolicy->HLimit[mptMaxWC][0];
    
    bmunset(&J->IFlags,mjifWCNotSpecified); 
    }
  
  /* use default job's walltime if exists and is specified */

  else if ((J->SpecWCLimit[0] == 0) &&
      (MSched.DefaultJ != NULL) &&
      (MSched.DefaultJ->SpecWCLimit[0]))
    {
    *JobWalltime = MSched.DefaultJ->SpecWCLimit[0];

    bmunset(&J->IFlags,mjifWCNotSpecified);
    }

    /* Check default user for default walltime.
          Example: 'USERCFG[DEFAULT] DEFAULT.WCLIMIT = 1:00 '*/

  else if ((J->SpecWCLimit[0] == 0) &&
           (MSched.DefaultU != NULL) && 
           (MSched.DefaultU->L.JobDefaults != NULL) && 
           (MSched.DefaultU->L.JobDefaults->SpecWCLimit[0] > 0))
    {
    *JobWalltime = MSched.DefaultU->L.JobDefaults->SpecWCLimit[0];

    bmunset(&J->IFlags,mjifWCNotSpecified);
    }
  else
    {
    /* unlimited or no walltime requested */

    MDB(7,fSCHED) MLog("WARNING:  Job %s will be assigned the default walltime limit (%d seconds).\n",
      J->Name,
      MDEF_SYSDEFJOBWCLIMIT);

    if ((MSched.DefaultJ != NULL) &&
        (MSched.DefaultJ->SpecWCLimit[0]))
      *JobWalltime = MSched.DefaultJ->SpecWCLimit[0];
    else
      *JobWalltime = MDEF_SYSDEFJOBWCLIMIT;
    }

  return SUCCESS;
  } /* END MJobGetDefaultWalltime */



/**
 *
 * NOTE: if PreserveOrder is not set, call with TL pointing to correct
 *       insertion point in tasklist array 
 *
 * @param J (I)
 * @param PreserveOrder (I)
 * @param TL (O) [minsize=TLSize]
 * @param TLSize (I)
 */

int MJobGetLocalTL(

  mjob_t  *J,              /* I */
  mbool_t  PreserveOrder,  /* I */
  int     *TL,             /* O (minsize=TLSize) */
  int      TLSize)         /* I */

  {
  int TIndex;

  int tindex;
  int rqindex;
  int nindex;

  mreq_t *RQ;

  int *tmpTL = NULL;

  mnode_t *N;

  const char *FName = "MJobGetLocalTL";

  MDB(7,fSTRUCT) MLog("%s(%s,%s,TL,%d)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    MBool[PreserveOrder],
    TLSize);

  /* NOTE:  if PreserveOrder is not set, append non-compute tasks to end 
            of tasklist which may cause nodelist to be out of req order! */

  /* NOTE:  does not handle non-compute reqs interspersed with compute reqs */
  /*        only handles leading and trailing non-compute reqs */

  if ((J == NULL) || (TL == NULL))
    {
    return(FAILURE);
    }

  TIndex = 0;
  rqindex = 0;

  tmpTL = (int *)MUMalloc(sizeof(int) * MSched.JobMaxTaskCount);

  if (PreserveOrder == TRUE)
    {
    if (TLSize > MSched.JobMaxTaskCount)
      {
      MUFree((char **)&tmpTL);

      return(FAILURE);
      }

    memcpy(tmpTL,TL,sizeof(int) * TLSize);

    for (;J->Req[rqindex] != NULL;rqindex++)
      {
      RQ = J->Req[rqindex];

      if (RQ->DRes.Procs != 0)
        break;

      for (nindex = 0;MNLGetNodeAtIndex(&RQ->NodeList,nindex,&N) == SUCCESS;nindex++)
        {
        for (tindex = 0;tindex < MNLGetTCAtIndex(&RQ->NodeList,nindex);tindex++)
          {
          TL[TIndex++] = N->Index;

          if (TIndex >= TLSize - 1)
            break;
          }  /* END for (tindex) */

        if (TIndex >= TLSize - 1)
          break;
        }    /* END for (nindex) */

      if (TIndex >= TLSize - 1)
        break;
      }      /* END for (rqindex) */

    /* copy over all compute tasks */

    for (tindex = 0;tmpTL[tindex] != -1;tindex++)
      {
      /* FIXME: this doesn't work for jobs that use the GLOBAL node */
      /*         Why?????                                           */

      if ((MNode[tmpTL[tindex]] != NULL) && 
          (MNode[tmpTL[tindex]]->CRes.Procs == 0))
        continue;

      TL[TIndex++] = tmpTL[tindex];

      if (TIndex >= TLSize - 1)
        break;
      }  /* END for (tindex) */
    }    /* END if (PreserveOrder == TRUE) */

  /* append remaining non-compute req nodes */

  for (;J->Req[rqindex] != NULL;rqindex++)
    {
    RQ = J->Req[rqindex];

    if (RQ->DRes.Procs != 0)
      continue;

    for (nindex = 0;MNLGetNodeAtIndex(&RQ->NodeList,nindex,&N) == SUCCESS;nindex++)
      {
      for (tindex = 0;tindex < MNLGetTCAtIndex(&RQ->NodeList,nindex);tindex++)
        { 
        if (TIndex >= TLSize - 1)
          break;

        TL[TIndex++] = N->Index;
        }  /* END for (tindex) */

      if (TIndex >= TLSize - 1)
        break;
      }    /* END for (nindex) */

    if (TIndex >= TLSize - 1)
      break;
    }      /* END for (rqindex) */

  TL[TIndex] = -1;

  MUFree((char **)&tmpTL);

  return(SUCCESS);
  }  /* END MJobGetLocalTL() */




/**
 *
 *
 * @param J (I) [modified if 'Cost' not passed]
 * @param CalculateMax (I) [display maximum possible cost]
 * @param Cost (O) [optional]
 */

int MJobGetCost(

  mjob_t  *J,             /* I (modified if 'Cost' not passed) */
  mbool_t  CalculateMax,  /* I (display maximum possible cost) */
  double  *Cost)          /* O (optional) */

  {
  double NodeChargeRate;  /* based on nodes allocated to req */
  double JobChargeRate;   /* based on time of day, credential discounts, and qos requested */

  double BaseReqUsage;
  double BaseJobUsage;

  double tmpD;

  long   Duration;

  int    rqindex;

  mreq_t *RQ;

  enum MAMConsumptionPolicyEnum ChargeMetric;

  if (J == NULL)
    {
    return(FAILURE);
    }

  /* NOTE:  only supports charging by dedicated time, should also support requested time */

  /* NOTE: must incorporate generic resource usage, network activity, generic events, etc (NYI) */

  if (CalculateMax == TRUE)
    {
    Duration = J->WCLimit;
    }
  else
    {
    Duration = (J->CompletionTime > J->StartTime) ?
      J->CompletionTime - J->StartTime : 0;
    }

  BaseJobUsage = 0.0;

  if (MSched.ChargeMetric != mamcpNONE)
    ChargeMetric = MSched.ChargeMetric;
  else if (MAM[0].Name[0] != '\0')
    ChargeMetric = MAM[0].ChargePolicy;
  else
    ChargeMetric = mamcpNONE;

  /* DebitSuccessfulWC is the documented default */
	
	if (ChargeMetric == mamcpNONE)
    ChargeMetric = mamcpDebitSuccessfulWC;

  /* cost = <CHARGEMETRIC> * <QOSRATE> */

  for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
    {
    RQ = J->Req[rqindex];

    if (RQ == NULL) 
      break;

    BaseReqUsage = 0.0;

    NodeChargeRate = 1.0;

    switch (ChargeMetric)
      {
      case mamcpDebitSuccessfulCPU:

        /* On a successful job, let it drop through to be charged */

        if ((J->State != mjsCompleted) && (CalculateMax == FALSE))
          break;

      case mamcpDebitAllCPU:

        if (CalculateMax == TRUE)
          {
          BaseReqUsage = J->TotalProcCount * Duration;
          }
        else if (RQ->URes != NULL)
          {
          BaseReqUsage = J->TotalProcCount * RQ->URes->Procs * RQ->TaskCount / 100.0;
          }
 
        break;

      case mamcpDebitSuccessfulPE:

        /* On a successful job, let it drop through to be charged */

        if ((J->State != mjsCompleted) && (CalculateMax == FALSE))
          break;

      case mamcpDebitAllPE:
        {
        double PE;

        MJobGetPE(J,&MPar[0],&PE);

        BaseReqUsage = PE * Duration;
        } /* END BLOCK */

        break;

      case mamcpDebitSuccessfulWC:

        /* On a successful job, let it drop through to be charged or CalculateMax */

        if ((J->State != mjsCompleted) && (CalculateMax == FALSE))
          break;

      case mamcpDebitAllWC:

        BaseReqUsage = J->TotalProcCount * Duration;

        break;

      case mamcpDebitSuccessfulBlocked:  

        if ((J->State != mjsCompleted) && (CalculateMax == FALSE))
          break;

      case mamcpDebitAllBlocked:
        {
        int     ProcCount;

        mbool_t ChargeByDedicatedNode = FALSE;

        if ((J->Credential.Q != NULL) &&
            (bmisset(&J->Credential.Q->Flags,mqfDedicated)) &&
            (!MNLIsEmpty(&J->NodeList)))
          {
          /* QoS 'dedicated' flag set */

          ChargeByDedicatedNode = TRUE;
          }
        else
          {
          enum MNodeAccessPolicyEnum NAPolicy;

          MJobGetNAPolicy(J,NULL,NULL,NULL,&NAPolicy);

          if ((NAPolicy == mnacSingleTask) || (NAPolicy == mnacSingleJob))
            {
            /* node dedicated by job or global node access policy */

            ChargeByDedicatedNode = TRUE;
            }
          }

        if (ChargeByDedicatedNode == TRUE)
          {
          mnode_t *N;

          int nindex;

          ProcCount = 0;

          for (nindex = 0;MNLGetNodeAtIndex(&J->NodeList,nindex,&N) == SUCCESS;nindex++)
            {
            ProcCount += N->CRes.Procs;
            }  /* END for (nindex) */
          }
        else
          {
          ProcCount = MAX(1,J->TotalProcCount);
          }

        BaseReqUsage = ProcCount * Duration;
        }  /* END BLOCK */

        break;

      default:

        BaseReqUsage = 0;

        break;
      }  /* END switch (ChargeMetric) */

    if (MUNLGetMinAVal(&RQ->NodeList,mnaChargeRate,NULL,(void **)&tmpD) == SUCCESS)
      NodeChargeRate = tmpD;

    BaseJobUsage += BaseReqUsage * NodeChargeRate;

    if (!MSNLIsEmpty(&RQ->DRes.GenericRes))
      {
      int gindex;

      for (gindex = 1;gindex < MSched.M[mxoxGRes];gindex++)
        {
        if (MGRes.Name[gindex][0] == '\0')
          break;

        if ((MSNLGetIndexCount(&RQ->DRes.GenericRes,gindex) == 0) ||
            (MGRes.GRes[gindex]->ChargeRate == 0.0))
          continue;

        BaseJobUsage += MSNLGetIndexCount(&RQ->DRes.GenericRes,gindex)* 
                        MGRes.GRes[gindex]->ChargeRate * 
                        RQ->TaskCount * 
                        Duration;
        }  /* END for (gindex) */
      }    /* END if (RQ->DRes.GRes[0].Count > 0) */
    }      /* END for (rqindex) */

  /* determine effective charge rate */

  JobChargeRate = 1.0;

  switch (MSched.ChargeRatePolicy)
    {
    case mamcrpQOSReq:

      if (J->Credential.Q == NULL)
        break;

      if (J->Credential.Q->DedResCost >= 0.0)
        JobChargeRate = J->Credential.Q->DedResCost;

      break;

    case mamcrpQOSDel:

      /* determine delivered QOS */

      if ((J->QOSDelivered == NULL) && (MJobGetQOSDel(J,&J->QOSDelivered) == FAILURE))
        break;

      if (J->QOSDelivered->DedResCost >= 0.0)
        JobChargeRate = J->QOSDelivered->DedResCost;

      break;

    default:

      /* NO-OP */

      break;
    }  /* END switch (MSched.ChargePolicy) */

  if (Cost != NULL)
    *Cost = BaseJobUsage * JobChargeRate;
  else
    J->Cost = BaseJobUsage * JobChargeRate;

  return(SUCCESS); 
  }  /* END MJobGetCost() */





/**
 *
 *
 * @see MQOSGetAccess() - child
 *
 * @param J (I) [modified if 'QDel' not passed]
 * @param QDel (O) [optional]
 */

int MJobGetQOSDel(

  mjob_t  *J,     /* I (modified if 'QDel' not passed) */
  mqos_t **QDel)  /* O (optional) */

  {
  mqos_t *BestXFQ;
  mqos_t *BestQTQ;

  double  XFDel;
  long    QTDel;

  double  XFSpan;
  long    QTSpan;

  double  BestXFSpan;
  long    BestQTSpan;

  mqos_t *Q;
  int     qindex;

  double  MaxXFDel = 999999.0;
  long    MaxQTDel = MMAX_TIME;

  double  Distance;
  double  BestDistance;

  mbitmap_t QAL;

  /* determine QOS's which allow job access */

  /* return QOS which possess best fit */

  if (QDel != NULL)
    *QDel = NULL;
  else
    J->QOSDelivered = NULL;

  if (J == NULL)
    {
    return(FAILURE);
    }

  /* determine QOS's which allow job access */

  if (MQOSGetAccess(J,NULL,NULL,&QAL,NULL) == FAILURE)
    {
    bmclear(&QAL);

    return(FAILURE);
    }

  QTDel = J->EffQueueDuration;

  XFDel = (double)(QTDel + J->AWallTime) / MAX(1,J->WCLimit);

  BestXFSpan = MaxXFDel;
  BestQTSpan = MaxQTDel;

  BestXFQ = J->Credential.Q;
  BestQTQ = J->Credential.Q;

  /* check active QOS targets */

  if ((BestXFQ != NULL) && (BestXFQ->XFTarget > 0.0))
    {
    if (XFDel < BestXFQ->XFTarget)
      {
      BestXFSpan = BestXFQ->XFTarget - XFDel;
      }
    else
      {
      /* QOS target not met */

      BestXFQ = NULL;
      }
    }

  if ((BestQTQ != NULL) && (BestQTQ->QTTarget > 0))
    {
    if (QTDel < BestQTQ->QTTarget)
      {
      BestQTSpan = BestQTQ->QTTarget - QTDel;
      }
    else
      {
      /* QOS target not met */

      BestQTQ = NULL;
      }
    }

  for (qindex = 1;qindex < MMAX_QOS;qindex++)
    {
    Q = &MQOS[qindex];

    if (Q->Name[0] == '\0')
      break;

    if (Q->Name[0] == '\1')
      continue;

    if (!bmisset(&QAL,Q->Index))
      continue;

    /* determine if targets are satisfied */

    XFSpan = 999999.0;
    QTSpan = MMAX_TIME;

    if (Q->XFTarget > 0.0)
      {
      if (XFDel > Q->XFTarget)
        continue;

      XFSpan = Q->XFTarget - XFDel;
      }

    if (Q->QTTarget > 0)
      {
      if (QTDel > Q->QTTarget)
        continue;

      QTSpan = Q->QTTarget - QTDel;
      }

    /* targets are satisfied */

    if (XFSpan < BestXFSpan)
      {
      /* improved match located */

      BestXFQ = Q;

      BestXFSpan = XFSpan;
      }

    if (QTSpan < BestQTSpan)
      {
      /* first match located */

      BestQTQ = Q;

      BestQTSpan = QTSpan;
      }
    }    /* END for (qindex) */

  bmclear(&QAL);

  if ((BestXFQ == NULL) && (BestQTQ == NULL))
    {
    /* cannot located acceptable QOS */

    return(FAILURE);
    }

  /* determine which performance metric is closest */

  Q = NULL;

  BestDistance = 999999999.9;

  Distance = BestXFSpan / XFDel;

  if (Distance < BestDistance)
    {
    BestDistance = Distance;

    Q = BestXFQ;
    }

  Distance = BestQTSpan / MAX(1,QTDel);

  if (Distance < BestDistance)
    {
    BestDistance = Distance;

    Q = BestQTQ;
    }

  if (Q == NULL)
    {
    return(FAILURE);
    }

  if (QDel != NULL)
    *QDel = Q;
  else
    J->QOSDelivered = Q;

  return(SUCCESS);
  }  /* END MJobGetQOSDel() */



/**
 * Report estimated node count requirements for job.
 *
 * NOTE:  consider taskcount, nodecount, and required hostlist
 *
 * NOTE:  requires J, and J->Req[0]
 *
 * @param J (I)
 * @param MaxNC (O) [optional]
 * @param MinNC (O) [optional]
 * @param ComputeOnly (I)
 */

int MJobGetEstNC(

  const mjob_t  *J,
  int           *MaxNC,
  int           *MinNC,
  mbool_t        ComputeOnly)

  {
  int tmpMaxNC;
  int tmpMinNC;
  int RQNC;

  int rqindex;
  int hindex;

  mbitmap_t *JNMatchBM = NULL;  /* effective job node match policy (bitmap) */

  mreq_t *RQ;

  if ((J == NULL) || (J->Req[0] == NULL))
    {
    if (MaxNC != NULL)
      *MaxNC = 0;

    if (MinNC != NULL)
      *MinNC = 0;

    return(FAILURE);
    }

  /* if job requested a specific number of nodes use that */

  if ((MJOBISACTIVE(J)) && (J->Req[0] != NULL) && (!MNLIsEmpty(&J->Req[0]->NodeList)))
    {
    tmpMaxNC = 0;

    /* for active jobs just count number of nodes (shared and dedicated) */

    for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
      {
      RQ = J->Req[rqindex];

      if (RQ == NULL)
        break;

      tmpMaxNC += MNLCount(&J->Req[0]->NodeList);
      }    /* END for (rqindex) */

    if (MaxNC != NULL)
      *MaxNC = tmpMaxNC;

    if (MinNC != NULL)
      *MinNC = tmpMaxNC;    

    return(SUCCESS);
    }  /* END if (MJOBISACTIVE(J)) */

  if (J->Request.NC != 0)
    {
    if (MaxNC != NULL)
      *MaxNC = J->Request.NC;

    if (MinNC != NULL)
      *MinNC = J->Request.NC;

    return(SUCCESS);
    }

  tmpMaxNC = MSched.M[mxoNode];
  tmpMinNC = 0;

  /* use taskcount if nodecount wasn't specified */

  if (J->Request.TC > 0)
    tmpMaxNC = J->Request.TC;

  /* count the number of requested hosts and use HostList mode to calculate */

  if (!MNLIsEmpty(&J->ReqHList))
    {
    hindex = MNLCount(&J->ReqHList);

    if (J->ReqHLMode == mhlmExactSet)
      {
      if (MaxNC != NULL)
        *MaxNC = hindex;

      if (MinNC != NULL)
        *MinNC = hindex;

      return(SUCCESS);
      }

    if (J->ReqHLMode == mhlmSubset)
      {
      tmpMinNC = MAX(tmpMinNC,hindex);
      }
    else if (J->ReqHLMode == mhlmSuperset)
      {
      tmpMaxNC = MIN(tmpMaxNC,hindex);
      }
    else
      {
      tmpMaxNC = hindex;
      }
    }    /* END if (J->ReqHList != NULL) */

  /* NOTE:  support single req only */

  RQNC = 0;

  for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
    {
    RQ = J->Req[rqindex];

    if (RQ == NULL)
      break;

    if (RQ->TasksPerNode >= 1)
      {
      RQNC += MIN(tmpMaxNC,J->Request.TC / RQ->TasksPerNode);
      }
    else
      {
      RQNC++;
      }
    }

  /* max of requested and inferred */
  /* NOTE: this routine estimates the highest possible nodecount */

  tmpMaxNC = MAX(RQNC,tmpMaxNC);

  if (MPar[0].MaxPPN > 0)
    {
    tmpMinNC = MAX(tmpMinNC,J->Request.TC / MPar[0].MaxPPN);
    }

  JNMatchBM = ((J->Credential.U != NULL) && (!bmisclear(&J->Credential.U->JobNodeMatchBM))) ?
    &J->Credential.U->JobNodeMatchBM :
    &MPar[0].JobNodeMatchBM;

  if (bmisset(JNMatchBM,mnmpExactProcMatch))
    {
    /* NYI */
    }

  if (bmisset(JNMatchBM,mnmpExactNodeMatch))
    {
    /* NYI */
    }

  if (MaxNC != NULL)
    *MaxNC = MAX(tmpMinNC,tmpMaxNC);

  if (MinNC != NULL)
    *MinNC = MIN(tmpMinNC,tmpMaxNC);

  return(SUCCESS);
  }  /* END MJobGetEstNC() */





/**
 * NOTE: return only historical and priority based estimates.
 *
 * @param JSpec        (I) [jobid|jobdesc]
 * @param P            (I) /O-(optional-best par) [optional-required par]
 * @param StartTime    (O) [optional]
 * @param JEP          (O) [optional]
 * @param DoHistorical (I)
 * @param RE           (I) [optional] 
 * @param Auth         (I) [optional]
 * @param EMsg         (O) [optional]
 * @param EMsgSize     (I)
 */

int MJobGetEstStartTime(

  char      *JSpec,
  mpar_t    *P,
  mulong    *StartTime,
  mxml_t   **JEP,
  mbool_t    DoHistorical,
  mxml_t    *RE,
  char      *Auth,
  char      *EMsg,
  int        EMsgSize)

  {
  double  Priority;

  mulong  TotalProcSeconds;
  mulong  EstStartTime;

  mulong  tmpWCTime; 

  int     rqindex;

  int     PC;

  mjob_t *J;
  mreq_t *RQ;

  mjob_t *JP;

  static mnl_t tmpNL;
  static mbool_t Init = FALSE;

  if (JSpec == NULL)
    {
    return(FAILURE);
    }

  if (Init == FALSE)
    {
    MNLInit(&tmpNL);

    Init = TRUE;
    }

  EstStartTime = 0;

  MJobMakeTemp(&J);

  if (MJobFromJSpec(
        JSpec,
        &J,    /* O */
        RE,
        Auth,
        NULL,
        EMsg,  /* O */
        EMsgSize,
        NULL) == FAILURE)
    {
    MJobFreeTemp(&J);

    return(FAILURE);
    }

  /* do feasibility test */

  for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
    {
    RQ = J->Req[rqindex];

    if (RQ == NULL)
      {
      break;
      }

    if (MReqGetFNL(
         J,
         RQ,
         (P != NULL) ? P : &MPar[0],
         NULL,   /* I */
         &tmpNL,  /* O (ignored) */
         NULL,
         NULL,
         MMAX_TIME,
         0,
         EMsg) == FAILURE)
      {
      MJobFreeTemp(&J);

      return(FAILURE);
      }
    }    /* END for (rqindex) */

  if ((J->PStartPriority[(P != NULL) ? P->Index : 0] == 0) || (bmisset(&J->IFlags,mjifTemporaryJob)))
    {
    /* must dynamically generate priority info for this job */

    MJobCalcStartPriority(J,P,&Priority,mpdJob,NULL,0,mfmHuman,FALSE);
    }
  else
    {
    Priority = J->PStartPriority[(P != NULL) ? P->Index : 0];
    }

  TotalProcSeconds = 0;

  if (MJOBISACTIVE(J) == TRUE)
    {
    EstStartTime = J->StartTime;
    }
  else if (J->Rsv != NULL)
    {
    EstStartTime = J->Rsv->StartTime;
    }
  else if (DoHistorical == TRUE)
    {
    int ProcIndex = 0;
    int TimeIndex = 0;

    PC = J->TotalProcCount;

    while (J->SpecWCLimit[0] > MStat.P.TimeStep[TimeIndex])
      {
      TimeIndex++;
      }

    while (PC > (int)MStat.P.ProcStep[ProcIndex])
      {
      ProcIndex++;
      }

    /* attempt to use short-term stats first, then try long term stats */

    if ((MStat.SMatrix[1].Grid[TimeIndex][ProcIndex].JC < 5) &&
        (MStat.SMatrix[1].Grid[TimeIndex][ProcIndex].JC != 0))
      {
      /* use current short-term interval */

      EstStartTime = 
        MStat.SMatrix[1].Grid[TimeIndex][ProcIndex].TotalQTS / 
        MStat.SMatrix[1].Grid[TimeIndex][ProcIndex].JC;
      }
    else if ((MStat.SMatrix[2].Grid[TimeIndex][ProcIndex].JC < 5) &&
             (MStat.SMatrix[2].Grid[TimeIndex][ProcIndex].JC != 0))
      {
      /* use previous short-term interval */

      EstStartTime =
        MStat.SMatrix[2].Grid[TimeIndex][ProcIndex].TotalQTS /
        MStat.SMatrix[2].Grid[TimeIndex][ProcIndex].JC;
      }
    else if (MStat.SMatrix[0].Grid[TimeIndex][ProcIndex].JC != 0)
      {
      /* use long-term stats */

      EstStartTime =
        MStat.SMatrix[0].Grid[TimeIndex][ProcIndex].TotalQTS /
        MStat.SMatrix[0].Grid[TimeIndex][ProcIndex].JC;
      }
    else if (MPar[0].S.JC > 0)
      {
      /* use general long-term stats */

      EstStartTime =
        MPar[0].S.TotalQTS /
        MPar[0].S.JC;
      }
    else
      {
      /* no stats available */

      EstStartTime = 0;
      }

    EstStartTime += MSched.Time;
    }   /* END else if (DoHistorical == TRUE) */
  else
    {
    int jindex = 0;

    mhashiter_t HTI;

    MUHTIterInit(&HTI);

    while (MUHTIterate(&MAQ,NULL,(void **)&JP,NULL,&HTI) == SUCCESS)
      {
      PC = JP->TotalProcCount;
      tmpWCTime = (JP->StartTime + JP->WCLimit) - MSched.Time;

      if ((JP->Credential.U->Stat.JobAcc > 0) && (JP->Credential.U->Stat.JC > 0))
        {
        TotalProcSeconds +=
          (mulong)(tmpWCTime * (JP->Credential.U->Stat.JobAcc / JP->Credential.U->Stat.JC) * PC);
        }
      else
        {
        TotalProcSeconds += tmpWCTime * PC;
        }
      }    /* END for (jindex) */

    MDB(8,fSTRUCT) MLog("INFO:      Backlog estimate for job '%s' ActiveProcSeconds=%ld TJC=%d\n",
        J->Name,
        TotalProcSeconds,
        MAQ.NumItems);

    for (jindex = 0;MUIQ[jindex] != NULL;jindex++)
      {
      JP = MUIQ[jindex];

      if (JP == J)
        break;

      if ((JP == NULL) || (JP->Name[0] == '\0') || (JP->Name[0] == '\1'))
        continue;

      if (JP->PStartPriority[(P != NULL) ? P->Index : 0] < Priority)
        break;

      PC = JP->TotalProcCount;

      tmpWCTime = JP->SpecWCLimit[1];

      if ((JP->Credential.U->Stat.JobAcc > 0) && (JP->Credential.U->Stat.JC > 0))
        {
        TotalProcSeconds +=
          (mulong)(tmpWCTime * (JP->Credential.U->Stat.JobAcc / JP->Credential.U->Stat.JC) * PC);
        }
      else
        {
        TotalProcSeconds += tmpWCTime * PC;
        }
      }

    MDB(8,fSTRUCT) MLog("INFO:      Backlog estimate for job '%s'(priority=%.2f) IdleProcSeconds=%ld TJC=%d \n",
        J->Name,
        Priority,
        TotalProcSeconds,
        jindex);
    }

  if (JEP != NULL)
    {
    int tmpI;

    mxml_t *JE = NULL;

    /* generate xml response */

    if (*JEP == NULL)
      {
      MXMLCreateE(&JE,(char *)MXO[mxoJob]);

      if (MUStrIsEmpty(J->Name))
        {
        MXMLSetAttr(JE,(char *)MJobAttr[mjaJobID],(void *)J->Name,mdfString);
        }
      else if (J->SystemJID != NULL)
        {
        MXMLSetAttr(JE,(char *)MJobAttr[mjaJobID],(void *)J->SystemJID,mdfString);
        }

      MXMLSetAttr(JE,"now",(void *)&MSched.Time,mdfLong);

      tmpI = J->Req[0]->DRes.Procs * J->Request.TC;

      MXMLSetAttr(JE,(char *)MJobAttr[mjaReqProcs],(void *)&tmpI,mdfInt);
      MXMLSetAttr(JE,(char *)MJobAttr[mjaReqAWDuration],(void *)&J->SpecWCLimit[0],mdfLong);
      MXMLSetAttr(JE,(char *)MJobAttr[mjaPAL],(void *)P->Name,mdfString);

      *JEP = JE;
      }

    if (EstStartTime == 0)
      {
      TotalProcSeconds = TotalProcSeconds / MAX(1,MSched.TotalSystemProcs);

      EstStartTime = MSched.Time + TotalProcSeconds;
      }
    }    /* END if (JEP != NULL) */

  if (JEP != NULL)
    {
    if (DoHistorical == TRUE)
      {
      MXMLSetAttr(*JEP,"HistoricalEstStart",(void *)&EstStartTime,mdfLong);
      }
    else
      {
      MXMLSetAttr(*JEP,"PriorityEstStart",(void *)&EstStartTime,mdfLong);
      }
    }  /* END if (JEP != NULL) */

  if (StartTime != NULL)
    *StartTime = EstStartTime;

  MJobFreeTemp(&J);

  return(SUCCESS);
  }  /* END MJobGetEstStartTime() */



/**
 *
 *
 * @param J (I)
 * @param Cmd (O)
 * @param CmdSize (I)
 */

int MJobGetCmd(

  mjob_t *J,        /* I */
  char   *Cmd,      /* O */
  int     CmdSize)  /* I */

  {
  char *BPtr = NULL;
  int   BSpace = 0;

  if (Cmd != NULL)
    Cmd[0] = '\0';

  if (J == NULL)
    {
    return(FAILURE);
    }

  if (J->Env.Cmd != NULL)
    {
    MUSNInit(&BPtr,&BSpace,Cmd,CmdSize);

    if (!strncmp(J->Env.Cmd,"$HOME",strlen("$HOME")))
      {
      MUSNPrintF(&BPtr,&BSpace,"%s%s",
        J->Credential.U->HomeDir,
        &J->Env.Cmd[strlen("$HOME")]);

      return(SUCCESS);
      }
    }

  return(FAILURE);
  }  /* END MJobGetCmd() */





/**
 * Get 'initial working directory' (IWD) associated with job.
 *
 * @param J (I)
 * @param IWD (O)
 * @param IWDSize (I)
 * @param ExpandHome (I)
 */

int MJobGetIWD(

  mjob_t  *J,          /* I */
  char    *IWD,        /* O */
  int      IWDSize,    /* I */
  mbool_t  ExpandHome) /* I */

  {
  char *BPtr = NULL;
  int   BSpace = 0;

  const char *FName = "MJobGetIWD";

  MDB(7,fSCHED) MLog("%s(%s,IWD,%d,%s)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    IWDSize,
    MBool[ExpandHome]);

  if (IWD != NULL)
    IWD[0] = '\0';

  if (J == NULL)
    {
    return(FAILURE);
    }

  if ((J->Env.IWD != NULL) && (J->Credential.U != NULL))
    {
    MUSNInit(&BPtr,&BSpace,IWD,IWDSize);

    if ((!strncmp(J->Env.IWD,"$HOME",strlen("$HOME"))) && (ExpandHome == TRUE))
      {
      /* expand $HOME */

      MUSNPrintF(&BPtr,&BSpace,"%s%s",
        J->Credential.U->HomeDir,
        &J->Env.IWD[strlen("$HOME")]);
      }
    else if (!strcmp(J->Env.IWD,NONE) && (J->Credential.U->HomeDir != NULL))
      {
      MUSNPrintF(&BPtr,&BSpace,"%s",
        J->Credential.U->HomeDir);
      }
    else
      {
      MUSNPrintF(&BPtr,&BSpace,"%s",
        J->Env.IWD);
      }
    }
  else if ((J->Credential.U != NULL) && (J->Credential.U->HomeDir != NULL))
    {
    /* no IWD specified, use U->HomeDir */

    MUStrCpy(IWD,J->Credential.U->HomeDir,IWDSize);
    }
  else
    {
    if (MSched.VarDir[0] != '\0')
      {
      snprintf(IWD,IWDSize,"%s/spool",
        MSched.VarDir);
      }
    else
      {
      MUStrCpy(IWD,"/tmp",IWDSize);
      }
    }

  return(SUCCESS);
  }  /* END MJobGetIWD() */





/**
 *
 *
 * @param Tree
 * @param J
 * @param Account
 * @param Class
 * @param User
 * @param Group
 * @param QOS
 */

int __MJobGetFSTree(

  mxml_t  *Tree,
  mjob_t  *J,
  mbool_t *Account,
  mbool_t *Class,
  mbool_t *User,
  mbool_t *Group,
  mbool_t *QOS)

  {
  mxml_t *CE;

  int     CTok;

  mfst_t *TData;

  mbool_t OAccount;
  mbool_t OClass;
  mbool_t OUser;   
  mbool_t OGroup;
  mbool_t OQOS;    

  if ((Tree == NULL) ||
      (J == NULL) ||
      (Account == NULL) ||
      (Class == NULL) ||
      (User == NULL) ||
      (Group == NULL) ||
      (QOS == NULL))
    {
    return(FAILURE);
    }

  OAccount = *Account;
  OClass   = *Class;
  OUser    = *User;
  OGroup   = *Group;
  OQOS     = *QOS;

  TData = Tree->TData;

  if ((MPar[0].FSC.FSTreeAcctShallowSearch == TRUE) &&
      (*Account == TRUE) && 
      (TData->OType == mxoAcct))
    {
    /* normally we do a deep search of the tree looking for
       a user in any of the subaccounts below the requested account.
       This option retricts the search to child nodes of the requested account. */

    /* This is a sub-account of the requested account */

    return(FAILURE);
    }

  switch (TData->OType)
    {
    case mxoUser:

      if (J->Credential.U == TData->O)
        *User = TRUE;
      else if (!strcmp(((mgcred_t *)TData->O)->Name,"ALL"))
        *User = TRUE;
      else if (*User == MBNOTSET)
        *User = FALSE;

      break;

    case mxoAcct:

      if (J->Credential.A == TData->O)
        *Account = TRUE;
      else if (!strcmp(((mgcred_t *)TData->O)->Name,"ALL"))
        *Account = TRUE;
      else if (*Account == MBNOTSET)
        *Account = FALSE;

      break;

    case mxoClass:

      if (J->Credential.C == TData->O)
        *Class = TRUE;
      else if (!strcmp(((mgcred_t *)TData->O)->Name,"ALL"))
        *Class = TRUE;
      else if (*Class == MBNOTSET)
        *Class = FALSE;

      break;

    case mxoGroup:

      if (J->Credential.G == TData->O)
        *Group = TRUE;
      else if (!strcmp(((mgcred_t *)TData->O)->Name,"ALL"))
        *Group = TRUE;
      else if (*Group == MBNOTSET)
        *Group = FALSE;

      break;

    case mxoQOS:

      if (J->Credential.Q == TData->O)
        *QOS = TRUE;
      else if (!strcmp(((mgcred_t *)TData->O)->Name,"ALL"))
        *QOS = TRUE;
      else if (*QOS == MBNOTSET)
        *QOS = FALSE;

      break;

    default:

      /* NO-OP */

      break;
    }

  CTok = -1;

  if ((Tree->CCount == 0) && (TData->OType != mxoNONE))
    {
    /* check if this leaf matches the job */

    if ((*Account != FALSE) &&
        (*Group != FALSE) &&
        (*User != FALSE) &&
        (*Class != FALSE) &&
        (*QOS != FALSE) &&
        ((MSched.FSTreeUserIsRequired == FALSE) ||
         (*User == TRUE)))

      {
      /* point job to this leaf for this partition */

      if (J->FairShareTree == NULL)
        {
        J->FairShareTree = (mxml_t **)MUCalloc(1,sizeof(mxml_t *) * MMAX_PAR);
        }

      J->FairShareTree[TData->PtIndex] = Tree;

      return(SUCCESS);
      }
    }

  while (MXMLGetChild(Tree,NULL,&CTok,&CE) == SUCCESS)
    {
    if (__MJobGetFSTree(CE,J,Account,Class,User,Group,QOS) == SUCCESS)
      {
      return(SUCCESS);
      }
    }

  *Account = OAccount;
  *Class   = OClass;
  *User    = OUser;
  *Group   = OGroup;
  *QOS     = OQOS;

  return(FAILURE);
  }  /* END __MJobGetFSTree() */




/**
 *
 *
 * @param J (I) [modified] 
 * @param PReq (I) [optional] only get fstree for requested partition 
 * @param ForceRecreate (I) 
 */

int MJobGetFSTree(

  mjob_t        *J,
  const mpar_t  *PReq,
  mbool_t        ForceRecreate)

  {
  int pindex = 0;

  mpar_t *P = NULL;

  mbool_t Account = FALSE;
  mbool_t Class = FALSE;
  mbool_t User = FALSE;
  mbool_t Group = FALSE;
  mbool_t QOS = FALSE;

  const char *FName = "MJobGetFSTree";

  MDB(5,fSCHED) MLog("%s(%s,%s,%s)\n",
    FName,
    (J != NULL) ? J->Name : NULL,
    (PReq != NULL) ? PReq->Name : NULL,
    MBool[ForceRecreate]);

  if (J == NULL)
    return(FAILURE);

  if ((J->FairShareTree != NULL) && 
      (PReq != NULL) &&
      (PReq->Index != 0) && 
      (J->FairShareTree[PReq->Index] != NULL) &&
      (ForceRecreate == FALSE))
    {
    MDB(7,fSTRUCT) MLog("INFO:     job %s already has fstree loaded for partition (%s), not reloading\n",
      J->Name,
      (PReq->Name != NULL) ? PReq->Name : "");

    return(SUCCESS);
    }

  /* if partition requested then use that index and if not then leave pindex at 0 */

  if (PReq != NULL)
    {
    pindex = PReq->Index;
    }

  /* if pindex is 0 then read all the partitions, otherwise just the requested partition */

  for (;pindex < MSched.M[mxoPar];pindex++)
    {
    P = &MPar[pindex];

    if (P->Name[0] == '\0')
      break;

    if (P->Name[0] == '\1')
      continue;

/*
 *   commented out for llnl - we may not have done a workload query yet for this partition
 *   but we should still be able to process the FSTree info for this partition.
 *
 *   if (P->ConfigNodes == 0)
 *     continue;
 */

    Account = MBNOTSET;
    Class = MBNOTSET;
    User = MBNOTSET;
    Group = MBNOTSET;
    QOS = MBNOTSET;

    if ((J->FairShareTree != NULL) && (J->FairShareTree[pindex] != NULL))
      {
      if (ForceRecreate == FALSE)
        continue;

      J->FairShareTree[pindex] = NULL;
      }

    __MJobGetFSTree(P->FSC.ShareTree,J,&Account,&Class,&User,&Group,&QOS);

    if ((J->FairShareTree != NULL) && (J->FairShareTree[pindex] != NULL))
      {
      MDB(7,fSTRUCT) MLog("INFO:     job %s fstree loaded for partition %s\n",
        J->Name,
        P->Name);
      }

    /* if we have read the requested partition then no need to iterate on the rest of the partitions */

    if ((PReq != NULL) && (PReq->Index != 0)) 
      break;
    }  /* END for (pindex) */

  __MJobGetFSTree(MPar[0].FSC.ShareTree,J,&Account,&Class,&User,&Group,&QOS);

  bmset(&J->IFlags,mjifFSTreeProcessed);

  return(SUCCESS);
  }  /*END MJobGetFSTree() */




/**
 * Returns a list of attr/value pairs (a list of variables). This
 * list includes the job's user, job variables (J->Variables), the
 * job's hostlist, and the job group variables.
 *
 * @param J         (I)
 * @param VarList   (O)
 * @param Buf       (O)
 * @param BufCountP (I/O)
 */

int MJobGetVarList(

  mjob_t  *J,
  mln_t  **VarList,
  char     Buf[][MMAX_BUFFER3],
  int     *BufCountP)

  {
  mjob_t *tmpJ;

  if ((J == NULL) || (VarList == NULL))
    {
    return(FAILURE);
    }

  MOGetCommonVarList(
    mxoJob,
    J->Name,
    J->Credential.U,
    VarList,
    Buf,
    BufCountP);  /* I/O */

  if (J->Credential.U != NULL)
    {
    MUAddVarLL(VarList,(char *)MJVarName[mjvnOwner],J->Credential.U->Name);
    }

  MUAddVarLL(VarList,(char *)MJVarName[mjvnJobID],J->Name);

  /* generate hostlist */

  if (!MNLIsEmpty(&J->NodeList) && (*BufCountP > 2))
    {
    char *BPtr;
    int   BSpace;

    int  nindex;

    mnode_t *N;

    mnl_t *NL = &J->NodeList;

    MUSNInit(&BPtr,&BSpace,Buf[*BufCountP],sizeof(Buf[0]));

    for (nindex = 0;MNLGetNodeAtIndex(NL,nindex,&N) == SUCCESS;nindex++)
      {
      if (nindex > 0)
        {
        MUSNPrintF(&BPtr,&BSpace,",%s",
          N->Name);
        }
      else
        {
        MUSNCat(&BPtr,&BSpace,N->Name);

        MUAddVarLL(VarList,(char *)MJVarName[mjvnMasterHost],N->Name);
        }
      }

    sprintf(Buf[*BufCountP - 1],"%d",
      nindex);

    /* always specify HOSTLIST - why? */

    MUAddVarLL(VarList,(char *)MJVarName[mjvnHostList],(Buf[*BufCountP][0] != '\0') ? Buf[*BufCountP] : (char *)NullP);

    MUAddVarLL(VarList,(char *)MJVarName[mjvnNodeCount],Buf[*BufCountP - 1]);
    }   /* END if (J->NodeList != NULL) */

  *BufCountP -= 2;

  /* add job variables */

  if (J->Variables.Table != NULL)
    {
    mhashiter_t Iter;
    char *tmpName;
    char *tmpVal;

    MUHTIterInit(&Iter);

    while (MUHTIterate(&J->Variables,&tmpName,(void **)&tmpVal,NULL,&Iter) == SUCCESS)
      {
      MUAddVarLL(VarList,tmpName,(tmpVal != NULL) ? tmpVal : (char *)"NULL");
      }
    }

  if (J->JGroup != NULL)
    {
    tmpJ = NULL;

    if (MJobFind(J->JGroup->Name,&tmpJ,mjsmExtended) == SUCCESS)
      {
      /* grab variables from master job */

      if (tmpJ->Variables.Table != NULL)
        {
        mhashiter_t Iter;
        char *tmpName;
        char *tmpVal;

        MUHTIterInit(&Iter);

        while (MUHTIterate(&tmpJ->Variables,&tmpName,(void **)&tmpVal,NULL,&Iter) == SUCCESS)
          {
          MUAddVarLL(VarList,tmpName,(tmpVal != NULL) ? tmpVal : (char *)"NULL");
          }
        }
      }
    }    /* END if (J->JGroup != NULL) */

  return(SUCCESS);
  }  /* END MJobGetVarList() */




/**
 * Determine effective job node access policy.
 *
 * @param J (I)
 * @param SRQ (I) [optional]
 * @param SP (I) [optional]
 * @param SN (I) [optional]
 * @param NAPolicyP (O)
 */

int MJobGetNAPolicy(

  const mjob_t               *J,
  const mreq_t               *SRQ,
  const mpar_t               *SP,
  const mnode_t              *SN,
  enum MNodeAccessPolicyEnum *NAPolicyP)

  {
  const mpar_t *P;
  const mreq_t *RQ;

  int nindex;

  enum MNodeAccessPolicyEnum tmpNAPolicy;

  if (NAPolicyP == NULL)
    {
    return(FAILURE);
    }
  
  *NAPolicyP = MSched.DefaultNAccessPolicy;

  if (J == NULL)
    {
    return(FAILURE);
    }

  RQ = (SRQ != NULL) ? SRQ : J->Req[0];

  if (SP != NULL)
    P = SP;
  else if ((RQ != NULL) && (RQ->PtIndex >= 0))
    P = &MPar[RQ->PtIndex];
  else
    P = &MPar[0];

  if ((SN != NULL) && (SN->SpecNAPolicy != mnacNONE))
    {
    /* node overrides partition */

    *NAPolicyP = SN->SpecNAPolicy;
    }
  else if (P->NAccessPolicy != mnacNONE)
    {
    /* partition overrides default */

    *NAPolicyP = P->NAccessPolicy;
    }
  
  /* job/req can only constrain options */ 

  if ((RQ != NULL) && (RQ->NAccessPolicy != mnacNONE))
    {
    /* can only constrain options */

    switch (*NAPolicyP)
      {
      case mnacNONE:
      case mnacShared: /* shared (default) is least constraining */

        *NAPolicyP = RQ->NAccessPolicy;

        break;

      case mnacSingleJob:

        if (RQ->NAccessPolicy == mnacSingleTask)
          *NAPolicyP = RQ->NAccessPolicy;

        break;

      case mnacSingleUser:
      case mnacSingleGroup:
      case mnacSingleAccount:

        /* if job access policy is 'more constraining, use it */

        /* NOTE:  singleuser NOT considered more constraining than singlegroup */
 
        if ((RQ->NAccessPolicy == mnacSingleTask) ||
            (RQ->NAccessPolicy == mnacSingleJob))
          *NAPolicyP = RQ->NAccessPolicy;

        break;

      case mnacUniqueUser:

        /* NYI */

        break;

      case mnacSingleTask:
      default:
    
        /* ignore job request */

        break;
      }  /* END switch (*NAPolicyP) */
    }    /* END if (RQ->NAccessPolicy != mnacNONE) */
  else if ((!MNLIsEmpty(&J->NodeList)) && (RQ != NULL))
    {
    mnode_t *N;
    enum MNodeAccessPolicyEnum naPolicy = mnacNONE;

    /* Scan through all job nodes and get the most restrictive Node Access Policy */

    for (nindex = 0;MNLGetNodeAtIndex(&J->NodeList,nindex,&N) == SUCCESS; nindex++)
      {
      MNodeGetEffNAPolicy(N,N->EffNAPolicy,P->NAccessPolicy,RQ->NAccessPolicy,&tmpNAPolicy);

      naPolicy = MAX(naPolicy,tmpNAPolicy);  
      }

    if (naPolicy != mnacNONE)
      *NAPolicyP = naPolicy;
    }

  return(SUCCESS);
  }  /* END MJobGetNAPolicy() */



/**
 * This function will return the job group via the GroupJob pointer
 * (if any exists) of the given job. If non exists, then the GroupJob pointer
 * will be set to NULL.
 *
 * @param J (I)
 * @param GroupJob (O)
 */

int MJobGetGroupJob(

  mjob_t *J,
  mjob_t **GroupJob)

  {
  mjob_t *JobPtr;

  *GroupJob = NULL;

  if ((J == NULL) || (J->JGroup == NULL))
    {
    return(FAILURE);
    }

  if (MJobFind(J->JGroup->Name,&JobPtr,mjsmExtended) == SUCCESS)
    {
    *GroupJob = JobPtr;
    
    return(SUCCESS);
    }

  return(FAILURE);
  }  /* END MJobGetGroupJob() */






/**
 * Return the effective JOBNODEMATCHPOLICY for a specific job.
 *
 * NOTE: order of precendence is User, 1st partition in PAL with a specified JNMP, global policy 
 *
 * @param J  (I)
 * @param BM (O)
 */

int MJobGetNodeMatchPolicy(

  mjob_t    *J,
  mbitmap_t *BM)

  {
  int pindex;

  mpar_t *P;

  const char *FName = "MJobGetNodeMatchPolicy";

  MDB(5,fCONFIG) MLog("%s(%s)\n",
    FName,
    (J != NULL) ? J->Name : "NULL");

  if ((J == NULL) || (BM == NULL))
    {
    return(FAILURE);
    }

  if ((J->Credential.U != NULL) && (!bmisclear(&J->Credential.U->JobNodeMatchBM)))
    {
    MDB(7,fSCHED) MLog("INFO:     job %s has user config job node match BM\n", 
      J->Name);

    bmcopy(BM,&J->Credential.U->JobNodeMatchBM);

    return(SUCCESS);
    }

  /* If MAPFEATURETOPARTITION is enabled and at least one feature was requested
     then modify the specified partition if a requested feature matches a partition */

  if ((MSched.MapFeatureToPartition == TRUE) &&
      (!bmisclear(&J->Req[0]->ReqFBM)))
    {
    mbool_t MatchFound = FALSE;

    /* look for a partition that matches one of the requested features */

    for (pindex = 1;pindex < MMAX_PAR;pindex++)
      {
      P = &MPar[pindex];
  
      if (P->Name[0] == '\0')
        break;
  
      if (P->Name[0] == '\1')
        continue;
  
      /* If a requested feature matches one of the partitions, then set that partition
         in the user specified partition list. A feature name which has nothing to do with
         partitions better not match a partition name. */

      if (MAttrSubset(&P->FBM,&J->Req[0]->ReqFBM,J->Req[0]->ReqFMode) == SUCCESS)
        {
        if (MatchFound == FALSE)
          {
          /* we have found at least one match so clear out the J->SpecPAL in preparation for
             setting the bits for partitions found in the feature bit map. */

          MatchFound = TRUE;

          bmclear(&J->SpecPAL);
          }

        /* update the user specified partition list with the partition that matched a requested feature */

        bmset(&J->SpecPAL,P->Index);

        MDB(3,fSCHED) MLog("INFO:     job %s feature mapped to partition %s updating SpecPAL\n",
          J->Name,
          P->Name);
        }
      } /* END for (pindex = 1;pindex < MMAX_PAR;pindex++) */

    if (MatchFound == TRUE)
      {
      /* make the change in J->PAL */

      MJobGetPAL(J,&J->SpecPAL,&J->SysPAL,&J->PAL);
      }
    } /* END If (MSched.MapFeatureToPartition == TRUE)... */

  for (pindex = 1;pindex < MMAX_PAR;pindex++)
    {
    P = &MPar[pindex];

    if (P->Name[0] == '\0')
      break;

    if (P->Name[0] == '\1')
      continue;

    if (P->ConfigNodes == 0)
      continue;

    /* check partition access */

    if (!bmisset(&J->PAL,P->Index))
      {
      char ParLine[MMAX_LINE];

      MDB(7,fSCHED) MLog("INFO:     job %s not allowed to run in partition %s (allowed %s)\n", 
        J->Name,
        P->Name,
        MPALToString(&J->PAL,NULL,ParLine));

      continue;
      }

    if (bmisclear(&P->JobNodeMatchBM))
      continue;

    bmcopy(BM,&P->JobNodeMatchBM);

    return(SUCCESS);
    }

  if (bmisclear(&J->NodeMatchBM))
    {
    /* only apply the default if the job didn't request something specific */

    bmcopy(BM,&MSched.GP->JobNodeMatchBM);
    }
 
  return(SUCCESS);
  }  /* END MJobGetNodeMatchPolicy() */








/**
 * Returns a job's Partition Access List from available partitions.
 *
 * Selects from requested partitions and system approved partitions.
 *
 * @param *J     (I) job                                
 * @param *RPAL  (I) requested partition access list    
 * @param *SPAL  (I) system approved partition access list  
 * @param *PAL   (O) final approved job partition access list
 */

int MJobGetPAL(

  mjob_t    *J,     /* I  job                                */
  mbitmap_t *RPAL,  /* I  requested partition access list    */
  mbitmap_t *SPAL,  /* I  system approved partition access list */ 
  mbitmap_t *PAL)   /* O  final approved job partition access list */

  {
  mbitmap_t tmpPAL;
 
  mbool_t AndMask;
 
  int  pindex;
 
  mclass_t *C;
  mgcred_t *U;
  mgcred_t *G;

  const char *FName = "MJobGetPAL";
 
  MDB(7,fSTRUCT) MLog("%s(%s,%s,%s,PAL)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (RPAL != NULL) ? "RPAL" : "NULL",
    (SPAL != NULL) ? "SPAL" : "NULL");
 
  if (J == NULL)
    {
    return(FAILURE);
    }

  MDB(7,fCONFIG)
    {
    char ParLine[MMAX_LINE];

    MLog("INFO:     job %s requested partition access list SpecPAL '%s'\n",
        J->Name,
        MPALToString(RPAL,NULL,ParLine));

    MLog("INFO:     job %s system partition access list SPAL '%s'\n",
        J->Name,
        MPALToString(SPAL,NULL,ParLine));
    }

  /* initialize */

  C = J->Credential.C;
  U = J->Credential.U;
  G = J->Credential.G;
 
  AndMask = FALSE;
 
  /* determine PAL */

  MDB(7,fCONFIG)
    {
    char ParLine[MMAX_LINE];

    MLog("INFO:     job %s default system partition access list '%s'\n",
        J->Name,
        MPALToString(&MPar[0].F.PAL,NULL,ParLine));
    }

  bmcopy(&tmpPAL,&MPar[0].F.PAL);
 
  MDB(7,fCONFIG)
    {
    char ParLine[MMAX_LINE];

    MLog("INFO:     job %s copy of default system partition access list '%s'\n",
        J->Name,
        MPALToString(&tmpPAL,NULL,ParLine));
    }

  MDB(7,fSTRUCT) MLog("INFO:     job %s default system partition access list type is (%d)\n",
    J->Name,
    MPar[0].F.PALType);

  if (MPar[0].F.PALType == malAND)
    {
    AndMask = TRUE;
    }
  else
    {
    /* obtain 'or' list */
 
    if ((J->Credential.U != NULL) && 
        (J->Credential.U->F.PALType == malOR) && 
        !bmisclear(&J->Credential.U->F.PAL))
      bmor(&tmpPAL,&J->Credential.U->F.PAL);
    else if ((MSched.DefaultU != NULL) && (MSched.DefaultU->F.PALType == malOR))
      bmor(&tmpPAL,&MSched.DefaultU->F.PAL);

    if ((G != NULL) && (G->F.PALType == malOR))
      bmor(&tmpPAL,&G->F.PAL);
 
    if ((J->Credential.A != NULL) && (J->Credential.A->F.PALType == malOR))
      bmor(&tmpPAL,&J->Credential.A->F.PAL);
 
    if ((C != NULL) && (C->F.PALType == malOR))
      bmor(&tmpPAL,&C->F.PAL);

    if ((J->Credential.Q != NULL) && (J->Credential.Q->F.PALType == malOR))
      bmor(&tmpPAL,&J->Credential.Q->F.PAL);
    }
 
  /* obtain 'exclusive' list */

  if ((J->Credential.Q != NULL) &&
      (J->Credential.Q->F.PALType == malAND) &&
      (!bmisclear(&J->Credential.Q->F.PAL)))
    {
    if (AndMask != TRUE)
      {
      bmcopy(&tmpPAL,&J->Credential.Q->F.PAL);

      AndMask = TRUE;
      }
    else
      {
      bmand(&tmpPAL,&J->Credential.Q->F.PAL);
      }
    }
 
  if ((C != NULL) && 
      (C->F.PALType == malAND) && 
      (!bmisclear(&C->F.PAL))) 
    {
    if (AndMask != TRUE)
      {
      bmcopy(&tmpPAL,&C->F.PAL);
 
      AndMask = TRUE;
      }
    else
      {
      bmand(&tmpPAL,&C->F.PAL);
      }
    }
 
  if ((J->Credential.A != NULL) && 
      (J->Credential.A->F.PALType == malAND) && 
      (!bmisclear(&J->Credential.A->F.PAL)))
    {
    if (AndMask != TRUE)
      {
      bmcopy(&tmpPAL,&J->Credential.A->F.PAL);
 
      AndMask = TRUE;
      }
    else
      {
      bmand(&tmpPAL,&J->Credential.A->F.PAL);
      }
    }

  if ((G != NULL) &&
      (G->F.PALType == malAND) && 
      (!bmisclear(&G->F.PAL)))
    {
    if (AndMask != TRUE)
      {
      bmcopy(&tmpPAL,&G->F.PAL);
 
      AndMask = TRUE; 
      }
    else
      {
      bmand(&tmpPAL,&G->F.PAL);
      }
    }
 
  if ((U != NULL) &&
      (U->F.PALType == malAND) &&
      (!bmisclear(&U->F.PAL)))                   
    {
    if (AndMask != TRUE)
      {
      bmcopy(&tmpPAL,&U->F.PAL);
 
      AndMask = TRUE;
      }
    else
      {
      bmand(&tmpPAL,&U->F.PAL);
      }
    }
  else if ((MSched.DefaultU != NULL) && (MSched.DefaultU->F.PALType == malAND) &&
           (!bmisclear(&MSched.DefaultU->F.PAL)))
    {
    if (AndMask != TRUE)
      {
      bmcopy(&tmpPAL,&MSched.DefaultU->F.PAL);

      AndMask = TRUE;
      }
    else
      {
      bmand(&tmpPAL,&MSched.DefaultU->F.PAL);
      }
    }

  if ((MSched.DefaultU != NULL) && (MSched.DefaultU->F.PALType == malAND) &&
      (!bmisclear(&MSched.DefaultU->F.PAL)))
    {
    if (AndMask != TRUE)
      {
      bmcopy(&tmpPAL,&MSched.DefaultU->F.PAL);

      AndMask = TRUE;
      }
    else
      {
      bmand(&tmpPAL,&MSched.DefaultU->F.PAL);
      }
    }

  if ((J->Credential.U != NULL) &&
      (J->Credential.U->F.PALType == malOnly) && 
      (!bmisclear(&J->Credential.U->F.PAL))) 
    {
    bmcopy(&tmpPAL,&U->F.PAL);
    }
  else if ((G != NULL) &&
           (G->F.PALType == malOnly) &&
           (!bmisclear(&G->F.PAL)))      
    {
    bmcopy(&tmpPAL,&G->F.PAL);
    }
  else if ((J->Credential.A != NULL) && (J->Credential.A->F.PALType == malOnly) &&
           (!bmisclear(&J->Credential.A->F.PAL)))      
    {
    bmcopy(&tmpPAL,&J->Credential.A->F.PAL);
    }
  else if ((C != NULL) && (C->F.PALType == malOnly) &&
           (!bmisclear(&C->F.PAL)))      
    {
    bmcopy(&tmpPAL,&C->F.PAL);
    }
  else if ((J->Credential.Q != NULL) && (J->Credential.Q->F.PALType == malOnly) &&
           (!bmisclear(&J->Credential.Q->F.PAL)))
    {
    bmcopy(&tmpPAL,&J->Credential.Q->F.PAL);
    }

  MDB(7,fCONFIG)
    {
    char ParLine[MMAX_LINE];

    MLog("INFO:     job %s tmpPAL default partition access list '%s'\n",
        J->Name,
        MPALToString(&tmpPAL,NULL,ParLine));
    }

  /* incorporate requested PAL */

  if ((RPAL != NULL) && (!bmisclear(RPAL)))
    bmand(&tmpPAL,RPAL);

  /* incorporate system approved PAL */

  if ((SPAL != NULL) && (!bmisclear(SPAL)))
    bmand(&tmpPAL,SPAL);

  MDB(7,fCONFIG)
    {
    char ParLine[MMAX_LINE];

    MLog("INFO:     job %s anded requested and system partition access list '%s'\n",
        J->Name,
        MPALToString(&tmpPAL,NULL,ParLine));
    }

  if ((J->DestinationRM != NULL) && ((J->TemplateExtensions == NULL) || (J->TemplateExtensions->WorkloadRM != J->DestinationRM)))
    {
    mbitmap_t RMPAL;

    bmset(&RMPAL,0);

    if (J->DestinationRM->PtIndex > 0)
      {
      /* combine with RMCFG[] PARTITION= to fix issue 
         where resource managers overlap partitions
         and job gets locked into wrong partition */

      bmset(&RMPAL,J->DestinationRM->PtIndex);
      }

    for (pindex = 1;pindex < MMAX_PAR;pindex++)
      {
      if (MPar[pindex].Name[0] == '\1')
        continue;

      if (MPar[pindex].Name[0] == '\0')
        break;

      if ((MPar[pindex].RM != NULL) &&
          (MPar[pindex].RM == J->DestinationRM))
        {
        bmset(&RMPAL,pindex);
        }
      }     /* END for (pindex) */

    MDB(7,fCONFIG)
      {
      char ParLine[MMAX_LINE];

      MLog("INFO:     job %s destination rm partition access list '%s'\n",
          J->Name,
          MPALToString(&RMPAL,NULL,ParLine));
      }

    /* allow job to only run on AND of previous PAL constraints
       and partitions available to RM */

    /* adjusting J->PAL may not be necessary, but should not cause problems */

    /* NOTE: update this code to find the real resource RM for the job */
    /* NYI */

    bmand(&tmpPAL,&RMPAL);
    }  /* END if (J->DRM != NULL) */

  if (PAL != NULL)
    {
    bmclear(PAL);

    bmcopy(PAL,&tmpPAL);

    MDB(7,fCONFIG)
      {
      char ParLine[MMAX_LINE];

      MLog("INFO:     job %s tmpPAL partition access list '%s'\n",
          J->Name,
          MPALToString(&tmpPAL,NULL,ParLine));

      MLog("INFO:     job %s PAL partition access list set to '%s' in MJobGetPAL\n",
          J->Name,
          MPALToString(PAL,NULL,ParLine));
      }
    }
 
  if ((RPAL != NULL) && (!bmisclear(RPAL)))
    {
    if (bmisclear(&tmpPAL))
      {
      char ParLine[MMAX_LINE];

      MDB(2,fSTRUCT) MLog("WARNING:  job %s cannot access requested partitions '%s'\n",
        J->Name,
        MPALToString(RPAL,NULL,ParLine));
 
      bmclear(&tmpPAL);

      return(FAILURE);
      }
    }
 
  bmclear(&tmpPAL);

  return(SUCCESS);
  }  /* END MJobGetPAL() */




/**
 * Return the job's default partition.
 *
 * NOTE: this is not currently used (neither is a job's default partition)
 *       but it could be useful in the future. Especially with per-partion
 *       scheduling.
 *
 * @param J (I)
 * @param PDef (O)
 */

int MJobGetPDef(

  mjob_t  *J,
  mpar_t **PDef)

  {
  int pindex;

  if ((J == NULL) || (PDef == NULL))
    {
    return(FAILURE);
    }
 
  /* determine allowed partition default (precedence: U,G,A,C,S,0) */
 
  if ((J->Credential.U != NULL) &&
      (J->Credential.U->F.PDef != NULL) &&
      (J->Credential.U->F.PDef != &MPar[0]) &&
       bmisset(&J->CurrentPAL,J->Credential.U->F.PDef->Index))
    {
    *PDef = J->Credential.U->F.PDef;
    }
  else if ((J->Credential.G != NULL) &&
           (J->Credential.G->F.PDef != NULL) &&
           (J->Credential.G->F.PDef != &MPar[0]) &&
            bmisset(&J->CurrentPAL,J->Credential.G->F.PDef->Index))
    {
    *PDef = J->Credential.G->F.PDef;
    }
  else if ((J->Credential.A != NULL) &&
           (J->Credential.A->F.PDef != NULL) &&
           (J->Credential.A->F.PDef != &MPar[0]) &&
            bmisset(&J->CurrentPAL,J->Credential.A->F.PDef->Index))
    {
    *PDef = J->Credential.A->F.PDef;
    }
  else if ((J->Credential.C != NULL) &&
           (J->Credential.C->F.PDef != NULL) &&
           (J->Credential.C->F.PDef != &MPar[0]) &&
            bmisset(&J->CurrentPAL,J->Credential.C->F.PDef->Index)) 
    {
    *PDef = J->Credential.C->F.PDef;
    }
  else if ((J->Credential.Q != NULL) &&
           (J->Credential.Q->F.PDef != NULL) &&
           (J->Credential.Q->F.PDef != &MPar[0]) &&
            bmisset(&J->CurrentPAL,J->Credential.Q->F.PDef->Index))
    {
    *PDef = J->Credential.Q->F.PDef;
    }
  else if ((MPar[0].F.PDef != NULL) &&
           (MPar[0].F.PDef != &MPar[0]))
    {
    *PDef = MPar[0].F.PDef;
    }
  else
    {
    *PDef = &MPar[MDEF_SYSPDEF];
    }

  /* verify access to default partition */

  if (!bmisset(&J->CurrentPAL,(*PDef)->Index))
    {
    *PDef = &MPar[0];

    /* locate first legal partition */

    for (pindex = 0;pindex < MMAX_PAR;pindex++)
      {
      if (MPar[pindex].Name[0] == '\1')
        continue;

      if (MPar[pindex].Name[0] == '\0')
        break;

      if (bmisset(&J->CurrentPAL,pindex))
        {
        *PDef = &MPar[pindex];

        break;
        }
      }    /* END for (pindex) */
    }      /* END if (!bmisset(&(*PDef)->Index,J->CPAL)) */

  MDB(3,fSTRUCT) MLog("INFO:     default partition for job %s set to %s (P:%s,U:%s,G:%s,A:%s,C:%s,Q:%s)\n",
    J->Name, 
    (*PDef)->Name,
    MPar[0].F.PDef->Name,
    (J->Credential.U == NULL) ? "NULL" : J->Credential.U->F.PDef->Name,
    (J->Credential.G == NULL) ? "NULL" : J->Credential.G->F.PDef->Name,
    (J->Credential.A == NULL) ? "NULL" : J->Credential.A->F.PDef->Name,
    (J->Credential.C == NULL) ? "NULL" : J->Credential.C->F.PDef->Name,
    (J->Credential.Q == NULL) ? "NULL" : J->Credential.Q->F.PDef->Name);

  return(SUCCESS);
  }  /* END MJobGetPDef() */









/**
 * Gets the number of nodeboards a job would need to run on.
 *
 * Determine number of nodeboards by number or requested procs and 
 * memory.
 * Request: 2proc,8mem NodeA=2proc,4mem = needs 2 of NodeA to fullfill
 * Request: 4proc,4mem NodeA=2proc,4mem = needs 2 of NodeA to fullfill
 *
 * @param J (I)
 * @param N (I)
 * @return Returns the number of nodeboards needed.
 */

int MJobGetNBCount(
   
  const mjob_t  *J,
  const mnode_t *N)

  {
  int         rqindex = 0;
  int         Result = 0;

  const char *FName = "MJobGetNBCount";

  MDB(5,fSTRUCT) MLog("%s(%s,%s)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (N != NULL) ? N->Name : "NULL");

  if (J == NULL || N == NULL)
    { 
    MDB(1,fSCHED) MLog("ERROR:    %s was NULL in MJobGetNBCount\n",
      (J == NULL) ? "J" : "N"); 

    return(FAILURE);
    }

  while (J->Req[rqindex] != NULL)
    {
    Result += MReqGetNBCount(J,J->Req[rqindex],N);

    rqindex++;
    }

  /* total lost resources is effectively the number of nodes consumed */

  MDB(7,fSTRUCT) MLog("INFO:     %d nodeboards needed for job %s using node %s (cprocs=%d cmem=%d)\n",
    Result,
    J->Name,
    N->Name,
    N->CRes.Procs,
    N->CRes.Mem);

  return(Result);
  } /*END int MJobGetNBCount() */




/**
 * Evaluate the possible destinations for this job by calling MJobGetRange() for each possible nodeset.
 *
 * NOTE: functions as a wrapper for MJobGetRange() 
 *
 * step 1 transform the job (optional)
 * step 2 gather all possible nodesets
 * step 3 step through nodesets and gather results for each
 * step 4 compare results for each nodeset and pick most desirable
 *
 * @param  J
 * @param  RQ
 * @param FNL
 * @param  P
 * @param  MinStartTime
 * @param  GRL
 * @param  MAvlNodeList (should ALWAYS be NULL as multi-req is not supported)
 * @param  NodeCount
 * @param  NodeMap
 * @param  RTBM
 * @param  SeekLong
 * @param  TRange
 * @param  BestFeature
 * @param  SEMsg
 */

int MJobGetRangeWrapper(

  mjob_t      *J,                  /* I job description                          */
  mreq_t      *RQ,                 /* I job req                                  */
  mnl_t      *FNL,                 /* I (optional) */
  mpar_t      *P,                  /* I partition in which resources must reside */
  long         MinStartTime,       /* I earliest possible starttime              */
  mrange_t    *GRL,                /* O array of time ranges located (optional,minsize=MMAX_RANGE) */
  mnl_t      **MAvlNodeList,       /* O multi-req list of nodes found            */
  int         *NodeCount,          /* O number of available nodes located (minsize=MMAX_NODE) */
  char        *NodeMap,            /* O (optional)                          */
  mbitmap_t   *RTBM,               /* I (bitmap)                                 */
  mbool_t      SeekLong,           /* I                                          */
  mrange_t    *TRange,             /* O (optional)                               */
  int         *BestFeature,        /* O */
  char        *SEMsg)              /* O (optional,minsize=MMAX_LINE)             */

  {
  char **SetList;

  int    SetIndex[MMAX_ATTR];
  int    MaxSet = 0;
  long   Delay = 0;

  int   sindex;
  int   findex;
  int   nindex;

  mnl_t NodeList = {0};
  msmpnode_t *SN;

  int   tmpSoonest;
  int   tmpBest;

  mrange_t     tmpGRL[MMAX_ATTR][MMAX_RANGE] = {{ {0UL,0UL,0,0,0,0UL,0UL,NULL,NULL} }};
  mrange_t     tmpTRange[MMAX_ATTR][MMAX_RANGE];
  
  int          tmpNC[MMAX_ATTR] = {0}; 
  char        *tmpNodeMap[MMAX_ATTR];

  const char *FName = "MJobGetRangeWrapper";

  MDB(5,fSTRUCT) MLog("%s(%s,RQ,P,%ld,GRL,MAvlNodeList,NodeCout,...)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    MinStartTime);
  
  if (BestFeature == NULL)
    {
    return(FAILURE);
    }

  *BestFeature = -1;
  
  if (!bmisclear(&RQ->ReqFBM))
    {
    return(MJobGetRange(
             J,
             RQ,
             NULL,
             P,
             MinStartTime,     /* I (earliest time job can start) */
             FALSE,
             GRL,              /* O */
             MAvlNodeList,
             NodeCount,        /* O */
             NodeMap,          /* O */
             RTBM,
             SeekLong,
             TRange,
             SEMsg));
    }

  /* only works with features for now */

  if ((RQ->SetList != NULL) && (RQ->SetList[0] != NULL))
    {
    SetList = RQ->SetList;
    }
  else if (MPar[0].NodeSetList != NULL)
    {
    SetList = MPar[0].NodeSetList;
    }
  else
    {
    SetList = NULL;
    }

  if ((SetList != NULL) && (SetList[0] != NULL))
    {
    for (sindex = 0;sindex < MMAX_ATTR;sindex++)
      {
      if ((SetList[sindex] == NULL) || (SetList[sindex][0] == '\0'))
        break;

      for (findex = 0;findex < MMAX_ATTR;findex++)
        {
        if (MAList[meNFeature][findex][0] == '\0')
          break;

        if (strcmp(MAList[meNFeature][findex],SetList[sindex]))
          continue;

        SetIndex[MaxSet] = findex;
        MaxSet++;

        break;
        }  /* END for (findex) */
      }    /* END for (sindex) */
    }      /* END if (SetList != NULL) */
  else
    {
    /* set not specified, determine all existing sets */

    for (findex = 0;findex < MMAX_ATTR;findex++)
      {
      if (MAList[meNFeature][findex][0] == '\0')
        break;

      SetIndex[MaxSet] = findex;
      MaxSet++;
      }  /* END for (findex) */
    }    /* END else (SetList != NULL) */

  SetIndex[MaxSet] = -1;
 
  memset(tmpNodeMap,0,sizeof(tmpNodeMap));

  for (sindex = 0;SetIndex[sindex] != -1;sindex++)
    {
    MNodeMapInit(&tmpNodeMap[sindex]);
    }

  for (sindex = 0;SetIndex[sindex] != -1;sindex++)
    {
    /* transform job from smp request into feasible job for this partition */

    MDB(7,fSTRUCT) MLog("INFO:     getting range for Job %s in nodeset %s\n",
      J->Name,
      SetList[sindex]);

    bmset(&RQ->ReqFBM,SetIndex[sindex]); 

    if (MJobPTransform(J,P,SetIndex[sindex],TRUE) == FAILURE)
      {
      bmunset(&RQ->ReqFBM,SetIndex[sindex]); 

      continue;
      }

    /* is this nodeset the best fit */

    if (MJobGetRange(
        J,
        RQ,
        NULL,
        P,
        MinStartTime,
        FALSE,
        tmpGRL[sindex],     /* O array of time ranges located (optional,minsize=MMAX_RANGE) */
        NULL,               /* O multi-req list of nodes found            */
        &tmpNC[sindex],     /* O number of available nodes located (minsize=MMAX_NODE) */
        tmpNodeMap[sindex], /* O (optional)                          */
        RTBM,
        SeekLong,
        tmpTRange[sindex],  /* O (optional)                               */
        SEMsg) == FAILURE)  /* O (optional,minsize=MMAX_LINE)             */
      {
      tmpGRL[sindex][0].STime = 0;
      }

    bmunset(&RQ->ReqFBM,SetIndex[sindex]); 

    MJobPTransform(J,P,SetIndex[sindex],FALSE);
    }

  /* evaluate results */

  nindex = 0;

  tmpSoonest = -1;

  MNLInit(&NodeList);

  for (sindex = 0;SetIndex[sindex] != -1;sindex++)
    {
    char TString[MMAX_LINE];
    /* check if tmpGRL[sindex] is best */

    if (tmpGRL[sindex][0].STime == 0)
      continue;

    MULToTString(tmpGRL[sindex][0].STime - MSched.Time,TString);

    MDB(7,fSTRUCT) MLog("INFO:     nodeset %s's starttime: %ld (%s) (%ld in the future)\n",
      SetList[sindex],
      tmpGRL[sindex][0].STime,
      TString,
      tmpGRL[sindex][0].STime - MSched.Time);

    /* creates list of representing nodes that represent smps 
     * to choose feasible set in MSMPGetMinLossIndex() */

    if ((SN = MSMPNodeFindByFeature(SetIndex[sindex])) != NULL)
      {
      MNLSetNodeAtIndex(&NodeList,nindex,SN->N);
      MNLSetTCAtIndex(&NodeList,nindex,SN->CRes.Procs); /* RT5543 - Give full smp proc count because all the vnodes aren't being
                                               added to the NodeList, just the smp nodes, else MJobSelectResourceSet
                                               will fail if the job requeusts more than one vnode's configured procs. */

      nindex++;
      }

    if (tmpSoonest == -1)
      {
      tmpSoonest = sindex;
      }
    else if ((tmpGRL[sindex][0].STime - MSched.Time) < 
             (tmpGRL[tmpSoonest][0].STime - MSched.Time))
      {
      /* Greater STime = earlier start time (STime - MSched.Time) */

      tmpSoonest = sindex;
      }
    }

  MNLTerminateAtIndex(&NodeList,nindex);

  /* determine best nodeset/smp with regards to min resources */

  if (bmisset(&J->IFlags,mjifHostList) == TRUE)
    {
    /* If the job has a hostlist, MSMPGetMinLostIndex will fail because
     * MJobSelectResourceSet won't do any work. tmpSoonest should be
     * the only one valid set unless nodes are spanning sets. */
    
    tmpBest = tmpSoonest;
    }
  else
    {
    tmpBest = MSMPGetMinLossIndex(
                J,
                SetList,
                SetIndex,
                &NodeList,
                MSched.Time + 1); /* look at configured not available */
    }

  if ((tmpBest == -1) || (tmpSoonest == -1))
    {
    MDB(7,fSTRUCT) MLog("INFO:     no best set/time found.\n");
  
    for (sindex = 0;SetIndex[sindex] != -1;sindex++)
      {
      MUFree(&tmpNodeMap[sindex]);
      }

    MNLFree(&NodeList);

    return(FAILURE);
    }
  
  if (tmpBest == tmpSoonest)
    {
    MDB(7,fSTRUCT) MLog("INFO:     best set's and soonest set's starttime is the same, not checking nodesetdelay\n");
    }
  else
    {
    /* check if nodesetdelay is set. if it is, see if the job can run within 
    * the delay time, else run in the soonest range */

    if (J->Req[0]->SetDelay > 0)
      Delay = J->Req[0]->SetDelay;
    else if (MPar[0].NodeSetDelay > 0)
      Delay = MPar[0].NodeSetDelay;

    if (Delay > 0)
      {
      mulong StartTime;

      StartTime = tmpGRL[tmpBest][0].STime - MSched.Time; 

      if (StartTime > (mulong)Delay)
        {
        MDB(7,fSTRUCT) MLog("INFO:     bestset's starttime is longer than nodesetdelay\n");

        tmpBest = tmpSoonest;
        }
      } /* END if (Delay > 0) */
    } /* END else (tmpBest == tmpSoonest) */

  *BestFeature = SetIndex[tmpBest];

  *NodeCount = tmpNC[tmpBest];
  memcpy(GRL,tmpGRL[tmpBest],sizeof(tmpGRL[tmpBest]));
  MNodeMapCopy(NodeMap,tmpNodeMap[tmpBest]);
  memcpy(TRange,tmpTRange[tmpBest],sizeof(tmpTRange[tmpBest]));

  for (sindex = 0;SetIndex[sindex] != -1;sindex++)
    {
    MUFree(&tmpNodeMap[sindex]);
    }

  MNLFree(&NodeList);

  MDB(7,fSTRUCT) MLog("INFO:     BestSet: %s  BestSTime: %ld (%ld in the future)\n",
    SetList[tmpBest],
    tmpGRL[tmpBest][0].STime,
    tmpGRL[tmpBest][0].STime - MSched.Time);

  return(SUCCESS);
  }  /* END MJobGetRangeWrapper() */






/**
 * Returns a list of standing reservations reservations that the job is running in.
 *
 * @param J (I) Job with the reservation to check.
 * @param RList (O) List to return standing reservations indexes in. Terminated with -1.
 */

int MJobGetOverlappingSRsvs(

  mjob_t    *J,     /*I*/
  marray_t  *RList) /*O*/

  {
  int sindex;

  msrsv_t *SR;
  mrsv_t  *Rsv;

  if ((J == NULL) || (RList == NULL))
    {
    return(FAILURE);
    }
    
  /* loop over each standing reservation */
  
  for (sindex = 0;sindex < MSRsv.NumItems;sindex++)
    {
    /* Can use R[0] since can't start jobs on depths greater than zero? */

    SR = (msrsv_t *)MUArrayListGet(&MSRsv,sindex);

    if (SR->Name[0] == '\0')
      break;

    Rsv = SR->R[0];

    /* see if job reservation and standing reservation overlap */

    if (MRsvHasOverlap(J->Rsv,Rsv,TRUE,&J->NodeList,NULL,NULL) == SUCCESS)
      {
      MDB(7,fSTRUCT) MLog("INFO:     job %s's reservation %s overlaps standing reservation %s\n",
        J->Name,
        J->Rsv->Name,
        SR->Name);

      MUArrayListAppendPtr(RList,Rsv);

      break;
      }
    } /* END for (sindex = 0; */

  return(SUCCESS);
  } /* END int MJobGetOverlappingSRsvs() */





/**
 *
 */

#define MMAX_NODESETLIST 16

int MJobGetRangeValue(

  mjob_t     *J,
  mpar_t     *P,
  mrange_t   *Range,
  int        *RangeValue,
  mulong     *MaxETime,   /* I/O (out should be minimum of IN and calculated OUT */
  int        *BestIndex,
  mbool_t    *DoAllocation,
  mnl_t     **tmpMNodeList,
  int        *NC)
  
  {

  char tmpLine[MMAX_LINE];

  const char *FName = "MJobGetRangeValue";

  MDB(5,fSCHED) MLog("%s(%s,%s)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (P != NULL) ? P->Name : "NULL");
 
  if ((J == NULL) || (Range == NULL) || (Range[0].TC == 0))
    {
    return(FAILURE);
    }

  *MaxETime = 0;
  *BestIndex = 0;

  *RangeValue = Range[0].STime;

  switch (MSched.NodeSetPlus)
    {
    default:
    case mnspNONE:

      *MaxETime   = 0;
      *BestIndex  = 0;

      *RangeValue = Range[0].STime;

      *DoAllocation = TRUE;

      break;

    case mnspSpanEvenly:

      {
      int  rindex;
      int  sindex;
      int  sindex2;
      int  nindex;
      int  findex;
     
      int *NodeSetCounts;
      int *NodeSetNLIndex;
      int  ReqTC;
      int  Count;
      int  MaxSet;
      int  tmpNodeCount;
     
      int  PerNodeSetTC;
      int  NodeSetTC;
      int  TotalTC;
      int  TC;

      mulong BestStartTime;
     
      int  SetIndex[MMAX_ATTR];

      long Delay = 0;
     
      mnl_t  NodeSetNL[MMAX_NODESETLIST];
     
      char **SetList;
      char  *tmpNodeMap = NULL;
     
      mbitmap_t BM;

      mnode_t *N;

      enum MResourceSetSelectionEnum NodeSetPolicy;

      bmset(&BM,mrtcStartEarliest);

      if (bmisset(&J->IFlags,mjifSpecNodeSetPolicy))
        NodeSetPolicy = J->Req[0]->SetSelection;
      else
        NodeSetPolicy = MPar[0].NodeSetPolicy;

      if (NodeSetPolicy == mrssNONE)
        {
        *MaxETime   = 0;
        *BestIndex  = 0;

        *RangeValue = Range[0].STime;

        *DoAllocation = TRUE;

        break;
        }

      if (J->Req[0]->SetDelay > 0)
        Delay = J->Req[0]->SetDelay;
      else
        Delay = MPar[0].NodeSetDelay;

      if ((J->Req[0]->SetList != NULL) && (J->Req[0]->SetList[0] != NULL))
        {
        SetList = J->Req[0]->SetList;
        }  /* END if ((J->Req[0]->SetList != NULL) && (J->Req[0]->SetList[0] != NULL)) */
      else if (MPar[0].NodeSetList != NULL)
        {
        SetList = MPar[0].NodeSetList;
        }
      else
        {
        MDB(1,fSCHED) MLog("ALERT:    cannot span nodesets, no nodesets defined\n");

        *MaxETime   = 0;
        *BestIndex  = 0;

        *RangeValue = Range[0].STime;

        *DoAllocation = TRUE;

        return(FAILURE);
        }
     
      MaxSet = 0;

      for (sindex = 0;sindex < MMAX_ATTR;sindex++)
        {
        if ((SetList[sindex] == NULL) || (SetList[sindex][0] == '\0'))
          break;
     
        for (findex = 0;findex < MMAX_ATTR;findex++)
          {
          if (MAList[meNFeature][findex][0] == '\0')
            break;
     
          if (strcmp(MAList[meNFeature][findex],SetList[sindex]))
            continue;
     
          SetIndex[MaxSet] = findex;
          MaxSet++;
     
          break;
          }  /* END for (findex) */
        }    /* END for (sindex) */
     
      if (MaxSet > MMAX_NODESETLIST)
        {
        MDB(1,fSCHED) MLog("ALERT:    internal nodeset buffer full, adjust MMAX_NODESETLIST\n");

        *MaxETime   = 0;
        *BestIndex  = 0;

        *RangeValue = Range[0].STime;

        *DoAllocation = TRUE;

        return(FAILURE);
        }

      ReqTC = J->Request.TC;
     
      SetIndex[MaxSet] = -1;
     
      NodeSetCounts  = (int *)MUMalloc(sizeof(int) * MaxSet);
      NodeSetNLIndex = (int *)MUMalloc(sizeof(int) * MaxSet);

      BestStartTime = Range[0].STime;
     
      MNodeMapInit(&tmpNodeMap);

      for (sindex = 0;sindex < MMAX_NODESETLIST;sindex++)
        {
        MNLInit(&NodeSetNL[sindex]);
        }

      for (rindex = 0;Range[rindex].ETime != 0;rindex++)
        {
        /* Step 1: get host lists that can start within start ranges */
      
        if (MJobGetRange(
              J,
              J->Req[0], 
              NULL,
              P,
              Range[rindex].STime,
              FALSE,
              NULL,                       /* do not pass GRL (not used) */
              tmpMNodeList,               /* O */
              &tmpNodeCount,              /* O */
              tmpNodeMap,                 /* O */
              &BM,
              MSched.SeekLong,
              NULL,
              NULL) == FAILURE)
          {
          /* cannot obtain range nodelist */
     
          MDB(2,fSCHED) MLog("ALERT:    cannot obtain nodelist for job %s in range %d\n",
            J->Name,
            rindex);
     
          break;
          }
 
        for (sindex = 0;SetIndex[sindex] != -1;sindex++)
          {
          MNLTerminateAtIndex(&NodeSetNL[sindex],0);
          NodeSetCounts[sindex] = 0;
          NodeSetNLIndex[sindex] = 0;
          }
    
        /* Step 2: for each nodeset, sum up the taskcount available */

        for (nindex = 0;MNLGetNodeAtIndex(tmpMNodeList[0],nindex,&N) == SUCCESS;nindex++)
          {
          for (sindex = 0;SetIndex[sindex] != -1;sindex++)
            {
            if (bmisset(&N->FBM,SetIndex[sindex]))
              {
              NodeSetCounts[sindex] += MNLGetTCAtIndex(tmpMNodeList[0],nindex);

              MNLSetNodeAtIndex(&NodeSetNL[sindex],NodeSetNLIndex[sindex],N);
              MNLSetTCAtIndex(&NodeSetNL[sindex],NodeSetNLIndex[sindex],MNLGetTCAtIndex(tmpMNodeList[0],nindex));
     
              NodeSetNLIndex[sindex]++;

              MNLTerminateAtIndex(&NodeSetNL[sindex],NodeSetNLIndex[sindex]);

              break;
              }
            }
          }

        MDB(5,fSCHED)
          {
          MLog("INFO:     taskcount available per nodeset for job '%s':\n",J->Name);

          for (sindex = 0;SetIndex[sindex] != -1;sindex++)
            {
            MLog("          %s[%d]\n",MAList[meNFeature][SetIndex[sindex]],NodeSetCounts[sindex]);
            }
          }
     
        /* at this point NodeSetCounts should have the available tasks for each nodeset */
        /* and NodeSetNL should have the available nodes for each nodeset */

        /* Step 3: determine if any single set or span of multiple nodesets can satisfy job */
    
        Count = 0;

        PerNodeSetTC = 0;
 
        /* J->Req[0]->NodeSetCount is 0 by default */

        if (J->Req[0]->NodeSetCount > 0)
          sindex = J->Req[0]->NodeSetCount - 1;
        else
          sindex = 0;

        for (;SetIndex[sindex] != -1;sindex++)
          {
          if ((J->Req[0]->NodeSetCount > 0) && (sindex != J->Req[0]->NodeSetCount - 1))
            {
            /* explicitly specified NodeSetCount, only allow this one variation */
      
            break;
            }

          if (ReqTC % (sindex + 1) != 0)
            continue;

          Count = 0;
     
          PerNodeSetTC = ReqTC / (sindex + 1);

#ifdef __MNOT
          if ((J->Req[0]->TasksPerNode > 0) &&
              (PerNodeSetTC % J->Req[0]->TasksPerNode != 0))
            {
            /* doesn't work */

            continue;
            }
#endif

          for (nindex = 0;SetIndex[nindex] != -1;nindex++)
            {
            if (NodeSetCounts[nindex] >= PerNodeSetTC)
              Count += PerNodeSetTC;
            }

          if (Count < ReqTC)
            {
            if (NodeSetPolicy == mrssOneOf)
              break;

            continue;
            }
     
          /* possible set found */

          /* although this range may not be valid it will have the best starttime */
          /* this is enough information to determine if job should be constrained by NodeSetDelay */

          if ((Delay > 0) &&
              (Range[rindex].STime - BestStartTime > (mulong)Delay))
            {
            /* range is too late, just let the job run wherever */

            MDB(5,fSCHED) MLog("INFO:     removing nodeset constraint for job %s\n",
              J->Name);

            bmset(&J->IFlags,mjifIgnoreNodeSets);

            sprintf(tmpLine,"Range is too late. Nodeset constraint removed. Job will be scheduled normally.");

            MMBAdd(&J->MessageBuffer,tmpLine,NULL,mmbtNONE,0,0,NULL);

            J->Req[0]->SetSelection = mrssNONE;
            J->Req[0]->SetBlock = FALSE;
            J->Req[0]->SetDelay = 0;
            
            *RangeValue = Range[0].STime;

            *MaxETime   = 0;

            *DoAllocation = TRUE;

            MUFree((char **)&NodeSetCounts);
            MUFree((char **)&NodeSetNLIndex);
            MUFree(&tmpNodeMap);

            for (sindex = 0;sindex < MMAX_NODESETLIST;sindex++)
              {
              MNLFree(&NodeSetNL[sindex]);
              }

            return(SUCCESS);
            }

          *BestIndex = rindex;

          *MaxETime  = 0;
          *RangeValue = Range[rindex].STime;
          *DoAllocation = FALSE;

          /* create allocation nodelist for reservation */

          /* allocate using NodeSetNL[sindex] and (ReqTC / (sindex + 1)) */

          findex  = 0;
          TotalTC = 0;

          for (sindex2 = 0;SetIndex[sindex2] != -1;sindex2++)
            {
            if (NodeSetCounts[sindex2] >= PerNodeSetTC)
              {
              nindex    = 0;
              NodeSetTC = 0;

              while (NodeSetTC < PerNodeSetTC)
                {
                if (MNLGetNodeAtIndex(&NodeSetNL[sindex2],nindex,NULL) == FAILURE)
                  {
                  /* should never reach this point */

                  break;
                  }

                if (NodeSetTC + MNLGetTCAtIndex(&NodeSetNL[sindex2],nindex) > PerNodeSetTC)
                  {
                  TC = PerNodeSetTC - NodeSetTC;
                  }
                else
                  {
                  TC = MNLGetTCAtIndex(&NodeSetNL[sindex2],nindex);
                  }

                if ((J->Req[0]->TasksPerNode > 0) && (TC % J->Req[0]->TasksPerNode != 0))
                  {
                  /* doesn't work */

                  break;
                  }

                NodeSetTC += TC;
                TotalTC   += TC;

                MNLSetNodeAtIndex(tmpMNodeList[0],findex,MNLReturnNodeAtIndex(&NodeSetNL[sindex2],nindex));
                MNLSetTCAtIndex(tmpMNodeList[0],findex,TC);

                findex++;
                nindex++;
                } /* END while (NodeSetTC < PerNodeSetTC) */
              }   /* END if (NodeSetCounts[sindex2] >= PerNodeSetTC) */

            if (TotalTC >= ReqTC)
              break;
            }     /* END for (sindex2) */

          if (TotalTC >= ReqTC)
            {
            *NC = findex;
            MNLTerminateAtIndex(tmpMNodeList[0],findex);
   
            bmset(&J->IFlags,mjifAllocatedNodeSets);
  
            MUFree((char **)&NodeSetCounts);
            MUFree((char **)&NodeSetNLIndex);
            MUFree(&tmpNodeMap);
  
            MDB(5,fSCHED) MLog("INFO:     spanning nodeset found for job '%s'\n",
              J->Name);

            for (sindex = 0;sindex < MMAX_NODESETLIST;sindex++)
              {
              MNLFree(&NodeSetNL[sindex]);
              }

            return(SUCCESS);
            }

          if (NodeSetPolicy == mrssOneOf)
            {
            /* for OneOf, only try a single nodeset */

            break;
            }

          if (J->Req[0]->NodeSetCount > 0)
            {
            break;
            }
          } /* END for (sindex = 0;SetIndex[sindex] != -1;sindex++) */
        }   /* END for (rindex) */

      /* could not find a nodeset but job doesn't care, remove nodeset constraint */

      MDB(5,fSCHED) MLog("INFO:     removing nodeset constraint for job %s\n",
        J->Name);

      bmset(&J->IFlags,mjifIgnoreNodeSets);

      sprintf(tmpLine,"Job cannot span evenly on nodeset constraints provided. Nodeset constraint removed. Job will be scheduled normally.");

      MMBAdd(&J->MessageBuffer,tmpLine,NULL,mmbtNONE,0,0,NULL);

      J->Req[0]->SetSelection = mrssNONE;
      J->Req[0]->SetBlock = FALSE;
      J->Req[0]->SetDelay = 0;

      *RangeValue = Range[0].STime;
      *BestIndex  = 0;

      *MaxETime   = 0;

      *DoAllocation = TRUE;

      MUFree((char **)&NodeSetCounts);
      MUFree((char **)&NodeSetNLIndex);
      MUFree(&tmpNodeMap);
 
      for (sindex = 0;sindex < MMAX_NODESETLIST;sindex++)
        {
        MNLFree(&NodeSetNL[sindex]);
        }

      return(SUCCESS);
      }     /* END case mnspSpanEvely */
     
      break;
    }  /* END switch (MSched.NodeSetPlus) */

  return(SUCCESS);
  }  /* END MJobGetRangeValue() */



 
/**
 * Return the list of moab environment variables for the job.
 *
 * Variables are to be put on the job if -E was requested at submission.
 *
 * MOAB_ACCOUNT      Account Name
 * MOAB_BATCH        Set if a batch job (non-interactive).
 * MOAB_CLASS        Class Name 
 * MOAB_DEPEND       Job dependency string.
 * MOAB_GROUP        Group Name
 * MOAB_JOBARRAYINDEX Job index into array. Handled in MJobLoadArray.
 * MOAB_JOBARRAYRANGE Job array count. Handled in MJobLoadArray.
 * MOAB_JOBID        Job ID. If submitted from the grid, grid jobid.
 * MOAB_JOBINDEX     Index in job array. - Not being used.
 * MOAB_JOBNAME      Job Name
 * MOAB_MACHINE      Name of the machine (ie. Destination RM) that the job is running on.
 * MOAB_NODECOUNT    Number of nodes allocated to job.
 * MOAB_NODELIST     Comma-separated list of nodes (listed singly with no ppn info).
 * MOAB_PARTITION    Partition name the job is running in. If grid job, cluster scheduler's name.
 * MOAB_PROCCOUNT    Number of processors allocated to job.
 * MOAB_QOS          QOS Name
 * MOAB_TASKMAP      Node list with procs per node listed. <nodename>.<procs>
 * MOAB_USER         User Name
 *
 * @param J
 * @param Delimiter
 * @param Buffer
 */

int MJobGetEnvVars(

  mjob_t    *J,         /* I */
  char       Delimiter, /* I */
  mstring_t *Buffer)    /* O */

  {
  int eindex;

  if ((J == NULL) || (Buffer == NULL))
    {
    return(FAILURE);
    }

  /* FORMAT <ENV_VAR>=<VAL>[<DELIMITER><ENV_VAR>=<VAL>...] */

  for (eindex = 1;eindex < mjenvLAST;eindex++)
    {
    switch (eindex)
      {
      case mjenvAccount:

        if (J->Credential.A != NULL)
          MStringAppendF(Buffer,"%s=%s%c",MJEnvVarAttr[eindex],J->Credential.A->Name,Delimiter);

        break;

      case mjenvBatch: 

        if (!bmisset(&J->Flags,mjfInteractive))
          MStringAppendF(Buffer,"%s=%c",MJEnvVarAttr[eindex],Delimiter);

        break;

      case mjenvClass:
        
        if (J->Credential.C != NULL)
          MStringAppendF(Buffer,"%s=%s%c",MJEnvVarAttr[eindex],J->Credential.C->Name,Delimiter);

        break;

      case mjenvDepend:

        if (J->Depend != NULL)
          {
          mstring_t   dependList("");

          /* use mrmtNONE since torque doesn't handle nested ','s. It displays them correctly
           * in qstat -f, but doesn't put them in the job's environment correctly. */

          MDependToString(&dependList,J->Depend,mrmtNONE);
          
          /* Append the following to the Buffer */
          *Buffer += MJEnvVarAttr[eindex];
          *Buffer += '=';
          *Buffer += dependList;
          *Buffer += Delimiter;
          }

        break;

      case mjenvGroup:

        if (J->Credential.G != NULL)
          MStringAppendF(Buffer,"%s=%s%c",MJEnvVarAttr[eindex],J->Credential.G->Name,Delimiter);

        break;

      case mjenvJobID:

        if (J->SystemJID != NULL) /* If grid job, SystemJID is grid jid. */
          MStringAppendF(Buffer,"%s=%s%c",MJEnvVarAttr[eindex],J->SystemJID,Delimiter);
        else if (!MUStrIsEmpty(J->Name))
          MStringAppendF(Buffer,"%s=%s%c",MJEnvVarAttr[eindex],J->Name,Delimiter);

        break;

#ifdef MNOT
      /* Not Currently being used. */
      case mjenvJobIndex:

        /* Job Array Index */

        if (J->Array != NULL)
          MStringAppendF(Buffer,"%s=%s%c",MJEnvVarAttr[eindex],J->Array->ArrayIndex,Delimiter);

        break;
#endif

      case mjenvJobName:

        if (J->AName != NULL)
          MStringAppendF(Buffer,"%s=%s%c",MJEnvVarAttr[eindex],J->AName,Delimiter);

        break;

      case mjenvMachine:

        if (J->DestinationRM != NULL)
          MStringAppendF(Buffer,"%s=%s%c",MJEnvVarAttr[eindex],J->DestinationRM->Name,Delimiter);

        break;

      case mjenvNodeCount:

        /* NodeCount will zero unless JOBNODEMATCHPOLICY is EXACTNODE 
         * Should NodeCount be displayed if ==0? */

        if (J->Req[0] != NULL)
          MStringAppendF(Buffer,"%s=%d%c",MJEnvVarAttr[eindex],J->Req[0]->NodeCount,Delimiter);

        break;

      case mjenvNodeList:

        /* FORMAT: <nodeid>[&<nodeid>...] */

        /* Use '&' as nodelist delimiter since torque isn't handling nested ','s. */
        
        if ((J->Req[0] != NULL) && 
            (!MNLIsEmpty(&J->Req[0]->NodeList)))
          {
          mstring_t tmpBuf(MMAX_LINE);

          MNLToMString(&J->Req[0]->NodeList,FALSE,"&",'\0',-1,&tmpBuf);

          MStringAppendF(Buffer,"%s=%s%c",MJEnvVarAttr[eindex],tmpBuf.c_str(),Delimiter);
          }

        break;

      case mjenvPartition:

        if (J->Grid != NULL) /* If grid job, partition is schedule name. */
          MStringAppendF(Buffer,"%s=%s%c",MJEnvVarAttr[eindex],MSched.Name,Delimiter);
        else 
          {
          mnode_t *N;

          int tmpPtIndex = 0;

          if (!bmisset(&J->Flags,mjfCoAlloc) && 
              (J->Req[0] != NULL) && 
              (MNLGetNodeAtIndex(&J->Req[0]->NodeList,0,&N) == SUCCESS))
            {
            tmpPtIndex = N->PtIndex; 
            }

          MStringAppendF(Buffer,"%s=%s%c",MJEnvVarAttr[eindex],MPar[tmpPtIndex].Name,Delimiter);
          }

        break;

      case mjenvProcCount:
        
        if (J->Req[0] != NULL)
          MStringAppendF(Buffer,"%s=%d%c",MJEnvVarAttr[eindex],J->Req[0]->TaskCount,Delimiter);

        break;

      case mjenvQos:

        if (J->Credential.Q != NULL)
          MStringAppendF(Buffer,"%s=%s%c",MJEnvVarAttr[eindex],J->Credential.Q->Name,Delimiter);

        break;

      case mjenvTaskMap:

        /* FORMAT: <nodeid>:<tc>[,<nodeid>:<tc>...] */
        
        /* Use '&' as nodelist delimiter since torque isn't handling nested ','s. */
        
        if ((J->Req[0] != NULL) && 
            (!MNLIsEmpty(&J->Req[0]->NodeList)))
          {
          mstring_t tmpBuf(MMAX_LINE);

          MNLToMString(&J->Req[0]->NodeList,TRUE,"&",'\0',-1,&tmpBuf);

          MStringAppendF(Buffer,"%s=%s%c",MJEnvVarAttr[eindex],tmpBuf.c_str(),Delimiter);
          }

        break;

      case mjenvUser:

        if (J->Credential.U != NULL)
          MStringAppendF(Buffer,"%s=%s%c",MJEnvVarAttr[eindex],J->Credential.U->Name,Delimiter);
        break;

      default:

        break;
      } /* END switch (eindex) */
    } /* END for (eindex = 0;eindex < mjenvLAST;eindex++) */

  /* Remove trailing delimiter */

  MStringStripFromIndex(Buffer,-1);

  return(SUCCESS);
  } /* END MJobGetEnvVars() */



/**
 * Report earliest time node can be available for use by job.
 *
 * @see MJobGetSNRange() - parent
 *
 * @param J (I)
 * @param RQ (I) [optional]
 * @param N (I)
 * @param DelayP (O)
 */

int MJobGetNodeStateDelay(

  const mjob_t  *J,
  const mreq_t  *RQ,
  const mnode_t *N,
  long          *DelayP)

  {
  if (DelayP != NULL)
    *DelayP = 0;

  if ((J == NULL) || (N == NULL) || (DelayP == NULL))
    {
    return(FAILURE);
    }

  /* enforce state based time constraints for all jobs and all rsvs where
     ignstate is not set - NOTE: only one range exists */

  if ((MPar[0].NodeDrainStateDelayTime != 0) &&
      MNODEISDRAINING(N))
    {
    *DelayP = MPar[0].NodeDrainStateDelayTime;
    }
  else if ((MPar[0].NodeDownStateDelayTime != 0) &&
           (MNODEISDRAINING(N) || (N->State == mnsDown)))
    {
    /* NOTE:  if drain time not explicitly set, apply down time to drained nodes */

    *DelayP = MPar[0].NodeDownStateDelayTime;
    }
  else if ((MPar[0].NodeBusyStateDelayTime != 0) &&
           (N->State == mnsBusy) &&
          ((N->EState == mnsIdle) || ((N->SubState != NULL) && (!strcasecmp(N->SubState,"cpabusy")))))
    {
    /* unexpected busy */

    /* NOTE:  for XT systems, cpa based busy states should be quickly retried */

    if ((N->SubState != NULL) && !strcasecmp(N->SubState,"cpabusy"))
      {
      /* if cpa busy detected, assume node will not be available for at least 2 seconds */

      *DelayP = 2;
      }
    else
      {
      if (MSched.IsPreemptTest == TRUE)
        *DelayP = 1;
      else
        *DelayP = MPar[0].NodeBusyStateDelayTime;
      }
    }
  else if (MPar[0].UntrackedResDelayTime != 0)
    {
    if (!strcmp(MPar[N->PtIndex].Name,"SHARED") &&
       (J->Req[1] != NULL) &&
       (!MSNLIsEmpty(&J->Req[1]->DRes.GenericRes)))
      {
      int gindex;

      /* Apply delay time to node only if job requests more than are available
       * and if there are gres that are unaccounted for. */

      for (gindex = 1;gindex < MSched.M[mxoxGRes];gindex++)
        {
        if (MGRes.Name[gindex][0] == '\0')
          break;

        if ((gindex < MSched.M[mxoxGRes]) &&
            (MSNLGetIndexCount(&J->Req[1]->DRes.GenericRes,gindex) > MSNLGetIndexCount(&N->ARes.GenericRes,gindex)) &&
            (MSNLGetIndexCount(&N->ARes.GenericRes,gindex) < MSNLGetIndexCount(&N->CRes.GenericRes,gindex) - MSNLGetIndexCount(&N->DRes.GenericRes,gindex)))
          {
          *DelayP = MPar[0].UntrackedResDelayTime;

          break;
          }
        }  /* END for (gindex) */
      }    /* ENDif (!strcmp(MPar[N->PtIndex].Name,"SHARED") && ...) */ 
    }      /* END else if (MPar[0].UntrackedResDelayTime != 0) */

  if (((MSched.GN == NULL) || (N != MSched.GN)) &&
      (MSched.ProvRM != NULL) &&
      ((J->Credential.Q == NULL) || !bmisset(&J->Credential.Q->Flags,mqfProvision)) &&
      ((J->VMUsagePolicy != mvmupNONE) && (J->VMUsagePolicy != mvmupVMCreate)) && 
      (J->Req[0]->Opsys != 0) && 
      (N->ActiveOS != J->Req[0]->Opsys))
    {
    mjob_t *ProvJ;

    /* node can be provisioned to required OS but does not currently
       have required OS active - add default provisioning time */

    /* NOTE:  Delay by 2X to 'push' jobs which require provisioning towards
              nodes which already have provisioning system jobs queued or
              actively provisioning */

    if (MNodeGetSysJob(N,msjtOSProvision,MBNOTSET,&ProvJ) == TRUE)
      {
      if ((ProvJ->System != NULL) && (ProvJ->System->ICPVar1 != J->Req[0]->Opsys))
        {
        /* provisioning to different OS - avoid node!!! */

        *DelayP = MAX(*DelayP,(long)MSched.DefProvDuration << 2);
        }
      else if (MJOBISACTIVE(ProvJ) == TRUE)
        {
        /* node already actively provisioning to correct OS - determine
           provisioning time remaining */

        /* NOTE:  if provisioning job is still running, assume it will run for
                  one minute more (bad assumption?) */

        *DelayP = MAX(
          *DelayP,
          (long)MAX(MCONST_MINUTELEN,ProvJ->WCLimit - (MSched.Time - ProvJ->StartTime)));
        }
      else
        {
        /* node provisioning scheduled but not started - favor node */

        *DelayP = MAX(*DelayP,(long)MSched.DefProvDuration);
        }
      }
    else
      {
      *DelayP = MAX(*DelayP,(long)MSched.DefProvDuration << 1);
      }

    *DelayP = MAX(*DelayP,(long)MSched.DefProvDuration);
    }

  if ((RQ != NULL) && (MSched.ResOvercommitSpecified[0] == TRUE))
    {
    int ATC;

    /* DOUG:  Look at me!!! */

    if ((MNodeGetAvailTC(N,J,RQ,TRUE,&ATC) == FAILURE) || (ATC < 1))
      {
      mrsv_t *JR;

      if (*DelayP > 0)
        MNodeGetRsv(N,TRUE,MSched.Time + *DelayP,&JR);
      else
        MNodeGetRsv(N,TRUE,MMAX_TIME,&JR);

      if (JR != NULL)
        *DelayP = MAX(*DelayP,(long)(JR->EndTime - MSched.Time));
      }
    }

  return(SUCCESS);
  }  /* END MJobGetNodeStateDelay() */





/**
 * Get the number of partions the job actually requested.
 *
 * @param J (I)
 */

int MJobGetSpecPALCount(

  mjob_t *J) /*I*/

  {
  int ParCount = 0;
  int pindex;

  if (J == NULL)
    return -1;

  for (pindex = 1;pindex < MMAX_PAR;pindex++)
    {
    if (MPar[pindex].Name[0] == '\0')
      break;

    if (MPar[pindex].Name[0] == '\1')
      continue;

    if (!bmisset(&J->SpecPAL,pindex))
      continue;

    ParCount++;
    }    /* END for (pindex) */

  return(ParCount);
  } /* END int MJobGetSpecPALCount() */




int MJobGetDynamicAttr(

  mjob_t              *J,      /* I (modified) */
  enum MJobAttrEnum    AIndex, /* I */
  void               **Value,  /* I */
  enum MDataFormatEnum Format) /* I */

  {
  if ((J == NULL) || 
      (Value == NULL) ||
      (AIndex == mjaNONE))
    return(FAILURE);

  switch (Format)
    {
    case mdfString:

      {
      switch (AIndex)
        {
        /* Put all accepted char * attrs here */

        case mjaTrigNamespace:
        case mjaCancelExitCodes:

          return(MUDynamicAttrGet(AIndex,J->DynAttrs,Value,Format));

          break;

        default:
          /* wrong aindex or type for aindex */

          return(FAILURE);

          break;
        }  /* END switch (AIndex) */
      }  /* END BLOCK mdfString */

      break;

    case mdfInt:

      {
      switch (AIndex)
        {
        default:
          /* wrong aindex or type for aindex */

          return(FAILURE);

          break;
        }  /* END switch (AIndex) */
      }  /* END BLOCK mdfInt */

      break;

    case mdfDouble:

      {
      switch (AIndex)
        {
        default:
          /* wrong aindex or type for aindex */

          return(FAILURE);

          break;
        }  /* END switch (AIndex) */
      }  /* END BLOCK mdfDouble */

      break;

    case mdfLong:

      {
      switch (AIndex)
        {
        default:
          /* wrong aindex or type for aindex */

          return(FAILURE);

          break;
        }  /* END switch (AIndex) */
      }  /* END BLOCK mdfLong */

      break;

    default:

      /* Type not supported */

      return(FAILURE);

      /* NOTREACHED */

      break;
    }  /* END switch (Format) */

  return(SUCCESS);
  }  /* END MJobGetDynamicAttr() */



/**
 * Returns TRUE if the job belongs to a workflow and there is at
 *  least one job in the workflow that is active or completed.
 *
 * @param J [I] - Job to check the workflow of.
 */

mbool_t MJobWorkflowHasStarted(

  mjob_t *J)

  {
  mvc_t *VC = NULL;
  mln_t *tmpL = NULL;
  mjob_t *tmpJ = NULL;

  if (J == NULL)
    return(FALSE);

  if (J->StartTime > 0)
    return(TRUE);

  for (tmpL = J->ParentVCs;tmpL != NULL;tmpL = tmpL->Next)
    {
    VC = (mvc_t *)tmpL->Ptr;

    if (VC == NULL)
      continue;

    if (bmisset(&VC->Flags,mvcfWorkflow))
      break;

    VC = NULL;
    }

  if (VC == NULL)
    return(FALSE);

  if (bmisset(&VC->Flags,mvcfHasStarted) == TRUE)
    return(TRUE);

  /* We have a workflow VC */

  for (tmpL = VC->Jobs;tmpL != NULL;tmpL = tmpL->Next)
    {
    tmpJ = (mjob_t *)tmpL->Ptr;

    if (tmpJ == NULL)
      continue;

    if (tmpJ->StartTime > 0) 
      return(TRUE);
    }

  return(FALSE);
  }  /* END MJobWorkflowHasStarted() */


/**
 *  Helper function for VC stuff when job is completing.
 *   Removes the job from VCs, also does some workflow checks.
 *
 * @see MJobRemove() - parent
 *
 * @param J [I] - Remove the parent VCs of this job
 */

int __MJobRemoveVCs(
  mjob_t *J)

  {
  mvc_t *VC = NULL;
  mln_t *tmpL = NULL;
  char   VCName[MMAX_NAME];
  mbool_t WorkflowStarted = FALSE;

  if (J == NULL)
    return(FAILURE);

  if (J->ParentVCs == NULL)
    return(SUCCESS);

  VCName[0] = '\0';

  for (tmpL = J->ParentVCs;tmpL != NULL;tmpL = tmpL->Next)
    {
    VC = (mvc_t *)tmpL->Ptr;

    if (VC == NULL)
      continue;

    if (bmisset(&VC->Flags,mvcfWorkflow))
      {
      MUStrCpy(VCName,VC->Name,sizeof(VCName));

      break;
      }
    }

  WorkflowStarted = MJobWorkflowHasStarted(J);

  MVCRemoveObjectParentVCs((void *)J,mxoJob);

  if ((WorkflowStarted == FALSE) &&
      (VCName[0] != '\0') &&
      (MVCFind(VCName,&VC) == SUCCESS))
    {
    /* Workflow VC still exists */

    /* This workflow VC has not yet started, and a job was removed out of it.
        Remove whole workflow */

    MVCRemove(&VC,TRUE,TRUE,NULL);
    }  /* END if ((VCName[0] != '\0') && (MVCFind(VCName,&VC) == SUCCESS)) */

  return(SUCCESS);
  }  /* END __MJobRemoveVCs() */



/**
 * Sees if there is a specific nodeset that must be used for a job.
 *   Requested node set, feature, VC nodeset, etc.
 *
 * @param J   [I] - The job to find a node set for
 * @param SetList [I] - Available nodesets (used for job features, not VC)
 * @param Ptr [O] - The name of the nodeset to use (if any)
 */

int MJobGetNodeSet(

  const mjob_t  *J,
  char         **SetList,
  char         **Ptr)

  {
  char *tmpPtr = NULL;
  int index;  /* iterates through job's FBM */
  int sindex; /* iterates through SetList */

  if ((J == NULL) || (Ptr == NULL))
    return(FAILURE);

  *Ptr = NULL;

  /* Check for requested nodeset/feature on the job */

  if ((SetList != NULL) && (SetList[0] != NULL) && (J->Req[0] != NULL))
    {
    for (index = 0;index < MMAX_ATTR;index++)
      {
      /* Go through each feature in job FBM */

      if (MAList[meNFeature][index][0] == '\0')
        break;

      if (!bmisset(&J->Req[0]->ReqFBM,index))
        continue;

      /* found a feature, see if it is in SetList */

      for (sindex = 0;sindex < MMAX_ATTR;sindex++)
        {
        if ((SetList[sindex] == NULL) || (SetList[sindex][0] == '\0'))
          break;

        if (strcmp(MAList[meNFeature][index],SetList[sindex]) != 0)
          continue;

        /* Found a feature that is part of the set list */

        *Ptr = MAList[meNFeature][index];

        return(SUCCESS);
        }  /* END for (sindex) */
      }  /* END for (index) */
    }  /* END if ((SetList != NULL) && (SetList[0] != NULL)) */

  /* Didn't find node set on the job, check VCs */

  if (MVCGetNodeSet(J,&tmpPtr) == SUCCESS)
    {
    int FIndex = 0;

    /* tmpPtr is allocated (came from a dynamic attr) 
        Need to point to MAList so that we can free it here
        (don't expect to free later) */

    FIndex = MUMAGetIndex(meNFeature,tmpPtr,mVerify);

    MUFree(&tmpPtr);

    if (FIndex <= 0)
      return(FAILURE);

    *Ptr = MAList[meNFeature][FIndex];

    return(SUCCESS);
    }

  return(FAILURE);
  }  /* END MJobGetNodeSet() */
/* END MJobGet.c */
