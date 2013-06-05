/* HEADER */

/**
 * @file MJobEval.c
 *
 * Contains Job Evaluate if job is complete
 */



#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 * Evaluate whether the job is complete and can run in at least one partition
 *
 * @see MRMJobPostLoad() - parent - evaluate newly loaded RM job
 * @see MTraceLoadWorkload() - parent - evaluate newly submited simulation job
 * @see MUIJobSubmit() - parent - evaluate newly submitted msub job
 * @see MJobCheckParAccess() - child - verify partition access
 *
 * NOTE:  MJobEval() should only be called once per job at job load time.
 *
 * @param J         (I)  job to evaluate
 * @param SrcR      (I)  source RM (optional)
 * @param DstR      (I)  destination RM (optional)
 * @param EMsg      (I)  minsize=MMAX_LINE (optional)
 * @param RejReason (O) (optional)
 *
 * NOTE: checks J->Cred.U
 * NOTE: checks J->Cred.*->NoSubmit
 * NOTE: checks J->Request.NC vs MSched.M[mxoNode]
 * NOTE: checks J->Request.TC vs MSched.M[mxoNode] * MPar[0].MaxPPN
 * NOTE: checks J->PAL vs P->Index
 *
 * NOTE: some failures may trigger if Moab is started and many/most compute 
 *       resources are not reporting in 
 */

int MJobEval(

  mjob_t *J,
  mrm_t  *SrcR,
  mrm_t  *DstR,
  char   *EMsg,
  enum MAllocRejEnum *RejReason)

  {
  mreq_t *RQ;
  mrm_t  *R;

  mbool_t FoundValidCred;

  int     rmindex;
  int     pindex;
  int     oindex;
  int     cindex;

  enum MAllocRejEnum RIndex[MMAX_PAR];

  char **ACList;  /* rm authorized cred list */
  char  *JCName;  /* job cred name */

  char   tEMsg[MMAX_LINE];
 
  const enum MXMLOTypeEnum OList[] = { 
    mxoUser, mxoGroup, mxoAcct, mxoClass, mxoQOS, mxoNONE };

  const char *FName = "MJobEval";

  MDB(7,fSTRUCT) MLog("%s(%s,%s,%s,EMsg)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (SrcR != NULL) ? SrcR->Name : "NULL",
    (DstR != NULL) ? DstR->Name : "NULL");
 
  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (J == NULL)
    {
    return(FAILURE);
    }

  if (J->Credential.U == NULL)
    {
    if (EMsg != NULL)
      strcpy(EMsg,"no user specified");

    if (RejReason != NULL)
      *RejReason = marUser;

    return(FAILURE);
    }
  else if (J->Credential.U->NoSubmit == TRUE)
    {
    if (EMsg != NULL)
      strcpy(EMsg,"user not allowed to submit jobs");

    if (RejReason != NULL)
      *RejReason = marUser;

    return(FAILURE);
    }

  if (J->Credential.G == NULL)
    {
    /* group is required, why? */

    if (EMsg != NULL)
      strcpy(EMsg,"group not specified");

    if (RejReason != NULL)
      *RejReason = marGroup;

    return(FAILURE);
    }
  else if (J->Credential.G->NoSubmit == TRUE)
    {
    if (EMsg != NULL)
      strcpy(EMsg,"group not allowed to submit jobs");

    if (RejReason != NULL)
      *RejReason = marNoSubmit;

    return(FAILURE);
    }

  if ((J->Credential.A != NULL) && (J->Credential.A->NoSubmit == TRUE))
    {
    if (EMsg != NULL)
      strcpy(EMsg,"account not allowed to submit jobs");

    if (RejReason != NULL)
      *RejReason = marNoSubmit;

    return(FAILURE);
    }

  if ((J->Credential.Q != NULL) && (J->Credential.Q->NoSubmit == TRUE))
    {
    if (EMsg != NULL)
      strcpy(EMsg,"QOS not allowed to submit jobs");

    if (RejReason != NULL)
      *RejReason = marNoSubmit;

    return(FAILURE);
    }

  if ((J->Credential.C != NULL) && (J->Credential.C->NoSubmit == TRUE))
    {
    if (EMsg != NULL)
      snprintf(EMsg,MMAX_LINE,"class '%s' not allowed to submit jobs",
          J->Credential.C->Name);

    if (RejReason != NULL)
      *RejReason = marNoSubmit;

    return(FAILURE);
    }

  if (J->Request.NC > MSched.M[mxoNode])
    {
    if (EMsg != NULL)
      strcpy(EMsg,"too many nodes requested");

    if (RejReason != NULL)
      *RejReason = marNodeCount;
 
    return(FAILURE);
    }

  if (MPar[0].MaxPPN > 0)
    {
    if (J->Request.TC / MPar[0].MaxPPN > MSched.M[mxoNode])
      {
      if (EMsg != NULL)
        strcpy(EMsg,"too many tasks requested");

      if (RejReason != NULL)
        *RejReason = marTaskCount;

      return(FAILURE);
      }
    }

  if ((DstR != NULL) && (J->Req[0]->DRes.Procs > 0))
    {
    /* check that there is a partition associated with with the RM
     * that has enough procs to run the job. */

    int      pindex;
    mpar_t  *tmpP         = NULL;
    mbool_t  PCSatisfied  = FALSE;
    mbool_t  PPNSatisfied = FALSE;
    mbool_t  ProcsLocated = FALSE;

    for (pindex = 0;pindex < MMAX_PAR;pindex++)
      {
      if (MPar[pindex].Name[0] == '\1')
        continue;

      if (MPar[pindex].Name[0] == '\0')
        break;

      tmpP = &MPar[pindex];

      if (tmpP->RM == DstR) 
        {
        /* partition provides resources to target RM */

        if (tmpP->CRes.Procs > 0)
          {
          ProcsLocated = TRUE;
          }

        if (J->Req[0]->DRes.Procs <= tmpP->CRes.Procs)
          {
          /* partition associated with RM has enough procs */

          PCSatisfied = TRUE;
          }
 
        if ((tmpP->MaxPPN > 0) &&
            (MAX(1,J->Req[0]->TasksPerNode) * J->Req[0]->DRes.Procs <= tmpP->MaxPPN))
          {
          /* partition associated with RM that has enough procs per node */

          PPNSatisfied = TRUE;

          break;
          }
        }
      }    /* END for (pindex = 0;MPar[pindex] != NULL;pindex++) */
     
    if (PCSatisfied == FALSE)
      {
      /* insufficient procs in any partition */

      if (EMsg != NULL)
        {
        if (ProcsLocated == TRUE)
          strcpy(EMsg,"too many procs requested");
        else
          strcpy(EMsg,"no procs located");
        }

      if (RejReason != NULL)
        *RejReason = marTaskCount; /* Closest thing to ppn */

      return(FAILURE);
      }
 
    if (PPNSatisfied == FALSE)
      {
      /* insufficient procs per node */

      if (EMsg != NULL)
        strcpy(EMsg,"too many procs per node requested");

      if (RejReason != NULL)
        *RejReason = marTaskCount; /* Closest thing to ppn */

      return(FAILURE);
      }
    }    /* END if ((DstR != NULL) && (J->Req[0]->DRes.Procs > 0)) */

  /* check ability to submit to src/dst rm */
 
  for (rmindex = 0;rmindex < 2;rmindex++)
    {
    if (rmindex == 0)
      R = SrcR;
    else
      R = DstR;

    if (R == NULL)
      continue;

    if ((R == SrcR) && (!bmisset(&R->Flags,mrmfClient)))
      continue;

    /* check all individual credentials */

    FoundValidCred = FALSE;
       
    for (oindex = 0;OList[oindex] != mxoNONE;oindex++)
      {
      ACList = NULL;
      JCName = NULL;

      switch (OList[oindex])
        { 
        case mxoUser:     

          ACList = R->AuthU;
          if (J->Credential.U != NULL)
            JCName = J->Credential.U->Name; 
          break;

        case mxoGroup:

          ACList = R->AuthG;
          if (J->Credential.G != NULL)
            JCName = J->Credential.G->Name; 
          break;

        case mxoAcct:

          ACList = R->AuthA;
          if (J->Credential.A != NULL)
            JCName = J->Credential.A->Name; 
          break;

        case mxoQOS:

          ACList = R->AuthQ;
          if (J->Credential.Q != NULL)
            JCName = J->Credential.Q->Name; 
          break;

        case mxoClass:

          ACList = R->AuthC;
          if (J->Credential.C != NULL)
            JCName = J->Credential.C->Name; 
          break;

        default:

          /* NO-OP */

          break;
        }  /* END switch (OList[oindex]) */

      if (ACList == NULL)
        {
        /* no cred of specified type required */

        continue;
        }

      if ((ACList != NULL) && (JCName == NULL))
        {
        /* job missing required cred */

        if (EMsg != NULL)
          {
          sprintf(EMsg,"job missing required %s cred to submit job",
            MXO[OList[oindex]]);
          }

        if (RejReason != NULL)
          *RejReason = marACL;

        return(FAILURE);
        }

      for (cindex = 0;cindex < MMAX_CREDS;cindex++)
        {
        if (ACList[cindex] == NULL)
          break;

        if (!strcmp(ACList[cindex],JCName))
          {
          FoundValidCred = TRUE;

          break;
          }
        }  /* END for (cindex) */

      if ((ACList[cindex] == NULL) || 
          (cindex >= MMAX_CREDS))
        {
        if (EMsg != NULL)
          {
          sprintf(EMsg,"%s %s not authorized to submit job",
            MXO[OList[oindex]],
            JCName);
          }

        if (RejReason != NULL)
          *RejReason = marACL;

        return(FAILURE);
        }
  
      if (FoundValidCred == TRUE)
        {
        /* NOTE:  only one valid cred required */

        break;
        }
      }    /* END for (oindex) */
    }      /* END for (rmindex) */

  /* check all destination partitions, only submit job if at least one destination *
     partition is acceptable */

  if (bmisclear(&J->PAL))
    {
    /* job has empty partition access list */

    if (EMsg != NULL)
      {
      sprintf(EMsg,"job cannot access any partition");
      }

    if (RejReason != NULL)
      *RejReason = marPartition;

    return(FAILURE);
    }

  /* NOTE:  do not validate partition access if only one partition exists */

  if (MPar[2].Name[0] != '\0')
    {
    mbool_t ParFound = FALSE;
    mbool_t InteractiveParFound = FALSE;
    int ParFoundIndex = 0;
    int NonCompParFoundIndex = 0;
    int ComputeParCount = 0;    /* # of compute pars accessible */
    int NonComputeParCount = 0; /* # of accessible pars w/o compute nodes */
    mclass_t *ParDefaultClass[MMAX_PAR]; /* keep an array of the default class for each partition if job does not have a class */
    int ParDefaultClassCount = 0;        /* number of entries in the partition default class array */

    memset(RIndex,0,sizeof(RIndex));

    if (EMsg != NULL)
      EMsg[0] = '\0';

    for (pindex = 1;pindex < MMAX_PAR;pindex++)
      {
      if (MPar[pindex].Name[0] == '\1')
        continue;

      if (MPar[pindex].Name[0] == '\0')
        break;

      if (MPar[pindex].RM != NULL)
        {
        if (bmisset(&MPar[pindex].RM->Flags,mrmfClient))
          {
          /* don't examine client resource manager partitions */

          continue;
          }
        else if (J->SubmitRM != NULL)
          {
          /* NOTE:  if grid job, do not allow return to source */

          /* NOTE:  if grid job and routing via MS3JobSubmit(), J->SRM points 
                    to the internal RM.  Consequently, cannot check if attempt 
                    is made to return to source (FIXME) */
          }
        }    /* END if (MPar[pindex].RM != NULL) */

      /* If the current partition RM is not SLURM or TORQUE and the job */
      /* is interactive, throw an error  */

      if ((J != NULL) &&
          (bmisset(&J->SpecFlags,mjfInteractive) ||
           bmisset(&J->Flags,mjfInteractive)))
        {
        if ((MPar[pindex].RM == NULL) ||
            ((MPar[pindex].RM->SubType != mrmstSLURM) &&
             (MPar[pindex].RM->Type != mrmtPBS) &&
             (MPar[pindex].RM->SubType != mrmstXT3) &&
             (MPar[pindex].RM->SubType != mrmstXT4) &&
             (J->SubmitRM != J->DestinationRM)))
          {
          /* Job is interactive, no valid SLURM or TORQUE partition found */

          MJobDisableParAccess(J,&MPar[pindex]);

          continue;
          }
        else
          {
          InteractiveParFound = TRUE;
          }
        }

      if (bmisset(&J->PAL,MPar[pindex].Index))
        {
        MDB(7,fSTRUCT) MLog("INFO:    %s's class: '%s'\n",
          J->Name,
          (J->Credential.C == NULL) ? "NULL" : J->Credential.C->Name);

        if (MJobCheckParAccess(
              J,
              &MPar[pindex],
              FALSE,
              &RIndex[pindex],   /* O */
              tEMsg) == SUCCESS) /* O */
          {
          ParFound = TRUE;

          MDB(7,fSTRUCT) MLog("INFO:    Partition found, configured nodes: %d\n",
            MPar[pindex].ConfigNodes);

          /* If the job does not have a class assigned, then continue looping
           * to see how many partitions the job can access */

          if ((J->Credential.C == NULL) || (J->Credential.C->FType == mqftRouting))
            {
            /* The partition must have compute nodes or be dynamic to qualify 
               as a partition that the job can access without a class. */

            if (MPar[pindex].ConfigNodes > 0)
              {

              /* if the job has no class, but the partition has a default class then save the class
                 so we can check later to see if all the partitions have the same default class */

              if ((J->Credential.C == NULL) && (MPar[pindex].RM != NULL)) 
                {
                ParDefaultClass[ParDefaultClassCount++] = MPar[pindex].RM->DefaultC;
                }
              else
                {
                ParDefaultClass[ParDefaultClassCount++] = NULL;
                }

              ParFoundIndex = pindex;
              ComputeParCount++;
              }
            else
              {
              NonCompParFoundIndex = pindex;
              NonComputeParCount++;
              }

            continue;
            }

          /* The job already has a class so since we have found at least one partition
           * then we can quit looking for partitions for this job. */

          break;
          }

        if ((J->Credential.C != NULL) && (J->Credential.C->FType == mqftRouting))
          {
          ParFound = TRUE;

          /* FIXME: routing queues might be specific to partitions, there needs to be
                    a way to verify whether a routing queue only has access to a specific
                    partition.  For now, job can go to any partition. */

          if (MPar[pindex].ConfigNodes > 0)
            {
            ParFoundIndex = pindex;
            ComputeParCount++;
            }
          else
            {
            NonCompParFoundIndex = pindex;
            NonComputeParCount++;
            }

          break;
          }

        MDB(7,fSTRUCT) MLog("INFO:    error from trying partition: %s\n",
              tEMsg);

        if ((EMsg != NULL) && (EMsg[0] == '\0'))
          {
          /* report error message only for first partition which is potential 
             but does not match */

          if ((J->Request.TC > 0) && (J->Credential.C != NULL) && (MPar[pindex].ConfigNodes > 0))
            {
            /* empty partition cannot support compute job */

            /* do not report this partition as possible failure reason */

            /* NO-OP */
            }
          else
            {
            MUStrCpy(EMsg,tEMsg,MMAX_LINE);
            }
          }

        continue;
        }  /* END if (bmisset(&)) */
      }    /* END for (pindex) */

    MDB(7,fSTRUCT) MLog("INFO:    ParFound=%s\n",
      MBool[ParFound]);

    if (ParFound == FALSE)
      {
      if (EMsg != NULL) 
        {
        /* Job is interactive, no valid SLURM or TORQUE partition found */

        if ((bmisset(&J->SpecFlags,mjfInteractive) ||
            bmisset(&J->Flags,mjfInteractive)) &&
            (InteractiveParFound == FALSE))
          {
          snprintf(EMsg,MMAX_LINE,"interactive msub jobs only supported with SLURM or TORQUE");
          }

        if ((EMsg[0] == '\0') || !strcmp(EMsg,"par access"))
          {
          char  tEMsg[MMAX_LINE];

          char *BPtr;
          int   BSpace;

          MUSNInit(&BPtr,&BSpace,tEMsg,sizeof(tEMsg));

          for (pindex = 0;pindex < MSched.M[mxoPar];pindex++)
            {
            if (MPar[pindex].Name[0] == '\0')
              break;

            if (MPar[pindex].Name[0] == '\1')
              continue;

            if (RIndex[pindex] == marNONE)
              continue;

            MUSNPrintF(&BPtr,&BSpace,"%s%s:%s",
              (tEMsg[0] != '\0') ? "," : "",
              MPar[pindex].Name,
              MAllocRejType[RIndex[pindex]]);
            }  /* END for (pindex) */

          snprintf(EMsg,MMAX_LINE,"no valid destinations available for job - %s",
            (tEMsg[0] != '\0') ? tEMsg : "N/A");
          }    /* END if ((EMsg != NULL) && ...) */
        }      /* END if (EMsg != NULL) */

      MDB(7,fSTRUCT) MLog("INFO:    %s returning FAILURE, par=%s, %s\n",
        FName,
        (pindex >= MSched.M[mxoPar]) ? "NULL" : MPar[pindex].Name,
        (pindex >= MSched.M[mxoPar]) ? "NONE" : MAllocRejType[RIndex[pindex]]);

      if (RejReason != NULL)
        *RejReason = marPartition;

      return(FAILURE);
      }  /* END if (ParFound == FALSE) */
    else
      {
      mbool_t SameDefaultClass = FALSE;

      /* Clear the FairShare fail flag in case it was set while checking partitions */
      bmunset(&J->IFlags,mjifFairShareFail);

      /* if more than one compute partition was found and the class has not been set for the job
         then check to see if all the compute partitions have the same default class */

      if ((J->Credential.C == NULL) && (ParDefaultClassCount > 1))
        {
        int index;
        mclass_t *tmpClass = ParDefaultClass[0]; 

        SameDefaultClass = TRUE;

        for (index = 1; index < ParDefaultClassCount; index++)
          {
          if (ParDefaultClass[index] != tmpClass)
            {
            SameDefaultClass = FALSE;
            break;
            }
          }
        }

      /* if the job does not have a class and it can only access one partition or all partitions have the same default
       * class then we can safely add the default class to the job. */

      if (((ComputeParCount == 1) || (SameDefaultClass == TRUE)) && (J->Credential.C == NULL))
        {
        /* can't set the job's class to the default rm's class if it is a routing
           queue because the routing queue is disabled */

        if (((R = MPar[ParFoundIndex].RM) != NULL) && 
            (R->DefaultC != NULL) &&
            (R->DefaultC->FType != mqftRouting))
          {
          MDB(7,fSTRUCT) MLog("INFO:    setting class for job '%s' to class '%s'\n",
            J->Name,
            (R->DefaultC != NULL) ? R->DefaultC->Name : "NULL");

          J->Credential.C = R->DefaultC;

          bmset(&J->IFlags,mjifClassSetByDefault);
          }
        } /* END if (ComputeParCount == 1) */
      else if ((NonComputeParCount == 1) && 
               (ComputeParCount == 0) &&
               (J->Credential.C == NULL))
        {
        /* Set default class to non computing partition if there is only 
           one and there are no computing partitions available */
      
        if ((R = MPar[NonCompParFoundIndex].RM) != NULL)
          {
          MDB(7,fSTRUCT) MLog("INFO:    setting class for job '%s' to class '%s'\n",
              J->Name,
              (R->DefaultC != NULL) ? R->DefaultC->Name : "NULL");
        
          J->Credential.C = R->DefaultC;
          }
        } /* END else if (NonComputeParCount == 1) ... */
      }  /* END else (ParFound == TRUE) */
    }    /* END if (MPar[2].Name[0] != '\0') */

  RQ = J->Req[0];

  if (RQ == NULL)
    {
    if (EMsg != NULL)
      strcpy(EMsg,"job is corrupt - no req");

    if (RejReason != NULL)
      *RejReason = marCorruption;

    return(FAILURE);
    }

  if ((RQ->NodeCount > 0) && (RQ->TaskCount > 0) && (RQ->TasksPerNode > 1))
    {
    if (RQ->TasksPerNode * RQ->NodeCount != RQ->TaskCount)
      {
      MDB(2,fSTRUCT) MLog("INFO:     job '%s' has invalid task layout (TPN:%d * N:%d != T:%d)\n",
        J->Name,
        RQ->TasksPerNode,
        RQ->NodeCount,
        RQ->TaskCount);

      if (EMsg != NULL)
        strcpy(EMsg,"job has invalid task layout");

      if (RejReason != NULL)
        *RejReason = marTaskCount;

      return(FAILURE);
      }
    }    /* END if ((RQ->NodeCount > 0) && ...) */

  /* NOTE: if user submitted from $HOME, replace E.IWD with $HOME unless interactive job */
  if (!bmisset(&J->Flags,mjfInteractive))
  MJobExpandPaths(J);

#ifdef __MNOT
  /* Dave requested a configuration parameter and a test to check to see if a job will ever start
   * and if not return a failure on the job eval. This is an expensive check so we only
   * do it if the configuration parameter is enabled. */

  if ((MSched.EnableJobEvalCheckStart == TRUE) && 
       !bmisset(&J->IFlags,mjifJobEvalCheckStart) &&
        bmisset(&J->IFlags,mjifSubmitEval))
    {
    long    StartTime = 0;

    bmset(&J->IFlags,mjifJobEvalCheckStart); /* only do this check once since this routine can be called several times. */

    if (MJobGetEStartTime(
           J,             /* I job                                */
           NULL,          /* I/O (optional) constraining/best partition */
           NULL,          /* O Nodecount   (optional) number of nodes located */
           NULL,          /* O TaskCount   (optional) tasks located           */
           NULL,          /* O mnodelist_t (optional) list of best nodes to execute job */
           &StartTime,    /* I/O earliest start time possible     */
           TRUE,          /* DoRemote */
           FALSE,         /* Check Partition Availability */
           NULL) == FAILURE)
      {
      if (EMsg != NULL)
        strcpy(EMsg,"job cannot be started");

      if (RejReason != NULL)
        *RejReason = marTime;

      return(FAILURE);
      }
    } 
#endif /* __MNOT */

  if (RejReason != NULL)
    *RejReason = marNONE;

  return(SUCCESS);
  }  /* END MJobEval() */
/* END MJobEval.c */
