/* HEADER */

/**
 * @file MNodeTransition.c
 *
 * Contains Node Transition functions
 */



#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  
#include "MBSON.h"

/**
 * Get a string for the given node attribute
 *
 * @see MNodeAToString (sibling)
 *
 * @param N       (I) the node transition object
 * @param AIndex  (I) the attribute we want a string for
 * @param Buf     (O) the output buffer for the string we return
 */

int MNodeTransitionAToString(

  mtransnode_t       *N,        /* I */
  enum MNodeAttrEnum  AIndex,   /* I */
  mstring_t          *Buf)      /* O */

  {
  if ((N == NULL) || (Buf == NULL))
    {
    return(FAILURE);
    }

  MDB(8,fSCHED) MLog("INFO:     Getting string value for node transition '%s' attribute '%s'.\n",
    N->Name,
    MNodeAttr[AIndex]);

  switch(AIndex)
    {
    case mnaNodeID:

      MStringSet(Buf,N->Name);

      break;

    case mnaArch:

      if (!MUStrIsEmpty(N->Architecture))
        {
        MStringSet(Buf,N->Architecture);
        }

      break;

    case mnaAvlGRes:

      if (!MUStrIsEmpty(N->AvailGRes))
        {
        MStringSet(Buf,N->AvailGRes);
        }

      break;

    case mnaCfgGRes:

      if (!MUStrIsEmpty(N->ConfigGRes))
        {
        MStringSet(Buf,N->ConfigGRes);
        }

      break;

    case mnaAvlClass:

      if (!MUStrIsEmpty(N->AvailClasses))
        {
        MStringSet(Buf,N->AvailClasses);
        }

      break;

    case mnaCfgClass:

      if (!MUStrIsEmpty(N->ConfigClasses))
        {
        MStringSet(Buf,N->ConfigClasses);
        }

      break;

    case mnaChargeRate:

      MStringSetF(Buf,"%.3f",N->ChargeRate);

      break;

    case mnaDedGRes:

      if (!MUStrIsEmpty(N->DedGRes))
        {
        MStringSet(Buf,N->DedGRes);
        }

      break;

    case mnaDynamicPriority:

      MStringSetF(Buf,"%.3f",N->DynamicPriority);

      break;

    case mnaEnableProfiling:

      if (N->EnableProfiling != MBNOTSET)
        {
        MStringSetF(Buf,"%s",MBool[N->EnableProfiling]);
        }

      break;

    case mnaFeatures:

      if (!MUStrIsEmpty(N->Features))
        {
        MStringSet(Buf,N->Features);
        }

      break;

    case mnaFlags:

      if (!MUStrIsEmpty(N->Flags))
        {
        MStringSet(Buf,N->Flags);
        }

      break;

    case mnaGMetric:

      if (!MUStrIsEmpty(N->GMetric.c_str()))
        {
        MStringSet(Buf,N->GMetric.c_str());
        }

      break;

    case mnaHopCount:

      MStringSetF(Buf,"%d",N->HopCount);

      break;

    case mnaHVType:

      if (N->HypervisorType != mhvtNONE)
        {
        MStringSet(Buf,(char *)MHVType[N->HypervisorType]);
        }

      break;

    case mnaIsDeleted:

      if (N->IsDeleted != MBNOTSET)
        {
        MStringSetF(Buf,"%s",MBool[N->IsDeleted]);
        }

      break;

    case mnaJobList:

      if (!MUStrIsEmpty(N->JobList))
        {
        MStringSet(Buf,N->JobList);
        }

      break;

    case mnaLastUpdateTime:

      MStringSetF(Buf,"%ld",N->LastUpdateTime);

      break;

    case mnaLoad:

      if (N->LoadAvg > 0.0)
        {
        MStringSetF(Buf,"%.6f",N->LoadAvg);
        }

      break;

    case mnaMaxLoad:

      MStringSetF(Buf,"%.6f",N->MaxLoad);

      break;

    case mnaMaxJob:

      MStringSetF(Buf,"%d",N->MaxJob);

      break;

    case mnaMaxJobPerUser:

      MStringSetF(Buf,"%d",N->MaxJobPerUser);

      break;

    case mnaMaxProc:

      if (N->MaxProc > 0)
        {
        MStringSetF(Buf,"%d",N->MaxProc);
        }

      break;

    case mnaMaxProcPerUser:

      if (N->MaxProcPerUser > 0)
        {
        MStringSetF(Buf,"%d",N->MaxProcPerUser);
        }

      break;

    case mnaMessages:

      if (!MUStrIsEmpty(N->Messages))
        {
        MStringSet(Buf,N->Messages);
        }

      break;

    case mnaOldMessages:

      if (!MUStrIsEmpty(N->OldMessages))
        {
        MStringSet(Buf,N->OldMessages);
        }

      break;

    case mnaNetAddr:

      if (!MUStrIsEmpty(N->NetworkAddress))
        {
        MStringSet(Buf,N->NetworkAddress);
        }

      break;

    case mnaNodeIndex:

      MStringSetF(Buf,"%d",N->NodeIndex);

      break;

    case mnaNodeState:

      if (N->State != mnsNONE)
        {
        MStringSet(Buf,(char *)MNodeState[N->State]);
        }

      break;

    /*
    mnaNodeSubState, -- FIXME don't have...
    */

    case mnaOperations:

      if (!MUStrIsEmpty(N->Operations))
        {
        MStringSet(Buf,N->Operations);
        }

      break;

    case mnaOS:

      /* need to have something here, but not the [NONE] that comes from
       * MNodeAToString when there's no OS specified */

      if ((!MUStrIsEmpty(N->OperatingSystem)) &&
          (strcmp(N->OperatingSystem,"[NONE]") != 0))
        {
        MStringSet(Buf,N->OperatingSystem);
        }

      break;

    case mnaOSList:

      if (!MUStrIsEmpty(N->OSList))
        {
        MStringSet(Buf,N->OSList);
        }

      break;

    case mnaOwner:

      if (!MUStrIsEmpty(N->Owner))
        {
        MStringSet(Buf,N->Owner);
        }

      break;

    case mnaResOvercommitFactor:
    case mnaAllocationLimits:

      if (!MUStrIsEmpty(N->ResOvercommitFactor))
        {
        MStringSet(Buf,N->ResOvercommitFactor);
        }

      break;

    case mnaResOvercommitThreshold:
    case mnaUtilizationThresholds:

      if (!MUStrIsEmpty(N->ResOvercommitThreshold))
        {
        MStringSet(Buf,N->ResOvercommitThreshold);
        }

      break;

    case mnaPartition:

      if (!MUStrIsEmpty(N->Partition))
        {
        MStringSet(Buf,N->Partition);
        }

      break;

    case mnaPowerIsEnabled:

      if (N->PowerIsEnabled != MBNOTSET)
        {
        MStringSetF(Buf,"%s",MBool[N->PowerIsEnabled]);
        }

      break;

    case mnaPowerPolicy:

      if (N->PowerPolicy != mpowpNONE)
        {
        MStringSet(Buf,(char *)MPowerPolicy[N->PowerPolicy]);
        }

      break;

    case mnaPowerSelectState:

      if (N->PowerSelectState != mpowNONE)
        {
        MStringSet(Buf,(char *)MPowerState[N->PowerSelectState]);
        }

      break;

    case mnaPowerState:

      if (N->PowerState != mpowNONE)
        {
        MStringSet(Buf,(char *)MPowerState[N->PowerState]);
        }

      break;

    case mnaPriority:

      if ((N->Priority == 0) && (MSched.DefaultN.PrioritySet == TRUE))
        {
        /* if a node's priority is not set use the default node priority */
        MStringSetF(Buf,"%d",MSched.DefaultN.Priority);
        }
      else
        MStringSetF(Buf,"%d",N->Priority);

      break;

    case mnaPrioF:

      if (!MUStrIsEmpty(N->PriorityFunction))
        {
        MStringSet(Buf,N->PriorityFunction);
        }

      break;

    case mnaProcSpeed:

      MStringSetF(Buf,"%d",N->ProcessorSpeed);

      break;

    case mnaProvData:

      if (!MUStrIsEmpty(N->ProvisioningData))
        {
        MStringSet(Buf,N->ProvisioningData);
        }

      break;

    case mnaRack:

      if (N->Rack >= 0)
        {
        MStringSetF(Buf,"%d",N->Rack);
        }

      break;

    case mnaRADisk:

      MStringSetF(Buf,"%d",N->AvailableDisk);

      break;

    case mnaRAMem:

      MStringSetF(Buf,"%d",N->AvailableMemory);

      break;

    case mnaRAProc:

      MStringSetF(Buf,"%d",N->AvailableProcessors);

      break;

    case mnaRASwap:

      MStringSetF(Buf,"%d",N->AvailableSwap);

      break;

    case mnaRCDisk:

      if (N->ConfiguredDisk > 0)
        {
        MStringSetF(Buf,"%d",N->ConfiguredDisk);
        }

      break;

    case mnaRCMem:

      if (N->ConfiguredMemory > 0)
        {
        MStringSetF(Buf,"%d",N->ConfiguredMemory);
        }

      break;

    case mnaRCProc:

      if (N->ConfiguredProcessors > 0)
        {
        MStringSetF(Buf,"%d",N->ConfiguredProcessors);
        }

      break;

    case mnaRCSwap:

      if (N->ConfiguredSwap > 0)
        {
        MStringSetF(Buf,"%d",N->ConfiguredSwap);
        }

      break;

    case mnaRsvCount:

      if (N->ReservationCount > 0)
        {
        MStringSetF(Buf,"%d",N->ReservationCount);
        }

      break;

    case mnaRsvList:

      if (!MUStrIsEmpty(N->ReservationList.c_str()))
        {
        MStringSet(Buf,N->ReservationList.c_str());
        }

      break;

    case mnaRMList:

      if (!MUStrIsEmpty(N->ResourceManagerList))
        {
        MStringSet(Buf,N->ResourceManagerList);
        }

      break;

    case mnaSize:

      if (N->Size > 0)
        {
        MStringSetF(Buf,"%d",N->Size);
        }

      break;

    case mnaSlot:

      if (N->Slot >= 0)
        {
        MStringSetF(Buf,"%d",N->Slot);
        }

      break;

    case mnaSpeed:

      MStringSetF(Buf,"%.6f",N->Speed);

      break;

    case mnaSpeedW:

      if (N->SpeedWeight > 0)
        {
        MStringSetF(Buf,"%.6f",N->SpeedWeight);
        }

      break;

    case mnaStatATime:

      if (N->TotalNodeActiveTime > 0)
        {
        MStringSetF(Buf,"%ld",N->TotalNodeActiveTime);
        }

      break;

    case mnaStatMTime:

      if (N->LastModifyTime > 0)
        {
        MStringSetF(Buf,"%ld",N->LastModifyTime);
        }

      break;

    case mnaStatTTime:

      if (N->TotalTimeTracked > 0)
        {
        MStringSetF(Buf,"%ld",N->TotalTimeTracked);
        }

      break;

    case mnaStatUTime:

      if (N->TotalNodeUpTime > 0)
        {
        MStringSetF(Buf,"%ld",N->TotalNodeUpTime);
        }

      break;

    case mnaTaskCount:

      if (N->TaskCount > 0)
        {
        MStringSetF(Buf,"%d",N->TaskCount);
        }

      break;

    case mnaVariables:

      /* not handled, shouldn't be done this way as it is XML (see MNodeTransitionToXML for the right way) */

      /* NO-OP */

      break;

    case mnaVMOSList:

      if (!MUStrIsEmpty(N->VMOSList))
        {
        MStringSet(Buf,N->VMOSList);
        }

      break;

    case mnaVarAttr:

      if (!MUStrIsEmpty(N->AttrList.c_str()))
        {
        MStringSet(Buf,N->AttrList.c_str());
        }

      break;

    default:

      /* NO-OP */

      MDB(7,fSCHED) MLog("Warning:  Request for node transition attribute '%s' not supported.\n",MNodeAttr[AIndex]);
      
      break;

    } /* END switch(AIndex) */

  return(SUCCESS);
  } /* END MNodeTransitionAToString() */





/**
 * MNodeTransitionFitsConstraintList
 *
 * check to see if a given mtransnode_t instance matches a constraint
 * found in a given list of constraints.
 *
 * Use the following rules:
 *
 * 1) within each constraint, there can be a list of matches--these
 *    are all OR'd with each other--i.e. a match of any of them results in 
 *    success.
 *
 * 2) if a constraint contains multiple items, they are stored as follows:
 *    a) in an AVal, the items are in a comma separated string in Aval[0]
 *    b) in a ALong, the items are in Along[i] where the list is terminated by
 *       msjNONE if there are fewer than MMAX_JOBCVAL items.
 *
 * 3) When there are more than one constraint in the JConstraintList, the trans-node
 *    must match all of them to ultimately return TRUE--i.e. AND'd between 
 *    individual constraints.
 *
 * @param   N               (I)
 * @param   ConstraintList  (I)
 * @return  TRUE or FALSE
 *          TRUE if the given trans-node matches all constraints in the list
 *          FALSE if the given trans-node fail any one of the contraints in the  list
 */

mbool_t MNodeTransitionFitsConstraintList(

  mtransnode_t *N,
  marray_t     *ConstraintList)

  {
  int                CIndex; /* Constraint index */
  mnodeconstraint_t *Constraint;
  mbool_t            DefaultNodeConstraint = FALSE;
  mbool_t            DisplayOverride = FALSE;
  mbool_t            DisplayAllOverride = FALSE;

  if (N == NULL)
    return(FALSE);

  if (ConstraintList == NULL)
    {
    if (!strcmp(N->Name,"DEFAULT"))
      return(FALSE);
    else
      return(TRUE);
    }

  /* Iterate over the constraint list, matching current node instance */
  for (CIndex = 0; CIndex < ConstraintList->NumItems;CIndex++)
    {
    /* Reference the current constraint */
    Constraint = (mnodeconstraint_t *)MUArrayListGet(ConstraintList,CIndex);

    /* Match to a NodeID constraint */
    switch((int) Constraint->AType)
      {
      case mnaNodeID:
        /* If 'ALL' constraint is set, then we match it */
        if (!strcmp(Constraint->AVal[0],"ALL"))
          continue;

        /* If 'DEFAULT' constraint is set, then we match it */
        if (!strcmp(Constraint->AVal[0],"DEFAULT"))
          DefaultNodeConstraint = TRUE;

        /* Finally, do a instance 'name' check with the constraint,
         * if no match, we fail the constraint check 
         */
        if (strcmp(N->Name,Constraint->AVal[0]))
          return(FALSE);
        break;

      case mnaPartition:

        /* If 'ALL' constraint is set, then we match it */
        if (!strcmp(Constraint->AVal[0],"ALL"))
          continue;

        /* Finally, do a instance 'name' check with the constraint,
         * if no match, we fail the constraint check 
         */
        if (strcmp(N->Partition,Constraint->AVal[0]))
          return(FALSE);
        break;

      /* NodeState checking */
      case mnaNodeState:
        /* If 'ALL' constraint is set, then we match it by default */
        if (!strcmp(Constraint->AVal[0],"ALL"))
          {
          DisplayAllOverride = TRUE;
          continue;
          }

        /* See if we hve a state of 'Drained', if so, then set the override flag */
        if (MUGetIndexCI(Constraint->AVal[0],MNodeState,FALSE,mnsNONE) == mnsDrained)
          {
          /* Mark that this Node is in a drained state and 
           * we have an override
           */
          DisplayOverride = TRUE;
          continue;
          }

        break;

      default:
        break;
      }

    } /* END for (CIndex = 0; CIndex < ConstraintList->NumItems;CIndex++) */

  /* do not include the default node unless requested by a constraint */

  if ((!DefaultNodeConstraint) &&
      (!strcmp(N->Name,"DEFAULT")))
    {
    return(FALSE);
    }

  /* If we made it to here, and '-w NODESTATE=ALL' was requested, 
   * then this Node should be displayed regardless of other constraints.
   */
  if (DisplayAllOverride == TRUE)
    return(TRUE);

  /* different code depending on if DISPLAYFLAGS is set with HIDEDRAINED or not */
  if (bmisset(&MSched.DisplayFlags,mdspfHideDrained) == TRUE)
    {
      /* Check this Node's effective state. 
       * Display only non-drained nodes, 
       *     depending on state of the Node and the Displayoverride
       */
      if (N->State == mnsDrained)
        {
        /* yet if the DisplayOverride is FALSE, no display of node */
        if (DisplayOverride == FALSE)
          return(FALSE);
        }
      else
        {
        /* with the DisplayOveride ON, non-drained nodes are not displayed */
        if (DisplayOverride == TRUE)
          return(FALSE);
        }
    }
  else
    {
      /* If -w NODESTATE=DRAINED is requested, Nodes in the Drained state are NOT displayed */
      if ((DisplayOverride == TRUE) && (N->State != mnsDrained))
        return(FALSE);
    }

  /* Reach here if this node matches ALL the constraints in the list */
  return(TRUE);
  } /* END MNodeTransitionFitsConstraintList() */



/**
 * Allocate storage for the given node transition object
 *
 * @param   NP (O) [alloc-ed] pointer to the newly allocated node transition object
 */

int MNodeTransitionAllocate(

  mtransnode_t **NP)

  {
  mtransnode_t *N;

  if (NP == NULL)
    {
    return(FAILURE);
    }

  N = (mtransnode_t *)MUCalloc(1,sizeof(mtransnode_t));

  /* The memory is already allocated above, 
   * but we need to call the ctor on each instance so we will
   * use the C++ 'placement new' operator to call the ctor
   */
  new (&N->GEvents) mstring_t(MMAX_LINE);
  new (&N->GMetric) mstring_t(MMAX_LINE);
  new (&N->ReservationList) mstring_t(MMAX_LINE);
  new (&N->AttrList) mstring_t(MMAX_LINE);

  *NP = N;

  return(SUCCESS);
  }  /* END MNodeTransitionAllocate() */




/**
 * Free the storage for the given node transtion object
 *
 * @param   NP (I) [freed] pointer to the node transition object
 */

int MNodeTransitionFree(

  void **NP)

  {
  mtransnode_t *N = (mtransnode_t *)*NP;

  if (NP == NULL)
    {
    return(FAILURE);
    }

  MUFree(&N->OperatingSystem);
  MUFree(&N->Architecture);
  MUFree(&N->AvailGRes);
  MUFree(&N->ConfigGRes);
  MUFree(&N->AvailClasses);
  MUFree(&N->ConfigClasses);
  MUFree(&N->Features);
  MUFree(&N->JobList);
  MUFree(&N->Messages);
  MUFree(&N->OldMessages);
  MUFree(&N->NetworkAddress);
  MUFree(&N->NodeSubState);
  MUFree(&N->Operations);
  MUFree(&N->OSList);
  MUFree(&N->Owner);
  MUFree(&N->ResOvercommitFactor);
  MUFree(&N->ResOvercommitThreshold);
  MUFree(&N->Partition);
  MUFree(&N->PriorityFunction);
  MUFree(&N->ProvisioningData);
  MUFree(&N->ResourceManagerList);
  MUFree(&N->VMOSList);
  MUFree(&N->DedGRes);
  MUFree(&N->Flags);

  /* call the dtor on the mstring_t's in order to free their resources */
  N->GEvents.~mstring_t();
  N->GMetric.~mstring_t();
  N->ReservationList.~mstring_t();
  N->AttrList.~mstring_t();

  MXMLDestroyE(&N->Variables);

#ifdef MUSEMONGODB
  if (N->BSON != NULL)
    {
    delete N->BSON;
    N->BSON = NULL;
    }
#endif

  MUFree((char **)NP);

  return(SUCCESS);
  } /* END MNodeTransitionFree() */



/**
 * Store node in transition structure for storage in databse.
 *  
 * Note - DN points memory allocated with calloc so it is intialized to zero 
 *  
 * @param   SN (I) the node to store
 * @param   DN (O) the transition object to store it in
 */

int MNodeToTransitionStruct(

  mnode_t      *SN,
  mtransnode_t *DN)

  {
  char tmpLine[MMAX_BUFFER];

  int AProcs = 0;

  mbitmap_t BM;

  mstring_t tmpString(MMAX_LINE);

  tmpLine[0] = '\0';
  MUStrCpy(DN->Name,SN->Name,MMAX_LINE);

  DN->Index = SN->Index;
  DN->NodeIndex = SN->NodeIndex;
  
  DN->State = SN->State;
  MUStrDup(&DN->OperatingSystem,MAList[meOpsys][SN->ActiveOS]);

  DN->ConfiguredProcessors = SN->CRes.Procs;

  if (SN->DRes.Procs >= 0)
    {
    AProcs = MIN(SN->CRes.Procs - SN->DRes.Procs,SN->ARes.Procs);
    }
  else
    {
    MDB(7,fSCHED) MLog("INFO:     node '%s' dedicated procs negative, ignoring.\n",SN->Name);
    AProcs = SN->ARes.Procs;
    }

  MDB(7,fSCHED) MLog("INFO:     Transitioning node '%s', setting available procs to %d (MIN(%d,%d))\n",
    SN->Name,
    AProcs,
    SN->CRes.Procs - SN->DRes.Procs,
    SN->ARes.Procs);
  MDB(7,fSCHED) MLog("INFO:     '%s' Configured Procs: %d\n",SN->Name,SN->CRes.Procs);
  MDB(7,fSCHED) MLog("INFO:     '%s' Dedicated Procs:  %d\n",SN->Name,SN->DRes.Procs);
  MDB(7,fSCHED) MLog("INFO:     '%s' Available Procs:  %d\n",SN->Name,SN->ARes.Procs);

  DN->AvailableProcessors  = MAX(AProcs,0);
  DN->ConfiguredMemory     = SN->CRes.Mem;
  DN->AvailableMemory      = SN->CRes.Mem - SN->DRes.Mem;

  if (SN->Arch != 0)
    MUStrDup(&DN->Architecture,MAList[meArch][SN->Arch]);

  bmset(&BM,mcmXML);

  MNodeAToString(SN,mnaAvlGRes,&tmpString,&BM);
  MUStrDup(&DN->AvailGRes,tmpString.c_str());

  MNodeAToString(SN,mnaCfgGRes,&tmpString,&BM);
  MUStrDup(&DN->ConfigGRes,tmpString.c_str());

  MNodeAToString(SN,mnaDedGRes,&tmpString,&BM);
  MUStrDup(&DN->DedGRes,tmpString.c_str());

  /* Classes should be [<name>:<num>] */

  tmpString.clear();
  
  if (!bmisclear(&SN->Classes))
    {
    MClassListToMString(&SN->Classes,&tmpString,NULL);

    MUStrDup(&DN->AvailClasses,tmpString.c_str());
    MUStrDup(&DN->ConfigClasses,tmpString.c_str());
    }

  /*MNodeAToString(SN,mnaAvlClass,&tmpString,0);
  MUStrCpy(DN->AvailClasses,tmpString.c_str(),MMAX_LINE);

  MNodeAToString(SN,mnaCfgClass,&tmpString,0);
  MUStrCpy(DN->ConfigClasses,tmpString.c_str(),MMAX_LINE);*/

  DN->ChargeRate = SN->ChargeRate;

  MNodeAToString(SN,mnaDynamicPriority,&tmpString,0);
  if (!tmpString.empty())
    DN->DynamicPriority = strtod(tmpString.c_str(),NULL);

  DN->EnableProfiling = SN->ProfilingEnabled;

  MNodeAToString(SN,mnaFeatures,&tmpString,0);
  MUStrDup(&DN->Features,tmpString.c_str());

  MNodeAToString(SN,mnaFlags,&tmpString,0);
  MUStrDup(&DN->Flags,tmpString.c_str());

  MNodeAToString(SN,mnaGMetric,&tmpString,&BM);
  if (!MUStrIsEmpty(tmpString.c_str()))
    MStringSet(&DN->GMetric,tmpString.c_str());

  MNodeAToString(SN,mnaGEvent,&tmpString,&BM);
  if (!MUStrIsEmpty(tmpString.c_str()))
    MStringSet(&DN->GEvents,tmpString.c_str());

  DN->HopCount = SN->HopCount;

  DN->HypervisorType = SN->HVType;

  DN->IsDeleted = bmisset(&SN->IFlags,mnifIsDeleted);

  /* TBD add an indication to end of string if filled up with nodes */

  MNodeAToString(SN,mnaJobList,&tmpString,0);
  MUStrDup(&DN->JobList,tmpString.c_str());

  DN->LastUpdateTime = SN->ATime;

  DN->LoadAvg = SN->Load;
  DN->MaxLoad = SN->MaxLoad;

  DN->MaxJob = SN->AP.HLimit[mptMaxJob];

  if (SN->NP != NULL)
    DN->MaxJobPerUser = SN->NP->MaxJobPerUser;

  DN->MaxProc = SN->AP.HLimit[mptMaxProc];

  if (SN->NP != NULL)
    DN->MaxProcPerUser = SN->NP->MaxProcPerUser;

  MUStrDup(&DN->OldMessages,SN->Message);
  MUStrDup(&DN->NetworkAddress,SN->NetAddr);

  MUStrDup(&DN->NodeSubState,SN->SubState);

  MNodeAToString(SN,mnaOperations,&tmpString,0);
  MUStrDup(&DN->Operations,tmpString.c_str());

  MNodeAToString(SN,mnaOSList,&tmpString,0);
  MUStrDup(&DN->OSList,tmpString.c_str());

  MNodeAToString(SN,mnaOwner,&tmpString,0);
  MUStrDup(&DN->Owner,tmpString.c_str());

  MNodeAToString(SN,mnaResOvercommitFactor,&tmpString,0);
  MUStrDup(&DN->ResOvercommitFactor,tmpString.c_str());

  MNodeAToString(SN,mnaResOvercommitThreshold,&tmpString,0);
  MUStrDup(&DN->ResOvercommitThreshold,tmpString.c_str());

  MNodeAToString(SN,mnaPartition,&tmpString,0);
  MUStrDup(&DN->Partition,tmpString.c_str());

  DN->PowerIsEnabled = SN->PowerIsEnabled;

  DN->PowerPolicy = SN->PowerPolicy;
  DN->PowerSelectState = SN->PowerSelectState;
  DN->PowerState = SN->PowerState;

  DN->Priority = SN->Priority;

  MNodeAToString(SN,mnaPrioF,&tmpString,0);
  MUStrDup(&DN->PriorityFunction,tmpString.c_str());

  DN->ProcessorSpeed = SN->ProcSpeed;

  MNodeAToString(SN,mnaProvData,&tmpString,0);
  MUStrDup(&DN->ProvisioningData,tmpString.c_str());

  DN->AvailableDisk = SN->ARes.Disk;
  DN->AvailableSwap = SN->ARes.Swap;
  DN->ConfiguredDisk = SN->CRes.Disk;
  DN->ConfiguredSwap = SN->CRes.Swap;

  MNodeAToString(SN,mnaRsvCount,&tmpString,0);
  DN->ReservationCount = atoi(tmpString.c_str());

  MNodeAToString(SN,mnaRsvList,&tmpString,0);
  if (!MUStrIsEmpty(tmpString.c_str()))
    MStringSet(&DN->ReservationList,tmpString.c_str());

  MNodeAToString(SN,mnaRMList,&tmpString,0);
  MUStrDup(&DN->ResourceManagerList,tmpString.c_str());

  DN->Size = SN->SlotsUsed;
  DN->Speed = SN->Speed;

  if ((SN->P != NULL) && (SN->P->CWIsSet))
    DN->SpeedWeight = SN->P->CW[mnpcSpeed];
  
  DN->TotalNodeActiveTime = SN->SATime;
  
  DN->LastModifyTime = SN->StateMTime;
  DN->TotalTimeTracked = SN->STTime;
  DN->TotalNodeUpTime = SN->SUTime;
  DN->TaskCount = SN->TaskCount;     

  DN->Rack = SN->RackIndex;
  DN->Slot = SN->SlotIndex;

  if (!MUHTIsEmpty(&SN->Variables))
    {
    MUVarsToXML(&DN->Variables,&SN->Variables);
    }

  MNodeAToString(SN,mnaVarAttr,&tmpString,0);
  if (!tmpString.empty())
    MStringSet(&DN->AttrList,tmpString.c_str());

  MNodeAToString(SN,mnaVMOSList,&tmpString,0);
  MUStrDup(&DN->VMOSList,tmpString.c_str());

  if (SN->MB != NULL)
    {
    mxml_t *E = NULL;

    MXMLCreateE(&E,(char *)MNodeAttr[mnaMessages]);

    MMBPrintMessages(SN->MB,mfmXML,FALSE,-1,(char *)E,0);
    MXMLToString(E,tmpLine,sizeof(tmpLine),NULL,TRUE);
    MUStrDup(&DN->Messages,tmpLine);

    MXMLDestroyE(&E);
    }

#ifdef MUSEMONGODB
  MNodeTransitionToBSON(SN,&DN->BSON);
#endif

  return(SUCCESS);
  }  /* END MNodeToTransitionStruct() */


/**
 * transition a node to the queue to be written to the database
 *
 * See MNodeToTransitionStruct (child)
 *
 * @param   N (I) the node to be transitioned
 */

int MNodeTransition(

  mnode_t *N)

  {
  mtransnode_t *TN;

  const char *FName = "MNodeTransition";

  MDB(7,fTRANS) MLog("%s(%s)\n",
    FName,
    (MUStrIsEmpty(N->Name)) ? "NULL" : N->Name);

  if (bmisset(&MSched.DisplayFlags,mdspfUseBlocking))
    return(SUCCESS);

#if defined (__NOMCOMMTHREAD)
  return(SUCCESS);
#endif

  MNodeTransitionAllocate(&TN);

  MNodeToTransitionStruct(N,TN);

  MOExportTransition(mxoNode,TN);

  return(SUCCESS);
  }  /* END MNodeTransition() */


/* END MNodeTransition.c */
