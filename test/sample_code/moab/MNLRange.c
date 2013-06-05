/* HEADER */

      
/**
 * @file MNLRange.c
 *
 * Contains: Node List Range functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  




/**
 * Convert a nodelist (or keylist) into a buffer with a hostlist range
 *
 * NOTE: Key<0 NYI 
 *
 * @see MUNLFromRangeString() - peer
 *
 * @param NL (I) [optional]
 * @param KeyList (I) [optional]
 * @param Key (I) [==0 ignore Key, <0 break range on Key changes, >0 filter by Key]
 * @param ShowKey (I)
 * @param DoSort (I) NodeList will be sorted before before compressing in order
 *                   to get the greatest compression. If DoSort is false then 
 *                   only ascending nodes will be compressed to a range. For ex.
 *                   517,516 will not be compressed to 516-517. Compressing
 *                   descending nodes can cause issues for some who expect 
 *                   node ordering to be actual node ordering.
 * @param Buf (O)
 *
 * @return   This function returns SUCCESS or FAILURE
 */

int MUNLToRangeString(

  mnl_t     *NL,       /* I node list (optional) */
  char      *KeyList,  /* I key list for node indicies (optional) */
  int        Key,      /* I (==0 ignore Key, <0 break range on Key changes, >0 filter by Key) */
  mbool_t    ShowKey,  /* I */
  mbool_t    DoSort,   /* I */
  mstring_t *Buf)      /* O */

  {
  int nindex;
  int MatchCount;  /* nodename character index where node indices should be compared */

  int StartIndex = 0;
  int EndIndex = 0;
  int StartIndexLen = 0;
  int EndIndexLen = 0;
  int PreviousIndex = 0;
  int NIndex;

  int Direction; /* < 0 = descending, 0 = undefined, > 0 = ascending */

  char *StartTail = NULL;

  char *tmpBuf = NULL;

  mnode_t *N = NULL;
  mnode_t *NPrevious = NULL;
  mnode_t *tmpN = NULL;

  int      StartTC = 0;
  int      NTC = 0;
  mnode_t *StartN;

  char HostListElement[MMAX_LINE];

  mnl_t tmpNL = {0}; /* copy of NL or MNode[] */

  if (((KeyList == NULL) && (NL == NULL)) || (Buf == NULL))
    {
    return(FAILURE);
    }

  if (NL != NULL)
    {
    MNLInit(&tmpNL);
    /* Copy NL and sort if necessary. */

    MNLCopy(&tmpNL,NL);

    if (DoSort == TRUE)
      {
      MUNLSortLex(&tmpNL,MNLCount(&tmpNL));
      }
    }

  /* FORMAT: <PREFIX><INDEX><SUFFIX>  (e.g.  odev14 or odev14abc) */

  StartN = NULL;
  MatchCount = -1;

  for (nindex = 0;nindex < MSched.M[mxoNode];nindex++)
    {
    NPrevious = N;
 
    if (NL != NULL)
      MNLGetNodeAtIndex(&tmpNL,nindex,&N);
    else
      N = MNode[nindex];

    if ((N == NULL) || (N->Name[0] == '\0'))
      break;

    if (N->Name[1] == '\1')
      continue;

    if (Key > 0)
      {
      if (KeyList != NULL) 
        {
        if (KeyList[nindex] != Key)
          continue;
        }
      else if (MNLGetTCAtIndex(&tmpNL,nindex) != Key)
        {
        continue;
        }
      }    /* END if (Key > 0) */

    if (StartN == NULL)
      {
      /* new range start detected */

      StartN = N;
      StartTC = (NL != NULL) ? MNLGetTCAtIndex(&tmpNL,nindex) : Key;

      /* Go get the next node */

      continue;
      }

    NTC = (NL != NULL) ? MNLGetTCAtIndex(&tmpNL,nindex) : Key;

    if (MatchCount == -1)
      {
      /* determine if range has started */

      Direction = 0; /* make sure that direction is not specified so that direction will be filled in */

      if ((MUNodeNameAdjacent(StartN->Name, N->Name, &StartIndex, &EndIndex, &Direction) == SUCCESS) &&
          ((DoSort == TRUE) || (Direction > 0)) &&
          ((ShowKey == TRUE) ? (StartTC == NTC) : 1))
        {
        int EndIndexPrefix;

        /* Length of the node name prefix which proceed node number digits at the end of the string 
         * and the length of the index digits at the end of the node name. */

        MUNodeNameNumberOffset(StartN->Name, &MatchCount, &StartIndexLen);

        /* Get the length of the potential end index */

        MUNodeNameNumberOffset(N->Name, &EndIndexPrefix, &EndIndexLen);

        /* Get a pointer to the node name suffix after the digits */
#ifdef __MNOT
        /* We currently do not support suffixes */

        StartTail = StartN->Name + MatchCount;  /* Set StartTail to the start of the digits */
        StartTail +=  strspn(StartTail,MDEF_DIGITCHARLIST); /* Move StartTail after the digits */
#endif
        }
      else
        {
        /* second node does not match range start node, 
           flush first node to buffer, start new range and continue */
      
        if ((MNodeFind(StartN->Name,&tmpN)) && 
            (MPar[tmpN->PtIndex].RM != NULL) && 
            (MPar[tmpN->PtIndex].RM->IsBluegene)) 
          {
          MUBuildBGLHostListElement(
              StartN->Name,
              NULL,
              HostListElement);
          }
        else 
          {
          MUBuildHostListElement(StartN->Name,
              -1,-1,-1,-1,-1,NULL,
              (ShowKey == FALSE) ? -1 : StartTC,
              HostListElement);
          }

        MStringAppendF(Buf,"%s%s",
            (!Buf->empty()) ? ":" : "",
            HostListElement);

        StartN = N;
        StartTC = (NL != NULL) ? MNLGetTCAtIndex(&tmpNL,nindex) : Key;

        }  /* END if MUNodeNameAdjacent() != SUCCESS */

      continue;
      }    /* END if (MatchCount == -1) */

    /* See if this node fits into the range that has been started by checking to see if it is adjacent to
     * the previous node added to this range. Note that direction was set when we started the range */

    if ((MUNodeNameAdjacent(NPrevious->Name, N->Name, &PreviousIndex, &NIndex, &Direction) == SUCCESS) &&
        ((DoSort == TRUE) || (Direction > 0)) &&
        ((ShowKey == TRUE) ? (StartTC == NTC) : 1))
      {
      int EndIndexPrefix;

      /* extend range */

      /* Get the length of the end index (e.g. odev04 would have an index length of 2 for the "04") */

      MUNodeNameNumberOffset(N->Name,&EndIndexPrefix,&EndIndexLen);

      EndIndex = NIndex;
      }
    else
      {
      /* flush current range and start a new one */

      if ((MNodeFind(StartN->Name,&tmpN)) && 
          (MPar[tmpN->PtIndex].RM != NULL) && 
          (MPar[tmpN->PtIndex].RM->IsBluegene)) 
        {
        MUBuildBGLHostListElement(
            StartN->Name,
            NPrevious->Name,
            HostListElement);
        }
      else 
        {
        MUBuildHostListElement(
            StartN->Name,
            MatchCount,
            StartIndexLen,
            StartIndex,
            EndIndexLen,
            EndIndex,
            (StartTail != NULL) ? StartTail : (char *)"",
            (ShowKey == FALSE) ? -1 : StartTC,
            HostListElement);
        }

      MStringAppendF(Buf,"%s%s",
          (!Buf->empty()) ? ":" : "",
          HostListElement);

      /* start a new range with this node */

      StartN = N;
      StartTC = (NL != NULL) ? MNLGetTCAtIndex(&tmpNL,nindex) : Key;

      MatchCount = -1;
      }  /* END if (strncmp(StartN->Name,N->Name,MatchCount) || ...) */ 
    }    /* END for (nindex) */

  /* flush final range */

  if (MatchCount >= 0)
    {
    /* Check if node is a Bluegene node. Prefix is length of the node name -3 because 
      * bg/l nodes are define in 3 dimensions */

    if ((MNodeFind(StartN->Name,&tmpN)) && 
        (MPar[tmpN->PtIndex].RM != NULL) && 
        (MPar[tmpN->PtIndex].RM->IsBluegene)) 
      {
      MUBuildBGLHostListElement(
          StartN->Name,
          NPrevious->Name,
          HostListElement);
      }
    else
      {
      /* NOTE:  MatchCount only specified if non-numeric prefix is specified */

      MUBuildHostListElement(
        StartN->Name,
        MatchCount,
        StartIndexLen,
        StartIndex,
        EndIndexLen,
        EndIndex,
        (StartTail != NULL) ? StartTail : (char *)"",
        (ShowKey == FALSE) ? -1 : StartTC,
        HostListElement);
      }

    MStringAppendF(Buf,"%s%s",
      (!Buf->empty()) ? ":" : "",
      HostListElement);
    }    /* END if (MatchCount > 0) */
  else if (StartN != NULL)
    {
    /* single node 'range' detected */

    if ((MNodeFind(StartN->Name,&tmpN)) && 
        (MPar[tmpN->PtIndex].RM != NULL) && 
        (MPar[tmpN->PtIndex].RM->IsBluegene)) 
      {
      MUBuildBGLHostListElement(
          StartN->Name,
          NPrevious->Name,
          HostListElement);
      }
    else
      {
      MUBuildHostListElement(StartN->Name, 
        -1,-1,-1,-1,-1,NULL,
        (ShowKey == FALSE) ? -1 : StartTC,
        HostListElement);
      }

    MStringAppendF(Buf,"%s%s",
      (!Buf->empty()) ? ":" : "",
      HostListElement);
    }    /* END else if (StartN != NULL) */

  if (MUStrDup(&tmpBuf,Buf->c_str()) == SUCCESS)
    {
    MUCollapseHostListRanges(tmpBuf);

    MStringSetF(Buf,"%s",tmpBuf); /* Overwrite Buf with compressed hostlist. */

    MUFree(&tmpBuf);
    }

  if (NL != NULL)
    MNLFree(&tmpNL);

  return(SUCCESS);
  }  /* END MUNLToRangeString() */



/**
 * Sorts a given node list lexicographically by name.
 *
 * @see MNLSort() - sorts by Machine Priority or NodeIndex.
 *
 * @param NL     (I/O, modified) The nodelist to sort.
 * @param NLSize (I) The number of nodes in the list.
 */
int MUNLSortLex(

  mnl_t     *NL,
  mulong     NLSize)

  {
  mnalloc_old_t *NodeList = NULL;

  NodeList = (mnalloc_old_t *)MUCalloc(1,sizeof(mnalloc_old_t) * MSched.M[mxoNode]);

  MNLToOld(NL,NodeList);

  if (MUStrHasAlpha(NodeList[0].N->Name) == TRUE)
    {
    qsort(
        (void*)NodeList,
        NLSize,
        sizeof(NodeList[0]),
        (int(*)(const void *,const void *))__MNodeCompareLex);
    }
  else
    {
    qsort(
        (void*)NodeList,
        NLSize,
        sizeof(NodeList[0]),
        (int(*)(const void *,const void *))__MNodeCompareIndex);
    }

  MUFree((char **)&NodeList);

  return(SUCCESS);
  }









/**
 * Take a node range expression and return a nodelist or nodeidlist
 *
 * NOTE:  RangeString can be very large, > 16K
 *
 * @see MUNLToRangeString() - peer
 * @see MUParseRangeString() - child
 *
 * @param RangeString (I) string with a node range expression [modified as side-affect] 
 * @param SDelim      (O) string with alternate delimeters (default is whitespace and comma if NULL) [optional] 
 * @param NL          (O) array of mnl_t with resulting nodelist [optional,minsize=MMAX_NODE]
 * @param NodeID      (O) array of nodeids [optional,minsize=MMAX_NODE]
 * @param StartIndex  (I) used only when CreateNode==TRUE [optional]
 * @param DefTC       (I) if -1 set TC to configured procs, if >= 0 apply from given TC [optional] only used with Bluegene
 * @param CreateNode  (I,TRUE/FALSE) create node if node not present 
 * @param IsBluegene  (I) Parse bluegene range string or not
 */

int MUNLFromRangeString(

  char       *RangeString,
  const char *SDelim,
  mnl_t      *NL,
  char        NodeID[][MMAX_NAME],
  int         StartIndex,
  int         DefTC,
  mbool_t     CreateNode,
  mbool_t     IsBluegene)

  {
  int NListIndex = 0;
  int globalNLIndex = 0;
  int NCount;
  int rc;

  int   *NNumberList = NULL;
  int   *ZPadLen = NULL; /* zero pad length for each node - if no padding this value is 0 */

  char **NCharList= NULL;  /* make dynamic */
                           /* BG could have more than MMAX_NODE, 36^3 = 46656*/

  char *ptr;
  char *TokPtr;

  char *head;
  char *tail;
  char *tptr;
  
  char *pre = NULL;

  char Delim[MMAX_NAME];

  char tmpName[MMAX_NAME];

  int  TC = 0;     /* task instance count */

  tmpName[0] = '\0';

  if (NL != NULL)
    MNLClear(NL);

  if ((RangeString == NULL) ||
      (RangeString[0] == '\0') ||
     ((NL == NULL) && (NodeID == NULL)))
    {
    return(FAILURE);
    }

  if (SDelim != NULL)
    {
    MUStrCpy(Delim,SDelim,sizeof(Delim));
    }
  else
    {
    /* default is whitespace and comma */
    /* NOTE:  see special comma handling below */

    strcpy(Delim," ,\t\n"); 
    }

  NNumberList = (int *)MUCalloc(1,sizeof(int) * MSched.M[mxoNode]);
  ZPadLen     = (int *)MUCalloc(1,sizeof(int) * MSched.M[mxoNode]);
  NCharList   = (char **)MUCalloc(1,sizeof(char *) * MSched.M[mxoNode]);

  /* FORMAT:  <PRE>'['<RANGESTRING>']'[*<TC>][<DELIM><PRE>'['<RANGESTRING>']'][*<TC>]... */

  /* FORMAT:  <RANGESTRING> -> <STARTINDEX>[-<ENDINDEX>][,<STARTINDEX>[-<ENDINDEX>]]... */
  /*   NOTE:  see MUParseRangeString() for details */

  /* NOTE:  support 'node[01-05]' and 'mdev[1,3-15]' */

  /* NOTE:  comma inside of range string must be ignored at this level, ie
            tokenize on comma in 'node[01-03],nodeB[13-22]' but not in
            'node[01-03,13-22]' */

  ptr = MUStrTokEArray(RangeString,Delim,&TokPtr,FALSE);

  /* if comma located before first '[' or after first ']', tokenize */

  while (ptr != NULL)
    {
    NListIndex = 0;

    /* extract range */

    head = strchr(ptr,'[');
    tail = strchr(ptr,']');

    tptr = strchr(ptr,'*'); /* task count delimiter */

    TC   = 1;

    NCount = 0;

    if ((head != NULL) && (tail != NULL))
      {
      /* range specified */

      *head = '\0';

      pre = ptr;

      *tail = '\0';

      if ((tptr != NULL) && (tptr > tail))
        {
        /* taskcount specified */

        TC = (int)strtol(tptr + 1,NULL,10);
 
        *tptr = '\0';
        }

      /* Put node numbers extracted from range in NNumberList array */
      /* If slurm and bluegene put node indexs in NCharList array */
     
      if (IsBluegene == TRUE)
        {
        /* parse bluegene ranges bgl[000x234,236x733] */

        rc = MUParseBGRangeString(
            head + 1,
            NCharList,
            MSched.M[mxoNode] - NListIndex, 
            &NCount);
        }
      else
        {
        rc = MUParseRangeString(
            head + 1,
            NULL,
            &NNumberList[NListIndex],   /* O */
            MSched.M[mxoNode] - NListIndex, 
            &NCount,                    /* O */
            &ZPadLen[NListIndex]);
        }

      if (rc == SUCCESS)
        NListIndex += NCount;
      }    /* END if ((head != NULL) && (tail != NULL)) */
    else
      {
      /* non-range specified */

      if (tptr != NULL)
        {
        /* taskcount specified */

        TC = (int)strtol(tptr + 1,NULL,10);
 
        *tptr = '\0';
        }

      /* check to see if the end of the node name is a node number e.g. sc15bsc10 */

      head = ptr;
      tail = &ptr[strlen(ptr) -1];

      /* Start at the end and back up until we hit a non digit */

      while ((tail >= ptr) && isdigit(*tail))
        tail--;

      tail++; /* we decremented one beyond the last digit, so move back up to the first digit,
                 or to the NULL at the end of the string if we didn't find a digit at the end. 
                 If the node name was all digits, then tail is now pointing at the first digit
                 after the increment. */ 

      if ((*tail != '\0') && isdigit(*tail))
        {
        /* node number found at the end of the node string */

        MUStrCpy(tmpName,ptr,sizeof(tmpName));

        tmpName[tail - ptr] = '\0';

        pre = tmpName;

        ZPadLen[NListIndex] = strlen(tail);

        if (IsBluegene == TRUE)
          {
          rc = MUParseBGRangeString(
            tail,
            NCharList,
            MSched.M[mxoNode] - NListIndex, 
            &NCount);

          if (rc == SUCCESS)
            NListIndex += NCount;
          } 
        else
          {
          NNumberList[NListIndex] = (int)strtol(tail,NULL,10);
          NCount = 1;
          NListIndex++;
          }
        }
      else
        {
        /* non-range, non-digit node name e.g. odevb */

        /* Note that with compressed mode we currently do not support ranges and non-numeric node names
         * mixed together in the same TASKLIST odev[10-11];odevb */

        NCount = 0;

        MDB(5,fWIKI) MLog("ALERT:    Cannot include non-numeric host name with hostlist ranges '%s'\n",
          ptr);
        }
      }
  
    NCount = NListIndex;

    /* if bluegene parse NCharList */

    if(IsBluegene == TRUE)
      {
      int      NIndex;
      char     tmpLine[MMAX_NAME];
      mnode_t *tN;

      for (NListIndex = 0; NListIndex < NCount;NListIndex++)
        {
        NIndex = -1;
        
        /* build node name */
        sprintf(tmpLine,"%s%s",pre,NCharList[NListIndex]);

        if (MNodeFind(tmpLine,&tN))
          {
          NIndex = tN->Index;
          }
        else if (CreateNode)
          {
          if (MNodeAdd(tmpLine,&tN) == FAILURE)
            {
            MUFree((char **)&NNumberList);
            MUFree((char **)&ZPadLen);
            MUFree((char **)&NCharList);

            return(FAILURE);
            }

          NIndex = tN->Index;
          }
          
        /* Add node to NL to pass back */
        if (NL != NULL && NIndex > -1)
          {
          MNLSetNodeAtIndex(NL,globalNLIndex,MNode[NIndex]);
          
          if (DefTC >=0)
            MNLSetTCAtIndex(NL,globalNLIndex,TC);
          else
            MNLSetTCAtIndex(NL,globalNLIndex,tN->CRes.Procs);
          }

        /* Add node id to NodeID to pass back */
        if (NodeID != NULL)
          {
          MUStrCpy(NodeID[globalNLIndex],tmpLine,sizeof(NodeID[globalNLIndex]));
          }

        globalNLIndex++;
        } /* END for (NListIndex */

      /* Free NCharList */

      for (NListIndex = 0;NListIndex < NCount;NListIndex++)
        {
        MUFree(&NCharList[NListIndex]);
        }
      }
    else 
      {
      for (NListIndex = 0;NListIndex < NCount;NListIndex++)
        {
        int NodeNumber = NNumberList[NListIndex];
        int NodeArrayIndex = -1;

        if (NodeNumber == -1)
          {
          if (NodeID != NULL)
            MUStrCpy(NodeID[globalNLIndex++],tmpName,sizeof(NodeID[0]));

          continue;
          }

        if ((NodeNumber >= ((MSched.M[mxoNode] <<2) + MMAX_NHBUF)) || (NodeNumber < 0))
          {
          MUFree((char **)&NNumberList);
          MUFree((char **)&ZPadLen);
          MUFree((char **)&NCharList);

          /* NodeNumber exceeds the size of the MNodeIndex array or is an illegal array index value */

          return(FAILURE);
          }

        NodeArrayIndex = MNodeIndex[NodeNumber];

        if (NodeArrayIndex >= (MSched.M[mxoNode] + 1))
          {
          MUFree((char **)&NNumberList);
          MUFree((char **)&ZPadLen);
          MUFree((char **)&NCharList);

          /* Make sure that NodeArrayIndex does not run off the end of the MNode array. 
            Note that -1 is OK since we will change it before we use it as an array index in that case. */

          return(FAILURE);
          }

        /* If the Node Index does not match the Node Number then we have to look up the node */

        if ((NodeArrayIndex == -1) ||              /* Not found in the MNodeIndex array by node number */
            (MNode[NodeArrayIndex] == NULL) ||     /* Not found in the MNode array using the MNodeIndex by Node Number */ 
            (MNode[NodeArrayIndex]->NodeIndex != NodeNumber) || /* Entry found in the MNode array but index and number do not match */
            ((pre != NULL) &&
             (strncmp(MNode[NodeArrayIndex]->Name,pre,strlen(pre))))) /* prefix doesn't match prefix of Node at index */
          {
          char tmpLine[MMAX_NAME];
          mnode_t *tN;

          /* specified range index does not match with node index */

          /* Build the node name with the character prefix and the node number (e.g. odev14) */

          snprintf(tmpLine,sizeof(tmpLine),"%s%0*d",
            (pre != NULL) ? pre : "",
            ZPadLen[NListIndex],
            NNumberList[NListIndex]);

          if (CreateNode == TRUE)
            {
            if (MNodeAdd(tmpLine,&tN) == FAILURE)
              {
              MUFree((char **)&NNumberList);
              MUFree((char **)&ZPadLen);
              MUFree((char **)&NCharList);

              return(FAILURE);
              }
            }
          else
            {
            if (MNodeFind(tmpLine,&tN) == FAILURE)
              {
              char HostName[MMAX_NAME];

              /* try full hostname */

              sprintf(HostName,"%s.%s",
                tmpLine,
                MSched.DefaultDomain);

              if (MNodeFind(HostName,&tN) != SUCCESS)
                {
                MUFree((char **)&NNumberList);
                MUFree((char **)&ZPadLen);
                MUFree((char **)&NCharList);

                return(FAILURE);
                }
              }
            }

          /* Use the Index from the node record since the Node Index and Node Number do not match */

          NodeArrayIndex = tN->Index;
          }

        if (NL != NULL)
          {
          MNLSetNodeAtIndex(NL,globalNLIndex,MNode[NodeArrayIndex]);

          if (DefTC >=0)
            {
            MNLSetTCAtIndex(NL,NListIndex,TC);
            }
          else
            {
            /* get the correct task count for ttc and ppn requests */

            MNLSetTCAtIndex(NL,NListIndex,MNode[NodeArrayIndex]->CRes.Procs);
            }
          }

        /* NOTE:  prefix only supported - no suffix support */

        if (NodeID != NULL)
          {
          if (ZPadLen[NListIndex] > 0)
            {
            snprintf(NodeID[globalNLIndex],sizeof(NodeID[globalNLIndex]),"%s%0*d",
              (pre != NULL) ? pre : "",
              ZPadLen[NListIndex],
              NNumberList[NListIndex]);
            }
          else
            {
            snprintf(NodeID[globalNLIndex],sizeof(NodeID[globalNLIndex]),"%s%d",
              (pre != NULL) ? pre : "",
              NNumberList[NListIndex]);
            }
          }

        globalNLIndex++;
        }  /* END for (NListIndex) */
      } /* END if else bluegene */

    if (NCount >= MSched.M[mxoNode])
      break;
   
    ptr = MUStrTokEArray(NULL,Delim,&TokPtr,FALSE);
    }  /* END while (ptr != NULL) */

  MUFree((char **)&NNumberList);
  MUFree((char **)&ZPadLen);
  MUFree((char **)&NCharList);

  if (NL != NULL)
    MNLTerminateAtIndex(NL,globalNLIndex);
 
  if (NodeID != NULL)
    NodeID[globalNLIndex][0] = '\0';
 
  return(SUCCESS);
  }  /* END MUNLFromRangeString() */

/* END MNLRange.c */
