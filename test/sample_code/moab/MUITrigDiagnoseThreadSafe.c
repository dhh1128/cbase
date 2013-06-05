/* HEADER */

/**
 * @file MUITrigDiagnoseThreadSafe.c
 *
 * Contains MUI Trigger Diagnose Thread Safe function
 */



#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 * process mdiag -T request in a threadsafe way.
 *
 * @param S       (I) [modified]
 * @param CFlagBM (I) enum MRoleEnum bitmap
 * @param Auth    (I) authorization credentials for request
 */

int MUITrigDiagnoseThreadSafe(

  msocket_t *S,       /* I (modified) */
  mbitmap_t *CFlagBM, /* I enum MRoleEnum bitmap */
  char      *Auth)    /* I */

  {
  int aindex;

  char DiagOpt[MMAX_LINE];
  char FlagString[MMAX_LINE];
  char TrigID[MMAX_LINE];

  marray_t TConstraintList;
  mrsvconstraint_t TConstraint;

  mxml_t *DE;
  mxml_t *TE;

  mtranstrig_t *T;

  marray_t TList;

  mdb_t *MDBInfo;

  mgcred_t *U;

  mbitmap_t CFlags;  /* bitmap of enum MCModeEnum */

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

  TrigID[0] = '\0';

  MUArrayListCreate(&TList,sizeof(mtranstrig_t *),10);

  MSchedGetAttr(msaDBHandle,mdfNONE,(void **)&MDBInfo,-1);

  if (MXMLGetAttr(S->RDE,MSAN[msanFlags],NULL,FlagString,sizeof(FlagString)) == SUCCESS)
    bmfromstring(FlagString,MClientMode,&CFlags);

  /* Get reservation name provided by user */

  MUArrayListCreate(&TConstraintList,sizeof(mrsvconstraint_t),10);  

  if (MXMLGetAttr(S->RDE,MSAN[msanOp],NULL,DiagOpt,sizeof(DiagOpt)) == SUCCESS)
    {
    char *TokPtr;
    char *ptr;

    /* Build job constraint list to only get job specified */
          
    TConstraint.AType = mraName;
    TConstraint.ACmp = mcmpEQ2;

    for (ptr = MUStrTok(DiagOpt,",",&TokPtr);ptr != NULL;ptr = MUStrTok(NULL,",",&TokPtr))
      {
      MUStrCpy(TConstraint.AVal[0],ptr,sizeof(TConstraint.AVal[0]));
    
      MUArrayListAppend(&TConstraintList,(void *)&TConstraint);
      }
    }

  MCacheTriggerLock(TRUE,TRUE);
  MCacheReadTriggers(&TList,&TConstraintList);

  if (MXMLCreateE(&S->SDE,(char *)MSON[msonData]) == FAILURE)
    {
    MUISAddData(S,"ERROR:    cannot create response\n");

    MCacheTriggerLock(FALSE,TRUE);
    MUArrayListFree(&TList);
    MUArrayListFree(&TConstraintList);
    return(FAILURE);
    }

  DE = S->SDE;

  /* find the user who issued the request */

  MUserAdd(Auth,&U);

  /* get the argument flags for the request */

  if (MXMLGetAttr(S->RDE,MSAN[msanFlags],NULL,FlagString,sizeof(FlagString)) == SUCCESS)
    bmfromstring(FlagString,MClientMode,&CFlags);

  /* look for a specified trigger id */

  if (MXMLGetAttr(S->RDE,MSAN[msanOp],NULL,DiagOpt,sizeof(DiagOpt)) == SUCCESS)
    {
    MUStrCpy(TrigID,DiagOpt,sizeof(TrigID));
    }

  /* loop through all reservations and create an xml element for each, adding
   * it to the return data element */

  /* NOTE/FIXME:  When we have finished storing the reservations in the db/cache,
   *        this loop will need to be replaced by a call to
   *        MCacheReadReservations, and loop through the list returned from
   *        that function */

  for (aindex = 0;aindex < TList.NumItems;aindex++)
    {
    T = (mtranstrig_t *)MUArrayListGetPtr(&TList,aindex);

    if ((T == NULL) || (MUStrIsEmpty(T->TrigID)))
      break;

    if (T->TrigID[0] == '\1')
      continue;

    if ((!MUStrIsEmpty(TrigID)) && (strcmp(TrigID,T->TrigID) != 0))
      continue;

    if (MUICheckAuthorization(
          U,
          NULL,
          (void *)T,
          mxoTrig,
          mcsDiagnose,
          mrcmQuery,
          NULL,
          NULL,
          0) == FAILURE)
      {
      /* no authority to diagnose reservation */

      continue;
      } /* END if (MUICheckAuthorization...) */

    MDB(6,fUI) MLog("INFO:     evaluating Trigger '%s'\n",
      T->TrigID);

    TE = NULL;

    MXMLCreateE(&TE,(char *)MXO[mxoTrig]);

    MTrigTransitionToXML(
      T,
      &TE,
      (enum MTrigAttrEnum *)DAList,
      NULL,
      mfmNONE);

    MXMLAddE(DE,TE);
    }  /* END for (rindex) */

  MCacheTriggerLock(FALSE,TRUE);
  MUArrayListFree(&TList);
  MUArrayListFree(&TConstraintList);

  return(SUCCESS);
  } /* end MUITrigDiagnoseThreadSafe() */
/* END MUITrigDiagnoseThreadSafe.c */
