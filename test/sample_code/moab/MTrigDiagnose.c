/* HEADER */

      
/**
 * @file MTrigDiagnose.c
 *
 * Contains: MTrigDiagnose
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  






/**
 *
 *
 * @param A
 * @param B
 */

int __MTrigETimeLToH(

  mtrig_t **A,
  mtrig_t **B)

  {
  /* order low to high */

  return((*A)->ETime - (*B)->ETime);
  }  /* END __MTrigETimeLToH() */





/**
 *
 *
 * @param A
 * @param B
 */

int __MTrigAlphaLToH(

  mtrig_t **A,
  mtrig_t **B)

  {
  /* order low to high */

  return(strcmp((*A)->TName,(*B)->TName));
  }  /* END __MTrigAlphaLToH() */




/**
 * Report trigger diagnostics, configuration, and state.
 *
 * NOTE: Process 'mdiag -T' request.
 *
 * @see MUIDiagnose() - parent
 *
 * @param Auth    (I) (FIXME: not used but should be)
 * @param ReqE    (I) [optional,contains 'where' constraints]
 * @param SXML    (O) [XML buffer]
 * @param String  (O)
 * @param DFormat (I)
 * @param Flags   (I) [enum MCModeEnum bitmap]
 * @param TListP  (I) triggers to evaluate [optional]
 * @param Mode    (I) [enum]
 */

int MTrigDiagnose(

  char                   *Auth, 
  mxml_t                 *ReqE,
  mxml_t                 *SXML,
  mstring_t              *String,
  enum MFormatModeEnum    DFormat,
  mbitmap_t              *Flags,
  marray_t               *TListP,
  enum MObjectSetModeEnum Mode)

  {
  enum MXMLOTypeEnum OType;
  char OID[MMAX_LINE];

  int tindex;
  int NumTrigsChecked;

  mtrig_t *T;

  enum MTrigStateEnum  ReqState;             /* required trigger state */
  char                 ReqName[MMAX_NAME];   /* required name */

  mclass_t            *FilterC = NULL;

  char  *DMsg = NULL;
  char   TString[MMAX_LINE];

  marray_t *TList;

  const char *FName = "MTrigDiagnose";

  MDB(2,fSCHED) MLog("%s(%s,%s,SXML,TList,%d)\n",
    FName,
    Auth,
    (ReqE != NULL) ? "ReqE" : "NULL",
    MFormatMode[DFormat],
    Mode);

#ifndef __MOPT
  if ((SXML == NULL) && (String == NULL))
    {
    return(FAILURE);
    }
#endif  /* !__MOPT */

  /* initialize variables */

  OID[0] = '\0';
  OType = mxoNONE;
  ReqName[0] = '\0';
  ReqState   = mtsLAST;

  if (ReqE != NULL)
    {
    int WTok;

    mxml_t *WE;

    char tmpName[MMAX_NAME];
    char tmpVal[MMAX_LINE];

    /* extract where */

    WTok = -1;

    while (MS3GetWhere(
        ReqE,
        &WE,
        &WTok,
        tmpName,
        sizeof(tmpName),
        tmpVal,
        sizeof(tmpVal)) == SUCCESS)
      {
      if (!strcasecmp(tmpName,"state"))
        {
        ReqState = (enum MTrigStateEnum)MUGetIndexCI(tmpVal,MTrigState,FALSE,0);

        if (ReqState == mtsNONE)
          {
          if (!strcmp(tmpVal,"blocked"))
            ReqState = mtsNONE;
          }
        }
      else if (!strcasecmp(tmpName,"otype"))
        {
        /* what is this otype?  how is it set? */
   
        OType = (enum MXMLOTypeEnum)MUGetIndexCI(tmpVal,MXO,FALSE,mxoNONE);
        }
      else if (!strcasecmp(tmpName,"oid"))
        {
        MUStrCpy(OID,tmpVal,sizeof(OID));
        }
      else if (!strcasecmp(tmpName,"name"))
        {
        MUStrCpy(ReqName,tmpVal,sizeof(ReqName));
        }
      else if (!strcasecmp(tmpName,"class"))
        {
        if (MClassFind(tmpVal,&FilterC) == FAILURE)
          {
          return(FAILURE);
          }
        }
      }    /* END while (MS3GetWhere() == SUCCESS) */

    MXMLGetAttr(ReqE,MSAN[msanOp],NULL,tmpVal,sizeof(tmpVal));

    if (!MUStrIsEmpty(tmpVal))
      {
      if (strchr(tmpVal,':') != NULL)
        {
        char tmpLine[MMAX_LINE];
   
        char *ptr;
        char *TokPtr = NULL;
   
        /* FORMAT:  [<OTYPE>:]OID */
   
        MUStrCpy(tmpLine,tmpVal,sizeof(tmpLine));
   
        if ((ptr = MUStrTok(tmpLine,":",&TokPtr)) != NULL)
          {
          OType = (enum MXMLOTypeEnum)MUGetIndexCI(ptr,MXO,FALSE,mxoNONE);
          }
   
        if ((ptr = MUStrTok(NULL,":",&TokPtr)) != NULL)
          {
          MUStrCpy(OID,ptr,sizeof(OID));
          }
        }    /* END else if (strchr(tmpVal,':') != NULL) */
      else
        {
        MUStrCpy(OID,tmpVal,sizeof(OID));
        }
      }
    }      /* END if (ReqE != NULL) */

  /* focus search to only include triggers attached to given
   * object (if specified) */

  if ((OID != NULL) &&
      (OType == mxoJob) &&
      (TListP == NULL))
    {
    mjob_t *J;

    if (MJobFind(OID,&J,mjsmExtended) == SUCCESS)
      {
      TListP = J->Triggers;

      /* need to do this to avoid checks in the loops below */

      OID[0] = '\0';
      }
    }

  mstring_t BufString(MMAX_BUFFER);

  if (TListP != NULL)
    {
    TList = TListP;
    }
  else
    {
    TList = &MTList;
    }

  switch (DFormat)
    {
    case mfmXML:

      {
      enum MTrigAttrEnum DAList[] = {
        mtaActionData,
        mtaActionType,
        mtaBlockTime,
        mtaDescription,
        mtaEBuf,
        mtaECPolicy,
        mtaEventTime,
        mtaEventType,
        mtaExpireTime,
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
        mtaStdIn,
        mtaThreshold,
        mtaTimeout,
        mtaTrigID,
        mtaNONE };

      mxml_t *DE;
      mxml_t *TE;

      DE = SXML;

      for (tindex = 0;tindex < TList->NumItems;tindex++)
        {
        T = (mtrig_t *)MUArrayListGetPtr(TList,tindex);

        if (MTrigIsReal(T) == FALSE)
          continue;

        if ((OType != mxoNONE) && (T->OType != OType))
          continue;

        if ((!MUStrIsEmpty(OID)) &&
            strcmp(OID,T->OID) &&
            strcmp(OID,T->TName) &&
            ((T->Name == NULL) || (strcmp(OID,T->Name))))
          continue;

        if ((FilterC != NULL) && 
           ((T->OType != mxoClass) || (strcmp(T->OID,FilterC->Name))))
          continue;

        if ((ReqState != mtsLAST) && (ReqState != T->State))
          continue;

        if ((ReqName[0] != '\0') && 
           ((T->Name == NULL) || (strncasecmp(ReqName,T->Name,strlen(ReqName)))))
          continue;

        TE = NULL;

        if (MXMLCreateE(&TE,(char *)MXO[mxoTrig]) == FAILURE)
          {
          /* cannot create object */

          continue;
          }

        if (MTrigToXML(T,TE,DAList) == FAILURE)
          {
          /* cannot populate xml */

          continue;
          }

        if (!bmisclear(&T->SFlags))
          {
          mstring_t tmp(MMAX_LINE);

          bmtostring(&T->SFlags,MTrigFlag,&tmp);
      
          MXMLSetAttr(TE,(char *)MTrigAttr[mtaFlags],(void *)tmp.c_str(),mdfString);
          }

        /* add to buffer */

        MXMLAddE(DE,TE);
        }  /* END for (tindex) */
      }    /* END BLOCK (case mfmXML) */

      break;

    case mfmHuman:
    default:

      {
      char *ptr;

      int   index;

      char  tmpName[MMAX_LINE];
      char  tmpADate[MMAX_NAME];
      char  tmpLaunch[MMAX_NAME];
      char  tmpRequires[MMAX_BUFFER];
      char  tmpObject[MMAX_LINE];
      char  tmpState[MMAX_NAME];

      char  EMsg[MMAX_LINE];

      mulong tmpUL;

      mbool_t ValueFound = FALSE;
      mbool_t TrigCompleted = FALSE;

      int    rc = FAILURE;  /* set for compiler warning */

      mtrig_t **TSort;

      int      TCount;

      /* convert DMsg to a mstring someday */

      mstring_t DepString(MMAX_LINE);

      if (Mode == mSet)
        {
        MStringSet(String,"\0");
        }

      if ((OID != NULL) && (OID[0] != '\0'))
        bmset(Flags,mcmSummary);

      /* display header */

      /* FORMAT:  TID      OBJECT:OID EVENT ATYPE ACTIONDATE STATE   */
      /*          %-21.21s  %20.20s    %8.8s %6.6s %20.20s    %11.11s */
#ifdef MYAHOO
      MStringAppendF(String,"%-21s %-20s %8.8s %6.6s %20.20s %11.11s\n",
#else
      MStringAppendF(String,"%-21.21s %-20.20s %8.8s %6.6s %20.20s %11.11s\n",
#endif /* MYAHOO */
        "TrigID",
        "Object ID",
        "Event",
        "AType",
        "ActionDate",
        "State");

#ifdef MYAHOO
      MStringAppendF(String,"%-21s %-20s %8.8s %6.6s %20.20s %11.11s\n",
#else
     MStringAppendF(String,"%-21.21s %-20.20s %8.8s %6.6s %20.20s %11.11s\n",
#endif /* MYAHOO */
        "-----------------------",
        "-----------------------",
        "-----------------------",
        "-----------------------",
        "-----------------------",
        "-----------------------");

      /* filter triggers */

      TCount = 0;

      NumTrigsChecked = 0;

      TSort = (mtrig_t **)MUCalloc(1,sizeof(mtrig_t *) * (TList->NumItems + 1));

      for (tindex = 0;tindex < TList->NumItems;tindex++)
        {
        T = (mtrig_t *)MUArrayListGetPtr(TList,tindex);

        if (MTrigIsReal(T) == FALSE)
          continue;

        NumTrigsChecked++;

        if ((OType != mxoNONE) && (T->OType != OType))
          continue;

        if ((ReqState != mtsLAST) && (ReqState != T->State))
          continue;

        if ((ReqName[0] != '\0') && 
           ((T->Name == NULL) || (strncasecmp(ReqName,T->Name,strlen(ReqName)))))
          continue;

        if ((OID != NULL) &&
            (OID[0] != '\0') &&
            strcmp(OID,NONE) &&
            strcmp(OID,T->OID) &&
            strcmp(OID,T->TName) &&
            ((T->Name == NULL) || (strcmp(OID,T->Name))))
          continue;

        ValueFound = TRUE;

        TSort[TCount++] = T;
        }  /* END for (tindex) */

      /* sort by EventTime (L -> H) */

      /* NOTE:  also support alphabetic sort, TID sort, and CreateTime sort */

      if (bmisset(Flags,mcmSort))
        {
        qsort(
          (void *)TSort,
          TCount,
          sizeof(mtrig_t *),
          (int(*)(const void *,const void *))__MTrigAlphaLToH);
        }
      else
        {
        qsort(
          (void *)TSort,
          TCount,
          sizeof(mtrig_t *),
          (int(*)(const void *,const void *))__MTrigETimeLToH);
        }

      for (tindex = 0;tindex < TCount;tindex++)
        {
        T = TSort[tindex];

        if (bmisset(Flags,mcmSummary) || bmisset(Flags,mcmVerbose))
          {
          double Usage;

          rc = MTrigCheckDependencies(T,Flags,&DepString);

          if (T->State == mtsNONE)
            {
            /* if trigger is blocked, indicate block reason */

            if ((T->ETime > MSched.Time) && 
                (!bmisset(&T->InternalFlags,mtifIsComplete)))
              {
              MULToTString(T->ETime - MSched.Time,TString);

              MStringSetF(&BufString,"(event time in %s)",
                TString);
              }
            else if ((T->ThresholdType != mtttNONE) &&
                     (!bmisset(&T->InternalFlags,mtifOIsRemoved)) &&
                     (MTrigCheckThresholdUsage(T,&Usage,EMsg) == FAILURE))
              {
              MStringSetF(&BufString,"(usage %f does not meet threshold %s)",
                Usage,
                EMsg);
              }
            else if (rc == FAILURE)
              {
              MStringSetF(&BufString,"(%s)",
                DepString.c_str());
              }
            else if ((T->O != NULL) && (T->OType == mxoJob))
              {
              mjob_t *J = (mjob_t *)T->O;

              if (MJOBISACTIVE(J) != TRUE)
                {
                MStringSetF(&BufString,"(parent job %s is in state %s)",
                  J->Name,
                  MJobState[J->State]);
                }
              }
            }    /* END if (T->State == mtsNONE) */
          }      /* END if (bmisset(Flags,mcmSummary) || bmisset(Flags,mcmVerbose)) */

        if (!bmisset(Flags,mcmSummary))
          MStringSet(&BufString,"");

        if (T->ETime > 0)
          {
          if (bmisset(Flags,mcmVerbose))
            {
            char DString[MMAX_LINE];

            tmpUL = (mulong)T->ETime;
            MULToDString(&tmpUL,DString);

            strcpy(tmpADate,DString);
            }
          else
            {
            MULToTString(T->ETime - MSched.Time,TString);

            strcpy(tmpADate,TString);
            }
          }
        else
          {
          strcpy(tmpADate,"-");
          }

        if ((ptr = strrchr(tmpADate,'\n')) != NULL)
          {
          *ptr = '\0';
          }

        if (T->LaunchTime != 0)
          {
          MULToTString(T->LaunchTime - MSched.Time,TString);

          strcpy(tmpLaunch,TString);
          }
        else
          {
          tmpLaunch[0] = '\0';
          }

        if ((T->OType != mxoNONE) && (T->OID != NULL))
          {
          char AName[MMAX_NAME];  /* alternate user-specified object name */

          AName[0] = '\0';

          if (T->OType == mxoJob)
            {
            mjob_t *tmpJ;

            if ((MJobFind(T->OID,&tmpJ,mjsmExtended) == SUCCESS) && (tmpJ->AName != NULL))
              snprintf(AName,sizeof(AName),"/%s",
                tmpJ->AName);
            }

          snprintf(tmpObject,sizeof(tmpObject),"%s:%s%s",
            MXO[T->OType],
            T->OID,
            AName);
          }

#ifdef MYAHOO
        if (T->Name != NULL)
          {
          snprintf(tmpName,sizeof(tmpName),"%s",T->Name);
          }
#else
         if (T->TName != NULL)
          {
          snprintf(tmpName,sizeof(tmpName),"%s",T->TName);
          }
#endif /* MYAHOO */
        else
          {
          strcpy(tmpName,"???");
          }

        if (bmisset(&T->InternalFlags,mtifIsComplete))
          {
          TrigCompleted = TRUE;

          strncat(tmpName,"*",sizeof(tmpName) - strlen(tmpName));
          }

        if (bmisset(&T->InternalFlags,mtifDisabled))
          strcpy(tmpState,"Disabled");
        else if (T->State != mtsNONE)
          {
          if (bmisset(Flags,mcmSummary) && MTRIGISRUNNING(T))
            {
            MULToTString(MSched.Time - T->LaunchTime,TString);

            MStringSetF(&BufString,"(active for %s)",
              TString);
            }

          strcpy(tmpState,MTrigState[T->State]);
          }
        else if (MSched.Time < T->ETime)
          strcpy(tmpState,"Idle");
        else
          strcpy(tmpState,"Blocked");

#ifdef MYAHOO
        MStringAppendF(String,"%-21s %-20.20s %8.8s %6.6s %20.20s %11.11s  %s\n",
#else
        MStringAppendF(String,"%-21.21s %-20.20s %8.8s %6.6s %20.20s %11.11s  %s\n",
#endif /* MYAHOO */
          tmpName,
          tmpObject,
          (T->EType != mttNONE) ? MTrigType[T->EType] : "-",
          (T->AType != mtatNONE) ? MTrigActionType[T->AType] : "-",
          tmpADate,
          tmpState,
          BufString.c_str());

        if (bmisset(Flags,mcmVerbose))
          {
          if (T->Name != NULL)
            {
            /* user specified name */

            MStringAppendF(String,"  Name:          %s\n",
              T->Name);
            }

          if (tmpLaunch[0] != '\0')
            {
            MStringAppendF(String,"  Launch Time:   %s\n",
              tmpLaunch);
            }

          if (T->ExpireTime > 0)
            {
            MULToTString(T->ExpireTime - MSched.Time,TString);

            MStringAppendF(String,"  Expire Time:   %s\n",
              TString);
            }

          if  (T->MaxRetryCount > 0)
            {
            MStringAppendF(String,"  Max Retries:   %d  Total Tries: %d\n",
              T->MaxRetryCount,
              T->RetryCount);
            }

          if (T->Description != NULL)
            {
            MStringAppendF(String,"  Description:   %s\n",
              T->Description);
            }

          if (T->UserName != NULL)
            {
            MStringAppendF(String,"  Exec User:     %s\n",
              T->UserName);
            }

          if (!bmisclear(&T->SFlags))
            {
            mstring_t tmp(MMAX_LINE);

            bmtostring(&T->SFlags,MTrigFlag,&tmp);

            MStringAppendF(String,"  Flags:         %s\n",
              tmp.c_str());
            }

          if (T->FailOffset != 0)
            {
            MULToTString(T->FailOffset,TString);

            MStringAppendF(String,"  FailOffset:    %s\n",
              TString);

            if (T->FailTime != 0)
              {
              MULToTString(T->FailTime - MSched.Time,TString);

              MStringAppendF(String,"  Last Fail Time: %s\n",
                strcpy(tmpADate,TString));
              }
            }

          if (T->LastExecStatus != mtsNONE)
            {
            MStringAppendF(String,"  Last Execution State: %s (ExitCode: %d)\n",
              (T->State != mtsNONE) ? MTrigState[T->State] : "---",
              T->ExitCode);
            }

          if (MSched.Time > T->ETime)
            { 
            char BTimeString[MMAX_NAME];  /* block time */
            char ATimeString[MMAX_NAME];  /* active time */

            char *bptr;

            mulong BTime;
            mulong ATime;

            if (T->LaunchTime > 0)
              BTime = T->LaunchTime - T->ETime;
            else
              BTime = MSched.Time - T->ETime;

            MULToTString(BTime,TString);

            strcpy(BTimeString,TString);

            if (T->LaunchTime > 0)
              {
              if (T->EndTime != 0)
                ATime = T->EndTime - T->LaunchTime;
              else
                ATime = MSched.Time - T->LaunchTime;

              MULToTString(ATime,TString);

              strcpy(ATimeString,TString);
              }
            else
              {
              strcpy(ATimeString,"---");
              }

            /* remove leading white space */

            for (bptr = BTimeString;*bptr == ' ';bptr++);
                      
            MStringAppendF(String,"  BlockUntil:    %s  ActiveTime:  %s\n",
              bptr,
              ATimeString);
            }  /* END if (MSched.Time > T->ETime) */

          if (T->PID != 0)
            {
            MStringAppendF(String,"  PID:           %d\n",
              T->PID);
            }

          if (T->BlockTime != 0)
            {
            MULToTString(T->BlockTime,TString);

            MStringAppendF(String,"  Block Time:    %s\n",
              TString);
            }

          if (T->Timeout != 0)
            {
            MULToTString(T->Timeout,TString);

            MStringAppendF(String,"  Timeout:       %s\n",
              TString);
            }

          if (T->ThresholdType != mtttNONE)
            {
            char tmpLine[MMAX_LINE];

            MTrigAToString(T,mtaThreshold,tmpLine,sizeof(tmpLine),mfmHuman);

            if (bmisset(Flags,mcmVerbose) && !bmisset(Flags,mcmSummary))
              { 
              double Usage;

              MTrigCheckThresholdUsage(T,&Usage,NULL);

              MStringAppendF(String,"  Threshold:     %s  (current value: %.2f)\n",
                tmpLine,
                Usage);
              }
            else
              {
              MStringAppendF(String,"  Threshold:     %s\n",
                tmpLine);
              }
            }

          if (T->RearmTime != 0)
            {
            MULToTString(T->RearmTime,TString);

            MStringAppendF(String,"  RearmTime:     %s\n",
              TString);
            }

          if (T->ECPolicy != mtecpNONE)
            {
            MStringAppendF(String,"  EC Policy:     %s\n",
              MTrigECPolicy[T->ECPolicy]);
            }

          if (T->Action != NULL)
            {
            MStringAppendF(String,"  Action Data:   %s\n",
              T->Action);
            }

          if (T->StdIn != NULL)
            {
            MStringAppendF(String,"  StdIn:         %s\n",
              T->StdIn);
            }

          if ((T->SSets != NULL) && (T->SSets->NumItems > 0))
            {
            char *tmpElement;

            MStringAppendF(String,"  Sets on Success: ");

            index = 0;

            while ((tmpElement = (char *)MUArrayListGetPtr(T->SSets,index++)) != NULL)
              {
             MStringAppendF(String," %s",tmpElement);
              }
  
            MStringAppendF(String,"\n");
            }  /* END if (T->SSets != NULL) */

          if ((T->FSets != NULL) && (T->FSets->NumItems > 0))
            {
            char *tmpElement;

            MStringAppendF(String,"  Sets on Failure: ");

            index = 0;

            while ((tmpElement = (char *)MUArrayListGetPtr(T->FSets,index++)) != NULL)
              {
              MStringAppendF(String," %s",tmpElement);
              }

            MStringAppendF(String,"\n");
            }  /* END if (T->FSets != NULL) */

          if ((T->Unsets != NULL) && (T->Unsets->NumItems > 0))
            {
            char *tmpElement;
            MStringAppendF(String,"  Unsets on Success: ");

            index = 0;

            while ((tmpElement = (char *)MUArrayListGetPtr(T->Unsets,index++)) != NULL)
              {
              MStringAppendF(String," %s",tmpElement);
              }
  
            MStringAppendF(String,"\n");
            } /* END if ((T->Unsets != NULL) ... */

          if ((T->FUnsets != NULL) && (T->FUnsets->NumItems > 0))
            {
            char *tmpElement;
            MStringAppendF(String,"  Unsets on Failure: ");

            index = 0;

            while ((tmpElement = (char *)MUArrayListGetPtr(T->FUnsets,index++)) != NULL)
              {
              MStringAppendF(String," %s",tmpElement);
              }

            MStringAppendF(String,"\n");
            } /* END if (T->FUnsets != NULL) ... */

          if (T->Depend.NumItems > 0)
            {
            MDAGToString(&T->Depend,tmpRequires,sizeof(tmpRequires));

            MStringAppendF(String,"  Requires:      %s\n", 
              tmpRequires);
            }

          if (T->OBuf != NULL)
            {
            MStringAppendF(String,"  StdOut:        %s\n",
              T->OBuf);
            }

          if (T->EBuf != NULL)
            {
            MStringAppendF(String,"  StdErr:        %s\n",
              T->EBuf);
            }

          if (bmisset(&T->InternalFlags,mtifFailureDetected))
            {
            MStringAppendF(String,"  ALERT:  trigger failure detected\n");
            }

          if (T->Msg != NULL)
            {
            MStringAppendF(String,"  Message:       '%s'\n",
              T->Msg);
            }

          if (T->Status != 0.0)
            {
            MStringAppendF(String,"  TrigStatus:    %.02f\n",
              T->Status);
            }

          /* summary set w/'-V' */

          if (bmisset(Flags,mcmVerbose) && !bmisset(Flags,mcmSummary))
            {
            double Usage;
    
            if (T->State == mtsNONE)
              {
              /* if trigger is blocked, indicate block reason */
    
              if (T->ETime > MSched.Time)
                {
                MULToTString(T->ETime - MSched.Time,TString);

                MStringAppendF(String,"  NOTE:  trigger cannot launch - event time not reached for %s\n",
                  TString);
                }
              else if ((T->ThresholdType != mtttNONE) &&
                       (!bmisset(&T->InternalFlags,mtifOIsRemoved)) &&
                       (MTrigCheckThresholdUsage(T,&Usage,EMsg) == FAILURE))
                {
                MStringAppendF(String,"  NOTE:  trigger cannot launch - threshold not satisfied - %s\n",
                  EMsg);
                }
              else if (rc == FAILURE)
                {
                MStringAppendF(String,"  NOTE:  trigger cannot launch - %s\n",
                  DMsg);
                }
              else if ((T->O != NULL) && (T->OType == mxoJob))
                {
                mjob_t *J = (mjob_t *)T->O;

                if (MJOBISACTIVE(J) != TRUE)
                  {
                  MStringAppendF(String,"  NOTE:  trigger cannot launch - parent job %s is in state %s\n",
                    J->Name,
                    MJobState[J->State]);
                  }
                }
              else
                {
                MStringAppendF(String,"  NOTE:  trigger can launch\n");
                }
              }
            }      /* END if (bmisset(Flags,mcmVerbose) && !bmisset(Flags,mcmSummary)) */

          MStringAppendF(String,"\n");
          }  /* END if (bmisset(Flags,mcmVerbose)) */
        }    /* END for (tindex) */

      MUFree((char **)&TSort);

      if (ValueFound == FALSE)
        {
        MStringAppendF(String,"No Data\n");
        }
      else if (TrigCompleted == TRUE)
        {
        MStringAppendF(String,"* indicates trigger has completed\n");
        }
      }    /* END BLOCK (default) */

      break;
    }  /* END switch (DFormat) */

  if (DMsg != NULL)
    MUFree(&DMsg);

  return(SUCCESS);
  }  /* END MTrigDiagnose() */
/* END MTrigDiagnose.c */
