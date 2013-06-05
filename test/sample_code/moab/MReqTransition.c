/* HEADER */

      
/**
 * @file MReqTransition.c
 *
 * Contains:
 */


#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 * Allocate storage for the given request transition object
 *
 * @param   RP (O) [alloc] pointer to the newly allocated request transition object
 */

int MReqTransitionAllocate(

  mtransreq_t **RP)

  {
  mtransreq_t *R;

  if (RP == NULL)
    {
    return(FAILURE);
    }

  R = (mtransreq_t *)MUCalloc(1,sizeof(mtransreq_t));

  /* Allocate a small starting buffer size upon mtransreq_t allocation */
  R->AllocNodeList = new mstring_t(MMAX_NAME >> 2);


  *RP = R;

  return(SUCCESS);
  } /* END MReqTransitionAllocate() */




/**
 * Free the storage for the given request transition object
 *
 * @param   RQ (I) [freed] pointer to the request transition object
 */

int MReqTransitionFree(

    void **RQ)

  {
  mtransreq_t *R;

  if (RQ == NULL)
    {
    return(FAILURE);
    }

  R = (mtransreq_t *)*RQ;

  /* Free resources */

  delete R->AllocNodeList;
  R->AllocNodeList = NULL;
 
  MUFree(&R->PreferredFeatures);
  MUFree(&R->RequestedApp);
  MUFree(&R->RequestedArch);
  MUFree(&R->ReqOS);
  MUFree(&R->ReqNodeSet);
  MUFree(&R->NodeDisk);
  MUFree(&R->NodeFeatures);
  MUFree(&R->NodeMemory);
  MUFree(&R->NodeSwap);
  MUFree(&R->NodeProcs);
  MUFree(&R->GenericResources);
  MUFree(&R->ConfiguredGenericResources);
  MUFree(&R->Partition);

  MXMLDestroyE(&R->RequestedAttrs);

  MUFree((char **)RQ);

  return(SUCCESS);
  } /* END MReqTransitionFree() */




/**
 * @param SR            (I)
 * @param DR            (O)
 */

int MReqTransitionCopy(

  mtransreq_t *SR,         /* I */
  mtransreq_t *DR)         /* O */

  {
  if ((DR == NULL) || (SR == NULL))
    return(FAILURE);

  memcpy(DR,SR,sizeof(mtransreq_t));

/* This shallow copy is BAD programming. Are callers aware of it????? */
#if 0
  /* DEEP COPY OBJECTS */

  /* Deep copy the AllocNodeList mstring */
  DR->AllocNodeList = new mstring_t(*(SR->AllocNodeList));

  /* Deep copy the various string attributes */

  /* Clear the Dest string pointers, before calling MUStrDup which would free the entries */
  DR->PreferredFeatures = NULL;
  DR->RequestedApp = NULL;
  DR->RequestedArch = NULL;
  DR->ReqOS = NULL;
  DR->ReqNodeSet = NULL;
  DR->NodeDisk = NULL;
  DR->NodeFeatures = NULL;
  DR->NodeMemory = NULL;
  DR->NodeSwap = NULL;
  DR->NodeProcs = NULL;
  DR->GenericResources = NULL;
  DR->ConfiguredGenericResources = NULL;
  DR->Partition = NULL;

  /* now do the strdup */
  MUStrDup(&DR->PreferredFeatures,SR->PreferredFeatures);
  MUStrDup(&DR->RequestedApp,SR->RequestedApp);
  MUStrDup(&DR->RequestedArch,SR->RequestedArch);
  MUStrDup(&DR->ReqOS,SR->ReqOS);
  MUStrDup(&DR->ReqNodeSet,SR->ReqNodeSet);
  MUStrDup(&DR->NodeDisk,SR->NodeDisk);
  MUStrDup(&DR->NodeFeatures,SR->NodeFeatures);
  MUStrDup(&DR->NodeMemory,SR->NodeMemory);
  MUStrDup(&DR->NodeSwap,SR->NodeSwap);
  MUStrDup(&DR->NodeProcs,SR->NodeProcs);
  MUStrDup(&DR->GenericResources,SR->GenericResources);
  MUStrDup(&DR->ConfiguredGenericResources,SR->ConfiguredGenericResources);
  MUStrDup(&DR->Partition,SR->Partition);

  /* Deep copy the XML of Requested Attributes */
  MXMLDupE(SR->RequestedAttrs,&DR->RequestedAttrs);
#endif

  return(SUCCESS);
  } /* END MReqTransitionCopy */ 



/**
 * Store a job request in a transition structure to be stored in the database
 *
 * @see MJobToTransitionStruct  for limitedtransition and ODBC usage
 *
 * @param J                (I) Job the Req belongs to
 * @param R                (I) Req to be stored
 * @param RQT              (O) Transition object
 * @param LimitedTransition
 */

int MReqToTransitionStruct(

  mjob_t        *J,
  mreq_t        *R,
  mtransreq_t   *RQT,
  mbool_t        LimitedTransition)

  {
  MUStrCpy(RQT->JobID,J->Name,MMAX_NAME);

  RQT->Index = R->Index; 

  MReqAToMString(J,R,mrqaAllocNodeList,RQT->AllocNodeList,mfmNONE);
  
  if (R->PtIndex > 0)
    MUStrDup(&RQT->Partition,MPar[R->PtIndex].Name);

  if (R->URes != NULL)
    RQT->UtilSwap = R->URes->Swap;

  /* RQT->MaxNodeCount NYI */
  RQT->MinNodeCount = R->NodeCount; /* mrqaNCReqMin */

  /* RQT->MaxTaskCount NYI */
  RQT->MinTaskCount = R->TaskRequestList[0]; /* mrqaTCReqMin */

  if ((LimitedTransition == TRUE) && (MSched.MDB.DBType != mdbODBC))
    {
    /* we don't support limited updates of the database yet, so if we're
       talking to ODBC then we always have to transition the entire request */

    return(SUCCESS);
    }

  /* these attributes are not set with LimitedTransition */

  mstring_t tmpBuf(MMAX_LINE);

  RQT->NodeAccessPolicy = R->NAccessPolicy;

  if ((MReqAToMString(J,R,mrqaPref,&tmpBuf,mfmNONE) == SUCCESS) && (!tmpBuf.empty()))
    MUStrDup(&RQT->PreferredFeatures,tmpBuf.c_str());

  if ((MReqAToMString(J,R,mrqaReqArch,&tmpBuf,mfmNONE) == SUCCESS) && (!tmpBuf.empty()))
    MUStrDup(&RQT->RequestedArch,tmpBuf.c_str());

  if ((MReqAToMString(J,R,mrqaReqOpsys,&tmpBuf,mfmNONE) == SUCCESS) && (!tmpBuf.empty()))
    MUStrDup(&RQT->ReqOS,tmpBuf.c_str());

  if ((MReqAToMString(J,R,mrqaNodeSet,&tmpBuf,mfmNONE) == SUCCESS) && (!tmpBuf.empty()))
    MUStrDup(&RQT->ReqNodeSet,tmpBuf.c_str());

  RQT->TaskCount = R->TaskCount;
  RQT->TaskCountPerNode = R->TasksPerNode;

  RQT->DiskPerTask = R->DRes.Disk;
  RQT->MemPerTask = R->DRes.Mem;
  RQT->ProcsPerTask = R->DRes.Procs;
  RQT->SwapPerTask = R->DRes.Swap;

  if ((MReqAToMString(J,R,mrqaReqNodeDisk,&tmpBuf,mfmNONE) == SUCCESS) && (!tmpBuf.empty()))
    MUStrDup(&RQT->NodeDisk,tmpBuf.c_str());

  if ((MReqAToMString(J,R,mrqaReqNodeFeature,&tmpBuf,mfmNONE) == SUCCESS) && (!tmpBuf.empty()))
    MUStrDup(&RQT->NodeFeatures,tmpBuf.c_str());

  if ((MReqAToMString(J,R,mrqaReqNodeMem,&tmpBuf,mfmNONE) == SUCCESS) && (!tmpBuf.empty()))
    MUStrDup(&RQT->NodeMemory,tmpBuf.c_str());

  if ((MReqAToMString(J,R,mrqaReqNodeSwap,&tmpBuf,mfmNONE) == SUCCESS) && (!tmpBuf.empty()))
    MUStrDup(&RQT->NodeSwap,tmpBuf.c_str());

  if ((MReqAToMString(J,R,mrqaReqNodeProc,&tmpBuf,mfmNONE) == SUCCESS) && (!tmpBuf.empty()))
    MUStrDup(&RQT->NodeProcs,tmpBuf.c_str());

  if ((MReqAToMString(J,R,mrqaGRes,&tmpBuf,mfmNONE) == SUCCESS) && (!tmpBuf.empty()))
    MUStrDup(&RQT->GenericResources,tmpBuf.c_str());

  if ((MReqAToMString(J,R,mrqaCGRes,&tmpBuf,mfmNONE) == SUCCESS) && (!tmpBuf.empty()))
    MUStrDup(&RQT->ConfiguredGenericResources,tmpBuf.c_str());

  RQT->TasksSpecified  = (bmisset(&J->IFlags,mjifTasksSpecified)) ? TRUE : FALSE;
  RQT->DProcsSpecified = (bmisset(&J->IFlags,mjifDProcsSpecified)) ? TRUE : FALSE;

  if (R->ReqAttr.size() > 0)
    {
    unsigned int jindex;
    mxml_t *RE;

    MXMLCreateE(&RQT->RequestedAttrs,"WorkloadProximityRules");
   
    for (jindex = 0;jindex < R->ReqAttr.size();jindex++)
      {
      RE = NULL;
   
      MXMLCreateE(&RE,"Rule");
      MXMLSetAttr(RE,"attribute",R->ReqAttr[jindex].Name.c_str(),mdfString);
      MXMLSetAttr(RE,"restriction",MAttrAffinityType[R->ReqAttr[jindex].Affinity],mdfString);
      MXMLSetAttr(RE,"operator",MComp[R->ReqAttr[jindex].Comparison],mdfString);
      MXMLSetAttr(RE,"value",R->ReqAttr[jindex].Value.c_str(),mdfString);
      MXMLAddE(RQT->RequestedAttrs,RE);
      }
    }

  return(SUCCESS);
  } /* END MReqToTransitionStruct() */


