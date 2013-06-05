/* HEADER */

      
/**
 * @file MJobAToString.c
 *
 * Contains:
 *     MJobAToString()
 */


#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  





/**
 * Report job attribute as string.
 *
 * NOTE:  There is also an MJobAToMString()
 * Sync with MJobAToMString().
 *
 * @see MJobSetAttr() - peer
 * @see MReqAToMString() - peer
 *
 * @param J (I)
 * @param AIndex (I)
 * @param Buf (O) [realloc,minsize=MMAX_LINE]
 * @param BufSize (I/O) [modified]
 * @param Mode (I)
 */

int MJobAToString(

  mjob_t              *J,       /* I */
  enum MJobAttrEnum    AIndex,  /* I */
  char                *Buf,     /* O (realloc,minsize=MMAX_LINE) */
  int                  BufSize, /* I/O (modified) */
  enum MFormatModeEnum Mode)    /* I */

  {
  return(FAILURE);
  }  /* END MJobAToString() */

