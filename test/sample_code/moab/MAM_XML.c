/* HEADER */

      
/**
 * @file MAM_XML.c
 *
 * Contains: MAM XML functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 * Convert an object to charge XML.
 *
 * @param O
 * @param OType
 * @param Op
 * @param EP
 */

int MAMOToXML(

  void                      *O,
  enum MXMLOTypeEnum         OType,
  enum MAMNativeFuncTypeEnum Op,
  mxml_t                   **EP)

  {
#define CHARGE_ATTR_VERSION 2

/*  NYI:
  double *GMetrics;

  GMetrics = (double *)MUCalloc(1,sizeof(double) * MMAX_GMETRICS);
*/
  if ((O == NULL) || (EP == NULL))
    {
    return(FAILURE);
    }

  *EP = NULL;

  switch (OType)
    {
    case mxoRsv:

      {
      mrsv_t *R = (mrsv_t *)O;

      MAMRsvToXML(R,Op,EP);
      }  /* END case mxoRsv */

      break;

    case mxoJob:

      {
      mjob_t *J = (mjob_t *)O; 

      MAMJobToXML(J,Op,EP);
      }  /* END case mxoJob */

      break;

    default:

      return(FAILURE);

      break;
    }  /* END switch (OType) */

  return(SUCCESS);
  }  /* END MAMOToXML() */


/**
 * Helper function for all of the object to charge XML functions.
 * Error checking should be done above this function
 *
 * @param E [O] (modified) - The XML element to add the created child element to
 * @param Name   [I] - The name to give to the created child element
 * @param Ptr    [I] - Value of the child
 * @param Format [I] - Format type of the value
 */

int __MAMOAddXMLChild(

  mxml_t *E,
  const char   *Name,
  void   *Ptr,
  enum MDataFormatEnum Format)

  {
  mxml_t *AE = NULL;

  MXMLCreateE(&AE,Name);
  MXMLSetVal(AE,Ptr,Format);

  MXMLAddE(E,AE);

  return(SUCCESS);
  }  /* END __MAMOAddXMLChild() */


/**
 * Helper function for object variables to charge XML functions
 * Error checking should be done above this function
 *
 * @param E [O] (modified) - The XML element to add child XML variable elements to
 * @param Variables [I] - The hash table of variables to convert
 */

int __MAMOAddXMLVariablesFromHT(

  mxml_t  *E,
  mhash_t *Variables)

  {
  mxml_t *AE = NULL;

  char *Name;
  char *Value;

  mhashiter_t HTI;

  MUHTIterInit(&HTI);

  while (MUHTIterate(Variables,&Name,(void **)&Value,NULL,&HTI) == SUCCESS)
    {
    AE = NULL;

    MXMLCreateE(&AE,"Var");

    MXMLSetAttr(AE,MSAN[msanName],Name,mdfString);

    if (Value != NULL)
      MXMLSetVal(AE,(void *)Value,mdfString);

    MXMLAddE(E,AE);
    }

  return(SUCCESS);
  }  /* END __MAMOAddXMLVariables() */





/**
 * Helper function for object variables to charge XML functions
 * Error checking should be done above this function
 *
 * @param E [O] (modified) - The XML element to add child XML variable elements to
 * @param Variables [I] - The linked list of variables to convert
 */

int __MAMOAddXMLVariables(

  mxml_t *E,
  mln_t  *Variables)

  {
  mxml_t *AE = NULL;
  mln_t *tmpLL = Variables;

  while (tmpLL != NULL)
    {
    AE = NULL;

    MXMLCreateE(&AE,"Var");

    MXMLSetAttr(AE,MSAN[msanName],tmpLL->Name,mdfString);

    if (tmpLL->Ptr != NULL)
      MXMLSetVal(AE,(void *)tmpLL->Ptr,mdfString);

    MXMLAddE(E,AE);

    tmpLL = tmpLL->Next;
    }

  return(SUCCESS);
  }  /* END __MAMOAddXMLVariables() */


/**
 * Helper function for object GRes to charge XML functions
 *
 * @param E [O] (modified) - the XML element to add child GRes XML to
 * @param GRes [I] - GRes to convert
 */

int __MAMOAddXMLGRes(

  mxml_t *E,
  msnl_t *GRes)

  {
  int gindex = 1;
  int tmpI = 0;
  mxml_t *AE = NULL;

  if (MSNLIsEmpty(GRes))
    return(SUCCESS);

  for (gindex = 1;gindex < MSched.M[mxoxGRes];gindex++)
    {
    if (MGRes.Name[gindex][0] == '\0')
      break;

    tmpI = MSNLGetIndexCount(GRes,gindex);

    if (tmpI <= 0)
      continue;

    AE = NULL;
    MXMLCreateE(&AE,"GRes");
    MXMLSetAttr(AE,MSAN[msanName],MGRes.Name[gindex],mdfString);
    MXMLSetVal(AE,(void *)&tmpI,mdfInt);
    MXMLAddE(E,AE);
    }

  return(SUCCESS);
  }  /* END __MAMOAddXMLGRres() */

/**
 * Convert reservation to charg XML.
 *
 * @see MAMOToXML - parent (sets *EP to NULL)
 *
 * @param R  [I] - The rsv to convert
 * @param Op [I] - The operation that is being done (charge, delete, etc.)
 * @param EP [O] - Output XML
 */

int MAMRsvToXML(

  mrsv_t  *R,
  enum MAMNativeFuncTypeEnum Op,
  mxml_t **EP)

  {
  mxml_t *E = NULL;
  int tmpI = 0;
  long tmpL = 0;

  if ((R == NULL) || (EP == NULL))
    {
    return(FAILURE);
    }

  MXMLCreateE(&E,"Reservation");
  __MAMOAddXMLChild(E,"ObjectID",(void *)R->Name,mdfString);

  /* USER */

  if (R->U != NULL)
    __MAMOAddXMLChild(E,"User",(void *)R->U->Name,mdfString);

  /* ACCOUNT */

  if (R->A != NULL)
    __MAMOAddXMLChild(E,"Account",(void *)R->A->Name,mdfString);

  /* GROUP */

  if (R->G != NULL)
    __MAMOAddXMLChild(E,"Group",(void *)R->G->Name,mdfString);

  /* CLASS */

  if (R->C != NULL)
    __MAMOAddXMLChild(E,"Class",(void *)R->C->Name,mdfString);

  /* QOS */

  if (R->Q != NULL) 
    __MAMOAddXMLChild(E,"QOS",(void *)R->Q->Name,mdfString);

  /* MACHINE */

    {
    mrm_t *RM = (R->PtIndex >= 0) ? MPar[R->PtIndex].RM : NULL;

    if ((RM != NULL) && (RM->Name[0] != '\0'))
      __MAMOAddXMLChild(E,"Machine",(void *)RM->Name,mdfString);
    else
      __MAMOAddXMLChild(E,"Machine",(void *)MSched.Name,mdfString);
    }

  /* NODES */

  if (R->AllocNC > 0)
    __MAMOAddXMLChild(E,"Nodes",(void *)&R->AllocNC,mdfInt);

  /* PROCS */

  /* ChargePolicy not yet implemented in Native Interface */
  tmpI = MAX(R->DRes.Procs,R->AllocPC) * R->AllocTC;

  if (tmpI > 0)
    __MAMOAddXMLChild(E,"Processors",(void *)&tmpI,mdfInt);

  /* MEMORY */

  tmpI = R->DRes.Mem * R->AllocTC;

  if (tmpI > 0)
    __MAMOAddXMLChild(E,"Memory",(void *)&tmpI,mdfInt);

  /* SWAP */

  tmpI = R->DRes.Swap * R->AllocTC;

  if (tmpI > 0)
    __MAMOAddXMLChild(E,"Swap",(void *)&tmpI,mdfInt);

  /* DISK */

  tmpI = R->DRes.Disk * R->AllocTC;

  if (tmpI > 0)
    __MAMOAddXMLChild(E,"Disk",(void *)&tmpI,mdfInt);

  /* DURATION */

  tmpL = R->EndTime - R->StartTime;

  if (tmpL > 0)
    __MAMOAddXMLChild(E,"WallDuration",(void *)&tmpL,mdfLong);

  /* CHARGE DURATION */
  /* START TIME */
  /* END TIME */

  tmpL = 0;

  switch (Op)
    {
    case mamnCreate:

      if (MAM[0].FlushPeriod != mpNONE)
        {
        if (R->StartTime > MSched.Time)
          {
          __MAMOAddXMLChild(E,"StartTime",(void *)&R->StartTime,mdfLong);
          
          tmpL = MIN(R->EndTime,((R->StartTime + MAM[0].FlushInterval) / MAM[0].FlushInterval) * MAM[0].FlushInterval) - R->StartTime;
          }
        else
          tmpL = MIN(R->EndTime,MAM[0].FlushTime) - R->StartTime;
        }
      else
        {
        tmpL = R->EndTime - R->StartTime;
        }

      __MAMOAddXMLChild(E,"LienDuration",(void *)&tmpL,mdfLong);

      break;

    case mamnPause:

      /* Always charge, even if R->CIPS is zero, so we get proper
       * updates to the usage record and to support usage tracking use case */
      tmpL = MSched.Time -
        ((R->LastChargeTime > 0) ? R->LastChargeTime : R->StartTime);

      __MAMOAddXMLChild(E,"ChargeDuration",(void *)&tmpL,mdfLong);

      if (R->CIPS > 0)
        {
        double PCRate;

        PCRate = (double)R->CIPS / (R->CAPS + R->CIPS);

        if (PCRate < 1.0)
          __MAMOAddXMLChild(E,"ConsumptionRate",(void *)&PCRate,mdfDouble);
        }

      break;

    case mamnResume:

      tmpL = MIN(R->EndTime,MAM[0].FlushTime) -
        ((R->LastChargeTime > 0) ? R->LastChargeTime : MSched.Time);

      __MAMOAddXMLChild(E,"LienDuration",(void *)&tmpL,mdfLong);

      break;

    case mamnStart:

      if (MAM[0].FlushPeriod != mpNONE)
        tmpL = MIN(R->EndTime,MAM[0].FlushTime) - R->StartTime;
      else
        tmpL = R->EndTime - R->StartTime;

      __MAMOAddXMLChild(E,"LienDuration",(void *)&tmpL,mdfLong);

      break;

    case mamnUpdate:

      /* Always charge, even if R->CIPS is zero, so we get proper
       * updates to the usage record and to support usage tracking use case */
      tmpL = MSched.Time -
        ((R->LastChargeTime > 0) ? R->LastChargeTime : R->StartTime);

      __MAMOAddXMLChild(E,"ChargeDuration",(void *)&tmpL,mdfLong);

      if (R->CIPS > 0)
        {
        double PCRate;

        PCRate = (double)R->CIPS / (R->CAPS + R->CIPS);

        if (PCRate < 1.0)
          __MAMOAddXMLChild(E,"ConsumptionRate",(void *)&PCRate,mdfDouble);
        }

      tmpL = MIN(R->EndTime,MAM[0].FlushTime) -
        ((R->LastChargeTime > 0) ? R->LastChargeTime : MSched.Time);

      __MAMOAddXMLChild(E,"LienDuration",(void *)&tmpL,mdfLong);

      if (R->StartTime > 0)
        __MAMOAddXMLChild(E,"StartTime",(void *)&R->StartTime,mdfLong);

      break;

    case mamnEnd:

      if (R->LastChargeTime > 0)
        tmpL = MSched.Time - R->LastChargeTime;
      else
        tmpL = MSched.Time - R->StartTime;

      __MAMOAddXMLChild(E,"ChargeDuration",(void *)&tmpL,mdfLong);

      if (R->StartTime > 0)
        __MAMOAddXMLChild(E,"StartTime",(void *)&R->StartTime,mdfLong);

      if (R->EndTime > 0)
        tmpL = MIN(R->EndTime,MSched.Time);
      else
        tmpL = MSched.Time;
      __MAMOAddXMLChild(E,"EndTime",(void *)&tmpL,mdfLong);

      break;

    default:

      break;
    } 

  /* VARIABLES */

  if (R->Variables != NULL)
    __MAMOAddXMLVariables(E,R->Variables);

  /* GRES */

  __MAMOAddXMLGRes(E,&R->DRes.GenericRes);

  *EP = E;

  return(SUCCESS);
  }  /* END MAMRsvToXML() */



/**
 * Convert job to charg XML.
 *
 * @see MAMOToXML - parent (sets *EP to NULL)
 *
 * @param J  [I] - The job to convert
 * @param Op [I] - The operation that is being done (charge, delete, etc.)
 * @param EP [O] - Output XML
 */

int MAMJobToXML(

  mjob_t  *J,
  enum MAMNativeFuncTypeEnum Op,
  mxml_t **EP)

  {
  mxml_t *E = NULL;
  int tmpI = 0;
  mulong tmpL = 0;
  int RQIndex;
  mxml_t *reqXML;
  enum MNodeAccessPolicyEnum NAPolicy;
  enum MAMConsumptionPolicyEnum ChargePolicy;

  if ((J == NULL) || (EP == NULL))
    {
    return(FAILURE);
    }

  if (MAM[0].Name[0] != '\0')
    ChargePolicy = MAM[0].ChargePolicy;
  else
    ChargePolicy = mamcpNONE;

  MXMLCreateE(&E,"Job");
  __MAMOAddXMLChild(E,"ObjectID",(void *)J->Name,mdfString);

  /* USER */

  if (J->Credential.U != NULL)
    __MAMOAddXMLChild(E,"User",(void *)J->Credential.U->Name,mdfString);

  /* ACCOUNT */

  if (J->Credential.A != NULL)
    __MAMOAddXMLChild(E,"Account",(void *)J->Credential.A->Name,mdfString);

  /* GROUP */

  if (J->Credential.G != NULL)
    __MAMOAddXMLChild(E,"Group",(void *)J->Credential.G->Name,mdfString);

  /* CLASS */

  if (J->Credential.C != NULL)
    __MAMOAddXMLChild(E,"Class",(void *)J->Credential.C->Name,mdfString);

  /* QOS */

  if (J->Credential.Q != NULL)
    __MAMOAddXMLChild(E,"QOS",(void *)J->Credential.Q->Name,mdfString);

  /* MACHINE */

  {
  mrm_t *RM = NULL;

  if (J->DestinationRM != NULL)
    {
    RM = J->DestinationRM;
    }
  else
    {
    mnode_t *N;

    if ((MNLGetNodeAtIndex(&J->NodeList,0,&N) == SUCCESS) && (N->PtIndex >= 0))
      RM = MPar[N->PtIndex].RM;
    }

  if (RM != NULL)
    __MAMOAddXMLChild(E,"Machine",(void *)RM->Name,mdfString);
  else
     __MAMOAddXMLChild(E,"Machine",(void *)MSched.Name,mdfString);
  }

  /* NODES */

  if (J->Request.NC > 0)
    {
    MJobGetEstNC(J,&tmpI,NULL,FALSE);

    if (tmpI > 0)
      __MAMOAddXMLChild(E,"NodeCount",(void *)&tmpI,mdfInt);
    }

  /* REQS */

  for (RQIndex = 0;RQIndex < MMAX_REQ_PER_JOB;RQIndex++)
    {
    if (J->Req[RQIndex] == NULL)
      break;

    reqXML = NULL;
    MXMLCreateE(&reqXML,"Req");

    /* PROCS */
    if (ChargePolicy == mamcpDebitAllBlocked)
      {
      MJobGetNAPolicy(J,NULL,NULL,NULL,&NAPolicy);

      if ((NAPolicy == mnacSingleTask) || (NAPolicy == mnacSingleJob))
        {
        mnode_t *N;
        int nindex;

        tmpI = 0;

        for (nindex = 0;MNLGetNodeAtIndex(&J->NodeList,nindex,&N) == SUCCESS;nindex++)
         {
         tmpI += N->CRes.Procs;
         } /* END for (nindex) */
        }
      else
        {
        tmpI = J->Req[RQIndex]->DRes.Procs * J->Req[RQIndex]->TaskCount;
        }
      }
    else
      {
      tmpI = J->Req[RQIndex]->DRes.Procs * J->Req[RQIndex]->TaskCount;
      }

    if (tmpI > 0)
      __MAMOAddXMLChild(reqXML,"Processors",(void *)&tmpI,mdfInt);

    /* MEMORY */

    tmpI = J->Req[RQIndex]->DRes.Mem * J->Req[RQIndex]->TaskCount;

    if (tmpI > 0)
      __MAMOAddXMLChild(reqXML,"Memory",(void *)&tmpI,mdfInt);

    /* SWAP */

    tmpI = J->Req[RQIndex]->DRes.Swap * J->Req[RQIndex]->TaskCount;

    if (tmpI > 0)
      __MAMOAddXMLChild(reqXML,"Swap",(void *)&tmpI,mdfInt);

    /* DISK */

    tmpI = J->Req[RQIndex]->DRes.Disk * J->Req[RQIndex]->TaskCount;

    if (tmpI > 0)
      __MAMOAddXMLChild(reqXML,"Disk",(void *)&tmpI,mdfInt);

    /* GRES */

    __MAMOAddXMLGRes(reqXML,&J->Req[RQIndex]->DRes.GenericRes);

    MXMLAddE(E,reqXML);
    }  /* END for (RQIndex = 0;RQIndex < MMAX_REQ;RQIndex++) */

  /* DURATION */

  tmpL = J->SpecWCLimit[0];

  if (tmpL > 0)
    __MAMOAddXMLChild(E,"WallDuration",(void *)&tmpL,mdfLong);

  /* CHARGE DURATION */
  /* START TIME */
  /* END TIME */

  tmpL = 0;

  switch (Op)
    {
    case mamnQuote:
    case mamnCreate:

      if (MAM[0].FlushPeriod != mpNONE)
        tmpL = MIN(J->SpecWCLimit[0],MAM[0].FlushInterval);
      else
        tmpL = J->SpecWCLimit[0];

      __MAMOAddXMLChild(E,"QuoteDuration",(void *)&tmpL,mdfLong);

      break;

    case mamnPause:

      if (J->LastChargeTime > 0)
        tmpL = MSched.Time - J->LastChargeTime;
      else
        tmpL = MSched.Time - J->StartTime;

      __MAMOAddXMLChild(E,"ChargeDuration",(void *)&tmpL,mdfLong);

      break;

    case mamnResume:

      tmpL = MIN(J->StartTime + J->WCLimit,MAM[0].FlushTime) - MSched.Time;

      __MAMOAddXMLChild(E,"LienDuration",(void *)&tmpL,mdfLong);

      break;

    case mamnStart:

      if (MAM[0].FlushPeriod != mpNONE)
        tmpL = MIN(J->WCLimit,MAM[0].FlushTime - MSched.Time);
      else
        tmpL = J->WCLimit;

      __MAMOAddXMLChild(E,"LienDuration",(void *)&tmpL,mdfLong);

      break;

    case mamnUpdate:

      if (J->LastChargeTime > 0)
        tmpL = MSched.Time - J->LastChargeTime;
      else
        tmpL = MSched.Time - J->StartTime;

      __MAMOAddXMLChild(E,"ChargeDuration",(void *)&tmpL,mdfLong);

      tmpL = MIN(J->StartTime + J->WCLimit,MAM[0].FlushTime) - MSched.Time;

      __MAMOAddXMLChild(E,"LienDuration",(void *)&tmpL,mdfLong);

      if (J->StartTime > 0)
        __MAMOAddXMLChild(E,"StartTime",(void *)&J->StartTime,mdfLong);

      break;

    case mamnEnd:
    case mamnModify:

      if (J->LastChargeTime > 0)
        tmpL = MSched.Time - J->LastChargeTime;
      else if (J->StartTime > 0)
        tmpL = MSched.Time - J->StartTime;
      else /* No start time - job didn't start - cancel, failed dep, etc. */
        tmpL = 0;

      __MAMOAddXMLChild(E,"ChargeDuration",(void *)&tmpL,mdfLong);

      if (J->StartTime > 0)
        __MAMOAddXMLChild(E,"StartTime",(void *)&J->StartTime,mdfLong);

      if (J->CompletionTime > 0)
        tmpL = MIN(J->CompletionTime,MSched.Time);
      else
        tmpL = MSched.Time;
      if (J->CompletionTime > 0)
        __MAMOAddXMLChild(E,"EndTime",(void *)&tmpL,mdfLong);

      __MAMOAddXMLChild(E,"CompletionCode",(void *)&J->CompletionCode,mdfInt);

      break;

    default:

      break;
    } 

  /* VARIABLES */

  if (J->Variables.Table != NULL)
    __MAMOAddXMLVariablesFromHT(E,&J->Variables);

  /* SYSTEM JOB INFO */

  if (J->System != NULL)
    {
    mxml_t *SysE = NULL;

    MXMLCreateE(&SysE,"System");

    __MAMOAddXMLChild(SysE,"Type",(void *)MSysJobType[J->System->JobType],mdfString);

    MXMLAddE(E,SysE);
    }

  /* TEMPLATES */

  if (J->TSets != NULL)
    {
    mln_t *tmpTSet;

    for (tmpTSet = J->TSets;tmpTSet != NULL;tmpTSet = tmpTSet->Next)
      {
      __MAMOAddXMLChild(E,"Template",(void *)tmpTSet->Name,mdfString);
      }
    }

  *EP = E;

  return(SUCCESS);
  }  /* END MAMJobToXML() */

/* END MAM-XML.c */
