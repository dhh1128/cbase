/* HEADER */

/**
 * Contains functions for VM Migrations
 *
 */


#include "moab.h"
#include "moab-proto.h"
#include "moab-const.h"
#include "moab-global.h"

/* Lookup table for VM migration policy function pointers (based on MVMMigrationPolicyEnum) */
static mvm_migrate_policy_if_t *VMMigrationPoliciesTable[mvmmpLAST+1];

/* Local prototypes */

int __PlanTypedVMMigrations(enum MVMMigrationPolicyEnum,int *,mhash_t *,mhash_t *,mln_t **);
int __VMsOnNodeToList(mnode_t *,mhash_t *,mvm_migrate_t ***,int *);
int __GetVMMigrationPoliciesTable(mvm_migrate_policy_if_t ***); /* For unit tests */




/**
 * Plans VM migrations based on the policy that is given.
 *
 * The IntendedMigrations list is always initialized here (unless NULL) and
 *   must be freed by the caller.
 *
 * Note: overcommit migration is always performed in addition to the requested
 *   migration type.  If overcommit is actually requested, only overcommit will
 *   tried.
 *
 * @param MigrationType      [I] - The type of migration policy to run (see note above)
 * @param IsManual           [I] - TRUE if this round of migrations was started by admin command
 * @param IntendedMigrations [O] - UNINITIALIZED m that will hold the migrations that were decided
 */

int PlanVMMigrations(

  enum MVMMigrationPolicyEnum MigrationType,
  mbool_t                     IsManual,
  mln_t                     **IntendedMigrations)

  {
  int MaxMigrations = -1;

  mhash_t NodeLoads;
  mhash_t VMLoads;

  const char *FName = "PlanVMMigrations";


  MDB(4,fALL) MLog("%s(%s,%s,IntendedMigrations)\n",
    FName,
    MVMMigrationPolicy[MigrationType],
    MBool[IsManual]);

  if (IntendedMigrations == NULL)
    return(FAILURE);

  if (VMMigrationPoliciesTable[MigrationType] == NULL)
    return(FAILURE);

  /* Check licensing */

  if (!MOLDISSET(MSched.LicensedFeatures,mlfVM))
    {
    MDB(4,fSTRUCT) MLog("INFO:     license does not allow VM management\n");

    return(FAILURE);
    }

  /* Count up current migrations, see how many migrations can be planned.
      -1 means no limit */

  MaxMigrations = CalculateMaxVMMigrations(IsManual);

  if (MaxMigrations >= 0)
    {
    MDB(6,fALL) MLog("INFO:     %d migration%s allowed this pass\n",
      MaxMigrations,
      (MaxMigrations != 1) ? "s" : "");
    }

  if (MaxMigrations == 0)
    {
    /* Not allowed to perform any migrations */

    return(SUCCESS);
    }

  /* Calculate load for all VMs and nodes */

  MUHTCreate(&NodeLoads,-1);
  MUHTCreate(&VMLoads,-1);

  if (CalculateNodeAndVMLoads(&NodeLoads,&VMLoads) == FAILURE)
    {
    MDB(4,fSTRUCT) MLog("INFO:     failed to calculate load for nodes and VMs\n");

    MUHTFree(&NodeLoads,TRUE,MVMMIGRATEUSAGEFREE);
    MUHTFree(&VMLoads,TRUE,MVMMIGRATEUSAGEFREE);

    return(FAILURE);
    }

  /* Call migration types (with associated overcommit, if applicable) */

  if (MigrationType == mvmmpConsolidation)
    {
    __PlanTypedVMMigrations(mvmmpConsolidationOvercommit,&MaxMigrations,&NodeLoads,&VMLoads,IntendedMigrations);
    __PlanTypedVMMigrations(MigrationType,&MaxMigrations,&NodeLoads,&VMLoads,IntendedMigrations);
    }
  else
    {
    __PlanTypedVMMigrations(MigrationType,&MaxMigrations,&NodeLoads,&VMLoads,IntendedMigrations);
    }

  /* Cleanup */

  MUHTFree(&NodeLoads,TRUE,MVMMIGRATEUSAGEFREE);
  MUHTFree(&VMLoads,TRUE,MVMMIGRATEUSAGEFREE);

  return(SUCCESS);
  }  /* END PlanVMMigrations() */





/**
 * Plans migrations of the given type.  This is where the bulk of the 
 *  VM migration intelligence takes place.
 *
 * Each migration policy is really determined by three sub-functions:
 *  1.  Ordering nodes (first node is highest priority to remove VMs from)
 *  2.  Ordering VMs (first VM should be the first to be migrated, if possible)
 *  3.  Selecting a destination
 *
 * There are also some optional policies (they need to be implemented, but can just return SUCCESS)
 *  * NodeSetup
 *  * NodeTearDown
 *
 *
 * @param MigrationType      [I] - The type of migration policy to run (see note above)
 * @param MaxMigrations      [I] - The maximum number of planned migrations (total, not just this pass)
 * @param NodeLoads          [I] (modified) - The node loads/usages - updated when a migration decision is made
 * @param VMLoads            [I] - The VM loads/usages
 * @param IntendedMigrations [O] - INITIALIZED mln_t ** that will hold the migrations that were decided
 */

int __PlanTypedVMMigrations(

  enum MVMMigrationPolicyEnum MigrationType,
  int                        *MaxMigrations,
  mhash_t                    *NodeLoads,
  mhash_t                    *VMLoads,
  mln_t                     **IntendedMigrations)

  {
  mnode_migrate_t **NodeList = NULL;
  mvm_migrate_t   **VMList = NULL;

  mnode_migrate_t *NodeItem = NULL;
  mvm_migrate_t   *VMItem = NULL;

  int NodeCount = 0;
  int NodeIndex = 0;
  int VMCount = 0;
  int VMIndex = 0;

  mvm_migrate_policy_if_t *Policies = NULL;

  const char *FName = "__PlanTypedVMMigrations";

  MDB(4,fALL) MLog("%s(%s,%d,%s)\n",
    FName,
    MVMMigrationPolicy[MigrationType],
    (MaxMigrations != NULL) ? *MaxMigrations : -2,
    (IntendedMigrations == NULL) ? "NULL" : "IntendedMigrations");

  if ((IntendedMigrations == NULL) ||
      (MaxMigrations == NULL) ||
      (NodeLoads == NULL) ||
      (VMLoads == NULL))
    return(FAILURE);

  /* Load policy functions */

  Policies = VMMigrationPoliciesTable[MigrationType];

  if (Policies == NULL)
    return(FAILURE);

  NodesHashToList(NodeLoads,&NodeList,&NodeCount);

  /* Order nodes according to Policies->OrderNodes -
      first node in list is highest priority to evaluate for migration */

  qsort((void *)&NodeList[0],
    NodeCount,
    sizeof(mnode_migrate_t *),
    (int(*)(const void *,const void *))Policies->OrderNodes);

  /* Go through each node and evaluate */

  for (NodeIndex = 0;NodeIndex < NodeCount;NodeIndex++)
    {
    NodeItem = NodeList[NodeIndex];

    MDB(7,fMIGRATE) MLog("INFO:     evaluating node '%s' for migration\n",
      NodeItem->N->Name);

    /* Check to see if we need to continue evaluating nodes for migration */

    if (*MaxMigrations == 0)
      break;

    if ((*Policies->MigrationsDone)(NodeLoads,VMLoads,NodeItem) == TRUE)
      {
      MDB(7,fMIGRATE) MLog("INFO:     done evaluating nodes for migration\n");

      break;
      }

    if (NodeItem->N->VMList == NULL)
      continue;

    /* Note: Don't need to check if node is eligible for migrations because
        that is done when creating the hash table of node usages, which is
        then converted into the nodes list that we're iterating through */

    if ((*Policies->NodeSetup)(NodeLoads,VMLoads,NodeItem,IntendedMigrations) == FAILURE)
      {
      /* Setup/precheck failed.  Continue to next node */

      continue;
      }

    /* Create list of VMs */

    __VMsOnNodeToList(NodeItem->N,VMLoads,&VMList,&VMCount);

    /* Order VMs - highest priority to migrate first */

    qsort((void *)&VMList[0],
      VMCount,
      sizeof(mvm_migrate_t *),
      (int(*)(const void *,const void *))Policies->OrderVMs);

    /* Check each VM on the node */

    for (VMIndex = 0;VMIndex < VMCount;VMIndex++)
      {
      mnode_t *DstN = NULL;

      VMItem = VMList[VMIndex];

      MDB(7,fMIGRATE) MLog("INFO:     evaluating VM '%s' for migration\n",
        VMItem->VM->VMID);

      /* See if we're done migrating for this node */

      if (*MaxMigrations == 0)
        break;

      if ((*Policies->MigrationsDoneForNode)(NodeLoads,VMLoads,NodeItem,VMItem) == TRUE)
        break;

      /* See if VM is eligible for migration */

      if (!VMIsEligibleForMigration(VMItem->VM,FALSE))
        continue;

      /* Find destination node for VM */

      if ((VMFindDestination(VMItem->VM,Policies,NodeLoads,VMLoads,IntendedMigrations,NULL,&DstN) == SUCCESS) &&
          (DstN != NULL))
        {
        mvm_migrate_usage_t *DstNUsage = NULL;

        if (MUHTGet(NodeLoads,DstN->Name,(void **)&DstNUsage,NULL) == FAILURE)
          {
          MDB(6,fMIGRATE) MLog("ERROR:    cannot find node usage information for node '%s'\n",
            DstN->Name);

          continue;
          }

        /* Update usages */

        AddVMMigrationUsage(DstNUsage,VMItem->Usage);
        SubtractVMMigrationUsage(NodeItem->Usage,VMItem->Usage);

        /* Queue up the decision */

        QueueVMMigrate(VMItem->VM,DstN,MigrationType,MaxMigrations,IntendedMigrations);
        }  /* END if (VMFindDestination() == SUCCESS) */
      }  /* END for (VMIndex) */

    (*Policies->NodeTearDown)(NodeLoads,VMLoads,NodeItem,IntendedMigrations);

    MDB(7,fMIGRATE) MLog("INFO:     done evaluating node '%s' for migration\n",
      NodeItem->N->Name);

    /* Free VM list */

    for (VMIndex = 0;VMIndex < VMCount;VMIndex++)
      {
      VMItem = VMList[VMIndex];
      MUFree((char **)&VMItem);
      }

    MUFree((char **)&VMList);
    }  /* END for (NodeIndex) */

  /* Free node list */

  for (NodeIndex = 0;NodeIndex < NodeCount;NodeIndex++)
    {
    NodeItem = NodeList[NodeIndex];
    MUFree((char **)&NodeItem);
    }

  MUFree((char **)&NodeList);

  return(SUCCESS);
  }  /* END __PlanTypedVMMigrations() */





/**
 * Helper function to create a nodelist that can be passed into a qsort.
 *
 * We pass in the NodeLoads because NodeLoads will already have filtered out nodes that are
 *  not eligible for VM migration.  If the node is not in the loads table, no reason to check
 *  it for migration.
 *
 * Returns FAILURE if there was an error or if there were no nodes to add to the list.
 *
 * @param NodeLoads [I] - Hash table of node loads (represents nodes that are eligible for VM migration)
 * @param NodeList  [O] - Node list to be allocated
 * @param NodeCount [O] - Number of nodes in the list
 */

int NodesHashToList(

  mhash_t           *NodeLoads,
  mnode_migrate_t ***NodeList,
  int               *NodeCount)

  {
  mnode_migrate_t **NL = NULL;
  mhashiter_t Iter;
  mvm_migrate_usage_t *Load = NULL;
  mnode_t *N = NULL;
  int NIndex = 0;

  const char *FName = "NodesHashToList";

  MDB(7,fALL) MLog("%s(NodeLoads,NodeList,NodeCount)\n",
    FName);

  if (NodeList != NULL)
    *NodeList = NULL;

  if (NodeCount != NULL)
    *NodeCount = 0;

  if ((NodeLoads == NULL) || (NodeList == NULL) || (NodeCount == NULL))
    return(FAILURE);

  if (NodeLoads->NumItems == 0)
    return(FAILURE);

  NL = (mnode_migrate_t **)MUCalloc(NodeLoads->NumItems,sizeof(mnode_migrate_t *));

  if (NL == NULL)
    return(FAILURE);

  MUHTIterInit(&Iter);

  while (MUHTIterate(NodeLoads,NULL,(void **)&Load,NULL,&Iter) == SUCCESS)
    {
    if ((Load != NULL) && (MNodeFind(Load->Name,&N) == SUCCESS) && (N != NULL))
      {
      NL[NIndex] = (mnode_migrate_t *)MUCalloc(1,sizeof(mnode_migrate_t));

      NL[NIndex]->N = N;
      NL[NIndex]->Usage = Load;

      NIndex++;
      }
    }

  *NodeList = NL;
  *NodeCount = NodeLoads->NumItems;

  return(SUCCESS);
  }  /* END NodesHashToList() */



/**
 * Helper function to create a sortable list of VMs that are on a node.
 *
 * Returns all VMs, whether they are migratable or not.  This is so that
 *  policies will know all VMs.  For example, green will want to know that
 *  there is a VM that can't be migrated, which means that there is no
 *  way to completely vacate that node.
 *
 * Returns FAILURE if there is an error or if there are no VMs to add.
 *
 * @param N       [I] - The node whose VMs will be in the list
 * @param VMLoads [I] - The loads of VMs (so that usage can be included for the qsort)
 * @param VMList  [O] - Output list of VMs
 * @param VMCount [O] - Number of VMs in the list
 */

int __VMsOnNodeToList(

  mnode_t         *N,
  mhash_t         *VMLoads,
  mvm_migrate_t ***VMList,
  int             *VMCount)

  {
  mln_t *tmpL = NULL;
  mvm_migrate_t **VML = NULL;
  mvm_t *VM = NULL;
  int VMIndex = 0;

  const char *FName = "__VMsOnNodeToList";

  MDB(7,fALL) MLog("%s(%s,VMLoads,VMList,VMCount)\n",
    FName,
    (N != NULL) ? N->Name : "NULL");

  if (VMList != NULL)
    *VMList = NULL;

  if (VMCount != NULL)
    *VMCount = 0;

  if ((N == NULL) ||
      (VMLoads == NULL) ||
      (VMList == NULL) ||
      (VMCount == NULL))
    return(FAILURE);

  for (tmpL = N->VMList;tmpL != NULL;tmpL = tmpL->Next)
    {
    VMIndex++;
    }

  if (VMIndex == 0)
    return(FAILURE);

  VML = (mvm_migrate_t **)MUCalloc(VMIndex,sizeof(mvm_migrate_t *));

  if (VML == NULL)
    return(FAILURE);

  VMIndex = 0;

  for (tmpL = N->VMList;tmpL != NULL;tmpL = tmpL->Next)
    {
    VM = (mvm_t *)tmpL->Ptr;

    if (VM == NULL)
      continue;

    VML[VMIndex] = (mvm_migrate_t *)MUCalloc(1,sizeof(mvm_migrate_t));

    if (VML[VMIndex] == NULL)
      {
      /* Failed to alloc, free everything and fail */

      int tmpI = 0;
      mvm_migrate_t *tmpVML = NULL;

      for (tmpI = 0;tmpI < VMIndex;tmpI++)
        {
        tmpVML = VML[VMIndex];
        MUFree((char **)&tmpVML);
        }

      MUFree((char **)&VML);

      return(FAILURE);
      }

    VML[VMIndex]->VM = VM;
    MUHTGet(VMLoads,VM->VMID,(void **)&VML[VMIndex]->Usage,NULL);

    VMIndex++;
    }  /* END for (tmpL = N->VMList) */

  *VMList = VML;
  *VMCount = VMIndex;

  return(SUCCESS);
  }  /* END __VMsOnNodeToList() */ 





/**
 * Helper function to determine how many migrations can be planned.
 *   Returns the maximum number of migrations, -1 means no limit.
 *
 * Maximimum number of migrations is -1 if no throttle.
 * If there is a throttle, max numbers is the throttle minus the current
 *  number of queued/active migrations.
 *
 * @param IsManual [I] - TRUE if VM migration was started by admin command
 */

int CalculateMaxVMMigrations(

  mbool_t IsManual)

  {
  int MaxMigrations = -1;
  mvm_t *V = NULL;
  mhashiter_t Iter;

  if ((IsManual == TRUE) || (MSched.VMMigrateThrottle <= 0))
    return(-1);

  MaxMigrations = MSched.VMMigrateThrottle;

  /* Find all migrating VMs and subtract one.  If we hit 0, return */

  MUHTIterInit(&Iter);

  while (MUHTIterate(&MSched.VMTable,NULL,(void **)&V,NULL,&Iter) == SUCCESS)
    {
    if (MVMIsMigrating(V))
      {
      MaxMigrations--;

      if (MaxMigrations <= 0)
        return 0;
      }
    }  /* END while (MUHTIterate(&MSched.VMTable) == SUCCESS) */

  return(MaxMigrations);
  }  /* END CalculateMaxVMMigrations() */



/**
 * Performs the VM migrations specified in the passed in list.
 *
 * @param IntendedMigrations [I] - Migrations to perform, as given by PlanVMMigrations()
 * @param NumberPerformed    [O] - The number of migrations successfully submitted
 * @param MigrationCause     [I] - The reason the migrations were submitted
 */

int PerformVMMigrations(

  mln_t     **IntendedMigrations,
  int        *NumberPerformed,
  const char *MigrationCause)
  {
  mvm_migrate_decision_t *Decision = NULL;
  mln_t *tmpL = NULL;
  int    CountSuccessful = 0;

  if (NumberPerformed != NULL)
    *NumberPerformed = 0;

  if ((IntendedMigrations == NULL) || (NumberPerformed == NULL))
    return(FAILURE);

  if (*IntendedMigrations == NULL)
    return(SUCCESS);

  /* Check that migrations are allowed (licensing was checked in planning stage) */

  if ((MSched.Mode == msmMonitor) || (MSched.Mode == msmTest))
    {
    MDB(4,fMIGRATE) MLog("INFO:     migration disabled in monitor mode\n");

    FreeVMMigrationDecisions(IntendedMigrations);

    return(SUCCESS);
    }

  if (MSched.DisableVMDecisions == TRUE)
    {
    MDB(6,fMIGRATE) MLog("INFO:     VM migration decisions are disabled\n");

    FreeVMMigrationDecisions(IntendedMigrations);

    return(FAILURE);
    }

  mstring_t MigrateReasonStr(MMAX_NAME);  /* string message to be put on the migrate job as a msg */

  MStringSetF(&MigrateReasonStr,
      "Reason for migration: %s",
      (!MUStrIsEmpty((char *)MigrationCause)) ? MigrationCause : "UNKNOWN");

  /* Iterate through the list and start all of the migrations */

  for (tmpL = *IntendedMigrations;tmpL != NULL;tmpL = tmpL->Next)
    {
    Decision = (mvm_migrate_decision_t *)tmpL->Ptr;

    if ((Decision == NULL) || (Decision->DstNode == NULL) || (Decision->VM == NULL))
      {
      MDB(6,fMIGRATE) MLog("ERROR:    VM migration decision was not set up properly\n");

      continue;
      }

    if (MUSubmitVMMigrationJob(Decision->VM,Decision->DstNode,NULL,NULL,MigrateReasonStr.c_str(),NULL) == FAILURE)
      {
      MDB(6,fMIGRATE) MLog("ERROR:    failed to submit VM migration job for VM '%s'\n",
        Decision->VM->VMID);

      continue;
      }

    CountSuccessful++;
    }  /* END for (MigrateIndex) */

  *NumberPerformed = CountSuccessful;

  FreeVMMigrationDecisions(IntendedMigrations);

  return(SUCCESS);
  }  /* END PerformVMMigrations() */




/**
 * Used to get the destination node if the VM is currently migrating.
 *
 * Returns SUCCESS if the VM is migrating, FAILURE otherwise.  Note that
 *  a VM is considered "migrating" if there is a migration job for that VM,
 *  whether that job is active or not.
 *
 * @param V                  [I] - The VM to get the destination node for
 * @param IntendedMigrations [I] (optional) - List of planned migrations (mvm_migrate_decision_t *)
 * @param DstN               [O] - The return node pointer
 */

int MVMGetCurrentDestination(
  
  mvm_t    *V,
  mln_t   **IntendedMigrations,
  mnode_t **DstN)

  {
  mln_t *tmpL = NULL;
  mjob_t *J = NULL;

  if (DstN != NULL)
    *DstN = NULL;

  if (V == NULL)
    return(FAILURE);

  /* This code is similar to MUCheckAction(), but we can't
      just check for the first job because it is possible to have
      multiple generic system jobs, but the first is not a migration
      job while a later one is. */ 

  for (tmpL = V->Action;tmpL != NULL;tmpL = tmpL->Next)
    {
    if (MJobFind(tmpL->Name,&J,mjsmBasic) == FAILURE)
      continue;

    if (MVMJobIsVMMigrate(J) == TRUE)
      {
      if (J->System->VMDestinationNode != NULL)
        {
        if (DstN != NULL)
          MNodeFind(J->System->VMDestinationNode,DstN);

        return(SUCCESS);
        }
      }
    }  /* END for (tmpL = V->Action) */

  if (IntendedMigrations != NULL)
    {
    mvm_migrate_decision_t *Decision = NULL;

    for (tmpL = *IntendedMigrations;tmpL != NULL;tmpL = tmpL->Next)
      {
      Decision = (mvm_migrate_decision_t *)tmpL->Ptr;

      if (Decision == NULL)
        continue;

      if (Decision->VM == V)
        {
        *DstN = Decision->DstNode;

        return(SUCCESS);
        }
      }
    }  /* END if (IntendedMigrations != NULL) */

  return(FAILURE);
  }  /* END MVMGetCurrentDestination() */




/**
 * Report TRUE if specified VM has active/pending migration operation 
 * scheduled.
 *
 * When a VM is being migrated, it must have that in the action
 *
 * @param V (I) - The VM to check
 */

mbool_t MVMIsMigrating(

  mvm_t *V)

  {
  if (MVMGetCurrentDestination(V,NULL,NULL) == SUCCESS)
    return(TRUE);

  return(FALSE);
  }  /* END MVMIsMigrating() */



/**
 *  Helper function, returns TRUE if job is a VM migrate job (either internally
 *   created or from a migrate action template)
 *
 * @param J [I] - the job to check
 */

mbool_t MVMJobIsVMMigrate(

  const mjob_t *J)

  {
  if (J == NULL)
    return(FALSE);

  if (J->System == NULL)
    return(FALSE);

  if (J->System->VMDestinationNode == NULL)
    return(FALSE);

  if (J->System->JobType == msjtVMMigrate)
    return(TRUE);

  if ((J->System->JobType == msjtGeneric) &&
      (J->TemplateExtensions != NULL) &&
      (J->TemplateExtensions->TJobAction == mtjatMigrate) &&
      (J->System->VM != NULL))
    return(TRUE);

  return(FALSE);
  }  /* END MVMJobIsVMMigrate() */


/**
 *  Frees the VM migration decisions list.
 *
 * @param IntendedMigrations [I] (freed) - the list of vm decisions to free (frees decisions and actual list)
 */

int FreeVMMigrationDecisions(

  mln_t **IntendedMigrations)

  {
  if ((IntendedMigrations == NULL) || (*IntendedMigrations == NULL))
    return(SUCCESS);

  MULLFree(IntendedMigrations,MUFREE);

  return(SUCCESS);
  }  /* END FreeVMMigrationDecisions() */



/**
 * Queues a migration decision (does not execute the migration).
 *
 * @param VM                 [I] - The VM to be migrated
 * @param DstNode            [I] - Destination for the VM
 * @param Type               [I] - The policy that caused the migration
 * @param MaxMigrations      [O] - The number of max migrations remaining (after queueing current VM)
 * @param IntendedMigrations [O] - The queue of VM migration decisions
 */

int QueueVMMigrate(

  mvm_t   *VM,
  mnode_t *DstNode,
  enum MVMMigrationPolicyEnum Type,
  int     *MaxMigrations,
  mln_t  **IntendedMigrations)

  {
  mvm_migrate_decision_t *Decision = NULL;

  if ((VM == NULL) ||
      (DstNode == NULL) ||
      (IntendedMigrations == NULL) ||
      (MaxMigrations == NULL))
    return(FAILURE);

  Decision = (mvm_migrate_decision_t *)MUCalloc(1,sizeof(mvm_migrate_decision_t));

  if (Decision == NULL)
    return(FAILURE);

  /* Populate decision struct */

  Decision->VM = VM;
  Decision->SrcNode = VM->N;
  Decision->DstNode = DstNode;
  Decision->Type    = Type;

  /* Add to list */

  if (MULLAdd(IntendedMigrations,VM->VMID,(void *)Decision,NULL,MUFREE) == FAILURE)
    {
    MUFree((char **)&Decision);

    return(FAILURE);
    }

  if ((*MaxMigrations) > 0)
    (*MaxMigrations)--;

  return(SUCCESS);
  }  /* END QueueVMMigrate */


/**
 * Helper function that returns TRUE if node can participate
 *  in VM migrations (as source or destination).
 *
 * @param N [I] - the node to evaluate
 */

mbool_t NodeAllowsVMMigration(

  mnode_t *N)

  {
  if (N == NULL)
    return(FALSE);

  if (N->HVType == mhvtNONE)
    {
    MDB(7,fMIGRATE) MLog("INFO:     Node '%s' does not allow migration (no hypervisor type)\n",
      N->Name);

    return(FALSE);
    }

  if (bmisset(&N->Flags,mnfNoVMMigrations))
    {
    MDB(7,fMIGRATE) MLog("INFO:     Node '%s' does not allow migration (%s flag set)\n",
      N->Name,
      MNodeFlags[mnfNoVMMigrations]);

    return(FALSE);
    }

  if ((MSched.MigrateToZeroLoadNodes == FALSE) &&
      ((N->Load == 0.0) || (N->CRes.Mem == N->ARes.Mem)))
    {
    /* No CPU load or mem load reported means that there is something wrong with the hypervisor */

    MDB(7,fMIGRATE) MLog("INFO:     Node '%s' does not allow migration (no load reported)\n",
      N->Name);

    return(FALSE);
    }

  return(TRUE);
  }  /* END NodeAllowsVMMigration() */



/**
 * Helper function that returns TRUE if a VM is eligible to be migrated.
 *
 * @param VM                [I] - VM to check
 * @param IsManualMigration [I] - TRUE if this is checking for manual migration request
 */

mbool_t VMIsEligibleForMigration(

  mvm_t *VM,
  mbool_t  IsManualMigration)

  {
  if (VM == NULL)
    return(FALSE);

  if (!bmisset(&VM->Flags,mvmfCanMigrate))
    return(FALSE);

  if (MVMIsMigrating(VM))
    return(FALSE);

  if (bmisset(&VM->Flags,mvmfInitializing) ||
      bmisset(&VM->Flags,mvmfDestroyed) ||
      bmisset(&VM->Flags,mvmfDestroyPending))
    {         
    return(FALSE);
    }

  /* If VM has no OS reported, can't migrate because we can't ensure
      that the hypervisor can support the VM */

  if (VM->ActiveOS <= 0)
    return(FALSE);

  /*
  TODO
  This was in the old VM migration code - need to see if this flag works
    with new workflow VMs

  if (!bmisset(&VM->Flags,mvmfCreationCompleted))
    {
    return(FALSE);
    }
  */

  if ((VM->TrackingJ == NULL) || (!MJOBISACTIVE(VM->TrackingJ)))
    return(FALSE);

  if (IsManualMigration == FALSE)
    {
    if ((VM->CRes.Procs <= 0) &&
        (VM->CRes.Mem <= 0))
      {
      /* VM is not being reported properly by RM, can't know the load */

      return(FALSE);
      }

    if (VM->State == mnsUnknown)
      {
      /* If VM is truly unknown, procs and mem may not be reported, so they
          won't be updated, don't migrate because again we can't know the load */

      return(FALSE);
      }
    }  /* END if (IsManualMigration == FALSE) */

  return(TRUE);
  }  /* END VMIsEligibleForMigration() */



/**
 * Sets up the function pointer table for migration policies.
 */

int SetupVMMigrationPolicies()

  {
  int rc = SUCCESS;

  VMMigrationPoliciesTable[mvmmpNONE] = NULL;
  VMMigrationPoliciesTable[mvmmpLAST] = NULL;

  rc = VMOvercommitMigrationSetup(&VMMigrationPoliciesTable[mvmmpOvercommit]);
  if (rc != SUCCESS)
    goto FreeAndExit;

  rc = VMConsolidationMigrationSetup(&VMMigrationPoliciesTable[mvmmpConsolidation]);
  if (rc != SUCCESS)
    goto FreeAndExit;

  rc = VMConsolidationOvercommitMigrationSetup(&VMMigrationPoliciesTable[mvmmpConsolidationOvercommit]);
  if (rc != SUCCESS)
    goto FreeAndExit;

  return(SUCCESS);

FreeAndExit:

  /* Something had an error.  Free everything and return FAILURE */

  MUFree((char **)&VMMigrationPoliciesTable[mvmmpOvercommit]);
  MUFree((char **)&VMMigrationPoliciesTable[mvmmpConsolidation]);
  MUFree((char **)&VMMigrationPoliciesTable[mvmmpConsolidationOvercommit]);

  return(FAILURE);
  }  /* END SetupVMMigrationPolicies() */


/**
 *  Helper function so that the policies table can be populated in unit tests.
 *
 * @param Table [O] - Pointer to the policies table
 */

int __GetVMMigrationPoliciesTable(

  mvm_migrate_policy_if_t ***Table)

  {
  *Table = &VMMigrationPoliciesTable[0];

  return(SUCCESS);
  }  /* END __GetVMMigrationPoliciesTable() */



/**
 * Creates a linked list pointing to VMs that are migrating (or planned
 *   to migrate) to the given node
 *
 * Returns SUCCESS if at least one VM migrating to the node is found, FAILURE otherwise
 *
 * @param N                  [I] - The node for which to get all VMs migrating to it
 * @param IntendedMigrations [I] (optional) - The list of current planned migrations
 * @param GetFirstOnly       [I] - If TRUE, return after finding the first VM migrating to the node
 * @param ReturnList         [O] (optional) - The output list to populate (elements should not be freed - pointers to VMs)
 */

int GetVMsMigratingToNode(

  mnode_t *N,
  mln_t  **IntendedMigrations,
  mbool_t  GetFirstOnly,
  mln_t  **ReturnList)

  {
  mnode_t *DstN = NULL;
  mvm_t *VM = NULL;
  mhashiter_t Iter;
  int rc = FAILURE;

  if (N == NULL)
    return(FAILURE);

  /* TODO - find better way to search for VMs moving to this node */

  /* Search in-flight/scheduled migrations */

  MUHTIterInit(&Iter);

  while (MUHTIterate(&MSched.VMTable,NULL,(void **)&VM,NULL,&Iter) == SUCCESS)
    {
    /* MVMGetCurrentDestination() will also check if the VM is in IntendedMigrations */

    if ((MVMGetCurrentDestination(VM,IntendedMigrations,&DstN) == SUCCESS) &&
        (DstN == N))
      {
      MULLAdd(ReturnList,VM->VMID,(void *)VM,NULL,NULL);

      rc = SUCCESS;

      if (GetFirstOnly == TRUE)
        return(rc);
      }
    }  /* END while (MUHTIterate(&MSched.VMTable) == SUCCESS) */

  return(rc);
  }  /* END GetVMsMigratingToNode() */
