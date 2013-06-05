
/* HEADER */

      
/**
 * @file: MReq.c 
 *
 * Contains:
 *      int MReqCheckResourceMatch
 *      int MReqGetFNL
 */


#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  

/**
 * Checks the requirements of the job against the resources of the node.
 * 
 * Includes things like OS, node features, configured processors, configured memory, configured classes,
 * the node's Resource Manager (RM), the node's partition, the Moab hostlist, the Moab exclusive-hostlist,
 * network capabilities, available software, and probably a few other things I missed.  
 *
 * @see MReqGetFNL() - parent
 *
 * @param J (I) [optional] The job to check node resources against.
 * @param RQ (I) The requirements to check.
 * @param N (I) The node whose resources are to be checked.
 * @param RIndex (O) [optional] index of rejection reason.
 * @param StartTime (I) The start time of the job? 
 * @param ForceVMEval (I) Force evaluation of VM resources on Node instead of Node resources
 * @param P (I) [optional] Partition 
 * @param Msg (O) [optional,minsize=MMAX_LINE]
 */

int MReqCheckResourceMatch(

  const mjob_t               *J,
  const mreq_t               *RQ,
  const mnode_t              *N,
  enum MAllocRejEnum         *RIndex,
  long                        StartTime,
  mbool_t                     ForceVMEval,
  const mpar_t               *P,
  char                       *Affinity,
  char                       *Msg)

  {
  char tmpLine[MMAX_LINE]; 

  int TC;
  int MinTPN;

  int index;

  enum MAllocRejEnum tRIndex;

  mcres_t tmpCRes;
  const mcres_t *NCRes;

  const char *FName = "MReqCheckResourceMatch";

  MDB(5,fSCHED) MLog("%s(%s,%d,%s,%s,%ld,%s,Msg)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (RQ != NULL) ? RQ->Index : -1,
    (N != NULL) ? N->Name : "NULL",
    (RIndex != NULL) ? "RIndex" : "NULL",
    StartTime - MSched.Time,
    (P != NULL) ? P->Name : "NULL");

  if (RIndex != NULL)
    *RIndex = marNONE;

  if (Msg != NULL)
    Msg[0] = '\0';

  if ((RQ == NULL) || (N == NULL))
    {
    return(FAILURE);
    }

  if ((((J != NULL) && (MSched.VMsAreStatic == TRUE) && MJOBREQUIRESVMS(J)) ||
       (ForceVMEval == TRUE)) && (!bmisset(&MSched.Flags,mschedfOutOfBandVMRsv)))
    {
    /* this is a problem, we need some way of free'ing tmpCRes but this routine is 
       full of "return(FAILURE)" */

    MCResInit(&tmpCRes);

    MNodeGetVMRes(N,J,TRUE,&tmpCRes,NULL,NULL);

    NCRes = &tmpCRes;
    }
  else
    {
    NCRes = &N->CRes;
    }
  /* check nodeset constraints */

  if ((bmisset(&MPar[N->PtIndex].Flags,mpfSharedMem)) &&
      (MPar[0].NodeSetPriorityType == mrspMinLoss))
    {
    msmpnode_t *SN;

    int OldLogLevel = mlog.Threshold;

    if ((SN = MSMPNodeFindByNode(N,FALSE)) != NULL)
      {
      if (MSMPJobIsWithinRLimits(J,SN) == FALSE)
        {
        MDB(5,fSCHED) MLog("INFO:     job requests more resources than allowed (%.2f%%) on smp %s - NODESETMAXUSAGE\n",
          MPar[0].NodeSetMaxUsage * 100,
          SN->Name);

        if (RIndex != NULL)
          *RIndex = marPolicy;

        if (Msg != NULL)
          strcpy(Msg,"request exceeds resources allowed on SMP");
          
        mlog.Threshold =  OldLogLevel;

 
        return(FAILURE);
        }
      }

    mlog.Threshold =  OldLogLevel;
    }     /* END if (MSched.SharedMem == TRUE) */

  if (J != NULL)
    {
    if ((J->ReqRID != NULL) &&
        (bmisset(&J->IFlags,mjifNotAdvres)))
      {
      if (MNodeRsvFind(N,J->ReqRID,TRUE,NULL,NULL) == SUCCESS)
        {
        MDB(5,fSCHED) MLog("INFO:     job %s requires all rsv's but this rsv, %s, contained on idle node %s\n",
          J->Name,
          J->ReqRID,
          N->Name);

        if (RIndex != NULL)
          *RIndex = marPolicy;

        if (Msg != NULL)
          strcpy(Msg,"reqrsv");

        return(FAILURE);
        }
      }

    if ((MSched.TwoStageProvisioningEnabled == FALSE) &&
        ((N->RM != NULL)  && (bmisset(&N->RM->RTypes,mrmrtCompute))))
      {
      if ((MUHVALLOWSCOMPUTE(N->HVType) == FALSE) && 
          (J->VMUsagePolicy == mvmupPMOnly) &&
          (J->System == NULL))
        {
        MDB(5,fSCHED) MLog("INFO:     job %s cannot run on hypervisor %s (VMUsagePolicy=%s)\n",
          J->Name,
          N->Name,
          MVMUsagePolicy[J->VMUsagePolicy]);

        if (RIndex != NULL)
          *RIndex = marPolicy;

        if (Msg != NULL)
          strcpy(Msg,"job cannot run on hypervisor");

        return(FAILURE);
        }

      if ((N->HVType == mhvtNONE) && 
          ((J->VMUsagePolicy == mvmupVMCreate) || (J->VMUsagePolicy == mvmupVMOnly)))
        {
        MDB(5,fSCHED) MLog("INFO:     job %s cannot run on compute node %s (VMUsagePolicy=%s)\n",
          J->Name,
          N->Name,
          MVMUsagePolicy[J->VMUsagePolicy]);

        if (RIndex != NULL)
          *RIndex = marPolicy;

        if (Msg != NULL)
          strcpy(Msg,"job must run on hypervisor");

        return(FAILURE);
        }
      } /* END if ((MSched.TwoStageProvisioningEnabled == FALSE) && ... */

    if (bmisset(&J->IFlags,mjifNoGRes) && (!MSNLIsEmpty(&NCRes->GenericRes)))
      {
      if (RIndex != NULL)
        *RIndex = marGRes;
   
      return(FAILURE);
      }
   
    if ((RQ->MinProcSpeed > 0) && (RQ->MinProcSpeed > N->ProcSpeed))
      {
      if (RIndex != NULL)
        *RIndex = marCPU;
   
      return(FAILURE);
      }
   
    if ((N->AP.HLimit[mptMaxWC] > 0) && (J->SpecWCLimit[0] > (mutime)N->AP.HLimit[mptMaxWC]))
      {
      if (RIndex != NULL)
        *RIndex = marPolicy;
  
      if (Msg != NULL)
        strcpy(Msg,"job exceeds max wclimit allowed on node");
 
      return(FAILURE);
      }

    /* reject if node rm does not match job rm */

    if ((bmisset(&MSched.Flags,mschedfAllowMultiCompute)) &&
        ((J->Credential.Q != NULL) && bmisset(&J->Credential.Q->Flags,mqfProvision)))
      {
      /* in multi-compute environment we assume that we don't need to check rm */

      /* NO-OP */
      }
    else if ((J->SubmitRM != NULL) &&
        (!bmisset(&J->SubmitRM->IFlags,mrmifLocalQueue)) &&
         (!bmisset(&J->Flags,mjfCoAlloc)) &&
        ((N->RM != NULL) && 
         (N->RM->IsVirtual != TRUE) && 
         (J->SubmitRM->Index != N->RM->Index) && 
         (N->PtIndex != MSched.SharedPtIndex)))
      {
      /* NOTE:  RM constraints should support MasterRM */

      MDB(5,fSCHED) MLog("INFO:     node rm does not match - job src rm=%s  node rm=%s\n",
        (J->SubmitRM != NULL) ? J->SubmitRM->Name : "NULL",
        (N->RM != NULL) ? N->RM->Name : "NULL");

      if (RIndex != NULL)
        *RIndex = marRM;

      if (Msg != NULL)
        strcpy(Msg,"node rm does not match job src rm");

      return(FAILURE);
      }

    if ((J->TemplateExtensions != NULL) && (J->TemplateExtensions->WorkloadRM != NULL) &&
        (MJOBREQUIRESVMS(J) == FALSE) && (MJOBISVMCREATEJOB(J) == FALSE) &&
        ((J->Credential.Q == NULL) || (!bmisset(&J->Credential.Q->Flags,mqfProvision))))
      {
      /* if a job can provision then we can assume that even if the node doesn't belong to
         the right RM now it will in the future if we provision it, this is true as long
         as we check OS constraints (which we do) and one workloadRM will eventually
         manage all jobs (this might not work in windows environments) */

      /* workload RM specified */

      int rmindex;
      mbool_t FoundRM = FALSE;

      if ((RQ != NULL) && (RQ->DRes.Procs <= 0))
        {
        FoundRM = TRUE;
        }
      else if (N->RMList != NULL)
        {
        for (rmindex = 0;N->RMList[rmindex] != NULL;rmindex++)
          {
          if (N->RMList[rmindex] == J->TemplateExtensions->WorkloadRM)
            {
            FoundRM = TRUE;
            break;
            }
          }  /* END for (rmindex) */
        }

      if (FoundRM == FALSE)
        {
        MDB(5,fSCHED) MLog("INFO:     node does not support workload RM %s\n",
          J->TemplateExtensions->WorkloadRM->Name);

        if (RIndex != NULL)
          *RIndex = marRM;

        if (Msg != NULL)
          strcpy(Msg,"node does not support workload RM");

        return(FAILURE);
        }
      }    /* END if ((J->TX != NULL) && (J->TX->WorkloadRM != NULL)) */

    MDB(7,fSCHED) MLog("INFO:     job src rm=%s  rm index %d node rm=%s node rm index %d  node pt index %d pt name %s shared pt index %d\n",
      (J->SubmitRM != NULL) ? J->SubmitRM->Name : "NULL",
      (J->SubmitRM != NULL) ? J->SubmitRM->Index : 0,
      (N->RM != NULL) ? N->RM->Name : "NULL",
      (N->RM != NULL) ? N->RM->Index : 0,
      N->PtIndex,
      MPar[N->PtIndex].Name,
      MSched.SharedPtIndex);

    /* node rejected if in excluded hostlist */

    if (!MNLIsEmpty(&J->ExcHList))
      {
      mnode_t *tmpN;

      for (index = 0;MNLGetNodeAtIndex(&J->ExcHList,index,&tmpN) == SUCCESS;index++)
        {
        if (N == tmpN)
          {
          MDB(5,fSCHED) MLog("INFO:     node %s rejected - in excluded hostlist\n",
            N->Name);
 
          if (RIndex != NULL)
            *RIndex = marHostList;

          return(FAILURE);
          }
        }    /* END for (index) */
      }      /* END if (J->ExcHList != NULL) */

    /* node is acceptable if it appears in hostlist */
 
    /* NOTE:  must allow super/sub host mask,                                 */
    /*        ie, node missing from hostlist does NOT imply node not feasible */

    /* NOTE:  removed proc check for hostlist (DBJ - Jan5,2006) */

    if (bmisset(&J->IFlags,mjifHostList) && 
       (!MNLIsEmpty(&J->ReqHList)) && 
       (J->HLIndex == RQ->Index))
      {
      mnode_t *tmpN;

      for (index = 0;MNLGetNodeAtIndex(&J->ReqHList,index,&tmpN) == SUCCESS;index++)
        {
        if (N == tmpN)
          {
          if (J->ReqHLMode == mhlmSuperset)
            {
            /* must perform additional checks */

            MDB(5,fSCHED) MLog("INFO:     node %s in superset hostlist\n",
              N->Name);

            break;
            }

          MDB(5,fSCHED) MLog("INFO:     node %s accepted - in requested hostlist\n",
            N->Name);

          /* BIG CHANGE: 7/16/09 DRW
               don't return SUCCESS, if job requested node we must still check whether the node
               is feasible.
          return(SUCCESS);
          */

          break;
          }
        }    /* END for (index) */

      if ((MNLGetNodeAtIndex(&J->ReqHList,index,&tmpN) == FAILURE) && 
          (J->ReqHLMode != mhlmSubset))
        {
        MDB(5,fSCHED) MLog("INFO:     node %s rejected - not in specified hostlist\n",
          N->Name);
 
        if (RIndex != NULL)
          *RIndex = marHostList;
 
        return(FAILURE);
        }
      }    /* END if ((bmisset(&J->IFlags,mjifHostList)) && ... ) */
    }      /* END if (J != NULL) */

  /* check OS, node features, configured procs, configured memory, configured classes ... */

  if (RQ->DRes.Procs != 0)
    { 
    int ReqOS = RQ->Opsys;

    mbool_t ReqSatisfied;

    /* check opsys */

    if (ReqOS)
      {
      ReqSatisfied = TRUE;
 
      if ((J != NULL) && (bmisset(&J->IFlags,mjifCheckCurrentOS)))
        {
        if (N->ActiveOS == ReqOS)
          ReqSatisfied = TRUE;
        else
          ReqSatisfied = FALSE;
        }
      else if (N->ActiveOS == ReqOS)
        {
        ReqSatisfied = TRUE;
        }
      else if ((J != NULL) && (MJOBREQUIRESVMS(J) || MJOBISVMCREATEJOB(J)))
        {
        mbool_t OSProvisionAllowed = ((J->Credential.Q != NULL) &&
          bmisset(&J->Credential.Q->Flags,mqfProvision));

        /* NOTE: not yet supporting OS type "any" on VMs */
        ReqSatisfied = MNodeIsValidVMOS(N,OSProvisionAllowed,ReqOS,NULL);
        }
      else if (((StartTime > (long)MSched.Time) ||
               ((J != NULL) && 
               ((J->Credential.Q != NULL) && bmisset(&J->Credential.Q->Flags,mqfProvision)) &&
               ((N->OSList != NULL) || (MSched.DefaultN.OSList != NULL)))))
        {
        /* request is in the future or OSList is specified and job can provision */

        mbool_t OSAvailable = TRUE;

        if (strcmp(MAList[meOpsys][N->ActiveOS],ANY) != 0)
          {
          mrsv_t *tRsv;
          mre_t  *RE;

          /* only allow one active os at any given time */

          long Overlap;

          for (RE = N->RE;RE != NULL;MREGetNext(RE,&RE))
            {
            tRsv = MREGetRsv(RE);

            Overlap = MIN((long)tRsv->EndTime,StartTime + 1) -
                      MAX((long)tRsv->StartTime,StartTime);
 
            if (Overlap > 0)
              {
              OSAvailable = FALSE;

              break; 
              } 
            }    /* END for (rindex) */

          if ((OSAvailable != TRUE) || (MNodeIsValidOS(N,ReqOS,NULL) == FALSE))
            {
            ReqSatisfied = FALSE;
            }
          }   /* END if (strcmp(MAList[meOpsys][N->ActiveOS],ANY)) */
        else 
          {
          /* so, i'm looking at this in revision 7411:9dc163e73b27 - if [ANY] 
           * is the active OS on the node, it still checks the OS list. does 
           * that make any sense?  - JDD */
          ReqSatisfied = MNodeIsValidOS(N, ReqOS, NULL);
          }
        }    /* END else if ((StartTime > MSched.Time) || ... */
      else
        {
        ReqSatisfied = FALSE;
        }

      if (ReqSatisfied == FALSE)
        {
        MDB(4,fSCHED) MLog("INFO:     opsys does not match ('%s' needed  '%s' available)\n",
          MAList[meOpsys][ReqOS],
          MAList[meOpsys][N->ActiveOS]);

        if (RIndex != NULL)
          *RIndex = marOpsys;

        return(FAILURE);
        }
      }    /* END if (ReqOS != 0) */

    /* check architecture */
 
    if ((RQ->Arch != 0) && (N->Arch != RQ->Arch))
      {
      MDB(5,fSCHED) MLog("INFO:     architectures do not match (%s needed  %s found)\n",
        MAList[meArch][RQ->Arch],
        MAList[meArch][N->Arch]);

      if (RIndex != NULL)        
        *RIndex = marArch;
 
      return(FAILURE);
      }
    }    /* END if (RQ->DRes.Procs != 0) */
 
  /* check features */

  /* verify all requested features are found */

  if (MReqCheckFeatures(&N->FBM,RQ) != SUCCESS)
    {
    char ReqBM[MMAX_LINE];

    MUNodeFeaturesToString(',',&N->FBM,tmpLine);
    MUNodeFeaturesToString(',',&RQ->ReqFBM,ReqBM);
 
    MDB(5,fSCHED) MLog("INFO:     inadequate features (%s needed  %s found)\n",
      ReqBM,
      tmpLine);

    if (RIndex != NULL)     
      *RIndex = marFeatures;
 
    return(FAILURE);
    }  /* END if (MReqCheckFeatures() != SUCCESS) */

  if ((!bmisclear(&RQ->ExclFBM)) && 
      (MAttrSubset(&N->FBM,&RQ->ExclFBM,RQ->ExclFMode) == SUCCESS))
    {
    char Excl[MMAX_LINE];

    MUNodeFeaturesToString(',',&N->FBM,tmpLine);

    MDB(5,fSCHED) MLog("INFO:     excluded features (%s excluded  %s found)\n",
      MUNodeFeaturesToString(',',&RQ->ExclFBM,Excl),
      tmpLine);

    if (RIndex != NULL)
      *RIndex = marFeatures;

    return(FAILURE);
    }  /* END if (MAttrSubset() != SUCCESS) */

  for (index = 1;index < MSched.M[mxoxGRes];index++)
    {
    if (MGRes.Name[index][0] == '\0')
      break;

    if ((index > MSched.M[mxoxGRes]) &&
        ((MSNLGetIndexCount(&RQ->CGRes,index) != 0) ||
         (MSNLGetIndexCount(&RQ->AGRes,index) != 0) ||
         ((RQ->CRes != NULL) && MSNLGetIndexCount(&RQ->CRes->GenericRes,index) != 0)))
      {
      MDB(5,fSCHED) MLog("INFO:     inadequate amount of resource %s(%d needed 0 found)\n",
        MGRes.Name[index],
        MSNLGetIndexCount(&RQ->CGRes,index));

      return(FAILURE);
      }

    if ((index < MSched.M[mxoxGRes]) &&
        (MSNLGetIndexCount(&RQ->CGRes,index) != 0) && 
        (MSNLGetIndexCount(&RQ->CGRes,index) > MSNLGetIndexCount(&NCRes->GenericRes,index)))
      {
      MDB(5,fSCHED) MLog("INFO:     inadequate amount of configured resource %s(%d needed  %d found)\n",
        MGRes.Name[index],
        MSNLGetIndexCount(&RQ->CGRes,index),
        MSNLGetIndexCount(&NCRes->GenericRes,index));

      return(FAILURE);
      }

    if ((index < MSched.M[mxoxGRes]) &&
        (MSNLGetIndexCount(&RQ->AGRes,index) != 0) && 
        (MSNLGetIndexCount(&RQ->AGRes,index) > MSNLGetIndexCount(&N->ARes.GenericRes,index)))
      {
      MDB(5,fSCHED) MLog("INFO:     inadequate amount of available resource %s(%d needed  %d found)\n",
        MGRes.Name[index],
        MSNLGetIndexCount(&RQ->AGRes,index),
        MSNLGetIndexCount(&N->ARes.GenericRes,index));

      return(FAILURE);
      }

    if ((RQ->CRes != NULL) &&
        (index < MSched.M[mxoxGRes]) &&
        (MSNLGetIndexCount(&RQ->CRes->GenericRes,index) != 0) && 
        (MSNLGetIndexCount(&RQ->CRes->GenericRes,index) > MSNLGetIndexCount(&NCRes->GenericRes,index)))
      {
      MDB(5,fSCHED) MLog("INFO:     inadequate amount of configured resource %s(%d needed  %d found)\n",
        MGRes.Name[index],
        MSNLGetIndexCount(&RQ->CRes->GenericRes,index),
        MSNLGetIndexCount(&NCRes->GenericRes,index));

      return(FAILURE); 
      }
    }    /* END for (index) */

  if (RQ->ReqAttr.size() != 0)
    {
    char EMsg[MMAX_LINE];

    if (MNodeCheckReqAttr(N,RQ,Affinity,EMsg) == FAILURE)
      {
      MDB(5,fSCHED) MLog("INFO:     invalid requested attributes on node - %s\n",
        EMsg);

      if (RIndex != NULL)
        *RIndex = marFeatures;

      return(FAILURE);
      }
    }      /* END if (RQ->ReqAttr != NULL) */

#if 0
  /* check optional requirements */

  if ((J != NULL) && (J->VMUsagePolicy == mvmupVMCreate) && !bmisset(&N->Flags,mnfVMCreateEnabled))
    {
    MDB(5,fSCHED) MLog("INFO:     node does not support dynamic vm creation\n");

    if (RIndex != NULL)
      *RIndex = marVM;

    return(FAILURE);
    }
#endif

  if (RQ->OptReq != NULL)
    {
#ifdef __MNOT
   
    /* NOTE: commented out for simplicity */
    /* when this section is used nodes must be configured with both RCSOFTWARE and GRES */
    /* when this section is not used only GRES is needed on the nodes */

    if (RQ->OptReq->Appl != NULL)
      {
      if (StartTime > (long)MSched.Time)
        {
        /* check configured software list */
  
        if (MULLCheck(N->CS,RQ->OptReq->Appl,NULL) == FAILURE)
          { 
          MDB(5,fSCHED) MLog("INFO:     inadequate configured software (%s not found)\n",
            RQ->OptReq->Appl);

          if (RIndex != NULL)
            *RIndex = marFeatures;

          return(FAILURE);
          }
        }
      else
        {
        /* check available software list */

        if (MULLCheck(N->AS,RQ->OptReq->Appl,NULL) == FAILURE)
          {
          MDB(5,fSCHED) MLog("INFO:     inadequate available software (%s not found)\n",
            RQ->OptReq->Appl);

          if (RIndex != NULL)
            *RIndex = marFeatures;

          return(FAILURE);
          }
        }
      }    /* END if (RQ->OptReq->Appl != NULL) */
#endif /* __MNOT */

    if (RQ->OptReq->GMReq != NULL)
      {
      if (MNodeCheckGMReq(N,RQ->OptReq->GMReq) == FAILURE)
        {
        MDB(5,fSCHED) MLog("INFO:     node generic metrics do not meet requirements\n");

        if (RIndex != NULL)
          *RIndex = marFeatures;

        return(FAILURE);
        }
      }    /* END if (RQ->OptReq->ReqGMetric != NULL) */
    }      /* END if (RQ->OptReq != NULL) */

  /* check configured procs */
 
  if (!MUCompare(NCRes->Procs,RQ->ReqNRC[mrProc],RQ->ReqNR[mrProc]))
    {
    MDB(5,fSCHED) MLog("INFO:     inadequate procs (%s %d needed  %d found)\n",
      MComp[(int)RQ->ReqNRC[mrProc]],
      RQ->ReqNR[mrProc],
      NCRes->Procs);

    if (RIndex != NULL)      
      *RIndex = marCPU;
 
    return(FAILURE);
    }
 
  /* check configured memory */
 
  if (!MUCompare(NCRes->Mem,RQ->ReqNRC[mrMem],RQ->ReqNR[mrMem]))
    {
    MDB(5,fSCHED) MLog("INFO:     inadequate total memory (%s %d needed  %d found)\n",
      MComp[(int)RQ->ReqNRC[mrMem]],
      RQ->ReqNR[mrMem],
      NCRes->Mem);

    if (RIndex != NULL)      
      *RIndex = marMemory;
 
    return(FAILURE);
    }

  /* check configured swap */

  if (!MUCompare(NCRes->Swap,RQ->ReqNRC[mrSwap],RQ->ReqNR[mrSwap]))
    {
    MDB(5,fSCHED) MLog("INFO:     inadequate total swap (%s %d needed  %d found)\n",
      MComp[(int)RQ->ReqNRC[mrSwap]],
      RQ->ReqNR[mrSwap],
      NCRes->Mem);

    if (RIndex != NULL)
      *RIndex = marSwap;

    return(FAILURE);
    }

  /* only check available resources if we're not overcommitting */

  if (MSched.ResOvercommitSpecified[0] == FALSE)
    {
    /* check available memory - unconsumed */
   
    if ((StartTime != MMAX_TIME) && (!MUCompare(MIN(
           N->ARes.Mem,NCRes->Mem - N->DRes.Mem),
           RQ->ReqNRC[mrAMem],
           RQ->ReqNR[mrAMem])))
      {
      MDB(5,fSCHED) MLog("INFO:     inadequate available memory (%s %d needed  %d found)\n",
        MComp[(int)RQ->ReqNRC[mrAMem]],
        RQ->ReqNR[mrAMem],
        MIN(N->ARes.Mem, NCRes->Mem - N->DRes.Mem));
   
      if (RIndex != NULL)
        *RIndex = marMemory;
   
      return(FAILURE);
      }

    /* check available swap - unconsumed */
   
    if ((StartTime != MMAX_TIME) && (!MUCompare(
           MIN(N->ARes.Swap,NCRes->Swap - N->DRes.Swap),
           RQ->ReqNRC[mrASwap],
           RQ->ReqNR[mrASwap])))
      {
      MDB(5,fSCHED) MLog("INFO:     inadequate available swap (%s %d needed  %d found)\n",
        MComp[(int)RQ->ReqNRC[mrASwap]],
        RQ->ReqNR[mrASwap],
        MIN(N->ARes.Swap, NCRes->Swap - N->DRes.Swap));
   
      if (RIndex != NULL)
        *RIndex = marSwap;
   
      return(FAILURE);
      }
   
    /* check dedicated / consumed memory */
   
    if ((bmisset(&MPar[N->PtIndex].Flags,mpfSharedMem)) &&
        (StartTime != MMAX_TIME))
      {
      int tmpDMem = 0;
      enum MNodeAccessPolicyEnum NAPolicy;
  
      MJobGetNAPolicy(J,NULL,NULL,NULL,&NAPolicy);
  
      if (NAPolicy == mnacShared)
        tmpDMem = RQ->ReqMem;
  
      if (!MUCompare(MIN(N->ARes.Mem,N->CRes.Mem - N->DRes.Mem), mcmpGE,tmpDMem))  /* only mcmpGE supported */
        {
        MDB(5,fSCHED) MLog("INFO:     inadequate available memory (%s %d requested  %d found)\n",
          MComp[mcmpGE],
          tmpDMem,
          MIN(N->ARes.Mem, N->CRes.Mem - N->DRes.Mem));
    
        if (RIndex != NULL)
          *RIndex = marMemory;
    
        return(FAILURE);
        }
      } 
    else if ((StartTime != MMAX_TIME) && (!MUCompare(MIN(N->ARes.Mem,NCRes->Mem - N->DRes.Mem), mcmpGE,RQ->DRes.Mem)))  /* only mcmpGE supported */
      {
      MDB(5,fSCHED) MLog("INFO:     inadequate available memory (%s %d requested  %d found)\n",
        MComp[mcmpGE],
        RQ->DRes.Mem,
        MIN(N->ARes.Mem, NCRes->Mem - N->DRes.Mem));
  
      if (RIndex != NULL)
        *RIndex = marMemory;
  
      return(FAILURE);
      }
  
    /* check dedicated / consumed swap */
   
    if ((StartTime != MMAX_TIME) && (!MUCompare(MIN(N->ARes.Swap,NCRes->Swap - N->DRes.Swap), mcmpGE,RQ->DRes.Swap)))     /* only mcmpGE supported */
      {
      MDB(5,fSCHED) MLog("INFO:     inadequate available swap (%s %d requested  %d found)\n",
        MComp[mcmpGE],
        RQ->DRes.Swap,
        MIN(N->ARes.Swap, NCRes->Swap - N->DRes.Swap)); 
   
      if (RIndex != NULL)
        *RIndex = marSwap;
   
      return(FAILURE);
      }
    }
 
  if (NCRes->Disk > 0)
    {
    /* check configured disk */

    if (!MUCompare(NCRes->Disk,RQ->ReqNRC[mrDisk],RQ->ReqNR[mrDisk]))
      {
      MDB(5,fSCHED) MLog("INFO:     inadequate disk (%s %d needed  %d found)\n",
        MComp[(int)RQ->ReqNRC[mrDisk]],
        RQ->ReqNR[mrDisk],
        NCRes->Disk);

      if (RIndex != NULL)
        *RIndex = marDisk;

      return(FAILURE);
      }
    }

  /* GMETRIC_NYI: need to enforce limits on GMetric consumption */

  if ((N != MSched.GN) && 
      (J != NULL) && 
      (J->Credential.C != NULL) && 
      !bmisset(&N->Classes,J->Credential.C->Index))
    {
    MDB(5,fSCHED) MLog("INFO:     node is missing class %s\n",
      J->Credential.C->Name);

    if (RIndex != NULL)   
      *RIndex = marClass;
 
    return(FAILURE);
    }
  else if ((J != NULL) && ((RQ == NULL) || (RQ->DRes.Procs > 0)) && /* gres only reqs shouldn't have class constraints */
           (!bmisset(&J->Flags,mjfRsvMap)) &&
           (J->Credential.C == NULL) && 
           (P != NULL) && (P->RM != NULL) && (P->RM->DefaultC != NULL))
    {
    if (!bmisset(&N->Classes,P->RM->DefaultC->Index))
      {
      MDB(5,fSCHED) MLog("INFO:     node is missing default class '%s'\n",
        P->RM->DefaultC->Name);

      if (RIndex != NULL)   
        *RIndex = marClass;
 
      return(FAILURE);
      }
    }

  /* the below function appears to be a feasibility check only (to determine max num tasks available) */
  /* NOTE: if MReqCheckResourceMatch() is called with StartTime < MMAX_TIME, then behavior may be incorrect (see Dave J & JMB. Jul 2, 2008) */
  /*   The call to MNodeGetTC() to determine availability is done from
       MReqCheckNRes(), which is called by MNodeSelectIdleTasks() */

  TC = MNodeGetTC(N,NCRes,NCRes,NULL,&RQ->DRes,MMAX_TIME,MAX(1,RQ->TasksPerNode),RIndex);

  if ((TC == 0) || (TC < RQ->TasksPerNode))
    {
    MDB(5,fSCHED) MLog("INFO:     insufficient tasks on node %s, node supports %d task%c (TPN=%d)\n",
      N->Name,
      TC,
      (TC == 1) ? ' ' : 's',
      RQ->TasksPerNode);

    return(FAILURE);
    }

  /* check rm constraints */

  if ((N->RM != NULL) && (N->RM->Type == mrmtLL))
    {
    /* handle LL geometry */
 
    MinTPN = (RQ->TasksPerNode > 0) ? RQ->TasksPerNode : 1;
 
    if (RQ->NodeCount > 0)
      {
      MinTPN = RQ->TaskCount / RQ->NodeCount;
 
      if (RQ->TaskCount % RQ->NodeCount)
        MinTPN++;
      }
 
    /* handle pre-LL22 geometries */

    if ((RQ->BlockingFactor != 1) &&
       ((TC < (MinTPN - 1)) ||
        (TC == 0) ||
       ((TC < MinTPN) && (RQ->NodeCount > 0) && (RQ->TaskCount % RQ->NodeCount == 0))))
      {
      MDB(5,fSCHED) MLog("INFO:     node supports %d task%c (%d tasks/node required:LL)\n",
        TC,
        (TC == 1) ? ' ' : 's',
        MinTPN);

      if (RIndex != NULL)
        *RIndex = marCPU;
 
      return(FAILURE);
      }
    }    /* END if ((N->RM != NULL) && (N->RM->Type == mrmtLL)) */

  if ((MSched.CheckSuspendedJobPriority == TRUE) && 
      (MNodeCheckSuspendedJobs(J,N,tmpLine) == FALSE))
    {
    MDB(5,fSCHED) MLog("INFO:     %s\n",
      tmpLine);

    if (RIndex != NULL)
      *RIndex = marNodePolicy;

    return(FAILURE);
    }

  /* check local constraints */

  if (MLocalReqCheckResourceMatch(J,RQ,N,&tRIndex) == FAILURE)
    {
    MDB(5,fSCHED) MLog("INFO:     inadequate %s (%s %d needed  %d found)\n",
      MAllocRejType[tRIndex],
      MComp[(int)RQ->ReqNRC[mrMem]],
      RQ->ReqNR[mrMem],
      NCRes->Mem);

    if (RIndex != NULL)
      *RIndex = tRIndex;

    return(FAILURE);
    }

  MDB(5,fSCHED) MLog("INFO:     node %s can provide resources for job %s:%d\n",
    N->Name,
    (J != NULL) ? J->Name : "NULL",
    RQ->Index);
 
  return(SUCCESS);
  }  /* END MReqCheckResourceMatch() */




/**
 * Determine feasibility/affinity of RQ on N.
 *
 * No affinity is returned if a "must" or "mustnot" is not satisfied, just FAILURE.
 * When there are multiple requested attributes of "should" and "shouldnot" this
 *   routine will add 1 for every satisfied should and shouldnot, and -1 for ever
 *   unsatisfied should and shouldnot.  If the final value > 0 then affinity is
 *   positive, if < 0 it is negative, and = 0 is none.
 *
 * @param N (I)
 * @param RQ (I)
 * @param Affinity (O)
 * @param EMsg
 */

int MNodeCheckReqAttr(

  const mnode_t *N,
  const mreq_t  *RQ,
  char          *Affinity,
  char          *EMsg)

  {
  int AffinityLevel = 0;

  unsigned int index;

  const char *ReqVal;
  const char *NVal;

  int Version;
  int ReqVersion;

  mbool_t MatchFound;

  if (Affinity != NULL)
    *Affinity = mnmNONE;

  for (index = 0;index < RQ->ReqAttr.size();index++)
    {
    MatchFound = FALSE;

    for (attr_iter it = N->AttrList.begin();(N->AttrList.size() != 0) && (it != N->AttrList.end());it++)
      {
      if (MUStrIsEmpty((*it).first.c_str()))
        {
        /* ignore empty variable */

        continue;
        }

      /* NOTE: nptr->BM encoded as MCompEnum */

      if ((*it).first != RQ->ReqAttr[index].Name)
        {
        /* variable name does not match */

        continue;
        }

      if (RQ->ReqAttr[index].Value.empty())
        {
        /* no value specified */

        MatchFound = TRUE;

        break;
        }

      NVal = (*it).second.c_str();
      ReqVal = RQ->ReqAttr[index].Value.c_str();

      Version = (int)strtol(NVal,NULL,10);
      ReqVersion = (int)strtol(ReqVal,NULL,10);    

      switch (RQ->ReqAttr[index].Comparison)
        {
        case mcmpSEQ:

          if ((NVal == NULL) || (NVal[0] == '\0'))
            {
            /* node variable does not match */

            break;
            }

          if (!strcasecmp(ReqVal,NVal))
            {
            MatchFound = TRUE;

            break;
            }

          break;

        case mcmpSNE:

          if ((NVal == NULL) || (NVal[0] == '\0'))
            {
            /* node variable not set */

            break;
            }

          if (strcasecmp(ReqVal,NVal))
            {
            MatchFound = TRUE;

            break;
            }

          break;

        case mcmpLT:

          if (Version < ReqVersion)
            {
            MatchFound = TRUE;
            }

          break;

        case mcmpGT:

          if (Version > ReqVersion)
            {
            MatchFound = TRUE;
            }

          break;

        case mcmpGE:

          if (Version >= ReqVersion)
            {
            MatchFound = TRUE;
            }

          break;

        case mcmpLE:

          if (Version <+ ReqVersion)
            {
            MatchFound = TRUE;
            }

          break;

        case mcmpNE:

          if (Version != ReqVersion)
            {
            MatchFound = TRUE;
            }

          break;

        default:

          MatchFound = TRUE;

          break;
        }  /* END switch (rptr->BM) */

      if (MatchFound == TRUE)
        break;
      }    /* END for (nptr) */

    switch (RQ->ReqAttr[index].Affinity)
      {
      default:
      case maatMust:

        if (MatchFound == FALSE)
          {
          if (EMsg != NULL)
            {
            snprintf(EMsg,MMAX_LINE,"inadequate node variables (%s needed)\n",RQ->ReqAttr[index].Name.c_str());
            }

          MDB(5,fSCHED) MLog("INFO:     inadequate node variables (%s needed)\n",
            RQ->ReqAttr[index].Name.c_str());

          if (Affinity != NULL)
            *Affinity = mnmNONE;

          return(FAILURE);
          }

        break;

      case maatShould:

        if (MatchFound == FALSE)
          {
          if (EMsg != NULL)
            {
            snprintf(EMsg,MMAX_LINE,"INFO:     mismatched node variables (%s wanted)\n",RQ->ReqAttr[index].Name.c_str());
            }

          MDB(5,fSCHED) MLog("INFO:     mismatched node variables (%s wanted)\n",
            RQ->ReqAttr[index].Name.c_str());

          AffinityLevel--;
          }
        else if ((Affinity != NULL) && (*Affinity != mnmNegativeAffinity))
          {
          AffinityLevel++;
          }

        break;

      case maatShouldNot:

        if (MatchFound == TRUE)
          {
          MDB(5,fSCHED) MLog("INFO:     mismatched node variables (%s not wanted)\n",
            RQ->ReqAttr[index].Name.c_str());

          AffinityLevel--;
          }
        else if ((Affinity != NULL) && (*Affinity != mnmNegativeAffinity))
          {
          AffinityLevel++;
          }

        break;

      case maatMustNot:

        if (MatchFound == TRUE)
          {
          if (EMsg != NULL)
            {
            snprintf(EMsg,MMAX_LINE,"INFO:     inadequate node variables (%s not needed)\n",RQ->ReqAttr[index].Name.c_str());
            }

          MDB(5,fSCHED) MLog("INFO:     inadequate node variables (%s not needed)\n",
            RQ->ReqAttr[index].Name.c_str());

          if (Affinity != NULL)
            *Affinity = mnmNONE;

          return(FAILURE);
          }

        break;
      }  /* END switch (RQ->ReqAttr[index].Affinity) */
    }    /* END for (rptr) */

  if (Affinity != NULL)
    {
    if (AffinityLevel > 0)
      *Affinity = mnmPositiveAffinity;
    else if (AffinityLevel < 0)
      *Affinity = mnmNegativeAffinity;
    }

  /* matches found for all required variables */
  return(SUCCESS);
  }  /* END MNodeCheckReqAttr() */




/**
 * Select nodes that are feasible for a specified job req to run on.
 * 
 * This routine should return the list of resources that could ever be suitable to the req through
 * manual or policy-driven provisioning or resource modification (provisioned via power, VM, OS, ...).
 *
 * Checks for several conditions.
 *  - If the job has coallocation set. If it is set, then all nodes visible to the scheduler can be used.
 *  - If the node currently being checked is in the requested partition, and hence can't be used for the job.
 *  - If the node is the "SHARED" node, it can't be used for the job.
 *
 * NOTE:  EMsg[] populated on FAILURE and SUCCESS
 *
 * @see MReqCheckResourceMatch() - child
 * @see MNodeDiagnoseEligibility() - peer - sync - reports node eligibility for given job
 *
 * @param J (I) The job that a feasible node list should be found for.
 * @param RQ (I) The requirements of the job.
 * @param P (I) The partition that the job should run in. Will be used to check if the node currently being examined is in the correct partition.
 * @param SrcNL (I, Optional) A list of nodes to examine for feasibility. If not specified, all nodes will examined.
 * @param DstNL (O) The list of feasible nodes that the job can run on.
 * @param NC (O, Optional) The number of feasible nodes found.
 * @param TC (O, Optional) The number of tasks found.
 * @param StartTime (I) The time the job must start.
 * @param ResMode (I) Currently not used?
 * @param EMsg (O, Optional) Any error message generated will be put into this buffer.
 */

int MReqGetFNL(

  const mjob_t *J,
  const mreq_t *RQ,
  const mpar_t *P,
  const mnl_t  *SrcNL,
  mnl_t        *DstNL,
  int          *NC,
  int          *TC,
  long          StartTime,
  mbitmap_t    *ResMode,
  char         *EMsg)

  {
  mnode_t *N;

  int  rindex;
  int  nindex;

  enum MAllocRejEnum RIndex;

  int  tc;

  int  TasksAllowed;
  int  EffNC;

  mrm_t *RM;
  mrm_t *InternalRM;

  char  *BPtr;
  int    BSpace;

  const char *FName = "MReqGetFNL";

  MDB(4,fSCHED) MLog("%s(%s,%d,%s,%s,DstNL,NC,TC,%ld,%ld,EMsg)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (RQ != NULL) ? RQ->Index : -1,
    (P != NULL) ? P->Name : "NULL",
    (SrcNL != NULL) ? "SrcNL" : "NULL",
    StartTime,
    ResMode);

  if (EMsg != NULL)
    {
    MUSNInit(&BPtr,&BSpace,EMsg,MMAX_LINE);
    }

  if (NC != NULL)
    *NC = 0;

  if (TC != NULL)
    *TC = 0;

  MNLClear(DstNL);

  if ((J == NULL) || (RQ == NULL) || (P == NULL))
    {
    return(FAILURE);
    }

  /* step through all nodes */

  rindex = 0;
  EffNC  = 0;

  tc = 0;

  for (nindex = 0;nindex < MSched.M[mxoNode];nindex++)
    {
    if (SrcNL != NULL)
      {
      if (MNLGetNodeAtIndex(SrcNL,nindex,&N) == FAILURE)
        break;

      if ((bmisset(ResMode,1)) &&
          (N != MSched.GN) && /* jobs can always backfill on GLOBAL node. */
          (MNodeCannotRunJobNow(N,J) == TRUE))
        {
        /* this is a special case for backfill (only MBFFirstFit sets ResMode == 1).
           If a large reservation is in place then backfill can be very slow as it
           literally schedules every job in the system only to find none of them can start.
           this is an optimization that tests whether the job can run "right now" on this
           node.  All it checks is if there is a reservation on the entire node to which
           the job does not have access. If there is the the node is not considered further */

        MDB(7,fSCHED) MLog("INFO:     node %s cannot run job %s right now\n",
          N->Name,
          J->Name);

        continue;
        } 

      /* if the node does not match specified par, and node is not shared, and job
       * is not coalloc'd, reject node from consideration */

      if ((N->PtIndex != P->Index) &&
          (N->PtIndex != MSched.SharedPtIndex) &&
          (P->Index != 0) &&
          (N->PtIndex != 0) &&
         !(bmisset(&J->Flags,mjfCoAlloc)))
        {
        MDB(7,fSCHED) MLog("INFO:     node %s is not in requested partition\n",
          N->Name);

        if (EMsg != NULL)
          {
          MUSNPrintF(&BPtr,&BSpace,"node %s is not in requested partition (%s)\n",
            N->Name,
            P->Name);
          }

        continue;
        }

      /* make sure that the node in consideration belongs to a 
       * partition we have access to */

      if (P->Index == 0)
        {
        if (bmisset(&J->PAL,N->PtIndex) == FALSE)
          {
          /* job cannot access partition */

          continue;
          }
        }

      if (N->ACL != NULL)
        {
        mbool_t IsInclusive=FALSE;

        MACLCheckAccess(N->ACL,J->Credential.CL,NULL,NULL,&IsInclusive);

        if (IsInclusive == FALSE)
          {
          RIndex = marACL;

          MDB(7,fSCHED) MLog("INFO:     node %s has exclusive ACL\n",
            N->Name);

          if (EMsg != NULL)
            {
            MUSNPrintF(&BPtr,&BSpace,"node %s has exclusive ACL\n",
              N->Name);
            }

          continue;
          }
        }  /* END if (N->ACL != NULL) */
      }    /* END if (SrcNL != NULL) */
    else
      {
      N = MNode[nindex];

      if ((N == NULL) || (N->Name[0] == '\0'))
        break;

      if (N->Name[0] == '\1')
        continue;

      /* if the node does not match specified par, and node is not shared, and job
       * is not coalloc'd, reject node from consideration */

      if ((N->PtIndex != P->Index) &&
          (N->PtIndex != MSched.SharedPtIndex) &&
          (P->Index != 0) &&
          (N->PtIndex != 0) &&
         !(bmisset(&J->Flags,mjfCoAlloc)))
        {
        MDB(7,fSCHED) MLog("INFO:     node %s is not in requested partition\n",
          N->Name);

        if (EMsg != NULL)
          {
          MUSNPrintF(&BPtr,&BSpace,"node %s is not in requested partition (%s)\n",
            N->Name,
            P->Name);
          }

        continue;
        }

      /* make sure that the node in consideration belongs to a 
       * partition we have access to */

      if (P->Index == 0)
        {
        if (bmisset(&J->PAL,N->PtIndex) == FALSE)
          {
          /* job cannot access partition */

          continue;
          }
        }

      if (N->ACL != NULL)
        {
        mbool_t IsInclusive;

        MACLCheckAccess(N->ACL,J->Credential.CL,NULL,NULL,&IsInclusive);

        if (IsInclusive == FALSE)
          {
          RIndex = marACL;

          MDB(7,fSCHED) MLog("INFO:     node %s has exclusive ACL\n",
            N->Name);

          if (EMsg != NULL)
            {
            MUSNPrintF(&BPtr,&BSpace,"node %s has exclusive ACL\n",
              N->Name);
            }

          continue;
          }
        }  /* end if (N->ACL != NULL) */

      /* enforces hostlist constraints */

      if (MReqCheckResourceMatch(
           J,
           RQ,
           N,
           &RIndex,    /* O */
           StartTime,
           FALSE,
           P,
           NULL,
           NULL) == FAILURE)
        {
        MDB(7,fSCHED) MLog("INFO:     node %s does not satisfy %s:%d (%s)\n",
          N->Name,
          J->Name,
          RQ->Index,
          MAllocRejType[RIndex]);

        if (EMsg != NULL)
          {
          MUSNPrintF(&BPtr,&BSpace,"node %s does not satisfy %s:%d (%s)\n",
            N->Name,
            J->Name,
            RQ->Index,
            MAllocRejType[RIndex]);
          }

        continue;
        }
      }  /* END else (SrcNL != NULL) */

    if (StartTime != MMAX_TIME)
      {
      if ((N->State != mnsIdle) &&
          (N->State != mnsActive) &&
          (N->State != mnsBusy))
        {
        MDB(7,fSCHED) MLog("INFO:     node %s is in unavailable state %s\n",
          N->Name,
          MNodeState[N->State]);

        continue;
        }
      }

    if (SrcNL != NULL)
      {
      mnode_t *tmpN;

      /* NOTE:  For non-SrcNL nodes, MReqCheckResourceMatch() above enforces hostlist constraints */

      /* check exclude list */

      if (!MNLIsEmpty(&J->ExcHList))
        {
        int hlindex;

        for (hlindex = 0;MNLGetNodeAtIndex(&J->ExcHList,hlindex,&tmpN) == SUCCESS;hlindex++)
          {
          if (tmpN->Index == nindex)
            break;
          }  /* END for (hlindex) */

        if (tmpN != NULL)
          {
          MDB(7,fSCHED) MLog("INFO:     node %s is in excluded hostlist\n",
            N->Name);

          if (EMsg != NULL)
            {
            MUSNPrintF(&BPtr,&BSpace,"node %s is in excluded hostlist\n",
              N->Name);
            }

          continue;
          }
        }  /* END if (J->ExcHList != NULL) */

      if (J->ReqVMList != NULL)
        {
        int vmindex;

        if (N->VMList == NULL)
          {
          MDB(7,fSCHED) MLog("INFO:     node %s does not provide required VM\n",
            N->Name);

          if (EMsg != NULL)
            {
            MUSNPrintF(&BPtr,&BSpace,"node %s does not provide required VM\n",
              N->Name);
            }
          }

        /* report success if ANY required VM is located */

        for (vmindex = 0;J->ReqVMList[vmindex] != NULL;vmindex++)
          {
          if (MULLCheckP(N->VMList,J->ReqVMList[vmindex],NULL) == SUCCESS)
            {
            /* matching VM located */

            break; 
            }
          }  /* END for (vmindex) */

        if (J->ReqVMList[vmindex] == NULL)
          {
          MDB(7,fSCHED) MLog("INFO:     node %s does not provide required VM\n",
            N->Name);

          if (EMsg != NULL)
            {
            MUSNPrintF(&BPtr,&BSpace,"node %s does not provide required VM\n",
              N->Name);
            }
          }
        }

      if ((bmisset(&J->IFlags,mjifHostList)) &&
          (!MNLIsEmpty(&J->ReqHList)) &&
          (N->PtIndex != MSched.SharedPtIndex) &&
          (J->ReqHLMode != mhlmSubset))
        {
        mnode_t *tmpN;

        int hlindex;

        /* check hostlist constraints */

        for (hlindex = 0;MNLGetNodeAtIndex(&J->ReqHList,hlindex,&tmpN) == SUCCESS;hlindex++)
          {
          if (tmpN == N)
            break;
          }  /* END for (hlindex) */

        if (tmpN == NULL)
          {
          MDB(7,fSCHED) MLog("INFO:     node %s is not in required hostlist\n",
            N->Name);

          if (EMsg != NULL)
            {
            MUSNPrintF(&BPtr,&BSpace,"node %s is not in required hostlist\n",
              N->Name);
            }

          continue;
          }
        }
      }    /* END if (SrcNL != NULL) */

    /* check node state */

    if (!bmisset(&J->SpecFlags,mjfIgnState))
      {
      if (MNODEISDRAINING(N) &&
          (MPar[0].NodeDrainStateDelayTime > 0) &&
         ((long)(StartTime - MSched.Time) < MPar[0].NodeDrainStateDelayTime))
        {
        MDB(7,fSCHED) MLog("INFO:     node %s is unavailable (state %s)\n",
          N->Name,
          MNodeState[N->State]);

        if (EMsg != NULL)
          {
          MUSNPrintF(&BPtr,&BSpace,"node %s is unavailable (state %s)\n",
            N->Name,
            MNodeState[N->State]);
          }

        continue;
        }
      else if ((N->State == mnsNone) ||
               (N->State == mnsUnknown) ||
               (N->State == mnsUp) ||
               ((N->State == mnsDown) && 
                ((long)(StartTime - MSched.Time) < MPar[0].NodeDownStateDelayTime)))
        {
        MDB(7,fSCHED) MLog("INFO:     node %s is unavailable (state %s)\n",
          N->Name,
          MNodeState[N->State]);

        if (EMsg != NULL)
          {
          MUSNPrintF(&BPtr,&BSpace,"node %s is unavailable (state %s)\n",
            N->Name,
            MNodeState[N->State]);
          }
 
        continue;
        }
      }    /* END if (!bmisset(&J->SpecFlags,mjfIgnState)) */

    /* check node policies */

    TasksAllowed = (N->CRes.Procs != 0) ? N->CRes.Procs : 9999;

    if (MNodeCheckPolicies(J,N,MMAX_TIME,&TasksAllowed,NULL) == FAILURE)
      {
      MDB(7,fSCHED) MLog("INFO:     node %s is unavailable (policies)\n",
        N->Name);

      if (EMsg != NULL)
        {
        MUSNPrintF(&BPtr,&BSpace,"node %s is unavailable (policies)\n",
          N->Name);
        }

      continue;
      }

    MRMGetInternal(&InternalRM);

    if ((bmisset(&J->Flags,mjfCoAlloc)) && 
        (RQ->RMIndex > 0) && 
        (&MRM[RQ->RMIndex] != InternalRM))
      {
      /* req locked into specific RM */

      RM = &MRM[RQ->RMIndex];

      if ((N->RM != NULL) && (N->RM->Index != RM->Index))
        {
        RIndex = marRM;

        MDB(7,fSCHED) MLog("INFO:     node %s does not meet requirements for job %s:%d (%s)\n",
          N->Name,
          J->Name,
          RQ->Index,
          MAllocRejType[RIndex]);

        if (EMsg != NULL)
          {
          MUSNPrintF(&BPtr,&BSpace,"node %s does not meet requirements (%s)\n",
            N->Name,
            MAllocRejType[RIndex]);
          }

        continue;
        }
      }
 
    if (MReqCheckNRes(
          J,
          N,
          RQ,
          StartTime,
          &TasksAllowed,  /* O */
          1.0,
          0,
          &RIndex,        /* O */
          NULL,
          NULL,
          TRUE,
          FALSE,
          NULL) == FAILURE)
      {
      MDB(7,fSCHED) MLog("INFO:     node %s does not meet requirements for job %s:%d (%s)\n",
        N->Name,
        J->Name,
        RQ->Index,
        MAllocRejType[RIndex]);

      if (EMsg != NULL)
        {
        MUSNPrintF(&BPtr,&BSpace,"node %s does not meet requirements (%s)\n",
          N->Name,
          MAllocRejType[RIndex]);
        }

      continue;
      }

    MDB(6,fSCHED) MLog("INFO:     node %s added to feasible list (%d tasks)\n",
      N->Name,
      TasksAllowed);

    if ((bmisset(&J->IFlags,mjifExactTPN) || bmisset(&J->IFlags,mjifMaxTPN)) && (RQ->TasksPerNode > 0))
      TasksAllowed = MIN(TasksAllowed,RQ->TasksPerNode);

    if ((RQ->NAccessPolicy == mnacSingleTask) || 
        (N->SpecNAPolicy == mnacSingleTask) || 
        (MPar[N->PtIndex].NAccessPolicy == mnacSingleTask))
      TasksAllowed = MIN(TasksAllowed,1);

    MNLSetNodeAtIndex(DstNL,rindex,N);
    MNLSetTCAtIndex(DstNL,rindex,TasksAllowed);

    tc += TasksAllowed;

    EffNC++;

    rindex++;
    }     /* END for (nindex) */

  MNLTerminateAtIndex(DstNL,rindex);

  if (rindex > 0)
    {
    /* determine if NodeSets are optional */

    mbool_t NodeSetIsOptional = TRUE;

    enum MResourceSetSelectionEnum tmpRSS;

    tmpRSS = (RQ->SetSelection != mrssNONE) ?
      RQ->SetSelection :
      MPar[0].NodeSetPolicy;

    if (RQ->SetBlock != MBNOTSET)
      {
      /* job is overriding the system setting */

      NodeSetIsOptional = !RQ->SetBlock;
      }
    else if (MPar[0].NodeSetIsOptional == FALSE)
      {
      /* nothing on the job, use the system setting */

      NodeSetIsOptional = FALSE;
      }

    if ((NodeSetIsOptional == FALSE) &&
        (tmpRSS == mrssNONE))
      {
      /* if the system does not have a nodeset and the job does not have a nodeset
         then there is no nodeset */

      NodeSetIsOptional = TRUE;
      }

    /* don't check NodeSet here if SpanEvenly is specified, nodesets will be enforced in
       MJobGetRangeValue */

    if ((NodeSetIsOptional == FALSE) &&
        (MSched.NodeSetPlus != mnspSpanEvenly))
      {
      mnl_t   *tmpNodeList[MMAX_REQ_PER_JOB];

      /* use tmpNodeList because we are only doing a general 
         "are any nodesets valid" query here and not a query
         for a specific nodeset, we will throw away the results
         and only care about whether MJobSelectResourceSet()
         succeeds or fails */

      /* TODO: MJobSelectResourceSet() should be able to return
         a list of feasible nodes from any nodeset, that way we 
         can eliminate later costly checks on nodes that won't
         fit any requested nodeset */

      MNLMultiInit(tmpNodeList);

      MNLCopy(tmpNodeList[0],DstNL);

      /* best effort node set */
 
      if (MJobSelectResourceSet(
            J,
            RQ,
            StartTime,
            (RQ->SetType != mrstNONE) ? RQ->SetType : MPar[0].NodeSetAttribute,
            (tmpRSS != mrssFirstOf) ? tmpRSS : mrssOneOf,
            ((RQ->SetList != NULL) && (RQ->SetList[0] != NULL)) ? RQ->SetList : MPar[0].NodeSetList,
            tmpNodeList[0],
            NULL,
            -1,
            NULL,
            NULL,
            NULL) == FAILURE)
        {
        tc = 0;

        MNLClear(DstNL);
        }
      else
        {
        /* NOTE:  how to handle if multiple options available? */

        /* FIXME */

        tc = MNLSumTC(DstNL);
        }

      MNLMultiFree(tmpNodeList);
      }    /* END if ((MPar[0].NodeSetIsOptional == TRUE) && ...) */

    if ((bmisset(&J->Flags,mjfCoAlloc)) && (RQ->RMIndex > 0))
      {
      /* req locked into specific RM */

      RM = &MRM[RQ->RMIndex];
      }
    else if (J->SubmitRM != NULL)
      {
      RM = J->SubmitRM;
      }
    else
      {
      RM = &MRM[0];
      }
    }    /* END if (rindex > 0) */

  MDB(5,fSCHED) MLog("INFO:     %d feasible tasks found for job %s:%d in partition %s (%d Needed)\n",
    tc,
    J->Name,
    RQ->Index,
    (P != NULL) ? P->Name : "NULL",
    RQ->TaskCount);

  if (NC != NULL)
    *NC = EffNC;

  if (TC != NULL)
    *TC = tc;

  if (tc <= 0)
    {
    MDB(7,fSCHED) MLog("INFO:     inadequate feasible tasks found for job %s:%d\n",
      J->Name,
      RQ->Index);

    return(FAILURE);
    }

  if (!bmisset(&J->Flags,mjfBestEffort))
    {
    if (tc < RQ->TaskCount)
      {
      MDB(4,fSCHED) MLog("INFO:     inadequate feasible tasks found for job %s:%d in partition %s (%d < %d)\n",
        J->Name,
        RQ->Index,
        (P != NULL) ? P->Name : "NULL",
        tc,
        RQ->TaskCount);

      return(FAILURE);
      }

    if (EffNC < RQ->NodeCount)
      {
      MDB(2,fSCHED) MLog("INFO:     inadequate feasible nodes found for job %s:%d in partition %s (%d < %d)\n",
        J->Name,
        RQ->Index,
        (P != NULL) ? P->Name : "NULL",
        EffNC,
        RQ->NodeCount);

      return(FAILURE);
      }
    }    /* END if (!bmisset(&J->Flags,mjfBestEffort)) */

  return(SUCCESS);
  }   /* END MReqGetFNL() */
