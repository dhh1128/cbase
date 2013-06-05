/* HEADER */

/**
 * @file MUBufferedFD.c
 *
 * Contains Buffer File Descriptor IO
 */

#include "moab.h"
#include "moab-proto.h"
#include "moab-const.h"
#include "moab-global.h"



/**
 * Initializes a "buffer FD" structure which wraps 
 * an I/O file descriptor and adds a buffer to improve
 * performance in reading/writing an unknown amount of data.
 * 
 * @param BufFD (I)
 * @param fd (I)
 */

int MUBufferedFDInit(

  mbuffd_t *BufFD,
  int       fd)

  {
  BufFD->fd = fd;
  BufFD->UnreadCount = 0;
  BufFD->ReadCount = 0;
  BufFD->BufPtr = BufFD->Buf;

  return(SUCCESS);
  }  /* END MUBufferedFDInit() */


/**
 * Reads up to 'n' bytes from the buffered FD and
 * puts them into the given Buf. You should initialize the buffered FD
 * with MUBufferedFDInit() before using this function!
 *
 * @see MUBufferedFDInit()
 *
 * @param BufFD (I)
 * @param Buf (I)
 * @param n (I)
 */

ssize_t MUBufferedFDRead(

  mbuffd_t *BufFD,
  char     *Buf,
  size_t    n)

  {
  int count;

  if ((BufFD == NULL) ||
      (Buf == NULL))
    {
    return(FAILURE);
    }

  /* keep trying to read data into internal buffer, unless we hit EOF
   * or non-interrupt error */

  while (BufFD->UnreadCount <= 0)
    {
    BufFD->UnreadCount = read(BufFD->fd,BufFD->Buf,sizeof(BufFD->Buf));
    BufFD->BufPtr = BufFD->Buf;

    if (BufFD->UnreadCount == 0)
      {
      /* EOF hit */

      return(0);
      }
    else if (BufFD->UnreadCount < 0)
      {
      /* check to see if we are being interrupted by a signal handler */

      if (errno != EINTR)
        return(-1);
      }
    }

  /* copy data from BufFD->Buf into passed in buffer (up to n) */
  count = n;

  if ((unsigned int)BufFD->UnreadCount < n)
    count = BufFD->UnreadCount;

  memcpy(Buf,BufFD->BufPtr,count);

  BufFD->BufPtr += count;
  BufFD->UnreadCount -= count;
  BufFD->ReadCount += count;

  return(count);  /* return number of bytes being returned */
  }  /* END MUBufferedFDRead() */



/**
 * Reads in a line (data ending in '\n') from the given buffered FD.
 * Stores the line in the given Buf. This function will NULL terminate
 * the string and DOES keep the '\n'.
 *
 * @param BufFD (I)
 * @param Buf (I)
 * @param BufSize (I)
 *
 * @return The number of bytes copied into Buf. Note that if the line is larger
 * than BufSize, then BufSize-1 bytes are copied into Buf and then it is NULL
 * terminated, thereby filling up the entirety of Buf.
 */

ssize_t MUBufferedFDReadLine(

  mbuffd_t *BufFD,
  char     *Buf,
  int       BufSize)

  {
  int bindex;
  int rc;

  char *ptr;
  char  Byte;

  if ((BufFD == NULL) ||
      (Buf == NULL))
    {
    return(-1);
    }

  Buf[0] = '\0';

  ptr = Buf;

  for (bindex = 1;bindex < BufSize;bindex++)
    {
    /* by using a buffered FD, we can efficiently read one byte at a time */

    if ((rc = MUBufferedFDRead(BufFD,&Byte,1)) == 1)
      {
      *ptr = Byte;
      ptr++;

      if (Byte == '\n')  /* we are done reading in the line */
        break;
      }
    else if (rc == 0)
      {
      /* error may have occurred */

      if (bindex == 1)
        return(0);  /* no data read */
      else
        break;      /* EOF (but data was read) */
      }
    else
      {
      /* error occurred */

      return(-1);
      }
    }

  *ptr = '\0';  /* terminate line */

  return(bindex); 
  }  /* END MUBufferedFDReadLine() */


/**
 * Reads in a line (data ending in '\n') from the given buffered FD.
 * Stores the line in the given mstring, Buf. This function will NULL terminate
 * the string and DOES keep the '\n'.
 *
 * If there is only one line in the entire file, then the entire file will
 * be placed into the mstring!
 *
 * @param BufFD (I)
 * @param Buf (I)
 *
 * @return SUCCESS if a line was succesfully extracted from the FD. FAILURE if an error occurred.
 */

int MUBufferedFDMStringReadLine(

  mbuffd_t  *BufFD,
  mstring_t *Buf)

  {
  int rc = 0;
  char tmpLine[MMAX_LINE];

  while (TRUE)
    {
    rc = MUBufferedFDReadLine(BufFD,tmpLine,sizeof(tmpLine));

    if (rc == 0)
      {
      /* EOF--done loading file */

      MStringAppend(Buf,tmpLine);

      return(SUCCESS);
      }
    else if (rc < 0)
      {
      /* error occurred */

      return(FAILURE);
      }
    else if ((unsigned int)rc == sizeof(tmpLine))
      {
      /* buffer overflow...continue reading */

      MStringAppend(Buf,tmpLine);

      continue;
      }

    MStringAppend(Buf,tmpLine);

    break;  /* we've loaded in the entire line! */
    }

  return(SUCCESS);
  }  /* END MUBufferedFDMStringReadLine() */
/* END MUBufferedFD.c */
