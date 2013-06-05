/* HEADER */

/**
 * @file MVMTransitionXML.c
 * 
 * Contains various functions VM to/from XML
 *
 */


#include "moab.h"
#include "moab-proto.h"
#include "moab-const.h"
#include "moab-global.h"


/**
 * Report the transition VM as XML
 *
 * @param V      (I)
 * @param VEP    (I/O) [modified,alloc]
 * @param SAList (I) [optional,terminated w/mvmaLAST]
 */

int MVMTransitionToXML(

  mtransvm_t       *V,
  mxml_t          **VEP,
  enum MVMAttrEnum *SAList)

  {
  char tmpString[MMAX_LINE];

  mxml_t *VE;     /* element for the VM */

  enum MVMAttrEnum *AList;  /**< VM attribute list */

  int aindex;

  const enum MVMAttrEnum DAList[] = {
    mvmaActiveOS,
    mvmaADisk,
    mvmaAlias,
    mvmaAMem,
    mvmaAProcs,
    mvmaCDisk,
    mvmaCMem,
    mvmaCProcs,
    mvmaCPULoad,
    mvmaContainerNode,
    mvmaDDisk,
    mvmaDMem,
    mvmaDProcs,
    mvmaDescription,
    mvmaEffectiveTTL,
    mvmaFlags,
    mvmaGMetric,
    mvmaID,
    mvmaJobID,
    mvmaLastMigrateTime,
    mvmaLastSubState,
    mvmaLastSubStateMTime,
    mvmaLastUpdateTime,
    mvmaMigrateCount,
    mvmaNetAddr,
    mvmaNextOS,
    mvmaOSList,
    mvmaPowerIsEnabled,
    mvmaPowerState,
    mvmaProvData,
    mvmaRackIndex,
    mvmaSlotIndex,
    mvmaSovereign,
    mvmaSpecifiedTTL,
    mvmaStartTime,
    mvmaState,
    mvmaSubState,
    mvmaStorageRsvNames,
    mvmaTrackingJob,
    mvmaVariables,
    mvmaLAST };

  *VEP = NULL;

  if ((V == NULL) || (VEP == NULL))
    {
    return(FAILURE);
    }

  if (MXMLCreateE(VEP,(char *)MXO[mxoxVM]) == FAILURE)
    {
    return(FAILURE);
    }

  VE = *VEP;
 
  if (SAList != NULL)
    AList = SAList;
  else
    AList = (enum MVMAttrEnum *)DAList;

  for (aindex = 0;AList[aindex] != mvmaLAST;aindex++)
    {
    if (AList[aindex] == mvmaVariables)
      {
      mxml_t *VarE = NULL;

      MXMLDupE(V->Variables,&VarE);

      MXMLAddE(VE,VarE);

      continue;
      }

    if ((MVMTransitionAToString(V,AList[aindex],tmpString,sizeof(tmpString)) == FAILURE) ||
        (tmpString[0] == '\0'))
      {
      continue;
      }

    MXMLSetAttr(VE,(char *)MVMAttr[AList[aindex]],tmpString,mdfString);
    }  /* END for (aindex) */

  return(SUCCESS);
  }  /* END MVMTransitionToXML() */

/* END MVMTransitionXML.c */
