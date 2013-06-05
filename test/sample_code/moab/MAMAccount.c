/* HEADER */

      
/**
 * @file MAMAccount.c
 *
 * Contains: MAMAccount
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  
#include "mcom.h"




/**
 * Create new account within accounting manager.
 * 
 * @see MAMAccountAddUser() - peer - add user to existing account
 * @see MCredCreate() - parent
 *
 * @param A (I)
 * @param AName (I) 
 * @param UName (I) [optional]
 * @param Allocation (I)
 * @param R (I) [optional]
 * @param Msg (I) [optional]
 * @param Account (O) [optional,minsize=MMAX_NAME]
 * @param S3C (O) [optional] - SSS Status Code Decade
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 */

int MAMAccountCreate(

  mam_t                  *A,
  char                   *AName,
  char                   *UName,
  double                  Allocation,
  mrm_t                  *R,
  const char             *Msg,
  char                   *Account,
  enum MS3CodeDecadeEnum *S3C,
  char                   *EMsg)

  {
  const char *FName = "MAMAccountCreate";

  MDB(3,fAM) MLog("%s(%s,%s,%s,%f,%s,%s,Account,%p,%p)\n",
    FName,
    (A != NULL) ? A->Name : "NULL",
    (AName != NULL) ? AName : "NULL",
    (UName != NULL) ? UName : "NULL",
    Allocation,
    (R != NULL) ? R->Name : "NULL",
    (Msg != NULL) ? Msg : "NULL",
    S3C,EMsg);

  if (S3C != NULL)
    *S3C = ms3cNone;

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (Account != NULL)
    Account[0] = '\0';

  if ((A == NULL) || (AName == NULL))
    {
    MDB(6,fAM) MLog("ERROR:    Invalid object/AM specified in %s\n",
      FName);

    return(FAILURE);
    }

  if ((A->Type == mamtNONE) ||
     ((MSched.Mode != msmNormal) && (MAMTestMode != TRUE)))
    {
    MDB(8,fAM) MLog("ALERT:    AM request ignored in %s\n",
      FName);

    return(SUCCESS);
    }

  if (A->State == mrmsDown)
    {
    MDB(6,fAM) MLog("WARNING:  AM is down in %s\n",
      FName);

    return(FAILURE);
    }

  switch (A->Type)
    {
    case mamtFILE:

      /* NO-OP */

      break;

    case mamtMAM:
    case mamtGOLD:

      {
      mxml_t *RE;
      mxml_t *OE = NULL;

      char *RspBuf = NULL;

      /* create request string, populate S->SE */

      RE = NULL;

      MXMLCreateE(&RE,(char*)MSON[msonRequest]);

      MXMLSetAttr(RE,(char *)MSAN[msanAction],(void *)"Create",mdfString);

      MS3SetObject(RE,"Account",NULL);

      MS3AddSet(RE,"Name",AName,NULL);

      OE = NULL;

      MXMLCreateE(&OE,(char*)MSON[msonOption]);

      MXMLSetAttr(OE,MSAN[msanName],(void *)"CreateFund",mdfString);

      MXMLSetVal(OE,(void *)"True",mdfString);

      MXMLAddE(RE,OE);

      /* submit request */

      if (MAMGoldDoCommand(
            RE,
            &A->P,
            (Account != NULL) ? &RspBuf : NULL,
            S3C,
            EMsg) == FAILURE)
        {
        MDB(2,fAM) MLog("WARNING:  Unable to create account %s\n",
          AName);

        return(FAILURE);
        }

      if (RspBuf != NULL)
        {
        if (Account != NULL)
          {
          char *ptr;

          ptr = strstr(RspBuf,"Auto-generated Fund ");

          if (ptr != NULL)
            {
            ptr += strlen("Auto-generated Fund ");

            sscanf(ptr,"%64s",
              Account);

            if ((ptr = strchr(Account,'<')) != NULL)
              *ptr = '\0';
            }
          }

        MUFree(&RspBuf);
        }  /* END if (RspBuf != NULL) */

      /* account create successful */
      }      /* END BLOCK */

      break;

    case mamtNONE:

      return(SUCCESS);

      /*NOTREACHED*/

      break;

    default:

      return(SUCCESS);

      /*NOTREACHED*/

      break;
    }  /* switch (A->Type) */

  return(SUCCESS);
  }  /* END MAMAccountCreate() */

 



/**
 * Query account within accounting manager.
 *
 * @see MAMAccountCreate() - peer
 * @see MCredCreate() - parent
 *
 * @param A (I)
 * @param AName (I)
 * @param AllocationP (O)
 * @param R (I) [optional]
 * @param Account (O) [optional,minsize=MMAX_NAME]
 * @param S3C (O) [optional] - SSS Status Code Decade
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 */

int MAMAccountQuery(

  mam_t                  *A,       /* I */
  char                   *AName,   /* I */
  double                 *AllocationP, /* O (optional) */
  mrm_t                  *R,       /* I (optional) */
  char                   *Account, /* O (optional,minsize=MMAX_NAME) */
  enum MS3CodeDecadeEnum *S3C,     /* O */
  char                   *EMsg)    /* O (optional,minsize=MMAX_LINE) */

  {
  const char *FName = "MAMAccountQuery";

  MDB(3,fAM) MLog("%s(%s,%s,%s,%s,Account,%p,%p)\n",
    FName,
    (A != NULL) ? A->Name : "NULL",
    (AName != NULL) ? AName : "NULL",
    (AllocationP != NULL) ? "AllocationP" : "NULL",
    (R != NULL) ? R->Name : "NULL",
    S3C,EMsg);

  if (S3C != NULL)
    *S3C = ms3cNone;

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (Account != NULL)
    Account[0] = '\0';

  if ((A == NULL) || (AName == NULL))
    {
    MDB(6,fAM) MLog("ERROR:    Invalid object/AM specified in %s\n",
      FName);

    return(FAILURE);
    }

  if ((A->Type == mamtNONE) ||
     ((MSched.Mode != msmNormal) && (MAMTestMode != TRUE)))
    {
    MDB(8,fAM) MLog("ALERT:    AM request ignored in %s\n",
      FName);

    return(SUCCESS);
    }

  if (A->State == mrmsDown)
    {
    MDB(6,fAM) MLog("WARNING:  AM is down in %s\n",
      FName);

    return(FAILURE);
    }

  switch (A->Type)
    {
    case mamtMAM:
    case mamtGOLD:

      {
      mxml_t *RE;
      mxml_t *OE;

      char *RspBuf = NULL;
      char tmpBuf[1024];

      /* create request string, populate S->SE */

      /* Example: <Request action="Query" actor="scottmo"><Object>Fund</Object><Get name="Id"></Get><Option name="Filter">Account=chemistry</Option></Request> */

      RE = NULL;

      MXMLCreateE(&RE,(char*)MSON[msonRequest]);

      MXMLSetAttr(RE,(char *)MSAN[msanAction],(void *)"Query",mdfString);

      MS3SetObject(RE,"Fund",NULL);

      MS3AddGet(RE,"Id",NULL);

      OE = NULL;

      MXMLCreateE(&OE,(char*)MSON[msonOption]);

      MXMLSetAttr(OE,MSAN[msanName],(void *)"Filter",mdfString);

      sprintf(tmpBuf, "Account=%s", AName);

      MXMLSetVal(OE,(void *)tmpBuf,mdfString);

      MXMLAddE(RE,OE);

      /* submit request */

      if (MAMGoldDoCommand(
            RE,
            &A->P,
            (Account != NULL) ? &RspBuf : NULL,
            S3C,
            EMsg) == FAILURE)
        {
        MDB(2,fAM) MLog("WARNING:  Unable to query account %s\n",
          AName);

        return(FAILURE);
        }

      if (RspBuf != NULL)
        {
        /* Example: <Response actor="scottmo"><Status><Value>Success</Value><Code>000</Code></Status><Count>2</Count><Data><Fund><Id>41</Id></Fund><Fund><Id>42</Id></Fund></Data></Response> */

        if (Account != NULL)
          {
          char *ptr;

          ptr = strstr(RspBuf,"<Fund><Id>");

          if (ptr != NULL)
            {
            ptr += strlen("<Fund><Id>");

            sscanf(ptr,"%64s",
              Account);

            if ((ptr = strchr(Account,'<')) != NULL)
              *ptr = '\0';
            }
          }
        }    /* END if (RspBuf != NULL) */
      }      /* END BLOCK (case mamtMAM) */

      break;

    case mamtFILE:
    default:

      /* NO-OP */

      break;
    }        /* END switch (A->Type) */

  return(SUCCESS);
  }  /* END MAMAccountQuery() */





/**
 * Add new user to existing accounting manager account.
 *
 * @see MAMAccountCreate() - peer - create new account
 * @see MCredCreate() - parent
 *
 * @param A (I)
 * @param AName (I)
 * @param UName (I) [optional]
 * @param PName (I) [optional]
 * @param Allocation (I)
 * @param R (I) [optional]
 * @param Msg (I) [optional]
 * @param S3C (O) [optional] - SSS Status Code Decade
 * @param EMsg (O) [optional]
 */

int MAMAccountAddUser(

  mam_t                  *A,
  char                   *AName,
  char                   *UName,
  char                   *PName,
  double                  Allocation,
  mrm_t                  *R,
  const char             *Msg,
  enum MS3CodeDecadeEnum *S3C,
  char                   *EMsg)

  {
  const char *FName = "MAMAccountAddUser";

  MDB(3,fAM) MLog("%s(%s,%s,%s,%s,%f,%s,%s,%p,%p)\n",
    FName,
    (A != NULL) ? A->Name : "NULL",
    (AName != NULL) ? AName : "NULL",
    (UName != NULL) ? UName : "NULL",
    (PName != NULL) ? PName : "NULL",
    Allocation,
    (R != NULL) ? R->Name : "NULL",
    (Msg != NULL) ? Msg : "NULL",
    S3C,EMsg);

  if (S3C != NULL)
    *S3C = ms3cNone;

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if ((A == NULL) || (AName == NULL) || (UName == NULL))
    {
    MDB(6,fAM) MLog("ERROR:    Invalid object/AM specified in %s\n",
      FName);

    return(FAILURE);
    }

  if ((A->Type == mamtNONE) ||
     ((MSched.Mode != msmNormal) && (MAMTestMode != TRUE)))
    {
    MDB(8,fAM) MLog("ALERT:    AM request ignored in %s\n",
      FName);

    return(SUCCESS);
    }

  if (A->State == mrmsDown)
    {
    MDB(6,fAM) MLog("WARNING:  AM is down in %s\n",
      FName);

    return(FAILURE);
    }

  switch (A->Type)
    {
    case mamtFILE:

      /* NO-OP */

      break;

    case mamtMAM:
    case mamtGOLD:

      {
      mxml_t *RE;

      /* create request string, populate S->SE */

      RE = NULL;

      MXMLCreateE(&RE,(char*)MSON[msonRequest]);

      MXMLSetAttr(RE,(char *)MSAN[msanAction],(void *)"Create",mdfString);

      MS3SetObject(RE,"AccountUser",NULL);

      MS3AddSet(RE,"Name",UName,NULL);

      MS3AddSet(RE,"Account",AName,NULL);

      /* submit request */

      if (MAMGoldDoCommand(RE,&A->P,NULL,S3C,EMsg) == FAILURE)
        {
        MDB(2,fAM) MLog("WARNING:  Unable to add user %s to account %s\n",
          UName,
          AName);

        return(FAILURE);
        }

      /* user add successful */

      /* should we free RE? */

      if ((PName != NULL) && (PName[0] != '\0') && (Allocation >= 0.0))
        {
        char AString[MMAX_NAME];

        sprintf(AString,"%ld",
          (long)Allocation);

        RE = NULL;

        MXMLCreateE(&RE,(char*)MSON[msonRequest]);

        MXMLSetAttr(RE,(char *)MSAN[msanAction],(void *)"Deposit",mdfString);

        MS3SetObject(RE,"Fund",NULL);

        MS3AddChild(RE,(char*)MSON[msonOption],"Id",PName,NULL);

        MS3AddChild(RE,(char*)MSON[msonOption],"Amount",AString,NULL);

        /* submit request */

        if (MAMGoldDoCommand(RE,&A->P,NULL,S3C,EMsg) == FAILURE)
          {
          MDB(2,fAM) MLog("WARNING:  Unable to deposit %f credits to account %s\n",
            Allocation,
            AName);

          return(FAILURE);
          }
        }    /* END if (Allocation >= 0.0) */
      }      /* END BLOCK (case mamtMAM) */

      break;

    case mamtNONE:

      return(SUCCESS);

      /*NOTREACHED*/

      break;

    default:

      return(SUCCESS);

      /*NOTREACHED*/

      break;
    }  /* switch (A->Type) */

  return(SUCCESS);
  }  /* END MAMAccountAddUser() */



/**
 * Get default account for specified user.
 *
 * NOTE:  uses static cache.
 * NOTE:  if A->CreateCred == TRUE, create all AM account/user creds if UName == NULL
 *
 * @see MJobGetAccount() - parent
 *
 * @param UName (I)
 * @param AName (O) [minsize=MMAX_NAME]
 * @param S3C (O) [optional] - SSS Status Code Decade
 */

int MAMAccountGetDefault(
 
  char                   *UName,  /* I */
  char                   *AName,  /* O (minsize=MMAX_NAME) */
  enum MS3CodeDecadeEnum *S3C)    /* O (optional) */
 
  {
  mgcred_t *User;
  mgcred_t *Acct;

  mam_t *AM;

  static long    LastLoadTime = -1;

  const char *FName = "MAMAccountGetDefault";

  MDB(3,fAM) MLog("%s(%s,AName,%p)\n",
    FName,
    (UName != NULL) ? UName : "NULL",
    S3C);

  if (S3C != NULL)
    *S3C = ms3cNone;

  if (AName != NULL)
    AName[0] = '\0';

  AM = &MAM[0];

  if (AM->CreateCred == FALSE)
    {
    if ((UName == NULL) || (AName == NULL))
      {
      return(FAILURE);
      }
    }

  if ((MSched.Mode != msmNormal) && (MAMTestMode != TRUE))
    {
    return(SUCCESS);
    }

  if ((AM->Type != mamtMAM) && (AM->Type != mamtGOLD))
    {
    return(FAILURE);
    }

  if (AM->State == mrmsDown)
    {
    return(FAILURE);
    }

  /* check if default account cached */

  if (LastLoadTime + 300 < (long)MSched.Time)
    {
    /* refresh */

    char *RspBuf = NULL;

    mxml_t *RE;

    int     CTok;

    mxml_t *DE;
    mxml_t *UE;
    mxml_t *NE;
    mxml_t *AE;

    /* FORMAT:  <Request action="Query"><Object>User</Object><Get object="Name"></Get><Get object="DefaultProject"></Get></Request> */

    /* create request string, populate S->SE */

    RE = NULL;

    MXMLCreateE(&RE,(char*)MSON[msonRequest]);

    MXMLSetAttr(RE,(char *)MSAN[msanAction],(void *)"Query",mdfString);

    MS3SetObject(RE,"User",NULL);

    MS3AddGet(RE,"Name",NULL);
    MS3AddGet(RE,"DefaultProject",NULL);

    /* attach XML request to socket */

    if (AM->P.S == NULL)
      { 
      return(FAILURE);
      }

    AM->P.S->SDE = RE;

    MS3DoCommand(&AM->P,NULL,&RspBuf,NULL,NULL,NULL);

    MDB(3,fAM) MLog("INFO:     Account query response '%s'\n",
      (RspBuf != NULL) ? RspBuf : "NULL");

    /* process response */
  
    DE = (mxml_t *)AM->P.S->RDE;

    if (DE == NULL)
      {
      /* data not available */

      MUFree(&RspBuf);

      return(FAILURE);
      }

    /* FORMAT:  <Data><User><Name>X</Name><DefaultProject>Y</DefaultProject></User></Data> */

    CTok = -1;

    while (MXMLGetChild(DE,"User",&CTok,&UE) == SUCCESS)
      {
      if (MXMLGetChild(UE,"Name",NULL,&NE) == FAILURE)
        continue;

      if ((NE->Val == NULL) || (NE->Val[0] == '\0'))
        continue;

      if (!strcmp(NE->Val,"$ANY")        ||
          !strcmp(NE->Val,"$NONE")       ||
          !strcmp(NE->Val,"$MEMBER")     ||
          !strcmp(NE->Val,"$DEFINED")    ||
          !strcmp(NE->Val,"$SPECIFIED"))
        {
        continue;
        }

      if (MXMLGetChild(UE,"DefaultProject",NULL,&AE) == FAILURE)
        continue;

      if ((AE->Val == NULL) || (AE->Val[0] == '\0'))
        continue;

      if ((MUserAdd(NE->Val,&User) == SUCCESS) &&
          (MAcctAdd(AE->Val,&Acct) == SUCCESS))
        {
        User->F.ADef = Acct;
        MULLAdd(&(User->F.AAL),Acct->Name,(void *)Acct,NULL,NULL);
        }
      }    /* END while (MXMLGetChild(DE,"User",&CTok,&UE) == SUCCESS) */

    LastLoadTime   = MSched.Time;

    MUFree(&RspBuf);
    }    /* END if (LastLoadTime + 300 < (long)MSched.Time) */

  if (UName == NULL)
    {
    return(SUCCESS);
    }

  if ((MUserFind(UName,&User) == SUCCESS) &&
      (User->F.ADef != NULL))
    {
    MUStrCpy(AName,User->F.ADef->Name,MMAX_NAME);

    return(SUCCESS);
    }
   
  /* NOTE: cache SHOULD contain all user/account mappings */

  if (AM->FallbackAccount[0] == '\0')
    {
    return(FAILURE);
    }

  if (AName != NULL)
    strcpy(AName,AM->FallbackAccount);

   MDB(3,fAM) MLog("INFO:     Default account '%s' located for user '%s'\n",
    AName,
    UName);

  return(SUCCESS);
  }  /* END MAMAccountGetDefault() */

/* END MAMAccount.c */
