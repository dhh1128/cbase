/* HEADER */

/** @file MString.c
 *
 * Contains mstring_t object functions
 *
 */

#include "moab.h"
#include "moab-proto.h"


using namespace std;


#define EXTRA_LEN   32

const size_t mstring_t::npos = -1;

      
/* private helper function to free any buffer now and alloc a new one */
void mstring_t::reinitInstance(int Size)
  {
  freeInstance();

  CString = NULL;
  CStringSize = 0;
  BPtr = NULL;
  BSpace = 0;

  allocInstance(Size);
  }


/*
 * mstring_t::operator= methods
 */

mstring_t& 
mstring_t::operator= (const mstring_t &rhs)
  {
  /* check for NULL rhs - yeah, moab does that */
  if (NULL == &rhs)
    return(*this);

  /* check for self reference */
  if (this == &rhs)
    return(*this);

  /* Get actual length of rhs string */
  int rhsLength = rhs.length();
    
  /* if RHS has data, copy it to the Dest */
  if ((rhs.CString != NULL) && (rhsLength > 0 ))
    {
    int newSize = (int) rhsLength + EXTRA_LEN;

    clear();  /* Reset the lhs pointers/space if valid */

    /* If the RHS size won't fit into the LHS, free LHS
     * and allocate a buffer big enough to fit
     */
    if ((NULL == CString) || (newSize >= CStringSize))
      {
      reinitInstance(newSize);
      }

    /* Copy the RHS CString to LHS CString */
    strcpy(CString,rhs.CString);

    /* determine offset withing src string of current cursor 
     * set the dest curser with that same offset
     */
    int offset = rhs.BPtr - rhs.CString;
    BPtr = CString + offset;
    BSpace = CStringSize - offset;
    }

  return *this;
  } /* END operator= */

/*
 * mstring_t::operator= (const char *s)
 *
 * const char *s -> null terminated string
 */

mstring_t& 
mstring_t::operator= (const char *s)
  {
  if (s != NULL)
    {
    size_t len = strlen(s);

    /* If string is greater than current buffer, free
     * old buffer and get a bigger one
     */
    if ((NULL == CString) || ((len + 1) > (size_t) CStringSize))
      {
      reinitInstance(len + EXTRA_LEN);
      }

    /* copy rhs src to lhs dest */
    strcpy(CString,s);

    BPtr = CString + len;
    BSpace = CStringSize - len;
    }

  return *this;
  } /* END operator= */

/*
 * mstring_t::operator= (char c)
 */

mstring_t& 
mstring_t::operator= (char c)
  {
  if ((NULL == CString) || (CStringSize <= 1))
    {
    reinitInstance(EXTRA_LEN);
    }

  /* Assigning a single char at the BEGINNING of the buffer, reset ptr/space */
  BPtr = CString;
  BSpace = CStringSize;

  /* Place the char therein w/trailing null byte, where the trailing byte is not counted */
  *BPtr++ = c;     BSpace--;
  *BPtr = '\0';  

  return *this;
  } /* END operator= */


/*
 * mstring_t(const mstring_t& rhs)  copy constructor 
 */

mstring_t::mstring_t(const mstring_t& rhs)
      : CString( (char *) calloc(1,rhs.CStringSize + EXTRA_LEN)), CStringSize(rhs.CStringSize + EXTRA_LEN)
  {
    /* copy the string to the dest */
    std::copy(rhs.CString,rhs.CString + rhs.CStringSize, CString);

    /* setup the BPtr and BSpace as well */
    int offset = rhs.BPtr - rhs.CString;
    BPtr = CString + offset;
    BSpace = rhs.CStringSize + EXTRA_LEN - offset;
  } /* END mstring_t () */

/* mstring_t::operator+= method */

mstring_t & mstring_t::operator+= (const mstring_t &rhs)
  {
  /* check for self reference */
  if (this == &rhs)
    return(*this);

  /* If rhs is empty, just return lhs */
  if ((rhs.CString == NULL) || (rhs.CStringSize == 0))
    return(*this);

  size_t lhsLength = BPtr - CString;
  size_t rhsLength = rhs.BPtr - rhs.CString;
  size_t newSize = lhsLength + rhsLength + 1;

  /* check if lhs buffer is big enough, 
   * if NOT, then get a bigger buffer and 
   * copy lhs data to the new buffer
  */
  if (newSize > (size_t) CStringSize)
    {
    reserve(newSize + EXTRA_LEN);
    }

  /* Append the rhs string to the end of the lhs string's buffer */
  std::copy(rhs.CString,rhs.CString + rhsLength,BPtr);

  /* bump the pointer and dec the space, then terminate */
  BPtr += rhsLength;
  BSpace -= rhsLength;

  /* Ensure string is terminated properly */
  *BPtr = '\0';
    
  return *this;
  }


/* mstring_t::operator+= method */

mstring_t & mstring_t::operator+= (const char *s)
  {
  if (NULL == s)
    return *this;

  if (s != NULL)
    {
    size_t lhsLength = strlen(CString);
    size_t rhsLength = strlen(s);
    size_t newSize = lhsLength + rhsLength + 1;

    /* If string is greater than current buffer space, free
     * old buffer and get a bigger one
     */
    if (newSize > (size_t) CStringSize)
      {
      reserve(newSize + EXTRA_LEN);
      }

    strcat(BPtr,s);

    BPtr += rhsLength;
    BSpace -= rhsLength;

    *BPtr = '\0';
    }

  return *this;
  } /* END operator+= */


/* mstring_t::operator+= method */

mstring_t & mstring_t::operator+= (const char c)
  {
  char str[2];

  str[0] = c;
  str[1] = '\0';

  *this += str;
  return(*this);
  }

/* mstring_t::operator+ method */
const mstring_t mstring_t::operator+ (const mstring_t &rhs) const
  {
  /* use the operator+= to our advantage */
  return mstring_t(*this) += rhs;
  }


/* API .reserve method */

void mstring_t::reserve(size_t res_arg)
  {
  size_t len = length();  /* what the length of current string ? */

  /* if request is less than current, then ignore the request 
   * this still allows capacity to shrink down to the size of the
   * current string lenth
   */
  if (res_arg <= len)
    return;

  /* new size is bigger than current buffer, get a new ZEROED buffer, 
   * size will already be at least length() + 1 due to above check
   */
  char *tmpBuff = (char *) calloc(1,(int) res_arg);

  /* set the current pointer */
  BPtr = tmpBuff + len;
  BSpace = res_arg - len;

  /* if we have an old buffer, copy old buffer contents to new memory */
  if (CString != NULL)
    {
    /*  strncpy will not null-terminate if maxed, but we know there is room */
    if (len != 0)
      strncpy(tmpBuff,CString,len+1);

    /* Free old buffer now */
    free(CString);
    }

  /* reinit the base/space pointer/value to new buffer */
  CString = tmpBuff;
  CStringSize = res_arg;
  }

/* API .find(char c, size_t pos = 0) */

size_t mstring_t::find(char c, size_t pos) const
  {
  char *p;

  /* If position is beyond the length of the string, then not found */
  if (pos >= length())
    return(npos);

  /* look for the character 'c' */
  p = strchr(&CString[pos],c);
  if (NULL == p)
    return(npos);   /* not found */
 
  /* Return the position of the found character in the buffer */
  return(p - CString);
  }

/* API .erase(size_t pos = 0, size_t n = npos_ */

mstring_t& mstring_t::erase(size_t pos, size_t n)
  {
  if (empty())
    return(*this);

  size_t len = BPtr - CString;

  /* check for a complete erasure */
  if ((pos == 0) && (n == npos))
    {
    clear();
    return(*this);
    }

  /* if count is MAX, then clip to str length */
  if (n == npos)
    n = len;

  /* check Out of bounds, but we won't throw an exception in this
   * case, just ignore it for now
   */
  if (pos >= len)
    return(*this);

  /* bound the upper position to count from pos to len (including null byte) */
  if ((pos + n) > len)
    n = len - pos;

  int move_cnt = len + 1 - pos - n;    /* how many bytes to move */

  /* move the string */
  memmove(&CString[pos],&CString[pos+n],move_cnt);

  /* Adjust pointers */
  BPtr -= n;
  BSpace += n;

  return(*this);
  }

/* 
 * __appendFormat support function 
 *
 * @param  MString    (I/O)
 * @param  Format     (I)
 * @param  Args       (I)
 */

int __appendFormat(

  mstring_t  *MString,
  bool       *repeat,
  int        *bufferLen,
  const char *Format,
  va_list     Args)

  {
  int   p_len;
  char *buffer = NULL;

  *repeat = false;

  /* Get a buffer block */
  buffer = (char *) calloc(1,*bufferLen);

  /* Attempt to format it, and if it doesn't fit it will
   * return what size the formated needed, so re-attempt
    */
  p_len = vsnprintf(buffer,*bufferLen,Format,Args);

  /* Check for error HERE */
  if (p_len < 0)
    {
    free(buffer);
    return(FAILURE);
    }

  /* if the length is greater than or equal to what we asked for, we need to grow the buffer */
  if (p_len >= *bufferLen)
    {
    /* buffer is too small for given format string and variables 
     * free current buffer and get a bigger one
     */
    free(buffer);
    *bufferLen <<= 1;
    *repeat = true;
    return(FAILURE);
    }

  /* Append the formated string to the mstring, then free the buffer */
  *MString += buffer;
  free(buffer);

  return(SUCCESS);
  }  /* END __appendFormat() */


/**
 * Sets the mstring to the given value. Allocates
 * memory as needed. Will remove any existing contents
 * of the mstring!!!
 *
 * @see MStringSetF() - peer
 * @see MStringAppend() - peer
 *
 * @param MString (I)
 * @param String (I)
 */

int MStringSet(

  mstring_t   *MString,
  const char  *String)

  {
  if ((MString == NULL) ||
      (String == NULL))
    {
    return(FAILURE);
    }

  *MString = String;

  return(SUCCESS);
  }  /* END MStringSet() */





/**
 * Sets the mstring to the given value. Allocates
 * memory as needed. Will remove any existing contents
 * of the mstring!!!
 *
 * @see MStringSet() - peer
 * @see MStringAppendF() - peer
 *
 * @param MString (I)
 * @param Format (I)
 * @param ... (I) Arguments to add to Format
 */

int MStringSetF(

  mstring_t  *MString,
  const char *Format,
  ...)

  {
  int     rc;
  va_list Args;
  bool    repeat = false;
  int     bufferLen = MMAX_LINE;

  if ((MString == NULL) ||
      (Format == NULL))
    {
    return(FAILURE);
    }

  if (MString->c_str() == NULL)
    {
    /* initialize string */

    MString->reinitInstance(MMAX_LINE);
    }

  /* clear contents of current string */
  MString->clear();

  /* Append the format output to the mstring */

  do {
    repeat = false;

    va_start(Args,Format);
    rc = __appendFormat(MString,&repeat,&bufferLen,Format,Args);
    va_end(Args);
  } while((rc == FAILURE) && (repeat == true));

  return(rc);
  }  /* END MStringSetF() */



/**
 * Appends a string to the given mstring. Allocates memory as needed.
 *
 * @see MStringAppendF() - peer
 * @see MStringSet() - peer
 *
 * @param MString (I)
 * @param String (I)
 */

int MStringAppend(

  mstring_t  *MString,
  char const *String)

  {
  if ((MString == NULL) ||
      (String == NULL))
    {
    return(FAILURE);
    }

  if (MString->c_str() == NULL)
    {
    /* initialize string */

    MString->reinitInstance(MMAX_LINE);
    }

  *MString += String;

  return(SUCCESS);
  }  /* END MStringAppend() */



/*
 * Appends Count characters from String to MString.
 *
 *  It does so by saving the character at String[Count], setting String[Count]
 *   to '\0' (terminating the string), and doing and MStringAppend.
 *  The original character is then put back into String[Count].
 *
 * @param MString - [O] The mstring_t to modify
 * @param String  - [I] The string to copy from
 * @param Count   - [I] How many characters to copy
 */

int MStringAppendCount(

  mstring_t  *MString,  /* O */
  char       *String,   /* I */
  int         Count)    /* I */

  {
  char *ptr;
  char  tmpChar;

  if ((MString == NULL) || (String == NULL) || (Count < 0))
    {
    return(FAILURE);
    }

  ptr = String + Count;

  /* Don't need to check end of the string.
      We put the char back, and if it ends before, MStringAppend
      will just append to there */

  tmpChar = *ptr;
  *ptr = '\0';

  *MString += String;

  *ptr = tmpChar;

  return(SUCCESS);
  }





/**
 * Appends a string to the given mstring. Allocates
 * memory as needed.
 *
 * @param MString (I)
 * @param Format (I)
 * @param ... (I) Arguments to add to Format
 */

int MStringAppendF(

  mstring_t  *MString,
  const char *Format,
  ...)

  {
  int     rc;
  va_list Args;
  bool    repeat = false;
  int     bufferLen = MMAX_LINE;

  if ((MString == NULL) ||
      (Format == NULL))
    {
    return(FAILURE);
    }

  if (MString->c_str() == NULL)
    {
    /* initialize string */

    MString->reinitInstance(MMAX_LINE);
    }

  do {
    repeat = false;

    va_start(Args,Format);
    rc = __appendFormat(MString,&repeat,&bufferLen,Format,Args);
    va_end(Args);
  } while((rc == FAILURE) && (repeat == true));

  return(rc);
  }  /* END MStringAppendF() */



/**
 * Strips the mstring from the index.  
 *
 * If index is a positive number, 
 *    mstring from that index on will be removed.  
 *    So, if index = 1, only the first character in mstring will remain (mstring[0]).  
 *
 * If index is *  a negative number, mstring will be removed that many from the end 
 *  (so -1 will remove the last character, -2 will remove the last 2 
 *  chars, etc).
 *
 * @param String (I) - modified
 * @param Index  (I)
 */

int MStringStripFromIndex(

  mstring_t *String,  /* I (modified) */
  int Index)          /* I (0 is not set) */

  {
  int CStrLen;
  int tmpI;

  /* bad parameter check */
  if (String == NULL)
    {
    return(FAILURE);
    }

  CStrLen = String->size();

  /* Check bounds of the Index, postive and negative check */
  if ((Index > (CStrLen - 1)) || ((0 - Index) > (CStrLen)))
    {
    /* Index is out of bounds */

    return(FAILURE);
    } 

  if ((Index == 0) || ((0 - Index) == CStrLen))
    {
    /* (0 - Index) == CStrLen means that if we count from the end we get index 0 */

    /* Just clear the string */

    String->clear();

    return(SUCCESS);
    }

  /* Now work on the positive Index */

  if (Index > 0)
    {
    /* Clear the string from 'Index' to the end */
    String->erase(Index);
    }
  else
    {
    /* Index is negative (0 checked above), remove from end (-1 is last char, etc.) */

    for (tmpI = CStrLen - 1;tmpI >= (CStrLen + Index);tmpI--)
      {
      String->erase(tmpI,1);
      }
    }

  return(SUCCESS);
  } /* END MStringStripFromIndex() */


/**
 * Will insert an argument (e.g. 'ARG=VAL' or 'ARG:VAL') into a string, seperating the
 * argument from others with the given delimeter. This is a general use function to help
 * cut down on the amount of copy-paste code throughout Moab, especially when dealing with
 * RM extension strings.
 *
 * @param Str   (I/O) The string where the arg/val pair is added. 
 * @param Arg   (I) The name of the arg to add.
 * @param Val   (I) The value corresponding to the arg to add.
 * @param EqStr (I) The string signifiying assignment.
 * @param Delim (I) The char to use as a delimeter between arguments.
 */

int MStringInsertArg(

  mstring_t  *Str,
  const char *Arg,
  const char *Val,
  const char *EqStr,
  const char *Delim)

  {
  char    argDelim[MMAX_NAME];

  if ((Str == NULL) ||
      (Arg == NULL) ||
      (Val == NULL) ||
      (Delim == NULL))
    {
    return(FAILURE);
    }

  /* form a search string */
  snprintf(argDelim,sizeof(argDelim),"%s%s",Arg,EqStr);

  /* Append the (optional) delimiter, Arg, EqStr and Value */
  if (strstr(Str->c_str(),argDelim) == NULL)
    {
    if (!Str->empty())   /* If string is currently NOT empty, append delimiter */
      *Str += Delim;
    *Str += Arg;
    *Str += EqStr;
    *Str += Val;
    }

  return(SUCCESS);
  }  /* END MStringInsertArg() */




/**
 * Walks through the source string and replaces each instance of a given match string 
 * encounctered with a replace string. 
 * The resultant string is built in the DstBuf and source string is not modified. 
 *  
 * NOTE - DstBuf must already be initialized
 * Note - FAILURE is returned if bad parameter input 
 *
 * @param SrcString (I) Source string - not modified 
 * @param MatchPattern (I) Substring which will be replaced in destination buffer.
 * @param ReplacePattern (I) Substring that replaces the matchpattern in the destination buffer.
 * @param DstBuf (I) Destination buffer where the modified string is copied.
 */

int MStringReplaceStr(

  const char *SrcString,
  const char *MatchPattern,
  const char *ReplacePattern, 
  mstring_t  *DstBuf)

  {
  int MatchPatternLen;
  const char *SrcMatchPtr;
  const char *SrcPtr;

  if ((SrcString == NULL) || (MatchPattern == NULL) || (ReplacePattern == NULL) || (DstBuf == NULL))
    {
    return(FAILURE);
    }

  MatchPatternLen = strlen(MatchPattern);

  *DstBuf = "";

  SrcPtr = SrcString;
  SrcMatchPtr = strstr(SrcPtr,MatchPattern);

  for (;;)
    {
    int SegmentLen;

    /* append portion of SrcString (starting at the beginning of the source string or after the last matched pattern)
     * up to but not including next matching pattern location */

    SegmentLen = (SrcMatchPtr == NULL) ? (int)strlen(SrcPtr) : (int)(SrcMatchPtr - SrcPtr);

    if ((SegmentLen > 0) && (SrcMatchPtr != NULL))
      {
      const char *runner = SrcPtr;

      /* move the substring to the destination */
      while(runner < SrcMatchPtr)
        {
        *DstBuf += *runner++;
        }
      }

    /* if no more pattern matches found, then we have just copied to the end of the source string and we are done. */

    if (SrcMatchPtr == NULL)
      break;

    /* append replacement pattern */

    *DstBuf += ReplacePattern;

    /* skip our source pointer over the match pattern */

    SrcPtr = SrcMatchPtr + MatchPatternLen;

    /* skip our source match pointer to the next instance of the match pattern in the source string */

    SrcMatchPtr = strstr(SrcPtr,MatchPattern);
    }  /* END for(;;) */

  return(SUCCESS);
  } /* END MStringReplaceStr */




/**
 * Pack characters into mstring_t object.
 *
 * (hide '#', ';', '\'', '\"', '<', '>', etc)  
 *
 * @see MUStringIsPacked()
 * @see MUStringUnpack()
 *
 * @param SString (I)
 * @param DString (O) [must be init'ed]
 */

int MUMStringPack(

  const char *SString,
  mstring_t  *DString)

  {
  const char *ptr;
  const char *delim = "\\;<>\"#";
  const char *FName = "MUMStringPack";

  MDB(5,fSTRUCT) MLog("%s(SString,DString)\n", FName);

  if ((SString == NULL) || (DString == NULL))
    {
    return(FAILURE);
    }

  /* determine if already packed */

  if (!strncmp(SString,"\\START",strlen("\\START")))
    {
    /* string already packed */

    *DString += SString;

    return(SUCCESS);
    }

  /* Add encoding prolog string */

  *DString += "\\START";

  for (ptr = SString;*ptr != '\0';ptr++)
    {
    /* check for character that MUST be encoded */

    if (isspace(*ptr) || strchr(delim,*ptr) || !isprint(*ptr))
      {
      static const char hexmap[] = "0123456789abcdef";
      char hexstring[4];

      /* Convert a byte to an encoded hex string of: '\\xx' */

      hexstring[0] = '\\';
      hexstring[1] = hexmap[(*ptr >> 4) & 0xF];
      hexstring[2] = hexmap[(*ptr) & 0xF];
      hexstring[3] = '\0';

      *DString += hexstring;   /* append to the destination string */
      }
    else
      {
      *DString += *ptr;       /* straight copy to destination w/o encoding */
      }
    }  /* END for (ptr) */

  return(SUCCESS);
  }  /* END MUMStringPack() */





/**
 * Pack characters to eliminate issues with both WIKI and XML.
 *
 * NOTE:  There is also an MUMStringPack()
 *
 * (hide '#', ';', '\'', '\"', '<', '>', etc)
 *
 * @see MUStringIsPacked()
 * @see MUStringUnpack()
 *
 * @param SString (I)
 * @param DString (O)
 * @param DStringLen (I)
 */

int MUStringPack(

  char *SString,     /* I */
  char *DString,     /* O */
  int   DStringLen)  /* I */

  {
  const char *FName = "MUStringPack";

  MDB(8,fSTRUCT) MLog("%s(SString,DString,%d)\n", FName, DStringLen);

#ifndef __MOPT
  if ((SString == NULL) || (DString == NULL))
    {
    return(FAILURE);
    }
#endif /* !__MOPT */

  DString[0] = '\0';

  /* determine if already packed */

  if (!strncmp(SString,"\\START",strlen("\\START")))
    {
    /* string already packed */

    if (SString != DString)
      MUStrCpy(DString,SString,DStringLen);

    return(SUCCESS);
    }

  mstring_t tmpString(MMAX_LINE);
  const char *SPtr = NULL;

  /* Determine if using passed in buffer or the local one */
  if (SString == DString)
    {
    tmpString = SString;

    SPtr = tmpString.c_str();
    }
  else
    {
    SPtr = SString;
    }

  /* Add the Prefix of "\\START" into the DString */
  strcpy(DString,"\\START");


  const char *ptr;
  const char *delim = "\\;<>\"#\a";
  int   next = strlen(DString);         /* determine next free char */
  int   endOfSpace = DStringLen - 4;    /* calc endOfSpace */

  for (ptr = SPtr;*ptr != '\0';ptr++)
    {
    if (next >= endOfSpace)
      {
      MDB(1,fSTRUCT) MLog("ALERT:    pack destination string too small (%d >= %d)\n", next, endOfSpace);

      return(FAILURE);
      }

    /* See if we need to pack or pass through a direct copy */
    if (isspace(*ptr) || strchr(delim,*ptr) || !isprint(*ptr))
      {
      DString[next++] = '\\';

      sprintf(&DString[next],"%02x",
        (int)*ptr);

      next += 2;
      }
    else
      {
      /* direct copy */
      DString[next++] = *ptr;
      }
    }  /* END for (ptr) */

  /* Terminate the DString */
  DString[next] = '\0';

  return(SUCCESS);
  }  /* END MUStringPack() */




/**
 * Determine if specified string is in packed format.
 *
 * @see MUStringPack()
 * @see MUStringUnpack()
 *
 * @param SString (I)
 */

mbool_t MUStringIsPacked(

  char *SString)  /* I */

  {
  if (SString == NULL)
    {
    return(FALSE);
    }

  if (!strncmp(SString,"\\START",strlen("\\START")))
    {
    /* string packed */

    return(TRUE);
    }

  return(FALSE);
  }  /* END MUStringIsPacked() */


/**
 * helper function: accept a valid 2 digit HEX string, convert to 8-bit char
 *
 * @param ptr   (I)   point to 2 HEX digits
 */

static inline char HexToChar(char *ptr)
  {
  char chr;

  /* convert first byte to UPPER nibble */
  if (*ptr >= 'a')
    chr = (*ptr - 'a' + 10) << 4;
  else
    chr = (*ptr - '0') << 4;

  ptr++;   /* bump to second byte */

  /* convert second byte to LOWER nibble */
  if (*ptr >= 'a')
    chr += (*ptr - 'a' + 10);
  else
    chr += (*ptr - '0');

  return chr;
  }



/**
 * Unpack 'packed' string.
 *
 * NOTE:  DString never needs to be longer that SString as the resulting string will always be
 *        of equal or lesser size (correct?)
 *
 * WARNING: Not threadsafe if DString is not specified!
 *
 * @see MUStringPack()
 *
 * @param SString (I) [source string]
 * @param DString (O) [destination string,optional,not threadsafe if not specified]
 * @param DStrLen (I)
 * @param SC (O) [optional]
 */

char *MUStringUnpack(

  char *SString,  /* I (source string) */
  char *DString,  /* O (destination string,optional,not threadsafe if not specified) */
  int   DStrLen,  /* I */
  int  *SC)       /* O (optional) */

  {
  unsigned int index;

  char *SPtr;

  char *dptr;
  unsigned int   dsize;

  char        tmpLine[MMAX_LINE << 2];  /* used for single in/out buffer */

  /* STATIC BUFFER to handle option 'DString' case */

  static char tmpString[MMAX_LINE];

  if (SC != NULL)
    *SC = SUCCESS;

  /* handle case where caller doesn't pass in a destination */
  if (DString != NULL)
    {
    dptr  = DString;
    dsize = DStrLen;
    }
  else
    {
    dptr  = tmpString;
    dsize = sizeof(tmpString);
    }

  if (SString == NULL)
    {
    /* FAILURE - source string not specified */

    if (SC != NULL)
      *SC = FAILURE;

    dptr[0] = '\0';

    return(dptr);
    }

  /* Check if src and destination are the same
   * If so, verify things will fit and if they do
   * use a local buffer (BAD).
   * If not the same, then use SString
   */
  if (SString == DString)
    {
    if (dsize > sizeof(tmpLine))
      {
      /* FAILURE - in/out buffer specified and temporary internal string is too small */

      if (SC != NULL)
        *SC = FAILURE;

      /* NOTE:  do not clear dptr as this will also clear source data */

      return(dptr);
      }

    /* Copy Src line to our local buffer */
    MUStrCpy(tmpLine,SString,sizeof(tmpLine));

    SPtr = tmpLine;
    }
  else
    {
    SPtr = SString;
    }

  dptr[0] = '\0';   /* Initialize the Dest */

  /* Check if the Input string is already packed 
   * by check first of string against "\\START"
   */
  if (strncmp(SPtr,"\\START",strlen("\\START")))
    {
    /* string not packed, copy src to dest buffer */

    MUStrCpy(dptr,SPtr,dsize);

    return(dptr);
    }

  /* Skip over the preamble */
  SPtr += strlen("\\START");
 
  for (index = 0;*SPtr != '\0';SPtr++)
    {
    if (SPtr[0] != '\\')
      {
      /* perform direct copy */

      dptr[index++] = *SPtr;

      if (index > dsize - 1)
        break;

      continue;
      }

    /* translate character to its encoded string and place into dest */

    /* SPtr -> "\\xx" */

    dptr[index++] = HexToChar(&SPtr[1]);

    SPtr += 2;

    if (index > dsize - 1)
      break;
    }  /* END for (SPtr) */

  /* terminate line */

  dptr[MIN(index,(dsize - 1))] = '\0';

  if (index >= dsize)
    {
    /* FAILURE - output string is truncated */

    if (SC != NULL)
      *SC = FAILURE;
    }

  return(dptr);
  }  /* END MUStringUnpack() */



/**
 * Unpack 'packed' string to mstring_t.
 *
 * NOTE:  Since DString is a dynamic string, it will grow if needed, but
 *        we are reducing the size of the string
 *
 * @see MUStringPack()
 *
 * @param SString (I) Source string
 * @param DString (O) Destination string
 */

int MUStringUnpackToMString(

  char      *SString,  /* I (source string) */
  mstring_t *DString)  /* O (destination string,optional,not threadsafe if not specified) */

  {
  if ((SString == NULL) || (DString == NULL))
    {
    /* FAILURE - source string not specified */

    return(FAILURE);
    }

  /* Check for 'packed' encoding:  "\\STARTxxxxxx...."  */

  if (strncmp(SString,"\\START",strlen("\\START")))
    {
    /* string not packed, so just assign to destination mstring_t */

    *DString = SString;

    return(SUCCESS);
    }

  /* Skip prolog encoding */
  SString += strlen("\\START");
 
  /* iterate til end of string */
  for (;*SString != '\0';SString++)
    {
    /* check for encoding char */

    if (SString[0] != '\\')
      {
      /* direct append to the mstring_t */

      *DString += *SString;

      continue;
      }

    /* We found an encoded character in hex digits, so 
     * translate the hex digits to a char and append to the mstring_t 
     *
     * ASSUMPTION:  that SString[1] and SString[2] are non-NULL bytes 
     *
     */
    *DString += HexToChar(&SString[1]);

    SString += 2;   /* skip hex bytes - "\\" is taken care of in for loop */
    }  /* END for (SString) */

  return(SUCCESS);
  }  /* END MUMStringUnpack() */

/* END MString.c */
