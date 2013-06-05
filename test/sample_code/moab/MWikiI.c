/* HEADER */

/**
 * @file MWikiI.c
 *
 * Moab Wiki Interface
 */

/*                                                       *
 * Contains:                                             *
 *                                                       *
 *  int MWikiAttrToTaskList(J,TaskList,JobAttr,TaskCount) *
 *  int MWikiGetData(R,NonBlock,Iteration,IsPending,EMsg,SC) *
 *                                                       */


#include "moab.h"
#include "moab-proto.h"
#include "moab-const.h"
#include "moab-global.h"

#include "MWikiInternal.h"



/* MWiki/Slurm globals */

mbool_t MWikiHasComment = MBNOTSET;

/* Global flags set when a SLURM event is processed.
 * A flag is set to indicate that configuration information may require a reload.
 * The flags are checked, processed, and cleared each scheduler iteration.
 */
mbitmap_t MWikiSlurmEventFlags[MMAX_RM]; /* Slurm Event flags (bitmap of enum MWikiSlurmEventFlagEnum) */


/**
 * Initialize WIKI interface.
 *
 * @see MRMInitialize() - parent
 * @see MSLURMInitialize() - child 
 *
 * @param R (I)
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MWikiInitialize(

  mrm_t                *R,    /* I */
  char                 *EMsg, /* O (optional,minsize=MMAX_LINE) */
  enum MStatusCodeEnum *SC)   /* O (optional) */

  {
  const char *FName = "MWikiInitialize";

  MDB(1,fWIKI) MLog("%s(%s,EMsg,SC)\n",
    FName,
    (R != NULL) ? R->Name : "NULL");

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (SC != NULL)
    *SC = mscNoError;
 
  if (R == NULL)
    {
    return(FAILURE);
    }

  if (R->S == NULL)
    {
    R->S = (void *)MUCalloc(1,sizeof(mwikirm_t));
 
    /* set default rm specific values */
 
    /* NYI */
    }

#ifdef __MSLURM
  if (R->SubType == mrmstNONE)
    R->SubType = mrmstSLURM;
#endif /* __MSLURM */

  if (R->Wiki.EventS.sd > 0)
    {
    close(R->Wiki.EventS.sd);

    R->Wiki.EventS.sd = -1;
    }

  if (R->SubType == mrmstSLURM)
    {
    if (getenv("MOABDELTARESOURCEINFO") != NULL)
      {
      bmset(&R->IFlags,mrmifAllowDeltaResourceInfo);
      bmset(&R->Wiki.Flags,mslfNodeDeltaQuery);
      bmset(&R->Wiki.Flags,mslfJobDeltaQuery);
      }

    if (getenv("MOABCOMPRESSOUTPUT") != NULL)
      bmset(&R->Wiki.Flags,mslfCompressOutput);

    bmset(&R->IFlags,mrmifIgnoreDefaultEnv);

    /* NOTE:  load SLURM partition and node index info using 'scontrol' */

    if (R->Version == 0)
      {
      MSLURMGetVersion(R);
      }      /* END if (R->Version == 0) */

    MSLURMInitialize(R);

    /* NOTE:  in SLURM 1.x+, partitions should be mapped to classes (NYI) */

    MSLURMLoadParInfo(R);
    } /* END if ((R->SubType == mrmstSLURM) || (R->SubType == mrmstBG)) */

  if (R->SubType == mrmstSLURM)
    {
    if (MWikiHasComment == MBNOTSET)
      MWikiHasComment = TRUE;

    if (R->P[0].Port == 0)
      R->P[0].Port = MDEF_SLURMSCHEDPORT;

    if ((R->P[1].HostName != NULL) && (R->P[1].Port == 0))
      R->P[1].Port = MDEF_SLURMSCHEDPORT;

    if (R->P[0].HostName == NULL)
      {
      /* NOTE:  setting hostname to localhost may not be required */

      MUStrDup(&R->P[0].HostName,"localhost");
      }

    /* create and configure standard SLURM QOS's */

    MSchedSetEmulationMode(mrmstSLURM);

    /* NOTE:  node access policy determined when parsing partition info */

    if (R->EPort > 0)
      {
      msocket_t tmpS;

      MSUInitialize(
        &tmpS,
        NULL,
        R->EPort,
        MSched.ClientTimeout);

      if (MSUListen(&tmpS,NULL) == FAILURE)
        {
        MDB(1,fWIKI) MLog("WARNING:  cannot connect to Wiki event port %d\n",
          R->EPort);

        R->Wiki.EventS.sd = -1;
        }
      else
        {
        R->Wiki.EventS.sd = tmpS.sd;

        MDB(1,fWIKI) MLog("INFO:     event interface enabled for WIKI RM %s on port %d\n",
          (R != NULL) ? R->Name : "NULL",
          R->EPort);
        }
      }    /* END if (R->EPort > 0) */
    }      /* END if (R->SubType == mrmstSLURM) */

  R->FailIteration = -1;

  return(SUCCESS);
  }  /* END MWikiInitialize() */





/**
 * Initialize WIKI function pointers.
 *
 * @see MRMLoadModule() - parent
 *
 * @param F (I)
 */

int MWikiLoadModule(
 
  mrmfunc_t *F)  /* I */
 
  {
  if (F == NULL)
    {
    return(FAILURE);
    }

  F->IsInitialized  = TRUE;

  F->ClusterQuery   = MWikiClusterQuery; 
  /* 
  F->GetData        = MWikiGetData;
  */
  F->JobCancel      = MWikiJobCancel;
  F->JobCheckpoint  = NULL;
  F->JobModify      = MWikiJobModify;
  F->JobRequeue     = MWikiJobRequeue;
  F->JobResume      = MWikiJobResume;
  F->JobSignal      = MWikiJobSignal;
  F->JobSuspend     = MWikiJobSuspend;
  F->JobSubmit      = MWikiJobSubmit;
  F->JobStart       = MWikiJobStart;
  F->QueueQuery     = NULL;
  F->ResourceModify = NULL;
  F->ResourceQuery  = NULL;
  F->RMEventQuery   = MWikiProcessEvent;
  F->RMInitialize   = MWikiInitialize;
  F->RMQuery        = NULL;
  F->SystemQuery    = MWikiSystemQuery;
  F->WorkloadQuery  = MWikiWorkloadQuery;

  /* F->RMEventQuery = MWikiEventQuery; */
 
  return(SUCCESS);
  }  /* END MWikiLoadModule() */



/**
 * This function takes a hostlist expression and fills in a -1 terminated tasklist array.
 *
 * A hostlist expression with a range can only be delimited with ';', for example odev14;odev[17-18]
 * Only hostlist expressions with a range allow a task count, for example odev[17-18]*8 is ok, 
 * but odev17*8 is not OK.
 *
 * NOTE:  Handles 'TASKLIST=X' WIKI attribute - see 'HOSTLIST=X' and 'TASKS=X' attributes
 *
 * @see MWikiJobLoad() - parent
 *
 * @param J (I)
 * @param TaskList (O) [minsize=MSched.JobMaxTaskCount,terminated w/-1] array of node indexes 
 * @param AList (I) hostlist expression [modified as a side effect]
 * @param TCP (O) [optional]  location to store the number of valid entries in the tasklist
 *
 * @return  This function returns SUCCESS or FAILURE
 */

int MWikiAttrToTaskList(

  mjob_t *J,           /* I (optional) */
  int    *TaskList,    /* O (minsize=MSched.JobMaxTaskCount,terminated w/-1) */
  char   *AList,       /* I (modified as side affect) */
  int    *TCP)         /* O (optional) */

  {
  char     *ptr;
  int       tindex = 0;
  char     *TokPtr = NULL;
  char      DelimList[MMAX_NAME];
  mbool_t   IsRange;

  const char *FName = "MWikiAttrToTaskList";

  MDB(5,fWIKI) MLog("%s(%s,TaskList,%s,TCP)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    AList);

  /* check to see if the hostlist has a range */

  if (MUStringIsRange(AList) == TRUE)
    {
    /* FORMAT:  <HOST>[{,}<HOST>]... or <RANGE>[*<TC>] */

    IsRange = TRUE;
    strcpy(DelimList,",:;"); /* ";" is the Wiki delimeter */
    }
  else
    {
    /* FORMAT:  <HOST>[{,:}<HOST>]... */

    IsRange = FALSE;
    strcpy(DelimList,",:;");
    }

  if (IsRange == TRUE)
    {
    /* Use the "tokenizer" function that ignores delimiters inside the [] */

    ptr = MUStrTokEArray(AList,DelimList,&TokPtr,FALSE);
    }
  else
    {
    ptr = MUStrTok(AList,DelimList,&TokPtr);
    }

  /* process each host or host range between delimiters */

  while (ptr != NULL)
    {
    if (IsRange == TRUE) 
      {
      mnl_t tmpNL;

      int TLCount; /* count of entries added to the tasklist in this iteration of the while loop */

      /* Note - If the host expression also has non range elements (e.g. odev[14-16];odev17) then
       *        when we process the non range element (odev17 in this example) we can still
       *        process that value here even though the element is not a range value. */

      /* Make a Node List from the host string */

      MNLInit(&tmpNL);

      if (MUNLFromRangeString(
            ptr,        /* I - node range expression, (modified as side-affect) */
            NULL,       /* I - delimiters */
            &tmpNL,     /* O - nodelist */
            NULL,       /* O - nodeid list */
            -1,         /* I */
            0,          /* I Set TC to what is given in expression */
            FALSE,
            (J->SubmitRM->IsBluegene) ? TRUE: FALSE) == FAILURE)
        {
        MDB(1,fWIKI) MLog("ALERT:    cannot expand range-based tasklist '%s' in %s\n",
          ptr,
          FName);
        }
      else
        {
        /* Add to the tasklist using the Node List that was just constructed */

        MUTLFromNL(
          &tmpNL,
          &TaskList[tindex],
          MSched.JobMaxTaskCount - tindex,
          &TLCount);

        tindex += TLCount;
        }

      MNLFree(&tmpNL);

      ptr = MUStrTokEArray(NULL,DelimList,&TokPtr,FALSE);
      }  /* END if (IsRange == TRUE) */
    else
      {
      /* range not specified */

      int TaskCount;
      int tmpIndex;
      char *tcPtr;

      tcPtr = strchr(ptr,'*');

      if (tcPtr != NULL) /* for example odev10*2 */
        {
        *tcPtr++ = '\0';

        TaskCount = (int)strtol(tcPtr,NULL,10);

        if (TaskCount == 0) 
          TaskCount = 1;
        }
      else
        {
        TaskCount = 1;
        }

      /* ptr is reported node id - may be physical or virtual */

      if (ptr != NULL)
        {
        /* NOTE - in the future we may wish to change this code to simply
         *        expand the TaskList directly instead of using MUTLFromNodeId()
         *        to do it for us when the TaskCount is greater than 1 */

        /* Add this node to the task list as many times as the task count indicates */

        for (tmpIndex = 0;tmpIndex < TaskCount;tmpIndex++)
          {
          if (MUTLFromNodeId(
              ptr,                                      /* I - NodeId */
              &TaskList[tindex],                        /* O - Location in the tasklist to add new entry */
              MSched.JobMaxTaskCount - tindex) == SUCCESS)   /* I - Number of free entries in the tasklist array */
            {
            tindex++;
            }
          else /* we were unable able to add an entry to the tasklist */
            {
            MDB(1,fWIKI) MLog("ALERT:    cannot process non-range tasklist '%s' in %s\n",
              ptr,
              FName);

            break;
            }
          }  /* END for (tmpIndex) */
        }    /* END if (ptr != NULL) */

      ptr = MUStrTok(NULL,DelimList,&TokPtr);
      }   /* END else (IsRange == TRUE) */
    }     /* END while (ptr != NULL) */

  if (TCP != NULL)
    *TCP = tindex;

  if (tindex == 0)
    {
    /* cannot identify any resources */

    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MWikiAttrToTaskList() */




/**
 * Generates the new gevent instance based on the passed in string
 *
 * @see __MNatClusterProcessData() - parent
 *
 * @param Name   (I) - Name of the gevent object to create
 * @param String (I) - GEvent description
 * @param EMsg   (I) [optional]
 */
int MWikiGEventUpdate(
  char *Name,     /* I */
  char *String,   /* I */
  char *EMsg)     /* O (optional) */
  {
  char *ptr;
  char *TokPtr = NULL;

  mgevent_obj_t *GEvent = NULL;
  mgevent_desc_t *GEDesc = NULL;
  char ParentName[MMAX_LINE];
  enum MGEWikiAttrEnum AIndex;
  enum MXMLOTypeEnum ParentType;

  mulong NewMTime = MSched.Time;

  if ((Name == NULL) || (String == NULL))
    {
    return(FAILURE);
    }

  MGEventAddDesc(Name);

  MGEventGetDescInfo(Name,&GEDesc);

  ptr = MUStrStr(String,(char *)MGEWikiAttr[mgewaParentNode],0,TRUE,FALSE);

  if (ptr != NULL)
    {
    /* parent is a node */
    mnode_t *N = NULL;

    ptr += strlen(MGEWikiAttr[mgewaParentNode]) + 1;
    MUStrCpy(ParentName,ptr,MMAX_LINE);

    ptr = ParentName;

    while ((ptr != NULL) && (!isspace(ptr[0])))
      ptr++;

    ptr[0] = '\0';

    if (MNodeFind(ParentName,&N) == FAILURE)
      {
      return(FAILURE);
      }

    MGEventGetOrAddItem(Name,&N->GEventsList,&GEvent);

    ParentType = mxoNode;
    } /* END if (ptr != NULL) */
  else
    {
    /* Parent is not node, or no parent at all */
    ptr = MUStrStr(String,(char *)MGEWikiAttr[mgewaParentVM],0,TRUE,FALSE);

    if (ptr != NULL)
      {
      /* parent is a VM */
      mvm_t *V = NULL;

      ptr += strlen(MGEWikiAttr[mgewaParentVM]) + 1;
      MUStrCpy(ParentName,ptr,MMAX_LINE);

      ptr = ParentName;

      while ((ptr != NULL) && (!isspace(ptr[0])))
        ptr++;

      ptr[0] = '\0';

      if (MVMFind(ParentName,&V) == FAILURE)
        {
        return(FAILURE);
        }

      MGEventGetOrAddItem(Name,&V->GEventsList,&GEvent);

      ParentType = mxoxVM;
      }
    else
      {
      /* no parent, just attach to scheduler */
      MGEventGetOrAddItem(Name,&MSched.GEventsList,&GEvent);

      ParentType = mxoSched;
      }
    } /* END else of if (ptr != NULL) */

  ptr = NULL;
  ptr = MUStrTokE(String," ;",&TokPtr);
  ptr = MUStrTokE(NULL," ;",&TokPtr);

  /* Pull out parent object first so that we can attach the gevent */

  while (ptr != NULL)
    {
    char *ValPtr;
    AIndex = (enum MGEWikiAttrEnum)MUGetIndexCI(ptr,MGEWikiAttr,TRUE,0);

    if ((ValPtr = MUStrChr(ptr,'=')) == NULL)
      return(FAILURE);

    while (isspace(*ValPtr) || (*ValPtr == '='))
      ValPtr++;

    switch (AIndex)
      {
      case mgewaStatusCode:

        GEvent->StatusCode = (int)strtol(ValPtr,NULL,10);

        break;

      case mgewaMessage:

        MUStrDup(&GEvent->GEventMsg,ValPtr);

        break;

      case mgewaMTime:

        NewMTime = (mulong)strtol(ValPtr,NULL,10);

        break;


      case mgewaSeverity:

        GEvent->Severity = (int)strtol(ValPtr,NULL,10);

        if ((GEvent->Severity > MMAX_GEVENTSEVERITY) || (GEvent->Severity < 0))
          GEvent->Severity = 0;

        break;

      default:

        break;
      } /* END switch (AIndex) */

    ptr = MUStrTokE(NULL," ;",&TokPtr);
    } /* END while (ptr != NULL) */

  if (GEvent->GEventMTime + GEDesc->GEventReArm <= MSched.Time)
    {
    /* process event as new */

    GEvent->GEventMTime = NewMTime;

    MGEventProcessGEvent(
      -1,
      ParentType,
      ParentName,
      Name,
      0.0,
      mrelGEvent,
      GEvent->GEventMsg);
    }

  return(SUCCESS);
  } /* END MWikiGEventUpdate() */




/**
 * Parse wiki object line into name, status, and attribute values.
 *
 * NOTE: will fail if data is corrupt or inadequate space available in ABuf.
 *
 * NOTE: used for bnoth job and node names.
 *
 * NOTE: Name may be name range.
 *
 * @param R      (I)
 * @param OIndex (I)
 * @param Name   (O) [minsize=MMAX_NAME]
 * @param Status (O) [optional]
 * @param Type   (O) [optional,minsize=MMAX_NAME]
 * @param ABuf   (O)
 * @param StartP (I/O)
 */

int MWikiGetAttr(

  mrm_t              *R,
  enum MXMLOTypeEnum  OIndex,
  char               *Name,
  int                *Status,
  char               *Type,
  mstring_t          *ABuf,
  char              **StartP)

  {
  char *ptr;
  char *attr;
  char *tail;

  char  State[MMAX_NAME];

  int   Len;
  int   rc;

  mbool_t BufTooSmall = FALSE;

  const char *FName = "MWikiGetAttr";
 
  MDB(4,fWIKI) MLog("%s(%s,%s,Name,Status,Type,ABuf,StartP)\n",
    FName,
    (R != NULL) ? R->Name : "",
    MXO[OIndex]);

 
  /* FORMAT:  <ID>:<FIELD>=<VALUE>;<FIELD>=<VALUE>...{#|'\0'} */

  /* NOTE: '#' is record delimiter, ';' is field delimiter */

  if (Status != NULL)
    *Status = (int)mjsNONE;

  if (Name != NULL)
    Name[0] = '\0';

  if (Type != NULL)
    Type[0] = '\0';

  if ((StartP == NULL) || (*StartP == NULL))
    {
    return(FAILURE);
    }

  /* locate ID delimiter */

  if ((ptr = MUStrChr(*StartP,':')) == NULL)
    {
    MDB(4,fWIKI) MLog("WARNING:  colon delimiter not located in %s wiki string '%.32s...' in %s\n",
      MXO[OIndex],
      *StartP,
      FName);

    return(FAILURE);
    }

  MUStrCpy(Name,*StartP,MIN(ptr - *StartP + 1,MMAX_LINE));

  attr = ptr + 1;

  /* terminate string at record delimiter ('#'), copy, then restore string */

  ptr = MUStrChr(attr,'#');

  if (ptr != NULL)
    *ptr = '\0';

  rc = MStringSet(ABuf,attr);

  if (ptr != NULL)
    *ptr = '#';

  if (rc == FAILURE)
    {
    BufTooSmall = TRUE;

    MDB(1,fWIKI) MLog("ALERT:    failed to allocate memory large enough hold WIKI attribute list for %s %s\n",
      MXO[OIndex],
      Name);
    }

  if (StartP != NULL)
    {
    if (ptr != NULL)
      *StartP = ptr + 1;
    else
      *StartP = *StartP + strlen(*StartP);
    }  /* END if (rc == FAILURE) */

  /* locate state attribute */

  Len = strlen(MWikiNodeAttr[mwnaState]);

  /* STATE could be at beginning or in middle preceded by ';' and followed by '='.
   * There could be other STATES in the ABuf as well which aren't wiki. */

  ptr = ABuf->c_str();

  while ((ptr = MUStrStr(ptr,(char *)MWikiNodeAttr[mwnaState],0,TRUE,FALSE)) != NULL)
    {
    if ((ptr == ABuf->c_str()) || ((*(ptr - 1) == ';') && (ptr[Len] == '=')))
      {
      ptr += Len;

      break;
      }

    ptr += Len;
    }

  switch (OIndex)
    {
    case mxoNode:

      /* load status */

      if (ptr == NULL)
        {
        if (Status != NULL)
          *Status = mnsNone;
        }
      else
        {
        while (!isalnum(*ptr))
          ptr++;

        if ((tail = MUStrChr(ptr,';')) != NULL)
          {
          strncpy(State,ptr,tail - ptr);

          State[tail - ptr] = '\0';
          }
        else
          {
          MUStrCpy(State,ptr,sizeof(State));
          }

        if (Status != NULL)
          *Status = MUGetIndexCI(State,MNodeState,FALSE,mnsNONE);
        }  /* END else */

      if (Type != NULL)
        {
        /* load type */

        if ((ptr = strstr(ABuf->c_str(),MWikiNodeAttr[mwnaType])) == NULL)
          {
          Type[0] = '\0';
          }
        else
          {
          ptr += strlen(MWikiNodeAttr[mwnaType]);

          while (!isalnum(*ptr))
            ptr++;

          if ((tail = MUStrChr(ptr,';')) != NULL)
            strncpy(Type,ptr,MIN(MMAX_NAME,tail - ptr));
          else
            strcpy(Type,ptr);

          Type[tail - ptr] = '\0';
          }
        }    /* END if (Type != NULL) */

      break;

    case mxoJob:

      if (ptr == NULL)
        {
        if (Status != NULL)
          *Status = mjsNONE;
        }
      else
        {
        while (!isalnum(*ptr))
          ptr++;

        if ((tail = MUStrChr(ptr,';')) != NULL)
          MUStrCpy(State,ptr,MIN(tail - ptr + 1,MMAX_NAME));
        else
          MUStrCpy(State,ptr,MMAX_NAME);

        if (Status != NULL)
          *Status = MUGetIndexCI(State,MJobState,FALSE,mjsNONE);
        }

      break;

    default:

      /* NO-OP */

      break;
    }  /* END switch (OIndex) */

  if (BufTooSmall == TRUE)
    {
    if (R != NULL)
      {
      char tmpLine[MMAX_LINE];

      sprintf(tmpLine,"memory allocation failure loading wiki attrlist for %s %s",
        MXO[OIndex],
        Name);

      MMBAdd(          
        &R->MB,
        tmpLine,
        NULL,
        mmbtNONE,
        0,
        0,
        NULL);
      }

    return(FAILURE);
    }  /* END if (BufTooSmall == TRUE) */

  return(SUCCESS);
  }  /* END MWikiGetAttr() */





/**
 * Issue low-level WIKI command.
 *
 * @param R (I)
 * @param Cmd (I)
 * @param P (I) peer interface
 * @param ReuseConnection (I) [connection is already established]
 * @param Auth (I)
 * @param Command (I)
 * @param DataP (O) [optional,alloc,only populated on success]
 * @param DataSizeP (O) [optional]
 * @param AllowRetry (I)
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MWikiDoCommand(

  mrm_t           *R,          /* I */
  enum MRMFuncEnum Cmd,        /* I */
  mpsi_t          *P,          /* I peer interface */
  mbool_t          ReuseConnection, /* I (connection is already established) */
  enum MRMAuthEnum Auth,       /* I */
  char            *Command,    /* I */
  char           **DataP,      /* O (optional,alloc,only populated on success) */
  long            *DataSizeP,  /* O (optional) */
  mbool_t          AllowRetry, /* I */
  char            *EMsg,       /* O (optional,minsize=MMAX_LINE) */
  int             *SC)         /* O (optional) */

  {
  msocket_t S;

  char *ptr;

  mbool_t UseCheckSum;

  mpsi_t *tmpP;

  enum MStatusCodeEnum tmpSC;

  char *Response;

  char *HostName;
  int   Port;
  long  TO;

  int   tSC;

  const char *FName = "MWikiDoCommand";

  MDB(2,fSCHED) MLog("%s(%s,%s,P,%s,%s,%s,DataP,DataSizeP,%s,EMsg,SC)\n",
    FName,
    (R != NULL) ? R->Name : "NULL",
    MRMFuncType[Cmd],
    MBool[ReuseConnection],
    MRMAuthType[Auth],
    MBool[AllowRetry],
    Command);

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (SC != NULL)
    *SC = -1;

  if (DataP != NULL)
    *DataP = NULL;

  if ((R == NULL) || (P == NULL))
    {
    return(FAILURE);
    }

  HostName = P->HostName;
  Port     = P->Port;
  TO       = P->Timeout; 

  /* timeout specified in seconds or microseconds - convert to microseconds if in seconds */

  if (TO < 1000)
    TO     *= MDEF_USPERSECOND;

  MSUInitialize(&S,HostName,Port,TO);

  if (Auth == mrmapCheckSum)
    UseCheckSum = TRUE;
  else
    UseCheckSum = FALSE;

  if (ReuseConnection == TRUE)
    {
    /* socket is already open */

    S.sd = Port;  /* HACK:  sd routed in via Port arg */
    }
  else
    {
    if (MSUConnect(&S,FALSE,EMsg) == FAILURE)
      {
      int NextServer;
      int rc;

      MDB(0,fSOCK) MLog("ERROR:    cannot connect to server %s:%d - %s\n",
        S.RemoteHost,
        S.RemotePort,
        (EMsg != NULL) ? EMsg : "");

      if ((R->ActingMaster + 1 < MMAX_RMSUBTYPE) && 
          (R->P[R->ActingMaster + 1].HostName != NULL))
        {
        char tmpLine[MMAX_LINE];

        if (R->ActingMaster == 0)
          {
          sprintf(tmpLine,"attempting fallback to server '%s' - cannot connect to primary RM server at host '%s' - %s",
            R->P[R->ActingMaster + 1].HostName,
            R->P[R->ActingMaster].HostName,
            (EMsg != NULL) ? EMsg : "");

          MMBAdd(
            &R->MB,
            tmpLine,
            NULL,
            mmbtNONE,
            0,
            0,
            NULL);
          }

        NextServer = R->ActingMaster + 1;
        }
      else
        {
        NextServer = 0;
        }

      if ((NextServer == R->ActingMaster) || (AllowRetry == FALSE))
        {
        MDB(0,fSOCK) MLog("ERROR:    cannot connect to server %s:%d\n",
          S.RemoteHost,
          S.RemotePort);

        return(FAILURE);
        }

      MDB(0,fSOCK) MLog("ERROR:    cannot connect to server %s:%d - attempting fallback server on host %s\n",
        S.RemoteHost,
        S.RemotePort,
        R->P[NextServer].HostName);

      R->ActingMaster = NextServer;

      rc = MWikiDoCommand(
            R,
            Cmd,
            &R->P[R->ActingMaster],
            ReuseConnection,
            Auth,
            Command,
            DataP,
            DataSizeP,
            FALSE,
            EMsg,
            SC); 

      return(rc);
      }  /* END if (MSUConnect(&S,FALSE,EMsg) == FAILURE) */
    }    /* END else (ReuseConnection == TRUE) */

  S.SBuffer  = Command;
  S.SBufSize = strlen(Command);

  if (MPeerFind(R->Name,TRUE,&tmpP,FALSE) == SUCCESS)
    {
    /* NOTE:  if AUTHTYPE=CHECKSUM is set, default CSAlgo should be DES to maintain support with WIKI interfaces */

    MUStrCpy(S.CSKey,tmpP->CSKey,sizeof(S.CSKey));

    if (tmpP->CSAlgo != mcsaNONE)
      S.CSAlgo = tmpP->CSAlgo;
    else if (UseCheckSum == TRUE)
      S.CSAlgo = mcsaDES;
    else
      S.CSAlgo = MSched.DefaultCSAlgo;
    }
  else
    {
    strcpy(S.CSKey,MSched.DefaultCSKey);

    if (UseCheckSum == TRUE)
      S.CSAlgo = mcsaDES;
    else
      S.CSAlgo = MSched.DefaultCSAlgo;
    }

  /* NOTE:  use MBNOTSET to pass partial WIKI header info */

  if (MSUSendData(
       &S,
       TO,
       (UseCheckSum == TRUE) ? TRUE : MBNOTSET,
       FALSE,
       &tSC,
       EMsg) == FAILURE)
    {
    MDB(0,fSOCK) MLog("ERROR:    cannot send data to server %s:%d\n",
      HostName,
      Port);

    if (S.StatusCode == msfConnRejected)
      {
      int NextServer;
      int rc;

      MDB(0,fSOCK) MLog("ERROR:    cannot connect to server %s:%d\n",
        S.RemoteHost,
        S.RemotePort);

      if ((R->ActingMaster + 1 < MMAX_RMSUBTYPE) &&
          (R->P[R->ActingMaster + 1].HostName != NULL))
        {
        NextServer = R->ActingMaster + 1;
        }
      else
        {
        NextServer = 0;
        }

      if ((NextServer == R->ActingMaster) || (AllowRetry == FALSE))
        {
        MDB(0,fSOCK) MLog("ERROR:    cannot connect to server %s:%d\n",
          S.RemoteHost,
          S.RemotePort);

        MSUDisconnect(&S);

        return(FAILURE);
        }

      MDB(0,fSOCK) MLog("ERROR:    cannot connect to server %s:%d - attempting fallback server on host %s\n",
        S.RemoteHost,
        S.RemotePort,
        R->P[NextServer].HostName);

      R->ActingMaster = NextServer;

      rc = MWikiDoCommand(
            R,
            Cmd,
            &R->P[R->ActingMaster],
            ReuseConnection,
            Auth,
            Command,
            DataP,
            DataSizeP,
            FALSE,
            EMsg,
            SC);

      MSUDisconnect(&S);

      return(rc);
      }  /* END if (S->StatusCode == msfConnRejected) */

    MSUDisconnect(&S);

    return(FAILURE);
    }  /* END if (MSUSendData() == FAILURE) */
  else
    {
    if (R->State == mrmsCorrupt)
      {
      /* interface has recovered */

      MDB(0,fSOCK) MLog("INFO:     corrupt RM %s interface successfully recovered\n",
        R->Name);

      R->State = mrmsActive;
      }
    }   /* END else (MSUSendData() == FAILURE) */

  MDB(1,fRM) MLog("INFO:     command sent to server\n");

  MDB(3,fRM) MLog("INFO:     message sent: '%s'\n",
    Command);

  if (DataP == NULL) 
    {
    /* single sided communication */

    switch (Cmd)
      {
      case mrmClusterQuery:
      case mrmWorkloadQuery:

        /* NO-OP */

        break;

      default:

        MSUDisconnect(&S);

        return(SUCCESS);

        /*NOTREACHED*/

        break;
      }  /* END switch (Cmd) */
    }    /* END if (DataP == NULL) */

  if (MSURecvData(&S,TO,UseCheckSum,&tmpSC,EMsg) == FAILURE)
    {
    switch (tmpSC)
      {
      case mscNoAuth:

        MMBAdd(
          &R->MB,
          "RM communication not authorized - validate keys and restart",
          NULL,
          mmbtNONE,
          0,
          0,
          NULL);

        break;

      default:

        /* NO-OP */

        break;
      }  /* END switch (tmpSC) */

    if (R->SubType == mrmstSLURM)
      {
      /* NOTE:  SLURM does not like to have the connection closed, but also 
                does not correctly send response */

      /* NOTE:  SLURM systems do not correctly report success/failure info back to WIKI */

      MDB(0,fSOCK) MLog("ERROR:    cannot receive data from server %s:%d\n",
        HostName,
        Port);
      }

    MSUDisconnect(&S);

    return(FAILURE);
    }  /* END if (MSURecvData(&S,TO,UseCheckSum,&tmpSC,EMsg) == FAILURE) */

  MDB(4,fUI) MLog("INFO:     received message '%s' from wiki server\n",
    S.RBuffer); 

  MSUDisconnect(&S);

  if ((ptr = strstr(S.RBuffer,MCONST_WRSPTOK)) != NULL)
    {
    ptr += strlen(MCONST_WRSPTOK);

    Response = ptr;
    }
  else if ((ptr = strstr(S.RBuffer,MCONST_WRSPTOK2)) != NULL)
    {
    ptr += strlen(MCONST_WRSPTOK2);

    Response = ptr;
    }
  else
    {
    Response = NULL;
    }

  if ((ptr = strstr(S.RBuffer,MCKeyword[mckStatusCode])) != NULL)
    {
    int tmpSC;

    ptr += strlen(MCKeyword[mckStatusCode]);

    sscanf(ptr,"%d",
      &tmpSC);

    if (SC != NULL)
      *SC = tmpSC;

    if ((tmpSC < 0) ||
       ((R->SubType == mrmstSLURM) && (tmpSC != 0)))
      {
      MDB(0,fSOCK) MLog("ERROR:    command '%s'  SC: %d  response: '%s'\n",
        Command,
        tmpSC,
        (Response == NULL) ? "" : Response);

      if ((Response != NULL) && (EMsg != NULL))
        MUStrCpy(EMsg,Response,MMAX_LINE);

      MSUFree(&S);

      return(FAILURE);
      }
    }    /* END if ((ptr = strstr(S.RBuffer,MCKeyword[mckStatusCode])) != NULL) */
  else
    {
    MDB(0,fSOCK) MLog("ERROR:    cannot determine statuscode for command '%s'  SC: %d  response: '%s'\n",
      Command,
      *SC,
      (Response == NULL) ? "NONE" : Response);

    MSUFree(&S);

    return(FAILURE);
    }

  if (DataP != NULL)
    *DataP = S.RBuffer;

  if (DataSizeP != NULL)
    *DataSizeP = S.RBufSize;

  return(SUCCESS);
  }  /* END MWikiDoCommand() */





/**
 *
 *
 * @param TString (I)
 */

int MWikiTestNode(

  char *TString)  /* I */

  {
  mnode_t *N = NULL;

  if (MNodeCreate(&N,FALSE) == FAILURE)
    {
    exit(1);
    }

  if (MWikiNodeLoad(TString,N,&MRM[0],NULL) == SUCCESS)
    {
    /* test succeeded */

    exit(0);
    }

  exit(1);

  /*NOTREACHED*/

  return(SUCCESS);
  }  /* END MWikiTestNode() */





/**
 *
 *
 * @param TString (I)
 */

int MWikiTestJob(

  char *TString)  /* I */

  {
  int  *tmpTaskList = NULL;

  char *TBuf;
 
  char  EMsg[MMAX_LINE];

  mjob_t *J = NULL;

  if (MJobCreate("testjob",TRUE,&J,NULL) == FAILURE)
    {
    exit(1);
    }

  if (!strncasecmp(TString,"file:",strlen("file:")))
    {
    TBuf = MFULoadNoCache(TString + strlen("file:"),1,NULL,NULL,NULL,NULL);
    }
  else
    {
    TBuf = TString;
    }

  tmpTaskList = (int *)MUMalloc(sizeof(int) * MSched.JobMaxTaskCount);

  if (MWikiJobLoad(
        "testjob",
        TBuf,
        J,
        NULL,
        tmpTaskList,
        &MRM[0],
        EMsg) == FAILURE)
    {
    fprintf(stderr,"ERROR:  %s\n",
      EMsg);

    exit(1);
    }

  /* initial load succeeded */

  if (MRMJobPostLoad(J,tmpTaskList,&MRM[0],EMsg) == FAILURE)
    {
    fprintf(stderr,"ERROR:  MRMJobPostLoad() failed - %s\n",
      EMsg);

    exit(1);
    }

  MUFree((char **)&tmpTaskList);

  /* full test succeeded */

  exit(0);

  /*NOTREACHED*/

  return(SUCCESS);
  }  /* END MWikiTestJob() */





/**
 *
 *
 * @param TString (I)
 */

int MWikiTestCluster(

  char *TString) /* I */

  {
  char      *ptr;
  mwikirm_t *S;

  if (MRM[0].S == NULL)
    MRM[0].S = calloc(1,sizeof(mwikirm_t));

  S = (mwikirm_t *)MRM[0].S;

  ptr = MFULoadNoCache(TString,1,NULL,NULL,NULL,NULL);

  MUStrDup(&S->ClusterInfoBuffer,ptr);

  return(SUCCESS);
  }  /* END MWikiTestCluster() */





/**
 * Convert Attribute-Value pair string to WIKI format string.
 *
 * @param OID     (I) [optional]
 * @param SrcAVP  (I)
 * @param DstWiki (O) ***MUST ALREADY BE INITIALIZED***
 *
 * NOTE:  should be single and double quote aware
 */

int MWikiFromAVP(

  char *OID,
  char *SrcAVP,
  mstring_t *DstWiki)

  {
  char *ptr;

  char *ptr1;
  char *ptr2;

  int   cindex;
  int   len;

  int   DQCount = 0;
  int   SQCount = 0;

  mbool_t OIDFound;

  /* SRC FORMAT:  <OID> <ATTR>=<VAL>[ <ATTR>=<VAL>]... */

  ptr1 = strchr(SrcAVP,'=');

  for (ptr2 = SrcAVP;!isalnum(*ptr2);ptr2++);  /* skip leading white space */

  SrcAVP = ptr2;

  ptr2 = strchr(ptr2,' ');

  if ((ptr1 == NULL) || 
     ((ptr1 != NULL) && ((ptr2 == NULL) || (ptr1 < ptr2))))
    {
    OIDFound = FALSE;
    }
  else
    {
    OIDFound = TRUE;
    }
 
  if ((OIDFound == FALSE) && (OID != NULL))
    {
    MStringAppendF(DstWiki,"%s %s",OID,SrcAVP);

    OIDFound = TRUE;
    }
  else
    {
    MStringAppend(DstWiki,SrcAVP);
    }

  /* convert header to wiki */

  /* adjust header/move to start of first attribute */

  ptr = DstWiki->c_str();

  len = strlen(DstWiki->c_str());

  if (OIDFound == TRUE)
    {
    while ((*ptr != '\0') && !isspace(*(ptr)))
      {
      ptr++;
      }

    if (*ptr != '\0')
      {
      /* colon delimit objectid from first AVP */

      *ptr = ':';
      }
    }  /* END if (OIDFound == TRUE) */

  /* convert attributes to wiki */

  for (cindex = 0;cindex < len;cindex++)
    {
    if (strchr("<",(*DstWiki)[cindex]) != NULL)
      {
      /* skip any xml */

      while ((cindex < len) && ((*DstWiki)[cindex] != '>'))
        {
        cindex++;
        }

      continue;
      }

    if ((*DstWiki)[cindex] == '\'')
      {
      SQCount++;
      }
    else if ((*DstWiki)[cindex] == '\"')
      {
      DQCount++;
      }
    else if ((SQCount % 2 == 0) && (DQCount % 2 == 0))
      {
      if (strchr(" \t\n",(*DstWiki)[cindex]) != NULL)
        (*DstWiki)[cindex] = ';';
      }
    }  /* END for (cindex) */

  MStringAppend(DstWiki,";");

  return(SUCCESS);
  }  /* END MWikiFromAVP() */






#define MWIKI_EVENTSIZE 4  /* number of bytes in WIKI event string (string is zero-padded) */
#define MWIKI_MAXEVENT  20 /* max number of events allowed per iteration */

/**
 * Checks to see if SLURM is sending any events on the event socket. If an event
 * has been sent, Moab reacts to it depending on the type of event. No 
 * acknowledgement is sent by Moab to SLURM upon successfully receiving an event.
 *
 * @param R  (I) The SLURM RM used to check for events.
 * @param SC (O) A status code reflecting how the event polling went. [optional]
 */

int MWikiProcessEvent(

  mrm_t *R,
  int   *SC)

  {
  int   WikiEvent;
  char *EPtr;

  int EventReceived     = FALSE;
  int EventsOutstanding = TRUE;

  int EventCount;

  msocket_t S;
  msocket_t C;

  struct timeval timeout;

  static struct timeval PrevT = { 0,0 };

  char tmpLine[MWIKI_EVENTSIZE + 1];

  if (R == NULL)
    {
    return(FAILURE);
    }

  if (R->Wiki.EventS.sd == -1)
    {
    MDB(8,fWIKI) MLog("INFO:     invalid Wiki event socket\n");

    return(FAILURE);
    }

  /* NOTE:  attempt to read in all outstanding events */

  EventCount = 0;

  while (EventsOutstanding == TRUE)
    {
    fd_set fdset;

    FD_ZERO(&fdset);

    FD_SET(R->Wiki.EventS.sd,&fdset);

    timeout.tv_sec  = 0;
    timeout.tv_usec = 10000;

    if (select(FD_SETSIZE,&fdset,NULL,NULL,&timeout) == -1)
      {
      MDB(1,fWIKI) MLog("ALERT:    select failed checking WIKI event socket\n");

      if (errno != EINTR)
        {
        /* bad failure */

        EventsOutstanding = FALSE;

        break;
        }

      EventsOutstanding = FALSE;

      break;
      }

    if (!FD_ISSET(R->Wiki.EventS.sd,&fdset))
      {
      /* no connections ready */

      MDB(9,fWIKI) MLog("INFO:     no Wiki event socket connections ready\n");

      EventsOutstanding = FALSE;

      break;
      }

    memset(&S,0,sizeof(S));

    S.sd = R->Wiki.EventS.sd;

    if (MSUAcceptClient(&S,&C,NULL) == FAILURE)
      {
      /* cannot accept new socket */

      MDB(5,fWIKI) MLog("ALERT:    cannot accept client on Wiki event socket\n");

      EventsOutstanding = FALSE;

      break;
      }

    EPtr = tmpLine;

    /* event format:  zero terminated 4 character string, ie '1234\0' */
    /* NOTE:  must be 5 bytes, '12\0\0\0' and '0012\0' are ok */

    if (MSURecvPacket(
          C.sd,
          (char **)&EPtr,
          sizeof(tmpLine),
          NULL,
          100000,
          NULL) == FAILURE)
      {
      MDB(2,fWIKI) MLog("ALERT:    cannot read on Wiki event socket\n");

      close(C.sd);

      EventsOutstanding = FALSE;

      break;
      }

    /* Wiki event received */

    close(C.sd);

    WikiEvent = (int)strtol(tmpLine,NULL,10);

    MDB(4,fWIKI) MLog("INFO:     WIKI event %d received\n",
      WikiEvent);

    switch (WikiEvent)
      {
      case MDEF_SLURMEVENT_JOB_STATE_CHANGE:

        bmset(&MWikiSlurmEventFlags[R->Index],mwsefJob);
        EventReceived = TRUE;

        break;

      case MDEF_SLURMEVENT_PARTITION_STATE_CHANGE:

        bmset(&MWikiSlurmEventFlags[R->Index],mwsefPartition);
        EventReceived = TRUE;

        break;

      default:

        /* ignore event */

        break;
      }  /* END switch (WikiEvent) */

    EventCount++;

    if (EventCount >= MWIKI_MAXEVENT)
      {
      /* we have received enough events for now, break out */

      break;
      }
    }  /* END while (EventsOutstanding == TRUE) */

  if (EventReceived == FALSE)
    {
    /* check timer */

    if ((R->EMinTime > 0) && (PrevT.tv_sec > 0))
      {
      long NowMS;
      long EMS;

      MUGetMS(NULL,&NowMS);
      MUGetMS(&PrevT,&EMS);

      if ((NowMS - EMS) > R->EMinTime)
        {
       /* adequate time has passed between events */

        /* clear timer */

        memset(&PrevT,0,sizeof(PrevT));

        return(SUCCESS);
        }
      }

    return(FAILURE);
    }  /* END if (EventReceived == FALSE) */

  /* event received, update timer */

  if (R->EMinTime > 0)
    {
    gettimeofday(&PrevT,NULL);

    return(FAILURE);
    }

  /* new event received */

  return(SUCCESS);
  }  /* END MWikiProcessEvent() */



/**
 * Parse list of preemptees jobids
 *
 * @param PreempteeStr (I) PREEMPT=id1[,id2...]
 * @param Preemptees (O)
 */

int MWillRunParsePreemptees(
  
  char *PreempteeStr,  /*I*/
  mjob_t **Preemptees) /*O*/

  {
  int jIndex;
  int numPreemptees;
  mjob_t *tmpJ;
  char *preempteeIds[MMAX_JOB];
  char *preempteeStr = PreempteeStr + strlen("PREEMPT=");

  if ((numPreemptees = MUStrSplit(
                        preempteeStr,
                        ",",
                        -1,
                        preempteeIds,
                        sizeof(preempteeIds))) <= 0)
    {
    MDB(7,fWIKI) MLog("ALERT:    failed to parse PREEMPTEES (%s) in response string\n",
        PreempteeStr);

    return(FAILURE);
    }

  for (jIndex = 0;jIndex < numPreemptees;jIndex++)
    {
    if (MJobFind(preempteeIds[jIndex],&tmpJ,mjsmExtended) == FAILURE)
      {
      MDB(7,fWIKI) MLog("ALERT:    failed to find preemptee %s from willrun response\n",
          preempteeIds[jIndex]);

      for (jIndex = 0;jIndex < numPreemptees;jIndex++)
        MUFree(&preempteeIds[jIndex]);

      return(FAILURE);
      }

    Preemptees[jIndex] = tmpJ;
    }

  Preemptees[jIndex] = NULL;

  for (jIndex = 0;jIndex < numPreemptees;jIndex++)
    MUFree(&preempteeIds[jIndex]);

  return(SUCCESS);
  }



/**
 * Account for time differences in moab and slurm.
 *
 * @param AvailStartTime (I) Starttime given by willrun
 * @param ReqStartTime (I) Required starttime given by moab
 */

mulong MWillRunAdjustStartTime (

  mulong AvailStartTime, /* Starttime given by willrun */
  mulong ReqStartTime)   /* Required starttime given by moab */

  {
  mulong startTime = 0;
  mulong now = 0;

  /* NOTE: Moab's internal clock can get behind realtime.  Slurm will return realtime 
            information.  Check realtime and if realtime=slurmtime then set the 
            startdate back to moab's internal clock */
      
  MUGetTime(&now,mtmNONE,NULL);

  if (AvailStartTime == now)
    {
    MDB(7,fWIKI) MLog("WARNING:  setting willrun response to moab's current time (%ld)\n",MSched.Time);

    startTime = MSched.Time;
    }
  else if ((ReqStartTime - MSched.Time) == (AvailStartTime - now))
    {
    MDB(7,fWIKI) MLog("WARNING:  setting willrun response to moab's future time (%ld)\n",MSched.Time);

    startTime = ReqStartTime;
    }
  else if ((AvailStartTime - ReqStartTime) <= (now - MSched.Time))
    {
    MDB(7,fWIKI) MLog("WARNING:  setting willrun response to moab's future time (%ld)\n",MSched.Time);

    startTime = ReqStartTime;
    }
  else
    {
    MDB(7,fWIKI) MLog("WARNING:  now=%lu MSched.Time=%lu ReqStartTime=%lu WillRun=%ld\n",
      now,
      MSched.Time,
      ReqStartTime,
      AvailStartTime);

    startTime = AvailStartTime;
    }

  return(startTime);
  }


/**
 * Issue system query request via native interface.
 *
 * @param R (I)
 * @param QType (I) [optional]
 * @param Attribute (I)
 * @param IsBlocking (I)
 * @param Value (O) [minsize=MMAX_BUFFER]
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MWikiSystemQuery(

  mrm_t                *R,           /* I */
  char                 *QType,       /* I (optional) */
  char                 *Attribute,   /* I */
  mbool_t               IsBlocking,  /* I */
  char                 *Value,       /* O (minsize=MMAX_BUFFER) */
  char                 *EMsg,        /* O (optional,minsize=MMAX_LINE) */
  enum MStatusCodeEnum *SC)          /* O (optional) */

  {
  int     rc;

  char   *MPtr;
  char    tmpLine[MMAX_LINE];

  const char *FName = "MWikiSystemQuery";

  MDB(2,fWIKI) MLog("%s(%s,%s,%s,%s,Value,EMsg,SC)\n",
    FName,
    (R != NULL) ? R->Name : "NULL",
    (QType != NULL) ? QType : "NULL",
    (Attribute != NULL) ? "Attribute" : "NULL",
    MBool[IsBlocking]);

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (SC != NULL)
    *SC = mscNoError;

  if ((R == NULL) || (QType == NULL))
    {
    return(FAILURE);
    }

  if (EMsg != NULL)
    MPtr = EMsg;
  else
    MPtr = tmpLine;

  MPtr[0] = '\0';

  if (!strcasecmp(QType,"willrun"))
    {
    mjwrs_t *JW;

    JW = (mjwrs_t *)Attribute;

    rc = MSLURMJobWillRun(
          JW,
          R,
          (char **)Value,
          EMsg,
          SC);

    return(rc);
    }  /* END if (!strcasecmp(QType,"willrun")) */

  return(SUCCESS);
  }  /* END MWikiSystemQuery() */





/**
 * Load cluster/workload data for specified iteration via WIKI interface.
 *
 * NOTE:  If NonBlock is set to false, Moab will block and wait for results.
 *
 * @see MWikiDoCommand() - child
 *
 * @param R (I) [modified]
 * @param NonBlock (I)
 * @param Iteration (I)
 * @param IsPending (O) [optional]
 * @param EMsg (O)
 * @param SC (O) [optional]
 */

int MWikiGetData(

  mrm_t                *R,         /* I (modified) */
  mbool_t               NonBlock,  /* I */
  int                   Iteration, /* I */
  mbool_t              *IsPending, /* O (optional) */
  char                 *EMsg,      /* O (optional,minsize=MMAX_LINE) */
  enum MStatusCodeEnum *SC)        /* O (optional) */

  {
  char   Command[MMAX_LINE];
  char   ArgLine[MMAX_LINE];

  char  *WikiBuf = NULL;

  int    tmpSC;

  int    rc;
  long   Duration;

  long   DataSize;

  mwikirm_t *S;

  const char *FName = "MWikiGetData";

  MDB(1,fWIKI) MLog("%s(%s,%s,%d,IsPending,EMsg,SC)\n",
    FName,
    (R != NULL) ? R->Name : "NULL",
    MBool[NonBlock],
    Iteration);

  /* load, but do not process raw cluster and workload data from WIKI interface */

  if (SC != NULL)
    *SC = mscNoError;

  if (IsPending != NULL)
    *IsPending = FALSE;

  if ((R == NULL) || (R->S == NULL))
    {
    return(FAILURE);
    }

  /* call MWikiDoCommand() for node and workload queries */

  S = (mwikirm_t *)R->S;

  /* with SLURM if R->DeltaGetNodesTime is non-zero, only node id, node state, and node class info is reported */

  /* the first time we query this rm the delta time should be zero */

  sprintf(ArgLine,"%ld:ALL",
    R->DeltaGetNodesTime);

  sprintf(Command,"%s%s %s%s",
    MCKeyword[mckCommand],
    MWMCommandList[mwcmdGetNodes],
    MCKeyword[mckArgs],
    ArgLine);

  MRMStartFunc(R,NULL,mrmXGetData);

  rc = MWikiDoCommand(
        R,
        mrmClusterQuery,
        &R->P[R->ActingMaster],
        FALSE,
        R->AuthType,
        Command,
        &WikiBuf,            /* O (alloc) */
        &DataSize,           /* O */
        TRUE,
        EMsg,                /* O */
        &tmpSC);             /* O */

  MRMEndFunc(R,NULL,mrmXGetData,&Duration);

  MSched.LoadEndTime[mschpRMLoad]      += Duration;
  MSched.LoadStartTime[mschpRMProcess] += Duration;

  if (rc == FAILURE)
    {
    MDB(2,fWIKI) MLog("ALERT:    cannot query cluster info from %s RM\n",
      MRMType[R->Type]);

    return(FAILURE);
    }

  if (tmpSC < 0)
    {
    MDB(2,fWIKI) MLog("ALERT:    cannot query cluster info from %s RM - SC=%d\n",
      MRMType[R->Type],
      tmpSC);

    return(FAILURE);
    }

  if (bmisset(&R->Wiki.Flags,mslfNodeDeltaQuery))
    {
    /* remember the time so that we can use it for our next delta query */

    /* NOTE:  update R->DeltaGetNodesTime for delta-based node info */
    /* with SLURM, only node id, node state, and node class info is reported */

    R->DeltaGetNodesTime = MSched.Time;
    }

  MDB(2,fWIKI) MLog("INFO:     received node list through %s RM\n",
    MRMType[R->Type]);

  if (S->ClusterInfoBuffer != NULL)
    MUFree(&S->ClusterInfoBuffer);

  S->ClusterInfoBuffer = WikiBuf;

  /* load job workload info */

  if (R->Version < 10400)
    {
    /* NOTE:  SLURM 1.2 and SLURM 1.3 have bugs in delta based job info, don't send a delta time*/

    R->DeltaGetJobsTime = 0;
    }

  sprintf(ArgLine,"%ld:ALL",
    R->DeltaGetJobsTime);

  sprintf(Command,"%s%s %s%s",
    MCKeyword[mckCommand],
    MWMCommandList[mwcmdGetJobs],
    MCKeyword[mckArgs],
    ArgLine);

  MRMStartFunc(R,NULL,mrmXGetData);

  rc = MWikiDoCommand(
        R,
        mrmWorkloadQuery,
        &R->P[R->ActingMaster],
        FALSE,
        R->AuthType,
        Command,
        &WikiBuf,    /* O */
        &DataSize,   /* O */
        FALSE,
        EMsg,        /* O */
        &tmpSC);     /* O */

  MRMEndFunc(R,NULL,mrmXGetData,&Duration);

  MSched.LoadEndTime[mschpRMLoad]      += Duration;
  MSched.LoadStartTime[mschpRMProcess] += Duration;

  if (rc == FAILURE)
    {
    MDB(2,fWIKI) MLog("ALERT:    cannot get workload info from %s RM\n",
      MRMType[R->Type]);

    /* NOTE: JOSH must differentiate between empty queue and failure */

    R->WorkloadUpdateIteration = MSched.Iteration;

    return(FAILURE);
    }

  if (tmpSC < 0)
    {
    MDB(2,fWIKI) MLog("ALERT:    cannot get workload info from %s RM\n",
      MRMType[R->Type]);

    /* NOTE: JOSH must differentiate between empty queue and failure */

    return(FAILURE);
    }

  MDB(2,fWIKI) MLog("INFO:     received workload info through %s RM %s (%d bytes)\n",
    MRMType[R->Type],
    R->Name,
    (int)strlen(WikiBuf));

  if (bmisset(&R->Wiki.Flags,mslfJobDeltaQuery))
    {
    /* remember the time so that we can use it for our next delta query */

    /* NOTE:  update R->DeltaGetNodesTime for delta-based node info */
    /* with SLURM, only node id, node state, and node class info is reported */

    R->DeltaGetJobsTime = MSched.Time;
    }

  R->WorkloadUpdateIteration = MSched.Iteration;

  S->WorkloadInfoBuffer = WikiBuf;

  return(SUCCESS);
  }  /* END MWikiGetData() */





/**
 * Parses the time format of slurm.
 *
 * v1.3 FORMAT: min:sec | hour:min:sec | day-hour:min:sec | UNLIMITED 
 * 
 * @param TimeStr The time string [modified].
 * @returns the time in minutes.
 */

int MWikiParseTimeLimit(
    
  char *TimeStr) /* I */

  {
  char *tmpPtr;
  char *dashPtr;
  int   seconds;
  int   itemCount = 0;

  int timeBuckets[16];

  if (TimeStr == NULL)
    {
    return(FAILURE);
    }

  if (MUStrStr(TimeStr,"UNLIMITED",0,FALSE,FALSE))
    {
    return(0);
    }

  /* Place the different segments of the time string into buckets */

  while ((tmpPtr = MUStrChr(TimeStr,':')) != NULL)
    {
    /* check if day-hour */
    if ((dashPtr = MUStrChr(TimeStr,'-')) != NULL)
      {
      timeBuckets[itemCount++] = strtol(TimeStr,NULL,10);   /* day */
      timeBuckets[itemCount++] = strtol(++dashPtr,NULL,10); /* hour */
      }
    else
      {
      timeBuckets[itemCount++] = strtol(TimeStr,NULL,10);   /* day */
      }

    TimeStr = tmpPtr + 1;

    /* catch dangling time spec */

    if ((tmpPtr = MUStrChr(TimeStr,':')) == NULL)
      {
      timeBuckets[itemCount++] = strtol(TimeStr,NULL,10);   /* day */

      break;
      }
    }

  /* Parse time string according to number or colons */

  switch (itemCount)
    {
    case 0:

      seconds = strtol(TimeStr,NULL,10);

      break;

    case 2:

      /* min:sec */
      seconds = (timeBuckets[0] * MCONST_MINUTELEN) + /* min */
                timeBuckets[1];                        /* sec */

      break;

    case 3:

      /* hour:min:sec */
      seconds = (timeBuckets[0] * MCONST_MINPERHOUR * MCONST_MINUTELEN) +  /* hour */
                (timeBuckets[1] * MCONST_MINUTELEN) +                      /* min */
                timeBuckets[2];                                            /* sec */

      break;

    case 4:

      seconds = (timeBuckets[0] * MCONST_HOURSPERDAY * MCONST_MINPERHOUR * MCONST_MINUTELEN) +  /* day */
                (timeBuckets[1] * MCONST_MINPERHOUR * MCONST_MINUTELEN) +  /* hour */
                (timeBuckets[2] * MCONST_MINUTELEN) +                      /* min */
                timeBuckets[3];                                            /* sec */

      break;

    default:

      seconds = 0;

      break;
    }  /* END switch (itemCount) */

  return(seconds);
  }  /* END int MWikiParseTimeLimit() */


/**
 * Get Attribute index
 *
 */

int MWikiGetAttrIndex(

  char  *AttrString,
  char **AttrList,
  int   *AIndexP,     /* O (optional) */
  char  *IName)       /* O (optional,minsize=MMAX_NAME) */

  {
  int aindex;
  int len;

  if (AIndexP != NULL)
    *AIndexP = 0;

  if (IName != NULL)
    IName[0] = '\0';

  if ((AttrString[0] == 'A') && (isdigit(AttrString[1])))
    {
    aindex = (int)strtol(&AttrString[1],NULL,10);

    /* NOTE:  should check for optional attr indexname (NYI) */

    if (aindex > mwnaLAST)
      aindex = 0;

    if (AIndexP != NULL)
      *AIndexP = aindex;

    return(SUCCESS);
    }

  for (aindex = 1;AttrList[aindex] != NULL;aindex++)
    {
    len = strlen(MWikiNodeAttr[aindex]);

    if (!strncasecmp(MWikiNodeAttr[aindex],AttrString,len) && !isalpha(AttrString[len]))
      {
      /* locate optional index name */

      if (AIndexP != NULL)
        *AIndexP = aindex;

      if ((IName != NULL) && (AttrString[len] == '['))
        {
        char *head;
        char *tail;

        head = &AttrString[len + 1];
        tail = strchr(head,']');

        if (tail != NULL)
          {
          len = MIN(MMAX_NAME - 1,tail - head);

          MUStrCpy(IName,head,len + 1);

          IName[len] = '\0';
          }
        }  /* END if (AttrString[len] == '[') */

      return(SUCCESS);
      }  /* END if (!strncasecmp()) */
    }    /* END for (aindex) */

  return(FAILURE);
  }  /* END MWikiGetAttrIndex() */


/**
 * MB to WikiString 
 *
 * @param MB      (O)
 * @param Msg     (I)
 * @param String  (O) [mstring_t must be initialized]
 */


int MWikiMBToWikiString(
  mmb_t *MB,
  char *Msg,
  mstring_t *String)

  {
  if (String == NULL)
    return(FAILURE);

  String->clear();

  if ((MB != NULL) || (Msg != NULL))
    {
    char tmpLine[MMAX_LINE << 2];
    mbool_t MMBPrinted = FALSE;

    /* We need a MMBPrintMesagesToMString to not have to use tmpLine */

    MMBPrintMessages(
      MB,
      mfmNONE,
      TRUE,    /* verbose */
      -1,
      tmpLine,
      sizeof(tmpLine));

    if (tmpLine[0] != '\0')
      {
      MUMStringPack(tmpLine,String);

      MMBPrinted = TRUE;
      }

    if (Msg != NULL)
      {
      if (MMBPrinted == FALSE)
        {
        MUMStringPack(Msg,String);
        }
      else
        {
        mstring_t tmpstring(MMAX_LINE);

        MStringAppend(String,",");
        MUMStringPack(Msg,&tmpstring);

        MStringAppend(String,tmpstring.c_str());
        }
      }
    } /* END  if ((MB != NULL) || (Msg != NULL)) */

  return(SUCCESS);
  } /* END MWikiMBToWikiString() */
 
/* END MWikiI.c */
