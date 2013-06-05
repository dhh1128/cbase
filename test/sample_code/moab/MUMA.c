/* HEADER */

/**
 * @file MUMA.c
 * 
 * Contains various functions dealing with MAList
 *
 */

#include "moab.h"
#include "moab-proto.h"
#include "moab-const.h"
#include "moab-global.h"

/**
 * Convert an experienced list to string.
 *
 * NOTE: returns empty string on empty list.
 *
 * @param AttrIndex (meNFeature etc)
 * @param Delim (delimiter to use)
 * @param ValueMap (bitmap of features to print)
 * @param String (initialized and set)
 */


int MUMAToString(

  enum MExpAttrEnum    AttrIndex,
  char                 Delim,
  mbitmap_t           *ValueMap,
  mstring_t           *String)

  {
  int         sindex;

  if (String != NULL)
    MStringSet(String,"\0");

  if ((ValueMap == NULL) || (String == NULL))
    {
    return(FAILURE);
    }
 
  /* The Delim check is done outside of the for loop for speed */

  if (Delim != '\0')
    {
    for (sindex = 1;MAList[AttrIndex][sindex][0] != '\0';sindex++)
      {
      if (bmisset(ValueMap,sindex))
        {
        if (!String->empty())
          {
          MStringAppendF(String,"%c%s",
            Delim,
            MAList[AttrIndex][sindex]);
          }
        else
          {
          MStringAppend(String,
            MAList[AttrIndex][sindex]);
          }
        }
      }      /* END for (sindex) */
    } /* END if (Delim != '\0') */
  else
    {
    for (sindex = 1;MAList[AttrIndex][sindex][0] != '\0';sindex++)
      {
      if (bmisset(ValueMap,sindex))
        {
        MStringAppendF(String,"[%s]",
          MAList[AttrIndex][sindex]);
        }
      }      /* END for (sindex) */
    }        /* END else */

  return(SUCCESS);
  }  /* END MUMAToString() */

/**
 * Locate/create entry in 'experienced' value array, MAList[]
 *
 * NOTE:  comparison is case-insensitive as of Moab 5.2.2
 *
 * If the attr value is not in MAList and Mode==mAdd, the attr will be added 
 * and the new index returned.
 *
 * @param AIndex (I) Attribute Type
 * @param Value (I) Attribute Index
 * @param Mode (I) Search Mode
 *
 * @return '0' on FAILURE
 * @return index of attribute in MAList[] on SUCCESS, 0 on FAILURE. 
 */

int MUMAGetIndex(

  enum MExpAttrEnum        AIndex, /* I  Attribute Type  */
  char const              *Value,  /* I  Attribute Index */
  enum MObjectSetModeEnum  Mode)   /* I  Search Mode     */

  {
  /* determine index of up to MMAX_ATTR attribute values */

  int index;
  int len;

  const char *FName = "MUMAGetIndex";

  MDB(9,fSTRUCT) MLog("%s(%s,%s,%s)\n",
    FName,
    MAttrType[AIndex],
    (Value != NULL) ? Value : "NULL",
    MObjOpType[Mode]);

  if ((AIndex == meGRes) || (AIndex == meGMetrics))
    {
    return(MUGenericGetIndex(AIndex,Value,Mode));
    }

  if ((Value == NULL) || (Value[0] == '\0'))
    {
    /* FAILURE */

    return(0);
    }

  len = sizeof(MAList[0][0]) - 1;

  for (index = 1;index < MMAX_ATTR;index++)
    {
    if (!strncasecmp(MAList[AIndex][index],Value,len))
      {
      return(index);
      }

    if (MAList[AIndex][index][0] == '\0')
      break;
    }  /* END for (index) */

  if (Mode == mVerify)
    {
    /* didn't find value -- FAILURE */

    return(0);
    }

  /* NOTE:  GRes may be more constrained than MAList, return failure */

  if ((AIndex == meGRes) &&
      (Mode == mAdd))
    {
    if (index >= MSched.M[mxoxGRes] + 1)
      {
      if (MSched.OFOID[mxoxGRes] == NULL)
        MUStrDup(&MSched.OFOID[mxoxGRes],Value);

      MDB(1,fCONFIG) MLog("ALERT:    no empty slots for %s in MAList[%s] (increase MAXGRES)\n",
        Value,
        MAttrType[AIndex]);

      /* FAILURE */

      return(0);
      }

    MSched.GResCount = MAX(MSched.GResCount,index);
    }  /* END if ((AIndex == meGRes) && ...) */

  /* add new value to table */

  MUStrCpy(MAList[AIndex][index],Value,sizeof(MAList[0][0]));

  MDB(5,fCONFIG) MLog("INFO:     adding MAList[%s][%d]: '%s'\n",
    MAttrType[AIndex],
    index,
    Value);

  /* SUCCESS */

  return(index);
  }  /* END MUMAGetIndex() */


