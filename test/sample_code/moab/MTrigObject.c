/* HEADER */

      
/**
 * @file MTrigObject.c
 *
 * Contains: Trigger Object Functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  

/**
 * Register object event with trigger subsystem.
 *
 * @see MOWriteEvent() - peer - write event to event file 
 *
 * @param O (I) [optional]
 * @param OID (I) [optional]
 * @param OType (I)
 * @param EType (I)
 * @param ETime (I)
 * @param ForceUpdate (I)
 */

int MOReportEvent(

  void               *O,            /* I (optional) */
  char               *OID,          /* I (optional) */
  enum MXMLOTypeEnum  OType,        /* I */
  enum MTrigTypeEnum  EType,        /* I */
  mulong              ETime,        /* I */
  mbool_t             ForceUpdate)  /* I */

  {
  int tindex;

  mtrig_t *T;
#ifdef MYAHOO
  mtrig_t **TList = NULL;
#endif /* MYAHOO */

  /* NOTE:  ETime may be in future for scheduled events */
#ifdef MYAHOO

  /* NOTE: with Yahoo, we only send events to triggers belonging to the same object. This may not work
   * for Yahoo always--it just works for right now. This also may not work for other customers. */

  if (O != NULL)
    {
    switch (OType)
      {
      case mxoAcct:
      case mxoGroup:
      case mxoUser:

        TList = ((mgcred_t *)O)->TList;

        break;

      case mxoClass:

        TList = ((mclass_t *)O)->TList;

        break;

      case mxoJob:

        TList = ((mjob_t *)O)->Triggers;

        break;

      case mxoNode:

        TList = ((mnode_t *)O)->T;

        break;

      case mxoQOS:

        TList = ((mqos_t *)O)->TList;

        break;

      case mxoRM:

        TList = ((mrm_t *)O)->T;

        break;

      case mxoRsv:

        TList = ((mrsv_t *)O)->T;

        break;

      case mxoSched:

        TList = ((msched_t *)O)->TList;

        break;

      case mxoSRsv:

        TList = ((msrsv_t *)O)->T;

        break;

      default:

        /* un-supported trigger object */

        return(FAILURE);

        /*NOTREACHED*/

        break;
      }

    if (TList == NULL)
      {
      /* no triggers to process */

      return(SUCCESS);
      }
    }  /* END if (O != NULL) */

#endif /* MYAHOO */

  for (tindex = 0;tindex < MTList.NumItems;tindex++)
    {
#ifdef MYAHOO
    T = TList[tindex];

    if (T == NULL)
      break;
#else
    T = (mtrig_t *)MUArrayListGetPtr(&MTList,tindex);
#endif /* MYAHOO */

    if (MTrigIsReal(T) == FALSE)
      continue;

    if (T->EType != EType)
      continue;

#ifndef MYAHOO
    /* Verify trigger object.  If not, try looking it up. */

    if (T->O == NULL)
      MOGetObject(T->OType,T->OID,&T->O,mVerify);

    if ((O != NULL) && (T->O != O))
      continue;

    if ((T->OType != OType) ||
        ((OID != NULL) && (T->OID != NULL) && 
         strcmp(T->OID,OID)))
      continue;
#endif /* !MYAHOO */

    /* located applicable trigger */
 
    if ((T->ETime > 0) && (ForceUpdate == FALSE))
      {
      /* event already reported */

      continue;
      }

    if ((T->EType == mttEnd) &&
        (bmisset(&T->SFlags,mtfResetOnModify)) &&
        (bmisset(&T->InternalFlags,mtifIsComplete)))
      {
      MTrigReset(T);
      }

    switch (T->Period)
      {
      case mpHour:
      case mpDay:
      case mpWeek:
      case mpMonth:

        {
        long PeriodStart;

        MUGetPeriodRange(
          (ETime > 0) ? ETime : MSched.Time,
          0,
          0,
          T->Period,
          &PeriodStart,
          NULL,
          NULL);

        T->ETime = PeriodStart + T->Offset;
        }  /* END BLOCK */

        break;

      default:

        T->ETime = (ETime > 0) ? ETime : MSched.Time;

        T->ETime += T->Offset;

        MDB(7,fSTRUCT) MLog("INFO:    trigger reported for '%s', type '%s'\n",
            T->OID,
            MTrigType[T->EType]);

        break;
      }
    }  /* END for (T) */

  return(SUCCESS);
  }  /* END MOReportEvent() */





/**
 * Adds trigger to object's TList (allocates TList if necessary)
 *
 * @param TListP (I) [modified/alloc]
 * @param ST     (I)
 */

int MOAddTrigPtr(

  marray_t **TListP,
  mtrig_t   *ST)

  {
  int tindex;

  mtrig_t *T;

  marray_t *TList;

  if ((TListP == NULL) || (ST == NULL))
    {
    return(FAILURE);
    }

  if (*TListP == NULL)
    {
    /* allocate object trigger list */

    if ((*TListP = (marray_t *)MUCalloc(1,sizeof(marray_t))) == NULL)
      {
      return(FAILURE);
      }

    MUArrayListCreate(*TListP,sizeof(mtrig_t *),10);
    }  /* END if (*TList == NULL) */

  TList = *TListP;

  if (MTrigInTList(TList,ST) == TRUE)
    {
    return(SUCCESS);
    }

  for (tindex = 0;tindex < TList->NumItems;tindex++)
    {
    T = (mtrig_t *)MUArrayListGetPtr(TList,tindex);

    if (MTrigIsValid(T) == FALSE)
      break;
    }

  if (tindex >= TList->NumItems)
    {
    MUArrayListAppendPtr(TList,ST);
    }
  else
    {
    /* assuming that we've found an empty slot that has been freed */

    memcpy(T,ST,sizeof(mtrig_t));
    }

  return(SUCCESS);
  }  /* END MOAddTrigPtr() */





/**
 *
 *
 * @param ST (I)
 */

int MODeleteTPtr(

  mtrig_t *ST)  /* I */

  {
  /* NOTE:  clear reference to trigger from T->O object, 
            adjust object trig pointer list, */

  mtrig_t *T;

  if ((ST == NULL) || (ST->O == NULL))
    {
    return(FAILURE);
    }

  switch (ST->OType)
    {
    case mxoRsv:

      {
      mrsv_t *R;

      int tindex;

      R = (mrsv_t *) ST->O;

      if (R->T == NULL)
        {
        return(SUCCESS);
        }

      for (tindex = 0;tindex < R->T->NumItems;tindex++)
        {
        T = (mtrig_t *)MUArrayListGetPtr(R->T,tindex);

        if (MTrigIsValid(T) == FALSE)
          continue;

        if (!strcmp(T->TName,ST->TName))
          break;
        } /* END for (tindex) */

      if ((tindex >= R->T->NumItems) || (T == NULL))
        {
        return(SUCCESS);
        }
      }   /* END BLOCK mxoRsv */

      break;

    default:

      /* NYI */

      break;
    } /* END switch (ST->OType) */

  return(SUCCESS);
  }  /* END MODeleteTPtr() */
/* END MTrigObject */
