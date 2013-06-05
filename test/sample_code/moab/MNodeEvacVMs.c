/* HEADER */


/**
 * @file MNodeEvacVMs.c
 *
 */

#include "moab.h"
#include "moab-proto.h"
#include "moab-const.h"
#include "moab-global.h"




/**
 * Migrates VMs away from a node.  This is a best-effort evacuation.
 *
 * @param N          [I] - The node to migrate VMs away from
 * @param SNodeLoads [I] - (optional) Specified node loads hash
 * @param SVMLoads   [I] - (optional) Specified VM loads hash
 */

int MNodeEvacVMs(

  mnode_t *N,
  mhash_t *SNodeLoads,
  mhash_t *SVMLoads)

  {
  mhash_t NL;
  mhash_t VML;
  mhash_t *NodeLoads;
  mhash_t *VMLoads;

  mvm_migrate_policy_if_t *Policy = NULL;

  mln_t *VMPtr = NULL;
  int MaxMigrations = -1;
  mln_t *IntendedMigrations = NULL;
  int NumberPerformed = 0;
  mvm_migrate_usage_t *SrcNUsage = NULL;

  const char *FName = "MNodeEvacVMs";

  MDB(4,fMIGRATE) MLog("%s(%s,%s,%s)\n",
    FName,
    (N != NULL) ? N->Name : "NULL",
    (SNodeLoads != NULL) ? "SNodeLoads" : "NULL",
    (SVMLoads != NULL) ? "SVMLoads" : "NULL");

  if (N == NULL)
    return(FAILURE);

  if (bmisset(&N->Flags,mnfNoVMMigrations))
    {
    MDB(5,fMIGRATE) MLog("INFO:     cannot migrate vms from node '%s', %s flag is set\n",
      N->Name,
      MNodeFlags[mnfNoVMMigrations]);

    return(FAILURE);
    }

  /* Get policy */

  if (VMOvercommitMigrationSetup(&Policy) == FAILURE)
    return(FAILURE);

  /* Calculate usages (if not given) */

  if ((SNodeLoads == NULL) || (SVMLoads == NULL))
    {
    MUHTCreate(&NL,-1);
    MUHTCreate(&VML,-1);

    NodeLoads = &NL;
    VMLoads = &VML;

    if (CalculateNodeAndVMLoads(NodeLoads,VMLoads) == FAILURE)
      {
      MDB(4,fMIGRATE) MLog("INFO:     failed to calculate load for nodes and VMs\n");

      MUHTFree(NodeLoads,TRUE,MVMMIGRATEUSAGEFREE);
      MUHTFree(VMLoads,TRUE,MVMMIGRATEUSAGEFREE);
      MUFree((char **)&Policy);

      return(FAILURE);
      }
    }  /* END if ((SNodeLoads == NULL) || (SVMLoads == NULL)) */
  else
    {
    NodeLoads = SNodeLoads;
    VMLoads = SVMLoads;
    }

  if (MUHTGet(NodeLoads,N->Name,(void **)&SrcNUsage,NULL) == FAILURE)
    {
    MDB(6,fMIGRATE) MLog("ERROR:    cannot find node usage information for node '%s'\n",
      N->Name);

    if ((SNodeLoads == NULL) || (SVMLoads == NULL))
      {
      MUHTFree(NodeLoads,TRUE,MVMMIGRATEUSAGEFREE);
      MUHTFree(VMLoads,TRUE,MVMMIGRATEUSAGEFREE);
      }

    MUFree((char **)&Policy);

    return(FAILURE);
    }

  /* Post GEvent for node chosen to be migrated */

  if (N->VMList != NULL)
    {
    char *msgPtr = NULL;

    MUStrDup(&msgPtr,"evacs_migrate:Node chosen for migration.");
    MNodeSetAttr(N,mnaGEvent,(void **)&msgPtr,mdfString,mSet);

    MUFree(&msgPtr);
    }

  while (MULLIterate(N->VMList,&VMPtr) == SUCCESS)
    {
    /* Iterate through each vm on current node */

    char tmpMsg[MMAX_LINE] = {0};

    mnode_t *DstN = NULL;

    mvm_t *VM = (mvm_t *)VMPtr->Ptr;
    mvm_migrate_usage_t *VMUsage = NULL;

    if (VM == NULL)
      continue;

    if (VMIsEligibleForMigration(VM,FALSE) == FALSE)
      {
      MDB(6,fMIGRATE) MLog("ERROR:    VM '%s' is not eligible for migration\n",
        VM->VMID);

      continue;
      }

    if (MUHTGet(VMLoads,VM->VMID,(void **)&VMUsage,NULL) == FAILURE)
      {
      MDB(6,fMIGRATE) MLog("ERROR:    cannot find VM usage information for VM '%s'\n",
        VM->VMID);

      continue;
      }

    if ((VMFindDestination(VM,Policy,NodeLoads,VMLoads,&IntendedMigrations,NULL,&DstN) == SUCCESS) &&
        (DstN != NULL))
      {
      char tmpName[MMAX_NAME];

      mvm_migrate_usage_t *DstNUsage = NULL;

      if (MUHTGet(NodeLoads,DstN->Name,(void **)&DstNUsage,NULL) == FAILURE)
        {
        MDB(6,fMIGRATE) MLog("ERROR:    cannot find node usage information for node '%s'\n",
          DstN->Name);

        continue;
        }

      /* Update usages */

      AddVMMigrationUsage(DstNUsage,VMUsage);
      SubtractVMMigrationUsage(SrcNUsage,VMUsage);

      /* Queue up the decision */

      QueueVMMigrate(VM,DstN,mvmmpOvercommit,&MaxMigrations,&IntendedMigrations);

      /* Write out events */

      snprintf(tmpMsg,sizeof(tmpMsg),"attempting to migrate vm '%s' from node '%s' to '%s' for evacvms",
        VM->VMID,
        N->Name,
        DstN->Name);

      MDB(5,fMIGRATE) MLog("INFO:     %s\n",
        tmpMsg);

      MUStrCpy(tmpName,"evacs_migrate",sizeof(tmpName));
      MVMAddGEvent(VM,tmpName,0,tmpMsg);
      MOWriteEvent((void *)VM,mxoxVM,mrelVMMigrate,tmpMsg,NULL,NULL);
      }  /* END if (VMFindDestination() == SUCCESS) */
    } /* END while (MULLIterate(N->VMList,&VMPtr) == SUCCESS) */

  /* Do the migrations */

  PerformVMMigrations(&IntendedMigrations,&NumberPerformed,"EVACVMS");

  /* Cleanup */

  if ((SNodeLoads == NULL) || (SVMLoads == NULL))
    {
    MUHTFree(NodeLoads,TRUE,MVMMIGRATEUSAGEFREE);
    MUHTFree(VMLoads,TRUE,MVMMIGRATEUSAGEFREE);
    }

  MUFree((char **)&Policy);

  return(SUCCESS);
  }  /* END MNodeEvacVMs() */



/**
 * Migrates VMs away from a list of nodes.  This is a best-effort evacuation.
 *
 * @param Nodes [I] - The comma-delimited list of node names to migrate VMs away from
 */

int MNodeEvacVMsMultiple(

  const mnl_t *Nodes)

  {
  int nindex;

  mhash_t NodeLoads;
  mhash_t VMLoads;

  mnode_t *N = NULL;

  const char *FName = "MNodeEvacVMsMultiple";

  MDB(4,fMIGRATE) MLog("%s(Nodes)\n",
    FName);

  if (Nodes == NULL)
    {
    return(FAILURE);
    }

  MUHTCreate(&NodeLoads,-1);
  MUHTCreate(&VMLoads,-1);

  if (CalculateNodeAndVMLoads(&NodeLoads,&VMLoads) == FAILURE)
    {
    MDB(4,fMIGRATE) MLog("INFO:     failed to calculate load for nodes and VMs\n");

    MUHTFree(&NodeLoads,TRUE,MVMMIGRATEUSAGEFREE);
    MUHTFree(&VMLoads,TRUE,MVMMIGRATEUSAGEFREE);

    return(FAILURE);
    }

  for (nindex = 0;MNLGetNodeAtIndex(Nodes,nindex,&N) == SUCCESS;nindex++)
    {
    MNodeEvacVMs(N,&NodeLoads,&VMLoads);
    }  /* END while (ptr != NULL) */

  MUHTFree(&NodeLoads,TRUE,MVMMIGRATEUSAGEFREE);
  MUHTFree(&VMLoads,TRUE,MVMMIGRATEUSAGEFREE);

  return(SUCCESS);
  }  /* END MNodeEvacVMsMultiple() */
