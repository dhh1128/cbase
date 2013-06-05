/* HEADER */

      
/**
 * @file MTrigString.c
 *
 * Contains: Various Trigger To/From String functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  

/**
 * This function will parse a string to and extract trigger information from it, and add
 * the resulting trigger to TList. Depending on the value of AddList, the extracted trigger will
 * either be appended to TList or replace TList.
 * 
 * @see MTrigFromString() - child
 * @see MJobProcessExtensionString() - parent
 * @see MTrigSetAttr() - child
 *
 * NOTE:  for new-style format, trigger attributes are ',' or '&' delimited.
 * NOTE:  new style format may require ':' delimited subcomponents
 * 
 * @param TList (I) [modified] The pointer to the object trigger list. The newly 
 *   created trigger will be appended to this list.
 * @param String (I) The string to extract the trigger from
 * @param AddList (I) A boolean specifying whether to add to the list or to override existing triggers
 * @param DoEval (I) 
 * @param OType (I) The object type that the trigger will operate on
 * @param OID (I) A character pointer specifying the ID of the object to operate on. 
 *   For jobs, this should be J->Name, for nodes, it should be N->Name?
 * @param NTList (O) [optional] Holds the newly created triggers
 * @param EMsg (O) [optional, minsize=MMAX_LINE] Any error message generated will be put here.
 * 
 */

int MTrigLoadString(

  marray_t **TList,   /* I object trigger list pointer (modified) */
  const char *String,  /* I */
  mbool_t    AddList, /* I (add to list or override existing triggers) */
  mbool_t    DoEval,  /* I */
  enum MXMLOTypeEnum OType, /* I */
  char      *OID,     /* I */
  marray_t  *NTList,  /* O newly created triggers (optional) */
  char      *EMsg)    /* O (optional,minsize=MMAX_LINE) */

  {
  mtrig_t *T;
  mtrig_t *tmpTP;

  char *ptr;
  char *TokPtr;

  enum MTrigAttrEnum       aindex;

  int  tmpI;

  char ValLine[MMAX_BUFFER];

  char OString[MMAX_BUFFER];

  char *BPtr;
  int   BSpace;

  const char *FName = "MTrigLoadString";


  MDB(6,fSTRUCT) MLog("%s(TList,%.50s,%s,%s,%d,OID,NTList,EMsg)\n",
    FName,
    (String != NULL) ? String : "NULL",
    MBool[AddList],
    MBool[DoEval],
    OType);

#if defined(__MDEMO)
  /* triggers not enabled in basic */

  if (EMsg != NULL)
    snprintf(EMsg,MMAX_LINE,"triggers are disabled");

  return(FAILURE);
#endif /* __MDEMO */

  if (EMsg != NULL)
    {
    MUSNInit(&BPtr,&BSpace,EMsg,MMAX_LINE);
    }

  MUStrCpy(OString,String,sizeof(OString));

  /* try new format <attr>=<val>[{,&}<attr>=<val>..] */

  if (NTList != NULL)
    MUArrayListClear(NTList);

  tmpTP = (mtrig_t *)MUCalloc(1,sizeof(mtrig_t));

  bmset(&tmpTP->InternalFlags,mtifIsAlloc);

  ptr = MUStrTokE(OString,",&",&TokPtr);

  while (ptr != NULL)
    {
    if (MUGetPairCI(
          ptr,
          (const char **)MTrigAttr,
          &tmpI,
          NULL,
          TRUE,
          NULL,
          ValLine,
          sizeof(ValLine)) == FAILURE)
      {
      if (EMsg != NULL)
        {
        if (EMsg[0] != '\0')
          MUSNCat(&BPtr,&BSpace,",");

        MUSNPrintF(&BPtr,&BSpace,"unknown attribute '%s'",
          ptr);
        }

      ptr = MUStrTokE(NULL," \t\n",&TokPtr);

      continue;
      }  /* END if (MUGetPair() == FAILURE) */

    aindex = (enum MTrigAttrEnum)tmpI;

    /* NOTE:  MTrigSetAttr() may dynamically allocate memory onto the template trigger */

    switch (aindex)
      {
      case mtaActionType:
      case mtaActionData:
      case mtaBlockTime:
      case mtaDependsOn:
      case mtaDisabled:
      case mtaDescription:
      case mtaECPolicy:
      case mtaEnv:
      case mtaEventTime:
      case mtaExpireTime:
      case mtaFlags:
      case mtaIsComplete:
      case mtaIsInterval:
      case mtaMaxRetry:
      case mtaRetryCount:
      case mtaMessage:
      case mtaMultiFire:
      case mtaName:
      case mtaOffset:
      case mtaPeriod:
      case mtaRearmTime:
      case mtaRequires:
      case mtaSets:
      case mtaStdIn:
      case mtaThreshold:
      case mtaTimeout:
      case mtaFailOffset:
      case mtaEventType:
      case mtaUnsets:

        if (MTrigSetAttr(tmpTP,aindex,ValLine,mdfString,mAdd) == FAILURE)
          {
          /* invalid */

          if (EMsg != NULL)
            {
            if (EMsg[0] != '\0')
              MUSNCat(&BPtr,&BSpace,",");

            MUSNPrintF(&BPtr,&BSpace,"cannot set attribute %s to '%s'",
              ptr,
              ValLine);
            }

          if (DoEval == FALSE)
            {
            MTListDestroy(TList);

            MTrigFree(&tmpTP);

            return(FAILURE);
            }
          }

        break;

      default:

        /* NO-OP */

        break;
      }  /* END switch (aindex) */

    ptr = MUStrTokE(NULL,",&",&TokPtr);
    }     /* END while (ptr != NULL) */

  /* indicate that trigger event type is correctly specified, but not locked in */

  if (MTrigValidate(tmpTP) == FAILURE)
    {
    if (EMsg != NULL)
      {
      if (EMsg[0] != '\0')
        MUSNCat(&BPtr,&BSpace,",");

      MUSNPrintF(&BPtr,&BSpace,"trigger validation failed");
      }

    MTrigFree(&tmpTP);

    return(FAILURE);
    }

  if (DoEval == TRUE)
    {
    MTrigFree(&tmpTP);

    if ((EMsg != NULL) && (EMsg[0] != '\0'))
      {
      /* EMsg set */

      return(FAILURE);
      }

    return(SUCCESS);
    }

#ifdef MNOT
  /* Someday this might be useful to turn on, since it's possible to add a 
   * trigger multiple times. */

  /* Check if trigger with same attributes already exists. If it does then
   * return the found trigger. */

  if ((T = MTListContains(&tmpT)) != NULL)
    {
    /* attach trigger to object trigger list */
    /* allocate object trigger list if required */

    if (MOAddTrigPtr(TList,T) == FAILURE)
      {
      /* could not add trigger to object, list is full */

      MTrigRemove(NULL,T->TName,TRUE);

      return(FAILURE);
      }

    return(SUCCESS);
    }
#endif

  if (MTrigAdd(NULL,tmpTP,&T) == FAILURE)
    {
    if (EMsg != NULL)
      {
      sprintf(EMsg,"internal error, could not add trigger to global table");
      }

    MTrigFree(&tmpTP);

    return(FAILURE);
    }

  T->OType = OType;

  bmset(&T->InternalFlags,mtifIsTemplate);

  MUStrDup(&T->OID,OID);

  /* attach trigger to object trigger list */
  /* allocate object trigger list if required */

  if (MOAddTrigPtr(TList,T) == FAILURE)
    {
    /* could not add trigger to object, list is full */

    if (EMsg != NULL)
        sprintf(EMsg,"cannot add trigger to object, object is full - please contact technical support for assistance");

    MTrigRemove(NULL,T->TName,TRUE);

    return(FAILURE);
    }

  T->IsExtant = TRUE;

  if (NTList != NULL)
    {
    /* record newly created triggers */

    MUArrayListAppendPtr(NTList,T);
    }  /* END if (NTList != NULL) */

  MTrigTransition(T);

  MTrigFree(&tmpTP);

  return(SUCCESS);
  }    /* END MTrigLoadString() */






#ifdef MOLDTRIGGER
/**
 * Loads a trigger from the "old-style" string format. (NO LONGER SUPPORTED)
 *
 * @param TList (I) [modified]
 * @param String (I)
 * @param AddList (I) [add to list or override existing triggers]
 * @param OType (I)
 * @param OID (I)
 * @param NTList (O) [optional - list of newly created triggers]
 */

int MTrigFromString(

  mtrig_t ***TList,   /* I object trigger list pointer (modified) */
  char      *String,  /* I */
  mbool_t    AddList, /* I (add to list or override existing triggers) */
  enum MXMLOTypeEnum OType, /* I */
  char      *OID,     /* I */
  mtrig_t  **NTList)  /* O (optional - list of newly created triggers) */

  {
  char *head;
  char *ptr;
  char *TokPtr;

  char *tptr;
  char *TTokPtr;

  long  Offset;
  double Threshold;

  int   ntindex = 0;

  char *Action;

  mbool_t IsComplete  = FALSE;
  mbool_t IsMultiFire = FALSE;

  enum MTrigTypeEnum       EIndex;
  enum MTrigActionTypeEnum AIndex;
 
  mtrig_t *T;

  mtrig_t  tmpT;

  int tindex;

  if ((String == NULL) || (TList == NULL))
    {
    /* invalid parameters */

    return(FAILURE);
    }
 
  if (AddList == FALSE)
    {
    /* free existing list */

    MTListDestroy(TList);
    }

  /* FORMAT: [<NAME>$]<ETYPE>[+<OFFSET>][@<THRESHOLD>];<ATYPE>[@<ADATA>[+<ADATA>]][:|<ATTR>[|<ATTR>]...][,<TRIGSTRING>]... */
  /* NOTE:  <THRESHOLD> may be float, ie contains '.' */

  if ((tptr = MUStrTok(String,",",&TTokPtr)) == NULL)
    {
    /* invalid format */

    return(FAILURE);
    }

  /* if TList exists, locate available slot in list */

  if ((AddList == TRUE) && (*TList != NULL))
    {
    for (tindex = 0;tindex < MMAX_TRIG;tindex++)
      {
      if ((*TList)[tindex] == NULL)
        break;

      if ((*TList)[tindex]->IsExtant == FALSE)
        break;
      }  /* END for (tindex) */

    if (tindex >= MMAX_TRIG)
      {
      /* buffer full */

      return(FAILURE);
      }
    }

  while (tptr != NULL)
    {
    if ((head = MUStrTok(tptr,";",&TokPtr)) == NULL)
      {
      /* invalid format */

      return(FAILURE);
      }

    /* FORMAT: head -> [<NAME>$]<ETYPE>[+<OFFSET>][@<THRESHOLD>] */

    if ((ptr = strchr(head,'$')) != NULL)
      { 
      /* ignore name */

      ptr++;
      }
    else
      {
      ptr = head;
      }

    if ((EIndex = (enum MTrigTypeEnum)MUGetIndex(ptr,MTrigType,TRUE,mttNONE)) == mttNONE)
      {
      /* invalid event type specified */

      return(FAILURE);
      }

    ptr += strlen(MTrigType[EIndex]);

    /* offset/threshold optional */

    Offset = 0;
    Threshold = 0.0;

    if (*ptr == '+')
      {
      Offset = MUTimeFromString(ptr + 1);
      }
    else if (*ptr == '-')
      {
      Offset = MUTimeFromString(ptr + 1) * (-1);
      }

    if ((ptr = strchr(head,'@')) != NULL)
      {
      Threshold = strtod(ptr + 1,NULL);
      }

    /* load action */

    if ((ptr = MUStrTok(NULL,",",&TokPtr)) == NULL)
      {
      /* invalid format */

      return(FAILURE);
      }

    /* FORMAT:  ptr -> <ATYPE>[@<ADATA>[+<ADATA>]] */

    if ((AIndex = (enum MTrigActionTypeEnum)MUGetIndex(
           ptr,
           MTrigActionType,
           TRUE,
           mtatNONE)) == mtatNONE)
      {
      /* invalid action type specified */

      return(FAILURE);
      }

    /* action data is optional */

    Action = NULL;

    if ((ptr = strchr(ptr,'@')) != NULL)
      {
      /* no action specified */

      if (strncmp(ptr + 1,NONE,strlen(NONE)))
        Action = ptr + 1;
      }

    /* create trigger and add to global list */

    T = &tmpT;

    memset(T,0,sizeof(tmpT));

    if (MTrigCreate(
          &tmpT,
          EIndex,
          Offset,
          Threshold,
          AIndex,
          Action,
          IsComplete) == FAILURE)
      {
      /* cannot create trigger */

      MTListDestroy(TList);
 
      return(FAILURE);
      }

    if (MTrigAdd(tmpT.TName,&tmpT,&T,FALSE) == FAILURE)
      {
      return(SUCCESS);
      }

    /* Attrs are optional */

    ptr = MUStrTok(NULL,"|",&TokPtr);

    while (ptr != NULL)
      {
      /* FORMAT:  ptr -> <ATTR>[|<ATTR>]... */

      if ((strstr(ptr,"Complete")) != NULL)
        {
        bmset(&T->InternalFlags,mtifIsComplete);
        }

      if ((strstr(ptr,"MultiFire")) != NULL)
        {
        bmset(&T->InternalFlags,mtifMultiFire);
        }

      if ((strstr(ptr,"Requires")) != NULL)
        {
        char *ptr2;
        char *TokPtr2;

        ptr2 = MUStrTok(ptr,".",&TokPtr2);

        ptr2 = MUStrTok(NULL,".",&TokPtr2);

        while (ptr2 != NULL)
          {
          /* add to dependency list */

          if (ptr2[0] == '!')
            {
            MDAGSetAnd(&T->Depend,mdVarNotSet,ptr2++,NULL,0);
            }
          else
            {
            MDAGSetAnd(&T->Depend,mdVarSet,ptr2,NULL,0);
            }

          ptr2 = MUStrTok(NULL,".",&TokPtr2);
          }
        }    /* END if ((strstr(ptr,"Requires")) != NULL) */

      if ((strstr(ptr,"Sets")) != NULL)
        {
        char *ptr2;
        char *TokPtr2;

        ptr2 = MUStrTok(ptr,".",&TokPtr2);

        ptr2 = MUStrTok(NULL,".",&TokPtr2);

        while (ptr2 != NULL)
          {
          char *tmpSets;
          /* add set to list */

          if (ptr2[0] == '!')
            {
            MUStrDup(&tmpSets,ptr2+1);

            MTrigInitSets(&T->FSets,FALSE);

            MUArrayListAppendPtr(T->FSets,&tmpSets); 
            }
          else
            {
            MUStrDup(&tmpSets,ptr2);

            MTrigInitSets(&T->SSets,FALSE);

            MUArrayListAppendPtr(T->SSets,&tmpSets);
            }

          ptr2 = MUStrTok(NULL,".",&TokPtr2);
          }
        }

      ptr = MUStrTok(NULL,"|",&TokPtr);
      }  /* END while (ptr != NULL) */

    /* link trigger to parent object */

    T->OType      = OType;

    bmset(&T->InternalFlags,mtifIsTemplate);

    bmsetbool(&T->InternalFlags,mtifMultiFire,IsMultiFire);

    MUStrDup(&T->OID,OID);

    /* attach trigger to object trigger list */
    /* allocate object trigger list if required */

    if (MOAddTrigPtr(TList,T) == FAILURE)
      {
      /* could not add trigger to object -> list full */

      MTrigRemove(NULL,T->TName,TRUE);

      if (ntindex == 0)
        {
        return(FAILURE);
        }

      break;
      }

    tptr = MUStrTok(NULL,",",&TTokPtr);

    if (NTList != NULL)
      {
      /* record newly created triggers */

      NTList[ntindex]     = T;
      NTList[ntindex + 1] = NULL;
      }

    ntindex++;
    }  /* END while (tptr != NULL) */

  return(SUCCESS);
  }  /* END MTrigFromString() */

#endif /* MOLDTRIG */




/**
 * Convert trigger attribute to string.
 *
 * @param T
 * @param AIndex (I)
 * @param Buf (O)
 * @param SBufSize (I) [optional]
 * @param Mode (I)
 */

int MTrigAToString(

  mtrig_t *T,
  enum MTrigAttrEnum AIndex, /* I */
  char   *Buf,      /* O */
  int     SBufSize, /* I (optional) */
  int     Mode)     /* I */

  {
  int BufSize;

  if ((T == NULL) || (Buf == NULL))
    {
    return(FAILURE);
    }

  BufSize = (SBufSize > 0) ? SBufSize : MMAX_LINE;

  Buf[0] = '\0';

  switch (AIndex)
    {
    case mtaActionData:
   
      if (T->Action != NULL)
        MUStrCpy(Buf,T->Action,BufSize);

      break;

    case mtaActionType:

      if (T->AType != mtatNONE)
        MUStrCpy(Buf,(char *)MTrigActionType[T->AType],BufSize);

      break;

    case mtaBlockTime:

      if (T->BlockTime != 0)
        {
        sprintf(Buf,"%lu",
          T->BlockTime);
        }

      break;

    case mtaDescription:
   
      if (T->Action != NULL)
        MUStrCpy(Buf,T->Description,BufSize);

      break;

    case mtaEnv:

      if (T->Env != NULL)
        {
        /* Convert to printable format (using ';' instead of ENVRS_ENCODED_CHAR) */

        mstring_t tmpS(MMAX_LINE);

        MStringSet(&tmpS,T->Env);

        MTrigTranslateEnv(&tmpS,TRUE);

        MUStrCpy(Buf,tmpS.c_str(),BufSize);
        }

      break;

    case mtaEBuf:

      if (T->EBuf != NULL)
        MUStrCpy(Buf,T->EBuf,BufSize);

      break;

    case mtaEventTime:
      
      if (T->ETime > 0)
        {
        sprintf(Buf,"%ld",
          T->ETime);
        }

      break;

    case mtaEventType:

      if (T->EType != mttNONE)
        MUStrCpy(Buf,(char *)MTrigType[T->EType],BufSize);

      break;

    case mtaExpireTime:

      if (T->ExpireTime > 0)
        {
        sprintf(Buf,"%ld",
          T->ExpireTime);
        }

      break;

    case mtaFailureDetected:

      if (bmisset(&T->InternalFlags,mtifFailureDetected))
        {
        strcpy(Buf,"TRUE");
        }

      break;

    case mtaFlags:

      {
      char tmpLine[MMAX_LINE];

      MTrigFlagsToString(T,tmpLine,sizeof(tmpLine));

      sprintf(Buf,"%s",tmpLine); 
      }

      break;

    case mtaIBuf:

      {
      if (T->IBuf != NULL)
        MUStrCpy(Buf,T->IBuf,BufSize);
      }

      break;

    case mtaIsComplete:

      if (T->ETime > 0)
        {
        strcpy(Buf,(bmisset(&T->InternalFlags,mtifIsComplete)) ? "TRUE" : "FALSE");
        }

      break;

    case mtaLaunchTime:

      if (T->LaunchTime != 0)
        {
        sprintf(Buf,"%ld",
          T->LaunchTime);
        }

      break;

    case mtaMaxRetry:

      if (T->MaxRetryCount != 0)
        {
        sprintf(Buf,"%d",
          T->MaxRetryCount);
        }

      break;

    case mtaRetryCount:

      if (T->RetryCount != 0)
        {
        sprintf(Buf,"%d",T->RetryCount);
        }

      break;

    case mtaMessage:

      if (T->Msg != NULL)
        MUStrCpy(Buf,T->Msg,BufSize);

      break;

    case mtaMultiFire:

      if (bmisset(&T->InternalFlags,mtifMultiFire))
        {
        strcpy(Buf,"TRUE");
        }

      break;

    case mtaName:

      if (T->Name != NULL)
        MUStrCpy(Buf,T->Name,BufSize);

      break;

    case mtaObjectID:

      if (T->OID != NULL)
        MUStrCpy(Buf,T->OID,BufSize);

      break;

    case mtaObjectType:

      if (T->OType > mxoNONE)
        MUStrCpy(Buf,(char *)MXO[T->OType],BufSize);

      break;

    case mtaOBuf:

      if (T->OBuf != NULL)
        MUStrCpy(Buf,T->OBuf,BufSize);

      break;

    case mtaOffset:

      if (T->Offset != 0)
        {
        sprintf(Buf,"%ld",
          T->Offset);
        }

      break;

    case mtaPID:

      if (T->PID != 0)
        {
        sprintf(Buf,"%d",
          T->PID);
        }

      break;

    case mtaRearmTime:

      if (T->RearmTime != 0)
        {
        sprintf(Buf,"%lu",
          T->RearmTime);
        }

      break;

    case mtaRequires:

      if (T->Depend.NumItems > 0)
        MDAGToString(&T->Depend,Buf,BufSize);

      break;

    case mtaSets:

      {
      char *BPtr;

      char  *tmpSet;

      int BSpace;
      int index = 0;

      int *SetsFlags = NULL;

      mbool_t Bool = FALSE;
      mbool_t IsExport = FALSE;

      MUSNInit(&BPtr,&BSpace,Buf,BufSize);

      if (T->SSetsFlags != NULL)
        SetsFlags = (int *)T->SSetsFlags->Array;

      if (T->SSets != NULL)
        {
        while((tmpSet = (char *)MUArrayListGetPtr(T->SSets,index)) != NULL)
          {
          if ((SetsFlags != NULL) && (MOLDISSET(SetsFlags[index],mtvaExport)))
            IsExport = TRUE;
          else
            IsExport = FALSE;
  
          if (index > 0)
            {
            MUSNPrintF(&BPtr,&BSpace,".%s%s",
              (IsExport == TRUE) ? "^" : "",
              tmpSet);
            }
          else
            {
            MUSNPrintF(&BPtr,&BSpace,"%s%s",
              (IsExport == TRUE) ? "^" : "",
              tmpSet);
  
            Bool = TRUE;
            }
  
          index++;
          }
        } /* END if (T->SSets != NULL) */

      index = 0;

      if (T->FSetsFlags != NULL)
        SetsFlags = (int *)T->FSetsFlags->Array;

      if (T->FSets != NULL)
        {
        while((tmpSet = (char *)MUArrayListGetPtr(T->FSets,index)) != NULL)
          {
          if ((SetsFlags != NULL) && (MOLDISSET(SetsFlags[index],mtvaExport)))
            IsExport = TRUE;
          else
            IsExport = FALSE;
  
          if ((index > 0) || (Bool == TRUE))
            {
            MUSNPrintF(&BPtr,&BSpace,".!%s%s",
              (IsExport == TRUE) ? "^" : "",
              tmpSet);
            }
          else
            {
            MUSNPrintF(&BPtr,&BSpace,"!%s%s",
              (IsExport == TRUE) ? "^" : "",
              tmpSet);
            }
  
          index++;
          }
        } /* END if (T->FSets != NULL) */
      }   /* END case mtaSets */

      break;

    case mtaState:

      if (T->State != mtsNONE)
        MUStrCpy(Buf,(char *)MTrigState[T->State],BufSize);

      break;

    case mtaStdIn:

      if (T->StdIn != NULL)
        MUStrCpy(Buf,T->StdIn,BufSize);

      break;

    case mtaThreshold:

      /* NYI: support for QueueTime and XFactor */

      if (T->ThresholdType == mtttNONE)
        break;

      if (Mode == mfmHuman)
        {
        if (T->ThresholdType != mtttGMetric)
          {
          snprintf(Buf,BufSize,"%s%s%s%s %s %.02f%s",
            MTrigThresholdType[T->ThresholdType],
            (T->ThresholdGMetricType == NULL) ? "" : "[",
            (T->ThresholdGMetricType == NULL) ? "" : T->ThresholdGMetricType,
            (T->ThresholdGMetricType == NULL) ? "" : "]",
            (T->ThresholdCmp == mcmpNONE) ? ">" : MComp[T->ThresholdCmp], 
            (bmisset(&T->InternalFlags,mtifThresholdIsPercent)) ? T->Threshold * 100 : T->Threshold,
            (bmisset(&T->InternalFlags,mtifThresholdIsPercent)) ? "%" : "");
          }
        else if (T->ThresholdType == mtttGMetric)
          {
          snprintf(Buf,BufSize,"%s[%s] %s %.02f",
            MTrigThresholdType[T->ThresholdType],
            (T->ThresholdGMetricType == NULL) ? "NONE" : T->ThresholdGMetricType,
            (T->ThresholdCmp == mcmpNONE) ? ">" : MComp[T->ThresholdCmp],
            T->Threshold);
          }
        }
      else
        {
        if (T->ThresholdType != mtttGMetric)
          {
          snprintf(Buf,BufSize,"%s%s%s%s%s%.02f%s",
            MTrigThresholdType[T->ThresholdType],
            (T->ThresholdGMetricType == NULL) ? "" : "[",
            (T->ThresholdGMetricType == NULL) ? "" : T->ThresholdGMetricType,
            (T->ThresholdGMetricType == NULL) ? "" : "]",
            (T->ThresholdCmp == mcmpNONE) ? ">" : MComp[T->ThresholdCmp], 
            (bmisset(&T->InternalFlags,mtifThresholdIsPercent)) ? T->Threshold * 100 : T->Threshold,
            (bmisset(&T->InternalFlags,mtifThresholdIsPercent)) ? "%" : "");
          }
        else if (T->ThresholdType == mtttGMetric)
          {
          snprintf(Buf,BufSize,"%s[%s]%s%.02f",
            MTrigThresholdType[T->ThresholdType],
            (T->ThresholdGMetricType == NULL) ? "NONE" : T->ThresholdGMetricType,
            (T->ThresholdCmp == mcmpNONE) ? ">" : MComp[T->ThresholdCmp],
            T->Threshold);
          }
        }

      break;

    case mtaTimeout:

      if (T->Timeout != 0)
        {
        sprintf(Buf,"%lu",
          T->Timeout);
        }

      break;

    case mtaTrigID:

      if (T->TName != NULL)
        {
        MUStrCpy(Buf,T->TName,BufSize);
        }

      break;

    case mtaUnsets:

      {
      char *BPtr;

      char *tmpSet;

      int BSpace;
      int index;

      mbool_t Bool = FALSE;

      MUSNInit(&BPtr,&BSpace,Buf,BufSize);

      index = 0;
      if (T->Unsets != NULL)
        {
        while((tmpSet = (char *)MUArrayListGetPtr(T->Unsets,index)) != NULL)
          {
          if (index > 0)
            {
            MUSNPrintF(&BPtr,&BSpace,".%s",tmpSet);
            }
          else
            {
            MUSNPrintF(&BPtr,&BSpace,"%s",tmpSet);
  
            Bool = TRUE;
            }
  
          index++;
          }
        } /* END if (T->Unsets != NULL) */

      index = 0;
      if (T->FUnsets != NULL)
        {
        while((tmpSet = (char *)MUArrayListGetPtr(T->FUnsets,index)) != NULL)
          {
          if ((index > 0) || (Bool == TRUE))
            {
            MUSNPrintF(&BPtr,&BSpace,".!%s",tmpSet);
            }
          else
            {
            MUSNPrintF(&BPtr,&BSpace,"!%s",tmpSet);
            }
  
          index++;
          }
        } /* END if (T->FUnsets != NULL) */
      }   /* END case mtaUnsets */

      break;

    default:

      /* specified attribute not supported */

      return(FAILURE);

      /*NOTREACHED*/

      break;
    }  /* END switch (AIndex) */

  return(SUCCESS);
  }  /* END MTrigAToString() */




/**
 * Report trigger attribute as string.
 *
 *
 * @param T      (I)
 * @param AIndex (I)
 * @param Buf    (O)
 * @param Mode   (I) [bitmap of enum MCModeEnum]
 */

int MTrigTransitionAToString(

  mtranstrig_t        *T,
  enum MTrigAttrEnum  AIndex,
  mstring_t           *Buf,
  int                 Mode)

  {

  if (T == NULL)
    {
    return(FAILURE);
    }

  switch (AIndex)
    {
    case mtaName:

      if (!MUStrIsEmpty(T->Name))
        MStringSet(Buf,T->Name);

      break;

    case mtaActionData:

      if (!MUStrIsEmpty(T->ActionData))
        MStringSet(Buf,T->ActionData);

      break;

    case mtaActionType:

      if (!MUStrIsEmpty(T->ActionType))
        MStringSet(Buf,T->ActionType);

      break;

    case mtaBlockTime:

      MStringSetF(Buf,"%d",T->BlockTime);

      break;

    case mtaDescription:

      if (!MUStrIsEmpty(T->Description))
        MStringSet(Buf,T->Description);

      break;

    case mtaDisabled:

      (T->Disabled) ? MStringSet(Buf,"TRUE") : MStringSet(Buf,"FALSE");

      break;

    case mtaEBuf:

      if (!MUStrIsEmpty(T->EBuf))
        MStringSet(Buf,T->EBuf);

      break;

    case mtaEventTime:

      MStringSetF(Buf,"%d",T->EventTime);

      break;

    case mtaEventType:

      if (!MUStrIsEmpty(T->EventType))
        MStringSet(Buf,T->EventType);

      break;

    case mtaFailOffset:

      if (!MUStrIsEmpty(T->FailOffset))
        MStringSet(Buf,T->FailOffset);

      break;

    case mtaFailureDetected:

      (T->FailureDetected) ? MStringSet(Buf,"TRUE") : MStringSet(Buf,"FALSE");

      break;

    case mtaFlags:

      if (!MUStrIsEmpty(T->Flags))
        MStringSet(Buf,T->Flags);

      break;

    case mtaIBuf:

      if (!MUStrIsEmpty(T->IBuf))
        MStringSet(Buf,T->IBuf);

      break;

    case mtaIsComplete:

      (T->IsComplete) ? MStringSet(Buf,"TRUE") : MStringSet(Buf,"FALSE");

      break;

    case mtaIsInterval:

      (T->IsInterval) ? MStringSet(Buf,"TRUE") : MStringSet(Buf,"FALSE");

      break;

    case mtaLaunchTime:

        MStringSetF(Buf,"%d",T->LaunchTime);

      break;

    case mtaMessage:

      if (!MUStrIsEmpty(T->Message))
        MStringSet(Buf,T->Message);

      break;

    case mtaMultiFire:

      if (!MUStrIsEmpty(T->MultiFire))
        MStringSet(Buf,T->MultiFire);

      break;

    case mtaObjectID:

      if (!MUStrIsEmpty(T->ObjectID))
        MStringSet(Buf,T->ObjectID);

      break;

    case mtaObjectType:

      if (!MUStrIsEmpty(T->ObjectType))
        MStringSet(Buf,T->ObjectType);

      break;

    case mtaOBuf:

      if (!MUStrIsEmpty(T->OBuf))
        MStringSet(Buf,T->OBuf);

      break;

    case mtaOffset:

      if (!MUStrIsEmpty(T->Offset))
        MStringSet(Buf,T->Offset);

      break;

    case mtaPeriod:

      if (!MUStrIsEmpty(T->Period))
        MStringSet(Buf,T->Period);

      break;

    case mtaPID:

        MStringSetF(Buf,"%d",T->PID);

      break;

   case mtaRearmTime:

        MStringSetF(Buf,"%d",T->RearmTime);

      break;

    case mtaRequires:

      if (!MUStrIsEmpty(T->Requires))
        MStringSet(Buf,T->Requires);

      break;

    case mtaSets:

      if (!MUStrIsEmpty(T->Sets))
        MStringSet(Buf,T->Sets);

      break;

    case mtaState :

      if (!MUStrIsEmpty(T->State))
        MStringSet(Buf,T->State);

      break;

    case mtaThreshold:

      if (!MUStrIsEmpty(T->Threshold))
        MStringSet(Buf,T->Threshold);

      break;

    case mtaTimeout:

        MStringSetF(Buf,"%d",T->Timeout);

      break;

    case mtaTrigID:

      if (!MUStrIsEmpty(T->TrigID))
        MStringSet(Buf,T->TrigID);

      break;
  
    case mtaUnsets:

      if (!MUStrIsEmpty(T->Unsets))
        MStringSetF(Buf,T->Unsets);

      break;

    default:

      return(FAILURE);

      /*NOTREACHED*/

      break;
    }  /* END switch (AIndex) */

  return(SUCCESS);
  }  /* END MRsvAToMString() */




/**
 * Produces a string which represents this trigger in the "trace" format (used
 * in Moab's event files).
 *
 * @param T      (I)
 * @param EType  (I) [not used]
 * @param Buffer (I)
 */

int MTrigToTString(

  mtrig_t                   *T,
  enum MRecordEventTypeEnum  EType,
  mstring_t                 *Buffer)

  {
  int aindex;

  enum MFormatModeEnum Mode;

  char tmpString[MMAX_LINE << 1];

  const enum MTrigAttrEnum AList[] = {
    mtaActionData,
    mtaActionType,
    mtaBlockTime,
    mtaDescription,
    mtaEBuf,
    mtaEventTime,
    mtaEventType,
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

  if ((T == NULL) || (Buffer == NULL))
    {
    return(FAILURE);
    }

  Buffer->clear();

  for (aindex = 0;AList[aindex] != mtaNONE;aindex++)
    {
    Mode = mfmNONE;

    if (AList[aindex] == mtaFlags)
      {
      Mode = mfmHuman;
      }

    if ((MTrigAToString(
          T,
          AList[aindex],
          tmpString,
          sizeof(tmpString),
          Mode) == FAILURE) || (tmpString[0] == '\0'))
      {
      continue;
      }

    MStringAppendF(Buffer,"%s=\"%s\" ",MTrigAttr[AList[aindex]],tmpString);
    }

  return(SUCCESS);
  }  /* END MTrigToTString() */





/**
 *
 *
 * @param T
 * @param Buf
 * @param BufSize
 */

int MTrigFlagsToString(

  mtrig_t *T,
  char    *Buf,
  int      BufSize)

  {
  char *BPtr = NULL;
  int   BSpace = 0;

  int   findex;

#if 0 
  enum MTrigFlagEnum FList[] = {
    mtfAttachError,
    mtfCleanup,
    mtfInterval,
    mtfMultiFire,
    mtfProbe,
    mtfProbeAll,
    mtfLAST };
#endif

  MUSNInit(&BPtr,&BSpace,Buf,BufSize);

  for (findex = 1;findex != mtfLAST;findex++)
    {
    if ((findex == mtfUser) && (bmisset(&T->SFlags,mtfUser)))
      {
      mgcred_t *U;

      if (MTrigGetOwner(T,&U) != FAILURE)
        {
        MUSNPrintF(&BPtr,&BSpace,"[%s+%s]",
          MTrigFlag[mtfUser],
          U->Name);
        }

      continue;
      }

    if ((findex == mtfGlobalVars) && (bmisset(&T->SFlags,mtfGlobalVars)))
      {
      MUSNPrintF(&BPtr,&BSpace,"[%s+%s]",
        MTrigFlag[mtfGlobalVars],
        T->GetVars);

      continue;
      }

    if (bmisset(&T->SFlags,findex))
      {
      MUSNPrintF(&BPtr,&BSpace,"[%s]",(char *)MTrigFlag[findex]);
      } 
    }    /* END for (findex) */

  return(SUCCESS);
  }  /* END MTrigFlagsToString() */





/**
 *
 *
 * @param T
 * @param Delim
 * @param Buffer
 * @param BufSize
 */

int MTrigToAVString(

  mtrig_t *T,
  char     Delim,
  char    *Buffer,
  int      BufSize)

  {
  char *BPtr = NULL;
  int   BSpace = 0;

  char  tmpBuf[MMAX_BUFFER];

  /* return trigger is specified ATTR=VAL format */

  if ((Buffer == NULL) || (T == NULL))
    {
    return(FAILURE);
    }

  MUSNInit(&BPtr,&BSpace,Buffer,BufSize);

  MUSNPrintF(&BPtr,&BSpace,"atype=%s%cetype=%s%caction=\"%s\"",
    MTrigActionType[T->AType],
    Delim,
    MTrigType[T->EType],
    Delim,
    T->Action);

  if (T->Name != NULL)
    {
    MUSNPrintF(&BPtr,&BSpace,"%cName=%s",
      Delim,
      T->Name);
    }

  if (bmisset(&T->InternalFlags,mtifIsInterval))
    {
    MUSNPrintF(&BPtr,&BSpace,"%cinterval=TRUE",
     Delim);
    }

  if (bmisset(&T->InternalFlags,mtifMultiFire))
    {
    MUSNPrintF(&BPtr,&BSpace,"%cmultifire=TRUE",
      Delim);
    }

  if (T->BlockTime > 0)
    {
    MUSNPrintF(&BPtr,&BSpace,"%cblocktime=%ld",
      Delim,
      T->BlockTime);
    }

  if (T->Period != mpNONE)
    {
    MUSNPrintF(&BPtr,&BSpace,"%cperiod=%s",
      Delim,
      MCalPeriodType[T->Period]);
    } 

  if (T->RearmTime > 0)
    {
    MUSNPrintF(&BPtr,&BSpace,"%crearmtime=%ld",
      Delim,
      T->RearmTime);
    }

  if (!bmisclear(&T->SFlags))
    {
    MTrigFlagsToString(T,tmpBuf,sizeof(tmpBuf));
  
    MUSNPrintF(&BPtr,&BSpace,"%cflags=%s",
      Delim,
      tmpBuf);
    }

  if (T->MaxRetryCount > 0)
    {
    MUSNPrintF(&BPtr,&BSpace,"%cmaxretry=%d",
      Delim,
      T->MaxRetryCount);
    }

  if (T->ExpireTime > 0)
    {
    MUSNPrintF(&BPtr,&BSpace,"%cexpiretime=%ld", 
      Delim,
      T->ExpireTime);
    }

  if (T->FailOffset > 0)
    {
    MUSNPrintF(&BPtr,&BSpace,"%cfailoffset=%ld",
      Delim,
      T->FailOffset);
    }

  if (T->Offset > 0)
    {
    MUSNPrintF(&BPtr,&BSpace,"%coffset=%ld",
      Delim,
      T->Offset);
    }

  if (T->Timeout > 0)
    {
    MUSNPrintF(&BPtr,&BSpace,"%ctimeout=%ld",
      Delim,
      T->Timeout);
    }

  if (T->ThresholdType != mtttNONE)
    {
    MTrigAToString(T,mtaThreshold,tmpBuf,sizeof(tmpBuf),mfmNONE);

    MUSNPrintF(&BPtr,&BSpace,"%cthreshold=%s",
      Delim,
      tmpBuf);
    }

  if (T->Description != NULL)
    {
    MUSNPrintF(&BPtr,&BSpace,"%cdescription=\"%s\"",
      Delim,
      T->Description);
    }

  if (T->Depend.NumItems > 0)
    {
    MTrigAToString(T,mtaRequires,tmpBuf,sizeof(tmpBuf),mfmHuman);

    MUSNPrintF(&BPtr,&BSpace,"%crequires=%s",
      Delim,
      tmpBuf);
    }

  if (((T->SSets != NULL) && (T->SSets->NumItems > 0)) || 
      ((T->FSets != NULL) && (T->FSets->NumItems > 0)))
    {
    MTrigAToString(T,mtaSets,tmpBuf,sizeof(tmpBuf),mfmHuman);

    MUSNPrintF(&BPtr,&BSpace,"%csets=%s",
      Delim,
      tmpBuf);
    }

  if (((T->Unsets != NULL) && (T->Unsets->NumItems > 0)) || 
      ((T->FUnsets != NULL) && (T->FUnsets->NumItems > 0)))
    {
    MTrigAToString(T,mtaUnsets,tmpBuf,sizeof(tmpBuf),mfmHuman);

    MUSNPrintF(&BPtr,&BSpace,"%cunsets=%s",
      Delim,
      tmpBuf);
    }

  return(SUCCESS);
  }  /* END MTrigToAVString() */
/* END MTrigString.c */
