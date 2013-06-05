/* HEADER */


/**
 * @file MAMHandle.c
 *
 * Contains: MAM routines for handling Moab object stages. For example,
 *           call MAMHandleQuote to determine a hypothetical cost
 *           call MAMHandleCreate before a job or reservation is created
 *           call MAMHandleStart before a job or reservation is started
 *           call MAMHandleUpdate before a job or reservation is continued
 *           call MAMHandleEnd after a job or reservation is ended
 *           call MAMHandleDelete after a job or reservation goes out of scope
 */

#include "moab.h"
#include "moab-proto.h"
#include "moab-const.h"
#include "moab-global.h"
#include "mcom.h"


/**
 * Request a quote from the accounting manager regarding how much it would cost
 * if a new object were to be used for the specified amount of time. The object
 * may or may not already be created inside of Moab.
 *
 * @see MRsvGetCost() - parent
 * @see MRsvGetAllocCost() - parent
 * @see MAMHandleCreate() - peer
 * @see MAMHandleStart() - peer
 * @see MAMNativeDoCommand() - child
 *
 * @param AM       [I] - AM to use
 * @param O        [I] - The Object reference
 * @param OType    [I] - Type of Object (job, rsv, etc.)
 * @param CostP    [O] - Quoted cost amount (pointer to double)
 * @param Msg      [O] - Status message
 * @param S3C      [O] - SSS Status Code Decade
 */

int MAMHandleQuote(

  mam_t                  *AM,
  void                   *O,
  enum MXMLOTypeEnum      OType,
  double                 *CostP,
  char                   *Msg,
  enum MS3CodeDecadeEnum *S3C)

  {
  const char *FName = "MAMHandleQuote";
  char        subMsg[MMAX_LINE];
  char        Message[MMAX_LINE];
  char        EventMessage[MMAX_LINE];

  mstring_t   Response(MMAX_LINE);

  int         rc = SUCCESS;

  double      tmpCost;

  mxml_t     *QuoteE = NULL;
  mxml_t     *CostE = NULL;

  mrsv_t                 *R = NULL;

  subMsg[0] = '\0';

  MDB(3,fAM) MLog("%s(%s,%s,%s,%p,%p,%p)\n",
    FName,
    (AM != NULL) ? AM->Name : "NULL",
    (OType == mxoJob && O != NULL) ? ((mjob_t *)O)->Name
       : (OType == mxoRsv && O != NULL) ? ((mrsv_t *)O)->Name
       : "unknown",
    (OType > mxoNONE && OType < mxoLAST) ? MXO[OType] : "unknown",
    CostP,Msg,S3C);

  /* Initialize variables */

  if (OType == mxoRsv)
    R = (mrsv_t *)O;

  if (CostP != NULL)
    *CostP = 0.0;

  if (S3C != NULL)
    *S3C = ms3cNone;

  if (Msg != NULL)
    Msg[0] = '\0';

  /* Validate pre-conditions */

  if (AM == NULL)
    {
    strcpy(Message,"Invalid accounting manager specified");
    MDB(3,fAM) MLog("ERROR:    %s in %s\n",Message,FName);
    if (Msg != NULL) strcpy(Msg,Message);
    return(FAILURE);
    }

  if ((O == NULL) || (OType == mxoNONE))
    {
    strcpy(Message,"Invalid object specified");
    MDB(3,fAM) MLog("ERROR:    %s in %s\n",Message,FName);
    if (Msg != NULL) strcpy(Msg,Message);
    return(FAILURE);
    }

  if (AM->State == mrmsDown)
    {
    strcpy(Message,"Accounting manager is down");
    MDB(3,fAM) MLog("WARNING:  %s in %s\n",Message,FName);
    if (Msg != NULL) strcpy(Msg,Message);
    return(FAILURE);
    }

  /* Check to see if this call should be skipped */

  if (AM->Type == mamtNONE)
    {
    strcpy(Message,"AM quote request ignored -- ");
    strcat(Message,"Accounting manager not enabled");
    MDB(8,fAM) MLog("ALERT:    %s in %s\n",Message,FName);
    if (Msg != NULL) strcpy(Msg,Message);
    return(SUCCESS);
    }
    
  if ((MSched.Mode != msmNormal) &&(MAMTestMode != TRUE))
    {
    strcpy(Message,"AM quote request ignored -- ");
    strcat(Message,"Incompatible scheduler mode");
    MDB(8,fAM) MLog("ALERT:    %s in %s\n",Message,FName);
    if (Msg != NULL) strcpy(Msg,Message);
    return(SUCCESS);
    }
    
  if ((OType == mxoRsv) && (bmisset(&R->Flags,mrfNoCharge)))
    {
    strcpy(Message,"AM end request ignored -- ");
    strcat(Message,"No charging on reservations");
    MDB(8,fAM) MLog("ALERT:    %s in %s\n",Message,FName);
    if (Msg != NULL) strcpy(Msg,Message);
    return(SUCCESS);
    }

  /* Invoke the appropriate subroutine */

  switch (AM->Type)
    {
    case mamtNative:

      rc = FAILURE;

      /* Execute NAMI script */
      if ((MAMNativeDoCommand(AM,O,OType,mamnQuote,Msg,&Response,S3C) == FAILURE) || (Response.empty()))
        {
        if (Msg != NULL)
          {
          MDB(3,fAM) MLog("WARNING:  %s in %s\n",Msg,FName);
          }
        }

      /* Extract response */
      /* Do we need to use MStringFree on Response ? */
      else if (MXMLFromString(&QuoteE,Response.c_str(),NULL,subMsg) == FAILURE)
        {
        sprintf(Message,"Unable to parse XML (%s): %s",Response.c_str(),subMsg);
        MDB(3,fAM) MLog("ERROR:    %s in %s\n",Message,FName);
        if (Msg != NULL) strcpy(Msg,Message);
        }

      /* Parse out recurring cost */
      else if ((MXMLGetChildCI(QuoteE,"recurring",NULL,&CostE) == FAILURE) || (CostE->Val == NULL) || (CostE->Val[0] == '\0'))
        {
        strcpy(Message,"Unable to determine quote cost");
        MDB(3,fAM) MLog("ERROR:    %s in %s\n",Message,FName);
        if (Msg != NULL) strcpy(Msg,Message);
        }

      /* Check that quote amount is a valid float */
      else
        {
        char *leftover;
        tmpCost = strtod(CostE->Val,&leftover);
        if (*leftover != 0)
          {
          sprintf(Message,"Invalid quote amount (%s)",CostE->Val);
          MDB(3,fAM) MLog("ERROR:    %s in %s\n",Message,FName);
          if (Msg != NULL) strcpy(Msg,Message);
          }
        else
          {
          rc = SUCCESS;

          if (CostP != NULL)
            *CostP = tmpCost;
          }
        }

      break;

    case mamtMAM:
    case mamtGOLD:

      /* A quote function has not yet been implemented for the Gold iface */
      rc = SUCCESS;

      break;

    default:

      rc = SUCCESS;

      break;
    }  /* switch (AM->Type) */

  /* Log event record */

  sprintf(EventMessage,"%s registering %s quote (%s) with accounting manager",
    (rc == SUCCESS) ? "Success" : "Failure",
    (OType > mxoNONE && OType < mxoLAST) ? MXO[OType] : "unknown",
    (OType == mxoJob && O != NULL) ? ((mjob_t *)O)->Name
      : (OType == mxoRsv && O != NULL) ? ((mrsv_t *)O)->Name
      : "unknown"
    );

  if ((Msg != NULL) && (Msg[0] != '\0'))
    {
    strcat(EventMessage," -- ");
    strncat(EventMessage,Msg,sizeof(EventMessage) - strlen(EventMessage));
    }

  MOWriteEvent(O,mxoJob,mrelAMQuote,EventMessage,MStat.eventfp,NULL);

  return(rc);
  }  /* END MAMHandleQuote() */


/**
 * Check with accounting manager before the creation of a new object that will
 * be tracked in the accounting system. Depending on the use case, this might
 * check for sufficient funds or instantiate a usage record. A rejection by the
 * accounting manager may prevent the object being created and in the normal
 * case should send a warning or failure message to the user in response to
 * their create/commit/submit request. For a job, this might check for
 * sufficient funds. For a reservation, it might reserve funds.
 *
 * @see MUIJobSubmit() - parent
 * @see MUGenerateSystemJobs() - parent
 * @see MRMJobPostLoad() - parent
 * @see MAMHandleQuote() - peer
 * @see MAMHandleStart() - peer
 * @see MAMNativeDoCommand() - child
 * @see MAMAllocJReserve() - child
 * @see MAMAllocRReserve() - child
 *
 * @param AM       [I] - AM to use
 * @param O        [I] - The Object reference
 * @param OType    [I] - Type of Object (job, rsv, etc.)
 * @param FA       [O] - Failure Action guidance
 * @param Msg      [O] - Status message
 * @param S3C      [O] - SSS Status Code Decade
 */

int MAMHandleCreate(

  mam_t                  *AM,
  void                   *O,
  enum MXMLOTypeEnum      OType,
  enum MAMJFActionEnum   *FA,
  char                   *Msg,
  enum MS3CodeDecadeEnum *S3C)

  {
  const char             *FName = "MAMHandleCreate";
  char                    Message[MMAX_LINE];
  char                    EventMessage[MMAX_LINE];

  int                     rc = SUCCESS;

  enum MS3CodeDecadeEnum  tmpS3C;
  enum MS3CodeDecadeEnum *S3CP;

  mjob_t                 *J = NULL;
  mrsv_t                 *R = NULL;

  MDB(3,fAM) MLog("%s(%s,%s,%s,%p,%p,%p)\n",
    FName,
    (AM != NULL) ? AM->Name : "NULL",
    (OType == mxoJob && O != NULL) ? ((mjob_t *)O)->Name
       : (OType == mxoRsv && O != NULL) ? ((mrsv_t *)O)->Name
       : "unknown",
    (OType > mxoNONE && OType < mxoLAST) ? MXO[OType] : "unknown",
    FA,Msg,S3C);

  /* Initialize variables */

  if (OType == mxoJob)
    J = (mjob_t *)O;
  else if (OType == mxoRsv)
    R = (mrsv_t *)O;

  if (FA != NULL)
    *FA = mamjfaNONE;

  S3CP = (S3C != NULL) ? S3C : &tmpS3C;
  *S3CP = ms3cNone;

  if (Msg != NULL)
    Msg[0] = '\0';

  /* Validate pre-conditions */

  if (AM == NULL)
    {
    strcpy(Message,"Invalid accounting manager specified");
    MDB(3,fAM) MLog("ERROR:    %s in %s\n",Message,FName);
    if (Msg != NULL) strcpy(Msg,Message);
    return(FAILURE);
    }

  if ((O == NULL) || (OType == mxoNONE))
    {
    strcpy(Message,"Invalid object specified");
    MDB(3,fAM) MLog("ERROR:    %s in %s\n",Message,FName);
    if (Msg != NULL) strcpy(Msg,Message);
    return(FAILURE);
    }

  if (AM->State == mrmsDown)
    {
    strcpy(Message,"Accounting manager is down");
    MDB(3,fAM) MLog("WARNING:  %s in %s\n",Message,FName);
    if (Msg != NULL) strcpy(Msg,Message);
    *S3CP = ms3cClientResourceUnavailable;
    return(FAILURE);
    }

  /* Check to see if this call should be skipped */

  if (AM->Type == mamtNONE)
    {
    strcpy(Message,"AM create request ignored -- ");
    strcat(Message,"Accounting manager not enabled");
    MDB(8,fAM) MLog("ALERT:    %s in %s\n",Message,FName);
    if (Msg != NULL) strcpy(Msg,Message);
    return(SUCCESS);
    }

  if ((MSched.Mode != msmNormal) && (MAMTestMode != TRUE))
    {
    strcpy(Message,"AM create request ignored -- ");
    strcat(Message,"Incompatible scheduling mode");
    MDB(8,fAM) MLog("ALERT:    %s in %s\n",Message,FName);
    if (Msg != NULL) strcpy(Msg,Message);
    return(SUCCESS);
    }

  if ((OType == mxoJob) && (J->System != NULL))
    {
    strcpy(Message,"AM create request ignored -- ");
    strcat(Message,"System job");
    MDB(8,fAM) MLog("ALERT:    %s in %s\n",Message,FName);
    if (Msg != NULL) strcpy(Msg,Message);
    return(SUCCESS);
    }

  if ((OType == mxoJob) && (J->Credential.C != NULL) &&
      (J->Credential.C->DisableAM == TRUE))
    {
    strcpy(Message,"AM create request ignored -- ");
    strcat(Message,"Accounting disabled for class");
    MDB(8,fAM) MLog("ALERT:    %s in %s\n",Message,FName);
    if (Msg != NULL) strcpy(Msg,Message);
    return(SUCCESS);
    }

  if ((OType == mxoRsv) && (R->Type == mrtJob))
    {
    strcpy(Message,"AM create request ignored -- ");
    strcat(Message,"Job Reservation");
    MDB(8,fAM) MLog("ALERT:    %s in %s\n",Message,FName);
    if (Msg != NULL) strcpy(Msg,Message);
    return(SUCCESS);
    }

  if ((OType == mxoRsv) && (bmisset(&R->Flags,mrfNoCharge)))
    {
    strcpy(Message,"AM end request ignored -- ");
    strcat(Message,"No charging on reservations");
    MDB(8,fAM) MLog("ALERT:    %s in %s\n",Message,FName);
    if (Msg != NULL) strcpy(Msg,Message);
    return(SUCCESS);
    }

  if ((OType == mxoRsv) && ((R->A == NULL) || !strcmp(R->A->Name,NONE)))
    {
    strcpy(Message,"AM create request ignored -- ");
    strcat(Message,"Reservation has no accountable account");
    MDB(8,fAM) MLog("ALERT:    %s in %s\n",Message,FName);
    if (Msg != NULL) strcpy(Msg,Message);
    return(SUCCESS);
    }

  if ((OType == mxoRsv) && (AM->IsChargeExempt[mxoRsv] == TRUE))
    {
    strcpy(Message,"AM create request ignored -- ");
    strcat(Message,"Reservations are exempt from charges");
    MDB(8,fAM) MLog("ALERT:    %s in %s\n",Message,FName);
    if (Msg != NULL) strcpy(Msg,Message);
    return(SUCCESS);
    }

  /* Invoke the appropriate subroutine */

  switch (AM->Type)
    {
    case mamtNative:

      rc = MAMNativeDoCommand(AM,O,OType,mamnCreate,Msg,NULL,S3CP);

      break;

    case mamtMAM:
    case mamtGOLD:
    case mamtFILE:

      if (OType == mxoJob)
        rc = MAMAllocJReserve(AM,J,TRUE,S3CP,Msg);
      else if (OType == mxoRsv)
        rc = MAMAllocRReserve(AM,R,S3CP,Msg);

      break;

    default:

      break;
    }     /* switch (AM->Type) */

  /* Log event record */
  if (rc == FAILURE)
    {
    if (FA != NULL)
      {
      if (*S3CP == ms3cNoFunds)
        {
        *FA = AM->CreateFailureNFAction;
        }
      else
        {
        *FA = AM->CreateFailureAction;
        }
      }
    }

  /* Log event record */

  sprintf(EventMessage,"%s registering %s create (%s) with accounting manager",
    (rc == SUCCESS) ? "Success" : "Failure",
    (OType > mxoNONE && OType < mxoLAST) ? MXO[OType] : "unknown",
    (OType == mxoJob && O != NULL) ? ((mjob_t *)O)->Name
      : (OType == mxoRsv && O != NULL) ? ((mrsv_t *)O)->Name
      : "unknown"
    );

  if ((Msg != NULL) && (Msg[0] != '\0'))
    {
    strcat(EventMessage," -- ");
    strncat(EventMessage,Msg,sizeof(EventMessage) - strlen(EventMessage));
    }

  MOWriteEvent(O,mxoJob,mrelAMCreate,EventMessage,MStat.eventfp,NULL);

  return(rc);
  }     /* END MAMHandleCreate() */


/**
 * Check with accounting manager before the starting a job or reservation.
 * Depending on the use case, this might create a lien. A rejection by the
 * accounting manager will set the failure action that should be taken by the
 * caller and should result in a message logged against the object.
 *
 * @see MJobStart() - parent
 * @see MJobSelectMNL() - parent
 * @see MRMJobUpdate() - parent
 * @see MRsvCreate() - parent
 * @see MUIRsvCreate() - parent
 * @see MRsvChargeAllocation() - parent
 * @see MAMHandleQuote() - peer
 * @see MAMHandleCreate() - peer
 * @see MAMHandleUpdate() - peer
 * @see MAMNativeDoCommand() - child
 * @see MAMAllocJReserve() - child
 *
 * @param AM       [I] - AM to use
 * @param O        [I] - The Object reference
 * @param OType    [I] - Type of Object (job, rsv, etc.)
 * @param FA       [O] - Failure Action guidance
 * @param Msg      [O] - Status message
 * @param S3C      [O] - SSS Status Code Decade
 */

int MAMHandleStart(

  mam_t                  *AM,
  void                   *O,
  enum MXMLOTypeEnum      OType,
  enum MAMJFActionEnum   *FA,
  char                   *Msg,
  enum MS3CodeDecadeEnum *S3C)

  {
  const char             *FName = "MAMHandleStart";
  char                   *MsgP;
  char                    tmpMsg[MMAX_LINE];
  char                    Message[MMAX_LINE];
  char                    EventMessage[MMAX_LINE];

  int                     rc = SUCCESS;

  enum MS3CodeDecadeEnum  tmpS3C;
  enum MS3CodeDecadeEnum *S3CP;

  mjob_t                 *J = NULL;
  mrsv_t                 *R = NULL;

  MDB(3,fAM) MLog("%s(%s,%s,%s,%p,%p,%p)\n",
    FName,
    (AM != NULL) ? AM->Name : "NULL",
    (OType == mxoJob && O != NULL) ? ((mjob_t *)O)->Name
       : (OType == mxoRsv && O != NULL) ? ((mrsv_t *)O)->Name
       : "unknown",
    (OType > mxoNONE && OType < mxoLAST) ? MXO[OType] : "unknown",
    FA,Msg,S3C);

  /* Initialize variables */

  if (OType == mxoJob)
    J = (mjob_t *)O;
  else if (OType == mxoRsv)
    R = (mrsv_t *)O;

  if (FA != NULL)
    *FA = mamjfaNONE;

  MsgP = (Msg != NULL) ? Msg : tmpMsg;
  MsgP[0] = '\0';

  S3CP = (S3C != NULL) ? S3C : &tmpS3C;
  *S3CP = ms3cNone;

  /* Validate pre-conditions */

  if (AM == NULL)
    {
    strcpy(Message,"Invalid accounting manager specified");
    MDB(3,fAM) MLog("ERROR:    %s in %s\n",Message,FName);
    strcpy(MsgP,Message);
    return(FAILURE);
    }

  if ((O == NULL) || (OType == mxoNONE))
    {
    strcpy(Message,"Invalid object specified");
    MDB(3,fAM) MLog("ERROR:    %s in %s\n",Message,FName);
    strcpy(MsgP,Message);
    return(FAILURE);
    }

  if (AM->State == mrmsDown)
    {
    strcpy(Message,"Accounting manager is down");
    MDB(3,fAM) MLog("WARNING:  %s in %s\n",Message,FName);
    strcpy(MsgP,Message);
    *S3CP = ms3cClientResourceUnavailable;
    return(FAILURE);
    }

  /* Check to see if this call should be skipped */

  if (AM->Type == mamtNONE)
    {
    strcpy(Message,"AM start request ignored -- ");
    strcat(Message,"Accounting manager not enabled");
    MDB(8,fAM) MLog("ALERT:    %s in %s\n",Message,FName);
    strcpy(MsgP,Message);
    return(SUCCESS);
    }

  if ((MSched.Mode != msmNormal) && (MAMTestMode != TRUE))
    {
    strcpy(Message,"AM start request ignored -- ");
    strcat(Message,"Incompatible scheduling mode");
    MDB(8,fAM) MLog("ALERT:    %s in %s\n",Message,FName);
    strcpy(MsgP,Message);
    return(SUCCESS);
    }

  if ((OType == mxoJob) && (bmisset(&J->IFlags,mjifAMReserved)))
    {
    strcpy(Message,"AM start request ignored -- ");
    strcat(Message,"A lien has already been created");
    MDB(8,fAM) MLog("ALERT:    %s in %s\n",Message,FName);
    strcpy(MsgP,Message);
    return(SUCCESS);
    }

  if ((OType == mxoJob) && (J->System != NULL))
    {
    strcpy(Message,"AM start request ignored -- ");
    strcat(Message,"System job");
    MDB(8,fAM) MLog("ALERT:    %s in %s\n",Message,FName);
    strcpy(MsgP,Message);
    return(SUCCESS);
    }

  if ((OType == mxoJob) && (AM->FallbackQOS[0] != '\0') &&
      (J->Credential.Q != NULL) &&
        (!strcmp(J->Credential.Q->Name,AM->FallbackQOS)))
    {
    strcpy(Message,"AM start request ignored -- ");
    strcat(Message,"Fallback QOS should always succeed");
    MDB(8,fAM) MLog("ALERT:    %s in %s\n",Message,FName);
    strcpy(MsgP,Message);
    return(SUCCESS);
    }

  if ((OType == mxoJob) &&
      (J->Credential.C != NULL) && (J->Credential.C->DisableAM == TRUE))
    {
    strcpy(Message,"AM start request ignored -- ");
    strcat(Message,"Accounting disabled for class");
    MDB(8,fAM) MLog("ALERT:    %s in %s\n",Message,FName);
    strcpy(MsgP,Message);
    return(SUCCESS);
    }

  if ((OType == mxoRsv) && (R->Type == mrtJob))
    {
    strcpy(Message,"AM start request ignored -- ");
    strcat(Message,"Job Reservation");
    MDB(8,fAM) MLog("ALERT:    %s in %s\n",Message,FName);
    strcpy(MsgP,Message);
    return(SUCCESS);
    }

  if ((OType == mxoRsv) && (bmisset(&R->Flags,mrfNoCharge)))
    {
    strcpy(Message,"AM end request ignored -- ");
    strcat(Message,"No charging on reservations");
    MDB(8,fAM) MLog("ALERT:    %s in %s\n",Message,FName);
    if (Msg != NULL) strcpy(Msg,Message);
    return(SUCCESS);
    }

  if ((OType == mxoRsv) && ((R->A == NULL) || !strcmp(R->A->Name,NONE)))
    {
    strcpy(Message,"AM create request ignored -- ");
    strcat(Message,"Reservation has no accountable account");
    MDB(8,fAM) MLog("ALERT:    %s in %s\n",Message,FName);
    strcpy(MsgP,Message);
    return(SUCCESS);
    }

  if ((OType == mxoRsv) && (AM->IsChargeExempt[mxoRsv] == TRUE))
    {
    strcpy(Message,"AM start request ignored -- ");
    strcat(Message,"Reservations are exempt from charges");
    MDB(8,fAM) MLog("ALERT:    %s in %s\n",Message,FName);
    strcpy(MsgP,Message);
    return(SUCCESS);
    }

  /* Invoke the appropriate subroutine */
  switch (AM->Type)
    {
    case mamtNative:

      rc = MAMNativeDoCommand(AM,O,OType,mamnStart,MsgP,NULL,S3CP);

      break;

    case mamtMAM:
    case mamtGOLD:
    case mamtFILE:

      if (OType == mxoJob)
        rc = MAMAllocJReserve(AM,J,FALSE,S3CP,MsgP);

      break;

    default:

      break;
    }     /* switch (AM->Type) */

  /* If start call failed */
  if (rc == FAILURE)
    {
    /* Set failure action */
    if (FA != NULL)
      {
      if (*S3CP == ms3cNoFunds)
        {
        *FA = AM->StartFailureNFAction;
        }
      else
        {
        *FA = AM->StartFailureAction;
        }
      }

    /* Log an error message to the object */
    if (MsgP[0] != '\0')
      {
      if (OType == mxoJob)
        {
        MJobSetAttr(J,mjaMessages,(void **)MsgP,mdfString,mSet);
        }
      else if (OType == mxoRsv)
        {
        MRsvSetAttr(R,mraMessages,(void **)MsgP,mdfString,mSet);
        }
      }
    }

  /* Log event record */

  sprintf(EventMessage,"%s registering %s start (%s) with accounting manager",
    (rc == SUCCESS) ? "Success" : "Failure",
    (OType > mxoNONE && OType < mxoLAST) ? MXO[OType] : "unknown",
    (OType == mxoJob && O != NULL) ? ((mjob_t *)O)->Name
      : (OType == mxoRsv && O != NULL) ? ((mrsv_t *)O)->Name
      : "unknown"
    );

  if (MsgP[0] != '\0')
    {
    strcat(EventMessage," -- ");
    strncat(EventMessage,MsgP,sizeof(EventMessage) - strlen(EventMessage));
    }

  MOWriteEvent(O,mxoJob,mrelAMStart,EventMessage,MStat.eventfp,NULL);

  return(rc);
  }     /* END MAMHandleStart() */


/**
 * Check with accounting manager at each charge flush interval before allowing
 * a job or reservation to continue running. Depending on the use case, this
 * might charge for the preceding interval and create a lien for the next, or
 * it might simply update the usage record. A rejection by the accounting
 * manager will set the failure action that should be taken by the caller and
 * should result in a message logged against the object.
 *
 * @see MRsvChargeAllocation() - parent
 * @see MJobDoPeriodicCharges() - parent
 * @see MAMHandleStart() - peer
 * @see MAMHandleEnd() - peer
 * @see MAMNativeDoCommand() - child
 * @see MAMAllocJReserve() - child
 * @see MAMAllocRReserve() - child
 * @see MAMAllocJDebit() - child
 * @see MAMAllocRDebit() - child
 *
 * @param AM       [I] - AM to use
 * @param O        [I] - The Object reference
 * @param OType    [I] - Type of Object (job, rsv, etc.)
 * @param FA       [O] - Failure Action guidance
 * @param Msg      [O] - Status message
 * @param S3C      [O] - SSS Status Code Decade
 */

int MAMHandleUpdate(

  mam_t                  *AM,
  void                   *O,
  enum MXMLOTypeEnum      OType,
  enum MAMJFActionEnum   *FA,
  char                   *Msg,
  enum MS3CodeDecadeEnum *S3C)

  {
  const char             *FName = "MAMHandleUpdate";
  char                   *MsgP;
  char                    tmpMsg[MMAX_LINE];
  char                    Message[MMAX_LINE];
  char                    EventMessage[MMAX_LINE];

  int                     rc = SUCCESS;

  enum MS3CodeDecadeEnum  tmpS3C;
  enum MS3CodeDecadeEnum *S3CP;

  mjob_t                 *J = NULL;
  mrsv_t                 *R = NULL;

  MDB(3,fAM) MLog("%s(%s,%s,%s,%p,%p,%p)\n",
    FName,
    (AM != NULL) ? AM->Name : "NULL",
    (OType == mxoJob && O != NULL) ? ((mjob_t *)O)->Name
       : (OType == mxoRsv && O != NULL) ? ((mrsv_t *)O)->Name
       : "unknown",
    (OType > mxoNONE && OType < mxoLAST) ? MXO[OType] : "unknown",
    FA,Msg,S3C);

  /* Initialize Variables */

  if (OType == mxoJob)
    J = (mjob_t *)O;
  else if (OType == mxoRsv)
    R = (mrsv_t *)O;

  if (FA != NULL)
    *FA = mamjfaNONE;

  MsgP = (Msg != NULL) ? Msg : tmpMsg;
  MsgP[0] = '\0';

  S3CP = (S3C != NULL) ? S3C : &tmpS3C;
  *S3CP = ms3cNone;

  /* Validate pre-conditions */

  if (AM == NULL)
    {
    strcpy(Message,"Invalid accounting manager specified");
    MDB(3,fAM) MLog("ERROR:    %s in %s\n",Message,FName);
    strcpy(MsgP,Message);
    return(FAILURE);
    }

  if ((O == NULL) || (OType == mxoNONE))
    {
    strcpy(Message,"Invalid object specified");
    MDB(3,fAM) MLog("ERROR:    %s in %s\n",Message,FName);
    strcpy(MsgP,Message);
    return(FAILURE);
    }

  if (AM->State == mrmsDown)
    {
    strcpy(Message,"Accounting manager is down");
    MDB(3,fAM) MLog("WARNING:  %s in %s\n",Message,FName);
    strcpy(MsgP,Message);
    *S3CP = ms3cClientResourceUnavailable;
    return(FAILURE);
    }

  /* Check to see if this call should be skipped */

  if (AM->Type == mamtNONE)
    {
    strcpy(Message,"AM update request ignored -- ");
    strcat(Message,"Accounting manager not enabled");
    MDB(8,fAM) MLog("ALERT:    %s in %s\n",Message,FName);
    strcpy(MsgP,Message);
    return(SUCCESS);
    }

  if ((MSched.Mode != msmNormal) && (MAMTestMode != TRUE))
    {
    strcpy(Message,"AM update request ignored -- ");
    strcat(Message,"Incompatible scheduling mode");
    MDB(8,fAM) MLog("ALERT:    %s in %s\n",Message,FName);
    strcpy(MsgP,Message);
    return(SUCCESS);
    }

  if ((OType == mxoJob) && (J->System != NULL))
    {
    strcpy(Message,"AM update request ignored -- ");
    strcat(Message,"System job");
    MDB(8,fAM) MLog("ALERT:    %s in %s\n",Message,FName);
    strcpy(MsgP,Message);
    return(SUCCESS);
    }

  if ((OType == mxoJob) && (AM->FallbackQOS[0] != '\0') &&
      (J->Credential.Q != NULL) &&
        (!strcmp(J->Credential.Q->Name,AM->FallbackQOS)))
    {
    strcpy(Message,"AM update request ignored -- ");
    strcat(Message,"Fallback QOS should always succeed");
    MDB(8,fAM) MLog("ALERT:    %s in %s\n",Message,FName);
    strcpy(MsgP,Message);
    return(SUCCESS);
    }

  if ((OType == mxoJob) &&
      (J->Credential.C != NULL) && (J->Credential.C->DisableAM == TRUE))
    {
    strcpy(Message,"AM update request ignored -- ");
    strcat(Message,"Accounting disabled for class");
    MDB(8,fAM) MLog("ALERT:    %s in %s\n",Message,FName);
    strcpy(MsgP,Message);
    return(SUCCESS);
    }

  if ((OType == mxoRsv) && (R->Type == mrtJob))
    {
    strcpy(Message,"AM update request ignored -- ");
    strcat(Message,"Job Reservation");
    MDB(8,fAM) MLog("ALERT:    %s in %s\n",Message,FName);
    strcpy(MsgP,Message);
    return(SUCCESS);
    }

  if ((OType == mxoRsv) && (bmisset(&R->Flags,mrfNoCharge)))
    {
    strcpy(Message,"AM end request ignored -- ");
    strcat(Message,"No charging on reservations");
    MDB(8,fAM) MLog("ALERT:    %s in %s\n",Message,FName);
    if (Msg != NULL) strcpy(Msg,Message);
    return(SUCCESS);
    }

  if ((OType == mxoRsv) && ((R->A == NULL) || !strcmp(R->A->Name,NONE)))
    {
    strcpy(Message,"AM update request ignored -- ");
    strcat(Message,"Reservation has no accountable account");
    MDB(8,fAM) MLog("ALERT:    %s in %s\n",Message,FName);
    strcpy(MsgP,Message);
    return(SUCCESS);
    }

  if ((OType == mxoRsv) && (AM->IsChargeExempt[mxoRsv] == TRUE))
    {
    strcpy(Message,"AM update request ignored -- ");
    strcat(Message,"Reservations are exempt from charges");
    MDB(8,fAM) MLog("ALERT:    %s in %s\n",Message,FName);
    strcpy(MsgP,Message);
    return(SUCCESS);
    }

  /* Invoke the appropriate subroutine */
  switch (AM->Type)
    {
    case mamtNative:

      rc = MAMNativeDoCommand(AM,O,OType,mamnUpdate,MsgP,NULL,S3CP);

      /* Update last charge time on success or failure since the charge
       * may have succeeded while the lien failed. We may later improve this
       * to determine whether it is a partial success and act accordingly */
      if (OType == mxoJob)
        J->LastChargeTime = MSched.Time;
      else if (OType == mxoRsv)
        R->LastChargeTime = MSched.Time;

      break;

    case mamtMAM:
    case mamtGOLD:
    case mamtFILE:

      if (OType == mxoJob)
        {
        rc = MAMAllocJDebit(AM,J,S3CP,MsgP);
        /* J->LastChargeTime is set on success in MAMAllocJDebit */

        if (rc == SUCCESS)
          rc = MAMAllocJReserve(AM,J,FALSE,S3CP,MsgP);
        }
      else if (OType == mxoRsv)
        {
        rc = MAMAllocRDebit(AM,R,TRUE,S3CP,MsgP);
        /* R->LastChargeTime is set on success in MAMAllocRDebit */

        if (rc == SUCCESS)
          rc = MAMAllocRReserve(AM,R,S3CP,MsgP);
        }

      break;

    default:

      break;
    }     /* switch (AM->Type) */

  /* If update call failed */
  if (rc == FAILURE)
    {
    /* Set failure action */
    if (FA != NULL)
      {
      if (*S3CP == ms3cNoFunds)
        {
        *FA = AM->UpdateFailureNFAction;
        }
      else
        {
        *FA = AM->UpdateFailureAction;
        }
      }

    /* Log an error message to the object */
    if (MsgP[0] != '\0')
      {
      if (OType == mxoJob)
        {
        MJobSetAttr(J,mjaMessages,(void **)MsgP,mdfString,mSet);
        }
      else if (OType == mxoRsv)
        {
        MRsvSetAttr(R,mraMessages,(void **)MsgP,mdfString,mSet);
        }
      }
    }

  /* Log event record */

  sprintf(EventMessage,"%s registering %s update (%s) with accounting manager",
    (rc == SUCCESS) ? "Success" : "Failure",
    (OType > mxoNONE && OType < mxoLAST) ? MXO[OType] : "unknown",
    (OType == mxoJob && O != NULL) ? ((mjob_t *)O)->Name
      : (OType == mxoRsv && O != NULL) ? ((mrsv_t *)O)->Name
      : "unknown"
    );

  if (MsgP[0] != '\0')
    {
    strcat(EventMessage," -- ");
    strncat(EventMessage,MsgP,sizeof(EventMessage) - strlen(EventMessage));
    }

  MOWriteEvent(O,mxoJob,mrelAMUpdate,EventMessage,MStat.eventfp,NULL);

  return(rc);
  }     /* END MAMHandleUpdate() */


/**
 * Update the accounting manager after pausing usage for a job or reservation.
 * Depending on the use case, this might charge for the preceding interval
 * without creating a lien for the next, or it might simply update the usage
 * record. A rejection by the accounting manager should result in a message
 * logged against the object.
 *
 * @see MJobPreempt() - parent
 * @see MAMHandleStart() - peer
 * @see MAMHandleResume() - peer
 * @see MAMNativeDoCommand() - child
 * @see MAMAllocJDebit() - child
 * @see MAMAllocRDebit() - child
 *
 * @param AM       [I] - AM to use
 * @param O        [I] - The Object reference
 * @param OType    [I] - Type of Object (job, rsv, etc.)
 * @param Msg      [O] - Status message
 * @param S3C      [O] - SSS Status Code Decade
 */

int MAMHandlePause(

  mam_t                  *AM,
  void                   *O,
  enum MXMLOTypeEnum      OType,
  char                   *Msg,
  enum MS3CodeDecadeEnum *S3C)

  {
  const char             *FName = "MAMHandlePause";
  char                   *MsgP;
  char                    tmpMsg[MMAX_LINE];
  char                    Message[MMAX_LINE];
  char                    EventMessage[MMAX_LINE];

  int                     rc = SUCCESS;

  enum MS3CodeDecadeEnum  tmpS3C;
  enum MS3CodeDecadeEnum *S3CP;

  mjob_t                 *J = NULL;
  mrsv_t                 *R = NULL;

  MDB(3,fAM) MLog("%s(%s,%s,%s,%p,%p)\n",
    FName,
    (AM != NULL) ? AM->Name : "NULL",
    (OType == mxoJob && O != NULL) ? ((mjob_t *)O)->Name
       : (OType == mxoRsv && O != NULL) ? ((mrsv_t *)O)->Name
       : "unknown",
    (OType > mxoNONE && OType < mxoLAST) ? MXO[OType] : "unknown",
    Msg,S3C);

  /* Initialize Variables */

  if (OType == mxoJob)
    J = (mjob_t *)O;
  else if (OType == mxoRsv)
    R = (mrsv_t *)O;

  MsgP = (Msg != NULL) ? Msg : tmpMsg;
  MsgP[0] = '\0';

  S3CP = (S3C != NULL) ? S3C : &tmpS3C;
  *S3CP = ms3cNone;

  /* Validate pre-conditions */

  if (AM == NULL)
    {
    strcpy(Message,"Invalid accounting manager specified");
    MDB(3,fAM) MLog("ERROR:    %s in %s\n",Message,FName);
    strcpy(MsgP,Message);
    return(FAILURE);
    }

  if ((O == NULL) || (OType == mxoNONE))
    {
    strcpy(Message,"Invalid object specified");
    MDB(3,fAM) MLog("ERROR:    %s in %s\n",Message,FName);
    strcpy(MsgP,Message);
    return(FAILURE);
    }

  if (AM->State == mrmsDown)
    {
    strcpy(Message,"Accounting manager is down");
    MDB(3,fAM) MLog("WARNING:  %s in %s\n",Message,FName);
    strcpy(MsgP,Message);
    *S3CP = ms3cClientResourceUnavailable;
    return(FAILURE);
    }

  /* Check to see if this call should be skipped */

  if (AM->Type == mamtNONE)
    {
    strcpy(Message,"AM update request ignored -- ");
    strcat(Message,"Accounting manager not enabled");
    MDB(8,fAM) MLog("ALERT:    %s in %s\n",Message,FName);
    strcpy(MsgP,Message);
    return(SUCCESS);
    }

  if ((MSched.Mode != msmNormal) && (MAMTestMode != TRUE))
    {
    strcpy(Message,"AM update request ignored -- ");
    strcat(Message,"Incompatible scheduling mode");
    MDB(8,fAM) MLog("ALERT:    %s in %s\n",Message,FName);
    strcpy(MsgP,Message);
    return(SUCCESS);
    }

  if ((OType == mxoJob) && (J->System != NULL))
    {
    strcpy(Message,"AM update request ignored -- ");
    strcat(Message,"System job");
    MDB(8,fAM) MLog("ALERT:    %s in %s\n",Message,FName);
    strcpy(MsgP,Message);
    return(SUCCESS);
    }

  if ((OType == mxoJob) &&
      (J->Credential.C != NULL) && (J->Credential.C->DisableAM == TRUE))
    {
    strcpy(Message,"AM update request ignored -- ");
    strcat(Message,"Accounting disabled for class");
    MDB(8,fAM) MLog("ALERT:    %s in %s\n",Message,FName);
    strcpy(MsgP,Message);
    return(SUCCESS);
    }

  if ((OType == mxoRsv) && (R->Type == mrtJob))
    {
    strcpy(Message,"AM update request ignored -- ");
    strcat(Message,"Job Reservation");
    MDB(8,fAM) MLog("ALERT:    %s in %s\n",Message,FName);
    strcpy(MsgP,Message);
    return(SUCCESS);
    }

  if ((OType == mxoRsv) && (bmisset(&R->Flags,mrfNoCharge)))
    {
    strcpy(Message,"AM end request ignored -- ");
    strcat(Message,"No charging on reservations");
    MDB(8,fAM) MLog("ALERT:    %s in %s\n",Message,FName);
    if (Msg != NULL) strcpy(Msg,Message);
    return(SUCCESS);
    }

  if ((OType == mxoRsv) && ((R->A == NULL) || !strcmp(R->A->Name,NONE)))
    {
    strcpy(Message,"AM create request ignored -- ");
    strcat(Message,"Reservation has no accountable account");
    MDB(8,fAM) MLog("ALERT:    %s in %s\n",Message,FName);
    strcpy(MsgP,Message);
    return(SUCCESS);
    }

  if ((OType == mxoRsv) && (AM->IsChargeExempt[mxoRsv] == TRUE))
    {
    strcpy(Message,"AM update request ignored -- ");
    strcat(Message,"Reservations are exempt from charges");
    MDB(8,fAM) MLog("ALERT:    %s in %s\n",Message,FName);
    strcpy(MsgP,Message);
    return(SUCCESS);
    }

  /* Invoke the appropriate subroutine */
  switch (AM->Type)
    {
    case mamtNative:

      rc = MAMNativeDoCommand(AM,O,OType,mamnPause,MsgP,NULL,S3CP);

      /* Reset LastChargeTime so it will not be used in next charge */
      if (rc == SUCCESS)
        {
        if (OType == mxoJob)
          J->LastChargeTime = 0;
        else if (OType == mxoRsv)
          R->LastChargeTime = 0;
        }

      break;

    case mamtMAM:
    case mamtGOLD:
    case mamtFILE:

      if (OType == mxoJob)
        {
        rc = MAMAllocJDebit(AM,J,S3CP,MsgP);
        J->LastChargeTime = 0;
        }
      else if (OType == mxoRsv)
        {
        rc = MAMAllocRDebit(AM,R,TRUE,S3CP,MsgP);
        R->LastChargeTime = 0;
        }
      else
        {
        rc = SUCCESS;
        }

      break;

    default:

      break;
    }     /* switch (AM->Type) */

  /* If update call failed */
  if (rc == FAILURE)
    {
    /* Log an error message to the object */
    if (MsgP[0] != '\0')
      {
      if (OType == mxoJob)
        {
        MJobSetAttr(J,mjaMessages,(void **)MsgP,mdfString,mSet);
        }
      else if (OType == mxoRsv)
        {
        MRsvSetAttr(R,mraMessages,(void **)MsgP,mdfString,mSet);
        }
      }
    }

  /* Log event record */

  sprintf(EventMessage,"%s registering %s pause (%s) with accounting manager",
    (rc == SUCCESS) ? "Success" : "Failure",
    (OType > mxoNONE && OType < mxoLAST) ? MXO[OType] : "unknown",
    (OType == mxoJob && O != NULL) ? ((mjob_t *)O)->Name
      : (OType == mxoRsv && O != NULL) ? ((mrsv_t *)O)->Name
      : "unknown"
    );

  if (MsgP[0] != '\0')
    {
    strcat(EventMessage," -- ");
    strncat(EventMessage,MsgP,sizeof(EventMessage) - strlen(EventMessage));
    }

  MOWriteEvent(O,mxoJob,mrelAMPause,EventMessage,MStat.eventfp,NULL);

  return(rc);
  }     /* END MAMHandlePause() */


/**
 * Check with accounting manager before allowing a job or reservation to
 * resume running. Depending on the use case, this might create a lient,
 * or it might simply update the usage record. A rejection by the accounting
 * manager will set the failure action that should be taken by the caller and
 * should result in a message logged against the object.
 *
 * @see MAMHandleStart() - peer
 * @see MAMHandlePause() - peer
 * @see MAMNativeDoCommand() - child
 * @see MAMAllocJReserve() - child
 * @see MAMAllocRReserve() - child
 *
 * @param AM       [I] - AM to use
 * @param O        [I] - The Object reference
 * @param OType    [I] - Type of Object (job, rsv, etc.)
 * @param FA       [O] - Failure Action guidance
 * @param Msg      [O] - Status message
 * @param S3C      [O] - SSS Status Code Decade
 */

int MAMHandleResume(

  mam_t                  *AM,
  void                   *O,
  enum MXMLOTypeEnum      OType,
  enum MAMJFActionEnum   *FA,
  char                   *Msg,
  enum MS3CodeDecadeEnum *S3C)

  {
  const char             *FName = "MAMHandleResume";
  char                   *MsgP;
  char                    tmpMsg[MMAX_LINE];
  char                    Message[MMAX_LINE];
  char                    EventMessage[MMAX_LINE];

  int                     rc = SUCCESS;

  enum MS3CodeDecadeEnum  tmpS3C;
  enum MS3CodeDecadeEnum *S3CP;

  mjob_t                 *J = NULL;
  mrsv_t                 *R = NULL;

  MDB(3,fAM) MLog("%s(%s,%s,%s,%p,%p,%p)\n",
    FName,
    (AM != NULL) ? AM->Name : "NULL",
    (OType == mxoJob && O != NULL) ? ((mjob_t *)O)->Name
       : (OType == mxoRsv && O != NULL) ? ((mrsv_t *)O)->Name
       : "unknown",
    (OType > mxoNONE && OType < mxoLAST) ? MXO[OType] : "unknown",
    FA,Msg,S3C);

  /* Initialize Variables */

  if (OType == mxoJob)
    J = (mjob_t *)O;
  else if (OType == mxoRsv)
    R = (mrsv_t *)O;

  if (FA != NULL)
    *FA = mamjfaNONE;

  MsgP = (Msg != NULL) ? Msg : tmpMsg;
  MsgP[0] = '\0';

  S3CP = (S3C != NULL) ? S3C : &tmpS3C;
  *S3CP = ms3cNone;

  /* Validate pre-conditions */

  if (AM == NULL)
    {
    strcpy(Message,"Invalid accounting manager specified");
    MDB(3,fAM) MLog("ERROR:    %s in %s\n",Message,FName);
    strcpy(MsgP,Message);
    return(FAILURE);
    }

  if ((O == NULL) || (OType == mxoNONE))
    {
    strcpy(Message,"Invalid object specified");
    MDB(3,fAM) MLog("ERROR:    %s in %s\n",Message,FName);
    strcpy(MsgP,Message);
    return(FAILURE);
    }

  if (AM->State == mrmsDown)
    {
    strcpy(Message,"Accounting manager is down");
    MDB(3,fAM) MLog("WARNING:  %s in %s\n",Message,FName);
    strcpy(MsgP,Message);
    *S3CP = ms3cClientResourceUnavailable;
    return(FAILURE);
    }

  /* Check to see if this call should be skipped */

  if (AM->Type == mamtNONE)
    {
    strcpy(Message,"AM update request ignored -- ");
    strcat(Message,"Accounting manager not enabled");
    MDB(8,fAM) MLog("ALERT:    %s in %s\n",Message,FName);
    strcpy(MsgP,Message);
    return(SUCCESS);
    }

  if ((MSched.Mode != msmNormal) && (MAMTestMode != TRUE))
    {
    strcpy(Message,"AM update request ignored -- ");
    strcat(Message,"Incompatible scheduling mode");
    MDB(8,fAM) MLog("ALERT:    %s in %s\n",Message,FName);
    strcpy(MsgP,Message);
    return(SUCCESS);
    }

  if ((OType == mxoJob) && (J->System != NULL))
    {
    strcpy(Message,"AM update request ignored -- ");
    strcat(Message,"System job");
    MDB(8,fAM) MLog("ALERT:    %s in %s\n",Message,FName);
    strcpy(MsgP,Message);
    return(SUCCESS);
    }

  if ((OType == mxoJob) && (AM->FallbackQOS[0] != '\0') &&
      (J->Credential.Q != NULL) &&
        (!strcmp(J->Credential.Q->Name,AM->FallbackQOS)))
    {
    strcpy(Message,"AM update request ignored -- ");
    strcat(Message,"Fallback QOS should always succeed");
    MDB(8,fAM) MLog("ALERT:    %s in %s\n",Message,FName);
    strcpy(MsgP,Message);
    return(SUCCESS);
    }

  if ((OType == mxoJob) &&
      (J->Credential.C != NULL) && (J->Credential.C->DisableAM == TRUE))
    {
    strcpy(Message,"AM update request ignored -- ");
    strcat(Message,"Accounting disabled for class");
    MDB(8,fAM) MLog("ALERT:    %s in %s\n",Message,FName);
    strcpy(MsgP,Message);
    return(SUCCESS);
    }

  if ((OType == mxoRsv) && (R->Type == mrtJob))
    {
    strcpy(Message,"AM update request ignored -- ");
    strcat(Message,"Job Reservation");
    MDB(8,fAM) MLog("ALERT:    %s in %s\n",Message,FName);
    strcpy(MsgP,Message);
    return(SUCCESS);
    }

  if ((OType == mxoRsv) && (bmisset(&R->Flags,mrfNoCharge)))
    {
    strcpy(Message,"AM end request ignored -- ");
    strcat(Message,"No charging on reservations");
    MDB(8,fAM) MLog("ALERT:    %s in %s\n",Message,FName);
    if (Msg != NULL) strcpy(Msg,Message);
    return(SUCCESS);
    }

  if ((OType == mxoRsv) && ((R->A == NULL) || !strcmp(R->A->Name,NONE)))
    {
    strcpy(Message,"AM create request ignored -- ");
    strcat(Message,"Reservation has no accountable account");
    MDB(8,fAM) MLog("ALERT:    %s in %s\n",Message,FName);
    strcpy(MsgP,Message);
    return(SUCCESS);
    }

  if ((OType == mxoRsv) && (AM->IsChargeExempt[mxoRsv] == TRUE))
    {
    strcpy(Message,"AM update request ignored -- ");
    strcat(Message,"Reservations are exempt from charges");
    MDB(8,fAM) MLog("ALERT:    %s in %s\n",Message,FName);
    strcpy(MsgP,Message);
    return(SUCCESS);
    }

  /* Invoke the appropriate subroutine */
  switch (AM->Type)
    {
    case mamtNative:

      /* Set last charge time so that the next debit charges from this point */
      if (OType == mxoJob)
        J->LastChargeTime = MSched.Time;
      else if (OType == mxoRsv)
        R->LastChargeTime = MSched.Time;

      rc = MAMNativeDoCommand(AM,O,OType,mamnResume,MsgP,NULL,S3CP);

      break;

    case mamtMAM:
    case mamtGOLD:
    case mamtFILE:

      if (OType == mxoJob)
        {
        /* Set last charge time so the next debit charges from this point */
        J->LastChargeTime = MSched.Time;

        rc = MAMAllocJReserve(AM,J,FALSE,S3CP,MsgP);
        }
      else if (OType == mxoRsv)
        {
        /* Set last charge time so the next debit charges from this point */
        R->LastChargeTime = MSched.Time;

        rc = MAMAllocRReserve(AM,R,S3CP,MsgP);
        }

      break;

    default:

      break;
    }     /* switch (AM->Type) */

  /* If update call failed */
  if (rc == FAILURE)
    {
    /* Set failure action */
    if (FA != NULL)
      {
      if (*S3CP == ms3cNoFunds)
        {
        *FA = AM->ResumeFailureNFAction;
        }
      else
        {
        *FA = AM->ResumeFailureAction;
        }
      }

    /* Log an error message to the object */
    if (MsgP[0] != '\0')
      {
      if (OType == mxoJob)
        {
        MJobSetAttr(J,mjaMessages,(void **)MsgP,mdfString,mSet);
        }
      else if (OType == mxoRsv)
        {
        MRsvSetAttr(R,mraMessages,(void **)MsgP,mdfString,mSet);
        }
      }
    }

  /* Log event record */

  sprintf(EventMessage,"%s registering %s resume (%s) with accounting manager",
    (rc == SUCCESS) ? "Success" : "Failure",
    (OType > mxoNONE && OType < mxoLAST) ? MXO[OType] : "unknown",
    (OType == mxoJob && O != NULL) ? ((mjob_t *)O)->Name
      : (OType == mxoRsv && O != NULL) ? ((mrsv_t *)O)->Name
      : "unknown"
    );

  if (MsgP[0] != '\0')
    {
    strcat(EventMessage," -- ");
    strncat(EventMessage,MsgP,sizeof(EventMessage) - strlen(EventMessage));
    }

  MOWriteEvent(O,mxoJob,mrelAMResume,EventMessage,MStat.eventfp,NULL);

  return(rc);
  }     /* END MAMHandleResume() */


/**
 * Notify accounting manager after a job or reservation ends.
 * Depending on the use case, this might make a final charge and clean up
 * liens, or it might simply update the usage record. A failure should result
 * in a message logged against the object.
 *
 * @see MJobProcessCompleted() - parent
 * @see MJobProcessRemoved() - parent
 * @see MJobProcessTerminated() - parent
 * @see MRsvChargeAllocation() - parent
 * @see MRsvConfigure() - parent
 * @see MAMHandleUpdate() - peer
 * @see MAMHandleDelete() - peer
 * @see MAMNativeDoCommand() - child
 * @see MAMAllocJDebit() - child
 * @see MAMAllocRDebit() - child
 *
 * @param AM       [I] - AM to use
 * @param O        [I] - The Object reference
 * @param OType    [I] - Type of Object (job, rsv, etc.)
 * @param Msg      [O] - Status message
 * @param S3C      [O] - SSS Status Code Decade
 */

int MAMHandleEnd(

  mam_t                  *AM,
  void                   *O,
  enum MXMLOTypeEnum      OType,
  char                   *Msg,
  enum MS3CodeDecadeEnum *S3C)

  {
  const char             *FName = "MAMHandleEnd";
  char                   *MsgP;
  char                    tmpMsg[MMAX_LINE];
  char                    Message[MMAX_LINE];
  char                    EventMessage[MMAX_LINE];

  int                     rc = SUCCESS;

  enum MS3CodeDecadeEnum  tmpS3C;
  enum MS3CodeDecadeEnum *S3CP;

  mjob_t                 *J = NULL;
  mrsv_t                 *R = NULL;

  MDB(3,fAM) MLog("%s(%s,%s,%s,%p,%p)\n",
    FName,
    (AM != NULL) ? AM->Name : "NULL",
    (OType == mxoJob && O != NULL) ? ((mjob_t *)O)->Name
       : (OType == mxoRsv && O != NULL) ? ((mrsv_t *)O)->Name
       : "unknown",
    (OType > mxoNONE && OType < mxoLAST) ? MXO[OType] : "unknown",
    Msg,S3C);

  /* Initialize Variables */

  if (OType == mxoJob)
    J = (mjob_t *)O;
  else if (OType == mxoRsv)
    R = (mrsv_t *)O;

  MsgP = (Msg != NULL) ? Msg : tmpMsg;
  MsgP[0] = '\0';

  S3CP = (S3C != NULL) ? S3C : &tmpS3C;
  *S3CP = ms3cNone;

  /* Validate pre-conditions */

  if (AM == NULL)
    {
    strcpy(Message,"Invalid accounting manager specified");
    MDB(3,fAM) MLog("ERROR:    %s in %s\n",Message,FName);
    strcpy(MsgP,Message);
    return(FAILURE);
    }

  if ((O == NULL) || (OType == mxoNONE))
    {
    strcpy(Message,"Invalid object specified");
    MDB(3,fAM) MLog("ERROR:    %s in %s\n",Message,FName);
    strcpy(MsgP,Message);
    return(FAILURE);
    }

  if (AM->State == mrmsDown)
    {
    strcpy(Message,"Accounting manager is down");
    MDB(3,fAM) MLog("WARNING:  %s in %s\n",Message,FName);
    strcpy(MsgP,Message);
    *S3CP = ms3cClientResourceUnavailable;
    return(FAILURE);
    }

  /* Check to see if this call should be skipped */

  if (AM->Type == mamtNONE)
    {
    strcpy(Message,"AM end request ignored -- ");
    strcat(Message,"Accounting manager not enabled");
    MDB(8,fAM) MLog("ALERT:    %s in %s\n",Message,FName);
    strcpy(MsgP,Message);
    return(SUCCESS);
    }

  if ((MSched.Mode != msmNormal) && (MAMTestMode != TRUE))
    {
    strcpy(Message,"AM end request ignored -- ");
    strcat(Message,"Incompatible scheduling mode");
    MDB(8,fAM) MLog("ALERT:    %s in %s\n",Message,FName);
    strcpy(MsgP,Message);
    return(SUCCESS);
    }

  if ((OType == mxoJob) && (J->System != NULL))
    {
    strcpy(Message,"AM end request ignored -- ");
    strcat(Message,"System job");
    MDB(8,fAM) MLog("ALERT:    %s in %s\n",Message,FName);
    strcpy(MsgP,Message);
    return(SUCCESS);
    }

  if ((OType == mxoJob) &&
      (J->Credential.C != NULL) && (J->Credential.C->DisableAM == TRUE))
    {
    strcpy(Message,"AM end request ignored -- ");
    strcat(Message,"Accounting disabled for class");
    MDB(8,fAM) MLog("ALERT:    %s in %s\n",Message,FName);
    strcpy(MsgP,Message);
    return(SUCCESS);
    }

  if ((OType == mxoRsv) && (R->Type == mrtJob))
    {
    strcpy(Message,"AM end request ignored -- ");
    strcat(Message,"Job Reservation");
    MDB(8,fAM) MLog("ALERT:    %s in %s\n",Message,FName);
    strcpy(MsgP,Message);
    return(SUCCESS);
    }

  if ((OType == mxoRsv) && (bmisset(&R->Flags,mrfNoCharge)))
    {
    strcpy(Message,"AM end request ignored -- ");
    strcat(Message,"No charging on reservations");
    MDB(8,fAM) MLog("ALERT:    %s in %s\n",Message,FName);
    if (Msg != NULL) strcpy(Msg,Message);
    return(SUCCESS);
    }

  if ((OType == mxoRsv) && ((R->A == NULL) || !strcmp(R->A->Name,NONE)))
    {
    strcpy(Message,"AM create request ignored -- ");
    strcat(Message,"Reservation has no accountable account");
    MDB(8,fAM) MLog("ALERT:    %s in %s\n",Message,FName);
    strcpy(MsgP,Message);
    return(SUCCESS);
    }

  if ((OType == mxoRsv) && (AM->IsChargeExempt[mxoRsv] == TRUE))
    {
    strcpy(Message,"AM end request ignored -- ");
    strcat(Message,"Reservations are exempt from charges");
    MDB(8,fAM) MLog("ALERT:    %s in %s\n",Message,FName);
    strcpy(MsgP,Message);
    return(SUCCESS);
    }

  /* Invoke the appropriate subroutine */
  switch (AM->Type)
    {
    case mamtNative:

      rc = MAMNativeDoCommand(AM,O,OType,mamnEnd,MsgP,NULL,S3CP);

      if (OType == mxoJob)
        J->LastChargeTime = MSched.Time;
      else if (OType == mxoRsv)
        R->LastChargeTime = MSched.Time;

      break;

    case mamtMAM:
    case mamtGOLD:
    case mamtFILE:

      if (OType == mxoJob)
        {
        rc = MAMAllocJDebit(AM,J,S3CP,MsgP);
        /* J->LastChargeTime is set on success in MAMAllocJDebit */
        }
      else if (OType == mxoRsv)
        {
        rc = MAMAllocRDebit(AM,R,FALSE,S3CP,MsgP);
        /* R->LastChargeTime is set on success in MAMAllocRDebit */
        }

      break;

    default:

      break;
    }     /* switch (AM->Type) */

  /* If end call failed */
  if (rc == FAILURE)
    {
    /* Log an error message to the object */
    if (MsgP[0] != '\0')
      {
      if (OType == mxoJob)
        {
        MJobSetAttr(J,mjaMessages,(void **)MsgP,mdfString,mSet);
        }
      else if (OType == mxoRsv)
        {
        MRsvSetAttr(R,mraMessages,(void **)MsgP,mdfString,mSet);
        }
      }
    }

  /* Log event record */

  sprintf(EventMessage,"%s registering %s end (%s) with accounting manager",
    (rc == SUCCESS) ? "Success" : "Failure",
    (OType > mxoNONE && OType < mxoLAST) ? MXO[OType] : "unknown",
    (OType == mxoJob && O != NULL) ? ((mjob_t *)O)->Name
      : (OType == mxoRsv && O != NULL) ? ((mrsv_t *)O)->Name
      : "unknown"
    );

  if (MsgP[0] != '\0')
    {
    strcat(EventMessage," -- ");
    strncat(EventMessage,MsgP,sizeof(EventMessage) - strlen(EventMessage));
    }

  MOWriteEvent(O,mxoJob,mrelAMEnd,EventMessage,MStat.eventfp,NULL);

  return(rc);
  }     /* END MAMHandleEnd() */


/**
 * Notify accounting manager when a job or reservation needs to be cleaned up.
 * Normally, MAMHandleEnd will clean up. This routine would be called in cases
 * where the normal sequence of events is disrupted. For example, if a job is
 * created and started, but fails and we don't want to make a charge to it, 
 * we might call MAMHandleDelete to clean up the lien. Or, if a reservation
 * were created which makes an initial lien, but is removed before starting,
 * we would call MAMHandleDelete to clean up the lien. Depending on the use
 * case, this routine might clean up liens or it might do nothing. A failure
 * should result in a message logged against the object if it still exists.
 *
 * @see MJobProcessFailedStart() - parent
 * @see MJobSelectMNL() - parent
 * @see MQueueCheckStatus() - parent
 * @see MRMJobPostUpdate() - parent
 * @see MJobProcessRemoved() - parent
 * @see MJobPreempt() - parent
 * @see MAMHandleDelete() - peer
 * @see MAMNativeDoCommand() - child
 * @see MAMAllocResCancel() - child
 *
 * @param AM       [I] - AM to use
 * @param O        [I] - The Object reference
 * @param OType    [I] - Type of Object (job, rsv, etc.)
 * @param Msg      [O] - Status message
 * @param S3C      [O] - SSS Status Code Decade
 */

int MAMHandleDelete(

  mam_t                  *AM,
  void                   *O,
  enum MXMLOTypeEnum      OType,
  char                   *Msg,
  enum MS3CodeDecadeEnum *S3C)

  {
  const char             *FName = "MAMHandleDelete";
  char                   *MsgP;
  char                    tmpMsg[MMAX_LINE];
  char                    Message[MMAX_LINE];
  char                    EventMessage[MMAX_LINE];

  int                     rc = SUCCESS;

  enum MS3CodeDecadeEnum  tmpS3C;
  enum MS3CodeDecadeEnum *S3CP;

  mjob_t                 *J = NULL;
  mrsv_t                 *R = NULL;

  MDB(3,fAM) MLog("%s(%s,%s,%s,%p,%p)\n",
    FName,
    (AM != NULL) ? AM->Name : "NULL",
    (OType == mxoJob && O != NULL) ? ((mjob_t *)O)->Name
       : (OType == mxoRsv && O != NULL) ? ((mrsv_t *)O)->Name
       : "unknown",
    (OType > mxoNONE && OType < mxoLAST) ? MXO[OType] : "unknown",
    Msg,S3C);

  /* Initialize Variables */

  if (OType == mxoJob)
    {
    J = (mjob_t *)O;

    /* Unset reserved flag */
    bmunset(&J->IFlags,mjifAMReserved);
    }
  else if (OType == mxoRsv)
    R = (mrsv_t *)O;

  MsgP = (Msg != NULL) ? Msg : tmpMsg;
  MsgP[0] = '\0';

  S3CP = (S3C != NULL) ? S3C : &tmpS3C;
  *S3CP = ms3cNone;

  /* Validate pre-conditions */

  if (AM == NULL)
    {
    strcpy(Message,"Invalid accounting manager specified");
    MDB(3,fAM) MLog("ERROR:    %s in %s\n",Message,FName);
    strcpy(MsgP,Message);
    return(FAILURE);
    }

  if ((O == NULL) || (OType == mxoNONE))
    {
    strcpy(Message,"Invalid object specified");
    MDB(3,fAM) MLog("ERROR:    %s in %s\n",Message,FName);
    strcpy(MsgP,Message);
    return(FAILURE);
    }

  if (AM->State == mrmsDown)
    {
    strcpy(Message,"Accounting manager is down");
    MDB(3,fAM) MLog("WARNING:  %s in %s\n",Message,FName);
    strcpy(MsgP,Message);
    *S3CP = ms3cClientResourceUnavailable;
    return(FAILURE);
    }

  /* Check to see if this call should be skipped */

  if (AM->Type == mamtNONE)
    {
    strcpy(Message,"AM end request ignored -- ");
    strcat(Message,"Accounting manager not enabled");
    MDB(8,fAM) MLog("ALERT:    %s in %s\n",Message,FName);
    strcpy(MsgP,Message);
    return(SUCCESS);
    }

  if ((MSched.Mode != msmNormal) && (MAMTestMode != TRUE))
    {
    strcpy(Message,"AM end request ignored -- ");
    strcat(Message,"Incompatible scheduling mode");
    MDB(8,fAM) MLog("ALERT:    %s in %s\n",Message,FName);
    strcpy(MsgP,Message);
    return(SUCCESS);
    }

  if ((OType == mxoJob) && (J->System != NULL))
    {
    strcpy(Message,"AM end request ignored -- ");
    strcat(Message,"System job");
    MDB(8,fAM) MLog("ALERT:    %s in %s\n",Message,FName);
    strcpy(MsgP,Message);
    return(SUCCESS);
    }

  if ((OType == mxoJob) &&
      (J->Credential.C != NULL) && (J->Credential.C->DisableAM == TRUE))
    {
    strcpy(Message,"AM end request ignored -- ");
    strcat(Message,"Accounting disabled for class");
    MDB(8,fAM) MLog("ALERT:    %s in %s\n",Message,FName);
    strcpy(MsgP,Message);
    return(SUCCESS);
    }

  if ((OType == mxoRsv) && (R->Type == mrtJob))
    {
    strcpy(Message,"AM end request ignored -- ");
    strcat(Message,"Job Reservation");
    MDB(8,fAM) MLog("ALERT:    %s in %s\n",Message,FName);
    strcpy(MsgP,Message);
    return(SUCCESS);
    }

  if ((OType == mxoRsv) && (bmisset(&R->Flags,mrfNoCharge)))
    {
    strcpy(Message,"AM end request ignored -- ");
    strcat(Message,"No charging on reservations");
    MDB(8,fAM) MLog("ALERT:    %s in %s\n",Message,FName);
    if (Msg != NULL) strcpy(Msg,Message);
    return(SUCCESS);
    }

  if ((OType == mxoRsv) && ((R->A == NULL) || !strcmp(R->A->Name,NONE)))
    {
    strcpy(Message,"AM create request ignored -- ");
    strcat(Message,"Reservation has no accountable account");
    MDB(8,fAM) MLog("ALERT:    %s in %s\n",Message,FName);
    strcpy(MsgP,Message);
    return(SUCCESS);
    }

  if ((OType == mxoRsv) && (AM->IsChargeExempt[mxoRsv] == TRUE))
    {
    strcpy(Message,"AM end request ignored -- ");
    strcat(Message,"Reservations are exempt from charges");
    MDB(8,fAM) MLog("ALERT:    %s in %s\n",Message,FName);
    strcpy(MsgP,Message);
    return(SUCCESS);
    }

  /* Invoke the appropriate subroutine */
  switch (AM->Type)
    {
    case mamtNative:

      rc = MAMNativeDoCommand(AM,O,OType,mamnDelete,MsgP,NULL,S3CP);

      break;

    case mamtMAM:
    case mamtGOLD:
    case mamtFILE:

      rc = MAMAllocResCancel(AM,O,OType,S3CP,MsgP);

      break;

    default:

      break;
    }     /* switch (AM->Type) */

  /* If delete call failed */
  if (rc == FAILURE)
    {
    /* Log an error message to the object */
    if (MsgP[0] != '\0')
      {
      if (OType == mxoJob)
        {
        MJobSetAttr(J,mjaMessages,(void **)MsgP,mdfString,mSet);
        }
      else if (OType == mxoRsv)
        {
        MRsvSetAttr(R,mraMessages,(void **)MsgP,mdfString,mSet);
        }
      }
    }

  /* Log event record */

  sprintf(EventMessage,"%s registering %s delete (%s) with accounting manager",
    (rc == SUCCESS) ? "Success" : "Failure",
    (OType > mxoNONE && OType < mxoLAST) ? MXO[OType] : "unknown",
    (OType == mxoJob && O != NULL) ? ((mjob_t *)O)->Name
      : (OType == mxoRsv && O != NULL) ? ((mrsv_t *)O)->Name
      : "unknown"
    );

  if (MsgP[0] != '\0')
    {
    strcat(EventMessage," -- ");
    strncat(EventMessage,MsgP,sizeof(EventMessage) - strlen(EventMessage));
    }

  MOWriteEvent(O,mxoJob,mrelAMDelete,EventMessage,MStat.eventfp,NULL);

  return(rc);
  }     /* END MAMHandleDelete() */


/* END MAMHandle.c */
