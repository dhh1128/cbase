/* HEADER */

      
/**
 * @file MRMJobValidate.c
 *
 * Contains: MRMJobValidate
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 * Call external method/protocol to validate job.
 *
 * @param J (I)
 * @param R (I)
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param FailureAction (I) [optional]
 * @param SC (O) [optional]
 */

int MRMJobValidate(

  mjob_t              *J,              /* I */
  mrm_t               *R,              /* I */
  char                *EMsg,           /* O (optional,minsize=MMAX_LINE) */
  enum MJobCtlCmdEnum *FailureAction,  /* I (optional) */
  int                 *SC)             /* O (optional) */

  {
  enum MWJobAttrEnum aindex;
  enum MRMSubTypeEnum SType;

  int     index;

  char    SPath[MMAX_BUFFER];

  char   *BPtr;
  int     BSpace;

  char   *ptr;
  char   *TokPtr;

  char   *ptr2;
  char   *TokPtr2;

  char    tEMsg[MMAX_LINE];

  enum MStatusCodeEnum tmpSC;

  mbool_t GRes = FALSE;

  const char *FName = "MRMJobValidate";

  MDB(2,fRM) MLog("%s(%s,%s,EMsg,FailureAction,SC)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (R != NULL) ? R->Name : "NULL");

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (SC != NULL)
    *SC = 0;

  if (J == NULL)
    {
    return(FAILURE);
    }

  if (R->ND.URL[mrmXJobValidate] == NULL)
    {
    /* no job validate script specified */

    return(SUCCESS);
    }

  MUSNInit(&BPtr,&BSpace,SPath,sizeof(SPath));

  MUSNPrintF(&BPtr,&BSpace,"%s=%s ",
    MWikiJobAttr[mwjaUName],
    J->Credential.U->Name);

  if (J->EUser != NULL)
    {
    /* report execution user */

    MUSNPrintF(&BPtr,&BSpace,"%s=%s ",
      "EUSER",
      J->EUser);
    }

  if (J->Credential.G != NULL)
    {
    MUSNPrintF(&BPtr,&BSpace,"%s=%s ",
      MWikiJobAttr[mwjaGName],
      J->Credential.G->Name);
    }

  if (J->SpecWCLimit[0] > 0)
    {
    MUSNPrintF(&BPtr,&BSpace,"%s=%ld ",
      MWikiJobAttr[mwjaWCLimit],
      J->SpecWCLimit[0]);
    }

/* 
  if (J->Request.TC > 0)
    {
    MUSNPrintF(&BPtr,&BSpace,"%s=%d ",
      MWikiJobAttr[mwjaTasks],
      J->Request.TC);
    }
*/

  if (J->Credential.C != NULL)
    {
    MUSNPrintF(&BPtr,&BSpace,"%s=%s ",
      MWikiJobAttr[mwjaRClass],
      J->Credential.C->Name);
    }

  if (J->QOSRequested != NULL)
    {
    MUSNPrintF(&BPtr,&BSpace,"%s=%s ",
      MWikiJobAttr[mwjaQOS],
      J->QOSRequested->Name);
    }

  if (J->Env.Cmd != NULL)
    {
    char *ptr;
    char *TokPtr;

    char tmpLine[MMAX_LINE];

    MUStrCpy(tmpLine,J->Env.Cmd,sizeof(tmpLine));

    ptr = MUStrTok(tmpLine," ",&TokPtr);

    MUStringChop(ptr);

    MUSNPrintF(&BPtr,&BSpace,"%s=%s ",
      MWikiJobAttr[mwjaExec],
      ptr);
    }    /* END if (J->E.Cmd != NULL) */

  if (!bmisclear(&J->Req[0]->ReqFBM))
    {
    char tmpLine[MMAX_LINE];

    MUSNPrintF(&BPtr,&BSpace,"%s=%s ",
      MWikiJobAttr[mwjaRFeatures],
      MUNodeFeaturesToString(',',&J->Req[0]->ReqFBM,tmpLine));
    }

  if (J->Variables.Table != NULL)
    {
    mstring_t tmp(MMAX_LINE);

    MJobAToMString(J,mjaVariables,&tmp);

    MUSNPrintF(&BPtr,&BSpace,"%s=%s ",
      MWikiJobAttr[mwjaVariables],
      tmp.c_str());
    }

  /* display generic resources */

  for (index = 1;index < MSched.M[mxoxGRes];index++)
    { 
    if (MGRes.Name[index][0] == '\0')
      break;
 
    if (MSNLGetIndexCount(&J->Req[0]->DRes.GenericRes,index) == 0)
      continue;

    if (GRes == FALSE)
      {
      MUSNPrintF(&BPtr,&BSpace,"GRES=%s:%d",
        MGRes.Name[index],
        MSNLGetIndexCount(&J->Req[0]->DRes.GenericRes,index));

      GRes = TRUE;
      }
    else
      {
      MUSNPrintF(&BPtr,&BSpace,",%s:%d",
        MGRes.Name[index],
        MSNLGetIndexCount(&J->Req[0]->DRes.GenericRes,index));
      }
    }  /* END for (index) */

  /* send this off to JOBVALIDATEURL */
  
  SType = R->ND.NatType;

  R->ND.NatType = mrmstX1E;

  mstring_t Response(MMAX_LINE);

  /* NOTE: should we pass-in Job Validate specific timeout, ie 2 seconds? (NYI) */

  if (MNatDoCommand(
        &R->ND,
        SPath,
        mrmXJobValidate,
        R->ND.Protocol[mrmXJobValidate],
        FALSE,
        NULL,
        NULL,
        &Response,
        tEMsg,               /* O */
        &tmpSC) == FAILURE)
    {
    R->ND.NatType = SType;

    /* make failure action configurable */

    if ((ptr = strstr(tEMsg,"ACTION=")) != NULL)
      {
      ptr += strlen("ACTION=");

      ptr2 = MUStrTok(ptr," \n\t",&TokPtr);

      if (FailureAction != NULL)
        *FailureAction = (enum MJobCtlCmdEnum)MUGetIndexCI(ptr2,MJobCtlCmds,FALSE,mjcmNONE);
      }

    if (EMsg != NULL)
      MUStrCpy(EMsg,tEMsg,MMAX_LINE);

    if (SC != NULL)
      *SC = (int)tmpSC;

    return(FAILURE);
    }

  if (SC != NULL)
    *SC = (int)tmpSC;

  R->ND.NatType = SType;

  for (index = 0;Response[index] != '\0';index++)
    {
    if (Response[index] == '\n')
      Response[index] = ' ';
    }

  mstring_t tmpString(MMAX_LINE);

  MWikiFromAVP(NULL,Response.c_str(),&tmpString);

  /* Need a char array that can be modified, not the immutable tmpString  */
  char *mutableResponse = NULL;
  MUStrDup(&mutableResponse,tmpString.c_str());

  ptr = MUStrTok(mutableResponse,";",&TokPtr);

  int rc = SUCCESS;

  /* NYI:  change other job attributes such as NodeCount RMXString User/Group */

  while (ptr != NULL)
    {
    ptr2 = MUStrTok(ptr,"=",&TokPtr2);

    aindex = (enum MWJobAttrEnum)MUGetIndexCI(ptr2,MWikiJobAttr,FALSE,mwjaNONE);

    switch (aindex)
      {
      case mwjaAccount:

        if ((J->Credential.A == NULL) || (strcmp(TokPtr2,J->Credential.A->Name)))
          {
          if (MRMJobModify(
                J,
                "Account_Name",
                NULL,
                TokPtr2,
                mSet,
                "account modified by jobvalidate interface",
                tEMsg,              /* O */
                NULL) != SUCCESS)
            {
            MDB(2,fSCHED) MLog("INFO:     cannot set account on job %s to '%s' - invalid account - %s\n",
              J->Name,
              TokPtr2,
              tEMsg);

            if (EMsg != NULL)
              strcpy(EMsg,"cannot set account");

            rc = FAILURE;
            goto cleanupExit;
            }

          MAcctAdd(TokPtr2,&J->Credential.A);

          bmset(&J->IFlags,mjifAccountLocked);
          }

        break;

      case mwjaRClass:

        if ((J->Credential.C == NULL) || (strcmp(TokPtr2,J->Credential.C->Name)))
          {
          mclass_t *C;

          if (MClassFind(TokPtr2,&C) == FAILURE)
            {
            MDB(2,fSCHED) MLog("INFO:     cannot set class on job %s to '%s' - invalid class\n",
              J->Name,
              TokPtr2);

            if (EMsg != NULL)
              strcpy(EMsg,"cannot locate class");

            rc = FAILURE;
            goto cleanupExit;
            }
          
          if (MJobSetClass(J,C,TRUE,tEMsg) == FAILURE)
            {
            MDB(2,fSCHED) MLog("INFO:     cannot set class on job %s to '%s' - %s\n",
              J->Name,
              C->Name,
              tEMsg);

            if (EMsg != NULL)
              strcpy(EMsg,"cannot set class");

            rc = FAILURE;
            goto cleanupExit;
            }

          bmset(&J->IFlags,mjifClassLocked);
          }  /* END if ((J->Cred.C == NULL) || ...) */

        break;

      case mwjaQOS:

        /* bypass qos checks and put qos directly onto job */

        J->QOSRequested = NULL;

        if (MQOSFind(TokPtr2,&J->Credential.Q) != SUCCESS)
          {
          MDB(2,fSCHED) MLog("INFO:     cannot set qos on job %s to '%s' - invalid QoS\n",
            J->Name,
            TokPtr2);

          if (EMsg != NULL)
            strcpy(EMsg,"cannot locate qos");

          rc = FAILURE;
          goto cleanupExit;
          }

        bmset(&J->IFlags,mjifQOSLocked);

        break;

      case mwjaRFeatures:

        MReqSetAttr(J,J->Req[0],mrqaReqNodeFeature,(void **)TokPtr2,mdfString,mSet);

        break;

      case mwjaWCLimit:

        if (MRMJobModify(
             J,
             "Resource_List",
             "walltime",
             TokPtr2,
             mSet,
             "walltime modified by jobvalidate interface",
             tEMsg,
             NULL) == FAILURE)
          {
          MDB(2,fSCHED) MLog("INFO:     cannot modify walltime for rm job %s - '%s'\n",
            J->Name,
            tEMsg);

          if (EMsg != NULL)
            strcpy(EMsg,"cannot modify walltime");

          rc = FAILURE;
          goto cleanupExit;
          }

        break;

      case mwjaVariables:

        MJobSetAttr(J,mjaVariables,(void **)TokPtr2,mdfString,mAdd);

        break;

      default:

        /* NO-OP */
 
        break;
      }  /* END switch (aindex) */

    ptr = MUStrTok(NULL,";",&TokPtr);
    }    /* END while (ptr != NULL) */

cleanupExit:
  MUFree(&mutableResponse);

  return(rc);
  }  /* END MRMJobValidate() */

/* END MRMJobValidate.c */
