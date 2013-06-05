/* HEADER */


/**
 * @file MAMDoCommand.c
 *
 * Contains: Various AM DoCommand functions
 */

#include "moab.h"
#include "moab-proto.h"
#include "moab-const.h"
#include "moab-global.h"
#include "mcom.h"


/**
 *
 */

mbool_t __MAMOpIsQueueable(

  enum MAMNativeFuncTypeEnum Op)

  {
  if ((Op == mamnEnd) ||
      (Op == mamnDelete)) /* NYI || (Op == mamnPause) */
    {
    return(TRUE);
    }

  return(FALSE);
  }  /* END MAMOpIsQueueable() */


/**
 * Issue low-level Gold command.
 *
 * @see MS3DoCommand() - child
 *
 * @param ReqE (I) [freed]
 * @param P (I)
 * @param Response (O) [optional,alloc]
 * @param S3C (O) [optional] - SSS Status Code Decade
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 */

int MAMGoldDoCommand(

  mxml_t                 *ReqE,     /* I (freed) */
  mpsi_t                 *P,        /* I */
  char                   **Response, /* O (optional,alloc) */
  enum MS3CodeDecadeEnum *S3C,      /* O (optional) */
  char                   *EMsg)     /* O (optional,minsize=MMAX_LINE) */

  {
  int       rc;

  mxml_t   *E;
  mxml_t   *BE;
  mxml_t   *DE;
  mxml_t   *RE;

  char      tmpLine[MMAX_LINE];

  char      tmpMsg[MMAX_LINE];

  int       tmpI;
  int       tmpSC;

  enum MSFC SC;

  mam_t    *A;

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (Response != NULL)
    Response[0] = '\0';

  if (S3C != NULL)
    *S3C = ms3cNone;

  if ((ReqE == NULL) || (P == NULL))
    {
    return(FAILURE);
    }

  A = &MAM[0];

  /* attach XML request to socket */

  P->S->SDE = ReqE;

  MRMStartFunc(NULL,P,mrmInfoQuery);

  rc = MS3DoCommand(
    P,
    NULL,
    Response, /* O - alloc */
    NULL,
    &tmpSC,   /* O - status code */
    tmpMsg);  /* O - status message */

  MRMEndFunc(NULL,P,mrmInfoQuery,NULL);

  /* remove time from sched load and add to rm action load */

  /*
  MSched.LoadEndTime[mschpRMAction] += RMTime;
  MSched.LoadStartTime[mschpSched]  += RMTime;
  */

  MDB(3,fAM) MLog("INFO:     Command response '%s'\n",
    (P->S->RBuffer != NULL) ? P->S->RBuffer : "NULL");

  /* process response */

  DE = (mxml_t *)P->S->RDE;

  if (DE == NULL)
    {
    /* optional job data not available */

    MDB(3,fAM) MLog("ALERT:    No job data available\n");
    }
  else
    {
    /* process job data */

    /* NYI */
    }

  /* extract response */

  E = NULL;

  MUGetRC(NULL,tmpSC,&tmpI,mrclCentury);

  SC = (enum MSFC)tmpI;

  /* S3C is SSS Status Code / 10 */
  if (S3C != NULL)
   *S3C = (enum MS3CodeDecadeEnum)(tmpSC / 10);

  if ((rc == FAILURE) ||
      (MXMLFromString(&E,P->S->RBuffer,NULL,NULL) == FAILURE))
    {
    switch (tmpSC)
      {
      case msfEGSecurity:

        MDB(3,fAM) MLog("ERROR:    Unable to authenticate server\n");

        /* NOTE:  hardcoded to function = mamJobAllocReserve */

        MAMSetFailure(
        A,
        mamJobAllocReserve,
        mscSysFailure,
        "cannot authenticate message, check moab-private.cfg");

        break;

      case msfEGServerBus:
      case msfEGNoFunds:

        /* if account fail as funds flag is set, we need to treat business
         * logic failure as a funds failure */

        if ((bmisset(&A->Flags,mamfAccountFailAsFunds)) ||
            (tmpSC == msfEGNoFunds))
          {
          /* NOTE:  if we received a "no valid allocations" message, we're
                    assuming it's because the user is not a member of the
                    specified account in Gold. */

          MDB(3,fAM) MLog("WARNING:  Insufficient funds - %s\n",
            tmpMsg);

          /* NOTE:  hardcoded to function = mamJobAllocReserve */

          MAMSetFailure(
            A,
            mamJobAllocReserve,
            mscSysFailure,
            tmpMsg);
          } /* END account fail as funds */

        else
          {
          MDB(3,fAM) MLog("ALERT:    Unable to access requested funds - %s\n",
            tmpMsg);

          /* NOTE:  hardcoded to function = mamJobAllocReserve */

          MAMSetFailure(
            A,
            mamJobAllocReserve,
            mscSysFailure,
            tmpMsg);
          } /* END else (default behavior) */

        break;

      default:

        if (tmpSC < 0)
          {
          MDB(3,fAM) MLog("ALERT:    Unable to extract status\n");

          break;
          }

        MDB(3,fAM) MLog("ALERT:    Unexpected AM error - %s\n",
        tmpMsg);

        /* NOTE:  hardcoded to function = mamJobAllocReserve */

        MAMSetFailure(
        A,
        mamJobAllocReserve,
        mscSysFailure,
        tmpMsg);

        break;
      } /* END switch(tmpSC) */

    if (EMsg != NULL)
      MUStrCpy(EMsg,tmpMsg,MMAX_LINE);

    return(FAILURE);
    }  /* END if ((rc == FAILURE) || ...) */

  if ((MXMLGetChild(E,"Body",NULL,&BE) == FAILURE) ||
      (MXMLGetChild(BE,(char *)MSON[msonResponse],NULL,&RE) == FAILURE))
    {
    if (S3C != NULL)
      *S3C = ms3cMessageFormat;

    MXMLDestroyE(&E);

    MSUFree(P->S);

    if (tmpSC == msfEGSecurity)
      {
      MDB(3,fAM) MLog("ERROR:    Unable to authenticate server\n");

      /* NOTE:  hardcoded to function = mamJobAllocReserve */

      MAMSetFailure(
        A,
        mamJobAllocReserve,
        mscSysFailure,
        "cannot authenticate message, check moab-private.cfg");

      /* full system failure, shutdown interface */

      MAMShutdown(A);

      A->State = mrmsCorrupt;
      }
    else if (tmpSC > 0)
      {
      MDB(3,fAM) MLog("ERROR:    Unexpected AM error\n");

      /* NOTE:  hardcoded to function = mamJobAllocReserve */

      MAMSetFailure(
        A,
        mamJobAllocReserve,
        mscSysFailure,
        tmpMsg);
      }
    else
      {
      MDB(3,fAM) MLog("ERROR:    Unable to extract status\n");
      }

    if (EMsg != NULL)
      MUStrCpy(EMsg,tmpMsg,MMAX_LINE);

    return(FAILURE);
    }

  MSUFree(P->S);

  if (MS3CheckStatus(RE,&SC,tmpLine) == FAILURE)
    {
    MXMLDestroyE(&E);

    MDB(3,fAM) MLog("WARNING:  Failure message '%s' received\n",
      tmpLine);

    MUGetRC(NULL,SC,&tmpI,mrclCentury);

    if (SC == msfEGSecurity)
      {
      MDB(3,fAM) MLog("ERROR:    Unable to authenticate server\n");

      /* NOTE:  hardcoded to AM[0] */
      /* NOTE:  hardcoded to function = mamJobAllocReserve */

      MAMSetFailure(
        A,
        mamJobAllocReserve,
        mscSysFailure,
        "cannot authenticate message, check moab-private.cfg");
      }

    return(FAILURE);
    }  /* END if (MS3CheckStatus(RE,&SC,tmpLine) == FAILURE) */

  MXMLDestroyE(&E);

  return(SUCCESS);
  }  /* END MAMGoldDoCommand() */


/**
 * Reports NAMI events (create, charge, etc.)
 *
 * Takes into account that billing solutions will differ and use different parts.
 *  If the Path to the event URL is NULL, return SUCCESS (must not be part of
 *  their billing plan).
 *
 * Returns FAILURE if the call failed (SC != mscNoError)
 *
 * @param A        [I] - AM to use
 * @param O        [I] - The object to bill for
 * @param OType    [I] - Type of Object (job, rsv, etc.)
 * @param Op       [I] - The type of billing event (create, charge, etc.)
 * @param EMsg     [O] - Error message
 * @param Response [O] - Output of the call
 * @param S3C      [O] - SSS Status Code Decade
 */

int MAMNativeDoCommand(

  mam_t                     *A,
  void                      *O,
  enum MXMLOTypeEnum         OType,
  enum MAMNativeFuncTypeEnum Op,
  char                      *EMsg,
  mstring_t                 *Response,
  enum MS3CodeDecadeEnum    *S3C)

  {
  char                    MyEMsg[MMAX_LINE];
  char                   *EMsgP;
  char                   *Path;

  mxml_t                 *E = NULL;

  mjob_t                 *tmpJ = NULL;
  mrsv_t                 *tmpR = NULL;

  int                     tmpSC;
  enum MS3CodeDecadeEnum  tmpS3C;
  enum MS3CodeDecadeEnum *S3CP;

  const char             *FName = "MAMNativeDoCommand";

  MDB(2,fAM) MLog("%s(%s,%s,%s,%s,%p,%p,%p)\n",
    FName,
    (A != NULL) ? A->Name : "NULL",
    (OType == mxoJob && O != NULL) ? ((mjob_t *)O)->Name
       : (OType == mxoRsv && O != NULL) ? ((mrsv_t *)O)->Name
       : "unknown",
    (OType > mxoNONE && OType < mxoLAST) ? MXO[OType] : "unknown",
    MAMNativeFuncType[Op],
    EMsg,Response,S3C);

  if ((A == NULL) ||
      (O == NULL) ||
      (OType == mxoNONE) ||
      (Op == mamnNONE))
    {
    return(FAILURE);
    }

  if ((A->Name[0] == '\0') || (A->Type != mamtNative))
    {
    /* No billing is set up or it is not Native */

    return(SUCCESS);
    }

  /* check to see if Allocation Manager is disabled by class */
  if (OType == mxoJob)
    {
    tmpJ = (mjob_t *)O;

    if ((tmpJ->Credential.C != NULL) && (tmpJ->Credential.C->DisableAM == TRUE))
      return(SUCCESS);
    }

  if (MAMOToXML(O,OType,Op,&E) == FAILURE)
    return(FAILURE);

  Path = A->ND.AMPath[Op];

  if (Path == NULL)
    {
    /* Path is NULL, not part of admin's billing plan */

    return(SUCCESS);
    }

  /* If queued action, add to queue (XML) and return */

  if ((MSched.QueueNAMIActions) && (__MAMOpIsQueueable(Op)))
    {
    return(MAMAddToNAMIQueue(A,E,Op));
    }

  /* Generate some XML to send */


  mstring_t String(MMAX_LINE);

  MXMLToMString(E,&String,NULL,TRUE);

  MXMLDestroyE(&E);

  mstring_t tmpResponse(MMAX_LINE);

  mstring_t *ResponseP;

  /* If no Response passed in use our own */
  if (Response == NULL)
    {
    ResponseP = &tmpResponse;
    }
  else
    {
    ResponseP = Response;
    }

  /* If no EMsg passed in use our own */
  if (EMsg == NULL)
    EMsgP = MyEMsg;
  else
    EMsgP = EMsg;
  EMsgP[0] = '\0';

  /* If no S3C passed in use our own */
  if (S3C == NULL)
    S3CP = &tmpS3C;
  else
    S3CP = S3C;
  *S3CP = ms3cNone;

  /* Init STDERR buffer, free it before leaving function */

  mstring_t EBuf(MMAX_LINE);

  /* Dispatch upon interface type */

  switch (A->ND.AMProtocol[Op])
    {
    default:
    case mbpExec:

      {
      int rc = 0;

      rc = MUReadPipe2(
        Path,             /* Command plus args */
        String.c_str(),   /* STDIN */
        ResponseP,        /* STDOUT */
        &EBuf,            /* STDERR */
        NULL,             /* Peer object (optional) */
        &tmpSC,           /* return code (optional) */
        EMsgP,            /* message encountered upon error state */
        NULL);            /* Status Code */

      *S3CP = (tmpSC >= (int)ms3cNone && tmpSC < (int)ms3cLAST)
        ? (enum MS3CodeDecadeEnum)tmpSC
        : ms3cUnknown;

      if ((rc == FAILURE) && (*S3CP == ms3cNone))
        {
        *S3CP = ms3cUnknown;
        }
      }                   /* END case mbpExec: */

      break;

    case mbpNoOp:
    case mbpFile:
    case mbpGanglia:
    case mbpHTTP:

      return(FAILURE);

      break;
    } /* END switch (A->ND.AMProtocol[Op]) */

  /* If Create, NO ERROR and other state.... */

  if ((Op == mamnCreate) &&
      (MSched.NAMITransVarName != NULL) &&
      (*S3CP >= ms3cNone && *S3CP < ms3cWireProtocol) &&
      (!ResponseP->empty()))
    {
    /* A billing transaction ID can be set by the create script */

    mstring_t tmpVar(MMAX_LINE);
    mstring_t tmpVal(MMAX_LINE);

    /* "keyVar=Value" */
    tmpVar = MSched.NAMITransVarName;
    tmpVar += '=';

    MStringReplaceStr(ResponseP->c_str(),"\n","",&tmpVal);

    tmpVar += tmpVal.c_str();

    /* Need to map to specific object in order to set object specific stuff */
    switch (OType)
      {
      case mxoJob:

        {
        tmpJ = (mjob_t *)O;

        MJobSetAttr(tmpJ,mjaVariables,(void **)tmpVar.c_str(),mdfString,mAdd);
        }

        break;

      case mxoRsv:

        {
        tmpR = (mrsv_t *)O;

        MRsvSetAttr(tmpR,mraVariables,(void **)tmpVar.c_str(),mdfString,mAdd);
        }

        break;

      default:

        break;
      }  /* END switch (OType) */
    }    /* END if ((Op == mamnCreate) && (MSched.NAMITransVarName != NULL) && ...) */


  /*
   * IF ERROR CONDITION:
   *    then LOG information and attach error reasons to the object to aid in determining
   *    the error event
   */

  if (*S3CP < ms3cNone || *S3CP >= ms3cWireProtocol)
    {
    MDB(7,fSTRUCT) MLog("WARNING:  Accounting manager call '%s' failed - used XML \"%s\"\n",
      MAMNativeFuncType[Op],
      String.c_str());

    /* if the object is a Job or a Rsv, then record the error messsage to that object */

    switch (OType)
      {
      case mxoJob:

        {
        tmpJ = (mjob_t *)O;

        /* record the STDERR output of the failed ReadPipe2 call */
        MMBAdd(&tmpJ->MessageBuffer,EBuf.c_str(),NULL,mmbtNONE,0,0,NULL);

        /* record the EMsg output of the failed ReadPipe2 call */
        MMBAdd(&tmpJ->MessageBuffer,EMsgP,NULL,mmbtNONE,0,0,NULL);
        }

        break;

      case mxoRsv:

        {
        tmpR = (mrsv_t *)O;

        /* record the STDERR output of the failed ReadPipe2 call */
        MMBAdd(&tmpR->MB,EBuf.c_str(),NULL,mmbtNONE,0,0,NULL);

        /* record the EMsg output of the failed ReadPipe2 call */
        MMBAdd(&tmpR->MB,EMsgP,NULL,mmbtNONE,0,0,NULL);
        }

        break;

      default:

        /* Don't attach error message */

        break;
      }  /* END switch (OType) */
    }    /* END if (*S3CP < ms3cNone || *S3CP >= ms3cWireProtocol) */

  /* MAMNativeDoCommand can return MS3CodeDecadeEnum errors as well as
   * MStatusCodeEnum errors so anything other than zero is a failure */
  if (*S3CP != ms3cNone)
    return(FAILURE);
  else
    return(SUCCESS);
  }  /* END MAMNativeDoCommand() */


/**
 * Does the NAMI command with the given string.
 *
 * Used for the NAMI queues.
 *  Returns SUCCESS if the call succeeded (0 return code, etc.), FAILURE otherwise.
 *
 * @param A      (I)
 * @param Action (I)
 * @param Op     (I)
 */

int MAMNativeDoCommandWithString(
  mam_t                     *A,
  const char                *Action,
  enum MAMNativeFuncTypeEnum Op)

  {
  char *Path;
  int   rc = 0;

  if ((A == NULL) || (Action == NULL))
    {
    return(FAILURE);
    }

  if (A->Type != mamtNative)
    {
    return(SUCCESS);
    }

  Path = A->ND.AMPath[Op];

  if (Path == NULL)
    {
    /* Path is NULL, not part of admin's billing plan */

    return(SUCCESS);
    }

  switch (A->ND.AMProtocol[Op])
    {
    default:
    case mbpExec:

      {
      mstring_t EBuf(MMAX_LINE);
      mstring_t Response(MMAX_LINE);

      enum MStatusCodeEnum SC;

      MUReadPipe2(
        Path,
        Action,
        &Response,
        &EBuf,
        NULL,
        NULL,
        NULL,
        &SC);

      if (SC == mscNoError)
        rc = 0;
      else
        rc = 1;
      }  /* END case mbpExec */

      break;

    case mbpHTTP:
    case mbpNoOp:
    case mbpFile:

      return(FAILURE);

      break;
    }  /* END switch (A->ND.AMProtocol[Op]) */

  if (rc == 0)
    return(SUCCESS);
  else
    {
    MDB(7,fSTRUCT) MLog("WARNING:  Accounting manager call '%s' failed - used XML \"%s\"\n",
      MAMNativeFuncType[Op],
      Action);

    return(FAILURE);
    }
  }  /* END MAMNativeDoCommandWithString() */


/* END MAMDoCommand.c */
