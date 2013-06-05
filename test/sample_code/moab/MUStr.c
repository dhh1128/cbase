/* HEADER */

/**
 * @file MUStr.c
 *
 * Contains Moab MUStrXXXX layer APIs
 */

#include "moab.h"
#include "moab-proto.h"
#include "moab-global.h"
#include "moab-const.h"


/**
 * Performs MUStrCpy if Src is not quoted. Otherwise copies Src
 * to Dst, ignoring the quotes on Src. Works for single or double quotes.
 *
 * @param Dst     (O)
 * @param Src     (I)
 * @param DstSize (I)
 */

int MUStrCpyQuoted(

  char       *Dst,
  char const *Src,
  int const DstSize)

  {
  int rc = SUCCESS;

  if ((Src == NULL) || (Dst == NULL))
    {
    return(FAILURE);
    }

  if (DstSize == 0)
    {
    return(rc);
    }

  if (Src[0] == '\'' || Src[0] == '"')
    {
    char const *End;
    int StrSize;

    for (End = Src + 1;End[0] != 0;End++) 
      {
      if (End[0] == Src[0])
        {
        Src++;

        break;
        }
      }

    StrSize = MIN(DstSize,End - Src);

    strncpy(Dst,Src,StrSize);

    if (StrSize == DstSize)
      {
      /* overflow detected */
      Dst[DstSize - 1] = 0;

      rc = FAILURE;
      }
    else
      {
      Dst[StrSize] = 0;
      }
    }
  else
    {
    rc = MUStrCpy(Dst,Src,DstSize);
    }

  return (rc);
  } /* END MUStrCpyQuoted() */


/**
 * Replaces ALL instances of a given char in a string with a new character.
 *
 * @see MUBufReplaceString() - peer
 *
 * @param String (I) String that should be modified by replacing the characters. [modified]
 * @param SChar (I) The character to replace or '\0' for all whitespace.
 * @param DChar (I) The character that will replace instances of SChar.
 * @param IgnBackSlash (I) Do not replace backslashed SChars.
 */

int MUStrReplaceChar(

  char    *String,       /* I (modified) */
  char     SChar,        /* I (or '\0' for all whitespace) */
  char     DChar,        /* I */
  mbool_t  IgnBackSlash) /* I (do not replace backslashed SChars) */

  {
  /* NOTE: only replaces chars! */

  int cindex;
  int len;

  if (String == NULL)
    {
    return(FAILURE);
    }

  len = strlen(String);

  if (IgnBackSlash == TRUE)
    {
    for (cindex = 0;cindex < len;cindex++)
      {
      if (String[cindex] == SChar)
        {
        switch (cindex)
          {
          case 0:

            String[cindex] = DChar;

            break;

          case 1:

            if (String[cindex - 1] != '\\') 
              String[cindex] = DChar;

            break;

          default:

            if ((String[cindex - 1] != '\\') || (String[cindex - 2] == '\\'))
              String[cindex] = DChar;

            break;
          }  /* END switch (cindex) */
        }    /* END if (String[cindex] == SChar) */
      }      /* END for (cindex) */
    }        /* END if (IgnBackSlash == TRUE) */
  else if (SChar == '\0')
    {
    for (cindex = 0;cindex < len;cindex++)
      {
      if (isspace(String[cindex]))
        String[cindex] = DChar;
      }  /* END for (cindex) */
    }
  else
    {
    for (cindex = 0;cindex < len;cindex++)
      {
      if (String[cindex] == SChar)
        String[cindex] = DChar;
      }  /* END for (cindex) */
    }    /* END else (IgnBackSlash == TRUE) */

  return(SUCCESS);
  }  /* END MUStrReplaceChar() */




/**
 * Walks through the source string and replaces each instance of a given match string encounctered with a replace string. 
 * The resultant string is built in the DstBuf and source string is not modified. 
 * 
 * NOTE:  There is also an MStringReplaceStr()
 * 
 * Note - FAILURE is returned if bad parameter input or destination buffer would 
 * overflow due to the string substitution. 
 *
 * @param SrcString (I) Source string - not modified 
 * @param MatchPattern (I) Substring which will be replaced in destination buffer.
 * @param ReplacePattern (I) Substring that replaces the matchpattern in the destination buffer.
 * @param DstBuf (I) Destination buffer where the modified string is copied.
 * @param MaxLen (I) Size of the destination buffer 
 */

int MUStrReplaceStr(

  char       *SrcString,
  const char *MatchPattern,
  const char *ReplacePattern, 
  char       *DstBuf,
  int         MaxLen)

  {
  int MatchPatternLen;
  int ReplacePatternLen;
  char *SrcMatchPtr;
  char *SrcPtr;

  if ((SrcString == NULL) || (MatchPattern == NULL) || (ReplacePattern == NULL) || (DstBuf == NULL) || (MaxLen <= 1))
    {
    return(FAILURE);
    }

  MatchPatternLen = strlen(MatchPattern);
  ReplacePatternLen = strlen(ReplacePattern);

  DstBuf[0] = '\0';

  SrcPtr = SrcString;
  SrcMatchPtr = strstr(SrcPtr,MatchPattern);

  for (;;)
    {
    int SegmentLen;

    /* append portion of SrcString (starting at the beginning of the source string or after the last matched pattern)
     * up to but not including next matching pattern location */

    SegmentLen = (SrcMatchPtr == NULL) ? (int)strlen(SrcPtr) : (int)(SrcMatchPtr - SrcPtr);

    /* make sure we don't overflow our destination buffer */

    if (((int)strlen(DstBuf) + SegmentLen) > (MaxLen - 1))
      {
      return(FAILURE);
      }

    if (SegmentLen > 0)
      strncat(DstBuf,SrcPtr,SegmentLen);

    /* if no more pattern matches found, then we have just copied to the end of the source string and we are done. */

    if (SrcMatchPtr == NULL)
      break;

    /* make sure that adding the replacement pattern doesn't overflow the destination buffer */

    if (((int)strlen(DstBuf) + ReplacePatternLen) > (MaxLen - 1))
      {
      return(FAILURE);
      }

    /* append replacement pattern */

    strcat(DstBuf,ReplacePattern);

    /* skip our source pointer over the match pattern */

    SrcPtr = SrcMatchPtr + MatchPatternLen;

    /* skip our source match pointer to the next instance of the match pattern in the source string */

    SrcMatchPtr = strstr(SrcPtr,MatchPattern);
    }  /* END for(;;) */

  return(SUCCESS);
  } /* END MUStrReplaceStr */



/**
 * Perform buffer-safe string copy.
 *
 * NOTE:  will report failure if Dst buffer is too small--as much as possible 
 * is copied.
 *
 * @param Dst (O)
 * @param Src (I)
 * @param Length of Dst (I) [-1 == MMAX_NAME]
 */

int MUStrCpy(

  char       *Dst,
  char const *Src,
  int         Length)

  {
  int index;

  if (Dst != NULL)
    Dst[0] = '\0';

  if ((Dst == NULL) || (Src == NULL) || (Length == 0))
    {
    return(FAILURE);
    }

  if (Length == -1)
    Length = MMAX_NAME;

  for (index = 0;index < Length;index++)
    {
    Dst[index] = Src[index];

    if (Src[index] == '\0')
      {
      return(SUCCESS);
      }
    }  /* END for (index) */

  /* src string is too large */

  Dst[Length - 1] = '\0';

  return(FAILURE);
  }  /* END MUStrCpy() */




/**
 * Removes all leading whitespace, where whitespace is defined using the
 * @isspace() function. 
 *
 * @param String (I/O)
 * @return the number of characters removed.
 * @return -1 if error
 *
 * DO NOT PASS IN CHARACTER CONSTANTS - IT WILL SEGV FAULT
 */

int MUStrLTrim(

  char *String)

  {
  int index;

  if (String == NULL)
    {
    return(-1);
    }

  /* Skip over all whitespace characters */
  for (index = 0;isspace(String[index]);index++) {}

  /* Move remaining part of string (including NULL byte) to the beginning of the buffer */
  memmove(String,String + index,strlen(String) + 1);

  return(index);
  } /* END MUSTrLTrim() */



/**
 * Removes all trailing whitespace from String, where whitespace is defined
 * using the @isspace() function.
 *
 * @param String (I/O)
 * @return the number of whitespace characters removed.
 * @return -1 if error
 */

int MUStrRTrim(

    char *String)

  {
  int numChars;
  char *EndPtr;

  if (String == NULL)
    {
    return(-1);
    }

  numChars = strlen(String);
  EndPtr = String + numChars - 1;

  while ((EndPtr >= String) && isspace(*EndPtr))
    {
    *EndPtr-- = 0;
    }

  return(numChars - (MAX(EndPtr - String, 0)));
  } /*END MUStrRTrim */





/**
 * Concatenates src string onto dest string.
 *
 * NOTE:  will report failure if Dst buffer is too small--as much as possible
 * is concatenated.
 *
 * @param Dst (I) [modified]
 * @param Src (I)
 * @param DstSize (I)
 */

int MUStrCat(

  char       *Dst,
  const char *Src,
  int         DstSize)

  {
  int index;
  int DEnd;

  if ((Dst == NULL) || (DstSize <= 0))
    {
    return(FAILURE);
    }

  if ((Src == NULL) || (Src[0] == '\0'))
    {
    return(SUCCESS);
    }

  DEnd = MIN((int)strlen(Dst),DstSize);

  for (index = 0;index < DstSize - DEnd;index++)
    {
    if (Src[index] == '\0')
      break;

    Dst[DEnd + index] = Src[index];
    }  /* END for (index) */

  if (DEnd + index < DstSize)
    {
    /* Null terminate at end of cat'ed string. */

    Dst[DEnd + index] = '\0';
    return(SUCCESS);
    }
  else
    {
    /* Null terminate at end of Dst (not all of Src was cat'ed) */

    Dst[DstSize -1] = '\0';
    return(FAILURE);
    }
  }  /* END MUStrCat() */




/**
 * Tokenize a string as with MUStrTok. However, instead of modifying the
 * string, return the length of each token, leaving the original string
 * unmodified.
 *
 * @param String     (I) [optional], the initial buffer to tokenize
 * @param DelimChars (I) the characteres to tokenize on
 * @param ResultSize (O) [required], will store the length of the next token,
 *   or 0 if no token is found.
 * @param Context    (I) [required] Stores the current tokenizing position in between calls to
 *   MUStrScan() on the same string.
 *
 * @return a pointer to the start of the next token, or NULL if no more tokens
 *   are found
 */

char *MUStrScan(

  char const  *String,     /* I */
  char const  *DelimChars, /* I */
  int         *ResultSize, /* O */
  char const **Context)    /* O */

  {
  char const *CurLoc;
  char const *Start;

  if (String != NULL)
    {
    Start = String;
    }
  else
    {
    Start = *Context;
    }

  while ((*Start != 0) && (strchr(DelimChars,*Start) != NULL))
    Start++;

  CurLoc = Start;

  while ((*CurLoc != 0) && (strchr(DelimChars,*CurLoc) == NULL))
    CurLoc++;

  *Context = CurLoc;
  *ResultSize = CurLoc - Start;

  if (CurLoc == Start)
    {
    return(NULL);
    }

  return((char *)Start);
  }  /* END MUStrScan */





/**
 * Tokenizes the string according to delimiters.
 * This function operates like strtok() or strtok_r().
 *
 * If delimiter characters exists in the string that shouldn't be tokenized, use
 * MUStrTokEPlus. MUStrTokEPlus will ignore any delimiters that are quoted with "'s, 
 * along with the quotes.
 *
 * (It changes all characters at the beginning of (*Ptr) that 
 * are found in DList to 0, up to but not including the first character in (*Ptr)
 * not found in DList. It then searches until it finds a null-terminator or
 * another  character found in DList, which it sets to 0 as well.
 * Finally, it returns a pointer to the first character found that was not in DList.
 * If Ptr is NULL, Line is NULL, or the function would ever return a pointer to
 * a zero-length string, it instead returns NULL.)
 * 
 * @see MUStrTokEPlus() - peer
 *
 * @param Line (I) [optional]
 * @param DList (I)
 * @param Ptr (O) point to termination character of returned token
 */

char *MUStrTok(

  char       *Line,  /* I (optional) */
  char const *DList, /* I */
  char      **Ptr)   /* O point to termination character of returned token */

  {
  char *Head = NULL;

  int dindex;

  mbool_t ignchar;

  /* NOTE:  no handling if Line is NULL and Ptr is uninitialized */

  if (Ptr == NULL)
    {
    /* tokPtr not specified */

    return(NULL);
    }

  if (Line != NULL)
    {
    *Ptr = Line;

    if (Line[0] == '\0')
      {
      /* line is empty */

      *Ptr = NULL;

      return(NULL);
      }
    }
  else if (*Ptr == NULL)
    {
    return(NULL);
    }

  ignchar = FALSE;

  Head = NULL;

  while (**Ptr != '\0')
    {
    for (dindex = 0;DList[dindex] != '\0';dindex++)
      {
      if (**Ptr == DList[dindex])
        {
        **Ptr = '\0';

        (*Ptr)++;

        if (Head != NULL)
          {
          return(Head);
          }
        else
          {
          ignchar = TRUE;

          break;
          }
        }
      }    /* END for (dindex) */

    if ((ignchar != TRUE) && (**Ptr != '\0'))
      {
      if (Head == NULL)
        Head = *Ptr;

      (*Ptr)++;
      }

    ignchar = FALSE;
    }  /* END while (**Ptr != '\0') */

  return(Head);
  }  /* END MUStrTok() */


/**
 *
 * NOTE:  only locate next delimiter which is not backslashed and not quoted
 *        within double quotes (") 
 *
 * @param Head (I)
 * @param Delim (I)
 */

char *MUStrChr(

  const char *Head,   /* I */
  const char  Delim)  /* I */

  {
  const char *ptr;

  mbool_t EscapeMode;
  mbool_t QuoteMode;

  EscapeMode = FALSE;
  QuoteMode  = FALSE;

  for (ptr = Head;*ptr != '\0';ptr++)
    {
    if (*ptr == '\\')
      {
      EscapeMode = TRUE;

      continue;
      }

    if ((*ptr == '\"') && (EscapeMode == FALSE))
      QuoteMode = 1 - QuoteMode;
      
    if ((*ptr == Delim) && (EscapeMode != TRUE) && (QuoteMode != TRUE))
      {
      return((char *)ptr);
      }

    EscapeMode = FALSE;
    }

  return(NULL);
  }  /* END MUStrChr() */



/**
 * Make the given string lower-case (in-place substitution).
 *
 * @param String (I) [modified]
 */

int MUStrToLower(

  char *String) /* I (modified) */

  {
  int sindex;

  if (String == NULL)
    {
    return(SUCCESS);
    }

  for (sindex = 0;String[sindex] != '\0';sindex++)
    {
    String[sindex] = tolower(String[sindex]);
    }  /* END for (sindex) */

  return(SUCCESS);
  }  /* END MUStrToLower() */



/**
 * Makes all characters in given string uppercase.
 *
 * @param String (I) [modified]
 * @param OBuf (O)
 * @param BufSize (I)
 */

int MUStrToUpper(

  const char *String,
  char       *OBuf,
  int         BufSize)

  {
  int sindex;
  char *ptr;

  if ((String == NULL) || (OBuf == NULL))
    {
    return(SUCCESS);
    }

  ptr = OBuf;

  if (BufSize > 0)
    {
    BufSize--;

    for (sindex = 0;String[sindex] != '\0';sindex++)
      {
      /* We're going out to BufSize - 1 (decremented above for speed) */
      if (sindex >= (BufSize))
        break;

      ptr[sindex] = toupper(String[sindex]);
      }  /* END for (sindex) */
    }
  else
    {
    for (sindex = 0;String[sindex] != '\0';sindex++)
      {
      ptr[sindex] = toupper(String[sindex]);
      }  /* END for (sindex) */
    }

  ptr[sindex] = '\0';

  return(SUCCESS);
  }  /* END MUStrToUpper() */



/**
 * Removes a substring/element (String) from a string representing a list, where each element of the
 * list is separated by a delimeter (Delim). The substring must be
 * found in the list as a whole element to be considered a match.
 * If the substring is not found, the function returns FAILURE.
 *
 * @param Src (I) The list/string to remove the substring from. [modified]
 * @param String (I) The substring/element to remove from the Src string.
 * @param Delim (I) The delimeter used in the list to separate elements.
 * @param IsPair (I) True if the given substring is the attr in an attr=value or attr:value pair
 * @return FAILURE if the substring is not found in the original string, SUCCESS otherwise.
 */

int MUStrRemoveFromList(

  char       *Src,
  const char *String,
  char        Delim,
  mbool_t     IsPair)

  {
  char *ptr;
  int   len;
  int   slen;
  int   cindex;

  mbool_t MatchFound;

  char *Head;

  if ((Src == NULL) || (Src[0] == '\0'))
    {
    return(SUCCESS);
    }
  
  if ((String == NULL) || (String[0] == '\0'))
    {
    return(SUCCESS);
    }

  if (Delim == '\0')
    {
    return(SUCCESS);
    }

  Head = Src;

  MatchFound = FALSE;

  while ((ptr = strstr(Head,String)) != NULL)
    {
    mbool_t PairUsed = FALSE;
    slen = strlen(String);

    if ((ptr[slen] != Delim) && (ptr[slen] != '\0'))
      {
      /* invalid match (post-char mismatch) */

      /* check if it is an attr=value or attr:value pair */

      if ((IsPair == TRUE) && ((ptr[slen] == '=') || (ptr[slen] == ':')))
        {
        PairUsed = TRUE;
        }
      else
        {
        Head = ptr + 1;

        continue;
        }
      }

    if (ptr > Src)
      {
      if (ptr[-1] != Delim)
        {
        /* invalid match (pre-char mismatch) */

        Head = ptr + 1;

        continue;
        }

      /* remove pre-match delimiter */

      ptr--;
      slen++;

      if (PairUsed == TRUE)
        {
        char *tmpEnd;

        tmpEnd = strchr(ptr + slen,Delim);
        if (tmpEnd != NULL)
          {
          slen += (tmpEnd - (ptr + slen));
          }
        else
          {
          /* Remove the rest of the string */

          slen += strlen(ptr + slen);
          }
        }
      } /* END if (ptr > Src) */
    else
      {
      /* at start of string */

      if (PairUsed == TRUE)
        {
        char *tmpEnd;

        tmpEnd = strchr(ptr + slen,Delim);
        if (tmpEnd != NULL)
          {
          slen += (tmpEnd - (ptr + slen));
          }
        else
          {
          /* Remove the rest of the string */

          slen += strlen(ptr + slen);
          }
        }

      if (ptr[slen] == Delim)
        {
        /* remove terminating delim */

        slen++;
        }
      } /* END else (ptr == Src) */

    MatchFound = TRUE;

    len = strlen(ptr);

    for (cindex = 0;cindex <= len;cindex++)
      {
      if (ptr[cindex + slen] == '\0')
        {
        ptr[cindex] = '\0';

        break;
        }

      ptr[cindex] = ptr[cindex + slen];
      }
    }    /* END while ((ptr = strstr(Head,String)) != NULL) */

  if (MatchFound == FALSE)
    return(FAILURE);
 
  return(SUCCESS);
  }  /* END MUStrRemoveFromList() */


/**
 * Removes a substring from the end of a given source string.
 * Note that this IS case-sensitive.
 *
 * @param Src (I) String to modify. [modified]
 * @param SubStr (I) Substring that should be removed from end of Src string.
 * @return SUCCESS if the substring was found and removed, FAILURE otherwise.
 */
int MUStrRemoveFromEnd(

  char       *Src,
  const char *SubStr)

  {
  int SrcIndex;
  int SubStrIndex;
  int SrcLen;
  int SubStrLen;

  if ((Src == NULL) ||
      (SubStr == NULL))
    {
    return(FAILURE);
    }

  if ((Src[0] == '\0') ||
      (SubStr[0] == '\0'))
    {
    return(FAILURE);
    }

  SrcLen = strlen(Src);
  SubStrLen = strlen(SubStr);

  SubStrIndex = SubStrLen-1;

  for (SrcIndex = SrcLen-1;SrcIndex >= 0;SrcIndex--)
    {
    if (Src[SrcIndex] != SubStr[SubStrIndex])
      {
      /* substring no longer matches end of string--no match found */

      break;
      }

    if (SubStrIndex == 0)
      {
      /* match found--remove it from string */

      Src[SrcIndex] = '\0';

      return(SUCCESS);
      }

    SubStrIndex--;

    if (SubStrIndex < 0)  /* don't go too far or we'll read bad memory! */
      break;
    }

  return(FAILURE);
  }  /* END MUStrRemoveFromEnd() */



/**
 * Splits the string at the delimiters and copies to an array.
 *
 * Mimics the python split function.
 *
 * @param InputString (I) String to split.
 * @param Delimiters (I) Delimiters.
 * @param Limit (I) Max number of splits to be made. -1 for unlimited. 
 * @param TokenArray (O)
 * @param TokenArraySize (I)
 * @return Returns the number tokens in the returned array. Either Limit + 1 tokens or TokenArraySize - 1.
 */

int MUStrSplit (

  char       *InputString, 
  const char *Delimiters, 
  int         Limit,
  char      **TokenArray,
  int         TokenArraySize)

  {
  char *head;
  char *curPtr;
  int   tIndex = 0;

  if (InputString == NULL)
    return(0);

  head = curPtr = InputString;

  while (*curPtr != '\0')
    {
    if (strchr(Delimiters,*curPtr) != NULL) /* found delimiter */
      {
      TokenArray[tIndex] = (char *)calloc(sizeof(char) * ((curPtr - head) + 1),sizeof(char));
      strncpy(TokenArray[tIndex++],head,curPtr - head); /* copy str upto delim */
      TokenArray[tIndex] = NULL;

      /* Copy remainder of string in to last non-null slot if token limit has
       * been reached or if TokenArraySize is reached. */

      if ((tIndex == TokenArraySize - 2) || (tIndex == Limit))
        {
        curPtr++;
        TokenArray[tIndex++] = strdup(curPtr);
        TokenArray[tIndex] = NULL;

        return(tIndex);
        }

      head = ++curPtr;

      continue;
      }

    /* string is done being split, return. */

    if (*curPtr == '\0')
      {
      /* catch last trailing delimiter - ex. "," */

      if (strchr(Delimiters,*curPtr - 1) != NULL)
        TokenArray[tIndex++] = (char *)calloc(sizeof(char) * 1,sizeof(char));

      break;
      }

    curPtr++;
    }

  /* Copy rest of string into last token slot and NULL terminate. */
        
  TokenArray[tIndex++] = strdup(head);
  TokenArray[tIndex] = NULL;

  return(tIndex);
  }


/**
 * Dynamically allocate memory for arbitrary string with varargs formatting.
 *
 * SEAN:  Please comment on function purpose
 *
 * @param Format (I)
 *
 * @return on SUCCESS - dynamically allocated and populated string 
 * @return on FAILURE - NULL
 */

char *MUStrFormat(

  const char *Format,
  ...)

  {
  char *Result = NULL;
  char DummyString[1];
  int StringLength;
  va_list Args;

  va_start(Args,Format);
  StringLength = vsnprintf(DummyString,0,Format,Args);
  va_end(Args);

  if (StringLength < 0)
    {
    return(NULL);
    }

  Result = (char *)MUMalloc(sizeof(char) * (StringLength + 1));

  if (Result == NULL)
    {
    return(NULL);
    }

  va_start(Args,Format);
  StringLength = vsnprintf(Result,StringLength + 1,Format,Args);
  va_end(Args);

  if (StringLength < 0)
    {
    MUFree(&Result);

    return(NULL);
    }

  return(Result);
  }  /* END MUStrFormat() */


/**
 * Tokenizes a string ignoring quoted delimiters.
 *
 * Ignore quoted delimiters and the quotes surrounding them
 * PtrP points to the next character after the token currently being processed
 *
 * WARNING: this routine will turn this:  '"hello world"' into 'hello world"'
 *          for better results use MUStrTokEPlus() 
 *
 * @see MUStrTok() - peer
 * @see MUStrTokEPlus() - peer
 *
 * @param  Line  (I)
 * @param  DList (I)
 * @param  PtrP  (I/O)   [iterator pointer]
 */

char *MUStrTokE(
 
  char       *Line,
  const char *DList,
  char      **PtrP)
 
  {
  /* provide strtok with quoting support */

  char *Head = NULL;

  int dindex;
  int ignchar;

  int SQCount = 0;  /* single quote count */
  int DQCount = 0;  /* double quote count */

  char *ptr;

  /* Ensure good parameters */
  if ((NULL == PtrP) || (NULL == DList))
    {
    return(NULL);
    }

  if (Line != NULL)
    {
    *PtrP = Line;
    }

  ignchar = FALSE;

  ptr = *PtrP;

  while (*ptr != '\0')
    {
    /* locate SQ/DQ */

    if (*ptr == '\'')
      {
      SQCount++;

      if ((Head != NULL) && !(SQCount % 2) && !(DQCount % 2))
        {
        /*
        *PtrP = ptr;

        return(Head);
        */
        }
      else
        {
        ptr++;

        ignchar = TRUE;
        }
      }
    else if (*ptr == '\"')
      {
      DQCount++;

      if ((Head != NULL) && !(SQCount % 2) && !(DQCount % 2))
        {
        /*
        *PtrP = ptr;

        return(Head);
        */
        }
      else
        {
        ptr++;

        ignchar = TRUE;
        }
      }
    else if (!(SQCount % 2) && !(DQCount % 2))
      {
      /* locate delimiter */

      for (dindex = 0;DList[dindex] != '\0';dindex++)
        {
        if (*ptr == DList[dindex])
          {
          /* NULL terminate the token where the delimiter was */
          *ptr = '\0';

          ptr++;

          if (Head != NULL)
            {
            *PtrP = ptr;

            return(Head);
            }

          ignchar = TRUE;

          break;
          }
        }    /* END for (dindex) */
      }

    if ((ignchar != TRUE) && (*ptr != '\0'))
      {
      if (Head == NULL)
        Head = ptr;

      ptr++;
      }

    ignchar = FALSE;
    }  /* END while (*ptr != '\0') */

  *PtrP = ptr;

  return(Head);
  }  /* END MUStrTokE() */



/**
 *  Backend function for MUStrTokEPlus derived from the
 *  original MUStrTokEPlus.  That is, I've modified and 
 *  wrapped the original function so that stripping 
 *  quotes is optional. 
 *
 * @param Line  (I) char * of string to be tokenized (modified)
 * @param DList (I) list of delimiters to tokenize around
 * @param PtrP  (O) pointer to use on subsequent calls 
 * @param StripQuotes (I) boolean indicating whether we would like to strip the quotes
 * @return Returns a pointer to the beginning of the tokenized string or NULL if not found.
 */

char* MUStrTokEPlusDoer(
 
  char       *Line,
  const char *DList,
  char      **PtrP,
  mbool_t     StripQuotes)
 
  {
  /* provide strtok with quoting support */

  char *Head = NULL;
 
  int dindex;
  
  mbool_t ignchar;

  int SQCount = 0;  /* single quote count */
  int DQCount = 0;  /* double quote count */

  /* NOTE: the temporary location is needed in order to properly handle cases
           like the following:  MUStrTok("hello world"this is the string);
           where we want to ignore the embedded '"' and shift things over. */

  char *tmpLine = NULL;
  int   tmpLineSize;
  int   tindex;

  char *ptr;
 
  if (PtrP == NULL)
    {
    /* FAILURE */

    return(Head);
    }
  if (Line != NULL)
    {
    *PtrP = Line;
    }

  if (StripQuotes)
    {
    tmpLineSize = (Line == NULL) ? strlen(*PtrP) + 1 : strlen(Line) + 1;

    tmpLine = (char *)MUMalloc(tmpLineSize);

    tmpLine[0] = '\0';
    }

  tindex = 0;

  ignchar = FALSE;

  ptr = *PtrP;

  while (*ptr != '\0')
    {
    /* locate SQ/DQ */

    /* If the current character is a single or double quote then make sure it is not escaped.
       It is not escaped if
          (a) it is the first character in the string (can't have a preceeding backslash since it is first)
          or
          (b) it is not the first character and the previous character is not a backslash
    */

    if ((*ptr == '\'') && 
        ((ptr == *PtrP) || (*(ptr-1) != '\\')))
      {
      SQCount++;

      ptr++;

      ignchar = TRUE;
      }
    else if ((*ptr == '\"') &&
             ((ptr == *PtrP) || (*(ptr-1) != '\\')))
      {
      DQCount++;
 
      ptr++;

      ignchar = TRUE;
      }
    else if (!(SQCount % 2) && !(DQCount % 2))
      {
      /* locate delimiter */    

      for (dindex = 0;DList[dindex] != '\0';dindex++)
        {
        if (*ptr == DList[dindex])
          {
          *ptr = '\0';
 
          ptr++;
 
          if (Head != NULL)
            {
            *PtrP = ptr;

            if (StripQuotes) 
              {
              tmpLine[tindex] = '\0';

              if (tindex > 0)
                strcpy(Head,tmpLine);
  
               MUFree(&tmpLine);
              }

            return(Head);
            }

          ignchar = TRUE;

          break;
          }
        }    /* END for (dindex) */
      }
 
    if ((ignchar != TRUE) && (*ptr != '\0'))
      {
      if (Head == NULL)
        Head = ptr;
 
      if (StripQuotes)
        tmpLine[tindex++] = ptr[0];

      ptr++;
      }
 
    ignchar = FALSE;
    }  /* END while (*ptr != '\0') */

  if (StripQuotes)
    tmpLine[tindex] = '\0';

  if (StripQuotes) 
    {
    if (tindex > 0)
      strcpy(Head,tmpLine);

    MUFree(&tmpLine);
    }

  *PtrP = ptr;
 
  return(Head);
  }  /* END MUStrTokEPlusDoer() */


/**
 * Ignore quoted delimiters and the quotes surrounding them.
 * Usage is the same as the standard MUStrTok()/strtok() routine.
 * 
 * This routine should replace MUStrTokE() as it is more correct
 *
 * @param Line  (I) char * of string to be tokenized (modified)
 * @param DList (I) list of delimiters to tokenize around
 * @param PtrP  (O) pointer to use on subsequent calls 
 * @return Returns a pointer to the beginning of the tokenized string or NULL if not found.
 */


char *MUStrTokEPlus(
 
  char        *Line,
  const char  *DList,
  char       **PtrP)
 
  {
  char* Head = NULL;

  Head = MUStrTokEPlusDoer(Line, DList, PtrP, TRUE); 

  return Head;
  }  /* END MUStrTokEPlus() */


/**
 * Same as above but doesn't strip the quotes off strings
 */


char *MUStrTokEPlusLeaveQuotes(

  char        *Line,
  const char  *DList,
  char       **PtrP)
 
  {
  char* Head = NULL;

  Head = MUStrTokEPlusDoer(Line, DList, PtrP, FALSE); 

  return Head;
  }  /* END MUStrTokEPlusLeaveQuotes() */


/**
 * Ignore single quote, double quote, and square bracket delimited 
 * delimiters.  Also ignore backslash characters if IgnBackslash is set.
 *
 * @param Line         (I)
 * @param DList        (I)
 * @param PtrP         (I/O)
 * @param IgnBackSlash (I)
 */

char *MUStrTokEArrayPlus(

  char       *Line,
  const char *DList,
  char      **PtrP,
  mbool_t     IgnBackSlash)

  {
  /* provide strtok with quoting/backslash support */

  char *Head  = NULL;
  char *Start = NULL;

  int     dindex;
  mbool_t ignchar;  /* delimiter found */
  int SQCount = 0;  /* single quote count */
  int DQCount = 0;  /* double quote count */
  int SBCount = 0;  /* square bracket count */

  mbool_t Ignore;

  char *ptr;

  /* NOTE: the temporary location is needed in order to properly handle cases
           like the following:  MUStrTok("hello world"this is the string);
           where we want to ignore the embedded '"' and shift things over. */

  char *tmpLine = NULL;
  int   tmpLineSize;
  int   tindex;

  if (NULL == DList)
    return(NULL);

  if (Line != NULL)
    {
    *PtrP = Line;
    }
  else if (PtrP == NULL)
    {
    /* FAILURE */

    return(Head);
    }

  Start = *PtrP;

  if (tmpLine == NULL)
    {
    tmpLineSize = (Line == NULL) ? strlen(*PtrP) + 1 : strlen(Line) + 1;

    tmpLine = (char *)MUMalloc(tmpLineSize);
    }

  tmpLine[0] = '\0';

  tindex = 0;

  ignchar = FALSE;

  ptr = *PtrP;

  while (*ptr != '\0')
    {
    /* locate SQ/DQ/SB */

    if (*ptr == '\'')
      {
      SQCount++;

      if ((Head != NULL) && !(SQCount % 2) && !(DQCount % 2))
        {
        ptr++;

        ignchar = TRUE;
        }
      else
        {
        Ignore = TRUE;

        if (IgnBackSlash == TRUE)
          {
          /* check if backslash precedes delimiter */

          if ((ptr > Start) && (*(ptr - 1) == '\\'))
            {
            /* check if backslash is backslashed */

            if ((ptr > Start + 1) && (*(ptr - 2) != '\\'))
              {
              /* delimiter is backslahsed, ignore */

              Ignore = FALSE;

              SQCount--;
              }
            }
          }    /* END if ((IgnBackSlash == TRUE) && (Head != NULL)) */

        if (Ignore == TRUE)
          {
          ptr++;

          ignchar = TRUE;
          }
        }
      }
    else if (*ptr == '\"')
      {
      DQCount++;

      if ((Head != NULL) && !(SQCount % 2) && !(DQCount % 2))
        {
        ptr++;

        ignchar = TRUE;
        }
      else
        {
        Ignore = TRUE;

        if (IgnBackSlash == TRUE)
          {
          /* check if backslash precedes delimiter */

          if ((ptr > Start) && (*(ptr - 1) == '\\'))
            {
            /* check if backslash is backslashed */

            if ((ptr > Start + 1) && (*(ptr - 2) != '\\'))
              {
              /* delimiter is backslahsed, ignore */

              Ignore = FALSE;

              DQCount--;
              }
            }
          }    /* END if ((IgnBackSlash == TRUE) && (Head != NULL)) */

        if (Ignore == TRUE)
          {
          ptr++;

          ignchar = TRUE;
          }
        }
      }
    else if (*ptr == '[')
      {
      SBCount = 1;
      }
    else if (*ptr == ']')
      {
      SBCount = 0;
      }
    else if (!(SQCount % 2) && !(DQCount % 2) && (SBCount == 0))
      {
      /* not inside quoted space, check if character is delimiter */

      for (dindex = 0;DList[dindex] != '\0';dindex++)
        {
        if (*ptr != DList[dindex])
          continue;

        if ((IgnBackSlash == TRUE) && (Head != NULL))
          {
          /* check if backslash precedes delimiter */

          if ((ptr > Head) && (*(ptr - 1) == '\\'))
            {
            /* check if backslash is backslashed */

            if ((ptr > Head + 1) && (*(ptr - 2) != '\\'))
              {
              /* delimiter is backslahsed, ignore */

              continue;
              }
            }
          }    /* END if ((IgnBackSlash == TRUE) && (Head != NULL)) */

        /* delimiter found */

        *ptr = '\0';

        ptr++;

        if (Head != NULL)
          {
          *PtrP = ptr;

          tmpLine[tindex] = '\0';

          if (tindex > 0)
            strcpy(Head,tmpLine);

          MUFree(&tmpLine);

          return(Head);
          }

        ignchar = TRUE;

        break;
        }    /* END for (dindex) */
      }

    if ((ignchar != TRUE) && (*ptr != '\0'))
      {
      if (Head == NULL)
        Head = ptr;

      tmpLine[tindex++] = ptr[0];

      ptr++;
      }

    ignchar = FALSE;
    }  /* END while (*ptr != '\0') */

  tmpLine[tindex] = '\0';

  if (tindex > 0)
    strcpy(Head,tmpLine);

  *PtrP = ptr;

  MUFree(&tmpLine);

  return(Head);
  }  /* END MUStrTokEArrayPlus() */





/**
 * Ignore single quote, double quote, and square bracket delimited 
 * delimiters.  Also ignore backslash characters if IgnBackslash is set
 *
 * WARNING: this routine will turn: '"hello world"' into 'hello world"'
 *          for more correct results use MUStrTokEArrayPlus()
 *
 * @param Line         (I)
 * @param DList        (I)
 * @param PtrP         (I/O)
 * @param IgnBackSlash (I)
 */


char *MUStrTokEArray(

  char        *Line,
  const char  *DList,
  char       **PtrP,
  mbool_t      IgnBackSlash)

  {
  /* provide strtok with quoting/backslash support */

  char *Head = NULL;

  int     dindex;
  mbool_t ignchar;  /* delimiter found */
  int SQCount = 0;  /* single quote count */
  int DQCount = 0;  /* double quote count */
  int SBCount = 0;  /* square bracket count */

  char *ptr;

  if (NULL == DList)
    return(NULL);

  if (Line != NULL)
    {
    *PtrP = Line;
    }
  else if (PtrP == NULL)
    {
    /* FAILURE */

    return(Head);
    }

  ignchar = FALSE;

  ptr = *PtrP;

  while (*ptr != '\0')
    {
    /* locate SQ/DQ/SB */

    if (*ptr == '\'')
      {
      SQCount++;

      if ((Head != NULL) && !(SQCount % 2) && !(DQCount % 2))
        {
        /*
        *PtrP = ptr;

        return(Head);
        */
        }
      else
        {
        ptr++;

        ignchar = TRUE;
        }
      }
    else if (*ptr == '\"')
      {
      DQCount++;

      if ((Head != NULL) && !(SQCount % 2) && !(DQCount % 2))
        {
        /*
        *PtrP = ptr;

        return(Head);
        */
        }
      else
        {
        ptr++;

        ignchar = TRUE;
        }
      }
    else if (*ptr == '[')
      {
      SBCount = 1;
      }
    else if (*ptr == ']')
      {
      SBCount = 0;
      }
    else if (!(SQCount % 2) && !(DQCount % 2) && (SBCount == 0))
      {
      /* not inside quoted space, check if character is delimiter */

      for (dindex = 0;DList[dindex] != '\0';dindex++)
        {
        if (*ptr != DList[dindex])
          continue;

        if ((IgnBackSlash == TRUE) && (Head != NULL))
          {
          /* check if backslash precedes delimiter */

          if ((ptr > Head) && (*(ptr - 1) == '\\'))
            {
            /* check if backslash is backslashed */

            if ((ptr > Head + 1) && (*(ptr - 2) != '\\'))
              {
              /* delimiter is backslashed, ignore */

              continue;
              }
            }
          }    /* END if ((IgnBackSlash == TRUE) && (Head != NULL)) */

        /* delimiter found */

        *ptr = '\0';

        ptr++;

        if (Head != NULL)
          {
          *PtrP = ptr;

          return(Head);
          }

        ignchar = TRUE;

        break;
        }    /* END for (dindex) */
      }

    if ((ignchar != TRUE) && (*ptr != '\0'))
      {
      if (Head == NULL)
        Head = ptr;

      ptr++;
      }

    ignchar = FALSE;
    }  /* END while (*ptr != '\0') */

  *PtrP = ptr;

  return(Head);
  }  /* END MUStrTokEArray() */


/**
 * Perform case-insensitive bounded string comparison.
 *
 * @param String (I)
 * @param Pattern (I)
 * @param SStrLen (I) [optional]
 */

int MUStrNCmpCI(

  const char *String,
  const char *Pattern,
  int         SStrLen)

  {
  int StrLen;

  int pindex;

  if ((String == NULL) || (Pattern == NULL))
    {
    return(FAILURE);
    }

  StrLen = (SStrLen > 0) ? SStrLen : MMAX_BUFFER;

  for (pindex = 0;pindex < StrLen;pindex++)
    {
    if (Pattern[pindex] == '\0') 
      {
      if (String[pindex] != '\0')
        { 
        return(FAILURE);
        }
  
      break;
      }

    if (tolower(String[pindex]) != tolower(Pattern[pindex]))
      {
      return(FAILURE);
      }
    }  /* END for (pindex) */

  return(SUCCESS);
  }  /* END MUStrNCmpCI() */


/**
 * Locate 'Pattern' in 'String'.
 *
 * NOTE: Equivalent to strcasestr() - NOTE strcasestr() is not unix standard
 * (performs case-insensitive string search)
 *
 * NOTE: equivalent to strcasestr() - strcasestr() is not unix standard
 *
 * @param String (I)
 * @param Pattern (I)
 * @param TIndex (I) [optional,used if GetLast==TRUE]
 * @param IsCaseInsensitive (I)
 * @param GetLast (I)
 *
 * @return pointer to first character of match if found, otherwise return NULL
 */

char *MUStrStr(

  char       *String,
  const char *Pattern,
  int         TIndex,
  mbool_t     IsCaseInsensitive,
  mbool_t     GetLast)

  {
  int sindex;
  int pindex;

  int slen;
  int plen;

  if ((String == NULL) || (Pattern == NULL))
    {
    return(NULL);
    }

  /* TIndex is starting tail index when GetLast is true */


  slen = strlen(String); 
  plen = strlen(Pattern);

  if (plen < 1)
    {
    return(&String[0]);
    }

  if (GetLast == FALSE)
    {
    if (IsCaseInsensitive == TRUE)
      {
      /* The last place where you would be able to find the entire string is at 
        len(String) - len(Pattern).  After that you are guaranteed to run off
        the end of String. */

      slen -= plen;

      for (sindex = 0;sindex <= slen;sindex++)
        {
        for (pindex = 0;Pattern[pindex] != '\0';pindex++)
          {
          if (tolower(String[sindex + pindex]) != tolower(Pattern[pindex]))
            break;
          }  /* END for (pindex) */

        if (Pattern[pindex] == '\0')
          {
          return(&String[sindex]);
          }
        }  /* END for (sindex) */
      }
    else
      {
      /* IsCaseInsensitive == FALSE */

      return strstr(String,Pattern);
      } /* END else (IsCaseInsensitive == FALSE) */
    } /* END if (GetLast == FALSE) */
  else
    {
    /* GetLast == TRUE */

    sindex = (TIndex > 0) ? TIndex - plen : slen - plen;

    if (IsCaseInsensitive == TRUE)
      {
      for (;sindex >= 0;sindex--)
        {
        for (pindex = 0;Pattern[pindex] != '\0';pindex++)
          {
          if (tolower(String[sindex + pindex]) != tolower(Pattern[pindex]))
            break;
          }  /* END for (pindex) */

        if (Pattern[pindex] == '\0')
          {
          return(&String[sindex]);
          }
        }  /* END for (sindex) */
      } /* END if (IsCaseInsensitive == TRUE) */
    else
      {
      /* IsCaseInsensitive == FALSE */

      for (;sindex >= 0;sindex--)
        {
        for (pindex = 0;Pattern[pindex] != '\0';pindex++)
          {
          if (String[sindex + pindex] != Pattern[pindex])
            break;
          }  /* END for (pindex) */

        if (Pattern[pindex] == '\0')
          {
          return(&String[sindex]);
          }
        }  /* END for (sindex) */
      } /* END else (IsCaseInsensitive == FALSE) */
    }    /* END else (GetLast == TRUE) */

  return(NULL);
  }  /* END MUStrStr() */


/**
 * This function will do a strcmp at the end of the string.
 * (It will see if Pattern exists at the end of String.)
 *
 * @param Pattern (I)
 * @param String (I)
 */

int MUStrCmpReverse(

  const char *Pattern,
  const char *String)

  {
  int PatternLen;
  int StringLen;
  int rc;
  const char *ptr;

  PatternLen = strlen(Pattern);
  StringLen = strlen(String);

  if (PatternLen > StringLen)
    {
    return(1);
    }

  ptr = &String[StringLen - PatternLen];

  rc = strcmp(Pattern,ptr);

  return(rc);
  }  /* END MUStrCmpReverse() */


/**
 * Append new string to end of existing dynamically allocated buffer with delimiter.
 * This function may be bad if used often--it can increase Moab's heap fragmentation
 * due to many mallocs of different sizes. Consider using a more robust function such
 * as MUSNXPrintF().
 *  
 * NOTE:  see MUStrAppendStatic() for appending to statically allocated buffers 
 *
 * @see MUCheckStringList()
 *
 * @param Dst (O) [modified,alloc]
 * @param DstLen (I) Then length of the string contained in Dst (not the size of Dst). Helps improve performance. [optional,modified]
 * @param Src (I)
 * @param Delim (I) [optional,'\0' indicates not set]
 */

int MUStrAppend(

  char      **Dst,     /* O (modified,alloc) */
  int        *DstLen,  /* I (optional,modified) */
  const char *Src,     /* I */
  char        Delim)   /* I (optional,'\0' indicates not set) */

  {
  int len = -1;
  int SrcLen = 0;

  if (Dst == NULL)
    {
    return(FAILURE);
    }

  if ((Src == NULL) || (Src[0] == '\0'))
    {
    return(SUCCESS);
    }

  if (*Dst == NULL)
    {
    if (DstLen != NULL)
      *DstLen = strlen(Src);

    return(MUStrDup(Dst,Src));
    }

  if (DstLen != NULL)
    {
    len = *DstLen;
    }
  else
    {
    len = strlen(*Dst);
    }

  /* extend existing buffer */

  SrcLen = strlen(Src);

  /* Get new buffer that as long as: original, plus the length of src, 
   * plus null byte and the Delim character all concatenated
   */
  *Dst = (char *)MOSrealloc(*Dst,len + SrcLen + 2);

  if (*Dst == NULL)
    {
    return(FAILURE);
    }

  /* Store the Delim character, and if DstLen ptr, bump count */
  if (Delim != '\0')
    {
    (*Dst)[len++] = Delim;

    if (DstLen != NULL)
      (*DstLen)++;
    }

  /* Do the copy now */
  strcpy(*Dst + len,Src);

  /* Add in the SrcLen count to the Destnation count */
  if (DstLen != NULL)
    *DstLen += SrcLen;

  return(SUCCESS);
  }  /* END MUStrAppend() */


/**
 * Append string to existing static buffer w/delimiter.
 * 
 * NOTE: see MUStrAppend() for appending to dynamic buffer
 *
 * @param IString (I) [modified]
 * @param Attr (I)
 * @param Delim (I)
 * @param IStringSize (I)
 */

int MUStrAppendStatic(

  char       *IString,
  const char *Attr,
  char        Delim,
  int         IStringSize)

  {
  char tmpS[2];

  if ((IString == NULL) ||
      (Attr == NULL))
    {
    return(FAILURE);
    }

  if (Attr[0] == '\0')
    {
    return(SUCCESS);
    }

  if (IString[0] == '\0')
    {
    MUStrCat(IString,Attr,IStringSize);

    return(SUCCESS);
    }

  tmpS[0] = Delim;
  tmpS[1] = '\0';

  MUStrCat(IString,tmpS,IStringSize);
  MUStrCat(IString,Attr,IStringSize);

  return(SUCCESS);
  }  /* END MUStrAppendStatic() */



/**
 * Report TRUE is string contains alpha characters.
 *
 * @param String (I)
 */

mbool_t MUStrHasAlpha(

  const char *String)

  {
  const char *ptr;

  if ((String == NULL) || (String[0] == '\0'))
    {
    return(FALSE);
    }

  for (ptr = String;*ptr != '\0';ptr++)
    {
    if (isalpha(*ptr))
      {
      return(TRUE);
      }
    }    /* END for (ptr) */

  return(FALSE);
  }  /* END MUStrHasAlpha() */




/**
 * return TRUE if the string contains at least one digit
 *
 * @param String (I)
 */

mbool_t MUStrHasDigit(

  const char *String)

  {
  const char *ptr;

  if ((String == NULL) || (String[0] == '\0'))
    {
    return FALSE;
    }

  for (ptr = String; *ptr != '\0'; ptr++)
    {
    if (isdigit(*ptr))
      {
      return TRUE;
      } 
    } /* END for (ptr = String...) */
  
  return FALSE;
  } /*END MUStrHasDigit() */



/**
 * return TRUE if the string is all digits
 *
 * @param String (I)
 */

mbool_t MUStrIsDigit(

  const char *String)   /* I */

  {
  const char *ptr;

  if ((String == NULL) || (String[0] == '\0'))
    {
    return FALSE;
    }

  for (ptr = String; *ptr != '\0'; ptr++)
    {
    if (!isdigit(*ptr))
      {
      return(FALSE);
      } 
    } /* END for (ptr = String...) */
  
  return(TRUE);
  } /*END MUStrHasDigit() */



/**
 * report whether or not the given char * is empty
 *
 * @param String (I)
 *
 * returns TRUE if String[0] == '\0'
 * returns FALSE otherwise
 */

mbool_t MUStrIsEmpty(

  const char *String) /* I */

  {
  /* NULL may not be considered the same as empty, but I added this test 
     to prevent core dumps if it is called with a NULL. */

  if (String == NULL)
    return(TRUE);

  if (String[0] == '\0')
    return(TRUE);

  return(FALSE);
  }



/**
 * Inserts a new string in a new buffer by overwriting existing data in that buffer.
 * Users specify where they wish the overwriting to occur. If users wish to append data
 * to the end of the string, they should set IIndex to strlen(Buf) and RLen to 0.
 *
 * Note that this function will dynamically grow the given Buf if it is not large enough
 * for the replacement to occur i.f.f. IsDynamic is set to TRUE.
 *
 * @see MUStrReplaceChar() - peer - for replacing individual characters in a string.
 * @see MUStrRemoveFromEnd() - peer - for removing all instances of a substring from the end of another string.
 * @see MUBufRemoveChar() - peer
 * @see MUStrRemoveFromList() - peer - for removing attr=val[delim] pairs from list.
 *
 * @param Buf (I) Buffer to modify/overwrite. [modified]
 * @param BufSize (I/O) Size of currently allocated memory for Buf. [modified]
 * @param IIndex (I) Location (index) in Buf where new string should start overwriting data.
 * @param RLen (I) Length of original string being replaced (i.e. how much of String to write to Buf).
 * @param String (I) New replacement string.
 * @param IsDynamic (I) Set to TRUE if Buf is alloc'd and you wish this function to grow Buf if necessary.
 */

int MUBufReplaceString(

  char    **Buf,       /* I (modified) */
  int      *BufSize,   /* I/O (modified) */
  int       IIndex,    /* I (insertion index) */
  int       RLen,      /* I (length of original string being replaced) */
  const char *String,    /* I (new replacement string) */
  mbool_t   IsDynamic) /* I */

  {
  int rc;
  int slen;
  int blen;
  int bsize;

  int tlen;
  int mylen;

  char *head;
  char *ptr;

  if ((Buf == NULL) || (BufSize == NULL) || (IIndex < 0) || (IIndex > *BufSize-1))
    {
    return(FAILURE);
    }

  if (String == NULL)
    {
    return(SUCCESS);
    }

  slen = strlen(String);

  if (*Buf != NULL)
    {
    blen = strlen(*Buf);
    bsize = blen + 1;
    }
  else
    {
    blen = 0;
    bsize = 0;
    *BufSize = 0;
    }
  
  tlen = MAX(0,bsize + slen - RLen);

  if (tlen > *BufSize)
    {
    /* current string is too small */

    if (FALSE == IsDynamic)
      {
      return(FAILURE);
      }

    if (NULL == *Buf)
      *Buf = (char *)MUCalloc(1,tlen + 1);
    else
      *Buf = (char *)MOSrealloc(*Buf,tlen);

    if (NULL == *Buf)
      {
      /* No memory error */
      return(FAILURE);
      }

    *BufSize = tlen;
    }  /* END if (tlen > *BufSize) */

  head = *Buf;

  mylen = MIN(*BufSize - IIndex, RLen);

  if (head[IIndex + mylen] != '\0')
    {
    ptr = MOSstrdup(head + IIndex + mylen);
    if (NULL == ptr)
      {
      return(FAILURE);
      }
    }
  else 
    {
    ptr = NULL;
    }
  
  /* Assume good return here */
  rc = SUCCESS;

  /* Ensure we have a valid destination buffer */
  if ((head + IIndex) != NULL)
    {
    strcpy(head + IIndex,String);
    }
  else
    {
    rc = FAILURE;
    }

  if (ptr != NULL)
    {
    /* Ensure we have a valid destination buffer */
    if ((head + IIndex + slen) != NULL)
      {
      strcpy(head + IIndex + slen,ptr);
      }
    else
      {
      rc = FAILURE;
      }

    MUFree(&ptr);
    }
 
  return(rc);
  }  /* END MUBufReplaceString() */





/**
 * Remove all instances of RemoveChar out to TerminationChar.
 * 
 * Fill remaining space with ReplacementChar
 *
 * NOTE:  Do NOT override TerminationChar
 */

int MUBufRemoveChar(

  char *Buf,              /* I (modified) */
  char  RemoveChar,       /* I */
  char  TerminationChar,  /* I */
  char  ReplacementChar)  /* I */

  {
  char *bptr;
  int   cindex;

  int   Offset = 0;

  if (Buf == NULL)
    {
    return(FAILURE);
    }

  for (bptr = Buf;*bptr != '\0';bptr++)
    {
    if (*bptr == TerminationChar)
      {
      /* end of string located */
 
      break;
      }

    if (*bptr == RemoveChar)
      {
      Offset++;

      continue;
      }

    if (Offset > 0)
      {
      /* copy character */

      *(bptr - Offset) = *bptr;
      }
    }  /* END for (bptr) */

  /* fill end of string with ReplacementChar */

  for (cindex = 0;cindex < Offset;cindex++)
    {
    *(bptr - cindex - 1) = ReplacementChar;
    }

  return(SUCCESS);
  }  /* END MUBufRemoveChar() */


/**
 * Removes whitespace ('\n', ' ', '\t', '\r', '\f', '\v') from end of the given string.
 *
 * NOTE:  acts like PERL's 'chomp'
 *
 * @param String (I) [modified]
 */

int MUStringChop(

  char *String)  /* I (modified) */

  {
  int i;

  if (String == NULL)
    {
    return(FAILURE);
    }

  /* remove whitespace from end of string */

  for (i = strlen(String) - 1;i >= 0;i--)
    {
    if (!isspace(String[i]))
      break;

    String[i] = '\0';
    }  /* END for (i) */

  return(SUCCESS);
  }  /* END MUStringChop() */


/**
 * Searches for a string in a list, separated by the delimiter.
 *
 * @param StrToSearch (I) - The list to search in
 * @param StrToFind   (I) - The string that we are trying to find
 * @param Delim       (I) - The delimiter
 */

mbool_t MUStrIsInList(
  const char *StrToSearch,
  const char *StrToFind,
  const char Delim)

  {
  const char *CurLoc = NULL;
  const char *tmpPtr = NULL;
  mbool_t StartValid = FALSE;
  mbool_t EndValid = FALSE;

  if ((StrToSearch == NULL) ||
      (StrToFind == NULL))
    return(FALSE);

  CurLoc = StrToSearch;

  while ((CurLoc != NULL) && (CurLoc[0] != '\0'))
    {
    StartValid = FALSE;
    EndValid = FALSE;

    CurLoc = strstr(CurLoc,StrToFind);

    if ((CurLoc == NULL) || (CurLoc[0] == '\0'))
      return(FALSE);

    /* Found a possible match */

    if (CurLoc == StrToSearch)
      StartValid = TRUE;
    else
      {
      CurLoc--;

      if (CurLoc[0] == Delim)
        StartValid = TRUE;

      CurLoc++;
      }

    tmpPtr = CurLoc + strlen(StrToFind);

    /* tmpPtr is now 1 past the end of StrToFind */

    if ((tmpPtr[0] == '\0') || (tmpPtr[0] == Delim))
      EndValid = TRUE;

    if ((StartValid == TRUE) && (EndValid == TRUE))
      return(TRUE);

    CurLoc = strchr(CurLoc,Delim);
    }  /* END while ((CurLoc != NULL) || (CurLoc[0] != '\0')) */ 

  return(FALSE);
  }  /* END MUStrIsInList() */



/**
 * Report index of matching comparison (enum MCompEnum)
 *
 * @see MUParseComp() - parent/peer
 *
 * @param Line (I)
 * @param Size (O) [optional] size of comparison string
 */

int MUCmpFromString(
 
  char *Line,  /* I */
  int  *Size)  /* O (optional) */
 
  {
  int index;
  int CIndex;
  int Len;

  const char *FName = "MUCmpFromString";
 
  MDB(9,fSTRUCT) MLog("%s(%s,Size)\n",
    FName,
    Line);
 
  CIndex = 0;

  if (Size != NULL)
    *Size = 0;
 
  for (index = 0;MComp[index] != '\0';index++)
    {
    Len = strlen(MComp[index]);
 
    if (strncmp(Line,MComp[index],Len) != 0)
      continue;
 
    if (Len == 2)
      {
      if (Size != NULL)
        *Size = Len;

      return(index);
      }
 
    CIndex = index;
    
    if (Size != NULL)
      *Size = Len;
    }  /* END for (index) */

  if (CIndex == mcmpEQ2)
    CIndex = mcmpEQ;
  else if (CIndex == mcmpNE2)
    CIndex = mcmpNE;
 
  return(CIndex);
  }  /* END MUCmpFromString() */


/* END MUStr.c */
