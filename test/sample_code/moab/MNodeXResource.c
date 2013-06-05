/* HEADER */

      
/**
 * @file MNodeXResource.c
 *
 * Contains: XLoad Resource functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 * Initializes the XLoad structure on the node.  Also creates
 *  the gevent arrays.
 */

int MNodeInitXLoad(

  mnode_t *N)    /* I */

  {
  if (N->XLoad != NULL)
    {
    return(SUCCESS);
    }

  if ((N->XLoad = (mxload_t *)MUCalloc(1,sizeof(mxload_t))) == NULL)
    {
    return(FAILURE);
    }

  MGMetricCreate(&N->XLoad->GMetric);

  return(SUCCESS);
  } /* END MNodeInitXLoad */


/**
 *
 *
 * @param XLP (I) [freed]
 */

int MXLoadDestroy(

  mxload_t **XLP)  /* I (freed) */

  {
  mxload_t *XL;
  int       xindex;

  if ((XLP == NULL) || (*XLP == NULL))
    {
    return(SUCCESS);
    }

  XL = *XLP;

  for (xindex = 0;xindex < MSched.M[mxoxGMetric];xindex++)
    {
    if (XL->GMetric[xindex] == NULL)
      continue;

    MUFree((char **)&XL->GMetric[xindex]);
    }

  MUFree((char **)&XL->GMetric);

  bmclear(&XL->RMSetBM);

  MUFree((char **)XLP);

  return(SUCCESS);
  }  /* END MXLoadDestroy() */
/* MNodeXResource.c */
