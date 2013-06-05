
/* HEADER */

      
/**
 * @file MReqTransitionXML.c
 *
 * Contains:
 */


#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 * get the string version of a given req transition attribute
 *
 * @param RQ      (I) the req transition
 * @param AIndex  (I) the attribute we want
 * @param Buf     (O) the output buffer
 */

int __MReqTransitionAToMString(

  mtransreq_t          *RQ,      /* I */
  enum MReqAttrEnum     AIndex,  /* I */
  mstring_t            *Buf)     /* O */

  {
  if (Buf == NULL)
    {
    return(FAILURE);
    }

  if (RQ == NULL)
    {
    return(FAILURE);
    }

  MStringSet(Buf,"");

  switch (AIndex)
    {
    case mrqaAllocNodeList:

      /* copy the AllocNodeList string to the output 'Buf' */
      *Buf = *RQ->AllocNodeList;

      break;
      
    case mrqaAllocPartition:

      if (!MUStrIsEmpty(RQ->Partition))
        MStringSet(Buf,RQ->Partition);

      break;

    case mrqaGRes:

      if (!MUStrIsEmpty(RQ->GenericResources))
        MStringSet(Buf,RQ->GenericResources);

      break;

    case mrqaCGRes:

      if (!MUStrIsEmpty(RQ->ConfiguredGenericResources))
        MStringSet(Buf,RQ->ConfiguredGenericResources);

      break;

    case mrqaIndex:

      MStringSetF(Buf,"%d",RQ->Index);

      break;

    case mrqaNodeAccess:

      if (RQ->NodeAccessPolicy != mnacNONE)
        MStringSet(Buf,(char *)MNAccessPolicy[RQ->NodeAccessPolicy]);

      break;

    case mrqaNodeSet:

      if (!MUStrIsEmpty(RQ->ReqNodeSet))
        MStringSet(Buf,RQ->ReqNodeSet);

      break;

    case mrqaPref:

      if (!MUStrIsEmpty(RQ->PreferredFeatures))
        MStringSet(Buf,RQ->PreferredFeatures);

      break;

    case mrqaReqArch:

      if (!MUStrIsEmpty(RQ->RequestedArch))
        MStringSet(Buf,RQ->RequestedArch);

      break;

    case mrqaReqOpsys:

      if (!MUStrIsEmpty(RQ->ReqOS))
        MStringSet(Buf,RQ->ReqOS);

      break;

    case mrqaReqNodeFeature:

      if (!MUStrIsEmpty(RQ->NodeFeatures))
        MStringSet(Buf,RQ->NodeFeatures);

      break;

    case mrqaReqNodeDisk:

      if (!MUStrIsEmpty(RQ->NodeDisk))
        MStringSet(Buf,RQ->NodeDisk);

      break;

    case mrqaReqNodeMem:

      if (!MUStrIsEmpty(RQ->NodeMemory))
        MStringSet(Buf,RQ->NodeMemory);

      break;

    case mrqaReqNodeProc:

      if (!MUStrIsEmpty(RQ->NodeProcs))
        MStringSet(Buf,RQ->NodeProcs);

      break;

    case mrqaReqNodeSwap:

      if (!MUStrIsEmpty(RQ->NodeSwap))
        MStringSet(Buf,RQ->NodeSwap);

      break;

    case mrqaReqPartition:

      if (!MUStrIsEmpty(RQ->Partition))
        MStringSet(Buf,RQ->Partition);

      break;

    case mrqaReqDiskPerTask:

      if (RQ->DiskPerTask > 0)
        MStringSetF(Buf,"%d",RQ->DiskPerTask);

      break;

    case mrqaReqMemPerTask:

      if (RQ->MemPerTask > 0)
        MStringSetF(Buf,"%d",RQ->MemPerTask);

      break;

    case mrqaReqSwapPerTask:

      if (RQ->SwapPerTask > 0)
        MStringSetF(Buf,"%d",RQ->SwapPerTask);

      break;

    case mrqaReqProcPerTask:

      if ((RQ->ProcsPerTask != 0) || (RQ->DProcsSpecified == TRUE))
        {
        /* mjifDProcsSpecified is needed for xt sizezero jobs to show
         * correctly when exported to the grid head */

        MStringSetF(Buf,"%d%s",
          RQ->ProcsPerTask,
          RQ->DProcsSpecified == TRUE ? "*" : "");
        }

      break;

    case mrqaNCReqMin:

      if (RQ->MinNodeCount > 0)
        MStringSetF(Buf,"%d",RQ->MinNodeCount);

      break;

    case mrqaTCReqMin:

      if (RQ->MinTaskCount != 0)
        {
        /* mjifTasksSpecified is needed for xt sizezero jobs to show
         * correctly when exported to the grid head */

        MStringSetF(Buf,"%d%s",
          RQ->MinTaskCount,
          RQ->TasksSpecified ? "*" : "");
        }

      break;

    case mrqaTPN:

      if (RQ->TaskCountPerNode > 0)
        {
        MStringSetF(Buf,"%d",RQ->TaskCountPerNode);
        }

      break;

    case mrqaUtilSwap:

      if (RQ->UtilSwap != 0)
        {
        MStringAppendF(Buf,"%d",RQ->UtilSwap);
        }

      break;

    default:

      /* no-op -- Not supported */

      MDB(7,fSCHED) MLog("WARNING:  Req attribute %s not yet translated to string value.\n",MReqAttr[AIndex]);

      break;

    } /* END switch(AIndex) */

  return(SUCCESS);
  } /* END __MReqTransitionAToMString() */


/**
 * Store a req transition object in the given XML object
 *
 * @param R        (I) the req transition object
 * @param RQE      (O) the XML object to store it in
 * @param SRQAList (I) the request attributes we want to include
 */

int MReqTransitionToXML(

  mtransreq_t        *R,
  mxml_t            **RQE,
  enum MReqAttrEnum  *SRQAList)

  {
  const enum MReqAttrEnum DRQAList[] = {
    mrqaIndex,
    mrqaGRes,
    mrqaNCReqMin,
    mrqaNodeAccess,
    mrqaReqPartition,
    mrqaReqProcPerTask,
    mrqaTCReqMin,
    mrqaIndex,
    mrqaNONE };

  int aindex;

  mxml_t *RE;

  enum MReqAttrEnum *RQAList;

  *RQE = NULL;

  if (MXMLCreateE(RQE,(char *)MXO[mxoReq]) == FAILURE)
    {
    return(FAILURE);
    }

  mstring_t tmpBuf(MMAX_LINE);

  RE = *RQE;

  if (SRQAList != NULL)
    RQAList = SRQAList;
  else
    RQAList = (enum MReqAttrEnum *)DRQAList;

  for (aindex = 0; RQAList[aindex] != mrqaNONE; aindex++)
    {
    if (RQAList[aindex] == mrqaReqAttr)
      {
      mxml_t *VE = NULL;
   
      MXMLDupE(R->RequestedAttrs,&VE);

      MXMLAddE(RE,VE);

      continue;
      }

    __MReqTransitionAToMString(R,
      RQAList[aindex],
      &tmpBuf);

    if (!tmpBuf.empty())
      {
      MXMLSetAttr(RE,(char *)MReqAttr[RQAList[aindex]],tmpBuf.c_str(),mdfString);
      }
    } /* END for(aindex...) */

  return(SUCCESS);
  } /* END MReqTransitionToXML() */
