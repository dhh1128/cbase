/* HEADER */

/**
 * @file MUsersGroups.c
 * 
 * Contains various functions for linux users and groups
 *
 */


#include "moab.h"
#include "moab-proto.h"
#include "moab-const.h"
#include "moab-global.h"


/**
 * Convert UID to user name.
 *
 * @param UID (I) The UID to convert to a username
 * @param NameBuf (I) The buffer in which to write the username [minsize=MMAX_NAME]
 *
 * @return Pointer to the NameBuf variable.
 */

char *MUUIDToName(

  int UID,       /* I */
  char *NameBuf) /* O */

  {
  struct passwd *bufptr = NULL;
  struct passwd  buf;
  char           pwbuf[MMAX_BUFFER];

#ifndef MNOREENTRANT
  int            rc = 0;
#endif /* !MNOREENTRANT */

  const char *FName = "MUUIDToName";

  MDB(10,fSTRUCT) MLog("%s(%d,NameBuf)\n",
    FName,
    UID);

  if (NameBuf == NULL)
    {
    return(NULL);
    }

  if (UID == -1)
    {
    strcpy(NameBuf,NONE);

    return(NameBuf);
    }

  pwbuf[0] = '\0';

  errno = 0;

  /* getpwuid_r() will return positive value on FAILURE */

#ifndef MNOREENTRANT
  rc = getpwuid_r(UID,&buf,pwbuf,sizeof(pwbuf),&bufptr);

  if ((rc != 0) || (bufptr == NULL))
    {
    snprintf(NameBuf,MMAX_NAME,"UID%d",
      UID);
    }
  else
    {
    MUStrCpy(NameBuf,bufptr->pw_name,MMAX_NAME);
    }
#else
  bufptr = getpwuid(UID);

  if (bufptr == NULL)
    {
    sprintf(NameBuf,"UID%d",
      UID);
    }
  else
    {
    strcpy(NameBuf,bufptr->pw_name);
    }
#endif /* !MNOREENTRANT */

  return(NameBuf);
  }  /* END MUUIDToName() */





/**
 * Returns the human-readable name of the given group ID.
 *
 * @param GID (I) The group ID to look up.
 * @param NameBuf (O) A buffer in which the group name is written. [minsize=MMAX_NAME]
 */

char *MUGIDToName(

  int GID,        /* I */
  char *NameBuf)  /* O */

  {
  struct group *bufptr = NULL;

#ifndef MNOREENTRANT
  struct group  buf;

  char          pwbuf[MMAX_BUFFER];
  int           rc;
#endif /* !MNOREENTRANT */

  const char *FName = "MUGIDToName";

  MDB(10,fSTRUCT) MLog("%s(%d,NameBuf)\n",
    FName,
    GID);

  if (NameBuf == NULL)
    {
    return(NULL);
    }

  if (GID == -1)
    {
    strcpy(NameBuf,NONE);

    return(NameBuf);
    }

#ifndef MNOREENTRANT

  /* IMPORTANT NOTE:  pwbuf must be big enough to store all information of the largest
                      single group record */
     
  rc = getgrgid_r(
    (gid_t)GID,     /* I */
    &buf,           /* O - store the retrieved group structure */
    pwbuf,          /* O - strings refrenced w/in buf */
    sizeof(pwbuf),
    &bufptr);       /* O - A pointer to the result (in case of success) or NULL */
      
  /* NOTE:  for failures, rc will be positive number 'errno' value (not found can return an rc of 0)*/
  /* NOTE: Posix man page - a NULL pointer shall be returned at the location pointed to by bufptr on error OR if the requested entry is not found */

  /* it appears that buf.gr_name can be NULL if the group has not been given
   * a name in the OS */

  if ((rc != 0) ||
      (bufptr == NULL) ||
      (buf.gr_name == NULL) ||
      (buf.gr_name[0] == '\0'))
    {
    /* FAILURE */

    sprintf(NameBuf,"GID%d",
      GID);
    }
  else
    {
    strcpy(NameBuf,buf.gr_name);
    }

#else /* !MNOREENTRANT */

  if (((bufptr = getgrgid((gid_t)GID)) == NULL) ||
      (bufptr->gr_name == NULL) ||
      (bufptr->gr_name[0] == '\0'))
    {
    sprintf(NameBuf,"GID%d",
      GID);
    }
  else
    {
    strcpy(NameBuf,bufptr->gr_name);
    }

#endif /* !MNOREENTRANT */

  return(NameBuf);
  }  /* END MUGIDToName() */





/**
 * Report primary group name for specified user.
 *
 * @see MUGIDFromUser()
 *
 * @param UName (I)
 * @param GName (O) [minsize=MMAX_NAME]
 */

int MUGNameFromUName(

  char *UName,  /* I */
  char *GName)  /* O (minsize=MMAX_NAME) */

  {
  int GID;

  char tmpLine[MMAX_LINE];

  if ((UName == NULL) || (GName == NULL))
    {
    return(FAILURE);
    }

  if ((GID = MUGIDFromUser(-1,UName,NULL)) < 0)
    {
    return(FAILURE);
    }

  MUGIDToName(GID,tmpLine);

  if (!strcmp(tmpLine,NONE) || !strncmp(tmpLine,"GID",3))
    {
    return(FAILURE);
    }

  strncpy(GName,tmpLine,MMAX_NAME);

  return(SUCCESS);
  }  /* END MUGNameFromUName() */





/**
 * Get GID from user name.
 *
 * @see MUNameGetGroups()
 *
 * @param UID (I) [optional,notset=-1]
 * @param UName (I) [optional]
 * @param GName (O) [optional minsize=MMAX_NAME]
 */

int MUGIDFromUser(

  int   UID,    /* I (optional,notset=-1) */
  char *UName,  /* I (optional) */
  char *GName)  /* O group name (optional minsize=MMAX_NAME) */

  {
  struct passwd *bufptr = NULL;
  struct passwd  buf;

  char           pwbuf[MMAX_BUFFER];

#ifndef MNOREENTRANT
  int            rc = 0;
#else
  struct passwd *rcPasswd;
#endif /* !MNOREENTRANT */

  const char *FName = "MUGIDFromUser";

  MDB(10,fSTRUCT) MLog("%s(%d,UName,GName)\n",
    FName,
    UID);

  if (GName != NULL)
    GName[0] = '\0';

  pwbuf[0] = '\0';

  if (MSched.OSCredLookup == mapNever)
    {
    return(-1);
    }

  if (UName != NULL)
    {
    /* getpwnam_r() will return positive value on FAILURE */

#ifndef MNOREENTRANT
    rc = getpwnam_r(UName,&buf,pwbuf,sizeof(pwbuf),&bufptr);

    if ((rc == 0) && (bufptr != NULL))
      {
      if (GName != NULL)
        {
        MUGIDToName(bufptr->pw_gid,GName); 
        }

      return(bufptr->pw_gid);
      }
#else
    bufptr = getpwnam(UName);

    if (bufptr != NULL) /*If there is no error*/
      {
      if (GName != NULL)
        {
        MUStrCpy(GName,MUGIDToName(bufptr->pw_gid,GName),MMAX_NAME);
        }

      return(bufptr->pw_gid);
      }
#endif /* !MNOREENTRANT */
    }
  else
    {
    /* getpwuid_r() will return positive value on FAILURE */

#ifndef MNOREENTRANT
    rc = getpwuid_r(UID,&buf,pwbuf,sizeof(pwbuf),&bufptr);

    if ((rc == 0) && (bufptr != NULL))
      {
      if (GName != NULL)
        {
        MUGIDToName(bufptr->pw_gid,GName);
        }

      return(bufptr->pw_gid);
      }
#else
    bufptr = getpwuid(UID);

    if (bufptr != NULL) /* If there is no error*/
      {
      if (GName != NULL)
        {
        MUStrCpy(GName,MUGIDToName(bufptr->pw_gid,GName),MMAX_NAME);
        }

      return(bufptr->pw_gid);
      }
#endif /* !MNOREENTRANT */
    }

  return(-1);
  }  /* END MUGIDFromUser() */




/**
 * Determine primary and secondary groups associated with specified user name.
 *
 * @see MUGIDFromUser()
 *
 * NOTE:  only one of GList or GString may be specified.
 *
 * @param UName (I)
 * @param GID (I) [optional,-1 = not set]
 * @param GList (O) - list of GIDs [optional,minsize=GListSize
 * @param GString (O) - comma delimited list of group names [optional]
 * @param NumEntries (O) - number of entries returned in the GList [optional]
 * @param GListSize (I)
 */

int MUNameGetGroups(

  char *UName,      /* I */
  int   GID,        /* I (optional,-1 = not set) */
  int  *GList,      /* O (optional,minsize=GListSize) - list of GIDs */
  char *GString,    /* O (optional,minsize=GLISTSIZE) - comma delimited list of group names */
  int  *NumEntries, /* O (optional) - number of entries returned in the GList */
  int   GListSize)  /* I (size of GList and/or GString!!!) */

  {
  int  gindex;
  int  tgcount;

  int  tList[MMAX_USERGROUPCOUNT];

  char GName[MMAX_NAME];

  int  PGID;

  if ((UName == NULL) || (UName[0] == '\0'))
    {
    return(FAILURE);
    }

  if ((GList != NULL) && (GString != NULL))
    {
    return(FAILURE);
    }

  PGID = MUGIDFromUser(-1,UName,GName);

#if defined(__LINUX) && !defined(__CYGWIN)
  
  tgcount = MMAX_USERGROUPCOUNT;

  if (getgrouplist(UName,PGID,(gid_t *)tList,&tgcount) == -1)
    {
    return(FAILURE);
    }

  if (NumEntries != NULL)
      *NumEntries = 0;

  if (GList != NULL)
    {
    memcpy(GList,tList,MIN(tgcount,GListSize) * sizeof(gid_t));

    if (NumEntries != NULL)
      *NumEntries = MIN(tgcount,GListSize);
    }

  if (GString != NULL)
    {
    char *BPtr = NULL;
    int   BSpace = 0;
    char  tmpGName[MMAX_NAME];

    MUSNInit(&BPtr,&BSpace,GString,GListSize);

    for (gindex = 0;gindex < tgcount;gindex++)
      {
      MUSNPrintF(&BPtr,&BSpace,"%s%s",
        (gindex > 0) ? "," : "",
        MUGIDToName(tList[gindex],tmpGName));
      }  /* END for (gindex) */
    }    /* END if (GString != NULL) */

#else /* __LINUX && !__CYGWIN */

  if (GString != NULL)
    {
    /* only returns secondary groups */

    MOSGetGListFromUName(UName,GString,NULL,GListSize,NULL);

    tList[0] = -1;
    }
  else
    { 
    MOSGetGListFromUName(UName,NULL,tList,0,&tgcount);

    if (GList != NULL)
      memcpy(GList,tList,MIN(tgcount,GListSize) * sizeof(gid_t));
    }

#endif /* __LINUX && !__CYGWIN */

  if (GID >= 0)
    {
    /* verify user has access to specified group */

    if (PGID == GID)
      {
      return(SUCCESS);
      }

    for (gindex = 0;gindex < tgcount;gindex++)
      {
      if ((int)tList[gindex] == GID)
        break;
      }

    if ((int)tList[gindex] != GID)
      {
      return(FAILURE);
      }
    }    /* END if (GID >= 0) */

  return(SUCCESS);
  }  /* END MUNameGetGroups() */





/**
 * Determine user id from user name.
 *
 * @param Name (I)
 * @param HomeDir (O) [optional,minsize=MMAX_LINE]
 * @param EMsg (O) [optional,minzise=MMAX_LINE]
 */

int MUUIDFromName(

  char *Name,    /* I */
  char *HomeDir, /* O (optional,minsize=MMAX_LINE) */
  char *EMsg)    /* O (optional,minzise=MMAX_LINE) */

  {
#define UTILUIDHEADER "UID"

  struct passwd *bufptr = NULL; 
  struct passwd  buf;
  char   pwbuf[MMAX_BUFFER];

#ifndef MNOREENTRANT
  int    rc = 0;
#endif /* !MNOREENTRANT */

  const char *FName = "MUUIDFromName";

  MDB(10,fSTRUCT) MLog("%s(%s,HomeDir,EMsg)\n",
    FName,
    (Name != NULL) ? Name : "NULL");

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (HomeDir != NULL)
    HomeDir[0] = '\0';

  if ((Name == NULL) ||
      !strcmp(Name,NONE) || 
      !strcmp(Name,ALL) || 
      !strcmp(Name,"ALL"))
    {
    if (EMsg != NULL)
      sprintf(EMsg,"user name '%s' is invalid",
        (Name != NULL) ? Name : "");

    return(-1);
    }

#ifndef MNOREENTRANT
  /* getpwnam_r() will return positive value on FAILURE */

  rc = getpwnam_r(Name,&buf,pwbuf,sizeof(pwbuf),&bufptr);

  if ((rc != 0) || (bufptr == NULL))
    {
    /* look for UID??? format */

    if (!strncmp(Name,UTILUIDHEADER,strlen(UTILUIDHEADER)))
      {
      return(atoi(Name + strlen(UTILUIDHEADER)));
      }

    if (EMsg != NULL)
      snprintf(EMsg,MMAX_LINE,"cannot lookup user '%s' - errno=%d %s",
        Name,
        errno,
        strerror(errno));

    MDB(3,fSTRUCT) MLog("cannot lookup user '%s' - errno=%d %s\n",
      Name,
      errno,
      strerror(errno));

    return(-1);
    }
#else
  bufptr = getpwnam(Name);

  if (bufptr == NULL)
    {
    /* look for UID??? format */

    if (!strncmp(Name,UTILUIDHEADER,strlen(UTILUIDHEADER)))
      {
      return(atoi(Name + strlen(UTILUIDHEADER)));
      }

    if (EMsg != NULL)
      snprintf(EMsg,MMAX_LINE,"cannot lookup user '%s' - errno=%d %s",
        Name,
        errno,
        strerror(errno));

    MDB(3,fSTRUCT) MLog("cannot lookup user '%s' - errno=%d %s\n",
      Name,
      errno,
      strerror(errno));

    return(-1);
    }
#endif /* !MNOREENTRANT */

  if (HomeDir != NULL)
    {
    MUStrCpy(HomeDir,bufptr->pw_dir,MMAX_LINE);

    MDB(7,fSTRUCT) MLog("setting user %s's home dir to '%s'\n",
      (Name != NULL) ? Name : "NULL",
      HomeDir);
    }

  return(bufptr->pw_uid);
  }  /* END MUUIDFromName() */





/**
 * Report GID associated with specified group name.
 *
 * @param GName (I)
 */

int MUGIDFromName(

  char *GName)

  {
  struct group *buf;
  int tmpGID = 0;

  const char *FName = "MUGIDFromName";

  MDB(7,fSTRUCT) MLog("%s(%s)\n",
    FName,
    (GName != NULL) ? GName : "NULL");

  if ((GName == NULL) || !strcmp(GName,NONE))
    {
    return(-1);
    }

  errno = 0;

  buf = getgrnam(GName);

  if (buf == NULL)
    {
    MDB(3,fSTRUCT) MLog("ALERT:    error looking up group '%s' - %s (errno:%d)\n",
      GName,
      strerror(errno),
      errno);
    
    /* look for GID??? format */

    if (!strncmp(GName,"GID",strlen("GID")))
      {
      return(atoi(GName + strlen("GID")));
      }

    /* check to see if GName is the GID number itself (group names cannot start
     * with a number) */

    tmpGID = (int)strtol(GName,NULL,10);

    if (tmpGID != 0)
      {
      return(tmpGID);
      }

    return(-1);
    }

  return(buf->gr_gid);
  }    /* END MUGIDFromName() */



