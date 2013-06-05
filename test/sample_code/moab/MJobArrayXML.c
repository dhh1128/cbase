/* HEADER */

/**
 * @file MJobArrayXML.c
 *
 * Contains JobArray ToXML and FromXML functions
 * Isolated due to dependencies on XML routines
 */


#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/* 
 * Translates a J->Array structure into mxml_t format.
 *
 * NOTE: this is used for checkpointing
 *
 * @param mjob_t   *J
 * @param mxml_t **DE
 *
 */

int MJobArrayToXML(

  mjob_t  *J,   /* I */
  mxml_t **DE)  /* O */

  {
  int jindex;

  mjarray_t *JA;

  mxml_t *JE;
  mxml_t *CE;

  if ((J == NULL) || (DE == NULL) || (J->Array == NULL))
    {  
    return(FAILURE);
    }

  JA = J->Array;

  *DE = NULL;

  JE = NULL;
  
  MXMLCreateE(&JE,(char *)MJobAttr[mjaArrayInfo]);

  if (JA->Limit > 0)
    MXMLSetAttr(JE,"Limit",(void **)&JA->Limit,mdfInt);

  /* save Active, Idle, and Complete counts for array */
  MXMLSetAttr(JE,"Active",(void **)&JA->Active,mdfInt);
  MXMLSetAttr(JE,"Idle",(void **)&JA->Idle,mdfInt);
  MXMLSetAttr(JE,"Complete",(void **)&JA->Complete,mdfInt);

  MXMLSetAttr(JE,"Name",(void **)JA->Name,mdfString);
  MXMLSetAttr(JE,"Count",(void **)&JA->Count,mdfInt);

  for (jindex = 0;jindex < MMAX_JOBARRAYSIZE;jindex++)
    {
    if (JA->Members[jindex] == -1)
      break;

    MXMLCreateE(&CE,(char *)"child");

    MXMLSetAttr(CE,"Name",(void **)JA->JName[jindex],mdfString);
    MXMLSetAttr(CE,"State",(void **)MJobState[JA->JState[jindex]],mdfString);
    MXMLSetAttr(CE,"JobArrayIndex",(void **)&JA->Members[jindex],mdfInt);

    MXMLAddE(JE,CE);
    }
  
  *DE = JE;

  return(SUCCESS);
  }  /* END MJobArrayToXML() */





/** 
 * Populates J->Array from an mxml_t structure.
 *
 * NOTE: this routine will free any old J->Array structure
 *
 * @param J
 * @param JA
 *
 */

int MJobArrayFromXML(

  mjob_t *J,
  mxml_t *JA)

  {
  mxml_t *CE;

  int CTok;
  int jindex;
  
  char tmpVal[MMAX_LINE];

  if ((J == NULL) || (JA == NULL))
    {
    return(FAILURE);
    }

  if (J->Array != NULL)
    {
    MJobArrayFree(J);
    }

  if (MXMLGetAttr(JA,"Name",NULL,tmpVal,sizeof(tmpVal)) == FAILURE)
    {
    return(FAILURE);
    }

  /* Alloc the JobArray, if no memory, then fail */
  if (MJobArrayAlloc(J) == FAILURE)
    {
    return(FAILURE);
    }

  MUStrCpy(J->Array->Name,tmpVal,MMAX_NAME);

  /* get limit, active, idle and complete for the array */
  if (MXMLGetAttr(JA,"Limit",NULL,tmpVal,sizeof(tmpVal)) != FAILURE)
    {
    J->Array->Limit = strtol(tmpVal,NULL,10);
    }

  if (MXMLGetAttr(JA,"Active",NULL,tmpVal,sizeof(tmpVal)) != FAILURE)
    {
    J->Array->Active = strtol(tmpVal,NULL,10);
    }
  
  if (MXMLGetAttr(JA,"Idle",NULL,tmpVal,sizeof(tmpVal)) != FAILURE)
    {
    J->Array->Idle = strtol(tmpVal,NULL,10);
    }
  
  if (MXMLGetAttr(JA,"Complete",NULL,tmpVal,sizeof(tmpVal)) != FAILURE)
    {
    J->Array->Complete = strtol(tmpVal,NULL,10);
    }

  if (MXMLGetAttr(JA,"Count",NULL,tmpVal,sizeof(tmpVal)) != FAILURE)
    {
    J->Array->Count = strtol(tmpVal,NULL,10);
    }

  J->Array->JPtrs  = (mjob_t **)MUCalloc(1,sizeof(mjob_t *) * (J->Array->Count + 1));
  J->Array->JName  = (char **)MUCalloc(1,sizeof(char *) * (J->Array->Count + 1));
  J->Array->JState = (enum MJobStateEnum *)MUCalloc(1,sizeof(enum MJobStateEnum) * (J->Array->Count + 1));

  CTok   = -1;
  jindex =  0;

  while (MXMLGetChild(JA,"child",&CTok,&CE) != FAILURE)
    {
    if (MXMLGetAttr(CE,"JobArrayIndex",NULL,tmpVal,sizeof(tmpVal)) == FAILURE)
      {
      J->Array->Members[jindex] = -1;

      return(FAILURE);
      }

    J->Array->Members[jindex] = strtol(tmpVal,NULL,10);

    if (MXMLGetAttr(CE,"Name",NULL,tmpVal,sizeof(tmpVal)) == SUCCESS)
      {
      MUStrDup(&J->Array->JName[jindex],tmpVal);
      }

    if (MXMLGetAttr(CE,"State",NULL,tmpVal,sizeof(tmpVal)) == SUCCESS)
      {
      J->Array->JState[jindex] = (enum MJobStateEnum)MUGetIndexCI(tmpVal,MJobState,FALSE,mjsNONE);
      }
   
    jindex++;
    }

  J->Array->Members[jindex] = -1;

  return(SUCCESS);
  }  /* END MJobArrayFromXML() */


/**
 * Create an XML element for the given job array with all the sub jobs as
 * attribute=value pairs of the type: <sub-job-id>=<state>
 *
 * @param MJ      (I) the array master job
 * @param JList   (I) the list of jobs we want to check for sub-jobs of the
 *                    array
 * @param SJE     (O) the xml element we want to populate
 */

int MJobTransitionArraySubjobsToXML(

    mtransjob_t  *MJ,     /* I */
    marray_t     *JList,  /* I */
    mxml_t      **SJE)    /* O */
  {
  int           jindex;
  mxml_t       *JE;
  mxml_t       *CE;
  mtransjob_t  *J;

  if ((MJ == NULL) || (JList == NULL))
    {
    return(FAILURE);
    }

  *SJE = NULL;

  if (MXMLCreateE(SJE,"ArraySubJobs") == FAILURE)
    {
    return(FAILURE);
    }

  JE = *SJE;

  for (jindex = 0; jindex < JList->NumItems; jindex++)
    {
    J = (mtransjob_t *)MUArrayListGetPtr(JList,jindex);

    if ((bmisset(&J->TFlags,mtransfArraySubJob)) &&
        (J->ArrayMasterID == MJ->ArrayMasterID))
      {
      CE = NULL;

      MXMLCreateE(&CE,(char *)MXO[mxoJob]);

      MXMLSetAttr(CE,(char *)MJobAttr[mjaJobID],(void *)J->Name,mdfString);
      MXMLSetAttr(CE,(char *)MJobAttr[mjaState],(void *)MJobState[J->State],mdfString);

      MXMLAddE(JE,CE);     
      }
    }

  return(SUCCESS);
  } /* END MJobTransitionArraySubjobsToXML() */




/**
 * Create a name for the given array master job transition that includes the
 * given number of jobs, so array jobs in showq are more readable, and set it
 * as the name of the xml element
 *
 * @param MJ        (I) the array master job transition
 * @param MJE       (O) the XML element for MJ
 * @param JobCount  (I) the number of sub-jobs to be added to the array
 *                      master name
 */

int MJobTransitionSetXMLArrayNameCount(

  mtransjob_t  *MJ,       /* I */
  mxml_t       *MJE,      /* O */
  int           JobCount) /* I */

  {
  char NameBuf[MMAX_LINE];

  if ((MJ == NULL) || (MJE == NULL))
    {
    return(FAILURE);
    }

  snprintf(NameBuf,MMAX_LINE,"%s(%d)",MJ->Name,JobCount);
  MXMLSetAttr(MJE,(char *)MJobAttr[mjaJobID],(void *)NameBuf,mdfString);

  return(SUCCESS);
  } /* END (MJobTransitionSetXMLArrayNameCount() */


