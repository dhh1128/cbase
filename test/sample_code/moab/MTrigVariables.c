/* HEADER */

      
/**
 * @file MTrigVariables.c
 *
 * Contains: Trigger Variable functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/* defines */

#ifdef MYAHOO
#  define MDEF_TRIGVARVAL "NULL"
#else
#  define MDEF_TRIGVARVAL "TRUE"
#endif /* MYAHOO */



/**
 * Adds a variable to the trigger's appropriate scope. This
 * scope is determined by the object the trigger is bound to.
 *
 * @param T (I)
 * @param SVarName (I)
 * @param FlagBM (I)
 * @param VarValue (I) [optional]
 * @param AddGlobal (I)
 */

int MTrigAddVariable(

  mtrig_t    *T,
  char       *SVarName,
  mbitmap_t  *FlagBM,
  const char *VarValue,
  mbool_t     AddGlobal)

  {
  mln_t *tmpL;

  char  tmpLine[MMAX_LINE];
  char *ptr;
  int   tmpI;

  char *VarName;
  char *VarTok;

  mbool_t DoIncr   = FALSE;
  mbool_t DoExport = FALSE;

  if (SVarName == NULL)
    {
    return(FAILURE);
    }

  if ((strchr(SVarName,'+') != NULL) || (bmisset(FlagBM,mtvaIncr)))
    {
    DoIncr = TRUE;
    }

  if ((strchr(SVarName,'^') != NULL) || (bmisset(FlagBM,mtvaExport)))
    {
    DoExport = TRUE;
    }

  VarName = MUStrTok(SVarName,"+^ \n\t",&VarTok);

  if ((VarName == NULL) || (VarName[0] == '\0'))
    {
    return(SUCCESS);
    }

  switch (T->OType)
    {
    case mxoRsv:

      {
      mrsv_t *R;

      R = (mrsv_t *)T->O;

      if (R == NULL)
        {
        return(FAILURE);
        }

      MULLAdd(&R->Variables,VarName,NULL,&tmpL,MUFREE);
  
      if (DoExport == TRUE)
        bmset(&tmpL->BM,mtvaExport);

      if (DoIncr == TRUE)
        {
        if (tmpL->Ptr != NULL)
          {
          ptr = (char *)tmpL->Ptr;

          tmpI = (int)strtol(ptr,NULL,10);

          tmpI++;
          }
        else
          {
          tmpI = 1;
          }

        snprintf(tmpLine,sizeof(tmpLine),"%d",tmpI);

        MUStrDup((char **)&tmpL->Ptr,tmpLine);

        if (DoExport == TRUE)
          {
          if ((R->RsvGroup != NULL) && (MRsvFind(R->RsvGroup,&R,mraNONE) == SUCCESS))
            {
            MULLAdd(&R->Variables,VarName,NULL,&tmpL,MUFREE);

            if (tmpL->Ptr != NULL)
              {
              ptr = (char *)tmpL->Ptr;

              tmpI = (int)strtol(ptr,NULL,10);

              tmpI++;
              }
            else
              {
              tmpI = 1;
              }

            snprintf(tmpLine,sizeof(tmpLine),"%d",tmpI);

            MUStrDup((char **)&tmpL->Ptr,tmpLine);

            if (DoExport == TRUE)
              bmset(&tmpL->BM,mtvaExport);
            }  /* END if ((R->RsvGroup != NULL) ...) */

          if (R->ParentVCs != NULL)
            {
            mvc_t *tmpVC;
            mln_t *tmpL;

            for (tmpL = R->ParentVCs;tmpL != NULL;tmpL = tmpL->Next)
              {
              tmpVC = (mvc_t *)tmpL->Ptr;

              MVCAddVarRecursive(tmpVC,VarName,NULL,DoExport,DoIncr,TRUE);
              }
            }  /* END if (R->ParentVCs != NULL) */
          }  /* END if (DoExport == TRUE) */
        }
      else
        {
        if (VarValue != NULL)
          MUStrDup((char **)&tmpL->Ptr,VarValue);

        if (DoExport == TRUE)
          {
          if ((R->RsvGroup != NULL) && (MRsvFind(R->RsvGroup,&R,mraNONE) == SUCCESS))
            {
            MULLAdd(&R->Variables,VarName,NULL,&tmpL,MUFREE);

            if (VarValue != NULL)
              MUStrDup((char **)&tmpL->Ptr,VarValue);

            if (DoExport == TRUE)
              bmset(&tmpL->BM,mtvaExport);
            }

          if (R->ParentVCs != NULL)
            {
            mvc_t *tmpVC;
            mln_t *tmpL;

            for (tmpL = R->ParentVCs;tmpL != NULL;tmpL = tmpL->Next)
              {
              tmpVC = (mvc_t *)tmpL->Ptr;

              MVCAddVarRecursive(tmpVC,VarName,VarValue,DoExport,DoIncr,TRUE);
              }
            }
          }
        }
      }  /* END case mxoRsv */

      break;

    case mxoJob:

      {
      mjob_t *J;
      mjob_t *tmpJ = NULL;
      mbitmap_t tmpBM;
      char *tmpVal = NULL;

      J = (mjob_t *)T->O;

      if (J == NULL)
        {
        return(FAILURE);
        }

      if (MUHTGet(&J->Variables,VarName,(void **)&tmpVal,&tmpBM) == FAILURE)
        {
        MUHTAdd(&J->Variables,VarName,NULL,&tmpBM,NULL);
        MJobWriteVarStats(J,mSet,VarName,NULL,&T->ExitCode);
        }

      if (DoExport == TRUE)
        bmset(&tmpBM,mtvaExport);

      if (DoIncr == TRUE)
        {
        if (tmpVal != NULL)
          {
          ptr = (char *)tmpVal;

          tmpI = (int)strtol(ptr,NULL,10);

          tmpI++;
          }
        else
          {
          tmpI = 1;
          }

        snprintf(tmpLine,sizeof(tmpLine),"%d",tmpI);

        MUHTAdd(&J->Variables,VarName,strdup(tmpLine),&tmpBM,MUFREE);
        MJobWriteVarStats(J,mSet,VarName,tmpLine,&T->ExitCode);

        if (DoExport == TRUE)
          {
          if (J->JGroup != NULL)
            {
            if (MJobFind(J->JGroup->Name,&tmpJ,mjsmExtended) == SUCCESS)
              {
              MUHTGet(&tmpJ->Variables,VarName,(void **)&tmpVal,&tmpBM);

              if (DoExport == TRUE)
                bmset(&tmpBM,mtvaExport);

              if (tmpVal != NULL)
                {
                ptr = (char *)tmpVal;

                tmpI = (int)strtol(ptr,NULL,10);

                tmpI++;
                }
              else
                {
                tmpI = 1;
                }
              
              sprintf(tmpLine,"%d",
                tmpI);

              MUHTAdd(&tmpJ->Variables,VarName,strdup(tmpLine),&tmpBM,MUFREE);
              MJobWriteVarStats(tmpJ,mSet,VarName,tmpLine,&T->ExitCode);
              }
            }

          if (J->ParentVCs != NULL)
            {
            mvc_t *tmpVC;
            mln_t *tmpL;

            for (tmpL = J->ParentVCs;tmpL != NULL;tmpL = tmpL->Next)
              {
              tmpVC = (mvc_t *)tmpL->Ptr;

              MVCAddVarRecursive(tmpVC,VarName,NULL,DoExport,DoIncr,TRUE);
              }
            }    /* END if (J->ParentVCs != NULL) */
          }    /* END if (DoExport == TRUE) */
        }    /* END if (DoIncr == TRUE) */
      else
        {
        if (VarValue != NULL)
          {
          MUHTAdd(&J->Variables,VarName,strdup(VarValue),&tmpBM,MUFREE);
          MJobWriteVarStats(J,mSet,VarName,VarValue,&T->ExitCode);
          }

        if (DoExport == TRUE)
          {
          if (J->JGroup != NULL)
            {
            if (MJobFind(J->JGroup->Name,&tmpJ,mjsmExtended) == SUCCESS)
              {
              bmclear(&tmpBM);

              if (DoExport == TRUE)
                bmset(&tmpBM,mtvaExport);

              if (VarValue != NULL)
                {
                MUHTAdd(&tmpJ->Variables,VarName,strdup(VarValue),&tmpBM,MUFREE);
                MJobWriteVarStats(tmpJ,mSet,VarName,VarValue,&T->ExitCode);
                }
              }
            }

          if (J->ParentVCs != NULL)
            {
            mvc_t *tmpVC;
            mln_t *tmpL;

            for (tmpL = J->ParentVCs;tmpL != NULL;tmpL = tmpL->Next)
              {
              tmpVC = (mvc_t *)tmpL->Ptr;

              MVCAddVarRecursive(tmpVC,VarName,VarValue,DoExport,DoIncr,TRUE);
              }
            }  /* END if (J->ParentVCs != NULL) */
          }  /* END if (DoExport == TRUE) */
        }  /* END else (DoIncr == TRUE) */

      /* checkpoint jobs to ensure we don't lose newly updated variables in a crash */

      /* NOTE: MUST MOVE THIS CHECKPOINT SOMEPLACE ELSE--too resource intensive here!
      MOCheckpoint(mxoJob,(void *)J,FALSE,NULL);
      MOCheckpoint(mxoJob,(void *)tmpJ,FALSE,NULL);
      */
      }  /* END BLOCK (case mxoJob) */

      break;

    case mxoNode:

      {
      mbitmap_t tmpBM;

      mnode_t *N;

      N = (mnode_t *)T->O;

      if (N == NULL)
        {
        return(FAILURE);
        }

      if (DoExport == TRUE)
        bmset(&tmpBM,mtvaExport);

      if (DoIncr == TRUE)
        {
        char *Ptr = NULL;

        if (MUHTGet(&N->Variables,VarName,(void **)&Ptr,NULL) == SUCCESS)
          {
          tmpI = (int)strtol(Ptr,NULL,10);

          tmpI++;
          }
        else
          {
          tmpI = 1;
          }

        snprintf(tmpLine,sizeof(tmpLine),"%d",tmpI);

        MUHTAdd(&N->Variables,VarName,strdup(tmpLine),&tmpBM,MUFREE);
        }
      else if (VarValue != NULL)
        {
        MUHTAdd(&N->Variables,VarName,strdup(VarValue),&tmpBM,MUFREE);
        }
      }  /* END case mxoRM */

      break;

    case mxoRM:

      {
      mrm_t *R;

      R = (mrm_t *)T->O;

      if (R == NULL)
        {
        return(FAILURE);
        }

      MULLAdd(&R->Variables,VarName,NULL,&tmpL,MUFREE);

      if (DoExport == TRUE)
        bmset(&tmpL->BM,mtvaExport);

      if (DoIncr == TRUE)
        {
        if (tmpL->Ptr != NULL)
          {
          ptr = (char *)tmpL->Ptr;

          tmpI = (int)strtol(ptr,NULL,10);

          tmpI++;
          }
        else
          {
          tmpI = 1;
          }

        snprintf(tmpLine,sizeof(tmpLine),"%d",tmpI);

        MUStrDup((char **)&tmpL->Ptr,tmpLine);
        }
      else if (VarValue != NULL)
        {
        MUStrDup((char **)&tmpL->Ptr,VarValue);
        }
      }  /* END case mxoRM */

      break;

    case mxoQOS:

      {
      mqos_t *Q;

      Q = (mqos_t *)T->O;

      if (Q == NULL)
        {
        return(FAILURE);
        }

      MULLAdd(&Q->Variables,VarName,NULL,&tmpL,MUFREE);

      if (DoExport == TRUE)
        bmset(&tmpL->BM,mtvaExport);

      if (DoIncr == TRUE)
        {
        if (tmpL->Ptr != NULL)
          {
          ptr = (char *)tmpL->Ptr;

          tmpI = (int)strtol(ptr,NULL,10);

          tmpI++;
          }
        else
          {
          tmpI = 1;
          }

        snprintf(tmpLine,sizeof(tmpLine),"%d",tmpI);

        MUStrDup((char **)&tmpL->Ptr,tmpLine);
        }
      else if (VarValue != NULL)
        {
        MUStrDup((char **)&tmpL->Ptr,VarValue);
        }
      }  /* END case mxoQOS */

      break;

    case mxoClass:

      {
      mclass_t *C;

      C = (mclass_t *)T->O;

      if (C == NULL)
        {
        return(FAILURE);
        }

      MULLAdd(&C->Variables,VarName,NULL,&tmpL,MUFREE);

      if (DoExport)
        bmset(&tmpL->BM,mtvaExport);

      if (bmisset(FlagBM,mtvaIncr))
        {
        if (tmpL->Ptr != NULL)
          {
          ptr = (char *)tmpL->Ptr;

          tmpI = (int)strtol(ptr,NULL,10);

          tmpI++;
          }
        else
          {
          tmpI = 1;
          }

        snprintf(tmpLine,sizeof(tmpLine),"%d",tmpI);

        MUStrDup((char **)&tmpL->Ptr,tmpLine);
        }
      else if (VarValue != NULL)
        {
        MUStrDup((char **)&tmpL->Ptr,VarValue);
        }
      }  /* END case mxoClass */

      break;

    case mxoGroup:
    case mxoUser:
    case mxoAcct:

      {
      mgcred_t *C;

      C = (mgcred_t *)T->O;

      if (C == NULL)
        {
        return(FAILURE);
        }

      MULLAdd(&C->Variables,VarName,NULL,&tmpL,MUFREE);

      if (DoExport == TRUE)
        bmset(&tmpL->BM,mtvaExport);

      if (DoIncr == TRUE)
        {
        if (tmpL->Ptr != NULL)
          {
          ptr = (char *)tmpL->Ptr;

          tmpI = (int)strtol(ptr,NULL,10);

          tmpI++;
          }
        else
          {
          tmpI = 1;
          }

        snprintf(tmpLine,sizeof(tmpLine),"%d",tmpI);

        MUStrDup((char **)&tmpL->Ptr,tmpLine);
        }
      else if (VarValue != NULL)
        {
        MUStrDup((char **)&tmpL->Ptr,VarValue);
        }
      }  /* END case mgcred_t */

      break;

    default:

      /* NYI */

      break;
    }  /* END switch (T->OType) */

  return(SUCCESS);
  }  /* END MTrigAddVariable() */




/**
 * Remove a variable from the given trigger's scope.
 *
 * @param T (I) [modified]
 * @param VarName (I)
 * @param RemoveGlobal (I)
 */

int MTrigRemoveVariable(

  mtrig_t *T,            /* I (modified) */
  char    *VarName,      /* I */
  mbool_t  RemoveGlobal) /* I */

  {
  switch (T->OType)
    {
    case mxoRsv:
      {
      mrsv_t *R;

      R = (mrsv_t *)T->O;

      if (R == NULL)
        {
        return(FAILURE);
        }

      MULLRemove(&R->Variables,VarName,MUFREE);

      if ((RemoveGlobal == TRUE) && 
          ((R->RsvGroup != NULL) && 
           (MRsvFind(R->RsvGroup,&R,mraNONE) == SUCCESS)))
        {
        MULLRemove(&R->Variables,VarName,MUFREE);
        }
      }  /* END case mxoRsv */

      break;

    case mxoJob:
      {
      mjob_t *J;
      mjob_t *tmpJ = NULL;

      J = (mjob_t *)T->O;

      if (J == NULL)
        {
        return(FAILURE);
        }

      MUHTRemove(&J->Variables,VarName,MUFREE);
      MJobWriteVarStats(J,mUnset,VarName,NULL,NULL);

      /* in the future we may altogether phase out the use of J->Variables for triggers */

      if ((RemoveGlobal == TRUE) && (J->JGroup != NULL))
        {
        if (MJobFind(J->JGroup->Name,&tmpJ,mjsmExtended) == SUCCESS)
          {
          MUHTRemove(&tmpJ->Variables,VarName,MUFREE);
          MJobWriteVarStats(tmpJ,mUnset,VarName,NULL,NULL);
          }
        }

      /* checkpoint jobs to ensure we don't lose newly updated variables in a crash */

      /* NOTE: MUST MOVE THIS CHECKPOINT SOMEPLACE ELSE!
      MOCheckpoint(mxoJob,(void *)J,FALSE,NULL);
      MOCheckpoint(mxoJob,(void *)tmpJ,FALSE,NULL);
      */
      }  /* END BLOCK (case mxoJob) */

      break;

    case mxoRM:

      {
      mrm_t *R;

      R = (mrm_t *)T->O;

      if (R == NULL)
        {
        return(FAILURE);
        }

      MULLRemove(&R->Variables,VarName,MUFREE);
      }  /* END case mxoRM */

      break;

    case mxoNode:

      {
      mnode_t *N;

      N = (mnode_t *)T->O;

      if (N == NULL)
        {
        return(FAILURE);
        }

      MUHTRemove(&N->Variables,VarName,MUFREE);
      }  /* END case mxoNode */

      break;

    case mxoQOS:

      {
      mqos_t *Q;

      Q = (mqos_t *)T->O;

      if (Q == NULL)
        {
        return(FAILURE);
        }

      MULLRemove(&Q->Variables,VarName,MUFREE);
      }  /* END case mxoQOS */

      break;

    case mxoClass:

      {
      mclass_t *C;

      C = (mclass_t *)T->O;

      if (C == NULL)
        {
        return(FAILURE);
        }

      MULLRemove(&C->Variables,VarName,MUFREE);
      }  /* END case mxoClass */

      break;

    case mxoUser:
    case mxoGroup:
    case mxoAcct:

      {
      mgcred_t *C;

      C = (mgcred_t *)T->O;

      if (C == NULL)
        {
        return(FAILURE);
        }

      MULLRemove(&C->Variables,VarName,MUFREE);
      }  /* END case mgcred_t */

      break;

    default:

      /* NYI */

      break;
    }  /* END switch (T->OType) */

  return(SUCCESS);
  }  /* END MTrigRemoveVariable() */



/**
 * Run only on completed triggers. After the trigger completes, this
 * function reads the trigger's output buffer and searches for variable
 * assignments.
 *
 * @param T (I) [modified]
 * @param TrigFailed (I) Specifies whether the trigger failed or was successful.
 * @param VName (I) [optional]
 */

int MTrigSetVars(

  mtrig_t *T,              /* I (modified) */
  mbool_t  TrigFailed,     /* I */
  char    *VName)          /* I (optional) */ 

  {
  /*int SIndex;*/

  char *ptr;

  int   index;

  char  tmpLine[MMAX_LINE];
  char  tmpName[MMAX_LINE];

  char *tmpSetsVar = NULL;

  int SetsIndex;
  marray_t *Sets;
  int *SetsFlags;

  char *OBuf;  /* alloc'd */

  const char *FName = "MTrigSetVars";

  MDB(4,fSCHED) MLog("%s(%s,%s)\n",
    FName,
    ((T != NULL) && (T->TName != NULL)) ? T->TName : "NULL",
    (VName != NULL) ? VName : "NULL");

  if (T == NULL)
    {
    return(FAILURE);
    }

  if (TrigFailed == TRUE)
    {
    MTrigInitSets(&T->FSets,FALSE);
    MTrigInitSets(&T->FSetsFlags,TRUE);

    Sets = T->FSets;
    SetsFlags = (int *)T->FSetsFlags->Array;
    }
  else
    {
    MTrigInitSets(&T->SSets,FALSE);
    MTrigInitSets(&T->SSetsFlags,TRUE);

    Sets = T->SSets;
    SetsFlags = (int *)T->SSetsFlags->Array;
    }

  if (Sets == NULL)
    {
    return(FAILURE);
    }

  if ((OBuf = MFULoadNoCache(T->OBuf,1,NULL,NULL,NULL,NULL)) == NULL)
    {
    MDB(2,fSCHED) MLog("INFO:     cannot load output data for trigger %s (File: %s)\n",
      T->TName,
      (T->OBuf == NULL) ? "NULL" : T->OBuf);

    OBuf = (char *)MUMalloc(sizeof(char));

    OBuf[0] = '\0';
    }

  /* NOTE:  should search entire buffer to locate needed variables (NYI)
            (currently only check validity of first potential match) */

  SetsIndex = 0;

  while ((tmpSetsVar = (char *)MUArrayListGetPtr(Sets,SetsIndex++)) != NULL)
    {
    mbitmap_t SetsFlagsBM;

    if (MOLDISSET(SetsFlags[SetsIndex-1],mtvaIncr))
      bmset(&SetsFlagsBM,mtvaIncr);
    if (MOLDISSET(SetsFlags[SetsIndex-1],mtvaExport))
      bmset(&SetsFlagsBM,mtvaExport);

    tmpLine[0] = '\0';

    /* FORMAT:  {HEAD|'\n'}<VARIABLE>[<WS>]=[<WS>]<VAL>'\n' */

    /* ignore '$' marker in variable name */

    if (tmpSetsVar[0] == '$')
      MUStrCpy(tmpName,&tmpSetsVar[1],sizeof(tmpName));
    else
      MUStrCpy(tmpName,&tmpSetsVar[0],sizeof(tmpName));

    if ((ptr = strstr(OBuf,tmpName)) == NULL)
      {
      MDB(6,fSCHED) MLog("INFO:     cannot locate 'sets' variable %s for trigger %s (Buffer: '%s') setting to %s\n",
        tmpSetsVar,
        T->TName,
        OBuf,
        MDEF_TRIGVARVAL);

      MTrigAddVariable(T,tmpSetsVar,&SetsFlagsBM,MDEF_TRIGVARVAL,TRUE);

      continue;
      }

    if ((ptr > OBuf) && (*(ptr - 1) != '\n') && (*(ptr - 1) != '$'))
      {
      MDB(6,fSCHED) MLog("INFO:     cannot load 'sets' variable %s for trigger %s (Buffer: %s) setting to %s\n",
        tmpSetsVar,
        T->TName,
        OBuf,
        MDEF_TRIGVARVAL);

      MTrigAddVariable(T,tmpSetsVar,&SetsFlagsBM,MDEF_TRIGVARVAL,TRUE);

      continue;
      }

    /* FORMAT:  [$]<VARNAME>[<WS>]=[<WS>]<VAL> */

    if (ptr[0] == '$')
      ptr++;

    ptr += strlen(tmpName);

    /* skip whitespace */

    while (isspace(*ptr) && (*ptr != '\0'))
      ptr++;

    if (*ptr != '=')
      {
      MTrigAddVariable(T,tmpSetsVar,&SetsFlagsBM,MDEF_TRIGVARVAL,TRUE);

      MDB(6,fSCHED) MLog("INFO:     cannot assign value to 'sets' variable %s for trigger %s (Buffer: %s)\n",
        tmpSetsVar,
        T->TName,
        OBuf);

      continue;
      }

    /* skip assignment '=' */

    ptr++;

    /* skip whitespace */

    while (isspace(*ptr) && (*ptr != '\0'))
      ptr++;

    /* load value (copy to end of line or end of buffer) */

    index = 0;

    while (index < MMAX_LINE - 1)
      {
      if ((ptr[index] == '\n') || (ptr[index] == '\0'))
        break;

      tmpLine[index] = ptr[index];

      index++;
      }

    if (index == 0)
      {
      MTrigAddVariable(T,tmpSetsVar,&SetsFlagsBM,MDEF_TRIGVARVAL,TRUE);

      MDB(6,fSCHED) MLog("INFO:     cannot parse value for 'sets' variable %s for trigger %s (Buffer: %s)\n",
        tmpSetsVar,
        T->TName,
        OBuf);

      continue;
      }

    tmpLine[index] = '\0';

    MDB(6,fSCHED) MLog("INFO:     adding value to 'sets' variable %s for trigger %s (Value: %s)\n",
      tmpSetsVar,
      T->TName,
      tmpLine);

    MTrigAddVariable(T,tmpSetsVar,&SetsFlagsBM,tmpLine,TRUE);
    }  /* END while((tmpSetsVar = (char *)MUArrayListGetPtr())) */


  /* check for special POSTACTION code */
  /* FORMAT: POSTACTION=<internal action> */

  if ((ptr = strstr(OBuf,"POSTACTION")) != NULL)
    {
    tmpLine[0] = '\0';

    ptr += strlen("POSTACTION");

    if (*ptr == '=')
      {
      char *TokPtr;

      ptr++;  /* skip past = */

      ptr = MUStrTok(ptr,"\n",&TokPtr);

      if (ptr != NULL)
        {
        /* post action successfully loaded--add to global list */

        MUStrDup(&T->PostAction,ptr);

        MUHTAdd(&MTPostActions,T->TName,T->PostAction,NULL,NULL);  /* we aren't checkpointing these...how do we recorver them in crash!? */
        }
      }
    }

  MUFree(&OBuf);

  return(SUCCESS);
  }  /* END MTrigSetVars() */


/**
 * Run after successful trigger completion. Unsets all of the applicable
 * trigger variables.
 *
 * @param T (I)
 * @param TrigFailed (I) Specified whether the trigger failed or not.
 * @param VName (I)
 */

int MTrigUnsetVars(
 
  mtrig_t *T,          /* I */
  mbool_t  TrigFailed, /* I */
  char    *VName)      /* I */

  {
  int SIndex = 0;

  char tmpName[MMAX_LINE];

  marray_t *Unsets;

  char *tmpVar = NULL;

  if (TrigFailed == FALSE)
    {
    Unsets = T->Unsets;
    }
  else
    {
    Unsets = T->FUnsets;
    }

  if (Unsets == NULL)
    {
    return(FAILURE);
    }

  while ((tmpVar = (char *)MUArrayListGetPtr(Unsets,SIndex++)) != NULL)
    {
    /* FORMAT:  {HEAD|'\n'}<VARIABLE>[<WS>]=[<WS>]<VAL>'\n' */

    /* ignore '$' marker in variable name */

    if (tmpVar[0] == '$')
      MUStrCpy(tmpName,&tmpVar[1],sizeof(tmpName));
    else
      MUStrCpy(tmpName,&tmpVar[0],sizeof(tmpName));

    MTrigRemoveVariable(T,tmpName,TRUE);
    }  /* END for (SIndex) */

  return(SUCCESS); 
  }  /* END MTrigUnsetVars() */


/* END MTrigVariables.c */
