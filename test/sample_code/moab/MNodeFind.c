/* HEADER */

      
/**
 * @file MNodeFind.c
 *
 * Contains: MNodeFind
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  

 
/**
 * Locate specified node by name.
 *
 * NOTE: set NP to correct structure if found, otherwise *NP is set to NULL.
 *
 * NOTE: as of 5.3, compare is case insensitive
 *
 * @see MNodeIndex[] - node hash table
 * @see MNodeAdd() - peer
 *
 * @param NName (I) node id
 * @param NP (O) [optional]
 */

int MNodeFind(
 
  const char   *NName,
  mnode_t     **NP)
 
  {
  int  nindex;
  int  len;
  int  rc;
  int *IndexPtr;
 
  char *ptr;
 
  char QHostName[MMAX_NAME];
  char LHostName[MMAX_NAME];
  char QNetwork[MMAX_NAME];
  char LNetwork[MMAX_NAME];

  mnode_t *N;

#ifdef __MPROFB
  int        nodeSearchCount = 0;     /* number of nodes searched until success on this iteration */
  static int NodeSearchCountTot = 0;  /* Total Nodes searched during program execution */
  static int CalledCount = 0;         /* number of times MNodeFind() called */
#endif /* __MPROFB */

  const char *FName = "MNodeFind";

  MDB(8,fSTRUCT) MLog("%s(%s,NP)\n",
    FName,
    (NName != NULL) ? NName : "NULL");
 
  if (NP != NULL)
    *NP = NULL;
 
  if ((NName == NULL) || (NName[0] == '\0'))
    {
    return(FAILURE);
    }

  if ((NName[0] == 'D') && (!strcmp(NName,"DEFAULT")))
    {
    if (NP != NULL)
      *NP = &MSched.DefaultN;
 
    if (MSched.DefaultN.Name[0] == '\0')
      strcpy(MSched.DefaultN.Name,"DEFAULT");
 
    return(SUCCESS);
    }

#ifdef __MPROFB
  CalledCount++;
#endif /* __MPROFP */

  if (MSched.NodeNameCaseInsensitive == TRUE)
    rc = MUHTGetCI(&MNodeIdHT,NName,(void **)&IndexPtr,NULL);
  else
    rc = MUHTGet(&MNodeIdHT,NName,(void **)&IndexPtr,NULL);

  if ((rc == SUCCESS) &&
      (*IndexPtr <= MSched.M[mxoNode]))
    {
    N = MNode[*IndexPtr];

    if ((N != NULL) && (!strcmp(N->Name,NName)))
      {
      if (NP != NULL)
        *NP = N;

      return(SUCCESS);
      }
    else
      {
      MDB(0,fSTRUCT) MLog("ALERT:    node hash table returned incorrect node (%s != %s)--possible corruption\n",
        N->Name,
        NName);
      }
    }
  else
    {
    MDB(7,fSTRUCT) MLog("INFO:     node %s not found in node hash table\n",
      NName);

    return(FAILURE);
    }

  /* WARNING: the below code is essentially dead-code if hash table is in use.
   * Remove code once we know it is safe. (Sept. 2, 2008 - JMB) */

  if (MSched.UseNodeIndex != FALSE)
    {
    int nindex;
    int index;

    /* require direct mapping of node name to node index */

    /* NOTE:  accept decimal or hex node names */

    if (isdigit(NName[0]))
      {
      nindex = (int)strtol(NName,NULL,0) % (MSched.M[mxoNode] << 2);
      }
    else
      {
      nindex = MUGetHash(NName) % (MMAX_NHBUF) + (MSched.M[mxoNode] << 2);
      }

    for (index = nindex;index < nindex + MMAX_NHBUF;index++)
      {
#ifdef __MPROFB
      nodeSearchCount++;
      NodeSearchCountTot++;
#endif  /* __MPROFB */

      if ((MNodeIndex[index] >= 0) &&
         !strcasecmp(MNode[MNodeIndex[index]]->Name,NName))
        {
        if (NP != NULL)
          *NP = MNode[MNodeIndex[index]];

#ifdef __MPROFB
        MDB(1,fSTRUCT) MLog("INFO:     %s succeeded for %s - searches: %d  average: %.2f (%d calls)\n",
          FName,
          NName,
          nodeSearchCount,
          (double)NodeSearchCountTot / CalledCount,
          CalledCount);
#endif /* END __MPROFB */

        return(SUCCESS);
        }

      if (MNodeIndex[index] == MCONST_NHNEVERSET)
        {
        break;
        }
      }    /* END for (index) */

#ifdef __MPROFB
    MDB(1,fSTRUCT) MLog("INFO:     %s failed for %s - searches: %d  average: %.2f (%d calls)\n",
      FName,
      NName,
      nodeSearchCount,
      (double)NodeSearchCountTot / CalledCount,
      CalledCount);
#endif /* END __MPROFB */
 
    return(FAILURE);
    }  /* END if (MSched.UseNodeIndex != FALSE) */

  /* NOTE:  should also support hash or binary based look-up */

  /* clear hostname to allow rapid comparison */

  memset(QHostName,0,sizeof(QHostName));

  if ((ptr = (char *)strchr(NName,'.')) != NULL)
    {
    MUStrCpy(QHostName,NName,MIN((int)sizeof(QHostName),ptr - NName + 1));
    MUStrCpy(QNetwork,ptr,sizeof(QNetwork));
    }
  else
    {
    MUStrCpy(QHostName,NName,sizeof(QHostName));
    MUStrCpy(QNetwork,MSched.DefaultDomain,sizeof(QNetwork));
    } 
 
  len = strlen(QHostName);

  for (nindex = 0;nindex < MSched.M[mxoNode];nindex++)
    {
#ifdef __MPROFB
    nodeSearchCount++;
    NodeSearchCountTot++;
#endif  /* __MPROFB */

    N = MNode[nindex];

    if ((N == NULL) || (N->Name[0] == '\0'))
      {
      /* end of table reached */

      /* return next available slot */

#ifdef __MPROFB
      MDB(1,fSTRUCT) MLog("INFO:     %s failed for %s - searches: %d  average: %.2f (%d calls)\n",
        FName,
        NName,
        nodeSearchCount,
        (double)NodeSearchCountTot / CalledCount,
        CalledCount);
#endif /* END __MPROFB */
 
      return(FAILURE);
      }

    if (((N->Name[0] != QHostName[0]) || 
         (N->Name[1] != QHostName[1])) && (N->Name[1] != '\0'))
      {
      /* empty or mismatch */

      continue;
      }
 
    if (strncasecmp(QHostName,N->Name,len))
      {
      /* partial host names must match */
 
      continue;
      }
 
    /* determine list host name */
 
    if ((ptr = strchr(N->Name,'.')) != NULL)
      {
      MUStrCpy(LHostName,N->Name,MIN((int)sizeof(LHostName),ptr - N->Name + 1));
      MUStrCpy(LNetwork,ptr,sizeof(LNetwork));
      }
    else
      {
      strcpy(LHostName,N->Name);
      strcpy(LNetwork,MSched.DefaultDomain);
      }
 
    if (strcasecmp(QHostName,LHostName))
      {
      /* host names must match */ 
 
      continue;
      }
 
    if ((QNetwork[0] != '\0') &&
        (LNetwork[0] != '\0') &&
         strcasecmp(QNetwork,LNetwork))
      {
      /* network names must match */
 
      continue;
      }
 
    /* host/network name matches */

    if (NP != NULL) 
      *NP = MNode[nindex];

#ifdef __MPROFB
    MDB(1,fSTRUCT) MLog("INFO:     %s succeeded for %s - searches: %d  average: %.2f (%d calls)\n",
      FName,
      NName,
      nodeSearchCount,
      (double)NodeSearchCountTot / CalledCount,
      CalledCount);
#endif /* END __MPROFB */
 
    return(SUCCESS);
    }  /* END for (nindex) */
 
  MDB(1,fSTRUCT) MLog("WARNING:  node buffer overflow on node %s in %s()\n",
    NName,
    FName);
 
  MDB(1,fSTRUCT) MLog("INFO:     ignoring node '%s'\n",
    NName);

#ifdef __MPROFB
  MDB(1,fSTRUCT) MLog("INFO:     %s failed for %s - searches: %d  average: %.2f (%d calls)\n",
    FName,
    NName,
    nodeSearchCount,
    (double)NodeSearchCountTot / CalledCount,
    CalledCount);
#endif /* END __MPROFB */
 
  return(FAILURE);
  }  /* END MNodeFind() */


/* END MNodeFind.c */
