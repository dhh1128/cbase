/* HEADER */

/**
 * @file MGMetric.c
 * 
 */

/*                                           *
 * Contains:                                 *
 *                                           *
 *  int MGMetric funcrions                   *
 *                                           */

#include "moab.h"
#include "moab-proto.h"
#include "moab-const.h"
#include "moab-global.h"



/**
 * Initialize the GMetric structure
 */

int MGMetricInitialize()

  {
  int gindex;

  /* Set default GMetric if it hasn't been previously setup in configuration */

  if (MSched.M[mxoxGMetric] == 0)
    MSched.M[mxoxGMetric] = MDEF_MAXGMETRIC;

  MGMetric.Name = (char **)MUCalloc(1,sizeof(char *) * MSched.M[mxoxGMetric]);
  MGMetric.GMetricReArm = (mulong *)MUCalloc(1,sizeof(mulong) * MSched.M[mxoxGMetric]);
  MGMetric.GMetricLastFired = (mulong *)MUCalloc(1,sizeof(mulong) * MSched.M[mxoxGMetric]);
  MGMetric.GMetricAction = (mbitmap_t *)MUCalloc(1,sizeof(mbitmap_t) * MSched.M[mxoxGMetric]);
  MGMetric.GMetricThreshold = (double *)MUCalloc(1,sizeof(double) * MSched.M[mxoxGMetric]);
  MGMetric.GMetricCmp = (enum MCompEnum *)MUCalloc(1,sizeof(enum MCompEnum) * MSched.M[mxoxGMetric]);

  MGMetric.GMetricActionDetails = (char ***)MUCalloc(1,sizeof(char **) * MSched.M[mxoxGMetric]);

  for (gindex = 0;gindex < MSched.M[mxoxGMetric];gindex++)
    {
    MGMetric.GMetricActionDetails[gindex] = (char **)MUCalloc(1,sizeof(char *) * mgeaLAST);

    MGMetric.Name[gindex] = (char *)MUMalloc(sizeof(char) * MMAX_NAME);
    MGMetric.Name[gindex][0] = '\0';
    }

  MGMetric.GMetricCount = (int *)MUCalloc(1,sizeof(int) * MSched.M[mxoxGMetric]);
  MGMetric.GMetricAThreshold = (int *)MUCalloc(1,sizeof(int) * MSched.M[mxoxGMetric]);
  MGMetric.GMetricChargeRate = (double *)MUCalloc(1,sizeof(double) * MSched.M[mxoxGMetric]);
  MGMetric.GMetricIsCummulative = (mbool_t *)MUCalloc(1,sizeof(mbool_t) * MSched.M[mxoxGMetric]);

  MUStrDup(&MGMetric.Name[0],NONE);

  return(SUCCESS);
  }  /* END MGMetricInitialize() */



/**
 * Create a mgmetric_t structure.
 *
 */

int MGMetricCreate(

  mgmetric_t ***GMP)

  {
  int gindex;

  mgmetric_t **tGM;

  if (*GMP != NULL)
    return(SUCCESS);

  tGM = (mgmetric_t **)MUCalloc(1,sizeof(mgmetric_t *) * MSched.M[mxoxGMetric]);

  for (gindex = 0;gindex < MSched.M[mxoxGMetric];gindex++)
    {
    tGM[gindex] = (mgmetric_t *)MUCalloc(1,sizeof(mgmetric_t));

    tGM[gindex]->IntervalLoad = MDEF_GMETRIC_VALUE;
    }

  *GMP = tGM;

  return(SUCCESS);
  }  /* END MGMetricCreate() */



/**
 * Frees a gmetric_t array.
 *
 * @param GMP (I) [modified] - The array of gmetrics to free
 */

int MGMetricFree(

  mgmetric_t ***GMP)

  {
  int GIndex;

  mgmetric_t **GM = NULL;

  if ((GMP == NULL) || (*GMP == NULL))
    return(SUCCESS);

  GM = *GMP;

  for (GIndex = 0;GIndex < MSched.M[mxoxGMetric];GIndex++)
    {
    if (GM[GIndex] == NULL)
      continue;

    MUFree((char **)&GM[GIndex]);
    }

  MUFree((char **)GMP);

  return(SUCCESS);
  }  /* END MGMetricFree() */



/**
 * Adds the interval loads from one gmetric array to another.
 *
 * @param Dst   (I/O) [modified] - the array to have values added to it
 * @param ToAdd (I) - array of gmetric values to add to Dst
 */

int MGMetricAdd(

  mgmetric_t **Dst,   /* I/O (modified) */
  mgmetric_t **ToAdd) /* I */

  {
  int GIndex;

  if ((Dst == NULL) || (ToAdd == NULL))
    return(FAILURE);

  for (GIndex = 0;GIndex < MSched.M[mxoxGMetric];GIndex++)
    {
    if ((ToAdd[GIndex] == NULL) ||
        (ToAdd[GIndex]->IntervalLoad == MDEF_GMETRIC_VALUE))
      continue;

    if (Dst[GIndex] == NULL)
      Dst[GIndex] = (mgmetric_t *)MUCalloc(1,sizeof(mgmetric_t));

    if (Dst[GIndex]->IntervalLoad == MDEF_GMETRIC_VALUE)
      Dst[GIndex]->IntervalLoad = 0;

    Dst[GIndex]->IntervalLoad += ToAdd[GIndex]->IntervalLoad;
    }  /* END for (GIndex) */

  return(SUCCESS);
  }  /* END MGMetricAdd() */


/**
 * Subtracts the interval loads of one gmetric array from another.
 *
 * @param Dst   (I/O) [modified] - the array to have values subtracted from it
 * @param ToRemove (I) - array of gmetric values to subtract from Dst
 */

int MGMetricSubtract(

  mgmetric_t **Dst,      /* I/O (modified) */
  mgmetric_t **ToRemove) /* I */

  {
  int GIndex;

  if ((Dst == NULL) || (ToRemove == NULL))
    return(FAILURE);

  for (GIndex = 0;GIndex < MSched.M[mxoxGMetric];GIndex++)
    {
    if ((ToRemove[GIndex] == NULL) ||
        (ToRemove[GIndex]->IntervalLoad == MDEF_GMETRIC_VALUE))
      continue;

    if (Dst[GIndex] == NULL)
      Dst[GIndex] = (mgmetric_t *)MUCalloc(1,sizeof(mgmetric_t));

    if (Dst[GIndex]->IntervalLoad == MDEF_GMETRIC_VALUE)
      Dst[GIndex]->IntervalLoad = 0;

    Dst[GIndex]->IntervalLoad -= ToRemove[GIndex]->IntervalLoad;
    }  /* END for (GIndex) */

  return(SUCCESS);
  }  /* END MGMetricSubtract() */


/**
 * Copies gmetric values from Src to Dst.  Both arrays must
 *  have already been initialized.
 *
 * @param Dst (O) - GMetric array to copy to
 * @param Src (I) - GMetric array to copy from
 */

int MGMetricCopy(

  mgmetric_t **Dst,
  mgmetric_t **Src)

  {
  int GIndex;

  if ((Dst == NULL) || (Src == NULL))
    return(FAILURE);

  for (GIndex = 0;GIndex < MSched.M[mxoxGMetric];GIndex++)
    {
    if (Dst[GIndex] == NULL)
      Dst[GIndex] = (mgmetric_t *)MUCalloc(1,sizeof(mgmetric_t));

    if (Src[GIndex] == NULL)
      Dst[GIndex]->IntervalLoad = MDEF_GMETRIC_VALUE;
    else
      Dst[GIndex]->IntervalLoad = Src[GIndex]->IntervalLoad;
    }  /* END for (GIndex) */

  return(SUCCESS);
  }  /* END MGMetricCopy() */

