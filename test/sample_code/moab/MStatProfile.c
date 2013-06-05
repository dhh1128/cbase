/* HEADER */

      
/**
 * @file MStatProfile.c
 *
 * Contains: MStatProfile
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 * Initialize profile stat (must_t) object/
 *
 * @param Stat (I) [modified/alloc]
 * @param ForceRecreate (I)
 * @param StatType (I)
 */

int MStatPInitialize(

  void                 *Stat,           /* I (modified/alloc) */
  mbool_t               ForceRecreate,  /* I */
  enum MStatObjectEnum  StatType)       /* I */

  {
  long    MidNight;

  mbool_t Reconfig = FALSE;

  const char *FName = "MStatPInitialize";

  MDB(7,fSCHED) MLog("%s(Stat,%s,%s)\n",
    FName,
    MBool[ForceRecreate],
    MStatObject[StatType]);

  /* NOTE: 'force' parameter re-creates profile structures 
           even if they already exist */

  if (MStat.P.IStatDuration == 0)
    {
    return(SUCCESS);
    }

  MUGetPeriodRange(MSched.Time,0,0,mpDay,&MidNight,NULL,NULL);

  while (MStat.P.MaxIStatCount * MStat.P.IStatDuration < MCONST_DAYLEN)
    {
    MStat.P.MaxIStatCount += 5;

    Reconfig = TRUE;
    }

  if (Reconfig == TRUE)
    {
    MDB(1,fCONFIG) MLog("WARNING:  invalid PROFILECOUNT specified, modified internally to '%ld' (see documentation)\n",
      (long)MStat.P.IStatDuration);
    }

  if ((Stat == NULL) && (StatType == msoCred))
    {
    int pindex;

    /* initialize global profiling */

    if ((ForceRecreate == TRUE) ||
        (MStat.P.IStatEnd == 0))
      {
      MStat.P.IStatEnd = 
        MSched.Time - 
        (MSched.Time % MAX(1,MStat.P.IStatDuration)) + 
        MStat.P.IStatDuration;
      }  /* END if (MStat.P.IStatEnd == 0) */

    /* enable default partition profiling */

    for (pindex = 0;pindex < MMAX_PAR;pindex++)
      {
      if (MPar[pindex].Name[0] == '\1')
        continue;

      if (MPar[pindex].Name[0] == '\0')
        break;
 
      MStatPInitialize((void *)&MPar[pindex].S,ForceRecreate,msoCred);
      }  /* END for (pindex) */

    return(SUCCESS);
    }  /* END if (Stat == NULL) */

  if ((Stat == NULL) && (StatType == msoNode))
    {
    mnode_t *N;

    mnust_t *SPtr = NULL;

    mbool_t GlobalUpdate = MSched.DefaultN.ProfilingEnabled;

    int nindex;

    /* loop through all nodes and initialize the stats structures */

    for (nindex = 0;nindex < MSched.M[mxoNode];nindex++)
      {
      N = MNode[nindex];
   
      if ((N == NULL) || (N->Name[0] == '\0'))
        break;
   
      if (N->Name[0] == '\1')
        continue;
   
      if (N->ProfilingEnabled == FALSE)
        continue;

      if ((N->ProfilingEnabled == MBNOTSET) && (GlobalUpdate != TRUE))
        continue;

      if ((GlobalUpdate == FALSE) && (N->ProfilingEnabled == FALSE))
        continue;
   
      if ((MOGetComponent((void *)N,mxoNode,(void **)&SPtr,mxoxStats) == SUCCESS) &&
          (SPtr->IStat == NULL))
        {
        MStatPInitialize((void *)SPtr,FALSE,msoNode);
        }
      }
    
    return(SUCCESS);
    }  /* END if ((Stat == NULL) && (StatType == msoCred)) */

  if (MStat.P.IStatEnd == 0)
    {
    /* if global profiling has not been initialized and we are attempting to
       initialize non-global profiling, then initialize global profiling */

    MStatPInitialize(NULL,FALSE,msoCred);

    /*
    MStat.P.IStatEnd =
      MSched.Time -
      (MSched.Time % MStat.P.IStatDuration) +
      MStat.P.IStatDuration;
    */
    }  /* END if (MStat.P.IStatEnd == 0) */

  switch (StatType)
    {
    case msoCred:

      {
      must_t *S = (must_t *)Stat;

      if (S->IStat != NULL)
        {
        if (ForceRecreate == TRUE)
          {
          int depth;
    
          if (S->IStat[0] != NULL)
            depth = S->IStat[0]->IStatCount;
          else
            depth = MStat.P.MaxIStatCount;

          MStatPDestroy((void ***)&S->IStat,msoCred,depth);

          S->IStat = (must_t **)MUCalloc(1,sizeof(must_t *) * MStat.P.MaxIStatCount);

          MStatCreate((void **)&S->IStat[0],msoCred);

          S->StartTime = MSched.Time;

          S->IStat[0]->IStatCount = MStat.P.MaxIStatCount;
          }
        }
      else
        {
        S->IStat = (must_t **)MUCalloc(1,sizeof(must_t *) * MStat.P.MaxIStatCount);

        MStatCreate((void **)&S->IStat[0],msoCred);

        S->IStat[0]->IStatCount = MStat.P.MaxIStatCount;

        S->StartTime = MSched.Time;
        }

      S->IStatCount = 1;

      MStatPInit(
        S->IStat[0],
        msoCred,
        0,
        MUGetStarttime(MAX(1,MStat.P.IStatDuration),MSched.Time));
      }  /* END case msoCred */

      break;

    case msoNode:

      {
      mnust_t *S = (mnust_t *)Stat;

      if (S->IStat != NULL)
        {
        if (ForceRecreate == TRUE)
          {
          int depth;
    
          if (S->IStat[0] != NULL)
            depth = S->IStat[0]->IStatCount;
          else
            depth = MStat.P.MaxIStatCount;

          MStatPDestroy((void ***)&S->IStat,msoNode,depth);

          S->IStat = (mnust_t **)MUCalloc(1,sizeof(mnust_t *) * MStat.P.MaxIStatCount);

          MStatCreate((void **)&S->IStat[0],msoNode);

          S->StartTime = MSched.Time;

          S->IStat[0]->IStatCount = MStat.P.MaxIStatCount;
          }
        }
      else
        {
        S->IStat = (mnust_t **)MUCalloc(1,sizeof(mnust_t *) * MStat.P.MaxIStatCount);

        MStatCreate((void **)&S->IStat[0],msoNode);

        S->IStat[0]->IStatCount = MStat.P.MaxIStatCount;

        S->StartTime = MSched.Time;
        }

      S->IStatCount = 1;

      MStatPInit(
        S->IStat[0],
        msoNode,
        0,
        MUGetStarttime(MAX(1,MStat.P.IStatDuration),MSched.Time));
      }  /* END case msoNode */

      break;

    default:

      /* NOT HANDLED */

      break;
    } /* END switch (StatType) */

  return(SUCCESS);
  }  /* MStatPInitialize() */





/**
 *
 *
 * @param Stat (I) [modified]
 * @param StatType (I)
 * @param SIndex (I)
 * @param StartTime (I)
 */

int MStatPInit(

  void                 *Stat,      /* I (modified) */
  enum MStatObjectEnum  StatType,  /* I */
  int                   SIndex,    /* I */
  long                  StartTime) /* I */

  {
  const char *FName = "MStatPInit";

  MDB(7,fSCHED) MLog("%s(Stat,%s,%d,%ld)\n",
    FName,
    MStatObject[StatType],
    SIndex,
    StartTime);

  if (Stat == NULL)
    {
    return(FAILURE);
    }

  switch (StatType)
    {
    case msoCred:

      {
      must_t *S = (must_t *)Stat;

      S->StartTime = StartTime;

      S->IsProfile = TRUE;

      S->Index     = SIndex;

      /* set initial values */

      /* NO-OP */
      }  /* END case msoCred */

      break;

    case msoNode:

      {
      mnust_t *S = (mnust_t *)Stat;

      S->StartTime = StartTime;

      S->IsProfile = TRUE;

      S->Index     = SIndex;

      /* set initial values */

      }

      break;

    default:

      /* NO-OP */

      break;
    }  /* END switch (StatType) */

  return(SUCCESS);
  }  /* END MStatPInit() */




/**
 * Free/destroy profile stat (must_t or mnust_t) array.
 *
 * @param Stat (I) [modified]
 * @param StatType (I)
 * @param Depth (I)
 */

int MStatPDestroy(

  void                 ***Stat,       /* I (modified) */
  enum MStatObjectEnum    StatType,   /* I */
  int                     Depth)      /* I */

  {
  int      sindex;

  const char *FName = "MStatPDestroy";

  MDB(7,fSCHED) MLog("%s(Stat,%s,%d)\n",
    FName,
    MStatObject[StatType],
    Depth);

  if ((Stat == NULL) || (*Stat == NULL))
    {
    return(SUCCESS);
    }

  switch (StatType)
    {
    case msoCred:
   
      {
      must_t **SList;
      must_t  *SPtr;

      SList = (must_t **)*Stat;

      if (Depth == -1)
        {
        if (SList[0] != NULL)
          Depth = SList[0]->IStatCount;
        else
          Depth = MStat.P.MaxIStatCount;
        }

      for (sindex = 0;sindex < Depth;sindex++)
        {
        SPtr = SList[sindex];

        if (SPtr != NULL)
          MStatFree((void **)&SPtr,msoCred);
        }  /* END for (sindex) */

      free(SList);

      *Stat = NULL;
      }

      break;
    
    case msoNode:

      {
      mnust_t **SList;
      mnust_t  *SPtr;

      SList = (mnust_t **)*Stat;

      if (Depth == -1)
        {
        if (SList[0] != NULL)
          Depth = SList[0]->IStatCount;
        else
          Depth = MStat.P.MaxIStatCount;
        }

      for (sindex = 0;sindex < Depth;sindex++)
        {
        SPtr = SList[sindex];

        if (SPtr != NULL)
          MStatFree((void **)&SPtr,msoNode);
        }  /* END for (sindex) */

      free(SList);

      *Stat = NULL;
      }  /* END case mxoNode */
 
      break;

    default:

      /* NO-OP */

      break;
    }  /* END switch (StatType) */

  return(SUCCESS);
  }  /* END MStatPDestroy() */





/* update profile statistics for partitions, nodes, and credentials */

int MStatPAdjust(void)

  {
  int pindex;
 
  mpar_t  *P;

  void    *OE;
  void    *O;
  void    *OP;

  char    *OName;

  mhashiter_t HTI;

  must_t  **SList;

  must_t  *S;

  mnode_t *N;

  mbool_t  UpdateStats;

  int nindex;
  int jindex;
  int oindex;

  mjob_t *TJ;

  enum MXMLOTypeEnum OList[] = {
    mxoUser,
    mxoGroup,
    mxoAcct,
    mxoQOS,
    mxoClass,
    mxoNONE };

  const char *FName = "MStatPAdjust";

  MDB(7,fSCHED) MLog("%s()\n",
    FName);

  if (MStat.P.IStatEnd <= 0)
    {
    /* profiling not enabled */
 
    return(SUCCESS);
    }

  if ((long)MSched.Time < MStat.P.IStatEnd)
    {
    /* current profile step still valid */

    UpdateStats = FALSE;
    }
  else
    {
    UpdateStats = TRUE;

    /* roll all credentials */

    for (oindex = 0;OList[oindex] != mxoNONE;oindex++)
      {
      MOINITLOOP(&OP,OList[oindex],&OE,&HTI);

      while ((O = MOGetNextObject(&OP,OList[oindex],OE,&HTI,&OName)) != NULL)
        {
        if (MOGetComponent(O,OList[oindex],(void **)&S,mxoxStats) == FAILURE)
          continue;

        if (S->IStat == NULL)
          continue;

        SList = S->IStat;

        MStatPRoll(
          OList[oindex],
          OName,
          (void **)SList,
          msoCred,
          &S->IStatCount,
          MStat.P.MaxIStatCount);

        if (S->IStatCount >= 2)
          {
          SList[S->IStatCount - 1]->DSUtilized = 
            (SList[S->IStatCount - 2] != NULL) ? SList[S->IStatCount - 2]->DSUtilized : 0;

          SList[S->IStatCount - 1]->IDSUtilized = 
            (SList[S->IStatCount - 2] != NULL) ? SList[S->IStatCount - 2]->IDSUtilized : 0;
          }
        }  /* END while((O = MOGetNextObject(&OP,OList[oindex],OE,&OName)) != NULL) */
      }    /* END for (oindex) */

    /* roll all nodes */

    for (nindex = 0;nindex < MSched.M[mxoNode];nindex++)
      {
      N = MNode[nindex];

      if ((N == NULL) || (N->Name[0] == '\0'))
        break;

      if (N->Name[0] == '\1')
        continue;

      if ((MSched.DefaultN.ProfilingEnabled == FALSE) &&
          (N->ProfilingEnabled == FALSE))
        continue;

      if ((N->Stat.IStat != 0) && (UpdateStats == FALSE))
        continue;

      MStatPRoll(
        mxoNode,
        N->Name,
        (void **)N->Stat.IStat,
        msoNode,
        &N->Stat.IStatCount,
        MStat.P.MaxIStatCount);

      if (N->Stat.IStatCount > 0)
        {
        MNodeUpdateProfileStats(N);
/*
        MNodeGetCat(N,&N->Stat.IStat[N->Stat.IStatCount - 1]);
*/
        }
      }   /* END for (nindex) */

    /* roll all job templates with stats */

    for (jindex = 0;MUArrayListGet(&MTJob,jindex) != NULL;jindex++)
      {
      TJ = *(mjob_t **)(MUArrayListGet(&MTJob,jindex));

      if (TJ == NULL)
        break;

      if (TJ->ExtensionData == NULL)
        continue;

      if (((mtjobstat_t *)TJ->ExtensionData)->S.IStat == NULL)
        continue;

      SList = ((mtjobstat_t *)TJ->ExtensionData)->S.IStat;
      S = &((mtjobstat_t *)TJ->ExtensionData)->S;

      MStatPRoll(
        mxoJob,
        TJ->Name,
        (void **)SList,
        msoCred,
        &S->IStatCount,
        MStat.P.MaxIStatCount);
      }   /* END for (jindex) */

    /* roll all active partition profile intervals */

    for (pindex = 1;pindex < MMAX_PAR;pindex++)
      {
      P = &MPar[pindex];
 
      if (P->Name[0] == '\1')
        continue;

      if (P->Name[0] == '\0')
        break;
 
      if (P->ConfigNodes <= 0)
        continue;

      if (P->S.IStat == NULL)
        continue;

      SList = P->S.IStat;

      MStatPRoll(
        mxoPar,
        P->Name,
        (void **)SList,
        msoCred,
        &P->S.IStatCount,
        MStat.P.MaxIStatCount);
      }   /* END for (pindex) */

    /* NOTE: there are lots of stats that rely on MPar[0].S so we can roll that until the very end */

    if (MPar[0].S.IStat != NULL)
      {
      P = &MPar[0];

      SList = P->S.IStat;

      MStatPRoll(
        mxoPar,
        P->Name,
        (void **)SList,
        msoCred,
        &P->S.IStatCount,
        MStat.P.MaxIStatCount);
      }
    }     /* END else () */

  if ((long)MSched.Time >= MStat.P.IStatEnd)
    {
    /* adjust global stat profile data */

    MStat.P.IStatEnd += MStat.P.IStatDuration;
    }

  return(SUCCESS);
  }  /* END MStatPAdjust() */




/**
 * Roll profile statistics (must_t,mnust_t).
 *
 * @param OType
 * @param OID
 * @param StatList (I) [modified]
 * @param StatType (I)
 * @param SCount (I) [modified]
 * @param Depth (I)
 */

int MStatPRoll(

  enum MXMLOTypeEnum      OType,     /* I */
  char                   *OID,       /* I */
  void                  **StatList,  /* I (modified) */
  enum MStatObjectEnum    StatType,  /* I */
  int                    *SCount,    /* I (modified) */
  int                     Depth)     /* I */

  {
  int sindex;

  int SIndex;

  const char *FName = "MStatPRoll";

  MDB(7,fSCHED) MLog("%s(StatList,%s,SCount,^%d)\n",
    FName,
    MStatObject[StatType],
    Depth);

  if ((StatList == NULL) || 
      (SCount == NULL) ||
      (OType == mxoNONE) ||
      (OID == NULL))
    {
    return(FAILURE);
    }

  SIndex = MIN(*SCount - 1,Depth - 1);

  switch (StatType)
    {
    case msoCred:
    
      {
      must_t  *S;
      must_t **SList;
 
      SList = (must_t **)StatList;

      if (SIndex >= 0)
        S = SList[SIndex];
      else
        S = NULL;
 
      if (S != NULL)
        {
        /* finalize profile record */

        if (S->Duration == 0)
          S->Duration = MStat.P.IStatDuration;

        if (MSched.MDB.DBType != mdbNONE)
          {
          enum MStatusCodeEnum SC = mscNoError;

          S->OType = OType;

          if (MSched.WebServicesURL != NULL)
            {
            MOExportToWebServices(mxoxStats,(void *)S,msoCred);
            }

          MDBWriteGeneralStats(&MSched.MDB,S,OID,NULL,&SC);
          }
        }  /* END if (S != NULL) */

      if (SIndex < Depth - 2)
        {
        /* advance pointer */

        SIndex++;

        *SCount = SIndex + 1;
        }
      else
        {
        /* roll statistics */

        MStatFree((void **)&SList[0],msoCred);

        for (sindex = 0;sindex < SIndex;sindex++)
          {
          SList[sindex] = SList[sindex + 1];
          }  /* END for (sindex) */
        }

      MStatCreate((void **)&SList[SIndex],msoCred);

      MStatPInit(
        (void *)SList[SIndex],
        msoCred,
        SIndex,
        MUGetStarttime(MStat.P.IStatDuration,MSched.Time));
      }  /* END case msoCred */

      break;

    case msoNode:

      {
      mnust_t *S;
      mnust_t **SList;
 
      SList = (mnust_t **)StatList;

      if (SIndex >= 0)
        S = SList[SIndex];
      else
        S = NULL;
 
      if (S != NULL)
        {
        /* finalize profile record */

        if (S->Duration == 0)
          S->Duration = MStat.P.IStatDuration;

        if (MSched.MDB.DBType != mdbNONE)
          {
          enum MStatusCodeEnum SC = mscNoError;

          S->OType = OType;

          if (MSched.WebServicesURL != NULL)
            {
            MOExportToWebServices(mxoxStats,(void *)S,msoNode);
            }

          MDBWriteNodeStats(&MSched.MDB,S,OID,NULL,&SC);
          }
        }  /* END if (S != NULL) */

      if (SIndex < Depth - 2)
        {
        /* "- 2" is because we advance SIndex *AND THEN* advance SCount one more beyond that */

        /* advance pointer */

        SIndex++;

        *SCount = SIndex + 1;
        }
      else
        {
        /* roll statistics */

        MStatFree((void **)&SList[0],msoNode);

        for (sindex = 0;sindex < SIndex;sindex++)
          {
          SList[sindex] = SList[sindex + 1];
          }  /* END for (sindex) */
        }

      MStatCreate((void **)&SList[SIndex],msoNode);

      MStatPInit(
        (void *)SList[SIndex],
        msoNode,
        SIndex,
        MUGetStarttime(MStat.P.IStatDuration,MSched.Time));
      }  /* END case msoNode */

      break;

    default:

      /* NO-OP */

      break;
    }  /* END switch (StatType) */

  return(SUCCESS);
  }  /* END MStatPRoll() */





/**
 * Report stat profile as XML string.
 *
 * @see MCredProfileToXML() - parent
 *
 * @param Stat (I) [profile start pointer]
 * @param StatType (I)
 * @param ICount (I) [number of profiles to display]
 * @param Type (I)
 * @param AIndex (I) [only used when printing out indexed stats (like GRes]
 * @param OBuf (O)
 * @param OBufSize (I)
 */

int MStatPToString(

  void                  **Stat,      /* I (profile start pointer) */
  enum MStatObjectEnum    StatType,  /* I */
  int                     ICount,    /* I (number of profiles to display) */
  int                     Type,      /* I */
  int                     AIndex,    /* I (only used when printing out indexed stats (like GRes,GMetric) */
  char                   *OBuf,      /* O */
  int                     OBufSize)  /* I */

  {
  char *BPtr = NULL;
  int   BSpace = 0;

  int   iindex;

  char  tmpLine[MMAX_LINE];
  char  tmpOStat[MMAX_LINE];
  char  tmpNStat[MMAX_LINE];

  mbool_t StatsMatch;

  long  tmpOLMax = 0;
  long  tmpOLMin = 0;

  int   tmpOIMax = 0;
  int   tmpOIMin = 0;

  double tmpODMax = 0.0;
  double tmpODMin = 0.0;

  int   StatCount;
  int   StatIndex;

  const char *FName = "MStatPToString";

  MDB(7,fSCHED) MLog("%s(Stat,%s,%d,%d,%d,OBuf,%d)\n",
    FName,
    MStatObject[StatType],
    ICount,
    Type,
    AIndex,
    OBufSize);

  /* initialize output */

  if (OBuf != NULL)
    OBuf[0] = '\0';

  /* check parameters */

  if ((Stat == NULL) || (ICount <= 0) || (OBuf == NULL))
    {
    return(FAILURE);
    }

  MUSNInit(&BPtr,&BSpace,OBuf,OBufSize);

  switch (StatType)
    {
    case msoCred:
 
      {
      must_t **IStat = (must_t **)Stat;

      enum MStatAttrEnum SType = (enum MStatAttrEnum)Type;

      switch (SType)
        {
        case mstaIStatCount:
        case mstaIStatDuration:

          MStatAToString(
            (void *)IStat[0],
            msoCred,
            (enum MStatAttrEnum)SType,
            0,
            tmpLine,
            mfmNONE);

          MUSNCat(&BPtr,&BSpace,tmpLine);

          break;

        case mstaStartTime:

          MStatAToString(
            (void *)IStat[0],
            msoCred,
            (enum MStatAttrEnum)SType,
            0,
            tmpNStat,
            mfmNONE);

          MUSNPrintF(&BPtr,&BSpace,"%s,",
            tmpNStat);

          if (ICount >= 1)
            {
            MStatAToString(
              (void *)IStat[ICount - 1],
              msoCred,
              (enum MStatAttrEnum)SType,
              0,
              tmpNStat,
              mfmNONE);

            MUSNCat(&BPtr,&BSpace,tmpNStat);
            }

          break;

        default:

          {
          mbool_t IsEnd = FALSE;

          /* initialize tmp variables to values which will not be matched */

          tmpOStat[0] = '\0';

          StatCount = 1;
          StatIndex = 0;

          for (iindex = 0;iindex < ICount;iindex++)
            {
            tmpNStat[0] = '\0'; /* So valgrind won't complain */

            MStatAToString(
              (void *)IStat[iindex],
              msoCred,
              (enum MStatAttrEnum)SType,
              AIndex,
              tmpNStat,  /* O */
              mfmNONE);

            if (tmpNStat[0] == '\0')
              {
              /* MStatAToString() will return 'empty string' when the stat is '0' */

              MUStrCpy(tmpNStat,"0",sizeof(tmpNStat));
              }

            if (iindex == 0)
              {
              /* first value reported */

              if (ICount > 1)
                {
                MUStrCpy(tmpOStat,tmpNStat,sizeof(tmpOStat));

                switch (MStatDataType[iindex])
                  {
                  case mdfLong:   
                     
                    {
                    long tmpL = strtol(tmpNStat,NULL,10);
 
                    tmpOLMax = (long)(MSched.StatMaxThreshold * tmpL);
                    tmpOLMin = (long)(MSched.StatMinThreshold * tmpL);
                    }

                    break;

                  case mdfInt:   

                    {
                    long tmpI = (int)strtol(tmpNStat,NULL,10);

                    tmpOIMax = (long)(MSched.StatMaxThreshold * tmpI);
                    tmpOIMin = (long)(MSched.StatMinThreshold * tmpI);
                    }

                    break;

                  case mdfDouble:

                    {
                    double tmpD = strtod(tmpNStat,NULL);

                    tmpODMax = MSched.StatMaxThreshold * tmpD;
                    tmpODMin = MSched.StatMinThreshold * tmpD;
                    }

                    break;

                  default: 

                    /* NO-OP */

                    break;
                  }  /* END switch (MStatDataType[iindex]) */
                }    /* END if (ICount > 1) */
              else
                {
                /* no further values to report - print value and finish */

                MUSNPrintF(&BPtr,&BSpace,"%s",
                  tmpNStat);
                }

              continue;
              }  /* END if (iindex == 0) */

            switch (MStatDataType[iindex])
              {
              case mdfInt:

                {
                int tmpI = (int)strtol(tmpNStat,NULL,10);

                StatsMatch = ((tmpI <= tmpOIMax) && (tmpI >= tmpOIMin));
                }

                break;

              case mdfLong:

                {
                long tmpL = strtol(tmpNStat,NULL,10);

                StatsMatch = ((tmpL <= tmpOLMax) && (tmpL >= tmpOLMin));
                }

                break;

              case mdfDouble:

                {
                double tmpD = strtod(tmpNStat,NULL);

                StatsMatch = ((tmpD <= tmpODMax) && (tmpD >= tmpODMin));
                }

                break;

              case mdfString:
              case mdfNONE:
              default:

                /* perform string comparision */

                StatsMatch = !strcmp(tmpNStat,tmpOStat);

                break;
              }  /* END switch (MStatDataType[iindex]) */

            if (StatsMatch == TRUE)
              {
              /* stats match */

              StatCount++;

              if (iindex < ICount - 1)
                continue;

              IsEnd = TRUE;
              }

            if (StatIndex > 0)
              MUSNCat(&BPtr,&BSpace,",");

            StatIndex++;

            if (tmpOStat[0] == '\0')
              strcpy(tmpOStat,"0");

            if (StatCount > 1)
              {
              MUSNPrintF(&BPtr,&BSpace,"%s*%d",
                tmpOStat,
                StatCount);
              }
            else
              {
              MUSNCat(&BPtr,&BSpace,tmpOStat);
              }

            if ((iindex + 1 >= ICount) && (IsEnd == FALSE))
              MUSNPrintF(&BPtr,&BSpace,",%s",tmpNStat);
            else
              {
              MUStrCpy(tmpOStat,tmpNStat,sizeof(tmpOStat));
              }

            StatCount = 1;
            }  /* END for (iindex) */
          }    /* END case (default) */

        break;
        }   /* END switch (SType) */
      }     /* END case (msoCred) */

      break;

    case msoNode:

      {
      mnust_t **IStat = (mnust_t **)Stat;

      enum MNodeStatTypeEnum SType = (enum MNodeStatTypeEnum)Type;

      switch (SType)
        {
        case mnstIStatCount:
        case mnstIStatDuration:

          MStatAToString((void *)IStat[0],msoNode,(enum MNodeStatTypeEnum)SType,0,tmpLine,mfmNONE);

          MUSNCat(&BPtr,&BSpace,tmpLine);

          break;

        case mnstStartTime:

          MStatAToString(
            (void *)IStat[0],
            msoNode,
            (enum MNodeStatTypeEnum)SType,
            0,
            tmpNStat,
            mfmNONE);

          MUSNPrintF(&BPtr,&BSpace,"%s,",
            tmpNStat);

          if (ICount >= 1)
            {
            MStatAToString(
              (void *)IStat[ICount - 1],
              msoNode,
              (enum MNodeStatTypeEnum)SType,
              0,
              tmpNStat,
              mfmNONE);

            MUSNCat(&BPtr,&BSpace,tmpNStat);
            }

          break;

        default:

          {
          mbool_t IsEnd = FALSE;

          tmpOStat[0] = '\0';

          StatCount = 1;
          StatIndex = 0;

          for (iindex = 0;iindex < ICount;iindex++)
            {
            MStatAToString(
              (void *)IStat[iindex],
              msoNode,
              (enum MNodeStatTypeEnum)SType,
              AIndex,
              tmpNStat,
              mfmNONE);

            if (tmpNStat[0] == '\0')
              {
              /* MStatAToString will return NULL when the stat is '0' */

              MUStrCpy(tmpNStat,"0",sizeof(tmpNStat));
              }

            if (iindex == 0)
              {
              if (ICount != 1)
                {
                MUStrCpy(tmpOStat,tmpNStat,sizeof(tmpOStat));
                }
              else
                {
                MUSNPrintF(&BPtr,&BSpace,"%s",
                  tmpNStat);
                }

              continue;
              }

            if (!strcmp(tmpNStat,tmpOStat))
              {
              StatCount++;

              if (iindex < ICount - 1)
                continue;

              IsEnd = TRUE;
              }

            if (StatIndex > 0)
              MUSNCat(&BPtr,&BSpace,",");

            StatIndex++;

            if (tmpOStat[0] == '\0')
              strcpy(tmpOStat,"0");

            if (StatCount > 1)
              {
              MUSNPrintF(&BPtr,&BSpace,"%s*%d",
                tmpOStat,
                StatCount);
              }
            else
              {
              MUSNCat(&BPtr,&BSpace,tmpOStat);
              }

            if ((iindex + 1 >= ICount) && (IsEnd == FALSE))
              {
              MUSNPrintF(&BPtr,&BSpace,",%s",
                tmpNStat);
              }
            else
              {
              MUStrCpy(tmpOStat,tmpNStat,sizeof(tmpOStat));
              }

            StatCount = 1;
            }  /* END for (iindex) */
          }    /* END BLOCK (default) */

        break;
        }      /* END switch (SType) */
      }        /* END BLOCK (case msoNode) */

      break;

    default:

      /* NO-OP */

      break;
    }  /* END switch (StatType) */
  
  return(SUCCESS);
  }  /* END MStatPToString() */





/**
 * Report summary (overall avg, max, min) of profile stats.
 *
 * @see MCredProfileToXML() - parent
 *
 * @param Stat (I) [profile start pointer]
 * @param StatType (I)
 * @param ICount (I) [number of profiles to display]
 * @param Type (I)
 * @param AIndex (I) ) [only used with indexed stats (like GRES]
 * @param OBuf (O)
 * @param OBufSize (I)
 */

int MStatPToSummaryString(

  void                  **Stat,      /* I (profile start pointer) */
  enum MStatObjectEnum    StatType,  /* I */
  int                     ICount,    /* I (number of profiles to display) */
  int                     Type,      /* I */
  int                     AIndex,    /* I (only used with indexed stats (like GRES)) */
  char                   *OBuf,      /* O */
  int                     OBufSize)  /* I */

  {
  int   iindex;

  double tmpD = 0.0;
  long   tmpL = 0;

  char  tmpStat[MMAX_LINE];

  const char *FName = "MStatPToSummaryString";

  MDB(7,fSCHED) MLog("%s(Stat,%s,%d,%d,%d,%d)\n",
    FName,
    MStatObject[StatType],
    ICount,
    Type,
    AIndex,
    OBufSize);

  /* initialize output */

  if (OBuf != NULL)
    OBuf[0] = '\0';

  /* check parameters */

  if ((Stat == NULL) || (ICount <= 0) || (OBuf == NULL))
    {
    return(FAILURE);
    }

  switch (StatType)
    {
    case msoCred:
 
      {
      must_t **IStat = (must_t **)Stat;

      enum MStatAttrEnum SType = (enum MStatAttrEnum)Type;

      switch (SType)
        {
        case mstaIStatCount:
        case mstaIStatDuration:

          MStatAToString(
            (void *)IStat[0],
            msoCred,
            (enum MStatAttrEnum)SType,
            0,
            tmpStat,
            mfmNONE);

          snprintf(OBuf,OBufSize,"%s",
            tmpStat);

          break;

        case mstaStartTime:

          /* summarize attributes of type 'string' */
 
          MStatAToString(
            (void *)IStat[0],
            msoCred,
            (enum MStatAttrEnum)SType,
            0,
            tmpStat,
            mfmNONE);

          snprintf(OBuf,OBufSize,"%s",
            tmpStat);

          if (ICount >= 1)
            {
            MStatAToString(
              (void *)IStat[ICount - 1],
              msoCred,
              (enum MStatAttrEnum)SType,
              0,
              tmpStat,
              mfmNONE);

            strcat(OBuf,",");
            strcat(OBuf,tmpStat);
            }

          break;

        case mstaAvgBypass:
        case mstaAvgQueueTime:
        case mstaAvgXFactor:
        case mstaQOSCredits:
        case mstaTDSAvl:
        case mstaTDSDed:
        case mstaTDSUtl:
        case mstaIDSUtl:
        case mstaTReqWTime:
        case mstaTExeWTime:
        case mstaTMSAvl:
        case mstaTMSDed:
        case mstaTMSUtl:
        case mstaTPSReq:
        case mstaTPSExe:
        case mstaTPSDed:
        case mstaTPSUtl:
        case mstaTJobAcc:
        case mstaTNJobAcc:
        case mstaTXF: 
        case mstaTNXF:
        case mstaMXF:
        case mstaTBypass:
        case mstaMBypass:
        case mstaGPHAvl:
        case mstaGPHUtl:
        case mstaGPHDed:
        case mstaGPHSuc:
        case mstaGMinEff:
        case mstaTPHPreempt:
        case mstaTQueuedPH:
        case mstaTStartXF:
        case mstaTSubmitPH:
        case mstaGMetric:
 
          /* summarize attributes of type 'double' */
  
          for (iindex = 0;iindex < ICount;iindex++)
            {
            MStatAToString(
              (void *)IStat[iindex],
              msoCred,
              (enum MStatAttrEnum)SType,
              0,
              tmpStat,
              mfmNONE);

            if (tmpStat[0] == '\0')
              continue;
         
            tmpD += strtod(tmpStat,NULL);
            }

          snprintf(OBuf,OBufSize,"%3f",
            tmpD);

          break;

        case mstaTCapacity:
        case mstaDuration:
        case mstaIterationCount:
        case mstaTNC:
        case mstaTPC:
        case mstaTJobCount:
        case mstaTNJobCount:
        case mstaTQueueTime:
        case mstaMQueueTime:
        case mstaTQOSAchieved:
        case mstaGCEJobs:      /* current eligible jobs */
        case mstaGCIJobs:      /* current idle jobs */
        case mstaGCAJobs:      /* current active jobs */
        case mstaGMinEffIteration:
        case mstaTJPreempt:
        case mstaTJEval:
        case mstaTQueuedJC:
        case mstaSchedDuration:
        case mstaSchedCount:
        case mstaTSubmitJC:
        case mstaTStartJC:
        case mstaTStartPC:
        case mstaTStartQT:

          /* summarize attributes of type 'int' or 'long' */

          for (iindex = 0;iindex < ICount;iindex++)
            {
            MStatAToString(
              (void *)IStat[iindex],
              msoCred,
              (enum MStatAttrEnum)SType,
              0,
              tmpStat,
              mfmNONE);

            if (tmpStat[0] == '\0')
              continue;
         
            tmpL += strtol(tmpStat,NULL,10);
            }

          snprintf(OBuf,OBufSize,"%ld",
            tmpL);

          break;

        default:

          /* not handled */

          /* NO-OP */

          break;
        }   /* END switch (SType) */
      }     /* END case (msoCred) */

      break;

    case msoNode:

      {
      mnust_t **IStat = (mnust_t **)Stat;

      enum MNodeStatTypeEnum SType = (enum MNodeStatTypeEnum)Type;

      switch (SType)
        {
        case mnstIStatCount:
        case mnstIStatDuration:

          MStatAToString(
            (void *)IStat[0],
            msoNode,
            (enum MNodeStatTypeEnum)SType,
            0,
            tmpStat,
            mfmNONE);

          snprintf(OBuf,OBufSize,"%s",
            tmpStat);

          break;

        case mnstStartTime:

          MStatAToString(
            (void *)IStat[0],
            msoNode,
            (enum MNodeStatTypeEnum)SType,
            0,
            tmpStat,
            mfmNONE);

          snprintf(OBuf,OBufSize,"%s",
            tmpStat);

          if (ICount >= 1)
            {
            MStatAToString(
              (void *)IStat[ICount - 1],
              msoNode,
              (enum MNodeStatTypeEnum)SType,
              0,
              tmpStat,
              mfmNONE);

            strcat(OBuf,",");
            strcat(OBuf,tmpStat);
            }

          break;

        case mnstAGRes:
        case mnstCGRes:
        case mnstCPULoad:
        case mnstDGRes:
        case mnstGMetric:
        case mnstUMem:
        case mnstQueuetime:
        case mnstXLoad:
        case mnstXFactor:

          /* handle range totals (double) */

          for (iindex = 0;iindex < ICount;iindex++)
            {
            MStatAToString(
              (void *)IStat[iindex],
              msoNode,
              (enum MNodeStatTypeEnum)SType,
              AIndex,
              tmpStat,
              mfmNONE);

            if (tmpStat[0] == '\0')
              continue;
         
            tmpD += strtod(tmpStat,NULL);
            }

          if (tmpD != MDEF_GMETRIC_VALUE)
            {
            snprintf(OBuf,OBufSize,"%3f",tmpD);
            }

          break;

        case mnstAProc:
        case mnstCProc:
        case mnstDProc:
        case mnstIterationCount:

          /* handle range totals (long) */

          for (iindex = 0;iindex < ICount;iindex++)
            {
            MStatAToString(
              (void *)IStat[iindex],
              msoNode,
              (enum MNodeStatTypeEnum)SType,
              AIndex,
              tmpStat,
              mfmNONE);

            if (tmpStat[0] == '\0')
              continue;

            tmpL += strtol(tmpStat,NULL,10);
            }  /* END for (iindex) */

          sprintf(OBuf,"%ld",
            tmpL);

          break;

        case mnstMinCPULoad:
        case mnstMinMem:

          /* handle range minimums */

          {
          double tmpD1;
          double tmpD2 = -1.0;

          for (iindex = 0;iindex < ICount;iindex++)
            {
            MStatAToString(
              (void *)IStat[iindex],
              msoNode,
              (enum MNodeStatTypeEnum)SType,
              0,
              tmpStat,
              mfmNONE);

            if (tmpStat[0] == '\0')
              continue; 

            tmpD1 = strtod(tmpStat,NULL);

            if ((tmpD2 == -1) || (tmpD1 < tmpD2))
              tmpD2 = tmpD1;
            }

          snprintf(OBuf,OBufSize,"%3f",
            tmpD2);
          }

          break;

        case mnstMaxCPULoad:
        case mnstMaxMem:

          /* handle range minimums */

          {
          double tmpD1;
          double tmpD2 = -1.0;

          for (iindex = 0;iindex < ICount;iindex++)
            {
            MStatAToString(
              (void *)IStat[iindex],
              msoNode,
              (enum MNodeStatTypeEnum)SType,
              0,
              tmpStat,
              mfmNONE);

            if (tmpStat[0] == '\0')
              continue;

            tmpD1 = strtod(tmpStat,NULL);

            if ((tmpD2 == -1) || (tmpD1 > tmpD2))
              tmpD2 = tmpD1;
            }

          snprintf(OBuf,OBufSize,"%3f",
            tmpD2);
          }  /* END BLOCK (vcase mnstMaxCPULoad) */

          break;

        case mnstCat:
        case mnstRCat:
        case mnstNodeState:

          /* handle range summaries */

          MStatPToString(Stat,StatType,ICount,Type,0,OBuf,OBufSize);

          break;

        default:
   
          /* string type NYI */
   
          break;
   
        }  /* END switch (AIndex) */
      }    /* END BLOCK (case msoNode) */

      break;

    default:

      /* NO-OP */

      break;
    }  /* END switch (StatType) */
  
  return(SUCCESS);
  }  /* END MStatPToSummaryString() */



/**
 * Copy stat structure (must_t/mnust_t ) from SrcS to DstS. 
 *
 * @param Stat     (I)
 * @param StatType (I)
 */

int MStatPClear(

  void                  *Stat,
  enum MStatObjectEnum   StatType)

  {
  if (Stat == NULL)
    {
    return(FAILURE);
    }

  switch (StatType)
    {
    case msoCred:

      {
      must_t *S = (must_t *)Stat;

      S->Index = 0;
     
      S->OType = mxoNONE;

      S->StartTime = 0;
      S->Duration = 0;
      S->IterationCount = 0;

     
      S->JC = 0;
      S->SubmitJC = 0;
      S->ISubmitJC = 0;
      S->SuccessJC = 0;
      S->RejectJC = 0;
      S->QOSSatJC = 0;
      S->JAllocNC = 0;
      S->ActiveNC = 0;
      S->ActivePC = 0;

      S->TotalQTS = 0;
      S->MaxQTS = 0;
 
     
      S->TQueuedJC = 0;

      S->AvgXFactor = 0;
      S->AvgQTime = 0;
      S->IICount = 0;

      S->TPC = 0;
      S->TNC = 0;

      S->PSRequest = 0;
      S->SubmitPHRequest = 0;
      S->PSRun = 0;
      S->PSRunSuccessful = 0;

      S->TQueuedPH = 0;
      S->TotalRequestTime = 0;
      S->TotalRunTime = 0;
     
      S->PSDedicated = 0;
      S->PSUtilized = 0;
     
      S->DSAvail = 0;
      S->DSUtilized = 0;
      S->IDSUtilized = 0;

      S->MSAvail = 0;
      S->MSUtilized = 0;
      S->MSDedicated = 0;

      S->JobAcc = 0;
      S->NJobAcc = 0;

      S->XFactor = 0;
      S->NXFactor = 0;
      S->PSXFactor = 0;
      S->MaxXFactor = 0;

      S->Bypass = 0;
      S->MaxBypass = 0;

      S->BFCount = 0;
      S->BFPSRun = 0;

      S->QOSCredits = 0;
      
      memset(S->Accuracy,0,sizeof(int) * MMAX_ACCURACY);

      S->StartJC = 0;
      S->StartPC = 0;
      S->StartXF = 0;
      S->StartQT = 0;

      S->TCapacity = 0;

      S->TNL = 0;
      S->TNM = 0;

      S->IsProfile = 0;
 
      S->IStatCount = 0;

      S->Verified = 0;
   
      memset(S->XLoad,0,sizeof(double) * MSched.M[mxoxGMetric]);
      }

      break;

    case msoNode:

      {
      mnust_t *S = (mnust_t *)Stat;

      S->Index = 0;
      S->OType = mxoNONE;
      S->StartTime = 0;
      S->Duration = 0;
      S->IterationCount = 0;
      S->SCat = mncNONE;
      S->RCat = mncNONE;
      S->Cat = mncNONE;
      S->AProc = 0;
      S->CProc = 0;
      S->DProc = 0;
      S->NState = mnsNONE;
      S->CPULoad = 0;
      S->MaxCPULoad = 0;
      S->MinCPULoad = 0;
      S->UMem = 0;
      S->MaxMem = 0;
      S->MinMem = 0;
      S->StartJC = 0;
      S->FailureJC = 0;
      S->Queuetime = 0;
      S->XFactor = 0;
      
      MSNLClear(&S->CGRes);
      MSNLClear(&S->AGRes);
      MSNLClear(&S->DGRes);

      memset(S->XLoad,0,sizeof(double) * MSched.M[mxoxGMetric]);

      S->IsProfile = 0;
      S->IStatCount = 0;
      
      S->FCurrentTime = 0;
      
      memset(S->FCount,0,sizeof(int) * MMAX_BCAT);
      memset(S->OFCount,0,sizeof(int) * MMAX_BCAT);
     
      S->SuccessiveJobFailures = 0;
      }

      break;

    default:

      /* NO-OP */

      break;
    }  /* END switch (StatType) */

  return(SUCCESS);
  }  /* END MStatPCopy() */




/**
 * Copy stat structure (must_t/mnust_t ) from SrcS to DstS. 
 *
 * @param DstS (O)
 * @param SrcS (I)
 * @param StatType (I)
 */

int MStatPCopy2(

  void                  *DstS,      /* O */
  void                  *SrcS,      /* I */
  enum MStatObjectEnum   StatType)  /* I */

  {
  if ((DstS == NULL) || (SrcS == NULL))
    {
    return(FAILURE);
    }

  switch (StatType)
    {
    case msoCred:

      {
      /* some structures cannot be memcpy'd so we must manually
         copy over the various attributes */

      must_t *Dst = (must_t *)DstS;
      must_t *Src = (must_t *)SrcS;

      Dst->Index = Src->Index;
     
      Dst->OType = Src->OType;

      Dst->StartTime = Src->StartTime;
      Dst->Duration = Src->Duration;
      Dst->IterationCount = Src->IterationCount;

     
      Dst->JC = Src->JC;
      Dst->SubmitJC = Src->SubmitJC;
      Dst->ISubmitJC = Src->ISubmitJC;
      Dst->SuccessJC = Src->SuccessJC;
      Dst->RejectJC = Src->RejectJC;
      Dst->QOSSatJC = Src->QOSSatJC;
      Dst->JAllocNC = Src->JAllocNC;
      Dst->ActiveNC = Src->ActiveNC;
      Dst->ActivePC = Src->ActivePC;

      Dst->TotalQTS = Src->TotalQTS;
      Dst->MaxQTS = Src->MaxQTS;
 
     
      Dst->TQueuedJC = Src->TQueuedJC;

      Dst->AvgXFactor = Src->AvgXFactor;
      Dst->AvgQTime = Src->AvgQTime;
      Dst->IICount = Src->IICount;

      Dst->TPC = Src->TPC; 
      Dst->TNC = Src->TNC;

      Dst->PSRequest = Src->PSRequest;
      Dst->SubmitPHRequest = Src->SubmitPHRequest;
      Dst->PSRun = Src->PSRun;
      Dst->PSRunSuccessful = Src->PSRunSuccessful;

      Dst->TQueuedPH = Src->TQueuedPH;
      Dst->TotalRequestTime = Src->TotalRequestTime;
      Dst->TotalRunTime = Src->TotalRunTime;
     
      Dst->PSDedicated = Src->PSDedicated;
      Dst->PSUtilized = Src->PSUtilized;
     
      Dst->DSAvail = Src->DSAvail;
      Dst->DSUtilized = Src->DSUtilized;
      Dst->IDSUtilized = Src->IDSUtilized;

      Dst->MSAvail = Src->MSAvail;
      Dst->MSUtilized = Src->MSUtilized;
      Dst->MSDedicated = Src->MSDedicated;

      Dst->JobAcc = Src->JobAcc;
      Dst->NJobAcc = Src->NJobAcc;

      Dst->XFactor = Src->XFactor;
      Dst->NXFactor = Src->NXFactor;
      Dst->PSXFactor = Src->PSXFactor;
      Dst->MaxXFactor = Src->MaxXFactor;

      Dst->Bypass = Src->Bypass;
      Dst->MaxBypass = Src->MaxBypass;

      Dst->BFCount = Src->BFCount;
      Dst->BFPSRun = Src->BFPSRun;

      Dst->QOSCredits = Src->QOSCredits;
      
      memcpy(Dst->Accuracy,Src->Accuracy,sizeof(int) * MMAX_ACCURACY);

      Dst->StartJC = Src->StartJC;
      Dst->StartPC = Src->StartPC;
      Dst->StartXF = Src->StartXF;
      Dst->StartQT = Src->StartQT;

      Dst->TCapacity = Src->TCapacity;

      Dst->TNL = Src->TNL;
      Dst->TNM = Src->TNM;

      Dst->IsProfile = Src->IsProfile;
 
      Dst->IStatCount = Src->IStatCount;

      Dst->Verified = Src->Verified;
   
#if 0
      mln_t *ApplStat;
#endif

      memcpy(Dst->XLoad,Src->XLoad,sizeof(double) * MSched.M[mxoxGMetric]);
      }

      break;

    case msoNode:

      {
      mnust_t *Dst = (mnust_t *)DstS;
      mnust_t *Src = (mnust_t *)SrcS;

      Dst->Index = Src->Index;
      Dst->OType = Src->OType;
      Dst->StartTime = Src->StartTime;
      Dst->Duration = Src->Duration;
      Dst->IterationCount = Src->IterationCount;
      Dst->SCat = Src->SCat;
      Dst->RCat = Src->RCat;
      Dst->Cat = Src->Cat;
      Dst->AProc = Src->AProc;
      Dst->CProc = Src->CProc;
      Dst->DProc = Src->DProc;
      Dst->NState = Src->NState;
      Dst->CPULoad = Src->CPULoad;
      Dst->MaxCPULoad = Src->MaxCPULoad;
      Dst->MinCPULoad = Src->MinCPULoad;
      Dst->UMem = Src->UMem;
      Dst->MaxMem = Src->MaxMem;
      Dst->MinMem = Src->MinMem;
      Dst->StartJC = Src->StartJC;
      Dst->FailureJC = Src->FailureJC;
      Dst->Queuetime = Src->Queuetime;
      Dst->XFactor = Src->XFactor;
      
      MSNLCopy(&Dst->CGRes,&Src->CGRes);
      MSNLCopy(&Dst->AGRes,&Src->AGRes);
      MSNLCopy(&Dst->DGRes,&Src->DGRes);

      memcpy(Dst->XLoad,Src->XLoad,sizeof(double) * MSched.M[mxoxGMetric]);

      Dst->IsProfile = Src->IsProfile;
      Dst->IStatCount = Src->IStatCount;
      
      Dst->FCurrentTime = Src->FCurrentTime;
      
      memcpy(Dst->FCount,Src->FCount,sizeof(int) * MMAX_BCAT);
      memcpy(Dst->OFCount,Src->OFCount,sizeof(int) * MMAX_BCAT);
     
#if 0
      struct mnust_t **IStat;
      mln_t *ApplStat;
#endif
      
      Dst->SuccessiveJobFailures = Src->SuccessiveJobFailures;
      }

      break;

    default:

      /* NO-OP */

      break;
    }  /* END switch (StatType) */

  return(SUCCESS);
  }  /* END MStatPCopy2() */








/**
 * Copy stat structure (must_t/mnust_t ) from SrcS to DstS. 
 *
 * NOTE: also creates DstS
 *
 * @param DstS (O)
 * @param SrcS (I)
 * @param StatType (I)
 */

int MStatPCopy(

  void                 **DstS,      /* O */
  void                  *SrcS,      /* I */
  enum MStatObjectEnum   StatType)  /* I */

  {
  if ((DstS == NULL) || (SrcS == NULL))
    {
    return(FAILURE);
    }

  switch (StatType)
    {
    case msoCred:

      {
      /* some structures cannot be memcpy'd so we must manually
         copy over the various attributes */

      must_t *tS;
      must_t *Src = (must_t *)SrcS;

      MStatCreate((void **)&tS,msoCred);

      tS->Index = Src->Index;
     
      tS->OType = Src->OType;

      tS->StartTime = Src->StartTime;
      tS->Duration = Src->Duration;
      tS->IterationCount = Src->IterationCount;

     
      tS->JC = Src->JC;
      tS->SubmitJC = Src->SubmitJC;
      tS->ISubmitJC = Src->ISubmitJC;
      tS->SuccessJC = Src->SuccessJC;
      tS->RejectJC = Src->RejectJC;
      tS->QOSSatJC = Src->QOSSatJC;
      tS->JAllocNC = Src->JAllocNC;
      tS->ActiveNC = Src->ActiveNC;
      tS->ActivePC = Src->ActivePC;

      tS->TotalQTS = Src->TotalQTS;
      tS->MaxQTS = Src->MaxQTS;
 
     
      tS->TQueuedJC = Src->TQueuedJC;

      tS->AvgXFactor = Src->AvgXFactor;
      tS->AvgQTime = Src->AvgQTime;
      tS->IICount = Src->IICount;

      tS->TPC = Src->TPC; 
      tS->TNC = Src->TNC;

      tS->PSRequest = Src->PSRequest;
      tS->SubmitPHRequest = Src->SubmitPHRequest;
      tS->PSRun = Src->PSRun;
      tS->PSRunSuccessful = Src->PSRunSuccessful;

      tS->TQueuedPH = Src->TQueuedPH;
      tS->TotalRequestTime = Src->TotalRequestTime;
      tS->TotalRunTime = Src->TotalRunTime;
     
      tS->PSDedicated = Src->PSDedicated;
      tS->PSUtilized = Src->PSUtilized;
     
      tS->DSAvail = Src->DSAvail;
      tS->DSUtilized = Src->DSUtilized;
      tS->IDSUtilized = Src->IDSUtilized;

      tS->MSAvail = Src->MSAvail;
      tS->MSUtilized = Src->MSUtilized;
      tS->MSDedicated = Src->MSDedicated;

      tS->JobAcc = Src->JobAcc;
      tS->NJobAcc = Src->NJobAcc;

      tS->XFactor = Src->XFactor;
      tS->NXFactor = Src->NXFactor;
      tS->PSXFactor = Src->PSXFactor;
      tS->MaxXFactor = Src->MaxXFactor;

      tS->Bypass = Src->Bypass;
      tS->MaxBypass = Src->MaxBypass;

      tS->BFCount = Src->BFCount;
      tS->BFPSRun = Src->BFPSRun;

      tS->QOSCredits = Src->QOSCredits;
      
      memcpy(tS->Accuracy,Src->Accuracy,sizeof(int) * MMAX_ACCURACY);

      tS->StartJC = Src->StartJC;
      tS->StartPC = Src->StartPC;
      tS->StartXF = Src->StartXF;
      tS->StartQT = Src->StartQT;

      tS->TCapacity = Src->TCapacity;

      tS->TNL = Src->TNL;
      tS->TNM = Src->TNM;

      tS->IsProfile = Src->IsProfile;
 
      tS->IStatCount = Src->IStatCount;

      tS->Verified = Src->Verified;
   
      memcpy(tS->XLoad,Src->XLoad,sizeof(double) * MSched.M[mxoxGMetric]);

      *DstS = tS;
      }

      break;

    case msoNode:

      {
      mnust_t *tS;
      mnust_t *Src = (mnust_t *)SrcS;

      MStatCreate((void **)&tS,msoNode);

      tS->Index = Src->Index;
      tS->OType = Src->OType;
      tS->StartTime = Src->StartTime;
      tS->Duration = Src->Duration;
      tS->IterationCount = Src->IterationCount;
      tS->SCat = Src->SCat;
      tS->RCat = Src->RCat;
      tS->Cat = Src->Cat;
      tS->AProc = Src->AProc;
      tS->CProc = Src->CProc;
      tS->DProc = Src->DProc;
      tS->NState = Src->NState;
      tS->CPULoad = Src->CPULoad;
      tS->MaxCPULoad = Src->MaxCPULoad;
      tS->MinCPULoad = Src->MinCPULoad;
      tS->UMem = Src->UMem;
      tS->MaxMem = Src->MaxMem;
      tS->MinMem = Src->MinMem;
      tS->StartJC = Src->StartJC;
      tS->FailureJC = Src->FailureJC;
      tS->Queuetime = Src->Queuetime;
      tS->XFactor = Src->XFactor;
      
      MSNLCopy(&tS->CGRes,&Src->CGRes);
      MSNLCopy(&tS->AGRes,&Src->AGRes);
      MSNLCopy(&tS->DGRes,&Src->DGRes);

      memcpy(tS->XLoad,Src->XLoad,sizeof(double) * MSched.M[mxoxGMetric]);

      tS->IsProfile = Src->IsProfile;
      tS->IStatCount = Src->IStatCount;
      
      tS->FCurrentTime = Src->FCurrentTime;
      
      memcpy(tS->FCount,Src->FCount,sizeof(int) * MMAX_BCAT);
      memcpy(tS->OFCount,Src->OFCount,sizeof(int) * MMAX_BCAT);
     
#if 0
      struct mnust_t **IStat;
      mln_t *ApplStat;
#endif
      
      tS->SuccessiveJobFailures = Src->SuccessiveJobFailures;

      *DstS = tS;
      }

      break;

    default:

      /* NO-OP */

      break;
    }  /* END switch (StatType) */

  return(SUCCESS);
  }  /* END MStatPCopy() */




/**
 *
 *
 * @param Line (I/O)
 * @param Length (I)
 */

int MStatPExpand(

  char *Line,    /* I/O */
  int   Length)  /* I */

  {
  char *ptr;
  char *TokPtr = NULL;

  char *ptr2;
  char *TokPtr2 = NULL;

  char scratch[MMAX_BUFFER];
  char tmpBuf[MMAX_BUFFER];

  char *BPtr = NULL;
  int   BSpace = 0;

  int  count;
  int  index;

  if (Line == NULL)
    {
    return(FAILURE);
    }

  MUSNInit(&BPtr,&BSpace,tmpBuf,sizeof(tmpBuf));

  MUStrCpy(scratch,Line,sizeof(scratch));

  ptr = MUStrTok(scratch,",",&TokPtr);

  index = 0;

  while (ptr != NULL)
    {
    ptr2 = MUStrTok(ptr,"*",&TokPtr2);

    count = strtol(TokPtr2,NULL,10);

    count = MAX(count,1);

    while (count > 0)
      {
      if (index > 0)
        {
        MUSNPrintF(&BPtr,&BSpace,",%s",
          ptr2);
        }
      else
        {
        MUSNCat(&BPtr,&BSpace,ptr2);

        index++;
        }

      count--;
      }

    ptr = MUStrTok(NULL,",",&TokPtr);
    }

  if ((int)strlen(tmpBuf) > Length)
    {
    return(FAILURE);
    }

  MUStrCpy(Line,tmpBuf,Length);

  return(SUCCESS);
  }  /* END MStatPExpand() */




/**
 *
 *
 * @param Line (I/O) [modified]
 * @param Length (I)
 * @param Duration (I)
 */

int MStatPStarttimeExpand(

   char      *Line,     /* I/O (modified) */
   int        Length,   /* I */
   int        Duration) /* I */

  {
  /* FORMAT: starttime,endtime */

  mulong STime;
  mulong ETime;

  mulong tmpTime;

  char *ptr;
  char *TokPtr = NULL;

  char *BPtr = NULL;
  int   BSpace = 0;

  ptr = MUStrTok(Line,",",&TokPtr);

  if (ptr == NULL)
    {
    return(FAILURE);
    }

  STime = strtol(ptr,NULL,10);

  ptr = MUStrTok(NULL," \t\n",&TokPtr);

  if (ptr == NULL)
    {
    return(FAILURE);
    }

  ETime = strtol(ptr,NULL,10);

  MUSNInit(&BPtr,&BSpace,Line,Length);

  for (tmpTime = STime;tmpTime <= ETime;tmpTime += Duration)
    {
    if (tmpTime != STime)
      {
      MUSNPrintF(&BPtr,&BSpace,",%lu",tmpTime);
      }
    else
      {
      MUSNPrintF(&BPtr,&BSpace,"%lu",tmpTime);
      }
    }

  return(SUCCESS);
  }  /* END MStatPStarttimeExpand() */

/* END MStatProfile.c */
