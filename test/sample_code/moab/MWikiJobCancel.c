/* HEADER */

      
/**
 * @file MWikiJobCancel.c
 *
 * Contains: MWikiJobCancel
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  

#include "MWikiInternal.h"



/**
 * Cancel job via WIKI interface.
 * 
 * @see MRMJobCancel() - parent
 * @see MWikiDoCommand() - child
 *
 * @param J (I)
 * @param R (I)
 * @param Message (I) [opt]
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MWikiJobCancel(

  mjob_t               *J,        /* I */
  mrm_t                *R,        /* I */
  char                 *Message,  /* I (opt) */
  char                 *EMsg,     /* O (optional,minsize=MMAX_LINE) */
  enum MStatusCodeEnum *SC)       /* O (optional) */

  {
  char    Command[MMAX_LINE];

  int     tmpSC;
  char   *Response;

  char    Comment[MMAX_LINE];

  char    CancelType[MMAX_NAME];

  char   *BPtr;
  int     BSpace;

  const char *FName = "MWikiJobCancel";

  MDB(2,fWIKI) MLog("%s(%s,%s,CMsg,EMsg,SC)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (R != NULL) ? R->Name : "NULL");

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (SC != NULL)
    *SC = mscNoError;

  if ((J == NULL) || (R == NULL))
    {
    return(FAILURE);
    }

  Comment[0] = '\0';

  /* fail in TEST mode */

  if ((MSched.Iteration == R->FailIteration) || (MSched.Mode == msmMonitor))
    {
    if (MSched.Mode == msmMonitor)
      {
      MDB(3,fWIKI) MLog("INFO:     cannot cancel job '%s' (monitor mode)\n",
        J->Name);

      sleep(2);
      }
    else
      {
      MDB(3,fWIKI) MLog("INFO:     cannot cancel job '%s' (fail iteration)\n",
        J->Name);
      }

    return(FAILURE);
    }

  if ((Message != NULL) && (strstr(Message,"wallclock") != NULL))
    strcpy(CancelType,"WALLCLOCK");
  else
    strcpy(CancelType,"ADMIN");

  MUSNInit(&BPtr,&BSpace,Command,sizeof(Command));

  MUSNPrintF(&BPtr,&BSpace,"%s%s %s%s TYPE=%s",
    MCKeyword[mckCommand],
    MWMCommandList[mwcmdCancelJob],
    MCKeyword[mckArgs],
    (J->DRMJID != NULL) ? J->DRMJID : J->Name,
    CancelType);

  if (Comment[0] != '\0')
    {
    /* add job cancel comment */

    MUSNPrintF(&BPtr,&BSpace," COMMENT=%s",
      Comment);
    }

  if (MWikiDoCommand(
        R,
        mrmJobCancel,
        &R->P[R->ActingMaster],
        FALSE,
        R->AuthType,
        Command,
        &Response,  /* O (alloc) */
        NULL,
        FALSE,
        EMsg,
        &tmpSC) == FAILURE)
    {
    MDB(2,fWIKI) MLog("ALERT:    cannot cancel job '%s' through %s RM\n",
      J->Name,
      MRMType[R->Type]);

    if ((EMsg != NULL) && (EMsg[0] == '\0'))
      {
      strcpy(EMsg,"rm failure");
      }

    R->FailIteration = MSched.Iteration;

    return(FAILURE);
    }

  MUFree(&Response);

  if (tmpSC < 0)
    {
    mbool_t IsFailure;

    MDB(2,fWIKI) MLog("ALERT:    cannot cancel job '%s' through %s RM\n",
      J->Name,
      MRMType[R->Type]);

    switch (tmpSC)
      {
      case -2021:  /* job finished */

        IsFailure = FALSE;

        break;

      case -1:     /* general error */
      case -2017:  /* bad job id */
      case -2010:  /* job has no user id */
      case -2009:  /* job script missing */
      case -2020:  /* bad state */
      default:

        IsFailure = TRUE;

        break;
      }

    if (IsFailure == TRUE)
      { 
      if ((EMsg != NULL) && (EMsg[0] == '\0'))
        {
        sprintf(EMsg,"rm failed with rc=%d",
          tmpSC);
        }

      R->FailIteration = MSched.Iteration;

      return(FAILURE);
      }
    }  /* END if (tmpSC < 0) */

  MDB(2,fWIKI) MLog("INFO:     job '%s' cancelled through %s RM\n",
    J->Name,
    MRMType[R->Type]);

  return(SUCCESS);
  }  /* END MWikiJobCancel() */

/* END MWikiJobCancel.c */
