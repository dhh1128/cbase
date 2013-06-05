/* HEADER */

      
/**
 * @file MTrigFind.c
 *
 * Contains: Trigger Find functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 * Locate trigger with specified trigger ID in global trigger table.
 *
 * @see MTrigFindByName()
 *
 * @param TName (I)
 * @param TP (O)
 */

int MTrigFind(

  const char *TName,
  mtrig_t   **TP)

  {
  int      tindex;
  mtrig_t *T;

  if (TP != NULL)
    *TP = NULL;

  /* first search in hashtable indices */

  if (MUHTGet(&MTUniqueNameHash,TName,(void **)TP,NULL) == SUCCESS)
    {
    return(SUCCESS);
    }

  if (MUHTGet(&MTSpecNameHash,TName,(void **)TP,NULL) == SUCCESS)
    {
    return(SUCCESS);
    }

  /* search manually based on trigger id */
  /* NOTE: if the above indices are working 100% correctly,
   * the below code is unnecessary */

  for (tindex = 0;tindex < MTList.NumItems;tindex++)
    {
    T = (mtrig_t *)MUArrayListGetPtr(&MTList,tindex);

    /* NOTE:  T->TName SHOULD mark end of list but does not currently */
    /*        removed triggers marked via T->IsExtant = FALSE         */

    if (MTrigIsValid(T) == FALSE)
      {
      /* empty trigger */

      continue;
      }

    if (!strcmp(TName,T->TName))
      {
      /* trigger located */

      if (TP != NULL)
        *TP = T;

      return(SUCCESS);
      }

    /* search based on trigger name */

    if ((T->Name == NULL) || (T->Name[0] == '\0'))
      continue;

    if (!strcmp(TName,T->Name))
      {
      /* trigger located */

      if (TP != NULL)
        *TP = T;

      return(SUCCESS);
      }
    }    /* END for (tindex) */

  return(FAILURE);
  }  /* END MTrigFind() */




/**
 * Locate trigger by user-specified trig name in the given trigger list.
 *
 * @see MTrigFind()
 *
 * @param Name (I)
 * @param TList (I)
 * @param TP (O)
 */

int MTrigFindByName(

  const char  *Name,
  marray_t    *TList,
  mtrig_t    **TP)

  {
  int      tindex;
  mtrig_t *T;

  if (TList == NULL)
    {
    return(FAILURE);
    }

  if (TP != NULL)
    *TP = NULL;

  if (TList == &MTList)
    {
    /* first check the index if this is the global trig list */

    if (MUHTGet(&MTSpecNameHash,Name,(void **)TP,NULL) == SUCCESS)
      {
      return(SUCCESS);
      }
    }

  for (tindex = 0;tindex < TList->NumItems;tindex++)
    {
    T = (mtrig_t *)MUArrayListGetPtr(TList,tindex);

    if ((MTrigIsValid(T) == FALSE) || (T->Name == NULL))
      continue;

    if (!strcmp(Name,T->Name))
      {
      /* trigger located */

      if (TP != NULL)
        *TP = T;

      return(SUCCESS);
      }
    }    /* END for (tindex) */
  
  return(FAILURE);
  }  /* END MTrigFindByName() */
 
/* END MTrigFind.c */
