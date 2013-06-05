/* HEADER */

      
/**
 * @file MSchedConfig.c
 *
 * Contains: MSched Configuration functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 * This function will load the config file from disk and
 * then only process the SCHEDCFG[] lines of the configuration
 * file(s). This is used primarily to help with HA, as we must
 * know the HA config before daemonizing, opening sockets, etc.
 */

int MSchedPreLoadConfig()

  {
  int  count = 0;
  char DatFName[MMAX_PATH];

  if (MSched.ConfigBuffer != NULL)
    return(FAILURE);
    
  if ((MSched.ConfigBuffer = MFULoad(MSched.ConfigFile,1,macmWrite,&count,NULL,NULL)) == NULL)
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

    if ((MSched.ConfigBuffer = MFULoad(tempFName,1,macmWrite,&count,NULL,NULL)) == NULL)
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
    }  /* END if (MSysFindMoabCfg(MSched.CfgDir) == FAILURE) */

  sprintf(DatFName,"%s%s",MSched.CfgDir,MDEF_DATFILE);

  MCfgAdjustBuffer(&MSched.ConfigBuffer,TRUE,NULL,FALSE,TRUE,FALSE);

  if ((MSched.DatBuffer = MFULoad(DatFName,1,macmWrite,&count,NULL,NULL)) == NULL)
    {
    MDB(6,fCONFIG) MLog("INFO:     cannot load dat file '%s'\n",
      DatFName);
    }

  /* load some config for HA purposes */

  MSchedLoadConfig(NULL,TRUE);

  return(SUCCESS);
  }  /* END MSchedPreLoadConfig() */





/**
 * Load SCHEDCFG[] Parameters from the moab configuration file.
 *
 * NOTE: LimitedLoad means we are only parsing loading limited values (for HA purposes).
 *       Currently only triggers are not loaded if LimitedLoad==TRUE, more will need to be
 *       added to this exclusion.
 *
 * @param Buf         (I) [optional]
 * @param LimitedLoad (I)
 */

int MSchedLoadConfig(

  char    *Buf,              /* I (optional) */
  mbool_t  LimitedLoad)      /* I */

  {
  char   IndexName[MMAX_NAME];

  char   Value[MMAX_LINE];

  char  *ptr;
  char  *head;

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
           MCredCfgParm[mxoSched],
           IndexName,
           NULL,
           Value,
           sizeof(Value),
           0,
           NULL) != FAILURE)
    {
    if (IndexName[0] != '\0')
      {
      /* set scheduler name */

      MSchedSetAttr(&MSched,msaName,(void **)IndexName,mdfString,mSet);
      }

    /* load sys specific attributes */

    MSchedProcessConfig(&MSched,Value,LimitedLoad);

    IndexName[0] = '\0';
    }  /* END while (MCfgGetSVal() != FAILURE) */

  MCfgEnforceConstraints(FALSE);
 
  return(SUCCESS);
  }  /* END MSchedLoadConfig() */





/**
 * Process attributes of the SCHEDCFG parameter.
 *
 * @see MSchedLoadConfig() - parent
 *
 * @param S           (I) [modified]
 * @param Value       (I)
 * @param LimitedLoad (I)
 */

int MSchedProcessConfig(

  msched_t *S,
  char     *Value,
  mbool_t   LimitedLoad)

  {
  int   aindex;

  enum MSchedAttrEnum AIndex;

  char *ptr;
  char *TokPtr;

  char  ValLine[MMAX_LINE];

  if ((S == NULL) ||
      (Value == NULL) ||
      (Value[0] == '\0'))
    {
    return(FAILURE);
    }

  /* process value line */

  ptr = MUStrTokE(Value," \t\n",&TokPtr);

  while (ptr != NULL)
    {
    /* parse name-value pairs */

    /* FOAMAT:  <VALUE>[,<VALUE>] */

    if (MUGetPair(
          ptr,
          (const char **)MSchedAttr,
          &aindex,
          NULL,
          TRUE,
          NULL,
          ValLine,                     /* O */
          sizeof(ValLine)) == FAILURE)
      {
      char tmpLine[MMAX_LINE];

      /* cannot parse value pair */

      snprintf(tmpLine,sizeof(tmpLine),"cannot parse attribute '%s'",
        ptr);
 
      MMBAdd(
        &MSched.MB,
        tmpLine,
        (char *)"N/A",
        mmbtOther,
        0,
        1,
        NULL);

      ptr = MUStrTokE(NULL," \t\n",&TokPtr);

      continue;
      }

    AIndex = (enum MSchedAttrEnum)aindex;

    switch (AIndex)
      {
      case msaAlias:

        {
        char *ptr;
        char *TokPtr;

        int   aindex;

        /* FORMAT: <ALIAS>[{, \t}<ALIAS>]... */

        aindex = 0;

        ptr = MUStrTok(ValLine,", \t\n",&TokPtr);

        while (ptr != NULL)
          {
          MUStrCpy(
            MSched.ServerAlias[aindex],
            ptr,
            sizeof(MSched.ServerAlias[0]));

          aindex++;

          if (aindex >= MMAX_HOSTALIAS)
            break;

          ptr = MUStrTok(NULL,", \t\n",&TokPtr);
          }  /* END while (ptr != NULL) */
        }    /* END (case msaAlias) */

        break;

      case msaJobMaxNodeCount:

        {
        long tmpL;

        tmpL = strtol(ValLine,NULL,10);

        if ((tmpL < MSched.M[mxoNode]) && (tmpL > 0))
          MSchedSetAttr(S,AIndex,(void **)ValLine,mdfString,mSet);
        }  /* END BLOCK */

        break;

      case msaChargeMetricPolicy:
      case msaChargeRatePolicy:
      case msaHALockCheckTime:
      case msaHALockFile:
      case msaHALockUpdateTime:
      case msaHomeDir:
      case msaHTTPServerPort:
      case msaSchedLockFile:
      case msaSpoolDir:
      case msaCheckpointDir:

        MSchedSetAttr(S,AIndex,(void **)ValLine,mdfString,mSet);

        break;

      case msaFBServer:

        MUURLParse(ValLine,NULL,S->AltServerHost,NULL,0,NULL,TRUE,TRUE);

        break;

      case msaFlags:

        bmfromstring(ValLine,MSchedFlags,&S->Flags);

        if (bmisset(&MSched.Flags,mschedfFastRsvStartup))
          {
          MSched.DisableMRE = TRUE;
          }

        break;

      case msaRecoveryAction:

        MSched.RecoveryAction = (enum MRecoveryActionEnum) MUGetIndex(ValLine,MRecoveryAction,FALSE,mrecaRestart);

        /* reset signal handlers to reflect change in recovery action config */

        MSysHandleSignals();

        break;

      case msaMinJobID:

        S->MinJobCounter = (int)strtol(ValLine,NULL,10);

        break; /* END case msaMinJobID */

      case msaMaxJobID:

        S->MaxJobCounter = (int)strtol(ValLine,NULL,10);

        break; /* END case msaMaxJobID */

      case msaMaxRecordedCJobID:

        S->MaxRecordedCJobID = (int)strtol(ValLine,NULL,10);

        break; /* END case msaMaxRecordedCJobID */

      case msaServer:

        {
        int tmpI;

        /* FORMAT:  <URL> */

        tmpI = S->ServerPort;

        MUURLParse(ValLine,NULL,S->ServerHost,NULL,0,&S->ServerPort,TRUE,TRUE);

        if (S->ServerPort <= 0)
          S->ServerPort = tmpI;
        }  /* END BLOCK */

        break;

      case msaMode:

        if (S->ForceMode == msmNONE)
          {
          S->Mode = (enum MSchedModeEnum)MUGetIndexCI(ValLine,MSchedMode,FALSE,S->Mode);

          if (S->Mode == msmTest)
            S->Mode = msmMonitor;

          if (S->Mode == msmSlave)
            S->State = mssmPaused;

          S->SpecMode = S->Mode;

          if ((S->Mode == msmSim) && (MSim.SpecStartTime != 0))
            {
            MSim.StartTime = MSim.SpecStartTime;

            MRsvAdjustTime(MSim.SpecStartTime - MSched.Time);

            MSched.Time = MSim.SpecStartTime;
            }
          }    /* END if (S->ForceMode == msmNONE) */

        break;

      case msaTrigger:

        if ((MSched.IsClient == TRUE) || (LimitedLoad == TRUE))
          {
          break;
          }
        else
          {
          int tindex;
   
          mtrig_t *T;
   
          MTrigLoadString(
            &S->TList,
            ValLine,
            TRUE,
            FALSE,
            mxoSched,
            S->Name,
            NULL,
            NULL);
   
          for (tindex = 0;tindex < S->TList->NumItems;tindex++)
            {
            T = (mtrig_t *)MUArrayListGetPtr(S->TList,tindex);
 
            if (MTrigIsValid(T) == FALSE)
              continue;
   
            MTrigInitialize(T);
            }   /* END for (tindex) */
          }     /* END BLOCK (case msaTrigger) */

        break;

      case msaVarList:

        /* FORMAT:  <name>[=<value>][[,<name[=<value]]...] */

        {
        mln_t *tmpL;

        char *ptr;
        char *TokPtr;

        char *TokPtr2;

        ptr = MUStrTok(ValLine,",",&TokPtr);

        while (ptr != NULL)
          {
          MUStrTok(ptr,"=",&TokPtr2);

          if (MULLAdd(&S->VarList,ptr,NULL,&tmpL,MUFREE) == FAILURE)
            break;

          if (TokPtr2[0] != '\0')
            {
            MUStrDup((char **)&tmpL->Ptr,TokPtr2);
            }

          ptr = MUStrTok(NULL,",",&TokPtr);
          }  /* END while (ptr != NULL) */
        }    /* END BLOCK (case msaVarList) */

        break;

      default:

        MDB(4,fAM) MLog("WARNING:  sys attribute '%s' not handled\n",
          MSchedAttr[AIndex]);

        break;
      }  /* END switch (AIndex) */

    ptr = MUStrTokE(NULL," \t\n",&TokPtr);
    }  /* END while (ptr != NULL) */

  return(SUCCESS);
  }  /* END MSchedProcessConfig() */
/* END MSchedConfig.c */
