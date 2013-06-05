/* HEADER */

      
/**
 * @file MNodeRack.c
 *
 * Contains: Location of Nodes on Rack functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  




/**
 *
 *
 * @param N
 * @param FIndex
 * @param SIndex
 */

int MNodeLocationFromName(

  mnode_t *N,
  int     *FIndex,
  int     *SIndex)

  {
  const char *FName = "MNodeLocationFromName";

  MDB(7,fSTRUCT) MLog("%s(%s,FIndex,SIndex)\n",
    FName,
    (N != NULL) ? N->Name : "NULL");

  if (N == NULL)
    {
    return(FAILURE);
    }

  /* NYI */

  return(FAILURE);
  }  /* END MNodeLocationFromName() */


/**
 * Determine node slot/frame location.
 *
 * @param N (I) [modified]
 */

int MNodeGetLocation(

  mnode_t *N)  /* I (modified) */
 
  {
  int   sindex;
  int   rindex;

  int   nindex;
 
  char *tmpName;
  char  tmpShortName[MMAX_NAME];
  char  tmpLongName[MMAX_NAME];
  char  tmpLine[MMAX_NAME];
 
  char *ptr;
 
  long  Address;

  mhost_t *S;

  const char *FName = "MNodeGetLocation";
 
  MDB(5,fCONFIG) MLog("%s(%s)\n",
    FName,
    (N != NULL) ? N->Name : "NULL");

  if (N == NULL)
    {
    return(FAILURE);
    }

  if ((N->RM != NULL) &&
     ((N->RM->Type == mrmtMoab) ||
     ((N->RM->Type == mrmtNative) && 
     ((N->RM->SubType == mrmstXT3) || 
      (N->RM->SubType == mrmstXT4) || 
      (N->RM->SubType == mrmstX1E) ||
      (N->RM->SubType == mrmstSLURM)))))
    {
    /* XT3s/X1Es have many nodes.  Do not waste time loading rack/frame info 
       unless explictly specified */

    return(FAILURE);
    }

  if (MSched.EnableHighThroughput == TRUE)
    {
    /* allow rapid start-up in high throughput mode */

    return(FAILURE);
    }

  if ((N->RackIndex >= 0) && (N->SlotIndex < 0))
    {
    int sindex;

    /* partial information specified */

    /* attempt to auto-assign next available slot */

    for (sindex = 1;sindex < MMAX_RACKSIZE;sindex++)
      {
      if (MSys[N->RackIndex][sindex].N == NULL)
        {
        MRackAddNode(&MRack[N->RackIndex],N,sindex);

        return(SUCCESS);
        }
 
      if (MSys[N->RackIndex][sindex].N == N)
        {
        MRackAddNode(&MRack[N->RackIndex],N,sindex);

        return(SUCCESS);
        }
      }  /* END for (sindex) */
    }    /* END if ((N->RackIndex >= 0) && (N->SlotIndex < 0)) */
  else if ((N->RackIndex >= 0) && (N->SlotIndex >= 0))
    {
    if (MSys[N->RackIndex][N->SlotIndex].N == NULL)
      {
      MRackAddNode(&MRack[N->RackIndex],N,N->SlotIndex);

      return(SUCCESS);
      }

    if (MSys[N->RackIndex][N->SlotIndex].N == N)
      {
      return(SUCCESS);
      }
    }  /* END if (N->RackIndex >= 0) */


  /* NOTE:  native resource manager may have pseudo-names for nodes,
            restricting access to MOSGetHostName may not be the best
            behavior */

  if ((N->RM != NULL) && 
      (N->RM->Type != mrmtNative) && 
      (MSched.EnableNodeAddrLookup == TRUE))
    {
    if (MOSGetHostName(N->Name,NULL,(unsigned long *)&Address,FALSE) == FAILURE)
      {
      MDB(2,fCONFIG) MLog("ALERT:    cannot determine address for host '%s'\n",
        N->Name);
 
      Address = -1;
      }
    }
  else
    {
    Address = -1;
    }

  /* Create short/long names */

  MUStrCpy(tmpLongName,N->Name,sizeof(tmpLongName));
  MUStrCpy(tmpShortName,N->Name,sizeof(tmpShortName));

  if (MSched.DefaultDomain[0] == '.')
    {
    sprintf(tmpLongName,"%s%s",
      N->Name,
      MSched.DefaultDomain);
    }
  else
    {
    sprintf(tmpLongName,"%s.%s",
      N->Name,
      MSched.DefaultDomain);
    }

  if ((ptr = strchr(tmpShortName,'.')) != NULL)
    {
    /* Make long name short */

    *ptr = '\0';
    }

  /* attempt to locate node in cluster table */
 
  for (rindex = 1;rindex < MMAX_RACK;rindex++)
    {
    int tmpRIndex = rindex;

    if (MSched.StartUpRacks != NULL)
      {
      rindex = MSched.StartUpRacks[tmpRIndex];

      if (rindex == 0)
        {
        /* We have run out of racks with a NetName set in any slots */

        break;
        }
      }

    for (sindex = 1;sindex <= MMAX_RACKSIZE;sindex++)
      { 
      S = &MSys[rindex][sindex];

      /* NetNameSet is only true if the host has had at least one name set in NetName */

      if (S->NetNameSet == FALSE)
        continue;

      /* match long/short hostnames */
 
      if ((S->NetName[mnetEN0] == NULL) || 
          (strchr(S->NetName[mnetEN0],'.') == NULL))
        {
        /* use short name */

        tmpName = tmpShortName;
        }
      if ((S->NetName[mnetEN0] != NULL) &&
          (strchr(S->NetName[mnetEN0],'.') != NULL))
        {
        /* use long name */

        tmpName = tmpLongName;
        }
      else
        {
        /* just use the name */

        tmpName = N->Name;
        }

      for (nindex = 1;nindex < MMAX_NETTYPE;nindex++) 
        {
        if (S->NetName[nindex] == NULL)  /* valgrind may report bogus uninit on this line */
          continue;

        if (!strcmp(tmpName,S->NetName[nindex]) ||
           (Address == (long)S->NetAddr[nindex]))
          {
          break;
          }
        }    /* END for (nindex) */

      if (nindex >= MMAX_NETTYPE)
        {
        /* host does not match */

        continue;
        }

      /* host match located */
 
      if (S->RMName[0] == '\0')
        {
        MDB(6,fCONFIG) MLog("INFO:     RMName '%s' set for node[%02d][%02d] '%s'\n",
          tmpName,
          rindex,
          sindex,
          N->Name);
 
        MUStrCpy(S->RMName,tmpName,sizeof(S->RMName));
 
        S->MTime            = MSched.Time;
        MRack[rindex].MTime = MSched.Time;
 
        MRM[0].RMNI = nindex;
        }

      sprintf(tmpLine,"%d:%d",
        rindex,
        sindex);
        
      MNodeSetAttr(N,mnaSlot,(void **)tmpLine,mdfString,mSet);
 
      return(SUCCESS);
      }  /* END for (sindex) */

    rindex = tmpRIndex;
    }    /* END for (rindex) */

  /* no host match located */

  if (MNodeLocationFromName(N,&rindex,&sindex) == FAILURE)
    {
    MDB(7,fSIM) MLog("INFO:     cannot determine node/rack of host '%s'\n",
      N->Name);

    N->RackIndex = -1;
    N->SlotIndex = -1;
    }
  else
    {
    /* found node.  update system table */

    sprintf(tmpLine,"%d:%d",
      rindex,
      sindex);
      
    MNodeSetAttr(N,mnaSlot,(void **)tmpLine,mdfString,mSet);

    return(SUCCESS);
    }

  return(FAILURE);
  }  /* END MNodeGetLocation() */





/**
 * Add a node to rack.
 *
 * @param R (I) [modified]
 * @param N (I)
 * @param SIndex (I)
 */

int MRackAddNode(

  mrack_t  *R,      /* I (modified) */
  mnode_t  *N,      /* I */
  int       SIndex) /* I */

  {
  mhost_t *S;

  if ((R == NULL) || (N == NULL))
    {
    return(FAILURE);
    }

  N->RackIndex = R->Index;

  /* NOTE:  only add when both rack and slot are set */

  if ((SIndex == -1) && (N->SlotIndex == -1))
    {
    /* full location info not yet available */

    MDB(1,fSCHED) MLog("INFO:     node slot not yet set on node '%s', delaying setting rack until slot is known\n",
      N->Name);

    return(SUCCESS);
    }

  if (SIndex != -1)
    N->SlotIndex = SIndex;

  /* adequate info available, add node to rack */

  R->NodeCount++;

  if (R->Name[0] == '\0')
    {
    sprintf(R->Name,"%02d",
      R->Index);
    } 

  R->MTime = MSched.Time;

  if ((N->PtIndex <= 0) && (R->PtIndex != 0))
    {
    N->PtIndex = R->PtIndex;
    }

  S = &MSys[R->Index][N->SlotIndex];

  S->N = (void *)N;

  if (S->NetName[mnetEN0] == NULL)
    {
    /* populate empty ethernet name */

    MUStrDup(&S->NetName[mnetEN0],N->Name);
    S->NetNameSet = TRUE;

    if (MSched.StartUpRacks != NULL)
      {
      /* Add in this rack to StartUpRacks */

      int tmpR;

      for (tmpR = 0;tmpR < MMAX_RACK;tmpR++)
        {
        if (MSched.StartUpRacks[tmpR] == 0)
          MSched.StartUpRacks[tmpR] = R->Index;
          break;

        if (MSched.StartUpRacks[tmpR] == R->Index)
          break;
        }
      }  /* END if (MSched.StartUpRacks != NULL) */
    } /* END if (S->NetName[mnetEN0] == NULL) */

  if (S->RMName[0] == '\0')
    {
    /* set resource manager name */

    strcpy(S->RMName,N->Name);
    }

  if (!bmisset(&S->Attributes,mattrSystem))
    {
    /* compute node located */

    bmset(&S->Attributes,mattrBatch);
    }

  if (N->NodeType == NULL)
    {
    MUStrDup(&N->NodeType,R->NodeType);
    }

  MDB(5,fRM) MLog("INFO:     node '%s' assigned to location R:%d/S:%d\n",
    N->Name,
    N->RackIndex,
    N->SlotIndex);

  return(SUCCESS);
  }  /* END MRackAddNode() */





/**
 *
 *
 * @param RName (I) [optional]
 * @param RIndex (I) [optional]
 * @param RP (O) [optional]
 */

int MRackAdd(

  char      *RName,  /* I (optional) */
  int       *RIndex, /* I (optional) */
  mrack_t  **RP)     /* O (optional) */

  {
  mrack_t  *R;

  int       rindex;

  if ((RName == NULL) && (RIndex == NULL))
    {
    return(FAILURE);
    }

  if (RP != NULL)
    *RP = NULL;

  if ((RIndex == NULL) || (*RIndex < 0))
    {
    /* locate available rack */

    for (rindex = 0;rindex < MMAX_RACK;rindex++)
      {
      R = &MRack[rindex];

      if (R->Name[0] == '\0')
        {
        break;
        }

      if ((RName != NULL) && !strcmp(RName,R->Name))
        {
        /* rack already exists */

        if (RP != NULL)
          *RP = &MRack[rindex];

        if (RIndex != NULL)
          *RIndex = rindex;

        return(SUCCESS);
        }

      break;
      }  /* END for (rindex) */

    if (rindex >= MMAX_RACK)
      {
      return(FAILURE);
      }

    if (RIndex != NULL)
      *RIndex = rindex;

    R = &MRack[rindex];
    }  /* END if (RIndex != NULL) */
  else
    {
    rindex = *RIndex;
    }

  R = &MRack[rindex];

  /* initialize rack */

  memset(R,0,sizeof(mrack_t));

  R->Index = rindex;

  if (RName != NULL)
    {
    MUStrCpy(R->Name,RName,sizeof(R->Name));
    }
  else
    {
    sprintf(R->Name,"RACK%02d",
      R->Index);
    }

  R->MTime = MSched.Time;

  return(SUCCESS);
  }  /* END MRackAdd() */

/* END MNodeRack.c */
