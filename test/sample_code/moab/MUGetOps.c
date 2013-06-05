/* HEADER */

/**
 * @file MUGetOps.c
 *
 * MUGetPair() and MUGetPairCI() functions
 * MUGetIndex(), MUGetIndex2(), MUGetIndexCI() functions
 *
 */

#include "moab.h"
#include "moab-proto.h"
#include "moab-const.h"
#include "moab-global.h"



/**
 * Parse string into parameter, index, comparison, and value components.
 *
 * NOTE: String to AttrName comparison is case insensitive if AttrName[0] is all uppercase
 *
 * NOTE: fail if attr is unknown or ValLine is too small
 *
 * @see MUGetPairCI() - peer
 * @see MUGetIndex() - child
 *
 * @param String (I)
 * @param AttrName (I) array of potential attribute names
 * @param AttrIndex (O)
 * @param AttrArray (O) [optional,minsize=MMAX_NAME]
 * @param CmpRelative (I)
 * @param CmpMode (O) [optional]
 * @param ValLine (O)
 * @param ValSize (I)
 */

int MUGetPair(

  char        *String,       /* I */
  const char **AttrName,     /* I - array of potential attribute names */
  int         *AttrIndex,    /* O */ 
  char        *AttrArray,    /* O (optional,minsize=MMAX_NAME) */
  mbool_t      CmpRelative,  /* I */
  int         *CmpMode,      /* O (optional) */
  char        *ValLine,      /* O */
  int          ValSize)      /* I */

  {
  char *ptr;

  char  tmpLine[MMAX_NAME + 16];  /* parameter component of String (ie, strip off index) */

  int   index;
  int   index2;
  int   aindex1 = 0;  /* marks end of attribute - initialized to handle compiler warning */

  int   CIndex;

  mbool_t DoCI; 

  const char *FName = "MUGetPair";

  MDB(5,fSTRUCT) MLog("%s(%s,AttrName,AttrIndex,AttrArray,%s,CmpMode,ValLine,%d)\n",
    FName,
    (String != NULL) ? String : "NULL",
    MBool[CmpRelative],
    ValSize);

  if (AttrIndex != NULL)
    *AttrIndex = 0;

  if (AttrArray != NULL)
    AttrArray[0] = '\0';

  if (CmpMode != NULL)
    *CmpMode = 0;

  if (ValLine != NULL)
    ValLine[0] = '\0';

  if ((String == NULL) ||
      (String[0] == '\0') ||
      (AttrName == NULL) ||
      (AttrName[0] == NULL) ||
      (AttrIndex == NULL) ||
      (ValLine == NULL) ||
      (ValSize <= 0))
    {
    return(FAILURE);
    }

  /* FORMAT:  [<WS>]<ATTRIBUTE>[\[<INDEX>\]][<WS>]<CMP>[<WS>]<VAL>[<WS>] */
  /* FORMAT:  <CMP>: =,==,+=,-= (relative) */

  ptr = String;

  /* remove leading WS */

  while (isspace(ptr[0]))
    {
    ptr++;
    }

  /* load attribute */

  for (index = 0;index < MMAX_NAME;index++)
    {  
    if (ptr[index] == '\0')
      {
      /* no comp or value specified */

      return(FAILURE);
      }

    if (CmpRelative == TRUE)
      {
      if (isspace(ptr[index]) || 
         (ptr[index] == '=') ||
         (ptr[index] == '+') ||
         (ptr[index] == '-'))
        {
        aindex1 = index;

        break;
        }
      }
    else
      {
      if (isspace(ptr[index]) ||
         (ptr[index] == '!') ||
         (ptr[index] == '=') ||
         (ptr[index] == '>') ||
         (ptr[index] == '+') ||
         (ptr[index] == '-') ||
         (ptr[index] == '<'))
        {
        aindex1 = index;

        break;
        }
      }  /* END else (CmpRelative == TRUE) */

    if (ptr[index] == '[')
      {
      int aindex2 = 0;

      aindex1     = index;

      /* attr index located */

      tmpLine[index] = '\0';
      
      for (index = index + 1;index < MMAX_NAME;index++)
        {
        if ((ptr[index] == ']') || (ptr[index] == '\0'))
          break;

        if (AttrArray != NULL)
          {
          AttrArray[aindex2] = ptr[index];
       
          aindex2++;
          }
        }  /* END for (index) */

      if (AttrArray != NULL)
        AttrArray[aindex2] = '\0';

      index++;

      break;
      }  /* END if (ptr[index] == '[') */

    tmpLine[index] = ptr[index];
    aindex1        = index;
    }  /* END for (index) */

  tmpLine[index] = '\0';
  ptr += index;

  /* case-sensitivity based on case of 'String' parameter */

  DoCI = FALSE;

  for (index2 = 0;index2 < aindex1;index2++)
    {
    if (islower(tmpLine[index2]))
      {
      DoCI = TRUE;

      break;
      }
    }  /* END for (index2) */

  if (DoCI == TRUE)
    *AttrIndex = MUGetIndexCI(tmpLine,AttrName,FALSE,0);
  else
    *AttrIndex = MUGetIndex(tmpLine,AttrName,FALSE,0);
 
  /* remove whitespace */

  while (isspace(ptr[0]))
    ptr++;

  if (CmpRelative == TRUE)
    {
    /* FORMAT: { = | += | -= } */

    if (strchr("=+-",ptr[0]) == NULL)
      {
      return(FAILURE);
      }

    switch (ptr[0])
      {
      case '+':

        CIndex = 1;

        break;

      case '-':

        CIndex = -1;

        break;

      default:

        CIndex = 0;

        break;
      }  /* END switch (ptr[0]) */

    ptr += 1;
    }  /* END if (CmpRelative == TRUE) */
  else
    {
    /* FORMAT:  { != | > | >= | = | <= | < | += | -= } */

    if (strchr("!><=+-",ptr[0]) == NULL)
      {
      return(FAILURE);
      }

    CIndex = MUCmpFromString(ptr,&index);

    ptr += index;
    }  /* END else (CmpRelative == TRUE) */

  if (CmpMode != NULL)
    *CmpMode = CIndex;

  if (ptr[0] == '=')
    {
    ptr++;
    }
    
  /* remove whitespace */

  while (isspace(ptr[0]))
    ptr++;

  /* load value */

  if (MUStrCpyQuoted(ValLine,ptr,ValSize) == FAILURE)
    {

      /* value buffer too small */

      /* NOTE:  will not return failure until Moab 5.2 (5.2, eh?) */

      /* return(FAILURE); */
    }

  if (*AttrIndex == 0)
    {
    /* cannot process attr name */

    *AttrIndex = -1;

    return(FAILURE);
    }
    
  return(SUCCESS);   
  }  /* END MUGetPair() */






/**
 * Parse string into parameter, index, comparison, and value components.
 *
 * NOTE: String to AttrName comparison is case insensitive
 *
 * NOTE: fail if attr is unknown or ValLine is too small
 *
 * @see MUGetPair() - peer
 *
 * @param String (I)
 * @param AttrName (I) array of potential attribute names
 * @param AttrIndex (O)
 * @param AttrArray (O) [optional,minsize=MMAX_NAME]
 * @param CmpRelative (I)
 * @param CmpMode (O) [optional]
 * @param ValLine (O)
 * @param ValSize (I)
 */

int MUGetPairCI(

  char        *String,       /* I */
  const char **AttrName,     /* I - array of potential attribute names */
  int         *AttrIndex,    /* O */
  char        *AttrArray,    /* O (optional,minsize=MMAX_NAME) */
  mbool_t      CmpRelative,  /* I */
  int         *CmpMode,      /* O (optional) */
  char        *ValLine,      /* O */
  int          ValSize)      /* I */

  {
  char *ptr;

  char  tmpLine[MMAX_NAME + 4];

  int   index;

  int   CIndex;

  int   vindex;

  mbool_t SingleQuoteBounding;
  mbool_t DoubleQuoteBounding;

  const char *FName = "MUGetPairCI";

  MDB(5,fSTRUCT) MLog("%s(%s,AttrName,AttrIndex,AttrArray,%s,CmpMode,ValLine,%d)\n",
    FName,
    (String != NULL) ? String : "NULL",
    MBool[CmpRelative],
    ValSize);

  if (AttrIndex != NULL)
    *AttrIndex = 0;

  if (AttrArray != NULL)
    AttrArray[0] = '\0';

  if (CmpMode != NULL)
    *CmpMode = 0;

  if (ValLine != NULL)
    ValLine[0] = '\0';

  if ((String == NULL) ||
      (String[0] == '\0') ||
      (AttrName == NULL) ||
      (AttrName[0] == NULL) ||
      (AttrIndex == NULL) ||
      (ValLine == NULL) ||
      (ValSize <= 0))
    {
    return(FAILURE);
    }

  /* FORMAT:  [<WS>]<ATTRIBUTE>[\[<INDEX>\]][<WS>]<CMP>[<WS>]<VAL>[<WS>] */
  /* FORMAT:  <CMP>: =,==,+=,-= (relative) */

  ptr = String;

  /* remove leading WS */

  while (isspace(ptr[0]))
    {
    ptr++;
    }

  /* load attribute */

  for (index = 0;index < MMAX_NAME;index++)
    {
    if (ptr[index] == '\0')
      {
      /* no comp or value specified */

      return(FAILURE);
      }

    if (CmpRelative == TRUE)
      {
      if (isspace(ptr[index]) ||
         (ptr[index] == '=') ||
         (ptr[index] == '+') ||
         (ptr[index] == '-'))
        {
        break;
        }
      }
    else
      {
      if (isspace(ptr[index]) ||
         (ptr[index] == '!') ||
         (ptr[index] == '=') ||
         (ptr[index] == '>') ||
         (ptr[index] == '+') ||
         (ptr[index] == '-') ||
         (ptr[index] == '<'))
        {
        break;
        }
      }  /* END else (CmpRelative == TRUE) */

    if (ptr[index] == '[')
      {
      int aindex2 = 0;

      /* attr index located */

      tmpLine[index] = '\0';

      for (index = index + 1;index < MMAX_NAME;index++)
        {
        if ((ptr[index] == ']') || (ptr[index] == '\0'))
          break;

        if (AttrArray != NULL)
          {
          AttrArray[aindex2] = ptr[index];

          aindex2++;
          }
        }  /* END for (index) */

      if (AttrArray != NULL)
        AttrArray[aindex2] = '\0';

      index++;

      break;
      }  /* END if (ptr[index] == '[') */

    tmpLine[index] = ptr[index];
    }  /* END for (index) */

  tmpLine[index] = '\0';
  ptr += index;

  *AttrIndex = MUGetIndexCI(tmpLine,AttrName,FALSE,0);

  /* remove whitespace */

  while (isspace(ptr[0]))
    ptr++;

  if (CmpRelative == TRUE)
    {
    /* FORMAT: { = | += | -= } */

    if (strchr("=+-",ptr[0]) == NULL)
      {
      return(FAILURE);
      }

    switch (ptr[0])
      {
      case '+':

        CIndex = 1;

        break;

      case '-':

        CIndex = -1;

        break;

      default:

        CIndex = 0;

        break;
      }  /* END switch (ptr[0]) */

    ptr += 1;
    }
  else
    {
    /* FORMAT:  { != | > | >= | = | <= | < | += | -= } */

    if (strchr("!><=+-",ptr[0]) == NULL)
      {
      return(FAILURE);
      }

    CIndex = MUCmpFromString(ptr,&index);

    ptr += index;
    }  /* END else (CmpRelative == TRUE) */

  if (CmpMode != NULL)
    *CmpMode = CIndex;

  if (ptr[0] == '=')
    {
    ptr++;
    }

  /* remove whitespace */

  while (isspace(ptr[0]))
    ptr++;

  /* load value */

  SingleQuoteBounding = FALSE;
  DoubleQuoteBounding = FALSE;

  vindex = 0;

  if (ptr[0] == '\'')
    {
    SingleQuoteBounding = TRUE;
    ptr++;
    }
  else if (ptr[0] == '\"')
    {
    DoubleQuoteBounding = TRUE;
    ptr++;
    }

  for (index = 0;index < ValSize - 1;index++)
    {
    if (ptr[index] == '\0')
      break;

    if ((SingleQuoteBounding == TRUE) && (ptr[index] == '\''))
      {
      break;
      }

    if ((DoubleQuoteBounding == TRUE) && (ptr[index] == '\"'))
      {
      break;
      }

    ValLine[vindex++] = ptr[index];
    }  /* END for (index) */

  ValLine[vindex] = '\0';

  if (vindex >= ValSize - 1)
    {
    if (ptr[index] != '\0')
      {
      /* value buffer too small */

      /* NOTE:  will not return failure until Moab 5.2 */

      /* return(FAILURE); */
      }
    }

  if (*AttrIndex == 0)
    {
    /* cannot process attr name */

    *AttrIndex = -1;

    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MUGetPairCI() */


/**
 * Limited functionality of MUGetIndex() with AllowSubstring always TRUE and '.'
 * as a valid delimiter.
 *
 * @param Value (I)
 * @param ValList (I)
 * @param DefaultValue (I)
 */

int MUGetIndex2(

  char        *Value,           /* I */
  const char **ValList,         /* I */
  int          DefaultValue)    /* I */

  {
  int index;

  int len;

  const char *FName = "MUGetIndex2";

  MDB(5,fSTRUCT) MLog("%s(%s,%s,%d)\n",
    FName,
    (Value != NULL) ? Value : "NULL",
    (ValList != NULL) ? "ValList" : "NULL",
    DefaultValue);

  if (ValList == NULL)
    {
    return(DefaultValue);
    }

  if (Value == NULL)
    {
    return(DefaultValue);
    }

  for (index = 0;ValList[index] != NULL;index++)
    {
    if (Value[0] != ValList[index][0])
      continue;
  
    len = strlen(ValList[index]);

    if (!strncmp(Value,ValList[index],len) &&
        ((Value[len] == '.') || (Value[len] == '\0')))
      {
      /* SUCCESS */

      return(index);
      }
    }    /* END for (index) */

  return(DefaultValue);
  }  /* END MUGetIndex2() */





/**
 * Report index of matching entry in specified list.
 *
 * NOTE: MUGetIndex2() is the same as this routine but it allows '.' as a delimiter.  Any changes
 *       made here must be made in MUGetIndex2().
 *
 * @param Value (I)
 * @param ValList (I)
 * @param AllowSubstring (I) [MBNOTSET allows delimiter based substring]
 * @param DefaultValue (I)
 */

int MUGetIndex(

  char        *Value,           /* I */
  const char **ValList,         /* I */
  mbool_t      AllowSubstring,  /* I (MBNOTSET allows delimiter based substring) */
  int          DefaultValue)    /* I */

  {
  int index;

  const char *FName = "MUGetIndex";

  MDB(5,fSTRUCT) MLog("%s(%s,%s,%s,%d)\n",
    FName,
    (Value != NULL) ? Value : "NULL",
    (ValList != NULL) ? "ValList" : "NULL",
    MBool[AllowSubstring],
    DefaultValue);

  if (ValList == NULL)
    {
    return(DefaultValue);
    }

  if (Value == NULL)
    {
    return(DefaultValue);
    }

  for (index = 0;ValList[index] != NULL;index++)
    {
    if (Value[0] != ValList[index][0])
      continue;

    if ((AllowSubstring == FALSE) &&
        (!strcmp(Value,ValList[index])))
      {
      /* SUCCESS */

      return(index);
      }
    else if ((AllowSubstring == TRUE) &&
             (!strncmp(Value,ValList[index],strlen(ValList[index]))))
      {
      /* SUCCESS */

      return(index);
      }
    else if (AllowSubstring == MBNOTSET)
      {
      int len = strlen(ValList[index]);

      if (!strncmp(Value,ValList[index],len) &&
         (strchr(" \t\n=<>,.:;|%",Value[len]) || (Value[len] == '\0')))
        {
        /* SUCCESS */

        return(index);
        }
      }
    }    /* END for (index) */

  return(DefaultValue);
  }  /* END MUGetIndex() */





/**
 * Searches for the given Value in the ValList. If found, the appropriate enum mapping is returned.
 * If the Value is not found in the ValList, then the DefaultValue is returned. The search is CASE-INSENSITIVE.
 *
 * @param Value (I)
 * @param ValList (I)
 * @param AllowSubstring (I) [TRUE, FALSE, or MBNOTSET=allow delimiter substring]
 * @param DefaultValue (I)
 *
 * @return The enum mapping corresponding to Value, otherwise it returns the given DefaultValue.
 */

int MUGetIndexCI(

  char const  *Value,           /* I */
  const char **ValList,         /* I */
  mbool_t      AllowSubstring,  /* I (TRUE, FALSE, or MBNOTSET=allow delimiter substring) */
  int          DefaultValue)    /* I */

  {
  char tmpLine[MMAX_LINE] = {0};
  char tmpLVal[MMAX_LINE] = {0};

  int  index;

  int  len;

  const char *FName = "MUGetIndexCI";

  MDB(5,fSTRUCT) MLog("%s(%s,%s,%s,%d)\n",
    FName,
    (Value != NULL) ? Value : "NULL",
    (ValList != NULL) ? "ValList" : "NULL",
    MBool[AllowSubstring],
    DefaultValue);

  if (ValList == NULL)
    {
    return(DefaultValue);
    }

  if ((Value == NULL) || (Value[0] == '\0'))
    {
    return(DefaultValue);
    }

  MUStrToUpper((char *)Value,tmpLine,sizeof(tmpLine));

  for (index = 0;ValList[index] != NULL;index++)
    {
    if (tmpLine[0] != toupper(ValList[index][0]))
      {
      /* quick test - first char does not match */

      continue;
      }

    MUStrToUpper((char *)ValList[index],tmpLVal,sizeof(tmpLVal));

    if ((AllowSubstring == FALSE) &&
        (!strcmp(tmpLine,tmpLVal)))
      {
      /* SUCCESS */

      return(index);
      }
    else if ((AllowSubstring == TRUE) &&
             (!strncmp(tmpLine,tmpLVal,strlen(tmpLVal))))
      {
      /* SUCCESS */

      return(index);
      }
    else if (AllowSubstring == MBNOTSET)
      {
      len = strlen(tmpLVal);

      if (!strncmp(tmpLine,tmpLVal,len) &&
         (strchr(" \t\n=<>,:;|",tmpLine[len]) || (tmpLine[len] == '\0')))
        {
        /* SUCCESS */

        return(index);
        }
      }
    }    /* END for (index) */

  return(DefaultValue);
  }  /* END MUGetIndexCI() */


/**
 * Returns ptr to beginning of first non-white space and a second ptr to 
 * second non-white space followed by white space.
 *
 * @param String (I)
 * @param Head (O)
 * @param Next (O)
 */

int MUGetWord(

  char  *String,  /* I */
  char **Head,    /* O */
  char **Next)    /* O */

  {
  char *ptr;

  if (Head != NULL)
    *Head = NULL;

  if (Next != NULL)
    *Next = NULL;

  if (String == NULL)
    {
    return(FAILURE);
    }

  ptr = String;

  /* traverse whitespace */

  while (isspace(*ptr) && (*ptr != '\0'))
    ptr++;

  if (*ptr == '\0')
    {
    return(FAILURE);
    }

  if (Head != NULL)
    *Head = ptr;

  /* traverse word */

  while (!isspace(*ptr) && (*ptr != '\0') && (*ptr != '\n'))
    ptr++;

  /* traverse whitespace */

  while (isspace(*ptr) && (*ptr != '\0') && (*ptr != '\n'))
    ptr++;

  if (Next != NULL)
    *Next = ptr;

  return(SUCCESS);
  }  /* END MUGetWord() */


/**
 * Getter function for:  GRes, GMetric, 
 */

int MUGenericGetIndex(

  enum MExpAttrEnum       AIndex,
  char const             *Value,
  enum MObjectSetModeEnum Mode)

  {
  /* determine index of up to MMAX_ATTR attribute values */

  int index;
  int len;

  const char *FName = "MUGenericGetIndex";

  MDB(9,fSTRUCT) MLog("%s(%s,%s,%s)\n",
    FName,
    MAttrType[AIndex],
    (Value != NULL) ? Value : "NULL",
    MObjOpType[Mode]);

  if (Value == NULL)
    {
    /* FAILURE */

    return(0);
    }

  switch (AIndex)
    {
    case meGRes:
   
      len = MMAX_NAME;

      for (index = 1;index < MSched.M[mxoxGRes];index++)
        {
        if (MGRes.Name[index][0] == '\0')
          break;

        if (!strncasecmp(MGRes.Name[index],Value,len))
          {
          return(index);
          }
        }  /* END for (index) */

      if (index == MSched.M[mxoxGRes])
        {
        if (Mode == mAdd)
          {
          /* not enough space, record value to "overflow" table so admins can see it with "mdiag -S" */

          if (MSched.OFOID[mxoxGRes] == NULL)
            MUStrDup(&MSched.OFOID[mxoxGRes],Value);
 
          MDB(1,fCONFIG) MLog("ALERT:    no empty slots for %s\n",
            Value);
          }

        /* FAILURE */

        return(0);
        }

      break;

    case meGMetrics:

      len = MMAX_NAME;

      for (index = 1;index < MSched.M[mxoxGMetric];index++)
        {
        if ((MGMetric.Name[index] == NULL) || 
            (MGMetric.Name[index][0] == '\0'))
          break;

        if (!strncasecmp(MGMetric.Name[index],Value,len))
          {
          return(index);
          }
        }  /* END for (index) */

      if (index == MSched.M[mxoxGMetric])
        {
        if (Mode == mAdd)
          {
          /* not enough space, record value to "overflow" table so admins can see it with "mdiag -S" */

          if (MSched.OFOID[mxoxGMetric] == NULL)
            MUStrDup(&MSched.OFOID[mxoxGMetric],Value);

          MDB(1,fCONFIG) MLog("ALERT:    no empty slots for %s\n",
            Value);
          }

        /* FAILURE */

        return(0);
        }

      break;

    default: 
      
      return(0);

      /* NOTREACHED */

      break;
    }

  if (Mode == mVerify)
    {
    /* FAILURE */

    return(0);
    }

  /* add new value to table */

  switch (AIndex)
    {
    case meGRes:

      MSched.GResCount = MAX(MSched.GResCount,index);

      /* We must initialize the MGRes here when a client command
          tries to search for a GRes index.  It will already have
          been initialized on the scheduler side. */

      MGResInitialize();

      MUStrCpy(MGRes.Name[index],Value,MMAX_NAME);

      MDB(5,fCONFIG) MLog("INFO:     adding MGRes.Name[%d]: '%s'\n",
        index,
        Value);

      break;

    case meGMetrics:

      MUStrCpy(MGMetric.Name[index],Value,MMAX_NAME);

      MDB(5,fCONFIG) MLog("INFO:     adding MGMetric.Name[%d]: '%s'\n",
        index,
        Value);

      break;

    default:

      /* NO-OP */

      break;
    }   /* END switch (AIndex) */

  /* SUCCESS */

  return(index);
  }   /* END MUGenericGetIndex() */
/* END MUGetOps.c */
