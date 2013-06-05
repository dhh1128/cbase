/* HEADER */

/**
 * @file MVMShow.c
 * 
 * Contains various functions to 'show' VM attributes
 *
 */


#include "moab.h"
#include "moab-proto.h"
#include "moab-const.h"
#include "moab-global.h"




/**
 * Report VM state info (process 'checknode' request).
 *
 * @see MNodeShowState() - parent
 *
 * @param Buffer (O)
 * @param N      (I)
 */

int MVMShowState(

  mstring_t *Buffer,
  mnode_t   *N)

  {
  mln_t *P;
  mvm_t *V;

  if (Buffer == NULL)
    {
    return(FAILURE);
    }

  for (P = N->VMList;P != NULL;P = P->Next)
    {
    V = (mvm_t *)P->Ptr;

    MVMShow(Buffer,V,FALSE,TRUE);
    }  /* END for (P) */

  return(SUCCESS);
  }  /* END MVMShowState() */



/*
 * @See MVMShowStat() - parent
 * @See MUIVMCtl() - parent
 *
 * Shows the information for an individual VM
 *
 * @param BPtr    (I/O) - Buffer to print to
 * @param BSpace  (I/O) - Remaining size
 * @param V       (I)   - VM to show
 * @param Verbose (I)   - Print everything or just a subset
 * @param Indent  (I)   - Indent the first line or not
 */

int MVMShow(

  mstring_t *Buffer, /* O (already initialized) */
  mvm_t     *V,
  mbool_t    Verbose,
  mbool_t    Indent)

  {
  int GIndex;
  mbool_t GMetricWritten = FALSE;

  if ((Buffer == NULL) || (V == NULL))
    {
    return(FAILURE);
    }

  /* Print the VMID, State, JobID, and Active OS */

  MStringAppendF(Buffer,"%sVM[%s]  State: %s  JobID: %s  ActiveOS: %s\n",
    (Indent == TRUE) ? "  " : "",
    V->VMID,
    MNodeState[V->State],
    V->JobID,
    MAList[meOpsys][V->ActiveOS]);

  if (bmisset(&V->Flags,mvmfUtilizationReported))
    {
    MStringAppendF(Buffer,"    Procs: %d/%d (CPULoad: %.2f/%.2f)  Mem: %d/%d MB Location: %d:%d\n",
      V->ARes.Procs,
      V->CRes.Procs,
      V->CPULoad,
      (double)V->LURes.Procs,
      V->ARes.Mem,
      V->CRes.Mem,
      V->Rack,
      V->Slot);
    }
  else
    {
    MStringAppendF(Buffer,"    Procs: %d  Mem: %d MB  Location: %d:%d\n",
      V->CRes.Procs,
      V->CRes.Mem,
      V->Rack,
      V->Slot);
    }

  if ((V->PowerState != mpowNONE) || (V->PowerSelectState != mpowNONE))
    {
    MStringAppendF(Buffer,"    PowerState: %s  PowerSelectState: %s\n",
      MPowerState[V->PowerState],
      MPowerState[V->PowerSelectState]);
    }

  if (V->LastActionDescription != NULL)
    {
    MStringAppendF(Buffer,"    Last operation %s completed %ld seconds ago (duration: %ld) - %s\n",
      MSysJobType[V->LastActionType],
      MSched.Time - V->LastActionEndTime,
      V->LastActionEndTime - V->LastActionStartTime,
      V->LastActionDescription);
    }

  /* CONTAINER NODE */

  if (Verbose)
    {
    if (V->N != NULL)
      {
      MStringAppendF(Buffer,"    Container node: %s\n",
        V->N->Name);
      }
    else
      {
      MStringAppendF(Buffer,"    ALERT: container node is NULL\n");
      }
    }

  /* VARIABLES */

  if (V->Variables.NumItems > 0)
    {
    mhashiter_t HTI;
    mbool_t VarFound = FALSE;
    char *VarName;
    char *VarVal;

    MUHTIterInit(&HTI);

    MStringAppendF(Buffer,"    Variables: ");

    while (MUHTIterate(&V->Variables,&VarName,(void **)&VarVal,NULL,&HTI) == SUCCESS)
      {
      if (VarFound)
        {
        MStringAppendF(Buffer," ");
        }

      MStringAppendF(Buffer,"%s",
        VarName);

      if (VarVal != NULL)
        {
        MStringAppendF(Buffer,"=\"%s\"",
          VarVal);
        }

      VarFound = TRUE;
      }

    MStringAppendF(Buffer,"\n");
    } /* END if (V->Variables.NumItems > 0) */

  /* TTL */

  if (V->EffectiveTTL > 0)
    {
    char TString[MMAX_LINE];
    char NCTime[MMAX_LINE];
    mulong tmpEndTime = V->StartTime + V->EffectiveTTL;

    MULToTString(V->EffectiveTTL,TString);
    MUSNCTime(&tmpEndTime,NCTime);

    MStringAppendF(Buffer,"    TTL: %s   %s%s\n",
      TString,
      (V->StartTime != 0) ? "Endtime: " : "",
      (V->StartTime != 0) ? NCTime : "");
    }

  /* STORAGE */

  if (V->Storage != NULL)
    {
    mln_t *StorePtr = NULL;
    mvm_storage_t *Storage = NULL;

    MStringAppendF(Buffer,"    Storage:\n");

    while (MULLIterate(V->Storage,&StorePtr) == SUCCESS)
      {
      Storage = (mvm_storage_t *)StorePtr->Ptr;

      MStringAppendF(Buffer,"         %s  %s:%d  MountPoint:%s",
        Storage->Name,
        Storage->Type,
        Storage->Size,
        Storage->MountPoint);

      if (!MUStrIsEmpty(Storage->MountOpts))
        {
        MStringAppendF(Buffer," MountOptions:%s",
          Storage->MountOpts);
        }

      MStringAppendF(Buffer,"\n");
      }
    } /* END if (V->Storage != NULL) */

  /* GEVENTS */

  if (MGEventGetItemCount(&V->GEventsList) > 0)
    {
    char            *GEName;
    mgevent_obj_t   *GEvent;
    mgevent_iter_t   GEIter;

    MStringAppendF(Buffer,"    GEvents: ");

    MGEventIterInit(&GEIter);
    while (MGEventItemIterate(&V->GEventsList,&GEName,&GEvent,&GEIter) == SUCCESS)
      {
      MStringAppendF(Buffer,"%s[%ld]='%s'",
        GEName,
        GEvent->GEventMTime,
        GEvent->GEventMsg);

      MStringAppendF(Buffer," ");
      }

    MStringAppendF(Buffer,"\n");
    } /* END if (V->GEvents.NumItems > 0) */

  /* GMETRICS */

  if (V->GMetric != NULL)
    {
    for (GIndex = 1;GIndex < MSched.M[mxoxGMetric];GIndex++)
      {
      if (V->GMetric[GIndex] == NULL)
        continue;

      if (MGMetric.Name[GIndex][0] == '\0')
        break;

      if (V->GMetric[GIndex]->SampleCount <= 0)
        continue;

      if (GMetricWritten == FALSE)
        {
        MStringAppendF(Buffer,"    GMetrics: %s:%.2f\n",
          MGMetric.Name[GIndex],
          V->GMetric[GIndex]->IntervalLoad);
  
        GMetricWritten = TRUE;
        }
      else
        {
        MStringAppendF(Buffer,"              %s:%.2f\n",
          MGMetric.Name[GIndex],
          V->GMetric[GIndex]->IntervalLoad);
        }
      } /* END for (GIndex = 1;... ) */
    } /* END if (V->GMetric != NULL) */

  /* ALIASES */

  if ((V->Aliases != NULL) && (V->Aliases->ListSize > 0))
    {
    mbitmap_t BM;

    char Aliases[MMAX_LINE];

    MStringAppendF(Buffer,"    Aliases: ");

    bmset(&BM,mcmUser);

    MVMAToString(V,NULL,mvmaAlias,Aliases,MMAX_LINE,&BM);
    MStringAppendF(Buffer,"%s",
      Aliases);

    MStringAppendF(Buffer,"\n");
    } /* END if (V->Aliases->ListSize > 0) */

  /* TRIGGERS */

  if ((Verbose) && (V->T != NULL))
    {
    mtrig_t *T;

    int tindex;

    char tmpTrig[MMAX_LINE];

    MStringAppendF(Buffer,"    Triggers:\n");

    for (tindex = 0;tindex < V->T->NumItems;tindex++)
      {
      T = (mtrig_t *)MUArrayListGetPtr(V->T,tindex);

      if (MTrigIsReal(T) == FALSE)
        continue;

      MTrigToAVString(T,',',tmpTrig,MMAX_LINE);

      MStringAppendF(Buffer,"       %s\n",
        tmpTrig);
      }
    } /* END if ((Verbose) && (V->T != NULL)) */

  if (V->TrackingJ != NULL)
    {
    MStringAppendF(Buffer,"    TrackingJob: %s\n",
      V->TrackingJ->Name);
    }

  return(SUCCESS);
  } /* END MVMShow() */

/* END MVMShow.c */
