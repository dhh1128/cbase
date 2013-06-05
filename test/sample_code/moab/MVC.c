/* HEADER */

/**
 * @file MVC.c
 *
 * Contains Virtual Container (VC) related functions.
 */

#include <assert.h>

#include "moab.h"
#include "moab-proto.h"
#include "moab-global.h"
#include "moab-const.h"

/* Local prototypes */

int __MVCGenerateName(char *);
int __MVCGetObjectByName(const char *,enum MXMLOTypeEnum,void **);
mbool_t __MVCJobCanBeCancelled(mjob_t *);
int __MVCGetNodeSetRecursive(mvc_t *,char **);

/* END Local prototypes */


/**
 * Allocate a VC.
 *  The name of a VC is always auto-generated.  If a description is given, that
 *  will be used for the VC description.  If not, the generated name will be
 *  used as the description as well.
 *
 *  If the VC was created, it will also be added into the global MVC hashtable.
 *
 *  Forcing the name is only to happen with checkpointing
 *
 * Returns FAILURE if failed to create the VC or if a VC of that name already
 *  exists.
 *
 * @param NewVC (O) - The VC that will be allocated
 * @param Desc  (I) [optional] - Description for VC
 * @param ForceName (I) [optional] -> Forces the name of the VC to this string
 */

int MVCAllocate(

  mvc_t       **NewVC,
  const char   *Desc,
  const char   *ForceName)

  {
  mvc_t *VC;

  const char *FName = "MVCAllocate";

  MDB(5,fSTRUCT) MLog("%s(Ptr,%s,%s)\n",
    FName,
    (Desc != NULL) ? Desc : "NULL",
    (ForceName != NULL) ? ForceName : "NULL");

  if (NewVC == NULL)
    {
    return(FAILURE);
    }

  *NewVC = NULL;

  if ((VC = (mvc_t *)MUCalloc(1,sizeof(mvc_t))) == NULL)
    return(FAILURE);

  memset(VC,0,sizeof(mvc_t));

  /* generate VC name */

  if ((ForceName != NULL) &&
      (ForceName[0] != '\0'))
    {
    MUStrCpy(VC->Name,ForceName,sizeof(VC->Name));
    }
  else
    {
    __MVCGenerateName(VC->Name);
    }

  MDB(5,fSTRUCT) MLog("INFO:     assigning VC name '%s'\n",
    VC->Name);

  MUHTAdd(&MVC,VC->Name,(void **)VC,NULL,NULL);

  if ((Desc != NULL) && (Desc[0] != '\0'))
    {
    MVCSetAttr(VC,mvcaDescription,(void **)Desc,mdfString,mSet);
    }
  else
    {
    MVCSetAttr(VC,mvcaDescription,(void **)VC->Name,mdfString,mSet);
    }

  VC->CreateTime = MSched.Time;

  *NewVC = VC;

  /* VC was already transitioned by the MVCSetAttr call above */

  return(SUCCESS);
  }  /* END MVCAllocate() */


/**
 * Returns SUCCESS if there is a VC with given name.  If VCPtr is not NULL,
 *  it will be set to VC that was found.
 *
 * @param Name  (I) - Name to search for (VC->Name)
 * @param VCPtr (O) [optional] - Returned pointer
 */

int MVCFind(

  const char *Name,
  mvc_t     **VCPtr)

  {
  /* MUHTGet will check the name and init the pointer to NULL,
      so we'll just let it do the work */

  return MUHTGet(&MVC,Name,(void **)VCPtr,NULL);
  }  /* END MVCFind() */




/**
 * Generates a unique VC name.
 *
 * Should only be used on VC->Name in MVCAllocate().
 *
 * @param Name [O] - Name to populate
 */

int __MVCGenerateName(

  char *Name)  /* O (minsize = MMAX_NAME) */

  {
  if (Name == NULL)
    return(FAILURE);

  do
    {
    snprintf(Name,MMAX_NAME,"vc%d",
      ++MSched.VCIDCounter);
    } while (MVCFind(Name,NULL) == SUCCESS);

  return(SUCCESS);
  }  /* END MVCGenerateName() */


/**
 * Helper function to tell if a job can be cancelled by a VC.
 *
 * @param J [I] - the job to be cancelled
 */

mbool_t __MVCJobCanBeCancelled(

  mjob_t *J)

  {
  if (J == NULL)
    return(FALSE);

  if ((J->System != NULL) && (MJOBISACTIVE(J)))
    {
    switch (J->System->JobType)
      {
      case msjtVMTracking:
      /*case msjtVMCreate:*/

        return(TRUE);

        break;

      default:

        return(FALSE);

        break;
      }
    }  /* END if ((tmpJ->System != NULL) && (MJOBISACTIVE(tmpJ))) */

  return(TRUE);
  }  /* END __MVCJobCanBeCancelled() */



/**
 * Add VC Child to VC Parent
 *
 * @param Child  [I] - The VC to be added as a child
 * @param Parent [I] - (modified) The VC to be the parent
 * @param EMsg   [O] - Error message buffer (minsize = MMAX_LINE)
 */

int MVCAddToVC(

  mvc_t *Child,
  mvc_t *Parent,
  char  *EMsg)

  {
  if ((Child == NULL) || (Parent == NULL) || (EMsg == NULL))
    {
    return(FAILURE);
    }

  if ((bmisset(&Parent->Flags,mvcfDeleting)) ||
      (bmisset(&Child->Flags,mvcfDeleting)))
    {
    snprintf(EMsg,MMAX_LINE,"ERROR:  cannot add objects to a VC that is being deleted\n");

    return(FAILURE);
    }

  if (MVCHasObject(Parent,Child->Name,mxoxVC))
    {
    return(SUCCESS);
    }

  if (MVCAddObject(Parent,Child,mxoxVC) == FAILURE)
    {
    snprintf(EMsg,MMAX_LINE,"ERROR:  failed to add %s to vc '%s'\n",
      Child->Name,
      Parent->Name);

    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MVCAddToVC() */



/**
 * Removes the VC from any of its children and frees it.
 *  This also also remove/free Sub-VCs!!!
 *
 * RemoveSubObjects should usually be FALSE, set the DestroyObjects flag on the VC instead
 *
 * @see MVCFree (child)
 *
 * May also be called recursively by MVCFree (freeing sub-VCs)
 *
 * @param VCP (I) - The VC to remove from Moab
 * @param RemoveSubObjects (I) - Remove VC's rsvs, jobs, and VMs
 * @param WaitForEmpty (I) - If TRUE, the VC will not be destroyed until all objects have been successfully removed
 * @param Auth         (I) - [optional] User that destroyed this VC (used for destroying sub-jobs)
 */

int MVCRemove(

  mvc_t     **VCP,
  mbool_t     RemoveSubObjects,
  mbool_t     WaitForEmpty,
  const char *Auth)

  {
  mln_t *tmpL = NULL;
  mvc_t *VC;

  mbool_t DeleteObjects = FALSE;
  char    RemoveMsg[MMAX_LINE];

  const char *FName = "MVCRemove";

  MDB(5,fSTRUCT) MLog("%s(%s,%s,%s)\n",
    FName,
    ((VCP != NULL) && (*VCP != NULL)) ? (*VCP)->Name : "NULL",
    MBool[RemoveSubObjects],
    MBool[WaitForEmpty]);

  if (VCP == NULL)
    return(FAILURE);

  VC = *VCP;

  if (VC == NULL)
    return(SUCCESS);

  /* WaitForEmpty checking/setup */

  if ((WaitForEmpty) && (bmisset(&VC->Flags,mvcfDeleting)))
    return(SUCCESS);

  if (WaitForEmpty)
    bmset(&VC->Flags,mvcfDestroyWhenEmpty);

  if (bmisset(&VC->IFlags,mvcifRemoved))
    return(SUCCESS);

  if (((bmisset(&VC->Flags,mvcfDestroyObjects)) || (RemoveSubObjects == TRUE)) &&
      (MSched.Shutdown != TRUE) &&
      (!bmisset(&VC->Flags,mvcfDeleting)))
    {
    /* Destroy jobs, rsvs, and VMs that belong to this VC, but not on shutdown */

    DeleteObjects = TRUE;

    snprintf(RemoveMsg,sizeof(RemoveMsg),"Removed by VC '%s'",
      VC->Name);
    }

  /* Set removal flags */
  bmset(&VC->Flags,mvcfDeleting);  /* VC has been told to delete */
  bmset(&VC->IFlags,mvcifRemoved); /* VC is currently being removed (to avoid double-frees) */

  bmset(&VC->IFlags,mvcifDontTransition);

  /* with the below for loops, start at the top because we will be 
      removing items */

  if (VC->Jobs != NULL)
    {
    mjob_t *tmpJ;

    if (WaitForEmpty == TRUE)
      {
      for (tmpL = VC->Jobs;tmpL != NULL;)
        {
        tmpJ = (mjob_t *)tmpL->Ptr;

        tmpL = tmpL->Next;

        if (!MJobPtrIsValid(tmpJ))
          {
          continue;
          }

        if ((tmpJ->TemplateExtensions != NULL) &&
            (tmpJ->TemplateExtensions->TJobAction != mtjatNONE))
          {
          /* Job is performing an action (migrate, destroy, etc.)
              Don't destroy, can be part of the tear-down. */

          continue;
          }

        if (DeleteObjects)
          {
          /* Job will be removed from VC when it finishes */

          if (__MVCJobCanBeCancelled(tmpJ) == FALSE)
            continue;
          else
            MJobCancelWithAuth(Auth,tmpJ,RemoveMsg,FALSE,NULL,NULL);
          }
        else
          {
          MULLRemove(&tmpJ->ParentVCs,VC->Name,NULL);
          MULLRemove(&VC->Jobs,tmpJ->Name,NULL);
          }
        }
      }
    else
      {
      for (tmpL = VC->Jobs;tmpL != NULL;)
        {
        tmpJ = (mjob_t *)tmpL->Ptr;
        tmpL = tmpL->Next;

        if (!MJobPtrIsValid(tmpJ))
          {
          continue;
          }

        if (DeleteObjects)
          {
          if (__MVCJobCanBeCancelled(tmpJ) == TRUE)
            MJobCancelWithAuth(Auth,tmpJ,RemoveMsg,FALSE,NULL,NULL);
          }

        MULLRemove(&tmpJ->ParentVCs,VC->Name,NULL);
        MULLRemove(&VC->Jobs,tmpJ->Name,NULL);
        }  /* END for (tmpL = VC->Jobs;tmpL != NULL;) */
      }  /* END else -- (WaitForEmtpy == FALSE) */
    }  /* END if (VC->Jobs != NULL) */

  if (VC->Nodes != NULL)
    {
    mnode_t *tmpN;

    for (tmpL = VC->Nodes;tmpL != NULL;)
      {
      tmpN = (mnode_t *)tmpL->Ptr;
      tmpL = tmpL->Next;

      MULLRemove(&tmpN->ParentVCs,VC->Name,NULL);
      MULLRemove(&VC->Nodes,tmpN->Name,NULL);
      }
    }

  if (VC->Rsvs != NULL)
    {
    mrsv_t *tmpR;

    for (tmpL = VC->Rsvs;tmpL != NULL;)
      {
      tmpR = (mrsv_t *)tmpL->Ptr;
      tmpL = tmpL->Next;

      MULLRemove(&tmpR->ParentVCs,VC->Name,NULL);
      MULLRemove(&VC->Rsvs,tmpR->Name,NULL);

      if (DeleteObjects)
        MRsvDestroy(&tmpR,TRUE,FALSE);
      }
    }  /* END if (VC->Rsvs != NULL) */

  if (VC->VMs != NULL)
    {
    mvm_t *tmpVM;

    if (WaitForEmpty == TRUE)
      {
      for (tmpL = VC->VMs;tmpL != NULL;)
        {
        tmpVM = (mvm_t *)tmpL->Ptr;

        tmpL = tmpL->Next;

        if ((DeleteObjects) && (tmpVM->TrackingJ != NULL))
          {
          /* If VM doesn't have a VMTracking job, Moab doesn't "own" it,
              don't destroy automatically */

          MJobCancelWithAuth(Auth,tmpVM->TrackingJ,RemoveMsg,FALSE,NULL,NULL);
#if 0
          if (tmpVM->TrackingJ != NULL)
            MJobCancelWithAuth(Auth,tmpVM->TrackingJ,RemoveMsg,FALSE,NULL,NULL);
          else
            MVMSubmitDestroyJob(NULL,tmpVM,NULL);
#endif
          }
        else
          {
          MULLRemove(&tmpVM->ParentVCs,VC->Name,NULL);
          MULLRemove(&VC->VMs,tmpVM->VMID,NULL);
          }
        }
      }
    else
      {
      for (tmpL = VC->VMs;tmpL != NULL;)
        {
        tmpVM = (mvm_t *)tmpL->Ptr;
        tmpL = tmpL->Next;

        MULLRemove(&tmpVM->ParentVCs,VC->Name,NULL);
        MULLRemove(&VC->VMs,tmpVM->VMID,NULL);

        if (DeleteObjects)
          {
          if (tmpVM->TrackingJ != NULL)
            MJobCancelWithAuth(Auth,tmpVM->TrackingJ,RemoveMsg,FALSE,NULL,NULL);
#if 0
          else
            MVMSubmitDestroyJob(NULL,tmpVM,NULL);
#endif
          }
        }
      }
    }

  /* Free sub-VCs */

  if (VC->VCs != NULL)
    {
    mln_t *tmpL = NULL;
    mvc_t *tmpVC = NULL;

    for (tmpL = VC->VCs;tmpL != NULL;)
      {
      tmpVC = (mvc_t *)tmpL->Ptr;

      /* NOTE: we have to check the next element in the linked list here,
          not as part of the for loop definition.
          MVCRemove will cause the child to remove itself from its
          parents.  This will modify this linked list as we are iterating
          through it. */

      tmpL = tmpL->Next;

      if (tmpVC != NULL)
        {
        MVCRemove(&tmpVC,DeleteObjects,WaitForEmpty,Auth);
        }
      }
    }

  if ((WaitForEmpty == TRUE) &&
      (MVCIsEmpty(VC) == FALSE))
    {
    /* Everything has been told to cancel, this VC will be removed when all objects finish */

    bmunset(&VC->IFlags,mvcifRemoved);
    bmunset(&VC->IFlags,mvcifDontTransition);

    /* Transition so that the VC shows the deleting flag */
    MVCTransition(VC);

    return(SUCCESS);
    }

  /* Only remove from parent when this VC is truly being "removed" */

  if (VC->ParentVCs != NULL)
    {
    /* Note: If this is the only child of the parent, and the parent
        has the mcvfDestroyWhenEmpty flag set, the parent will also
        be destroyed here.  We won't hit an infinite loop though, because
        the parent won't have this VC as a child anymore, and will simply
        be removed */

    MVCRemoveObjectParentVCs((void *)VC,mxoxVC);
    }

  if (MSched.Shutdown != TRUE)
    {
    /* If we're shutting down, the cache will have already been destroyed */

    bmset(&VC->IFlags,mvcifDeleteTransition);
    MVCTransition(VC);
    }

  /* Sub-VCs will be taken care of by the recursion in MVCFree */

  MVCFree(VCP);

  return(SUCCESS);
  }  /* END MVCRemove() */






/**
 * Frees the given VC.  Does NOT free nodes, jobs, rsvs, or VMs, but will free
 *    sub-VCs.
 *
 * @see MVCRemove (parent)
 *
 * @param VCP (I) [modified] - The VC to free.
 */

int MVCFree(

  mvc_t **VCP) /* I (modified) */

  {
  mvc_t *VC;

  const char *FName = "MVCFree";

  MDB(5,fSTRUCT) MLog("%s(%s)\n",
    FName,
    ((VCP != NULL) && (*VCP != NULL)) ? ((*VCP)->Name) : "NULL");

  if ((VCP == NULL) || (*VCP == NULL))
    {
    return(FAILURE);
    }

  VC = *VCP;

  /* This frees up the structures on the VC, NOT the actual
      resources! */

  MULLFree(&VC->Jobs,NULL);
  MULLFree(&VC->Nodes,NULL);
  MULLFree(&VC->Rsvs,NULL);
  MULLFree(&VC->VMs,NULL);
  MULLFree(&VC->VCs,NULL);

  /* Free variables */

  MUHTFree(&VC->Variables,TRUE,MUFREE);

  if (VC->Description != NULL)
    MUFree(&VC->Description);

  if (VC->CheckpointDesc != NULL)
    MUFree(&VC->CheckpointDesc);

  MUFree(&VC->Owner);
  MUFree(&VC->CreateJob);

  MACLFreeList(&VC->ACL);

  if (VC->MB != NULL)
    MMBFree(&VC->MB,TRUE);

  MUDynamicAttrFree(&VC->DynAttrs);

  /* If any triggers, free them */
  if (NULL != VC->T)
    {
    MTrigFreeList(VC->T);
    MUArrayListFree(VC->T);
    MUFree((char **)&VC->T);
    }

  MUHTRemove(&MVC,VC->Name,NULL);
  MULLRemove(&MVCToHarvest,VC->Name,NULL);

  bmclear(&VC->Flags);
  bmclear(&VC->IFlags);

  MUFree((char **)VCP);

  *VCP = NULL;

  return(SUCCESS);
  }  /* END MVCFree() */






/**
 * Returns TRUE if the given VC is empty (no jobs, rsvs, nodes, vms),
 *  returns FALSE otherwise.
 *  To be defined as empty, all sub-VCs must also be empty.
 *
 * @param VC (I) - The VC to evaluate
 */

mbool_t MVCIsEmpty(

  mvc_t *VC)   /* I */

  {
  if (VC == NULL)
    {
    /* A VC that is NULL is empty */

    return(TRUE);
    }

  /* If the linked list only has one item, when it is removed,
      the whole list becomes NULL */

  if (VC->Jobs != NULL)
    {
#if 0
    mln_t   *tmpL;
    mjob_t  *tmpJ;

    /* Stub code in case any check is needed for jobs */
    /* When a job completes, it will be removed from the VC */
    for (tmpL = VC->Jobs;tmpL != NULL;tmpL = tmpL->Next)
      {
      tmpJ = (mjob_t *)tmpL->Ptr;

      return(FALSE);
      }
#endif

    return(FALSE);
    }

  if (VC->Nodes != NULL)
    {
    return(FALSE);
    }

  if (VC->Rsvs != NULL)
    {
    return(FALSE);
    }

  if (VC->VMs != NULL)
    {
    return(FALSE);
    }

  if (VC->VCs != NULL)
    {
    mln_t *tmpL = NULL;
    mvc_t *tmpVC = NULL;

    for (tmpL = VC->VCs;tmpL != NULL;tmpL = tmpL->Next)
      {
      tmpVC = (mvc_t *)tmpL->Ptr;

      if (MVCIsEmpty(tmpVC) == FALSE)
        return(FALSE);
      }
    }

  return(TRUE);
  } /* END MVCIsEmpty */


int __MVCGetObjectByName(

  const char         *Name,
  enum MXMLOTypeEnum  Type,
  void              **OPtr)

  {
  void *Object;

  if (OPtr == NULL)
    return(FAILURE);

  *OPtr = NULL;

  if (Name == NULL)
    return(FAILURE);

  switch (Type)
    {
    case mxoJob:

      MJobFind(Name,(mjob_t **)&Object,mjsmExtended);

      break;

    case mxoNode:

      MNodeFind(Name,(mnode_t **)&Object);

      break;

    case mxoRsv:

      MRsvFind(Name,(mrsv_t **)&Object,mraNONE);

      break;

    case mxoxVM:

      MVMFind(Name,(mvm_t **)&Object);

      break;

    case mxoxVC:

      MVCFind(Name,(mvc_t **)&Object);

      break;

    default:

      return(FAILURE);

      /* NOTREACHED */

      break;
    }  /* END switch (Type) */

  if (Object == NULL)
    return(FAILURE);

  *OPtr = Object;

  return(SUCCESS); 
  }  /* END __MVCGetObjectByName() */



/**
 * Adds an object from the VC (object given by name and type)
 *
 * @see MVCAddObject() - child
 *
 * @param VC   [I] - The VC to add the object from
 * @param Name [I] - The name of object to add
 * @param Type [I] - Type of the object
 * @param U    [I] - User to check credentials of
 */

int MVCAddObjectByName(

  mvc_t             *VC,
  const char        *Name,
  enum MXMLOTypeEnum Type,
  mgcred_t          *U)

  {
  void *Object = NULL;

  if ((VC == NULL) || (Name == NULL))
    {
    return(FAILURE);
    }

  if ((__MVCGetObjectByName(Name,Type,&Object) == FAILURE) ||
      (Object == NULL))
    {
    return(FAILURE);
    }

  if ((U != NULL) && (U->Role != mcalAdmin1))
    {
    /* check if user has access to the requested object, used by mvcctl */

    if (MVCUserHasAccessToObject(U,Object,Type) == FALSE)
      return(FAILURE);
    }  /* END if (U != NULL) */

  return MVCAddObject(VC,Object,Type);
  }  /* END MVCAddObjectByName() */




/**
 * Removes an object from the VC (object given by name and type)
 *
 * @see MVCRemoveObject() - child
 *
 * @param VC   [I] - The VC to remove the object from
 * @param Name [I] - The name of object to remove
 * @param Type [I] - Type of the object
 */

int MVCRemoveObjectByName(

  mvc_t             *VC,
  const char        *Name,
  enum MXMLOTypeEnum Type)

  {
  void *Object = NULL;

  if ((VC == NULL) || (Name == NULL))
    {
    return(FAILURE);
    }

  if ((__MVCGetObjectByName(Name,Type,&Object) == FAILURE) ||
      (Object == NULL))
    {
    return(FAILURE);
    }

  return MVCRemoveObject(VC,Object,Type);
  }  /* END MVCRemoveObjectByName() */




/**
 * Adds the given object into the VC, and also adds the VC to the object.
 *
 * This should be the only place that an object's ParentVCs array is allocated.
 *
 * For some objects (jobs), the VC will be added to the credential list (CL)
 *
 * @param VC     (I) [modified] - The VC to add the object to
 * @param Object (I) [modified] - The object to add
 * @param Type   (I)            - The type of the object
 */

int MVCAddObject(

  mvc_t *VC,
  void  *Object,
  enum MXMLOTypeEnum Type)

  {
  char ObjectName[MMAX_NAME];

  const char *FName = "MVCAddObject";

  MDB(5,fSTRUCT) MLog("%s(%s,Object,%s)\n",
    FName,
    (VC != NULL) ? VC->Name : "NULL",
    MXO[Type]);

  if ((VC == NULL) || (Object == NULL))
    return(FAILURE);

  /* NOTE: object (jobs, rsvs, etc.) names should
      be unique, as well as VC names.  */

  switch (Type)
    {
    case mxoJob:

      {
      mjob_t *J;

      J = (mjob_t *)Object;

      MUStrCpy(ObjectName,J->Name,sizeof(ObjectName));

      if (MVCHasObject(VC,J->Name,mxoJob))
        break;

      MULLAdd(&J->ParentVCs,VC->Name,(void *)VC,NULL,NULL);
      MULLAdd(&VC->Jobs,J->Name,(void *)J,NULL,NULL);

      /* Add VC to job's CL */

      MACLSet(&J->Credential.CL,maVC,(void *)VC->Name,mcmpSEQ,mnmPositiveAffinity,0,TRUE);
      }  /* END case mxoJob */

      break;

    case mxoNode:

      {
      mnode_t *N;

      N = (mnode_t *)Object;

      MUStrCpy(ObjectName,N->Name,sizeof(ObjectName));

      if (MVCHasObject(VC,N->Name,mxoNode))
        break;

      MULLAdd(&N->ParentVCs,VC->Name,(void *)VC,NULL,NULL);
      MULLAdd(&VC->Nodes,N->Name,(void *)N,NULL,NULL);
      }  /* END case mxoNode */

      break;

    case mxoRsv:

      {
      mrsv_t *R;

      R = (mrsv_t *)Object;

      MUStrCpy(ObjectName,R->Name,sizeof(ObjectName));

      if (MVCHasObject(VC,R->Name,mxoRsv))
        break;

      MULLAdd(&R->ParentVCs,VC->Name,(void *)VC,NULL,NULL);
      MULLAdd(&VC->Rsvs,R->Name,(void *)R,NULL,NULL);
      }  /* END case mxoRsv */

      break;

    case mxoxVM:

      {
      mvm_t *VM;

      VM = (mvm_t *)Object;

      MUStrCpy(ObjectName,VM->VMID,sizeof(ObjectName));

      if (MVCHasObject(VC,VM->VMID,mxoxVM))
        break;

      MULLAdd(&VM->ParentVCs,VC->Name,(void *)VC,NULL,NULL);
      MULLAdd(&VC->VMs,VM->VMID,(void *)VM,NULL,NULL);
      }  /* END case mxoxVM */

      break;

    case mxoxVC:

      {
      mvc_t *SubVC;

      SubVC = (mvc_t *)Object;

      if ((SubVC == VC) ||
          (!strcmp(SubVC->Name,VC->Name)) ||
          (MVCIsInVCChain(SubVC,VC) == TRUE))
        {
        /* VC is already in this chain, would be an infinite loop */

        MDB(3,fSTRUCT) MLog("ERROR:  VC '%s' could not be added to VC '%s', would create circular VC chain\n",
          SubVC->Name,
          VC->Name);

        return(FAILURE);
        }

      MUStrCpy(ObjectName,SubVC->Name,sizeof(ObjectName));

      if (MVCHasObject(VC,SubVC->Name,mxoxVC))
        break;

      MULLAdd(&SubVC->ParentVCs,VC->Name,(void *)VC,NULL,NULL);
      MULLAdd(&VC->VCs,SubVC->Name,(void *)SubVC,NULL,NULL);

      /* Parent will be transitioned below, need to transition sub-VC
          so that the cache will know that this VC has a parent */

      MVCTransition(SubVC);
      }  /* END case mxoxVC */

      break;

    default:

      return(FAILURE);

      /* NOTREACHED */

      break;
    }  /* END switch (Type) */

  MDB(5,fSTRUCT) MLog("INFO:     added %s '%s' to VC '%s'\n",
    MXO[Type],
    ObjectName,
    VC->Name);

  MVCTransition(VC);

  return(SUCCESS);
  }  /* END MVCAddObject() */




/**
 * Removes the given object from the VC, and removes the VC from the object.  This does not
 *   remove it from any sub-VCs.
 *
 *  NOTE: If the mvcfDestroyWhenEmpty flag is set, this function will also destroy
 *         the VC if it is empty.  That means that you cannot assume that the VC
 *         will still exist after calling this function!!!
 *
 * @param VC     (I) [modified] - The VC to remove the object from
 * @param Object (I) [modified] - The object to remove 
 * @param Type   (I)            - The type of the object
 */

int MVCRemoveObject(

  mvc_t *VC,
  void  *Object,
  enum MXMLOTypeEnum Type)

  {
  char ObjectName[MMAX_NAME];

  const char *FName = "MVCRemoveObject";

  MDB(5,fSTRUCT) MLog("%s(%s,Object,%s)\n",
    FName,
    (VC != NULL) ? VC->Name : "NULL",
    MXO[Type]);

  if ((VC == NULL) || (Object == NULL))
    return(FAILURE);

  switch (Type)
    {
    case mxoJob:

      {
      mjob_t *J;

      J = (mjob_t *)Object;

      MUStrCpy(ObjectName,J->Name,sizeof(ObjectName));

      MULLRemove(&J->ParentVCs,VC->Name,NULL);
      MULLRemove(&VC->Jobs,J->Name,NULL);

      MACLRemove(&J->Credential.CL,maVC,(void *)VC->Name);
      }  /* END case mxoJob */

      break;

    case mxoNode:

      {
      mnode_t *N;

      N = (mnode_t *)Object;

      MUStrCpy(ObjectName,N->Name,sizeof(ObjectName));

      MULLRemove(&N->ParentVCs,VC->Name,NULL);
      MULLRemove(&VC->Nodes,N->Name,NULL);
      }  /* END case mxoNode */

      break;

    case mxoRsv:

      {
      mrsv_t *R;

      R = (mrsv_t *)Object;

      MUStrCpy(ObjectName,R->Name,sizeof(ObjectName));

      MULLRemove(&R->ParentVCs,VC->Name,NULL);
      MULLRemove(&VC->Rsvs,R->Name,NULL);
      }  /* END case mxoRsv */

      break;

    case mxoxVM:

      {
      mvm_t *VM;

      VM = (mvm_t *)Object;

      MUStrCpy(ObjectName,VM->VMID,sizeof(ObjectName));

      MULLRemove(&VM->ParentVCs,VC->Name,NULL);
      MULLRemove(&VC->VMs,VM->VMID,NULL);
      }  /* END case mxoxVM */

      break;

    case mxoxVC:

      {
      mvc_t *SubVC;

      SubVC = (mvc_t *)Object;

      MUStrCpy(ObjectName,SubVC->Name,sizeof(ObjectName));

      MULLRemove(&SubVC->ParentVCs,VC->Name,NULL);
      MULLRemove(&VC->VCs,SubVC->Name,NULL);
      }  /* END case mxoxVC */

      break;

    default:

      return(FAILURE);

      /* NOTREACHED */

      break;
    }  /* END switch (Type) */

  MDB(5,fSTRUCT) MLog("INFO:     removed %s '%s' from VC '%s'\n",
    MXO[Type],
    ObjectName,
    VC->Name);

  if ((bmisset(&VC->Flags,mvcfDestroyWhenEmpty)) &&
      (MSched.Shutdown != TRUE))
    {
    /* If VC is empty, mark for harvesting */

    /* Don't remove on shutdown (when sub-objects get destroyed, this VC
        could go away before being properly checkpointed) */

    if (MVCIsEmpty(VC))
      {
      MUHTRemove(&MVC,VC->Name,NULL);
      MULLAdd(&MVCToHarvest,VC->Name,(void *)VC,NULL,NULL);

      bmset(&VC->IFlags,mvcifHarvest);
      bmset(&VC->IFlags,mvcifDeleteTransition);
      MVCTransition(VC);
      }
    }

  if (!bmisset(&VC->IFlags,mvcifHarvest))
    {
    /* If VC was not marked for deletion, transition */

    MVCTransition(VC);
    }

  return(SUCCESS);
  }  /* END MVCRemoveObject() */




/**
 * Removes all of an object's VCParents
 *
 * This function is here because it can be tricky to remove the VCParents because
 *  the linked list node for each parent is removed when you do the MVCRemoveObject
 *
 * @param Object [I] (modified) - object to remove all of its VCParents from
 * @param Type   [I] - the type of the object
 */

int MVCRemoveObjectParentVCs(

  void *Object,
  enum MXMLOTypeEnum Type)

  {
  mln_t *tmpL;

  const char *FName = "MVCRemoveObjectParentVCs";

  char OName[MMAX_NAME];

  if (Object != NULL)
    {
    switch (Type)
      {
      case mxoJob: MUStrCpy(OName,((mjob_t *)Object)->Name,sizeof(OName)); break;
      case mxoNode:MUStrCpy(OName,((mnode_t *)Object)->Name,sizeof(OName)); break;
      case mxoRsv: MUStrCpy(OName,((mrsv_t *)Object)->Name,sizeof(OName)); break;
      case mxoxVM: MUStrCpy(OName,((mvm_t *)Object)->VMID,sizeof(OName)); break;
      case mxoxVC: MUStrCpy(OName,((mvc_t *)Object)->Name,sizeof(OName)); break;

      default:     MUStrCpy(OName,"Object",sizeof(OName)); break;
      }
    }
  else
    {
    MUStrCpy(OName,"Object",sizeof(OName));
    }

  MDB(5,fSTRUCT) MLog("%s(%s,%s)\n",
    FName,
    OName,
    MXO[Type]);

  if (Object == NULL)
    return(FAILURE);

  switch (Type)
    {
    case mxoJob:

      tmpL = ((mjob_t *)Object)->ParentVCs;

      break;

    case mxoNode:

      tmpL = ((mnode_t *)Object)->ParentVCs;

      break;

    case mxoRsv:

      tmpL = ((mrsv_t *)Object)->ParentVCs;

      break;

    case mxoxVM:

      tmpL = ((mvm_t *)Object)->ParentVCs;

      break;

    case mxoxVC:

      tmpL = ((mvc_t *)Object)->ParentVCs;

      break;

    default:

      /* not supported */

      return(FAILURE);

      /* NOTREACHED */

      break;
    }  /* END switch (Type) */

  while (tmpL != NULL)
    {
    mvc_t *VC;
    mln_t *curL = tmpL;

    tmpL = tmpL->Next;

    VC = (mvc_t *)curL->Ptr;

    MVCRemoveObject(VC,Object,Type);

    /* Transition so that the VC will show that the object was removed */

    MVCTransition(VC);
    }  /* END while (tmpL != NULL) */

  return(SUCCESS);
  }  /* END MVCRemoveObjectParentVCs() */




/**
 * Frees all VCs for shutdown.
 */

int MVCFreeAll()
  {
  mhashiter_t HIter;
  char       *VCName;
  mvc_t      *VC;

  const char *FName = "MVCFreeAll";

  MDB(5,fSTRUCT) MLog("%s()\n",
    FName);

  /* Go through the hash one at a time so that MVCRemove doesn't clobber
      the hash table as we're iterating through it */

  while (TRUE)
    {
    MUHTIterInit(&HIter);

    if(MUHTIterate(&MVC,&VCName,(void **)&VC,NULL,&HIter) == SUCCESS)
      {
      /* Any sub-VCs will be removed from the hashtable in MVCRemove */ 

      MVCRemove(&VC,FALSE,FALSE,NULL);
      }
    else
      break;
    }

  MUHTFree(&MVC,FALSE,NULL);

  return(SUCCESS);
  }  /* END MVCFreeAll() */



/**
 * Returns TRUE if the given type can be added to a VC
 *
 * @param Type - The type in question
 */

mbool_t MVCIsSupportedOType(

  enum MXMLOTypeEnum Type)

  {
  if ((Type == mxoJob) ||
      (Type == mxoNode) ||
      (Type == mxoRsv) ||
      (Type == mxoxVM) ||
      (Type == mxoxVC))
    {
    return(TRUE);
    }

  return(FALSE);
  }




/**
 * Returns TRUE if the VC contains the given object
 *
 * @param VC   [I] - The VC
 * @param Name [I] - Object name
 * @param Type [I] - Object type
 */

mbool_t MVCHasObject(

  mvc_t             *VC,
  const char        *Name,
  enum MXMLOTypeEnum Type)

  {
  mln_t *tmpL = NULL;

  if ((VC == NULL) || (Name == NULL))
    {
    return(FALSE);
    }

  switch (Type)
    {
    case mxoJob:

      tmpL = VC->Jobs;

      break;

    case mxoNode:

      tmpL = VC->Nodes;

      break;

    case mxoRsv:

      tmpL = VC->Rsvs;

      break;

    case mxoxVM:

      tmpL = VC->VMs;

      break;

    case mxoxVC:

      tmpL = VC->VCs;

      break;

    default:

      return(FALSE);

      /* NOTREACHED */

      break;
    }  /* END switch (Type) */

  if (tmpL == NULL)
    return(FALSE);

  if (MULLCheckCS(tmpL,Name,NULL) == SUCCESS)
    return(TRUE);

  return(FALSE);
  }  /* END MVCHasObject() */


/**
 * Returns TRUE if the User is an owner of the VC, or has access to
 *  the owner object if the owner is a group or account.
 *
 * @param SpecVC [I] - The VC to test ownership of it.  Not necessary if VCName is specified.  Means using a blocking command.
 * @param VCName [I] - The name of the VC.  Not necessary if SpecVC is specified.  Means using a non-blocking command.
 * @param U      [I] - The user to check if they are the owner (or part of owner object)
 */

mbool_t MVCCredIsOwner(

  mvc_t    *SpecVC,
  char     *VCName,
  mgcred_t *U)

  {
  enum MXMLOTypeEnum OwnerType;
  char     *Owner = NULL;
  mbool_t   RC = FALSE;

  if (U == NULL)
    return(FALSE);

  /* If user is level 1 admin, they have access */

  if (U->Role == mcalAdmin1)
    {
    return(TRUE);
    }

  if (SpecVC != NULL)
    {
    OwnerType = SpecVC->OwnerType;
    MUStrDup(&Owner,SpecVC->Owner);
    }
  else
    {
    if ((VCName != NULL) && (VCName[0] != '\0'))
      {
      mtransvc_t *TVC = NULL;
      marray_t  VCConstraintList;
      mvcconstraint_t VCConstraint;
      char *Ptr = NULL;
      char *TokPtr = NULL;
      char tmpOwner[MMAX_LINE];
      marray_t  VCList;

      MUArrayListCreate(&VCList,sizeof(mtransvc_t *),2);
      MUArrayListCreate(&VCConstraintList,sizeof(mtransvc_t *),2);

      VCConstraint.AType = mvcaName;
      VCConstraint.ACmp = mcmpEQ2;

      MUStrCpy(VCConstraint.AVal[0],VCName,sizeof(VCConstraint.AVal[0]));

      MUArrayListAppend(&VCConstraintList,(void *)&VCConstraint);

      MCacheVCLock(TRUE,TRUE);
      MCacheReadVCs(&VCList,&VCConstraintList);

      TVC = (mtransvc_t *)MUArrayListGetPtr(&VCList,0);

      MUArrayListFree(&VCList);
      MUArrayListFree(&VCConstraintList);

      if (TVC == NULL)
        {
        MCacheVCLock(FALSE,TRUE);

        return(FAILURE);
        }

      MUStrCpy(tmpOwner,TVC->Owner,sizeof(tmpOwner));

      Ptr = MUStrTok(tmpOwner,":",&TokPtr);

      OwnerType = (enum MXMLOTypeEnum)MUGetIndexCI(Ptr,MXO,FALSE,mxoNONE);

      Ptr = MUStrTok(NULL,":",&TokPtr);

      MUStrDup(&Owner,Ptr);

      MCacheVCLock(FALSE,TRUE);
      }
    else
      {
      /* Neither SpecVC nor VCName was specified */

      return(FALSE);
      }
    }  /* END else of if (SpecVC != NULL) */

  if ((Owner == NULL) ||
      (Owner[0] == '\0'))
    {
    MUFree(&Owner);

    return(FALSE);
    }

  switch (OwnerType)
    {
    case mxoUser:

      if (!strcmp(U->Name,Owner))
        RC = TRUE;

      break;

    case mxoGroup:

      if (U->F.GAL == NULL)
        RC = FALSE;

      if (MULLCheck(U->F.GAL,Owner,NULL) == SUCCESS)
        RC = TRUE;

      break;

    case mxoAcct:

      if (U->F.AAL == NULL)
        RC = FALSE;

      if (MULLCheck(U->F.AAL,Owner,NULL) == SUCCESS)
        RC = TRUE;

      break;

    default:

      /* Not a valid owner type */

      break;
    }  /* END switch (VC->OwnerType) */

  MUFree(&Owner);

  return(RC);
  }  /* END MVCCredIsOwner() */


/**
 * Adds a trigger var to a VC and passes it up to parent VCs with namespace
 *   NOTE:  VCIsStartingVC should always be TRUE by the caller.  It
 *           is set to FALSE for recursion.
 *
 * @param VC             [I] (modified) - VC to add the variable to
 * @param Name           [I] - Variable name (if VCIsStartingVC is FALSE, this includes namespace already)
 * @param Value          [I] (optional) - Variable value, optional if DoIncr == TRUE
 * @param DoExport       [I] - Export up to parent VCs
 * @param DoIncr         [I] - Increment the var value
 * @param VCIsStartingVC [I] - TRUE if this is the main VC (namespace will be based on this VC)
 */

int MVCAddVarRecursive(

  mvc_t      *VC,
  const char *Name,
  const char *Value,
  mbool_t     DoExport,
  mbool_t     DoIncr,
  mbool_t     VCIsStartingVC)

  {
  mln_t     *tmpL = NULL;
  mvc_t     *tmpVC = NULL;
  char      *tmpVal = NULL;
  mbitmap_t  tmpBM;

  if ((VC == NULL) ||
      (Name == NULL))
    {
    return(FAILURE);
    }

  /* Add to current VC */

  if (DoIncr == TRUE)
    {
    int tmpI;
    char tmpLine[MMAX_NAME];

    MUHTGet(&VC->Variables,Name,(void **)&tmpVal,&tmpBM);

    if (DoExport == TRUE)
      bmset(&tmpBM,mtvaExport);

    if (tmpVal != NULL)
      {
      char *ptr = (char *)tmpVal;

      tmpI = (int)strtol(ptr,NULL,10);

      tmpI++;
      }
    else
      {
      tmpI = 1;
      }

    sprintf(tmpLine,"%d",
      tmpI);

    MUHTAdd(&VC->Variables,Name,strdup(tmpLine),&tmpBM,MUFREE);
    }  /* END if (DoIncr == TRUE) */
  else
    {
    if (DoExport == TRUE)
      bmset(&tmpBM,mtvaExport);

    if (Value != NULL)
      {
      MUHTAdd(&VC->Variables,Name,strdup(Value),&tmpBM,MUFREE);
      }
    }  /* END else if (DoIncr == FALSE) */

  if (DoExport == TRUE)
    {
    /* Pass to all parent VCs */

    if (VCIsStartingVC)
      {
      mstring_t VarWithNameSpace(MMAX_LINE);

      MStringSetF(&VarWithNameSpace,"%s.%s",
        VC->Name,
        Name);

      for (tmpL = VC->ParentVCs;tmpL != NULL;tmpL = tmpL->Next)
        {
        tmpVC = (mvc_t *)tmpL->Ptr;

        if (tmpVC == NULL)
          continue;

        MVCAddVarRecursive(
          tmpVC,
          VarWithNameSpace.c_str(),
          Value,
          DoExport,
          DoIncr,
          FALSE);
        }  /* END for (tmpL = VC->ParentVCs; ...) */
      }  /* END if (VCIsStartingVC) */
    else
      {
      /* Namespace is already in the name */

      for (tmpL = VC->ParentVCs;tmpL != NULL;tmpL = tmpL->Next)
        {
        tmpVC = (mvc_t *)tmpL->Ptr;
    
        if (tmpVC == NULL)
          continue;

        MVCAddVarRecursive(
          tmpVC,
          Name,
          Value,
          DoExport,
          DoIncr,
          FALSE);
        }
      }  /* END else (VCIsStartingVC == FALSE) */
    } /* END if (DoExport == TRUE) */

  MVCTransition(VC);

  return(SUCCESS);
  }  /* END MVCAddVarWithExport */


/**
 * Sees if the given variable name matches the desired variable name.
 *  Works with namespaces.
 *
 * @see MVCSearchForVar (parent)
 *
 * @param CurrentVar [I] - The name of the variable we are checking
 * @param DesiredVar [I] - The variable we are searching for
 */

int MVCVarNameFits(

  const char *CurrentVar,
  const char *DesiredVar)

  {
  const char *Ptr;

  if (CurrentVar == NULL)
    {
    /* Don't need to check the desired name because that is checked in the
        parameter list in MVCSearchForVar() */

    return(FAILURE);
    }

  if (!strcmp(CurrentVar,DesiredVar))
    return(SUCCESS);

  /* Check namespace */

  if (MTrigVarIsNamespace(CurrentVar,&Ptr) == FALSE)
    return(FAILURE);

  if (!strcmp(Ptr,DesiredVar))
    return(SUCCESS);

  return(FAILURE);
  }  /* END MVCVarNameFits() */






/**
 * Searches for a variable in the given VC and up recursively
 *  through parent VCs.  Takes namespace into account.
 *  Appends onto a list of found variables.  (Can have multiple
 *  because of namespace).
 *
 * Returns SUCCESS if a variable was found at any level.
 *
 * @param VC            [I] - Current VC to check
 * @param VarName       [I] - The variable name to search for
 * @param ReturnList    [O] - List of found variables (names, values)
 * @param SearchParents [I] - TRUE to search above this VC
 * @param UseNameSpace  
 */

int MVCSearchForVar(

  mvc_t  *VC,
  const char   *VarName,
  mln_t **ReturnList,
  mbool_t SearchParents,
  mbool_t UseNameSpace)

  {
  mln_t *tmpL;
  mvc_t *tmpVC;
  mhashiter_t HIter;

  char  *tmpName;
  char  *tmpVal;

  if ((VC == NULL) ||
      (VarName == NULL) ||
      (ReturnList == NULL))
    return(FAILURE);

  /* Check this VC's variables */

  MUHTIterInit(&HIter);

  while (MUHTIterate(&VC->Variables,&tmpName,(void **)&tmpVal,NULL,&HIter) == SUCCESS)
    {
    if (UseNameSpace == FALSE)
      {
      if (strcmp(tmpName,VarName))
        continue;
      }
    else if (MVCVarNameFits(tmpName,VarName) == FAILURE)
      {
      /* Variable name does not match */

      continue;
      }

    if (MULLCheck(*ReturnList,tmpName,NULL) == SUCCESS)
      {
      /* We have already found this variable, don't overwrite */

      continue;
      }

    /* Found a valid variable */

    MUAddVarLL(ReturnList,tmpName,tmpVal);
    }  /* END while (MUHTIterate(&VC->Variables, ...) == SUCCESS) */

  if (SearchParents == TRUE)
    {
    /* Call to parents */

    for (tmpL = VC->ParentVCs;tmpL != NULL;tmpL = tmpL->Next)
      {
      tmpVC = (mvc_t *)tmpL->Ptr;

      if (tmpVC == NULL)
        continue;

      MVCSearchForVar(
        tmpVC,
        VarName,
        ReturnList,
        SearchParents,
        UseNameSpace);
      }
    }  /* END if (SearchParents == TRUE) */

  if (*ReturnList != NULL)
    return(SUCCESS);

  return(FAILURE);
  }  /* END int MVCSearchForVar() */


/**
 * Adds variables from the VC to the linked list.
 *  Repeats for parent VCs.  Does not add the variable if already present.
 *
 * @param VC      [I] - The VC whose variables will be added
 * @param VarList [I/O] (modified) - The list to append to
 * @param AcceptedNS 
 */

int MVCAddVarsToLLRecursive(

  mvc_t *VC,
  mln_t **VarList,
  const char  *AcceptedNS)

  {
  mhashiter_t HIter;
  char  *tmpName;
  char  *tmpVal;
  mln_t *tmpL;
  mvc_t *tmpVC;

  if ((VC == NULL) || (VarList == NULL))
    {
    return(FAILURE);
    }

  MUHTIterInit(&HIter);

  while (MUHTIterate(&VC->Variables,&tmpName,(void **)&tmpVal,NULL,&HIter) == SUCCESS)
    {
    if ((AcceptedNS != NULL) && (MTrigValidNamespace(tmpName,AcceptedNS) == FALSE))
      {
      /* Used a namespace, but not one we're looking for */

      continue;
      }

    if ((tmpVal == NULL) || (MULLCheck(*VarList,tmpName,NULL) == SUCCESS))
      {
      /* Don't overwrite */

      continue;
      }

    MUAddVarLL(VarList,tmpName,tmpVal);
    }

  for (tmpL = VC->ParentVCs;tmpL != NULL;tmpL = tmpL->Next)
    {
    tmpVC = (mvc_t *)tmpL->Ptr;

    if (tmpVC == NULL)
      continue;

    MVCAddVarsToLLRecursive(tmpVC,VarList,AcceptedNS);
    }

  return(SUCCESS);
  }  /* END MVCAddVarsToLLRecursive() */




/**
 * Returns TRUE if the user has access to the VC (or any parent VC).
 *
 * Not to be used for non-blocking commands.
 *
 * @param VC [I] - VC to check access to
 * @param U  [I] - User trying to access VC
 */

mbool_t MVCUserHasAccess(

  mvc_t    *VC,
  mgcred_t *U)

  {
  mln_t *tmpVCL;
  mvc_t *tmpVC;

  if ((VC == NULL) ||
      (U == NULL))
    {
    return(FALSE);
    }

  if (MVCCredIsOwner(VC,NULL,U) == TRUE)
    {
    return(TRUE);
    }

  if (MCredCheckAccess(U,NULL,maUser,VC->ACL) == SUCCESS)
    return(TRUE);

  for (tmpVCL = VC->ParentVCs;tmpVCL != NULL;tmpVCL = tmpVCL->Next)
    {
    tmpVC = (mvc_t *)tmpVCL->Ptr;

    if (tmpVC == NULL)
      continue;

    if (MVCUserHasAccess(tmpVC,U) == TRUE)
      return(TRUE);
    }  /* END for (tmpVCL) */

  return(FALSE);
  }  /* END MVCUserHasAccess() */


/**
 * Returns TRUE if the user has access to the object via VC ACLs.
 *  NOT FOR NON-BLOCKING THREADS
 *
 * @param U      [I] - The user to check against
 * @param Object [I] - Object we are checking for access
 * @param Type   [I] - Type of the object
 */

mbool_t MVCUserHasAccessToObject(

  mgcred_t *U,
  void     *Object,
  enum MXMLOTypeEnum Type)

  {
  mln_t *LLPtr = NULL;
  mbool_t AccessGranted = FALSE;

  if ((U == NULL) ||
      (Object == NULL))
    return(FALSE);

  if (U->Role == mcalAdmin1)
    return(TRUE);

  switch(Type)
    {
    case mxoJob:

      {
      mjob_t *J = NULL;
      mfst_t *TData = NULL;

      J = (mjob_t *)Object;

      if ((J->FairShareTree != NULL) && (J->FairShareTree[J->Req[0]->PtIndex] != NULL))
        TData = J->FairShareTree[J->Req[0]->PtIndex]->TData;
      else if ((J->FairShareTree != NULL) && (J->FairShareTree[0] != NULL))
        TData = J->FairShareTree[0]->TData;

      if (MUserIsOwner(U,J->Credential.U,J->Credential.A,J->Credential.C,NULL,&J->PAL,mxoJob,TData) == TRUE)
        {
        AccessGranted = TRUE;

        break;
        }
      }  /* END case mxoJob */

      break;

    case mxoRsv:

      {
      mrsv_t *R = NULL;

      R = (mrsv_t *)Object;

      if ((R->OType == mxoUser) &&
          (R->O != NULL) &&
          (R->O == U))
        {
        AccessGranted = TRUE;

        break;
        }

      if ((R->Type == mrtJob) && (R->J != NULL))
        {
        mjob_t *J = R->J;

        if (MUserIsOwner(U,J->Credential.U,J->Credential.A,J->Credential.C,NULL,&J->PAL,mxoNONE,NULL) == TRUE)
          {
          AccessGranted = TRUE;

          break;
          }
        }
      else if (MUserIsOwner(U,R->U,R->A,NULL,R->O,NULL,R->OType,NULL) == TRUE)
        {
        AccessGranted = TRUE;

        break;
        }
      }  /* END case mxoRsv */

      break;

    case mxoNode:

      if (U->Role == mcalAdmin1)
        AccessGranted = TRUE;

      break;

    case mxoxVM:

      {
      /* VMs currently have no ACL, use tracking job creds */

      mjob_t *TJ = ((mvm_t *)Object)->TrackingJ;

      if (TJ == NULL)
        break;

      AccessGranted = MVCUserHasAccessToObject(U,(void *)TJ,mxoJob);
      }

      break;

    case mxoxVC:

      /* NO-OP - VC will be checked below */

      break;

    default:

      /* unsupported type */

      return(FALSE);

      /* NOTREACHED */

      break;
    }  /* END switch (Type) */

  if (AccessGranted == TRUE)
    return(TRUE);

  /* Now check access via VCs */

  switch (Type)
    {
    case mxoJob:  LLPtr = ((mjob_t *)Object)->ParentVCs; break;
    case mxoNode: LLPtr = ((mnode_t *)Object)->ParentVCs; break;
    case mxoRsv:  LLPtr = ((mrsv_t *)Object)->ParentVCs; break;
    case mxoxVM:  LLPtr = ((mvm_t *)Object)->ParentVCs; break;
    case mxoxVC:

      return(MVCUserHasAccess((mvc_t *)Object,U));

      /* NOTREACHED */

      break;

    default:

      /* unsupported type */

      return(FALSE);

      /* NOTREACHED */

      break;
    }  /* END switch (Type) */

  while (LLPtr != NULL)
    {
    mvc_t *VC = (mvc_t *)LLPtr->Ptr;

    if (MVCUserHasAccess(VC,U))
      return(TRUE);

    LLPtr = LLPtr->Next;
    }

  return(FALSE);
  }  /* END MVCUserHasAccessToObject() */



/**
 * Helper function to ensure that we don't have infinite loops in VCs
 *  (A in B in A, etc.)
 *
 * @param VCChild  [I] - The VC to be added
 * @param VCParent [I] - VC to check against (checks parents, not the actual VC)
 */

mbool_t MVCIsInVCChain(

  mvc_t *VCChild,
  mvc_t *VCParent)

  {
  mln_t *tmpL;
  mvc_t *tmpVC;

  if ((VCParent == NULL) || (VCChild == NULL))
    return(FALSE);

  for (tmpL = VCParent->ParentVCs;tmpL != NULL;tmpL = tmpL->Next)
    {
    tmpVC = (mvc_t *)tmpL->Ptr;

    if (tmpVC == NULL)
      continue;

    if ((tmpVC == VCChild) || (!strcmp(tmpVC->Name,VCChild->Name)))
      return(TRUE);

    if (MVCIsInVCChain(VCChild,tmpVC) == TRUE)
      return(TRUE);
    }

  return(FALSE);
  }  /* END MVCIsINVCChain() */


/**
 * Sets an dynamic attribute (minternal_attr_t)
 *  There is no mode because these are simple types (char *,int,etc).
 *
 * Format technically refers to the format of Value, but this function will
 *  return FAILURE if it is not the same as the expected type for the given
 *  index.  Only indexes that use the dynamic attributes should be supported.
 *
 * Returns SUCCESS if the var was set.
 *
 * @param VC     [I] (modified) - The VC to set the attribute on
 * @param AIndex [I] - The attribute to modify
 * @param Value  [I] - Value to set attribute to
 * @param Format [I] - Format of the attribute
 */

int MVCSetDynamicAttr(
  mvc_t               *VC,     /* I (modified) */
  enum MVCAttrEnum     AIndex, /* I */
  void               **Value,  /* I */
  enum MDataFormatEnum Format) /* I */

  {
  if ((VC == NULL) ||
      (Value == NULL) ||
      (AIndex == mvcaNONE))
    return(FAILURE);

  switch (Format)
    {
    case mdfString:

      {
      switch (AIndex)
        {
        /* Put all accepted char * attrs here */

        case mvcaReqNodeSet:

          return(MUDynamicAttrSet(AIndex,&VC->DynAttrs,Value,Format));

          break;

        default:
          /* wrong aindex or type for aindex */

          return(FAILURE);

          break;
        }  /* END switch (AIndex) */
      }  /* END BLOCK mdfString */

      break;

    case mdfInt:

      {
      switch (AIndex)
        {
        default:
          /* wrong aindex or type for aindex */

          return(FAILURE);

          break;
        }  /* END switch (AIndex) */
      }  /* END BLOCK mdfInt */

      break;

    case mdfDouble:

      {
      switch (AIndex)
        {
        default:
          /* wrong aindex or type for aindex */

          return(FAILURE);

          break;
        }  /* END switch (AIndex) */
      }  /* END BLOCK mdfDouble */

      break;

    case mdfLong:

      {
      switch (AIndex)
        {
        default:
          /* wrong aindex or type for aindex */

          return(FAILURE);

          break;
        }  /* END switch (AIndex) */
      }  /* END BLOCK mdfLong */

      break;

    case mdfMulong:

      {
      switch (AIndex)
        {
        case mvcaReqStartTime:

          return(MUDynamicAttrSet(AIndex,&VC->DynAttrs,Value,Format));

          break;

        default:
          /* wrong aindex or type for aindex */

          return(FAILURE);

          break;
        }  /* END switch (AIndex) */
      }  /* END BLOCK mdfMulong */

      break;

    case mdfOther:

      {
      switch (AIndex)
        {
        case mvcaThrottlePolicies:

          return(MUDynamicAttrSet(AIndex,&VC->DynAttrs,Value,Format));

          break;

        default:
           /* wrong aindex or type for aindex */

          return(FAILURE);

          break;
        }  /* END switch (AIndex) */
      }    /* END BLOCK mdfOther */

      break;

    default:

      /* Type not supported */

      return(FAILURE);

      /* NOTREACHED */

      break;
    }  /* END switch (Format) */

  return(SUCCESS);
  }  /* END MVCSetDynamicAttr() */



int MVCGetDynamicAttr(

  mvc_t               *VC,      /* I (modified) */
  enum MVCAttrEnum     AIndex, /* I */
  void               **Value,  /* I */
  enum MDataFormatEnum Format) /* I */

  {
  if ((VC == NULL) ||
      (Value == NULL) ||
      (AIndex == mvcaNONE))
    return(FAILURE);

  switch (Format)
    {
    case mdfString:

      {
      switch (AIndex)
        {
        /* Put all accepted char * attrs here */

        case mvcaReqNodeSet:

          return(MUDynamicAttrGet(AIndex,VC->DynAttrs,Value,Format));

          break;

        default:
          /* wrong aindex or type for aindex */

          return(FAILURE);

          break;
        }  /* END switch (AIndex) */
      }  /* END BLOCK mdfString */

      break;

    case mdfInt:

      {
      switch (AIndex)
        {
        default:
          /* wrong aindex or type for aindex */

          return(FAILURE);

          break;
        }  /* END switch (AIndex) */
      }  /* END BLOCK mdfInt */

      break;

    case mdfDouble:

      {
      switch (AIndex)
        {
        default:
          /* wrong aindex or type for aindex */

          return(FAILURE);

          break;
        }  /* END switch (AIndex) */
      }  /* END BLOCK mdfDouble */

      break;

    case mdfLong:

      {
      switch (AIndex)
        {
        default:
          /* wrong aindex or type for aindex */

          return(FAILURE);

          break;
        }  /* END switch (AIndex) */
      }  /* END BLOCK mdfLong */

      break;

    case mdfMulong:

      {
      switch (AIndex)
        {
        case mvcaReqStartTime:

          return(MUDynamicAttrGet(AIndex,VC->DynAttrs,Value,Format));

          break;

        default:
          /* wrong aindex or type for aindex */

          return(FAILURE);

          break;
        }  /* END switch (AIndex) */
      }  /* END BLOCK mdfMulong */

    case mdfOther:

      {
      switch (AIndex)
        {
        /* Put all accepted char * attrs here */

        case mvcaThrottlePolicies:

          return(MUDynamicAttrGet(AIndex,VC->DynAttrs,Value,Format));

          break;

        default:
          /* wrong aindex or type for aindex */

          return(FAILURE);

          break;
        }  /* END switch (AIndex) */
      }  /* END BLOCK mdfOther */

      break;

    default:

      /* Type not supported */

      return(FAILURE);

      /* NOTREACHED */

      break;
    }  /* END switch (Format) */

  return(SUCCESS);
  }  /* END MVCGetDynamicAttr() */


/**
 * Sets the requested node set on a VC.
 *  Checks if it is a valid feature, NOT if it is a valid node set feature.
 *  If a feature is already set, returns SUCCESS.
 *
 * @see MJobSelectResourceSet() - parent
 *
 * @param VC      [I] (modified) - VC to set the ReqNodeSet on
 * @param Feature [I] - Feature to set
 */

int MVCSetNodeSet(
  mvc_t *VC,
  int    Feature)

  {
  char  *tmpAttr = NULL;
  mln_t *tmpL;
  mvc_t *tmpVC;

  if ((VC == NULL) || (Feature <= 0))
    return(FAILURE);

  if (MAList[meNFeature][Feature][0] == '\0')
    return(FAILURE);

  /* ReqNodeSet only gets written here if there is not a previous value */

  if ((MVCGetDynamicAttr(VC,mvcaReqNodeSet,(void **)&tmpAttr,mdfString) == SUCCESS) &&
      (tmpAttr != NULL) &&
      (tmpAttr[0] != '\0'))
    {
    MUFree(&tmpAttr);

    return(SUCCESS);
    }

  MVCSetAttr(VC,mvcaReqNodeSet,(void **)MAList[meNFeature][Feature],mdfString,mSet);

  for (tmpL = VC->ParentVCs;tmpL != NULL;tmpL = tmpL->Next)
    {
    tmpVC = (mvc_t *)tmpL->Ptr;

    if (tmpVC == NULL)
      continue;

    /* Apply upwards as far as possible */

    MVCSetNodeSet(tmpVC,Feature);
    }

  return(SUCCESS);
  }  /* END MVCSetNodeSet() */

/**
 *  Helper function to recursively go through VCs and search for a node set.
 *
 * @see MVCGetNodeSet() - parent
 */

int __MVCGetNodeSetRecursive(

  mvc_t *VC,
  char **Ptr)

  {
  char  *tmpAttr;
  mln_t *tmpL;
  mvc_t *tmpVC;

  if ((MVCGetDynamicAttr(VC,mvcaReqNodeSet,(void **)&tmpAttr,mdfString) == SUCCESS) &&
      (tmpAttr != NULL) &&
      (tmpAttr[0] != '\0'))
    {
    *Ptr = tmpAttr;

    return(SUCCESS);
    }

  for (tmpL = VC->ParentVCs;tmpL != NULL;tmpL = tmpL->Next)
    {
    tmpVC = (mvc_t *)tmpL->Ptr;

    if (tmpVC == NULL)
      continue;

    if (__MVCGetNodeSetRecursive(tmpVC,Ptr) == SUCCESS)
      return(SUCCESS);
    }

  return(FAILURE);
  }  /* END __MVCGetNodeSetRecursive() */

/**
 *  Searched up through job's parent VCs to find the first one that has a requested node set.
 *   Returns SUCCESS if a node set is found and selected, FAILURE otherwise.
 *
 *  Note that the first nodeset found will be used.
 *
 * @see MJobSelectResourceSet() - parent
 *
 * @param J   [I] - The job to find a node set for
 * @param Ptr [O] - The name of the nodeset to use (if any)
 */

int MVCGetNodeSet(

  const mjob_t  *J,
  char         **Ptr)

  {
  char  *tmpPtr = NULL;
  mln_t *tmpL;
  mvc_t *tmpVC;

  if ((J == NULL) || (Ptr == NULL))
    return(FAILURE);

  *Ptr = NULL;

  for (tmpL = J->ParentVCs;tmpL != NULL;tmpL = tmpL->Next)
    {
    tmpVC = (mvc_t *)tmpL->Ptr;

    if (tmpVC == NULL)
      continue;

    if ((__MVCGetNodeSetRecursive(tmpVC,&tmpPtr) == SUCCESS) &&
        (tmpPtr != NULL))
      break;
    }

  if ((tmpPtr == NULL) ||
      (tmpPtr[0] == '\0'))
    return(FAILURE);

  /* Found a node set, use it */

  *Ptr = tmpPtr;

  return(SUCCESS);
  }  /* END MVCGetNodeSet() */


/**
 * Performs an action on a VC.
 *
 * NOTE: always populate Msg with a description of what happened
 *
 * @param VC         [I] (possibly modified) - The VC to execute the action on
 * @param ActionType [I] - The type of action
 * @param Msg        [O] - Output message
 */

int MVCExecuteAction(

  mvc_t *VC,
  enum MVCExecuteEnum ActionType,
  mstring_t *Msg,
  enum MFormatModeEnum Format)

  {
  if ((VC == NULL) || (Msg == NULL))
    return(FAILURE);

  switch (ActionType)
    {
    case mvceClearMB:

      {
      int rc = SUCCESS;

      /* Clear messages on VC */

      if (VC->MB != NULL)
        {
        rc = MMBFree(&VC->MB,TRUE);

        MVCTransition(VC);
        }

      if (rc == SUCCESS)
        {
        MStringSetF(Msg,"Successfully cleared messages on VC '%s'",
          VC->Name);
        }
      else
        {
        MStringSetF(Msg,"Failed to clear messages on VC '%s'",
          VC->Name);
        }

      return(rc);
      }

      break;

    case mvceScheduleVCJobs:

      {
      mulong StartTime = 0;

      if ((MVCGetGuaranteedStartTime(VC,&StartTime,Msg) == SUCCESS) &&
          (StartTime > 0))
        {
        if (Format == mfmXML)
          {
          MStringSetF(Msg,"%ld",StartTime);
          }
        else
          {
          char Date[MMAX_NAME];

          MUSNCTime(&StartTime,Date);

          MStringSetF(Msg,"Successfully scheduled VC '%s' for '%s'",
            VC->Name,
            Date);
          }

        return(SUCCESS);
        }
      else
        {
        MStringSetF(Msg,"Could not schedule VC '%s'",
          VC->Name);

        return(FAILURE);
        }
      }

      break;

    default:

      MStringSet(Msg,"Invalid arguments");

      return(FAILURE);

      break;
    }

  return(SUCCESS);
  }  /* END MVCExecuteAction() */




/**
 *
 *
 * @param PTypeBM 
 * @param PCredTotal (I) 
 */

int MVCStatClearUsage(

  mbool_t Active,
  mbool_t Idle,
  mbool_t PCredTotal)

  {
  mhashiter_t HTI;
  mvc_t      *VC;
  mcredl_t   *L;

  MUHTIterInit(&HTI);

  while (MUHTIterate(&MVC,NULL,(void **)&VC,NULL,&HTI) == SUCCESS)
    {
    L = NULL;

    if ((MVCGetDynamicAttr(VC,mvcaThrottlePolicies,(void **)&L,mdfOther) == FAILURE) ||
        (L == NULL))
      {
      continue;
      }

    if (Active == TRUE)
      MPUClearUsage(&L->ActivePolicy);

    if ((Idle == TRUE) && (L->IdlePolicy != NULL))
      MPUClearUsage(L->IdlePolicy);

    if (PCredTotal)
      memset(L->TotalJobs,0,sizeof(L->TotalJobs));
    }  /* END for (pindex) */

  return(SUCCESS);
  }   /* END MVCStatClearUsage() */



/**
 * Gets the workflow VC for a job.  Only returns FAILURE if there's a problem
 *  with the parameters, so check if VC was set to see if the job belongs to
 *  a workflow.
 *
 * @param J  [I] - Job to check for
 * @param VC [O] - Set to the workflow VC of the job (if there is one)
 */

int MVCGetJobWorkflow(

  mjob_t *J,
  mvc_t **VC)

  {
  mln_t *tmpL;
  mvc_t *tmpVC;

  if ((J == NULL) || (VC == NULL))
    return(FAILURE);

  *VC = NULL;

  for (tmpL = J->ParentVCs;tmpL != NULL;tmpL = tmpL->Next)
    {
    tmpVC = (mvc_t *)tmpL->Ptr;

    if (tmpVC == NULL)
      continue;

    if (bmisset(&tmpVC->Flags,mvcfWorkflow))
      {
      *VC = tmpVC;

      return(SUCCESS);
      }
    }  /* END for (tmpL) */

  return(SUCCESS);
  }  /* END MVCGetJobWorkflow() */


/**
 * Grabs all variables from a VC hierarchy (from bottom up) and puts
 *  them into the hash table.
 *
 * The VCList is a list of VCs, and this function will add their variables,
 *  along with all of the variables of their parent VCs, and so on until
 *  the top level VCs.
 *
 * The if two variables have the same name, the first one found will be used.
 *
 * Also note that the strings in the hash table values are alloc'd and will
 *  need to be freed.
 *
 * @param VCList         [I] - The linked list of VCs to grab variables from
 * @param VarHash        [O] - The hashtable to put vars into (must already be initialized
 * @param AllowNamespace [I] - If FALSE, will not include namespace vars (i.e. vc3.VMID)
 */

int MVCGrabVarsInHierarchy(

  mln_t   *VCList,
  mhash_t *VarHash,
  mbool_t  AllowNamespace)

  {
  mln_t *tmpL = NULL;
  mvc_t *tmpVC = NULL;

  mhashiter_t Iter;
  char *VarName = NULL;
  char *VarVal = NULL;

  if ((VCList == NULL) ||
      (VarHash == NULL))
    {
    return(FAILURE);
    }

  for (tmpL = VCList;tmpL != NULL;tmpL = tmpL->Next)
    {
    tmpVC = (mvc_t *)tmpL->Ptr;

    if (tmpVC == NULL)
      continue;

    MUHTIterInit(&Iter);

    while (MUHTIterate(&tmpVC->Variables,&VarName,(void **)&VarVal,NULL,&Iter) == SUCCESS)
      {
      if (MUHTGet(VarHash,VarName,NULL,NULL) == SUCCESS)
        {
        /* Variable already added */

        continue;
        }

      if (AllowNamespace == FALSE)
        {
        /* Check if this variable is a namespace variable */

        if (MTrigVarIsNamespace(VarName,NULL) == TRUE)
          continue;
        }

      /* Everything's good, add variable */

      if (VarVal != NULL)
        MUHTAdd(VarHash,VarName,strdup(VarVal),NULL,MUFREE);
      else
        MUHTAdd(VarHash,VarName,NULL,NULL,MUFREE);
      }  /* END while (MUHTIterate(tmpVC->Variables) == SUCCESS) */

    /* Add in variables for this VC's parents */

    MVCGrabVarsInHierarchy(tmpVC->ParentVCs,VarHash,AllowNamespace);
    }  /* END for (tmpL = VCList) */

  return(SUCCESS);
  }  /* END MVCGrabVarsInHierarchy() */


/**
 * Removes VCs that were marked to be harvested.  Called once an iteration.
 * VCs to harvest should be empty before being added to the harvest list.
 */

int MVCHarvestVCs()
  {
  mln_t *tmpL = NULL;
  mvc_t *VC = NULL;
  char   tmpLName[MMAX_NAME];

  MDB(3,fSTRUCT) MLog("INFO:     Checking for VCs to harvest\n");

  /* This for loop will reset to MVCToHarvest because as each VC is free'd,
      it will remove itself from the list.  Each VC removes itself from the
      list so that we don't get problems when sub-VCs are free'd and corrupt
      the pointers on the list */

  for (tmpL = MVCToHarvest;tmpL != NULL;tmpL = MVCToHarvest)
    {
    VC = (mvc_t *)tmpL->Ptr;

    /* Keep the name in case something weird happens to tmpL */

    MUStrCpy(tmpLName,tmpL->Name,MMAX_NAME);

    if ((VC == NULL) ||
        (MVCRemove(&VC,FALSE,FALSE,NULL) == FAILURE))
      {
      /* Remove this link because it wasn't removed by MVCRemove (MVCFree),
          and so we'd get an infinite loop */

      MDB(3,fSTRUCT) MLog("ERROR:  VC '%s' could not be harvested\n",
        tmpLName);

      MULLRemove(&MVCToHarvest,tmpLName,NULL);
      }
    }

  MULLFree(&MVCToHarvest,NULL);

  return(SUCCESS);
  }  /* END MVCHarvestVCs() */

/**
 * Searchs the current VC and all parents to find a 
 *  container hold.
 *  
 *  NOTE:  Calls self recursively until all parents have been
 *  called.
 *
 * @param VC  [I] - The current VC. 
 * @return TRUE for one or more holds, FALSE for no holds found. 
 */

mbool_t MVCIsContainerHold(mvc_t *VC)
  {
  mln_t *Iter = NULL;

  if (bmisset(&VC->Flags,mvcfHoldJobs))
    return(TRUE);

  while(MULLIteratePayload(VC->ParentVCs,&Iter,NULL) == SUCCESS)
    {
    if (MVCIsContainerHold((mvc_t *)Iter->Ptr) == TRUE)
      return(TRUE);
    }

  return(FALSE);
  } /* END MVCIsContainerHold */
