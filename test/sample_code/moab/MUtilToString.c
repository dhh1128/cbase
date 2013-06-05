/* HEADER */

      
/**
 * @file MUtilToString.c
 *
 * Contains: Misc ToString functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 * Display mcres_t structure in specified mode.
 *
 * @param R (I)
 * @param WallTime (I)
 * @param DisplayMode (I) [one of mfmHuman, mfmAVP, or mfmVerbose]
 * @param Buf (O) [optional,minsize=MMAX_LINE]
 */

char *MUUResToString(

  mcres_t             *R,           /* I */
  long                 WallTime,    /* I */
  enum MFormatModeEnum DisplayMode, /* I (one of mfmHuman, mfmAVP, or mfmVerbose) */
  char                *Buf)         /* O (optional,minsize=MMAX_LINE) */

  {
  static char LocalBuf[MMAX_BUFFER];

  /* should sync with (or replace with) MResourceType[] */

  const char *ResName[] = {
    "PROCS",
    "MEM",
    "SWAP",
    "DISK",
    NULL };

  char *BPtr;
  int   BSpace;

  char *Head;

  int index;

  int *ResPtr[4];

  int   Val;
  char *N;

  int tmpI;

  if (Buf != NULL)
    {
    MUSNInit(&BPtr,&BSpace,Buf,MMAX_LINE);
    }
  else
    {
    MUSNInit(&BPtr,&BSpace,LocalBuf,sizeof(LocalBuf));
    }

  Head = BPtr;

  ResPtr[0] = &R->Procs;
  ResPtr[1] = &R->Mem;
  ResPtr[2] = &R->Swap;
  ResPtr[3] = &R->Disk;

  /* FORMAT:  <ATTR>=<VAL>[;<ATTR>=<VAL>]... */

  for (index = 0;ResName[index] != NULL;index++)
    {
    Val = *ResPtr[index];
    N   =  (char *)ResName[index];

    if (Val == 0)
      continue;

    if (Head[0] != '\0')
      {
      if (DisplayMode == mfmAVP)
        MUSNCat(&BPtr,&BSpace,";");
      else
        MUSNCat(&BPtr,&BSpace,"  ");
      }

    if (Val > 0)
      {
      tmpI = (WallTime > 0) ? Val / WallTime : Val;        
 
      if (DisplayMode == mfmVerbose)
        {
        /* human readable - percent */

        MUSNPrintF(&BPtr,&BSpace,"%s: %0.2f",
          N,
          (double)tmpI / 100.0);
        }
      else if (DisplayMode == mfmAVP)
        {
        /* machine readable */

        MUSNPrintF(&BPtr,&BSpace,"%s=%d",
          N,
          tmpI);
        }
      else
        {
        /* human readable - basic */

        if (index > 0)
          {
          MUSNPrintF(&BPtr,&BSpace,"%s: %s",
            N,
            MULToRSpec((long)tmpI,mvmMega,NULL));
          }
        else
          {
          double tmpD;

          tmpD = (((double)tmpI)/100.0);

          MUSNPrintF(&BPtr,&BSpace,"%s: %.02f",
            N,
            tmpD);
          }
        }
      }
    else 
      {
      if (DisplayMode == mfmAVP)
        {
        MUSNPrintF(&BPtr,&BSpace,"%s=%s",
          N,
          ALL);
        }
      else
        {
        MUSNPrintF(&BPtr,&BSpace,"%s: %s",
          N,
          ALL);
        }
      }
    }    /* END for (index)   */

  /* display classes */

  /* NYI */

  /* display generic resources */

  for (index = 1;index < MSched.M[mxoxGRes];index++)
    { 
    if (MGRes.Name[index][0] == '\0')
      break;

    if (MSNLGetIndexCount(&R->GenericRes,index) == 0)
      continue;

    if (DisplayMode == mfmAVP)
      {
      if (Head[0] != '\0')
        MUSNCat(&BPtr,&BSpace,";");

      MUSNPrintF(&BPtr,&BSpace,"gres=%s:%d",
        MGRes.Name[index],
        MSNLGetIndexCount(&R->GenericRes,index));
      }
    else
      {
      if (Head[0] != '\0')
        MUSNCat(&BPtr,&BSpace,"  ");

      MUSNPrintF(&BPtr,&BSpace,"%s: %d",
        MGRes.Name[index],
        MSNLGetIndexCount(&R->GenericRes,index));
      }
    }  /* END for (index) */

  if (Head[0] == '\0')
    strcpy(Head,NONE);
     
  return(Head); 
  }  /* END MUUResToString() */




/**
 *
 *
 * @param S (I) [soft limit]
 * @param H (I) [hard limit]
 * @param Buf (I) [optional]
 * @param BufSize (I)
 */

char *MSLimitToString(

  char *S,       /* I (soft limit) */
  char *H,       /* I (hard limit) */
  char *Buf,     /* I (optional) */
  int   BufSize) /* I */

  {
  char *ptr;

  static char tmpLine[MMAX_LINE];

  if (Buf != NULL)
    {
    ptr = Buf;
    }
  else
    {
    ptr = tmpLine;
    BufSize = sizeof(tmpLine);
    }

  if ((S != NULL) && (S[0] != '\0') && strcmp(S,NONE))
    {
    snprintf(ptr,BufSize,"%s,%s",
      S,
      H);
    }
  else
    {
    snprintf(ptr,BufSize,"%s,%s", 
      NONE, 
      H);
    }

  return(ptr);
  }  /* END MSLimitToString() */





/**
 *
 *
 * @param SArray (I)
 * @param SBM (I) [soft limit]
 * @param HBM (I) [hard limit]
 * @param Buf (I) [optional]
 * @param BufSize (I)
 */

char *MSLimitBMToString(

  const char **SArray,  /* I */
  mbitmap_t   *SBM,     /* I (soft limit) */
  mbitmap_t   *HBM,     /* I (hard limit) */
  char        *Buf,     /* I (optional) */
  int          BufSize) /* I */

  {
  char *Head;

  char *BPtr = NULL;
  int   BSpace = 0;

  char  tmpSLine[MMAX_LINE];
  char  tmpHLine[MMAX_LINE];

  static char tmpLine[MMAX_LINE];

  tmpSLine[0] = 0;
  tmpHLine[0] = 0;

  if (Buf != NULL)
    {
    Head = Buf;

    MUSNInit(&BPtr,&BSpace,Buf,BufSize);
    }
  else
    {
    Head = tmpLine;

    MUSNInit(&BPtr,&BSpace,tmpLine,sizeof(tmpLine));
    }

  bmtostring(SBM,SArray,tmpSLine,"+");
  bmtostring(HBM,SArray,tmpHLine,"+");

  if (tmpSLine[0] != '\0')
    {
    MUSNPrintF(&BPtr,&BSpace,"%s,%s",
      tmpSLine,
      tmpHLine);
    }
  else
    {
    MUSNPrintF(&BPtr,&BSpace,"%s,%s",
      NONE,
      tmpHLine);
    }

  return(Head);
  }  /* END MSLimitBMToString() */




/**
 * Create comma delimited taskmap string.
 *
 * NOTE: There is also an MUTaskMapToMString()
 *
 * @param TM (I) [terminated w/-1]
 * @param SpecD (I) [optional]
 * @param PrefixD (I) [optional]
 * @param Buf (O)
 * @param BufSize (I)
 */

char *MUTaskMapToString(

  int   *TM,       /* I (terminated w/-1) */
  const char  *SpecD,    /* I task delimiter (optional) */
  char   PrefixD,  /* I (optional) */
  char  *Buf,      /* O */
  int    BufSize)  /* I */

  {
  char *BPtr = NULL;
  int   BSpace = 0;

  int   tindex;

  const char *DDelim = ",";

  const char *Delim;

  char *ptr;

  if (Buf != NULL)
    MUSNInit(&BPtr,&BSpace,Buf,BufSize);

  if ((TM == NULL) || (Buf == NULL))
    {
    return(NULL);
    }

  Delim = (SpecD != NULL) ? SpecD : DDelim;

  for (tindex = 0;tindex < MSched.JobMaxTaskCount;tindex++)
    {
    if (TM[tindex] < 0)
      break;

    if (tindex > 0)
      MUSNCat(&BPtr,&BSpace,Delim);

    if ((PrefixD == '\0') || ((ptr = strchr(MNode[TM[tindex]]->Name,PrefixD)) == NULL))
      {
      MUSNCat(&BPtr,&BSpace,MNode[TM[tindex]]->Name);
      }
    else
      {
      MUSNCat(&BPtr,&BSpace,ptr + 1);
      }
    }  /* END for (tindex) */

  if (BSpace == 0)
    {
    /* buffer overflow */

    return(NULL);
    }

  return(Buf);
  }  /* END MUTaskMapToString() */


/**
 * Create comma delimited taskmap mstring.
 *
 * @param TM      (I) [terminated w/-1]
 * @param SpecD   (I) [optional]
 * @param PrefixD (I) [optional]
 * @param Buf     (O) [must be initialized]
 */

char *MUTaskMapToMString(

  int        *TM,
  const char *SpecD,
  char        PrefixD,
  mstring_t  *Buf)

  {
  int   tindex;

  const char *DDelim = ",";
  const char *Delim;

  char *ptr;

  if ((TM == NULL) || (Buf == NULL))
    {
    return(NULL);
    }

  Buf->clear();

  Delim = (SpecD != NULL) ? SpecD : DDelim;

  for (tindex = 0;tindex < MSched.JobMaxTaskCount;tindex++)
    {
    if (TM[tindex] < 0)
      break;

    if (tindex > 0)
      MStringAppend(Buf,Delim);

    if ((PrefixD == '\0') || ((ptr = strchr(MNode[TM[tindex]]->Name,PrefixD)) == NULL))
      {
      MStringAppend(Buf,MNode[TM[tindex]]->Name);
      }
    else
      {
      MStringAppend(Buf,ptr + 1);
      }
    }  /* END for (tindex) */

  return(Buf->c_str());
  }  /* END MUTaskMapToMString() */





/**
 * Report resource factor/threshold array as string 
 * 
 * @see MUResFactorArrayFromString()
 * @see MVMGetUsageThresholdString()
 *
 * @param ResFactor (I) [array of mrlXXX]
 * @param GMetricFactor (I) [array of size MSched.M[mxoxGMetric]]
 * @param String (O)
 */

int MUResFactorArrayToString(

  double    *ResFactor,
  double    *GMetricFactor,
  mstring_t *String)

  {
  int   rindex;

  MStringSet(String,"\0");

  for (rindex = 0;rindex < mrlLAST;rindex++)
    {

    /* gmetrics will be displayed individually below */

    if (rindex == mrlGMetric)
      continue;

    if (ResFactor[rindex] > 0.0)
      {
      MStringAppendF(String,"%s%s:%.2f",
        (!MUStrIsEmpty(String->c_str())) ? "," : "",
        MResLimitType[rindex],
        ResFactor[rindex]);
      }
    }  /* END for (rindex) */

  if (GMetricFactor != NULL)
    {
    for (rindex = 1;rindex < MSched.M[mxoxGMetric];rindex++)
      {
      if (GMetricFactor[rindex] != MDEF_GMETRIC_VALUE)
        {
        MStringAppendF(String,"%sGMETRIC:%s:%.2f",
          (!MUStrIsEmpty(String->c_str())) ? "," : "",
          MGMetric.Name[rindex],
          GMetricFactor[rindex]);
        }
      }  /* END for (rindex) */
    }  /* END (GMetricFactor != NULL) */

  return(SUCCESS);
  }  /* END MUResFactorArrayToString() */






/**
 * convert resource factor string to resource factor/threshold array
 *
 * @see MUResFactorArrayToString() - peer
 *
 * @param ResString (I) [modified] - The string definition of the overcommit factors/thresholds
 * @param ResFactor (O) - Resource overcommit factors/thresholds (procs, mem, etc.)
 * @param GMetricFactor (O) - GMetric overcommit factors/thresholds
 * @param IsOvercommit (I) -
 * @param BoundValues (I) - Values are bound between 0 and 1 (ResFactor only)
 */

int MUResFactorArrayFromString(

  char        *ResString,    /* I (modified as side-affect) */
  double      *ResFactor,    /* O (minsize=mrlLAST) */
  double      *GMetricFactor, /* O */
  mbool_t      IsOvercommit, /* I */
  mbool_t      BoundValues)  /* I */

  {
  char *ptr;
  char *TokPtr = NULL;

  char *tail;

  double dval;

  int    rindex;

  /* FORMAT:  <RES>:<FACTOR>[,<RES>:<FACTOR>]... */

  ptr = MUStrTok(ResString,",",&TokPtr);

  while (ptr != NULL)
    {
    tail = NULL;
    rindex = MUGetIndexCI(ptr,MResLimitType,TRUE,0);

    if (rindex == mrlGMetric)
      {
      /* FORMAT:  GMETRIC:<NAME>:<VALUE> */

      char *GMetricName = strchr(ptr,':');

      if (GMetricName != NULL)
        {
        GMetricName++;

        if (*GMetricName != '\0')
          tail = strchr(GMetricName,':');

        if (tail != NULL)
          {
          char *tmpTail = tail;
          int GMIndex = 0;

          /* Zero this out so that GMetricName won't include the :<VALUE> stuff */
          *tmpTail = '\0';
          tail++;

          if (*tail != '\0')
            {
            dval = strtod(tail,NULL);

            GMIndex = MUMAGetIndex(meGMetrics,GMetricName,mAdd);

            /* Set it back to ':' just so there's no chance of messing up MUStrTok */
            *tmpTail = ':';

            if ((GMIndex > 0) && (GMetricFactor != NULL))
              {
              GMetricFactor[GMIndex] = dval;

              /* Set the 'standard' factor/threshold to indicate that there is a gmetric present */
              ResFactor[mrlGMetric] = 1.0;

              }
            }  /* END if (*tail != '\0') */
          }  /* END if (tail != NULL) */
        }  /* END if (GMetricName != NULL) */
      }  /* END if (rindex == mrlGMetric) */
    else if (rindex != 0)
      {
      tail = strchr(ptr,':');

      if (tail != NULL)
        {
        dval = strtod(tail + 1,NULL);

        if (BoundValues == TRUE)
          {
          dval = MAX(dval,0.0);
          dval = MIN(dval,1.0);
          }

        if (dval > 0.0)
          {
          if (IsOvercommit == TRUE)
            {
            MSched.ResOvercommitSpecified[0] = TRUE;
            MSched.ResOvercommitSpecified[rindex] = TRUE;
            }

          ResFactor[rindex] = dval;
          }
        }
      }

    ptr = MUStrTok(NULL,",",&TokPtr);
    }  /* END while (ptr != NULL) */

  return(SUCCESS);
  }  /* END MUResFactorArrayFromString() */

/* END MUtilToString.c */
