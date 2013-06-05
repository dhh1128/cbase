/* HEADER */

      
/**
 * @file MRsvCheck.c
 *
 * Contains: Functions for Rsv Checking
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 *
 *
 * @param R (I) existing reservation
 * @param ReqR (I) requesting reservation
 * @param Overlap (I) duration of overlap
 * @param Same (O) reservations are identical
 * @param Affinity (O) affinity of access
 */

int MRsvCheckRAccess(

  mrsv_t  *R,        /* I  existing reservation */
  mrsv_t  *ReqR,     /* I  requesting reservation */
  long     Overlap,  /* I  duration of overlap */
  mbool_t *Same,     /* O  reservations are identical */
  char    *Affinity) /* O  affinity of access */

  {
  mbool_t InclusiveReservation=FALSE;

  /* R:  existing job or user reservation */

  /* attributes of a grid reservation */

  /*   container reservation          */
  /*   owned by user                  */
  /*   cannot be moved in time        */
  /*   queries should be allowed to request grid rsv <X> or free resources */

  const char *FName = "MRsvCheckRAccess";

  MDBO(8,R,fSTRUCT) MLog("%s(%s,%s,%ld,Same,Affinity)\n",
    FName,
    (R != NULL) ? R->Name : "NULL",
    (ReqR != NULL) ? ReqR->Name : "NULL",
    Overlap);

  if ((R == NULL) || (ReqR == NULL))
    {
    return(FAILURE);
    }

  if ((R->ExpireTime > 0) && (R->ExpireTime != MMAX_TIME))
    {
    return(FALSE);
    }

  if (Same != NULL)
    *Same = FALSE;

  if (bmisset(&R->Flags,mrfExcludeAll) ||
      bmisset(&ReqR->Flags,mrfExcludeAll))
    {
    /* exclusive reservations */

    MDBO(8,R,fSTRUCT) MLog("INFO:     rsv access is exclusive\n");

    return(FALSE);
    }

  if ((bmisset(&R->Flags,mrfExcludeJobs) && (ReqR->Type == mrtJob)) ||
      (bmisset(&ReqR->Flags,mrfExcludeJobs) && (R->Type == mrtJob)))
    {
    /* exclusive reservations */

    MDBO(8,R,fSTRUCT) MLog("INFO:     rsv access is job exclusive\n");

    return(FALSE);
    }

  if ((bmisset(&R->Flags,mrfExcludeAllButSB) &&
      !bmisset(&ReqR->Flags,mrfAllowPRsv) &&
      !bmisset(&ReqR->Flags,mrfAllowGrid)) ||
      (bmisset(&ReqR->Flags,mrfExcludeAllButSB) &&
      !bmisset(&R->Flags,mrfAllowPRsv) &&
      !bmisset(&R->Flags,mrfAllowGrid)))
    {
    /* exclusive reservations */

    MDBO(8,R,fSTRUCT) MLog("INFO:     rsv access is sandbox exclusive\n");

    return(FALSE);
    }

  MACLCheckAccess(R->ACL,ReqR->CL,NULL,Affinity,&InclusiveReservation);

  return(InclusiveReservation);
  }  /* END MRsvCheckRAccess() */






/**
 * Report TRUE if job J can access rsv R.
 *
 * @param R (I) [existing reservation]
 * @param J (I) [requesting consumer]
 * @param Overlap (I)
 * @param AC (I) [optional]
 * @param IgnBinding (I) do not reject access if binding not satisfied 
 * @param IsSame (O) rsv and job represent same object [optional]
 * @param IsBound (O) job is bound to rsv (optional) 
 * @param Affinity (O) [optional] affinity of job to rsv
 * @param Msg (O) [optional] MMAX_LINE
 */

mbool_t MRsvCheckJAccess(

  const mrsv_t *R,
  const mjob_t *J,
  long          Overlap,
  maclcon_t    *AC,
  mbool_t       IgnBinding,
  mbool_t      *IsSame,
  mbool_t      *IsBound,
  char         *Affinity,
  char         *Msg)

  {
  mbool_t IsInclusive;

  mjob_t *RJ;

  /* R:  existing job or user reservation */
  /* J:  job wrapper for new reservation  */

  /* attributes of a grid reservation */

  /*   container reservation          */
  /*   owned by user                  */
  /*   cannot be moved in time        */
  /*   queries should be allowed to request META res <X> or free resources */

  const char *FName = "MRsvCheckJAccess";

  MDBO(8,R,fSTRUCT) MLog("%s(%s,%s,%ld,%s,%s,IsSame,IsBound,Affinity,Msg)\n",
    FName,
    (R != NULL) ? R->Name : "NULL",
    (J != NULL) ? J->Name : "NULL",
    Overlap,
    (AC != NULL) ? "AC" : "NULL",
    MBool[IgnBinding]);

  if (Msg != NULL)
    Msg[0] = '\0';

  if ((R == NULL) || (J == NULL))
    {
    return(FAILURE);
    }

  if (IsSame != NULL)
    *IsSame = FALSE;

  if (IsBound != NULL)
    *IsBound = FALSE;

  /* Check if R is a standing reservation and if 
   * the job would violate its MAXJob policy */

  if (R->MaxJob > 0)
    {
    MDB(7,fSTRUCT) MLog("INFO:     reservation %s's jobcount is %d (MAXJOB policy: %d)\n",
      R->Name,
      R->JCount,
      R->MaxJob);

    if (R->JCount >= R->MaxJob)
      {
      MDB(7,fSTRUCT) MLog("INFO:     job denied access to reservation %s due to MaxJob policy (%d >= %d).\n",
        R->Name,
        R->JCount,
        R->MaxJob);

      if (Msg != NULL)
        {
        sprintf(Msg,"job denied access to reservation %s due to MaxJob policy (%d >= %d).\n",
          R->Name,
          R->JCount,
          R->MaxJob);
        }

      return(FAILURE);
      }
    }

  IsInclusive = FALSE;

  if ((J->RsvExcludeList != NULL) && (*(J->RsvExcludeList) != NULL))
    {
    int aindex;

    for (aindex = 0;aindex < MMAX_PJOB;aindex++)
      {
      if ((J->RsvExcludeList[aindex] == NULL) ||
          (J->RsvExcludeList[aindex][0] == '\0'))
        break;

      if (!strcmp(J->RsvExcludeList[aindex],R->Name) ||
         !strcmp(J->RsvExcludeList[aindex],ALL) ||
         ((R->J != NULL) && !strcmp(J->RsvExcludeList[aindex],"[ALLJOB]")))
        {
        /* reservation access explicitly denied */

        if (Affinity != NULL)
          *Affinity = mnmNegativeAffinity;

        MDBO(8,R,fSTRUCT) MLog("INFO:     denied direct access to reservation\n");

        if (Msg != NULL)
          {
          sprintf(Msg,"denied direct access to reservation '%s'",
            J->RsvExcludeList[aindex]);
          }
  
        return(FALSE);
        }
      }    /* END for (aindex) */
    }      /* END if (J->RsvExcludeList != NULL) */

  if (bmisset(&J->IFlags,mjifIgnRsv))
    {
    if (Affinity != NULL)
      *Affinity = mnmPositiveAffinity;

    MDBO(8,R,fSTRUCT) MLog("INFO:     job has ignrsv set, granted access\n");

    if (Msg != NULL)
      {
      sprintf(Msg,"job has ignrsv set, granted access\n");
      }

    return(TRUE);
    }

  if ((bmisset(&J->IFlags,mjifIgnJobRsv)) &&
      (R->Type == mrtJob))
    {
    if (Affinity != NULL)
      *Affinity = mnmPositiveAffinity;

    MDBO(8,R,fSTRUCT) MLog("INFO:     job has ignjobrsv set, granted access\n");

    if (Msg != NULL)
      {
      sprintf(Msg,"job has ignjobrsv set, granted access\n");
      }

    return(TRUE);
    }

  if (!bmisclear(&J->RsvAccessBM))
    {
    if (bmisset(&J->RsvAccessBM,mjapAllowSandboxOnly))
      {
      if (!bmisset(&R->Flags,mrfAllowGrid) &&
          !bmisset(&R->Flags,mrfAllowPRsv) &&
          (R->Type != mrtJob))
        {
        MDBO(8,R,fSTRUCT) MLog("INFO:     exclusive (non-sandbox rsv)\n");

        if (Msg != NULL)
          {
          sprintf(Msg,"denied access to non-sandbox reservation");
          }

        return(FALSE);
        }
      }

    if (bmisset(&J->RsvAccessBM,mjapAllowAll))
      {
      MDBO(8,R,fSTRUCT) MLog("INFO:     granted direct access to reservation\n");

      /* reservation access explicitly granted */

      if (Affinity != NULL)
        *Affinity = mnmPositiveAffinity;

      if (Msg != NULL)
        {
        sprintf(Msg,"granted direct access to reservation '%s'",
          R->Name);
        }

      return(TRUE);
      }

    if (bmisset(&J->RsvAccessBM,mjapAllowAllNonJob))
      {
      if (R->Type != mrtJob)
        {
        MDBO(8,R,fSTRUCT) MLog("INFO:     granted direct access to non-job reservation\n");

        /* reservation access explicitly granted */

        if (Affinity != NULL)
          *Affinity = mnmPositiveAffinity;

        if (Msg != NULL)
          {
          sprintf(Msg,"granted direct access to non-job reservation '%s'",
            R->Name);
          }

        return(TRUE);
        }
      }
    }    /* END if (J->RsvAccessBM != 0) */

  if (J->RsvAList != NULL)
    {
    /* check job's reservation access list for matching rsvid or rsvgroup */

    int aindex;

    for (aindex = 0;aindex < MMAX_PJOB;aindex++)
      {
      if ((J->RsvAList[aindex] == NULL) ||
          (J->RsvAList[aindex][0] == '\0'))
        break;

      if (!strcmp(J->RsvAList[aindex],R->Name) ||
         ((R->RsvGroup != NULL) && !strcmp(J->RsvAList[aindex],R->RsvGroup) && !bmisset(&R->Flags,mrfExcludeMyGroup)) ||
          !strcmp(J->RsvAList[aindex],ALL) ||
         ((R->J != NULL) && !strcmp(J->RsvAList[aindex],"[ALLJOB]")))
        {
        /* reservation access explicitly granted */

        if (Affinity != NULL)
          *Affinity = mnmPositiveAffinity;

        MDBO(8,R,fSTRUCT) MLog("INFO:     granted direct access to reservation\n");

        if (Msg != NULL)
          {
          sprintf(Msg,"granted direct access to reservation '%s'",
            J->RsvAList[aindex]);
          }

        return(TRUE);
        }
      }    /* END for (aindex) */
    }      /* END if (J->RsvAList != NULL) */

  if (R->Type == mrtJob)
    {
    RJ = R->J;

    if (bmisset(&J->Flags,mjfRsvMap) == FALSE)
      {
      if ((RJ == J) || /* reservation may be job's req reservation */
          (!strcmp(R->Name,J->Name)))
        {
        /* job located its own reservation */

        if (Affinity != NULL)
          *Affinity = mnmPositiveAffinity;

        MDBO(8,R,fSTRUCT) MLog("INFO:     granted access to reservation (own reservation)\n");

        if (Msg != NULL)
          {
          sprintf(Msg,"granted access to job reservation");
          }

        return(TRUE);
        }
      else if ((RJ != NULL) && (bmisset(&J->Flags,mjfIgnIdleJobRsv)) &&
               (MJOBISACTIVE(RJ) == FALSE) &&
               (MJobIsPreempting(RJ) == FALSE)) /* Don't start another ignidlejobrsv job in front 
                                                   of a preemptor that is going to cancel it anyways. */
        {
        /* job granted access to idle job reservation */

        if (Affinity != NULL)
          *Affinity = mnmPositiveAffinity;

        MDBO(8,R,fSTRUCT) MLog("INFO:     granted access to idle job reservation\n");

        if (Msg != NULL)
          {
          sprintf(Msg,"granted access to idle job reservation");
          }

        return(TRUE);
        }
      else if (MJobCanAccessIgnIdleJobRsv(J,R) == TRUE)
        {
        /* job granted access to idle job reservation */

        if (Affinity != NULL)
          *Affinity = mnmPositiveAffinity;

        MDBO(8,R,fSTRUCT) MLog("INFO:     granted access to running idle job reservation\n");

        if (Msg != NULL)
          {
          sprintf(Msg,"granted access to running idle job reservation");
          }

        return(TRUE);
        }
      else if (!strncmp(R->Name,J->Name,strlen(J->Name)))
        {
        /* BOC 7-18-11: Can this check be removed? It is now being handled
         * in the first check above with RJ == J. */

        /* maybe multi-req rsv (174 and 174.1) */

        /* allow acl test */

        /* NO-OP */
        }
#ifdef __GATECH_SUSPEND_RSV_ACCESS
      else if ((RJ != NULL) && (RJ->State == mjsSuspended))
        {
        if (Affinity != NULL)
          *Affinity = mnmNeutralAffinity;

        /* NOTE:  this should be handled by adjusting suspend job reservation to
                  not allocate processor resources in MRMJobSuspend() - NYI */

        MDBO(8,R,fSTRUCT) MLog("INFO:     granted access to suspended job reservation\n");

        if (Msg != NULL)
          {
          sprintf(Msg,"granted access to suspended job reservation");
          }

        return(TRUE);
        }
#endif /* __GATECH_SUSPEND_RSV_ACCESS */
      else if ((bmisset(&J->Flags,mjfIgnIdleJobRsv)) &&
               (MJOBISACTIVE(RJ) == FALSE) &&
               (MJobIsPreempting(RJ) == FALSE)) /* Don't start another ignidlejobrsv job in front 
                                                   of a preemptor that is going to cancel it anyways. */
        {
        /* job granted access to idle job reservation */
   
        if (Affinity != NULL)
          *Affinity = mnmPositiveAffinity;
   
        MDBO(8,R,fSTRUCT) MLog("INFO:     granted access to idle job reservation\n");
   
        if (Msg != NULL)
          {
          sprintf(Msg,"granted access to idle job reservation");
          }
   
        return(TRUE);
        }
      else if (MJobCanAccessIgnIdleJobRsv(J,R) == TRUE)
        {
        /* job granted access to idle job reservation */
   
        if (Affinity != NULL)
          *Affinity = mnmPositiveAffinity;
   
        MDBO(8,R,fSTRUCT) MLog("INFO:     granted access to running idle job reservation\n");
   
        if (Msg != NULL)
          {
          sprintf(Msg,"granted access to running idle job reservation");
          }
   
        return(TRUE);
        }
      else
        {
        /* two job reservations (ALWAYS EXCLUSIVE) */

        MDBO(8,R,fSTRUCT) MLog("INFO:     exclusive (two job reservations)\n");

        if (Msg != NULL)
          {
          sprintf(Msg,"denied access to exclusive reservation");
          }

        return(FALSE);
        }
      }  /* END if (bmisset(&J->Flags,mjfRsvMap) == FALSE) */
    else if (!bmisset(&J->IFlags,mjifSharedResource) &&
            (!bmisset(&J->IFlags,mjifACLOverlap)))
      {
      /* Job is a pseudo job representing an exclusive reservation */

      MDBO(8,R,fSTRUCT) MLog("INFO:     exclusive job denied access to reservation\n");

      if (Msg != NULL)
        {
        sprintf(Msg,"exclusive job denied access to reservation");
        }

      return(FALSE);
      } /* END else if (!MUBMFLAGISSET(J->IFlags,mjifSharedResource)) */
    else if (bmisset(&J->IFlags,mjifACLOverlap))
      {
      /* we are creating a reservation (not a job) and checking if we can overlap it
         onto this reservation's (is a job) resources */

      MACLCheckAccess(J->Credential.CL,R->CL,AC,Affinity,&IsInclusive);

      if (IsInclusive == TRUE)
        {
        MDBO(8,R,fSTRUCT) MLog("INFO:     reservation granted access to job via ACLOverlap\n");

        return(TRUE);
        }
      }
    else if (bmisset(&J->SpecFlags,mjfIgnIdleJobRsv))
      {
      /* we are creating a reservation (not a job) and checking if this is an idle job reservation */

      if (R->StartTime > MSched.Time)
        {
        IsInclusive = TRUE;

        MDBO(8,R,fSTRUCT) MLog("INFO:     reservation granted access to job via IgnIdleJobs flag\n");

        return(TRUE);
        }
      }
    }    /* END if (R->Type == mrtJob) */
  else
    {
    RJ = NULL;
    }    /* END else (R->Type == mrtJob) */

  if (bmisset(&R->Flags,mrfByName))
    {
    if (R->Type != mrtJob)
      {
      if (!bmisset(&J->IFlags,mjifBFJob) && /* allow bf windows to use by name reservations */
          (IgnBinding == FALSE) &&
          ((J->ReqRID == NULL) ||
           (strcmp(J->ReqRID,R->Name) &&
            ((R->RsvGroup == NULL) || strcmp(J->ReqRID,R->RsvGroup)))))
        {
        /* rsv-id/rsv-group is not explicitly requested */

        MDBO(8,R,fSTRUCT) MLog("INFO:     exclusive (not by name)\n");

        if (Msg != NULL)
          {
          sprintf(Msg,"denied access to 'byname' reservation");
          }

        return(FALSE);
        }
      }    /* END if (R->Type != mrtJob) */
    else
      {
      if ((RJ == NULL) || strcmp(J->Name,RJ->Name))
        {
        /* reservation is not specifically requested */

        MDBO(8,R,fSTRUCT) MLog("INFO:     exclusive (not by name)\n");

        if (Msg != NULL)
          {
          sprintf(Msg,"denied access to 'byname' reservation");
          }

        return(FALSE);
        }
      }
    }    /* END if (bmisset(&R->Flags,mrfByName) */

  if ((R->Type == mrtUser) && bmisset(&J->Flags,mjfRsvMap))
    {
    /* two user reservations */

    /* changed 8/21/09 by DRW, every RsvMap job belong to a standing reservation 
       should have J->MasterJobName populated in MRsvConfigure().  We can 
       easily check R->RsvParent and MasterJobName to see if they match */

    if ((bmisset(&R->Flags,mrfStanding)) &&
        (J->MasterJobName != NULL) &&
        (R->RsvParent != NULL) &&
        (!strcmp(R->RsvParent,J->MasterJobName)))
      {
      if (IsSame != NULL)
        *IsSame = TRUE;

      if (Affinity != NULL)
        *Affinity = mnmPositiveAffinity;

      MDBO(8,R,fSTRUCT) MLog("INFO:     granted access to shared standing reservation\n");

      if (Msg != NULL)
        {
        sprintf(Msg,"granted access to shared standing reservation");
        }

      return(TRUE);
      }

    /* FIXME:  incomplete exclusivity mapping (rsv <-> job) */

    if (!bmisset(&R->Flags,mrfAllowGrid) &&
        !bmisset(&R->Flags,mrfAllowPRsv) &&
        (strcmp(R->Name,J->Name)) &&  /* allow myself */
        (bmisset(&R->Flags,mrfExcludeAll) ||
         !bmisset(&J->IFlags,mjifSharedResource)))
      {
      MDBO(8,R,fSTRUCT) MLog("INFO:     exclusive (not shared resource)\n");

      if (Msg != NULL)
        {
        sprintf(Msg,"denied access to exclusive reservation");
        }

      return(FALSE);
      }
    else if (bmisset(&R->Flags,mrfExcludeMyGroup) &&
             (R->RsvGroup != NULL) &&
             (J->RsvAList != NULL))
      {
      /* check job's reservation access list for matching rsvid or rsvgroup */

      int aindex;

      for (aindex = 0;aindex < MMAX_PJOB;aindex++)
        {
        if (J->RsvAList[aindex] == NULL)
          break;

        if (!strcmp(J->RsvAList[aindex],R->Name) ||
           ((R->RsvGroup != NULL) && !strcmp(J->RsvAList[aindex],R->RsvGroup) && !bmisset(&R->Flags,mrfExcludeMyGroup)) ||
            !strcmp(J->RsvAList[aindex],ALL) ||
           ((R->J != NULL) && !strcmp(J->RsvAList[aindex],"[ALLJOB]")))
          {
          /* reservation access explicitly granted */

          if (Affinity != NULL)
            *Affinity = mnmPositiveAffinity;

          MDBO(8,R,fSTRUCT) MLog("INFO:     granted direct access to reservation via ACLList\n");

          if (Msg != NULL)
            {
            sprintf(Msg,"granted direct access to reservation '%s'",
              J->RsvAList[aindex]);
            }

          return(TRUE);
          }
        }    /* END for (aindex) */
      }
    else
      {
      /* at least one reservation will share resources */

      if (Affinity != NULL)
        *Affinity = mnmPositiveAffinity;

      MDBO(8,R,fSTRUCT) MLog("INFO:     granted direct access to shared reservation\n");

      if (Msg != NULL)
        {
        sprintf(Msg,"granted access to shared reservation");
        }

      return(TRUE);
      }
    }    /* END if ((R->Type == mrtUser) && bmisset(&J->Flags,mjfRsvMap)) */

  /* check VPC constraints */

  if (bmisset(&J->IFlags,mjifVPCMap) && bmisset(&R->Flags,mrfIsVPC))
    {
    if (MSched.EnableVPCPreemption == FALSE)
      {
      MDBO(8,R,fSTRUCT) MLog("INFO:     exclusive (VPC reservation)\n");

      if (Msg != NULL)
        {
        sprintf(Msg,"denied access to VPC");
        }

      return(FALSE);
      }
    else if ((J->ReqVPC != NULL) && (R->RsvGroup != NULL))
      {
      mvpcp_t *Preemptor = NULL;
      mpar_t  *Preemptee = NULL;

      MVPCFind(R->RsvGroup,&Preemptee,TRUE);
      MVPCProfileFind(J->ReqVPC,&Preemptor);

      return(MVPCCanPreempt(Preemptor,J,Preemptee,R,NULL));
      }
    }   /* END if (bmisset(&J->IFlags,mjifVPCMap) && bmisset(&R->Flags,mrfIsVPC)) */

  /* check 'ReqRID' specification */

  if ((IgnBinding == FALSE) && (J->ReqRID != NULL))
    {
    /* check standing reservation name */
    
    if (bmisset(&J->IFlags,mjifNotAdvres) &&
        (MRsvIsReqRID(R,J->ReqRID) == TRUE))
      {
      if (Msg != NULL)
        sprintf(Msg,"denied access to non-required reservation");

      return(FALSE);
      }
    else if (MRsvIsReqRID(R,J->ReqRID) == FALSE)
      {
      if (Msg != NULL)
        sprintf(Msg,"denied access to non-required reservation");

      return(FALSE);
      }

    if (Affinity != NULL)
      *Affinity = mnmPositiveAffinity;
    }   /* END if (J->ReqRID != NULL) */

  if (R->Type == mrtJob)
    {
    if (RJ->ReqRID != NULL)
      {
      /* NOTE:  a job, representing an rsv resource request, may carry a *
       *        MasterJobName attribute mapping the rsv group to the job */

      if ((J->MasterJobName != NULL) && !strcmp(RJ->ReqRID,J->MasterJobName))
        {
        MDBO(8,R,fSTRUCT) MLog("INFO:     inclusive ('ReqRID' rsv group)\n");
        }     /* END if (bmisset(&R->RsvGroup != NULL) && ...) */
      else if (!strcmp(RJ->ReqRID,J->Name))
        {
        MDBO(8,R,fSTRUCT) MLog("INFO:     inclusive ('ReqRID' rsv id)\n");
        } /* END else if (strcmp(R->Name,J->ReqRID)) */
      else
        {
        MDBO(8,R,fSTRUCT) MLog("INFO:     exclusive (not 'ReqRID' reservation)\n");

        if (Msg != NULL)
          {
          sprintf(Msg,"denied access to non-required reservation");
          }

        return(FALSE);
        }
      }    /* END if (RJ->ReqRID != NULL) */
    }      /* END if (R->Type == mrtJob) */

  if (MACLIsEmpty(J->Credential.CL))
    {
    /* some temp jobs don't have CL */

    if (!bmisset(&J->IFlags,mjifTemporaryJob))
      {
      MDBO(8,R,fSTRUCT) MLog("INFO:     job '%s' has NULL cred list\n",
        J->Name);
  
      if (Msg != NULL)
        {
        sprintf(Msg,"denied access because of invalid job cred list");
        }
      }

    return(FALSE);
    }

  /* one reservation/one job */

  /* job seeks access to reservation */

  if ((R->ExpireTime <= 0) || (R->ExpireTime == MMAX_TIME))
    {
    macl_t *tmpCL = NULL;

    /* NOTE:  temporarily adjust 'overlap' time */

    MACLCopy(&tmpCL,J->Credential.CL);

    if (Overlap >= 0)
      MACLSet(&tmpCL,maDuration,&Overlap,mcmpEQ,mnmPositiveAffinity,0,FALSE);

    /* NOTE:  rsv wrappers gain access if R(job) is inclusive on J(rsv) */

    if (bmisset(&J->IFlags,mjifBFJob))
      {
      /* BF Jobs have access to all RSV's but system */

      if (!MACLIsEmpty(R->ACL))
        {
        IsInclusive = TRUE;

        if (Affinity != NULL)
          *Affinity = mnmPositiveAffinity;
        }
      else
        {
        IsInclusive = FALSE;

        if (Affinity != NULL)
          *Affinity = mnmUnavailable;
        }
      }  /* END if (bmisset(&J->IFlags,mjifBFJob)) */
    else
      {
      if (bmisset(&J->SpecFlags,mjfRsvMap))
        MACLCheckAccess(tmpCL,R->CL,AC,Affinity,&IsInclusive);
      else
        MACLCheckAccess(R->ACL,J->Credential.CL,AC,Affinity,&IsInclusive);
      }

    MACLFreeList(&tmpCL);
    }  /* END if ((R->ExpireTime <= 0) || (R->ExpireTime == MMAX_TIME)) */

  if (IsInclusive == FALSE)
    {
    /* check to see if this rsv allows jobs to overlap (start before
     * the rsv and then run into its boundaries) */

    if (bmisset(&R->Flags,mrfAllowJobOverlap))
      {
      /* check to see if we are considering starting
       * this job during during the reservation--if not, allow
       * job access */

       if (R->StartTime > MSched.Time)
        {
        MDBO(8,R,fSTRUCT) MLog("INFO:     granted overlap access to reservation\n");

        if (Msg != NULL)
          {
          sprintf(Msg,"granted overlap access to reservation");
          }
   
        return(TRUE);              
        }
      }
    }

  if (IsInclusive == TRUE)
    {
    MDBO(8,R,fSTRUCT) MLog("INFO:     granted access via ACL constraints\n");
    }
  else
    {
    MDBO(8,R,fSTRUCT) MLog("INFO:     denied access via ACL constraints\n");
    }

  if (Msg != NULL)
    {
    if (IsInclusive == TRUE)
      sprintf(Msg,"granted access via ACL constraints");
    else
      sprintf(Msg,"denied access via ACL constraints");
    }

  return(IsInclusive);
  }  /* END MRsvCheckJAccess() */





/**
 * Validate all current reservations.
 *
 * @see MSysMainLoop() - parent
 * @see MQueueCheckStatus() - peer
 * @see MNodeCheckStatus() - peer
 *
 * @param SR (I) [optional]
 */

int MRsvCheckStatus(

  mrsv_t *SR)  /* I (optional) */

  {
  rsv_iter RTI;

  mrsv_t  *R;

  mjob_t  *J;

  char     TString[MMAX_LINE];

  const char *FName = "MRsvCheckStatus";

  MDBO(4,SR,fSTRUCT) MLog("%s(%s)\n",
    FName,
    (SR != NULL) ? SR->Name : "NULL");

  MRsvIterInit(&RTI);

  while (MRsvTableIterate(&RTI,&R) == SUCCESS)
    {
    if ((SR != NULL) && (R != SR))
      continue;

    if (R->StartTime == 0)
      continue;

    MULToTString(R->EndTime - MSched.Time,TString);

    MDBO(5,SR,fSTRUCT) MLog("INFO:     checking Rsv '%s'  end: %s\n",
      R->Name,
      TString);

    if ((MSched.Iteration > 1) && (R->HostExpIsSpecified == MBNOTSET))
      {
      /* NOTE:  handle checkpoint rsv allocation bootstrap */

      /* NOTE:  should we change this to check if R->NL is already allocated? */
      /*        if so, set R->HostExpIsSpecified and clear R->HostExp */
      /*        see MRsvReplaceNode() try change in 5.0 beta? */

      /* 1/20/10 BOC Reservations have already been populated at startup
       * with MRsvPopulateNL */

      if ((bmisset(&MSched.Flags,mschedfFastRsvStartup) == TRUE) || 
          (MRsvCreateNL(
           &R->NL,  /* O (alloc) */
           R->HostExp,
           0,             /* pass 0 to request all possible matches */
           0,
           NULL,
           &R->DRes,
           NULL) == SUCCESS))
        {
        R->HostExpIsSpecified = FALSE;

        MUFree(&R->HostExp);
        }
      }

    if ((MSched.Time > R->EndTime) ||
       ((R->ExpireTime > 0) && (MSched.Time > R->ExpireTime)))
      {
      J = R->J;

      if (((R->Type == mrtJob) || (R->Type == mrtMeta)) &&
          (J != NULL) &&
          (MJOBISACTIVE(J) == TRUE))
        {
        /* extend reservation */

        if ((bmisset(&J->IFlags,mjifFlexDuration)) &&
            (J->WCLimit < 6000) &&
            (MJobCheckExpansion(J,MSched.Time,MSched.Time + 600) == SUCCESS))
          {
          /* MRMJobModify(); */

          MJobSetAttr(J,mjaReqAWDuration,(void **)"600",mdfString,mSet);
          }

        MDBO(1,SR,fSTRUCT) MLog("INFO:     extending reservation for overrun job %s by %ld seconds\n",
          J->Name,
          MSched.Time + MSched.MaxRMPollInterval - R->EndTime);

        R->EndTime = MSched.Time + MSched.MaxRMPollInterval;

        MRsvAdjustTimeframe(R);
        }
      else
        {
        mtrig_t *T;

        int tindex;

        mbool_t ActiveTriggerFound = FALSE;

        if (R->T != NULL)
          {
          for (tindex = 0; tindex < R->T->NumItems;tindex++)
            {
            /* Check for any active triggers */

            T = (mtrig_t *)MUArrayListGetPtr(R->T,tindex);
     
            if (MTrigIsReal(T) == FALSE)
              continue;
     
            if ((T->EType != mttEnd) || (T->Offset <= 0))
              continue;

            if (T->State == mtsActive)
              {
              ActiveTriggerFound = TRUE;

              break;
              }
            }  /* END for (tindex) */
          }    /* END if (R->T != NULL) */

        if (ActiveTriggerFound == TRUE)
          {
          /* Do not allow rsv to be removed if there is a trigger active */

          MDBO(1,SR,fSTRUCT) MLog("INFO:     extending reservation by %ld seconds (trigger still active)\n",
            MSched.Time + MSched.MaxRMPollInterval - R->EndTime);

          R->EndTime = MSched.Time + MSched.MaxRMPollInterval;

          MRsvAdjustTimeframe(R);
          }
        else if ((R->Type == mrtJob) || (R->Type == mrtMeta))
          {
          if ((R->Type == mrtJob) &&    /* allow idle job reservations to be removed */
             (J != NULL) &&
             (MJOBISIDLE(J) == TRUE))
            {

            if (MJobReleaseRsv(J,TRUE,TRUE) == FAILURE)
              {
              MDBO(1,SR,fSTRUCT) MLog("ERROR:    unable to destroy job reservation\n");
              }

            continue;
            }
          else
            continue;
          /* Job will clean up it's own reservations, including multi-req reservations -- MJobReleaseRsv */

          } /* END else (ActiveTriggerFound == TRUE) */
        else
          {
          MDBO(1,SR,fSTRUCT) MLog("INFO:     clearing expired%s reservation '%s' on iteration %d  (start: %ld end: %ld)\n",
            ((R->ExpireTime > 0) && (R->ExpireTime < MMAX_TIME)) ? " courtesy" : "",
            R->Name,
            MSched.Iteration,
            R->StartTime - MSched.Time,
            R->EndTime - MSched.Time);

          MRsvDestroy(&R,TRUE,TRUE);

          /* R is no longer valid - cannot be processed further */

          continue;
          }
        }
      }     /* END if ((MSched.Time) || ... ) */

    if ((R->StartTime <= MSched.Time) &&
        (R->Type != mrtJob) &&
        (HardPolicyRsvIsActive == FALSE))
      {
      macl_t *tmpACL = R->ACL;

      while (tmpACL != NULL)
        {
        if (bmisset(&tmpACL->Flags,maclHPEnable))
          {
          HardPolicyRsvIsActive = TRUE;

          break;
          }

        tmpACL = tmpACL->Next;
        }
      }    /* END if ((R->StartTime <= MSched.Time)...) */
    }      /* END while (MRsvTableIterate()) */

  return(SUCCESS);
  }  /* END MRsvCheckStatus() */




/**
 *
 *
 * @param J (I)
 * @param R (I)
 */

int MRsvCheckJobMatch(

  mjob_t *J,  /* I */
  mrsv_t *R)  /* I */

  {
  mre_t *RE;

  int nindex;
  int nlindex;
  int rqindex;

  mnode_t *N;
  mnode_t *tmpN;

  mreq_t  *RQ;

  long     tmpEndTime;

  double   NPFactor;

  int      NLTaskCount;

  const char *FName = "MRsvCheckJobMatch";

  MDB(6,fSTRUCT) MLog("%s(%s,%s)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (R != NULL) ? R->Name : "NULL");

  if ((J == NULL) || (R == NULL))
    {
    return(FAILURE);
    }

  /* return success if reservation adequately 'covers' up-to-date job requirements */

  /* NOTE:  DISABLE UNTIL FULLY TESTED */

  return(FAILURE);

  /*NOTREACHED*/

  /* verify reservation timeframe  */

  if (J->DispatchTime != R->StartTime)
    {
    /* job start time has changed */

    MDB(3,fSTRUCT) MLog("ALERT:    start time has changed for job %s (%ld != %ld)\n",
      J->Name,
      J->DispatchTime,
      R->StartTime);

    return(FAILURE);
    }

  NPFactor = 9999999.0;

  for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
    {
    RQ = J->Req[rqindex];

    if (!MNLIsEmpty(&RQ->NodeList))
      {
      double tmpD;

      if (MUNLGetMinAVal(
            &RQ->NodeList,
            mnaSpeed,
            NULL,
            (void **)&tmpD) == SUCCESS)
        {
        NPFactor = MIN(NPFactor,tmpD);
        }
      }
    }  /* END for (rqindex) */

  if (NPFactor == 9999999.0)
    {
    /* node speed factor not set */

    NPFactor = 1.0;
    }

  /* verify endtime */

  tmpEndTime = MAX(
    J->DispatchTime + (long)((double)J->SpecWCLimit[0] / NPFactor),
    MSched.Time + 1);

  if (tmpEndTime != (long)R->EndTime)
    {
    /* job completion time has changed */

    MDB(3,fSTRUCT) MLog("ALERT:    completion time has changed for job %s (%ld != %ld)\n",
      J->Name,
      J->DispatchTime + (long)((double)J->SpecWCLimit[0] / NPFactor),
      R->EndTime);

    return(FAILURE);
    }

  /* verify reservation tasks match job tasks */

  /* verify all job tasks have matching reservation task */
  /* verify all node reservations have matching job task */

  NLTaskCount = 0;

  for (nlindex = 0;MNLGetNodeAtIndex(&J->Req[0]->NodeList,nlindex,NULL) == SUCCESS;nlindex++)
    {
    NLTaskCount += MNLGetTCAtIndex(&J->Req[0]->NodeList,nlindex);
    }

  for (nindex = 0;nindex < MSched.M[mxoNode];nindex++)
    {
    N = MNode[nindex];

    if ((N == NULL) || (N->Name[0] == '\0'))
      break;

    if (N->Name[0] == '\1')
      continue;

    for (RE = N->RE;RE != NULL;MREGetNext(RE,&RE))
      {
      if (MREGetRsv(RE) != R)
        continue;

      /* matching reservation found, verify job requests node */

      for (nlindex = 0;MNLGetNodeAtIndex(&J->Req[0]->NodeList,nlindex,&tmpN) == SUCCESS;nlindex++)
        {
        if (nindex == tmpN->Index)
          break;
        }

      if (MNLGetNodeAtIndex(&J->Req[0]->NodeList,nlindex,NULL) == FAILURE)
        {
        /* reserved node not in current job nodelist */

        MDB(3,fSTRUCT) MLog("ALERT:    reserved node '%s' no longer in job %s nodelist\n",
          N->Name,
          J->Name);

        return(FAILURE);
        }

      NLTaskCount -= RE->TC;
      }  /* END for (MREGetNext) */
    }    /* END for (nindex) */

  if (NLTaskCount != 0)
    {
    /* node list task count mismatch */

    MDB(3,fSTRUCT) MLog("ALERT:    node list taskcount mismatch for job %s (%d %s tasks)\n",
      J->Name,
      (NLTaskCount > 0) ? NLTaskCount : -NLTaskCount,
      (NLTaskCount > 0) ? "missing" : "additional");

    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MRsvCheckJobMatch() */





/**
 * Return success if node meets all rsv constraints.
 *
 *  OS,Arch,Network,Mem,ReqFBM,Partition
 *
 * NOTE: does not utilize R->ReqNL
 *       assume parent checks R->HostExp
 *
 * @param R (I)
 * @param N (I)
 */

int MRsvCheckNodeAccess(

  mrsv_t  *R,  /* I */
  mnode_t *N)  /* I */

  {
  if ((R == NULL) || (N == NULL))
    {
    return(FAILURE);
    }

  if ((R->ReqOS != 0) && (N->ActiveOS != R->ReqOS))
    {
    return(FAILURE);
    }

  if ((R->ReqArch != 0) && (N->Arch != R->ReqArch))
    {
    return(FAILURE);
    }

  if (N->CRes.Mem < R->ReqMemory)
    {
    return(FAILURE);
    }

  /* check feature list */

  if (!bmisclear(&R->ReqFBM))
    {
    if (MAttrSubset(&R->ReqFBM,&N->FBM,mclAND) == FAILURE)
      {
      return(FAILURE);
      }
    }

  if ((R->PtIndex > 0) && (R->PtIndex != N->PtIndex))
    {
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MRsvCheckNodeAccess() */





/**
 * Make sure reservation nodelist is ready with provisioning or vms.
 *
 */

int MRsvCheckAdaptiveState(

  mrsv_t *R,
  char   *EMsg)

  {
  if ((R->J == NULL) ||
      (R->ReqOS == 0) ||
      (!bmisset(&R->Flags,mrfProvision)) ||
      (R->Q == NULL) ||
      (!bmisset(&R->Q->Flags,mqfProvision)))
    {
    return(SUCCESS);
    }  /* END if (ROS != NULL) */

  R->J->VMUsagePolicy = R->VMUsagePolicy;
  R->J->Req[0]->Opsys = R->ReqOS;

  return(SUCCESS);
  }  /* END MRsvCheckAdaptiveReqs() */
/* END MRsvCheck.c */
