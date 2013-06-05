/* HEADER */

      
/**
 * @file MCredAToString.c
 *
 * Contains: MCredAToString
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  

/**
 * Report specified credential attribute as string.
 *
 * @see MCredSetAttr() - peer
 *
 * @param O       (I)
 * @param OIndex  (I)
 * @param AIndex  (I)
 * @param String  (O) [minsize=MMAX_LINE]
 * @param DFormat (I)
 * @param DFlags  (I) [bitmap of enum mcm*]
 */

int MCredAToMString(

  void                *O,
  enum MXMLOTypeEnum   OIndex,
  enum MCredAttrEnum   AIndex,
  mstring_t           *String,
  enum MFormatModeEnum DFormat,
  mbitmap_t           *DFlags)

  {
  static mfs_t    *F;
  static mcredl_t *L;
  static must_t   *S;

  static void *CO = NULL;

  char   tempConvertBuf[64];

  if (String == NULL)
    {
    return(FAILURE);
    }

  String->clear();

  if (O == NULL)
    {
    return(FAILURE);
    }

  if (O != CO)
    {
    if ((MOGetComponent(O,OIndex,(void **)&F,mxoxFS) == FAILURE) ||
        (MOGetComponent(O,OIndex,(void **)&L,mxoxLimits) == FAILURE) ||
        (MOGetComponent(O,OIndex,(void **)&S,mxoxStats) == FAILURE))
      {
      /* cannot get components */
 
      return(FAILURE);
      }

    CO = O;
    }
  
  switch (AIndex)
    {
    case mcaAList:
      {
      mln_t *tmpL = F->AAL;
      mln_t *Next = NULL;

      mgcred_t *Acct = NULL;

      /* delete any account items that reference nonexistent or delete accounts */

      while (tmpL != NULL)
        {
        Next = tmpL->Next;

        MAcctFind(tmpL->Name,&Acct);

        if ((Acct == NULL) || (Acct->IsDeleted))
          {
          MULLRemove(&F->AAL,tmpL->Name,NULL);
          }

        tmpL = Next;
        }

      MULLToMString(F->AAL,FALSE,NULL,String);
      }

      break;

    case mcaADef:

     if (F->ADef != NULL)
       MStringSet(String,((mgcred_t *)F->ADef)->Name);

      break;

    case mcaCDef:

      if (F->CDef != NULL)
        MStringSet(String,(F->CDef)->Name);

      break;

    case mcaEMailAddress:

      switch (OIndex)
        {
        case mxoUser:

          if (((mgcred_t *)O)->EMailAddress != NULL)
            MStringSet(String,((mgcred_t *)O)->EMailAddress);

          break;

        default:

          /* NO-OP */

          break;
        }

      break;

    case mcaFlags:

       switch (OIndex)
        {
        case mxoClass:

          {
          mclass_t *C = (mclass_t *)O;

          if (!bmisclear(&C->Flags))
            bmtostring(&C->Flags,MClassFlags,String);
          }

          break;

        default:

          /* NO-OP */

          break;
        }

      break;

    case mcaFSCWeight:

      switch (OIndex)
        {
        case mxoAcct:
        case mxoGroup:
        case mxoUser:

          if (((mgcred_t *)O)->FSCWeight != 0)
            {
            snprintf(tempConvertBuf,sizeof(tempConvertBuf),"%ld", ((mgcred_t *)O)->FSCWeight);

            *String += tempConvertBuf;
            }

          break;

        default:

          /* NO-OP */

          break;
        }  /* END switch (OIndex) */

      break;

    case mcaDefTPN:

      if ((L->JobDefaults == NULL) ||
        (L->JobDefaults->FloatingNodeCount == 0))
        break;

      snprintf(tempConvertBuf,sizeof(tempConvertBuf),"%d", L->JobDefaults->FloatingNodeCount);
      *String += tempConvertBuf;
      break;

    case mcaDefWCLimit:

      {
      char TString[MMAX_LINE];

      if ((L->JobDefaults == NULL) ||
        (L->JobDefaults->SpecWCLimit[0] == 0))
        break;

      MULToTString(L->JobDefaults->SpecWCLimit[0],TString);

      MStringSet(String,TString);
      }

      break;

    case mcaEnableProfiling:

      {
      must_t *SPtr;

      if ((MOGetComponent(O,OIndex,(void **)&SPtr,mxoxStats) == FAILURE) ||
          (SPtr->IStat == NULL))
        {
        break;
        }

      /* if S->IStat != NULL, profiling is enabled */

      MStringSet(String,"true");
      }  /* END BLOCK */

      break;

    case mcaFSCap:

      {
      mfs_t *F;

      switch (OIndex)
        {
        case mxoAcct:
        case mxoGroup:
        case mxoUser:

          F = &((mgcred_t *)O)->F;

          break;

        case mxoQOS:

          F = &((mqos_t *)O)->F;

          break;

        case mxoClass:

          F = &((mclass_t *)O)->F;

          break;

        default:

          F = NULL;

          break;
        }  /* END switch (OIndex) */

      if ((F != NULL) && (F->FSCap > 0.0))
        MStringSet(String,MFSTargetToString(F->FSCap,F->FSCapMode));
      }    /* END BLOCK (case mcaFSCap) */

      break;

    case mcaFSTarget:

      {
      mfs_t *F;

      switch (OIndex)
        {
        case mxoAcct:
        case mxoGroup:
        case mxoUser:

          F = &((mgcred_t *)O)->F;

          break;

        case mxoQOS:

          F = &((mqos_t *)O)->F;

          break;

        case mxoClass:

          F = &((mclass_t *)O)->F;

          break;

        default:

          F = NULL;

          break;
        }  /* END switch (OIndex) */

      if ((F != NULL) && (F->FSCap > 0.0))
        MStringSet(String,MFSTargetToString(F->FSTarget,F->FSTargetMode));
      }    /* END BLOCK mcaFSTarget */

      break;

    case mcaHold:

      switch (OIndex)
        {
        case mxoAcct:
        case mxoGroup:
        case mxoUser:

          if (((mgcred_t *)O)->DoHold != MBNOTSET)
            *String += MBool[((mgcred_t *)O)->DoHold];

          break;

        case mxoQOS:

          if (((mqos_t *)O)->DoHold != MBNOTSET)
            *String += MBool[((mqos_t *)O)->DoHold];

          break;

        case mxoClass:

          if (((mclass_t *)O)->DoHold != MBNOTSET)
            *String += MBool[((mclass_t *)O)->DoHold];

          break;

        default:

          /* NO-OP */

          break;
        }  /* END switch (OIndex) */

      break;

    case mcaID:

      switch (OIndex)
        {
        case mxoUser:
        case mxoGroup:
        case mxoAcct:

          MStringSet(String,((mgcred_t *)O)->Name);

          break;

        case mxoQOS:

          MStringSet(String,((mqos_t *)O)->Name);

          break;

        case mxoPar:

          MStringSet(String,((mpar_t *)O)->Name);

          break;

        case mxoClass:

          MStringSet(String,((mclass_t *)O)->Name);

          break;

        default:

          /* NOT HANDLED */

          return(FAILURE);

          /*NOTREACHED*/

          break;
        }  /* END switch (OIndex) */

      break;

    case mcaIsDeleted:

      switch (OIndex)
        {
        case mxoAcct:
        case mxoGroup:
        case mxoUser:

          if (((mgcred_t *)O)->IsDeleted == TRUE)
            *String += MBool[TRUE];

          break;

        case mxoQOS:

          if (((mqos_t *)O)->IsDeleted == TRUE)
            *String += MBool[TRUE];

          break;

        case mxoClass:

          if (((mclass_t *)O)->IsDeleted == TRUE)
            *String += MBool[TRUE];

          break;

        default:

          /* NO-OP */

          break;
        }  /* END switch (OIndex) */

      break;

    case mcaJobFlags:

      {
      mfs_t* FPtr = &((mclass_t *)O)->F;
  
      if (FPtr == NULL)
         break; 
  
      switch (OIndex)
        {
        case mxoClass: 

          if (!bmisclear(&FPtr->JobFlags))
            MJobFlagsToMString(NULL,&FPtr->JobFlags,String);

          break;
  
        default:
          /* NYI */

          break;
        }
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

          {
          int mindex;
          char tmpLine[MMAX_LINE];

          mfs_t *F;

          tmpLine[0] = '\0';

          if (MOGetComponent(O,OIndex,(void **)&F,mxoxFS) == FAILURE)
            {
            /* cannot get FS component */
  
            return(FAILURE);
            }

          for (mindex = 0;mindex < MMAX_CREDMANAGER;mindex++)
            {
            if ((F->ManagerU[mindex] != NULL) &&
                (F->ManagerU[mindex]->Name[0] != '\0'))
              {
              MUStrAppendStatic(
                tmpLine,
                F->ManagerU[mindex]->Name,
                ',',
                sizeof(tmpLine));
              }
            }    /* END for (mindex) */

          MStringSet(String,tmpLine);
          }  /* END BLOCK (case mxoAcct,...) */

          break;

        default:

          /* NO-OP */

          break;
        }  /* END switch (OIndex) */

      break;

    case mcaMaxIJob:

      break;

    case mcaMaxIProc:

      break;

    case mcaMaxIWC:

      {
      char *ptr;

      if (L->IdlePolicy != NULL &&
          ((ptr = MCredShowLimit(L->IdlePolicy,mptMaxWC,0,FALSE)) != NULL))
        {
        MStringSet(String,ptr);
        }
      }

      break;

    case mcaMaxJob:

      {
      char *ptr;
      ptr = NULL;

      if ((ptr = MCredShowLimit(&L->ActivePolicy,mptMaxJob,0,FALSE)) != NULL)
        {
        MStringSet(String,ptr);
        }
      }

      break;

    case mcaMaxMem:

      {
      char *ptr;
      ptr = NULL;

      if ((ptr = MCredShowLimit(&L->ActivePolicy,mptMaxMem,0,FALSE)) != NULL)
        {
        MStringSet(String,ptr);
        }
      }

      break;

    case mcaMaxNode:

      /* NYI */

      break;

    case mcaMaxProc:

      {
      char *ptr;
      ptr = NULL;

      if ((ptr = MCredShowLimit(&L->ActivePolicy,mptMaxProc,0,FALSE)) != NULL)
        {
        MStringSet(String,ptr);
        }
      }

      break;

    case mcaMaxPE:

      /* NYI */

      break;

    case mcaMaxPS:

      /* NYI */

      break;

    case mcaMinNodePerJob:

      {
      int tmpI = 0;

      if (L->JobMinimums[0] != NULL)
        tmpI = L->JobMinimums[0]->Request.NC;

      if (tmpI == 0)
        {
        break;
        }

      snprintf(tempConvertBuf,sizeof(tempConvertBuf),"%d", tmpI);
      *String += tempConvertBuf;
      }  /* END BLOCK */

      break;

    case mcaMaxNodePerJob:

      {
      int tmpI = 0;

      if (L->JobMaximums[0] != NULL)
        tmpI = L->JobMaximums[0]->Request.NC;

      if (tmpI == 0)
        {
        break;
        }

      snprintf(tempConvertBuf,sizeof(tempConvertBuf),"%d", tmpI);
      *String += tempConvertBuf;
      }  /* END BLOCK */

      break;

    case mcaMaxProcPerJob:

      {
      int tmpI = 0;

      /* NOTE:  temporarily map tasks to procs */

      if (L->JobMaximums[0] != NULL)
        tmpI = L->JobMaximums[0]->Request.TC;

      if (tmpI == 0)
        {
        break;
        }

      snprintf(tempConvertBuf,sizeof(tempConvertBuf),"%d", tmpI);
      *String += tempConvertBuf;
      }  /* END BLOCK */

      break;

    case mcaMaxProcPerNode:
    
      switch (OIndex)
        {
        case mxoAcct:
        case mxoGroup:
        case mxoUser:
        case mxoQOS:
        default:

          /* NO-OP */

          break;

        case mxoClass:

          if (((mclass_t *)O)->MaxProcPerNode > 0)
            {
            snprintf(tempConvertBuf,sizeof(tempConvertBuf),"%d",((mclass_t *)O)->MaxProcPerNode);
            *String += tempConvertBuf;
            }

          break;

        }  /* END switch (OIndex) */

      break;

    case mcaMaxProcPerUser:

      if (L->UserActivePolicy != NULL)
        {
        mpu_t *P = NULL;
 
        if (MUHTGet(L->UserActivePolicy,"DEFAULT",(void **)&P,NULL) == SUCCESS)
          {
          snprintf(tempConvertBuf,sizeof(tempConvertBuf),
            "%ld,%ld",
            P->SLimit[mptMaxProc][0],
            P->HLimit[mptMaxProc][0]);
          *String += tempConvertBuf;
          }
        }

      break;

    case mcaMaxNodePerUser:

      if (L->UserActivePolicy != NULL)
        {
        mpu_t *P = NULL;
 
        if (MUHTGet(L->UserActivePolicy,"DEFAULT",(void **)&P,NULL) == SUCCESS)
          {
          snprintf(tempConvertBuf,sizeof(tempConvertBuf),
            "%ld,%ld",
            P->SLimit[mptMaxNode][0],
            P->HLimit[mptMaxNode][0]);
          *String += tempConvertBuf;
          }
        }

      break;

    case mcaMaxJobPerUser:

      if (L->UserActivePolicy != NULL)
        {
        mpu_t *P = NULL;
 
        if (MUHTGet(L->UserActivePolicy,"DEFAULT",(void **)&P,NULL) == SUCCESS)
          {
          snprintf(tempConvertBuf,sizeof(tempConvertBuf),
            "%ld,%ld",
            P->SLimit[mptMaxJob][0],
            P->HLimit[mptMaxJob][0]);
          *String += tempConvertBuf;
          }
        }

      break;

    case mcaMaxJobPerGroup:

      if (L->GroupActivePolicy != NULL)
        {
        mpu_t *P = NULL;
 
        if (MUHTGet(L->GroupActivePolicy,"DEFAULT",(void **)&P,NULL) == SUCCESS)
          {
          snprintf(tempConvertBuf,sizeof(tempConvertBuf),
            "%ld,%ld",
            P->SLimit[mptMaxJob][0],
            P->HLimit[mptMaxJob][0]);
          *String += tempConvertBuf;
          }
        }

      break;

    case mcaMaxSubmitJobs:

      if (L->MaxSubmitJobs[0] != 0)
        {
        snprintf(tempConvertBuf,sizeof(tempConvertBuf),
          "%d",
          L->MaxSubmitJobs[0]);
        *String += tempConvertBuf;
        }

      break;

    case mcaMaxWCLimitPerJob:
    case mcaMinWCLimitPerJob:
      {
      char TString[MMAX_LINE];
      mjob_t *J = (AIndex == mcaMaxWCLimitPerJob) ? L->JobMaximums[0] : L->JobMinimums[0];

      if ((J == NULL) ||
        (J->SpecWCLimit[0] == 0))
        {
        break;
        }

      MULToTString(J->SpecWCLimit[0],TString);

      MStringSet(String,TString);
      }

      break;

    case mcaMemberUList:

      {
      /* NYI */
      }  /* END BLOCK */

      break;

    case mcaMessages:
    case mcaComment:

      {
      char tmpBuf[MMAX_BUFFER];

      mmb_t *MB;

      switch (OIndex)
        {
        case mxoAcct:
        case mxoGroup:
        case mxoUser:

          MB = ((mgcred_t *)O)->MB;

          break;

        case mxoQOS:

          MB = ((mqos_t *)O)->MB;

          break;

        case mxoClass:

          MB = ((mclass_t *)O)->MB;
 
          break;

        default:

          MB = NULL;

          break;
        }  /* END switch (OIndex) */

      if (MB == NULL)
        break;

      if (DFormat != mfmXML)
        {
        MMBPrintMessages(
          MB,
          DFormat,
          TRUE,
          bmisset(DFlags,mcmVerbose) ? -1 : 0,
          tmpBuf,
          sizeof(tmpBuf));

        MStringSet(String,tmpBuf);
        }  /* END if (DFormat != mdfXML) */
      else
        {
        mxml_t *E = NULL;

        MXMLCreateE(&E,(char *)MCredAttr[mcaMessages]);

        MMBPrintMessages(
          MB,
          mfmXML,
          TRUE,
          bmisset(DFlags,mcmVerbose) ? -1 : 0,
          (char *)E,
          0);

        MXMLToMString(E,String,NULL,TRUE);

        MXMLDestroyE(&E);
        }
      }   /* END BLOCK (case mcaMessages) */

      break;

    case mcaOverrun:

      /* NYI */

      break;

    case mcaPDef:

      {
      mpar_t *P;

      P = (mpar_t *)F->PDef;

      if ((P != NULL) && (P != &MPar[0]))
        {
        /* P is set and is not default */

        MStringSet(String,P->Name);
        }
      }    /* END BLOCK */

      break;
 
    case mcaPList:

      if (!bmisclear(&F->PAL))
        {
        char ptr[MMAX_LINE];

        MPALToString(&F->PAL,",",ptr);

        if (strcmp(ptr,NONE))
          MStringSet(String,ptr);
        }

      break;

    case mcaPref:

      {
      switch (OIndex)
        {
        case mxoUser:
        case mxoGroup:
        case mxoAcct:

          if (((mgcred_t *)O)->Pref != NULL)
            {
            MStringSet(String,((mgcred_t *)O)->Pref);
            } 

          break;

        default:

          /* NO-OP */

          break;
        }  /* END switch (OIndex) */
      }    /* END BLOCK */

      break;

    case mcaPriority:

      if (F->Priority != 0)
        {
        snprintf(tempConvertBuf,sizeof(tempConvertBuf),
          "%ld",
          F->Priority);
        *String += tempConvertBuf;
        }

      break;

    case mcaQDef:

      if (F->QDef[0] != NULL)
        MStringSet(String,((mqos_t *)F->QDef[0])->Name);

      break;

    case mcaQList:

      {
      char QList[MMAX_LINE];

      MStringSet(String,MQOSBMToString(&F->QAL,QList));
      }

      break;

    case mcaReqRID:

      MStringSet(String,F->ReqRsv);

      break;

    case mcaRole:

      if ((OIndex != mxoUser) || (((mgcred_t *)O)->Role == mcalNONE))
        break;

      MStringSet(String,(char *)MRoleType[((mgcred_t *)O)->Role]);

      break;

    case mcaTrigger:

      switch (OIndex)
        {
        case mxoUser:
        case mxoGroup:
        case mxoAcct:

          if (((mgcred_t *)O)->TList != NULL)
            MTListToMString(((mgcred_t *)O)->TList,TRUE,String);

          break;

        case mxoQOS:

          if (((mqos_t *)O)->TList != NULL)
            MTListToMString(((mqos_t *)O)->TList,TRUE,String);

          break;

        case mxoClass:

          if (((mclass_t *)O)->TList != NULL)
            MTListToMString(((mclass_t *)O)->TList,TRUE,String);

          break;

        default:

          /* NOT HANDLED */

          return(FAILURE);

          /*NOTREACHED*/

          break;
        }  /* END switch (OIndex) */

      break;

    case mcaVariables:

      switch (OIndex)
        {
        case mxoUser:
        case mxoGroup:
        case mxoAcct:

          if (((mgcred_t *)O)->Variables != NULL)
            {
            mln_t *tmpL;

            tmpL = ((mgcred_t *)O)->Variables;

            while (tmpL != NULL)
              {
              MStringAppend(String,tmpL->Name);

              if (tmpL->Ptr != NULL)
                {
                MStringAppendF(String,"=%s;",(char *)tmpL->Ptr);
                }
              else
                {
                MStringAppend(String,";");
                }

              tmpL = tmpL->Next;
              }  /* END while (tmpL != NULL) */
            }    /* END BLOCK mraVariables */

          break;

        case mxoClass:

          {
          mclass_t *tmpC;

          tmpC = (mclass_t *)O;
          
          if (tmpC->Variables != NULL)
            {
            MULLToMString(tmpC->Variables,FALSE,";",String);
            }

          break;
          }

        default:

          /* NOT-SUPPORTED */

          break;
        }  /* END switch (OIndex) */

      break;

    default:

      /* attribute not handled */

      return(FAILURE);

      /*NOTREACHED*/

      break;
    }  /* END switch (AIndex) */

  return(SUCCESS);
  }  /* END MCredAToMString() */

/* END MCredAToString.c */
