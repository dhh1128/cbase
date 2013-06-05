/* HEADER */

      
/**
 * @file MNodeCRUD.c
 *
 * Contains: CRUD is Create, Read, Update and Delete.
 *           This file has the high level functions that do the CRUD
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/* Global data used by functions in this file */

static int MFreeNodeIndex = 0;




/**
 * Initialize/set defaults for new node.
 *
 * @see MNodeCreate()
 * @see MNodeAdd()
 *
 * @param N (I) [cleared/initialized]
 * @param Name (I) [optional]
 */

int MNodeInitialize(

  mnode_t    *N,    /* I (cleared/initialized) */
  const char *Name) /* I (optional) */

  {
  if (N == NULL)
    {
    return(FAILURE);
    }

  memset(N,0,sizeof(mnode_t));

  if (Name != NULL)
    strcpy(N->Name,Name);

  /* set defaults */

  N->KbdDetectPolicy = MDEF_NODEKBDDETECTPOLICY;

  if ((Name != NULL) && !strcmp(Name,DEFAULT))
    {
    N->MinResumeKbdIdleTime = MDEF_MINRESUMEKBDIDLETIME;
    }
  else
    {
    N->MinResumeKbdIdleTime = -1;
    }

  N->SpecNAPolicy   = mnacNONE;
  N->EffNAPolicy    = N->SpecNAPolicy;

  N->MinPreemptLoad = MDEF_MINPREEMPTLOAD;

  N->PowerIsEnabled = MBNOTSET;
  N->PowerState     = mpowNONE;
  N->PowerPolicy    = mpowpNONE;

  N->MIteration = -1;

  N->RackIndex  = -1;
  N->SlotIndex  = -1;

  N->ProfilingEnabled = MBNOTSET;

  return(SUCCESS);
  }  /* END MNodeInitialize() */


/**
 * Alloc/create new node.
 * 
 * @see MNodeFind()
 * @see MNodeAdd() - adds node to node table
 * @see MNodeDestroy() - peer - destroys/frees node
 *
 * @param NP (I)
 * @param IsShared (I)
 */

int MNodeCreate(

  mnode_t **NP,        /* I */
  mbool_t   IsShared)  /* I */

  {
  mnode_t *N;

  if (NP == NULL)
    {
    return(FAILURE);
    }

  if (*NP == NULL)
    {
    *NP = (mnode_t *)MUMalloc(sizeof(mnode_t));

    N = *NP;      

    MSched.A[mxoNode]++;
    }
  else
    {
    N = *NP;
    }

  MNodeInitialize(N,NULL);

  N->MaxJCount = MMAX_JOB_PER_NODE;

  N->JList = (mjob_t **)MUCalloc(1,sizeof(mjob_t *) * (N->MaxJCount + 1));
  N->JTC   = (int *)MUCalloc(1,sizeof(int) * (N->MaxJCount + 1));

  if (MSched.AS[mxoNode] == 0)
    {
    MSched.AS[mxoNode] =
      sizeof(mnode_t) +
      sizeof(mrsv_t *) * (MSched.MaxRsvPerNode + 1) +
      sizeof(mre_t) * ((MSched.MaxRsvPerNode << 1) + 1) +
      sizeof(int) * (MSched.MaxRsvPerNode + 1) +
      sizeof(void *) * (N->MaxJCount + 1) +
      sizeof(int) * (N->MaxJCount + 1);
    }

  MSNLInit(&N->CRes.GenericRes);
  MSNLInit(&N->DRes.GenericRes);
  MSNLInit(&N->ARes.GenericRes);

  N->Stat.XLoad = (double *)MUCalloc(1,sizeof(double) * MSched.M[mxoxGMetric]);

  MSNLInit(&N->Stat.CGRes);
  MSNLInit(&N->Stat.DGRes);
  MSNLInit(&N->Stat.AGRes);

#ifdef __MNOT
  if (MSched.UseNodeIndex != FALSE)
    {
    int niindex;
    int index;

    /* NOTE:  allow decimal and hex specification */

    if (isdigit(N->Name[0]))
      {
      niindex = (int)strtol(N->Name,NULL,0) % (MSched.M[mxoNode] << 2);
      }
    else
      {
      nindex = MUGetHash(NName) % (MSched.M[mxoNode] << 2);
      }

    for (index = niindex;index < niindex + MMAX_NHBUF;index++)
      {
      if ((MNodeIndex[index] <= MCONST_NHNEVERSET) ||
          (MNodeIndex[index] == N->Index))
        {
        MNodeIndex[index] = N->Index;

        break;
        }
      }    /* END for (index) */

    if (MNodeIndex[index] != N->Index)
      {
      MDB(5,fSTRUCT) MLog("ERROR:    cannot add node '%s' in %s - node index %d already used\n",
        N->Name,
        FName,
        niindex);

      MMBAdd(&MSched.MB,"ERROR:  node hash buffer full - expand node table",NULL,mmbtNONE,0,0,NULL);
      }
    }    /* END if (MSched.UseNodeIndex != FALSE) */
#endif

  /* NOTE:  should insert new node into sorted node list for quick lookup (NYI) */

  return(SUCCESS);
  }  /* END MNodeCreate() */





/**
 * Add new node to node table.
 *
 * Doesn't add node if already exists
 *
 * @see MNodeCreate() - peer - creates node object
 * @see MNodeRemove() - peer - removes node from global node table
 * @see MNodeFind() - peer - locate existing node
 *
 * @param NName (I) node name
 * @param NP (O) [optional]
 */

int MNodeAdd(
 
  const char  *NName,
  mnode_t    **NP)
 
  {
  int nindex;
  int rc;

  mnode_t *N;

  mbool_t  IsGlobal;

  const char *FName = "MNodeAdd";

  MDB(5,fSTRUCT) MLog("%s(%s,NP)\n",
    FName,
    (NName != NULL) ? NName : NULL);

  if (NP != NULL)
    *NP = NULL;

  if ((NName == NULL) || (NName[0] == '\0'))
    {
    /* invalid name specified */

    return(FAILURE);
    }

  if ((MSched.SpecNodes.List != NULL) || (MSched.IgnoreNodes.List != NULL))
    {
    if (MUCheckListR(NName,&MSched.SpecNodes,&MSched.IgnoreNodes) == FALSE)
      {
      return(FAILURE);
      }
    }

  if (MNodeFind(NName,&N) == SUCCESS)
    {
    /* node already exists */

    if (NP != NULL)
      *NP = N;
   
    return(SUCCESS);
    }

  /* NOTE:  Global check changed to be case-sensitive in 5.2, should be 
            case insensitive in 5.3 */

  if (!strcmp(NName,"GLOBAL"))
    IsGlobal = TRUE;
  else
    IsGlobal = FALSE;

  /* NOTE:  MFreeNodeIndex is global shared with MNodeFind()/MNodeDestroy() */
 
  for (nindex = MFreeNodeIndex;nindex < MSched.M[mxoNode];nindex++)
    {
    /* if available node slot is found */

    N = MNode[nindex];        
 
    if ((N == NULL) ||
        (N->Name[0] == '\0') || 
        (N->Name[0] == '\1'))
      {
      /* empty node slot located */

      MFreeNodeIndex = nindex;

      break;
      }
    }    /* END for (nindex) */

  if (nindex >= MSched.M[mxoNode])
    {
    /* no space found in node table */
 
    MDB(1,fSTRUCT) MLog("WARNING:  node buffer overflow in %s()\n",
      FName);
 
    MDB(1,fSTRUCT) MLog("INFO:     node table is full (cannot add node '%s')\n",
      NName);

    if (MSched.OFOID[mxoNode] == NULL)
      MUStrDup(&MSched.OFOID[mxoNode],NName);

    return(FAILURE);
    }

  /* available node slot found */

  if (MNodeCreate(&N,IsGlobal) == FAILURE)
    {
    MDB(1,fSTRUCT) MLog("ERROR:  cannot create node '%s'\n",
      NName);

    return(FAILURE);
    }

  MNode[nindex] = N;

  MSched.AddDynamicNC -= 1;

  /* initialize node */

  if (IsGlobal == TRUE)
    {
    mpar_t *P;

    /* link/initialize global node */

    MSched.GN = N;

    MNodeSetState(N,mnsIdle,FALSE);

    if (MSched.SharedPtIndex <= 0)
      {
      if (MParAdd(MCONST_SHAREDPARNAME,&P) == FAILURE)
        {
        MDB(5,fSTRUCT) MLog("ERROR:    cannot create 'shared' partition for 'global' node\n");
 
        MSched.ParBufOverflow = TRUE;

        return(FAILURE);
        }

      MSched.SharedPtIndex = P->Index;
      }

    if (MSched.DefaultN.ProfilingEnabled == TRUE)
      {
      mnust_t *SPtr = NULL;

      if ((MOGetComponent((void *)N,mxoNode,(void **)&SPtr,mxoxStats) == SUCCESS) &&
          (SPtr->IStat == NULL))
        {
        MStatPInitialize((void *)SPtr,FALSE,msoNode);
        }
      }

    N->PtIndex = MSched.SharedPtIndex;

    memset(N->Name,0,sizeof(N->Name));

    MUStrCpy(N->Name,"GLOBAL",sizeof(N->Name));
    }  /* END if (IsGlobal == TRUE) */
  else
    {
    memset(N->Name,0,sizeof(N->Name));

    MUStrCpy(N->Name,NName,sizeof(N->Name));

    if (MSched.DefaultN.ProfilingEnabled == TRUE)
      {
      mnust_t *SPtr = NULL;

      if ((MOGetComponent((void *)N,mxoNode,(void **)&SPtr,mxoxStats) == SUCCESS) &&
          (SPtr->IStat == NULL))
        {
        MStatPInitialize((void *)SPtr,FALSE,msoNode);
        }
      }

    MNodeProcessName(N,N->Name);

    if (MSched.JobCCodeFailSampleSize > 0)
      {
      N->JobCCodeSampleCount = (int *)calloc(1,sizeof(int) * MMAX_CCODEARRAY);
      N->JobCCodeFailureCount = (int *)calloc(1,sizeof(int) * MMAX_CCODEARRAY);
      }
    }  /* END else */

  N->Index = nindex;

  if (MSched.UseNodeIndex != FALSE)
    {
    int niindex;
    int index;

    /* NOTE:  accept decimal or hex node names */

    /* MNodeIndex[] is 4x MSched.M[mxoNode] to handle sparse arrays */

    if ((MSched.UseNodeIndex == MBNOTSET) && 
        (IsGlobal == FALSE) && 
        !isdigit(N->Name[0]))
      {
      MSched.UseNodeIndex = FALSE;
      }
    else
      {
      if (isdigit(N->Name[0]))
        {
        niindex = (int)strtol(N->Name,NULL,0) % (MSched.M[mxoNode] << 2);
        }
      else
        {
        niindex = MUGetHash(N->Name) % (MMAX_NHBUF) + (MSched.M[mxoNode] << 2);
        }

      for (index = niindex;index < niindex + MMAX_NHBUF;index++)
        {
        if ((MNodeIndex[index] <= MCONST_NHNEVERSET) || 
            (MNodeIndex[index] == N->Index))
          {
          MNodeIndex[index] = N->Index;

          break;
          }
        }    /* END for (index) */
   
      if (MNodeIndex[index] != N->Index)
        {
        MDB(5,fSTRUCT) MLog("ERROR:    cannot add node '%s' - node index %d already used\n",
          N->Name,
          niindex);
        }
      }    /* END else ((MSched.UseNodeIndex == MBNOTSET) && ...) */
    }      /* END if (MSched.UseNodeIndex != FALSE) */

  /* NOTE:  insert new node into sorted node list for quick lookup? (NYI) */
 
  if (NP != NULL)
    *NP = N;
 
  /* Add the node to the hash table wth the new name as the key */

  if (MSched.NodeNameCaseInsensitive == TRUE)
    rc = MUHTAddCI(&MNodeIdHT,N->Name,MUIntDup(N->Index),NULL,MUFREE);
  else
    rc = MUHTAdd(&MNodeIdHT,N->Name,MUIntDup(N->Index),NULL,MUFREE);

  if (rc == FAILURE)
    {
    MDB(3,fSTRUCT) MLog("ERROR:    cannot add node '%s' to hash table with node index %d \n",
      N->Name,
      N->Index);

    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MNodeAdd() */




/**
 * Remove node from node table.
 *
 * NOTE:  also remove from all RM's which support system modify operation
 *
 * @see MNodeAdd() - peer - add node to node table
 * @see MRMDestroy() - parent - remove dynamic RM and child objects
 * @see MSysDestroyObjects() - parent - remove objects at shutdown
 * @see MNodeCheckStatus() - parent - purge stale node objects
 *
 * @param N (I/O) [modified - freed]
 * @param KeepProvisioning (I) don't remove node from provisioning RM
 * @param Policy (I)
 * @param EMsg (I/O) [minsize=MMAX_LINE,optional]
 */

int MNodeRemove(

  mnode_t *N,
  mbool_t  KeepProvisioning,
  enum MNodeDestroyPolicyEnum Policy,
  char    *EMsg)

  {
  mbool_t RemoveNode;

  char    NodeName[MMAX_NAME << 1];

  int      rmindex;
  mrm_t   *RM;

  enum MNodeDestroyPolicyEnum NDPolicy;

  const char *FName = "MNodeRemove";

  MDBO(5,N,fSTRUCT) MLog("%s(%s,%s,EMsg)\n",
    FName,
    (N != NULL) ? N->Name : "NULL",
    MNodeDestroyPolicy[Policy]);

  if (EMsg != NULL)
    {
    EMsg[0] = '\0';
    }
 
  if ((N == NULL) || (N->Name[0] == '\1'))
    {
    return(SUCCESS);
    }

  RM = N->RM;

  RemoveNode = TRUE;

  if ((Policy == mndpNONE) && (RM != NULL))
    NDPolicy = RM->NodeDestroyPolicy;
  else
    NDPolicy = Policy;

  /* Remove from VCs */

  if (N->ParentVCs != NULL)
    {
    MVCRemoveObjectParentVCs((void *)N,mxoNode);
    }

  if (N->VMList != NULL)
    {
    if (EMsg != NULL)
      MUStrCpy(EMsg,"cannot remove node with active VMs",MMAX_LINE);

    return(FAILURE);
    }

  switch (NDPolicy)
    {
    case mndpAdmin:

      /* NOTE:  node will be purged at end of iteration in MNodeCheckStatus() */

      if (EMsg != NULL)
        MUStrCpy(EMsg,"node will be purged at end of iteration",MMAX_LINE);

      bmset(&N->IFlags,mnifPurge);

      RemoveNode = FALSE;

      break;

    case mndpHard:

      /* not supported */

      RemoveNode = FALSE;

      break;

    case mndpSoft:

      if (((N->RE != NULL) &&
           (N->RE->Type != 0)) ||
           (N->JList != NULL))
        {
        MDBO(5,N,fSTRUCT) MLog("WARNING:  cannot remove node %s, reservation references remain\n",
          (N != NULL) ? N->Name : "NULL");

        RemoveNode = FALSE;
        }

      break;

    case mndpDrain:

      if ((N->State == mnsIdle) || (N->EState == mnsIdle))
        {
        RemoveNode = TRUE;

        N->SpecState      = mnsDrained;

        if (N->JList[0] != NULL)
          {
          RemoveNode = FALSE;
          }
        }
      else
        {
        /* mark node for deletion when idle */

        N->SpecState = mnsDrained;

        MNodeSetState(N,mnsDrained,TRUE);

        RemoveNode = FALSE;
        }

      break;

    default:

      RemoveNode = FALSE;

      break;
    }  /* END switch (MSched.NodeDestroyPolicy) */

  if (RemoveNode == FALSE)
    {
    return(SUCCESS);
    }

  if (N->RackIndex > 0)
    MRack[N->RackIndex].NodeCount--;

  MUStrCpy(NodeName,N->Name,sizeof(NodeName));

  /* perform per-RM operations */

  for (rmindex = 0;rmindex < MMAX_RM;rmindex++)
    {
    char tmpLine[MMAX_LINE];
    char OBuf[MMAX_LINE];
    char tEMsg[MMAX_LINE];

    /* RMList may be null */
    if (N->RMList != NULL)
      {
      RM = N->RMList[rmindex];
      }

    if (RM == NULL)
      {
      /* end of list reached */

      break;
      }

    /* NOTE:  if system modify interface defined, use it, otherwise just purge 
              node record */

    if ((RM->Type == mrmtNative) && 
        (RM->ND.URL[mrmSystemModify] == NULL))
      {
      /* no node destroy URL specified */

      continue;
      }

    if ((bmisset(&RM->RTypes,mrmrtProvisioning)) && 
        (KeepProvisioning == TRUE))
      {
      /* do not remove provisioning RM's */

      continue;
      }

    if (MRMSystemModify(RM,"node:destroy",NodeName,TRUE,OBuf,mfmNONE,tEMsg,NULL) == FAILURE)
      {
      /* NOTE:  routine will fail if any system modify RM cannot remove node - is this correct? */

      MDB(1,fSTRUCT) MLog("ALERT:    cannot remove node %s - %s\n",
        N->Name,
        tEMsg);

      snprintf(tmpLine,sizeof(tmpLine),"cannot remove node - %s",
        tEMsg);

      MMBAdd(
        &N->MB,
        (char *)tmpLine,
        MSCHED_SNAME,
        mmbtAnnotation,
        MMAX_TIME,
        0,
        NULL);

      /* NOTE:  continue to remove node locally even if one or more node RM's fail */

      continue;
      }
    }    /* END for (rmindex) */

  /* perform node operations */

  MNodeDestroy(&N);

  if (MSched.NodeNameCaseInsensitive == TRUE)
    MUHTRemoveCI(&MNodeIdHT,NodeName,MUFREE);
  else
    MUHTRemove(&MNodeIdHT,NodeName,MUFREE);

  return(SUCCESS);
  }  /* MNodeRemove() */




/**
 * Free an OSList array
 *
 * @see mnode_t.{OSList,VMOSList} 
 *
 * @param OSListPtr (I) freed, pointer to an OSList array
 * @param OSListSize (I) size of OSList
 */

int MNodeFreeOSList(

  mnodea_t  **OSListPtr,  /* I */
  int const   OSListSize) /* I */

  {
  if ((OSListPtr == NULL) || (*OSListPtr == NULL))
    {
    return(SUCCESS);
    }

  MUFree((char **)OSListPtr);

  return(SUCCESS);
  }  /* END MNodeFreeOSList() */





/**
 * Destroy node and free dynamically allocated memory.
 *
 * @see MNodeCreate() - peer
 *
 * NOTE:  clears N object, free attributes, but does NOT free NP
 *
 * @param NP (I) [modified]
 */

int MNodeDestroy(

  mnode_t **NP)  /* I (modified) */

  {
  int               rindex;
  mnode_t          *N;
  char             *GEName;
  mgevent_obj_t    *GEvent;
  mgevent_iter_t    GEIter;

  const char *FName = "MNodeDestroy";

  MDB(4,fSCHED) MLog("%s(%s)\n",
    FName,
    (NP != NULL) ? (*NP)->Name : "NULL");

  if ((NP == NULL) || (*NP == NULL))
    {
    return(SUCCESS);
    }

  N = *NP;

  MDB(2,fSCHED) MLog("INFO:     destroying node '%s'\n",
    N->Name);

  if (MSched.Shutdown == FALSE)
    {
    char tmpLine[MMAX_LINE];

    snprintf(tmpLine,sizeof(tmpLine),"Destroying node %s",
      N->Name);

    MOWriteEvent((void *)N,mxoNode,mrelNodeDestroy,tmpLine,MStat.eventfp,NULL);
    }

  /* release dynamic memory */

  MUFree(&N->FullName);

  if (N->RE != NULL)
    {
    MREFree(&N->RE);
    }

  if (N->Stat.IStat != NULL)
    {
    MStatPDestroy((void ***)&N->Stat.IStat,msoNode,N->Stat.IStatCount);
    }

  MUFree(&N->Message);

  MUFree(&N->SubState);
  MUFree(&N->LastSubState);

  MUFree((char **)&N->ResOvercommitFactor);
  MUFree((char **)&N->ResOvercommitThreshold);
  MUFree((char **)&N->GMetricOvercommitThreshold);

  MUFree((char **)&N->JList);
  MUFree((char **)&N->JTC);
  MUFree((char **)&N->ACL);

  MUFree((char **)&N->P);

  MCResFree(&N->CRes);
  if (N->RealRes != NULL)
    {
    MCResFree(N->RealRes);
    MUFree((char **)&N->RealRes);
    }
  MCResFree(&N->ARes);
  MCResFree(&N->DRes);

  MSNLFree(&N->Stat.CGRes);
  MSNLFree(&N->Stat.DGRes);
  MSNLFree(&N->Stat.AGRes);

  MGEventIterInit(&GEIter);
  while (MGEventItemIterate(&N->GEventsList,&GEName,&GEvent,&GEIter) == SUCCESS)
    {
    MGEventRemoveItem(&N->GEventsList,GEName);
    
    MUFree((char **)&GEvent->GEventMsg);
    MUFree((char **)&GEvent->Name);
    MUFree((char **)&GEvent);
    }
  
  MGEventFreeItemList(&N->GEventsList,FALSE,NULL);

  if (N->XLoad != NULL)
    MXLoadDestroy(&N->XLoad);

  MUFree((char **)&N->XLimit);

  for (rindex = 0;rindex < MSched.M[mxoRM];rindex++)
    {
    MUFree(&N->RMMsg[rindex]);
    }

  MUFree(&N->KbdSuspendedJobList);

  if (N->FSys != NULL)
    {
    int fsindex;

    for (fsindex = 0;fsindex < MMAX_FSYS;fsindex++)
      MUFree(&N->FSys->Name[fsindex]);

    MUFree((char **)&N->FSys);
    }  /* END if (N->FSys != NULL) */

  MMBFree(&N->MB,TRUE);

  MNodeFreeOSList(&N->OSList,MMAX_NODEOSTYPE);
  MNodeFreeOSList(&N->VMOSList,MMAX_NODEOSTYPE);

  MUFree((char **)&N->RMList);

  MUHTFree(&N->Variables,TRUE,MUFREE);
  MULLFree(&N->AllocRes,NULL);

  MVMListDestroy(N->VMList);  /* cannot simply free VM's...need to destroy them one-by-one */

  MULLFree(&N->VMList,NULL);  /* actual VM objects are free'd in MVMListDestroy() */

  MUFree((char **)&N->AttrString);

  MUFree((char **)&N->NP);

  MUFree((char **)&N->Stat.XLoad);

  if (N->Stat.IStat != NULL)
    MStatPDestroy((void ***)&N->Stat.IStat,msoNode,-1);

  if (N->T != NULL)
    {
    MTListClearObject(N->T,FALSE);

    MUArrayListFree(N->T);

    MUFree((char **)&N->T);
    }  /* END if (R->T != NULL) */

  /* MFreeNodeIndex is global shared w/MNodeAdd() */

  MFreeNodeIndex = MIN(MFreeNodeIndex,N->Index);

  /* NOTE:  clear but do not release memory allocated to mnode_t struct */

  if (MSched.UseNodeIndex != FALSE) 
    {
    /* clear node hash table */

    int niindex;
    int index;

    /* NOTE:  accept decimal or hex node names */

    if (isdigit(N->Name[0]))
      niindex = (int)strtol(N->Name,NULL,0) % (MSched.M[mxoNode] << 2);
    else
      niindex = MUGetHash(N->Name) & (MSched.M[mxoNode] << 2);

    for (index = niindex;index < niindex + MMAX_NHBUF;index++)
      {
      if (MNodeIndex[niindex + index] == N->Index)
        {
        /* matching has entry located */

        MNodeIndex[niindex + index] = MCONST_NHCLEARED;

        break;
        }

      if (MNodeIndex[niindex + index] == MCONST_NHNEVERSET)
        break;
      }  /* END for (index) */
    }    /* END if (MSched.UseNodeIndex != FALSE) */

  N->AttrList.clear();

  MUFreeActionTable(&N->Action,TRUE);

  MUFree(&N->NodeType);
  MUFree(&N->NetAddr);

  bmclear(&N->Flags);
  bmclear(&N->IFlags);
  bmclear(&N->RMAOBM);
  bmclear(&N->Classes);
  bmclear(&N->FBM);

  if (MSched.Shutdown == FALSE)
    {
    MCacheRemoveNode(N->Name);
    }

  memset(N,0,sizeof(mnode_t));

  /* node Name[0] set to value '\1' indicates a deleted node */        
 
  N->Name[0] = '\1';
  N->Name[1] = '\0';

  MSched.A[mxoNode]--;

  return(SUCCESS);
  }  /* END MNodeDestroy() */






/**
 *
 *
 * @param SpecName (I)
 * @param AdjustMode (I) [0=short,1=full]
 * @param Line (O) [should hold MMAX_NAME characters]
 */

char *MNodeAdjustName(
 
  char *SpecName,    /* I */
  int   AdjustMode,  /* I  (0=short,1=full) */
  char *Line)        /* O */
 
  {
  char        *ptr;

  if (Line == NULL)
    return(Line);

  *Line = '\0';
 
  switch (AdjustMode)
    {
    case 0:
    default:
 
      /* get short name */
 
      if ((ptr = strchr(SpecName,'.')) != NULL)
        {
        MUStrCpy(Line,SpecName,ptr - SpecName + 1);
 
        if (MSched.DefaultDomain[0] == '\0')
          MUStrCpy(MSched.DefaultDomain,ptr,sizeof(MSched.DefaultDomain)); 
 
        return(Line);
        }
      else
        {
        MUStrCpy(Line,SpecName,MMAX_NAME);
        return(Line);
        }
 
      /*NOTREACHED*/
 
      break;
 
    case 1:
 
      /* get long name */
 
      if ((ptr = strchr(SpecName,'.')) != NULL)
        {
        MUStrCpy(Line,SpecName,MMAX_NAME);
        return(Line);
        }
      else
        {
        if (MSched.DefaultDomain[0] != '\0')
          {
          if (MSched.DefaultDomain[0] == '.')
            {
            sprintf(Line,"%s%s",
              SpecName,
              MSched.DefaultDomain);
            }
          else
            {
            sprintf(Line,"%s.%s",
              SpecName,
              MSched.DefaultDomain);
            }
 
          return(Line);
          }
        else
          {
          MUStrCpy(Line,SpecName,MMAX_NAME);
          return(Line);
          }
        }
 
      /*NOTREACHED*/
 
      break;
    }  /* END switch (AdjustMode) */

  return(Line);
  }    /* MNodeAdjustName() */



/**
 * Set specified node state.
 *
 * @param N (I)
 * @param NState (I)
 * @param Validate (I)
 */

int MNodeSetState(

  mnode_t            *N,        /* I */
  enum MNodeStateEnum NState,   /* I */
  mbool_t             Validate) /* I */

  {
  const char *FName = "MNodeSetState";

  MDB(7,fSTRUCT) MLog("%s(%s,%s,%s)\n",
    FName,
    (N != NULL) ? N->Name : "NULL",
    MNodeState[NState],
    (Validate == TRUE) ? "TRUE" : "FALSE");

  if (N == NULL)
    {
    return(FAILURE);
    }

  /* allows flush to over-ride any other state */

  if (N->SpecState == mnsFlush)
    {
    NState = mnsFlush;
    }

  if (N->State != NState)
    {
    N->StateMTime = MSched.Time;
    N->MTime      = MSched.Time;

    switch (N->State)
      {
      case mnsDown:

        switch (NState)
          {
          case mnsIdle:
          case mnsActive:
          case mnsBusy:

            /* incorporate into standing reservations */

            /* NYI */

            break;

          default:

            /* NOT HANDLED */
  
            break;
          }  /* END switch (NState) */

        break;

      default:

        if (NState == mnsDown)
          {
          /* MOAB-3285.  PM doesn't want the trigger being fired on the DEFAULT node. */

          MOReportEvent((void *)N,N->Name,mxoNode,mttFailure,MSched.Time,TRUE);
          }

        break; 
      }    /* END switch (N->State) */
    }      /* END if (N->State != NState) */

  N->State  = NState;
  N->EState = NState;

  return(SUCCESS);
  }  /* END MNodeSetState() */





/**
 * Modify node attributes.
 *
 * NOTE: only support OS and State for now.
 *
 * @param SN (I) [optional,modified]
 * @param Ext (I) [optional]
 * @param AIndex (I)
 * @param Val (I)
 * @param EMsg (O) [optional]
 * @param SC (O) [optional]
 */

int MNodeModify(

  mnode_t                *SN,      /* I (optional,modified) */
  char                   *Ext,     /* I (optional) */
  enum MNodeAttrEnum      AIndex,  /* I */
  void                   *Val,     /* I */
  char                   *EMsg,    /* O (optional) */
  enum MStatusCodeEnum   *SC)      /* O (optional) */

  {
  int NIndex;

  mbool_t RMActionRequired = FALSE;

  marray_t NodeList;

  mnode_t *N;

  if ((SN == NULL) && (Ext == NULL))
    {
    return(FAILURE);
    }

  MUArrayListCreate(&NodeList,sizeof(mnode_t *),-1);

  if (Ext != NULL)
    {
    MUREToList(Ext,mxoNode,NULL,&NodeList,FALSE,NULL,0);
    }
  else
    {
    MUArrayListAppendPtr(&NodeList,SN);
    }

  if (NodeList.NumItems == 0)
    {
    MUArrayListFree(&NodeList);
    return(FAILURE);
    }

  switch (AIndex)
    {
    case mnaNodeState:
    case mnaOS:

      RMActionRequired = TRUE;

      break;

    default:

      MUArrayListFree(&NodeList);
      return(FAILURE);
    }  /* END switch (AIndex) */

  /* NOTE:  assume only one owner per attribute */

  for (NIndex = 0;NIndex < NodeList.NumItems;NIndex++)
    {
    N = (mnode_t *)MUArrayListGetPtr(&NodeList,NIndex);

    if (RMActionRequired == TRUE)
      {
      MRMNodeModify(N,NULL,NULL,NULL,AIndex,NULL,Val,TRUE,mSet,EMsg,SC);
      }    /* END if (RMActionRequired == TRUE) */

    /* NYI: also modify internally */
    }   /* END for (NIndex) */

  MUArrayListFree(&NodeList);
  return(SUCCESS);
  }  /* END MNodeModify() */

/* END MNodeCRUD.c */
