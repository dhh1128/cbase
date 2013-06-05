/* HEADER */

      
/**
 * @file MTrigIs.c
 *
 * Contains: various functions that tests triggers
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 * Determines if two triggers are equal or not?
 *
 * Not all checks are implemented. 
 *
 * @param Trig1
 * @param Trig2
 */

mbool_t MTrigIsEqual(

  mtrig_t *Trig1,
  mtrig_t *Trig2)

  {
  int tindex;
  mbool_t IsEqual = TRUE;

  enum MTrigAttrEnum TrigAttrList[] = {
    mtaActionType,
    mtaActionData,
    mtaBlockTime,
    mtaDependsOn,
    mtaDisabled,
    mtaDescription,
    mtaECPolicy,
    mtaEnv,
    mtaEventType,
    mtaEventTime,
    mtaExpireTime,
    mtaFlags,
    mtaIsComplete,
    mtaIsInterval,
    mtaMaxRetry,
    mtaMessage,
    mtaMultiFire,
    mtaName,
    mtaOffset,
    mtaObjectID,
    mtaPeriod,
    mtaRearmTime,
    mtaRequires,
    mtaSets,
    mtaStdIn,
    mtaThreshold,
    mtaTimeout,
    mtaFailOffset,
    mtaUnsets,
    mtaNONE };

  if ((Trig1 == NULL) || (Trig2 == NULL))
    return(FALSE);

  for (tindex = TrigAttrList[0];TrigAttrList[tindex] != mtaNONE;tindex++)
    {
    switch (tindex)
      {
      case mtaActionType:

        if (Trig1->AType != Trig2->AType)
          IsEqual = FALSE;

        break;

      case mtaActionData:

        if (((Trig1->Action == NULL) && (Trig2->Action != NULL)) ||
            ((Trig1->Action != NULL) && (Trig2->Action == NULL)))
          IsEqual = FALSE;

        if (((Trig1->Action != NULL) && (Trig2->Action != NULL)) &&
            (strcmp(Trig1->Action,Trig2->Action) != 0))
            IsEqual = FALSE;

        break;

      case mtaBlockTime:

        if (Trig1->BlockTime != Trig2->BlockTime)
          IsEqual = FALSE;

        break;

      case mtaDependsOn:
        
        if (((Trig1->TrigDepString == NULL) && (Trig2->TrigDepString != NULL)) ||
            ((Trig1->TrigDepString != NULL) && (Trig2->TrigDepString == NULL)))
          IsEqual = FALSE;

        if (((Trig1->TrigDepString != NULL) && (Trig2->TrigDepString != NULL)) &&
            (strcmp(Trig1->TrigDepString->c_str(),Trig2->TrigDepString->c_str()) != 0))
            IsEqual = FALSE;

        break;

      case mtaDisabled:

        if (bmisset(&Trig1->InternalFlags,mtifDisabled) != 
            bmisset(&Trig2->InternalFlags,mtifDisabled))
          IsEqual = FALSE;

        break;

      case mtaDescription:

        if (((Trig1->Description == NULL) && (Trig2->Description != NULL)) ||
            ((Trig1->Description != NULL) && (Trig2->Description == NULL)))
          IsEqual = FALSE;

        if (((Trig1->Description != NULL) && (Trig2->Description != NULL)) &&
            (strcmp(Trig1->Description,Trig2->Description) != 0))
            IsEqual = FALSE;

        break;

      case mtaECPolicy:
        
        if (Trig1->ECPolicy != Trig2->ECPolicy)
          IsEqual = FALSE;
        
        break;

      case mtaEnv:

        if (((Trig1->Env == NULL) && (Trig2->Env != NULL)) ||
            ((Trig1->Env != NULL) && (Trig2->Env == NULL)))
          IsEqual = FALSE;

        if (((Trig1->Env != NULL) && (Trig2->Env != NULL)) &&
            (strcmp(Trig1->Env,Trig2->Env) != 0))
            IsEqual = FALSE;

        break;

      case mtaEventType:
        
        if (Trig1->EType != Trig2->EType)
          IsEqual = FALSE;
        
        break;

      case mtaEventTime:

        /*
        if (Trig1->ETime != Trig2->ETime)
          IsEqual = FALSE;
          */
        
        break;

      case mtaExpireTime:

        if (Trig1->ExpireTime != Trig2->ExpireTime)
          IsEqual = FALSE;

        break;

      case mtaFlags:
        
        /* 
        if (Trig1->SFlags != Trig2->SFlags)
          IsEqual = FALSE;
          */
        
        if (((Trig1->GetVars == NULL) && (Trig2->GetVars != NULL)) ||
            ((Trig1->GetVars != NULL) && (Trig2->GetVars == NULL)))
          IsEqual = FALSE;

        if (((Trig1->GetVars != NULL) && (Trig2->GetVars != NULL)) &&
            (strcmp(Trig1->GetVars,Trig2->GetVars) != 0))
            IsEqual = FALSE;

        break;

      case mtaIsComplete:
        
        if (bmisset(&Trig1->InternalFlags,mtifIsComplete) != 
            bmisset(&Trig2->InternalFlags,mtifIsComplete))
          IsEqual = FALSE;

        break;

      case mtaIsInterval:
        
        if (bmisset(&Trig1->InternalFlags,mtifIsInterval) != 
            bmisset(&Trig2->InternalFlags,mtifIsInterval))
          IsEqual = FALSE;

        break;
        
      case mtaMaxRetry:
        
        if (Trig1->MaxRetryCount != Trig2->MaxRetryCount)
          IsEqual = FALSE;

        break;

      case mtaMessage:

        if (((Trig1->Msg == NULL) && (Trig2->Msg != NULL)) ||
            ((Trig1->Msg != NULL) && (Trig2->Msg == NULL)))
          IsEqual = FALSE;

        if (((Trig1->Msg != NULL) && (Trig2->Msg != NULL)) &&
            (strcmp(Trig1->Msg,Trig2->Msg) != 0))
            IsEqual = FALSE;

        break;

      case mtaMultiFire:
        
        if (bmisset(&Trig1->InternalFlags,mtifMultiFire) != 
            bmisset(&Trig2->InternalFlags,mtifMultiFire))
          IsEqual = FALSE;

        break;

      case mtaName:

        if (((Trig1->Name == NULL) && (Trig2->Name != NULL)) ||
            ((Trig1->Name != NULL) && (Trig2->Name == NULL)))
          IsEqual = FALSE;

        if (((Trig1->Name != NULL) && (Trig2->Name != NULL)) &&
            (strcmp(Trig1->Name,Trig2->Name) != 0))
            IsEqual = FALSE;

        break;

      case mtaOffset:

        if (Trig1->Offset != Trig2->Offset)
          IsEqual = FALSE;

        break;

      case mtaObjectID:

        if (((Trig1->OID == NULL) && (Trig2->OID != NULL)) ||
            ((Trig1->OID != NULL) && (Trig2->OID == NULL)))
          IsEqual = FALSE;

        if (((Trig1->OID != NULL) && (Trig2->OID != NULL)) &&
            (strcmp(Trig1->OID,Trig2->OID) != 0))
            IsEqual = FALSE;

        break;

      case mtaPeriod:
        
        if (Trig1->Period != Trig2->Period)
          IsEqual = FALSE;

        break;

      case mtaRearmTime:
        
        if (Trig1->RearmTime != Trig2->RearmTime)
          IsEqual = FALSE;

        break;

      case mtaRequires:

        /* Trig1->Depend != Trig2->Depend */

        break;
        
      case mtaSets:

        /* need to create MUArrayListIsEqual() before
         * the below code will work */

        /* 
        if (Trig1->FSetsFlags != Trig2->FSetsFlags)
          IsEqual = FALSE;
        */

        /* Trig1->FSets != Trig2->FSets */

        /*
        if (Trig1->SSetsFlags != Trig2->SSetsFlags)
          IsEqual = FALSE;
        */
        
        /* Trig1->SSets != Trig2->SSets */

        break;
        
      case mtaStdIn:

        if (((Trig1->StdIn == NULL) && (Trig2->StdIn != NULL)) ||
            ((Trig1->StdIn != NULL) && (Trig2->StdIn == NULL)))
          IsEqual = FALSE;

        if (((Trig1->StdIn != NULL) && (Trig2->StdIn != NULL)) &&
            (strcmp(Trig1->StdIn,Trig2->StdIn) != 0))
            IsEqual = FALSE;

        break;

      case mtaThreshold:

        if (Trig1->Threshold != Trig2->Threshold)
          IsEqual = FALSE;

        if (bmisset(&Trig1->InternalFlags,mtifThresholdIsPercent) != 
            bmisset(&Trig2->InternalFlags,mtifThresholdIsPercent))
          IsEqual = FALSE;

        if (Trig1->ThresholdType != Trig2->ThresholdType)
          IsEqual = FALSE;

        if (Trig1->ThresholdCmp != Trig2->ThresholdCmp)
          IsEqual = FALSE;

        if (((Trig1->ThresholdGMetricType == NULL) && (Trig2->ThresholdGMetricType != NULL)) ||
            ((Trig1->ThresholdGMetricType != NULL) && (Trig2->ThresholdGMetricType == NULL)))
          IsEqual = FALSE;

        if (((Trig1->ThresholdGMetricType != NULL) && (Trig2->ThresholdGMetricType != NULL)) &&
            (strcmp(Trig1->ThresholdGMetricType,Trig2->ThresholdGMetricType) != 0))
            IsEqual = FALSE;

        break;

      case mtaTimeout:

        if (Trig1->Timeout != Trig2->Timeout)
          IsEqual = FALSE;

        break;

      case mtaFailOffset:

        if (Trig1->FailOffset != Trig2->FailOffset)
          IsEqual = FALSE;

        break;

      case mtaUnsets:

        /* Trig1->Unsets != Trig2->Unsets */
        /* Trig1->FUnsets != Trig2->FUnsets */

      default:
        /* no-op */

        break;
      }

    if (IsEqual == FALSE)
      {
      MDB(4,fSTRUCT) MLog("INFO:     triggers differ in %s\n",
        MTrigAttr[tindex]);

      break;
      }
    }

  return(IsEqual);
  } /* END mbool_t MTrigIsEqual() */



/**
 * Determines if a trigger is real (non-template).
 *
 * @param T
 */

mbool_t MTrigIsReal(

  mtrig_t *T)

  {
  if (T == NULL)
    return(FALSE);

  if (T->TName == NULL)
    return(FALSE);

  if (T->IsExtant == FALSE)
    return(FALSE);

  if (bmisset(&T->InternalFlags,mtifIsTemplate))
    return(FALSE);

  return(TRUE);
  }  /* END MTrigIsReal() */



/**
 * Determines if a trigger is valid.
 *
 * @param T
 */

mbool_t MTrigIsValid(

  mtrig_t *T)

  {
  if (T == NULL)
    return(FALSE);

  if (T->TName == NULL)
    return(FALSE);

  if (T->IsExtant == FALSE)
    return(FALSE);

  return(TRUE);
  }  /* END MTrigIsValid() */




/*
 * Returns SUCCESS if this trigger belongs to a generic system job, FAILURE otherwise.
 *
 * @param T - the trigger to check
 */

int MTrigIsGenericSysJob(

  mtrig_t *T)

  {
  mjob_t *J = NULL;

  if (T == NULL)
    return(FAILURE);

  if (T->OType != mxoJob)
    return(FAILURE);

  if (T->O == NULL)
    return(FAILURE);

  J = (mjob_t *)T->O;

  if (((J == NULL) ||
        (!bmisset(&J->IFlags,mjifGenericSysJob))) &&
      (!bmisset(&T->SFlags,mtfGenericSysJob)))
    return(FAILURE);

  return(SUCCESS);
  } /* END MTrigIsGenericSysJob() */



/**
 * Returns TRUE if the varname has a namespace on it.
 *
 * @param VariableName    [I] - Name to check
 * @param NameWONameSpace [O] (optional) - Output to the varname w/o the namespace on it
 */

mbool_t MTrigVarIsNamespace(

  const char  *VariableName,
  const char **NameWONameSpace)

  {
  char VarName[MMAX_LINE];
  char *Ptr;
  char *EPtr = NULL;

  if (NameWONameSpace != NULL)
    *NameWONameSpace = NULL;

  if (VariableName == NULL)
    return(FALSE);

  MUStrCpy(VarName,VariableName,sizeof(VarName));

  Ptr = VarName;
  /* lowest name is vc1 */

  if (strlen(VarName) < 3)
    return(FALSE);

  if ((Ptr[0] != 'v') ||
      (Ptr[1] != 'c'))
    return(FALSE);

  Ptr += 2;

  if ((strtol(Ptr,&EPtr,10) == 0) ||
      (EPtr == NULL) ||
      (*EPtr != '.'))
    return(FALSE);

  /* it is a namespace */

  if (NameWONameSpace != NULL)
    {
    const char *tmpPtr = VariableName;

    while ((*tmpPtr != '.') &&
           (*tmpPtr != '\0'))
      {
      tmpPtr++;
      }

    if (*tmpPtr == '.')
      tmpPtr++;

    *NameWONameSpace = tmpPtr;
    }

  return(TRUE);
  }  /* END MTrigVarIsNamespace() */



/**
 * Returns TRUE if the Varname has a valid namespace.  Also returns TRUE if the
 *  var isn't using a namespace.
 *
 * If ValidNamespaces == NULL, all namespaces are accepted
 *
 * @param VarName         [I] - Name to check
 * @param ValidNamespaces [I] - Valid namespaces
 */

mbool_t MTrigValidNamespace(
  const char *VarName,
  const char *ValidNamespaces)

  {
  char tmpName[MMAX_LINE];
  char tmpVNames[MMAX_LINE << 2];
  char *Ptr;
  char *TokPtr;
  char *VPtr;    /* For tokenizing valid names */
  char *VTokPtr;

  if (VarName == NULL)
    return(FALSE);

  if (ValidNamespaces == NULL)
    return(TRUE);

  if (MTrigVarIsNamespace(VarName,NULL) == FALSE)
    return(TRUE);

  MUStrCpy(tmpName,VarName,sizeof(tmpName));
  Ptr = MUStrTok(tmpName,".",&TokPtr);

  if (Ptr == NULL)
    {
    /* this should not happen because we checked in MTrigVarIsNamespace() */

    return(FALSE);
    }

  MUStrCpy(tmpVNames,ValidNamespaces,sizeof(tmpVNames));
  VPtr = MUStrTok(tmpVNames,",",&VTokPtr);

  while (VPtr != NULL)
    {
    if (!strcmp(VPtr,Ptr))
      return(TRUE);

    VPtr = MUStrTok(NULL,",",&VTokPtr);
    }

  return(FALSE);
  }  /* END MTrigValidNamespace() */


/* END MTrigIs.c */
