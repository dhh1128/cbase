/* HEADER */

/**
 * @file MVCSetAttr.c
 *
 * Contains MVCSetAttr(), for setting attributes on Virtual Containers.
 */

#include <assert.h>

#include "moab.h"
#include "moab-proto.h"
#include "moab-global.h"
#include "moab-const.h"


/**
 *  Set an attribute on a VC.
 *
 *  Jobs, nodes, VMs, rsvs, and sub-VCs (children objects) must be comma-delimited lists of object names.
 *  Also, for the objects just listed, it is faster to just call MVC(Add/Remove)Object directly, and this
 *  function will be mostly for reading in checkpoint files.  For other attributes, this function is ok
 *  to call in runtime.
 *
 * @param VC      (I) [modified] - The VC
 * @param AIndex  (I) - Which attribute
 * @param Value   (I) - The value to set to (NULL can be valid for some attrs)
 * @param Format  (I) - Format of Value
 * @param Mode    (I) - Set, Unset, etc.
 */

int MVCSetAttr(
  mvc_t                  *VC,
  enum MVCAttrEnum        AIndex,
  void                  **Value,
  enum MDataFormatEnum    Format,
  enum MObjectSetModeEnum Mode)

  {
  const char *FName = "MVCSetAttr";

  MDB(7,fSTRUCT) MLog("%s(%s,%s,%s,%s,%s)\n",
    FName,
    (VC != NULL) ? VC->Name : "NULL",
    MVCAttr[AIndex],
    ((Format == mdfString) && (Value != NULL)) ? (char *)Value : "Value",
    MDFormat[Format],
    MObjOpType[Mode]);

  if (VC == NULL)
    {
    return(FAILURE);
    }

  switch (AIndex)
    {
    case mvcaACL:

      {
      macl_t *ACLIter;
      macl_t **tmpACL;
      int rc = SUCCESS;

      tmpACL = &VC->ACL;

      if ((Value == NULL) ||
          (strcmp((char *)Value,NONE) == 0))
        {
        /* clear */

        if ((Mode == mSet) || (Mode == mNONE2) || (Mode == mUnset))
          MACLFreeList(tmpACL);

        break;
        }
      else if (Format != mdfString)
        {
        if ((Mode == mIncr) || (Mode == mAdd))
          {
          MACLMerge(tmpACL,(macl_t *)Value);
          }
        else
          {
          MACLCopy(tmpACL,(macl_t *)Value);
          }
        }  /* END else if (Format != mdfString) */
      else if (strcmp((char *)Value,NONE) != 0)
        {

        if (((char *)Value)[0] == '\0')
          {
          /* clear */

          if ((Mode == mSet) || (Mode == mNONE2))
            MACLFreeList(tmpACL);
          }
        else if ((Mode == mUnset) || (Mode == mDecr))
          {
          macl_t *tPtr = NULL;

          /* remove specified ACL's */

          char *tmpBuffer = NULL;

          MUStrDup(&tmpBuffer,(char *)Value);

          rc = MACLLoadConfigLine(&tPtr,tmpBuffer,mSet,NULL,FALSE);

          MUFree(&tmpBuffer);

          if (rc != FAILURE)
            MACLSubtract(tmpACL,tPtr);

          MACLFreeList(&tPtr);
          }    /* END else if ((Mode == mUnset) || (Mode == mDecr)) */
        else
          {
          /* Set or increment */

          char *tmpBuffer = NULL;

          MUStrDup(&tmpBuffer,(char *)Value);

          /* FORMAT:  <ACLTYPE>==<ACLVALUE>[,<ACLTYPE>==<ACLVALUE>]... */

          if ((Mode == mIncr) || (Mode == mAdd))
            {
            rc = MACLLoadConfigLine(tmpACL,tmpBuffer,Mode,NULL,FALSE);
            }
          else
            {
            MACLFreeList(tmpACL);

            rc = MACLLoadConfigLine(tmpACL,tmpBuffer,Mode,NULL,FALSE);
            }

          MUFree(&tmpBuffer);
          }
        }  /* END else if (strcmp((char *)Value,NONE)) */

      if (rc == FAILURE)
        return(FAILURE);

      /* For VCs, affinity should always is positive (it just grants access) */

      for (ACLIter = VC->ACL;ACLIter != NULL;ACLIter = ACLIter->Next)
        {
        ACLIter->Affinity = mnmPositiveAffinity;
        }
      }  /* END case mraACL */

      break;

    case mvcaCreateJob:

      if ((Value == NULL) || (Format != mdfString))
        return(FAILURE);

      MUStrDup(&VC->CreateJob,(char *)Value);

      break;

    case mvcaCreateTime:

      if (Value == NULL)
        return(FAILURE);

      if (Format == mdfString)
        {
        VC->CreateTime = (mulong)strtol((char *)Value,NULL,10); 
        }
      else
        {
        VC->CreateTime = *(mulong *)Value;
        }

      break;

    case mvcaCreator:

      if ((Format != mdfString) || (Value == NULL))
        {
        return(FAILURE);
        }

      if (MUserFind((char *)Value,&VC->Creator) == FAILURE)
        return(FAILURE);

      break;

    case mvcaDescription:

      if (Format != mdfString)
        {
        return(FAILURE);
        }

      if ((Value != NULL) && (Mode != mUnset))
        {
        MUStrDup(&VC->Description,(char *)Value);
        }
      else
        {
        /* Value was NULL or mode was unset, set to VC->Name 
            A VC must ALWAYS have a name because that is what
            Viewpoint will display */

        MUFree(&VC->Description);

        if (VC->Name[0] != '\0')
          MUStrDup(&VC->Description,VC->Name);
        }

      /* END case mvcaDescription */

      break;

    case mvcaFlags:

      if (Format == mdfString)
        {
        mbitmap_t tmpL;

        if (Mode == mClear)
          {
          bmclear(&VC->Flags);

          break;
          }

        if (Value == NULL)
          return(FAILURE);

        bmfromstring((char *)Value,MVCFlags,&tmpL,",:");

        if ((Mode == mSet) || (Mode == mSetOnEmpty))
          {
          bmcopy(&VC->Flags,&tmpL);
          }
        else if ((Mode == mAdd) || (Mode == mIncr))
          {
          bmor(&VC->Flags,&tmpL);
          }
        else if ((Mode == mDecr) || (Mode == mUnset))
          {
          int index;

          for (index = 0;index < mvcfLAST;index++)
            {
            if (bmisset(&tmpL,index))
              bmunset(&VC->Flags,index);
            }
          }

        bmclear(&tmpL);
        }  /* END if (Format == mdfString) */
      else
        {
        /* Invalid format */

        return(FAILURE);
        }

      /* END case mvcaFlags */

      break;

    case mvcaJobs:

      if (Format != mdfString)
        {
        return(FAILURE);
        }

      {
      char *ptr;
      char *TokPtr = NULL;
      mjob_t *J;

      if ((Mode == mSet) || (Mode == mClear))
        {
        mln_t *tmpL;

        for (tmpL = VC->Jobs;tmpL != NULL;)
          {
          J = (mjob_t *)tmpL->Ptr;

          tmpL = tmpL->Next;

          if ((Mode == mClear) || (!MUStrIsInList((char *)Value,J->Name,',')))
            MVCRemoveObject(VC,(void *)J,mxoJob);
          }
        }

      if (Mode == mClear)
        break;

      ptr = MUStrTok((char *)Value,",",&TokPtr);

      while (ptr != NULL)
        {
        if (MJobFind(ptr,&J,mjsmExtended) == SUCCESS)
          {
          if ((Mode == mSet) || (Mode == mAdd))
            {
            MVCAddObject(VC,(void *)J,mxoJob);
            }
          else if (Mode == mUnset)
            {
            MVCRemoveObject(VC,(void *)J,mxoJob);
            }
          }

        ptr = MUStrTok(NULL,",",&TokPtr);
        }
      }  /* END case mvcaJobs */

      break;

    case mvcaNodes:

      if (Format != mdfString)
        {
        return(FAILURE);
        }

      {
      char *ptr;
      char *TokPtr = NULL;
      mnode_t *N;

      if ((Mode == mSet) || (Mode == mClear))
        {
        mln_t *tmpL;

        for (tmpL = VC->Nodes;tmpL != NULL;)
          {
          N = (mnode_t *)tmpL->Ptr;

          tmpL = tmpL->Next;

          if ((Mode == mClear) || (!MUStrIsInList((char *)Value,N->Name,',')))
            MVCRemoveObject(VC,(void *)N,mxoNode);
          }
        }

      if (Mode == mClear)
        break;

      ptr = MUStrTok((char *)Value,",",&TokPtr);

      while (ptr != NULL)
        {
        if (MNodeFind(ptr,&N) == SUCCESS)
          {
          if ((Mode == mSet) || (Mode == mAdd))
            {
            MVCAddObject(VC,(void *)N,mxoNode);
            }
          else if (Mode == mUnset)
            {
            MVCRemoveObject(VC,(void *)N,mxoNode);
            }
          }

        ptr = MUStrTok(NULL,",",&TokPtr);
        }
      }  /* END case mvcaNodes */

      break;

    case mvcaOwner:

      /* FORMAT: <TYPE>:<NAME> */

      {
      mgcred_t *CredPtr = NULL;

      char *TokPtr;
      char *ptr;
      char tmpValue[MMAX_LINE];
      enum MXMLOTypeEnum OwnerType;

      if (Format != mdfString)
        {
        return(FAILURE);
        }

      MUStrCpy(tmpValue,(char *)Value,sizeof(tmpValue));

      ptr = MUStrTokE(tmpValue,":",&TokPtr);

      if (ptr == NULL)
        {
        return(FAILURE);
        }

      OwnerType = (enum MXMLOTypeEnum)MUGetIndexCI(
                  ptr,
                  MXO,
                  FALSE,
                  mxoNONE);

      ptr = MUStrTokE(NULL,",",&TokPtr);

      if ((ptr == NULL) ||
          (OwnerType == mxoNONE))
        {
        return(FAILURE);
        }

      /* Check for valid OwnerType and if Owner exists */

      switch (OwnerType)
        {
        case mxoAcct:

          if (MAcctFind(ptr,&CredPtr) == FAILURE)
            return(FAILURE);

          break;

        case mxoGroup:

          if (MGroupFind(ptr,&CredPtr) == FAILURE)
            return(FAILURE);

          break;

        case mxoUser:

          if (MUserFind(ptr,&CredPtr) == FAILURE)
            return(FAILURE);

          break;

        default:

          /* Invalid owner type */

          return(FAILURE);

          /*NOTREACHED*/

          break;
        }  /* END switch (OwnerType) */

      /* Everything checks out, set owner */

      MUStrDup(&VC->Owner,ptr);
      VC->OwnerType = OwnerType;

      mstring_t ACLStr(MMAX_LINE);
      MStringSetF(&ACLStr,"%s==%s",
        MXO[OwnerType],
        ptr);

      MVCSetAttr(VC,mvcaACL,(void **)ACLStr.c_str(),mdfString,mAdd);
      }  /* END case mvcaOwner */

      break;

   case mvcaParentVCs:

      if (Format != mdfString)
        {
        return(FAILURE);
        }

      {
      char *ptr = NULL;
      char *TokPtr = NULL;
      mvc_t *tmpVC;

      if ((Mode == mSet) || (Mode == mClear))
        {
        mln_t *tmpL;

        for (tmpL = VC->ParentVCs;tmpL != NULL;)
          {
          tmpVC = (mvc_t *)tmpL->Ptr;
          tmpL = tmpL->Next;

          /* Don't remove the parent if is going to be set later.
              It would be problematic if this VC is the only child and
              the parent has DestroyWhenEmpty set */

          if ((Mode == mClear) || (!MUStrIsInList((char *)Value,tmpVC->Name,',')))
            MVCRemoveObject(tmpVC,(void *)VC,mxoxVC);
          }  /* END for (tmpL = VC->ParentVCs;tmpL != NULL;) */
        }  /* END if ((Mode == mSet) || (Mode == mClear)) */

      if (Mode == mClear)
        break;

      ptr = MUStrTok((char *)Value,",",&TokPtr);

      while (ptr != NULL)
        {
        if (MVCFind(ptr,&tmpVC) == SUCCESS)
          {
          if ((Mode == mSet) || (Mode == mAdd))
            {
            MVCAddObject(tmpVC,(void *)VC,mxoxVC);
            }
          else if (Mode == mUnset)
            {
            MVCRemoveObject(tmpVC,(void *)VC,mxoxVC);
            }
          }

        ptr = MUStrTok(NULL,",",&TokPtr);
        }
      }  /* END case mvcaParentVCs */

      break;

    case mvcaReqNodeSet:

      {
      if ((Format != mdfString) ||
          (Value == NULL))
        return(FAILURE);

      if (MVCSetDynamicAttr(VC,mvcaReqNodeSet,(void **)Value,mdfString) == FAILURE)
        return(FAILURE);
      }

      break;

    case mvcaReqStartTime:

      {
      if (Value == NULL)
        return(FAILURE);

      if (Format == mdfMulong)
        {
        if (MVCSetDynamicAttr(VC,mvcaReqStartTime,(void **)Value,mdfMulong) == FAILURE)
          return(FAILURE);
        }
      else if (Format == mdfString)
        {
        /* Need to convert to mulong */

        mulong StartTime = 0;

        StartTime = (mulong)MUTimeFromString((char *)Value);

        if (strchr((char *)Value,'+') != NULL)
          StartTime += MSched.Time;

        if (StartTime > 0)
          {
          if (MVCSetDynamicAttr(VC,mvcaReqStartTime,(void **)&StartTime,mdfMulong) == FAILURE)
            return(FAILURE);
          }
        }  /* END else if (Format == mdfString) */
      else
        {
        /* unsupported type */

        return(FAILURE);
        }
      }

      break;

    case mvcaRsvs:

      if (Format != mdfString)
        {
        return(FAILURE);
        }

      {
      char *ptr;
      char *TokPtr = NULL;
      mrsv_t *R;

      if ((Mode == mSet) || (Mode == mClear))
        {
        mln_t *tmpL;

        for (tmpL = VC->Rsvs;tmpL != NULL;)
          {
          R = (mrsv_t *)tmpL->Ptr;

          tmpL = tmpL->Next;

          if ((Mode == mClear) || (!MUStrIsInList((char *)Value,R->Name,',')))
            MVCRemoveObject(VC,(void *)R,mxoRsv);
          }
        }

      if (Mode == mClear)
        break;

      ptr = MUStrTok((char *)Value,",",&TokPtr);

      while (ptr != NULL)
        {
        if (MRsvFind(ptr,&R,mraNONE) == SUCCESS)
          {
          if ((Mode == mSet) || (Mode == mAdd))
            {
            MVCAddObject(VC,(void *)R,mxoRsv);
            }
          else if (Mode == mUnset)
            {
            MVCRemoveObject(VC,(void *)R,mxoRsv);
            }
          }

        ptr = MUStrTok(NULL,",",&TokPtr);
        }
      }  /* END case mvcaRsvs */

      break;

    case mvcaThrottlePolicies:

      if (Format != mdfString)
        {
        return(FAILURE);
        }

      {
      char *ptr;
      char *TokPtr = NULL;

      mcredl_t *L = NULL;

      /* FORMAT: MAX{X}=Y */

      if (Mode != mSet)
        {
        break;
        }

      if (MVCGetDynamicAttr(VC,mvcaThrottlePolicies,(void **)&L,mdfOther) == FAILURE)
        {
        L = (mcredl_t *)MUCalloc(1,sizeof(mcredl_t));

        MPUInitialize(&L->ActivePolicy);

        MVCSetDynamicAttr(VC,mvcaThrottlePolicies,(void **)&L,mdfOther);
        }

      ptr = MUStrTokEPlus((char *)Value,":",&TokPtr);

      while (ptr != NULL)
        {
        MCredProcessConfig(VC,mxoxVC,NULL,ptr,L,NULL,FALSE,NULL);

        ptr = MUStrTokEPlus(NULL,":",&TokPtr);
        }
      } /* END case mvcaThrottlePolicies */

      break;

    case mvcaVariables:

      if (Format != mdfString)
        {
        return(FAILURE);
        }

      {
      char *ptr;
      char *TokPtr = NULL;
      char *ptr2;
      char *TokPtr2 = NULL;

      if ((Mode == mSet) || (Mode == mClear))
        {
        MUHTFree(&VC->Variables,TRUE,MUFREE);
        }

      if (Mode == mClear)
        break;

      ptr = MUStrTokEPlus((char *)Value,":",&TokPtr);

      while (ptr != NULL)
        {
        ptr2 = MUStrTok(ptr,"=",&TokPtr2);

        if ((Mode == mSet) || (Mode == mAdd))
          {
          char *VarName = ptr2;

          ptr2 = MUStrTok(NULL,"=",&TokPtr2);

          if (ptr2 != NULL)
            {
            MUHTAdd(&VC->Variables,VarName,strdup(ptr2),NULL,MUFREE);
            }
          else
            {
            MUHTAdd(&VC->Variables,VarName,NULL,NULL,MUFREE);
            }
          }
        else if ((Mode == mUnset) || (Mode == mDecr))
          {
          MUHTRemove(&VC->Variables,ptr2,MUFREE);
          }

        ptr = MUStrTokEPlus(NULL,":",&TokPtr);
        }
      }  /* END case mvcaVariables */

      break;

    case mvcaVCs:

      if (Format != mdfString)
        {
        return(FAILURE);
        }

      {
      char *ptr = NULL;
      char *TokPtr = NULL;
      mvc_t *tmpVC;

      if ((Mode == mSet) || (Mode == mClear))
        {
        mln_t *tmpL;

        for (tmpL = VC->VCs;tmpL != NULL;)
          {
          tmpVC = (mvc_t *)tmpL->Ptr;

          tmpL = tmpL->Next;

          /* Don't remove a VC if in the set list (could be only child) */

          if ((Mode == mClear) || (!MUStrIsInList((char *)Value,tmpVC->Name,',')))
            MVCRemoveObject(VC,(void *)tmpVC,mxoxVC);
          }
        }

      if (Mode == mClear)
        break;

      ptr = MUStrTok((char *)Value,",",&TokPtr);

      while (ptr != NULL)
        {
        if (MVCFind(ptr,&tmpVC) == SUCCESS)
          {
          if ((Mode == mSet) || (Mode == mAdd))
            {
            MVCAddObject(VC,(void *)tmpVC,mxoxVC);
            }
          else if (Mode == mUnset)
            {
            MVCRemoveObject(VC,(void *)tmpVC,mxoxVC);
            }
          }

        ptr = MUStrTok(NULL,",",&TokPtr);
        }
      }  /* END case mvcaVCs */

      break;

    case mvcaVMs:

      if (Format != mdfString)
        {
        return(FAILURE);
        }

      {
      char *ptr;
      char *TokPtr = NULL;
      mvm_t *VM;

      if ((Mode == mSet) || (Mode == mClear))
        {
        mln_t *tmpL;

        for (tmpL = VC->VMs;tmpL != NULL;)
          {
          VM = (mvm_t *)tmpL->Ptr;

          tmpL = tmpL->Next;

          if ((Mode == mClear) || (!MUStrIsInList((char *)Value,VM->VMID,',')))
            MVCRemoveObject(VC,(void *)VM,mxoxVM);
          }
        }

      if (Mode == mClear)
        break;

      ptr = MUStrTok((char *)Value,",",&TokPtr);

      while (ptr != NULL)
        {
        if (MVMFind(ptr,&VM) == SUCCESS)
          {
          if ((Mode == mSet) || (Mode == mAdd))
            {
            MVCAddObject(VC,(void *)VM,mxoxVM);
            }
          else if (Mode == mUnset)
            {
            MVCRemoveObject(VC,(void *)VM,mxoxVM);
            }
          }

        ptr = MUStrTok(NULL,",",&TokPtr);
        }
      }  /* END case mvcaVMs */

      break;

    case mvcaName:
    default:

      /* NOT HANDLED */

      return(FAILURE);

      /*NOTREACHED*/

      break;
    }  /* END switch (AIndex) */

  MVCTransition(VC);

  return(SUCCESS);
  }  /* END MVCSetAttr() */


