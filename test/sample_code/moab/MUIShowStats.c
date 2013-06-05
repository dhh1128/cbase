/* HEADER */

/**
 * @file MUIShowStats.c
 *
 * Contains MUI Show Statistics
 */



#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 * Report object statistics (process 'showstats' client request).
 *
 * @see MUIStatShowMatrix() - child - process 'showstats -f'
 * @see MUIShowCStats() - child
 * @see MCredProfileToXML() - child - process 'showstats -{uagcq} [-t XXX]' 
 * @see MTJobStatProfileToXML() - child - process 'showstats -j -t XXX'
 * @see MTJobStatToXML() - child - process 'showstats -j'
 * @see 'mcredctl -q profile <CREDTYPE>'
 *
 * @param S (I)
 * @param CFlags (I) [bitmap of enum XXX]
 * @param Auth (I)
 */

int MUIShowStats(

  msocket_t *S,      /* I */
  mbitmap_t *CFlags, /* I (bitmap of enum XXX) */
  char      *Auth)   /* I */

  {
  char tmpBuf[MMAX_BUFFER];

  enum MXMLOTypeEnum OType;

  char OID[MMAX_NAME];

  char CmdFlags[MMAX_NAME];

  char tmpLine[MMAX_LINE];
  char tmpName[MMAX_NAME];
  char tmpVal[MMAX_NAME];

  char SName[MMAX_NAME];

  long STime;
  long ETime;

  mbitmap_t  FlagBM;
  mbitmap_t  Flags;
  mbitmap_t  AuthBM;

  int  CTok;

  mpar_t *P;
  mpsi_t *Peer = NULL;

  int  rc;

  mxml_t *RE;

  char *ptr;
  char *TokPtr;

  mgcred_t *U = NULL;

  mclass_t *FilterC = NULL;

  /* FORMAT:  <OTYPE> <OID> */

  const char *FName = "MUIShowStats";

  MDB(3,fUI) MLog("%s(S,%ld,%s)\n",
    FName,
    CFlags,
    (Auth != NULL) ? Auth : "NULL");

  if (S->RDE == NULL)
    {
    MDB(3,fUI) MLog("WARNING:  corrupt command received '%32.32s'\n",
      (S->RBuffer != NULL) ? S->RBuffer : "NULL");

    MUISAddData(S,"ERROR:    corrupt command received\n");

    return(FAILURE);
    }

  STime = -1;
  ETime = -1;

  SName[0] = '\0';

  RE = S->RDE;
 
  if ((Auth != NULL) &&
      (!strncasecmp(Auth,"peer:",strlen("peer:"))))
    {
    if (MPeerFind(Auth,FALSE,&Peer,FALSE) == FAILURE)
      {
      /* NYI */
      }      
    }
  else
    {
    MUserAdd(Auth,&U);
    }

  if (MXMLGetAttr(RE,MSAN[msanArgs],NULL,tmpLine,0) == FAILURE)
    {
    MUISAddData(S,"internal error - cannot generate requested statistics\n");

    return(FAILURE);
    }

  if (MXMLCreateE(&S->SDE,(char *)MSON[msonData]) == FAILURE)
    {
    MUISAddData(S,"ERROR:    cannot create response\n");

    return(FAILURE);
    }

  /* FORMAT: <OTYPE>[:<OID>] */

  ptr = MUStrTok(tmpLine,":",&TokPtr);

  OType = (enum MXMLOTypeEnum)MUGetIndexCI(ptr,MXO,FALSE,mxoNONE);

  ptr = MUStrTok(NULL,":",&TokPtr);

  if (ptr != NULL)
    MUStrCpy(OID,ptr,sizeof(OID));
  else
    strcpy(OID,NONE);

  P = &MPar[0];

  CTok = -1;

  while (MS3GetWhere(
        RE,
        NULL,
        &CTok,
        tmpName,
        sizeof(tmpName),
        tmpVal,
        sizeof(tmpVal)) == SUCCESS)
    {
    if (!strcasecmp(tmpName,"class"))
      {
      if (MClassFind(tmpVal,&FilterC) == FAILURE)
        {
        MUISAddData(S,"cannot locate specified class");

        return(FAILURE);
        }
      }
    if (!strcmp(tmpName,"partition"))
      {
      if (MParFind(tmpVal,&P) == FAILURE)
        P = &MPar[0];
      }
    else if (!strcmp(tmpName,"type"))
      {
      if (!strcmp(tmpVal,MJobStateType[mjstEligible]))
        bmset(&FlagBM,mjstEligible);
      }
    else if (!strcmp(tmpName,"timeframe"))
      {
      MStatGetRange(tmpVal,&STime,&ETime);

      /* If the start time is greater than the end time then this is a malformed
         timeframe and we cannot do a showstats for it. */

      if (STime > ETime)
        {
        snprintf(tmpBuf,sizeof(tmpBuf),"          timeframe '%s' is not valid\n\n",
          tmpVal);

        MUISAddData(S,tmpBuf);

        return(FAILURE);
        }
      else if (ETime - STime > MCONST_YEARLEN)
        {
        snprintf(tmpBuf,sizeof(tmpBuf),"          timeframe '%s' is too long\n\n",
          tmpVal);

        MUISAddData(S,tmpBuf);

        return(FAILURE);
        }
      }
    }    /* END while (MS3GetWhere() == SUCCESS) */

  if (MXMLGetAttr(RE,MSAN[msanOp],NULL,tmpLine,sizeof(tmpLine)) != FAILURE)
    {
    bmfromstring(tmpLine,MClientMode,&Flags);
    }

  MXMLGetAttr(RE,MSAN[msanOp],NULL,CmdFlags,sizeof(CmdFlags));

  /* NOTE:  currently no subtypes for StatShow, allow 'granted' access by passing subcommand '1' */

  MSysGetAuth(Auth,mcsStatShow,1,&AuthBM);

  if ((!bmisset(&AuthBM,mcalGranted)) && (OID[0] != '\0'))
    {
    switch (OType)
      {
      case mxoAcct:

        {
        mgcred_t *A;

        if ((MAcctFind(OID,&A) == SUCCESS) &&
            (A->F.ManagerU != NULL) &&
            (MCredFindManager(Auth,A->F.ManagerU,NULL) == SUCCESS))
          {
          bmset(&AuthBM,mcalGranted);
          }
        }

        break;

      case mxoClass:

        {
        mclass_t *C;

        if ((MClassFind(OID,&C) == SUCCESS) &&
            (C->F.ManagerU != NULL) &&
            (MCredFindManager(Auth,C->F.ManagerU,NULL) == SUCCESS))
          {
          bmset(&AuthBM,mcalGranted);
          }
        }

        break;

      default:

        /* NO-OP */

        break;
      }  /* END switch (OType) */
    }    /* END if ((!bmisset(&AuthBM,mcalGranted)) && (OID[0] != '\0')) */

  switch (OType)
    {
    case mxoUser:
    case mxoGroup:
    case mxoAcct:
    case mxoClass:
    case mxoQOS:

      /* process showstats -u/showstats -g/showstats -a/showstats -c/showstats -q */

      {
      char EMsg[MMAX_LINE] = {0};

      /* MUIShowCStats() handles authorization */

      if (STime != -1)
        {
        /* show stats for specific time range */

        rc = MCredProfileToXML(
               OType,
               (strcmp(OID,NONE) == 0) ? (char *)"" : OID,
               SName,
               STime,
               ETime,
               -1,
               NULL,
               TRUE,
               TRUE,
               S->SDE);  /* O */

        MSchedStatToString(
          &MSched,
          mwpXML,
          S->SDE,
          0,
          TRUE);
        }  /* END if (STime != -1) */
      else
        {
        /* show current stats */

        rc = MUIShowCStats(
               &AuthBM,
               RE,
               OID,
               OType,
               &FlagBM,
               Auth,     /* I */
               S->SDE,
               STime,
               ETime,
               EMsg);

        if (rc == FAILURE)
          {
          MUISAddData(S,EMsg);
          }
        }
      }    /* END BLOCK (case mxoQOS) */

      break;

    case mxoxFS:

      {
      mfsc_t *FC;

      FC = &P->FSC;

      MXMLDestroyE(&S->SDE);

      MFSTreeStatShow(
        FC->ShareTree,
        NULL,
        mxoUser,
        0,
        0,
        &S->SDE);

      MSchedStatToString(
        &MSched,
        mwpXML,
        S->SDE,
        0,
        TRUE);

      rc = SUCCESS;
      }  /* END BLOCK (case mxoxFS) */

      break;

    case mxoJob:

      {
      mjob_t *tmpJ;

      /* process 'showstats -j' - report job template statistics */

      if (!strcmp(OID,NONE))
        {
        /* JOBID not specified */

        if (MUArrayListGet(&MTJob,0) == NULL)
          {
          MUISAddData(S,"no job templates defined");
 
          rc = FAILURE;

          break;
          }
        }
      else if (MTJobFind(OID,&tmpJ) == FAILURE)
        {
        MUISAddData(S,"cannot locate specified job template");

        rc = FAILURE;

        break;
        }

      if (STime != -1)
        {
        /* show stats for specific time range */

        rc = MTJobStatProfileToXML(
               NULL,
               (strcmp(OID,NONE) == 0) ? NULL : OID,
               SName,
               STime,
               ETime,
               -1,
               TRUE,
               TRUE,
               S->SDE);
        }  /* END if (STime != -1) */
      else
        {
        MXMLDestroyE(&S->SDE);

        rc = MUITJobStatToXML(OID,&S->SDE);
        }

      MSchedStatToString(
        &MSched,
        mwpXML,
        S->SDE,
        0,
        TRUE);
      }  /* END BLOCK (case mxoJob) */

      break;

    case mxoNode:

      {
      char EMsg[MMAX_LINE] = {0};

      /* authorization check in MUINodeShowStats NYI */

      /* NOTE: we have to free this because MUINodeStatsToXML() creates its own
               <data> element */

      MXMLDestroyE(&S->SDE);

      rc = MUINodeStatsToXML(
             OID,
             STime,
             ETime,
             EMsg,
             &S->SDE);

      if (rc == FAILURE)
        {
        MXMLSetVal(S->SDE,EMsg,mdfString);
        }
      else
        {
        MSchedStatToString(
          &MSched,
          mwpXML,
          S->SDE,
          0,
          TRUE);
        }
      }    /* END BLOCK (case mxoNode) */
 
      break;

    case mxoPar:

      if (MParFind(OID,&P) == FAILURE)
        P = &MPar[0];

      /* MUIParShowStats() handles authorization */

      rc = MUIParShowStats(Auth,P,tmpBuf,sizeof(tmpBuf));

      MXMLFromString(&S->SDE,tmpBuf,NULL,NULL);

      break;

    case mxoSched:

      /* no scheduler ownership auth condition */

      if (strstr(CmdFlags,"peer") != NULL)
        {
        /* FIXME: Flags is a BM of MCModeEnum not MProfModeEnum */

        bmset(&Flags,mcmPeer);
        }

      if (strcmp(OID,NONE))
        {
        rc = MUIStatShowMatrix(OID,tmpBuf,&Flags,Auth,sizeof(tmpBuf));  /* returns <matrix>... */

        if (bmisset(&Flags,mcmXML))
          {
          mxml_t *ME = NULL;

          MXMLFromString(&ME,tmpBuf,NULL,NULL);

          MXMLAddE(S->SDE,ME);
          }
        else
          {
          MUISAddData(S,tmpBuf);
          }
        }
      else if (bmisset(&AuthBM,mcalGranted))
        {
        rc = MSchedStatToString(&MSched,mwpXML,tmpBuf,sizeof(tmpBuf),FALSE);

        MXMLFromString(&S->SDE,tmpBuf,NULL,NULL);
        }
      else
        {
        snprintf(tmpBuf,sizeof(tmpBuf),"          user '%s' not authorized to run command\n\n",
          Auth);

        MUISAddData(S,tmpBuf);

        rc = FAILURE;
        }

      break;

    default:

      sprintf(tmpBuf,"internal error - cannot generate requested statistics for %s\n",
        MXO[MIN(OType,mxoALL - 1)]);

      rc = FAILURE;

      break;
    }  /* END switch (OType) */

  return(rc);
  }  /* END MUIShowStats() */
/* END MUIShowStats.c */
