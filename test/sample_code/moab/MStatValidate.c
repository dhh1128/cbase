/* HEADER */

      
/**
 * @file MStatValidate.c
 *
 * Contains: MStatValidate
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 * Build a complete, hole-free, profile array filled with valid data
 *   (where possible) and 0's where no data exists.
 *
 * NOTE:  on SUCCESS, all children must be allocated 
 * NOTE:  requires SpecS->IStat
 *
 * @param O (I)
 * @param OIndex (I)
 * @param OName (I)
 * @param SpecS (I)
 * @param NewS (O) [minsize=SpecPCount,allocated children]
 * @param STime (I)
 * @param ETime (I)
 * @param Time (I) [optional]
 * @param DiscoveryOnly (I)
 * @param SpecPCount (I)
 * @param MStatAttrsBM (I) [optional]
 */

int MStatValidatePData(
 
  void                  *O,             /* I */
  enum MXMLOTypeEnum     OIndex,        /* I */
  char                  *OName,         /* I */
  must_t                *SpecS,         /* I */
  must_t               **NewS,          /* O (minsize=SpecPCount,allocated children) */
  long                   STime,         /* I */
  long                   ETime,         /* I */
  long                   Time,          /* I (optional) */
  mbool_t                DiscoveryOnly, /* I */
  int                    SpecPCount,    /* I */
  mbitmap_t             *MStatAttrsBM)  /* I (optional, bitmap of statistic attributes) */

  {
  /* FILE FORMAT: <Data><OBJECTTYPE OBJECTID="X"><Profile>... */

  must_t Stat;

  must_t *S = NULL;

  int  PCount;
  int  PDuration;

  int  IStatCountTotal = 0;

  char StatFile[MMAX_LINE];

  long MidNight;

  long tmpTime;
  long CurrentTime;
  
  int NewSIndex;
  int sindex;
  int saindex;

  const char *FName = "MStatValidatePData";

  MDB(7,fSCHED) MLog("%s(O,%s,%s,SpecS,NewS,%ld,%ld,%ld,%s,%d,MStatAttrsBM)\n",
    FName,
    MXO[OIndex],
    (OName != NULL) ? OName : "",
    STime,
    ETime,
    Time,
    MBool[DiscoveryOnly],
    SpecPCount);

  if (DiscoveryOnly == FALSE)
    {
    if ((SpecS == NULL) || (NewS == NULL))
      {
      MDB(2,fSCHED) MLog("ALERT:    invalid parameters in %s\n",
        FName);

      return(FAILURE);
      }

    if (SpecS->IStat == NULL)
      {
      return(SUCCESS);
      }

    S = SpecS;

    Stat.IStat = NULL;
    }  /* END if (DiscoveryOnly == FALSE) */

  if (Time < 0)
    {
    CurrentTime = MSched.Time;
    }
  else
    {
    CurrentTime = Time;
    }

   MUGetPeriodRange(CurrentTime,0,0,mpDay,&MidNight,NULL,NULL);

  /* if STime is not specified start at midnight of current day */

  if (STime <= 0)
    {
    tmpTime = MidNight;
    }
  else
    {
    tmpTime = STime;
    }

  PCount    = MStat.P.MaxIStatCount;
  PDuration = MStat.P.IStatDuration;

  sindex = 0;

  /* if ETime is not specified end now */

  if (ETime <= 0)
    {
    PCount = (CurrentTime - tmpTime) / PDuration;
    PCount += (MCONST_DAYLEN / PDuration);
    }
  else
    {
    PCount = (ETime - tmpTime) / PDuration;
    PCount += (MCONST_DAYLEN / PDuration);
    }

  /* determine whether we need to get data from files */

  if (((tmpTime < MidNight) &&
       (DiscoveryOnly == TRUE)) ||
      (((SpecS != NULL) && 
        (tmpTime < SpecS->IStat[sindex]->StartTime)) ||
       (tmpTime < MidNight)))
    {
    /* requested starttime is before real data and is not from today */

    long Day = tmpTime;

    char Suffix[MMAX_LINE];
    char tmpVal[MMAX_BUFFER];
    char tmpLine[MMAX_BUFFER];

    enum MStatAttrEnum saindex;

    int  CTok;
    int  aindex;
    int  tmpI;

    char *FileBuf;
    char *ptr;
    char *TokPtr;

    mxml_t *CE;
    mxml_t *PE;
   
    mxml_t *DDE = NULL;

    /* requested data is older than existing (memory) data and older than today's data */

    if (DiscoveryOnly == FALSE)
      {
      /* create temporary stat structure */

      Stat.IStat = (must_t **)MUCalloc(1,sizeof(must_t *) * PCount);

      S = &Stat;

      S->IStatCount   = 0;
      IStatCountTotal = 0;
      }

    while (Day < MidNight)
      {
      /* get data that is older than today from files */

      FileBuf = NULL;

      MStatGetFName(Day,Suffix,sizeof(Suffix));

      sprintf(StatFile,"%s%s.%s",
        MStat.StatDir,
        MCalPeriodType[mpDay],
        Suffix);

      if (MFUGetInfo(StatFile,NULL,NULL,NULL,NULL) != SUCCESS)
        {
        Day += MCONST_DAYLEN;

        continue;
        }

      /* load in from file */

      FileBuf = MFULoadNoCache(StatFile,1,NULL,NULL,NULL,NULL);

      if ((FileBuf == NULL) ||
          (MXMLFromString(&DDE,FileBuf,NULL,NULL) == FAILURE))
        {
        Day += MCONST_DAYLEN;

        MUFree(&FileBuf);

        continue;          
        }

      if (DiscoveryOnly == TRUE)
        {
        MCredDoDiscovery(DDE,OIndex);

        MUFree(&FileBuf);
        MXMLDestroyE(&DDE);

        Day += MCONST_DAYLEN;

        continue;
        }

      CTok = -1;

      while (MXMLGetChild(DDE,(char *)MXO[OIndex],&CTok,&CE))
        {
        if ((CE->ACount <= 0) ||
            (CE->AVal[0] == NULL) ||
            ((OName != NULL) && (strcmp(CE->AVal[0],OName))))
          continue;

        break;
        }  /* END while (MXMLGetChild(DDE,(char *)MXO[OIndex],&CTok,&CE)) */

      if (CE == NULL)
        {
        Day += MCONST_DAYLEN;

        MUFree(&FileBuf);
        MXMLDestroyE(&DDE);

        continue;
        }

      if (MXMLGetChild(CE,(char *)MStatAttr[mstaProfile],NULL,&PE) == FAILURE)
        {
        Day += MCONST_DAYLEN;

        MUFree(&FileBuf);
        MXMLDestroyE(&DDE);

        continue;
        }

      tmpI = 0;

      for (aindex = 0;aindex < PE->ACount;aindex++)
        {
        if (!strcmp(PE->AName[aindex],"SpecDuration"))
          {
          tmpI = strtol((char *)PE->AVal[aindex],NULL,10);

          break;
          }
        }

      if ((tmpI == 0) || (tmpI != PDuration))
        {
        Day += MCONST_DAYLEN;

        MUFree(&FileBuf);
        MXMLDestroyE(&DDE);

        continue;
        }

      for (aindex = 0;aindex < PE->ACount;aindex++)
        {
        saindex = (enum MStatAttrEnum)MUGetIndex(PE->AName[aindex],MStatAttr,FALSE,0);

        if (saindex == mstaNONE)
          continue;

        if ((MStatAttrsBM != NULL) && (!bmisset(MStatAttrsBM,saindex)) && (saindex != mstaStartTime))
          {
          /* attribute not included in specified attribute list (or is Start Time, which must be included)*/
          continue;
          }
                
        MUStrCpy(tmpVal,PE->AVal[aindex],sizeof(tmpVal));

        MUStrCpy(tmpLine,tmpVal,sizeof(tmpLine));

        if ((saindex != mstaStartTime) && (strchr(tmpLine,'*') != NULL))
          {
          MStatPExpand(tmpLine,sizeof(tmpLine));
          MStatPExpand(tmpVal,sizeof(tmpVal));
          }
        else if (saindex == mstaStartTime)
          {
          MStatPStarttimeExpand(tmpLine,sizeof(tmpLine),MStat.P.IStatDuration);
          MStatPStarttimeExpand(tmpVal,sizeof(tmpVal),MStat.P.IStatDuration);
          }

        ptr = MUStrTok(tmpVal,",",&TokPtr);

        S->IStatCount = IStatCountTotal;

        while ((ptr != NULL) && (S->IStatCount < PCount))
          {
          if (S->IStat[S->IStatCount] == NULL)
            {
            MStatCreate((void **)&S->IStat[S->IStatCount],msoCred);

            MStatPInit(S->IStat[S->IStatCount],msoCred,S->IStatCount,MStat.P.IStatEnd);
            }

          if (saindex == mstaStartTime)
            {
            S->IStat[S->IStatCount]->StartTime = strtol(ptr,NULL,10);
            }
          else if (saindex == mstaIStatCount)
            {
            /* if restoring checkpoint, use configured value, not checkpointed value */

            /* NO-OP */
            }
          else 
            {
            MStatSetAttr(
              (void *)S->IStat[S->IStatCount],
              msoCred,
              (int)saindex,
              (void **)ptr,
              0,
              mdfString,
              mSet);
            }

          S->IStatCount++;

          ptr = MUStrTok(NULL,",",&TokPtr);
          } /* END while ((ptr != NULL) && (S->IStatCount < PCount)) */
        }   /* END for (aindex) */

      IStatCountTotal = S->IStatCount;

      Day += MCONST_DAYLEN;

      MUFree(&FileBuf);
      MXMLDestroyE(&DDE);
      }   /* END while (Day < (ETime <= 0) ? CurrentTime : ETime) */

    /* check if data from today is required */

    if ((ETime > MidNight) && (DiscoveryOnly == FALSE))
      {
      while ((SpecS->IStat[sindex] != NULL) && 
             (SpecS->IStat[sindex]->StartTime < MidNight))
        {
        /* this data was already gathered from the files...can be ignored */

        sindex++;
        }

      while ((S->IStatCount < PCount) &&
             (sindex < SpecS->IStatCount) &&
             (SpecS->IStat[sindex] != NULL) && 
             (SpecS->IStat[sindex]->StartTime <= ETime))
        {
        /* data from today is required, load everything we have into S */

        MStatPCopy((void **)&S->IStat[S->IStatCount],SpecS->IStat[sindex],msoCred);

        sindex++;
        S->IStatCount++;
        }
      }   /* END if (ETime > MidNight) */
    }     /* END if ((tmpTime > (S->IStat[sindex]->StartTime) ...*/

  if (DiscoveryOnly == TRUE)
    {
    return(SUCCESS);
    }

  /* this section builds a complete, hole-free, structure filled with good data 
     (where possible) and 0's where no data exists */

  sindex  = 0;
  saindex = 0;

  NewSIndex = 0;

  while ((NewSIndex < SpecPCount) && (S != NULL))
    {
    if ((saindex >= PCount) ||
        (sindex >= S->IStatCount) ||
        (S->IStat[sindex] == NULL) ||
        (S->IStat[sindex]->StartTime > tmpTime))
      {
      /* no existing data for this time slot */

      MStatCreate((void **)&NewS[NewSIndex],msoCred);

      MStatPInit((void *)NewS[NewSIndex],msoCred,NewSIndex,tmpTime);
      }  /* END if ((S->IStat[sindex] == NULL) || ...) */
    else if (S->IStat[sindex]->StartTime < tmpTime)
      {
      /* existing data is earlier than needed */

      sindex++;

      continue;
      }
    else
      {
      /* copy existing data */

      MStatPCopy((void **)&NewS[NewSIndex],S->IStat[sindex],msoCred);

      sindex++;
      }

    saindex++;

    NewSIndex++;

    tmpTime += PDuration;
    }  /* END while (S->IStat[sindex] != NULL) */

  MStatPDestroy((void ***)&Stat.IStat,msoCred,SpecPCount);

  return(SUCCESS);
  }  /* END MStatValidatePData() */




/**
 * Using SpecS as source statistics, populate NewS with validated 
   and fully populated statistics.
 *
 * @param N (I)
 * @param SpecS (I)
 * @param NewS (O)
 * @param STime (I)
 * @param ETime (I)
 * @param Time (I) [optional]
 * @param SpecPCount (I)
 */

int MStatValidateNPData(

  mnode_t                *N,          /* I */
  mnust_t                *SpecS,      /* I */
  mnust_t               **NewS,       /* O (minsize=SpecPCount) */
  long                    STime,      /* I */
  long                    ETime,      /* I */
  long                    Time,       /* I (optional) */
  int                     SpecPCount) /* I */

  {
  /* FILE FORMAT: <Data><OBJECTTYPE OBJECTID="blah"><Profile */

  mnust_t Stat;

  mnust_t *S;

  int  PCount;
  int  PDuration;

  int  IStatCountTotal;

  char StatFile[MMAX_LINE];

  long MidNight;

  long tmpTime;
  long CurrentTime;
  
  int NewSIndex = 0;
  int sindex;

  int gindex;

  if ((SpecS == NULL) || (NewS == NULL))
    {
    return(FAILURE);
    }

  if (SpecS->IStat == NULL)
    {
    return(SUCCESS);
    }

  S = SpecS;

  if (Time < 0)
    {
    CurrentTime = MSched.Time;
    }
  else
    {
    CurrentTime = Time;
    }

   MUGetPeriodRange(CurrentTime,0,0,mpDay,&MidNight,NULL,NULL);

  /* if STime is not specified start at midnight of current day */

  if (STime <= 0)
    {
    tmpTime = MidNight;
    }
  else
    {
    tmpTime = STime;
    }

  PCount    = MStat.P.MaxIStatCount;
  PDuration = MStat.P.IStatDuration;

  sindex = 0;

  /* if ETime is not specified end now */

  if (ETime <= 0)
    {
    PCount = (CurrentTime - tmpTime) / PDuration;
    PCount += (MCONST_DAYLEN / PDuration);
    }
  else
    {
    PCount = (ETime - tmpTime) / PDuration;
    PCount += (MCONST_DAYLEN / PDuration);
    }

  /* determine whether we need to get data from files */

  Stat.IStat = NULL;

  if ((tmpTime < (long)SpecS->IStat[sindex]->StartTime) || 
      (tmpTime < MidNight))
    {
    /* requested starttime is before real data and is not from today */

    long Day = tmpTime;

    char Suffix[MMAX_LINE];
    char tmpVal[MMAX_BUFFER];
    char tmpLine[MMAX_BUFFER];

    enum MNodeStatTypeEnum saindex;

    int  CTok;
    int  aindex;
    int  tmpI;

    char *FileBuf;
    char *ptr;
    char *TokPtr;

    mxml_t *CE;
    mxml_t *SE;
    mxml_t *PE;
   
    mxml_t *DDE = NULL; 

    /* requested data is older than existing (memory) data and older than today's data */

    Stat.IStat = (mnust_t **)MUCalloc(1,sizeof(mnust_t *) * PCount);

    S = &Stat;

    S->IStatCount   = 0;
    IStatCountTotal = 0;

    while (Day < MidNight)
      {
      /* get data that is older than today from files */

      FileBuf = NULL;

      MStatGetFName(Day,Suffix,sizeof(Suffix));

      sprintf(StatFile,"%s%s.%s",
        MStat.StatDir,
        MCalPeriodType[mpDay],
        Suffix);

      if (MFUGetInfo(StatFile,NULL,NULL,NULL,NULL) != SUCCESS)
        {
        Day += MCONST_DAYLEN;

        continue;
        }

      /* load in from file */

      if (((FileBuf = MFULoadNoCache(StatFile,1,NULL,NULL,NULL,NULL)) == NULL) ||
           (MXMLFromString(&DDE,FileBuf,NULL,NULL) == FAILURE))
        {
        Day += MCONST_DAYLEN;

        MUFree(&FileBuf);

        continue;          
        }

      CTok = -1;

      while (MXMLGetChild(DDE,(char *)MXO[mxoNode],&CTok,&CE))
        {
        if ((CE->ACount <= 0) || (strcmp(CE->AVal[0],N->Name) != 0))
          continue;

        break;
        }

      if (CE == NULL)
        {
        Day += MCONST_DAYLEN;

        MUFree(&FileBuf);
        MXMLDestroyE(&DDE);

        continue;
        }

      if (MXMLGetChild(CE,(char *)MNodeStatType[mnstProfile],NULL,&PE) == FAILURE)
        {
        if ((MXMLGetChild(CE,(char *)MXO[mxoxStats],NULL,&SE) == FAILURE) ||
            (MXMLGetChild(SE,(char *)MNodeStatType[mnstProfile],NULL,&PE) == FAILURE))
          {
          Day += MCONST_DAYLEN;

          MUFree(&FileBuf);
          MXMLDestroyE(&DDE);

          continue;
          }
        }

      tmpI = 0;

      for (aindex = 0;aindex < PE->ACount;aindex++)
        {
        if (!strcmp(PE->AName[aindex],"SpecDuration"))
          {
          tmpI = strtol((char *)PE->AVal[aindex],NULL,10);

          break;
          }
        }

      if ((tmpI == 0) || (tmpI != PDuration))
        {
        Day += MCONST_DAYLEN;

        MUFree(&FileBuf);
        MXMLDestroyE(&DDE);

        continue;
        }

      for (aindex = 0;aindex < PE->ACount;aindex++)
        {
        saindex = (enum MNodeStatTypeEnum)MUGetIndex2(PE->AName[aindex],MNodeStatType,mnstNONE);

        if (saindex == mnstNONE)
          continue;

        if (saindex == mnstGMetric)
          {
          gindex = MUMAGetIndex(meGMetrics,PE->AName[aindex] + strlen(MNodeStatType[mnstGMetric]) + 1,mAdd) - 1;
          }
        else if (saindex == mnstAGRes)
          {
          gindex = MUMAGetIndex(meGRes,PE->AName[aindex] + strlen(MNodeStatType[mnstAGRes]) + 1,mAdd);
          }
        else if (saindex == mnstCGRes)
          {
          gindex = MUMAGetIndex(meGRes,PE->AName[aindex] + strlen(MNodeStatType[mnstCGRes]) + 1,mAdd);
          }
        else if (saindex == mnstDGRes)
          {
          gindex = MUMAGetIndex(meGRes,PE->AName[aindex] + strlen(MNodeStatType[mnstDGRes]) + 1,mAdd);
          }
        else
          {
          gindex = 0;
          }

        MUStrCpy(tmpVal,PE->AVal[aindex],sizeof(tmpVal));

        MUStrCpy(tmpLine,tmpVal,sizeof(tmpLine));

        if ((saindex != mnstStartTime) && (strchr(tmpLine,'*') != NULL))
          {
          MStatPExpand(tmpLine,sizeof(tmpLine));
          MStatPExpand(tmpVal,sizeof(tmpVal));
          }
        else if (saindex == mnstStartTime)
          {
          MStatPStarttimeExpand(tmpLine,sizeof(tmpLine),MStat.P.IStatDuration);
          MStatPStarttimeExpand(tmpVal,sizeof(tmpVal),MStat.P.IStatDuration);
          }

        ptr = MUStrTok(tmpVal,",",&TokPtr);

        S->IStatCount = IStatCountTotal;

        while ((ptr != NULL) && (S->IStatCount < PCount))
          {
          if (S->IStat[S->IStatCount] == NULL)
            {
            MStatCreate((void **)&S->IStat[S->IStatCount],msoNode);

            MStatPInit(S->IStat[S->IStatCount],msoNode,S->IStatCount,MStat.P.IStatEnd);
            }

          if (saindex == mnstStartTime)
            {
            S->IStat[S->IStatCount]->StartTime = strtol(ptr,NULL,10);
            }
          else if (saindex == mnstIStatCount)
            {
            /* if restoring checkpoint, use configured value, not checkpointed value */

            /* NO-OP */
            }
          else 
            {
            MStatSetAttr(
              (void *)S->IStat[S->IStatCount],
              msoNode,
              saindex,
              (void **)ptr,
              gindex,
              mdfString,
              mSet);
            }

          S->IStatCount++;

          ptr = MUStrTok(NULL,",",&TokPtr);
          } /* END while (ptr != NULL) */
        }   /* END for (aindex) */

      IStatCountTotal = S->IStatCount;

      Day += MCONST_DAYLEN;

      MUFree(&FileBuf);
      MXMLDestroyE(&DDE);
      }   /* END while (Day < (ETime <= 0) ? CurrentTime : ETime) */

    /* check if data from today is required */

    if (ETime > MidNight)
      {
      while ((SpecS->IStat[sindex] != NULL) && 
             ((long)SpecS->IStat[sindex]->StartTime < MidNight))
        {
        /* this data was already gathered from the files... can be ignored */

        sindex++;
        }

      while ((S->IStatCount < PCount) &&
             (sindex < SpecS->IStatCount) &&
             (SpecS->IStat[sindex] != NULL) && 
             ((long)SpecS->IStat[sindex]->StartTime <= ETime))
        {
        /* data from today is required, load everything we have into S */

        MStatPCopy((void **)&S->IStat[S->IStatCount],SpecS->IStat[sindex],msoNode);

        sindex++;
        S->IStatCount++;
        }
      }   /* END if (ETime > MidNight) */
    }     /* END if ((tmpTime > (S->IStat[sindex]->StartTime) ...*/

  /* this section builds a complete, hole-free, structure filled with good data (where possible)
     and 0's where no data exists */

  sindex = 0;

  while (NewSIndex < SpecPCount)
    {
    if ((sindex >= PCount) ||
        (sindex >= S->IStatCount) ||
        (S->IStat[sindex] == NULL) ||
        ((long)S->IStat[sindex]->StartTime > tmpTime))
      {
      /* no existing data for this time slot */

      MStatCreate((void **)&NewS[NewSIndex],msoNode);

      MStatPInit((void *)NewS[NewSIndex],msoNode,NewSIndex,tmpTime);
      }  /* END if ((S->IStat[sindex] == NULL) || ...) */
    else if ((long)S->IStat[sindex]->StartTime < tmpTime)
      {
      /* existing data is earlier than needed */

      sindex++;

      continue;
      }
    else
      {
      /* copy existing data */

      MStatPCopy((void **)&NewS[NewSIndex],S->IStat[sindex],msoNode);

      sindex++;
      }

    NewSIndex++;

    tmpTime += PDuration;
    }  /* END while (S->IStat[sindex] != NULL) */

  MStatPDestroy((void ***)&Stat.IStat,msoNode,SpecPCount);

  return(SUCCESS);
  }  /* END MStatValidateNPData() */

/* END MStatValidate.c */
