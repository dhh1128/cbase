/* HEADER */


#include "moab.h"
#include "moab-proto.h"
#include "moab-global.h"


/**
 * MLicense theory of operation
 *
 * DEFINES:
 *    __MLICENSE
 *
 *    __MOABLTIME
 *
 *    __MOABSLTIME
 *
 * For development the above defines are turned OFF which effectively turns off Licensing verification and check.
 * This allows developers to develop without having to periodicly get updated licenses, etc.
 *
 * __MLICENSE is the primary enable/disable flag. When defined, code in this file is 
 *            enabled and licensing is enforced
 *      
 * __MOABLTIME is a key=value define, where the value is a time stamp (epoch based) of when an evaluation
 *            license is to be expired. It is a drop dead time stamp for moab.
 *
 * __MOABSLTIME is also a key=value define, where the value is a time stamp (epoch based) of when an
 *            evaluation license is to be expired. BUT its code utilizes a different calculation
 *            of when that is. This needs further research to properly document.
 */


/**
 * @file MLicense.c
 *
 * Code for managing, check, enforcing moab licensing
 *
 */


/* Newest license format */

#define RANDOMSIZE (MMAX_NAME >> 2)
#define BUFFERZONE 1024

typedef struct mlicense_t {
  char       Random1[RANDOMSIZE];

  char       HostName[MMAX_NAME];

  uint32_t   ExpirationDate;
  uint32_t   Features;

  uint32_t   MaxProcs;
  uint32_t   MaxGRes;
  uint32_t   MaxVM;

  char       Random3[RANDOMSIZE + BUFFERZONE]; /* added BUFFERZONE bytes as a buffer for keys that are longer than we would expect */
  } mlicense_t;

mlicense_t MLicense;


/* Old license format */

typedef struct mlicense_old_t {
  char       HostName[MMAX_NAME];
  
  uint32_t   ExpirationDate;

  uint32_t   MaxProcs;
  uint32_t   MaxGRes;

  char       Buffer[MMAX_BUFFER];
  } mlicense_old_t;


/**
 * Report the license has expired
 * Possibly return with error message OR may simply exit the program
 *
 * @param   LicStr      (I/O)
 * @param   SoftFail    (I) If set, the function will return from the 
 *                       function and not exit the process and the 
 *                       expired message will be reported in LicStr.
 *                       Will return status of valid or expired license
 * @param   LicStrSize  (I)
 */
int  __MLicenseReportExpiredLicense(

  mbool_t  SoftFail,    
  char    *LicStr,
  int      LicStrSize)

{
  char  *BPtr;
  int    BSpace;

  char   ErrorBuf[MMAX_BUFFER];

  MUSNInit(&BPtr,&BSpace,ErrorBuf,sizeof(ErrorBuf));

  MDB(0,fSCHED) MLog("ALERT:    %s License has expired.\n",
        MSCHED_NAME);

  if (MSched.LicMsg == NULL)
    {
    MDB(0,fSCHED) MLog("   Please contact Adaptive Computing for assistance.\n");
    MDB(0,fSCHED) MLog("   (email: sales@adaptivecomputing.com/phone +1 801 717 3700)\n");
    }

  MDB(0,fSCHED) MLog("ALERT:    %s License has expired.\n",
        MSCHED_NAME);

  MUSNCat(&BPtr,&BSpace," License has expired.\n");

  if (MSched.LicMsg == NULL)
    {
    MUSNCat(&BPtr,&BSpace,"       Please contact Adaptive Computing for assistance.\n");
    MUSNCat(&BPtr,&BSpace,"       (email: sales@adaptivecomputing.com/phone: +1 801 717 3700)\n");
    }
  else
    {
    MUSNPrintF(&BPtr,&BSpace,"%s\n",MSched.LicMsg);
    }

  if (SoftFail == TRUE)
    {
    if (LicStr != NULL)
      MUStrCpy(LicStr,ErrorBuf,LicStrSize);

    return(FAILURE);
    }

  fprintf(stderr,"%s",ErrorBuf);

  exit(1);          /* NO RETURN */

  return(SUCCESS);  /* return SUCCESS, even though never executed */
} /* END __MLicenseReportExpiredLicense */


#ifdef __MOABLTIME

/**
 * IF __MOABLTIME defined:
 *
 * Check if evaluation period for moab has expired. If so report and exit
 *
 * @param SoftFail    (I) If set, the function will return from the 
 *                       function and not exit the process and the 
 *                       expired message will be reported in LicStr.
 *                       Will return status of valid or expired license
 * @param LicStr      (O) string to report license information back in 
 * @param LicStrSize  (I) size of LicStr string 
 */

int  __MOABLTIME_CheckEvaluationPeriodExpired(

  mbool_t  SoftFail,
  char    *LicStr,
  int      LicStrSize)
  
  {
  char  *BPtr;
  int    BSpace;

  char   ErrorBuf[MMAX_BUFFER];

  MUSNInit(&BPtr,&BSpace,ErrorBuf,sizeof(ErrorBuf));

  if (MSched.NoEvalLicense == FALSE)
    MSched.SLTime = __MOABLTIME;

  if (MSched.Time > MSched.SLTime)
    {
    MDB(0,fSCHED) MLog("ALERT:    %s evaluation period has expired\n",
      MSCHED_NAME);

    if (MSched.LicMsg == NULL)
      {
      MDB(0,fSCHED) MLog("   Please contact Adaptive Computing for assistance.\n");
      MDB(0,fSCHED) MLog("   (email: sales@adaptivecomputing.com/phone +1 801 717 3700)\n");
      }
    
    MUSNPrintF(&BPtr,&BSpace,"NOTE:  %s evaluation period has expired.\n",
      MSCHED_NAME);

    if (MSched.LicMsg == NULL)
      {
      MUSNPrintF(&BPtr,&BSpace,"       Please contact Adaptive Computing for assistance.\n");
      MUSNPrintF(&BPtr,&BSpace,"       (email: sales@adaptivecomputing.com/phone: +1 801 717 3700)\n");
      }
    else
      {
      MUSNPrintF(&BPtr,&BSpace,"%s\n",MSched.LicMsg);
      }

    if (SoftFail == TRUE)
      {
      if (LicStr != NULL)
        MUStrCpy(LicStr,ErrorBuf,LicStrSize);

      return(FAILURE);
      }

    fprintf(stderr,"%s",ErrorBuf);

    exit(1);
    }

  return(SUCCESS);
  }
#endif /* __MOABLTIME */


#ifdef __MOABSLTIME
/**
 * IF __MOABSLTIME defined:
 *
 * Check if evaluation period for moab has expired. If so report and exit
 *
 * @param SoftFail    (I) If set, the function will return from the 
 *                       function and not exit the process and the 
 *                       expired message will be reported in LicStr.
 *                       Will return status of valid or expired license
 * @param LicStr      (O) string to report license information back in 
 * @param LicStrSize  (I) size of LicStr string 
 */

int __MOABSLTIME_CheckEvaluationPeriodExpired(

  mbool_t  SoftFail,
  char    *LicStr,
  int      LicStrSize)

  {
    if ((MCP.CPInitTime != 0) && (MSched.Time - MCP.CPInitTime > __MOABSLTIME))
      {
      MDB(0,fSCHED) MLog("ALERT:    %s evaluation period has expired\n");

      if (MSched.LicMsg == NULL)
        {
        MDB(0,fSCHED) MLog("   Please contact Adaptive Computing for assistance.\n");
        MDB(0,fSCHED) MLog("   (email: sales@adaptivecomputing.com/phone +1 801 717 3700)\n");
        }

      MUSNPrintF(&BPtr,&BSpace,"NOTE:  %s evaluation period has expired.\n",
        MSCHED_NAME);

      if (MSched.LicMsg == NULL)
        {
        MUSNPrintF(&BPtr,&BSpace,"       Please contact Adaptive Computing for assistance.\n");
        MUSNPrintF(&BPtr,&BSpace,"       (email: sales@adaptivecomputing.com/phone +1 801 717 3700)\n");
        }
      else
        {
        MUSNPrintF(&BPtr,&BSpace,"%s\n",MSched.LicMsg);
        }

      if (SoftFail == TRUE)
        {
        if (LicStr != NULL)
          MUStrCpy(LicStr,ErrorBuf,LicStrSize);

        return(FAILURE);
        }
      
      fprintf(stderr,"%s",ErrorBuf);

      exit(1);
      }
    }
#endif /* __MOABSLTIME */



/**
 * Check license expiration time.
 *
 * Return:
 *    SUCCESS if license is still valid
 *    FAILURE if license has expired
 *
 * Don't return, just EXIT if:
 *    SoftFail is FALSE and license is expired
 *
 * @see MLicenseGetLicense() - parent
 *
 * @param ExpireTime  (I) [optional]
 * @param SoftFail    (I) If set, the function will return from the 
 *                       function and not exit the process and the 
 *                       expired message will be reported in LicStr.
 *                       Will return status of valid or expired license
 * @param LicStr      (O) string to report license information back in 
 * @param LicStrSize  (I) size of LicStr string 
 */

int MSysCheckLicenseTime(

  mulong   ExpireTime,
  mbool_t  SoftFail,
  char    *LicStr,
  int      LicStrSize)

  {
  char  *BPtr;
  int    BSpace;

  char   ErrorBuf[MMAX_BUFFER];

  MUSNInit(&BPtr,&BSpace,ErrorBuf,sizeof(ErrorBuf));

  /* If ExpireTime is present, and current time exceed that 'time',
   * we have an error condition, so report it and fail
   */
  if ((ExpireTime != 0) && (MSched.Time > ExpireTime))
    {
    __MLicenseReportExpiredLicense(SoftFail,LicStr,LicStrSize);
    return(FAILURE);
    }    /* END if (ExpireTime != 0) */

  /* If __MOABLTIME is defined, then check evaluation period expired */
#ifdef __MOABLTIME
  if (ExpireTime == 0)
    {
    if (FAILURE == __MOABLTIME_CheckEvaluationPeriodExpired(SoftFail,LicStr,LicStrSize))
      return(FAILURE);
    }    /* END if (ExpireTime == 0) */
#endif /* __MOABLTIME */

  /* If __MOABSLTIME is defined, then check evaluation period expired */
#ifdef __MOABSLTIME
  if (ExpireTime == 0)
    {
    if (FAILURE == __MOABSLTIME_CheckEvaluationPeriodExpired(SoftFail,LicStr,LicStrSize))
      return(FAILURE);
  }
#endif /* __MOABSLTIME */

  return(SUCCESS);
  }  /* END __MSysCheckLicenseTime() */


/**
 * Given the number of days until the license has expired, return TRUE or FALSE
 *  which indicate whether to print/log/email a notification that the license WIL
 *  expire in 'days_left' days:  0..N
 *
 * Business Logic here:  60 days, 45, days 30 to 0 days  RETURN TRUE else FALSE
 */
#define MDEF_LICENSEDAYSMAXDAYS 64
#define MDEF_LICENSEDAYSFLAGSIZE MBMSIZE(MDEF_LICENSEDAYSMAXDAYS)


mbool_t  __MLicenseExpiringNotificationTrigger(

  int days_left)

  {
  static mbitmap_t LicenseDaysFlags; /* Each bit indicates email notification has been sent for that day.*/
  mbool_t trigger = FALSE;

  /* 'Sales/Custumer support' Business Policy logic here: */
  if ((days_left >= 0) && (days_left <= 30))
    {
    trigger = TRUE;
    }
  else if ((days_left == 60) || (days_left == 45))
    {
    trigger = TRUE;
    }

  /* If trigger = TRUE and we've already sent a notification, return FALSE */

  if (trigger == TRUE)
    {
    /* If notifications already been sent, return FALSE */

    if (bmisset(&LicenseDaysFlags,days_left))
      return(FALSE);

    /* NOTE:  We really just want to set the days flag so we don't send emails more than once
       a day.  However, The license file get read every check, so we need to zero out the bitmap
       for prior days in the event things change. */

    bmclear(&LicenseDaysFlags);
    bmset(&LicenseDaysFlags,days_left);
    }

  return (trigger);
  }


/**
 *  Returns the number of days until the current license expires
 *
 *  If negative number of days is returned, then that is the negative number of days since the license has expired
 *  Positive number is thenumber of days until license expires
 */

int MLicenseGetDaysUntilLicenseExpires(void)

  {
#ifdef __MLICENSE

  double sec_difference;
  time_t die_time;

  /* get difference in number of days between now (MSched.Time) and 
   * the expiration data, which is either evaluation cutoff or license cutoff
   */
#ifdef __MOABLTIME
  /* Expiration evaluation time was compiled in on the binary build  BUT
   * if license time is greater than the EVAL time, then we use that
   */
  die_time = __MOABLTIME;
  if (MSched.ActionDate > die_time)
    die_time = MSched.ActionDate;
#else
  /* Expiration time was captured in MLicenseGetLicense() */
  die_time = MSched.ActionDate;
#endif /* __MOABLTIME */

  sec_difference = difftime(die_time,MSched.Time);

  return((int) (sec_difference / MCONST_DAYLEN));

#else
  return(144);    /* return positive number of days all the time if __MLICENSE is NOT set */
#endif /* __MLICENSE */
  }


/**
 * Perform a live license recheck
 *  If license has expired, log message, email customer adamins and do Initiate Shutdown call.
 *  If license is near to expiration, then check if the number of days to expiration
 *    should trigger an email event and if so, perform the email
 *
 * @see MLicenseGetLicense() - child - to re-read the license and set things up once more
 * @see MLicenseGetDaysUntilLicenseExpires() - child - determine numbe of days left on license
 */
void MLicenseRecheck(void)
  {
  char message[MMAX_BUFFER];
  char licStr[MMAX_BUFFER];
  int days_to_expired;
  char *BPtr;
  int BSpace;
  char *list;
  int rc;

  const char *FName = "MLicenseRecheck";

  MDB(3,fALL) MLog("%s\n", FName);

  MUSNInit(&BPtr,&BSpace,message,sizeof(message));    /* Init the message buffer */

  /* Here, perform a new MLicenseGetLicense() call with  SilentMode set to TRUE, 
   * so it won't exit on us if lic is expired. 
   *
   * Calling this now will do the following:
   *  1) Update the in memory license attributes
   *  2) including the expiration date, which we use below to determine if expiry date is approaching
   *  3) allows for dynamic license updates without shutting down moab
   */
  rc = MLicenseGetLicense(TRUE, licStr, sizeof(licStr));
  if (rc == FAILURE)
    {
    /* If getting and validating the current the license failed in any way, 
     * then we do not have a valid license and we need to issue an error message and shutdown
     */
    MUSNPrintF(&BPtr,&BSpace,"ALERT:  Moab license failure.\n");
    MUSNCat(&BPtr,&BSpace,"See log file or contact Adaptive Computing for assistance\n");
    MUSNCat(&BPtr,&BSpace,"   (http://www.adaptivecomputing.com/contact.shtml)\n");
    MUSNCat(&BPtr,&BSpace,licStr);

    /* Issue a log message of expired license */
    MDB(0,fALL) MLog("%s", message);

    MSysInitiateShutdown(SIGQUIT);
    return;
    }

  /* NOW perform policy checking on whether we need to 
   * 1) shutdown moab due to an expired license
   * 2) issue "licensing expiring" emails to the customer's admins
   */
  days_to_expired = MLicenseGetDaysUntilLicenseExpires();

  /* First check: has it already expired? If so, then we issue alerts and orderly shutdown the program */
  if (days_to_expired < 0)
    {
    /* Build an error message to be emailed and logged concerning the expired license */
    MUSNPrintF(&BPtr,&BSpace,"ALERT:  Moab license EXPIRED %d day(s) ago\n",-(days_to_expired));
    MUSNCat(&BPtr,&BSpace,"Please contact Adaptive Computing for assistance\n");
    MUSNCat(&BPtr,&BSpace,"   (http://www.adaptivecomputing.com/contact.shtml)\n");

    /* issue email to admin with a notice of expired license being detected */
    list = MSysGetAdminMailList(1);
    MSysSendMail( list, NULL, "Your moab license has expired, stopping moab", NULL, message);

    /* Issue a log message of expired license */
    MDB(0,fALL) MLog("%s", message);

    /* initiate a graceful shutdown of the moab iterator 
     * This just registers the shutdown request. The main iterator will check this
     * after we return, sometime in this or the next iteration. But we WILL shutdown.
     */
    MSysInitiateShutdown(SIGQUIT);
    }
  else    /* 0..n days to expiration day then */
    {
    /* not expired but 'today' could fall on a 'notification day' prior to the actual
     * expiration day. On such notification days we send an email notifying the admin 
     * that the expired license date is approaching
     */
    if (__MLicenseExpiringNotificationTrigger(days_to_expired) == TRUE)
      {
      /* Build an error message to be emailed and logged concerning the expiring license */
      MUSNPrintF(&BPtr,&BSpace,"ALERT:  Moab license is EXPIRING in %d day(s)\n",days_to_expired);
      MUSNCat(&BPtr,&BSpace,"Please contact Adaptive Computing for assistance\n");
      MUSNCat(&BPtr,&BSpace,"   (http://www.adaptivecomputing.com/contact.shtml)\n");

      /* issue email to admin of the approaching expired license day */
      list = MSysGetAdminMailList(1);
      MSysSendMail( list, NULL, "Your moab license is expiring shortly", NULL, message);

      /* Issue a log message of the approaching expired license day */
      MDB(0,fALL) MLog("%s", message);
      }
    }
  }



/**
 * Return license expiration date.
 *
 * @param ETimeP (O) [optional]
 */

int MLicenseGetExpirationDate(

  mulong *ETimeP) /* O (optional) */

  {
  if (ETimeP != NULL)
    *ETimeP = MMAX_TIME;

  if (MLicense.ExpirationDate != 0)
    {
    if (ETimeP != NULL)
      *ETimeP = MLicense.ExpirationDate;
    }

  return(SUCCESS);
  }  /* END MSysQueryLicenseExpiration() */



/**
 * Return information on the expiration date of the license
 * IF licensing is enabled a string is returned indicating
 * one of the following:
 *
 *   A) license will expire in so many days
 *   B) license has been expired for so many days
 *
 * @param     Buffer     (O)  to place expiration date information
 * @param     BufferSize (I)  size of buffer
 */

int MLicenseGetExpirationDateInfo(
    
  char *Buffer,
  int   BufferSize)

{
#ifdef __MLICENSE
  mulong  tmpL;
#endif /* __MLICENSE */

  /* Create an empty buffer as a default */
  Buffer[0] = '\0';

#ifdef __MLICENSE
  /* Get when the license expires or expired, then
   * format a return string based on time left or time since it expired
   */
  if (MLicenseGetExpirationDate(&tmpL) == SUCCESS)
    {
    char TString[MMAX_LINE];

    if (MSched.Time > tmpL)
      {
      MULToTString(MSched.Time - tmpL,TString);

      snprintf(Buffer,BufferSize,"NOTE:  Moab license has been expired for %s",
        TString);
      }
    else if (MSched.Time + (MCONST_DAYLEN * 5) > tmpL)
      {
      MULToTString(tmpL - MSched.Time,TString);

      snprintf(Buffer,BufferSize,"NOTE:  Moab license will expire in %s",
        TString);
      }
    }
#endif /* __MLICENSE */

  return(SUCCESS);
}



/**
 * Find where the license file is (moab.lic) and return a path to that file:
 * Check in the following paths, in the following order, looking for 'moab.lic':
 *
 * 1) $MOABHOMEDIR/etc/moab.lic
 * 2) $MOABHOMEDIR/moab.lic
 * 3) /etc/moab.lic
 *
 * We CANNOT use MSched.CfgDir as the basis for our base pathname because
 * if moab.cfg is stored in $MOABHOMEDIR/etc and our license is in
 * $MOABHOME itself, MSched.CfgDir (get this) modified and thus brake
 * a dependency between these files. So we build our OWN file pathname
 * generator.
 *
 * If found, set the path buffer and return SUCCESS
 * If not found, return FAILURE
 */

int MGetLicenseFilePath(
    
  char *LicPath,
  int   LicPathSize)

  {
  const char *homedir;
  char *BPtr = NULL;
  int   BSpace = 0;
  char *sep;

  homedir = MUGetHomeDir();

  /* Determine trailing separator by peeking if homedir already has it or not */
  sep = (char *)((homedir[strlen(homedir) - 1] != '/') ? "/" : "");

  LicPath[0] = '\0';

  /* First try the HOMEDIR/etc/moab.lic. So get the homedir directory pathname */
  MUSNInit(&BPtr,&BSpace,LicPath,LicPathSize);
  MUSNCat(&BPtr,&BSpace,homedir);
  MUSNCat(&BPtr,&BSpace,sep);
  MUSNCat(&BPtr,&BSpace,"etc/");
  MUSNCat(&BPtr,&BSpace,MCONST_LICFILE);

  /* test for the file */
  if (MFUGetInfo(LicPath,NULL,NULL,NULL,NULL) == SUCCESS)
    {
    /* found the file */
    return(SUCCESS);
    }

  /* Second try the HOMEDIR/moab.lic. Start with the homedir directory pathname */
  MUSNInit(&BPtr,&BSpace,LicPath,LicPathSize);
  MUSNCat(&BPtr,&BSpace,homedir);
  MUSNCat(&BPtr,&BSpace,sep);
  MUSNCat(&BPtr,&BSpace,MCONST_LICFILE);

  /* test for the file */
  if (MFUGetInfo(LicPath,NULL,NULL,NULL,NULL) == SUCCESS)
    {
    /* found the file */
    return(SUCCESS);
    }

  /* Third try the ./moab.lic. */
  MUSNInit(&BPtr,&BSpace,LicPath,LicPathSize);
  MUSNCat(&BPtr,&BSpace,"./");
  MUSNCat(&BPtr,&BSpace,MCONST_LICFILE);

  /* test for the file */
  if (MFUGetInfo(LicPath,NULL,NULL,NULL,NULL) == SUCCESS)
    {
    /* found the file */
    return(SUCCESS);
    }

  LicPath[0] = '\0';
  return(FAILURE);
  } /* END MGetLicenseFilePath() */



/**
 * NOTE:  This routine is deprecated - it will only be called if a new style 
 *        license cannot be located.
 *
 * @see MLicenseGetLicense() - parent 
 *
 * @param SilentMode (I)
 * @param MLicense (I)
 * @param License (I)
 */

int __MLicenseGetOldLicense(

  mbool_t          SilentMode, /* I */
  mlicense_old_t  *MLicense,   /* I */
  char            *License)    /* I */

  {
#ifdef __MLICENSE

  char  *NewBuf;

  char   tmpHostExp[MMAX_BUFFER];
  char   NodeName[MMAX_LINE];

  if (MSecDecompress(
        (unsigned char *)License,
        strlen(License),
        NULL,
        0,
        (unsigned char **)&NewBuf,
        MCONST_LKEY) == FAILURE)
    {
    return(FAILURE);  
    }

  MSecComp64BitToBufDecoding((char *)NewBuf,strlen(NewBuf),(char *)MLicense,NULL);

  strcpy(NodeName,MSched.ServerHost);

  snprintf(tmpHostExp,sizeof(tmpHostExp),"%s",
    MLicense->HostName);
 
  if (MUREToList(
       tmpHostExp,
       mxoNode,
       NULL,
       NULL,
       FALSE,
       NodeName,
       sizeof(NodeName)) == FAILURE)
    {
    return(FAILURE);
    }
#endif /* __MLICENSE */

  return(SUCCESS);
  }  /* END __MLicenseGetOldLicense() */



/**
 * Read License from file and verify if it is valid
 *
 * This routine has both compile time and run time flags.
 *
 * Compile time flags:
 *
 *    _MLICENSE is set for licensed product and for evaulation binaries
 *    _MLICENSE is usually not set for development purposes
 *
 *    _MOABLTIME is set for evaulation expiring binaries. When license has expired, binary no longer runs
 *      The expired time is set at compile time and compared at run time.
 *
 * For licensed product (_MLICENSE defined):
 *
 *    Run time argument flag 'SilentMode':
 *
 *        If set to FALSE the function does the following:
 *          1)  policy enforcement is performed. This means that if the license has expired, corrupt
 *              or missing, this function terminates the running instance of moab RIGHT NOW - DOES NOT RETURN
 *          2)  If valid license, will print to stdout the license information
 *
 *        If set to TRUE the function does the following:
 *          1)  Still check for expired license, but will NOT exit, but rather generate an message
 *              of the problem in 'LicStr' and return FAILURE
 *          2)  Will not print message to stdout, but return message to caller in 'LicStr'
 *          3)  Return SUCCESS or FAILURE state of license
 *
 * Does NOT Return IF:
 *      SilentMode is FALSE and bad/missing/expired license 
 *
 * Returns:
 *
 *      SUCCESS if license detected and is valid for current environment. LicStr has license information.
 *      FAILURE if license is bad in some way, then LicStr contains an error message.
 *
 * @see MSysProcessArgs() - parent
 * @see MSysCheckLicenseTime() - child
 *
 * @param SilentMode (I)
 * @param LicStr (O) string to report license information back in or error message
 * @param LicStrSize (I) size of LicStr string 
 */

int MLicenseGetLicense(

  mbool_t  SilentMode, /* I */
  char    *LicStr,     /* O */
  int      LicStrSize) /* I */

  {
  int rc = SUCCESS;
#ifdef __MLICENSE
  FILE  *InFile;

  char   LicPath[MMAX_LINE];
  char   License[MMAX_BUFFER];

  char  *ptr;
  char  *TokPtr;

  char  *BPtr;
  int    BSpace;

  char  *NewBuf = NULL;

  char   tmpHostExp[MMAX_BUFFER];
  char   tmpBuf[MMAX_BUFFER];
  char   NodeName[MMAX_LINE];

  char   ErrorBuf[MMAX_BUFFER];
  char   ExpireBuf[MMAX_BUFFER];

  mbool_t Failure = FALSE;
  mbool_t LicenseFound = TRUE;

  mbool_t UseOld = FALSE;

  mlicense_old_t MOldLicense;

  MUSNInit(&BPtr,&BSpace,ErrorBuf,sizeof(ErrorBuf));

  tmpBuf[0] = '\0';
  ExpireBuf[0] = '\0';

  /* Get the full path to the moab license file */
  if ((MGetLicenseFilePath(LicPath,sizeof(LicPath)) == SUCCESS))
    {
    /* IF file was found, open it */
    InFile = fopen(LicPath,"r");
    }
  else
    {
    /* set error state of file NOT found */
    InFile = NULL;
    errno = ENOENT;

    /* Fill in a filename, since this file can be found in multiple places */
    MUStrCpy(LicPath,MCONST_LICFILE,sizeof(LicPath));
    }

  if (InFile == NULL)
    {
    switch (errno)
      {
      case EPERM:

        MUSNPrintF(&BPtr,&BSpace,"ERROR:  Moab not authorized to open file %s - please check file permissions\n",
          LicPath);

        break;

      case ENOENT:
      default:

        MUSNPrintF(&BPtr,&BSpace,"ERROR:  license file %s does not exist\n\n",
          LicPath);

        MUSNCat(&BPtr,&BSpace,"No License File Found\n");

        if (MSched.LicMsg == NULL)
          {
          MUSNCat(&BPtr,&BSpace,"Please contact Adaptive Computing for assistance\n");
          MUSNCat(&BPtr,&BSpace,"   (http://www.adaptivecomputing.com/contact.shtml)\n");
          }
        else
          {
          MUSNCat(&BPtr,&BSpace,MSched.LicMsg);
          }

        LicenseFound = FALSE;
 
        break;
      }  /* END switch (errno) */

    Failure = TRUE;
    }    /* END if (InFile == NULL) */

  /* IF file was found, now try to read it */
  if (Failure == FALSE)
    {
    errno = 0;
   
    if (fread((void *)tmpBuf,sizeof(tmpBuf),1,InFile) == 0)
      {
      if (feof(InFile) == 0)
        {
        MUSNPrintF(&BPtr,&BSpace,"ERROR:  license file %s is empty\n",
          LicPath);

        if (MSched.LicMsg == NULL)
          {
          MUSNCat(&BPtr,&BSpace,"Please contact Adaptive Computing for assistance\n");
          MUSNCat(&BPtr,&BSpace,"   (http://www.adaptivecomputing.com/contact.shtml)\n");
          }
        else
          {
          MUSNCat(&BPtr,&BSpace,MSched.LicMsg);
          }

        Failure = TRUE;
        }
      else if (tmpBuf[0] == '\0')
        {
        MUSNPrintF(&BPtr,&BSpace,"ERROR:  license file %s is empty\n",
          LicPath);

        if (MSched.LicMsg == NULL)
          {
          MUSNCat(&BPtr,&BSpace,"Please contact Adaptive Computing for assistance\n");
          MUSNCat(&BPtr,&BSpace,"   (http://www.adaptivecomputing.com/contact.shtml)\n");
          }
        else
          {
          MUSNCat(&BPtr,&BSpace,MSched.LicMsg);
          }

        Failure = TRUE;
        }
      }    /* END if (fread((void *)tmpBuf,sizeof(tmpBuf),1,InFile) == 0) */
    else if (tmpBuf[0] == '\0')
      {
      MUSNPrintF(&BPtr,&BSpace,"ERROR:  license file %s is empty\n",
        LicPath);

      if (MSched.LicMsg == NULL)
        {
        MUSNCat(&BPtr,&BSpace,"Please contact Adaptive Computing for assistance\n");
        MUSNCat(&BPtr,&BSpace,"   (http://www.adaptivecomputing.com/contact.shtml)\n");
        }
      else
        {
        MUSNCat(&BPtr,&BSpace,MSched.LicMsg);
        }

      Failure = TRUE;
      }
    }    /* END if (FAILURE == FALSE) */

  if (InFile != NULL)
    fclose(InFile); /* done using lic file */
    
  /* IF license was read into memory successful, test its contents */
  if (Failure == FALSE)
    {
    ptr = MUStrTok(tmpBuf,"\r\n",&TokPtr);
   
    while (ptr[0] == '#')
      {
      ptr = MUStrTok(NULL,"\r\n",&TokPtr);
   
      if (ptr == NULL)
        {
        MUSNPrintF(&BPtr,&BSpace,"ERROR:  no key in license file\n\n",
          LicPath);

        if (MSched.LicMsg == NULL)
          {    
          MUSNCat(&BPtr,&BSpace,"Please contact Adaptive Computing for assistance\n");
          MUSNCat(&BPtr,&BSpace,"   (http://www.adaptivecomputing.com/contact.shtml)\n");
          }
        else
          {
          MUSNCat(&BPtr,&BSpace,MSched.LicMsg);
          }

        Failure = TRUE;
   
        break;
        } 
      }
    }

  /* IF file looks good, then decompress the license */
  if (Failure == FALSE)
    {
    MUStrCpy(License,ptr,sizeof(License));
   
    if (MSecDecompress(
          (unsigned char *)ptr,
          strlen(ptr),
          NULL,
          0,
          (unsigned char **)&NewBuf,
          MCONST_LKEY) == FAILURE)
      {
      MUSNPrintF(&BPtr,&BSpace,"Invalid license file for current host - '%s'\n",
        MSched.ServerHost);
  
      if (MSched.LicMsg == NULL)
        { 
        MUSNCat(&BPtr,&BSpace,"Please contact Adaptive Computing for assistance\n");
        MUSNCat(&BPtr,&BSpace,"   (http://www.adaptivecomputing.com/contact.shtml)\n");
        }
      else
        {
        MUSNCat(&BPtr,&BSpace,MSched.LicMsg);
        }

      Failure = TRUE;
      }
    }  /* END if (Failure == FALSE) */


  /* check the key length for any issues */

  if (Failure == FALSE)
    {
    if (sizeof(MLicense) < strlen(NewBuf)) 
      {

      /* The length of the key is longer than our license struct (including a generous safety buffer margin).
         Something must be seriously wrong with this key and we cannot copy it to our license structure without
         corrupting memory. */

      MUSNCat(&BPtr,&BSpace,"Please contact Adaptive Computing for assistance\n");
      MUSNCat(&BPtr,&BSpace,"   (http://www.adaptivecomputing.com/contact.shtml)\n");

      Failure = TRUE;
      }
    else if ((sizeof(MLicense) - BUFFERZONE) < strlen(NewBuf)) 
      {

      /* for some reason the length of the key is longer than we would expect, but we can still safely copy
         it into our license structure without the risk of corrupting memory.
         Log the exception but let the key processing proceed. */

      MDB(8,fALL) MLog("INFO:      License Info mismatch %ld %d\n",
         sizeof(MLicense),
         strlen(NewBuf));
      }
    }

  /* IF decompression worked, then decrypt the license */
  if (Failure == FALSE)
    {
    mbool_t rc = TRUE;
    int rc2 = FAILURE;

    MSecComp64BitToBufDecoding((char *)NewBuf,strlen(NewBuf),(char *)&MLicense,NULL);
   
    /* Enforce we are running on host specified by license */
    strcpy(NodeName,MSched.ServerHost);
   
    snprintf(tmpHostExp,sizeof(tmpHostExp),"%s",
      MLicense.HostName);

    if (MUStringIsMacAddress(tmpHostExp) == TRUE)
      {
      char tmpLine[MMAX_LINE];

      MGetMacAddress(tmpLine);
      
      if (strncasecmp(tmpLine,tmpHostExp,strlen(tmpLine)))
        {
        Failure = TRUE;
        }
      else
        {
        Failure = FALSE;
        }
      }
    else if ((rc = MOSHostIsLocal(tmpHostExp,TRUE)) == TRUE)
      {
      /* NO-OP: but preserve return code for later check for failure */
      }
    else if ((rc2 = MUREToList(
         tmpHostExp,
         mxoNode,
         NULL,
         NULL,
         FALSE,
         NodeName,
         sizeof(NodeName))) == FAILURE)
      {
      if (__MLicenseGetOldLicense(SilentMode,&MOldLicense,License) == FAILURE)
        {
        MUSNPrintF(&BPtr,&BSpace,"Invalid license file for current host - '%s'\n",
          MSched.ServerHost);

        if (MSched.LicMsg == NULL)
          {   
          MUSNCat(&BPtr,&BSpace,"Please contact Adaptive Computing for assistance\n");
          MUSNCat(&BPtr,&BSpace,"   (http://www.adaptivecomputing.com/contact.shtml)\n");
          }
        else
          {
          MUSNCat(&BPtr,&BSpace,MSched.LicMsg);
          }

        Failure = TRUE;
        }
      else
        {
        UseOld = TRUE;
        }
      }
    else if ((rc2 == FAILURE) && (rc == FALSE))
      {
      /* Current host different than what Moab expects. MOSHostIsLocal() failed */

      MUSNPrintF(&BPtr,&BSpace,"Invalid host for current Moab license - '%s'\n",
        tmpHostExp);

      Failure = TRUE;
      }
    }    /* END if (Failure == FALSE) */

  /* LOOKING GOOD STATE: If no errors upto to this point, then extract license attributes */
  if (Failure == FALSE)
    {
    time_t ExpirationDate = 0; /* use to overcome problem with casting 32-bit int to 64-bit long */

#ifdef MBIGENDIAN
    if (UseOld == FALSE)
      {
      MLicense.ExpirationDate   = MUIntReverseEndian(MLicense.ExpirationDate);
      MLicense.MaxProcs         = MUIntReverseEndian(MLicense.MaxProcs);
      MLicense.Features         = MUIntReverseEndian(MLicense.Features);
      }
    else
      {
      MOldLicense.ExpirationDate   = MUIntReverseEndian(MOldLicense.ExpirationDate);
      MOldLicense.MaxProcs         = MUIntReverseEndian(MOldLicense.MaxProcs);
      }
#endif

    /* Now check if license has expired by performing the expiry check */
    rc = MSysCheckLicenseTime((UseOld == FALSE) ? MLicense.ExpirationDate: MOldLicense.ExpirationDate,
                                  SilentMode,ExpireBuf,sizeof(ExpireBuf));
   
    MUSNPrintF(&BPtr,&BSpace,"Moab Workload Manager Version '%s' License Information:\n",
      MOAB_VERSION);
  
    MUSNPrintF(&BPtr,&BSpace,"  Current License:  Max Procs   = %d\n",
      (UseOld == FALSE) ? MLicense.MaxProcs : MOldLicense.MaxProcs);
  
    ExpirationDate = ((UseOld == FALSE) ? (time_t)MLicense.ExpirationDate : (time_t)MOldLicense.ExpirationDate);

    MUSNPrintF(&BPtr,&BSpace,"  Current License:  Valid Until - %s",
      ctime(&ExpirationDate));

    if (ExpireBuf[0] != '\0')
      MUSNPrintF(&BPtr,&BSpace,"%s",ExpireBuf);
   
    /* Store importation license attributes into MSched object */
    MSched.ActionDate = ExpirationDate;

    MSched.MaxCPU = (UseOld == FALSE) ? MLicense.MaxProcs : MOldLicense.MaxProcs;
    MSched.MaxVM  = (UseOld == FALSE) ? MLicense.MaxVM : 0;
   
    MSched.LicensedFeatures = (UseOld == FALSE) ? MLicense.Features : 0;

    if ((MSched.MaxVM > 0) && (MOLDISSET(MSched.LicensedFeatures,mlfVM)))
      {
      MUSNPrintF(&BPtr,&BSpace,"  Current License:  Max VMs   = %d\n",
        MSched.MaxVM);
      }

    if (SilentMode == FALSE)
      {
      fprintf(stdout,"%s",ErrorBuf);
      }
    else if (LicStr != NULL)
      {
      MUStrCpy(LicStr,ErrorBuf,LicStrSize);
      }
    }  /* END if (Failure == FALSE) */
  else if (LicenseFound == FALSE)
    { 
    /* at this point NO LICENSE has been found */

    MUSNInit(&BPtr,&BSpace,ErrorBuf,sizeof(ErrorBuf));

    /* Check if license has expired (including evaluation license)
     * No return if SilentMode is FALSE and license has expired
     * If 1st argument is 0, and no __MOABLTIME, no action is taken by MSysCheckLicenseTime
     */
    rc = MSysCheckLicenseTime(0,SilentMode,ExpireBuf,sizeof(ExpireBuf));

    /* If caller just wants a report, generate the report here */
    if (SilentMode == TRUE)
      {
      /* Check if caller provided a buffer, if so then generate return message */
      if (LicStr != NULL)
        {
        MUSNCat(&BPtr,&BSpace,"No License File Found\n");

        /* If 'base' license message is empty, construct on here */
        if (MSched.LicMsg == NULL)
          {
          MUSNCat(&BPtr,&BSpace,"Please contact Adaptive Computing for assistance\n");
          MUSNCat(&BPtr,&BSpace,"   (http://www.adaptivecomputing.com/contact.shtml)\n");
          }
        else
          {
          /* Otherwise use the base license message */
          MUSNCat(&BPtr,&BSpace,MSched.LicMsg);
          }
      
        /* If the license time check failed, then pass back the error message */
        if (ExpireBuf[0] != '\0')
          {
          MUSNPrintF(&BPtr,&BSpace,"%s",ExpireBuf);
          }
#ifdef __MOABLTIME
        else
          {
          /* This is a configured EVAL build of moab and its license is still valid 
           * so report all is well and return a status message
           */
          char TString[MMAX_LINE];

          MULToTString(__MOABLTIME - MSched.Time,TString);
          rc = SUCCESS;       /* Ensure good status returned on eval license */
          MUSNPrintF(&BPtr,&BSpace,"Evaulation expires in %s\n",
            TString);
          MUSNPrintF(&BPtr,&BSpace,"   If you have received a license file from Adaptive Computing,\n");
          MUSNPrintF(&BPtr,&BSpace,"   please ensure this file is named moab.lic and copy it to\n");
          MUSNPrintF(&BPtr,&BSpace,"   $MOABHOMEDIR (e.g. /opt/moab/moab.lic)\n");
          }
#endif /* __MOABLTIME */

        /* Copy any message we constructed here back to caller */
        MUStrCpy(LicStr,ErrorBuf,sizeof(ErrorBuf));
        } /* END if LicStr != NULL)) */
        /* else: no caller buffer, so no report, no code */

      } /* END if (SilentMode == TRUE) */
#ifndef __MOABLTIME
    else
      {
      /* SilentMode == FALSE
       * Enforcement of license is ON, but no license file was found, so hard exit 
       */
      MUSNCat(&BPtr,&BSpace,"Moab will now exit due to license file not found\n");
   
      fprintf(stderr,"%s",ErrorBuf);

      exit(1);    /* NO RETURN */
      }
#endif /* __MOABLTIME */
    }
  else
    {  
    /* some other type of license error: not found or not viable in some way (Hard exit) */
    rc = FAILURE;

    MDB(0,fALL) MLog("ERROR:    %s\n",ErrorBuf);

    if (SilentMode == FALSE)
      {
      MUSNCat(&BPtr,&BSpace,"Moab will now exit due to invalid license file\n");
   
      fprintf(stderr,"%s",ErrorBuf);

      exit(1);    /* NO RETURN */
      }
    else if (LicStr != NULL)
      {
      MUStrCpy(LicStr,ErrorBuf,LicStrSize);
      }
    }
#else
    /* for non licensed binary, set the License features to work */
  MSched.LicensedFeatures = MBMALL;

  rc = MSysCheckLicenseTime(0,SilentMode,NULL,0);
#endif /* __MLICENSE */

  return(rc);
  }  /* END MLicenseGetLicense() */


/**
 * Perform a license CPU high mark check with the arrival of a Node indicated by 'N'
 *
 * @param        N                      (I)    Node
 * @param        CurrentTotalCPUCount   (I)    Current SUM of CPUs found already by caller
 */

int MLicenseCheckNodeCPULimit(

  mnode_t    *N,
  int         CurrentTotalCPUCount)

  {
  int rc = SUCCESS;

#ifdef __MLICENSE
  int procs = MNodeGetBaseValue(N,mrlProc);

  /* Only if __MLICENSE is defined do we enforce the limit */
  if (procs + CurrentTotalCPUCount > MSched.MaxCPU)
    {
    /* This new Node will cause too many cpu's to be on this system, 
     * therefore, NO MORE nodes will be allowed to be added, including this one
     */

    MMBAdd(&MSched.MB,"WARNING:  too many nodes detected for current licensed limit\n",NULL,mmbtNONE,0,0,NULL);

    MDB(-1,fRM) MLog("ERROR:    node '%s' violates current license count (%d > %d) and will be removed\n",
      N->Name,
      procs + CurrentTotalCPUCount,
      MSched.MaxCPU);

    rc = FAILURE;
    }
#endif /* __MLICENSE */

  return(rc);
  }  /* END  MLicenseCheckNodeCPULimit() */




/**
 * Perform a license VM check
 *
 * @param     EMsg (O) string to return error message
 */

int MLicenseCheckVMUsage(

  char *EMsg)

  {
  int rc = SUCCESS;

#ifdef __MLICENSE
  /* Only if __MLICENSE is defined do we enforce the limit */
  if ((MSched.MaxVM == 0) || !MOLDISSET(MSched.LicensedFeatures,mlfVM))
    {
    if (EMsg != NULL)
      {
      snprintf(EMsg,MMAX_LINE,"ERROR:   license does not allow for VM usage, please contact Adaptive Computing\n");
      }

    MMBAdd(&MSched.MB,"ERROR:   license does not allow for VM usage, please contact Adaptive Computing",NULL,mmbtNONE,0,0,NULL);

    rc = FAILURE;
    }
  else if ((MSched.MaxVM > 0) && MSched.VMTable.NumItems >= MSched.MaxVM)
    {
    if (EMsg != NULL)
      {
      snprintf(EMsg,MMAX_LINE,"ERROR:   maximum licensed VM count reached\n");
      }

    MMBAdd(&MSched.MB,"ERROR:   maximum licensed VM count reached",NULL,mmbtNONE,0,0,NULL);

    rc = FAILURE;
    }
#endif /* __MLICENSE */

  return(rc);
  }  /* END  MLicenseCheckVMUsage() */


/* END MLicense.c */
