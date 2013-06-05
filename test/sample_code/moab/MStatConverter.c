/* 
 * @file MStatConverter.c
 *
 * Created on November 18, 2008, 9:26 AM
 *
 * contains code used by the mstat_converter program
 */

#include "MStatConverter.h"
#include "mapi.h"


int MDBConvertFiles(

  char *EMsg)

  {
  /* Need to initialize some data structures for use by the external
   * program that converts old stat files to DB entries, PRIOR
   * to performing the actual conversion. These data structures 
   * are normally initialized during the start of moab proper, but here
   * need to be done as a one-off on behalf of the external program
   */
  MSched.M[mxoxGMetric] = MDEF_MAXGMETRIC;
  MGMetricInitialize();
  MSched.M[mxoxGRes]    = MDEF_MAXGRES;
  MGResInitialize();

  /* now configure the DB connection and then make the connection */
  if (MConfigureDBConn(EMsg) == FAILURE)
    {
    fprintf(stderr,"ERROR:    could not configure database connection: '%s'\n",EMsg);
    return(1);
    }
  else
    {
    fprintf(stdout,"Converting file-based statistics to a database of type '%s'\n",
        MDBType[MSched.MDB.DBType]);
    }

  /* Perform the translation from stat files to DB entries */
  if (MStatTransferToDB(&MSched.MDB,EMsg) == FAILURE)
    {
    fprintf(stderr,"ERROR: could not transfer statistics - '%s'\n",EMsg);
    }

  if (MCPTransferToDB(&MSched.MDB,EMsg) == FAILURE)
    {
    fprintf(stderr,"ERROR: could not transfer checkpoint information - '%s'\n",EMsg);
    }

  MDBFree(&MSched.MDB);

  return(0);
  }


/**
 * initialize Range
 *
 * @param Range (I/O)
 * @param StartTime (I)
 * @param TimeInterval (I)
 */

int MStatRangeInit(

  must_range_t *Range,
  mulong        StartTime,
  mulong        TimeInterval)

  {
  MUArrayListCreate(&Range->Stats,sizeof(mstat_union),10);
  Range->StartTime = StartTime;
  Range->TimeInterval = TimeInterval;

  return (SUCCESS);
  } /* END MStatRangeInit() */



/**
 * Clear resources associated with Range
 * @param Range (I)
 */

int MStatRangeClear(

  must_range_t *Range)

  {
  MUArrayListFree(&Range->Stats);

  return (SUCCESS);
  } /* END MStatRangeClear() */



/**
 * Append heap-allocated strings onto Out for every file in Dir that matches Regex
 *
 * Should we rewind Dir before starting? Should we save Dir's current position
 *   and reset it when done?
 *
 * @param Dir   (I) modified
 * @param Regex (I)
 * @param Out   (O) assumed to be an array of char *
 * @param EMsg  (O) [optional,minsize=MMAX_LINE]
 */

int MDirMatchingFiles(

  DIR *     Dir,
  regex_t  *Regex,
  marray_t *Out,
    char   *EMsg)

  {
  char fakeEMsg[1] = {0};
  char *ErrBuf;
  int ErrBufSize;
  struct dirent EntryStruct;
  struct dirent *Entry = NULL;
  int rc;

  if (EMsg == NULL)
    {
    ErrBuf = fakeEMsg;
    ErrBufSize = 0;
    }
  else
    {
    ErrBuf = EMsg;
    ErrBufSize = MMAX_LINE;
    }

#ifdef __SOLARISX86
    strncpy(ErrBuf,"Statconverter not supported on Solaris x86.", ErrBufSize);
    return(FAILURE);
#else

  /**get the latest non-empty events file */
  while (TRUE)
    {
    rc = readdir_r(Dir,&EntryStruct,&Entry);

    if (rc != 0)
      {
#ifndef MNOREENTRANT
      char errnoMsg[MMAX_LINE];

      strerror_r(errno,errnoMsg,sizeof(errnoMsg));

      snprintf(ErrBuf,ErrBufSize,"File system error: %s",errnoMsg);
#endif
      return(FAILURE); /*readdir error */
      }
    else if (Entry == NULL)
      break;

    if (regexec(Regex,Entry->d_name,0,NULL,0) == 0)
      {
      MUArrayListAppendPtr(Out,strdup(Entry->d_name));
      }
    }

  return (SUCCESS);
#endif
  } /* END MDirMatchingFiles() */



/**
 * Read the XML profile Profile into OutRange, overwriting existing data in OutRange.
 *
 * @param Profile    (I)
 * @param ObjectType (I)
 * @param OutRange   (O)
 * @param EMsg       (O) [optional,minsize=MMAX_LINE]
 */

int MStatReadProfile(

  mxml_t *Profile,
  enum MXMLOTypeEnum ObjectType,
  must_range_t *OutRange,
    char *EMsg)

  {
  char fakeEMsg[1] = {0};
  char *ErrBuf;
  int ErrBufSize;
  char tmpLine[MMAX_BUFFER];
  char tmpVal[MMAX_BUFFER];
  char *ptr;
  char *TokPtr;
  int StatIndex;
  int aindex;
  int MaxIndexSoFar = -1;
  int index;
  int rc;
  mulong Time;
  int saindex;
  enum MStatObjectEnum StatType;
  char const ** AttrNames;

  if (EMsg == NULL)
    {
    ErrBuf = fakeEMsg;
    ErrBufSize = 0;
    }
  else
    {
    ErrBuf = EMsg;
    ErrBufSize = MMAX_LINE;
    }

  StatType = ((ObjectType == mxoNode) ? msoNode : msoCred );
  AttrNames = (StatType == msoNode) ? MNodeStatType : MStatAttr;

  memset(OutRange->Stats.Array,0,MUArrayListByteSize(&OutRange->Stats));

  for (aindex = 0;aindex < Profile->ACount;aindex++)
    {
    char * const AttrName = Profile->AName[aindex];
    char *DotLoc;
    mbool_t BadGResSpecified = FALSE;
    int gindex = 0;

    saindex = MUGetIndex(AttrName,AttrNames,FALSE,0);

    if (saindex == 0)
      {
      if (!strcmp(AttrName,"SpecDuration"))
        {
        OutRange->TimeInterval = strtol((char *)Profile->AVal[aindex],NULL,10);
        continue;
        }
      else if (!strncasecmp(AttrName,"GMetric.",strlen("GMetric.")))
        {
        saindex = (StatType == msoNode) ? (int)mnstGMetric : (int)mstaGMetric;
        }
      else if ((StatType == msoNode) && ((DotLoc = strchr(AttrName,'.')) != NULL))
        {
        int PrefixSize = DotLoc - AttrName;

        if (!strncasecmp(AttrName,"AGRes",PrefixSize))
          {
          saindex = mnstAGRes;
          }
        else if (!strncasecmp(AttrName,"CGRes",PrefixSize))
          {
          saindex = mnstCGRes;
          }
        else if (!strncasecmp(AttrName,"DGRes",PrefixSize))
          {
          saindex = mnstDGRes;
          }
        else if (!strncasecmp(AttrName,"GRes",PrefixSize))
          {
          /*hack to deal with bug in MNodeProfileToXML existing in moab as of 11/20/2008.
           Treat the value as both mnstCGRes and mnstAGRes*/
          saindex = mnstCGRes;
          BadGResSpecified = TRUE;
          }

        if ((saindex == mnstCGRes) || (saindex == mnstAGRes) || (saindex == mnstDGRes))
          {
          gindex = MUMAGetIndex(meGRes,AttrName + PrefixSize + 1,mAdd);
          }
        }
      else if (MUGetIndex(AttrName,MProfAttr,FALSE,mpraNONE) != mpraNONE)
        {
        /* silently ignore */
        continue;
        }
      }

    if (saindex == 0)
      {
      fprintf(stderr,"WARNING:  unrecognized attribute %s\n",AttrName);
      continue;
      }

    if (((StatType == msoNode) && (saindex == mnstGMetric)) || ((StatType == msoCred) && (saindex == mstaGMetric)))
      {
      gindex = MUMAGetIndex(meGMetrics,AttrName + strlen(MNodeStatType[mnstGMetric]) + 1,mAdd) - 1;
      }

    MUStrCpy(tmpVal,Profile->AVal[aindex],sizeof(tmpVal));

    MUStrCpy(tmpLine,tmpVal,sizeof(tmpLine));

    if (   ((StatType == msoNode) && (saindex == mnstStartTime))
        || ((StatType == msoCred) && (saindex == mstaStartTime)))
      {
      int StartTime = strtol(tmpVal,NULL,10);

      if (StartTime != 0)
        OutRange->StartTime = StartTime;

      continue;
      }
    if ((strchr(tmpLine,'*') != NULL))
      {
      MStatPExpand(tmpLine,sizeof(tmpLine));
      MStatPExpand(tmpVal,sizeof(tmpVal));
      }

    ptr = MUStrTok(tmpVal,",",&TokPtr);

    StatIndex = 0;

    while ((ptr != NULL))
      {
      mstat_union *StatUnion;
      if (StatIndex >= OutRange->Stats.ArraySize)
        {
        int NewByteSize;
        int OldByteSize;

        OldByteSize = MUArrayListByteSize(&OutRange->Stats);
        rc = MUArrayListResize(&OutRange->Stats,OutRange->Stats.ArraySize * 2);

        if (rc == FAILURE)
          {
          snprintf(ErrBuf,ErrBufSize,"Out of memory");
          return(FAILURE);
          }

        NewByteSize = MUArrayListByteSize(&OutRange->Stats);
        memset(OutRange->Stats.Array + OldByteSize,0,NewByteSize - OldByteSize);
        }

      StatUnion = (mstat_union *)MUArrayListGet(&OutRange->Stats,StatIndex);

      if (ObjectType == mxoNode)
        {
        StatUnion->NStat.OType = ObjectType;
        StatUnion->NStat.XLoad = (double *)MUCalloc(1,sizeof(double) * MSched.M[mxoxGMetric]);

        MSNLInit(&StatUnion->NStat.CGRes);
        MSNLInit(&StatUnion->NStat.AGRes);
        MSNLInit(&StatUnion->NStat.DGRes);
        }
      else
        {
        StatUnion->GStat.OType = ObjectType;
        StatUnion->GStat.XLoad = (double *)MUCalloc(1,sizeof(double) * MSched.M[mxoxGMetric]);
        }

      if (StatIndex > MaxIndexSoFar)
        {
        MaxIndexSoFar = StatIndex;
        MStatPInit(StatUnion,StatType,StatIndex,0);
        }

      MStatSetAttr(
        (void *)StatUnion,
        StatType, /*FIXME */
        (int)saindex,
        (void **)ptr,
        gindex,
        mdfString,
        mSet);

      if (BadGResSpecified)
        {
        MStatSetAttr(
          (void *)StatUnion,
          StatType, /*FIXME */
          (int)mnstAGRes,
          (void **)ptr,
          gindex,
          mdfString,
          mSet);
        }

      StatIndex++;

      ptr = MUStrTok(NULL,",",&TokPtr);
      } /* END while ((ptr != NULL) && (S->IStatCount < PCount)) */
    }   /* END for (aindex) */

  if ((OutRange->TimeInterval == 0) || (OutRange->StartTime == 0))
    {
    /* If both are zero/missing then the record is bad and we caanot continue, report that fact */
    if ((OutRange->TimeInterval == 0) && (OutRange->StartTime == 0))
      {
      snprintf(ErrBuf,ErrBufSize,"XML data missing or corrupt: Both \"SpecDuration\" and \"StartTime\" are zero/missing");
      }
    else if (OutRange->TimeInterval == 0)
      {
      /* If just SpecDuration (TimeInterval) is zero then just report that */
      snprintf(ErrBuf,ErrBufSize,"XML data missing or corrupt: \"SpecDuration\" is zero/missing");
      }
    else /* otherwise, report that StartTime is zero/missing */
      {
      snprintf(ErrBuf,ErrBufSize,"XML data missing or corrupt: \"StartTime\" is zero/missing");
      }

    return(FAILURE);
    }

  Time = OutRange->StartTime;

  OutRange->Stats.NumItems = MaxIndexSoFar + 1;

  for (index = 0; index <= MaxIndexSoFar;index++)
    {
    mstat_union *StatUnion = (mstat_union *)MUArrayListGet(&OutRange->Stats,index);

    switch (StatType)
      {
      case msoCred:

        StatUnion->GStat.StartTime = Time;

        break;

      case msoNode:

        StatUnion->NStat.StartTime = Time;
        break;

      default:
        /* ERROR */
        break;
      }
    Time += OutRange->TimeInterval;
    }

  return (SUCCESS);
  } /* END MStatReadProfile() */



/**
 * Write profile data to DBConn
 *
 * @param DBConn     (I)
 * @param ObjectName (I)
 * @param ObjectType (I)
 * @param Range      (I)
 * @param EMsg       (O) [optional,minsize=MMAX_LINE]
 */

int MStatWriteProfileToDB(

  mdb_t              *DBConn,
  char const         *ObjectName,
  enum MXMLOTypeEnum  ObjectType,
  must_range_t       *Range,
  char               *EMsg)

  {
  enum MStatusCodeEnum SC = mscNoError;

  int index;
  int rc;

  for (index = 0;index < Range->Stats.NumItems;index++)
    {
    mstat_union *StatUnion = (mstat_union *)MUArrayListGet(&Range->Stats,index);

    if (ObjectType == mxoNode)
      {
      rc = MDBWriteNodeStats(DBConn,&StatUnion->NStat,ObjectName,EMsg,&SC);
      }
    else
      {
      rc = MDBWriteGeneralStats(DBConn,&StatUnion->GStat,ObjectName,EMsg,&SC);
      }

    if (rc == FAILURE)
      return(FAILURE);
    }

  return (SUCCESS);
  } /* END MStatWriteProfileToDB() */



/**
 * Write XML-based profiling statistics in FilePath to DBConn
 *
 * @param DBConn   (I)
 * @param FilePath (I)
 * @param EMsg     (I) [optional,minsize=MMAX_LINE]
 */

int MStatFileToDB(

  mdb_t      *DBConn,
  char const *FilePath,
  char       *EMsg)

  {
  char ELine[MMAX_LINE] = {0};
  char *ErrBuf;
  int ErrBufSize;
  must_range_t Range;
  int rc = SUCCESS;
  char *FileContents;
  mxml_t *Root = NULL;

  if (EMsg == NULL)
    {
    ErrBuf = ELine;
    }
  else
    {
    ErrBuf = EMsg;
    }

  ErrBufSize = MMAX_LINE;

  FileContents = MFULoadNoCache((char *)FilePath,1,NULL,NULL,NULL,NULL);

  if (FileContents == NULL)
    {
    snprintf(ErrBuf,ErrBufSize,"Error reading file");
    return(FAILURE);
    }

  MStatRangeInit(&Range,0,0);

  rc = MXMLFromString(&Root,FileContents,NULL,NULL);

  if (rc == FAILURE)
    {
    snprintf(ErrBuf,ErrBufSize,"XML for file %s is corrupt", FilePath);
    }
  else
    {
    mxml_t *Child;
    mxml_t *Profile;
    int CurPos = -1;
    int rc2;

    while (MXMLGetChild(Root,(char *)NULL,&CurPos,&Child) != FAILURE)
      {
      enum MXMLOTypeEnum ObjectType;
      char const * const ObjectTypeString = Child->Name;
      char const * const ObjectName = (Child->AVal == NULL) ? NULL : Child->AVal[0];

      char const *DBObjectName;
      enum MXMLOTypeEnum DBObjectType;

      if ((Child->ACount <= 0) ||
          (ObjectName == NULL))
        {
        fprintf(stderr,"WARNING:  XML element %s has no ID attribute. Skipping.\n",Child->Name);
        continue;
        }

      ObjectType = (enum MXMLOTypeEnum)MUGetIndexCI((char *)ObjectTypeString,MXO,FALSE,mxoNONE);

      /* scheduler profile statistics are stored in files as a sched object with
       * the name of the scheduler as configured in moab.cfg, whereas they 
       * are stored as a partition object with name "ALL". This is because 
       * scheduler profile statistics are actually stored in MPar[0].
       */

      if (ObjectType == mxoSched)
        {
        DBObjectType = mxoPar;
        DBObjectName = "ALL";
        }
      else
        {
        DBObjectType = ObjectType;
        DBObjectName = ObjectName;
        }

      if (DBObjectType == mxoNONE)
        {
        fprintf(stderr,"WARNING:  skipping profile %s of type `%s'. Type is invalid.\n",
            ObjectName,ObjectTypeString);

        continue;
        }

      if (DBObjectType == mxoNode)
        {
        mxml_t *StatElement = NULL;
        if (MXMLGetChild(Child,(char *)MXO[mxoxStats],NULL,&StatElement) == FAILURE)
          {
          fprintf(stderr,"WARNING:  Node element %s does not contain a Stats element. Skipping.\n",ObjectName);
          continue;
          }
        rc2 = MXMLGetChild(StatElement,(char *)MNodeStatType[mnstProfile],NULL,&Profile);
        }
      else
        {
        rc2 = MXMLGetChild(Child,(char *)MStatAttr[mstaProfile],NULL,&Profile);
        }

      if ((rc2 == FAILURE) || (Profile == NULL))
        {
        fprintf(stderr,"WARNING:  XML element %s does not contain a profile. Skipping.\n",Child->Name);
        continue;
        }

      rc2 = MStatReadProfile(Profile,DBObjectType,&Range,ErrBuf);

      if (rc2 == FAILURE)
        {
        fprintf(stderr,"ERROR:    reading profile for %s name=%s: %s.\n",
            MXO[ObjectType],
            ObjectName,
            ErrBuf);

        continue;
        }

      rc2 = MStatWriteProfileToDB(DBConn,DBObjectName,DBObjectType,&Range,ErrBuf);

      if (rc2 == FAILURE)
        {
        fprintf(stderr,"ERROR:    writing profile for %s name=%s: %s.\n",
            MXO[ObjectType],
            ObjectName,
            ErrBuf);

        continue;
        }

      printf("\tWrote %d profiling iterations for %s name=%s \n",
          Range.Stats.NumItems,
          MXO[ObjectType],
          ObjectName);
      }
    MXMLDestroyE(&Root);
    }

  MStatRangeClear(&Range);
  MUFree(&FileContents);
  return (rc);
  } /* END MStatFileToDB() */




/**
 * Transfer all currently stored file-based statistics to DBConn
 *
 * @param DBConn (I)
 * @param EMsg   (O) [optional,minsize=MMAX_LINE]
 */

int MStatTransferToDB(

    mdb_t *DBConn,
    char  *EMsg)

  {
  char fakeEMsg[1] = {0};
  char *ErrBuf;
  int ErrBufSize;
  marray_t FileNames;
  regex_t regex;
  DIR *Dir;
  int index;
  char StatsDir[MMAX_LINE << 2];
  char FilePath[MMAX_LINE << 2];
  char tmpEMsg[MMAX_LINE];
  int rc = SUCCESS;

  /* if no caller's emsg buffer, then fake one */
  if (EMsg == NULL)
    {
    ErrBuf = fakeEMsg;
    ErrBufSize = 0;
    }
  else
    {
    ErrBuf = EMsg;
    ErrBufSize = MMAX_LINE;
    }

  /* Form a Directory path to our Home Dir and open it*/
  snprintf(StatsDir,sizeof(StatsDir),"%s/stats",MUGetHomeDir());

  Dir = opendir(StatsDir);
  if (Dir == NULL)
    return (FAILURE);

  /* These are the files we are looking for */
  rc = regcomp(&regex,
      "^DAY\\."
      "[A-Z]\\{3\\}_" /* match the day name */
      "[A-Z]\\{3\\}_" /* match the month name */
      "[0-9]\\{1,2\\}_"  /* match the day of month */
      "[0-9]\\{4\\}$",    /* match the year */
      REG_ICASE|REG_NOSUB
      );

  assert (rc == 0);

  /* Now we need an Array data structure to manage this files we find in this dir */
  MUArrayListCreate(&FileNames,sizeof(char *),10);

  /* Find all the files that match the regex */
  rc = MDirMatchingFiles(Dir,&regex,&FileNames,tmpEMsg);

  if (rc == FAILURE)
    {
    snprintf(ErrBuf,ErrBufSize,"Error reading directory %s: %s",StatsDir,tmpEMsg);
    }
  else
    {
    /* Iterate over the files found and process each one */
    for (index = 0;index < FileNames.NumItems;index++)
      {
      char *File = *(char **)MUArrayListGet(&FileNames,index);

      snprintf(FilePath,sizeof(FilePath),"%s/%s",StatsDir,File);
      printf("Processing file %s\n",FilePath);

      rc = MStatFileToDB(DBConn,FilePath,tmpEMsg);

      if (rc == FAILURE)
        {
        snprintf(ErrBuf,ErrBufSize,"Error writing file %s to database: %s",FilePath,tmpEMsg);
        }

      MUFree(&File);
      }
    }

  closedir(Dir);
  regfree(&regex);
  MUArrayListFree(&FileNames);

  /* add existing checkpoint info to database */

  return (rc);
  } /* END MStatTransferToDB() */


int MCPTransferToDB(

  mdb_t *DBConn,
  char  *EMsg)

  {
  enum MStatusCodeEnum SC;

  char *ptr;
  char *TokPtr;

  char *Data;
  char *FileP;
  char  CPFile[MMAX_PATH];

  char  tmpName[MMAX_NAME + 1];
  char  OType[MMAX_NAME + 1];

  mulong  CkTime;

  snprintf(CPFile,sizeof(CPFile),"%s/.moab.ck",MUGetHomeDir());

  FileP = MFULoadNoCache(CPFile,1,NULL,NULL,NULL,NULL);

  if (FileP == NULL)
    {
    snprintf(EMsg,MMAX_LINE,"ERROR: could not open %s\n",CPFile);

    return(FAILURE);
    }

  ptr = MUStrTok(FileP,"\n",&TokPtr);

  while (ptr != NULL)
    {
    sscanf(ptr,"%64s %64s %ld",
      OType,
      tmpName,
      &CkTime);

    if ((Data = strchr(ptr,'<')) == NULL)
      {
      ptr = MUStrTok(NULL,"\n",&TokPtr);

      continue;
      }

    MDBInsertCP(DBConn,OType,tmpName,CkTime,Data,EMsg,&SC);

    ptr = MUStrTok(NULL,"\n",&TokPtr);
    }  /* END while (ptr != NULL) */ 

  MUFree(&FileP);

  return(SUCCESS);
  }  /* END MCPTransferToDB() */


/**
 * Return the path to the moab configuration path
 */
int MGetConfigFilePath(

  char *CfgFilePath,
  int CfgFilePathSize)

  {
  /* Generate a path to the '$homedir/moab.cfg' file */
  snprintf(CfgFilePath,CfgFilePathSize,"%s/%s",MUGetHomeDir(),"moab.cfg");

  return(SUCCESS);
  }

/**
 * Search the configuration file for the keyword 'KeyWard' and return
 * 'value' for the Keyword
 */
int MConfigMapKeyToValue(

  const char *KeyWord,
  char       *returnedValueString,
  char       *EMsg)

  {
  char CfgFilePath[MMAX_LINE << 2];
  char *CfgFileContents;
  char *FieldContext;
  char *LinePtr;
  char *FieldPtr;
  char *LineContext;
  int rc = FAILURE;

  /* Construct the file path to the configuration file */
  MGetConfigFilePath(CfgFilePath,sizeof(CfgFilePath));

  /* Now read in the contents of the file */
  CfgFileContents = MFULoadNoCache(CfgFilePath,1,NULL,NULL,NULL,NULL);

  /* check for NO return value from the file */
  if (CfgFileContents == NULL)
    {
    if (EMsg != NULL)
      {
      snprintf(EMsg,MMAX_LINE,"Could not load config file contents from '%s'", CfgFilePath);
      }
    return(rc);
    }

  LineContext = CfgFileContents;

  /* scan the file looking for the 'KeyWord' */
  while ((LinePtr = MUStrTok(NULL,"\n\r\f",&LineContext)) != NULL)
    {
    FieldContext = LinePtr;
    FieldPtr = MUStrTok(NULL," \t",&FieldContext);

    if (!strcasecmp(FieldPtr,KeyWord) && (returnedValueString != NULL))
      {
      /* Trim leading spaces/tabs */
      FieldPtr = MUStrTok(NULL," \t",&FieldContext);

      /* Found an entry so copy the value for the key and terminate the search */
      strcpy(returnedValueString, FieldPtr);
      rc = SUCCESS;
      break;
      }
    }

  if ((rc != SUCCESS) && (EMsg != NULL))
    snprintf(EMsg,MMAX_LINE,"No database type specified in config file: '%s'",CfgFilePath);

  MUFree(&CfgFileContents);
  return(rc);
  }


/**
 * load database type from moab's configuration file
 * and then connect to the appropriate database.
 *
 * Should we check moab.dat and any other sources as well?
 *
 * @return Success if a database type was found and connected to successfully,
 *   or FAILURE otherwise, error message is returned in EMsg if not NULL
 */

int MConfigureDBConn(

  char *EMsg)

  {
  MDBTypeEnum dbType;
  char ValueBuffer[MMAX_LINE << 2];
  int rc = FAILURE;

  /* Look for the USEDATABASE keyword and if found, get its value string back */
  rc = MConfigMapKeyToValue("USEDATABASE",ValueBuffer,EMsg);
  if (rc == SUCCESS)
    {
    /* Map the value to a Database type or the default */
    dbType = (enum MDBTypeEnum)MUGetIndex(
      ValueBuffer,
      MDBType,
      FALSE,
      mdbSQLite3);

    /* Initialize the database with the db type requested
     * then make a connection to the data base
     */
    rc = MDBInitialize(&MSched.MDB,dbType);
    if (rc == SUCCESS)
      {
      rc = MDBConnect(&MSched.MDB,EMsg);
      }
    else if (EMsg != NULL)
      {
        snprintf(EMsg,MMAX_LINE,"MDBInititalize() failed");
      }
    }

  return (rc);
  } /* END MConfigureDBConn() */
