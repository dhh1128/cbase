/* HEADER */

      
/**
 * @file MJobExtendStartWallTime.c
 *
 * Contains: MJobExtendStartWallTime
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 * If job wall time ranges are supported, check to see how long the job can run
 * (within the min and max ranges specified) for the nodes assigned to the
 * job and set the walltime to use the amount of time available.
 *
 * @param J (I)
 */

int MJobExtendStartWallTime(

  mjob_t    *J)   /* I */
  {
  mpar_t   *P;
  mrange_t  Range[MMAX_RANGE];
  mrange_t *RangePtr = Range;
  mulong    NewWCLimit;
  char      tmpLine[MMAX_LINE];
  char      EMsg[MMAX_LINE];
  char     *tmpPtr;

  mnode_t  *N;

  const char *FName = "MJobStartMaxWallTime";

  MDB(4,fUI) MLog("%s(%s))\n",
    FName,
    (J != NULL) ? J->Name : "NULL");

  if (J == NULL)
    {
    return(FAILURE);
    }

  if (MSched.JobExtendStartWallTime != TRUE)
    {
    /* feaature to pick a walltime at job start from a range not enabled */

    return(SUCCESS);
    }

  if (J->OrigMinWCLimit <= 0)
    {
    /* no min range so we are done */

    return(SUCCESS);
    }

  if (J->OrigWCLimit <= 0)
    {
    J->OrigWCLimit = J->SpecWCLimit[0];
    }

  if ((J->Req[0] == NULL) ||
      (MNLIsEmpty(&J->Req[0]->NodeList)))
    {
    /* the nodes this job is going to start on must already be in the job node list */

    return(SUCCESS);
    }

  /* get the partition the job is going to be started on */

  MNLGetNodeAtIndex(&J->Req[0]->NodeList,0,&N);

  P = &MPar[N->PtIndex];

  /* find out how much walltime is available for this job on the selected nodes */

  if (MJobGetRange(
        J,
        J->Req[0],
        &J->Req[0]->NodeList, /* I */
        P,
        MSched.Time,         /* I */
        MSched.JobExtendStartWallTime, /* I */
        RangePtr,            /* O */
        NULL,
        NULL,
        NULL,
        0,  /* do not support firstonly/notask */
        FALSE,
        NULL,
        EMsg) == FAILURE)
    {
    MDB(4,fSCHED) MLog("INFO:     job '%s' unable to get resource range info - '%s'\n",
      J->Name,
      EMsg);

    return(FAILURE);
    }

  /* make sure the available walltime is at least as long as the min walltime */

  if ((MSched.Time + J->MinWCLimit) >= RangePtr->ETime)
    {
    return(SUCCESS);
    }

  /* if the range endtime is less than the max walltime then use the range
     end time, otherwise, use the max walltime */

  NewWCLimit = MIN((RangePtr->ETime - MSched.Time),J->OrigWCLimit);

  /* some resource managers (e.g. slurm) round the walltime up to the nearest minute.
     round our walltime down so that the resource manager will not bump up the walltime */

  NewWCLimit = (NewWCLimit / MCONST_MINUTELEN) * MCONST_MINUTELEN;

  /* if the new walltime is less than 1 minute then don't bother trying to change the walltime */

  if (NewWCLimit == 0)
    {
    MDB(4,fSCHED) MLog("INFO:     job '%s' walltime range adjustment less than 1 minute\n",
      J->Name);

    return(SUCCESS);
    }

  if (!bmisset(&J->Flags,mjfIgnPolicies))
    {
    mjob_t *JMax = NULL;

    /* enforce partition limits */

    if (((JMax = P->L.JobMaximums[0]) != NULL) &&
        (JMax->SpecWCLimit[0] > 0) &&
        (NewWCLimit > JMax->SpecWCLimit[0]))
      {
      NewWCLimit = JMax->SpecWCLimit[0];
      }

    /* enforce class limits */

    if ((J->Credential.C != NULL) &&
        ((JMax = J->Credential.C->L.JobMaximums[0]) != NULL) &&
        (JMax->SpecWCLimit[0] > 0) &&
        (NewWCLimit > JMax->SpecWCLimit[0]))
      {
      NewWCLimit = JMax->SpecWCLimit[0];
      }

    /* enforce user limits */

    if ((J->Credential.U != NULL) &&
        ((JMax = J->Credential.U->L.JobMaximums[0]) != NULL) &&
        (JMax->SpecWCLimit[0] > 0) &&
        (NewWCLimit > JMax->SpecWCLimit[0]))
      {
      NewWCLimit = JMax->SpecWCLimit[0];
      }

    /* enforce account limits */

    if ((J->Credential.A != NULL) &&
        ((JMax = J->Credential.A->L.JobMaximums[0]) != NULL) &&
        (JMax->SpecWCLimit[0] > 0) &&
        (NewWCLimit > JMax->SpecWCLimit[0]))
      {
      NewWCLimit = JMax->SpecWCLimit[0];
      }

    /* enforce group limits */

    if ((J->Credential.G != NULL) &&
        ((JMax = J->Credential.G->L.JobMaximums[0]) != NULL) &&
        (JMax->SpecWCLimit[0] > 0) &&
        (NewWCLimit > JMax->SpecWCLimit[0]))
      {
      NewWCLimit = JMax->SpecWCLimit[0];
      }

    /* enforce qos limits */

    if ((J->Credential.Q != NULL) &&
        ((JMax = J->Credential.Q->L.JobMaximums[0]) != NULL) &&
        (JMax->SpecWCLimit[0] > 0) &&
        (NewWCLimit > JMax->SpecWCLimit[0]))
      {
      NewWCLimit = JMax->SpecWCLimit[0];
      }

    } /* END if !bmisset(&J->Flags,mjfIgnPolicies) */

  /* adjust the walltime up to the new walltime limit */

  snprintf(tmpLine,sizeof(tmpLine),"walltime range %ld-%ld adjusted to %ld at job start",
    J->OrigMinWCLimit,
    J->OrigWCLimit,
    NewWCLimit);

  if (MJobAdjustWallTime(
      J,
      J->WCLimit,        /* I (minimum new walltime) */
      NewWCLimit,        /* I (maximum new walltime) */
      TRUE,              /* I ModfyRM*/
      FALSE,             /* I allow preemption of reservations (NYI) */
      TRUE,              /* I (fail if walltime modification results in conflict) */
      FALSE,             /* I (increase walltime by MinWallTime increments) */
      EMsg) == FAILURE)
    {
    MDB(4,fSCHED) MLog("INFO:     job '%s' adjust walltime failed - '%s'\n",
      J->Name,
      EMsg);

    return(FAILURE);
    }

  MDB(7,fSCHED) MLog("INFO:     job '%s' in partition %s adjust walltime from %ld to %ld\n",
    J->Name,
    P->Name,
    J->MinWCLimit,
    NewWCLimit);

 /* JobExtendStartWallTime uses the wall time range only at job start so 
 *      so remove the range now that we have used it. 
 *           Note that this feature cannot be used with jobextendduration */

  J->OrigMinWCLimit = 0;
  J->MinWCLimit = 0;

  J->SpecWCLimit[0] = J->WCLimit;
  J->SpecWCLimit[1] = J->WCLimit;

  bmset(&J->Flags,mjfExtendStartWallTime);
  bmset(&J->SpecFlags,mjfExtendStartWallTime);

  MMBAdd(&J->MessageBuffer,tmpLine,NULL,mmbtNONE,0,0,NULL);

  if (J->RMXString != NULL)
    {
    tmpPtr = J->RMXString;

    if ((J->RMXString[0] == 'x') && (J->RMXString[1] == '='))
      tmpPtr += 2;  

    MUStrRemoveFromList(tmpPtr,"minwclimit",';',TRUE);
    }

  if ((J->RMXString != NULL) && 
      ((tmpPtr = strstr(J->RMXString,"MINWCLIMIT")) != NULL))
    {
    MUStrRemoveFromList(tmpPtr,"MINWCLIMIT",'?',TRUE);
    }

  return(SUCCESS);
  } /* END MJobExtendStartWallTime() */

/* END MJobExtendStartWallTime.c */
