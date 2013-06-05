/* HEADER */

      
/**
 * @file MUtilFromString.c
 *
 * Contains:  MUBoolFromString MUCmpFromString2
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 * Extract boolean value from string.
 *
 * @param Value (I)
 * @param Default (I)
 */

mbool_t MUBoolFromString(

  char const *Value,   /* I */
  mbool_t     Default) /* I */

  {
  int index;

  if ((Value == NULL) || (Value[0] == '\0'))
    {
    return(Default);
    }

  if (Default == TRUE)
    {
    for (index = 0;MFalseString[index] != NULL;index++)
      {
      if (!strcasecmp(Value,MFalseString[index]))
        {
        return(FALSE);
        }
      }    /* END for (index) */
    }
  else  
    {
     for (index = 0;MTrueString[index] != NULL;index++)
      {
      if (!strcasecmp(Value,MTrueString[index]))
        {
        return(TRUE);
        }
      }    /* END for (index) */
    }  

  return(Default);     
  }  /* END MUBoolFromString() */

  


int MUCmpFromString2(

  char *Line)  /* I */

  {
  int CIndex;               /* the return value */
  int InLen;                /* length of input string */
  int LenS;                 /* length of comparator symbols */
  int LenA = 2;             /* length of comparator abbreviation 
                             * HACK: always 2 right now */

  const char *FName = "MUCmpFromString2";
  
  MDB(9,fSTRUCT) MLog("%s(%s,Size)\n",
    FName,
    Line);

  if (Line == NULL)
    return mcmpNONE;

  InLen = strlen(Line);

  for (CIndex = 1; (CIndex != mcmpLAST) && (MCompAlpha[CIndex] != '\0'); CIndex++)
    {

    LenS = strlen(MComp[CIndex]);

    if (InLen == LenS)
      {

      const char* MCompPtr;
      char* LinePtr;
      mbool_t match = TRUE;

      for (MCompPtr = MComp[CIndex], LinePtr = Line; *MCompPtr != '\0'; MCompPtr++, LinePtr++)
        {
        if (*MCompPtr != *LinePtr)
          {
          match = FALSE;
          break;
          }
        }
    
      if (match) 
        {
        assert(!strcmp(Line,MComp[CIndex]));
        break;
        }

      }

    if (InLen == LenA)
      {

      const char* MCompPtr;
      char* LinePtr;
      mbool_t match = TRUE;


      for (MCompPtr = MCompAlpha[CIndex], LinePtr = Line; (*MCompPtr != '\0') && (*Line != '\0'); MCompPtr++, LinePtr++)
        {
        if (tolower(*MCompPtr) != tolower(*LinePtr))
          {
          match = FALSE;
          break;
          }
        }

        
      if (match)
        {
        assert(!strcmp(Line,MCompAlpha[CIndex]));
        break;
        }

      }

    }

  return CIndex;
  }
/* END MUtilFromString.c */
