/* HEADER */

/**
 * @file MUSNBuffer.c
 *
 * overrun-save buffer functions
 *
 */

#include "moab.h"
#include "moab-proto.h"



/**
 * Initialize overrun-safe buffer.
 *
 * @see MUSNUpdate() - peer
 * @see MUSNPrintF() - peer
 * @see MUSNCat() - peer
 *
 * @param BPtr (O) [modified]
 * @param BSpace (O)
 * @param SrcBuf (I)
 * @param SrcBufSize (I)
 */

int MUSNInit(

  char  **BPtr,       /* O (modified) */
  int    *BSpace,     /* O */
  char   *SrcBuf,     /* I */
  int     SrcBufSize) /* I */

  {
  if ((BPtr == NULL) ||
      (BSpace == NULL) ||
      (SrcBuf == NULL) ||
      (SrcBufSize <= 0))
    {
    return(FAILURE);
    }

  *BPtr = SrcBuf;

  *BSpace = SrcBufSize;

  (*BPtr)[0] = '\0';

  return(SUCCESS);
  }  /* END MUSNInit() */





/**
 * Adjust BPtr and BSpace values to point to end of string.
 *
 * (Looks at the length of string pointed to by BPtr
 * and decrements BSpace accordingly. Also places
 * the BPtr at the end of the string.)
 *
 * @param BPtr (I/O) [modified]
 * @param BSpace (I/O)
 */

int MUSNUpdate(

  char  **BPtr,       /* I/O (modified) */
  int    *BSpace)     /* I/O */

  {
  int len;

  if ((BPtr == NULL) ||
      (BSpace == NULL) ||
      (*BPtr == NULL))
    {
    return(FAILURE);
    }

  if (*BSpace <= 0)
    {
    return(FAILURE);
    }

  len = strlen(*BPtr);

  if (len >= *BSpace)
    {
    *BPtr = NULL;
    *BSpace = 0;
    }
  else
    {
    *BPtr += len;

    *BSpace -= len;
    }

  return(SUCCESS);
  }  /* END MUSNUpdate() */





#ifndef __MTEST  
/**
 * Writes data to the given buffer (BPtr), but will not overflow
 * the buffer per BSpace. Uses a format string like sprintf() and
 * a variable amount of args.
 *
 * @see MUSNCat() - peer
 * @see MUSNInit() - peer
 * @see MUSNXPrintF() - peer
 *
 * @param BPtr (I) Pointer to the buffer that the string should be written to.
 * @param BSpace (I) Size left in the buffer.
 * @param Format (I) The format string.
 * @param ... (I) Variable amount of args.
 */

int MUSNPrintF(

  char       **BPtr,
  int         *BSpace,
  const char  *Format,
  ...)

  {
  int len;

  va_list Args;

  if ((BPtr == NULL) ||
      (BSpace == NULL) ||
      (Format == NULL) ||
      (*BSpace <= 0))
    {
    return(FAILURE);
    }

  va_start(Args,Format);

  len = vsnprintf(*BPtr,*BSpace,Format,Args);

  va_end(Args);

  if (len <= 0)
    {
    return(FAILURE);
    }

  if (len >= *BSpace)
    {
    /* truncation occurred due to attempted
     * overflow! */

    /* do not place BPtr past the end of the buffer:
     * it is too dangerous (calling function could derference it
     * to check for empty string, etc.)! */

    *BPtr += (*BSpace) - 1;
    *BSpace = 0;

    return(FAILURE);
    }

  *BPtr += len;
  *BSpace -= len;

  return(SUCCESS);
  }  /* END MUSNPrintF() */
#endif /*! __MTEST */


/**
 * Writes data to the given buffer (BPtr), but will grow the buffer (realloc)
 * if it is not large enough for the format string and variables. This function
 * is used by MUSNXPrintF(), MStringAppend(), MStringSet(), etc.
 *
 * BPtr and BHead must point to the same allocated buffer.
 *
 * @param BPtr        (I) Pointer to the location in the buffer that the string should be written to. [realloc]
 * @param BSpace      (I/O) Size left in the buffer. [modified]
 * @param BHead       (I/O) Pointer to the beginning of the buffer. [realloc]
 * @param BSize       (I/O) The total size of the buffer. This will change if the buffer is realloc'd.
 * @param TryAgain    (O)
 * @param HaveVarArgs (I) Specifies whether to interpret String as a format String with subsequent arguments or as a literal string with no subsequent arguments
 * @param String      (I) The format string.
 * @param Args        (I) Variable list of args
 */

int MUSNXFlexiblePrint(

  char      **BPtr,
  int        *BSpace,
  char      **BHead,
  int        *BSize,
  mbool_t    *TryAgain,
  mbool_t     HaveVarArgs,
  const char *String,
  va_list     Args)

  {
  int len;

  if ((BPtr == NULL) ||
      (BHead == NULL) ||
      (BSpace == NULL) ||
      (BSize == NULL) ||
      (String == NULL) ||
      (TryAgain == NULL) ||
      (*BSize <= 0) ||
      (*BSpace <= 0))
    {
    return(FAILURE);
    }

  *TryAgain = FALSE;

  if (HaveVarArgs)
    {
    /* perform the formating with the Arguments, possible error returned */
    len = vsnprintf(*BPtr,*BSpace,String,Args);
    }
  else
    {
    /* len is the actual length of the string, with an implicit NULL byte
     * from the end of the string, so the true length of the string buffer
     * is 'len + 1'
     */
    len = strlen(String);
    }

  /* if the length is greater than or equal to the space in the buffer, we need to
   * grow the buffer, and then have the caller re-call us.
   * If len is < 0, the error check is below, while the following check will fail
   * since len < 0 and BSpace is 0 and greater
   */
  if (len >= *BSpace)
    {
    /* buffer is too small for given format string and variables */

    int GrowSize;
    int NewPos;
    int NewSize;

    GrowSize = len - *BSpace + 1;

    /* grow by 1/2 original size + grow size
     * (this gives us extra space for future and guarantees that the current string will fit in) */

    NewSize = *BSize + (int)(*BSize * 0.5) + GrowSize + 1; /* +1 for NULL terminator */

    /* find out how far we are already in the buffer (in case the OS gives us a new address for the realloc'd buf) */

    NewPos = *BPtr - *BHead;

    *BHead = (char *)realloc(*BHead,sizeof(char) * NewSize);

    if (*BHead == NULL)
      {
      return(FAILURE);
      }

    /* set the controls to point to the new buffer (if needed by the realloc() call) */
    *BSize = NewSize;
    *BSpace = NewSize - NewPos;
    *BPtr = &(*BHead)[NewPos];

    *TryAgain = TRUE;

    return(FAILURE);
    }

  /* Check for error HERE */
  if (len < 0)
    {
    return(FAILURE);
    }

  /* strncpy will not null-terminate unless the third parameter is at least strlen(String) + 1 */

  if (!HaveVarArgs)
    strncpy(*BPtr,String,len + 1);

  *BPtr += len;
  *BSpace -= len;

#if 0
  /* the following cannot be reached, b/c the case of len being >= BSpace was checked above
   * Hence the following really is dead code.
   *
   * Try to generate a unittest to trip the IF statement, but BSpace would never become
   * zero nor negative. Leaving the code for now, but with this explaination for further research.
   */
  if (*BSpace <= 0)
    {
    int GrowSize;
    int NewPos;
    int NewSize;

    GrowSize = *BSpace + 1;

    /* grow by 1/2 original size + grow size
     * (this gives us extra space for future and guarantees that the current string will fit in) */

    NewSize = *BSize + (int)(*BSize * 0.5) + GrowSize + 1; /* +1 for NULL terminator */

    /* find out how far we are already in the buffer (in case the OS gives us a new address for the realloc'd buf) */

    NewPos = *BPtr - *BHead;

    *BHead = (char *)realloc(*BHead,sizeof(char) * NewSize);

    if (*BHead == NULL)
      {
      return(FAILURE);
      }

    *BSize = NewSize;
    *BSpace = NewSize - NewPos;

    *BPtr = &(*BHead)[NewPos];
    }
#endif

  return(SUCCESS);
  }  /* END MUSNXPrintFVAList() */






/**
 * Concatenates two strings together.
 *
 * @param BPtr (I) [modified]
 * @param BSpace (I) [modified]
 * @param Src (I)
 */

int MUSNCat(

  char **BPtr,   /* I (modified) */
  int   *BSpace, /* I (modified) */
  char  const *Src)    /* I */

  {
  int index;

  if ((BPtr == NULL) || (BSpace == NULL) || (*BSpace <= 0))
    {
    return(FAILURE);
    }

  if ((Src == NULL) || (Src[0] == '\0'))
    {
    return(SUCCESS);
    }

  for (index = 0;index < *BSpace - 1;index++)
    {
    if (Src[index] == '\0')
      break;

    (*BPtr)[index] = Src[index];
    }  /* END for (index) */

  (*BPtr)[index] = '\0';

  *BPtr   += index;
  *BSpace -= index;

  return(SUCCESS);
  }  /* END MUSNCat() */


#if 0

/* Not called/used in  moab and not clear as to its purpose nor function
 * turn off for now
 */

/**
 * Join elements of an array into a string delimited by Delim. Each element
 * is printed using the function Printer.
 *
 * @param BPtr        (I/O)
 * @param BSpace      (I/O)
 * @param DataStart   (I)
 * @param DataEnd     (I)
 * @param DataSize    (I)
 * @param Printer     (I)
 * @param Delim      (I)
 */

int MUStrJoin(

  char             **BPtr,   /* I/O the string to append to */
  int               *BSpace,
  void const        *DataStart, /* I The start of the data segment to print */
  void const        *DataEnd,   /* I the exclusive end of the data segment to print */
  int unsigned       DataSize,  /* I The size of each element in the data segment */
  mustr_printer_t    Printer,   /* I The function that will render each item as text */
  char const * const Delim)     /* I The delimeter to separate each piece of text */

  {
  unsigned char *CurData = (unsigned char *)DataStart;
  unsigned char *End = (unsigned char *)DataEnd;
  char *BPtrLast = *BPtr;

  if (DataStart >= DataEnd)
    return(SUCCESS);

  Printer(BPtr,BSpace,CurData);

  for (CurData += DataSize; CurData < End; CurData += DataSize)
    {
    if (BPtrLast != *BPtr)
      MUSNCat(BPtr,BSpace,(char *)Delim);

    BPtrLast = *BPtr;
    Printer(BPtr,BSpace,CurData);
    }

  return(SUCCESS);
  } /* END MUStrJoin() */

#endif

