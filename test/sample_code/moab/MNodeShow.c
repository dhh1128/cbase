/* HEADER */

      
/**
 * @file MNodeShow.c
 *
 * Contains: MNodeShowState()
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 * Report node state to log.
 *
 * @see MNodeShowState() - peer - report node state for 'checknode'
 *
 * @param N (I)
 */

int MNodeShow(
 
  mnode_t *N)  /* I */
 
  {
  char tmpLine[MMAX_LINE];

  mstring_t Classes(MMAX_LINE);

  const char *FName = "MNodeShow";

  MDB(9,fSTRUCT) MLog("%s(%s)\n",
    FName,
    (N->Name[0] == '\0') ? "NONE" : N->Name);
 
  MClassListToMString(&N->Classes,&Classes,NULL);

  MLogNH("[%03d] %s: (P:%d,S:%d,M:%d,D:%d) [%s][%5s][%5s]<%f> C:%s ",
    N->Index,
    N->Name,
    N->ARes.Procs,
    N->ARes.Swap,
    N->ARes.Mem,
    N->ARes.Disk,
    MNodeState[N->State],
    MAList[meOpsys][N->ActiveOS],
    MAList[meArch][N->Arch],
    N->Load,
    (Classes.empty()) ? NONE : Classes.c_str());

  MLogNH("%s ",MUNodeFeaturesToString(',',&N->FBM,tmpLine));
  MLogNH("\n");
 
  return(SUCCESS);
  }  /* END MNodeShow() */





 
/**
 * Report node config, health, and state of specified node (handle 'checknode' request).
 *
 * @see MUINodeShow() - parent
 * @see MNodeDiagnoseState() - peer
 * @see MUINodeDiagnose() - peer (handle 'mdiag -n')
 *   NOTE:  update NAList[] in MUINodeDiagnose() to add attributes 
 *
 * NOTE:  This routine does not provide output in XML.  For node attribute
 *        reporting in XML, see MUINodeDiagnose()
 *
 * @param N (I)
 * @param Flags (I) [bitmap of enum MCModeEnum]
 * @param Buffer (O)
 */

int MCheckNode(

  mnode_t                *N,        /* I */
  mbitmap_t              *Flags,    /* I (bitmap of enum MCModeEnum) */
  mstring_t              *Buffer)   /* O (already initialized) */

  {
  char  tmpLine[MMAX_LINE];
  char  TString[MMAX_LINE];

  mbool_t IsCompute;
  mbool_t IsStorage;

  const char *FName = "MNodeShowState";

  MDB(5,fSTRUCT) MLog("%s(%s,Flags,Buffer)\n",
    FName,
    (N != NULL) ? N->Name : "NULL");

  if ((N == NULL) || (Buffer == NULL))
    {
    return(FAILURE);
    }

  IsCompute = FALSE;
  IsStorage = FALSE;

  /* NOTE:  compute node's master RM may be provisioning RM which also provides
            asset manager/inventory services */

  if (N == &MSched.DefaultN)
    {
    /* NO-OP */
    }
  else if ((N->RM == NULL) || 
           (bmisclear(&N->RM->RTypes)) || 
           bmisset(&N->RM->RTypes,mrmrtCompute) ||
           bmisset(&N->RM->RTypes,mrmrtProvisioning))
    {
    IsCompute = TRUE;
    }
  else if (bmisset(&N->RM->RTypes,mrmrtStorage))
    {
    IsStorage = TRUE;
    }

  MStringAppendF(Buffer,"node %s\n\n",
    N->Name);

  if (N != &MSched.DefaultN)
    {
    MULToTString((long)MSched.Time - N->StateMTime,TString);

    if (N->SubState != NULL)
      {
      MStringAppendF(Buffer,"State:  %8s:%s  (in current state for %s)\n",
        MNodeState[N->State],
        N->SubState,
        TString);
      }
    else
      {
      MStringAppendF(Buffer,"State:  %8s  (in current state for %s)\n",
        MNodeState[N->State],
        TString);
      }
    }

  if (N->State != N->EState)
    {
    char DString[MMAX_LINE];

    MULToDString((mulong *)&N->SyncDeadLine,DString);

    MStringAppendF(Buffer,"Expected State: %8s   SyncDeadline: %s",
      MNodeState[N->EState],
      DString);
    }

  /* display consumed resources */

  if (N != &MSched.DefaultN)
    {
    char   *ptr;

    mcres_t URes;

    MCResInit(&URes);

    if (MSched.ResOvercommitSpecified[0] == FALSE)
      {
      ptr = MCResToString(&N->CRes,0,mfmHuman,NULL);

      if ((ptr != NULL) && !strcmp(ptr,NONE))
        ptr = NULL;

      MStringAppendF(Buffer,"Configured Resources: %s\n",
        (ptr != NULL) ? ptr : "---");

      MCResCopy(&URes,&N->CRes);

      MCResRemove(&URes,&N->CRes,&N->ARes,1,FALSE);

      ptr = MCResToString(&URes,0,mfmHuman,NULL);

      if ((ptr != NULL) && !strcmp(ptr,NONE))
        ptr = NULL;

      MStringAppendF(Buffer,"Utilized   Resources: %s\n",
        (ptr != NULL) ? ptr : "---");

      ptr = MCResToString(&N->DRes,0,mfmHuman,NULL);
  
      if ((ptr != NULL) && !strcmp(ptr,NONE))
        ptr = NULL;

      MStringAppendF(Buffer,"Dedicated  Resources: %s\n",
        (ptr != NULL) ? ptr : "---");

      if (bmisset(Flags,mcmVerbose2))
        {
        MCResClear(&URes);

        MNodeSumJobUsage(N,&URes);

        ptr = MCResToString(&URes,0,mfmHuman,NULL);

        MStringAppendF(Buffer,"Not dedicated to jobs: %s\n",
          (ptr != NULL) ? ptr : "---");
        }

      if (N->XRes != NULL)
        {
        ptr = MCResToString(N->XRes,0,mfmHuman,NULL);
  
        if ((ptr != NULL) && !strcmp(ptr,NONE))
          ptr = NULL;

        MStringAppendF(Buffer,"Externally Used Resources: %s\n",
          (ptr != NULL) ? ptr : "---");
        }
      }
    else
      {
      mcres_t tmpCRes;

      MCResInit(&tmpCRes);

      MCResGetBase(N,&N->CRes,&tmpCRes);

      ptr = MCResToString(&tmpCRes,0,mfmHuman,NULL);

      if ((ptr != NULL) && !strcmp(ptr,NONE))
        ptr = NULL;

      MStringAppendF(Buffer,"Configured Resources: %s\n",
        (ptr != NULL) ? ptr : "---");

      /* utilized should not be based off of adjusted */

      MCResCopy(&URes,&tmpCRes);

      MCResRemove(&URes,&N->CRes,&N->ARes,1,FALSE);

      ptr = MCResToString(&URes,0,mfmHuman,NULL);

      if ((ptr != NULL) && !strcmp(ptr,NONE))
        ptr = NULL;

      MStringAppendF(Buffer,"Utilized   Resources: %s\n",
        (ptr != NULL) ? ptr : "---");

      ptr = MCResToString(&N->DRes,0,mfmHuman,NULL);

      if ((ptr != NULL) && !strcmp(ptr,NONE))
        ptr = NULL;

      MStringAppendF(Buffer,"Dedicated  Resources: %s\n",
        (ptr != NULL) ? ptr : "---");

      MCResFree(&tmpCRes);
      }

    if (!MSNLIsEmpty(&N->CRes.GenericRes))
      {
      int gindex;

      for (gindex = 1;gindex < MSched.M[mxoxGRes];gindex++)
        {
        if (MUStrIsEmpty(MGRes.Name[gindex]))
          break;

        if ((MSNLGetIndexCount(&N->CRes.GenericRes,gindex) > 0) && (MGRes.GRes[gindex]->StartDelayDate > MSched.Time))
          {
          char DString[MMAX_LINE];

          MULToTString((long)MGRes.GRes[gindex]->StartDelayDate - MSched.Time,DString);

          MStringAppendF(Buffer,"StartDelay active on '%s' for:        %s\n",MGRes.Name[gindex],DString);
          }
        }
      }  /* END if (!MSNLIsEmpty(N->CRes.GenericRes)) */

    if (N->ChargeRate != 0.0)
      {
      MStringAppendF(Buffer,"Charge Rate:        %.2f\n",
        N->ChargeRate);
      }

    if (N->AttrList.size() != 0)
      {
      MStringAppend(Buffer,"Attributes:         ");

      for (attr_iter it = N->AttrList.begin();it != N->AttrList.end();++it)
        {
        MStringAppendF(Buffer,"%s%s%s ",
          (*it).first.c_str(),
          MUStrIsEmpty((*it).second.c_str()) ? "" : "=",
          MUStrIsEmpty((*it).second.c_str()) ? "" : (*it).second.c_str());
        } 

      MStringAppend(Buffer,"\n");
      }

    if (N->ACL != NULL)
      {
      mbitmap_t BM;

      tmpLine[0] = '\0';

      bmset(&BM,mfmHuman);
      bmset(&BM,mfmVerbose);

      MACLListShow(N->ACL,maNONE,&BM,tmpLine,sizeof(tmpLine));

      if (tmpLine[0] != '\0')
        {
        MStringAppendF(Buffer,"ACL:        %s\n",
          tmpLine);
        }
      }

    if (N->XLoad != NULL)
      {
      mbitmap_t BM;

      mstring_t tmp(MMAX_LINE);

      bmset(&BM,mcmUser);

      MNodeAToString(N,mnaGMetric,&tmp,&BM);

      if (!MUStrIsEmpty(tmp.c_str()))
        {
        MStringAppendF(Buffer,"Generic Metrics:    %s\n",
          tmp.c_str());
        }
      }

    if ((N->PowerPolicy != mpowpNONE) || 
        (MSched.DefaultN.PowerPolicy != mpowpNONE))
      {
      if (N->PowerSelectState != N->PowerState)
        {
        MStringAppendF(Buffer,"Power State:        %s (state %s selected)  Power Policy:       %s %s\n",
          MPowerState[N->PowerState],
          MPowerState[N->PowerSelectState],
          (N->PowerPolicy != mpowpNONE) ? MPowerPolicy[N->PowerPolicy] : MPowerPolicy[MSched.DefaultN.PowerPolicy],
          (N->PowerPolicy != mpowpNONE) ? "" : "(global policy)");
        }
      else
        {
        MStringAppendF(Buffer,"Power State:        %s  Power Policy:       %s %s\n",
          MPowerState[N->PowerState],
          (N->PowerPolicy != mpowpNONE) ? MPowerPolicy[N->PowerPolicy] : MPowerPolicy[MSched.DefaultN.PowerPolicy],
          (N->PowerPolicy != mpowpNONE) ? "" : "(global policy)");
        }
      }
    else if (N->PowerState != mpowNONE)
      {
      MStringAppendF(Buffer,"Power State:   %s\n",
        MPowerState[N->PowerState]);
      }

#ifdef MNOT
    /* N->PowerIsEnabled SHOULD be deprecated in Moab 5.4+ */

    if (N->PowerIsEnabled != MBNOTSET)
      {
      MStringAppendF(Buffer,"Power:      %s\n",
        (N->PowerIsEnabled == TRUE) ? "On" : "Off");
      }
#endif /* MNOT */

    if (MGEventGetItemCount(&N->GEventsList) > 0)
      {
      char              *GEName;
      mgevent_obj_t     *GEvent;
      mgevent_iter_t     GEIter;

      MGEventIterInit(&GEIter);

      while (MGEventItemIterate(&N->GEventsList,&GEName,&GEvent,&GEIter) == SUCCESS)
        {
        mgevent_desc_t *GEDesc = NULL;
        mulong          tmpReArm = 0;

        if (MGEventGetDescInfo(GEName,&GEDesc) == SUCCESS)
          {
          tmpReArm = GEDesc->GEventReArm;
          }

        if (GEvent->GEventMTime + MAX(5 * MCONST_MINUTELEN,tmpReArm) < MSched.Time)
          {
          /* do not show old events */

          continue;
          }

        MStringAppendF(Buffer,"GenericEvent[%s]='%s'\n",
          GEName,
          GEvent->GEventMsg);
        } /* END while (MGEventItemIterate(&N->GEvents,...) == SUCCESS) */
      } /* END if (MGEventGetItemCount(&N->GEventsList) > 0) */

    if (N->Stat.FCurrentTime > 0)
      {
      mulong MTBFL;
      mulong MTBF24;

      char   tmpLine[MMAX_NAME];

      if (N->Stat.FCount[MMAX_BCAT - 1] > 0)
        MTBFL = (MSched.Time - MSched.FStatInitTime) / N->Stat.FCount[MMAX_BCAT - 1];
      else 
        MTBFL = MCONST_EFFINF;

      if (N->Stat.OFCount[mtbf8h] + N->Stat.OFCount[mtbf16h] > 0)
        MTBF24 = MCONST_DAYLEN / (N->Stat.OFCount[mtbf8h] + N->Stat.OFCount[mtbf16h]);
      else
        MTBF24 = MCONST_EFFINF;

      MULToTString(MTBFL,TString);

      strcpy(tmpLine,TString);

      MULToTString(MTBF24,TString);

      MStringAppendF(Buffer,"  MTBF(longterm): %s  MTBF(24h): %s\n",
        tmpLine,
        TString);

      if (N->Stat.OFCount[mtbf15m] + N->Stat.FCount[mtbf15m] > 0)
        {
        /* if recent failures occurred, report additional info */

        int tindex;

        for (tindex = 0;tindex < mtbfLAST;tindex++)
          {
          MULToTString(900 << tindex,TString);

          if (N->Stat.OFCount[tindex] != 0)
            MStringAppendF(Buffer,"  NOTE:  %d failures in last %s\n",
              N->Stat.OFCount[tindex],
              TString);
          }  /* END for (tindex) */
        } 
      }      /* END if (N->NP.FCurrentTime > 0) */

    if (N->AllocRes != NULL)
      {
      MULLToString(N->AllocRes,FALSE,NULL,tmpLine,sizeof(tmpLine));

      MStringAppendF(Buffer,"Allocated Resources:  %s\n",
        tmpLine);
      }

    if (N->FSys != NULL)
      {
      int fsindex;

      for (fsindex = 0;fsindex < MMAX_FSYS;fsindex++)
        {
        if (N->FSys->Name[fsindex] == NULL)
          break;

        if (N->FSys->Name[fsindex] == (char *)1)
          continue;

        MStringAppendF(Buffer,"FS[%s]  Size=%d/%d  IO=%.2f/%.2f\n",
          N->FSys->Name[fsindex],
          N->FSys->ASize[fsindex],
          N->FSys->CSize[fsindex],
          N->FSys->AIO[fsindex],
          N->FSys->CMaxIO[fsindex]);
        }  /* END for (fsindex) */
      }    /* END if (N->FSys != NULL) */

    if (!MUHTIsEmpty(&N->Variables))
      {
      mstring_t tmp(MMAX_LINE);

      /* display opaque attributes */

      MNodeAToString(N,mnaVariables,&tmp,0);

      MStringAppendF(Buffer,"Vars:       %s\n",
        tmp.c_str());
      }

    MCResFree(&URes);
    }      /* END if (N != &MSched.DefaultN) */
  else
    {
    /* show configured resources */

    char   *ptr;

    ptr = MCResToString(&N->CRes,0,mfmHuman,NULL);

    if ((ptr != NULL) && !strcmp(ptr,NONE))
      ptr = NULL;

    MStringAppendF(Buffer,"Configured Resources: %s\n",
      (ptr != NULL) ? ptr : "---");
    }

  if (IsCompute == TRUE)
    {
    mstring_t tmp(MMAX_LINE);

    if (N->ResOvercommitFactor != NULL)
      {
      MNodeAToString(N,mnaAllocationLimits,&tmp,0);

      if ((N != &MSched.DefaultN) || (!MUStrIsEmpty(tmp.c_str())))
        {
        MStringAppendF(Buffer,"Overcommit Factor:  %s\n",
          tmp.c_str());
        }
      }

    if (N->ResOvercommitThreshold != NULL)
      {
      MNodeAToString(N,mnaUtilizationThresholds,&tmp,0);

      if ((N != &MSched.DefaultN) || (!MUStrIsEmpty(tmp.c_str())))
        {
        MStringAppendF(Buffer,"Overcommit Threshold:  %s\n",
          tmpLine);
        }
      }

    if (MSched.ManageTransparentVMs == TRUE)
      {
      if (N != &MSched.DefaultN) 
        {
        if (!bmisset(&N->IFlags,mnifCanCreateVM))
          {
          MStringAppendF(Buffer,"NOTE:  node does not support dynamic VM creation\n");
          }
        }
      }

    if (N->HVType != mhvtNONE)
      {
      MStringAppendF(Buffer,"HVType:     %s\n",
        MHVType[N->HVType]);
      }

    if (N->VMList != NULL)
      {
      char tmpLine[MMAX_LINE];

      MULLToString(
        N->VMList,
        FALSE,
        ",",
        tmpLine,
        sizeof(tmpLine));

      MStringAppendF(Buffer,"VMList:     %s\n",
        tmpLine);

      if (bmisset(Flags,mcmVerbose2))
        {
        MVMShowState(Buffer,N);
        }
      }      /* END if (N->VMList != NULL) */

    MStringAppendF(Buffer,"Opsys:      %-8s  Arch:      %-6s\n",
      (N->ActiveOS > 0) ? MAList[meOpsys][N->ActiveOS] : "---",
      (N->Arch > 0)     ? MAList[meArch][N->Arch] : "---");

    if ((N->NextOS != 0) && (N->NextOS != N->ActiveOS))
      {
      int oindex;

      const mnodea_t *OSList = (N->OSList != NULL) ? N->OSList : MSched.DefaultN.OSList;

      mulong ProvDuration;

      char PStartTime[MMAX_NAME];
      char PEndTime[MMAX_NAME];

      assert((N->OSList != NULL) || (MSched.DefaultN.OSList != NULL));

      ProvDuration = MSched.DefProvDuration;

      for (oindex = 0;(OSList[oindex].AIndex != 0) && (oindex < MMAX_NODEOSTYPE);oindex++)
        {
        if (OSList[oindex].AIndex == N->NextOS)
          {
          if (OSList[oindex].CfgTime > 0)
            {
            ProvDuration = OSList[oindex].CfgTime;
            }

          break;
          }
        }    /* END for (oindex) */

      MULToTString((long)MSched.Time - N->LastOSModRequestTime,TString);

      strcpy(PStartTime,TString);

      if (N->LastOSModRequestTime + ProvDuration > MSched.Time)
        {
        MULToTString(N->LastOSModRequestTime + ProvDuration - MSched.Time,TString);

        strcpy(PEndTime,TString);
        }
      else
        {
        MULToTString(1,TString);

        strcpy(PEndTime,TString);
        }

      MStringAppendF(Buffer,"NOTE:  node is currently provisioning:  NextOS: %s  Provision StartTime: %s  Estimated Completion: %s\n",
        MAList[meOpsys][N->NextOS],
        PStartTime,
        PEndTime);
      }

    if ((N->OSList != NULL) || (MSched.DefaultN.OSList != NULL))
      {
      int oindex;
      
      const mnodea_t *OSList = ((N->OSList != NULL) ? N->OSList : MSched.DefaultN.OSList);
 
      for (oindex = 0;OSList[oindex].AIndex != 0;oindex++)
        {
        if (OSList[oindex].CfgTime > 0)
          {
          MULToTString(OSList[oindex].CfgTime,TString);

          MStringAppendF(Buffer,"  OS Option: %-10s  SetupTime: %s\n",
            MAList[meOpsys][OSList[oindex].AIndex],
            TString);
          }
        else
          {
          MStringAppendF(Buffer,"  OS Option: %-10s\n",
            MAList[meOpsys][OSList[oindex].AIndex]);
          }
        }
      }    /* END if (OSList != NULL) */

    if ((N->VMOSList != NULL) || (MSched.DefaultN.VMOSList != NULL))
      {
      int oindex;

      mnodea_t *OSList;

      OSList = (N->VMOSList != NULL) ? N->VMOSList : MSched.DefaultN.VMOSList;

      for (oindex = 0;OSList[oindex].AIndex != 0;oindex++)
        {
        if (OSList[oindex].CfgTime > 0)
          {
          MULToTString(OSList[oindex].CfgTime,TString);

          MStringAppendF(Buffer,"  VM OS Option: %-10s  SetupTime: %s\n",
            MAList[meOpsys][OSList[oindex].AIndex],
            TString);
          }
        else
          {
          MStringAppendF(Buffer,"  VM OS Option: %-10s\n",
            MAList[meOpsys][OSList[oindex].AIndex]);
          }
        }
      }    /* END if (OSList != NULL) */

    if (N != &MSched.DefaultN)
      {
      sprintf(tmpLine,"Speed:      %-6.2f    CPULoad:  %6.3f",
        N->Speed,
/*
        (N->STTime > 0) ? N->Load : 0.0);
*/
        N->Load);
      }

    if (N->MaxLoad > 0.01)
      {
      sprintf(tmpLine,"%s (MaxCPULoad: %.2f)",
        tmpLine,
        N->MaxLoad);
      }
    
#if 0
    if (N->ExtLoad > 0.01)
      {
      sprintf(tmpLine,"%s (ExtLoad: %.2f)",
        tmpLine,
        N->ExtLoad);
      }
#endif

    if (N->ProcSpeed > 0)
      {
      sprintf(tmpLine,"%s (ProcSpeed: %d)",
        tmpLine,
        N->ProcSpeed);
      }

    strcat(tmpLine,"\n");

    MStringAppend(Buffer,tmpLine);
    }  /* END if (IsCompute == TRUE) */
  else if (IsStorage == TRUE)
    {
    int index;

    index = MUMAGetIndex(meGRes,"dsop",mVerify);

    if (MSched.DefStorageRM != NULL) 
      {
      char tmpLine[MMAX_LINE];

      if (MSched.DefStorageRM->MaxDSOp > 0)
        {
        sprintf(tmpLine,"(limit: %d)",
          MSched.DefStorageRM->MaxDSOp);
        }
      else
        {
        tmpLine[0] = '\0';
        }

      /* display current number of active staging sessions */

      if ((index > 0) || bmisset(Flags,mcmVerbose))
        {
        MStringAppendF(Buffer,"Active Data Staging Operations:  ");
        
        if (index > 0)
          MStringAppendF(Buffer,"%d ",MSNLGetIndexCount(&N->DRes.GenericRes,index));

        MStringAppendF(Buffer,"%s\n",tmpLine);
        }

      /* NOTE:  if verbose, list each active session */

      if (bmisset(Flags,mcmVerbose))
        {
        job_iter JTI;

        mjob_t   *J;
        msdata_t *D;
        mbool_t   ReportMB;

        int       SFSize;
        int       DFSize;

        MJobIterInit(&JTI);

        /* walk all jobs, locate all active data staging sessions */

        while (MJobTableIterate(&JTI,&J) == SUCCESS)
          {
          if ((J->DataStage == NULL) || (J->DataStage->SIData == NULL))
            continue;

          for (D = J->DataStage->SIData;D != NULL;D = D->Next)
            {
            ReportMB = (J->DataStage->SIData->SrcFileSize >= MDEF_MBYTE);

            if (ReportMB == TRUE)
              {
              SFSize = MAX(1,D->SrcFileSize / MDEF_MBYTE);
              DFSize = D->DstFileSize / MDEF_MBYTE;
              }
            else
              {
              SFSize = MAX(1,D->SrcFileSize);
              DFSize = D->DstFileSize;
              }

            MStringAppendF(Buffer,"  job %16s  %-7s (%d of %d %s transferred)  (%s)\n",
              J->Name, 
              MDataStageState[D->State],
              DFSize,
              SFSize,
              (ReportMB == TRUE) ? "MB" : "bytes",
              D->SrcFileName);
            }    /* END for (D) */
          }      /* END for (J) */

        MStringAppendF(Buffer,"\n");
        }
      }    /* END if ((MSched.DefStorageRM != NULL) && (index > 0)) */
    else
      {
      if (index <= 0)
        {
        MStringAppendF(Buffer,"WARNING:  storage resource manager misconfigured - datastage op not specified\n");
        }
      else
        {
        MStringAppendF(Buffer,"WARNING:  storage resource manager misconfigured\n");
        }
      }

    /* display current diskspace utilized/dedicated */

    if (MSched.DefStorageRM->TargetUsage > 0.0)
      {
      MStringAppendF(Buffer,"Dedicated Storage Manager Disk Usage:  %d of %d MB (Target=%d MB)\n",
        N->DRes.Disk,
        N->CRes.Disk,
        (int)(N->CRes.Disk * MSched.DefStorageRM->TargetUsage / 100.0));
      }
    else
      {
      MStringAppendF(Buffer,"Dedicated Storage Manager Disk Usage:  %d of %d MB\n",
        N->DRes.Disk,
        N->CRes.Disk);
      }

    /* display current network utilized/dedicated */

    /* display estimated/average throughput */
    }  /* END else if (IsStorage == TRUE) */

  if (bmisset(Flags,mcmVerbose))
    {
    if (N != &MSched.DefaultN)
      {
      if (N->P != NULL)
        {
        MStringAppendF(Buffer,"Priority:   Static: %.2f  Dynamic:  %.2f\n",
          N->P->SP,
          N->P->DP);
        }
      }

    if (N->ReleasePriority != 0)
      {
      MStringAppendF(Buffer,"ReleasePriority:  %d\n",
        N->ReleasePriority);
      }
    }

  if (bmisset(Flags,mcmVerbose))
    {
    if (N->RackIndex >= 0)
      {
      MStringAppendF(Buffer,"Partition:  %s  Rack/Slot:  %d/%d",
        (N->PtIndex >= 0) ? MPar[N->PtIndex].Name : "---",
        N->RackIndex,
        N->SlotIndex);
      }
    else
      {
      MStringAppendF(Buffer,"Partition:  %s  Rack/Slot:  %s",
        (N->PtIndex >= 0) ? MPar[N->PtIndex].Name : "---",
        "---");
      }

    if (N->NodeIndex > 0)
      {
      MStringAppendF(Buffer,"  NodeIndex:  %d",
        N->NodeIndex);
      }

    MStringAppend(Buffer,"\n");

    if ((MSched.JobCCodeFailSampleSize > 0) &&
        (N->JobCCodeFailureCount != NULL))
      {
      double CCodeRate = 0;
      double CCodeSampleSize = 0;
      int CCodeIndex;

      for (CCodeIndex = 0;CCodeIndex < MMAX_CCODEARRAY;CCodeIndex++)
        {
        if (N->JobCCodeSampleCount[CCodeIndex] <= 0)
          break;

        CCodeRate += (double)N->JobCCodeFailureCount[CCodeIndex];
        CCodeSampleSize += (double)N->JobCCodeSampleCount[CCodeIndex];
        }

      if (CCodeSampleSize != 0)
        CCodeRate = CCodeRate / CCodeSampleSize;
      else
        CCodeRate = 0;

      MStringAppendF(Buffer,"Job Failure Rate: %.f%% (%.f samples)\n",
        CCodeRate * 100,
        CCodeSampleSize);
      }
    }  /* END if (bmisset(Flags,mcmVerbose)) */

  if (!bmisclear(&N->Flags))
    {
    mstring_t Flags(MMAX_LINE);

    bmtostring(&N->Flags,MNodeFlags,&Flags);

    MStringAppendF(Buffer,"Flags:      %s\n",
      Flags.c_str());
    }

  if (!bmisclear(&N->FBM))
    {
    char tmpLine[MMAX_LINE];

    MStringAppendF(Buffer,"Features:   %s\n",
      MUNodeFeaturesToString(',',&N->FBM,tmpLine));
    }

  if (N->NodeType != NULL)
    {
    MStringAppendF(Buffer,"NodeType:   %s\n",
      N->NodeType);
    }
  else if ((N->RackIndex > 0) &&
           (MRack[N->RackIndex].NodeType[0] != '\0'))
    {
    MStringAppendF(Buffer,"NodeType:   %s\n",
      MRack[N->RackIndex].NodeType);
    }

  if (N->KbdIdleTime > 0)
    {
    if (bmisset(Flags,mcmVerbose))
      {
      MULToTString(N->KbdIdleTime,TString);

      MStringAppendF(Buffer,"IdleTime:   %s\n",
        TString);
      }
    }

  if (bmisset(&N->IFlags,mnifKbdDrainActive))
    {
    MStringAppendF(Buffer,"NOTE:  node is in DRAIN state to to local desktop access\n");
    }

  if (N->KbdDetectPolicy != mnkdpNONE)
    {
    MStringAppendF(Buffer,"KeyboardDetectPolicy: %s\n",
      MNodeKbdDetectPolicy[N->KbdDetectPolicy]);
    }

  if (N->MinResumeKbdIdleTime > 0)
    {
    MULToTString(N->MinResumeKbdIdleTime,TString);

    MStringAppendF(Buffer,"ResumeKbdTime:        %s\n",
      TString);
    }

  if (N->KbdSuspendedJobList != NULL)
    {
    MStringAppendF(Buffer,"NOTE:  jobs suspended due to keyboard activity: %s\n",
      N->KbdSuspendedJobList);
    }

  /* Need a tmp string */
  mstring_t Classes(MMAX_LINE);

  MClassListToMString(&N->Classes,&Classes,NULL);

  if (!Classes.empty())
    {
    MStringAppendF(Buffer,"Classes:    %s\n",Classes.c_str());
    }

  if (N->RMList != NULL)
    {
    int  rmindex;

    char tmpLine[MMAX_LINE];
    char tmpLine2[MMAX_LINE];

    for (rmindex = 0;N->RMList[rmindex] != NULL;rmindex++)
      {
      mrm_t *tR;

      tR = N->RMList[rmindex];

      sprintf(tmpLine,"RM[%s]%s:               ",
        tR->Name,
        (tR == N->RM) ? "*" : "");

      sprintf(tmpLine2,"%-11.11s",
        tmpLine);

      MStringAppendF(Buffer,"%s TYPE=%s",
        tmpLine2,
        MRMType[tR->Type]);

      if (tR->SubType != mrmstNONE)
        {
        MStringAppendF(Buffer,":%s",
          MRMSubType[tR->SubType]);
        }

      if ((N->RMState[rmindex] != mnsNONE) && 
          (N->RMState[rmindex] != mnsUnknown))
        {
        MStringAppendF(Buffer,"  STATE=%s",
          MNodeState[N->RMState[rmindex]]);
        }

      if (!bmisclear(&N->RMAOBM))
        {
        mstring_t RMOBM(MMAX_LINE);

        bmtostring(&N->RMAOBM,MNodeAttr,&RMOBM);

        MStringAppendF(Buffer,"  ATTRO=%s",
          RMOBM.c_str());
        }

      MStringAppend(Buffer,"\n");
      }  /* END for (rmindex) */
    }    /* END if (N->RMList != NULL) */

  /* display node policies */

  if ((N->AP.HLimit[mptMaxJob] > 0) || 
      (N->AP.HLimit[mptMaxWC] > 0) ||
      (N->AP.HLimit[mptMaxPE] > 0) ||
      (N->AP.HLimit[mptMaxProc] > 0) ||
      ((N->NP != NULL) && 
       ((N->NP->MaxJobPerGroup > 0) ||
        (N->NP->MaxProcPerGroup > 0) ||
        (N->NP->MaxJobPerUser > 0) ||
        (N->NP->MaxProcPerUser > 0) ||
        (N->NP->MaxProcPerClass > 0) ||
        (N->NP->PreemptPolicy != mppNONE) ||
        (N->NP->MaxPEPerJob > 0.01))))
    {
    MStringAppend(Buffer,"Policies:   ");

    if (N->AP.HLimit[mptMaxJob] > 0)
      {
      MStringAppendF(Buffer,"MAXJOB=%d  ",
        N->AP.HLimit[mptMaxJob]);
      }

    if (N->AP.HLimit[mptMaxProc] > 0)
      {
      MStringAppendF(Buffer,"MAXPROC=%d  ",
        N->AP.HLimit[mptMaxProc]);
      }

    if (N->AP.HLimit[mptMaxWC] > 0)
      {
      MULToTString(N->AP.HLimit[mptMaxWC],TString);

      MStringAppendF(Buffer,"MAXWCLIMITPERJOB=%s",
        TString);
      }

    if (N->AP.HLimit[mptMaxPE] > 0)
      {
      MStringAppendF(Buffer,"%s=%d  ",
        MNodeAttr[mnaMaxPE],
        N->AP.HLimit[mptMaxPE]);
      }

    if (N->NP != NULL)
      {
      if (N->NP->MaxPEPerJob > 1.0)
        {
        MStringAppendF(Buffer,"%s=%d  ",
          MNodeAttr[mnaMaxPEPerJob],
          (int)N->NP->MaxPEPerJob);
        }
      else if (N->NP->MaxPEPerJob > 0.01)
        {
        MStringAppendF(Buffer,"%s=%d%%  ",
          MNodeAttr[mnaMaxPEPerJob],
          (int)(N->NP->MaxPEPerJob * 100.0));
        }

      /* group policies */

      if (N->NP->MaxJobPerGroup > 0)
        {
        MStringAppendF(Buffer,"%s=%d  ",
          MNodeAttr[mnaMaxJobPerGroup],
          N->NP->MaxJobPerGroup);
        }

      if (N->NP->MaxProcPerGroup > 0)
        {
        MStringAppendF(Buffer,"%s=%d  ",
          MNodeAttr[mnaMaxProcPerGroup],
          N->NP->MaxProcPerGroup);
        }

      /* user policies */

      if (N->NP->MaxJobPerUser > 0)
        {
        MStringAppendF(Buffer,"%s=%d  ",
          MNodeAttr[mnaMaxJobPerUser],
          N->NP->MaxJobPerUser);
        }

      if (N->NP->MaxProcPerUser > 0)
        {
        MStringAppendF(Buffer,"%s=%d  ",
          MNodeAttr[mnaMaxProcPerUser],
          N->NP->MaxProcPerUser);
        }

      /* class policies */
      if (N->NP->MaxProcPerClass > 0)
        {
        MStringAppendF(Buffer,"%s=%d  ",
          MNodeAttr[mnaMaxProcPerClass],
          N->NP->MaxProcPerClass);
        }

      /* preempt policy */
      if (N->NP->PreemptPolicy != mppNONE)
        {
        MStringAppendF(Buffer,"%s=%s  ",
          MNodeAttr[mnaPreemptPolicy],
          MPreemptPolicy[N->NP->PreemptPolicy]);
        }
      }    /* END if (N->NP != NULL) */

    MStringAppend(Buffer,"\n");
    }  /* END if ((N->AP.HLimit[mptMaxJob] > 0) || ...) */

  if (N->SpecNAPolicy != mnacNONE)
    {
    MStringAppendF(Buffer,"NodeAccessPolicy: %s\n",
      MNAccessPolicy[N->SpecNAPolicy]);
    }

  if (N->EffNAPolicy != mnacNONE)
    {
    MStringAppendF(Buffer,"EffNodeAccessPolicy: %s\n",
      MNAccessPolicy[N->EffNAPolicy]);
    }

  if (N->ProfilingEnabled == TRUE)
    {
    MStringAppendF(Buffer,"EnableProfiling:  %s\n",
      MBool[N->ProfilingEnabled]);
    }

  if (N->MinPreemptLoad > 0)
    {
    MStringAppendF(Buffer,"MINPREEMPTLOAD:  %.2f\n",
      N->MinPreemptLoad);
    }

  if (N->Stat.SuccessiveJobFailures > 0)
    {
    MStringAppendF(Buffer,"Successive Job Failures: %d\n",
      N->Stat.SuccessiveJobFailures);
    }

  /* display node statistics */

  if (N->STTime > 0)
    {
    char STotalTime[MMAX_NAME];
    char SUpTime[MMAX_NAME];
    char SBusyTime[MMAX_NAME];

    MULToTString(N->STTime,TString);
    strcpy(STotalTime,TString);
    MULToTString(N->SUTime,TString);
    strcpy(SUpTime,TString);
    MULToTString(N->SATime,TString);
    strcpy(SBusyTime,TString);

    MStringAppendF(Buffer,"\nTotal Time: %s  Up: %s (%.2f%c)  Active: %s (%.2f%c)\n",
      STotalTime,
      SUpTime,
      (double)N->SUTime / N->STTime * 100.0,
      '%',
      SBusyTime,
      (double)N->SATime / N->STTime * 100.0,
      '%');
    }  /* END if (N->STTime > 0) */

  return(SUCCESS);
  }  /* END MNodeShowState() */




/**
 * Report node reservations.
 *
 * @see MNodeShowState() - peer
 *
 * @param N (I)
 * @param Buffer (O)
 */

int MNodeShowReservations(

  mnode_t   *N,
  mstring_t *Buffer)

  {
  RsvTable printedRsvs;

  char  StartTime[MMAX_NAME];
  char  EndTime[MMAX_NAME];
  char  Duration[MMAX_NAME];
  char  RsvType[MMAX_NAME];
  char  TString[MMAX_LINE];

  int   RC;

  mrsv_t  *R;
  mcres_t *BR;

  mre_t *RE;

  const char *FName = "MNodeShowReservations";

  MDB(5,fSTRUCT) MLog("%s(%s,Buffer)\n",
    FName,
    (N != NULL) ? N->Name : "NULL");

  if ((N == NULL) || (Buffer == NULL))
    {
    return(FAILURE);
    }

  /* display node reservation information */

  MStringAppend(Buffer,"\nReservations:\n");

  if (N->RE == NULL)
    {
    MStringAppend(Buffer,"  ---\n");

    return(SUCCESS);
    }

  for (RE = N->RE;RE != NULL;MREGetNext(RE,&RE))
    {
    R = MREGetRsv(RE);

    if (RE->Type != mreStart)
      continue;

    BR = &RE->BRes;
    RC = RE->TC;

    MULToTString(R->StartTime - MSched.Time,TString);
    strcpy(StartTime,TString);
    MULToTString(R->EndTime - MSched.Time,TString);
    strcpy(EndTime,TString);
    MULToTString(R->EndTime - R->StartTime,TString);
    strcpy(Duration,TString);

    if (R->Type == mrtJob)
      {
      snprintf(RsvType,sizeof(RsvType),"%s:%-7s",
        MRsvType[R->Type],
        (R->J != NULL) ? MJobState[(R->J)->State] : "idle");
      }
    else
      {
      strcpy(RsvType,MRsvType[R->Type]);
      }

    if ((R->Type != mrtUser) || (printedRsvs.find(R->Name) == printedRsvs.end()))
      {
      MStringAppendF(Buffer,"  %sx%d  %s  %s -> %s (%s)\n",
        R->Name,
        RC,
        RsvType,
        StartTime,
        EndTime,
        Duration);

      printedRsvs.insert(std::pair<std::string,mrsv_t *>(R->Name,R));
      }

    if (R->Type == mrtUser)
      {
      char TString[MMAX_LINE];
      /* NOTE:  change R->ReqTC -> RC in MCResRatioToString() */

      MULToTString(RE->Time - MSched.Time,TString);

      MStringAppendF(Buffer,"    Blocked Resources@%-11s %s\n",
        TString,
        MCResRatioToString(&R->DRes,BR,&N->CRes,RC));
      }  /* END if (R->Type)  */
    }    /* END for (reindex) */

  return(SUCCESS);
  }  /* END MNodeShowReservations() */




/**
 * Process 'showres -n' request.
 *
 * @see MUIRsvShow() - parent
 * @see MSysToXML() - child
 * @see MNRsvToString() - child
 * @see MCShowReservedNodes() - peer - displays 'showres' report w/in client
 *
 * @param Auth (I)
 * @param RID (I) [optional]
 * @param P (I) [optional]
 * @param DFlags (I) [bitmap of enum mcm*]
 * @param DE (I/O)
 */

int MNodeShowRsv(

  char      *Auth,
  char      *RID,
  mpar_t    *P,
  mbitmap_t *DFlags,
  mxml_t    *DE,
  char      *EMsg)

  {
  int    nindex;

  mre_t   *RsvEvent;

  mrsv_t  *R;
  mnode_t *N;

  mrsv_t  *SR;

  mrsv_t **ARsv = NULL;
  int     *ARsvCount = NULL;

  mbool_t RsvIDIsSpecified;

  mbool_t RsvFound; 

  mbool_t NRsvFound;  /* requested reservation is found on specified node */

  #define MMAX_TSLOT 30

  char DVal[MMAX_TSLOT + 4][MMAX_NAME];

  mgcred_t *U;

  const char *FName = "MNodeShowRsv";

  MDB(3,fSTRUCT) MLog("%s(%s,%s,P,DE)\n",
    FName,
    (Auth != NULL) ? Auth : "NULL",
    (RID != NULL) ? RID : "NULL");

  if ((DE == NULL) || (EMsg == NULL))
    {
    return(FAILURE);
    }

  EMsg[0] = '\0';

  if ((RID != NULL) && strcmp(RID,NONE))
    {
    RsvIDIsSpecified = TRUE;
    }
  else
    {
    RsvIDIsSpecified = FALSE;
    }

  SR = NULL;

  if ((RID == NULL) || !strcmp(RID,NONE))
    {
    RsvFound = TRUE;
    }
  else
    {
    char *RPtr;

    if (RID[0] == '!')
      {
      RPtr = &RID[1];
      }
    else
      {
      RPtr = &RID[0];
      }
  
    if (MRsvFind(RPtr,&SR,mraNONE) == FAILURE)
      {
      snprintf(EMsg,MMAX_LINE,"ERROR:  cannot locate reservation '%s'\n",RID);

      MDB(2,fSTRUCT) MLog("INFO:     cannot locate reservation '%s'\n",
        RID);

      return(FAILURE);
      }

    RsvFound = FALSE;
    }  /* END else ((RID == NULL) || !strcmp(RID,NONE)) */

  if (MUserAdd(Auth,&U) == FAILURE)
    {
    snprintf(EMsg,MMAX_LINE,"INFO:     no reservation info available\n");

    return(FAILURE);
    }
       
  ARsv = (mrsv_t **)MUCalloc(1,sizeof(mrsv_t *) * (MRsvHT.size() + 1));
  ARsvCount = (int *)MUCalloc(1,sizeof(int) * (MRsvHT.size() + 1));

  /* NOTE:  default behavior, display only specified reservations */
  /*        verbose behavior, display all reservations on nodes with specified reservation */
  /*        summary behavior, display summary of all active reservations */
  /*        free behavior, display all nodes with no reservations */

  for (nindex = 0;MNode[nindex] != NULL;nindex++)
    {
    N = MNode[nindex];

    if ((N == NULL) || (N->Name[0] == '\0'))
      break;

    if (N->Name[0] == '\1')
      continue;

    if ((P != NULL) && (P->Index > 0) && (N->PtIndex != P->Index))
      continue;

    /* NOTE:  disable node level check - all checks enforced via rsv check */

    if (bmisset(DFlags,mcmNonBlock))
      {
      mxml_t *tE = NULL;

      if ((N->RE != NULL) && 
          (N->RE->Type == mreStart) && 
          ((mulong)N->RE->Time <= MSched.Time))
        {
        /* node has active rsv, ignore */

        continue;
        }

      MNRsvToString(
        N,
        SR,
        bmisset(DFlags,mcmNonBlock),
        &tE,
        NULL,
        0);

      if (tE != NULL)
        {
        MXMLAddE(DE,tE);
        }

      continue;
      }  /* END if (bmisset(DFlags,mcmNonBlock)) */

    MDB(5,fSTRUCT) MLog("INFO:     checking reservations for node '%s'\n",
      N->Name);

    NRsvFound = MBNOTSET;

    /* create header */

    strcpy(DVal[0],N->Name);

    for (RsvEvent = N->RE;RsvEvent != NULL;MREGetNext(RsvEvent,&RsvEvent))
      {
      if (RsvEvent->Type != mreStart)
        continue;

      R = MREGetRsv(RsvEvent);

      if (SR != NULL)
        {
        if (bmisset(DFlags,mcmOverlap))
          {
          /* NOTE:  support NegateFind when enabled */

          if (MRsvHasOverlap(SR,R,TRUE,&SR->NL,NULL,NULL) == FALSE)
            continue;
          }
        }

      /* continue if specified rsv not matched */

      if ((RsvIDIsSpecified == TRUE) && (NRsvFound == MBNOTSET))
        {
        mre_t *tRE;

        NRsvFound = FALSE;

        for (tRE = RsvEvent;tRE != NULL;MREGetNext(tRE,&tRE))
          {
          R = MREGetRsv(tRE);

          if (!strcmp(R->Name,RID))
            {
            NRsvFound = TRUE;
            RsvFound = TRUE;

            break;
            }
          }  /* END for (rindex2) */

        if (NRsvFound == FALSE)
          {
          if (!bmisset(DFlags,mcmVerbose))
            break;
          }
        }  /* END if ((RsvIDIsSpecified == TRUE) && (NRsvFound == MBNOTSET)) */

      if ((RsvIDIsSpecified == TRUE) && (!bmisset(DFlags,mcmVerbose)))
        {
        if (strcmp(RID,R->Name))
          continue;
        }

      if (MUICheckAuthorization(
            U,
            NULL,
            (void *)R,
            mxoRsv,
            mcsRsvShow,
            mrcmList,
            NULL,
            NULL,
            0) == FAILURE)
        {
        /* user does not have access to view rsv */

        continue;
        }

      MDB(7,fSTRUCT) MLog("INFO:     displaying rsv '%s' on node %s\n",
        R->Name,
        N->Name);

      mxml_t *tE = NULL;

      /* NOTE:  single call shows all reservations for node */

      /* if 'R' is specified, show only R */

      MNRsvToString(
        N,
        (!bmisset(DFlags,mcmVerbose)) ? SR : NULL,
        bmisset(DFlags,mcmNonBlock),
        &tE, 
        NULL,
        0);

      if (tE != NULL)
        {
        MXMLAddE(DE,tE);
        }

      /* single pass displays all reservations in XML mode */

      break;
      }    /* END for (rindex) */
    }      /* END for (nindex) */

  MUFree((char **)&ARsv);
  MUFree((char **)&ARsvCount);

  if (RsvFound == FALSE)
    {
    snprintf(EMsg,MMAX_LINE,"ERROR:  cannot locate reservation");

    MDB(2,fSTRUCT) MLog("INFO:     cannot locate reservation '%s'\n",
      RID);

    return(FAILURE);
    }
  else
    {
    enum MSysAttrEnum SAList[] = {
      msysaPresentTime,
      msysaNONE };

    enum MXMLOTypeEnum SCList[] = {
      mxoNONE };

    mxml_t *tmpE = NULL;

    if (MSysToXML(&MSched,&tmpE,SAList,SCList,TRUE,0) == SUCCESS)
      {
      MXMLAddE(DE,tmpE);
      }
    }

  return(SUCCESS);
  }  /* END MNodeShowRsv() */
/* END MNodeShow.c */
