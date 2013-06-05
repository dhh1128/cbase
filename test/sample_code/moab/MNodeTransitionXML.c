/* HEADER */

/**
 * @file MNodeTransitionXML.c
 *
 * Contains Node Transition XML functions
 */



#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 * Store a node transition object in the given XML object
 *
 * @param   N      (I) the node transition object
 * @param   SAList (I) [optional]
 * @param   NEP    (O) the XML object to store it in
 */

int MNodeTransitionToXML(

  mtransnode_t         *N,
  enum MNodeAttrEnum   *SAList,
  mxml_t              **NEP)

  {
  int vmindex;

  marray_t VMList;
  marray_t VMConstraintList;
  mvmconstraint_t VMConstraint;

  mtransvm_t *V;

  mbool_t DoMessages = FALSE;

  mxml_t *NE;
  mxml_t *VME;


  const enum MNodeAttrEnum *AList;

  int aindex;

  /* int tmpIndex; NODEINDEX is reported as 1-based */

  const enum MNodeAttrEnum DAList[] = {    
    mnaOldMessages,
    mnaStatATime,
    mnaStatTTime,
    mnaStatUTime,
    mnaNONE };

  *NEP = NULL;

  if (MXMLCreateE(NEP,(char *)MXO[mxoNode]) == FAILURE)
    {
    return(FAILURE);
    }

  if (SAList == NULL)
    {
    AList = DAList;
    }
  else
    {
    AList = SAList;
    }

  NE = *NEP;

  mstring_t Buffer(MMAX_LINE);

  for (aindex = 0; AList[aindex] != mnaNONE; aindex++)
    {
    Buffer.clear();

    switch(AList[aindex])
      {
      case mnaVariables:

        if (N->Variables != NULL)
          {
          mxml_t *VE = NULL;

          MXMLDupE(N->Variables,&VE);

          MXMLAddE(NE,VE);
          }

        break;

      case mnaMessages:

        DoMessages = TRUE;

        break;

      case mnaGEvent:

        {
        mxml_t *CP = NULL;

        if (!MUStrIsEmpty(N->GEvents.c_str()))
          {
          MXMLAddChild(NE,(char *)"GEvents",NULL,&CP);
          MXMLFromString(&CP,N->GEvents.c_str(),NULL,NULL);
          }
        }

        break;

      default:

        MNodeTransitionAToString(N,AList[aindex],&Buffer);

        if (!Buffer.empty())
          {
          MXMLSetAttr(NE,(char *)MNodeAttr[AList[aindex]],(void *)Buffer.c_str(),mdfString);
          }

      } /* END switch(AList[aindex]) */
    } /* END for(aindex... ) */

  if (N->HypervisorType != mhvtNONE)
    {
    /* Add VM's if any */

    MUArrayListCreate(&VMList,sizeof(mtransvm_t *),10);
    MUArrayListCreate(&VMConstraintList,sizeof(mvmconstraint_t),10);

    VMConstraint.AType = mvmaContainerNode;
    VMConstraint.ACmp = mcmpEQ;
    MUStrCpy(VMConstraint.AVal[0],N->Name,MMAX_NAME);
    MUArrayListAppend(&VMConstraintList,(void*)&VMConstraint);

    MCacheVMLock(TRUE,TRUE);
   
    /* Apply the list of VMs against the newly built VM ConstraintList as per the user request */
    MCacheReadVMs(&VMList,&VMConstraintList);
   
    /* Done with the constraint list, free it */
    MUArrayListFree(&VMConstraintList);

    for (vmindex = 0;vmindex < VMList.NumItems;vmindex++)
      {
      V = (mtransvm_t *)MUArrayListGetPtr(&VMList,vmindex);
   
      if ((V == NULL) || (MUStrIsEmpty(V->VMID)))
        break;
   
      if (V->VMID[0] == '\1')
        continue;

      VME = NULL;

      MVMTransitionToXML(
        V,
        &VME,
        NULL);

      MXMLAddE(NE,VME);
      }  /* END for (vmindex) */

      /* Release the VM Cache lock */
      MCacheVMLock(FALSE,TRUE);
     
      /* Free the temp VM list that was sent back to client */
      MUArrayListFree(&VMList);
      }  /* END if (N->VMOSList != NULL) */

  if ((DoMessages == TRUE) && (N->Messages != NULL))
    {
    mxml_t *CP = NULL;

    MXMLAddChild(NE,(char *)MNodeAttr[mnaMessages],NULL,&CP);
    MXMLFromString(&CP,N->Messages,NULL,NULL);
    }

  return(SUCCESS);
  }  /* END MNodeTransitionToXML() */



/**
 * Export node attributes according to destination rm.
 * Adds to already created node xml. Must call MNodeTransitionToXML previously.
 *
 * Currently only supports:
 * mnaAvlClass
 * mnaCfgClass
 *
 * @param N (I)
 * @param SAList (I)
 * @param NEP (O)
 * @param PeerRM (I)
 */

int MNodeTransitionToExportXML(

  mtransnode_t         *N,      /* I */
  enum MNodeAttrEnum   *SAList, /* I (optional) */
  mxml_t              **NEP,    /* O */
  mrm_t                *PeerRM) /* I */

  {
  int     aindex;

  mxml_t *NE;

  const enum MNodeAttrEnum *AList;

  const enum MNodeAttrEnum DAList[] = {    
    mnaAvlClass,
    mnaCfgClass,
    mnaNONE };

  if ((NEP == NULL) || (*NEP == NULL) || (PeerRM == NULL))
    return(FAILURE);

  if (SAList == NULL)
    {
    AList = DAList;
    }
  else
    {
    AList = SAList;
    }

  NE = *NEP;

  mstring_t  tmpString(MMAX_LINE);

  for (aindex = 0;AList[aindex] != mnaNONE;aindex++)
    {
    tmpString.clear();

    switch (AList[aindex])
      {
      case mnaAvlClass:
      case mnaCfgClass:

        {
        mbitmap_t Flags;

        MClassListFromString(&Flags,N->AvailClasses,PeerRM);

        MClassListToMString(&Flags,&tmpString,PeerRM);
        }

        break;

      default:

        /* NO-OP */

        break;
      } /* END switch (aindex) */

    if (!MUStrIsEmpty(tmpString.c_str()))
      MXMLSetAttr(NE,(char *)MNodeAttr[AList[aindex]],tmpString.c_str(),mdfString);
    } /* END for (aindex = 0;AList[aindex] != mnaNONE;aindex++) */

  return(SUCCESS);
  } /* END int MNodeTransitionToExportXML() */
/* END MNodeTransitionXML.c */
