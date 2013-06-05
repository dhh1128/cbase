/* HEADER */

/**
 * @file MVCShow.c
 *
 * Contains Virtual Container (VC) printing related functions.
 */

#include <assert.h>

#include "moab.h"
#include "moab-proto.h"
#include "moab-global.h"
#include "moab-const.h"

/* Local prototypes */


int __MVCPrintOneVC(mstring_t *,mvc_t *VC,enum MFormatModeEnum,mbool_t,int);

/**
 * Print the requested VC(s)
 *
 * @param Buf     [O] - The INITIALIZED mstring to print to
 * @param VCExp   [I] - Name of VC to print -> "ALL" or NULL prints all VCs
 * @param U       [I] (optional) -  The user who is requesting (NULL assumes admin)
 * @param Format  [I] - Required output format (human or XML)
 * @param FullXML [I] - Include sub-object XML info in VCs
 */

int MVCShowVCs(

  mstring_t *Buf,
  char      *VCExp,
  mgcred_t  *U,
  enum MFormatModeEnum Format,
  mbool_t    FullXML)

  {
  mvc_t *VC;
  mhashiter_t HIter;
  char       *VCName;

  if (Buf == NULL)
    return(FAILURE);

  MUHTIterInit(&HIter);
  MStringSet(Buf,"");

  if ((VCExp == NULL) ||
      (!strcmp(VCExp,"ALL")))
    {
    /* print all VCs */

    while (MUHTIterate(&MVC,&VCName,(void **)&VC,NULL,&HIter) == SUCCESS)
      {
      if ((FullXML == TRUE) && (VC->ParentVCs != NULL))
        continue;

      if ((U == NULL) ||
          (MVCUserHasAccess(VC,U) == TRUE))
        {
        __MVCPrintOneVC(Buf,VC,Format,FullXML,0);
        }
      }
    }  /* END if (VCExp == NULL) || (!strcmp(VCExp,"ALL")) */
  else
    {
    /* Expression given.  This could be the name, but need to include all VCs
        with that label as well */

    while (MUHTIterate(&MVC,&VCName,(void **)&VC,NULL,&HIter) == SUCCESS)
      {    
      if ((U == NULL) ||
          (MVCUserHasAccess(VC,U) == TRUE))
        {
        if (!strcmp(VC->Name,VCExp))
          {
          __MVCPrintOneVC(Buf,VC,Format,FullXML,0);
          }
        }
      }  /* END while (MUHTIterate(&MVC...) */
    }  /* END else */

  return(SUCCESS);
  }  /* END MVCShowVCs() */




/**
 * sync with __PrintVCFromXML (mclient.c)
 *
 * @param Buf     [O] - The INITIALIZED mstring to print (append) to
 * @param VC      [I] - The VC to print
 * @param Format  [I] - The format to use (human vs. XML)
 * @param FullXML [I] - TRUE to put sub-object info under the VC
 * @param Indent  [I] - Level of indentation (for FullXML but using human format)
 */

int __MVCPrintOneVC(

  mstring_t *Buf,
  mvc_t     *VC,
  enum MFormatModeEnum Format,
  mbool_t    FullXML,
  int        Indent)

  {
  if ((Buf == NULL) || (VC == NULL))
    return(FAILURE);

  /* Don't print off VCs that are going to be harvested */

  if (bmisset(&VC->IFlags,mvcifHarvest))
    return(SUCCESS);

  if ((Format == mfmHuman) || (Format == mfmNONE))
    {
    mstring_t tmpStr(MMAX_LINE);
    mstring_t IndentStr(MMAX_LINE);
    int tmpI;

    MStringSet(&IndentStr,"");

    if (FullXML == TRUE)
      {
      for (tmpI = 0;tmpI < Indent;tmpI++)
        {
        /* Indent 4 spaces */
        MStringAppend(&IndentStr,"    ");
        }
      }

    MStringAppendF(Buf,"%sVC[%s]%s%s%s\n",
      IndentStr.c_str(),
      VC->Name,
      (VC->Description != NULL) ? " (" : "",
      (VC->Description != NULL) ? VC->Description : "",
      (VC->Description != NULL) ? ")" : "");

    /* Create time and creator on same line */

    MVCAToMString(VC,mvcaCreateTime,&tmpStr);
    MStringAppendF(Buf,"%s    Create Time: %s",
      IndentStr.c_str(),
      tmpStr.c_str());

    if (VC->Creator != NULL)
      {
      MStringAppendF(Buf,"    Creator: %s",
        VC->Creator->Name);
      }

    MStringAppend(Buf,"\n");

    if (VC->Owner != NULL)
      {
      MVCAToMString(VC,mvcaOwner,&tmpStr);

      MStringAppendF(Buf,"%s    Owner: %s\n",
        IndentStr.c_str(),
        tmpStr.c_str());
      }

    if (!MACLIsEmpty(VC->ACL))
      {
      MVCAToMString(VC,mvcaACL,&tmpStr);

      MStringAppendF(Buf,"%s    ACL:   %s\n",
        IndentStr.c_str(),
        tmpStr.c_str());
      }

    if (VC->Jobs != NULL)
      {
      MVCAToMString(VC,mvcaJobs,&tmpStr);

      MStringAppendF(Buf,"%s    Jobs:  %s\n",
        IndentStr.c_str(),
        tmpStr.c_str());
      }

    if (VC->Nodes != NULL)
      {
      MVCAToMString(VC,mvcaNodes,&tmpStr);

      MStringAppendF(Buf,"%s    Nodes: %s\n",
        IndentStr.c_str(),
        tmpStr.c_str());
      }

    if (VC->Rsvs != NULL)
      {
      MVCAToMString(VC,mvcaRsvs,&tmpStr);

      MStringAppendF(Buf,"%s    Rsvs:  %s\n",
        IndentStr.c_str(),
        tmpStr.c_str());
      }

    if (VC->VMs != NULL)
      {
      MVCAToMString(VC,mvcaVMs,&tmpStr);

      MStringAppendF(Buf,"%s    VMs:   %s\n",
        IndentStr.c_str(),
        tmpStr.c_str());
      }


    /* Variables */

    MVCAToMString(VC,mvcaVariables,&tmpStr);

    if (!tmpStr.empty())
      {
      MStringAppendF(Buf,"%s    Vars:  %s\n",
        IndentStr.c_str(),
        tmpStr.c_str());
      }

    /* Flags */

    MVCAToMString(VC,mvcaFlags,&tmpStr);

    if (!tmpStr.empty())
      {
      MStringAppendF(Buf,"%s    Flags: %s\n",
        IndentStr.c_str(),
        tmpStr.c_str());
      }

    /* VCs should always be last (makes fullXML easier) */

    if (VC->VCs != NULL)
      {
      if (FullXML == TRUE)
        {
        mln_t *tmpT;
        mvc_t *tmpVC;

        MStringAppendF(Buf,"%s    VCs:\n",
          IndentStr.c_str());

        for (tmpT = VC->VCs;tmpT != NULL;tmpT = tmpT->Next)
          {
          tmpVC = (mvc_t *)tmpT->Ptr;

          __MVCPrintOneVC(Buf,tmpVC,Format,FullXML,Indent + 1);
          }

        /* Don't need to append a final '\n' because eventually we'll
            hit a VC without sub-VCs, and it will have a '\n' */
        }
      else
        {
        MVCAToMString(VC,mvcaVCs,&tmpStr);

        MStringAppendF(Buf,"%s    VCs:   %s\n",
          IndentStr.c_str(),
          tmpStr.c_str());
        }
      }  /* END if (VC->VCs != NULL) */
    }  /* END if ((Format == mfmHuman) || (Format == mfmNONE)) */
  else if (Format == mfmXML)
    {
    mxml_t *VE;

    MXMLCreateE(&VE,(char *)MXO[mxoxVC]);
    MVCToXML(VC,VE,NULL,FullXML);

    /* This function appends, just pass in Buf */

    MXMLToMString(VE,Buf,NULL,TRUE);

    MXMLDestroyE(&VE);
    }

  return(SUCCESS);
  }  /* END __MVCPrintOneVC() */



