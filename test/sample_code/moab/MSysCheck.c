/* HEADER */

      
/**
 * @file MSysCheck.c
 *
 * Contains: MSysCheck 
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 * Redump everything to the cache.
 *
 * WARNING: this can be an expensive routine on certain systems (especially if using a database on a different node)
 */

int MSysRefreshCache()

  {
  mjob_t  *J;

  job_iter JTI;

  MJobIterInit(&JTI);

  while (MJobTableIterate(&JTI,&J) == SUCCESS)
    {
    MJobTransition(J,TRUE,FALSE);
    }  /* END while (MJobTableIterate()) */
 
  MCJobIterInit(&JTI);

  while (MCJobTableIterate(&JTI,&J) == SUCCESS)
    {
    MJobTransition(J,FALSE,FALSE);
    } /* END while (MCJobTableIterate()) */

#if 0 /* for now we're just doing jobs */
  int nindex;
  mhashiter_t HTI;

  MUHTIterInit(&HTI);

  mrsv_t  *R;
  mnode_t *N;

  while (MRsvTableIterate(&HTI,&R) == SUCCESS)
    {
    MRsvTransition(R);
    } /* END for (rindex) */

  for (nindex = 0;nindex < MSched.M[mxoNode];nindex++)
    {
    N = MNode[nindex];

    if ((N == NULL) || (N->Name[0] == '\0'))
      break;

    if (N->Name[0] == '\1')
      continue;
    }  /* END for (nindex) */
#endif

  return(SUCCESS);
  }  /* END MSysRefreshCache() */



/**
 * check general system aspects 
 *
 */

int MSysCheck()

  {
  job_iter JTI;

  int      nindex;

  mjob_t  *J;

  mnode_t *N;

  mrm_t   *RM;

  char Message[MMAX_LINE];

  enum MAllocResFailurePolicyEnum ARFPolicy;

  enum MNodeStateEnum State;

  const char *FName = "MSysCheck";

  MDB(4,fCORE) MLog("%s()\n",
    FName);

  MLimitEnforceAll(&MPar[0]);

  MJobIterInit(&JTI);

  /* locate jobs with various issues */

  while (MJobTableIterate(&JTI,&J) == SUCCESS)
    {
    if ((MSched.LimitedJobCP == TRUE) && (MSched.EnableHighThroughput == FALSE))
      {
      /* in high-throughput only transition on <events>, never at a regular interval */
      MJobTransition(J,TRUE,FALSE);
      }

    if (MJOBISACTIVE(J))
      {
      RM = (J->SubmitRM != NULL) ? J->SubmitRM : &MRM[0];

      /* locate jobs which have allocated 'down' nodes */
   
      if ((long)(MSched.Time - J->StartTime) > (long)MPar[0].MaxJobStartTime)
        {
        if (RM->Type == mrmtPBS)
          {
          if ((J->State == mjsIdle) &&
             ((J->EState == mjsStarting) || (J->EState == mjsRunning)))
            {
            char tmpLine[MMAX_LINE];
            char TString[MMAX_LINE];

            MULToTString(MSched.Time - J->StartTime,TString);

            MDB(2,fCORE) MLog("ALERT:    PBS job '%s' in state '%s' was started %s ago.  assuming prolog hang and cancelling job\n",
              J->Name,
              MJobState[J->State],
              TString);
   
            sprintf(tmpLine,"%s:  job cannot start\n",
              MSched.MsgInfoHeader);
   
            MJobCancel(J,tmpLine,FALSE,NULL,NULL);
            }
          }
	      }
   
      if (J->System != NULL)
        {
	      continue;
	      }

      for (nindex = 0;MNLGetNodeAtIndex(&J->NodeList,nindex,&N) == SUCCESS;nindex++)
        {
        State = mnsActive;

        if (((MNodeGetRMState(N,mrmrtCompute,&State) == FAILURE) &&
             (MNodeGetRMState(N,mrmrtAny,&State) == FAILURE)) ||
            (State == mnsDown))
          {
          mbool_t ARFOnMaster = FALSE;
          char tmpLine[MMAX_LINE];
 
          if ((int)(MSched.Time - N->StateMTime) < MSched.ARFDuration)
            {
            continue;
            }

          /* job will not necessarily be cancelled */
          MDB(1,fCORE) MLog("ALERT:    job '%s' has been in state '%s' for %ld seconds.  node '%s' is in state '%s' \n",
            J->Name,
            MJobState[J->State],
            MSched.Time - J->StartTime,
            N->Name,
            MNodeState[N->State]);
 
          sprintf(Message,"JOBCORRUPTION:  job '%s' (user %s) has been in state '%s' for %ld seconds.  node '%s' is in state '%s' \n",
            J->Name,
            J->Credential.U->Name,
            MJobState[J->State],
            MSched.Time - J->StartTime,
            N->Name,
            MNodeState[N->State]);
 
          MSysRegEvent(Message,mactNONE,1);
 
          sprintf(tmpLine,"%s:  job has 'DOWN' node allocated\n",
            MSched.MsgErrHeader);


          /* precedence:  job, class, partition, scheduler */

          if (J->ResFailPolicy != marfNONE)
            {
            ARFPolicy = J->ResFailPolicy;
            }
          else if ((J->Credential.C != NULL) && (J->Credential.C->ARFPolicy != marfNONE))
            {
            ARFPolicy = J->Credential.C->ARFPolicy;
            ARFOnMaster = J->Credential.C->ARFOnMasterTaskOnly;
            }
          else if (MPar[N->PtIndex].ARFPolicy != marfNONE)
            {
            ARFPolicy = MPar[N->PtIndex].ARFPolicy;
            }
          else if (MPar[0].ARFPolicy != marfNONE) /* look on the default partition for this policy */
            {
            ARFPolicy = MPar[0].ARFPolicy;
            }
          else
            {
            ARFPolicy = MSched.ARFPolicy;
            ARFOnMaster = MSched.ARFOnMasterTaskOnly;
            }

          if ((ARFOnMaster == FALSE) || (MNLReturnNodeAtIndex(&J->Req[0]->NodeList,0) == N))
            {
            switch (ARFPolicy)
              {
              case marfCancel:

                MJobPreempt(J,NULL,NULL,NULL,"node failure",mppCancel,TRUE,NULL,NULL,NULL);

                MDB(1,fRM) MLog("ALERT:    job '%s' cancelled (allocated node %s has failed)\n",
                  J->Name,
                  N->Name);

                break;

              case marfFail:

                MDB(1,fRM) MLog("ALERT:    job '%s' is being marked as failed (allocated node %s has failed)\n",
                  J->Name,
                  N->Name);

                MJobFail(J,MDEF_NODE_FAILURE_CCODE);

                return(SUCCESS);

                /*NOTREACHED*/

                break;

              case marfRequeue:

                bmset(&J->IFlags,mjifFailedNodeAllocated);

                MJobPreempt(J,NULL,NULL,NULL,"node failure",mppRequeue,TRUE,NULL,NULL,NULL);

                MDB(1,fRM) MLog("ALERT:    job '%s' requeued (allocated node %s has failed)\n",
                  J->Name,
                  N->Name);

                break;

              case marfHold:

                bmset(&J->IFlags,mjifFailedNodeAllocated);

                MJobPreempt(J,NULL,NULL,NULL,"node failure",mppRequeue,TRUE,NULL,NULL,NULL);

                MDB(1,fRM) MLog("ALERT:    job '%s' requeued and held (allocated node %s has failed)\n",
                  J->Name,
                  N->Name);

                MJobSetHold(J,mhBatch,0,mhrRMReject,NULL);

                break;

              case marfNotify:

                /* only send one notification per failure */

                if (N->State != N->OldState)
                  {
                  MSysSendMail(
                    MSysGetAdminMailList(1),
                    NULL,
                    NULL,
                    NULL,
                    NULL);
                  }

                break;

              default:

                /* NO-OP */

                break;
              }  /* END switch (ARFPolicy) */
            }    /* END if ((ARFOnMaster == FALSE) || (J->Req[0]->NodeList[0].N == N)) */
          }    /* END if (((N->State == mnsIdle) || ...)) */
        }      /* END for (nindex) */
      }          /* END if (MJOBISACTIVE(J)) */

    if (MJOBISIDLE(J) == TRUE)
      {
      if ((J->Credential.Q != NULL) && (J->Credential.Q->TList != NULL))
        {
        /* verify trigger thresholds */

        /* MJobCheckCredTriggers(J); */
        }
      }

    if (!MJOBISACTIVE(J))
      {
      if ((!bmisclear(&J->Hold)) && (MSched.MaxJobHoldTime > 0) &&
          (((J->HoldTime != 0) && 
            (MSched.Time - J->HoldTime > (unsigned int)MSched.MaxJobHoldTime)) ||
          ((J->HoldTime == 0) && 
            (MSched.Time - J->SubmitTime > (unsigned int)MSched.MaxJobHoldTime))))
        {
        MJobCancel(J,"job violates JOBMAXHOLDTIME",FALSE,NULL,NULL);
        }
      }
    }        /* END for (jindex) */

  if (bmisset(&MSched.Flags,mschedfAlwaysUpdateCache))
    {
    MSysRefreshCache();
    }  /* END if (bmisset(&MSched.Flags,mschedfAlwaysUpdateCache)) */

  /* Harvest any old VCs */

  MVCHarvestVCs();

  /* clear all defunct child processes */

  MUClearChild(NULL);

  if (MAM[0].Type != mamtNONE)
    {
    /* check health of primary accounting manager */

    if (MAM[0].State == mrmsDown)
      {
      MAMInitialize(&MAM[0]);
      }
    }    /* END if (MAM[0].Type != mamtNONE) */

  return(SUCCESS);
  }  /* END MSysCheck() */

/* END MSysCheck.c */
