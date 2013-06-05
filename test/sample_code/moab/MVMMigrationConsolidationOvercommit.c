/* HEADER */

/**
 * Contains functions for VM Consolidation Overcommit migration policies
 *
 */


#include "moab.h"
#include "moab-proto.h"
#include "moab-const.h"
#include "moab-global.h"

/**
 * Setup function that will populate the function table for overcommit migration policies.
 *
 * @param MPIF [I] - the table to create and populate
 */

int VMConsolidationOvercommitMigrationSetup(

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

  mpif->OrderNodes = OrderNodesConsolidationOvercommit;
  mpif->OrderVMs   = OrderVMsConsolidationOvercommit;

  mpif->OrderDestinations = OrderDestinationsConsolidationOvercommit;

  mpif->MigrationsDone = MigrationsDoneConsolidationOvercommit;
  mpif->MigrationsDoneForNode = MigrationsDoneForNodeConsolidationOvercommit;

  mpif->NodeSetup = NodeSetupConsolidationOvercommit;
  mpif->NodeTearDown = NodeTearDownConsolidationOvercommit;

  mpif->NodeCanBeDestination = NodeCanBeDestinationConsolidationOvercommit;

  *MPIF = mpif;

  return(SUCCESS);
  }  /* END VMConsolidationOvercommitMigrationSetup() */

/**
 * Function for sorting nodes - most overloaded first
 *
 * Order of evaluation/importance is memory load, proc load, memory allocated,
 *   procs allocated, and gmetrics
 *
 * @param N1P - First node
 * @param N2P - Second node
 */

int OrderNodesConsolidationOvercommit(

  mnode_migrate_t **N1P,
  mnode_migrate_t **N2P)

  {
  double P1; /* For calculating percentages for N1 */
  double P2; /* For calculating percentages for N2 */
  double T1; /* Threshold to see if N1 is overcommitted */
  double T2; /* Threshold to see if N2 is overcommitted */

  int GIndex;

  mnode_migrate_t *N1 = *N1P;
  mnode_migrate_t *N2 = *N2P;

  /* Bigger number (P1,P2) means more overloaded.
      For return code, -1 means more overloaded */

  /* CHECK WITH THRESHOLDS */

  /* Mem Load */

  P1 = ((double)N1->Usage->MemLoad / MAX(1.0,(double)MNodeGetBaseValue(N1->N,mrlMem)));
  P2 = ((double)N2->Usage->MemLoad / MAX(1.0,(double)MNodeGetBaseValue(N2->N,mrlMem)));
  T1 = MNodeGetOCThreshold(N1->N,mrlMem);
  T2 = MNodeGetOCThreshold(N2->N,mrlMem);

  if (((T1 > 0.0) && (P1 > T1)) &&
      ((P2 <= T2) || (T2 <= 0.0)))  /* N1 over threshold, N2 is not */
    return(-1);
  else if (((T1 <= 0.0) || (P1 <= T1)) &&
           ((P2 > T2) && (T2 > 0.0))) /* N2 is over, N1 is not */
    return(1);
  else if ((P1 > T1) && (P2 > T2) &&
           (T1 > 0.0) && (T2 > 0.0))  /* Both are over threshold */
    {
    if (P1 > P2)      /* P1 is higher */
      return(-1);
    else if (P1 < P2) /* P2 is higher */
      return(1);
    }

  /* Proc Load */

  P1 = (N1->Usage->ProcsLoad / MAX(1.0,(double)MNodeGetBaseValue(N1->N,mrlProc)));
  P2 = (N2->Usage->ProcsLoad / MAX(1.0,(double)MNodeGetBaseValue(N2->N,mrlProc)));
  T1 = MNodeGetOCThreshold(N1->N,mrlMem);
  T2 = MNodeGetOCThreshold(N2->N,mrlMem);

  if (((T1 > 0.0) && (P1 > T1)) &&
      ((P2 <= T2) || (T2 <= 0.0)))  /* N1 over threshold, N2 is not */
    return(-1);
  else if (((T1 <= 0.0) || (P1 <= T1)) &&
           ((P2 > T2) && (T2 > 0.0))) /* N2 is over, N1 is not */
    return(1);
  else if ((P1 > T1) && (P2 > T2) &&
           (T1 > 0.0) && (T2 > 0.0))  /* Both are over threshold */
    {
    if (P1 > P2)      /* P1 is higher */
      return(-1);
    else if (P1 < P2) /* P2 is higher */
      return(1);
    }

  /* Mem Allocated */

  P1 = ((double)N1->Usage->Mem / MAX(1.0,(double)N1->N->CRes.Mem));
  P2 = ((double)N2->Usage->Mem / MAX(1.0,(double)N2->N->CRes.Mem));
  T1 = 1.0;
  T2 = 1.0;

  if ((P1 > T1) && (P2 <= T2))  /* N1 over threshold, N2 is not */
    return(-1);
  else if ((P1 <= T1) && (P2 > T2)) /* N2 is over, N1 is not */
    return(1);
  else if ((P1 > T1) && (P2 > T2))  /* Both are over threshold */
    {
    if (P1 > P2)
      return(-1);
    else if (P1 < P2)
      return(1);
    }

  /* Proc Allocated */

  P1 = ((double)N1->Usage->Procs / MAX(1.0,(double)N1->N->CRes.Procs));
  P2 = ((double)N2->Usage->Procs / MAX(1.0,(double)N2->N->CRes.Procs));
  T1 = 1.0;
  T2 = 1.0;

  if ((P1 > T1) && (P2 <= T2))  /* N1 over threshold, N2 is not */
    return(-1);
  else if ((P1 <= T1) && (P2 > T2)) /* N2 is over, N1 is not */
    return(1);
  else if ((P1 > T1) && (P2 > T2))  /* Both are over threshold */
    {
    if (P1 > P2)
      return(-1);
    else if (P1 < P2)
      return(1);
    }

  /* GMetrics */

  /* Get gmetric that is highest over threshold (if any) */

  for (GIndex = 0;GIndex < MSched.M[mxoxGMetric];GIndex++)
    {
    T1 = MNodeGetGMetricThreshold(N1->N,GIndex);
    T2 = MNodeGetGMetricThreshold(N2->N,GIndex);
    P1 = N1->Usage->GMetric[GIndex]->IntervalLoad;
    P2 = N2->Usage->GMetric[GIndex]->IntervalLoad;


    if ((T1 != MDEF_GMETRIC_VALUE) && (P1 != MDEF_GMETRIC_VALUE))
      P1 = P1 / T1;
    else
      P1 = -MDEF_GMETRIC_VALUE;

    if ((T2 != MDEF_GMETRIC_VALUE) && (P2 != MDEF_GMETRIC_VALUE))
      P2 = P2 / T2;
    else
      P2 = -MDEF_GMETRIC_VALUE;

    /* P1 and P2 are now either -MDEF_GMETRIC_VALUE or the percentage of threshold */
    if ((P1 > 1.0) && (P2 <= 1.0))
      return(-1);
    else if ((P2 > 1.0) && (P1 <= 1.0))
      return(1);
    else if ((P1 > 1.0) && (P2 > 1.0))
      {
      if (P1 > P2)
        return(-1);
      else if (P1 < P2)
        return(1);
      }
    }  /* END for (GIndex) */

  /* BOTH NODES ARE BELOW THRESHOLD - JUST GO WITH BIGGEST */

  return(OrderNodesHighToLowLoad(N1P,N2P));
  }  /* END OrderNodesConsolidationOvercommit() */



/**
 * Function for order VMs.  Biggest to smallest.
 *
 * @param VM1P [I] - First VM
 * @param VM2P [I] - Second VM
 */

int OrderVMsConsolidationOvercommit(

  mvm_migrate_t **VM1P,
  mvm_migrate_t **VM2P)

  {
  return(OrderVMsBigToSmall(VM1P,VM2P));
  }  /* END OrderVMsConsolidationOvercommit() */



/**
 * Function to order destinations for VM migrations in overcommit.
 *
 * Orders by most overloaded first - nodes that are overloaded will be
 *  filtered out by the NodeIsOvercommitted check.  This means that a
 *  VM will be migrated to pack nodes as much as possible without going
 *  over limits.  This sets up well for green.
 *
 * @param N1P [I] - first node
 * @param N2P [I] - second node
 */

int OrderDestinationsConsolidationOvercommit(

  mnode_migrate_t **N1P,
  mnode_migrate_t **N2P)

  {
  return(OrderNodesHighToLowLoad(N1P,N2P));
  }  /* END OrderDestinationsConsolidationOvercommit() */



/**
 * Returns TRUE if migrations are done for this iteration.
 *
 * @param NodeLoads [I] - Usages for all nodes
 * @param VMLoads   [I] - Usages for all VMs
 * @param NodeItem  [I] - The usage/node that is next to be evaluated for migration
 */

mbool_t MigrationsDoneConsolidationOvercommit(

  mhash_t *NodeLoads,
  mhash_t *VMLoads,
  mnode_migrate_t *NodeItem)

  {
  /* Nodes were already sorted in order of most overcommitted.  If this node 
      is not overcommitted, we're done */

  if (!NodeIsOvercommitted(NodeItem,FALSE))
    return(TRUE);

  return(FALSE);
  }  /* END MigrationsDoneConsolidationOvercommit() */


/**
 * Returns TRUE if done evaluating current node for migrations
 *
 * @param NodeLoads [I] - Usages for all nodes
 * @param VMLoads   [I] - Usages for all VMs
 * @param NodeItem  [I] - Current node that is being evaluated for migration
 * @param VMItem    [I] - Current VM that is next up to evaluate for migration
 */

mbool_t MigrationsDoneForNodeConsolidationOvercommit(

  mhash_t         *NodeLoads,
  mhash_t         *VMLoads,
  mnode_migrate_t *NodeItem,
  mvm_migrate_t   *VMItem)

  {
  /* If node is not overcommitted anymore, than we don't evaluate anymore for overcommit */

  if (!NodeIsOvercommitted(NodeItem,FALSE))
    return(TRUE);

  return(FALSE);
  }  /* END MigrationsDoneForNodeConsolidationOvercommit() */



/**
 * No node setup for overcommit.
 *
 * @param NodeLoads [I] - Usages for all nodes
 * @param VMLoads   [I] - Usages for all VMs
 * @param NodeItem  [I] - Current node that is being evaluated for migration
 * @param IntendedMigrations [I] - List of intended migrations
 */

int NodeSetupConsolidationOvercommit(
  mhash_t *NodeLoads,
  mhash_t *VMLoads,
  mnode_migrate_t *NodeItem,
  mln_t          **IntendedMigrations)
  {
  return(SUCCESS);
  }  /* END NodeSetupConsolidationOvercommit() */


/**
 * No node teardown for overcommit.
 *
 * @param NodeLoads [I] - Usages for all nodes
 * @param VMLoads   [I] - Usages for all VMs
 * @param NodeItem  [I] - Current node that was just evaluated for migration
 * @param IntendedMigrations [I] - List of intended migrations
 */

int NodeTearDownConsolidationOvercommit(
  mhash_t *NodeLoads,
  mhash_t *VMLoads,
  mnode_migrate_t *NodeItem,
  mln_t          **IntendedMigrations)
  {
  return(SUCCESS);
  }  /* END NodeTearDownConsolidationOvercommit() */


/**
 * No extra destination requirements for overcommit
 *
 * @param NodeLoads [I] - Usages for all nodes
 * @param VMLoads   [I] - Usages for all VMs
 * @param IntendedMigrations [I] - Current intended migrations
 * @param NodeItem  [I] - Current node that is being evaluated as a destination
 * @param VM        [I] - VM to be migrated
 */

mbool_t NodeCanBeDestinationConsolidationOvercommit(

  mhash_t *NodeLoads,
  mhash_t *VMLoads,
  mln_t  **IntendedMigrations,
  mnode_migrate_t *NodeItem,
  mvm_t *VM)
  {
  return(TRUE);
  }  /* END NodeCanBeDestinationConsolidationOvercommit() */
