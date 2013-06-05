/* HEADER */

/**
 * @file MVMGMetric.c
 * 
 * Contains various functions for  VM GMetric 
 *
 */


#include "moab.h"
#include "moab-proto.h"
#include "moab-const.h"
#include "moab-global.h"




/**
 * Set generic metric to specified value on VM (based MNodeSetGMetric)
 *
 * @param VM      (I) [modified] -> VM to put the GMetric on
 * @param GMIndex (I) -> Index of the GMetric to change
 * @param Value   (I) -> Value to set to (will be converted to double)
 */

int MVMSetGMetric(

  mvm_t  *VM,      /* I (modified) */
  int     GMIndex, /* I */
  char   *Value)   /* I */

  {
  double DVal;

  if ((VM == NULL)   ||
      (GMIndex <= 0) ||
      (GMIndex >= MSched.M[mxoxGMetric]) ||
      (Value == NULL))
    {
    return(FAILURE);
    }

  MGMetricCreate(&VM->GMetric);

  DVal = strtod(Value,NULL);

  if (DVal == MDEF_GMETRIC_VALUE)
    return(FAILURE);

  VM->GMetric[GMIndex]->IntervalLoad = DVal;
  VM->GMetric[GMIndex]->SampleCount++; 

  if ((MGMetric.GMetricThreshold[GMIndex] != 0.0) &&
      (MUDCompare(
         DVal,
         MGMetric.GMetricCmp[GMIndex],
         MGMetric.GMetricThreshold[GMIndex])) &&
      (VM->GMetric[GMIndex]->EMTime + MGMetric.GMetricReArm[GMIndex] <= MSched.Time))
    {
    VM->GMetric[GMIndex]->EMTime = MSched.Time;

    MGEventProcessGEvent(
      GMIndex,
      mxoxVM,
      VM->VMID,
      NULL,
      MGMetric.GMetricThreshold[GMIndex],
      mrelGEvent,
      Value);
    }

  return(SUCCESS);
  } /* END MVMSetGMetric() */




/**
 * Report specified VM GMetric attribute as string
 *
 * @see MVMAToString() - parent
 *
 * @param GMetric (I)
 * @param Buf     (O) [minsize=MMAX_LINE]
 * @param BufSize (I) [optional,-1 if not set,default=MMAX_LINE]
 * @param DModeBM (I) [bitmap of enum MCModeEnum - mcmVerbose,mcmXML,mcmUser=human_readable]
 */

int MVMGMetricAToString(

  mgmetric_t **GMetric,
  char        *Buf,
  int          BufSize,
  mbitmap_t   *DModeBM)

  {
  char *BPtr = NULL;
  int   BSpace = 0;
  int   gmindex;

  /* FORMAT:  GMETRIC=<GMNAME>:<VAL>[,<GMNAME>:<VAL>]... */

  MUSNInit(&BPtr,&BSpace,Buf,BufSize);

  if (GMetric != NULL)
    {
    for (gmindex = 1;gmindex < MSched.M[mxoxGMetric];gmindex++)
      {
      if (GMetric[gmindex] == NULL)
        {
        continue;
        }

      if (MGMetric.Name[gmindex][0] == '\0')
        break;

      if (GMetric[gmindex]->SampleCount <= 0)
        continue; 

      if (Buf[0] != '\0')
        MUSNCat(&BPtr,&BSpace,",");

      if (bmisset(DModeBM,mcmUser))
        {
        /* NOTE:  0th entry in MAList is always "NONE" */
        MUSNPrintF(&BPtr,&BSpace,"%s=%.2f",
          MGMetric.Name[gmindex],
          GMetric[gmindex]->IntervalLoad);
        }
      else
        {
        MUSNPrintF(&BPtr,&BSpace,"%s:%.2f",
          MGMetric.Name[gmindex],
          GMetric[gmindex]->IntervalLoad);
        }
      }  /* END for (gmindex) */
    }  /* END if (GMetric != NULL) */

  return(SUCCESS);
  } /* END MVMGMetricAToString() */

/* END MVMGMetric.c */
