/* HEADER */

      
/**
 * @file MVMFind.c
 *
 * Contains: MVMFind and MVMFindOrAdd amd MVMFindCompleted
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 * Locate specified VM by VM ID.
 *
 * NOTE: set VP to correct structure if found.
 *
 * NOTE: compare is case insensitive
 *
 * @see MUHTGet() - child - hash table lookup routine
 * @see MSched.VMTable[] - global VM table
 * @see MNodeFind() - peer - find routine for node objects
 * @see design doc VMManagementDoc
 *
 * @param VMID (I) VM id
 * @param VP (O) [optional]
 */

int MVMFind(

  const char *VMID,
  mvm_t     **VP)

  {
  mvm_t *V;
  mvoid_t *tmpVoid = NULL;

  if (VP != NULL)
    *VP = NULL;

  if ((VMID == NULL) || (VMID[0] == '\0'))
    {
    return(FAILURE);
    }

  if ((MUHTGet(&MSched.VMTable,VMID,(void **)&V,NULL) == SUCCESS) &&
      (V != NULL))
    {
    if (VP != NULL)
      *VP = V;
    
    return(SUCCESS);
    }

  /* Check if the VM is being referenced by a network alias */

  if ((MUHTGet(&MNetworkAliasesHT,VMID,(void **)&tmpVoid,NULL) == SUCCESS) &&
      (tmpVoid != NULL))
    {
    if ((tmpVoid->PtrType == mxoxVM) && (tmpVoid->Ptr != NULL))
      {
      if (VP != NULL)
        *VP = (mvm_t *)tmpVoid->Ptr;

      return(SUCCESS);
      }
    }

  return(FAILURE);
  }  /* END MVMFind() */


/**
 * Locate specified VM by VM ID in the completed table.
 *
 * NOTE: set VP to correct structure if found.
 *
 * @see MUHTGet() - child - hash table lookup routine
 * @see MSched.VMCompletedTable[] - global completed VM table
 * @see design doc VMManagementDoc
 *
 * @param VMID (I) VM id
 * @param VP (O) [optional] 
 *  
 * NOTE:  RETURNS SUCCESS IF VMID IS FOUND IN COMPLETED HASH
 * TABLE.  HOWEVER, THE VM MAY HAVE BEEN DESTROYED, IN WHICH 
 * CASE, WE RETURN SUCCESS WITH A NULL VM POINTER.
 */

int MVMFindCompleted(

  const char   *VMID,
  mvm_t       **VP)

  {
  mvm_t *V;

  if (VP != NULL)
    *VP = NULL;

  if ((VMID == NULL) || (VMID[0] == '\0'))
    {
    return(FAILURE);
    }

  if ((MUHTGet(&MSched.VMCompletedTable,VMID,NULL,NULL) == SUCCESS))
    {
    if (MVMFind(VMID,&V) == SUCCESS)
      {
      if (VP != NULL)
        *VP = V;
      }
    return(SUCCESS);
    }

  return(FAILURE);
  }  /* END MVMFindCompleted() */
