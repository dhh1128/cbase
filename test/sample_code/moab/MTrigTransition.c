/* HEADER */

      
/**
 * @file MTrigTransition.c
 *
 * Contains: Trigger Transition functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 * Store trigger in transition structure for storage in 
 * database. 
 *
 * @see MReqToTransitionStruct (child)
 * 
 * @param   ST (I) the trigger to store
 * @param   DT (O) the transition object to store it in
 */

int MTrigToTransitionStruct(

  mtrig_t      *ST,
  mtranstrig_t *DT)

  {
  char tmpLine[MMAX_LINE];

  MTrigAToString(ST,mtaTrigID,DT->TrigID,MMAX_LINE,mfmNONE);
  MTrigAToString(ST,mtaName,tmpLine,sizeof(tmpLine),mfmNONE);
  MUStrDup(&DT->Name,tmpLine);
  MTrigAToString(ST,mtaActionData,tmpLine,sizeof(tmpLine),mfmNONE);
  MUStrDup(&DT->ActionData,tmpLine);
  MTrigAToString(ST,mtaActionType,tmpLine,sizeof(tmpLine),mfmNONE);
  MUStrDup(&DT->ActionType,tmpLine);
  MTrigAToString(ST,mtaDescription,tmpLine,sizeof(tmpLine),mfmNONE);
  MUStrDup(&DT->Description,tmpLine);
  MTrigAToString(ST,mtaEBuf,tmpLine,sizeof(tmpLine),mfmNONE);
  MUStrDup(&DT->EBuf,tmpLine);
  MTrigAToString(ST,mtaEventType,tmpLine,sizeof(tmpLine),mfmNONE);
  MUStrDup(&DT->EventType,tmpLine);
  MTrigAToString(ST,mtaFailOffset,tmpLine,sizeof(tmpLine),mfmNONE);
  MUStrDup(&DT->FailOffset,tmpLine);
  MTrigAToString(ST,mtaFlags,tmpLine,sizeof(tmpLine),mfmNONE);
  MUStrDup(&DT->Flags,tmpLine);
  MTrigAToString(ST,mtaIBuf,tmpLine,sizeof(tmpLine),mfmNONE);
  MUStrDup(&DT->IBuf,tmpLine);
  MTrigAToString(ST,mtaMessage,tmpLine,sizeof(tmpLine),mfmNONE);
  MUStrDup(&DT->Message,tmpLine);
  MTrigAToString(ST,mtaMultiFire,tmpLine,sizeof(tmpLine),mfmNONE);
  MUStrDup(&DT->MultiFire,tmpLine);
  MTrigAToString(ST,mtaObjectID,tmpLine,sizeof(tmpLine),mfmNONE);
  MUStrDup(&DT->ObjectID,tmpLine);
  MTrigAToString(ST,mtaObjectType,tmpLine,sizeof(tmpLine),mfmNONE);
  MUStrDup(&DT->ObjectType,tmpLine);
  MTrigAToString(ST,mtaOBuf,tmpLine,sizeof(tmpLine),mfmNONE);
  MUStrDup(&DT->OBuf,tmpLine);
  MTrigAToString(ST,mtaOffset,tmpLine,sizeof(tmpLine),mfmNONE);
  MUStrDup(&DT->Offset,tmpLine);
  MTrigAToString(ST,mtaPeriod,tmpLine,sizeof(tmpLine),mfmNONE);
  MUStrDup(&DT->Period,tmpLine);
  MTrigAToString(ST,mtaRequires,tmpLine,sizeof(tmpLine),mfmNONE);
  MUStrDup(&DT->Requires,tmpLine);
  MTrigAToString(ST,mtaSets,tmpLine,sizeof(tmpLine),mfmNONE);
  MUStrDup(&DT->Sets,tmpLine);
  MTrigAToString(ST,mtaState,tmpLine,sizeof(tmpLine),mfmNONE);
  MUStrDup(&DT->State,tmpLine);
  MTrigAToString(ST,mtaThreshold,tmpLine,sizeof(tmpLine),mfmNONE);
  MUStrDup(&DT->Threshold,tmpLine);
  MTrigAToString(ST,mtaUnsets,tmpLine,sizeof(tmpLine),mfmNONE);
  MUStrDup(&DT->Unsets,tmpLine);

  DT->Disabled              = (bmisset(&ST->InternalFlags,mtifDisabled));
  DT->FailureDetected       = (bmisset(&ST->InternalFlags,mtifFailureDetected));
  DT->EventTime             = ST->ETime;
  DT->IsComplete            = (bmisset(&ST->InternalFlags,mtifIsComplete));
  DT->IsInterval            = (bmisset(&ST->InternalFlags,mtifIsInterval));
  DT->LaunchTime            = ST->LaunchTime;
  DT->BlockTime             = ST->BlockTime;
  DT->PID                   = ST->PID;
  DT->RearmTime             = ST->RearmTime;
  DT->Timeout               = ST->Timeout;

  return(SUCCESS);
  }  /* END MTrigToTransitionStruct() */



/**
 * @param ST            (I)
 * @param DT            (O)
 */

int MTrigTransitionCopy(

  mtranstrig_t *ST,
  mtranstrig_t *DT)

  {

  if ((DT == NULL) || (ST == NULL))
    return(FAILURE);

  memcpy(DT,ST,sizeof(mtransrsv_t));

  return(SUCCESS);
  } /* END MTrigTransitionCopy */ 




/**
 * check to see if a given trigger matches a list of constraints 
 *
 * @param T                (I)
 * @param TConstraintList  (I)
 */
mbool_t MTrigTransitionMatchesConstraints(

    mtranstrig_t *T,
    marray_t     *TConstraintList)

  {
  int cindex;
  mtrigconstraint_t *TConstraint;

  if (T == NULL)
    {
    return(FALSE);
    }

  if ((TConstraintList == NULL) || (TConstraintList->NumItems == 0))
    {
    return(TRUE);
    }

  MDB(7,fSCHED) MLog("INFO:     Checking trigger %s against constraint list.\n",
    T->TrigID);

  for (cindex = 0; cindex < TConstraintList->NumItems; cindex++)
    {
    TConstraint = (mtrigconstraint_t *)MUArrayListGet(TConstraintList,cindex);

    if (strcmp(T->TrigID,TConstraint->AVal[0]) != 0)
      {
      return(FALSE);
      }
    }

  return(TRUE);
  } /* END MTrigTransitionMatchesConstraints */




/**
 * Store a trigger transition object in the given XML object.
 *
 * @param   T        (I) the trigger transition object
 * @param   TEP      (O) the XML object to store it in
 * @param   STAList  (I) [optional] 
 * @param   SRQAList (I) [optional] 
 * @param   CFlags   (I)
 */

int MTrigTransitionToXML(

  mtranstrig_t        *T,
  mxml_t              **TEP,
  enum MTrigAttrEnum  *STAList,
  enum MReqAttrEnum   *SRQAList,
  long                CFlags)

  {
  mxml_t *TE;     /* element for the reservation */

  int aindex;

  enum MTrigAttrEnum *TList;


  const enum MTrigAttrEnum DAList[] = {
    mtaActionData,
    mtaActionType,
    mtaBlockTime,
    mtaDescription,
    mtaDisabled,
    mtaEBuf,
    mtaEventTime,
    mtaEventType,
    mtaFailOffset,
    mtaFailureDetected,
    mtaFlags,
    mtaIBuf,
    mtaIsComplete,
    mtaIsInterval,
    mtaLaunchTime,
    mtaMessage,
    mtaMultiFire,
    mtaName,
    mtaObjectID,
    mtaObjectType,
    mtaOBuf,
    mtaOffset,
    mtaPeriod,
    mtaPID,
    mtaRearmTime,
    mtaRequires,
    mtaSets,
    mtaState,
    mtaThreshold,
    mtaTimeout,
    mtaTrigID,
    mtaUnsets,
    mtaNONE };

  *TEP = NULL;

  if (MXMLCreateE(TEP,(char *)MXO[mxoTrig]) == FAILURE)
    {
    return(FAILURE);
    }

  TE = *TEP;

  if (STAList != NULL)
    TList = STAList;
  else
    TList = (enum MTrigAttrEnum *)DAList;

  mstring_t tmpBuf(MMAX_LINE);

  for (aindex = 0; TList[aindex] != mtaNONE; aindex++)
    {
    MTrigTransitionAToString(T,TList[aindex],&tmpBuf,mfmNONE);

    if (!MUStrIsEmpty(tmpBuf.c_str()))
      MXMLSetAttr(TE,(char *)MTrigAttr[TList[aindex]],(void *)tmpBuf.c_str(),mdfString);
      
    MStringSet(&tmpBuf,"");
    } /* END for (AIndex...) */

  return(SUCCESS);
  }  /* END MTrigTransitionToXML() */




/**
 * Allocate storage for the given trigger transition object
 *
 * @param   TP (O) [alloc-ed] pointer to the newly allocated 
 *                 trigger transition object
 */

int MTrigTransitionAllocate(

  mtranstrig_t **TP)

  {
  mtranstrig_t *T;

  if (TP == NULL)
    {
    return(FAILURE);
    }

  T = (mtranstrig_t *)MUCalloc(1,sizeof(mtranstrig_t));

  *TP = T;

  return(SUCCESS);
  }  /* END MTrigTransitionAllocate() */




/**
 * Free the storage for the given trigger transtion object
 *
 * @param   TP (I) [freed] pointer to the trigger transition
 *                 object
 */

int MTrigTransitionFree(

  void **TP)

  {
  mtranstrig_t *T;

  if (TP == NULL)
    {
    return(FAILURE);
    }

  T= (mtranstrig_t *)*TP;

  MUFree(&T->Name);
  MUFree(&T->ActionData);
  MUFree(&T->ActionType);
  MUFree(&T->Description);
  MUFree(&T->EBuf);
  MUFree(&T->EventType);
  MUFree(&T->FailOffset);
  MUFree(&T->Flags);
  MUFree(&T->IBuf);
  MUFree(&T->Message);
  MUFree(&T->MultiFire);
  MUFree(&T->ObjectID);
  MUFree(&T->ObjectType);
  MUFree(&T->OBuf);
  MUFree(&T->Offset);
  MUFree(&T->Period);
  MUFree(&T->Requires);
  MUFree(&T->Sets);
  MUFree(&T->State);
  MUFree(&T->Threshold);
  MUFree(&T->Unsets);

  /* free the actual trigger transition */
  MUFree((char **)TP);

  return(SUCCESS);
  }  /* END MTrigTransitionFree() */




/**
 * transition a trigger to the queue to be written to the 
 * database 
 *
 * @see MTrigToTransitionStruct (child) 
 *      MReqToTransitionStruct (child)
 *
 * @param   T (I) the trigger to be transitioned
 */

int MTrigTransition(

  mtrig_t *T)

  {
  mtranstrig_t *TT;

#if defined (__NOMCOMMTHREAD)
  return(SUCCESS);
#endif

  MTrigTransitionAllocate(&TT);
  
  MTrigToTransitionStruct(T,TT);

  MOExportTransition(mxoTrig,TT);

  return(SUCCESS);
  }  /* END MTrigTransition() */
/* END MTrigTransition.c */
