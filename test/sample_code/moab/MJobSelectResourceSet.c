/* HEADER */

/**
 * @file MJobSelectResourceSet.c
 * 
 * Contains the function MJobSelectResourceSet.c
 *
 */


#include "moab.h"
#include "moab-proto.h"
#include "moab-const.h"
#include "moab-global.h"




/**
 * Determines the set of nodes a job can run in. 
 *
 * NOTE:  the GLOBAL node is included in all feature node sets
 * NOTE:  resource set required to satisfy all tasks 
 * NOTE:  if req specified, select set only for req, otherwise, select for job 
 * NOTE:  w/licenses, this should be changed to 'required to satisfy all procs' (NYI) 
 *
 * @param J (I)
 * @param RQ (I) [optional]
 * @param StartTime (I)
 * @param SetType (I)
 * @param SetSelection (I)
 * @param SetList (I) [optional]
 * @param NodeList (I/O) constraining nodes/selected nodes
 * @param NodeMap (I) [optional]
 * @param SetDepth (I) optional, only enforced when >= 0
 * @param ForcedSet (I) optional, only enforced with SetDepth
 * @param SelSetIndex (O) optional, pointer to selected node set
 * @param NestedNodeSet (I)
 */

int MJobSelectResourceSet(

  const mjob_t *J,
  const mreq_t *RQ,
  mulong        StartTime,
  enum MResourceSetTypeEnum SetType,
  enum MResourceSetSelectionEnum SetSelection,
  char        **SetList,
  mnl_t        *NodeList,
  char         *NodeMap,
  int           SetDepth,
  char         *ForcedSet,
  int          *SelSetIndex,
  mln_t        *NestedNodeSet)

  {
  int      nindex;
  int      sindex;
  int      findex = 0;

  int      SetTC[MMAX_ATTR];
  int      SetNC[MMAX_ATTR];
  int      SetMem[MMAX_ATTR];
  int      SetIndex[MMAX_ATTR];
  int      SetAffinity[MMAX_ATTR]; /* positive/required=1,neutral=0,negative=-1 */
  int      SetPriority[MMAX_ATTR]; /* set priority based on nodes within set */
  int      EffSetTC[MMAX_ATTR];
  mnode_t *SetN[MMAX_ATTR];        /* representative node for specified nodeset */
  double   SetDistance[MMAX_ATTR];

  int      MinSetLoss[MMAX_ATTR];

  mpar_t  *P = NULL;

  int      MaxSet;
  int      SetMembers;

  int      Offset;

  mbool_t  NodeIsFeasible;

  int      ReqTC;
  int      ReqNC;
  int      ReqMem = 0;

  int      BestSet = -1;
  int      SetValue;

  int      TC;
  int      NC;
  int      Mem = 0;

  mnode_t *N;

  mbool_t  NodeSetIsOptional = MPar[0].NodeSetIsOptional;

  char    *VCFeature = NULL;
  mln_t   *VCL;

  const char *FName = "MJobSelectResourceSet";

  MDB(4,fSCHED) MLog("%s(%s,%d,%ld,%s,%s,%s,NodeList,SelSetIndex)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (RQ != NULL) ? RQ->Index : -1,
    StartTime,
    MResSetAttrType[SetType],
    MResSetSelectionType[SetSelection],
    (SetList != NULL) ? "SetList" : "NULL");

  if ((J == NULL) || 
      (NodeList == NULL) ||
      (MNLGetNodeAtIndex(NodeList,0,NULL) == FAILURE))
    {
    if (J == NULL)
      {
      MDB(4,fSCHED) MLog("INFO:     ignoring nodesets - job not specified\n");
      }
    else
      {
      MDB(4,fSCHED) MLog("INFO:     ignoring nodesets - invalid nodelist\n");
      }

    return(FAILURE);
    }

  if (MVMJobIsVMMigrate(J) == TRUE)
    {
    /* Ignore node set checks for VM migration jobs (already checked) */

    return(SUCCESS);
    }

  /* Allow generic system jobs to have hostlist specified.
      NOTE: why do we not allow system jobs to have a hostlist with
      nodesets on? */

  if (((J->System == NULL) || (J->System->JobType == msjtGeneric)) &&
      ((SetType == mrstNONE) || 
       (SetSelection == mrssNONE) ||
       bmisset(&J->IFlags,mjifHostList)))
    {
    /* no node set specified or job hostlist specified */

    MDB(4,fSCHED) MLog("INFO:     ignoring nodesets - %s\n",
      bmisset(&J->IFlags,mjifHostList) ? "hostlist specified" : "node set not specified");

    return(SUCCESS);
    }

  if ((SetDepth < 0) && (J->System == NULL) && (RQ != NULL) && (RQ->DRes.Procs == 0)) 
    {
    /* only apply to compute host based reqs
		 * and system jobs */

    MDB(4,fSCHED) MLog("INFO:     ignoring nodesets - noncompute req %d\n",
      RQ->Index);

    return(SUCCESS);
    }

  MNLGetNodeAtIndex(NodeList,0,&N);
  P = &MPar[N->PtIndex];

  MDB(7,fSCHED)
    {
    int nindex;
    int tc;

    tc = 0;

    for (nindex = 0;MNLGetNodeAtIndex(NodeList,nindex,NULL) == SUCCESS;nindex++)
      {
      tc += MNLGetTCAtIndex(NodeList,nindex);
      }  /* END for (nindex) */
   
    MLog("INFO:     %d nodes, %d tasks available in %s before nodeset constraints applied\n",
      nindex,
      tc,
      FName);
    }  /* END MDB(7,fSCHED) */

  /* initialize sets */

  if (RQ != NULL)
    {
    ReqTC = RQ->TaskCount;
    ReqNC = RQ->NodeCount;
    ReqMem = RQ->ReqMem;
    }
  else
    {
    ReqTC = J->Request.TC;
    ReqNC = J->Request.NC;
    }

  memset(SetTC,0,sizeof(SetTC));
  memset(SetNC,0,sizeof(SetNC));
  memset(SetMem,0,sizeof(SetMem));
  memset(SetAffinity,0,sizeof(SetAffinity));
  memset(SetPriority,0,sizeof(SetPriority));

  memset(SetIndex,0,sizeof(SetIndex));       
  memset(SetDistance,0,sizeof(SetDistance));  
  memset(MinSetLoss,0,sizeof(MinSetLoss));

  MaxSet     = 0;
  SetMembers = 0;

  if (MPar[0].NodeSetForceMinAlloc == TRUE)
    {
    if (MPar[0].NodeSetMaxSize == -1)
      {
      /* calculate maximum feasible node set size */

      if (MJobSelectResourceSet(
            J,
            RQ,
            StartTime,
            SetType,
            SetSelection,
            NULL,
            NULL,
            NULL,
            -1,
            NULL,
            NULL,
            NULL) == SUCCESS)
        {
        MPar[0].NodeSetMaxSize = MNLCount(NodeList);
        }
      }
    }    /* END if (MPar[0].NodeSetForceMinAlloc == TRUE) */

/*
  if ((NL = MUCalloc(1,sizeof(mnl_t) * ListSize)) == NULL)
    {
    return(FAILURE);
    }
*/

  /* See if there is a feature required by job or VCs (first feature 
      found will be used) */

  MJobGetNodeSet(J,SetList,&VCFeature);

  /* delineate sets */

  switch (SetType)
    {
    case mrstClass:

      MaxSet = 0;

      if ((SetList != NULL) && (SetList[0] != NULL))
        {
        int cindex;

        for (sindex = 0;sindex < MMAX_ATTR;sindex++)
          {
          if ((SetList[sindex] == NULL) || (SetList[sindex][0] == '\0'))
            break;

          /* Using MCLASS_FIRST_INDEX to skip DEFAULT class */
          for (cindex = MCLASS_FIRST_INDEX;cindex < MMAX_CLASS;cindex++)
            {
            if (MClass[cindex].Name[0] == '\0')
              break;

            if (MClass[cindex].Name[1] == '\1')
              continue;

            if (strcmp(MClass[cindex].Name,SetList[sindex]))
              continue;

            SetIndex[MaxSet] = cindex;
            MaxSet++;

            break;
            }  /* END for (cindex) */

          if ((MClass[cindex].Name[0] == '\0') || (cindex >= MMAX_CLASS))
            {
            MDB(2,fSTRUCT) MLog("WARNING:  specified class set list is invalid '%s'\n",
              SetList[sindex]);
            }
          }    /* END for (sindex) */
        }      /* END if (SetList != NULL) */
      else
        {
        int cindex;

        /* set not specified, determine all existing sets */

        SetIndex[0] = -1;

        /* search all nodes in NodeList */

        for (nindex = 0;MNLGetNodeAtIndex(NodeList,nindex,&N) == SUCCESS;nindex++)
          {
          /* determine if configured node classes match current list */

          for (cindex = MCLASS_FIRST_INDEX;cindex < MMAX_CLASS;cindex++)
            {
            if (!bmisset(&N->Classes,cindex))
              continue;

            for (findex = 0;findex < MMAX_ATTR;findex++)
              {
              if ((SetIndex[findex] == -1) || (SetIndex[findex] == cindex))
                break;
              }  /* END for (findex) */

            if (findex >= MMAX_ATTR - 1)
              {
              /* table is full */

              break;
              }

            if (SetIndex[findex] == -1)
              {
              /* new class discovered - add to list */

              SetIndex[findex] = cindex;
              SetIndex[findex + 1] = -1;

              MaxSet = findex + 1;
              }
            }    /* END for (cindex) */

          if (findex >= MMAX_ATTR - 1)
            {
            /* table is full */

            break;
            }
          }      /* END for (nindex) */

        /* terminate list */

        SetIndex[MMAX_ATTR - 1] = -1;
        }    /* END else (SetList != NULL) */

      SetIndex[MaxSet] = -1;

      break;

    case mrstFeature:

      MaxSet = 0;

      if ((SetList != NULL) && (SetList[0] != NULL))
        {
        for (sindex = 0;sindex < MMAX_ATTR;sindex++)
          {
          if ((SetList[sindex] == NULL) || (SetList[sindex][0] == '\0'))
            break;

          if ((SetDepth >= 0) && 
              (ForcedSet != NULL) &&
              (strcmp(SetList[sindex],ForcedSet)))
            {
            continue;
            }

          for (findex = 0;findex < MMAX_ATTR;findex++)
            {
            if (MAList[meNFeature][findex][0] == '\0')
              break;

            if (strcmp(MAList[meNFeature][findex],SetList[sindex]) != 0)
              continue;

            if ((VCFeature != NULL) &&
                (strcmp(MAList[meNFeature][findex],VCFeature) != 0))
              continue;

            SetIndex[MaxSet] = findex;
            MaxSet++;

            break;
            }  /* END for (findex) */
          }    /* END for (sindex) */
        }      /* END if (SetList != NULL) */
      else
        {
        /* set not specified, determine all existing sets */

        for (findex = 0;findex < MMAX_ATTR;findex++)
          {
          if (MAList[meNFeature][findex][0] == '\0')
            break;

          if ((VCFeature != NULL) &&
              (strcmp(MAList[meNFeature][findex],VCFeature) != 0))
            continue;

          SetIndex[MaxSet] = findex;
          MaxSet++;
          }  /* END for (findex) */
        }    /* END else (SetList != NULL) */
      
      SetIndex[MaxSet] = -1;
     
      break;

    case mrstLAST:
    case mrstNONE:

      return(SUCCESS);

      /*NOTREACHED*/

      break;
    }  /* END switch (SetType) */

  /* populate sets */

  memset(SetN,0,sizeof(SetN));

  for (nindex = 0;MNLGetNodeAtIndex(NodeList,nindex,&N) == SUCCESS;nindex++)
    {
    /* The J->System check is needed for VM migration stuff.  VMTracking jobs need to have
        VLAN boundaries enforced (this is called by MVMCheckVLANMigrationBoundaries */

    TC = MNLGetTCAtIndex(NodeList,nindex);
    
    /* Look at currently available memory when looking at jobs that are 
     * starting now otherwise use configured memory. */
    if (StartTime == MSched.Time)
      Mem = N->CRes.Mem - N->DRes.Mem;
    else
      Mem = N->CRes.Mem;

    switch (SetType) 
      {
      case mrstClass:

        /* step through set list */

        for (sindex = 0;sindex < MaxSet;sindex++)
          {
          if (bmisset(&N->Classes,SetIndex[sindex]))
            {
            SetTC[sindex] += TC;
            SetNC[sindex] ++;

            SetMembers += TC;

            if (SetN[sindex] == NULL)
              SetN[sindex] = N;

            if (SetSelection == mrssAnyOf)
              break;
            }
          }    /* END for (sindex) */

        break;

      case mrstFeature:

        /* step through set list */

        for (sindex = 0;sindex < MaxSet;sindex++)
          {
          if ((bmisset(&N->FBM,SetIndex[sindex])) || (N == MSched.GN))
            {
            SetTC[sindex] += TC;
            SetNC[sindex] ++;
            SetMem[sindex] += Mem;

            SetMembers += TC;

            if (SetN[sindex] == NULL)
              SetN[sindex] = N;

            if (NodeMap != NULL)
              {
              switch (NodeMap[N->Index])
                {
                case mnmNegativeAffinity:

                  SetAffinity[sindex] = -1;

                  break;

                case mnmUnavailable:
                case mnmRequired:
                case mnmPositiveAffinity:
                case mnmNeutralAffinity:
                case mnmPreemptible:
                case mnmNONE:
                default:

                  /* NO-OP */

                  break;
                } /* END switch (NodeMap[nindex]) */
              }   /* END if (NodeMap != NULL) */

            if (SetSelection == mrssAnyOf)
              break;
            }
          }    /* END for (sindex) */
            
        break;

      default:

        /* NYI */

        break;
      }  /* END switch (SetType) */
    }    /* END for (nindex)    */

  /* perform set level operations */

  /* determine set 'fitness' levels */

  memcpy(EffSetTC,SetTC,sizeof(EffSetTC));

  if ((bmisset(&P->Flags,mpfSharedMem)) &&
      (MPar[0].NodeSetPriorityType == mrspMinLoss))
    {
    /* calculate number of nodes required to satisfy job */

    for (sindex = 0;sindex < MaxSet;sindex++)
      {
      if (SetN[sindex] == NULL)
        {
        MinSetLoss[sindex] = -1;

        continue;
        }

      if (MSMPJobIsFeasible(J,SetIndex[sindex],StartTime) == SUCCESS)
        MinSetLoss[sindex] = (RQ != NULL) ? MReqGetNBCount(J,RQ,SetN[sindex]) : MJobGetNBCount(J,SetN[sindex]);
      else
        MinSetLoss[sindex] = -1;
      }
    } /* if (MSched.SharedMem == TRUE) */

  MDB(6,fSCHED)
    {
    for (sindex = 0;sindex < MaxSet;sindex++)
      {
      MLog("INFO:     set[%d] '%s' %d  TC=%d  NC=%d\n",
        sindex,
        "N/A",
        SetIndex[sindex],
        SetTC[sindex],
        SetNC[sindex]);
      }
    }

  if ((J->System == NULL) &&
      (SetMembers < ReqTC))
    {
    MDB(2,fSCHED) MLog("INFO:     inadequate resources found in any set (%d < %d)\n",
      SetMembers,
      ReqTC);

    /* NOTE: scary DAVE code */

    MNLClear(NodeList);

    return(FAILURE);
    }

  /* select resources from various sets */

  switch (SetSelection)
    {
    case mrssAnyOf:

      if (MSched.NodeSetPlus == mnspSpanEvenly)
        {
        /* span evenly is implemented in MJobGetEStartTime() and MJobGetRangeValue() */

        /* this section of code should return an error so that those routines get called */

        MNLClear(NodeList);

        return(FAILURE);
        }

      /* Set the NodeSetIsOptional flag and let code fall into mrssOneOf. */

      NodeSetIsOptional = TRUE;

      /* break;  Let code fall through. */

    case mrssOneOf:

      /* select all sets which contain adequate resources to support job */ 

      /* if NodeSetIsOptional == TRUE, select all sets which contain any resources *
       * capable of supporting job */

      /* verify at least one set contains adequate resources */

      /* NOTE:  set resources are allowed if set tolerance is specified *
       *        and border sets lie within tolerances                   */

      /* perform feasibility check */    

      TC = 0;
      NC = 0;
      Mem = 0;

      for (sindex = 0;sindex < MaxSet;sindex++)
        {
        if (((EffSetTC[sindex] < ReqTC) || 
             (SetNC[sindex] < ReqNC) ||
             ((bmisset(&P->Flags,mpfSharedMem)) && (SetMem[sindex] < ReqMem))) &&
            (NodeSetIsOptional == FALSE))
          {
          continue;
          }

        /* set contains partial or complete resources */

        TC += EffSetTC[sindex];
        NC += SetNC[sindex];
        Mem += SetMem[sindex];
        }

      if (((J->System == NULL) && (TC < ReqTC)) || 
          (NC < ReqNC) ||
          ((bmisset(&P->Flags,mpfSharedMem)) && (Mem < ReqMem)))
        {
        /* no compute set contains adequate resources
         * or no system job set contains adequate nodes  */

        /* NOTE: scary DAVE code */

        MNLClear(NodeList);

        return(FAILURE);
        }

      /* at least one adequate set exists */

      Offset = 0;

      for (nindex = 0;MNLGetNodeAtIndex(NodeList,nindex,&N) == SUCCESS;nindex++)
        {
        /* determine if node is a member of any feasible sets */

        NodeIsFeasible = FALSE;

        for (sindex = 0;sindex < MaxSet;sindex++)
          {
          if (((EffSetTC[sindex] < ReqTC) || 
               (SetNC[sindex] < ReqNC) ||
               ((bmisset(&P->Flags,mpfSharedMem)) && (SetMem[sindex] < ReqMem))) &&
              (NodeSetIsOptional == FALSE))
            {
            continue;
            }

          switch (SetType)
            {
            case mrstClass:

              if (bmisset(&N->Classes,SetIndex[sindex]))
                {
                /* node is feasible */

                if (Offset > 0)
                  MNLCopyIndex(NodeList,nindex - Offset,NodeList,nindex);

                /* bound available tasks by available/config class instances */

                MNLSetTCAtIndex(NodeList,nindex - Offset,MNLGetTCAtIndex(NodeList,nindex - Offset));

                NodeIsFeasible = TRUE;
                }
 
              break;

            case mrstFeature:

              if ((bmisset(&N->FBM,SetIndex[sindex])) || (N == MSched.GN))
                {  
                /* node is feasible */

                if (Offset > 0)
                  MNLCopyIndex(NodeList,nindex - Offset,NodeList,nindex);
  
                NodeIsFeasible = TRUE;
                }         

              break;

            default:

              /* not supported */

              break;
            }  /* END switch (SetType) */

          if (NodeIsFeasible == TRUE)
            break;
          }    /* END for (sindex)    */

        if (NodeIsFeasible == FALSE)
          Offset++;
        }      /* END for (nindex)    */

      MNLTerminateAtIndex(NodeList,nindex - Offset);

      break;

    case mrssFirstOf:

      {
      /* select best set which contains adequate resources to support job */

      SetValue = -1;
      BestSet  = -1;

      for (sindex = 0;sindex < MaxSet;sindex++)
        {
        /* check feasiablility of set. */
        if ((EffSetTC[sindex] < ReqTC) || 
            (SetNC[sindex] < ReqNC) ||
            ((bmisset(&P->Flags,mpfSharedMem)) && (SetMem[sindex] < ReqMem)))
          {
          continue;
          }

        MDB(7,fSCHED) MLog("INFO:     Checking set %s (index: %d)\n",
          SetList[sindex],
          sindex);

        switch (MPar[0].NodeSetPriorityType)
          {
          case mrspAffinity:

            if ((BestSet == -1) || (SetAffinity[sindex] > SetValue))
              {
              BestSet  = sindex;
              SetValue = SetAffinity[sindex];
              }                   

            break;

          case mrspFirstFit:

            if (BestSet == -1)
              BestSet = sindex;

            break;

          case mrspBestFit:

            if ((SetValue == -1) || (EffSetTC[sindex] < SetValue)) 
              {
              BestSet  = sindex;
              SetValue = EffSetTC[sindex];
              }                   

            break;

          case mrspWorstFit:

            if (EffSetTC[sindex] > SetValue)
              {
              BestSet  = sindex;
              SetValue = EffSetTC[sindex];
              }
 
            break;          

          case mrspMinLoss:

            /* select set of resources with minimum delta between fastest and slowest nodes */

            if (bmisset(&P->Flags,mpfSharedMem))
              {
              /* select nodeset based on minimum number of nodes required */

              /* NOTE:  assume all nodes have identical proc/node and mem/node 
                        config within nodeset */

              /* once CPUBoard implementation is complete, see if this stanza can
                 be collapsed with stanza above */
              
              if ((SetNC[sindex] >= MinSetLoss[sindex]) &&
                  ((SetValue == -1) || (MinSetLoss[sindex] <= SetValue)))
                {
                if (MinSetLoss[sindex] == -1)
                  continue;

                /* select min span nodes */
                if ((BestSet != -1) && (MinSetLoss[sindex] == SetValue))
                  {
                  double Use1;
                  double Use2;

                  /* determine which one is the best of the two */

                  /* ex. smp1 = per node: 2 procs, 11856 mb; total: 504 procs, 2987648 mb
                   *     smp2 = per node: 4 procs, 11856 mb; total: 248 procs, 735041 mb
                   *     job requests 2 procs and 20480 mem both need 2 nodeboards. 
                   *     The job should go to smp1 since it uses a lesser percentage of the node than smp2. */

                  if (((Use1 = MSMPNodeGetJobUse(SetN[sindex],J,StartTime)) < 0.0) ||
                      ((Use2 = MSMPNodeGetJobUse(SetN[BestSet],J,StartTime)) < 0.0))
                    continue;

                  if (Use1 < Use2)
                    {
                    MDB(7,fSCHED) MLog("INFO:     choosing set %d over %d due to job using fewer resources.\n",sindex,BestSet);

                    BestSet  = sindex;
                    SetValue = MinSetLoss[sindex];
                    }
                  }
                else
                  {
                  BestSet  = sindex;
                  SetValue = MinSetLoss[sindex];
                  }
                } /* END if ((SetValue == -1) */
              }
            else
              {
              BestSet = sindex;

              break;
              }

            break;

          case mrspBestResource:         
          default:
  
            BestSet = sindex;

            break;
          }  /* END switch (BestSetSelection) */

        if (MPar[0].NodeSetPriorityType == mrspBestResource)
          {
          break;
          }
        }      /* END for (sindex) */

      if (MPar[0].NodeSetPriorityType == mrspMinLoss)
        {
        MDB(7,fSCHED)
          {
          for(sindex = 0;sindex < MaxSet;sindex++)
            {
            MLog("INFO:     MinSetLoss[%d] = %d\n",
              sindex,
              MinSetLoss[sindex]);
            }
          
          MLog("INFO:     BestSet = %d (0 indexed)\n",BestSet);
          }
        }

      if (BestSet == -1)
        {
        /* no set contains adequate resources */

        /* NOTE: scary DAVE code */

        MNLClear(NodeList);
  
        if (SelSetIndex != NULL)
          *SelSetIndex = -1;

        return(FAILURE);
        } 

      /* Set VC feature */

      for (VCL = J->ParentVCs;VCL != NULL;VCL = VCL->Next)
        {
        mvc_t *tmpVC = (mvc_t *)VCL->Ptr;

        if (tmpVC == NULL)
          continue;

        MVCSetNodeSet(tmpVC,SetIndex[BestSet]);
        }

      Offset = 0;
 
      for (nindex = 0;MNLGetNodeAtIndex(NodeList,nindex,&N) == SUCCESS;nindex++)
        {
        /* determine if node is a member of selected set */
 
        switch (SetType)
          {
          case mrstClass:

            if (bmisset(&N->Classes,SetIndex[BestSet]))
              {
              /* node is in set */

              if (Offset > 0)
                MNLCopyIndex(NodeList,nindex - Offset,NodeList,nindex);

              /* adjust available taskcount */

              MNLSetTCAtIndex(NodeList,nindex - Offset,MNLGetTCAtIndex(NodeList,nindex - Offset));
              }
            else
              {
              Offset++;
              }

            break;

          case mrstFeature:
 
            if ((bmisset(&N->FBM,SetIndex[BestSet])) || (N == MSched.GN))
              {
              /* node is in set */
 
              if (Offset > 0)
                MNLCopyIndex(NodeList,nindex - Offset,NodeList,nindex);
              }
            else
              {
              Offset++;
              }
 
            break;

          default:                      

            /* NO-OP */

            break;
          }  /* END switch (SetType) */
        }    /* END for (nindex)    */
 
      MNLTerminateAtIndex(NodeList,nindex - Offset);
      }

      break;                                     
                  
    case mrssNONE:
    default:

      /* no set selection mode specified */

      return(SUCCESS);

      /*NOTREACHED*/

      break;
    }  /* END switch (SetSelection) */

  MDB(7,fSCHED)
    {
    int nindex;
    int tc;

    tc = 0;

    for (nindex = 0;MNLGetNodeAtIndex(NodeList,nindex,NULL) == SUCCESS;nindex++)
      {
      tc += MNLGetTCAtIndex(NodeList,nindex);
      }  /* END for (nindex) */

    MLog("INFO:     %d nodes, %d tasks available in %s after nodeset constraints applied\n",
      nindex,
      tc,
      FName);
    }  /* END MDB(7,fSCHED) */

  if (MNLIsEmpty(NodeList))
    {
    return(FAILURE);
    }

  if (SelSetIndex != NULL)
    {
    if (BestSet != -1)
      *SelSetIndex = SetIndex[BestSet];
    else
      *SelSetIndex = 0;
    }
 
  if (SetDepth >= 0)
    {
    int    index;

    char   tmpName[MMAX_NAME];

    mln_t *tmpL;

    mnl_t  tmpNodeList;

    char  **List;

    snprintf(tmpName,sizeof(tmpName),"%d",SetDepth + 1);

    if (MULLCheck(NestedNodeSet,tmpName,&tmpL) == FAILURE)
      {
      /* we're as deep as we're going to go */

      return(SUCCESS);
      }

    List = (char **)tmpL->Ptr;

    MNLInit(&tmpNodeList);

    for (index = 0;List[index] != NULL;index++)
      {
      /* need to copy NodeList, SelSetIndex */

      MNLCopy(&tmpNodeList,NodeList);

      if (MJobSelectResourceSet(
            J,
            RQ,
            StartTime,
            SetType,
            SetSelection,
            (char **)tmpL->Ptr,
            NodeList,     /* keep constraining futher */
            NodeMap,
            SetDepth + 1,     /* I (optional) */
            List[index],
            SelSetIndex,
            NULL) == SUCCESS)
        {
        /* found something...hurray! */

        MNLFree(&tmpNodeList);

        return(SUCCESS);
        }
      
      /* copy back */

      MNLCopy(NodeList,&tmpNodeList);
      }   /* END for (index) */

    MNLFree(&tmpNodeList);

    return(FAILURE);
    }   /* END if (SetDepth >= 0) */

  return(SUCCESS);
  }  /* END MJobSelectResourceSet() */


