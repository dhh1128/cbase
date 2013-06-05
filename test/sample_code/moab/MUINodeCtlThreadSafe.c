/* HEADER */

/**
 * @file MUINodeCtlThreadSafe.c
 *
 * Contains Job Control function
 */



#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 * compare two node transition objects by their index low to high
 *
 * @param A (I)
 * @param B (I)
 */

int __MTransNodeIndexCompLToH(

  mtransnode_t **A, /* I */
  mtransnode_t **B) /* I */

  {
  return((*A)->Index - (*B)->Index);
  } /* END __MTransNodeIndexCompLToH */





/**
 * Process 'mdiag -n' request in a threadsafe way.
 *
 * @param S (I) [modified]
 * @param CFlagBM (I) enum MRoleEnum bitmap
 * @param Auth (I)
 */

int MUINodeCtlThreadSafe(

  msocket_t  *S,
  mbitmap_t *CFlagBM,
  char *Auth)

  {
  int rc = SUCCESS;
  int rindex;
  int CTok;

  char DiagOpt[MMAX_LINE];
  char FlagString[MMAX_LINE];
  char tmpVal[MMAX_LINE];
  char tmpName[MMAX_LINE];

  mgcred_t *U = NULL;

  marray_t ConstraintList;
  mnodeconstraint_t Constraint;

  enum MNodeAttrEnum AList[] = {
    mnaNodeID,
    mnaArch,
    mnaAvlClass,
    mnaCfgClass,
    mnaAvlGRes,
    mnaCfgGRes,
    mnaDedGRes,
    /* mnaGEvent, only verbose */
    mnaGMetric,
    mnaAvlTime,
    mnaEnableProfiling,
    mnaFeatures,
    mnaFlags,
    mnaHVType,
    mnaJobList,
    mnaLastUpdateTime,
    mnaLoad,
    mnaMaxJob,
    mnaMaxJobPerUser,
    mnaMaxProcPerUser,
    mnaMaxLoad,
    mnaMaxProc,
    /* mnaMessages, only verbose */
    mnaNetAddr,
    mnaNodeIndex,
    mnaNodeState,
    mnaOldMessages,
    mnaOperations,
    mnaOS,
    mnaOSList,
    mnaOwner,
    mnaPartition,
    mnaPowerPolicy,
    mnaPowerSelectState,
    mnaPowerState,
    mnaPriority,
    mnaPrioF, 
    mnaProcSpeed,
    mnaRack,
    mnaRADisk,
    mnaRAMem,
    mnaRAProc,
    mnaRASwap,
    mnaRCDisk,
    mnaRCMem,
    mnaRCProc,
    mnaRCSwap,
    mnaRsvCount,
    mnaResOvercommitFactor,
    mnaResOvercommitThreshold,
    mnaRMList,
    mnaRsvList,
    mnaSize,
    mnaSlot,
    mnaSpeed,
    mnaSpeedW,
    mnaStatATime,
    mnaStatMTime,
    mnaStatTTime,
    mnaStatUTime,
    mnaVariables,
    mnaVMOSList,
    mnaVarAttr,
    mnaNONE };

  enum MNodeAttrEnum VAList[] = {
    mnaNodeID,
    mnaArch,
    mnaAvlClass,
    mnaCfgClass,
    mnaAvlGRes,
    mnaCfgGRes,
    mnaDedGRes,
    mnaGEvent,
    mnaGMetric,
    mnaAvlTime,
    mnaEnableProfiling,
    mnaFeatures,
    mnaFlags,
    mnaHVType,
    mnaJobList,
    mnaLastUpdateTime,
    mnaLoad,
    mnaMaxJob,
    mnaMaxJobPerUser,
    mnaMaxProcPerUser,
    mnaMaxLoad,
    mnaMaxProc,
    mnaMessages,
    mnaNetAddr,
    mnaNodeIndex,
    mnaNodeState,
    mnaOldMessages,
    mnaOperations,
    mnaOS,
    mnaOSList,
    mnaOwner,
    mnaPartition,
    mnaPowerPolicy,
    mnaPowerSelectState,
    mnaPowerState,
    mnaPriority,
    mnaPrioF, 
    mnaProcSpeed,
    mnaRack,
    mnaRADisk,
    mnaRAMem,
    mnaRAProc,
    mnaRASwap,
    mnaRCDisk,
    mnaRCMem,
    mnaRCProc,
    mnaRCSwap,
    mnaRsvCount,
    mnaResOvercommitFactor,
    mnaResOvercommitThreshold,
    mnaRMList,
    mnaRsvList,
    mnaSize,
    mnaSlot,
    mnaSpeed,
    mnaSpeedW,
    mnaStatATime,
    mnaStatMTime,
    mnaStatTTime,
    mnaStatUTime,
    mnaVariables,
    mnaVMOSList,
    mnaVarAttr,
    mnaNONE };

  mxml_t *DE;
  mxml_t *NE;

  mtransnode_t *N;

  marray_t NList;

  mdb_t *MDBInfo;

  mbitmap_t CFlags;  /* bitmap of enum MCModeEnum */

  /* check to see of user authorized to run this command */

  MUserAdd(Auth,&U);

  if (MUICheckAuthorization(
        U,
        NULL,
        NULL,
        mxoNode,
        mcsDiagnose,
        mncmQuery,
        NULL,
        NULL,
        0) == FAILURE)
    {
    char tmpLine[MMAX_LINE];

    snprintf(tmpLine,sizeof(tmpLine),"ERROR:    user '%s' is not authorized to run command '%s'\n",
      Auth,
      MUI[mcsDiagnose].SName);

    MUISAddData(S,tmpLine);

    return(FAILURE);
    }

  MUArrayListCreate(&NList,sizeof(mtransnode_t *),10);

  MSchedGetAttr(msaDBHandle,mdfNONE,(void **)&MDBInfo,-1);

  /* Construct the ConstraintList, and add found constraints to the list */

  MUArrayListCreate(&ConstraintList,sizeof(mnodeconstraint_t),10);  

  /* Parse out and get node name specified by user */

  if (MXMLGetAttr(S->RDE,MSAN[msanOp],NULL,DiagOpt,sizeof(DiagOpt)) == SUCCESS)
    {
    char *TokPtr;
    char *ptr;
  
    /* Build node constraint list for each specified in DiagOpt */
  
    for (ptr = MUStrTok(DiagOpt,",",&TokPtr);ptr != NULL;ptr = MUStrTok(NULL,",",&TokPtr))
      {
      Constraint.AType = mnaNodeID;
      MUStrCpy(Constraint.AVal[0],ptr,sizeof(Constraint.AVal[0]));
      Constraint.ACmp = mcmpEQ2;
    
      MUArrayListAppend(&ConstraintList,(void *)&Constraint);
      }
    }

  /* -w key=value[, ...]   Parse out 'NODESTATE=' specifiers that might be present */

  CTok = -1;    /* Set iterator to initial value */

  /* Parse the -w option */
  while (MS3GetWhere(
        S->RDE,
        NULL,
        &CTok,
        tmpName,
        sizeof(tmpName),
        tmpVal,
        sizeof(tmpVal)) == SUCCESS)
  {
    enum MNodeAttrEnum AIndex;

    /* Parse out a valid Node attribute keyword */
    AIndex = (enum MNodeAttrEnum) MUGetIndexCI(tmpName,MNodeAttr,FALSE,mnaNONE);

    /* now process the Node attribute */
    switch(AIndex)
      {
        case mnaNodeState:
          Constraint.AType = mnaNodeState;
          Constraint.ACmp = mcmpEQ;
          MUStrCpy(Constraint.AVal[0],tmpVal,sizeof(Constraint.AVal[0]));
          MUArrayListAppend(&ConstraintList,(void*)&Constraint);
          break;

        default:
          /* Do nothing for anything else at this time */
          break;
      } /* switch(AIndex) */
    
  } /* END while(MS3GetWhere) */


  /* Parse out 'partition' specifiers */

  if (MXMLGetAttr(S->RDE,"partition",NULL,DiagOpt,sizeof(DiagOpt)) == SUCCESS)
    {
    char *TokPtr;
    char *ptr;
  
    /* Build node constraint list for each specified in DiagOpt */
  
    for (ptr = MUStrTok(DiagOpt,",",&TokPtr);ptr != NULL;ptr = MUStrTok(NULL,",",&TokPtr))
      {
      if (strcmp(ptr,"ALL") == 0)
        {
        continue;
        }

      Constraint.AType = mnaPartition;
      MUStrCpy(Constraint.AVal[0],ptr,sizeof(Constraint.AVal[0]));
      Constraint.ACmp = mcmpEQ2;
    
      MUArrayListAppend(&ConstraintList,(void *)&Constraint);
      }
    }

  if (MXMLGetAttr(S->RDE,MSAN[msanFlags],NULL,FlagString,sizeof(FlagString)) == SUCCESS)
    {
    bmfromstring(FlagString,MClientMode,&CFlags);
    }

  /* The constraint list is populated now, so time to apply the constraint list
   * against the nodes specified by the database, and generate a
   * list that matches the constraints
   */

  /* Lock the list, then apply the constraint list: 
   *  Resultant list back into 'NList'
   */
  MCacheNodeLock(TRUE,TRUE);
  MCacheReadNodes(&NList,&ConstraintList);

  /* Done with the Constraint List so free it */
  MUArrayListFree(&ConstraintList);

  /* Create a XML Element to place the data into */
  if (MXMLCreateE(&S->SDE,(char *)MSON[msonData]) == FAILURE)
    {
    MUISAddData(S,"ERROR:    cannot create response\n");

    rc = FAILURE;    /* Failure mode, so mark rc and leave with resource freeing below */
    }
  else
    {
    DE = S->SDE;

    /* sort the nodes by index before we process them into XML output */

    qsort(
      (void *)NList.Array,
      NList.NumItems,
      sizeof(mjob_t *),
      (int(*)(const void *,const void *))__MTransNodeIndexCompLToH);

    /* Now generate the XML data of constrained nodes */
    for (rindex = 0;rindex < NList.NumItems;rindex++)
      {
      N = (mtransnode_t *)MUArrayListGetPtr(&NList,rindex);
  
      MNodeTransitionToXML(N,bmisset(&CFlags,mcmVerbose) ? VAList : AList,&NE);
  
      MXMLAddE(DE,NE);
      }   /* END for (rindex) */
      /* leave rc as SUCCESS */
    } /* END if (MXMLCreateE) */

  /* unlock the Cache node loc */
  MCacheNodeLock(FALSE,TRUE);

  /* done with the node list that was constrainted */
  MUArrayListFree(&NList);

  return(rc);
  }  /* END MUINodeCtlThreadSafe() */
/* END MUINodeCtlThreadSafe.c */
