/* HEADER */

/**
 * @file MTime.c
 *
 * Moab Time
 */
        
/* Contains:                                 *
 *   int MUStringToE(String,EpochTime,IsCanonical) *
 *   long MUTimeFromString(TString)          *
 *                                           */


#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 *
 *
 * @param Time (I/O)
 * @param RefreshMode (I)
 * @param S (I) [optional]
 */

int MUGetTime(

  mulong   *Time,                  /* I/O */
  enum MTimeModeEnum RefreshMode,  /* I */
  msched_t *S)                     /* I (optional) */

  {
  mulong tmpTime = MMAX_TIME;

  time_t tmpT;

  if (Time == NULL)
    {
    return(FAILURE);
    }

  if (((S != NULL) &&
        (S->Mode == msmSim) &&
        (S->TimePolicy != mtpNONE)) ||
       (RefreshMode == mtmRefresh))
    {
    tmpTime = MIN(*Time,MMAX_TIME);
    }

  if (S != NULL)
    {
    if (S->Mode == msmSim)
      {
      if (S->TimePolicy != mtpReal)
        {
        switch (RefreshMode)
          {
          case mtmNONE:
          default:

            /* no action necessary */

            tmpTime = S->Time;

            break;

          case mtmRefresh:

            /* refresh */

            tmpTime += S->MaxRMPollInterval;

            break;

          case mtmInit:

            /* initialize (load real time) */

            time(&tmpT);

            tmpTime = (mulong)tmpT;
  
            break;
          }  /* END switch (RefreshMode) */
        }
      else
        {
        tmpTime = S->Time;
        }

      *Time = tmpTime;

      return(SUCCESS);
      }  /* END if (S->Mode == msmSim) */

    if (RefreshMode == mtmInternalSchedTime)
      {
      /* assign internal scheduler time */

      *Time = S->Time;

      return(SUCCESS);
      }
    }    /* END if (S != NULL) */

  /* load real time */

  time(&tmpT);

  tmpTime = (mulong)tmpT;

  *Time = tmpTime;

  return(SUCCESS);
  }  /* END MUGetTime() */


/**
 * Convert string to epoch time
 *
 * @see MUTimeFromString() - peer
 *
 * NOTE:  handles 'NOW', relative (ie '+XXX'), epoch (ie '1213139463') and 
 *        absolute (ie, 'HH[:MM[:SS]][_MM[/DD[/YY]]]') formats
 *
 * String (I) (not modified) 
 * EpochTime (O)
 * IsCanonical (I)
 */

int MUStringToE(

  char    *String,      /* I */
  long    *EpochTime,   /* O */
  mbool_t  IsCanonical) /* I */

  {
  char       Second[MMAX_NAME];
  char       Minute[MMAX_NAME];
  char       Hour[MMAX_NAME];
  char       Day[MMAX_NAME];
  char       Month[MMAX_NAME];
  char       Year[MMAX_NAME];
  char       TZ[MMAX_NAME];

  char       StringTime[MMAX_NAME];
  char       StringDate[MMAX_NAME];
  char       Line[MMAX_LINE];

  char      *ptr;
  char      *tail;

  int        index;

  struct tm  Time;
  struct tm *DefaultTime;

  mulong     ETime;             /* calculated epoch time */
  mulong     Now = 0;

  time_t     tmpTime;

  int        YearVal;

  char      *TokPtr;

  long       tmpL;

  const char *FName = "MUStringToE";

  MDB(2,fCONFIG) MLog("%s(%s,EpochTime,IsCanonical)\n",
    FName,
    String);

  MUGetTime(&Now,mtmInternalSchedTime,&MSched);

  *EpochTime = MMAX_TIME;

  /* check 'NOW' keyword */

  if (!strcmp(String,"NOW"))
    {
    *EpochTime = Now;
   
    return(SUCCESS);
    }

  ETime = strtol(String,NULL,10);

  if ((ptr = strchr(String,'+')) != NULL)
    {
    /* relative time specified */

    /* Format [ +d<DAYS> ][ +h<HOURS> ][ +m<MINUTES> ][ +s<SECONDS> ] */

    ETime = Now + MUTimeFromString(ptr + 1);
    }
  else if ((IsCanonical == TRUE) &&
           (ETime > 23) &&
           !strchr(String,':') && 
           !strchr(String,'/') &&
           !strchr(String,'_'))
    {
    /* epoch time specified */

    /* NO-OP */
    }
  else
    { 
    time_t tmpTime;

    /* formatted absolute time specified */

    /* Format:  HH[:MM[:SS]][_MM[/DD[/YY]]] */
    /*   or     MM/DD/YY                    */

    setlocale(LC_TIME,"en_US.iso88591");

    tmpTime = Now;

    DefaultTime = localtime(&tmpTime);

    /* copy default values into time structure */

    strcpy(Second,"00");
    strcpy(Minute,"00");
    strftime(Hour  ,MMAX_NAME,"%H",DefaultTime);

    strftime(Day   ,MMAX_NAME,"%d",DefaultTime);
    strftime(Month ,MMAX_NAME,"%m",DefaultTime);
  
    strftime(Year  ,MMAX_NAME,"%Y",DefaultTime);
  
    strftime(TZ    ,MMAX_NAME,"%Z",DefaultTime);

    if (IsCanonical == TRUE)
      {
      if ((tail = strchr(String,'_')) != NULL)
        {
        /* time and date specified */

        /* Check format */

        for (index = 1; index < (int) strlen(tail); index++)
          {
          if (!isdigit(tail[index]) && (tail[index] != '/'))
            {
            /* cannot have ':' after '_' */

            return(FAILURE);
            }
          }

        strncpy(StringTime,String,(tail - String));
        StringTime[(tail - String)] = '\0';

        for (index = 0; index < (int) strlen(StringTime); index++)
          {
          if (!isdigit(StringTime[index]) && (StringTime[index] != ':'))
            {
            /* cannot have '/' before '_' */

            return(FAILURE);
            }
          }

        MUStrCpy(StringDate,(tail + 1),sizeof(StringDate));
        }
      else if (strchr(String,'/') != NULL)
        {
        /* date only specified */

        for (index = 0; index < (int) strlen(String); index++)
          {
          if (!isdigit(String[index]) && (String[index] != '/'))
            {
            /* invalid character in date string */

            return(FAILURE);
            }
          }

        MUStrCpy(StringDate,String,sizeof(StringDate));

        StringTime[0] = '\0';
        }
      else
        {
        /* time only specified */

        MUStrCpy(StringTime,String,sizeof(StringTime));

        for (index = 0; index < (int) strlen(StringTime); index++)
          {
          if (!isdigit(StringTime[index]) && (StringTime[index] != ':'))
            {
            /* invalid character in time string */

            return(FAILURE);
            }
          }

        StringDate[0] = '\0';
        }
 
      MDB(7,fCONFIG) MLog("INFO:     time: '%s'  date: '%s'\n",
        StringTime,
        StringDate);

      if (StringDate[0] != '\0')
        {      
        /* parse date */

        /* FORMAT:  MM[/DD[/[YY]YY]]] */

        if ((ptr = MUStrTok(StringDate,"/",&TokPtr)) != NULL)
          {
          strcpy(Month,ptr);

          if ((ptr = MUStrTok(NULL,"/",&TokPtr)) != NULL)
            {
            strcpy(Day,ptr);

            if ((ptr = MUStrTok(NULL,"/",&TokPtr)) != NULL)
              {
              YearVal = (int)strtol(ptr,NULL,10);

              if (YearVal < 97)
                {
                sprintf(Year,"%d",
                  YearVal + 2000);
                }
              else if (YearVal < 1900) 
                {
                sprintf(Year,"%d",
                  YearVal + 1900);
                }
              else 
                {
                sprintf(Year,"%d",
                  YearVal);
                }
              }
            }
          }
        }    /* END if (StringDate[0] != '\0') */

      /* parse time */

      if (StringTime[0] != '\0')
        {
        /* FORMAT:  HH[:MM[:SS]] */

        if ((ptr = MUStrTok(StringTime,":_",&TokPtr)) != NULL)
          {
          strcpy(Hour,ptr);

          if ((ptr = MUStrTok(NULL,":_",&TokPtr)) != NULL)
            {
            strcpy(Minute,ptr);

            if ((ptr = MUStrTok(NULL,":_",&TokPtr)) != NULL)
              strcpy(Second,ptr);
            }
          }
        }    /* END if (StringTime[0] != '\0') */
      }
    else
      {
      int len;

      char *ptr;
      char *TokPtr = NULL;

      /* FORMAT:  [[[[CC]YY]MM]DD]hhmm[.SS] */

      if ((ptr = MUStrTok(String,".",&TokPtr)) == NULL)
        {
        return(FAILURE);
        }

      len = strlen(ptr);

      if (len < 4)
        {
        return(FAILURE);
        }

      MUStrCpy(Minute,&ptr[len - 2],sizeof(Minute)); 
      ptr[len - 2] = '\0';

      MUStrCpy(Hour,&ptr[len - 4],sizeof(Hour));
      ptr[len - 4] = '\0';

      if (len > 6)
        {
        MUStrCpy(Day,&ptr[len - 6],sizeof(Day));
        ptr[len - 6] = '\0';
        }

      if (len > 8)
        {
        MUStrCpy(Month,&ptr[len - 8],sizeof(Month));
        ptr[len - 8] = '\0';
        }

      if (len > 12)
        {
        strcpy(Year,ptr);
        }
      else if (len > 10)
        {
        sprintf(Year,"20%s",
          &ptr[len - 10]);
        }

      ptr = MUStrTok(NULL,".",&TokPtr);

      if (ptr != NULL)
        MUStrCpy(Second,ptr,sizeof(Second)); 
      }  /* END else (IsCanonical == TRUE) */

    /* create time string */

    sprintf(Line,"%s:%s:%s %s/%s/%s %s",
      Hour,
      Minute,
      Second,
      Month,
      Day,
      Year,
      TZ);

    /* perform bounds checking */

    if ((strtol(Second,NULL,10) > 59) || 
        (strtol(Minute,NULL,10) > 59) || 
        (strtol(Hour,NULL,10)   > 23) || 
        (strtol(Month,NULL,10)  > 12) || 
        (strtol(Day,NULL,10)    > 31) || 
        (strtol(Year,NULL,10)   > 2097))
      {
      MDB(1,fCONFIG) MLog("ERROR:    invalid time specified '%s' (bounds exceeded)\n",
        Line);

      return(FAILURE);
      }

    memset(&Time,0,sizeof(Time));

    Time.tm_hour = (int)strtol(Hour,NULL,10);
    Time.tm_min  = (int)strtol(Minute,NULL,10);
    Time.tm_sec  = (int)strtol(Second,NULL,10);
    Time.tm_mon  = (int)strtol(Month,NULL,10) - 1;
    Time.tm_mday = (int)strtol(Day,NULL,10);
    Time.tm_year = (int)strtol(Year,NULL,10) - 1900;

    /* adjust for TZ */

    Time.tm_isdst = -1;

    /* place current time into tm structure */

    MDB(5,fCONFIG) MLog("INFO:     generated time line: '%s'\n",
      Line);

    /* strptime(Line,"%T %m/%d/%Y %Z",&Time); */

    if ((tmpL = mktime(&Time)) == -1)
      {
      MDB(5,fCONFIG) MLog("ERROR:    cannot determine epoch time for '%s', errno: %d (%s)\n",
        Line,
        errno,
        strerror(errno));

      return(FAILURE);
      }

    ETime = (mulong)tmpL;
    }  /* END else (strchr(String,'+')) */

  tmpTime = (time_t)Now;

  MDB(3,fCONFIG) MLog("INFO:     current   epoch:  %lu  time:  %s\n",
    Now,
    ctime(&tmpTime));

  tmpTime = (time_t)ETime;

  MDB(3,fCONFIG) MLog("INFO:     calculated epoch: %lu  time:  %s\n",
    ETime,
    ctime(&tmpTime));

  *EpochTime = ETime;

  return(SUCCESS);
  }  /* END MUStringToE() */





/**
 * Converts a relative, absolute, or epoch time based string representation to
 * epoch time or relative time.  
 *
 * NOTE: relative time format: [{+|-}][[[DD:]HH:]MM:]SS or [{+|-}days
 * NOTE: absolute time format: [{+|-}][HH[:MM[:SS]]][_MO[/DD[/YY]]]  
 *
 * NOTE: if relative time format is specified, routine with return time
 *       'delta' in seconds, not epoch time.
 *
 * NOTE: should this routine support conversion of '-1' to infinity?
 *
 * @see MStatGetRange() - parent
 * 
 * Other keywords have specific meanings.  "NOW" returns the
 * time according to the scheduler.  "INFINITY" returns Moab's
 * infinity (aka MMAX_TIME).  "<integer>days" is also a valid relative
 * time format. 
 *
 * @param TString the string to parse into a time. (I)
 */

long MUTimeFromString(

  char *TString)  /* I */

  {
  long  val;

  char *Dp;
  char *Hp;
  char *Mp;
  char *Sp;

  char *ptr;

  char *TokPtr;

  char  Line[MMAX_LINE];

  mbool_t IsNeg = FALSE;

  const char *FName = "MUTimeFromString";

  MDB(5,fCONFIG) MLog("%s(%s)\n",
    FName,
    (TString != NULL) ? TString : "NULL");

  if (TString == NULL)
    {
    return(0);
    }

  /* FORMAT:  'INFINITY | NOW | <ETIME> | <ABSTIME> | <RELTIME>' */

  if (!strcmp(TString,"INFINITY"))
    {
    return(MMAX_TIME);
    }

  if (!strcmp(TString,"NOW"))
    {
    return(MSched.Time);
    }

  if ((strchr(TString,'_') != NULL) || (strchr(TString,'/') != NULL))
    {
    /* line specified as 'absolute' time */

    if (MUStringToE(TString,&val,TRUE) == FAILURE)
      {
      /* invalid time specification */

      return(MMAX_TIME);
      }

    MDB(4,fCONFIG) MLog("INFO:     string '%s' specified as absolute time\n",
      TString);
 
    return(val);
    }

  /* time is relative - report as duration */

  MUStrCpy(Line,TString,sizeof(Line));

  if ((ptr = strstr(Line,"days")) != NULL)
    {
    /* line specified in days -> FORMAT: XXX days */

    *ptr = '\0';

    val = (int)strtol(Line,NULL,10) * MCONST_DAYLEN;

    return(val);
    }

  if (!strchr(TString,':') && !strchr(TString,'-') && !strchr(TString,'+'))
    {
    /* epoch time specified */

    val = strtol(TString,NULL,10);

    MDB(4,fCONFIG) MLog("INFO:     string '%s' specified as seconds\n",
      TString);

    return(val);
    }

  /* line specified in 'military' time ->[{+|-}][[[DD:]HH:]MM:]SS */

  ptr = TString;

  /* handle + or - on the front of the specified time */

  if (*ptr == '-')
    IsNeg = TRUE;
    
  if (IsNeg || (*ptr == '+')) 
    ptr++; /* skip over the + or - if present */
 
  MUStrCpy(Line,ptr,sizeof(Line));

  Dp = NULL;
  Hp = NULL;
  Mp = NULL;
  Sp = NULL;

  if ((Dp = MUStrTok(Line,":",&TokPtr)) != NULL)
    {
    if ((Hp = MUStrTok(NULL,":",&TokPtr)) != NULL)
      {
      if ((Mp = MUStrTok(NULL,":",&TokPtr)) != NULL)
        {
        Sp = MUStrTok(NULL,":",&TokPtr);
        }
      }
    }

  if (Dp == NULL)
    {
    MDB(4,fCONFIG) MLog("INFO:     cannot read string '%s'\n",
      TString);

    return(0);
    }

  if (Hp == NULL)
    {
    /* adjust from SS to DD:HH:MM:SS notation */

    Sp = Dp;
    Mp = NULL;
    Hp = NULL;
    Dp = NULL;
    }
  else if (Mp == NULL)
    {
    /* adjust from MM:SS to DD:HH:MM:SS notation */

    Sp = Hp;
    Mp = Dp;
    Hp = NULL;
    Dp = NULL;
    }
  else if (Sp == NULL)
    {
    /* adjust from HH:MM:SS to DD:HH:MM:SS notation */

    Sp = Mp;
    Mp = Hp;
    Hp = Dp;
    Dp = NULL;
    }

  /* fail when there are non numeric elements in the time string */
  /* at this point because the format should be digitdigit:digitdigit...*/

  if ((Dp != NULL) && (MUIsOnlyNumeric(Dp) == FALSE))
    {
    return(-1);
    }

  if ((Hp != NULL) && (MUIsOnlyNumeric(Hp) == FALSE))
    {
    return(-1);
    }

  if ((Mp != NULL) && (MUIsOnlyNumeric(Mp) == FALSE))
    {
    return(-1);
    }

  /* we don't need to check if Sp != NULL because if it did we'd have failed
   * already */

  if (MUIsOnlyNumeric(Sp) == FALSE)
    {
    return(-1);
    }

  val = (((Dp != NULL) ? (int)strtol(Dp,NULL,10) : 0) * MCONST_DAYLEN) +
        (((Hp != NULL) ? (int)strtol(Hp,NULL,10) : 0) * MCONST_HOURLEN) +
        (((Mp != NULL) ? (int)strtol(Mp,NULL,10) : 0) * MCONST_MINUTELEN) +
        (((Sp != NULL) ? (int)strtol(Sp,NULL,10) : 0) * 1);

  if (IsNeg == TRUE)
    val *= -1;

  MDB(4,fCONFIG) MLog("INFO:     string '%s' -> %ld\n",
    TString,
    val);

  return(val);
  }  /* END MUTimeFromString() */






/**
 *
 *
 * @param iTime   (I) - epoch time
 * @param tmpLine (O) - return string (minsize = MMAX_LINE)
 */

int MULToSnapATString(

  long    iTime,
  char   *tmpLine)

  {
  time_t tmpTime;

  struct tm *TP;

  if (tmpLine == NULL)
    return(FAILURE);

  /* FORMAT:  SNAP -> YYYYMMDDHHMMSS */

  tmpLine[0] = '\0';

  tmpTime = (time_t)iTime;

  TP = localtime(&tmpTime);

  sprintf(tmpLine,"%04d%02d%02d%02d%02d%02d",
    TP->tm_year + 1900,
    TP->tm_mon + 1,
    TP->tm_mday,
    TP->tm_hour,
    TP->tm_min,
    TP->tm_sec);

  return(SUCCESS);
  }  /* END MULToSnapATString() */




/**
 *
 *
 * @param String (I)
 * @param RMType (I)
 * @param TimeP (O)
 */

int MUGetRMTime(

  char             *String,  /* I */
  enum MRMTypeEnum  RMType,  /* I */
  long             *TimeP)   /* O */

  {
  char tmpLine[MMAX_LINE];

  int  len;

  if (TimeP != NULL)
    *TimeP = 0;

  if ((String == NULL) || (String[0] == '\0') || (TimeP == NULL))
    {
    return(FAILURE);
    }

  MUStrCpy(tmpLine,String,sizeof(tmpLine));

  switch (RMType)
    {
    case mrmtLL:

      {
      time_t     now;
      struct tm *TP;

      char *TokPtr;
      char *ptr;

      char *sptr;
      char *TokPtr2;

      char  tmpBuf[MMAX_LINE];

      now = (time_t)MSched.Time;

      TP = localtime(&now);

      /* FORMAT:  MM/DD/YYYY HH:MM */

      ptr = MUStrTok(tmpLine," \t\n",&TokPtr);

      while (ptr != NULL)
        {
        strncpy(tmpBuf,ptr,sizeof(tmpBuf));

        if (strchr(tmpBuf,':') != NULL)
          {
          sptr = MUStrTok(ptr,":",&TokPtr2);

          TP->tm_hour = (int)strtol(sptr,NULL,10);

          sptr = MUStrTok(ptr,":",&TokPtr2);

          if (sptr != NULL)
            TP->tm_min = (int)strtol(sptr,NULL,10);
          }
        else if (strchr(tmpBuf,'/') != NULL)
          {
          sptr = MUStrTok(ptr,"/",&TokPtr2);

          TP->tm_mon = (int)strtol(sptr,NULL,10) - 1;

          sptr = MUStrTok(ptr,"/",&TokPtr2);

          if (sptr != NULL)
            TP->tm_mday = (int)strtol(sptr,NULL,10);

          sptr = MUStrTok(ptr,"/",&TokPtr2);

          /* NOTE:  should we handle 'YY' as well as 'YYYY' ?? */

          if (sptr != NULL)
            TP->tm_year = (int)strtol(sptr,NULL,10);
          }

        ptr = MUStrTok(NULL," \t\n",&TokPtr);
        }  /* END while (ptr != NULL) */

      /* adjust for TZ */

      TP->tm_isdst = -1;

      /* place current time into tm structure */

      if ((*TimeP = mktime(TP)) == -1)
        {
        MDB(5,fCONFIG) MLog("ERROR:    cannot determine epoch time for '%s', errno: %d (%s)\n",
          String,
          errno,
          strerror(errno));

        return(FAILURE);
        }
      }    /* END BLOCK (case mrmtLL) */

      break;

    case mrmtPBS:
    default:

      {
      time_t     now;
      struct tm *TP;

      char *TokPtr = NULL;

      char *sptr;

      if (!isdigit(tmpLine[0]))
        {
        MDB(5,fCONFIG) MLog("ERROR:    invalid value '%s' specified as time string\n",
          tmpLine);

        return(FAILURE);
        }

      now = (time_t)MSched.Time;

      TP = localtime(&now);

      /* release time (FORMAT: [[[[CC]YY]MM]DD]hhmm[.SS]) */

      /* convert to epoch */

      MUStrTok(tmpLine,".",&TokPtr);
      sptr = MUStrTok(NULL,".",&TokPtr);

      if (sptr != NULL)
        {
        /* extract seconds */

        TP->tm_sec = (int)strtol(sptr,NULL,10);
        }

      len = strlen(tmpLine);

      if (len >= 2)
        {
        /* extract minutes */

        TP->tm_min = (int)strtol(&tmpLine[len - 2],NULL,10);

        tmpLine[len - 2] = '\0';

        len = strlen(tmpLine);
        }

      if (len >= 2)
        {
        /* extract hour */

        TP->tm_hour = (int)strtol(&tmpLine[len - 2],NULL,10);

        tmpLine[len - 2] = '\0';

        len = strlen(tmpLine);
        }

      if (len >= 2)
        {
        /* extract day */

        TP->tm_mday = (int)strtol(&tmpLine[len - 2],NULL,10);

        tmpLine[len - 2] = '\0';

        len = strlen(tmpLine);
        }

      if (len >= 2)
        {
        /* extract month */

        TP->tm_mon = (int)strtol(&tmpLine[len - 2],NULL,10) - 1;

        tmpLine[len - 2] = '\0';

        len = strlen(tmpLine);
        }

      if (len >= 2)
        {
        int tmpI;

        /* extract year */

        tmpI = (int)strtol(&tmpLine[len - 2],NULL,10);

        if (tmpI < 100)
          {
          TP->tm_year = tmpI + 100;
          }
        else if (tmpI < 1900)
          {
          TP->tm_year = tmpI;
          }
        else
          {
          TP->tm_year = tmpI - 1900;
          }

        tmpLine[len - 2] = '\0';

        len = strlen(tmpLine);
        }  /* END if (len >= 2) */

      /* adjust for TZ */

      TP->tm_isdst = -1;

      /* place current time into tm structure */

      if ((*TimeP = mktime(TP)) == -1)
        {
        MDB(5,fCONFIG) MLog("ERROR:    cannot determine epoch time for '%s', errno: %d (%s)\n",
          String,
          errno,
          strerror(errno));

        return(FAILURE);
        }
      }  /* END BLOCK (case mrmtPBS) */

      break;
    }  /* END switch (RMType) */

  return(SUCCESS);
  }  /* END MUGetRMTime() */





/**
 * Converts epoch time to a requested RM format.
 *
 * @param ETime (I)
 * @param RMType (I)
 * @param tmpLine (O) - Return string (minsize = MMAX_LINE)
 */

int MTimeToRMString(

  mulong           ETime,  /* I */
  enum MRMTypeEnum RMType, /* I */
  char            *tmpLine)/* O */ 

  {
  time_t tmpTime;

  struct tm  *tp;

  if (tmpLine == NULL)
    return(FAILURE);

  tmpLine[0] = '\0';

  switch (RMType)
    {
    case mrmtPBS:
    default:

      setlocale(LC_TIME,"en_US.iso88591");

      tmpTime = (time_t)ETime;

      tp = localtime(&tmpTime);

      /* release time (FORMAT: [[[[CC]YY]MM]DD]hhmm[.SS]) */

      snprintf(tmpLine,sizeof(tmpLine),"%04d%02d%02d%02d%02d",
        tp->tm_year + 1900,
        tp->tm_mon + 1,
        tp->tm_mday,
        tp->tm_hour,
        tp->tm_min);

      break;
    }  /* switch (RMType) */

  return(SUCCESS);
  }  /* END MTimeToRMString() */






/**
 * NOTE:  handles leap year until year 3000
 */

int MUGetNumDaysInMonth(

  long    iTime)    /* I  epoch time */

  {
  time_t tmpTime;

  struct tm *TP;

  tmpTime = (time_t)iTime;

  TP = localtime(&tmpTime);

  switch (TP->tm_mon)
    {  
    case 0:  return(31); break; /* January */
    case 1:  /* February */

      {
      int yindex;

      /* incorporate leap year */

      yindex = TP->tm_year + 1900;

      if (yindex % 400 == 0)
        return(29);

      if (yindex % 100 == 0)
        return(28); 

      if (yindex % 4 == 0)
        return(29);

      return(28);
      }  /* END BLOCK */

      break;

    case 2:  return(31); break; /* March */
    case 3:  return(30); break; /* April */
    case 4:  return(31); break; /* May */
    case 5:  return(30); break; /* June */
    case 6:  return(31); break; /* July */
    case 7:  return(31); break; /* August */
    case 8:  return(30); break; /* September */
    case 9:  return(31); break; /* October */
    case 10: return(30); break; /* November */
    case 11: return(31); break; /* December */
    default: break;  /* FAILURE */
    }  /* END switch (TP->tm_mon) */

  /* FAILURE */

  return(0);
  }  /* END MUGetNumDaysInMonth() */





/**
 * Report days and months transpiring between specified start and end times
 */

int MUGetTimeRangeDuration(

  long  StartTime,  /* I (epoch time) */
  long  EndTime,    /* I (epoch time) */
  int  *MonthCount, /* O */
  int  *DayCount)   /* O */

  {
  time_t tmpTime;

  int startyear;
  int startmonth;
  int startday;

  int endyear;
  int endmonth;
  int endday;

  struct tm *tp;

  if (MonthCount != NULL)
    *MonthCount = 0;

  if (DayCount != NULL)
    *DayCount = 0;

  if ((StartTime <= 0) || (EndTime <= 0) || (EndTime < StartTime))
    {
    return(FAILURE);
    }

  tmpTime = (time_t)StartTime;

  tp = localtime(&tmpTime);

  if (tp == NULL)
    {
    return(FAILURE);
    }

  startyear  = tp->tm_year + 1900;
  startmonth = tp->tm_mon + 1;
  startday   = tp->tm_mday;

  tmpTime = (time_t)EndTime;

  tp = localtime(&tmpTime);

  if (tp == NULL)
    {
    return(FAILURE);
    }

  endyear  = tp->tm_year + 1900;
  endmonth = tp->tm_mon + 1;
  endday   = tp->tm_mday;

  if (DayCount != NULL)
    {
    if (endday >= startday)
      *DayCount = endday - startday;
    else
      *DayCount = MUGetNumDaysInMonth(StartTime) - startday + endday;
    }

  if (MonthCount != NULL)
    {
    *MonthCount = MCONST_MONTHSPERYEAR * (endyear - startyear) + endmonth - startmonth;
 
    if (endday < startday)
      *MonthCount -= 1;
    }

  return(SUCCESS);
  }  /* END MUGetTimeRangeDuration() */





/**
 * Report day of month and month of year for specified epoch time.
 *
 * @param Time (I)
 * @param MonthIndex (O) [optional]
 * @param DayIndex (O) [optional]
 */

int MUTimeGetDay(

  long  Time,       /* I (epoch time) */
  int  *MonthIndex, /* O (optional) */
  int  *DayIndex)   /* O (optional) */

  {
  time_t tmpTime;

  struct tm *tp;

  if (MonthIndex != NULL)
    *MonthIndex = 0;

  if (DayIndex != NULL)
    *DayIndex = 0;

  if (Time <= 0)
    {
    return(FAILURE);
    }

  tmpTime = (time_t)Time;

  tp = localtime(&tmpTime);

  if (tp == NULL)
    {
    return(FAILURE);
    }

  if (MonthIndex != NULL)
    *MonthIndex = tp->tm_mon + 1;

  if (DayIndex != NULL)
    *DayIndex = tp->tm_mday;

  return(SUCCESS);
  }  /* END MUTimeGetDay() */


/**
 * Convert relative time in seconds to [[DD]:HH]:MM]:SS] format. Returns a variable length string.
 *
 * @param STime (I) relative time in seconds
 * @param tmpPtr (O) - pointer to output string
 */

int MULToVTString(

  long STime,  /* I */
  char **tmpPtr)

  {
  char TString[MMAX_LINE];
  char *ptr;

  /* NOTE:  returns variable length string */

  if (tmpPtr == NULL)
    return(FAILURE);

  MULToTString(STime,TString);

  if (TString[0] == '\0')
    {
    MUStrDup(tmpPtr,"\0");

    return(SUCCESS);
    }

  ptr = TString;

  while (isspace(*ptr) && (*ptr != '\0'))
    ptr++;

  MUStrDup(tmpPtr,ptr);

  return(SUCCESS);
  }  /* END MULToVTString() */





/**
 * Convert relative time in seconds to [[DD]:HH]:MM]:SS] format. Returns a fixed length (11 char)
 * string.
 *
 * @param STime (I) relative time in seconds
 * @param String (O) output string (minlength = 12)
 */

int MULToTString(

  long STime,    /* I  relative time in seconds */
  char *String)

  {
  long        Time;
  int         Negative = FALSE;

  int         index;

  if (String == NULL)
    return(FAILURE);

  String[0] = '\0';

  /* NOTE:  returns fixed length string (11 char) */

  /* FORMAT:  [DD:]HH:MM:SS */

  if (STime >= 86400000) 
    {
    strcpy(String,"  INFINITY");

    return(SUCCESS);
    }
  else if (STime >= 8640000)
    {
    sprintf(String,"  %4ddays",
      (int)(STime / MCONST_DAYLEN));

    return(SUCCESS);
    }

  if (STime <= -86400000)
    {
    strcpy(String," -INFINITY");

    return(SUCCESS);
    }
  else if (STime <= -864000)
    {
    sprintf(String,"  %4ddays",
      (int)(STime / MCONST_DAYLEN));

    return(SUCCESS);
    }

  /* determine if number is negative */

  if (STime < 0)
    {
    Negative = TRUE;

    Time = -STime;
    }
  else
    {
    Time = STime;
    }

  String[11] = '\0';

  /* seconds */

  String[10] = (Time)    % 10 + '0';    
  String[9]  = (Time/10) %  6 + '0';   
  String[8]  = ':';

  Time /= 60;

  /* minutes */

  String[7] = (Time)    % 10 + '0';
  String[6] = (Time/10) %  6 + '0'; 
  String[5] = ':';

  /* hours */

  Time /= 60;

  String[4] = (Time % 24) % 10 + '0';
  String[3] = (Time/10) ? (((Time % 24)/10) % 10 + '0') : ' ';

  if ((String[4] == '0') && (String[3] == ' '))
    String[3] = '0';

  /* days */

  Time /= 24;

  if (Time > 0)
    {
    String[2] = ':';
    String[1] = (Time) % 10 + '0';
    String[0] = (Time/10) ? ((Time/10) % 10 + '0') : ' ';
    }
  else
    {
    String[2] = ' ';
    String[1] = ' ';
    String[0] = ' ';
    }

  if (Negative == TRUE)
    {
    if (String[3] == ' ')
      {
      String[3] = '-';
      }
    else if (String[2] == ' ')
      {
      String[2] = '-';
      }
    else if (String[1] == ' ')
      {
      String[1] = '-';
      }
    else if (String[0] == ' ')
      {
      String[0] = '-';
      }
    else
      {
      String[1] = '9';
      String[0] = '-';
      }
    }    /* END if (Negative == TRUE) */

  for (index = 3;index >= 0;index--)
    {
    if (String[index] == ' ')
      {
      char tmpString[MMAX_LINE];

      MUStrCpy(tmpString,&String[index + 1],MMAX_LINE);

      MUStrCpy(String,tmpString,11);
      String[11] = '\0';

      return(SUCCESS);
      }
    }  /* END for (index) */

  return(SUCCESS);
  }  /* END MULToTString() */



/**
 * Changes a time to a string.  Gives it in seconds if not a special value
 *  (like INFINITY).  Threadsafe.  Should be used for times passed in XML
 *  and such.
 *
 * @param STime  [I] - The time to report
 * @param String [O] - Output string
 * @param Length [I] - Length of output string (must be at least 11)
 */

char *MULToTStringSeconds(

  long  STime,
  char *String,
  int   Length)

  {
  if ((String == NULL) || (Length < 11))
    return(NULL);

  String[0] = '\0';

  if (STime >= MMAX_TIME - 1)
    {
    strncpy(String,"INFINITY",Length);

    return(String);
    }
 else if (STime <= -(MMAX_TIME - 1))
    {
    strncpy(String,"-INFINITY",Length);

    return(String);
    }

  snprintf(String,Length,"%ld",
    STime);

  return(String);
  }  /* END MULToTStringSeconds() */




/**
 * Epoch time to string
 *
 * @param iTime  (I) - epoch time
 * @param String (O) - output string (minsize = 12)
 */

int MUBStringTime(

  long  iTime,  /* I:  epoch time */
  char *String)

  {
  long        Time;

  if (String == NULL)
    return(FAILURE);

  Time = iTime;

  String[9] = '\0';

  /* seconds */

  String[8] = (Time)    % 10 + '0';
  String[7] = (Time/10) %  6 + '0';
  String[6] = ':';

  Time /= 60;

  /* minutes */

  String[5] = (Time)    % 10 + '0';
  String[4] = (Time/10) %  6 + '0';
  String[3] = ':';

  /* hours */

  Time /= 60;

  String[2] = (Time)    % 10 + '0';

  String[1] = (Time/10) ? ((Time/10) % 10 + '0') : ' ';

  String[0] = (Time/100) ? ((Time/100) % 10 + '0') : ' ';

  return(SUCCESS);
  }  /* END MUBStringTime() */


/**
 * Display long as human readable absolute date string.
 *
 * FORMAT: "Wed Jun 30 21:49:08 1993\n" 
 *
 * NOTE:  response terminated with newline, use MUSNCTime() otherwise
 *
 * @param Time (I)
 * @param String (O) - output string (minsize = 21)
 */

int MULToDString(

  mulong *Time, /* I */
  char   *String)

  {
  time_t tmpTime;

  if (String == NULL)
    return(FAILURE);

  /* FORMAT: "Wed Jun 30 21:49:08 1993\n" */

  tmpTime = (time_t)*Time;

  strncpy(String,ctime(&tmpTime),19);

  String[19] = '\n';
  String[20] = '\0';

  return(SUCCESS);
  }  /* END MULToDString() */




/**
 * FORMAT:  HH:MM:SS_MO/DD 
 *
 * @see MULToDString() - peer
 *
 * @param Time (I)
 * @param Buf (O) [optional,minsize=MMAX_NAME]
 */

char *MULToATString(

  mulong  Time, /* I */
  char   *Buf)  /* O (optional,minsize=MMAX_NAME) */

  {
  struct tm *T;
  time_t     tmpTime;

  char      *ptr;

  static char String[MMAX_NAME];

  ptr = (Buf != NULL) ? Buf : String;

  ptr[0] = '\0';

  /* display absolute time */

  tmpTime = (time_t)Time;

  if ((T = localtime(&tmpTime)) == NULL)
    {
    return(ptr);
    }

  /* FORMAT:  HH:MM:SS_MO/DD */

  sprintf(ptr,"%02d:%02d:%02d_%02d/%02d",
    T->tm_hour,
    T->tm_min,
    T->tm_sec,
    T->tm_mon + 1,
    T->tm_mday);

  return(ptr);
  }  /* END MULToATString() */




/**
 * Display long as human readable absolute date string.
 *
 * NOTE:  the magic number 19 below cuts the time string off after the time;
 *        no room left for the year (ATE)
 *
 * FORMAT: "Wed Jun 30 21:49:08 1993" 
 *
 * @see MULToDString() - adds newline to string.
 *
 * @param Time   (I)
 * @param String (O) - Output string (minsize = 20)
 */

int MUSNCTime(

  mulong *Time,  /* I */
  char   *String)

  {
  time_t tmpTime;

  if (String == NULL)
    return(FAILURE);

  tmpTime = (time_t)*Time;
  
  strncpy(String,ctime(&tmpTime),19);

  String[19] = '\0';

  return(SUCCESS);
  }  /* END MUSNCTime() */


/* NOTE:  leap year addressed in MUGetNumDaysInMonth() */

const int MMonthLen[] = {
  31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };






/**
 *
 *
 * @param BaseTime (I)
 * @param PeriodOffset (I) [not used]
 * @param DIndex (I)
 * @param PeriodType (I)
 * @param PeriodStart (O) [optional]
 * @param PeriodEnd (O) [optional]
 * @param PIndex (O) [optional]
 */

int MUGetPeriodRange(

  long  BaseTime,      /* I */
  long  PeriodOffset,  /* I (not used) */
  int   DIndex,        /* I */
  enum MCalPeriodEnum PeriodType, /* I */
  long *PeriodStart,   /* O (optional) */
  long *PeriodEnd,     /* O (optional) */
  int  *PIndex)        /* O period index (optional) */

  {
  time_t     tmpTime;
  struct tm *Time;

  long       tmpPeriodStart;
  long       tmpPeriodEnd;

  long       Offset;
  long       Duration;

  /* returns start time of period[DIndex] (including expired) */

  /* NOTE:  must incorporate daylight savings time            */
  /*        return PeriodStart DIndex periods in the future   */

  if (PeriodStart != NULL)
    *PeriodStart = 0;

  if (PeriodEnd != NULL)
    *PeriodEnd = 0;

  if (PIndex != NULL)
    *PIndex = 0;

  tmpTime = (time_t)BaseTime;

  Time = localtime(&tmpTime);

  if ((PeriodType != mpHour) && (PeriodType != mpMinute))
    {
#if defined(__LINUX) && !defined(__CYGWIN)
    Offset = (BaseTime + Time->tm_gmtoff) % MCONST_DAYLEN;
#else /* __LINUX && !__CYGWIN */
    Offset = Time->tm_hour * MCONST_HOURLEN +
             Time->tm_min  * MCONST_MINUTELEN +
             Time->tm_sec;
#endif /* __LINUX && !__CYGWIN */
    }
  else
    {
    Offset = Time->tm_min * MCONST_MINUTELEN + 
             Time->tm_sec;
    }

  switch (PeriodType)
    {
    case mpMinute:

      Duration = MCONST_MINUTELEN;

      if (PIndex != NULL)
        *PIndex = Time->tm_min;

      break;

    case mpHour:

      Duration = MCONST_HOURLEN;

      if (PIndex != NULL)
        *PIndex = Time->tm_hour;

      break;

    case mpDay:

      /* no adjustment needed */

      Duration = MCONST_DAYLEN;
    
      if (PIndex != NULL)
        *PIndex = Time->tm_wday;
 
      break;

    case mpWeek:

      /* wday ranges from 0 - 6 */

      Offset += Time->tm_wday * MCONST_DAYLEN;

      Duration = MCONST_WEEKLEN;

      if (PIndex != NULL)
        *PIndex = Time->tm_wday;

      break;

    case mpMonth:

      /* mday ranges from 1 to [28 - 31] */

      Offset += (Time->tm_mday - 1) * MCONST_DAYLEN;

      if (PIndex != NULL)
        *PIndex = Time->tm_mday - 1;

      Duration = MMonthLen[Time->tm_mon] * MCONST_DAYLEN;

      if (Time->tm_mon == 1)
        {
        /* Month is feb, support leap year */

        if ((Time->tm_year % 4) == 0)
          Duration += MCONST_DAYLEN;
        }

      break;

    case mpInfinity:
    default:

      Offset = BaseTime;

      Duration = MMAX_TIME;

      break;
    }  /* END switch (PeriodType) */
  
  tmpPeriodStart = BaseTime - Offset;

  switch (PeriodType)
    {
    case mpMinute:

      tmpPeriodStart += (DIndex * MCONST_MINUTELEN);

      break;

    case mpHour:

      tmpPeriodStart += (DIndex * MCONST_HOURLEN);

      break;

    case mpDay:

      tmpPeriodStart += (DIndex * MCONST_DAYLEN);

      break;

    case mpWeek:

      tmpPeriodStart += (DIndex * MCONST_WEEKLEN);

      break;

    case mpMonth:

      /* NOTE:  month must be calculated by walking through table of month lengths */

      /* NYI */

      tmpPeriodStart += (DIndex * MCONST_DAYLEN * 30);

      break;

    case mpInfinity:
    default:

      /* no adjustment needed */

      break;
    }  /* END switch (PeriodType) */

  tmpTime = (time_t)tmpPeriodStart;

  Time = localtime(&tmpTime);

  /* handle daylight savings */

  if ((PeriodType != mpHour) && (PeriodType != mpMinute))
    {
    /* force offset deeper into range */

    if (Time->tm_hour == 23)
      {
      tmpPeriodStart += MCONST_HOURLEN;
      }
    else if (Time->tm_hour == 1)
      {
      tmpPeriodStart -= MCONST_HOURLEN;
      }
    }

  tmpPeriodEnd = tmpPeriodStart + Duration;

  tmpPeriodEnd = MIN(MMAX_TIME,tmpPeriodEnd);

  if (PeriodStart != NULL)
    *PeriodStart = MAX(0,tmpPeriodStart);

  if (PeriodEnd != NULL)
    *PeriodEnd = MAX(0,tmpPeriodEnd);

  return(SUCCESS);
  }  /* END MUGetPeriodRange() */




/**
 *
 *
 * @param Duration (I)
 * @param Time (I)
 */

long MUGetStarttime(

  int  Duration,    /* I */
  long Time)        /* I */

  {
  long MidNight;

  long tmpTime;

  MUGetPeriodRange(Time,0,0,mpDay,&MidNight,NULL,NULL);

  tmpTime = MidNight;

  while (tmpTime <= Time)
    { 
    tmpTime += Duration;
    }

  tmpTime -= Duration;

  return(tmpTime);
  }  /* END MUGetStartTime() */



/**
 * Returns the epoch time in milliseconds OR returns the number of milliseconds
 * associated with given timeval.
 *
 * NOTE: This function is thread-safe.
 *
 * @param TV (I) calculate milliseconds for this value [optional]
 * @param MS (O) populates with epoch time in milliseconds
 */

int MUGetMS(

  struct timeval *TV,  /* I (optional) report milliseconds for this value */
  long           *MS)  /* O */

  {
  struct timeval  tvp;

  /* report milliseconds (NOTE:  boundary condition may exist at 1M second offsets) */

  if (MS == NULL)
    {
    return(FAILURE);
    }

  /* NOTE:  if gettimeofday() is too slow, consider using jiffies */

  if (TV == NULL)
    {
    gettimeofday(&tvp,NULL);

    /* determine millisecond offset in current time interval */

    *MS = (tvp.tv_sec % MDEF_USPERSECOND) * 1000 + (tvp.tv_usec / 1000);
    }
  else
    {
    *MS = (TV->tv_sec % MDEF_USPERSECOND) * 1000 + (TV->tv_usec / 1000);
    }

  return(SUCCESS);
  }  /* END MUGetMS() */



/**
 * Perform micro-second sleep/delay.
 *
 * NOTE: This function is not thread-safe if AdjustSchedClock is TRUE!
 *
 * @param SleepDuration (I) How long to sleep in microseconds.
 * @param AdjustSchedClock (I) Whether or not MSched.Time should be incremented the
 * amount of time that was slept. FUNCTION IS NOT THREAD-SAFE IF THIS IS TRUE!
 */

int MUSleep(

  long    SleepDuration,      /* I (in us) */
  mbool_t AdjustSchedClock)   /* I */

  {
  struct timeval timeout;              

  long tmpS;
  long tmpUS;

  tmpS  = SleepDuration / MDEF_USPERSECOND;
  tmpUS = SleepDuration % MDEF_USPERSECOND;

  timeout.tv_sec  = tmpS;
  timeout.tv_usec = tmpUS;

  if (MSched.TimeRatio > 0.0)
    {
    timeout.tv_sec  = (int)(timeout.tv_sec / MSched.TimeRatio);
    timeout.tv_usec = (int)(timeout.tv_usec / MSched.TimeRatio);
    }

  /* sleep for real time */

  select(0,(fd_set *)NULL,(fd_set *)NULL,(fd_set *)NULL,&timeout);

  if (AdjustSchedClock == TRUE)
    { 
    /* adjust scheduler clock */

    /* Just do a call to the system clock.  If we do it ourselves, there is
        a chance that MSched.Time > System Clock, and if a restart is done
        then, the checkpoint file will be "in the future", and so
        the calculation for checkpoint expiration will say that the checkpoint
        file is old (MSched.Time < checkpoint time, both unsigned longs, so even
        -1 would be a large number and therefore greater than the expiration
        time).  Checkpoint info would be discarded. */

    MUGetTime(&MSched.Time,mtmRefresh,&MSched);
    }    /* END if (AdjustSchedClock == TRUE) */

  return(SUCCESS);
  }  /* END MUSleep() */




/**
 *
 *
 * @param Now
 * @param ETime (I/O)
 * @param Offset (I/O)
 */

int MUCheckExpBackoff(

  mulong  Now,
  mulong *ETime,  /* I/O */
  mulong *Offset) /* I/O */

  {
  if (*ETime == 0)
    {
    *Offset = 120;

    *ETime = Now;
    
    return(SUCCESS);
    }

  if (Now < *ETime + *Offset)
    {
    /* too early - ignore event */

    return(FAILURE);
    }

  if (Now <= *ETime + *Offset + 60)
    {
    /* events continue at a rapid rate */

    if (*Offset < MCONST_DAYLEN)
      (*Offset) <<= 1;
    }
  else if (Now - *ETime > MCONST_HOURLEN)
    {
    /* no event in an hour - reset offset */

    *Offset = 120;
    }
  else
    {
    /* scale down offset */

    (*Offset) >>= 1;
    }
  
  *ETime = Now;

  return(SUCCESS);  
  }  /* END MUCheckExpBackoff() */



/* END MTime.c */
