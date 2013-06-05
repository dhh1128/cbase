/* HEADER */

      
/**
 * @file MQOSProcessConfig.c
 *
 * Contains: MQOSProcessConfig()
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 *  Parses and adds MQOSDEFAULTORDER to MSched
 *  
 *  FORMAT:  qos[,qos]...
 *  
 *  @param QOS       [I]  Input QOS order string 
 *  @param QosString [I]  Input QOS order string 
 */

int __MCfgParseQOSPreemptees(

  mqos_t *QOS,
  char *QosString)

  {
  int qindex = 0;
  char *ptr;
  char *TokPtr;


  MASSERT(QOS != NULL, "Unexpected NULL QOS parsing PREEMPTEES");
  MASSERT(QosString != NULL, "Unexpected NULL QosString parsing PREEMPTEES");

  /* FORMAT:  qos[,qos]... */

  /* Tokenize the string */
  ptr = MUStrTok(QosString,",",&TokPtr);
  while (ptr != NULL)
    {
    mqos_t *Q;

    /* Make sure this won't overflow the table*/
    if (qindex >= MMAX_QOS)
      {
      MDB(3,fCONFIG) MLog("ALERT:    Default QOS Order table overflow.\n");
      return(FAILURE);
      }

    if (MQOSFind(ptr,&Q) == SUCCESS)
      {
      QOS->Preemptees[qindex++] = Q;
      QOS->Preemptees[qindex] = NULL;
      }
    else if (MQOSAdd(ptr,&QOS->Preemptees[qindex]) == SUCCESS)
      {
      MDB(5,fCONFIG) MLog("INFO:     QOS '%s' added\n",ptr);
      qindex++;
      QOS->Preemptees[qindex] = NULL;
      }
    else
      {
      MDB(1,fCONFIG) MLog("ALERT:    cannot add QOS '%s'\n",ptr);
      return(FAILURE);
      }

    ptr = MUStrTok(NULL,",",&TokPtr);
    }
  return(SUCCESS);
  }



/**
 * Process QoS-specific config attribute
 *
 * @see MQOSSetAttr() - child
 * @see MCredProcessConfig() - peer - process general cred attributes
 *
 * @param Q     (I) [modified]
 * @param Value (I)
 * @param IsPar (I)
 */

int MQOSProcessConfig(

  mqos_t *Q,
  char   *Value,
  mbool_t IsPar)

  {
  mbitmap_t ValidBM;

  char *ptr;
  char *TokPtr;

  int   aindex;

  char  ValLine[MMAX_LINE];

  mbool_t AttrIsValid = MBNOTSET;

  const char *FName = "MQOSProcessConfig";

  MDB(5,fCONFIG) MLog("%s(%s,%s)\n",
    FName,
    (Q != NULL) ? Q->Name : "NULL",
    (Value != NULL) ? Value : "NULL");

  if ((Q == NULL) ||
      (Value == NULL) ||
      (Value[0] == '\0'))
    {
    return(FAILURE);
    }

  if (IsPar == TRUE)
    {
    MParSetupRestrictedCfgBMs(NULL,NULL,&ValidBM,NULL,NULL,NULL,NULL);
    }

  /* process value line */

  ptr = MUStrTokE(Value," \t\n",&TokPtr);

  while (ptr != NULL)
    {
    /* parse name-value pairs */

    if (MUGetPair(
          ptr,
          (const char **)MQOSAttr,
          &aindex,
          NULL,
          TRUE,
          NULL,
          ValLine,
          sizeof(ValLine)) == FAILURE)
      {
      mmb_t *M;

      char tmpLine[MMAX_LINE];

      /* cannot parse value pair */

      snprintf(tmpLine,sizeof(tmpLine),"unknown attribute '%.128s' specified",
        ptr);

      MDB(3,fSTRUCT) MLog("WARNING:  unknown attribute '%s' specified for QOS %s\n",
        ptr,
        Q->Name);

      if (MMBAdd(&Q->MB,tmpLine,NULL,mmbtNONE,0,0,&M) == SUCCESS)
        {
        M->Priority = -1;  /* annotation, do not checkpoint */
        }

      if (AttrIsValid == MBNOTSET)
        AttrIsValid = FALSE;

      ptr = MUStrTokE(NULL," \t\n",&TokPtr);

      continue;
      }

    if ((IsPar == TRUE) && (!bmisset(&ValidBM,aindex)))
      {
      ptr = MUStrTokE(NULL," \t\n",&TokPtr);

      continue;
      }

    AttrIsValid = TRUE;
   
    switch (aindex)
      {
      case mqaDynAttrs:

        bmfromstring(ValLine,MJobDynAttr,&Q->DynAttrs);

        break;

      case mqaPreemptPolicy:

        Q->PreemptPolicy = (enum MPreemptPolicyEnum)MUGetIndex(
          ValLine,
          (const char **)MPreemptPolicy,
          FALSE,
          0);

        break;

      case mqaSpecAttrs:

        Q->SpecAttrsSpecified = TRUE;

        bmfromstring(ValLine,MJobDynAttr,&Q->SpecAttrs);

        break;

      case mqaPreemptMaxTime:
      case mqaPreemptMinTime:
      case mqaDedResCost:
      case mqaUtlResCost:
      case mqaFSScalingFactor:

      case mqaBLACLThreshold:
      case mqaBLPowerThreshold:
      case mqaBLPreemptThreshold:
      case mqaBLRsvThreshold:
      case mqaBLTriggerThreshold:

      case mqaQTACLThreshold:
      case mqaQTPowerThreshold:
      case mqaQTPreemptThreshold:
      case mqaQTRsvThreshold:
      case mqaQTTriggerThreshold:

      case mqaXFACLThreshold:
      case mqaXFPowerThreshold:
      case mqaXFPreemptThreshold:
      case mqaXFRsvThreshold:
      case mqaXFTriggerThreshold:
      case mqaExemptLimit:
      case mqaSystemPrio:

      case mqaGreenBacklogThreshold:
      case mqaGreenQTThreshold:
      case mqaGreenXFactorThreshold:

        MQOSSetAttr(
          Q,
          (enum MQOSAttrEnum)aindex,
          (void **)ValLine,
          mdfString,
          0);

        break;

      case mqaQTTarget:

        Q->QTTarget = MUTimeFromString(ValLine);

        break;

      case mqaQTWeight:

        Q->QTSWeight = strtol(ValLine,NULL,10);

        break;

      case mqaRsvAccessList:

        /* FORMAT:  <RSVID[,<RSVID>]... */

        {
        char *ptr;
        char *TokPtr;

        int   rindex;

        ptr = MUStrTok(ValLine,",",&TokPtr);

        rindex = 0;

        while (ptr != NULL)
          {
          MUStrDup(&Q->RsvAccessList[rindex],ptr);

          rindex++;

          if (rindex >= MMAX_QOSRSV)
            break;

          ptr = MUStrTok(NULL,",",&TokPtr);
          }  /* END while (ptr != NULL) */
        }    /* END case mqaRsvAccessList */

        break;

      case mqaRsvCreationCost:

        Q->RsvCreationCost = strtod(ValLine,NULL);

        break;

      case mqaRsvPreemptList:

        /* FORMAT:  <RSVID[,<RSVID>]... */

        {
        char *ptr;
        char *TokPtr;

        int   rindex;

        ptr = MUStrTok(ValLine,",",&TokPtr);

        rindex = 0;

        while (ptr != NULL)
          {
          MUStrDup(&Q->RsvPreemptList[rindex],ptr);

          rindex++;

          if (rindex >= MMAX_QOSRSV)
            break;

          ptr = MUStrTok(NULL,",",&TokPtr);
          }  /* END while (ptr != NULL) */
        }    /* END case mqaRsvAccessList */

        break;

      case mqaRsvRlsMinTime:

        Q->RsvRlsMinTime = strtol(ValLine,NULL,10);

        break;

      case mqaRsvTimeDepth:
  
        Q->RsvTimeDepth = MUTimeFromString(ValLine);

        break;

      case mqaXFTarget:

        Q->XFTarget = strtod(ValLine,NULL);       

        break;

      case mqaXFWeight:

        Q->XFSWeight = strtol(ValLine,NULL,0);

        break;

      case mqaFSTarget:

        Q->F.FSTarget = strtod(ValLine,NULL);
 
        break;

      case mqaFlags:

        /* FORMAT:  FLAGS=A:B:C:D */

        MQOSFlagsFromString(Q,ValLine);

        break;

      case mqaOnDemand:

        MQOSSetAttr(Q,(enum MQOSAttrEnum)aindex,(void **)ValLine,mdfString,0);

        break;

      case mqaJobPrioAccrualPolicy:

        MCfgParsePrioAccrualPolicy(&Q->JobPrioAccrualPolicy,&Q->JobPrioExceptions,ValLine);

        break;

      case mqaJobPrioExceptions:

        MCfgParsePrioExceptions(&Q->JobPrioExceptions,ValLine);
  
        break;

      case mqaPreemptees:

        __MCfgParseQOSPreemptees(Q, ValLine);

        break;
        
      default:

        /* not handled */

        return(FAILURE);

        /*NOTREACHED*/

        break;
      }  /* END switch (AIndex) */

    ptr = MUStrTokE(NULL," \t\n",&TokPtr);       
    }  /* END while (ptr != NULL) */

  bmclear(&ValidBM);

  if (AttrIsValid == FALSE)
    {
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MQOSProcessConfig() */
/* END MQOSProcessConfig.c */
