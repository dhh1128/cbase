
/* HEADER */

/**
 * @file MProfMemory.c
 *
 * Contains Memory Profiling functions
 */

#include "moab.h"
#include "moab-proto.h"
#include "moab-const.h"
#include "moab-global.h"

mulong AllocMemory = 0;  /* temporary global variable */

/**
 * This function will free the given piece of memory.
 * It is used when profiling memory usage.
 *
 * @see MUProfMalloc()
 *
 * NOTE: Compile with -DMPROFMEM to automatically
 * use this function. It will be called via a macro
 * and automatically fill in the File and Line params.
 *
 * @param File
 * @param Line
 * @param Ptr  (I) [modified,freed]
 */

int MUProfFree(

  char *File,
  int   Line,
  char **Ptr)

  {
  if ((Ptr == NULL) || (*Ptr == NULL))
    {
    return(SUCCESS);
    }

  MDB(1,fSTRUCT) MLog("MEM:    free() @ %s:%d %p\n",
    File,
    Line,
    *Ptr);

  free(*Ptr);

  *Ptr = NULL;

  return(SUCCESS);
  }  /* END MUProfFree() */


/**
 * This function is used for freeing within data structures (like in MULLAdd(), etc.) when
 * profiling memory.
 *
 * NOTE: Compile with -DMPROFMEM to automatically
 * use this function. It will be called via a macro
 * and automatically fill in the File and Line params.
 *
 * @see MUProfMalloc()
 *
 */
int MUProfFreeTrad(

  char **Ptr)  /* I (modified,freed) */

  {
  if ((Ptr == NULL) || (*Ptr == NULL))
    {
    return(SUCCESS);
    }

  MDB(1,fSTRUCT) MLog("MEM:    free() %p\n",
    *Ptr);

  free(*Ptr);

  *Ptr = NULL;

  return(SUCCESS);
  }  /* END MUFree() */




/**
 * A Moab wrapper function for the calloc system call.
 * Used for profiling memory usage.
 *
 * @see MUProfMalloc()
 *
 * NOTE: Compile with -DMPROFMEM to automatically
 * use this function. It will be called via a macro
 * and automatically fill in the File and Line params.
 *
 * @param File  (I)
 * @param Line  (I)
 * @param Count (I)
 * @param Size  (I)
 */

void *MUProfCalloc(

  char *File,
  int   Line,
  int   Count,
  int   Size)

  {
  void *ptr;

#ifdef MPROFMEM
#undef calloc
#endif
  ptr = calloc(Count,Size);
#ifdef MPROFMEM
#define calloc(x,y) MUProfCalloc(__FILE__,__LINE__,(x),(y))
#endif

  if (ptr == NULL)
    {
    MDB(0,fSTRUCT) MLog("MEM:    calloc() failed, errno=%d (%s)\n",
      errno,
      strerror(errno));

    return(NULL);
    }

  AllocMemory += Count * Size;

  MDB(1,fSTRUCT) MLog("MEM:    calloc(%d,%d) @ %s:%d %p (%ld total)\n",
      Count,
      Size,
      File,
      Line,
      ptr,
      AllocMemory);

  return(ptr);
  }  /* END MUProfCalloc() */




/**
 * A Moab function wrapper for the malloc system call.
 * Used for profiling memory usage. This function, or other MUProf* functions,
 * should be be called directly. Rather, compiling with -DMPROFMEM will
 * cause macros to be defined to ensure that all allocations/frees are logged in the
 * moab.log file. These logs can then be saved off and processed using a set of
 * Python scripts to help developers understand Moab's memory patterns.
 *
 * @see MUProfMalloc()
 *
 * NOTE: Compile with -DMPROFMEM to automatically
 * use this function. It will be called via a macro
 * and automatically fill in the File and Line params.
 *
 * @param File (I)
 * @param Line (I)
 * @param Size (I)
 */

void *MUProfMalloc(

  char *File,
  int   Line,
  int   Size)  /* I */

  {
  void *ptr;

#ifdef MPROFMEM
#undef malloc
#endif
  ptr = malloc(Size);
#ifdef MPROFMEM
#define malloc(x) MUProfMalloc(__FILE__,__LINE__,(x))
#endif


  if (ptr == NULL)
    {
    MDB(0,fSTRUCT) MLog("MEM:    malloc() failed, errno=%d (%s)\n",
      errno,
      strerror(errno));

    return(NULL);
    }

  AllocMemory += Size;

  MDB(1,fSTRUCT) MLog("MEM:    malloc(%d) @ %s:%d %p (%ld total)\n",
      Size,
      File,
      Line,
      ptr,
      AllocMemory);

  return(ptr);
  }  /* END MUProfMalloc() */





/**
 * A Moab function wrapper for the realloc() system call.
 * Used for profiling memory usage.
 *
 * @see MUProfMalloc()
 *
 * NOTE: Compile with -DMPROFMEM to automatically
 * use this function. It will be called via a macro
 * and automatically fill in the File and Line params.
 *
 * @param File (I)
 * @param Line (I)
 * @param Ptr (I)
 * @param Size (I)
 */

void *MUProfRealloc(

  char *File,
  int   Line,
  void *Ptr,
  int   Size)

  {
  void *ptr;

#ifdef MPROFMEM
#undef realloc
#endif
  ptr = realloc(Ptr,Size);
#ifdef MPROFMEM
#define realloc(x,y) MUProfRealloc(__FILE__,__LINE__,(x),(y))
#endif

  if (ptr == NULL)
    {
    MDB(0,fSTRUCT) MLog("MEM:    realloc() failed, errno=%d (%s)\n",
      errno,
      strerror(errno));

    return(NULL);
    }

  AllocMemory += Size;

  MDB(1,fSTRUCT) MLog("MEM:    realloc(%d) @ %s:%d %p (%ld total)\n",
      Size,
      File,
      Line,
      ptr,
      AllocMemory);

  return(ptr);
  }  /* END MUProfRealloc() */





/**
 * Performs a strdup on src to the dest. This version of the function is used
 * for memory profiling.
 *
 * @see MUProfMalloc()
 *
 * NOTE: Compile with -DMPROFMEM to automatically
 * use this function. It will be called via a macro
 * and automatically fill in the File and Line params.
 *
 * @param File (I)
 * @param Line (I)
 * @param DstP (O)
 * @param Src (I)
 * @return return SUCCESS with NULL Dst if Src is NULL or Src[0] == '\0'
 */

int MUProfStrDup(

  char  *File,  /* I */
  int    Line,  /* I */
  char **DstP,  /* O */
  char  *Src)   /* I */

  {
  int Size = -1;

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

  MUFree(DstP);

  if ((Src == NULL) || (Src[0] == '\0'))
    {
    return(SUCCESS);
    }

#ifdef MPROFMEM
#undef strdup
#endif
  *DstP = strdup(Src);
#ifdef MPROFMEM
#define strdup(x,y) MUProfStrDup(__FILE__,__LINE__,(x),(y))
#endif

  if (*DstP == NULL)
    {
    MDB(0,fSTRUCT) MLog("ERROR:    strdup failed, errno=%d (%s)\n",
      errno,
      strerror(errno));

    return(FAILURE);
    }

  Size = strlen(Src) + 1;  /* Need to +1 for terminating NULL char */

  MDB(1,fSTRUCT) MLog("MEM:    strdup(%d) @ %s:%d %p\n",
      Size,
      File,
      Line,
      *DstP);

  return(SUCCESS);
  }  /* END MUProfStrDup() */




/**
 * Performs a traditional strdup of Src. This version of the function is used
 * for memory profiling.
 *
 * @see MUProfMalloc()
 *
 * NOTE: Compile with -DMPROFMEM to automatically
 * use this function. It will be called via a macro
 * and automatically fill in the File and Line params.
 *
 * @param File (I)
 * @param Line (I)
 * @param Src (I)
 * @return Pointer to the newly allocated string.
 */

char *MUProfStrDupTrad(

  char  *File,  /* I */
  int    Line,  /* I */
  char  *Src)   /* I */

  {
  char *Result = NULL;
  int   Size = -1;

  if (Src == NULL)
    {
    return(NULL);
    }

#ifdef MPROFMEM
#undef strdup
#endif
  Result = strdup(Src);
#ifdef MPROFMEM
#define strdup(x) MUProfStrDupTrad(__FILE__,__LINE__,(x))
#endif

  if (Result == NULL)
    {
    MDB(0,fSTRUCT) MLog("ERROR:    strdup failed, errno=%d (%s)\n",
      errno,
      strerror(errno));

    return(NULL);
    }

  Size = strlen(Src) + 1;

  MDB(1,fSTRUCT) MLog("MEM:    strdup(%d) @ %s:%d %p\n",
      Size,
      File,
      Line,
      Result);

  return(Result);
  }  /* END MUProfStrDupTrad() */

/* END MUProfMemory.c */
