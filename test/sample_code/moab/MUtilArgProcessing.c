/* HEADER */

      
/**
 * @file MUtilArgProcessing.c
 *
 * Contains: Argument processing utilities
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 *
 *
 * @param AList (I) [modified,minsize=MMAX_ARG]
 * @param Arg (I)
 * @param RemoveOption (I)
 */

int MArgListRemove(

  char        **AList,
  const char   *Arg,
  mbool_t       RemoveOption)

  {
  int aindex;
  int offset;

  if ((AList == NULL) || (Arg == NULL))
    {
    return(FAILURE);
    }

  for (aindex = 0;aindex < MMAX_ARG;aindex++)
    {
    if (AList[aindex] == NULL)
      break;

    if (!strcmp(AList[aindex],Arg))
      {
      MUFree(&AList[aindex]);

      if ((RemoveOption == TRUE) && (AList[aindex + 1] != NULL) && (AList[aindex + 1][0] != '-'))
        {
        MUFree(&AList[aindex + 1]);

        offset = 2;
        }
      else
        {
        offset = 1;
        }

      for (;aindex < MMAX_ARG;aindex++)
        {
        AList[aindex] = AList[aindex + offset];

        if (AList[aindex] == NULL)
          {
          return(SUCCESS);
          }
        }
      }
    }    /* END for (aindex) */

  return(SUCCESS);
  }  /* END MArgListRemove() */




/**
 *
 *
 * @param AList (I) [modified,minsize=MMAX_ARG]
 * @param Arg1 (I)
 * @param Arg2 (I) [optional]
 */

int MArgListAdd(

  char       **AList,
  const char  *Arg1,
  char        *Arg2)

  {
  int aindex;

  if ((AList == NULL) || (Arg1 == NULL))
    {
    return(FAILURE);
    }

  for (aindex = 0;aindex < MMAX_ARG;aindex++)
    {
    if (AList[aindex] == NULL)
      {
      /* add args to end of list */

      MUStrDup(&AList[aindex],Arg1);
      AList[aindex + 1] = NULL;

      aindex++;

      if (aindex >= MMAX_ARG)
        break;

      if (Arg2 != NULL)
        {
        MUStrDup(&AList[aindex],Arg2);
        AList[aindex + 1] = NULL;
        }

      break;
      }
 
    if (!strcmp(AList[aindex],Arg1))
      {
      MArgListRemove(
        AList,
        Arg1,
        (Arg2 != NULL) ? TRUE : FALSE);

      MArgListAdd(AList,Arg1,Arg2);

      return(SUCCESS);
      }
    }    /* END for (aindex) */

  return(SUCCESS);
  }  /* END MArgListAdd() */



/**
 * Finds and returns the index of an argument in the parseline.
 *
 * Only finds characters, doesn't find options that are strings.
 *
 * NOTE:  NOT THREAD-SAFE
 * NOTE:  Output value truncated at MMAX_BUFFER 
 * NOTE:  Routine modifies ArgC/ArgV removing args as processed
 *
 * @param ArgC (I) argc pointer
 * @param ArgV (I) argv
 * @param ParseLine (I) colon delimited parse string (only the first char matters in each param)
 * @param OptArg (O) [optional,maxsize=MMAX_BUFFER]
 * @param Tok (I/O) position token
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 */

int MUGetOpt(

  int        *ArgC,      /* I argc pointer     */
  char      **ArgV,      /* I argv             */
  const char *ParseLine, /* I parse string     */
  char      **OptArg,    /* O (optional,maxsize=MMAX_BUFFER) */
  int        *Tok,       /* I/O position token */
  char       *EMsg)      /* O (optional,minsize=MMAX_LINE) */

  {
  int   flag;
  int   index;
  int   aindex;

  int   AStart;

  char *ptr;

  static char ArgVal[MMAX_BUFFER << 2];  /* allow up to 256 KB */

  const char *FName = "MUGetOpt";

  MDB(3,fCONFIG) MLog("%s(%d,ArgV,%s,OptArg,Tok,EMsg)\n",
    FName,
    *ArgC,
    ParseLine);

  /* NOTE:  extract requested args, ignore others */

  if (EMsg != NULL)
    EMsg[0] = '\0';

  flag = -1;

  *OptArg = NULL;

  if (*ArgC == 0)
    {
    /* SUCCESS */

    /* no arguments remain */

    return(flag);
    }

  if (Tok != NULL)
    AStart = *Tok;
  else
    AStart = 0;

  for (aindex = AStart;ArgV[aindex] != NULL;aindex++)
    {
    if (ArgV[aindex][0] != '-')
      {
      /* arg is not flag, ignore */

      if (Tok != NULL)
        (*Tok)++;

      continue;
      }

    if (isdigit(ArgV[aindex][1]))
      {
      /* negative number specified, ignore */

      continue;
      }

    /* if flag is not in ParseLine, return '?' */

    if (!(ptr = (char *)strchr(ParseLine,ArgV[aindex][1])))
      {
      if (MUStrCpy(ArgVal,ArgV[aindex],sizeof(ArgVal)) == FAILURE)
        {
        MDB(3,fCONFIG) MLog("ALERT:    flag '%c' arg is too large (exceeds %d character max)\n",
          (char)flag,
          (int)sizeof(ArgVal));

        if (EMsg != NULL)
          {
          sprintf(EMsg,"flag '%c' arg is too large (exceeds %d character max)\n",
            (char)flag,
            (int)sizeof(ArgVal));
          }

        /* FAILURE */

        return('?');        
        }

      if (EMsg != NULL)
        {
        sprintf(EMsg,"flag '%c' is not valid\n",
          (char)flag);
        }

      *OptArg = ArgVal;

      if (Tok != NULL)
        (*Tok)++;

      /* FAILURE */

      return('?');
      }  /* END if (!(ptr = strchr(ParseLine,ArgV[aindex][1]))) */

    flag = (int)ArgV[aindex][1];

    MDB(3,fCONFIG) MLog("INFO:     flag '%c' detected\n",
      (char)flag);

    /* mark arg for removal */

    ArgV[aindex][0] = '\1';

    (*ArgC)--;
 
    if (*(ptr + 1) == ':')
      {
      /* flag value expected */

      if (ArgV[aindex][2] != '\0')
        {
        /* if flag value contained in flag argument */

        MDB(3,fCONFIG) MLog("INFO:     arg '%s' found for flag '%c'\n",
          &ArgV[aindex][2],
          (char)flag);

        if (MUStrCpy(ArgVal,&ArgV[aindex][2],sizeof(ArgVal)) == FAILURE)
          {
          /* FAILURE */

          MDB(3,fCONFIG) MLog("ALERT:    flag '%c' arg is too large (exceeds %d character max)\n",
            (char)flag,
            (int)sizeof(ArgVal));

          if (EMsg != NULL)
            {
            sprintf(EMsg,"flag '%c' arg is too large (exceeds %d character max)\n",
              (char)flag,
              (int)sizeof(ArgVal));
            }

          return('?');
          }

        *OptArg = ArgVal;
        }
      else if ((aindex < *ArgC) && 
               (ArgV[aindex + 1] != NULL) &&
               ((ArgV[aindex + 1][0] != '-') || isdigit(ArgV[aindex + 1][1]) || (ArgV[aindex + 1][1] == '=')))
        {
        /* if flag value contained in next arg */

        MDB(3,fCONFIG) MLog("INFO:     arg '%s' found for flag '%c'\n",
          ArgV[aindex + 1],
          (char)flag);

        if (MUStrCpy(ArgVal,ArgV[aindex + 1],sizeof(ArgVal)) == FAILURE)
          {
          /* FAILURE */

          if (EMsg != NULL)
            {
            sprintf(EMsg,"flag '%c' arg is too large (exceeds %d character max)\n",
              (char)flag,
              (int)sizeof(ArgVal));
            }

          MDB(3,fCONFIG) MLog("ALERT:    flag '%c' arg is too large (exceeds %d character max)\n",
            (char)flag,
            (int)sizeof(ArgVal));

          return('?');
          }

        *OptArg = ArgVal;

        ArgV[aindex + 1][0] = '\1';

        (*ArgC)--;
        }
      else
        {
        /* if flag option not supplied */

        MDB(3,fCONFIG) MLog("INFO:     expected arg not supplied for flag '%c'\n",
          flag);

        ArgVal[0] = '\0';

        *OptArg = ArgVal;
        }
      }    /* END if (*(ptr + 1) == ':') */ 

    /* re-pack all unmarked args */

    for (index = aindex + 1;ArgV[index] != NULL;index++)
      {
      if (ArgV[index][0] == '\1')
        continue;

      ArgV[aindex] = ArgV[index];

      aindex++;
      }  /* END for (index) */

    /* NULL-out remaining args */

    for (;aindex < index;aindex++)
      {
      ArgV[aindex] = NULL;  
      }

    break;
    }  /* END for (aindex) */

  /* SUCCESS */

  return(flag);
  }  /* END MUGetOpt() */


/* END MUtilArgProcessing.c */
