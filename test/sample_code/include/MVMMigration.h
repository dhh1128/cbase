/* HEADER */

/**
 * @file MVMMigration.h
 *
 */

#ifndef  __MVMMIGRATION_H__
#define  __MVMMIGRATION_H__

/**
 * Structure to hold decisions from VM migration planning
 */

typedef struct mvm_migrate_decision_t {
  struct mvm_t   *VM;       /* VM to migrate */
  struct mnode_t *SrcNode;  /* Node to migrate from */
  struct mnode_t *DstNode;  /* Node to migrate to */

  enum MVMMigrationPolicyEnum Type; /* Type of migration (green, overcommit, etc.) */
  }mvm_migrate_decision_t;


/**
 * Structure to hold VM and Node usage stats to be used in VM migration decisions
 */

typedef struct mvm_migrate_usage_t {
  char *Name;          /* Name of node/VM */

  /* Allocated - for a VM, this is CRes.  For a Node, it's allocated (CRes (overcommit) - ARes) */
  int Procs;
  int Mem;
  int Swap;
  int Disk;

  /* Load - ABSOLUTE NUMBER - NOT A PERCENTAGE */
  double ProcsLoad;  /* Reported by RM */
  int MemLoad;       /* Utilized (non-overcommit value) */
  int SwapLoad;      /* Utilized (non-overcommit value) */
  int DiskLoad;      /* Utilized (non-overcommit value) */

  /* GMetrics */
  struct mgmetric_t **GMetric;
  }mvm_migrate_usage_t;



/* Used in the list of nodes to be qsorted, acted on in VM migration */
typedef struct mnode_migrate_t {
  mnode_t *N;
  mvm_migrate_usage_t *Usage;
  }mnode_migrate_t;

/* Used in the list of VMs to be qsorted, acted on in VM migration */
typedef struct mvm_migrate_t {
  mvm_t *VM;
  mvm_migrate_usage_t *Usage;
  }mvm_migrate_t;



/**
 * Policy Interface for VM migration policies.
 *
 * ALL of the below functions must be implemented!  No NULL checks will be performed to see if the function exists.
 */

typedef struct mvm_migrate_policy_if_t {

  /* PLAN MIGRATIONS FUNCTIONS */

  /* Args - node/VM migrate objects to sort */
  int (*OrderNodes)(mnode_migrate_t **,mnode_migrate_t **);  /* Sort nodes (passed to qsort) - first node in list is first to be evaluated for migration */
  int (*OrderVMs)(mvm_migrate_t **,mvm_migrate_t **);        /* Order VMs on a node (passed to qsort) - first VM is first to evaluate for migration */

  /* Args - (Nodes usage hash, VM usage hash, current node up for evaluation) */
  mbool_t (*MigrationsDone)(mhash_t *,mhash_t *,mnode_migrate_t *);/* Return TRUE if no more migrations need to be performed to fulfill this policy */

  /* Args - (Nodes usage hash, VM usage hash, current node, current VM) */
  mbool_t (*MigrationsDoneForNode)(mhash_t *,mhash_t *,mnode_migrate_t *,mvm_migrate_t *);/* Return TRUE if done evaluating current node for migrations */

  /* Args - (Nodes usage hash, VM usage hash, current node, intended migrations) */
  int (*NodeSetup)(mhash_t *,mhash_t *,mnode_migrate_t *,mln_t **);    /* Setup function called for each node BEFORE evaluating VMs */
  int (*NodeTearDown)(mhash_t *,mhash_t *,mnode_migrate_t *,mln_t **); /* Teardown function called AFTER evaluating VMs on the node */

  /* FIND DESTINATIONS FUNCTIONS */

  /* Args - node/VM migrate objects to sort */
  int (*OrderDestinations)(mnode_migrate_t **,mnode_migrate_t **);  /* Sort nodes (passed to qsort) - first node in list is first to be considered as a destination */

  /* Args - (Nodes usage hash, VM usage hash, intended migrations, current node, current VM) */
  mbool_t (*NodeCanBeDestination)(mhash_t *,mhash_t *,mln_t **,mnode_migrate_t *,mvm_t *);/* Return TRUE if done evaluating current node for migrations */

  }mvm_migrate_policy_if_t;

#endif /* __MVMMIGRATION_H__ */
