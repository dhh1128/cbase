/* HEADER */

/**
 * Contains functions for VM Consolidation migration policies
 *
 */


#include "moab.h"
#include "moab-proto.h"
#include "moab-const.h"
#include "moab-global.h"



/**
 * Setup function that will populate the function table for green migration policies.
 *
 * @param MPIF [I] - the table to create and populate
 */

int VMConsolidationMigrationSetup(

  mvm_migrate_policy_if_t **MPIF)

  {
  mvm_migrate_policy_if_t *mpif = NULL;

  if (MPIF == NULL)
    return(FAILURE);

  *MPIF = NULL;

  /* alloc */

  mpif = (mvm_migrate_policy_if_t *)MUCalloc(1,sizeof(mvm_migrate_policy_if_t));

  if (mpif == NULL)
    return(FAILURE);

  /* Set functions */

  mpif->OrderNodes = OrderNodesConsolidation;
  mpif->OrderVMs   = OrderVMsConsolidation;

  mpif->OrderDestinations = OrderDestinationsConsolidation;

  mpif->MigrationsDone = MigrationsDoneConsolidation;
  mpif->MigrationsDoneForNode = MigrationsDoneForNodeConsolidation;

  mpif->NodeSetup = NodeSetupConsolidation;
  mpif->NodeTearDown = NodeTearDownConsolidation;

  mpif->NodeCanBeDestination = NodeCanBeDestinationConsolidation;

  *MPIF = mpif;

  return(SUCCESS);
  }  /* END VMConsolidationMigrationSetup() */



/**
 * Function for sorting nodes - least overloaded first
 *
 * Order of evaluation/importance is memory load, proc load, memory allocated,
 *   procs allocated
 *
 * @param N1P - First node
 * @param N2P - Second node
 */

int OrderNodesConsolidation(

  mnode_migrate_t **N1P,
  mnode_migrate_t **N2P)

  {
  return(OrderNodesLowToHighLoad(N1P,N2P));
  }  /* END OrderNodesConsolidation() */




/**
 * Function for order VMs.  Biggest to smallest.
 *
 * @param VM1P [I] - First VM
 * @param VM2P [I] - Second VM
 */

int OrderVMsConsolidation(

  mvm_migrate_t **VM1P,
  mvm_migrate_t **VM2P)

  {
  return(OrderVMsBigToSmall(VM1P,VM2P));
  }  /* END OrderVMsConsolidation() */


/**
 * Function to order destinations for VM migrations in green.
 *
 * @param N1P [I] - first node
 * @param N2P [I] - second node
 */

int OrderDestinationsConsolidation(

  mnode_migrate_t **N1P,
  mnode_migrate_t **N2P)

  {
  return(OrderNodesHighToLowLoad(N1P,N2P));
  }  /* END OrderDestinationsConsolidation() */



/**
 * Returns TRUE if migrations are done for this iteration.
 *
 * @param NodeLoads [I] - Usages for all nodes
 * @param VMLoads   [I] - Usages for all VMs
 * @param NodeItem  [I] - The usage/node that is currently being evaluated
 */

mbool_t MigrationsDoneConsolidation(

  mhash_t *NodeLoads,
  mhash_t *VMLoads,
  mnode_migrate_t *NodeItem)

  {
  /* TODO - how do we decide that we're done migrating for green?  Evaluating everything
      will give the same answer, but will be grossly inefficient */

  return(FALSE);
  }  /* END MigrationsDoneConsolidation() */


/**
 * Returns TRUE if migrations are done for this node.
 *
 * @param NodeLoads [I] - Usages for all nodes
 * @param VMLoads   [I] - Usages for all VMs
 * @param NodeItem  [I] - The usage/node that is currently being evaluated
 * @param VMItem    [I] - The usage/VM that is next to be evaluated for migration
 */

mbool_t MigrationsDoneForNodeConsolidation(

  mhash_t *NodeLoads,
  mhash_t *VMLoads,
  mnode_migrate_t *NodeItem,
  mvm_migrate_t   *VMItem)

  {
  /* For green migration, we try to evacuate the entire node */

  return(FALSE);
  }  /* END MigrationsDoneForNodeConsolidation() */



/**
 * Check that node isn't the destination for other migrations.
 *
 * @param NodeLoads [I] - Usages for all nodes
 * @param VMLoads   [I] - Usages for all VMs
 * @param NodeItem  [I] - Current node that is being evaluated for migration
 * @param IntendedMigrations [I] - List of intended migrations
 */

int NodeSetupConsolidation(

  mhash_t *NodeLoads,
  mhash_t *VMLoads,
  mnode_migrate_t *NodeItem,
  mln_t          **IntendedMigrations)

  {
  mvm_t *tmpVM = NULL;
  mnode_t *DstN = NULL;
  mhashiter_t Iter;

  MUHTIterInit(&Iter);

  while (MUHTIterate(&MSched.VMTable,NULL,(void **)&tmpVM,NULL,&Iter) == SUCCESS)
    {
    if ((MVMGetCurrentDestination(tmpVM,IntendedMigrations,&DstN) == SUCCESS) &&
        (DstN == NodeItem->N))
      {
      /* Node has a VM migrating to it, isn't eligible for green migration */

      MDB(6,fMIGRATE) MLog("INFO:     Node '%s' is not valid as a source for green migration (already a destination for migration)\n",
        NodeItem->N->Name);

      return(FAILURE);
      }
    }  /* END while (MUHTIterate(&MSched.VMTable) == SUCCESS) */

  return(SUCCESS);
  }  /* END NodeSetupConsolidation() */


/** 
 * See if all VMs were evacuated off of node.  If not, back out changes.
 *
 * @param NodeLoads [I] - Usages for all nodes
 * @param VMLoads   [I] - Usages for all VMs
 * @param NodeItem  [I] - Current node that was just evaluated for migration
 * @param IntendedMigrations [I] - List of intended migrations
 */

int NodeTearDownConsolidation(

  mhash_t *NodeLoads,
  mhash_t *VMLoads,
  mnode_migrate_t *NodeItem,
  mln_t          **IntendedMigrations)

  {
  mnode_t *DstN = NULL;
  mvm_t *VM = NULL;
  mln_t *tmpL = NULL;
  mbool_t ClearDecisions = FALSE;


  for (tmpL = NodeItem->N->VMList;tmpL != NULL;tmpL = tmpL->Next)
    {
    VM = (mvm_t *)tmpL->Ptr;

    if (VM == NULL)
      continue;

    if ((MVMGetCurrentDestination(VM,IntendedMigrations,&DstN) == FAILURE) ||
        (DstN == NodeItem->N))
      {
      /* Node has a VM that isn't migrating away, clear all decisions from this node */

      ClearDecisions = TRUE;

      MDB(6,fMIGRATE) MLog("INFO:     Node '%s' could not be emptied for green migration, reverting migration decisions\n",
        NodeItem->N->Name);

      break;
      }
    }  /* END for (tmpL = NodeItem->N->VMList) */

  if ((ClearDecisions == FALSE) &&
      (GetVMsMigratingToNode(NodeItem->N,IntendedMigrations,TRUE,NULL) == SUCCESS))
    {
    ClearDecisions = TRUE;

    MDB(6,fMIGRATE) MLog("INFO:     Node '%s' could not be emptied for green migration, reverting migration decisions\n",
        NodeItem->N->Name);
    }

  if ((ClearDecisions == TRUE) && (*IntendedMigrations != NULL))
    {
    /* Go through intended migrations and remove every green migration away from
        this node so that we aren't taking up possible destination space for a node
        that can't be cleared */

    mvm_migrate_decision_t *Decision = NULL;
    mvm_migrate_usage_t *DstNUsage = NULL;
    mvm_migrate_usage_t *VMUsage = NULL;

    for (tmpL = *IntendedMigrations;tmpL != NULL;)
      {
      Decision = (mvm_migrate_decision_t *)tmpL->Ptr;

      /* Go to next one right now, in case current link gets removed */
      tmpL = tmpL->Next;

      if ((Decision->Type == mvmmpConsolidation) && (Decision->SrcNode == NodeItem->N))
        {
        /* Update usages */
        MUHTGet(VMLoads,Decision->VM->VMID,(void **)&VMUsage,NULL);
        MUHTGet(NodeLoads,Decision->DstNode->Name,(void **)&DstNUsage,NULL);

        /* Add back to src node (NodeItem) usage, remove from dst node usage */

        AddVMMigrationUsage(NodeItem->Usage,VMUsage);
        SubtractVMMigrationUsage(DstNUsage,VMUsage);

        /* Remove from decision list */

        MULLRemove(IntendedMigrations,Decision->VM->VMID,MUFREE);
        }
      }  /* END for (tmpL = *IntendedMigrations) */
    }  /* END if *(ClearDecisions == TRUE) && (*IntendedMigrations != NULL)) */

  return(SUCCESS);
  }  /* END NodeTearDownConsolidation() */



/**
 * Make sure that we don't use empty nodes as destinations for green
 *
 * @param NodeLoads          [I] - Usages for all nodes
 * @param VMLoads            [I] - Usages for all VMs
 * @param IntendedMigrations [I] - Current intended migrations
 * @param NodeItem           [I] - Current node that is being evaluated as a destination
 * @param VM                 [I] - VM to be migrated
 */

mbool_t NodeCanBeDestinationConsolidation(

  mhash_t *NodeLoads,
  mhash_t *VMLoads,
  mln_t  **IntendedMigrations,
  mnode_migrate_t *NodeItem,
  mvm_t *VM)
  {
  mln_t *tmpL = NULL;
  mvm_t *tmpVM = NULL;
  mnode_t *DstN = NULL;

  for (tmpL = NodeItem->N->VMList;tmpL != NULL;tmpL = tmpL->Next)
    {
    tmpVM = (mvm_t *)tmpL->Ptr;

    if (tmpVM == NULL)
      continue;

    if ((MVMGetCurrentDestination(tmpVM,IntendedMigrations,&DstN) == SUCCESS) &&
        (DstN != NodeItem->N))
      {
      /* VM is migrating away */
      continue;
      }

    /* Found a VM that is not migrating away, node is not empty - valid destination for green */

    return(TRUE);
    }  /* END for (tmpL = N->VMList) */

  if (GetVMsMigratingToNode(NodeItem->N,IntendedMigrations,TRUE,NULL) == SUCCESS)
    {
    return(TRUE);
    }

  /* No VMs on or coming to this node, should be left empty */

  MDB(6,fMIGRATE) MLog("INFO:     Node '%s' is not valid as a destination for VM '%s' for green migration (node is already empty)\n",
    NodeItem->N->Name,
    VM->VMID);

  return(FALSE);
  }  /* END NodeCanBeDestinationConsolidation() */
