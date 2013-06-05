/* HEADER */

/**
 * @file MVCString.c
 *
 * Contains Virtual Container (VC) string related functions.
 */

#include <assert.h>

#include "moab.h"
#include "moab-proto.h"
#include "moab-global.h"
#include "moab-const.h"




/**
 *  Report a VC attribute as mstring_t.
 *
 *  The mstring_t that is passed in (Buf) should be initialized before calling this function.
 *
 * @param VC     (I) - the VC
 * @param AIndex (I) - which attribute
 * @param Buf    (O) - output buffer
 */

int MVCAToMString(

  mvc_t               *VC,     /* I */
  enum MVCAttrEnum     AIndex, /* I */
  mstring_t           *Buf)    /* O */
  {

  mln_t *tmpL = NULL;

  if ((VC == NULL) || (Buf == NULL))
    return(FAILURE);

  MStringSet(Buf,"");

  switch (AIndex)
    {
    case mvcaACL:

      {
      mbitmap_t BM;

      bmset(&BM,mfmAVP);
      bmset(&BM,mfmVerbose);

      MACLListShowMString(VC->ACL,maNONE,&BM,Buf);

      if (!strcmp(Buf->c_str(),NONE))
        {
        MStringSet(Buf,"");
        }
      }

      break;

    case mvcaCreateJob:

      if (VC->CreateJob != NULL)
        MStringSet(Buf,VC->CreateJob);

      break;

    case mvcaCreateTime:

      /* CreateTime should ALWAYS be set */

      MStringSetF(Buf,"%ld",VC->CreateTime);

      break;

    case mvcaCreator:

      if (VC->Creator != NULL)
        MStringSet(Buf,VC->Creator->Name);

      break;

    case mvcaDescription:

      if (VC->Description != NULL)
        {
        MStringSet(Buf,VC->Description);
        }

      break;

    case mvcaFlags:

      bmtostring(&VC->Flags,MVCFlags,Buf);

      break;

    case mvcaJobs:

      for (tmpL = VC->Jobs;tmpL != NULL;tmpL = tmpL->Next)
        {
        mjob_t *J = (mjob_t *)tmpL->Ptr;

        if (Buf->empty())
          {
          MStringSet(Buf,J->Name);
          }
        else
          {
          MStringAppend(Buf,",");
          MStringAppend(Buf,J->Name);
          }
        }

      break;

    case mvcaMessages:

      {
      if (VC->MB != NULL)
        {
        char  tmpLine[MMAX_LINE];
        char *BPtr = NULL;
        int   BSpace;

        MUSNInit(&BPtr,&BSpace,tmpLine,MMAX_LINE);

        MMBPrintMessages(VC->MB,mfmHuman,TRUE,-1,(char *)BPtr,BSpace);

        MUSNUpdate(&BPtr,&BSpace);

        MStringSet(Buf,tmpLine);
        }
      }

      break;

    case mvcaName:

      MStringSet(Buf,VC->Name);

      break;

    case mvcaNodes:

      for (tmpL = VC->Nodes;tmpL != NULL;tmpL = tmpL->Next)
        {
        mnode_t *N = (mnode_t *)tmpL->Ptr;

        if (Buf->empty())
          {
          MStringSet(Buf,N->Name);
          }
        else
          {
          MStringAppend(Buf,",");
          MStringAppend(Buf,N->Name);
          }
        }

      break;

    case mvcaOwner:

      if ((VC->Owner != NULL) && (VC->OwnerType != mxoNONE))
        {
        MStringSetF(Buf,"%s:%s",
          MXO[VC->OwnerType],
          VC->Owner);
        }

      break;

    case mvcaParentVCs:

      for (tmpL = VC->ParentVCs;tmpL != NULL;tmpL = tmpL->Next)
        {
        mvc_t *tmpVC = (mvc_t *)tmpL->Ptr;

        if (Buf->empty())
          {
          MStringSet(Buf,tmpVC->Name);
          }
        else
          {
          MStringAppend(Buf,",");
          MStringAppend(Buf,tmpVC->Name);
          }
        }

      break;

    case mvcaReqNodeSet:

      {
      char *tmpAttr;

      if (MVCGetDynamicAttr(VC,mvcaReqNodeSet,(void **)&tmpAttr,mdfString) == FAILURE)
        return(FAILURE);

      MStringSet(Buf,tmpAttr);

      MUFree(&tmpAttr);
      }

      break;

    case mvcaReqStartTime:
      {
      mulong tmpAttr;

      if (MVCGetDynamicAttr(VC,mvcaReqStartTime,(void **)&tmpAttr,mdfMulong) == FAILURE)
        return(FAILURE);

      MStringSetF(Buf,"%ld",tmpAttr);
      }

      break;

    case mvcaRsvs:

      for (tmpL = VC->Rsvs;tmpL != NULL;tmpL = tmpL->Next)
        {
        mrsv_t *R = (mrsv_t *)tmpL->Ptr;

        if (Buf->empty())
          {
          MStringSet(Buf,R->Name);
          }
        else
          {
          MStringAppend(Buf,",");
          MStringAppend(Buf,R->Name);
          }
        }

      break;

    case mvcaThrottlePolicies:

      {
      mcredl_t *L = NULL;

      int caindex;

      enum MActivePolicyTypeEnum PList[] = {
        mptMaxJob,
        mptMaxPS,
        mptMaxNode,
        mptMaxProc,
        mptMaxPE,
        mptNONE };

      char *ptr;

      if (MVCGetDynamicAttr(VC,mvcaThrottlePolicies,(void **)&L,mdfOther) == FAILURE)
        {
        break;
        }

      for (caindex = 0;PList[caindex] != mptNONE;caindex++)
        {
        if ((ptr = MCredShowLimit(&L->ActivePolicy,PList[caindex],0,FALSE)) != NULL)
          {
          MStringAppendF(Buf,"%s=%s:",
            MPolicyType[PList[caindex]],
            ptr);
          }
        }  /* END for (caindex) */
      }  /* END case mvcaThrottlePolicies */

      break;

    case mvcaVariables:

      if (MUHTToMStringDelim(&VC->Variables,Buf,":") == FAILURE)
        {
        return(FAILURE);
        }

      break;

    case mvcaVCs:

      for (tmpL = VC->VCs;tmpL != NULL;tmpL = tmpL->Next)
        {
        mvc_t *tmpVC = (mvc_t *)tmpL->Ptr; 

        if (Buf->empty())
          {
          MStringSet(Buf,tmpVC->Name);
          }
        else
          {
          MStringAppend(Buf,",");
          MStringAppend(Buf,tmpVC->Name);
          }
        }

      break;

    case mvcaVMs:

      for (tmpL = VC->VMs;tmpL != NULL;tmpL = tmpL->Next)
        {
        mvm_t *VM = (mvm_t *)tmpL->Ptr;

        if (Buf->empty())
          {
          MStringSet(Buf,VM->VMID);
          }
        else
          {
          MStringAppend(Buf,",");
          MStringAppend(Buf,VM->VMID);
          }
        }

      break;

    default:

      /* not handled */

      return(FAILURE);

      /*NOTREACHED*/

      break;
    }

  return(SUCCESS);
  }  /* END MVCAToMString() */







/**
 * Populates the VC with the XML info from Buf.
 *
 * NOTE: all objects should be loaded before this is called.
 *
 * @see MCPSecondPass() - parent
 * @see MVCFromXML() - child
 *
 * @param VC  [I] (modified)
 * @param Buf [I] - string of XML description of VC
 */

int MVCFromString(
  mvc_t *VC,
  char  *Buf)

  {
  int     rc;
  mxml_t *E = NULL;

  const char *FName = "MVCFromString";

  MDB(7,fSTRUCT) MLog("%s(%s,%s)\n",
    FName,
    (VC != NULL) ? VC->Name : "NULL",
    (Buf != NULL) ? Buf : "NULL");

  if ((VC == NULL) || (Buf == NULL))
    {
    return(FAILURE);
    }

  rc = MXMLFromString(&E,Buf,NULL,NULL);

  if (rc == SUCCESS)
    {
    rc = MVCFromXML(VC,E);
    }

  MXMLDestroyE(&E);

  return(rc);
  }  /* END MVCFromString() */



