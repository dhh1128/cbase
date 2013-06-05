/* HEADER */

      
/**
 * @file MUtilRM.c
 *
 * Contains: RM Utility functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/* resource manager directives */

#define MPBSDIR   "#PBS"
#define MLLDIR    "#@"
#define MGDIR     "#!"
#define MMOABDIR1 "#MOAB"
#define MMOABDIR2 "#MSUB"


/**
 * Return TRUE if string is has a valid RM-directive prefix.
 *
 * @param Line (I)
 * @param Type (I)
 */

mbool_t MUIsRMDirective(

  char             *Line,  /* I */
  enum MRMTypeEnum  Type)  /* I */

  {
  if (Line == NULL)
    {
    return(FALSE);
    }

  switch (Type)
    {
    case mrmtPBS:
    default:

      /* add support for flexible PBS directive detection (i.e. '# PBS', '#PBS', '  #PBS', etc) */

      if ((!strncasecmp(Line,MPBSDIR,strlen(MPBSDIR))) || 
          (!strncasecmp(Line,MMOABDIR1,strlen(MMOABDIR1))) ||
          (!strncasecmp(Line,MMOABDIR2,strlen(MMOABDIR2))))
        {
        return(TRUE);
        }

      return(FALSE);

      /*NOTREACHED*/

      break;
    }  /* END switch (Type) */

  return(FALSE);
  }  /* END MUIsRMDirective() */




/**
 * Strips out RM directives from job submit string.
 *
 * @param SubmitString (I) The job submit string to stip out rm directives [modified]
 * @param Type (I) The type rm directives to remove
 */

int MURemoveRMDirectives(
    
  char             *SubmitString,  /* I (modified) */
  enum MRMTypeEnum  Type)          /* I */
  
  {
  char *linePtr;
  char *TokPtr = NULL;
  char *tmpStr = NULL;

  MUStrDup(&tmpStr,SubmitString);

  SubmitString[0] = '\0';

  linePtr = MUStrTok(tmpStr,"\n",&TokPtr);

  while (linePtr != NULL)
    {
    if (!MUIsRMDirective(linePtr,Type))
      {
      /* strip rm directive by not copying line */

      strcat(SubmitString,linePtr);
      strcat(SubmitString,"\n");
      }

    /* Preserve new lines if FILTERCMDFILE set to FALSE */

    if ((MSched.FilterCmdFile == FALSE) && 
        (TokPtr != NULL) && 
        (TokPtr[0] == '\n'))
      {
      strcat(SubmitString,"\n");
      }

    linePtr = MUStrTok(NULL,"\n",&TokPtr);
    }  /* END while (linePtr != NULL) */

  MUFree(&tmpStr);

  return(SUCCESS);
  }  /* END MURemoveRMDirectives() */



/**
 *
 *
 * @param SrcBuffer (I)
 * @param DstLine (O) [minsize=MMAX_LINE]
 */

char *MURMScriptExtractShell(
  
  char *SrcBuffer,  /* I */
  char *DstLine)    /* O (minsize=MMAX_LINE) */

  {
  char  tmpScript[MMAX_BUFFER];

  char *ptr;
  char *TokPtr;

  if (DstLine == NULL)
    {
    return(NULL);
    }

  /* default value */

  snprintf(DstLine,MMAX_LINE,"#!%s",
    MDEF_SHELL);

  if (SrcBuffer == NULL)
    {
    return(DstLine);
    }

  MUStrCpy(tmpScript,SrcBuffer,sizeof(tmpScript));

  ptr = MUStrTok(tmpScript,"\n",&TokPtr);

  if ((ptr[0] == '#') && (ptr[1] == '!'))
    {
    snprintf(DstLine,MMAX_LINE,"#!%s",
      &ptr[2]);
    }

  return(DstLine);
  }  /* END MURMScriptExtractShell() */





/**
 * Filters out all rm directives that not of RMType.
 *
 * @param SrcBuffer (I)
 * @param RMType (I)
 * @param DstBuffer (O)
 * @param DstBufSize (I)
 */

char *MURMScriptFilter(

  char             *SrcBuffer,  /* I */
  enum MRMTypeEnum  RMType,     /* I */
  char             *DstBuffer,  /* O */
  int               DstBufSize) /* I */

  {
  char *BPtr = NULL;
  int   BSpace = 0;

  char *ptr;
  char *TokPtr;

  if ((SrcBuffer == NULL) || (DstBuffer == NULL))
    {
    return(FAILURE);
    }

  MUSNInit(&BPtr,&BSpace,DstBuffer,DstBufSize);

  ptr = MUStrTok(SrcBuffer,"\n",&TokPtr);

  while (ptr != NULL)
    {
    if ((MUIsRMDirective(ptr,RMType) == FALSE) && 
        (MUIsRMDirective(ptr,mrmtOther) == FALSE))
      {
      MUSNPrintF(&BPtr,&BSpace,"%s\n",
        ptr);
      }

    ptr = MUStrTok(NULL,"\n",&TokPtr);
    }  /* END while (ptr != NULL) */

  return(DstBuffer);
  }  /* END MURMScriptFilter() */

/* END MUtilRM.c */
