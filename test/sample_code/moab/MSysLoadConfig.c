/* HEADER */

      
/**
 * @file MSysLoadConfig.c
 *
 * Contains: MSysLoadConfig
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  

#include <libgen.h>

int MServerStrictFileCheck();

/**
 * Load and process general and private config file parameters.
 *
 * NOTE:  This routine handles both object-based (ie, NODECFG, USERCFG, etc) and old-style 
 *        parameters (ie, LOGLEVEL, etc)
 * NOTE:  This routine handles special 'out-of-order' processing required to prevent parameter race conditions.
 *
 * @param CfgName (I)
 * @param Mode    (I) [mcmNONE or mcmForce]
 */

int MSysLoadConfig(
  char            *CfgName,
  enum MCModeEnum  Mode)
 
  {
  char  DirName[MMAX_LINE + 1];
  char  CfgFName[MMAX_LINE + 1];
  char  DatFName[MMAX_LINE + 1];
  char  PrivFName[MMAX_LINE + 1];
  char  Default[MMAX_NAME];
  char  *tmpFName;

  char  Value[MMAX_NAME];
  char  Name[MMAX_NAME];

  int   count;
  int   cindex;
  int   dindex;
  int   dcount = 0;
 
  int   SC;

  mnode_t *N;
  mnode_t *GN;



  const char *FName = "MSysLoadConfig";
 
  MDB(3,fCONFIG) MLog("%s(%s,%s)\n",
    FName,
    CfgName,
    MClientMode[Mode]);

  CfgFName[0] = '\0';
  DatFName[0] = '\0';
  PrivFName[0] = '\0';
 
  tmpFName = basename(CfgName);

  if (!strcmp(tmpFName, CfgName)) /* no path expression */
    {
    strcpy(CfgFName,MSched.CfgDir); /* add $HOMEDIR path */
    strcat(CfgFName, CfgName);
    }
  else
    {
    strcpy(CfgFName, CfgName); /* $HOMEDIR/configfilename */
    }

  /* Look for the configuration file in $HOMEDIR, $HOMEDIR/etc, and then in '.' */

  if (MFUGetInfo(CfgFName, NULL, NULL, NULL, NULL) == FAILURE)
    {
    strcpy(CfgFName, MSched.CfgDir);
    strcat(CfgFName, MCONST_MASTERCONFIGFILE); /* $HOMEDIR/etc/configfilename */
    if (MFUGetInfo(CfgFName, NULL, NULL, NULL, NULL) == SUCCESS)
      {
      strcat(MSched.CfgDir,"etc/");
      }
    else
      {
      strcpy(CfgFName, "./");
      strcat(CfgFName, tmpFName);

      if (MFUGetInfo(CfgFName, NULL, NULL, NULL, NULL) == FAILURE)
        {
        MDB(2,fCONFIG) MLog("WARNING:  cannot load config file '%s' errno %d\n",
          CfgFName, errno);
        return(FAILURE);
        }
      }
    }

  /* find the path of the correct config file */
  for (dindex = 0; dindex < (int)strlen(CfgFName); dindex++)
    {
    if (CfgFName[dindex] == '/')
      dcount = dindex;
    }
  memset(DirName, 0, sizeof(DirName));
  strncpy(DirName, CfgFName, dcount);
  strcat(DirName, "/");

  /* copy the path for dat and private file */
  strcpy(PrivFName, DirName);
  strcpy(DatFName, DirName);
  /* copy name of dat and private file */
  strcat(DatFName,MDEF_DATFILE);
  strcat(PrivFName,MDEF_PRIVFILE); /* moab-private.cfg is expected to be found in 
                                      the cfgdir since it contains sensitive info. */

  /* check to see if the dat file exists */
  if (MFUGetInfo(DatFName, NULL, NULL, NULL, NULL) == FAILURE)
    {
    /* it doesn't exist -- set the path to $HOMEDIR/etc/datfilename */
    strcpy(DatFName, MSched.CfgDir);
    strcat(DatFName, "etc/"); /* $HOMEDIR/etc/configfilename */
    strcat(DatFName,MDEF_DATFILE);
    }
 
  if ((MSched.ConfigBuffer == NULL) || (Mode == mcmForce))
    {
    if (MSched.ConfigBuffer != NULL)
      MUFree(&MSched.ConfigBuffer);

    if ((MSched.ConfigBuffer = MFULoad(CfgFName,1,macmWrite,&count,NULL,&SC)) == NULL)
      {
      MDB(2,fCONFIG) MLog("WARNING:  cannot load config file '%s' (using internal defaults)\n",
        CfgFName);
 
      return(FAILURE);
      }
 
    MCfgAdjustBuffer(&MSched.ConfigBuffer,TRUE,NULL,FALSE,TRUE,FALSE);

    if ((MSched.DatBuffer = MFULoad(DatFName,1,macmWrite,&count,NULL,&SC)) == NULL)
      {
      MDB(6,fCONFIG) MLog("INFO:     cannot load dat file '%s'\n",
        DatFName);
      }
    }  /* END if ((MSched.ConfigBuffer == NULL) || ...) */

  /* loop is needed as there is no direct lookup into MCfg[] */

  for (cindex = 1;MCfg[cindex].Name != NULL;cindex++)
    {
    /* We need to set the logdir before we set the loglevel. */
    
     /* NOTE:  This will work as long as logdir comes before loglevel in MCfg[]*/

    if (((MCfg[cindex].PIndex == mcoMaxGMetric) ||
         (MCfg[cindex].PIndex == mcoMaxNode) ||
         (MCfg[cindex].PIndex == mcoMaxGRes) ||
         (MCfg[cindex].PIndex == mcoLogDir) ||
         (MCfg[cindex].PIndex == mcoSpoolDir) ||
         (MCfg[cindex].PIndex == mcoStatDir) ||
         (MCfg[cindex].PIndex == mcoSchedToolsDir) ||
         (MCfg[cindex].PIndex == mcoJobMaxTaskCount) ||
         (MCfg[cindex].PIndex == mcoLogLevel)) &&
        (MCfgGetParameter(
           MSched.ConfigBuffer,
           MCfg[cindex].Name,
           NULL,  /* O */
           Name,    /* O */
           Value,     /* O */
           sizeof(Value),
           FALSE,
           NULL) == SUCCESS))
      {
      MDB(3,fCONFIG) MLog("INFO:     value for parameter '%s': '%s'\n",
        MCfg[cindex].Name,
        Value);

      MCfgProcessLine(cindex,Name,NULL,Value,FALSE,NULL);
      /* we could probably break here for now... */
      }  /* END if((MCfg[cindex].PIndex == mcoLogLevel) && MCfgGetParameter... */
    }/* END for MCfg[cindex].Name != NULL... */

  /* Perform "-e" check for missing directories. */

  if (MSched.StrictConfigCheck == TRUE) /* -e */
    {
    if (MServerStrictFileCheck() == FAILURE)
      {
      exit(1);
      }
    }
 
  /* load general client config */
  
  MCredSetDefaults(); /* initializes hash tables. User hash table must be
                         created before loading the privated config because if
                         client peer exists moab creates a user by the name 
                         PEER:<rm_name> and will be lossed if called later.

  NOTE: The MCredSetDefaults must be after MCfgProcessLine for
        MaxMetrics & Max GRes, because there are memory allocations 
        based up those default sizes.	 */

  MOLoadPvtConfig(NULL,mxoNONE,NULL,NULL,PrivFName,NULL);

  MGResInitialize();

  /* process ALL partition */

  MParAdd(MCONST_GLOBALPARNAME,NULL);

  MGMetricInitialize();

  MParLoadConfig(&MPar[0],NULL);

  for (cindex = 1;MCfg[cindex].Name != NULL;cindex++)
    {
    if ((MCfg[cindex].PIndex == mcoCheckPointFile) &&
        (MCfgGetParameter(
           MSched.ConfigBuffer,
           MCfg[cindex].Name,
           NULL,  /* O */
           Name,    /* O */
           Value,     /* O */
           sizeof(Value),
           FALSE,
           NULL) == SUCCESS))
      {
      MDB(7,fCONFIG) MLog("INFO:     value for parameter '%s': '%s'\n",
        MCfg[cindex].Name,
        Value);

      MCfgProcessLine(cindex,Name,NULL,Value,FALSE,NULL);
      }  /* END while (MCfgGetParameter() == SUCCESS) */
    }

  /* create global node */

  GN = NULL;

  if (MNodeCreate(&GN,TRUE) == SUCCESS)
    {
    strcpy(GN->Name,MDEF_GNNAME);
    }

  /* allocate sub-structures/initialize default node */

  N = &MSched.DefaultN;

  if (MNodeCreate(&N,FALSE) == SUCCESS)
    {
    strcpy(MSched.DefaultN.Name,"DEFAULT");
    }

  N = NULL;

  MSched.DefaultN.ResOvercommitFactor = (double *)MUCalloc(1,sizeof(double) * mrlLAST);

  MSched.DefaultN.ResOvercommitThreshold = (double *)MUCalloc(1,sizeof(double) * mrlLAST);

  MSched.DefaultN.GMetricOvercommitThreshold = (double *)MUCalloc(1,sizeof(double) * MSched.M[mxoxGMetric]);

  /* initialize all GMetric thresholds to 'unset' value */
  for (cindex = 0;cindex < MSched.M[mxoxGMetric]; cindex++)
    {
    MSched.DefaultN.GMetricOvercommitThreshold[cindex] = MDEF_GMETRIC_VALUE;
    }

  /* initialize configuration data */

  if (MCfgGetSVal(
        MSched.ConfigBuffer,
        NULL,
        MParam[mcoStatProfCount],
        NULL,
        NULL,
        Value,
        sizeof(Value),
        0,
        NULL) != FAILURE)
    {
    /* must load PROFILECOUNT before loading credentials, nodes, or other profiled objects */

    MStat.P.MaxIStatCount = (int)strtol(Value,NULL,10);
    }

  if (MCfgGetSVal(
        MSched.ConfigBuffer,
        NULL,
        MParam[mcoStatProfDuration],
        NULL,
        NULL,
        Value,
        sizeof(Value),
        0,
        NULL) != FAILURE)
    {
    /* must load PROFILECOUNT before loading credentials, nodes, or other profiled objects */

    MStat.P.IStatDuration = (int)strtol(Value,NULL,10);

    if (MStat.P.IStatDuration == 0)
      MStat.P.IStatDuration = MDEF_PSTEPCOUNT;
    }

  if (MCfgGetSVal(
        MSched.ConfigBuffer,
        NULL,
        MParam[mcoSchedToolsDir],
        NULL,
        NULL,
        Value,
        sizeof(Value),
        0,
        NULL) != FAILURE)
    {
    /* must load TOOLSDIR before processing any URL values */

    MSchedProcessOConfig(&MSched,mcoSchedToolsDir,0,0.0,Value,NULL,NULL);
    }

  /* NYI: need to process mxoQOS "DEFAULT" here */

  MCfgProcessBuffer(MSched.ConfigBuffer);

  MCPLoad(MCP.CPFileName,mckptSched);

  MSchedLoadConfig(NULL,FALSE);

  MAdminLoadConfig(NULL);
  MVMLoadConfig(NULL);

  MClientLoadConfig(NULL,MSched.ConfigBuffer);

  /* NOTE:  load RM before creds to eliminate possible race condition */

  MRMLoadConfig(NULL,NULL,NULL);

  MUStrCpy(Default,"DEFAULT",sizeof(Default));

  MCredLoadConfig(mxoSys,NULL,NULL,NULL,FALSE,NULL);

  MCredLoadConfig(mxoQOS,Default,NULL,NULL,FALSE,NULL);
  MCredLoadConfig(mxoQOS,NULL,NULL,NULL,FALSE,NULL);      

  MCredLoadConfig(mxoUser,Default,NULL,NULL,FALSE,NULL);
  MCredLoadConfig(mxoUser,NULL,NULL,NULL,FALSE,NULL);      

  MCredLoadConfig(mxoGroup,Default,NULL,NULL,FALSE,NULL);
  MCredLoadConfig(mxoAcct,Default,NULL,NULL,FALSE,NULL);
  MCredLoadConfig(mxoClass,Default,NULL,NULL,FALSE,NULL);
 
  MCredLoadConfig(mxoGroup,NULL,NULL,NULL,FALSE,NULL);
  MCredLoadConfig(mxoAcct,NULL,NULL,NULL,FALSE,NULL);
  MCredLoadConfig(mxoClass,NULL,NULL,NULL,FALSE,NULL);

  /* load FSTree after creds and before rsv's */

  if (MSched.FSTreeConfigDetected == TRUE)
    {
    MFSTreeLoadConfig(NULL);
    }

  /* load SR's */
 
  MSRLoadConfig(NULL,NULL,NULL);

  /* load rsv profiles */

  MSRLoadConfig(NULL,MPARM_RSVPROFILE,NULL);

  MTJobLoadConfig(NULL,NULL,NULL);

  MTJobMatchLoadConfig(NULL,NULL);

  MNodeLoadConfig(&MSched.DefaultN,NULL);

  MClusterLoadConfig(NULL,NULL);

  MImageLoadConfig(NULL,NULL);

  /* accounting managers are disabled in basic */

  MAMLoadConfig(NULL,NULL);

  MIDLoadConfig(NULL,NULL);

  /* NOTE:  only add global node if global settings detected */

  MNodeLoadConfig(GN,NULL);

  if ((MCResIsEmpty(&GN->CRes) == FALSE) ||
     ((MUMemCCmp((char *)&GN->AP,'\0',sizeof(GN->AP)) == FAILURE)))
    {
    /* global resources/policies are configured - node linked/initialized in MNodeAdd() */

    if (MNodeAdd(GN->Name,&N) == SUCCESS)
      {
      MCResCopy(&N->CRes,&GN->CRes);

      bmcopy(&N->FBM,&GN->FBM);
      memcpy(&N->AP,&GN->AP,sizeof(N->AP));

      N->PowerPolicy = mpowpStatic;

      MNodeTransition(GN);
      }
    else
      {
      MDB(2,fCONFIG) MLog("WARNING:  cannot add global node '%s'\n",
        GN->Name);
      }
    }  /* END if (MUMemCCmp() == FAILURE) */

  MCredSyncDefaultConfig();

  MCfgEnforceConstraints(TRUE);

  sprintf(MSched.KeyFile,"%s/%s",
    MSched.CfgDir,
    MSCHED_KEYFILE);

  MSched.UID = MOSGetEUID();
  MSched.GID = MOSGetGID();

  /* free temporary memory */

  MNodeDestroy(&GN);

  /* NOTE:  must free core node structure */

  MUFree((char **)&GN);

  return(SUCCESS);
  }  /* END MSysLoadConfig() */

/* END MSysLoadConfig.c */
