/* HEADER */

/**
 * @file MRM.c
 *
 * Moab Resource Manager
 */

#include "moab.h"
#include "moab-proto.h"
#include "moab-const.h"
#include "moab-global.h"
 
/* Forward reference locals */

int MRMLoadCredMap(mrm_t *,char *);


/**
 * Load RM modules.
 *
 */

int MRMModuleLoad(void)

  {
  const char *FName = "MRMModuleLoad";

  MDB(2,fRM) MLog("%s()\n",
    FName);

  MS3LoadModule(&MRMFunc[mrmtS3]);

  MWikiLoadModule(&MRMFunc[mrmtWiki]);

  MNatLoadModule(&MRMFunc[mrmtNative]);

  MMLoadModule(&MRMFunc[mrmtMoab]);

  MRMIModuleLoad();

  return(SUCCESS);
  }  /* END MRMLoadModules() */


/**
 * send 'ping' to specified RM
 *
 * @param R (I)
 * @param SSC (O) [optional]
 */

int MRMPing(

  mrm_t                *R,   /* I */
  enum MStatusCodeEnum *SSC) /* O (optional) */

  {
  int rc;
  int SC;

  const char *FName = "MRMPing";

  MDB(2,fRM) MLog("%s(%s,SC)\n",
    FName,
    (R != NULL) ? R->Name : "NULL");

  if (SSC != NULL)
    {
    *SSC = mscNoError;
    }

  if (MRMFunc[R->Type].Ping == NULL)
    {
    MDB(7,fRM) MLog("ALERT:    cannot ping RM (RM '%s' does not support function '%s')\n",
      R->Name,
      MRMFuncType[mrmPing]);

    return(FAILURE);
    }

  MUThread(
    (int (*)(void))MRMFunc[R->Type].Ping,
    R->P[0].Timeout,
    &rc,
    NULL,
    R,
    2,
    R,
    &SC);

  if (rc != SUCCESS)
    {
    MDB(6,fRM) MLog("INFO:     cannot ping RM (RM '%s' failed in function '%s')\n",
      R->Name,
      MRMFuncType[mrmPing]);

    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MRMPing() */




/**
 * Perform pre-clusterquery functions.
 *
 * @see MRMUpdate - parent
 */

int MRMPreClusterQuery()

  {
  mnode_t *N;

  int nindex;

  /* perform pre-query actions on nodes */

  for (nindex = 0;nindex < MSched.M[mxoNode];nindex++)
    {
    N = MNode[nindex];

    if ((N == NULL) || (N->Name[0] == '\0'))
      break;

    if (N->Name[0] == '\1')
      continue;

    if (bmisset(&MSched.Flags,mschedfAggregateNodeFeatures))
      {
      bmclear(&N->FBM);
      }

    N->AttrList.clear();
    }   /* END for (nindex) */

  return(SUCCESS);
  }  /* END MRMPreClusterQuery() */

 


 
/**
 * Set specific job attribute via RM
 *
 * @see MRMAToString() - peer
 *
 * @param R
 * @param J
 * @param A
 * @param ValLine
 * @param TRM
 * @param TaskList
 * @param AdjustJob
 * @param SC (O) [optional]
 */

int MRMJobSetAttr(

  mrm_t    *R,
  mjob_t   *J,
  void     *A,
  char     *ValLine,
  mtrma_t  *TRM,
  int      *TaskList,
  mbool_t   AdjustJob,
  int      *SC)         /* O (optional) */

  {
  int rc;

  if (SC != NULL)
    *SC = 0;

  if ((R == NULL) || (J == NULL))
    {
    return(FAILURE);
    }

  if (MRMFunc[R->Type].JobSetAttr == NULL)
    {
    MDB(7,fRM) MLog("ALERT:    cannot set job attr on RM (RM '%s' does not support function '%s')\n",
      R->Name,
      MRMFuncType[mrmJobSetAttr]);

    return(SUCCESS);
    }

  MRMStartFunc(R,NULL,mrmJobSetAttr);

  rc = (MRMFunc[R->Type].JobSetAttr)(
    J,
    R,
    A,
    ValLine,
    TRM,
    TaskList,
    AdjustJob,
    SC);

  MRMEndFunc(R,NULL,mrmJobSetAttr,NULL);

  return(rc);
  }  /* END MRMJobSetAttr() */





/**
 * Checks each resource manager to ensure no default classes are routing queues
 * If a resource manager has a routing queue as the default, the default is set to NULL
 *
 * having a routing queue be the default class makes it so that moab can't do 
 * standing reservations
 */

int MRMCheckDefaultClass()

  {
  mrm_t *R;

  int    rmindex;

  for (rmindex = 0;rmindex < MSched.M[mxoRM];rmindex++)
    {
    R = &MRM[rmindex];

    if (R->Name[0] == '\0')
      break;

    if (MRMIsReal(R) == FALSE)
      continue;

    if ((R->DefaultC != NULL) && 
        (R->DefaultC->FType == mqftRouting))
      {
      R->DefaultC = NULL;
      }
    }

  return(SUCCESS);
  }  /* END MRMCheckDefaultClass() */



/**
 * Determine if events reported via any RM interface.
 *
 * NOTE: return SUCCESS if event detected
 *
 */
 
int MRMCheckEvents(void)
 
  {
  int rmindex;

  mbool_t EventFound;
  mbool_t CheckPeer;
 
  mrm_t *R;

  const char *FName = "MRMCheckEvents";

  MDB(9,fRM) MLog("%s()\n",
    FName);

  EventFound = FALSE;
  CheckPeer  = FALSE;
 
  for (rmindex = 0;rmindex < MSched.M[mxoRM];rmindex++)
    {
    R = &MRM[rmindex];

    if (R->Name[0] == '\0')
      break;

    if (MRMIsReal(R) == FALSE)
      continue;

    if ((R->Type == mrmtMoab) && (R->State == mrmsActive))
      CheckPeer = TRUE;

    if (MRMFunc[R->Type].RMEventQuery == NULL)
      {
      MDB(9,fRM) MLog("INFO:     cannot query events on RM (RM '%s' does not support function '%s')\n",
        R->Name,
        MRMFuncType[mrmRMEventQuery]);

      continue;
      }

    if ((MRMFunc[R->Type].RMEventQuery)(R,NULL) == SUCCESS)
      {
      EventFound = TRUE;

      break;
      }
    }  /* END for (rmindex) */

  if (CheckPeer == TRUE)
    {
    static int CurrentIteration = -1;
    static mbool_t PeerReqPendingAtUIStart;

    if (CurrentIteration != MSched.Iteration)
      {
      CurrentIteration = MSched.Iteration;

      PeerReqPendingAtUIStart = MMPeerRequestsArePending(NULL);
      }  /* END if (CurrentIteration != MSched.Iteration) */

    if (PeerReqPendingAtUIStart == TRUE) 
      {
      if (MMPeerRequestsArePending(NULL) == FALSE)
        {
        /* force event to process queued up job/node 
           state change reported by peer */
         
        EventFound = TRUE;

        /* because event is found, UI cycle will be truncated */

        MSched.PeerTruncatedUICycle = TRUE;
        }
      } 
    }  /* END if (CheckPeer == TRUE) */
 
  if (EventFound == FALSE)
    {
    return(FAILURE);
    }

  /* event detected */
 
  return(SUCCESS);
  }  /* END MRMCheckEvents() */



/**
 * Modify generic attribute of system/environment via RM API.
 *
 * @param R (I)
 * @param AType (I) [optional]
 * @param Attribute (I)
 * @param IsBlocking (I)
 * @param Value (O) [minsize=MMAX_LINE]
 * @param Format (I)
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MRMSystemModify(

  mrm_t                *R,
  const char           *AType,
  const char           *Attribute,
  mbool_t               IsBlocking,
  char                 *Value,
  enum MFormatModeEnum  Format,
  char                 *EMsg,
  enum MStatusCodeEnum *SC)

  {
  int rc;
  char tmpLine[MMAX_LINE];

  const char *FName = "MRMSystemModify";

  MDB(2,fRM) MLog("%s(%s,%s,%s,Value,EMsg,SC)\n",
    FName,
    (R != NULL) ? R->Name : "NULL",
    (AType != NULL) ? AType : "NULL",
    (Attribute != NULL) ? Attribute : "NULL");

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (SC != NULL)
    *SC = mscNoError;

  if ((R == NULL) || (Attribute == NULL) || (Value == NULL))
    {
    return(FAILURE);
    }

  if (MRMFunc[R->Type].SystemModify == NULL)
    {
    MDB(7,fRM) MLog("ALERT:    cannot complete system modify on RM (RM '%s' does not support function '%s')\n",
      R->Name,
      MRMFuncType[mrmSystemModify]);

    return(FAILURE);
    }

  MRMStartFunc(R,NULL,mrmSystemModify);

  MUThread(
    (int (*)(void))MRMFunc[R->Type].SystemModify,
    R->P[0].Timeout,
    &rc,
    NULL,
    R,
    8,
    R,
    AType,
    Attribute,
    IsBlocking,
    Value,       /* O */
    Format,
    EMsg,
    SC);

  MRMEndFunc(R,NULL,mrmSystemModify,NULL);

  if (rc != SUCCESS)
    {
    MDB(3,fRM) MLog("ALERT:    cannot complete system modify on RM (RM '%s' failed in function '%s')\n",
      R->Name,
      MRMFuncType[mrmSystemModify]);

    snprintf(tmpLine,sizeof(tmpLine),"cannot modify system (%s)",
      (EMsg != NULL) ? EMsg : "N/A");

    if (rc == mscTimeout)
      MRMSetFailure(R,mrmSystemModify,mscSysFailure,"timeout");
    else
      MRMSetFailure(R,mrmSystemModify,mscSysFailure,tmpLine);

    return(FAILURE);
    }

  MDB(5,fRM) MLog("INFO:     value received for modify qtype=%s attr=%s on RM %s (value: '%.64s')\n",
    (AType != NULL) ? AType : "NULL",
    Attribute,
    R->Name,
    Value);

  return(SUCCESS);
  }  /* END MRMSystemModify() */




/**
 *
 *
 * @param R (I)
 * @param RM (I)
 * @param RCmd (I)
 * @param OData (O) [optional]
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MRMRsvCtl(

  mrsv_t                *R,         /* I */
  mrm_t                 *RM,        /* I */
  enum MRsvCtlCmdEnum    RCmd,      /* I */
  void                 **OData,     /* O (optional) */
  char                  *EMsg,      /* O (optional,minsize=MMAX_LINE) */
  enum MStatusCodeEnum  *SC)        /* O (optional) */

  {
  int rc;

  const char *FName = "MRMRsvCtl";

  MDB(2,fRM) MLog("%s(R,%s,%s,OData,EMsg,SC)\n",
    FName,
    (R != NULL) ? RM->Name : "NULL",
    MRsvCtlCmds[RCmd]);

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (SC != NULL)
    *SC = mscNoError;

  if ((RM == NULL) || (RCmd == mrcmNONE))
    {
    return(FAILURE);
    }

  if (MRMFunc[RM->Type].RsvCtl == NULL)
    {
    MDB(7,fRM) MLog("ALERT:    cannot complete job query on RM (RM '%s' does not support function '%s')\n",
      R->Name,
      MRMFuncType[mrmRsvCtl]);

    return(FAILURE);
    }

  MRMStartFunc(RM,NULL,mrmRsvCtl);

  MUThread(
    (int (*)(void))MRMFunc[RM->Type].RsvCtl,
    RM->P[0].Timeout,
    &rc,
    NULL,
    RM,
    7,
    R,
    RM,
    RCmd,
    OData,
    EMsg,
    SC);

  MRMEndFunc(RM,NULL,mrmRsvCtl,NULL);

  if (rc != SUCCESS)
    {
    MDB(3,fRM) MLog("ALERT:    cannot complete rsv control RM (RM '%s' failed in function '%s')\n",
      RM->Name,
      MRMFuncType[mrmRsvCtl]);

    if (rc == mscTimeout)
      MRMSetFailure(RM,mrmRsvCtl,mscSysFailure,"timeout");
    else
      MRMSetFailure(RM,mrmRsvCtl,mscSysFailure,"cannot perform rsv action");

    return(FAILURE);
    }

  MDB(5,fRM) MLog("INFO:     value received for rsv control action=%s rsv=%s on RM %s (value: 'N/A')\n",
    MRsvCtlCmds[RCmd],
    (R != NULL) ? R->Name : "NULL",
    RM->Name);

  return(SUCCESS);
  }  /* END MRMRsvCtl() */




/**
 * Load resource manager attributes for RMCFG parameter.
 *
 * @see MRMProcessConfig() - child
 *
 * @param RMName (I) [optional]
 * @param Buf (I) [optional] config buf
 * @param PBuf (I) [optional] private config buf
 */

int MRMLoadConfig(
 
  char *RMName,  /* I (optional) */
  char *Buf,     /* I (optional) */
  char *PBuf)    /* I (optional) */
 
  {
  char    IndexName[MMAX_NAME];
 
  char    Value[MMAX_LINE];
 
  char   *ptr;
 
  mrm_t  *R;
  mrm_t  *tmpR;

  int     index;
  int     rmindex;
 
  /* FORMAT:  <KEY>=<VAL>[<WS><KEY>=<VAL>]...         */
  /*          <VAL> -> <ATTR>=<VAL>[:<ATTR>=<VAL>]... */
 
  /* load all/specified RM config info */
 
  if (MSched.ConfigBuffer == NULL)
    {
    return(FAILURE);
    }

  ptr = (Buf != NULL) ? Buf : MSched.ConfigBuffer;
 
  if ((RMName == NULL) || (RMName[0] == '\0'))
    {
    /* load ALL RM config info */
 
    IndexName[0] = '\0';
 
    while (MCfgGetSVal(
             MSched.ConfigBuffer,
             &ptr,
             MCredCfgParm[mxoRM],
             IndexName,
             NULL,
             Value,
             sizeof(Value),
             0,
             NULL) != FAILURE)
      {
      if (MRMFind(IndexName,&R) == FAILURE)
        {
        if (MRMAdd(IndexName,&R) == FAILURE)
          {
          /* unable to locate/create RM */
 
          IndexName[0] = '\0';
 
          continue;
          }

        MRMSetDefaults(R);

        MOLoadPvtConfig((void **)R,mxoRM,NULL,NULL,NULL,PBuf);
        }
 
      /* load RM specific attributes */
 
      MRMProcessConfig(R,Value);
 
      IndexName[0] = '\0';
      }  /* END while (MCfgGetSVal() != FAILURE) */

    for (index = 0;index < MSched.M[mxoRM];index++)
      {
      R = &MRM[index];

      if (R->Name[0] == '\0')
        break;

      if (MRMIsReal(R) == FALSE)
        continue;

      if (MRMCheckConfig(R) == FAILURE)
        {
        /* warn only, do not destroy */

        MDB(2,fSIM) MLog("WARNING:  rm '%s' does not pass config check\n",
          R->Name);
        }
      }
    }    /* END if ((RMName == NULL) || (RMName[0] == '\0')) */
  else
    { 
    /* load specified RM config info */
 
    R = NULL;
 
    while (MCfgGetSVal(
             MSched.ConfigBuffer,
             &ptr,
             MCredCfgParm[mxoRM],
             RMName,
             NULL,
             Value,
             sizeof(Value),
             0,
             NULL) == SUCCESS)
      {
      if ((R == NULL) &&
          (MRMFind(RMName,&R) == FAILURE))
        {
        if (MRMAdd(RMName,&R) == FAILURE)
          {
          /* unable to add rm */
 
          return(FAILURE);
          }  

        MRMSetDefaults(R);

        MOLoadPvtConfig((void **)R,mxoRM,NULL,NULL,NULL,NULL);
        }
 
      /* load RM attributes */
 
      MRMProcessConfig(R,Value);
      }  /* END while (MCfgGetSVal() == SUCCESS) */
 
    if (R == NULL)
      { 
      return(FAILURE);
      }
 
    if (MRMCheckConfig(R) == FAILURE)
      {
      MRMDestroy(&R,NULL);
 
      /* invalid rm destroyed */
 
      return(FAILURE);
      }
    }  /* END else ((RMName == NULL) || (RMName[0] == '\0')) */

  /* make sure that an implicit RM doesn't conflict with another RM */

  for (rmindex = 0;rmindex < MSched.M[mxoRM];rmindex++)
    {
    tmpR = &MRM[rmindex];

    if (tmpR->Name[0] == '\0')
      break;

    if (MRMIsReal(R) == FALSE)
      continue;

    if (tmpR->Index == rmindex)
      continue;

    if ((tmpR->Type == R->Type) &&
        (tmpR->P[0].HostName == NULL))
      {
      char tmpLine[MMAX_LINE];

      /* possible conflict because of ill-defined resource manager */

      snprintf(tmpLine,sizeof(tmpLine),"  ALERT:  RM '%s' has implicit type that may conflict with '%s' - disabling '%s'\n",
        R->Name,
        tmpR->Name,
        R->Name);

      MMBAdd(&MSched.MB,tmpLine,NULL,mmbtNONE,0,0,NULL);

      MMBAdd(&tmpR->MB,tmpLine,NULL,mmbtNONE,0,0,NULL);

      MRMDestroy(&R,NULL);

      return(FAILURE);
      }
    }
 
  return(SUCCESS);
  }  /* END MRMLoadConfig() */




void MRMUpdateInternalJobCounter(

  mrm_t *R)

  {
  job_iter JTI;

  char *ptr;

  int tmpBig = 0;

  mjob_t *J;

  if (MSched.UseMoabJobID == FALSE) /* if we use moabjobids, database, and jobcounter hasn't been set */
    {
    return;
    }

  /* run through the active job table to get the job with the highest number at the end */

  MJobIterInit(&JTI);

  while (MJobTableIterate(&JTI,&J) == SUCCESS)
    {
    ptr = strrchr(J->Name, '.');   /* get to the last . in the name */

    if (ptr != NULL)
      {
      ptr++; /* get the digit at the end of the name */

      /* compare to see if it's bigger than the last one */
      if (tmpBig < atoi(ptr))
        tmpBig = atoi(ptr);
      }
    } /* end for */

  /* we should have the biggest digit now */

  if (tmpBig >= R->JobCounter)
    {
    /* assign last job number + 1 to the job counter */

    R->JobCounter = tmpBig + 1;
    }
  }   /* END MRMUpdateInternalJobCounter() */


/**
 * Add new resource manager to MRM[] table.
 * 
 * @see MRMFind()
 * @see MRMAddInternal()
 *
 * @param RMName (I)
 * @param RP (O) [optional]
 */

int MRMAdd(
 
  const char  *RMName,
  mrm_t      **RP)
 
  {
  int    rmindex;
 
  mrm_t *R;

  const char *FName = "MRMAdd";
 
  MDB(6,fSTRUCT) MLog("%s(%s,%s)\n",
    FName,
    (RMName != NULL) ? "RMName" : "NULL",
    (RP != NULL) ? "RP" : "NULL");

  if (RP != NULL)
    *RP = NULL;
 
  if ((RMName == NULL) || (RMName[0] == '\0'))
    {
    return(FAILURE);
    }
 
  for (rmindex = 0;rmindex < MSched.M[mxoRM];rmindex++)
    {
    R = &MRM[rmindex];
 
    if (!strcasecmp(R->Name,RMName))
      {
      /* RM already exists */
 
      if (RP != NULL)
        *RP = R;
 
      return(SUCCESS);
      }
 
    if ((R->Name[0] != '\0') &&
        (R->Name[0] != '\1'))
      {
      continue;
      } 
 
    /* empty slot found */

    /* create/initialize new record */
 
    if (MRMCreate(RMName,&MRM[rmindex]) == FAILURE)
      {
      MDB(1,fALL) MLog("ERROR:    cannot alloc memory for RM '%s'\n",
        RMName);
 
      return(FAILURE);
      }
 
    R = &MRM[rmindex];

    /* create matching partition for compute resources in MRMInitialize() */

    if (RP != NULL)
      *RP = R;
 
    R->Index = rmindex;

    R->JobRsvRecreate = TRUE;

    R->JobCounter = 1;

    /* update RM record */
 
    MCPRestore(mcpRM,RMName,(void *)R,NULL);

    if (!strcmp(R->Name,"internal"))
      {
      /* handle special case */

      bmset(&R->IFlags,mrmifLocalQueue);

      R->Type = mrmtS3;

      MS3InitializeLocalQueue(R,NULL);

      MSched.LocalQueueIsActive = TRUE;

      /* correct problem where database doesn't have job counter value */

      MRMUpdateInternalJobCounter(R);

      R->JobCounter = MAX(R->JobCounter,MSched.MinJobCounter);
      }
 
    MDB(5,fSTRUCT) MLog("INFO:     RM %s added\n",
      RMName);
 
    return(SUCCESS);
    }    /* END for (rmindex) */
 
  /* end of table reached */
 
  MDB(1,fSTRUCT) MLog("ALERT:    RM table overflow.  cannot add %s\n",
    RMName);

  if (MSched.OFOID[mxoRM] == NULL)
    MUStrDup(&MSched.OFOID[mxoRM],RMName);
 
  return(FAILURE); 
  }  /* END MRMAdd() */





/**
 * Create internal resource manager.
 * 
 * @see MRMInitializeLocalQueue() 
 *
 * @param RP (O) [optional] 
 */

int MRMAddInternal(

  mrm_t **RP)  /* O (optional) */

  {
  mrm_t *R;
  int    Perm;
  int    RequestedPerm;

  mbool_t IsDir;

  static mbool_t CheckSpoolDir = TRUE;

  const char *FName = "MRMAddInternal";

  MDB(6,fSTRUCT) MLog("%s(RP)\n",
    FName);

  if (RP != NULL)
    *RP = NULL;

  if (MRMAdd(MCONST_INTERNALRMNAME,&R) == FAILURE)
    {
    return(FAILURE);
    }

  if (bmisset(&MSched.Flags,mschedfStrictSpoolDirPermissions))
    RequestedPerm = MCONST_STRICTSPOOLDIRPERM;
  else
    RequestedPerm = MCONST_SPOOLDIRPERM;

  if (RP != NULL)
    *RP = R;

  if (MSched.EnableHighThroughput == TRUE)
    {
    R->SyncJobID = TRUE;
    R->JobIDFormatIsInteger = TRUE;
    }

  if ((CheckSpoolDir == TRUE) && (MSched.SpoolDir[0] != '\0'))
    {
    CheckSpoolDir = FALSE;

    if (MFUGetInfo(MSched.SpoolDir,NULL,NULL,NULL,&IsDir) == FAILURE)
      {
      /* attempt to create spooldir */

      MOMkDir(MSched.SpoolDir,RequestedPerm);

      /* NOTE:  if a system umask is present, the spooldir will need
                its permissions fixed. */

      MFUChmod(MSched.SpoolDir,RequestedPerm);
      }
    else if (IsDir == FALSE)
      {
      /* warn that spooldir is not directory and allow admin to fix */

      /* NYI */
      }
    else
      {
      if ((MFUGetAttributes(
             MSched.SpoolDir,
             &Perm,  /* O */
             NULL,
             NULL,
             NULL,
             NULL,
             NULL,
             NULL) == SUCCESS) &&
          ((Perm & RequestedPerm) != RequestedPerm))
        {
        /* set correct permissions on spool dir */

        MFUChmod(MSched.SpoolDir,RequestedPerm);
        }
      }
    }    /* END if ((CheckSpoolDir == TRUE) && (MSched.SpoolDir[0] != '\0')) */

  return(SUCCESS);
  }  /* END MRMAddInternal() */


 
/**
 * Create new resource manager.
 * 
 * @see MRMAdd()
 *
 * @param RMName (I)
 * @param R (O) [modified]
 */

int MRMCreate(
 
  const char *RMName,
  mrm_t      *R)
 
  {
  if (R == NULL)
    {
    return(FAILURE);
    }
 
  /* use static memory for now */
 
  memset(R,0,sizeof(mrm_t));
 
  if ((RMName != NULL) && (RMName[0] != '\0'))
    MUStrCpy(R->Name,RMName,sizeof(R->Name));
 
  return(SUCCESS);
  }  /* END MRMCreate() */




/**
 * Create a new resource on the specified resource manager.
 *
 * 
 * @param Data (I)
 * @param Response (O)
 * @param EMsg (O) can be error message or output information - should already be initialized
 * @param SC (O) [optional]
 */

int MRMVMCreate(

  char                   *Data,
  mstring_t              *Response,
  char                   *EMsg,
  enum   MStatusCodeEnum *SC)

  {
  int    rc;
  char   tmpLine[MMAX_LINE];
  char  *EMsgPtr;

  mrm_t *R;
  long   RMTime;

  const char *FName = "MRMVMCreate";

  MDB(3,fRM) MLog("%s(%s,Response,EMsg,SC)\n",
    FName,
    (Data != NULL) ? Data : "NULL");

  if (SC != NULL)
    *SC = mscNoError;

  EMsgPtr = (EMsg == NULL) ? tmpLine : EMsg;

  R = MSched.ProvRM;

  if ((R == NULL) || (R->Type != mrmtNative))
    {
    /* resource creation only supported on native resource managers */

    return(FAILURE);
    }

  /* Data Format:  <OID>:<CREATE_DATA> */

  if (MRMFunc[R->Type].ResourceCreate == NULL)
    {
    MDB(3,fRM) MLog("ALERT:    cannot create VM (RM '%s' does not support function '%s')\n",
      R->Name,
      MRMFuncType[mrmResourceCreate]);

    return(FAILURE);
    }

  MRMStartFunc(R,NULL,mrmResourceCreate);

  MUThread(
    (int (*)(void))MRMFunc[R->Type].ResourceCreate,
    R->P[0].Timeout,
    &rc,
    NULL,
    R,
    6,    /* parameter count */
    R,
    mxoxVM,
    Data,
    Response,
    EMsgPtr,
    SC);

  MRMEndFunc(R,NULL,mrmResourceCreate,&RMTime);

  MSched.LoadEndTime[mschpRMAction] += RMTime;
  MSched.LoadStartTime[mschpSched]  += RMTime;

  if (rc != SUCCESS)
    {
    char tmpLine[MMAX_LINE];

    MDB(3,fRM) MLog("ALERT:    cannot create resource %s (RM '%s' failed in function '%s')\n",
      Data,
      R->Name,
      MRMFuncType[mrmResourceCreate]);

    snprintf(tmpLine,sizeof(tmpLine),"cannot create resource %s with RM %s - %s",
      Data,
      R->Name,
      EMsgPtr);

    MMBAdd(&R->MB,tmpLine,NULL,mmbtNONE,0,0,NULL);

    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MRMVMCreate() */






/**
 * Set RM default attributes
 * 
 * @see MRMInitialize()
 *
 * @param R (I)
 */

int MRMSetDefaults(

  mrm_t *R)  /* I */

  {
  const char *FName = "MRMSetDefaults";

  MDB(2,fSTRUCT) MLog("%s(%s)\n",
    FName,
    (R != NULL) ? R->Name : "NULL");

  if (R == NULL) 
    {
    return(FAILURE);
    }

  if (R->Type == mrmtNONE)
    {
    /* set general defaults */

    if (strstr(R->Name,".INBOUND") != NULL)
      {
      R->Type = mrmtMoab;

      bmset(&R->Flags,mrmfClient);
      }
    else
      {
      R->Type = MDEF_RMTYPE;
      }

    R->AuthType = MDEF_RMAUTHTYPE;

    /* NOTE:  Set stage threshold w/in MMInitialize() so it only impact grid RM's */

    /* timeout specified in microseconds */

    R->P[0].Timeout = MDEF_RMUSTIMEOUT;
    R->P[1].Timeout = MDEF_RMUSTIMEOUT;

    R->MaxJobHop  = MMAX_JSHOP;
    R->MaxNodeHop = MDEF_RMMAXHOP;

    R->FailIteration = -1;

    R->JobCounter = 1;   /* start counter at index one - some peer RM's cannot handle
                            a jobid of '0' */

    R->PTYString = NULL;
    
    return(SUCCESS);
    }  /* END if (R->Type == mrmtNONE) */

  MUStrCpy(R->StageAction,"tools/stagedata",sizeof(R->StageAction));
 
  /* set rm type specific defaults */

  switch (R->Type)
    {
    default:

      /* NO-OP */

      break;
    }  /* END switch (R->Type) */
 
  return(SUCCESS);
  }  /* END MRMSetDefaults() */





/**
 * Free/remove a dynamic resource manager.
 *
 * @see MNodeDestroy() - child
 *
 * @param RP (I) [cleared,freed]
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 */

int MRMDestroy(
 
  mrm_t **RP,    /* I (cleared,freed) */
  char   *EMsg)  /* O (optional,minsize=MMAX_LINE) */
 
  {
  mrm_t *R;

  int    index;
  int    rfindex;

  if (EMsg !=NULL)
    EMsg[0] = '\0';

  if (RP == NULL)
    {
    return(SUCCESS);
    }

  R = *RP;
 
  if (R == NULL)
    {
    return(SUCCESS);
    }

  if ((R->Type != mrmtNONE) && (MRMFunc[R->Type].Free != NULL))
    {
    (MRMFunc[R->Type].Free)(R);
    }

  /* free dynamic data */

  MUFree(&R->Env);

  MUFree(&R->Description);

  MUFree(&R->ExtHost);

  MPeerFree(&R->P[0]);
  MPeerFree(&R->P[1]);

  MUFree(&R->ND.ServerHost);

  MPSIClearStats(&R->P[0]);

  MUFree((char **)&R->ND.S);

  for (rfindex = 0;rfindex < mrmLAST;rfindex++)
    {
    if (R->ND.URL[rfindex] != NULL)
      MUFree(&R->ND.URL[rfindex]);

    if (R->ND.Path[rfindex] != NULL)
      MUFree(&R->ND.Path[rfindex]);

    if (R->ND.Host[rfindex] != NULL)
      MUFree(&R->ND.Host[rfindex]);
    }  /* END for (rfindex) */

  /* Call the mstring destructor */
  R->ND.QueueQueryData.~mstring_t();
  R->ND.ResourceQueryData.~mstring_t();
  R->ND.WorkloadQueryData.~mstring_t();

  MUFree(&R->AuthAServer);
  MUFree(&R->AuthCServer);
  MUFree(&R->AuthGServer);
  MUFree(&R->AuthQServer);
  MUFree(&R->AuthUServer);

  MULLFree(&R->Variables,MUFREE);

  if (R->Type == mrmtMoab)
    {
    MUFree(&R->U.Moab.ReqOS);
    MUFree(&R->U.Moab.ReqArch);
    }

  if (R->AuthA != NULL)
    {
    for (index = 0;index < MMAX_CREDS;index++)
      {
      if (R->AuthA[index] == NULL)
        break;

      MUFree((char **)&R->AuthA);
      }

    MUFree((char **)&R->AuthA);
    }  /* END if (AuthA != NULL) */

  if (R->AuthC != NULL)
    {
    for (index = 0;index < MMAX_CREDS;index++)
      {
      if (R->AuthC[index] == NULL)
        break;

      MUFree((char **)&R->AuthC);
      }

    MUFree((char **)&R->AuthC);
    }  /* END if (AuthC != NULL) */

  if (R->AuthG != NULL)
    {
    for (index = 0;index < MMAX_CREDS;index++)
      {
      if (R->AuthG[index] == NULL)
        break;

      MUFree((char **)&R->AuthG);
      }

    MUFree((char **)&R->AuthG);
    }  /* END if (AuthG != NULL) */

  if (R->AuthQ != NULL)
    {
    for (index = 0;index < MMAX_CREDS;index++)
      {
      if (R->AuthQ[index] == NULL)
        break;

      MUFree((char **)&R->AuthQ);
      }

    MUFree((char **)&R->AuthQ);
    }  /* END if (AuthQ != NULL) */

  if (R->AuthU != NULL)
    {
    for (index = 0;index < MMAX_CREDS;index++)
      {
      if (R->AuthU[index] == NULL)
        break;

      MUFree((char **)&R->AuthU);
      }

    MUFree((char **)&R->AuthU);
    }  /* END if (AuthU != NULL) */

  /* clear structure */

  if (R->OMap != NULL)
    {
    int index = 0;

    MUFree(&R->OMap->Path);

    while (R->OMap->OType[index] != mxoNONE)
      {
      MUFree(&R->OMap->SpecName[index]);
      MUFree(&R->OMap->TransName[index]);

      index++;
      }
 
    MUFree((char **)&R->OMap);
    }

  MUFree(&R->OMapServer);
  MUFree(&R->OMapMsg);
  MUFree(&R->CMapEMsg);
  MUFree(&R->FMapEMsg);

  MUFree(&R->DefHighSpeedAdapter);

  MUFree(&R->SubmitCmd);
  MUFree(&R->StartCmd);

  if (R->MB != NULL)
    {
    MMBFree(&R->MB,TRUE);
    }

  MUFree((char **)&R->NL);

  if (R->MaxN != NULL)
    {
    /* NYI */
    }

  if (R->DataRM != NULL)
    {
    /* only free data RM's that are dynamic as well */

    /* free alloc'd array structure */

    MUFree((char **)&R->DataRM);
    }

  if (R->T != NULL)
    {
    MTListClearObject(R->T,FALSE);

    MUFree((char **)&R->T);
    }  /* END if (R->T != NULL) */

  if (R->OrderedJobs != NULL)
    {
    MUFree((char **)&R->OrderedJobs);
    }

  memset(R,0,sizeof(mrm_t));

  R->Name[0] = '\1';
 
  return(SUCCESS);
  }  /* END MRMDestroy() */

 



/**
 * Validate RM configuration.
 *
 * @param R (I)
 */

int MRMCheckConfig(

  mrm_t *R)  /* I */

  {
  if (R == NULL)
    {
    return(FAILURE);
    }

  if (R->Type == mrmtNative)
    {
    if (R->TypeIsExplicit == FALSE)
      {
      R->SubType = mrmstAgFull;
      }

    if (bmisclear(&R->RTypes))
      bmset(&R->RTypes,mrmrtCompute);

    if (bmisset(&R->RTypes,mrmrtInfo) && (MSched.InfoRM == NULL))
      MSched.InfoRM = R;
    }
  else if (bmisclear(&R->RTypes))
    {
    bmset(&R->RTypes,mrmrtCompute);
    }
  
  return(SUCCESS);
  }  /* END MRMCheckConfig() */



/**
 * Shows the configuration options for an individual resource manager.
 *
 * @param SR (I) [optional]
 * @param Mode (I)
 * @param String (O)
 */

int MRMConfigShow(
 
  mrm_t      *SR,
  int         Mode,
  mstring_t  *String)

  {
  int  rmindex;
  int  aindex;
 
  char tmpLine[MMAX_LINE];
  char tmpVal[MMAX_LINE];
 
  mrm_t *R;

  char *tBPtr;
  int   tBSpace;

  const enum MRMAttrEnum AList[] = {
    mrmaAuthType,
    mrmaAuthAList,
    mrmaAuthCList,
    mrmaAuthGList,
    mrmaAuthQList,
    mrmaAuthUList,
    mrmaConfigFile,
    mrmaCSAlgo,
    mrmaEPort,
    mrmaHost,
    mrmaIgnHNodes,
    mrmaLocalDiskFS,
    mrmaMinETime,
    mrmaNMPort,
    mrmaNMServer,
    mrmaNodeStatePolicy,
    mrmaOMap,
    mrmaPort,
    mrmaSocketProtocol,
    mrmaStartCmd,
    mrmaSubmitCmd,
    mrmaSuspendSig,
    mrmaTargetUsage,
    mrmaTimeout,
    mrmaType,
    mrmaUseVNodes,
    mrmaVMOwnerRM,
    mrmaVersion,
    mrmaWireProtocol,
    mrmaNONE };

  const char *FName = "MRMConfigShow";

  MDB(6,fSTRUCT) MLog("%s(%s,%d,String)\n",
    FName,
    (SR != NULL) ? "SR" : "NULL",
    Mode);

  if (String == NULL)
    {
    return(FAILURE);
    }

  MStringSet(String,"\0");
 
  /* NOTE:  allow mode to specify verbose, etc */
 
  for (rmindex = 0;rmindex < MSched.M[mxoRM];rmindex++)
    {
    R = &MRM[rmindex];

    if (R->Name[0] == '\0')
      break;

    if (R->Name[0] == '\1')
      continue;
 
    if ((SR != NULL) && (SR != R))
      continue;
 
    MUSNInit(&tBPtr,&tBSpace,tmpLine,sizeof(tmpLine));
 
    if (MRMIsReal(R) == FALSE)
      continue;
 
    MUSNPrintF(&tBPtr,&tBSpace,"RMCFG[%s]",
      R->Name);
 
    for (aindex = 0;AList[aindex] != mrmaNONE;aindex++)
      {
      tmpVal[0] = '\0';
 
      MRMAToString(R,AList[aindex],tmpVal,sizeof(tmpVal),0);

      if (tmpVal[0] != '\0')
        {
        MUSNPrintF(&tBPtr,&tBSpace," %s=%s",
          MRMAttr[AList[aindex]],
          tmpVal);
        }
      }    /* END for (aindex) */
 
    MStringAppendF(String,"%s\n",
      tmpLine);
    }  /* END for (rmindex) */
 
  return(SUCCESS);
  }  /* END MRMConfigShow() */





/**
 * Initialize job req before first load.
 *
 * @see MRMJobPreLoad() - parent
 *
 * @param RQ (I)
 * @param R (I) [optional]
 */

int MRMReqPreLoad(
 
  mreq_t *RQ,  /* I */
  mrm_t  *R)   /* I (optional) */
 
  {
  if (RQ == NULL)
    {
    return(FAILURE);
    }

  MNLClear(&RQ->NodeList);

  memset(RQ->ReqNRC,MDEF_RESCMP,sizeof(RQ->ReqNRC));
 
  RQ->DRes.Procs = 1;
  RQ->DRes.Mem   = 0;
  RQ->DRes.Disk  = 0;
  RQ->DRes.Swap  = 0;

  if (R != NULL)
    RQ->RMIndex = R->Index;
 
  return(SUCCESS);
  }  /* END MRMReqPreLoad() */





/**
 */

int MRMFinalizeCycle()

  {
  int rc;
  int SC;

  int rmindex;

  mrm_t *R;

  const char *FName = "MRMFinalizeCycle";

  MDB(3,fRM) MLog("%s()\n",
    FName);

  for (rmindex = 0;rmindex < MSched.M[mxoRM];rmindex++)
    {
    R = &MRM[rmindex];

    if (R->Name[0] == '\0')
      break;

    if (MRMIsReal(R) == FALSE)
      continue;

    if ((R->SubType == mrmstXT3) || (R->SubType == mrmstXT4))
      {
      MRMCheckAllocPartition(R,NULL,TRUE);
      }

    /* reset failures */

    R->FailIteration = -1;

    if (MRMFunc[R->Type].CycleFinalize == NULL)
      {
      MDB(8,fRM) MLog("INFO:     no RM cycle finalize - RM '%s' does not support function '%s'\n",
        R->Name,
        MRMFuncType[mrmXCycleFinalize]);

      continue;
      }

    MUThread(
      (int (*)(void))MRMFunc[R->Type].CycleFinalize,
      R->P[0].Timeout,
      &rc,  /* O */
      NULL,
      R,
      2,
      R,
      &SC);

    if (rc != SUCCESS)
      {
      MDB(3,fRM) MLog("ALERT:    cannot finalize RM cycle (RM '%s' failed in function '%s')\n",
        R->Name,
        MRMFuncType[mrmXCycleFinalize]);

      continue;
      }
    }    /* END for (rmindex) */

  return(SUCCESS);
  }  /* END MRMFinalizeCycle() */



/**
 * Modify node attribute via RM.
 *
 * @see MRMNodeSetPowerState() - peer - adjust node power state
 * @see MNodeProvision() - parent
 *
 * NOTE: if provisioning VM, use RM list of parent node 
 *
 * @param SN (I) [optional]
 * @param SV (I) [optional]
 * @param Ext (I) [optional]
 * @param SR (I) [optional]
 * @param AIndex (I)
 * @param SubAttr (I) [optional]
 * @param Value (I)
 * @param Block (I)
 * @param Mode (I)
 * @param EMsg (I) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MRMNodeModify(

  mnode_t                *SN,      /* I (optional) - specified node */
  mvm_t                  *SV,      /* I (optional) - specified VM */
  char                   *Ext,     /* I extension string (optional) */
  mrm_t                  *SR,      /* I (optional) - specified RM */
  enum MNodeAttrEnum      AIndex,  /* I */
  char                   *SubAttr, /* I (optional) */
  void                   *Value,   /* I */
  mbool_t                 Block,   /* I */
  enum MObjectSetModeEnum Mode,    /* I */
  char                   *EMsg,    /* I (optional,minsize=MMAX_LINE) */
  enum MStatusCodeEnum   *SC)      /* O (optional) */

  {
  int rmindex;
  int rmcount;

  int rc;

  mrm_t *R;

  int RMList[MMAX_RM];

  mnode_t *N;

  long     RMTime;

  const char *FName = "MRMNodeModify";

  MDB(1,fRM) MLog("%s(%s,%s,%s,%s,%s,SubAttr,Value,%s,%d,EMsg,SC)\n",
    FName,
    (SN != NULL) ? SN->Name : "",
    (SV != NULL) ? SV->VMID : "",
    (Ext != NULL) ? Ext : "NULL",
    (SR != NULL) ? SR->Name : "NULL",
    (AIndex > mnaNONE) ? MNodeAttr[AIndex] : "NULL",
    MBool[Block],
    Mode);

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if ((SN == NULL) && (Ext == NULL))
    {
    return(FAILURE);
    }

  if (Ext != NULL)
    {
    char *tail;

    if ((tail = strchr(Ext,',')) != NULL)
      *tail = '\0';

    if (MNodeFind(Ext,&N) == FAILURE)
      {
      if (tail != NULL)
        *tail = ',';

      return(FAILURE);
      }

    if (tail != NULL)
      *tail = ',';
    }
  else
    {
    N = SN;
    }

  if (N == NULL)
    {
    return(FAILURE);
    }
 
  switch (MSched.Mode)
    {
    case msmMonitor:
    case msmTest:

      MSchedAddTestOp(mrmNodeModify,mxoNode,(void *)N,NULL);

      MDB(3,fRM) MLog("INFO:     cannot modify node '%s' (monitor mode)\n",
        N->Name);

      return(FAILURE);

      /*NOTREACHED*/

      break;

    default:

      /* NO-OP */

      break;
    }  /* END switch (MSched.Mode) */

  memset(RMList,0,sizeof(RMList));

  rmcount = 0;

  /* determine resource managers used */

  if ((AIndex == mnaOS) && (N->ProvRM != NULL))
    {
    /* NOTE:  should allow multiple ProvRM's per system (FIXME) */

    RMList[0] = N->ProvRM->Index;

    rmcount = 1;
    }
  else if ((AIndex == mnaOS) && (MSched.ProvRM != NULL))
    {
    /* NOTE:  should allow multiple ProvRM's per system (FIXME) */

    RMList[0] = MSched.ProvRM->Index;

    rmcount = 1;
    }
  else if (SR != NULL)
    {
    RMList[0] = SR->Index;

    rmcount = 1;
    }
  else if (N->RMList != NULL)
    {
    for (rmindex = 0;rmindex < rmcount;rmindex++)
      {
      if (RMList[rmindex] == N->RMList[rmindex]->Index)
        {
        break;
        }
      }

    if (rmindex == rmcount)
      {
      RMList[rmindex] = N->RMList[rmindex]->Index;
  
      rmcount++;
      }
    }
  else if (N->RM != NULL)
    {
    RMList[0] = N->RM->Index;

    rmcount = 1;
    }

  for (rmindex = 0;rmindex < rmcount;rmindex++)
    {
    R = &MRM[RMList[rmindex]];

    if (MSched.Iteration == R->FailIteration)
      {
      MDB(3,fRM) MLog("INFO:     cannot modify node '%s' (fail iteration)\n",
        N->Name);

      return(FAILURE);
      }

    if (MRMFunc[R->Type].ResourceModify == NULL)
      {
      MDB(3,fRM) MLog("ALERT:    cannot modify node %s (RM '%s' does not support function '%s')\n",
        N->Name,
        R->Name,
        MRMFuncType[mrmResourceModify]);

      return(FAILURE);
      }

    MRMStartFunc(R,NULL,mrmNodeModify);

    MUThread(
      (int (*)(void))MRMFunc[R->Type].ResourceModify,
      (Block == TRUE) ? R->P[0].Timeout : R->P[0].Timeout,
      &rc,
      NULL,
      R,
      9,    /* parameter count */
      SN,
      SV,
      Ext,
      R,
      AIndex,
      Value,
      Mode,
      EMsg,
      SC);

    MRMEndFunc(R,NULL,mrmNodeModify,&RMTime);

    /* remove time from sched load and add to rm action load */

    MSched.LoadEndTime[mschpRMAction] += RMTime;
    MSched.LoadStartTime[mschpSched]  += RMTime;

    if (rc != SUCCESS)
      {
      char tmpLine[MMAX_LINE];

      MDB(3,fRM) MLog("ALERT:    cannot modify node %s (RM '%s' failed in function '%s')\n",
        N->Name,
        R->Name,
        MRMFuncType[mrmResourceModify]);

      snprintf(tmpLine,sizeof(tmpLine),"cannot modify node %s (%s = %s) with RM %s - %s",
        N->Name,
        MNodeAttr[AIndex],
        (Value != NULL) ? (char *)Value : "", 
        R->Name,
        EMsg);

      MMBAdd(&R->MB,tmpLine,NULL,mmbtNONE,0,0,NULL);

      return(FAILURE);
      }
    }    /* END for (rmindex) */

  return(SUCCESS);
  }  /* END MRMNodeModify() */





/**
 * Modify queue attributes w/in RM.
 *
 * @see MUIRMCtl() - parent
 *
 * @param C (I) [required]
 * @param SR (I) [optional]
 * @param AIndex (I)
 * @param SubAttr (I)
 * @param Value (I)
 * @param Block (I)
 * @param EMsg (I) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MRMQueueModify(

  mclass_t *C,       /* I (required) */
  mrm_t    *SR,      /* I (optional) */
  enum MClassAttrEnum AIndex, /* I */
  char     *SubAttr, /* I */
  void     *Value,   /* I */
  mbool_t   Block,   /* I */
  char     *EMsg,    /* I (optional,minsize=MMAX_LINE) */
  enum MStatusCodeEnum *SC) /* O (optional) */

  {
  int rmindex;
  int rmcount;

  int rc;

  mrm_t *R;

  int RMList[MMAX_RM];

  const char *FName = "MRMQueueModify";

  MDB(1,fRM) MLog("%s(%s,SR,%s,%s,%s,%s,EMsg,SC)\n",
    FName,
    (C != NULL) ? C->Name : "NULL",
    (SubAttr != NULL) ? SubAttr : "NULL",
    (AIndex > mclaNONE) ? MClassAttr[AIndex] : "NULL",
    (Value != NULL) ? "Value" : "NULL",
    MBool[Block]);

  if ((C == NULL) || (AIndex <= mclaNONE))
    {
    return(FAILURE);
    }

  switch (MSched.Mode)
    {
    case msmMonitor:
    case msmTest:

      MDB(3,fRM) MLog("INFO:     cannot modify class '%s' (monitor mode)\n",
        C->Name);

      return(FAILURE);

      /*NOTREACHED*/

      break;

    default:

      /* NO-OP */

      break;
    }  /* END switch (MSched.Mode) */

  memset(RMList,0,sizeof(RMList));

  rmcount = 0;

  /* determine resource managers used */

  if (SR != NULL)
    {
    RMList[0] = SR->Index;

    rmcount = 1;
    }
  else if (C->RM != NULL)
    {
    RMList[0] = ((mrm_t *)C->RM)->Index;

    rmcount = 1;
    }

  for (rmindex = 0;rmindex < rmcount;rmindex++)
    {
    R = &MRM[RMList[rmindex]];

    if (MSched.Iteration == R->FailIteration)
      {
      MDB(3,fRM) MLog("INFO:     cannot modify class '%s' (fail iteration)\n",
        C->Name);

      return(FAILURE);
      }

    if (MRMFunc[R->Type].QueueModify == NULL)
      {
      MDB(3,fRM) MLog("ALERT:    cannot modify class %s (RM '%s' does not support function '%s')\n",
        C->Name,
        R->Name,
        MRMFuncType[mrmQueueModify]);

      return(FAILURE);
      }

    MUThread(
      (int (*)(void))MRMFunc[R->Type].QueueModify,
      (Block == TRUE) ? R->P[0].Timeout : R->P[0].Timeout,
      &rc,
      NULL,
      R,
      7,
      C,
      R,
      AIndex,
      SubAttr,
      Value,
      EMsg,
      SC);

    if (rc != SUCCESS)
      {
      MDB(3,fRM) MLog("ALERT:    cannot modify class %s (RM '%s' failed in function '%s')\n",
        C->Name,
        R->Name,
        MRMFuncType[mrmQueueModify]);

      return(FAILURE);
      }
    }    /* END for (rmindex) */

  return(SUCCESS);
  }  /* END MRMQueueModify() */




/**
 *
 *
 * @param C (I) [required]
 * @param SR (I) [optional]
 * @param QID (I)
 * @param Block (I)
 * @param EMsg (I) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MRMQueueCreate(

  mclass_t *C,       /* I (required) */
  mrm_t    *SR,      /* I (optional) */
  char     *QID,     /* I */
  mbool_t   Block,   /* I */
  char     *EMsg,    /* I (optional,minsize=MMAX_LINE) */
  enum MStatusCodeEnum *SC) /* O (optional) */

  {
  int rmindex;
  int rmcount;

  int rc;

  mrm_t *R;

  int RMList[MMAX_RM];

  const char *FName = "MRMQueueCreate";

  MDB(1,fRM) MLog("%s(%s,R,%s,%s,EMsg,SC)\n",
    FName,
    (C != NULL) ? C->Name : "NULL",
    (QID != NULL) ? QID : "NULL",
    (Block == TRUE) ? "TRUE" : "FALSE");

  if (C == NULL)
    {
    return(FAILURE);
    }

  switch (MSched.Mode)
    {
    case msmMonitor:
    case msmTest:

      MDB(3,fRM) MLog("INFO:     cannot create class '%s' (monitor mode)\n",
        C->Name);

      return(FAILURE);

      /*NOTREACHED*/

      break;

    default:

      /* NO-OP */

      break;
    }  /* END switch (MSched.Mode) */

  memset(RMList,0,sizeof(RMList));

  rmcount = 0;

  /* determine resource managers used */

  if (SR != NULL)
    {
    RMList[0] = SR->Index;

    rmcount = 1;
    }
  else if (C->RM != NULL)
    {
    RMList[0] = ((mrm_t *)C->RM)->Index;

    rmcount = 1;
    }

  for (rmindex = 0;rmindex < rmcount;rmindex++)
    {
    R = &MRM[RMList[rmindex]];

    if (MSched.Iteration == R->FailIteration)
      {
      MDB(3,fRM) MLog("INFO:     cannot create class '%s' (fail iteration)\n",
        C->Name);

      return(FAILURE);
      }

    if (MRMFunc[R->Type].QueueCreate == NULL)
      {
      MDB(3,fRM) MLog("ALERT:    cannot create class %s (RM '%s' does not support function '%s')\n",
        C->Name,
        R->Name,
        MRMFuncType[mrmQueueCreate]);

      return(FAILURE);
      }

    MUThread(
      (int (*)(void))MRMFunc[R->Type].QueueCreate,
      (Block == TRUE) ? R->P[0].Timeout : R->P[0].Timeout,
      &rc,
      NULL,
      R,
      5,
      C,
      R,
      QID,
      EMsg,
      SC);

    if (rc != SUCCESS)
      {
      MDB(3,fRM) MLog("ALERT:    cannot create class %s (RM '%s' failed in function '%s')\n",
        C->Name,
        R->Name,
        MRMFuncType[mrmQueueCreate]);

      return(FAILURE);
      }
    }    /* END for (rmindex) */

  return(SUCCESS);
  }  /* END MRMQueueCreate() */





/**
 * Record failure into RM subsystem.
 *
 * WARNING: do not hand a static string to this routine with a '\n' in it
 *
 * @param R (I)
 * @param FunctionType (I)
 * @param FailureType (I)
 * @param FMsg (I) [optional]
 */

int MRMSetFailure(

  mrm_t *R,
  int    FunctionType,
  enum MStatusCodeEnum FailureType,
  const char  *FMsg)

  {
  int findex;
  int rc;

  enum MRMStateEnum OldState;

  mpsi_t *P;

  if (R == NULL)
    {
    return(FAILURE);
    }

  OldState = R->State;

  P = &R->P[0];  /* NOTE:  record all failures against primary peer */

  /* record failure */

  switch (FailureType)
    {
    case mscBadParam:
    case mscRemoteFailure:
    case mscNoError:

      R->FailCount++;

      break;

    case mscSysFailure:
    default:

      /* widespread RM failure, prevent further calls this iteration */

      R->FailIteration = MSched.Iteration;

      /* update per iteration fail count */

      R->FailCount++;

      break;
    }  /* END switch (FailureType) */
 
  findex = P->FailIndex;

  P->FailTime[findex] = MSched.Time;
  P->FailType[findex] = FunctionType;

  P->RespFailCount[FunctionType]++;

  if ((FMsg != NULL) && (FMsg[0] != '\0'))
    {
    char tmpMsg[MMAX_BUFFER];

    int cindex;
    int len;

    MUStrCpy(tmpMsg,FMsg,sizeof(tmpMsg));

    MUStringChop(tmpMsg);

    MUStrDup(&P->FailMsg[findex],tmpMsg);

    len = strlen(tmpMsg);

    for (cindex = 0;cindex < len;cindex++)
      {
      if (P->FailMsg[findex][cindex] == '\n')
        P->FailMsg[findex][cindex] = '\0';
      }
    }
  else
    {
    MUFree(&P->FailMsg[findex]);    
    }

  P->FailIndex = (findex + 1) % MMAX_PSFAILURE;

  /* adjust interface state */

  switch (FunctionType)
    {
    case mrmClusterQuery:    

      switch (FailureType)
        {
        case mscBadParam:
        case mscBadRequest:
        case mscBadResponse:
        case mscSysFailure:
        case mscNoAuth:

          MRMSetState(R,mrmsCorrupt);

          break;

        case mscRemoteFailure:
        case mscRemoteFailureTransient:
        case mscTimeout:

          MRMSetState(R,mrmsDown);

          break;

        default:

          /* NO-OP */

          break;
        }  /* END switch (FailureType) */
     
      break;

    default:

      /* NO-OP */

      break;
    }  /* END switch (FunctionType) */

  if (OldState != R->State)
    {
    /* perform specific tasks per RM if state has actually changed */

    if (MRMFunc[R->Type].RMFailure == NULL)
      {
      MDB(7,fRM) MLog("INFO:     cannot fail RM (RM '%s' does not support function '%s')\n",
        R->Name,
        MRMFuncType[mrmXRMFailure]);

      return(SUCCESS);
      }

    MUThread(
      (int (*)(void))MRMFunc[R->Type].RMFailure,
      R->P[0].Timeout,
      &rc,
      NULL,
      R,
      2,
      R,
      FailureType);

    if (rc != SUCCESS)
      {
      MDB(6,fRM) MLog("INFO:     cannot fail RM (RM '%s' failed in function '%s')\n",
        R->Name,
        MRMFuncType[mrmXRMFailure]);

      return(FAILURE);
      }
    }  /* END if (OldState != R->State) */

  return(SUCCESS);
  }  /* END MRMSetFailure() */





/**
 * Begin RM function timer.
 *
 * @see MRMEndFunc()
 *
 * @param R (I) [optional]
 * @param SP (I) [optional]
 * @param FType (I)
 */

int MRMStartFunc(

  mrm_t            *R,     /* I (optional) */
  mpsi_t           *SP,    /* I (optional) */
  enum MRMFuncEnum  FType) /* I */

  {
  /* primary peer only */

  mpsi_t *P;

  P = (SP != NULL) ? SP : &R->P[0];  /* primary peer only */

  MUGetMS(NULL,&P->RespStartTime[FType]);
  
  return(SUCCESS);
  }  /* END MRMStartFunc() */





/**
 * End RM function timer and report duration.
 *
 * @see MRMEndFunc()
 *
 * @param R (I) [optional]
 * @param SP (I) [optional]
 * @param FType (I)
 * @param IntervalP (O) [optional]
 */

int MRMEndFunc(

  mrm_t            *R,         /* I (optional) */
  mpsi_t           *SP,        /* I (optional) */
  enum MRMFuncEnum  FType,     /* I */
  long             *IntervalP) /* O (optional) */

  {
  long NowMS;
  long Interval;

  mpsi_t *P;

  if (IntervalP != NULL)
    *IntervalP = 0;

  P = (SP != NULL) ? SP : &R->P[0];  /* primary peer only */

  if (P->RespStartTime[FType] <= 0)
    {
    /* invalid time */

    return(FAILURE);
    }

  MUGetMS(NULL,&NowMS);

  if (NowMS < P->RespStartTime[FType])
    Interval = P->RespStartTime[FType] - NowMS;
  else
    Interval = NowMS - P->RespStartTime[FType];

  if (IntervalP != NULL)
    *IntervalP = Interval;

  P->RespTotalTime[FType] += Interval;
  P->RespMaxTime[FType] = MAX(P->RespMaxTime[FType],Interval);
  P->RespTotalCount[FType]++;

  if (FType != 0)
    {
    P->RespTotalTime[0] += Interval;
    P->RespMaxTime[0] = MAX(P->RespMaxTime[0],Interval);
    P->RespTotalCount[0]++;
    }

  /* reset start time */

  P->RespStartTime[FType] = -1;

  return(SUCCESS);
  }  /* EMD MRMEndFunc() */



/**
 * Initialize table for local/internal job queue.
 *
 * @see MRMAddInteral() 
 *
 */

int MRMInitializeLocalQueue(void)

  {
  int   count;

  char *Head = NULL;
  char String[MMAX_NAME];

  mrm_t *R;
  mdb_t *MDBInfo;

  const char *FName = "MRMInitializeLocalQueue";

  MDB(5,fSTRUCT) MLog("%s()\n",
    FName);

  if (MRMAddInternal(&R) == FAILURE)
    {
    return(FAILURE);
    }

  bmset(&R->IFlags,mrmifLocalQueue);

  R->Type = mrmtS3;

  MS3InitializeLocalQueue(R,NULL);

  /* load checkpoint file */

  MSchedGetAttr(msaDBHandle,mdfNONE,(void **)&MDBInfo,-1);

  if ((MCP.UseDatabase == FALSE) || (MDBInfo->DBType == mdbNONE))
    {
    char tmpBuf[MMAX_BUFFER << 2];
    tmpBuf[0] = '\0';
    if ((Head = MFULoad(MCP.CPFileName,1,macmWrite,&count,NULL,NULL)) == NULL)
      {
      MDB(1,fCKPT) MLog("WARNING:  cannot load checkpoint file '%s'\n",
        MCP.CPFileName);
   
      return(FAILURE);
      }

    snprintf(String,sizeof(String),"%-9s %20s",
      MCPType[mcpRM],
      R->Name);

    if (MUBufGetMatchToken(
         Head,
         String,
         "\n",
         NULL,
         tmpBuf,
         sizeof(tmpBuf)) == NULL)
      {
      MDB(1,fCKPT) MLog("INFO:     no checkpoint info for job '%s'\n",
        R->Name);
      }
    if (tmpBuf[0] != '\0')
      MRMLoadCP(R,tmpBuf);
    }
  else
    {
    mstring_t MString(MMAX_BUFFER);;

    MDBQueryCP(MDBInfo,(char *)MCPType[mcpRM],R->Name,NULL,1,&MString,NULL);

    if (!MString.empty())
      MRMLoadCP(R,MString.c_str());
    }

  MUFree(&Head);

  /* job counter must be >= 1 */

  R->JobCounter = MAX(1,R->JobCounter);
  R->JobCounter = MAX(R->JobCounter,MSched.MinJobCounter);

  return(SUCCESS);
  }  /* END MRMInitializeLocalQueue() */




/**
 * Sets default functionality values for the resource manager object.
 *
 * @param R (I) [modified]
 */

int MRMSetDefaultFnList(

  mrm_t *R)  /* I (modified) */

  {
  if (R == NULL)
    {
    return(FAILURE);
    }

  bmclear(&R->FnList);

  if (MRMFunc[R->Type].ClusterQuery != NULL)
    bmset(&R->FnList,mrmClusterQuery);

  if (MRMFunc[R->Type].InfoQuery != NULL)
    bmset(&R->FnList,mrmInfoQuery);

  if (MRMFunc[R->Type].JobCancel != NULL)
    bmset(&R->FnList,mrmJobCancel);

  if (MRMFunc[R->Type].JobMigrate != NULL)
    bmset(&R->FnList,mrmJobMigrate);

  if (MRMFunc[R->Type].JobModify != NULL)
    bmset(&R->FnList,mrmJobModify);

  if (MRMFunc[R->Type].JobRequeue != NULL)
    bmset(&R->FnList,mrmJobRequeue);

  if (MRMFunc[R->Type].JobCheckpoint != NULL)
    bmset(&R->FnList,mrmJobCheckpoint);

  if (MRMFunc[R->Type].JobResume != NULL)
    bmset(&R->FnList,mrmJobResume);

  if (MRMFunc[R->Type].JobSetAttr != NULL)
    bmset(&R->FnList,mrmJobSetAttr);

  if (MRMFunc[R->Type].JobStart != NULL)
    bmset(&R->FnList,mrmJobStart);

  if (MRMFunc[R->Type].JobSubmit != NULL)
    bmset(&R->FnList,mrmJobSubmit);

  if (MRMFunc[R->Type].JobSuspend != NULL)
    bmset(&R->FnList,mrmJobSuspend);

  if (MRMFunc[R->Type].QueueCreate != NULL)
    bmset(&R->FnList,mrmQueueCreate);

  if (MRMFunc[R->Type].QueueModify != NULL)
    bmset(&R->FnList,mrmQueueModify);

  if (MRMFunc[R->Type].QueueQuery != NULL)
    bmset(&R->FnList,mrmQueueQuery);

  if (MRMFunc[R->Type].ResourceModify != NULL)
    bmset(&R->FnList,mrmResourceModify);

  if (MRMFunc[R->Type].ResourceQuery != NULL)
    bmset(&R->FnList,mrmResourceQuery);

  if (MRMFunc[R->Type].RsvCtl != NULL)
    bmset(&R->FnList,mrmRsvCtl);

  if (MRMFunc[R->Type].RMInitialize != NULL)
    bmset(&R->FnList,mrmRMInitialize);

  /* NOTE:  sizeof FnList bitmap does not allow mrmX values */

  /*
  if (MRMFunc[R->Type].RMFailure != NULL)
    bmset(&R->FnList,mrmXRMFailure);

  if (MRMFunc[R->Type].CredCtl != NULL)
    bmset(&R->FnList,mrmXCredCtl);
  */

  if (MRMFunc[R->Type].RMQuery != NULL)
    bmset(&R->FnList,mrmRMQuery);

  if (MRMFunc[R->Type].SystemModify != NULL)
    bmset(&R->FnList,mrmSystemModify);

  if (MRMFunc[R->Type].SystemQuery != NULL)
    bmset(&R->FnList,mrmSystemQuery);

  if (MRMFunc[R->Type].WorkloadQuery != NULL)
    bmset(&R->FnList,mrmWorkloadQuery);

  if (MRMFunc[R->Type].RMEventQuery != NULL)
    bmset(&R->FnList,mrmRMEventQuery);

  return(SUCCESS);
  }  /* END MRMSetDefaultFnList() */





/**
 * Will return the internal resource manager and populate MSched.InternalRM if this is the
 * first time this function has been called.
 *
 * @param RP (O) Will return a pointer to the internal resource manager in this variable.
 */

int MRMGetInternal(

  mrm_t **RP)  /* O */

  {
  if (RP == NULL)
    {
    return(FAILURE);
    }

  if (MSched.InternalRM != NULL)
    {
    *RP = MSched.InternalRM;
   
    return(SUCCESS);
    }

  if (MRMFind(MCONST_INTERNALRMNAME,RP) == FAILURE)
    {
    if (MRMAddInternal(RP) == FAILURE)
      {
      return(FAILURE);
      }

    MRMInitializeLocalQueue();
    }  /* END if (MRMFind("internal",RP) == FAILURE) */

  MSched.InternalRM = *RP;

  return(SUCCESS);
  }  /* END MRMGetInternal() */





/**
 * Restore RM interface.
 *
 * @see MRMUpdate() - parent
 *
 * NOTE:  called once per iteration regardless of detected RM state 
 * NOTE:  currently only recovers 'Moab' type interfaces 
 */

int MRMRestore()

  {
  int rmindex;

  mrm_t *R;

  /* mulong StandbyBackLog; (NYI) */

  char EMsg[MMAX_LINE];

  for (rmindex = 0;rmindex < MSched.M[mxoRM];rmindex++)
    {
    R = &MRM[rmindex];

    if (R->Name[0] == '\0')
      break;

    if (MRMIsReal(R) == FALSE)
      continue;

    R->FailCount = 0;  /* reset per iteration failure stats */

    if (R->State == mrmsActive)
      {
      if ((R->Type != mrmtMoab) && 
         ((bmisclear(&R->RTypes)) || bmisset(&R->RTypes,mrmrtCompute)) &&
          (!bmisset(&R->Flags,mrmfNoCreateResource)))
        {
        /* active, local, non-slave compute RM located */

        MSched.ActiveLocalRMCount++;
        }

      continue;
      }

    if (bmisset(&R->Flags,mrmfClient))
      {
      /* NOTE:  clients cannot recover their own state */

      continue;
      }

    if ((R->Type == mrmtNative) && (bmisset(&R->IFlags,mrmifRMStartRequired)))
      {
      char RMTool[MMAX_NAME];

      mnat_t *ND;

      RMTool[0] = '\0';

      ND = &R->ND;

      if (MNatRMStart(R,NULL,EMsg,NULL) == FAILURE)
        {
        char tmpLine[MMAX_LINE];

        MDB(2,fNAT) MLog("WARNING:  unable to start RM '%s' - %s\n",
          R->Name,
          (EMsg != NULL) ? EMsg : "???");

        snprintf(tmpLine,sizeof(tmpLine),"RMFAILURE:  unable to start RM %s\n",
          R->Name);

        MSysRegEvent(tmpLine,mactNONE,1);

        continue;
        }

      bmunset(&R->IFlags,mrmifRMStartRequired);

      if (ND->URL[mrmRMInitialize] != NULL)
        {
        MNatRMInitialize(R,RMTool,EMsg,NULL);
        }

      MRMSetState(R,mrmsActive);
      }    /* END else if (R->Type == mrmtNative) */
    else if ((R->Type == mrmtMoab) &&
             ((R->State == mrmsNONE) || 
              (R->State == mrmsConfigured) || 
              (R->State == mrmsCorrupt) ||
              (R->State == mrmsDown)))
      {
      /* Moab will reconnect to Torque in MPBSGetData if it is down. */
      MRMInitialize(R,NULL,NULL);
      }
    else if (R->State == mrmsConfigured)
      {
      /* RM is put in configured state mrmctl -m state=enabled <rmid>  */
      MRMInitialize(R,NULL,NULL);
      }
    }      /* END for (rmindex) */

  /* look for checkpointed internal queue jobs */

  /* NYI */

  return(SUCCESS);
  }  /* END MRMRestore() */



/**
 * Modifies the state of the given resource manager.
 *
 * @param R (I) [modified]
 * @param NewState (I)
 */

int MRMSetState(

  mrm_t             *R,        /* I (modified) */
  enum MRMStateEnum  NewState) /* I */

  {
#ifndef __MOPT
  if (R == NULL)
    {
    return(FAILURE);
    }
#endif /* !__MOPT */

  if ((R->State == NewState) ||
      (NewState == mrmsNONE))
    {
    return(SUCCESS);
    }

  switch (R->State)
    {
    case mrmsActive:

      switch (NewState)
        {
        case mrmsDisabled:
        case mrmsDown:
        case mrmsCorrupt:
        case mrmsConfigured:

          /* Active --> Down/Corrupt */

          R->State = NewState;
 
          R->StateMTime = MSched.Time;

          break;

        default:

          /* NO-OP */

          break;
        }

      break;

    case mrmsDown:
    case mrmsCorrupt:

      switch (NewState)
        {
        case mrmsConfigured:
        case mrmsActive:

          /* Down/Corrupt --> Active/Configured */

          R->State = NewState;
 
          R->StateMTime = MSched.Time;

          break;

        default:

          /* NO-OP */

          break;
        }

      break;

    case mrmsConfigured:
    case mrmsDisabled:

      switch (NewState)
        {
        case mrmsDown:
        case mrmsCorrupt:
        case mrmsActive:
        case mrmsConfigured:

          /* Configured --> Active/Down/Corrupt */

          R->State = NewState;
 
          R->StateMTime = MSched.Time;

          break;

        default:

          /* NO-OP */

          break;
        }

      break;

    case mrmsNONE:
    default:

      switch (NewState)
        {
        case mrmsActive:
        case mrmsConfigured:
        case mrmsCorrupt:
        case mrmsDisabled:
        case mrmsDown:

          /* NONE --> Active/Down/Corrupt/Configured */

          R->State = NewState;

          R->StateMTime = MSched.Time;

          break;

        default:

          /* NO-OP */

          break;
        }

      break;
    }  /* END switch (R->State) */

  return(SUCCESS);
  }  /* END MRMSetState() */




/**
 *
 *
 * @param R (I)
 * @param Action (I)
 * @param CredID (I)
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MRMCredCtl(

  mrm_t                *R,      /* I */  
  const char           *Action, /* I */
  char                 *CredID, /* I */
  char                 *EMsg,   /* O (optional,minsize=MMAX_LINE) */
  enum MStatusCodeEnum *SC)     /* O (optional) */

  {
  char CmdPath[MMAX_LINE];

  int rc;

  if ((R == NULL) || (Action == NULL) || (CredID == NULL))
    {
    return(FAILURE);
    }

  if (R->ND.Path[mrmCredCtl] == NULL)
    {
    return(FAILURE);
    }

  if (!strcasecmp(Action,"renew"))
    {
    snprintf(CmdPath,sizeof(CmdPath),"%s %s",
      R->ND.Path[mrmCredCtl],
      "renew");
    }
  else if (!strcasecmp(Action,"destroy"))
    {
    snprintf(CmdPath,sizeof(CmdPath),"%s %s",
      R->ND.Path[mrmCredCtl],
      "destroy");
    }
  else
    {
    return(FAILURE);
    }

  mstring_t Response(MMAX_LINE);

  rc = MNatDoCommand(
     &R->ND,
     NULL,
     mrmXCredCtl,
     R->ND.Protocol[mrmCredCtl],
     TRUE,
     R->ND.ServerHost,
     CmdPath,            /* I (job description) */
     &Response,          /* O */
     EMsg,
     NULL);              /* O */

  if (rc == FAILURE)
    {
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MRMCredCtl() */







/**
 * Stop/shutdown specified resource manager.
 *
 * @see MRMStart() - peer
 *
 * @param SR (I) [optional]
 * @param EMsg (I) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MRMStop(

  mrm_t                *SR,    /* I (optional) */
  char                 *EMsg,  /* I (optional,minsize=MMAX_LINE) */
  enum MStatusCodeEnum *SC)    /* O (optional) */

  {
  int rmindex;
  int rc;

  char   tEMsg[MMAX_LINE];

  char  *EMsgP;

  mrm_t *R;

  EMsgP = (EMsg != NULL) ? EMsg : tEMsg;

  for (rmindex = 0;rmindex < MSched.M[mxoRM];rmindex++)
    {
    R = &MRM[rmindex];

    if (R->Name[0] == '\0')
      break;

    if (MRMIsReal(R) == FALSE)
      continue;

    if ((SR != NULL) && (SR != R))
      continue;
  
    if ((SR == NULL) && !bmisset(&R->Flags,mrmfAutoSync))
      {
      continue;
      }

    if (R->ND.URL[mrmRMStop] == NULL)
      continue;

    if (MRMFunc[R->Type].RMStop == NULL)
      {
      MDB(3,fRM) MLog("ALERT:    cannot stop RM  (RM '%s' does not support function '%s')\n",
        R->Name,
        MRMFuncType[mrmRMStop]);

      return(FAILURE);
      }

    MUThread(
      (int (*)(void))MRMFunc[R->Type].RMStop,
      R->P[0].Timeout,
      &rc,
      NULL,
      R,
      4,    /* parameter count */
      R,
      NULL,
      EMsgP,
      SC);

    if (rc != SUCCESS)
      {
      char tmpLine[MMAX_LINE];

      MDB(3,fRM) MLog("ALERT:    cannot stop RM (RM '%s' failed in function '%s') - %s\n",
        R->Name,
        MRMFuncType[mrmRMStop],
        EMsgP);

      snprintf(tmpLine,sizeof(tmpLine),"cannot stop RM - %s",
        EMsgP);

      MMBAdd(&R->MB,tmpLine,NULL,mmbtNONE,0,0,NULL);

      return(FAILURE);
      }
    }  /* END for (rmindex) */

  return(SUCCESS);
  }  /* END MRMStop() */



/**
 *
 *
 * @param A (I)
 * @param B (I)
 */

int __MNodeRPSortHighFirst(

  mnalloc_old_t *A,  /* I */
  mnalloc_old_t *B)  /* I */

  {
  /* order hi to low */

  if (A->N->ReleasePriority > B->N->ReleasePriority)
    {
    return(-1);
    }
  else if (A->N->ReleasePriority < B->N->ReleasePriority)
    {
    return(1);
    }

  return(0);
  }  /* END __MNodeRPSortHighFirst() */






/**
 *
 *
 * @param DstN (I) [destination of node migration]
 * @param DstR (I) [RM that new node will report to]
 * @param SrcN (I) [virtualized node to migrate]
 * @param SrcR (I) [RM that SrcN currently reports to]
 * @param Args (I)
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MRMNodeMigrate(
    
  mnode_t *DstN,             /* I (destination of node migration) */
  mrm_t   *DstR,             /* I (RM that new node will report to) */
  mnode_t *SrcN,             /* I (virtualized node to migrate) */
  mrm_t   *SrcR,             /* I (RM that SrcN currently reports to) */
  char    *Args,             /* I */
  char    *EMsg,             /* O (optional,minsize=MMAX_LINE) */
  enum MStatusCodeEnum *SC)  /* O (optional) */

  {
  int rc;
  const char *FName = "MRMNodeMigrate";

  MDB(2,fRM) MLog("%s(%s,%s,%s,%s,%s,EMsg,SC)\n",
    FName,
    (DstN != NULL) ? DstN->Name : "NULL",
    (DstR != NULL) ? DstR->Name : "NULL",
    (SrcN != NULL) ? SrcN->Name : "NULL",
    (SrcR != NULL) ? SrcR->Name : "NULL",
    (Args != NULL) ? Args : "NULL");

  if ((DstN == NULL) ||
      (DstR == NULL) ||
      (SrcN == NULL) ||
      (SrcR == NULL))
    {
    return(FAILURE);
    }

  if (MRMFunc[SrcR->Type].NodeMigrate == NULL)
    {
    MDB(7,fRM) MLog("ALERT:    cannot migrate node from RM (RM '%s' does not support function '%s')\n",
      SrcR->Name,
      MRMFuncType[mrmXNodeMigrate]);

    return(SUCCESS);
    }

  MRMStartFunc(SrcR,NULL,mrmXNodeMigrate);

  MUThread(
    (int (*)(void))MRMFunc[SrcR->Type].NodeMigrate,
    SrcR->P[0].Timeout,
    &rc,
    NULL,
    SrcR,
    5,
    DstN,
    DstR,
    SrcN,
    SrcR,
    Args,
    EMsg,
    SC);

  MRMEndFunc(SrcR,NULL,mrmXNodeMigrate,NULL);

  if (rc == FAILURE)
    {
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MRMNodeMigrate() */





/**
 * Report number of compute resource managers (those which can potentially run a job).
 *
 * @param CountP (O)
 */

int MRMGetComputeRMCount(

  int *CountP)  /* O */

  {
  int rmindex;
  mrm_t *R;
  mrm_t *InternalRM;

  if (CountP == NULL)
    {
    return(FAILURE);
    }

  *CountP = 0;

  MRMGetInternal(&InternalRM);

  for (rmindex = 0;rmindex < MMAX_RM;rmindex++)
    {
    R = &MRM[rmindex];

    if (R->Name[0] == '\0')
      break;

    if (R->Name[0] == '\1')
      continue;

    if (bmisset(&R->Flags,mrmfNoCreateAll) || bmisset(&R->Flags,mrmfNoCreateResource))
      {
      /* ignore non-master RM's which only update attributes of master RM's */

      continue;
      }

    if (R == InternalRM)
      continue;

    if ((!bmisclear(&R->RTypes)) &&
        !bmisset(&R->RTypes,mrmrtCompute))
      continue;

    if (bmisset(&R->Flags,mrmfClient)) /* Client moabs can't be scheduled. */
      continue;

    (*CountP)++;  
    }  /* END for (rmindex) */

  return(SUCCESS);
  }  /* END MRMGetComputeRMCount() */





/**
 * This function increments the resource manager job counter.
 * It checks the max job id and handles wrap around if we have
 * reached the maximum job id. 
 * 
 * Note: The min and max allowed job id is stored in MSched.
 * It returns the job counter before the increment was applied.
 *
 * @param R (O) The owning RM of the JobCounter to check.
 */

int MRMJobCounterIncrement(

  mrm_t *R)  /* O */

  {
  int preIncJobCount;

  if (R == NULL)
    {
    return(FAILURE);
    }

  /* save the current job counter and then increment the job counter */

  preIncJobCount = R->JobCounter++;

  /* check to see if we have a wrap around condition on the job counter */

  if (R->JobCounter > MSched.MaxJobCounter)
    {
    MDB(3,fRM) MLog("INFO:     job counter for RM %s has reached max value (%d) - resetting to %d\n",
      R->Name,
      R->JobCounter,
      MSched.MinJobCounter);

    R->JobCounter = MSched.MinJobCounter;
    }
  else if (R->JobCounter < MSched.MinJobCounter)
    {

    /* note that this situation could occur if the min job id was changed to a higher value
     * in the moab.cfg file and then moab is restarted. In this case when moab starts, workload
     * query might find that the job id of the most recent job is less than the new min job id. */

    MDB(3,fRM) MLog("INFO:     job counter for RM %s set to min job counter %d\n",
      R->Name,
      MSched.MinJobCounter);
 
    R->JobCounter = MSched.MinJobCounter;
    }

  if (R->Type == mrmtS3)
    {
    char tmpInt[MMAX_NAME];

    /* save off internal RM job counter quickly in case of crash */
    snprintf(tmpInt,sizeof(tmpInt),"%d",
      R->JobCounter);

    MCPJournalAdd(mxoRM,R->Name,(char *)MRMAttr[mrmaJobCounter],tmpInt,mAdd);
    }

  return(preIncJobCount);
  }  /* END MRMJobCounterIncrement() */





/**
 * Diagnose/clean-up allocation partitions (for BProc, XT*,etc).
 *
 * @see MRMFinalizeCycle() - parent
 *
 * @param SR (I) [optional]
 * @param String (I) [optional]
 * @param PurgeStalePars (I)
 */

int MRMCheckAllocPartition(

  mrm_t     *SR,
  mstring_t *String,
  mbool_t    PurgeStalePars)

  {
  char EMsg[MMAX_LINE];
  enum MStatusCodeEnum SC;

  char tmpBuf[MMAX_BUFFER];

  int    RIndex;
  mrm_t *R;

  const char *FName = "MRMCheckAllocPartition";

  MDB(3,fRM) MLog("%s(%s,String,%s)\n",
    FName,
    (SR != NULL) ? SR->Name : "",
    MBool[PurgeStalePars]);

  /* don't clear String */
 
  for (RIndex = 0;RIndex < MSched.M[mxoRM];RIndex++)
    {
    R = &MRM[RIndex];

    if (R->Name[0] == '\0')
      break;

    if ((SR != NULL) && (R != SR))
      continue;

    if (R->State != mrmsActive)
      continue;

    MRMSystemQuery(R,"partition","status",TRUE,tmpBuf,EMsg,&SC);

    if ((SC == mscNoError) && (tmpBuf[0] != '\0'))
      {
      char *ptr;
      char *ptr2;
      char *ptr3;
      char *TokPtr;
      char *TokPtr2;
      char *TokPtr3;

      char  JobID[MMAX_NAME];
      char  ParID[MMAX_NAME];

      mulong  CTime;
      mbool_t Orphaned;

      int     Procs;

      enum MXT3ParAttrEnum AIndex;

      if (String != NULL)
        {
        MStringAppendF(String,"\nXT3 Partition Summary (*Orphaned)\n\n");

        MStringAppendF(String," %-12s %-22s %-24s %-5s\n",
          "ID",
          "JobID",
          "Creation Time",
          "Procs");
        }

      ptr3 = MUStrTok(tmpBuf,"\n",&TokPtr3);

      while (ptr3 != NULL)
        {
        ptr = MUStrTok(ptr3," \t",&TokPtr);

        ParID[0] = '\0';
        JobID[0] = '\0';
        CTime    =   0;
        Procs    =   0;
        Orphaned =   FALSE;

        if (ptr != NULL)
          {
          MUStrCpy(ParID,ptr,sizeof(ParID));

          ptr = MUStrTok(NULL," \t",&TokPtr);
          }

        /* FORMAT:  <PARID> <ATTR>=<VAL> []... where <ATTR> is one of {jobid,createtime,orphaned,procs} */

        /* ie: '243879 ADMINCOOKIE=685956333 CREATETIME=1186510251 JOBID=117684 ORPHANED=True PROCS=12
' */

        while (ptr != NULL)
          {
          ptr2 = MUStrTok(ptr,"=",&TokPtr2);

          AIndex = (enum MXT3ParAttrEnum)MUGetIndexCI(ptr2,MXT3ParAttr,FALSE,mxt3paNONE);

          switch (AIndex)
            {
            case mxt3paJobID:

              MUStrCpy(JobID,TokPtr2,sizeof(JobID));

              break;

            case mxt3paCreateTime:

              CTime = (mulong)strtol(TokPtr2,NULL,10);

              break;

            case mxt3paOrphaned:

              Orphaned = MUBoolFromString(TokPtr2,FALSE);

              break;

            case mxt3paProcs:

              Procs = (int)strtol(TokPtr2,NULL,10);

              break;

            default:

              /* NO-OP */

              break;
            }  /* END switch (AIndex) */

          ptr = MUStrTok(NULL," \t",&TokPtr);
          }    /* END while (ptr != NULL) */

        if (String != NULL)
          {
          if ((ParID[0] != '\0') && (JobID[0] != '\0') && (CTime != 0) && (Procs != 0))
            {
            MStringAppendF(String,"%c%-12s %-22s %-24s %-5d\n",
              (Orphaned == TRUE) ? '*' : ' ',
              ParID,
              JobID,
              MULToATString(CTime,NULL),
              Procs);
            }
          }

        if ((PurgeStalePars == TRUE) && (Orphaned == TRUE))
          {
          mjob_t *CJ;

          mjob_t  *tmpJob = NULL;

          /* NOTE:  job IFlags2 is currently not checkpointed */

          if (MJobCFind(JobID,&CJ,mjsmBasic) == FAILURE)
            {
            if (MJobFind(JobID,&CJ,mjsmExtended) == FAILURE)
              {
              MDB(2,fCORE) MLog("INFO:     orphan partition %s reported but cannot locate associated active/completed job %s\n",
                ParID,
                JobID);
              }
            else
              {
              MDB(2,fCORE) MLog("INFO:     orphan partition %s reported for active job %s\n",
                ParID,
                JobID);
              }
            }

          if ((CJ == NULL) && (MSched.AllocPartitionCleanup == TRUE))
            {
            /* aggressive allocation partition cleanup enabled */

            /* job not found in active or completed job tables - create temporary job with
               attributes required by MJobDestroyAllocPartition() */

            MDB(2,fCORE) MLog("INFO:     creating temporary job to process orphan partition %s for job %s\n",
              ParID,
              CJ->Name);

            /* NOTE:  must set cookie, etc */

            MJobMakeTemp(&tmpJob);

            MUStrCpy(tmpJob->Name,JobID,MMAX_NAME);

            CJ = tmpJob;

            /* NYI */
            }

          if ((CJ != NULL) &&
             ((bmisset(&CJ->IFlags,mjifStalePartition)) ||
             ((MSched.AllocPartitionCleanup != FALSE) && (JobID[0] != '\0'))))
            {
            MDB(2,fCORE) MLog("INFO:     attempting to remove stale partition for completed job %s\n",
              CJ->Name);

            /* verify job 'alloc partition' info still associated with job */

            if (MUHTGet(&CJ->Variables,"ALLOCPARTITION",NULL,NULL) == FAILURE)
              {
              if (MUHTAdd(
                    &CJ->Variables,
                    "ALLOCPARTITION",
                    strdup(ParID),
                    NULL,
                    MUFREE) == FAILURE)
                {
                MDB(2,fCORE) MLog("ALERT:    cannot restore ALLOCPARTITION to completed job %s - value %s\n",
                  CJ->Name,
                  ParID);
                }
              else
                {
                MDB(2,fCORE) MLog("INFO:     ALLOCPARTITION missing from completed job %s - restoring variable with value %s\n",
                  CJ->Name,
                  ParID);
                }
              }      /* END if ((MULLCheck() == FAILURE) || ...) */

            if (MUHTGet(&CJ->Variables,"ALLOCADMINCOOKIE",NULL,NULL) == FAILURE)
              {
              MUHTAdd(&CJ->Variables,"ALLOCADMINCOOKIE",strdup("1"),NULL,MUFREE);
              }  /* END if ((MULLCheck() == FAILURE) || ...) */

            if (MJobDestroyAllocPartition(CJ,R,"stale partition") == SUCCESS)
              {
              MDB(2,fCORE) MLog("INFO:     successfully removed stale partition from completed job %s\n",
                CJ->Name);

              bmunset(&CJ->IFlags,mjifStalePartition);
              }
            }    /* END if ((CJ != NULL) && ...) */
          }      /* END if ((PurgeStalePars == TRUE) && (Orphaned == TRUE)) */

        ptr3 = MUStrTok(NULL,"\n",&TokPtr3);
        }  /* END while (ptr3 != NULL) */
      }    /* END if ((SC == mscNoError) && (tmpBuf[0] != '\0')) */
    else if (PurgeStalePars == TRUE)
      {
      MDB(0,fSCHED) MLog("ERROR:    partition status query failed '%s'\n",
        EMsg);
      }
    }      /* END for (RIndex) */

  return(SUCCESS);
  }        /* END MRMCheckAllocPartition() */






/**
 * Load (but do not process) cluster/workload data from various RM interfaces.
 *
 * @param SR (I) [optional]
 * @param NoBlock (I)
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MRMGetData(

  mrm_t                *SR,      /* I (optional) - if not set, load data for all RM's */
  mbool_t               NoBlock, /* I */
  char                 *EMsg,    /* O (optional,minsize=MMAX_LINE) */
  enum MStatusCodeEnum *SC)      /* O (optional) */

  {
  int rmindex;
  int rc;

  long RMTime;

  mrm_t *R;

  enum MStatusCodeEnum tmpSC;

  mbool_t IsPending;

  char tmpLine[MMAX_LINE];

  char *MPtr;    /* message pointer */

  int FinalReturn = FAILURE;

  if (EMsg != NULL)
    MPtr = EMsg;
  else
    MPtr = tmpLine;

  MPtr[0] = '\0';

  if (SC == NULL)
    SC = &tmpSC;

  *SC = mscNoError;

  MSchedSetActivity(macRMWorkloadLoad,"starting general cluster & workload load");

  for (rmindex = 0;rmindex < MSched.M[mxoRM];rmindex++)
    {
    R = &MRM[rmindex];

    if ((SR != NULL) && (R != SR))
      continue;

    /* for now we only do get data for dest-peers */

    if ((R->Type != mrmtMoab) || bmisset(&R->Flags,mrmfClient))
      continue;

    if (R->State != mrmsActive)
      {
      /* only get data for active RMs */

      continue;
      }

    if (MRMFunc[R->Type].GetData == NULL)
      {
      MDB(3,fRM) MLog("ALERT:    cannot get/query data from RM '%s' (does not support function '%s')\n",
        R->Name,
        MRMFuncType[mrmXGetData]);

      sprintf(MPtr,"cannot get/query data through RM %s - RM doesn't support \"get data\" function",
        R->Name);

      return(FAILURE);
      }

    /* thread off MUThread requests by either creating thread here or in MUThread (NYI) */

    MRMStartFunc(R,NULL,mrmXGetData);

    MUThread(
      (int (*)(void))MRMFunc[R->Type].GetData,
      R->P[0].Timeout,
      &rc,
      NULL,
      R,
      6,
      R,
      NoBlock,
      MSched.Iteration,
      &IsPending,
      MPtr,
      SC);

    MRMEndFunc(R,NULL,mrmXGetData,&RMTime);

    FinalReturn |= rc;

    if (rc != SUCCESS)
      {
      MDB(3,fRM) MLog("ALERT:    cannot get/query data (RM '%s' failed in function '%s')\n",
        R->Name,
        MRMFuncType[mrmXGetData]);

      if (MPtr[0] != '\0')
        MRMSetFailure(R,mrmXGetData,*SC,MPtr);
      else if (rc == mscTimeout)
        MRMSetFailure(R,mrmXGetData,mscSysFailure,"timeout");
      else
        MRMSetFailure(R,mrmXGetData,*SC,"cannot get data");

      /*return(FAILURE);*/
      }  /* END if (rc != SUCCESS) */
    }    /* END for (rmindex) */

  return(FinalReturn);
  }  /* END MRMGetData() */





/**
 * Report is workload/cluster data is loaded for RM.
 * 
 * @param SR (I) [optional] 
 * @param SC (O) [optional]
 */

mbool_t MRMDataIsLoaded(

  mrm_t *SR,                /* I (optional) - if not set, report TRUE only if all RM's are loaded */
  enum MStatusCodeEnum *SC) /* O (optional) */
 
  {
  int rmindex;

  mbool_t DataIsLoaded = TRUE;

  mrm_t *R;

  if (SC != NULL)
    *SC = mscNoError;

  for (rmindex = 0;rmindex < MSched.M[mxoRM];rmindex++)
    {
    R = &MRM[rmindex];

    if ((SR != NULL) && (R != SR))
      continue;

    /* for now only check data for remote dest-peers */

    if ((R->Type != mrmtMoab) || bmisset(&R->Flags,mrmfClient))
      continue;

    if (R->State != mrmsActive)
      {
      /* we must break out of the UI
       * loop if even one RM has not yet successfully initialized */

      return(TRUE);
      }

    if ((R->U.Moab.CData == NULL) ||
        (R->U.Moab.CDataIsProcessed == TRUE) ||
        (R->U.Moab.WData == NULL) ||
        (R->U.Moab.WDataIsProcessed == TRUE))
      {
      DataIsLoaded = FALSE;

      break;
      }
    }  /* END for (rmindex) */

  return(DataIsLoaded);
  }  /* END MRMDataIsLoaded() */
/* 
 * Purge nodes, jobs (completed/active/idle) and reservations associated with the rm 
 *
 * @param R (I)
 */

int MRMPurge(

  mrm_t *R)

  {
  job_iter JTI;
  rsv_iter RTI;

  mnode_t *N;

  mjob_t  *J;

  mrsv_t  *Rsv;

  mpar_t  *P;

  mln_t *JobsRemovedList = NULL;
  mln_t *JobListPtr = NULL;

  int index;

  if (R == NULL)
    {
    return(FAILURE);
    }

  P = &MPar[R->PtIndex];

  /* traverse nodes, jobs, reservations */

  /* remove all jobs whose primary req is within this partition */

  MULLCreate(&JobsRemovedList);

  MJobIterInit(&JTI);

  while (MJobTableIterate(&JTI,&J) == SUCCESS)
    {
    if (J == NULL)
      break;

    if ((J->Req[0] != NULL) && 
        (J->Req[0]->PtIndex > 0) &&
        (J->Req[0]->PtIndex != P->Index))
      {
      continue;
      }

    if (((J->DestinationRM != NULL) && (J->SubmitRM != NULL)) && 
        ((J->DestinationRM != R) || (J->SubmitRM != R)))
      {
      continue;
      }

    /* add job to removed list to be removed after exiting this loop */

    MULLAdd(&JobsRemovedList,J->Name,(void *)J,NULL,NULL);
    }  /* END for (J) */

  /* delete removed jobs */

  while (MULLIterate(JobsRemovedList,&JobListPtr) == SUCCESS)
    MJobRemove((mjob_t **)&JobListPtr->Ptr);

  if (JobsRemovedList != NULL)
    MULLFree(&JobsRemovedList,NULL);

  /* remove all completed jobs that ran in this partition */

  MCJobIterInit(&JTI);

  while (MCJobTableIterate(&JTI,&J) == SUCCESS)
    {
    if ((J->Req[0] != NULL) && (J->Req[0]->PtIndex != P->Index))
      continue;

    /* MJobDestroy assumes that a job with a J->Index of 0 is
     * a template job and in that case will not free J->Req[0].
     * We still need to free that memory for the completed job. */

    MCJobHT.erase(J->Name);

    MJobDestroy(&J);
    }  /* END for (index) */

  /* remove all reservations in this partition */

  MRsvIterInit(&RTI);

  while (MRsvTableIterate(&RTI,&Rsv) == SUCCESS)
    {
    if (P->Index != Rsv->PtIndex)
      continue;

    MRsvDestroy(&Rsv,TRUE,TRUE);
    }  /* END while (MRsvTableIterate()) */

  /* remove all nodes in this partition */

  for (index = 0;index < MSched.M[mxoNode];index++)
    {
    N = MNode[index];

    if ((N == NULL) || (N->Name[0] == '\0'))
      break;

    if (N->Name[0] == '\1')
      continue;

    if (N == MSched.GN)
      {
      /* global node displayed later */

      continue;
      }

    if (P->Index != N->PtIndex)
      continue;

    MNodeDestroy(&N);
    }   /* END for (index) */

  R->State = mrmsDisabled;

  return(SUCCESS);
  }  /* END MRMPurge() */




/*
 * Check whether the RM is real.
 *
 * @param R
 */ 

mbool_t MRMIsReal(

  mrm_t *R)

  {
  if ((R == NULL) || (R->Name[0] == '\0'))
    {
    return(FALSE);
    }

  if (R->Type == mrmtNONE)
    {
    return(FALSE);
    }

  if (R->Name[0] == '\1')
    {
    return(FALSE);
    }

  if (bmisset(&R->Flags,mrmfTemplate))
    {
    return(FALSE);
    }

  return(TRUE);
  }  /* END MRMIsReal() */


/**
 * Set BM for all functions.
 *
 */

int MRMFuncSetAll(

  mrm_t *R)

  {
  int index;

  for (index = 0;index != mrmLAST;index++)
    {
    bmset(&R->FnList,index);
    }

  return(SUCCESS);
  }  /* END MRMFuncSetAll() */




/*
 * Determines if RM has reliable state (and power state) information.
 * This is motivated by hybrid configurations where a cluster updates
 * from a provisioning, MSM RM report that they don't know the state 
 * and the power state. 
 * 
 * @param R - RM in question
 */

mbool_t MRMIgnoreNodeStateInfo(

  const mrm_t *R)
  
  {
  if (bmisset(&R->RTypes,mrmrtProvisioning))
    {
    return(TRUE);
    }

  return(FALSE);
  }



/**
 * Updates a sharedmem job's ReqMem requirment and removes any partitions
 * from the job's PAL that aren't sharedmem.
 *
 * @param J (I)
 */

int MRMUpdateSharedMemJob(
    
  mjob_t *J)

  {
  mpar_t *P;

  int tindex;

  MASSERT(J != NULL,"job is null when updating shared memory job.");

  /* set RQ->ReqMem instead of RQ->DRes.Mem */

  for (tindex = 0;tindex < MMAX_REQ_PER_JOB;tindex++)
    {
    if (J->Req[tindex] == NULL)
      break;

    if (J->Req[tindex]->DRes.Mem > 0)
      {
      /* DRes.Mem has been divied by the number of tasks, original value
       * needs to be restored. Also don't update ReqMem if it was already
       * set from an update from the RM. The RM doesn't do a divide and 
       * multiply and lose precision. */
      if (J->Req[tindex]->ReqMem == 0)
        J->Req[tindex]->ReqMem = J->Req[tindex]->DRes.Mem * J->Req[tindex]->TaskCount;

      J->Req[tindex]->DRes.Mem = 0;
      }
    }  /* END for (tindex) */

  /* remove any non-SharedMem partitions from job's PAL */

  for (tindex = 1;tindex < MSched.M[mxoPar];tindex++)
    {
    P = &MPar[tindex];

    if (P == NULL)
      break;

    if (P->Name[0] == '\1')
      continue;

    if (P->Name[0] == '\0')
      break;

    if (bmisset(&J->PAL,P->Index) == FALSE)
      continue;

    if (bmisset(&P->Flags,mpfSharedMem))
      continue;

    MDB(3,fRM) MLog("INFO:     removing partition '%s' from job '%s' PAL (sharedmem)\n",
      P->Name,
      J->Name);

    bmunset(&J->PAL,P->Index);
    }  /* END for (tindex) */

  return(SUCCESS);
  }  /* END MRMUpdateSharedMemJob */


 
/* END MRM.c */

/* vim: set ts=2 sw=2 expandtab nocindent filetype=c: */
