/* HEADER */

/**
 * @file MUIsStringA.c
 *
 * A set of functions that perform various tests on a given string
 * to return if that string IsA status
 */

#include "moab.h"
#include "moab-proto.h"
#include "moab-const.h"
#include "moab-global.h"



/**
 * Verifies whether the given string is a valid regular expression or not.
 *
 * @param String (I)
 * @return TRUE if string is 'ALL' or a regular expression.
 */

mbool_t MUStringIsRE(

  char *String) /* I */

  {
  int index;

  const char *Delim = "[]()*^$,";

  if (!strcmp(String,ALL) ||
      !strcmp(String,"ALL"))
    {
    return(TRUE);
    }

  if (!strncasecmp(String,"x:",strlen("x:")))
    {
    return(TRUE);
    }

  if ((String[0] == ',') || (String[strlen(String) - 1] == ','))
    {
    /* This is a special case.  The regex routines will match these
       to EVERYTHING and that could result in a very bad typo, for now
       just return that this is NOT regex (MOAB-1033) */

    return(FALSE);
    }

  for (index = 0;Delim[index] != '\0';index++)
    {
    if (strchr(String,Delim[index]))
      {
      return(TRUE);
      }
    }    /* END for (index) */

  return(FALSE);
  } /* END MUStringIsRE() */




/**
 * Verifies whether the given string appears to be XML or not.
 *
 * @param String (I)
 * @return TRUE if string appears to be XML.
 */

mbool_t MUStringIsXML(

  char *String) /* I */

  {
  int index;

  if (String == NULL)
    {
    return(FALSE);
    }

  for (index = 0;String[index] != '\0';index++)
    {
    if (!isspace(String[index]))
      break;
    }  /* END for (index) */

  if (String[index] != '<')
    {
    return(FALSE);
    }

  return(TRUE);
  }  /* END MUStringIsXML() */




/**
 * Detects whether a string is a valid base 10 number or not.
 *
 * @param String The string that is checked to see if it is a number.
 * @return TRUE if the string is a number, false otherwise.
 */

mbool_t MUIsNum(

  char *String)

  {
  char *EPtr;

  if ((strtol(String,&EPtr,10) == 0) &&
      (*EPtr != '\0'))
    {
    /* not a number */

    return(FALSE);
    }

  return(TRUE);
  }  /* END MUIsNum() */


/**
 * MUIsOnlyNumeric tells you if a string has any non-numeric characters in it
 *
 * @param TString (I)- a string that is supposed to represent some part of time
 * @return TRUE if every character in TString is a digit
 */

mbool_t MUIsOnlyNumeric(

  char *TString) /* I */

  {
  int Len;
  int StrIndex;

  /* check for a NULL string */
  if (TString == NULL)
    return(FALSE);

  Len = strlen(TString);

  for (StrIndex = 0;StrIndex < Len;StrIndex++)
    {
    if(!isdigit(TString[StrIndex]))
      {
      return(FALSE);
      }
    }

  return(TRUE);
  }  /* END MUIsOnlyNumeric() */

/* END MUIsStringA */
