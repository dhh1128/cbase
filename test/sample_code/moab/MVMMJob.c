
/* HEADER */

/**
 * @file MVMMJob.c
 * 
 * Contains various functions VM and Job interactions
 *
 */


#include "moab.h"
#include "moab-proto.h"
#include "moab-const.h"
#include "moab-global.h"





/**
 * Locate VM's running on node N which are allocated to job J.
 */

int MJobFindNodeVM(

  mjob_t   *J,
  mnode_t  *N,
  mvm_t   **VP)  /* O (optional) */

  {
  mvm_t *V;
  mln_t *vptr;

  if (VP != NULL) 
    *VP = NULL;

  if ((J == NULL) || (N == NULL) || (N->VMList == NULL))
    {
    return(FAILURE);
    }

  /* locate VM on N allocated to job J */

  /* NOTE:  assumes VM is dedicated */

  for (vptr = N->VMList;vptr != NULL;vptr = vptr->Next)
    {
    V = (mvm_t *)vptr->Ptr;

    if ((V != NULL) && !strcmp(V->JobID,J->Name))
      {
      if (VP != NULL)
        *VP = V;

      return(SUCCESS);
      }
    }    /* END for (vptr) */

  return(FAILURE);
  }  /* END MJobFindNodeVM() */




/* END MVMMJob.c */
