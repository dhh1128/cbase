/* HEADER */

      
/**
 * @file MJGroup.c
 *
 * Contains: Job Group functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 * Initialize J->JGroup structure.
 *
 * @see MJobApplySetTemplate() - parent
 *
 * NOTE:  will alloc J->JGroup
 *
 * @param J (I) [modified]
 * @param Name (I) [optional]
 * @param ArrayIndex (I) [optional]
 * @param JGroup (I) [optiona]
 */

int MJGroupInit(

  mjob_t *J,
  char   *Name,
  int     ArrayIndex,
  mjob_t *JGroup)

  {
  if (J == NULL)
    {
    return(FAILURE);
    }

  if (J->JGroup != NULL)
    {
    MUFree((char **)&J->JGroup);

    J->JGroup = NULL;
    }

  J->JGroup = (mjobgroup_t *)MUCalloc(1,sizeof(mjobgroup_t));

  if ((Name != NULL) && (Name[0] != '\0'))
    MUStrCpy(J->JGroup->Name,Name,sizeof(J->JGroup->Name));

  /* JGroup may or may not be NULL */

  J->JGroup->RealJob = JGroup;

  J->JGroup->ArrayIndex = ArrayIndex;

  return(SUCCESS);
  }  /* END MJGroupInit() */





/**
 * Free J->JGroup structure.
 *
 * @param J (modified)
 */

int MJGroupFree(

  mjob_t *J) /* I */

  {
  if ((J == NULL) || (J->JGroup == NULL))
    {
    return(SUCCESS);
    }

  MULLFree(&J->JGroup->SubJobs,NULL);

  if (J->JGroup != NULL)
    {
    MUFree((char **)&J->JGroup);
    }

  J->JGroup = NULL;

  return(SUCCESS);
  }  /* END MJGroupInit() */





/**
 * Duplicate J->JGroup structure.
 *
 * NOTE:  alloc first with MJGroupInit
 *
 * @param SrcJGroup (I)
 * @param DestJ     (I)
 */

int MJGroupDup(

  mjobgroup_t *SrcJGroup,
  mjob_t      *DestJ)

  {
  if (SrcJGroup == NULL)
    {
    return(FAILURE);
    }

  MJGroupFree(DestJ);

  MJGroupInit(DestJ,SrcJGroup->Name,-1,SrcJGroup->RealJob);

  MULLDup(&DestJ->JGroup->SubJobs,SrcJGroup->SubJobs);

  return(SUCCESS);
  }  /* END MJGroupDup() */
/* END MJGroup.c */
