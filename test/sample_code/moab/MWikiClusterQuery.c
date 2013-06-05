/* HEADER */

      
/**
 * @file MWikiClusterQuery.c
 *
 * Contains: MWikiClusterQuery
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  

#include "MWikiInternal.h"  


/**
 * Query cluster/resource information from WIKI interface.
 *
 * NOTE:  This is the only WIKI call which is allowed to recover by using a
 *        fallback WIKI interface.  If recovery is successful, R->State will 
 *        change to Active
 *
 * @see MRMClusterQuery() - parent
 * @see MWikiGetData() - child
 *
 * @param R (I)
 * @param RCount (O) [optional]
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MWikiClusterQuery(

  mrm_t                *R,       /* I */
  int                  *RCount,  /* O (optional) */
  char                 *EMsg,    /* O (optional,minsize=MMAX_LINE) */
  enum MStatusCodeEnum *SC)      /* O (optional) */

  {
  char **Data;

  char  *ptr;

  char   Name[MMAX_LINE];    /* Name must be large enough to handle long node ranges */
  char   Type[MMAX_NAME];

  mnode_t *N;

  int    nindex;  /* node index */
  int    ncount;  /* node count */
  int    rindex;  /* record index */
  int    rcount;  /* wiki node record count */

  mnode_t **NList = NULL;

  int Status;

  mwikirm_t *S;

  const char *FName = "MWikiClusterQuery";

  MDB(2,fWIKI) MLog("%s(%s,RCount,EMsg,SC)\n",
    FName,
    (R != NULL) ? R->Name : "NULL");

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (SC != NULL)
    *SC = mscNoError;

  if (RCount != NULL)
    *RCount = 0;

  if (R == NULL)
    {
    return(FAILURE);
    }

  if ((R->State == mrmsConfigured) || (R->State == mrmsDisabled))
    {
    return(SUCCESS);
    }

  /* If any SLURM event flags have been set since the last iteration then
   * reload configuration based on the event and clear the SLURM event flags.
   */
  if (bmisset(&MWikiSlurmEventFlags[R->Index],mwsefPartition))
    {
    MSLURMLoadParInfo(R);

    MMRegisterEvent(mpetClassChange);
    }

  if (bmisset(&MWikiSlurmEventFlags[R->Index],mwsefJob))
    {
    /* No processing at this time for Job events */
    }

  bmclear(&MWikiSlurmEventFlags[R->Index]); /* config has been reloaded so clear SLURM event flags */ 

  S = (mwikirm_t *)R->S;

  if ((S == NULL) || (S->ClusterInfoBuffer == NULL))
    {
    if (MWikiGetData(
          R,
          FALSE,
          -1,
          NULL,
          EMsg,
          SC) == FAILURE)
      {
      MDB(2,fWIKI) MLog("ALERT:    cannot update cluster data for %s RM %s\n",
        MRMType[R->Type],
        R->Name);

      return(FAILURE);
      }
    }  /* END if ((S == NULL) || ... ) */

  Data = &S->ClusterInfoBuffer;

  /* load node data */

  if ((NULL == Data) ||
      (NULL == *Data) ||
      (ptr = strstr(*Data,MCKeyword[mckArgs])) == NULL)
    {
    MDB(1,fWIKI) MLog("ALERT:    cannot locate ARG marker in %s()\n",
      FName);

    MUFree(Data);

    return(FAILURE);
    }
    
  ptr += strlen(MCKeyword[mckArgs]);

  /* FORMAT:  <RECORDCOUNT>#{<NODEID>|<NODERANGE>}:<FIELD>=<VALUE>;[<FIELD>=<VALUE>;]...
              #[{<NODEID>|<NODERANGE>}:...] */

  /* NOTE: <RECORDCOUNT> should point to number of node name / node range records reported,
           not number of nodes represented by ranges. */

  rcount = (int)strtol(ptr,NULL,10);

  if (rcount <= 0)
    {
    MDB(1,fWIKI) MLog("INFO:     no node data sent by %s RM\n",
      MRMType[R->Type]);

    MUFree(Data);

    return(FAILURE);
    }

  ncount = 0;

  MDB(2,fWIKI) MLog("INFO:     loading %d node record(s)\n",
    rcount);

  if ((ptr = MUStrChr(ptr,'#')) == NULL)
    {
    MDB(1,fWIKI) MLog("ALERT:    cannot locate start marker for first node record in %s()\n",
      FName);

    MUFree(Data);

    return(FAILURE);
    }

  NList = (mnode_t **)MUCalloc(1,sizeof(mnode_t *) * MSched.M[mxoNode]);

  /* advance ptr to point to first <nodeid> */

  ptr++;

  mstring_t ABuf(MMAX_BUFFER);

  for (rindex = 0;rindex < rcount;rindex++)
    {
    if (MWikiGetAttr(
         R,
         mxoNode,
         Name,             /* O */
         &Status,          /* O */
         Type,             /* O */
         &ABuf,            /* O */
         &ptr) == FAILURE) /* I/O */
      {
      MDB(2,fWIKI) MLog("ALERT:    cannot get wiki state information\n");

      break;
      }

    if ((Type[0] != '\0') && (strcmp(Type,"compute")))
      {
      /* process extension resources */

      /* NYI */

      continue;
      }  /* END ((Type[0] != '\0') && (strcmp(Type,"compute"))) */

    /* if Name is node range, create and populate temporary node */

    if (MUStringIsRange(Name) == TRUE)
      {
      mnl_t NodeList; /* node range list */

      enum MWNodeAttrEnum AList[mwnaLAST]; /* list of attrs to copy to range of nodes */

      mnode_t tmpN; /* temp node to hold attrs for range */

      mnode_t *nPtr = &tmpN;

      MNodeInitialize(nPtr,NULL);

      MCResInit(&nPtr->CRes);
      MCResInit(&nPtr->DRes);
      MCResInit(&nPtr->ARes);

      bmset(&nPtr->Flags,mnifIsTemp);

      MRMNodePreLoad(nPtr,mnsDown,R);

      /* apply attrs to tempNode and get attr list back to apply to range */

      MWikiNodeLoad(ABuf.c_str(),nPtr,R,AList);

      /* convert Name to node range - create nodes if they don't yet exist */

      MNLInit(&NodeList);

      if (MUNLFromRangeString(
          Name,       /* I - node range expression, (modified as side-affect) */
          NULL,       /* I - delimiters */
          &NodeList,   /* O - nodelist */
          NULL,       /* O - nodeid list */
          0,          /* I */
          0,          /* I Set TC to what is given in expression */
          TRUE,
          (R->IsBluegene) ? TRUE : FALSE) == FAILURE)
        {
        MDB(1,fWIKI) MLog("ALERT:    cannot expand range-based tasklist '%s' in %s\n",
          Name,
          FName);

        continue;
        }
     
      /* apply tempNode attrs to range nodes */

      for (nindex = 0;MNLGetNodeAtIndex(&NodeList,nindex,&N) == SUCCESS;nindex++)
        {
        if(N->State == mnsNONE) /* Is this ok to key off to know if node hasn't already been set up? */
          MRMNodePreLoad(N,mnsDown,R);
        else
          MRMNodePreUpdate(N,(enum MNodeStateEnum)Status,R);

        MWikiNodeApplyAttr(N,nPtr,R,AList);

        NList[nindex] = N;
        } /* END for (nindex = 0;NodeList[nindex].N != NULL;nindex++) */

      NList[nindex] = NULL;

      MNLFree(&NodeList);

      MNodeDestroy(&nPtr);
      } /* END if (MUStringIsRange(Name) == TRUE) */
    else /* non-range nodes */
      {
      if ((MNodeFind(Name,&N) == SUCCESS) && (bmisset(&N->IFlags,mnifRMDetected)))
        {
        /* update existing node */
  
        MRMNodePreUpdate(N,(enum MNodeStateEnum)Status,R);

        MWikiNodeUpdate(ABuf.c_str(),N,R,NULL);

        MRMNodePostUpdate(N,R);
        }
      else if (MNodeAdd(Name,&N) == SUCCESS)
        {
        /* if new node, load data */

        MRMNodePreLoad(N,mnsDown,R);

        MWikiNodeLoad(ABuf.c_str(),N,R,NULL);

        MRMNodePostLoad(N,R);

        MDB(3,fWIKI)
          MNodeShow(N);
        }
      else
        {
        MDB(1,fWIKI) MLog("ERROR:    node buffer is full  (ignoring node '%s')\n",
          Name);

        continue;
        }

      NList[0] = N;
      NList[1] = NULL;
      }  /* END else of (if (MUStringIsRange(Name) == TRUE)) */

    for (nindex = 0;NList[nindex] != NULL;nindex++)
      {
      N = NList[nindex];

      /* adjust node state */

      if ((N->State == mnsIdle) || (N->State == mnsActive))
        {
        mbool_t UseUtil = FALSE;
        mbool_t UseDed  = FALSE;

        if ((MPar[0].NAvailPolicy[mrProc] == mrapUtilized) ||
            (MPar[0].NAvailPolicy[mrProc] == mrapCombined))
          {
          UseUtil = TRUE;
          }

        if ((MPar[0].NAvailPolicy[mrProc] == mrapDedicated) ||
            (MPar[0].NAvailPolicy[mrProc] == mrapCombined))
          {
          UseDed = TRUE;
          }

        if (((UseDed == TRUE) && (N->DRes.Procs >= N->CRes.Procs)) ||
            ((UseUtil == TRUE) && (N->URes.Procs >= N->CRes.Procs)))
          {
          N->State = mnsBusy;
          }
        else if (((UseDed == TRUE) && (N->DRes.Procs >= 1)) ||
            ((UseUtil == TRUE) && (N->URes.Procs >= 1)))
          {
          N->State = mnsActive;
          }
        }    /* END if ((N->State == mnsIdle) || (N->State == mnsActive)) */
      }      /* END for (nindex) */

    ncount += nindex;
    }        /* END for (rindex) */

  if (RCount != NULL)
    *RCount = ncount;

  /* clean up */

  MUFree((char **)&NList);
  MUFree(Data);

  return(SUCCESS);
  }  /* END MWikiClusterQuery() */

/* END MWikiClusterQuery.c */
