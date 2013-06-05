/* HEADER */

      
/**
 * @file MNodeRM.c
 *
 * Contains: Node and RM functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 * Add resource manager to node.
 *
 * NOTE:  checks for duplicates and only assigns R as node master RM if N->RM
 *        is not set.
 *
 * NOTE:  allocs N->RMList, adds R to N->RMList
 *
 * @see MVMAddRM() - sync
 *
 * @param N (I) [modified]
 * @param R (I)
 */

int MNodeAddRM(

  mnode_t *N,      /* I (modified) */
  mrm_t   *R)      /* I */

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
      {
      N->RMList[rmindex] = R;

      break;
      }

    if (N->RMList[rmindex] == R)
      break;
    }  /* END for (rmindex) */

  if (rmindex >= MMAX_RM)
    {
    return(FAILURE);
    }

  if (N->RM == NULL)
    {
    /* only add as master RM if N->RM is not set */

    N->RM = R;
    }

  return(SUCCESS);
  }  /* END MNodeAddRM() */





/**
 * Process RM-specified failure/event message.
 *
 * @see MClusterAdjustXXX() - parent
 * @see MNodeSetState() - child
 *
 * @param N (I) [modified] 
 */

int MNodeProcessRMMsg(

  mnode_t *N)  /* I (modified) */

  {
  mrm_t *R;

  char *ptr;

  int   rindex;

  if ((N == NULL) || (N->RMMsg == NULL))
    {
    return(FAILURE);
    }

  for (rindex = 0;rindex < MSched.M[mxoRM];rindex++)
    {
    R = &MRM[rindex];

    if (R->Name[0] == '\0')
      break;

    if (MRMIsReal(R) == FALSE)
      continue;

    if (N->RMMsg[rindex] == NULL)
      continue;

    if (!strncasecmp(N->RMMsg[rindex],"ERROR",strlen("ERROR")))
      {
      ptr = N->RMMsg[rindex] + strlen("ERROR");
   
      if (!strncasecmp(ptr,"event:",strlen("event:")))
        {
        /* FORMAT:  ERROREVENT:<EVENT_TYPE> ... */
   
        char tmpLine[MMAX_LINE];
   
        char *ptr2;
        char *TokPtr2;
   
        int eindex;
   
        MUStrCpy(tmpLine,ptr + strlen("event:"),sizeof(tmpLine));
   
        ptr2 = MUStrTok(tmpLine," \t:",&TokPtr2);
   
        eindex = MUMAGetIndex(meGEvent,ptr2,mVerify);
   
        if (eindex != 0)
          {
          MNodeSetAttr(N,mnaGEvent,(void **)&MAList[meGEvent][eindex],mdfString,mSet);
          }
        }  /* END else (!strncasecmp(N->RMMsg[rindex],"event:",strlen("event:"))) */
      else
        {
        MDB(7,fPBS) MLog("INFO:     node '%s' marked down - reports internal error '%s'\n",
          N->Name,
          N->RMMsg[rindex] + strlen("ERROR"));
   
        if (N->RMMsgNew[rindex] == TRUE)
          {
          char tmpLine[MMAX_LINE];
   
          /* record node message */
   
          MNodeSetAttr(N,mnaMessages,(void **)N->RMMsg[rindex],mdfString,mSet);
   
          snprintf(tmpLine,sizeof(tmpLine),"RMFAILURE:  local rm failure on node %s - %s",
            N->Name,
            N->RMMsg[rindex]);
   
          MSysRegEvent(tmpLine,mactNONE,1);
          }
   
        if (MSched.RMMsgIgnore == FALSE)
          N->RMState[rindex] = mnsDown;
        }
      }  /* END if (!strncasecmp(N->RMMsg[rindex],"ERROR",strlen("ERROR"))) */
    else if (N->RMMsgNew[rindex] == TRUE)
      {
      /* FORMAT:  EVENT:<EVENT_TYPE> ... */
   
      if (!strncasecmp(N->RMMsg[rindex],"event:",sizeof("event:")))
        {
        char tmpLine[MMAX_LINE];
   
        char *ptr;
        char *TokPtr;
   
        int eindex;
   
        MUStrCpy(tmpLine,N->RMMsg[rindex] + strlen("event:"),sizeof(tmpLine));
   
        ptr = MUStrTok(tmpLine," \t:",&TokPtr);
   
        eindex = MUMAGetIndex(meGEvent,ptr,mVerify);
   
        if (eindex != 0)
          {
          MNodeSetAttr(N,mnaGEvent,(void **)&MAList[meGEvent][eindex],mdfString,mSet);
          }
        }  /* END else (!strncasecmp(N->RMMsg[rindex],"event:",strlen("event:"))) */
      }    /* END else if (N->RMMsgNew[rindex] == TRUE) */
    }      /* END for (rindex) */

  return(SUCCESS);
  }      /* END MNodeProcessRMMsg() */






/**
 * Get RM consensus state.
 *
 * Apply pessimistic/optimistic node state rules
 *
 * @see MNodeApplyStatePolicy() - peer - sync?
 *
 * @param  N (I)
 * @param  Type (I)
 * @param  State (O)
 */

int MNodeGetRMState(

  mnode_t                  *N,
  enum MRMResourceTypeEnum  Type,
  enum MNodeStateEnum      *State)

  {
  mrm_t *R;

  enum MRMNodeStatePolicyEnum NSPolicy;

  int rindex;

  mbool_t FoundUp   = MBNOTSET;
  mbool_t FoundDown = MBNOTSET;

  if ((N == NULL) || (State == NULL))
    {
    /* should not happen */

    return(FAILURE);
    }

  *State = mnsNONE;

  if (N->RMList == NULL)
    {
    *State = N->State;

    return(SUCCESS);
    }

  for (rindex = 0;N->RMList[rindex] != NULL;rindex++)
    {
    R = N->RMList[rindex];

    if (MRMIsReal(R) == FALSE)
      continue;

    if ((!bmisset(&R->RTypes,Type)) &&
        (Type != mrmrtAny))
      {
      continue;
      }

    if (MNODESTATEISUP(N->RMState[R->Index]) == TRUE)
      FoundUp = TRUE;
    else if (N->RMState[R->Index] == mnsNONE)
      continue;
    else
      FoundDown = TRUE;
    }  /* END for (rindex) */

  R = N->RM;

  if (R->NodeStatePolicy != mrnspNONE)
    NSPolicy = R->NodeStatePolicy;
  else if (bmisset(&N->IFlags,mnifMultiComputeRM))
    NSPolicy = mrnspOptimistic;
  else
    NSPolicy = mrnspPessimistic;

  if ((FoundUp == TRUE) && (FoundDown == TRUE))
    {
    /* found both, use NSPolicy */

    if (NSPolicy == mrnspPessimistic)
      {
      *State = mnsDown;
      }
    else
      {
      *State = mnsIdle;
      }
    }
  else if (FoundUp == TRUE)
    {
    /* only found up */

    *State = mnsIdle;
    }
  else if (FoundDown == TRUE)
    {
    /* only found down */

    *State = mnsDown;
    }
  else 
    {
    /* didn't find anything */

    *State = mnsNONE;
    }

  if (*State == mnsNONE)
    {
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MNodeGetRMState() */



/**
 * Set message on node and determine whether it is new, blank, or old.
 *
 * @param N
 * @param Msg (may be NULL)
 * @param RM
 */

int MNodeSetRMMessage(

  mnode_t  *N,
  char     *Msg,
  mrm_t    *RM)

  {
  if ((N == NULL) || (RM == NULL))
    {
    return(FAILURE);
    }

  if ((Msg == NULL) && (N->RMMsg[RM->Index] != NULL))
    {
    N->RMMsgNew[RM->Index] = TRUE;
 
    MUFree(&N->RMMsg[RM->Index]);
    }
  else if ((Msg != NULL) && 
          ((N->RMMsg[RM->Index] == NULL) || strcmp(N->RMMsg[RM->Index],Msg)))
    {
    N->RMMsgNew[RM->Index] = TRUE;
 
    MUStrDup(&N->RMMsg[RM->Index],Msg);
    }
  else
    {
    N->RMMsgNew[RM->Index] = FALSE;
    }

  return(SUCCESS);
  }  /* END MNodeSetRMMessage() */





/*
 * Returns the first RM of the given ResourceType that is associated with the node.
 *
 * Returns SUCCESS if an RM was found, FAILURE otherwise.
 *
 * NOTE: You must specify either Node or Name.  If both are specified, Node, takes precedence.
 *
 * @param Node  [I] Node to check
 * @param Name  [I] Name of node to check
 * @param RType [I] The resource type to search for
 * @param RM    [O] Pointer to the resulting RM (NULL if not found)
 */

int MNodeGetResourceRM(

  mnode_t *Node,                  /* I (semi-optional) */
  char    *Name,                  /* I (semi-optional) */
  enum MRMResourceTypeEnum RType, /* I */
  mrm_t  **RM)                    /* O */

  {
  mnode_t *N = NULL;

  if (Node != NULL)
    {
    N = Node;
    }
  else if (Name != NULL)
    {
    if (MNodeFind(Name,&N) == FAILURE)
      return(FAILURE);
    }

  if ((RM == NULL) ||
      (N == NULL))
    {
    return(FAILURE);
    }

  *RM = NULL;

  if ((N->RM != NULL) &&
      (bmisset(&N->RM->RTypes,RType)))
    {
    *RM = N->RM;

    return(SUCCESS);
    }

  if (N->RMList != NULL)
    {
    int RMIndex;

    for (RMIndex = 0;N->RMList[RMIndex] != NULL;RMIndex++)
      {
      if (bmisset(&N->RMList[RMIndex]->RTypes,RType))
        {
        *RM = N->RMList[RMIndex];

        return(SUCCESS);
        }
      }
    } /* END if (N->RMList != NULL) */

  return(FAILURE);
  } /* END MNodeGetResourceRM() */
/* END MNodeRM.c */
