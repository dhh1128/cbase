/* HEADER */

/**
 * @file MDAG.c
 *
 * Moab Dependencies
 */

/*  Contains:                                        */
/* int MDAGSetAnd(mdepend_t **,enum MDependEnum,char *,char *); */
/* int MDAGSetOr(mdepend_t **,enum MDependEnum,char *,char *); */
/* int MDAGCopy(mdepend_t **,mdepend_t **); */
/* int MDAGCreate(mdepend_t **,mdepend_t **) */
/* int MDAGDestroy(mdepend_t **); */


#include "moab.h"
#include "moab-proto.h"
#include "moab-global.h"
#include "moab-const.h"



/**
 * Creates a new dependency in the given dependency list. Returns
 * a pointer to the newly created dependency.
 *
 * @see MDAGSetAnd() - parent
 * @see MDAGSetOr() - parent
 * 
 * @param DList (I)
 * @param DP (O) [modified]
 */

int MDAGCreate(

  marray_t   *DList,  /* I */
  mdepend_t **DP)     /* O (modified) */

  {
  mdepend_t *NewD = NULL;

  const char *FName = "MDAGCreate";

  MDB(5,fALL) MLog("%s(DList,DP)\n",
    FName);

  if (DP != NULL)
    {
    *DP = NULL;
    }

  if ((DList == NULL) || (DP == NULL))
    {
    return(FAILURE);
    }

  NewD = (mdepend_t *)MUCalloc(1,sizeof(mdepend_t));

  if (NewD == NULL)
    {
    MDB(1,fALL) MLog("ALERT:    could not allocate new dependency\n");

    return(FAILURE);
    }

  NewD->Satisfied = MBNOTSET;

  if (MUArrayListAppendPtr(DList,NewD) == FAILURE)
    {
    return(FAILURE);
    }

  *DP = NewD;

  NewD->Index = DList->NumItems-1;

  return(SUCCESS);
  }  /* END MDAGCreate() */





/**
 * Adds an "AND" dependency to the given trigger.
 *
 * @param DP (I)
 * @param DType (I)
 * @param Value (I) [optional]
 * @param SValue (I) [optional]
 * @param FlagBM (I)
 */

int MDAGSetAnd(

  marray_t          *DP,       /* I */
  enum MDependEnum   DType,    /* I */
  char              *Value,    /* I (optional) */
  char              *SValue,   /* I (optional) */
  mbitmap_t         *FlagBM)   /* I */

  {
  mdepend_t *tmpD;
  mdepend_t *NewD;

  const char *FName = "MDAGSetAnd";

  MDB(5,fALL) MLog("%s(DP,%s,%s,%s)\n",
    FName,
    MDependType[DType],
    (Value != NULL) ? Value : "NULL",
    (SValue != NULL) ? SValue : "NULL");

  if ((DP == NULL) ||
      (DP->Array == NULL))
    return(FAILURE);

  tmpD = ((mdepend_t **)DP->Array)[0];

  if (MDAGCreate(DP,&NewD) == FAILURE)
    {
    return(FAILURE);
    }

  if (tmpD != NULL)
    {
    while (tmpD->NextAnd != NULL)
      {
      tmpD = tmpD->NextAnd;
      }

    tmpD->NextAnd = NewD;
    }

  NewD->Type = DType;

  MUStrDup(&NewD->SValue,SValue);
  MUStrDup(&NewD->Value,Value);

  bmcopy(&NewD->BM,FlagBM);

  return(SUCCESS);
  }  /* END MDAGSetAnd() */




/**
 * Adds an "OR" dependency to the given trigger.
 *
 * @param DList (I)
 * @param DType (I)
 * @param Value (I) [optional]
 * @param SValue (I) [optional]
 */

int MDAGSetOr(

  marray_t          *DList,
  enum MDependEnum   DType,
  char              *Value,
  char              *SValue)

  {
  mdepend_t *tmpD;
  mdepend_t *NewD;

  const char *FName = "MDAGSetOr";

  MDB(5,fALL) MLog("%s(DList,%s,%s,%s)\n",
    FName,
    MDependType[DType],
    (Value != NULL) ? Value : "NULL",
    (SValue != NULL) ? SValue : "NULL");

  if ((DList == NULL) ||
      (DList->Array == NULL))
    return(FAILURE);

  tmpD = (mdepend_t *)MUArrayListGetPtr(DList,0);

  if (MDAGCreate(DList,&NewD) == FAILURE)
    {
    return(FAILURE);
    }

  if (tmpD != NULL)
    {
    while (tmpD->NextOr != NULL)
      {
      tmpD = tmpD->NextOr;
      }

    tmpD->NextOr = NewD;
    }

  NewD->Type = DType;

  MUStrDup(&NewD->SValue,SValue);
  MUStrDup(&NewD->Value,Value);

  return(SUCCESS);
  }  /* END MDAGSetOr() */




/**
 * Performs deep copy from one dependency
 * array to another.
 *
 * WARNING: Will free DList first!
 *
 * @param DList (I) [modified]
 * @param SList (I)
 */

int MDAGCopy(

  marray_t *DList,
  marray_t *SList)

  {
  mdepend_t *tmpS;

  const char *FName = "MDAGCopy";

  MDB(5,fALL) MLog("%s(DList,SList)\n",
    FName);

  if ((SList == NULL) || (DList == NULL))
    {
    return(FAILURE);
    }

 if (DList->Array != NULL)
    {
    MUArrayListFree(DList);
    }

  if (SList->Array == NULL)
    {
    /* nothing to copy */

    return(SUCCESS);
    }

  MUArrayListCreate(DList,SList->ElementSize,SList->NumItems);

  tmpS = ((mdepend_t **)SList->Array)[0];

  if (tmpS == NULL)
    {
    /* nothing to copy */

    return(SUCCESS);
    }

  while (tmpS != NULL)
    {
    MDAGSetAnd(DList,tmpS->Type,tmpS->Value,tmpS->SValue,&tmpS->BM);

    tmpS = tmpS->NextAnd;
    }

  tmpS = ((mdepend_t **)SList->Array)[0]->NextOr;

  while (tmpS != NULL)
    {
    MDAGSetOr(DList,tmpS->Type,tmpS->Value,NULL);

    tmpS = tmpS->NextOr;
    }

  return(SUCCESS);
  }  /* END MDAGCopy */






/**
 * Frees the memory associated with the given DAG list.
 *
 * @param DList (I) [modified]
 */

int MDAGDestroy(

  marray_t *DList)  /* I (modified) */

  {
  int index;
  mdepend_t **DArray;
  mdepend_t  *tmpD;

  const char *FName = "MDAGDestroy";

  MDB(9,fALL) MLog("%s(DList)\n",
    FName);

  if (DList == NULL)
    {
    return(FAILURE);
    }

  DArray = (mdepend_t **)DList->Array;

  for (index = 0;index < DList->NumItems;index++)
    {
    tmpD = DArray[index];

    MUFree(&tmpD->Value);
    MUFree(&tmpD->SValue);

    MUFree((char **)&DArray[index]);
    }

  MUArrayListFree(DList);

  return(SUCCESS);
  } /* END MDAGDestroy() */





/**
 * This function is now deprecated for JOB triggers! Use MDAGCheckMultiWithHT() instead
 * when dealing with job triggers (job triggers use hashtables for variables, not the
 * mln_t linked list strucutres).
 *
 * @param DHead (I) The beginning of a dependency linked-list.
 * @param EList (I) [array of linked lists]
 * @param EListFlags (I)
 * @param EMsg (O) Needs to be MMAX_BUFFER if Verbose2 set in EMsg Flags! [optional,minsize=MMAX_LINE]
 * @param VCs  (I) [optional] - List of VCs to check (goes up through all parent VCs recursively)
 */

int MDAGCheckMulti(

  mdepend_t   *DHead,      /* I */
  mln_t      **EList,      /* I (array of linked lists) */
  int         *EListFlags, /* I */
  mstring_t   *EMsg,       /* O (optional,minsize=MMAX_LINE) */
  mln_t       *VCs)        /* I (optional) */

  {
  int eindex;

  mbool_t DSatisfied;
  mbool_t Failure;

  mbool_t VarFound = FALSE;

  mdepend_t *tmpD;
  mln_t     *LP;

  mln_t *tmpVCL;
  mvc_t *tmpVC;

  const char *FName = "MDAGCheckMulti";

  MDB(5,fALL) MLog("%s(D,EList,EListFlags,EMsg)\n",
    FName);

  if (EMsg != NULL)
    *EMsg = "";

  if (DHead == NULL)
    {
    return(SUCCESS);
    }

  if ((EList == NULL) || (EListFlags == NULL))
    {
    return(FAILURE);
    }

  tmpD = DHead;

  Failure = FALSE;

  while (tmpD != NULL)
    {
    /* step through each dependency */

    eindex = 0;

    DSatisfied = FALSE;

#if 0
    /* Variable could still be in a VC */

    if ((EList[0] == NULL) && (tmpD->Type == mdVarNotSet))
      {
      MDB(6,fSTRUCT) MLog("INFO:     dependency satisfied ('%s')\n",
        tmpD->Value);

      DSatisfied = TRUE;
      }
#endif

    while (EList[eindex] != NULL)
      {
      /* search all lists to satisfy dependency */

      if ((EListFlags[eindex] == 1) && !bmisset(&tmpD->BM,mdbmExtSearch))
        break;

      switch (tmpD->Type)
        {
        case mdVarNotSet:
 
          /* verify that variable has not been set */
   
          if (MULLCheck(EList[eindex],tmpD->Value,&LP) == FAILURE)
            {
            /* Can't assume that the var has not been set, we haven't checked all of the lists yet */

            break;
            }  /* END if (MULLCheck(EList[eindex],tmpD->Value,&LP) == FAILURE) */
          else
            {
            VarFound = TRUE;
            }
   
          break;
  
        case mdVarEQ:
        case mdVarNE:
        case mdVarGE:
        case mdVarLE:
        case mdVarLT:
        case mdVarGT:

          if ((MULLCheck(EList[eindex],
                ((tmpD->Value[0] != '$') && (tmpD->Value[0] != '+')) ? tmpD->Value : &tmpD->Value[1],
                &LP) == SUCCESS) &&
              (LP->Ptr != NULL))
            {
            int Value    = (int)strtol((char *)LP->Ptr,NULL,10);
            int ReqValue = (int)strtol(tmpD->SValue,NULL,10);

            switch (tmpD->Type)
              {
              case mdVarEQ:

                if (!strcmp((char *)LP->Ptr,tmpD->SValue))
                  DSatisfied = TRUE;

                break;

              case mdVarNE:

                if (strcmp((char *)LP->Ptr,tmpD->SValue))
                  DSatisfied = TRUE;

                break;

              case mdVarGE:

                if (Value >= ReqValue)
                  DSatisfied = TRUE;

                break;

              case mdVarGT:

                if (Value > ReqValue)
                  DSatisfied = TRUE;

                break;

              case mdVarLT:

                if (Value < ReqValue)
                  DSatisfied = TRUE;

                break;

              case mdVarLE:

                if (Value <= ReqValue)
                  DSatisfied = TRUE;

                break;

              default:

                /* NO-OP */

                break;
              }
            }

          break;

        case mdJobStart:                /* job must have previously started */
        case mdJobCompletion:           /* job must have previously completed */
        case mdJobSuccessfulCompletion: /* job must have previosuly completed successfully */
        case mdJobFailedCompletion:
        case mdJobSyncMaster:           /* slave jobs must start at same time */
        case mdJobSyncSlave:            /* must start at same time as master */

          /* how to handle job events here? */

          /* ignore for now */

          DSatisfied = TRUE;

          /* NYI */

          break;
 
        case mdVarSet:
        default:
   
          /* verify that variable has been set */
   
          if (MULLCheck(
                EList[eindex],
                (tmpD->Value[0] != '$') ? tmpD->Value : &tmpD->Value[1],
                &LP) == SUCCESS)
            {
            MDB(6,fSTRUCT) MLog("INFO:     dependency satisfied ('%s')\n",
              tmpD->Value);

            DSatisfied = TRUE;
            }
   
          if ((tmpD->Value[0] == '$') && (LP->Ptr != NULL))
            {
            MDB(6,fSTRUCT) MLog("INFO:     dependency satisfied ('%s')\n",
              tmpD->Value);

            DSatisfied = TRUE;
            }

          break;
        }  /* END switch (tmpD->Type) */

      if (DSatisfied == TRUE)
        {
        if ((tmpD->Type == mdVarNotSet) && 
            (bmisset(&tmpD->BM,mdbmExtSearch)) &&
            (EList[eindex + 1] != NULL))
          {
          /* we need to make sure the variable is not set on anything else, so keep looking */

          DSatisfied = FALSE;
          }
        else
          {
          break;
          }
        }  /* END if if (DSatisfied == TRUE) */

      if ((tmpD->Type == mdVarNotSet) && (VarFound == TRUE))
        break;

      eindex++;
      }    /* END while (EList[eindex] != NULL) */

    if ((DSatisfied != TRUE) && (VarFound == FALSE))
      {
      /* Check VCs */

      for (tmpVCL = VCs;tmpVCL != NULL;tmpVCL = tmpVCL->Next)
        {
        mln_t *tmpResults = NULL;

        tmpVC = (mvc_t *)tmpVCL->Ptr;

        if (MVCSearchForVar(tmpVC,tmpD->Value,&tmpResults,TRUE,FALSE) == SUCCESS)
          {
          MULLFree(&tmpResults,MUFREE);

          switch (tmpD->Type)
            {
            case mdVarNotSet:

              /* Variable was found, dependency failed */

              VarFound = TRUE;

              break;

            case mdVarEQ:
            case mdVarNE:
            case mdVarGE:
            case mdVarLE:
            case mdVarLT:
            case mdVarGT:
            case mdJobStart:                /* job must have previously started */
            case mdJobCompletion:           /* job must have previously completed */
            case mdJobSuccessfulCompletion: /* job must have previosuly completed successfully */
            case mdJobFailedCompletion:
            case mdJobSyncMaster:           /* slave jobs must start at same time */
            case mdJobSyncSlave:            /* must start at same time as master */

              /* Not handled for VCs */

              break;

            case mdVarSet:
            default:

              MDB(6,fSTRUCT) MLog("INFO:     dependency satisfied ('%s')\n",
                tmpD->Value);

              DSatisfied = TRUE;

              break;
            }  /* END switch (tmpD->Type) */
          }  /* END if (MVCSearchForVar() == SUCCESS) */
        }  /* END for (tmpVCL) */
      }  /* END if ((DSatisfied != TRUE) && (VarFound == FALSE)) */

    if ((tmpD->Type == mdVarNotSet) &&
        (VarFound == FALSE))
      {
      MDB(6,fSTRUCT) MLog("INFO:     dependency satisfied ('%s')\n",
        tmpD->Value);

      DSatisfied = TRUE;
      }

    if (DSatisfied == FALSE)
      {
      Failure = TRUE;

      MDB(6,fSTRUCT) MLog("INFO:     dependency not satisfied ('%s')\n",
        tmpD->Value);

      if (EMsg != NULL)
        {
        *EMsg = "cannot satisfy dependency '";
        *EMsg += tmpD->Value;
        *EMsg += "'";
        }

      if (bmisset(&tmpD->BM,mdbmImport))
        {
        MRMInfoQueryRegister(NULL,tmpD->Value,NULL,NULL);
        }
      }  /* END if (DSatisfied == FALSE) */

    if (tmpD->NextAnd != NULL)
      tmpD = tmpD->NextAnd;
    else
      tmpD = tmpD->NextOr;
    }    /* END while (D != NULL) */

  if (Failure == TRUE)
    return(FAILURE);

  return(SUCCESS);
  } /* END MDAGCheckMulti() */



/**
 * Checks trigger dependencies using the hashtable variables implementation
 * instead of the standard linked-list implementation. In the future, this
 * should probably be the *only* dependency check routine--we should phase
 * out MDAGCheckMulti().
 *
 * WARNING: either DAList or DDList must be specified but not both
 *
 * @param DAList (I)
 * @param DDList (I)
 * @param HList (I) [array of hash tables]
 * @param HListFlags (I)
 * @param EMsgFlags (I) Flags that specify how verbose of an EMsg to create [enum MCModeEnum bitmap]
 * @param EMsg (O) Needs to be MMAX_DEPSTRINGSIZE if Verbose2 set in EMsg Flags! [optional,minsize=MMAX_LINE]
 * @param VCs  (I) [optional] - VCs to check recursively for the variable
 */

int MDAGCheckMultiWithHT(

  marray_t    *DAList,
  mdepend_t   *DDList,
  mhash_t    **HList,
  int         *HListFlags,
  mbitmap_t   *EMsgFlags,
  mstring_t   *EMsg,
  mln_t       *VCs)

  {
  int hindex;

  mbool_t DSatisfied;
  mbool_t Failure;
  mbool_t VarNotSet;  /* used for type mdVarNotSet */

  mdepend_t *tmpD;
  char      *tmpVal;

  mln_t     *tmpVCL;
  mvc_t     *tmpVC;

  int        RemainingDeps;

  mhash_t OutstandingDepHT;  /* used to eliminate duplicates in display */

  const char *FName = "MDAGCheckMultiWithHT";

  MDB(5,fALL) MLog("%s(DList,HList,HListFlags,EMsg)\n",
    FName);

  if (EMsg != NULL)
    *EMsg = "";

  if (DAList != NULL)
    tmpD = (mdepend_t *)MUArrayListGetPtr(DAList,0);
  else
    tmpD = DDList;

  Failure = FALSE;
  RemainingDeps = 0;

  MUHTCreate(&OutstandingDepHT,-1);

  while (tmpD != NULL)
    {
    /* step through each dependency */

    hindex = 0;

    DSatisfied = FALSE;
    VarNotSet = TRUE;

    while (HList[hindex] != NULL)
      {
      /* search all lists to satisfy dependency */

      if ((HListFlags[hindex] == 1) && !bmisset(&tmpD->BM,mdbmExtSearch))
        break;

      switch (tmpD->Type)
        {
        case mdVarNotSet:
   
          /* verify that variable has NOT been set */
   
          if (MUHTGet(HList[hindex],tmpD->Value,NULL,NULL) == SUCCESS)
            {
            /* dependency is not met since this variable is defined */

            VarNotSet = FALSE;
            }
   
          break;
  
        case mdVarEQ:
        case mdVarNE:
        case mdVarGE:
        case mdVarLE:
        case mdVarLT:
        case mdVarGT:

          if ((MUHTGet(HList[hindex],
                ((tmpD->Value[0] != '$') && (tmpD->Value[0] != '+')) ? tmpD->Value : &tmpD->Value[1],
                (void **)&tmpVal,
                NULL) == SUCCESS) &&
              (tmpVal != NULL))
            {
            int Value    = (int)strtol(tmpVal,NULL,10);
            int ReqValue = (int)strtol(tmpD->SValue,NULL,10);

            switch (tmpD->Type)
              {
              case mdVarEQ:

                if (!strcmp(tmpVal,tmpD->SValue))
                  DSatisfied = TRUE;

                break;

              case mdVarNE:

                if (strcmp(tmpVal,tmpD->SValue))
                  DSatisfied = TRUE;

                break;

              case mdVarGE:

                if (Value >= ReqValue)
                  DSatisfied = TRUE;

                break;

              case mdVarGT:

                if (Value > ReqValue)
                  DSatisfied = TRUE;

                break;

              case mdVarLT:

                if (Value < ReqValue)
                  DSatisfied = TRUE;

                break;

              case mdVarLE:

                if (Value <= ReqValue)
                  DSatisfied = TRUE;

                break;

              default:

                /* NO-OP */

                break;
              }
            }

          break;

        case mdJobStart:                /* job must have previously started */
        case mdJobCompletion:           /* job must have previously completed */
        case mdJobSuccessfulCompletion: /* job must have previosuly completed successfully */
        case mdJobFailedCompletion:
        case mdJobSyncMaster:           /* slave jobs must start at same time */
        case mdJobSyncSlave:            /* must start at same time as master */

          /* how to handle job events here? */

          /* ignore for now */

          DSatisfied = TRUE;

          /* NYI */

          break;
 
        case mdVarSet:
        default:
   
          /* verify that variable has been set */
   
          if (MUHTGet(
                HList[hindex],
                (tmpD->Value[0] != '$') ? tmpD->Value : &tmpD->Value[1],
                (void **)&tmpVal,
                NULL) == SUCCESS)
            {
            MDB(6,fSTRUCT) MLog("INFO:     dependency satisfied ('%s')\n",
              tmpD->Value);

            DSatisfied = TRUE;
            }
   
          if ((tmpD->Value[0] == '$') && (tmpVal != NULL))
            {
            MDB(6,fSTRUCT) MLog("INFO:     dependency satisfied ('%s')\n",
              tmpD->Value);

            DSatisfied = TRUE;
            }
   
          break;
        }  /* END switch (tmpD->Type) */

      if (DSatisfied == TRUE)
        break;

      if ((tmpD->Type == mdVarNotSet) &&
          (VarNotSet == FALSE))
        {
        /* we can shortcircuit since we already found the variable set */

        break;
        }
   
      hindex++;
      }    /* END while (HList[eindex] != NULL) */

    if (DSatisfied == FALSE)
      {
      /* Check VCs */

      for (tmpVCL = VCs;tmpVCL != NULL;tmpVCL = tmpVCL->Next)
        {
        mln_t *tmpResults = NULL;

        tmpVC = (mvc_t *)tmpVCL->Ptr;

        if (MVCSearchForVar(tmpVC,tmpD->Value,&tmpResults,TRUE,FALSE) == SUCCESS)
          {
          MULLFree(&tmpResults,MUFREE);

          switch (tmpD->Type)
            {
            case mdVarNotSet:

              /* Variable was found, dependency failed */

              VarNotSet = FALSE;

              break;

            case mdVarEQ:
            case mdVarNE:
            case mdVarGE:
            case mdVarLE:
            case mdVarLT:
            case mdVarGT:
            case mdJobStart:                /* job must have previously started */
            case mdJobCompletion:           /* job must have previously completed */
            case mdJobSuccessfulCompletion: /* job must have previosuly completed successfully */
            case mdJobFailedCompletion:
            case mdJobSyncMaster:           /* slave jobs must start at same time */
            case mdJobSyncSlave:            /* must start at same time as master */

              /* Not handled for VCs */

              break;

            case mdVarSet:
            default:

              MDB(6,fSTRUCT) MLog("INFO:     dependency satisfied ('%s')\n",
                tmpD->Value);

              DSatisfied = TRUE;

              break;
            }  /* END switch (tmpD->Type) */
          }  /* END if (MVCSearchForVar() == SUCCESS) */
        }  /* END for (tmpVCL) */
      }  /* END if (DSatisfied == FALSE) */

    if ((tmpD->Type == mdVarNotSet) &&
        (VarNotSet == TRUE))
      {
      /* dependency IS satisfied for VarNotSet deps ONLY if 
       * the variable is not set! */

      DSatisfied = TRUE;
      }

    if (DSatisfied == FALSE)
      {
      Failure = TRUE;

      MDB(6,fSTRUCT) MLog("INFO:     dependency not satisfied ('%s')\n",
        tmpD->Value);

      /* only display extra dependency failures if we are in verbose mode */

      if ((RemainingDeps < 1) || bmisset(EMsgFlags,mcmVerbose2))
        {
        RemainingDeps++;

        /* Store so that all unfulfilled dependencies can be listed */

        MUHTAdd(&OutstandingDepHT,tmpD->Value,NULL,NULL,NULL);
        }

      if (bmisset(&tmpD->BM,mdbmImport))
        {
        MRMInfoQueryRegister(NULL,tmpD->Value,NULL,NULL);
        }
      }  /* END if (DSatisfied == FALSE) */

    if (tmpD->NextAnd != NULL)
      tmpD = tmpD->NextAnd;
    else
      tmpD = tmpD->NextOr;
    }    /* END while (tmpD != NULL) */

  /* List unfinished dependencies */

  if ((RemainingDeps > 0) && (EMsg != NULL))
    {
    mhashiter_t HTIter;
    char *DepName = NULL;
    char *BPtr;
    int   BSpace;
    char  tmpBuf[MMAX_BUFFER];

    MUHTIterInit(&HTIter);
    MUSNInit(&BPtr,&BSpace,tmpBuf,sizeof(tmpBuf));

    while (MUHTIterate(&OutstandingDepHT,&DepName,NULL,NULL,&HTIter) == SUCCESS)
      {
      if (tmpBuf[0] != '\0')
        MUSNPrintF(&BPtr,&BSpace,",'%s'",DepName);
      else
        MUSNPrintF(&BPtr,&BSpace,"'%s'",DepName);
      }

    /* Determine singular or plural output string */
    if (RemainingDeps == 1)
      {
      *EMsg = "cannot satisfy dependency ";
      }
    else
      {
      *EMsg = "cannot satisfy dependencies ";
      }

    *EMsg += tmpBuf;
    }

  MUHTFree(&OutstandingDepHT,FALSE,NULL);

  if (Failure == TRUE)
    return(FAILURE);

  return(SUCCESS);
  } /* END MDAGCheckMultiWithHT() */



/**
 * Converts the given list of dependencies to a string format.
 *
 * @param D (I)
 * @param Buf (I) [modified]
 * @param BufSize (I)
 */

int MDAGToString(

  marray_t   *D,       /* I */
  char       *Buf,     /* I (modified) */
  int         BufSize) /* I */

  {
  int index;

  char *BPtr = NULL;
  int   BSpace = 0;

  char tmpLine[MMAX_LINE];

  mdepend_t **DArray = NULL;

  const char *FName = "MDAGToString";

  MDB(5,fALL) MLog("%s(D,Buf,BufSize)\n",
    FName);

  if (D == NULL)
    {
    return(SUCCESS);
    }

  DArray = (mdepend_t **)D->Array;

  MUSNInit(&BPtr,&BSpace,Buf,BufSize);

  /* FORMAT:  <NAME>[,<VALUE>][[|<or>]...][[&<and>]...].[<NAME>...] */

  for (index = 0;index < D->NumItems;index++)
    {
    if (DArray[index] == NULL)
      break;

    if (index > 0)
      MUSNCat(&BPtr,&BSpace,".");

    if ((DArray[index]->Type == mdVarNotSet) &&
        (DArray[index]->Value[0] != '!'))
      {
      snprintf(tmpLine,sizeof(tmpLine),"!%s%s%s",
        (bmisset(&DArray[index]->BM,mdbmImport)) ? "*" : "",
        (bmisset(&DArray[index]->BM,mdbmExtSearch)) ? "^" : "",
        DArray[index]->Value);

      MUSNCat(&BPtr,&BSpace,tmpLine);
      }
    else
      {
      MUSNCat(&BPtr,&BSpace,DArray[index]->Value);
      }

    if (DArray[index]->SValue != NULL)
      {
      snprintf(tmpLine,sizeof(tmpLine),"%s%s:%s%s%s",
        (DArray[index]->Type != mdNONE) ? ":" : "",
        (DArray[index]->Type != mdNONE) ? MDependType[DArray[index]->Type] : "",
        (bmisset(&DArray[index]->BM,mdbmImport)) ? "*" : "",
        (bmisset(&DArray[index]->BM,mdbmExtSearch)) ? "^" : "",
        DArray[index]->SValue);

      MUSNCat(&BPtr,&BSpace,tmpLine);
      }
    }  /* END for (index) */

  return(SUCCESS);
  }  /* END MDAGToString() */

/* END MDAG.c */

