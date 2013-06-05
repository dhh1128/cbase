/* HEADER */

/**
 * @file MNodeXML.c
 *
 * Contains Node To/From XML
 */



#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 * Convert node object to XML.
 *
 * @see MNodeAToString() - child
 * @see MUINodeDiagnose() - parent
 * @see MGEventListToXML() - child
 * @see MXResourceToXML() - child
 *
 * NOTE:  reports details for 'mdiag -n --xml'
 *
 * NOTE:  if R is specified, R-specific information mapping and
 *        control policies will be applied to NE 
 *
 * @param N            (I)
 * @param NE           (O) - existing 'node' XML element
 * @param SAList       (I) [optional]
 * @param DModeBM      (I) [bitmap of enum MCModeEnum]
 * @param ClientOutput (I)
 * @param ShowChildren (I)
 * @param R            (I) [optional]
 * @param NullVal      (I) [optional]
 */

int MNodeToXML(
  
  mnode_t            *N,
  mxml_t             *NE,
  enum MNodeAttrEnum *SAList,
  mbitmap_t          *DModeBM,
  mbool_t             ShowChildren,
  mbool_t             ClientOutput,
  mrm_t              *R,
  char               *NullVal)

  {
  const enum MNodeAttrEnum DAList[] = {    
    mnaOldMessages,
    mnaStatATime,
    mnaStatTTime,
    mnaStatUTime,
    mnaNONE };

  int  aindex;
 
  enum MNodeAttrEnum *AList;

  if ((N == NULL) || (NE == NULL))
    {
    return(FAILURE);
    }

  if (SAList != NULL)
    AList = SAList;
  else
    AList = (enum MNodeAttrEnum *)DAList;

  bmset(DModeBM,mcmXML);

  mstring_t tmpString(MMAX_LINE);

  if (R == NULL)
    { 
    for (aindex = 0;AList[aindex] != mnaNONE;aindex++)
      {
      if (AList[aindex] == mnaVariables)
        {
        if (!MUHTIsEmpty(&N->Variables))
          MUAddVarsToXML(NE,&N->Variables);

        continue;
        }

      if ((MNodeAToString(N,AList[aindex],&tmpString,DModeBM) == FAILURE) ||
          (MUStrIsEmpty(tmpString.c_str())))
        {
        if (NullVal == NULL)
          continue;

        MStringSet(&tmpString,NullVal);
        }
 
      MXMLSetAttr(NE,(char *)MNodeAttr[AList[aindex]],tmpString.c_str(),mdfString);
      }  /* END for (aindex) */
    }
  else
    {
    for (aindex = 0;AList[aindex] != mnaNONE;aindex++)
      {
      switch (AList[aindex])
        {
        case mnaVariables:

          if (!MUHTIsEmpty(&N->Variables))
            MUAddVarsToXML(NE,&N->Variables);

          break;

        default:

          MNodeAToString(N,AList[aindex],&tmpString,DModeBM);

          break;
        }    /* END switch (AList[aindex]) */

      if (tmpString.empty())
        {
        if (NullVal == NULL)
          continue;

        MStringSet(&tmpString,NullVal);
        }

      MXMLSetAttr(NE,(char *)MNodeAttr[AList[aindex]],tmpString.c_str(),mdfString);
      }      /* END for (aindex) */
    }        /* END else */

  /* show child structures */

  if (ShowChildren == TRUE)
    {
    if (N->MB != NULL)
      {
      mxml_t *ME = NULL;

      MNodeAToString(N,mnaMessages,&tmpString,DModeBM);
  
      if (MXMLFromString(&ME,tmpString.c_str(),NULL,NULL) == SUCCESS)
        {
        MXMLAddE(NE,ME);
        }
      }

    if (N->XLoad != NULL)
      {
      /* add gevent child */

      if (MGEventGetItemCount(&N->GEventsList) > 0)
        {
        mxml_t *GE = NULL;
        
        MXMLCreateE(&GE,(char *)"GEvents");
        MGEventListToXML(GE,&N->GEventsList);
        MXMLAddE(NE,GE);
        }
      }  /* END if (N->XLoad != NULL) */

    if (((MSched.DefaultN.ProfilingEnabled == TRUE) && (N->ProfilingEnabled == MBNOTSET)) ||
        (N->ProfilingEnabled == TRUE))
      {
      mxml_t *XE = NULL;

      MStatToXML((void *)&N->Stat,msoNode,&XE,ClientOutput,NULL);

      MXMLAddE(NE,XE);
      }

    if (N->VMList != NULL)
      {
      MNodeVMListToXML(N,NE);
      }
    }    /* END if (ShowChildren == TRUE) */

  return(SUCCESS);
  }  /* END MNodeToXML() */






/**
 *
 *
 * @param N (I) [modified]
 * @param E (I)
 * @param NodeIsExternal (I)
 */

int MNodeFromXML(

  mnode_t *N,                /* I (modified) */
  mxml_t  *E,                /* I */
  mbool_t  NodeIsExternal)   /* I */

  {
  int aindex;
  int CTok = -1;

  enum MNodeAttrEnum naindex;

  mxml_t *ME;

#ifndef __MOPT

  if ((N == NULL) || (E == NULL))
    {
    return(FAILURE);
    }

#endif /* !__MOPT */

  /* NOTE:  do not initialize node.  may be overlaying data */
 
  for (aindex = 0;aindex < E->ACount;aindex++)
    {
    naindex = (enum MNodeAttrEnum)MUGetIndex(E->AName[aindex],MNodeAttr,FALSE,0);
 
    if (naindex == mnaNONE)
      continue;

    if ((NodeIsExternal == TRUE) && 
        ((naindex == mnaNodeID) ||
         (naindex == mnaPartition)))
      {
      continue;
      }

    if ((naindex == mnaPowerSelectState) &&
        (bmisset(&N->RMAOBM,mnaPowerIsEnabled)))
      {
      /* RM reports power, don't restore PowerSelectState from .moab.ck */

      continue;   
      }
 
    MNodeSetAttr(N,naindex,(void **)E->AVal[aindex],mdfString,mSet);
    }  /* END for (aindex) */

  if (MXMLGetChild(E,(char *)MNodeAttr[mnaMessages],NULL,&ME) == SUCCESS)
    {
    MMBFromXML(&N->MB,ME);
    }

  /* Get the variables.  Example:  failure=1 is <Variables><Variable name="failure">1</Variable></Variables> */

  if (MXMLGetChild(E,"Variables",NULL,&ME) == SUCCESS)
    {
    mxml_t *VarE = NULL;
    int VarETok = -1;

    mstring_t VarName(MMAX_LINE);

    /* get each "Variable" child and add to linked list */

    while (MXMLGetChildCI(ME,(char*)MNodeAttr[mnaVariables],&VarETok,&VarE) == SUCCESS)
      {
      VarName.clear();

      MXMLGetAttrMString(VarE,"name",NULL,&VarName);

      if (!MUStrIsEmpty(VarE->Val))
        MUHTAdd(&N->Variables,VarName.c_str(),strdup(VarE->Val),NULL,MUFREE);
      else
        MUHTAdd(&N->Variables,VarName.c_str(),NULL,NULL,MUFREE);
      }
    }

  if (MXMLGetChild(E,(char *)MXO[mxoxStats],NULL,&ME) == SUCCESS)
    {
    MStatFromXML((void *)&N->Stat,msoNode,ME);
    }

  while (MXMLGetChild(E,(char *)MXO[mxoxVM],&CTok,&ME) == SUCCESS)
    {
    int AttrIndex;
    mvm_t *tmpVM = NULL;

    /* VMs have already been reported at this time */

    for (AttrIndex = 0;ME->AName[AttrIndex] != NULL;AttrIndex++)
      {
      if (!strcmp(ME->AName[AttrIndex],MVMAttr[mvmaID]))
        {
        if (MVMFind(ME->AVal[AttrIndex],&tmpVM) == FAILURE)
          {
          /* Add in VM, mark as unreported */

          if (MVMAdd(ME->AVal[AttrIndex],&tmpVM) == SUCCESS)
            {
            /* Successful VM add, now crosslink Node to VM and insert VM into Node's list */
            bmset(&tmpVM->IFlags,mvmifUnreported);

            tmpVM->N = N;

            MULLAdd(&N->VMList,tmpVM->VMID,tmpVM,NULL,NULL);
            }
          else
            {
            MDB(3,fCKPT) MLog("ERROR:    Could not add VM '%s' (for Node '%s') from checkpoint stream into the system\n",
                ME->AVal[AttrIndex], N->Name);
            }
          }

        break;
        }
      }

    /* only if we could add the VM above, can we parse its data stream */
    if (tmpVM != NULL)
      {
      MVMFromXML(tmpVM,ME);
      }
    } /* END if (MXMLGetChild(E,(char *)MXO[mxoxVM],...) */

  return(SUCCESS);
  }  /* END MNodeFromXML() */

/* END MNodeXML.c */
