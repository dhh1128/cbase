/* HEADER */

      
/**
 * @file MJobTriggers.c
 *
 * Contains: MJobCopyTrigs MJobWorkflowCreateTriggers
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 * Copy triggers from SourceJob to DestJob.
 * 
 * It updates all relevant information like job pointer, OID name, etc.
 * Much was taken from MJobApplyDefTemplate().
 * 
 * @param SourceJob (I) The job that triggers that will be copied from
 * @param DestJob (I) [modified] The job that triggers will be copied to.
 * @param CopyGeneric (I) [boolean] Copy the "Generic" job triggers (J->T)
 * @param CopyProlog (I) [boolean] Copy the "Prolog" job triggers (J->Prolog)
 * @param CopyEpilog (I) [boolean] Copy the "Epilog" job triggers (J->Epilog)
 */

int MJobCopyTrigs(

  const mjob_t  *SourceJob,
  mjob_t        *DestJob,
  mbool_t        CopyGeneric,
  mbool_t        CopyProlog,
  mbool_t        CopyEpilog)

  {
  /* First copy the generic trigger */

  if ((CopyGeneric == TRUE) && (SourceJob->Triggers != NULL))
    {
    mtrig_t *T;

    int tindex;

    MTListCopy(SourceJob->Triggers,&DestJob->Triggers,TRUE);

    for (tindex = 0;tindex < DestJob->Triggers->NumItems;tindex++)
      {
      T = (mtrig_t *)MUArrayListGetPtr(DestJob->Triggers,tindex);

      if (MTrigIsValid(T) == FALSE)
        continue;

      /* update trigger OID with new RID */

      T->O = (void *)DestJob;
      T->OType = mxoJob;

      MUStrDup(&T->OID,DestJob->Name);

      MTrigInitialize(T);

      /* register trigger in global table */

      if ((T->EType == mttHold) && (bmisset(&DestJob->IFlags,mjifNewBatchHold)))
        {
        T->ETime = MSched.Time;
        }
      }      /* END for (tindex) */
    }        /* END if (DestJob->T != NULL) */

  /* Now copy the Prolog triggers */

  if ((CopyProlog == TRUE) && (SourceJob->PrologTList != NULL))
    {
    mtrig_t *T;

    int tindex;

    MTListCopy(SourceJob->PrologTList,&DestJob->PrologTList,FALSE);

    bmset(&DestJob->IFlags,mjifProlog);

    for (tindex = 0;tindex < DestJob->PrologTList->NumItems;tindex++)
      {
      T = (mtrig_t *)MUArrayListGetPtr(DestJob->PrologTList,tindex);

      if (MTrigIsValid(T) == FALSE)
        continue;

      /* update trigger OID with new RID */

      T->O = (void *)DestJob;
      T->OType = mxoJob;

      MUStrDup(&T->OID,DestJob->Name);
      T->ETime = MSched.Time;
      MUFree(&T->TName);
      MUStrDup(&T->TName,DestJob->Name);

      if ((T->EType == mttHold) &&
           bmisset(&DestJob->IFlags,mjifNewBatchHold))
        {
        T->ETime = MSched.Time;
        }
      }      /* END for (tindex) */
    }        /* END if (DestJob->PrologTList != NULL) */

  /* Now copy the Epilog triggers */

  if ((CopyEpilog == TRUE) && (SourceJob->EpilogTList != NULL))
    {
    mtrig_t *T;

    int tindex;

    MTListCopy(SourceJob->EpilogTList,&DestJob->EpilogTList,FALSE);

    for (tindex = 0; tindex < DestJob->EpilogTList->NumItems;tindex++)
      {
      T = (mtrig_t *)MUArrayListGetPtr(DestJob->EpilogTList,tindex);

      if (MTrigIsValid(T) == FALSE)
        continue;

      /* update trigger OID with new RID */

      T->O = (void *)DestJob;
      T->OType = mxoJob;

      MUStrDup(&T->OID,DestJob->Name);

      if ((T->EType == mttHold) &&
           bmisset(&DestJob->IFlags,mjifNewBatchHold))
        {
        T->ETime = MSched.Time;
        }
      }      /* END for (tindex) */
    }        /* END if (DestJob->Epilog != NULL) */

  return(SUCCESS);
  }  /* END MJobCopyTrigs() */





/**
 * Create workflow triggers on master job.
 *
 * @param J  (can be any member of the workflow - optional)
 * @param MJ (master job of workflow - optional)
 */

int MJobWorkflowCreateTriggers(

  mjob_t *J,
  mjob_t *MJ)

  {
  int jindex;
  int dindex;
  int MaxDepth = MMAX_WORKFLOWJOBCOUNT;


  mjworkflow_t  WList[MMAX_WORKFLOWJOBCOUNT];

  mjob_t *MasterJ = NULL;
  mxml_t *DE      = NULL;

  if ((J == NULL) && (MasterJ == NULL))
    {
    return(FAILURE);
    }

  if (MJ == NULL)
    {
    if (J->JGroup == NULL)
      {
      return(FAILURE);
      }

    if (MJobFind(J->JGroup->Name,&MasterJ,mjsmExtended) == FAILURE)
      {
      return(FAILURE);
      }
    }
  else
    {
    MJ = MasterJ;
    }

  if ((MasterJ == NULL) || 
      (MasterJ->TemplateExtensions == NULL) || 
      (MasterJ->TemplateExtensions->FailurePolicy == mjtfpNONE))
    {
    return(FAILURE);
    }

  if (MXMLCreateE(&DE,(char *)MSON[msonData]) == FAILURE)
    {
    return(FAILURE);
    }

  for (jindex = 0; jindex < MMAX_WORKFLOWJOBCOUNT; jindex++)
    {
    WList[jindex].J = NULL;

    for (dindex = 0; dindex < MMAX_WCHILD; dindex++)
      {
      WList[jindex].ChildIndex[dindex] = -1;
      }
    }

  /* get list of all jobs in workflow */

  __MUIJobQueryWorkflowRecursive(
    J,
    -1,
    WList,
    NULL,
    MaxDepth,
    DE);

  if (WList[0].J == NULL)
    {
    return(FAILURE);
    }

  mstring_t String(MMAX_LINE);

  switch (MasterJ->TemplateExtensions->FailurePolicy)
    {
    case mjtfpCancel:

      /* 1 trigger: 1) cancel all jobs in job group if variable FAILURE=TRUE */

      String.clear();   /* Reset string */

      MStringAppendF(&String,"atype=internal,requires=ccode:ne:0,etype=start,action=jgroup:-:cancel");

      MTrigLoadString(&MasterJ->Triggers,String.c_str(),TRUE,FALSE,mxoJob,MasterJ->Name,NULL,NULL);

      break;

    case mjtfpRetry:

      /* 2 triggers: 1) cancels all jobs in job group if variable FAILURE=TRUE 
                     2) resubmit job to template */

      /* atype=internal,requires=FAILURE,etype=start,action=job:-:cancel,sets=cancelled */
      /* atype=internal,requires=cancelled,etype=start,action=job:-:submit */

      break;

    case mjtfpRetryStep:

      for (jindex = 0;WList[jindex].J != NULL;jindex++)
        {
        bmset(&WList[jindex].J->IFlags,mjifRequeueOnFailure);
        }    /* END for (jindex) */

      break;

    case mjtfpHold:

      /* 1 trigger: 1) hold all jobs in job group if variable FAILURE=TRUE */

      /* atype=internal,requires=FAILURE,etype=start,action=job:-:hold */

      String.clear();   /* Reset string */

      MStringAppendF(&String,"atype=internal,requires=ccode:ne:0,etype=start,action=jgroup:-:modify:hold=user,sets=HOLD");
      MTrigLoadString(&MasterJ->Triggers,String.c_str(),TRUE,FALSE,mxoJob,MasterJ->Name,NULL,NULL);
      String.clear();   /* Reset string */

      MStringAppendF(&String,"atype=internal,requires=HOLD,etype=start,action=job:-:requeue,sets=REQUEUED");
      MTrigLoadString(&MasterJ->Triggers,String.c_str(),TRUE,FALSE,mxoJob,MasterJ->Name,NULL,NULL);
      String.clear();   /* Reset string */

      MStringAppendF(&String,"atype=internal,requires=REQUEUED,etype=start,action=job:-:hold");
      MTrigLoadString(&MasterJ->Triggers,String.c_str(),TRUE,FALSE,mxoJob,MasterJ->Name,NULL,NULL);
      String.clear();   /* Reset string */

      break;

    default:

      return(FAILURE);

      /*NOTREACHED*/

      break;
    }  /* END switch (MasterJ->TX->FailurePolicy) */

  if (MasterJ->Triggers != NULL)
    {
    mtrig_t *T;

    int tindex;

    for (tindex = 0; tindex < MasterJ->Triggers->NumItems;tindex++)
      {
      T = (mtrig_t *)MUArrayListGetPtr(MasterJ->Triggers,tindex);

      if (MTrigIsValid(T) == FALSE)
        continue;

      MTrigInitialize(T);
      }  /* END for (tindex) */

    MOReportEvent((void *)MasterJ,MasterJ->Name,mxoJob,mttCreate,MasterJ->CTime,TRUE);
    }    /* END if (MasterJ->T != NULL) */

  return(SUCCESS);
  }  /* END MJobWorkflowCreateTriggers() */
/* END MJobTriggers.c */
