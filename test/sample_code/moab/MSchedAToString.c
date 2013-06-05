/* HEADER */

      
/**
 * @file MSchedAToString.c
 *
 * Contains: MSchedAToString
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 * Convert sched attribute to string.
 *
 * @see MSchedSetAttr() - peer - set scheduler attributes
 * @see MSchedToString() - parent
 *
 * @param S (I)
 * @param AIndex (I)
 * @param String (O)
 * @param DFormat (I)
 */

int MSchedAToString(

  msched_t *S,
  enum MSchedAttrEnum AIndex,
  mstring_t *String,
  enum MFormatModeEnum DFormat)

  {
  if (String == NULL)
    {
    return(FAILURE);
    }

  MStringSet(String,"\0");

  if (S == NULL)
    {
    return(FAILURE);
    }

  switch (AIndex)
    {
    case msaChargeMetricPolicy:

      if (S->ChargeMetric != mamcpNONE)
        MStringAppendF(String,"%s",MAMChargeMetricPolicy[S->ChargeMetric]);

      break;

    case msaChargeRatePolicy:

      if (S->ChargeRatePolicy != mamcrpNONE)
        MStringAppendF(String,"%s",MAMChargeRatePolicy[S->ChargeRatePolicy]);

      break;

    case msaCPVersion:

      MStringAppendF(String,"%s",MCP.WVersion);

      break;
     
    case msaEventCounter:

      MStringAppendF(String,"%d",
        S->EventCounter);

      break;
 
    case msaFBServer:

      if (S->AltServerHost[0] != '\0')
        {
        MStringAppend(String,S->AltServerHost);
        }

      break;

    case msaFlags:

      if (!bmisclear(&S->Flags))
        {
        bmtostring(&S->Flags,MSchedFlags,String);
        }

      break;

    case msaHTTPServerPort:

      MStringAppendF(String,"%d",
        S->HTTPServerPort);

      break;

    case msaIterationCount:

      MStringAppendF(String,"%d",
        S->Iteration);

      break;

    case msaLastCPTime:

      if (MCP.LastCPTime > 0)
        {
        MStringAppendF(String,"%ld",
          MCP.LastCPTime);
        }

      break;

    case msaLastTransCommitted:

      if (MSched.LastTransCommitted > 0)
        {
        MStringAppendF(String,"%ld",
          MSched.LastTransCommitted);
        }

      break;

    case msaLocalQueueIsActive:

      if (S->LocalQueueIsActive == TRUE)
        {
        MStringAppendF(String,"TRUE");
        }

      break;

    case msaMaxJobID:

      MStringAppendF(String,"%d",
        S->MaxJobCounter);

      break;

    case msaMaxRecordedCJobID:

      MStringAppendF(String,"%d",
        S->MaxRecordedCJobID);

      break;

    case msaMessage:

      {
      char tmpBuf[MMAX_BUFFER];

      tmpBuf[0] = '\0';

      if (S->MB == NULL)
        break;

      MMBPrintMessages(
        S->MB,
        (DFormat == mfmVerbose) ? mfmHuman : DFormat,
        TRUE,     /* verbose */
        -1,
        tmpBuf,      /* O */
        sizeof(tmpBuf));

      MStringAppend(String,tmpBuf);
      }

      break;

    case msaMinJobID:

      MStringAppendF(String,"%d",
        S->MinJobCounter);

      break;

    case msaMode:

      MStringAppendF(String,"%s",MSchedMode[S->Mode]);

      break;

    case msaName:

      MStringAppendF(String,"%s",S->Name);

      break;

    case msaRestartState:

      MStringAppendF(String,"%s",
        MSchedState[S->RestartState]);

      break;

    case msaRsvCounter:

      MStringAppendF(String,"%d",
        S->RsvCounter);

      break;

    case msaRsvGroupCounter:

      MStringAppendF(String,"%d",
        S->RsvGroupCounter);

      break;

    case msaServer:

      /* FORMAT: <URL> */

      MUURLCreate(NULL,S->ServerHost,NULL,S->ServerPort,String);

      break;
    
    case msaState:

      MStringAppendF(String,"%s","active");

      break;

    case msaStateMAdmin:

      if(S->StateMAdmin != NULL)
        MStringAppendF(String,"%s", S->StateMAdmin);

      break;

    case msaStateMTime:

      if(S->StateMTime > 0)
        MStringAppendF(String, "%lu", S->StateMTime);

      break;

    case msaTrigCounter:

      if (S->TrigCounter > 0)
        {
        MStringAppendF(String,"%d",
          S->TrigCounter);
        }

      break;

    case msaTransCounter:

      if (S->TransCounter > 0)
        {
        MStringAppendF(String,"%d",
          S->TransCounter);
        }

      break;

    case msaTrigger:

      if (S->TList == NULL)
        break;

      MTListToMString(S->TList,TRUE,String);

      break;

    default:

      /* NO-OP */

      break;
    }  /* END switch (AIndex) */

  return(SUCCESS);
  }  /* END MSchedAToString() */
/* END MSchedAToString.c */
