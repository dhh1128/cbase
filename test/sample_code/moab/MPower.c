/* HEADER */

/**
 * @file MPower.c
 * 
 * Contains power-management related functions.
 */

/* Contains:                                 *
 *                                           */

#include <assert.h>

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  

/**
 * Initialize mpoweroff_req_t struct. 
 *  
 *  @param R (I) [modified]
 */

int MPowerOffReqInit(

  mpoweroff_req_t *R)

  {
  MNLInit(&R->NodeList);
  MUArrayListCreate(&R->Dependencies,sizeof(mln_t *),10);

  R->ForceSpecified = FALSE;

  return(SUCCESS);
  }  /* END MPowerOffReqInit() */


/**
 * Perform 'deep' free of R
 * 
 * @see MPowerOffReqInit() - peer
 *
 * @param R (I) [freed]
 */

int MPowerOffReqFree(

  mpoweroff_req_t *R) /* I [freed] */

  {
  int index;
  mln_t **ListOfLists = (mln_t **)R->Dependencies.Array;

  MNLFree(&R->NodeList);

  for (index = 0;index < R->Dependencies.NumItems;index++)
    {
    MULLFree(ListOfLists + index,NULL);
    }
 
  MUArrayListFree(&R->Dependencies);

  return(SUCCESS);
  } /* END MPowerOffReqFree */


/**
 * Determine which nodes can be powered off.
 *
 */

#define MMAX_GREENIDLEACTIVATIONDURATION 600

int MSysScheduleOnDemandPower()

  {
  mnl_t  NL = {0};

  int        nindex;
  int        pindex;
  int        nlindex;

  int        IdleCount = 0;

  mpar_t  *P;

  mnode_t *N;

  enum MNodeStateEnum State;

  mulong MaxIdleDuration = MMAX_GREENIDLEACTIVATIONDURATION;

  /* only turn nodes off, and don't create a greenpool rsv */
  /* moab can favor nodes that are already on in the NodePriority sections */

  MNLInit(&NL);

  for (pindex = 0;pindex < MSched.M[mxoPar];pindex++)
    {
    P = &MPar[pindex];
   
    if (P->Name[0] == '\0')
      break;

    if (P->Name[0] == '\1')
      continue;

    nlindex = 0;

    for (nindex = 0;nindex < MSched.M[mxoNode];nindex++)
      {
      mbool_t HasOnVMs = FALSE;
      mln_t *Link;

      N = MNode[nindex];

      if ((N == NULL) || (N->Name[0] == '\0'))
        break;

      if (N->Name[0] == '\1')
        continue;
   
      if (N->PtIndex != P->Index)
        continue;
   
      if (N->PowerPolicy == mpowpOnDemand) 
        {
        /* node-specific green power policy set */

        /* NO-OP */
        }
      else if ((N->PowerPolicy == mpowpNONE) && 
               (MSched.DefaultN.PowerPolicy == mpowpOnDemand))
        {
        /* global green power policy set */

        /* NO-OP */
        }
      else
        {
        /* node possesses non-green power policy */

        continue;
        }

      State = mnsNONE;

      if (((MNodeGetRMState(N,mrmrtCompute,&State) == FAILURE) &&
           (MNodeGetRMState(N,mrmrtAny,&State) == FAILURE)) ||
           (State == mnsDown))
        {
        /* node is down, not eligible for Green */

        continue;
        }

      for (Link = N->VMList;Link != NULL;Link = Link->Next)
        {
        mvm_t *VM = (mvm_t *)Link->Ptr;

        if ((VM->PowerSelectState != mpowOff)  || (VM->PowerState != mpowOff))
          {
          HasOnVMs = TRUE;
          break;
          }
        }

      if (HasOnVMs == TRUE)
        {
        /* cannot power node off because it has on VMs */

        continue;
        }

      /* if node is ON and doesn't have ANY reservations then power it off */

      if ((N->PowerState == mpowOn) && 
          (N->PowerSelectState != mpowOff) &&
          (MNodeGetSysJob(N,msjtPowerOff,MBNOTSET,NULL) == FAILURE) &&
          (MNodeGetRsv(N,FALSE,MSched.Time + MaxIdleDuration,NULL) == FAILURE) &&
          (MSched.Time >= (N->StateMTime + MSched.NodeIdlePowerThreshold)))
        {
        assert(MSched.NodeIdlePowerThreshold >= 0);

        if ((MSched.MaxGreenStandbyPoolSize > 0) && (IdleCount < MSched.MaxGreenStandbyPoolSize))
          {
          IdleCount++;

          /* utilize node for green pool */

          MDB(7,fALL) MLog("INFO:     node '%s' is idle and will be added to green pool (%d of %d)\n",
            N->Name,
            IdleCount,
            MSched.MaxGreenStandbyPoolSize);

          continue;
          }

        if (MSched.NodeIdlePowerThreshold > 0)
          {
          MDB(7,fALL) MLog("INFO:     node '%s' has no poweroff job, is on, has no rsv, and has been idle for %ld seconds (of %d allowable), will be powered off\n",
            N->Name,
            (MSched.Time - N->StateMTime),
            MSched.NodeIdlePowerThreshold);
          }
        else
          {
          MDB(7,fALL) MLog("INFO:     node '%s' has no poweroff job, is on, has no rsv, will be powered off\n",
            N->Name);
          }

        /* add node to system job */

        MNLSetNodeAtIndex(&NL,nlindex,N);
        MNLSetTCAtIndex(&NL,nlindex,1);

        nlindex++;
        }
      }   /* END for (nindex) */

    MNLTerminateAtIndex(&NL,nlindex);

    if (nlindex == 0)
      continue;

    if (MSched.DisableVMDecisions == TRUE)
      {
      int tmpNIndex;

      for (tmpNIndex = 0;MNLGetNodeAtIndex(&NL,tmpNIndex,NULL) == SUCCESS;tmpNIndex++)
        {
        char tmpEvent[MMAX_LINE];

        snprintf(tmpEvent,sizeof(tmpEvent),"Intended: turn off node %s to green policies",
          MNLGetNodeName(&NL,tmpNIndex,NULL));

        MOWriteEvent((void *)MNLReturnNodeAtIndex(&NL,tmpNIndex),mxoNode,mrelNodePowerOff,tmpEvent,NULL,NULL);
        }
      }
    else
      {
      MUGenerateSystemJobs(
        NULL,
        NULL,
        &NL,
        msjtPowerOff,
        "poweroff",
        -1,
        NULL,
        NULL,
        FALSE,
        (MSched.AggregateNodeActions == TRUE) ? FALSE : TRUE,
        FALSE, /* force execution */
        NULL);
      }

    MNLClear(&NL);
    }  /* END for(pindex) */

  MNLFree(&NL);

  return(SUCCESS);
  }  /* END MSysScheduleOnDemandPower() */



/**
 * Diagnose/report green state and configuration.
 *
 * NOTE:  process 'mdiag -G' request.
 *
 * @see MUIDiagnose() - parent
 *
 * @param Auth (I) (FIXME: not used but should be)
 * @param Buffer (O)
 * @param IFlags (I) [bitmap of enum MCModeEnum]
 */

int MSysDiagGreen(

  char      *Auth,
  mstring_t *Buffer,  
  mbitmap_t *IFlags)

  {
  double Watts = 0.0;
  double PWatts = 0.0;

  int pindex;
  int nindex;
  int qindex;

  mpar_t  *P;
  mnode_t *N;
  mjob_t  *J;
  mqos_t  *Q;
 
  if (Buffer == NULL)
    {
    return(FAILURE);
    }

  if (!MOLDISSET(MSched.LicensedFeatures,mlfGreen))
    {
    MStringAppendF(Buffer,"NOTE:  license does not allow power management, please contact Adaptive Computing\n");

    return(SUCCESS);
    }

  if (MPOWPISGREEN(MSched.DefaultN.PowerPolicy))
    {
    MStringAppendF(Buffer,"NOTE:  power management enabled for all nodes\n");
    }
  else
    {
    MStringAppendF(Buffer,"NOTE:  power management not enabled globally - power policy may be set on individual nodes\n");

    MStringAppendF(Buffer,"       use 'NODECFG[DEFAULT] powerpolicy=ondemand' to enable power management for all nodes\n");
    }

  if ((MSched.OnDemandProvisioningEnabled == FALSE) &&
      (MSched.OnDemandGreenEnabled == FALSE))
    {
    if (MSched.MaxGreenStandbyPoolSize > 0)
      {
      MStringAppendF(Buffer,"       %d idle standby nodes will be maintained\n",
        MSched.MaxGreenStandbyPoolSize);
      }

    if ((MPar[0].QOSBucket[0].RsvDepth == 1) &&
        (MPar[0].QOSBucket[1].Name[0] == '\0'))
      {
      MStringAppendF(Buffer,"NOTE:  on-demand power management performs best when reservation depth is set to values greater than 1 (see online documentation for details)\n");
      }
    }

  for (qindex = 1;qindex < MMAX_QOS;qindex++)
    {
    Q = &MQOS[qindex];

    if (Q->Name[0] == '\0')
      break;

    if (Q->Name[0] == '\1')
      continue;

    if ((Q->BLPowerThreshold > 0) ||
        (Q->XFPowerThreshold > 0) ||
        (Q->QTPowerThreshold > 0))
      {
      MStringAppendF(Buffer,"NOTE:  QOS-based power management enabled for QOS %s\n",
        Q->Name);
      }
    }  /* END for (qindex) */

  for (pindex = 0;pindex < MSched.M[mxoPar];pindex++)
    {
    P = &MPar[pindex];
   
    if (P->Name[0] == '\0')
      break;

    if (P->Name[0] == '\1')
      continue;

    if (P->NodePowerOnDuration > 0)
      {
      MStringAppendF(Buffer,"  partition power-on duration:  %ld seconds\n",
        P->NodePowerOnDuration);
      }

    if (P->NodePowerOffDuration > 0)
      {
      MStringAppendF(Buffer,"  partition power-off duration:  %ld seconds\n",
        P->NodePowerOffDuration);
      }

    if ((P->ConfigNodes == 0) || (pindex == 0))
      {
      continue;
      }

    MStringAppendF(Buffer," %.20s %10s %10s  %6s  %6s\n",
      "NodeID",
      "State",
      "Power",
      "Watts",
      "PWatts");

    for (nindex = 0;nindex < MSched.M[mxoNode];nindex++)
      {
      N = MNode[nindex];

      if (N == NULL)
        break;

      if (N->PtIndex != P->Index)
        continue;

      if (N->Name[0] == '\0')
        break;

      if (N->Name[0] == '\1')
        continue;

      MNodeGetPowerUsage(N,&Watts,&PWatts);

      /* FORMAT: Name State PowerState Watts PWatts Msg */

      MStringAppendF(Buffer,"  %.20s %10s %10s  %5.2f  %5.2f",
        N->Name,
        MNodeState[N->State],
        MPowerState[N->PowerState],
        Watts,
        PWatts);

      if (N->PowerSelectState == mpowOff)
        {
        MStringAppendF(Buffer,"  (powered off by Moab)\n");
        } 
      else if (N->PowerState == mpowOff)
        {
        MStringAppendF(Buffer,"  powered off externally (Moab will not manage power)\n");
        }
      else if (!MPOWPISGREEN(N->PowerPolicy) && 
               ((N->PowerPolicy != mpowpNONE) || (!MPOWPISGREEN(MSched.DefaultN.PowerPolicy))))
        {
        MStringAppendF(Buffer,"  (green powerpolicy NOT enabled - %s/%s)\n",
          MPowerPolicy[N->PowerPolicy],
          MPowerPolicy[MSched.DefaultN.PowerPolicy]);
        }
      else
        {
        MStringAppendF(Buffer,"\n");
        }

      if (MNodeGetSysJob(N,msjtPowerOn,MBNOTSET,&J) == SUCCESS)
        {
        if (MJOBISACTIVE(J) == TRUE)
          {
          MStringAppendF(Buffer,"  NOTE:  node is currently being powered on (request %s started %ld seconds ago)\n",
            J->Name,
            MSched.Time - J->StartTime);
          }
        else
          {
          MStringAppendF(Buffer,"  NOTE:  node power-on request is pending (see %s)\n",
            J->Name);
          }
        }
 
      /* NOTE:  no power-off job at this time */

      if (MNodeGetSysJob(N,msjtPowerOff,MBNOTSET,&J) == SUCCESS)
        {
        if (MJOBISACTIVE(J) == TRUE)
          {
          MStringAppendF(Buffer,"  NOTE:  node is currently being powered off (request started %ld seconds ago)\n",
            MSched.Time - J->StartTime);
          }
        else
          {
          MStringAppendF(Buffer,"  NOTE:  node is scheduled to be shutdown\n");
          }
        }
      }    /* END for (nindex) */
    }      /* END for (pindex) */

  if (MPowerGetPowerStateRM(NULL) == FAILURE)
    {
    MStringAppendF(Buffer,"WARNING: the only RM is an MSM RM with provisioning.  node power state cannot be determined and power jobs will fail\n");
    }

  return(SUCCESS);
  }  /* END MSysDiagGreen() */




/**
 * Get all migrating VMs on N into Out, using N->Action
 *
 * @param N (I)
 * @param Out (O) should be pre-initialized
 */

int MUGetMigratingVMs(

  mnode_t const *N,
  marray_t *Out)

  {
  mjob_t *J;
  mvm_t *VM;
  mln_t *tmpL = NULL;

  for (tmpL = N->Action;tmpL != NULL;tmpL = tmpL->Next)
    {
    /* See if it is a VM migrate job or VM migrate template action */

    if ((MJobFind(tmpL->Name,&J,mjsmBasic) == SUCCESS) &&
        (MVMJobIsVMMigrate(J) == TRUE) &&
        (MVMFind(J->System->VM,&VM) == SUCCESS))
      {
      MUArrayListAppendPtr(Out,VM);
      }
    else
      {
      assert((J == NULL) || (MJobGetSystemType(J) != msjtVMMigrate));
      }
    }

  return (SUCCESS);
  } /* END MUGetMigratingVMs() */





/**
 * Submit requisite migration jobs for nodes in NL, storing  the
 * jobs in OutListOfLists. All nodes that can't be completely vacated
 * are purged from the Node List.
 *
 * TODO: If desired, refactor the function so that the function
 * simply marks failed nodes with a TC of -1 and rely on another function
 * to resize NL and OutListOfLists
 * 
 * TODO: figure out how to do error reporting. We need a way to report potentially multiple errors
 *
 * TODO: this function does not work with multiple power off requests, because we can attempt to migrate
 *  jobs to nodes that are already powered off. We could create an ExclNL for this function, but it will
 *  not fix the problem for multiple poweroff requests across multiple mnodectl requests. We need some
 *  logic in MVMSelectDestination() to not pick a node if it is requested to be powered off.
 *
 * @param NL              (I) modified, list of nodes that will be powered off
 * @param OutListOfLists  (O) modified, must be empty and at least as big as as the number of nodes in NL
 * @param FailedNodes     (O) modified, must be empty and at least as big as as the number of nodes in NL
 * @param OutNLEnd
 *
 */

int MPowerOffSubmitMigrationJobs(

  mnl_t *NL,
  mln_t    **OutListOfLists,
  mnode_t  **FailedNodes,
  int       *OutNLEnd) 

  {
  typedef struct mvm_migrate_t {
    mvm_t *VM;
    mnode_t *DstNode;
    } mvm_migrate_t;

  int nindex = 0; /*index used for NL */
  int Offset = 0; /* used later for compressing NL if any nodes are rejected */
  int fnindex = 0; /*insert index for FailedNodes */
  marray_t MigratesList; /* of type mvm_migrate_t[], stores info needed to perform a single migrate */
  marray_t VMsToIgnore; /* of type mvm_t *[], stores VMs to ignore because they are already migrating */
  mnl_t ExclNL = {0};
  mnode_t  *N;

  MNLInit(&ExclNL);

  /* populate ExclNL with all requested poweroff nodes, so that we don't 
   * attempt to migrate to nodes we want to power off */

  for (nindex = 0;MNLGetNodeAtIndex(NL,nindex,&N) == SUCCESS;nindex++)
    {
    MNLSetNodeAtIndex(&ExclNL,nindex,N);
    MNLSetTCAtIndex(&ExclNL,nindex,N->CRes.Procs);
    } 

  MNLTerminateAtIndex(&ExclNL,nindex);

  MUArrayListCreate(&MigratesList,sizeof(mvm_migrate_t),10);
  MUArrayListCreate(&VMsToIgnore,sizeof(mvm_t *),10);

  for (nindex = 0;MNLGetNodeAtIndex(NL,nindex,&N) == SUCCESS;nindex++)
    {
    mln_t   *VMLink;
    mln_t   *JobSubmissionList = NULL;
    mbool_t  MigrateSpaceAvailable = TRUE; /* specifies whether destinations were found for all VM's on a node */
    mbool_t  CanMigrate = TRUE; /* specifies whether all jobs were successfully submitted to completely vacate N */
    mvm_t **IgnoreList;

    MigratesList.NumItems = 0; /* reset the list */
    VMsToIgnore.NumItems = 0; /* reset the list */

    MUGetMigratingVMs(N,&VMsToIgnore);
    IgnoreList =  (mvm_t **)VMsToIgnore.Array;

    for (VMLink = N->VMList;VMLink != NULL;VMLink = VMLink->Next) 
      {
      mvm_migrate_t *Migrate; 
      mvm_t *VM = (mvm_t *)VMLink->Ptr;
      int vmindex;
      mbool_t ShouldIgnore = FALSE;

      for (vmindex = 0;vmindex < VMsToIgnore.NumItems;vmindex++)
        {
        if (VM == IgnoreList[vmindex])
          {
          ShouldIgnore = TRUE;
          break;
          }
        }

      if (ShouldIgnore == TRUE)
        continue;

      Migrate = (mvm_migrate_t *)MUArrayListAppendEmpty(&MigratesList);
      Migrate->VM = VM;

      assert(Migrate->VM->N == N);

      if (VMFindDestinationSingleRun(Migrate->VM,NULL,&Migrate->DstNode) == FAILURE)
        {
        /* cannot find enough destinations to completely vacate node */

        MigrateSpaceAvailable = FALSE;

        break;
        }
      }  /* END for (VMLink = N->VMList) */

    if (MigrateSpaceAvailable)
      {
      char GEventID[MMAX_NAME];

      /* submit migration jobs */

      int index;

      mvm_migrate_t *Migrates = (mvm_migrate_t *)MigratesList.Array;

      MUStrCpy(GEventID,"power_migrate",sizeof(GEventID));

      for (index = 0;index < MigratesList.NumItems;index++)  
        {
        mjob_t *J;

        if (MSched.DisableVMDecisions == TRUE)
          {
          char tmpEvent[MMAX_LINE];

          snprintf(tmpEvent,sizeof(tmpEvent),"Intended: migrate VM %s to node %s to vacate node %s",
            Migrates[index].VM->VMID,
            Migrates[index].DstNode->Name,
            Migrates[index].VM->N->Name);

          MOWriteEvent((void *)Migrates[index].VM,mxoxVM,mrelVMMigrate,tmpEvent,NULL,NULL);

          CanMigrate = FALSE;

          break;
          }

        MVMAddGEvent(Migrates[index].VM,GEventID,0,"VM migration submitted.");
 
        if (MUSubmitVMMigrationJob(
             Migrates[index].VM,
             Migrates[index].DstNode,
             NULL,
             &J,
             NULL,
             NULL) == FAILURE)
          {
          /* cannot completely vacate node, as at least one migrate failed */

          CanMigrate = FALSE;

          break;
          }

        MULLAdd(&JobSubmissionList,J->Name,J,NULL,NULL);

        J->System->RecordEventInfo = MUStrFormat("move away from node %s before poweroff",
          Migrates[index].VM->N->Name);
        }
      }
    else
      {
      CanMigrate = FALSE;
      }  /* END if MigrateSpaceAvailable */

    if (CanMigrate)
      {
      ((mln_t **)OutListOfLists)[nindex] = JobSubmissionList;
      }
    else
      {
      /* Undo any migration job submissions made, as at least one job submission failed */
      mln_t *JobLink;

      for (JobLink = JobSubmissionList;JobLink !=NULL;JobLink = JobLink->Next)
        {
        MJobCancel((mjob_t *)JobLink->Ptr,"poweroff cancel\n",FALSE,NULL,NULL);

        JobLink->Ptr = NULL;
        }

      MNLSetTCAtIndex(NL,nindex,-1);
      MULLFree(&JobSubmissionList,NULL);

      if (FailedNodes != NULL)
        FailedNodes[fnindex++] = N;
      }
    }  /* END for (nindex) */

  if (FailedNodes != NULL)
    FailedNodes[fnindex] = NULL;

  /* remove all nodes that could not be vacated */

  /* find first node to be rejected */

  for (nindex = 0;(MNLGetNodeAtIndex(NL,nindex,NULL) == SUCCESS) && (MNLGetTCAtIndex(NL,nindex) >= 0);nindex++);

  /* start shifting pointers */

  while (MNLGetNodeAtIndex(NL,nindex,&N) == SUCCESS)
    {
    if (MNLGetTCAtIndex(NL,nindex) < 0)
      {
      Offset++;
      }

    nindex++;

    MNLSetNodeAtIndex(NL,nindex - Offset,N);
    MNLSetTCAtIndex(NL,nindex - Offset,MNLGetTCAtIndex(NL,nindex));

    OutListOfLists[nindex - Offset] = OutListOfLists[nindex];
    }

  if (OutNLEnd != NULL)
    {
    *OutNLEnd = (nindex - Offset);

    assert(MNLReturnNodeAtIndex(NL,*OutNLEnd) == NULL);
    assert(OutListOfLists[*OutNLEnd] == NULL);
    }

  MUArrayListFree(&MigratesList);
  MUArrayListFree(&VMsToIgnore);

  MNLFree(&ExclNL);
  return(SUCCESS);
  }  /* END MPowerOffSubmitMigrationJobs() */





/**
 * Get the watt usage from nodes, either made up or real
 *
 * @param N
 * @param Watts  (O - modified) 
 * @param PWatts (O - modified)
 */

int MNodeGetPowerUsage(

  mnode_t *N,
  double  *Watts,
  double  *PWatts)

  {
  mbool_t  RMReportsWatts;

  mpar_t  *P;

  if ((N == NULL) || (Watts == NULL) || (PWatts == NULL))
    {
    return(FAILURE);
    } 

  if (MNodeInitXLoad(N) == FAILURE)
    {
    /* cannot allocate memory */

    return(FAILURE);
    }

  *Watts = 0;
  *PWatts = 0;

  if (bmisset(&N->XLoad->RMSetBM,MSched.WattsGMetricIndex))
    RMReportsWatts = TRUE;
  else
    RMReportsWatts = FALSE;

  P = &MPar[N->PtIndex];

  if (MNODEISACTIVE(N) == TRUE)
    {
    /* Handle ACTIVE nodes */

    if (RMReportsWatts == FALSE)
      {
      MDB(4,fALL) MLog("INFO:     node '%s' is active but no power usage reported - using defaults\n",
        N->Name);

      /* RM is not reporting wattage, set passive watts and grab default */

      *Watts  = P->DefNodeActiveWatts;

      *PWatts = P->DefNodeIdleWatts;
      }
    else
      {
      if (N->XLoad->GMetric[MSched.WattsGMetricIndex]->IntervalLoad != MDEF_GMETRIC_VALUE)
        *Watts = N->XLoad->GMetric[MSched.WattsGMetricIndex]->IntervalLoad;

      if ((N->XLoad->GMetric[MSched.WattsGMetricIndex] != NULL) &&
          (N->XLoad->GMetric[MSched.WattsGMetricIndex]->IntervalLoad != MDEF_GMETRIC_VALUE))
        *PWatts = N->XLoad->GMetric[MSched.WattsGMetricIndex]->IntervalLoad;
      else
        *PWatts = P->DefNodeIdleWatts;
      }
    }   /* END if (MNODEISACTIVE(N) == TRUE) */
  else if (MNODEISGREEN(N) == FALSE)
    {
    /* Handle nodes that are ON but not active */

    if (RMReportsWatts == FALSE)
      {
      MDB(4,fALL) MLog("INFO:     node '%s' is on but no power usage reported - using defaults\n",
        N->Name);

      /* RM is not reporting wattage, set passive watts and grab default */

      *Watts  = P->DefNodeIdleWatts;

      *PWatts = P->DefNodeIdleWatts;
      }
    else
      {
      if (N->XLoad->GMetric[MSched.WattsGMetricIndex]->IntervalLoad != MDEF_GMETRIC_VALUE)
        *Watts = N->XLoad->GMetric[MSched.WattsGMetricIndex]->IntervalLoad;

      if ((N->XLoad->GMetric[MSched.WattsGMetricIndex] != NULL) &&
          (N->XLoad->GMetric[MSched.WattsGMetricIndex]->IntervalLoad != MDEF_GMETRIC_VALUE))
        *PWatts = N->XLoad->GMetric[MSched.WattsGMetricIndex]->IntervalLoad;
      else
        *PWatts = P->DefNodeIdleWatts;
      }
    }   /* END else if (MNODEISGREEN(N) == FALSE) */
  else
    {
    /* Handle nodes that are OFF */

    if (RMReportsWatts == FALSE)
      {
      MDB(4,fALL) MLog("INFO:     node '%s' is off but no power usage reported - using defaults\n",
        N->Name);

      *Watts = 0;

      *PWatts = P->DefNodeIdleWatts;
      }
    else
      {
      if (N->XLoad->GMetric[MSched.WattsGMetricIndex]->IntervalLoad != MDEF_GMETRIC_VALUE)
        *Watts = N->XLoad->GMetric[MSched.WattsGMetricIndex]->IntervalLoad;

      *PWatts = P->DefNodeIdleWatts;
      }
    }  /* END else */

  return(SUCCESS);
  }  /* END MNodeGetPowerUsage() */



/**
 * Update all (or specified) node(s) power usage.
 *
 * @see MStatUpdateBacklog() - parent
 *
 */

int MNodeUpdatePowerUsage(

  mnode_t *SN)      /* I - optional */

  {
  mpar_t *P;
  
  mnode_t *N;

  int      pindex;
  int      nindex;

  double NodeWatts = 0.0;
  double NodePWatts = 0.0;

  const char *FName = "MNodeUpdatePowerUsage";

  MDB(4,fALL) MLog("%s(%s)\n",
    FName,
    (SN != NULL) ? SN->Name : "NULL");

  if (SN == NULL)
    {
    for (pindex = 0;pindex < MSched.M[mxoPar];pindex++)
      {
      P = &MPar[pindex];

      if (P->Name[0] == '\0')
        break;

      if (P->Name[0] == '\1')
        continue;

      P->TotalWattsInUse = 0.0;
      }
    }

  if ((MSched.WattsGMetricIndex  <= 0) ||
      (MSched.PWattsGMetricIndex <= 0))
    {
    MDB(4,fALL) MLog("INFO:     scheduler not configured to track power usage\n");

    return(SUCCESS);
    }

  /* update per-node power usage */
  /* only update total system power usage if no SN is specified */

  for (nindex = 0;nindex < MSched.M[mxoNode];nindex++)
    {
    N = MNode[nindex];

    if ((N == NULL) || (N->Name[0] == '\0'))
      break;

    if (N->Name[0] == '\1')
      continue;

    if ((SN != NULL) && (N != SN))
      continue;

    MNodeGetPowerUsage(N,&NodeWatts,&NodePWatts);

    if ((NodeWatts == 0) && (NodePWatts == 0))
      continue;

    MDB(4,fALL) MLog("INFO:     calculated/reported power usage for node '%s' is %.2f\n",
      N->Name,
      NodeWatts);

    MNodeSetGMetric(N,MSched.WattsGMetricIndex,NodeWatts);
    MNodeSetGMetric(N,MSched.PWattsGMetricIndex,NodePWatts);

    if (SN == NULL)
      MPar[N->PtIndex].TotalWattsInUse += NodeWatts;
    }  /* END for (nindex) */

  return(SUCCESS);
  }  /* END MNodeUpdatePowerUsage() */





/*
 * Set the node's state based on power circumstances.
 *
 * @see XXX() - parent
 *
 * @param N (I)
 */

int MNodeSetAdaptiveState(

  mnode_t *N)  /* I */

  {
  const char *FName = "MNodeSetAdaptiveState";

  MDB(7,fALL) MLog("%s(%s)\n",
    FName,
    (N != NULL) ? N->Name : "NULL");

  if ((N->PowerPolicy != mpowpNONE) || 
     ((N->PowerPolicy == mpowpNONE) && (MSched.DefaultN.PowerPolicy != mpowpNONE)))
    {
    enum MPowerPolicyEnum PPolicy;

    PPolicy = (N->PowerPolicy != mpowpNONE) ? N->PowerPolicy : MSched.DefaultN.PowerPolicy;

    switch (PPolicy)
      {
      case mpowpOnDemand:
      case mpowpGreen:

        if (MNodeGetSysJob(N,msjtPowerOn,TRUE,NULL) == SUCCESS)
          {
          N->PowerSelectState = mpowOn;
          }
        else if (MNodeGetSysJob(N,msjtPowerOff,TRUE,NULL) == SUCCESS)
          {
          N->PowerSelectState = mpowOff;
          }
        else if (MSched.ProvRM != NULL)
          {
          /* no relevant active system jobs on node */
 
          N->PowerSelectState = N->PowerState;
          }

        if (N->PowerState == mpowOff)
          {
          MNodeSetState(N,mnsIdle,FALSE);
          }
        else if ((N->PowerSelectState == mpowOn) && 
                 (N->State == mnsDown) &&
                 ((MNodeGetSysJob(N,msjtPowerOn,MBNOTSET,NULL) == SUCCESS) ||
                  (MNodeGetSysJob(N,msjtOSProvision,MBNOTSET,NULL) == SUCCESS)))
          {
          /* node has power system job scheduled consider active */

          MNodeSetState(N,mnsActive,FALSE); 
          }
        else if ((N->PowerSelectState == mpowOff) &&
                 (N->State != mnsDown) &&
                 (MNodeGetSysJob(N,msjtPowerOff,MBNOTSET,NULL) == SUCCESS))
          {
          /* node has power system job scheduled consider active */

          MNodeSetState(N,mnsDown,FALSE);
          }
        else if (MSched.ProvRM != NULL)
          {
          if ((N->PowerSelectState == mpowNONE) &&
              (N->PowerState == mpowOff))
            {
            /* MSM reports node power state, if node is off then Moab can power it back on */

            MNodeSetState(N,mnsIdle,FALSE);
            N->PowerSelectState = mpowOff;
            }
          else if ((N->PowerSelectState == mpowOn) &&
                   (N->PowerState == mpowOff) &&
                   (!MNODEISUP(N)) &&
                   (MNodeGetSysJob(N,msjtPowerOn,MBNOTSET,NULL) == FAILURE) &&
                   (MNodeGetSysJob(N,msjtOSProvision,MBNOTSET,NULL) == FAILURE))
            {
            /* node is down, MSM is reporting node as OFF, Moab thinks the node should be ON
               but there is no PowerOn job on the node, add it to the green pool */

            MNodeSetState(N,mnsIdle,FALSE);
            N->PowerSelectState = mpowOff;
            }
          }

        MNodeUpdatePowerUsage(N);

        break;

      default:

        /* NO-OP */
 
        break;
      }  /* END switch (PPolicy) */
    }    /* END ((N->PowerPolicy != mpowpNONE) || ...) */

  if ((N->NextOS != 0) && 
      (N->NextOS != N->ActiveOS) &&
      (MNodeGetSysJob(N,msjtOSProvision,MBNOTSET,NULL) == SUCCESS))
    {
    /* node is provisioning */

    /* NOTE:  only mark node up if provisioning RM indicated node is up (NYI) */
 
    if (MNODEISUP(N) == FALSE) 
      MNodeSetState(N,mnsIdle,FALSE);
    }

  return(SUCCESS);
  }  /* END MNodeSetAdaptiveState() */




/** 
 * Checks for a resource manager that is not a 
 * provisioning + MSM resource manager. This is 
 * a necessary condition for being able to 
 * determine power state.
 *
 * If RMIndex is provided the function will
 * set it to the index of the resource manager
 * that fulfills these criteria.
 * 
 * @param RMIndex (O) [optional]
 */

mbool_t MPowerGetPowerStateRM (

  int *RMIndex)   /* O */
  
  {
  int rindex;

  for (rindex = 0;(rindex < MSched.M[mxoRM]) && (MRM[rindex].Name[0] != '\0');++rindex)
    { 
    if (!bmisset(&MRM[rindex].RTypes,mrmrtProvisioning) && 
        (rindex != MSched.InternalRM->Index))
      break;
    }  /* END for (rindex) */

  if (MRM[rindex].Name[0] != '\0')
    {
    if (RMIndex != NULL)
      *RMIndex = rindex;
  
    return(SUCCESS);
    }

  return(FAILURE);
  }  /* END MPowerGetPowerStateRM() */

/* END MPower.c */

/* vim: set ts=2 sw=2 expandtab nocindent filetype=c: */
