/* HEADER */

/**
 * @file MInit.c
 *
 * Moab Initialization
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "moab-local.h"

#define SUCCESS              1
#define FAILURE              0


/**
 *
 *
 * @param VarDir (I)
 * @param VarDirLength (I)
 */

int MCGetVarDir(

  char *VarDir,
  int   VarDirLength)

  {
  if (VarDir == NULL)
    {
    return(FAILURE);
    }

#if defined(MBUILD_VARDIR)
  snprintf(VarDir,VarDirLength,"%s",MBUILD_VARDIR);
#endif /* MBUILD_VARDIR */

  return(SUCCESS);
  }



/**
 *
 *
 * @param EtcDir
 * @param EtcDirLength
 */

int MCGetEtcDir(

  char *EtcDir,
  int   EtcDirLength)

  {
  if (EtcDir == NULL)
    {
    return(FAILURE);
    }

#if defined(MBUILD_ETCDIR)
  snprintf(EtcDir,EtcDirLength,"%s",MBUILD_ETCDIR);
#endif /* MBUILD_ETCDIR */

  return(SUCCESS);
  }



/**
 *
 *
 * @param BuildDir
 * @param BuildDirLength
 */

int MCGetBuildDir(

  char *BuildDir,
  int   BuildDirLength)

  {
  if (BuildDir == NULL)
    {
    return(FAILURE);
    }

#if defined(MBUILD_DIR)
  snprintf(BuildDir,BuildDirLength,"%s",MBUILD_DIR);
#else
  snprintf(BuildDir,BuildDirLength,"NA");
#endif /* MBUILD_DIR */

  return(SUCCESS);
  }





/**
 *
 *
 * @param BuildHost (O)
 * @param BuildHostLength (I)
 */

int MCGetBuildHost(

  char *BuildHost,       /* O */
  int   BuildHostLength) /* I */

  {
  if (BuildHost == NULL)
    {
    return(FAILURE);
    }

#if defined(MBUILD_HOST)
  snprintf(BuildHost,BuildHostLength,"%s",MBUILD_HOST);
#else
  snprintf(BuildHost,BuildHostLength,"NA");
#endif /* MBUILD_HOST */

  return(SUCCESS);
  }  /* END MCGetBuildHost() */



/**
 *
 *
 * @param BuildDate (O)
 * @param BuildDateLength (I)
 */

int MCGetBuildDate(

  char *BuildDate,       /* O */
  int   BuildDateLength) /* I */
   
  {
  if (BuildDate == NULL)
    {
    return(FAILURE);
    }

#if defined(MBUILD_DATE)
  snprintf(BuildDate,BuildDateLength,"%s",MBUILD_DATE);
#else
  snprintf(BuildDate,BuildDateLength,"NA");
#endif /* MBUILD_DATE */

  return(SUCCESS);
  }  /* END MCGetBuildDate() */




/**
 *
 *
 * @param BuildArgs (O)
 * @param BuildArgsLength (I)
 */

int MCGetBuildArgs(

  char *BuildArgs,       /* O */
  int   BuildArgsLength) /* I */

  {
  if (BuildArgs == NULL)
    {
    return(FAILURE);
    }

#if defined(MBUILD_ARGS)
  {
  char *ptr;

  snprintf(BuildArgs,BuildArgsLength,"%s",MBUILD_ARGS);

  /* hide sensitive key info */

  ptr = strstr(BuildArgs,"--with-key=");

  if (ptr != NULL)
    {
    ptr += strlen("--with-key=");

    while (isalnum(*ptr))
      {
      *ptr = '?';
      ptr++;
      }
    }  /* END if (ptr != NULL) */
  }
#else
  snprintf(BuildArgs,BuildArgsLength,"NA");
  
#endif /* MBUILD_ARGS */

  return(SUCCESS);
  }  /* END MCGetBuildArgs() */


/* END MInit.c */
