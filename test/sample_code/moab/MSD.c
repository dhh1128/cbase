/* HEADER */

/**
 * @file MSD.c
 *
 * Moab Staging Data - This module contains most of the functions related to 
 *   Moab's data-staging capabilities in the grid.
 */

#include "moab.h"
#include "moab-proto.h"
#include "moab-global.h"
#include "moab-const.h"


/* globals */

mln_t *MSDJobHolds;  /* holds list of jobs that need data dependency holds when discovered by RM */

/* local prototypes */

int MSDConvertRMDataPath(mjob_t *J,mrm_t *,char *,mbool_t,char *,char *);
int MSDAToString(msdata_t *,enum MSDataAttrEnum,char *,int,enum MFormatModeEnum);
int MSDResolveURLVars(mjob_t *,char *,char *,int);
int __MSDGenerateURL(msdata_t *,mbool_t,char **);
int MSDFindDataReq(char *,mjob_t *,msdata_t **);
mbool_t MSDIsRMDataPathLocal(mrm_t *,char *);
void MSDBuildStageRequest(mjob_t *,char *,char *,char *,enum MDataStageTypeEnum,mstring_t *);



/**
 * 
 * 
 * @param DP (I) [modified]
 */

int MSDDestroy(

  msdata_t **DP)  /* I (modified) */

  {
  msdata_t *D;

  if ((DP == NULL) || (*DP == NULL))
    {
    return(SUCCESS);
    }

  D = *DP;

  if (D->Next != NULL)
    {
    /* use recursion to delete data chain */

    MSDDestroy(&D->Next);
    }

  MUFree(&D->SrcLocation);
  MUFree(&D->DstLocation);
  
  MUFree(&D->SrcProtocol);
  MUFree(&D->SrcFileName);
  MUFree(&D->SrcHostName);
  
  MUFree(&D->DstProtocol);
  MUFree(&D->DstFileName);
  MUFree(&D->DstHostName);
  MUFree(&D->EMsg);

  MUFree((char **)DP);

  return(SUCCESS);
  }  /* END MSDDestroy() */




/**
 *
 *
 * @param DP (O) [alloc]
 */

int MSDCreate(
 
  msdata_t **DP)  /* O (alloc) */
 
  {
  if (DP == NULL)
    {
    return(FAILURE);
    }

  if (*DP != NULL)
    {
    return(SUCCESS);
    }

  *DP = (msdata_t *)MUCalloc(1,sizeof(msdata_t));

  if (*DP == NULL)
    {
    return(FAILURE);
    }
 
  return(SUCCESS);
  }  /* END MSDCreate() */




#define MDEF_TRANSFERRATE 2000



/**
 *
 *
 * @param J (I)
 * @param SR (I)
 * @param FileURL (I)
 * @param FileSize (O) [bytes]
 * @param EMsg (O) [minsize=MMAX_LINE]
 */

int MSDGetFileSize(
  
  mjob_t *J,         /* I */
  mrm_t  *SR,        /* I */
  char   *FileURL,   /* I */
  mulong *FileSize,  /* O (bytes) */
  char   *EMsg)      /* O (minsize=MMAX_LINE) */

  {  
  /* use storage RM systemquery scripts to find out file size */

  char tmpBuffer[MMAX_BUFFER];
  char tmpEMsg[MMAX_LINE];

  char CmdLine[MMAX_LINE];
  char ResURL[MMAX_LINE];

  enum MStatusCodeEnum SC;

  if ((SR == NULL) ||
      (FileURL == NULL) ||
      (FileSize == NULL) ||
      (J == NULL))
    {
    return(FAILURE);
    }

  if (EMsg != NULL)
    EMsg[0] = '\0';

  *FileSize = 0;

  /* query source file size */

  MSDResolveURLVars(J,FileURL,ResURL,sizeof(ResURL));

  snprintf(CmdLine,sizeof(CmdLine),"%s %s",
    J->Credential.U->Name,
    ResURL);    

  /* FORMAT:  <USER> <FILEURL> */

  if (MRMSystemQuery(
       SR,
       "filesize",       /* I - cmd */
       CmdLine,          /* I - attribute */
       TRUE,             /* I - is blocking */
       tmpBuffer,        /* O (minsize=MMAX_BUFFER) */
       tmpEMsg,          /* O - EMsg (minsize=MMAX_LINE) */
       &SC) == FAILURE)
    {
    if (!strcasecmp(tmpEMsg,"badfile"))
      {
      if (EMsg != NULL)
        {
        sprintf(EMsg,"cannot query stagein file '%s'",
          ResURL);
        }

      /* NOTE:  destination file may not be queriable until data server has migrated data into place */
      }
    else if (!strcasecmp(tmpEMsg,"syntax"))
      {
      if (EMsg != NULL)
        {
        sprintf(EMsg,"invalid filequery/systemquery script '%s'",
          (SR->ND.URL[mrmSystemQuery] != NULL) ? SR->ND.URL[mrmSystemQuery] : "???");
        
        MMBAdd(&SR->MB,EMsg,NULL,mmbtNONE,0,0,NULL);
        }
      }
    else
      {
      /* query failed w/ unspecified error */

      if (EMsg != NULL)
        {
        sprintf(EMsg,"systemquery failed (%s)",
          tmpEMsg);
        }
      }

    return(FAILURE);
    }

  *FileSize = strtoul(tmpBuffer,NULL,10);

  return(SUCCESS);
  }  /* END MSDGetFileSize() */






/**
 * Parse RMPath into URL/File info.
 *
 * @param J (I) The job which "owns" the file paths fed into this function.
 * @param DRM (I) The RM whose stdout/stderr path needs to be converted.
 * @param RMPath (I) The RM's path of the file (<RM HOST>:<FILE PATH>).
 * @param IsRemote (I) Specifies whether or not the RM's file path should be considered local or remote.
 * @param OURL (O) The completed URL that can be given to the storage manager to reference the RM's file. [minsize=MMAX_LINE]
 * @param OFile (O) The filename ONLY of the completed URL. [optional,minsize=MMAX_NAME]
 */

int MSDConvertRMDataPath(

  mjob_t  *J,         /* I */
  mrm_t   *DRM,       /* I */
  char    *RMPath,    /* I */
  mbool_t  IsRemote,  /* I */
  char    *OURL,      /* O full path (minsize=MMAX_LINE) */
  char    *OFile)     /* O filename only (optional,minsize=MMAX_NAME) */

  {
  char tmpPath[MMAX_LINE];
  char *Host;
  char *Path;
  char *Tok = NULL;

  const char *FName = "MSDConvertRMDataPath";
  
  MDB(4,fGRID) MLog("%s(%s,%s,%s,%s,OURL,OFile)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (DRM != NULL) ? DRM->Name : "NULL",
    (RMPath != NULL) ? RMPath : "NULL",
    MBool[IsRemote]);

  if ((J == NULL) ||
      (DRM == NULL) ||
      (RMPath == NULL) ||
      (OURL == NULL))
    {
    return(FAILURE);
    }

  OURL[0] = '\0';

  if (OFile != NULL)
    OFile[0] = '\0';

  MUStrCpy(tmpPath,RMPath,sizeof(tmpPath));

  switch (DRM->Type)
    {
    case mrmtPBS:
    case mrmtMoab:

      /* FORMAT: <FQDN HOSTNAME>:<PATH> */

      /* extract hostname */
      
      Host = MUStrTok(tmpPath,":",&Tok);

      /* extract path */

      Path = MUStrTok(NULL,":",&Tok);

      if (Path == NULL)
        {
        MDB(3,fGRID) MLog("WARNING:  no path in data staging specification '%s' (bad format)\n",
          RMPath);

        MMBAdd(&J->MessageBuffer,"data staging path not specified",NULL,mmbtNONE,0,0,NULL);

        return(FAILURE);
        }

      break;
      
    default:

      MDB(3,fGRID) MLog("WARNING:  cannot stage-out stdout/stderr (unsupported RM type '%s')\n",
        MRMType[DRM->Type]);
      
      return(FAILURE);
      
      break;
    }  /* END switch (DRM->Type) */

  if (IsRemote == TRUE)
    {    
    /* TODO: need to make URL protocol specific? */

    if (Host == NULL)
      {
      Host = DRM->ClientName;
      }

    if (Host == NULL)
      {
      /* no remote host to reference */
      
      return(FAILURE);
      }
    
    snprintf(OURL,MMAX_LINE,"ssh://%s%s",
      Host,
      Path);
    }  /* END if (IsRemote == TRUE) */
  else
    {
    snprintf(OURL,MMAX_LINE,"file://%s",
      Path);    
    }

  if (OFile != NULL)
    {
    int cindex;
    
    /* extract filename from path */

    for (cindex = strlen(Path) - 1;cindex >= 0;cindex--)
      {
      if (Path[cindex] == '/')
        {
        break;
        }        
      }

    if (cindex > 0)
      {
      MUStrCpy(OFile,&Path[cindex + 1],MMAX_NAME);
      }
    }

  return(SUCCESS);
  }  /* END MSDConvertRMDataPath() */





/**
 *
 *
 * @param J (I)
 * @param R (I)
 * @param DataRM (I)
 * @param OMinStartDelay (O)
 * @param OMinWallTime (O) [optional]
 */

int MSDGetMinStageInDelay(

  mjob_t  *J,              /* I */
  mrm_t   *R,              /* I */
  mrm_t  **DataRM,         /* I */
  mulong  *OMinStartDelay, /* O */ 
  mulong  *OMinWallTime)   /* O (optional) */

  {
  mulong    MinStartDelay;
  mulong    MinWallTime;
  msdata_t *SD;
  mrm_t    *SR;    /* storage RM */
  int       TotalSIData;
  
  if ((J == NULL) ||
      (R == NULL) ||
      (DataRM == NULL) ||
      (OMinStartDelay == NULL))
    {
    return(FAILURE);
    }

  *OMinStartDelay = 0;

  if (OMinWallTime != NULL)
    *OMinWallTime = 0;

  /* for now just use DataRM[0] for storage RM */

  SR = DataRM[0];

  if (SR == NULL)
    {
    return(FAILURE);
    }

  MinStartDelay = 0;
  MinWallTime   = 0;
  TotalSIData   = 0;

  /* first take into account any minimum data-staging delays */

  if (SR != NULL)
    {
    MinStartDelay += SR->MinStageTime;
    }

  /* next determine how much data must be transfered */

  if (J->DataStage != NULL)
    {
    for (SD = J->DataStage->SIData;SD != NULL;SD = SD->Next)
      {
      TotalSIData += SD->SrcFileSize;
      }
    }

  if ((SR != NULL) && (TotalSIData > 0))
    {
    mulong tmpL;

    /* calculate min staging time (assume at least a MB/s staging bw) */

    tmpL = (long)(TotalSIData / MAX(1.0,SR->MaxStageBW));

    MinStartDelay = MAX(MinStartDelay, tmpL);
    }

  /* should be in earliest completion ONLY */

  if (R->MinExecutionSpeed > 0.0)
    {
    MinWallTime = (long)(J->SpecWCLimit[0] * R->MinExecutionSpeed);
    }
  else
    {
    MinWallTime = J->SpecWCLimit[0];
    }

  *OMinStartDelay = MinStartDelay;
  
  if (OMinWallTime != NULL)
    *OMinWallTime = MinWallTime;

  return(SUCCESS);
  }  /* END MSDGetMinStageInDelay() */




/**
 *
 *
 * @param Data (I)
 * @param AIndex (I)
 * @param Buf (O) [minsize=MMAX_LINE]
 * @param BufSize (I)
 * @param Mode (I)
 */

int MSDAToString(

  msdata_t            *Data,    /* I */
  enum MSDataAttrEnum  AIndex,  /* I */
  char                *Buf,     /* O (minsize=MMAX_LINE) */
  int                  BufSize, /* I */
  enum MFormatModeEnum Mode)    /* I */

  {
  if (Buf == NULL)
    {
    return(FAILURE);
    }

  Buf[0] = '\0';

  if (Data == NULL)
    {
    return(FAILURE);
    }

  switch (AIndex)
    {
    case msdaDstFileName:

      if (Data->DstFileName == NULL)
        break;

      MUStrCpy(Buf,Data->DstFileName,BufSize);      
      
      break;

    case msdaDstFileSize:

      snprintf(Buf,BufSize,"%ld",
        Data->DstFileSize);

      break;

    case msdaDstHostName:

      if (Data->DstHostName == NULL)
        break;

      MUStrCpy(Buf,Data->DstHostName,BufSize);

      break;

    case msdaDstLocation:

      if (Data->DstLocation == NULL)
        break;

      MUStrCpy(Buf,Data->DstLocation,BufSize);

      break;

    case msdaESDuration:

      snprintf(Buf,BufSize,"%ld",
        Data->ESDuration);

      break;

    case msdaIsProcessed:

      snprintf(Buf,BufSize,"%d",
        Data->IsProcessed);

      break;

    case msdaSrcFileName:

      if (Data->SrcFileName == NULL)
        break;

      MUStrCpy(Buf,Data->SrcFileName,BufSize);

      break;

    case msdaSrcFileSize:

      snprintf(Buf,BufSize,"%ld",
        Data->SrcFileSize);

      break;

    case msdaSrcHostName:

      if (Data->SrcHostName == NULL)
        break;

      MUStrCpy(Buf,Data->SrcHostName,BufSize);

      break;

    case msdaSrcLocation:

      if (Data->SrcLocation == NULL)
        break;

      MUStrCpy(Buf,Data->SrcLocation,BufSize);

      break;

    case msdaState:

      MUStrCpy(Buf,(char *)MDataStageState[Data->State],BufSize);

      break;

    case msdaTransferRate:

      snprintf(Buf,BufSize,"%f",
        Data->TransferRate);

      break;

    case msdaTStartDate:

      snprintf(Buf,BufSize,"%ld",
        Data->TStartDate);

      break;

    case msdaType:

      MUStrCpy(Buf,(char *)MDataStageType[Data->Type],BufSize);

      break;

    case msdaUpdateTime:

      snprintf(Buf,BufSize,"%ld",
        Data->UTime);

      break;

    default:

      /* not handled */

      return(FAILURE);

      /*NOTREACHED*/

      break;
    }

  return(SUCCESS);
  }  /* END MSDAToString() */





/**
 * Report Staging Data object as XML.
 *
 * @see MJobToXML() - parent
 *
 * @param Data (I)
 * @param E (O)
 * @param SMSDAList (I) [optional]
 * @param NullVal (I) [optional]
 */

int MSDToXML(

  msdata_t *Data,   /* I */
  mxml_t *E,        /* O */
  enum MSDataAttrEnum *SMSDAList, /* I (optional) */
  char   *NullVal)  /* I (optional) */

  {
  const enum MSDataAttrEnum DMSDAList[] = {
    msdaDstFileName,
    msdaDstFileSize,
    msdaDstHostName,
    msdaDstLocation,
    msdaESDuration,
    msdaIsProcessed,
    msdaSrcFileName,
    msdaSrcFileSize,
    msdaSrcHostName,
    msdaSrcLocation,
    msdaState,
    msdaTransferRate,
    msdaTStartDate,
    msdaType,
    msdaUpdateTime,
    msdaNONE };

  char tmpString[MMAX_LINE];
  int aindex;
  enum MSDataAttrEnum *MSDAList;

  if ((Data == NULL) ||
      (E == NULL))
    {
    return(FAILURE);
    }

  if (SMSDAList != NULL)
    {
    MSDAList = SMSDAList;
    }
  else
    {
    MSDAList = (enum MSDataAttrEnum *)DMSDAList;
    }

  for (aindex = 0;MSDAList[aindex] != msdaNONE;aindex++)
    {
    if ((MSDAToString(Data,MSDAList[aindex],tmpString,sizeof(tmpString),mfmXML) == FAILURE) ||
        (tmpString[0] == '\0'))
      {
      if (NullVal == NULL)
        continue;

      MUStrCpy(tmpString,NullVal,sizeof(tmpString));
      }

    MXMLSetAttr(E,(char *)MSDataAttr[MSDAList[aindex]],tmpString,mdfString);
    }  /* END for (aindex) */

  return(SUCCESS);
  }  /* END MSDToXML() */





/**
 *
 *
 * @param Data (I) [modified]
 * @param E (I)
 */

int MSDFromXML(

  msdata_t *Data, /* I (modified) */
  mxml_t   *E)    /* I */

  {
  int aindex;

  enum MSDataAttrEnum msdaindex;

  const char *FName = "MSDFromXML";

  MDB(2,fGRID) MLog("%s(%s,%s)\n",
    FName,
    (Data != NULL) ? "Data" : "NULL",
    (E != NULL) ? "E" : "NULL");

  if ((Data == NULL) || (E == NULL))
    {
    return(FAILURE);
    }

  /* NOTE:  do not initialize.  may be overlaying data */

  for (aindex = 0;aindex < E->ACount;aindex++)
    {
    msdaindex = (enum MSDataAttrEnum)MUGetIndexCI(E->AName[aindex],MSDataAttr,FALSE,0);

    if (msdaindex == msdaNONE)
      continue;

    MSDSetAttr(Data,msdaindex,(void **)E->AVal[aindex],mdfString,mSet);
    }  /* END for (aindex) */

  return(SUCCESS);
  }  /* END MSDFromXML() */




/**
 *
 *
 * @param Data (I) [modified]
 * @param AIndex (I)
 * @param Value (I)
 * @param Format (I)
 * @param Mode (I)
 */

int MSDSetAttr(

  msdata_t               *Data,   /* I (modified) */
  enum MSDataAttrEnum     AIndex, /* I */
  void                  **Value,  /* I */
  enum MDataFormatEnum    Format, /* I */
  enum MObjectSetModeEnum Mode)   /* I */

  {
  const char *FName = "MSDSetAttr";

  MDB(7,fSCHED) MLog("%s(%s,%s,Value,%u,%u)\n",
    FName,
    (Data != NULL) ? "Data" : "NULL",
    MSDataAttr[AIndex],
    Format,
    Mode);

  if (Data == NULL)
    {
    return(FAILURE);
    }

  switch (AIndex)
    {
    case msdaDstFileName:

      MUStrDup(&Data->DstFileName,(char *)Value);
          
      break;
      
    case msdaDstFileSize:

      {
      mulong tmpL;
        
      if (Format == mdfLong)
        tmpL = *(mulong *)Value;
      else
        tmpL = (mulong)strtol((char *)Value,NULL,10);

      Data->DstFileSize = tmpL;
      }  /* END BLOCK */

      break;
      
    case msdaDstHostName:

      MUStrDup(&Data->DstHostName,(char *)Value);

      break;
      
    case msdaDstLocation:

      MUStrDup(&Data->DstLocation,(char *)Value);

      break;
      
    case msdaESDuration:

      {
      mulong tmpL;
        
      if (Format == mdfLong)
        tmpL = *(mulong *)Value;
      else
        tmpL = (mulong)strtol((char *)Value,NULL,10);

      Data->ESDuration = tmpL;
      }  /* END BLOCK */

      break;
      
    case msdaIsProcessed:

      {
      int tmpI;
        
      if (Format == mdfLong)
        tmpI = *(int *)Value;
      else
        tmpI = (int)strtol((char *)Value,NULL,10);

      Data->IsProcessed = (mbool_t)tmpI;
      }  /* END BLOCK */
      
      break;
      
    case msdaSrcFileName:

      MUStrDup(&Data->SrcFileName,(char *)Value);

      break;
      
    case msdaSrcFileSize:

      {
      mulong tmpL;

      if (Format == mdfLong)
        tmpL = *(mulong *)Value;
      else
        tmpL = (mulong)strtol((char *)Value,NULL,10);

      Data->SrcFileSize = tmpL;
      }  /* END BLOCK */

      break;
      
    case msdaSrcHostName:

      MUStrDup(&Data->SrcHostName,(char *)Value);

      break;
      
    case msdaSrcLocation:

      MUStrDup(&Data->SrcLocation,(char *)Value);
      
      break;

    case msdaState:

      {
      int tmpI;

      if (Format == mdfInt)
        {
        tmpI = *(int *)Value;
        }
      else
        {
        tmpI = MUGetIndexCI((char *)Value,MDataStageState,FALSE,0);
        }

      if (tmpI != 0)
        Data->State = (enum MDataStageStateEnum)tmpI;
      }  /* END BLOCK */

      break;
      
    case msdaTransferRate:

      {
      mulong tmpL;

      if (Format == mdfLong)
        tmpL = *(mulong *)Value;
      else
        tmpL = (mulong)strtol((char *)Value,NULL,10);

      Data->TransferRate = tmpL;
      }  /* END BLOCK */

      break;
      
    case msdaTStartDate:

      {
      mulong tmpL;

      if (Format == mdfLong)
        tmpL = *(mulong *)Value;
      else
        tmpL = (mulong)strtol((char *)Value,NULL,10);

      Data->TStartDate = tmpL;
      }  /* END BLOCK */

      break;

    case msdaType:

      {
      int tmpI;

      if (Format == mdfInt)
        {
        tmpI = *(int *)Value;
        }
      else
        {
        tmpI = MUGetIndexCI((char *)Value,MDataStageType,FALSE,0);
        }

      if (tmpI != 0)
        Data->Type = (enum MDataStageTypeEnum)tmpI;
      }  /* END BLOCK */

      break;
      
    case msdaUpdateTime:

      {
      mulong tmpL;

      if (Format == mdfLong)
        tmpL = *(mulong *)Value;
      else
        tmpL = (mulong)strtol((char *)Value,NULL,10);

      Data->UTime = tmpL;
      }  /* END BLOCK */

      break;

    default:

      return(FAILURE);

      /*NOTREACHED*/

      break;
      
    }  /* END switch (AIndex) */

  return(SUCCESS);
  }  /* END MSDSetAttr() */





/**
 * Search through URL and replace all valid vars ($HOME, $DEST, $SUBMITHOST,
 * etc.) 
 * 
 * Supported Vars: $HOME, $DEST, $SUBMITHOST, $LOCALDATASTAGEHEAD, $JOBID
 *
 * @param J (I)
 * @param URL (I)
 * @param OURL (O)
 * @param OURLSize (I)
 */

int MSDResolveURLVars(

  mjob_t *J,        /* I */
  char   *URL,      /* I */
  char   *OURL,     /* O */
  int     OURLSize) /* I */
    
  {
  muenv_t EP;
  mrm_t *DRM = NULL;
  char *tmpVar;

  if ((J == NULL) ||
      (URL == NULL) ||
      (OURL == NULL))
    {
    return(FAILURE);
    } 

  OURL[0] = '\0';
  memset(&EP,0,sizeof(EP));

  /* search through URL and replace all valid vars ($HOME, $DEST, $SUBMITHOST, etc.) */
  
  /* init env */

  MUEnvAddValue(&EP,"HOME",J->Credential.U->HomeDir);

  /* if RHOME (Remote Home Directory) is needed it should have already been 
   * inserted into variables and control shoudln't be here unless it has. */

  if (MUHTGet(&J->Variables,"RHOME",(void **)&tmpVar,NULL) == SUCCESS)
    {
    MUEnvAddValue(&EP,"RHOME",tmpVar);
    }

  /* NOTE: $DEST only refers to primary RM found in Req[0] */

  if (J->Req[0] != NULL)
    {
    DRM = &MRM[J->Req[0]->RMIndex];

    if ((DRM->P != NULL) && (DRM->P->HostName != NULL))
      {    
      MUEnvAddValue(&EP,"DEST",DRM->P->HostName);

      MUHTAdd(&J->Variables,"DEST",strdup(DRM->P->HostName),NULL,MUFREE);
      }
    }

  if (J->SubmitHost != NULL)
    {
    MUEnvAddValue(&EP,"SUBMITHOST",J->SubmitHost);

    MUHTAdd(&J->Variables,"SUBMITHOST",strdup(J->SubmitHost),NULL,MUFREE);
    }

  if ((J->DataStage != NULL) && (J->DataStage->LocalDataStageHead != NULL))
    {
    MUEnvAddValue(&EP,"LOCALDATASTAGEHEAD",J->DataStage->LocalDataStageHead);

    MUHTAdd(&J->Variables,"LOCALDATASTAGEHEAD",strdup(J->DataStage->LocalDataStageHead),NULL,MUFREE);
    }

  MUEnvAddValue(&EP,"JOBID",J->Name);

  /* replace variables */

  if (MUInsertEnvVar(URL,&EP,OURL) == FAILURE)
    {
    /* couldn't replace */

    MUEnvDestroy(&EP);

    return(FAILURE);
    }

  MUEnvDestroy(&EP);

  return(SUCCESS);
  }  /* END MSDResolveURLVars() */



/**
 * Parses a data stage request string.
 *
 * Will destroy previous staging requests.
 *
 * @param Desc (I)
 * @param DSHead (O) [alloc]
 * @param Type (I)
 * @param EMsg (O) [optional]
 */

int MSDParseURL(

  char      *Desc,   /* I */
  msdata_t **DSHead, /* O (alloc) */
  enum MDataStageTypeEnum Type, /* I */
  char      *EMsg)   /* O (optional) */

  {
  char *TokPtr = NULL;
  char *SrcURL;
  char *DstURL;
  char *ptr;

  char *CurrRequest = NULL;
  char *CurrRequestTokPtr = NULL;

  char  DstProtocol[MMAX_NAME];
  char  DstHostName[MMAX_LINE];
  char  DstPath[MMAX_LINE];      /* can be either a file or directory */

  char  SrcProtocol[MMAX_NAME];
  char  SrcHostName[MMAX_LINE];
  char  SrcPath[MMAX_LINE];      /* directory path including filename */
  char  SrcFileName[MMAX_NAME];
  char  SrcFileSize[MMAX_LINE];  /* filename only - not path information included */

  msdata_t *tmpData = NULL;

  int index;
  int DataCount = 0;

  if ((Desc == NULL) || (DSHead == NULL))
    {
    return(FAILURE);
    }

  if (EMsg != NULL)
    EMsg[0] = '\0';

  /* parse source and destination URLS -  FORMAT: [<SRC_URL>[?<SIZE>][|...],]<DST_URL>&[<SRC_URL>... */

  /* Start fresh in creating staging requests and prevent memory leaks */

  if (*DSHead != NULL)
    {
    MSDDestroy(DSHead);

    *DSHead = NULL;
    }

  CurrRequest = MUStrTok(Desc,"&",&CurrRequestTokPtr);

  while (CurrRequest != NULL)
    {
    /* Prior to toruqe ~2.4 -W wasn't delimiting on ","s. Now that that is fixed
     * mstagein/out requests have problems because of the comma. Because of this
     * issue "%" was made a valid delimiter for src and dst urls. RT8313 */

    SrcURL = MUStrTok(CurrRequest," \t\n,%",&TokPtr);
    DstURL = MUStrTok(NULL," \t\n,%",&TokPtr);

    DstProtocol[0] = '\0';
    DstHostName[0] = '\0';
    DstPath[0] = '\0';

    if (DstURL != NULL)
      {
      if (MUURLParse(
            DstURL,
            DstProtocol,
            DstHostName,
            DstPath,
            sizeof(DstPath),
            NULL,
            TRUE,
            FALSE) == FAILURE)
        {
        /* Bad DstURL given! Don't assume where file goes! */

        if (EMsg != NULL)
          {
          snprintf(EMsg,MMAX_LINE,"cannot parse destination URL '%s'",
            DstURL);
          }
            
        return(FAILURE);
        }
      }

    /* Parse each segment of source URL */

    ptr = MUStrTok(SrcURL,"|",&TokPtr);

    DataCount = 0;

    while (ptr != NULL)
      {
      if (MUURLQueryParse(
            ptr,
            SrcProtocol,
            SrcHostName,
            NULL,
            SrcPath,
            sizeof(SrcPath),
            SrcFileSize,
            TRUE) == FAILURE)
        {
        /* Not valid URL */
    
        ptr = MUStrTok(NULL,"|",&TokPtr);

        continue;
        }

      /* extract file name */

      SrcFileName[0] = '\0';
      for (index = strlen(SrcPath);index >= 0;index--)
        {
        if (SrcPath[index] == '/')
          {
          MUStrCpy(SrcFileName,&SrcPath[index+1],sizeof(SrcFileName));
          break;
          }
        }

      if (DataCount != 0)
        {
        /* Append this new segment to data stage structure linked-list */

        if ((DstPath[0] != '\0') && (DstPath[strlen(DstPath) - 1] != '/'))
          {
          /* not a directory - cannot support multiple source files */

          /* Currently, destination paths must be unique in order to distinguish
          * between multiple data reqs for a job. See MSDUpdateDataReqs and
          * MSDFindDataReq. */

          if (EMsg != NULL)
            {
            snprintf(EMsg,MMAX_LINE,"destination URL must be a directory and end with '/' when staging multiple files, only the first file will be staged");
            }

          return(FAILURE);
          }
        }

      if (*DSHead == NULL)
        {
        /* Assign this first data stage structure to head */

        MSDCreate(&tmpData);
        *DSHead = tmpData;
        DataCount++;
        }
      else
        {
        MSDCreate(&(tmpData->Next));
        tmpData = tmpData->Next;
        DataCount++;
        }

      /* setup src data */

      tmpData->Type = Type;
      MUStrDup(&tmpData->SrcProtocol,SrcProtocol);
      MUStrDup(&tmpData->SrcHostName,SrcHostName);
      MUStrDup(&tmpData->SrcFileName,SrcPath);
      tmpData->SrcFileSize = strtol(SrcFileSize,NULL,10);

      __MSDGenerateURL(tmpData,TRUE,&tmpData->SrcLocation);

      /* setup dst data */

      if (DstURL != NULL)
        {
        MUStrDup(&tmpData->DstProtocol,DstProtocol);
        MUStrDup(&tmpData->DstHostName,DstHostName);

        if ((DataCount > 1) || 
            ((DstPath[0] != '\0') && (DstPath[strlen(DstPath) - 1] == '/')))
          {
          char tmpLine[MMAX_LINE];

          /* NOTE: at this point DstPath MUST be a directory - a check above prevents otherwise */

          snprintf(tmpLine,sizeof(tmpLine),"%s%s",
            DstPath,
            SrcFileName);

          MUStrDup(&tmpData->DstFileName,tmpLine);
          }
        else
          {
          MUStrDup(&tmpData->DstFileName,DstPath);
          }
        }
      else
        {
        /* if no DstURL was given, we will set the protocol and filename to be the same as source */
        
        MUStrDup(&tmpData->DstProtocol,SrcProtocol);
        MUStrDup(&tmpData->DstHostName,"$DEST");
        MUStrDup(&tmpData->DstFileName,SrcPath);
        }

      __MSDGenerateURL(tmpData,FALSE,&tmpData->DstLocation);

      /* duration is unknown until it is queried in MRMWorkloadQuery() */

      tmpData->ESDuration = MMAX_TIME;
      tmpData->UTime = MSched.Time;

      ptr = MUStrTok(NULL,"|",&TokPtr);
      }  /* END while (ptr != NULL) */

    CurrRequest = MUStrTok(NULL,"&",&CurrRequestTokPtr);
    } /* END while (CurrRequest != NULL) */

  return(SUCCESS);
  }  /* END MSDParseURL() */



/**
 * Genereate a new URL.
 *
 * @param Data (I)
 * @param IsSrc (I)
 * @param URL (O) [alloc]
 */

int __MSDGenerateURL(

  msdata_t *Data,   /* I */
  mbool_t   IsSrc,  /* I */
  char     **URL)   /* O (alloc) */
  
  {
  char tmpURL[MMAX_LINE * 2];

  if ((Data == NULL) || (URL == NULL))
    {
    return(FAILURE);
    }

  if (IsSrc == TRUE)
    {
    snprintf(tmpURL,sizeof(tmpURL),"%s://%s%s",
      (Data->SrcProtocol != NULL) ? Data->SrcProtocol : "file",
      (Data->SrcHostName != NULL) ? Data->SrcHostName : "",
      /* (!strcasecmp(Data->SrcProtocol,"file")) ? "" : "/", */
      (Data->SrcFileName != NULL) ? Data->SrcFileName : "");
    }
  else
    {
    snprintf(tmpURL,sizeof(tmpURL),"%s://%s%s",
      (Data->DstProtocol != NULL) ? Data->DstProtocol : "ssh",
      (Data->DstHostName != NULL) ? Data->DstHostName : "",
      /* (!strcasecmp(Data->DstProtocol,"file")) ? "" : "/", */
      (Data->DstFileName != NULL) ? Data->DstFileName : "");
    }

  MUStrDup(URL,tmpURL);

  return(SUCCESS);
  }  /* END MSDGenerateURL() */



/**
 *
 *
 * @param JobID (I)
 * @param HoldType (I)
 */

int MSDAddJobHold(

  char *JobID,                  /* I */
  enum MHoldTypeEnum HoldType)  /* I */

  {
  mln_t *LP;

  if (JobID == NULL)
    {
    return(FAILURE);
    }

  if (MULLAdd(&MSDJobHolds,JobID,NULL,&LP,MUFREE) == FAILURE)
    {
    return(FAILURE);
    }

  bmset(&LP->BM,HoldType);

  return(SUCCESS);
  }  /* END MSDAddJobHold() */



/**
 *
 *
 * @param JobID (I)
 * @param HoldType (I) [optional]
 */

int MSDGetJobHold(

  char *JobID,     /* I */
  enum MHoldTypeEnum  *HoldType)  /* I (optional) */

  {
  mln_t *LP;

  if (JobID == NULL)
    {
    return(FAILURE);
    }

  if (HoldType != NULL)
    {
    *HoldType = mhNONE;
    }

  if (MULLCheck(MSDJobHolds,JobID,&LP) == FAILURE)
    {
    return(FAILURE);
    }

  if (HoldType != NULL)
    {
    if (bmisset(&LP->BM,mhUser))
      {
      *HoldType = mhUser;
      }
    else if (bmisset(&LP->BM,mhSystem))
      {
      *HoldType = mhSystem;
      }
    else if (bmisset(&LP->BM,mhSystem))
      {
      *HoldType = mhBatch;
      }
    else if (bmisset(&LP->BM,mhSystem))
      {
      *HoldType = mhDefer;
      }
    }

  return(SUCCESS);
  }  /* END MSDGetJobHold() */



/**
 *
 *
 * @param JobID (I)
 * @param HoldType (I) [optional]
 */

int MSDRemoveJobHold(

  char *JobID,     /* I */
  enum MHoldTypeEnum  *HoldType)  /* I (optional) */

  { 
  if (JobID == NULL)
    {
    return(FAILURE);
    }

  /* extract value before deleting */

  if (MSDGetJobHold(JobID,HoldType) == FAILURE)
    {
    return(FAILURE);
    }

  if (MULLRemove(&MSDJobHolds,JobID,NULL) == FAILURE)
    {
    return(FAILURE);
    }
  
  return(SUCCESS);
  }  /* END MSDRemoveJobHold() */



/**
 *
 *
 * @param DstURL (I)
 * @param J (I)
 * @param OData (O) [optional]
 */

int MSDFindDataReq(

  char   *DstURL,    /* I */
  mjob_t *J,         /* I */
  msdata_t **OData)  /* O (optional) */

  {
  char tmpURL[MMAX_LINE];
  msdata_t *SD;
  
  if ((DstURL == NULL) ||
      (J == NULL))
    {
    return(FAILURE);
    }

  if (OData != NULL)
    {
    *OData = NULL;
    }

  /* if job is completed only search through stage-out reqs */

  if (MJOBISCOMPLETE(J) == TRUE)
    {
    SD = (J->DataStage != NULL) ? J->DataStage->SOData : NULL;
    }
  else
    {
    SD = (J->DataStage != NULL) ? J->DataStage->SIData : NULL;
    }

  while (SD != NULL)
    {
    if (SD->DstLocation == NULL)
      {
      SD = SD->Next;
      
      continue;
      }

    MSDResolveURLVars(J,SD->DstLocation,tmpURL,sizeof(tmpURL));

    if (!strcasecmp(tmpURL,DstURL))
      {
      /* found match! */

      if (OData != NULL)
        {
        *OData = SD;
        }

      return(SUCCESS);
      }

    SD = SD->Next;
    }

  return(FAILURE);
  }  /* END MSDFindDataReq() */
    




/**
 * Queries the given storage resource manager, getting data about in-transit 
 * data-staging operations.  Based on the data, it updates the respective jobs 
 * and their data staging depedencies.
 *
 * @param SR (I) The storage manager to query.
 * @param EMsg (O) [optional]
 */

int MSDUpdateDataReqs(

  mrm_t *SR,    /* I */
  char  *EMsg)  /* O (optional) */
  
  {
  msdata_t *SD;
  enum MStatusCodeEnum SC;
  mjob_t *J;

  char Results[MMAX_BUFFER];
  char tmpEMsg[MMAX_LINE];
  
  char *ptr;
  char *tmpPtr;
  
  char *TokPtr1;
  char *TokPtr2;

  char tmpURL[MMAX_LINE];
  char tmpJobID[MMAX_LINE];
  
  const char *FName = "MSDUpdateDataReqs";

  MDB(5,fRM) MLog("%s(%s,EMsg)\n",
    FName,
    (SR != NULL) ? SR->Name : "NULL");

  if (EMsg != NULL)
    {
    EMsg[0] = '\0';
    }

  if (SR == NULL)
    {
    return(FAILURE);
    }

  /* verify SR is indeed a storage RM */

  if (!bmisset(&SR->RTypes,mrmrtStorage) )
    {
    if (EMsg != NULL)
      MUStrCpy(EMsg,"storage manager misconfigured (resource type not set)",MMAX_LINE);

    return(FAILURE);
    }

  if (MNLIsEmpty(&SR->NL))
    {
    if (EMsg != NULL)
      MUStrCpy(EMsg,"storage manager missing node list - check cluster query",MMAX_LINE);

    return(FAILURE);
    }

  /* only calculate transfer rate if we have at least one stage req */

  /* query storage resource manager for all data reqs */

  if (MRMSystemQuery(
       SR,
       "",               /* I - cmd */
       "",               /* I - attribute */
       TRUE,             /* I - is blocking */
       Results,          /* O (minsize=MMAX_BUFFER) */
       tmpEMsg,          /* O - EMsg (minsize=MMAX_LINE) */
       &SC) == FAILURE)
    {
    if (EMsg != NULL)
      {
      sprintf(EMsg,"systemquery failed (%s)",
        tmpEMsg);
      }

    return(FAILURE);
    }

  /* process results, updating each job's data-reqs */

  /* FORMAT: <destination url>|<jobID> <FIELD>=<VALUE>;[<FIELD>=<VALUE>]... */

  ptr = MUStrTok(Results,"\n",&TokPtr1);

  while (ptr != NULL)
    {
    tmpPtr = MUStrTok(ptr," |",&TokPtr2);

    MUStrCpy(tmpURL,tmpPtr,sizeof(tmpURL));

    tmpPtr = MUStrTok(NULL," |",&TokPtr2);

    MUStrCpy(tmpJobID,tmpPtr,sizeof(tmpJobID));

    /* find job and update appropriate data req */

    if (MJobFind(tmpJobID,&J,mjsmBasic) == FAILURE)
      {
      if (MJobCFind(tmpJobID,&J,mjsmBasic) == FAILURE)
        {
        /* job no longer exists or mismatch has occured! */

        MDB(3,fRM) MLog("WARNING:  storage RM '%s' reporting data operation for non-existant job '%s'\n",
          SR->Name,
          tmpJobID);

        ptr = MUStrTok(NULL,"\n",&TokPtr1);

        continue;
        }
      }

    if (MSDFindDataReq(tmpURL,J,&SD) == FAILURE)
      {
      /* job has no matching data req! */

      ptr = MUStrTok(NULL,"\n",&TokPtr1);

      continue;
      }

    if (MWikiSDUpdate(TokPtr2,SD,SR) == FAILURE)
      {
      /* invalid input data */

      MDB(1,fRM) MLog("ALERT:    storage RM '%s' has returned invalid data for job '%s' (URL: %s)\n",
        SR->Name,
        tmpJobID,
        tmpURL);
      }

    ptr = MUStrTok(NULL,"\n",&TokPtr1);
    }  /* END while (ptr != NULL) */

  return(SUCCESS);
  }  /* END int MSDUpdateDataReqs() */





/**
 *
 *
 * @param J (I) [modified]
 * @param ETime (O) [optional]
 */

int MSDUpdateStageInStatus(

  mjob_t *J,     /* I (modified) */
  mulong *ETime) /* O (optional) */
  
  {
  /* ETime == MMAX_TIME when there are no more data staging requirements */
  
  mulong MaxETime = 0;

  mrm_t *DRM;
  msdata_t *SD;
  mbool_t HasDependency = FALSE;

  long DataRemaining;   /* bytes */
  mulong TimeRemaining;
  
  const char *FName = "MSDUpdateStageInStatus";

  MDB(5,fRM) MLog("%s(%s,ETime)\n",
    FName,
    (J != NULL) ? J->Name : "NULL");

  if (J == NULL)
    {
    return(FAILURE);
    }

  if (ETime != NULL)
    *ETime = MMAX_TIME;

  SD = (J->DataStage != NULL) ? J->DataStage->SIData : NULL;

  if (SD != NULL)
    {
    DRM = NULL;

    if (J->Req[0] != NULL)
      {
      /* FIXME: support multiple-reqs */

      DRM = &MRM[J->Req[0]->RMIndex];
      }
    
    if ((DRM == NULL) || bmisset(&DRM->IFlags,mrmifLocalQueue))
      {
      /* there is a data-req, but we can't determine if it is already fulfilled (need to migrate job first) */
      /* default to saying that it is NOT yet fulfilled until job is migrated */

      bmset(&J->IFlags,mjifDataDependency);

      return(SUCCESS);
      }

    /* NOTE:  RM (ie, TORQUE) may be handling data stage-in by itself, may not need warning */

    if ((DRM->DataRM == NULL) ||
        (DRM->DataRM[0] == NULL))
      {
      char tmpLine[MMAX_LINE];

      /* data-req found, but dest RM has no way to stage data */
      /* don't set a dependency, however, as RM may be handling stage-in externally */

      snprintf(tmpLine,sizeof(tmpLine),"job stage-in data reqs may not be satisfied - RMCFG[%s] has no 'DATARM' parameter",
        DRM->Name);

      MMBAdd(&DRM->MB,tmpLine,NULL,mmbtNONE,0,0,NULL);

      return(SUCCESS);
      }
    }  /* END if (SD != NULL) */

  while (SD != NULL)
    {
    /* calculate time required for this data req. */

    DataRemaining = SD->SrcFileSize - SD->DstFileSize;

    TimeRemaining = (mulong)((DataRemaining / MDEF_MBYTE) / MAX(1,MDEF_TRANSFERRATE));

    /* prevent erroneous assumption that DS is over */

    if ((DataRemaining > 0) || (SD->DstFileSize == 0))
      TimeRemaining += 1; 

    SD->ESDuration = TimeRemaining;

    /* check to see if data req. is finished */

    if ((SD->DstFileSize < SD->SrcFileSize) || (SD->ESDuration > 0))
      {
      /* check if dependency still exists */

      if (SD->State == mdssStaged)
        {
        /* dependency no longer exists */

        SD = SD->Next;

        continue;
        }

      MaxETime = MAX(MaxETime,TimeRemaining);

      HasDependency = TRUE;
      }

    if (SD->IsProcessed == FALSE)
      HasDependency = TRUE;

    SD = SD->Next;
    }  /* END while (SD != NULL) */

  if (HasDependency == TRUE)
    {
    MDB(7,fSCHED) MLog("INFO:     job '%s' has outstanding stage-in requirements\n",
      J->Name);

    bmset(&J->IFlags,mjifDataDependency);

    if (ETime != NULL)
      *ETime = MaxETime;

    return(SUCCESS);
    }

  /* mark that job has no outstanding data dependencies */

  bmunset(&J->IFlags,mjifDataDependency);

  if (ETime != NULL)
    *ETime = MMAX_TIME;

  return(SUCCESS);
  }  /* END int MSDUpdateStageInStatus() */





/**
 * Will process all of the data stage-in requirements attached to a job. This
 * function will send a data stage request to the storage manager which will
 * then handle the actual transfer. Each iteration the storage RM will report
 * to Moab on the different staging operations' progress.  Based on that
 * progress report, this function will mark the operation as finished. Once a
 * job's stage-in requirements are all finished or met, then all data holds
 * will be removed from the job and it should be permitted to run.
 *
 * If a stage-in file already exists on a remote system, the corresponding
 * stage-in req. will be marked as completed.
 *
 * @param Q (I) prioritized list of jobs,terminated w/-1 [optional]
 */

int MSDProcessStageInReqs(

  mjob_t **Q)  /* I (prioritized list of jobs,terminated w/-1 (optional)) */

  {
  int       jindex;

  mjob_t   *J;
  msdata_t *SD;
  mrm_t    *SR;
  mnode_t  *N;

  static mjob_t **tmpQ = NULL;

  int       ADisk;  /* available disk space (in MB) */

  char      EMsg[MMAX_LINE];
  char      tmpLine[MMAX_LINE];
  char      Value[MMAX_LINE];

  int       ActiveDSOp;
  int       AvailDSOp;
  int       dsindex;

  int       NumDSReqs;  /* number of unprocessed data-staging reqs (per job) */

  int RMList[MMAX_RM];
  int rmindex;
  int rqindex;
  int rmcount;

  mrm_t *DRM;

  const char *FName = "MSDProcessStageInReqs";

  MDB(4,fGRID) MLog("%s(%s)\n",
    FName,
    (Q != NULL) ? "Q" : "NULL");

  if (Q == NULL)
    {
    job_iter JTI;

    int qindex;

    /* create queue here */

    if (tmpQ == NULL)
      {
      if ((tmpQ = (mjob_t **)MUCalloc(1,sizeof(mjob_t *) * (MSched.M[mxoJob] + 1))) == NULL)
        {
        return(FAILURE);
        }
      }
    
    MOQueueInitialize(tmpQ);

    qindex = 0;

    MJobIterInit(&JTI);

    while (MJobTableIterate(&JTI,&J) == SUCCESS)
      {
      if (!bmisset(&J->IFlags,mjifDataDependency))
        continue;

      tmpQ[qindex] = J;

      qindex++;
      }  /* END for (J->Next) */

    /* terminate */

    tmpQ[qindex + 1] = NULL;

    Q = tmpQ;
    }  /* END if (Q == NULL) */

  /* process stage-in requests for jobs in priority queue */

  for (jindex = 0;Q[jindex] != NULL;jindex++)
    {
    J = Q[jindex];

    if (J == NULL)
      break;

    if (bmisset(&J->Flags,mjfSystemJob))
      {
      /* don't process system jobs */

      continue;
      }

    memset(RMList,0,sizeof(RMList));

    rmcount = 0;

    /* determine resource managers used */

    for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
      {
      if (J->Req[rqindex] == NULL)
        continue;

      /* ensure added RM is unique in RMList */

      for (rmindex = 0;rmindex < rmcount;rmindex++)
        {
        if (RMList[rmindex] == J->Req[rqindex]->RMIndex)
          {
          break;
          }
        }

      if (rmindex == rmcount)
        {
        RMList[rmindex] = J->Req[rqindex]->RMIndex;

        rmcount++;
        }
      }  /* END for (rqindex) */

    for (rmindex = 0;rmindex < rmcount;rmindex++)
      {
      DRM = &MRM[RMList[rmindex]];

      if ((DRM == NULL) || 
          bmisset(&DRM->IFlags,mrmifLocalQueue) ||
          (DRM->DataRM == NULL) ||
          (DRM->DataRM[0] == NULL))
        {
        /* only stage data for jobs that have already been migrated to non-local RM */

        continue;
        }
      
      SR = DRM->DataRM[0];

      if (!bmisset(&SR->RTypes,mrmrtStorage) || 
          (SR->State != mrmsActive) || 
          (MNLIsEmpty(&SR->NL)))  /* we use SR->NL later, so this cannot be empty or we'll segfault */
        {
        /* data can't be staged via non-storage RM */

        snprintf(tmpLine,sizeof(tmpLine),"data stage for RM '%s' not possible - it has no nodelist (check CLUSTERQUERYURL)",
          SR->Name);

        MDB(4,fGRID) MLog("WARNING:  %s\n",
          tmpLine);

        MMBAdd(&J->MessageBuffer,tmpLine,NULL,mmbtNONE,0,0,NULL);

        MJobSetHold(J,MSched.DataStageHoldType,MMAX_LINE,mhrData,tmpLine);

        continue;
        }

      N = MNLReturnNodeAtIndex(&SR->NL,0);

      if (N == NULL)
        {
        continue;
        }

      /* NOTE:  for storage managers, target usage is based on 'dedicated' resources *
       *        available resources are monitored and new staging operations prevented *
       *        if utilized resources exceed MaxUsage */

      /* NOTE:  if RM does not report available disk, available disk will be set to configured *
                disk */

      if (N->CRes.Disk > 0)
        {
        ADisk = N->ARes.Disk;

        if (SR->TargetUsage > 0.0)
          {
          int TargetDisk;

          /* (Configured * (Target/100)) - Utilized */

          TargetDisk = (int)(N->CRes.Disk * (SR->TargetUsage / 100.0));

          ADisk = MIN(
            ADisk,
            TargetDisk - N->DRes.Disk);
          }

        if (ADisk <= 0.0)
          {
          /* storage RM currently full, not accepting additional staging requests */

          MDB(4,fGRID) MLog("INFO:     storage manager '%s' is full (avail disk: %d MB/dedicated disk: %d)\n",
            N->Name,
            ADisk,
            N->DRes.Disk);

          continue;
          }
        }    /* END if (N->CRes.Disk) */
      else
        {
        ADisk = MMAX_INT;
        }

      if ((dsindex = MUMAGetIndex(meGRes,"dsop",mVerify)) > 0)
        {
        ActiveDSOp = MSNLGetIndexCount(&N->DRes.GenericRes,dsindex);
        AvailDSOp  = MSNLGetIndexCount(&N->CRes.GenericRes,dsindex);
        }
      else
        {
        ActiveDSOp = 0;
        AvailDSOp = MMAX_INT;
        }

      if (ActiveDSOp >= AvailDSOp)
        {
        /* storage RM currently full, not accepting additional staging requests */

        MDB(4,fGRID) MLog("INFO:     storage manager '%s' is busy (active storage operations: %d MB)\n",
          N->Name,
          ActiveDSOp);

        continue;
        }

      /* NOTE: J->SIData is updated every iteration in MSDUpdateStageInStatus(),
       * which is inside of MRMJobPostUpdate */

      SD = (J->DataStage != NULL) ? J->DataStage->SIData : NULL;

      NumDSReqs = -1;

      while (SD != NULL)
        {
        if ((SD->IsProcessed == TRUE) && MJOBISACTIVE(J))
          {
          /* already been fully processed - proceed */

          SD = SD->Next;

          continue;
          }
        else
          {
          if (NumDSReqs == -1)
            {
            NumDSReqs = 0;
            }

          NumDSReqs++;
          }

        /* if staging a file with differeing home dirs between hosts,
         * have to wait for the Remote Home Dir to be returned from 
         * the remote cluster. */

        if ((((SD->SrcLocation != NULL) && (strstr(SD->SrcLocation,"$RHOME") != NULL)) ||
             ((SD->DstLocation != NULL) && (strstr(SD->DstLocation,"$RHOME") != NULL))) &&
            (MUHTGet(&J->Variables,"RHOME",NULL,NULL) == FAILURE))
          {
          SD = SD->Next;

          continue;
          }

        switch (SD->State)
          {
          case mdssNONE:
            
            {
            /* data staging has not yet been started */

            /* NOTE: SD->SrcFileSize in bytes, ADisk in MB */

            int SrcSizeMB;

            int rc;
            char DstURL[MMAX_LINE];
            char SrcURL[MMAX_LINE];
            char tmpUName[MMAX_LINE];

            SrcSizeMB = SD->SrcFileSize / MDEF_MBYTE;

            if (SrcSizeMB > ADisk)
              {
              /* inadequate space to handle request */

              if (SrcSizeMB > N->CRes.Disk)
                {
                /* job can never be handled */

                MJobSetHold(J,mhBatch,MMAX_TIME,mhrData,"input data file too large");

                SD = NULL;

                continue;
                }

              /* stop staging data to allow space for current job ??? */

              SD = SD->Next;

              continue;
              }

            /* initiate staging request, but do not schedule */

            /* stage data only if a destination was specified */

            if (SD->DstLocation == NULL)
              break;

            /* FORMAT:  --stage <SRCURL> <DSTURL> --user <USER> [--mapped-user <USER2>]*/

            MSDResolveURLVars(J,SD->DstLocation,DstURL,sizeof(DstURL));
            MSDResolveURLVars(J,SD->SrcLocation,SrcURL,sizeof(SrcURL));

            MOMap(
              (SR->OMap != NULL) ? SR->OMap : DRM->OMap,
              mxoUser,
              J->Credential.U->Name,
              FALSE,
              FALSE,
              tmpUName);

            mstring_t Args(MMAX_LINE);

            MSDBuildStageRequest(
                J,
                SrcURL,
                DstURL,
                tmpUName,
                mdstStageIn,
                &Args);

            rc = MRMSystemModify(
                  SR,
                  "",
                  Args.c_str(),
                  TRUE,  /* IsBlocking */
                  Value,
                  mfmNONE,
                  EMsg,
                  NULL);

            if (rc == FAILURE)
              {
              /* NO-OP */

              snprintf(tmpLine,sizeof(tmpLine),"data stage in failed for job '%s' file '%s' (%s)",
                J->Name,
                SrcURL,
                EMsg);

              MDB(4,fGRID) MLog("WARNING:  %s\n",
                tmpLine);

              MMBAdd(&J->MessageBuffer,tmpLine,NULL,mmbtNONE,0,0,NULL);

              MJobSetHold(J,MSched.DataStageHoldType,MSched.DeferTime,mhrData,tmpLine);

              SD = SD->Next;

              continue;
              }

            /* set stage start date */

            SD->TStartDate = MSched.Time;

            /* remove newly dedicated space for stage-in job */

            ADisk -= SD->SrcFileSize / MDEF_MBYTE; /* ADisk in MB, SrcFileSize in bytes */
            }    /* END BLOCK */

            break;

          case mdssPending:
          
            /* data staging is currently underway */

            /* FIXME: make length for stalled transfers configurable */

            if ((MSched.Time - SD->UTime) > MCONST_MINUTELEN)
              {
              /* transfer is stalled */

              snprintf(tmpLine,sizeof(tmpLine),"data stage file %s not updated in %d seconds - could be stalled",
                SD->DstFileName,
                MCONST_MINUTELEN);

              MMBAdd(&J->MessageBuffer,tmpLine,NULL,mmbtNONE,0,0,NULL);
              }

            if (dsindex > 0)
              {
              MSNLSetCount(&N->DRes.GenericRes,dsindex,MSNLGetIndexCount(&N->DRes.GenericRes,dsindex) + 1);
              }

            break;

          case mdssStaged:

            /* data staging is complete */

            /* adjust dedicated disk (may need to do this after DS is first started, instead of here) */

            N->DRes.Disk += (SD->SrcFileSize / MDEF_MBYTE);

            /* cleanup - one req has been processed and can be eliminated from count */

            NumDSReqs--;

            SD->IsProcessed = TRUE;

            break;

          case mdssFailed:

            /* data staging has failed */

            /* adjust dedicated disk space here? */
            /* attempt to restart the operation after a given amount of time? */
            /* NYI */

            snprintf(tmpLine,sizeof(tmpLine),"data stage in failed for job '%s' file '%s' (%s)",
              J->Name,
              SD->SrcLocation,
              (SD->EMsg != NULL) ? SD->EMsg : "NULL");

            MDB(4,fGRID) MLog("WARNING:  %s\n",
              tmpLine);

            MMBAdd(&J->MessageBuffer,tmpLine,NULL,mmbtNONE,0,0,NULL);

            MJobSetHold(J,MSched.DataStageHoldType,MSched.DeferTime,mhrData,tmpLine);

            break;

          default:

            /* NO-OP */

            break;

          }  /* END switch (SD->State) */

        SD = SD->Next;
        }  /* END while (SD != NULL) */

      if (NumDSReqs == 0)
        {
        /* remove block from this job since no more unprocessed reqs exist */
        
        if (DRM->Type == mrmtMoab)
            /* && (!bmisset(&DRM->Flags,mrmfSlavePeer)))--this only works if we don't put on
             * jobs with data staging reqs on slaves (we were still putting the holds on, even
             * if the receiving peer was a slave, so the job would never run) */
          {
          if (MJobSetAttr(J,mjaHold,(void **)MHoldType[mhBatch],mdfString,mUnset) == FAILURE)
            {
            snprintf(tmpLine,sizeof(tmpLine),"cannot remove data staging block for job '%s'",
              J->Name);

            MDB(4,fGRID) MLog("WARNING:  %s\n",
              tmpLine);

            MMBAdd(&J->MessageBuffer,tmpLine,NULL,mmbtNONE,0,0,NULL);

            MJobSetHold(J,MSched.DataStageHoldType,MSched.DeferTime,mhrData,tmpLine);
            }
          else
            {
            /* remove hold locally--NOTE: this function already calls MRMJobModify for remote jobs... */

            /* MJobSetAttr(J,mjaHold,(void **)MHoldType[mhBatch],mdfString,mUnset); */
            }
          
          /* It takes time for the slave to remove the above hold ... this will usually
           * fail and cause another hold to be placed on the job. Just wait until next
           * iteration to try and start the job.
          if (bmisset(&DRM->Flags,mrmfSlavePeer))
            {
            if (MRMJobStart(J,EMsg,NULL) == FAILURE)
              {
              snprintf(tmpLine,sizeof(tmpLine),"post-data stage job start failed for job '%s' (%s)",
                J->Name,
                EMsg);

              MDB(2,fGRID) MLog("ALERT:    %s\n",
                tmpLine);

              MMBAdd(&J->MB,tmpLine,NULL,mmbtNONE,0,0,NULL);

              MJobSetHold(J,(1 << mhDefer),MSched.DeferTime,mhrData,tmpLine);
              }
            }
          */
          }
        else
          {
          if (MRMJobStart(J,EMsg,NULL) == FAILURE)
            {
            MDB(2,fGRID) MLog("ALERT:    post-data stage job start failed for job '%s' (%s)\n",
              J->Name,
              EMsg);

            MJobSetHold(J,MSched.DataStageHoldType,MSched.DeferTime,mhrData,"post-data stage job start failed");
            }
          }
        }
      }  /* END for (rmindex) */
    }    /* END for (jindex) */

  MOQueueDestroy(tmpQ,FALSE);

  return(SUCCESS);
  }  /* END MSDProcessStageInReqs() */





/**
 * All implicit data-staging operations (those involving the std. err and
 * output files) are handled in this function automatically. The files are
 * either staged back to the submission host (default) or will be staged back
 * to the configured local data stage head node.
 *
 * After the implicit data-stage operations are issued to the storage RM (which
 * actually performs the copy operations), explicit or user-specified stage-out
 * requirements are executed. Note that if a data stage operation fails, the
 * calling procedure usally attaches a message to the completed job, but the
 * job still completes successfully. In the future it will be important to
 * prevent the job from completing successfully until all data stage reqs. are
 * also successful. In case of a failure, the job should be blocked or given a
 * special state signifying the failed attempt to stage data back.
 *
 * @param J The job whose stage-out requirements should be handled. (I)
 */

int MSDProcessStageOutReqs(

  mjob_t *J)  /* I */

  {
  mrm_t *R;   /* workload RM */
  mrm_t *SR;  /* storage RM */

  char   tmpLine[MMAX_LINE];
  char   EMsg[MMAX_LINE];
  char   Value[MMAX_LINE];
  char   tmpUName[MMAX_LINE];
  mbool_t IsFailure = FALSE;

  msdata_t *SD;

  const char *FName = "MSDProcessStageOutReqs";

  MDB(4,fGRID) MLog("%s(%s)\n",
    FName,
    (J != NULL) ? J->Name : "NULL");

  /* NOTE: this function currently supports implicit stage-out reqs. and explicit.
   * Implict stage-out means the stdout and stderr files created by the underlying RM are
   * found and staged back to the submission host (where the job was originally submitted). */

  if (J == NULL)
    {
    return(FAILURE);
    }

  if (J->StartTime <= 0) /* Job never ran, nothing to stageout. */
    return(SUCCESS);

  if (MSched.EnableImplicitStageOut == FALSE)
    {
    /* stage-out reqs currently not available under current config */
    
    return(SUCCESS);
    }

#ifdef MNOT
  /* BOC RT5215 11-5-09 - Jobs can be migrated to a peer that will handle the staging. 
   * Preventing of processing on the src peer will be handled by the checks below,
   * since the DRM won't have a DataRM configured. */

  if (bmisset(&J->Flags,mjfClusterLock))
    {
    /* do not handle jobs that are locked to a local cluster! */

    return(SUCCESS);
    }
#endif

  if ((J->DestinationRM == NULL) ||
      (J->DestinationRM->DataRM == NULL) ||
      (J->DestinationRM->DataRM[0] == NULL))
    {
    /* no data RM to satisfy reqs - assuming externally managed */

    return(SUCCESS);
    }

  R = J->DestinationRM;
  SR = J->DestinationRM->DataRM[0];

  MOMap(
    (SR->OMap != NULL) ? SR->OMap : R->OMap,
    mxoUser,
    J->Credential.U->Name,
    FALSE,
    FALSE,
    tmpUName);

  /* Parse RM specific stdout/stderr paths and stage implicit data. 
   * Interactive jobs don't have stdout and stderr. */
  
  if (!bmisset(&J->Flags,mjfInteractive))
    {
    if (MSDExecuteStageOut(J,R,tmpUName,mjaStdOut,EMsg) == FAILURE)
      {
      IsFailure = TRUE;
      }
    
    if (MSDExecuteStageOut(J,R,tmpUName,mjaStdErr,EMsg) == FAILURE)
      {
      IsFailure = TRUE;
      }
    }
  
  /* process explicitly requested stage out operations */

  SD = (J->DataStage != NULL) ? J->DataStage->SOData : NULL;

  while (SD != NULL)
    {
    char DstURL[MMAX_LINE];
    char SrcURL[MMAX_LINE];

    /* initiate staging request, but do not schedule */
    /* stage data only if a destination was specified */

    if (SD->DstLocation != NULL)
      {
      /* FORMAT:  --stage <SRCURL> <DSTURL> --user <USER> [--mapped-user <USER2>]*/

      MSDResolveURLVars(J,SD->DstLocation,DstURL,sizeof(DstURL));
      MSDResolveURLVars(J,SD->SrcLocation,SrcURL,sizeof(SrcURL));

      mstring_t Args(MMAX_LINE);

      MSDBuildStageRequest(
          J,
          SrcURL,
          DstURL,
          tmpUName,
          mdstStageOut,
          &Args);

      if (MSched.DeleteStageOutFiles == TRUE)
        {
        MStringAppendF(&Args," --autoremove");
        }
         
      if ((J->DataStage != NULL) && 
          (J->DataStage->PostStagePath != NULL) && 
          bmisset(&J->DataStage->PostStageAfterType,mpdsfExplicit))
        {
        MStringAppendF(&Args," --post-stage-action %s",J->DataStage->PostStagePath);
        }

      if (MRMSystemModify(
            SR,
            "",
            Args.c_str(),
            TRUE,  /* IsBlocking */
            Value,
            mfmNONE,
            EMsg,
            NULL) == FAILURE)
        {
        /* NO-OP */

        snprintf(tmpLine,sizeof(tmpLine),"data stage out failed for job '%s' file '%s' (%s)",
          J->Name,
          SrcURL,
          EMsg);

        MDB(4,fGRID) MLog("WARNING:  %s\n",
          tmpLine);

        MMBAdd(&J->MessageBuffer,tmpLine,NULL,mmbtNONE,0,0,NULL);

        IsFailure = TRUE;
        }
      }  /* END if (SD->DstLocation != NULL) */

    SD = SD->Next;
    }  /* END while (SD != NULL) */

  if (IsFailure == TRUE)
    {
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MSDProcessStageOutReqs() */
 
 
  
  
  
/**
 * This file will perform a stage out data operation for the given job. The StageType parameter
 * determines whether it is a stdout or stderr file that is ultimately staged. Note that this
 * function will NOT stage files that are local--in other words, they are already in the desired
 * location.
 *
 * @param J (I)
 * @param R (I) [workload/dest. RM]
 * @param UName (I) [mapped username]
 * @param StageType (I) [mjaStdOut OR mjaStdErr]
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 */

int MSDExecuteStageOut(

  mjob_t *J,    /* I */
  mrm_t  *R,    /* I (workload/dest. RM) */  
  char *UName,  /* I (mapped username) */
  enum MJobAttrEnum StageType,  /* I type of file to stage (mjaStdOut OR mjaStdErr) */
  char *EMsg)   /* O (optional,minsize=MMAX_LINE) */
  
  {
  char RMFilePath[MMAX_PATH_LEN];
  char *JobFilePath;
  char *DstHost = NULL;
  char *SrcHost;
  char *SrcPath;
  char tmpLine[MMAX_LINE];
  char StageOutSrcURL[MMAX_PATH_LEN];
  char StageOutDstURL[MMAX_PATH_LEN];
  char StageOutDisplayURL[MMAX_PATH_LEN];  /* what output/error path will be shown via checkjob */
  char tmpName[MMAX_LINE];
  char Value[MMAX_LINE];
  
  mrm_t *SR;  /* storage RM */

  mbool_t DoPostStage = FALSE;

  const char *FName = "MSDExecuteStageOut";

  MDB(4,fGRID) MLog("%s(%s,%s,%s,%s,EMsg)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (R != NULL) ? R->Name : "NULL",
    (UName != NULL) ? UName : "NULL",
    MJobAttr[StageType]);

  if ((J == NULL) ||
      (R == NULL) ||      
      (UName == NULL))
    {
    return(FAILURE);
    }
    
  if (EMsg != NULL)
    EMsg[0] = '\0';

  StageOutDisplayURL[0] = '\0';
    
  SR = R->DataRM[0];
     
  if (StageType == mjaStdOut)
    {
    MUStrCpy(RMFilePath,J->Env.RMOutput,sizeof(RMFilePath));
    JobFilePath = J->Env.Output;

    if ((J->DataStage != NULL) &&
        (!bmisclear(&J->DataStage->PostStageAfterType)) &&
        (bmisset(&J->DataStage->PostStageAfterType,mpdsfStdOut) || 
         bmisset(&J->DataStage->PostStageAfterType,mpdsfStdAll)))
      {
      DoPostStage = TRUE;
      }
    }
  else if (StageType == mjaStdErr)
    {
    MUStrCpy(RMFilePath,J->Env.RMError,sizeof(RMFilePath));
    JobFilePath = J->Env.Error;

    if ((J->DataStage != NULL) && 
        (!bmisclear(&J->DataStage->PostStageAfterType)) &&
        (bmisset(&J->DataStage->PostStageAfterType,mpdsfStdErr) || 
         bmisset(&J->DataStage->PostStageAfterType,mpdsfStdAll)))
      {
      DoPostStage = TRUE;
      }
    }
  else
    {
    /* unsupported type */
    
    return(FAILURE);
    }

  mstring_t CmdLine(MMAX_LINE * 2);

  /* check to make sure that we are not trying to perform a stage operation when the
   * file is already at the local file system */

  if (MSDIsRMDataPathLocal(R,RMFilePath) == TRUE)
    {
    /* we don't need to move any files */

    /* Post data staging actions still required */

    if ((J->DataStage != NULL) && (J->DataStage->PostStagePath != NULL) && (DoPostStage == TRUE))
      {
      MStringSetF(&CmdLine,"--user %s --job %s --mapped-user %s --post-stage-action %s --post-stage-after-file %s",
        J->Credential.U->Name,
        J->Name,
        UName,
        J->DataStage->PostStagePath,
        J->Env.RMOutput);
      }
    else
      {
      return(SUCCESS);
      }
    }
  
  if (MSDConvertRMDataPath(
        J,
        R,
        RMFilePath,
        TRUE,
        StageOutSrcURL,
        tmpName) == SUCCESS)
    {
    /* first determine file's destination path */

    if (JobFilePath != NULL)
      {
      char tmpPath[MMAX_PATH];

      MJobResolveHome(J,JobFilePath,tmpPath,sizeof(tmpPath));

      if (tmpPath[0] != '/')
        {
        /* relative path specified */

        snprintf(tmpLine,sizeof(tmpLine),"%s/%s",
          J->Credential.U->HomeDir,
          tmpPath);

        MDB(7,fSCHED) MLog("INFO:     path is relative. HomeDir: %s Path: %s\n",
            J->Credential.U->HomeDir,
            tmpPath);
        }
      else
        {
        MUStrCpy(tmpLine,tmpPath,sizeof(tmpLine));
        }
      }
    else
      {
      snprintf(tmpLine,sizeof(tmpLine),"%s/%s.%c",
        J->Credential.U->HomeDir,
        J->Name,
        (StageType == mjaStdOut) ? 'o' : 'e');
      }

    /* now determine file's destination host */

    if ((J->DataStage != NULL) && 
        (J->DataStage->LocalDataStageHead != NULL) &&
        (MOSHostIsLocal(J->DataStage->LocalDataStageHead,FALSE) == FALSE))
      {
      snprintf(StageOutDstURL,sizeof(StageOutDstURL),"ssh://%s%s",
        J->DataStage->LocalDataStageHead,
        tmpLine);

      snprintf(StageOutDisplayURL,sizeof(StageOutDisplayURL),"%s:%s",
        J->DataStage->LocalDataStageHead,
        tmpLine);

      DstHost = J->DataStage->LocalDataStageHead; 
        
      MDB(7,fSCHED) MLog("INFO:     dest host is LocalDataStageHead (%s)\n",
        J->DataStage->LocalDataStageHead);
      }
    else if ((J->SubmitHost != NULL) &&
        (MOSHostIsLocal(J->SubmitHost,FALSE) == FALSE))
      {
      snprintf(StageOutDstURL,sizeof(StageOutDstURL),"ssh://%s%s",
        J->SubmitHost,
        tmpLine);

      snprintf(StageOutDisplayURL,sizeof(StageOutDisplayURL),"%s:%s",
        J->SubmitHost,
        tmpLine);

      DstHost = J->SubmitHost;

      MDB(7,fSCHED) MLog("INFO:     dest host is SubmitHost (%s)\n",
        J->SubmitHost);
      }
    else
      {
      snprintf(StageOutDstURL,sizeof(StageOutDstURL),"file://%s",
        tmpLine);

      snprintf(StageOutDisplayURL,sizeof(StageOutDisplayURL),"%s:%s",
        MSched.ServerHost,
        tmpLine);
      }

    MDB(7,fSCHED) MLog("INFO:     RMFilePath: %s StageOutSrcURL: %s StageOutDstURL: %s\n",
      (RMFilePath == NULL) ? "NULL" : RMFilePath,
      (StageOutSrcURL[0] == '\0') ? "" : StageOutSrcURL,
      (StageOutDstURL[0] == '\0') ? "" : StageOutDstURL);

    SrcHost = MUStrTok(RMFilePath,":",&SrcPath); 

    if (!strcmp(SrcPath,tmpLine) &&
        (MOSHostIsSame(DstHost,SrcHost) == TRUE))
      {
      /* Source and Destination are the same; do nothing */

      MDB(7,fSCHED) MLog("INFO:     stage-out src and dest paths are the same. src path: %s dst path: %s src host: %s dst host: %s\n",
        SrcPath,
        tmpLine,
        SrcHost,
        DstHost);

      /* Post data staging actions still required */

      if ((J->DataStage != NULL) && (J->DataStage->PostStagePath != NULL) && (DoPostStage == TRUE))
        {
        MStringSetF(&CmdLine,"--user %s --job %s --mapped-user %s --post-stage-action %s --post-stage-after-file %s",
          J->Credential.U->Name,
          J->Name,
          UName,
          J->DataStage->PostStagePath,
          SrcPath);
        }
      else
        {
        return(SUCCESS);
        }
      }
    else
      { 
      /* copy remote file */
      /* FORMAT:  --stage <SRCURL> <DSTURL> --user <USER> [--mapped-user <USER2>] [--autoremove] */

      MSDBuildStageRequest(
          J,
          StageOutSrcURL,
          StageOutDstURL,
          UName,
          mdstStageOut,
          &CmdLine);
      
      MStringAppendF(&CmdLine," --autoremove");

      if ((J->DataStage != NULL) && (J->DataStage->PostStagePath != NULL) && (DoPostStage == TRUE))
        {
        MStringAppendF(&CmdLine," --post-stage-action %s",J->DataStage->PostStagePath);
        }
      }
      
    if (MRMSystemModify(
          SR,
          "",
          CmdLine.c_str(),
          TRUE,    /* IsBlocking */
          Value,
          mfmNONE,
          EMsg,
          NULL) == FAILURE)
      {
      MDB(4,fGRID) MLog("WARNING:  data stage out failed for job '%s' file '%s' (%s)\n",
        J->Name,
        StageOutSrcURL,
        EMsg);

      snprintf(tmpLine,sizeof(tmpLine),"data stage out failed for file '%s' (%s)",
        StageOutSrcURL,
        EMsg);

      MMBAdd(&J->MessageBuffer,tmpLine,NULL,mmbtNONE,0,0,NULL);

      return(FAILURE);
      }
    }  /* END if (MSDConvertRMDataPath() == SUCCESS) */

  /* update Moab to report where the stdout/stderr files
   * ended up after staging */

  if ((StageType == mjaStdOut) &&
      (J->Env.Output == NULL))
    {
    MUStrDup(&J->Env.Output,StageOutDisplayURL);
    }
  else if ((StageType == mjaStdErr) &&
      (J->Env.Error == NULL))
    {
    MUStrDup(&J->Env.Error,StageOutDisplayURL);
    }
    
  return(SUCCESS);    
  }  /* END MSDExecuteStageOut() */





/**
 * Report TRUE if RM data is local.
 *
 * @param DRM (I)
 * @param RMPath (I)
 */

mbool_t MSDIsRMDataPathLocal(

  mrm_t  *DRM,       /* I */
  char   *RMPath)    /* I */

  {
  char tmpPath[MMAX_PATH];
  char *ptr;
  char *TokPtr;

  if ((DRM == NULL) ||
      (RMPath == NULL))
    {
    return(FALSE);
    }

  MUStrCpy(tmpPath,RMPath,sizeof(tmpPath));

  ptr = MUStrTok(tmpPath,":",&TokPtr);    

  if (ptr != NULL)
    {
    if (MOSHostIsLocal(ptr,FALSE))
      {
      return(TRUE);
      }
    }

  return(FALSE);
  }  /* END MSDIsRMDataPathLocal() */




/**
 * Generate generic stagein/out request.
 *
 * FORMAT:  --stage <SRCURL> <DSTURL> --user <USER> --type <STAGEIN|STAGEOUT> [--mapped-user <USER2>] 
 *
 * @param J (I)
 * @param SrcURL (I)
 * @param DstURL (I)
 * @param MappedName (I)
 * @param Type (I)
 * @param Request must be MStringInit'ed prior to call.
 */

void MSDBuildStageRequest(

  mjob_t    *J,      /*I*/
  char      *SrcURL, /*I*/
  char      *DstURL, /*I*/
  char      *MappedName, /*I*/
  enum MDataStageTypeEnum Type, /*I*/
  mstring_t *Request) /*O*/

  {
  MStringSetF(Request,"--stage %s %s --user %s --mapped-user %s --job %s --type %s",
    SrcURL,
    DstURL,
    J->Credential.U->Name,
    MappedName,
    J->Name,
    MDataStageType[Type]);

  /* Pass credentials for accouting */

  if (J->Credential.G != NULL)
    MStringAppendF(Request," --group %s",J->Credential.G->Name);

  if (J->Credential.A != NULL)
    MStringAppendF(Request," --account %s",J->Credential.A->Name);

  if (J->Credential.C != NULL)
    MStringAppendF(Request," --class %s",J->Credential.C->Name);

  if (J->Credential.Q != NULL)
    MStringAppendF(Request," --qos %s",J->Credential.Q->Name);

  } /* END int MSDBuildStageRequest() */





/**
 * Create an mds_t object.
 */

int MDSCreate(

  mds_t **DP)

  {
  mds_t *D;

  MAssert((DP != NULL),"internal error\n",__FUNCTION__,__LINE__);

  D = (mds_t *)MUCalloc(1,sizeof(mds_t));

  if (D == NULL)
    return(FAILURE);

  *DP = D;

  return(SUCCESS);
  }  /* END MDSCreate() */


/**
 * Free an mds_t object.
 */

int MDSFree(

  mds_t **DP)

  {
  mds_t *D;

  if (DP == NULL)
    return(FAILURE);

  D = *DP;

  if (D == NULL)
    return(FAILURE);

  if (D->SIData != NULL)
    MSDDestroy(&D->SIData);

  if (D->SOData != NULL)
    MSDDestroy(&D->SOData);

  MUFree((char **)DP);

  return(SUCCESS);
  }  /* END MDSFree() */


/**
 * Copy an mds_t object
 */

int MDSCopy(

  mds_t **DP,
  mds_t  *SP)

  {
  mds_t *D;

  if ((DP == NULL) || (SP == NULL))
    return(SUCCESS);

  if (*DP != NULL)
    {
    MDSFree(DP);
    }

  D = (mds_t *)MUCalloc(1,sizeof(mds_t));

  MUStrDup(&D->LocalDataStageHead,SP->LocalDataStageHead);

  MMovePtr((char **)&SP->SIData,(char **)&D->SIData);
  MMovePtr((char **)&SP->SOData,(char **)&D->SOData);

  return(SUCCESS);
  }  /* END MDSCopy() */

/* END MSD.c */
