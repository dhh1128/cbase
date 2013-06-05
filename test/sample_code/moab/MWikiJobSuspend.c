/* HEADER */

      
/**
 * @file MWikiJobSuspend.c
 *
 * Contains: MWikiJobSuspend
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  

#include "MWikiInternal.h"

/**
 *
 *
 * @param J (I) [modified]
 * @param R (I)
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MWikiJobSuspend(

  mjob_t *J,    /* I (modified) */
  mrm_t  *R,    /* I */
  char   *EMsg, /* O (optional,minsize=MMAX_LINE) */
  int    *SC)   /* O (optional) */

  {
  char    Command[MMAX_LINE];

  char   *Response;

  int     tmpSC = 0;

  const char *FName = "MWikiJobSuspend";

  MDB(2,fWIKI) MLog("%s(%s,%s,EMsg,SC)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (R != NULL) ? R->Name : "NULL");

  if (EMsg != NULL) 
    EMsg[0] = '\0';

  if (SC != NULL)
    *SC = 0;

  if ((J == NULL) || (R == NULL))
    {
    return(SUCCESS);
    }

  if ((R->SubType == mrmstSLURM) && (R->Version < 10202))
    {
    enum MStatusCodeEnum tSC;

    char Buf[MMAX_LINE];

    snprintf(Command,sizeof(Command),"scontrol suspend %s",
      J->Name);

    /* if there are multiple SLURM RMs, we need to be sure to access the correct config via scontrol */
    MUSetEnv("SLURM_CONF",R->ConfigFile);

    if (MUReadPipe(Command,NULL,Buf,sizeof(Buf),&tSC) == FAILURE)
      {
      MDB(2,fWIKI) MLog("ALERT:    cannot suspend job '%s' through %s RM - command failed\n",
        J->Name,
        MRMType[R->Type]);

      tmpSC = (int)tSC;

      if (SC != NULL)
        *SC = tmpSC;

      if (EMsg != NULL)
        strcpy(EMsg,"command failed");
      }
    }
  else
    {
    sprintf(Command,"%s%s %s%s",
      MCKeyword[mckCommand],
      MWMCommandList[mwcmdSuspendJob],
      MCKeyword[mckArgs],
      J->Name);

    if (MWikiDoCommand(
          R,
          mrmJobSuspend,
          &R->P[R->ActingMaster],
          FALSE,       /* false */
          R->AuthType,
          Command,
          &Response,   /* O (alloc) */
          NULL,
          FALSE,
          NULL,
          &tmpSC) == FAILURE)
      {
      MDB(2,fWIKI) MLog("ALERT:    cannot suspend job '%s' through %s RM\n",
        J->Name,
        MRMType[R->Type]);

      MUFree(&Response);

      return(FAILURE);
      }

    MUFree(&Response);
    }  /* END else ((R->SubType == mrmstSLURM) && (R->Version < 10202)) */

  if (tmpSC < 0)
    {
    MDB(2,fWIKI) MLog("ALERT:    cannot suspend job '%s' through %s RM\n",
      J->Name,
      MRMType[R->Type]);

    return(FAILURE);
    }

  MDB(2,fWIKI) MLog("INFO:     job '%s' suspended through %s RM\n",
    J->Name,
    MRMType[R->Type]);

  return(SUCCESS);
  }  /* END MWikiJobSuspend() */



/**
 * Resume suspended job via WIKI interface.
 *
 * @see MRMJobResume() - parent
 * @see MWikiJobSuspend() - peer
 *
 * @param J (I)
 * @param R (I)
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MWikiJobResume(

  mjob_t *J,    /* I */
  mrm_t  *R,    /* I */
  char   *EMsg, /* O (optional,minsize=MMAX_LINE) */
  int    *SC)   /* O (optional) */

  {
  char    Command[MMAX_LINE];

  int     tmpSC = 0;
  char   *Response;

  const char *FName = "MWikiJobResume";

  MDB(2,fWIKI) MLog("%s(%s,%s,EMsg,SC)\n",
    FName,
    J->Name,
    R->Name);

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (SC != NULL)
    *SC = 0;

  if ((R->SubType == mrmstSLURM) && (R->Version < 10202))
    {
    enum MStatusCodeEnum tSC;
  
    char Buf[MMAX_LINE];
 
    /* if there are multiple SLURM RMs, we need to be sure to access the correct config via scontrol */
    MUSetEnv("SLURM_CONF",R->ConfigFile);

    snprintf(Command,sizeof(Command),"scontrol resume %s",
      J->Name);
 
    if (MUReadPipe(Command,NULL,Buf,sizeof(Buf),&tSC) == FAILURE)
      {
      MDB(2,fWIKI) MLog("ALERT:    cannot resume job '%s' through %s RM - command failed\n",
        J->Name,
        MRMType[R->Type]);

      tmpSC = (int)tSC;

      if (SC != NULL)
        *SC = tmpSC;

      if (EMsg != NULL)
        strcpy(EMsg,"command failed");
      }
    }
  else
    {
    sprintf(Command,"%s%s %s%s",
      MCKeyword[mckCommand],
      MWMCommandList[mwcmdResumeJob],
      MCKeyword[mckArgs],
      J->Name);

    if (MWikiDoCommand(
          R,
          mrmJobResume,
          &R->P[R->ActingMaster],
          FALSE,
          R->AuthType,
          Command,
          &Response,  /* O (alloc) */
          NULL,
          FALSE,
          NULL,
          &tmpSC) == FAILURE)
      {
      MDB(2,fWIKI) MLog("ALERT:    cannot resume job '%s' through %s RM\n",
        J->Name,
        MRMType[R->Type]);

      MUFree(&Response);

      return(FAILURE);
      }

    MUFree(&Response);
    }

  if (tmpSC < 0)
    {
    MDB(2,fWIKI) MLog("ALERT:    cannot resume job '%s' through %s RM\n",
      J->Name,
      MRMType[R->Type]);

    return(FAILURE);
    }

  MDB(2,fWIKI) MLog("INFO:     job '%s' resumed through %s RM\n",
    J->Name,
    MRMType[R->Type]);

  return(SUCCESS);
  }  /* END MWikiJobResume() */


/* END MWikiJobSuspend.c */
