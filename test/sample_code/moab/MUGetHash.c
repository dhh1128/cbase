/* HEADER */

      
/**
 * @file MUGetHash.c
 *
 * Contains: MUGetHash 
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 *
 *
 * @param Name (I)
 */

unsigned long MUGetHash(
 
  char const *Name)  /* I */
 
  {
  const int     x[] = { 7, 11, 17, 23, 31, 37, 43, 47, 51, 53, 57 };
  int           i   = 0;
  unsigned long key = 0;

  const char *FName = "MUGetHash";
 
  MDB(8,fSTRUCT) MLog("%s(%s)\n",
    FName,
    Name);
 
  while (Name[i] != '\0')
    {
    key += (0x0f0f * x[i % 11] * (Name[i] - '.')) << (i % 6);

    i++;
    }  /* END while (Name[i] != '\0') */
 
  MDB(8,fSTRUCT) MLog("INFO:     hash '%s' --> %6ld\n",
    Name,
    key);
 
  return(key);
  }  /* END MUGetHash() */

/* END MUGetHash.c */
