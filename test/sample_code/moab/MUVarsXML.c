/* HEADER */

/**
 * @file MUVarsXML.c
 *
 * Contains functions for translating hash table of variable to/from XML 
 */

#include "moab.h"
#include "moab-proto.h"
#include "moab-const.h"
#include "moab-global.h"


/**
 * This function will take a given hash table of variables and create
 * XML elements to contain them.
 *
 * @param XMLE (I) The XML element to create
 * @param Variables (I) The Variables mash_t * that contains the variables.
 */

int MUVarsToXML(

  mxml_t  **XMLE,
  mhash_t  *Variables)

  {
  mhashiter_t VarIter;
  char *VarName = NULL;
  char *VarValue = NULL;
  mxml_t *VarListE = NULL;
  mxml_t *VarE = NULL;

  if ((XMLE == NULL) ||
      (Variables == NULL))
    {
    return(FAILURE);
    }

  if (Variables->NumItems <= 0)
    return(SUCCESS);

  MXMLCreateE(&VarListE,"Variables");

  MUHTIterInit(&VarIter);

  /* iterate through all variables and create XML children for the "Variables" list */

  while (MUHTIterate(Variables,&VarName,(void **)&VarValue,NULL,&VarIter) == SUCCESS)
    {
    VarE = NULL;

    MXMLCreateE(&VarE,"Variable");
    MXMLSetAttr(VarE,"name",VarName,mdfString);
    MXMLSetVal(VarE,VarValue,mdfString);
    MXMLAddE(VarListE,VarE);
    } /* END while (MUHTIterate(...) == SUCCESS) */

  *XMLE = VarListE;

  return(SUCCESS);
  }  /* END MUAddVarsToXML() */


/**
 * This function will take the given hash table of variables and create
 * XML elements to contain them. It will then attach these elements to the given
 * XML tag as a child.
 *
 * @param XMLE (I) The XML element to attach the variable tags to.
 * @param Variables (I) The Variables mhash_t * that contains the variables.
 */

int MUAddVarsToXML(
  
  mxml_t  *XMLE,
  mhash_t *Variables)

  {
  mhashiter_t VarIter;
  char *VarName = NULL;
  char *VarValue = NULL;
  mxml_t *VarListE = NULL;
  mxml_t *VarE = NULL;

  if ((XMLE == NULL) ||
      (Variables == NULL))
    {
    return(FAILURE);
    }

  if (Variables->NumItems <= 0)
    return(SUCCESS);

  MXMLCreateE(&VarListE,"Variables");

  MUHTIterInit(&VarIter);

  /* iterate through all variables and create XML children for the "Variables" list */

  while (MUHTIterate(Variables,&VarName,(void **)&VarValue,NULL,&VarIter) == SUCCESS)
    {
    VarE = NULL;

    MXMLCreateE(&VarE,"Variable");
    MXMLSetAttr(VarE,"name",VarName,mdfString);
    MXMLSetVal(VarE,VarValue,mdfString);
    MXMLAddE(VarListE,VarE);
    } /* END while (MUHTIterate(...) == SUCCESS) */

  /* add the variable list to the given XML element */

  MXMLAddE(XMLE,VarListE);

  return(SUCCESS);
  }  /* END MUAddVarsToXML() */


/*
 * Functions the same as MUAddVarsToXML, but takes a linked list instead of a hash.
 *
 * @param XMLE (I) The XML element to attach the variable tags to.
 * @param Variables (I) The Variables mln_t * that contains the variables.
 */

int MUAddVarsLLToXML(

  mxml_t *XMLE,
  mln_t  *Variables)

  {
  mxml_t *VarListE = NULL;
  mxml_t *VarE = NULL;
  mln_t *tmpL = NULL;

  if (XMLE == NULL)
    {
    return(FAILURE);
    }

  if (Variables == NULL)
    return(SUCCESS);

  tmpL = Variables;
  MXMLCreateE(&VarListE,"Variables");

  while (tmpL != NULL)
    {
    VarE = NULL;

    MXMLCreateE(&VarE,"Variable");
    MXMLSetAttr(VarE,"name",tmpL->Name,mdfString);
    MXMLSetVal(VarE,(char *)tmpL->Ptr,mdfString);
    MXMLAddE(VarListE,VarE);

    tmpL = tmpL->Next;
    }

  MXMLAddE(XMLE,VarListE);

  return(SUCCESS);
  }  /* END MUAddVarsLLToXML() */



/**
 * Adds variables to the given hash table (clears old variables)
 *
 * @param XMLE      [I] - 
 * @param Variables [O] (modified) - 
 */

int MUAddVarsFromXML(

  mxml_t *XMLE,
  mhash_t *Variables)
  {
  char VarName[MMAX_LINE];
  char *VarValue = NULL;
  mxml_t *VarE = NULL;
  int VarETok = -1;

  if ((XMLE == NULL) || (Variables == NULL))
    {
    return(FAILURE);
    }

  if (strcasecmp(XMLE->Name,"Variables"))
    return(FAILURE);

  /* clear out existing variables */
  MUHTClear(Variables,TRUE,MUFREE);

  /* iterate through children and extract info to build up the variables */
  while (MXMLGetChildCI(XMLE,"Variable",&VarETok,&VarE) == SUCCESS)
    {
    VarValue = NULL;

    MXMLGetAttr(VarE,"name",NULL,VarName,sizeof(VarName));
    MUStrDup(&VarValue,VarE->Val);

    MUHTAdd(Variables,VarName,(void *)VarValue,NULL,NULL);
    }

  return(SUCCESS);
  } /* END MUAddVarsFromXML() */

/* END MUVarsXML.c */
