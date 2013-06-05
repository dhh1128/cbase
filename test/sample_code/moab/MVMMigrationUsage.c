/* HEADER */

/**
 * Contains functions for VM Migrations
 *
 */


#include "moab.h"
#include "moab-proto.h"
#include "moab-const.h"
#include "moab-global.h"

/* Local prototypes */

int __CalculateVMLoad(mvm_t *,mvm_migrate_usage_t *);
int __CalculateNodeLoad(mnode_t *,mhash_t *,mvm_migrate_usage_t *);
int __UpdateNodeLoadForMigrations(mnode_t *,mhash_t *,mvm_migrate_usage_t *);
int __AddVMAllocatedToNode(mnode_t *,int *,int *,int *,int *);
int __AddJobAllocatedToNode(mnode_t *,int *,int *,int *,int *);
mbool_t __WorkflowJobVMWasReported(mjob_t *);

/**
 * Calculate the node for each VM and hypervisor on the system.
 *
 * Loads will be placed into the provided hash tables.
 *
 * @param NodeLoads [I] - Initialized hash table to be populated with node load/usage
 * @param VMLoads   [I] - Initialized hash table to be populated with VM load/usage
 */

int CalculateNodeAndVMLoads(

  mhash_t *NodeLoads,
  mhash_t *VMLoads)

  {
  mnode_t *N = NULL;
  mvm_t *VM = NULL;
  mhashiter_t Iter;
  mvm_migrate_usage_t *Usage = NULL;
  int nindex = 0;

  const char *FName = "CalculateNodeAndVMLoads";

  MDB(7,fALL) MLog("%s(NodeLoads,VMLoads)\n",
    FName);

  if ((NodeLoads == NULL) ||
      (VMLoads == NULL))
    {
    return(FAILURE);
    }

  /* VM loads need to be calculated first so that if a VM is migrating, its load
      can be removed from the destination and added to the source.  This keeps
      us from doing a migration for overcommit when the thresholds may be
      satisfied by an already running/scheduled migration */

  /* VMs */

  MUHTIterInit(&Iter);

  while (MUHTIterate(&MSched.VMTable,NULL,(void **)&VM,NULL,&Iter) == SUCCESS)
    {
    if (VM == NULL)
      continue;

    Usage = (mvm_migrate_usage_t *)MUCalloc(1,sizeof(mvm_migrate_usage_t));

    if (__CalculateVMLoad(VM,Usage) == SUCCESS)
      {
      MDB(8,fMIGRATE) MLog("INFO:     Usage calculated for VM '%s'\n",
        VM->VMID);

      MUHTAdd(VMLoads,VM->VMID,(void *)Usage,NULL,MVMMIGRATEUSAGEFREE);
      }
    else
      {
      FreeVMMigrationUsage(&Usage);
      }
    }  /* END while (MUHTIterate(&MSched.VMTable) == SUCCESS) */

  /* Nodes */

  for (nindex = 0;nindex < MSched.M[mxoNode];nindex++)
    {
    N = MNode[nindex];

    if ((N == NULL) || (N->Name[0] == '\0'))
      break;

    if (NodeAllowsVMMigration(N) == FALSE)
      continue;

    Usage = (mvm_migrate_usage_t *)MUCalloc(1,sizeof(mvm_migrate_usage_t));

    if (__CalculateNodeLoad(N,VMLoads,Usage) == SUCCESS)
      {
      MDB(8,fMIGRATE) MLog("INFO:     Usage calculated for node '%s'\n",
        N->Name);

      MUHTAdd(NodeLoads,N->Name,(void *)Usage,NULL,MVMMIGRATEUSAGEFREE);
      }
    else
      {
      FreeVMMigrationUsage(&Usage);
      }
    }  /* END for (nindex) */


  return(SUCCESS);
  }  /* END CalculateNodeAndVMLoads() */


/**
 * Calculate the node for an individual VM.
 *
 * Allocated values are the VM's dedicated resources.
 * Load values are Dedicated - Available (but Procs just uses reported CPULoad)
 *
 * @param VM    [I] - The VM to get the usage for
 * @param Usage [O] - Already alloc'd structure to store usage in
 */

int __CalculateVMLoad(
  mvm_t *VM,
  mvm_migrate_usage_t *Usage)

  {
  if ((VM == NULL) || (Usage == NULL))
    return(FAILURE);

  MUStrDup(&Usage->Name,VM->VMID);

  /* Allocated */

  Usage->Procs = VM->CRes.Procs;
  Usage->Mem   = VM->CRes.Mem;
  Usage->Disk  = VM->CRes.Disk;
  Usage->Swap  = VM->CRes.Swap;

  /* Load */

  /* NOTE:  Loads are absolute numbers, not percentages.  Percentage has to be
      calculated for the VM vs. node for every node because the percentage can
      be different across nodes.

     Example: VM is using 7 GB of mem.  If it is currently on a node that has
      100 GB of mem, the "percentage" of usage would be 7%.  If we are looking
      at migrating the VM to a node that has 50 GB of mem, it would be adding
      14% memory usage, not 7%.

    CPULoad must be reported by the resource manager, and is a double of how many
      procs are being used (i.e. 6.5, etc.).
  */

  Usage->ProcsLoad = VM->CPULoad;
  Usage->MemLoad   = VM->CRes.Mem - VM->ARes.Mem;
  Usage->DiskLoad  = VM->CRes.Disk - VM->ARes.Disk;
  Usage->SwapLoad  = VM->CRes.Swap - VM->ARes.Swap;

  /* GMetrics */

  if (VM->GMetric == NULL)
    MGMetricCreate(&VM->GMetric);

  if (Usage->GMetric == NULL)
    MGMetricCreate(&Usage->GMetric);

  MGMetricCopy(Usage->GMetric,VM->GMetric);

  return(SUCCESS);
  }  /* END __CalculateVMLoad() */





/**
 * Calculates the current load for a node.  Also checks migrations that are
 *  currently active/queued and will add/remove those resources.
 *
 * Allocated values are sum of dedicated values for all jobs (except VM-related and noresource jobs)
 *  and all VMs (minus those that are migrating away, plus those that are migrating to this node).
 *
 * @param N        [I] - The node to calculate usage for
 * @param VMLoads  [I] - The hash table of VM loads
 * @param Usage    [O] - The output usage structure
 */

int __CalculateNodeLoad(

  mnode_t *N,
  mhash_t *VMLoads,
  mvm_migrate_usage_t *Usage)

  {
  int Procs = 0;
  int Mem = 0;
  int Swap = 0;
  int Disk = 0;

  if ((N == NULL) || (Usage == NULL))
    return(FAILURE);

  MUStrDup(&Usage->Name,N->Name);

  /* Allocated - Add up all resources from VMs, jobs */

  __AddVMAllocatedToNode(N,&Procs,&Mem,&Swap,&Disk);

  /* Add in any jobs */

  __AddJobAllocatedToNode(N,&Procs,&Mem,&Swap,&Disk);

  Usage->Procs = Procs;
  Usage->Mem   = Mem;
  Usage->Swap  = Swap;
  Usage->Disk  = Disk;

  /* We currently don't add reservations to the allocated because reservations
      can overlap, and we don't want just any reservation to trigger a
      migration */ 

  /* Load */

  Usage->ProcsLoad = N->Load;
  Usage->MemLoad   = (MNodeGetBaseValue(N,mrlMem) - N->ARes.Mem);
  Usage->SwapLoad  = (MNodeGetBaseValue(N,mrlSwap) - N->ARes.Swap);
  Usage->DiskLoad  = (MNodeGetBaseValue(N,mrlDisk) - N->ARes.Disk);

  /* GMetrics */

  MNodeInitXLoad(N);  /* Make sure that gmetrics exist */

  if (Usage->GMetric == NULL)
    {
    MGMetricCreate(&Usage->GMetric);
    }

  MGMetricCopy(Usage->GMetric,N->XLoad->GMetric);

  __UpdateNodeLoadForMigrations(N,VMLoads,Usage);

  return(SUCCESS);
  }  /* END __CalculateNodeLoad */



/**
 * Helper function to update the node usage for VMs
 *  migrating to and away from N.
 *
 * No error checking
 *
 * @see __CalculateNodeLoad() - parent
 *
 * @param N        [I] - The node to update usage for
 * @param VMLoads  [I] - The hash table of VM loads
 * @param Usage    [O] - N's usage structure
 */

int __UpdateNodeLoadForMigrations(
  mnode_t *N,
  mhash_t *VMLoads,
  mvm_migrate_usage_t *Usage)

  {
  mln_t *tmpL = NULL;
  mvm_t *VM = NULL;
  mln_t *VMsToN = NULL;
  mvm_migrate_usage_t *VMUsage = NULL;
  mnode_t *DstN = NULL;

  /* Migrations to N */

  if (GetVMsMigratingToNode(N,NULL,FALSE,&VMsToN) == SUCCESS)
    {
    for (tmpL = VMsToN;tmpL != NULL;tmpL = tmpL->Next)
      {
      VM = (mvm_t *)tmpL->Ptr;

      if (VM->N == N)
        {
        /* VM was already added */

        continue;
        }

      /* Found a VM migrating to this node, add its usage */

      if (MUHTGet(VMLoads,VM->VMID,(void **)&VMUsage,NULL) == FAILURE)
        {
        MDB(6,fMIGRATE) MLog("ERROR:    cannot find usage for VM '%s'\n",
          VM->VMID);

        continue;
        }

      AddVMMigrationUsage(Usage,VMUsage);
      }  /* END for (tmpL = VMsToN) */

    MULLFree(&VMsToN,NULL);
    }  /* END if (GetVMsMigratingToNode(N,NULL,FALSE,&VMsToN) == SUCCESS) */

  /* Migrations away from N */

  for (tmpL = N->VMList;tmpL != NULL;tmpL = tmpL->Next)
    {
    VM = (mvm_t *)tmpL->Ptr;

    if (VM == NULL)
      continue;

    if ((MVMGetCurrentDestination(VM,NULL,&DstN) == FAILURE) ||
        (DstN == N))
      {
      continue;
      }

    /* Found a VM migrating away, subtract its usage */

    if (MUHTGet(VMLoads,VM->VMID,(void **)&VMUsage,NULL) == FAILURE)
      {
      MDB(6,fMIGRATE) MLog("ERROR:    cannot find usage for VM '%s'\n",
        VM->VMID);

      continue;
      }

    SubtractVMMigrationUsage(Usage,VMUsage);
    }  /* END for (tmpL = N->VMList) */  

  return(SUCCESS);
  }  /* END __UpdateNodeLoadForMigrations() */




/**
 * Helper function to add allocated VM resources to node usage.
 * No error checking.
 *
 * This is not the most efficient way, since we will be iterating
 *  over all VMs multiple times - make it work, then optimize...
 *
 * @param N     [I] - Node to add resources to
 * @param Procs [O] - Procs
 * @param Mem   [O] - Mem
 * @param Swap  [O] - Swap
 * @param Disk  [O] - Disk
 */

int __AddVMAllocatedToNode(

  mnode_t *N,
  int *Procs,
  int *Mem,
  int *Swap,
  int *Disk)

  {
  mln_t *tmpL = NULL;
  mvm_t *VM = NULL;

  for (tmpL = N->VMList;tmpL != NULL;tmpL = tmpL->Next)
    {
    VM = (mvm_t *)tmpL->Ptr;

    if (VM == NULL)
      continue;

    /* We still add a VM if it is migrating away from the node here.  It's
        usage will be subtracted later.  This is done so that we can remove
        its loads from the node's load (not just configured resources) */

    *Procs += VM->CRes.Procs;
    *Mem   += VM->CRes.Mem;
    *Swap  += VM->CRes.Swap;
    *Disk  += VM->CRes.Disk;
    }  /* END for (tmpL = N->VMList) */

  /* VMs migrating to this node will be added later */

  return(SUCCESS);
  }  /* __AddVMAllocatedToNode() */


/**
 * Helper function to say whether a job is part of a VM workflow
 *  where the VM has already been reported.  This is because there
 *  is a small window of time where a VM has been reported, but
 *  a setup job is still running, so the VM's resources may be
 *  double counted in the migration calculations.  This could
 *  cause a node to be considered overcommitted when it is not.
 *
 * No param checking.
 *
 * Returns TRUE if there is an associated VM that has been reported.
 *
 * @param tmpJ - [I] The job to evaluate
 */

mbool_t __WorkflowJobVMWasReported(

  mjob_t *tmpJ)

  {
  mvc_t *WorkflowVC = NULL;
  mjob_t *TrackingJ = NULL;

  if ((MVCGetJobWorkflow(tmpJ,&WorkflowVC) == SUCCESS) &&
      (WorkflowVC != NULL))
    {
    mln_t *tmpL = NULL;

    /* Find the tracking job */

    for (tmpL = WorkflowVC->Jobs;tmpL != NULL;tmpL = tmpL->Next)
      {
      if ((tmpL->Ptr) == NULL)
        continue;

      if ((((mjob_t *)tmpL->Ptr)->System != NULL) &&
          (((mjob_t *)tmpL->Ptr)->System->JobType == msjtVMTracking))
        {
        TrackingJ = ((mjob_t *)tmpL->Ptr);
        break;
        }
      }  /* END for (tmpL = WorkflowVC->Jobs) */
    }  /* END if ((MVCGetJobWorkflow(tmpJ,&WorkflowVC) == SUCCESS) && ...) */
  else
    {
    return(FALSE);
    }

  char *VMIDPtr = NULL;

  /* See if the VM has already been reported */

  if ((TrackingJ != NULL) &&
      (MUHTGet(&TrackingJ->Variables,"VMID",(void **)&VMIDPtr,NULL) == SUCCESS) &&
      (!MUStrIsEmpty(VMIDPtr)) &&
      (MVMFind(VMIDPtr,NULL) == SUCCESS))
    {
    /* VM has already been reported.  Don't count this job's resources
        because they have already been counted by the VM object */

    return(TRUE);
    }

  return(FALSE);
  }  /* END __WorkflowJobVMWasReported() */



/**
 * Helper function to add allocated job resources to node usage.
 * No error checking.
 *
 * @param N     [I] - Node to add resources to
 * @param Procs [O] - Procs
 * @param Mem   [O] - Mem
 * @param Swap  [O] - Swap
 * @param Disk  [O] - Disk
 */

int __AddJobAllocatedToNode(

  mnode_t *N,
  int *Procs,
  int *Mem,
  int *Swap,
  int *Disk)

  {
  int jindex = 0;
  mjob_t *tmpJ = NULL;

  for (jindex = 0;jindex < N->MaxJCount;jindex++)
    {
    int reqindex = 0;
    mreq_t *RQ = NULL;

    tmpJ = N->JList[jindex];

    if (tmpJ == NULL)
      break;

    if (tmpJ == (mjob_t *)1)
      continue;

    if ((tmpJ->System != NULL) &&
        (tmpJ->System->JobType == msjtVMTracking))
      {
      /* VMs were already counted above */

      continue;
      }

    if (MVMJobIsVMMigrate(tmpJ))
      continue;

    if (bmisset(&tmpJ->Flags,mjfNoResources))
      continue;

    /* Check if job is part of a VM creation workflow where
        the VM has already been reported */

    if (__WorkflowJobVMWasReported(tmpJ) == TRUE)
      continue;

    for (reqindex = 0;reqindex < MMAX_REQ_PER_JOB;reqindex++)
      {
      int NodePosition = -1;
      int TC = -1;

      if (tmpJ->Req[reqindex] == NULL)
        break;

      RQ = tmpJ->Req[reqindex];

      if (MNLFind(&RQ->NodeList,N,&NodePosition) == FAILURE)
        continue;

      TC = MNLGetTCAtIndex(&RQ->NodeList,NodePosition);

      *Procs += (tmpJ->Req[reqindex]->DRes.Procs * TC);
      *Mem   += (tmpJ->Req[reqindex]->DRes.Mem * TC);
      *Disk  += (tmpJ->Req[reqindex]->DRes.Disk * TC);
      *Swap  += (tmpJ->Req[reqindex]->DRes.Swap * TC);
      }  /* END for (reqindex) */
    }  /* END for (jindex) */

  return(SUCCESS);
  }  /* END __AddJobAllocatedToNode() */


/**
 * Frees the given mvm_migrate_usage_t struct.
 *
 * @param UsageP [I] (modified) - Pointer to usage object to free
 */

int FreeVMMigrationUsage(
  mvm_migrate_usage_t **UsageP)
  {
  mvm_migrate_usage_t *Usage = NULL;

  if (UsageP == NULL)
    return(SUCCESS);

  Usage = *UsageP;

  if (Usage != NULL)
    {
    MUFree(&Usage->Name);

    MGMetricFree(&Usage->GMetric);

    MUFree((char **)UsageP);
    }

  *UsageP = NULL;

  return(SUCCESS);
  }  /* END FreeVMMigrationUsage() */


/**
 * Adds the usage from ToAdd to Dst.
 *
 * @param Dst   [I] (modified) - The usage to be added on to (node usage)
 * @param ToAdd [I] - The usage (vm usage) to add to dst
 */

int AddVMMigrationUsage(

  mvm_migrate_usage_t *Dst,
  mvm_migrate_usage_t *ToAdd)

  {
  if ((Dst == NULL) || (ToAdd == NULL))
    return(FAILURE);

  Dst->Procs += ToAdd->Procs;
  Dst->Mem   += ToAdd->Mem;
  Dst->Swap  += ToAdd->Swap;
  Dst->Disk  += ToAdd->Disk;

  Dst->ProcsLoad += ToAdd->ProcsLoad;
  Dst->MemLoad   += ToAdd->MemLoad;
  Dst->SwapLoad  += ToAdd->SwapLoad;
  Dst->DiskLoad  += ToAdd->DiskLoad;

  MGMetricAdd(Dst->GMetric,ToAdd->GMetric);

  return(SUCCESS);
  }  /* END AddVMMigrationUsage() */



/**
 * Subtracts the usage of ToSubtract from Dst.  Makes sure usage doesn't go below 0 (except for gmetrics).
 *
 * @param Dst        [I] (modified) - The usage to be added on to (node usage)
 * @param ToSubtract [I] - The usage (vm usage) to add to dst
 */

int SubtractVMMigrationUsage(

  mvm_migrate_usage_t *Dst,
  mvm_migrate_usage_t *ToSubtract)

  {
  if ((Dst == NULL) || (ToSubtract == NULL))
    return(FAILURE);

  Dst->Procs = MAX(0,Dst->Procs - ToSubtract->Procs);
  Dst->Mem   = MAX(0,Dst->Mem - ToSubtract->Mem);
  Dst->Swap  = MAX(0,Dst->Swap - ToSubtract->Swap);
  Dst->Disk  = MAX(0,Dst->Disk - ToSubtract->Disk);

  Dst->ProcsLoad = MAX(0,Dst->ProcsLoad - ToSubtract->ProcsLoad);
  Dst->MemLoad   = MAX(0,Dst->MemLoad - ToSubtract->MemLoad);
  Dst->SwapLoad  = MAX(0,Dst->SwapLoad - ToSubtract->SwapLoad);
  Dst->DiskLoad  = MAX(0,Dst->DiskLoad - ToSubtract->DiskLoad);

  MGMetricSubtract(Dst->GMetric,ToSubtract->GMetric);

  return(SUCCESS);
  }  /* END SubtractVMMigrationUsage() */
