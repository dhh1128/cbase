/* HEADER */

/**
 * @file MUtilMisc.c
 *
 * Contains Misc utility functions for Moab.
 */

#include "moab.h"
#include "moab-proto.h"
#include "moab-const.h"
#include "moab-global.h"



/**
 * Return list of primary and secondary group.
 *
 * NOTE:  should use cached user based group membership list in U->F.GAL (NYI) 
 *
 * @see MUserAdd() - parent
 *
 * @param UName (I)
 * @param Buf (O) [optional]
 * @param GList (O) - terminated w/-1 [optional]
 * @param BufSize (I)
 * @param GCount (I) [optional]
 */

int MOSGetGListFromUName(

  char *UName,    /* I */
  char *Buf,      /* O (optional) */
  int  *GList,    /* O (optional) - terminated w/-1 */
  int   BufSize,  /* I */
  int  *GCount)   /* I (optional) */

  {
  char *BPtr = NULL;
  int   BSpace = 0;

  struct group *GPtr;
  int           uindex;
  int           gindex;

  char GName[MMAX_NAME];

  if ((UName == NULL) || (MSched.OSCredLookup == mapNever))
    {
    return(FAILURE);
    }

  /* FORMAT:  <GNAME>[,<GNAME>]... */

  if (Buf != NULL)
    MUSNInit(&BPtr,&BSpace,Buf,BufSize);

  gindex = MUGIDFromUser(-1,UName,GName);

  if (GList != NULL)
    GList[0] = gindex;

  if (Buf != NULL)
    {
    MUSNPrintF(&BPtr,&BSpace,"%s",
      GName);
    }

  setgrent();

  /* walk all groups, walk all users in each group (less efficient) */

  gindex = 1;

  while ((GPtr = getgrent()) != NULL)
    {
    for (uindex = 0;GPtr->gr_mem[uindex] != NULL;uindex++)
      {
      if (!strcmp(GPtr->gr_mem[uindex],UName))
        {
        if (Buf != NULL)
          {
          if (BPtr > Buf)
            MUSNCat(&BPtr,&BSpace,",");

          MUSNPrintF(&BPtr,&BSpace,"%s",
            GPtr->gr_name);
          }
        else 
          {
          if (GList != NULL)
            GList[gindex] = GPtr->gr_gid;
     
          gindex++;

          if (gindex >= (BufSize - 1))
            break;
          }

        break;
        }
      }  /* END for (uindex) */
    }    /* END while (getgrent() != NULL) */

  endgrent();

  if (GList != NULL)
    GList[gindex] = -1;

  if (GCount != NULL)
    *GCount = gindex;

  return(SUCCESS);
  }  /* END MOSGetGListFromUName() */


/**
 * Convienence function that moves the contents of a pointer to another (shallow copy).
 *
 * @param SrcP (I) [modified]
 * @param DstP (O) [modified]
 */

int MMovePtr(
  
  char **SrcP,  /* I (modified) */
  char **DstP)  /* O (modified) */

  {
  if ((SrcP == NULL) || (DstP == NULL))
    {
    return(FAILURE);
    }

  *DstP = *SrcP;

  *SrcP = NULL;
 
  return(SUCCESS);
  }  /* END MMovePtr() */



#define MMAX_VERIFY_BYTES 50

/**
 * Uses a simple heuristic to determine if the given file is composed of printable
 * text. This heuristic simply looks at the first x bytes (default: x = 50) and if
 * all are either printable or whitespace, then this function assumes the file
 * is completely text.
 *
 * @see MCProcessRMScript()
 *
 * @param FileName (I) [optional] The file to check.
 * @param SrcBuf (I) [optional]
 * @param IsText (O) Boolean that indicates whether or not the file is thought
 * to be entirely made of text. [optional]
 *
 * @return Returns SUCCESS if the file is successfully opened and analyzed. The return value is
 * NOT dependent on whether the file is actually text or not.
 */

int MUFileIsText(

  char    *FileName,  /* I (optional) */
  char    *SrcBuf,    /* I (optional) */
  mbool_t *IsText)    /* O (optional) */

  {
  FILE *fp = NULL;

  int i;
  int c;

  if ((FileName == NULL) && (SrcBuf == NULL))
    {
    return(FAILURE);
    }

  if (IsText != NULL)
    *IsText = FALSE;

  if (SrcBuf == NULL)
    {
    fp = fopen(FileName,"r");

    if (fp == NULL)
      {
      MDB(3,fSTRUCT) MLog("ERROR:    cannot open file '%s' for reading (errno: %d '%s')\n",
        FileName,
        errno,
        strerror(errno));

      fprintf(stderr,"ERROR:    cannot open file '%s' for reading (errno: %d '%s')\n",
        FileName,
        errno,
        strerror(errno));

      return(FAILURE);
      }

    /* if any of the first 'MMAX_VERIFY_BYTES' characters are not printable, consider file
       to be binary */

    for (i = 0;i < MMAX_VERIFY_BYTES;i++)
      {
      c = fgetc(fp);

      if (c == EOF)
        break;

      if (!isprint(c) && !isspace(c))
        {
        fclose(fp);

        return(SUCCESS);
        }
      }  /* END for (i) */
    }
  else
    {
    for (i = 0;i < MMAX_VERIFY_BYTES;i++)
      {
      c = SrcBuf[i];

      if (c == '\0')
        break;

      if (!isprint(c) && !isspace(c))
        {
        return(SUCCESS);
        }
      }  /* END for (i) */
    }

  if (IsText != NULL)
    *IsText = TRUE;

  if (fp != NULL)
    fclose(fp);

  return(SUCCESS);
  }  /* END MUFileIsText() */



/**
 *
 *
 * @param SList (I) [null terminated array of string pointers]
 * @param OBuf (O)
 * @param OBufSize (I)
 */

char *MUSListToString(

  char **SList,    /* I (null terminated array of string pointers) */
  char  *OBuf,     /* O */
  int    OBufSize) /* I */

  {
  char *BPtr = NULL;
  int   BSpace = 0;

  int   sindex;

  if (OBuf == NULL)
    {
    return(NULL);
    }

  MUSNInit(&BPtr,&BSpace,OBuf,OBufSize);

  if (SList == NULL)
    {
    return(OBuf);
    }

  for (sindex = 0;SList[sindex] != NULL;sindex++)
    {
    if (sindex != 0)
      {
      MUSNCat(&BPtr,&BSpace,",");
      }

    MUSNCat(&BPtr,&BSpace,SList[sindex]);
    }  /* END for (sindex) */

  return(OBuf);
  }  /* END MUSListToString() */




/**
 * This routine extracts the job number from a job name (for example 1234 from "moab.1234" or "1234")
 *
 * @param JobName   (I)
 * @param JobNumber (O)  
 */

int MUGetJobIDFromJobName(

  const char *JobName,
  int        *JobNumber)

  {
  const char *tmpPtr;

  size_t len;

  if ((JobName == NULL) || (JobNumber == NULL))
    {
    return(FAILURE);
    }

  tmpPtr = strrchr(JobName,'.');

  if (tmpPtr == NULL)
    {
    /* No '.' in the job name so the job name should be just as number (e.g. 1234). */

    tmpPtr = JobName;
    }
  else
    {
    /* If joab name is in the format moab.1234 then set tmpPtr to 1234 */

    tmpPtr++;
    }

  len = strlen(tmpPtr);

  /* If there was nothing after the '.' then we did not find a number */

  if (len == 0)
    {
    return(FAILURE);
    }

  /* There should be only numbers from tmpPtr to the end of the string */

  if (strspn(tmpPtr,"0123456789") != len)
    {
    return(FAILURE);
    }

  *JobNumber = atoi(tmpPtr);

  return(SUCCESS);
  }  /* END MUGetJobIDFromJobName() */




/*
 * Generates a random string of size length.  Terminates with \0.
 *
 * @param Str    [I - modified] The string to populate
 * @param StrLen [I]            Length of the string passed int
 * @param Size   [I]            Size to make the random string
 */

int MUGenRandomStr(

  char *Str,     /* I/O (modified) */
  int   StrLen,  /* I */
  int   Size)    /* I */

  {
  int counter;
  int Max;

  char text[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

  if (StrLen <= Size)
    {
    Max = StrLen - 1;
    }
  else
    Max = Size;


  for (counter = 0;counter < Max;counter++)
    {
    Str[counter] = text[rand() % (sizeof(text) - 1)];
    }

  Str[counter] = '\0';

  return(SUCCESS);
  } /* END MUGenRandomStr() */


/**
 * Function that abstracts abort so that it can be stubbed out.
 */

void MAbort()
  {
  abort();
  }

/* END MUtilMisc.c */
