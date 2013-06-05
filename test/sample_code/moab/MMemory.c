/* HEADER */

/**
 * @file MMemory.c
 *
 * Contains Moab Memory APIs
 */

#include "moab.h"
#include "moab-proto.h"



/* Pointer to the memory tracking table
 * If != NULL, then tracking is enabled, else not enabled
 */
memtracker_t *MemoryControlTracker = NULL;

/**
 * MUMemoryInitialize(void)
 *  called to initialize the memory tracking data structure (which
 *  in turns enables the code to cache the tracked memory allocations
 *
 *  Idempotent.  
 *
 * If the MemoryControlTracker is NOT set, then proceed to attempt to initialize
 * if the environment variable MOABMEMTEST is present then allocate the table
 * otherwise, do not initialize
 *
 */

memtracker_t *MUMemoryInitialize(void)
  {
  /* If table not yet initialized and 'MOABMEMTEST' has been enabled THEN
   * allocate the memory table
   */
  if ((MemoryControlTracker == NULL) && (getenv("MOABMEMTEST") != NULL))
    {   
    /* get a memtracker_t struct that has many mmemtab_t entries in it */
    MemoryControlTracker = (memtracker_t *)calloc(1,(sizeof(memtracker_t)) +
                                  (sizeof(mmemtab_t) * (MMAX_MEMTABLESIZE + MMAX_MEMTABLEBUF)));
    if (MemoryControlTracker != NULL)
      {
      /* Set the MemTable to point at the memory just passed the end of the memtracker_t struct */
      MemoryControlTracker->MemTable = (mmemtab_t *)((char *) MemoryControlTracker) + sizeof(memtracker_t);
      }

    /* if the calloc fails, well, we won't track memory, but don't report it */
    }   

  return(MemoryControlTracker);
  }

/**
 *  MUMemoryTeardown is the reverse of the MUMemoryInitialize
 * it will free resources that were allocated during the initialization call
 */

void MUMemoryTeardown(memtracker_t *memory_tracker)
  {
  if ((MemoryControlTracker != NULL) && (MemoryControlTracker == memory_tracker))
    {
    free(MemoryControlTracker);
    MemoryControlTracker = NULL;
    }
  }


/**
 * Very profound function. Perfect example of creating
 * stub code that never grows up to be anything more than a waste
 * of space.
 */

int MUMemTrap(void)

  {
  return(SUCCESS);
  }


#ifndef MPROFMEM
/**
 * __MUMemAddToTrackingTable common local function 
 * for the main allocate functions to call to insert new 
 */
void __MUMemAddToTrackingTable(

  void *ptr,
  int size, 
  const char *who)

  {
    int index;

    mbool_t Found = FALSE;

    if (MemoryControlTracker == NULL)
      {
      return;
      }

    /* find an empty entry then add new ptr to the table 
     * (record ptr, size, and contents of alloc block) 
     * hash into the table for a starting point
     */
    for (
        index = ((mulong)ptr) % MMAX_MEMTABLESIZE;
        index < MMAX_MEMTABLESIZE + MMAX_MEMTABLEBUF;
        index++)
      {
      if (MemoryControlTracker->MemTable[index].Ptr <= (void *)1)
        {
        MemoryControlTracker->MemTable[index].Ptr = ptr;
        MemoryControlTracker->MemTable[index].Size = size;
   
        MDB(3,fSTRUCT) MLog("INFO:     %s %d bytes (%p)\n",
          who, MemoryControlTracker->MemTable[index].Size, MemoryControlTracker->MemTable[index].Ptr);
 
        MemoryControlTracker->TotalMemCount ++;
        MemoryControlTracker->TotalMemAlloc += MemoryControlTracker->MemTable[index].Size;

        Found = TRUE;
 
        break;
        }
      }    /* END for (index) */

    if (Found == FALSE)
      {
      MDB(0,fSTRUCT) MLog("ALERT:    mem table is full, cannot record %s of %p\n",
        ptr, who);
      }
  }


/**
 * __MUMemRemoveFromTrackingTable(char **Ptr)
 *
 * local support routine for tracking memory allocations
 */
void __MUMemRemoveFromTrackingTable(char **Ptr)
  {
  mbool_t Found = FALSE;
  int     index;
  void   *ptr;

  if (MemoryControlTracker == NULL)
    {
    return;
    }

  /* clear memory table entry for this 'returned' memory region */

  ptr = *Ptr;

  for (index = (mulong)ptr % MMAX_MEMTABLESIZE;
       index < MMAX_MEMTABLESIZE + MMAX_MEMTABLEBUF;
       index++)
    {
    if (MemoryControlTracker->MemTable[index].Ptr == ptr)
      {
      MemoryControlTracker->TotalMemCount --;
      MemoryControlTracker->TotalMemAlloc -= MemoryControlTracker->MemTable[index].Size;  
      /* note that this is not accurate for buffers that have been realloc'ed */

      MDB(3,fSTRUCT) MLog("INFO:     free %d bytes (%p)\n",
        MemoryControlTracker->MemTable[index].Size,
        MemoryControlTracker->MemTable[index].Ptr);

      MemoryControlTracker->MemTable[index].Ptr = (void *)1;
      MemoryControlTracker->MemTable[index].Size = 0;

      Found = TRUE;

      break;
      }
    }    /* END for (index) */

  if (Found == FALSE)
    {
    MDB(0,fSTRUCT) MLog("ALERT:    cannot locate memory %p to free\n", ptr);

    MUMemTrap();
    }
  }

/**
 *
 *
 * @param Ptr (I) [modified,freed]
 */

int MUFree(

  char **Ptr)  /* I (modified,freed) */

  {
  if ((Ptr == NULL) || (*Ptr == NULL))
    {
    return(SUCCESS);
    }

  __MUMemRemoveFromTrackingTable(Ptr);

  free(*Ptr);

  *Ptr = NULL;

  return(SUCCESS);
  }  /* END MUFree() */



/**
 * A Moab wrapper function for the calloc system call.
 *
 * @see MUFree() - peer
 * @see MUMalloc() - peer - alloc memory without clearing it
 *
 * @param Count (I)
 * @param Size (I)
 */

void *MUCalloc(

  int Count, /* I */
  int Size)  /* I */

  {
  void *ptr;

  ptr = calloc(Count,Size);

  if (ptr == NULL)
    {
    MDB(0,fSTRUCT) MLog("ERROR:    calloc() failed (count %d, size %d) errno=%d (%s)\n",
      Count,
      Size,
      errno,
      strerror(errno));

    MUMemTrap();

    return(NULL);
    }

  /* NYI - Note that we currently do not update the MemoryControlTracker->MemTable for realloc 
   *       calls.  We may need to have a MURealloc() function when we decide to
   *       track memory usage for reallocs.
   */

  __MUMemAddToTrackingTable(ptr,Count * Size, "calloc");

  return(ptr);
  }  /* END MUCalloc() */


/**
 * A Moab function wrapper for the malloc system call.
 *
 * @param Size (I)
 */

void *MUMalloc(

  int Size)  /* I */

  {
  void *ptr;

  ptr = malloc(Size);

  if (ptr == NULL)
    {
    MDB(0,fSTRUCT) MLog("ERROR:    malloc() failed, errno=%d (%s)\n",
      errno,
      strerror(errno));

    MUMemTrap();

    return(NULL);
    }

  __MUMemAddToTrackingTable(ptr,Size,"malloc");

  return(ptr);
  }  /* END MUMalloc() */


int MUStrNDup(

  char **DstP,  /* O */
  char  *Src,
  int   MaxSize)   /* I */

  {
  int index;
  char *newDest;
  if (DstP == NULL)
    {
    return(FAILURE);
    }

  if ((*DstP != NULL) &&
      (Src != NULL) &&
      (Src[0] == (*DstP)[0]) &&
      (!strcmp(Src,*DstP)))
    {
    /* strings are identical */
    return(SUCCESS);
    }

  /* Free the Destination pointer if any now */
  MUFree(DstP);

  if ((Src == NULL) || (Src[0] == '\0'))
    {
    return(SUCCESS);    /* Nothing to copy */
    }

  /* MUMalloc will add ptr to memory tracker table and
   * Ensure we have one byte for the NULL byte as well
   */
  newDest = (char *)MUMalloc((MaxSize + 1) * sizeof(char));
  if(newDest == NULL)
    {
    return(FAILURE);
    }

  /* Copy the src to the NEW destination */
  for(index = 0; index < MaxSize; index++)
    {
    if(Src[index] == 0)
      break;
    newDest[index] = Src[index];
    }

  newDest[index++] = 0;   /* ALWAYS ensure dest has a null byte at the end */

  /* If the src string is smaller than max size requested,
   * then shorten the allocated buffer to match actual string length
   */
  if(index < MaxSize + 1)
    {
    /* Must remove the newDest ptr from the tracking table because
     * it might be freed in realloc() and we would "lose" tracking of it
     */
    __MUMemRemoveFromTrackingTable(&newDest);

    /* If 'index' is zero (0), the memory will be freed and the return will be NULL */
    newDest = (char *)realloc(newDest,index);
    if(newDest == NULL)
      {
      MDB(0,fSTRUCT) MLog("ERROR:    realloc failed within MUMemStrNDup, errno=%d (%s)\n",
        errno,
        strerror(errno));

      MUMemTrap();

      return(FAILURE);
      }

    /* add back into the tracking table, the realloc() return  value */
    __MUMemAddToTrackingTable((void*) &newDest,index,"MUMemStrNDup");
    }

  /* Update the Destination pointer passed in, to the new buffer of data */
  *DstP = newDest;

  return(SUCCESS);
  }


/**
 * Performs a strdup on src to the dest.
 *
 * @param DstP (O)
 * @param Src (I)
 * @return return SUCCESS with NULL Dst if Src is NULL or Src[0] == '\0'
 */

int MUStrDup(

  char       **DstP,  /* O */
  char const *Src)   /* I */

  {
  if (DstP == NULL)
    {
    return(FAILURE);
    }

  if ((*DstP != NULL) &&
      (Src != NULL) &&
      (Src[0] == (*DstP)[0]) &&
      (!strcmp(Src,*DstP)))
    {
    /* strings are identical */

    return(SUCCESS);
    }

  MUFree(DstP);   /* Free any lingering dest ptr */

  /* If src is null or just empty, then we are done */
  if ((Src == NULL) || (Src[0] == '\0'))
    {
    return(SUCCESS);
    }

  *DstP = strdup(Src);    /* do the strdup now */

  if (*DstP == NULL)
    {
    MDB(0,fSTRUCT) MLog("ERROR:    strdup failed, errno=%d (%s)\n",
      errno,
      strerror(errno));

    MUMemTrap();

    return(FAILURE);
    }
 
  /* We have a new destptr, so add it to the tracking table as well */
  __MUMemAddToTrackingTable((void*) *DstP,strlen(*DstP) + 1,"strdup");

  return(SUCCESS);
  }  /* END MUStrDup() */
#endif /* !MPROFMEM */


/**
 *
 *
 * @param DoVerbose (I)
 * @param Buf (O)
 * @param BufSize (I)
 */

int MUMemReport(

  mbool_t  DoVerbose,  /* I */
  char    *Buf,        /* O */
  int      BufSize)    /* I */

  {
  char *BPtr = NULL;
  int   BSpace = 0;

  int   index;
  int   bcount;
 
  if (MemoryControlTracker == NULL)
    {
    return(SUCCESS);
    }

  if (Buf != NULL)
    {
    MUSNInit(&BPtr,&BSpace,Buf,BufSize);

    MUSNPrintF(&BPtr,&BSpace,"Total Memory:  %.2f MB (%d bytes) - %d blocks\n",
      (double)MemoryControlTracker->TotalMemAlloc / (1 << 20),
      MemoryControlTracker->TotalMemAlloc,
      MemoryControlTracker->TotalMemCount);
    }
  else
    {
    MDB(0,fSTRUCT) MLog("INFO:     Total Memory:  %.2f MB (%d bytes) - %d blocks\n",
      (double)MemoryControlTracker->TotalMemAlloc / (1 << 20),
      MemoryControlTracker->TotalMemAlloc,
      MemoryControlTracker->TotalMemCount);
    }

  if (DoVerbose == FALSE)
    {
    return(SUCCESS);
    }

  bcount = 0;

  for (index = 0;index < MMAX_MEMTABLESIZE + MMAX_MEMTABLEBUF;index++)
    {
    if (MemoryControlTracker->MemTable[index].Ptr > (void *)1)
      {
      bcount++;

      if (Buf != NULL)
        {
        MUSNPrintF(&BPtr,&BSpace,"Block[%06d]  Size=%-7d  Address=%p\n",
          bcount,
          MemoryControlTracker->MemTable[index].Size,
          MemoryControlTracker->MemTable[index].Ptr);
        }
      else
        {
        MDB(0,fSTRUCT) MLog("INFO:     Block[%06d]  Size=%-7d  Address=%p\n",
          bcount,
          MemoryControlTracker->MemTable[index].Size,
          MemoryControlTracker->MemTable[index].Ptr);
        }
      }
    }    /* END for (index) */

  return(SUCCESS);
  }  /* MUMemReport() */

/* END MMemory.c */
