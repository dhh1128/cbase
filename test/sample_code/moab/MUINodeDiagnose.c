/* HEADER */

/**
 * @file MUINodeDiagnose.c
 *
 * Contains MUI Node Diagnose function
 */



#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  







/**
 * Report node state, configuration, and diagnostics (handle 'mdiag -n').
 *
 * NOTE:  also handles 'mnodectl -q'
 *
 * @see MUICheckNode() - peer (handle 'checknode')
 * @see MNodeToXML() - child to handle XML requests
 * @see MUIDiagnose() - parent
 *
 * NOTE:  To add node attribute to output of 'mdiag -n --xml', add attr to 
 *        NAList[] table below.
 *
 * @param Auth (I) (FIXME: not used (except for peer) but should be)
 * @param ReqE (I) [optional] 
 * @param FilterOType (I) [optional]
 * @param FilterOID (I) [optional]
 * @param SDEP (O) [optional]
 * @param String (O) [optional]
 * @param P (I) [optional]
 * @param AString (I) access string
 * @param DiagOpt (I) [optional]
 * @param Flags (I) - Verbose,XML,Overlap [enum MCModeEnum bitmap]
 */

int MUINodeDiagnose(

  char      *Auth,
  mxml_t    *ReqE,
  enum MXMLOTypeEnum FilterOType,
  char      *FilterOID,
  mxml_t   **SDEP,
  mstring_t *String,
  mpar_t    *P,
  char      *AString,
  char      *DiagOpt,
  mbitmap_t *Flags)

  {
  int nindex;
  int index;

  mnode_t *N;  /* physical node currently being processed */
  mvm_t   *V = NULL;  /* virtual machine ID */

  mbool_t  JobIsReal = FALSE;

  mjob_t  *J;

  char     tmpLine[MMAX_BUFFER];  /* make big for possible license exports */
  char     JobName[MMAX_NAME];
  char     TString[MMAX_LINE];

  int      NodeIndex;

  int      rindex;
  int      sindex;

  mbool_t  ApplyGridSandbox = FALSE;

  int      ANC;
  int      INC;
  int      TNC;
  int      DNC;

  mjob_t *tmpJ = NULL;

  long  SameStateTime;

  mcres_t TotalCRes;
  mcres_t TotalARes;

  enum MFormatModeEnum DFormat;

  enum MAllocRejEnum   RIndex;

  mnl_t SpecNL;
  mbool_t UseFilter = FALSE;

  int   SpecClassIndex = -1;

  mxml_t *DE;

  mrm_t *PR = NULL;

  mbitmap_t RTypes;

  int    MaxRsv;

  const char *FName = "MUINodeDiagnose";

  MDB(3,fUI) MLog("%s(Auth,ReqE,FOType,FOID,SDEP,String,%s,%s,%s)\n",
    FName,
    (P != NULL) ? P->Name : "NULL",
    (AString != NULL) ? AString : "NULL",
    (DiagOpt != NULL) ? DiagOpt : "NULL");

  if ((SDEP == NULL) && (String == NULL))
    {
    return(FAILURE);
    }

  /* Step 1.0  Initialize() */

  MDB(4,fUI) MLog("INFO:     evaluating node table (%d slots)\n",
    MSched.M[mxoNode]);

  MCResInit(&TotalCRes);
  MCResInit(&TotalARes);

  if (bmisset(Flags,mcmXML))
    {
    DFormat = mfmXML;
    }
  else
    {
    DFormat = mfmNONE;
    }

  if ((DFormat == mfmNONE) &&
      (SDEP != NULL) &&
      (String == NULL))
    {
    DFormat = mfmXML;
    }

  if (DFormat == mfmXML)
    {
    if ((SDEP != NULL) && (*SDEP != NULL))
      {
      DE = *SDEP;
      }
    else
      {
      DE = NULL;

      MXMLCreateE(&DE,(char *)MSON[msonData]);

      if (SDEP != NULL)
        *SDEP = DE;
      }
    }    /* END if (DFormat == mfmXML) */
  else
    {
    if (String == NULL)
      {
      MCResFree(&TotalCRes);
      MCResFree(&TotalARes);

      return(FAILURE);
      }

    MStringSetF(String,"compute node summary\n");

    /* create header */

    if (bmisset(Flags,mcmVerbose))
      {
      /* FORMAT:                NODE  ST  PRC MEM  DISK SWAP SPD OS  ARC PARTI LOD  CLASS FEAT */

      MStringAppendF(String,"%-20s %8s %9s %13s %13s %13s %5s %7s %6s %3.3s %6s %-30s %-22s\n\n",
        "Name",
        "State",
        "  Procs  ",
        "   Memory    ",
        "    Disk     ",
        "    Swap     ",
        "Speed",
        "Opsys",
        "Arch",
        "Partition",
        "Load",
        "Classes",
        "Features");
      }
    else
      {
      MStringAppendF(String,"%-20s %8s %9s %13s %9s\n\n",
        "Name",
        "State",
        "  Procs  ",
        "   Memory    ",
        "Opsys");
      }
    }  /* END else (DFormat == mfmXML) */

  /* Step 1.1  Initialize Statistics */

  if (!strncasecmp(Auth,"peer:",strlen("peer:")))
    {
    MRMFindByPeer(Auth + strlen("peer:"),&PR,TRUE);

    if ((PR != NULL) && (PR->GridSandboxExists == TRUE))
      {
      ApplyGridSandbox = TRUE;
      }
    }
    
  TNC = 0;
  INC = 0;
  ANC = 0;
  DNC = 0;

  if ((AString != NULL) && (strstr(AString,"job:") == NULL))
    {
    char *ptr;
    char *TokPtr;

    enum MXMLOTypeEnum OType;

    void *O;

    MJobMakeTemp(&tmpJ);
 
    strcpy(tmpJ->Name,"diagnose");
 
    tmpJ->Req[0]->TaskCount = 1;
    tmpJ->SpecWCLimit[0] = 1;
 
    /* initialize credentials for temp job used to analyze node eligibility */

    tmpJ->Credential.U = NULL;
    tmpJ->Credential.G = NULL;
    tmpJ->Credential.A = NULL;
    tmpJ->Credential.Q = NULL;
    tmpJ->Credential.C = NULL;

    /* FORMAT:  user:x,group:y,... */

    /* parse AString */

    ptr = MUStrTok(AString,", \t\n",&TokPtr);

    while (ptr != NULL)
      {
      if (MCredParseName(ptr,&OType,&O) == SUCCESS)
        {
        switch (OType)
          {
          case mxoUser:  tmpJ->Credential.U = (mgcred_t *)O; break;
          case mxoGroup: tmpJ->Credential.G = (mgcred_t *)O; break;
          case mxoAcct:  tmpJ->Credential.A = (mgcred_t *)O; break;
          case mxoQOS:   tmpJ->Credential.Q = (mqos_t *)O; break;
          case mxoClass: tmpJ->Credential.C = (mclass_t *)O; break;
          default: break;
          }
        }

      ptr = MUStrTok(NULL,", \t\n",&TokPtr);
      }
    }    /* END if (AString != NULL) */
  else if (AString != NULL)
    {
    char *ptr;
    char *TokPtr = NULL;

    ptr = MUStrTok(AString,":= \n\t",&TokPtr);

    if (strcmp(ptr,"job"))
      {
      if (DFormat != mfmXML)
        {
        MStringAppendF(String,"ERROR:  job not specified\n");
        }

      MCResFree(&TotalCRes);
      MCResFree(&TotalARes);

      return(FAILURE);
      }

    ptr = MUStrTok(NULL,":= \n\t",&TokPtr);

    if (MJobFind(ptr,&J,mjsmBasic) == FAILURE)
      {
      MCResFree(&TotalCRes);
      MCResFree(&TotalARes);

      return(FAILURE);
      }

    JobIsReal = TRUE;
    } /* END else if (AString != NULL) */

  /* Step 1.2  Update Rack/Slot Info */

  /* determine max slot index */

  MNLInit(&SpecNL);

  if (!MUStrIsEmpty(DiagOpt))
    {
    /* determine if requestor specified VM */
    if (MVMFind(DiagOpt,&V) == FAILURE)
      {
      name_t NodeName;
      char const *TokPtr = DiagOpt;
      int Size;
      char *ptr;
      nindex = 0;

      while((ptr = MUStrScan(NULL,",",&Size,&TokPtr)) != NULL)
        {
        int CopySize = MIN((int)sizeof(NodeName),Size + 1);
        MUStrCpy(NodeName,ptr,CopySize);

        if (!strcmp(NodeName,"noauto"))
          continue;

        if (MNodeFind(NodeName,&N) == FAILURE)
          {
          if (DFormat != mfmXML)
            {
            MStringAppendF(String,"ERROR:  cannot locate specified resource %s\n",
              NodeName);
            }

          MJobFreeTemp(&tmpJ);

          MNLFree(&SpecNL);

          MCResFree(&TotalCRes);
          MCResFree(&TotalARes);

          return(FAILURE);
          }

        MNLSetNodeAtIndex(&SpecNL,nindex,N);
        MNLSetTCAtIndex(&SpecNL,nindex,1);
        nindex++;

        /* Only put on UseFilter if a valid node was specified */
        UseFilter = TRUE;
        }

      MNLTerminateAtIndex(&SpecNL,nindex);
      }
    }

  if ((FilterOType == mxoxTJob) && (FilterOID != NULL))
    {
    /* returns a subset of nodes that can run the psuedo job */

    MUIJobSpecGetNL(
      FilterOID,
      NULL,
      P,
      &SpecNL);  /* O */

    UseFilter = TRUE;
    }

  if (ReqE != NULL)
    {
    if ((FilterOType == mxoClass) && (FilterOID != NULL))
      {
      mclass_t *C;

      if (MClassFind(FilterOID,&C) == SUCCESS)
        SpecClassIndex = C->Index;
      }
    }
  
  /* Step 2.0  Show Node List */

  if (V != NULL)
    {
    if (DFormat == mfmXML)
      {
      mxml_t *VE = NULL;

      MXMLCreateE(&VE,(char *)MXO[mxoxVM]);

      MVMToXML(V,VE,NULL);

      MXMLAddE(DE,VE);
      }
    else
      {
      /* NOTE:  for VM query, always show verbose output */

      if (bmisset(Flags,mcmVerbose) || !bmisset(Flags,mcmVerbose))
        {
        /* FORMAT:                 NODE STA APR:CPRC AME:CMEM ADI:CDSK ASW:CSWP SPEED OPS ARC PARTI LOAD  */

        MStringAppendF(String,"%-20s %8s %4d:%-4d %6d:%-6d %6d:%-6d %6d:%-6d %5.2f %7s %6s %3.3s %6.2f ",
          V->VMID,
          (V->State != mnsNONE) ? MNodeState[V->State] : "---",
          MIN(V->ARes.Procs,V->CRes.Procs - V->DRes.Procs),
          V->CRes.Procs,
          MIN(V->ARes.Mem,V->CRes.Mem - V->DRes.Mem),
          V->CRes.Mem,
          MIN(V->ARes.Disk,V->CRes.Disk - V->DRes.Disk),
          V->CRes.Disk,
          MIN(V->ARes.Swap,V->CRes.Swap - V->DRes.Swap),
          V->CRes.Swap,
          (V->N != NULL) ? V->N->Speed : 0,
          (V->ActiveOS > 0) ? MAList[meOpsys][V->ActiveOS] : "-",
          ((V->N != NULL) && (V->N->Arch > 0)) ? MAList[meArch][V->N->Arch] : "-",
          (V->N != NULL) ? MPar[V->N->PtIndex].Name : "-",
          V->CPULoad);

        for (index = 0;tmpLine[index] != '\0';index++)
          {
          if ((tmpLine[index] == ' ') && isdigit(tmpLine[index + 1]))
            tmpLine[index] = '_';
          }  /* END for (index) */

        if (V->N != NULL)
          {
          char Line[MMAX_LINE];

          MUNodeFeaturesToString(',',&V->N->FBM,Line);

          MStringAppendF(String,"%s %-20s",
            tmpLine,
            (!MUStrIsEmpty(Line)) ? Line : "-");
          }
        }

      MJobFreeTemp(&tmpJ);

      MNLFree(&SpecNL);

      MCResFree(&TotalCRes);
      MCResFree(&TotalARes);

      return(SUCCESS);
      }
    }    /* END if (V != NULL) */
  else
    {
    mcres_t tmpCRes;

    mjob_t *JPtr;

    for (nindex = 0;nindex < MSched.M[mxoNode];nindex++)
      {
      if (UseFilter == TRUE)
        MNLGetNodeAtIndex(&SpecNL,nindex,&N);
      else
        N = MNode[nindex]; 

      if ((N == NULL) || (N->Name[0] == '\0'))
        break;

      if (N->Name[0] == '\1')
        continue;

      if ((N == MSched.GN) || (N == &MSched.DefaultN))
        {
        /* global and DEFAULT nodes displayed later */

        continue;
        }

      if ((SpecClassIndex > 0) && !bmisset(&N->Classes,SpecClassIndex))
        {
        /* node does not possess required queue/class */

        continue;
        }
     
      if ((P != NULL) && (P->Index != 0) && (P->Index != N->PtIndex))
        continue;

      /* NOTE:  filter nodes if access is 'owner' */

      /* NYI */

      if ((N->RackIndex > 0) &&
           bmisset(&MSys[N->RackIndex][N->SlotIndex].Attributes,mattrSystem))
        {
        continue;
        }

      if ((ApplyGridSandbox == TRUE) && (MNodeIsInGridSandbox(N,PR) == FALSE))
        {
        /* Only report nodes that are in the sandbox. If not in the sandbox then skip this node. */

        continue;
        }

      if (bmisset(Flags,mcmLocal) && (N->RM != NULL) && (N->RM->Type == mrmtMoab))
        {
        /* return only local nodes */

        continue;
        }

      if ((PR != NULL) && (N->HopCount >= PR->MaxNodeHop))
        {
        continue;
        }

      MDB(6,fUI) MLog("INFO:     evaluating node %s \n",
        N->Name);

      MCResInit(&tmpCRes);

      MCResGetBase(N,&N->CRes,&tmpCRes);

      bmclear(&RTypes);

      if ((N->RM == NULL) || (bmisclear(&N->RM->RTypes)))
        {
        bmset(&RTypes,mrmrtCompute);
        }
      else
        {
        bmcopy(&RTypes,&N->RM->RTypes);
        }

      rindex = N->RackIndex;
      sindex = N->SlotIndex;

      /* update per node statistics */

      if (N->PtIndex == MSched.SharedPtIndex)
        MaxRsv = MSched.MaxRsvPerGNode;
      else
        MaxRsv = MSched.MaxRsvPerNode;  

      /* update cluster statistics */

      TotalCRes.Procs += tmpCRes.Procs;
      TotalARes.Procs += MIN(N->ARes.Procs,tmpCRes.Procs - N->DRes.Procs);

      TotalCRes.Mem   += tmpCRes.Mem;
      TotalARes.Mem   += MIN(N->ARes.Mem,tmpCRes.Mem     - N->DRes.Mem);

      TotalCRes.Disk  += tmpCRes.Disk;
      TotalARes.Disk  += MIN(N->ARes.Disk,tmpCRes.Disk   - N->DRes.Disk);

      TotalCRes.Swap  += tmpCRes.Swap;
      TotalARes.Swap  += MIN(N->ARes.Swap,tmpCRes.Swap   - N->DRes.Swap);

      /* display node attributes */

      if (DFormat == mfmXML)
        {
        mbitmap_t BM;

        mxml_t *NE = NULL;

        enum MNodeAttrEnum NAList[] = {
          mnaNodeID,
          mnaArch,
          mnaAvlGRes,
          mnaCfgGRes,
          mnaAvlClass,
          mnaAvlTime,
          mnaCfgClass,
          mnaChargeRate,
          mnaDynamicPriority, 
          mnaEnableProfiling,
          mnaFeatures,
          mnaFlags,
          mnaGMetric,
          mnaHopCount,
          mnaHVType,
          mnaIsDeleted,
          mnaJobList,
          mnaLastUpdateTime,
          mnaLoad,
          mnaMaxJob,
          mnaMaxJobPerUser,
          mnaMaxProcPerUser,
          mnaMaxLoad,
          mnaMaxProc,
          mnaOldMessages,
          mnaNetAddr,
          mnaNodeIndex,
          mnaNodeState,
          mnaNodeSubState,
          mnaOperations,
          mnaOS,
          mnaOSList,
          mnaOwner,
          mnaResOvercommitFactor,
          mnaResOvercommitThreshold,
          mnaPartition,
          mnaPowerIsEnabled,
          mnaPowerPolicy,
          mnaPowerSelectState,
          mnaPowerState,
          mnaPriority,
          mnaPrioF, 
          mnaProcSpeed,
          mnaProvData,
          mnaRADisk,
          mnaRAMem,
          mnaRAProc,
          mnaRASwap,
          mnaRCDisk,
          mnaRCMem,
          mnaRCProc,
          mnaRCSwap,
          mnaRsvCount,
          mnaRsvList,
          mnaRMList,
          mnaSize,
          mnaSpeed,
          mnaSpeedW,
          mnaStatATime,
          mnaStatMTime,
          mnaStatTTime,
          mnaStatUTime,
          mnaTaskCount,
          mnaVariables,
          mnaVMOSList,
          mnaVarAttr,
          mnaNONE };

        /* list of node attributes affected by grid mapping or information access policies */

        enum MNodeAttrEnum NAList2[] = {
          mnaAvlClass,
          mnaCfgClass,
          mnaNONE };     

        /* NOTE:  MNodeToXML() includes xload and xlimit children if specified */

        bmset(&BM,mcmVerbose);

        if ((MXMLCreateE(&NE,(char *)MXO[mxoNode]) == SUCCESS) &&
            (MNodeToXML(N,NE,NAList,&BM,bmisset(Flags,mcmVerbose),TRUE,NULL,NULL) == SUCCESS))
          {
          if (AString != NULL)
            {
            /* add node access information */

            if (MNodeDiagnoseEligibility(
                  N,
                  (JobIsReal == TRUE) ? J : tmpJ,
                  (JobIsReal == TRUE) ? J->Req[0] : tmpJ->Req[0],
                  &RIndex,
                  NULL,
                  MSched.Time + MCONST_MONTHLEN,
                  NULL,
                  NULL) == FAILURE)
              { 
              MXMLSetAttr(NE,"blockreason",(void *)MAllocRejType[RIndex],mdfString);
              }
            }

          rindex = (int)N->RackIndex;
          sindex = (int)N->SlotIndex;

          if (rindex > 0)
            MXMLSetAttr(NE,(char *)MNodeAttr[mnaRack],(void *)&rindex,mdfInt);

          if (sindex > 0)
            MXMLSetAttr(NE,(char *)MNodeAttr[mnaSlot],(void *)&sindex,mdfInt);

          if (bmisset(Flags,mcmOverlap))
            {
            int ReleasePriority;

            /* calculate release priority - nodes with multiple rsv's are
             * more constrainted and should be released last */

            /* for now, simply use current active rsv count */

            ReleasePriority = MaxRsv - MNodeGetRsvCount(N,FALSE,TRUE);

            MXMLSetAttr(NE,(char *)MNodeAttr[mnaReleasePriority],(void *)&ReleasePriority,mdfInt);
            }

          MNodeToXML(N,NE,NAList2,&BM,FALSE,TRUE,PR,NULL);

          /* Usually VM info is added in MNodeToXML if mcmVerbose is set,
           * but we want it to be added regardless--but we can't do that in
           * MNodeToXML() because it is called twice! */

          if (!bmisset(Flags,mcmVerbose) && (N->VMList != NULL))
            {
            MNodeVMListToXML(N,NE);
            }

          MXMLAddE(DE,NE);
          }  /* END if ((MXMLCreateE() == SUCCESS) && ...) */
        }    /* END if (DFormat == mfmXML) */
      else
        {
        if (bmisset(Flags,mcmVerbose))
          {
          enum MNodeAttrEnum AList[] = {
            mnaNodeIndex,
            mnaNONE };

          int aindex;

          char Line[MMAX_LINE];

          /* FORMAT:                 NODE STA APR:CPRC AME:CMEM ADI:CDSK ASW:CSWP SPEED OPS ARC PARTI LOAD  */

          MStringAppendF(String,"%-20s %8s %4d:%-4d %6d:%-6d %6d:%-6d %6d:%-6d %5.2f %7s %6s %3.3s %6.2f ",
            N->Name,
            (N->State != mnsNONE) ? MNodeState[N->State] : "---",
            MIN(N->ARes.Procs,tmpCRes.Procs - N->DRes.Procs),
            tmpCRes.Procs,
            MIN(N->ARes.Mem,tmpCRes.Mem - N->DRes.Mem),
            tmpCRes.Mem,
            MIN(N->ARes.Disk,tmpCRes.Disk - N->DRes.Disk),
            tmpCRes.Disk,
            MIN(N->ARes.Swap,tmpCRes.Swap - N->DRes.Swap),
            tmpCRes.Swap,
            N->Speed,
            (N->ActiveOS > 0) ? MAList[meOpsys][N->ActiveOS] : "-",
            (N->Arch > 0) ? MAList[meArch][N->Arch] : "-",
            MPar[N->PtIndex].Name,
            N->Load);

          if (bmisclear(&N->Classes))
            {
            sprintf(tmpLine,"-");
            }
          else
            {
            mstring_t Classes(MMAX_LINE);

            MClassListToMString(&N->Classes,&Classes,NULL);

            sprintf(tmpLine,"%-30s",(Classes.empty()) ? NONE : Classes.c_str());
            }
     
          MUNodeFeaturesToString(',',&N->FBM,Line);

          MStringAppendF(String,"%s %-20s",
            tmpLine,
            (!MUStrIsEmpty(Line)) ? Line : "-");

          mstring_t tmp(MMAX_LINE);

          if (!MSNLIsEmpty(&N->CRes.GenericRes))
            {
            char *ptr;

            MSNLToMString(&N->CRes.GenericRes,NULL,",",&tmp,meGRes);

            for (ptr = tmp.c_str();*ptr != '\0';ptr++)
              {
              if (*ptr == '=')
                *ptr = ':';
              }

            MStringAppendF(String," GRES=%s",
              tmp.c_str());
            }

          for (aindex = 0;AList[aindex] != mnaNONE;aindex++)
            {
            if ((MNodeAToString(N,AList[aindex],&tmp,0) == SUCCESS) && 
                (!MUStrIsEmpty(tmp.c_str())))
              {
              MStringAppendF(String," %s=%s",
                MNodeAttr[AList[aindex]],
                tmp.c_str());
              }
            }    /* END for (aindex) */

          MStringAppend(String,"\n");
          }    /* END if (bmisset(Flags,mcmVerbose)) */
        else
          {
          MStringAppendF(String,"%-20.20s %8.8s %4d:%-4d %6d:%-6d %9.9s\n",
            N->Name,
            (N->State != mnsNONE) ? MNodeState[N->State] : "---",
            MIN(N->ARes.Procs,tmpCRes.Procs - N->DRes.Procs),
            tmpCRes.Procs,
            MIN(N->ARes.Mem,tmpCRes.Mem - N->DRes.Mem),
            tmpCRes.Mem,
            (N->ActiveOS != 0) ? MAList[meOpsys][N->ActiveOS] : "-");
          }
        }      /* END else (DFormat == mfmXML) */

      if (DFormat == mfmXML)
        {
        /* TEMP:  no messages for XML */

        MCResFree(&tmpCRes);

        continue;
        }

      /* display diagnostic messages */

      if ((N->State == mnsUnknown) || 
          (N->State == mnsUp) ||
          (N->HopCount > 0))
        {
        if ((N->State == mnsUnknown) || (N->State == mnsUp))
          {
          MCResFree(&tmpCRes);

          continue;
          }

        /* Message? (NYI) */
        }
      else if ((N->State == mnsActive) ||
               (N->State == mnsIdle) ||
               (N->State == mnsBusy))
        {
        /* check substate */

#ifdef MNOT

  /* This message should only occur if it is established that there is no job 
   * running on the node. SMJ and BOC - 12/11/08  */

        if ((N->SubState != NULL) && (!strcasecmp(N->SubState,"cpabusy")))
          {
          if (DFormat == mfmXML)
            {
            /* NYI */
            }
          else
            {
            MStringAppendF(String,"  WARNING:  node '%s' has CPA partition allocated but is not running a job\n",
              N->Name);
            }
          } 
#endif

        /* check resource utilization */

        /* NOTE:  only report warning if background utilization exceeds one processor */

        /* if N.Util > N.Ded + 1) */

        if ((tmpCRes.Procs - N->ARes.Procs) > (N->DRes.Procs + 1))
          {
          if (DFormat == mfmXML)
            {
            /* NYI */
            }
          else
            {
            MStringAppendF(String,"  WARNING:  node '%s' has more processors utilized than dedicated (%d > %d)\n",
              N->Name,
              tmpCRes.Procs - N->ARes.Procs,
              N->DRes.Procs);
            }
          }

        if ((N->ARes.Mem < (tmpCRes.Mem - N->DRes.Mem)) &&
            (N->DRes.Mem > 0))
          {
          if (DFormat == mfmXML) 
            {
            /* NYI */
            }
          else
            {
            MStringAppendF(String,"  WARNING:  node '%s' has more memory utilized than dedicated (%d > %d)\n",
              N->Name,
              tmpCRes.Mem - N->ARes.Mem,
              N->DRes.Mem);
            }
          }

        if ((N->ARes.Swap < (tmpCRes.Swap - N->DRes.Swap - MMIN_OSSWAPOVERHEAD)) &&
            (N->DRes.Swap > 0))
          {
          if (DFormat == mfmXML)
            {
            /* NYI */
            }
          else
            {
            MStringAppendF(String,"  WARNING:  node '%s' has more swap utilized than dedicated (%d > %d)\n",
              N->Name,
              tmpCRes.Swap - N->ARes.Swap,
              N->DRes.Swap);
            }
          }

        if ((N->ARes.Disk < (tmpCRes.Disk - N->DRes.Disk)) && (N->DRes.Disk > 0))
          {
          if (DFormat == mfmXML)
            {
            /* NYI */
            }
          else
            {
            MStringAppendF(String,"  WARNING:  node '%s' has more disk utilized than dedicated (%d > %d)\n",
              N->Name,
              tmpCRes.Disk - N->ARes.Disk,
              N->DRes.Disk);
            }
          }
        }    /* END else if ((N->State == mnsActive) || ...) */

      /* check update time */

      if (((MSched.Time > N->ATime) && ((MSched.Time - N->ATime) > 600)) &&
           (bmisset(&N->IFlags,mnifRMDetected)))
        {
        if (DFormat == mfmXML)
          {
          /* NYI */
          }
        else
          {
          MULToTString(MSched.Time - N->ATime,TString);

          MStringAppendF(String,"  WARNING:  node '%s' has not been updated in %s\n",
            N->Name,
            TString);
          }
        }

      /* check node memory */

      if ((N->RackIndex != -1) &&
          (MRack[N->RackIndex].Memory > 0) &&
          (tmpCRes.Mem != MRack[N->RackIndex].Memory))
        {
        if (DFormat == mfmXML)
          {
          /* NYI */
          }
        else
          {
          MStringAppendF(String,"  WARNING:  node '%s' memory (%d MB) does not match rack %d memory (%d MB)\n",
            N->Name,
            tmpCRes.Mem,
            N->RackIndex,
            MRack[N->RackIndex].Memory);
          }
        }

      /* check swap space */

      if ((tmpCRes.Swap > 0) && (N->ARes.Swap < MDEF_MINSWAP))
        {
        if (DFormat == mfmXML)
          {
          /* NYI */
          }
        else
          {
          MStringAppendF(String,"  WARNING:  node '%s' swap space is low (%d MB)\n",
            N->Name,
            N->ARes.Swap);
          }
        }

      /* check memory */

      if ((tmpCRes.Mem > 10) && (N->ARes.Mem < 10))
        {
        if (DFormat == mfmXML)
          {
          /* NYI */
          }
        else
          {
          MStringAppendF(String,"  WARNING:  node '%s' memory is low (%d MB)\n",
            N->Name,
            N->ARes.Mem);
          }
        }

      /* check state */

      SameStateTime = MSched.Time - N->StateMTime;

      if ((N->State != N->EState) && (SameStateTime > 120))
        {
        if (DFormat == mfmXML)
          {
          /* NYI */
          }
        else
          {
          char DString[MMAX_LINE];

          MULToTString(N->SyncDeadLine - MSched.Time,TString);
          MULToDString((mulong *)&N->SyncDeadLine,DString);

          MStringAppendF(String,"  WARNING:  node '%s' state '%s' does not match expected state '%s'.  sync deadline in %s at %s",
            N->Name,
            MNodeState[N->State],
            MNodeState[N->EState],
            TString,
            DString);
          }
        }

      JPtr = NULL;

      if ((N->State == mnsBusy) || (N->State == mnsActive))
        {
        mhashiter_t HTI;

        MUHTIterInit(&HTI);

        while (MUHTIterate(&MAQ,NULL,(void **)&J,NULL,&HTI) == SUCCESS)
          {
          for (NodeIndex = 0;MNLGetNodeAtIndex(&J->NodeList,NodeIndex,NULL) == SUCCESS;NodeIndex++)
            {
            if (MNLReturnNodeAtIndex(&J->NodeList,NodeIndex) == N)
              {
              JPtr = J;

              break;
              }
            }

          if (JPtr != NULL)
            break;
          }  /* END for (jindex) */

        JobName[0] = '\0';

        if ((JPtr == NULL) && 
            ((N->RM == NULL) || (N->RM->Type != mrmtMoab)))
          {
          /* peer moab's may report local utilization */

          if (DFormat == mfmXML)
            {
            /* NYI */
            }
          else
            {
            MStringAppendF(String,"  WARNING:  node '%s' is busy/running but not assigned to an active job\n",
              N->Name);
            }

          JobName[0] = '\0';
          }
        else if (JPtr != NULL)
          {
          MUStrCpy(JobName,JPtr->Name,sizeof(JobName));
          }
        }    /* END if (N->State == mnsBusy) */

      /* check disk space */

      if ((tmpCRes.Disk > 0) && (N->ARes.Disk <= 0) && (N->State != mnsBusy))
        {
        if (DFormat == mfmXML)
          {
          /* NYI */
          }
        else
          {
          MStringAppendF(String,"  WARNING:  node '%s' disk space is full (%d of %d MB available)\n",
            N->Name,
            N->ARes.Disk,
            tmpCRes.Disk);
          }
        }

      MCResFree(&tmpCRes);

      /* check reservations */

      if (DFormat != mfmXML)
        {
        mbitmap_t BM;

        mstring_t tmpBuf(MMAX_LINE);

        bmset(&BM,mcmSummary);

        MNodeDiagnoseRsv(N,&BM,&tmpBuf);

        if (!tmpBuf.empty())
          {
          MStringAppendF(String,"%s",
            tmpBuf.c_str());
          }
        }  /* END if (DFormat != mfmXML) */

      /* check CPU usage */

      if ((N->State == mnsIdle) && (SameStateTime > 600) && (N->Load > 0.50) )
        {
        if (DFormat == mfmXML)
          {
          /* NYI */
          }
        else
          {
          MULToTString(SameStateTime,TString);

          MStringAppendF(String,"  WARNING:  node '%s' has been idle for %s but load is HIGH.  load: %6.3f (check for runaway processes?)\n",
            N->Name,
            TString,
            N->Load);
          }
        }

      if (bmisset(&RTypes,mrmrtCompute))
        {
        if (!bmisset(&N->IFlags,mnifRMDetected))
          {
          if (DFormat == mfmXML)
            {
            /* NYI */
            }
          else
            {
            MStringAppendF(String,"  WARNING:  node '%s' referenced by either jobs, reservations, or config but not reported by RM\n",
              N->Name);
            }
          }
        else if (N->CRes.Procs <= 0)
          {
          if (MSNLIsEmpty(&N->CRes.GenericRes))
            {
            if (DFormat == mfmXML)
              {
              /* NYI */
              }
            else
              {
              if (N->State != mnsDown)
                {
                MStringAppendF(String,"  WARNING:  node '%s' has no configured processors\n",
                  N->Name);
                }
              }
            }
           }
        else if (N->Load > (N->CRes.Procs * 2))
          {
          if (DFormat == mfmXML)
            {
            /* NYI */
            }
          else
            {
            MStringAppendF(String,"  WARNING:  node '%s' has excessive load (state: '%s'  load: %6.3f)\n",
              N->Name,
              MNodeState[N->State],
              N->Load);
            }
          }
        }    /* END if (bmisset(&RTypes,mrmrtCompute)) */

      if (((N->State == mnsBusy) || (N->State == mnsActive)) &&
          (SameStateTime > 900) && ((N->Load / N->CRes.Procs) < 0.20) &&
          ((N->RM == NULL) || ((N->RM->SubType != mrmstXT3) && (N->RM->SubType != mrmstXT4))))
        {
        if (DFormat == mfmXML)
          {
          /* NYI */
          }
        else
          {
          MULToTString(SameStateTime,TString);

          MStringAppendF(String,"  WARNING:  node '%s' is active for %s but load is LOW.  load: %6.3f",
            N->Name,
            TString,
            N->Load);

          if (JobName[0] != '\0')
            {
            MStringAppendF(String," (check job %s?)\n",
              JobName);
            }
          else
            {
            MStringAppendF(String,"\n");
            }
          }
        }

      TNC++;

      switch (N->State)
        {
        case mnsIdle:

          INC++;

          break;

        case mnsBusy:
        case mnsActive:
        case mnsDraining:

          if (MSched.BluegeneRM == TRUE)
            ANC += N->DRes.Procs;
          else
            ANC++;

          break;

        case mnsReserved:

          if ((N->RM != NULL) && (N->RM->Type == mrmtLL))
            {
            MStringAppendF(String,"  WARNING:  node '%s' is reserved by an interactive POE job\n",
              N->Name);
            }

          if (MSched.BluegeneRM == TRUE)
            DNC += N->CRes.Procs;
          else
            DNC++;

          break;

        case mnsDown:
        case mnsDrained:
        case mnsFlush:
        case mnsNONE:

          if (MSched.BluegeneRM == TRUE)
            DNC += N->CRes.Procs;
          else
            DNC++;

          break;

        default:

          MStringAppendF(String,"  WARNING:  node '%s' has unexpected node state (%d)\n",
            N->Name,
            N->State);

          if (MSched.BluegeneRM == TRUE)
            DNC += N->CRes.Procs;
          else
            DNC++;

          break;
        }  /* END switch (N->State) */

      if ((bmisset(Flags,mcmVerbose2)) &&
          (N->VMList != NULL))
        {
        mstring_t tmpString(MMAX_LINE);

        /* report node's virtual machine status */

        MVMShowState(&tmpString,N);

        MStringAppend(String,tmpString.c_str());
        }    /* END if (bmisset(Flags,mcmVerbose2)) */
      }  /* END for (nindex = 0;nindex < MSched.M[mxoNode];nindex++)  */
    }    /* END else (V != NULL) */

  if (MSched.BluegeneRM == TRUE)
    {
    /* Show how many nodes there are in a system, not how many midplanes.
     * 1 Midplane = 512 c-nodes. Moab sees 1 node as 512 procs. */

    TNC *= MSched.BGBasePartitionNodeCnt;
    ANC /= MSched.BGNodeCPUCnt;  /* Total Available Procs / # of nodes per c-node */
    DNC /= MSched.BGNodeCPUCnt;
    INC  = TNC - ANC - DNC;
    }

  /* display smp information */

  if ((bmisset(&MPar[0].Flags,mpfSharedMem)) && 
      (MPar[0].NodeSetPriorityType == mrspMinLoss) &&
      (DFormat != mfmXML) &&
      (bmisset(Flags,mcmVerbose)))
    {
    char smpBuffer[MMAX_BUFFER];

    MSMPDiagnose(smpBuffer,sizeof(smpBuffer));

    MStringAppend(String,"\n");
    MStringAppend(String,smpBuffer);
    }

  if (DFormat != mfmXML)
    {
    /* display cluster summary */

    if (bmisset(Flags,mcmVerbose))
      {
      MStringAppendF(String,"%-20s %8s %4d%s:%-4d%s %6d:%-6d %6d:%-6d %6d:%-6d\n",
        "-----",
        "---",
        (bmisset(&MSched.DisplayFlags,mdspfCondensed) && (TotalARes.Procs % 1024 == 0)) ? TotalARes.Procs >> 10 : TotalARes.Procs,
        (bmisset(&MSched.DisplayFlags,mdspfCondensed) && (TotalARes.Procs % 1024 == 0)) ? "k" : "",
        (bmisset(&MSched.DisplayFlags,mdspfCondensed) && (TotalCRes.Procs % 1024 == 0)) ? TotalCRes.Procs >> 10 : TotalCRes.Procs,
        (bmisset(&MSched.DisplayFlags,mdspfCondensed) && (TotalCRes.Procs % 1024 == 0)) ? "k" : "",
        TotalARes.Mem,
        TotalCRes.Mem,
        TotalARes.Disk,
        TotalCRes.Disk,
        TotalARes.Swap,
        TotalCRes.Swap);
      }
    else
      {
      MStringAppendF(String,"%-20s %8s %4d%s:%-4d%s %6d:%-6d %9.9s\n",
        "-----",
        "---",
        (bmisset(&MSched.DisplayFlags,mdspfCondensed) && (TotalARes.Procs % 1024 == 0)) ? TotalARes.Procs >> 10 : TotalARes.Procs,
        (bmisset(&MSched.DisplayFlags,mdspfCondensed) && (TotalARes.Procs % 1024 == 0)) ? "k" : "",
        (bmisset(&MSched.DisplayFlags,mdspfCondensed) && (TotalCRes.Procs % 1024 == 0)) ? TotalCRes.Procs >> 10 : TotalCRes.Procs,
        (bmisset(&MSched.DisplayFlags,mdspfCondensed) && (TotalCRes.Procs % 1024 == 0)) ? "k" : "",
        TotalARes.Mem,
        TotalCRes.Mem,
        "-----");
      }  /* END else */

    MStringAppendF(String,"\nTotal Nodes: %d%s  (Active: %d%s  Idle: %d%s  Down: %d%s)\n",
      (bmisset(&MSched.DisplayFlags,mdspfCondensed) && (TNC % 1024 == 0)) ? TNC >> 10 : TNC,
      (bmisset(&MSched.DisplayFlags,mdspfCondensed) && (TNC % 1024 == 0)) ? "k" : "",
      (bmisset(&MSched.DisplayFlags,mdspfCondensed) && (ANC % 1024 == 0)) ? ANC >> 10 : ANC,
      (bmisset(&MSched.DisplayFlags,mdspfCondensed) && (ANC % 1024 == 0)) ? "k" : "",
      (bmisset(&MSched.DisplayFlags,mdspfCondensed) && (INC % 1024 == 0)) ? INC >> 10 : INC,
      (bmisset(&MSched.DisplayFlags,mdspfCondensed) && (INC % 1024 == 0)) ? "k" : "",
      (bmisset(&MSched.DisplayFlags,mdspfCondensed) && (DNC % 1024 == 0)) ? DNC >> 10 : DNC,
      (bmisset(&MSched.DisplayFlags,mdspfCondensed) && (DNC % 1024 == 0)) ? "k" : "");
    }
  else
    {
    /* NYI */
    }

  MCResFree(&TotalCRes);
  MCResFree(&TotalARes);

  /* Don't report global node to slave's master, 
   * but return to slave client request */
  if (((!ISMASTERSLAVEREQUEST(PR,MSched.Mode)) && (MSched.GN != NULL)) || 
      (!MUStrIsEmpty(DiagOpt) && !strcasecmp(DiagOpt,"DEFAULT")))
    {
    /* non-default global resources set */

    if (DFormat == mfmXML)
      {
      mbitmap_t BM;

      mxml_t *NE = NULL;

      enum MNodeAttrEnum AList[] = {
        mnaNodeID,
        mnaArch,
        mnaAvlGRes,
        mnaCfgGRes,
        mnaAvlTime,
        mnaEnableProfiling,
        mnaFeatures,
        mnaLastUpdateTime,
        mnaMaxJob,
        mnaMaxJobPerUser,
        mnaMaxProcPerUser,
        mnaMaxLoad,
        mnaMaxProc,
        mnaNetAddr,
        mnaNodeState,
        mnaOldMessages,
        mnaOS,
        mnaOSList,
        mnaOwner,
        mnaPartition,
        mnaPowerPolicy,
        mnaPriority,
        mnaPrioF, 
        mnaProcSpeed,
        mnaRADisk,
        mnaRAMem,
        mnaRAProc,
        mnaRASwap,
        mnaRCDisk,
        mnaRCMem,
        mnaRCProc,
        mnaRCSwap,
        mnaRsvCount,
        mnaResOvercommitFactor,
        mnaResOvercommitThreshold,
        mnaRMList,
        mnaRsvList,
        mnaSize,
        mnaSpeed,
        mnaSpeedW,
        mnaStatATime,
        mnaStatMTime,
        mnaStatTTime,
        mnaStatUTime,
        mnaTaskCount,
        mnaVMOSList,
        mnaNONE };

      bmset(&BM,mcmVerbose);

      if ((!MUStrIsEmpty(DiagOpt)) && !strcasecmp(DiagOpt,"DEFAULT"))
        {
        /* NOTE:  MNodeToXML() includes xload and xlimit children if specified */

        if ((MXMLCreateE(&NE,(char *)MXO[mxoNode]) == SUCCESS) &&
            (MNodeToXML(&MSched.DefaultN,NE,AList,&BM,bmisset(Flags,mcmVerbose),TRUE,NULL,NULL) == SUCCESS))
          {
          MXMLAddE(DE,NE);
          }
        }
      else
        {
        /* NOTE:  MNodeToXML() includes xload and xlimit children if specified */

        if ((MXMLCreateE(&NE,(char *)MXO[mxoNode]) == SUCCESS) &&
            (MNodeToXML(MSched.GN,NE,AList,&BM,bmisset(Flags,mcmVerbose),TRUE,NULL,NULL) == SUCCESS))
          {
          if (AString != NULL)
            {
            /* add node access information */

            if (MNodeDiagnoseEligibility(
                  MSched.GN,
                  (JobIsReal == TRUE) ? J : tmpJ,
                  (JobIsReal == TRUE) ? J->Req[0] : tmpJ->Req[0],
                  &RIndex,
                  NULL,
                  MSched.Time + MCONST_MONTHLEN,
                  NULL,
                  NULL) == FAILURE)
              {
              MXMLSetAttr(NE,"blockreason",(void *)MAllocRejType[RIndex],mdfString);
              }
            }
          }

        MXMLAddE(DE,NE);
        }  /* END else (!strcasecmp(DiagOpt,"DEFAULT")) */
      }    /* END if (DFormat == mfmXML) */
    else if (MSched.GN != NULL)
      {
      char *ptr;

      MStringAppend(String,"\n\n");

      ptr = MCResToString(&MSched.GN->CRes,0,mfmNONE,NULL);

      if (!strcmp(ptr,NONE))
        strcpy(ptr,"---");

      MStringAppendF(String,"NODE[%s] Config Res     %s\n",
        MDEF_GNNAME,
        ptr);

      ptr = MCResToString(&MSched.GN->DRes,0,mfmNONE,NULL);

      if (!strcmp(ptr,NONE))
        strcpy(ptr,"---");

      MStringAppendF(String,"NODE[%s] Dedicated Res  %s\n",
        MDEF_GNNAME,
        ptr);

      ptr = MCResToString(&MSched.GN->ARes,0,mfmNONE,NULL);

      if (!strcmp(ptr,NONE))
        strcpy(ptr,"---");

      MStringAppendF(String,"NODE[%s] Available Res  %s\n",
        MDEF_GNNAME,
        ptr);

      MStringAppend(String,"\n\n");
      }
    }    /* END if ((MSched.GN != NULL) || ...) */

  if (DFormat == mfmXML)
    {
    if (String != NULL)
      {
      if (MXMLToMString(DE,String,NULL,TRUE) == FAILURE)
        {
        MDB(2,fUI) MLog("ALERT:    buffer truncated in %s",
          FName);
        }
      }

    if ((SDEP != NULL) && (*SDEP == NULL))
      *SDEP = DE;

    if (SDEP == NULL)
      MXMLDestroyE(&DE);
    }  /* END if (DFormat == mfmXML) */

  MJobFreeTemp(&tmpJ);

  MNLFree(&SpecNL);

  return(SUCCESS);
  }  /* END MUINodeDiagnose() */
/* END MUINodeDiagnose.c */
