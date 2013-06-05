/* HEADER */

/**
 * @file MVCTransition.c
 *
 * Contains Virtual Container Transition (mtransvc_t) related functions.
 */

#include <assert.h>

#include "moab.h"
#include "moab-proto.h"
#include "moab-global.h"
#include "moab-const.h"

/* Local prototypes */



/**
 * Helper function for testing user access to a VC transition object.
 *
 * @param VC [I] - Transition VC to check
 * @param U  [I] - User to check access for
 */

mbool_t MVCTransitionUserHasAccess(

  mtransvc_t *VC,
  mgcred_t   *U)

  {
  macl_t *tmpACL = NULL;
  mxml_t *CE;
  int CIndex = -1;
  mbool_t rc = FALSE;
  mtransvc_t *tmpVC = NULL;

  if ((VC == NULL) ||
      (U == NULL))
    {
    return(FALSE);
    }

  if (MVCCredIsOwner(NULL,VC->Name,U) == TRUE)
    {
    return(TRUE);
    }

  if (VC->ACL != NULL)
    {
    while (MXMLGetChild(VC->ACL,(char *)MVCAttr[mvcaACL],&CIndex,&CE) == SUCCESS)
      {
      MACLFromXML(&tmpACL,CE,FALSE);
      }

    if ((tmpACL != NULL) &&
        (MCredCheckAccess(U,NULL,maUser,tmpACL) == SUCCESS))
      {
      rc = TRUE;
      }

    MACLFreeList(&tmpACL);
    }  /* END if (VC->ACL != NULL) */

  if ((rc == FALSE) && (VC->ParentVCs != NULL) && (VC->ParentVCs[0] != '\0'))
    {
    /* Check parents */

    marray_t  VCList;
    marray_t  VCConstraintList;
    mvcconstraint_t VCConstraint;

    int   aindex;
    char *Ptr = NULL;
    char *TokPtr = NULL;
    char *TmpNames = NULL;

    MUStrDup(&TmpNames,VC->ParentVCs);
    MUArrayListCreate(&VCList,sizeof(mtransvc_t *),10);
    MUArrayListCreate(&VCConstraintList,sizeof(mvcconstraint_t),10);

    Ptr = MUStrTok(TmpNames,",",&TokPtr);

    while (Ptr != NULL)
      {
      VCConstraint.AType = mvcaName;
      VCConstraint.ACmp = mcmpEQ2;

      MUStrCpy(VCConstraint.AVal[0],Ptr,sizeof(VCConstraint.AVal[0]));

      MUArrayListAppend(&VCConstraintList,(void *)&VCConstraint);

      Ptr = MUStrTok(NULL,",",&TokPtr);
      }  /* END while (Ptr != NULL) */

    MCacheVCLock(TRUE,TRUE);
    MCacheReadVCs(&VCList,&VCConstraintList);

    for (aindex = 0;aindex < VCList.NumItems;aindex++)
      {
      tmpVC = (mtransvc_t *)MUArrayListGetPtr(&VCList,aindex);

      if ((tmpVC == NULL) || (MUStrIsEmpty(tmpVC->Name)))
        break;

      if (tmpVC->Name[0] == '\1')
        continue;

      if (MVCTransitionUserHasAccess(tmpVC,U) == TRUE)
        {
        rc = TRUE;

        break;
        }
      }  /* END for (aindex) */

    MCacheVCLock(FALSE,TRUE);
    MUArrayListFree(&VCList);
    MUArrayListFree(&VCConstraintList);

    MUFree(&TmpNames);
    }  /* END if (rc == FALSE) */

  return(rc);
  }  /* END MVCTransitionUserHasAccess() */



/**
 * Store VC in transition structure for storage in database.
 *
 * @param SVC (I) The VC to store
 * @param DVC (O) The transition object to store it in
 */

int MVCToTransitionStruct(

  mvc_t      *SVC,  /* I */
  mtransvc_t *DVC)  /* O */

  {
  if ((SVC == NULL) || (DVC == NULL))
    return(FAILURE);

  mstring_t tmpStr(MMAX_LINE);

  MUStrCpy(DVC->Name,SVC->Name,MMAX_NAME);
  MUStrDup(&DVC->Description,SVC->Description);

  if (SVC->Creator != NULL)
    MUStrCpy(DVC->Creator,SVC->Creator->Name,MMAX_NAME);

  MVCAToMString(SVC,mvcaCreateTime,&tmpStr);
  MUStrCpy(DVC->CreateTime,tmpStr.c_str(),sizeof(DVC->CreateTime));

  MVCAToMString(SVC,mvcaOwner,&tmpStr);
  MUStrCpy(DVC->Owner,tmpStr.c_str(),sizeof(DVC->Owner));

  MVCAToMString(SVC,mvcaJobs,&tmpStr);
  MUStrDup(&DVC->Jobs,tmpStr.c_str());

  MVCAToMString(SVC,mvcaNodes,&tmpStr);
  MUStrDup(&DVC->Nodes,tmpStr.c_str());

  MVCAToMString(SVC,mvcaVMs,&tmpStr);
  MUStrDup(&DVC->VMs,tmpStr.c_str());

  MVCAToMString(SVC,mvcaRsvs,&tmpStr);
  MUStrDup(&DVC->Rsvs,tmpStr.c_str());

  MVCAToMString(SVC,mvcaVCs,&tmpStr);
  MUStrDup(&DVC->VCs,tmpStr.c_str());

  if (SVC->Variables.NumItems > 0)
    {
    MUVarsToXML(&DVC->Variables,&SVC->Variables);
    }

  MVCAToMString(SVC,mvcaFlags,&tmpStr);
  MUStrCpy(DVC->Flags,tmpStr.c_str(),MMAX_LINE);

  MVCAToMString(SVC,mvcaParentVCs,&tmpStr);
  MUStrDup(&DVC->ParentVCs,tmpStr.c_str());

  if (!MACLIsEmpty(SVC->ACL))
    {
    macl_t *tACL;
    mxml_t *tmpE = NULL;
    mxml_t *AE;

    MXMLCreateE(&tmpE,(char *)MVCAttr[mvcaACL]);

    if (DVC->ACL != NULL)
      {
      MXMLDestroyE(&DVC->ACL);
      DVC->ACL = NULL;
      }

    for (tACL = SVC->ACL;tACL != NULL;tACL = tACL->Next)
      {
      AE = NULL;

      MACLToXML(tACL,&AE,NULL,FALSE);

      MXMLAddE(tmpE,AE);
      }

    if (tmpE->CCount > 0)
      {
      DVC->ACL = tmpE;
      }
    else
      {
      MXMLDestroyE(&tmpE);
      }
    }  /* END if (!MACLIsEmpty(SVC->ACL)) */

  MVCAToMString(SVC,mvcaReqStartTime,&tmpStr);
  if (!tmpStr.empty())
    MUStrDup(&DVC->ReqStartTime,tmpStr.c_str());

  if (SVC->MB != NULL)
    {
    DVC->MB = NULL;

    MXMLCreateE(&DVC->MB,(char *)MVCAttr[mvcaMessages]);

    MMBPrintMessages(SVC->MB,mfmXML,TRUE,-1,(char *)DVC->MB,0);
    }

  if (bmisset(&SVC->IFlags,mvcifDeleteTransition))
    {
    DVC->IsDeleteRequest = TRUE;
    }

  return(SUCCESS);
  }  /* END MVCToTransitionStruct() */



/**
 * Copies a mtransvc_t struct
 *
 * @param SVC - source
 * @param DVC - destination
 */

int MVCTransitionCopy(

  mtransvc_t *SVC,   /* I */
  mtransvc_t *DVC)   /* O */

  {
  if ((DVC == NULL) || (SVC == NULL))
    return(FAILURE);

  memcpy(DVC,SVC,sizeof(mtransvc_t));

  return(SUCCESS);
  }  /* END MVCTransitionCopy() */



/**
 * check to see if a given VC matches a list of constraints
 *
 * @param VC               (I)
 * @param VCConstraintList (I)
 */

mbool_t MVCTransitionMatchesConstraints(

  mtransvc_t   *VC,               /* I */
  marray_t     *VCConstraintList) /* I */

  {
  int cindex;
  mvcconstraint_t *VCConstraint;

  if (VC == NULL)
    return(FALSE);

  if ((VCConstraintList == NULL) || (VCConstraintList->NumItems == 0))
    return(TRUE);

  MDB(7,fSCHED) MLog("INFO:     Checking VC %s against constraint list.\n",
    VC->Name);

  for (cindex = 0; cindex < VCConstraintList->NumItems; cindex++)
    {
    VCConstraint = (mvcconstraint_t *)MUArrayListGet(VCConstraintList,cindex);

    if ((strcmp(VC->Name,VCConstraint->AVal[0]) != 0) &&
        (VCConstraint->AVal[0][0] != '\0') &&
        (strcmp(VCConstraint->AVal[0],"ALL")))
      {
      return(FALSE);
      }
    }  /* END for (cindex) */

  return(TRUE);
  }  /* END MVCTransitionMatchesConstraints() */

/**
 * Store a VC transition object in the given XML object
 *
 * @param VC       (I) The VC
 * @param VCEP     (O) The return XML
 * @param SVCAList (I) [optional] Attributes to save
 * @param CFlags   (I) Flags
 * @param FullXML  (I) Do hierarchical XML (include subVC XML in VC XML)
 */

int MVCTransitionToXML(

  mtransvc_t       *VC,       /* I */
  mxml_t          **VCEP,     /* O */
  enum MVCAttrEnum *SVCAList, /* I (optional) */
  long              CFlags,   /* I */
  mbool_t           FullXML)  /* I */

  {
  mxml_t *VCE;

  int aindex;

  enum MVCAttrEnum *AList;

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
    mvcaReqStartTime,/* requested starttime for guaranteed start time */
    mvcaRsvs,        /* names of rsvs in VC */
    mvcaVariables,   /* VC vars */
    mvcaVMs,         /* names of VMs in VC */
    mvcaVCs,         /* sub-VCs (always should be last) */
    mvcaNONE };

  if ((VC == NULL) || (VCEP == NULL))
    return(FAILURE);

  if (MXMLCreateE(VCEP,(char *)MXO[mxoxVC]) == FAILURE)
    {
    return(FAILURE);
    }

  VCE = *VCEP;

  if (SVCAList != NULL)
    AList = SVCAList;
  else
    AList = (enum MVCAttrEnum *)DAList;

  mstring_t tmpBuf(MMAX_LINE);

  for (aindex = 0; AList[aindex] != mvcaNONE; aindex++)
    {
    if (AList[aindex] == mvcaACL)
      {
      mxml_t *tmpACL = NULL;
      mxml_t *tmpE = NULL;
      int ACLIndex = -1;

      /* ACL stored as XML (as sub elements of a top element) */

      while (MXMLGetChild(VC->ACL,(char *)MVCAttr[mvcaACL],&ACLIndex,&tmpACL) == SUCCESS)
        {
        tmpE = NULL;

        MXMLDupE(tmpACL,&tmpE);

        MXMLAddE(VCE,tmpE);
        }

      continue;
      }  /* END if (AList[aindex] == mvcaACL) */

    if (FullXML == TRUE)
      {
      mbool_t DoContinue = TRUE;

      switch (AList[aindex])
        {
        case mvcaVCs:

          {
          /* Search for children VCs, add their XML */

          char *tmpVCList = NULL;
          char *Ptr;
          char *TokPtr = NULL;

          if ((VC->VCs == NULL) ||
              (VC->VCs[0] == '\0'))
            continue;

          MUStrDup(&tmpVCList,VC->VCs);

          Ptr = MUStrTok(tmpVCList,",",&TokPtr);

          while (Ptr != NULL)
            {
            marray_t VCList;
            marray_t VCConstraintList;
            mvcconstraint_t VCConstraint;
            mtransvc_t *tmpVC;
            mxml_t *tmpVCE = NULL;

            /* Create constraint list */

            MUArrayListCreate(&VCList,sizeof(mtransvc_t *),2);
            MUArrayListCreate(&VCConstraintList,sizeof(mvcconstraint_t),2);

            VCConstraint.AType = mvcaName;
            VCConstraint.ACmp = mcmpEQ2;

            MUStrCpy(VCConstraint.AVal[0],Ptr,sizeof(VCConstraint.AVal[0]));
            MUArrayListAppend(&VCConstraintList,(void *)&VCConstraint);

            /* Use constraint list to find VC */

            MCacheVCLock(TRUE,TRUE);
            MCacheReadVCs(&VCList,&VCConstraintList);

            if (VCList.NumItems >= 1)
              {
              tmpVC = (mtransvc_t *)MUArrayListGetPtr(&VCList,0);

              MVCTransitionToXML(
                tmpVC,
                &tmpVCE,
                AList,
                mfmNONE,
                FullXML);

              MXMLAddE(VCE,tmpVCE);
              }

            MUArrayListFree(&VCList);
            MUArrayListFree(&VCConstraintList);

            MCacheVCLock(FALSE,TRUE);

            Ptr = MUStrTok(NULL,",",&TokPtr);
            }  /* END while (Ptr != NULL) */

          MUFree(&tmpVCList);

          continue;
          }  /* END case mvcaVCs */

          break;

        case mvcaNodes:

          {
          char *tmpNList = NULL;
          char *Ptr;
          char *TokPtr = NULL;

          enum MNodeAttrEnum NAList[] = {
            mnaNodeID,
            mnaArch,
            mnaAvlGRes,
            mnaCfgGRes,
            mnaAvlTime,      
            mnaFeatures,
            mnaHopCount,
            mnaLoad,
            mnaNodeState,
            mnaOS,
            mnaOSList,
            mnaProcSpeed,
            mnaRADisk,
            mnaRAMem,
            mnaRAProc,
            mnaRASwap,
            mnaRCDisk,
            mnaRCMem,
            mnaRCProc,
            mnaRCSwap,
            mnaSpeed,
            mnaVMOSList,
            mnaNONE };

          if ((VC->Nodes == NULL) ||
              (VC->Nodes[0] == '\0'))
            continue;

          MUStrDup(&tmpNList,VC->Nodes);

          Ptr = MUStrTok(tmpNList,",",&TokPtr);

          while (Ptr != NULL)
            {
            marray_t NList;
            marray_t NConstraintList;
            mnodeconstraint_t NConstraint;
            mtransnode_t *tmpN;
            mxml_t *tmpNE = NULL;

            /* Create constraint list */

            MUArrayListCreate(&NList,sizeof(mtransnode_t *),2);
            MUArrayListCreate(&NConstraintList,sizeof(mnodeconstraint_t),2);

            NConstraint.AType = mnaNodeID;
            NConstraint.ACmp = mcmpEQ2;

            MUStrCpy(NConstraint.AVal[0],Ptr,sizeof(NConstraint.AVal[0]));
            MUArrayListAppend(&NConstraintList,(void *)&NConstraint);

            /* Use constraint list to find node */

            MCacheNodeLock(TRUE,TRUE);
            MCacheReadNodes(&NList,&NConstraintList);

            if (NList.NumItems >= 1)
              {
              tmpN = (mtransnode_t *)MUArrayListGetPtr(&NList,0);

              MNodeTransitionToXML(
                tmpN,
                NAList,
                &tmpNE);

              MXMLAddE(VCE,tmpNE);
              }

            MUArrayListFree(&NList);
            MUArrayListFree(&NConstraintList);
            MCacheNodeLock(FALSE,TRUE);

            Ptr = MUStrTok(NULL,",",&TokPtr);
            }  /* END while (Ptr != NULL) */

          MUFree(&tmpNList);
          }  /* END case mvcaNodes */

          break;

        case mvcaRsvs:

          {
          char *tmpRList = NULL;
          char *Ptr;
          char *TokPtr = NULL;

          enum MRsvAttrEnum RAList[] = {
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
            mraMessages,
            mraOwner,
            mraPartition,
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

          if ((VC->Rsvs == NULL) ||
              (VC->Rsvs[0] == '\0'))
            continue;

          MUStrDup(&tmpRList,VC->Rsvs);

          Ptr = MUStrTok(tmpRList,",",&TokPtr);

          while (Ptr != NULL)
            {
            marray_t RList;
            marray_t RConstraintList;
            mrsvconstraint_t RConstraint;

            mtransrsv_t *tmpR;
            mxml_t *tmpRE = NULL;

            /* Create constraint list */

            MUArrayListCreate(&RList,sizeof(mtransrsv_t *),2);
            MUArrayListCreate(&RConstraintList,sizeof(mrsvconstraint_t),2);

            RConstraint.AType = mraName;
            RConstraint.ACmp = mcmpEQ2;

            MUStrCpy(RConstraint.AVal[0],Ptr,sizeof(RConstraint.AVal[0]));
            MUArrayListAppend(&RConstraintList,(void *)&RConstraint);

            /* Use constraint list to find Rsv */

            MCacheRsvLock(TRUE,TRUE);
            MCacheReadRsvs(&RList,&RConstraintList);

            if (RList.NumItems >= 1)
              {
              tmpR = (mtransrsv_t *)MUArrayListGetPtr(&RList,0);

              MRsvTransitionToXML(
                tmpR,
                &tmpRE,
                RAList,
                NULL,
                mfmNONE);

              MXMLAddE(VCE,tmpRE);
              }

            MUArrayListFree(&RList);
            MUArrayListFree(&RConstraintList);
            MCacheRsvLock(FALSE,TRUE);

            Ptr = MUStrTok(NULL,",",&TokPtr);
            }  /* END while (Ptr != NULL) */

          MUFree(&tmpRList);
          }  /* END case mvcaRsvs */

          break;

        case mvcaJobs:

          {
          char *tmpJList = NULL;
          char *Ptr;
          char *TokPtr = NULL;

          if ((VC->Jobs == NULL) ||
              (VC->Jobs[0] == '\0'))
            continue;

          MUStrDup(&tmpJList,VC->Jobs);

          Ptr = MUStrTok(tmpJList,",",&TokPtr);

          while (Ptr != NULL)
            {
            marray_t JList;
            marray_t JConstraintList;
            mjobconstraint_t JConstraint;

            mtransjob_t *tmpJ;
            mxml_t *tmpJE = NULL;

            /* Create constraint list */

            MUArrayListCreate(&JList,sizeof(mtransjob_t *),2);
            MUArrayListCreate(&JConstraintList,sizeof(mjobconstraint_t),2);

            JConstraint.AType = mjaJobID;
            JConstraint.ACmp = mcmpEQ2;

            MUStrCpy(JConstraint.AVal[0],Ptr,sizeof(JConstraint.AVal[0]));
            MUArrayListAppend(&JConstraintList,(void *)&JConstraint);

            /* Use constraint list to find Job */

            MCacheJobLock(TRUE,TRUE);
            MCacheReadJobs(&JList,&JConstraintList);

            if (JList.NumItems >= 1)
              {
              tmpJ = (mtransjob_t *)MUArrayListGetPtr(&JList,0);
              MJobTransitionBaseToXML(tmpJ,&tmpJE,NULL,NULL);
              MXMLAddE(VCE,tmpJE);
              }

            MUArrayListFree(&JList);
            MUArrayListFree(&JConstraintList);
            MCacheJobLock(FALSE,TRUE);

            Ptr = MUStrTok(NULL,",",&TokPtr);
            }  /* END while (Ptr != NULL) */

          MUFree(&tmpJList);
          }  /* END case mvcaJobs */

          break;

        case mvcaVMs:

          {
          char *tmpVMList = NULL;
          char *Ptr;
          char *TokPtr = NULL;

          if ((VC->VMs == NULL) ||
              (VC->VMs[0] == '\0'))
            continue;

          MUStrDup(&tmpVMList,VC->VMs);

          Ptr = MUStrTok(tmpVMList,",",&TokPtr);

          while (Ptr != NULL)
            {
            marray_t VMList;
            marray_t VMConstraintList;
            mjobconstraint_t VMConstraint;

            mtransvm_t *tmpVM = NULL;
            mxml_t *tmpVE = NULL;

            /* Create constraint list */

            MUArrayListCreate(&VMList,sizeof(mtransvm_t *),2);
            MUArrayListCreate(&VMConstraintList,sizeof(mvmconstraint_t),2);

            VMConstraint.AType = mjaJobID;
            VMConstraint.ACmp = mcmpEQ2;

            MUStrCpy(VMConstraint.AVal[0],Ptr,sizeof(VMConstraint.AVal[0]));
            MUArrayListAppend(&VMConstraintList,(void *)&VMConstraint);

            /* Use constraint list to find VM */

            MCacheVMLock(TRUE,TRUE);
            MCacheReadVMs(&VMList,&VMConstraintList);

            if (VMList.NumItems >= 1)
              {
              tmpVM = (mtransvm_t *)MUArrayListGetPtr(&VMList,0);

              MVMTransitionToXML(tmpVM,&tmpVE,NULL);

              MXMLAddE(VCE,tmpVE);
              }

            MUArrayListFree(&VMList);
            MUArrayListFree(&VMConstraintList);
            MCacheVMLock(FALSE,TRUE);

            Ptr = MUStrTok(NULL,",",&TokPtr);
            }  /* END while (Ptr != NULL) */

          MUFree(&tmpVMList);
          }  /* END case mvcaVMs */

          break;

        default:

          DoContinue = FALSE;

          break;
        }  /* END switch (AList[aindex]) */

      if (DoContinue == TRUE)
        continue;
      }  /* END if (FullXML == TRUE) */

    if (AList[aindex] == mvcaVariables)
      {
      mxml_t *VE = NULL;

      MXMLDupE(VC->Variables,&VE);

      MXMLAddE(VCE,VE);

      continue;
      }
    else if (AList[aindex] == mvcaMessages)
      {
      mxml_t *ME = NULL;

      MXMLDupE(VC->MB,&ME);

      MXMLAddE(VCE,ME);

      continue;
      }

    MVCTransitionAToString(VC,AList[aindex],&tmpBuf,mfmNONE);

    if (!MUStrIsEmpty(tmpBuf.c_str()))
      MXMLSetAttr(VCE,(char *)MVCAttr[AList[aindex]],(void *)tmpBuf.c_str(),mdfString);
    }  /* END for (aindex = 0; AList[aindex] != mvcaNONE; aindex++) */

  return(SUCCESS);
  }  /* END MVCTransitionToXML() */



/**
 * Report VC transition attribute as string
 *
 * @param VC     (I)
 * @param AIndex (I)
 * @param Buf    (O)
 * @param Mode   (I)
 */

int MVCTransitionAToString(

  mtransvc_t       *VC,     /* I */
  enum MVCAttrEnum  AIndex, /* I */
  mstring_t        *Buf,    /* O */
  int               Mode)   /* I (bitmap of enum MCModeEnum) */

  {
  if ((VC == NULL) || (Buf == NULL))
    return(FAILURE);

  MStringSet(Buf,"");

  switch (AIndex)
    {
    case mvcaCreateTime:

      MStringSet(Buf,VC->CreateTime);

      break;

    case mvcaCreator:

      MStringSet(Buf,VC->Creator);

      break;

    case mvcaDescription: /* description */

      MStringSet(Buf,VC->Description);

      break;

    case mvcaFlags:       /* flags (from MVCFlagEnum) */

      MStringSet(Buf,VC->Flags);

      break;

    case mvcaJobs:        /* names of jobs in VC */

      MStringSet(Buf,VC->Jobs);

      break;

    case mvcaName:        /* VC name */

      MStringSet(Buf,VC->Name);

      break;

    case mvcaNodes:       /* names of nodes in VC */

      MStringSet(Buf,VC->Nodes);

      break;

    case mvcaOwner:

      MStringSet(Buf,VC->Owner);

      break;

    case mvcaParentVCs:

      MStringSet(Buf,VC->ParentVCs);

      break;

    case mvcaReqStartTime:

      MStringSet(Buf,VC->ReqStartTime);

      break;

    case mvcaRsvs:        /* names of rsvs in VC */

      MStringSet(Buf,VC->Rsvs);

      break;

    case mvcaVariables:   /* VC vars */

      /* not handled, shouldn't be done this way as it is XML (see MVCTransitionToXML for the right way) */

      /* NO-OP */

      break;

    case mvcaVCs:         /* sub-VCs */

      MStringSet(Buf,VC->VCs);

      break;

    case mvcaVMs:         /* names of VMs in VC */

      MStringSet(Buf,VC->VMs);

      break;

    default:

      /* Not supported */

      return(FAILURE);

      /* NOTREACHED */
      break;
    }

  return(SUCCESS);
  }  /* END MVCTransitionAToString() */



/**
 * Allocate memory for the given VC transition object
 *
 * @param VCP (O) [alloc-ed] point to newly allocated VC transition object
 */

int MVCTransitionAllocate(

  mtransvc_t **VCP)

  {
  mtransvc_t *VC;

  if (VCP == NULL)
    return(FAILURE);

  VC = (mtransvc_t *)MUCalloc(1,sizeof(mtransvc_t));

  *VCP = VC;

  return(SUCCESS);
  }  /* END MVCTransitionAllocate() */



int MVCTransitionFree(

  void **VCP)

  {
  mtransvc_t *VC;

  const char *FName = "MVCTransitionFree";

  MDB(8,fSTRUCT) MLog("%s(%s)\n",
    FName,
    ((VCP != NULL) && (*VCP != NULL)) ? (*((mtransvc_t **)VCP))->Name : "NULL");

  if (VCP == NULL)
    return(FAILURE);

  VC = (mtransvc_t *)*VCP;

  MUFree(&VC->Jobs);
  MUFree(&VC->Nodes);
  MUFree(&VC->VMs);
  MUFree(&VC->Rsvs);
  MUFree(&VC->VCs);
  MUFree(&VC->Description);
  MUFree(&VC->ParentVCs);
  MUFree(&VC->ReqStartTime);

  MXMLDestroyE(&VC->ACL);
  MXMLDestroyE(&VC->Variables);
  MXMLDestroyE(&VC->MB);

  MUFree((char **)VCP);

  return(SUCCESS);
  }  /* END MVCTransitionFree() */


int MVCTransition(

  mvc_t *VC)

  {
  mtransvc_t *TVC;

  if (VC == NULL)
    return(FAILURE);

#if defined (__NOMCOMMTHREAD)
  return(SUCCESS);
#endif

  if ((bmisset(&VC->IFlags,mvcifDontTransition)) &&
      (!bmisset(&VC->IFlags,mvcifDeleteTransition)))
    return(SUCCESS);

  MVCTransitionAllocate(&TVC);

  MVCToTransitionStruct(VC,TVC);

  MOExportTransition(mxoxVC,TVC);

  return(SUCCESS);
  }  /* END MVCTransition() */



