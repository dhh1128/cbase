/* HEADER */

      
/**
 * @file MUtilFile.c
 *
 * Contains: File IO utility functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/* global variables */

mhash_t TmpFileTemplates;



/**
 * This function is a may be a slightly faster version of mkstemp
 * when creating a lot of temp files in the same dir. It will create
 * unique temporary files just like mkstemp, but will do so in a 
 * more efficient manner. NOTE: This function is only faster if
 * MSched.EnableFastSpawn == TRUE!!! We may want to change this
 * in the future.
 * 
 * This function uses mkstemp to create a "base template" that is
 * guaranteed at creation time to be unique. Subsequent calls for
 * the same template use this base template plus a unique counter
 * value to create a new tempfile. The file is opened using O_EXCL
 * which guarantees the file doesn't already exist.
 *
 * @see 'man mkstemp'
 *
 * WARNING: NOT THREAD SAFE (accesses global variable and static var)
 *
 * @param Template (I/O) Similar to mkstemp's template var (see 'man mkstemp'). Will be modified. [minsize=MMAX_PATH_LEN]
 * @param FileDescriptor (O) The file descriptor to the open temporary file.
 */

int MUMksTemp(

  char *Template,
  int  *FileDescriptor)

  {
  int fd = -1;
  static unsigned int Counter = 0;

  if (FileDescriptor != NULL)
    *FileDescriptor = -1;

  if ((Template == NULL) ||
      (FileDescriptor == NULL))
    {
    return(FAILURE);
    }

#ifdef MJOSH
  fd = open(Template,O_CREAT|O_RDWR,S_IRUSR|S_IWUSR);  /* permissions mimic that of mkstemp (0600,exclusive,etc.) */
  *FileDescriptor = fd;
  return(SUCCESS);
#endif /* MJOSH */

    /*
  if ((MSched.EnableFastSpawn == FALSE) ||
      (getenv("MENABLEMKSTEMP") != NULL))
    */
  if (TRUE)
    {
    /* perform old mkstemp() file creation */

    fd = mkstemp(Template);   
    }
  else
    {
    char *BaseTemplate = NULL;

    if (Counter == 0)
      {
      /* make counter random to further reduce chance of temp. file name collision */

      srandom((unsigned int)time(NULL));
      Counter = random() % 1000;
      }

    if (MUHTGet(&TmpFileTemplates,Template,(void **)&BaseTemplate,NULL) == SUCCESS)
      {
      snprintf(Template,MMAX_PATH_LEN,"%s%d",
        BaseTemplate,
        Counter);

      Counter++;

      if (Counter == 0)
        {
        /* overflow protection--we don't want to assign a random num again to Counter */

        Counter = 1;
        }

      fd = open(Template,O_CREAT|O_EXCL|O_RDWR,S_IRUSR|S_IWUSR);  /* permissions mimic that of mkstemp (0600,exclusive,etc.) */
      }
    else
      {
      char FileName[MMAX_PATH_LEN];

      MUStrCpy(FileName,Template,sizeof(FileName));

      fd = mkstemp(Template);

      /* base future filenames off of this guaranteed unique file name */

      MUHTAdd(&TmpFileTemplates,Template,strdup(FileName),NULL,MUFREE);
      }
    }

  *FileDescriptor = fd;

  return(SUCCESS);
  }  /* END MUMksTemp() */



/**
 * get a copy of the scheduler's spool dir variable
 *
 * @param Buf
 * @param BufLen
 */

int MUGetSchedSpoolDir(

  char *Buf,      /* O (buffer to copy the spooldir to) */
  int   BufLen)   /* I (the length of the receiving buffer */
  {
  int rc;

  rc = MUStrCpy(Buf,MSched.SpoolDir,BufLen);

  return(rc);
  } /* END MUGetSchedSpoolDir */



/**
 * This function will extract the directory portion of
 * the given path and copy it into the Dir parameter.
 *
 * @param FullPath (I)
 * @param Dir (O)
 * @param DirSize
 */

char *MUExtractDir(

  char *FullPath,
  char *Dir,
  int   DirSize)

  {
  char *ptr;

  if ((FullPath == NULL) ||
      (Dir == NULL))
    {
    return(NULL);
    }

  Dir[0] = '\0';

  MUStrCpy(Dir,FullPath,DirSize);

  ptr = strrchr(Dir,'/');

  if (ptr != NULL)
    {
    *ptr = '\0';
    }

  return(Dir);
  }  /* END MUExtractDir() */


/* END MUtilFile.c */
