/* HEADER */

/**
 * @file MJobGMetric.c
 *
 * Contains Functions dealing with a Job's GMetrics
 */


#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  

/**
 * Update a Job's GMetric value
 *
 * GMETRIC[<subattr>]=<value> has been parsed and this function
 * will update/add the GMETRIC <subattr> with a new <value> to the 
 * IntervalLoad for the specified Job
 *
 * @param  J       (I)   (job reference)
 * @param  SubAttr (I)   (GMETRIC subattr)
 * @param  Value   (I)   (GMETRIC value)
 */

int MJobGMetricUpdateAttr(

  mjob_t     *J,
  const char *SubAttr,
  const char *Value)

  {
  int       aindex;
  double    DVal;

  /* Parameter check */
  if ((NULL == J) || (NULL == SubAttr) || (NULL == Value) || (NULL == J->Req[0]))
    return(FAILURE);

  /* Ensure that the J->Req[0]->XURes is initialized,
   * if it is already initialized, the function will return success w/o modifying it
   */
  if (FAILURE == MReqInitXLoad(J->Req[0]))
    return(FAILURE);

  /* search for the SubAttr name, add it if necessary to the global name table  */
  aindex = MUGenericGetIndex(meGMetrics,SubAttr,mAdd);
  if (0 == aindex)
    {
    /* Could not find-add the SubAttr name, and table is full */
    return(FAILURE);
    }

  /* with a valid aindex, we can now update the gmetric for this job */

  DVal = strtod(Value,NULL);

  J->Req[0]->XURes->GMetric[aindex]->IntervalLoad = DVal;
  J->Req[0]->XURes->GMetric[aindex]->SampleCount++;

  return(SUCCESS);
  }  /* END MJobGMetricUpdateAttr() */
