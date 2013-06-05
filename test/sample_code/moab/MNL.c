/* HEADER */

/**
 * @file MNL.c
 * 
 * Contains various functions that simplify common operations for the mnl_t object.
 *
 */

#include "moab.h"
#include "moab-proto.h"
#include "moab-global.h"



int __MNLGrow(mnl_t *,int);


/**
 * Returns the mnl_t's size.
 *
 * Size is how large the mnalloc_old_t Array is. The size is not how many elements have
 * been added to the Array.
 */

int MNLSize(

  mnl_t *NL)

  {
  MASSERT(NL != NULL,"nodelist null when getting nodelist's size");

  return(NL->Size);
  }

/**
 * Convert specified mnode_t array into mnl_t array.
 *
 * @param Nodes (I)
 * @param OutNL (O)
 * @param TC (I) - if set to '-1', use node config proc count
 */

int MNLFromArray(

  mnode_t   **Nodes, /* I */
  mnl_t      *OutNL, /* O */
  int         TC)    /* I - use '-1' for not specified */

  {
  int nindex;

  if ((Nodes == NULL) || (OutNL == NULL))
    {
    return(FAILURE);
    }

  if (TC < 0)
    {
    for (nindex = 0;Nodes[nindex] != NULL;nindex++)
      {
      MNLSetNodeAtIndex(OutNL,nindex,Nodes[nindex]);
      MNLSetTCAtIndex(OutNL,nindex,Nodes[nindex]->CRes.Procs);
      }
    }
  else
    {
    for (nindex = 0;Nodes[nindex] != NULL;nindex++)
      {
      MNLSetNodeAtIndex(OutNL,nindex,Nodes[nindex]);
      MNLSetTCAtIndex(OutNL,nindex,TC);
      }
    }

  /* terminate list */

  MNLTerminateAtIndex(OutNL,nindex);

  return(SUCCESS);
  }  /* END MNLFromArray() */





/**
 * Convert mnalloc array to a regex safe string.
 *
 * @param NL
 * @param String (WARNING: *NOT* initialized or freed in this routine)
 */

int MNLToRegExString(

  mnl_t      *NL,
  mstring_t  *String)

  {
  int nindex;

  mnode_t *N;

  for (nindex = 0;MNLGetNodeAtIndex(NL,nindex,&N) == SUCCESS;nindex++)
    {
    MStringAppendF(String,"%s^%s$",
      (nindex > 0) ? "," : "",
      N->Name);
    }

  return(SUCCESS);
  }  /* END MNLToRegExString() */



/**
 * Convert mnl_t array to a string.
 *
 * NOTE: There's also an MNLToMString()
 *
 * NOTE: if PrefixC is set, it will be removed from the start of the node's name in output
 *
 * @param NL (I)
 * @param ShowTC (I) [if MBNOTSET, show if TC > 1]
 * @param SDelim (I)
 * @param PrefixC (I) [optional] char of node name prefix ('\0' is not set)
 * @param Buf (O)
 * @param BufSize (I)
 */

int MNLToString(

  mnl_t      *NL,
  mbool_t     ShowTC,
  const char *SDelim,
  char        PrefixC,
  char       *Buf,
  int         BufSize)

  {
  char *BPtr;
  int   BSpace = 0;

  int   nindex;

  char  DelimBuf[MMAX_LINE];

  const char *FName = "MNLToString";

  char *ptr = NULL;

  mnode_t *N;

  int TC;

  if (Buf != NULL)
    MUSNInit(&BPtr,&BSpace,Buf,BufSize);

  if ((NL == NULL) || (MNLIsEmpty(NL)))
    {
    return(SUCCESS);
    }

  if (SDelim == NULL)
    strcpy(DelimBuf,",");
  else
    MUStrCpy(DelimBuf,SDelim,sizeof(DelimBuf));

  if (ShowTC != FALSE)
    {
    if (((MNLGetNodeAtIndex(NL,0,&N) == SUCCESS) &&
         ((ptr = strchr(N->Name,PrefixC)) == NULL)) || 
        (PrefixC == '\0'))
      {
      if ((ShowTC == TRUE) || (MNLGetTCAtIndex(NL,0)))
        {
        MUSNPrintF(&BPtr,&BSpace,"%s:%d",
          N->Name,
          MNLGetTCAtIndex(NL,0));
        }
      else
        {
        MUSNPrintF(&BPtr,&BSpace,"%s",
          N->Name);
        }
      }
    else if (ptr != NULL)
      {
      if ((ShowTC == TRUE) || (MNLGetTCAtIndex(NL,0) > 1))
        {
        MUSNPrintF(&BPtr,&BSpace,"%s:%d",
          ptr + 1,
          MNLGetTCAtIndex(NL,0));
        }
      else
        {
        MUSNPrintF(&BPtr,&BSpace,"%s",
          ptr + 1);
        }
      }

    for (nindex = 1;MNLGetNodeAtIndex(NL,nindex,&N) == SUCCESS;nindex++)
      {
      TC = MNLGetTCAtIndex(NL,nindex);

      if ((PrefixC == '\0') || ((ptr = strchr(N->Name,PrefixC)) == NULL))
        {
        if ((ShowTC == TRUE) || (TC > 1))
          {
          MUSNPrintF(&BPtr,&BSpace,"%s%s:%d",
            DelimBuf,
            N->Name,
            TC);
          }
        else
          {
          MUSNPrintF(&BPtr,&BSpace,"%s%s",
            DelimBuf,
            N->Name);
          }
        }
      else
        {
        if ((ShowTC == TRUE) || (TC > 1))
          {
          MUSNPrintF(&BPtr,&BSpace,"%s%s:%d",
            DelimBuf,
            ptr + 1,
            TC);
          }
        else
          {
          MUSNPrintF(&BPtr,&BSpace,"%s%s",
            DelimBuf,
            ptr + 1);
          }
        }
      }  /* END for (nindex) */
    }    /* END if (ShowTC != FALSE) */
  else
    {
    /* do not display task info */

    if (((MNLGetNodeAtIndex(NL,0,&N) == SUCCESS) &&
         (ptr = strchr(N->Name,PrefixC)) == NULL) ||
        (PrefixC == '\0'))
      {
      MUSNPrintF(&BPtr,&BSpace,"%s",
        N->Name);
      }
    else if (ptr != NULL)
      {
      MUSNPrintF(&BPtr,&BSpace,"%s",
        ptr + 1);
      }

    for (nindex = 1;MNLGetNodeAtIndex(NL,nindex,&N) == SUCCESS;nindex++)
      {
      if ((PrefixC == '\0') || ((ptr = strchr(N->Name,PrefixC)) == NULL))
        {
        MUSNPrintF(&BPtr,&BSpace,"%s%s",
          DelimBuf,
          N->Name);
        }
      else
        {
        MUSNPrintF(&BPtr,&BSpace,"%s%s",
          DelimBuf,
          ptr + 1);
        }
      }  /* END for (nindex) */
    }    /* END else (ShowTC != FALSE) */

  if (BSpace <= 0)
    {
    /* inadequate buffer space to hold string */

    MDB(1,fSOCK) MLog("ERROR:    inadequate buffer space to hold string in %s\n",
      FName);

    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MNLToString() */




/**
 * Convert mnl_t array to an mstring_t.
 *
 * NOTE: no MUNLToString(), use MNLToString()
 *
 * @param NL (I)
 * @param ShowTC (I) [if MBNOTSET, show if TC > 1]
 * @param SDelim (I)
 * @param PrefixD (I) [optional] delimiter of node name prefix
 * @param Multiplier (I)
 * @param String (O) REQUIRED: call MStringInit() on String before calling this function.
 */

int MNLToMString(

  mnl_t      *NL,
  mbool_t     ShowTC,
  const char *SDelim,
  char        PrefixD,
  int         Multiplier,
  mstring_t  *String)

  {
  int   nindex;

  char  DelimBuf[MMAX_LINE];

  char *ptr = NULL;

  int TC;
 
  mnode_t *N;

  if ((NL == NULL) || (MNLIsEmpty(NL)))
    {
    return(SUCCESS);
    }

  if (Multiplier <= 0)
    Multiplier = 1;

  if (SDelim == NULL)
    strcpy(DelimBuf,",");
  else
    MUStrCpy(DelimBuf,SDelim,sizeof(DelimBuf));

  if (ShowTC != FALSE)
    {
    if (((MNLGetNodeAtIndex(NL,0,&N) == SUCCESS) &&
         (ptr = strchr(N->Name,PrefixD)) == NULL) ||
        (PrefixD == '\0'))
      {
      if ((ShowTC == TRUE) || (MNLGetTCAtIndex(NL,0) > 1))
        {
        MStringAppendF(String,"%s:%d",
          N->Name,
          MNLGetTCAtIndex(NL,0) * Multiplier);
        }
      else
        {
        MStringAppend(String,N->Name);
        }
      }
    else if (ptr != NULL) 
      {
      if ((ShowTC == TRUE) || (MNLGetTCAtIndex(NL,0) > 1))
        {
        MStringAppendF(String,"%s:%d",
          ptr + 1,
          MNLGetTCAtIndex(NL,0) * Multiplier);
        }
      else
        {
        MStringAppend(String,ptr + 1);
        }
      }

    for (nindex = 1;MNLGetNodeAtIndex(NL,nindex,&N) == SUCCESS;nindex++)
      {
      TC = MNLGetTCAtIndex(NL,nindex);

      if (TC == 0)
        break;

      if ((PrefixD == '\0') || ((ptr = strchr(N->Name,PrefixD)) == NULL))
        {
        if ((ShowTC == TRUE) || (TC > 1))
          {
          MStringAppendF(String,"%s%s:%d",
            DelimBuf,
            N->Name,
            TC * Multiplier);
          }
        else
          {
          MStringAppendF(String,"%s%s",
            DelimBuf,
            N->Name);
          }
        }
      else
        {
        if ((ShowTC == TRUE) || (TC > 1))
          {
          MStringAppendF(String,"%s%s:%d",
            DelimBuf,
            ptr + 1,
            TC * Multiplier);
          }
        else
          {
          MStringAppendF(String,"%s%s",
            DelimBuf,
            ptr + 1);
          }
        }
      }  /* END for (nindex) */
    }    /* END if (ShowTC != FALSE) */
  else
    {
    /* do not display task info */

    if (((MNLGetNodeAtIndex(NL,0,&N) == SUCCESS) &&
         (ptr = strchr(N->Name,PrefixD)) == NULL) ||
        (PrefixD == '\0'))
      {
      MStringAppend(String,N->Name);
      }
    else if (ptr != NULL)
      {
      MStringAppend(String,ptr + 1);
      }

    for (nindex = 1;MNLGetNodeAtIndex(NL,nindex,&N) == SUCCESS;nindex++)
      {
      TC = MNLGetTCAtIndex(NL,nindex);

      if (TC == 0)
        break;

      if ((PrefixD == '\0') || ((ptr = strchr(N->Name,PrefixD)) == NULL))
        {
        MStringAppendF(String,"%s%s",
          DelimBuf,
          N->Name);
        }
      else
        {
        MStringAppendF(String,"%s%s",
          DelimBuf,
          ptr + 1);
        }
      }  /* END for (nindex) */
    }    /* END else (ShowTC != FALSE) */

  return(SUCCESS);
  }  /* END MNLToMString() */








/**
 * Convert a node string with optional taskcounts into an mnl_t structure.
 *
 * @param Buffer (I)
 * @param NL (O) [minsize=MMAX_NODE]
 * @param DefaultTC (I) [optional,-1 for notset]
 * @param AddNode (I)
 */

int MNLFromString(

  const char *Buffer,    /* I */
  mnl_t      *NL,        /* O (minsize=MMAX_NODE) */
  int         DefaultTC, /* I (optional,-1 for notset) */
  mbool_t     AddNode)   /* I */

  {
  int nindex;

  char *ptr;
  char *TokPtr = NULL;

  char *nodeid;
  char *tc;

  char *TokPtr2 = NULL;

  char *String = NULL;

  int   rc;

  mnode_t *N;

  /* FORMAT:  <NODEID>[:<TC>][,<NODEID>[:<TC>]]... */

  if (NL != NULL)
    MNLClear(NL);

  if ((Buffer == NULL) || (NL == NULL))
    {
    return(FAILURE);
    }

  MUStrDup(&String,Buffer);

  nindex = 0;

  ptr = MUStrTok(String,", \t\n",&TokPtr);

  while (ptr != NULL)
    {
    nodeid = MUStrTok(ptr,":",&TokPtr2);
    tc     = MUStrTok(NULL,":",&TokPtr2);

    if (AddNode == TRUE)
      rc = MNodeAdd(nodeid,&N);
    else
      rc = MNodeFind(nodeid,&N);

    if (rc == SUCCESS)
      {
      MNLSetNodeAtIndex(NL,nindex,N);

      if (tc != NULL)
        {
        MNLSetTCAtIndex(NL,nindex,(int)strtol(tc,NULL,10));
        }
      else
        {
        /* NOTE: TC default was 0 before 4.5.0p5 */

        if (DefaultTC > 0)
          MNLSetTCAtIndex(NL,nindex,DefaultTC);
        else
          MNLSetTCAtIndex(NL,nindex,1);
        }

      nindex++;

      if (nindex >= MSched.M[mxoNode])
        break;
      }  /* END if (rc == SUCCESS) */

    ptr = MUStrTok(NULL,", \t\n",&TokPtr);
    }  /* END while (ptr != NULL) */

  MNLTerminateAtIndex(NL,MIN(nindex,MSched.M[mxoNode] - 1));

  MUFree(&String);

  return(SUCCESS);
  }  /* END MNLFromString() */





/**
 * Select nodes in NL which are associated with the specified RM. 
 *
 * @param NL (I) [modified]
 * @param R (I)
 */

int MNLRMFilter(

  mnl_t     *NL,  /* I (modified) */
  mrm_t     *R)   /* I */

  {
  int nindex;

  mnode_t *N;

  if ((NL == NULL) || (R == NULL))
    {
    return(FAILURE);
    }

  for (nindex = 0;MNLGetNodeAtIndex(NL,nindex,&N) == SUCCESS;nindex++)
    {
    if (N->RM != R)
      {
      MNLRemove(NL,N);

      continue;
      }
    }  /* END for (nindex) */

  return(SUCCESS); 
  }  /* END MNLFilter() */





/**
 * Compare two nodelists and report differences.
 *
 * NOTE: Performs MNLCompare() operation
 *
 * @see MNLAND() - peer
 *
 * @param NList1 (I)
 * @param NList2 (I)
 * @param Result (O) [optional,minsize=MMAX_NODE] nodes in NList1 but missing in NList2
 * @param Negative (O) [optional] TRUE if nodes in NList1 but missing in NList2
 * @param Positive (O) [optional] TRUE if nodes in NList2 but missing in NList1
 *
 * NOTE:  Only returns FAILURE if one or more inputs are corrupt.
 */

int MNLDiff(

  mnl_t     *NList1,      /* I */
  mnl_t     *NList2,      /* I */
  mnl_t     *Result,   /* O (optional,minsize=MMAX_NODE) nodes in NList1 but missing in NList2 */
  mbool_t   *Negative, /* O (optional) TRUE if nodes in NList1 but missing in NList2 */
  mbool_t   *Positive) /* O (optional) TRUE if nodes in NList2 but missing in NList1 */

  {
  /* return NList1 - NList2 */

  int nindex1;
  int nindex2;
  int RCount = 0;

  int N2Count = 0;
  int MatchCount = 0;

  mnode_t *N1;
  mnode_t *N2 = NULL;  /* initialized to avoid compiler warning */

  if (Negative != NULL)
    *Negative = FALSE;

  if (Positive != NULL)
    *Positive = FALSE;

  if ((NList1 == NULL) || (NList2 == NULL))
    {
    return(FAILURE);
    }

  if (Positive != NULL)
    for (N2Count = 0;MNLGetNodeAtIndex(NList2,N2Count,NULL) == SUCCESS;N2Count++);

  for (nindex1 = 0;MNLGetNodeAtIndex(NList1,nindex1,&N1) == SUCCESS;nindex1++)
    {
    for (nindex2 = 0;MNLGetNodeAtIndex(NList2,nindex2,&N2) == SUCCESS;nindex2++)
      {
      if (N1 == N2)
        break;
      }  /* END for (nindex2) */

    if ((MNLGetNodeAtIndex(NList2,nindex2,NULL) == FAILURE) ||
        (N1 != N2))
      {
      if (Result != NULL)
        {
        MNLSetNodeAtIndex(Result,RCount,N1);
        MNLSetTCAtIndex(Result,RCount,1);
        }

      RCount++;

      if (Negative != NULL)
        *Negative = TRUE;
      }
    else
      {
      MatchCount++;
      }
    }    /* END for (nindex1) */

  if (Positive != NULL)
    {
    if (N2Count > MatchCount)
      *Positive = TRUE;
    }

  if (Result != NULL)
    MNLTerminateAtIndex(Result,RCount);

  return(SUCCESS);
  } /* END MNLDiff() */





/**
 * Report nodes in both nodelists.
 *
 * NOTE: Performs logical AND operation
 *
 * @see MNLDiff() - peer
 *
 * @param NList1 (I)
 * @param NList2 (I)
 * @param Result (O) [optional,minsize=MMAX_NODE] nodes in NList1 and NList2
 *
 * NOTE: Returns FAILURE only if inputs are corrupt.
 */

int MNLAND(

  mnl_t     *NList1,      /* I */
  mnl_t     *NList2,      /* I */
  mnl_t     *Result)   /* O (minsize=MMAX_NODE) nodes in NList1 and NList2 */

  {
  /* return NList1 & NList2 */

  int nindex1;
  int nindex2;
  int RCount = 0;

  mnode_t *N1;
  mnode_t *N2 = NULL;

  if ((NList1 == NULL) || (NList2 == NULL))
    {
    return(FAILURE);
    }

  for (nindex1 = 0;MNLGetNodeAtIndex(NList1,nindex1,&N1) == SUCCESS;nindex1++)
    {
    for (nindex2 = 0;MNLGetNodeAtIndex(NList2,nindex2,&N2) == SUCCESS;nindex2++)
      {
      if (N1 == N2)
        break;
      }  /* END for (nindex2) */

    if (N1 == N2)
      {
      if (Result != NULL)
        {
        MNLSetNodeAtIndex(Result,RCount,N1);
        MNLSetTCAtIndex(Result,RCount,1);
        }

      RCount++;
      }
    }    /* END for (nindex1) */

  if (Result != NULL)
    MNLTerminateAtIndex(Result,RCount);

  return(SUCCESS);
  } /* END MNLAND() */



int MNLSumTC(

  mnl_t *NL)

  {
  int index;
  int TC = 0;

  /* direct access for speed */

  for (index = 0;NL->Array[index].N != NULL;index++)
    {
    TC += NL->Array[index].TC;
    }

  return(TC);
  }  /* END MNLSumTC() */





/**
 * Return TRUE if there are no nodes or the nodelist is NULL.
 *
 * @param NL
 */

mbool_t MNLIsEmpty(

  const mnl_t     *NL)

  {
  if (NL == NULL)
    return(TRUE);

  if (NL->Size == 0)
    return(TRUE);

  if (NL->Array == NULL)
    return(TRUE);

  if (NL->Array[0].N == NULL)
    return(TRUE);

  if (NL->Array[0].TC == 0)
    return(TRUE);

  return(FALSE);
  }  /* END MNLIsEmpty() */



/**
 * Copy a particular index from src to dst.
 *
 * @param Dst
 * @param DstIndex
 * @param Src
 * @param SrcIndex
 */

int MNLCopyIndex(

  mnl_t     *Dst,
  int        DstIndex,
  mnl_t     *Src,
  int        SrcIndex)

  {
  MASSERT((SrcIndex < Src->Size),"internal error: read error beyond end of array\n");

  if (SrcIndex > Src->Size)
    {
    return(FAILURE);
    }

  MNLSetNodeAtIndex(Dst,DstIndex,MNLReturnNodeAtIndex(Src,SrcIndex));
  MNLSetTCAtIndex(Dst,DstIndex,MNLGetTCAtIndex(Src,SrcIndex));

  return(SUCCESS);
  }  /* END MNLCopyIndex() */


/**
 * Terminate the nodelist at the given index.
 *
 * @param NL
 * @param Index
 */

int MNLTerminateAtIndex(

  mnl_t *NL,
  int    Index)

  {
  /* no bounds checking */

  if ((NL->Size == 0) || (NL->Array == NULL))
    return(FAILURE);

  MNLSetNodeAtIndex(NL,Index,NULL);
  MNLSetTCAtIndex(NL,Index,0);

  return(SUCCESS);
  }  /* END MNLTerminateAtIndex() */


/**
 * Clear the nodelist.
 *
 * @param NL
 */

int MNLClear(

  mnl_t     *NL)

  {
  if (NL == NULL)
    return(FAILURE);
    
  if ((NL->Size == 0) || (NL->Array == NULL))
    return(SUCCESS);

  MNLTerminateAtIndex(NL,0);
 
  return(SUCCESS);
  }  /* END MNLClear() */



/**
 * Clear the multi nodelist.
 *
 * @param MNL
 */

int MNLMultiClear(

  mnl_t     **MNL)

  {
  int index;

  for (index = 0;index < MMAX_REQ_PER_JOB;index++)
    {
    MNLClear(MNL[index]);
    }
 
  return(SUCCESS);
  }  /* END MNLMultiClear() */



/**
 * Initialize mnalloc structure.
 * 
 * @param NL
 */

int MNLInit(

  mnl_t     *NL)

  {
  int defaultSize = 2;

  if (NL == NULL)
    return(FAILURE);

  memset(NL,0,sizeof(mnl_t));

  NL->Array = (mnalloc_old_t *)MUCalloc(defaultSize,sizeof(mnalloc_old_t));

  if (NL->Array == NULL)
    return(FAILURE);

  NL->Size = defaultSize;

  return(SUCCESS);
  }  /* END MNLMultiInit() */


/**
 * Free mnalloc structure.
 *
 * @param NL
 */

int MNLFree(

  mnl_t     *NL)

  {
  if (NL == NULL)
    return(FAILURE);

  MUFree((char **)&NL->Array);

  NL->Size = 0;

  return(SUCCESS);
  }  /* END MNLFree() */




/**
 * Initialize multi-mnalloc structure.
 * 
 * NOTE: Assumes array is size MMAX_REQ_PER_JOB.
 *
 * @param MNL
 */

int MNLMultiInit(

  mnl_t     **MNL)

  {
  int index;

  if (MNL == NULL)
    return(FAILURE);

  memset(MNL,0,sizeof(mnl_t *) * MMAX_REQ_PER_JOB);

  for (index = 0;index < MMAX_REQ_PER_JOB;index++)
    {
    MNL[index] = (mnl_t *)MUCalloc(1,sizeof(mnl_t));

    MNLInit(MNL[index]);
    }  /* END for (index) */

  return(SUCCESS);
  }  /* END MNLMultiInit() */

/**
 * Initialize multi-mnalloc structure.
 * 
 * NOTE: array is size Count passed in
 *
 * @param MNL
 * @param Count
 */

int MNLMultiInitCount( 
  mnl_t     **MNL,
  int       Count)

  {
  int index;

  if (MNL == NULL)
    return(FAILURE);

  memset(MNL,0,sizeof(mnl_t *) * Count);

  for (index = 0;index < Count;index++)
    {
    MNL[index] = (mnl_t *)MUCalloc(1,sizeof(mnl_t));

    MNLInit(MNL[index]);
    }  /* END for (index) */

  return(SUCCESS);
  }  /* END MNLMultiInitCount() */

/**
 * Free multi-mnalloc structure.
 *
 * NOTE: Assumes array is size MMAX_REQ_PER_JOB.
 *
 * @param MNL
 */

int MNLMultiFree(

  mnl_t     **MNL)

  {
  int index;

  if (MNL == NULL)
    return(FAILURE);

  for (index = 0;index < MMAX_REQ_PER_JOB;index++)
    {
    MNLFree(MNL[index]);

    MUFree((char **)&MNL[index]);
    }  /* END for (index) */

  memset(MNL,0,sizeof(mnl_t *) * MMAX_REQ_PER_JOB);

  return(SUCCESS);
  }  /* END MNLMultiFree() */

/**
 * Free multi-mnalloc structure.
 *
 * NOTE: array is size Count - passed in
 *
 * @param MNL
 * @param Count
 */

int MNLMultiFreeCount(

  mnl_t     **MNL,
  int       Count)

  {
  int index;

  if (MNL == NULL)
    return(FAILURE);

  for (index = 0;index < Count;index++)
    {
    MNLFree(MNL[index]);

    MUFree((char **)&MNL[index]);
    }  /* END for (index) */

  memset(MNL,0,sizeof(mnl_t *) * Count);

  return(SUCCESS);
  }  /* END MNLMultiFreeCount() */


/**
 * Will grow NL by 2. If the new size is less than the index then the the new size will be Index + 1.
 *
 * @param NL (I)
 * @param Index (I)
 */

int __MNLGrow(

  mnl_t *NL,
  int    Index)

  {
  int NewSize;

  if (NL == NULL)
    return(FAILURE);

  if (NL->Size > Index)
    return(SUCCESS);

  NewSize = (NL->Size > 0) ? NL->Size * 2 : 1;

  if (NewSize <= Index)
    {
    NewSize = Index + 1;
    }

  if (NL->Size == 0)
    {
    NL->Array = (mnalloc_old_t *)MUCalloc(NewSize,sizeof(mnalloc_old_t));
    }
  else
    {
    mnalloc_old_t *tmpArray;
    tmpArray = (mnalloc_old_t *)realloc(NL->Array,NewSize * sizeof(mnalloc_old_t));

    MASSERT(tmpArray != NULL,"can't grow node list.");

    NL->Array = tmpArray;
    memset(NL->Array + NL->Size,0,(NewSize - NL->Size) * sizeof(mnalloc_old_t));
    }

  if (NL->Array == NULL)
    {
    NL->Size = 0;

    return(FAILURE);
    }

  NL->Size = NewSize;

  return(SUCCESS);
  }  /* END __MNLGrow() */


/**
 * Add the TC to the existing nodelist.
 *
 * @param NL
 * @param Index
 * @param TC
 */

int MNLAddTCAtIndex(

  mnl_t     *NL,
  int        Index,
  int        TC)

  {
  int tmpTC = MNLGetTCAtIndex(NL,Index);

  tmpTC += TC;

  MNLSetTCAtIndex(NL,Index,tmpTC);
  
  return(SUCCESS);
  }  /* END MNLAddTCAtIndex() */

/**
 * Set the taskcount the nodelist.
 *
 * @param NL
 * @param Index
 * @param TC
 */

int MNLSetTCAtIndex(

  mnl_t     *NL,
  int        Index,
  int        TC)

  {
  if (NL == NULL)
    return(FAILURE);

  if ((TC < 0) || (Index < 0))
    return(FAILURE);

  if (((NL->Size == 0) || (Index + 1 > NL->Size)) &&
      (__MNLGrow(NL,Index) == FAILURE))
    {
    return(FAILURE);
    }

  /* Shutup coverty although __MNLGrow should handle this. */
  if (NL->Array == NULL)
    return(FAILURE);

  NL->Array[Index].TC = TC;
  
  return(SUCCESS);
  }  /* END MNLSetNodeAtIndex() */



/**
 * Add the node the nodelist.
 *
 * @param NL
 * @param Index
 * @param N
 */

int MNLSetNodeAtIndex(

  mnl_t     *NL,
  int        Index,
  mnode_t   *N)

  {
  if (NL == NULL)
    return(FAILURE);

  if (Index < 0)
    return(FAILURE);

  if (((NL->Size == 0) || (Index + 1 > NL->Size)) &&
      (__MNLGrow(NL,Index) == FAILURE))
    {
    return(FAILURE);
    }

  /* Shutup coverty although __MNLGrow should handle this. */
  if (NL->Array == NULL)
    return(FAILURE);

  NL->Array[Index].N = N;
  
  return(SUCCESS);
  }  /* END MNLSetNodeAtIndex() */




/**
 * Return the node associated with the given index.
 *
 * NOTE: returns NULL if there is no node at that index or
 *       if the NL is too small.
 * 
 * @param NL
 * @param Index
 * @param N
 */

int MNLGetNodeAtIndex(
 
  const mnl_t   *NL,
  int            Index,
  mnode_t      **N)

  {
  if (N != NULL)
    *N = NULL;

  if ((NL == NULL) || ((Index + 1) > NL->Size) || (NL->Size == 0))
    return(FAILURE);

  if (NL->Array[Index].N == NULL)
    return(FAILURE);
 
  if (N != NULL)
    *N = NL->Array[Index].N;
  
  return(SUCCESS);
  }  /* END MNLGetNodeAtIndex() */


/**
 * Return the taskcount associated with the given index.
 *
 * NOTE: returns 0 if there is no node at that index or
 *       if the NL size is too small at that index.
 *
 * @param NL (I)
 * @param Index (I)
 */

int MNLGetTCAtIndex(

  const mnl_t  *NL,
  int           Index)

  {
  if ((Index > NL->Size) || (NL->Size == 0))
    return(0);

  return (NL->Array[Index].TC);
  }  /* END MNLGetTCAtIndex() */



/**
 * Returns the name of the node at the given index.
 *
 * @param NL
 * @param Index
 * @param Name
 */

char *MNLGetNodeName(

  mnl_t     *NL,
  int        Index,
  char      *Name)

  {
  if (NL->Size <= 0)
    return(NULL);

  if ((NL->Size < Index) || (NL->Array[Index].N == NULL))
    return(NULL);

  return(NL->Array[Index].N->Name);
  }
  


/**
 * Returns the the node at the given index.
 *
 * @param NL
 * @param Index
 */

mnode_t *MNLReturnNodeAtIndex(

  mnl_t     *NL,
  int        Index)

  {
  if ((NL == NULL) || (NL->Array == NULL))
    return(NULL);
    
   if ((NL->Size < Index) || (NL->Array[Index].N == NULL))
    return(NULL);

  return(NL->Array[Index].N);
  }  /* END MNLReturnNodeAtIndex() */





/**
 * Convert a new mnl_t structure to an old mnalloc_old_t structure.
 *
 * NOTE: does no bounds checking, calling routine should use MNLCount()
 *       and alloc to the correct size (MNLCount + 1)
 *
 * @param NL
 * @param Old
 */

int MNLToOld(

  mnl_t         *NL,
  mnalloc_old_t *Old)

  {
  mnode_t *N;

  int index;

  for (index = 0;MNLGetNodeAtIndex(NL,index,&N) == SUCCESS;index++)
    {
    Old[index].N  = N;
    Old[index].TC = MNLGetTCAtIndex(NL,index);
    }

  Old[index].N  = NULL;
  Old[index].TC = 0;

  return(SUCCESS);
  }  /* END MNLToOld() */


/**
 * Searches for N in NL and appends it with TC to the end of NL if not found,
 * or increments TC if N is found
 *
 * @see MNLAdd - doesn't control conflict or return out index.
 *
 * @param NL       (I)
 * @param N        (I) node to add
 * @param TC       (I) TaskCount to add
 * @param IncrTCIfFound (I)
 * @param OutIndex (I) set to the last index searched. This value is the index
 *                     where N is located if it is found or appened to the end,
 *                     or Size if the end of the list is reached, or the index
 *
 * @return SUCCESS if N is added to the end,
 *   FAILURE it cannot be appended because there is no more room,
 *   and MBNOTSET if it exists in the list already
 */

int MNLAddNode(

  mnl_t     *NL,
  mnode_t   *N,
  int        TC,
  mbool_t    IncrTCIfFound,
  int       *OutIndex)

  {
  int index;
  int rc = FAILURE;

  if ((N == NULL) || (NL == NULL))
    return(FAILURE);

  if ((MNLFind(NL,N,&index) == FAILURE))
    {
    rc = SUCCESS;

    MNLSetNodeAtIndex(NL,index,N);
    MNLSetTCAtIndex(NL,index,TC);
    MNLTerminateAtIndex(NL,index + 1);
    }
  else
    {
    rc = MBNOTSET;
    if (IncrTCIfFound)
      MNLAddTCAtIndex(NL,index,TC);
    }

  if (OutIndex != NULL)
    *OutIndex = index;

  return(rc);
  } /*END MNLAddNode() */




/**
 * Searches for N in NL.
 *
 * @param NL       (I)
 * @param N        (I) node to search for
 * @param OutIndex (I) set to the last index searched. This value is the index
 *                     where N is located if found, or the index of the first 
 *                     located NULL pointer
 *
 * @return SUCCESS if N is found, FAILURE otherwise
 */

int MNLFind(

  const mnl_t   *NL,
  const mnode_t *N,
  int           *OutIndex)

  {
  int index;

  mnode_t *tmpN;

  for (index = 0;MNLGetNodeAtIndex(NL,index,&tmpN) == SUCCESS;index++)
    {
    if (tmpN == N)
      {
      if (OutIndex != NULL)
        *OutIndex = index;

      return(SUCCESS);
      }
    }

  if (OutIndex != NULL)
    *OutIndex = index;

  return(FAILURE);
  }  /* END MNLFind() */


int MNLRemoveNL(
 
  mnl_t *NL,
  mnl_t *RNL)

  {
  mnode_t *N;
  int      TC;

  int nindex;

  for (nindex = 0;MNLGetNodeAtIndex(RNL,nindex,&N) == SUCCESS;nindex++)
    {
    TC = MNLGetTCAtIndex(RNL,nindex);

    MNLRemoveNode(NL,N,TC);
    }  /* END for (nindex) */

  return(SUCCESS);
  }  /* END MNLRemoveNL() */




/**
 * Remove specified node from existing node list.
 *
 * @param NL (I) [modified]
 * @param N (I)
 * @param TC (I)
 */

int MNLRemoveNode(

  mnl_t     *NL,      /* I (modified)  */
  mnode_t   *N,       /* I */
  int        TC)      /* I (if < 0, remove N regardless of TC, otherwise reduce TC by value) */

  {
  int nindex;

  if ((NL == NULL) || (N == NULL))
    {
    return(FAILURE);
    }

  for (nindex = 0;NL->Array[nindex].N != NULL;nindex++)
    {
    if (NL->Array[nindex].N == N)
      break;
    }  /* END for (nindex) */

  if (NL->Array[nindex].N != N)
    {
    /* match not located */

    return(SUCCESS);
    }

  if ((TC > 0) && (TC < NL->Array[nindex].TC))
    {
    /* keep node but reduce node taskcount */

    NL->Array[nindex].TC -= TC;
  
    return(SUCCESS);
    }
    
  /* remove node from list by sliding remaining nodes in list forward */

  for (;NL->Array[nindex].N != NULL;nindex++)
    {
    NL->Array[nindex].N = NL->Array[nindex + 1].N;
    NL->Array[nindex].TC = NL->Array[nindex + 1].TC;
    }  /* END for (nindex) */
 
  return(SUCCESS);
  }  /* END MNLRemoveNode() */




/**
 * Count the number of nodes in a node list 
 *
 * @param NL (I)
*/
int MNLCount (

  const mnl_t     *NL)

  {
  int index;

  if (NL->Size <= 0)
    {
    return(0);
    }

  /* direct access for speed */

  for (index = 0;NL->Array[index].N != NULL;index++);

  return(index);
  }







/**
 * Copy specified sub-array of MNodeList into NodeList
 *
 * @param DstNL (O)
 * @param SrcNL (I)
 */

int MNLCopy(

  mnl_t        *DstNL,
  const mnl_t  *SrcNL)

  {
  int nindex;

  mnode_t *N;

  if (DstNL != NULL)
    MNLClear(DstNL);

  if ((DstNL == NULL) || (SrcNL == NULL))
    {
    return(FAILURE);
    }

  for (nindex = 0;MNLGetNodeAtIndex(SrcNL,nindex,&N) == SUCCESS;nindex++)
    {
    MNLSetNodeAtIndex(DstNL,nindex,N);
    MNLSetTCAtIndex(DstNL,nindex,MNLGetTCAtIndex(SrcNL,nindex));
    }  /* END for (nindex) */

  MNLTerminateAtIndex(DstNL,nindex);

  return(SUCCESS);
  }  /* END MNLCopy() */



/**
 * Copy the multi nodelist.
 *
 * NOTE: assumes both arrays are size MMAX_REQ_PER_JOB.
 *
 * @param DstMNL (O)
 * @param SrcMNL (I)
 */

int MNLMultiCopy(

  mnl_t      **DstMNL,
  mnl_t      **SrcMNL)

  {
  int nindex;
  int rqindex;

  if ((DstMNL == NULL) || (SrcMNL == NULL))
    {
    return(FAILURE);
    }

  for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
    {
    for (nindex = 0;SrcMNL[rqindex]->Array[nindex].N != NULL;nindex++)
      {
      MNLSetNodeAtIndex(DstMNL[rqindex],nindex,SrcMNL[rqindex]->Array[nindex].N);
      MNLSetTCAtIndex(DstMNL[rqindex],nindex,SrcMNL[rqindex]->Array[nindex].TC);
      }

    MNLTerminateAtIndex(DstMNL[rqindex],nindex);
    }  /* END for (index) */

  return(SUCCESS);
  }  /* END MNLMultiCopy() */





/**
 * Add specified node to nodelist
 *
 * @see MNLAddNode - controls conflicts and returns index into mnl_t. 
 *
 * @param NL (I) *modified,minsize=MMAX_NODE)
 * @param N (I)
 * @param TC (I) [optional 0 = not specified]
 */

int MNLAdd(

  mnl_t     *NL,  /* I *modified,minsize=MMAX_NODE) */
  mnode_t   *N,   /* I */
  int        TC)  /* I (optional 0 = not specified) */

  {
  int nindex;

  mnode_t *tmpN;

  int tmpTC;

  if ((NL == NULL) || (N == NULL))
    {
    return(FAILURE);
    }

  for (nindex = 0;MNLGetNodeAtIndex(NL,nindex,&tmpN) == SUCCESS;nindex++)
    {
    if (tmpN == N)
      {
      tmpTC = MNLGetTCAtIndex(NL,nindex);

      tmpTC += (TC > 0) ? TC : 0;

      MNLSetTCAtIndex(NL,nindex,tmpTC);

      return(SUCCESS);
      }
    }    /* END for (nindex) */

  MNLSetNodeAtIndex(NL,nindex,N);
  MNLSetTCAtIndex(NL,nindex,(TC > 0) ? TC : 1);

  nindex++;

  MNLTerminateAtIndex(NL,nindex);

  return(SUCCESS);
  }  /* END MNLAdd() */





/**
 * Remove a node from a nodelist
 *
 * @param NL (I) *modified,minsize=MMAX_NODE)
 * @param N (I)
 */

int MNLRemove(

  mnl_t     *NL,         /* I *modified,minsize=MMAX_NODE) */
  mnode_t   *N)          /* I */

  {
  int nindex;
  int offset = 0;

  mnode_t *N1;

  if ((NL == NULL) || (N == NULL))
    {
    return(FAILURE);
    }

  for (nindex = 0;MNLGetNodeAtIndex(NL,nindex,&N1) == SUCCESS;nindex++)
    {
    if ((N != NULL) && (N1 == N))
      {
      offset++;
      }
    else if (offset > 0)
      {
      MNLSetNodeAtIndex(NL,nindex - offset,N1);
      MNLSetTCAtIndex(NL,nindex - offset,MNLGetTCAtIndex(NL,nindex));
      }
    }    /* END for (nindex) */

  if (nindex - offset < MSched.M[mxoNode])
    {
    MNLTerminateAtIndex(NL,nindex - offset);
    }

  return(SUCCESS);
  }  /* END MNLRemove() */


/**
 * Convert task list into node list.
 *
 * NOTE:  may dynamically reallocate job nodelists
 * 
 * @see MUTLFromNL()
 *
 * @param J      (I) [optional] allow dynamic reallocation if specified
 * @param NL     (O)
 * @param TL     (I)
 * @param NCount (O) [optional]
 */

int MJobNLFromTL(

  mjob_t    *J,
  mnl_t     *NL,
  int       *TL,
  int       *NCount)

  {
  int tindex;
  int nindex;
  int NC = 0;

  mnode_t *N;

  const char *FName = "MJobNLFromTL";

  MDB(5,fCONFIG) MLog("%s(NL,TL,NCount)\n",
    FName);

  if (NCount != NULL)
    *NCount = 0;

  /* NOTE: this call is also used in "msub" where much of the job is not populated.  
           In "msub" J->NLSize is zero but NL is still valid, so set size equal to size 
           of JobMaxNodeCount, this could cause a problem if (J != NULL) and 
           (sizeof(NL) != MSched.JobMaxNodeCount */

  if ((NL == NULL) || (TL == NULL))
    {
    MDB(5,fCONFIG) MLog("INFO:     bad parameter in %s\n",
      FName);

    return(FAILURE);
    }

  MNLClear(NL);

  if (TL[0] == -1)
    {
    /* tasklist is empty */

    return(SUCCESS);
    }

  nindex = 0;

  for (tindex = 0;TL[tindex] != -1;tindex++)
    {
    for (nindex = 0;MNLGetNodeAtIndex(NL,nindex,&N) == SUCCESS;nindex++)
      {
      if (TL[tindex] == N->Index)
        {
        MNLAddTCAtIndex(NL,nindex,1);

        break;
        }
      }    /* END for (nindex) */

    if (MNLGetNodeAtIndex(NL,nindex,NULL) == FAILURE)
      {
      MNLSetNodeAtIndex(NL,nindex,MNode[TL[tindex]]);
      MNLSetTCAtIndex(NL,nindex,1);

      MNLTerminateAtIndex(NL,nindex + 1);

      NC = nindex + 1;
      }
    }     /* END for (tindex) */

  if (NCount != NULL)
    {
    *NCount = NC;
    }

  if (MNLIsEmpty(NL))
    {
    MDB(5,fCONFIG) MLog("INFO:     resulting nodelist is empty in %s\n",
      FName);
 
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MJobNLFromTL() */





/**
 * This function uses a nodelist to add entries to a tasklist starting at the requested tasklist location.
 *
 * TL points at the next unused slot within a tasklist array. This routine will store the node index of each 
 * record on the node list in tasklist (starting at TL) and it will also put a -1 after the last entry added 
 * to the tasklist to make sure that we have a -1 terminator at the end of the last valid entry in the tasklist array.
 *
 * @see also MJobNLFromTL()
 * @param NL     (Input)  Linked list of mnl_t records (each list entry has a pointer to a Node record)
 * @param TL     (Output) Starting location within a TaskList where we wish to add additional entries to the list
 * @param TLSize (Input)  The number of free entries available in the tasklist
 * @param TLCountPtr (Output) Location to store the count of entries added in the tasklist
 * @return                This function returns SUCCESS or FAILURE
 */

int MUTLFromNL(

  mnl_t     *NL,          /* I */
  int       *TL,          /* O */
  int        TLSize,      /* I */
  int       *TLCountPtr)  /* O (optional) */

  {
  int nindex;
  int tindex = 0;
  int tcindex;

  mnode_t *N;

  int TC;

  if ((NL == NULL) || (TL == NULL))
    {
    return(FAILURE);
    }

  for (nindex = 0;MNLGetNodeAtIndex(NL,nindex,&N) == SUCCESS;nindex++)
    {
    TC = MNLGetTCAtIndex(NL,nindex);

    if (TC <= 0)
      break;

    for (tcindex = 0;tcindex < TC;tcindex++)
      { 
      TL[tindex] = N->Index;

      tindex++;

      if (tindex >= TLSize)
        {
        /* tasklist buffer too small */

        TL[TLSize - 1] = -1;

        MDB(2,fSTRUCT)
          MLog("ALERT:    nodelist cannot be mapped to tasklist, task buffer too small (check JOBMAXTASKCOUNT)\n");

        return(FAILURE);
        }
      }    /* END for (tcindex) */
    }      /* END for (nindex) */

  TL[tindex] = -1;

  if (TLCountPtr != NULL)
    *TLCountPtr = tindex;

  return(SUCCESS);
  }  /* END MUTLFromNL() */





/**
 * This function puts the node index for the specified nodeid at the requested tasklist location.
 *
 * TL points at the next unused slot within a tasklist. This routine will fill in that slot with a 
 * node index and it will also put a -1 in the next slot to make sure that we have a -1 terminator
 * after the last valid index in the tasklist array.
 *
 * @param NodeId (I) Nodeid string (e.g. odev14 )
 * @param TL     (O) Pointer to the location within a TaskList where we wish to add the next entry
 * @param TLSize (I) The number of free entries available in the tasklist
 *
 * @return                This function returns SUCCESS or FAILURE
 */

int MUTLFromNodeId(

  char  *NodeId,      /* I */
  int   *TL,          /* O */
  int    TLSize)      /* I */

  {
  mnode_t *N;

  if ((NodeId == NULL) || (TL == NULL))
    {
    return(FAILURE);
    }

  /* Check to make sure that tasklist array has room for another entry */

  if (TLSize <= 0)
    {
    MDB(2,fWIKI)
      MLog("ALERT:    nodelist cannot be mapped to tasklist, task buffer too small (check JOBMAXTASKCOUNT)\n");

    return(FAILURE);
    }

  /* Look up the node so that we can get the node index to store in the tasklist */

  if (MNodeFind(NodeId,&N) == FAILURE)
    {
    /* If we couldn't find the node then we can't get the node index */

    if (N == NULL)
      {
      MDB(1,fWIKI) MLog("ALERT:    cannot locate host '%s' for hostlist\n",
        NodeId);

      return(FAILURE);
      }
    }    /* END if (MNodeFind(ptr,&N) == FAILURE) */

  /* add task to list */

  TL[0] = N->Index;
  TL[1] = -1;       /* make sure that there is a -1 in the tasklist after the last valid entry */

  return(SUCCESS);
  } /*END MUTLFromNodeId() */



int MUJobNLGetReqIndex(
  
  mjob_t    *J,
  mnode_t   *N,
  int       *RIndex)

  {
  int nindex;
  int rqindex;

  mnode_t *tmpN;

  if ((J == NULL) || (N == NULL) || (RIndex == NULL))
    {
    return(FAILURE);
    }

  *RIndex = -1;

  for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
    {
    for (nindex = 0;MNLGetNodeAtIndex(&J->Req[rqindex]->NodeList,nindex,&tmpN) == SUCCESS;nindex++)
      {
      if (tmpN == N)
        {
        *RIndex = rqindex;

        return(SUCCESS);
        }
      }
    }   /* END for (rqindex) */

  return(FAILURE);
  }  /* END MUJobNLGetReqIndex() */





/**
 * Merge Secondary into Primary, resolving conflicts with ConflictResolver.
 *
 * @param Primary (I) modified
 * @param Secondary (I)
 * @param IncrTC (I) Sums secondary and primary taskcounts when applicable.
 */

int MNLMerge2(

  mnl_t     *Primary,
  mnl_t     *Secondary,
  mbool_t    IncrTC)

  {
  int index;
  int rc;
  int FoundIndex;

  mnode_t *N;
  int      TC;

  for (index = 0;MNLGetNodeAtIndex(Secondary,index,&N) == SUCCESS;index++)
    {
    TC = MNLGetTCAtIndex(Secondary,index);

    rc = MNLAddNode(Primary,N,TC,IncrTC,&FoundIndex);

    if (rc == FAILURE)
      {
      MDB(3,fSTRUCT) MLog("ALERT:    MNLMerge2 failed because of lack of space\n");

      return(FAILURE);
      }
    }

  return(SUCCESS);
  }  /* END MNLMerge2() */





/**
 * Merge two node lists.
 *
 * @param ONL (O) [modified]
 * @param NList1 (I)
 * @param NList2 (I)
 * @param TCP (O) [optional]
 * @param NCP (O) [optional]
 */

int MNLMerge(

  mnl_t  *ONL,    /* O (modified) */
  mnl_t  *NList1, /* I */
  mnl_t  *NList2, /* I */
  int    *TCP,    /* O (optional) */
  int    *NCP)    /* O (optional) */

  {
  int nindex;

  mnode_t *N;

  if ((ONL != NULL) && (ONL != NList1))
    MNLClear(ONL);
  
  if (TCP != NULL)
    *TCP = 0;

  if (NCP != NULL)
    *NCP = 0;

  if ((ONL == NULL) || (NList1 == NULL) || (NList2 == NULL))
    {
    return(FAILURE);
    }

  if (ONL != NList1)
    {
    MNLCopy(ONL,NList1);
    }

  for (nindex = 0;MNLGetNodeAtIndex(NList2,nindex,&N) == SUCCESS;nindex++)
    {
    MNLAddNode(ONL,N,MNLGetTCAtIndex(NList2,nindex),TRUE,NULL);
    }

  if (TCP != NULL)
    *TCP = MNLSumTC(ONL);

  if (NCP != NULL)
    *NCP = MNLCount(ONL);

  return(SUCCESS);
  }  /* END MNLMerge() */



/**
 *
 *
 * @param A (I)
 * @param B (I)
 */

int __MSchedQSortLHPComp(

  mnpri_t *A,  /* I */
  mnpri_t *B)  /* I */

  {
  /* order low to high */

  if (A->Prio > B->Prio)
    {
    return(1);
    }
  else if (A->Prio < B->Prio)
    {
    return(-1);
    }

  return(0);
  }  /* END __MSchedQSortLHPComp() */




/**
 *
 *
 * @param A (I)
 * @param B (I)
 */

int __MSchedQSortHLPComp(

  mnpri_t *A,  /* I */
  mnpri_t *B)  /* I */

  {
  /* order hi to low */

  if (A->Prio > B->Prio)
    {
    return(-1);
    }
  else if (A->Prio < B->Prio)
    {
    return(1);
    }

  return(0);
  }  /* END __MSchedQSortHLPComp() */




/**
 * Sorts the given nodelist according to the node allocation priority.
 *
 * @see MJobAllocatePriority() - peer
 * @see MNLCopy() - peer - copy full nodelist to destination
 * @see MNodeGetPriority() - child
 *
 * @param NL (I)
 * @param J (I) [optional]
 * @param StartTime (I)
 * @param Policy (I)
 * @param SpecP (I) [optional]
 * @param SortLowToHigh (I)
 */

int MNLSort(

  mnl_t                     *NL,
  const mjob_t              *J,
  mulong                     StartTime,
  enum MNodeAllocPolicyEnum  Policy,
  mnprio_t                  *SpecP,
  mbool_t                    SortLowToHigh)

  {
  int    NIndex;
  int    nindex;

  mnode_t *N;

  mbool_t  IsPref=FALSE;

  static mnpri_t *NodePrio = NULL;

  if (NL == NULL)
    {
    return(FAILURE);
    }

  if ((NodePrio == NULL) &&
     ((NodePrio = (mnpri_t *)MUCalloc(1,sizeof(mnpri_t) * (MSched.M[mxoNode] + 1))) == NULL))
    {
    MDB(3,fSCHED) MLog("ALERT:    cannot allocate memory for nodelist\n");

    return(FAILURE);
    }

  NIndex = 0;
 
  switch (Policy)
    {
    case mnalNONE:
    case mnalMachinePrio:
    case mnalCustomPriority:

      for (nindex = 0;MNLGetNodeAtIndex(NL,nindex,&N) == SUCCESS;nindex++)
        {
        /* N is pointer in node list */
        /* taskcount is node's priority    */
     
        NodePrio[NIndex].N = N;           
        NodePrio[NIndex].TC = MNLGetTCAtIndex(NL,nindex);
     
        if ((Policy == mnalMachinePrio) ||
            (Policy == mnalCustomPriority))
          {
          if (J != NULL)
            MReqGetPref(J->Req[0],N,&IsPref);
          else
            IsPref = FALSE;

          MNodeGetPriority(
            N,
            J,
            (J != NULL) ? J->Req[0] : NULL,
            SpecP,
            1,
            IsPref,
            &NodePrio[NIndex].Prio,
            StartTime,
            NULL);
          }
        else
          {
          NodePrio[NIndex].Prio = (double)N->NodeIndex * -1;
          }
     
        MDB(7,fSCHED) MLog("INFO:     node '%s' priority '%.2f'\n",
          N->Name,
          NodePrio[NIndex].Prio);
     
        NIndex++;
     
        if (NIndex >= MSched.M[mxoNode])
          break;
        }  /* END for (nindex) */
     
      NodePrio[NIndex].N = NULL;
     
      if (NIndex == 0)
        {
        return(FAILURE);
        }

      break;

    default:

      /* NYI */

      return(FAILURE);

      /*NOTREACHED*/

      break;
    }
 
  /* HACK:  machine prio and fastest first both use same qsort algo */

  if (NIndex > 1)
    {
    qsort(
      (void *)&NodePrio[0],
      NIndex,
      sizeof(mnpri_t),
      (SortLowToHigh == TRUE) ?
        (int(*)(const void *,const void *))__MSchedQSortLHPComp :
        (int(*)(const void *,const void *))__MSchedQSortHLPComp);
    }

  for (nindex = 0;NodePrio[nindex].N != NULL;nindex++)
    {
    MDB(7,fSTRUCT) MLog("INFO:     Sorted NL[%d] %s:%d (%.2f)\n",
      nindex,
      NodePrio[nindex].N->Name,
      NodePrio[nindex].TC,
      NodePrio[nindex].Prio);

    MNLSetNodeAtIndex(NL,nindex,NodePrio[nindex].N);
    MNLSetTCAtIndex(NL,nindex,NodePrio[nindex].TC);
    }

  MNLTerminateAtIndex(NL,nindex);

  return(SUCCESS);
  }  /* END MNLSort() */




/* END  MNL.c */
