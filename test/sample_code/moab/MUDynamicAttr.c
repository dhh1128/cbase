
/* HEADER */

/**
 * @file MUDynamicAttr.c
 *
 * Contains Dynamic Attribute functions
 */

#include "moab.h"
#include "moab-proto.h"
#include "moab-const.h"
#include "moab-global.h"

/**
 *  Searches for a node of the given attribute index in the list.
 *
 *  Returns success if the attribute was found and the format is the same
 *   as the requested format
 *
 *  NOTE: with strings, you will be returned an ALLOCATED COPY of the string
 *        in the list, so you will have to free it afterwards!  (This is so
 *        that the only way to modify the list is via the Set function)
 *
 *  What should be passed in for Data:
 *   char **
 *   int *
 *   double *
 *   long *
 *   mulong *
 *
 * @param AIndex [I] - The attribute to look for
 * @param LHead  [I] - The start of the attribute list
 * @param Data   [O] - Output pointer (optional)
 * @param Format [I] - Format of the pointer that was passed in
 */

int MUDynamicAttrGet(
  int AIndex,
  mdynamic_attr_t *LHead,
  void **Data,
  enum MDataFormatEnum Format)

  {
  mdynamic_attr_t *ptr = NULL;

  if ((Data != NULL) && (Format == mdfString))
    *Data = NULL;

  if (LHead == NULL)
    return(FAILURE);

  for (ptr = LHead;ptr != NULL;ptr = ptr->Next)
    {
    if (ptr->AttrType == AIndex)
      {
      /* Found it */

      if (ptr->Format != Format)
        return(FAILURE);

      if (Data == NULL)
        {
        /* Just checking if a value exists */

        return(SUCCESS);
        }

      switch (ptr->Format)
        {
        case mdfString:

          MUStrDup((char **)Data,ptr->Data.String);
          return(SUCCESS);

          break;

        case mdfInt:

          *(int *)Data = ptr->Data.Int;
          return(SUCCESS);

          break;

        case mdfDouble:

          *(double *)Data = ptr->Data.Double;
          return(SUCCESS);

          break;

        case mdfLong:

          *(long *)Data = ptr->Data.Long;
          return(SUCCESS);

          break;

        case mdfMulong:

          *(mulong *)Data = ptr->Data.Mulong;
          return(SUCCESS);

          break;

        case mdfOther:

          *Data = ptr->Data.Void;
          return(SUCCESS);

          break;

        default:

          /* This should never be reached */

          return(FAILURE);

          break;
        }  /* END switch (ptr->Format) */
      }  /* END if (ptr->AttrType == AIndex) */

    if (ptr->AttrType > AIndex)
      {
      /* These lists are stored in order, so we've passed where it would be */

      return(FAILURE);
      }
    }  /* END for (ptr) */

  /* Didn't find the attribute */

  return(FAILURE);
  }  /* END MUDynamicAttrGet() */



/**
 *  Adds the dynamic attr to the given list.  If it already exists, the value
 *   will be overwritten.
 *
 *  The list is kept in order to speed up searching.
 *
 *  What should be passed in for Data (cast as a void **):
 *    char *
 *    int *
 *    double *
 *    long *
 *    mulong *
 *
 * @param AIndex [I] - Which attribute to modify
 * @param LHead  [I] (modified) - The head of the dynamic attr list
 * @param Data   [I] - The value to set the attribute to
 * @param Format [I] - Format of Data
 */

int MUDynamicAttrSet(
  int                  AIndex,
  mdynamic_attr_t    **LHead,
  void               **Data,
  enum MDataFormatEnum Format)

  {
  mdynamic_attr_t *ptr;
  mdynamic_attr_t *prev = NULL;

  const char *FName = "MUDynamicAttrSet";

  MDB(5,fSTRUCT) MLog("%s(%d,%s,%s,%s)\n",
    FName,
    AIndex,
    (LHead != NULL) ? "LHead" : "NULL",
    (Data != NULL) ? "Data" : "NULL",
    (char *)MDFormat[Format]);

  if ((LHead == NULL) ||
      ((Data == NULL) &&
       (Format != mdfString)))
    return(FAILURE);

  if ((Format != mdfString) &&
      (Format != mdfInt) &&
      (Format != mdfDouble) &&
      (Format != mdfLong) &&
      (Format != mdfOther) &&
      (Format != mdfMulong))
    {
    /* Invalid format type */

    return(FAILURE);
    }

  /* NULL is a valid input for Data (unset a string) */

  ptr = *LHead;

  while (ptr != NULL)
    {
    if (ptr->AttrType == AIndex)
      {
      /* Set on this one */

      if (ptr->Format == mdfString)
        MUFree((char **)&ptr->Data.String);

      switch (Format)
        {
        case mdfString:

          MUStrDup(&ptr->Data.String,(char *)Data);

          break;

        case mdfInt:

          ptr->Data.Int = *(int *)Data;

          break;

        case mdfDouble:

          ptr->Data.Double = *(double *)Data;

          break;

        case mdfLong:

          ptr->Data.Long = *(long *)Data;

          break;

        case mdfMulong:

          ptr->Data.Mulong = *(mulong *)Data;

          break;

        case mdfOther:

          ptr->Data.Void = *Data;

          break;

        default:

          /* NOTREACHED */

          return(FAILURE);

          break;
        }  /* END switch (Format) */

      ptr->Format = Format;

      return(SUCCESS);
      }  /* END if (ptr->AttrType == AIndex) */

    if (ptr->AttrType > AIndex)
      {
      /* Need to insert before this one */

      break;
      }

    prev = ptr;
    ptr = ptr->Next;
    }  /* END while (ptr != NULL) */

  /* Need to create a new node */

  if ((ptr = (mdynamic_attr_t *)MUCalloc(1,sizeof(mdynamic_attr_t))) == NULL)
    {
    return(FAILURE);
    }

  if (prev != NULL)
    {
    ptr->Next = prev->Next;
    prev->Next = ptr;
    }
  else
    {
    *LHead = ptr;
    }

  switch (Format)
    {
    case mdfString:

      MUStrDup(&ptr->Data.String,(char *)Data);

      break;

    case mdfInt:

      ptr->Data.Int = *(int *)Data;

      break;

    case mdfDouble:

      ptr->Data.Double = *(double *)Data;

      break;

    case mdfLong:

      ptr->Data.Long = *(long *)Data;

      break;

    case mdfMulong:

      ptr->Data.Mulong = *(mulong *)Data;

      break;

    case mdfOther:

      ptr->Data.Void = *Data;

      break;

    default:

      /* NOTREACHED */

      return(FAILURE);

      break;
    }  /* END switch (Format) */

  ptr->AttrType = AIndex;
  ptr->Format = Format;

  return(SUCCESS);
  }  /* END MUDynamicAttrSet() */


/**
 * Frees the given dynamic attr list.
 *
 * @param DAList [I] (freed) - the list to free
 */

int MUDynamicAttrFree(

  mdynamic_attr_t **DAList)

  {
  mdynamic_attr_t *tmpDA = NULL;
  mdynamic_attr_t *NextDA = NULL;

  if ((DAList == NULL) ||
      (*DAList == NULL))
    return(SUCCESS);

  tmpDA = *DAList;

  while (tmpDA != NULL)
    {
    NextDA = tmpDA->Next;

    if (tmpDA->Format == mdfString)
      MUFree(&tmpDA->Data.String);

    MUFree((char **)&tmpDA);

    tmpDA = NextDA;
    }

  *DAList = NULL;

  return(SUCCESS);
  }  /* END MUDynamicAttrFree() */


/**
 * Copies a list of mdynamic_attr_t.
 *
 * @param Src [I] - the list to copy
 * @param Dst [I] (modified) - list to copy to
 */

int MUDynamicAttrCopy(
  mdynamic_attr_t *Src,
  mdynamic_attr_t **Dst)

  {
  mdynamic_attr_t *tmpDA = NULL;

  if ((Dst == NULL) || (Src == NULL))
    return(FAILURE);

  MUDynamicAttrFree(Dst);

  tmpDA = Src;

  while (tmpDA != NULL)
    {
    switch(tmpDA->Format)
      {
      case mdfString:
        {
        char *tmpStr = NULL;
        MUDynamicAttrGet(tmpDA->AttrType,tmpDA,(void **)&tmpStr,tmpDA->Format);
        MUDynamicAttrSet(tmpDA->AttrType,Dst,(void **)tmpStr,tmpDA->Format);

        MUFree(&tmpStr);
        }

        break;

      case mdfInt:
        {
        int tmpI;
        MUDynamicAttrGet(tmpDA->AttrType,tmpDA,(void **)&tmpI,tmpDA->Format);
        MUDynamicAttrSet(tmpDA->AttrType,Dst,(void **)&tmpI,tmpDA->Format);
        }

        break;

      case mdfDouble:
        {
        double tmpD;
        MUDynamicAttrGet(tmpDA->AttrType,tmpDA,(void **)&tmpD,tmpDA->Format);
        MUDynamicAttrSet(tmpDA->AttrType,Dst,(void **)&tmpD,tmpDA->Format);
        }

        break;

      case mdfLong:
        {
        long tmpL;
        MUDynamicAttrGet(tmpDA->AttrType,tmpDA,(void **)&tmpL,tmpDA->Format);
        MUDynamicAttrSet(tmpDA->AttrType,Dst,(void **)&tmpL,tmpDA->Format);
        }

        break;

      case mdfMulong:
        {
        mulong tmpML;
        MUDynamicAttrGet(tmpDA->AttrType,tmpDA,(void **)&tmpML,tmpDA->Format);
        MUDynamicAttrSet(tmpDA->AttrType,Dst,(void **)&tmpML,tmpDA->Format);
        }

      case mdfOther:
        {
        void *tmpV = NULL;
        MUDynamicAttrGet(tmpDA->AttrType,tmpDA,(void **)&tmpV,tmpDA->Format);
        MUDynamicAttrSet(tmpDA->AttrType,Dst,(void **)&tmpV,tmpDA->Format);
        }

        break;

      default:

        /* We should never hit this! */

        break;
      }  /* END switch(tmpDA->AttrType) */

    tmpDA = tmpDA->Next;
    }  /* END while (tmpDA != NULL) */

  return(SUCCESS);
  }  /* END MUDynamicAttrCopy() */
/* END MUDynamicAttr.c */
