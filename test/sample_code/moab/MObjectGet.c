/* HEADER */

      
/**
 * @file MObjectGet.c
 *
 * Contains: MObjectGet
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  

/**
 *
 *
 * @param OIndex (I)
 * @param OName (I)
 * @param O (O)
 * @param Mode (I) [mAdd or mVerify]
 */

int MOGetObject(
 
  enum MXMLOTypeEnum        OIndex,
  const char               *OName,
  void                    **O,
  enum MObjectSetModeEnum   Mode)
 
  {
  int rc;

  if (O != NULL)
    *O = NULL;
 
  if (O == NULL)
    {
    return(FAILURE);
    }

  if ((OName == NULL) || (OName[0] == '\0'))
    {
    /* should trig be here? */

    if ((OIndex == mxoUser) || 
        (OIndex == mxoGroup) || 
        (OIndex == mxoAcct) || 
        (OIndex == mxoNode) ||
        (OIndex == mxoPar) || 
        (OIndex == mxoRM) || 
        (OIndex == mxoRsv) ||
        (OIndex == mxoClass) || 
        (OIndex == mxoJob) ||
        (OIndex == mxoxVC) ||
        (OIndex == mxoQOS))
      {
      /* object name required */

      return(FAILURE);
      }
    }
 
  switch (OIndex)
    {
    case mxoAcct:

      if (Mode == mAdd)
        rc = MAcctAdd(OName,(mgcred_t **)O);
      else
        rc = MAcctFind(OName,(mgcred_t **)O);

      break;

    case mxoClass:

      if (Mode == mAdd)
        rc = MClassAdd(OName,(mclass_t **)O);
      else
        rc = MClassFind(OName,(mclass_t **)O);

      break;

    case mxoGroup:

      if (Mode == mAdd)
        rc = MGroupAdd(OName,(mgcred_t **)O);          
      else
        rc = MGroupFind(OName,(mgcred_t **)O);         
 
      break;

    case mxoJob:

      /* NOTE:  do not allow job add */

      if (Mode == mAdd)
        {
        rc = MJobFind(OName,(mjob_t **)O,mjsmExtended);
        }
      else
        {
        rc = MJobFind(OName,(mjob_t **)O,mjsmExtended);
        }

      break;

    case mxoUser:

      if (Mode == mAdd)
        rc = MUserAdd(OName,(mgcred_t **)O);
      else
        rc = MUserFind(OName,(mgcred_t **)O);

      break;
 
    case mxoNode:

      if (Mode == mAdd)
        rc = MNodeAdd(OName,(mnode_t **)O);
      else
        rc = MNodeFind(OName,(mnode_t **)O);

      break;
 
    case mxoQOS:

      if (strcmp(OName,DEFAULT) &&
          strcmp(OName,"DEFAULT"))
        {
        if (Mode == mAdd)
          rc = MQOSAdd(OName,(mqos_t **)O);
        else
          rc = MQOSFind(OName,(mqos_t **)O);
        }
      else
        {
        *O = (mqos_t *)&MQOS[0];

        rc = SUCCESS;
        }

      break;

    case mxoPar:

      if (Mode == mAdd)
        rc = MParAdd(OName,(mpar_t **)O);
      else
        rc = MParFind(OName,(mpar_t **)O);

      break;

    case mxoRM:

      if (Mode == mAdd)
        rc = MRMAdd(OName,(mrm_t **)O);
      else
        rc = MRMFind(OName,(mrm_t **)O);

      break;

    case mxoRsv:

      {
      mrsv_t **RP;
   
      /* NOTE:  do not allow rsv add */

      RP = (mrsv_t **)O;

      rc = MRsvFind(OName,RP,mraNONE);
      }    /* END BLOCK */

      break;

    case mxoSys:

      *O = (void *)&MPar[0];  

      rc = SUCCESS;

      break;

    case mxoSched:

      *O = (void *)&MSched;

      rc = SUCCESS;

      break;
 
    case mxoTrig:

      if (Mode == mAdd)
        {
        rc = MTrigAdd(OName,NULL,(mtrig_t **)O);
        }
      else
        {
        rc = MTrigFind(OName,(mtrig_t **)O);
        }

      break;

    case mxoxVM:

      if (Mode == mAdd)
        rc = FAILURE;
      else
        rc = MVMFind(OName,(mvm_t **)O);

      break;

    case mxoxVC:

      if (Mode == mAdd)
        rc = FAILURE;     /* Might change later for dynamic 'add' */
      else
        rc = MVCFind(OName,(mvc_t **)O);

      break;

 
    default:
 
      rc = FAILURE;
 
      break;
    }  /* END switch (OIndex); */
 
  return(rc);
  }  /* END MOGetObject() */



/**
 * Returns the child object of the parent object (ex. fairshare object of user 
 * object).
 *
 * @param O (I) [parent object]
 * @param OIndex (I)
 * @param C (O) [child object]
 * @param CIndex (I)
 */

int MOGetComponent(

  void               *O,      /* I (parent object) */
  enum MXMLOTypeEnum  OIndex, /* I */
  void              **C,      /* O (child object) */
  enum MXMLOTypeEnum  CIndex) /* I */

  {
  if ((O == NULL) || (C == NULL))
    {
    return(FAILURE);
    }

  *C = NULL;

  switch (OIndex)
    {
    case mxoAcct:

      if (CIndex == mxoxLimits)
        *C = (void *)&((mgcred_t *)O)->L;
      else if (CIndex == mxoxFS)
        *C = (void *)&((mgcred_t *)O)->F;
      else if (CIndex == mxoxStats)
        *C = (void *)&((mgcred_t *)O)->Stat;

      break;

    case mxoClass:

      if (CIndex == mxoxLimits)
        *C = (void *)&((mclass_t *)O)->L;
      else if (CIndex == mxoxFS)
        *C = (void *)&((mclass_t *)O)->F;
      else if (CIndex == mxoxStats)
        *C = (void *)&((mclass_t *)O)->Stat;

      break;

    case mxoGroup:

      if (CIndex == mxoxLimits)
        *C = (void *)&((mgcred_t *)O)->L;
      else if (CIndex == mxoxFS)
        *C = (void *)&((mgcred_t *)O)->F;
      else if (CIndex == mxoxStats)
        *C = (void *)&((mgcred_t *)O)->Stat;
 
      break;

    case mxoJob:

      if (CIndex == mxoxFS)
        {
        mtjobstat_t *JS = (mtjobstat_t *)((mjob_t *)O)->ExtensionData;

        if (JS != NULL)
          *C = (void *)&JS->F;
        }
      else if (CIndex == mxoxLimits)
        {
        mtjobstat_t *JS = (mtjobstat_t *)((mjob_t *)O)->ExtensionData;

        if (JS != NULL)   
          *C = (void *)&JS->L;
        }
      else if (CIndex == mxoxStats)
        {
        mtjobstat_t *JS = (mtjobstat_t *)((mjob_t *)O)->ExtensionData;

        if (JS != NULL)
          *C = (void *)&JS->S;
        }
      else if (CIndex == mxoTrig)
        *C = (void *)&((mjob_t *)O)->Triggers;

      break;

    case mxoNode:

      if (CIndex == mxoTrig)
        *C = (void *)&((mnode_t *)O)->T;
      else if (CIndex == mxoxStats)
        *C = (void *)&((mnode_t *)O)->Stat;

      break;

    case mxoPar:

      if (CIndex == mxoxLimits)
        *C = (void *)&((mpar_t *)O)->L;
      else if (CIndex == mxoxFS)
        *C = (void *)&((mpar_t *)O)->F;
      else if (CIndex == mxoxStats)
        *C = (void *)&((mpar_t *)O)->S;

      break;

    case mxoQOS:

      if (CIndex == mxoxLimits)
        *C = (void *)&((mqos_t *)O)->L;
      else if (CIndex == mxoxFS)
        *C = (void *)&((mqos_t *)O)->F;
      else if (CIndex == mxoxStats)
        *C = (void *)&((mqos_t *)O)->Stat;

      break;

    case mxoRM:

      if (CIndex == mxoTrig)
        *C = (void *)&((mrm_t *)O)->T;

      break;

    case mxoRsv:

      if (CIndex == mxoTrig)
        *C = (void *)&((mrsv_t *)O)->T;

      break;

    case mxoxVC:

      if (CIndex == mxoTrig)
        {
        *C = (void *)&((mvc_t *)O)->T;
        }
      else if (CIndex == mxoxLimits)
        {
        if (MVCGetDynamicAttr((mvc_t *)O,mvcaThrottlePolicies,C,mdfOther) == FAILURE)
          {
          *C = NULL;
          }
        }

      break;

    case mxoSched:

      if (CIndex == mxoxLimits)
        *C = (void *)&((msched_t *)O)->GP->L;
      else if (CIndex == mxoxFS)
        *C = (void *)&((msched_t *)O)->GP->F; 
      else if (CIndex == mxoxStats)
        *C = (void *)&((msched_t *)O)->GP->S; 
      else if (CIndex == mxoTrig)
        *C = (void *)&((msched_t *)O)->TList;

      break;

    case mxoSys:

      /* object is global partition */

      if (CIndex == mxoxLimits)
        *C = (void *)&((mpar_t *)O)->L;
      else if (CIndex == mxoxFS)
        *C = (void *)&((mpar_t *)O)->F;
      else if (CIndex == mxoxStats)
        *C = (void *)&((mpar_t *)O)->S;

      break;

    case mxoUser:

      if (CIndex == mxoxLimits)
        *C = (void *)&((mgcred_t *)O)->L;
      else if (CIndex == mxoxFS)
        *C = (void *)&((mgcred_t *)O)->F;
      else if (CIndex == mxoxStats)
        *C = (void *)&((mgcred_t *)O)->Stat;

      break;

    case mxoxFSC:

      if (CIndex == mxoxLimits)
        *C = (void *)((mfst_t *)O)->L;
      else if (CIndex == mxoxFS)
        *C = (void *)((mfst_t *)O)->F;
      else if (CIndex == mxoxStats)
        *C = (void *)((mfst_t *)O)->S;

      break;

    default:

      /* NO-OP */

      break;
    }  /* END switch (OIndex) */

  if (*C == NULL)
    {
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MOGetComponent() */


/**
 * Returns a string representing the name of the given object.
 *
 * @param O (I)
 * @param OIndex (I)
 * @param NameP (O) [optional]
 */

char *MOGetName(

  void  *O,      /* I */
  enum MXMLOTypeEnum OIndex, /* I */
  char **NameP)  /* O (optional) */

  {
  char *ptr = NULL;

  if (O == NULL)
    {
    return(NULL);
    }

  switch (OIndex)
    {
    case mxoAcct:

      ptr = ((mgcred_t *)O)->Name;

      break;

    case mxoClass:

      ptr = ((mclass_t *)O)->Name;

      break;

    case mxoGroup:

      ptr = ((mgcred_t *)O)->Name;

      break;

    case mxoJob:

      ptr = ((mjob_t *)O)->Name;

      break;

    case mxoNode:

      ptr = ((mnode_t *)O)->Name;

      break;

    case mxoPar:

      ptr = ((mpar_t *)O)->Name;

      break;

    case mxoQOS:

      ptr = ((mqos_t *)O)->Name;

      break;

    case mxoRsv:

      ptr = ((mrsv_t *)O)->Name;

      break;

    case mxoSched:

      ptr = ((msched_t *)O)->Name;

      break;

    case mxoTrig:

      ptr = ((mtrig_t *)O)->TName;

      break;

    case mxoUser:

      ptr = ((mgcred_t *)O)->Name;

      break;

    case mxoxVM:

      ptr = ((mvm_t *)O)->VMID;

      break;

    default:

      ptr = NULL;

      break;
    }  /* END switch (OIndex) */

  if (NameP != NULL)
    *NameP = ptr;

  return(ptr);
  }  /* END MOGetName() */


/* END MObjectGet.c */
