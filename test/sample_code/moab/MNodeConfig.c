/* HEADER */

      
/**
 * @file MNodeConfig.c
 *
 * Contains:  MNode Configuration functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 * Load NODECFG parameter values for specified node.
 *
 * @see MNodeProcessConfig() - child
 * @see MRMNodePostLoad() - parent
 *
 * @param N (I)
 * @param Buf (I) [optional]
 */

int MNodeLoadConfig(

  mnode_t *N,    /* I */
  char    *Buf)  /* I (optional) */

  {
  char *ptr;
  char *head;

  char  Value[MMAX_BUFFER];

  char  tmpSName[MMAX_NAME];
  char  tmpLName[MMAX_NAME];

  const char *FName = "MNodeLoadConfig";

  MDB(5,fSTRUCT) MLog("%s(%s,%s)\n",
    FName,
    (N != NULL) ? N->Name : "NULL",
    (Buf != NULL) ? Buf : "NULL");

  /* load specified node config info */

  if ((N == NULL) || (N->Name[0] == '\0'))
    {
    return(FAILURE);
    }
 
  /* search for parameters specified with both long and short names */

  /* search short names */

  MNodeAdjustName(N->Name,0,tmpSName);

  if (Buf != NULL)
    head = Buf;
  else
    head = MSched.ConfigBuffer;

  if (head == NULL)
    {
    return(FAILURE);
    }

  ptr = head;
 
  while (MCfgGetSVal(
           head,
           &ptr,
           MCredCfgParm[mxoNode],
           tmpSName,
           NULL,
           Value,
           sizeof(Value),
           0,
           NULL) != FAILURE)
    {
    /* node attribute located */

    /* FORMAT:  <ATTR>=<VALUE>[ <ATTR>=<VALUE>]... */

    if (MNodeProcessConfig(N,NULL,Value) == FAILURE)
      {
      return(FAILURE);
      } 
    }

  MNodeAdjustName(N->Name,1,tmpLName);

  if (strcmp(tmpSName,tmpLName))
    {
    /* search long names */

    ptr = MSched.ConfigBuffer;
 
    while (MCfgGetSVal(
             head,
             &ptr,
             MCredCfgParm[mxoNode],
             tmpLName,
             NULL,
             Value,
             sizeof(Value),
             0,
             NULL) != FAILURE)
      {
      /* node attribute located */
 
      /* FORMAT:  <ATTR>=<VALUE>[ <ATTR>=<VALUE>]... */
 
      if (MNodeProcessConfig(N,NULL,Value) == FAILURE)
        {
        return(FAILURE);
        }
      }
    }    /* END if (strcmp(tmpSName,tmpLName)) */

  return(SUCCESS);
  }  /* END MNodeLoadConfig() */





/**
 * Process node attributes specified via config file.
 *
 * @see MNodeSetAttr() - child
 * @see MNodeLoadConfig() - parent
 *
 * @param N (I) [modified]
 * @param R (I) [optional]
 * @param Value (I) modified
 */

int MNodeProcessConfig(

  mnode_t *N,     /* I (modified) */
  mrm_t   *R,     /* I (optional) */
  char    *Value) /* I (modified) */

  {
  char  *ptr;
  char  *TokPtr;

  char  *ptr2;

  char   ValBuf[MMAX_BUFFER];
  char   AttrArray[MMAX_NAME];

  int    aindex;

  enum   MNodeAttrEnum AIndex;

  int    tmpInt;
  int    tmpI;
  double tmpD;

  int    SlotsUsed = -1;

  int    rc;

  enum MObjectSetModeEnum CIndex;

  mbool_t FailureDetected;

  const char *FName = "MNodeProcessConfig";

  MDB(5,fSTRUCT) MLog("%s(%s,%s,%s)\n",
    FName,
    (N != NULL) ? N->Name : "NULL",
    (R != NULL) ? R->Name : "NULL",
    (Value != NULL) ? Value : "NULL");

  if ((N == NULL) || 
      (Value == NULL) ||
      (Value[0] == '\0'))
    {
    return(FAILURE);
    }

  ptr = MUStrTokE(Value," \t\n",&TokPtr);

  FailureDetected = FALSE;

  while (ptr != NULL)
    {
    /* parse name-value pairs */
 
    /* FORMAT:  <ATTR>=<VALUE>[,<VALUE>] */
 
    if (MUGetPair(
          ptr,
          (const char **)MNodeAttr,
          &aindex,   /* O */
          AttrArray,
          TRUE,
          &tmpI,     /* O */
          ValBuf,    /* O */
          MMAX_BUFFER) == FAILURE)
      {
      char tmpLine[MMAX_LINE];

      /* cannot parse value pair */

      snprintf(tmpLine,sizeof(tmpLine),"unknown attribute '%.128s' specified",
        ptr);

      MDB(3,fSTRUCT) MLog("WARNING:  unknown attribute '%s' specified for node %s\n",
        ptr,
        (N != NULL) ? N->Name : "NULL");

      MMBAdd(&N->MB,tmpLine,NULL,mmbtNONE,0,0,NULL);
 
      ptr = MUStrTokE(NULL," \t\n",&TokPtr);
 
      continue;
      }

    rc = SUCCESS;

    AIndex = (enum MNodeAttrEnum)aindex;

    switch (tmpI)
      {
      case 1:

        CIndex = mIncr;

        break;

      case -1:

        CIndex = mDecr; 

        break;

      case 0:
      default:

        CIndex = mSet;

        break;
      }  /* END switch (CIndex) */

    switch (AIndex)
      {
      case mnaAccess:
      case mnaAccountList:
      case mnaArch:
      case mnaCfgClass:
      case mnaClassList:
      case mnaFlags:
      case mnaGroupList:
      case mnaJTemplateList:
      case mnaNodeIndex:
      case mnaPowerPolicy:
      case mnaMaxProcPerClass:
      case mnaProvRM:
      case mnaQOSList:
      case mnaUserList:
      
        MNodeSetAttr(N,AIndex,(void **)ValBuf,mdfString,CIndex);

        break;
 
      case mnaComment:

        /* add as node message */

        {
        mmb_t *MB;

        MMBAdd(
          &N->MB,
          (char *)ValBuf,
          MSCHED_SNAME,
          mmbtAnnotation,
          MMAX_TIME,
          0,
          &MB);

        if (MB != NULL)
          MUStrDup(&MB->Label,"comment");
        }  /* END BLOCK (case mnaComment) */

        break;

      case mnaAvlClass:

        if (!bmisset(&N->IFlags,mnifRMClassUpdate))
          {
          MNodeSetAttr(N,AIndex,(void **)ValBuf,mdfString,mSet);
          }
 
        break;

      case mnaAvlMemW:

        if ((N->P != NULL) || 
            (MNPrioAlloc(&N->P) == SUCCESS))
          {
          N->P->CW[mnpcAMem] = strtod(ValBuf,NULL);
          N->P->CWIsSet = TRUE;
          }

        break;

      case mnaAvlProcW:

        if ((N->P != NULL) ||
            (MNPrioAlloc(&N->P) == SUCCESS))
          {
          N->P->CW[mnpcAProcs] = strtod(ValBuf,NULL);    
          N->P->CWIsSet = TRUE;
          }
 
        break;

      case mnaRCMem:

        {
        bmunset(&N->IFlags,mnifMemOverride); /* unset so that new values can be set dynamically */

        MNodeSetAttr(N,mnaRCMem,(void **)ValBuf,mdfString,mSet);

        if (ValBuf[strlen(ValBuf) - 1] == '^')
          bmset(&N->IFlags,mnifMemOverride);
        }

        break;

      case mnaCfgMemW:

        if ((N->P != NULL) ||
            (MNPrioAlloc(&N->P) == SUCCESS))
          {
          N->P->CW[mnpcCMem] = strtod(ValBuf,NULL);    
          N->P->CWIsSet = TRUE;
          }
 
        break;

      case mnaRCProc:

        N->CRes.Procs = (int)strtol(ValBuf,NULL,10);

        break;

      case mnaCfgProcW:

        if ((N->P != NULL) ||
            (MNPrioAlloc(&N->P) == SUCCESS))
          {
          N->P->CW[mnpcCProcs] = strtod(ValBuf,NULL);    
          N->P->CWIsSet = TRUE;
          }
 
        break;

      case mnaRCSwap:
        
        {
        bmunset(&N->IFlags,mnifSwapOverride); /* unset so that value can be set dynamically */

        MNodeSetAttr(N,mnaRCSwap,(void **)ValBuf,mdfString,mSet);

        if (ValBuf[strlen(ValBuf) - 1] == '^')
          bmset(&N->IFlags,mnifSwapOverride);
        }

        break;

      case mnaFailure:

        {
        char *ptr;
        char *TokPtr;

        int findex;

        enum MNodeFailEnum {
          mnfNONE,
          mnfState,
          mnfMem,
          mnfSwap,
          mnfLoad };

        const char *NFType[] = {
          NONE,
          "STATE",
          "MEM",
          "SWAP",
          "LOAD",
          NULL };

        ptr = MUStrTok(ValBuf,",",&TokPtr);

        while (ptr != NULL)
          {
          findex = MUGetIndex(ptr,NFType,FALSE,0);

          switch (findex)
            {
            case mnfState:

              MNodeSetState(N,mnsDown,FALSE);

              if (N->Message == NULL)
                {
                MUStrDup(&N->Message,"node is down");
                }

              break;

            case mnfMem:

              tmpInt = 0;
              MNodeSetAttr(N,mnaRAMem,(void **)&tmpInt,mdfInt,mSet);

              MUStrDup(&N->Message,"memory is low");

              break;

            case mnfSwap:
  
              tmpInt = 3;
              MNodeSetAttr(N,mnaRASwap,(void **)&tmpInt,mdfInt,mSet);

              MUStrDup(&N->Message,"swap is low");

              break;

            case mnfLoad:

              N->ARes.Procs = 0;
              N->Load = 4.3;

              MUStrDup(&N->Message,"excessive processor usage");

              break;

            case mnfNONE:
            default:

              /* NO-OP */
  
              break;
            }  /* END switch (findex) */        

          ptr = MUStrTok(NULL,",",&TokPtr);
          }    /* END while (ptr != NULL) */
        }      /* END BLOCK */

        break;

      case mnaFeatures:
 
        {
        char  tmpBuffer[MMAX_BUFFER];
        char *TokPtr2;
 
        /* FORMAT:  <FEATURE>[{ ,:}<FEATURE>]... */

        /* NOTE: Mode == mDecr NOT supported */

        /* if '!' specified, update named feature */

        /* NOTE:  if features specified via config file, should be appended to features
                  discovered via RM */

        MSched.NodeFeaturesSpecifiedInConfig = TRUE;

        if (CIndex == mSet)
          {
          bmclear(&N->FBM);
          }

        strcpy(tmpBuffer,ValBuf);

        ptr2 = MUStrTok(tmpBuffer,":, \t\n",&TokPtr2);

        while (ptr2 != NULL)
          {
          MNodeProcessFeature(N,ptr2);

          ptr2 = MUStrTok(NULL,":, \t\n",&TokPtr2);
          }  /* END while (ptr != NULL) */
        }    /* END BLOCK (case mnaFeatures) */
 
        /* Moab owns feature indicated by clearing RM ownership bit */

        bmunset(&N->RMAOBM,mnaFeatures);

        break;

      case mnaCfgGRes:
      case mnaChargeRate:
      case mnaEnableProfiling:

        if (MNodeSetAttr(N,AIndex,(void **)ValBuf,mdfString,mSet) == FAILURE)
          {
          /* complete or partial failure setting attribute */

          /* NO-OP */
          }

        break;

      case mnaKbdDetectPolicy:
      case mnaKbdIdleTime:
      case mnaMinPreemptLoad:
      case mnaMinResumeKbdIdleTime:

        MNodeSetAttr(N,AIndex,(void **)ValBuf,mdfString,mSet);

        break;

      case mnaLoadW:
 
        if ((N->P != NULL) ||
            (MNPrioAlloc(&N->P) == SUCCESS))
          {
          N->P->CW[mnpcLoad] = strtod(ValBuf,NULL);    
          N->P->CWIsSet = TRUE;
          }
 
        break;

      case mnaLogLevel:

        N->LogLevel = (int)strtol(ValBuf,NULL,10);

        break;

      case mnaMaxJob:

        {
        tmpI = (int)strtol(ValBuf,NULL,10);

        if (AttrArray[0] == '\0')
          {
          N->AP.HLimit[mptMaxJob] = tmpI;
          }
        else
          {
          /* NYI */
          }
        }   /* END BLOCK */

        break;

      case mnaMaxProc:

        {
        tmpI = (int)strtol(ValBuf,NULL,10);

        if (AttrArray[0] == '\0')
          {
          N->AP.HLimit[mptMaxProc] = tmpI;
          }
        }      /* END BLOCK */

        break;

      case mnaMaxJobPerGroup:

        tmpI = (int)strtol(ValBuf,NULL,10);

        if (N->NP == NULL)
          {
          if ((N->NP = (mnpolicy_t *)MUCalloc(1,sizeof(mnpolicy_t))) == NULL)
            break;
          }

        N->NP->MaxJobPerGroup = tmpI;

        break;

      case mnaMaxJobPerUser:
 
        tmpI = (int)strtol(ValBuf,NULL,10);

        if (N->NP == NULL)
          {
          if ((N->NP = (mnpolicy_t *)MUCalloc(1,sizeof(mnpolicy_t))) == NULL)
            break;
          }
 
        N->NP->MaxJobPerUser = tmpI;
 
        break;

      case mnaMaxLoad:
      case mnaMaxWCLimitPerJob:

        MNodeSetAttr(N,AIndex,(void **)ValBuf,mdfString,mSet);
 
        break;

      case mnaMaxProcPerGroup:

        tmpI = (int)strtol(ValBuf,NULL,10);

        if (N->NP == NULL)
          {
          if ((N->NP = (mnpolicy_t *)MUCalloc(1,sizeof(mnpolicy_t))) == NULL)
            break;
          }

        N->NP->MaxProcPerGroup = tmpI;

        break;

      case mnaMaxProcPerUser:

        tmpI = (int)strtol(ValBuf,NULL,10);

        if (N->NP == NULL)
          {
          if ((N->NP = (mnpolicy_t *)MUCalloc(1,sizeof(mnpolicy_t))) == NULL)
            break;
          }

        N->NP->MaxProcPerUser = tmpI;

        break;

      case mnaMaxWatts:

        MNodeSetAttr(N,AIndex,(void **)ValBuf,mdfString,mSet);

        break;

      case mnaOS:
      case mnaOSList:
      case mnaVMOSList:

        MNodeSetAttr(N,AIndex,(void **)ValBuf,mdfString,mSet);

        break;

      case mnaNodeType:
 
        MUStrDup(&N->NodeType,ValBuf);
 
        break;

      case mnaResOvercommitFactor:
      case mnaAllocationLimits:

        if (N->ResOvercommitFactor == NULL)
          {
          if ((N->ResOvercommitFactor = (double *)MUCalloc(1,sizeof(double) * mrlLAST)) == NULL)
            {
            return(FAILURE);
            }
          }

        MUResFactorArrayFromString((char *)ValBuf,N->ResOvercommitFactor,NULL,TRUE,FALSE);

        break;

      case mnaResOvercommitThreshold:
      case mnaUtilizationThresholds:

        if (N->ResOvercommitThreshold == NULL)
          {
          if ((N->ResOvercommitThreshold = (double *)MUCalloc(1,sizeof(double) * mrlLAST)) == NULL)
            {
            return(FAILURE);
            }
          }

        if (N->GMetricOvercommitThreshold == NULL)
          {
          int gmindex;
          if ((N->GMetricOvercommitThreshold = (double *)MUCalloc(1,sizeof(double) * MSched.M[mxoxGMetric])) == NULL)
            {
            return(FAILURE);
            }

          /* initialize all thresholds to 'unset' value */
          for (gmindex = 0;gmindex < MSched.M[mxoxGMetric]; gmindex++)
            {
            N->GMetricOvercommitThreshold[gmindex] = MDEF_GMETRIC_VALUE;
            }

          }

        MUResFactorArrayFromString((char *)ValBuf,N->ResOvercommitThreshold,N->GMetricOvercommitThreshold,TRUE,FALSE);

        break;

      case mnaOwner:

        MNodeSetAttr(N,AIndex,(void **)ValBuf,mdfString,mSet);

        break;

      case mnaPartition:
      case mnaPreemptMaxCPULoad:
      case mnaPreemptMinMemAvail:
      case mnaPreemptPolicy:
  
        MNodeSetAttr(N,AIndex,(void **)ValBuf,mdfString,mSet);
 
        break;

      case mnaPrioF:

        {
        mnprio_t *tmpPriorityFunc = NULL;

        if (MProcessNAPrioF(&tmpPriorityFunc,ValBuf) == FAILURE)
          {
          MNPrioFree(&tmpPriorityFunc);
          return(FAILURE);
          }

        /* clear out existing priority function */
        MNPrioFree(&N->P);
        N->P = tmpPriorityFunc;
        }  /* END BLOCK */

        break;

      case mnaPriority:
 
        N->Priority = (int)strtol(ValBuf,NULL,10);
        N->PrioritySet = TRUE;
 
        break;

      case mnaPrioW:
 
        if ((N->P != NULL) ||
            (MNPrioAlloc(&N->P) == SUCCESS))
          {
          N->P->CW[mnpcPriority] = strtod(ValBuf,NULL);    
          N->P->CWIsSet = TRUE;
          }
 
        break;

      case mnaProcSpeed:
 
        tmpI = (int)strtol(ValBuf,NULL,10);
 
        N->ProcSpeed = tmpI;
 
        break;

      case mnaRack:

        {
        /* FORMAT:  <RACKINDEX>|<RACKNAME> */

        if (isdigit(ValBuf[0]))
          {
          tmpI = (int)strtol(ValBuf,NULL,10);
          }
        else
          {
          if (MRackAdd(ValBuf,&tmpI,NULL) == FAILURE)
            {
            /* cannot add rack */

            break;
            }
          }

        rc = MNodeSetAttr(N,mnaRack,(void **)&tmpI,mdfInt,mSet); 
        }  /* END BLOCK */

        break;

      case mnaRADisk:
      case mnaRCDisk:
      case mnaMaxPE:
      case mnaMaxPEPerJob:
      case mnaRMList:
   
        MNodeSetAttr(N,AIndex,(void **)ValBuf,mdfString,mSet);

        break;

      case mnaSize:

        tmpI = (int)strtol(ValBuf,NULL,10);

        SlotsUsed = tmpI; 
 
        break;

      case mnaSlot:
 
        rc = MNodeSetAttr(N,mnaSlot,(void **)ValBuf,mdfString,mSet); 

        break;

      case mnaSpeed:
 
        tmpD = strtod(ValBuf,NULL);
 
        if (tmpD == 0.0)
          {
          tmpD = 0.01;
          }

        N->Speed = MIN(100.0,tmpD);
 
        break;

      case mnaSpeedW:
 
        if ((N->P != NULL) ||
            (MNPrioAlloc(&N->P) == SUCCESS))
          {
          N->P->CW[mnpcSpeed] = strtod(ValBuf,NULL);    
          N->P->CWIsSet = TRUE;
          }
 
        break;

      case mnaTrigger:

        {
        marray_t TList;

        int tindex;

        mtrig_t *T;

        MUArrayListCreate(&TList,sizeof(mtrig_t *),4);

        MTrigLoadString(&N->T,ValBuf,TRUE,FALSE,mxoNode,N->Name,&TList,NULL);

        if (N->T != NULL)
          {
          for (tindex = 0;tindex < TList.NumItems;tindex++)
            {
            T = (mtrig_t *)MUArrayListGetPtr(&TList,tindex);
   
            if (MTrigIsValid(T) == FALSE)
              continue;

            MTrigInitialize(T);
            }  /* END for (tindex) */
          }

        MUArrayListFree(&TList);
        }    /* END BLOCK mnaTrigger */

        break;

      case mnaUsageW:
 
        if ((N->P != NULL) ||
            (MNPrioAlloc(&N->P) == SUCCESS))
          {
          N->P->CW[mnpcUsage] = strtod(ValBuf,NULL);    
          N->P->CWIsSet = TRUE;
          }
 
        break;

#if 0
      /* this is not used anywhere */

      case mnaResource:

        /* FORMAT:  <RESNAME>:<RESCOUNT>[,<RESNAME>:<RESCOUNT>]... */

        {
        char *ptr;
        char *TokPtr;

        ptr = MUStrTok(ValBuf,", \t",&TokPtr);

        while (ptr != NULL)
          {
          /* FORMAT:  <RESDESC>[{ \t,}<RESDESC]... */

          MNodeSetAttr(N,mnaCfgGRes,(void **)ptr,mdfString,mSet);
          
          ptr = MUStrTok(NULL,", \t",&TokPtr);
          }
        }    /* END BLOCK */

        break;
#endif

      case mnaVariables:

        MNodeSetAttr(N,AIndex,(void **)ValBuf,mdfString,mSet);

        break;

      default:

        /* NO-OP */

        break;
      }  /* END switch (AIndex) */

    if (rc == FAILURE)
      FailureDetected = TRUE;

    ptr = MUStrTokE(NULL," \t\n",&TokPtr);     
    }    /* END while (ptr != NULL) */

  if (FailureDetected == TRUE)
    {
    return(FAILURE);
    }

  /* sync 'side-effects' */

  if (SlotsUsed > 0)
    {
    N->SlotsUsed = SlotsUsed;

    if ((N->RackIndex > 0) && (N->SlotIndex > 0))
      MSys[N->RackIndex][N->SlotIndex].SlotsUsed = SlotsUsed;
    }

  return(SUCCESS);
  }  /* END MNodeProcessConfig() */





/**
 * Display NODECFG[] values within 'showconfig'
 *
 * @see MNodeProcessConfig() - peer
 *
 * @param NS (I)
 * @param VFlag (I)
 * @param P (I) [optional]
 * @param String (O)
 */

int MNodeConfigShow(

  mnode_t   *NS,
  int        VFlag,
  mpar_t    *P,
  mstring_t *String)

  {
  enum MNodeAttrEnum AList[] = {
    mnaMaxLoad,
    mnaPrioF,
    mnaPriority,
    mnaResOvercommitFactor,
    mnaResOvercommitThreshold,
    mnaNONE };

  mbitmap_t BM;

  int aindex;
  int nindex;

  mnode_t *N;

  if (String == NULL)
    {
    return(FAILURE);
    }

  String->clear();   /* Ensure we have a null/empty string */

  mstring_t NodeString(MMAX_LINE);
  mstring_t AttrLine(MMAX_LINE);

  if (VFlag == TRUE)
    bmset(&BM,mcmVerbose);

  for (nindex = 0;nindex < MSched.M[mxoNode];nindex++)
    {
    if (NS != NULL)
      {
      if (nindex != 0)
        break;

      N = NS;
      }
    else
      {
      N = MNode[nindex];
      }

    if ((N == NULL) || (N->Name[0] == '\0'))
      break;

    if (N->Name[0] == '\1')
      continue;

    NodeString.clear();

    MStringAppendF(&NodeString,"%s[%s] ",
      MCredCfgParm[mxoNode],
      N->Name);

    for (aindex = 0;AList[aindex] != mnaNONE;aindex++)
      {
      if ((MNodeAToString(
             N,
             AList[aindex],
             &AttrLine,
             &BM) == FAILURE) ||
          (AttrLine.empty()))
        {
        continue;
        }

      MStringAppendF(&NodeString," %s=%s",
        MNodeAttr[AList[aindex]],
        AttrLine.c_str());
      }    /* END for (aindex) */

    MStringAppendF(String,"%s\n",NodeString.c_str());
    }  /* END for (nindex) */

  return(SUCCESS);
  }  /* END MNodeConfigShow() */

/* END MNodeConfig.c */
