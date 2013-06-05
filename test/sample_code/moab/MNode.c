/* HEADER */

/**
 * @file MNode.c
 * 
 * Contains all node related functions that do NOT have unit tests.
 */

/* Contains:                                 *
 *  int MNodeCreate(NP,IsShared)             *
 *                                           */



#include <assert.h>

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/* private prototypes */

int __MNodeGetAvailTCGetLoad(const mnode_t *,const int [][5],int *,int);

/* END private prototypes */



/**
 * Get available taskcount for specified node.
 *
 * @param N (I) [optional]
 * @param Avl (I) Available resources--only used if Time <= now
 * @param Cfg (I) Configured resources--always used in calculating task count.
 * @param Ded (I) [optional] Only used if Time <= now
 * @param Req (I)
 * @param Time (I) The timeframe in which you wish to get the task count. It is either <= now or in the future.
 * @param MinTC (I)
 * @param RIndex (O) [optional]
 */

int MNodeGetTC(

  const mnode_t *N,
  const mcres_t *Avl,
  const mcres_t *Cfg,
  const mcres_t *Ded,
  const mcres_t *Req,
  long           Time,
  int            MinTC,
  enum MAllocRejEnum *RIndex)

  {
  int TC;
  int tmpTC = 0;
  int tmpRes;
 
  mbool_t UseUtil = FALSE;
  mbool_t UseDed  = FALSE;

  int ARes[mrLAST];
  int DRes[mrLAST];
  int CRes[mrLAST];  /* configured resources */
  int RRes[mrLAST];  /* requested resources */

  int rindex;

  enum MResourceAvailabilityPolicyEnum pval;

  const char *FName = "MNodeGetTC";

  MDB(8,fSCHED) MLog("%s(%s,%d,%d,%d,%d,%ld,%d,RIndex)\n",
    FName,
    (N != NULL) ? N->Name : "NULL",
    Avl->Procs,
    Cfg->Procs,
    (Ded != NULL) ? Ded->Procs : 0,
    Req->Procs,
    Time - MSched.Time,
    MinTC);

  if (RIndex != NULL)
    *RIndex = marNONE;

  if ((Avl == NULL) || (Cfg == NULL) || (Req == NULL))
    {
    return(0);
    }
  
  /* NOTE:  NULL node allowed */

  TC = MDEF_TASKS_PER_NODE;
 
  RRes[mrProc] = Req->Procs;
  CRes[mrProc] = Cfg->Procs;
  DRes[mrProc] = (Ded != NULL) ? Ded->Procs : 0;
  ARes[mrProc] = Avl->Procs;

  RRes[mrMem] = Req->Mem;
  CRes[mrMem] = Cfg->Mem;
  DRes[mrMem] = (Ded != NULL) ? Ded->Mem : 0;
  ARes[mrMem] = Avl->Mem;

  RRes[mrSwap] = Req->Swap;
  CRes[mrSwap] = Cfg->Swap;
  DRes[mrSwap] = (Ded != NULL) ? Ded->Swap : 0;
  ARes[mrSwap] = Avl->Swap;

  RRes[mrDisk] = Req->Disk;
  DRes[mrDisk] = (Ded != NULL) ? Ded->Disk : 0;

  if ((N != NULL) && (N->FSys != NULL))
    {
    int fsindex;

    ARes[mrDisk] = 0;
    CRes[mrDisk] = 0;

    /* if multiple filesystems specified, job only allowed to use single fs */

    for (fsindex = 0;fsindex < MMAX_FSYS;fsindex++)
      {
      if (N->FSys->Name[fsindex] == NULL)
        break;

      ARes[mrDisk] = MAX(ARes[mrDisk],N->FSys->ASize[fsindex]);
      CRes[mrDisk] = MAX(CRes[mrDisk],N->FSys->CSize[fsindex]);
      }  /* END for (fsindex) */
    }    /* END if (N->Sys != NULL) */
  else  
    {
    CRes[mrDisk] = Cfg->Disk;
    ARes[mrDisk] = Avl->Disk;
    }

  for (rindex = mrProc;rindex <= mrDisk;rindex++)
    {
    if (RRes[rindex] == 0)
      continue;

    if (RIndex != NULL)
      {
      switch (rindex)
        {
        case mrProc: *RIndex = marCPU; break;
        case mrMem:  *RIndex = marMemory; break;
        case mrSwap: *RIndex = marSwap; break;
        case mrDisk: *RIndex = marDisk; break;
        default: break;
        }
      }

    if (CRes[rindex] < RRes[rindex])
      {
      MDB(8,fSCHED) MLog("INFO:     inadequate %s (C:%d < R:%d)\n",
        MResourceType[rindex],
        CRes[rindex],
        RRes[rindex]);

      return(0);
      }

    /* incorporate availability policy information */

    /* determine effective specific/default policy */

    UseUtil = FALSE;
    UseDed  = FALSE;

    pval = mrapNONE;

    if ((N != NULL) && (N->NAvailPolicy != NULL))
      {
      pval = (N->NAvailPolicy[rindex] != mrapNONE) ?
        N->NAvailPolicy[rindex] : N->NAvailPolicy[0];
      }

    if (pval == mrapNONE)
      {
      pval = (MPar[0].NAvailPolicy[rindex] != mrapNONE) ?
        MPar[0].NAvailPolicy[rindex] : MPar[0].NAvailPolicy[0];
      }

    /* determine policy impact */

    if ((pval == mrapUtilized) || (pval == mrapCombined))
      {
      if (Time <= (long)MSched.Time)
        UseUtil = TRUE;
      }

    if ((pval == mrapDedicated) || (pval == mrapCombined))
      {
      if (Time <= (long)MSched.Time)
        UseDed = TRUE;
      }

    if (RRes[rindex] == -1)
      {
      if (CRes[rindex] <= 0)
        {
        MDB(8,fSCHED) MLog("INFO:     cannot dedicate %s (C:%d == 0)\n",
          MResourceType[rindex],
          CRes[rindex]);

        return(0);
        }

      if ((UseUtil == TRUE) && (ARes[rindex] != CRes[rindex]))
        {
        MDB(8,fSCHED) MLog("INFO:     cannot dedicate %s (A:%d != C:%d)\n",
          MResourceType[rindex],
          ARes[rindex],
          CRes[rindex]);

        return(0);
        }

      if ((UseDed == TRUE) && (DRes[rindex] > 0))
        {
        MDB(8,fSCHED) MLog("INFO:     cannot dedicate %s (D:%d != 0)\n",
          MResourceType[rindex],
          DRes[rindex]);

        return(0);
        }

      TC = MIN(1,CRes[rindex]);
      }
    else
      {
      tmpRes = CRes[rindex];

      if (UseUtil == TRUE)
        tmpRes = ARes[rindex];
      else
        tmpRes = CRes[rindex];
 
      if (UseDed == TRUE)
        tmpRes = MIN(tmpRes,CRes[rindex] - DRes[rindex]);

      TC = MIN(TC,tmpRes / RRes[rindex]);
      }  /* END else (RRes[rindex] == -1) */
 
    if (TC < MinTC)
      {
      MDB(9,fSCHED) MLog("INFO:     TC from %s: %d\n",
        MResourceType[rindex],
        TC);

      return(0);
      }
    }    /* END for (rindex) */

  /* calculate generic pval */
 
  UseUtil = FALSE;
  UseDed  = FALSE;

  pval = mrapNONE;

  if ((N != NULL) && (N->NAvailPolicy != NULL))
    {
    pval = N->NAvailPolicy[0];
    }

  if (pval == mrapNONE)
    {
    pval = MPar[0].NAvailPolicy[0];
    }

  /* determine policy impact */

  if ((pval == mrapUtilized) || (pval == mrapCombined))
    {
    if (Time <= (long)MSched.Time)
      UseUtil = TRUE;
    }

  if ((pval == mrapDedicated) || (pval == mrapCombined))
    {
    if (Time <= (long)MSched.Time)
      UseDed = TRUE;
    }
 
  /* is this code duplicated below? JOSH */
 
  if (UseUtil == TRUE)
    { 
    MSNLGetCount(0,&Req->GenericRes,&Avl->GenericRes,&tmpTC);
    }
  else
    {
    MSNLGetCount(0,&Req->GenericRes,&Cfg->GenericRes,&tmpTC);
    }

  TC = MIN(TC,tmpTC);

  if (TC < MinTC)
    {
    if (RIndex != NULL)
      *RIndex = marGRes;

    return(0);
    }

  if (!MSNLIsEmpty(&Req->GenericRes))
    {
    /* process dedicated generic resources */

    /* NOTE:  always check dedicated resources regardless of pval policy */

    UseDed = FALSE;

    if (Time <= (long)MSched.Time)
      UseDed = TRUE;

    if (UseDed == TRUE)
      {
      int     gindex;

      msnl_t tmpGRes;

      MSNLInit(&tmpGRes);

      for (gindex = 1;gindex <= MSched.GResCount;gindex++)
        {
        MSNLSetCount(&tmpGRes,gindex,MSNLGetIndexCount(&Cfg->GenericRes,gindex) - ((Ded != NULL) ? MSNLGetIndexCount(&Ded->GenericRes,gindex) : 0));
        }  /* END for (gindex) */

      MSNLGetCount(
        0,
        &Req->GenericRes,
        &tmpGRes,
        &tmpTC);

      TC = MIN(TC,tmpTC);

      MSNLFree(&tmpGRes);
      }  /* END if (UseDed == TRUE) */

    if (UseUtil == TRUE)
      {
      int     gindex;

      msnl_t tmpGRes;

      MSNLInit(&tmpGRes);

      for (gindex = 1;gindex <= MSched.GResCount;gindex++)
        {
        MSNLSetCount(&tmpGRes,gindex, ((Avl != NULL) ? MSNLGetIndexCount(&Avl->GenericRes,gindex) : 0));
        }  /* END for (gindex) */

      MSNLGetCount(
        0,
        &Req->GenericRes,
        &tmpGRes,
        &tmpTC);

      TC = MIN(TC,tmpTC);

      MSNLFree(&tmpGRes);
      }

    if (TC < MinTC)
      {
      /* inadequate generic resources */
  
      if (RIndex != NULL)
        *RIndex = marGRes;

      return(0);
      }
    }  /* END if (!MSNLIsEmpty(&Req->GenericRes)) */

  if (RIndex != NULL)
    *RIndex = marNONE;

  MDB(8,fSCHED) MLog("INFO:     %d tasks located\n",
    TC);
 
  return(TC);
  }  /* END MNodeGetTC() */






/**
 * Once per iteration update a node's attrlist based on non-job attrs.
 *
 * @param N
 */

void MNodeUpdateAttrList(

  mnode_t *N)

  {
  int aindex;

  mln_t *tmpL = NULL;

  for (tmpL = N->ParentVCs;tmpL != NULL;tmpL = tmpL->Next)
    {
    N->AttrList.insert(std::pair<std::string,std::string>("VC",tmpL->Name));
    }

  for (aindex = 1;MAList[meNFeature][aindex][0] != '\0';aindex++)
    {
    if (bmisset(&N->FBM,aindex))
      {
      N->AttrList.insert(std::pair<std::string,std::string>(MAList[meNFeature][aindex],""));
      }
    } 
  }  /* END MNodeUpdateAttrList() */





/**
 * Once per iteration update a node's attrlist.
 *
 * NOTE: this will look through active jobs, called AFTER jobs have been scheduled.
 *
 * @param N
 */

void MNodeUpdateJobAttrList(

  mnode_t *N)

  {
  char tmpLine[MMAX_LINE];

  mjob_t *J;

  int jindex;

  mstring_t Value(MMAX_LINE);

  std::set<std::string> Procs;
  std::set<std::string> Memory;
  std::set<std::string> OS;
  std::set<std::string> Vars;

  if (N == NULL)
    return;

  for (jindex = 0;jindex < N->MaxJCount;jindex++)
    {
    J = N->JList[jindex];

    if (J == NULL)
      break;

    if (!MJobPtrIsValid(J))
      continue;

    if (!MJOBISACTIVE(J))
      {
      continue;
      }

    /* grab the following from every active job:
          VMID (if the job is a vmtracking job)
          All job variables (do not mark them with "IsSetByUser")
          Requested processor/core count
          Requested memory
          Requested operating system
    */

    snprintf(tmpLine,sizeof(tmpLine),"%d",J->TotalProcCount);

    Procs.insert(tmpLine);
 
    if (J->Req[0]->DRes.Mem > 0)
      {
      snprintf(tmpLine,sizeof(tmpLine),"%d",J->Req[0]->DRes.Mem * J->Req[0]->TaskCount);

      Memory.insert(tmpLine);
      }

    if (J->Req[0]->Opsys != 0)
      {
      snprintf(tmpLine,sizeof(tmpLine),"%s",MAList[meOpsys][J->Req[0]->Opsys]);

      OS.insert(tmpLine);
      }

    Value.clear();  /* reset string */

    if ((MUHTToMString(&J->Variables,&Value) == SUCCESS) && (!Value.empty()))
      {
      Vars.insert(Value.c_str());
      }
    }  /* END for (jindex) */


  std::set<std::string>::iterator it;

  for (it = Procs.begin();it != Procs.end();it++)
    {
    N->AttrList.insert(std::pair<std::string,std::string>("Processors",*it));
    }

  for (it = Memory.begin();it != Memory.end();it++)
    {
    N->AttrList.insert(std::pair<std::string,std::string>("Memory",*it));
    }

  for (it = OS.begin();it != OS.end();it++)
    {
    N->AttrList.insert(std::pair<std::string,std::string>("OS",*it));
    }

  for (it = Vars.begin();it != Vars.end();it++)
    {
    N->AttrList.insert(std::pair<std::string,std::string>("Variables",*it));
    }
  }  /* END MNodeUpdateJobAttrList() */







/**
 * Checks to see if the given attribute is contained in list of attributes.
 *
 * Depending on the mode, the attribute is either set, added or just verifed.
 * <p>NOTE: for MAList[] attrs only 
 *
 * NOTE: if AttrValue doesn't exist and Mode is mVerify this will return -1
 * NOTE: returns the index if found
 *
 * @param AttrIndex (I)
 * @param AttrValue (I)
 * @param SearchMode (I) [mSet, mAdd, mVerify,mDecr]
 * @param AttrMap (I/O) [optional]
 */

int MUGetNodeFeature(

  const char             *AttrValue,  /* I */
  enum MObjectSetModeEnum SearchMode, /* I (mSet, mAdd, mVerify)  */
  mbitmap_t              *AttrMap)    /* I/O attr BM to evaluate (optional) */

  {
  int index;

  int len;

  const char *FName = "MUGetNodeFeature";

  MDB(9,fSTRUCT) MLog("%s(%s,%s)\n",
    FName,
    (AttrValue != NULL) ? AttrValue : "NULL",
    (SearchMode != mVerify) ? "ADD" : "VERIFY");

  if (((SearchMode == mClear) || (SearchMode == mSet)) && 
      (AttrMap != NULL))
    {
    /* clear attr map */

    bmclear(AttrMap);

    if (SearchMode == mClear)
      return(0);
    }

  if (AttrValue == NULL)
    {
    return(-1);
    }

  index = 0;

  /* determine if attr already set */

  len = sizeof(MAList[meNFeature][0]) - 1;

  for (index = 0;index < MMAX_ATTR;index++)
    {
    if (MAList[meNFeature][index][0] == '\0')
      break;

    if (!strncmp(MAList[meNFeature][index],AttrValue,len))
      {
      if (AttrMap != NULL)
        {
        if(SearchMode == mDecr || SearchMode == mUnset)
          bmunset(AttrMap,index);
        else 
          bmset(AttrMap,index);
        }

      return(index);
      }
    }    /* END for (index) */

  if (SearchMode == mVerify)
    {
    /* attribute cannot be found */

    return(-1);
    }

  if (index == MMAX_ATTR)
    {
    /* table is full */

    if (MSched.OFOID[mxoxNodeFeature] == NULL)
      MUStrDup(&MSched.OFOID[mxoxNodeFeature],AttrValue);

    return(-1);
    }

  if (AttrMap != NULL) 
    {
    /* add new value to table */

    MUStrCpy(MAList[meNFeature][index],AttrValue,sizeof(MAList[0][0]));

    bmset(AttrMap,index);

    MDB(5,fSTRUCT) MLog("INFO:     added MAList[%s][%d]: '%s'\n",
      MAttrType[meNFeature],
      index,
      AttrValue);
    }

  return(index);
  }  /* END MUGetNodeFeature() */





/**
 * Build BM for attrs listed in MAList[] 
 *
 * NOTE: this routine should only be called for things in MAList (ie. size MMAX_ATTR)
 *
 * @param AttrLine (I)
 * @param Mode (I)
 * @param AttrMap (O)
 */

int MUNodeFeaturesFromString(

  char                    *AttrLine,
  enum MObjectSetModeEnum  Mode,
  mbitmap_t               *AttrMap)    /* O */

  {
  char *ptr;
  char *TokPtr = NULL;

  char  Line[MMAX_LINE];

  /* FORMAT:  <ATTR>[{:|[] \t}<ATTR>]... */

  const char *FName = "MUNodeFeatureFromString";

  MDB(6,fCONFIG) MLog("%s('%s',%d,AttrMap,%d)\n",
    FName,
    AttrLine,
    Mode);

  if (AttrMap == NULL)
    {
    return(FAILURE);
    }

  bmclear(AttrMap);

  strcpy(Line,AttrLine);

  /* terminate at ';', '\n', '#' */

  ptr = MUStrTok(Line,";#\n",&TokPtr);

  ptr = MUStrTok(ptr," :[]\t|",&TokPtr);

  while (ptr != NULL)
    {
    if (MUGetNodeFeature(ptr,(enum MObjectSetModeEnum)Mode,AttrMap) == FAILURE)
      {
      return(FAILURE);
      }

    ptr = MUStrTok(NULL," :[]\t|",&TokPtr);
    }  /* END while (ptr != NULL) */

  MDB(5,fCONFIG) MLog("INFO:     parsed the following node features: '%s'\n",
    MUNodeFeaturesToString(',',AttrMap,Line));

  return(SUCCESS);
  }  /* END MUNodeFeaturesFromString() */





/**
 * Converts a mbitmap_t of node features to a string.
 *
 * NOTE: returns empty string on empty list.
 *
 * @param Delim (delimiter to use)
 * @param ValueMap (bitmap of features to print)
 * @param String (initialized and set)
 */


char *MUNodeFeaturesToString(

  char                 Delim,
  const mbitmap_t     *ValueMap,
  char                *String)

  {
  char *BPtr;
  int   BSpace;

  int   sindex;

  if (String != NULL)
    String[0] = '\0';

  if ((ValueMap == NULL) || (String == NULL))
    {
    return(FAILURE);
    }
 
  MUSNInit(&BPtr,&BSpace,String,MMAX_LINE);

  /* The Delim check is done outside of the for loop for speed */

  if (Delim != '\0')
    {
    for (sindex = 1;MAList[meNFeature][sindex][0] != '\0';sindex++)
      {
      if (bmisset(ValueMap,sindex))
        {
        if (!MUStrIsEmpty(String))
          {
          MUSNPrintF(&BPtr,&BSpace,"%c%s",
            Delim,
            MAList[meNFeature][sindex]);
          }
        else
          {
          MUSNCat(&BPtr,&BSpace,MAList[meNFeature][sindex]);
          }
        }
      }      /* END for (sindex) */
    } /* END if (Delim != '\0') */
  else
    {
    for (sindex = 1;MAList[meNFeature][sindex][0] != '\0';sindex++)
      {
      if (bmisset(ValueMap,sindex))
        {
        MUSNPrintF(&BPtr,&BSpace,"[%s]",
          MAList[meNFeature][sindex]);
        }
      }      /* END for (sindex) */
    }        /* END else */

  return(String);
  }  /* END MUNodeFeaturesToString() */






/**
 * Converts a mbitmap_t of node features to an mstring_t object.
 *
 * NOTE: returns empty string on empty list.
 *
 * @param Delim (delimiter to use)
 * @param ValueMap (bitmap of features to print)
 * @param String (initialized and set)
 */

int MUNodeFeaturesToString(

  char                 Delim,
  mbitmap_t           *ValueMap,
  mstring_t           *String)

  {
  int   sindex;

  if (String != NULL)
    MStringSet(String,"\0");

  if ((ValueMap == NULL) || (String == NULL))
    {
    return(FAILURE);
    }
 
  /* The Delim check is done outside of the for loop for speed */

  if (Delim != '\0')
    {
    for (sindex = 1;MAList[meNFeature][sindex][0] != '\0';sindex++)
      {
      if (bmisset(ValueMap,sindex))
        {
        if (!String->empty())
          {
          MStringAppendF(String,"%c%s",
            Delim,
            MAList[meNFeature][sindex]);
          }
        else
          {
          MStringAppend(String,
            MAList[meNFeature][sindex]);
          }
        }
      }      /* END for (sindex) */
    } /* END if (Delim != '\0') */
  else
    {
    for (sindex = 1;MAList[meNFeature][sindex][0] != '\0';sindex++)
      {
      if (bmisset(ValueMap,sindex))
        {
        MStringAppendF(String,"[%s]",
          MAList[meNFeature][sindex]);
        }
      }      /* END for (sindex) */
    }        /* END else */

  return(SUCCESS);
  }  /* END MUNodeFeaturesToString() */











/**
 * Adjust node's available procs, memory, swap, and disk and set
 * node state, estate accordingly.
 *
 * @see MJobUpdateResourceUsage() - peer - sets node's dedicated resources
 * @see MClusterUpdateNodeState() - parent
 * @see MNodePostUpdate() - peer - incorporate VM usage modifications to ARes
 *
 * @param N (I) [modified]
 * @param UProcs (I)
 * @param DProcs (I)
 * @param DTasks (I)
 */

int MNodeAdjustAvailResources(

  mnode_t *N,       /* I (modified) */
  double   UProcs,  /* I */
  int      DProcs,  /* I */
  int      DTasks)  /* I */

  {
  mbool_t UseUtil = FALSE;
  mbool_t UseDed  = FALSE;

  int  rindex;
  enum MResLimitTypeEnum rlindex;

  int  CRes[mrLAST];
  int  DRes[mrLAST];
  int  URes[mrLAST];
  int *ARes[mrLAST];

  int  BaseCRes;

  double MaxLoad;

  mxload_t *XLimit;

  mbool_t LoadExceeded;

  enum MResourceAvailabilityPolicyEnum PVal;
  
  const char *FName = "MNodeAdjustAvailResources";
 
  MDB(7,fSTRUCT) MLog("%s(%s,%f,%hd,%hd)\n",
    FName,
    (N != NULL) ? N->Name : "NULL",
    UProcs,
    DProcs,
    DTasks);
    

  if (N == NULL)
    {
    return(FAILURE);
    }

  /* determine processor availability */

  PVal = (MPar[0].NAvailPolicy[mrProc] != mrapNONE) ?
    MPar[0].NAvailPolicy[mrProc] :
    MPar[0].NAvailPolicy[mrNONE];

  if ((PVal == mrapUtilized) ||
      (PVal == mrapCombined)) 
    {
    UseUtil = TRUE;
    }
 
  if ((PVal == mrapDedicated) ||
      (PVal == mrapCombined))
    {
    UseDed = TRUE;
    }
  
  MDB(7,fRM) MLog("INFO:     determining proc availability for node %s w/ UseDed=%s, UseUtil=%s\n",
    N->Name,
    MBool[UseDed],
    MBool[UseUtil]);

  /* consider using MNSISUP() */

  if ((N->State != mnsIdle) &&
      (N->State != mnsActive) &&
      (N->State != mnsUnknown) &&
      (N->State != mnsUp))
    {
    N->ARes.Procs = 0;
    }
  else
    {
    int BaseCProcs;

    /* determine available processors */

    /* compare UProcs to BaseCProcs and DProcs to CProcs */

    BaseCProcs = MNodeGetBaseValue(N,mrlProc);

    if (UseUtil == TRUE)
      {
      if (UseDed == TRUE)
        {
#ifdef MNOT

        /* 2-5-10 BOC RT6829 - UProcs is cput from the job which is the 
         * "Maximum amount of CPU time used by all processes in the job." 
         * This number can be off if one node is working more than another 
         * and doesn't map directly to UProcs. It can make the node appear 
         * more utilized than what the actual load average is. Now the node 
         * is bounded at what the node's load actually is, and by the job. */

        N->ARes.Procs = (int)MAX(
          0,
          MIN(BaseCProcs - UProcs,N->CRes.Procs - DProcs) -
          (int)(MPar[0].UntrackedProcFactor *
          MAX(0.0,(N->Load - UProcs))));
#endif

        N->ARes.Procs = (int)MAX(
          0,
          MIN(N->CRes.Procs - DProcs,
              BaseCProcs - floor(N->Load + 0.5)));
        }
      else
        {
#ifdef MNOT

        /* 2-5-10 BOC RT6829 - UProcs is cput from the job which is the 
         * "Maximum amount of CPU time used by all processes in the job." 
         * This number can be off if one node is working more than another 
         * and doesn't map directly to UProcs. It can make the node appear 
         * more utilized than what the actual load average is. Now the node 
         * is bounded at what the node's load actually is, and by the job. */

        N->ARes.Procs = (int)MAX(
          0,
          BaseCProcs - UProcs -
          (int)(MPar[0].UntrackedProcFactor *
          MAX(0.0,(N->Load - UProcs))));
#endif

        N->ARes.Procs = (int)MAX(
          0,
          BaseCProcs - floor(N->Load + 0.5)); 
        }
      }
    else if (UseDed == TRUE)
      {
      /* use only dedicated proc information */

      N->ARes.Procs = N->CRes.Procs - DProcs;
      }
    else
      {
      if (N->ARes.Procs < 0)
        N->ARes.Procs = BaseCProcs;
      }
    }

  /* evaluate node task count */
 
  if ((UseDed == TRUE) &&
     (((N->State == mnsIdle) && (DProcs > 0)) ||
     ((N->State == mnsActive) && (DProcs >= N->CRes.Procs))))
    {
    MDB(4,fRM) MLog("ALERT:    node %s in RM state %s has %d:%d active tasks:procs\n",
      N->Name,
      MNodeState[N->State],
      DTasks,
      DProcs);
 
    if ((DProcs >= N->CRes.Procs) && (N->CRes.Procs > 0))
      {
      N->State  = mnsBusy; 
      N->EState = N->State;
      }
    else
      {
      N->State = mnsActive;
      }
    }

  if (N->DRes.Procs == -1)
    {
    N->DRes.Procs = DProcs;
    }
 
  if ((N->RM != NULL) && (N->RM->Type == mrmtLL) && (UseUtil == TRUE))
    {
    if ((N->State == mnsIdle) || (N->State == mnsActive))
      {
      int BaseCProcs;

      /* NOTE:  MIN_OS_CPU_OVERHEAD and MPar[0].UntrackedProcFactor do
                the same thing - consider removing this section in Q2 2007 */
 
      /* if load is an issue, consider setting NODEAVAILABILITYPOLICY or 
         UntrackedProcFactor */

      BaseCProcs = MNodeGetBaseValue(N,mrlProc);

      N->ARes.Procs = (int)MAX(
        0,
        MIN(BaseCProcs - N->Load + MIN_OS_CPU_OVERHEAD,
            N->CRes.Procs - N->DRes.Procs));
      }
    }

  LoadExceeded = FALSE;

  /* adjust node state by 'maxload' constraints */    

  if (N->MaxLoad > 0.01)
    MaxLoad = N->MaxLoad;
  else if (MSched.DefaultN.MaxLoad > 0.01)
    MaxLoad = MSched.DefaultN.MaxLoad;
  else
    MaxLoad = 0.0;
 
  if (MaxLoad > 0.01)
    {
    if (N->Load >= MaxLoad)
      {
      LoadExceeded = TRUE;
      }
    }    /* END if (MaxLoad > 0.01) */

  if (N->XLimit != NULL)
    XLimit = N->XLimit;
  else if (MSched.DefaultN.XLimit != NULL)
    XLimit = MSched.DefaultN.XLimit;
  else
    XLimit = NULL;

  if ((N->XLoad != NULL) && (XLimit != NULL))
    {
    /* check I/O load */

    /* NYI: check all gmetrics against Limits */
    }   /* END if ((N->XLoad != NULL) && (XLimit != NULL)) */

  if (LoadExceeded == TRUE)
    {
    if ((N->State == mnsIdle) || (N->State == mnsActive))
      {
      N->State = mnsBusy;

      /* NOTE:  do not call MNodeSetState() as it will set both State and EState
                NODEBUSYSTATEDELAYTIME will not take affect if State == EState 
                removed 8/20/07 by DRW */

      /*
      MNodeSetState(N,mnsBusy,FALSE);
      */
      }
    }    /* END if (LoadExceeded == TRUE) */

  /* adjust node state */
 
  if ((N->State == mnsUnknown) || (N->State == mnsUp))
    {
    if (N->ARes.Procs == MNodeGetBaseValue(N,mrlProc))
      N->State = mnsIdle;
    else if (N->ARes.Procs <= 0)
      N->State = mnsBusy;
    else
      N->State = mnsActive; 

    N->EState = N->State;
    }

  /* NOTE:  running indicates active workload on system regardless of 
            UseDed/UseUtl */

  if ((N->State == mnsIdle) && (DProcs > 0))
    N->State = mnsActive;

  /* adjust non-proc consumable resources */

  URes[mrMem] = N->URes.Mem;
  CRes[mrMem] = N->CRes.Mem;
  DRes[mrMem] = N->DRes.Mem;
  ARes[mrMem] = &N->ARes.Mem;
 
  URes[mrSwap] = N->URes.Swap;
  CRes[mrSwap] = N->CRes.Swap;
  DRes[mrSwap] = N->DRes.Swap;
  ARes[mrSwap] = &N->ARes.Swap;
 
  URes[mrDisk] = N->URes.Disk;
  CRes[mrDisk] = N->CRes.Disk;
  DRes[mrDisk] = N->DRes.Disk;
  ARes[mrDisk] = &N->ARes.Disk;

  for (rindex = mrMem;rindex <= mrDisk;rindex++)
    {
    if ((ARes[rindex] == NULL) || (*ARes[rindex] != -1))
      continue;

    /* determine effective specific/default policy */

    UseUtil = FALSE;
    UseDed  = FALSE;

    PVal = mrapNONE;

    if (N->NAvailPolicy != NULL)
      {
      PVal = (N->NAvailPolicy[rindex] != mrapNONE) ? 
        N->NAvailPolicy[rindex] : N->NAvailPolicy[0];
      }

    if (PVal == mrapNONE)
      {
      PVal = (MPar[0].NAvailPolicy[rindex] != mrapNONE) ?
        MPar[0].NAvailPolicy[rindex] : MPar[0].NAvailPolicy[0];
      }
 
    /* determine policy impact */
 
    if ((PVal == mrapUtilized) || (PVal == mrapCombined)) 
      {
      UseUtil = TRUE;
      }
 
    if ((PVal == mrapDedicated) || (PVal == mrapCombined))
      {
      UseDed = TRUE;
      }

    switch (rindex)
      {
      case mrMem:  rlindex = mrlMem; break;
      case mrSwap: rlindex = mrlSwap; break;
      case mrDisk: rlindex = mrlDisk; break;
      default:     rlindex = mrlNONE; break;
      }

    /* compare dres to cres and ures to basecres */
 
    BaseCRes = MNodeGetBaseValue(N,rlindex);

    if (UseUtil == TRUE)
      {
      if (UseDed == TRUE)
        *ARes[rindex] = MAX(0,MIN(BaseCRes - URes[rindex],CRes[rindex] - DRes[rindex]));
      else
        *ARes[rindex] = MAX(0,BaseCRes - URes[rindex]);
      }
    else if (UseDed == TRUE)
      {
      /* dedicated resources only */

      /* for dedicated only, use scaled cres */

      *ARes[rindex] = MAX(0,CRes[rindex] - DRes[rindex]);  
      }
    else
      {
      *ARes[rindex] = BaseCRes;
      }
    }  /* END for (rindex) */

  return(SUCCESS);
  }  /* END MNodeAdjustAvailResources() */



/**
 *
 *
 * @param R (I)
 * @param P (I)
 * @param PEPtr (O)
 */

int MResGetPE(

  mcres_t *R,      /* I */
  mpar_t  *P,      /* I */
  double  *PEPtr)  /* O */
 
  {
  double PE;
 
  PE = 0.0;

  if (P->CRes.Procs > 0)
    {
    PE = (double)R->Procs / P->CRes.Procs;
 
    if ((R->Mem > 0) && (P->CRes.Mem > 0))
      PE = MAX(PE,(double)R->Mem  / P->CRes.Mem);
 
    if ((R->Disk > 0) && (P->CRes.Disk > 0))
      PE = MAX(PE,(double)R->Disk / P->CRes.Disk);
 
    if ((R->Swap > 0) && (P->CRes.Swap > 0))
      PE = MAX(PE,(double)R->Swap / P->CRes.Swap);

    PE *= P->CRes.Procs;  
    }

  if (PEPtr != NULL)
    *PEPtr = PE;

  return(SUCCESS);
  }  /* END MResGetPE() */





/**
 * Verify node status.
 *
 * @see MSysMainLoop() - parent
 * @see MClusterUpdateNodeState() - peer - called before MSchedProcessJobs()
 *
 * NOTE:  enforce node purge time, update node expected state.
 * NOTE:  called AFTER MSchedProcessJobs()
 * NOTE:  N->IsNew cleared before this routine is called
 * NOTE:  updates N->AttrList each iteration
 *
 * @param NPtr (I) [optional]
 */

int MNodeCheckStatus(

  mnode_t *NPtr)  /* I (optional) */

  {
  int  nindex;

  long Delta;

  mnode_t *N;
  char     TString[MMAX_LINE];

  const char *FName = "MNodeCheckStatus";
 
  MDB(3,fSTRUCT) MLog("%s()\n",
    FName);
 
  MNodeTransition(&MSched.DefaultN);
   
  /* update EState */
  /* purge expired node data */

  for (nindex = 0;nindex < MSched.M[mxoNode];nindex++)
    {
    N = MNode[nindex];

    if ((N == NULL) || (N->Name[0] == '\0'))
      break;

    if ((NPtr != NULL) && (NPtr != N))
      continue;

    if (N->Name[0] == '\1')
      continue;
 
    MDB(7,fSTRUCT) MLog("INFO:     checking node '%s'\n",
      N->Name);

    Delta = (long)MSched.Time - N->ATime;

    MNodeUpdateJobAttrList(N);

    if ((Delta > MSched.NodeDownTime) && (MNODEISUP(N)))
      {
      MULToTString((long)MSched.Time - N->ATime,TString);

      MDB(2,fSTRUCT) MLog("WARNING:  node %s not detected in %s - marking node down\n",
        N->Name,
        TString);

      MNodeSetState(N,mnsDown,FALSE);
      }

    if (bmisset(&N->IFlags,mnifPurge))
      {
      MULToTString((long)MSched.Time - N->ATime,TString);

      MDB(2,fSTRUCT) MLog("WARNING:  node %s not detected in %s (%d iterations) - removing node\n",
        N->Name,
        TString,
        MSched.Iteration - N->MIteration);

      MNodeRemove(N,FALSE,mndpDrain,NULL);

      continue;
      }

    if (((MSched.NodePurgeIteration >= 0) && 
         (MSched.Iteration - N->MIteration > MSched.NodePurgeIteration)) ||
         (Delta > MSched.NodePurgeTime))
      {
      MULToTString((long)MSched.Time - N->ATime,TString);

      MDB(2,fSTRUCT) MLog("WARNING:  node %s not detected in %s (%d iterations) - removing node\n",
        N->Name,
        TString,
        MSched.Iteration - N->MIteration);
 
      MNodeRemove(N,FALSE,mndpHard,NULL);

      continue;
      }

    if (N->EState == mnsNONE)
      N->EState = N->State;

    if ((N->State != N->EState) &&
        (MSched.Time > N->SyncDeadLine))
      {
      char DString[MMAX_LINE];

      MULToDString(&MSched.Time,DString);

      MDB(2,fSTRUCT) MLog("ALERT:    node '%s' sync from expected state '%s' to state '%s' at %s",
        N->Name,
        MNodeState[N->EState],
        MNodeState[N->State],
        DString);
 
      N->EState = N->State;
      }

    /* NOTE:  keyboard/interactive policy must be enforced before jobs scheduled
              to prevent new jobs from running on keyboard-active nodes */
    }    /* END for (nindex) */

  if (MSched.VMStorageNodeThreshold > 0)
    MVMCheckStorage();

  return(SUCCESS);
  }  /* END MNodeCheckStatus() */



/**
 * Process node feature names to determine if they activate special side-affects.
 *
 * @see MNodeConfigShow() - peer - display NODECFG values
 *
 * @param N (I)
 * @param FString (I)
 */

int MNodeProcessFeature(
 
  mnode_t *N,         /* I */
  char    *FString)   /* I */
 
  {
  char *C;

  char *vptr;
  int   hlen;
  int   clen;

  mbool_t NumOnly;
 
  if ((N == NULL) || (FString == NULL))
    {
    return(FAILURE);
    }

  if (!strcmp(FString,NONE))
    {
    /* Clear features */

    bmclear(&N->FBM);

    return(SUCCESS);
    }

  /* FORMAT:  x | x$ */

  /* NOTE:  if '$' specified, only process if following char is digit */
 
  C = MSched.ProcSpeedFeatureHeader;

  hlen = strlen(C);

  if (C[hlen - 1] == '$')
    {
    NumOnly = TRUE;
    clen    = hlen - 1;
    }
  else
    {
    NumOnly = FALSE;
    clen    = hlen;
    }

  vptr = &FString[clen];

  if (C[0] != '\0')
    {
    if (!strncmp(FString,C,clen) && ((NumOnly == FALSE) || (isdigit(*vptr))))
      {
      N->ProcSpeed = strtol(vptr,NULL,10);
      }
    }
 
  C = MSched.NodeTypeFeatureHeader; 

  hlen = strlen(C);

  if (C[hlen - 1] == '$')
    {
    NumOnly = TRUE;
    clen    = hlen - 1;
    }
  else
    {
    NumOnly = FALSE;
    clen    = hlen;
    }

  vptr = &FString[clen];
 
  if (C[0] != '\0')
    {
    if (!strncmp(FString,C,clen) && ((NumOnly == FALSE) || (isdigit(*vptr))))
      {
      MUStrDup(&N->NodeType,vptr);
      }
    }
 
  C = MSched.PartitionFeatureHeader;

  hlen = strlen(C);

  if (C[hlen - 1] == '$')
    {
    NumOnly = TRUE;
    clen    = hlen - 1;
    }
  else
    {
    NumOnly = FALSE;
    clen    = hlen;
    }

  vptr = &FString[clen];

  if (C[0] != '\0')
    {
    if (!strncmp(FString,C,clen) && ((NumOnly == FALSE) || (isdigit(*vptr))))
      {
      mpar_t *P;

      if (MParAdd(vptr,&P) == SUCCESS)
        {
        N->PtIndex = P->Index;
        }
      else
        {
        MDB(6,fSTRUCT) MLog("ALERT:    cannot add partition '%s', ignoring partition header\n",
          vptr);
        }
      }
    }

  C = MSched.RackFeatureHeader;

  hlen = strlen(C);

  if (C[hlen - 1] == '$')
    {
    NumOnly = TRUE;
    clen    = hlen - 1;
    }
  else
    {
    NumOnly = FALSE;
    clen    = hlen;
    }

  vptr = &FString[clen];

  if (C[0] != '\0')
    {
    if (!strncmp(FString,C,clen) && ((NumOnly == FALSE) || (isdigit(*vptr))))
      {
      MNodeSetAttr(N,mnaRack,(void **)vptr,mdfString,mSet);
      }
    }

  C = MSched.SlotFeatureHeader;

  hlen = strlen(C);

  if (C[hlen - 1] == '$')
    {
    NumOnly = TRUE;
    clen    = hlen - 1;
    }
  else
    {
    NumOnly = FALSE;
    clen    = hlen;
    }

  vptr = &FString[clen];

  if (C[0] != '\0')
    {
    if (!strncmp(FString,C,clen) && ((NumOnly == FALSE) || (isdigit(*vptr))))
      {
      MNodeSetAttr(N,mnaSlot,(void **)vptr,mdfString,mSet);
      }
    }

  if (!strncmp(FString,"xm",2))
    {
    /* handle scheduler extension feature */
 
    vptr = &FString[strlen("xm")]; 
 
    if (!strncmp(vptr + 2,"ML",2))
      {
      /* max load */
 
      N->MaxLoad = strtod(vptr + 4,NULL);
      }
    }

  /* add feature to node */
 
  MUGetNodeFeature(FString,mAdd,&N->FBM);
 
  return(SUCCESS);
  }  /* END MNodeProcessFeature() */




/**
 * Report SUCCESS if specified node is allocated to any active job.
 *
 * @param N (I)
 *
 * @see MRMNodePreUpdate() - parent
 * @see MJobProcessRemoved() - parent
 * @see MJobProcessCompleted() - parent
 */

int MNodeCheckAllocation(
 
  mnode_t *N)  /* I */
 
  {
  int     jindex;
 
  mjob_t *J;

  const char *FName = "MNodeCheckAllocation";
 
  MDB(5,fRM) MLog("%s(%s)\n",
    FName,
    (N != NULL) ? N->Name : "NULL");
 
  if (N == NULL)
    {
    return(FAILURE);
    }

  /* NOTE:  far more efficient to search N->JList[] - switch to this model */

  if (N->JList != NULL)
    {
    for (jindex = 0;jindex < N->MaxJCount;jindex++)
      {
      J = N->JList[jindex];

      if (J == NULL)
        break;

      if (!MJobPtrIsValid(J))
        continue;

      if (MJOBISACTIVE(J))
        {
        return(SUCCESS);
        }
      }
    }   /* END if (N->JList != NULL) */

#ifdef MREMOVE 
  {
  int     nindex;

  for (jindex = 0;MAQ[jindex] != -1;jindex++)
    {
    J = MJob[MAQ[jindex]];

    if (J == NULL)
      continue;
 
    if ((J->State != mjsRunning) &&
        (J->State != mjsStarting))
      {
      continue;
      }
 
    for (nindex = 0;J->NodeList[nindex].N != NULL;nindex++)
      {
      if (J->NodeList[nindex].N == N)
        {
        return(SUCCESS);
        }
      }   /* END for (nindex) */
    }     /* END for (jindex) */
  }
#endif /* MREMOVE */
 
  return(FAILURE);
  }  /* END MNodeCheckAllocation() */





/**
 * Iterate over the MSched.AResInfoAvail table and free the entries still present there
 * Then free the actual container table
 *
 * FIXME:  ????? Why is this function in MNode.c ?????
 */

int MNodeFreeAResInfoAvailTable()

  {
  int nindex;

  for (nindex = 0;nindex < MSched.M[mxoNode];nindex++)
    {
    if ((MSched.AResInfoAvail != NULL) &&
        (MSched.AResInfoAvail[nindex] != NULL))
      MUFree((char **)&MSched.AResInfoAvail[nindex]);
 
    if (MNode[nindex] == NULL)
      continue;

    MNodeDestroy(&MNode[nindex]);

    MUFree((char **)&MNode[nindex]);
    }  /* END for (nindex) */

  if (MSched.AResInfoAvail != NULL)
    MUFree((char **)&MSched.AResInfoAvail);

  MUFree((char **)&MNode);

  return(SUCCESS);
  }  /* END MNodeFreeAResInfoAvailTable() */





/**
 * Determines when job will be able to run on given node
 *  (Partial feasibility -> checks OS and procs to see if job can run here)
 *
 * @param N (I)
 * @param J (I)
 * @param RQ (I)
 * @param ATime (O) [optional]
 */

int MNodeGetCfgTime(

  const mnode_t *N,
  const mjob_t  *J,
  const mreq_t  *RQ,
  mulong        *ATime)

  {
  mulong BestTime;

  const char *FName = "MNodeGetCfgTime";

  MDB(6,fSTRUCT) MLog("%s(%s,%s,%d,%s)\n",
    FName,
    (N != NULL) ? N->Name : "NULL",
    (J != NULL) ? J->Name : "NULL",
    (RQ != NULL) ? RQ->Index : -1,
    (ATime != NULL) ? "ATime" : "NULL");

  if ((N == NULL) || (J == NULL))
    {
    return(FAILURE);
    }

  BestTime = MSched.Time;

  if ((J->Credential.Q != NULL) &&
      (bmisset(&J->Credential.Q->Flags,mqfProvision)))
    {
    if (ATime != NULL)
      *ATime = BestTime;

    return(SUCCESS);
    }

  /* return success if node can be configured to meet needs of job J */

  /* set ATime to earliest epoch time by which node can be available */
  /* ATime should incorporate configuration needs of active and reserved workload */

  /* initially limit to OSList, Procs */

  /* NOTE:  do we need to track cfg requirements of each resource consumer? */
  /* NOTE:  is there a primary or high priority consumer which mandate node configuration? */

  /* NOTE:  for now, assume dedicated resource access */

  /* NYI */

  /* check OS */

  if ((RQ != NULL) &&
      (RQ->Opsys != 0))
    {
    int CurOS;
    mnodea_t *ptr = NULL;
    int NumTasks;

    if (MJOBISVMCREATEJOB(J))
      {
      CurOS = -1; /* There is no current OS with jobs that create VMs */
      }
    else if (MJOBREQUIRESVMS(J))
      {
      /* determine available VM resources matching the request opsys. If
       * the resources are sufficient, no delay is needed. otherwise, a delay is
       * needed */

      mcres_t ARes;
      mcres_t Needed;
      mcres_t Available;
      int nindex;
      mln_t *Link;

      memset(&Needed,0,sizeof(Needed));
      memset(&Available,0,sizeof(Available));
      MCResInit(&ARes);

      MCResPlus(&Needed,&RQ->DRes);
      NumTasks = 1;

      if (MNLFind(&RQ->NodeList,N,&nindex) == SUCCESS)
        {
        /* MSchedProcessJobs()
           - MQueueScheduleIJobs()
             - MJobSelectMNL()
               - MNodeSelectIdleTasks()
                 - MReqCheckNRes()
                   - MNodeGetCfgTime() 

           RQ->NodeList populated in MJobAllocMNL()
        */

        NumTasks = MNLGetTCAtIndex(&RQ->NodeList,nindex);
        }

      MCResTimes(&Needed,NumTasks);

      for (Link = N->VMList;Link != NULL;Link = Link->Next)
        {
        mvm_t *VM = (mvm_t *)Link->Ptr;

        if (VM->ActiveOS != RQ->Opsys)
          continue;

        MCResClear(&ARes);

        MCResPlus(&ARes,&VM->CRes);
        MCResMinus(&ARes,&VM->DRes);
        MCResPlus(&Available,&ARes);
        }  /* END for (Link) */

      MCResFree(&ARes);

      if (MCResCanOffer(&Available,&Needed,TRUE,NULL) == FALSE)
        CurOS = -1; /* need delay for provisioning */
      else
        CurOS = RQ->Opsys; /* no delay needed */
      }
    else
      {
      CurOS = N->ActiveOS;
      }

    if (RQ->Opsys != CurOS)
      {
      if (MNodeCanProvision(N,J,RQ->Opsys,NULL,&ptr) == FALSE)
        {
        /* resources do not match and cannot be provisioned at any time */

        MDB(6,fSTRUCT) MLog("INFO:    cannot provision node %s to opsys '%s' for job %s\n",
          N->Name,
          MAList[meOpsys][RQ->Opsys],
          J->Name);

        return(FAILURE);
        }

      if (MJOBISVMCREATEJOB(J) == FALSE)
        {
        /* If we are not creating a VM, there is a delay needed to provision
         * the node or VMs on it */

        if ((ptr != NULL) && (ptr->CfgTime > 0))
          BestTime = MAX(BestTime,MSched.Time + ptr->CfgTime);
        }
      }
    }    /* END if (RQ->Opsys != 0) */

  if (ATime != NULL)
    {
    *ATime = BestTime;
    }

  return(SUCCESS);
  }  /* END MNodeGetCfgTime() */




/**
 * Set attribute owner on specified node.
 *
 * @param N (I) [modified]
 * @param R (I)
 * @param AIndex (I)
 */

int MNodeSetAttrOwner(

  mnode_t            *N,      /* I (modified) */
  mrm_t              *R,      /* I */
  enum MNodeAttrEnum  AIndex) /* I */

  {
  int rmindex;

  if ((N == NULL) || (R == NULL))
    {
    return(FAILURE);
    }

  if ((N->RMList == NULL) &&
     ((N->RMList = (mrm_t **)MUCalloc(1,sizeof(mrm_t *) * (MMAX_RM + 1))) == NULL))
    {
    return(FAILURE);
    }

  for (rmindex = 0;rmindex < MMAX_RM;rmindex++)
    {
    if (N->RMList[rmindex] == NULL) 
      N->RMList[rmindex] = R;

    if (N->RMList[rmindex] != R)
      continue;

    bmset(&N->RMAOBM,AIndex);

    return(SUCCESS);
    }  /* END for (rmindex) */
  
  return(FAILURE);
  }  /* END MNodeSetAttrOwner() */





/**
 * Report if taskmap arguments are effectively identical.
 *
 * @param TM1 (I)
 * @param TM2 (I)
 */

mbool_t MTaskMapIsEquivalent(

  int *TM1,  /* I */
  int *TM2)  /* I */

  {
  int tmindex;
  int nindex;

  mbool_t TM1IsActive;
  mbool_t TM2IsActive;

  int *tmpN = NULL;

  if ((TM1 == NULL) || (TM2 == NULL))
    {
    return(FALSE);
    }

  tmpN = (int *)MUCalloc(1,sizeof(int) * MSched.M[mxoNode]);

  TM1IsActive = TRUE;
  TM2IsActive = TRUE;

  for (tmindex = 0;tmindex < MSched.JobMaxTaskCount;tmindex++)
    {
    if (TM1IsActive == TRUE)
      {
      if (TM1[tmindex] == -1)
        TM1IsActive = FALSE;
      else
        tmpN[TM1[tmindex]]++;
      }

    if (TM2IsActive == TRUE)
      {
      if (TM2[tmindex] == -1)
        TM2IsActive = FALSE;
      else
        tmpN[TM2[tmindex]]--;
      }
    }    /* END for (tmindex) */ 

  for (nindex = 0;nindex < MSched.M[mxoNode];nindex++)
    {
    if (tmpN[nindex] != 0)
      {
      /* mismatch detected */

      MUFree((char **)&tmpN);

      return(FALSE);
      }
    }    /* END for (nindex) */

  MUFree((char **)&tmpN);

  return(TRUE);
  }  /* END MTaskMapIsEquivalent() */





/**
 * Associate node with specified partition.
 *
 * @see MParAddNode() - child
 * @see MNodeSetAttr() - parent
 * @see MRMNodePostLoad() - parent
 *
 * NOTE:  will reallocate reservation tables if node is moved to shared partition
 *
 * @param N (I) [modified]
 * @param SP (I) [optional]
 * @param SPName (I) [optional,minsize=MMAX_NAME]
 */

int MNodeSetPartition(

  mnode_t    *N,
  mpar_t     *SP,
  const char *SPName)

  {
  char PName[MMAX_NAME];

  mpar_t *P;

  const char *FName = "MNodeSetPartition";

  MDB(3,fSTRUCT) MLog("%s(%s,%s)\n",
    FName,
    (N != NULL) ? N->Name : "NULL",
    (SPName != NULL) ? SPName : "NULL");

  if (N == NULL)
    {
    return(FAILURE);
    }

  P = NULL;
  PName[0] = '\0';

  if (SP != NULL)
    {
    P = SP;
    } 
  else if ((SPName == NULL) || (SPName[0] == '\0'))
    {
    strcpy(PName,MCONST_DEFAULTPARNAME);
    }
  else
    {
    strcpy(PName,SPName);
    }

  if ((P != NULL) && (N->PtIndex == P->Index))
    {
    /* no change */

    return(SUCCESS);
    }

  if ((N->PtIndex > 0) && !strcmp(MPar[N->PtIndex].Name,PName))
    {
    /* no change */

    return(SUCCESS);
    }

  if (P == NULL)
    {
    if (!strcmp(N->Name,MDEF_GNNAME))
      {
      strcpy(PName,MCONST_GLOBALPARNAME);
      
      MDB(6,fSTRUCT) MLog("INFO:     adding %s node to ALL partition\n",
        MDEF_GNNAME);
      }  

    if (MParAdd(PName,&P) != SUCCESS)
      {
      MDB(6,fSTRUCT) MLog("ALERT:    cannot add node '%s' to partition '%s' (insufficient partitions?)\n",
        (N != NULL) ? N->Name : "NULL",
        PName);

      return(FAILURE);
      }
    }

  if (N->RM == NULL)
    {
    /* NO-OP */
    }
  else if ((!bmisset(&MSched.Flags,mschedfAllowMultiCompute)) &&
           (P->RM != NULL) && 
           (P->RM != N->RM) && 
           bmisset(&P->RM->RTypes,mrmrtCompute))
    {
    /* avoid jobs spanning multiple resource managers */

    if (strcmp(P->Name,P->RM->Name))
      {
      strcat(PName,".");
      strcat(PName,P->RM->Name);
      }

    if (MParAdd(PName,&P) != SUCCESS)
      {
      MDB(6,fSTRUCT) MLog("ALERT:    cannot add node '%s' to partition '%s' (insufficient partitions?)\n",
          (N != NULL) ? N->Name : "NULL",
          PName);

      return(FAILURE);
      }
    }

  MParAddNode(P,N);

  return(SUCCESS);
  }  /* END MNodeSetPartition() */




/**
 * Determine effective node category.
 *
 * @see NodeCat
 * @param N (I)
 * @param StatP (I/O) [updated]
 */

int MNodeGetCat(

  mnode_t           *N,     /* I */
  mnust_t          **StatP) /* I/O (updated) */ 

  {
  enum MNodeCatEnum CIndex;

  enum MNodeCatEnum RCIndex;

  mnust_t *S;

  mrsv_t *R;
  mre_t  *RE;

  mbool_t DoLoop;

  if ((N == NULL) ||
      (StatP == NULL) ||
      (*StatP == NULL))
    {
    return(FAILURE);
    }

  S = *StatP;
   
  /* determine node's category */

  /* determine rsv specified category */

  RCIndex = mncNONE;

  if (N->RE != NULL)
    {
    DoLoop = TRUE;

    for (RE = N->RE;RE != NULL;MREGetNext(RE,&RE))
      {
      R = MREGetRsv(RE);

      /* NOTE:  previously used R->IsActive in comparison below but race 
                condition exists on first iteration because node cat is 
                calculated before R->IsActive is determined. */

      if (R->StartTime > MSched.Time)
        continue;

      if ((R->SubType == mrsvstNONE) || 
          (R->SubType == mrsvstStandingRsv))
        continue;

      switch (R->SubType)
        {
        case mrsvstAdmin_BatchFailure:

          RCIndex = mncDown_Batch;

          DoLoop = FALSE;

          break;

        case mrsvstAdmin_HWFailure:

          RCIndex = mncDown_HW;

          DoLoop = FALSE;

          break;

        case mrsvstAdmin_SWFailure:

          RCIndex = mncDown_SW;

          DoLoop = FALSE;

          break;

        case mrsvstAdmin_OtherFailure:

          RCIndex = mncDown_Other;

          DoLoop = FALSE;

          break;

        case mrsvstAdmin_HWMaintenance:

          RCIndex = mncMain_HW;

          DoLoop = FALSE;

          break;

        case mrsvstAdmin_SWMaintenance:

          RCIndex = mncMain_SW;

          DoLoop = FALSE;

          break;

        case mrsvstUserRsv:

          RCIndex = mncRsv_User;

          break;

        case mrsvstGridRsv:

          RCIndex = mncRsv_Grid;

          break;

        case mrsvstJob:

          /* NO-OP */

          break;

        default:

          if (RCIndex == mncNONE) 
            {
            if (MACLIsEmpty(R->ACL))
              {
              /* system rsv active */

              RCIndex = mncRsv_User;
              }
            else
              {
              /* cannot determine if this is policy rsv or user rsv */

              /* NOTE:  line below only works when enum MNodeCat and enum 
                        MRsvSubTypeEnum are synchronized */

              RCIndex = (enum MNodeCatEnum)R->SubType;
              }
            }    /* END if (RCIndex == mncNONE) */

          break;
        }  /* END switch (R->SubType) */

      if (DoLoop == FALSE)
        break;
      }  /* END for (rindex) */
    }    /* END if (N->R != NULL) */

  /* adjust category by state */

  switch (RCIndex)
    {
    case mncDown_Batch:    /* batch system is down on node */
    case mncDown_HW:       /* node system hardware failure prevents node usage */
    case mncDown_Net:      /* node is down due to network failure */
    case mncDown_Other:    /* node is down for unknown/other reason */
    case mncDown_Storage:  /* node is down due to storage failure */
    case mncDown_SW:       /* node system software failure prevents node usage */
    case mncRsv_SysBench:  /* node is reserved for system testing and benchmarking */
    case mncMain_HW:       /* node is reserved for scheduled HW system maintenance */
    case mncMain_SW:       /* node is reserved for scheduled HW system maintenance */
    case mncMain_Unsched:  /* node is reserved for unscheduled system maintenance */
    case mncOther:
    case mncRsv_Standing:

      /* these rsv categories always override */

      CIndex = RCIndex; 

      break;

    case mncNONE:

      /* rsv category not specified */

      if (MNODEISUP(N) == FALSE)
        {
        /* node is down */

        CIndex = mncDown_HW;
        }  /* END switch (RCIndex) */
      else if (MNODEISACTIVE(N) == TRUE)
        {
        CIndex = mncActive;
        }
      else
        {
        if (N->State == mnsIdle)
          {
          /* check if there is a non-active priority job reservation on the node */
             
          if ((MSched.TrackIdleJobPriority == TRUE) && 
              (N->RE != NULL) && 
              (N->RE->R != NULL) &&
              (N->RE->R->Type == mrtJob))
            {
            CIndex = mncRsv_Job;
            }
          else
            {
            CIndex = mncIdle;
            }
          }
        else
          CIndex = mncDown_HW;
        }

      break;

    case mncActive:
    case mncIdle:
    case mncRsv_PUser:  /* reserved by user */
    case mncRsv_User:   /* reserved for user */
    default:

      if (MNODEISUP(N) == FALSE)
        {
        /* node is down */

        CIndex = mncDown_HW;
        }  /* END switch (RCIndex) */
      else if (MNODEISACTIVE(N) == TRUE)
        {
        if (RCIndex == mncIdle)
          CIndex = mncActive;
        else 
          CIndex = RCIndex;
        }
      else
        {
        CIndex = RCIndex;
        }

      break;
    }  /* END switch (RCIndex) */

  S->Cat  = CIndex;
  S->RCat = RCIndex;

  return(SUCCESS);
  }  /* END MNodeGetCat() */


/**
 * Report effective node charge rate.
 *
 * @see MRsvGetCost() - parent
 *
 * @param N (I)
 */

double MNodeGetChargeRate(

  mnode_t *N)  /* I */

  {
  if (N == NULL)
    {
    return(FAILURE);
    }

  if (N->ChargeRate != 0.0)
    {
    return(N->ChargeRate);
    }
  else if (MSched.DefaultN.ChargeRate != 0.0)
    {
    return(MSched.DefaultN.ChargeRate);
    }

  /* no charge rate specified */

  return(1.0);
  }    /* END MNodeGetChargeRate() */





/**
 *
 *
 * @param N (I)
 * @param PR (I) [optional, if NULL this function just checks to see if N is in a grid sandbox]
 */

mbool_t MNodeIsInGridSandbox(

  mnode_t *N,  /* I */
  mrm_t   *PR) /* I (optional, if NULL this function just checks to see if N is in a grid sandbox) */

  {
  mrsv_t *R;
  mre_t  *RE;

  if ((N == NULL) || (N->RE == NULL))
    {
    return(FALSE);
    }

  for (RE = N->RE;RE != NULL;MREGetNext(RE,&RE))
    {
    R = MREGetRsv(RE);
    
    if (bmisset(&R->Flags,mrfAllowGrid))
      {
      if (R->StartTime > MSched.Time)
        {
        /* rsv still in future--node isn't "in" the sandbox yet */

        continue;
        }

      if (PR == NULL)
        {
        return(TRUE);
        }

      if (MACLCheckGridSandbox(
           R->ACL,
           NULL,
           PR,
           mVerify) == SUCCESS)
        {
        return(TRUE);
        }
      }
    }    /* END for (rindex) */

  return(FALSE);
  }  /* END MNodeIsInGridSandbox() */





/**
 * Create default triggers for a given node.
 * 
 * @see MNodeAdd() - parent
 *
 * This should only be called at node creation because it checks
 * to see if create and/or discover triggers should be fired.
 */

int MNodeSetDefaultTriggers(

  mnode_t *N)

  {
  mtrig_t *T;

  int tindex;

  if ((N == NULL) || (N->Name[0] == '\0'))
    {
    return(FAILURE);
    }

  if (N->Name[0] == '\1')
    {
    return(FAILURE);
    }

  if (N == MSched.GN)
    {
    return(SUCCESS);
    }

  if (MSched.DefaultN.T != NULL)
    {
    MTListCopy(MSched.DefaultN.T,&N->T,TRUE);
    }

  if (N->T == NULL)
    {
    return(SUCCESS);
    }

  for (tindex = 0; tindex < N->T->NumItems;tindex++)
    {
    T = (mtrig_t *)MUArrayListGetPtr(N->T,tindex);

    if (MTrigIsValid(T) == FALSE)
      continue;

    T->OType = mxoNode;

    MUStrDup(&T->OID,N->Name);

    MTrigInitialize(T);
    } /* END for (tindex) */

  /* report creation event for node */

  MOReportEvent((void *)N,N->Name,mxoNode,mttCreate,MSched.Time,TRUE);

  if (!bmisset(&N->IFlags,mnifLoadedFromCP))
    {
    MOReportEvent((void *)N,N->Name,mxoNode,mttDiscover,MSched.Time,TRUE);
    }

  return(SUCCESS);
  }  /* END MNodeSetDefaultTriggers() */





/**
 * Process node id to extract cluster, node, and rack information.
 *
 * @param N (I) [modified]
 * @param NodeID (I)
 */

int MNodeProcessName(

  mnode_t *N,      /* I (modified) */
  char    *NodeID) /* I */

  {
  int   flen;    /* length of format string */
  int   vcount;  /* number of variables extracted from node id */
  int   vmax;    /* number of variables specified within format string */
  int   nindex;  /* character index within node id */
  int   findex;  /* character index within format string */
  int   nlen;
  int   ival;

  const char *Format;
  char *tail;

  char  tmpName[MMAX_LINE];

  if ((N == NULL) || (NodeID == NULL))
    {
    return(FAILURE);
    }

  /* NOTE:  should add support for '*$3R$3S*' to process 'node-001013' (NYI) */

  if (MSched.NodeIDFormat != NULL)
    {
    Format = MSched.NodeIDFormat;
    }
  else
    {
    /* assume format is:  <PREFIX><NODEID><SUFFIX> */

    Format = "*$N*";
    }

  /* NOTE:  keywords C-cluster, N-node, R-rack, S-slot */

  MUStrCpy(tmpName,NodeID,sizeof(tmpName));

  flen = strlen(Format);

  vmax = 0;

  for (findex = 0;findex < flen;findex++)
    {
    if (Format[findex] == '$')
      vmax++;
    }  /* END for (findex) */

  /* process format string */

  vcount = 0;
  nindex = 0;

  nlen = strlen(tmpName);

  for (findex = 0;findex < flen;findex++)
    {
    if (vcount >= vmax)
      {
      /* all requested variables extracted */

      break;
      }

    if (Format[findex] == '*')
      {
      /* skip non-numeric characters */

      for (;nindex < nlen;nindex++)
        {
        if (isdigit(tmpName[nindex]))
          break;
        }

      continue;
      }

    if (Format[findex] == '$')
      {
      findex++;

      ival = (int)strtol(&tmpName[nindex],&tail,10);

      nindex += (int)(tail - &tmpName[nindex]);

      switch (Format[findex])
        {
        case 'c':  /* cluster index */
        case 'C':

          /* NYI */

          break;

        case 'n':  /* node index */
        case 'N':

          N->NodeIndex = ival;

          MSched.NodeIDCounter = MAX(MSched.NodeIDCounter,N->NodeIndex + 1);

          break;

        case 'r':  /* rack index */
        case 'R':

          N->RackIndex = ival;

          break;

        case 's':  /* slot index */
        case 'S':

          N->SlotIndex = ival;

          break;

        default:

          /* unexpected variable */

          break;
        }  /* END switch (Format[findex]) */

      vcount++;

      continue;
      }  /* END if (Format[findex] == '$') */

    if (Format[findex] != tmpName[nindex])
      {
      /* string does not match format */

      break;
      }

    nindex++;
    }  /* END for (findex) */

  return(SUCCESS);
  }  /* END MNodeProcessName() */




/* provisioning design */

/*
  MSysScheduleProvisioning()
    MJobScheduleDynamic()
    MUGenerateSystemJobs()
      MS3JobSubmit()

  ...

  MJobStart()
    MJobStartSystemJob()
      MTrigLaunchInternalAction()
        MNodeProvision()
*/

/**
 * Change node OS/software stack.
 *
 * @see MSysScheduleProvisioning()
 * @see MRMNodeModify() - child
 * @see MJobProvision() - deprecated?
 * @see MNodeModify() - sync?
 * @see MNodeModifyConfig() - deprecated?
 *
 * NOTE:  This routine will be called by system jobs spawned to initiate and 
 *        manage provisioning tasks.
 *
 * NOTE:  Will set N->NextOS to match OS.
 * 
 * @see MClusterUpdateNodeState() - will clear N->NextOS once provisioning is
 *        complete
 * @see MJobPreempt() - N->NextOS directly modified - why?
 *
 * @param SN (I) [modified] N->NextOS set
 * @param V (I) [optional,modified] V->NextOS set
 * @param Ext (I) [optional] nodelist
 * @param OS (I)
 * @param ModifyNode (I) modify external node attributes
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional] 
 */

int MNodeProvision(

  mnode_t    *SN,
  mvm_t      *V,
  char       *Ext,
  char       *OS,
  mbool_t     ModifyNode,
  char       *EMsg,
  int        *SC)

  {
  int oindex;
  int OSIndex;

  mnodea_t *OSList = NULL;  /**< potential operating systems */
  int CurOS;
  enum MPowerStateEnum CurPowerState;

  mnode_t  *N;

  const char *FName = "MNodeProvision";

  MDB(1,fCORE) MLog("%s(%s,%s,%s,%s,EMsg,SC)\n",
    FName,
    (SN != NULL) ? SN->Name : "NULL",
    (V != NULL) ? V->VMID : "",
    (OS != NULL) ? OS : "NULL",
    MBool[ModifyNode]);

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (SC != NULL)
    *SC = 0;

  if (Ext != NULL)
    {
    char *tail;

    if ((tail = strchr(Ext,',')) != NULL)
      *tail = '\0';

    if (MNodeFind(Ext,&N) == FAILURE)
      {
      if (tail != NULL)
        *tail = ',';

      return(FAILURE);
      }

    if (tail != NULL)
      *tail = ',';
    }
  else
    {
    N = SN;
    }

  if ((N == NULL) || (OS == NULL))
    {
    return(FAILURE);
    }

  if (V == NULL)
    {
    CurOS = N->ActiveOS;
    CurPowerState = N->PowerSelectState;
    }
  else
    {
    CurOS = V->ActiveOS;
    CurPowerState = V->PowerSelectState;
    }

  /* If MSched.ForceNodeReprovision is true, moab will force the reinstallation of the OS
     even if that same OS is currently installed.*/

  if (!MSched.ForceNodeReprovision)
    {
    if (!strcasecmp(OS,MAList[meOpsys][CurOS]) &&
        (CurPowerState != mpowOff))
      {
      /* node already has requested OS */
  
      if (EMsg != NULL)
        strcpy(EMsg,"requested OS already active on node");
  
      return(SUCCESS);
      }
    }

  OSIndex = MUMAGetIndex(meOpsys,OS,mVerify);

  if (OSIndex == 0)
    {
    /* unknown OS requested */

    if (EMsg != NULL)
      strcpy(EMsg,"unknown OS requested");

    return(FAILURE);
    }

  if (V != NULL)
    {
    if (V->OSList != NULL)
      OSList = V->OSList;
    else if (N->VMOSList != NULL)
      OSList = N->VMOSList;
    else if (MSched.DefaultN.VMOSList != NULL)
      OSList = MSched.DefaultN.VMOSList;
    }
  else if ((N != NULL) && (N->OSList != NULL))
    {
    OSList = N->OSList;
    }
  else 
    {
    OSList = MSched.DefaultN.OSList;
    }

  if (OSList == NULL)
    {
    if (EMsg != NULL)
      strcpy(EMsg,"node has no OS list and cannot be provisioned");

    return(FAILURE);
    }

  for (oindex = 0;oindex < MMAX_NODEOSTYPE;oindex++)
    {
    if (OSList[oindex].AIndex == OSIndex)
      break;

    if (OSList[oindex].AIndex <= 0)
      break;
    }  /* END for (oindex) */

  if (OSList[oindex].AIndex != OSIndex)
    {
    /* node/VM does not support requested OS */

    if (EMsg != NULL)
      strcpy(EMsg,"node does not support requested OS");

    return(FAILURE);
    }

  if (ModifyNode == FALSE)
    {
    /* NO-OP */
    }
  else if ((MSched.ProvRM != NULL) ||
          ((N->RM != NULL) && (N->RM->ND.URL[mrmNodeModify] != NULL)))
    {
    /* NOTE:  if set, MRMNodeModify() will utilize MSched.ProvRM to provision */

    if ((V == NULL) && (MSched.AggregateNodeActions == TRUE))
      {
      int tmpOS = MUMAGetIndex(meOpsys,OS,mAdd);

      MNLProvisionAddToBucket(NULL,(char *)Ext,tmpOS);
      }
    else if (MRMNodeModify(
         N,
         V,
         Ext,
         N->RM,
         mnaOS,
         NULL,
         (void *)OS,
         FALSE,
         mSet,
         EMsg,  /* O */
         NULL) == FAILURE)
      {
      return(FAILURE);
      }
    }
  else if (ModifyNode == TRUE)
    {
    /* no method available to provision node */

    if (EMsg != NULL)
      {
      snprintf(EMsg,MMAX_LINE,"cannot provision node %s%s%s - no methods available",
        N->Name,
        (V != NULL) ? ":" : "",
        (V != NULL) ? V->VMID : "");
      }

    return(FAILURE);
    }

  if (V != NULL)
    {
    V->NextOS = MUMAGetIndex(meOpsys,OS,mAdd);

    V->LastOSModRequestTime = MSched.Time;
    }
  else if (Ext != NULL)
    {
    char *ptr;
    char *TokPtr = NULL;
    char *NodeList = NULL;

    char tmpLine[MMAX_LINE];

    MUStrDup(&NodeList,Ext);

    ptr = MUStrTok(NodeList,",",&TokPtr);

    snprintf(tmpLine,sizeof(tmpLine),"node provisioned to %s",OS);

    while (ptr != NULL)
      {
      if (MNodeFind(ptr,&N) == SUCCESS)
        {
        N->PowerSelectState = mpowOn;

        N->NextOS = MUMAGetIndex(meOpsys,OS,mAdd);

        N->LastOSModRequestTime = MSched.Time;

        MOWriteEvent((void *)N,mxoNode,mrelNodeModify,tmpLine,MStat.eventfp,NULL);
        } 

      ptr = MUStrTok(NULL,",",&TokPtr);
      } 

    MUFree(&NodeList);
    }
  else
    {
    N->NextOS = MUMAGetIndex(meOpsys,OS,mAdd);

    N->LastOSModRequestTime = MSched.Time;
    }

  /* SUCCESS */

  MDB(1,fCORE) MLog("INFO:     set attribute '%s' to '%s' on node '%s'\n",
    MNodeAttr[mnaOS],
    OS,
    N->Name);

  return(SUCCESS);
  }  /* END MNodeProvision() */

/**
 * Reboot node.
 *
 * @see MNodeReset() - peer
 * 
 * NOTE:  reset single node or specified node list (NL) 
 *
 * @param N (I)
 * @param NL (I)
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MNodeReboot(

  mnode_t              *N,           /* I */
  mnl_t                *NL,          /* I */
  char                 *EMsg,        /* O (optional,minsize=MMAX_LINE) */
  enum MStatusCodeEnum *SC)          /* O (optional) */

  {
  mnode_t *tmpN;

  mnat_t *ND;

  char Response[MMAX_LINE];

  int rc;

  const char *FName = "MNodeReboot";

  MDB(1,fCORE) MLog("%s(%s,NL,EMsg,SC)\n",
    FName,
    (N != NULL) ? N->Name : "NULL");

  if (SC != NULL)
    *SC = mscNoError;

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (NL != NULL)
    MNLGetNodeAtIndex(NL,0,&tmpN);
  else
    tmpN = N;

  if (tmpN == NULL)
    {
    return(FAILURE);
    }

  if ((tmpN->RM != NULL) && (tmpN->RM->ND.Path[mrmXNodePower] != NULL))
    {
    ND = &tmpN->RM->ND;
    }
  else
    {
    MDB(1,fSTRUCT) MLog("WARNING:  cannot reset node %s, NODEPOWERURL not specified.\n",
      tmpN->Name);

    return(FAILURE);
    }

  mstring_t NodeList(MMAX_LINE);
  mstring_t tmpString(MMAX_LINE);

  if (NL != NULL)
    {
    MNLToMString(NL,TRUE,",",'\0',1,&NodeList);
    }

  /* execute script */

  MStringAppendF(&tmpString,"%s %s %s",
    ND->Path[mrmXNodePower],
    (!NodeList.empty()) ? NodeList.c_str() : N->Name,
    (NL != NULL) ? NodeList.c_str() : N->Name,
    "RESET");

  rc = MUReadPipe(tmpString.c_str(),NULL,Response,sizeof(Response),NULL);

  if (rc == FAILURE)
    {
    MDB(1,fSTRUCT) MLog("ALERT:    cannot read output of command '%s'\n",
      ND->Path[mrmXNodePower]);

    return(FAILURE);
    }

  if (strstr(Response,"ERROR") != NULL)
    {
    MDB(1,fSTRUCT) MLog("ALERT:    unable to set power to ON for node %s with command '%s'\n",
      tmpN->Name,
      ND->Path[mrmXNodePower]);

    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MNodeReboot() */





/**
 * Reset node - remove all RM's other than provisioning RM, clear node OS image, 
 * and power down node w/PowerSelectState=off
 *
 * NOTE:  reset single node or specified node list (NL) 
 *
 * @see MNodeReboot() - peer
 *
 * @param SpecN (I)
 * @param NL (I) - not supported
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MNodeReinitialize(

  mnode_t              *SpecN,
  mnl_t                *NL,
  char                 *EMsg,
  enum MStatusCodeEnum *SC)

  {
  mnode_t *N;
  char     tEMsg[MMAX_LINE];

  mbool_t  StateLess = FALSE;

  const char *FName = "MNodeReinitialize";

  MDB(1,fCORE) MLog("%s(%s,NL,EMsg,SC)\n",
    FName,
    (SpecN != NULL) ? SpecN->Name : "NULL");

  if (SC != NULL)
    *SC = mscNoError;

  if (EMsg != NULL)
    EMsg[0] = '\0';

  N = SpecN;

  if (N == NULL)
    {
    return(FAILURE);
    }
  
  /* turn the node off */

  if (MRMNodeSetPowerState(N,NL,NULL,mpowOff,tEMsg) == FAILURE)
    {
    if (EMsg != NULL)
      MUStrCpy(EMsg,tEMsg,MMAX_LINE);

    return(FAILURE);
    }

  /* remove it from Moab */

  if (MNodeRemove(N,TRUE,mndpHard,NULL) == FAILURE)
    {
    if (EMsg != NULL)
      MUStrCpy(EMsg,tEMsg,MMAX_LINE);

    return(FAILURE);
    }

  if (StateLess == FALSE)
    {
    /* reprovision node to 'baremetal' */
    }

  return(SUCCESS);
  }  /* END MNodeReinitialize() */





/**
 * Update node profiling statistics.
 *
 * @see MStatPAdjust() - parent
 *
 * @param SN () optional
 */

int MNodeUpdateProfileStats(

  mnode_t *SN) /* optional */

  {
  int nindex;

  mnode_t *N;

  mpar_t  *P = NULL;

  mnust_t *S;

  double TotalPWatts = 0.0;
  double TotalWatts  = 0.0;

  mbool_t GlobalUpdate = MSched.DefaultN.ProfilingEnabled;

  for (nindex = 0;nindex < MSched.M[mxoNode];nindex++)
    {
    N = MNode[nindex];

    if ((N == NULL) || (N->Name[0] == '\0'))
      break;

    if (N->Name[0] == '\1')
      continue;

    if ((SN != NULL) && (SN != N))
      continue;

    if ((N->ProfilingEnabled == MBNOTSET) && (GlobalUpdate != TRUE))
      continue;

    if (N->ProfilingEnabled == FALSE)
      continue;

    if ((N->Stat.IStat == NULL) ||
        (N->Stat.IStatCount <= 0))
      continue;

    /***************************/
    /* update total node stats */
    /***************************/

    P = &MPar[N->PtIndex];

    S = &N->Stat;

    MNodeGetCat(N,(mnust_t **)&N->Stat.IStat[N->Stat.IStatCount - 1]);

    N->Stat.Cat = ((mnust_t **)N->Stat.IStat)[N->Stat.IStatCount - 1]->Cat;

    S->AProc += N->ARes.Procs;
    S->CProc += N->CRes.Procs;
    S->DProc += N->DRes.Procs;

    /* pessimistic node state */

    if (!MNODEISUP(N) || MNSISUP(S->NState) || (S->NState == mnsNONE))
      {
      S->NState = N->State;
      }

    if (!MSNLIsEmpty(&N->CRes.GenericRes))
      {
      int gindex;

      for (gindex = 1;gindex < MSched.M[mxoxGRes];gindex++)
        {
        if (MGRes.Name[gindex][0] == '\0')
          break;

        if (MSNLGetIndexCount(&N->CRes.GenericRes,gindex) == 0)
          continue;

        MSNLSetCount(&S->CGRes,gindex,MSNLGetIndexCount(&S->CGRes,gindex) + MSNLGetIndexCount(&N->CRes.GenericRes,gindex));
        MSNLSetCount(&S->AGRes,gindex,MSNLGetIndexCount(&S->AGRes,gindex) + MSNLGetIndexCount(&N->ARes.GenericRes,gindex));
        MSNLSetCount(&S->DGRes,gindex,MSNLGetIndexCount(&S->DGRes,gindex) + MSNLGetIndexCount(&N->DRes.GenericRes,gindex));
        }
      }    /* END if (!MSNLIsEmpty(&N->CRes.GenericRes)) */

    if (N->XLoad != NULL)
      {
      int xindex;

      if (!MNODEISACTIVE(N))
        {
        if ((N->XLoad->GMetric[MSched.WattsGMetricIndex] != NULL) &&
            (N->XLoad->GMetric[MSched.WattsGMetricIndex]->IntervalLoad != MDEF_GMETRIC_VALUE))
          TotalWatts += N->XLoad->GMetric[MSched.WattsGMetricIndex]->IntervalLoad;

        if ((N->XLoad->GMetric[MSched.PWattsGMetricIndex] != NULL) &&
            (N->XLoad->GMetric[MSched.PWattsGMetricIndex]->IntervalLoad != MDEF_GMETRIC_VALUE))
          TotalPWatts += N->XLoad->GMetric[MSched.PWattsGMetricIndex]->IntervalLoad;
        }

      for (xindex = 1;xindex < MSched.M[mxoxGMetric];xindex++)
        {
        if ((N->XLoad->GMetric[xindex] != NULL) &&
            (N->XLoad->GMetric[xindex]->IntervalLoad != MDEF_GMETRIC_VALUE))
          {
          S->XLoad[xindex] += N->XLoad->GMetric[xindex]->IntervalLoad;
          }
        }
      }    /* END if (N->XLoad != NULL) */

    S->CPULoad += N->Load;

    S->IterationCount++;

    if (N->Stat.IStatCount == 0)
      continue;

    /*******************************/
    /* update Iteration node stats */
    /*******************************/

    S = (mnust_t *)N->Stat.IStat[N->Stat.IStatCount - 1];

    if (S == NULL)
      continue;

    S->AProc += N->ARes.Procs;
    S->CProc += N->CRes.Procs;
    S->DProc += N->DRes.Procs;

    /* pessimistic node state */

    if (!MNODEISUP(N) || MNSISUP(S->NState) || (S->NState == mnsNONE))
      {
      S->NState = N->State;
      }

    if (!MSNLIsEmpty(&N->CRes.GenericRes))
      {
      int gindex;

      for (gindex = 1;gindex < MSched.M[mxoxGRes];gindex++)
        {
        if (MGRes.Name[gindex][0] == '\0')
          break;

        MSNLSetCount(&S->CGRes,gindex,MSNLGetIndexCount(&S->CGRes,gindex) + MSNLGetIndexCount(&N->CRes.GenericRes,gindex));
        MSNLSetCount(&S->AGRes,gindex,MSNLGetIndexCount(&S->AGRes,gindex) + MSNLGetIndexCount(&N->ARes.GenericRes,gindex));
        MSNLSetCount(&S->DGRes,gindex,MSNLGetIndexCount(&S->DGRes,gindex) + MSNLGetIndexCount(&N->DRes.GenericRes,gindex));
        }
      }

    if (N->XLoad != NULL)
      {
      int xindex;

      for (xindex = 1;xindex < MSched.M[mxoxGMetric];xindex++)
        {
        if ((N->XLoad->GMetric[xindex] != NULL) &&
            (N->XLoad->GMetric[xindex]->IntervalLoad != MDEF_GMETRIC_VALUE))
          {
          S->XLoad[xindex] += N->XLoad->GMetric[xindex]->IntervalLoad;
          }
        }
      }

    S->CPULoad += N->Load;
    S->UMem    += N->CRes.Mem - N->ARes.Mem;
 
    if (N->Load > S->MaxCPULoad)
      S->MaxCPULoad = N->Load;

    if (N->Load < S->MinCPULoad)
      S->MinCPULoad = N->Load;

    if (N->CRes.Mem - N->ARes.Mem > S->MaxMem)
      S->MaxMem = N->CRes.Mem - N->ARes.Mem;

    if (N->CRes.Mem - N->ARes.Mem < S->MinMem)
      S->MinMem = N->CRes.Mem - N->ARes.Mem;

    S->IterationCount++;
    }  /* END for (nindex) */

  if ((SN == NULL) && (P != NULL))
    {
    /* updating all nodes, store totalwatts and totalpwatts */

    if (MSched.GP->S.XLoad != NULL)
      {
      if (TotalWatts != MDEF_GMETRIC_VALUE)
        MSched.GP->S.XLoad[MSched.WattsGMetricIndex] += TotalWatts;
      if (TotalPWatts != MDEF_GMETRIC_VALUE)
        MSched.GP->S.XLoad[MSched.PWattsGMetricIndex] += TotalPWatts;
      }

    if ((MSched.GP->S.IStatCount > 0) && (MSched.GP->S.IStat[MSched.GP->S.IStatCount] != NULL))
      {
      if (TotalWatts != MDEF_GMETRIC_VALUE)
        MSched.GP->S.IStat[MSched.GP->S.IStatCount]->XLoad[MSched.WattsGMetricIndex] += TotalWatts;
      if (TotalPWatts != MDEF_GMETRIC_VALUE)
        MSched.GP->S.IStat[MSched.GP->S.IStatCount]->XLoad[MSched.PWattsGMetricIndex] += TotalPWatts;
      }
    }

  return(SUCCESS);
  }  /* MNodeUpdateProfileStats() */



/**
 * Check node for keyboard/mouse activity and take appropriate actions.
 *
 * @see MClusterUpdateNodeState() - parent
 *
 * @param N (I) [optional]
 */

int MNodeCheckKbdActivity(

  mnode_t *N) /* I (optional) */

  {
  enum MNodeKbdDetectPolicyEnum KbdDetectPolicy;
  long    MinResumeKbdIdleTime;
  double  MinPreemptLoad;

  /* handle local keyboard/mouse activity */

  /* if local keyboard/mouse activity is detected, and preemptonkbd is true *
   * and minpreemptload is reached, mark node as drain and preempt active jobs 
     a list of all jobs suspended due to kbd activity will be kept */

  KbdDetectPolicy = (N->KbdDetectPolicy != mnkdpNONE) ? 
    N->KbdDetectPolicy : 
    MSched.DefaultN.KbdDetectPolicy;

  MinResumeKbdIdleTime = (N->MinResumeKbdIdleTime > 0) ?
    N->MinResumeKbdIdleTime :
    MSched.DefaultN.MinResumeKbdIdleTime;

  MinPreemptLoad = (N->MinPreemptLoad >= 0.0) ?
    N->MinPreemptLoad :
    MSched.DefaultN.MinPreemptLoad;

  if ((KbdDetectPolicy != mnkdpNONE) &&
      (N->KbdIdleTime <= MinResumeKbdIdleTime) &&
      (N->Load >= MinPreemptLoad))
    {
    /* kbd activity detected */

    if ((KbdDetectPolicy == mnkdpPreempt) &&
        (!bmisset(&N->IFlags,mnifKbdDrainActive)))
      {
      int      jindex;
      mjob_t  *tmpJ;

      char *BPtr;
      int   BSpace;

      /* suspend all active jobs */

      if (MSched.PreemptPolicy == mppSuspend)
        {
        if ((N->KbdSuspendedJobList == NULL)  &&
           ((N->KbdSuspendedJobList = (char *)MUCalloc(1,MMAX_BUFFER)) == NULL))
          {
          /* NOTE       must free memory on node destroy/node resume */

          /* cannot allocate memory for suspended job list */

          return(FAILURE);
          }

        MUSNInit(&BPtr,&BSpace,N->KbdSuspendedJobList,MMAX_BUFFER);
        }

      for (jindex = 0;jindex < MMAX_JOB_PER_NODE;jindex++)
        {
        if ((N->JList == NULL) || (N->JList[jindex] == NULL))
          break;

        if (N->JList[jindex] == (mjob_t *)1)
          continue;

        tmpJ = N->JList[jindex];

        if (MJobPreempt(
              tmpJ,
              NULL,
              N->JList,
              NULL,
              NULL,
              mppNONE,
              TRUE,
              NULL,
              NULL,
              NULL) == FAILURE)
          {
          MDB(3,fRM) MLog("WARNING:  could not preempt job '%s' due to kdb activity on node %s\n", 
            tmpJ->Name,
            N->Name);

          continue;
          }

        if (MSched.PreemptPolicy == mppSuspend)
          {
          if (N->KbdSuspendedJobList[0] == '\0')
            {
            MUSNPrintF(&BPtr,&BSpace,"%s",
              tmpJ->Name);
            }
          else
            {
            MUSNPrintF(&BPtr,&BSpace,",%s",
              tmpJ->Name);
            }
          }
        }    /* END for (jindex) */
      }      /* END if ((KbdDetectPolicy == mnkdpPreempt) && ...) */

    if ((KbdDetectPolicy == mnkdpDrain) || (KbdDetectPolicy == mnkdpPreempt))
      {
      /* set state to drained every iteration kdb thresholds are satisfied */

      enum MNodeStateEnum KBDState = mnsDrained;

      MDB(5,fRM) MLog("INFO:     setting node '%s' to state '%s' (idletime: %ld/load:  %.2f)\n",
        N->Name,
        MNodeState[KBDState],
        N->KbdIdleTime,
        N->Load);

      if ((N->State != mnsDown) && (MNodeSetState(N,KBDState,FALSE) == FAILURE))
        {
        MDB(3,fRM) MLog("WARNING:  could not set node state to '%s' (kdb activity detected)\n",
          MNodeState[KBDState]);
        }
      else
        {
        bmset(&N->IFlags,mnifKbdDrainActive);
        }
      }

    MDB(3,fRM) MLog("INFO:     kbd activity processed on node %s\n",
      N->Name);
    }  /* END if ((KbdDetectPolicy != mnkdpNONE) && ...) */

  if ((KbdDetectPolicy != mnkdpNONE) &&
      (N->KbdIdleTime > MinResumeKbdIdleTime) &&
      (bmisset(&N->IFlags,mnifKbdDrainActive)))
    {
    /* kbd no longer active */

    if ((KbdDetectPolicy == mnkdpDrain) || (KbdDetectPolicy == mnkdpPreempt))
      {
      /* NOTE:  system should set correct node state */

      bmunset(&N->IFlags,mnifKbdDrainActive);
      }

    if (KbdDetectPolicy == mnkdpPreempt)
      {
      /* resume jobs if required */

      if (N->KbdSuspendedJobList != NULL)
        {
        char *ptr;
        char *TokPtr;

        mjob_t *J;

        ptr = MUStrTok(N->KbdSuspendedJobList,",",&TokPtr);

        while (ptr != NULL)
          {
          if (MJobFind(ptr,&J,mjsmBasic) != SUCCESS)
            {
            MDB(5,fRM) MLog("INFO:     could not locate job %s to restart\n",
              ptr);
            }
          else
            {
            MJobResume(J,NULL,NULL);
            }

          ptr = MUStrTok(NULL,",",&TokPtr);
          }  /* END while (ptr != NULL) */
        }    /* END if ((N->KbdSuspendedJobList != NULL) */
      }      /* END if (KbdDetectPolicy == mnkdpPreempt) */
    }        /* END if ((KbdDetectPolicy != mnkdpNONE) && ...) */

  return(SUCCESS);
  }  /* END MNodeCheckKbdActivity() */



/**
 * Set generic metric to specified value on node.
 *
 * @param N (I) [modified]
 * @param GMIndex (I)
 * @param Value (I)
 */

int MNodeSetGMetric(

  mnode_t *N,       /* I (modified) */
  int      GMIndex, /* I */
  double   Value)   /* I */

  {
  if (N == NULL) 
    {
    return(FAILURE);
    }

  if ((GMIndex <= 0) || (GMIndex >= MSched.M[mxoxGMetric]))
    {
    return(FAILURE);
    }

  if (MNodeInitXLoad(N) == FAILURE)
    {
    /* cannot allocate memory */

    return(FAILURE);
    }

  if (Value == MDEF_GMETRIC_VALUE)
    return(FAILURE);

  N->XLoad->GMetric[GMIndex]->IntervalLoad = Value;
  N->XLoad->GMetric[GMIndex]->SampleCount++;

  return(SUCCESS);
  }  /* END MNodeSetGMetric() */


/**
 * Remove nodes in specified node list from multi-node list
 *
 * @param SrcMNL   (I) [modified]
 * @param RemoveNL (I)
 */

int MMNLRemoveNL(

  mnl_t **SrcMNL,   /* I (modified) */
  mnl_t  *RemoveNL) /* I */

  {
  int rqindex;
  int nindex;
  int nindex2;

  mnode_t *N;
  mnode_t *Remove;

  int TC;

  for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
    {
    if (MNLIsEmpty(SrcMNL[rqindex]))
      break;

    for (nindex = 0;MNLGetNodeAtIndex(SrcMNL[rqindex],nindex,&N) == SUCCESS;nindex++)
      {
      for (nindex2 = 0;MNLGetNodeAtIndex(RemoveNL,nindex2,&Remove) == SUCCESS;nindex2++)
        {
        if (Remove == N)
          {
          /* match located - multiple matches possible */

          /* assume one proc per task */

          if (MNLGetTCAtIndex(RemoveNL,nindex2) >= MNLGetTCAtIndex(SrcMNL[rqindex],nindex))
            {
            /* all node resources already consumed - remove node */

            MNLRemove(SrcMNL[rqindex],Remove);

            break;
            }

          /* reduce available node resources */

          TC = MNLGetTCAtIndex(SrcMNL[rqindex],nindex);

          TC -= MNLGetTCAtIndex(RemoveNL,nindex2);

          MNLSetTCAtIndex(SrcMNL[rqindex],nindex,TC);
          }
        }  /* END for (nindex2) */
      }    /* END for (nindex) */
    }      /* END for (rqindex) */

  return(SUCCESS);
  } /* END MMNLRemoveNL() */





/**
 * Set N->EState based on node state and job allocation.
 *
 * NOTE:  requires N->JList[] to be properly populated
 *
 * NOTE:  sets N->EState to idle if no jobs located and not associated with
 *        peer moab - no other action taken!
 *
 * @param N (I) [modified]
 */

int MNodeDetermineEState(

  mnode_t *N) /* I (modified) */

  {
  int jindex;

  mjob_t *J;

  /* take action if either N->State or N->EState are busy */

  if ((N == NULL) || 
     ((N->State != mnsBusy) && (N->EState != mnsBusy)))
    {
    return(SUCCESS);
    }

  /* NOTE:  N->JList[] can contain active and suspended jobs */

  for (jindex = 0;N->JList[jindex] != NULL;jindex++)
    {
    J = N->JList[jindex];

    if (J == (mjob_t *)1)
      continue;

    if (J->State == mjsSuspended)
      continue;

    break;
    }  /* END for (jindex) */

  if ((N->JList[jindex] == NULL) && 
     ((N->RM == NULL) || (N->RM->Type != mrmtMoab)))
    {
    /* job not located - only set estate to idle if not peer moab */

    /* NOTE:  peer moab can explicitly report local utilization */

    N->EState = mnsIdle;
    }

  return(SUCCESS);
  }  /* END MNodeDetermineEState() */



/**
 * Handle multi-compute (aka hybrid) environments.
 * 
 * @see __MNatNodeProcess() - parent
 * @see MPBSClusterQuery() - parent
 *
 * NOTE:  update N->RM, N->RMList, and N->RMState
 *
 */

int MNodeUpdateMultiComputeStatus(

  mnode_t *N,                 /* I (modified) */
  mrm_t   *R,                 /* I */
  char    *OSName,            /* I */
  mbool_t *ShouldUpdateAttr)  /* O */

  {
  int rmindex;

  if (ShouldUpdateAttr != NULL)
    *ShouldUpdateAttr = TRUE;

  if ((N == NULL) || (R == NULL))
    {
    return(FAILURE);
    }

  /* NOTE:  N->RMState[] not yet set for this RM for this iteration */

  if (!bmisset(&N->IFlags,mnifMultiComputeRM))
    {
    bmset(&N->IFlags,mnifMultiComputeRM);

    /* add OS to N->OSList on first iteration */

    if ((OSName != NULL) && (OSName[0] != '\0'))
      {
      /* add os supported by RM to node's OS list */

      MNodeSetAttr(N,mnaOSList,(void **)OSName,mdfString,mIncr);
      }
    }

  /* WARNING:  tricky code below!!! Add R to N->RMList if not present */
  for (rmindex = 0;N->RMList[rmindex] != R;rmindex++)
    {
    if (N->RMList[rmindex] == NULL)
      {
      MNodeAddRM(N,R);

      break;
      }
    }

  if ((N->RMState[N->RM->Index] != mnsDown) &&
      (N->RMState[N->RM->Index] != mnsNONE) &&
      (!bmisset(&MRM[N->RM->Index].RTypes,mrmrtProvisioning)))
    {
    /* multiple compute RM's reporting - node is currently attached
       to other RM and other RM is up */

    /* should apply default OS to node OSList - NYI */

    /* do not load/update info from this RM */

    if (ShouldUpdateAttr != NULL)
      *ShouldUpdateAttr = FALSE;

    return(SUCCESS);
    }

  if ((N->RMState[R->Index] == mnsNONE) ||
      (N->RMState[R->Index] == mnsDown))
    {
    /* if this RM reports node as down, do not switch,
       continue loading off of primary compute RM */

    /* NO-OP */
    }
  else
    {
    if ((N->RM == NULL) || (!MNSISUP(N->RMState[N->RM->Index]) 
        && MNSISUP(N->RMState[R->Index])))
      {
      /* node primary RM reports node is down but this RM reports node is up -
         switch primary RM to this RM */

      N->RM = R;

      /* NOTE:  need to run MRMNodePostLoad() after processing */

      /* This will clear all node attributes loaded by previous RM - preserve stats */

      bmset(&N->IFlags,mnifIsNew);
      }
    }

  return(SUCCESS);
  }  /* END MNodeUpdateMultiComputeStatus() */



int __MNodeCompareIndex(

  const void *Left,
  const void *Right)

  {
  mnalloc_old_t *LNode = (mnalloc_old_t *)Left;
  mnalloc_old_t *RNode = (mnalloc_old_t *)Right;

  if (LNode->N->NodeIndex < RNode->N->NodeIndex)
    {
    return (-1);
    }
  else if (LNode->N->NodeIndex == RNode->N->NodeIndex)
    {
    return(0);
    }

  return(1);
  }


/**
 * Lexicographically compare the names of 2 given nodes
 *
 * @param Left    The node on the left of the comparison.
 * @param Right   The node on the right of the comparison.
 *
 * Beware that strcmp("a1024","a513") will return < 0 because strcmp will
 * only look at the first 4 characters because a513 is only 4 characters
 * long. And 102 < 513. It's recommended that node names are padded to
 * the same length.
 *
 * @returns:      < 0 if Left->Name < Right->Name
 *                0 if Left->Name == Right->Name
 *                > 0 if Left->Name == Right->Name
 */

int __MNodeCompareLex(

  const void *Left,  /*I*/
  const void *Right) /*I*/
  {
  mnalloc_old_t *LNode = (mnalloc_old_t *)Left;
  mnalloc_old_t *RNode = (mnalloc_old_t *)Right;

  int rc = strcmp(LNode->N->Name,RNode->N->Name);

  return(rc);
  }  /* END __MNodeCompareLex() */





int MNodeCheckGMReq(

  const mnode_t *N,      /* I */
  const mln_t   *GMReq)  /* I */

  {
  const mln_t *gmptr;

  mgmreq_t *G;

  int   gmindex;

  if (N == NULL) 
    {
    return(FAILURE);
    }
 
  if (GMReq == NULL)
    {
    return(SUCCESS);
    }

  for (gmptr = GMReq;gmptr != NULL;gmptr = gmptr->Next)
    {
    G = (mgmreq_t *)gmptr->Ptr;

    if (G == NULL)
      continue;

    gmindex = G->GMIndex;

    if ((N->XLoad == NULL) ||
        (N->XLoad->GMetric[gmindex] == NULL))
      {
      MDB(5,fSCHED) MLog("INFO:     node does not support required generic metric (%s not found)\n",
        MGMetric.Name[G->GMIndex]);

      return(FAILURE);
      }

    if (N->XLoad->GMetric[gmindex]->SampleCount == 0)
      {
      /* This gmetric has not been reported on this node */

      MDB(5,fSCHED) MLog("INFO:     node has not had required generic metric reported\n");

      return(FAILURE);
      }

    if (G->GMVal != NULL)
      {
      double gmload;
      double tmpD;
      int    rc;

      /* verify value comparison matches */

      gmload = N->XLoad->GMetric[gmindex]->IntervalLoad;

      if (gmload == MDEF_GMETRIC_VALUE)
        {
        /* never match on the default value */

        return(FAILURE);
        }

      tmpD = strtod(G->GMVal,NULL);

      rc = MUDCompare(
            gmload,
            G->GMComp,
            tmpD);

      if (rc == 0)
        {
        /* gmetric value does not match */

        MDB(5,fSCHED) MLog("INFO:     node generic metric value does not meet requirements (%s value does not match)\n",
          MGMetric.Name[G->GMIndex]);

        return(FAILURE);
        }
      }    /* END if (G->GMVal != NULL) */
    }      /* END for (gptr) */

  return(SUCCESS);
  }  /* END MNodeCheckGMReq() */




/**
 * returns TRUE if MUOSListContains returns TRUE for either N->OSList or
 * MSched.DefaultN.OSList
 *
 * @see MUOSListContains() - child
 * @see MNodeIsValidVMOS() - peer
 *
 * @param N       (I) non-null
 * @param OSIndex (I) should be > 0
 * @param OSPtr   (O) optional,modified
 */

mbool_t MNodeIsValidOS(

  const mnode_t *N,
  int            OSIndex,
  mnodea_t     **OSPtr)

  {
  mnodea_t *OSes; 

  assert(N != NULL);

  OSes = (N->OSList == NULL) ? MSched.DefaultN.OSList : N->OSList;

  return(MUOSListContains(OSes,OSIndex,OSPtr));
  }  /* END MNodeIsValidOS() */





/**
 * returns TRUE if MUOSListContains returns TRUE for either N->VMOSList or
 * MSched.DefaultN.VMOSList
 *
 * DEPRECATED
 *
 * NOTE:  bad logic - assumes single stage provisioing - assume existing HV 
 *        installed is the only one that can provide VM's
 *
 * @see MUOSListContains() - child
 * @see MNodeCanProvisionVMOS() - peer
 *
 * @param N                  (I) non-null
 * @param OSProvisionAllowed (I)
 * @param OSIndex            (I) should be > 0
 * @param OSPtr              (O) optional,modified
 */

mbool_t MNodeIsValidVMOS(

  const mnode_t *N,
  mbool_t        OSProvisionAllowed,
  int            OSIndex,
  mnodea_t     **OSPtr)

  {
  assert(N != NULL);

  return(MNodeCanProvisionVMOS(N,OSProvisionAllowed,OSIndex,OSPtr));
  }  /* END MNodeIsValidVMOS() */





/**
 * returns TRUE if MUOSListContains returns TRUE for either N->VMOSList or
 * MSched.DefaultN.VMOSList
 *
 * NOTE:  assumes HV can provide list of VM OS's on any/all nodes 
 *
 * @see MUOSListContains() - child
 * @see MNodeCanProvision() - parent
 *
 * @param N                  (I)
 * @param OSProvisionAllowed (I)
 * @param VMOSIndex          (I) target VM OS (should be > 0)
 * @param VMOSPtr            (O) optional,modified
 */

mbool_t MNodeCanProvisionVMOS(

  const mnode_t  *N,
  mbool_t         OSProvisionAllowed,
  int             VMOSIndex,
  mnodea_t      **VMOSPtr)

  {
  mnodea_t *OSes = NULL;

  if ((N == NULL) || (VMOSIndex <= 0))
    {
    return(FALSE);
    }

  if (VMOSPtr != NULL)
    VMOSPtr = NULL;

  OSes = (N->VMOSList == NULL) ? MSched.DefaultN.VMOSList : N->VMOSList;

  if ((OSes != NULL) && (MUOSListContains(OSes,VMOSIndex,VMOSPtr) == TRUE))
    {
    /* current active OS can support requested VM image */

    return(TRUE);
    }

  if ((OSProvisionAllowed == TRUE) &&
      (MSched.TwoStageProvisioningEnabled == TRUE))
    {
    int osindex;

    for (osindex = 1;osindex < MMAX_ATTR;osindex++)
      {
      if (MAList[meOpsys][osindex][0] == '\0')
        {
        /* end of list reached */

        break;
        }

      if (&MSched.VMOSList[osindex] == NULL)
        {
        /* OS is not hypervisior - does not support VM's */

        continue;
        }

      if (MUHTGet(&MSched.VMOSList[osindex],MAList[meOpsys][VMOSIndex],NULL,NULL) == SUCCESS)
        {
        /* VM image supported by available node image */

        return(TRUE);
        }
      }
    }

  return(FALSE);
  }  /* END MNodeCanProvisionVMOS() */





/**
 * Report if specified node can provision required OS 
 *
 * @see MNodeCanProvisionVMOS() - child/peer
 *
 * @param N (I)
 * @param J (I)
 * @param OSIndex (I)
 * @param ProvTypeP (O) [optional]
 * @param OSPtr (O) [optional]
 */

mbool_t MNodeCanProvision(

  const mnode_t            *N,
  const mjob_t             *J,
  int                       OSIndex,
  enum MSystemJobTypeEnum  *ProvTypeP,
  mnodea_t                **OSPtr)

  {
  mbool_t CanProvision = FALSE;
  mbool_t VMProvision = FALSE;

  mbool_t VMJob;

  enum MSystemJobTypeEnum tmpProvType;

  mbool_t OSProvisionAllowed = ((J->Credential.Q != NULL) &&
    bmisset(&J->Credential.Q->Flags,mqfProvision));

  const char *FName = "MNodeCanProvision";

  MDB(7,fSCHED) MLog("%s(%s,%s,%s,ProvTypeP,OSPtr)\n",
    FName,
    (N != NULL) ? N->Name : "NULL",
    (J != NULL) ? J->Name : "NULL",
    (OSIndex > 0) ? MAList[meOpsys][OSIndex] : "NONE");

  if (N == NULL)
    {
    return(FALSE);
    }

  if (OSPtr != NULL)
    *OSPtr = NULL;

  if (ProvTypeP == NULL)
    ProvTypeP = &tmpProvType;

  *ProvTypeP = msjtNONE;

  if (OSIndex == 0)
    {
    return(FALSE);
    }

  VMJob = ((MJOBREQUIRESVMS(J) == TRUE) || (MJOBISVMCREATEJOB(J) == TRUE)) ? TRUE : FALSE;

  if (VMJob == TRUE)
    {
    /* validate VM image provisiong */

    CanProvision = MNodeIsValidVMOS(N,OSProvisionAllowed,OSIndex,OSPtr);

    VMProvision = (MUHTGet(
        &MSched.VMOSList[N->ActiveOS],
        MAList[meOpsys][OSIndex],
        NULL,
        NULL) == SUCCESS);
    }
  else
    {
    /* validate server image provisioning */

    CanProvision = MNodeIsValidOS(N,OSIndex,OSPtr);
    }

  if ((CanProvision == FALSE) && (MSched.TwoStageProvisioningEnabled == TRUE))
    {
    mbool_t CanProvision2;

    CanProvision = TRUE;
    *ProvTypeP = msjtOSProvision2;

    /* NOTE:  must determine HV required - NYI */

    CanProvision2 = MNodeCanProvisionVMOS(N,OSProvisionAllowed,OSIndex,OSPtr);

    if (CanProvision2 == FALSE)
      {
      MDB(7,fSCHED) MLog("INFO:    cannot provision (1/2 stage) node %s to OS %s for job %s%s\n",
        N->Name,
        MAList[meOpsys][OSIndex],
        J->Name,
        (OSProvisionAllowed == FALSE) ? " QOS 'provision' flag not set)" : "");
      }
    else
      {
      MDB(7,fSCHED) MLog("INFO:    two stage provision possible for node %s to OS %s for job %s\n",
        N->Name,
        MAList[meOpsys][OSIndex],
        J->Name);
      }

    return(CanProvision2);
    }
  else if (CanProvision == TRUE)
    {
    /* determine if provisioning is already happening, if it is then 
       just assume this node is ready for the job.  MAdaptiveJobStart()
       and MJobStartSystemJob() will handle things if the node isn't
       quite ready, we just need to know here if we're close */

    if (MNodeGetSysJob(N,msjtOSProvision,TRUE,NULL) == SUCCESS)
      {
      /* provisioning job is active, determine if node is being provisinioned 
         to correct OS */

      if ((VMJob == TRUE) &&
          (MUHTGet(&MSched.VMOSList[N->NextOS],MAList[meOpsys][OSIndex],NULL,NULL) == SUCCESS))
        {
        /* NO-OP */

        /* node is being provisioned to correct OS, no provisioning required */
        }
      else if ((VMJob == FALSE) &&
               (N->NextOS == OSIndex))
        {
        /* NO-OP */

        /* node is being provisioned to correct OS, no provisioning required */
        }
      else
        {
        *ProvTypeP = msjtOSProvision;
        }
      }
    else if (VMProvision == FALSE)
      {
      /* TODO: In the future, we may want this routine to figure out if
       * a VM creation is needed for the job to run and return msjtVMCreatee.
       * Doug said, however, not to worry about it for now.
       */

      *ProvTypeP = msjtOSProvision;
      }
    }    /* END else if (CanProvision == TRUE) */

  if (CanProvision == FALSE)
    {
    MDB(7,fSCHED) MLog("INFO:    cannot provision (1/2 stage) node %s to OS %s for job %s%s\n",
      N->Name,
      MAList[meOpsys][OSIndex],
      J->Name,
      (OSProvisionAllowed == FALSE) ? " QOS 'provision' flag not set)" : "");
    }
  else
    {
    MDB(7,fSCHED) MLog("INFO:    provision possible for node %s to OS %s for job %s\n",
      N->Name,
      MAList[meOpsys][OSIndex],
      J->Name);
    }

  return(CanProvision);
  }  /* END MNodeCanProvision() */




int MNodeGetOSCount(

  mnode_t *N)  /* I */

  {
  int osindex;

  mnodea_t *OSList;

  if (N == NULL)   
    {
    return(0);
    }

  OSList = ((N->OSList != NULL) ? N->OSList : MSched.DefaultN.OSList);

  if (OSList == NULL)
    {
    return(0);
    }

  for (osindex = 0;OSList[osindex].AIndex != 0;osindex++);

  return(osindex);
  }  /* END MNodeGetOSCount() */





/**
 * Updates operations which node is capable of supporting - ie, not only 
 * allowed to operate but also configure with appropriate URL's, etc
 *
 * NOTE:  see mnaOperations
 *
 * @see MNodeCheckStatus() - parent - called once when node is first loaded
 * @see MWikiNodeUpdateAttr() - parent - called when baremetal OS is modified 
 *        to update for VM operations
 * @param N (I) [modified]
 */

int MNodeUpdateOperations(

  mnode_t *N)  /* I (modify) */

  {
  mbool_t OSProvisionReady = FALSE;
  mbool_t VMMigrateReady = FALSE;
  mbool_t VMCreateReady = FALSE;

  mrm_t *R;

  int rmindex;

  if ((N == NULL) || (N->RMList == NULL))
    {
    return(FAILURE);
    }

  if ((N->VMList != NULL) || (bmisset(&N->IFlags,mnifCanCreateVM)))
    {
    VMCreateReady = TRUE;
    VMMigrateReady = TRUE;  /* NOTE: bad assumption - FIXME */
    }

  if (MNodeGetOSCount(N) > 1)
    {
    OSProvisionReady = TRUE;
    }

  for (rmindex = 0;N->RMList[rmindex] != NULL;rmindex++)
    {
    R = N->RMList[rmindex];

    if (R->ND.URL[mrmXNodePower] != NULL)
      {
      bmset(&N->IFlags,mnifCanModifyPower);
      }

    if ((OSProvisionReady == TRUE) &&
        (R->ND.URL[mrmNodeModify] != NULL))
      {
      bmset(&N->IFlags,mnifCanModifyOS);
      }

    if ((VMCreateReady == TRUE) &&
        (R->ND.URL[mrmResourceCreate] != NULL))
      {
      bmset(&N->IFlags,mnifCanCreateVM);
      }

    if (VMMigrateReady == TRUE)
      {
      if (R->ND.URL[mrmJobMigrate] != NULL)
        bmset(&N->IFlags,mnifCanMigrateVM);
      }
    }  /* END for (rmindex) */

  return(SUCCESS);
  }  /* END MNodeUpdateOperations() */





/**
 * NOTE:  add new OS if not found 
 *
 * NOTE:  no code to properly handle 'mSet' which removes no longer specified values - FIXME 
 *        (ie, oldvalue='x,y,z'  newvalue='x,y' or newvalue='')
 *
 * @see MNodeSetAttr() - parent
 */

int MNodeParseOSList(

  mnode_t   *N,    /* I (optional,notused) */
  mnodea_t **OLP,  /* O (alloc) */
  char      *Buf,  /* I */
  int        Mode, /* I */
  mbool_t    IsVM) /* I */
 
  {
  char *ptr;
  char *TokPtr = NULL;

  char *TokPtr2 = NULL;

  char *optr;
  char *dptr;

  int   oindex;

  char  tmpLine[MMAX_LINE];

  mnodea_t *OSList;

  if ((OLP == NULL) || (Buf == NULL))
    {
    return(FAILURE);
    }

  if ((*OLP == NULL) &&
     ((*OLP = (mnodea_t *)MUCalloc(1,sizeof(mnodea_t) * MMAX_NODEOSTYPE)) == NULL))
    {
    return(FAILURE);
    }

  OSList = *OLP;

  if (Mode == mIncr)
    {
    /* FORMAT:  <OS> */

    int tmpOS;

    tmpOS = MUMAGetIndex(meOpsys,Buf,mAdd);

    for (oindex = 0;OSList[oindex].AIndex > 0;oindex++)
      {
      if (tmpOS == OSList[oindex].AIndex)
        break;
      }  /* END for (oindex) */

    if ((OSList[oindex].AIndex == 0) && (oindex < MMAX_NODEOSTYPE))
      {
      OSList[oindex].AIndex = tmpOS;
      }
      
    return(SUCCESS);
    }  /* END if (Mode == mIncr) */

  /* FORMAT:  [<APPLICATION>@]<OS>[:<PROVDURATION>][,[<APPLICATION>@]<OS>[:<PROVDURATION>]... */

  ptr = MUStrTok(Buf,",:",&TokPtr);

  oindex = 0;

  while (ptr != NULL)
    {
    MUStrCpy(tmpLine,ptr,sizeof(tmpLine));

    dptr = NULL;

    if (strchr(tmpLine,'@') != NULL)
      {
      MUStrTok(tmpLine,"@",&TokPtr2);
      optr = MUStrTok(NULL,"@",&TokPtr2);
      }
    else
      {
      optr = tmpLine;
      }

    if (optr == NULL)
      { 
#if 0
      /* NOTE: how could this code ever work if optr is NULL? */

      /* record OS */

      if (strchr(optr,':') != NULL)
        {
        optr = MUStrTok(optr,":",&TokPtr2);
        dptr = MUStrTok(NULL,":",&TokPtr2);
        }

      OSList[oindex].AIndex = MUMAGetIndex(meOpsys,optr,mAdd);
#endif
      }
    else
      {
      OSList[oindex].AIndex = MUMAGetIndex(meOpsys,optr,mAdd);
      }

    if (dptr != NULL)
      {
      /* duration specified */

      OSList[oindex].CfgTime = MUTimeFromString(dptr);
      }

    if (IsVM == TRUE)
      {
      /* NOTE:  race condition - node OS must be reported before VM OS */

      MUHTAdd(&MSched.VMOSList[0],optr,NULL,NULL,NULL);

      if (N->ActiveOS > 0)
        MUHTAdd(&MSched.VMOSList[N->ActiveOS],optr,NULL,NULL,NULL);
      }
   
    oindex++;

    if (oindex >= MMAX_NODEOSTYPE - 1)
      break;

    ptr = MUStrTok(NULL,",:",&TokPtr);
    }  /* END while (ptr != NULL) */      

  /* terminate list */

  OSList[oindex].AIndex = 0;

  return(SUCCESS);
  }  /* END MNodeParseOSList() */




/**
 * Load IMAGECFG parameter values for specified image.
 *
 * @param IName (I)
 * @param Buf (I) [optional]
 */

int MImageLoadConfig(

  char    *IName, /* I */
  char    *Buf)   /* I (optional) */

  {
  char *ptr;
  char *head;

  char  Value[MMAX_BUFFER];
  char  IndexName[MMAX_NAME];

  const char *FName = "MImageLoadConfig";

  MDB(5,fSTRUCT) MLog("%s(%s,%s)\n",
    FName,
    (IName != NULL) ? IName : "NULL",
    (Buf != NULL) ? Buf : "NULL");

  /* load specified image config info */

  if (Buf != NULL)
    head = Buf;
  else
    head = MSched.ConfigBuffer;

  if (head == NULL)
    {
    return(FAILURE);
    }

  ptr = head;

  if (IName == NULL)
    IndexName[0] = '\0';
  else
    MUStrCpy(IndexName,IName,sizeof(IndexName));
 
  while (MCfgGetSVal(
           head,
           &ptr,
           MCredCfgParm[mxoxImage],
           IndexName,
           NULL,
           Value,
           sizeof(Value),
           0,
           NULL) != FAILURE)
    {
    /* node attribute located */

    /* FORMAT:  <ATTR>=<VALUE>[ <ATTR>=<VALUE>]... */

    if (MImageProcessConfig(IndexName,Value) == FAILURE)
      {
      return(FAILURE);
      } 
 
    if (IName == NULL)
      IndexName[0] = '\0';
    else
      MUStrCpy(IndexName,IName,sizeof(IndexName));
    }    /* END while (MCfgGetSVal() != FAILURE) */

  return(SUCCESS);
  }  /* END MImageLoadConfig() */



/**
 * Initialize a nodemap (affinity map) of MSched.M[mxoNode].
 *
 * NOTE: Map must be NULL, otherwise failure
 *
 * @param Map (I/O) alloced, CALLER MUST FREE!!
 */

int MNodeMapInit(

  char **Map)

  {
  if ((Map == NULL) || (*Map != NULL))
    {
    return(FAILURE);
    }

  *Map = (char *)MUCalloc(1,sizeof(char) * MSched.M[mxoNode]);

  if (*Map == NULL)
    return(FAILURE);

  return(SUCCESS);
  }  /* END MNodeMapInit() */



int MNodeMapCopy(

  char *Dst,
  char *Src)

  {
  if ((Dst == NULL) || (Src == NULL))
    return(FAILURE);

  memcpy(Dst,Src,sizeof(char) * MSched.M[mxoNode]);

  return(SUCCESS);
  }  /* END MNodeMapCopy() */




/**
 * Process image attributes specified via config file.
 *
 * @see MImageLoadConfig() - parent
 *
 * @param IName (I) [modified]
 * @param Value (I) [modified]
 */

int MImageProcessConfig(

  char    *IName,
  char    *Value)

  {
  char  *ptr;
  char  *TokPtr;

  char   ValBuf[MMAX_BUFFER];
  char   AttrArray[MMAX_NAME];

  int    aindex;

  int    tmpI;

  int    rc;

  mbool_t FailureDetected;

  const char *FName = "MImageProcessConfig";

  MDB(5,fSTRUCT) MLog("%s(%s,%s)\n",
    FName,
    (IName != NULL) ? IName : "NULL",
    (Value != NULL) ? Value : "NULL");

  if ((IName == NULL) || 
      (IName[0] == '\0') ||
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
          (const char **)MImageAttr,
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

      MDB(3,fSTRUCT) MLog("WARNING:  unknown attribute '%s' specified for image %s\n",
        ptr,
        (IName != NULL) ? IName : "NULL");

      MMBAdd(&MSched.MB,tmpLine,NULL,mmbtNONE,0,0,NULL);
 
      ptr = MUStrTokE(NULL," \t\n",&TokPtr);
 
      continue;
      }

    rc = SUCCESS;

    switch (aindex)
      {
      case miaOSList:

        {
        char *ptr2;
        char *TokPtr2;

        /* FORMAT:  <OS>[,<OS>]... */

        /* MImageSetAttr(IName,AIndex,(void **)ValBuf,mdfString,mSet); */

        ptr2 = MUStrTok(ValBuf,",",&TokPtr2);

        while (ptr2 != NULL)
          {
          MUHTAdd(&MSched.OSList,ptr2,NULL,NULL,NULL);

          ptr2 = MUStrTok(NULL,",",&TokPtr2);
          }
        }    /* END BLOCK (case miaOSList) */

        break;

      case miaReqMem:

        {
        int   OSIndex;  /* OS object index */

        OSIndex = MUMAGetIndex(meOpsys,(char *)IName,mAdd);

        MSched.VMOSSize[OSIndex] = MAX(0,(int)strtol(ValBuf,NULL,10));

        if (MSched.VMOSSize[0] <= 0)
          MSched.VMOSSize[0] = MSched.VMOSSize[OSIndex];
        }

        break;

      case miaVMOSList:

        {
        char *ptr2;
        char *TokPtr2;

        int   OSIndex;  /* OS object index */

        /* FORMAT:  <VMOS>[,<VMOS>]... */

        OSIndex = MUMAGetIndex(meOpsys,(char *)IName,mAdd);

        /* MImageSetAttr(IName,AIndex,(void **)ValBuf,mdfString,mSet); */

        ptr2 = MUStrTok(ValBuf,",",&TokPtr2);

        while (ptr2 != NULL)
          {
          /* associate VM image name with both global table and OS specific
             table */

          MUHTAdd(&MSched.VMOSList[0],ptr2,NULL,NULL,NULL);

          if (OSIndex > 0)
            MUHTAdd(&MSched.VMOSList[OSIndex],ptr2,NULL,NULL,NULL);

          ptr2 = MUStrTok(NULL,",",&TokPtr2);
          }
        }    /* END BLOCK (case miaVMOSList) */

        break;

      default:

        /* NOTSUPPORTED */

        rc = FAILURE;

        break;
      }  /* END switch (aindex) */

    if (rc == FAILURE)
      FailureDetected = TRUE;

    ptr = MUStrTokE(NULL," \t\n",&TokPtr);     
    }    /* END while (ptr != NULL) */

  if (FailureDetected == TRUE)
    {
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MImageProcessConfig() */





double MNodeGetOCFactor(

  mnode_t                *N,     /* I */
  enum MResLimitTypeEnum  RType) /* I */

  {
  if (bmisset(&N->IFlags,mnifIsVM))
    {
    /* POLICY:  do not overcommit VM resources */

    return(1.0);
    }

  if ((N->ResOvercommitFactor != NULL) && (N->ResOvercommitFactor[RType] > 0.0))
    {
    return(N->ResOvercommitFactor[RType]);
    }

  if (MSched.DefaultN.ResOvercommitFactor[RType] > 0.0)
    {
    return(MSched.DefaultN.ResOvercommitFactor[RType]);
    }

  return(1.0);
  }  /* END MNodeGetOCFactor() */



/**
 * Accessor for the OCThreshold for a given node
 * specified either by default or by the node itself.
 *
 * @see MNodeGetGmetricThreshold() - peer
 *
 * @param N     (I) 
 * @param RType (I) 
 */

double MNodeGetOCThreshold(

  mnode_t                *N,     /* I */
  enum MResLimitTypeEnum  RType) /* I */

  {

  if ((N != NULL) && (N->ResOvercommitThreshold != NULL) && (N->ResOvercommitThreshold[RType] > 0.0))
    {
    return(N->ResOvercommitThreshold[RType]);
    }

  if (MSched.DefaultN.ResOvercommitThreshold[RType] > 0.0)
    {
    return(MSched.DefaultN.ResOvercommitThreshold[RType]);
    }

    /* POLICY:  Default is that the threshold is unset (0.0)  */
  return(0.0);

  }  /* END MNodeGetOCThreshold() */



/**
 * Accessor for the GMetricThreshold for a given node
 * specified either by default or by the node itself.
 *
 * @see MNodeGetOCThreshold() - peer
 *
 * @param N       (I) 
 * @param GMIndex (I) 
 */

double MNodeGetGMetricThreshold(

  mnode_t                *N,       /* I */
  int                     GMIndex) /* I */

  {

  if ((N != NULL) && 
      (N->GMetricOvercommitThreshold != NULL) && 
      (N->GMetricOvercommitThreshold[GMIndex] != MDEF_GMETRIC_VALUE))
    {
    return(N->GMetricOvercommitThreshold[GMIndex]);
    }

  if (MSched.DefaultN.GMetricOvercommitThreshold[GMIndex] != MDEF_GMETRIC_VALUE)
    {
    return(MSched.DefaultN.GMetricOvercommitThreshold[GMIndex]);
    }

    /* POLICY:  Default is that the threshold is unset (MDEF_GMETRIC_VALUE)  */
  return(MDEF_GMETRIC_VALUE);

  }  /* END MNodeGetGMetricThreshold() */


/**
 *  Sets the base (non-over-committed value) when overcommit is specified.
 *   Will allocate the N->RealRes structure.  Use MNodeGetBaseValue to get
 *   the value.
 *
 * @param N     [I] (modified) - The node to set the value on
 * @param RType [I] - Resource type
 * @param Value [I] - Value to set as base value
 */

int MNodeCreateBaseValue(

  mnode_t *N,
  enum MResLimitTypeEnum RType,
  int                    Value)

  {
  if (N == NULL)
    return(FAILURE);

  if (MSched.ResOvercommitSpecified[RType] == FALSE)
    return(SUCCESS);

  if (N->RealRes == NULL)
    {
    N->RealRes = (mcres_t *)MUCalloc(1,sizeof(mcres_t));

    MCResInit(N->RealRes);
    }

  switch (RType)
    {
    case mrlDisk:

      N->RealRes->Disk = Value;

      break;

    case mrlProc:

      N->RealRes->Procs = Value;

      break;

    case mrlMem:

      N->RealRes->Mem = Value;

      break;

    case mrlSwap:

      N->RealRes->Swap = Value;

      break;

    default:

      return(FAILURE);
      break;
    }  /* END switch (RType) */

  return(SUCCESS);
  }  /* END MNodeCreateBaseValue() */


/**
 * Returns the non-overcommitted resource value
 *
 * Return value is NOT success/failure.  0 is used in any failure condition.
 *
 * @see MNodeGetOCValue() - peer
 * 
 * @param N (I)
 * @param RType (I)
 */

int MNodeGetBaseValue(

  const mnode_t          *N,
  enum MResLimitTypeEnum  RType)

  {
  const mcres_t *Res = NULL;

  if (N == NULL)
    return(0);

  if ((MSched.ResOvercommitSpecified[RType] == TRUE) &&
      (N->RealRes != NULL))
    {
    Res = N->RealRes;
    }
  else
    {
    Res = &N->CRes;
    }

  switch (RType)
    {
    case mrlDisk:

      return(Res->Disk);

      break;

    case mrlProc:

      return(Res->Procs);

      break;

    case mrlMem:

      return(Res->Mem);

      break;

    case mrlSwap:

      return(Res->Swap);

      break;

    default:

      break;
    }

  return(0);
  }  /* END MNodeGetBaseValue() */





/**
 * @see MNodeGetOVFactor() - peer
 * @see MNodeGetBaseValue() - peer
 */

int MNodeGetOCValue(

  mnode_t                *N,          /* I */
  int                     BaseValue,  /* I */
  enum MResLimitTypeEnum  RType)      /* I */

  {
  if (N == NULL)
    return(0);

  if ((RType != mrlProc) &&
      (RType != mrlDisk) &&
      (RType != mrlSwap) &&
      (RType != mrlMem))
    return(0);

  if (bmisset(&N->IFlags,mnifIsVM))
    {
    /* POLICY:  do not overcommit VM resources */

    return(BaseValue);
    }

  if ((N->ResOvercommitFactor != NULL) && (N->ResOvercommitFactor[RType] > 0.0))
    {
    return((int)(BaseValue * N->ResOvercommitFactor[RType]));
    }

  if (MSched.DefaultN.ResOvercommitFactor[RType] > 0.0)
    {
    return((int)(BaseValue * MSched.DefaultN.ResOvercommitFactor[RType]));
    }

  return(BaseValue);
  }  /* END MNodeGetOCValue() */



double MNodeGetOCValueD(

  mnode_t                *N,          /* I */
  double                  BaseValue,  /* I */
  enum MResLimitTypeEnum  RType)      /* I */

  {
  if (bmisset(&N->IFlags,mnifIsVM))
    {
    /* POLICY:  do not overcommit VM resources */

    return(BaseValue);
    }

  if ((N->ResOvercommitFactor != NULL) && (N->ResOvercommitFactor[RType] > 0.0))
    {
    return(BaseValue * N->ResOvercommitFactor[RType]);
    }

  if (MSched.DefaultN.ResOvercommitFactor[RType] > 0.0)
    {
    return(BaseValue * MSched.DefaultN.ResOvercommitFactor[RType]);
    }

  return(BaseValue);
  }  /* END MNodeGetOCValueD() */




/**
 * Apply DefaultN attributes to local node structures.
 *
 * @see MNodeApplyTemplates() - peer
 *
 * @param N (I) [modified]
 */

int MNodeApplyDefaults(

  mnode_t *N)

  {
  if (N == NULL)
    {
    return(FAILURE);
    }

  if ((N->CRes.Procs == 0) && (MSNLIsEmpty(&N->CRes.GenericRes)) && (MSched.DefaultN.CRes.Procs > 0))
    {
    /* apply only if node isn't a license node */

    N->CRes.Procs = MSched.DefaultN.CRes.Procs;
    }

  return(SUCCESS);
  }  /* END MNodeApplyDefaults() */





/**
 * Reports number of additional tasks specified node can support
 * 
 * @see MNodeGetEffectiveLoad() - peer 
 *
 * NOTE:  If on-demand provisioning will occur, special handling must
 *        occur (what?)
 *
 * WARNING: This function contains code that is also duplicated in MNodeGetEffectiveLoad()! Until
 * this code is put into its own function, make sure you update both places where the code exists!!!
 *
 * @param N (I)
 * @param J (I) [optional] Not currently used!
 * @param RQ (I)
 * @param AnticipateLoad (I) - if set, calc TC based on expected future load
 * @param TCP (O)
 * 
 * @return FAILURE is no tasks are available
 */

int MNodeGetAvailTC(

  const mnode_t *N,
  const mjob_t  *J,
  const mreq_t  *RQ,
  mbool_t        AnticipateLoad,
  int           *TCP)

  {
  int TC = 999999;

  int    EffARes;   /* what resources are effectively availabe (may be scaled by overcommit factor) */

  int rindex;

  /* Sync this with the enums in __MNodeGetAvailTCLoad() */
  enum { mrtNodeConfigured = 0, mrtNodeDedicated, mrtNodeAvailable, mrtRequested, mrtResourceType };

  /* With OVERCOMMIT on, NodeConfigured will be based on the overcommit ratio.  Available WILL NOT.
      This means that if you have a 4-proc job/vm on a node with 8 procs, and overcommit of 2,
      NodeConfigured will be 16 (8 * 2), but available will only be 4 (8 - 4).  So, when 
      calculating tasks, we need to use NodeConfigured - NodeDedicated, not NodeAvailable */

  /* Sync this with the args for __MNodeGetAvailTCLoad() */

  int ResVal[][5] = {
    {N->CRes.Procs,N->DRes.Procs,N->ARes.Procs,RQ->DRes.Procs,mrlProc},
    {N->CRes.Mem,N->DRes.Mem,N->ARes.Mem,RQ->DRes.Mem,mrlMem},
    {N->CRes.Swap,N->DRes.Swap,N->ARes.Swap,RQ->DRes.Swap,mrlSwap},
    {N->CRes.Disk,N->DRes.Disk,N->ARes.Disk,RQ->DRes.Disk,mrlDisk},
    {0,0,0,0,mrlNONE} };

  /* NOTE:  take into account overcommit factors to determine if specified 
            node can support one or more tasks of specified req */

  /* NOTE:  take into account max utilization thresholds */

  if (AnticipateLoad == TRUE)
    {
    /* adjust available resources by anticipated load */

    double tmpD;

    MNodeGetAnticipatedLoad(N,FALSE,-1.0,&tmpD);

    /* FIX: must fix for multidimensional utilization */

    ResVal[0][mrtNodeAvailable] = (int)((double)MNodeGetBaseValue(N,mrlProc) - tmpD);
    }

  if (TCP != NULL)
    *TCP = 0;

  if ((N == NULL) || (RQ == NULL))
    {
    return(FAILURE);
    }

  /* NOTE:  in overcommit eval for immediate start, the following must be  
            satisfied:

      RQ->DRes.X / OCFactor <= N->ARes.X
      RQ->DRes.X <= (N->CRes.X * OCFactor) - N->DRes.X

  */


  for (rindex = 0;ResVal[rindex][mrtResourceType] != mrlNONE;rindex++)
    {
    if (ResVal[rindex][mrtRequested] <= 0)
      {
      /* specified resource type not requested - ignore */

      continue;
      }

    EffARes = ResVal[rindex][mrtNodeConfigured] - ResVal[rindex][mrtNodeDedicated];

#ifdef MNOT /* don't subtract VM resources, they are already included in the the dedicated resources */
    if (N->VMList != NULL)
      {
      /* Subtract VM resources */

      mln_t *Link;

      for (Link = N->VMList;Link != NULL;Link = Link->Next)
        {
        mvm_t *VM = (mvm_t *)Link->Ptr;

        switch(ResVal[rindex][mrtResourceType])
          {
          case mrlProc:

            EffARes -= VM->CRes.Procs;

            break;

          case mrlMem:

            EffARes -= VM->CRes.Mem;

            break;

          case mrlSwap:

            EffARes -= VM->CRes.Swap;

            break;

          case mrlDisk:

            EffARes -= VM->CRes.Disk;

            break;

          default:

            break;
          }
        } /* END for (Link = N->VMList;Link != NULL;Link = Link->Next) */
      } /* END if (N->VMList != NULL) */
#endif /* MNOT */

    __MNodeGetAvailTCGetLoad(N,ResVal,&EffARes,rindex);

    /* RQ->DRes.X > (N->CRes.X * OCFactor) - N->DRes.X */

    if (ResVal[rindex][mrtRequested] > EffARes)
      {
      return(FAILURE);
      }

    /* find out how many tasks we have by choosing the lowest of:
     *   - what the effective available resources will allow us to have OR
     *   - what the dedicated resources will allow us to have */

    TC = MIN(TC,EffARes / ResVal[rindex][mrtRequested]);
    TC = MIN(TC,(ResVal[rindex][mrtNodeConfigured] - ResVal[rindex][mrtNodeDedicated]) / ResVal[rindex][mrtRequested]);
    }    /* END for (rindex) */

  if (TCP != NULL)
    *TCP = TC;

  return(SUCCESS);
  }  /* END MNodeGetAvailTC() */


/*
 * Changes the EffARes (effective available resources) reported in MNodeGetAvailTC according
 *  to current load on the node.  Currently only sets it for mrlProc.  Note that this function
 *  is always called, so DON'T set EffARes unless you actually mean to change it! (no 
 *  initialization, etc.)
 *
 * Parameters are just from MNodeGetAvailTC
 *
 * @param N       (I) - Node that we're checking
 * @param ResVal  (I) - Current resource info for the node (see comments in MNodeGetAvailTC)
 * @param EffARes (O) - Output based on load (if we set it)
 * @param rindex  (I) - Resource index (procs, mem, etc.) for ResVal
 */

int __MNodeGetAvailTCGetLoad(
  const mnode_t *N,
  const int      ResVal[][5],
  int           *EffARes,
  int            rindex)
  {
  enum { mrtNodeConfigured = 0, mrtNodeDedicated, mrtNodeAvailable, mrtRequested, mrtResourceType };

  MNodeGetBaseValue(N,(enum MResLimitTypeEnum)ResVal[rindex][mrtResourceType]);

  /* if N->URes > Load but less than Load + 1 */

  /* This is a NO-OP it checks that Available is greater than Load, but less 
      that Load + 1, essentially that Available is less than 1 above Load.  
      Then it takes BaseCRes - Load, which must be Available + (< 1).  It 
      then converts that to an int (EffARes), which will drop the (< 1) in 
      the conversion, and you get- EffARes.  Same thing you started with. */

  /*
  if ((ResVal[rindex][mrtResourceType] == mrlProc) &&
      (BaseCRes - ResVal[rindex][mrtNodeAvailable] > N->Load) &&
      (BaseCRes - ResVal[rindex][mrtNodeAvailable] < N->Load + 1.0))
    {*/
    /* address rounding error - only N->Load used but N->ARes.Procs rounds up */

/*    *EffARes = (double)BaseCRes - N->Load;
    }*/

  return(SUCCESS);
  }




/**
 * Report effective resource utilization load for specified node.
 * This function returns a double with a lower bound of 0. If a node is fully loaded, but
 * not overloaded, a number of 1.0 would be returned. This "effective load"
 * represents the load of the machine as a whole, and is NOT the sum of the VM's loads.
 *
 * WARNING: This function contains code that is also duplicated in MNodeGetAvailTC()! Until
 * this code is put into its own function, make sure you update both places where the code exists!!!
 *
 * @see MNodeGetAvailTC() - peer (sync)
 * @see MVMSchedulePolicyBasedMigration() - parent
 * @see MNodeGetPriority() - parent
 *
 * @param N (I)
 * @param RType (I) [optional]
 * @param AnticipateLoad (I) - if TRUE, apply expected future load associated
 *          with pending actions
 * @param EvalCPULoad (I) - if TRUE, use hypervisor/node's CPU load to adjust calculation
 * @param OverrideVMLoad (I) [optional] - If supplied, this load will be used instead of the load of all VM's
 * @param ELoadP (O)
 * @param MaxLoadP (O) [optional]
 */

int MNodeGetEffectiveLoad(

  mnode_t            *N,
  enum MAllocRejEnum  RType,
  mbool_t             AnticipateLoad,
  mbool_t             EvalCPULoad,
  double              OverrideVMLoad,
  double             *ELoadP,
  double             *MaxLoadP)

  {
  int BaseCRes;    /* true/unscaled configured resources */
  double MaxARes;  /* maximum available resources after accounting for resource usage targets */
  double EffARes;  /* <MaxARes> - <nodeutilization> */

  double Threshold=0.0; /* Overcommit Threshold for this node */

  mbool_t CPULoadIsLoadBased = FALSE;

  int rindex;

  enum MResLimitTypeEnum rlindex;

  enum { mrtNCRes = 0, mrtNDRes, mrtNARes, mrtRQDRes, mrtRType };

  int ResVal[][5] = {
    {N->CRes.Procs,N->DRes.Procs,N->ARes.Procs,0,mrlProc},
    {N->CRes.Mem,N->DRes.Mem,N->ARes.Mem,0,mrlMem},
    {N->CRes.Swap,N->DRes.Swap,N->ARes.Swap,0,mrlSwap},
    {N->CRes.Disk,N->DRes.Disk,N->ARes.Disk,0,mrlDisk},
    {0,0,0,0,marNONE} };

  if (ELoadP != NULL)
    {
    /* indicate server is overloaded */

    *ELoadP = 0.0;
    }

  if (N == NULL)
    {
    return(FAILURE);
    }

  if (AnticipateLoad == TRUE)
    {
    double tmpD;

    MNodeGetAnticipatedLoad(N,FALSE,OverrideVMLoad,&tmpD);

    /* NOTE:  only enabled for proc utilization - must fix for multidimensional utilization (FIXME) */

    ResVal[0][mrtNARes] = (int)((double)MNodeGetBaseValue(N,mrlProc) - tmpD);
    }

  /* report value:  0.0 (no load) to 1.0 (full load) > 1.0 indicates server is
     overcommitted */

  /* loop through all resource types and discover if any of them are overcommitted */
  /* the resource that is MOST overcommited will be the one to set the effective load */

  for (rindex = 0;ResVal[rindex][mrtRType] != mrlNONE;rindex++)
    {
    if (RType != marNONE)
      {
      switch (RType)
        {
        case marCPU:     if (ResVal[rindex][mrtRType] != mrlProc) continue; break;
        case marMemory:  if (ResVal[rindex][mrtRType] != mrlMem) continue; break;
        case marSwap:    if (ResVal[rindex][mrtRType] != mrlSwap) continue; break;
        case marDisk:    if (ResVal[rindex][mrtRType] != mrlDisk) continue; break;
        default: continue; break;
        }
      }

    /* N->DRes.X > N->CRes.X */

    if (ResVal[rindex][mrtNDRes] > ResVal[rindex][mrtNCRes])
      {
      /* dedicated resources are overcommitted */

      if (ELoadP != NULL)
        {
        /* indicate server is overloaded */
        /* why are we doing 2.0 * NDRes???? (JMB) */

        *ELoadP = MAX(*ELoadP,(double)(1.0 + (2.0 * ResVal[rindex][mrtNDRes]) / ResVal[rindex][mrtNCRes]));
        }

      continue;
      }

    /* Validation of Overcommited Resources */

    /* CRes (BaseCRes*OCFactor) +++++++++++++++++++++++X++++++++++++++++++++++++++++++
       BaseCRes                                        ^
       ARes                                      ^
       MigrationThreshold (Threshold*CRes.X)        ^

       -------------------------------------------------------------
       EffectiveARes = ARes - BaseCRes * (1 - Threshold)
    */

    rlindex = (enum MResLimitTypeEnum) ResVal[rindex][mrtRType];

    BaseCRes = MNodeGetBaseValue(N,rlindex);

#ifdef MNOT
    /* what is the below if-statement code supposed to be doing!? */
    /* It messes stuff up and allows migrations to occur when they shouldn't--removing (JMB) */

    if ((rlindex == mrlProc) &&
        (BaseCRes - ResVal[rindex][mrtNARes] > N->Load) &&
        (BaseCRes - ResVal[rindex][mrtNARes] < N->Load + 2.0))
      {
      /* if load is close to matching aprocs - do not scale up resource availability */

      CPULoadIsLoadBased = TRUE;
      }
#endif /* MNOT */

    /* if N->URes > Load but less than Load + 1 */

    if ((rlindex == mrlProc) &&
        (BaseCRes - ResVal[rindex][mrtNARes] > N->Load) &&
        (BaseCRes - ResVal[rindex][mrtNARes] < N->Load + 1.0))
      {
      /* address rounding error - only N->Load used but N->ARes.Procs rounds up */
   
      EffARes = (double)BaseCRes - N->Load;
      }
    else
      {
      /* external factors at play - use more conservative value */

      EffARes = (double)ResVal[rindex][mrtNARes];
      }

    Threshold = MNodeGetOCThreshold(N,rlindex);
    if (Threshold > 0.0)
      {
      MaxARes = (double)BaseCRes * Threshold;

      EffARes -= (double)BaseCRes * (1 - Threshold);
      }
    else
      {
      MaxARes = (double)BaseCRes;
      }
 
    if ((rlindex == mrlProc) && (EvalCPULoad == TRUE))
      {
      double tmpLoad;

      /* incorporate CPU load */

      tmpLoad = MaxARes - N->Load;

      if ((tmpLoad < EffARes) || (CPULoadIsLoadBased == TRUE))
        {
        /* do not scale CPU load */

        EffARes = MaxARes - N->Load;
        }
      else if (EffARes > 0.0)
        {
        /* NOTE:  if Effective Available Resources is negative, do not scale since
                  load is actual and info is used to migrate */

        /* scale EffARes back up to effectively compare to scaled required resources */
       
        EffARes = MNodeGetOCValueD(N,EffARes,rlindex);
        }
      }
    else if (EffARes > 0.0)
      { 
      /* scale EffARes back up to effectively compare to scaled required resources */
 
      EffARes = MNodeGetOCValueD(N,EffARes,rlindex);
      }

    /* NOTE:  EffARes is bounded between -INFINITY and MaxARes */

    EffARes = MIN(EffARes,MaxARes);

    /* RQ->DRes.X <= N->ARes.X * OCFactor */

    if ((MaxARes > 0.0) && (EffARes < 0.0))
      {
      /* utilization overload detected */

      if (ELoadP != NULL)
        {
        /* should be:  Util / MaxARes */

        *ELoadP = MAX(*ELoadP,(double)(MaxARes - EffARes) / MaxARes);
        }

      continue;
      }

    /* server not currently overcommitted */

    if (MaxARes > 0)
      {
      if (ELoadP != NULL)
        {
        /* should be Util / MaxARes */

        *ELoadP = MAX(*ELoadP,(double)(MaxARes - EffARes) / MaxARes);
        }
      }
    }    /* END for (rindex) */

  return(SUCCESS);
  }  /* END MNodeGetEffectiveLoad() */






/**
 * Report effective anticipated load.
 *
 * @see MNodeGetAvailTC() - parent
 * @see MNodeGetEffectiveLoad() - parent
 * @see MNodeGetVMLoad() - child
 * @see MNodeGetNumVMCreate() - child
 */

int MNodeGetAnticipatedLoad(

  const mnode_t *N,
  mbool_t        JobCentric,
  double         OverrideVMLoad,
  double         *LoadP)

  {
  double BaseLoad;

  if (LoadP != NULL)
    *LoadP = 0.0;

  if ((N == NULL) || (LoadP == NULL))
    {
    return(FAILURE);
    }

  BaseLoad = N->CRes.Procs - N->ARes.Procs;

  switch (N->HVType)
    {
    case mhvtKVM:

      {
      double VMLoad;
      int    PJCount;

      /* for hypervisors with direct utilization monitor, cpu utilization 
         associated with VM create is reported w/in hypervisor load,
         do not double count */

      MNodeGetVMLoad(N,&VMLoad,&PJCount);

      if (OverrideVMLoad > 0.0)
        {
        VMLoad = OverrideVMLoad;
        }

      if (PJCount > 0)
        {
        /* compute jobs present - cannot differentiate between compute load
           and vm create load - be conservative - available load is reduced
           by both measured load and anticipated vm create load */
#if 0
        *LoadP = BaseLoad + MSched.FutureVMLoadFactor * MNodeGetNumVMCreate(N,JobCentric);
#endif
        *LoadP = BaseLoad;
        }
      else
        {
        /* no compute jobs present - effective load is reduced by
           load inside of VM's and anticipated vm create load */
#if 0
        *LoadP = VMLoad + MSched.FutureVMLoadFactor * MNodeGetNumVMCreate(N,JobCentric);
#endif
        *LoadP = VMLoad;
        }
      }  /* END BLOCK (case mhvtKVM) */

      break;

    default:

      {
      double VMLoad = 0.0;

      /* hypervisor does not have direct utilization monitor, cpu utilization
         associated w/VM create is NOT reported w/in hypervisor load, and will
         not be double counted */

      /* without a proper monitor on the HV, assume HV's load is the sum of the VM's load */

      MNodeGetVMLoad(N,&VMLoad,NULL);

      if (OverrideVMLoad > 0.0)
        {
        VMLoad = OverrideVMLoad;
        }

#if 0
      /* Add in VM's that are currently being created */
      VMLoad += MSched.FutureVMLoadFactor * MNodeGetNumVMCreate(N,JobCentric);
#endif

      *LoadP = VMLoad;
      }  /* END BLOCK */

      break;
    }  /* END switch (N->HVType) */

  return(SUCCESS);
  }    /* END MNodeGetAnticipatedLoad() */




/**
 * Calculates and returns the VM load and physical job count
 * for the given node.
 *
 * @param N (I) The given node.
 * @param VMLoadP (I/O) [optional] Is used to return the aggregate CPU load of all VM's on given node.
 * @param PJCountP (I/O) [optional] Is used to return the total physical job count on this node.
 */

int MNodeGetVMLoad(

  const mnode_t *N,
  double        *VMLoadP,
  int           *PJCountP)

  {
  if (VMLoadP != NULL)
    *VMLoadP = 0.0;

  if (PJCountP != NULL)
    *PJCountP = 0;

  if (N == NULL)
    {
    return(FAILURE);
    }

  /* add up load of all VMs */

  if ((VMLoadP != NULL) && (N->VMList != NULL))
    {
    mln_t *P;
    mvm_t *V;

    for (P = N->VMList;P != NULL;P = P->Next)
      {
      V = (mvm_t *)P->Ptr;

      (*VMLoadP) += V->CPULoad;
      }
    }

  /* count up the number of physical jobs on the node */

  if ((PJCountP != NULL) && (N->JList != NULL))
    {
    int jindex;
    mjob_t *J;

    for (jindex = 0;jindex < N->MaxJCount;jindex++)
      {
      J = N->JList[jindex];

      if (J == NULL)
        break;

      if (J == (mjob_t *)1)
        continue;

      if (MJOBISACTIVE(J))
        {
        /* physical machine job detected */

        (*PJCountP)++;
        }
      } 
    } 

  return(SUCCESS);
  }  /* END MNodeGetVMLoad() */



/**
 * Returns the name of the first VLAN found on the node.
 *
 * Assumes that nodes are members of only one VLAN.  For the
 * purposes of VM migration the target hypervisor would certainly 
 * need to be part of all the same networks for which interfaces are 
 * presented to the VM in question in order to maintain connectivity.
 * This would require the notion of VM-attached networks which are not 
 * present in Moab. 
 * 
 * @see MVMSelectDestination() - parent
 *
 * @param N (I) Node to get VLAN info from.
 * @param VLAN (I/O) String that represents VLAN node belongs to [minsize=MMAX_LINE]
 */
int MNodeGetVLAN(

  mnode_t  *N,
  char     *VLAN)  /* feature corresponding to VLAN */
    
  {
  int ix; 

  if ((N == NULL) || (VLAN == NULL))
    {
    return(FAILURE);
    }

  VLAN[0] = '\0';

  /* search node feature list for vlan in NodeSetList */
  for (ix = 0;ix < MMAX_ATTR;++ix)
    {
    if (MPar[0].NodeSetList[ix] == NULL)
      {
      break;
      }

    if (bmisset(&N->FBM,MUMAGetIndex(meNFeature,MPar[0].NodeSetList[ix],mVerify)))
      {
      MUStrCpy(VLAN,MPar[0].NodeSetList[ix],MMAX_LINE);

      return(SUCCESS);
      }
    }
    
  return(FAILURE);
  }  /* END MNodeGetVLAN() */


int MNodeGetVMConfiguredRes(

  mnode_t *N,
  int *Procs,
  int *Mem)

  {
  mln_t *vptr;

  if ((N == NULL) ||
      (Procs == NULL) ||
      (Mem == NULL))
    return(FAILURE);

  *Procs = 0;
  *Mem = 0;

  for (vptr = N->VMList;vptr != NULL;vptr = vptr->Next)
    {
    mvm_t *V = (mvm_t *)vptr->Ptr;

    if (V == NULL)
      continue;

    if (MVMIsMigrating(V))
      continue;

    *Mem += V->CRes.Mem;
    *Procs += V->CRes.Procs;
    }  /* END for (vptr) */

  return(SUCCESS);
  }  /* END MNodeGetVMConfiguredRes() */


mbool_t MNodeVMsAreOverConfiguredRes(

  mnode_t *N)

  {
  int ConfMem = 0;
  int ConfProc = 0;

  if (N == NULL)
    return(FALSE);

  MNodeGetVMConfiguredRes(N,&ConfProc,&ConfMem);

  /* Have to assume that we're already using the overcommit value here */

  if (ConfMem > N->CRes.Mem)
    return(TRUE);

  if (ConfProc > N->CRes.Procs)
    return(TRUE);

  return(FALSE);
  }  /* END MNodeVMsAreOverConfiguredRes() */

/* END MNode.c */

/* vim: set ts=2 sw=2 expandtab nocindent filetype=c: */
