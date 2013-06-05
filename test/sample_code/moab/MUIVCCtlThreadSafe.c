/* HEADER */

/**
 * @file MUIVCCtlThreadSafe.c
 *
 * Contains MUI MV Control, thread safe operation
 */



#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  

/**
 * Process mvcctl -q request in a threadsafe way.
 *
 * @param S       (I) [modified]
 * @param CFlagBM (I) enum MRoleEnum bitmap
 * @param Auth    (I) authorization credentials for request
 */

int MUIVCCtlThreadSafe(

  msocket_t *S,       /* I (modified) */
  mbitmap_t *CFlagBM, /* I enum MRoleEnum bitmap */
  char      *Auth)    /* I */

  {
  int aindex;

  mtransvc_t *VC;
  char VCID[MMAX_LINE];

  char DiagOpt[MMAX_LINE];
  char FlagString[MMAX_LINE];

  mdb_t    *MDBInfo;
  marray_t  VCList;

  mgcred_t *U = NULL;

  mxml_t *DE;
  mxml_t *VCE;

  mxml_t *RDE;
  mxml_t *WE;
  int     WTok;
  char    tmpName[MMAX_LINE];
  char    tmpVal[MMAX_LINE];

  mbitmap_t CFlags;  /* bitmap of enum MCModeEnum */
  mbool_t FullXML = FALSE;  /* Do a full XML -sub objects XML as well */
  mbool_t ALLUsed = FALSE;    /* TRUE if user requested ALL (used for FullXML) */

  marray_t  VCConstraintList;
  mvcconstraint_t VCConstraint;

  const enum MVCAttrEnum DAList[] = {
    mvcaACL,         /* ACL */
    mvcaCreateTime,  /* creation time */
    mvcaCreator,     /* creator */
    mvcaDescription, /* description */
    mvcaFlags,       /* flags (from MVCFlagEnum) */
    mvcaJobs,        /* names of jobs in VC */
    mvcaMessages,    /* VC messages */
    mvcaName,        /* VC name */
    mvcaNodes,       /* names of nodes in VC */
    mvcaOwner,       /* VC owner */
    mvcaRsvs,        /* names of rsvs in VC */
    mvcaVariables,   /* VC vars */
    mvcaVCs,         /* sub-VCs */
    mvcaVMs,         /* names of VMs in VC */
    mvcaNONE };

  VCID[0] = '\0';

  if (S->RDE != NULL)
    {
    RDE = S->RDE;
    }
  else
    {
    RDE = NULL;

    if (MXMLFromString(&RDE,S->RPtr,NULL,NULL) == FAILURE)
      {
      MUISAddData(S,"ERROR:  corrupt command received\n");

      return(FAILURE);
      }
    }

  MUArrayListCreate(&VCList,sizeof(mtransvc_t *),10);

  MSchedGetAttr(msaDBHandle,mdfNONE,(void **)&MDBInfo,-1);

  if (MXMLGetAttr(RDE,MSAN[msanFlags],NULL,FlagString,sizeof(FlagString)) == SUCCESS)
    bmfromstring(FlagString,MClientMode,&CFlags);

  MUArrayListCreate(&VCConstraintList,sizeof(mvcconstraint_t),10);

  WE = NULL;
  WTok = -1;

  while (MS3GetWhere(
          RDE,
          &WE,
          &WTok,
          tmpName,
          sizeof(tmpName),
          tmpVal,
          sizeof(tmpVal)) == SUCCESS)
    {
    if (!strcmp(tmpName,MSAN[msanName]))
      {
      VCConstraint.AType = mvcaName;
      VCConstraint.ACmp = mcmpEQ2;

      MUStrCpy(VCConstraint.AVal[0],tmpVal,sizeof(VCConstraint.AVal[0]));

      MUArrayListAppend(&VCConstraintList,(void *)&VCConstraint);

      if (!strcmp(tmpVal,"ALL"))
        ALLUsed = TRUE;

      break;
      }
    }
/*
  if (MXMLGetAttr(S->RDE,MSAN[msanOp],NULL,DiagOpt,sizeof(DiagOpt)) == SUCCESS)
    {
    char *TokPtr;
    char *ptr;

    VCConstraint.AType = mvcaName;
    VCConstraint.ACmp = mcmpEQ2;

    for (ptr = MUStrTok(DiagOpt,",",&TokPtr);ptr != NULL;ptr = MUStrTok(NULL,",",&TokPtr))
      {
      MUStrCpy(VCConstraint.AVal[0],ptr,sizeof(VCConstraint.AVal[0]));

      MUArrayListAppend(&VCConstraintList,(void *)&VCConstraint);
      }
    }*/

  MCacheVCLock(TRUE,TRUE);
  MCacheReadVCs(&VCList,&VCConstraintList);

  if (MXMLCreateE(&S->SDE,(char *)MSON[msonData]) == FAILURE)
    {
    MUISAddData(S,"ERROR:    cannot create response\n");

    MCacheVCLock(FALSE,TRUE);
    MUArrayListFree(&VCList);
    MUArrayListFree(&VCConstraintList);
    return(FAILURE);
    }

  DE = S->SDE;

  /* find the user who issued the request */

  MUserAdd(Auth,&U);

  if (U == NULL)
    {
    MUISAddData(S,"ERROR:  user not authorized to run this command\n");

    MCacheVCLock(FALSE,TRUE);
    MUArrayListFree(&VCList);
    MUArrayListFree(&VCConstraintList);
    return(FAILURE);
    }

  /* get the argument flags for the request */

  if (MXMLGetAttr(S->RDE,MSAN[msanFlags],NULL,FlagString,sizeof(FlagString)) == SUCCESS)
    {
    bmfromstring(FlagString,MClientMode,&CFlags);

    if (MUStrStr(FlagString,"fullXML",0,TRUE,FALSE) != NULL)
      FullXML = TRUE;
    }

  if (MXMLGetAttr(S->RDE,MSAN[msanOp],NULL,DiagOpt,sizeof(DiagOpt)) == SUCCESS)
    {
    MUStrCpy(VCID,DiagOpt,sizeof(VCID));
    }

  for (aindex = 0;aindex < VCList.NumItems;aindex++)
    {
    VC = (mtransvc_t *)MUArrayListGetPtr(&VCList,aindex);

    if ((VC == NULL) || (MUStrIsEmpty(VC->Name)))
      break;

    if (VC->Name[0] == '\1')
      continue;

    if ((!MUStrIsEmpty(VCID)) && (strcmp(VCID,VC->Name) != 0))
      continue;

    if (MVCTransitionUserHasAccess(VC,U) == FALSE)
      continue;

    /* If full XML view, and ALL was specified, only show top-level VCs
        Sub-VC XML will be part of the parent VC XML */

    if ((FullXML == TRUE) &&
        (VC->ParentVCs != NULL) &&
        (VC->ParentVCs[0] != '\0') &&
        (ALLUsed == TRUE))
      continue;

    MDB(6,fUI) MLog("INFO:     evaluating VC '%s'\n",
      VC->Name);

    VCE = NULL;

    MVCTransitionToXML(
      VC,
      &VCE,
      (enum MVCAttrEnum *)DAList,
      mfmNONE,
      FullXML);

    MXMLAddE(DE,VCE);
    }  /* END for (aindex) */

  MCacheVCLock(FALSE,TRUE);
  MUArrayListFree(&VCList);
  MUArrayListFree(&VCConstraintList);

  return(SUCCESS);
  }  /* END MUIVCCtlThreadSafe() */
/* END MUIVCCtlThreadSafe.c */
