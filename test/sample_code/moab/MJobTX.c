/* HEADER */

      
/**
 * @file MJobTX.c
 *
 * Contains: Functions for mjtx_t manipulation
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 * Get a TX (mjtx_t) object from XML.
 *
 * @param TX - [I] The mjtx_t structure to be populated
 * @param TE - [I] The XML structure
 * @param J  - [I] The job that TX is attached to (optional, mostly)
 */

int MTXFromXML(

  mjtx_t *TX, /* I (Modified) */
  mxml_t *TE, /* I */
  mjob_t *J)  /* I (optional, used for inheritres and genericsysjob) */

  {
  char tmpVal[MMAX_LINE];

  if ((TX == NULL) || (TE == NULL))
    {
    return(FAILURE);
    }

  if (J != NULL)
    {
    /* Generic system job */

    if (MXMLGetAttr(TE,(char *)MJobCfgAttr[mjcaGenSysJob],NULL,tmpVal,sizeof(tmpVal)) == SUCCESS)
      {
      if (MUBoolFromString(tmpVal,FALSE) == TRUE)
        bmset(&J->IFlags,mjifGenericSysJob);
      else
        bmunset(&J->IFlags,mjifGenericSysJob);
      }

    /* Inherit res */

    if (MXMLGetAttr(TE,(char *)MJobCfgAttr[mjcaInheritResources],NULL,tmpVal,sizeof(tmpVal)) == SUCCESS)
      {
      if (MUBoolFromString(tmpVal,FALSE) == TRUE)
        bmset(&J->IFlags,mjifInheritResources);
      else
        bmunset(&J->IFlags,mjifInheritResources);
      }

    /* Check template dependencies */

    if (MXMLGetAttr(TE,(char *)MJobCfgAttr[mjcaTemplateDepend],NULL,tmpVal,sizeof(tmpVal)) == SUCCESS)
      {
      mstring_t tmpString(MMAX_LINE);

      /* FORMAT: <type:name>[<type:name>] */

      MStringSet(&tmpString,(char *)MJTXAttr[mjtxaTemplateDepend]);
      MStringAppend(&tmpString,"=");
      MStringAppend(&tmpString,tmpVal);

      MTJobProcessAttr(J,tmpString.c_str());
      }
    }  /* END if (J != NULL) */

  if (MXMLGetAttr(TE,(char *)"JobReceivingAction",NULL,tmpVal,sizeof(tmpVal)) == SUCCESS)
    {
    MUStrDup(&TX->JobReceivingAction,tmpVal);
    }

  if (MXMLGetAttr(TE,(char *)"TJobAction",NULL,tmpVal,sizeof(tmpVal)) == SUCCESS)
    {
    TX->TJobAction = (enum MTJobActionTemplateEnum)MUGetIndexCI(tmpVal,MTJobActionTemplate,FALSE,mtjatNONE);
    }

  return(SUCCESS);
  }  /* END MTXFromXML() */



/* Checks if J->TX has been allocated, and allocates if it hasn't. */

int MJobCheckAllocTX(
  mjob_t *J) /* I (modified) */

  {
  if (J->TemplateExtensions == NULL)
    return MJobAllocTX(J);

  return(SUCCESS);
  }


/**
 * Allocate job extension structure.
 *
 * @see MJobDestroyTX() - peer - free job mjtx_t structure 
 *
 * @param J (I) [modified]
 */

int MJobAllocTX(

  mjob_t *J) /* I (modified) */

  {
  mbool_t AlreadyInitialized = FALSE;

  if (J->TemplateExtensions != NULL)
    {
    AlreadyInitialized = TRUE;
    }

  if ((J->TemplateExtensions == NULL) &&
     ((J->TemplateExtensions = (mjtx_t *)MUCalloc(1,sizeof(mjtx_t))) == NULL))
    {
    MDB(0,fSCHED) MLog("ALERT:    no memory for template data\n");

    return(FAILURE);
    }

  if (AlreadyInitialized == FALSE)
    {
    memset(J->TemplateExtensions,0,sizeof(mjtx_t));

    MUArrayListCreate(&J->TemplateExtensions->Signals,sizeof(mreqsignal_t),2);

    /* perform a 'placement new' constructor call on the mstring_t */
    new (&J->TemplateExtensions->ReqSetTemplates) mstring_t(MMAX_NAME);
    }

  return(SUCCESS);
  }  /* END MJobAllocTX() */





/**
 * Destroy job extension structure.
 *
 * @see MJobDestroy() - parent
 * @see MJobAllocTX() - peer - alloc job mjtx_t structure
 *
 * @param J (I) [modified]
 */

int MJobDestroyTX(

  mjob_t *J)  /* I (modified) */

  {
  int tindex;

  if (J == NULL)
    {
    return(FAILURE);
    }

  if (J->TemplateExtensions == NULL)
    {
    return(SUCCESS);
    }

  if (J->TemplateExtensions->WorkloadRMID != NULL)
    MUFree(&J->TemplateExtensions->WorkloadRMID);

  for (tindex = 0;tindex < MMAX_SPECTRIG;tindex++)
    {
    MUFree(&J->TemplateExtensions->SpecTrig[tindex]);
    }

  if (J->TemplateExtensions->Action != NULL)
    {
    MULLFree(&J->TemplateExtensions->Action,MUFREE);
    }

  MULLFree(&J->TemplateExtensions->Depends, MUFREE);

  MUFree(&J->TemplateExtensions->DestroyTemplate);
  MUFree(&J->TemplateExtensions->MigrateTemplate);
  MUFree(&J->TemplateExtensions->ModifyTemplate);

  MUArrayListFree(&J->TemplateExtensions->Signals);

  /* call the mstring_t destructor */
  J->TemplateExtensions->ReqSetTemplates.~mstring_t();

  MUFree(&J->TemplateExtensions->SpecTemplates);
  MUFree(&J->TemplateExtensions->SpecArray);

  MUFree(&J->TemplateExtensions->JobReceivingAction);

  bmclear(&J->TemplateExtensions->TemplateFlags);
  bmclear(&J->TemplateExtensions->SetAttrs);

  MUFree((char **)&J->TemplateExtensions);

  return(SUCCESS);
  }  /* END MJobDestroyTX() */
/* END MJobTX.c */
