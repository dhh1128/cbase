/* HEADER */

/**
 * @file MVMCRXML.c
 * 
 * Contains various functions VM CR XML  
 *
 */


#include "moab.h"
#include "moab-proto.h"
#include "moab-const.h"
#include "moab-global.h"


/*
 * Converts a mvm_req_create_t to XML.
 *
 * @param VMCR (I) The structure to convert to XML
 * @param VEP  (O) XML pointer to be created and/or populated
 */

int MVMCRToXML(

  mvm_req_create_t *VMCR, /* I */
  mxml_t          **VEP)  /* O */

  {
  mxml_t *VE = NULL;

  if (VMCR == NULL)
    return(FAILURE);

  if (*VEP == NULL)
    {
    if (MXMLCreateE(VEP,(char *)MXO[mxoxVMCR]) == FAILURE)
      {
      MDB(3,fCKPT) MLog("INFO:     failed to create xml for VMCR for '%s'\n",
        VMCR->VMID);

      return(FAILURE);
      }
    }

  VE = *VEP;

  if (VMCR->VMID[0] != '\0')
    {
    MXMLSetAttr(VE,"ID",(void *)VMCR->VMID,mdfString);
    }

  if (VMCR->Vars[0] != '\0')
    {
    MXMLSetAttr(VE,"VARS",(void *)VMCR->Vars,mdfString);
    }

  if (VMCR->Storage[0] != '\0')
    {
    MXMLSetAttr(VE,"STORAGE",(void *)VMCR->Storage,mdfString);
    }

  if (VMCR->Aliases[0] != '\0')
    {
    MXMLSetAttr(VE,"ALIAS",(void *)VMCR->Aliases,mdfString);
    }

  if (VMCR->Triggers[0] != '\0')
    {
    MXMLSetAttr(VE,"TRIGGER",(void *)VMCR->Triggers,mdfString);
    }

  if (VMCR->N != NULL)
    {
    MXMLSetAttr(VE,"HYPERVISOR",(void *)VMCR->N->Name,mdfString);
    }

  if (VMCR->JT != NULL)
    {
    MXMLSetAttr(VE,"TEMPLATE",(void *)VMCR->JT->Name,mdfString);
    }

  if (VMCR->OwnerJob != NULL)
    {
    MXMLSetAttr(VE,"OWNERJOB",(void *)VMCR->OwnerJob->Name,mdfString);
    }

  if (VMCR->OSIndex != 0)
    {
    /* Set to string so that OS loading order doesn't matter */

    MXMLSetAttr(VE,"IMAGE",(void *)MAList[meOpsys][VMCR->OSIndex],mdfString);
    }

  if (VMCR->TrackingJ[0] != '\0')
    MXMLSetAttr(VE,"TRACKINGJ",(void *)VMCR->TrackingJ,mdfString);

  MXMLSetAttr(VE,"WALLTIME",(void *)&VMCR->Walltime,mdfLong);
  MXMLSetAttr(VE,"SOVEREIGN",(void *)MBool[VMCR->IsSovereign],mdfString);
  MXMLSetAttr(VE,"ISONETIMEUSE",(void *)MBool[VMCR->IsOneTimeUse],mdfString);

  /* CRes info */

  MXMLSetAttr(VE,"DISK",(void *)&VMCR->CRes.Disk,mdfInt); 
  MXMLSetAttr(VE,"MEM",(void *)&VMCR->CRes.Mem,mdfInt); 
  MXMLSetAttr(VE,"PROCS",(void *)&VMCR->CRes.Procs,mdfInt);

  return(SUCCESS);
  } /* END MVMCRToXML() */



/*
 * Populates a mvm_req_create_t structure from XML.
 *
 * @param VMCR (I) [modified] - The VMCR structure to populate (must already exist)
 * @param E    (I) - The XML structure
 */

int MVMCRFromXML(

  mvm_req_create_t *VMCR, /* I (modified) */
  mxml_t           *E)    /* I */

  {
  int aindex;

  if ((VMCR == NULL) || (E == NULL))
    {
    return(FAILURE);
    }

  for (aindex = 0;aindex < E->ACount;aindex++)
    {
    if (!strcmp("ID",E->AName[aindex]))
      {
      MUStrCpy(VMCR->VMID,E->AVal[aindex],sizeof(VMCR->VMID));
      }
    else if (!strcmp("VARS",E->AName[aindex]))
      {
      MUStrCpy(VMCR->Vars,E->AVal[aindex],sizeof(VMCR->Vars));
      }
    else if (!strcmp("STORAGE",E->AName[aindex]))
      {
      MUStrCpy(VMCR->Storage,E->AVal[aindex],sizeof(VMCR->Storage));
      }
    else if (!strcmp("ALIAS",E->AName[aindex]))
      {
      MUStrCpy(VMCR->Aliases,E->AVal[aindex],sizeof(VMCR->Aliases));
      }
    else if (!strcmp("TRIGGER",E->AName[aindex]))
      {
      MUStrCpy(VMCR->Triggers,E->AVal[aindex],sizeof(VMCR->Triggers));
      }
    else if (!strcmp("HYPERVISOR",E->AName[aindex]))
      {
      /* Find node for hypervisor */

      MNodeFind(E->AVal[aindex],&VMCR->N);
      }
    else if (!strcmp("TEMPLATE",E->AName[aindex]))
      {
      MJobFind(E->AVal[aindex],&VMCR->JT,mjsmExtended);
      }
    else if (!strcmp("OWNERJOB",E->AName[aindex]))
      {
      MJobFind(E->AVal[aindex],&VMCR->OwnerJob,mjsmExtended);
      }
    else if (!strcmp("IMAGE",E->AName[aindex]))
      {
      VMCR->OSIndex = MUMAGetIndex(meOpsys,E->AVal[aindex],mAdd);
      }
    else if (!strcmp("WALLTIME",E->AName[aindex]))
      {
      VMCR->Walltime = strtol(E->AVal[aindex],NULL,10);
      }
    else if (!strcmp("SOVEREIGN",E->AName[aindex]))
      {
      VMCR->IsSovereign = MUBoolFromString(E->AName[aindex],FALSE);
      }
    else if (!strcmp("ISONETIMEUSE",E->AName[aindex]))
      {
      VMCR->IsOneTimeUse = MUBoolFromString(E->AName[aindex],FALSE);
      }
    else if (!strcmp("DISK",E->AName[aindex]))
      {
      VMCR->CRes.Disk = (int)strtol(E->AVal[aindex],NULL,10);
      }
    else if (!strcmp("MEM",E->AName[aindex]))
      {
      VMCR->CRes.Mem = (int)strtol(E->AVal[aindex],NULL,10);
      }
    else if (!strcmp("PROCS",E->AName[aindex]))
      {
      VMCR->CRes.Procs = (int)strtol(E->AVal[aindex],NULL,10);
      }
    else if (!strcmp("TRACKINGJ",E->AName[aindex]))
      {
      MUStrCpy(VMCR->TrackingJ,E->AVal[aindex],sizeof(VMCR->TrackingJ));
      }
    }  /* END for (aindex = 0;aindex < E->ACount;aindex++) */
    
  return(SUCCESS);
  } /* END MVMCRFromXML() */
/* END MVMCRXML.c */
