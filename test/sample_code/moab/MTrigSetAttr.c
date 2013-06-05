/* HEADER */

      
/**
 * @file MTrigSetAttr.c
 *
 * Contains: MTrigSetAttr
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 * Set specified trigger attribute.
 *
 * @see MTrigLoadString() - parent
 * @see MTrigInitiateAction() - parent
 *
 * @param T (I) [modified]
 * @param AIndex (I)
 * @param AVal (I)
 * @param Format (I)
 * @param Mode (I)
 */

int MTrigSetAttr(

  mtrig_t                *T,      /* I (modified) */
  enum MTrigAttrEnum      AIndex, /* I */
  void                   *AVal,   /* I */
  enum MDataFormatEnum    Format, /* I */
  enum MObjectSetModeEnum Mode)   /* I */

  {
  const char *FName = "MTrigSetAttr";

  MDB(7,fSTRUCT) MLog("%s(%s,%s,%s,%d,%d)\n",
    FName,
    ((T != NULL) && (T->TName != NULL)) ? T->TName : "NULL",
    MTrigAttr[AIndex],
    (AVal != NULL) ? "AVal" : "NULL",
    Format,
    Mode);

  if (T == NULL)
    {
    return(FAILURE);
    }

  switch (AIndex)
    {
    case mtaActionData:

      if (AVal != NULL)
        {
        if (MUCheckString((char *)AVal,MSched.TrustedCharList) != FAILURE)
          {
          MUStrDup(&T->Action,(char *)AVal); 
          }
        else
          {
          MDB(3,fSTRUCT) MLog("ALERT:    Trigger action '%s' contains characters not in the trusted character list ('%s')\n",
              (char *)AVal,
              MSched.TrustedCharList);

          return(FAILURE);
          }
        }
      else
        MUFree(&T->Action);

      break;

    case mtaActionType:

      if (AVal != NULL)
        {
        T->AType = (enum MTrigActionTypeEnum)MUGetIndexCI(
          (char *)AVal,
          MTrigActionType,
          TRUE,
          mtaNONE);
        }
      else
        {
        T->AType = mtatNONE;
        }

      break;

    case mtaBlockTime:

      if (AVal != NULL)
        T->BlockTime = MUTimeFromString((char *)AVal);
      else
        T->BlockTime = 0;

      break;

    case mtaDependsOn:

      if (AVal != NULL)
        {
        /* need to assign the trigger dep string */
        if (T->TrigDepString == NULL)
          T->TrigDepString = new mstring_t(MMAX_LINE);

        *T->TrigDepString = (char *) AVal;
        }
      else
        {
        /* Caller is requesting the string be cleared, if allocated, free it and mark as NULL */
        if (T->TrigDepString != NULL)
          {
          delete T->TrigDepString;
          T->TrigDepString = NULL;
          }
        }

      break;

    case mtaDescription:

      if (AVal != NULL)
        MUStrDup(&T->Description,(char *)AVal);
      else
        MUFree(&T->Description);

      break;

    case mtaDisabled:

      if (AVal != NULL)
        {
        bmsetbool(&T->InternalFlags,mtifDisabled,MUBoolFromString((char *)AVal,FALSE));
        }
      else
        {
        bmunset(&T->InternalFlags,mtifDisabled);
        }

      break;

    case mtaEBuf:

      if (AVal != NULL)
        MUStrDup(&T->EBuf,(char *)AVal);
      else
        MUFree(&T->EBuf);

      break;

    case mtaECPolicy:

      T->ECPolicy = (enum MTrigECPolicyEnum)MUGetIndexCI((char *)AVal,MTrigECPolicy,TRUE,mtaNONE);

      break;

    case mtaEnv:

      {
      /* Convert to internal format (using ENVRS_ENCODED_CHAR instead of ';') */

      mstring_t tmpS(MMAX_LINE);

      MStringSet(&tmpS,(char *)AVal);

      MTrigTranslateEnv(&tmpS,FALSE);

      MUStrDup(&T->Env,tmpS.c_str());
      }

      break;
    
    case mtaEventTime:

      if (AVal != NULL)
        {
        T->ETime = strtol((char *)AVal,NULL,10);
        }
      else
        {
        T->ETime = 0;
        }

      break;

    case mtaEventType:

      /* FORMAT:  <ETYPE> || epoch<XXX> */
      {
      char *ptr;
      char *TokPtr = NULL;

      ptr = MUStrTok((char *)AVal,": \n\t",&TokPtr);

      if (ptr != NULL)
        T->EType = (enum MTrigTypeEnum)MUGetIndexCI(ptr,MTrigType,TRUE,mtaNONE);
      else
        T->EType = mttNONE;

      if ((TokPtr != NULL) && (TokPtr[0] != '\0'))
        {
        if (T->EType == mttEpoch)
          {
          MUStringToE(TokPtr,(long *)&T->ETime,TRUE);
          }
        }

      if (T->EType == mttStanding)
        {
        bmset(&T->InternalFlags,mtifIsInterval);
        bmset(&T->InternalFlags,mtifMultiFire);
        }
      }

      break;

    case mtaExpireTime:

      /* FORMAT:  <EPOCHTIME> */

      if (AVal != NULL)
        {
        T->ExpireTime = strtol((char *)AVal,NULL,10);
        }
      else
        {
        T->ExpireTime = 0;
        }

      break;

    case mtaFailOffset:

      {
      char *ptr = (char *)AVal;

      if (AVal == NULL)
         break;

      if (*ptr == '+')
        {
        T->FailOffset = MUTimeFromString(ptr + 1);
        }
      else if (*ptr == '-')
        {
        T->FailOffset = MUTimeFromString(ptr + 1) * (-1);
        }
      else
        {
        T->FailOffset = MUTimeFromString(ptr);
        }

      if (strchr(ptr,'%') != NULL)
        {
        bmset(&T->InternalFlags,mtifThresholdIsPercent);
        }
      }

      break;

    case mtaFlags:

      MTrigSetFlags(T,(char *)AVal);      

      break;

    case mtaIBuf:

      if (AVal != NULL)
        MUStrDup(&T->IBuf,(char *)AVal);
      else
        MUFree(&T->IBuf);

      break;

    case mtaIsComplete:

      if (AVal != NULL)
        {
        bmsetbool(&T->InternalFlags,mtifIsComplete,MUBoolFromString((char *)AVal,FALSE));
        }
      else 
        {
        bmunset(&T->InternalFlags,mtifIsComplete);
        }

      break;

    case mtaIsInterval:

      if (AVal != NULL)
        {
        bmsetbool(&T->InternalFlags,mtifIsInterval,MUBoolFromString((char *)AVal,FALSE));
        }
      else
        {
        bmunset(&T->InternalFlags,mtifIsInterval);
        }

      break;

    case mtaLaunchTime:

      if (AVal != NULL)
        {
        T->LaunchTime = (int)strtol((char*)AVal,NULL,10);
        }
      else
        {
        T->LaunchTime = 0;
        }

      break;

    case mtaMaxRetry:

      if (AVal != NULL)
        {
        T->MaxRetryCount = (int)strtol((char*)AVal,NULL,10);
        }
      else
        {
        T->MaxRetryCount = 0;
        }

      break;

    case mtaRetryCount:

      T->RetryCount = (AVal != NULL) ? (int)strtol((char*)AVal,NULL,10) : 0;

      break;

    case mtaMessage:

      if (AVal != NULL)
        {
        MUStrDup(&T->Msg,(char *)AVal);
        }
      else
        {
        MUFree(&T->Msg);
        }

      break;

    case mtaMultiFire:

      if (AVal != NULL)
        {
        bmsetbool(&T->InternalFlags,mtifMultiFire,MUBoolFromString((char *)AVal,FALSE));
        }
      else
        {
        bmunset(&T->InternalFlags,mtifMultiFire);
        }

      break;

    case mtaName:

      if (AVal != NULL)
        MUStrDup(&T->Name,(char *)AVal);
      else
        MUFree(&T->Name);

      break;

    case mtaObjectID:

      if (AVal != NULL)
        MUStrDup(&T->OID,(char *)AVal);
      else
        MUFree(&T->OID);

      T->O = NULL;

      break;

    case mtaObjectType:

      if (AVal != NULL)
        T->OType = (enum MXMLOTypeEnum)MUGetIndexCI((char *)AVal,MXO,FALSE,mtaNONE);
      else
        T->OType = mxoNONE;

      T->O = NULL;

      break;

    case mtaOBuf:

      if (AVal != NULL)
        MUStrDup(&T->OBuf,(char *)AVal);
      else
        MUFree(&T->OBuf);

      break;

    case mtaOffset:

      {
      char *ptr = (char *)AVal;

      if (AVal == NULL)
         break;

      if (*ptr == '+')
        {
        T->Offset = MUTimeFromString(ptr + 1);
        }
      else if (*ptr == '-')
        {
        T->Offset = MUTimeFromString(ptr + 1) * (-1);
        }
      else
        {
        T->Offset = MUTimeFromString(ptr);
        }
      }

      break;

    case mtaPeriod:

      T->Period = (enum MCalPeriodEnum)MUGetIndexCI(
        (char *)AVal,
        MCalPeriodType,
        TRUE,
        mpDay);

      break;

    case mtaPID:

      if (AVal == NULL)
         break;

      if (Format == mdfInt)
        T->PID = *(int *)AVal;       
      else 
        T->PID = strtol((char *)AVal,NULL,10);

      break;

    case mtaRearmTime:

      if (AVal == NULL)
         break;

      if (Format == mdfLong)
        T->RearmTime = *(mulong *)AVal;
      else
        T->RearmTime = MUTimeFromString((char *)AVal);
      
      /* setting RearmTime implies that MultiFire is on. Make it so! MOAB-2710 */
      bmset(&T->SFlags,mtfMultiFire);
      bmset(&T->InternalFlags,mtifMultiFire);

      break;

    case mtaRequires:

      if (Format == mdfString)
        {
        mbool_t Not;

        enum MDependEnum DType;

        mbitmap_t Flags;

        char *ptr;
        char *TokPtr = NULL;

        char *ptr2;
        char *TokPtr2 = NULL;

        char *tmpTok = NULL;
        char *DName;
        char *DSValue;

        /* Check for allocated mstring, if not, allocate one */
        if (T->TrigDepString == NULL)
          T->TrigDepString = new mstring_t(MMAX_LINE);

        ptr = MUStrTokE((char *)AVal,".",&TokPtr);

        if (T->Depend.Array == NULL)
          MUArrayListCreate(&T->Depend,sizeof(mdepend_t *),MDEF_DEPLIST_SIZE);

        while (ptr != NULL)
          {
          Not = FALSE;

          /* add to dependency list */

          /* FORMAT: [*][^][%]<VARID>[:<TYPE>:<VARVAL>] */

          DType   = mdVarSet;
          DName   = NULL;
          DSValue = NULL;  /* dependency set value */

          bmclear(&Flags);

          DName = MUStrTokE(ptr,": \t\n",&TokPtr2);

          if ((TokPtr2 != NULL) && (TokPtr2[0] != '\0')) 
            {
            char *TokPtr3 = NULL;

            /* may have dependency type--see if we have a value as well */

            ptr2 = MUStrTokE(NULL,": \t\n",&TokPtr2);

            DType = (enum MDependEnum)MUGetIndexCI(ptr2,MDependType,FALSE,mdVarSet);

            if ((TokPtr2 != NULL) && (TokPtr2[0] != '\0'))
              {
              DSValue = MUStrTok(TokPtr2,"\"'",&TokPtr3);
              }
            }

          if (strchr(DName,'*') != NULL)
            {
            bmset(&Flags,mdbmImport);
            }

          if (strchr(DName,'^') != NULL)
            {
            bmset(&Flags,mdbmExtSearch);
            }

          if (strchr(DName,'!') != NULL)
            {
            Not = TRUE;
            }

          DName = MUStrTokE(DName,"^!*",&tmpTok);

#ifdef MYAHOO3
          if (strchr(DValue,'%') != NULL)
            {
            char tmpLine[MMAX_LINE];

            /* this variable represents another trigger's name--add to the "depends-on" list */

            DValue = MUStrTokE(DValue,"%",&tmpTok);

            snprintf(tmpLine,sizeof(tmpLine),"%s_WATCHER", DValue);

            /* If first instance on the line, then don't add the delimiter */
            if (!T->TrigDepString->empty())
              {
                /* Add the delimiter between items */
                T->TrigDepString += '.';
              }

            T->TrigDepString += tmpLine;
            }
#endif /* MYAHOO3 */

          if ((DName == NULL) || (DName[0] == '\0'))
            {
            ptr = MUStrTokE(NULL,".",&TokPtr);

            continue;
            }

          if (Not == TRUE)
            {
            MDAGSetAnd(
              &T->Depend,
              mdVarNotSet,
              DName,
              DSValue,
              &Flags);
            }
          else
            {
            MDAGSetAnd(
              &T->Depend,
              DType,
              DName,
              DSValue,
              &Flags);
            }

          ptr = MUStrTokE(NULL,".",&TokPtr);
          }
        }  /* END if (Format == mdfString) */
      else
        {
        /* NYI */

        return(FAILURE);
        }

      break;
      
    case mtaSets:

      if (Format == mdfString)
        {
        char  *ptr;
        char  *TokPtr = NULL;

        char  *ptr2;
        char  *TokPtr2 = NULL;

        mbool_t DoIncr;
        mbool_t DoExport;
        mbool_t DoFailure;

        ptr = MUStrTok((char *)AVal,".",&TokPtr);

        while (ptr != NULL)
          {
          DoIncr    = FALSE;
          DoExport  = FALSE;
          DoFailure = FALSE;
 
          /* add set to list */

          if (strchr(ptr,'+') != NULL)
            {
            DoIncr = TRUE;
            }

          if (strchr(ptr,'^') != NULL)
            {
            DoExport = TRUE;
            }

          if (strchr(ptr,'!') != NULL)
            {
            DoFailure = TRUE;
            }

          ptr2 = MUStrTok(ptr,"+!^$",&TokPtr2);

          if (DoFailure == TRUE)
            {
            char *tmpString = NULL;
            int tmpInt = 0;

            MTrigInitSets(&T->FSets,FALSE);
            MTrigInitSets(&T->FSetsFlags,TRUE);

            if (T->FSets->Array == NULL)
              {
              MUArrayListCreate(T->FSets,sizeof(char *),MDEF_DEPLIST_SIZE);
              }

            if (T->FSetsFlags->Array == NULL)
              {
              MUArrayListCreate(T->FSetsFlags,sizeof(int),MDEF_DEPLIST_SIZE);
              }

            MUStrDup(&tmpString,ptr2);

            MUArrayListAppendPtr(T->FSets,tmpString);

            if (DoIncr == TRUE)
              MOLDSET(tmpInt,mtvaIncr);

            if (DoExport == TRUE)
              MOLDSET(tmpInt,mtvaExport);

            MUArrayListAppend(T->FSetsFlags,&tmpInt);
            }  /* END if (DoFailure == TRUE) */
          else
            {
            char *tmpString = NULL;
            int tmpInt = 0;

            MTrigInitSets(&T->SSets,FALSE);
            MTrigInitSets(&T->SSetsFlags,TRUE);

            if (T->SSets->Array == NULL)
              {
              MUArrayListCreate(T->SSets,sizeof(char *),MDEF_DEPLIST_SIZE);
              }

            if (T->SSetsFlags->Array == NULL)
              {
              MUArrayListCreate(T->SSetsFlags,sizeof(int),MDEF_DEPLIST_SIZE);
              }

            MUStrDup(&tmpString,ptr2);

            MUArrayListAppendPtr(T->SSets,tmpString);

            if (DoIncr == TRUE)
              MOLDSET(tmpInt,mtvaIncr);

            if (DoExport == TRUE)
              MOLDSET(tmpInt,mtvaExport);

            MUArrayListAppend(T->SSetsFlags,&tmpInt);
            }

          ptr = MUStrTok(NULL,".",&TokPtr);
          }
        }   /* END if (Format == mdfString) */
      else
        {
        /* NYI */
        }

      break;

    case mtaState:

      if (Format == mdfString)
        {
        T->State = (enum MTrigStateEnum)MUGetIndexCI(
          (char *)AVal,
          MTrigState,
          FALSE,
          mtsNONE);
        }

      break;

    case mtaStdIn:

      if (AVal != NULL)
        {
        if (MUCheckString((char *)AVal,MSched.TrustedCharList) != FAILURE)
          {
          MUStrDup(&T->StdIn,(char *)AVal);
          }
        else
          {
          MDB(3,fSTRUCT) MLog("ALERT:    Trigger action '%s' contains characters not in the trusted character list ('%s')\n",
              (char *)AVal,
              MSched.TrustedCharList);

          return(FAILURE);
          }
        }
      else
        {
        MUFree(&T->StdIn);
        }

      break;

    case mtaThreshold:

      {
      /* FORMAT: [{USAGE|GMETRIC[<type>]}<cmp>]Value */

      /* NYI: support for QueueTime and GMetric */

      int ThresholdType;
      int CType;
  
      char tmpType[MMAX_NAME];
      char ValLine[MMAX_NAME];

      if ((AVal == NULL) || isdigit(((char *)AVal)[0]))
        {
        if (AVal == NULL)
          T->Threshold = 0.0;
        else
          T->Threshold = strtod((char *)AVal,NULL);

        T->ThresholdType = mtttUsage;

        return(SUCCESS);
        }

      if (strchr((char *)AVal,'%') != NULL)
        {
        bmset(&T->InternalFlags,mtifThresholdIsPercent);
        }

      if (MUGetPairCI(
            (char *)AVal,
            (const char **)MTrigThresholdType,
            &ThresholdType,
            tmpType,
            FALSE,
            &CType,
            ValLine,
            sizeof(ValLine)) == FAILURE)
        {
        return(FAILURE);
        }

      T->ThresholdType = (enum MTrigThresholdTypeEnum)ThresholdType;

      if (tmpType[0] != '\0')
        MUStrDup(&T->ThresholdGMetricType,tmpType);

      T->ThresholdCmp = (enum MCompEnum)CType;

      T->Threshold = strtod((char *)ValLine,NULL);

      if (bmisset(&T->InternalFlags,mtifThresholdIsPercent))
        {
        T->Threshold = T->Threshold / 100;
        }
      }  /* END BLOCK (case mtaThreshold) */

      break;

    case mtaTimeout:

      if (AVal == NULL)
         break;

      if (Format == mdfLong)
        {
        T->Timeout = *((long *)AVal);
        }
      else
        {
        /* FORMAT: [=-][[[DD:]HH:]MM:]SS */

        if ((((char *)AVal)[0] == '+') || (((char *)AVal)[0] == '-'))
          bmset(&T->InternalFlags,mtifTimeoutIsRelative);

        T->Timeout = MUTimeFromString((char *)AVal);
        }

      break;

    case mtaTrigID:

      if (AVal == NULL)
        {
        MUFree(&T->TName);

        break;
        }

      {
      int TCount;
      
      MUStrDup(&T->TName,(char *)AVal);

      TCount = strtol((char *)AVal,NULL,10);

      MSched.TrigCounter = MAX(MSched.TrigCounter,TCount + 1);
      }  /* END case mtaTrigID */

      break;

    case mtaUnsets:

      if (Format == mdfString)
        {
        char  *tmpPtr = NULL;
        char  *ptr2;
        char  *TokPtr2 = NULL;

        ptr2 = MUStrTok((char *)AVal,".",&TokPtr2);

        while (ptr2 != NULL)
          {
          /* add set to list */

          if (ptr2[0] == '!')
            {
            MTrigInitSets(&T->FUnsets,FALSE);

            if (T->FUnsets->Array == NULL)
              {
              MUArrayListCreate(T->FUnsets,sizeof(char *),MDEF_DEPLIST_SIZE);
              }

            tmpPtr = NULL;

            MUStrDup(
              &tmpPtr,
              (ptr2[1] != '$') ? ptr2 + 1: ptr2 + 2);

            MUArrayListAppendPtr(T->FUnsets,tmpPtr);
            }
          else
            {
            MTrigInitSets(&T->Unsets,FALSE);

            if (T->Unsets->Array == NULL)
              {
              MUArrayListCreate(T->Unsets,sizeof(char *),MDEF_DEPLIST_SIZE);
              }

            tmpPtr = NULL;

            MUStrDup(
              &tmpPtr,
              (ptr2[0] != '$') ? ptr2 : ptr2 + 1);

            MUArrayListAppendPtr(T->Unsets,tmpPtr);
            }

          ptr2 = MUStrTok(NULL,".",&TokPtr2);
          }
        }   /* END if (Format == mdfString) */
      else
        {
        /* NYI */
        }

      break;

    default:

      /* specified attribute not supported */

      return(FAILURE);

      /*NOTREACHED*/

      break;
    }  /* END switch (AIndex) */

  return(SUCCESS);
  }  /* END MTrigSetAttr() */
/* END MTrigSetAttr.c */
