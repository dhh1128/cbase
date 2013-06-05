/* HEADER */

/**
 * @file MVMString.c
 * 
 * Contains various functions for VM to/from strings
 *
 */


#include "moab.h"
#include "moab-proto.h"
#include "moab-const.h"
#include "moab-global.h"


/**
 * Convert VM string to physical node mstring
 *
 * NOTE:  only support format '<VMID>[{ ,:+}<VMID>]...'
 *
 * @param PNString (O) [must be initialized already]
 * @param VMString (I) [modified as side affect] [must be initialized already]
 */

int MVMStringToPNString(

  mstring_t *PNString,     /* O */
  mstring_t *VMString)     /* I (modified as side affect) */

  {
  mstring_t tmpBuffer(MMAX_LINE);

  mstring_t *DPtr;

  char *ptr;
  char *TokPtr=NULL;

  char  tmpDelim[2];

  mvm_t *V;

  if ((PNString == NULL) || (VMString == NULL))
    {
    return(FAILURE);
    }

  if (PNString == VMString)
    {
    DPtr = &tmpBuffer;
    }
  else
    {
    DPtr = PNString;
    }

  ptr = MUStrTok(VMString->c_str(),", \t:+",&TokPtr);

  MStringSet(DPtr,"");

  tmpDelim[0] = '\0';
  tmpDelim[1] = '\0';

  while (ptr != NULL)
    {
    char * Name = ptr;
    if ((MUHTGet(&MSched.VMTable,ptr,(void **)&V,NULL) == SUCCESS) &&
        (V != NULL) &&
        (V->N != NULL))
      {
      Name = V->N->Name;
      }

    MStringAppendF(DPtr,"%s%s",
      tmpDelim,
      Name);

    tmpDelim[0] = *TokPtr;

    ptr = MUStrTok(NULL,", \t:+",&TokPtr);
    }  /* END while (ptr != NULL) */

  if (DPtr == &tmpBuffer)
    {
    MStringSet(PNString,tmpBuffer.c_str());
    }

  return(SUCCESS);
  }  /* END MVMStringToPNString() */


/* END MVMString.c */
