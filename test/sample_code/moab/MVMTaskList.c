/* HEADER */

/**
 * @file MVMTaskList.c
 * 
 * Contains various functions for VM Tasklists
 *
 */


#include "moab.h"
#include "moab-proto.h"
#include "moab-const.h"
#include "moab-global.h"



/**
 * Convert virtual machine string to physical node tasklist.
 *
 * @see designdoc VMManagementDoc
 *
 * @param VMString (I) [modified]
 * @param VMTaskMap (O) [optional]
 * @param TL (O) [minsize=TLSize]
 * @param TLSize (I)
 * @param MarkActive (I)
 * @param VMUsage (I)
 */

int MVMStringToTL(

  char                   *VMString,   /* I (modified) */
  mvm_t                 **VMTaskMap,  /* O (optional) */
  int                    *TL,         /* O (minsize=TLSize) */
  int                     TLSize,     /* I */
  mbool_t                 MarkActive, /* I */
  enum MVMUsagePolicyEnum VMUsage)    /* virtual machine usage policy */

  {
  char *ptr;
  char *TokPtr;

  int   tindex;

  mvm_t   *V;

  /* FORMAT:  <VM>[,<VM>]... */

  if ((VMString == NULL) || (TL == NULL))
    {
    return(FAILURE);
    }

  ptr = MUStrTok(VMString,",",&TokPtr);
 
  tindex = 0;

  while (ptr != NULL)
    {
    if ((MUHTGet(&MSched.VMTable,ptr,(void **)&V,NULL) == SUCCESS) && 
        (V != NULL) &&
        (V->N != NULL))
      {
      if (MarkActive == TRUE)
        {
        V->ARes.Procs = 0;

        if (V->State == mnsIdle)
          V->State = mnsBusy;
        }

      TL[tindex] = V->N->Index;

      if (VMTaskMap != NULL)
        VMTaskMap[tindex] = V;

      tindex++;

      if (tindex >= TLSize)
        break;
      }

    ptr = MUStrTok(NULL,",",&TokPtr);
    }  /* END while (ptr != NULL) */

  TL[tindex] = -1;

  return(SUCCESS);
  }  /* END MVMStringToTL() */


