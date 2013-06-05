/* HEADER */

/**
 * @file MJobPolicy.c
 *
 * Contains routines to test and manage Policies within the Job
 */


#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/** 
 * Returns the active JobRejectPolicy Bitmap
 *
 * @param J (I)
 */

const mbitmap_t *MJobGetRejectPolicy(

  const mjob_t  *J)

  {
  if (J == NULL)
    {
    return(NULL);
    }

  /* Job policy overrides the global policy */

  if (!bmisclear(&J->JobRejectPolicyBM))
    return &J->JobRejectPolicyBM;
  else
    return &MSched.JobRejectPolicyBM;
  }  /* END MJobGetRejectPolicy() */

