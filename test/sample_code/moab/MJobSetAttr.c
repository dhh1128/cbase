/* HEADER */

      
/**
 * @file MJobSetAttr.c
 *
 * Contains:
 *    MJobSetAttr()
 */


#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  
#include "MEventLog.h"


/**
 * Sets the given PAL with a given value.
 *
 * @param J Job containing the passed in PAL so that the EffPAL can get updated.
 * @param ToSetPAL The PAL to set (Ex. SysPAL, SpecPAL).
 * @param Value (I) The value to set the PAL to.
 * @param Format (I) The format that "Value" is in (can be string or other).
 * @param Mode (I) Tells whether Value should replace the job attribute, add to it, remove it, etc. 
 */

void __MJobSetPAL(

  mjob_t                   *J,
  mbitmap_t                *ToSetPAL,
  void                    **Value,  /* I */
  enum MDataFormatEnum      Format, /* I */
  enum MObjectSetModeEnum   Mode)   /* I (mSet,mSetOnEmpty,mUnset,mIncr,mDecr) */

  {
  char ParLine[MMAX_LINE];

  if (Value == NULL)
    {
    /* only NULLs SpecPAL for real jobs, why? */

    MDB(3,fCONFIG) MLog("INFO:     job %s passed in PAL (pmask) being set to ALL since no value supplied\n",
        J->Name);

    bmsetall(ToSetPAL,MMAX_PAR);

    if (bmisset(&J->IFlags,mjifTemporaryJob))
      {
      /* we are being lazy - init for tmp jobs */

      bmsetall(&J->SysPAL,MMAX_PAR);
      bmsetall(&J->PAL,MMAX_PAR);
      }

    return;
    }

  /* NOTE: sets SpecPAL, SysPAL, and PAL */
  
  if (Format == mdfString)
    {
    /* FORMAT:  <ATTR>[{: \t,}<ATTR>]... */

    if (strcasecmp((char *)Value,"ALL") &&
        (strstr((char *)Value,MPar[0].Name) == NULL))  /* ALL */
      {
      mbitmap_t tmpPAL;

      /* non-global pmask specified */

      MPALFromString((char *)Value,mAdd,&tmpPAL);

      /* make the appropriate operation for the requested mode */

      switch(Mode)
        {
        case mAdd:

          bmor(ToSetPAL,&tmpPAL);

          MJobGetPAL(J,&J->SpecPAL,&J->SysPAL,&J->PAL);

          break;

        case mDecr:

          {
          int pindex; 
          mbool_t PartitionDeleted = FALSE;

          /* only clear it if it is currently set */

          for (pindex = 0;pindex < MMAX_PAR;pindex++)
            {
            if (bmisset(&tmpPAL,pindex))
              {
              bmunset(ToSetPAL,pindex);

              PartitionDeleted = TRUE;
              }
            } /* END for pindex */

          if (PartitionDeleted == TRUE)
            {
            MJobGetPAL(J,&J->SpecPAL,&J->SysPAL,&J->PAL);
            }

          } /* END block */

          break;

        default:

          bmcopy(ToSetPAL,&tmpPAL);

          MJobGetPAL(J,&J->SpecPAL,&J->SysPAL,&J->PAL);

          break;
        } /* END switch(MODE) */

      bmclear(&tmpPAL);
      }    /* END if (strstr() == NULL) */
    else
      {
      /* set global pmask */

      bmsetall(ToSetPAL,MMAX_PAR);

      MDB(3,fCONFIG) MLog("INFO:     job %s passed in PAL (pmask) global '%s' (%s) in set attr\n",
          J->Name,
          MPALToString(ToSetPAL,NULL,ParLine),
          (char *)Value);
      }
    }
  else
    {
    /* make the appropriate operation for the requested mode */

    switch(Mode)
      {
      case mAdd:

        bmor(ToSetPAL,(mbitmap_t *)Value);

        MJobGetPAL(J,&J->SpecPAL,&J->SysPAL,&J->PAL);

        break;

      case mDecr:

        {
        mbitmap_t pal;
        int pindex; 
        mbool_t PartitionDeleted = FALSE;

        bmcopy(&pal,(mbitmap_t *)Value);

        /* only clear it if it is currently set */

        for (pindex = 0;pindex < MMAX_PAR;pindex++)
          {
          if (bmisset(&pal,pindex))
            {
            bmunset(ToSetPAL,pindex);

            PartitionDeleted = TRUE;
            }
          } /* END for pindex */

        if (PartitionDeleted == TRUE)
          {
          MJobGetPAL(J,&J->SpecPAL,&J->SysPAL,&J->PAL);
          }

        bmclear(&pal);
        }

        break;

      default:

        bmcopy(ToSetPAL,(mbitmap_t *)Value);

        MJobGetPAL(J,&J->SpecPAL,&J->SysPAL,&J->PAL);

        break;
      } /* END switch(MODE) */
    }

  if (bmisclear(ToSetPAL))
    {
    MDB(1,fSTRUCT) MLog("ALERT:    PAL (pmask) passed in set to empty for job '%s'.  (job cannot run)\n",
      J->Name);
    }

  /* update all PAL's */

  MJobGetPAL(J,&J->SpecPAL,&J->SysPAL,&J->PAL);

  MDB(7,fCONFIG)
    {
    char ParLine[MMAX_LINE];

    MDB(3,fCONFIG) MLog("INFO:     job %s SpecPAL (pmask) '%s' specified\n",
        J->Name,
        MPALToString(&J->SpecPAL,NULL,ParLine));

    MDB(7,fCONFIG) MLog("INFO:     job %s SysPAL partition access list '%s'\n",
        J->Name,
        MPALToString(&J->SysPAL,NULL,ParLine));

    MDB(7,fCONFIG) MLog("INFO:     job %s PAL partition access list set to '%s' in MJobSetAttr\n",
        J->Name,
        MPALToString(&J->PAL,NULL,ParLine));
    }
  } /* END __MJobSetPAL() */



/**
 * Adjusts the give priority to the user priority bounds.
 *
 * @param Priority (I)
 */

long __MJobAdjustUserPriority(
      
  long Priority)

  {
  long tmpL;
  /* NOTE:  only negative job priorities are allowed if EnablePosUserPriority is FALSE */

  if (Priority > 0)
    {
    if (!bmisset(&MPar[0].Flags,mpfEnablePosUserPriority))
      {
      tmpL = 0;
      }
    else
      {
      tmpL = MIN(MDEF_MAXUSERPRIO,Priority);
      }
    }
  else
    {
    tmpL = MAX(MDEF_MINUSERPRIO,Priority);
    }

  return(tmpL);
  } /* END __MJobAdjustUserPriority() */



/**
 * Set an attribute of the specified job.
 * 
 * @see MReqSetAttr() - set attribute in job 'req' substructure
 * @see MJobAToMString() - report job attribute as string
 *
 * NOTE: sync w/MJobDupAttr()
 *
 * @param J (I, Modified) The job to modify.
 * @param AIndex (I) The job attribute to modify.
 * @param Value (I) The value to set the attribute to.
 * @param Format (I) The format that "Value" is in (can be string, double, array, etc.). See mcom.h
 * @param Mode (I) Tells whether Value should replace the job attribute, add to it, remove it, etc. 
 * NOTE: Mode == mSetOnEmpty should map to mSet in most cases - only exceptions are creds,
 *  jobid, drm, etc.
 */

int MJobSetAttr(

  mjob_t                   *J,      /* I (modified) */
  enum MJobAttrEnum         AIndex, /* I */
  void                    **Value,  /* I */
  enum MDataFormatEnum      Format, /* I */
  enum MObjectSetModeEnum   Mode)   /* I (mSet,mSetOnEmpty,mUnset,mIncr,mDecr) */

  {
  mreq_t *RQ;

  const char *FName = "MJobSetAttr";

  MDB(7,fSCHED) MLog("%s(%s,%s,Value,%s,%d)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    MJobAttr[AIndex],
    MDFormat[Format],
    Mode);

  if (J == NULL)
    {
    return(FAILURE);
    }

  RQ = J->Req[0];

  switch (AIndex)
    {
    case mjaAccount:

      /* NOTE:  currently no way to clear account (FIXME) */

      if ((Mode == mSetOnEmpty) && (J->Credential.A != NULL))
        break;

      if (Value != NULL)
        {
        char *ptr;
        char *TokPtr;

        ptr = (char *)Value;

        /* clear account information value is NONE */
        if (!strcmp(ptr,"NONE"))
          {
            if (Mode == mSet)
              {
              J->Credential.A = NULL;

              /* do not attempt to clear an empty CL */
              if (J->Credential.CL == NULL)
                return(SUCCESS);

              if (MACLRemove(&J->Credential.CL,maAcct,NULL) == SUCCESS)
                return(SUCCESS);
              else
                return(FAILURE);
              }
            else
              {
              /* cannot clear information unless the mode is mSet */

              return(FAILURE);
              }
          }

        if (ptr[0] == '\0')
          {
          /* do not allow empty strings */

          return(FAILURE);
          }

        if (bmisset(&MSched.Flags,mschedfNoGridCreds) == FALSE)
          ptr = MUStrTok((char *)Value,"/",&TokPtr);

        if (ptr != NULL)
          {
          if ((Mode == mIncr) && (MAcctFind(ptr,&J->Credential.A) == FAILURE))
            {
            MDB(1,fSCHED) MLog("ERROR:    cannot add account for job %s (Name: %s)\n",
              J->Name,
              ptr);
 
            return(FAILURE);
            }
          else if (MAcctAdd(ptr,&J->Credential.A) == FAILURE)
            {
            MDB(1,fSCHED) MLog("ERROR:    cannot add account for job %s (Name: %s)\n",
              J->Name,
              ptr);
 
            return(FAILURE);
            }

          if (!MACLIsEmpty(J->Credential.CL))
            {
            /* update job cred list */

            MACLSet(&J->Credential.CL,maAcct,(void *)J->Credential.A->Name,mcmpSEQ,mnmNeutralAffinity,0,FALSE);
            }
          }  /* END if (ptr != NULL) */

        if (!bmisset(&MSched.Flags,mschedfNoGridCreds))
          {
          ptr = MUStrTok(NULL," \t\n",&TokPtr);

          if (ptr != NULL)
            {
            if (J->Grid == NULL)
              J->Grid = (mjgrid_t *)MUCalloc(1,sizeof(mjgrid_t));

            if (J->Grid != NULL)
              MUStrDup(&J->Grid->Account,ptr);
            }
          }
        }   /* END if (Value != NULL) */

      break;

    case mjaAllocNodeList:

      /* WARNING: does NOT modify J->Req[X]->NodeList */

      {
      int  *TaskList = NULL;
      int   tindex;

      mnode_t *N;

      /* assign allocated node to job */

      if (Format == mdfString)
        {
        char *ptr;
        char *ptr2;
        char *TokPtr = NULL;
        char *TokPtr2 = NULL;
        int   TC;

        /* FORMAT:  <NODEID>[:<TASKCOUNT>][,<NODEID>[:<TASKCOUNT>]]... */

        tindex = 0;

        ptr = MUStrTok((char *)Value,",",&TokPtr);

        TaskList = (int *)MUMalloc(sizeof(int) * (MSched.JobMaxTaskCount + 1));

        while (ptr != NULL)
          {
          TC = 1;

          ptr2 = MUStrTok(ptr,":",&TokPtr2); /* <NODEID> */

          if (MNodeFind(ptr2,&N) == FAILURE)
            {
            ptr = MUStrTok(NULL,",",&TokPtr);

            continue;
            }

          ptr2 = MUStrTok(NULL,":",&TokPtr2); /* <TASKCOUNT> */

          if (ptr2 != NULL)
            {
            TC = strtol(ptr2,NULL,10);
            }

          for (;TC > 0;TC--)
            {
            TaskList[tindex] = N->Index;

            tindex++;
            }

          if (tindex >= MSched.JobMaxTaskCount)
            break;

          ptr = MUStrTok(NULL,",",&TokPtr);
          }  /* END while (ptr != NULL) */

        TaskList[tindex] = -1;
        }  /* END if (Format == mdfString) */
      else if (Format == mdfOther)
        {
        mnl_t *NL = (mnl_t *)Value;

        if ((Mode == mIncr) || (Mode == mAdd))
          {
          MNLMerge(&J->NodeList,&J->NodeList,NL,NULL,NULL);
          }
        else if (Mode == mDecr)
          {
          /* NYI */

          return(FAILURE);
          }
        else
          {
          /* assume mSet */

          MNLCopy(&J->NodeList,NL);
          }

        /* NOTE:  mdfOther does not adjust J->TaskList */

        J->Alloc.NC = MNLCount(&J->NodeList);

        return(SUCCESS);
        }

#if 0 /* Why do we need this?  What path is there that could possibly get here with NULL Req[0]? */

      if ((J->Req[0] == NULL) && (MReqCreate(J,NULL,NULL,FALSE) == FAILURE))
        {
        MUFree((char **)&TaskList);

        return(FAILURE);
        }
#endif

      MJobSetAllocTaskList(J,NULL,TaskList);

      MUFree((char **)&TaskList);
      }  /* END BLOCK (case mjaAllocNodeList) */

      break;

    case mjaAllocVMList:

      if (Format != mdfString)
        {
        return(FAILURE);
        }

      break;

    case mjaArgs:

      MUStrDup(&J->Env.Args,(char *)Value);

      break;

    case mjaAWDuration:

      {
      long tmpL;
      long ComputedDrift = 0;

      if (Value == NULL)
        break;

      if (Format == mdfString)
        {
        tmpL = MUTimeFromString((char *)Value);
        }
      else
        {
        tmpL = *(long *)Value;
        }

      J->AWallTime = tmpL;

      if ((J->StartTime > 0) && (J->AWallTime > 0))
        {
        if (J->CompletionTime > 0)
          {
          ComputedDrift = J->CompletionTime - J->StartTime - J->SWallTime - J->AWallTime;
          }
        else
          {
          ComputedDrift = MSched.Time - J->StartTime - J->SWallTime - J->AWallTime;
          }
        }

      MDB(7,fSCHED) MLog("INFO:     job %s setting walltime to %ld, computed drift %ld in function %s\n",
        J->Name,
        J->AWallTime,
        ComputedDrift,
        __FUNCTION__);
      }  /* END BLOCK */

      break;

    case mjaCompletionCode:

      if (Format == mdfString)
        {
        /* (@see MJobAToMString)
           The completion code can be output in one of 3 forms...
           
           CNCLD(%s) -- job is canceled and return code is non-zero
           CNCLD     -- job is canceled and return code is zero
           %d        -- bare return code.
            
        */
        if (!strncmp((char *)Value,"CNCLD(",strlen("CNCLD(")))
          {
          bmset(&J->IFlags,mjifWasCanceled);
          J->CompletionCode = (int)strtol((char *)Value+strlen("CNCLD("),NULL,10);
          }
        else if (!strncmp((char *)Value,"CNCLD",strlen("CNCLD")))
          {
          bmset(&J->IFlags,mjifWasCanceled);
          J->CompletionCode = 0;
          }
        else
          {
          J->CompletionCode = (int)strtol((char *)Value,NULL,10);
          }
        }
      else if (Format == mdfInt)
        J->CompletionCode = *(int *)Value;
      else if (Format == mdfLong)
        J->CompletionCode = (int)(*(long *)Value);

      break;

    case mjaClass:

      /* NOTE:  currently no way to clear class/queue (FIXME) */

      /* FIXME: should this case call MJobSetClass()? */

      if ((Mode == mSetOnEmpty) && (J->Credential.C != NULL))
        break;

      if (Value != NULL)
        {
        char *ptr;
        char *TokPtr;

        ptr = MUStrTok((char *)Value,"/",&TokPtr);

        if (ptr != NULL)
          {
          if (MClassAdd(ptr,&J->Credential.C) == FAILURE)
            {
            MDB(1,fSCHED) MLog("ERROR:    cannot add class for job %s (Name: %s)\n",
              J->Name,
              ptr);

            return(FAILURE);
            }

          if (!MACLIsEmpty(J->Credential.CL))
            {
            /* update ACL */

            MACLSet(&J->Credential.CL,maClass,(void *)J->Credential.C->Name,mcmpSEQ,mnmNeutralAffinity,0,FALSE);
            }
          }    /* END if (ptr != NULL) */

        ptr = MUStrTok(NULL," \t\n",&TokPtr);

        if (ptr != NULL)
          {
          if (J->Grid == NULL)
            J->Grid = (mjgrid_t *)MUCalloc(1,sizeof(mjgrid_t));

          if (J->Grid != NULL)
            MUStrDup(&J->Grid->Class,ptr);
          }
        }   /* END if (Value != NULL) */

      break;

    case mjaCmdFile:

      if (Value != NULL)
        MUStrDup(&J->Env.Cmd,(char *)Value);

      break;

    case mjaCompletionTime:

      {
      long tmpL;

      if (Value == NULL)
        break;

      if (Format == mdfString)
        {
        sscanf((char *)Value,"%ld",
          &tmpL);
        }
      else
        {
        tmpL = *(long *)Value;
        }

      J->CompletionTime = tmpL;
      }  /* END BLOCK */

    case mjaCost:

      if (Value == NULL)
        break;

      if (Format == mdfString)
        {
        sscanf((char *)Value,"%lf",
          &J->Cost);
        }
      else
        {
        J->Cost = *(double *)Value;
        }

      break;

    case mjaCPULimit:

      if (Value == NULL)
        break;

      if (Format == mdfString)
        {
        J->CPULimit = MUTimeFromString((char *)Value);
        }
      else 
        {
        J->CPULimit = *(long *)Value;
        }

      break;

    case mjaDepend:

      if (Format != mdfString)
        {
        return(FAILURE);
        }

      {
      enum  MDependEnum Type;
      char  DepValue[MMAX_NAME + 1];
      char  DepSValue[MMAX_NAME + 1];
      char *tmpValue;
      char *tmpSValue;
      char *ptr;
      char *TokPtr;
      char *String = NULL;

      char  tmpType[MMAX_NAME + 1];

      MUStrDup(&String,(const char *)Value);

      ptr = MUStrTok(String,",",&TokPtr);

      /* FORMAT:  <TYPE> <DEPVALUE> [<SVALUE>][,<TYPE> <DEPVALUE> [<SVALUE>]]... */
      /*   or     <DEPVALUE> */

      while (ptr != NULL)
        {
        if (strchr(ptr,' ') == NULL)
          {
          MUStrCpy(DepValue,ptr,sizeof(DepValue));
          MUStrCpy(DepSValue,ptr,sizeof(DepSValue));

          Type = mdJobCompletion;
          }
        else
          {
          int rc;

          rc = sscanf(ptr,"%64s %64s %64s",
            tmpType,
            DepValue,
            DepSValue);

          if (rc == 2)
            MUStrCpy(DepSValue,DepValue,sizeof(DepSValue));

          if (isdigit(tmpType[0]))
            Type = (enum MDependEnum)strtol(tmpType,NULL,10);
          else
            Type = (enum MDependEnum)MUGetIndexCI(tmpType,MDependType,FALSE,0);
          }

        tmpSValue = DepSValue;

        tmpValue = DepValue;

        if (!strcmp(DepSValue,"NULL"))
          {
          tmpSValue = NULL;
          }

        if (!strcmp(DepValue,"NULL"))
          {
          tmpValue = NULL;
          }

        MJobSetDependency(J,Type,tmpValue,tmpSValue,0,FALSE,NULL);

        ptr = MUStrTok(NULL,",",&TokPtr);
        }  /* END while (ptr != NULL) */
      
      MUFree(&String);
      }    /* END BLOCK (case mjaDepend) */

      break;

    case mjaDescription:

      MUStrDup(&J->Description,(char *)Value); 

      break;

    case mjaDRM:

      /* NOTE:  DRM is resource manager for primary compute req only */

      {
      int pindex;
      int rqindex;

      if (Format == mdfOther)
        {
        J->DestinationRM = (mrm_t *)Value;
        }
      else if (Format == mdfString)
        {
        mrm_t *tRM;

        if (MRMFind((char *)Value,&tRM) == FAILURE)
          {
          MDB(5,fSCHED) MLog("ERROR:    cannot set DRM for job %s (RM: %s)\n",
            J->Name,
            (char *)Value);

          return(FAILURE);
          }

        J->DestinationRM = tRM;
        }      

      if (!bmisset(&J->Flags,mjfCoAlloc))
        {
        mreq_t *RQ;

        /* NOTE:  co alloc jobs have req rmindex handled elsewhere */

        for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
          {
          RQ = J->Req[rqindex];

          if (RQ == NULL)
            break;

          if (J->DestinationRM != NULL)
            {
            /* NOTE:  must handle extended resource allocation within shared
                      partition */

            if (!MNLIsEmpty(&RQ->NodeList))
              {
              mnode_t *N;

              MNLGetNodeAtIndex(&RQ->NodeList,0,&N);

              if ((N->RM != NULL) && 
                 (bmisset(&N->RM->RTypes,mrmrtStorage) ||
                  bmisset(&N->RM->RTypes,mrmrtNetwork) ||
                  bmisset(&N->RM->RTypes,mrmrtLicense)))
                {
                /* req has extended resource allocated */

                /* do not modify RM index for extended resource req */

                continue;
                }
              }

            RQ->RMIndex = J->DestinationRM->Index;
            }
          else if (J->SubmitRM != NULL)
            {
            RQ->RMIndex = J->SubmitRM->Index;
            }
          else
            {          
            /* NOTE: should set to invalid value (i.e. -1) - not yet supported */

            RQ->RMIndex = 0;
            }
          }    /* END for (rqindex) */
        }      /* END if (!bmisset(&J->Flags,mjfCoAlloc)) */
     
      if ((J->TemplateExtensions == NULL) || (J->TemplateExtensions->WorkloadRMID == NULL))
        {
        mbitmap_t tmpPAL;
        mpar_t *P;
        char ParLine[MMAX_LINE];

        switch (J->VMUsagePolicy)
          {
          case mvmupPMOnly:

            /* NOTE:  constrain job to only access rm partitions */

            if (J->DestinationRM == NULL)
              {
              /* restore to original PAL */

              bmor(&J->SysPAL,&J->SpecPAL);

              break;
              }

            /* allow access to global partition */

            bmset(&tmpPAL,0);

            for (pindex = 1;pindex < MMAX_PAR;pindex++)
              {
              P = &MPar[pindex];

              if (P->Name[0] == '\1')
                continue;

              if (P->Name[0] == '\0')
                break;

              if (P->RM != NULL)
                {
                if (P->RM == J->DestinationRM)
                  {
                  bmset(&tmpPAL,pindex);
                  }
                }
              }   /* END for (pindex) */

            /* allow job to only run on AND of previous PAL constraints 
               and partitions available to RM */

            /* adjusting J->PAL may not be necessary, but should not cause problems */


            MDB(7,fSCHED) MLog("INFO:     job %s tmpPAL partition access list %s\n", 
              J->Name,
              MPALToString(&tmpPAL,NULL,ParLine));

            bmand(&J->PAL,&tmpPAL);  

            MDB(7,fSCHED) MLog("INFO:     job %s partition access list set to %s in MJobSetAttr\n", 
              J->Name,
              MPALToString(&J->PAL,NULL,ParLine));

            break;

          default:

            /* evil!!!  assume VM will make everything ok! (HACK) */

            /* NO-OP */

            break;
          }  /* END switch */

        bmclear(&tmpPAL);
        }    /* END if ((J->TX == NULL) || (J->TX->WorkloadRMID == NULL)) */
      else
        {
        /* job is dynamic service job with private workload RM - do not adjust PAL */

        /* NO-OP */
        }
      }    /* END BLOCK (case mjaDRM) */

      break;

    case mjaDRMJID:

      if (!bmisset(&J->IFlags,mjifTemporaryJob))
        {
        if (J->DRMJID != NULL)
          MUHTRemove(&MJobDRMJIDHT,J->DRMJID,NULL);

        MUHTAdd(&MJobDRMJIDHT,(char *)Value,J,NULL,NULL);
        }

      MUStrDup(&J->DRMJID,(char *)Value);

      if ((J->DRMJID != NULL) && (strcmp(J->Name,J->DRMJID)))
        MSched.UseDRMJID = TRUE;

      break;

    case mjaEFile:

      /* this is not E.Error because MJobToExportXML()
       * sets mjaEFile to RMError ... */

      MUStrDup(&J->Env.RMError,(char *)Value);

      break;

    case mjaEstWCTime:

      if (Value == NULL)
        break;

      if (Format == mdfString)
        {
        sscanf((char *)Value,"%lu",
          &J->EstWCTime);
        }
      else
        {
        J->EstWCTime = *(long *)Value;
        }

      break;

    case mjaEEWDuration:

      if (Value == NULL)
        break;

      if (Format == mdfString)
        {
        sscanf((char *)Value,"%ld",
          &J->EffQueueDuration);
        }
      else
        {
        J->EffQueueDuration = *(long *)Value;
        
        MDB(7,fSCHED) MLog("INFO:    (JobSetAttr) job %s new EffqueueDuration %ld\n", /* BRIAN - debug (2400) */
            J->Name,
            J->EffQueueDuration);
        }

      break;

    case mjaEnv:

      if (Mode == mIncr)
        {
        MUStrAppend(&J->Env.BaseEnv,NULL,(char *)Value,ENVRS_ENCODED_CHAR);
        }
      else
        {
        /* Initializing to strlen(Value) because unpacked and replaced strings
         * should be smaller equal or smaller than the original Value. */
        mstring_t tmpEnvString(strlen((char*)Value) + 1);
        mstring_t unpackedString(strlen((char*)Value) + 1);

        /* Env string is checkpointed packed and with ENVRS_ENCODED_CHAR (record separator) 
         * converted to ENVRS_SYMBOLIC_STR to handle the case of un-printable characters 
         * (ex.'\n', ENVRS_ENCODED_CHAR) in the environment. Because MU[M]StringPack converts
         * ';'s in ENVRS_SYMBOLIC_STR to ~rs\3b, the ';' must be first restored and then
         * ENVRS_SYMBOLIC_STR must be converted back to ENVRS_ENCODED_CHAR. */

        MUStringUnpackToMString((char *)Value,&unpackedString);
        MStringReplaceStr(unpackedString.c_str(),ENVRS_SYMBOLIC_STR,ENVRS_ENCODED_STR,&tmpEnvString);
        MUStrDup(&J->Env.BaseEnv,tmpEnvString.c_str());
        }

      MDB(7,fSCHED) MLog("INFO:     job %s environment set to %s using mode %d\n",
        J->Name,
        J->Env.BaseEnv,
        Mode);

      bmset(&J->IFlags,mjifEnvSpecified);

      break;

    case mjaEnvOverride:

      if (Mode == mIncr)
        {
        MUStrAppend(&J->Env.IncrEnv,NULL,(char *)Value,ENVRS_ENCODED_CHAR);
        }
      else
        {
        MUStrDup(&J->Env.IncrEnv,(char *)Value);
        }

      break;

    case mjaExcHList:

      {
      int nindex;

      mnode_t *N;

      mnl_t     NodeList;       /* node range list */

      if (Value == NULL)
        {
        return(FAILURE);
        }

      if (MSched.DisableExcHList == TRUE)
        {
        MDB(7,fSCHED) MLog("INFO:     Exclude Host List attribute not set for job %s (disabled in config)\n",
          J->Name);

        return(SUCCESS);
        }

      MNLInit(&NodeList);

      switch (Format)
        {
        case mdfString:

          if (MUStringIsRange((char *)Value))
            {
            if (MUNLFromRangeString(
                (char *)Value,     /* I - node range expression, (modified) */
                ",:",              /* I - delimiters */
                &NodeList,         /* O - nodelist */
                NULL,              /* O - nodeid list */
                0,                 /* I */
                0,                 /* I */
                FALSE,             /* I - create nodes */
                FALSE) == FAILURE) /* I - Is Bluegene */
              {
              MDB(1,fNAT) MLog("ALERT:    cannot expand exclude nodes list '%s'\n",
                (char *)Value);

              MNLFree(&NodeList);

              return(FAILURE);
              }
            }
          else
            {
            MUStrReplaceChar((char *)Value,':',',',FALSE);

            if (MNLFromString(
                (char *)Value,
                &NodeList,
                -1,
                FALSE) == FAILURE)
              {
              MDB(1,fNAT) MLog("ALERT:    cannot expand exclude nodes list '%s'\n",
                (char *)Value);

              MNLFree(&NodeList);

              return(FAILURE);
              }
            }

          break;

        default:

          MNLSetNodeAtIndex(&NodeList,0,(mnode_t *)Value);
          MNLTerminateAtIndex(&NodeList,1);

          break;
        }  /* END switch (Format) */

      /* initialize hostlist */

      if (Mode == mSet)
        MNLClear(&J->ExcHList);

      for (nindex = 0;MNLGetNodeAtIndex(&NodeList,nindex,&N) == SUCCESS;nindex++)
        {
        MNLAddNode(&J->ExcHList,N,1,TRUE,NULL);
        } /* END for (nindex) */

      MNLFree(&NodeList);
      } /* END BLOCK */

      break;

    case mjaFlags:

      {
      mbitmap_t tmpL;  /* MJobFlagEnum */

      /* WARNING:  may overwrite flags set thru alternate methods */

      if (Format == mdfLong)
        {
        MDB(0,fSTRUCT) MLog("ERROR:   attempting to set job flags from invalid source\n");

        return(FAILURE);
        }
      else
        {
        char *ResPtr = NULL;

        /* check to see if there is a reservation in the flag string */
        /* For example ADVRES:system.9,RESTARTABLE,PREEMPTOR,.... */

        /* We save the reservation name in the J->PeerReqRID so that it may be reported
         * back to a peer (in a grid environment) in a workload query.
         * The peer may have lost this information due to a moab restart. */

        ResPtr = MUStrStr((char *)Value,"ADVRES:",0,FALSE,FALSE);

        if ((ResPtr != NULL) && (Mode != mUnset) && (Mode != mDecr))
          {
          char ResID[MMAX_LINE];
          char *tmpPtr;

          /* Get a copy of the reservation name */

          ResPtr += strlen("ADVRES:");
          MUStrCpy(ResID,ResPtr,MMAX_LINE);

          if ((tmpPtr = strchr(ResID,',')) != NULL)
            {
            /* Null terminate the name at the comma to get rid of the trailing flag info
             * to leave just the reservation name */

            *tmpPtr = '\0';
            }

          /* Save the reservation name as the PeerReqRID in the job. */

          MUStrDup(&J->PeerReqRID,ResID);

          if (J->ReqRID == NULL)
            MUStrDup(&J->ReqRID,ResID);
          }

        bmfromstring((char *)Value,MJobFlags,&tmpL,",:");
        }

      if ((Value != NULL) && (((char *)Value)[0] != '\0') && (bmisclear(&tmpL)))
        {
        return(FAILURE);
        }

      if ((Mode == mSet) || (Mode == mSetOnEmpty))
        {
        bmcopy(&J->SpecFlags,&tmpL);
        }
      else if ((Mode == mAdd) || (Mode == mIncr))
        {
        bmor(&J->SpecFlags,&tmpL);
        }
      else if ((Mode == mDecr) || (Mode == mUnset))
        {
        int index;

        for (index = 0;index < mjfLAST;index++)
          {
          if (bmisset(&tmpL,index))
            bmunset(&J->SpecFlags,index);
          }  /* END for (index) */
        }
      }  /* END BLOCK */

      break;

    case mjaGeometry:

      MUStrDup(&J->Geometry,(char *)Value);

      break;

    case mjaGAttr:

      {
      char  tmpLine[MMAX_LINE];

      char *ptr;
      char *TokPtr;

      int index;

      /* FORMAT:  <ATTR>[{,=:}<VALUE>] */    

      /* sometimes constants are passed in */

      MUStrCpy(tmpLine,(char *)Value,sizeof(tmpLine));

      ptr = MUStrTok(tmpLine,",=:",&TokPtr);

      while (ptr != NULL)
        {
        if ((index = MUMAGetIndex(meJFeature,ptr,mAdd)) > 0)
          {
          if ((Mode == mSet) || (Mode == mIncr) || (Mode == mSetOnEmpty))
            {
            bmset(&J->AttrBM,index);

            MDB(5,fSCHED) MLog("INFO:     attribute '%s' set for job %s\n",
              ptr,
              J->Name);
            }
          else
            {
            /* unset */

            if (bmisset(&J->AttrBM,index))
              bmunset(&J->AttrBM,index);

            MDB(8,fSCHED) MLog("INFO:     attribute '%s' cleared for job %s\n",
              ptr,
              J->Name);
            }
          }

        ptr = MUStrTok(NULL,",=:",&TokPtr);
        }    /* END while (ptr != NULL) */
   
      /* NOTE:  <VALUE> not yet handled */
      }  /* END BLOCK (case mjaGAttr) */

      break;

    case mjaImmediateDepend:

      {
      char DepType[MMAX_NAME];
      char *ptr;
      char *TokPtr;

      ptr = MUStrTok((char *)Value,",",&TokPtr);

      while (ptr != NULL)
        {
        if (strchr(ptr,' ') == NULL)
          {
          MULLAdd(&J->ImmediateDepend,ptr,(void *)strdup(MDependType[mdJobCompletion]),NULL,MUFREE);
          }
        else
          {
          char *ptr2;
          char *TokPtr2;

          ptr2 = MUStrTok(ptr," ",&TokPtr2);
          MUStrCpy(DepType,ptr2,sizeof(DepType));

          ptr2 = MUStrTok(NULL," ",&TokPtr2);

          if (ptr2 != NULL)
            {
            MULLAdd(&J->ImmediateDepend,ptr2,(void *)strdup(DepType),NULL,MUFREE);
            }
          }

        ptr = MUStrTok(NULL,",",&TokPtr);
        } /* END while (ptr != NULL) */
      } /* END BLOCK (case mjaImmediateDepend) */

      break;

    case mjaLogLevel:

      J->LogLevel = (int)strtol((char *)Value,NULL,10);

      break;

    case mjaShell:

      if (Value != NULL)
        {
        MUStrDup(&J->Env.Shell,(char *)Value);
        }

      break;

    case mjaSID:

      if (Value == NULL)
        {
        /* clear existing system id (NYI) */

        break;
        }

      if (J->SystemID != NULL)
        {
        /* system id already processed (NOTE: should recheck - NYI) */

        break;
        }

      if (J->Grid == NULL)
        {
        J->Grid = (mjgrid_t *)MUCalloc(1,sizeof(mjgrid_t));
        }

      if (J->Grid != NULL)
        {
        int index;

        char *ptr;
        char *TokPtr;

        /* FORMAT:  <SID>[,<SID>]... */
      
        ptr = MUStrTok((char *)Value,", \t\n/",&TokPtr);

        MUStrDup(&J->SystemID,ptr);

        if (MRMFindByPeer(J->SystemID,&J->SystemRM,TRUE) == FAILURE)
          {
          MDB(2,fSCHED) MLog("WARNING:  cannot find client peer for job %s (Name: %s)\n",
            J->Name,
            J->SystemID);
          }

        index = 0;

        while (ptr != NULL)
          {
          MUStrDup(&J->Grid->SID[index],ptr);

          index++;
          ptr = MUStrTok(NULL,", \t\n/",&TokPtr);
          }  /* END while (ptr != NULL) */
        }    /* END if (J->G != NULL) */

      break;

    case mjaSize:

      {
      if (RQ == NULL)
        {
        MDB(3,fSCHED) MLog("ERROR:    Req must be alloced before size is set.");

        break;
        }

      J->Size = strtol((char *)Value,NULL,10);

      /* For the XT to use msub, moab needs to natively understand what size means 
       * so that moab can know of the job's size without having to migrate it to 
       * torque and have the job.query.xt#.pl script report back the actual size.
       * Not doing this causes problems when having to use JOBMIGRATEPOLICY IMMEDIATE.
       * job.query.xt#.pl translates size to tasks if > 0 and tasks=1, dprocs=0 if 
       * size == 0. The IFlags mjifDProcsSpecified and mjifTasksSpecified have to 
       * be set to have size 0 jobs print out correctly in showq. The same logic happens
       * below when the job is reported through the wiki. */

      if (J->Size == 0.0)
        {
        RQ->DRes.Procs = 0;
        bmset(&J->IFlags,mjifDProcsSpecified);
        MWikiJobUpdateAttr("DGRES=master:1",J,J->Req[0],NULL,NULL);
        }
      else
        {
        J->Request.TC = J->Size;
        RQ->TaskCount = J->Size;

        RQ->TaskRequestList[0] = J->Size;
        RQ->TaskRequestList[1] = J->Size;      
        RQ->TaskRequestList[2] = 0;
        }
        
      bmset(&J->IFlags,mjifTasksSpecified);
      }

      break;

    case mjaStorage:

      if (strstr((char *)Value,"destroystorage") != NULL)
        bmset(&J->IFlags,mjifDestroyDynamicStorage);

      break;

    case mjaServiceJob:
   
      {
      mbool_t ServiceJob = MUBoolFromString((char *)Value,FALSE);

      if (ServiceJob == TRUE)
        {
        bmset(&J->IFlags,mjifServiceJob);
        }
      }    /* END case mjaServiceJob */
        
      break;

    case mjaGJID:

      if (Value == NULL)
        {
        /* clear existing system JID (NYI) */

        break;
        }

      if (J->SystemJID != NULL)
        {
        /* system JID already processed (NOTE: should recheck - NYI) */

        break;
        }

      if (J->Grid == NULL)
        {
        J->Grid = (mjgrid_t *)MUCalloc(1,sizeof(mjgrid_t));
        }

      if (J->Grid != NULL)
        {
        int index;

        char *ptr;
        char *TokPtr;

        /* FORMAT:  <SJID>[,<SJID>]... */

        ptr = MUStrTok((char *)Value,", \t\n/",&TokPtr);

        MUStrDup(&J->SystemJID,ptr);

        if (MSched.UseMoabJobID == TRUE)
          {
          /* Interactive jobs can be submitted with SJID and SID so that when
           * the job is discovered by moab the moab job id will used instead 
           * of the rm job id. Renaming the job regardless of the SystemID,
           * because we can't guarantee the SystemID will be set at this 
           * point, should be fine since the job on the peer will be the 
           * SJID/GJID anyways. */

          MJobRename(J,ptr);

          /* UseDRMJID must be set for job to found in extended MJobFind */

          MSched.UseDRMJID = TRUE; 
          }

        index = 0;

        while (ptr != NULL)
          {
          MUStrDup(&J->Grid->SJID[index],ptr);

          index++;

          ptr = MUStrTok(NULL,", \t\n/",&TokPtr);
          }  /* END while (ptr != NULL) */
        }    /* END if (J->G != NULL) */

      break;

    case mjaGroup:

      /* NOTE:  currently no way to clear group (FIXME) */

      if ((Mode == mSetOnEmpty) && (J->Credential.G != NULL))
        break;

      if (Value != NULL)
        {
        char *ptr;
        char *TokPtr;

        ptr = MUStrTok((char *)Value,"/",&TokPtr);

        if (ptr != NULL)
          {
          /* NOTE: proper functionality requires that user must be set before group */

          if (!strcasecmp(ptr,"%DEFAULT"))
            {
            if (J->Credential.U == NULL)
              {
              return(FAILURE);
              }
    
            if (J->Credential.U->F.GDef == NULL)
              {
              return(FAILURE);
              }

            ptr = ((mgcred_t *)J->Credential.U->F.GDef)->Name;
            }

          if (MGroupAdd(ptr,&J->Credential.G) == FAILURE)
            {
            MDB(1,fSCHED) MLog("ERROR:    cannot add group for job %s (Name: %s)\n",
              J->Name,
              ptr);

            return(FAILURE);
            }

          if (!MACLIsEmpty(J->Credential.CL))
            {
            /* update job cred list */

            MACLSet(&J->Credential.CL,maGroup,(void *)J->Credential.G->Name,mcmpSEQ,mnmNeutralAffinity,0,FALSE);
            }
          }    /* END if (ptr != NULL) */

        ptr = MUStrTok(NULL," \t\n",&TokPtr);

        if (ptr != NULL)
          {
          if (J->Grid == NULL)
            J->Grid = (mjgrid_t *)MUCalloc(1,sizeof(mjgrid_t));

          if (J->Grid != NULL)
            MUStrDup(&J->Grid->Group,ptr);
          }
        }   /* END if (Value != NULL) */

      break;

    case mjaHold:

      {
      mbitmap_t tmpH;
      mbitmap_t oldHold;

      if (Value == NULL)
        break;

      bmcopy(&oldHold,&J->Hold);

      if (Format == mdfString)
        {
        if (*(char *)Value == '\0')
          {
          /* empty hold list specified, clear all holds */

          bmset(&tmpH,mhAll);

          Mode = mUnset;
          }
        else if (!isdigit(((char *)Value)[0]))
          {
          mbitmap_t tmpL;

          bmfromstring((char *)Value,MHoldType,&tmpL);

          bmcopy(&tmpH,&tmpL);
          }
        }
      else if (Format == mdfOther)
        {
        bmcopy(&tmpH,(mbitmap_t *)Value);
        }

      if (bmisset(&tmpH,mhAll))
        {
        bmclear(&tmpH);
   
        bmset(&tmpH,mhBatch);
        bmset(&tmpH,mhUser);
        bmset(&tmpH,mhSystem);
        bmset(&tmpH,mhDefer);
        }

      if (Mode != mUnset)
        {
        if (bmisset(&tmpH,mhBatch) && (MSched.DisableBatchHolds == TRUE))
          bmset(&tmpH,mhBatch);
        }

      if (Mode == mUnset)
        {
        int hindex;

        for (hindex = mhUser;hindex <= mhDefer;hindex++)
          {
          if (bmisset(&J->Hold,hindex) && bmisset(&tmpH,hindex))
            bmunset(&J->Hold,hindex);
          }  /* END for (hindex) */

        /* If there are no holds on the job and it's in a hold state set to Idle. 
        * A job can still be in a hold state until an update from the rm, even 
        * after all holds have been released. This is the case with a grid -- a
        * job will display in showq with a hold until the job is updated.  */

        if ((bmisclear(&J->Hold)) && (J->State == mjsHold))
          MJobSetState(J,mjsIdle);
        }
      else if (Mode == mAdd)
        {
        bmor(&J->Hold,&tmpH);
        }
      else
        {
        bmcopy(&J->Hold,&tmpH);
        }

      /* Send change to a remote peer, if one exists, AND if we aren't
       * getting this change from the remote peer itself (RMAOBM). MMJobLoad
       * and MMJobUpdate set owner before calling MJobFromXML and unsets
       * it after the call.*/

      if ((bmisset(&J->IFlags,mjifRMOwnsHold) == FALSE) &&
          (J->DestinationRM != NULL) &&
          (J->DestinationRM->Type == mrmtMoab))
        {
        char tEMsg[MMAX_LINE];
        char BMString[MMAX_LINE];

        if (MRMJobModify(
             J,
             (char *)MJobAttr[mjaHold],
             NULL,
             bmtostring(&tmpH,MHoldType,BMString),
             Mode,
             NULL,
             tEMsg,
             NULL) == FAILURE)
          {
          MDB(2,fSCHED) MLog("INFO:     cannot adjust holds on remote peer for job %s - '%s'\n",
            J->Name,
            tEMsg);

          return(FAILURE);
          }
        }

#ifdef MYAHOO
      if (bmisset(&J->Flags,mjfNoResources) &&
          (J->Hold != 0))
        {
        /* jobs w/ no resources should only be "Idle" if held */

        MJobToIdle(J,FALSE,TRUE);
        }
#endif /* MYAHOO */

      if ((J->EState == mjsDeferred) && !bmisset(&J->Hold,mhDefer))
        J->EState = J->State;

#ifdef MYAHOO
      if (J->Hold != 0)
        {
        /* if job is held, disable triggers */

        MTrigDisable(J->Triggers);
        }
      else
        {
        /* if held is removed, re-enabled triggers */

        MTrigEnable(J->Triggers);
        }
#endif /* MYAHOO */

      if ((!bmisclear(&oldHold)) &&
          (bmisclear(&J->Hold)))
        {
        /* Holds have been cleared, send jobrelease event */

        CreateAndSendEventLog(meltJobRelease,J->Name,mxoJob,(void *)J);
        }
      }    /* END BLOCK */

      break;

    case mjaHoldTime:

      if (Format == mdfString)
        {
        J->HoldTime = strtol((char *)Value,NULL,10);
        }
      else
        {
        J->HoldTime = *(int *)Value;
        }

      break;

    case mjaHopCount:

      if ((J->Grid == NULL) &&
         ((J->Grid = (mjgrid_t *)MUCalloc(1,sizeof(mjgrid_t))) == NULL))
        {
        break;
        }

      if (Value == NULL)
        {
        /* clear hop count */        
        
        J->Grid->HopCount = 0;

        break;
        }

      if (Format == mdfString)
        {
        J->Grid->HopCount = (int)strtol((char *)Value,NULL,10);
        }
      else if (Format == mdfInt)
        {
        J->Grid->HopCount = *(int *)Value;
        }

      break;

    case mjaReqHostList:

      /* NOTE:  no support for host taskcount specification */

      {
      int hindex;

      mnode_t *N = NULL;

      char *ptr;
      char *TokPtr;

      char *tmpBuf = NULL;

      if (Value == NULL)
        {
        return(FAILURE);
        }

      J->HLIndex = 0;  /* assign hostlist to primary req */

      switch (Format)
        {
        case mdfString:

          ptr = (char *)Value;

          if (ptr[0] == '\0')
            {
            /* unset existing hostlist */

            bmunset(&J->IFlags,mjifHostList);

            MNLFree(&J->ReqHList);

            break;
            }

          bmset(&J->IFlags,mjifHostList);

          /* FORMAT:  <HOST>[{ \t\n,:}<HOST>]... */

          MUStrDup(&tmpBuf,(char *)Value);

          ptr = MUStrTok(tmpBuf," \t\n,:+",&TokPtr);

          if (((MSched.IsServer == TRUE) && (MNodeFind(ptr,&N) == FAILURE) && (MNodeAdd(ptr,&N) == FAILURE)) ||
              ((MSched.IsServer == FALSE) && (MNodeAdd(ptr,&N) == FAILURE)))
            {
            /* cannot locate/add requested node */

            MDB(1,fSCHED) MLog("ERROR:    cannot add node '%s' to hostlist for job %s\n",
              ptr,
              J->Name);

            return(FAILURE);
            }

          break;

        case mdfOther:
        default:

          {
          mnl_t     *NL;

          NL = (mnl_t     *)Value;

          bmset(&J->IFlags,mjifHostList);

          if ((Mode == mIncr) || (Mode == mAdd))
            {
            MNLMerge(&J->ReqHList,&J->ReqHList,NL,NULL,NULL);
            }
          else if (Mode == mDecr)
            {
            /* NYI */

            return(FAILURE);
            }
          else
            {
            /* assume mSet */

            MNLCopy(&J->ReqHList,NL);
            }

          return(SUCCESS);
          }
        }  /* END switch (Format) */

      if (N != NULL)
        {
        /* one or more nodes specified within hostlist - set hostlist flag */

        hindex = 0;

        bmset(&J->IFlags,mjifHostList);
        }

      while (N != NULL)
        {
        if (Mode == mAdd)
          {
          /* locate available slot */

          MNLAddNode(&J->ReqHList,N,1,TRUE,NULL);
          }      /* END if (Mode == mAdd) */
        else 
          {
          /* assume Mode == mSet */

          MNLSetNodeAtIndex(&J->ReqHList,hindex,N);
          MNLSetTCAtIndex(&J->ReqHList,hindex,1);
  
          MNLTerminateAtIndex(&J->ReqHList,hindex + 1);

          hindex++;
          }  /* END else (Mode == mAdd) */

        /* FORMAT:  <HOST>[{ \t\n,:}<HOST>]... */

        ptr = MUStrTok(NULL," \t\n,:+",&TokPtr);

        MNodeFind(ptr,&N);
        }    /* END while (N != NULL) */

      bmset(&J->IFlags,mjifHostList);

      MUFree(&tmpBuf);
      }      /* END BLOCK (case mjaReqHostList) */

      break;

    case mjaReqVMList:

      {
      if (Value == NULL)
        {
        return(FAILURE);
        }

      if (((char *)Value)[0] == '\0')
        {
        return(SUCCESS);
        }

      switch (Format)
        {
        case mdfString:

          {
          char tmpBuf[MMAX_BUFFER];
          int  vmindex;
 
          mvm_t *V;

          char  *ptr;
          char  *TokPtr;

          ptr = (char *)Value;

          if (ptr[0] == '\0')
            {
            /* unset existing vmlist */

            MUFree((char **)&J->ReqVMList);

            break;
            }

          /* initialize hostlist */

          if (J->ReqVMList == NULL)
            {
            if ((J->ReqVMList = (mvm_t **)MUCalloc(1,sizeof(mvm_t *) * (MSched.JobMaxTaskCount))) == NULL)
              {
              return(FAILURE);
              }
            }

          /* FORMAT:  <HOST>[{ \t\n,:}<HOST>]... */

          MUStrCpy(tmpBuf,(char *)Value,sizeof(tmpBuf));

          ptr = MUStrTok((char *)tmpBuf," \t\n,:+",&TokPtr);

          vmindex = 0;

          while (ptr != NULL)
            {
            if (MVMFind(ptr,&V) == FAILURE)
              {
              return(FAILURE);
              }

            J->ReqVMList[vmindex] = V;

            if (MSched.VMGResMap == TRUE)
              {
              int gindex;

              gindex = MUMAGetIndex(meGRes,V->VMID,mVerify);

              if ((gindex > 0) && 
                  (V->N != NULL) && 
                  (J->Req[0] != NULL) &&
                  (MSNLGetIndexCount(&J->Req[0]->DRes.GenericRes,gindex) == 0))
                {
                /* add VM map GRes */

                MSNLSetCount(&J->Req[0]->DRes.GenericRes,gindex,1);
                }

              /* NOTE:  big issue:  J->Req[0] can be NULL at job load from CP and code above
                        will not be used due to race condition - FIXME */
              }

            vmindex++;

            if (vmindex >= MSched.JobMaxTaskCount)
              {
              return(FAILURE);
              }

            ptr = MUStrTok(NULL," \t\n,:+",&TokPtr);
            }  /* END while (ptr != NULL) */

          J->ReqVMList[vmindex] = NULL;

          /* VM explicitly required - safe to assign appropriate VM policy */

          if (J->VMUsagePolicy == mvmupNONE)
            J->VMUsagePolicy = mvmupVMOnly;
          }  /* END BLOCK (case mdfString) */

          break;

        default:

          /* NOT SUPPORTED */

          return(FAILURE);

          /*NOTREACHED*/

          break;
        }  /* END switch (Format) */
      }    /* END BLOCK (case mjaReqVMList) */

      break;

    case mjaInteractiveScript:

      MUStrDup(&J->InteractiveScript,(char *)Value);

      break;

    case mjaIsInteractive:

      J->IsInteractive = MUBoolFromString((char *)Value,TRUE);

      bmset(&J->SpecFlags,mjfInteractive);
      bmset(&J->Flags,mjfInteractive);

      break;

    case mjaIsRestartable:

      if (MUBoolFromString((char *)Value,TRUE) == TRUE)
        {
        bmset(&J->Flags,mjfRestartable);
        bmset(&J->SpecFlags,mjfRestartable);
        }
      else
        {
        bmunset(&J->Flags,mjfRestartable);
        bmunset(&J->SpecFlags,mjfRestartable);
        }

      break;

    case mjaIsSuspendable:

      if (MUBoolFromString((char *)Value,TRUE) == TRUE)
        bmset(&J->SpecFlags,mjfSuspendable);
      else
        bmunset(&J->SpecFlags,mjfSuspendable);

      break;

    case mjaIWD:

      MUStrDup(&J->Env.IWD,(char *)Value);

      break;

    case mjaJobID:

      {
      if ((Mode == mSetOnEmpty) && (J->Name[0] != '\0'))
        break;
          
      /* job name may be changed so delete the entry from hash table and re-add with new name */

      if ((J->Name[0] != '\0') && (!bmisset(&J->IFlags,mjifTemporaryJob)) && !bmisset(&J->IFlags,mjifIsTemplate))
        MJobHT.erase(J->Name);

      MUStrCpy(J->Name,(char *)Value,sizeof(J->Name));

      if ((!bmisset(&J->IFlags,mjifTemporaryJob)) && !bmisset(&J->IFlags,mjifIsTemplate))
        MJobTableInsert(J->Name,J);
      }  /* END BLOCK */

      break;

    case mjaJobName:

      /* a space within a job name is not allowed */

      if (Value == NULL)
        {
        MDB(5,fSCHED) MLog("ERROR:    NULL Value for Job Name for job %s\n",
          J->Name);

        return(FAILURE);
        }

      if (strchr((char *)Value,' ') != NULL)
        {
        MDB(5,fSCHED) MLog("ERROR:    attempted to set a job name (%s) with spaces for job %s\n",
          (char *)Value,
          J->Name);

        return(FAILURE);
        }

#if 0  /* disabled in 6.1 to use for DRMJID hash table */
      if (J->Index > 0)
        {
        /* only add real jobs to alternate name hash tables */

        if (J->AName != NULL)
          MUHTRemove(&MJobANameHT,J->AName,MUFREE);

        MUHTAdd(&MJobANameHT,(char *)Value,MUIntDup(J->Index),NULL,MUFREE);
        }
#endif

      MUStrDup(&J->AName,(char *)Value);

      break;

    case mjaJobGroup:

      {
      MDB(5,fSTRUCT) MLog("INFO:     job '%s' setting JGroup to '%s'\n",
        J->Name,
        (char *)Value);

      /* Save off old array index in case that was set first */
      int ArrayIndex = -1;

      if (J->JGroup != NULL)
        ArrayIndex = J->JGroup->ArrayIndex;

      MJGroupInit(J,(char *)Value,ArrayIndex,NULL);
      }

      break;

    case mjaJobGroupArrayIndex:

      if ((Value != NULL) && (((char *)Value)[0] != '\0'))
        {
        if (J->JGroup != NULL)
          J->JGroup->ArrayIndex = (int)(strtol((char *)Value,NULL,10));
        else
          MJGroupInit(J,NULL,(int)(strtol((char *)Value,NULL,10)),NULL);
        }

      break;

    case mjaLastChargeTime:

      if ((Value != NULL) && (((char *)Value)[0] != '\0'))
        J->LastChargeTime = strtol((char *)Value,NULL,10);

      break;

    case mjaMasterHost:

      {
      if (bmisset(&J->IFlags,mjifHostList))
        {
        /* job already has hostlist active */

        break;
        }

      /* Create a subset hostlist composed of the specified host */

      J->ReqHLMode = mhlmSubset;

      MJobSetAttr(J,mjaReqHostList,(void **)Value,mdfString,mSet);
      
      MUStrDup(&J->MasterHostName,(char *)Value);
      }  /* END BLOCK (case mjaMasterHost) */

      break;

    case mjaMessages:

      if (Format == mdfOther)
        {
        mmb_t *MP = (mmb_t *)Value;

        MMBAdd(&J->MessageBuffer,MP->Data,MP->Owner,MP->Type,MP->ExpireTime,MP->Priority,NULL);
        }
      else if (Format == mdfString)
        {
        MMBAdd(&J->MessageBuffer,(char *)Value,NULL,mmbtNONE,0,0,NULL);
        }

      break;

    case mjaMinPreemptTime:

      if ((Value == NULL) || (*(char *)Value == '\0'))
        break;

      {
      mulong tmpL; 

      if (Format == mdfLong)
        tmpL = *(mulong *)Value;
      else
        tmpL = MUTimeFromString((char *)Value);

      if ((Mode == mSet) || (Mode == mSetOnEmpty))
        J->MinPreemptTime = tmpL;
      else 
        J->MinPreemptTime = 0;
      }  /* END BLOCK (case mjaMinPreemptTime) */

      break;
      
    case mjaMinWCLimit:

      {
      long tmpL;

      if (Value == NULL)
        break;

      if (Format == mdfLong)
        {
        tmpL = *(long *)Value;
        }
      else
        {
        tmpL = MUTimeFromString((char *)Value);
        }
         
      if (tmpL > 0)
        {
        J->MinWCLimit = tmpL;
        }
      else
        {
        J->MinWCLimit = 0;
        }
      }  /* END BLOCK (case mjaMinWCLimit) */
        
      break;

    case mjaNodeAllocationPolicy:

      MJobProcessExtensionString(J,(char *)Value,mxaNAllocPolicy,NULL,NULL);

      break;

    case mjaNotification:

      MUStrDup(&J->SpecNotification,(char *)Value);
  
      /* adjust NotifyBM */

      bmfromstring((char *)Value,MNotifyType,&J->NotifyBM);

      break;

    case mjaNotificationAddress:
       
      /* just need to replace the old one
       *
       * NOTE:  might need to deal with mIncr/mDecr for notification lists in
                the future */

      MUStrDup(&J->NotifyAddress,(char *)Value);

      break;

    case mjaOFile:

      /* this is not E.Output because MJobToExportXML()
       * sets mjaOFile to RMError ... changing this could
       * potentially break this chain of communication */

      MUStrDup(&J->Env.RMOutput,(char *)Value);

      break;

    case mjaOMinWCLimit:

      {
      long tmpL;

      if (Value == NULL)
        break;

      if (Format == mdfLong)
        {
        tmpL = *(long *)Value;
        }
      else
        {
        tmpL = MUTimeFromString((char *)Value);
        }

      J->OrigMinWCLimit = tmpL;
      }  /* END BLOCK */

      break;

    case mjaOWCLimit:
 
      {
      long tmpL;

      if (Value == NULL)
        break;

      if (Format == mdfLong)
        {
        tmpL = *(long *)Value;
        }
      else
        {
        tmpL = MUTimeFromString((char *)Value);
        }

      J->OrigWCLimit = tmpL;
      }  /* END BLOCK */

      break;

    case mjaOwner:

      MUStrDup(&J->Owner,(char *)Value);

      break;
     
    case mjaReqNodes:

      {
      int NC;

      if (Value == NULL)
        {
        return(FAILURE);
        }

      if (Format == mdfInt)
        {
        NC = *(int *)Value;
        }
      else
        {
        NC = (int)strtol((char *)Value,NULL,0);
        }

      J->Request.NC = NC;

      if (J->Req[0] != NULL)
        {
        J->Req[0]->NodeCount = NC;
        }
      }    /* END BLOCK */

      break;

    case mjaReqProcs:

      {
      int PC;

      /* NOTE:  temporarily map procs to tasks */

      if (Value == NULL)
        {
        return(FAILURE);
        }

      if (Format == mdfInt)
        {
        PC = *(int *)Value;
        }
      else
        {
        PC = (int)strtol((char *)Value,NULL,0);
        }

      J->Request.TC = PC;

      bmunset(&J->IFlags,mjifTaskCountIsDefault);

      if (J->Req[0] != NULL)
        {
        J->Req[0]->TaskCount = PC;
        }
      }    /* END BLOCK (case mjaReqProcs) */

      break;

    case mjaPAL:  /* NOTE: this should be called SpecPAL */

      __MJobSetPAL(J,&J->SpecPAL,Value,Format,Mode);

      break;

    case mjaQOS:

      /* NOTE:  currently no way to clear QOS (FIXME) */

      if ((Mode == mSetOnEmpty) && (J->Credential.Q != NULL))
        break;

      if (Value != NULL)
        {
        char *ptr;
        char *TokPtr;

        ptr = MUStrTok((char *)Value,"/",&TokPtr);

        if (ptr != NULL)
          {
          if (MQOSAdd(ptr,&J->Credential.Q) == FAILURE)
            {
            MDB(1,fSCHED) MLog("ERROR:    cannot add QOS for job %s (Name: %s)\n",
              J->Name,
              ptr);

            return(FAILURE);
            }

          if (!MACLIsEmpty(J->Credential.CL))
            {
            /* update job cred list */

            MACLSet(&J->Credential.CL,maQOS,(void *)J->Credential.Q->Name,mcmpSEQ,mnmNeutralAffinity,0,FALSE);
            }
          }    /* END if (ptr != NULL) */

        ptr = MUStrTok(NULL," \t\n",&TokPtr);

        if (ptr != NULL)
          {
          if (J->Grid == NULL)
            J->Grid = (mjgrid_t *)MUCalloc(1,sizeof(mjgrid_t));

          if (J->Grid != NULL)
            MUStrDup(&J->Grid->QOS,ptr);
          }
        }   /* END if (Value != NULL) */

      break;
    
    case mjaQOSReq:

      if ((Mode == mSetOnEmpty) && (J->QOSRequested != NULL))
        break;

      if (Value != NULL)
        {
        char *ptr;
        char *TokPtr;

        ptr = MUStrTok((char *)Value,"/",&TokPtr);

        if (ptr != NULL)
          {
          if (MQOSAdd(ptr,&J->QOSRequested) == FAILURE)
            {
            MDB(1,fSCHED) MLog("ERROR:    cannot add QOS for job %s (Name: %s)\n",
              J->Name,
              ptr);

            return(FAILURE);
            }
          }

        ptr = MUStrTok(NULL," \t\n",&TokPtr);

        if (ptr != NULL)
          {
          if (J->Grid == NULL)
            J->Grid = (mjgrid_t *)MUCalloc(1,sizeof(mjgrid_t));

          if (J->Grid != NULL)
            MUStrDup(&J->Grid->QOSReq,ptr);
          }
        }   /* END if (Value != NULL) */

      break;

    case mjaReqAWDuration:

      {
      long tmpL;

      if (Value == NULL)
        break;

      if (Format == mdfLong)
        {
        tmpL = *(long *)Value;
        }
      else
        {
        tmpL = MUTimeFromString((char *)Value);
        }

      if (tmpL < 0)
        {
        /* unlimited walltime requested */

        if (MPar[0].L.JobPolicy->HLimit[mptMaxWC][0] > 0)
          tmpL = MPar[0].L.JobPolicy->HLimit[mptMaxWC][0];
        else
          tmpL = MDEF_SYSMAXJOBWCLIMIT;
        }
      else if (tmpL == 0)
        {
        if ((MSched.DefaultJ != NULL) &&
            (MSched.DefaultJ->SpecWCLimit[0]))
          tmpL = MSched.DefaultJ->SpecWCLimit[0];
        else 
          tmpL = MDEF_SYSDEFJOBWCLIMIT;
        }
      else
        {
        tmpL = MIN(MMAX_TIME,tmpL);
        }

      if ((J->OrigMinWCLimit > 0) && (MSched.JobExtendStartWallTime == TRUE))
        {
        /* MRMJobPostUpdate() will change J->SpecWCLimit[] back to whatever is in
           J->MinWCLimit so if we are supporting a walltime range then we have to
           change J->OrigWCLimit which is used as the max walltime and is not 
           overwritten in MRMJobPostUpdate() */

        J->OrigWCLimit = tmpL;
        }
      else
        {
        J->SpecWCLimit[0] = tmpL;
        J->SpecWCLimit[1] = tmpL;
        }
      }  /* END BLOCK (case mjaReqAWDuration) */

      break;

    case mjaReqCMaxTime:

      {
      long tmpL = 0;

      if (Mode == mUnset)
        {
        J->CMaxDate = 0;

        break;
        }

      if ((Format == mdfLong) && (Value != NULL))
        {
        tmpL = *(long *)Value;
        }

      J->CMaxDate = tmpL;
      }  /* END BLOCK */

      break;

    case mjaReqReservation:

      if ((Value == NULL) || (((char *)Value)[0] == '\0'))
        {
        bmunset(&J->SpecFlags,mjfAdvRsv);
        bmunset(&J->Flags,mjfAdvRsv);

        MUFree(&J->ReqRID);
        }
      else
        {
        bmset(&J->SpecFlags,mjfAdvRsv);     
        bmset(&J->Flags,mjfAdvRsv);

        MUStrDup(&J->ReqRID,(char *)Value);        
        }

      break;

    case mjaReqReservationPeer:

      if ((Value == NULL) || (((char *)Value)[0] == '\0'))
        {
        MUFree(&J->PeerReqRID);
        }
      else
        {
        MUStrDup(&J->PeerReqRID,(char *)Value);        
        }

      break;

    case mjaReqRMType:

      {
      int rindex;

      if (Format == mdfString)
        {
        rindex = MUGetIndexCI((char *)Value,MRMType,FALSE,mrmtNONE);
        }
      else
        {
        rindex = *(int *)Value;
        }

      J->ReqRMType = (enum MRMTypeEnum)rindex;
      }

    case mjaReqSMinTime:

      {
      long tmpL = 0;

      if (Format == mdfString)
        {
        tmpL = MUTimeFromString((char *)Value);

        if (((char *)Value)[0] == '+')
          {
          tmpL += MSched.Time;
          }
        }
      else if ((Format == mdfLong) && (Value != NULL))
        {
        tmpL = *(long *)Value;
        }

      /* NOTE:  evaluate various modes */

      if ((Mode == mSet) || (Mode == mSetOnEmpty))
        J->SpecSMinTime = tmpL;
      else if (J->SysSMinTime != NULL)
        J->SpecSMinTime = MAX(J->SysSMinTime[0],(mulong)tmpL);
      else
        J->SpecSMinTime = (mulong)tmpL;

      if (J->SysSMinTime != NULL)
        J->SMinTime = MAX(J->SpecSMinTime,J->SysSMinTime[0]);
      else
        J->SMinTime = J->SpecSMinTime;

      if ((J->Rsv != NULL) && 
          (MJOBISACTIVE(J) == FALSE) && 
          (J->Rsv->StartTime < J->SpecSMinTime) &&
          (MJobIsPreempting(J) == FALSE))  /* Preemptor must keep rsv */
        MJobReleaseRsv(J,TRUE,TRUE);
      }  /* END BLOCK */

      break;

    case mjaResFailPolicy:

      if (Format == mdfString)
        {
        J->ResFailPolicy = (enum MAllocResFailurePolicyEnum)MUGetIndexCI(
          (char *)Value,
          MARFPolicy,
          FALSE,
          marfNONE);
        }

      break;

    case mjaRM:

      {
      int rqindex;

      /* RM only impacts source RM */

      if (Format == mdfOther)
        {
        J->SubmitRM = (mrm_t *)Value;
        }
      else if (Format == mdfString)
        {
        mrm_t *tRM;

        if (MRMFind((char *)Value,&tRM) == FAILURE)
          {
          MDB(5,fSCHED) MLog("ERROR:    cannot set SRM for job %s (RM: %s)\n",
            J->Name,
            (char *)Value);

          return(FAILURE);
          }  

        J->SubmitRM = tRM;
        }

      for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
        {
        if (J->Req[rqindex] == NULL)
          break;

        if (J->DestinationRM != NULL)
          {
          /* see comment in Req->RMIndex comment in moab.h */

          /* NO-OP */
          }
        else if (J->SubmitRM != NULL)
          {
          J->Req[rqindex]->RMIndex = J->SubmitRM->Index;
          }
        else 
          {
          J->Req[rqindex]->RMIndex = 0;
          }
        }
      }  /* END BLOCK */

      break;

    case mjaRMError:

      MUStrDup(&J->Env.RMError,(char *)Value);

      break;

    case mjaRMOutput:

      MUStrDup(&J->Env.RMOutput,(char *)Value);

      break;

    case mjaRMCompletionTime:

      {
      long tmpL;

      if (Value == NULL)
        break;

      tmpL = 0;

      if (Format == mdfLong)
        {
        tmpL = *(long *)Value;
        }
      else
        {
        tmpL = strtol((char *)Value,NULL,0);
        }

      J->RMCompletionTime = tmpL;
      }  /* END BLOCK */

      break;

    case mjaRMSubmitFlags:

      MUStrDup(&J->RMSubmitFlags,(char *)Value);

      break;

    case mjaRMXString:

      /* RM extension string */

      if (Mode == mIncr)
        {
        if (Value == NULL)
          {
          break;
          }
        else if (J->RMXString == NULL)
          {
          MUStrDup(&J->RMXString,(char *)Value);
          }
        else if (!strstr(J->RMXString,(char *)Value))
          {
          /* append string to end */ 

          /* Init enough for RMXString + Value + ";" + NULL character */

          mstring_t tmpLine(strlen(J->RMXString) + strlen((char *)Value) + 2);
 
          MStringSetF(&tmpLine,"%s;%s",J->RMXString,(char *)Value);

          MUStrDup(&J->RMXString,tmpLine.c_str());
          } 
        }
      else
        {
        MUStrDup(&J->RMXString,(char *)Value);
        }

      break;

    case mjaRsvAccess:

      {
      char *ptr;
      char *TokPtr;

      char  RsvLine[MMAX_LINE];

      int   rindex;

      MUStrCpy(RsvLine,(char *)Value,sizeof(RsvLine));

      if ((ptr = MUStrTok(RsvLine,":",&TokPtr)) == NULL)
        {
        /* no rsv access specified */

        break;
        }

      if (strstr((char *)Value,"FREE") == NULL)
        {
        /* reserved resources required */

        MUStrDup(&J->ReqRID,ptr);

        bmset(&J->SpecFlags,mjfAdvRsv);
        }

      /* associate all accessible reservations */

      rindex = 0;

      if (J->RsvAList == NULL)
        {
        J->RsvAList = (char **)MUCalloc(1,sizeof(char *) * MMAX_PJOB);
        }

      while (ptr != NULL)
        {
        if (strcmp(ptr,"FREE"))
          {
          MUStrDup(&J->RsvAList[rindex],ptr);

          MDB(4,fSCHED) MLog("INFO:     job %s granted access to rsv %s\n",
            J->Name,
            J->RsvAList[rindex]);

          if (rindex == MMAX_PJOB)
            break;

          rindex++;
          }
  
        ptr = MUStrTok(NULL,":",&TokPtr);
        }  /* END while (ptr != NULL) */
      }    /* END BLOCK (case mjaRsvAccess) */

      break;

    case mjaSRMJID:

      if ((Mode == mSetOnEmpty) && (J->SRMJID != NULL))
        break;

      MUStrDup(&J->SRMJID,(char *)Value);

      break;

    case mjaStartCount:

      if (Value == NULL)
        break;

      if (Format == mdfString)
        {
        sscanf((char *)Value,"%d",
          &J->StartCount);
        }
      else
        {
        J->StartCount = *(int *)Value;
        }
 
      break;

    case mjaStartTime:

      {
      long tmpL;

      if (Value == NULL)
        break;

      tmpL = 0;

      if (Format == mdfLong)
        {
        tmpL = *(long *)Value;
        }
      else
        {
        tmpL = strtol((char *)Value,NULL,0);
        }

      J->StartTime    = tmpL;
      J->DispatchTime = tmpL;
      }  /* END BLOCK */

      break;

    case mjaState:

      {
      int cindex;

      if (Value == NULL)
        break;

      if (Format == mdfInt)
        {
        cindex = *(int *)Value;
        }
      else
        {
        cindex = MUGetIndex((char *)Value,MJobState,FALSE,0);
        }

      if (cindex != 0)
        MJobSetState(J,(enum MJobStateEnum)cindex);
      }  /* END BLOCK */

      break;

    case mjaStatMSUtl:

      if (Value == NULL)
        break;

      if (Format == mdfString)
        {
        sscanf((char *)Value,"%lf",
          &J->MSUtilized);
        }
      else
        {
        J->MSUtilized = *(double *)Value;
        }
 
      break;

    case mjaStatPSDed:

      if (Value == NULL)
        break;

      if (Format == mdfString)
        {
        sscanf((char *)Value,"%lf",
          &J->PSDedicated);
        }
      else
        {
        J->PSDedicated = *(double *)Value;
        }
 
      break;

    case mjaStatPSUtl:

      if (Value == NULL)
        break;

      if (Format == mdfString)
        {
        sscanf((char *)Value,"%lf",
          &J->PSUtilized);
        }
      else
        {
        J->PSUtilized = *(double *)Value;
        }
 
      break;

    case mjaStdErr:

      {
      MUStrDup(&J->Env.Error,(char *)Value);
      }  /* END BLOCK */

      break;

    case mjaStdIn:

      {
      MUStrDup(&J->Env.Input,(char *)Value);
      }  /* END BLOCK */

      break;

    case mjaStdOut:

      {
      MUStrDup(&J->Env.Output,(char *)Value);
      }  /* END BLOCK */

      break;

    case mjaSubmitHost:

      if ((Value != NULL) && (J->SubmitHost == NULL))
        MUStrDup(&J->SubmitHost,(char *)Value);

      break;

    case mjaSubmitLanguage:

      J->RMSubmitType = (enum MRMTypeEnum)MUGetIndex((char *)Value,MRMType,FALSE,0);

      break;

    case mjaSubmitString:

      {
      char tmpBuf[MMAX_SCRIPT];

      /* NOTE;  unpack string */

      MUStringUnpack((char *)Value,tmpBuf,sizeof(tmpBuf),NULL);

      MUStrDup(&J->RMSubmitString,tmpBuf);
      }  /* END BLOCK */

      break;

    case mjaSubmitTime:

      {
      long tmpL;

      /* Submit time should never changed from when it has been submitted. 
         This keeps the submit time from changing in grids. */

      if ((Mode == mSetOnEmpty) && (J->SubmitTime != 0))
        break;

      if (Value == NULL)
        break;

      tmpL = 0;

      if (Format == mdfLong)
        {
        tmpL = *(long *)Value;
        }
      else
        {
        tmpL = strtol((char *)Value,NULL,0);
        }

      J->SubmitTime = tmpL;
      }  /* END BLOCK */

      break;

    case mjaSuspendDuration:

      {
      long tmpL;

      if (Value == NULL)
        break;

      tmpL = 0;

      if (Format == mdfLong)
        {
        tmpL = *(long *)Value;
        }
      else
        {
        tmpL = strtol((char *)Value,NULL,0);
        }

      J->SWallTime = tmpL;
      }  /* END BLOCK */

      break;

    case mjaSysPrio:

      if (Mode == mUnset)
        {
        J->SystemPrio = 0;
        
        break;
        }

      if (Format == mdfString)
        {
        mbool_t IsRelative = FALSE;

        if (((char *)Value)[0] == '+')
          {
          /* relative system priority is indicated by a leading '+' */

          IsRelative = TRUE;
          J->SystemPrio = strtol((char *)Value + 1,NULL,10);
          }
        else
          {
          J->SystemPrio = strtol((char *)Value,NULL,10);
          }

        if ((J->SystemPrio < 0) || (IsRelative == TRUE))
          {
          J->SystemPrio += (MMAX_PRIO_VAL << 1);
          }
        }
      else
        { 
        J->SystemPrio = *(long *)Value;
        }
 
      break;

    case mjaSysSMinTime:
     
      {
      char TString[MMAX_LINE];
      long tmpL = 0;

      if (Format == mdfString)
        {
        tmpL = MUTimeFromString((char *)Value);
        }
      else if (Value != NULL)
        {
        tmpL = *(long *)Value;
        }

      if (J->SysSMinTime == NULL)
        {
        if ((J->SysSMinTime = (mulong *)MUCalloc(1,sizeof(mulong) * MMAX_PAR)) == NULL)
          break;
        }

      if (Mode == mSet)
        J->SysSMinTime[0] = tmpL;
      else
        J->SysSMinTime[0] = MAX(J->SysSMinTime[0],(mulong)tmpL);

      J->SMinTime = MAX(J->SpecSMinTime,J->SysSMinTime[0]);

      MULToTString(J->SysSMinTime[0] - MSched.Time,TString);

      MDB(5,fSCHED) MLog("INFO:     system min start time set on job %s for %s\n",
        J->Name,
        TString);
      }  /* END (case mjaSysSMinTime) */

      break;

    case mjaTaskMap:

      /* FIXME: reports of tindex being unintialized */

      /* FORMAT:  <NODEID>[,<NODEID>]... */

      if (Value == NULL)
        break;

      {
      int tindex;

      if (Format == mdfOther)
        {
        int        nindex;
        int        tcindex;

        mnode_t   *N;
        mnl_t     *NL = (mnl_t     *)Value;

        tindex = 0;

        for (nindex = 0;MNLGetNodeAtIndex(NL,nindex,&N) == SUCCESS;nindex++)
          {
          if (((Mode == mAdd) || (Mode == mIncr)) && (J->TaskMap != NULL))
            {
            /* adding node to existing list, look for match */

            for (tindex = 0;tindex < J->TaskMapSize;tindex++)
              {
              if (J->TaskMap[tindex] == -1)
                {
                /* end of list located */

                break;
                }

              if (J->TaskMap[tindex] == N->Index)
                {
                /* taskmap already includes specified node */

                break;
                }
              }    /* END for (tindex) */

            if ((tindex < J->TaskMapSize) && (J->TaskMap[tindex] != -1))
              {
              /* task already listed in map */
     
              continue;
              } 
            }  /* END if (((Mode == mAdd) || ...) */

          if ((tindex >= J->TaskMapSize) && (MJobGrowTaskMap(J,0) == FAILURE))
            {
            /* taskmap is full */

            J->TaskMap[J->TaskMapSize - 1] = -1;

            return(FAILURE);
            }

          /* ensure that J->TaskMap is not NULL to eliminate warnings further on... */
          if (J->TaskMap == NULL)
            break;

          /* add node to taskmap */

          /* NOTE:  currently does not support nodelist taskcount (FIXME) */

          for (tcindex = 0;tcindex < MNLGetTCAtIndex(NL,nindex);tcindex++)
            {
            if ((tindex >= J->TaskMapSize) && (MJobGrowTaskMap(J,0) == FAILURE))
              {
              return(FAILURE);
              }

            J->TaskMap[tindex] = N->Index;

            J->TaskMap[tindex + 1] = -1;

            tindex++;  /* NOTE:  increment tindex to allow correct J->TaskMap[] termination later in block */
            }  /* END for (tcindex) */
          }    /* END for (nindex) */
        }      /* END if (Format == mdfOther) */
      else
        {
        char *ptr;
        char *TokPtr;

        mnode_t *N;

        ptr = MUStrTok((char *)Value,",",&TokPtr);

        tindex = 0;

        while (ptr != NULL)
          {
          if (MNodeFind(ptr,&N) == FAILURE)
            {
            /* log failure (NYI) */

            ptr = MUStrTok(NULL,",",&TokPtr);

            continue;
            }

          if ((tindex >= J->TaskMapSize) && (MJobGrowTaskMap(J,0) == FAILURE))
            {
            J->TaskMap[J->TaskMapSize - 1] = -1;

            return(FAILURE);
            }

          /* ensure that J->TaskMap is not NULL to eliminate warnings further on... */
          if (J->TaskMap == NULL)
            break;

          J->TaskMap[tindex] = N->Index;

          tindex++;

          if (tindex >= J->TaskMapSize)
            break;

          ptr = MUStrTok(NULL,",",&TokPtr);
          }  /* END while (ptr != NULL) */
        }    /* END else (Format == mdfOther) */

      if ((tindex >= J->TaskMapSize) && (MJobGrowTaskMap(J,0) == FAILURE))
        {
        J->TaskMap[J->TaskMapSize - 1] = -1;
        
        return(FAILURE);
        }

      /* terminate taskmap */

      J->TaskMap[tindex] = -1;

      /* Include the local task map resources (that may not have been included
       * in Value) For example, in the Grid a workload query from a slave node
       * would not include GRES Node 0, so we need to make sure that we add it
       * back into the TaskMap. */

      if (J->Req[1] != NULL)
        {
        MJobGetLocalTL(J,FALSE,&J->TaskMap[tindex],J->TaskMapSize - tindex);
        }
      }  /* END BLOCK (case mjaTaskMap) */

      break;

    case mjaTermTime:

      {
      long tmpL;

      if (Value == NULL)
        break;

      tmpL = 0;

      if (Format == mdfLong)
        {
        tmpL = *(long *)Value;
        }
      else
        {
        tmpL = MUTimeFromString((char *)Value);
        }

      if (tmpL >= (long)MSched.Time)
        {
        /* absolute time specified */

        J->TermTime = tmpL;
        }
      else if (J->TermTime != 0)
        {
        /* relative time specified */

        J->TermTime = MSched.Time + tmpL;
        }
      }  /* END BLOCK */

      break;

    case mjaTemplateFailurePolicy:

      if (J->TemplateExtensions == NULL)
        MJobAllocTX(J);

      J->TemplateExtensions->FailurePolicy = (enum MJobTemplateFailurePolicyEnum)MUGetIndexCI((char *)Value,MJobTemplateFailurePolicy,FALSE,mjtfpNONE);

      break;

    case mjaTemplateFlags:

      {
      char *ptr;
      char *tTokPtr;        
      enum MTJobFlagEnum findex;  
      char tmpLine[MMAX_LINE];
      mbool_t Selectable;
  
      MUStrCpy(tmpLine,(char *)Value,sizeof(tmpLine));
  
      ptr = MUStrTok(tmpLine,",",&tTokPtr);

      if (J->TemplateExtensions == NULL)
        {
        MJobAllocTX(J);
        }

      /* Keep/set SELECT, but do not take out here, use SELECT=FALSE */

      Selectable = bmisset(&J->TemplateExtensions->TemplateFlags,mtjfTemplateIsSelectable);

      bmclear(&J->TemplateExtensions->TemplateFlags);

      while (ptr != NULL)
        {
        findex = (enum MTJobFlagEnum)MUGetIndexCI(ptr,MTJobFlags,FALSE,mtjfNONE);

        if (findex != mtjfNONE)
          {
          bmset(&J->TemplateExtensions->TemplateFlags,findex);
          }

        ptr = MUStrTok(NULL,",",&tTokPtr);
        }

      /* Reset SELECT to TRUE if it was set before */

      if (Selectable == TRUE)
        bmset(&J->TemplateExtensions->TemplateFlags,mtjfTemplateIsSelectable);
      }

      break;

    case mjaTemplateSetList:

      {
      char *tmpPtr;
      char *tmpTokPtr;
      char  tmpValue[MMAX_LINE << 2];
      mjob_t *tmpTJ;

      if (Format != mdfString)
        return(FAILURE);

      MUStrCpy(tmpValue,(char *)Value,sizeof(tmpValue));

      tmpPtr = MUStrTok(tmpValue,",",&tmpTokPtr);

      while (tmpPtr != NULL)
        {
        if (MTJobFind(tmpPtr,&tmpTJ) == SUCCESS)
          {
          MULLAdd(&J->TSets,tmpTJ->Name,(void *)tmpTJ,NULL,NULL);
          }

        tmpPtr = MUStrTok(NULL,",",&tmpTokPtr);
        }  /* END while (tmpPtr != NULL) */
      }  /* END BLOCK (case mjaTemplateSetList) */

    case mjaTrigNamespace:

      {
      if ((Format != mdfString) ||
          (Value == NULL))
        return(FAILURE);

      if (MJobSetDynamicAttr(J,mjaTrigNamespace,(void **)Value,mdfString) == FAILURE)
        return(FAILURE);
      }

      break;

    case mjaVariables:

      /* FORMAT:  <name>[=<value>][[;<name[=<value]]...] */

      {
      int  tmpI;
      char tmpLine[MMAX_LINE];
      mjob_t *GroupJob = NULL;

      mbitmap_t VarBM;
      mbitmap_t SetBM;

      char *ptr;
      char *TokPtr;

      char *ptr2;
      char *TokPtr2;

      char *VarName = NULL;
      char *VarVal;
      char *VarTok;

      char *SetVal;

      mbool_t DoIncr   = FALSE;
      mbool_t DoExport = FALSE;

      /* UNSET */

      if ((Mode == mUnset) || (Mode == mDecr))
        { 
        /* remove specified variables */

        if (!strcasecmp((char *)Value,"all"))
          {
          /* remove all variables */
          
          MUHTFree(&J->Variables,TRUE,MUFREE);

          MJobWriteVarStats(J,mUnset,"ALL",NULL,NULL);

          break;
          }

        ptr = MUStrTok((char *)Value,",;",&TokPtr);

        while (ptr != NULL)
          {
          /* FORMAT [<VAR>] */

          if (MUHTRemove(&J->Variables,ptr,MUFREE) == FAILURE)
            {
            MDB(4,fSCHED) MLog("INFO:     cannot remove variable for job %s (variable not found)\n",
              J->Name);

            return(FAILURE);
            }

          MJobWriteVarStats(J,mUnset,ptr,NULL,NULL);

          ptr = MUStrTok(NULL,",;",&TokPtr);
          }  /* END while (ptr != NULL) */

        break;
        }  /* END if ((Mode == mUnset) || (Mode == mDecr)) */

      /* SET - loop through each variable */

      ptr = MUStrTok((char *)Value,"+,;",&TokPtr);

      while (ptr != NULL)
        {
        bmclear(&SetBM);
        bmclear(&VarBM);

        SetVal = NULL;

        /* FORMAT [<VAR>=<VAL>]
           Note: Colon is a valid delimiter, as documented under set templates VARIABLE=attr:value. */

        ptr2 = MUStrTok(ptr,"=:",&TokPtr2);

        if (ptr != ptr2)
          {
          MDB(1,fSCHED) MLog("ERROR:    cannot set variable for job %s (variable name not specified)\n",
            J->Name);

          return(FAILURE);
          }

        /* NYI - in the future, consolidate the code here and in MTrigSetVariable() so we aren't duplicating
         * so much */

        if (strchr(ptr,'+') != NULL)
          {
          DoIncr = TRUE;
          bmset(&SetBM,mtvaIncr);
          }

        if (strchr(ptr,'^') != NULL)
          {
          DoExport = TRUE;
          bmset(&SetBM,mtvaExport);
          }

        VarName = MUStrTok(ptr,"+^ \n\t",&VarTok);

        if (VarName == NULL)
          {
          MDB(1,fSCHED) MLog("ERROR:    cannot set variable for job %s (no variable name specified)\n",
            J->Name);

          return(FAILURE);
          }

        /* check to see if this variable already exists */

        if (MUHTGet(&J->Variables,VarName,(void **)&VarVal,&VarBM) == SUCCESS)
          {
          bmor(&SetBM,&VarBM);
          }

        /* see if we need to perform any special operations on the value */

        if (DoIncr == TRUE)
          {
          if (VarVal != NULL)
            {
            tmpI = (int)strtol((char *)VarVal,NULL,10);

            tmpI++;
            }
          else
            {
            tmpI = 0;
            }

          snprintf(tmpLine,sizeof(tmpLine),"%d",
            tmpI);

          SetVal = tmpLine;
          }
        else
          {
          SetVal = TokPtr2;
          }

        if ((SetVal != NULL) && (SetVal[0] != '\0'))
          {
          /* dup string value and continue to next variable */

          MUHTAdd(&J->Variables,VarName,strdup(SetVal),&SetBM,MUFREE);
          MJobWriteVarStats(J,mSet,VarName,SetVal,NULL);

          if (DoExport == TRUE)
            {
            if (J->JGroup != NULL)
              {
              /* we use J->JGroupIndex in for faster lookups */

              if (MJobFind(J->JGroup->Name,&GroupJob,mjsmExtended) == SUCCESS)
                {
                mbitmap_t tmpBM;

                bmset(&tmpBM,mtvaExport);

                MUHTAdd(&GroupJob->Variables,VarName,strdup(SetVal),&tmpBM,MUFREE);
                MJobWriteVarStats(GroupJob,mSet,VarName,SetVal,NULL);
                }
              }

            if (J->ParentVCs != NULL)
              {
              mln_t *tmpL;
              mvc_t *VC;
              mbitmap_t tmpBM;

              bmset(&tmpBM,mtvaExport);

              for (tmpL = J->ParentVCs;tmpL != NULL;tmpL = tmpL->Next)
                {
                VC = (mvc_t *)tmpL->Ptr;

                MUHTAdd(&VC->Variables,VarName,strdup(SetVal),&tmpBM,MUFREE);
                }
              }
            }  /* END if (DoExport == TRUE) */
          }  /* END (SetVal != NULL) && ... */
        else
          {
          /* variable has name but not value */

          MUHTAdd(&J->Variables,VarName,NULL,&SetBM,NULL);
          }

        ptr = MUStrTok(NULL,"+;,",&TokPtr);
        }  /* END while (ptr != NULL) */
      }    /* END BLOCK (case mjaVariables) */

      break;

    case mjaTTC:

      if (Value == NULL)
        break;

      if (Format == mdfString)
        {
        J->TotalTaskCount = (int)strtol((char *)Value,NULL,10);
        }
      else if (Format == mdfInt)
        {
        J->TotalTaskCount = *(int *)Value;
        }

      break;

    case mjaUMask:

      if (Format == mdfInt)
        {
        J->Env.UMask = *(int *)Value;
        }
      else
        {
        J->Env.UMask = (int)strtol((char *)Value,NULL,0);
        }

      break;

    case mjaUser:

      /* NOTE:  currently no way to clear user (FIXME) */

      if ((Mode == mSetOnEmpty) && (J->Credential.U != NULL))
        break;

      if (Value != NULL)
        {
        char *ptr;
        char *TokPtr;

        ptr = MUStrTok((char *)Value,"/",&TokPtr);

        if (ptr != NULL)
          {
          if (MUserAdd(ptr,&J->Credential.U) == FAILURE)
            {
            MDB(1,fSCHED) MLog("ERROR:    cannot add user for job %s (Name: %s)\n",
              J->Name,
              ptr);

            return(FAILURE);
            }

          /* update job cred list */

          MACLSet(&J->Credential.CL,maUser,(void *)J->Credential.U->Name,mcmpSEQ,mnmNeutralAffinity,0,FALSE);
          }

        ptr = MUStrTok(NULL," \t\n",&TokPtr);

        if (ptr != NULL)
          {
          if (J->Grid == NULL)
            J->Grid = (mjgrid_t *)MUCalloc(1,sizeof(mjgrid_t));

          if (J->Grid != NULL)
            MUStrDup(&J->Grid->User,ptr);
          }
        }   /* END if (Value != NULL) */

      break;

    case mjaUserPrio:

      if ((Value == NULL) || (Mode == mUnset))
        {
        J->UPriority = 0;

        break;
        }

      {
      long tmpL = 0;

      if (Format == mdfString)
        {
        sscanf((char *)Value,"%ld",
          &tmpL);
        }
      else if (Format == mdfInt)
        {
        tmpL = (long)*(int *)Value;
        }
      else
        {
        tmpL = *(long *)Value;
        }

      tmpL = __MJobAdjustUserPriority(tmpL);

      J->UPriority = tmpL;
      }  /* END BLOCK */

      break;

    case mjaVM:

      if (strstr((char *)Value,"destroyvm") != NULL)
        bmset(&J->IFlags,mjifDestroyDynamicVM);

      break;

    case mjaVMUsagePolicy:

      if (Format == mdfString)
        {
        J->VMUsagePolicy = (enum MVMUsagePolicyEnum)MUGetIndexCI(
          (char *)Value,
          MVMUsagePolicy,
          FALSE,
          0);

        if (J->VMUsagePolicy == mvmupNONE)
          {
          return(FAILURE);
          }

        switch (J->VMUsagePolicy)
          {
          case mvmupPMOnly:

            break;

          case mvmupVMCreatePersist:
          case mvmupAny:
          case mvmupPMPref:
          case mvmupVMCreate:
          case mvmupVMOnly:
          case mvmupVMPref:

            /* NOTE:  The mjifDestroyDynamicVM bit determines persistent or dynamic.  1 = DYNAMIC, 0 = PERSISTENT.
            ** If VM usage policy is VMCreate, then set the DestroyDynamicVM flag. IF usage policy 
            ** is VMCreatePersist, then clear mjifDestroyDynamicVM bit & sets policy to VMCreate. */

            if (J->VMUsagePolicy == mvmupVMCreate)
              bmset(&J->IFlags,mjifDestroyDynamicVM);
            else
              {
              if (J->VMUsagePolicy == mvmupVMCreatePersist)
                {
                J->VMUsagePolicy = mvmupVMCreate;
                bmunset(&J->IFlags,mjifDestroyDynamicVM);
                }
              }
            break;

          default:

            assert(FALSE);

            break;
          }  /* END switch (J->VMUsagePolicy) */
        }    /* END if (Format == mdfString) */
 
      break;

    case mjaVWCLimit:

      {
      long tmpL;

      if (Value == NULL)
        break;

      if (Format == mdfLong)
        {
        tmpL = *(long *)Value;
        }
      else
        {
        tmpL = MUTimeFromString((char *)Value);
        }

      J->VWCLimit = tmpL;
      }  /* END BLOCK */

      break;

    case mjaWorkloadRMID:

      if (J->TemplateExtensions == NULL)
        {
        MJobAllocTX(J);
        }

      MUStrDup(&J->TemplateExtensions->WorkloadRMID,(char *)Value);

      MRMFind(J->TemplateExtensions->WorkloadRMID,&J->TemplateExtensions->WorkloadRM);

      break;

    default:

      /* NOT HANDLED */

      return(FAILURE);

      /*NOTREACHED*/

      break;
    }  /* END switch (AIndex) */

  return(SUCCESS);
  }  /* END MJobSetAttr() */


