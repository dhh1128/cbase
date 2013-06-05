/* HEADER */

/**
 * @file MUActions.c
 * 
 * Contains various functions for Actions
 *
 */


#include "moab.h"
#include "moab-proto.h"
#include "moab-const.h"
#include "moab-global.h"




/**
 * Add pending action to object.
 *
 * @see MURemoveAction() - peer
 *
 * @param AList (I) [modified]
 * @param AName (I)
 * @param J (I)
 */

int MUAddAction(

  mln_t  **AList,  /* I (modified) */
  char    *AName,  /* I */
  mjob_t  *J)      /* I */

  {
  return(MULLAdd(AList,J->Name,NULL,NULL,MUFREE));
  }  /* END MUAddAction() */





/**
 *
 * @param Table      (I) - the actions table to be freed
 * @param CancelJobs (I) - if TRUE, will cancel jobs referenced in the actions table
 */

int MUFreeActionTable(

  mln_t   **Table,
  mbool_t   CancelJobs)

  {
  mjob_t *J;

  if (CancelJobs == TRUE)
    {
    mln_t *tmpL = NULL;

    for (tmpL = *Table;tmpL != NULL;tmpL = tmpL->Next)
      {
      if (MJobFind(tmpL->Name,&J,mjsmBasic) == SUCCESS)
        {
        bmset(&J->IFlags,mjifCancel);
        }
      }
    }  /* END for (CancelJobs == TRUE) */

  return(MULLFree(Table,MUFREE));
  } /* END MUFreeActionTable() */




/**
 * Remove action from action table
 *
 * @see MUAddAction() - peer
 *
 * @param AList (I) [modified] - Action list head
 * @param AName (I)            - Action name to add
 */

int MURemoveAction(

  mln_t  **AList,  /* I (modified) */
  char    *AName)  /* I */

  {
  return(MULLRemove(AList,AName,MUFREE));
  }  /* END MURemoveAction() */





mbool_t MNodeCheckAction(

  mnode_t                  *N,     /* I */
  enum MSystemJobTypeEnum   AType, /* I (use msjtNONE for any) */
  mjob_t                  **JP)    /* O (optional) */

  {
  mln_t *L;
  mvm_t *V;

  if (JP != NULL)
    *JP = NULL;

  if (N == NULL)
    {
    return(FALSE);
    }

  for (L = N->VMList;L != NULL;L = L->Next)
    {
    V = (mvm_t *)L->Ptr;

    if (V != NULL)
      {
      if (MUCheckAction(&V->Action,AType,JP) == TRUE)
        {
        return(TRUE);
        }
      }
    }    /* END for (L) */

  return(FALSE);
  }  /* END MNodeCheckAction() */



/**
 * Searches the given action table for the job name
 *
 * @param AList (I) - The table of actions to check
 * @param AType (I) - The action that we're checking for
 * @param JP (O)    - Job pointer to the referenced job (if found)
 */

mbool_t MUCheckAction(

  mln_t                   **AList, /* I */
  enum MSystemJobTypeEnum   AType, /* I (use msjtNONE for any) */
  mjob_t                  **JP)    /* O (optional) */

  {
  mjob_t *J = NULL;
  mln_t *tmpL = NULL;

  if (JP != NULL)
    *JP = NULL;

  if (AList == NULL) 
    {
    return(FALSE);
    }

  for (tmpL = *AList;tmpL != NULL;tmpL = tmpL->Next)
    {
    if (MJobFind(tmpL->Name,&J,mjsmBasic) == FAILURE)
      {
      continue;
      }

    if (J->System == NULL)
      {
      continue;
      }

    if ((AType != msjtNONE) && (AType != J->System->JobType))
      {
      continue;
      }

    if (JP != NULL)
      *JP = J;

    return(TRUE);
    }  /* END for (tmpL) */

  return(FALSE);
  }  /* END MUCheckAction() */


