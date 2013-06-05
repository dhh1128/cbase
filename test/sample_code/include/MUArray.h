/* HEADER */


/**
 * @file MUArray.h
 *
 * Moab Array object interfact
 *
 **/


#ifndef __MUARRAY_H__
#define __MUARRAY_H__


/* The following control structure for dynamic arrays
 * provides the state information for a given array of
 * information
 */


#define MCONST_MARRAYEMPTYSIZE 5

typedef struct marray_t {

  int ArraySize;   /* the size of the dynamically allocated array */
  int NumItems;    /* the number of items in this array */
  int ElementSize; /* the size of each element contained in the array */

  /* small cache which lists which slots in the array are "empty" */
  int EmptyTable[MCONST_MARRAYEMPTYSIZE];  
  int NumEmptyItems;  /* number of empty indexes inide the EmptyTable cache */

  unsigned char *Array; /* g++ complains about void ptr arithmetic */
  } marray_t;


int   MUArrayListCreate(marray_t *,int,int);
int   MUArrayListByteSize(marray_t *);
int   MUArrayListResize(marray_t *,int);
void *MUArrayListAppendEmpty(marray_t *);
int   MUArrayListAppend(marray_t *,void *);
int   MUArrayListAppendPtr(marray_t *,void *);
int   MUArrayListInsertPtrAtIndex(marray_t *,void *,int);
int   MUArrayListInsertAtIndex(marray_t *,void *,int);
void *MUArrayListGet(marray_t *,int);
int   MUArrayListFree(marray_t *);
int   MUArrayListFreePtrElements(marray_t *ArrayP);
void *MUArrayListGetPtr(marray_t *,int);
int   MUArrayListRemove(marray_t *,int,mbool_t);
int   MUArrayListClear(marray_t *);
int   MUArrayListCopy(marray_t *,marray_t *);
int   MUArrayListCopyPtrElements(marray_t *,marray_t *,mbool_t,int);
int   MUArrayListCopyStrElements(marray_t *,marray_t *);

#endif /* __MUARRAY_H__ */

