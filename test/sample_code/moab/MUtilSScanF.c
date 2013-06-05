/* HEADER */

      
/**
 * @file MUtilSScanF.c
 *
 * Contains: MUSSscanF and MUPrintBuffer
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 *
 *
 * @param StringBuffer (I)
 * @param Format (I)
 */

int MUSScanF(
 
  const char *StringBuffer,  /* I */
  const char *Format,        /* I */
  ...)
 
  {
  const char *fptr;
  const char *sptr;
 
  char *tail = NULL;
 
  char *tmpS;
  long *tmpL;
  int  *tmpI;

  char  IFSList[MMAX_NAME];
 
  long  size;
  long  length;
 
  va_list VA;

  int   ArgCount;

  ArgCount = 0;
 
  /* FORMAT:  "%x%s %ld %d" */
 
  if (StringBuffer == NULL)
    {
    return(FAILURE);
    }
 
  if (Format == NULL)
    {
    return(FAILURE);
    }

  sptr = StringBuffer;

  if (!strncmp(sptr,"IFS-",strlen("IFS-")))
    {
    sptr += strlen("IFS-");

    IFSList[0] = *sptr;

    IFSList[1] = '\0';

    sptr++;
    }
  else
    {
    strcpy(IFSList," \t\n");
    }

  va_start(VA,Format);
 
  size = MMAX_NAME;
 
  for (fptr = Format;*fptr != '\0';fptr++)
    {
    if (*fptr == '%')
      {
      fptr++;

      /* remove IFS chars */

      while (strchr(IFSList,sptr[0]) && (sptr[0] != '\0'))
        sptr++;
 
      switch (*fptr)
        {
        case 'd':                                    
        case 'u':

          /* read integer */
 
          tmpI = va_arg(VA,int *);
 
          if (tmpI != NULL)
            {
            *tmpI = (int)strtol(sptr,&tail,10);
            sptr = tail;
            }

          while (!strchr(IFSList,sptr[0]) && (sptr[0] != '\0'))  
            sptr++;

          ArgCount++;

          break;
 
        case 'l':
 
          tmpL = va_arg(VA,long *);
 
          if (tmpL != NULL)
            {
            *tmpL = strtol(sptr,&tail,10);
            sptr = tail;
            }

          while (!strchr(IFSList,sptr[0]) && (sptr[0] != '\0')) 
            sptr++; 

          ArgCount++;
 
          break;
 
        case 's':
 
          if (size == 0)
            {
            return(FAILURE);
            }
 
          tmpS = va_arg(VA,char *);
          tmpS[0] = '\0';
 
          length = 0;
 
          while ((sptr[0] != '\0') && strchr(IFSList,sptr[0]))
            sptr++;
 
          while(length < (size - 1))
            {
            if (*sptr == '\0')
              break;
 
            if (strchr(IFSList,sptr[0]))
              break;
 
            if (tmpS != NULL)
              tmpS[length] = *sptr;
 
            length++;
 
            sptr++;
            }
 
          if (tmpS != NULL)
            tmpS[length] = '\0';

          if (length > 0)
            ArgCount++;
 
          break;
 
        case 'x':
 
          size = va_arg(VA,int);                                       
 
          break;
 
        default:

          /* unexpected format */
 
          break;
        }  /* END switch (*fptr) */
      }    /* END if (*fptr == '%') */
    }      /* END for (fptr = Format,*fptr != '\0';fptr++) */
 
  va_end(VA);
 
  return(ArgCount);
  }  /* END MUSScanF() */                    



/**
 *
 * NOTE: This function MUST be threadsafe!
 * NOTE: This function appears to be thread-safe!
 *
 * @param Buf (I)
 * @param BufSize (I)
 * @param OBuf (O) [minsize=MMAX_LINE]
 */

char *MUPrintBuffer(

  char *Buf,     /* I */
  int   BufSize, /* I */
  char *OBuf)    /* O (minsize=MMAX_LINE) */

  {
  int  bindex;
  int  lindex;

  int  bcount;

  lindex = 0;

  bcount = (MMAX_LINE-1 < BufSize) ? MMAX_LINE-1 : BufSize;

  for (bindex = 0;bindex < bcount;bindex++)
    {
    if (!isprint(Buf[bindex]) && !isspace(Buf[bindex]))
      {
      break;
      }

    OBuf[lindex++] = Buf[bindex];
    }  /* END for (bindex) */

  OBuf[lindex] = '\0';
   
  return(OBuf);
  }  /* END MUPrintBuffer() */

/* END MUtilSScanF.c */
