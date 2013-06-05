/* HEADER */

/**
 * @file MUIRsvDiagnoseThreadSafe.c
 *
 * Contains MUI Reservation Diagnose Thread safe function
 */



#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  




/**
 * compare reservation transition objects by end time, 
 * ordered low to high 
 *
 * @param A (I)
 * @param B (I)
 */

int __MTransRsvEndTimeLToH(

  mtransrsv_t **A,  /* I */
  mtransrsv_t **B)  /* I */

  {
  long AEndTime = (*A)->EndTime;
  long BEndTime = (*B)->EndTime;

  return((int)(AEndTime - BEndTime));
  }



/**
 * process mdiag -r request in a threadsafe way.
 *
 * @see MUIRsvDiagnose (sibling)
 *
 * @param S       (I) [modified]
 * @param CFlagBM (I) enum MRoleEnum bitmap
 * @param Auth    (I) authorization credentials for request
 */

int MUIRsvDiagnoseThreadSafe(

  msocket_t *S,       /* I (modified) */
  mbitmap_t *CFlagBM, /* I enum MRoleEnum bitmap */
  char      *Auth)    /* I */

  {
  int rindex;

  char DiagOpt[MMAX_LINE];
  char FlagString[MMAX_LINE];
  char RsvID[MMAX_LINE];
  char tmpType[MMAX_LINE];
  char RsvType[MMAX_LINE];

  char *ptr;

  marray_t RConstraintList;
  mrsvconstraint_t RConstraint;

  mxml_t *DE = NULL;
  mxml_t *RE = NULL;
  mxml_t *E = NULL;
  mxml_t *WE = NULL;

  int  WTok;

  mtransrsv_t *R;

  marray_t RList;

  mdb_t *MDBInfo;

  mgcred_t *U;

  mbitmap_t CFlags;  /* bitmap of enum MCModeEnum */

  const enum MRsvAttrEnum RAList[] = {
    mraName,
    mraACL,       /**< @see also mraCL */
    mraAAccount,
    mraAGroup,
    mraAUser,
    mraAQOS,
    mraAllocNodeCount,
    mraAllocNodeList,
    mraAllocProcCount,
    mraAllocTaskCount,
    mraCL,        /**< credential list */
    mraComment,
    mraCost,      /**< rsv AM lien/charge */
    mraCTime,     /**< creation time */
    mraDuration,
    mraEndTime,
    mraExcludeRsv,
    mraExpireTime,
    mraFlags,
    mraGlobalID,
    mraHostExp,
    mraHistory,
    mraLabel,
    mraLastChargeTime, /* time rsv charge was last flushed */
    mraLogLevel,
    mraMaxJob,
    mraMessages,
    mraOwner,
    mraPartition,
    mraPriority,
    mraProfile,
    mraReqArch,
    mraReqFeatureList,
    mraReqMemory,
    mraReqNodeCount,
    mraReqNodeList,
    mraReqOS,
    mraReqTaskCount,
    mraReqTPN,
    mraResources,
    mraRsvAccessList, /* list of rsv's and rsv groups which can be accessed */
    mraRsvGroup,
    mraRsvParent,
    mraSID,
    mraStartTime,
    mraStatCAPS,
    mraStatCIPS,
    mraStatTAPS,
    mraStatTIPS,
    mraSubType,
    mraTrigger,
    mraType,
    mraVariables,
    mraVMList,
    mraNONE };

  RsvID[0] = '\0';
  tmpType[0] = '\0';
  RsvType[0] = '\0';

  MUArrayListCreate(&RList,sizeof(mtransrsv_t *),10);

  MSchedGetAttr(msaDBHandle,mdfNONE,(void **)&MDBInfo,-1);

  if (MXMLGetAttr(S->RDE,MSAN[msanFlags],NULL,FlagString,sizeof(FlagString)) == SUCCESS)
    bmfromstring(FlagString,MClientMode,&CFlags);

  /* Get reservation name provided by user */

  MUArrayListCreate(&RConstraintList,sizeof(mrsvconstraint_t),10);  

  if (MXMLGetAttr(S->RDE,MSAN[msanOp],NULL,DiagOpt,sizeof(DiagOpt)) == SUCCESS)
    {
    char *TokPtr;
    char *ptr;

    /* Build job constraint list to only get job specified */
          
    RConstraint.AType = mraName;
    RConstraint.ACmp = mcmpEQ2;

    for (ptr = MUStrTok(DiagOpt,",",&TokPtr);ptr != NULL;ptr = MUStrTok(NULL,",",&TokPtr))
      {
      MUStrCpy(RConstraint.AVal[0],ptr,sizeof(RConstraint.AVal[0]));
    
      MUArrayListAppend(&RConstraintList,(void *)&RConstraint);
      }
    }

    /* extract type of reservation to display if given on command line */ 
  if (((ptr = strstr(S->RBuffer,"<Body")) != NULL) &&
       (MXMLFromString(&E,ptr,NULL,NULL) != FAILURE))
    {
    if (MXMLGetChild(E,(char *)MSON[msonRequest],NULL,&RE) != FAILURE)
      {
      WTok = -1;

      while (MS3GetWhere(
           RE,
           &WE,
           &WTok,
           tmpType,  /* O */
           sizeof(tmpType),
           RsvType,   /* O */
           sizeof(RsvType)) == SUCCESS)
       {
       if (!MUStrIsEmpty(RsvType))
         break;  
       }    /* END while (MS3GetWhere() == SUCCESS) */
      } /* END if MXMLGetChild */

    MXMLDestroyE(&E);
    }

  MCacheRsvLock(TRUE,TRUE);
  MCacheReadRsvs(&RList,&RConstraintList);

  /* sort reservations by end time , low to high */

  if (RList.NumItems > 1)
    {
    qsort(
      (void *)RList.Array,
      RList.NumItems,
      sizeof(mtransrsv_t *),
      (int(*)(const void *,const void *))__MTransRsvEndTimeLToH);
    }

  if (MXMLCreateE(&S->SDE,(char *)MSON[msonData]) == FAILURE)
    {
    MUISAddData(S,"ERROR:    cannot create response\n");

    MCacheRsvLock(FALSE,TRUE);
    MUArrayListFree(&RList);
    MUArrayListFree(&RConstraintList);
    return(FAILURE);
    }

  DE = S->SDE;

  /* find the user who issued the request */

  MUserAdd(Auth,&U);

  /* get the argument flags for the request */

  if (MXMLGetAttr(S->RDE,MSAN[msanFlags],NULL,FlagString,sizeof(FlagString)) == SUCCESS)
    bmfromstring(FlagString,MClientMode,&CFlags);

  /* look for a specified resveration id */

  if (MXMLGetAttr(S->RDE,MSAN[msanOp],NULL,DiagOpt,sizeof(DiagOpt)) == SUCCESS)
    {
    MUStrCpy(RsvID,DiagOpt,sizeof(RsvID));
    }

  /* loop through all reservations and create an xml element for each, adding
   * it to the return data element */

  for (rindex = 0;rindex < RList.NumItems;rindex++)
    {
    R = (mtransrsv_t *)MUArrayListGetPtr(&RList,rindex);

    if ((R == NULL) || (MUStrIsEmpty(R->Name)))
      break;

    if (R->Name[0] == '\1')
      continue;

    if ((!MUStrIsEmpty(RsvID)) && (strcmp(RsvID,R->Name) != 0))
      continue;

    if (!MUStrIsEmpty(tmpType))
      {
      /* compare type of reservation and command line requirement for type */
      if ((!MUStrIsEmpty(RsvType)) && strcasecmp(RsvType,R->Type))
          continue;
      }

    if (!MUStrIsEmpty(R->AUser) && (!strcmp(R->AUser,U->Name)))
      {
      MDB(6,fUI) MLog("INFO:     User %s is creator of Rsv %s\n", U->Name,R->Name);
      }
    else if (MUICheckAuthorization(
          U,
          NULL,
          NULL,
          mxoRsv,
          mcsDiagnose,
          mrcmQuery,
          NULL,
          NULL,
          0) == FAILURE)
      {
      /* no authority to diagnose reservation */

      continue;
      } /* END if (MUICheckAuthorization...) */

    MDB(6,fUI) MLog("INFO:     evaluating MRsv[%d] '%s'\n",
      rindex,
      R->Name);

    RE = NULL;

    /* XML will be alloc'd inside of MRsvTransitionToXML() */

    MRsvTransitionToXML(
      R,
      &RE,
      (enum MRsvAttrEnum *)RAList,
      NULL,
      mfmNONE);

    MXMLAddE(DE,RE);
    }  /* END for (rindex) */

  MCacheRsvLock(FALSE,TRUE);
  MUArrayListFree(&RList);
  MUArrayListFree(&RConstraintList);

  return(SUCCESS);
  } /* end MUIRsvDiagnoseThreadSafe() */
/* END MUIRsvDiagnoseThreadSafe.c */
