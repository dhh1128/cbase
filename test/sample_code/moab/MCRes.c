/* HEADER */

      
/**
 * @file MCRes.c
 *
 * Contains: Resource structure functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 * Initialize an mcres_t structure and populate the GRes pointer.
 *
 * NOTE: must eventually call MCResFree()
 *
 * @param R
 */

int MCResInit(

  mcres_t *R)

  {
  memset(R,0,sizeof(mcres_t));

#ifndef MCONSTGRES
  MSNLInit(&R->GenericRes);
#else
  R->GenericRes.Size = MMAX_CONSTGRES;
#endif

  return(SUCCESS);
  }  /* END MCResInit() */



/**
 * Compare 2 mcres_t structures.
 *
 * NOTE: return SUCCESS if they are the same, FAILURE if they are different 
 *
 * @param R1 (I)
 * @param R2 (I)
 */

int MCResCmp(

  const mcres_t *R1,
  const mcres_t *R2)

  {
  int index;

  if (R1->Procs != R2->Procs)
    {
    return(FAILURE);
    }

  if (R1->Disk != R2->Disk)
    {
    return(FAILURE);
    }

  if (R1->Swap != R2->Swap)
    {
    return(FAILURE);
    }

  if (R1->Mem != R2->Mem)
    {
    return(FAILURE);
    }

  if (MSNLGetIndexCount(&R1->GenericRes,0) != MSNLGetIndexCount(&R2->GenericRes,0))
    {
    return(FAILURE);
    }
 
  for (index = 1;index < MSched.M[mxoxGRes];index++)
    {
    if (MSNLGetIndexCount(&R1->GenericRes,index) != MSNLGetIndexCount(&R2->GenericRes,index))
      return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MCResCmp() */




/**
 * Copies an mcres_t structure from Src to Dst.
 * 
 * @param Dst
 * @param Src
 */

int MCResCopy(

  mcres_t       *Dst,
  const mcres_t *Src)

  {
  if ((Dst == NULL) || (Src == NULL))
    {
    return(FAILURE);
    }

#ifdef MCONSTGRES
  memcpy(Dst,Src,sizeof(mcres_t));
#else
  Dst->Procs = Src->Procs;
  Dst->Swap  = Src->Swap;
  Dst->Disk  = Src->Disk;
  Dst->Mem   = Src->Mem;

  MSNLCopy(&Dst->GenericRes,&Src->GenericRes);
#endif

  return(SUCCESS);
  }  /* END MCResCopy() */


/**
 * Copies a mcres_t structure from Src to Dst and converts 
 * Dst->Procs to Configured->Procs if Src->Procs is -1.
 *
 * Note:  mcres_t->Procs == -1 means that all the procs are blocked on the node. 
 *        Does this apply to the attributes of mcres_t?
 * 
 * @param Dst (I) [modified]
 * @param Src (I)
 * @param Configured (I)
 */

int MCResCopyBResAndAdjust(

  mcres_t       *Dst,
  const mcres_t *Src,
  const mcres_t *Configured)

  {
  if ((Dst == NULL) || (Src == NULL) || (Configured == NULL))
    {
    return(FAILURE);
    }

  if (MCResCopy(Dst,Src) == FAILURE)
    return(FAILURE);

  if (Dst->Procs == -1)
    Dst->Procs = Configured->Procs;

  return(SUCCESS);
  }  /* END MCResCopyBResAndAdjust() */


/**
 * Checks to see if an mcres_t structure is empty.
 * 
 * @param Res
 */

mbool_t MCResIsEmpty(

  mcres_t *Res) /* I */

  {
  if ((Res->Procs > 0) || 
      (Res->Swap > 0) ||  
      (Res->Disk > 0) || 
      (Res->Mem > 0))
    return(FALSE);
    
  if (MSNLIsEmpty(&Res->GenericRes) == FALSE)
    return(FALSE);

  return(TRUE);
  }  /* END MCResIsEmpty() */


/**
 * Frees an mcres_t structure populated by MCResInit().
 *
 * @param Res
 */

int MCResFree(

  mcres_t *Res)

  {
  if (Res == NULL)
    {
    return(SUCCESS);
    }

  MSNLFree(&Res->GenericRes);

  return(SUCCESS);
  }  /* END MCResFree() */




/**
 * Clears an mcres_t structure (with zeros).
 *
 * @param Res
 */

int MCResClear(

  mcres_t *Res)

  {
  Res->Procs = 0;
  Res->Swap  = 0;
  Res->Disk  = 0;
  Res->Mem   = 0;

  MSNLClear(&Res->GenericRes);

  return(SUCCESS);
  }  /* END MCResClear() */




int MCResSumGRes(

  msnl_t *GRes1,
  int     TC1,
  msnl_t *GRes2,
  int     TC2,
  msnl_t *DGRes)

  {
  int gindex;

  MSNLClear(DGRes);

  for (gindex = 1;gindex < MSched.M[mxoxGRes];gindex++)
    {
    if (MGRes.Name[gindex][0] == '\0')
      break;

    MSNLSetCount(DGRes,gindex,MSNLGetIndexCount(GRes1,gindex)*TC1 + MSNLGetIndexCount(GRes2,gindex)*TC2);
    }  /* END for (gindex) */

  return(SUCCESS);
  }  /* END MRsvJoinSumGRes() */


/**
 * Add Def x TC to Res.
 *
 * NOTE: Res is not initialized here
 */

int MCResAddDefTimesTC(

  mcres_t *Res,
  mcres_t *Def,
  int      TC)
  
  {
  int gindex;

  if ((Res == NULL) || (Def == NULL) || (TC <= 0))
    {
    return(FAILURE);
    }

  Res->Procs += Def->Procs * TC;
  Res->Swap  += Def->Swap * TC;
  Res->Mem   += Def->Mem * TC;
  Res->Disk  += Def->Disk * TC;

  for (gindex = 1;gindex < MSched.M[mxoxGRes];gindex++)
    {
    if (MGRes.Name[gindex][0] == '\0')
      break;

    MSNLSetCount(&Res->GenericRes,gindex,MSNLGetIndexCount(&Res->GenericRes,gindex) + MSNLGetIndexCount(&Def->GenericRes,gindex));
    }

  return(SUCCESS);
  }  /* END MCResAdd() */



/**
 * Normalize a mcres_t structure down to one proc and appropriate everything else.
 *
 * @param R
 * @param TC
 */

int MCResNormalize(

  mcres_t *R,
  int     *TC)

  {
  int TotalProc;

  int tmpTC;

  int gindex;

  if ((R == NULL) || (TC == NULL))
    {
    return(FAILURE);
    } 

  if (R->Procs <= 1)
    {
    return(SUCCESS);
    }

  tmpTC = *TC;

  TotalProc = R->Procs * tmpTC;

  /* redefine task to be 1 proc and [mem|swap|disk|net|gres]/TotalProc */

  R->Procs = 1;
  R->Mem   = (R->Mem * tmpTC) / TotalProc;
  R->Swap  = (R->Swap * tmpTC) / TotalProc;
  R->Disk  = (R->Disk * tmpTC) / TotalProc;
  
  for (gindex = 1;gindex < MSched.M[mxoxGRes];gindex++)
    {
    if (MGRes.Name[gindex][0] == '\0')
      break;

    if (MSNLGetIndexCount(&R->GenericRes,gindex) <= 0)
      continue;

    MSNLSetCount(&R->GenericRes,gindex,(MSNLGetIndexCount(&R->GenericRes,gindex) * tmpTC) / TotalProc);
    }

  *TC = TotalProc;

  return(SUCCESS);
  }  /* END MCResNormalize() */







/**
 * Return the number of Res2 that can fit in Res1.
 */

int MCResGetTC(

  mcres_t *Cfg,  /* Configured resources */
  int      CTCM, /* Configured TC multiplier */
  mcres_t *Req,  /* Requested resources */
  int     *TCP)

  {
  int Factor;

  int CRes[mrLAST];  /* configured resources */
  int RRes[mrLAST];  /* requested resources */

  int rindex;

  int TC;
  int tmpTC = 0;

  if ((Cfg == NULL) || (Req == NULL) || (TCP == NULL))
    {
    return(FAILURE);
    }

  if (CTCM <= 0)
    {
    Factor = 1;
    }
  else
    {
    Factor = CTCM;
    }

  TC = MDEF_TASKS_PER_NODE;

  RRes[mrProc] = Req->Procs;
  CRes[mrProc] = Cfg->Procs * Factor;

  RRes[mrMem] = Req->Mem;
  CRes[mrMem] = Cfg->Mem * Factor;

  RRes[mrSwap] = Req->Swap;
  CRes[mrSwap] = Cfg->Swap * Factor;

  RRes[mrDisk] = Req->Disk;
  CRes[mrDisk] = Cfg->Disk * Factor;

  for (rindex = mrProc;rindex <= mrDisk;rindex++)
    {
    if (RRes[rindex] == 0)
      continue;

    if (CRes[rindex] < RRes[rindex])
      {
      MDB(8,fSCHED) MLog("INFO:     inadequate %s (C:%d < R:%d)\n",
        MResourceType[rindex],
        CRes[rindex],
        RRes[rindex]);

      return(0);
      }

    TC = MIN(TC,CRes[rindex] / RRes[rindex]);
    }    /* END for (rindex) */

  if (!MSNLIsEmpty(&Req->GenericRes))
    {
    /* process dedicated generic resources */

    /* NOTE:  always check dedicated resources regardless of pval policy */

    int     gindex;

    msnl_t  tmpGRes;

    MSNLInit(&tmpGRes);

    for (gindex = 1;gindex <= MSched.GResCount;gindex++)
      {
      MSNLSetCount(&tmpGRes,gindex,MSNLGetIndexCount(&Cfg->GenericRes,gindex) * Factor);
      }  /* END for (gindex) */

    MSNLGetCount(
      0,
      &Req->GenericRes,
      &tmpGRes,
      &tmpTC);

    TC = MIN(TC,tmpTC);
    }  /* END if (!MSNLIsEmpty(&Req->GenericRes)) */

  *TCP = TC;
  
  return(SUCCESS);
  }  /* END MCResGetTC() */




/**
 * Add a node's idle resources (non-dedicated) to Res.
 *
 * NOTE: MNodeGetTC() doesn't work as it requires a task definition and we
 *       don't want to define the task, just get the idle resources.
 *
 * @param N
 * @param Res (O) - NOT initialized in this routine
 */

int MCResAddNodeIdleResources(

  mnode_t *N,
  mcres_t *Res)

  {
  int gindex;

  if ((N == NULL) || (Res == NULL))
    {
    return(FAILURE);
    }

  Res->Procs += N->CRes.Procs - N->DRes.Procs;
  Res->Swap  += N->CRes.Swap - N->DRes.Swap;
  Res->Mem   += N->CRes.Mem - N->DRes.Mem;
  Res->Disk  += N->CRes.Disk - N->DRes.Disk;

  for (gindex = 1;gindex < MSched.M[mxoxGRes];gindex++)
    {
    if (MGRes.Name[gindex][0] == '\0')
      break;

    MSNLSetCount(&Res->GenericRes,gindex,MSNLGetIndexCount(&Res->GenericRes,gindex) + (MSNLGetIndexCount(&N->CRes.GenericRes,gindex) - MSNLGetIndexCount(&N->DRes.GenericRes,gindex)));
    }

  return(SUCCESS);
  }  /* END MCResAddNodeIdleResources() */




/**
 * Return TRUE if any resources in Res2 are available Res1 (regardless of quantity).
 *
 */

mbool_t MCResHasOverlap(

  mcres_t *Res1,
  mcres_t *Res2)

  {
  int gindex;

  if ((Res1->Procs != 0) && (Res2->Procs != 0))
    {
    return(TRUE);
    }

  if ((Res1->Swap != 0) && (Res2->Swap != 0))
    {
    return(TRUE);
    }

  if ((Res1->Disk != 0) && (Res2->Disk != 0))
    {
    return(TRUE);
    }

  if ((Res1->Mem != 0) && (Res2->Mem != 0))
    {
    return(TRUE);
    }

  if (MSNLIsEmpty(&Res1->GenericRes) && MSNLIsEmpty(&Res2->GenericRes))
    {
    return(FALSE);
    }

  for (gindex = 1;gindex < MSched.M[mxoxGRes];gindex++)
    {
    if (MGRes.Name[gindex][0] == '\0')
      break;

    if ((MSNLGetIndexCount(&Res1->GenericRes,gindex) != 0) && (MSNLGetIndexCount(&Res2->GenericRes,gindex) != 0))
      {
      return(TRUE);
      }
    } 

  return(FALSE);
  }  /* END MCResHasOverlap() */



/**
 *
 *
 * @param R
 * @param Res1
 * @param Res2
 */

int MCResGetMin(
 
  mcres_t *R,
  mcres_t *Res1,
  mcres_t *Res2)
 
  {
  int index;
 
  R->Procs = MIN(Res1->Procs,Res2->Procs);
  R->Mem   = MIN(Res1->Mem,  Res2->Mem  );
  R->Disk  = MIN(Res1->Disk, Res2->Disk );
  R->Swap  = MIN(Res1->Swap, Res2->Swap );
 
  for (index = 1;index < MSched.M[mxoxGRes];index++)
    {
    if (MGRes.Name[index][0] == '\0')
      break;
 
    MSNLSetCount(&R->GenericRes,index,MIN(MSNLGetIndexCount(&Res1->GenericRes,index),MSNLGetIndexCount(&Res2->GenericRes,index)));
    }  /* END for (index) */
 
  return(SUCCESS);
  }  /* END MCResGetMin() */




/**
 *
 *
 * @param R
 * @param Res1
 * @param Res2
 */

int MCResGetMax(
 
  mcres_t *R,
  mcres_t *Res1,
  mcres_t *Res2)
 
  {
  int index;
 
  R->Procs = MAX(Res1->Procs,Res2->Procs);
  R->Mem   = MAX(Res1->Mem  ,Res2->Mem  );
  R->Disk  = MAX(Res1->Disk ,Res2->Disk );
  R->Swap  = MAX(Res1->Swap ,Res2->Swap );
 
  for (index = 1;index < MSched.M[mxoxGRes];index++)
    {
    if (MGRes.Name[index][0] == '\0')
      break;
 
    MSNLSetCount(&R->GenericRes,index,MAX(MSNLGetIndexCount(&Res1->GenericRes,index),MSNLGetIndexCount(&Res2->GenericRes,index)));
    }  /* END for (index) */
 
  return(SUCCESS);
  }  /* END MCResGetMax() */




/**
 * Return TRUE if resources are negative.
 * NOTE: DRes is what resources will be used, what resources we "care" about.  If DRes is not NULL,
 *        then MCResIsNeg will return TRUE on GRes only if the DRes uses any of the negative GRes.
 *        If a GRes is negative and DRes does not use it, MCResIsNeg will still return FALSE.
 *
 * @param R (I)
 * @param RIndex (O) [optional]
 * @param DRes (I) [optional]
 */

mbool_t MCResIsNeg(

  mcres_t            *R,
  enum MAllocRejEnum *RIndex,
  const mcres_t      *DRes)
 
  {
  int index;

  if (RIndex != NULL)
    *RIndex = marNONE;

  if ((R->Procs < 0) ||
      (R->Mem   < 0) ||
      (R->Disk  < 0) ||
      (R->Swap  < 0))
    {
    if (RIndex != NULL)
      {
      if (R->Procs < 0)
        *RIndex = marCPU;
      else if (R->Mem < 0)
        *RIndex = marMemory;
      else if (R->Disk < 0)
        *RIndex = marDisk;
      else
        *RIndex = marSwap;
      }

    return(TRUE);
    }
 
  if (((MSNLGetIndexCount(&R->GenericRes,0) < 0)) &&
      (DRes == NULL))
    {
    if (RIndex != NULL)
      {
      *RIndex = marGRes;
      }

    return(TRUE);
    }

  for (index = 1;index < MSched.M[mxoxGRes];index++)
    {
    if (MGRes.Name[index][0] == '\0')
      break;

    if ((MSNLGetIndexCount(&R->GenericRes,index) < 0) &&
        ((DRes == NULL) ||
         (MSNLGetIndexCount(&DRes->GenericRes,index) > 0)))
      {
      if (RIndex != NULL)
        *RIndex = marGRes;

      return(TRUE);
      }
    }
 
  return(FALSE);
  }  /* END MCResIsNeg() */


/**
 * Display mcres_t structure in specified mode.
 *
 * NOTE:  There is also an MCResToString()
 *
 * @see MCResFromString() - peer
 *
 * @param R (I)
 * @param WallTime (I)
 * @param DisplayMode (I) [one of mfmHuman, mfmAVP, or mfmVerbose]
 * @param Buf (O) [optional,minsize=MMAX_LINE]
 */

char *MCResToString(

  const mcres_t       *R,
  long                 WallTime,
  enum MFormatModeEnum DisplayMode,
  char                *Buf)

  {
  static char LocalBuf[MMAX_BUFFER];

  /* should sync with (or replace with) MResourceType[] */

  const char *ResName[] = {
    "PROCS",
    "MEM",
    "SWAP",
    "DISK",
    NULL };

  char *BPtr;
  int   BSpace;

  char *Head;

  int index;

  const int *ResPtr[4];

  int   Val;
  char *N;

  int tmpI;

  if (Buf != NULL)
    {
    MUSNInit(&BPtr,&BSpace,Buf,MMAX_LINE);
    }
  else
    {
    MUSNInit(&BPtr,&BSpace,LocalBuf,sizeof(LocalBuf));
    }

  Head = BPtr;

  ResPtr[0] = &R->Procs;
  ResPtr[1] = &R->Mem;
  ResPtr[2] = &R->Swap;
  ResPtr[3] = &R->Disk;

  /* FORMAT:  <ATTR>=<VAL>[;<ATTR>=<VAL>]... */

  for (index = 0;ResName[index] != NULL;index++)
    {
    Val = *ResPtr[index];
    N   =  (char *)ResName[index];

    if (Val == 0)
      continue;

    if (Head[0] != '\0')
      {
      if (DisplayMode == mfmAVP)
        MUSNCat(&BPtr,&BSpace,";");
      else
        MUSNCat(&BPtr,&BSpace,"  ");
      }

    if (Val > 0)
      {
      tmpI = (WallTime > 0) ? Val / WallTime : Val;        
 
      if (DisplayMode == mfmVerbose)
        {
        /* human readable - percent */

        MUSNPrintF(&BPtr,&BSpace,"%s: %0.2f",
          N,
          (double)tmpI / 100.0);
        }
      else if (DisplayMode == mfmAVP)
        {
        /* machine readable */

        MUSNPrintF(&BPtr,&BSpace,"%s=%d",
          N,
          tmpI);
        }
      else
        {
        /* human readable - basic */

        if (index > 0)
          {
          MUSNPrintF(&BPtr,&BSpace,"%s: %s",
            N,
            MULToRSpec((long)tmpI,mvmMega,NULL));
          }
        else
          {
          MUSNPrintF(&BPtr,&BSpace,"%s: %d",
            N,
            tmpI);
          }
        }
      }
    else 
      {
      if (DisplayMode == mfmAVP)
        {
        MUSNPrintF(&BPtr,&BSpace,"%s=%s",
          N,
          ALL);
        }
      else
        {
        MUSNPrintF(&BPtr,&BSpace,"%s: %s",
          N,
          ALL);
        }
      }
    }    /* END for (index)   */

  /* display generic resources */

  for (index = 1;index < MSched.M[mxoxGRes];index++)
    { 
    if (MGRes.Name[index][0] == '\0')
      break;

    if (MSNLGetIndexCount(&R->GenericRes,index) == 0)
      continue;

    if (DisplayMode == mfmAVP)
      {
      if (Head[0] != '\0')
        MUSNCat(&BPtr,&BSpace,";");

      MUSNPrintF(&BPtr,&BSpace,"gres=%s:%d",
        MGRes.Name[index],
        MSNLGetIndexCount(&R->GenericRes,index));
      }
    else
      {
      if (Head[0] != '\0')
        MUSNCat(&BPtr,&BSpace,"  ");

      MUSNPrintF(&BPtr,&BSpace,"%s: %d",
        MGRes.Name[index],
        MSNLGetIndexCount(&R->GenericRes,index));
      }
    }  /* END for (index) */

  if (Head[0] == '\0')
    strcpy(Head,NONE);
     
  return(Head); 
  }  /* END MCResToString() */



/**
 * Display mcres_t structure in specified mode in an mstring_t.
 *
 * @see MCResFromString() - peer
 *
 * @param R (I)
 * @param WallTime (I)
 * @param DisplayMode (I) [one of mfmHuman, mfmAVP, or mfmVerbose]
 * @param Buf (O)
 */

int MCResToMString(

  const mcres_t       *R,
  long                 WallTime,
  enum MFormatModeEnum DisplayMode,
  mstring_t           *Buf)

  {
  /* should sync with (or replace with) MResourceType[] */

  const char *ResName[] = {
    "PROCS",
    "MEM",
    "SWAP",
    "DISK",
    NULL };

  int index;

  const int *ResPtr[4];

  int   Val;
  char *N;

  int tmpI;

  if ((Buf == NULL) || (R == NULL))
    {
    return(FAILURE);
    }

  MStringSet(Buf,"");

  ResPtr[0] = &R->Procs;
  ResPtr[1] = &R->Mem;
  ResPtr[2] = &R->Swap;
  ResPtr[3] = &R->Disk;

  /* FORMAT:  <ATTR>=<VAL>[;<ATTR>=<VAL>]... */

  for (index = 0;ResName[index] != NULL;index++)
    {
    Val = *ResPtr[index];
    N   =  (char *)ResName[index];

    if (Val == 0)
      continue;

    if (!Buf->empty())
      {
      if (DisplayMode == mfmAVP)
        MStringAppend(Buf,";");
      else
        MStringAppend(Buf,"  ");
      }

    if (Val > 0)
      {
      tmpI = (WallTime > 0) ? Val / WallTime : Val;        
 
      if (DisplayMode == mfmVerbose)
        {
        /* human readable - percent */

        MStringAppendF(Buf,"%s: %0.2f",
          N,
          (double)tmpI / 100.0);
        }
      else if (DisplayMode == mfmAVP)
        {
        /* machine readable */

        MStringAppendF(Buf,"%s=%d",
          N,
          tmpI);
        }
      else
        {
        /* human readable - basic */

        if (index > 0)
          {
          MStringAppendF(Buf,"%s: %s",
            N,
            MULToRSpec((long)tmpI,mvmMega,NULL));
          }
        else
          {
          MStringAppendF(Buf,"%s: %d",
            N,
            tmpI);
          }
        }
      }
    else 
      {
      if (DisplayMode == mfmAVP)
        {
        MStringAppendF(Buf,"%s=%s",
          N,
          ALL);
        }
      else
        {
        MStringAppendF(Buf,"%s: %s",
          N,
          ALL);
        }
      }
    }    /* END for (index)   */

  /* display classes */

  /* NYI */

  /* display generic resources */

  for (index = 1;index < MSched.M[mxoxGRes];index++)
    {
    if (MGRes.Name[index][0] == '\0')
      break;

    if (MSNLGetIndexCount(&R->GenericRes,index) == 0)
      continue;

    if (DisplayMode == mfmAVP)
      {
      if (!Buf->empty())
        MStringAppend(Buf,";");

      MStringAppendF(Buf,"gres=%s:%d",
        MGRes.Name[index],
        MSNLGetIndexCount(&R->GenericRes,index));
      }
    else
      {
      if (!Buf->empty())
        MStringAppend(Buf,"  ");

      MStringAppendF(Buf,"%s: %d",
        MGRes.Name[index],
        MSNLGetIndexCount(&R->GenericRes,index));
      }
    }  /* END for (index) */

  if (Buf->empty())
    MStringSet(Buf,NONE);
     
  return(SUCCESS); 
  }  /* END MCResToMString() */

/**
 * Process Configurable Resource String
 *
 * @see MCResToString() - peer
 * @see MResourceType[]
 *
 * NOTE:  parsing is case insensitive
 *
 * NOTE:  FORMAT:  <RES>{:|=}<VAL>[{,|+|;}<RES>{:|=}<VAL>]... 
 *                 ie, gres=large:2
 *
 * @param R (O)
 * @param Resources (I)
 */

int MCResFromString(

  mcres_t *R,
  char    *Resources)

  {
  char *ptr;
  char *TokPtr = NULL;

  char *ptr2;

  char *String = NULL;

  mbool_t  DoProcs = FALSE;

  if ((Resources == NULL) || (R == NULL))
    {
    return(FAILURE);
    }

  MCResClear(R);

  MUStrDup(&String,Resources);

  /* NOTE:  do not initialize R */           

  /* FORMAT:  <RES>{:|=}<VAL>[{,|+|;}<RES>{:|=}<VAL>]... */

  ptr = MUStrTok(String,",+;",&TokPtr);

  while (ptr != NULL)
    {
    /* load gres */
   
    /* FORMAT:  gres=<ATTR>:<COUNT>[,gres=<ATTR>:<COUNT>]... */
 
    /* NOTE: must load gres first as it may contain a gres with a name
             like "proc" or "disk" */
   
    if ((ptr2 = MUStrStr(ptr,"gres",0,TRUE,FALSE)) != NULL)
      {
      char *tail;
   
      char  tmpLine[MMAX_LINE];
   
      int   count;
      int   index;
   
      ptr2 += strlen("gres") + 1;
   
      MUStrCpy(tmpLine,ptr2,sizeof(tmpLine));
   
      if ((tail = strchr(tmpLine,',')) != NULL)
        {
        *tail = '\0';
        }
   
      /* FORMAT:  <ATTR>[:<COUNT>] */
   
      if ((tail = strchr(tmpLine,':')) != NULL)
        {
        *tail = '\0';
   
        count = (int)strtol(tail + 1,NULL,10);
        }
      else
        {
        count = 1;
        }
   
      if ((index = MUMAGetIndex(
            meGRes,
            tmpLine,
            mAdd)) == FAILURE)
        {
        /* cannot add gres */
   
        return(FAILURE);
        }
   
      MSNLSetCount(&R->GenericRes,index,MSNLGetIndexCount(&R->GenericRes,index) + count);
      }  /* END if ((ptr2 = MUStrStr(ptr,"gres",0,TRUE,FALSE)) != NULL) */
    else if ((ptr2 = MUStrStr(ptr,"procs",0,TRUE,FALSE)) != NULL) 
      {
      DoProcs = TRUE;
   
      ptr2 += strlen("PROCS") + 1;
   
      if (!strcmp(ptr2,"ALL") || !strcmp(ptr2,ALL))
        R->Procs = -1;
      else
        R->Procs = (int)strtol(ptr2,NULL,10);
      }
    else if ((ptr2 = MUStrStr(ptr,(char *)MResourceType[mrProc],0,TRUE,FALSE)) != NULL)
      {
      DoProcs = TRUE;
   
      ptr2 += strlen(MResourceType[mrProc]) + 1;
   
      if (!strcmp(ptr2,"ALL"))
        R->Procs = -1;
      else 
        R->Procs = (int)strtol(ptr2,NULL,10);
      }
    else if ((ptr2 = MUStrStr(ptr,(char *)MResourceType[mrMem],0,TRUE,FALSE)) != NULL)
      {
      ptr2 += strlen(MResourceType[mrMem]) + 1;
   
      if (!strcmp(ptr2,"ALL"))
        R->Mem = -1;
      else 
        R->Mem = (int)MURSpecToL(ptr2,mvmMega,mvmMega);
   
      if ((MAM[0].Type != mamtNONE) && (MAM[0].UseDisaChargePolicy == TRUE) && (DoProcs == FALSE))
        {
        R->Procs = 0;
        }
      }
    else if ((ptr2 = MUStrStr(ptr,(char *)MResourceType[mrDisk],0,TRUE,FALSE)) != NULL)
      {
      ptr2 += strlen(MResourceType[mrDisk]) + 1;
   
      if (!strcmp(ptr2,"ALL"))
        R->Disk = -1;
      else 
        R->Disk = (int)MURSpecToL(ptr2,mvmMega,mvmMega);
      }
    else if ((ptr2 = MUStrStr(ptr,(char *)MResourceType[mrSwap],0,TRUE,FALSE)) != NULL)
      {
      ptr2 += strlen(MResourceType[mrSwap]) + 1;
   
      if (!strcmp(ptr2,"ALL"))
        R->Swap = -1;
      else 
        R->Swap = (int)MURSpecToL(ptr2,mvmMega,mvmMega);
      }
   
    /* load classes */
   
    /* NYI */
   
    ptr = MUStrTok(NULL,",+;",&TokPtr);
    }    /* END while (ptr != NULL) */

  if (DoProcs == FALSE)
    {
    R->Procs = 0;
    }

  MUFree(&String);

  return(SUCCESS);
  }  /* END MCResFromString() */


/**
 *
 *
 * @param DRes (I) [dedicated resources]
 * @param BRes (I) [blocked resources]
 * @param CRes (I) [configured resources]
 * @param RC (I) [reservation count]
 */

char *MCResRatioToString(

  mcres_t *DRes,  /* I (dedicated resources) */
  mcres_t *BRes,  /* I (blocked resources) */
  mcres_t *CRes,  /* I (configured resources) */
  int      RC)    /* I (reservation count) */

  {
  static char Line[MMAX_LINE];

  const char *ResName[] = {
    "Procs",
    "Mem",
    "Swap",
    "Disk",
    NULL };

  char *N;
 
  int index;
 
  int *DResPtr[4];
  int *BResPtr[4];      
  int *CResPtr[4];      

  int  DVal;
  int  BVal;
  int  CVal;

  int TotalRes;

  char *BPtr;
  int   BSpace;

  MUSNInit(&BPtr,&BSpace,Line,sizeof(Line));

  DResPtr[0] = &DRes->Procs;
  DResPtr[1] = &DRes->Mem;
  DResPtr[2] = &DRes->Swap;
  DResPtr[3] = &DRes->Disk;

  BResPtr[0] = &BRes->Procs;
  BResPtr[1] = &BRes->Mem;
  BResPtr[2] = &BRes->Swap;
  BResPtr[3] = &BRes->Disk;

  CResPtr[0] = &CRes->Procs;
  CResPtr[1] = &CRes->Mem;
  CResPtr[2] = &CRes->Swap;
  CResPtr[3] = &CRes->Disk;

  for (index = 0;ResName[index] != NULL;index++)
    {
    DVal = *DResPtr[index];
    BVal = *BResPtr[index];      
    CVal = *CResPtr[index];      

    N = (char *)ResName[index];

    /* NOTE:  RC should be multiplied by DVal not TotalRes??? investigate */
   
    if (CVal > 0)
      TotalRes = CVal;
    else if (DVal != -1)
      TotalRes = RC * DVal;
    else if (BVal != -1)
      TotalRes = BVal;
    else
      TotalRes = 1;

    if (BVal == -1)
      BVal = TotalRes;
 
    if (TotalRes > 0)
      {
      if (Line[0] != '\0')
        MUSNCat(&BPtr,&BSpace,"  ");

      MUSNPrintF(&BPtr,&BSpace,"%s: %d/%d (%.2f%c)",
        N,
        BVal,
        TotalRes,
        (double)BVal * 100.0 / TotalRes,
        '%');
      }
    }  /* END for (index) */

  if (!MSNLIsEmpty(&DRes->GenericRes))
    {
    /* display generic resources */

    for (index = 1;index < MSched.M[mxoxGRes];index++)
      {
      if (MGRes.Name[index][0] == '\0')
        break;

      if ((MSNLGetIndexCount(&BRes->GenericRes,index) == 0) ||
          (MSNLGetIndexCount(&CRes->GenericRes,index) == 0))
        continue;

      if (Line[0] != '\0')
        MUSNCat(&BPtr,&BSpace,"  ");

      BVal     = MSNLGetIndexCount(&BRes->GenericRes,index);

      TotalRes = MSNLGetIndexCount(&CRes->GenericRes,index);

      MUSNPrintF(&BPtr,&BSpace,"gres:%s: %d/%d (%.2f%c)",
        MGRes.Name[index],
        BVal,
        TotalRes,
        (double)BVal * 100.0 / TotalRes,
        '%');
      }  /* END for (index) */
    }    /* END if (DRes->GRes->Count[0] > 0) */
 
  if (Line[0] == '\0')
    strcpy(Line,NONE);

  return(Line);
  }  /* END MCResRatioToString() */


/**
 * Sum 'consumable' resources structures.
 *
 * @param R (I/O) [resources to add to]
 * @param Cfg (I) [resources configured on object]
 * @param Req (I) [task req definition]
 * @param Cnt (I) [tasks to add]
 * @param EnforceConstraints (I) [enforce cfg node min/max resources]
 */

int MCResAdd(

  mcres_t       *R,
  const mcres_t *Cfg,
  const mcres_t *Req,
  int            Cnt,
  mbool_t        EnforceConstraints)

  {
  int index;

  if (R == NULL)
    {
    return(FAILURE);
    }
 
  R->Procs += (Req->Procs == -1) ? Cfg->Procs : MIN(Cfg->Procs,Cnt * Req->Procs);
  R->Mem   += (Req->Mem   == -1) ? Cfg->Mem   : MIN(Cfg->Mem,  Cnt * Req->Mem);
  R->Disk  += (Req->Disk  == -1) ? Cfg->Disk  : MIN(Cfg->Disk, Cnt * Req->Disk);
  R->Swap  += (Req->Swap  == -1) ? Cfg->Swap  : MIN(Cfg->Swap, Cnt * Req->Swap);

  if (EnforceConstraints == TRUE)
    {
    R->Procs = MIN(Cfg->Procs,R->Procs);
    R->Mem   = MIN(Cfg->Mem,  R->Mem);
    R->Disk  = MIN(Cfg->Disk, R->Disk);
    R->Swap  = MIN(Cfg->Swap, R->Swap);
    }

  for (index = 1;index <= MSched.GResCount;index++)
    {
    if (MGRes.Name[index][0] == '\0')
      break;
 
    MSNLSetCount(&R->GenericRes,
                  index,
                  MIN(MSNLGetIndexCount(&Cfg->GenericRes,index),
                  MSNLGetIndexCount(&Req->GenericRes,index) * Cnt) + MSNLGetIndexCount(&R->GenericRes,index));
    }  /* END for (index) */
 
  return(SUCCESS);
  }  /* END MCResAdd() */



/**
 *
 *
 * @param R                  (I/O) [resources to add to]
 * @param Cfg                (I)   [resources configured on object]
 * @param Req                (I)   [task req definition]
 * @param Cnt                (I)   [tasks to add]
 * @param EnforceConstraints (I)   [enforce cfg node min/max resources]
 * @param Spec               (I)   [resources to add (BM of enum MResourceTypeEnum) 
 */

int MCResAddSpec(

  mcres_t       *R,
  const mcres_t *Cfg,
  const mcres_t *Req,
  int            Cnt,
  mbool_t        EnforceConstraints,
  mbitmap_t     *Spec)
    
  {
  int index; 
    
  if (R == NULL)
    {
    return(FAILURE);
    }
  
  if (bmisset(Spec,mrProc))
    R->Procs += (Req->Procs == -1) ? Cfg->Procs : MIN(Cfg->Procs,Cnt * Req->Procs);

  if (bmisset(Spec,mrMem))
    R->Mem   += (Req->Mem   == -1) ? Cfg->Mem   : MIN(Cfg->Mem,  Cnt * Req->Mem);
    
  if (bmisset(Spec,mrDisk))
    R->Disk  += (Req->Disk  == -1) ? Cfg->Disk  : MIN(Cfg->Disk, Cnt * Req->Disk);
    
  if (bmisset(Spec,mrSwap))
    R->Swap  += (Req->Swap  == -1) ? Cfg->Swap  : MIN(Cfg->Swap, Cnt * Req->Swap);
  
  if (EnforceConstraints == TRUE)
    {
    R->Procs = MIN(Cfg->Procs,R->Procs);
    R->Mem   = MIN(Cfg->Mem,  R->Mem);
    R->Disk  = MIN(Cfg->Disk, R->Disk);
    R->Swap  = MIN(Cfg->Swap, R->Swap);
    }

  for (index = 1;index <= MSched.GResCount;index++)
    {     
    if (MGRes.Name[index][0] == '\0')
      break;
    
    MSNLSetCount(&R->GenericRes,index,MIN(MSNLGetIndexCount(&Cfg->GenericRes,index),MSNLGetIndexCount(&Req->GenericRes,index) * Cnt) + MSNLGetIndexCount(&R->GenericRes,index));
    }  /* END for (index) */

  return(SUCCESS);
  }  /* END MCResAddSpec() */ 





/**
 * perform X *= F for all elements in C
 *
 * @param C (I) modified
 * @param F (I) factor to multiply by
 */

int MCResTimes(

  mcres_t *C,
  int      F)

  {
  int index;

  C->Procs *= F;
  C->Mem   *= F;
  C->Disk  *= F;
  C->Swap  *= F;

  for (index = 1;index < MSched.M[mxoxGRes];index++)
    {
    if (MGRes.Name[index][0] == '\0')
      break;

    MSNLSetCount(&C->GenericRes,index,MSNLGetIndexCount(&C->GenericRes,index) * F);
    }

  return(SUCCESS);
  } /* END MCresTimes() */

 
 
 
 
/**
 *
 * Lower bound A with B
 *
 * @param A (I/O)
 * @param B (I)
 */

int MCResLowerBound(

  mcres_t *A,
  mcres_t *B)

  {
  unsigned int index;

  A->Procs = MAX(A->Procs,B->Procs);
  A->Mem   = MAX(A->Mem,B->Mem);
  A->Disk  = MAX(A->Disk,B->Disk);
  A->Swap  = MAX(A->Swap,B->Swap);

  for (index = 1;index < (unsigned int)MSched.M[mxoxGRes];index++)
    {
    if (MGRes.Name[index][0] == '\0')
      break;

    MSNLSetCount(&A->GenericRes,index,MAX(MSNLGetIndexCount(&A->GenericRes,index),MSNLGetIndexCount(&B->GenericRes,index)));
    }

  MSNLRefreshTotal(&A->GenericRes);

  return(SUCCESS);
  }  /* END MCResLowerBound() */




/**
 * Perform A += B. No extra parameters. No constraints. Just addition.
 *
 * @param A (I/O)
 * @param B (I)
 */

int MCResPlus(

  mcres_t       *A,
  const mcres_t *B)

  {
  A->Procs += B->Procs;
  A->Mem   += B->Mem;
  A->Disk  += B->Disk;
  A->Swap  += B->Swap;

  MGResPlus(&A->GenericRes,&B->GenericRes);

  return(SUCCESS);
  }  /* END MCResPlus() */





/**
 *
 * Perform A -= B. No extra parameters. No constraints. Just subtraction.
 *
 * @param A (I/O)
 * @param B (I)
 */

int MCResMinus(

  mcres_t *A,
  mcres_t *B)

  {
  unsigned int index;

  A->Procs -= B->Procs;
  A->Mem   -= B->Mem;
  A->Disk  -= B->Disk;
  A->Swap  -= B->Swap;

  for (index = 1;index < (unsigned int)MSched.M[mxoxGRes];index++)
    {
    if (MGRes.Name[index][0] == '\0')
      break;

    MSNLSetCount(&A->GenericRes,index,MSNLGetIndexCount(&A->GenericRes,index) - MSNLGetIndexCount(&B->GenericRes,index));
    }

  return(SUCCESS);
  }  /* END MCResMinus() */





/**
 * Report unscaled 'original' resource values
 *
 * @see MNodeShowState() - parent
 * @see MNodeGetBaseValue() - child
 */

int MCResGetBase(

  mnode_t *N,     /* I */
  mcres_t *CRes,  /* I specified resources */
  mcres_t *BCRes) /* O base, unmodified resources */

  {
  if ((N == NULL) || (CRes == NULL) || (BCRes == NULL))
    {
    return(FAILURE);
    }

  MCResCopy(BCRes,CRes);

  if (MSched.ResOvercommitSpecified[mrlProc] == TRUE)
    BCRes->Procs = MNodeGetBaseValue(N,mrlProc);

  if (MSched.ResOvercommitSpecified[mrlMem] == TRUE)
    BCRes->Mem = MNodeGetBaseValue(N,mrlMem);

  if (MSched.ResOvercommitSpecified[mrlSwap] == TRUE)
    BCRes->Swap = MNodeGetBaseValue(N,mrlSwap);

  if (MSched.ResOvercommitSpecified[mrlDisk] == TRUE)
    BCRes->Disk = MNodeGetBaseValue(N,mrlDisk);

  return(SUCCESS);
  }  /* END MCResGetBase() */




  

/**
 * Report if needed resources can be found within available resource block.
 *
 * @param ResAvailable (I)
 * @param ResNeeded (I)
 * @param ResIsVM (I)
 * @param EMsg (O)
 *
 * @return TRUE if ResAvailable is a superset of ResNeeded, FALSE otherwise.
 */

mbool_t MCResCanOffer(

  const mcres_t *ResAvailable,
  const mcres_t *ResNeeded,
  mbool_t        ResIsVM,
  char          *EMsg)
  
  {
  unsigned int index;

  int ResInts[][3] = {
      {ResAvailable->Procs,ResNeeded->Procs,marCPU},
      {ResAvailable->Mem,ResNeeded->Mem,marMemory},
      {ResAvailable->Swap,ResNeeded->Swap,marSwap},
      {ResAvailable->Disk,ResNeeded->Disk,marDisk},
      {0,0,marNONE} };

  if (EMsg != NULL)
    EMsg[0] = '\0';

  for (index = 0;ResInts[index][2] != marNONE;index++)
    {
    if (ResInts[index][0] < ResInts[index][1])
      {
      /* resource request exceeds resources available */

      if (ResInts[index][1] > 0)
        {
        if (EMsg != NULL)
          {
          snprintf(EMsg,MMAX_LINE,"%d%s %s required but only %d available",
            ResInts[index][1],
            ((ResInts[index][2] == marMemory) || (ResInts[index][2] == marDisk) || (ResInts[index][2] == marSwap)) ? 
              "MB" : "",
            MAllocRejType[ResInts[index][2]],
            ResInts[index][0]);
          }

        return(FALSE);
        }
      else
        {
        /* job doesn't need this resource--don't stop it, but alert in logs */
        /* this will only happen if ResInts[index][0] <= 0 */

        MDB(3,fSTRUCT) MLog("ALERT:    negative resource (%s = %d) found to be available!\n",
          MAllocRejType[ResInts[index][2]],
          ResInts[index][0]);

        if (ResIsVM == TRUE)
          return(FAILURE);
        }
      }
    }  /* END for (index) */

  if ((ResIsVM != TRUE) || (MSched.ValidateVMGRes == TRUE))
    {
    int tmpTC = 0;

    MSNLGetCount(0,&ResNeeded->GenericRes,&ResAvailable->GenericRes,&tmpTC);

    if (tmpTC <= 0)
      {
      return(FALSE);
      }
    }

  return(TRUE);
  }  /* END MCResCanOffer() */





/**
 *
 *
 * @param R (I/O) [resources to remove from]
 * @param Cfg (I) [resources configured on object]
 * @param Req (I) [task req definition]
 * @param Cnt (I) [# tasks to remove]
 * @param EnforceConstraints (I) [enforce cfg node min/max resources]
 */

int MCResRemove(
 
  mcres_t       *R,
  const mcres_t *Cfg,
  const mcres_t *Req,
  int            Cnt,
  mbool_t        EnforceConstraints)
 
  {
  int index;

  int req;
  int cfg;

  if ((R == NULL) || (Cfg == NULL) || (Req == NULL))
    {
    return(FAILURE);
    }
 
  if (Cnt == 0)
    {
    return(SUCCESS);
    }
 
  R->Procs -= (Req->Procs == -1) ? Cfg->Procs : MIN(Cfg->Procs,Cnt * Req->Procs);
  R->Mem   -= (Req->Mem   == -1) ? Cfg->Mem   : MIN(Cfg->Mem,  Cnt * Req->Mem);
  R->Disk  -= (Req->Disk  == -1) ? Cfg->Disk  : MIN(Cfg->Disk, Cnt * Req->Disk);
  R->Swap  -= (Req->Swap  == -1) ? Cfg->Swap  : MIN(Cfg->Swap, Cnt * Req->Swap);
 
  if (EnforceConstraints == TRUE)
    {
    R->Procs = MAX(0,R->Procs);
    R->Mem   = MAX(0,R->Mem);
    R->Disk  = MAX(0,R->Disk);
    R->Swap  = MAX(0,R->Swap);
    }
 
  if (EnforceConstraints == TRUE)
    { 
    for (index = 1;index <= MSched.GResCount;index++)
      {
      if (MGRes.Name[index][0] == '\0')
        break;

      req = MSNLGetIndexCount(&Req->GenericRes,index) * Cnt;

      if (req <= 0)
        continue;

      cfg = MSNLGetIndexCount(&Cfg->GenericRes,index);

      MSNLSetCount(&R->GenericRes,index,MSNLGetIndexCount(&R->GenericRes,index) - MIN(cfg,req));
      }  /* END for (index) */
    }
  else
    {
    for (index = 1;index <= MSched.GResCount;index++)
      {
      if (MGRes.Name[index][0] == '\0')
        break;

      req = MSNLGetIndexCount(&Req->GenericRes,index) * Cnt;

      if (req <= 0)
        continue;

      cfg = MSNLGetIndexCount(&Cfg->GenericRes,index);

      MSNLSetCount(&R->GenericRes,index,MSNLGetIndexCount(&R->GenericRes,index) - req);
      }  /* END for (index) */
    }

  return(SUCCESS);
  }  /* END MCResRemove() */ 




/**
 *
 *
 * @param R                  (I/O) [resources to remove from]
 * @param Cfg                (I)   [resources configured on object]
 * @param Req                (I)   [task req definition]
 * @param Cnt                (I)   [# tasks to remove]
 * @param EnforceConstraints (I)   [enforce cfg node min/max resources]
 * @param Spec               (I)   [resources to remove (BM of enum MResourceTypeEnum) 
 */

int MCResRemoveSpec(

  mcres_t       *R,
  const mcres_t *Cfg,
  const mcres_t *Req,
  int            Cnt,
  mbool_t        EnforceConstraints,
  mbitmap_t     *Spec)

  {
  int index;

  if ((R == NULL) || (Cfg == NULL) || (Req == NULL))
    {
    return(FAILURE);
    }

  if (Cnt == 0)
    {
    return(SUCCESS);
    }

  if (bmisset(Spec,mrProc))
    R->Procs -= (Req->Procs == -1) ? Cfg->Procs : MIN(Cfg->Procs,Cnt * Req->Procs);

  if (bmisset(Spec,mrMem))
    R->Mem   -= (Req->Mem   == -1) ? Cfg->Mem   : MIN(Cfg->Mem,  Cnt * Req->Mem);

  if (bmisset(Spec,mrDisk))
    R->Disk  -= (Req->Disk  == -1) ? Cfg->Disk  : MIN(Cfg->Disk, Cnt * Req->Disk);

  if (bmisset(Spec,mrSwap))
    R->Swap  -= (Req->Swap  == -1) ? Cfg->Swap  : MIN(Cfg->Swap, Cnt * Req->Swap);

  if (EnforceConstraints == TRUE)
    {
    R->Procs = MAX(0,R->Procs);
    R->Mem   = MAX(0,R->Mem);
    R->Disk  = MAX(0,R->Disk);
    R->Swap  = MAX(0,R->Swap);
    }

  if (bmisset(Spec,mrGRes))
    {
    if (EnforceConstraints == TRUE)
      {
      for (index = 1;index <= MSched.GResCount;index++)
        {
        if (MGRes.Name[index][0] == '\0')
          break;
  
        MSNLSetCount(&R->GenericRes,index,MIN(MSNLGetIndexCount(&Cfg->GenericRes,index),MSNLGetIndexCount(&Req->GenericRes,index) * Cnt));
        }  /* END for (index) */
      }
    else
      {
      for (index = 1;index <= MSched.GResCount;index++)
        {
        if (MGRes.Name[index][0] == '\0')
          break;
  
        MSNLSetCount(&R->GenericRes,index,MSNLGetIndexCount(&R->GenericRes,index) - MSNLGetIndexCount(&Req->GenericRes,index) * Cnt);
        }  /* END for (index) */
      }
    }

  return(SUCCESS);
  }  /* END MCResRemoveSuspendedResources() */
/* END MCRes.c */
