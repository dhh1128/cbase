/* HEADER */

/**
 * Contains common functions for VM migration policies
 *
 */


#include "moab.h"
#include "moab-proto.h"
#include "moab-const.h"
#include "moab-global.h"



/**
 * Sorts two nodes from low load to high load.
 *
 * Evaluates on memload, procload, mem alloc, then proc alloc.
 *
 * @param N1P [I] - First node
 * @param N2P [I] - Second node
 */

int OrderNodesLowToHighLoad(

  mnode_migrate_t **N1P,
  mnode_migrate_t **N2P)

  {
  double P1; /* For calculating percentages for N1 */
  double P2; /* For calculating percentages for N2 */

  mnode_migrate_t *N1 = *N1P;
  mnode_migrate_t *N2 = *N2P;

  /* Memory load */

  P1 = ((double)N1->Usage->MemLoad / MAX(1.0,(double)MNodeGetBaseValue(N1->N,mrlMem)));
  P2 = ((double)N2->Usage->MemLoad / MAX(1.0,(double)MNodeGetBaseValue(N2->N,mrlMem)));

  if (P1 > P2)
    return(1);
  else if (P2 > P1)
    return(-1);

  /* Proc load */

  P1 = (N1->Usage->ProcsLoad / MAX(1.0,(double)MNodeGetBaseValue(N1->N,mrlProc)));
  P2 = (N2->Usage->ProcsLoad / MAX(1.0,(double)MNodeGetBaseValue(N2->N,mrlProc)));

  if (P1 > P2)
    return(1);
  else if (P2 > P1)
    return(-1);

  /* Mem alloc */

  P1 = ((double)N1->Usage->Mem / MAX(1.0,(double)N1->N->CRes.Mem));
  P2 = ((double)N2->Usage->Mem / MAX(1.0,(double)N2->N->CRes.Mem));

  if (P1 > P2)
    return(1);
  else if (P2 > P1)
    return(-1);

  /* Procs alloc */

  P1 = ((double)N1->Usage->Procs / MAX(1.0,(double)N1->N->CRes.Procs));
  P2 = ((double)N2->Usage->Procs / MAX(1.0,(double)N2->N->CRes.Procs));

  if (P1 > P2)
    return(1);
  else if (P2 > P1)
    return(-1);

  return(0);
  }  /* END OrderNodesLowToHighLoad() */




/**
 * Sorts two nodes from high load to low load.
 *
 * Evaluates on memload, procload, mem alloc, then proc alloc.
 *
 * @param N1P [I] - First node
 * @param N2P [I] - Second node
 */

int OrderNodesHighToLowLoad(

  mnode_migrate_t **N1P,
  mnode_migrate_t **N2P)

  {
  /* Order most loaded first */

  double P1; /* For calculating percentages for N1 */
  double P2; /* For calculating percentages for N2 */

  mnode_migrate_t *N1 = *N1P;
  mnode_migrate_t *N2 = *N2P;

  /* Mem Load */

  P1 = ((double)N1->Usage->MemLoad / MAX(1.0,(double)MNodeGetBaseValue(N1->N,mrlMem)));
  P2 = ((double)N2->Usage->MemLoad / MAX(1.0,(double)MNodeGetBaseValue(N2->N,mrlMem)));

  if (P1 > P2)
    return(-1);
  else if (P1 < P2)
    return(1);

  /* Proc Load */

  P1 = (N1->Usage->ProcsLoad / MAX(1.0,(double)MNodeGetBaseValue(N1->N,mrlProc)));
  P2 = (N2->Usage->ProcsLoad / MAX(1.0,(double)MNodeGetBaseValue(N2->N,mrlProc)));

  if (P1 > P2)
    return(-1);
  else if (P1 < P2)
    return(1);

  /* Mem Allocated */

  P1 = ((double)N1->Usage->Mem / MAX(1.0,(double)N1->N->CRes.Mem));
  P2 = ((double)N2->Usage->Mem / MAX(1.0,(double)N2->N->CRes.Mem));

  if (P1 > P2)
    return(-1);
  else if (P1 < P2)
    return(1);

  /* Proc Allocated */

  P1 = ((double)N1->Usage->Procs / MAX(1.0,(double)N1->N->CRes.Procs));
  P2 = ((double)N2->Usage->Procs / MAX(1.0,(double)N2->N->CRes.Procs));

  if (P1 > P2)
    return(-1);
  else if (P1 < P2)
    return(1);

  return(0);
  }  /* END OrderNodesHighToLowLoad() */



/**
 * Orders VMs from biggest to smallest
 * Order of evaluation: memory used, procs used, mem allocated, procs allocated
 *
 * @param VM1P - First VM
 * @param VM2P - Second VM
 */

int OrderVMsBigToSmall(

  mvm_migrate_t **VM1P,
  mvm_migrate_t **VM2P)

  {
  int VM1Int;  /* For storing calculations for VM1 */
  int VM2Int;  /* For storing calculations for VM2 */

  mvm_migrate_t *VM1 = *VM1P;
  mvm_migrate_t *VM2 = *VM2P;

  /* Memory used */

  VM1Int = (VM1->VM->CRes.Mem - VM1->VM->ARes.Mem);
  VM2Int = (VM2->VM->CRes.Mem - VM2->VM->ARes.Mem);

  if (VM1Int > VM2Int)
    return(-1);
  else if (VM2Int > VM1Int)
    return(1);

  /* Procs used */

  if (VM1->VM->CPULoad > VM2->VM->CPULoad)
    return(-1);
  else if (VM2->VM->CPULoad > VM1->VM->CPULoad)
    return(1);

  /* Mem allocated */

  if (VM1->VM->CRes.Mem > VM2->VM->CRes.Mem)
    return(-1);
  else if (VM2->VM->CRes.Mem > VM1->VM->CRes.Mem)
    return(1);

  /* Procs allocated */

  if (VM1->VM->CRes.Procs > VM2->VM->CRes.Procs)
    return(-1);
  else if (VM2->VM->CRes.Procs > VM1->VM->CRes.Procs)
    return(1);

  return(0);
  }  /* END OrderVMsBigToSmall() */



/**
 * Orders VMs from smallest to biggest
 * Order of evaluation: memory used, procs used, mem allocated, procs allocated
 *
 * @param VM1P - First VM
 * @param VM2P - Second VM
 */

int OrderVMsSmallToBig(

  mvm_migrate_t **VM1P,
  mvm_migrate_t **VM2P)

  {
  int VM1Int;  /* For storing calculations for VM1 */
  int VM2Int;  /* For storing calculations for VM2 */

  mvm_migrate_t *VM1 = *VM1P;
  mvm_migrate_t *VM2 = *VM2P;

  /* Memory used */

  VM1Int = (VM1->VM->CRes.Mem - VM1->VM->ARes.Mem);
  VM2Int = (VM2->VM->CRes.Mem - VM2->VM->ARes.Mem);

  if (VM1Int > VM2Int)
    return(1);
  else if (VM2Int > VM1Int)
    return(-1);

  /* Procs used */

  if (VM1->VM->CPULoad > VM2->VM->CPULoad)
    return(1);
  else if (VM2->VM->CPULoad > VM1->VM->CPULoad)
    return(-1);

  /* Mem allocated */

  if (VM1->VM->CRes.Mem > VM2->VM->CRes.Mem)
    return(1);
  else if (VM2->VM->CRes.Mem > VM1->VM->CRes.Mem)
    return(-1);

  /* Procs allocated */

  if (VM1->VM->CRes.Procs > VM2->VM->CRes.Procs)
    return(1);
  else if (VM2->VM->CRes.Procs > VM1->VM->CRes.Procs)
    return(-1);

  return(0);
  }  /* END OrderVMsSmallToBig() */
