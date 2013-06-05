/* HEADER */

      
/**
 * @file MTrigList.c
 *
 * Contains: Trigger List functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 * Returns a trigger list as an mstring_t.
 *
 * @param TList (I)
 * @param ShowList (I)
 * @param Buf (O)  [must be initialized]
 */

int MTListToMString(

  marray_t  *TList,    /* I */
  mbool_t    ShowList, /* I */
  mstring_t *Buf)      /* O */

  {
  int      tindex;

  mtrig_t *T;
  
  if ((TList == NULL) || (Buf == NULL))
    {
    return(FAILURE);
    }

  Buf->clear();

  /* FORMAT:  [<NAME>$]<ETYPE>[+<OFFSET>][@<THRESHOLD>];<ATYPE>[@<ADATA>][;<ESTATE>] */

  for (tindex = 0;tindex < TList->NumItems;tindex++)
    {
    T = (mtrig_t *)MUArrayListGetPtr(TList,tindex);

    if (MTrigIsReal(T) == FALSE)
      continue;

    if (!Buf->empty())
      MStringAppend(Buf,",");

    MStringAppendF(Buf,"%s$%s+%ld@%f:%s@%s:%s",
      T->TName,
      MTrigType[T->EType],
      T->Offset,
      T->Threshold,
      MTrigActionType[T->AType],
      (T->Action != NULL) ? T->Action : NONE,
      (bmisset(&T->InternalFlags,mtifIsComplete)) ? "TRUE" : "FALSE");

    if (ShowList == FALSE)
      break;
    }  /* END for (tindex) */

  return(SUCCESS);
  }  /* END MTListToMString() */





/**
 *
 *
 * @param TListP (I) [modified]
 */

int MTListDestroy(

  marray_t **TListP)

  {
  /* remove trigger pointer list and all object triggers */

  int tindex;

  mtrig_t *T;

  marray_t *TList;

  if ((TListP == NULL) || (*TListP == NULL))
    {
    return(SUCCESS);
    }

  TList = *TListP;

  for (tindex = 0;tindex < TList->NumItems;tindex++)
    {
    T = (mtrig_t *)MUArrayListGetPtr(TList,tindex);

    MTrigFree(&T);
    }  /* END for (TPtr) */

  MUFree((char **)TListP);
 
  return(SUCCESS);
  }  /* END MTListDestroy() */




/**
 * Copy trigger list.
 *
 * WARNING: please read moabdocs.h as this function's behavior has changed with Moab 6.0
 *
 * NOTE: copy configuration and policies but not state.
 *
 * NOTE: this routine can add the copied triggers to the global trigger table but it will not
 *       initialize them (ie. they may be marked as templates).
 *
 * NOTE: if you don't want to add the trigger to the global table set AddGlobal=FALSE. You will
 *       need to make sure you handle the memory that is alloc'ed here (see J->PrologTList for an
 *       example of how to handle this case).
 *
 * NOTE: if (AddGlobal == FALSE) the trigger will have "mtifIsAlloc" set.
 *
 * @see MTrigFree() - peer
 * @see moabdocs.h
 *
 * @param STList    (I)
 * @param DTList    (O)
 * @param AddGlobal (I)
 */

int MTListCopy(

  marray_t   *STList,
  marray_t  **DTList,
  mbool_t     AddGlobal)

  {
  int sindex;
  int dindex;

  mtrig_t  *S;
  mtrig_t  *D;
  mtrig_t  *T = NULL;

  marray_t *DList;

  if ((STList == NULL) || (DTList == NULL))
    {
    return(FAILURE);
    }

  if (*DTList == NULL)
    {
    if ((*DTList = (marray_t *)MUCalloc(1,sizeof(marray_t))) == NULL)
      {
      return(FAILURE);
      }

    MUArrayListCreate(*DTList,sizeof(mtrig_t *),10);
    }  /* END if (*DTList == NULL) */

  DList = *DTList;

  for (sindex = 0;sindex < STList->NumItems;sindex++)
    {
    S = (mtrig_t *)MUArrayListGetPtr(STList,sindex);

    if (MTrigIsValid(S) == FALSE)
      {
      /* do not call MTrigIsReal() as we want to copy templates and such */

      continue;
      }

    if ((S->Name != NULL) && 
        (MTrigFindByName(S->Name,DList,NULL) == SUCCESS))
      continue;

    for (dindex = 0;dindex < DList->NumItems;dindex++)
      {
      D = (mtrig_t *)MUArrayListGetPtr(DList,dindex);

      if (MTrigIsValid(D) == FALSE)
        break;
      }

    if (dindex >= DList->NumItems)
      {
      /* no free space, we need to add a new trigger */

      if (AddGlobal == TRUE)
        {
        MTrigAdd(NULL,NULL,&T);
        }
      else
        {
        T = (mtrig_t *)MUCalloc(1,sizeof(mtrig_t));

        MUStrDup(&T->TName,"nonglobal-trigger");
        }

      MUArrayListAppendPtr(DList,T);

      dindex = DList->NumItems - 1;
      }

    D = (mtrig_t *)MUArrayListGetPtr(DList,dindex);

    MTrigCopy(D,S);

    if ((AddGlobal == FALSE) && (T != NULL))
      {
      bmset(&T->InternalFlags,mtifIsAlloc);
      }
    }  /* END for (sindex) */

  return(SUCCESS);
  }  /* END MTListCopy() */





/**
 *
 *
 * @param TList (I)
 * @param IsCancel (I)
 */

int MTListClearObject(

  marray_t *TList,     /* I */
  mbool_t   IsCancel)  /* I */

  {
  int tindex;

  mtrig_t *T;

  if (TList == NULL)
    {
    return(SUCCESS);
    }

  /* NOTE:  further enhancements include leaving triggers that
            fire with a 'cancel' or 'end' */

  for (tindex = 0;tindex < TList->NumItems;tindex++)
    {
    T = (mtrig_t *)MUArrayListGetPtr(TList,tindex);

    if ((T->State == mtsNONE) && (T->IsExtant == TRUE) && 
        (!bmisset(&T->InternalFlags,mtifIsTemplate)))
      {
      /* if trigger fires with 'cancel' leave it */

      if ((IsCancel == TRUE) && (T->EType == mttCancel))
        {
        MDB(3,fSTRUCT) MLog("INFO:     cancel trigger '%s' detected, checking dependecies\n",
          T->TName);

        T->ETime = MSched.Time;
 
        MSchedCheckTriggers(TList,-1,NULL);

        T->O = NULL;
        bmset(&T->InternalFlags,mtifOIsRemoved);

        continue;
        } /* END if ((IsCancel == TRUE) && (T->EType == mttCancel)) */
      else if ((IsCancel == FALSE) && (T->EType == mttEnd))
        {
        T->ETime = MSched.Time;

        MSchedCheckTriggers(TList,-1,NULL);

        T->O = NULL;
        bmset(&T->InternalFlags,mtifOIsRemoved);

        continue;
        }
      }   /* END if ((T->State == mtsNONE) && ...) */

    if (T->TName != NULL)
      MTrigFree(&T);
    }  /* END for (tindex) */   

  return(SUCCESS);
  }  /* END MTListClearObject() */



/**
 * Returns TRUE if there is a valid trigger in TList that is the same as ST
 *   (same address or same TName)
 *
 * @param TList
 * @param ST
 */

mbool_t MTrigInTList(
  marray_t *TList,
  mtrig_t  *ST)

  {
  int tindex;
  mtrig_t *T;

  if ((TList == NULL) || (ST == NULL))
    {
    return(FALSE);
    }

  for (tindex = 0;tindex < TList->NumItems;tindex++)
    {
    T = (mtrig_t *)MUArrayListGetPtr(TList,tindex);

    /* A trigger is not in the list if the one in the list is not a valid
        trigger and could be overwritten */

    if (MTrigIsValid(T) == FALSE)
      continue;

    if ((T == ST) || (!strcmp(T->TName,ST->TName)))
      {
      return(TRUE);
      }
    }  /* END for (tindex) */

  return(FALSE);
  }  /* END MTrigInTList() */


/**
 * Determines if the specified trigger exists in the GLOBAL list of triggers.
 *
 * returns a pointer if contained or a NULL if not contained
 *
 * @see MTrigIsEqual()
 *
 * @param Trig
 */

mtrig_t *MTListContains(
  
  mtrig_t  *Trig)

  {
  int tindex;
  mtrig_t *tmpT;

  for (tindex = 0;tindex < MTList.NumItems;tindex++)
    {
    tmpT = (mtrig_t *)MUArrayListGetPtr(&MTList,tindex);

    if (MTrigIsReal(tmpT) == FALSE)
      continue;

    if (MTrigIsEqual(tmpT,Trig) == TRUE)
      return(tmpT);
    }

  return(NULL);
  } /* END mbool_t MTListContains() */

/* END MTrigList.c */
