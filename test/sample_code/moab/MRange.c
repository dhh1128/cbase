/* HEADER */

      
/**
 * @file MRange.c
 *
 * Contains: Reservation Range functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  

/* Local prototypes */
mbool_t __GetMergeRange(mbool_t,mbool_t,long,long,long,long);
long    __GetNextRangeDuration(mrange_t *,mbool_t,mbool_t,long,int);
int     __MRLMergeTimeMergeRanges(mrange_t *,mrange_t *,int *,int *,long *,mbool_t,mbool_t,long,int);

/**
 *
 *
 * @param RL (I)
 * @param STime (I)
 * @param Duration (I)
 */

int MRLCheckTime(

  mrange_t *RL,       /* I */
  mulong    STime,    /* I */
  mulong    Duration) /* I */

  {
  int rindex;

  mulong    ATime;

  mrange_t *R;

  if (RL == NULL)
    {
    return(FAILURE);
    }

  ATime = 0;

  for (rindex = 0;rindex < MMAX_RANGE;rindex++)
    {
    R = &RL[rindex];

    if (R->ETime < STime)
      continue;

    if (R->STime > STime)
      break;

    if ((ATime > 0) && (R->STime != RL[rindex - 1].ETime))
      {
      /* ranges do not touch */

      return(FAILURE);
      }

    ATime += R->ETime - MAX(R->STime,STime);

    if (ATime >= Duration)
      {
      return(SUCCESS);
      }
    }  /* END for (rindex) */

  return(FAILURE);
  }  /* END MRLCheckTime() */






/**
 *
 *
 * @param String (I) [modified]
 * @param R (O) [minsize=MMAX_RANGE]
 */

int MRLFromString(

  char     *String,  /* I (modified) */
  mrange_t *R)       /* O (minsize=MMAX_RANGE) */

  {
  char *ptr;
  char *TokPtr = NULL;

  char *ptr2;
  char *TokPtr2 = NULL;

  int   rindex;

  if ((String == NULL) || (R == NULL))
    {
    return(FAILURE);
    }

  /* FORMAT:  <STIME>[-<ENDTIME>][{;,}<STIME>[-<ENDTIME>]]... */

  ptr = MUStrTok(String,",;",&TokPtr);

  rindex = 0;

  while (ptr != NULL)
    {
    ptr = MUStrTok(ptr,"-",&TokPtr2);
    ptr2 = MUStrTok(NULL,"-",&TokPtr2);

    /* accept relative, absolute, or epoch time specification */

    if (MUStringToE(ptr,(long *)&R[rindex].STime,TRUE) == FAILURE)
      {
      R[rindex].STime = 0;
      }

    if ((ptr2 == NULL) ||
        (MUStringToE(ptr2,(long *)&R[rindex].ETime,TRUE) == FAILURE))
      {
      R[rindex].ETime = MMAX_TIME;
      }

    rindex++;

    R[rindex].STime = 0;
    R[rindex].ETime = 0;

    if ((ptr2 == NULL) || (rindex >= MMAX_RANGE))
      break;

    ptr = MUStrTok(NULL,",;",&TokPtr);
    }  /* END while (ptr != NULL) */

  return(SUCCESS);
  }  /* END MRLFromString() */




/**
 * Return logical AND to R1 and R2 ranges. 
 *
 * NOTE:  if ANDTC is set, report minimum TC, if not, return R1 TC. 
 *
 * @param RDst (O) [may be R1]
 * @param R1 (I) [source range]
 * @param R2 (I) [mask range]
 * @param ANDTC (I) logically AND range taskcount
 */

int MRLAND(

  mrange_t *RDst,  /* O (may be R1) */
  mrange_t *R1,    /* I (source range) */
  mrange_t *R2,    /* I (mask range) */
  mbool_t   ANDTC) /* I logically AND range taskcount */

  {
  int index1;
  int index2;
  int cindex;

  mrange_t C[MMAX_RANGE] = {{0}};

  long ETime = 0;
  long STime = 0;

  long ETime2 = 0;
  long STime2 = 0;

  int  TC2 = 0;

  mbool_t IgnRange;

  /* step through range events */
  /* if R2 is not active, RDst is not active */

  index1 = 0;
  index2 = 0;
  cindex = 0;

  if ((R1[index1].ETime != 0) && (R2[index2].ETime != 0))
    {
    while ((R2[index2 + 1].ETime != 0) &&
           (R2[index2].ETime < R1[index1].STime))
      {
      /* ignore early ranges */

      index2++;
      }

    if (R2[index2].ETime >= R1[index1].STime)
      {
      STime2 = R2[index2].STime;
      TC2    = R2[index2].TC;

      /* merge adjacent mask ranges */

      while ((R2[index2 + 1].ETime != 0) &&
             (R2[index2].ETime == R2[index2 + 1].STime))
        {
        index2++;

        TC2 = MIN((mulong)TC2,R2[index2].ETime);
        }

      ETime2 = R2[index2].ETime;
      }
    }  /* END if ((R1[index1].ETime != 0) && ...) */

  for (index1 = 0;R1[index1].ETime != 0;index1++)
    {
    if (R2[index2].ETime == 0)
      break;

    /* process R1 range */

    while ((mulong)STime2 <= R1[index1].ETime)
      {
      STime = STime2;
      ETime = ETime2;

      /* determine next range */

      /* if current mask range endtime is exceeded by R1 range endtime */

      if ((mulong)ETime <= R1[index1].ETime)
        {
        index2++;

        if (R2[index2].ETime != 0)
          {
          /* ignore early ranges */

          while ((R2[index2].ETime < R1[index1].STime) &&
                 (R2[index2 + 1].ETime != 0))
            {
            index2++;
            }

          if (R2[index2].ETime < R1[index1].STime)
            {
            /* end of R2 list detected.  no overlapping ranges */

            STime2 = MMAX_TIME + 1;
            TC2    = 0;
            }
          else
            {
            STime2 = R2[index2].STime;
            TC2    = R2[index2].TC;

            /* merge adjacent mask ranges */

            while ((R2[index2 + 1].ETime != 0) &&
                   (R2[index2].ETime == R2[index2 + 1].STime))
              {
              index2++;

              TC2 = MIN(TC2,(long)R2[index2].ETime);
              }

            ETime2 = R2[index2].ETime;
            }
          }
        else
          {
          /* end of list reached */

          STime2 = MMAX_TIME + 1;
          TC2 = 0;
          }
        }    /* END if ((mulong)ETime <= R1[index1].ETime) */

      /* ignore early ranges */

      if ((mulong)ETime < R1[index1].STime)
        {
        continue;
        }

      /* overlapping range located */

      C[cindex].STime = MAX((mulong)STime,R1[index1].STime);
      C[cindex].ETime = MIN((mulong)ETime,R1[index1].ETime);

      if (ANDTC == TRUE)
        {
        C[cindex].TC  = MIN(R1[index1].TC,R2[index2].TC);
        C[cindex].NC  = MIN(R1[index1].NC,R2[index2].TC);
        }
      else
        {
        C[cindex].TC  = R1[index1].TC;
        C[cindex].NC  = R1[index1].NC;
        }

      IgnRange = FALSE;

      if (cindex > 0)
        {
        /* eliminate reduced instant range */

        if ((C[cindex].TC <= C[cindex - 1].TC))
          {
          if (C[cindex].STime == C[cindex].ETime)
            {
            /* current range is reduced instant range */

            IgnRange = TRUE;
            }
          }
        else if ((C[cindex].TC >= C[cindex - 1].TC))
          {
          if (C[cindex - 1].STime == C[cindex - 1].ETime)
            {
            /* previous range is reduced instant range */

            C[cindex - 1].STime = C[cindex].STime;
            C[cindex - 1].ETime = C[cindex].ETime;
            C[cindex - 1].TC    = C[cindex].TC;
            C[cindex - 1].NC    = C[cindex].NC;

            IgnRange = TRUE;
            }
          }
        }    /* END if (cindex > 0) */

      if (IgnRange == FALSE)
        cindex++;

      if (cindex >= (MMAX_RANGE - 1))
        {
        /* range buffer full */

        break;
        }

      if (R1[index1].ETime < (mulong)ETime)
        {
        /* advance R1 range */

        break;
        }
      }  /* END while (STime2 <= R1[index1].ETime) */
    }    /* END for (index1) */

  /* terminate range */

  C[cindex].ETime = 0;
  C[cindex].NC = R1[index1].NC;
  C[cindex].TC = R1[index1].TC;

  if (RDst != NULL)
    memcpy(RDst,C,sizeof(C));

  return(SUCCESS);
  }  /* END MRLAND() */





/**
 *
 *
 * @param RL (I) [modified]
 */

int MRLClear(

  mrange_t  *RL)  /* I (modified) */

  {
  if (RL == NULL)
    {
    return(FAILURE);
    }

  RL[0].ETime = 0;

  return(SUCCESS);
  }  /* END MRLClear() */





/**
 * NOTE:  range max size = MMAX_RANGE
 * NOTE:  re max size = MMAX_RANGE << 1
 *
 * @param RE (O)
 * @param R1 (I)
 * @param R2 (I)
 */

int MRLGetRE(

  mre_t    **RE,  /* O */
  mrange_t  *R1,  /* I */
  mrange_t  *R2)  /* I */

  {
  int R1Index;
  int R2Index;

  enum MREEnum R1State;
  enum MREEnum R2State;

  mre_t tmpRE = {0};

  mulong RETime;
  mulong R1Time;
  mulong R2Time;

  const char *FName = "MRLGetRE";

  MDB(5,fSTRUCT) MLog("%s(%s,%s,%s)\n",
    FName,
    (RE != NULL) ? "RE" : "NULL",
    (R1 != NULL) ? "R1" : "NULL",
    (R2 != NULL) ? "R2" : "NULL");

  if (RE == NULL)
    {
    return(FAILURE);
    }

  if (R1 == NULL)
    {
    return(FAILURE);
    }

  if (R2 == NULL)
    {
    return(FAILURE);
    }

  /* return time sorted list of events available within the union of R1 and R2 */
  /* NOTE:  if each range has event at a given time, return re for each */

  /* NOTE:  only populates event type, event index, and event time (OIndex forced to '1') */

  R1Index = 0;
  R2Index = 0;

  R1State = mreStart;
  R2State = mreStart;

  /* NOTE:  precedence:
      R1 << R2
      End << Instant << Start
  */

  while ((R1[R1Index].ETime != 0) ||
         (R2[R2Index].ETime != 0))
    {
    if (R1[R1Index].ETime == 0)
      {
      R1State = mreNONE;
      R1Time  = MMAX_TIME + 1;
      }
    else
      {
      R1Time = (R1State == mreStart) ? R1[R1Index].STime : R1[R1Index].ETime;
      }

    if (R2[R2Index].ETime == 0)
      {
      R2State = mreNONE;
      R2Time  = MMAX_TIME + 1;
      }
    else
      {
      R2Time = (R2State == mreStart) ? R2[R2Index].STime : R2[R2Index].ETime;
      }

    RETime = MIN(R1Time,R2Time);

    tmpRE.Time = RETime;

    /* handle end */

    if ((RETime == R1Time) && (R1State == mreEnd))
      {
      tmpRE.Type   = mreEnd;

      MREAddSingleEvent(RE,&tmpRE);

      R1Index++;

      R1State = mreStart;

      if ((RETime == R2Time) && (R2State == mreEnd))
        {
        tmpRE.Time = RETime;
        tmpRE.Type = mreEnd;

        MREAddSingleEvent(RE,&tmpRE);

        R2Index++;

        R2State = mreStart;
        }

      continue;
      }

    if ((RETime == R2Time) && (R2State == mreEnd))
      {
      tmpRE.Type = mreEnd;

      MREAddSingleEvent(RE,&tmpRE);

      R2Index++;

      R2State = mreStart;

      continue;
      }

    /* handle instant */

    if ((RETime == R1Time) && (R1State == mreStart) && (R1[R1Index].ETime == R1Time))
      {
      tmpRE.Type  = mreInstant;

      MREAddSingleEvent(RE,&tmpRE);

      R1Index++;

      R1State = mreStart;

      if (RETime == R2Time)
        {
        tmpRE.Time = RETime;
        tmpRE.Type  = mreInstant;

        MREAddSingleEvent(RE,&tmpRE);

        R2Index++;

        R2State = mreStart;
        }

      continue;
      }

    if ((RETime == R2Time) && (R2State == mreStart) && (R2[R2Index].ETime == R2Time))
      {
      tmpRE.Type = mreInstant;

      MREAddSingleEvent(RE,&tmpRE);

      R2Index++;

      R2State = mreStart;

      continue;
      }

    /* handle start */

    if ((RETime == R1Time) && (R1State == mreStart))
      {
      tmpRE.Type   = mreStart;

      MREAddSingleEvent(RE,&tmpRE);

      R1State = mreEnd;

      if (RETime == R2Time)
        {
        tmpRE.Time = RETime;
        tmpRE.Type   = mreStart;

        MREAddSingleEvent(RE,&tmpRE);

        R2State = mreEnd;
        }

      continue;
      }

    if ((RETime == R2Time) && (R2State == mreStart))
      {
      tmpRE.Type   = mreStart;

      MREAddSingleEvent(RE,&tmpRE);

      R2State = mreEnd;

      continue;
      }
    }    /* END while ((R1[R1Index].ETime != 0) || ...) */

  return(SUCCESS);
  }  /* END MRLGetRE() */






/* max size is MMAX_RANGE */

/**
 *
 *
 * @param Result (O)
 * @param R1 (I)
 * @param R2 (I)
 */

int MRLGetIntersection(

  mrange_t  *Result, /* O */
  mrange_t  *R1,     /* I */
  mrange_t  *R2)     /* I */

  {
  int      RIndex;
  int      REIndex;

  int      ActiveRC;  /* number of ranges currently active */

  mrange_t tmpR[MMAX_RANGE];

  mre_t   *RE = NULL;
  mre_t   *tmpRE = NULL;
  mre_t   *PrevRE = NULL;
  mre_t   *NextRE = NULL;

  const char *FName = "MRLGetIntersection";

  MDB(5,fSTRUCT) MLog("%s(%s,%s,%s)\n",
    FName,
    (Result != NULL) ? "Result" : "NULL",
    (R1 != NULL) ? "R1" : "NULL",
    (R2 != NULL) ? "R2" : "NULL");

  if (Result == NULL)
    {
    return(FAILURE);
    }

  if (R1 == NULL)
    {
    return(FAILURE);
    }

  if (R2 == NULL)
    {
    return(FAILURE);
    }

  /* NOTE:  Result and R1 may point to the same object */

  /* return TC=1 for all ranges which are satisfied by both R1 and R2 timeframes */

  /* NOTE:  sum overlapping portions of ranges */

  MRLClear(tmpR);

  MRLGetRE(&tmpRE,R1,R2);

  REIndex = 0;
  RIndex  = 0;

  ActiveRC = 0;

  PrevRE = NULL;

  for (RE = tmpRE;RE != NULL;((PrevRE = RE) && MREGetNext(RE,&RE)))
    {
    MREGetNext(RE,&NextRE);

    if (REIndex >= (MMAX_RANGE << 2) - 1)
      {
      /* NOTE:  2 range events may be created per pass */

      break;
      }

    if (RE->Type == mreStart)
      {
      ActiveRC++;

      if (ActiveRC == 2)
        {
        /* intersection located (two ranges active) */

        /* start new range */

        tmpR[RIndex].STime = RE->Time;
        tmpR[RIndex].TC = 1;
        tmpR[RIndex].NC = 1;
        }

      REIndex++;
      }
    else if (RE->Type == mreEnd)
      {
      ActiveRC--;

      if (ActiveRC == 1)
        {
        /* close existing range */

        tmpR[RIndex].ETime = RE->Time;
        RIndex++;
        }

      REIndex++;
      }
    else if (RE->Type == mreInstant)
      {
      /* NOTE:  valid intersections:  E-I / I-I / I-S / S->I->E */

      if (ActiveRC == 1)
        {
        /* S->I->E */

        /* create instant range */

        tmpR[RIndex].STime = RE->Time;
        tmpR[RIndex].TC    = 1;
        tmpR[RIndex].NC    = 1;
        tmpR[RIndex].ETime = RE->Time;
        RIndex++;
        }
      else if ((REIndex > 0) &&
               (PrevRE->Time == tmpRE[REIndex].Time))
        {
        /* E-I */

        /* create instant range */

        tmpR[RIndex].STime = RE->Time;
        tmpR[RIndex].TC    = 1;
        tmpR[RIndex].NC    = 1;
        tmpR[RIndex].ETime = RE->Time;
        RIndex++;
        }
      else if ((NextRE->Type == mreStart) &&
               (NextRE->Time == tmpRE[REIndex].Time))
        {
        /* I-S */

        /* create instant range */

        tmpR[RIndex].STime = RE->Time;
        tmpR[RIndex].TC    = 1;
        tmpR[RIndex].NC    = 1;
        tmpR[RIndex].ETime = RE->Time;
        RIndex++;
        }
      else if ((NextRE->Type == mreInstant) &&
               (NextRE->Time == tmpRE[REIndex].Time))
        {
        /* I-I */

        /* create instant range */

        tmpR[RIndex].STime = RE->Time;
        tmpR[RIndex].TC    = 1;
        tmpR[RIndex].NC    = 1;
        tmpR[RIndex].ETime = RE->Time;
        RIndex++;

        REIndex++;
        }

      REIndex++;
      }  /* END else if (tmpRE[REIndex].Type == mreInstant) */
    }    /* END while (tmpRE[REIndex].Type != mreNONE)      */

  tmpR[RIndex].ETime = 0;

  memcpy(Result,tmpR,sizeof(mrange_t) * (RIndex + 1));

  return(SUCCESS);
  }      /* END MRLGetIntersection() */




/**
 *
 *
 * @param R (I)
 * @param Time (I)
 */

int MRangeGetTC(

  mrange_t *R,     /* I */
  mulong    Time)  /* I */

  {
  int rindex;

  if ((R == NULL) || (Time <= 0))
    {
    /* FAILURE */

    return(0);
    }

  for (rindex = 0;rindex < MMAX_RANGE;rindex++)
    {
    if (R[rindex].ETime == 0)
      break;

    if (R[rindex].ETime < Time)
      continue;

    if (R[rindex].STime > Time)
      break;

    return(R[rindex].TC);
    }  /* END for (rindex) */
  /* FAILURE */

  return(0);
  }  /* END MRangeGetTC() */




/**
 *
 *
 * @param R (I) [modified]
 * @param Offset (I)
 */

int MRLOffset(

  mrange_t *R,       /* I (modified) */
  mulong    Offset)  /* I */

  {
  int index;

  const char *FName = "MRLOffset";

  MDB(8,fSTRUCT) MLog("%s(R,%ld)\n",
    FName,
    Offset);

  if (R == NULL)
    {
    return(FAILURE);
    }

  /* initialize variables */

  for (index = 0;index < MMAX_RANGE;index++)
    {
    if (R[index].ETime == 0)
      break;

    R[index].STime += Offset;
    R[index].ETime += Offset;

    R[index].ETime = MIN(R[index].ETime,MMAX_TIME);
    }  /* END for (index) */

  return(SUCCESS);
  }  /* END MRLOffset() */





/**
 * Merge two availability ranges.
 *
 * @see MJobGetSNRange() - parent
 * @see __MSysTestRLMerge() - parent - blackbox test for MOABTEST
 *
 * @param R1 (I)
 * @param R2 (I)
 * @param MinTC (I)
 * @param SeekLong (I)
 * @param StartTime (O) [optional]
 */

int MRLMerge(

  mrange_t *R1,         /* I */
  mrange_t *R2,         /* I */
  int       MinTC,      /* I */
  mbool_t   SeekLong,   /* I */
  long     *StartTime)  /* O earliest time <MinTC> tasks are available (optional) */

  {
  int index1;
  int index2;
  int cindex;

  mrange_t C[MMAX_RANGE + 2] = {{0}};

  enum MREEnum R1State;
  enum MREEnum R2State;

  mulong R1Time;
  mulong R2Time;

  mulong ETime;

  int TC;
  int OldTC;

  int NC;
  int OldNC;

  mbool_t IsCorrupt;
  mbool_t IsRangeOverflow;

  const char *FName = "MRLMerge";

  MDB(8,fSTRUCT) MLog("%s(R1,R2,%d,%s,StartTime)\n",
    FName,
    MinTC,
    MBool[SeekLong]);

  if ((R1 == NULL) || (R2 == NULL))
    return(FAILURE);

  /* initialize variables */

  index1 = 0;
  index2 = 0;
  cindex = 0;

  R1State = mreStart; /* 'mreStart' : next time to be processed is start */
  R2State = mreStart;

  TC = 0;
  NC = 0;

  IsCorrupt = FALSE;
  IsRangeOverflow = FALSE;

  /* TC/NC indicate resources in current range */

  if (StartTime != NULL)
    *StartTime = MMAX_TIME;

  while ((R1[index1].ETime != 0) ||
         (R2[index2].ETime != 0))
    {
    if (cindex >= MMAX_RANGE)
      {
      MDB(2,fSTRUCT) MLog("ALERT:    range overflow in %s(), MMAX_RANGE=%d\n",
        FName,
        MMAX_RANGE);

      IsCorrupt = TRUE;
      IsRangeOverflow = TRUE;

      break;
      }

    if (R1[index1].ETime == 0)
      {
      R1State = mreNONE;
      R1Time  = MMAX_TIME + 1;
      }
    else
      {
      R1Time = (R1State != mreEnd) ? R1[index1].STime : R1[index1].ETime;
      }

    if (R2[index2].ETime == 0)
      {
      R2State = mreNONE;
      R2Time  = MMAX_TIME + 1;
      }
    else
      {
      R2Time = (R2State != mreEnd) ? R2[index2].STime : R2[index2].ETime;
      }

    ETime = MIN(R1Time,R2Time);

    if ((R1[index1].ETime == ETime) && (R1[index1].STime == ETime))
      R1State = mreInstant;

    if ((R2[index2].ETime == ETime) && (R2[index2].STime == ETime))
      R2State = mreInstant;

    /* handle instant states */

    if (((R1State == mreInstant) && (R1Time == ETime)) ||
        ((R2State == mreInstant) && (R2Time == ETime)))
      {
      if ((R1State == mreInstant) && (R1Time == ETime))
        {
        /* R1 instant, R2 ??? */

        if ((R2State == mreEnd) && (R2Time == ETime))
          {
          /* R1 instant, R2 bordering end */

          /* implied open range */

          C[cindex].ETime = ETime;
          cindex++;

          OldTC = TC;
          OldNC = NC;

          /* advance R2 */

          TC -= R2[index2].TC;
          NC -= R2[index2].NC;

          index2++;

          R2State = mreStart;

          /* create instant range */

          C[cindex].STime = ETime;

          if (R2[index2].ETime == 0)
            R2Time = MMAX_TIME + 1;
          else
            R2Time = R2[index2].STime;

          /* R1 instant, R2 ??? (new)                    */
          /* if instant R2 exists, it must be this range */

          if (R2Time == ETime)
            {
            C[cindex].TC = R1[index1].TC + R2[index2].TC;
            C[cindex].NC = R1[index1].NC + R2[index2].NC;

            if (R2Time == R2[index2].ETime)
              {
              /* R1 instant, R2 instant */

              TC = 0;
              NC = 0;

              index2++;

              R2State = mreStart;
              }
            else
              {
              /* R1 instant, R2 start */

              /* DO NOTHING */
              }
            }
          else
            {
            /* R1 instant only */

            C[cindex].TC = R1[index1].TC + OldTC;
            C[cindex].NC = R1[index1].NC + OldNC;
            }

          C[cindex].ETime = ETime;
          }   /* END if ((R2State == mreEnd) && (R2Time == ETime)) */
        else
          {
          if ((R2State == mreEnd) && (R1Time == ETime) && (C[cindex].STime != ETime))
            {
            /* implied non-instant open range - close it */

            C[cindex].ETime = ETime;
            cindex++;
            }

          /* implied closed range   */
         
          /* start instant range */

          C[cindex].STime = ETime;
          C[cindex].ETime = ETime;

          if (R2State == mreInstant)
            {
            /* R1 instant, R2 instant */

            TC = R1[index1].TC + R2[index2].TC;
            NC = R1[index1].NC + R2[index2].NC;

            C[cindex].TC = TC;
            C[cindex].NC = NC;

            TC = 0;
            NC = 0;

            index2++;

            R2State = mreStart;
            }
          else if ((R2State == mreStart) && (R2Time == ETime))
            {
            /* R1 instant, R2 start */

            C[cindex].TC = R1[index1].TC + R2[index2].TC;
            C[cindex].NC = R1[index1].NC + R2[index2].NC;
            }
          else
            {
            /* R1 instant only */

            C[cindex].TC = TC + R1[index1].TC;
            C[cindex].NC = NC + R1[index1].NC;
            }
          }

        index1++;

        R1State = mreStart;
        }
      else
        {
        /* R1 ???, R2 instant */
        /* R1 not instant     */

        if ((R1State == mreEnd) && (R1Time == ETime))
          {
          /* R2 instant, R1 bordering end */

          /* implied open range */

          C[cindex].ETime = ETime;
          cindex++;

          OldTC = TC;
          OldNC = NC;

          /* advance R1 */

          TC -= R1[index1].TC;
          NC -= R1[index1].NC;

          index1++;

          R1State = mreStart;

          /* create instant range */

          C[cindex].STime = ETime;

          if (R1[index1].ETime == 0)
            R1Time = MMAX_TIME + 1;
          else
            R1Time = R1[index1].STime;

          /* R2 instant, R1 ??? (new)                    */
          /* if instant R1 exists, it must be this range */

          if (R1Time == ETime)
            {
            C[cindex].TC = R1[index1].TC + R2[index2].TC;
            C[cindex].NC = R1[index1].NC + R2[index2].NC;

            if (R1Time == R1[index1].ETime)
              {
              /* R1 instant, R2 instant */

              index1++;

              R1State = mreStart;
              }
            else
              {
              /* R2 instant, R1 start */

              /* DO NOTHING */
              }
            }
          else
            {
            /* R2 instant only */

            C[cindex].TC = R2[index2].TC + OldTC;
            C[cindex].NC = R2[index2].NC + OldNC;
            }

          C[cindex].ETime   = ETime;
          }   /* END if ((R1State == mreEnd) && (R1Time == ETime)) */
        else
          {
          /* R1 not instant, R2 instant, R1 not end */

          if (TC > 0)
            {
            C[cindex].ETime = ETime;

            cindex++;
            }

          /* start instant range */

          C[cindex].STime = ETime;

          if ((R1State == mreStart) && (R1Time == ETime))
            {
            /* R2 instant, R1 start */

            C[cindex].TC = R2[index2].TC + R1[index1].TC;
            C[cindex].NC = R2[index2].NC + R1[index1].NC;
            }
          else
            {
            /* R2 instant only */

            C[cindex].TC = TC + R2[index2].TC;
            C[cindex].NC = NC + R2[index2].NC;
            }

          C[cindex].ETime   = ETime;
          }    /* END else ((R1State == mreEnd) && (R1Time == ETime)) */

        index2++;

        R2State = mreStart;
        }  /* END else ((R1State == mreInstant) && (R1Time == ETime)) */

      cindex++;

      if (TC > 0)
        {
        C[cindex].STime = ETime;
        C[cindex].TC = TC;
        C[cindex].NC = NC;
        }

      continue;
      }    /* END if (((R1State == mreInstant) && (R1Time == ETime)) || ... */

    if (((R1State == mreEnd) && (R1Time == ETime)) ||
        ((R2State == mreEnd) && (R2Time == ETime)))
      {
      /* implied open range */

      C[cindex].ETime = ETime;
      cindex++;

      if ((R1State == mreEnd) && (R1Time == ETime))
        {
        TC -= R1[index1].TC;
        NC -= R1[index1].NC;

        index1++;

        R1State = mreStart;
        }

      if ((R2State == mreEnd) && (R2Time == ETime))
        {
        TC -= R2[index2].TC;
        NC -= R2[index2].NC;

        index2++;

        R2State = mreStart;
        }

      if (TC > 0)
        {
        C[cindex].STime = ETime;
        C[cindex].TC = TC;
        C[cindex].NC = NC;
        }

      continue;
      }  /* END if (((R1State == mreEnd) && (R1Time == ETime)) || ... */

    if (((R1State == mreStart) && (R1Time == ETime)) ||
        ((R2State == mreStart) && (R2Time == ETime)))
      {
      if ((TC > 0) && (ETime > C[cindex].STime))
        {
        C[cindex].ETime = ETime;
        cindex++;
        }

      if ((R1State == mreStart) && (R1Time == ETime))
        {
        TC += R1[index1].TC;
        NC += R1[index1].NC;

        R1State = mreEnd;
        }

      if ((R2State == mreStart) && (R2Time == ETime))
        {
        TC += R2[index2].TC;
        NC += R2[index2].NC;

        R2State = mreEnd;
        }

      if (TC > 0)
        {
        C[cindex].STime = ETime;
        C[cindex].TC = TC;
        C[cindex].NC = NC;
        }

      continue;
      }    /* END if (((R1State == mreStart) && (R1Time == ETime)) || ...          */
    }      /* END while ((R1[index1].ETime != 0) || (R2[index2].ETime != 0)) */

  /* terminate range */

  cindex = MIN(cindex,MMAX_RANGE);

  C[cindex].ETime = 0;

  MSched.Max[mxoxRange] = MAX(MSched.Max[mxoxRange],cindex + 1);

  if (SeekLong == TRUE)
    {
    MRLLinkTime(C,0,MinTC);
    }

  MDB(3,fALL)
    {
    char StartTime[MMAX_NAME];
    char EndTime[MMAX_NAME];
    char TString[MMAX_LINE];

    MDB(6,fSCHED) MLog("INFO:     range count: %d\n",
      cindex);

    for (index1 = 0;C[index1].ETime != 0;index1++)
      {
      MULToTString(C[index1].STime - MSched.Time,TString);
      MUStrCpy(StartTime,TString,sizeof(StartTime));
      MULToTString(C[index1].ETime - MSched.Time,TString);
      MUStrCpy(EndTime,TString,sizeof(EndTime));

      MDB(5,fSCHED) MLog("INFO:     C[%02d]  S: %ld (%s)  E: %ld (%s) T: %3d  N: %d\n",
        index1,
        C[index1].STime,
        StartTime,
        C[index1].ETime,
        EndTime,
        C[index1].TC,
        C[index1].NC);

      if (C[index1].TC <= 0)
        {
        IsCorrupt = TRUE;

        MLog("ALERT:    corrupt taskcount detected (%d)\n",
          C[index1].TC);
        }

      if ((C[index1].NC <= 0) || (C[index1].NC > C[index1].TC))
        {
        IsCorrupt = TRUE;

        MLog("ALERT:    corrupt nodecount detected (%d)\n",
          C[index1].NC);
        }

      if (C[index1].ETime < C[index1].STime)
        {
        IsCorrupt = TRUE;

        MLog("ALERT:    corrupt time detected:  E(%ld) < S(%ld)\n",
          C[index1].ETime,
          C[index1].STime);
        }

      if ((C[index1 + 1].ETime != 0) &&
          (C[index1].ETime > C[index1 + 1].STime))
        {
        IsCorrupt = TRUE;

        MLog("ALERT:    corrupt time detected:  S2(%ld) < E1(%ld)\n",
          C[index1 + 1].STime,
          C[index1].ETime);
        }
      }    /* END for (index1) */
    }      /* END MDB(3,fALL) */

  if (IsCorrupt == TRUE)
    {
    if (IsRangeOverflow == TRUE)
      {
      char tmpLine[MMAX_LINE];

      snprintf(tmpLine,sizeof(tmpLine),"range overflow detected in %s (increase MMAX_RANGE > %d)",
        FName,
        MMAX_RANGE);

      MSchedSetAttr(
        &MSched,
        msaMessage,
        (void **)tmpLine,
        mdfString,
        mIncr);
      }
    else
      {
      MSchedSetAttr(
        &MSched,
        msaMessage,
        (void **)"reservation corruption detected in MRLMerge (contact support)",
        mdfString,
        mIncr);
      }

    MDB(5,fSCHED)
      {
      for (index1 = 0;R1[index1].ETime != 0;index1++)
        {
        MLog("INFO:     R1[%d]  S: %ld  E: %ld  T: %d  N: %d\n",
          index1,
          R1[index1].STime,
          R1[index1].ETime,
          R1[index1].TC,
          R1[index1].NC);
        }

      for (index1 = 0;R2[index1].ETime != 0;index1++)
        {
        MLog("INFO:     R2[%d]  S: %ld  E: %ld  T: %d  N: %d\n",
          index1,
          R2[index1].STime,
          R2[index1].ETime,
          R2[index1].TC,
          R2[index1].NC);
        }  /* END for (index1) */
      }    /* END MDB(5,fSCHED) */
    }      /* END if (IsCorrupt == TRUE) */

  memcpy(R1,C,sizeof(mrange_t) * (cindex + 1));

  return(SUCCESS);
  }   /* END MRLMerge() */





/**
 *
 *
 * @param GRange (I/O)
 * @param J (I)
 * @param AvlTaskCount (O)
 */

int MRangeApplyGlobalDistributionConstraints(

  mrange_t *GRange,       /* I/O */
  mjob_t   *J,            /* I */
  int      *AvlTaskCount) /* O */

  {
  int rindex;

  if (GRange == NULL)
    {
    return(FAILURE);
    }

  for (rindex = 0;rindex < MMAX_RANGE;rindex++)
    {
    /* NYI */
    }  /* END for (rindex) */

  return(SUCCESS);
  }  /* END MRangeApplyGlobalDistributionConstraints() */




/**
 *
 *
 * @param Range (I) [modified]
 * @param J (I)
 * @param N (I)
 */

int MRangeApplyLocalDistributionConstraints(

  mrange_t *Range, /* I (modified) */
  mjob_t   *J,     /* I */
  mnode_t  *N)     /* I */

  {
  int rindex;
  int MinTPN;

  int RangeAdjusted = FALSE;

  mrm_t *R;

  if ((Range == NULL) || (J == NULL))
    {
    return(FAILURE);
    }

  R = (J->DestinationRM != NULL) ? J->DestinationRM : &MRM[0];

  switch (R->Type)
    {
    case mrmtLL:

      if (J->Req[0]->TasksPerNode > 1)
        MinTPN = J->Req[0]->TasksPerNode;
      else
        MinTPN = 1;

      if ((J->Req[0]->BlockingFactor != 1) && (J->Request.NC > 0))
        {
        MinTPN = MAX(MinTPN,J->Request.TC / J->Request.NC);
        }

      break;

    default:

       MinTPN = 1;

       break;
    }  /* END switch (R->Type) */

  for (rindex = 0;Range[rindex].ETime > 0;rindex++)
    {
    switch (R->Type)
      {
      case mrmtLL:

        if (Range[rindex].TC < MinTPN)
          {
          Range[rindex].TC = 0;

          RangeAdjusted = TRUE;

          MDB(2,fSCHED) MLog("INFO:     range[%d] taskcount for node %s violates task distribution policies (%d < %d)\n",
            rindex,
            N->Name,
            Range[rindex].TC,
            MinTPN);
          }

        break;

      default:

        /* NO-OP */

        break;
      }  /* END switch (R->Type) */
    }    /* END for (rindex) */

  if (RangeAdjusted == TRUE)
    {
    int rindex2;

    /* remove empty ranges */

    rindex2 = 0;

    for (rindex = 0;Range[rindex].ETime > 0;rindex++)
      {
      if (Range[rindex].TC > 0)
        {
        if (rindex != rindex2)
          memcpy(&Range[rindex2],&Range[rindex],sizeof(mrange_t));

        rindex2++;
        }
      }    /* END for (rindex) */

    Range[rindex2].ETime = 0;
    }  /* END if (RangeAdjusted == TRUE) */

  return(SUCCESS);
  }  /* END MRangeApplyLocalDistributionConstraints() */





/**
 *
 *
 * @param ARL (I/O) [available]
 * @param URL (O) [utilized]
 * @param DRL (O)
 * @param TCMax (I)
 */

int MRLLimitTC(

  mrange_t *ARL,    /* I/O (available) */
  mrange_t *URL,    /* O   (utilized)  */
  mrange_t *DRL,    /* O   */
  int       TCMax)  /* I   */

  {
  int index1;
  int index2;
  int cindex;

  mrange_t *C;

  mrange_t tmpRL[MMAX_RANGE];

  long ATime;
  long UTime;

  int AType;
  int UType;

  long ETime;

  int OldTC;
  int TC;

  mbool_t IsInstantA;
  mbool_t IsInstantU;

  int ATC;
  int FTC;

  if ((ARL == NULL) || (URL == NULL))
    {
    return(FAILURE);
    }

  /* initialize variables */

  if (DRL != NULL)
    C = DRL;
  else
    C = tmpRL;

  index1 = 0;
  index2 = 0;
  cindex = 0;

  AType = mreStart;
  UType = mreStart;

  ATime = (ARL[0].ETime == 0) ? MMAX_TIME + 1 : ARL[0].STime;
  UTime = (URL[0].ETime == 0) ? MMAX_TIME + 1 : URL[0].STime;

  IsInstantA = ((ARL[index1].STime == ARL[index1].ETime) && (ARL[index1].ETime > 0)) ?
    TRUE :
    FALSE;

  IsInstantU = ((URL[index2].STime == URL[index2].ETime) && (URL[index2].ETime > 0)) ?
    TRUE :
    FALSE;

  FTC = TCMax;
  ATC = 0;

  TC = 0;

  while (TRUE)
    {
    if ((ARL[index1].ETime == 0) && (URL[index2].ETime == 0))
      {
      C[cindex].ETime = 0;

      break;
      }

    OldTC = TC;

    ETime = MIN(ATime,UTime);

    if (ATime == ETime)
      {
      if (AType == mreStart)
        {
        ATC += ARL[index1].TC;

        AType = mreEnd;
        ATime = ARL[index1].ETime;
        }
      else
        {
        ATC -= ARL[index1].TC;

        index1++;

        IsInstantA = ((ARL[index1].STime == ARL[index1].ETime) && (ARL[index1].ETime > 0)) ?
          TRUE :
          FALSE;

        AType = mreStart;
        ATime = (ARL[index1].ETime == 0) ? MMAX_TIME + 1: ARL[index1].STime;
        }
      }

    if (UTime == ETime)
      {
      if (UType == mreStart)
        {
        FTC -= URL[index2].TC;

        UType = mreEnd;
        UTime = URL[index2].ETime;
        }
      else
        {
        FTC += URL[index2].TC;

        index2++;

        IsInstantU = ((URL[index2].STime == URL[index2].ETime) && (URL[index2].ETime > 0)) ?
          TRUE :
          FALSE;

        UType = mreStart;
        UTime = (URL[index2].ETime == 0) ? MMAX_TIME + 1: URL[index2].STime;
        }
      }

    TC = MIN(ATC,FTC);

    if (TC != OldTC)
      {
      if (OldTC > 0)
        {
        if ((ETime > (long)C[cindex].STime) ||
            (IsInstantA == TRUE) ||
            (IsInstantU == TRUE))
          {
          /* terminate existing range */

          C[cindex].ETime = ETime;

          cindex++;
          }
        }

      if (TC > 0)
        {
        /* start new range */

        C[cindex].STime = ETime;
        C[cindex].TC = TC;
        }

      if (((ATime == ETime) && (IsInstantA == TRUE)) ||
          ((UTime == ETime) && (IsInstantU == TRUE)))
        {
        /* terminate instant range */

        C[cindex].ETime = ETime;

        cindex++;

        if ((ATime == ETime) && (IsInstantA == TRUE))
          {
          ATC += ARL[index1].TC;

          index1++;

          IsInstantA = ((ARL[index1].STime == ARL[index1].ETime) && (ARL[index1].ETime > 0)) ?
            TRUE :
            FALSE;

          AType = mreStart;
          ATime = (ARL[index1].ETime == 0) ? MMAX_TIME + 1: ARL[index1].STime;
          }

        if ((UTime == ETime) && (IsInstantU == TRUE))
          {
          FTC -= URL[index2].TC;

          index2++;

          IsInstantU = ((URL[index2].STime == URL[index2].ETime) && (URL[index2].ETime > 0)) ?
            TRUE :
            FALSE;

          UType = mreStart;
          UTime = (URL[index2].ETime == 0) ? MMAX_TIME + 2: URL[index2].STime;
          }

        TC = MIN(ATC,FTC);

        if (TC > 0)
          {
          /* start new range */

          C[cindex].STime = ETime;
          C[cindex].TC = TC;
          }
        }
      }      /* END if (TC != OldTC) */
    }        /* END while(TRUE)      */

  if (DRL == NULL)
    {
    memcpy(ARL,C,sizeof(mrange_t) * (cindex + 1));
    }

  return(SUCCESS);
  }  /* END MRLGetMinTC() */





/**
 * Convert availability range to start range.
 *
 * @param MinDuration (I)
 * @param ARL (I)
 * @param SRL (O) [overwritten]
 */

int MRLSFromA(

  long      MinDuration,  /* I */
  mrange_t *ARL,          /* I */
  mrange_t *SRL)          /* O (overwritten) */

  {
  int  aindex;
  int  sindex;

  int  rindex;

  long RangeDuration;

  int TC;
  int NC;

  /* NOTE:  assumes intra-node ranges */

  const char *FName = "MRLSFromA";

  MDB(6,fSCHED) MLog("%s(%ld,ARL,SRL)\n",
    FName,
    MinDuration);

  if ((ARL == NULL) || (SRL == NULL))
    {
    return(FAILURE);
    }

  sindex = 0;

  /* NOTE:  only intelligently processes two adjacent ranges */

  for (aindex = 0;ARL[aindex].ETime > 0;aindex++)
    {
    /* NOTE:  must take into account connected ranges */

    RangeDuration = ARL[aindex].ETime - ARL[aindex].STime;

    if (ARL[aindex].ETime >= MCONST_RSVINF)
      {
      /* Infinite duration */

      RangeDuration = MMAX_TIME;
      }

    TC = ARL[aindex].TC;
    NC = ARL[aindex].NC;

    /* NOTE:  even if range is large enough, if range connects with subsequent range,
              legal start time timeframe can be extended */

    /* NOTE:

      Req Duration:  RRR

          Avail Range   --->  Start Range
              YYYYYYY             YYYY
          XXXXYYYYYYY         XXXXYYYY

      Req Duration:  RRR

          Avail Range   --->  Start Range
          XXXX                X
          XXXXYYYYYYY         XYYYYYYY
    */

    if (RangeDuration < MinDuration)
      {
      /* NOTE:  should only merge ranges shorter than MinDuration */

      for (rindex = aindex + 1;ARL[rindex].ETime != 0;rindex++)
        {
        if (ARL[rindex - 1].ETime != ARL[rindex].STime)
          break;

        /* merge connected ranges */

        RangeDuration += ARL[rindex].ETime - ARL[rindex].STime;
        TC = MIN(TC,ARL[rindex].TC);
        NC = MIN(NC,ARL[rindex].NC);

        if (RangeDuration > (long)(ARL[aindex].ETime - ARL[aindex].STime + MinDuration))
          break;
        }  /* END for (rindex) */
      }    /* END if (RangeDuration < MinDuration) */
    else
      {
      if ((ARL[aindex].ETime != 0) &&
          ((ARL[aindex + 1].ETime != 0) &&
           (ARL[aindex + 1].STime == ARL[aindex].ETime)))
        {
        /* allow current range to utilize resources of next adjacent range increasing effective range duration */

        if (ARL[aindex + 1].TC >= ARL[aindex].TC)
          {
          RangeDuration += ARL[aindex].ETime - ARL[aindex].STime;
          }
        }
      }

    if (RangeDuration >= MinDuration)
      {
      if ((sindex > 0) &&
          (SRL[sindex - 1].TC >= TC) &&
          (ARL[aindex - 1].ETime == ARL[aindex].STime) &&
          (SRL[sindex - 1].ETime < ARL[aindex].STime))
        {
        /* slide current range backwards to reconnect to previously adjacent range */

        SRL[sindex].STime = SRL[sindex - 1].ETime;
        }
      else
        {
        SRL[sindex].STime = ARL[aindex].STime;
        }

      SRL[sindex].ETime = MIN(
        ARL[aindex].ETime,
        ARL[aindex].STime + RangeDuration - MinDuration);

      if ((ARL[aindex].ETime >= MCONST_RSVINF) &&
          (MinDuration >= MCONST_RSVINF))
        {
        /* Infinite walltime, above calculation won't work */

        SRL[sindex].ETime = ARL[aindex].ETime;
        }

      SRL[sindex].TC = TC;
      SRL[sindex].NC = MAX(1,NC);

      sindex++;
      }
    }    /* END for (aindex) */

  SRL[sindex].ETime = 0;

  if ((SRL[0].ETime == 0) || (SRL[0].TC == 0))
    {
    if (SRL[0].ETime != 0)
      {
      MDB(0,fSCHED) MLog("ALERT:    corrupt range found in %s (initial range empty)\n",
        FName);

      for (aindex = 0;ARL[aindex].ETime > 0;aindex++)
        {
        MDB(2,fSCHED) MLog("INFO:     ARL[%d]  %ld - %ld  %d  %d\n",
          aindex,
          ARL[aindex].STime,
          ARL[aindex].ETime,
          ARL[aindex].TC,
          ARL[aindex].NC);
        }  /* END for (aindex) */
      }

    return(FAILURE);
    }  /* END if ((SRL[0].ETime == 0) || (SRL[0].TC == 0)) */

  return(SUCCESS);
  }  /* MRLSFromA() */




/**
 *
 *
 * @param RL (I) [modified]
 * @param MinDuration (I) [set to 0 to ignore constraint]
 * @param MinTasks (I) [set to 0 to ignore constraint]
 */

int MRLLinkTime(

  mrange_t *RL,          /* I (modified) */
  mulong    MinDuration, /* I (set to 0 to ignore constraint) */
  int       MinTasks)    /* I (set to 0 to ignore constraint) */

  {
  int index;
  int index2;

  int RCount;

  if (RL == NULL)
    {
    return(FAILURE);
    }

  RCount = 0;

  for (index = 0;RL[index].ETime != 0;index++)
    {
    if (index != RCount)
      memcpy(&RL[RCount],&RL[index],sizeof(RL[index]));

    /* merge ranges if immediate */

    for (index2 = index + 1;RL[index2].ETime != 0;index2++)
      {
      if (RL[RCount].ETime != RL[index2].STime)
        break;

      if (RL[index2].TC < MinTasks)
        break;

      RL[RCount].ETime = RL[index2].ETime;
      RL[RCount].TC    = MIN(RL[RCount].TC,RL[index2].TC);
      RL[RCount].NC    = MIN(RL[RCount].NC,RL[index2].NC);
      }  /* END for (index2) */

    /* skip merged ranges */

    index = index2 - 1;

    RCount++;
    }  /* END for (index = 0;RL[index].ETime != 0;index++) */

  RL[RCount].ETime = 0;

  if ((RL[0].ETime == 0) || (RL[0].TC == 0))
    {
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MRLLinkTime() */



/**
 * Helper function to calculate NextRangeDuration for MRLMergeTime.
 *
 * @param RL          [I] - The range list
 * @param IsBFJob     [I] - TRUE if job is a backfill job
 * @param SeekLong    [I] - TRUE if seeking the longest possible range
 * @param MinDuration [I] - Minimum range duration
 * @param StartIndex  [I] - Index to be looked at first in RL
 */

long __GetNextRangeDuration(

  mrange_t *RL,
  mbool_t   IsBFJob,
  mbool_t   SeekLong,
  long      MinDuration,
  int       StartIndex)

  {
  long NextRangeDuration = 0;
  int  index2 = 0;

  for (index2 = StartIndex + 1;RL[index2].ETime != 0;index2++)
    {
    if (RL[index2 - 1].ETime != RL[index2].STime)
      break;

    NextRangeDuration += (RL[index2].ETime - RL[index2].STime);

    if (RL[index2].ETime == MCONST_RSVINF)
      {
      NextRangeDuration = MCONST_RSVINF;

      break;
      }

    /* NOTE: could this be a problem here that we are breaking because TC is
        too small, but we've already added the time? (jgardner) */

    if ((IsBFJob == TRUE) && (SeekLong == FALSE))
      {
      if (RL[index2].TC < RL[0].TC)
        {
        /* for backfill jobs, do not allow range[0] TC to be reduced */

        break;
        }
      }

    if ((MinDuration != -1) && (NextRangeDuration >= MinDuration))
      break;
    }  /* END for (index2) */

  return(NextRangeDuration);
  }  /* END __GetNextRangeDuration() */


/**
 * Helper function to tell MRLMergeTime if it should should merge ranges or not.
 *
 * @param SeekLong    [I] - 
 * @param IsBFJob     [I] - TRUE if job is a backfill job
 * @param PRDuration  [I] - Previous range duration 
 * @param NRDuration  [I] - Next range duration 
 * @param CRDuration  [I] - Current range duration
 * @param MinDuration [I] - 
 */

mbool_t __GetMergeRange(

  mbool_t SeekLong,
  mbool_t IsBFJob,
  long    PRDuration,
  long    NRDuration,
  long    CRDuration,
  long    MinDuration)

  {
  mbool_t MergeRange = FALSE;

  /* does adjacent previous range exist */

  if (((SeekLong == TRUE) || (IsBFJob == TRUE)) && (PRDuration > 0))
    {
    /* if SeekLong/IsBFJob is TRUE, always pursue longest range */

    /* NOTE:  previous IsBFJob checks verify merging will optimize
              range 0 size */

    MergeRange = TRUE;
    }
  else if ((PRDuration > 0) &&
          ((MinDuration == -1) || (PRDuration < MinDuration)))
    {
    /* PR is too short, merge CR to PR */

    MergeRange = TRUE;
    }
  else if ((NRDuration > 0) && (NRDuration < MinDuration))
    {
    /* NR is too short, start new range (merge w/NR) */

    }
  else if ((CRDuration < MinDuration) && (PRDuration > 0))
    {
    /* CR is too short and PR exists, merge CR to PR */

    MergeRange = TRUE;
    }

  /* All other conditions are FALSE */

  return(MergeRange);
  }  /* END __GetMergeRange() */


/**
 * Helper function to do the range merges in MRLMergeTime.
 *
 * NOTE:  no error checking
 *
 * @param RL
 * @param RRange
 * @param PreviousRangeIndexP
 * @param LastRangeTCP
 * @param CurrentRangeDurationP
 * @param SeekLong
 * @param IsBFJob
 * @param MinDuration
 * @param MinTasks
 */

int __MRLMergeTimeMergeRanges(

  mrange_t *RL,
  mrange_t *RRange,
  int      *PreviousRangeIndexP,
  int      *LastRangeTCP,
  long     *CurrentRangeDurationP,
  mbool_t   SeekLong,
  mbool_t   IsBFJob,
  long      MinDuration,
  int       MinTasks)

  {
  int index = 0;
  long CurrentRangeDuration = 0;   /* duration of current range */
  long PreviousRangeDuration = 0;  /* duration of previous range */
  long NextRangeDuration = 0;      /* duration of next range */

  mbool_t MergeRange;
  int PreviousRangeIndex;
  int LastRangeTC;

  PreviousRangeIndex = *PreviousRangeIndexP;
  LastRangeTC = *LastRangeTCP;

  for (index = 0;RL[index].ETime != 0;index++)
    {
    MDB(7,fSTRUCT) MLog("INFO:     eval RL[%d] %ld -> %ld T: %d N:%d\n",
      index,
      RL[index].STime - MSched.Time,
      RL[index].ETime - MSched.Time,
      RL[index].TC,
      RL[index].NC);

    if (RL[index].TC < MinTasks)
      {
      continue;
      }

    LastRangeTC = RL[index].TC;

    CurrentRangeDuration = RL[index].ETime - RL[index].STime;

    NextRangeDuration = __GetNextRangeDuration(RL,IsBFJob,SeekLong,MinDuration,index);

    if ((PreviousRangeIndex > -1) && (RL[PreviousRangeIndex].ETime == RL[index].STime))
      {
      PreviousRangeDuration = RL[PreviousRangeIndex].ETime - RL[PreviousRangeIndex].STime;
      }
    else
      {
      /* ranges are disconnected */

      PreviousRangeDuration = 0;
      }

    MergeRange = __GetMergeRange(
                    SeekLong,
                    IsBFJob,
                    PreviousRangeDuration,
                    NextRangeDuration,
                    CurrentRangeDuration,
                    MinDuration);

    if (MinDuration > 0)
      {
      if ((long)(PreviousRangeDuration + CurrentRangeDuration + NextRangeDuration) < MinDuration)
        {
        if ((MinDuration >= MCONST_RSVINF) &&
            (RL[index].ETime >= MCONST_RSVINF))
          {
          /* Dealing with an infinite duration and we found an infinite duration */

          /* NO-OP */
          }
        else
          {
          MDB(6,fSCHED) MLog("INFO:     RL[%d] too short for request (RD: %ld < M: %ld):  removing range\n",
            index,
            PreviousRangeDuration + CurrentRangeDuration + NextRangeDuration,
            MinDuration);

          continue;
          }
        }
      }  /* END if (MinDuration > 0) */

    if (IsBFJob != TRUE)
      {
      /* check/enforce range bounds (but not for backfill jobs) */

      if (RL[index].ETime > RRange[0].ETime)
        {
        /* truncate range to fit within requested bounds */

        MDB(6,fSCHED) MLog("INFO:     RL[%d] too late for request (E: %ld > E: %ld  P: %ld): truncating range\n",
          index,
          RL[index].ETime,
          RRange[0].ETime,
          MSched.Time);

        CurrentRangeDuration = RL[index].ETime - RL[index].STime;

        RL[index].ETime = RRange[0].ETime;
        }
      }

    if (MergeRange == TRUE)
      {
      RL[PreviousRangeIndex].ETime = RL[index].ETime;
      RL[PreviousRangeIndex].TC = MIN(RL[PreviousRangeIndex].TC,RL[index].TC);

      CurrentRangeDuration = RL[PreviousRangeIndex].ETime - RL[PreviousRangeIndex].STime;

      continue;
      }

    /* request to start new range, verify range is valid */

    /* NOTE: mintime and mintask checks performed previously */

    if (RL[index].STime > RRange[0].ETime)
      {
      MDB(6,fSCHED) MLog("INFO:     RL[%d] (%ld -> %ld)x%d too late for request by %ld seconds\n",
        index,
        RL[index].STime,
        RL[index].ETime,
        RL[index].TC,
        RL[index].STime - RRange[0].ETime);

      continue;
      }

    /* start new range */

    PreviousRangeIndex++;

    if (index != PreviousRangeIndex)
      {
      /* assign instead of memset to avoid purify errors */

      RL[PreviousRangeIndex].STime = RL[index].STime;
      RL[PreviousRangeIndex].ETime = RL[index].ETime;
      RL[PreviousRangeIndex].TC    = RL[index].TC;
      RL[PreviousRangeIndex].NC    = RL[index].NC;
      RL[PreviousRangeIndex].Aff   = RL[index].Aff;
      }
    }    /* END for (index) */

  *PreviousRangeIndexP   = PreviousRangeIndex;
  *CurrentRangeDurationP = CurrentRangeDuration;
  *LastRangeTCP          = LastRangeTC;

  return(SUCCESS);
  }  /* END __MRLMergeTimeMergeRanges() */
  


/**
 *
 * NOTE:  if MinDuration > 0 and MinTasks == 0, and if Range[0] is too short,
 *        MRLMergeTime() may delay Range[1] so as to allow Range[0] to be
 *        adequately long
 *
 * NOTE: In the case above, this can cause issues with backfill. FSU was seeing this in RT5510.
 *       Scenario: 2 nodes 2 procs each. Fill up the active queue with low-priority jobs that have jobs finishing 
 *       every 30 seconds. Then fill up the eligible queue with 2 minute jobs. Then submit one high-priority 
 *       15 minute job that requests all the procs. The high priority job will get a reservation ~15 minutes
 *       out, even though the job could run in ~2 minutes. In this case all of the low priority jobs 
 *       will backfill before the high priority job and continue to push back, starve, the high-priority 
 *       job. The solution to this problem is to add RSVRESEARCHALGO WIDE to moab.cfg, which sets MinDuration
 *       to 0, when called in MJobGetSNRange, which won't push the job's available start time out.
 *
 * @see MJobGetSNRange() - parent
 *
 * @param RL (I) [minsize=MMAX_RANGE,modified]
 * @param RRange (I)
 * @param SeekLong (I)
 * @param MinDuration (I) [set to 0 to ignore constraint,set to -1 to merge at every opportunity]
 * @param MinTasks (I) [set to 0 to ignore constraint]
 */

int MRLMergeTime(

  mrange_t *RL,          /* I (minsize=MMAX_RANGE,modified) */
  mrange_t *RRange,      /* I */
  mbool_t   SeekLong,    /* I */
  long      MinDuration, /* I (set to 0 to ignore constraint,set to -1 to merge at every opportunity) */
  int       MinTasks)    /* I (set to 0 to ignore constraint) */

  {
  int PreviousRangeIndex = -1;

  long CurrentRangeDuration = 0;   /* duration of current range */
  long PreviousRangeDuration = 0;  /* duration of previous range */

  int  LastRangeTC = -1;   /* last range taskcount */

  mbool_t IsBFJob = FALSE;

  const char *FName = "MRLMergeTime";

  MDB(5,fSTRUCT) MLog("%s(RL,RRange,%s,%ld,%d)\n",
    FName,
    MBool[SeekLong],
    MinDuration,
    MinTasks);

  if ((RL == NULL) || (RRange == NULL))
    {
    return(FAILURE);
    }

  if (RRange[0].ETime == RRange[0].STime)
    {
    IsBFJob = TRUE;

    /* if BFJob is true the desired behavior is too always maximize range 0
       even at expense of other ranges, if SeekLong is TRUE, get the longest
       possible range 0, if SeekLong is FALSE, get the longest possible range
       which does not reduce available taskcount */

    MinDuration = 0;
    }

  /* if ranges are adjacent and previous range is too small or current range
     is too small, merge */

  /* 1) if PreviousRange is too short, merge CurrentRange to PreviousRange */
  /* 2) if NextRange is too short, start new range (merge w/NextRange) */
  /* 3) if CurrentRange is too short and PreviousRange exists, merge CurrentRange to PreviousRange */
  /* 4) if CurrentRange is too short and NextRange exists, start new range (merge w/NextRange) */
  /* 5) if CurrentRange is long enough, start new range */

  __MRLMergeTimeMergeRanges(
      RL,
      RRange,
      &PreviousRangeIndex,
      &LastRangeTC,
      &CurrentRangeDuration,
      SeekLong,
      IsBFJob,
      MinDuration,
      MinTasks);

  if (MinDuration > 0)
    {
    /* CurrentRangeDuration is duration of this range plus connected previous ranges */

    if (CurrentRangeDuration < MinDuration)
      {
      if ((MinDuration >= MCONST_RSVINF) &&
          (RL[PreviousRangeIndex].ETime >= MCONST_RSVINF))
        {
        /* Dealing with infinite walltimes */

        /* NO-OP */
        }
      else if (PreviousRangeDuration > 0)
        {
        /* final range is invalid - remove previous range if connected */

        RL[PreviousRangeIndex].ETime = 0;
        }
      }
    }

  /* NOTE:  SeekLong should always be set for backfill jobs */

  if ((PreviousRangeIndex != -1) && (LastRangeTC != -1) && (PreviousRangeIndex < MMAX_RANGE) && (MinDuration != -1))
    {
    /* if last range is longer than minduration and can be expanded in the future,
         break last range in two and expand taskcount in second part */

    /* PreviousRangeIndex is current last range */

    if ((LastRangeTC > RL[PreviousRangeIndex].TC) &&
        ((long)(RL[PreviousRangeIndex].ETime - RL[PreviousRangeIndex].STime) > MinDuration))
      {
      /* dup last range */

      memcpy(&RL[PreviousRangeIndex + 1],&RL[PreviousRangeIndex],sizeof(RL[0]));

      if ((MinDuration > 0) && (MinDuration < MCONST_RSVINF))
        {
        /* if duration specified, give them requested duration for next to last range */
        /* final range will be expanded to max task count */

        RL[PreviousRangeIndex].ETime = (mulong)(RL[PreviousRangeIndex].STime + MinDuration);
        }
      else if (IsBFJob == FALSE)
        {
        /* attempt to maximize size of max taskcount range */

        /* minduration must be 0, ie, not specified */
        /* NOTE:  backfill does not specify min duration */

        /* NOTE:  for backfill, only first range matters, maintain maximum length */

        /* NO-OP */

        RL[PreviousRangeIndex].ETime = RL[PreviousRangeIndex].STime + MCONST_HOURLEN;
        }

      /* create new 'last' range with all available tasks */

      RL[PreviousRangeIndex + 1].STime = RL[PreviousRangeIndex].ETime;
      RL[PreviousRangeIndex + 1].TC = LastRangeTC;

      PreviousRangeIndex++;
      }
    }    /* END if ((PreviousRangeIndex != -1) && (LastRangeTC != -1) && ...) */

  /* terminate range list */

  RL[PreviousRangeIndex + 1].ETime = 0;

  if ((RL[0].ETime == 0) || (RL[0].TC == 0))
    {
    return(FAILURE);
    }

  MDB(6,fSTRUCT) MLog("INFO:     range[0] found %ld -> %ld T: %d N:%d\n",
    RL[0].STime - MSched.Time,
    RL[0].ETime - MSched.Time,
    RL[0].TC,
    RL[0].NC);

  return(SUCCESS);
  }  /* END MRLMergeTime() */




/**
 *
 *
 * @param R (I/O)
 * @param CollapseSmallInstants (I)
 * @param MaxTC (I) [maximum allowed range taskcount]
 */

int MRLReduce(

  mrange_t *R,         /* I/O */
  mbool_t   CollapseSmallInstants, /* I */
  int       MaxTC)     /* I (maximum allowed range taskcount) */

  {
  int rindex;
  int RIndex;

  static mrange_t tmpR[MMAX_RANGE + 1];

  if (R == NULL)
    {
    return(FAILURE);
    }

  memcpy(tmpR,R,sizeof(tmpR));

  RIndex = 0;

  for (rindex = 0;rindex < MMAX_RANGE;rindex++)
    {
    if (tmpR[rindex].ETime == 0)
      break;

    if (CollapseSmallInstants == TRUE)
      {
      if ((tmpR[rindex + 1].ETime != 0) &&                     /* not end of list */
          (tmpR[rindex].STime == tmpR[rindex].ETime) &&        /* range is instant */
          (tmpR[rindex + 1].STime == tmpR[rindex].ETime) &&    /* ranges touch */
          (MIN(MaxTC,tmpR[rindex].TC) <= MIN(MaxTC,tmpR[rindex + 1].TC))) /* first range is smaller */
        {
        /* instant range located which is smaller than bounding subsequent range */

        /* do not copy list to output range */

        continue;
        }

      if ((tmpR[rindex + 1].ETime != 0) &&                     /* not end of list */
          (tmpR[rindex +1].STime == tmpR[rindex].ETime)&&      /* ranges touch */
          (MIN(MaxTC,tmpR[rindex].TC) <= MIN(MaxTC,tmpR[rindex + 1].TC))) /* first range is smaller */
        {
        /* merge ranges */

        R[RIndex].ETime = tmpR[rindex + 1].ETime;

        R[RIndex].TC = MIN(tmpR[rindex + 1].TC,R[RIndex].TC);

        continue;
        }
      }

    if (R[RIndex].STime > tmpR[rindex].STime)
      {
      memcpy(&R[RIndex],&tmpR[rindex],sizeof(tmpR[0]));

      R[RIndex].TC = MIN(MaxTC,R[RIndex].TC);

      RIndex++;
      }
    else
      {
      RIndex++;

      R[RIndex].STime = tmpR[rindex + 1].STime;
      R[RIndex].ETime = tmpR[rindex + 1].ETime;
      }
    }  /* END for (rindex) */

  /* terminate list */

  R[MIN(RIndex,MMAX_RANGE - 1)].ETime = 0;

  return(SUCCESS);
  }  /* END MRLReduce() */



/**
 *
 *
 * @param A
 * @param B
 */

int MLongComp(

  long *A,
  long *B)

  {
  if (*A < *B)
    return(-1);

  return(1);
  }




/**
 * Return an array of 'time' longs, harvested from the ARange array
 * and sorted
 *
 * NOTE: this routine may fill up ETime with lots of ranges
 *       the calling routine (MClusterShowARes) should be smart
 *       enough to stop when it has what it wants
 *
 * @param ARange  (I)   [2D input array of 'ranges', [ACount][MMAX_RANGE]]
 * @param ACount  (I)   [count of items in ARange, and returned in ETime]
 * @param ETime   (O)   [array, not pointer]
 *
 * global:  MRE[] is used
 */

int MRLGetEList(

  mrange_t   ARange[][MMAX_RANGE],  /* I */
  int        ACount,                /* I */
  long      *ETime)                 /* O (array, not pointer) */

  {
  rsv_iter RTI;

  mrsv_t *R;

  int aindex;
  int rindex;
  int eindex;

  long SETime[MMAX_RANGE+1];

  SETime[0] = 0;

  eindex = 0;

  /* Populate the temp SETime array from the input array */

  for (aindex = 0;aindex < ACount;aindex++)
    {
    for (rindex = 0;rindex < MMAX_RANGE;rindex++)
      {
      if ((ARange[aindex][rindex].ETime == 0) ||
          (ARange[aindex][rindex].STime == 0))
        break;

      SETime[eindex++] = ARange[aindex][rindex].STime;
      }
    }

  if ((eindex > 0) && (MRsvHT.size() > 0))
    eindex--;

  /* just walk all rsvs and get all times then sort - NYI */

  MRsvIterInit(&RTI);

  while (MRsvTableIterate(&RTI,&R) == SUCCESS)
    {
    if (eindex >= MMAX_RANGE)
      break;

    if (R->StartTime >= MSched.Time)
      {
      eindex++;

      SETime[eindex] = R->StartTime;
      }

    if (eindex >= MMAX_RANGE)
      break;

    eindex++;

    SETime[eindex] = R->EndTime;
    }  /* END while (MRsvTableIterate()) */

  SETime[eindex] = 0;

  /* Now sort the temp array of ETimes */

  qsort((void *)&SETime[0],
    eindex,
    sizeof(long),
    (int(*)(const void *,const void *))MLongComp);

  /* Store the temp array back to the dest array */

  ETime[0] = SETime[0];

  eindex = 0;
  ETime[1] = -1;
  eindex++;

  for (rindex = 1;rindex < MMAX_RANGE;rindex++)
    {
    if (SETime[rindex] == 0)
      break;

    if (ETime[eindex] == SETime[rindex])
      {
      continue;
      }

    ETime[eindex] = SETime[rindex];
    ETime[eindex + 1] = -1;

    eindex++;
    }

  return(SUCCESS);
  }  /* END MRLGetEList() */




/**
 * Fill RIndex with 'main' indexes into range array where ETime is valid.
 *
 * Accept an array of arrays of mrange_t instances,
 * walk the arrays finding valid 
 *
 * @param ARange
 * @param ACount    (I) (minsize=[ACount][MMAX_RANGE] 
 * @param ETime     (I)
 * @param RIndex    (O) [minsize=[ACount]]
 */

int MRLGetIndexArray(

  mrange_t   ARange[][MMAX_RANGE], 
  int        ACount,
  mulong     ETime,
  int       *RIndex)

  {
  int aindex;
  int rindex;

  if ((ARange == NULL) || (RIndex == NULL))
   {
   return(FAILURE);
   }

  /* clear the return array */
  memset(RIndex,0,sizeof(int) * ACount);

  /* Iterate over the 2-d array */

  for (aindex = 0;aindex < ACount;aindex++)
    {
    for (rindex = 0;rindex < MMAX_RANGE;rindex++)
      {
      /* End of list check */
      if (ARange[aindex][rindex].ETime == 0)
        break;

      /* Check for a valid "hit" */
      if ((ETime >= ARange[aindex][rindex].STime) &&
          (ETime < ARange[aindex][rindex].ETime))
        {
        /* This "row" has a hit, so record it and go to next row */
        RIndex[aindex] = rindex;

        break;
        }
      }
    }

  return(SUCCESS);
  }  /* END MRLGetIndexArray() */
/* END MRange.c */
