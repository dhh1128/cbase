/* HEADER */


/**
 * @file MUArray.c
 *
 * Contains functions for the Moab Array data collection
 *
 */

#include "moab.h"
#include "moab-proto.h"
#include "MUArray.h"
#include "MMemory.h"


/* Local implementation defines - to limit scope to this file only */

#define MEMPTYSLOT (void *)0x1


/**
 * Grow the give array.
 *
 * @param ArrayP (I)
 */

int __MUArrayListGrow(

  marray_t *ArrayP)

  {
  char  *Dst = NULL;
  int    OldArraySize;

  if (ArrayP == NULL)
    return(FAILURE);
  
  /* need to grow array */
  
  OldArraySize = ArrayP->ArraySize;

  if (OldArraySize >= 8)
    {
    ArrayP->ArraySize = (int)(ArrayP->ArraySize * 1.5);  /* this could be tweaked to help performance/heap fragmentation */
    }
  else
    {
    ArrayP->ArraySize = 10;  /* set array size to 10 to prevent having to re-allocate after every insertion */
    }

  ArrayP->Array = (unsigned char *)realloc(ArrayP->Array,ArrayP->ElementSize * ArrayP->ArraySize);

#if 0
  if (ArrayP->Array == NULL)
    {
    MDB(1,fSTRUCT) MLog("ERROR:    error growing marray!\n");

    return(FAILURE);
    }
#endif

  /* intialize new part of array */

  Dst = (char *)ArrayP->Array + (OldArraySize * ArrayP->ElementSize);
  
  memset(Dst,0x0,(ArrayP->ElementSize * (ArrayP->ArraySize - OldArraySize))); 

  return(SUCCESS);
  } /* END int __MUArrayListGrow() */



/**
 * Sets the arrays Empty Slots to reflect that the given index is empty.
 *
 * @param ArrayP (I)
 * @param Index  (I)
 */

int __MUArrayListSetEmpty(

  marray_t *ArrayP, /*I*/
  int       Index)  /*I*/

  {
  int eindex;
  int *EmptySlots;
  int TotEmpty;
  int oldIndex;

  if (ArrayP == NULL)
    {
    return(FAILURE);
    }

  /* always sort going in */

  EmptySlots = ArrayP->EmptyTable;

  TotEmpty = ArrayP->NumEmptyItems;

  for (eindex = 0;eindex <= TotEmpty;eindex++) /* <= because want to check even if TotEmpty == 0 */
    {
    if (eindex == MCONST_MARRAYEMPTYSIZE) /* 1 past array */
      break;

    if (Index < EmptySlots[eindex]) /* insert Index sorted low to high */
      {
      /* slide original values to down - to the right */

      oldIndex = EmptySlots[eindex];

      EmptySlots[eindex] = Index;

      Index = oldIndex;
      }
    else if ((EmptySlots[eindex] == -1) ||      /* end of empty items found, just add */
             (eindex == ArrayP->NumEmptyItems)) /* end of empty items found, just add */
      {
      EmptySlots[eindex] = Index;
      }
    } /* for (eindex = 0;eindex <= TotEmpty;eindex++) */

  ArrayP->NumEmptyItems++;

  return(SUCCESS);
  } /* END int __MUArrayListSetEmpty() */


#if 0

/* called by routines that are currently disabled due to possible buggy conditions
 */


/**
 * Removes the index from the EmptySlots table if it exists inside.
 *
 * @param ArrayP (I)
 * @param Index (I)
 */

int __MUArrayListSetFilled(

  marray_t *ArrayP, /*I*/
  int       Index)  /*I*/

  {
  int eindex;
  
  if (ArrayP == NULL)
    return(FAILURE);

  for (eindex = 0;eindex < ArrayP->NumEmptyItems;eindex++)
    {
    if (ArrayP->EmptyTable[eindex] == Index) /* if given index is in empty table */
      {
      /* slide trailing indexes down to fill in removed index */

      for (;eindex < ArrayP->NumEmptyItems;eindex++)
        {
        if ((eindex == MCONST_MARRAYEMPTYSIZE - 1) || /* at end of table */
            (ArrayP->EmptyTable[eindex + 1] == -1))   /* at last real index */
          {
          ArrayP->EmptyTable[eindex] = -1; 

          break;
          }

        /* move index to the right to the current index */

        ArrayP->EmptyTable[eindex] = ArrayP->EmptyTable[eindex + 1];
        }
      
      break;
      }
    } /* END for (eindex = 0;eindex < ArrayP->NumItems;eindex++) */
      
  /* NumEmptyItems is 0 at creation, don't want to go negavtive. */

  if (ArrayP->NumEmptyItems > 0)
    ArrayP->NumEmptyItems--;

  return(SUCCESS);
  } /* int __MUArrayListSetFilled() */
#endif   /* #if 0 buggy code */


#if 0
/**
 * Gets the index of an empty slot in the array.
 *
 * @param ArrayP (I)
 * @return Returns -1, if none are found.
 */

int __MUArrayListGetEmptyIndex(

  marray_t *ArrayP) /*I*/

  {
  int eindex;
  void *item;

  if (ArrayP == NULL)
    return(-1);

  if (ArrayP->NumEmptyItems == 0) /* there are no empty slots in the array - append */
    return(ArrayP->NumItems);

  if ((ArrayP->NumEmptyItems != 0) &&
      (ArrayP->EmptyTable[0] != -1))
    {
    /* There exists an index in the empty table that point to an open spot. */

    return(ArrayP->EmptyTable[0]);
    }

  /* EmptySlots are sorted with closest index at 0. */

  eindex = 0;

  /* search whole array for empty slot.*/

  while((item = MUArrayListGet(ArrayP,eindex)) != NULL)
    {
    if (item == MEMPTYSLOT)
      break;

    eindex++;
    }

  return(eindex);
  } /* END int __MUArrayListGetEmptyIndex() */
#endif 


/**
 * Clears memory of the given marray pointer, allocs the initial
 * array to InitialSize or, if InitialSize < 0, it uses the default
 * capacity size of 25.
 *
 * WARNING: This will NOT free any existing elements in an already
 * populated array!
 *
 * NOTE: marrays are NOT made to have items removed!!! They can only be added!!!
 *
 * @param ArrayP (I) This should be allocated already, can go on the stack (~10bytes).
 * @param ElementSize (I) Should be a "sizeof()" for the data type you wish to store in the array. (Ex. sizeof(mjob_t) or sizeof(char *))
 * @param InitialSize (I) Also could be called "InitialCapacity" [optional] 
 *
 */

int MUArrayListCreate(

  marray_t *ArrayP,
  int ElementSize,
  int InitialSize)

  {
  if (ArrayP == NULL)
    {
    return(FAILURE);
    }

  memset(ArrayP,0x0,sizeof(marray_t));

  ArrayP->NumItems = 0;

  if (InitialSize > 0)
    ArrayP->ArraySize = InitialSize;
  else
    ArrayP->ArraySize = 25;  /* default */

  ArrayP->ElementSize = ElementSize;

  ArrayP->NumEmptyItems = 0;

  ArrayP->Array = (unsigned char *)MUCalloc(1,ArrayP->ElementSize * ArrayP->ArraySize);

  if (ArrayP->Array == NULL)
    {
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MUArrayListCreate() */





/**
 * returns the number of bytes in array, logically equivalent to a sizeof operation
 * of Array's internal buffer.
 *
 * @param Array (I)
 */

int MUArrayListByteSize(

  marray_t *Array)

  {
  return(Array->ArraySize * Array->ElementSize);
  } /* END MUArrayListByteSize() */





/**
 * Resize Array's capacity to NewSize
 *
 * @param Array (I/O)
 * @param NewSize (I)
 */

int MUArrayListResize(

  marray_t *Array,
  int       NewSize)

  {
  Array->Array = (unsigned char *)realloc(Array->Array,Array->ElementSize * NewSize);

  if (Array->Array == NULL)
    {
    Array->ArraySize = 0;
    Array->NumItems = 0;

    return(FAILURE);
    }

  Array->ArraySize = NewSize;

  return(SUCCESS);
  } /* END MUArrayListResize() */





/**
 * Allocate space for an uninitialized element and return a pointer to it
 * 
 * @param List (I)
 */

void *MUArrayListAppendEmpty(

  marray_t *List) /* I */

  {
  void *Result;

  if (List->NumItems == List->ArraySize)
    __MUArrayListGrow(List);

  Result = (void *)(List->Array + (List->NumItems * List->ElementSize));
  List->NumItems++;

  return(Result);
  }  /* END MUArrayListAppendEmpty() */





/**
 * Adds the given Object to an marray. (Copies the mem pointed to by the
 * Object pointer into the array.) Note that the Object should
 * match the data type that was originally used to create the marray!!!
 *
 * The array will automatically grow and reallocate memory dynamically as needed.
 *
 * WARNING: If you wish to add a pointer to the array use MUArrayListAppendPtr() instead!
 *
 * @see MUArrayListCreate()
 * @see MUArrayListFree()
 *
 * @param ArrayP (I)
 * @param Object (I) Pointer to object that should be appended to array. Use MUArrayListAppendPtr() to append pointers!!!
 */

int MUArrayListAppend(

  marray_t *ArrayP,
  void *Object)

  {
  char *Dst = NULL;

  /* for now, allow users to append NULL objects */

  if (ArrayP == NULL)
    {
    return(FAILURE);
    }

  if (ArrayP->Array == NULL)
    {
    return(FAILURE);
    }

  if (ArrayP->NumItems == ArrayP->ArraySize)
    {
    if (__MUArrayListGrow(ArrayP) == FAILURE)
      {
      MDB(1,fSTRUCT) MLog("ERROR:    error growing marray!\n");
      
      return(FAILURE);
      }
    }

  Dst = (char *)ArrayP->Array + (ArrayP->NumItems++ * ArrayP->ElementSize);

  memcpy(Dst,Object,ArrayP->ElementSize);

  return(SUCCESS);
  }  /* END MUArrayListAppend() */





/**
 * Adds the given pointer to an marray. (Copies the pointer value itself
 * into the array.) Note that the Ptr should
 * match the pointer data type that was originally used to create the marray!!!
 *
 * The array will automatically grow and reallocate memory dynamically as needed.
 *
 * WARNING: If you wish to add non-pointer to an array use MUArrayListAppend() instead!
 *
 * NOTE: Use MUArrayListGetPtr()
 *
 * @see MUArrayListCreate()
 * @see MUArrayListFree()
 *
 * @param ArrayP (I)
 * @param Ptr (I) Pointer that should be appended to array. Use MUArrayListAppend() to append non-pointer data!!!
 */

int MUArrayListAppendPtr(

  marray_t *ArrayP,
  void *Ptr)

  {
  void **tmpPtr;

  if (Ptr == NULL)
    return(FAILURE);

  tmpPtr = &Ptr;

  return(MUArrayListAppend(ArrayP,(void *)tmpPtr));
  }  /* END MUArrayListAppendPtr() */



#if 0

/* no one uses this code and during unittest development, found some
 * bugs concerning increasing/decreasing empty counts.
 *
 * Suggest some code review before re-enabling this code
 */


/**
 * Inserts the given pointer into an marray. (Copies the pointer value itself
 * into the array.) If the array has an empty slot, it will be inserted
 * in that slot. Otherwise, it will be appended to the end of the array.
 * Note that the Ptr should match the pointer data type that was originally used to create the marray!!!
 *
 * The array will automatically grow and reallocate memory dynamically as needed.
 *
 * WARNING: If you wish insert a non-pointer in an array use MUArrayListInsert() instead!
 *
 * @param ArrayP (I)
 * @param Ptr (I)
 */

int MUArrayListInsertPtr(

  marray_t *ArrayP,
  void     *Ptr)

  {
  int    index;
  char  *Dst = NULL;
  void **tmpPtr;

  if (ArrayP == NULL)
    return(FAILURE);
  
  tmpPtr = (void **)&Ptr;

  index = __MUArrayListGetEmptyIndex(ArrayP);

  /* If index is the end of the array, then array should be 
   * grown and index will be at an empty spot. */

  /* check if we should grow array */

  if (ArrayP->NumItems == ArrayP->ArraySize)
    {
    if (__MUArrayListGrow(ArrayP) == FAILURE)
      return(FAILURE);
    }

  if (index > ArrayP->ArraySize)
    {
    MDB(1,fSTRUCT) MLog("ERROR:    trying to insert element at index %d which is past the array size of %d\n",
        index,
        ArrayP->ArraySize);

    return(FAILURE);
    }

  Dst = (char *)ArrayP->Array + (index * ArrayP->ElementSize);

  memcpy(Dst,tmpPtr,ArrayP->ElementSize);

  __MUArrayListSetFilled(ArrayP,index);

  ArrayP->NumItems++;

  return(SUCCESS);
  } /* END int MUArrayListInsertPtr() */
#endif  /* #if 0 buggy code */


#if 0

/* No one uses this and in the unittest creation found it to be buggy */

/*
 * Inserts the given pointer into an marray. (Copies the pointer value itself
 * into the array.)  It will be inserted into the specified index.  Note that
 * any existing value at that index will be overwritten.
 *
 * @param ArrayP (I) The array list
 * @param Ptr    (I) The pointer to copy
 * @param index  (I) The index to insert
 */

int MUArrayListInsertPtrAtIndex(

  marray_t *ArrayP,  /* I */
  void     *Ptr,     /* I */
  int       index)   /* I */

  {
  char  *Dst = NULL;
  void **tmpPtr;
  mbool_t IncrNumCount = FALSE;

  if (ArrayP == NULL)
    return(FAILURE);

  tmpPtr = (void **)&Ptr;

  if (index < 0)
    {
    return(FAILURE);
    }

  /* NOTE: if you give a high number, it will fill until that number */
  while (index > ArrayP->ArraySize)
    {
    /* Need to grow the array to get to the index */

    if (__MUArrayListGrow(ArrayP) == FAILURE)
      return(FAILURE);
    }

  if (MUArrayListGet(ArrayP,index) == NULL)
    {
    IncrNumCount = TRUE;
    }

  Dst = (char *)ArrayP->Array + (index * ArrayP->ElementSize);

  memcpy(Dst,tmpPtr,ArrayP->ElementSize);

  if (IncrNumCount == TRUE)
    {
    __MUArrayListSetFilled(ArrayP,index);

    ArrayP->NumItems++;
    }

  return(SUCCESS);
  } /* END int MUArrayListInsertPtrAtIndex() */
#endif /* END #if 0 buggy code */


#if 0

/* no one uses it and found it to be buggy during unittest generation */

/*  
 * Copies the given object into an marray_t.  
 * It will be inserted into the specified index.  Note that                          
 * any existing value at that index will be overwritten.                                               
 *
 * @param ArrayP (I) The array list
 * @param Ptr    (I) The object to copy
 * @param index  (I) The index to insert
 */

int MUArrayListInsertAtIndex(                                                                       
  
  marray_t *ArrayP,  /* I */                                                                           
  void     *Ptr,     /* I */                                                                           
  int       index)   /* I */                                                                           
  
  {
  char  *Dst = NULL;                                                                                   
  mbool_t IncrNumCount = FALSE;                                                                        
  
  if (ArrayP == NULL)                                                                                  
    return(FAILURE);                                                                                   
  
  if (index < 0)                                                                                       
    {
    return(FAILURE);                                                                                   
    }                                                                                                  
  
  /* NOTE: if you give a high number, it will fill until that number */                                
  while (index > ArrayP->ArraySize)                                                                    
    {
    /* Need to grow the array to get to the index */                                                   
    
    if (__MUArrayListGrow(ArrayP) == FAILURE)                                                          
      return(FAILURE);                                                                                 
    }                                                                                                  
  
  if (MUArrayListGet(ArrayP,index) == NULL)                                                            
    {
    IncrNumCount = TRUE;                                                                               
    }                                                                                                  
  
  Dst = (char *)ArrayP->Array + (index * ArrayP->ElementSize);                                         
  
  memcpy(Dst,Ptr,ArrayP->ElementSize);                                                              
  
  if (IncrNumCount == TRUE)                                                                            
    {
    __MUArrayListSetFilled(ArrayP,index);                                                              
    
    ArrayP->NumItems++;                                                                                
    }                                                                                                  
  
  return(SUCCESS);
  } /* END int MUArrayListInsertAtIndex() */
#endif  /* END #if 0  buggy code */


/** 
 * Returns A POINTER to the item at the given index. This means
 * if you are storing pointers in the array, a ** will be returned.
 * If you are storing int's in the array, then a int * will be returned!
 *
 * Note: You need to manually cast what is returned.
 *
 * @param ArrayP (I) The arraylist pointer.
 * @param Index (I) The index of the element to return.
 * @return A POINTER to the item at the given index, NULL otherwise.
 */

void *MUArrayListGet(

  marray_t *ArrayP,
  int Index)

  {
  if (ArrayP == NULL)
    {
    return(FAILURE);
    }

  if ((Index < 0) || (Index >= ArrayP->ArraySize))
    {
    return(NULL);
    }

  return(ArrayP->Array + (Index * ArrayP->ElementSize));
  }  /* END MUArrayListGet() */


/**
 * Frees all the resources for a given array and sets its values to 0. This will NOT
 * free any of the elements in the array--only the array memory itself!
 *
 * @see MUArrayListFreePtrElements()
 *
 * @param ArrayP (I)
 */

int MUArrayListFree(

  marray_t *ArrayP)

  {
  if (ArrayP == NULL)
    {
    return(FAILURE);
    }

  MUFree((char **)&ArrayP->Array);
  ArrayP->NumItems = 0;
  ArrayP->ElementSize = 0;
  ArrayP->ArraySize = 0;

  return(SUCCESS);
  }  /* END MUArrayListFree() */



/**
 * Assumes that all elements in the array are alloc'd char *. This routine will
 * free all elements in the array using MUFree(). This is a convienence function.
 * This function will NOT free the array itself, only its elements.
 *
 * @see MUArrayListFree()
 *
 * @param ArrayP (I)
 */

int MUArrayListFreePtrElements(

  marray_t *ArrayP)

  {
  int i;
  char **A;

  if (ArrayP == NULL)
    {
    return(FAILURE);
    }

  A = (char **)ArrayP->Array;

  for (i = 0;i < ArrayP->NumItems;i++)
    {
    if (A[i] != (char *)MEMPTYSLOT)  /* only free non-empty elements */
      MUFree(&A[i]);
    }
  
  return(SUCCESS);
  }  /* END MUArrayListFreePtrElements() */



/**
 * Similar to MUArrayListGet() except that this function will
 * return the actual pointer being stored in the array (and not a 
 * double-pointer).
 *
 * This simply makes it easier to handle pointers in an array.
 *
 * WARNING: Only use on an Moab array list that stores pointers!
 *
 * NOTE: should be used with MUArrayListAppendPtr()
 *
 * NOTE: this worked for me in gdb (when storing "mtrig_t *"):
 * (gdb) p *((mtrig_t **)((void **)MTList.Array))[0]
 *
 * @param ArrayP The arraylist pointer.
 * @param Index The index of the element to return.
 * @return The actual pointer that is stored in the array (not a double-pointer).
 */

void *MUArrayListGetPtr(

  marray_t *ArrayP,
  int       Index)

  {
  void **ResultPtr = NULL;

  if ((NULL == ArrayP) || (Index < 0) || (Index >= ArrayP->NumItems))
    {
    return(NULL);
    }

  ResultPtr = (void **)(ArrayP->Array + (Index * ArrayP->ElementSize));

  return(*ResultPtr);
  }  /* END MUArrayListGetPtr() */





/*
 * Removes, and frees, the item at the given index.
 * Also marks the index as empty and available for further
 * insertions.
 *
 * @param ArrayP (I)The arraylist pointer.
 * @param Index (I) The index of the element to remove.
 * @param FreeItem (I) Whether or not to free allocation memory.
 */

int MUArrayListRemove(

  marray_t *ArrayP,    /*I*/
  int       Index,     /*I*/
  mbool_t   FreeItem)  /*I*/

  {
  void **tmpArray;

  if (ArrayP == NULL)
    {
    return(FAILURE);
    }

  if ((Index < 0) || (Index >= ArrayP->ArraySize -1)) /* ArrayP->ArraySize -1 is NULL */
    {
    return(FAILURE);
    }

  tmpArray = (void **)ArrayP->Array;

  if (FreeItem == TRUE)
    {
    MUFree((char **)&tmpArray[Index]);
    }

  tmpArray[Index] = MEMPTYSLOT; /* mark empty */

  __MUArrayListSetEmpty(ArrayP,Index);

  ArrayP->NumItems--;

  return(SUCCESS);
  } /* END int MUArrayListRemove() */


/**
 * Clears the list be freeing and realloc'ing.
 *
 * @param A
 */

int MUArrayListClear(

  marray_t *A)

  {
  int Size;
  int ElementSize;

  if (A == NULL)
    {
    return(FAILURE);
    }

  Size = A->NumItems;
  ElementSize = A->ElementSize;

  MUArrayListFree(A);

  MUArrayListCreate(A,ElementSize,Size);

  return(SUCCESS);
  }  /* END MUArrayListClear() */



/**
 * Will copy the contents of SrcList into DstList. The resulting DstList array
 * will contain the same type and number of items as the SrcList. This is a shallow
 * copy!
 *
 * WARNING: This function will free DstList before making the copy!
 *
 * @param DstList (I) - A pointer to the array list that will be copied to.
 * @param SrcList (I) - A pointer to the array list that will be copied from.
 */

int MUArrayListCopy(

  marray_t *DstList,
  marray_t *SrcList)

  {
  if ((DstList == NULL) ||
      (SrcList == NULL))
    {
    return(FAILURE);
    }

  if (DstList->Array != NULL)
    {
    MUArrayListFree(DstList);
    }

  if (SrcList->Array == NULL)
    {
    /* nothing to copy */

    return(SUCCESS);
    }

  MUArrayListCreate(DstList,SrcList->ElementSize,SrcList->NumItems);

  memcpy(DstList->Array,SrcList->Array,SrcList->ElementSize * SrcList->NumItems);

  DstList->ElementSize = SrcList->ElementSize;
  DstList->NumItems = SrcList->NumItems;

  return(SUCCESS);
  }  /* END MUArrayListCopy() */




/**
 * This function will do a deep copy of the given array list if it
 * contains pointers! Use MUArrayListCopy() for a shallow copy of 
 * non-pointer content.
 *
 * @see MUArrayListCopy()
 *
 * WARNING: This function will free DstList before making the copy!
 *
 * @param DstList    (I) [modified]
 * @param SrcList    (I)
 * @param AreStrings (I)
 * @param ObjectSize (I) The size of the struct/object that the pointers
 *                       POINT to.
 */

int MUArrayListCopyPtrElements(

  marray_t *DstList,
  marray_t *SrcList,
  mbool_t   AreStrings,
  int       ObjectSize)

  {
  void **SrcA;
  void **DstA;

  char **SrcStrA;
  char **DstStrA;

  void  *tmpPtr = NULL;
  int i;

  if ((DstList == NULL) ||
      (SrcList == NULL))
    {
    return(FAILURE);
    }

  if (DstList->Array != NULL)
    {
    MUArrayListFree(DstList);
    }

  if (SrcList->Array == NULL)
    {
    /* nothing to copy */

    return(SUCCESS);
    }

  MUArrayListCreate(DstList,SrcList->ElementSize,SrcList->NumItems);

  DstList->ElementSize = SrcList->ElementSize;
  DstList->NumItems = SrcList->NumItems;

  if (AreStrings == FALSE)
    {
    SrcA = (void **)SrcList->Array;
    DstA = (void **)DstList->Array;

    for (i = 0;i < SrcList->NumItems;i++)
      {
      tmpPtr = MUMalloc(ObjectSize);

      memcpy(tmpPtr,SrcA[i],ObjectSize);

      DstA[i] = tmpPtr;
      }
    }
  else
    {
    SrcStrA = (char **)SrcList->Array;
    DstStrA = (char **)DstList->Array;

    for (i = 0;i < SrcList->NumItems;i++)
      {
      MUStrDup(&DstStrA[i],SrcStrA[i]);
      }
    }

  return(SUCCESS);
  }  /* END MUArrayListCopyPtrElements() */




/**
 * This function will do a deep-copy of the given array list, assuming
 * that the array lists contain "char *"!
 *
 * @see MUArrayListCopyPtrElements()
 *
 * WARNING: This function will free DstList before making the copy!
 *
 * @param DstList (I) [modified]
 * @param SrcList (I)
 */

int MUArrayListCopyStrElements(

  marray_t *DstList,
  marray_t *SrcList)

  {
  return (MUArrayListCopyPtrElements(DstList,SrcList,TRUE,0));
  }  /* END MUArrayListCopyStrElements() */

/**
 * Concatenates an array of strings into one single string, using a given delimeter to seperate
 * each individual element on the original array.
 *
 * @param AList (I) The array containing the strings to concatenate.
 * @param Delim (I,optional) The delimeter to use in separating the array's elements.
 * @param Buffer (O,minsize=MMAX_LINE) The target string buffer, if NULL, then NULL is returned.
 * @param BufSize (I) The size of the Buffer
 * @return The concatenated string.
 */

char *MUArrayToString(

  const char **AList,    /* I */
  const char  *Delim,    /* I (optional) */
  char        *Buffer,   /* O (minsize=MMAX_LINE) */
  int          BufSize)  /* I */

  {
  int         index;

  char *head;

  char *BPtr;
  int   BSpace;

  if ((Buffer == NULL) || (BufSize < 1))
    {
    return(NULL);
    }

  MUSNInit(&BPtr,&BSpace,Buffer,BufSize);
 
  if (Delim == NULL)
    Delim = ",";

  head = BPtr;

  for (index = 0;AList[index] != NULL;index++)
    {
    MUSNPrintF(&BPtr,&BSpace,"%s%s",     
      (index > 0) ? Delim : "",
      AList[index]);
    }  /* END for (index) */

  return(head);
  }  /* END MUArrayToString() */



/**
 * Concatenates an array of strings into one single string, using a given delimeter to seperate
 * each individual element on the original array.
 *
 * @param AList (I) The array containing the strings to concatenate.
 * @param Delim (I,optional) The delimeter to use in separating the array's elements.
 * @param Buffer (O,minsize=MMAX_LINE) The target string buffer, if NULL, then NULL is returned.
 * @return The concatenated string (Buffer->c_str())
 */

char *MUArrayToMString(

  const char **AList,    /* I */
  const char  *Delim,    /* I (optional) */
  mstring_t   *Buffer)   /* O (minsize=MMAX_LINE) */

  {
  int         index;

  if (Buffer == NULL)
    {
    return(NULL);
    }

  if (Delim == NULL)
    Delim = ",";

  for (index = 0;AList[index] != NULL;index++)
    {
    MStringAppendF(Buffer,"%s%s",     
      (index > 0) ? Delim : "",
      AList[index]);
    }  /* END for (index) */

  return(Buffer->c_str());
  }  /* END MUArrayToMString() */
/* END MUArray.c */
