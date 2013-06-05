/* HEADER */

      
/**
 * @file MStatFileFunctions.c
 *
 * Contains: MStatFileFunctions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  
#include "MEventLog.h"


/**
 * Determine name for current stat file.
 *
 * @param StatTime (I)
 * @param FName (O) [minsize=BufLen]
 * @param BufLen (I)
 */

int MStatGetFName(

  long  StatTime,  /* I */
  char *FName,     /* O (minsize=BufLen) */
  int   BufLen)    /* I */

  {
  char    tmpSLine[MMAX_LINE];

  char   *ptr;
  char   *TokPtr = NULL;

  char   *BPtr = NULL;
  int     BSpace = 0;

  time_t  tmpTime;

  if (FName == NULL)
    {
    return(FAILURE);
    }

  tmpTime = (time_t)StatTime;

  strcpy(tmpSLine,ctime(&tmpTime));

  MUSNInit(&BPtr,&BSpace,FName,BufLen);

  /* TIME FORMAT:  WWW MMM DD TTTT YYYY */

  /* FILE FORMAT:  WWW_MMM_DD_YYYY (ie, Thu_Nov_27_2003) */

  /* get day of week */

  ptr = MUStrTok(tmpSLine," \t\n",&TokPtr);

  MUSNCat(&BPtr,&BSpace,ptr);

  /* get month */

  ptr = MUStrTok(NULL," \t\n",&TokPtr);

  MUSNPrintF(&BPtr,&BSpace,"_%s",
    ptr);

  /* get day of month */

  ptr = MUStrTok(NULL," \t\n",&TokPtr);

  MUSNPrintF(&BPtr,&BSpace,"_%02d",
    (int)strtol(ptr,NULL,10));

  /* ignore time */

  ptr = MUStrTok(NULL," \t\n",&TokPtr);

  /* get year */

  ptr = MUStrTok(NULL," \t\n",&TokPtr);

  MUSNPrintF(&BPtr,&BSpace,"_%s",
    ptr);

  return(SUCCESS);
  }  /* END MStatGetFName() */





/* Gets the next events file using StatTime as a reference.  
 * If there is no events file for the StatsTime, this method 
 * will search for the next events file chronologically till
 * the EndTime is reached.
 * 
 * The StatTime is updated to reflect the time of the events 
 * file found, or the day after the EndTime if no file was found.
 *
 * @param StatTime  (I/O)
 * @param EndTime   (I)
 * @param EFileName (O)
 * 
 */

int MStatGetFullNameSearch(

  mulong *StatTime,   /* I/O */
  long    EndTime,    /* I */
  char   *EFileName)  /* O (minsize=MMAX_LINE) */

  {
  struct  stat sbuf;
  int     rc;

  EndTime = MIN((mulong)EndTime,MSched.Time);

  /* If the StatTime is later than the day after the EndTime */
  /* NOTE: day after EndTime because events are stored by the entire day */

  while (*StatTime < (mulong)(EndTime + MCONST_DAYLEN))
    {
    MStatGetFullName(*StatTime,EFileName);

    rc = stat(EFileName,&sbuf);

    if (rc >= 0)
      {
      /* File exists for this time */

      return(SUCCESS);
      }

    /* File does not exist, try next day */

    *StatTime = (*StatTime + MCONST_DAYLEN);
    } /* END while (*StatTime > (EndTime + MCONST_DAYLEN)) */

  return(FAILURE);
  } /* END MStatGetFullNameSearch() */




int MStatGetFullName(

  long  StatTime,   /* I */
  char *EFileName)  /* O (minsize=MMAX_LINE) */

  {
  char  Suffix[MMAX_LINE];

  const char *FName = "MStatGetFullName";

  MDB(7,fSTAT) MLog("%s(%ld,EFileName)\n",
    FName,
    StatTime);

  /* FORMAT:  WWW_MMM_DD_YYYY */

  MStatGetFName(StatTime,Suffix,sizeof(Suffix));

  /* create event file */

  sprintf(EFileName,"%sevents.%s",
    MStat.StatDir,
    Suffix);

  return(SUCCESS);
  }  /* END MStatGetFullName() */





/**
 * Open new statistics file.
 *
 * @param StatTime (I)
 * @param FP (O) [optional]
 */

int MStatOpenFile(
 
  long   StatTime,  /* I */
  FILE **FP)        /* O (optional) */  
 
  {
  char  EventFile[MMAX_LINE];

  int   flags;
 
  const char *FName = "MStatOpenFile";
 
  MDB(3,fSTAT) MLog("%s(%ld,%s)\n",
    FName, 
    StatTime,
    (FP != NULL) ? "FP" : "NULL");

  if (FP != NULL)
    *FP = NULL;

  MStatGetFullName(StatTime,EventFile);
 
  /* create event file */

  if (FP == NULL)
    { 
    if ((MStat.eventfp != NULL) && 
        (MStat.eventfp != mlog.logfp) &&
        (MStat.eventfp != stderr))
      {
      MDB(5,fSTAT) MLog("INFO:     closing old stat file\n");
 
      fclose(MStat.eventfp);

      MStat.eventfp = NULL;
      }
    }    /* END if (FP == NULL) */

  if (FP != NULL)
    {
    *FP = fopen(EventFile,"r");

    if (*FP == NULL)
      {
      return(FAILURE);
      }

    return(SUCCESS);
    }

  /* external file pointer not specified - update global event file */

  MStat.eventfp = fopen(EventFile,"a+");

  MDB(1,fSTAT) MLog("INFO:     opening event file: '%s'\n",EventFile);

  if (MStat.eventfp == NULL)
    {
    MDB(0,fSTAT) MLog("WARNING:  cannot open statfile '%s', errno: %d (%s)\n",
      EventFile,
      errno,
      strerror(errno));
 
    /* on failure, send stats to logfile */ 
 
    if (mlog.logfp != NULL)
      MStat.eventfp = mlog.logfp;
    else
      MStat.eventfp = stderr;
 
    return(FAILURE);
    }

  /* remove old event files */

  if (MStat.EventFileRollDepth > 0)
    {
    MURemoveOldFilesFromDir(MStat.StatDir,(MCONST_DAYLEN * MStat.EventFileRollDepth),FALSE,NULL);
    }

  flags = fcntl(fileno(MStat.eventfp),F_GETFD,0);

  if (flags >= 0)
    {
    flags |= FD_CLOEXEC;

    fcntl(fileno(MStat.eventfp),F_SETFD,flags);
    }

  return(SUCCESS);
  }  /* END MStatOpenFile() */





/**
 * Report filename for 'compat' stat file.
 *
 * NOTE: file format is YYYYMMDD 
 *
 * @param FName (O) [minsize=MMAX_LINE]
 * @param StatTime (I)
 */

static char *MStatGetCompatFName(
 
  char *FName,    /* O (minsize=MMAX_LINE) */
  long  StatTime) /* I */

  {
  struct tm *ptm;
  time_t tmpTime;

  if (FName == NULL)
    {
    return(NULL);
    }

  tmpTime = (time_t)StatTime;

  ptm = localtime(&tmpTime);

  sprintf(FName,"/%04d%02d%02d", 
    ptm->tm_year + 1900,
    ptm->tm_mon + 1, 
    ptm->tm_mday);

  return(FName);
  }  /* END MStatGetCompatFName() */





/**
 * Open 'compat-style' statistics file.
 *
 * @param StatTime (I)
 */

int MStatOpenCompatFile(
 
  long StatTime)  /* I */
 
  {
  char  EventFile[MMAX_LINE];
  char  Suffix[MMAX_LINE];

  int   flags;
 
  const char *FName = "MStatOpenCompatFile";
 
  MDB(2,fSTAT) MLog("%s(%ld)\n",
    FName, 
    StatTime);
 
  /* FORMAT:  WWW_MMM_DD_YYYY */
 
  MStatGetCompatFName(Suffix,StatTime);

  /* create event file */
 
  sprintf(EventFile,"%s%s",
    MSched.PBSAccountingDir,
    Suffix);
 
  if ((MStat.eventcompatfp != NULL) && 
      (MStat.eventcompatfp != mlog.logfp) &&
      (MStat.eventcompatfp != stderr))
    {
    MDB(5,fSTAT) MLog("INFO:     closing old stat file\n");
 
    fclose(MStat.eventcompatfp);

    MStat.eventcompatfp = NULL;
    }
 
  if ((MStat.eventcompatfp = fopen(EventFile,"a+")) == NULL)
    {
    MDB(0,fSTAT) MLog("WARNING:  cannot open statfile '%s', errno: %d (%s)\n",
      EventFile,
      errno,
      strerror(errno));
 
    /* on failure, send stats to logfile */ 
 
    if (mlog.logfp != NULL)
      MStat.eventcompatfp = mlog.logfp;
    else
      MStat.eventcompatfp = stderr;
 
    return(FAILURE);
    }

  flags = fcntl(fileno(MStat.eventcompatfp),F_GETFD,0);

  if (flags >= 0)
    {
    flags |= FD_CLOEXEC;

    fcntl(fileno(MStat.eventcompatfp),F_SETFD,flags);
    }

  if (MSched.Iteration <= 0)
    {
    MSysRecordEvent(mxoSched,MSched.Name,mrelSchedStart,NULL,NULL,NULL);

    if (MSched.State == mssmPaused)
      {
      MSysRecordEvent(
        mxoSched,
        MSched.Name,
        mrelSchedPause,
        NULL,
        (char *)"scheduler paused at startup",
        NULL);

      CreateAndSendEventLogWithMsg(meltSchedPause,MSched.Name,mxoSched,(void *)&MSched,"scheduler paused at startup");
      }
    else if (MSched.State == mssmStopped)
      {
      MSysRecordEvent(
        mxoSched,
        MSched.Name,
        mrelSchedStop,
        NULL,
        (char *)"scheduler stopped at startup",
        NULL);

      CreateAndSendEventLogWithMsg(meltSchedStop,MSched.Name,mxoSched,(void *)&MSched,"scheduler stopped at startup");
      }
    }
 
  return(SUCCESS);
  }  /* END MStatOpenCompatFile() */





/**
 * Write profile statistics to file.
 * 
 * @see MServerUpdate() - parent
 * @see MCredProfileToXML() - child
 * @see MNodeProfileToXML() - child
 * @see MTJobStatProfileToXML() - child
 *
 * @param StatTime (I)
 * @param Period (I)
 */

int MStatWritePeriodPStats(

  long                StatTime,  /* I */
  enum MCalPeriodEnum Period)    /* I */

  {
  char *tmpBuffer;

  char  StatFile[MMAX_LINE];
  char  Suffix[MMAX_LINE];

  mxml_t *DE;

  int     oindex;
  int     jindex;

  void   *O;
  void   *OP;

  void   *OEnd;

  char   *NameP;

  long    STime;
  long    ETime;

  int     rc;

  mnode_t *N;

  mhashiter_t HTI;

  const enum MXMLOTypeEnum OList[] = {
    mxoGroup,
    mxoUser,
    mxoClass,
    mxoQOS,
    mxoAcct,
    mxoSched,
    mxoNONE };

  const char *FName = "MStatWritePeriodPStats";

  MDB(2,fSTAT) MLog("%s(%ld,%s)\n",
    FName,
    StatTime,
    MCalPeriodType[Period]);

  /* FORMAT:  WWW_MMM_DD_YYYY */

  MUGetPeriodRange(StatTime,0,0,Period,&STime,&ETime,NULL);

  MStatGetFName(
    STime,
    Suffix, 
    sizeof(Suffix));

  /* create Profile file */

  sprintf(StatFile,"%s%s.%s",
    MStat.StatDir,
    MCalPeriodType[Period],
    Suffix);

  if (MFUGetInfo(StatFile,NULL,NULL,NULL,NULL) == SUCCESS)
    {
    /* file already exists */

    return(SUCCESS); 
    }

  DE = NULL;

  switch (Period)
    {
    case mpDay:

      MXMLCreateE(&DE,(char*)MSON[msonData]);

      /* STime and ETime already calculated above for mpDay */

      /* loop through all credentials to get statistics */

      for (oindex = 0;OList[oindex] != mxoNONE;oindex++)
        {
        MOINITLOOP(&OP,OList[oindex],&OEnd,&HTI);

        while ((O = MOGetNextObject(&OP,OList[oindex],OEnd,&HTI,&NameP)) != NULL)
          {
          if ((NameP == NULL) ||
              !strcmp(NameP,NONE) ||
              !strcmp(NameP,ALL) ||
              !strcmp(NameP,"ALL") ||
              !strcmp(NameP,"DEFAULT") ||
              !strcmp(NameP,"NOGROUP"))
            {
            continue;
            }

          MCredProfileToXML(
            OList[oindex],
            NameP,
            "Stat",
            STime,
            ETime - 1,
            ETime - 1,
            NULL,
            FALSE,
            FALSE,
            DE);
          }   /* while ((O = MOGetNextObject()) != NULL) */
        }     /* END for (oindex) */

      for (oindex = 0;oindex < MSched.M[mxoNode];oindex++)
        {
        N = MNode[oindex];

        if ((N == NULL) || (N->Name[0] == '\0'))
          break;
          
        if (N->Name[0] == '\1')
          continue;

        if ((MSched.DefaultN.ProfilingEnabled == TRUE) ||
            (N->ProfilingEnabled == TRUE))
          {
          MNodeProfileToXML(
            N->Name,
            N,
            NULL,
            NULL,
            STime,
            ETime - 1, 
            ETime - 1,
            DE, 
            FALSE,
            FALSE,
            FALSE);
          }
        }

      for (jindex = 0;MUArrayListGet(&MTJob,jindex) != NULL;jindex++)
        {
        mjob_t *TJ;

        TJ = *(mjob_t **)(MUArrayListGet(&MTJob,jindex));

        if (TJ == NULL)
          break;

        if (TJ->ExtensionData == NULL)
          continue;

        MTJobStatProfileToXML(
          TJ,
          NULL,
          NULL,
          STime,
          ETime - 1,
          ETime - 1,
          FALSE,
          FALSE,
          DE);
        }

      break;

    default:

      /* NOT SUPPORTED */

      MDB(0,fSTAT) MLog("WARNING:  cannot process period type %s stats\n",
        MCalPeriodType[Period]);

      MXMLDestroyE(&DE);

      return(FAILURE);
    }  /* END switch (Period) */

  tmpBuffer = NULL;

  /* NOTE:  will need to increase buffer size for quarterly and yearly stats */

  rc = MXMLToXString(
        DE,
        &tmpBuffer,
        NULL,
        MMAX_BUFFER << 8,  /* allow up to 16 MB stats files */
        NULL,
        TRUE);

  MXMLDestroyE(&DE);

  if (rc == FAILURE)
    {
    /* could not generate stats buffer */

    MDB(0,fSTAT) MLog("WARNING:  cannot record %s stats - buffer too small\n",
      MCalPeriodType[Period]);

    MUFree(&tmpBuffer);

    return(FAILURE);
    }

  rc = MFUCreate(
         StatFile,
         NULL,
         tmpBuffer,
         strlen(tmpBuffer),
         -1,
         -1,
         -1,
         TRUE,
         NULL);

  MUFree(&tmpBuffer);

  if (rc == FAILURE)
    {
    /* cannot write stats file */

    MDB(0,fSTAT) MLog("WARNING:  cannot create statfile '%s'\n",
      StatFile);

    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MStatWritePeriodPStats() */

/* END MStatFileFunctions.c */
