/* HEADER */

      
/**
 * @file MAttr.c
 *
 * Contains: MAttr data structure's getters/setters, etc
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 * NOTE: if Mode == mclAND, return SUCCESS if Req is a subset of Avl 
 * NOTE: if Mode == mclOR, return SUCCESS if any Req attr is found in Avl and Req 
 * NOTE: if Mode == mclNOT, return SUCCESS if Req is not a subset of Avl 
 *
 * @param AvlMap (I)
 * @param ReqMap (I)
 * @param Mode (I)
 */

int MAttrSubset(

  const mbitmap_t *AvlMap,
  const mbitmap_t *ReqMap,
  enum MCLogicEnum Mode)

  {
  int index;

  if ((NULL == AvlMap) || (NULL == ReqMap))
    {
    return(FAILURE);
    }

  if (Mode == mclAND)
    {
    for (index = 0;index < MMAX_ATTR ;index++)
      {
      if (MAList[meNFeature][index][0] == '\0')
        break;

      if ((bmisset(AvlMap,index) != bmisset(ReqMap,index) &&
          bmisset(ReqMap,index)))
        {
        return(FAILURE);
        }
      }  /* END for (index) */
    }
  else if (Mode == mclNOT)
    {
    for (index = 0;index < MMAX_ATTR;index++)
      {
      if (MAList[meNFeature][index][0] == '\0')
        break;

      if ((bmisset(AvlMap,index) == bmisset(ReqMap,index)) &&
          bmisset(ReqMap,index))
        {
        return(FAILURE);
        }
      }  /* END for (index) */
    }
  else
    {
    /* Mode == mclOR */

    for (index = 0;index < MMAX_ATTR;index++)
      {
      if (MAList[meNFeature][index][0] == '\0')
        break;

      if (!bmisset(ReqMap,index))
        continue;

      if (bmisset(AvlMap,index))
        {
        return(SUCCESS);
        }
      }  /* END for (index) */

    return(FAILURE);
    }    /* END else (Mode == mclAND) */

  return(SUCCESS);
  }  /* END MAttrSubset() */


/* END MAttr.c */
