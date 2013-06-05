/* HEADER */

      
/**
 * @file MUtilNumeric.c
 *
 * Contains: Numeric utility functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 *
 *
 * @param String (I)
 * @param Modifier (I) [output mode]
 * @param DefMod (I) [default resource modifier - used if none is specified]
 */

long MURSpecToL(

  char             *String,    /* I */
  enum MValModEnum  Modifier,  /* I (output mode) */
  enum MValModEnum  DefMod)    /* I (default resource modifier - used if none is specified) */

  {
  long  val;
  char *ptr;
  char *ptr2;

  int   tmpMod;

  const char ModC1[] = "bwkmgtpezy";
  const char ModC2[] = "BWKMGTPEZY";

  const int  ModVal[] = { 0, 3, 10, 20, 30, 40, 50, 60, 70, 80 };

  int index;

  /* translate resource quantity spec to long */

  /* FORMAT:  HH:MM:SS || <VAL>[<MOD>] */

  if ((String == NULL) || (String[0] == '\0'))
    {
    return(0);
    }
    
  val = strtol(String,&ptr,10);

  if (*ptr == ':')   /* time resource -> convert to seconds */
    {
    /* time value detected */

    /* FORMAT [[hours:]minutes:]seconds */
    /* currently supports { seconds | hours:minutes[:seconds] } */

    /* currently drop milliseconds */

    val *= 3600;                             /* hours   */

    val += (strtol(ptr + 1,&ptr2,10) * 60);  /* minutes */

    if (*ptr2 == ':')
      val += strtol(ptr2 + 1,&ptr,10);       /* seconds */

    return(val);
    }

  tmpMod = 0;

  tmpMod -= ModVal[Modifier];

  if (!strncmp(ptr,"-1",2))
    {
    return(-1);
    }

  if ((ptr[0] != '\0') && ((ptr[1] == ModC1[mvmWord]) || (ptr[1] == ModC2[mvmWord])))
    {
    /* adjust for word */

    tmpMod += ModVal[mvmWord];
    }

  for (index = mvmByte;index <= mvmTera;index++)
    {
    if ((ptr[0] == ModC1[index]) || (ptr[0] == ModC2[index]))
      {
      tmpMod += ModVal[index];

      break;
      }
    }    /* END for (index) */

  if (index > mvmTera)
    {
    tmpMod += ModVal[DefMod];
    }

  if (tmpMod > 0)
    {
    val <<= tmpMod;
    }
  else
    {
    tmpMod *= -1;

    val >>= tmpMod;
    }

  return(val);
  }  /* END MURSpecToL() */




/**
 * Converts a number into the respective mode's output.
 *
 * ex. convert  111222333444 -> 111G 
 * NOTE:  Max output length is 4 or fewer chars
 *
 * @param LVal (I)
 * @param BaseMod (I)
 * @param Buf (O) [optional,minsize=MMAX_NAME]
 */

char *MULToRSpec(

  long              LVal,    /* I */
  enum MValModEnum  BaseMod, /* I */
  char             *Buf)     /* O (optional,minsize=MMAX_NAME) */

  {
  int index;

  long tmpL;

  static char tmpBuf[MMAX_NAME];

  const char ModC[] = "\0KMGTPEZY";

  char *ptr;

  if (Buf != NULL)
    ptr = Buf;
  else
    ptr = tmpBuf;

  ptr[0] = '\0';

  tmpL = LVal;

  for (index = 0;index < 6;index++)
    {
    LVal = tmpL;

    tmpL >>= 10;

    if (tmpL < 10)
      { 
      sprintf(ptr,"%ld%c",
        LVal,
        (LVal > 0) ? ModC[index + MAX(0,(int)BaseMod - 1)] : '\0');

      return(ptr);
      }
    }    /* END for (index) */

  sprintf(ptr,"%ld%c",
    tmpL,
    (tmpL > 0) ? ModC[index + MAX(0,(int)BaseMod - 1)] : '\0');

  return(ptr);
  }  /* END MULToRSpec() */







/**
 * Converts the given value to a relative number
 *
 * If the given value is larger than 999,999 then the number 
 * is converted to a relative amount, for example:111222333444.55 -> 111.22G 
 * <p>NOTE:  Max output length with suffix is 6 or fewer chars 
 *
 * @param DVal (I)
 * @param BaseMod (I)
 * @param Buf (O) [optional,minsize=MMAX_NAME]
 * @return Returns the relative value as a string
 */

char *MUDToRSpec(

  double            DVal,    /* I */
  enum MValModEnum  BaseMod, /* I */
  char             *Buf)     /* O (optional,minsize=MMAX_NAME) */

  {
  int index;

  double tmpD;

  static char tmpBuf[MMAX_NAME];

  const char ModC[] = "\0KMGTPEZY";

  char *ptr;

  char  s;   /* modifier suffix */

  if (Buf != NULL)
    ptr = Buf;
  else
    ptr = tmpBuf;

  ptr[0] = '\0';

  tmpD = DVal;

  for (index = 0;index < 6;index++)
    {
    DVal = tmpD;

    if (tmpD < 1000.0)
      {
      s = (DVal > 0) ? ModC[index + MAX(0,(int)BaseMod - 1)] : '\0';

      if ((tmpD < 100.0) || (s == '\0'))
        {
        sprintf(ptr,"%.2f%c",
          DVal,
          s);
        }
      else
        {
        sprintf(ptr,"%.1f%c",
          DVal,
          s);
        }

      return(ptr);
      }

    tmpD /= 1000.0;
    }    /* END for (index) */

  s = (tmpD > 0.0) ? ModC[index + MAX(0,(int)BaseMod - 1)] : '\0';

  if ((tmpD < 100.0) || (s == '\0'))
    {
    sprintf(ptr,"%.2f%c",
      tmpD,
      s);
    }
  else
    {
    sprintf(ptr,"%.1f%c",
      tmpD,
      s);
    }

  return(ptr);
  }  /* END MUDToRSpec() */





/**
 *
 *
 * @param A (I)
 * @param Cmp (I)
 * @param B (I)
 */

int MUCompare(

  int A,   /* I */
  int Cmp, /* I */
  int B)   /* I */

  { 
  int val;

  switch (Cmp)
    {
    case mcmpGT:

      val = (A > B);

      break;

    case mcmpGE:

      val = (A >= B);

      break;

    case mcmpEQ:

      val = (A == B);

      break;

    case mcmpLE:

      val = (A <= B);

      break;

    case mcmpLT:

      val = (A < B);

      break;

    case mcmpNE:

      val = (A != B);

      break;

    case mcmpNONE:

      val = 1;

      break;
   
    default:

      val = 0;

      break;
    }  /* END switch (Cmp) */

  return(val);
  }  /* END MUCompare() */





/**
 * Perform requested comparison on double values.
 *
 * @param A (I)
 * @param Cmp (I)
 * @param B (I)
 */

int MUDCompare(

  double         A,   /* I */
  enum MCompEnum Cmp, /* I */
  double         B)   /* I */

  {
  int val;

  switch (Cmp)
    {
    case mcmpGT:

      val = (A > B);

      break;

    case mcmpGE:

      val = (A >= B);

      break;

    case mcmpEQ:

      val = (A == B);

      break;

    case mcmpLE:

      val = (A <= B);

      break;

    case mcmpLT:

      val = (A < B);

      break;

    case mcmpNE:

      val = (A != B);

      break;

    case mcmpNONE:

      val = 1;

      break;

    default:

      val = 0;

      break;
    }  /* END switch (Cmp) */

  return(val);
  }  /* END MUDCompare() */



/**
 * Expands the compressed number (64K) to the respective number.
 *
 * @param NumStr The string with the compressed number.
 * @return Returns the expanded number.
 */

int MUExpandKNumber(

  char *NumStr)

  {
  if (NumStr == NULL)
    {
    return(FAILURE);
    }

  if (MUStrChr(NumStr,'K') == NULL)
    {
    return(strtol(NumStr,NULL,10));
    }

  return(strtol(NumStr,NULL,10) * 1024);
  } /* END int MUExpandKNumber() */



/*  Check integer value against list of valid values.  
 *  Returns SUCCESS if value in list or FAILURE otherwise.
 *
 * Example: List = "1-3,9,18", Value=4 return FAILURE.
 *
 * @param Value (I)
 * @param List (I)
 * @param delim (I)

 */

int MUIsIntValueInList(

  int         Value,
  char       *List,
  const char *delim)

  {
  char *TokPtr = NULL;
  char *ptr;
  char buffer[MMAX_LINE];

  if (List == NULL)
    return FAILURE;

  MUStrCpy(buffer,List,MMAX_LINE);

  /* Traverse comma deliminated list evaluating each value */

  ptr = MUStrTok(buffer,delim,&TokPtr);
  while (ptr != NULL)
    {
    int minValue;
    int maxValue;
    char *tmp;
    char *TokPtr2 = NULL;

    /* Parse range, like "0-3" */

    tmp = MUStrTok(ptr,"-",&TokPtr2);
    if ((tmp != NULL) && (TokPtr2 != NULL) && (TokPtr2[0] != '\0'))
      {
      /* Check value in range. */

      if ((!MUStrHasAlpha(tmp)) && (!MUStrHasAlpha(TokPtr2)))
        {
        minValue = atoi(tmp);
        maxValue = atoi(TokPtr2);
        if ((Value >= minValue) && (Value <= maxValue))
          return (SUCCESS);
        }
      }
    else
      {
      /* Check single value */

      if (!MUStrHasAlpha(ptr))
        {
        minValue = atoi(ptr);
        if(Value == minValue)
          return (SUCCESS);
        }
      }

    ptr = MUStrTok(NULL,delim,&TokPtr);
    } /* END MUStrTok */

  return (FAILURE);
  } /* END MUIsIntValueInList */


/**
 * Allocates memory for an int, sets the mem equal
 * to the value of Src, and returns a pointer to the
 * allocated memory.
 *
 * @param Src (I)
 *
 * @return Pointer to a newly allocated int. CALLER MUST FREE!
 *
 */

int *MUIntDup(

  int Src)

  {
  int *Result;

  Result = (int *)malloc(sizeof(int));

  *Result = Src;

  return(Result);
  }  /* END MUIntDup() */



/**
 * Reverses the endian-ness of a 32-bit value.
 *
 * @param Value (I)
 */

uint32_t MUIntReverseEndian(

  uint32_t Value)  /* I */

  {
  char *ptr;
  char NewValue[sizeof(uint32_t)];
   
  ptr = (char *)&Value;
    
  NewValue[0] = ptr[3];
  NewValue[1] = ptr[2];
  NewValue[2] = ptr[1];
  NewValue[3] = ptr[0];
  
  return(*(uint32_t *)NewValue);
  }  /* END MUIntReverseEndian() */


/* END MUtilNumeric.c */
