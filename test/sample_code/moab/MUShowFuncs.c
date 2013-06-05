
/* HEADER */

/**
 * @file MUShowFuncs.c
 * 
 * Contains various functions for displaying various itmes
 *
 */


#include "moab.h"
#include "moab-proto.h"
#include "moab-const.h"
#include "moab-global.h"



#define MCFG_HDR_LEN 30


/**
 *
 *
 * @param Parm (I)
 * @param Val (I)
 */

char *MUShowString(

  const char *Parm,    /* I */
  const char *Val)     /* I */

  {
  static char Line[MMAX_LINE];

  if (Parm == NULL)
    {
    Line[0] = '\0';
    }
  else
    {
    sprintf(Line,"%s%-.*s  %s\n",
      Parm,
      MCFG_HDR_LEN - (int)strlen(Parm),
      "                                ",
      (Val != NULL) ? Val : "");
    }

  return(Line);
  }  /* END MUShowString() */




/**
 *
 *
 * @param Parm (I)
 * @param Val (I)
 */

char *MUShowInt(

  const char *Parm,     /* I */
  int        Val)       /* I */

  {
  static char Line[MMAX_LINE];

  if (Parm == NULL)
    {
    Line[0] = '\0';
    }
  else
    {
    sprintf(Line,"%s%-.*s  %d\n",
      Parm,
      MCFG_HDR_LEN - (int)strlen(Parm),
      "                                ",
     Val);
    }

  return(Line);
  }  /* END MUShowInt() */


/**
 *
 *
 * @param Parm (I)
 * @param Val (I)
 */

char *MUShowOctal(

  const char *Parm,     /* I */
  int        Val)       /* I */

  {
  static char Line[MMAX_LINE];

  if (Parm == NULL)
    {
    Line[0] = '\0';
    }
  else
    {
    sprintf(Line,"%s%-.*s  %o\n",
      Parm,
      MCFG_HDR_LEN - (int)strlen(Parm),
      "                                ",
     Val);
    }

  return(Line);
  }  /* END MUShowOctal() */


/**
 *
 *
 * @param Parm (I)
 * @param Val (I)
 */

char *MUShowLong(

  const char *Parm,     /* I */
  long       Val)       /* I */

  {
  static char Line[MMAX_LINE];

  if (Parm == NULL)
    {
    Line[0] = '\0';
    }
  else
    {
    sprintf(Line,"%s%-.*s  %ld\n",
      Parm,
      MCFG_HDR_LEN - (int)strlen(Parm),
      "                                ",
     Val);
    }

  return(Line);
  }  /* END MUShowLong() */




/**
 *
 *
 * @param Parm (I)
 * @param Val (I)
 */

char *MUShowDouble(

  const char *Parm,     /* I */
  double        Val)    /* I */

  {
  static char Line[MMAX_LINE];

  if (Parm == NULL)
    {
    Line[0] = '\0';
    }
  else
    {
    sprintf(Line,"%s%-.*s  %g\n",
      Parm,
      MCFG_HDR_LEN - (int)strlen(Parm),
      "                                ",
     Val);
    }

  return(Line);
  }  /* END MUShowDouble() */




/**
 *
 *
 * @param Parm
 * @param PIndex
 * @param Val
 */

char *MUShowIArray(
 
  const char *Parm,
  int         PIndex,
  int         Val)
 
  {
  static char Line[MMAX_LINE];

  if (Parm == NULL)
    {
    Line[0] = '\0';
    }
  else
    {
    sprintf(Line,"%s[%1d]%-.*s  %d\n",
      Parm,
      PIndex,
      MCFG_HDR_LEN - 3 - (int)strlen(Parm),
      "                                ",
      Val);
    }
 
  return(Line);
  }  /* END MUShowIArray() */
 
 
 

/**
 *
 *
 * @param Param (I)
 * @param IndexName (I)
 * @param LVal (I)
 * @param Buffer (I/O)
 * @param BufSize (I)
 */

char *MUShowSLArray(

  const char *Param,     /* I */
  char       *IndexName, /* I */
  long        LVal,      /* I */
  char       *Buffer,    /* I/O */
  int         BufSize)   /* I */

  {
  static char tmpLine[MMAX_LINE];

  if (Buffer != NULL)
    {
    snprintf(Buffer,BufSize,"%s[%s]%-.*s  %ld\n",
      Param,
      IndexName,
      MCFG_HDR_LEN - 2 - (int)(strlen(Param) + strlen(IndexName)),
      "                                        ",
      LVal);

    return(Buffer);
    }

  snprintf(tmpLine,sizeof(tmpLine),"%s[%s]%-.*s  %ld\n",
    Param,
    IndexName,
    MCFG_HDR_LEN - 2 - (int)(strlen(Param) + strlen(IndexName)),
    "                                        ",
    LVal);

  return(tmpLine);
  }  /* END MUShowSLArray() */



 
/**
 *
 *
 * @param Parm (I)
 * @param PIndex (I)
 * @param Val (I)
 */

char *MUShowSArray(
 
  const char *Parm,
  int         PIndex,
  const char *Val)
 
  {
  static char Line[MMAX_LINE];

  if (Parm == NULL)
    {
    Line[0] = '\0';
    }
  else
    {
    sprintf(Line,"%s[%1d]%-.*s  %s\n",
      Parm,
      PIndex,
      MCFG_HDR_LEN - 3 - (int)strlen(Parm),
      "                                ",
      (Val != NULL) ? Val : "");
    }
 
  return(Line);
  }  /* END MUShowSArray() */

 
 
 
 
/**
 *
 *
 * @param Param (I)
 * @param IndexName (I)
 * @param ValLine (I)
 * @param Buffer (I/O)
 * @param BufSize (I)
 */

char *MUShowSSArray(
 
  const char *Param,     /* I */
  char       *IndexName, /* I */
  const char *ValLine,   /* I */
  char       *Buffer,    /* I/O */
  int         BufSize)   /* I */
 
  { 
  snprintf(Buffer,BufSize,"%s[%s]%-.*s  %s\n",
    Param,
    IndexName,
    MCFG_HDR_LEN - 2 - (int)(strlen(Param) + strlen(IndexName)),
    "                                        ",
    (ValLine != NULL) ? ValLine : "");
 
  return(Buffer);
  }  /* END MUShowSSArray() */
 
 

 
 
/**
 * Report formated floating point based config parameters.
 *
 * @param Parm (I)
 * @param PIndex (I)
 * @param Val (I)
 */

char *MUShowFArray(
 
  const char *Parm,   /* I */
  int         PIndex, /* I */
  double      Val)    /* I */
 
  {
  static char Line[MMAX_LINE];
 
  sprintf(Line,"%s[%1d]%-.*s  %-6.2f\n",
    Parm,
    PIndex,
    MCFG_HDR_LEN - 3 - (int)strlen(Parm),
    "                                ",
    Val);
 
  return(Line);
  }  /* END MUShowFArray() */ 




/**
 *
 *
 * @param Cfg (I)
 * @param PList (O)
 */

int MUBuildPList(

  mcfg_t      *Cfg,   
  const char **PList) 

  {
  int cindex;

  if ((Cfg == NULL) || (PList == NULL))
    {
    return(FAILURE);
    }

  for (cindex = 0;Cfg[cindex].Name != NULL;cindex++)
    {
    PList[Cfg[cindex].PIndex] = Cfg[cindex].Name;
    }  /* END for (cindex) */

  return(SUCCESS);
  }  /* END MUBuildPList() */





/**
 * Report copyright info for clients and general Moab tools.
 *
 * @see MSysShowCopy() - parent/sync
 */

int MUShowCopy()

  {
  fprintf(stderr,"This software utilizes the Moab Scheduling Library, version %s\n",
    MOAB_VERSION);

  fprintf(stderr,"Copyright (C) 2000-2012 by Adaptive Computing Enterprises, Inc. All Rights Reserved.\n");

  return(SUCCESS);
  }  /* END MUShowCopy() */


