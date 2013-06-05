/* HEADER */

/**
 * @file MClient.c
 *
 * Moab Client
 */

/* Contains:                   *
 *  int MCSubmitProcessArgs()  *
 *                             */


/* NOTE:  
 *
 * int MCCredCtlCreateRequest() - located in server/mclient.c
 *
 */

#include "moab.h"
#include "moab-proto.h"
#include "cmoab.h"
#include "mapi.h"


/* Constants and Globals - cannot use moab-globals.h due to use of cmoab.h */

extern const char *MParam[];
extern const char *MJobState[];
extern const char *MCKeyword[];
extern const char *MClientAttr[];
extern const char *MSockProtocol[];
extern const char *MCDisplayType[];
extern const char *MSchedAttr[];
extern const char *MSchedMode[];
extern const char *MBalCmds[];
extern const char *MClusterAttr[];
extern const char *MXO[];
extern const char *MXOC[];
extern const char *MRMType[];
extern const char *MJobCtlCmds[];
extern const char *MNodeCtlCmds[];
extern const char *MBool[];
extern const char *MJobAttr[];
extern const char *MJobLLAttr[];
extern const char *MRMXAttr[];
extern const char *MDependType[];
extern const char *MObjOpType[];
extern const char *MRMResList[];
extern const char *MJobFlags[];

extern char       *MSON[];
extern char       *MSAN[];

extern msched_t    MSched;
extern mlog_t      mlog;
extern muis_t      MUI[];
extern mcfg_t      MCfg[];
extern m64_t       M64;

extern enum MRMTypeEnum MDefRMLanguage;


/* Forward reference */

int __MCMakeArgv(int *,char **,char *);
int __MCCJobListAllocRes(void *,char *,char **,char *);
int __MCCInitialize(char *,int,mccfg_t **);


/* globals */

#define          MAX_MBALEXECLIST 32

enum MBalEnum    MBalMode;
char            *MBalExecList[MAX_MBALEXECLIST + 2];
mccfg_t         *MClientC = NULL;

char            UserDirective[MMAX_LINE];
mbool_t         UserDirectiveSpecified = FALSE;

#ifndef MMAX_S3VERS
#define MMAX_S3VERS 4
#endif /* MMAX_S3VERS */

#ifndef MMAX_S3ATTR
#define MMAX_S3ATTR 256
#endif /* MMAX_S3ATTR */

#define MCSERVERENV "MOABSERVER"

#define MMAX_ARGVLEN 128

#ifndef MCCSUCCESS
#define MCCSUCCESS 0 /* sync w/ MCSUCCESS in mapi.h */
#endif /* MCCSUCCESS */

#ifndef MCCFAILURE
#define MCCFAILURE -1 /* sync w/ MCFAILURE in mapi.h */
#endif /* MCCFAILURE */

mccfg_t C;

extern char *MS3JobAttr[];
extern char *MS3ReqAttr[];



/* From PBS ERS
Resource   Explanation
--------------------------------------------------------------------------------
arch       Specifies the administrator defined system architecture requried. This
           defaults to whatever the PBS_MACH string is set to in "local.mk". Units: string.
cput       Maximum amount of CPU time used by all processes in the job. Units: time.
file       The largest size of any single file that may be created by the job. Units: size.
host       ---NYI--- Name of host on which job should be run. This resource is provided
           for use by the site s scheduling policy. The allowable values and effect
           on job placement is site dependent. Units: string.
mem        Maximum amount of physical memory (workingset) used by the job. Units: size.
nice       ---NYI--- The nice value under which the job is to be run. Units: unitary.
nodes      Number and/or type of nodes to be reserved for exclusive use by the job.
           The value is one or more node_specs joined with the  +  character,
           "node_spec[+node_spec...]. Each node_spec is an number of nodes required
           of the type declared in the node_spec and a name or one or more properity
           or properities desired for the nodes. The number, the name, and each properity
           in the node_spec are separated by a colon  : . If no number is specified,
           one (1) is assumed. Units: string. The name of a node is its hostname.
           The properities of nodes are: ppn=# specifying the number of processors
           er node requested. Defaults to 1. . arbitrary string assigned by the
           system administrator, please check with your administrator as to the node
           names and properities available to you.
pcput      ---NYI--- Maximum amount of CPU time used by any single process in the job. Units: time.
pmem       ---NYI--- Maximum amount of physical memory (workingset) used by any single
           process of the job. Units: size.
pvmem      ---NYI--- Maximum amount of virtual memory used by any single process in the job. Units: size.
vmem       Maximum amount of virtual memory used by all concurrent processes in the job. Units: size.
walltime   Maximum amount of real time during which the job can be in the running state. Units: time.
software   name and number of consumable generic resources (can be floating or node locked) required by job.

*/



/* END globals */

/* prototypes */

/* END prototypes */





/**
 * Process general client args (ex. about, flags, format, help).
 * 
 * NOTE:  only processes '--' command line args 
 *
 * @param CIndex (I)
 * @param ArgV (I) [modified]
 * @param ArgC (I) [modified]
 * @param E (O)
 */

int MCProcessGeneralArgs(

  enum MSvcEnum   CIndex,  /* I */
  char          **ArgV,    /* I (modified) */
  int            *ArgC,    /* I (modified) */
  mxml_t         *E)       /* O */

  {
  const char *OptString = "-:";

  char *OptArg;
  int   OptTok;

  char *ptr;
  char *TokPtr=NULL;

  char *Value = NULL;

  int Flag;

  mbool_t ShowVersion = FALSE;

  if ((ArgV == NULL) || (E == NULL))
    {
    return(FAILURE);
    }

  /* process '-v' verbose */

  /* extract global flags */

  OptTok = 1;

  while ((Flag = MUGetOpt(ArgC,ArgV,OptString,&OptArg,&OptTok,NULL)) != -1)
    {
    MDB(6,fCONFIG) MLog("INFO:     processing flag '%c'\n",
      (char)Flag);

    switch ((char)Flag)
      {
      case '-':

        MDB(6,fCONFIG) MLog("INFO:     received '-' (%s)\n",
          OptArg);

        if ((ptr = MUStrTok(OptArg,"=",&TokPtr)) != NULL)
          {
          Value = MUStrTok(NULL,"=",&TokPtr);
          }

        if (ptr == NULL)
          {
          if (C.ShowUsage != NULL)
            (*C.ShowUsage)(CIndex,NULL);

          exit(1);
          }

        if (!strcmp(ptr,"about"))
          {
          char *hptr;

          fprintf(stderr,"Version: %s client %s (revision %s, changeset %s)\n",
            MSCHED_SNAME,
            MOAB_VERSION,
#ifdef MOAB_REVISION
            MOAB_REVISION,
#else
            "NA",
#endif /* MOAB_REVISION */
#ifdef MOAB_CHANGESET
            MOAB_CHANGESET);
#else
            "NA");
#endif /* MOAB_CHANGESET */

          /* NOTE:  specify:  default configfile, serverhost, serverport */

          hptr = getenv(MSCHED_ENVHOMEVAR);

          fprintf(stderr,"Defaults:  server=%s:%d  cfgdir=%s%s  vardir=%s\n",
            C.ServerHost,
            C.ServerPort,
            (hptr != NULL) ? hptr : C.CfgDir,
            (hptr != NULL) ? " (env)" : "",
            C.VarDir);

          /* NOTE:  specify build location, build date, build host       */

          fprintf(stderr,"Build dir:  %s\nbuild host: %s\n",
            C.BuildDir,
            C.BuildHost);

#ifdef MBUILD_DATE
          fprintf(stderr,"Build date: %s\n",
            MBUILD_DATE);
#endif /* MBUILD_DATE */

          fprintf(stderr,"Build args: %s\n",
            C.BuildArgs);

          exit(0);
          }  /* END if (!strcmp(ptr,"about")) */
        else if (!strcmp(OptArg,"configfile"))
          {
          if (Value == NULL)
            {
            if (C.ShowUsage != NULL)
              (*C.ShowUsage)(CIndex,"configfile not specified");

            exit(1);
            }

          MUStrCpy(C.ConfigFile,OptArg,sizeof(C.ConfigFile));

          MDB(2,fCONFIG) MLog("INFO:     configfile set to '%s'\n",
            C.ConfigFile);
          }
        else if (!strcmp(ptr,"flags"))
          {
          MUStrAppend(&C.CmdFlags,NULL,Value,',');

          if ((CIndex == mcsMSchedCtl) &&
              (Value != NULL) &&
              (strstr(Value,"status")))
            {
            /* 'schedctl --flags=status' (scheduler ping) should always report XML
               and should always be non-blocking */

            C.Format = mfmXML;
            C.IsNonBlocking = TRUE;
            }
          }
        else if (!strcmp(OptArg,"format"))
          {
          /* NOTE:  currently only support XML */

          C.Format = mfmXML;
          }
        else if (!strcmp(OptArg,"forward"))
          {
          if ((Value == NULL) || (Value[0] == '\0'))
            {
            if (C.ShowUsage != NULL)
              (*C.ShowUsage)(CIndex,"forwarding destination not specified");

            exit(1);
            }

          MUStrDup(&C.Forward,Value);
          }
        else if (!strcmp(OptArg,"help"))
          {
          if (C.ShowUsage != NULL)
            (*C.ShowUsage)(CIndex,NULL);

          exit(1);
          }
        else if ((!strcmp(OptArg,"host")) && (Value != NULL))
          {
          MUStrCpy(C.ServerHost,Value,sizeof(C.ServerHost));

          MDB(3,fCONFIG) MLog("INFO:     server host set to %s\n",
            C.ServerHost);
          }
        else if (!strcmp(OptArg,"keyfile"))
          {
          int rc;

          FILE *fp;

          char tmpKey[MMAX_LINE + 1];

          if (Value == NULL)
            {
            if (C.ShowUsage != NULL)
              (*C.ShowUsage)(CIndex,"keyfile not specified");

            exit(1);
            }

          if ((fp = fopen(Value,"r")) == NULL)
            {
            fprintf(stderr,"ERROR:    cannot open keyfile '%s' errno: %d, (%s)\n",
              Value,
              errno,
              strerror(errno));

            exit(1);
            }

          rc = fscanf(fp,"%1025s",  /* sync w/sizeof tmpKey */
            tmpKey);

          strncpy(C.ServerCSKey,tmpKey,sizeof(C.ServerCSKey));

          fclose(fp);

          MDB(10,fALL) MLog("INFO:     fscanf returned %d\n",rc);
          }  /* END else if (!strcmp(OptArg,"keyfile")) */
        else if (!strcmp(OptArg,"loglevel"))
          {
          if (Value == NULL)
            {
            if (C.ShowUsage != NULL)
              (*C.ShowUsage)(CIndex,"loglevel not specified");

            exit(1);
            }

          mlog.Threshold = (int)strtol(Value,NULL,10);

          MDB(2,fCONFIG) MLog("INFO:     LOGLEVEL set to %d\n",
            mlog.Threshold);
          }
        else if (!strcmp(OptArg,"msg"))
          {
          if (Value == NULL)
            {
            if (C.ShowUsage != NULL)
              (*C.ShowUsage)(CIndex,"msg not specified");

            exit(1);
            }

          MUStrCpy(C.Msg,Value,sizeof(C.Msg));
          }
        else if (!strcasecmp(OptArg,"noblock"))
          {
          C.IsNonBlocking = TRUE;
          }
        else if (!strcasecmp(ptr,"blocking"))
          {
          C.IsNonBlocking = FALSE;
          }
        else if (!strcmp(OptArg,"orange"))
          {
          if (Value == NULL)
            {
            if (C.ShowUsage != NULL)
              (*C.ShowUsage)(CIndex,"invalid object range specified");

            exit(1);
            }

          MUStrDup(&C.ORange,Value);
          }
        else if (!strcmp(OptArg,"port"))
          {
          if (Value == NULL)
            {
            if (C.ShowUsage != NULL)
              (*C.ShowUsage)(CIndex,"port not specified");

            exit(1);
            }

          C.ServerPort = (int)strtol(Value,NULL,10);

          MDB(2,fCONFIG) MLog("INFO:     port set to %d\n",
            C.ServerPort);
          }
        else if (!strcmp(ptr,"proxy"))
          {
          if (Value == NULL)
            {
            if (C.ShowUsage != NULL)
              (*C.ShowUsage)(CIndex,NULL);

            exit(1);
            }

          MUStrCpy(C.Proxy,Value,sizeof(C.Proxy));

          MDB(2,fCONFIG) MLog("INFO:     proxy set to %s\n",
            C.Proxy);
          }
        else if (!strcasecmp(OptArg,"rmflags") ||
                 !strcasecmp(OptArg,"slurm"))
          {
          char tmpBuf[MMAX_BUFFER];

          char *tBPtr;
          int   tBSpace;

          int   aindex;

          MUSNInit(&tBPtr,&tBSpace,tmpBuf,sizeof(tmpBuf));

          /* move all remaining flags into rmstring */

          for (aindex = OptTok;aindex < *ArgC;aindex++)
            {
            MUSNPrintF(&tBPtr,&tBSpace," %s",
              ArgV[aindex]);

            /* clear, but do not free RMFlag arg */

            ArgV[aindex][0] = '\0';
            }  /* END for (aindex) */

          /* If we had to clear any RMFlags then decrement
             the argument count for the number of arguments we
             cleared. */

          *ArgC -= (aindex - OptTok);

          if (tmpBuf[0] != '\0')
            MUStrDup(&C.PeerFlags,tmpBuf);

          return(SUCCESS);
          }    /* END else if (!strcasecmp(OptArg,"rmflags") || ...) */
        else if (!strcmp(OptArg,"test"))
          {
          bmset(&C.DisplayFlags,mdspfTest);
          }
        else if (!strcmp(OptArg,"timeout"))
          {
          C.Timeout = MIN(MUTimeFromString(Value),3000) * MDEF_USPERSECOND;
          }
        else if (!strcmp(OptArg,"version"))
          {
          /* We use ShowVersion instead of just printing the version here because
              there may be a --xml flag coming later */

          ShowVersion = TRUE;
          }
        else if (!strcmp(OptArg,"with-group"))
          {
          MUStrDup(&C.SpecGroup,Value);
          }
        else if (!strcmp(OptArg,"with-jobid"))
          {
          MUStrDup(&C.SpecJobID,Value);
          }
        else if (!strcmp(OptArg,"with-user"))
          {
          MUStrDup(&C.SpecUser,Value);

          /* Set the default group when with-group hasn't been specified by user. */

          if (C.SpecGroup == NULL)
            {
            char GName[MMAX_NAME];

            if (MUGNameFromUName(C.SpecUser,GName) == SUCCESS)
              MUStrDup(&C.SpecGroup,GName);
            }
          }
        else if (!strcmp(OptArg,"xml"))
          {
          C.Format = mfmXML;
          }
        else
          {
          char tmpLine[MMAX_LINE];

          char *BPtr;
          int   BSpace;

          MUSNInit(&BPtr,&BSpace,tmpLine,sizeof(tmpLine));

          MUSNPrintF(&BPtr,&BSpace,"Unknown flag: '%s'",
            OptArg);
          
          if (C.ShowUsage != NULL)
            (*C.ShowUsage)(CIndex,tmpLine);

          exit(1);
          }

        break;
      }  /* END switch ((char)Flag) */
    }    /* END while ((Flag = MUGetOpt()) != -1) */

  if (ShowVersion == TRUE)
    {
    if (C.Format == mfmXML)
      {
      mstring_t Buffer;
  
      MXMLSetAttr(E,(char *)MSchedAttr[msaSchedulerName],(void **)MSCHED_SNAME,mdfString);

      MXMLSetAttr(E,(char *)MSchedAttr[msaVersion],(void **)MOAB_VERSION,mdfString);

#ifdef MOAB_REVISION
      MXMLSetAttr(E,(char *)MSchedAttr[msaRevision],(void **)MOAB_REVISION,mdfString);
#else
      MXMLSetAttr(E,(char *)MSchedAttr[msaRevision],"NA",mdfString;
#endif /* MOAB_REVISION */

#ifdef MOAB_CHANGESET
      MXMLSetAttr(E,(char *)MSchedAttr[msaChangeset],(void **)MOAB_CHANGESET,mdfString);
#else
      MXMLSetAttr(E,(char *)MSchedAttr[msaChangeset],"NA",mdfString);
#endif /* MOAB_CHANGESET */

      if (MXMLToMString(
            E,
            &Buffer,
            NULL,
            TRUE) == SUCCESS)
        {
        fprintf(stdout,"\n\n%s\n",
          Buffer.c_str());
        }
      }
    else
      {
      fprintf(stderr,"Version: %s client %s (revision %s, changeset %s)\n",
        MSCHED_SNAME,
        MOAB_VERSION,
#ifdef MOAB_REVISION
        MOAB_REVISION,
#else
        "NA",
#endif /* MOAB_REVISION */
#ifdef MOAB_CHANGESET
        MOAB_CHANGESET);
#else
        "NA");
#endif /* MOAB_CHANGESET */

      }

    exit(0);
    }  /* END if (ShowVersion == TRUE) */

  return(SUCCESS);
  }  /* END MCProcessGeneralArgs() */





/**
 * Load client-specific information from config file.
 *
 * @see MClientProcessConfig() - parent
 *
 * @param C (I) [modified]
 * @param ConfigBuf (I) [optional]
 */

int MClientLoadConfig(

  mccfg_t *C,          /* I (modified) */
  char    *ConfigBuf)  /* I (optional) */

  {
  char *ptr = NULL;

  char  tmpLine[MMAX_LINE];

  while (MCfgGetSVal(
          (ConfigBuf != NULL) ? ConfigBuf : MSched.ConfigBuffer,
          &ptr,
          MParam[mcoClientCfg],
          NULL,
          NULL,
          tmpLine,
          sizeof(tmpLine),
          0,
          NULL) == SUCCESS)
    {
    MClientProcessConfig(C,tmpLine,TRUE);
    }  /* END while MCfgGetSVal() == SUCCESS) */

  return(SUCCESS);
  }  /* END MClientLoadConfig() */





/**
 * Process client-specific information from config file.
 *
 * @see MClientLoadConfig()
 *
 * @param C (I) [optional,modified]
 * @param Value (I)
 * @param IsSecure (I)
 */

int MClientProcessConfig(

  mccfg_t *C,        /* I (optional,modified) */
  char    *Value,    /* I */
  mbool_t  IsSecure) /* I */

  {
  char *ptr;
  char *TokPtr;

  int   aindex;

  char  ValLine[MMAX_LINE];

  char  AttrArray[MMAX_NAME];

  if (Value == NULL)
    {
    return(FAILURE);
    }

  ptr = MUStrTokEPlus(Value," \t\n",&TokPtr);

  while (ptr != NULL)
    {
    /* parse name-value pairs */

    /* FORMAT:  <ATTR>=<VALUE>[,<VALUE>] */

    if (MUGetPair(
          ptr,
          (const char **)MClientAttr,
          &aindex,
          AttrArray,
          TRUE,
          NULL,
          ValLine,
          MMAX_NAME) == FAILURE)
      {
      /* cannot parse value pair */

      ptr = MUStrTokEPlus(NULL," \t\n",&TokPtr);

      continue;
      }

    switch (aindex)
      {
      case mcltaAuthCmd:

        /* If the authcmd in our moab.cfg file has the string "munge" in it,
           then we assume that the Authorization algorithm is munge.

           Note that the authcmd must be "unmunge" when we process a munge
           credential. We make this change in MSecGetChecksum when we do
           an unmunge (based on the munge algo type passed in). */

        if (MUStrStr(ValLine,"munge",0,TRUE,TRUE) != NULL)
          {
          MSched.DefaultCSAlgo = mcsaMunge; /* This also handles the unmunge condition */
                                            /* We only use mcsaUnMunge as an input to MSecGetChecksum */ 
          }

        MUStrDup(&MSched.AuthCmd,ValLine);

        break;

      case mcltaAuthCmdOptions:

        if (MSched.AuthCmdOptions == NULL)
          {
          MUStrDup(&MSched.AuthCmdOptions,ValLine);
          }

        break;

      case mcltaClockSkew:

        MSched.ClockSkew = MUTimeFromString(ValLine);

        break;

      case mcltaCSAlgo:
                                                                                
        /* NYI */
                                                                                
        break;

      case mcltaDefaultSubmitPartition:

        if (C != NULL)
          MUStrDup(&C->DefaultSubmitPartition,ValLine);

        break;
    
      case mcltaEncryption:
      
        if (C != NULL)                                                                          
          C->DoEncryption = MUBoolFromString(ValLine,FALSE);
                                                                                
        break;

      case mcltaFlags:

        if (C != NULL)
          {
          if (MUStrStr(ValLine,"allowunknownresource",0,TRUE,FALSE) != NULL)
            C->AllowUnknownResource = TRUE;
          }

        break;

      case mcltaLocalDataStageHead:

        if (C != NULL)
          MUStrDup(&C->LocalDataStageHead,ValLine);

        break;

      case mcltaTimeout:

        if (MSched.IsServer == TRUE)
          {
          /* seconds */

          MSched.ClientTimeout = MIN(MUTimeFromString(ValLine),1000);
          }
        else if (C != NULL) 
          {
          /* microseconds */

          C->Timeout = MUTimeFromString(ValLine) * MDEF_USPERSECOND;
                                                                                
          if (C->Timeout <= 0)
            {
            C->Timeout = MDEF_CLIENTTIMEOUT;
            }
          }

        break;

      default:

        /* attribute not supported */

        return(FAILURE);

        /*NOTREACHED*/

        break;
      }  /* END switch (aindex) */

    ptr = MUStrTokEPlus(NULL," \t\n",&TokPtr);
    }    /* END while (ptr != NULL) */

  return(FAILURE);
  }  /* END MClientProcessConfig() */





/**
 * Send request to server and receive the response.
 *
 * NOTE: MCSendRequest() should not write to the log, but should populate EMsg
 *         (This routine will be run by Moab clients and non-XML error logging 
 *          cause parsing issues)
 *
 * @see MCDoCommand() - parent
 *
 * @param S (I)
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 */

int MCSendRequest(

  msocket_t *S,    /* I */
  char      *EMsg) /* O (optional,minsize=MMAX_LINE) */

  {
  mbool_t DoAuthenticate;

  const char *FName = "MCSendRequest";

  MDB(2,fUI) MLog("%s(%s,EMsg)\n",
    FName,
    (S != NULL) ? "S" : "NULL");

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (S == NULL)
    {
    return(FAILURE);
    }

  if (S->SBufSize == 0)
    S->SBufSize = (long)strlen(S->SBuffer);

  if (MSUSendData(S,S->Timeout,TRUE,FALSE,NULL,EMsg) == FAILURE)
    {
    MDB(1,fSOCK) MLog("ERROR:    cannot send request to %s:%d (%s may not be running)\n",
      S->RemoteHost,
      S->RemotePort,
      MSCHED_SNAME);

    MSUDisconnect(S);

    return(FAILURE);
    }

  MDB(1,fUI) MLog("INFO:     message sent to server\n");

  MDB(3,fUI) MLog("INFO:     message sent: '%s'\n",
    (S->SBuffer != NULL) ? S->SBuffer : "NULL");

  if (S->WireProtocol == mwpNONE)
    {
    /* NOTE:  socket is persistent - no need to validate response */

    DoAuthenticate = FALSE;
    }
  else
    {
    DoAuthenticate = TRUE;
    }

  if (MSURecvData(S,S->Timeout,DoAuthenticate,NULL,EMsg) == FAILURE)
    {
    if (MSched.IsServer == FALSE)
      {
      /* routine is running within Moab client command (ie, showq) */

      if (S->StatusCode == (long)msfESecServerAuth)
        {
        if (S->CSAlgo == mcsaRemote)
          {
          fprintf(stderr,"ERROR:    server rejected request - could not authenticate client using .moab.key\n");
          }
        else if (S->CSAlgo == mcsaMunge)
          {
          fprintf(stderr,"ERROR:    server rejected request - %s\n",
            EMsg);
          }
        else
          {
          fprintf(stderr,"ERROR:    server rejected request - could not authenticate client\n");
          }
        }    /* END if (S->StatusCode == (long)msfESecServerAuth) */
      else
        {
        if ((EMsg != NULL) && (EMsg[0] == '\0'))
          {
          sprintf(EMsg,"lost connection to server");
          }
        }
      }      /* END if (MSched.IsServer == FALSE) */

    return(FAILURE);
    }        /* END if (MSURecvData() == FAILURE) */

  MDB(3,fUI) MLog("INFO:     message received\n");

  MDB(4,fUI) MLog("INFO:     received message '%s' from server\n",
    S->RBuffer);

  return(SUCCESS);
  }  /* END MCSendRequest() */





/**
 *
 *
 * @param ServerHost (I) [optional]
 * @param ServerPort (I) [optional,<=0 for not set]
 * @param CP (O) [alloc]
 */

int __MCCInitialize(

  char     *ServerHost,  /* I (optional) */
  int       ServerPort,  /* I (optional,<=0 for not set) */
  mccfg_t **CP)          /* O (alloc) */

  {
  mccfg_t *C;

  int rc;

  if (CP == NULL)
    {
    return(-1);
    }

  if (*CP == NULL)
    {
    if ((*CP = (mccfg_t *)MUCalloc(1,sizeof(mccfg_t))) == NULL)
      {
      return(-1);
      }
    }

  C = *CP;

  rc = MCInitialize(
    C,
    &MSched,
    ServerHost,
    ServerPort,
    (ServerHost == NULL) ? TRUE : FALSE,
    (ServerHost == NULL) ? TRUE : FALSE);

  if (rc == FAILURE)
    {
    return(-1);
    }

  C->IsInitialized = TRUE;
 
  return(0);
  }  /* END __MCCInitialize() */


/**
 * Determines whether to use an authentication service or not.
 *
 * In order to use an authentication server like mauth or munge,
 * .moab.key must be specified in the moab home dir or by an environment
 * variable.
 *
 * @param C (Input) Ptr to the client object
 * @param S (Input) Ptr to the schedule object
 */
void MCCheckAuthService(

  mccfg_t  *C,             /* I */
  msched_t *S)             /* I (optional) */

  {
  char EMsg[MMAX_LINE];
  mbool_t UseAuthFile;

  if (S == NULL)
    {
    return;
    }

  S->UID = MOSGetEUID();

  /* Check to see if auth key file exists as specified in environment or in default location */

  MUCheckAuthFile(
    S,
    S->DefaultCSKey,
    NULL,
    &UseAuthFile,
    EMsg,   /* O */
    FALSE);

  /* If key file exists determine from scheduler config which auth service to use */

  if (UseAuthFile == TRUE)
    {
    S->DefaultCSAlgo = mcsaRemote;
    }
  else
    {
#ifdef __MREQKEYFILE
    fprintf(stderr,"\nERROR:  cannot locate/use keyfile '.moab.key' (%s)\n",EMsg);

    exit(1);
#else /* __MREQKEYFILE */
    S->DefaultCSAlgo = MDEF_CSALGO;
#endif /* __MREQKEYFILE */
    }  /* END else (UseAuthFile == TRUE) */

  return;
  }    /* END MCCheckAuthService */





/**
 * Initialize and establish connection with the server.
 *
 * If ServerHost is not supplied, localhost will be used.
 * If ServerPort is not supplied, the standard Moab port
 * will be used.
 * 
 * @see __MCInitialize() - parent
 *
 * @param C (I)
 * @param S (I) [optional]
 * @param ServerHost (I) [optional]
 * @param ServerPort (I) <= 0 means value is not set [optional]
 * @param DoLoadEnv (I)
 * @param DoLoadConfig (I)
 */

int MCInitialize(

  mccfg_t  *C,             /* I */
  msched_t *S,             /* I (optional) */
  char     *ServerHost,    /* I (optional) */
  int       ServerPort,    /* I (optional, <=0 is not set) */
  mbool_t   DoLoadEnv,     /* I */
  mbool_t   DoLoadConfig)  /* I */

  {
  char *ptr;
  char *buf;

  int   count;
  int   SC;

  char tmpLine[MMAX_LINE];

  const char *FName = "MCInitialize";

  MDB(2,fALL) MLog("%s(C,S,%s,%d,%s,%s)\n",
    FName,
    (ServerHost != NULL) ? ServerHost : "NULL",
    ServerPort,
    MBool[DoLoadEnv],
    MBool[DoLoadConfig]);

  MUBuildPList(MCfg,MParam);

  M64Init(&M64);

  if ((ServerHost != NULL) && (ServerHost[0] != '\0'))
    MUStrCpy(C->ServerHost,ServerHost,sizeof(C->ServerHost));
  else
    strcpy(C->ServerHost,MDEF_SERVERHOST);

  if (ServerPort > 0)
    C->ServerPort = ServerPort;
  else
    C->ServerPort = MDEF_SERVERPORT;

  MSched.ClockSkew = MMAX_CLIENTSKEW;

  MCGetEtcDir(C->CfgDir,sizeof(C->CfgDir));

  MCGetVarDir(C->VarDir,sizeof(C->VarDir));

  if (C->ConfigFile[0] == '\0')
    {
    sprintf(C->ConfigFile,"%s%s",
      MSCHED_SNAME,
      MDEF_CONFIGSUFFIX);
    }

  MCGetBuildDir(C->BuildDir,sizeof(C->BuildDir));

  MCGetBuildHost(C->BuildHost,sizeof(C->BuildHost));

  MCGetBuildDate(C->BuildDate,sizeof(C->BuildDate));

  MCGetBuildArgs(C->BuildArgs,sizeof(C->BuildArgs));

  MFUCacheInitialize(&MSched.Time);

  if ((ptr = getenv("MOABLOGLEVEL")) != NULL)
    {
    mlog.Threshold = (int)strtol(ptr,NULL,10);
    }

  if ((ptr = getenv("MOABLOGFILE")) != NULL)
    {
    MLogInitialize(ptr,-1,0);
    }

  /* load environment required for configfile 'bootstrap' */

  if ((ptr = getenv(MSCHED_ENVHOMEVAR)) != NULL)
    {
    MDB(4,fCONFIG) MLog("INFO:     using %s environment variable (value: %s)\n",
      MSCHED_ENVHOMEVAR,
      ptr);

    MUStrCpy(C->CfgDir,ptr,sizeof(C->CfgDir));
    }
  else if ((ptr = getenv(MParam[mcoMServerHomeDir])) != NULL)
    {
    MDB(4,fCONFIG) MLog("INFO:     using %s environment variable (value: %s)\n",
      MParam[mcoMServerHomeDir],
      ptr);

    MUStrCpy(C->CfgDir,ptr,sizeof(C->CfgDir));
    }
  else
    {
    /* check master configfile */

    if ((buf = MFULoad(MCONST_MASTERCONFIGFILE,1,macmRead,&count,NULL,&SC)) != NULL)
      {
      /* Master can either point to config file or can be config file */

      if ((ptr = strstr(buf,MParam[mcoMServerHomeDir])) != NULL)
        {
        MUSScanF(ptr,"%x%s %x%s",
          sizeof(tmpLine),
          tmpLine,
          sizeof(C->CfgDir),
          C->CfgDir);
        }
      }
    }    /* END else ((ptr = getenv()) != NULL) */

  if ((ptr = getenv(MParam[mcoSchedConfigFile])) != NULL)
    {
    MDB(4,fCONFIG) MLog("INFO:     using %s environment variable (value: %s)\n",
      MParam[mcoSchedConfigFile],
      ptr);

    MUStrCpy(C->ConfigFile,ptr,sizeof(C->ConfigFile));
    }

  if (S != NULL)
    {
    char **PathList;

    S->IsClient = TRUE;

    /* See if the PathList needs to be initialized */
    PathList = MUGetPathList();

    /* If the PathList is NOT already initialized, then initialize it */
    if ((PathList == NULL) || (PathList[0] == NULL))
      {
      char *pathBuff;

      strcpy(S->SpoolDir,"/tmp");

      S->UID = getuid();
      S->GID = getgid();

      /* Initialize the PathList with current env PATH */
      MUInitPathList(getenv("PATH"));

      /* Get a buffer to build a path string item and then add that item to the pathlist */
      pathBuff = (char *) MUCalloc(1,sizeof(MBUILD_INSTDIR) + sizeof("/bin") + 10);
      strcpy(pathBuff,MBUILD_INSTDIR);
      strcat(pathBuff,"/bin");
      MUAddToPathList(pathBuff);
      MUFree(&pathBuff);

      if (S->VarDir[0] == '\0')
        {
        ptr = getenv(MSCHED_ENVHOMEVAR);

        if (ptr != NULL)
          MUStrCpy(S->VarDir,ptr,sizeof(S->VarDir));

#ifdef MBUILD_VARDIR
        if (S->VarDir[0] == '\0')
          MUStrCpy(S->VarDir,MBUILD_VARDIR,sizeof(S->VarDir));
#endif /* MBUILD_VARDIR */
        }
      } /* END if ((PathList == NULL) || (PathList[0] == NULL)) */

    S->JobMaxNodeCount = MDEF_JOBMAXNODECOUNT;
    }  /* END if (S != NULL) */

  if (DoLoadEnv == TRUE)
    {
    MCLoadEnvironment(
      C->PName,
      C->ServerHost,
      &C->ServerPort);
    }

  if (DoLoadConfig == TRUE)
    {
    if (MCLoadConfig(C,C->CfgDir,C->ConfigFile) == FAILURE)
      {
      return(FAILURE);
      }
    }

  /* For mauth the .moab.key file must exist.
   * We use the .moab.key file for munge, but it is not a requirement that the file exist. */

  if (S->DefaultCSAlgo != mcsaMunge) 
    {
    MCCheckAuthService(C,S); 
    }

  C->IsInitialized = TRUE;

  return(SUCCESS);
  }  /* END MCInitialize() */






/**
 * Execute requested command on the server and gather response.
 *
 * @see MCSendRequest() - child
 * @see mclient.c:main() - parent
 *
 * @param CIndex (I)
 * @param C (I)
 * @param CmdBuf (I) encoded according to wire protocol
 * @param RspBuf (O) [optional,ptr]
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SS (O) [modified,optional]
 */

int MCDoCommand(

  enum MSvcEnum   CIndex,  /* I */
  mccfg_t        *C,       /* I */
  const char     *CmdBuf,  /* I encoded according to wire protocol */
  char          **RspBuf,  /* O (optional,ptr) */
  char           *EMsg,    /* O (optional,minsize=MMAX_LINE) */
  msocket_t      *SS)      /* O (modified,optional) */

  {
  static msocket_t tmpS;

  msocket_t   *S;

  static char         SBuffer[MMAX_SCRIPT];  /* output send buffer */

  char        *ptr;

  enum MStatusCodeEnum status = mscNoError;
  enum MSFC scode = msfENone; 

  char        SMsg[MMAX_LINE];
  char        tmpUName[MMAX_NAME];

  mbool_t     ServerIsActive = TRUE;
  mbool_t     DoStatus;
  mbool_t     HaveFBServer = TRUE;

  const char *FName = "MCDoCommand";

  MDB(2,fCORE) MLog("%s(%s,C,%s,RspBuf,EMsg,%s)\n",
    FName,
    MUI[CIndex].SName,
    (CmdBuf != NULL) ? CmdBuf : "NULL",
    (SS != NULL) ? "SS" : "NULL");

  if (EMsg != NULL)
    EMsg[0] = '\0';

  S = (SS != NULL) ? SS : &tmpS;

  /* NOTE:  all config/alloc mem in SS is lost */

  MSUInitialize(
    S,
    (C != NULL) ? C->ServerHost : NULL,
    (C != NULL) ? C->ServerPort : 0,
    (C != NULL) ? C->Timeout : 0);

  if (RspBuf != NULL)
    *RspBuf = NULL;

  if ((C == NULL) || (CmdBuf == NULL))
    {
    if (EMsg != NULL)
      strcpy(EMsg,"internal error - invalid parameters");

    return(FAILURE);
    }

  if ((C->CmdFlags == NULL) || !strstr(C->CmdFlags,"status"))
    DoStatus = FALSE;
  else
    DoStatus = TRUE;

  ptr = NULL;

  SMsg[0] = '\0';

  switch (CIndex)
    {
    case mcsDiagnose:
    case mcsShowState:
    case mcsSetJobSystemPrio:
    case mcsSetJobUserPrio:
    case mcsShowQueue:
    case mcsSetJobHold:
    case mcsReleaseJobHold:
    case mcsStatShow:
    case mcsRsvCreate:
    case mcsRsvDestroy:
    case mcsRsvShow:
    case mcsSetJobQOS:
    case mcsShowResAvail:
    case mcsShowConfig:
    case mcsCheckJob:
    case mcsCheckNode:
    case mcsJobStart:
    case mcsCancelJob:
    case mcsConfigure:
    case mcsShowEstimatedStartTime:
    case mcsMNodeCtl:
    case mcsMVCCtl:
    case mcsMVMCtl:
    case mcsMJobCtl:
    case mcsMShow:
    case mcsMRsvCtl:
    case mcsMBal:
    case mcsCredCtl:
    case mcsRMCtl:
    case mcsMSchedCtl:
    case mcsMSubmit:

      if (C->RID[0] != '\0')
        MUStrDup(&S->ClientName,C->RID);

      if (C->ServerCSKey[0] != '\0')
        strcpy(S->CSKey,C->ServerCSKey);
      else
        strcpy(S->CSKey,MSched.DefaultCSKey);

#ifdef MENABLETID

      S->LocalTID = (rand() % MMIN_TRANSACTIONID) + MMIN_TRANSACTIONID;
 
#endif /* MENABLETID */

#ifdef __MDEBUG

      {
      char *UserName;
      char *MoabKey;

      if ((UserName = getenv("MOABUSER")) != NULL)
        {
        MUStrDup(&S->ClientName,UserName);
        }

      if ((MoabKey = getenv("MOABKEY")) != NULL)
        {
        strncpy(S->CSKey,MoabKey,sizeof(S->CSKey));
        } 
      }

#endif /* __MDEBUG */

      if (C->ServerCSAlgo != mcsaNONE)
        S->CSAlgo = C->ServerCSAlgo;
      else
        S->CSAlgo = MSched.DefaultCSAlgo;

      S->DoEncrypt      = C->DoEncryption;

      S->SocketProtocol = C->SocketProtocol;

      S->SBuffer        = SBuffer;

      S->Timeout        = C->Timeout;

      S->IsNonBlocking  = C->IsNonBlocking;
      S->P              = C->Peer;

      strcpy(S->Label,C->Label);

      if (C->Forward != NULL)
        {
        MUStrDup(&S->Forward,C->Forward);
        }

      if (S->Timeout == 0)
        S->Timeout = MDEF_CLIENTTIMEOUT;

      /* set communication protocol */

      switch (CIndex)
        {
        case mcsShowState:
        case mcsRsvShow:
        case mcsShowQueue:
        case mcsMShow:
        case mcsDiagnose:
        case mcsCredCtl:
        case mcsMRsvCtl:
        case mcsRMCtl:
        case mcsMSchedCtl:
        case mcsMJobCtl:
        case mcsMSubmit:
        case mcsMNodeCtl:
        case mcsMVCCtl:
        case mcsMVMCtl:
        case mcsStatShow:
        case mcsMBal:
        case mcsCheckJob:
        case mcsCheckNode:
        case mcsMStat:
        case mcsMDiagnose:

          S->WireProtocol = mwpS32;

          MUStrCpy(S->SBuffer,CmdBuf,sizeof(SBuffer));

          break;

/*      the following are mapped to mcsMJobCtl: */
        case mcsSetJobHold:
        case mcsReleaseJobHold:
        case mcsSetJobQOS:
        case mcsSetJobSystemPrio:
        case mcsSetJobUserPrio:
        case mcsCancelJob:
        case mcsJobStart:
        case mcsShowEstimatedStartTime:
/*      the following are mapped to mcsMSchedCtl: */
        case mcsShowConfig:
        case mcsConfigure:
/*      the following are mapped to mcsMRsvCtl: */
        case mcsRsvCreate:
        case mcsRsvDestroy:

        case mcsShowResAvail:
        case mcsNONE:
        case mcsLAST:

          snprintf(S->SBuffer,sizeof(SBuffer),"%s%s %s%s %s%s\n",
            MCKeyword[mckCommand],
            MUI[CIndex].SName,
            MCKeyword[mckAuth],
            MUUIDToName(MOSGetEUID(),tmpUName),
            MCKeyword[mckArgs],
            CmdBuf);

          break;
        }  /* END switch (CIndex) */

      S->SBufSize = (long)strlen(S->SBuffer);

      HaveFBServer = TRUE;

      /* attempt connection to primary */

      if ((MSUConnect(S,FALSE,EMsg) == FAILURE) || 
          (MCSendRequest(S,EMsg) == FAILURE))
        {
        mbool_t PrimaryConnected = FALSE;

        if ((EMsg != NULL) &&  
            (!strcmp(EMsg,"cannot load packet size")))
          {
          /* do not attempt connection to fallback if primary intentionally dropped response */

          return(FAILURE);
          }
  
        /* try to reconnect to primary server */

        if ((MSched.IsServer == FALSE) &&
            (MSched.ClientMaxPrimaryRetry > 0))
          {
          int retried = 0;

          while (retried < MSched.ClientMaxPrimaryRetry)
            {
            MSUDisconnect(S);

            fprintf(stderr,"INFO:     connection failed, will retry in %.2f seconds : attempt %d of %ld\n",
                (double)MSched.ClientMaxPrimaryRetryTimeout / 1000000.0, /* divide micorseconds to get seconds */
                retried + 1,
                MSched.ClientMaxPrimaryRetry);

            MUSleep(MSched.ClientMaxPrimaryRetryTimeout,FALSE);  /* sleep until attempt next retry */

            if ((MSUConnect(S,FALSE,EMsg) == SUCCESS) && 
                (MCSendRequest(S,EMsg) == SUCCESS))
              {
              PrimaryConnected = TRUE;

              printf("SUCCESS:  connected to primary server\n");

              break;
              }
            
            retried++;
            }
          } /* END if ((MSched.IsServer == FALSE) && */

        if (PrimaryConnected == FALSE)
          { 
          MSUDisconnect(S);
          }

        if (PrimaryConnected == FALSE)
          {
          if ((MSched.IsServer == TRUE) &&
              (S->P != NULL) &&
              (((mpsi_t *)S->P)->FallBackP != NULL))
            {
            /* we are a source peer trying to talk to a destination peer--use specified fallback server */

            mpsi_t *FBP = ((mpsi_t *)S->P)->FallBackP;

            MUStrCpy(S->RemoteHost,FBP->HostName,sizeof(S->RemoteHost));
            S->RemotePort = FBP->Port;
            }
          else if ((MSched.IsServer == FALSE) && (MSched.AltServerHost[0] != '\0'))
            {
            /* attempt connection to fallback server */

            MUStrCpy(S->RemoteHost,MSched.AltServerHost,sizeof(S->RemoteHost));
            }
          else
            {
            HaveFBServer = FALSE;
            }
          } /* END if (PrimaryConnected == FALSE) */

        if ((PrimaryConnected == FALSE) && (HaveFBServer == TRUE))
          {
          if ((MSUConnect(S,FALSE,NULL) == FAILURE) || 
              (MCSendRequest(S,NULL) == FAILURE))
            {
            MSUDisconnect(S);

            if (DoStatus == FALSE)
              {
              MDB(0,fSOCK) MLog("ERROR:    cannot contact %s:%d (%s may not be running)\n",
                S->RemoteHost,
                S->RemotePort,
                MSCHED_SNAME);

              if (EMsg != NULL)
                {
                sprintf(EMsg,"cannot connect to remote server at %s:%d",
                  S->RemoteHost,
                  S->RemotePort);
                }

              return(FAILURE);
              }

            ServerIsActive = FALSE;
            }  /* END if ((MSUConnect() == FAILURE) || ...) */

          /* successful connect to FB server */
          }    /* END if (HaveFBServer == TRUE) */
        else if (PrimaryConnected == FALSE)
          {
          /* cannot connect to primary, no FB server available */

          if (DoStatus == FALSE)
            {
            if ((EMsg != NULL) && (EMsg[0] != '\0'))
              {
              if (strstr(EMsg,"connection refused"))
                {
                if (C->Format == mfmXML)
                  {
                  mxml_t *SE = NULL;
                  mxml_t *RE = NULL;

                  char tmpLine[MMAX_LINE];

                  MS3SetStatus(NULL,&SE,"Failure",msfCannotConnect,EMsg);

                  MXMLCreateE(&RE,MSON[msonResponse]);

                  MXMLAddE(RE,SE);
           
                  /* NOTE:  line below reports only 'status' element.  Consider
                            reporting 'response' element in Moab 6.0 and higher
                            by changing 'SE' to 'RE' below */
 
                  MXMLToString(
                    SE,
                    tmpLine,
                    sizeof(tmpLine),
                    NULL,
                    TRUE);

                  MXMLDestroyE(&RE);
 
                  fprintf(stderr,"%s\n",
                    tmpLine); 
                  }
                else
                  {
                  MLog("ERROR:    %s\n",
                    EMsg);
                  }
                }    /* END if (strstr(EMsg,"connection refused")) */
              else
                {
                if (C->Format == mfmXML)
                  {
                  mxml_t *SE = NULL;
                  mxml_t *RE = NULL;
 
                  char tmpLine[MMAX_LINE];

                  sprintf(tmpLine,"communication error %s:%d (%s)\n",
                    S->RemoteHost,
                    S->RemotePort,
                    EMsg);        

                  MS3SetStatus(NULL,&SE,"Failure",msfCannotConnect,tmpLine);

                  MXMLCreateE(&RE,MSON[msonResponse]);

                  MXMLAddE(RE,SE);

                  /* NOTE:  line below reports only 'status' element.  Consider
                            reporting 'response' element in Moab 6.0 and higher
                            by changing 'SE' to 'RE' below */

                  MXMLToString(
                    SE,
                    tmpLine,
                    sizeof(tmpLine),
                    NULL,
                    TRUE);

                  MXMLDestroyE(&RE);

                  fprintf(stderr,"%s\n",
                    tmpLine);
                  }  /* END if (C->Format == mfmXML) */
                else
                  {
                  MLog("ERROR:    communication error %s:%d (%s)\n",
                    S->RemoteHost,
                    S->RemotePort,
                    EMsg);
                  }
                }
              }    /* END if ((EMsg != NULL) && (EMsg[0] == '\0')) */
            else
              {
              if (C->Format == mfmXML)
                {
                mxml_t *SE = NULL;
                mxml_t *RE = NULL;

                char tmpLine[MMAX_LINE];

                sprintf(tmpLine,"cannot contact %s:%d (%s may not be running)\n",
                  S->RemoteHost,
                  S->RemotePort,
                  MSCHED_SNAME);

                MS3SetStatus(NULL,&SE,"Failure",msfCannotConnect,tmpLine);

                MXMLCreateE(&RE,MSON[msonResponse]);

                MXMLAddE(RE,SE);

                /* NOTE:  line below reports only 'status' element.  Consider
                          reporting 'response' element in Moab 6.0 and higher
                          by changing 'SE' to 'RE' below */

                MXMLToString(
                  SE,
                  tmpLine,
                  sizeof(tmpLine),
                  NULL,
                  TRUE);

                MXMLDestroyE(&RE);

                fprintf(stderr,"%s\n",
                  tmpLine);
                }  /* END if (C->Format == mfmXML) */
              else
                {
                MLog("ERROR:    cannot contact %s:%d (%s may not be running)\n",
                  S->RemoteHost,
                  S->RemotePort,
                  MSCHED_SNAME);
                }
              }    /* END else */
             
            if ((EMsg != NULL) && (EMsg[0] == '\0'))
              { 
              sprintf(EMsg,"cannot connect to remote server at %s:%d",
                S->RemoteHost,
                S->RemotePort);
              }
              
            return(FAILURE);
            }  /* END if (DoStatus == FALSE) */

          ServerIsActive = FALSE;
          }  /* END else (HaveFBServer == TRUE) */
        }    /* END if ((MSUConnect(S,FALSE,NULL) == FAILURE) || ...) */

      if (ServerIsActive == FALSE)
        {
        /* cannot connect/send data to server */

        if (DoStatus == FALSE)
          {
          if ((S->WireProtocol == mwpXML) || (S->WireProtocol == mwpS32))
            {
            MDB(0,fSOCK) MLog("ERROR:    cannot contact %s:%d (%s may not be running)\n",
              S->RemoteHost,
              S->RemotePort,
              MSCHED_SNAME);

            if (EMsg != NULL)
              {
              sprintf(EMsg,"cannot connect to remote server at %s:%d",
                S->RemoteHost,
                S->RemotePort);
              }

            return(FAILURE);
            }
          }
        else
          {
          /* HACK:  stub in code for non-xml based status processing */

          /* do not process envelope */

          MUStrDup(&S->RBuffer,"<Data><sched Status=\"down\"></sched></Data>\n");

          S->RPtr = S->RBuffer;

          MXMLFromString(&S->RDE,S->RPtr,NULL,NULL);

          status = mscNoError;
          }
        }

      /* connection established and data sent */

      switch (S->WireProtocol)
        {
        case mwpXML:

          /* should the below be changed to populate S->RE? */
 
          if (((ptr = strchr(S->RBuffer,'<')) == NULL) ||
               (MXMLFromString(&S->SE,ptr,NULL,NULL) == FAILURE))
            {
            MDB(1,fUI) MLog("ERROR:    cannot parse server response (status)\n");

            if (EMsg != NULL)
              {
              sprintf(EMsg,"corrupt response received from remote server");
              }

            return(FAILURE);
            }

          status = mscNoError;

          break;

        case mwpS32:

          /* locate status */

          {
          mxml_t *BE;
          mxml_t *RE; 

          if ((S->RE == NULL) || 
              (MXMLGetChild(S->RE,"Body",NULL,&BE) == FAILURE) ||
              (MXMLGetChild(BE,"Response",NULL,&RE) == FAILURE) ||
              (MS3CheckStatus(RE,&scode,SMsg) == FAILURE))
            {
            if (DoStatus == FALSE)      
              status = mscFuncFailure;
            }
          else
            {
            status = mscNoError;
            }
          }  /* END BLOCK */

          /* locate data */

          if (S->RDE != NULL)
            {
            S->RPtr = (S->RDE)->Val;
            }

         break;

        default:

          {
          /* locate StatusCode */

          if ((ptr = strstr(S->RBuffer,MCKeyword[mckStatusCode])) == NULL)
            {
            MDB(1,fUI) MLog("ERROR:    cannot parse server response (status)\n");

            if (EMsg != NULL)
              {
              sprintf(EMsg,"corrupt response received from remote server");
              }

            return(FAILURE);
            }

          ptr += strlen(MCKeyword[mckStatusCode]);

          /* 1 = success, 0 = failure */

          if (strtol(ptr,NULL,10) == 0)
            status = mscFuncFailure;

          switch (S->SocketProtocol)
            {
            case mspHalfSocket:

              /* NO-OP */
 
              break;

            default:

              /* locate Data */

              if ((ptr = strstr(S->RBuffer,MCKeyword[mckData])) == NULL)
                {
                MDB(1,fUI) MLog("ERROR:    cannot parse server response (data)\n");

                if (EMsg != NULL)
                  {
                  sprintf(EMsg,"corrupt response received from remote server");
                  }

                return(FAILURE);
                }

              ptr += strlen(MCKeyword[mckData]);

              break;
            }  /* END switch (S->SocketProtocol) */

          /* locate Args (optional) */

          if (strstr(ptr,MCKeyword[mckArgs]) != NULL)
            {
            ptr = strstr(ptr,MCKeyword[mckArgs]) + strlen(MCKeyword[mckArgs]);
            }

          S->RPtr = ptr;

          if ((S->RPtr != NULL) && (S->RPtr[0] == '<'))
            {
            if (MXMLFromString(&S->RDE,S->RPtr,NULL,NULL) == SUCCESS)
              {
              mxml_t *SE;
              char    tmpLine[MMAX_LINE];

              int     tmpI;

              if ((MXMLGetChild(S->RDE,"status",NULL,&SE) == SUCCESS) &&
                  (MXMLGetAttr(SE,"code",NULL,tmpLine,sizeof(tmpLine)) == SUCCESS))
                {
                tmpI = (int)strtol(tmpLine,NULL,10);

                MXMLGetAttr(SE,"message",NULL,SMsg,sizeof(SMsg));

                if (tmpI >= 0)
                  scode = (enum MSFC)tmpI;
                else
                  scode = msfUnknownError;
                }
              }
            }
          }  /* END BLOCK */

          break;
        }    /* END switch (S->WireProtocol) */

      break;

    default:

      MDB(1,fUI) MLog("ERROR:    service '%s' (%u) not supported\n",
        MUI[CIndex].SName,
        CIndex);

      if (EMsg != NULL)
        {
        sprintf(EMsg,"local server does not support requested service");
        }

      return(FAILURE);

      /*NOTREACHED*/

      break;
    }  /* END switch (CIndex) */

  if (status != mscNoError)
    {
    if (EMsg != NULL)
      {
      char *tmpEMsg = NULL;

      if (S->RBuffer != NULL)
        tmpEMsg = strstr(S->RBuffer, "ARG=ERROR: ");

      if ((S->WireProtocol == mwpNONE) && (tmpEMsg != NULL))
        {
        sprintf(EMsg,"remote server reported request failure: %s",
          &tmpEMsg[strlen("ARG=ERROR: ")]);

        while (EMsg[strlen(EMsg)-1] == '\n')
          EMsg[strlen(EMsg)-1] = '\0';
        }
      else
        {
        sprintf(EMsg,"remote server reported request failure");
        }
      } /* END if (EMsg != NULL) */

    return(FAILURE);
    } /* END if (status != mscNoError) */

  if (scode > msfGWarning)
    {
    if (EMsg != NULL)
      {
      if (SMsg[0] != '\0')
        {
        sprintf(EMsg,"remote server reported request failure %d - %s",
          scode,
          SMsg);
        }
      else
        {
        sprintf(EMsg,"remote server reported request failure %d",
          scode);
        }
      }

    return(FAILURE);
    }

  if (RspBuf != NULL)
    *RspBuf = S->RPtr;

  return(SUCCESS);
  }  /* END MCDoCommand() */





/**
 *
 *
 * @param PName (O)
 * @param Host (O)
 * @param Port (O)
 */

int MCLoadEnvironment(

  char *PName,  /* O */
  char *Host,   /* O */
  int  *Port)   /* O */

  {
  char *ptr;

  /* load environment variables */

  if (PName != NULL)
    {
    /* load partition */

    if ((ptr = getenv(MSCHED_ENVPARVAR)) != NULL)
      {
      MDB(4,fCONFIG) MLog("INFO:     loaded environment variable %s=%s\n",
        MSCHED_ENVPARVAR,
        ptr);

      MUStrCpy(PName,ptr,MMAX_NAME);
      }
    else
      {
      MDB(5,fCONFIG) MLog("INFO:     partition not set  (using default)\n");

      MUStrCpy(PName,MCONST_GLOBALPARNAME,MMAX_NAME);
      }
    }    /* END if (PName != NULL) */

  if ((ptr = getenv(MCSERVERENV)) != NULL)
    {
    char *ptr1;
    char *ptr2;
    char *TokPtr=NULL;

    MDB(4,fCONFIG) MLog("INFO:     loaded environment variable %s=%s)\n",
      MCSERVERENV,
      ptr);

    /* FORMAT:  <HOST>[:<PORT>] */

    ptr1 = MUStrTok(ptr,": \t\n",&TokPtr);
    ptr2 = MUStrTok(NULL,": \t\n",&TokPtr);

    if ((ptr1 != NULL) && (Host != NULL))
      MUStrCpy(Host,ptr1,MMAX_NAME);

    if ((ptr2 != NULL) && (Port != NULL))
      *Port = (int)strtol(ptr2,NULL,0);
    }
  else
    {
    if (Port != NULL)
      {
      /* get port environment variable */

      if ((ptr = getenv(MParam[mcoServerPort])) != NULL)
        {
        MDB(4,fCONFIG) MLog("INFO:     loaded environment variable %s=%s)\n",
          MParam[mcoServerPort],
          ptr);

        *Port = (int)strtol(ptr,NULL,0);
        }
      }    /* END if (Port != NULL) */

    if (Host != NULL)
      {
      if ((ptr = getenv(MParam[mcoServerHost])) != NULL)
        {
        MDB(4,fCONFIG) MLog("INFO:     using %s environment variable (value: %s)\n",
          MParam[mcoServerHost],
          ptr);

        MUStrCpy(Host,ptr,MMAX_NAME);
        }
      }    /* END if (Host != NULL) */
    }      /* END if ((ptr = getenv("MOABSERVER")) != NULL) */

  return(SUCCESS);
  }  /* END MCLoadEnvironment() */





/**
 * Load and process client config file.
 *
 * @see MCInitialize() - parent
 * @see MClientLoadConfig()
 *
 * @param C (I) [modified]
 * @param Directory (I) [optional]
 * @param ConfigFile (I)
 */

int MCLoadConfig(

  mccfg_t *C,          /* I (modified) */
  char    *Directory,  /* I (optional) */
  char    *ConfigFile) /* I */

  {
  char *buf;
  char *ptr;

  char  Name[MMAX_LINE + 1];
  char  tmpLine[MMAX_LINE];
  char  tmpHome[MMAX_LINE];
  char  tmpUName[MMAX_NAME];
  char  tmpDir[MMAX_LINE];

  int   index;
  long  FSize;

  int   count;

  int   SC;

  const char *FName = "MCLoadConfig";

  MDB(2,fUI) MLog("%s(C,%s,%s)\n",
    FName,
    Directory,
    (ConfigFile != NULL) ? ConfigFile : "NULL");

  if ((C == NULL) || (ConfigFile == NULL))
    {
    return(FAILURE);
    }

  tmpDir[0] = '\0';
  if (Directory != NULL)
    MUStrCpy(tmpDir,Directory,sizeof(tmpDir));

  if ((ConfigFile[0] == '/') || (ConfigFile[0] == '~') || 
      (Directory == NULL) || (Directory[0] == '\0'))
    {
    strcpy(Name,ConfigFile);
    }
  else
    {
    if (Directory[strlen(Directory) - 1] == '/')
      {
      tmpDir[strlen(tmpDir) - 1] = '\0';

      if (MSched.CfgDir[0] == '\0')
        MUStrCpy(MSched.CfgDir,Directory,sizeof(MSched.CfgDir));
      }
    else
      {
      if (MSched.CfgDir[0] == '\0')
        {
        snprintf(MSched.CfgDir,sizeof(MSched.CfgDir),"%s/",
          Directory);
        }
      }

    sprintf(Name,"%s/%s",
      tmpDir,
      ConfigFile);
    }
  
  if ((MSched.ConfigBuffer = MFULoad(Name,1,macmRead,&count,NULL,&SC)) == NULL)
    {
    char tempDir[MMAX_PATH];
    char tempFName[MMAX_PATH];

    /* We didn't find the moab config file in MSched.CfgDir.  Check MSched.CfgDir/etc. */

    sprintf(tempDir,"%setc/",MSched.CfgDir);
    sprintf(tempFName, "%s%s",tempDir,MCONST_CONFIGFILE);
  
    MDB(3,fALL) MLog("WARNING:  cannot locate configuration file '%s' in '%s'.  Checking '%s'.\n",
      MSched.ConfigFile,
      MSched.CfgDir,
      tempDir);

    if ((MSched.ConfigBuffer = MFULoad(tempFName,1,macmRead,&count,NULL,&SC)) == NULL)
      {
      /* We didn't find the moab config file MSched.CfgDir/etc.  Check /etc.*/

      strcpy(tempDir,"/etc");
      sprintf(tempFName, "/etc/%s",MCONST_CONFIGFILE);
    
      MDB(3,fALL) MLog("WARNING:  cannot locate configuration file '%s' in '%s'.  Checking '%s'.\n",
        MSched.ConfigFile,
        MSched.CfgDir,
        tempDir);

      if ((MSched.ConfigBuffer = MFULoad(tempFName,1,macmWrite,&count,NULL,NULL)) == NULL)
        {
        /* We didn't find the moab config file /etc either. */
      
        MDB(3,fALL) MLog("ERROR:  cannot locate configuration file in any predeterminted location.\n");
  
        return(FAILURE);   
        }  /* END if(MSysFindMoabCfg(tempDir) == FAILURE) */
      }  /* END if(MSysFindMoabCfg(tempDir) == FAILURE) */

    strcpy(MSched.CfgDir,tempDir);
    strcpy(MSched.ConfigFile,tempFName);
    }  /* END if (MFULoad) */

  /* process moab.cfg */

  MCfgAdjustBuffer(&MSched.ConfigBuffer,TRUE,NULL,FALSE,TRUE,FALSE);

  MCfgGetSVal(
    MSched.ConfigBuffer,
    NULL,
    MParam[mcoServerHost],
    NULL,
    NULL,
    C->ServerHost,
    sizeof(C->ServerHost),
    1,
    NULL);

  MCfgGetIVal(
    MSched.ConfigBuffer,
    NULL,
    MParam[mcoServerPort],
    NULL,
    NULL,
    &C->ServerPort,
    NULL);

  MCfgGetSVal(
    MSched.ConfigBuffer,
    NULL,
    MParam[mcoSchedMode],
    NULL,
    NULL,
    C->SchedulerMode,
    sizeof(C->SchedulerMode),
    1,
    NULL);

  if (MCfgGetSVal(
        MSched.ConfigBuffer,
        NULL,
        MParam[mcoMCSocketProtocol],
        NULL,
        NULL,
        tmpLine,
        sizeof(tmpLine),
        1,
        NULL) == SUCCESS)
    {
    C->SocketProtocol = (enum MSocketProtocolEnum)MUGetIndex(
      tmpLine,
      MSockProtocol,
      FALSE,
      mspSingleUseTCP);
    }

  /* get admin4 user */

  if (MCfgGetSVal(
      MSched.ConfigBuffer,
      NULL,
      MParam[mcoAdmin4Users],
      NULL,
      NULL,
      tmpLine,
      sizeof(tmpLine),
      1,
      NULL) == SUCCESS)
    {
    MUStrCpy(MSched.Admin[4].UName[0],tmpLine,MMAX_NAME);
    }

  /* get timeout */

  if (C->Timeout <= 0)
    {
    if ((ptr = getenv(MParam[mcoClientTimeout])) != NULL)
      {
      C->Timeout = MUTimeFromString(ptr) * MDEF_USPERSECOND;
      }
    else if (MCfgGetSVal(
        MSched.ConfigBuffer,
        NULL,
        MParam[mcoClientTimeout],
        NULL,
        NULL,
        tmpLine,         /* O */
        sizeof(tmpLine),
        1,
        NULL) == SUCCESS)
      {
      C->Timeout = MUTimeFromString(tmpLine) * MDEF_USPERSECOND;
      }
    }

  if (C->Timeout == 0)
    {
    C->Timeout = MDEF_CLIENTTIMEOUT;
    }

  /* get primary retry numbers */

  /* defaults */

  MSched.ClientMaxPrimaryRetry        = MDEF_CLIENTRETRYCOUNT;
  MSched.ClientMaxPrimaryRetryTimeout = MDEF_CLIENTRETRYTIME;
  
  if (MCfgGetSVal(
      MSched.ConfigBuffer,
      NULL,
      MParam[mcoClientMaxPrimaryRetry],
      NULL,
      NULL,
      tmpLine,
      sizeof(tmpLine),
      1,
      NULL) == SUCCESS)
    {
    MSched.ClientMaxPrimaryRetry = MUTimeFromString(tmpLine); /* handle INFINITY */
    }

  if (MCfgGetIVal(
      MSched.ConfigBuffer,
      NULL,
      MParam[mcoClientMaxPrimaryRetryTimeout],
      NULL,
      NULL,
      &MSched.ClientMaxPrimaryRetryTimeout,
      NULL) == SUCCESS)
    {
    /* Config reports timeout in milli seconds, time to get microseconds */

    MSched.ClientMaxPrimaryRetryTimeout *= 1000; 
    }

  /* Get config values for MAXGMETRIC & MAXGRES */

  MCfgGetIVal(MSched.ConfigBuffer,NULL,MParam[mcoMaxGMetric],NULL,NULL,
              &MSched.M[mxoxGMetric],NULL);

  MCfgGetIVal(MSched.ConfigBuffer,NULL,MParam[mcoMaxGRes],NULL,NULL,
              &MSched.M[mxoxGRes],NULL);

  /* load enable encryption */

  if (MCfgGetSVal(
        MSched.ConfigBuffer,
        NULL,
        MParam[mcoEnableEncryption],
        NULL,
        NULL,
        tmpLine,         /* O */ 
        sizeof(tmpLine),
        0,
        NULL) == SUCCESS)
    {
    C->EnableEncryption = MUBoolFromString(tmpLine,FALSE);
    }

  /* load disable UI compression */

  if (MCfgGetSVal(
        MSched.ConfigBuffer,
        NULL,
        MParam[mcoDisableUICompression],
        NULL,
        NULL,
        tmpLine,         /* O */
        sizeof(tmpLine),
        0,
        NULL) == SUCCESS)
    {
    MSched.DisableUICompression = MUBoolFromString(tmpLine,FALSE);
    }

  if (MCfgGetSVal(
        MSched.ConfigBuffer,
        NULL,
        MParam[mcoClientCfg],
        NULL,
        NULL,
        tmpLine,         /* O */
        sizeof(tmpLine),
        0,
        NULL) == SUCCESS)
    {
    MClientProcessConfig(C,tmpLine,TRUE);
    }

  if (MCfgGetSVal(
        MSched.ConfigBuffer,
        NULL,
        MParam[mcoSubmitFilter],
        NULL,
        NULL,
        tmpLine,         /* O */
        sizeof(tmpLine),
        0,
        NULL) == SUCCESS)
    {
    MUStrCpy(MSched.ClientSubmitFilter,tmpLine,sizeof(MSched.ClientSubmitFilter));
    }  

  if (MCfgGetSVal(
        MSched.ConfigBuffer,
        NULL,
        MParam[mcoEnableFastSpawn],
        NULL,
        NULL,
        tmpLine,         /* O */
        sizeof(tmpLine),
        0,
        NULL) == SUCCESS)
    {
    MSched.EnableFastSpawn = MUBoolFromString(tmpLine,FALSE);
    }

  MCfgGetSVal(
    MSched.ConfigBuffer,
    NULL,
    MParam[mcoDisplayFlags],
    NULL,
    NULL,
    tmpLine,         /* O */
    sizeof(tmpLine),
    0,
    NULL);

  bmclear(&C->DisplayFlags);

  if (tmpLine[0] != '\0')
    {
    for (index = 0;MCDisplayType[index] != NULL;index++)
      {
      if (strstr(tmpLine,MCDisplayType[index]) != NULL)
        {
        bmset(&C->DisplayFlags,index);
        }
      }    /* END for (index) */
    }  /* END if (tmpLine[0] != '\0') */

  MCfgGetSVal(
    MSched.ConfigBuffer,
    NULL,
    MParam[mcoEnablePosUserPriority],
    NULL,
    NULL,
    tmpLine,         /* O */
    sizeof(tmpLine),
    0,
    NULL);

  if (MUBoolFromString(tmpLine,FALSE) == TRUE)
    {
    bmset(&C->DisplayFlags,mdspfEnablePosPrio);

    MDB(2,fALL) MLog("INFO:     %s set to %s\n",
      MParam[mcoEnablePosUserPriority],
      MBool[TRUE]);
    }

  if (MCfgGetSVal(
        MSched.ConfigBuffer,
        NULL,
        MParam[mcoFilterCmdFile],
        NULL,
        NULL,
        tmpLine,          /* O */
        sizeof(tmpLine),
        0,
        NULL) == SUCCESS)
    {
    mbool_t tmpB;

    tmpB = MUBoolFromString(tmpLine,FALSE);

    if (tmpB == FALSE)
      C->NoProcessCmdFile = TRUE;
 
#ifdef MNOFILTERCMDFILE
    C->NoProcessCmdFile = TRUE;
#endif /* MNOFILTERCMDFILE */

    MDB(2,fALL) MLog("INFO:     %s set to %s\n",
      MParam[mcoFilterCmdFile],
      MBool[C->NoProcessCmdFile]);
    }

  if ((MUBoolFromString(tmpLine,FALSE) == TRUE) ||
      (strcasecmp(tmpLine,"FORCE") == 0))
    {
    bmset(&C->DisplayFlags,mdspfServiceProvisioning);
    }

  if (MCfgGetSVal(
        MSched.ConfigBuffer,
        NULL,
        MParam[mcoMapFeatureToPartition],
        NULL,
        NULL,
        tmpLine,          /* O */
        sizeof(tmpLine),
        0,
        NULL) == SUCCESS)
    {
    if (MUBoolFromString(tmpLine,FALSE) == TRUE)
      C->MapFeatureToPartition = TRUE;
 
    MDB(2,fALL) MLog("INFO:     %s set to %s\n",
      MParam[mcoMapFeatureToPartition],
      MBool[C->MapFeatureToPartition]);
    }

  /* reject dos scripts set at compile time */

#ifdef REJECTDOSSCRIPTS
  C->RejectDosScripts = TRUE;
#endif

  /* reject dos scripts can be overridden in the config file */

  if (MCfgGetSVal(
        MSched.ConfigBuffer,
        NULL,
        MParam[mcoRejectDosScripts],
        NULL,
        NULL,
        tmpLine,          /* O */
        sizeof(tmpLine),
        0,
        NULL) == SUCCESS)
    {
    C->RejectDosScripts = MUBoolFromString(tmpLine,FALSE);
 
    MDB(2,fALL) MLog("INFO:     %s set to %s\n",
      MParam[mcoRejectDosScripts],
      MBool[C->RejectDosScripts]);
    }

  /* check for user-specific config */

  MUUIDFromName(MUUIDToName(getuid(),tmpUName),tmpHome,NULL);

  snprintf(tmpLine,sizeof(tmpLine),"%s/.%s%s",
      tmpHome,
      MSCHED_SNAME,
      MDEF_CONFIGSUFFIX);

  if ((MFUGetInfo(tmpLine,NULL,&FSize,NULL,NULL) != FAILURE) &&
      (FSize > 0) &&
     ((buf = MFULoad(Name,1,macmRead,&count,NULL,&SC)) == NULL))
    {
    /* process .moab.cfg */

    /* load CLIENTCFG parameter */

    MCfgAdjustBuffer(&buf,TRUE,NULL,FALSE,TRUE,FALSE);

    MClientLoadConfig(C,buf);

    MUFree(&buf);
    }  /* END if ((MFUGetInfo(tmpLine,NULL,&FSize,NULL,NULL) != FAILURE) && ...) */

  MClientLoadConfig(C,NULL);

  if (MSchedLoadConfig(NULL,FALSE) == SUCCESS)
    {
    if (MSched.ServerPort > 0)
      C->ServerPort = MSched.ServerPort;

    if (MSched.ServerHost[0] != '\0')
      strcpy(C->ServerHost,MSched.ServerHost);

    if (MSched.Mode != 0)
      strcpy(C->SchedulerMode,MSchedMode[MSched.Mode]);
    }  /* END if (MSchedLoadConfig() == SUCCESS) */

  return(SUCCESS);
  }  /* END MCLoadConfig() */



/** 
 * Wrapper for MUGetOpt to make it stop parsing at the first 
 * non-flag argument.  If the -w arguments get more complicated
 * a new argument parser will be warranted.
 *
 * @see MuGetOpt()
 *
 * @param ArgC (I) argc pointer
 * @param ArgV (I) argv
 * @param ParseLine (I) colon delimited parse string (only the first char matters in each param)
 * @param OptArg (O) [optional,maxsize=MMAX_BUFFER]
 * @param Tok (I/O) position token
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 */

int MBalGetOpt(

  int         *ArgC,      /* I argc pointer     */
  char       **ArgV,      /* I argv             */
  const char  *ParseLine, /* I parse string     */
  char       **OptArg,    /* O (optional,maxsize=MMAX_BUFFER) */
  int         *Tok,       /* I/O position token */
  char        *EMsg)      /* O (optional,minsize=MMAX_LINE) */

  {
  int aindex = 0;

  if (Tok != NULL)
    aindex = *Tok;

  /* the argument is not a flag */

  if ((ArgV[aindex] != NULL) && (ArgV[aindex][0] != '-'))
    {
    return('?');
    }
    
  return(MUGetOpt(ArgC,ArgV,ParseLine,OptArg,Tok,EMsg));
  }  /* END MBalGetOpt() */





/**
 * Create 'balance' client request (create 'mbal' request)
 *
 * @see MUIBal() - process server side request
 * @see MCBal() - process server response w/in client
 *
 * @param ArgV (I)
 * @param ArgC (I)
 * @param SDEP (O)
 */

int MCBalCreateRequest(

  char   **ArgV,  /* I */
  int      ArgC,  /* I */
  mxml_t **SDEP)  /* O */

  {
  int   Flag;

  mxml_t *E  = NULL;
  mxml_t *SE = NULL;
  mxml_t *WE = NULL;

  char  OptString[MMAX_LINE];
  char *OptArg;

  int   ArgCount;
  int   OptTok;

  mbool_t StopArgParsing = FALSE;   /* ditches argument parsing once a non-flag is reached */

  /* FORMAT */

  /* NOTE:  global command modifiers should be checked and incorporated  (NYI) */
  /*  ie, verbose, format, etc */

  MXMLCreateE(&E,MSON[msonRequest]);

  MS3SetObject(E,"cluster",NULL);

  ArgCount = ArgC;

  MCProcessGeneralArgs(mcsMBal,ArgV,&ArgCount,E);

  /* first extract command subtype */

  OptTok = 1;

  MBalMode = mbcmExecute;

  /* q-QUERY r-RUN w-host list X-forward X11 t-pseudo TTY Q-quiet */

  C.MBalForwardX = FALSE;
  C.MBalPseudoTTY = FALSE;
  C.MBalQuiet = FALSE;

  while ((StopArgParsing == FALSE) && ((Flag = MBalGetOpt(&ArgCount,ArgV,"qrw:XtQ",&OptArg,&OptTok,NULL)) != -1))
    {
    MDB(6,fCONFIG) MLog("INFO:     processing flag '%c'\n",
      (char)Flag);

    switch (Flag)
      {
      case 'q':

        MBalMode = mbcmQuery;

        MXMLCreateE(&SE,(char *)MSON[msonSet]);
        MXMLAddE(E,SE);

        break;

      case 'r':

        MBalMode = mbcmExecute;

        MXMLCreateE(&SE,(char *)MSON[msonSet]);
        MXMLAddE(E,SE);

        break;

      case 'w':
        {
        char *aptr;
        char *vptr;

        char *TokPtr;
        
        /* FORMAT:  <ATTR>=<VAL> */

        aptr = MUStrTok(OptArg,"=",&TokPtr);
        vptr = MUStrTok(NULL,"=",&TokPtr);

        if ((aptr != NULL) && (vptr != NULL))
          MS3AddWhere(E,aptr,vptr,NULL);
        }

        break;

      case 'X':

        C.MBalForwardX = TRUE;

        break;

      case 't':

        C.MBalPseudoTTY = TRUE;

        break;

      case 'Q':

        C.MBalQuiet = TRUE;

        break;

      case '?':

        StopArgParsing = TRUE;

        break;

      default:
        /* NO-OP */
        break;
      }  /* END switch (Flag) */
    }    /* END while (Flag = MUGetOpt()) */

  MXMLSetAttr(E,MSAN[msanAction],(void *)MBalCmds[MBalMode],mdfString);

  /* extract subcommand arguments */

  strcpy(OptString,"");

  OptTok = 1;

  while ((Flag = MUGetOpt(&ArgCount,ArgV,OptString,&OptArg,&OptTok,NULL)) != -1)
    {
    MDB(6,fCONFIG) MLog("INFO:     processing flag '%c'\n",
      (char)Flag);

    switch (Flag)
      {
      default:

        /* NO-OP */

        break;
      }  /* END switch (Flag) */
    }    /* END while (Flag = MUGetOpt() != -1) */

  if (ArgCount > 1)
    {
    int aindex;
    int eindex;

    MXMLCreateE(&WE,(char *)MSON[msonWhere]);
    MXMLAddE(E,WE);

    for (aindex = 1,eindex = 0;aindex < ArgCount && eindex < MAX_MBALEXECLIST;aindex++)
      {
      MBalExecList[eindex++] = ArgV[aindex];

      MXMLSetAttr(WE,"name",(void *)MClusterAttr[mcluaName],mdfString);
      MXMLSetAttr(WE,"value",(void *)ArgV[aindex],mdfString);
      }  /* END for (aindex) */

    MBalExecList[eindex + 1] = NULL;   /* array has +2 elements than MAX_MBALEXECLIST */
    }    /* END if (ArgCount > 1) */

  if (SDEP != NULL)
    *SDEP = E;

  return(SUCCESS);
  }  /* END MCBalCreateRequest() */





/**
 * @see MCBalCreateRequest() - process 'mbal' request.
 *
 * @param Parm1 (I)
 * @param Parm2 (I)
 */

int MCBal(

  void *Parm1,  /* I */
  int   Parm2)  /* I */

  {
  msocket_t *S = (msocket_t *)Parm1;

  mxml_t *DE = NULL;

  const char *FName = "MCBal";

  MDB(2,fUI) MLog("%s(Parm1,Parm2)\n",
    FName);

  if ((S == NULL) || (S->RDE == NULL))
    {
    return(FAILURE);
    }

  /* extract response */

  DE = S->RDE;

  if (DE->Val == NULL)
    {
    fprintf(stderr,"ERROR:  cannot determine best host (no available host)\n");

    return(FAILURE);
    }

  if (MBalMode == mbcmExecute)
    {
    char ForwardX[MMAX_NAME];
    char PseudoTTY[MMAX_NAME];
    char DashQ[MMAX_NAME];
    char DashO[MMAX_NAME];
    char Strict[MMAX_NAME];
    char Batch[MMAX_NAME];

    char *MBalArgList[32];

    int aindex = 0;

    MUStrCpy(ForwardX,"-X",MMAX_NAME);
    MUStrCpy(PseudoTTY,"-t",MMAX_NAME);
    MUStrCpy(DashQ,"-q",MMAX_NAME);
    MUStrCpy(DashO,"-o",MMAX_NAME);
    MUStrCpy(Strict,"StrictHostKeyChecking=no",MMAX_NAME);
    MUStrCpy(Batch,"BatchMode=yes",MMAX_NAME);

    /* launch job on ideal node if available */

    MBalArgList[aindex++] = strdup(MNAT_RCMD);   /* shell program string */
    MBalArgList[aindex++] = DE->Val;       /* hostname */

    if (C.MBalForwardX)                /* optional X forward flag */
      MBalArgList[aindex++] = ForwardX;
    if (C.MBalPseudoTTY)               /* optional pseudo-tty flag */
      MBalArgList[aindex++] = PseudoTTY;
    if (C.MBalQuiet)                   /* optional batch quiet flag */
      {
      MBalArgList[aindex++] = DashQ;
      MBalArgList[aindex++] = DashO;
      MBalArgList[aindex++] = Strict;
      MBalArgList[aindex++] = DashO;
      MBalArgList[aindex++] = Batch;
      }

    if (MBalExecList[0] != NULL)
      {
      int eindex = 0;

      fprintf(stderr,"\nexecuting command '%s' on host '%s'\n\n",
        MBalExecList[0],
        MBalArgList[1]);

      for (eindex = 0;MBalExecList[eindex] != NULL;eindex++)
        {
        if (eindex >= 32)
          break;

        MBalArgList[aindex++] = MBalExecList[eindex];
        }
      }
    else
      {
      fprintf(stderr,"\nlogin to host '%s'\n\n",
        MBalArgList[1]);
      }

    MBalArgList[aindex] = NULL;

    execv(MBalArgList[0],MBalArgList);

    exit(0);
    }
  else
    {
    fprintf(stdout,"\n\n%s\n",
      DE->Val);
    }

  return(SUCCESS);
  }  /* END MCBal() */


/**
 * Displays Credential Information
 *
 * @param RDE (I)
 */

void MDisplayCredInfo(

  mxml_t *RDE)  /* I */
  
  {
  mxml_t *BE;
  int CTok;
  int ccode;
  int index;
  int index2;
  char name[MMAX_LINE];
  char buffer[MMAX_LINE];

  /* XML attribute tags */

  const char *attr[] = {
    "Profile",
    "limits",
    "CList",
    "AList",
    "GList",
    "role",
    "services",
    "UList",
    NULL};

  /* User Display String for each tag. */

  const char *displayAttr[] = {
    "Profile",
    "Limits",
    "Class List",
    "Acct List",
    "Group List",
    "Role",
    "Services",
    "User List",
    NULL};

  enum MXMLOTypeEnum credentialObjects[] = { 
    mxoUser,
    mxoGroup,
    mxoAcct,
    mxoClass,
    mxoQOS,
    mxoLAST};

  /* Get credential object info, such as user, group, acct, class, qos. */

  for (index=0;credentialObjects[index] != mxoLAST;index++)
    {
    CTok = -1;
    
    ccode = MXMLGetChild(RDE,MXO[credentialObjects[index]],&CTok,&BE); 

    while (ccode == SUCCESS)
      {
      /* Display the attribute name */

      if(MXMLGetAttr(BE,"ID",NULL,name,sizeof(name)))
        fprintf(stdout,"\n%s: %s\n",
          MXOC[credentialObjects[index]],
          name);

      /* Display sub-attributes */

      for (index2=0;attr[index2] != NULL;index2++)
        {
        if (MXMLGetAttr(BE,attr[index2],NULL,buffer,sizeof(buffer)) == SUCCESS)
          {
          fprintf(stdout,"  %s: %s\n",
            displayAttr[index2],
            buffer);
          }
        }

      /* Display all sub-values */

      for (index2=0;attr[index2] != NULL;index2++)
        {
        if ((MXMLGetChild(BE,attr[index2],NULL,&BE) == SUCCESS) && (BE->ACount > 0))
          {
          int x;
          fprintf(stdout,"  %s: \n",displayAttr[index2]);
          for (x=0; x < BE->ACount; x++)
            {
            fprintf(stdout,"    %s: %s\n",
              BE->AName[x],
              BE->AVal[x]);
            }
          fprintf(stdout,"\n");
          } 
        }

      ccode = MXMLGetChild(RDE,MXO[credentialObjects[index]],&CTok,&BE);
      }

    }

  /* Display Role information */

  ccode = MXMLGetChild(RDE,"admin",&CTok,&BE); 

  while (ccode == SUCCESS)
    {
    /* Display the attribute name */

    if (MXMLGetAttr(BE,"ID",NULL,name,sizeof(name)))
      fprintf(stdout,"\nName:     %s\n",name);

    if (MXMLGetAttr(BE,"services",NULL,buffer,sizeof(buffer)))
      fprintf(stdout,"Services: %s\n",buffer);

    ccode = MXMLGetChild(RDE,"admin",&CTok,&BE);
    }

  return;
  }


/**
 * Displays Limit Information
 *
 * @param RDE (I)
 */

void MDisplayCredLimits(

  mxml_t *RDE)  /* I */
  
  {
  mxml_t *PE;
  mxml_t *CE;
  mxml_t *LE;

  int CTok;
  int ATok;

  int index;

  char name[MMAX_LINE];
  char val[MMAX_LINE];

  const char *id;

  mbool_t FirstPartition = TRUE;
  mbool_t FirstCred = TRUE;
  mbool_t FirstRM = TRUE;

  enum MXMLOTypeEnum credentialObjects[] = { 
    mxoUser,
    mxoGroup,
    mxoAcct,
    mxoClass,
    mxoQOS,
    mxoLAST};

  id = "NAME"; /* rm's identifying attribute is called NAME */

  CTok = -1;

  fprintf(stdout,"\n");

  /* display RM limits if any are found */

  while (MXMLGetChild(RDE,"rm",&CTok,&PE) == SUCCESS)
    {
    /* found an rm */

    if (FirstRM)
      {
      fprintf(stdout,"resource manager limits\n");
      fprintf(stdout,"-----------------------");
      FirstRM = FALSE;
      }

    MXMLGetAttr(PE,id,NULL,val,sizeof(val));

    fprintf(stdout,"\nRM[%s]\n",val);

    ATok = -1;

    name[0] = '\0';

    /* display each RM limit one per line */

    while (MXMLGetAnyAttr(PE,name,&ATok,val,sizeof(val)) == SUCCESS)
      {
      if (strcmp(id,name))
        {
        /* valid attribute value pair name=val */

        fprintf(stdout,"  %s=%s\n",name,val);
        }

      name[0] = '\0';
      }
    }

  CTok = -1;

  /* display partition limits */

  while (MXMLGetChild(RDE,"par",&CTok,&PE) == SUCCESS)
    {
    /* found a partition */

    if (FirstPartition)
      {
      fprintf(stdout,"\npartition limits\n");
      fprintf(stdout,"-----------------");
      FirstPartition = FALSE;
      }
    else
      {
      fprintf(stdout,"\n");
      }

    id = "ID"; /* par's identifying attribute is called ID */

    MXMLGetAttr(PE,id,NULL,val,sizeof(val));

    fprintf(stdout,"\n%s\n",val);

    ATok = -1;

    name[0] = '\0';

    /* partition non cred limits are displayed one per line */

    while (MXMLGetAnyAttr(PE,name,&ATok,val,sizeof(val)) == SUCCESS)
      {
      if (strcmp(id,name))
        {
        /* valid attribute value pair name=val */

        fprintf(stdout,"  %s=%s\n",name,val);
        }

      name[0] = '\0';
      }

    FirstCred = TRUE;

    /* check to see if this partition has limit(s) for each cred (user, group class, etc) */

    for (index=0;credentialObjects[index] != mxoLAST;index++)
      {
      int CTok2 = -1;
      const char *OutputName = NULL;

      switch(credentialObjects[index])
        {
        case mxoClass:
          id = "NAME"; /* class's identifying attribute is called NAME */
          OutputName = "CLASSCFG";
          break;

        case mxoQOS:
          id = "QOS"; /* qos's identifying attribute is called QOS */
          OutputName = "QOSCFG";
          break;

        case mxoAcct:
          id = "Account"; /* account's identifying attribute is called Account */
          OutputName = "ACCOUNTCFG";
          break;

        case mxoGroup:
          id = "Group"; /* group's identifying attribute is called Group */
          OutputName = "GROUPCFG";
          break;

        case mxoUser:
          id = "User"; /* user's identifying attribute is called User */
          OutputName = "USERCFG";
          break;

        default:
          /* NO OP */
          break;
        } /* END switch */

      /* a partition may have multiple creds of the same type with limits.
         for example two classes (c1,c2).*/

      while (MXMLGetChild(PE,MXO[credentialObjects[index]],&CTok2,&CE) == SUCCESS)
        {
        int CTok3 = -1;

        /* Display the credential name */

        MXMLGetAttr(CE,id,NULL,val,sizeof(val));

        fprintf(stdout,"%s  %s[%s]",(FirstCred ? "" : "\n"),OutputName,val);

        FirstCred = FALSE;

        /* Display the per job limits for this credential on the same line as the credential name */

        ATok = -1;

        name[0] = '\0';

        while (MXMLGetAnyAttr(CE,name,&ATok,val,sizeof(val)) == SUCCESS)
          {
          /* attribute value pair: name=val */

          if (strcmp(id,name))
            {
            /* valid pair */

            fprintf(stdout," %s=%s",name,val);
            }

          name[0] = '\0';
          }

        /* display each of the mpu limits on their own line */

        while (MXMLGetChild(CE,"limits",&CTok3,&LE) == SUCCESS)
          {
          if (MXMLGetAttr(LE,"NAME",NULL,val,sizeof(val)) == SUCCESS)
            fprintf(stdout,"\n      %s",val);
          else
            continue;/* ERROR!! */

          /* optional active policy information */

          if (MXMLGetAttr(LE,"aptype",NULL,val,sizeof(val)) == SUCCESS)
            fprintf(stdout,"[%s]",val);

          if (MXMLGetAttr(LE,"SLIMIT",NULL,val,sizeof(val)) == SUCCESS)
            fprintf(stdout,"=%s",val);
          else
            continue;/* ERROR!! */

          /* we only get a hlimit attribute if hlimit is different than the slimit */

          if (MXMLGetAttr(LE,"HLIMIT",NULL,val,sizeof(val)) == SUCCESS)
            fprintf(stdout,",%s",val);

          /* we only get a usage arrtibute if it is non-zero */

          if (MXMLGetAttr(LE,"USAGE",NULL,val,sizeof(val)) == SUCCESS)
            fprintf(stdout," Usage=%s",val);

          } /* END if (MXMLGetChild("limits") */

        } /* END while (MXMLGetChild(PE,credObject[index])) */

      } /* END for (index,credentialObjects) */

    } /* END while (MXMLGetChild("par")) */

  fprintf(stdout,"\n\n");

  return;
  } /* END MDisplayCredLimits() */


/**
 *
 *
 * @param Parm1
 * @param Parm2 (I)
 */

int MCCredCtl(

  void *Parm1,
  int   Parm2)  /* I */

  {
  msocket_t *S = (msocket_t *)Parm1;

  mxml_t *RDE;
 
  if ((S == NULL) || (S->RDE == NULL))
    {
    return(FAILURE);
    }

  RDE = S->RDE;

  if (C.Format == mfmXML)
    {
    char *tmpBuf;

    int  BufSize;
    int  rc;

    tmpBuf = NULL;

    rc = MXMLToXString(
      RDE,
      &tmpBuf,
      &BufSize,
      MMAX_BUFFER << 9,
      NULL,
      TRUE);

    if (rc == SUCCESS)
      {
      fprintf(stdout,"\n\n%s\n",
        tmpBuf);
      }
    else
      {
      fprintf(stderr,"ERROR:  XML is corrupt\n");
      }

    MUFree(&tmpBuf);
    }
  else
    {
    if ((RDE != NULL) && (S->RPtr == NULL))
      {
      mxml_t *Policies;
      /* Display all credential information, like user,group,acct,class,qos,limit. */

      if (MXMLGetChild(RDE,"policies",NULL,&Policies) == SUCCESS)
        MDisplayCredLimits(Policies);
      else
        MDisplayCredInfo(RDE);

      /* Print the messages, like error messages */

      if((strcmp(RDE->Name,"Data") == 0) && (RDE->Val != NULL))
        fprintf(stdout,"\n%s\n",RDE->Val); 
      }
    else
      {
      if (S->RPtr != NULL)
        {
        fprintf(stdout,"\n\n%s\n",S->RPtr);
        }
      }
    }

  return(SUCCESS);
  }  /* END MCCredCtl() */




/**
 *
 *
 * @param Parm1
 * @param Parm2 (I)
 */

int MCRMCtl(

  void *Parm1,
  int   Parm2)  /* I */

  {
  msocket_t *S = (msocket_t *)Parm1;

  mxml_t *RDE;

  if ((S == NULL) || (S->RDE == NULL))
    {
    return(FAILURE);
    }

  RDE = S->RDE;

  fprintf(stdout,"\n\n%s\n",
    (RDE->Val != NULL) ? RDE->Val : "");

  return(SUCCESS);
  }  /* END MCRMCtl() */





/**
 * Report job submit results.  
 *
 * @see MCSubmitCreateRequest() - peer - create initial submit request to send to server
 * @see MCSubmitProcessArgs()
 *
 * NOTE:  If interactive submission, exec results and 'become' RM-specific 
 *        submission client.
 *
 * @param Parm1 (I)
 * @param Parm2 (I)
 */

int MCSubmit(

  void *Parm1,  /* I */
  int   Parm2)  /* I */

  {
  char SubmitShell[MMAX_NAME];

  char* ptr;

  int QueryInterval = 5;

  msocket_t *S = (msocket_t *)Parm1;

  mxml_t *RDE;

  if ((S == NULL) || (S->RDE == NULL))
    {
    return(FAILURE);
    }

  RDE = S->RDE;

  if (C.IsInteractive == TRUE)
    {
    char *ptr;
    char *tmpPtr;
    char *TokPtr;

    char *ArgV[MMAX_ARGVLEN + 1];

    int   aindex;

    if ((RDE->Val != NULL) && (strstr(RDE->Val,"ERROR")))
      {
      /* error detected */

      fprintf(stderr,"\n%s\n",
        (RDE->Val != NULL) ? RDE->Val : "");

      exit(1);
      }

    aindex = 0;

    /* NOTE:  FORMAT:  [<ATTR>=<VAL>] <EXE> [<ARG>] ... */

    if (RDE->Val != NULL)
      {
      ptr = MUStrTokEPlus(RDE->Val," \t\n",&TokPtr);

      if (ptr != NULL)
        {
        if (strchr(ptr,'=') != NULL)
          {
          char *eptr;
          char *vptr;
          char *TokPtr2=NULL;

          /* push var into env */

          eptr = MUStrTok(ptr,"=",&TokPtr2);
          vptr = MUStrTok(NULL,"=",&TokPtr2);

          MUSetEnv(eptr,vptr);
 
          ptr = MUStrTokEPlus(NULL," \t\n",&TokPtr);
          }
        }

      while (ptr != NULL)
        {
        char *ReplaceBuf = ptr;

        if (strcmp(ptr,"$SHELL") == 0)
          {
          if ((tmpPtr = getenv("SHELL")) != NULL)
            ReplaceBuf = tmpPtr;
          }

        if (strstr(ptr, "\\\"") != 0)
          {
          /* We don't free ReplaceBuf because the ArgV parameters are used in exec */

          int len = strlen(ptr);

          ReplaceBuf = (char *)MUMalloc(len + 1);   /* remember to alloc space for NULL byte */
          MUStrReplaceStr(ptr, "\\\"", "\"", ReplaceBuf, len);
          }

        ArgV[aindex++] = ReplaceBuf;

        if (aindex >= MMAX_ARGVLEN)
          {
          fprintf(stderr,"ERROR:  too many args to submit job\n");

          exit(1);
          }

        ptr = MUStrTokEPlus(NULL," \t\n",&TokPtr);
        }  /* END while (ptr != NULL) */
      }    /* END if (RDE->Val != NULL) */

    if (aindex == 0)
      {
      MUStrCpy(SubmitShell,"/bin/sh",sizeof(SubmitShell));

      /* no script specified - exec default shell */

      ArgV[aindex] = SubmitShell;

      aindex++;
      }
    
    ArgV[aindex] = NULL;

    /* exec translated script */

    if (strstr(S->RBuffer,"salloc") != NULL)
      {
      int eindex;

      /* exec salloc according to cmd line args */

      /* C.SpecEnv will have env variables if -v was on the command line. If
       * -V was specifed then full env needs to added to SpecEnv. */

      if ((C.FullEnvRequested == TRUE) && (C.EnvP != NULL))
        {
        for (eindex = 0;C.EnvP[eindex] != NULL;eindex++)
          MUArrayListAppendPtr(&C.SpecEnv,strdup(C.EnvP[eindex]));
        }

      if (execve(ArgV[0],ArgV,(char **)C.SpecEnv.Array) == -1)
        {
        fprintf(stderr,"ERROR:  cannot execute '%s', %s\n",
          ArgV[0],
          strerror(errno));

        exit(1);
        }
      } /* END if (strstr(S->RBuffer,"salloc") != NULL) */
    else
      {
      /* exec qsub interactive job */

      if (execvp(ArgV[0],ArgV) == -1)
        {
        fprintf(stderr,"ERROR:  cannot execute '%s', %s\n",
          ArgV[0],
          strerror(errno));

        exit(1);
        }
      }

    /*NOTREACHED*/

    exit(1);
    }  /* END if (C.IsInteractive == TRUE) */

  if (!bmisset(&C.DisplayFlags,mdspfHideOutput))
    {
    /* display jobid */

    if ((RDE->Val != NULL) && (strstr(RDE->Val,"ERROR")))
      {
      /* error detected */
  
      fprintf(stderr,"\n%s\n",
        (RDE->Val != NULL) ? RDE->Val : "");
      }
    else
      {
      if (C.Format != mfmXML)
        {
        fprintf(stdout,"\n%s\n",
          (RDE->Val != NULL) ? RDE->Val : "");
        }
      else
        {
        mstring_t OutStr(MMAX_LINE);

        MXMLToMString(RDE,&OutStr,NULL,FALSE);

        fprintf(stdout,"\n%s\n",
          OutStr.c_str());
        }
      }
    }    /* END if (!bmisset(&C.DisplayFlags,mdspfHideOutput)) */

  if (C.KeepAlive == TRUE)
    {
    mccfg_t *CP = &C;
    int tmpQueryInterval;

    if (MCfgGetIVal(
        MSched.ConfigBuffer,
        NULL,
        MParam[mcoMSubQueryInterval],
        NULL,
        NULL,
        &tmpQueryInterval,
        NULL) == SUCCESS)
      {
      QueryInterval = tmpQueryInterval;
      }
    else
      {
      ptr = getenv("MSUBQUERYINTERVAL");

      if (ptr != NULL)
        {
        QueryInterval = (int)strtol(ptr,NULL,10);
        QueryInterval = MAX(QueryInterval,1);
        }
      }

    if (QueryInterval <= 0)
      {
      /* Admin has disabled the keep alive option.  Job is still submitted, 
          but client won't keep querying */

      fprintf(stdout,"The -K option is disabled.  Job %s has still been submitted successfully.\n",
        RDE->Val);

      return(SUCCESS);
      }

    /* keep retrying for 1 trillion seconds */

    MSched.ClientMaxPrimaryRetry = 1000000000;    
    MSched.ClientMaxPrimaryRetryTimeout = QueryInterval * 100000; /* seconds (in us) */

    while (MCCJobQuery((void **)&CP,RDE->Val,NULL,NULL) == 0)
      {
      sleep(QueryInterval);
      }

    /* Reset to what it was before */

    MSched.ClientMaxPrimaryRetryTimeout = 5000000; /* 5 seconds (in us) */

    fprintf(stdout,"Job %s completed\n",RDE->Val);
    }  /* END if (C.KeepAlive == TRUE) */


  MUArrayListFreePtrElements(&C.SpecEnv);
  MUArrayListFree(&C.SpecEnv);

  return(SUCCESS);
  }  /* END MCSubmit() */





/**
 * Report 'mschedctl' results.
 *
 * @see MCSchedCtlCreateRequest() - peer - generate schedctl request
 * @see MUISchedCtl() - peer - process schedule request within server
 *
 * @param Parm1 (I)
 * @param DFormat (I)
 */

int MCSchedCtl(

  void *Parm1,    /* I */
  int   DFormat)  /* I */

  {
  msocket_t *S = (msocket_t *)Parm1;

  mxml_t *RDE;
  mxml_t *CE;

  if ((S == NULL) || (S->RDE == NULL))
    {
    return(FAILURE);
    }
  
  mstring_t Buffer(MMAX_LINE);

  RDE = S->RDE;
                           
  if (MXMLGetChild(RDE,(char *)MXO[mxoSched],NULL,&CE) == SUCCESS)
    {
    MXMLToMString(RDE,
      &Buffer,
      NULL,
      TRUE);

    fprintf(stdout,"\n%s\n",
      Buffer.c_str());
    }
  else if (MXMLGetChild(RDE,(char *)MXO[mxoTrig],NULL,&CE) == SUCCESS)
    {
    MXMLToMString(RDE,
      &Buffer,
      NULL,
      TRUE);

    fprintf(stdout,"\n%s\n",
      Buffer.c_str());
    }
  else if (MXMLGetChild(RDE,(char *)MXO[mxoxEvent],NULL,&CE) == SUCCESS)
    {
    MXMLToMString(RDE,
      &Buffer,
      NULL,
      TRUE);

    fprintf(stdout,"\n%s\n",
      Buffer.c_str());
    }
  else if (DFormat == mfmXML)
    {
    MXMLToMString(RDE,
      &Buffer,
      NULL,
      TRUE);

    fprintf(stdout,"\n%s\n",
      Buffer.c_str());
    }
  else
    {
    /* send empty/populated 'Data' element */

    if (RDE->Val != NULL)
      {
      if (!strncmp(RDE->Val,"ERROR:",strlen("ERROR:")))
        {
        if (C.Format != mfmXML)
          {
          fprintf(stderr,"<Error code=\"%u\">command failed - %s</Error>\n",
            msfECInternal,
            &RDE->Val[strlen("ERROR:")]);
          }
        else
          {
          fprintf(stdout,"\n%s\n",
            RDE->Val);
          }

        exit(1);
        }

      fprintf(stdout,"\n%s\n",
        RDE->Val);
      }
    else if (DFormat == mfmXML)
      {
      fprintf(stdout,"<Data></Data>");
      }
    }
                                                    
  return(SUCCESS);
  }  /* END MCSchedCtl() */





/**
 * Handle 'showbf' request
 *
 * @see XXX() - server side request
 *
 * @param Parm1 (I)
 * @param Parm2 (I)
 */

int MCClusterShowARes(

  void *Parm1,  /* I */
  int   Parm2)  /* I */

  {
  char *Buffer = (char *)Parm1;

  mxml_t *E = NULL;
  mxml_t *JE;
  mxml_t *PE;
  mxml_t *RE;

  int PTok;
  int RTok;

  char PName[MMAX_NAME];
  char Tasks[MMAX_NAME];
  char Nodes[MMAX_NAME];
  char Cost[MMAX_NAME];
  char Duration[MMAX_NAME];
  char StartTime[MMAX_NAME];

  char TID[MMAX_NAME];
  char ReqID[MMAX_NAME];
  char TString[MMAX_NAME];

  char tmpS[MMAX_NAME];
  long tmpDuration;
  long tmpStart;

  mulong Now = 0;

  const char *FName = "MCClusterShowARes";

  MDB(2,fUI) MLog("%s(Buffer)\n",
    FName);

  if (C.Format == mfmXML)
    {
    fprintf(stdout,"%s\n",
      Buffer);

    exit(0);
    }  /* END if (C.Format == mfmXML) */

  if (MXMLFromString(&E,Buffer,NULL,NULL) == FAILURE)
    {
    fprintf(stderr,"cannot parse response\n");

    return(FAILURE);
    }

  if (MXMLGetChild(E,(char *)MXO[mxoJob],NULL,&JE) == SUCCESS)
    {
    /* display query context */

    if (MXMLGetAttr(JE,"time",NULL,tmpS,sizeof(tmpS)) == SUCCESS)
      {
      Now = strtol(tmpS,NULL,0);
      }

    /* NYI */
    }

  PTok = -1;

  while (MXMLGetChild(E,(char *)MXO[mxoPar],&PTok,&PE) == SUCCESS)
    {
    RTok = -1;

    MXMLGetAttr(PE,"Name",NULL,PName,sizeof(PName));

    while (MXMLGetChild(PE,"range",&RTok,&RE) == SUCCESS)
      {
      MXMLGetAttr(RE,"proccount",NULL,Tasks,sizeof(Tasks));
      MXMLGetAttr(RE,"nodecount",NULL,Nodes,sizeof(Nodes));
      MXMLGetAttr(RE,"cost",NULL,Cost,sizeof(Cost));
      MXMLGetAttr(RE,"duration",NULL,Duration,sizeof(Duration));
      MXMLGetAttr(RE,"starttime",NULL,StartTime,sizeof(StartTime));
      MXMLGetAttr(RE,"tid",NULL,TID,sizeof(TID));
      MXMLGetAttr(RE,"reqid",NULL,ReqID,sizeof(ReqID));

      /* NOTE:  currently ignore starttime */

      if (!strcmp(PName,"template"))
        {
        /* display header */

        if (Cost[0] != '\0')
          {
          fprintf(stdout,"%-12s  %5s  %5s  %12s  %7s  %12s  %14s\n",
            "Partition",
            "Tasks",
            Nodes,
            Duration,
            Cost,
            "StartOffset",
            "StartDate");

          fprintf(stdout,"%-12s  %5s  %5s  %12s  %7s  %12s  %14s\n",
            "---------",
            "-----",
            "-----",
            "------------",
            "-------",
            "------------",
            "--------------");
          }
        else
          {
          fprintf(stdout,"%-12s  %5s  %5s  %12s  %12s  %14s\n",
            "Partition",
            "Tasks",
            Nodes,
            Duration,
            "StartOffset",
            "StartDate");

          fprintf(stdout,"%-12s  %5s  %5s  %12s  %12s  %14s\n",
            "---------",
            "-----",
            "-----",
            "------------",
            "------------",
            "--------------");
          }

        continue;
        }  /* END if (!strcmp(PName,"template")) */

      /* display resource data */

      tmpDuration = strtol(Duration,NULL,10);
      tmpStart    = strtol(StartTime,NULL,10);

      MULToTString(tmpStart - Now,TString);

      strcpy(tmpS,TString);

      if (Cost[0] != '\0')
        {
        MULToTString(tmpDuration,TString);

        fprintf(stdout,"%-12s  %5s  %5s  %12s  %7s  %12s  %14s%s%s%s%s\n",
          PName,
          Tasks,
          Nodes,
          TString,
          Cost,
          tmpS,
          MULToATString(tmpStart,NULL),
          (TID[0] != '\0') ? "  TID=" : "",
          TID,
          ((ReqID[0] != '\0') && (ReqID[0] != '0')) ? "  ReqID=" : "",
          (ReqID[0] != '0') ? ReqID : "");
        }
      else
        {
        MULToTString(tmpDuration,TString);

        fprintf(stdout,"%-12s  %5s  %5s  %12s  %12s  %14s%s%s%s%s\n",
          PName,
          Tasks,
          Nodes,
          TString,
          tmpS,
          MULToATString(tmpStart,NULL),
          (TID[0] != '\0') ? "  TID=" : "",
          TID,
          ((ReqID[0] != '\0') && (ReqID[0] != '0')) ? "  ReqID=" : "",
          (ReqID[0] != '0') ? ReqID : "");
        }

      if (tmpDuration > MCONST_EFFINF)
        {
        /* show only first range at infinity */

        break;
        }
      }    /* END while (MXMLGetChild(PE,"range",&RTok,&RE) == SUCCESS) */
    }      /* END while (MXMLGetChild(E,MXO[mxoPar],&PTok,&PE) == SUCCESS) */

  return(SUCCESS);
  }  /* END MCClusterShowARes() */





/**
 *
 *
 * @param Parm1 (I)
 * @param Parm2 (I)
 */

int MCShowConfig(

  void *Parm1,  /* I */
  int   Parm2)  /* I */

  {
  char *Buffer = (char *)Parm1;

  const char *FName = "MCShowConfig";

  MDB(2,fUI) MLog("%s(Parm1,%d)\n",
    FName,
    Parm2);

  if (C.Format != mfmXML)
    {
    fprintf(stderr,"<Error code=\"%u\">command failed - %s</Error>\n",
      msfECInternal,
      Buffer);
    }
  else
    {
    fprintf(stdout,"%s\n\n",
      Buffer);
    }

  return(SUCCESS);
  }  /* END MCShowConfig() */






/**
 *
 *
 * @param Parm1 (I)
 * @param Parm2 (I)
 */

int MCShowEstimatedStartTime(

  void *Parm1,  /* I */
  int   Parm2)  /* I */

  {
  char *Buffer = (char *)Parm1;

  const char *FName = "MCShowEstimatedStartTime";

  MDB(2,fUI) MLog("%s(Parm1,Parm2)\n",
    FName);

  fprintf(stdout,"\n\n%s\n",
    Buffer);

  return(SUCCESS);
  }  /* END MCShowEstimatedStartTime() */





/**
 *
 *
 * @param JE (I) [modified]
 */

int MCJobLoadDefaultEnv(

  mxml_t *JE)  /* I (modified) */

  {
  char *ptr;

  char  tmpLine[MMAX_LINE];
  char  tmpUName[MMAX_NAME];
  char  tmpGName[MMAX_NAME];

  mxml_t *CE;

  int CurrentUMask;

  if (JE == NULL)
    {
    return(FAILURE);
    }

  /* NYI */

  /* load user, group, cwd, env */

  if ((ptr = MUUIDToName(MOSGetUID(),tmpUName)) == NULL)
    {
    fprintf(stderr,"ERROR:  cannot validate user\n");

    return(FAILURE);
    }

  MXMLSetChild(JE,(char *)MS3JobAttr[ms3jaOwner],&CE);

  MXMLSetVal(CE,(void *)ptr,mdfString);

  MXMLSetChild(JE,(char *)MS3JobAttr[ms3jaUser],&CE);

  if (C.SpecUser != NULL)
    {
    MXMLSetVal(CE,C.SpecUser,mdfString);
    }
  else
    {
    MXMLSetVal(CE,(void *)ptr,mdfString);
    }

  MXMLSetChild(JE,(char *)MS3JobAttr[ms3jaGroup],&CE);

  if (C.SpecGroup != NULL)
    {
    MXMLSetVal(CE,(void *)C.SpecGroup,mdfString);
    }
  else
    {
    if ((ptr = MUGIDToName(MOSGetGID(),tmpGName)) == NULL)
      {
      fprintf(stderr,"ERROR:  cannot validate group\n");

      return(FAILURE);
      }

    MXMLSetVal(CE,(void *)ptr,mdfString);
    }

  if ((ptr = getcwd(tmpLine,sizeof(tmpLine))) == NULL)
    {
    return(FAILURE);
    }

  if (C.SpecJobID != NULL)
    {
    MXMLSetChild(JE,(char *)MS3JobAttr[ms3jaJobID],&CE);

    MXMLSetVal(CE,(void *)C.SpecJobID,mdfString);
    }

  MXMLSetChild(JE,(char *)MS3JobAttr[ms3jaIWD],&CE);

  MXMLSetVal(CE,(void *)ptr,mdfString);

  /* Get the current umask for this user and send it with the job info. */

  CurrentUMask = umask(0); /* Note - this sets the umask to zero and returns the old value */
  umask(CurrentUMask);     /*      - this resets the umask back to the original value*/

  MXMLSetChild(JE,(char *)MJobAttr[mjaUMask],&CE);

  MXMLSetVal(CE,(void *)&CurrentUMask,mdfInt);

  /* load env */

  /* NYI */

  return(SUCCESS);
  }  /* END MCJobLoadDefaultEnv() */





/**
 *
 *
 * @param JE (I) [modified]
 */

int MCJobLoadRequiredEnv(

  mxml_t *JE)  /* I (modified) */

  {
  char *ptr;
  char  tmpUName[MMAX_NAME];
  char  tmpGName[MMAX_NAME];

  mxml_t *CE;

  if (JE == NULL)
    {
    return(FAILURE);
    }

  /* load user, group */

  if ((ptr = MUUIDToName(MOSGetUID(),tmpUName)) == NULL)
    {
    fprintf(stderr,"ERROR:  cannot validate user\n");

    return(FAILURE);
    }

  MXMLSetChild(JE,(char *)MS3JobAttr[ms3jaUser],&CE);

  if (C.SpecUser != NULL)
    {
    MXMLSetVal(CE,C.SpecUser,mdfString);
    }
  else
    {
    MXMLSetVal(CE,(void *)ptr,mdfString);
    }

  if ((ptr = MUGIDToName(MOSGetGID(),tmpGName)) == NULL)
    {
    fprintf(stderr,"ERROR:  cannot validate group\n");

    return(FAILURE);
    }

  MXMLSetChild(JE,(char *)MS3JobAttr[ms3jaGroup],&CE);

  if (C.SpecGroup != NULL)
    {
    MXMLSetVal(CE,C.SpecGroup,mdfString);
    }
  else
    {
    MXMLSetVal(CE,(void *)ptr,mdfString);
    }

  return(SUCCESS);
  }  /* END MCJobLoadRequiredEnv() */


/**
 * Add a string to the already existing extension string buffer.
 *
 * NOTE: does NOT check for duplicates or repeats
 *
 * @param Attr (I)
 * @param Value (I)
 * @param RMXBuf (O)
 * @param RMXBufSize (I)
 * @param Override (I)
 */

int MCSubmitAddRMXString(

  const char *Attr,       /* I */
  const char *Value,      /* I */
  char       *RMXBuf,     /* O */
  int         RMXBufSize, /* I */
  mbool_t     Override)   /* I */

  {
  enum MXAttrType xattr;
  char tmpRMXBuf[MMAX_BUFFER];
  char tmpBuf[MMAX_BUFFER];

  if (Attr == NULL)
    {
    fprintf(stderr,"ALERT:    empty resource requested\n");

    MDB(1,fS3) MLog("ALERT:    empty resource requested\n");

    return(FAILURE);
    }

  /* changed call to MUGetIndexCI() to not allow substrings - rt4559 dch 4/13/09 */

  if ((xattr = (enum MXAttrType)MUGetIndexCI(Attr,MRMXAttr,FALSE,0)) == 0)
    {
    if (C.AllowUnknownResource == TRUE)
      {
      char tmpLine[MMAX_LINE];

      snprintf(tmpLine,sizeof(tmpLine),"%s=%s",
        Attr,
        (Value != NULL) ? Value : "");

      MUStrAppend(&C.PeerFlags,NULL,tmpLine,',');

      return(SUCCESS);
      }

    fprintf(stderr,"ALERT:    unexpected resource '%s' requested\n",
      Attr);

    MDB(1,fS3) MLog("ALERT:    unexpected resource '%s' requested\n",
      Attr);

    return(FAILURE);
    }

  /* switch to direct specification (without -W) Jan. 2006 */

  /* simply add to RM extension string */

  if ((xattr == mxaStageIn) && (Value[0] != '/'))
    {
    char *cwd = NULL;

    cwd = getcwd(tmpBuf,sizeof(tmpBuf));

    snprintf(tmpRMXBuf,sizeof(tmpRMXBuf),"%s:%s/%s",
      Attr,
      tmpBuf,
      Value);

    MDB(10,fALL) MLog("INFO:     getcwd returned %s\n",cwd);
    }
  else if (xattr != mxaTrig)
    {
    /* need to remove semicolons from the value--semicolons are valid for msub -l but not for x=<ARGS> RM ext. strings. */

    char tmpVal[MMAX_BUFFER];

    MUStrCpy(tmpVal,Value,sizeof(tmpVal));

    MUStrReplaceChar(tmpVal,';',':',FALSE);

    snprintf(tmpRMXBuf,sizeof(tmpRMXBuf),"%s:%s",
      Attr,
      tmpVal);
    }
  else
    {
    snprintf(tmpRMXBuf,sizeof(tmpRMXBuf),"%s:%s",
      Attr,
      Value);
    }

  /* If override is set, remove previous instances */

  if (Override == TRUE)
    MUStrRemoveFromList(RMXBuf,Attr,';',TRUE);

  MUStrAppendStatic(RMXBuf,tmpRMXBuf,';',RMXBufSize);

  return(SUCCESS);
  }  /* END MCSubmitAddRMXString() */

/**
 * build XML Tree for Accelerators
 *
 * @Param   (I/O)
 * @Param   (O)
 * @Param   (I)
 */

void __ResourceListNodes_ProcessAccelerator(

  mxml_t              *ReqE,
  msnl_t              *GenericResList,
  enum MS3ReqAttrEnum  ms3ReqAttrEnum,
  mbool_t              Override)
    
  {
  char const *accelKey = (ms3ReqAttrEnum == ms3rqaGPUS) ? MCONST_GPUS : MCONST_MICS;
  int         GIndex;
  mxml_t     *SE;

  /* Check here because it may not have existed earlier (added when we check for gpus/mics) */

  GIndex = MUMAGetIndex(meGRes,accelKey,mVerify);

  if ((GIndex > 0) && (GIndex < MMAX_GRES))
    {
    /* GPUS/MICS gres exists, check if this req asked for it */

    if (MSNLGetIndexCount(GenericResList,GIndex) > 0)
      {
      int Count = MSNLGetIndexCount(GenericResList,GIndex);

      if ((MXMLGetChild(ReqE,(char *)MS3ReqAttr[ms3ReqAttrEnum],NULL,&SE) == FAILURE))
        {
        SE = NULL;

        MXMLCreateE(&SE,(char *)MS3ReqAttr[ms3ReqAttrEnum]);

        MXMLSetVal(SE,(void *)&Count,mdfInt);

        MXMLAddE(ReqE,SE);
        }
      else if (Override == TRUE)
        {
        MXMLSetVal(SE,(void *)&Count,mdfInt);
        }
      }    /* END if (MSNLGetIndexCount(GenericResList,GIndex) > 0) */
    }    /* END if ((GIndex > 0) && ...) */
  } /* END __ResourceListNodes_ProcessAccelerator() */


/**
 * parse ResourceListNodes()
 *
 */

int MCSubmitProcessArgs_ResourceListNodes(

  mxml_t    *JE,         /* I (modified) */
  mxml_t    *TGE[],
  char      *TokPtr,
  int       *ReqDisk,
  mbool_t    Override,
  mbool_t    IsNCPUs,
  mtrma_t&   TA,
  int        ProcsUsed,
  mbool_t   *NodesUsed,
  char      *EMsg)

  {
  mjob_t  *J = NULL; /* (alloc) */
  mnode_t *N;

  mreq_t  *RQ;

  int      TaskCount = 1;
  int      NodeCount = 0;
  int      rqindex; 
  int      nindex;
  char     TaskString[MMAX_BUFFER];  /* make dynamic (fixme) */
  char     tmpLine[MMAX_LINE];

  int      TPN;

  mxml_t  *SE;
  mxml_t  *NE;
  mxml_t  *ReqE;


  if (ProcsUsed == TRUE)
    {
    /* don't allow both "nodes" and "procs" */

    if (EMsg != NULL)
      {
      snprintf(EMsg,MMAX_LINE,"cannot specify both \"procs\" and \"nodes\" in job submission");
      }

    return(FAILURE);
    }

  if (MJobAllocBase(&J,NULL) == FAILURE)
    {
    if (J != NULL)
      {
      MUFree((char **)&J);
      }

    return(FAILURE);
    }

  if (strlen(TokPtr) > sizeof(TaskString))
    {
    MUFree((char **)&J);

    return(FAILURE);
    }

  *NodesUsed = TRUE;

  MUStrCpy(TaskString,TokPtr,sizeof(TaskString));

  if (MJobGetPBSTaskList(J,TaskString,NULL,FALSE,TRUE,FALSE,FALSE,NULL,NULL) == FAILURE)
      {
      MUFree((char **)&J);

      if (EMsg != NULL)
        {
        snprintf(EMsg,MMAX_LINE,"invalid node specification '%s'",
          TaskString); 
        }

      return(FAILURE);
      }

  if (J->Req[0] == NULL)
    {
    MUFree((char **)&J);

    return(FAILURE);
    }

  if (J->Req[1] != NULL)
    {
    /* multi-req job requested, create master taskgroup element */

    for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
      { 
      RQ = J->Req[rqindex];

      if (RQ == NULL)
        break;

      TGE[rqindex] = NULL;

      MXMLCreateE(&TGE[rqindex],"TaskGroup");

      if (MXMLGetChild(TGE[0],"Requested",NULL,&ReqE) == FAILURE)
        {
        MXMLCreateE(&ReqE,"Requested");

        MXMLAddE(TGE[rqindex],ReqE);
        }

      MXMLAddE(JE,TGE[rqindex]);
      }  /* END for (rqindex) */
    }    /* END if (J->Req[1] != NULL) */
  else
    {
    memset(TGE,0,sizeof(TGE));

    /* use job as master taskgroup element */

    TGE[0] = JE;

    if (MXMLGetChild(TGE[0],"Requested",NULL,&ReqE) == FAILURE)
      {
      MXMLCreateE(&ReqE,"Requested");

      MXMLAddE(TGE[0],ReqE);
      }
    }

  if (!MNLIsEmpty(&J->ReqHList))
    {
    /* send NodeList */

    bmset(&C.SubmitConstraints,mjaHostList);

    SE = NULL;

    MXMLCreateE(&SE,(char *)MS3JobAttr[ms3jaAllocNodeList]);

    MXMLSetVal(SE,(void *)NULL,mdfString);

    /* map to Requested child of TGE */

    if (MXMLGetChild(TGE[J->HLIndex],"Requested",NULL,&ReqE) == SUCCESS)
      MXMLAddE(ReqE,SE);

    for (nindex = 0;MNLGetNodeAtIndex(&J->ReqHList,nindex,&N) == SUCCESS;nindex++)
      {
      NE = NULL;

      MXMLCreateE(&NE,"Node");

      MXMLSetVal(NE,(void *)N->Name,mdfString);

      MXMLAddE(SE,NE);

      /* free temporary nodes */

      MNodeDestroy(&N);
      }  /* END for (nindex = 0;nindex <= MSched.JobMaxNodeCount;nindex++) */
    }    /* END if (J->ReqHList != NULL) */

  for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
    {
    RQ = J->Req[rqindex];

    if (RQ == NULL)
      break;

    if (MXMLGetChild(TGE[rqindex],"Requested",NULL,&ReqE) == FAILURE)
      {
      MXMLCreateE(&ReqE,"Requested");

      MXMLAddE(TGE[rqindex],ReqE);
      }

    /* send Features */

    MUNodeFeaturesToString(',',&RQ->ReqFBM,tmpLine);

    if (!MUStrIsEmpty(tmpLine) && 
        (MXMLGetChild(ReqE,(char *)MS3ReqAttr[ms3rqaReqNodeFeature],NULL,&SE) == FAILURE))
      {
      bmset(&C.SubmitConstraints,mjaReqFeatures);

      SE = NULL;

      MXMLCreateE(&SE,(char *)MS3ReqAttr[ms3rqaReqNodeFeature]);

      MXMLSetVal(SE,(void *)tmpLine,mdfString);

      MXMLAddE(ReqE,SE);
      }
    else if ((tmpLine[0] != '\0') &&
             strcmp(tmpLine,NONE) &&
             (Override == TRUE))
      {
      MXMLSetVal(SE,(void *)tmpLine,mdfString);
      }

    /* send NodeCount */

    NodeCount = J->Request.NC;

    if (NodeCount == 0)
      {
      NodeCount = RQ->NodeCount;
      }

    if ((NodeCount > 0) &&
        (MXMLGetChild(ReqE,(char *)MS3ReqAttr[ms3rqaNCReqMin],NULL,&SE) == FAILURE))
      {
      SE = NULL;

      MXMLCreateE(&SE,(char *)MS3ReqAttr[ms3rqaNCReqMin]);

      MXMLSetVal(SE,(void *)&(NodeCount),mdfInt);

      MXMLAddE(ReqE,SE);
      }
    else if ((NodeCount > 0) && 
             (Override == TRUE))
      {
      MXMLSetVal(SE,(void *)&(NodeCount),mdfInt);
      }
    /* send TaskCount */

    TaskCount = J->Request.TC;

    if (TaskCount == 0)
      {
      TaskCount = RQ->TaskCount;
      }
    else if ((RQ->TaskCount > 0) && 
             (TaskCount != RQ->TaskCount))
      {
      TaskCount = RQ->TaskCount;
      }

    /* what is this IF/ELSE test doing? */
    if ((bmisset(&J->IFlags,mjifPBSGPUSSpecified)) || (bmisset(&J->IFlags,mjifPBSGPUSSpecified)))
      {
      if (RQ->DRes.Procs > 1)
        {
        SE = NULL;

        MXMLCreateE(&SE,(char *)MS3ReqAttr[ms3rqaReqProcPerTask]);
        MXMLSetVal(SE,(void *)&RQ->DRes.Procs,mdfInt);
        MXMLAddE(ReqE,SE);
        }

      if (RQ->TaskCount > 1)
        {
        SE = NULL;

        MXMLCreateE(&SE,(char *)MS3ReqAttr[ms3rqaTCReqMin]);
        MXMLSetVal(SE,(void *)&RQ->TaskCount,mdfInt);
        MXMLAddE(ReqE,SE);
        }
      }
    else if (TaskCount > 0)
      {

      /* storing this in our static TA so we can properly deal
        * with #PBS lines requesting memory/swap/disk in the job
        * script */
      TA.ProcsRequested = TaskCount;

      if ((MXMLGetChild(ReqE,(char *)MS3ReqAttr[ms3rqaTCReqMin],NULL,&SE) == FAILURE))
        {
        SE = NULL;

        if (IsNCPUs == TRUE)
          MXMLCreateE(&SE,(char *)MS3ReqAttr[ms3rqaReqProcPerTask]);
        else
          MXMLCreateE(&SE,(char *)MS3ReqAttr[ms3rqaTCReqMin]);

        MXMLSetVal(SE,(void *)&(TaskCount),mdfInt);

        MXMLAddE(ReqE,SE);
        }
      else if (Override == TRUE)
        {
        MXMLSetVal(SE,(void *)&(TaskCount),mdfInt);
        }
      }

    /* send TasksPerNode */

    TPN = RQ->TasksPerNode;

    C.PPN = TPN;

    if (TPN > 0)
      {
      if ((MXMLGetChild(ReqE,(char *)MS3ReqAttr[ms3rqaTPN],NULL,&SE) == FAILURE))
        {
        SE = NULL;

        MXMLCreateE(&SE,(char *)MS3ReqAttr[ms3rqaTPN]);

        MXMLSetVal(SE,(void *)&TPN,mdfInt);

        MXMLAddE(ReqE,SE);
        }
      else if (Override == TRUE)
        {
        MXMLSetVal(SE,(void *)&TPN,mdfInt);
        }
      }

   /* Process any Accelerators: GPUS and MICS requested for this node,
    * build the XML Tree in ReqE
   */

   __ResourceListNodes_ProcessAccelerator(ReqE,&RQ->DRes.GenericRes,ms3rqaGPUS,Override);
   __ResourceListNodes_ProcessAccelerator(ReqE,&RQ->DRes.GenericRes,ms3rqaMICS,Override);
    }    /* END for (rqindex) */

  MUFree((char **)&J);

  return(SUCCESS);
  }  /* END MCSubmitProcessArgs_ResourceListNodes (case mrmrNodes) */



/**
 * Process the 'l' option - ResourceList parser function
 *
 */

int MCSubmitProcessArgs_ResourceList(

  mxml_t    *JE,         /* I (modified) */
  char      *OptArg,
  char      *SBuffer,    /* I/O (modified) - resulting submit script contents */
  int        SBufSize,   /* I */
  mtrma_t&   TA,
  int       *ReqDisk,
  char       RMXString[],
  int        RMXStringSize,
  mbool_t    Override,
  char      *EMsg)       /* O (optional,minsize=MMAX_LINE) */

  {
  static mbool_t ProcsUsed = FALSE;
  static mbool_t NodesUsed = FALSE;

  int     ReqMem;
  int     ReqSwap;

  mxml_t *SE;

  int   aindex;

  char *ptr1;

  char *TokPtr1;
  char *TokPtr2;

  char tmpString[MMAX_BUFFER];  /* must be large enough to contain all
                                   args in '-l' output simultaneously */

                      /*  ie -l walltime=X,nodes=Y,trig=Z,flags=A,... */

  char tmpOpt[MMAX_LINE];

  mxml_t *ReqE;

  mxml_t *TGE[MMAX_REQ_PER_JOB];

  mbool_t IsNCPUs = FALSE;

  /* FORMAT:  <RES>=<VAL>[,<RES>=<VAL>]... */

  if ((OptArg == NULL) || (OptArg[0] == '\0'))
    {
    fprintf(stderr,"ERROR:  no resources specified in resource request\n");

    return(FAILURE);
    }

  MUStrCpy(tmpString,OptArg,sizeof(tmpString));

  ptr1 = MUStrTokEArrayPlus(tmpString,",",&TokPtr1,FALSE);

  while (ptr1 != NULL)
    {
    ReqE = NULL;

    if (!strchr(ptr1,'='))
      {
      /* show usage */

      fprintf(stderr,"ERROR:  malformed resource request '%s'\n",
        ptr1);

      return(FAILURE);
      }

    if ((MUStrTok(ptr1,"=",&TokPtr2) == NULL) ||
        (strlen(TokPtr2)==0))
      {
      /* malformed '-l' value */

      continue;
      }

    if (!strcmp(ptr1,"procs_bitmap"))
      {
      fprintf(stderr,"ERROR:  procs_bitmap must be submitted using -W x=procs_bitmap...\n");

      return(FAILURE);
      }

    aindex = MUGetIndex(ptr1,MRMResList,FALSE,mrmrNONE);

    switch (aindex)
      {
      case mrmrArch:

        if (MXMLGetChild(JE,"Requested",NULL,&ReqE) == FAILURE)
          {
          MXMLCreateE(&ReqE,"Requested");

          MXMLAddE(JE,ReqE);
          }

        SE = NULL;

        MXMLCreateE(&SE,(char *)MS3ReqAttr[ms3rqaReqArch]);

        MXMLSetVal(SE,(void *)TokPtr2,mdfString);

        MXMLAddE(ReqE,SE);

        break;

      case mrmrCput:

        if (MXMLGetChild(JE,"Requested",NULL,&ReqE) == FAILURE)
          {
          MXMLCreateE(&ReqE,"Requested");

          MXMLAddE(JE,ReqE);
          }

        SE = NULL;

        MXMLCreateE(&SE,(char *)MS3JobAttr[ms3jaCPULimit]);

        MXMLSetVal(SE,(void *)TokPtr2,mdfString);

        MXMLAddE(ReqE,SE);

        break;

      case mrmrFile:

        *ReqDisk = MURSpecToL(TokPtr2,mvmMega,mvmMega);

        break;

      case mrmrFlags:

        if (MCCheckJobFlags(TokPtr2,EMsg) == FAILURE)
          {
          return(FAILURE);
          }
       
        if (MCSubmitAddRMXString(
              ptr1,
              TokPtr2,
              RMXString,  /* O */
              RMXStringSize,
              Override) == FAILURE)
          {
          return(FAILURE);
          }

        break;

      case mrmrHosts:

        memset(TGE,0,sizeof(TGE));

        /* use job as master taskgroup element */

        TGE[0] = JE;

        if (MXMLGetChild(TGE[0],"Requested",NULL,&ReqE) == FAILURE)
          {
          MXMLCreateE(&ReqE,"Requested");

          MXMLAddE(TGE[0],ReqE);
          }

        if (TokPtr2 != NULL)
          {
          mxml_t *NE;

          /* send NodeList */

          SE = NULL;

          if (MXMLGetChild(ReqE,(char *)MS3JobAttr[ms3jaAllocNodeList],NULL,&SE) == FAILURE)
            {
            MXMLCreateE(&SE,(char *)MS3JobAttr[ms3jaAllocNodeList]);

            MXMLSetVal(SE,(void *)NULL,mdfString);

            /* map to Requested child of TGE */

            if (MXMLGetChild(TGE[0],"Requested",NULL,&ReqE) == SUCCESS)
              MXMLAddE(ReqE,SE);

            NE = NULL;

            MXMLCreateE(&NE,"Node");

            MXMLSetVal(NE,(void *)TokPtr2,mdfString);

            MXMLAddE(SE,NE);
            }
          else if (Override == TRUE)
            {
            if (MXMLGetChild(SE,"Node",NULL,&NE) == FAILURE)
              {
              NE = NULL; 
              MXMLCreateE(&NE,"Node");

              MXMLAddE(SE,NE);
              }

            MXMLSetVal(NE,(void *)TokPtr2,mdfString);
            }
          }    /* END if (J->ReqHList != NULL) */

        break;

      case mrmrMem:

        TA.JobMemLimit = MURSpecToL(TokPtr2,mvmMega,mvmMega);

        break;

      case mrmrNCPUs: 

        /* fall through - no break */

        IsNCPUs = TRUE;

      case mrmrNodes:

        {
        int rc = MCSubmitProcessArgs_ResourceListNodes(
                    JE,         /* I (modified) */
                    TGE,
                    TokPtr2,
                    ReqDisk,
                    Override,
                    IsNCPUs,
                    TA,
                    ProcsUsed,
                    &NodesUsed,
                    EMsg);

        if (FAILURE == rc)
          return(rc);

        }

        break;

      case mrmrOpsys:
      case mrmrOS:

        if (MXMLGetChild(JE,"Requested",NULL,&ReqE) == FAILURE)
          {
          MXMLCreateE(&ReqE,"Requested");

          MXMLAddE(JE,ReqE);
          }

        if (MXMLGetChild(ReqE,(char *)MS3ReqAttr[ms3rqaReqOpsys],NULL,&SE) == FAILURE)
          {
          SE = NULL;

          MXMLCreateE(&SE,(char *)MS3ReqAttr[ms3rqaReqOpsys]);

          MXMLSetVal(SE,(void *)TokPtr2,mdfString);

          MXMLAddE(ReqE,SE);
          }
        else if (Override == TRUE)
          {
          MXMLSetVal(SE,(void *)TokPtr2,mdfString);
          }

        break;

      case mrmrPlacement:
        
        {
        /* The msub "placement" option is not an option used for Moab
           but is only intended for Torque.  However, we have to do some
           basic error checking so we do it here.
           (Example usage: sockets=10:numa=10:usethreads) */

        char *head = TokPtr2;
        char *tail = strchr(head,':');

        while (head != NULL)
          {
          /* fail unless it's "sockets", "numa", or "usethreads" */
          if ((strncmp(head,"sockets",strlen("sockets")) != 0) &&
              (strncmp(head,"numa",strlen("numa")) != 0)       &&
              (strncmp(head,"usethreads",strlen("usethreads")) != 0))
            {
            return(FAILURE);
            }

          /* if it's "sockets", or "numa" check for '=' and then
             check for following digits (i.e. sockets=2) */
          if ((!strncmp(head,"sockets",strlen("sockets"))) ||
              (!strncmp(head,"numa",strlen("numa"))))
            {
            /* fail unless it has '=' */
            char *mid = strchr(head,'=');

            if (mid == NULL)
              return(FAILURE);

            /* fail unless followed by digits */
            if ((*(mid + 1) != '\0') && (*(mid + 1) != ':'))
              {
              char valueStr[MMAX_NAME];
              int  count = 0;

              /* grab all the following digits */

              while ((*(mid + 1) != '\0') && (*(mid + 1) != ':'))
                {
                mid++;

                if (!isdigit(*mid))
                  return(FAILURE);

                /* save the digits */

                valueStr[count++] = *mid;
                }
              
              valueStr[count] = '\0';

              /* do not allow 0 as a valid input */
              if (!strcmp(valueStr,"0"))
                {
                if (EMsg != NULL)
                  {
                  sprintf(EMsg,"The placement value cannot be 0.\n");
                  }
                return(FAILURE);
                }

              /* Save off the number of sockets or numa to be checked later
                 against the ppn */
              if (!strncmp(head,"sockets",strlen("sockets")))
                C.Sockets = atoi(valueStr);
              else if (!strncmp(head,"numa",strlen("numa")))
                C.Numa = atoi(valueStr);
              }
            else
              {
              if (EMsg != NULL)
                {
                sprintf(EMsg,"The placement value is missing.\n");
                }
              return(FAILURE);
              }
            }

          /* reset pointers if tail != NULL and loop again */
          if (tail != NULL)
            {
            head = ++tail;
            tail = strchr(head,':');
            }
          else
            head = NULL;              
          }
        }

        break;

      case mrmrPmem:

        if (MXMLGetChild(JE,"Requested",NULL,&ReqE) == FAILURE)
          {
          MXMLCreateE(&ReqE,"Requested");

          MXMLAddE(JE,ReqE);
          }

        ReqMem = MURSpecToL(TokPtr2,mvmMega,mvmMega);

        SE = NULL;
     
        /* check to see if the memory element is already created */
     
        MXMLGetChild(JE,(char *)MS3ReqAttr[ms3rqaReqMemPerTask],NULL,&SE);
     
        /* if not, create it with what we have */
     
        if (SE == NULL)
          {
          MXMLCreateE(&SE,(char *)MS3ReqAttr[ms3rqaReqMemPerTask]);
     
          MXMLSetVal(SE,(void *)&ReqMem,mdfInt);
     
          MXMLAddE(JE,SE);
          }
        else 
          {
          /* if it's already there, reset the Val to the correct ReqMem */
     
          MXMLSetVal(SE,(void*)&ReqMem,mdfInt);
          }

        break;

      case mrmrPvmem:

        if (MXMLGetChild(JE,"Requested",NULL,&ReqE) == FAILURE)
          {
          MXMLCreateE(&ReqE,"Requested");

          MXMLAddE(JE,ReqE);
          }

        ReqSwap = MURSpecToL(TokPtr2,mvmMega,mvmMega);
     
        SE = NULL;
        
        /* check to see if the swap element is already created */
     
        MXMLGetChild(JE,(char *)MS3ReqAttr[ms3rqaReqSwapPerTask],NULL,&SE);
     
        /* if not, create it */
     
        if (SE == NULL)
          {
          MXMLCreateE(&SE,(char *)MS3ReqAttr[ms3rqaReqSwapPerTask]);
     
          MXMLSetVal(SE,(void *)&ReqSwap,mdfInt);
     
          MXMLAddE(JE,SE);
          }
        else
          {
          /* if it's already there, reset the Val to the correct ReqSwap */
     
          MXMLSetVal(SE,(void*)&ReqSwap,mdfInt);
          }

        break;

      case mrmrSoftware:

        if (MXMLGetChild(JE,"Requested",NULL,&ReqE) == FAILURE)
          {
          MXMLCreateE(&ReqE,"Requested");

          MXMLAddE(JE,ReqE);
          }

        if (MXMLGetChild(ReqE,(char *)MS3ReqAttr[ms3rqaReqSoftware],NULL,&SE) == FAILURE)
          {
          SE = NULL;

          MXMLCreateE(&SE,(char *)MS3ReqAttr[ms3rqaReqSoftware]);

          MXMLSetVal(SE,(void *)TokPtr2,mdfString);

          MXMLAddE(ReqE,SE);
          }
        else if (Override == TRUE)
          {
          MXMLSetVal(SE,(void *)TokPtr2,mdfString);
          }

        break;

      case mrmrVMUsagePolicy:

        if (MXMLGetChild(JE,"Requested",NULL,&ReqE) == FAILURE)
          {
          MXMLCreateE(&ReqE,"Requested");

          MXMLAddE(JE,ReqE);
          }

        if (MXMLGetChild(ReqE,(char *)MS3JobAttr[ms3jaVMUsagePolicy],NULL,&SE) == FAILURE)
          {
          SE = NULL;

          MXMLCreateE(&SE,(char *)MS3JobAttr[ms3jaVMUsagePolicy]);

          MXMLSetVal(SE,(void *)TokPtr2,mdfString);

          MXMLAddE(ReqE,SE);
          }
        else if (Override == TRUE)
          {
          MXMLSetVal(SE,(void *)TokPtr2,mdfString);
          }

        break;

      case mrmrVmem:

        TA.JobSwapLimit = MURSpecToL(TokPtr2,mvmMega,mvmMega);

        break;

      case mrmrWalltime:

        {
        long  tmpL;
        char *ptr;

        /* FORMAT:  <X>[-<Y>] */

        ptr = strchr(TokPtr2,'-');

        if ((ptr != NULL) && (ptr != TokPtr2))
          {
          /* walltime range specified */

          *ptr = '\0';

          ptr++;

          if (MCSubmitAddRMXString(
               "minwclimit",
               TokPtr2,
               RMXString,
               RMXStringSize,
               Override) == FAILURE)
            {
            return(FAILURE);
            }

          tmpL = MUTimeFromString(ptr);

          if (tmpL <= 0)
            {
            if (EMsg != NULL)
              {
              snprintf(EMsg,MMAX_LINE,"invalid walltime specifcation '%s'",
                ptr);
              }

            return(FAILURE);
            }
          }
        else
          { 
          tmpL = MUTimeFromString(TokPtr2);

          if (tmpL <= 0)
            {
            if (EMsg != NULL)
              {
              snprintf(EMsg,MMAX_LINE,"invalid walltime specification '%s'",
                TokPtr2); 
              }

            return(FAILURE);
            }
          }

        if ((tmpL >= MCONST_DAYLEN) && (OptArg != NULL) && strchr(OptArg,':'))
          {
          /* NOTE:  DD not supported by PBS/TORQUE -- 
                    switch [[[DD:]HH:]MM:]SS format to long */

          sprintf(strstr(OptArg,"walltime="), "walltime=%ld%s%s",
             tmpL,
             (*TokPtr1 != '\0') ? ",":"",
             TokPtr1);

          }

        /* NOTE:  only single walltime allowed */
        /*        copy walltime to all reqs before sending job to server */

        if (MXMLGetChild(JE,"Requested",NULL,&ReqE) == FAILURE)
          {
          MXMLCreateE(&ReqE,"Requested");

          MXMLAddE(JE,ReqE);
          }

        if (MXMLGetChild(ReqE,(char *)MS3JobAttr[ms3jaReqAWDuration],NULL,&SE) == FAILURE)
          {
          SE = NULL;

          MXMLCreateE(&SE,(char *)MS3JobAttr[ms3jaReqAWDuration]);

          MXMLSetVal(SE,(void *)&tmpL,mdfLong);

          MXMLAddE(ReqE,SE);
          }
        else if (Override == TRUE)
          {
          MXMLSetVal(SE,(void *)&tmpL,mdfLong);
          }
        }    /* END BLOCK (case mrmrWallTime) */

        break;

      case mrmrNice: /* NYI */
      case mrmrPcput: /* NYI */

        /* NYI */

        break;

      case mrmrProcs:

        if (MCSubmitAddRMXString(
              "flags",
              "procspecified",
              RMXString,  /* O */
              RMXStringSize,
              Override) == FAILURE)
          {
          return(FAILURE);
          }

        if (NodesUsed == TRUE)
          {
          /* don't allow both "nodes" and "procs" */

          if (EMsg != NULL)
            {
            snprintf(EMsg,MMAX_LINE,"cannot specify both \"procs\" and \"nodes\" in job submission");
            }

          return(FAILURE);
          }

        ProcsUsed = TRUE;

        if (MXMLGetChild(JE,"Requested",NULL,&ReqE) == FAILURE)
          {
          MXMLCreateE(&ReqE,"Requested");

          MXMLAddE(JE,ReqE);
          }

        if ((MXMLGetChild(ReqE,(char *)MS3ReqAttr[ms3rqaTCReqMin],NULL,&SE) == FAILURE))
          {
          SE = NULL;
  
          MXMLCreateE(&SE,(char *)MS3ReqAttr[ms3rqaTCReqMin]);
  
          MXMLSetVal(SE,(void *)TokPtr2,mdfString);
  
          MXMLAddE(ReqE,SE);
          }
        else if (Override == TRUE)
          {
          MXMLSetVal(SE,(void *)TokPtr2,mdfString);
          }

        break;

      case mrmrTrl:
      case mrmrNONE:
      default:

        {
        enum MXMLOTypeEnum tmpI;

        /* if we have a gres=gpus:x request, fix the XML we'll be
         * sending to the server so the job can run */

        if ((strstr(ptr1,"gres") != NULL) && (strstr(TokPtr2,"gpus") != NULL))
          {
          snprintf(EMsg,MMAX_LINE,"Request '-l gres=gpus:x' not supported. Use 'nodes:ppn:gpus'.");

          return(FAILURE);
          }

        /* if we have a gres=mics:x request, fix the XML we'll be
         * sending to the server so the job can run */
        if ((strstr(ptr1,"gres") != NULL) && (strstr(TokPtr2,"mics") != NULL))
          {
          snprintf(EMsg,MMAX_LINE,"Request '-l gres=mics:x' not supported. Use 'nodes:ppn:mics'.");

          return(FAILURE);
          }


        /* request not explicitly handled - add as RM extension */

        if ((Override == TRUE) && (SBuffer != NULL))
          {
          /* Remove previously added extension attributes. For example
           * there is a -l depend=1 in the script. It gets added to 
           * RMXString which gets added to SBuffer as -W x=depend=1 at 
           * the end of the function. Then if -l depend=2 is listed on 
           * the command line the depend in the -W x=depend should 
           * overwite the previouly added -l depend and the depend stored
           * in -W x=. */

          MJobPBSExtractArg("-W",ptr1,SBuffer,NULL,-1,TRUE);
          }

        if ((Override == TRUE) &&
            (MXMLGetChild(JE,(char *)MS3JobAttr[ms3jaRMXString],NULL,&SE) == SUCCESS))
          {
          char *tmpSEVal = NULL;
          char *tmpPtr;

          /* Overwrite previously added RMXString attributes */

          MUStrDup(&tmpSEVal,SE->Val);

          tmpPtr = tmpSEVal;

          if ((tmpPtr[0] == 'x') && (tmpPtr[1] == '='))
            {
            /* skip the 'x=' part */
            tmpPtr++;
            tmpPtr++;
            }

          MUStrRemoveFromList(tmpPtr,ptr1,';',TRUE);

          MXMLSetVal(SE,(void *)tmpSEVal,mdfString);

          MUFree(&tmpSEVal);
          }

        tmpI = (enum MXMLOTypeEnum)MUGetIndexCI(ptr1,(const char **)MXO,FALSE,0);

        if (MCSubmitAddRMXString(
              ptr1,
              TokPtr2,
              RMXString,  /* O */
              RMXStringSize,
              (tmpI == mxoxDepends) ? FALSE : Override) == FAILURE)
          {
          return(FAILURE);
          }

        break; 
        } /* END default block */
      }  /*  END switch (aindex) */

    snprintf(tmpOpt,sizeof(tmpOpt),"%s=%s",ptr1,TokPtr2);
        
    if ((Override == TRUE) && (SBuffer != NULL))
      {
      /* Only -l attr=value is being added, not -l attr=value,attr=value,
       * so MJobPBSRemoveArg can be used to remove all instances of
       * -l attr */

      MJobPBSRemoveArg("-l",ptr1,SBuffer,SBuffer,SBufSize);
      }

    /* Only insert extensions that torque won't complain about. */

    /* Note that NodeSetIsOptional is still sent with the extensions (just not in the PBS args) */

    if ((strncasecmp(ptr1,MRMXAttr[mxaExcHList],strlen(ptr1)) != 0) &&
        (strncasecmp(ptr1,MRMXAttr[mxaJobRejectPolicy],strlen(ptr1)) != 0) && 
        (strncasecmp(ptr1,MRMXAttr[mxaNodeSetIsOptional],strlen(MRMXAttr[mxaNodeSetIsOptional])) != 0))
      {
      MJobPBSInsertArg("-l",tmpOpt,SBuffer,SBuffer,SBufSize);
      }

    ptr1 = MUStrTokEArrayPlus(NULL,",",&TokPtr1,FALSE);
    }      /* END while (ptr1 != NULL) */

  return(SUCCESS);
  }



/**
 * Process qsub/msub style job submission args.
 *
 * NOTE: build up XML and insert into command file
 * NOTE: used to process both cmdline and cmd script directives
 * NOTE: cmdline args should take precedence over cmd script directives
 *
 * @see MUGetOpt() - child
 *
 * @param JE (I) [modified]
 * @param ArgC (I)
 * @param ArgV (I)
 * @param SBuffer (I/O) - resulting submit script contents [modified]
 * @param SBufSize (I)
 * @param Override (I) - Signals to override previous values in the XML
 * @param QuickPass (I)
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 */

int MCSubmitProcessArgs(

  mxml_t   *JE,         /* I (modified) */
  int      *ArgC,       /* I */
  char    **ArgV,       /* I */
  char     *SBuffer,    /* I/O (modified) - resulting submit script contents */
  int       SBufSize,   /* I */
  mbool_t   Override,   /* I */
  mbool_t   QuickPass,  /* I */
  char     *EMsg)       /* O (optional,minsize=MMAX_LINE) */

  {
  char   *OptArg;
  int     OptTok;

  int     ArgIndex;

  mxml_t *SE;
  mxml_t *AE;

  /* (1/28/09) made static so jobs submitted with #PBS lines in the job script
   * itself will be able to properly set requests per tasks (mem,disk,swap) */

  static mtrma_t  TA;

  int ReqDisk = 0;
  int ReqMem  = 0;
  int ReqSwap = 0;

  mbool_t HasInteractiveScript = FALSE;

  char RMXString[MMAX_BUFFER];

  /* FORMAT: [<ARGS>] [<COMMANDFILE>] */

  /* args:  -a -A -c -C -d -e -E -F -h -I -j -k -K -l -m -M -N -o -p -q -r -s -S -t -u -v -V -W -x -X -z */

  const char *ParamString = "-:a:A:c:C:d:e:EF:hi:Ij:k:Kl:L:m:M:N:o:p:q:r:s:S:t:u:v:VW:x:Xz?";

  const char *FName = "MCSubmitProcessArgs";

  MDB(3,fCONFIG) MLog("%s(%s,%d,%s,SBuffer,%d,%s,%s)\n",
    FName,
    (JE != NULL) ? "JE" : "NULL",
    *ArgC,
    (ArgV != NULL) ? "ArgV" : "NULL",
    SBufSize,
    MBool[Override],
    (EMsg != NULL) ? "EMsg" : "NULL");

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (ArgV == NULL)
    {
    return(FAILURE);
    }

  /* don't need to memset this because statics are automatically initialized to
     zero as per K&R */
  /*memset(&TA,0,sizeof(TA));*/

  RMXString[0] = '\0';

  ArgIndex = 0;

  OptTok = 1;

  while (ArgIndex != -1)
    {
    /* NOTE:  MUGetOpt() will remove arg if acceptable */

    ArgIndex = MUGetOpt(ArgC,ArgV,(char *)ParamString,&OptArg,&OptTok,NULL);

    if (QuickPass == TRUE)
      {
      char tmpString[MMAX_NAME];

      if ((char)ArgIndex == '?')
        {
        if (EMsg != NULL)
          {
          if (OptArg != NULL)
            snprintf(EMsg,MMAX_LINE,"invalid argument '%s'",
              OptArg);
          else
            {
            strcpy(EMsg,"invalid argument");
            }
          }

        return(FAILURE);
        }

      if ((char)ArgIndex == 'I')
        {
        C.IsInteractive = TRUE;
        }

      if ((char)ArgIndex == 'x')
        {
        HasInteractiveScript = TRUE;
        }

      if ((char)ArgIndex == 'C')
        {
        UserDirectiveSpecified = TRUE;

        /* Store the argument for parsing the directives. */
        MUStrCpy(UserDirective,OptArg,sizeof(UserDirective)); 
        }

      if (ArgIndex > 0)
        {
        sprintf(tmpString,"-%c",
          (char)ArgIndex);

        if (SBuffer != NULL)
          MJobPBSInsertArg(tmpString,OptArg,SBuffer,SBuffer,SBufSize);
        }

      continue;
      }  /* END if (QuickPass == TRUE) */
    else
      {
      if ((OptArg != NULL) && (OptArg[0] == '\0'))
        {
        if (((char)ArgIndex) == 'F')
          {
          /* with certain args we should FAIL if they are empty */

          /*NO-OP*/
          }
        else
          {
          /* ignore empty args */

          continue;
          }
        }
      }

    switch ((char)ArgIndex)
      {
      case 'a':

        {
        long tmpL;

        /* release time (FORMAT: [[[[CC]YY]MM]DD]hhmm[.SS]) */

        if (MXMLGetChild(
              JE,
              (char *)MS3JobAttr[ms3jaReqSMinTime],
              NULL,
              &SE) == FAILURE)
          {
          SE = NULL;

          MXMLCreateE(&SE,(char *)MS3JobAttr[ms3jaReqSMinTime]);

          /* convert to epochtime */

          if (MUGetRMTime(OptArg,mrmtPBS,&tmpL) == FAILURE)
            {
            fprintf(stderr,"ERROR:  invalid time specified\n");

            return(FAILURE);
            }

          /* MUStringToE(OptArg,&tmpL,FALSE); */

          MXMLSetVal(SE,(void *)&tmpL,mdfLong);

          MXMLAddE(JE,SE);

          if (SBuffer != NULL)
            MJobPBSInsertArg("-a",OptArg,SBuffer,SBuffer,SBufSize);
          }
        else if (Override == TRUE)
          {
          if (MUGetRMTime(OptArg,mrmtPBS,&tmpL) == FAILURE)
            {
            fprintf(stderr,"ERROR:  invalid time specified\n");

            return(FAILURE);
            }

          MXMLSetVal(SE,(void *)&tmpL,mdfLong);

          if (SBuffer != NULL)
            {
            MJobPBSRemoveArg("-a",NULL,SBuffer,SBuffer,SBufSize);

            MJobPBSInsertArg("-a",OptArg,SBuffer,SBuffer,SBufSize);
            }
          }
        }    /* END BLOCK (case 'a') */

        break;

      case 'A':

        /* account_string */

        if (MXMLGetChild(JE,(char *)MS3JobAttr[ms3jaAccount],NULL,&SE) == FAILURE)
          {
          SE = NULL;

          MXMLCreateE(&SE,(char *)MS3JobAttr[ms3jaAccount]);

          MXMLSetVal(SE,(void *)OptArg,mdfString);

          MXMLAddE(JE,SE);

          if (SBuffer != NULL)
            MJobPBSInsertArg("-A",OptArg,SBuffer,SBuffer,SBufSize);
          }
        else if (Override == TRUE)
          {
          if (SBuffer != NULL)
            {
            MJobPBSRemoveArg("-A",NULL,SBuffer,SBuffer,SBufSize);
            MJobPBSInsertArg("-A",OptArg,SBuffer,SBuffer,SBufSize);
            }

          MXMLSetVal(SE,(void *)OptArg,mdfString);
          }

        break;

      case 'c':

        /* interval */

        if (SBuffer != NULL)
          MJobPBSInsertArg("-c",OptArg,SBuffer,SBuffer,SBufSize);

        break;

      case 'C':

        /* directive_prefix */
        /* Already processed on the quick pass */
        
        /*if (SBuffer != NULL)
          MJobPBSInsertArg("-C",OptArg,SBuffer,SBuffer,SBufSize);*/

        break;

      case 'd':  /* EXTENSION ARG */

        /* set execution directory */

        {
        char        tmpLine[MMAX_LINE];
        char        AbsolutePath[MMAX_LINE];
        char       *ptr;
        char       *TokPtr;
        char       *SubmitDir = NULL;
        mxml_t     *SubmitDirE;

        struct stat st;

        /* FORMAT:  [<SRCDIR>:]<DSTDIR> */

        if (OptArg == NULL)
          break;

        if (strchr(OptArg,':') != NULL)
          {
          MUStrCpy(tmpLine,OptArg,sizeof(OptArg));

          ptr = MUStrTok(tmpLine,":",&TokPtr);
          }
        else
          {
          ptr = OptArg;
          }

        if ((stat(ptr,&st) != 0) || !S_ISDIR(st.st_mode))
          {
          if (EMsg != NULL)
            {
            sprintf(EMsg,"WARNING:  directory %s may be invalid\n",
              ptr);
            }
          }

        if (MXMLGetChild(JE,(char *)MS3JobAttr[ms3jaIWD],NULL,&SE) != FAILURE)
          {
          if (MXMLGetChild(JE,(char *)MS3JobAttr[ms3jaSubmitDir],NULL,&SubmitDirE) == FAILURE)
            {
            /* Copy IWD to SubmitDir because it will be overwritten */
            MUStrDup(&SubmitDir,SE->Val);

            SubmitDirE = NULL;
            MXMLCreateE(&SubmitDirE,(char *)MS3JobAttr[ms3jaSubmitDir]);
            MXMLSetChild(JE,(char *)MS3JobAttr[ms3jaSubmitDir],&SubmitDirE);

            MXMLSetVal(SubmitDirE,(void *)SubmitDir,mdfString);
            }

          /* determine if it is a relative path */

          if (*OptArg != '/')
            {
             /* it's a relative path */
            sprintf(AbsolutePath, "%s/%s", SubmitDir, OptArg);
            MXMLSetVal(SE,(void *)AbsolutePath,mdfString);
            }
          else
            MXMLSetVal(SE,(void *)OptArg,mdfString);
          }
        else
          {
          SE = NULL;

          MXMLCreateE(&SE,(char *)MS3JobAttr[ms3jaIWD]);

          MXMLSetVal(SE,(void *)OptArg,mdfString);

          MXMLAddE(JE,SE);

          if (MXMLGetChild(JE,(char *)MS3JobAttr[ms3jaSubmitDir],NULL,&SubmitDirE) == FAILURE)
            {
            char tmpLine[MMAX_LINE];

            SubmitDirE = NULL;

            MXMLCreateE(&SubmitDirE,(char *)MS3JobAttr[ms3jaSubmitDir]);

            if ((SubmitDir = getcwd(tmpLine,sizeof(tmpLine))) != NULL)
              {
              MXMLSetVal(SubmitDirE,(void *)SubmitDir,mdfString);

              MXMLAddE(JE,SubmitDirE);
              }
            else
              {
              MXMLDestroyE(&SubmitDirE);
              }
            }
          }

        if (SBuffer != NULL)
          {
          if (*OptArg != '/')
            MJobPBSInsertArg("-d",AbsolutePath,SBuffer,SBuffer,SBufSize);
          else
            MJobPBSInsertArg("-d",OptArg,SBuffer,SBuffer,SBufSize);
          }
        }   /* END BLOCK 'd' */

        break;

      case 'e':

        /* stderr path */

        if (MXMLGetChild(JE,(char *)MS3JobAttr[ms3jaStdErr],NULL,&SE) == FAILURE)
          {
          SE = NULL;

          MXMLCreateE(&SE,(char *)MS3JobAttr[ms3jaStdErr]);

          MXMLSetVal(SE,(void *)OptArg,mdfString);

          MXMLAddE(JE,SE);

          if (SBuffer != NULL)
            MJobPBSInsertArg("-e",OptArg,SBuffer,SBuffer,SBufSize);
          }
        else if (Override == TRUE)
          {
          if (SBuffer != NULL)
            {
            MJobPBSRemoveArg("-e",NULL,SBuffer,SBuffer,SBufSize);
            MJobPBSInsertArg("-e",OptArg,SBuffer,SBuffer,SBufSize);
            }

          MXMLSetVal(SE,(void *)OptArg,mdfString);
          }

        break;

      case 'E':

        /* moab environment variables */

        if (MCSubmitAddRMXString(
              (char *)MRMXAttr[mxaEnvRequested],
              (char *)MBool[TRUE],
              RMXString,
              sizeof(RMXString),
              Override) == FAILURE)
          {
          return(FAILURE);
          }

        break;

      case 'F':

        if ((OptArg == NULL) || (OptArg[0] == '\0') || (OptArg[0] != '\"'))
          {
          return(FAILURE);
          }

        /* script flags - flags to pass to scripts J->E.Args */

        if (MXMLGetChild(JE,(char *)MS3JobAttr[ms3jaArgs],NULL,&SE) == FAILURE)
          {
          SE = NULL;

          MXMLCreateE(&SE,(char *)MS3JobAttr[ms3jaArgs]);

          MXMLSetVal(SE,(void *)OptArg,mdfString);

          MXMLAddE(JE,SE);

          if (SBuffer != NULL)
            MJobPBSInsertArg("-F",OptArg,SBuffer,SBuffer,SBufSize);
          }
        else if (Override == TRUE)
          {
          MXMLSetVal(SE,(void *)OptArg,mdfString);
          }

        break;

      case 'h':

        /* hold job */

        /* NOTE:  hold only used wrt global queue */

        if (MXMLGetChild(JE,(char *)MS3JobAttr[ms3jaHold],NULL,&SE) == FAILURE)
          {
          SE = NULL;

          MXMLCreateE(&SE,(char *)MS3JobAttr[ms3jaHold]);

          MXMLSetVal(SE,(void *)"User",mdfString);

          MXMLAddE(JE,SE);

          if (SBuffer != NULL)
            MJobPBSInsertArg("-h",OptArg,SBuffer,SBuffer,SBufSize);
          }
        else if (Override == TRUE)
          {
          MXMLSetVal(SE,(void *)"User",mdfString);
          }
        break;

      case 'i':

        /* stdin path */

        if (MXMLGetChild(JE,(char *)MS3JobAttr[ms3jaStdIn],NULL,&SE) == FAILURE)
          {
          SE = NULL;

          MXMLCreateE(&SE,(char *)MS3JobAttr[ms3jaStdIn]);

          MXMLSetVal(SE,(void *)OptArg,mdfString);

          MXMLAddE(JE,SE);

          if (SBuffer != NULL)
            MJobPBSInsertArg("-i",OptArg,SBuffer,SBuffer,SBufSize);
          }
        else if (Override == TRUE)
          {
          MXMLSetVal(SE,(void *)OptArg,mdfString);
          }

        break;

      case 'I':

        /* interactive job */

        C.IsInteractive = TRUE;

        if (MXMLGetChild(JE,(char *)MS3JobAttr[ms3jaIsInteractive],NULL,&SE) == FAILURE)
          {
          SE = NULL;

          MXMLCreateE(&SE,(char *)MS3JobAttr[ms3jaIsInteractive]);

          MXMLSetVal(SE,(void *)"TRUE",mdfString);

          MXMLAddE(JE,SE);

          if (SBuffer != NULL)
            MJobPBSInsertArg("-I",OptArg,SBuffer,SBuffer,SBufSize);

          /* If the Submit language has not already been set, then set it */

          if (MXMLGetChild(JE,(char *)MS3JobAttr[ms3jaSubmitLanguage],NULL,&SE) == FAILURE)
            {
            MXMLCreateE(&SE,(char *)MS3JobAttr[ms3jaSubmitLanguage]);

            MXMLSetVal(SE,(void *)MRMType[MDefRMLanguage],mdfString);

            MXMLAddE(JE,SE);
            }
          }

        break;

      case 'j':

        /* join */

        if (SBuffer != NULL)
          MJobPBSInsertArg("-j",OptArg,SBuffer,SBuffer,SBufSize);

        break;

      case 't':

        {
        int sindex = 0;
        mbool_t EndBracket = FALSE;
        char HostName[MMAX_NAME];

        char *tmpName = NULL;
        char *tmpString = NULL;
        char *TokPtr = NULL;

        SE = NULL;

        if (OptArg == NULL)
           break;

        if (!strcmp(OptArg,"1"))
          {
          /* this is not an array -- process as a normal job */
          ArgC -= 2;
          return(SUCCESS);
          }

        /* job array */

        mstring_t arrayString(MMAX_LINE);
        mstring_t tmpOptArg(MMAX_LINE);

        tmpOptArg = OptArg;

        /* normalize the string in case array was entered in torque format */
        /* if there are no brackets (as in torque), add them for further processing*/
        if (!strstr(tmpOptArg.c_str(), "["))
          {
          /* find the first digit */
          while (!isdigit(tmpOptArg[sindex]))
            {
            MStringAppendF(&arrayString,"%c",tmpOptArg[sindex]);
            sindex++;
            }

          /* insert a bracket before the first digit */
          MStringAppend(&arrayString,"[");

          while (tmpOptArg[sindex] != '\0')
            {
            switch(tmpOptArg[sindex])
              {
              case '%':
                if ((strlen(tmpOptArg.c_str()) - sindex) <= 3)
                  {
                  MStringAppend(&arrayString,"]");
                  EndBracket = TRUE;
                  }

                MStringAppendF(&arrayString,"%c",tmpOptArg[sindex]);
                sindex++;

                /* number follows % sign */
                MStringAppendF(&arrayString,"%c",tmpOptArg[sindex]);
                sindex++;

                break;
              case ':':
                MStringAppendF(&arrayString,"%c",tmpOptArg[sindex]);
                sindex++;

                /* number follows the colon */
                MStringAppendF(&arrayString,"%c",tmpOptArg[sindex]);
                sindex++;

                break;
              default:
                MStringAppendF(&arrayString,"%c",tmpOptArg[sindex]);
                sindex++;

                break;
              }
            }

          if (EndBracket == FALSE)
            MStringAppend(&arrayString,"]");

          /* give it a name if one doesn't exist and make the brackets permanent */
          if (arrayString[0] == '[')
            {
            strcpy(HostName,MSched.Name);

            if (HostName[0] != '\0')
              MStringSet(&tmpOptArg,HostName);
            else
              MStringSet(&tmpOptArg,"moab.");

            MStringAppend(&tmpOptArg,arrayString.c_str());
            }
          else
            {
            MStringAppend(&tmpOptArg,arrayString.c_str());
            }
          }

        /* initialize sindex */
        sindex = 0;

        arrayString.clear();  /* Reset the string */

        if (tmpOptArg[0] == '[')
          {
          /* user didn't enter a name for the job, so name it */
          strcpy(HostName,MSched.Name);

          if (HostName[0] != '\0')
            MStringSet(&arrayString,HostName);
          else
            MStringSet(&arrayString,"moab.");

          MStringAppend(&arrayString,tmpOptArg.c_str());
          }
        else
          {
          MStringAppend(&arrayString,tmpOptArg.c_str());
          }

        if (MCSubmitAddRMXString((char *)MRMXAttr[mxaArray],arrayString.c_str(),RMXString,sizeof(RMXString),Override) == FAILURE)
          {
          return(FAILURE);
          }

        /* add the JobName if it doesn't already have one */

        if (MXMLFind(JE,(char *)MJobAttr[mjaJobName],-1,NULL) == FAILURE)
          {
          MUStrDup(&tmpString,arrayString.c_str());
  
          /* name looks like: jobname.[1-4] so get the first part and use it for the name */
          tmpName = MUStrTok(tmpString,"[",&TokPtr);
          if (tmpName != NULL)
            {
            /* add the job name */
            MXMLCreateE(&SE,(char *)MS3JobAttr[ms3jaJobName]);
  
            /* remove period if there is one */
            if (tmpName[strlen(tmpName)-1] == '.')
              tmpName[strlen(tmpName)-1] = '\0';
  
            MXMLSetVal(SE,(void *)tmpName,mdfString);
            MXMLAddE(JE,SE);
            }
  
          MUFree(&tmpString);
          }

        /* check for valid arguments in the string*/

        /* reset this index */
        sindex = 0;

        /* go to the first bracket */
        while (arrayString[sindex] != '[')
          {
          sindex++;
          }

        /* get past the first bracket */
        sindex++;

        /* check for invalid characters */
        for (; arrayString[sindex] != '\0'; sindex++)
          {
          switch(arrayString[sindex])
            {
            case ' ':
            case ',':
            case '%':
            case ']':
            case ':':
              /* do nothing */
              break;
            case '-':
              /* dash not allowed after the first bracket */
              if (arrayString[sindex - 1] == '[')
                {
                if (EMsg != NULL)
                  {
                  sprintf(EMsg,"ERROR:  invalid characters in string");
                  }

                return(FAILURE); 
                }
              break;
            default:
              if (!isdigit(arrayString[sindex]))
                {
                if (EMsg != NULL)
                  {
                  sprintf(EMsg,"ERROR: invalid characters in string %c", arrayString[sindex]);
                  }

                return(FAILURE);
                }
            }
          }
        }

        break;

      case 'k':

        /* keep */

        /* currently not processed, but passed in RMSubmitScript
           (i.e., not translated) */

        if (SBuffer != NULL)
          MJobPBSInsertArg("-k",OptArg,SBuffer,SBuffer,SBufSize);

        break;

      case 'K':
        
        /* Keep Alive */

        C.KeepAlive = TRUE;

        break;

      case 'l':

        /* resource_list */

        /* insert everything into 'Requested' XML */

        {
        int rc = MCSubmitProcessArgs_ResourceList(
                    JE,
                    OptArg,
                    SBuffer,
                    SBufSize,
                    TA,
                    &ReqDisk,
                    RMXString,
                    sizeof(RMXString),
                    Override,
                    EMsg);

        if (FAILURE == rc)
          return(FAILURE);
        }

        break;

      case 'm':

        {
        char tmpLine[MMAX_LINE];

        /* mail_options */

        if (OptArg == NULL)
          break;

        if (SBuffer != NULL)
          MJobPBSInsertArg("-m",OptArg,SBuffer,SBuffer,SBufSize);

        tmpLine[0] = '\0';

        if (strchr(OptArg,'a') || strchr(OptArg,'A'))
          MUStrAppendStatic(tmpLine,"JobFail",',',sizeof(tmpLine));
  
        if (strchr(OptArg,'b') || strchr(OptArg,'B'))
          MUStrAppendStatic(tmpLine,"JobStart",',',sizeof(tmpLine));

        if (strchr(OptArg,'e') || strchr(OptArg,'E'))
          MUStrAppendStatic(tmpLine,"JobEnd",',',sizeof(tmpLine));

        if (tmpLine[0] == '\0')
          {
          MDB(1,fS3) MLog("ALERT:    invalid mail option '%s' requested\n",
            OptArg);
          }

        if (MXMLGetChild(JE,(char *)MS3JobAttr[ms3jaNotification],NULL,&SE) == FAILURE)
          {
          SE = NULL;

          MXMLCreateE(&SE,(char *)MS3JobAttr[ms3jaNotification]);

          MXMLSetVal(SE,(void *)tmpLine,mdfString);

          MXMLAddE(JE,SE);
          }
        else if ((Override == TRUE) || (SE->Val == NULL))
          {
          MXMLSetVal(SE,(void *)tmpLine,mdfString);
          }
        }    /* END BLOCK (case 'm') */

        break;

      case 'M':

        /* mail destination */

        if (SBuffer != NULL)
          MJobPBSInsertArg("-M",OptArg,SBuffer,SBuffer,SBufSize);

        if (MXMLGetChild(JE,(char *)MS3JobAttr[ms3jaNotification],NULL,&SE) == FAILURE)
          {
          SE = NULL;

          MXMLCreateE(&SE,(char *)MS3JobAttr[ms3jaNotification]);

          MXMLSetAttr(SE,"URI",(void *)OptArg,mdfString);

          MXMLAddE(JE,SE);

          if (SBuffer != NULL)
            MJobPBSInsertArg("-M",OptArg,SBuffer,SBuffer,SBufSize);
          }
        else if (MXMLGetAttr(SE,"URI",NULL,NULL,-1) == FAILURE)
          {
          MXMLSetAttr(SE,"URI",(void *)OptArg,mdfString);

          if (SBuffer != NULL)
            MJobPBSInsertArg("-M",OptArg,SBuffer,SBuffer,SBufSize);
          }
        else if (Override == TRUE)
          {
          MXMLSetAttr(SE,"URI",(void *)OptArg,mdfString);

          if (SBuffer != NULL)
            {
            MJobPBSRemoveArg("-M",NULL,SBuffer,SBuffer,SBufSize);

            MJobPBSInsertArg("-M",OptArg,SBuffer,SBuffer,SBufSize);
            }
          }

        break;

      case 'N':

        /* job name */

        if ((OptArg == NULL) || (OptArg[0] == '\0'))
          {
          fprintf(stderr,"ERROR:  '-N' missing job name\n");

          return(FAILURE);
          }

        if (strchr(OptArg,' ') != NULL)
          {
          fprintf(stderr,"ERROR:  Spaces in job name not allowed\n");

          return(FAILURE);
          }

        if (MXMLGetChild(JE,(char *)MS3JobAttr[ms3jaJobName],NULL,&SE) == FAILURE)
          {
          char *ptr;
          char *TokPtr;

          SE = NULL;

          MXMLCreateE(&SE,(char *)MS3JobAttr[ms3jaJobName]);

          /* strip out XML-unfriendly characters */

          ptr = MUStrTok(OptArg,"\"<>&",&TokPtr);

          MXMLSetVal(SE,(void *)ptr,mdfString);

          MXMLAddE(JE,SE);

          if (SBuffer != NULL)
            MJobPBSInsertArg("-N",OptArg,SBuffer,SBuffer,SBufSize);
          }
        else if (Override == TRUE)
          {
          char *ptr;
          char *TokPtr;
          ptr = MUStrTok(OptArg,"\"<>&",&TokPtr);

          if (SBuffer != NULL)
            {
            MJobPBSRemoveArg("-N",NULL,SBuffer,SBuffer,SBufSize);
            MJobPBSInsertArg("-N",OptArg,SBuffer,SBuffer,SBufSize);
            }

          MXMLSetVal(SE,(void *)ptr,mdfString);
          }

        break;

      case 'o':

        /* stdout path */

        if (MXMLGetChild(JE,(char *)MS3JobAttr[ms3jaStdOut],NULL,&SE) == FAILURE)
          {
          SE = NULL;

          MXMLCreateE(&SE,(char *)MS3JobAttr[ms3jaStdOut]);

          MXMLSetVal(SE,(void *)OptArg,mdfString);

          MXMLAddE(JE,SE);

          if (SBuffer != NULL)
            MJobPBSInsertArg("-o",OptArg,SBuffer,SBuffer,SBufSize);
          }
        else if (Override == TRUE)
          {
          if (SBuffer != NULL)
            {
            MJobPBSRemoveArg("-o",NULL,SBuffer,SBuffer,SBufSize);
            MJobPBSInsertArg("-o",OptArg,SBuffer,SBuffer,SBufSize);
            }

          MXMLSetVal(SE,(void *)OptArg,mdfString);
          }
        /*
        else
          {
          MXMLSetVal(SE,(void *)OptArg,mdfString);
          }
        */

        break;

      case 'p':

        {
        int tmpI;
        int MaxUserPrio = 0;

        /* priority */

        if ((OptArg == NULL) || (OptArg[0] == '\0'))
          {
          fprintf(stderr,"ERROR:  missing priority argument.\n");

          return(FAILURE);
          }

        tmpI = (int)strtol(OptArg,NULL,10);

        MaxUserPrio = ((bmisset(&C.DisplayFlags,mdspfEnablePosPrio)) ? 
          MDEF_MAXUSERPRIO : 
          0);

        /* NOTE: (OptArg[0] != '0') should not be necessary...remove after testing */

        if ((tmpI == 0) && ((errno == EINVAL) || (OptArg[0] != '0')))
          {
          fprintf(stderr,"ERROR:  invalid priority '%s' specified (use integer in range %d to %d, inclusive)\n",
            OptArg,
            MDEF_MINUSERPRIO,
            MaxUserPrio);

          return(FAILURE);
          }
   
        if ((tmpI > MaxUserPrio) ||
            (tmpI < MDEF_MINUSERPRIO))
          {
          fprintf(stderr,"ERROR:  invalid priority value '%d' specified (use integer in range %d to %d, inclusive)\n",
            tmpI,
            MDEF_MINUSERPRIO,
            MaxUserPrio);

          return(FAILURE);
          }

        /* will not overwrite existing user priorities in XML/job script */

        if (MXMLGetChild(JE,(char *)MS3JobAttr[ms3jaUserPrio],NULL,&SE) == FAILURE)
          {
          SE = NULL;

          MXMLCreateE(&SE,(char *)MS3JobAttr[ms3jaUserPrio]);

          MXMLSetVal(SE,(void *)OptArg,mdfString);

          MXMLAddE(JE,SE);

          if (SBuffer != NULL)
            MJobPBSInsertArg("-p",OptArg,SBuffer,SBuffer,SBufSize);
          }
        else if (Override == TRUE)
          {
          MXMLSetVal(SE,(void *)OptArg,mdfString);
          }
        }    /* END BLOCK (case 'p') */

        break;

      case 'q':

        if ((OptArg == NULL) || (OptArg[0] == '\0'))
          {
          fprintf(stderr,"ALERT:    no queue specified\n");

          MDB(1,fS3) MLog("ALERT:    no queue specified\n");

          return(FAILURE);
          }

        /* destination queue mrqaReqClass */

        if (MXMLGetChild(JE,(char *)MS3JobAttr[ms3jaClass],NULL,&SE) == FAILURE)
          {
          char *CPtr;    /* class pointer */
          char *PPtr;    /* partition pointer */
          char *TTokPtr;

          /* FORMAT:  <QUEUE>[@<PARTITION>] */

          SE = NULL;

          MXMLCreateE(&SE,(char *)MS3JobAttr[ms3jaClass]);

          CPtr = MUStrTok(OptArg,"@",&TTokPtr);

          MXMLSetVal(SE,(void *)CPtr,mdfString);

          MXMLAddE(JE,SE);

          if (SBuffer != NULL)
            MJobPBSInsertArg("-q",CPtr,SBuffer,SBuffer,SBufSize);

          PPtr = MUStrTok(NULL,"@",&TTokPtr);

          if (PPtr != NULL)
            {
            char tmpLine[MMAX_LINE];

            /* partition specified */

            snprintf(tmpLine,sizeof(tmpLine),"%s:%s",
              "partition",
              PPtr);

            MUStrAppendStatic(RMXString,tmpLine,';',sizeof(RMXString));
            }
          }    /* END if (MXMLGetChild() == FAILURE) */
        else if (Override == TRUE)
          {
          char *CPtr;    /* class pointer */
          char *TTokPtr;
          CPtr = MUStrTok(OptArg,"@",&TTokPtr);

          MXMLSetVal(SE,(void *)CPtr,mdfString);
          }

        /* in the case that we have a command-line -q request _and_ a job
         * script -q requtest, we want to remove the job script request.
         * this will prevent us from having multiple walltime requests in
         * the command buffer, and should leave the command-line request
         * intact */
        
        if (SBuffer != NULL)
          {
          MJobPBSRemoveArg("-q",NULL,SBuffer,SBuffer,SBufSize);

          MJobPBSInsertArg("-q",OptArg,SBuffer,SBuffer,SBufSize);
          }

        break;

      case 'r':

        /* re-runable */

        if (SBuffer != NULL)
          {
          MJobPBSRemoveArg("-r",NULL,SBuffer,SBuffer,SBufSize);

          MJobPBSInsertArg("-r",OptArg,SBuffer,SBuffer,SBufSize);
          }

        break;

      case 's':

        /* system job */

        if (MXMLGetChild(JE,(char *)MS3JobAttr[ms3jaServiceJob],NULL,&SE) == FAILURE)
          {
          SE = NULL;

          MXMLCreateE(&SE,(char *)MS3JobAttr[ms3jaServiceJob]);

          MXMLSetVal(SE,(void *)OptArg,mdfString);

          MXMLAddE(JE,SE);
          }

        if (SBuffer != NULL)
          MJobPBSInsertArg("-s",OptArg,SBuffer,SBuffer,SBufSize);

        break;

      case 'S':

        /* shell path_list */

        if (MXMLGetChild(JE,(char *)MS3JobAttr[ms3jaShell],NULL,&SE) == FAILURE)
          {
          SE = NULL;

          MXMLCreateE(&SE,(char *)MS3JobAttr[ms3jaShell]);

          MXMLSetVal(SE,(void *)OptArg,mdfString);

          MXMLAddE(JE,SE);

          if (SBuffer != NULL)
            {
            MJobPBSInsertArg("-S",OptArg,SBuffer,SBuffer,SBufSize);
            }
          }
        else if (Override == TRUE)
          {
          MXMLSetVal(SE,(void *)OptArg,mdfString);

          if (SBuffer != NULL)
            {
            MJobPBSRemoveArg("-S",NULL,SBuffer,SBuffer,SBufSize);

            MJobPBSInsertArg("-S",OptArg,SBuffer,SBuffer,SBufSize);
            }
          }

        break;

      case 'u':

        /* user_list */

        if (MXMLGetChild(JE,(char *)MS3JobAttr[ms3jaUser],NULL,&SE) == FAILURE)
          {
          SE = NULL;

          MXMLCreateE(&SE,(char *)MS3JobAttr[ms3jaUser]);
 
          MXMLSetVal(SE,(void *)OptArg,mdfString);
 
          MXMLAddE(JE,SE);

          if (SBuffer != NULL)
            {
            MJobPBSInsertArg("-u",OptArg,SBuffer,SBuffer,SBufSize);
            }
          }
        else if (Override == TRUE)
          {
          MXMLSetVal(SE,(void *)OptArg,mdfString);

          if (SBuffer != NULL)
            {
            MJobPBSRemoveArg("-u",NULL,SBuffer,SBuffer,SBufSize);

            MJobPBSInsertArg("-u",OptArg,SBuffer,SBuffer,SBufSize);
            }
          }

        break;

      case 'v':
        
        {
        int     ctok = -1;
        char    tmpOp[MMAX_LINE];
        char    OptArgFixedQuotes[MMAX_LINE] = {0};
        mbool_t PreviousEnvFound = FALSE;

        /* -v and -V both use mjaEnv as the xml child. -V is distinguised from
         * -v by setting the attr MSAN[msanOp] with value MObjOpType[mSet]
         * which signals to the server that the job specified the whole env 
         * (mjifEnvRequested in MS3JobSetAttr). 
         *
         * -v should not clobber the -V mjaEnv element and only override a
         *  previuos -v mjaEnv element. */

        while (MXMLGetChild(JE,MS3JobAttr[ms3jaEnv],&ctok,&SE) == SUCCESS)
          {
          if (MXMLGetAttr(SE,MSAN[msanOp],NULL,tmpOp,sizeof(tmpOp)) == FAILURE)
            {
            PreviousEnvFound = TRUE;

            break;
            }
          }

        if (PreviousEnvFound == FALSE)
          {
          char tmpLine[MMAX_BUFFER << 1];

          char *ptr;
          char *TokPtr;

          char *BPtr;
          int   BSpace;

          /* variable list */

          /* FORMAT:  <VAR>[=<VAL>][,<VAR>[=<VAL>]]... */

          /* push specified value into buffer before parsing */

          if (SBuffer != NULL)
            {
            if ((strchr(OptArg,'\'') != NULL) ||
                (strchr(OptArg,'"') != NULL))
              {
              /* Restore quotes to variables that contain commas.  See
                 function description for further details */
              MCRestoreQuotes(OptArgFixedQuotes,OptArg);
              OptArg = OptArgFixedQuotes;
              }

            MJobPBSInsertArg("-v",OptArg,SBuffer,SBuffer,SBufSize);
            }

          SE = NULL;

          MXMLCreateE(&SE,(char *)MS3JobAttr[ms3jaEnv]);

          MUSNInit(&BPtr,&BSpace,tmpLine,sizeof(tmpLine));

          ptr = MUStrTokEPlus(OptArg,",",&TokPtr);

          /* NOTE:  must only push variables without assigned values onto job env list */
              
          /* note that ENVRS_SYMBOLIC_STR is used as a delimiter instead of ';' to avoid problems with embedded ';' in the variable */

          while (ptr != NULL)
            {
            if (!strchr(ptr,'='))
              {
              char *vptr = getenv(ptr);

              MUSNPrintF(&BPtr,&BSpace,"%s=%s" ENVRS_SYMBOLIC_STR,
                ptr,
                (vptr != NULL) ? vptr : "");
              }
            else
              {
              MUSNPrintF(&BPtr,&BSpace,"%s" ENVRS_SYMBOLIC_STR,
                ptr);
              }

            MUArrayListAppendPtr(&C.SpecEnv,strdup(ptr));

            ptr = MUStrTokEPlus(NULL,",",&TokPtr);
            }  /* END while (ptr != NULL) */

          MXMLSetVal(SE,(void *)tmpLine,mdfString);

          MXMLAddE(JE,SE);
          }  /* END if (MXMLGetChild(JE) == FAILURE) */
        else if (Override == TRUE)
          {
          char tmpLine[MMAX_LINE];

          char *ptr;
          char *TokPtr;

          char *BPtr;
          int   BSpace;

          MUSNInit(&BPtr,&BSpace,tmpLine,sizeof(tmpLine));

          ptr = MUStrTokEPlus(OptArg,",",&TokPtr);

          /* Free the pre-existing -v variables and create the new ones. 
           * -V variables will be added when salloc is exec'ed' in MCSubmit */

          MUArrayListFreePtrElements(&C.SpecEnv);
          MUArrayListFree(&C.SpecEnv);
          MUArrayListCreate(&C.SpecEnv,sizeof(char *),-1);
          
          /* NOTE:  must only push variables without assigned values onto job env list */

          while (ptr != NULL)
            {
            if (!strchr(ptr,'='))
              {
              char *vptr = getenv(ptr);

              MUSNPrintF(&BPtr,&BSpace,"%s=%s" ENVRS_SYMBOLIC_STR,
                ptr,
                (vptr != NULL) ? vptr : "");
              }
            else
              {
              MUSNPrintF(&BPtr,&BSpace,"%s" ENVRS_SYMBOLIC_STR,
                ptr);
              }

            MUArrayListAppendPtr(&C.SpecEnv,strdup(ptr));

            ptr = MUStrTokEPlus(NULL,",",&TokPtr);
            }  /* END while (ptr != NULL) */

          if (SBuffer != NULL)
            {
            MJobPBSRemoveArg("-v",NULL,SBuffer,SBuffer,SBufSize);

            MJobPBSInsertArg("-v",OptArg,SBuffer,SBuffer,SBufSize);
            }

          MXMLSetVal(SE,(void *)tmpLine,mdfString);
          } /* END else if (Override == TRUE) */
        } /* END Block */

        break;

      case 'V':

        {
        /* full environment */

        int     ctok = -1;
        char    tmpOp[MMAX_LINE];
        mbool_t PreviousEnvFound = FALSE;

        /* -v and -V both use mjaEnv as the xml child. -V is distinguised from
         * -v by setting the attr MSAN[msanOp] with value MObjOpType[mSet]
         * which signals to the server that the job specified the whole env 
         * (mjifEnvRequested in MS3JobSetAttr). 
         *
         * -V env variables should only be added once. */

        while (MXMLGetChild(JE,MS3JobAttr[ms3jaEnv],&ctok,&SE) == SUCCESS)
          {
          if ((MXMLGetAttr(SE,MSAN[msanOp],NULL,tmpOp,sizeof(tmpOp)) == SUCCESS) &&
              (strcmp(tmpOp,MObjOpType[mSet]) == 0))
            {
            PreviousEnvFound = TRUE;

            break;
            }
          }

        if (PreviousEnvFound == FALSE)
          {
          if (SBuffer != NULL)
            MJobPBSInsertArg("-V",NULL,SBuffer,SBuffer,SBufSize);

          if (C.EnvP != NULL)
            {
            char *BPtr;
            int   BSpace;

            char  tmpBuf[MMAX_BUFFER << 1];

            int   eindex;

            MUSNInit(&BPtr,&BSpace,tmpBuf,sizeof(tmpBuf));
          
            /* NOTE: must support ';' and ''' embedded in variable values (NYI) */

            /* EnvP[] format:  <VAR>[=<VALUE>] */

            for (eindex = 0;C.EnvP[eindex] != NULL;eindex++)
              {
              if (eindex > 0)
                {
                /* note that ENVRS_SYMBOLIC_STR is used as a delimiter instead of ';' to avoid problems with embedded ';' in the variable */

                MUSNPrintF(&BPtr,&BSpace,ENVRS_SYMBOLIC_STR "%s",
                  C.EnvP[eindex]);
                }
              else
                {
                MUSNPrintF(&BPtr,&BSpace,"%s",
                  C.EnvP[eindex]);
                }
              }    /* END for (eindex) */

            /* always create a new <Environment> tag because previous ones may
              be op=incr */

            SE = NULL;

            MXMLCreateE(&SE,(char *)MS3JobAttr[ms3jaEnv]);

            MXMLSetVal(SE,(void *)tmpBuf,mdfString);

            MXMLSetAttr(SE,MSAN[msanOp],(void *)MObjOpType[mSet],mdfString);

            MXMLAddE(JE,SE);

            C.FullEnvRequested = TRUE;
            }  /* END if (C.EnvP != NULL) */
          }  /* END if (PreviousEnvFound == FALSE) */
        }  /* END BLOCK (case 'V') */

        break;

      case 'W':

        /* do not process, just send it across */

        if ((OptArg != NULL) && (OptArg[0] == 'x'))
          {
          MUStrAppendStatic(RMXString,&OptArg[2],';',sizeof(RMXString));
          }
        else
          {
          /* NOTE: in the future we may need to read in non-rm ext. string values and process them
           * more intelligently */

          MJobPBSInsertArg("-W",OptArg,SBuffer,SBuffer,SBufSize);
          }

        break;

      case 'x':

        if (SBuffer != NULL)
          {
          MJobPBSRemoveArg("-x",NULL,SBuffer,SBuffer,SBufSize);

          MJobPBSInsertArg("-x",OptArg,SBuffer,SBuffer,SBufSize);
          }

        if (MXMLGetChild(JE,(char *)MS3JobAttr[ms3jaInteractiveScript],NULL,&SE) == FAILURE)
          {
          SE = NULL;

          MXMLCreateE(&SE,(char *)MS3JobAttr[ms3jaInteractiveScript]);

          MXMLSetVal(SE,(void *)OptArg,mdfString);

          MXMLAddE(JE,SE);
          }

        break;

      case 'X':

        /* do not process, just send it across */

        /* NOTE: in the future we may need to read in non-rm ext. string values and process them
         * more intelligently */

        MJobPBSInsertArg("-X",NULL,SBuffer,SBuffer,SBufSize);

        break;

      case 'z':

        /* do not write job identifier to submit STDOUT */

        /* NOTE:  do not insert '-z' into buffer, Moab needs job id to link migrated job
                  to grid job */

        /*
        if (SBuffer != NULL)
          MJobPBSInsertArg("-z",OptArg,SBuffer,SBuffer,SBufSize);
        */

        bmset(&C.DisplayFlags,mdspfHideOutput);

        break;

      case (char)-1:

        /* end of args */

        break;

      case '?':
      default:

        if (EMsg != NULL)
          {
          sprintf(EMsg,"invalid flag '%s'",
            OptArg);
          }

        return(FAILURE);

        /*NOTREACHED*/

        break;
      }  /* END switch ((char)ArgIndex) */
    }    /* END while (ArgIndex != -1) */

  /* post-processing */

  /* catch case where -x is specified without -I */

  if ((!C.IsInteractive) && (HasInteractiveScript))
    {
      if (EMsg != NULL)
        {
        strcpy(EMsg,"Cannot run interactive script without being in interactive mode.");
        }

      return(FAILURE);
    }

  /* Check to see if partition or feature was specified */ 

  if (MXMLGetChild(JE,"Requested",NULL,&AE) == SUCCESS)
    {
    mxml_t *NE;

    if (MXMLGetChild(AE,"NodeProperties",NULL,&NE) == SUCCESS)
      {
      if (MXMLGetChild(NE,"Feature",NULL,NULL) == SUCCESS)
        {
        bmset(&C.SubmitConstraints,mjaReqFeatures);
        }

      if (MXMLGetChild(NE,"Partition",NULL,NULL) == SUCCESS)
        {
        bmset(&C.SubmitConstraints,mjaPAL);
        }

      if (MXMLGetChild(NE,"HostList",NULL,NULL) == SUCCESS)
        {
        bmset(&C.SubmitConstraints,mjaHostList);
        }
      }
    }    /* END if (MXMLGetChild(JE,"Requested",NULL,&AE) == SUCCESS) */

  if (RMXString[0] != '\0')
    {
    if (MUStrStr(RMXString,"partition",0,TRUE,FALSE) != NULL)
      bmset(&C.SubmitConstraints,mjaPAL);

    if (MUStrStr(RMXString,"feature",0,TRUE,FALSE) != NULL)
      bmset(&C.SubmitConstraints,mjaReqFeatures);
    }
  else if (MXMLGetChild(JE,(char *)MS3JobAttr[ms3jaRMXString],NULL,&SE) == SUCCESS)
    {
    /* If the msub was submitted with an XML string then RMXString may not
     * be filled in as yet, so check to see if there is an Extension
     * attribute with partition info. */

    /* Check to see if a partition was submitted by extension to msub 
     * (e.g. <Extension>x=partition:node04</Extension> */

    if (MUStrStr(SE->Val,"partition:",0,TRUE,FALSE) != NULL)
      bmset(&C.SubmitConstraints,mjaPAL);
    }
  else if (MXMLGetChild(JE,(char *)MS3JobAttr[ms3jaPAL],NULL,&SE) == SUCCESS)
    {
    /* If the msub was submitted with an XML string then RMXString may not be filled in
     * as yet and there is a PartitionName attribute with partition info.
     * (e.g. <PartitionName>node04</PartitionName> */

    bmset(&C.SubmitConstraints,mjaPAL);
    }

  /* if feature, hostname, and partition not specified
   * use the default from the config file if specified there. */

  if ((C.DefaultSubmitPartition != NULL) && (!QuickPass))
    {
    if (!bmisset(&C.SubmitConstraints,mjaPAL) &&
        !bmisset(&C.SubmitConstraints,mjaReqFeatures)   &&
        !bmisset(&C.SubmitConstraints,mjaHostList))
      {
      /* add partition constraint as RMXString */

      /* NOTE:  pass partition using 'old-style' job extension semantics */

      if (RMXString[0] != '\0')
        {
        MUStrCat(RMXString,";",sizeof(RMXString));
        }

      MUStrCat(RMXString,"partition:",sizeof(RMXString));
      MUStrCat(RMXString,C.DefaultSubmitPartition,sizeof(RMXString));
      }
    }

  /* tell Moab to send stdout/stderr files to a different default location */

  if (C.LocalDataStageHead != NULL)
    {
    if (RMXString[0] != '\0')
      {
      MUStrCat(RMXString,";",sizeof(RMXString));
      }

    MUStrCat(RMXString,"localdatastagehead:",sizeof(RMXString));
    MUStrCat(RMXString,C.LocalDataStageHead,sizeof(RMXString));
    }

  if ((SBuffer != NULL) &&
      (RMXString[0] != '\0'))
    {
    char tmpBuffer[MMAX_BUFFER];
   
    int  rc;
 
    /* insert resource manager extensions */

    if (MXMLGetChild(JE,(char *)MS3JobAttr[ms3jaRMXString],NULL,&SE) == SUCCESS)
      {
      char *tmpSEVal = NULL;
      mbool_t DelimNeeded = FALSE;
      char *FirstPartition;
      char *LastPartition;
      const char *SearchString = "partition:";

      if ((MUStrStr(RMXString,"size",0,TRUE,FALSE) != NULL) &&
          (MUStrStr(SE->Val,"size",0,TRUE,FALSE) != NULL))
        {
        /* Remove any previous size data */

        char *TokPtr=NULL;
        char *tmpPtr;
        char *SEValTokenized = NULL;

        MUStrDup(&SEValTokenized,SE->Val);

        tmpPtr = MUStrTok(SEValTokenized,";",&TokPtr);

        if ((tmpPtr != NULL) && 
            (MUStrStr(tmpPtr,"size",0,TRUE,FALSE) != NULL))
          {
          /* if size is the first in the list, start with x= and don't add size */

          MUStrAppend(&tmpSEVal,NULL,"x=",'\0');
          }
        else if (tmpPtr != NULL)
          {
          /* Don't need to worry about adding x= because first element has it already
           * prepended from the time it was added to the XML */

          MUStrAppend(&tmpSEVal,NULL,tmpPtr,'\0');
          DelimNeeded = TRUE;
          }

        while ((tmpPtr = MUStrTok(NULL,";",&TokPtr)) != NULL)
          {
          if (MUStrStr(tmpPtr,"size",0,TRUE,FALSE) == NULL)
            {
            MUStrAppend(&tmpSEVal,NULL,tmpPtr,(DelimNeeded == TRUE) ? ';' : '\0');
            DelimNeeded = TRUE;
            }
          }

        MUFree(&SEValTokenized);
        }
      else
        {
        /* tmpSEVal will have x= prepended from when it was first added to the xml */

        MUStrDup(&tmpSEVal,SE->Val);
        }

      rc = snprintf(tmpBuffer,sizeof(tmpBuffer),"%s%s%s",
        (tmpSEVal != NULL) ? tmpSEVal : "x=",
        ((tmpSEVal != NULL) && (strlen(tmpSEVal) > 0) && (tmpSEVal[strlen(tmpSEVal) - 1] == '=')) ? "" : ";", 
        RMXString);

      /* If we are mapping feature to partition and there is already a partition in the RMX String (e.g. the default partition)
         then override the partition with the feature by taking out the partition. */

      if ((C.MapFeatureToPartition == TRUE) &&
          (((FirstPartition = MUStrStr(tmpBuffer,"partition:",0,TRUE,FALSE)) != NULL)) &&
          ((MUStrStr(tmpBuffer,"feature:",0,TRUE,FALSE) != NULL)))
        {
        char *EndPtr;

        EndPtr = strchr(FirstPartition,';');

        if ((EndPtr != NULL) && (EndPtr[1] != '\0')) 
          {
          /* partition is not the last element in the buffer so shift everything after partition  over top of partition */

          EndPtr++;

          while (*EndPtr != '\0')
            {
            *FirstPartition++ = *EndPtr++;
            }
          }
        *FirstPartition = '\0';
        }

      MUFree(&tmpSEVal);

      /* remove multiple occurences of the partition parameter from tmpBuffer */
 
      if ((FirstPartition = MUStrStr(tmpBuffer,SearchString,0,TRUE,FALSE)) != NULL)
        {
        if ((LastPartition = MUStrStr(FirstPartition +
                strlen(SearchString),SearchString,0,TRUE,TRUE)) != NULL)
          {
          char *ShiftString;

          /* We found another partition in the RMXString.  Copy the rest
           * of the line over the top of the first partition entry
           * (shortening the RMXString). */

          if ((ShiftString = MUStrStr(FirstPartition,";",0,TRUE,FALSE)) != NULL)
            {
            ShiftString++;

            while (*ShiftString != '\0')
              {
              *FirstPartition++ = *ShiftString++;
              }
            *FirstPartition = '\0';
            }
          }/* END if(LastPartition */
        }/* END if(FirstPartition) */
      }
    else
      {
      SE = NULL;

      MXMLCreateE(&SE,(char *)MS3JobAttr[ms3jaRMXString]);

      MXMLAddE(JE,SE);

      rc = snprintf(tmpBuffer,sizeof(tmpBuffer),"x=%s", /* this guarantees the rmxstring has x= in the xml. */
        RMXString);
      }

    if (rc >= (int)sizeof(tmpBuffer))
      {
      fprintf(stderr,"ERROR:  cannot insert resource manager extension string (buffer too small)\n");

      exit(1);
      }

    MXMLSetVal(SE,(void *)tmpBuffer,mdfString);

    /* Remove -W x= before inserting. Use x= to not remove other -W's that don't have an x=. 
     * One concern about removing this is that there could be some extension string lost. Looking,
     * at the previous code this appears safe to do. BOC 10/29/09 */

    MJobPBSRemoveArg("-W","x=",SBuffer,SBuffer,SBufSize);
    MJobPBSInsertArg("-W",tmpBuffer,SBuffer,SBuffer,SBufSize);
    }

  /* the following values are all dependent on the number of tasks requests.  Therefore
     we store the tasks in a static variable and process these when it is present */

  if (ReqDisk > 0)
    {
    /* send requested disk req */

    /* set the requested disk space by dividing our request by the number of
     * tasks requested in the static TA struct */

    ReqDisk /= MAX(1,TA.ProcsRequested);

    SE = NULL;

    /* check to see if the disk request element is already created */

    MXMLGetChild(JE,(char *)MS3ReqAttr[ms3rqaReqDiskPerTask],NULL,&SE);

    /* if not, create it with what we have */

    if (SE == NULL) 
      {
      MXMLCreateE(&SE,(char *)MS3ReqAttr[ms3rqaReqDiskPerTask]);

      MXMLSetVal(SE,(void *)&ReqDisk,mdfInt);

      MXMLAddE(JE,SE);
      }
    else
      {
      /* if it's already there, reset the Val to the correct ReqDisk */

      MXMLSetVal(SE,(void*)&ReqDisk,mdfInt);
      }
    }  /* END if (ReqDisk > 0) */

  if (TA.JobMemLimit > 0)
    {
    /* send requested mem req */

    /* set the requested memory by dividing our request by the number of tasks
     * requested in the static TA struct */

    ReqMem = TA.JobMemLimit / MAX(1,TA.ProcsRequested); 

    SE = NULL;

    /* check to see if the memory element is already created */

    MXMLGetChild(JE,(char *)MS3ReqAttr[ms3rqaReqMemPerTask],NULL,&SE);

    /* if not, create it with what we have */

    if (SE == NULL)
      {
      MXMLCreateE(&SE,(char *)MS3ReqAttr[ms3rqaReqMemPerTask]);

      MXMLSetVal(SE,(void *)&ReqMem,mdfInt);

      MXMLAddE(JE,SE);
      }
    else 
      {
      /* if it's already there, reset the Val to the correct ReqMem */

      MXMLSetVal(SE,(void*)&ReqMem,mdfInt);
      }
    }  /* END if (TA.JobMemLimit > 0) */

  if (TA.JobSwapLimit > 0)
    {
    /* send requested vmem req */

    /* set the requested swap by dividing our request by the number of tasks
     * requested in the static TA struct */

    ReqSwap = TA.JobSwapLimit / MAX(1,TA.ProcsRequested);

    SE = NULL;
    
    /* check to see if the swap element is already created */

    MXMLGetChild(JE,(char *)MS3ReqAttr[ms3rqaReqSwapPerTask],NULL,&SE);

    /* if not, create it */

    if (SE == NULL)
      {
      MXMLCreateE(&SE,(char *)MS3ReqAttr[ms3rqaReqSwapPerTask]);

      MXMLSetVal(SE,(void *)&ReqSwap,mdfInt);

      MXMLAddE(JE,SE);
      }
    else
      {
      /* if it's already there, reset the Val to the correct ReqSwap */

      MXMLSetVal(SE,(void*)&ReqSwap,mdfInt);
      }
    } /* END if (TA.JobSwapLimit > 0) */

  /* check the SMP/SMT placement values for Torque; sockets, numa, and (sockets * numa) */

  if (C.Sockets)
    {
    if ((C.PPN < C.Sockets) != 0)
      {
      /* make sure the sockets is less than the ppn on the job */
      if (EMsg != NULL)
        {
        sprintf(EMsg,"The 'ppn=' value (%d) is less than the placement 'sockets=' value (%d).\n",
          C.PPN,
          C.Sockets);
        }
      return(FAILURE);
      }
    else if ((C.PPN % C.Sockets) != 0)
      {
      /* make sure the ppn value is evenly divisible by the placement sockets */
      if (EMsg != NULL)
        {
        sprintf(EMsg,"The 'ppn=' value (%d) is not evenly divisible by the placement 'sockets=' value (%d).\n",
          C.PPN,
          C.Sockets);
        }
      return(FAILURE);
      }
    }

  if (C.Numa)
    {
    if ((C.PPN < C.Numa) != 0)
      {
      /* make sure the numa is less than the ppn on the job */
      if (EMsg != NULL)
        {
        sprintf(EMsg,"The 'ppn=' value (%d) is less than the placement 'numa=' value (%d).\n",
          C.PPN,
          C.Numa);
        }
      return(FAILURE);
      }
    else if ((C.PPN % C.Numa) != 0)
      {
      /* make sure the ppn value is evenly divisible by the placement numa */
      if (EMsg != NULL)
        {
        sprintf(EMsg,"The 'ppn=' value (%d) is not evenly divisible by the placement 'numa=' value (%d).\n",
          C.PPN,
          C.Numa);
        }
      return(FAILURE);
      }
    }

  if ((C.Numa) && (C.Sockets))
    {
      int product = C.Numa * C.Sockets;
      if ((C.PPN % product) != 0)
      {
      /* make sure the ppn value is evenly divisible by the product of sockets and numa */
      if (EMsg != NULL)
        {
        sprintf(EMsg,"The 'ppn=' value (%d) is not evenly divisible by the product (%d) of the placement 'numa=' value (%d) and the 'socket=' value (%d).\n",
          C.PPN,
          product,
          C.Numa,
          C.Sockets);
        }
      return(FAILURE);
      }
    }

  return(SUCCESS);
  }  /* END MCSubmitProcessArgs() */





/**
 * If there is a comma in a variable name it must be quoted with
 * one set of both single and double quotes in the msub 
 * submission (e.g. "'ABC,DEF'").  This is how qsub also 
 * behaves.  Because the outter quotes are stripped off the 
 * argument when it's passed into the program from the command 
 * line, they need to be restored.  This function restores 
 * either the single quotes or double quotes depending upon 
 * which ones are missing.  See MOAB-2899. 
 *
 * @see MCSubmitProcessArgs() - parent
 *
 * @param VarList     (I) Original Variable List 
 * @param NewVarList  (0) New Variable List with restored quotes
 */

int MCRestoreQuotes(

  char    *NewVarList,  /* O */
  char    *VarList)     /* I */

  {
  int sindex = 0;
  char *tmpPtr;
  mbool_t isFirst = TRUE;

  if (VarList == NULL)
    {
    return(FAILURE);
    }

  tmpPtr = VarList;

  while (*tmpPtr != '\0')
    {
    if (isFirst)
      {
      if (*tmpPtr == '"')
        {
        NewVarList[sindex++] = '\'';
        isFirst = !isFirst;
        }
      /* Because sing quotes can also be found in the middle of
         a variable (e.g. Jack's House), the following are conditions
         to recognize a single quote at the BEGINING of
         a variable or variable's value (which are the begining
         of VarList, a comma, or and '=') */
      else if ((*tmpPtr == '\'') && 
               ((tmpPtr == VarList) ||
               (*(tmpPtr-1) == ',') ||
               (*(tmpPtr-1) == '=')))
        {
        NewVarList[sindex++] = '"';
        isFirst = !isFirst;
        }

      /* copy character into new char array */
      NewVarList[sindex++] = *tmpPtr;
      }
    else if (!isFirst)
      {
      /* copy character into new char array */
      NewVarList[sindex++] = *tmpPtr;

      if (*tmpPtr == '"')
        {
        NewVarList[sindex++] = '\'';
        isFirst = !isFirst;
        }
      /* The following are conditions to recognize a single
         quote at the END of a variable or variable's value (which
         are a comma or an string null character) */
      else if ((*tmpPtr == '\'') &&
               ((*(tmpPtr+1) == ',') ||
                (*(tmpPtr+1) == '\0')))
        {
        NewVarList[sindex++] = '"';
        isFirst = !isFirst;
        }
      }

    tmpPtr++;
    }

  NewVarList[sindex] = '\0';

  return(SUCCESS);
  }
  




/**
 * Error check the job flags syntax 
 *  
 * The format is flags= any flags but only one CancelOn flag
 *  
 * OR either [CancelOnFirstFailure,CancelOnFirstSuccess]
 * followed by one of the follwing:
 * :[CancelOnAnyFailure,CancelOnAnySuccess,CancelOnExitCode:[Codes]]
 *
 * @see MCSubmitProcessArgs() - parent
 *
 * @param FlagsStr (I) flag string 
 * @param EMsg (O) Error Message
 */

int MCCheckJobFlags(

  char    *FlagsStr,
  char     *EMsg)

  {
  char *flags = NULL;
  char *currFlagPtr;
  char *endFlagPtr=NULL;

  int cancelFlagPos = 0;

  if (EMsg != NULL)
    EMsg[0] = '\0';

  /* syntax check the job array cancellation flags */

  MUStrDup(&flags,FlagsStr);

  currFlagPtr = MUStrTok(flags,":",&endFlagPtr);

  while (currFlagPtr != NULL)
    {

    /* fail unless a valid flag */

    if (MUGetIndexCI(currFlagPtr,MJobFlags,MBNOTSET,mjfNONE) == mjfNONE)
      {
      sprintf(EMsg,"invalid job flag specified %s-rejecting job submission", 
        currFlagPtr);
 
      return(FAILURE);
      }
    else if ((!strcmp(currFlagPtr,"CancelOnFirstFailure")) ||
             (!strcmp(currFlagPtr,"CancelOnFirstSuccess")))
      {
      cancelFlagPos++;

      /* fail unless CancelOnFirstFailure or CancelOnFirstSuccess is the first cancel flag specified */

      if (cancelFlagPos != 1)
        {
        sprintf(EMsg,"Invalid job array cancellation policy option '%s'-can only be specified as the first option--rejecting job submission", 
          currFlagPtr);
   
        return(FAILURE);
        }
      }
    else if ((!strcmp(currFlagPtr,"CancelOnAnyFailure")) ||
             (!strcmp(currFlagPtr,"CancelOnAnySuccess")) ||
             (!strcmp(currFlagPtr,"CancelOnExitCode")))
      {
      cancelFlagPos++;

      /* fail unless CancelOnAnyFailure, CancelOnAnySuccess or CancelOnExitCode is the first or second cancel flag specified */

      if (cancelFlagPos > 2)
        {
        sprintf(EMsg,"Invalid job array cancellation policy option '%s'-cannot be specified after an option already specified--rejecting job submission", 
        currFlagPtr);
 
        return(FAILURE);
        }

      /* The rest of the flags are set to position 2 to make sure they're not followed by any other flags. */

      cancelFlagPos = 2;

      if (!strcmp(currFlagPtr,"CancelOnExitCode"))
        {
        char *exitcodes = NULL;
        char *currCodePtr=NULL;
        char *endCodePtr;
  
        /* check the exit codes - advance to next token */
  
        currFlagPtr = MUStrTok(NULL,":",&endFlagPtr);

        if (currFlagPtr == NULL)
          {
          sprintf(EMsg,"Invalid CancelOnExitCode policy option-not followed by an ':' with an exit code or exit code list--rejecting job submission");
          return(FAILURE);
          }
  
        MUStrDup(&exitcodes,currFlagPtr);
  
        currCodePtr = MUStrTok(exitcodes,"+",&endCodePtr);
  
        while(currCodePtr != NULL)
          {
          if (strchr(currCodePtr,'-') != NULL)
            {
            char *range = NULL;
            char *lowPtr;
            char *highPtr=NULL;
            int highVal;
            int lowVal;
  
            MUStrDup(&range,currCodePtr);

            /* if the first char is a '-' fail because they tried to make it negative */
  
            lowPtr = MUStrTok(range,"-",&highPtr);

            if ((*lowPtr == '\0') || (*highPtr == '\0'))
              {
              sprintf(EMsg,"Invalid CancelOnExitCode policy option '%s'-exit code range must be two decimal numbers separated by a hyphen--rejecting job submission", 
                   currCodePtr);
              return(FAILURE);
              }

            /* check for valid digits for low ptr */
            if (MUStrIsDigit(lowPtr) == FALSE)
              {
              sprintf(EMsg,"Invalid CancelOnExitCode policy option '%s'-exit code list must contain decimal numbers--rejecting job submission",
                      lowPtr);
              return(FAILURE);
              }
  
            if (MUStrIsDigit(highPtr) == FALSE)
              {
              sprintf(EMsg,"Invalid CancelOnExitCode policy option '%s'-exit code list must contain decimal numbers--rejecting job submission", 
                      highPtr);
              return(FAILURE);
              }

            highVal = atoi(highPtr);
            lowVal = atoi(lowPtr);
  
            /* make sure the highVal is greater than the low val */
            if (highVal < lowVal)
              {
              sprintf(EMsg,"Invalid CancelOnExitCode policy option-exit code range '%s' must have lower bound less than upper bound--rejecting job submission",
                 currCodePtr);
              return(FAILURE);
              }
            
            /* make sure the lowVal and highVal fall within the limits */

            if (lowVal > 383)
              {
              sprintf(EMsg,"Invalid CancelOnExitCode policy option '%d'-exit code value outside the allowed values 0-383--rejecting job submission",
                 lowVal);
              
              return(FAILURE);
              }  

            if (highVal > 383)
              {
              sprintf(EMsg,"Invalid CancelOnExitCode policy option '%d'-exit code value outside the allowed values 0-383--rejecting job submission",
                highVal);

              return(FAILURE);
              }
  
            MUFree(&range);
            } /* END if (strchr(currCodePtr,'-') != NULL) */
          else
            {
            int val;
  
            /* check the single error code for valid digits */
            if (MUStrIsDigit(currCodePtr) == FALSE)
              {
              sprintf(EMsg,"Invalid CancelOnExitCode policy option '%s'-exit code list must contain decimal numbers--rejecting job submission", 
                      currCodePtr);

              return(FAILURE);
              }
            
            val = atoi(currCodePtr);
  
            /* make sure the val fall within the limits */

            if ((val > 383) || (val < 0))
              {
              sprintf(EMsg,"Invalid CancelOnExitCode policy option '%d'-exit code value outside the allowed values 0-383--rejecting job submission",
                 val);

              return(FAILURE);
              } /* END if ((val > 383) || (val < 0)) */
            } /* END else */
  
          currCodePtr = MUStrTok(NULL,"+",&endCodePtr);
          }  /* END while(currCodePtr != NULL) */
  
        MUFree(&exitcodes);
        }  /* END else if (!strcmp(currFlagPtr,"CancelOnExitCode"))*/
      }

    currFlagPtr = MUStrTok(NULL,":",&endFlagPtr);
    }   /* END while (currFlagPtr != NULL) */

  MUFree(&flags);

  return(SUCCESS);
  }




/**
 * Load and process RM submit script.
 *
 * @see MCSubmitCreateRequest() - parent
 * @see MCSubmitProcessArgs() - peer
 * @see MCProcessRMScriptDirective() - child
 *
 * NOTE:  will verify SrcBuf.
 *
 * @param SrcBuf (I) [string buffer or file depending on IsFile]
 * @param IsFile (I)
 * @param JE (I) [modified]
 * @param SBuffer (I) [modified]
 * @param SBufSize (I)
 * @param QuickPass (I)
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 */

int MCProcessRMScript(

  char    *SrcBuf,    /* I (string buffer or file depending on IsFile) */
  mbool_t  IsFile,    /* I */
  mxml_t  *JE,        /* I (modified) */
  char    *SBuffer,   /* I (modified) */
  int      SBufSize,  /* I */
  mbool_t  QuickPass, /* I */
  char    *EMsg)      /* O (optional,minsize=MMAX_LINE) */

  {
  char *ptr;
  char *Head;

  char *nptr;

  char *tmpPtr;

  char *BPtr=NULL;
  int BSpace=0;

  char  tmpBuf[MMAX_SCRIPT];  /* FIXME:  make dynamic */

  mbool_t ExecFound = FALSE;

  mxml_t *CE;

  enum MRMTypeEnum RMType = mrmtNONE;

  FILE *SFile;

  mbool_t IsText;  /* specified command file is ASCII text file */

  const char *FName = "MCProcessRMScript";

  MDB(2,fUI) MLog("%s(SrcBuf,%s,JE,SBuffer,%d,%s,EMsg)\n",
    FName,
    MBool[IsFile],
    SBufSize,
    MBool[QuickPass]);

  if (EMsg != NULL)
    EMsg[0] = '\0';

#ifndef __MOPT

  if ((SrcBuf == NULL) || (JE == NULL))
    {
    return(FAILURE);
    }

#endif  /* !__MOPT */

  /* NOTE:  submit script directives should be overridden by cmdline args 
            which have been previously loaded. */

  if (IsFile == TRUE)
    {
    int rc;
    mbuffd_t BufFD;
    char *BPtr;
    char  curLine[MMAX_LINE*4];

    if (!strcasecmp(SrcBuf,"/dev/null"))
      {
      return(SUCCESS);
      }

    if ((SFile = fopen(SrcBuf,"r")) == NULL)
      {
      fprintf(stderr,
        "msub: could not open script %s\n",
        SrcBuf);

      if (EMsg != NULL)
        {
        snprintf(EMsg,MMAX_LINE,"cannot open script '%s', errno=%d, '%s'",
          SrcBuf,
          errno,
          strerror(errno));
        }

      return(FAILURE);
      }

    MUSNInit(&BPtr,&BSpace,tmpBuf,sizeof(tmpBuf));
    MUBufferedFDInit(&BufFD,fileno(SFile));

    curLine[0] = '\0';

    rc = MUBufferedFDReadLine(&BufFD,curLine,sizeof(curLine));

    while (rc != 0)
      {
      if (rc == -1)
        {
        /* error reading */

        if (EMsg != NULL)
          {
          snprintf(EMsg,MMAX_LINE,"cannot read file '%s', errno=%d, '%s'",
            SrcBuf,
            errno,
            strerror(errno));
           }

        fclose(SFile);
        return(FAILURE);
        }
 
      if (C.NoProcessCmdFile == FALSE)
        {
        tmpPtr = curLine;
        MCfgAdjustBuffer(&tmpPtr,FALSE,NULL,TRUE,FALSE,TRUE);
        }

      MUSNCat(&BPtr,&BSpace,curLine);

      rc = MUBufferedFDReadLine(&BufFD,curLine,MMAX_LINE);
      }

    fclose(SFile);
    }    /* END if (IsFile == TRUE) */
  else
    {
    MUStrCpy(tmpBuf,SrcBuf,sizeof(tmpBuf));

    /* process command script */

    if (C.NoProcessCmdFile == FALSE)
      {
      tmpPtr = tmpBuf;
      MCfgAdjustBuffer(&tmpPtr,FALSE,NULL,TRUE,FALSE,FALSE);
      }
    }

  MUFileIsText(NULL,tmpBuf,&IsText);

  if (IsText == FALSE)
    {
    if (EMsg != NULL)
      {
      snprintf(EMsg,MMAX_LINE,"msub: '%s' must be text/command file (binary executable specified?)\n",
        SrcBuf);
      }
   
    return(FAILURE);
    }

  /* check for specific RM variables -- lock accordingly */

  if ((C.NoProcessCmdFile == FALSE) &&
      (strstr(tmpBuf,"$PBS")))
    {
    mxml_t *EE;

    if (MXMLGetChild(JE,(char *)MS3JobAttr[ms3jaReqRMType],NULL,&EE) == FAILURE)
      {
      /* put Line into JE */

      MXMLCreateE(&EE,(char *)MS3JobAttr[ms3jaReqRMType]);

      MXMLSetVal(EE,(void *)MRMType[mrmtPBS],mdfString);

      MXMLAddE(JE,EE);
      }
    }

#ifdef __MNOT /* FIXME:  breaks when shell scripts use "$(" */
  else if (strstr(tmpBuf,"$("))
    {
    mxml_t *EE;

    if (MXMLGetChild(JE,(char *)MS3JobAttr[ms3jaReqRMType],NULL,&EE) == FAILURE)
      {
      /* put Line into JE */

      MXMLCreateE(&EE,(char *)MS3JobAttr[ms3jaReqRMType]);

      MXMLSetVal(EE,(void *)MRMType[mrmtLL],mdfString);

      MXMLAddE(JE,EE);
      }
    }
#endif

  /* NOTE:  cannot use strtok because it cannot distinguish between '\n\n\n' and '\n' */

  ptr  = tmpBuf;
  nptr = strchr(tmpBuf,'\n');

  if (nptr != NULL)
    *nptr = '\0';

  MUSNInit(&BPtr,&BSpace,SBuffer,SBufSize);

  while (ptr != NULL)
    {
    Head = ptr;

    if (ExecFound == FALSE)
      {
      char *Line = Head;

      /* check if line contains a RM directive */

      while (isspace(*Line))
        {
        /* skip white space */

        Line++;
        }

      
      if (strcmp(Line,"##") &&
         ((MUIsRMDirective(Line,mrmtPBS) || 
           MUIsRMDirective(Line,mrmtLL))))
        {
        if (MCProcessRMScriptDirective(
              Line,
              JE,
              &RMType,  /* O */
              SBuffer,
              SBufSize,
              QuickPass,
              EMsg) == FAILURE)
          {
          return(FAILURE);
          }

        MUSNUpdate(&BPtr,&BSpace);
        }      /* END if (!MUIsRMDirective() || ...) */
      else if ((UserDirectiveSpecified == TRUE) && 
               !strncasecmp(Line,UserDirective,strlen(UserDirective)))
        {
        if (MCProcessRMScriptDirective(
              Line,
              JE,
              &RMType,  /* O */
              SBuffer,
              SBufSize,
              QuickPass,
              EMsg) == FAILURE)
          {
          return(FAILURE);
          }

        MUSNUpdate(&BPtr,&BSpace);
        }
      else if ((Line != NULL) && (*Line != '\0'))
        {
        /* check for executable line */

        if ((*Line != ':') && (*Line != '#'))
          {
          mxml_t *EE;

          if (MXMLGetChild(JE,(char *)MS3JobAttr[ms3jaCmdFile],NULL,&EE) == FAILURE)
            {
            /* put Line into JE */

            MXMLCreateE(&EE,(char *)MS3JobAttr[ms3jaCmdFile]);

            MXMLSetVal(EE,Line,mdfString);

            MXMLAddE(JE,EE);
            }

          ExecFound = TRUE;
          }

        MUSNCat(&BPtr,&BSpace,Line);
        MUSNCat(&BPtr,&BSpace,"\n");
        }      /* END else if ((Line != NULL) && (*Line != '\0')) */
      else if (C.NoProcessCmdFile == TRUE)
        {
        /* preserve empty lines */

        MUSNCat(&BPtr,&BSpace,Head);
        MUSNCat(&BPtr,&BSpace,"\n");
        }
      }        /* END if (IsExec == FALSE) */
    else
      {
      /* restore command file script lines */

      MUSNCat(&BPtr,&BSpace,ptr);
      MUSNCat(&BPtr,&BSpace,"\n");
      } /* END else (ExecFound == FALSE) */

    if (nptr == NULL)
      {
      ptr = NULL;
      }
    else
      {
      ptr = nptr + 1;
      nptr = strchr(ptr,'\n');

      if (nptr != NULL)
        *nptr = '\0';
      }
    }   /* END while (ptr != NULL) */

  if (MXMLGetChild(
        JE,
        (char *)MS3JobAttr[ms3jaSubmitLanguage],
        NULL,
        &CE) == FAILURE)
    {
    /* set submit language */

    if (RMType == mrmtNONE)
      {
      if (C.EmulationMode != mrmtNONE)
        RMType = C.EmulationMode;
      else
        RMType = MDefRMLanguage;
      }

    CE = NULL;

    MXMLCreateE(&CE,(char *)MS3JobAttr[ms3jaSubmitLanguage]);

    MXMLSetVal(CE,(void *)MRMType[RMType],mdfString);

    MXMLAddE(JE,CE);
    }

  if (bmisset(&C.DisplayFlags,mdspfTest))
    {
    fprintf(stderr,"%s",
      SBuffer);

    exit(0);
    }

  return(SUCCESS);
  } /* END MCProcessRMScript() */





/**
 * Process all RM directives found in command script. 
 *
 * @see MCProcessRMScript() - parent
 * @see MCSubmitProcessArgs() - child
 *
 * @param Line (I)
 * @param JE (I) [modified]
 * @param Type (O)
 * @param SBuffer (I)
 * @param SBufSize (I)
 * @param QuickPass (I)
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 */

int MCProcessRMScriptDirective(

  char             *Line,        /* I */
  mxml_t           *JE,          /* I (modified) */
  enum MRMTypeEnum *Type,        /* O */
  char             *SBuffer,     /* I */
  int               SBufSize,    /* I */
  mbool_t           QuickPass,   /* I */
  char             *EMsg)        /* O (optional,minsize=MMAX_LINE) */

  {
  static int Pass = 1;

  static char *ArgV[MMAX_ARGVLEN + 1];

  const char *FName = "MCProcessRMScriptDirective";

  MDB(2,fUI) MLog("%s(%.32s...,JE,Type,SBuffer,SBufSize,EMsg)\n",
    FName,
    Line);

  if (EMsg != NULL)
    EMsg[0] = '\0';

#ifndef __MOPT

  if ((Line == NULL) || (JE == NULL) || (Type == NULL))
    {
    return(FAILURE);
    }

#endif  /* !__MOPT */

  /* set default type NYI */

  /* If it is a custom directive, assume that it is PBS syntax. */

  if (MUIsRMDirective(Line,mrmtPBS) ||
     ((UserDirectiveSpecified == TRUE) && 
      (!strncasecmp(Line,UserDirective,strlen(UserDirective)))))
    {
    /* process pbs directive */

    int ArgCount;

    *Type = mrmtPBS;

    if (Pass == 1)
      {
      ArgCount = 0;

      while (ArgCount < MMAX_ARGVLEN + 1)
        ArgV[ArgCount++] = NULL;
      }

    __MCMakeArgv(&ArgCount,ArgV,Line);

    Pass++;

    if (MCSubmitProcessArgs(
          JE,
          &ArgCount,
          ArgV,
          SBuffer,
          SBufSize,
          TRUE, /* overwrite duplicates in a script */
          QuickPass,
          EMsg) == FAILURE)
      {
      return(FAILURE);
      }
    }
  else
    {
    /* un-supported directive */

    fprintf(stderr, "INFO:     unexpected line '%s' - submit language not supported",
        Line);

    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MCProcessRMScriptDirective() */





/**
 *
 *
 * @param ArgCount (O)
 * @param ArgV (O)
 * @param Line (I)
 */

int __MCMakeArgv(

  int   *ArgCount,  /* O */
  char **ArgV,      /* O */
  char  *Line)      /* I */

  {
  char *l, *b, *c;
  char Buffer[MMAX_BUFFER];
  int Length;
  char Quote;

  *ArgCount = 0;

  ArgV[(*ArgCount)++] = strdup("msub");

  l = Line;
  b = Buffer;

  while (isspace(*l))
    l++;

  c = l;

  if (*c == '#')
    *b++ = *c++;

  while (*c != '\0')
    {
    if ((*c == '"') || (*c == '\''))
      {
      Quote = *c;
      c++;

      while ((*c != Quote) && *c)
        *b++ = *c++;

      if (*c == '\0')
        {
        fprintf(stderr,"msub: unmatched %c\n",
          *c);

        exit(1);
        }

      c++;
      }
    else if (*c == '\\')
      {
      c++;

      *b++ = *c++;
      }
    else if (isspace(*c) || (*c == '#'))
      {
      Length = c - l;

      if (ArgV[*ArgCount] != NULL)
        MUFree(&ArgV[*ArgCount]);

      ArgV[*ArgCount] = (char *)MUCalloc(1,Length + 1);

      if (ArgV[*ArgCount] == NULL)
        {
        fprintf(stderr, "msub: out of memory\n");

        exit(2);
        }

      *b = '\0';

      strcpy(ArgV[(*ArgCount)++],Buffer);

      while (isspace(*c))
        c++;

      l = c;

      b = Buffer;

      /* unescaped # in the line are considered an embedded comment to the end of the line */

      if (*c == '#')
        {
        while (*c != '\0')
          c++;
        continue;
        }
      }
    else
      {
      *b++ = *c++;
      }
    }

  if (c != l)
    {
    Length = c - l;

    if (ArgV[*ArgCount] != NULL)
      MUFree(&ArgV[*ArgCount]);

    ArgV[*ArgCount] = (char *)MUCalloc(1,Length + 1);

    if (ArgV[*ArgCount] == NULL)
      {
      fprintf(stderr, "msub: out of memory\n");

      exit(2);
      }

    *b = '\0';

    strcpy(ArgV[(*ArgCount)++],Buffer);
    }

  return(SUCCESS);
  }  /* END __MCMakeArgv() */



/**
 *
 *
 * @param JIDP (O)
 */

int __MCCGetJID(

  char **JIDP)  /* O */

  {
  char *ptr;

  if (JIDP == NULL)
    {
    return(FAILURE);
    }

  *JIDP = NULL;

  ptr = getenv("SLURM_JOBID");

  if (ptr != NULL)
    {
    *JIDP = ptr;

    return(SUCCESS);
    }

  ptr = getenv("PBS_JOBID");

  if (ptr != NULL)
    {
    *JIDP = ptr;

    return(SUCCESS);
    }

  ptr = getenv("MOAB_JOBID");

  if (ptr != NULL)
    {
    *JIDP = ptr;

    return(SUCCESS);
    }

  return(FAILURE);
  }  /* END __MCCGetJID() */




/**
 *
 *
 * @param NIDP (O)
 */

int __MCCGetNID(

  char **NIDP)  /* O */

  {
  static char tmpHost[MMAX_NAME];

  if (NIDP == NULL)
    {
    return(FAILURE);
    }

  *NIDP = NULL;

  if (MOSGetHostName(NULL,tmpHost,NULL,FALSE) == FAILURE)
    {
    return(FAILURE);
    }

  *NIDP = tmpHost;

  return(FAILURE);
  }  /* END __MCCGetNID() */




/**
 *
 *
 * @param CP (I) [modified/alloc]
 * @param JID (I) [optional]
 * @param Action (I)
 * @param Arg1 (I) [optional]
 * @param EMsg (I) [optional,minsize=MMAX_LINE]
 */

int __MCCJobAction(

  void                **CP,      /* I (modified/alloc) */
  char                 *JID,     /* I (optional) */
  enum MJobCtlCmdEnum   Action,  /* I */
  void                 *Arg1,    /* I (optional) */
  char                 *EMsg)    /* I (optional,minsize=MMAX_LINE) */

  {
  msocket_t S;

  mxml_t   *E = NULL;

  mccfg_t  *C;

  char      CommandBuffer[MMAX_LINE];

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (CP == NULL)
    {
    CP = (void **)&MClientC;
    }

  if (*CP == NULL)
    {
    *CP = MUCalloc(1,sizeof(mccfg_t));

    if (*CP == NULL)
      {
      if (EMsg != NULL)
        {
        strcpy(EMsg,"no memory");
        }

      return(MCCFAILURE);
      }
    }

  C = (mccfg_t *)*CP;

  if (C->IsInitialized == FALSE)
    {
    __MCCInitialize(NULL,0,&C);
    }

  if (JID == NULL)
    {
    if (__MCCGetJID(&JID) == FAILURE)
      {
      if (EMsg != NULL)
        strcpy(EMsg,"cannot get jobid");

      return(MCCFAILURE);
      }
    }

  /* FORMAT: <Request action=\"cancel\" job=\"87\"><Object>job</Object></Request> */

  MXMLCreateE(&E,MSON[msonRequest]);

  MS3SetObject(E,MXO[mxoJob],NULL);

  MXMLSetAttr(E,MSAN[msanAction],(void *)MJobCtlCmds[Action],mdfString);

  MXMLSetAttr(E,MXO[mxoJob],JID,mdfString);

  if (Arg1 != NULL)
    MXMLAddE(E,(mxml_t *)Arg1);

  MXMLToString(E,CommandBuffer,sizeof(CommandBuffer),NULL,FALSE);

  if (MCDoCommand(mcsMJobCtl,C,CommandBuffer,NULL,EMsg,&S) == FAILURE)
    {
    char *OBuf;

    MXMLDestroyE(&E);

    E = NULL;

    E = S.RDE;

    if (E != NULL)
      {
      int   OBufSize;

      OBuf = NULL;

      MXMLToXString(E,&OBuf,&OBufSize,MMAX_BUFFER << 5,NULL,FALSE);

      if (EMsg != NULL)
        MUStrCpy(EMsg,OBuf,MMAX_LINE);

      MUFree((char **)&OBuf);
      }

    MSUFree(&S);

    return(MCCFAILURE);
    }  /* END if (MCDoCommand(mcsMJobCtl,C,CommandBuffer,NULL,EMsg,&S) == FAILURE) */

  MXMLDestroyE(&E);

  MSUFree(&S);

  return(MCCSUCCESS);
  }  /* END __MCCJobAction() */





/**
 *
 *
 * @param CP (I) [modified/alloc]
 * @param NID (I) [optional]
 * @param Action (I)
 * @param Arg1 (I) [optional]
 * @param EMsg (I) [optional,minsize=MMAX_LINE]
 */

int __MCCNodeAction(

  void                 **CP,     /* I (modified/alloc) */
  char                  *NID,    /* I (optional) */
  enum MNodeCtlCmdEnum   Action, /* I */
  void                  *Arg1,   /* I (optional) */
  char                  *EMsg)   /* I (optional,minsize=MMAX_LINE) */

  {
  msocket_t S;

  mxml_t   *E = NULL;

  mccfg_t  *C;

  char      CommandBuffer[MMAX_LINE];

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (CP == NULL)
    {
    CP = (void **)&MClientC;
    }

  if (*CP == NULL)
    {
    *CP = MUCalloc(1,sizeof(mccfg_t));

    if (*CP == NULL)
      {
      if (EMsg != NULL)
        {
        strcpy(EMsg,"no memory");
        }

      return(MCCFAILURE);
      }
    }

  C = (mccfg_t *)*CP;

  if (C->IsInitialized == FALSE)
    {
    __MCCInitialize(NULL,0,&C);
    }

  if (NID == NULL)
    {
    if (__MCCGetNID(&NID) == FAILURE)
      {
      if (EMsg != NULL)
        strcpy(EMsg,"cannot get nodeid");

      return(MCCFAILURE);
      }
    }

  /* FORMAT: <Request action=\"cancel\" job=\"87\"><Object>job</Object></Request> */

  MXMLCreateE(&E,MSON[msonRequest]);

  MS3SetObject(E,(char *)MXO[mxoNode],NULL);

  MXMLSetAttr(E,MSAN[msanAction],(void *)MNodeCtlCmds[Action],mdfString);

  MXMLSetAttr(E,(char *)MXO[mxoNode],NID,mdfString);

  if (Arg1 != NULL)
    MXMLAddE(E,(mxml_t *)Arg1);

  MXMLToString(E,CommandBuffer,sizeof(CommandBuffer),NULL,FALSE);

  if (MCDoCommand(mcsMNodeCtl,C,CommandBuffer,NULL,EMsg,&S) == FAILURE)
    {
    char *OBuf;

    MXMLDestroyE(&E);

    E = NULL;

    E = S.RDE;

    if (E != NULL)
      {
      int   OBufSize;

      OBuf = NULL;

      MXMLToXString(E,&OBuf,&OBufSize,MMAX_BUFFER << 5,NULL,FALSE);

      if (EMsg != NULL)
        MUStrCpy(EMsg,OBuf,MMAX_LINE);

      MUFree((char **)&OBuf);
      }

    MSUFree(&S);

    return(MCCFAILURE);
    }  /* END if (MCDoCommand(mcsMNodeCtl,C,CommandBuffer,NULL,EMsg,&S) == FAILURE) */

  MXMLDestroyE(&E);

  MSUFree(&S);

  return(MCCSUCCESS);
  }  /* END __MCCNodeAction() */






/**
 *
 *
 * @param CP (I) [modified/alloc]
 * @param JID (I) [optional]
 * @param OBuf (O) [alloc]
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 */

int MCCJobQuery(

  void **CP,    /* I (modified/alloc) */
  char  *JID,   /* I (optional) */
  char **OBuf,  /* O (alloc) */
  char  *EMsg)  /* O (optional,minsize=MMAX_LINE) */

  {
  msocket_t S;

  mxml_t   *E = NULL;

  char      CommandBuffer[MMAX_LINE];

  int       OBufSize;

  mccfg_t  *C;

  if (OBuf != NULL)
    *OBuf = NULL;

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (CP == NULL)
    {
    CP = (void **)&MClientC;
    }

  if (*CP == NULL)
    {
    *CP = MUCalloc(1,sizeof(mccfg_t));

    if (*CP == NULL)
      {
      if (EMsg != NULL)
        {
        strcpy(EMsg,"no memory");
        }

      return(-1);
      }
    }

  C = (mccfg_t *)*CP;

  if (C->IsInitialized == FALSE)
    {
    __MCCInitialize(NULL,0,&C);
    }

  if (JID == NULL)
    {
    if (__MCCGetJID(&JID) == FAILURE)
      {
      if (EMsg != NULL)
        strcpy(EMsg,"cannot get jobid");

      return(-1);
      }
    }

  /* FORMAT: <Request action=\"query\" args=\"diag\" job=\"ALL\"><Object>job</Object></Request> */

  MXMLCreateE(&E,MSON[msonRequest]);

  MS3SetObject(E,(char *)MXO[mxoJob],NULL);

  MXMLSetAttr(E,MSAN[msanAction],(void *)MJobCtlCmds[mjcmQuery],mdfString);

  MXMLSetAttr(E,MSAN[msanArgs],(void *)"diag",mdfString);
 
  if ((JID == NULL) || (JID[0] == '\0'))
    MXMLSetAttr(E,(char *)MXO[mxoJob],(char *)"ALL",mdfString);
  else
    MXMLSetAttr(E,(char *)MXO[mxoJob],JID,mdfString);

  MXMLToString(E,CommandBuffer,sizeof(CommandBuffer),NULL,FALSE);

  if (MCDoCommand(mcsMJobCtl,(mccfg_t *)C,CommandBuffer,NULL,EMsg,&S) == FAILURE)
    {
    MXMLDestroyE(&E);

    E = NULL;

    E = S.RDE;

    if (E != NULL)
      {
      int   OBufSize;

      MXMLToXString(E,OBuf,&OBufSize,MMAX_BUFFER << 5,NULL,FALSE);

      if ((EMsg != NULL) && (OBuf != NULL))
        MUStrCpy(EMsg,*OBuf,MMAX_LINE);

      MUFree((char **)OBuf);
      }

    MSUFree(&S);

    return(-1);
    }

  MXMLDestroyE(&E);

  E = S.RDE;

  if (E != NULL)
    {
    MXMLToXString(E,OBuf,&OBufSize,MMAX_BUFFER << 5,NULL,TRUE);
    }

  MSUFree(&S);

  return(0);
  }  /* END MCCJobQuery() */





/**
 *
 *
 * @param C (I)
 * @param JobID (I)
 * @param OBuf (O) [alloc]
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 */

int __MCCJobListAllocRes(

  void  *C,      /* I */
  char  *JobID,  /* I */
  char **OBuf,   /* O - comma delimited list of hosts (alloc) */
  char  *EMsg)   /* O (optional,minsize=MMAX_LINE) */

  {
  msocket_t S;

  mxml_t   *E = NULL;

  char      CommandBuffer[MMAX_LINE];

  if (OBuf != NULL)
    *OBuf = NULL;

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if ((JobID == NULL) ||
      (JobID[0] == '\0'))
    {
    if (EMsg != NULL)
      strcpy(EMsg,"JobID parameter is required");

    return(-1);
    }    

  if (C == NULL)
    {
    return(-1);
    }

  /* FORMAT: <Request action=\"query\" args=\"hostlist\" job=\"<JOBID>\"><Object>job</Object></Request> */

  MXMLCreateE(&E,MSON[msonRequest]);

  MS3SetObject(E,(char *)MXO[mxoJob],NULL);

  MXMLSetAttr(E,MSAN[msanAction],(void *)MJobCtlCmds[mjcmQuery],mdfString);

  MXMLSetAttr(E,MSAN[msanArgs],(void *)"hostlist",mdfString);
 
  MXMLSetAttr(E,(char *)MXO[mxoJob],JobID,mdfString);

  MXMLToString(E,CommandBuffer,sizeof(CommandBuffer),NULL,FALSE);

  if (MCDoCommand(mcsMJobCtl,(mccfg_t *)C,CommandBuffer,NULL,EMsg,&S) == FAILURE)
    {
    MXMLDestroyE(&E);

    E = NULL;

    E = S.RDE;

    if (E != NULL)
      {
      int   OBufSize;

      MXMLToXString(E,OBuf,&OBufSize,MMAX_BUFFER << 5,NULL,FALSE);

      if ((EMsg != NULL) && (OBuf != NULL))
        MUStrCpy(EMsg,*OBuf,MMAX_LINE);

      MUFree((char **)OBuf);
      }

    MSUFree(&S);

    return(-1);
    }

  MXMLDestroyE(&E);

  E = S.RDE;

  /* only return delimited list */

  if (E != NULL)
    {
    MUStrDup(OBuf,E->Val);
    }

  MSUFree(&S);

  return(0);
  }  /* END __MCCJobListAllocRes() */






/**
 * Create new argv of command line args and args listed in /etc/msubrc into new argv.
 *
 * @param NewArgV (I) New list of args containing commandline args and those in /etc/msubrc. [modified,contents alloc'ed]
 * @param ArgV (I)
 * @param ArgC (I/O) [modified]
 */

int MCSubmitInsertArgs(

  char         **NewArgV,
  char         **ArgV,
  int           *ArgC)

  {
  FILE *RCFile = NULL;

  char *tmpPtr;

  struct stat sbuf;

  char *ptr;
  char *TokPtr;

  int aindex;
  int count;

  int argc = *ArgC;

  for (aindex = 0;aindex < argc;aindex++)
    {
    if ((ArgV[aindex] == NULL) || (aindex >= MMAX_ARG))
      break;

    NewArgV[aindex] = NULL;

    MUStrDup(&NewArgV[aindex],ArgV[aindex]);
    }  /* END for (aindex) */

  NewArgV[aindex] = NULL;

  const char *fileName = "/etc/msubrc";

  if (stat(fileName,&sbuf) == -1)
    {
    MDB(7,fUI) MLog("INFO:     no %s file to process\n",fileName);

    return(SUCCESS);
    }

  tmpPtr = (char *)MUMalloc(sbuf.st_size + 1);

  RCFile = fopen(fileName,"r");
  if (NULL == RCFile)
    {
    MDB(7,fUI) MLog("WARNING:  cannot open %s file (errno: %d, %s)\n",
      fileName,
      errno,
      strerror(errno));

    free(tmpPtr);
    return(FAILURE);
    }

  if ((count = (int)fread(tmpPtr,sbuf.st_size,1,RCFile)) < 0)
    {
    MDB(7,fUI) MLog("WARNING:  cannot read %s file (errno: %d, %s)\n",
      fileName,
      errno,
      strerror(errno));

    free(tmpPtr);
    fclose(RCFile);
    return(SUCCESS);
    }

  tmpPtr[sbuf.st_size] = '\0';

  MDB(2,fUI) MLog("INFO:     processing %s ('%.256s...')\n",
    fileName,
    tmpPtr);

  ptr = MUStrTok(tmpPtr," \t\n",&TokPtr);

  while (ptr != NULL)
    {
    NewArgV[aindex] = NULL;

    MUStrDup(&NewArgV[aindex],ptr);

    aindex++;

    ptr = MUStrTok(NULL," \t\n",&TokPtr);
    }

  *ArgC = aindex;

  NewArgV[aindex] = NULL;

  free(tmpPtr);
  fclose(RCFile);
  return(SUCCESS);
  }  /* END MCInsertArgs() */




static const char *MSubFilter = "/usr/local/sbin/msub_submitfilter";

/**
 *
 *
 * @param PrioScript (I)
 * @param JobScript (I)
 * @param IsFile (I)
 * @param NewFileName (O) name of filtered file
 */

int MCSubmitFilter(

  char          *PrioScript,   /* I */
  char          *JobScript,    /* I */
  mbool_t        IsFile,       /* I */
  char          *NewFileName)  /* O name of filtered file */

  {
  char  tmpFilter[] = "/tmp/msub.XXXXXX";

  FILE *FPipe = NULL;

  int tmpfd;
  int rc;

  char CmdArgs[MMAX_BUFFER];
  char tmpPrioScript[MMAX_BUFFER << 2];

  struct stat SBuf;
   
  char *ScriptPtr;

  char *ptr;
  char *TokPtr=NULL;
  char *ptr2;
  char *TokPtr2;

  int   ScriptLength = 0;

  const char *FName = "MCSubmitFilter";

  MDB(4,fUI) MLog("%s(%s,%s,%s,NewFileName)\n",
    FName,
    (PrioScript != NULL) ? PrioScript : "NULL",
    (JobScript != NULL) ? JobScript : "NULL",
    MBool[IsFile]);

  /* if the submitfilter exists, run it */

  if (stat(MSubFilter,&SBuf) != 0)
    {
    return(SUCCESS);
    }

  NewFileName[0] = '\0';

  MUStrCpy(tmpPrioScript,PrioScript,sizeof(tmpPrioScript));

  /* copy job script */

  if (IsFile == TRUE)
    {
    ScriptPtr = MFULoadNoCache(JobScript,1,NULL,NULL,NULL,NULL);
    }
  else
    {
    ScriptLength = (JobScript != NULL) ? strlen(JobScript) : 0;

    ScriptPtr = (char *)MUMalloc(ScriptLength + 1);

    MUStrCpy(ScriptPtr,JobScript,ScriptLength + 1);
    }

  /* run the copy through the submit filter. */

  if ((tmpfd = mkstemp(tmpFilter)) < 0) 
    {
    fprintf(stderr,
      "msub: could not create filter o/p %s\n",
      tmpFilter);

    MUFree(&ScriptPtr);

    return(FAILURE);
    }

  close(tmpfd);

  strcpy(CmdArgs,MSubFilter);
  strcat(CmdArgs," >");
  strcat(CmdArgs,tmpFilter);

  FPipe = popen(CmdArgs,"w");

  ptr = MUStrTok(ScriptPtr,"\n",&TokPtr);

  if ((fputs(ptr,FPipe) < 0) ||
      (fputs("\n",FPipe) < 0))
    {
    fprintf(stderr,"msub: error writing to filter stdin\n");

    fclose(FPipe);
    unlink(tmpFilter);

    MUFree(&ScriptPtr);

    exit(1);
    }

  ptr  = MUStrTok(NULL,"\n",&TokPtr);
  ptr2 = MUStrTok(tmpPrioScript,"\n",&TokPtr2);

  while (ptr2 != NULL)
    {
    if ((fputs(ptr2,FPipe) < 0) ||
        (fputs("\n",FPipe) < 0))
      {
      fprintf(stderr,"msub: error writing to filter stdin\n");

      fclose(FPipe);
      unlink(tmpFilter);

      MUFree(&ScriptPtr);

      exit(1);
      }

    ptr2 = MUStrTok(NULL,"\n",&TokPtr2);
    }  /* END while (ptr2 != NULL) */

  while (ptr != NULL)
    {
    if ((fputs(ptr,FPipe) < 0) ||
        (fputs("\n",FPipe) < 0))
      {
      fprintf(stderr,"msub: error writing to filter stdin\n");

      fclose(FPipe);
      unlink(tmpFilter);

      MUFree(&ScriptPtr);

      exit(1);
      }

    ptr = MUStrTok(NULL,"\n",&TokPtr);
    }  /* END while (ptr != NULL) */

  MUFree(&ScriptPtr);

  rc = fclose(FPipe);

  if (WEXITSTATUS(rc) == (unsigned char)0xff)
    {
    fprintf(stderr,"msub: Your job has been administratively rejected by the queueing system.\n");
    fprintf(stderr,"msub: There may be a more detailed explanation prior to this notice.\n");

    unlink(tmpFilter);

    exit(1);
    }

  if (WEXITSTATUS(rc))
    {
    fprintf(stderr,"msub: submit filter returned an error code, aborting job submission.\n");

    unlink(tmpFilter);

    exit(1);
    }

  MUStrCpy(NewFileName,tmpFilter,MMAX_LINE);

  return(SUCCESS);
  }  /* END MCSubmitFilter() */

/* END MClient.c */

/* vim: set ts=2 sw=2 expandtab: */
