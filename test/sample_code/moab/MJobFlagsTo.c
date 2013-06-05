/* HEADER */

      
/**
 * @file MJobFlagsTo.c
 *
 * Contains: MJobFlagsToString and MJobFlagsToMString
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 * Report job flags as mstring_t.
 *
 * NOTE:  Will initialize string to empty.  May return empty string if no flags set.
 *
 * @see MJobFlagsFromString() - peer
 * @see MJobCanPreempt() - parent
 * @see MJobSelectMNL() - parent
 *
 * @param J (I) [optional]
 * @param SFlags (I) [optional, -1 = not set]
 * @param Buf (I)
 */

int MJobFlagsToMString(

  mjob_t        *J,       /* I (optional) */
  mbitmap_t     *SFlags,  /* I (optional, -1 = not set) */
  mstring_t     *Buf)     /* I */

  {
  int   index;

  mbitmap_t Flags;

  if (Buf == NULL)
    return(FAILURE);

  MStringSet(Buf,"");

  if (SFlags != NULL)
    {
    bmcopy(&Flags,SFlags);
    }
  else
    {
    bmcopy(&Flags,&J->Flags);
    }

  for (index = 0;MJobFlags[index] != NULL;index++)
    {
    if (!bmisset(&Flags,index))
      continue;

    if (!Buf->empty())
      MStringAppend(Buf,",");

    MStringAppend(Buf,(char *)MJobFlags[index]);

    if ((index == mjfAdvRsv) && (J != NULL) && (J->ReqRID != NULL))
      {
      MStringAppend(Buf,":");

      if (bmisset(&J->IFlags,mjifNotAdvres))
        MStringAppend(Buf,"!");

      MStringAppend(Buf,J->ReqRID);
      }
    }    /* END for (index) */

  return(SUCCESS);
  }      /* END MJobFlagsToMString() */
/* END MJobFlagsTo.c */
