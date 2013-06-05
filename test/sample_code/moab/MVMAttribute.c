/* HEADER */

/**
 * @file MVMAttribute.c
 * 
 * Contains various function to Set Attribute on a VM
 *
 */


#include "moab.h"
#include "moab-proto.h"
#include "moab-const.h"
#include "moab-global.h"



/**
 * Set specified VM attribute.
 *
 * @param VM     (I) [modified]
 * @param AIndex (I)
 * @param Value  (I)
 * @param Format (I)
 * @param Mode   (I)
 */

int MVMSetAttr(

  mvm_t                  *VM,      /* I * (modified) */
  enum MVMAttrEnum        AIndex, /* I */
  void const             *Value,  /* I */
  enum MDataFormatEnum    Format, /* I */
  enum MObjectSetModeEnum Mode)   /* I */

  {
  const char *FName = "MVMSetAttr";

  char const *ValAsString = (char const *)Value;

  MDB(6,fSTRUCT) MLog("%s(%s,%s,Value,%s,%s)\n",
    FName,
    (VM != NULL) ? VM->VMID : "NULL",
    MVMAttr[AIndex],
    MFormatMode[Format],
    MObjOpType[Mode]);

  if (VM == NULL)
    {
    return(FAILURE);
    }

  switch (AIndex)
    {
    case mvmaActiveOS:

      {
      int OS = MUMAGetIndex(meOpsys,ValAsString,mSet);

      if (OS != 0)
        VM->ActiveOS = OS;

      /*TODO: what if OS is not in VM's OSList? */
      }

      break;

    case mvmaADisk:

      VM->ARes.Disk = strtol(ValAsString,NULL,0);

      break;

    case mvmaAlias:

      {
      if (Mode != mClear)
        {
        mvoid_t *Element;

        if (MUHTGet(&MNetworkAliasesHT,ValAsString,NULL,NULL) == SUCCESS)
          {
          /* Specified alias is already in use.  Do not allow */

          return(FAILURE);
          }

        /* Add to table now */
        /* Done as mvoid_t so that node aliases work as well */

        Element = (mvoid_t *)MUCalloc(1,sizeof(mvoid_t));
        Element->Ptr = (void *)VM;
        Element->PtrType = mxoxVM;
        MUStrCpy(Element->Name,VM->VMID,sizeof(Element->Name));

        MUHTAdd(&MNetworkAliasesHT,ValAsString,(void *)Element,NULL,MUFREE);
        }

      if ((Mode == mSet) || (Mode == mClear))
        {
        /* Clear list, remove from global table */

        mln_t *freePtr = NULL;

        while (MULLIterate(VM->Aliases,&freePtr) == SUCCESS)
          {
          MUHTRemove(&MNetworkAliasesHT,freePtr->Name,MUFREE);
          }

        /* TODO: send out signal to outside system to free alias */

        MULLFree(&VM->Aliases,MUFREE);
        }

      if ((Mode == mSet) || (Mode == mAdd))
        {
        /* Set has already been cleared, treat as add */
        /* These must be added one at a time */

        if (VM->Aliases == NULL)
          {
          MULLCreate(&VM->Aliases);
          }

        if (MULLAdd(&VM->Aliases,ValAsString,NULL,NULL,MUFREE) == FAILURE)
          return(FAILURE);

        /* TODO: send out signal to outside system to set alias */
        }
      } /* END BLOCK mvmaAlias */

      break;

    case mvmaAMem:

      VM->ARes.Mem = strtol(ValAsString,NULL,0);

      break;

    case mvmaAProcs:

      /* NOTE:  set ARes.Procs to 0 if VM is not available */

      if (MNSISUP(VM->State) && (VM->State != mnsUnknown))
        {
        VM->ARes.Procs = strtol(ValAsString,NULL,0);
        }
      else
        {
        VM->ARes.Procs = 0;
        }

      break;

    case mvmaCDisk:

      VM->CRes.Disk = strtol(ValAsString,NULL,0);

      break;

    case mvmaCMem:

      VM->CRes.Mem = strtol(ValAsString,NULL,0);

      break;

    case mvmaCProcs:

      VM->CRes.Procs = strtol(ValAsString,NULL,0);

      break;

    case mvmaCPULoad:

      VM->CPULoad = strtod(ValAsString,NULL);

      break;

    case mvmaDescription:

      MUStrDup(&VM->Description,ValAsString);

      break;

    case mvmaEffectiveTTL:

      VM->EffectiveTTL = strtol(ValAsString,NULL,10);

      break;

    case mvmaFlags:

      bmfromstring((char *)Value,MVMFlags,&VM->Flags);

      break;

    case mvmaGMetric:

      {
      char *GMTok;
      char *GMName;
      char *GMVal;
      char *TokPtr = NULL;

      char *TokPtr2 = NULL;

      int   gmindex;

      /* generic metrics */

      /* FORMAT:  <GMETRIC>{:=}<VAL>[,<GMETRIC>{:=}<VAL>]... */

      GMTok = MUStrTok((char *)Value,", \t\n",&TokPtr);

      while (GMTok != NULL)
        {
        GMName = MUStrTok(GMTok,"=:",&TokPtr2);
        GMVal  = MUStrTok(NULL,"=:",&TokPtr2);

        GMTok = MUStrTok(NULL,", \t\n",&TokPtr);

        if ((GMName == NULL) || (GMVal == NULL))
          {
          /* mal-formed value, ignore */

          continue;
          }

        gmindex = MUMAGetIndex(meGMetrics,GMName,mAdd);

        if ((gmindex <= 0) || (gmindex >= MSched.M[mxoxGMetric]))
          continue;

        /* valid gmetric located */

        MVMSetGMetric(VM,gmindex,GMVal);
        }  /* END while (GMTok != NULL) */
      }    /* END BLOCK (case mvmaGMetric) */

      break;


    case mvmaID:

      MUStrCpy(VM->VMID,ValAsString,sizeof(VM->VMID));

      break;

    case mvmaLastMigrateTime:

      VM->LastMigrationTime = (mulong)strtol(ValAsString,NULL,10);

      break;

    case mvmaLastSubState:

      MUStrDup(&VM->LastSubState,ValAsString);

      break;

    case mvmaLastSubStateMTime:

      VM->LastSubStateMTime = (mulong)strtol(ValAsString,NULL,10);

      break;

    case mvmaMigrateCount:

      VM->MigrationCount = (int)strtol(ValAsString,NULL,10);

      break;

    case mvmaNextOS:

      {
      int OS = MUMAGetIndex(meOpsys,ValAsString,mSet);

      if (OS != 0)
        VM->NextOS = OS;

      /*TODO: what if OS is not in VM's OSList? */
      }

    case mvmaNodeTemplate:
    case mvmaOSList:
    case mvmaSlotIndex:
    case mvmaState:

      break;

    case mvmaSovereign:

      {
      mbool_t WasSovereign = bmisset(&VM->Flags,mvmfSovereign);
      mbool_t NewVal = MUBoolFromString(ValAsString,FALSE);

      if (NewVal == TRUE)
        bmset(&VM->Flags,mvmfSovereign);
      else
        bmunset(&VM->Flags,mvmfSovereign);

      if ((!bmisset(&VM->Flags,mvmfSovereign)) && (WasSovereign == TRUE))
        {
        if (VM->J != NULL)
          {
          /* is this how to handle previously sovereign VMs correctly? */

          bmset(&VM->J->IFlags,mjifCancel);
          }
        }
      }

      break;

    case mvmaSpecifiedTTL:

      VM->SpecifiedTTL = strtol(ValAsString,NULL,10);

      break;

    case mvmaStartTime:

      VM->StartTime = (mulong)strtol(ValAsString,NULL,10);

      break;

    case mvmaSubState:

      if (ValAsString == NULL)
        {
        MUFree(&VM->SubState);
        }
      else
        {
        MUStrDup(&VM->SubState,(char *)ValAsString);
        }

      break;

    case mvmaStorageRsvNames:

      MUStrDup(&VM->StorageRsvNames,(char *)ValAsString);

      break;

    case mvmaVariables:

      if (Mode == mSet)
        {
        /* Clear the current attrs */

        MUHTClear(&VM->Variables,TRUE,(mfree_t)MUFree);
        }

      /* FORMAT:  attr:value[+attr:value]... */
      /*     value is a string (alphanumeric and '_') */

      if ((Mode == mAdd) || (Mode == mSet))
        {
        char *ptr;
        char *TokPtr = NULL;

        char *TokPtr2 = NULL;

        ptr = MUStrTok((char *)ValAsString,"+",&TokPtr);

        while (ptr != NULL)
          {
          char *AName;
          char *AVal;
          char *MallocVal = NULL;
          char ANameNoQuotes[MMAX_LINE];

          AName = MUStrTok(ptr,":",&TokPtr2);
          AVal = MUStrTok(NULL,":",&TokPtr2);

          MUStrReplaceStr(AName,"\"","",ANameNoQuotes,sizeof(ANameNoQuotes));

          if (AVal != NULL)
            {
            int AValLen = (strlen(AVal) * sizeof(char)) + 1;
            MallocVal = (char *)MUMalloc(AValLen);
            MUStrReplaceStr(AVal,"\"","",MallocVal,strlen(AVal) + 1);
            }

          MUHTAdd(&VM->Variables,ANameNoQuotes,(void *)MallocVal,NULL,MUFREE);

          ptr = MUStrTok(NULL,"+",&TokPtr);
          } /* END while (ptr != NULL) */
        } /* END if ((Mode == mAdd) || (Mode == mSet)) */

      break;

    case mvmaTrigger:

      if (Format == mdfString)
        {
        mtrig_t *T;

        marray_t TList;

        int tindex;

        MUArrayListCreate(&TList,sizeof(mtrig_t *),10);

        if (MTrigLoadString(
              &VM->T,
              (char *)Value,
              TRUE,
              FALSE,
              mxoxVM,
              VM->VMID,
              &TList,
              NULL) == FAILURE)
          {
          return(FAILURE);
          }

        for (tindex = 0; tindex < TList.NumItems;tindex++)
          {
          T = (mtrig_t *)MUArrayListGetPtr(&TList,tindex);

          if (MTrigIsValid(T) == FALSE)
            continue;

          MTrigInitialize(T);

          MOAddTrigPtr(&VM->T,T);
          } /* END for (tindex) */

        return(SUCCESS);
        } /* END if (Format == mdfString) */

      /* END BLOCK mvmaTrigger */

      break;

    case mvmaNONE:
    case mvmaLAST:
    default:

      break;
    }  /* END switch (AIndex) */

  return(SUCCESS);
  } /* END MVMSetAttr() */



/**
 * Report specified VM attribute as string
 *
 * @see MNodeSetAttr() - peer
 * @see MNodeAToString() - peer
 * @see MVMSetAttr() - peer
 * @see MVMToXML() - parent
 *
 * @param V       (I)
 * @param N       (I) [optional]
 * @param AIndex  (I)
 * @param Buf     (O) [minsize=MMAX_LINE]
 * @param BufSize (I) [optional,-1 if not set,default=MMAX_LINE]
 * @param DModeBM (I) [bitmap of enum MCModeEnum - mcmVerbose,mcmXML,mcmUser=human_readable]
 */

int MVMAToString(

  mvm_t     *V,
  mnode_t   *N,
  enum MVMAttrEnum AIndex,
  char      *Buf,
  int        BufSize,
  mbitmap_t *DModeBM)

  {
  if (BufSize <= 0)
    BufSize = MMAX_LINE;

  if (Buf == NULL)
    {
    return(FAILURE);
    }

  Buf[0] = '\0';

  if (V == NULL)
    {
    return(FAILURE);
    }

  switch (AIndex)
    {
    case mvmaActiveOS:

      if (V->ActiveOS != 0)
        strcpy(Buf,MAList[meOpsys][V->ActiveOS]);

      break;

    case mvmaADisk:

      if ((V->ARes.Disk >= 0) || (bmisset(DModeBM,mcmVerbose)))
        {
        sprintf(Buf,"%d",
          V->ARes.Disk);
        }

      break;

    case mvmaAlias:

      {
      char   *BPtr;
      int     BSpace;
      mln_t  *Alias = NULL;
      mbool_t AliasPrinted = FALSE;

      MUSNInit(&BPtr,&BSpace,Buf,BufSize);

      while (MULLIterate(V->Aliases,&Alias) == SUCCESS)
        {
        if (AliasPrinted)
          MUSNPrintF(&BPtr,&BSpace,",");

        MUSNPrintF(&BPtr,&BSpace,"%s",
          Alias->Name);

        AliasPrinted = TRUE;
        }
      }

      break;

    case mvmaAMem:

      if ((V->ARes.Mem >= 0) || (bmisset(DModeBM,mcmVerbose)))
        {
        sprintf(Buf,"%d",
          V->ARes.Mem);
        }

      break;

    case mvmaAProcs:

      if ((V->ARes.Procs >= 0) || (bmisset(DModeBM,mcmVerbose)))
        {
        sprintf(Buf,"%d",
          V->ARes.Procs);
        }

      break;

    case mvmaCDisk:

      if ((V->CRes.Disk >= 0) || (bmisset(DModeBM,mcmVerbose)))
        {
        sprintf(Buf,"%d",
          V->CRes.Disk);
        }

      break;

    case mvmaCPULoad:

      sprintf(Buf,"%f",
        V->CPULoad);

      break;

    case mvmaCMem:

      if ((V->CRes.Mem >= 0) || (bmisset(DModeBM,mcmVerbose)))
        {
        sprintf(Buf,"%d",
          V->CRes.Mem);
        }

      break;

    case mvmaCProcs:

      if ((V->CRes.Procs >= 0) || (bmisset(DModeBM,mcmVerbose)))
        {
        sprintf(Buf,"%d",
          V->CRes.Procs);
        }

      break;

    case mvmaContainerNode:

      if (V->N != NULL)
        {
        snprintf(Buf,BufSize,"%s",V->N->Name);
        }

      break;

    case mvmaDDisk:

      if ((V->DRes.Disk >= 0) || (bmisset(DModeBM,mcmVerbose)))
        {
        sprintf(Buf,"%d",
          V->DRes.Disk);
        }

      break;

    case mvmaDMem:

      if ((V->DRes.Mem >= 0) || (bmisset(DModeBM,mcmVerbose)))
        {
        sprintf(Buf,"%d",
          V->DRes.Mem);
        }

      break;

    case mvmaDProcs:

      if ((V->DRes.Procs >= 0) || (bmisset(DModeBM,mcmVerbose)))
        {
        sprintf(Buf,"%d",
          V->DRes.Procs);
        }

      break;

    case mvmaDescription:

      if ((V->Description != NULL) &&
          (V->Description[0] != '\0'))
        {
        strcpy(Buf,V->Description);
        }

      break;

    case mvmaEffectiveTTL:

      {
      sprintf(Buf,"%ld",V->EffectiveTTL);
      }

      break;

    case mvmaFlags:

      {
      mstring_t tmp(MMAX_LINE);

      bmtostring(&V->Flags,MVMFlags,&tmp);

      MUStrCpy(Buf,tmp.c_str(),BufSize);
      }    /* END BLOCK (case mvmaFlags) */

      break;

    case mvmaGMetric:

      {

      MVMGMetricAToString(V->GMetric,Buf,BufSize,DModeBM);

#if 0
      char *BPtr;
      int   BSpace;

      int   gmindex;

      /* FORMAT:  GMETRIC=<GMNAME>:<VAL>[,<GMNAME>:<VAL>]... */

      MUSNInit(&BPtr,&BSpace,Buf,BufSize);

      if (V->GMetric != NULL)
        {
        for (gmindex = 1;gmindex < MSched.M[mxoxGMetric];gmindex++)
          {
          if (V->GMetric[gmindex] == NULL)
            {
            continue;
            }

          if (MGMetric.Name[gmindex][0] == '\0')
            break;

          if (V->GMetric[gmindex]->SampleCount <= 0)
            continue; 

          if (Buf[0] != '\0')
            MUSNCat(&BPtr,&BSpace,",");

          if (bmisset(DModeBM,mcmUser))
            {
            /* NOTE:  0th entry in MAList is always "NONE" */
            MUSNPrintF(&BPtr,&BSpace,"%s=%.2f",
              MGMetric.Name[gmindex],
              V->GMetric[gmindex]->IntervalLoad);
            }
          else
            {
            MUSNPrintF(&BPtr,&BSpace,"%s:%.2f",
              MGMetric.Name[gmindex],
              V->GMetric[gmindex]->IntervalLoad);
            }
          }  /* END for (gmindex) */
        }  /* END if (V->GMetric != NULL) */
#endif

      }  /* END BLOCK (case mvmaGMetric) */

      break;

    case mvmaID:

      if (V->VMID[0] != '\0')
        strcpy(Buf,V->VMID);
       
      break;

    case mvmaJobID:

      if (V->JobID[0] != '\0')
       strcpy(Buf,V->JobID);

      break;

    case mvmaLastMigrateTime:

      if (V->LastMigrationTime > 0)
        sprintf(Buf,"%lu",V->LastMigrationTime);

      break;

    case mvmaLastSubState:

      if (V->LastSubState != NULL)
        MUStrCpy(Buf,V->LastSubState,BufSize);

      break;

    case mvmaLastSubStateMTime:

      if (V->LastSubStateMTime > 0)
        sprintf(Buf,"%lu",V->LastSubStateMTime);

      break;

    case mvmaLastUpdateTime:

      if (V->UpdateTime <= 0)
        {
        /* What happens in this case? */
        }
      else
        {
        sprintf(Buf,"%lu",V->UpdateTime);
        }

      break;

    case mvmaMigrateCount:

      if (V->MigrationCount > 0)
        sprintf(Buf,"%d",V->MigrationCount);

      break;

    case mvmaNextOS:

      if ((V->NextOS > 0) && (V->NextOS != V->ActiveOS))
        {
        strcpy(Buf,MAList[meOpsys][V->NextOS]);
        }

      break;

    case mvmaNetAddr:

      if (V->NetAddr != NULL)
        MUStrCpy(Buf,V->NetAddr,BufSize);

      break;

    case mvmaOSList:

      if (V->OSList != NULL)
        {
        char *BPtr;
        int   BSpace;

        int   osindex;

        MUSNInit(&BPtr,&BSpace,Buf,BufSize);

        for (osindex = 0;V->OSList[osindex].AIndex > 0;osindex++)
          {
          if (Buf[0] != '\0')
            {
            MUSNPrintF(&BPtr,&BSpace,",%s",
              MAList[meOpsys][V->OSList[osindex].AIndex]);
            }
          else
            {
            MUSNCat(&BPtr,&BSpace,MAList[meOpsys][V->OSList[osindex].AIndex]);
            }
          }  /* END for (osindex) */
        }    /* END if (V->OSList != NULL) */

      break;

    case mvmaPowerIsEnabled:

      {
      mbool_t IsOn = TRUE;

      switch (V->PowerState)
        {
        case mpowOff:

          IsOn = FALSE;

          break;

        default:

          break;
        }

      strcpy(Buf,MBool[IsOn]);
      }  /* END BLOCK */

      break;

    case mvmaPowerState:

      if (V->PowerState != mpowNONE)
        {
        strcpy(Buf,MPowerState[V->PowerState]);
        }

      break;

    case mvmaProvData:

      {
      mstring_t tmp(MMAX_LINE);

      /* sync with MNodeSetAttr#mnaProvData */
      mnode_t tmpN;
      memset(&tmpN,0,sizeof(tmpN));

      tmpN.ActiveOS = V->ActiveOS;
      tmpN.NextOS =   V->NextOS;
      tmpN.LastOSModRequestTime = V->LastOSModRequestTime;

      tmpN.OSList = (V->N == NULL) ? NULL : V->N->VMOSList;

      if (tmpN.OSList == NULL)
        {
        tmpN.OSList = MSched.DefaultN.VMOSList;

        if (tmpN.OSList == NULL)
          /* No valid OS information */
          break;
        }

      MNodeAToString(&tmpN,mnaProvData,&tmp,DModeBM);

      MUStrCpy(Buf,tmp.c_str(),BufSize);
      }  /* END BLOCK */

      break;

    case mvmaRackIndex:

      if (V->Rack >= 0)
        {
        sprintf(Buf,"%d",
          V->Rack);
        }

      break;

    case mvmaSlotIndex:

      if (V->Slot >= 0)
        {
        sprintf(Buf,"%d",
          V->Slot);
        }

      break;

    case mvmaSpecifiedTTL:

      {
      sprintf(Buf,"%ld",V->SpecifiedTTL);
      }

      break;

    case mvmaStartTime:

      {
      sprintf(Buf,"%lu",V->StartTime);
      }

      break;

    case mvmaState:
      
      if (V->State != mnsNONE)
        {
        strcpy(Buf,MNodeState[V->State]);
        }

      break;

    case mvmaSubState:

      if ((bmisset(&V->Flags,mvmfInitializing)) &&
          (V->SubState != NULL) &&
          (strstr(V->SubState,"pbs")))
        {
        strcpy(Buf,"rminitializing");
        }
      else if (V->SubState != NULL)
        {
        MUStrCpy(Buf,V->SubState,BufSize);
        }

      break;

    case mvmaStorageRsvNames:

      if (V->StorageRsvs != NULL)
        {
        char *BPtr;
        int   BSpace;
        mln_t *LLIter = NULL;

        MUSNInit(&BPtr,&BSpace,Buf,BufSize);

        while (MULLIterate(V->StorageRsvs,&LLIter) == SUCCESS)
          {
          if (Buf[0] != '\0')
            MUSNPrintF(&BPtr,&BSpace,",");

          MUSNPrintF(&BPtr,&BSpace,"%s",
            ((mrsv_t *)LLIter->Ptr)->Name);
          }
        } /* END case mvmaStorageRsvNames */

      break;

    case mvmaTrackingJob:

      if ((V->TrackingJ != NULL) && (V->TrackingJ->Name != NULL))
        MUStrCpy(Buf,V->TrackingJ->Name,BufSize);

      break;

    case mvmaVariables:

      if (V->Variables.NumItems > 0)
        {
        char *BPtr;
        int   BSpace;
        char *VarName;
        char *VarValue;

        mhashiter_t VarIter;

        MUHTIterInit(&VarIter);
        MUSNInit(&BPtr,&BSpace,Buf,BufSize);

        while (MUHTIterate(&V->Variables,&VarName,(void **)&VarValue,NULL,&VarIter) == SUCCESS)
          {
          if (Buf[0] != '\0')
            {
            MUSNPrintF(&BPtr,&BSpace,"+%s",
              VarName);
            }
          else
            {
            MUSNPrintF(&BPtr,&BSpace,"%s",
              VarName);
            }

          /* Add the value, if its not null */

          if (VarValue != NULL)
            {
            MUSNCat(&BPtr,&BSpace,":");
            MUSNCat(&BPtr,&BSpace,VarValue);
            }
          } /* END while (MUHTIterate(...) == SUCCESS) */
        } /* END case mvmaAttrs */

      break;

    default:

      /* NO-OP */

      break;
    }  /* END switch (AIndex) */

  return(SUCCESS);
  }  /* END MVMAToString() */


/* END MVMAttribute.c */
