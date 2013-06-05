/* HEADER */

/**
 * @file MVMXML.c
 * 
 * Contains various functions for VM to/from XML operations
 *
 */


#include "moab.h"
#include "moab-proto.h"
#include "moab-const.h"
#include "moab-global.h"


/**
 * Report VM as XML
 *
 * @see designdoc VMManagementDoc
 * @see MNodeToXML() - peer
 * @see MVMAToString() - child
 *
 * @param V (I)
 * @param VE (I/O) [modified,alloc]
 * @param SAList (I) [optional,terminated w/mvmaLAST]
 */

int MVMToXML(

  mvm_t            *V,      /* I */
  mxml_t           *VE,     /* I/O (modified,alloc) */
  enum MVMAttrEnum *SAList) /* I (optional,terminated w/mvmaLAST) */

  {
  char tmpString[MMAX_LINE];

  enum MVMAttrEnum *AList;  /**< VM attribute list */

  int aindex;

  mbitmap_t BM;

  const enum MVMAttrEnum DAList[] = {
    mvmaActiveOS,
    mvmaADisk,
    mvmaAlias,
    mvmaAMem,
    mvmaAProcs,
    mvmaCDisk,
    mvmaCMem,
    mvmaCProcs,
    mvmaCPULoad,
    mvmaContainerNode,
    mvmaDDisk,
    mvmaDMem,
    mvmaDProcs,
    mvmaDescription,
    mvmaEffectiveTTL,
    mvmaFlags,
    mvmaGMetric,
    mvmaID,
    mvmaJobID,
    mvmaLastMigrateTime,
    mvmaLastSubState,
    mvmaLastSubStateMTime,
    mvmaLastUpdateTime,
    mvmaMigrateCount,
    mvmaNetAddr,
    mvmaNextOS,
    mvmaOSList,
    mvmaPowerIsEnabled,
    mvmaPowerState,
    mvmaProvData,
    mvmaRackIndex,
    mvmaSlotIndex,
    mvmaSovereign,
    mvmaSpecifiedTTL,
    mvmaStartTime,
    mvmaState,
    mvmaSubState,
    mvmaStorageRsvNames,
    mvmaVariables,
    mvmaLAST };

  if ((V == NULL) || (VE == NULL))
    {
    return(FAILURE);
    }
 
  if (SAList != NULL)
    AList = SAList;
  else
    AList = (enum MVMAttrEnum *)DAList;

  bmset(&BM,mcmXML);

  for (aindex = 0;AList[aindex] != mvmaLAST;aindex++)
    {
    if ((MVMAToString(V,NULL,AList[aindex],tmpString,sizeof(tmpString),&BM) == FAILURE) ||
        (tmpString[0] == '\0'))
      {
      continue;
      }

    if (AList[aindex] == mvmaVariables)
      {
      MUAddVarsToXML(VE,&V->Variables);

      continue;
      }

    MXMLSetAttr(VE,(char *)MVMAttr[AList[aindex]],tmpString,mdfString);
    }  /* END for (aindex) */

  /* Set GEvents */

  MGEventListToXML(VE,&V->GEventsList);

  MVMStorageMountsToXML(VE,V);

  /* Triggers */

  if (V->T != NULL)
    {
    mtrig_t *T;

    int tindex;
    mxml_t *TE = NULL;

    for (tindex = 0;tindex < V->T->NumItems;tindex++)
      {
      T = (mtrig_t *)MUArrayListGetPtr(V->T,tindex);

      if (MTrigIsReal(T) == FALSE)
        continue;

      TE = NULL;

      MXMLCreateE(&TE,(char *)MXO[mxoTrig]);

      MTrigToXML(T,TE,NULL);

      MXMLAddE(VE,TE);
      }
    } /* END if (V->T != NULL) */

  if (V->Action != NULL)
    {
    mln_t  *tmpL = NULL;
    mjob_t *J;

    mxml_t *AE;

    /* report pending actions */

    for (tmpL = V->Action;tmpL != NULL;tmpL = tmpL->Next)
      {
      if ((MJobFind(tmpL->Name,&J,mjsmExtended) == FAILURE) ||
          (J->System == NULL))
        {
        continue;
        }

      AE = NULL;

      MXMLCreateE(&AE,"action");

      MXMLSetAttr(AE,"id",(void **)tmpL->Name,mdfString);

      MXMLSetAttr(AE,"type",(void **)MSysJobType[J->System->JobType],mdfString);

      if (MJOBISACTIVE(J) == TRUE)
        MXMLSetAttr(AE,"starttime",(void **)&J->StartTime,mdfLong);
      else if (J->Rsv != NULL)
        MXMLSetAttr(AE,"starttime",(void **)&J->Rsv->StartTime,mdfLong);

      MXMLSetAttr(AE,"duration",(void **)&J->SpecWCLimit[0],mdfLong);

      switch (J->System->JobType)
        {
        case msjtVMMigrate:

          if (!MNLIsEmpty(&J->Req[0]->NodeList))
            {
            MXMLSetAttr(AE,"destination",(void **)MNLGetNodeName(&J->Req[0]->NodeList,0,NULL),mdfString);
            }
          
          break;

        case msjtOSProvision:

          MXMLSetAttr(AE,"os",(void **)MAList[meOpsys][J->System->ICPVar1],mdfString);

          break;

#if 0
        case msjtVMCreate:

          if (J->System->Action != NULL) 
            {
            char *ptr;

            if ((ptr = strstr(J->System->Action,"create:")) != NULL)
              {
              /* report new VM configuration */

              ptr += strlen("create:");

              MXMLSetAttr(AE,"config",(void **)ptr,mdfString);
              }
            }

          break;
#endif

        default:

          /* NO-OP */

          break;
        }  /* END switch (J->System->JobType) */

      MXMLAddE(VE,AE);
      }  /* END for (tmpL = V->Action) */
    }    /* END if ((V->Action != NULL) && (V->Action[0] != NULL)) */

  /* VMTracking job */

  if (V->TrackingJ != NULL)
    {
    MXMLSetAttr(VE,"TrackingJob",(void **)V->TrackingJ->Name,mdfString);
    }

  return(SUCCESS);
  }  /* END MVMToXML() */


/**
 * MVMFromXML
 *
 *@param VM - (I) VM, modified
 *@param  E - (I) XML description
 */

int MVMFromXML(

  mvm_t  *VM,
  mxml_t *E)

  {
  int aindex;

  enum MVMAttrEnum vaindex;

  if ((VM == NULL) || (E == NULL))
    {
    return(FAILURE);
    }

  for (aindex = 0;aindex < E->ACount;aindex++)
    {
    vaindex = (enum MVMAttrEnum)MUGetIndex(E->AName[aindex],MVMAttr,FALSE,0);

    if (vaindex == mvmaNONE)
      continue;

    switch (vaindex)
      {
      case mvmaVariables:
      case mvmaMigrateCount:
      case mvmaLastMigrateTime:
      case mvmaLastSubState:
      case mvmaLastSubStateMTime:
      case mvmaStorageRsvNames:
      case mvmaEffectiveTTL:
      case mvmaSpecifiedTTL:
      case mvmaStartTime:

        MVMSetAttr(VM,vaindex,(void **)E->AVal[aindex],mdfString,mSet);

        break;

      case mvmaAlias:

        {
        /* Aliases are comma delimited in the checkpoint file
            MVMSetAttr expects one at a time */

        char tmpLine[MMAX_LINE];
        char *ptr;
        char *TokPtr;

        MUStrCpy(tmpLine,E->AVal[aindex],sizeof(tmpLine));

        ptr = MUStrTok(tmpLine,",",&TokPtr);

        while (ptr != NULL)
          {
          MVMSetAttr(VM,vaindex,(void **)ptr,mdfString,mAdd);

          ptr = MUStrTok(NULL,",",&TokPtr);
          }
        } /* END BLOCK mvmaAlias */

        break;

      default:

        MVMSetAttr(VM,vaindex,(void **)E->AVal[aindex],mdfString,mSet);

        break;
      }
    } /* END for (aindex = 0;...) */

  /* Parse remaining children (triggers, storage, variables, etc.) */

  for (aindex = 0;aindex < E->CCount;aindex++)
    {
    if (!strcmp(E->C[aindex]->Name,"trig"))
      {
      mtrig_t *TPtr = NULL;

      TPtr = (mtrig_t *)MUMalloc(sizeof(mtrig_t));

      memset(TPtr,0,sizeof(mtrig_t));
      bmset(&TPtr->InternalFlags,mtifIsAlloc);

      MTrigFromXML(TPtr,E->C[aindex]);
      MTrigInitialize(TPtr);
      }
    else if (!strcasecmp(E->C[aindex]->Name,"Variables"))
      {
      MVMVarsFromXML(VM,E->C[aindex]);
      }
    else if (!strcasecmp(E->C[aindex]->Name,"action"))
      {
      MVMActionFromXML(VM,E->C[aindex]);
      }
    else
      {
      /* NOTE: function will check to make sure child is of type "storage" */

      MVMStorageMountFromXML(VM,E->C[aindex]);
      }
    }

  return(SUCCESS);
  } /* END MVMFromXML() */




/**
 * Examines VM and returns any failures or messages in a "failuredetails"
 * XML child. 
 *
 * @param V (I) [optional]
 * @param VMID (I) [optional]
 * @param FDE (O)
 */

int MVMFailuresToXML(

  mvm_t   *V,
  char    *VMID,
  mxml_t **FDE)

  {
  mxml_t *GEE = NULL;

  char             *GEName;
  mgevent_obj_t    *GEvent;
  mgevent_iter_t    GEIter;

  if (FDE == NULL)
    return(FAILURE);

  if ((V == NULL) &&
      (VMID == NULL))
    {
    /* at least one of these must be valid */

    return(FAILURE);
    }

  if (V == NULL)
    {
    if (MVMFind(VMID,&V) == FAILURE)
      return(FAILURE);
    }

  /* cycle through generic events and create failuredetails XML
   * if one is found! */
  MGEventIterInit(&GEIter);

  while (MGEventItemIterate(&V->GEventsList,&GEName,&GEvent,&GEIter) == SUCCESS)
    {
    if (GEvent->GEventMTime <= 0)
      continue;

    MXMLCreateE(&GEE,"failuredetails");

    /*  Not currently part of the spec, but may soon be
    MXMLSetAttr(GEE,"name",(void *)GEName,mdfString);
    MXMLSetAttr(GEE,"time",(void *)&GEvent->GEventMTime,mdfLong);
    MXMLSetAttr(GEE,"vmid",(void *)V->VMID,mdfString);
    */

    MXMLSetVal(GEE,(void *)GEvent->GEventMsg,mdfString);

    break;
    } /* END while (MUHTIterate(&V->GEvents...) == SUCCESS) */

  if (GEE == NULL)
    {
    return(FAILURE);
    }

  *FDE = GEE;  

  return(SUCCESS);
  }  /* END MVMFailuresToXML() */




/**
 * Analyzes VM objects and converts them to pending action XML if
 * an action is being performed on them.
 *
 * @see MSysQueryPendingActionToXML() - parent
 *
 * @param V (I)
 * @param VEP (O) [optional]
 */

int MVMToPendingActionXML(

  mvm_t   *V,    /* I */
  mxml_t **VEP)  /* O (alloc) */

  {
  char tmpLine[MMAX_LINE];

  mxml_t  *PE;
  mxml_t  *CE;
  mulong JDuration = 0;
  mbool_t  GEventsSpecified = FALSE;

  enum MSystemJobTypeEnum PendingActionType = msjtNONE;
  char PendingActionTypeStr[MMAX_LINE];

  if (VEP == NULL)
    {
    return(FAILURE);
    }

  *VEP = NULL;

  if (V == NULL)
    {
    return(FAILURE);
    }

  PE = NULL;

  /* See if there are any GEvents */

  if (MGEventGetItemCount(&V->GEventsList) > 0)
    {
    GEventsSpecified = TRUE;
    }

  /* NOTE:  do not change pending actions strings w/o coordinating with MCM 
            team */

  if ((V->NextOS != 0) &&
      (V->NextOS != V->ActiveOS))
    {
    PendingActionType = msjtOSProvision;
    MUStrCpy(PendingActionTypeStr,(char *)MPendingActionType[mpatVMOSProvision],sizeof(PendingActionTypeStr));
    }
  else if (((V->PowerSelectState == mpowOn) || (V->PowerSelectState == mpowOff)) &&
            (V->PowerState != V->PowerSelectState) &&
            (MSched.Time - V->PowerMTime < MCONST_HOURLEN))
    {
    if (V->PowerSelectState == mpowOn)
      {
      PendingActionType = msjtPowerOn;
      MUStrCpy(PendingActionTypeStr,(char *)MPendingActionType[mpatVMPowerOn],sizeof(PendingActionTypeStr));
      }
    else if (V->PowerSelectState == mpowOff)
      {
      PendingActionType = msjtPowerOff;
      MUStrCpy(PendingActionTypeStr,(char *)MPendingActionType[mpatVMPowerOff],sizeof(PendingActionTypeStr));
      }
    }
  else
    {
    PendingActionType = msjtNONE;
    }

  if (PendingActionType == msjtNONE)
    {
    /* has no action on it to report as a pending action */

    return(FAILURE);
    }

    /* create common elements */

  if (MXMLCreateE(&PE,(char *)MXO[mxoxPendingAction]) == FAILURE)
    {
    return(FAILURE);
    }

  snprintf(tmpLine,sizeof(tmpLine),"%s-%ld",
    V->VMID,
    V->LastOSModRequestTime);

  MXMLSetAttr(PE,(char *)MPendingActionAttr[mpaaPendingActionID],(void *)tmpLine,mdfString);
  MXMLSetAttr(PE,(char *)MPendingActionAttr[mpaaSched],(void *)MSched.Name,mdfString);

  if (PendingActionType != msjtNONE)
    {
    MXMLSetAttr(PE,(char *)MPendingActionAttr[mpaaType],(void *)PendingActionTypeStr,mdfString);

    if (V->LastOSModRequestTime > 0)
      {
      /* Viewpoint only wants positive starttimes reported */

      MXMLSetAttr(PE,(char *)MPendingActionAttr[mpaaStartTime],(void *)&V->LastOSModRequestTime,mdfLong);
      }

    if (MSched.DefProvDuration > 0)
      JDuration = MSched.DefProvDuration;
    else
      JDuration = MDEF_PROVDURATION;

    MXMLSetAttr(PE,(char *)MPendingActionAttr[mpaaMaxDuration],(void *)&JDuration,mdfLong);

    if (V->N != NULL)
      MXMLSetAttr(PE,(char *)MPendingActionAttr[mpaaHostlist],(void *)V->N->Name,mdfString);

    MXMLSetAttr(PE,(char *)MPendingActionAttr[mpaaState],(void *)MJobState[mjsRunning],mdfString);

    if (V->LastSubState != NULL)
      {
      MXMLSetAttr(PE,(char *)MPendingActionAttr[mpaaSubState],(void *)V->LastSubState,mdfString);
      }

    /* now set action specific data */
  
    switch (PendingActionType)
      {
      case msjtOSProvision:
  
        /* handle re-provisioning of VM's */
  
        MXMLAddChild(PE,PendingActionTypeStr,NULL,&CE);
        MXMLSetAttr(CE,(char *)MPendingActionAttr[mpaaVMID],(void *)V->VMID,mdfString);
        MXMLSetAttr(CE,(char *)MPendingActionAttr[mpaaTargetOS],(void *)MAList[meOpsys][V->NextOS],mdfString);
  
        break;
  
      case msjtPowerOn:
      case msjtPowerOff:
  
        /* handle start/stop */
  
        MXMLAddChild(PE,PendingActionTypeStr,NULL,&CE);
        MXMLSetAttr(CE,(char *)MPendingActionAttr[mpaaVMID],(void *)V->VMID,mdfString);
  
        break;
  
      default:
  
        /* NO-OP */
  
        break;
      }  /* END switch (PendingActionType) */
    } /* END if (PendingActionType != msjtNONE) */

  if (GEventsSpecified != FALSE)
    {
    mxml_t *GEE;

    if (MVMFailuresToXML(V,NULL,&GEE) == SUCCESS)
      MXMLAddE(PE,GEE);
    } /* END if (GEventsSpecified != FALSE) */

  *VEP = PE;

  return(SUCCESS);
  }  /* END MVMToPendingActionXML() */


/**
 * Convert node VM list to XML.
 *
 * @see MVMToXML() - child
 * @see MNodeToXML() - parent
 *
 * @param N (I)
 * @param NE (I/O) [modified,alloc]
 */

int MNodeVMListToXML(

  mnode_t *N,  /* I */
  mxml_t  *NE) /* I/O (modified,alloc) */

  {
  mln_t  *P;  /* VM list pointer */
  mvm_t  *V;
  mxml_t *VE;

  if ((N == NULL) || (NE == NULL))
    {
    return(FAILURE);
    }

  for (P = N->VMList;P != NULL;P = P->Next)
    {
    V = (mvm_t *)P->Ptr;

    if (V != NULL)
      {
      VE = NULL;

      MXMLCreateE(&VE,(char *)MXO[mxoxVM]);

      MVMToXML(V,VE,NULL);

      MXMLAddE(NE,VE);
      }
    }  /* END for (P) */

  return(SUCCESS);
  }  /* END MNodeVMListToXML() */

/* END MVMXML.c */
