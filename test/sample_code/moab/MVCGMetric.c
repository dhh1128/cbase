/* HEADER */

/**
 * @file MVCGMetric.c
 *
 * Contains Virtual Container (VC) GMETRIC handling functions
 */

#include <assert.h>

#include "moab.h"
#include "moab-proto.h"
#include "moab-global.h"
#include "moab-const.h"


/**
 * Receive a linked list of Jobs that need harvesting of data
 *
 * Extract the GMETRIC[subattr] on this list of jobs (if any) and then add
 * those values to the 'UsageP' total value, and bump the count of such
 * Jobs encounter. Not thread save.
 *
 * @param   JList     (I)   [required]
 * @param   Name      (I)   [required]
 * @param   UsageP    (O)   [required]
 * @param   CountP    (O)   [required]
 */
int MVCGMetricHarvestJobListUsage(
    
  mln_t     *JList,
  const char      *Name,
  double    *UsageP,
  int       *CountP)
 
  {
  mjob_t    *J;
  mln_t     *Iter;
  int        aindex;
  double     usage;

  /* check for no jobs on this list */
  if (NULL == JList)
    return(SUCCESS);

  /* Lookup the GMETRIC subattr 'name' in the GMETRIC table, if
   * not found, then we are done, because this GMETRIC is NOT defined (yet)
   */
  aindex = MUGenericGetIndex(meGMetrics,Name,mVerify);
  if (0 == aindex)
    return(SUCCESS);

  Iter = NULL;

  /* Iterate over the job list and extract matching data */
  while(SUCCESS == MULLIteratePayload(JList,&Iter,(void**) &J))
    {
    /* proceed to verify that we have a GMetric 'chain' of containers for this Name */
    if ((NULL == J->Req[0]) || 
        (NULL == J->Req[0]->XURes) || 
        (NULL == J->Req[0]->XURes->GMetric) || 
        (NULL == J->Req[0]->XURes->GMetric[aindex]))
    {
      continue;
    }

    /* We now have a pointer chain to the IntervalLoad data for this Job/GMETRIC.
     * IF the InternvalLoad IS equal to MDEF_GEMTRIC_VALUE we ignore this instance.
     *
     * Otherwise add to the running 'total', this job's value, then bump the count of Jobs 
     * harvested so far
     */
    usage = J->Req[0]->XURes->GMetric[aindex]->IntervalLoad;

    if (MDEF_GMETRIC_VALUE != usage)
      {
      *UsageP += usage;
      (*CountP)++;
      }
    }

  return(SUCCESS);
  } /* END  MVCGMetricHarvestJobListUsage() */

/**
 * Recursively Walk the VC tree specified by the VC argument
 *
 * return the TOTAL usage amount and the COUNT of instance that
 * contributed to that TOTAL
 *
 * NOTE: If a Job is "in the tree" more than once, (IE a Job
 * is contained in two or more VCs, which in turn are contained
 * in a top level VC)
 * it will be harvested for each instance. As such, it's Interval
 * load value will be double (or more) added to the total.
 * At this time, this is acceptable.
 *
 * @param   VC        (I)   [required]
 * @param   Name      (I)   [required]
 * @param   UsageP    (O)   [required]
 * @param   CountP    (O)   [required]
 */
int MVCGMetricHarvestTreeWalk(

    mvc_t      *VC,
    const char *Name,
    double     *UsageP,
    int        *CountP)

{
  mln_t       *Iter;    /* Iterator for VCs */

  /* need to gather threadhold data on the Jobs in this list */
  MVCGMetricHarvestJobListUsage(VC->Jobs,Name,UsageP,CountP);

  /* now harvest from all the children VCs (if any) of this instance */
  if (NULL != VC->VCs)
  {
    /* Iterate the VC list and harvest the Jobs' data from each 
     * and their respective children
     */

    Iter = NULL;

    while (SUCCESS == MULLIteratePayload(VC->VCs,&Iter,NULL))
    {
      /* NOTE:  This is recursive here
       *
       * For each child VC harvest its data
       */
      MVCGMetricHarvestTreeWalk((mvc_t *) Iter->Ptr,Name,UsageP,CountP);
    }
  }

  return(SUCCESS);
}


/**
 * Harvest the GMETRIC usage data for the specified VC and
 * all of it children
 *
 * @param   VC        (I)   [required]
 * @param   Name      (I)   [required]
 * @param   UsageP    (O)   [required]
 * @param   EMsg      (O)   ([optional,minsize=MMAX_LINE]
 */
int MVCGMetricHarvestAverageUsage(

    mvc_t *VC,        /* I */
    const char *Name,       /* I */
    double *UsageP,   /* O (required) */
    char *EMsg)       /* O (optional,minsize=MMAX_LINE) */

{
  int Count;

  /* param check */
  if ((NULL == VC) || (NULL == Name) || (NULL == UsageP))
    return(FAILURE);

  if (NULL != EMsg)
    EMsg[0] = '\0';

  /* Initialize the Usage and the count values */
  *UsageP = 0.0;
  Count = 0;

  /* Now go walk the MVC tree */
  MVCGMetricHarvestTreeWalk(VC,Name,UsageP,&Count);

  /* at this point the *UsageP value is a 'total' of all
   * threshold. We need to convert to an average by dividing
   * the 'total' by how many instances we found:
   */
  if (0 != Count)
  {
    *UsageP /= Count;
  }
  else
  {
    *UsageP = 0.0;
  }

  return(SUCCESS);
}

