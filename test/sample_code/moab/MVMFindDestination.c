/* HEADER */

/**
 * Functions for finding a destination for VM migration
 *
 */


#include "moab.h"
#include "moab-proto.h"
#include "moab-const.h"
#include "moab-global.h"

/* Local prototypes */

int __CreateVMJob(mvm_t *,mjob_t **);



/**
 * Chooses a destination for VM for migration.
 *
 * IntendedMigrations is passed in for reference only, not to be changed here.
 *
 * @param VM                 [I] - The VM to try to migrate
 * @param Policies           [I] - The policies package to use
 * @param NodeLoads          [I] - The hash table of node usages
 * @param VMLoads            [I] - The hash table of VM usages
 * @param IntendedMigrations [I] - Current intended migrations
 * @param EMsg               [O] - (Optional) Error message (already init'd)
 * @param DstN               [O] - Node that was chosen as a destination
 */

int VMFindDestination(

  mvm_t                   *VM,
  mvm_migrate_policy_if_t *Policies,
  mhash_t                 *NodeLoads,
  mhash_t                 *VMLoads,
  mln_t                  **IntendedMigrations,
  mstring_t               *EMsg,
  mnode_t                **DstN)

  {
  int rc = FAILURE;

  mnode_migrate_t **NodeList = NULL;
  int               NodeCount = 0;
  mnode_migrate_t  *NodeItem = NULL;

  int               NodeIndex = 0;

  mvm_migrate_usage_t *VMUsage = NULL;

  mjob_t *VMJob = NULL;           /* Job representing VM in scheduling routines */

  char TmpEMsg[MMAX_LINE];

  const char *FName = "VMFindDestination";

  MDB(4,fALL) MLog("%s(%s,Policies,NodeLoads,VMLoads,DstN)\n",
    FName,
    (VM != NULL) ? VM->VMID : "NULL");

  mrange_t RequestedRange[2];         /* Input range */
  mrange_t AvailRange[MMAX_RANGE];    /* Available range */
  char     BlockRsvID[MMAX_LINE];     /* Name of reservation blocking VM */

  /* Rejection reasons - these are just a list of reject reasons across the board, not
      reported on a per-hypervisor basis.  Check the logs for that. */

  mbool_t RejectReasons[10];
  int RejIndex;

  /* Shorthand so we don't have to create an enum for this one thing */

  enum RejectReasonsEnum {
    OvercommitReject  = 0, /* HV is/would be overcommitted */
    ReservationReject,     /* HV has reservation that precludes VM */
    PowerReject,           /* HV is off */
    HypervisorReject,      /* HV is wrong type, in wrong VLAN, etc. */
    FeatureReject,         /* HV doesn't have required features */
    PolicyReject};         /* HV was rejected by Policies->NodeCanBeDestination() */
                           /* POLICYREJECT MUST BE LAST! */

  const char *RejectString[] = {
    "Overcommit",
    "Reservation",
    "Powered Off",
    "Hypervisor",
    "Feature",
    "Policy" };

  if (DstN != NULL)
    *DstN = NULL;

  if ((VM == NULL) ||
      (Policies == NULL) ||
      (NodeLoads == NULL) ||
      (VMLoads == NULL) ||
      (DstN == NULL))
    return(FAILURE);

  /* SETUP */

  for (RejIndex = 0;RejIndex < PolicyReject + 1;RejIndex++)
    {
    RejectReasons[RejIndex] = FALSE;
    }  

  if (MUHTGet(VMLoads,VM->VMID,(void **)&VMUsage,NULL) == FAILURE)
    {
    MDB(6,fMIGRATE) MLog("ERROR:    cannot find VM usage information for VM '%s'\n",
      VM->VMID);

    MStringSetF(EMsg,"ERROR:    cannot find VM usage information for VM '%s'\n",
      VM->VMID);

    return(FAILURE);
    }

  /* Setup range for MJobGetSNRange below */

  memset(RequestedRange,0x0,sizeof(RequestedRange));
  RequestedRange[0].STime = MSched.Time;
  RequestedRange[0].ETime = MSched.Time;
  RequestedRange[1].ETime = 0;

  memset(AvailRange,0,sizeof(AvailRange));

  /* Create job to represent the VM when checking reservations below */

  if (__CreateVMJob(VM,&VMJob) == FAILURE)
    {
    MDB(6,fMIGRATE) MLog("ERROR:    failed to create temporary job for VM '%s'\n",
      VM->VMID);

    MStringSetF(EMsg,"ERROR:    failed to create temporary job for VM '%s'\n",
      VM->VMID);

    return(FAILURE);
    }

  /* Create list of possible destination nodes */

  NodesHashToList(NodeLoads,&NodeList,&NodeCount);

  /* FIND DESTINATION */

  /* Sort nodes - most desirable destination first */

  qsort((void *)&NodeList[0],
    NodeCount,
    sizeof(mnode_migrate_t *),
    (int(*)(const void *,const void *))Policies->OrderDestinations);

  /* Go through each destination, find first possible destination */

  for (NodeIndex = 0;NodeIndex < NodeCount;NodeIndex++)
    {
    NodeItem = NodeList[NodeIndex];

    if (NodeItem->N == VM->N)
      continue;

    /* Node valid? */

    if (MVMNodeIsValidDestination(VM,NodeItem->N,FALSE,TmpEMsg) == FALSE)
      {
      RejectReasons[HypervisorReject] = TRUE;

      MDB(6,fMIGRATE) MLog("INFO:     Node '%s' is not a valid destination for VM '%s' (%s)\n",
        NodeItem->N->Name,
        VM->VMID,
        TmpEMsg);

      continue;
      }

    /* Features - check the job */

    /* NOTE:  We may want to change this call to MReqCheckResourceMatch() in the future, but
        that may give problems with the reqos */

    if (MReqCheckFeatures(&NodeItem->N->FBM,VMJob->Req[0]) == FAILURE)
      {
      RejectReasons[FeatureReject] = TRUE;

      MDB(6,fMIGRATE) MLog("INFO:     Node '%s' is not a valid destination for VM '%s' (due to features)\n",
        NodeItem->N->Name,
        VM->VMID);

      continue;
      }

    if (NodeItem->N->PowerSelectState == mpowOff)
      {
      RejectReasons[PowerReject] = TRUE;

      MDB(6,fMIGRATE) MLog("INFO:     Node '%s' is not a valid destination for VM '%s' (currently powered off)\n",
        NodeItem->N->Name,
        VM->VMID);

      continue;
      } /* END if (DstN->PowerSelectState == mpowOff) */

    /* Node valid as destination by policy */

    if ((*Policies->NodeCanBeDestination)(NodeLoads,VMLoads,IntendedMigrations,NodeItem,VM) == FALSE)
      {
      RejectReasons[PolicyReject] = TRUE;

      continue;
      }

    /* Node overcommitted? */

    AddVMMigrationUsage(NodeItem->Usage,VMUsage);

    if (NodeIsOvercommitted(NodeItem,TRUE))
      {
      RejectReasons[OvercommitReject] = TRUE;

      MDB(6,fMIGRATE) MLog("INFO:     Node '%s' is not a valid destination for VM '%s' (would be overcommitted)\n",
        NodeItem->N->Name,
        VM->VMID);

      SubtractVMMigrationUsage(NodeItem->Usage,VMUsage);

      continue;
      }

    SubtractVMMigrationUsage(NodeItem->Usage,VMUsage);

    /* Reservations, etc.? */

    if (MJobGetSNRange(
          VMJob,
          VMJob->Req[0],
          NodeItem->N,
          RequestedRange,
          1,
          NULL,
          NULL,      /* O */
          AvailRange,
          NULL,
          TRUE,
          BlockRsvID) == FAILURE)
      {
      RejectReasons[ReservationReject] = TRUE;

      MDB(6,fMIGRATE) MLog("INFO:     Node '%s' is not a valid destination for VM '%s' (reserved by %s)\n",
        NodeItem->N->Name,
        VM->VMID,
        BlockRsvID);

      continue;
      }  /* END if (MJobGetSNRange() == FAILURE) */

    if (AvailRange[0].STime != MSched.Time)
      {
      /* start time isn't now - reservations, etc. */

      RejectReasons[ReservationReject] = TRUE;

      MDB(6,fMIGRATE) MLog("INFO:     Node '%s' is not a valid destination for VM '%s' (VM job can't run on node now)\n",
        NodeItem->N->Name,
        VM->VMID);

      continue;
      }

    /* FOUND IT! */

    *DstN = NodeItem->N;
    rc = SUCCESS;

    MDB(6,fMIGRATE) MLog("INFO:     Node '%s' chosen as destination for VM '%s'\n",
      NodeItem->N->Name,
      VM->VMID);

    break;
    }  /* END for (NodeIndex) */

  if (rc == FAILURE)
    {
    mstring_t Reasons(MMAX_LINE);
    mbool_t Printed = FALSE;

    for (RejIndex = 0;RejIndex < PolicyReject + 1;RejIndex++)
      {
      if (RejectReasons[RejIndex] == TRUE)
        MStringAppendF(&Reasons,"%s%s",
          (Printed == TRUE) ? "," : "",
          RejectString[RejIndex]);

      Printed = TRUE;
      }

    MDB(6,fMIGRATE) MLog("INFO:     Unable to find destination for VM '%s' (%s)\n",
      VM->VMID,
      Reasons.c_str());

    MStringSetF(EMsg,"INFO:     Unable to find destination for VM '%s' (%s)\n",
      VM->VMID,
      Reasons.c_str());
    }

  /* CLEANUP */                                       

  for (NodeIndex = 0;NodeIndex < NodeCount;NodeIndex++)
    {
    NodeItem = NodeList[NodeIndex];
    MUFree((char **)&NodeItem);
    } 

  MUFree((char **)&NodeList);

  /* Set this flag so that the reqs will be freed in MJobDestroy */

  bmset(&VMJob->IFlags,mjifReqIsAllocated);

  /* Wipe out VMJob's name so that the real tracking job doesn't get
      removed from the global hash table by MJobDestroy() */
  VMJob->Name[0] = '\0';

  MJobDestroy(&VMJob);

  /* Because VMJob is not considered a "real" job (VMJob->Index == 0), 
      the job itself is not freed by MJobDestroy.
     This is also true for the reqs */

  MUFree((char **)&VMJob);

  return(rc);
  }  /* END VMFindDestination() */



#define MMIGRATION_OC_BUFFER 0.9

/**
 * Return TRUE if the given node is overcommitted.  Can specify a given threshold to check.
 *
 * UseLowThreshold means that instead of using the normal threshold, the checked threshold
 *  will be a percentage of the original threshold.  This is used for checking if a node would
 *  be a good destination for migration.  If we check the normal threshold, we could be really
 *  close and a small change could cause the VM to migrate again.  This is used for load only.
 *
 * @param N               [I] - Node to check to see if it's overcommitted
 * @param UseLowThreshold [I] - If TRUE, will use the lower threshold
 */

mbool_t NodeIsOvercommitted(

  mnode_migrate_t       *N,
  mbool_t                UseLowThreshold)

  {
  double P; /* For checking percentages */
  double Threshold; /* Threshold to check against */
  int GIndex;

  if (N == NULL)
    return(FALSE);

  /* Mem Load */

  Threshold = MNodeGetOCThreshold(N->N,mrlMem);
  if (Threshold > 0.0)
    {
    if (UseLowThreshold)
      Threshold = Threshold * MMIGRATION_OC_BUFFER;

    P = ((double)N->Usage->MemLoad / MAX(1.0,(double)MNodeGetBaseValue(N->N,mrlMem)));

    if (P > Threshold)
      return(TRUE);
    }

  /* Mem Allocated */

  if (N->Usage->Mem > N->N->CRes.Mem)
    return(TRUE);

  /* Procs Load */

  Threshold = MNodeGetOCThreshold(N->N,mrlProc);
  if (Threshold > 0.0)
    {
    if (UseLowThreshold)
      Threshold = Threshold * MMIGRATION_OC_BUFFER;

    P = (N->Usage->ProcsLoad / MAX(1.0,(double)MNodeGetBaseValue(N->N,mrlProc)));

    if (P > Threshold)
      return(TRUE);
    }

  /* Procs Allocated */

  if (N->Usage->Procs > N->N->CRes.Procs)
    return(TRUE);

  /* GMetrics */
  for (GIndex = 0;GIndex < MSched.M[mxoxGMetric];GIndex++)
    {
    P = N->Usage->GMetric[GIndex]->IntervalLoad;
    Threshold = MNodeGetGMetricThreshold(N->N,GIndex);

    if ((P != MDEF_GMETRIC_VALUE) && (P > Threshold))
      return(TRUE);
    }  /* END for (GIndex) */

  return(FALSE);
  }  /* END NodeIsOvercommitted() */



/**
 *  Helper function to generate a job to represent a VM.
 *  No error checking.
 *
 * @see VMFindDestination (parent)
 *
 * @param VM    [I] - The VM to create a job for
 * @param VMJob [O] - The newly created job
 */

int __CreateVMJob(
  mvm_t   *VM,
  mjob_t **VMJob)

  {
  char *tmpFeatureList = NULL;

  mjob_t *J = NULL;

  MJobMakeTemp(&J);

  if (J == NULL)
    return(FAILURE);

  if (VM->TrackingJ != NULL)
    {
    MJobDuplicate(J,VM->TrackingJ,FALSE);
    }
  else
    {
    /* No tracking job, generate one -
        this is taken from the old MVMSelectDestination */

    mulong  tmpWC;
    /* apply vm attributes to pseudo-job */

    MCResCopy(&J->Req[0]->DRes,&VM->CRes);

    tmpWC = MCONST_HOURLEN;

    MJobSetAttr(J,mjaReqAWDuration,(void **)&tmpWC,mdfLong,mSet);

    J->Request.TC = 1;
    J->Req[0]->TaskCount = 1;

    /* Apply credentials to temporary job so affinities work using MJobGetFNL & MJobSelectMNL
     * using tracking job credentials.  NOTE: When tracking jobs aren't being used, credentials
     * won't be applied, because we don't have credentials on VM's, that will impact affinities on 
     * standing reservations.  For Example, consider the following negative affinity on node03: 
     *  
     * SRCFG[staging] QoSList=vmmigrate- Hostlist=node03 PERIOD=INFINITY */
    }  /* END else -- (VM->TrackingJ == NULL) */

  /* if VM Variables have VMFEATURES, use it. 
   * add features to the new system job to prevent migrations
   * onto nodes that do not share similar feature sets */

  if (SUCCESS == MUHTGet(&VM->Variables,"VMFEATURES",(void**)&tmpFeatureList,NULL))
    {
    char *tmpPtr = NULL;
    char *tmpTokPtr = NULL;
    char *copyFeatureList = NULL;
    mbitmap_t FBM;

    MUStrDup(&copyFeatureList,tmpFeatureList);
    tmpPtr = MUStrTok(copyFeatureList,",",&tmpTokPtr);

    /* we want VMFEATURES to override any existing features on the migration job
     * which may have been inherited by the tracking job */

    bmand(&J->Req[0]->ReqFBM,&FBM); /* will clear out ReqFBM */

    while (tmpPtr != NULL)
      {
      MUGetNodeFeature(tmpPtr,mVerify,&FBM);
      tmpPtr = MUStrTok(NULL,",",&tmpTokPtr);
      }

    if ((J->Req[0] != NULL) && !bmisclear(&FBM))
      {
      bmor(&J->Req[0]->ReqFBM,&FBM);
      }

    MUFree(&copyFeatureList);
    }

  *VMJob = J;

  return(SUCCESS);
  }  /* END __CreateVMJob() */


/**
 * Finds a destination for a VM, assuming that this is the only VM to be planned for.
 *  Uses the overcommit policies.
 *
 * @param VM   [I] - The VM to find a destination for
 * @param EMsg [O] - (optional) Error message
 * @param DstN [O] - The return node pointer
 */

int VMFindDestinationSingleRun(
  mvm_t     *VM,
  mstring_t *EMsg,
  mnode_t  **DstN)

  {
  mhash_t NodeLoads;
  mhash_t VMLoads;
  mln_t  *IntendedMigrations = NULL;

  mvm_migrate_policy_if_t *Policy = NULL;
  int rc = FAILURE;

  if (DstN != NULL)
    *DstN = NULL;

  if ((VM == NULL) || (DstN == NULL))
    return(FAILURE);

  /* Setup policy */

  if (VMOvercommitMigrationSetup(&Policy) == FAILURE)
    return(FAILURE);

  /* Get loads */

  MUHTCreate(&NodeLoads,-1);
  MUHTCreate(&VMLoads,-1);

  if (CalculateNodeAndVMLoads(&NodeLoads,&VMLoads) == FAILURE)
    {
    MDB(4,fMIGRATE) MLog("INFO:     failed to calculate load for nodes and VMs\n");

    MUHTFree(&NodeLoads,TRUE,MVMMIGRATEUSAGEFREE);
    MUHTFree(&VMLoads,TRUE,MVMMIGRATEUSAGEFREE);
    MUFree((char **)&Policy);

    return(FAILURE);
    }

  /* Find destination */

  rc = VMFindDestination(VM,Policy,&NodeLoads,&VMLoads,&IntendedMigrations,EMsg,DstN);

  /* Cleanup */

  /* IntendedMigrations should not be allocated (nothing planned), don't need to free */

  MUHTFree(&NodeLoads,TRUE,MVMMIGRATEUSAGEFREE);
  MUHTFree(&VMLoads,TRUE,MVMMIGRATEUSAGEFREE);

  MUFree((char **)&Policy);

  return(rc);
  }  /* END VMFindDestinationSingleRun() */
