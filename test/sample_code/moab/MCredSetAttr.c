/* HEADER */

      
/**
 * @file MCredSetAttr.c
 *
 * Contains: MCredSetAttr
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 * Parse and load trigger attributes for credentials.
 *
 * @param O (I) [modified]
 * @param OIndex (I)
 * @param Value (I)
 * @param L (I)
 */

int MCredSetTrigAttr(

  void                *O,      /* I (modified) */
  enum MXMLOTypeEnum   OIndex, /* I */
  char                *Value,  /* I */
  mcredl_t            *L)      /* I */
 
  {
  int tindex;
  mtrig_t *T;
  marray_t *aTList;
  char *credName;
  char EMsg[MMAX_LINE];
 
  switch (OIndex)
    {
    case mxoUser:
    case mxoGroup:
    case mxoAcct:
      {
      mgcred_t *C = (mgcred_t *)O;
      aTList = C->TList;
      credName = C->Name;
      }
      break;
 
    case mxoQOS:
      {
      mqos_t  *Q = (mqos_t *)O;
      aTList = Q->TList;
      credName = Q->Name;
      }
      break;
 
    case mxoClass:
      {
      mclass_t *C = (mclass_t *)O;
      aTList = C->TList;
      credName = C->Name;
      }
      break;
 
    default:
 
      /* NOT HANDLED */
 
      return(FAILURE);
 
      /*NOTREACHED*/
 
      break;
    }
 
  if (MTrigLoadString(
      &aTList,
      Value,
      TRUE,
      FALSE,
      OIndex,
      credName,
      NULL,
      EMsg) == FAILURE)
    {
    MDB(1,fCONFIG) MLog("ALERT:     Cannot load trigger '%s' for %s %s (%s)\n",
      Value, 
      MXO[OIndex],
      credName,
      EMsg);
 
    return(FAILURE);
    }
 
  for (tindex = 0;tindex < aTList->NumItems;tindex++)
    {
    T = (mtrig_t *)MUArrayListGetPtr(aTList, tindex);
 
    if (MTrigIsValid(T) == FALSE)
      continue;
 
    if ((T->ThresholdType == mtttBacklog) &&
        (L->IdlePolicy == NULL) && (MPUCreate(&L->IdlePolicy) == FAILURE))
      {
      break;
      }
 
    if (MTrigInitialize(T) == FAILURE)
      {
      return(FAILURE);
      }
    }   /* END for (tindex) */
 
  MOReportEvent(O,credName,OIndex,mttStart,MSched.Time,TRUE);
 
  return(SUCCESS);
 
  }  /* END for MCredSetTrigAttr */



/**
 * Set specified credential attribute.
 *
 * @param O         (I) [modified]
 * @param OIndex    (I)
 * @param AIndex    (I) 
 * @param P         (I) [optional] 
 * @param Value     (I)
 * @param ArrayLine (I) [optional]
 * @param Format    (I)
 * @param Mode      (I) [mSet/mAdd/mClear]
 */

int MCredSetAttr(

  void                   *O,
  enum MXMLOTypeEnum      OIndex,
  enum MCredAttrEnum      AIndex,
  mpar_t                 *P,
  void                  **Value,
  char                   *ArrayLine,
  enum MDataFormatEnum    Format,
  enum MObjectSetModeEnum Mode)
 
  {
  mcredl_t *L;
  mfs_t    *F;
  int       pindex = 0;

  const char *FName = "MCredSetAttr";

  MDB(4,fSTRUCT) MLog("%s(O,%s,%s,Value,%d,%d)\n",
    FName,
    MXO[OIndex],
    MCredAttr[AIndex],
    Format,
    Mode);

  if ((O == NULL) || (Value == NULL))
    {
    return(FAILURE);
    }

  if (MOGetComponent(O,OIndex,(void **)&L,mxoxLimits) == FAILURE)
    {
    /* cannot get component */

    return(FAILURE);
    }

  if (P != NULL)
    pindex = P->Index;

  switch (AIndex)
    {
    case mcaAList:

      {
      mfs_t *F;

      char *ptr;
      char *TokPtr;

      mgcred_t *A;

      if (MOGetComponent(O,OIndex,(void **)&F,mxoxFS) == FAILURE)
        {
        /* cannot get component */

        return(FAILURE);
        }

      if (Format != mdfString)
        {
        /* format not supported */

        break;
        }

      /* FORMAT:  <AID>[{,:}<AID>]... */

      ptr = MUStrTok((char *)Value,",: \n\t",&TokPtr);

      if ((Mode != mAdd) && (Mode != mDecr))
        {
        /* clear AAL */

        MULLFree(&F->AAL,NULL);
        }

      /* NOTE:  mcmpDECR not supported */

      while (ptr != NULL)
        {
        if (strcmp(ptr,DEFAULT) && strcmp(ptr,"DEFAULT"))
          {
          if ((MAcctFind(ptr,(mgcred_t **)&A) == SUCCESS) ||
              (MAcctAdd(ptr,(mgcred_t **)&A) == SUCCESS))
            {
            /* add account to AList */

            MULLAdd(&F->AAL,ptr,(void *)A,NULL,NULL);  /* no free routine */
            }
          }

        ptr = MUStrTok(NULL,",: \n\t",&TokPtr);
        }  /* END while (ptr != NULL) */

      if ((F->AAL != NULL) && (F->ADef == NULL))
        {
        F->ADef = (mgcred_t *)F->AAL->Ptr;
        }
      }    /* END BLOCK (case mcaAList) */

      break;

    case mcaDefTPN:

      {
      int tmpI;

      mjob_t *J;

      if ((L->JobDefaults == NULL) &&
          (MJobAllocBase(&L->JobDefaults,NULL) == FAILURE))
        {
        return(FAILURE);
        }

      J = L->JobDefaults;

      if (Format == mdfString)
        tmpI = (int)strtol((char *)Value,NULL,10);
      else
        tmpI = *(int *)Value;

      J->FloatingNodeCount = tmpI;
      }  /* END BLOCK */

      break;

    case mcaDefWCLimit:

      {
      long tmpL;

      mjob_t *J;

      if (L->JobDefaults == NULL)
        MJobAllocBase(&L->JobDefaults,NULL);

      J = L->JobDefaults;

      if (Format == mdfString)
        tmpL = MUTimeFromString((char *)Value);
      else
        tmpL = *(long *)Value;

      J->SpecWCLimit[0] = tmpL;
      }  /* END BLOCK */

      break;
   
    case mcaGFSTarget:
    case mcaGFSUsage:

      {
      double tmpD;

      if (MOGetComponent(O,OIndex,(void **)&F,mxoxFS) == FAILURE)
        {
        /* cannot get FS component */

        return(FAILURE);
        }

      if (Format == mdfString)
        tmpD = strtod((char *)Value,NULL);
      else
        tmpD = *(double *)Value;

      if (AIndex == mcaGFSTarget)
        F->GFSTarget = tmpD;
      else
        F->GFSUsage = tmpD;
      }  /* END (case mcaGFSUsage) */
 
      break;
 
    case mcaHold:

      {
      switch (OIndex)
        {
        case mxoUser:
        case mxoGroup:
        case mxoAcct:

          if (((mgcred_t *)O)->DoHold == TRUE)
            {
            ((mgcred_t *)O)->DoHold = FALSE;
            }
          else
            {
            ((mgcred_t *)O)->DoHold = TRUE;
            }

          break;

        case mxoQOS:

          if (((mqos_t *)O)->DoHold == TRUE)
            {
            ((mqos_t *)O)->DoHold = FALSE;
            }
          else
            {
            ((mqos_t *)O)->DoHold = TRUE;
            }

          break;

        case mxoClass:

          if (((mclass_t *)O)->DoHold == TRUE)
            {
            ((mclass_t *)O)->DoHold = FALSE;
            }
          else
            {
            ((mclass_t *)O)->DoHold = TRUE;
            }

          break;

        default:

          /* NOT HANDLED */

          return(FAILURE);

          /*NOTREACHED*/

          break;
        }  /* END switch (OIndex) */
      }    /* END BLOCK */

      break;

    case mcaID:
  
      {
      int index = 0;

      while ((((char *)Value)[index] == ' ') && (((char *)Value[index] != '\0'))) { index++; }

      switch (OIndex)
        {
        case mxoUser:
        case mxoGroup:
        case mxoAcct:
 
          strcpy(((mgcred_t *)O)->Name,(char *)&((char *)Value)[index]);
 
          break;
 
        case mxoQOS:
 
          strcpy(((mqos_t *)O)->Name,(char *)Value);
 
          break;
 
        case mxoPar:
 
          strcpy(((mpar_t *)O)->Name,(char *)Value);
 
          break;
 
        case mxoClass:
 
          strcpy(((mclass_t *)O)->Name,(char *)Value);
 
          break;

        default:

          /* NOT HANDLED */

          return(FAILURE);

          /*NOTREACHED*/

          break;
        }  /* END switch (OIndex) */
      } 

      break;

    case mcaManagers:

      switch (OIndex)
        {
        case mxoAcct:
        case mxoGroup:
        case mxoUser:
        case mxoClass:
        case mxoQOS:

          if (Format != mdfString)
            break;

          if (Value != NULL)
            {
            char *ptr;
            char *TokPtr;

            int   mindex;

            mfs_t *F;

            /* FORMAT:  <USER>[,<USER>]... */

            if (MOGetComponent(O,OIndex,(void **)&F,mxoxFS) == FAILURE)
              {
              /* cannot get FS component */

              return(FAILURE);
              }

            memset(F->ManagerU,0,sizeof(F->ManagerU));

            mindex = 0;

            ptr = MUStrTok((char *)Value,",",&TokPtr);

            while (ptr != NULL)
              {
              if (MUserAdd(ptr,&F->ManagerU[mindex]) == FAILURE)
                break;

              mindex++;
       
              ptr = MUStrTok(NULL,",",&TokPtr);

              if (mindex >= MMAX_CREDMANAGER)
                break;
              }
            }  /* END if (Value != NULL) */

          break;

        default:
 
          /* NO-OP */

          break;
        }  /* END switch (OIndex) */

      break;

    case mcaNoSubmit:

      switch (OIndex)
        {
        case mxoAcct:
        case mxoGroup:
        case mxoUser:

          if (Format != mdfString)
            break;

          if (Value != NULL)
            {
            ((mgcred_t *)O)->NoSubmit = MUBoolFromString((char *)Value,FALSE);
            }

          break;

        case mxoClass:

          if (Format != mdfString)
            break;

          if (Value != NULL)
            {
            ((mclass_t *)O)->NoSubmit = MUBoolFromString((char *)Value,FALSE);
            }

          break;

        case mxoQOS:

          if (Format != mdfString)
            break;

          if (Value != NULL)
            {
            ((mqos_t *)O)->NoSubmit = MUBoolFromString((char *)Value,FALSE);
            }

          break;
 
      
        default:
 
          /* NO-OP */

          break;
        }  /* END switch (OIndex) */

      break;

    case mcaMaxIJob:
    case mcaMaxIProc:
    case mcaMaxIPS:
    case mcaMaxGRes:
    case mcaMaxIGRes:
    case mcaMaxJob:
    case mcaMaxArrayJob:
    case mcaMaxIArrayJob:
    case mcaMaxNode:
    case mcaMaxPE:
    case mcaMaxProc:
    case mcaMaxWC:
    case mcaMaxPS:
    case mcaMaxMem:
    case mcaRMaxCount:
    case mcaRMaxProc:
    case mcaRMaxPS:
    case mcaRMaxDuration:
    case mcaRMaxTotalProc:
    case mcaRMaxTotalPS:
    case mcaRMaxTotalDuration:

      {
      enum MLimitTypeEnum LType = mlActive;  /* set default */
 
      long tmpLS = -1;
      long tmpLH = -1;

      mpu_t *P = NULL;

      enum MActivePolicyTypeEnum PIndex = mptNONE;

      __MCredParseLimit(
        (char *)Value,
        &tmpLS,
        &tmpLH);

      switch (AIndex)
        {
        case mcaMaxIGRes:

          /* non multidimensional, so we don't have to do anything with the ArrayLine[].
             Go ahead and set P even if something in ArrayLine[]. */

          if ((L->IdlePolicy == NULL) && (MPUCreate(&L->IdlePolicy) == FAILURE))
            {
            break;
            }

          P = L->IdlePolicy;

          PIndex = mptMaxGRes;
          LType  = mlIdle;

          break;

        case mcaMaxIJob:

          if (MUStrIsEmpty(ArrayLine))
            {
            if ((L->IdlePolicy == NULL) && (MPUCreate(&L->IdlePolicy) == FAILURE))
              {
              break;
              }

            P = L->IdlePolicy;
            }

          PIndex = mptMaxJob;
          LType  = mlIdle;

          break;

        case mcaMaxIArrayJob:

          if (MUStrIsEmpty(ArrayLine))
            {
            if ((L->IdlePolicy == NULL) && (MPUCreate(&L->IdlePolicy) == FAILURE))
              {
              break;
              }

            P = L->IdlePolicy;
            }

          PIndex = mptMaxArrayJob;
          LType  = mlIdle;

          break;

        case mcaMaxIProc:

          if (MUStrIsEmpty(ArrayLine))
            {
            if ((L->IdlePolicy == NULL) && (MPUCreate(&L->IdlePolicy) == FAILURE))
              {
              break;
              }

            P = L->IdlePolicy;
            }

          PIndex = mptMaxProc;
          LType  = mlIdle;

          break;

        case mcaMaxIPS:

          if (MUStrIsEmpty(ArrayLine))
            {
            if ((L->IdlePolicy == NULL) && (MPUCreate(&L->IdlePolicy) == FAILURE))
              {
              break;
              }

            P = L->IdlePolicy;
            }

          PIndex = mptMaxPS;
          LType  = mlIdle;

          break;

        case mcaMaxIPE:

          if (MUStrIsEmpty(ArrayLine))
            {
            if ((L->IdlePolicy == NULL) && (MPUCreate(&L->IdlePolicy)))
              {
              break;
              }

            P = L->IdlePolicy;
            }

          PIndex = mptMaxPE;
          LType  = mlIdle;

          break;

        case mcaMaxGRes:

          PIndex = mptMaxGRes;
          P      = &L->ActivePolicy;

          break;

        case mcaMaxJob:

          PIndex = mptMaxJob;
          P      = &L->ActivePolicy;

          break;

        case mcaMaxArrayJob:

          PIndex = mptMaxArrayJob;
          P      = &L->ActivePolicy;

          break;

        case mcaMaxMem:

          PIndex = mptMaxMem;
          P      = &L->ActivePolicy;

          break;

        case mcaMaxNode:

          PIndex = mptMaxNode;
          P      = &L->ActivePolicy;

          break;

        case mcaMaxPE:

          PIndex = mptMaxPE;
          P      = &L->ActivePolicy;

          break;

        case mcaMaxProc:

          PIndex = mptMaxProc;
          P      = &L->ActivePolicy;

          break;

        case mcaMaxPS:

          PIndex = mptMaxPS;
          P      = &L->ActivePolicy;

          break;

        case mcaMaxWC:

          PIndex = mptMaxWC;
          P      = &L->ActivePolicy;

          break;

        /* reservation policies */

        case mcaRMaxDuration:

          PIndex = mptMaxWC;

          if ((L->RsvPolicy == NULL) && (MPUCreate(&L->RsvPolicy) == FAILURE))
            {
            break;
            }

          P = L->RsvPolicy;

          break;

        case mcaRMaxPS:

          PIndex = mptMaxPS;

          if ((L->RsvPolicy == NULL) && (MPUCreate(&L->RsvPolicy) == FAILURE))
            {
            break;
            }

          P = L->RsvPolicy;

          break;

        case mcaRMaxProc:

          PIndex = mptMaxProc;

          if ((L->RsvPolicy == NULL) && (MPUCreate(&L->RsvPolicy) == FAILURE))
            {
            break;
            }

          P = L->RsvPolicy;

          break;

        case mcaRMaxCount:

          PIndex = mptMaxJob;

          if ((L->RsvActivePolicy == NULL) && (MPUCreate(&L->RsvActivePolicy) == FAILURE))
            {
            break;
            }

          P = L->RsvActivePolicy;

          break;

        case mcaRMaxTotalDuration:

          PIndex = mptMaxWC;

          if ((L->RsvActivePolicy == NULL) && (MPUCreate(&L->RsvActivePolicy) == FAILURE))
            {
            break;
            }

          P = L->RsvActivePolicy;

          break;

        case mcaRMaxTotalPS:

          PIndex = mptMaxPS;

          if ((L->RsvActivePolicy == NULL) && (MPUCreate(&L->RsvActivePolicy) == FAILURE))
            {
            break;
            }

          P = L->RsvActivePolicy;

          break;

        case mcaRMaxTotalProc:

          PIndex = mptMaxProc;

          if ((L->RsvActivePolicy == NULL) && (MPUCreate(&L->RsvActivePolicy) == FAILURE))
            {
            break;
            }

          P = L->RsvActivePolicy;

          break;

        default:

          return(FAILURE);

          /*NOTREACHED*/

          break;
        }  /* END switch (AIndex) */

      if ((!MUStrIsEmpty(ArrayLine)) && 
          ((AIndex == mcaMaxGRes) || (AIndex == mcaMaxIGRes)))
        {
        /* multi-dimensional not supported, however, specific GRes has been 
           used */

        mgpu_t *GP = NULL;

        int gindex;

        MUStrToLower(ArrayLine);

        gindex = MUMAGetIndex(meGRes,ArrayLine,mAdd);

        if (gindex < 0)
          {
          /* could not get gindex */

          return(FAILURE);
          }
        
        if (P != NULL)
          {
          if (P->GLimit == NULL)
            {
            P->GLimit = (mhash_t *)MUCalloc(1,sizeof(mhash_t));
           
            MUHTCreate(P->GLimit,-1);
            }
           
          if (MUHTGet(P->GLimit,ArrayLine,(void **)&GP,NULL) == FAILURE)
            {
            GP = (mgpu_t *)MUCalloc(1,sizeof(mgpu_t));
           
            MUHTAdd(P->GLimit,ArrayLine,(void *)GP,NULL,MUFREE);
            }
          }

        MGPUInitialize(GP);

        GP->SLimit[pindex] = tmpLS;
        GP->HLimit[pindex] = tmpLH;
        }
      else if (!MUStrIsEmpty(ArrayLine))
        {
        int oindex;

        char *OType;
        char *OName;

        char *TokPtr;

        /* add multidimensional policy */

        if (((OType = MUStrTok(ArrayLine,":",&TokPtr)) == NULL) ||
            ((oindex = MUGetIndexCI(OType,MXOC,TRUE,mxoNONE)) == mxoNONE))
          {
          break;
          }

        OName = MUStrTok(NULL,":,",&TokPtr);

        do
          {
          if (OName != NULL)
            {
            /* specific credential policy specified */
 
            switch (oindex)
              {
              case mxoAcct:
 
                {
                mpu_t *P = NULL;

                mgcred_t *A = NULL;
 
                if (MAcctAdd(OName,&A) == FAILURE)
                  {
                  break;
                  }
 
                MLimitGetMPU(L,A,mxoAcct,LType,&P);

                P->SLimit[PIndex][pindex] = tmpLS;
                P->HLimit[PIndex][pindex] = tmpLH;
                }
 
                break;

              case mxoClass:
 
                {
                mpu_t *P = NULL;

                mclass_t *C = NULL;
 
                if (MClassAdd(OName,&C) == FAILURE)
                  {
                  break;
                  }
 
                MLimitGetMPU(L,C,mxoClass,LType,&P);

                P->SLimit[PIndex][pindex] = tmpLS;
                P->HLimit[PIndex][pindex] = tmpLH;
                }  /* END BLOCK */
 
                break;
 
              case mxoQOS:
 
                {
                mpu_t *P = NULL;

                mqos_t *Q = NULL;
 
                if (MQOSAdd(OName,&Q) == FAILURE)
                  {
                  break;
                  }
 
                MLimitGetMPU(L,Q,mxoQOS,LType,&P);

                P->SLimit[PIndex][pindex] = tmpLS;
                P->HLimit[PIndex][pindex] = tmpLH;
                }  /* END BLOCK */
 
                break;
 
              case mxoGroup:
 
                {
                mpu_t *P = NULL;

                mgcred_t *G = NULL;
 
                if (MGroupAdd(OName,&G) == FAILURE)
                  {
                  break;
                  }
 
                MLimitGetMPU(L,G,mxoGroup,LType,&P);
 
                P->SLimit[PIndex][pindex] = tmpLS;
                P->HLimit[PIndex][pindex] = tmpLH;
                }  /* END BLOCK */
 
                break;
 
              case mxoUser:
 
                {
                mgcred_t *U = NULL;
 
                if (MUserAdd(OName,&U) == FAILURE)
                  {
                  break;
                  }
 
                if (LType == mlIdle)
                  {
                  MLimitGetMPU(L,U,mxoUser,mlIdle,&P);
                  }
                else
                  {
                  MLimitGetMPU(L,U,mxoUser,LType,&P);
                  }

                P->SLimit[PIndex][pindex] = tmpLS;
                P->HLimit[PIndex][pindex] = tmpLH;
                }
 
                break;
 
              default:
 
                /* NO-OP */
 
                break;
              }  /* END switch (oindex) */
            }    /* END if ((OName = MUStrTok(NULL,":",&TokPtr)) != NULL) */
          else
            {
            /* general policy specified */
 
            switch (oindex)
              {
              case mxoClass:
 
                MLimitGetMPU(L,MSched.DefaultC,mxoClass,LType,&P);
 
                P->SLimit[PIndex][pindex] = tmpLS;
                P->HLimit[PIndex][pindex] = tmpLH;
 
                break;

              case mxoQOS:
 
                MLimitGetMPU(L,MSched.DefaultQ,mxoQOS,LType,&P);
 
                P->SLimit[PIndex][pindex] = tmpLS;
                P->HLimit[PIndex][pindex] = tmpLH;

                break;
 
              case mxoAcct:
 
                MLimitGetMPU(L,MSched.DefaultA,mxoAcct,LType,&P);

                P->SLimit[PIndex][pindex] = tmpLS;
                P->HLimit[PIndex][pindex] = tmpLH;
 
                break;

              case mxoGroup:
 
                MLimitGetMPU(L,MSched.DefaultG,mxoGroup,LType,&P);

                P->SLimit[PIndex][pindex] = tmpLS;
                P->HLimit[PIndex][pindex] = tmpLH;

                break;
 
              case mxoUser:
 
                MLimitGetMPU(L,MSched.DefaultU,mxoUser,LType,&P);

                P->SLimit[PIndex][pindex] = tmpLS;
                P->HLimit[PIndex][pindex] = tmpLH;
 
                break;
 
              default:
 
                /* NO-OP */
 
                break;
              }  /* END switch (oindex) */
            }    /* END else ((OName = MUStrTok(NULL,":",&TokPtr)) != NULL) */

          OName = MUStrTok(NULL,":,",&TokPtr);

          } while (OName != NULL);
        }      /* END if (ArrayLine[0] != '\0') */
      else
        {
        /* set basic limits */

        P->SLimit[PIndex][pindex] = tmpLS;
        P->HLimit[PIndex][pindex] = tmpLH;

        if ((MSched.PerPartitionScheduling == TRUE) && (pindex == 0))
          {
          int pindex;

          /* if we set the limit for partition 0  set the same limit for
             all other partitions that are not already set. */

          for (pindex = 1;pindex < MMAX_PAR;pindex++)
            {
            if ((tmpLS > 0) && (P->SLimit[PIndex][pindex] == 0))
              P->SLimit[PIndex][pindex] = tmpLS;

            if ((tmpLH > 0) && (P->HLimit[PIndex][pindex] == 0))
              P->HLimit[PIndex][pindex] = tmpLH;
            }
          }    /* END for pindex */
        }    /* END else */
      }    /* END BLOCK (case mcaMaxPE, ...) */

      break;

   case mcaMaxJobPerUser:

      {
      switch (OIndex)
        {
        case mxoClass:
        case mxoQOS:

          {
          mpu_t *P = NULL;

          long tmpS = 0;
          long tmpH = 0;

          MLimitGetMPU(L,(void *)MSched.DefaultU,mxoUser,mlActive,&P);

          if (Format == mdfInt)
            {
            tmpS = *(int *)Value;
            tmpH = *(int *)Value;
            }
          else
            {
            /* default format is 'string' */

            __MCredParseLimit((char *)Value,&tmpS,&tmpH);
            }

          P->SLimit[mptMaxJob][0] = tmpS;
          P->HLimit[mptMaxJob][0] = tmpH;
          }    /* END BLOCK */

          break;

        default:

          /* NOT HANDLED */

          return(FAILURE);

          /*NOTREACHED*/

          break;
        }  /* END switch (OIndex) */
      }    /* END BLOCK */

      break;

    case mcaMaxJobPerGroup:

      {
      switch (OIndex)
        {
        case mxoClass:
        case mxoQOS:

          {
          mpu_t *P = NULL;

          long tmpS = 0;
          long tmpH = 0;

          MLimitGetMPU(L,MSched.DefaultG,mxoGroup,mlActive,&P);

          if (Format == mdfInt)
            {
            tmpS = *(long *)Value;
            tmpH = *(long *)Value;
            }
          else
            {
            /* default format is 'string' */

            __MCredParseLimit((char *)Value,&tmpS,&tmpH);
            }

          P->SLimit[mptMaxJob][0] = tmpS;
          P->HLimit[mptMaxJob][0] = tmpH;
          }    /* END BLOCK */

          break;

        default:

          /* NOT HANDLED */

          return(FAILURE);

          /*NOTREACHED*/

          break;
        }  /* END switch (OIndex) */
      }    /* END BLOCK */

      break;

    case mcaMinNodePerJob:
    case mcaMaxNodePerJob:

      {
      long     tmpL;

      mjob_t *J;
      mjob_t **JP;

      JP = (AIndex == mcaMinNodePerJob) ?&L->JobMinimums[pindex] :
                                         &L->JobMaximums[pindex];

      if ((*JP == NULL) &&
          (MJobAllocBase(JP,NULL) == FAILURE))
        {
        return(FAILURE);
        }

      J = *JP;

      if (Value == NULL)
        tmpL = 0;
      else if (Format == mdfInt)
        tmpL = *(int *)Value;
      else
        tmpL = strtol((char *)Value,NULL,10);

      J->Request.NC = tmpL;
      }  /* END BLOCK */

      break;

    case mcaMaxProcPerJob:

      {
      long     tmpL;

      mjob_t *J;

      if ((L->JobMaximums[pindex] == NULL) &&
          (MJobAllocBase(&L->JobMaximums[pindex],NULL) == FAILURE))
        {
        return(FAILURE);
        }

      J = L->JobMaximums[pindex];

      if (Value == NULL)
        tmpL = 0;
      else if (Format == mdfInt)
        tmpL = *(int *)Value;
      else
        tmpL = strtol((char *)Value,NULL,10);

      J->Request.TC = tmpL;
      
      bmset(&J->IFlags,mjifTasksSpecified);
      }  /* END BLOCK */

      break;

    case mcaMaxProcPerUser:

      switch (OIndex)
        {
        case mxoClass:
        case mxoQOS:

          {
          mpu_t *P = NULL;

          long tmpS = 0;
          long tmpH = 0;

          MLimitGetMPU(L,(void *)MSched.DefaultU,mxoUser,mlActive,&P);

          if (Format == mdfInt)
            {
            tmpS = *(int *)Value;
            tmpH = *(int *)Value;
            }
          else
            {
            /* default format is 'string' */

            __MCredParseLimit((char *)Value,&tmpS,&tmpH);
            }

          P->SLimit[mptMaxProc][0] = tmpS;
          P->HLimit[mptMaxProc][0] = tmpH;

          break;
          }    /* END BLOCK */

        default:

          /* NO-OP */

          break;
        }  /* END switch (OIndex) */

      break;

    case mcaMaxNodePerUser:

      switch (OIndex)
        {
        case mxoClass:
        case mxoQOS:

          {
          mpu_t *P = NULL;

          long tmpS = 0;
          long tmpH = 0;

          MLimitGetMPU(L,(void *)MSched.DefaultU,mxoUser,mlActive,&P);

          if (Format == mdfInt)
            {
            tmpS = *(int *)Value;
            tmpH = *(int *)Value;
            }
          else
            {
            /* default format is 'string' */

            __MCredParseLimit((char *)Value,&tmpS,&tmpH);
            }

          P->SLimit[mptMaxNode][0] = tmpS;
          P->HLimit[mptMaxNode][0] = tmpH;

          break;
          }    /* END BLOCK */

        default:

          /* NO-OP */

          break;
        }  /* END switch (OIndex) */

      break;

    case mcaMaxSubmitJobs:

      if (Format != mdfString)
        break;

      if (Value != NULL)
        {
        int pindex = 0;

        if (P != NULL)
          pindex = P->Index;

        L->MaxSubmitJobs[pindex] = (int)strtol((char *)Value,NULL,10);

        if ((MSched.PerPartitionScheduling == TRUE) &&
            (L->MaxSubmitJobs[0] != 0))
          {
          int pindex;
    
          /* if this limit is not partition specific then
             apply it to all partitions that do not already
             have a limit */
    
          for (pindex = 1; pindex < (MMAX_PAR + 1); pindex++)
            {
            if (L->MaxSubmitJobs[pindex] == 0)
              {
              L->MaxSubmitJobs[pindex] = L->MaxSubmitJobs[0];  
              }
            } /* END for pindex */
          } /* END if MSched.PerPartitionScheduling .... */
        }

      break;

    case mcaMaxWCLimitPerJob:
    case mcaMinWCLimitPerJob:

      {
      long tmpL;

      mjob_t *J;
      mjob_t **JP;
      mreq_t *RQ;

      JP = (AIndex == mcaMaxWCLimitPerJob) ? &L->JobMaximums[pindex] : &L->JobMinimums[pindex];

      if ((*JP == NULL) &&
          (MJobAllocBase(JP,&RQ) == FAILURE))
        {
        return(FAILURE);
        }

      J = *JP;

      bmset(&J->IFlags,mjifIsTemplate);

      if (Format == mdfString)
        {
        char *ptr;

        char *wptr;  /* wclimit */
        char *nptr;  /* node */
        char *TokPtr = NULL;
        char *TokPtr2 = NULL;

        int   sindex;

        sindex = 0;

        /* FORMAT:  <WCLIMIT>[@<PROCCOUNT>][,<WCLIMIT>[@<PROCCOUNT>]]... */

        ptr = MUStrTok((char *)Value,",",&TokPtr);

        while (ptr != NULL)
          {
          wptr = MUStrTok(ptr,"@",&TokPtr2);
          nptr = MUStrTok(NULL,"@",&TokPtr2);

          if (nptr == NULL)
            {
            tmpL = MUTimeFromString((char *)wptr);

            J->WCLimit = tmpL;
            J->SpecWCLimit[0] = tmpL;

            if (sindex == 0)
              sindex++;
            }
          else
            {
            tmpL = MUTimeFromString((char *)wptr);

            J->SpecWCLimit[sindex] = tmpL;
            J->Req[0]->TaskRequestList[sindex] = (int)strtol(nptr,NULL,10);

            sindex++;

            if (sindex >= MMAX_SHAPE)
              break;
            }

          ptr = MUStrTok(NULL,",",&TokPtr);
          }  /* END while (ptr != NULL) */
        }    /* END if (Format == mdfString) */
      else
        {
        tmpL = *(long *)Value;

        J->WCLimit        = tmpL;
        J->SpecWCLimit[0] = tmpL;
        }

      if (Format == mdfString)
        tmpL = MUTimeFromString((char *)Value);
      else
        tmpL = *(long *)Value;

      J->WCLimit        = tmpL;
      J->SpecWCLimit[0] = tmpL;
      }  /* END BLOCK */

      break;

    case mcaMessages:
    case mcaComment:

      {
      mmb_t **MBP;

      switch (OIndex)
        {
        case mxoUser:
        case mxoGroup:
        case mxoAcct:
        default:

          MBP = &((mgcred_t *)O)->MB;

          break;

        case mxoQOS:

          MBP = &((mqos_t *)O)->MB;

          break;

        case mxoClass:

          MBP = &((mclass_t *)O)->MB;

          break;
        }  /* END switch (OIndex) */

      if (Format == mdfOther)
        {
        mmb_t *MP = (mmb_t *)Value;

        MMBAdd(MBP,MP->Data,MP->Owner,MP->Type,MP->ExpireTime,MP->Priority,NULL);

        break;
        }
      else
        {
        char *mptr;

        char tmpLine[MMAX_LINE << 2];

        /* append message to message buffer */

        mptr = (char *)Value;

        if (mptr == NULL)
          break;

        if (MUStringIsPacked(mptr) == TRUE)
          {
          MUStringUnpack(mptr,tmpLine,sizeof(tmpLine),NULL);

          mptr = tmpLine;
          }
 
        if (mptr[0] == '<')
          {
          /* extract XML */

          MMBFromString(mptr,MBP);
          }      /* END if (mptr[0] == '<') */
        else if (mptr[0] == '\0')
          {
          if (AIndex == mcaComment)
            {
            MMBRemoveMessage(MBP,NULL,mmbtAnnotation);
            }
          }
        else
          {
          MMBAdd(
            MBP,
            mptr,
            NULL,
            (AIndex == mcaComment) ? mmbtAnnotation : mmbtOther,
            MSched.Time + MCONST_DAYLEN,
            0,
            NULL);
          }
        }  /* END else if (Format == mdfOther) */
      }    /* END BLOCK */

      break;

    case mcaPList:

      if (MOGetComponent(O,OIndex,(void **)&F,mxoxFS) == FAILURE)
        {
        /* cannot get FS component */

        return(FAILURE);
        }

      if (Mode == mSet)
        {
        /* Clear out current PAL */

        bmclear(&F->PAL);

        MPALFromRangeString((char *)Value,&F->PAL,0);
        }
      else if (Mode == mAdd)
        {
        MPALFromRangeString((char *)Value,&F->PAL,mAdd);
        }

      break;

    case mcaQList:

      if (MOGetComponent(O,OIndex,(void **)&F,mxoxFS) == FAILURE)
        {
        /* cannot get FS component */

        return(FAILURE);
        }

      {
      char QALLine[MMAX_LINE];
      mbitmap_t tmpQAL;  /* QOS access list */

      mqos_t *FQ;

      if (strchr((char *)Value,'&') != NULL)
        F->QALType = malAND;
      else if (strchr((char *)Value,'^') != NULL)
        F->QALType = malOnly;
      else
        F->QALType = malOR;

      /* support '=' and '+=' */

      /* FORMAT:  <QOS>[,<QOS>]... */

      if (MQOSListBMFromString((char *)Value,&tmpQAL,&FQ,mAdd) == FAILURE)
        {
        MDB(5,fFS) MLog("ALERT:    cannot parse string '%s' as rangelist\n",
          (char *)Value);
        }

      /* support '=' and '+=' */

      if (Mode == mIncr)
        {
        bmor(&tmpQAL,&F->QAL);
        }
      else if (Mode == mDecr)
        {
        bmnot(&tmpQAL,MMAX_QOS);

        bmand(&F->QAL,&tmpQAL);
        }
      else
        {
        bmcopy(&F->QAL,&tmpQAL);
        }

      MDB(2,fFS) MLog("INFO:     QOS access list set to %s\n",
        MQOSBMToString(&F->QAL,QALLine));

      if (F->QDef[0] != NULL)
        {
        /* if set, add QDef to QList */

        bmset(&F->QAL,((mqos_t *)F->QDef[0])->Index);
        }
      else if ((FQ != NULL) && (MSched.QOSIsOptional == FALSE))
        {
        /* if not set, add first QList QOS as QDef */

        F->QDef[0] = FQ;
        }
      }  /* END BLOCK (case mcaQList) */

      break;

    case mcaRole:

      switch (OIndex)
        {
        case mxoUser:

          {
          int       aindex;

          int RIndex;

          int AIndex = 4;

          if ((RIndex = MUGetIndexCI((char *)Value,MRoleType,FALSE,mcalNONE)) == mcalNONE)
            break;

          ((mgcred_t *)O)->Role = (enum MRoleEnum)RIndex;

          /* add entry to ADMIN* list */

          switch (RIndex)
            {
            case mcalAdmin1:

              AIndex = 1;

              break;

            case mcalAdmin2:

              AIndex = 2;

              break;

            case mcalAdmin3:

              AIndex = 3;

              break;

            case mcalAdmin4:

              AIndex = 4;

              break;

            default:

              /* unreachable */

              break;
            }  /* END switch (RIndex) */

          for (aindex = 0;aindex < MMAX_ADMINUSERS;aindex++)
            {
            if (MSched.Admin[AIndex].UName[aindex][0] == '\0')
              break;
            }  /* END for (aindex) */

          if (aindex >= MMAX_ADMINUSERS)
            {
            /* max admin count reached */

            break;
            }

          MUStrCpy(
            MSched.Admin[AIndex].UName[aindex],
            ((mgcred_t *)O)->Name,
            MMAX_NAME);

          MSched.Admin[AIndex].UName[aindex + 1][0] = '\0';
          }  /* END BLOCK (case mxoUser) */

          break;

        default:

          /* not supported */

          break;
        }  /* END switch (OIndex) */
     
      break;

    case mcaTrigger:

      if (MCredSetTrigAttr(O,OIndex,(char *)Value,L) == FAILURE)
        return(FAILURE);

      break;

    case mcaVariables:

      switch (OIndex)
        {
        case mxoUser:
        case mxoGroup:
        case mxoAcct:

          /* FORMAT:  <name>[=<value>][[;<name[=<value]]...] */

          {
          mln_t *tmpL;

          char *ptr;
          char *TokPtr;

          char *TokPtr2;

          ptr = MUStrTok((char *)Value,";",&TokPtr);

          while (ptr != NULL)
            {
            MUStrTok(ptr,"=",&TokPtr2);

            MULLAdd(&((mgcred_t *)O)->Variables,ptr,NULL,&tmpL,MUFREE);

            if (TokPtr2[0] != '\0')
              {
              MUStrDup((char **)&tmpL->Ptr,TokPtr2);
              }

            ptr = MUStrTok(NULL,";",&TokPtr);
            }  /* END while (ptr != NULL) */
          }    /* END BLOCK (case mxoAcct) */

          break;

        case mxoClass:

	        {
          mln_t *tmpL;

          char *ptr;
          char *TokPtr;

          char *TokPtr2;

          ptr = MUStrTok((char *)Value,";",&TokPtr);

          while (ptr != NULL)
            {
            MUStrTok(ptr,"=",&TokPtr2);

            MULLAdd(&((mclass_t *)O)->Variables,ptr,NULL,&tmpL,MUFREE);

            if (TokPtr2[0] != '\0')
              {
              MUStrDup((char **)&tmpL->Ptr,TokPtr2);
              }

            ptr = MUStrTok(NULL,";",&TokPtr);
            }  /* END while (ptr != NULL) */
          }    /* END BLOCK (case mxoClass) */

	        break;

        case mxoQOS:

          {
          mln_t *tmpL;

          char *ptr;
          char *TokPtr;

          char *TokPtr2;

          ptr = MUStrTok((char *)Value,";",&TokPtr);

          while (ptr != NULL)
            {
            MUStrTok(ptr,"=",&TokPtr2);

            MULLAdd(&((mqos_t *)O)->Variables,ptr,NULL,&tmpL,NULL);

            if (TokPtr2[0] != '\0')
              {
              MUStrDup((char **)&tmpL->Ptr,TokPtr2);
              }

            ptr = MUStrTok(NULL,";",&TokPtr);
            }  /* END while (ptr != NULL) */
          }    /* END BLOCK (case mxoAcct) */

      	  break;

        default:

          /* NOT-SUPPORTED */

          break;
        }  /* END switch (OIndex) */

    case mcaRsvCreationCost:

      if (Format == mdfDouble)
        ((mgcred_t *)O)->RsvCreationCost = *(double *)Value;

      break;

    default:

      /* not supported */

      return(FAILURE);

      /*NOTREACHED*/

      break;
    }  /* END switch (AIndex) */

  return(SUCCESS);
  }  /* END MCredSetAttr() */
/* END MCredSetAttr.c */
