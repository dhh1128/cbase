/* HEADER */

      
/**
 * @file MAdmin.c
 *
 * Contains: Admin functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 * Loads ADMINCFG parameters from configuration.
 *
 * @see MAdminProcessConfig()
 *
 * @param Buf (I) [optional]
 */

int MAdminLoadConfig(

  char *Buf)  /* I (optional) */

  {
  char   IndexName[MMAX_NAME];

  char   Value[MMAX_LINE];

  char  *ptr;
  char  *head;

  int    AIndex;

  /* FORMAT:  <KEY>=<VAL>[<WS><KEY>=<VAL>]...         */
  /*          <VAL> -> <ATTR>=<VAL>[:<ATTR>=<VAL>]... */

  /* load all/specified AM config info */

  head = (Buf != NULL) ? Buf : MSched.ConfigBuffer;

  if (head == NULL)
    {
    return(FAILURE);
    }

  /* load all sched config info */

  ptr = head;

  IndexName[0] = '\0';

  while (MCfgGetSVal(
           head,
           &ptr,
           MParam[mcoAdminCfg],
           IndexName,
           NULL,
           Value,
           sizeof(Value),
           0,
           NULL) != FAILURE)
    {
    if (IndexName[0] != '\0')
      {
      /* set admin index */

      AIndex = (int)strtol(IndexName,NULL,10);

      if ((AIndex <= 0) || (AIndex > MMAX_ADMINTYPE))
        {
        /* invalid admin type specified */

        IndexName[0] = '\0';

        continue;
        }
      }
    else
      {
      AIndex = 1;
      }

    /* load sys specific attributes */

    MAdminProcessConfig(&MSched.Admin[AIndex],Value);

    IndexName[0] = '\0';
    }  /* END while (MCfgGetSVal() != FAILURE) */

  return(SUCCESS);
  }  /* END MAdminLoadConfig() */


/**
 * Process attributes of ADMINCFG[] USERS= parameter.
 *
 * @see MAdminProcessConfig()
 *
 * @param A (I) [modified]
 * @param Value (I)
 */

int MAdminProcessConfigUserList(

  madmin_t *A,     /* I (modified) */
  char     *ValLine) /* I */

  {
  int uindex, uindex2;

  char *UName;
  char *TokPtr2 = NULL;

  /* Move to the end of the users list. */
  uindex = 0;
  while(A->UName[uindex][0] != '\0')
    uindex++;

  if (uindex >= MMAX_ADMINUSERS)
    {
    MDB(0,fCONFIG) MLog("ALERT:    max admin users (%d) reached\n", MMAX_ADMINUSERS);

    return(FAILURE);
    }

  /* Parse each username in turn. */
  UName = MUStrTok(ValLine,", \t",&TokPtr2);
  while (UName != NULL)
    {
    mgcred_t *U;
    mbool_t InList;

    /* Make sure this user is not already in the Admin users list. */
    InList = FALSE;
    for(uindex2=0;
         (InList == FALSE) && (uindex2<MMAX_ADMINUSERS) && (A->UName[uindex2][0]!='\0');
         uindex2++ )
      {
      if (strcmp(UName, A->UName[uindex2])==0)
        {
        InList = TRUE;
        }
      }

    if (InList)
      {
      UName = MUStrTok(NULL,", \t",&TokPtr2);
      continue;
      }

    if (strncasecmp(UName,"PEER:",strlen("PEER:")))
      {
      /* update user record for non-peer admin specification */

      if (MUserAdd(UName,&U) == SUCCESS)
        {
        U->Role = (enum MRoleEnum)A->Index;
        MDB(3,fCONFIG) MLog("INFO:     admin %d user '%s' added\n",
          A->Index,
          UName);
        }
      }

    /* Add this user to the admin users list. */
    MUStrCpy(
      A->UName[uindex],
      UName,
      MMAX_NAME);

    uindex++;

    /* If the list is full, stop processing. */
    if (uindex == MMAX_ADMINUSERS)
      {
      MDB(0,fCONFIG) MLog("ALERT:    max admin users (%d) reached\n",
        MMAX_ADMINUSERS);

      break;
      }

    if (uindex >= MMAX_ADMINUSERS)
      break;

    UName = MUStrTok(NULL,", \t",&TokPtr2);
    }  /* END while (UName != NULL) */

  return(SUCCESS);
  } /* END MAdminProcessConfigUserList() */



/**
 * Process attributes of ADMINCFG[] parameter.
 *
 * @see MAdminLoadConfig()
 *
 * @param A (I) [modified]
 * @param Value (I)
 */

int MAdminProcessConfig(

  madmin_t *A,     /* I (modified) */
  char     *Value) /* I */

  {
  int   aindex;

  int   tmpI;    /* attribute operation */

  char *ptr;
  char *TokPtr;

  char  ValLine[MMAX_LINE];

  if ((A == NULL) ||
      (Value == NULL) ||
      (Value[0] == '\0'))
    {
    return(FAILURE);
    }

  /* process value line */

  ptr = MUStrTok(Value," \t\n",&TokPtr);

  while (ptr != NULL)
    {
    /* parse name-value pairs */

    /* FORMAT:  <VALUE>[,<VALUE>] */

    if (MUGetPair(
          ptr,
          (const char **)MAdminAttr,
          &aindex,  /* O */
          NULL,
          TRUE,
          &tmpI,    /* O (comparison) */
          ValLine,
          sizeof(ValLine)) == FAILURE)
      {
      /* cannot parse value pair */

      MDB(3,fCONFIG) MLog("INFO:     unknown attribute '%s' specified for admin %d\n",
        ptr,
        A->Index);

      ptr = MUStrTok(NULL," \t\n",&TokPtr);

      continue;
      }

    switch (aindex)
      {
      case madmaCancelJobs:

        A->AllowCancelJobs = MUBoolFromString(ValLine,FALSE);
	
        break;
  
      case madmaEnableProxy:

        A->EnableProxy = MUBoolFromString(ValLine,FALSE);

        break;

      case madmaName:

        MUStrCpy(A->Name,ValLine,sizeof(A->Name));

        break;
 
      case madmaServiceList:

        {
        int sindex;

        char *SName;    /* service name (don't tokenize it!!!) */
        char *TokPtr;

        char *SSName;   /* sub service name */
        char *SSTokPtr; /* sub service token */

        char DiagnoseString[sizeof("diagnose\0")];
        MUStrCpy(DiagnoseString, "diagnose", sizeof("diagnose"));

        /* FORMAT:  <SERV>[{.:}<SUBSERV>]...[,<SERV>[{.:}<SUBSERV>]...]... */

        SName = MUStrTok(ValLine,", \t",&TokPtr);

        if (SName == NULL)
          {
          /* do not force services to be specified, even though
           * it makes no sense not to have services */

          break;
          }

        SName = MUStrTok(SName,".:",&SSTokPtr);

        if (SName == NULL)
          {
          /* again, do not force the services to be specified */

          break;
          }

        if (!strcasecmp(SName,"mdiag"))
          {
          SName = DiagnoseString;
          }

        if ((SName != NULL) && !strcmp(SName,"ALL"))
          {
          /* set all services to true */

          for (sindex = 0;MUI[sindex].SName != NULL;sindex++)
            {
            MUI[sindex].AdminAccess[A->Index] = TRUE;
            }

          break;
          }  /* END if (!strcmp(SName,"ALL")) */

        if (tmpI == 0)
          {
          /* operation is set - clear all services */

          for (sindex = 0;MUI[sindex].SName != NULL;sindex++)
            {
            if ((sindex == mcsCheckJob) ||
                (sindex == mcsShowResAvail))
              {
              MUI[sindex].AdminAccess[A->Index] = MBNOTSET;
              }
            else
              {
              MUI[sindex].AdminAccess[A->Index] = FALSE;
              }
            }
          }    /* END if (tmpI == 0) */

        if ((SName == NULL) || !strcmp(SName,"NONE"))
          {
          /* no access */

          break;
          }

        /* add requested services */

        while (SName != NULL)
          {
          for (sindex = 0;MUI[sindex].SName != NULL;sindex++)
            {
            if (strcasecmp(SName,MUI[sindex].SName))
              continue;

            MUI[sindex].AdminAccess[A->Index] = TRUE;

            MDB(3,fCONFIG) MLog("INFO:     admin %d service '%s' added\n",
              A->Index,
              SName);

            SSName = MUStrTok(NULL,".:",&SSTokPtr);

            if (SSName == NULL)
              {
              /* remap deprecated services */

              if (sindex == mcsShowConfig)
                {
                sindex = mcsMSchedCtl;
                }  /* END switch (sindex) */
              }
            }    /* END for (sindex) */

          SName = MUStrTok(NULL,", \t",&TokPtr);
          SName = MUStrTok(SName,".:",&SSTokPtr);
          }  /* END while (SName != NULL) */
        }    /* END BLOCK (case madmaServiceList) */

        break;

      case madmaUserList:
        
        /* It is assumed that there is only zero or one "ADMINCFG[1] USERS=" lines
           in moab.cfg.  The user "root" is added to the list of admin users
           by default in MSchedSetDefaults() in case no users are specified
           for Admin 1.

           If we are processing a "ADMINCFG[1] USERS=" line, from the config 
           file before moab has started any iterations, we must assume
           that they intend that to be the definitive list, and we can
           remove the default of "root". 

           However, if moab is operating in a grid configuration, there might be 
           an admin user in the list (in the form of "PEER:MoabMaster"--from moab-private.cfg). 
           This user must be preserved in the list, but must NOT be the first entry
           in the list.  (the first entry is magical and is used to setuid the moab process)
           MAdminProcessConfigUserList appends users to the end of the list, so 
           we will go through some code gymnastics here to save and re-append 
           any such users after we read in values from the config file... (ugh!)
        */

        {
        int uindex;
        int uindex2;
        char  OldUList[MMAX_ADMINUSERS + 1][MMAX_NAME]; /* sync with madmin_t.UName */

        if ((MSched.Iteration == -1) && (A == &MSched.Admin[1]))
          {
          /* NOTE:  preserve peer admin settings */
  
          memcpy(OldUList,A->UName,sizeof(OldUList));
          memset(A->UName,0,sizeof(A->UName));

          }
        
        MAdminProcessConfigUserList(A,ValLine);


        if ((MSched.Iteration == -1) && (A == &MSched.Admin[1]))
          {

          /* the for loop is a copy/paste from MSchedProcessConfig */
  
          for (uindex2 = 0;uindex2 < MMAX_ADMINUSERS;uindex2++)
            {
            if (OldUList[uindex2][0] == '\0')
              break;
  
            /* remember the purpose here is to eliminate the default "root" */
            /* so don't copy it back into the original list                 */
           
            if (!strncasecmp(OldUList[uindex2],"root",strlen("root")))            
              continue;
  
            for (uindex = 0;uindex < MMAX_ADMINUSERS;uindex++)
              {
              if (!strcasecmp(A->UName[uindex],OldUList[uindex2]))
                break;
  
              if (A->UName[uindex][0] == '\0')
                {
                strcpy(A->UName[uindex],OldUList[uindex2]);
  
                break;
                }
              }  /* END for (uindex) */
            }    /* END for (uindex2) */
          }      /* END if (first iteration and admin1) */


        }    /* END BLOCK (case madmaUserList) */
        break;
        
      case madmaGroupList:
        
        {
        char *GName;
        char *TokPtr2;
        struct group *grp;
        int uindex;
        char UserList[MMAX_LINE];
        int SpaceLeft;

        GName = MUStrTok(ValLine,", \t",&TokPtr2);
        while (GName != NULL) /* Each Group listed */
          {
          /* Create a comma delimited list of users in the group to pass to
             MAdminProcessConfigUserList(). We have to do this because
             getgrent() may use a static buffer for the list of group
             members, and MAdminProcessConfigUserList() can call MUserAdd
             which eventually calls getgrent() again wiping out our list. */

          uindex=0;
          UserList[0] = '\0';
          mbool_t FirstUser = TRUE;
          SpaceLeft = sizeof(UserList);

          setgrent();
          while ((grp = getgrent()) != NULL) /* Each Group entry in system */
            {
            if (!strcmp(grp->gr_name,GName))  /* If it matches */
              {
              while (grp->gr_mem[uindex] != NULL)  /* Each user in the group */
                {
                int ULen = strlen(grp->gr_mem[uindex]);
                if (SpaceLeft > ULen) /* Is there room in our buffer? */
                  {
                  if (!FirstUser) /* Don't put a comma before the first one. */
                    {
                    strcat(UserList,",");
                    }
                  strcat(UserList,grp->gr_mem[uindex]);
                  FirstUser = FALSE;
                  SpaceLeft -= ULen+1;
                  }
                else
                  {
                  break;  /* No space left, might as well quit now. */
                  }
                  uindex++;
                }
              }
            }
          endgrent();
          if (UserList[0] != '\0')  /* Any users in the list? */
            {
            /* Add them to the list of Admin users. */
            MAdminProcessConfigUserList(A,UserList);
            }
          GName = MUStrTok(NULL,", \t",&TokPtr2);
          }
        }

        break;

      default:

        MDB(4,fAM) MLog("WARNING:  admin attribute '%s' not handled\n",
          MAdminAttr[aindex]);

        break;
      }  /* END switch (aindex) */

    ptr = MUStrTok(NULL," \t\n",&TokPtr);
    }  /* END while (ptr != NULL) */

  return(SUCCESS);
  }  /* END MAdminProcessConfig() */




/**
 *
 *
 * @param A (I)
 * @param AIndex (I)
 * @param String (O)
 * @param DFormat (I)
 */

int MAdminAToString(

  madmin_t *A,
  enum MAdminAttrEnum AIndex,
  mstring_t   *String,
  enum MFormatModeEnum DFormat)

  {
  if (String == NULL)
    {
    return(FAILURE);
    }

  MStringSet(String,"\0");

  if (A == NULL)
    {
    return(FAILURE);
    }

  switch (AIndex)
    {
    case madmaIndex:

      MStringAppendF(String,"%d",
        A->Index);

      break;

    case madmaName:

      if (A->Name[0] != '\0')
        MStringAppend(String,A->Name);

      break;

    case madmaServiceList:

      {
      int sindex;

      /* FORMAT:  <SERVICE>[,<SERVICE>]... */

      for (sindex = 0;MUI[sindex].SName != NULL;sindex++)
        {
        if (MUI[sindex].AdminAccess[A->Index] == FALSE)
          continue;

        if (MUStrIsEmpty(String->c_str()))
          {
          MStringAppendF(String,"%s",
            MUI[sindex].SName);
          }
        else
          {
          MStringAppendF(String,",%s",
            MUI[sindex].SName);
          }
        }     /* END for (sindex) */
      }       /* END BLOCK */

      break;

    case madmaUserList:

      {
      int uindex;

      /* FORMAT:  <SERVICE>[,<SERVICE>]... */

      for (uindex = 0;uindex < MMAX_ADMINUSERS;uindex++)
        {
        if (A->UName[uindex][0] == '\0')
          break;

        if (MUStrIsEmpty(String->c_str()))
          {
          MStringAppendF(String,"%s",
            A->UName[uindex]);
          }
        else
          {
          MStringAppendF(String,",%s",
            A->UName[uindex]);
          }
        }     /* END for (uindex) */
      }       /* END BLOCK */

      break;

    default:

      /* NO-OP */

      break;
    }  /* END switch (AIndex) */

  return(SUCCESS);
  }  /* END MAdminAToString() */
/* END MAdmin.c */
