/* HEADER */

/**
 * @file MVCXML.c
 *
 * Contains Virtual Container (VC) XML related functions.
 */

#include <assert.h>

#include "moab.h"
#include "moab-proto.h"
#include "moab-global.h"
#include "moab-const.h"



/**
 * Reports the given VC as XML.
 *
 * FullXML is not used for checkpointing, only for output
 *
 * @param VC      (I) - the VC
 * @param VE      (O) - existing 'VC' XML element
 * @param SAList  (I) [optional] - specified attr list
 * @param FullXML (I) - if TRUE, include sub-object XML as child, not attribute
 */

int MVCToXML(
  mvc_t  *VC,
  mxml_t *VE,
  enum MVCAttrEnum *SAList,
  mbool_t FullXML)

  {
  const enum MVCAttrEnum DAList[] = {
    mvcaACL,         /* ACL */
    mvcaCreateTime,  /* time VC was created */
    mvcaCreator,     /* creator */
    mvcaDescription, /* description */
    mvcaFlags,       /* flags (from MVCFlagEnum) */
    mvcaJobs,        /* names of jobs in VC */
    mvcaMessages,    /* VC messages */
    mvcaName,        /* VC name */
    mvcaNodes,       /* names of nodes in VC */
    mvcaOwner,       /* VC owner */
    mvcaParentVCs,   /* names of parent VCs */
    mvcaReqStartTime,/* requested start time for guaranteed start time */
    mvcaRsvs,        /* names of rsvs in VC */
    mvcaVariables,   /* VC vars */
    mvcaVCs,         /* sub-VCs */
    mvcaVMs,         /* names of VMs in VC */
    mvcaThrottlePolicies, /* throttling policies attached to VC via mvcctl */
    mvcaNONE };

  int aindex;

  enum MVCAttrEnum *AList;
  mbool_t PrintACL = FALSE;

  if ((VC == NULL) || (VE == NULL))
    {
    return(FAILURE);
    }

  if (SAList != NULL)
    AList = SAList;
  else
    AList = (enum MVCAttrEnum *)DAList;

  mstring_t tmpBuf(MMAX_LINE);

  for (aindex = 0;AList[aindex] != mvcaNONE;aindex++)
    {
    if (AList[aindex] == mvcaACL)
      {
      PrintACL = TRUE;

      continue;
      }

    if ((FullXML == TRUE) && 
         ((AList[aindex] == mvcaJobs) ||
          (AList[aindex] == mvcaNodes) ||
          (AList[aindex] == mvcaRsvs) ||
          (AList[aindex] == mvcaVMs) ||
          (AList[aindex] == mvcaVCs) ||
          (AList[aindex] == mvcaParentVCs)))
      continue;

    if (AList[aindex] == mvcaVariables)
      {
      MUAddVarsToXML(VE,&VC->Variables);

      continue;
      }
    else if ((AList[aindex] == mvcaMessages) &&
              (VC->MB != NULL))
      {
      /* Use the already existing MMB to XML functionality */

      mxml_t *ME = NULL;

      MXMLCreateE(&ME,(char *)MVCAttr[mvcaMessages]);

      MMBPrintMessages(VC->MB,mfmXML,TRUE,-1,(char *)ME,0);

      MXMLAddE(VE,ME);

      continue;
      }

    MVCAToMString(VC,AList[aindex],&tmpBuf);

    if (!tmpBuf.empty())
      {
      MXMLSetAttr(VE,(char *)MVCAttr[AList[aindex]],tmpBuf.c_str(),mdfString);
      }
    }  /* END for (aindex) */

  if (FullXML)
    {
    /* If you specify FullXML, you get all of the children.  Don't check AList */

    mln_t *tmpL;
    mxml_t *CE;

    if (VC->Jobs != NULL)
      {
      mjob_t *J;

      enum MJobAttrEnum JAList[] = {
        mjaJobID,
        mjaAccount,
        mjaArgs,
        mjaClass,
        mjaCmdFile,
        mjaDescription,
        mjaFlags,
        mjaGroup,
        mjaPAL,
        mjaQOS,
        mjaRCL,
        mjaReqAWDuration,
        mjaReqHostList,    /**< requested hosts */
        mjaReqNodes,
        mjaReqReservation,
        mjaRM,
        mjaStatPSDed,
        mjaSysPrio,
        mjaTemplateFlags,
        mjaTrigger,
        mjaUser,
        mjaVMUsagePolicy,
        mjaNONE };

      for (tmpL = VC->Jobs;tmpL != NULL;tmpL = tmpL->Next)
        {
        CE = NULL;
        J = (mjob_t *)tmpL->Ptr;

        MJobToXML(J,&CE,JAList,NULL,NULL,FALSE,FALSE);

        MXMLAddE(VE,CE);
        }
      }  /* END if (VC->Jobs != NULL) */

    if (VC->Nodes != NULL)
      {
      mnode_t *N;

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

      for (tmpL = VC->Nodes;tmpL != NULL;tmpL = tmpL->Next)
        {
        CE = NULL;
        N = (mnode_t *)tmpL->Ptr;

        MXMLCreateE(&CE,(char *)MXO[mxoNode]);
        MNodeToXML(N,CE,NAList,0,FALSE,FALSE,NULL,NULL);

        MXMLAddE(VE,CE);
        }
      }  /* END if (VC->Nodes != NULL) */

    if (VC->Rsvs != NULL)
      {
      mrsv_t *R;

      for (tmpL = VC->Rsvs;tmpL != NULL;tmpL = tmpL->Next)
        {
        CE = NULL;
        R = (mrsv_t *)tmpL->Ptr;

        MRsvToXML(R,&CE,NULL,NULL,FALSE,mcmNONE);

        MXMLAddE(VE,CE);
        }
      }  /* END if (VC->Rsvs != NULL) */

    if (VC->VMs != NULL)
      {
      mvm_t *V;

      for (tmpL = VC->VMs;tmpL != NULL;tmpL = tmpL->Next)
        {
        CE = NULL;
        V = (mvm_t *)tmpL->Ptr;

        MXMLCreateE(&CE,(char *)MXO[mxoxVM]);
        MVMToXML(V,CE,NULL);

        MXMLAddE(VE,CE);
        }
      }  /* END if (VC->VMs != NULL) */

    if (VC->VCs != NULL)
      {
      mvc_t *V;

      for (tmpL = VC->VCs;tmpL != NULL;tmpL = tmpL->Next)
        {
        CE = NULL;
        V = (mvc_t *)tmpL->Ptr;

        MXMLCreateE(&CE,(char *)MXO[mxoxVC]);
        MVCToXML(V,CE,SAList,FullXML);

        MXMLAddE(VE,CE);
        }
      }  /* END if (VC->VCs != NULL) */
    }  /* END if (FullXML) */

  if ((PrintACL == TRUE) && (!MACLIsEmpty(VC->ACL)))
    {
    macl_t *tACL;
    mxml_t *AE = NULL;

    for (tACL = VC->ACL;tACL != NULL;tACL = tACL->Next)
      {
      AE = NULL;

      MACLToXML(tACL,&AE,NULL,FALSE);

      MXMLAddE(VE,AE);
      }
    }  /* END if ((PrintACL == TRUE) && (!MACLIsEmpty(VC->ACL))) */

  return(SUCCESS);
  }  /* END MVCToXML() */





/**
 * Populates the VC with info from the XML.
 *
 * @see MVCFromString() - parent
 *
 * Note that this must be called after all objects have been loaded.
 * This function does NOT check if the XML is for the same VC that was passed in!
 *
 * @param VC [I] (modified) - The VC
 * @param E  [I] - XML description of VC
 */

int MVCFromXML(
  mvc_t  *VC, /* I (modified) */
  mxml_t *E)  /* I */

  {
  mcredl_t        *L = NULL;
  int              aindex;
  enum MVCAttrEnum saindex;
  mxml_t          *CE = NULL;
  int              CTok;

  if ((VC == NULL) || (E == NULL))
    {
    return(FAILURE);
    }

  for (aindex = 0;aindex < E->ACount;aindex++)
    {
    saindex = (enum MVCAttrEnum)MUGetIndex(E->AName[aindex],MVCAttr,FALSE,0);

    if (saindex == mvcaNONE)
      continue;

    if ((saindex == mvcaThrottlePolicies) &&
        (MVCGetDynamicAttr(VC,mvcaThrottlePolicies,(void **)&L,mdfOther) == FAILURE))
      {
      L = (mcredl_t *)MUCalloc(1,sizeof(mcredl_t));

      MPUInitialize(&L->ActivePolicy);

      MVCSetDynamicAttr(VC,mvcaThrottlePolicies,(void **)&L,mdfOther);
      }

    MVCSetAttr(VC,saindex,(void **)E->AVal[aindex],mdfString,mSet);
    }

  CTok = -1;

  if (MXMLGetChild(E,"Variables",&CTok,&CE) == SUCCESS)
    {
    MUAddVarsFromXML(CE,&VC->Variables);
    }

  CTok = -1;

  if (MXMLGetChild(E,MVCAttr[mvcaMessages],&CTok,&CE) == SUCCESS)
    {
    MMBFromXML(&VC->MB,CE);
    }

  CTok = -1;

  while (MXMLGetChild(E,(char *)MVCAttr[mvcaACL],&CTok,&CE) == SUCCESS)
    {
    /* ACL's */

    MACLFromXML(&VC->ACL,CE,FALSE);
    }

  MVCTransition(VC);

  return(SUCCESS);
  }  /* END MVCFromXML() */



