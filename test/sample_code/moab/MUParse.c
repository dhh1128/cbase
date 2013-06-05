/* HEADER */

/**
 * @file MUParse.c
 * 
 * Contains various functions for parsing options, etc
 *
 */


#include "moab.h"
#include "moab-proto.h"
#include "moab-const.h"
#include "moab-global.h"


/**
 * Parse formatted comparison string.
 *
 * @see MUCmpFromString() - child/peer
 *
 * FORMAT:  <KEYWORD>[<WS>]<COMP>[<WS>]["]<VAL>[<WS>][<SYMBOL>]
 * NOTE:    <VAL> -> [<ALNUM>{._-}]
 *
 * NOTE:  supports incr/decr (+=/-=) 
 * NOTE:  parses X=Y=Z to AName='X' Val='Y=Z' 
 * 
 * @param IString        (I)
 * @param AName          (O) [minsize=MMAX_NAME]
 * @param Cmp            (O) [enum MCompEnum *]
 * @param ValLine        (O) [minsize=VBufSize]
 * @param VBufSize       (I)
 * @param ConstrainValue (I) [if TRUE, only allow alnum and limited symbols]
 */

int MUParseComp(

  char           *IString,  
  char           *AName,   
  enum MCompEnum *Cmp,    
  char           *ValLine,
  int             VBufSize, 
  mbool_t         ConstrainValue) 

  {
  char tmpLine[MMAX_LINE];

  char *base;

  char *ptr;

  const char *IgnCList = " \t\n()[]\'\"";
  const char *CmpCList = "+-<>!=";
  const char *FName = "MUParseComp";

  /* FORMAT:  <KEYWORD>[<WS>]<COMP>[<WS>]["]<VAL>[<WS>][<SYMBOL>] */
  /* NOTE:    <VAL> -> [<ALNUM>{._-@}] */

  MDB(9,fLL) MLog("%s('%s',AName,Cmp,ValLine)\n",
    FName,
    IString);

  /* Must have an AName pointer to complete this function successfully */

  if (AName == NULL)
    return(FAILURE);

  AName[0] = '\0';    /* Ensure at least an empty string */

  if (Cmp != NULL)
    *Cmp = mcmpNONE;

  if (ValLine != NULL)
    ValLine[0] = '\0';

  if (IString == NULL)
    {
    return(FAILURE);
    }

  ptr = IString;

  /* ignore whitespace */

  while (strchr(IgnCList,*ptr))
    {
    ptr++;
    }

  /* step over keyword */

  base = ptr;

  while (isalnum(*ptr) || (*ptr == '_'))
    {
    AName[ptr - base] = *ptr;
    AName[ptr - base + 1] = '\0';

    ptr++;

    if (ptr - base >= MMAX_NAME)
      break;
    }

  /*support map-style attributes. Will a map-style attribute ever need more space
    than MMAX_NAME? */
  if (ptr[0] == '[')
    {
    char *endPtr;
    endPtr = strchr(ptr + 1, ']');

    if (endPtr != NULL)
      {
      int len = MIN(endPtr - ptr + 1,MMAX_NAME - (ptr - base));

      strncpy(AName + (ptr - base),ptr,len);
      AName[(ptr - base) + len] = 0;

      ptr = endPtr + 1;

      }
    else
      {
      AName[ptr - base] = '\0';
      }
    }


  if (*ptr == '\0')
    {
    /* end of string reached */

    return(SUCCESS);
    }

  /* step over white space */

  while ((*ptr != '\0') && strchr(IgnCList,*ptr))
    {
    ptr++;
    }

  /* get comparison */

  base = ptr;

  if (Cmp != NULL)
    *Cmp = mcmpEQ;

  if ((*ptr == 'e') || (*ptr == 'n'))
    {
    /* handle 'english' comparisons */

    if (!strncmp(ptr,"eq",2))
      {
      if (Cmp != NULL)
        *Cmp = mcmpEQ; 

      ptr += 2;
      }
    else if (!strncmp(ptr,"ne",2))
      {
      if (Cmp != NULL)
        *Cmp = mcmpNE;

      ptr += 2;
      }
    }
  else
    { 
    /* NOTE:  with '=-X', the negative should not be ignored */

    if ((ptr[0] == '=') && ((ptr[1] == '+') || (ptr[1] == '-')))
      { 
      /* remove first char only */

      tmpLine[ptr - base] = *ptr;

      ptr++;
      }
    else
      {
      /* NOTE:  strchr() can return SUCCESS if *ptr == '\0' */

      while ((*ptr != '\0') && (strchr(CmpCList,*ptr) != NULL))
        {
        tmpLine[ptr - base] = *ptr;

        ptr++;
        }
      }

    tmpLine[ptr - base] = '\0';

    if (Cmp != NULL)
      *Cmp = (enum MCompEnum)MUCmpFromString(tmpLine,NULL);
    }

  if (Cmp != NULL)
    {
    if (*Cmp == mcmpNE2)
      *Cmp = mcmpNE;
    }

  /* step over white space */

  while ((*ptr != '\0') && strchr(IgnCList,*ptr))
    {
    ptr++;
    }

  /* load alphanumeric/symbol value */

  base = ptr;

  /* support { <anything> } */

  if (*ptr == '{')
    {
    ptr++;

    base = ptr;

    while (*ptr != '}')
      {
      ValLine[ptr - base] = *ptr;

      ptr++;

      if (ptr - base >= VBufSize)
        break;
      }
    
    ValLine[ptr - base] = *ptr;
    }
  else if (ConstrainValue == TRUE)
    {
    while (isalnum(*ptr) || 
          (*ptr == '-') || 
          (*ptr == '_') || 
          (*ptr == '.') || 
          (*ptr == ':') ||
          (*ptr == '=') ||
          (*ptr == '+'))
      {
      ValLine[ptr - base] = *ptr;

      ptr++;

      if (ptr - base >= VBufSize)
        break;
      }
    }    /* END else if (ConstrainValue == TRUE) */
  else
    {
    while (*ptr != '\0')
      {
      ValLine[ptr - base] = *ptr;

      ptr++;

      if (ptr - base >= VBufSize)
        break;
      }
    }

  ValLine[ptr - base] = '\0';

  return(SUCCESS);
  }  /* END MUParseComp() */





/**
 *
 *
 * @param String (I) [modified]
 */

int MUPurgeEscape(

  char *String)  /* I (modified) */

  {
  char *hptr;
  char *tptr;

  mbool_t EscapeMode;
  mbool_t QuoteMode;

  EscapeMode = FALSE;
  QuoteMode  = FALSE;

  tptr = String;

  /* NOTE:  routine conflicts w/XML */

  /* remove double quotes unless backslashed */

  /* NOTE:  test \"cat\" "dog"  ->  test "cat" dog */

  /* NOTE:  QuoteMode ignored (incomplete) */

  for (hptr = String;*hptr != '\0';hptr++)
    {
    if ((EscapeMode == FALSE) && (*hptr == '\\'))
      {
      EscapeMode = TRUE;

      continue;
      }

    if ((*hptr == '\"') && (EscapeMode == FALSE))
      {
      QuoteMode = 1 - QuoteMode;

      continue;
      }

    /* copy character */

    *tptr = *hptr;

    tptr++;

    EscapeMode = FALSE;
    }

  /* terminate string */

  *tptr = '\0';

  return(SUCCESS);
  }  /* END MUPurgeEscape() */



/**
 * Translate range string into list of matching nodes.
 *
 * @see MUNLFromRangeString() - parent
 *
 * NOTE: primarily used with SLURM and job arrays
 *
 * @param RangeString (I)
 * @param BM (O) [optional]
 * @param NIndexList (O) [optional]
 * @param NIndexSize (I)
 * @param NCount (O) [optional]
 * @param NZPadLen (O) [optional]
 */

int MUParseRangeString(

  const char   *RangeString, /* I */
  mulong *BM,          /* O range values represented in bitmap format - only valid for range 0-32 (optional) */
  int    *NIndexList,  /* O (optional) */
  int     NIndexSize,  /* I */
  int    *NCount,      /* O number of node indices located (optional) */
  int    *NZPadLen)    /* O length of index in name if zeropad used (optional) */

  {
  int   rangestart;
  int   rangeend;

  int   rindex;

  int   NIndex;

  int   StepIndex;       /* per range step size */

  char *tptr;            /* temp char pointer used to extract range step */
  char *Line;

  char *rtok;
  char *tail;

  char *TokPtr = NULL;

  /* FORMAT:  RANGESTRING:   <RANGE>[:<STEP>][{,}<RANGE>[<:STEP>]]... */
  /*          RANGE:         <VALUE>[-<VALUE>]    */

  /* NOTE:    The following non-numeric values may appear in the string */
  /*          and should be handled: '&'                                */

  /* NOTE:    Must preserve 'zero-pad' */

  if (NZPadLen != NULL)
    *NZPadLen = 0;

  if ((BM == NULL) && (NIndexList == NULL))
    {
    return(FAILURE);
    }

  if (BM != NULL)
    *BM = 0;

  if (NIndexList != NULL)
    NIndexList[0] = -1;

  NIndex = 0;

  if (RangeString == NULL)
    {
    return(SUCCESS);
    }
 
  for (rindex = 0;RangeString[rindex] != '\0';rindex++)
    {
    if (isalpha(RangeString[rindex]))
      {
      /* string contains name referenced string objects */

      return(FAILURE);
      }
    }    /* END for (rindex) */

  Line = (char *)MUMalloc(sizeof(char) * (strlen(RangeString) + 1));
  
  MUStrCpy(Line,RangeString,strlen(RangeString) + 1);

  rtok = MUStrTok(Line,", \t\n",&TokPtr);

  if (rtok == NULL)
    return(FAILURE);
  
  tptr = strchr(rtok,':');

  if (tptr != NULL)
    {
    *tptr = '\0';

    tptr++;

    StepIndex = (int)strtol(tptr,NULL,10);
    }
  else
    {
    StepIndex = 1;
    }

  while (rtok != NULL)
    {
    int IndexLen = 0;

    while (*rtok == '&')
      rtok++;

    rangestart = strtol(rtok,&tail,10);

    /* if 0 padded */

    if (*rtok == '0')
      {
      if (tail != NULL)
        IndexLen = tail - rtok;
      else
        IndexLen = strlen(rtok);
      }
    
    if ((tail != NULL) && (*tail == '-'))
      rangeend = strtol(tail + 1,&tail,10);
    else
      rangeend = rangestart;

    /* If this is a descending range then switch indexes */

    if (rangestart > rangeend)
      {
      int tmpInt;

      tmpInt = rangestart;
      rangestart = rangeend;
      rangeend = tmpInt;
      }

    if (BM != NULL)
      {
      for (rindex = rangestart;rindex <= rangeend;rindex++)
        {
        *BM |= (1 << rindex);
        }
      }

    if (NIndexList != NULL)
      {
      for (rindex = rangestart;rindex <= rangeend;rindex += StepIndex)
        {
        if (NIndex >= NIndexSize)
          break;

        NIndexList[NIndex] = rindex;

        if (NZPadLen != NULL)
          NZPadLen[NIndex] = IndexLen;

        NIndex++;
        }
      }

    rtok = MUStrTok(NULL,", \t\n",&TokPtr);

    if (rtok != NULL)
      {
      tptr = strchr(rtok,':');

      if (tptr != NULL)
        {
        *tptr = '\0';
   
        tptr++;
   
        StepIndex = (int)strtol(tptr,NULL,10);
        }
      else
        {
        StepIndex = 1;
        }
      }
    }  /* END while (rtok) */

  if (NIndexList != NULL)
    NIndexList[NIndex] = -1;

  if (NCount != NULL)
    *NCount = NIndex;

  MUFree((char **)&Line);

  return(SUCCESS);
  }  /* END MUParseRangeString() */ 





/**
 * Returns the index to the alpha array of the given char.
 *
 * Slurm only reports capital letters for index.
 *
 * @param axis The given character to find an index for.
 * @return Returns the index into the alpha array.
 */

int MUGetBGAlphaIndex( 
    
  char axis)  /* I */

  {
  int   i;
  const char *alpha_num = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"; /* base 36 */

  for (i = 0;i < 36;i++)
    {
    if (axis == alpha_num[i])
      {
      return(i);
      }
    } /* END for( i = 0 */

  return(-1);
  }  /* END MUGetBGAlphaIndex() */





/**
 * Parses a slurm bluegene range expression and returns the list node ids.
 *
 * Range format: 000x213,205,306,400x733, where range 000x203 is:<br> 
 * 000,001,002,003,010,...,013,100,...,213<br>
 * Allocates space for indexes.
 *
 * @param RangeString (I) Comes as 000x213,205,306,400x733
 * @param NCharList (O) List to store node indexes in [alloc for second pointer]
 * @param NIndexSize (I) Size of NCharList
 * @param NCount (I) The number of nodes add to the list [optional]
 */

int MUParseBGRangeString(

  char   *RangeString, /* I */
  char  **NCharList,   /* O [alloc]*/
  int     NIndexSize,  /* I */
  int    *NCount)      /* O number of node indices located (optional) */
  
  {
  const char *alpha_num = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

  /* FORMAT: sXsYsZxeXeYeZ */
  int OrigStartY;
  int OrigStartZ;

  int startX;
  int startY;
  int startZ;

  int endX;
  int endY;
  int endZ;

  char *rangePtr; /* Pointer to RangeString */
  char *TokPtr;

  int NIndex;
  
  NIndex = 0;

  if (RangeString == NULL || NCharList == NULL || NIndexSize <= 0)
    {
    return(FAILURE);
    }

  rangePtr = MUStrTokEArrayPlus(RangeString,",",&TokPtr,FALSE);

  /* Tokenize on commas to get all ranges */

  while (rangePtr != NULL)
    { 
    if (strchr(rangePtr,'x') != NULL) /* range 000x204 */
      {
      /* Get start xyz and end xyz */

      startX = MUGetBGAlphaIndex(*rangePtr++);
      startY = MUGetBGAlphaIndex(*rangePtr++);
      startZ = MUGetBGAlphaIndex(*rangePtr++);

      OrigStartY = startY;
      OrigStartZ = startZ;

      rangePtr++; /* increment past x */

      endX = MUGetBGAlphaIndex(*rangePtr++);
      endY = MUGetBGAlphaIndex(*rangePtr++);
      endZ = MUGetBGAlphaIndex(*rangePtr++);

      /* check to see if there was an invalid character in the range */

      if ((startX < 0) || (startY < 0) || (startZ < 0) ||
            (endX < 0) ||   (endY < 0) ||   (endZ < 0))
        {
        return(FAILURE);
        }

      /* make node index list */

      for (;startX <= endX; startX++)
        {
        for(;startY <= endY; startY++)
          {
          for(;startZ <= endZ; startZ++)
            {
            NCharList[NIndex] = (char *)MUMalloc(sizeof(char) * 4); /* malloc 4: 3 indicies + NULL */

            sprintf(NCharList[NIndex++],"%c%c%c",
                alpha_num[startX],
                alpha_num[startY],
                alpha_num[startZ]);

            if (NIndex >= NIndexSize)
              {
              /* break all loops */

              startX = endX + 1;
              startY = endY + 1;
              startZ = endZ + 1;
              }
            } /* END for(;startZ <= endZ; startZ++) */
          startZ = OrigStartZ; /* reinitialize loop to 0 */
          } /* END for(;startY <= endY; startY++) */
        startY = OrigStartY;
        } /* END for (;startX <= endX; startX++) */
      } /* END if (strchr(rangePtr,'x') */
    else
      {
      /* single node number */

      NCharList[NIndex] = (char *)MUMalloc(sizeof(char) * 4); /* malloc 4: 3 indicies + NULL */

      strcpy(NCharList[NIndex++],rangePtr);
      } /* END if (strchr(rangePtr,'x') != NULL) */

    if (NIndex >= NIndexSize)
      {
      break; /* break while */
      }
  
    rangePtr = MUStrTokEArrayPlus(NULL,",",&TokPtr,FALSE);
    } /* END while (rangePtr != NULL) */

  if (NCount != NULL)
    *NCount = NIndex;

  return(SUCCESS);
  } /* END MUParseBGRangeString() */




