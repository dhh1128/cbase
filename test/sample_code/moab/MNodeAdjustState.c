/* HEADER */

/**
 * @file MNodeAdjustState.c
 * 
 * Contains various functions that operate on MNode structure
 *
 */


#include "moab.h"
#include "moab-proto.h"
#include "moab-const.h"
#include "moab-global.h"


/**
 *
 *
 * @param N (I)
 * @param State (I)
 */

int MNodeAdjustState(

  mnode_t             *N,      /* I */
  enum MNodeStateEnum *State)  /* I */

  {
  const char *FName = "MNodeAdjustState";

  MDB(7,fSTRUCT) MLog("%s(%s,%s)\n",
    FName,
    (N != NULL) ? N->Name : "NULL",
    (State != NULL) ? "State" : "NULL");

  if ((N == NULL) || (State == NULL))
    {
    return(FAILURE);
    }

  switch (*State)
    {
    case mnsDown:
    case mnsNone:
    case mnsDraining:

      break;

    case mnsIdle:
    case mnsActive:   
    case mnsBusy:

      if (N->CRes.Procs == 0)
        {
        /* don't do any of this kind of manipulation for nodes without procs, 
           just let the other routines handle virtual resources */

        break;
        }

      if (N->ARes.Procs == -1)
        {
        *State = mnsUp;
        }
      else if ((N->ARes.Procs <= 0) || (N->DRes.Procs >= N->CRes.Procs))
        {
        *State = mnsBusy;
        }
      else if ((N->ARes.Procs >= N->CRes.Procs) && (N->DRes.Procs == 0))
        {
        *State = mnsIdle;
        }
      else
        {
        *State = mnsActive;
        }

      break;

    default:

      /* do not modify state */

      /* NO-OP */

      break;
    }  /* END switch (*State) */

  MDB(7,fSTRUCT) MLog("INFO:     node %s state/estate set to %s\n",
    N->Name,
    MNodeState[*State]);

  return(SUCCESS);
  }  /* END MNodeAdjustState() */


