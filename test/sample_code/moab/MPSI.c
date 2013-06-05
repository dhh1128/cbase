/* HEADER */

      
/**
 * @file MPSI.c
 *
 * Contains: PSI (peer service interface) functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 *
 *
 * @param P (I) [modified]
 */

int MPSIClearStats(

  mpsi_t *P)  /* I (modified) */

  {
  int findex;

  if (P == NULL)
    {
    return(FAILURE);
    }

  /* clear failure tracking */

  memset(P->FailTime,0,sizeof(P->FailTime));
  memset(P->FailType,0,sizeof(P->FailType));

  for (findex = 0;findex < MMAX_PSFAILURE;findex++)
    {
    MUFree(&P->FailMsg[findex]);
    }

  P->FailIndex = 0;

  /* clear performance data */

  memset(P->RespTotalTime,0,sizeof(P->RespTotalTime));
  memset(P->RespMaxTime,0,sizeof(P->RespMaxTime));
  memset(P->RespTotalCount,0,sizeof(P->RespTotalCount));
  memset(P->RespFailCount,0,sizeof(P->RespFailCount));
  memset(P->RespStartTime,0,sizeof(P->RespStartTime));
 
  return(SUCCESS);
  }  /* END MPSIClearStats() */




/**
 * Convert 'peer service interface' to XML.
 *
 * @see MRMToXML() - parent
 *
 * @param P (I)
 * @param PE (I) [modified]
 */

int MPSIToXML(

  mpsi_t *P,   /* I */
  mxml_t *PE)  /* I (modified) */

  {
  int findex;
  int index;

  double tmpD;

  char   *ptr;

  mxml_t *FE;

  if ((P == NULL) || (PE == NULL))
    {
    return(FAILURE);
    }

  /* specify attributes */

  if (P->RespTotalCount[0] > 0)
    tmpD = (double)P->RespTotalTime[0] / P->RespTotalCount[0] / 1000;
  else
    tmpD = 0.0;

  MXMLSetAttr(
    PE,
    (char *)MPSIAttr[mpsiaAvgResponseTime],
    (void *)&tmpD,
    mdfDouble);

  tmpD = (double)P->RespMaxTime[0] / 1000;

  MXMLSetAttr(
    PE,
    (char *)MPSIAttr[mpsiaMaxResponseTime],
    (void *)&tmpD,
    mdfDouble);

  MXMLSetAttr(
    PE,
    (char *)MPSIAttr[mpsiaTotalRequests],
    (void *)&P->RespTotalCount[0],
    mdfInt);

  MXMLSetAttr(
    PE,
    (char *)MPSIAttr[mpsiaRespFailCount],
    (void *)&P->RespFailCount[0],
    mdfInt);

  /* specify function children */

  for (index = 0;index < MMAX_PSFAILURE;index++)
    {
    findex = (index + P->FailIndex) % MMAX_PSFAILURE;

    if (P->FailTime[findex] <= 0)
      continue;

    FE = NULL;

    MXMLCreateE(&FE,"failure");

    MXMLSetAttr(
      FE,
      (char *)MPSFAttr[mpsfaFailTime],
      (void *)&P->FailTime[findex],
      mdfLong);

    if (P->FailType[findex] > 0)
      {
      ptr = (P->Type == mpstAM) ? 
        (char *)MAMFuncType[P->FailType[findex]] : 
        (char *)MRMFuncType[P->FailType[findex]];

      MXMLSetAttr(
        FE,
        (char *)MPSFAttr[mpsfaFailType],
        (void *)ptr,
        mdfString);
      }

    if (P->FailMsg[findex] != NULL)
      {
      MXMLSetAttr(
        FE,
        (char *)MPSFAttr[mpsfaFailMsg],
        (void *)P->FailMsg[findex],
        mdfString);
      }

    MXMLSetAttr(
      FE,
      (char *)MPSFAttr[mpsfaFailCount],
      (void *)&P->RespFailCount[P->FailType[findex]],
      mdfInt);

    MXMLSetAttr(
      FE,
      (char *)MPSFAttr[mpsfaTotalCount],
      (void *)&P->RespTotalCount[P->FailType[findex]],
      mdfInt);

    MXMLAddE(PE,FE);
    }  /* END for (index) */

  return(SUCCESS);
  }  /* END MPSIToXML() */




/**
 * create/update 'peer service interface' from XML
 *
 * @param P
 * @param E
 */

int MPSIFromXML(

  mpsi_t *P,
  mxml_t *E)

  {
  int CTok;
  int findex;
  int index;

  char tmpLine[MMAX_LINE];

  mxml_t *CE;

  const char *FName = "MPSIFromXML";

  MDB(2,fSTRUCT) MLog("%s(%s,%s)\n",
    FName,
    (P != NULL) ? "P" : "NULL",
    (E != NULL) ? "E" : "NULL");

  if ((P == NULL) || (E == NULL))
    {
    return(FAILURE);
    }

  MXMLGetAttrF(E,(char *)MPSIAttr[mpsiaTotalRequests],NULL,(void *)&P->RespTotalCount[0],mdfInt,0);

  MXMLGetAttrF(E,(char *)MPSIAttr[mpsiaRespFailCount],NULL,(void *)&P->RespFailCount[0],mdfInt,0);

  CTok = -1;

  P->FailIndex = 0;

  index = 0;

  while (MXMLGetChild(E,"failure",&CTok,&CE) == SUCCESS)
    {
    /* locate function children */

    findex = (index + P->FailIndex) % MMAX_PSFAILURE;

    MXMLGetAttrF(CE,(char *)MPSFAttr[mpsfaFailTime],NULL,(void *)&P->FailTime[findex],mdfLong,0);

    if (MXMLGetAttr(CE,(char *)MPSFAttr[mpsfaFailType],NULL,tmpLine,sizeof(tmpLine)) == SUCCESS)
      {
      P->FailType[findex] = MUGetIndex(tmpLine,MRMFuncType,FALSE,0);
      }

    if (MXMLGetAttr(CE,(char *)MPSFAttr[mpsfaFailMsg],NULL,tmpLine,sizeof(tmpLine)) == SUCCESS)
      {
      MUStrDup(&P->FailMsg[findex],tmpLine);
      }

    MXMLGetAttrF(CE,(char *)MPSFAttr[mpsfaFailCount],NULL,(void *)&P->RespFailCount[P->FailType[findex]],mdfInt,0);

    MXMLGetAttrF(CE,(char *)MPSFAttr[mpsfaTotalCount],NULL,(void *)&P->RespTotalCount[P->FailType[findex]],mdfInt,0);

    index++;
    }  /* END while (MXMLGetChild(E,"failure",&CTok,&CE) == SUCCESS) */

  return(SUCCESS);
  }  /* END MPSIFromXML() */
/* END MPSI.c */
