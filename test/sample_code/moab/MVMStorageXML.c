/* HEADER */

/**
 * @file MVMStorageXML.c
 * 
 * Contains various functions for VM Storage to/from XML
 *
 */


#include "moab.h"
#include "moab-proto.h"
#include "moab-const.h"
#include "moab-global.h"


/*
 * Helper function to convert a VM's storage mounts to XML.
 *
 * @param OE [I - modified] XML to put the storage mounts XML under.
 * @param V  [I] VM whose storage mounts to convert
 */

int MVMStorageMountsToXML(

  mxml_t *OE,  /* I (modified) */
  mvm_t  *V)   /* I */

  {
  mln_t *Iter = NULL;
  mxml_t *SEE;

  if ((OE == NULL) || (V == NULL))
    {
    return(FAILURE);
    }

  if (V->Storage == NULL)
    {
    return(SUCCESS);
    }

  while (MULLIterate(V->Storage,&Iter) == SUCCESS)
    {
    mvm_storage_t *SMount = (mvm_storage_t *)Iter->Ptr;

    SEE = NULL;

    MXMLCreateE(&SEE,"storage");

    MXMLSetAttr(SEE,"name",(void *)SMount->Name,mdfString);
    MXMLSetAttr(SEE,"type",(void *)SMount->Type,mdfString);
    MXMLSetAttr(SEE,"size",(void *)&SMount->Size,mdfInt);

    if (SMount->MountPoint[0] != '\0')
      MXMLSetAttr(SEE,"mount_point",(void *)SMount->MountPoint,mdfString);

    if (SMount->Location[0] != '\0')
      MXMLSetAttr(SEE,"location",(void *)SMount->Location,mdfString);

    if (SMount->R != NULL)
      MXMLSetAttr(SEE,"rsv",(void *)SMount->R->Name,mdfString);

    if (SMount->T != NULL)
      MXMLSetAttr(SEE,"trigger",(void *)SMount->T->TName,mdfString);

    if (SMount->TVar[0] != '\0')
      MXMLSetAttr(SEE,"tvar",(void *)SMount->TVar,mdfString);

    if (SMount->Hostname[0] != '\0')
      MXMLSetAttr(SEE,"hostname",(void *)SMount->Hostname,mdfString);

    if (SMount->MountOpts[0] != '\0')
      MXMLSetAttr(SEE,"mountOpts",(void *)SMount->MountOpts,mdfString);

    MXMLAddE(OE,SEE);
    }

  return(SUCCESS);
  } /* END MVMStorageMountsToXML() */ 




/*
 * Adds a storage mount to the given VM based on the passed in XML
 *
 * Does not check if the storage mount already exists
 *
 * @param VM [I] (modified) - VM to add the storage mount to
 * @param E  [I] - XML structure containing storage mount info
 */

int MVMStorageMountFromXML(
  mvm_t *VM,
  mxml_t *E)

  {
  mvm_storage_t *StorageMount;

  int aindex;

  if ((VM == NULL) || (E == NULL))
    {
    return(FAILURE);
    }

  if (strcmp(E->Name,"storage"))
    {
    /* Only parse if it is actually a storage child */

    return(FAILURE);
    }

  StorageMount = (mvm_storage_t *)MUCalloc(1,sizeof(mvm_storage_t));

  memset(StorageMount,0,sizeof(mvm_storage_t));

  for (aindex = 0;aindex < E->ACount;aindex++)
    {
    if (!strcmp(E->AName[aindex],"name"))
      {
      MUStrCpy(StorageMount->Name,E->AVal[aindex],sizeof(StorageMount->Name));
      }
    else if (!strcmp(E->AName[aindex],"type"))
      {
      MUStrCpy(StorageMount->Type,E->AVal[aindex],sizeof(StorageMount->Type));
      }
    else if (!strcmp(E->AName[aindex],"size"))
      {
      StorageMount->Size = (int)strtol(E->AVal[aindex],NULL,10);
      }
    else if (!strcmp(E->AName[aindex],"mount_point"))
      {
      MUStrCpy(StorageMount->MountPoint,E->AVal[aindex],sizeof(StorageMount->MountPoint));
      }
    else if (!strcmp(E->AName[aindex],"location"))
      {
      MUStrCpy(StorageMount->Location,E->AVal[aindex],sizeof(StorageMount->Location));
      }
    else if (!strcmp(E->AName[aindex],"rsv"))
      {
      /* How do we know that the reservation has already been loaded? */
      }
    else if (!strcmp(E->AName[aindex],"trigger"))
      {
      /* Same question as for rsv */
      /* Don't really need trigger, though, since it is covered by TVar */
      }
    else if (!strcmp(E->AName[aindex],"tvar"))
      {
      MUStrCpy(StorageMount->TVar,E->AVal[aindex],sizeof(StorageMount->TVar));
      }
    else if (!strcmp(E->AName[aindex],"hostname"))
      {
      MUStrCpy(StorageMount->Hostname,E->AVal[aindex],sizeof(StorageMount->Hostname));
      }
    else if (!strcmp(E->AName[aindex],"mountOpts"))
      {
      MUStrCpy(StorageMount->MountOpts,E->AVal[aindex],sizeof(StorageMount->MountOpts));
      }
    } /* END for (aindex = 0;aindex < E->ACount;aindex++) */

  if (VM->Storage == NULL)
    {
    MULLCreate(&VM->Storage);
    }

  MULLAdd(&VM->Storage,StorageMount->Name,(void *)StorageMount,NULL,NULL);

  return(SUCCESS);
  } /* END MVMStorageMountFromXML() */

/* END MVMStorageXML.c */
