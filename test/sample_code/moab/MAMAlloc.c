/* HEADER */

      
/**
 * @file MAMAlloc.c
 *
 * Contains: Various alloc routines for various AM objects
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  
#include "mcom.h"



#define MAM_KITTYSTRING "KITTY"





/**
 * Load account details via accounting manager.
 *
 * @param AM (I)
 * @param OIndex (I)
 * @param OPtr (I)
 * @param R (I) [optional]
 * @param AName (O)
 */

int MAMGetAccount(

  mam_t               *AM,      /* I */
  enum MXMLOTypeEnum   OIndex,  /* I */
  void                *OPtr,    /* I */
  mrm_t               *R,       /* I (optional) */
  char                *AName)   /* O */

  {
  mgcred_t *APtr = NULL;

  char     *BPtr;
  int       BSpace;

  if (AName[0] != '\0')
    AName[0] = '\0';

  if ((AM == NULL) || (AName == NULL))
    {
    return(FAILURE);
    }

  /* determine account */

  switch (OIndex)
    {
    case mxoJob:

      {
      mjob_t *J;

      J = (mjob_t *)OPtr;

      if (((J->Credential.A != NULL) && strcmp(J->Credential.A->Name,NONE)))
        {
        APtr = J->Credential.A;
        }
      }  /* END BLOCK */

      break;

    default:

      /* NO-OP */

      break;
    }  /* END switch (OIndex) */

  if ((APtr == NULL) && (MSched.DefaultAccountName[0] != '\0'))
    {
    MAcctFind(MSched.DefaultAccountName,&APtr);
    }

  if (APtr == NULL)
    {
    switch (AM->Type)
      {
      case mamtNative:
      case mamtMAM:
      case mamtGOLD:
      case mamtFILE:

        /* NO-OP */

        break;

      default:

        /* account required */

        return(FAILURE);

        /*NOTREACHED*/

        break;
      }  /* END switch (AM->Type) */
    }    /* END if (APtr == NULL) */

  MUSNInit(&BPtr,&BSpace,AName,MMAX_NAME);

  MUSNPrintF(&BPtr,&BSpace,"%s",
    APtr->Name);

  return(SUCCESS);
  }  /* END MAMGetAccount() */


/**
 * Perform job debit via accounting manager interface.
 * 
 * @see MAMAllocJReserve() - peer - create allocation reservation
 * @see MAMAllocRDebit() - peer - debit reservation
 *
 * @param AM (I)
 * @param J (I)
 * @param S3C (O) [optional] - SSS Status Code Decade
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 */

int MAMAllocJDebit(

  mam_t                  *AM,    /* I */
  mjob_t                 *J,     /* I */
  enum MS3CodeDecadeEnum *S3C,   /* O (optional) */
  char                   *EMsg)  /* O (optional,minsize=MMAX_LINE) */

  {
  char NodeType[MMAX_NAME];
 
  char AccountName[MMAX_NAME];
  char QOSString[MMAX_NAME];
  char GMLine[MMAX_LINE];

  mnode_t *N;
 
  double ProcConsumptionRate;

  int    ProcCount = 0;
  int    NodeCount = 0;
  int    DedicatedMemory = 0; 
  int    UtilizedMemory = 0; 
  int    rc;

  long   Duration = 0;

  mbool_t ChargeByDedicatedNode;

  mrm_t *RM;

  mreq_t *RQ;

  const char *FName = "MAMAllocJDebit";

  MDB(3,fAM) MLog("%s(AM,%s,%p,%p)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    S3C,EMsg);

  if (S3C != NULL)
    *S3C = ms3cNone;

  if (EMsg != NULL)
    EMsg[0] = '\0';

#ifndef __MOPT
  if ((AM == NULL) || (J == NULL))
    {
    MDB(6,fAM) MLog("ERROR:    Invalid job/AM specified in %s\n",
      FName);

    return(FAILURE);
    }
#endif /* !__MOPT */
 
  if ((AM->Type == mamtNONE) || (AM->Type == mamtNative) ||
     ((MSched.Mode != msmNormal) && (MAMTestMode != TRUE)))
    {
    MDB(8,fAM) MLog("ALERT:    AM requests ignored in %s\n",
      FName);

    bmunset(&J->IFlags,mjifAMReserved);

    return(SUCCESS);
    }

  if ((J->Credential.C != NULL) && (J->Credential.C->DisableAM == TRUE))
    {
    bmunset(&J->IFlags,mjifAMReserved);

    return(SUCCESS);
    }

  if (AM->State == mrmsDown)
    {
    MDB(6,fAM) MLog("WARNING:  AM is down in %s\n",
      FName);

    if (EMsg != NULL)
      strcpy(EMsg,"AM is down");

    return(FAILURE);
    }

  RM = J->DestinationRM;
  if (RM == NULL)
    {
    mnode_t *N;

    if ((MNLGetNodeAtIndex(&J->NodeList,0,&N) == SUCCESS) && (N->PtIndex >= 0))
      RM = MPar[N->PtIndex].RM;

    /* NOTE:  This still doesn't guarentee a valid pointer.  Must still check for NULL.  */
    }

  if (((J->Credential.A == NULL) || (!strcmp(J->Credential.A->Name,NONE))) &&
       (MSched.DefaultAccountName[0] == '\0') &&
       (AM->Type != mamtMAM) &&
       (AM->Type != mamtGOLD) &&
       (AM->Type != mamtFILE))
    {
    MDB(2,fAM) MLog("WARNING:  No account specified for job %s\n",
      J->Name);
 
    return(FAILURE);
    }
 
  if ((J->Credential.A != NULL) && strcmp(J->Credential.A->Name,NONE))
    {
    strcpy(AccountName,J->Credential.A->Name);
    }
  else
    {
    strcpy(AccountName,MSched.DefaultAccountName);
    }

  RQ = J->Req[0];

  /* determine how much memory was dedicated for this job, and how much it
   * utilized */

  /* NOTE: This is not multi-req friendly... */

  DedicatedMemory = RQ->DRes.Mem;
  
  UtilizedMemory = (RQ->URes == NULL) ? 0 : RQ->URes->Mem;

  /* Job has completed */
  if ((J->StartTime > 0) && (J->CompletionTime > J->StartTime))
    {
    if (J->LastChargeTime > 0)
      Duration = (long)J->CompletionTime - J->LastChargeTime;
    else
      Duration = (long)J->CompletionTime - J->StartTime;
    }
  /* Job has not completed */
  else
    {
    if (J->LastChargeTime > 0)
      Duration = MSched.Time - J->LastChargeTime;
    else if (J->StartTime > 0)
      Duration = MSched.Time - J->StartTime;
    else
      Duration = 0;
    }

  MDB(7,fSCHED) MLog("INFO:     Duration for %s set to %d\n",
    J->Name,
    Duration);

  /* NOTE:  support QOS dedicated (NYI) */

  ChargeByDedicatedNode = FALSE;

  if ((J->Credential.Q != NULL) && 
      (bmisset(&J->Credential.Q->Flags,mqfDedicated)) &&
      (!MNLIsEmpty(&J->NodeList)))
    {
    /* QoS 'dedicated' flag set */

    ChargeByDedicatedNode = TRUE;
    }
  else if ((AM->ChargePolicy == mamcpDebitAllBlocked) ||
           (AM->ChargePolicy == mamcpDebitSuccessfulBlocked))
    {
    enum MNodeAccessPolicyEnum NAPolicy;

    MJobGetNAPolicy(J,NULL,NULL,NULL,&NAPolicy);

    if ((NAPolicy == mnacSingleTask) || (NAPolicy == mnacSingleJob))  
      {
      /* node dedicated by job or global node access policy */

      ChargeByDedicatedNode = TRUE;
      }
    }

  if (ChargeByDedicatedNode == TRUE)
    {
    mnode_t *N;

    int nindex;

    ProcCount = 0;

    for (nindex = 0;MNLGetNodeAtIndex(&J->NodeList,nindex,&N) == SUCCESS;nindex++)
      {
      ProcCount += N->CRes.Procs;
      }  /* END for (nindex) */
    }
  else
    { 
    ProcCount = MAX(1,J->TotalProcCount);
    } 

  MJobGetEstNC(J,&NodeCount,NULL,FALSE);

  ProcConsumptionRate = 1.0;
 
  switch (AM->ChargePolicy)
    {
    case mamcpDebitAllPE:
    case mamcpDebitSuccessfulPE:

      /* NO-OP */
 
      break;
 
    case mamcpDebitAllCPU:
    case mamcpDebitSuccessfulCPU:
 
      if ((J->PSUtilized > 0.01))
        {
        if ((J->Req[0]->LURes != NULL) && (J->Req[0]->LURes->Procs > 0))
          ProcConsumptionRate = (double)J->Req[0]->LURes->Procs / 100.0 / ((Duration > 0) ? Duration : 1) / ProcCount;
        else
          ProcConsumptionRate = (double)J->PSUtilized / ((Duration > 0) ? Duration : 1) / ProcCount;
        }
      else
        {
        ProcConsumptionRate = 1.0;
        }
 
      break;

    case mamcpDebitAllBlocked:
    case mamcpDebitSuccessfulBlocked:
    case mamcpDebitSuccessfulNode:

      /* indicate that job charged for all blocked resources */

      ProcConsumptionRate = -1.0;

      break;
  
    case mamcpDebitAllWC:
    case mamcpDebitSuccessfulWC:
    default: 

      ProcConsumptionRate = 1.0;
 
      break;
    }  /* END switch (MSched.AMChargePolicy) */

  if (MUNLGetMinAVal(&J->NodeList,mnaNodeType,NULL,(void **)NodeType) == FAILURE)
    {
    strcpy(NodeType,MDEF_NODETYPE);
    
    if (MNLGetNodeAtIndex(&J->Req[0]->NodeList,0,&N) == SUCCESS)
      {
      if ((N->RackIndex != -1) && (MRack[N->RackIndex].NodeType[0] != '\0'))
        strcpy(NodeType,MRack[N->RackIndex].NodeType);
      else if (N->NodeType != NULL)
        strcpy(NodeType,N->NodeType);
      }
    }

  /* generate generic metric info */

  if ((RQ->XURes != NULL) &&
      (RQ->XURes->GMetric != NULL))
    {
    char *BPtr;
    int   BSpace;

    int gmindex;

    MUSNInit(&BPtr,&BSpace,GMLine,sizeof(GMLine));

    for (gmindex = 1;gmindex < MSched.M[mxoxGMetric];gmindex++)
      {
      double GMFactor;

      if (MGMetric.Name[gmindex][0] == '\0')
        break;

      if (RQ->XURes->GMetric[gmindex] == NULL)
        continue;

      if (RQ->XURes->GMetric[gmindex]->Avg <= 0.0)
        continue;

      GMFactor = RQ->XURes->GMetric[gmindex]->Avg;

      if (GMFactor <= 0.0)
        continue;

      GMFactor *= J->Alloc.NC;

      MUSNPrintF(&BPtr,&BSpace," %s=%f",
        MGMetric.Name[gmindex],
        GMFactor);
      }  /* END for (gmindex) */
    }    /* END if (J->XURes != NULL) */
  else
    {
    GMLine[0] = '\0';
    }
 
  /* determine QOS */
 
  if ((J->Credential.Q != NULL) && strcmp(J->Credential.Q->Name,DEFAULT))
    {
    sprintf(QOSString," QOS=%s",
      J->Credential.Q->Name);
    }
  else
    {
    QOSString[0] = '\0';
    }
 
  switch (AM->Type)
    {
    case mamtNative:

      rc = MAMNativeDoCommand(AM,J,mxoJob,mamnEnd,NULL,NULL,NULL);

      if (rc == SUCCESS)
        J->LastChargeTime = MSched.Time;

      break;

    case mamtFILE:

      {
      double JCost = 0.0;

      MJobGetCost(J,FALSE,&JCost);

      fprintf(AM->FP,"%-15s TIME=%ld TYPE=job MACHINE=%s ACCOUNT=%s USER=%s CLASS=%s PROCS=%d PROCCRATE=%.2f RESOURCETYPE=%s DURATION=%ld%s%s REQUESTID=%s CHARGE=%f\n",
        "WITHDRAWAL",
        MSched.Time,
        (RM != NULL) ? RM->Name : MSched.Name,
        AccountName,
        J->Credential.U->Name,
        (J->Credential.C != NULL) ? J->Credential.C->Name : DEFAULT,
        ProcCount,
        ProcConsumptionRate,
        NodeType,
        Duration,
        QOSString,
        GMLine,
        J->Name,
        JCost);
 
      fflush(AM->FP);

      J->LastChargeTime = MSched.Time;
      }  /* END BLOCK (case mamtFILE) */
 
      break;

    case mamtMAM:
    case mamtGOLD:

      {
      int     rqindex;

      enum MS3CodeDecadeEnum  tmpS3C;

      mreq_t *RQ;

      mxml_t *URE = NULL;

      char *RspBuf = NULL;

      mxml_t *RE;

      mxml_t *DE;
      mxml_t *AE;
      mxml_t *OE = NULL;

      /* Request */
      RE = NULL;
      MXMLCreateE(&RE,(char*)MSON[msonRequest]);
      MXMLSetAttr(RE,(char *)MSAN[msanAction],(void *)"Charge",mdfString);
      MS3SetObject(RE,"UsageRecord",NULL);

      /* Data */
      DE = NULL;
      MXMLCreateE(&DE,(char*)MSON[msonData]);
      MXMLAddE(RE,DE);

      /* Usage Record */
      URE = NULL;
      MXMLCreateE(&URE,"UsageRecord");
      MXMLAddE(DE,URE);

      /* Type */
      AE = NULL;
      MXMLCreateE(&AE,"Type");
      MXMLSetVal(AE,(void *)"Job",mdfString);
      MXMLAddE(URE,AE);

      /* Instance */
      AE = NULL;
      MXMLCreateE(&AE,"Instance");
      MXMLSetVal(AE,(void *)J->Name,mdfString);
      MXMLAddE(URE,AE);

      /* User */
      if (J->Credential.U != NULL)
        {
        AE = NULL;
        MXMLCreateE(&AE,"User");
        MXMLSetVal(AE,(void *)J->Credential.U->Name,mdfString);
        MXMLAddE(URE,AE);
        }

      /* Group */
      if (J->Credential.G != NULL)
        {
        AE = NULL;
        MXMLCreateE(&AE,"Group");
        MXMLSetVal(AE,(void *)J->Credential.G->Name,mdfString);
        MXMLAddE(URE,AE);
        }

      /* Account */
      if (J->Credential.A != NULL)
        {
        AE = NULL;
        MXMLCreateE(&AE,"Account");
        MXMLSetVal(AE,(void *)J->Credential.A->Name,mdfString);
        MXMLAddE(URE,AE);
        }

      /* Class */
      if (J->Credential.C != NULL)
        {
        AE = NULL;
        MXMLCreateE(&AE,"Class");
        MXMLSetVal(AE,(void *)&J->Credential.C->Name,mdfString);
        MXMLAddE(URE,AE);
        }

      /* QOS */
      if (J->Credential.Q != NULL)
        {
        AE = NULL;
        MXMLCreateE(&AE,(char *)MS3JobAttr[ms3jaQOSReq]);
        MXMLSetVal(AE,(void *)J->Credential.Q->Name,mdfString);
        MXMLAddE(URE,AE);
        }

      /* Machine */
      AE = NULL;
      MXMLCreateE(&AE,"Machine");
      MXMLSetVal(AE,(RM != NULL) ? (void *)RM->Name : (void*)MSched.Name,mdfString);
      MXMLAddE(URE,AE);

      /* Nodes */
      if (NodeCount > 0)
        {
        AE = NULL;
        MXMLCreateE(&AE,(char *)MS3JobAttr[ms3jaAllocNodeList]);
        MXMLSetVal(AE,(void *)&NodeCount,mdfInt);
        MXMLAddE(URE,AE);
        }

      /* Processors */
      if (ProcCount > 0)
        {
        AE = NULL;
        MXMLCreateE(&AE,(char *)MS3ReqAttr[ms3rqaTCReqMin]);
        MXMLSetVal(AE,(void *)&ProcCount,mdfInt);
        MXMLAddE(URE,AE);
        }

      /* Memory */

      /* SSS specification is <Memory context="Dedicated">1234</Memory> */
      if (DedicatedMemory > 0)
        {
        AE = NULL;
        MXMLCreateE(&AE,"Memory");
        MXMLSetAttr(AE,(char *)"context",(void *)"Dedicated",mdfString);
        MXMLSetVal(AE,(void *)&DedicatedMemory,mdfInt);
        MXMLAddE(URE,AE);
        }

      /* SSS specification is <Memory context="Utilized">1234</Memory> */
      if (UtilizedMemory > 0)
        {
        AE = NULL;
        MXMLCreateE(&AE,"Memory");
        MXMLSetAttr(AE,(char *)"context",(void *)"Utilized",mdfString);
        MXMLSetVal(AE,(void *)&UtilizedMemory,mdfInt);
        MXMLAddE(URE,AE);
        }

      /* Duration */
      if (Duration >= 0)
        {
        OE = NULL;
        MXMLCreateE(&OE,(char*)MSON[msonOption]);
        MXMLSetAttr(OE,MSAN[msanName],(void *)"Duration",mdfString);
        MXMLSetVal(OE,(void *)&Duration,mdfLong);
        MXMLAddE(RE,OE);
        }

      /* Start Time */
      if (J->StartTime > 0)
        {
        AE = NULL;
        MXMLCreateE(&AE,(char *)MS3JobAttr[ms3jaStartTime]);
        MXMLSetVal(AE,&J->StartTime,mdfLong);
        MXMLAddE(URE,AE);
        }

      /* End Time */
      if (J->CompletionTime > 0)
        {
        AE = NULL;
        MXMLCreateE(&AE,"EndTime");
        MXMLSetVal(AE,&J->CompletionTime,mdfLong);
        MXMLAddE(URE,AE);
        }
      else
        {
        MS3AddChild(RE,(char*)MSON[msonOption],"Incremental","True",NULL);
        }

      if (NodeType[0] != '\0')
        {
        AE = NULL;
        MXMLCreateE(&AE,"NodeType");
        MXMLSetVal(AE,(void *)NodeType,mdfString);
        MXMLAddE(URE,AE);
        }

      /* Generic Resources */
      for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
        {
        RQ = J->Req[rqindex];

        if (!MSNLIsEmpty(&RQ->DRes.GenericRes))
          {
          int gindex;
          double TaskSeconds;

          for (gindex = 1;gindex < MSched.M[mxoxGRes];gindex++)
            {
            if (MGRes.Name[gindex][0] == '\0')
              break;

            if (MSNLGetIndexCount(&RQ->DRes.GenericRes,gindex) == 0) 
              continue;

            /* NOTE:    we only need to send the number of GRES tokens to
                        Gold, not the duration of the GRES tokens */

            TaskSeconds = MSNLGetIndexCount(&RQ->DRes.GenericRes,gindex) * RQ->TaskCount;

            AE = NULL;
            MXMLCreateE(&AE,MGRes.Name[gindex]);
            MXMLSetVal(AE,(void *)&TaskSeconds,mdfDouble);
            MXMLAddE(URE,AE);
            }  /* END for (gindex) */
          }    /* END if (RQ->DRes.GRes[0].count > 0) */
        }      /* END for (rqindex) */

      /* report generic metrics */

      /* NOTE:  gres reporting format above should be modified to sync with 
                gmetric format below */

      if (MGMetric.Name[1][0] != '\0')
        {
        mnode_t *N;

        int gmindex;
        char tmpVal[MMAX_LINE];

        /* one or more generic metrics defined w/in system */

        if (MNLGetNodeAtIndex(&J->NodeList,0,&N) == SUCCESS)
          {
          /* NOTE:  only report generic metrics for primary task */

          for (gmindex = 1;gmindex < MSched.M[mxoxGMetric];gmindex++)
            {
            if (MGMetric.Name[gmindex][0] == '\0')
              break;

            if ((N->XLoad == NULL) ||
                (N->XLoad->GMetric[gmindex] == NULL))
              continue;

            snprintf(tmpVal,sizeof(tmpVal),"%s=%.2f",
              MGMetric.Name[gmindex],
              N->XLoad->GMetric[gmindex]->IntervalLoad);

            AE = NULL;
            MXMLCreateE(&AE,"gmetric");
            MXMLSetVal(AE,(void *)tmpVal,mdfString);
            MXMLAddE(URE,AE);
            }    /* END for (gmindex) */
          }      /* END if ((J->NodeList != NULL) && ...) */ 
        }        /* END if (MAList[meGMetric][1][0] != '\0') */

      if (bmisset(&AM->Flags,mamfLocalCost))
        {
        double Cost;

        /* add charge option */

        OE = NULL;

        MXMLCreateE(&OE,(char*)MSON[msonOption]);

        MXMLSetAttr(OE,MSAN[msanName],(void *)"Charge",mdfString);

        MJobGetCost(J,FALSE,&Cost);

        MXMLSetVal(OE,(void *)&Cost,mdfDouble);

        MXMLAddE(RE,OE);
        }

      /* submit request */

      if (MAMGoldDoCommand(RE,&AM->P,NULL,&tmpS3C,EMsg) == FAILURE)
        {
        MDB(2,fAM) MLog("WARNING:  Unable to charge funds for job\n");

        if (S3C != NULL)
          *S3C = tmpS3C;

        if ((AM->StartFailureAction == mamjfaNONE) && (tmpS3C != ms3cNoFunds))
          return(SUCCESS);

        return(FAILURE);
        }

      /* charge successful */
      J->LastChargeTime = MSched.Time;

      MUFree(&RspBuf);
      }      /* END BLOCK */

      break;
 
    case mamtNONE:

      bmunset(&J->IFlags,mjifAMReserved); 

      return(SUCCESS);
 
      /*NOTREACHED*/
 
      break;
 
    default:
 
      bmunset(&J->IFlags,mjifAMReserved);

      return(SUCCESS);
 
      /*NOTREACHED*/ 
 
      break;
    }  /* switch (AM->Type) */
 
  bmunset(&J->IFlags,mjifAMReserved);

  return(SUCCESS);
  }  /* END MAMAllocJDebit() */




/**
 * Debit for reservation usage.
 * 
 * @see MRsvChargeAllocation() - parent
 * @see MAMAllocRReserve() - peer
 *
 * @param AM (I)
 * @param R (I)
 * @param IsPartial (I) [additional charges will be forthcoming for rsv]
 * @param S3C (O) [optional] - SSS Status Code Decade
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 */

int MAMAllocRDebit(

  mam_t                  *AM,        /* I */
  mrsv_t                 *R,         /* I */
  mbool_t                 IsPartial, /* I */
  enum MS3CodeDecadeEnum *S3C,       /* O (optional) */
  char                   *EMsg)      /* O (optional,minsize=MMAX_LINE) */

  {
  char  NodeType[MMAX_NAME];
 
  char  AName[MMAX_NAME];
  char  UName[MMAX_NAME];
 
  int   rc;
 
  long  Duration;
 
  double PCRate = 1.0;
  double RCost = 0.0;

  const char *FName = "MAMAllocRDebit";
 
  MDB(3,fAM) MLog("%s(%s,%s,%s,%p,%p)\n",
    FName,
    (AM != NULL) ? AM->Name : "NULL",
    (R != NULL) ? R->Name : "NULL",
    MBool[IsPartial],S3C,EMsg);

  if (S3C != NULL)
    *S3C = ms3cNone;
 
  if (EMsg != NULL)
    EMsg[0] = '\0';
 
  if ((AM == NULL) || (R == NULL))
    {
    return(FAILURE);
    }
 
  if ((AM->Type == mamtNONE) ||
     ((MSched.Mode != msmNormal) && (MAMTestMode != TRUE)))
    {
    return(SUCCESS);
    }

  if (R->CIPS <= 0.0)
    {
    char Message[] = "Reservation debit unnecessary -- No idle cycles in reservation";
    MDB(7,fAM) MLog("INFO:     %s in %s\n",Message,FName);
    if (EMsg != NULL) strcpy(EMsg,Message);
    return(SUCCESS);
    }

  if (R->A != NULL)
    MUStrCpy(AName,R->A->Name,sizeof(AName));
  else
    AName[0] = '\0';
 
  if (R->U != NULL)
    MUStrCpy(AName,R->U->Name,sizeof(UName));
  else
    UName[0] = '\0';
 
  /* Calculate Charge Duration */
  {
    long ChargeStartTime;
    long ChargeEndTime;

    ChargeStartTime = (R->LastChargeTime > 0) ? R->LastChargeTime : R->StartTime;
    ChargeEndTime = (AM->ChargePolicy == mamcpDebitRequestedWC) ? R->EndTime : MSched.Time;
    Duration = ChargeEndTime - ChargeStartTime;
  }

  PCRate = MIN(1.0,(double)R->CIPS / (R->CAPS + R->CIPS));

  strcpy(NodeType,MDEF_NODETYPE);      

  if (AM->IsChargeExempt[mxoRsv] == TRUE)
    return(SUCCESS);

  MRsvGetAllocCost(R,NULL,&RCost,IsPartial,FALSE);

  if (AM->ChargePolicyIsLocal == FALSE)
    {
    /* set cost to 0.0 so we don't add the charge option */

    RCost = 0.0;
    }
  else if ((AM->ChargePolicyIsLocal == TRUE) && 
      (RCost == 0.0))
    {
    /* no charge required */
    return(SUCCESS);
    }

  switch (AM->Type)
    {
    case mamtNative:

      rc = MAMNativeDoCommand(AM,R,mxoRsv,mamnEnd,NULL,NULL,NULL);

      if (rc == SUCCESS)
        R->LastChargeTime = MSched.Time;

      break;

    case mamtFILE:
 
      fprintf(AM->FP,"%-15s TIME=%ld TYPE=res MACHINE=DEFAULT ACCOUNT=%s USER=%s RESOURCE=%d RESOURCETYPE=%s PROCCRATE=%.2f DURATION=%ld QOS=%s REQUESTID=%s COST=%.2f\n",
        "WITHDRAWAL",
        MSched.Time,
        AName,
        UName,
        R->AllocPC,
        NodeType,
        PCRate,
        Duration,
        DEFAULT,
        R->Name,
        RCost);
 
      fflush(AM->FP);

      R->LastChargeTime = MSched.Time;

      break;
 
    case mamtMAM:
    case mamtGOLD:

      {
      enum MS3CodeDecadeEnum tmpS3C;

      mxml_t *URE = NULL;
      mxml_t *RE;
      mxml_t *DE;
      mxml_t *AE;
      mxml_t *OE = NULL;

      char *RspBuf = NULL;

      /* NOTE:  we only want to charge on the following conditions: 
                -the rsv has a chargeuser specified
                -the rsv has a chargeaccount AND an owner of type user */

      if (R->U == NULL)
        {
        if (R->A == NULL)
          {
          MDB(3,fSTRUCT) MLog("WARNING:  No chargeuser or chargeaccount. Reservation '%s' will not be charged.\n",
              R->Name);

          return(SUCCESS);
          }
        else if (R->OType != mxoUser)
          {
          MDB(3,fSTRUCT) MLog("WARNING:  Reservation charging requires user. Reservation '%s' will not be charged.\n",
              R->Name);

          return(SUCCESS);
          }
        } /* END if(R->U == NULL) */

      /* Request */
      RE = NULL;
      MXMLCreateE(&RE,(char*)MSON[msonRequest]);
      MXMLSetAttr(RE,(char *)MSAN[msanAction],(void *)"Charge",mdfString);
      MS3SetObject(RE,"UsageRecord",NULL);

      /* Data */
      DE = NULL;
      MXMLCreateE(&DE,(char*)MSON[msonData]);
      MXMLAddE(RE,DE);

      /* Usage Record */
      URE = NULL;
      MXMLCreateE(&URE,"UsageRecord");
      MXMLAddE(DE,URE);

      /* Type */
      AE = NULL;
      MXMLCreateE(&AE,"Type");
      MXMLSetVal(AE,(void *)"Reservation",mdfString);
      MXMLAddE(URE,AE);

      /* Instance */
      AE = NULL;
      MXMLCreateE(&AE,"Instance");
      MXMLSetVal(AE,(void *)R->Name,mdfString);
      MXMLAddE(URE,AE);

      /* User */
      if (R->U != NULL)
        {
        AE = NULL;
        MXMLCreateE(&AE,"User");
        MXMLSetVal(AE,(void *)R->U->Name,mdfString);
        MXMLAddE(URE,AE);
        }
      else /* create the user from the owner if chargeuser isn't specified */
        {
        mgcred_t *RsvOwner = (mgcred_t *)R->O;
        AE = NULL;
        MXMLCreateE(&AE,"User");
        MXMLSetVal(AE,(void *)RsvOwner->Name,mdfString);
        MXMLAddE(URE,AE);
        }

      /* Group */
      if (R->G != NULL)
        {
        AE = NULL;
        MXMLCreateE(&AE,"Group");
        MXMLSetVal(AE,(void *)R->G->Name,mdfString);
        MXMLAddE(URE,AE);
        }

      /* Account */
      if (R->A != NULL)
        {
        AE = NULL;
        MXMLCreateE(&AE,"Account");
        MXMLSetVal(AE,(void *)R->A->Name,mdfString);
        MXMLAddE(URE,AE);
        }
      else
        {
        mgcred_t *tmpA = NULL;

        /* look for user-based defaults */
        if ((R->U != NULL) && (R->U->F.ADef != NULL))
          {
          tmpA = (mgcred_t *)R->U->F.ADef;
          }

        if (tmpA != NULL)
          {
          AE = NULL;
          MXMLCreateE(&AE,"Account");
          MXMLSetVal(AE,(void *)tmpA->Name,mdfString);
          MXMLAddE(URE,AE);
          }
        }

      /* Class */
      if (R->C != NULL)
        {
        AE = NULL;
        MXMLCreateE(&AE,"Class");
        MXMLSetVal(AE,(void *)R->C->Name,mdfString);
        MXMLAddE(URE,AE);
        }

      /* QOS */
      if (R->Q != NULL)
        {
        AE = NULL;
        MXMLCreateE(&AE,(char *)MS3JobAttr[ms3jaQOSReq]);
        MXMLSetVal(AE,(void *)R->Q->Name,mdfString);
        MXMLAddE(URE,AE);
        }

      /* Machine */
      /* Set Machine equal to RM name thats obtained off the partition information if available, otherwise use the scheduler name. */
      AE = NULL;
      if (MXMLCreateE(&AE,"Machine") == SUCCESS)
        {
        mrm_t *RM = (R->PtIndex >= 0) ? MPar[R->PtIndex].RM : NULL;

        if ((RM != NULL) && (RM->Name[0] != '\0'))
          MXMLSetVal(AE,(void *)RM->Name,mdfString);
        else
          MXMLSetVal(AE,(void *)MSched.Name,mdfString);
        MXMLAddE(URE,AE);
        }      

      /* Nodes */
      if (R->AllocNC > 0)
        {
        AE = NULL;
        MXMLCreateE(&AE,"Nodes");
        MXMLSetVal(AE,(void *)&R->AllocNC,mdfInt);
        MXMLAddE(URE,AE);
        }

      /* Processors */
      if (R->AllocPC > 0)
        {
        AE = NULL;
        MXMLCreateE(&AE,(char *)MS3ReqAttr[ms3rqaTCReqMin]);
        MXMLSetVal(AE,(void *)&R->AllocPC,mdfInt);
        MXMLAddE(URE,AE);
        }

      /* Consumption Rate */
      if (PCRate < 1.0)
        {
        AE = NULL;
        MXMLCreateE(&AE,"ConsumptionRate");
        MXMLSetVal(AE,(void *)&PCRate,mdfDouble);
        MXMLAddE(URE,AE);
        }

      /* Memory */
      if (R->DRes.Mem > 0)
        {
        int Memory;

        Memory = R->AllocTC * R->DRes.Mem;

        AE = NULL;
        MXMLCreateE(&AE,"Memory");
        MXMLSetVal(AE,(void *)&Memory,mdfInt);
        MXMLAddE(URE,AE);
        }

      /* Duration */
      OE = NULL;
      MXMLCreateE(&OE,(char*)MSON[msonOption]);
      MXMLSetAttr(OE,MSAN[msanName],(void *)"Duration",mdfString);
      MXMLSetVal(OE,(void *)&Duration,mdfLong);
      MXMLAddE(RE,OE);

      /* Start Time */
      if (R->StartTime > 0)
        {
        AE = NULL;
        MXMLCreateE(&AE,"StartTime");
        MXMLSetVal(AE,&R->StartTime,mdfLong);
        MXMLAddE(URE,AE);
        }

      /* End Time */
      if (IsPartial == FALSE)
        {
        mulong tmpL;
        tmpL = MIN(R->EndTime,MSched.Time);

        AE = NULL;
        MXMLCreateE(&AE,"EndTime");
        MXMLSetVal(AE,&tmpL,mdfLong);
        MXMLAddE(URE,AE);
        }

      if (NodeType[0] != '\0')
        {
        AE = NULL;
        MXMLCreateE(&AE,"NodeType");
        MXMLSetVal(AE,(void *)NodeType,mdfString);
        MXMLAddE(URE,AE);
        }

      if (RCost != 0.0)
        {
        /* add charge option */
        OE = NULL;
        MXMLCreateE(&OE,(char*)MSON[msonOption]);
        MXMLSetAttr(OE,MSAN[msanName],(void *)"Charge",mdfString);
        MXMLSetVal(OE,(void *)&RCost,mdfDouble);
        MXMLAddE(RE,OE);
        }

      if (IsPartial == TRUE)
        {
        MS3AddChild(RE,(char*)MSON[msonOption],"Incremental","True",NULL);
        }

      /* submit request */

      if (MAMGoldDoCommand(RE,&AM->P,NULL,&tmpS3C,EMsg) == FAILURE)
        {
        MDB(2,fAM) MLog("WARNING:  Unable to charge funds for job\n");

        if (S3C != NULL)
          *S3C = tmpS3C;

        if ((AM->StartFailureAction == mamjfaNONE) && (tmpS3C != ms3cNoFunds))
          return(SUCCESS);

        return(FAILURE);
        }

      /* charge successful */
      R->LastChargeTime = MSched.Time;

      if (RCost != 0.0) 
        {
        double tmpD;

        tmpD = MIN(RCost,R->LienCost);

        R->LienCost -= tmpD;
        }    /* END if (RCost != 0.0) */

      MUFree(&RspBuf);
      }      /* END BLOCK (GOLD) */

      R->LastChargeTime = MSched.Time;

      return(SUCCESS);
 
      /*NOTREACHED*/
 
      break;

    case mamtNONE:
 
      return(SUCCESS);
 
      /*NOTREACHED*/
 
      break;

    default:

      return(SUCCESS);
 
      /*NOTREACHED*/
 
      break;
    }   /* switch (AM->Type) */

  return(SUCCESS);
  }  /* END MAMAllocRDebit() */





/**
 * Create accounting manager lien for job.
 *
 * @see MAMAllocJDebit() - peer
 *
 * NOTE:  does not create lien for anticipated gmetric usage.
 *
 * @param AM (I)
 * @param J (I)
 * @param TestAlloc (I) [TRUE==getquote/verify alloc availability]
 * @param S3C (O) [optional] - SSS Status Code Decade
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 */

int MAMAllocJReserve(

  mam_t                  *AM,        /* I */
  mjob_t                 *J,         /* I */
  mbool_t                 TestAlloc, /* I (TRUE==getquote/verify alloc availability) */
  enum MS3CodeDecadeEnum *S3C,       /* O (optional) */
  char                   *EMsg)      /* O (optional,minsize=MMAX_LINE) */

  {
  char    AccountName[MMAX_NAME];
  char    QOSString[MMAX_NAME];
 
  char    NodeType[MMAX_NAME];
  long    WCLimit;

  int     ProcCount;
  int     DedicatedMemory = 0;
  int     NodeCount = 0;

  double  NodeCost = 0.0;
  double  JobCost  = 0.0;
 
  mnode_t *N;
 
  mpar_t  *P;

  mrm_t   *RM;

  mreq_t  *RQ;

  mbool_t  ChargeByDedicatedNode;

  const char *FName = "MAMAllocJReserve";
 
  MDB(3,fAM) MLog("%s(%s,%s,%s,%p,%p)\n",
    FName,
    (AM != NULL) ? AM->Name : "NULL",
    (J != NULL) ? J->Name : "NULL",
    MBool[TestAlloc],
    S3C,EMsg);

  if (S3C != NULL)
    *S3C = ms3cNone;
 
  if (EMsg != NULL)
    EMsg[0] = '\0';

  if ((AM == NULL) || (J == NULL))
    {
    if (EMsg != NULL)
      strcpy(EMsg,"Internal error when reserving funds");

    return(FAILURE);
    }

  if ((AM->Type == mamtNONE) || 
     ((MSched.Mode != msmNormal) && (MAMTestMode != TRUE)))
    {
    /* no action required */

    return(SUCCESS);
    }

  if ((J->Credential.C != NULL) && (J->Credential.C->DisableAM == TRUE))
    {
    return(SUCCESS);
    }

  if ((TestAlloc == FALSE) && (bmisset(&J->IFlags,mjifAMReserved)))
    {
    /* Lien has already been created */

    return(SUCCESS);
    }

  if (bmisset(&J->IFlags,mjifVPCMap))
    {
    /* do not reserve here - VPC's charged directly */

    return(SUCCESS);
    }

  if (AM->State == mrmsDown)
    {
    MDB(5,fAM) MLog("WARNING:  Unable to create lien for job %s - AM is down\n",
      J->Name);

    if (S3C != NULL)
      *S3C = ms3cConnection; /* Connection Failure */

    if (EMsg != NULL)
      strcpy(EMsg,"Accounting manager is down");

    return(FAILURE);
    }
 
  if (((J->Credential.A == NULL) || (!strcmp(J->Credential.A->Name,NONE))) &&
       (AM->Type != mamtMAM) &&    
       (AM->Type != mamtGOLD) &&    
       (AM->Type != mamtFILE))
    {
    MDB(2,fAM) MLog("WARNING:  No account specified for job %s\n",
      J->Name);

    if (S3C != NULL) 
      *S3C = ms3cClientBusinessLogic;

    if (EMsg != NULL)
      strcpy(EMsg,"No account specified");
 
    return(FAILURE);
    }

  /* Get access to the destination RM. */

  RM = J->DestinationRM;
  if (RM == NULL)
    {
    mnode_t *N;

    if ((MNLGetNodeAtIndex(&J->NodeList,0,&N) == SUCCESS) && (N->PtIndex >= 0))
      RM = MPar[N->PtIndex].RM;

    /* NOTE:  This still doesn't guarentee a valid pointer.  Must check for NULL.  */
    } 
  RQ = J->Req[0];

  MJobGetEstNC(J,&NodeCount,NULL,FALSE);

  /* determine how much memory was dedicated for this job */

  /* NOTE: This is not multi-req friendly... */

  DedicatedMemory = RQ->DRes.Mem;

  if ((J->Credential.A != NULL) && strcmp(J->Credential.A->Name,NONE))
    {
    strcpy(AccountName,J->Credential.A->Name);
    }
  else
    {
    AccountName[0] = '\0';
    }

  MJobGetCost(J,TRUE,&JobCost);

  MNLGetNodeAtIndex(&J->Req[0]->NodeList,0,&N);

  P = &MPar[0];

  NodeType[0] = '\0';

  if (N != NULL)
    {
    if (MUNLGetMinAVal(
          &J->NodeList,
          mnaNodeType,
          NULL,
          (void **)NodeType) == FAILURE)
      { 
      if ((N->RackIndex != -1) && 
          (MRack[N->RackIndex].NodeType[0] != '\0'))
        {
        strcpy(NodeType,MRack[N->RackIndex].NodeType);
        }
      else if (N->NodeType != NULL)
        {
        strcpy(NodeType,N->NodeType);
        }
      else
        {
        strcpy(NodeType,MDEF_NODETYPE);
        }
      }

    switch (AM->NodeChargePolicy)
      {
      case mamncpAvg:

        MUNLGetAvgAVal(
          &J->NodeList,
          mnaChargeRate,
          NULL,
          (void **)&NodeCost);

        break;

      case mamncpMax:

        MUNLGetMaxAVal(
          &J->NodeList,
          mnaChargeRate,
          NULL,
          (void **)&NodeCost);

        break;

      case mamncpMin:
      default:

        MUNLGetMinAVal(
          &J->NodeList,
          mnaChargeRate,
          NULL,
          (void **)&NodeCost);

        break;
      }  /* END switch (A->NodeChargePolicy) */
    }    /* END if (N != NULL) */
 
  /* determine QOS */
 
  if ((J->Credential.Q != NULL) && strcmp(J->Credential.Q->Name,DEFAULT))
    {
    sprintf(QOSString,"QOS=%s ",
      J->Credential.Q->Name);
    }
  else
    {
    QOSString[0] = '\0';
    }

  ChargeByDedicatedNode = FALSE;

  if ((J->Credential.Q != NULL) &&
      (bmisset(&J->Credential.Q->Flags,mqfDedicated)) &&
      (!MNLIsEmpty(&J->NodeList)))
    {
    /* QoS 'dedicated' flag set */

    ChargeByDedicatedNode = TRUE;
    }
  else if ((AM->ChargePolicy == mamcpDebitAllBlocked) ||
           (AM->ChargePolicy == mamcpDebitSuccessfulBlocked))
    {
    enum MNodeAccessPolicyEnum NAPolicy;

    MJobGetNAPolicy(J,NULL,NULL,NULL,&NAPolicy);

    if ((NAPolicy == mnacSingleTask) || (NAPolicy == mnacSingleJob))
      {
      /* node dedicated by job or global node access policy */

      ChargeByDedicatedNode = TRUE;
      }
    }

  if (ChargeByDedicatedNode == TRUE)
    {
    mnode_t *tmpN;

    int nindex;

    ProcCount = 0;

    for (nindex = 0;MNLGetNodeAtIndex(&J->NodeList,nindex,&tmpN) == SUCCESS;nindex++)
      {
      ProcCount += N->CRes.Procs;
      }  /* END for (nindex) */
    }    /* END if (ChargeByDedicatedNode == TRUE) */
  else
    {
    ProcCount = MAX(1,J->TotalProcCount);
    }

  /* if we're just doing a test quote and we don't have a node list, get
    * the processor count from the job */

  if ((bmisset(&AM->Flags,mamfStrictQuote)) && (TestAlloc == TRUE) && (ProcCount == 0))
    {
    ProcCount = MAX(1,J->TotalProcCount);
    }

  WCLimit = J->WCLimit;

  MDB(7,fSCHED) MLog("INFO:     WCLimit for AM reservation set to WCLimit of job (%d)\n",
    WCLimit);

  if ((N != NULL) && (N->Speed != 0) &&
      bmisset(&P->Flags,mpfUseMachineSpeed))
    {
    WCLimit = (long)((double)WCLimit / N->Speed);
    }
 
  switch (AM->Type)
    {
    case mamtNative:

      MAMNativeDoCommand(AM,J,mxoJob,mamnStart,NULL,NULL,NULL);

      break;

    case mamtFILE:

      {
      fprintf(AM->FP,"%-15s TIME=%ld TYPE=job ACCOUNT=%s USER=%s CLASS=%s RESOURCE=%d RESOURCETYPE=%s DURATION=%ld %sREQUESTID=%s CHARGE=%f\n",
        "RESERVE",
        MSched.Time,
        AccountName,
        J->Credential.U->Name,
        (J->Credential.C != NULL) ? J->Credential.C->Name : DEFAULT,
        ProcCount,
        NodeType,
        WCLimit,
        QOSString,
        J->Name,
        JobCost);
 
      fflush(AM->FP);
      }      /* END BLOCK (case mamtFILE) */

      break;
 
    case mamtNONE:

      return(SUCCESS);

      /*NOTREACHED*/
 
      break;

    case mamtMAM:
    case mamtGOLD:

      {
      mulong tmpL;

      mxml_t *RE  = NULL;
      mxml_t *DE  = NULL;
      mxml_t *URE = NULL;
      mxml_t *AE  = NULL;
      mxml_t *OE  = NULL;

      /* FORMAT: <Request action="Reserve"><Object>UsageRecord</Object><Data></Data></Request> */

      /* Request */
      RE = NULL;
      MXMLCreateE(&RE,(char*)MSON[msonRequest]);

      if (TestAlloc == TRUE)
        {
        /* change lien to quote */
        MXMLSetAttr(RE,MSAN[msanAction],(void *)"Quote",mdfString);
        }
      else
        {
        MXMLSetAttr(RE,MSAN[msanAction],(void *)"Reserve",mdfString);
        }

      /* add replace option to replace old liens */
      OE = NULL;
      MXMLCreateE(&OE,(char*)MSON[msonOption]);
      MXMLSetAttr(OE,MSAN[msanName],(void *)"Replace",mdfString);
      MXMLSetVal(OE,(void *)"True",mdfString);
      MXMLAddE(RE,OE);
      MS3SetObject(RE,"UsageRecord",NULL);

      /* Date */
      DE = NULL;
      MXMLCreateE(&DE,(char*)MSON[msonData]);
      MXMLAddE(RE,DE);

      /* Usage Record */
      URE = NULL;
      MXMLCreateE(&URE,"UsageRecord");
      MXMLAddE(DE,URE);

      /* Type */
      AE = NULL;
      MXMLCreateE(&AE,"Type");
      MXMLSetVal(AE,(void *)"Job",mdfString);
      MXMLAddE(URE,AE);

      /* Instance */
      AE = NULL;
      MXMLCreateE(&AE,"Instance");
      MXMLSetVal(AE,(void *)J->Name,mdfString);
      MXMLAddE(URE,AE);

      /* User */
      if (J->Credential.U != NULL)
        {
        AE = NULL;
        MXMLCreateE(&AE,"User");
        MXMLSetVal(AE,(void *)J->Credential.U->Name,mdfString);
        MXMLAddE(URE,AE);
        }

      /* Group */
      if (J->Credential.G != NULL)
        {
        AE = NULL;
        MXMLCreateE(&AE,"Group");
        MXMLSetVal(AE,(void *)J->Credential.G->Name,mdfString);
        MXMLAddE(URE,AE);
        }

      /* Account */
      if (J->Credential.A != NULL)
        {
        AE = NULL;
        MXMLCreateE(&AE,"Account");
        MXMLSetVal(AE,(void *)J->Credential.A->Name,mdfString);
        MXMLAddE(URE,AE);
        }

      /* Class */
      if (J->Credential.C != NULL)
        {
        AE = NULL;
        MXMLCreateE(&AE,"Class");
        MXMLSetVal(AE,(void *)&J->Credential.C->Name,mdfString);
        MXMLAddE(URE,AE);
        }

      /* QOS */
      if (J->Credential.Q != NULL)
        {
        AE = NULL;
        MXMLCreateE(&AE,(char *)MS3JobAttr[ms3jaQOSReq]);
        MXMLSetVal(AE,(void *)J->Credential.Q->Name,mdfString);
        MXMLAddE(URE,AE);
        }

      /* Machine */
      AE = NULL;
      MXMLCreateE(&AE,"Machine");
      MXMLSetVal(AE,(RM != NULL) ? (void*)RM->Name : (void*)MSched.Name,mdfString);
      MXMLAddE(URE,AE);
     
      /* Nodes */
      if (NodeCount > 0)
        {
        AE = NULL;
        MXMLCreateE(&AE,(char *)MS3JobAttr[ms3jaAllocNodeList]);
        MXMLSetVal(AE,(void *)&NodeCount,mdfInt);
        MXMLAddE(URE,AE);
        }

      /* Processors */
      if (ProcCount > 0)
        {
        AE = NULL;
        MXMLCreateE(&AE,(char *)MS3ReqAttr[ms3rqaTCReqMin]);
        MXMLSetVal(AE,(void *)&ProcCount,mdfInt);
        MXMLAddE(URE,AE);
        }

      /* Memory */
      /* SSS specification is <Memory context="Dedicated">1234</Memory> */
      if (DedicatedMemory > 0)
        {
        AE = NULL;
        MXMLCreateE(&AE,"Memory");
        MXMLSetAttr(AE,(char *)"context",(void *)"Dedicated",mdfString);
        MXMLSetVal(AE,(void *)&DedicatedMemory,mdfInt);
        MXMLAddE(URE,AE);
        }

      /* Duration */
      if (J->WCLimit > 0)
        {
        if (TestAlloc == TRUE) /* Policy Quote */
          {
          if (MAM[0].FlushPeriod != mpNONE)
            tmpL = MIN(J->WCLimit,MAM[0].FlushInterval);
          else
            tmpL = J->WCLimit;
          }
        else /* Lien */
          {
          long LienStartTime;
          long LienEndTime;

          LienStartTime = (J->LastChargeTime > 0) ? J->LastChargeTime : (J->StartTime > 0) ? J->StartTime : MSched.Time;
          if (MAM[0].FlushPeriod != mpNONE)
            LienEndTime = MIN(MAM[0].FlushTime,(J->StartTime > 0) ? (J->StartTime + J->WCLimit) : (MSched.Time + J->WCLimit));
          else
            LienEndTime = (J->StartTime > 0) ? (J->StartTime + J->WCLimit) : (MSched.Time + J->WCLimit);
          
          tmpL = LienEndTime - LienStartTime;
          }

        OE = NULL;
        MXMLCreateE(&OE,(char*)MSON[msonOption]);
        MXMLSetAttr(OE,MSAN[msanName],(void *)"Duration",mdfString);
        MXMLSetVal(OE,(void *)&tmpL,mdfLong);
        MXMLAddE(RE,OE);
        }

      if (NodeType[0] != '\0')
        {
        AE = NULL;
        MXMLCreateE(&AE,"NodeType");
        MXMLSetVal(AE,(void *)NodeType,mdfString);
        MXMLAddE(URE,AE);
        }

      /* Generic Resources */
      if (!MSNLIsEmpty(&RQ->DRes.GenericRes))
        {
        int gindex;
        double TaskSeconds;

        for (gindex = 1;gindex < MSched.M[mxoxGRes];gindex++)
          {
          if (MGRes.Name[gindex][0] == '\0')
            break;

          if (MSNLGetIndexCount(&RQ->DRes.GenericRes,gindex) == 0) 
            continue;

          TaskSeconds = MSNLGetIndexCount(&RQ->DRes.GenericRes,gindex) * RQ->TaskCount;
  
          AE = NULL;
          MXMLCreateE(&AE,MGRes.Name[gindex]);
          MXMLSetVal(AE,(void *)&TaskSeconds,mdfDouble);
          MXMLAddE(URE,AE);
          }
        }

      if (NodeCost != 0)
        {
        AE = NULL;
        MXMLCreateE(&AE,"NodeCost");
        MXMLSetVal(AE,(void *)&NodeCost,mdfDouble);
        MXMLAddE(URE,AE);
        }

      if (bmisset(&AM->Flags,mamfLocalCost))
        {
        /* add charge option */
        OE = NULL;
        MXMLCreateE(&OE,(char*)MSON[msonOption]);
        MXMLSetAttr(OE,MSAN[msanName],(void *)"Charge",mdfString);
        MXMLSetVal(OE,(void *)&JobCost,mdfDouble);
        MXMLAddE(RE,OE);
        }

      /* submit request */

      if (MAMGoldDoCommand(
            RE,
            &AM->P,
            NULL,
            S3C,               /* O - failure reason */
            EMsg) == FAILURE)  /* O */
        {
        MDB(2,fAM) MLog("WARNING:  Unable to reserve funds for job %s - %s\n",
          J->Name,
          (EMsg != NULL) ? EMsg : "Reason Unknown");

        return(FAILURE);
        }

      /* reserve successful */

      if (TestAlloc != TRUE)
        bmset(&J->IFlags,mjifAMReserved);

      }  /* END BLOCK */

      return(SUCCESS);
 
      /*NOTREACHED*/
 
      break;
       
    default:

      if (TestAlloc != TRUE)
        bmset(&J->IFlags,mjifAMReserved);
 
      return(SUCCESS);

      /*NOTREACHED*/
 
      break;
    }  /* END switch (AM->Type) */

  if (TestAlloc != TRUE)
    bmset(&J->IFlags,mjifAMReserved);

  return(SUCCESS);
  }  /* END MAMAllocJReserve() */


/**
 * Cancel accounting manager lien.
 *
 * @see MAMAllocRReserve() - peer - create lien for reservations
 *
 * @param AM (I)
 * @param O (I)
 * @param OType (I)
 * @param S3C (O) [optional] - SSS Status Code Decade
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 */

int MAMAllocResCancel(

  mam_t                  *AM,
  void                   *O,
  enum MXMLOTypeEnum      OType,
  enum MS3CodeDecadeEnum *S3C,
  char                   *EMsg)

  {
  char  tmpMsg[MMAX_LINE];
  char  ReqID[MMAX_NAME];

  mrsv_t *R = NULL;
  mjob_t *J = NULL;

  mgcred_t *tmpA = NULL;  /* pointer to chargeable account */

  const char *FName = "MAMAllocResCancel";

  MDB(3,fAM) MLog("%s(%s,%s,%s,%p,%p)\n",
    FName,
    (AM != NULL) ? AM->Name : "NULL",
    (OType == mxoJob && O != NULL) ? ((mjob_t *)O)->Name
       : (OType == mxoRsv && O != NULL) ? ((mrsv_t *)O)->Name
       : "unknown",
    (OType > mxoNONE && OType < mxoLAST) ? MXO[OType] : "unknown",
    S3C,
    EMsg);

  if (S3C != NULL)
    *S3C = ms3cNone;

  if (EMsg != NULL)
    EMsg[0] = '\0';
 
  if (OType == mxoJob)
    J = (mjob_t *)O;
  else if (OType == mxoRsv)
    R = (mrsv_t *)O;

  if ((O == NULL) || (OType == mxoNONE))
    {
    if (EMsg != NULL)
      strcpy(EMsg,"Invalid object specified");

    return(FAILURE);
    }

  if ((MSched.Mode != msmNormal) && (MAMTestMode != TRUE))
    {
    if (OType == mxoJob)
      {
      bmunset(&J->IFlags,mjifAMReserved);
      }

    return(SUCCESS);
    }

  if (AM->Type == mamtNONE)
    {
    if (OType == mxoJob)
      {
      bmunset(&J->IFlags,mjifAMReserved);
      }

    return(SUCCESS);
    }

  if (AM->State == mrmsDown)
    {
    if (EMsg != NULL)
      strcpy(EMsg,"AM is down");

    return(FAILURE);
    }

  strcpy(ReqID,(OType == mxoJob && O != NULL) ? ((mjob_t *)O)->Name
             : (OType == mxoRsv && O != NULL) ? ((mrsv_t *)O)->Name
             : "unknown");

  tmpMsg[0] = '\0';

  switch (AM->Type)
    {
    case mamtNative:

      MAMNativeDoCommand(AM,O,OType,mamnDelete,NULL,NULL,NULL);

      break;

    case mamtFILE:

      fprintf(AM->FP,"%-15s TIME=%ld REQUESTID=%s%s\n",
        "REMOVE",
        MSched.Time,
        ReqID,
        tmpMsg);

      fflush(AM->FP);

      break;

    case mamtMAM:
    case mamtGOLD:

      {
      mxml_t *RE = NULL;

      MXMLCreateE(&RE,(char*)MSON[msonRequest]);

      MXMLSetAttr(RE,MSAN[msanAction],(void *)"Delete",mdfString);

      MS3SetObject(RE,"Lien",NULL);

      MS3AddWhere(RE,"Instance",ReqID,NULL);

      /* submit request */

      if (MAMGoldDoCommand(
            RE,
            &AM->P,
            NULL,
            S3C,               /* O - failure reason */
            EMsg) == FAILURE)  /* O */
        {
        MDB(2,fAM) MLog("WARNING:  Unable to cancel lien for instance %s - %s\n",
          ReqID,
          (EMsg != NULL) ? EMsg : "Reason Unknown");

        return(FAILURE);
        }

      /* accounting manager lien cancel successful */

      if ((tmpA != NULL) && (R != NULL))
        {
        if ((R->StartTime > MSched.Time) && (R->LienCost > 0.0))
          {
          R->LienCost = 0.0;
          }
        }
      }    /* END BLOCK (case mamtMAM) */

      if (OType == mxoJob)
        bmunset(&J->IFlags,mjifAMReserved);

      return(SUCCESS);

      /*NOTREACHED*/

      break;

    case mamtNONE:

      if (OType == mxoJob)
        bmunset(&J->IFlags,mjifAMReserved);

      return(SUCCESS);

      /*NOTREACHED*/

      break;

    default:

      /* NO-OP */

      if (OType == mxoJob)
        bmunset(&J->IFlags,mjifAMReserved);

      return(SUCCESS);

      /*NOTREACHED*/

      break;
    }  /* END switch (AM->Type) */

  if (OType == mxoJob)
    bmunset(&J->IFlags,mjifAMReserved);

  return(SUCCESS);
  }  /* MAMAllocResCancel() */


/**
 * Create accounting manager lien for resource reservation.
 *
 * @see MAMAllocRDebit() - peer - charge credits against associated account
 * @see MAMAllocResCancel() - peer - remove existing lien
 * @see MAMAllocJReserve() - peer 
 *
 * @param AM (I)
 * @param R (I)
 * @param S3C (O) [optional] - SSS Status Code Decade
 * @param SEMsg (O) [optional,minsize=MMAX_LINE]
 */

int MAMAllocRReserve(

  mam_t                  *AM,
  mrsv_t                 *R,
  enum MS3CodeDecadeEnum *S3C,
  char                   *SEMsg)

  {
  long    Duration = 0;

  double  RCost = 0.0;

  char    tmpEMsg[MMAX_LINE];
  char   *EMsg = NULL;

  mgcred_t *tmpA = NULL;

  const char *FName = "MAMAllocRReserve";

  MDB(3,fAM) MLog("%s(%s,%s,%p,%p)\n",
    FName,
    (AM != NULL) ? AM->Name : "NULL",
    (R != NULL) ? R->Name : "NULL",
    S3C,
    SEMsg);

  if (S3C != NULL)
    *S3C = ms3cNone;

  EMsg = (SEMsg != NULL) ? SEMsg : tmpEMsg;
  EMsg[0] = '\0';

  if ((AM == NULL) || (R == NULL))
    {
    return(FAILURE);
    }

  if ((MSched.Mode != msmNormal) && (MAMTestMode != TRUE))
    {
    return(SUCCESS);
    }

  if (AM->Type == mamtNONE)
    {
    return(SUCCESS);
    }

  if ((R->A == NULL) || !strcmp(R->A->Name,NONE))
    {
    MDB(2,fAM) MLog("WARNING:  No account specified for reservation %s\n",
      R->Name);

    sprintf(EMsg,"No account specified for reservation %s\n",R->Name);

    if (S3C != NULL)
      *S3C = ms3cClientBusinessLogic;

    return(FAILURE);
    }

  if (MAM[0].FlushPeriod != mpNONE)
    Duration = MIN(R->EndTime,MAM[0].FlushTime) -
      ((R->LastChargeTime > 0) ? R->LastChargeTime : MSched.Time);
  else
    Duration = R->EndTime - MSched.Time;

  MAcctFind(R->A->Name,&tmpA);

  if (AM->Type != mamtNative)
    {
    if (AM->IsChargeExempt[mxoRsv] == TRUE)
      return(SUCCESS);

    RCost = 0.0;
    }

  if ((AM->ChargePolicyIsLocal == TRUE) &&
      (RCost == 0.0))
    {
    /* no charge required */

    return(SUCCESS);
    }

  switch (AM->Type)
    {
    case mamtNative:

      MAMNativeDoCommand(AM,R,mxoRsv,mamnCreate,EMsg,NULL,S3C);

      break;

    case mamtFILE:

      fprintf(AM->FP,"%-15s TIME=%ld TYPE=res ACCOUNT=%s USER=%s RESOURCE=%d RESOURCETYPE=%s DURATION=%ld QOS=%s REQUESTID=%s COST=%f\n",
        "RESERVE",
        MSched.Time,
        R->A->Name,
        R->U->Name,
        R->AllocPC,
        MDEF_NODETYPE,
        Duration,
        DEFAULT,
        R->Name,
        RCost);

      fflush(AM->FP);

      break;

    case mamtMAM:
    case mamtGOLD:

      {
      mxml_t *RE  = NULL;
      mxml_t *DE  = NULL;
      mxml_t *URE = NULL;
      mxml_t *AE  = NULL;
      mxml_t *OE  = NULL;

      /* FORMAT: <Request action="Reserve"><Object>Lien</Object><Data></Data></Request> */

      /*
      MXMLSetAttr(E,"xmlns","http://www.scidac.org/ScalableSystems/AllocationManager",mdfString);
      */

      if (Duration <= 0)
        {
        MDB(1,fAM) MLog("ALERT:    Invalid duration (%ld) for specified reservation '%s'\n",
          Duration,
          R->Name);

        sprintf(EMsg,"Invalid duration (%ld) for specified reservation '%s'\n",
          Duration, R->Name);

        if (S3C != NULL)
          *S3C = ms3cClientBusinessLogic;

        return(FAILURE);
        }

      /* Request */
      RE = NULL;
      MXMLCreateE(&RE,(char*)MSON[msonRequest]);
      MXMLSetAttr(RE,MSAN[msanAction],(void *)"Reserve",mdfString);

      /* add replace option to replace old liens */
      OE = NULL;
      MXMLCreateE(&OE,(char*)MSON[msonOption]);
      MXMLSetAttr(OE,MSAN[msanName],(void *)"Replace",mdfString);
      MXMLSetVal(OE,(void *)"True",mdfString);
      MXMLAddE(RE,OE);

      MS3SetObject(RE,"UsageRecord",NULL);

      /* Data */
      DE = NULL;
      MXMLCreateE(&DE,(char*)MSON[msonData]);
      MXMLAddE(RE,DE);

      /* Usage Record */
      URE = NULL;
      MXMLCreateE(&URE,"UsageRecord");
      MXMLAddE(DE,URE);

      /* Type */
      AE = NULL;
      MXMLCreateE(&AE,"Type");
      MXMLSetVal(AE,(void *)"Reservation",mdfString);
      MXMLAddE(URE,AE);

      /* Instance */
      AE = NULL;
      MXMLCreateE(&AE,"Instance");
      MXMLSetVal(AE,(void *)R->Name,mdfString);
      MXMLAddE(URE,AE);

      /* User */
      if (R->U != NULL)
        {
        AE = NULL;
        MXMLCreateE(&AE,"User");
        MXMLSetVal(AE,(void *)R->U->Name,mdfString);
        MXMLAddE(URE,AE);
        }

      /* Group */
      if (R->G != NULL)
        {
        AE = NULL;
        MXMLCreateE(&AE,"Group");
        MXMLSetVal(AE,(void *)R->G->Name,mdfString);
        MXMLAddE(URE,AE);
        }

      /* Account */
      if (R->A != NULL)
        {
        AE = NULL;
        MXMLCreateE(&AE,"Account");
        MXMLSetVal(AE,(void *)R->A->Name,mdfString);
        MXMLAddE(URE,AE);
        }
      else
        {
        mgcred_t *tmpA = NULL;

        /* look for user-based defaults */
        if ((R->U != NULL) && (R->U->F.ADef != NULL))
          {
          tmpA = (mgcred_t *)R->U->F.ADef;
          }

        if (tmpA != NULL)
          {
          AE = NULL;
          MXMLCreateE(&AE,"Account");
          MXMLSetVal(AE,(void *)tmpA->Name,mdfString);
          MXMLAddE(URE,AE);
          }
        }

      /* Class */
      if (R->C != NULL)
        {
        AE = NULL;
        MXMLCreateE(&AE,"Class");
        MXMLSetVal(AE,(void *)R->C->Name,mdfString);
        MXMLAddE(URE,AE);
        }

      /* QOS */
      if (R->Q != NULL)
        {
        AE = NULL;
        MXMLCreateE(&AE,(char *)MS3JobAttr[ms3jaQOSReq]);
        MXMLSetVal(AE,(void *)R->Q->Name,mdfString);
        MXMLAddE(URE,AE);
        }

      /* Machine */
      AE = NULL;
      if (MXMLCreateE(&AE,"Machine") == SUCCESS)
        {
        mrm_t *RM = (R->PtIndex >= 0) ? MPar[R->PtIndex].RM : NULL;

        if ((RM != NULL) && (RM->Name[0] != '\0'))
          MXMLSetVal(AE,(void *)RM->Name,mdfString);
        else
          MXMLSetVal(AE,(void *)MSched.Name,mdfString);
        MXMLAddE(URE,AE);
        }

      /* Nodes */
      if (R->AllocNC > 0)
        {
        AE = NULL;
        MXMLCreateE(&AE,"Nodes");
        MXMLSetVal(AE,(void *)&R->AllocNC,mdfInt);
        MXMLAddE(URE,AE);
        }

      /* Processors */
      if (R->AllocPC > 0)
        {
        AE = NULL;
        MXMLCreateE(&AE,(char *)MS3ReqAttr[ms3rqaTCReqMin]);
        MXMLSetVal(AE,(void *)&R->AllocPC,mdfInt);
        MXMLAddE(URE,AE);
        }

      /* Memory */
      if (R->DRes.Mem > 0)
        {
        int Memory;

        Memory = R->AllocTC * R->DRes.Mem;

        AE = NULL;
        MXMLCreateE(&AE,"Memory");
        MXMLSetVal(AE,(void *)&Memory,mdfInt);
        MXMLAddE(URE,AE);
        }

      /* Duration */
      if (Duration > 0)
        {
        OE = NULL;
        MXMLCreateE(&OE,(char*)MSON[msonOption]);
        MXMLSetAttr(OE,MSAN[msanName],(void *)"Duration",mdfString);
        MXMLSetVal(OE,(void *)&Duration,mdfLong);
        MXMLAddE(RE,OE);
        }

      if (RCost != 0.0)
        {
        /* add charge option */
        OE = NULL;
        MXMLCreateE(&OE,(char*)MSON[msonOption]);
        MXMLSetAttr(OE,MSAN[msanName],(void *)"Charge",mdfString);
        MXMLSetVal(OE,(void *)&RCost,mdfDouble);
        MXMLAddE(RE,OE);
        }

      /* submit request */

      if (MAMGoldDoCommand(
            RE,
            &AM->P,
            NULL,
            S3C,               /* O - failure reason */
            EMsg) == FAILURE)  /* O */
        {
        MDB(2,fAM) MLog("WARNING:  Unable to reserve funds for reservation %s - %s\n",
          R->Name,
          EMsg);

        return(FAILURE);
        }

      /* reserve successful */

      if (tmpA != NULL)
        {
        /* handle rsv modification, ie 2 proc rsv grows to 3 proc rsv */

        R->LienCost = RCost;
        }
      }  /* END BLOCK */

      return(SUCCESS);

      /*NOTREACHED*/

      break;

    case mamtNONE:

      return(SUCCESS);

      /*NOTREACHED*/

      break;

    default:

      return(SUCCESS);

      /*NOTREACHED*/

      break;
    }  /* END switch (AM->Type) */

  return(SUCCESS);
  }   /* END MAMAllocRReserve() */


/* END MAMAlloc.c */
