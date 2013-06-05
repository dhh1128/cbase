/* HEADER */

/**
 * @file MLocal.c
 *
 * Moab Local
 */
        
/* Contains:                                               *
 *                                                         *
 *  int MLocalJobInit(J)                                   *
 *  int MLocalJobPostLoad(J)                               *
 *  int MLocalNodeInit(N)                                  *
 *  int MLocalJobStart(J)                                  *
 *  int MLocalJobCheckNRes(J,N,Time)                       *
 *  int MLocalCheckFairnessPolicy(J,StartTime,Message)     *
 *  int MLocalGetNodePriority(J,N)                         *
 *  int MLocalInitialize()                                 *
 *  int MLocalJobAllocateResources(J,RQ,NodeList,RQIndex,...) *
 *  int MLocalJobDistributeTasks(J,R,NodeList,TaskMap)     *
 *  int MLocalQueueScheduleIJobs(J)                        *
 *  int MLocalAdjustActiveJobStats(J)                      *
 *  double MLocalApplyCosts(Duration,TC,Env)               *
 *                                                         */

  
#include "moab.h"
#include "moab-proto.h"  
#include "moab-global.h"  
#include "moab-const.h"  

#ifdef MDYNAMIC
#include <dlfcn.h>
#endif  /* MDYNAMIC */




/* #include "../../contrib/checkreq/AussieJobCheckNRes.c" */


/**
 *
 *
 * @param J (I)
 * @param N (I)
 * @param StartTime (I)
 */

int MLocalJobCheckNRes(

  const mjob_t  *J,
  const mnode_t *N,
  long           StartTime)

  {
  int rc = SUCCESS;

  const char *FName = "MLocalJobCheckNRes";

  MDB(8,fSCHED) MLog("%s(%s,%s,%ld)\n",
    FName,
    J->Name,
    N->Name,
    StartTime);

/*
  rc = ContribAussieJobCheckNRes(J,N,StartTime);
*/

  return(rc);
  }  /* END MLocalJobCheckNRes */




/* #include "../../contrib/jobinit/AussieJobInit.c" */

/**
 *
 *
 * @param J (I)
 */

int MLocalJobInit(

  mjob_t *J)  /* I */

  {
/*
  ContribAussieJobInit(J); 
*/

  return(SUCCESS);
  }  /* END MLocalJobInitialize() */





/**
 *
 *
 * @param J (I)
 */

int MLocalJobPostLoad(

  mjob_t *J)  /* I */

  {
  /* NYI */

  return(SUCCESS);
  }  /* END MLocalJobPostLoad() */





/**
 *
 *
 * @param N (I) [modified]
 */

int MLocalNodeInit(

  mnode_t *N)  /* I (modified) */

  {
  /* NO-OP */

  return(SUCCESS);
  }  /* END MLocalNodeInit() */




/* #include "../../contrib/fairness/JobLength.c" */
/* #include "../../contrib/fairness/ASCBackgroundJobPolicy.c" */

/**
 *
 *
 * @param J
 * @param StartTime
 * @param Message
 */

int MLocalCheckFairnessPolicy(

  mjob_t *J,
  long    StartTime,
  char   *Message)

  {
  const char *FName = "MLocalCheckFairnessPolicy";

  MDB(6,fSCHED) MLog("%s(%s,%ld,Message)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (unsigned long)StartTime);

/*
  if (ContribJobLengthFairnessPolicy(J,StartTime,Message) == FAILURE)
    {
    return(FAILURE);
    }
*/

/*
  if (ContribASCBackgroundJobPolicy(J,StartTime,Message) == FAILURE)
    {
    return(FAILURE);
    }
*/

  /* all local policies passed */

  return(SUCCESS);
  }  /* END MLocalFairnessPolicy() */





/* #include "../../contrib/nodeallocation/PNNLGetNodePriority.c" */

/**
 *
 *
 * @param N (I)
 * @param J (I) [optional]
 * @param Prio (O)
 */

int MLocalNodeGetPriority(

  mnode_t       *N,
  const mjob_t  *J,
  double        *Prio)

  {
  double Priority = 0.0;

/*
  Priority = ContribPNNLGetNodePriority(J,N));
*/

  *Prio = Priority;

  return(SUCCESS);
  }  /* END MLocalGetNodePriority() */





/* #include "../../contrib/nodeallocation/OSCProximityNodeAlloc.c" */

/* #include "../../contrib/nodeallocation/CASTORGetNodePriority.c" */
/* #include "../../contrib/nodeallocation/CASTORJobAllocateResources.c" */
 
/**
 *
 *
 * @param J             (I) job allocating nodes
 * @param RQ            (I) req allocating nodes
 * @param NodeList      (I) eligible nodes
 * @param RQIndex       (I) index of job req to evaluate
 * @param MinTPN        (I) min tasks per node allowed
 * @param MaxTPN        (I) max tasks per node allowed
 * @param NodeMap       (I) array of node alloc states
 * @param AffinityLevel (I) current reservation affinity level to evaluate
 * @param NodeIndex     (I/O) index of next node to find in BestList
 * @param BestList      (I/O) list of selected nodes
 * @param TaskCount     (I/O) total tasks allocated to job req
 * @param NodeCount     (I/O) total nodes allocated to job req
 * @param StartTime     (I)
 */

int MLocalJobAllocateResources(
 
  mjob_t *J,
  mreq_t *RQ,
  mnl_t  *NodeList,
  int     RQIndex,
  int     MinTPN[],
  int     MaxTPN[],
  char    NodeMap[],
  int     AffinityLevel,
  int     NodeIndex[],
  mnl_t **BestList,
  int     TaskCount[],
  int     NodeCount[],
  mulong  StartTime)
 
  {
  int rc;

  rc = SUCCESS;

/*
  rc = ContribOSCProximityNodeAlloc(
        J,
        RQ,
        NodeList,
        RQIndex,
        MinTPN,
        MaxTPN,
        NodeMap,
        AffinityLevel,
        NodeIndex,
        BestList,
        TaskCount,
        NodeCount);
*/

/* 
  rc = ContribCASTORJobAllocateResources(
        J,
        RQ,
        NodeList,
        RQIndex,
        MinTPN,
        MaxTPN,
        NodeMap,
        AffinityLevel,
        NodeIndex,
        BestList,
        TaskCount,
        NodeCount);
*/

  if (rc == FAILURE)
    {
    return(FAILURE);
    }
 
  return(FAILURE);  
  }  /* END MLocalJobAllocateResources() */




/* #include "../../contrib/appsim/SCHSMSim.c" */  

int MLocalInitialize()

  {
/*
  if (ContribSCHSMSimInitialize() == FAILURE)
    {
    return(FAILURE);
    }
*/

  return(SUCCESS);
  }  /* END MLocalInitialize() */





/**
 *
 *
 * @param J
 * @param R
 * @param NodeList (I)  nodelist with taskcount information
 * @param TaskMap (O)  task distribution list
 * @param TaskMapSize 
 */

int MLocalJobDistributeTasks(
 
  mjob_t    *J,
  mrm_t     *R,
  mnl_t     *NodeList,
  int       *TaskMap,
  int        TaskMapSize)
 
  {
  return(SUCCESS);
  }  /* END MLocalJobDistributeTasks() */






/* #include "../../contrib/sched/CASTORReqCheckResourceMatch.c" */

/**
 *
 *
 * @param J (I) [optional]
 * @param RQ (I)
 * @param N (I)
 * @param RIndex (O) [optional]
 */

int MLocalReqCheckResourceMatch(

  const mjob_t  *J,
  const mreq_t  *RQ,
  const mnode_t *N,
  enum MAllocRejEnum *RIndex)

  {
  int rc;

  if (RIndex != NULL)
    *RIndex = marNONE;

  if ((RQ == NULL) || (N == NULL))
    {
    return(FAILURE);
    }

  /* if node 'N' cannot feasibly support job req 'RQ' at ANY time, return failure */

  rc = SUCCESS;

  /* 
  rc = ContribCASTORReqCheckResourceMatch(J,RQ,N,RIndex);
  */

  if (rc == FAILURE)
    {
    return(FAILURE);
    }
 
  return(SUCCESS);
  }  /* END MLocalReqCheckResourceMatch() */




/**
 *
 *
 * @param J (I)
 */

int MLocalAdjustActiveJobStats(

  mjob_t *J)  /* I */

  {
  if (J == NULL)
    {
    return(FAILURE);
    }

  /* NOTE:  J->NodeList lists all nodes allocated to job */

  /* NYI */

  return(SUCCESS);
  }  /* END MLocalAdjustActiveJobStats() */





/* #include "../../contrib/jobpoststart/CASTORJobPostStart.c" */

/**
 *
 *
 * @param J (I)
 */

int MLocalJobPostStart(

  mjob_t *J)  /* I */

  {
  int rc;

  if (J == NULL)
    {
    return(FAILURE);
    }

  rc = SUCCESS;

  /* 
  rc = ContribCASTORJobPostStart(J);
  */

  if (rc == FAILURE)
    {
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MLocalJobPostStart() */




/**
 *
 *
 * @param J (I)
 */

int MLocalJobPreStart(

  mjob_t *J)  /* I */

  {
  if (J == NULL)
    {
    return(FAILURE);
    }

  /* NO-OP */

  return(SUCCESS);
  }  /* END MLocalJobPreStart() */





/**
 * Apply custom costs for services and other request-specific attributes.
 *
 * @see MLocalRsvGetAllocCost() - peer
 *
 * @param Duration (I)
 * @param TC (I)
 * @param VarList (I)
 */

double MLocalApplyCosts(

  mulong  Duration,  /* I */
  int     TC,        /* I */
  char   *VarList)   /* I */

  {
  char tmpLine[MMAX_LINE];

  char *ptr;
  char *TokPtr;

  int   aindex;
  int   tmpI;

  double Cost;

  char   ValLine[MMAX_LINE];

  const char *VName[] = {
    NONE,
    "collection",
    "querytype",
    NULL };

  enum {
    mlvlNONE = 0,
    mlvlCollection,
    mlvlQueryType };

  if ((Duration == 0) || (TC == 0) || (VarList == NULL) || (VarList[0] == '\0'))
    {
    return(0.0);
    }

  Cost = 0.0;

  MUStrCpy(tmpLine,VarList,sizeof(tmpLine));

  /* FORMAT:  <ATTR>=<VAL>[,<ATTR>=<VAL>]... */

  ptr = MUStrTok(tmpLine,", \t\n",&TokPtr);

  while (ptr != NULL)
    {
    if (MUGetPair(
          ptr,      /* I */
          (const char **)VName,
          &aindex,  /* O (attribute) */
          NULL,
          FALSE,
          &tmpI,    /* O (relative comparison) */
          ValLine,  /* O (value) */
          sizeof(ValLine)) == FAILURE)
      {
      /* cannot parse value pair */

      MDB(2,fSCHED) MLog("ALERT:    cannot parse env variable '%s'\n",
        ptr);

      ptr = MUStrTok(NULL," \t\n",&TokPtr);

      continue;
      }

    switch (aindex)
      {
      case mlvlCollection:

        if (!strcasecmp(ValLine,"master"))
          Cost += 5000;
        else if (!strcasecmp(ValLine,"basic"))
          Cost += 1000;

        break;

      case mlvlQueryType:

        if (!strcasecmp(ValLine,"full"))
          Cost += 2000;
        else if (!strcasecmp(ValLine,"basic"))
          Cost += 400;

        break;

      default:

        /* ignore variable */

        /* NO-OP */

        break;
      }  /* END switch (aindex) */

    ptr = MUStrTok(NULL," \t\n",&TokPtr);
    }  /* END while (ptr != NULL) */

  return(Cost);
  }  /* END MLocalApplyCosts() */






/**
 * @see MLocalApplyCosts() - peer
 * @see MRsvGetCost() - parent
 * @see MRsvGetAllocCost() - parent
 *
 * @param R (I)
 * @param V (I) [optional]
 * @param RCostP (O)
 * @param IsPartial (I)
 * @param IsQuote (I)
 */

int MLocalRsvGetAllocCost(

  mrsv_t  *R,         /* I */
  mvpcp_t *V,         /* I (optional) */
  double  *RCostP,    /* O */
  mbool_t  IsPartial, /* I */
  mbool_t  IsQuote)   /* I */

  {
  long RsvDuration;
  long PeriodDuration;
 
  int  PeriodCount;
  int  ProcCount;

  double ChargeFactor = 1.0;

  if ((R == NULL) || (RCostP == NULL))
    {
    return(FAILURE);
    }

  *RCostP = 0.0;

  if (MAM[0].ChargePolicyIsLocal != TRUE)
    {
    return(FAILURE);
    }

  if (MAM[0].UseDisaChargePolicy == TRUE)
    {
    return(MLocalRsvGetDISAAllocCost(R,V,RCostP,IsPartial,IsQuote));
    }

  /* NOTE:  need to bill at end of each month 
   *        ie, rsv from 1/13 to 4/13
   *        charge for 18 days on 2/1, 1 month on 3/1, 1 month on 4/1 and 13 days on 4/13
   */

  /* NOTE:  charges rounded up to next highest full period */

  /* NOTE:  if mamcpDebitRequestedWC==TRUE, bill for all remaining time out to original
            rsv termination */

  if (MAM[0].ChargePeriod != mpNONE)
    {
    int MCount;
    int DCount;

    mulong PStartTime;  /* start of charging period */

    mulong StartTime;
    mulong EndTime;

    StartTime = R->StartTime;
    EndTime = R->EndTime;

    RsvDuration = EndTime - StartTime;

    MDB(5,fSCHED) MLog("INFO:     rsv duration %ld for time range %ld->%ld\n",
      RsvDuration,
      StartTime - MSched.Time,
      EndTime - MSched.Time);

    PeriodCount = 0;

    switch (MAM[0].ChargePeriod)
      {
      case mpMonth:  

        {
        int NowDIndex;
        int StartDIndex;  /* start day index */

        PeriodDuration = MUGetNumDaysInMonth(StartTime);

        if (PeriodDuration <= 0)
          {
          return(FAILURE);
          }

        MUTimeGetDay(StartTime,NULL,&StartDIndex);

        if (IsQuote == TRUE)
          {
          /* evaluate total requested period */

          MUGetTimeRangeDuration(StartTime,EndTime,&MCount,&DCount);

          PStartTime = StartTime;
          }
        else
          {
          /* NOTE:  determine start for 'previous' period (previous part NYI) */
           
          MUGetPeriodRange(
            MSched.Time,
            0,
            0,
            mpMonth,
            (long *)&PStartTime,  /* O */
            NULL,
            NULL);

          if ((IsPartial == FALSE) && (MAM[0].ChargePolicy == mamcpDebitRequestedWC))
            {
            /* rsv is being terminated - charge from start of current period to 
               originally requested end time */

            MUGetTimeRangeDuration(PStartTime,EndTime,&MCount,&DCount);

            MDB(5,fSCHED) MLog("INFO:     %d months+%d days from period start at %ld until end at %ld\n",
              MCount,
              DCount,
              PStartTime - MSched.Time,
              EndTime - MSched.Time);

            MDB(5,fSCHED) MLog("INFO:     period start: %s",
              ctime((time_t *)&PStartTime));

            MDB(5,fSCHED) MLog("INFO:     end time:     %s",
              ctime((time_t *)&EndTime));

            MDB(5,fSCHED) MLog("INFO:     now:          %s",
              ctime((time_t *)&MSched.Time));
            }
          else
            {
            /* evaluate time to end of month only */

            MUGetTimeRangeDuration(StartTime,MSched.Time,&MCount,&DCount);

            MDB(5,fSCHED) MLog("INFO:     %d months+%d days from rsv start at %ld until now at %ld\n",
              MCount,
              DCount,
              PStartTime - MSched.Time,
              EndTime - MSched.Time);

            MDB(5,fSCHED) MLog("INFO:     rsv start:    %s",
              ctime((time_t *)&StartTime));

            MDB(5,fSCHED) MLog("INFO:     now:          %s",
              ctime((time_t *)&MSched.Time));
            }
          }    /* END else */

        if (MCount == 0)
          {
          long EffDuration;

          /* partial month charge - possibly first monthly charge of multi-month allocation */

          /* EffDuration = fraction of days in month covered by charge */
          /* ie, MIN(X,31 - 17) * 86400 */

          /*   chargetime->                                v      
               |----S---------M----------M----------M---E-------| 
                    ^- StartDIndex
                                                    |-----------| <- PeriodDuration (in Days)
                    |-----------------------------------|         <- RsvDuration
          */

          if (MSched.Time >= EndTime)
	    {
	    /* end time reached - charge for all days */

            /* partial month charge - possibly first monthly charge of multi-month allocation */
  
            /* EffDuration = fraction of days in month covered by charge */
            /* ie, MIN(X,31 - 17) * 86400 */
 
            /*  chargetime->                                v      
                |----S---------M----------M----------M---E-------| 
                     ^- StartDIndex
                                                     |-----------| <- PeriodDuration (in Days)
                     |-----------------------------------|         <- RsvDuration
            */

            EffDuration = DCount * MCONST_DAYLEN;
	    }
	  else if ((IsPartial == FALSE) && (MAM[0].ChargePolicy == mamcpDebitRequestedWC))
            {
            /* rsv has been cancelled before endtime reached */

            /*  chargetime->                           v           
	        |----S---------M----------M----------M---E-------| 
	             ^- StartDIndex
	                                             |-----------| <- PeriodDuration (in Days)
	             |-----------------------------------|         <- RsvDuration
	    */

            /* charge from start of period to original end time */

	    EffDuration = DCount * MCONST_DAYLEN;
            }
          else
	    {
	    /* end time not reached - ??? */

            /*   chargetime->                           v      
                 |----S---------M----------M----------M---E-------| 
                      ^- StartDIndex
                                                      |-----------| <- PeriodDuration (in Days)
                      |-----------------------------------|         <- RsvDuration
            */

            EffDuration = MIN(RsvDuration,(PeriodDuration - StartDIndex) * MCONST_DAYLEN);
            }

          ChargeFactor = (double)EffDuration / (PeriodDuration * MCONST_DAYLEN);

          PeriodCount = 1;

          DCount = 0;
          }  /* END if (MCount == 0) */
        else if (MSched.Time >= EndTime)
          {
          /* partial month charge - most likely last charge at end of rsv */

          MUTimeGetDay(EndTime,NULL,&NowDIndex);
         
          ChargeFactor = (double)NowDIndex / PeriodDuration;

          PeriodCount = 1;

          DCount = 0;
          }
        else if ((MSched.Time < EndTime) && 
                 (IsPartial == FALSE) && 
                 (IsQuote == FALSE))
          {
          /* partial month charge - rsv cancelled early */

          MUTimeGetDay(EndTime,NULL,&NowDIndex);

          ChargeFactor = (double)NowDIndex / PeriodDuration;

          if (MAM[0].ChargePolicy == mamcpDebitRequestedWC)
            PeriodCount = MCount;
          else
            PeriodCount = 1;
          }
        else
          {
          MUGetTimeRangeDuration(StartTime,EndTime,&MCount,&DCount);

          ChargeFactor = 1.0;

          PeriodCount = MCount;
          }

        /* convert period duration to match rsv duration */

        PeriodDuration *= MCONST_DAYLEN;
        }    /* END BLOCK (case mpMonth) */

        break;

      case mpWeek:   PeriodDuration = MCONST_WEEKLEN; break;
      case mpDay:    PeriodDuration = MCONST_DAYLEN; break;
      case mpHour:   PeriodDuration = MCONST_HOURLEN; break;
      case mpMinute: PeriodDuration = MCONST_MINUTELEN; break;
      default:       PeriodDuration = MCONST_MONTHLEN; break;
      }  /* END switch (MAM[0].ChargePeriod) */

    if (PeriodCount == 0)
      PeriodCount = RsvDuration / PeriodDuration;

    if (MAM[0].ChargePeriod == mpMonth)
      {
      if (DCount > 0)
        {
        /* partial month period requested - round up to next period */

        PeriodCount++;
        }
      }
    else if (PeriodDuration * PeriodCount < RsvDuration)
      {
      /* partial non-month period requested - round up to next period */

      PeriodCount++;
      }
    }    /* END if (MAM[0].ChargePeriod != mpNONE) */
  else
    {
    PeriodCount = 1;
    }

  if (R->AllocPC > 0)
    ProcCount = R->AllocPC;
  else if (R->DRes.Procs > 0)
    ProcCount = R->AllocTC * R->DRes.Procs;
  else
    ProcCount = R->AllocTC;

  /* NOTE:  must handle rsv's which dedicate full nodes (NYI) */

  if ((IsQuote == TRUE) || 
      (MAM[0].FlushPeriod == mpNONE)) 
    {
    /* quote for full rsv duration */

    if (IsQuote == TRUE)
      *RCostP = MAM[0].ChargeRate * ProcCount * PeriodCount;
    else if (PeriodCount > 1)
      *RCostP = MAM[0].ChargeRate * ProcCount * (PeriodCount - 1 + ChargeFactor);
    else
      *RCostP = MAM[0].ChargeRate * ChargeFactor * ProcCount;
    }
  else if ((IsPartial == FALSE) && (MAM[0].ChargePolicy == mamcpDebitRequestedWC))
    {
    /* NOTE:  identical to stanza above - is this correct? */

    if (PeriodCount > 1)
      *RCostP = MAM[0].ChargeRate * ProcCount * (PeriodCount - 1 + ChargeFactor);
    else
      *RCostP = MAM[0].ChargeRate * ChargeFactor * ProcCount;
    }
  else
    {
    /* assume charge period matches flush period - DANGER */

    /* only charge for one period */

    *RCostP = MAM[0].ChargeRate * ChargeFactor * ProcCount;
    }

  return(SUCCESS);
  }  /* MLocalRsvGetAllocCost() */





int MDISASetDefaults(

  mam_t *A)

  {
  if (A == NULL)
    {
    return(FAILURE);
    }

  A->ChargePolicyIsLocal   = TRUE;
  A->UseDisaChargePolicy   = TRUE;
  A->ChargePeriod          = mpMonth;
  A->StartFailureAction    = mamjfaIgnore;
  A->FlushPeriod           = mpMonth;
  A->ChargePolicy          = mamcpDebitRequestedWC;
  A->CreateCred            = TRUE;

  memset(A->IsChargeExempt,TRUE,sizeof(A->IsChargeExempt));

  A->IsChargeExempt[mxoxVPC] = FALSE;

  return(SUCCESS);
  }  /* END MDISASetDefaults() */





int MDISAGetRsvCost(

  mrsv_t *R,
  mulong  StartTime,
  int     DayCount,
  double *Cost)       /* NOT INITIALIZED HERE!!! */

  {

  long DayStart;
  long DayEnd;

  int hindex;
  int dindex;

  int RsvStartMonth;
  int RsvEndMonth;
  int Month;

  int Days;

  int SAN;

  mhistory_t *History;

  long NextHistoryStart;

  enum DISAVMSizeEnum {
    disa1proc1gb,
    disa1proc2gb,
    disa1proc3gb,
    disa1proc4gb,
    disa2proc2gb,
    disa2proc3gb,
    disa2proc4gb,
    disa4proc4gb,
    disa4proc8gb,
    disaSAN,
    disaLAST };

  enum DISAVMSizeEnum DSize;

  const char *DISAVMSize[] = {
    "1 Proc 1GB",
    "1 Proc 2GB",
    "1 Proc 3GB",
    "1 Proc 4GB",
    "2 Proc 2GB",
    "2 Proc 3GB",
    "2 Proc 4GB",
    "4 Proc 4GB",
    "4 Proc 8GB",
    "SAN",
    NULL };

  double DISASizeCost[disaLAST] = {500.0, 862.0, 1293.0, 1724.0, 1000.0, 1293.0, 1724.0, 2000.0, 3448.0,2.82};

  MDB(5,fSCHED) MLog("INFO:     DISA2 charging rsv '%s' starting at %ld for %d days\n",
    R->Name,
    StartTime,
    DayCount);

  SAN = MUMAGetIndex(meGRes,"SAN",mVerify);

  MUTimeGetDay(R->StartTime,&RsvStartMonth,NULL);
  MUTimeGetDay(R->EndTime,&RsvEndMonth,NULL);

  /* for each day we are charging find out the size of the reservation on that day and
     what the daily charge should be */

  hindex = 0;

  if (R->History == NULL)
    {
    return(FAILURE);
    }

  History = (mhistory_t *)MUArrayListGet(R->History,hindex++);

  if (hindex < R->History->NumItems)
    {
    mhistory_t *tmpH = (mhistory_t *)MUArrayListGet(R->History,hindex);

    NextHistoryStart = tmpH->Time;
    }   /* END if (R->History->NumItems >= hindex) */
  else
    {
    NextHistoryStart = MMAX_TIME;
    }

  for (dindex = 0;dindex < DayCount;dindex++)
    {
    /* get boundaries of the day */

    MUGetPeriodRange(StartTime + (dindex * MCONST_DAYLEN),0,0,mpDay,&DayStart,&DayEnd,NULL);

    /* MUGetPeriodRange() returns exactly midnight for DayEnd, which is technically the next
       day, so subtract one second and use that */

    DayEnd--;

    MDB(5,fSCHED) MLog("INFO:     DISA2 evaluating rsv '%s' on day ending '%ld'\n",
      R->Name,
      DayEnd);

    /* find history of the rsv on that day */

    while (DayEnd > NextHistoryStart)
      {
      /* loop until the end of the day we are looking fits in the reservation
         history */

      History = (mhistory_t *)MUArrayListGet(R->History,hindex++);
     
      if (hindex < R->History->NumItems)
        {
        mhistory_t *tmpH = (mhistory_t *)MUArrayListGet(R->History,hindex);
     
        NextHistoryStart = tmpH->Time;
        }   /* END if (R->History->NumItems >= hindex) */
      else
        {
        NextHistoryStart = MMAX_TIME;
        }
      }    /* END while (DayEnd > NextHistoryStart) */

    MDB(5,fSCHED) MLog("INFO:     DISA2 using history from rsv '%s' that ends before %ld\n",
      R->Name,
      NextHistoryStart);

    /* get reservation size category based off of history */

    DSize = disa1proc1gb;

    switch (History->Res.Procs)
      {
      default:
      case 1:
      
        if (History->Res.Mem < 1500)
          {
          DSize = disa1proc1gb;
          }
        else if ((History->Res.Mem >= 1500) && (History->Res.Mem < 2500))
          {
          DSize = disa1proc2gb;
          }
        else if ((History->Res.Mem >=2500) && (History->Res.Mem < 3500))
          {
          DSize = disa1proc3gb;
          }
        else
          {
          DSize = disa1proc4gb;
          }

        break;
   
      case 2:
   
        if (History->Res.Mem < 2500)
          {
          DSize = disa2proc2gb;
          }
        else if ((History->Res.Mem >= 2500) && (History->Res.Mem < 3500))
          {
          DSize = disa2proc3gb;
          }
        else
          {
          DSize = disa2proc4gb;
          }

        break;
 
      case 4:
   
        if (History->Res.Mem < 4500)
          {
          DSize = disa4proc4gb;
          }
        else
          {
          DSize = disa4proc8gb;
          }

        break;

      case 0:

        if (MSNLGetIndexCount(&History->Res.GenericRes,SAN) > 0)
          {
          DSize = disaSAN;
          }

        break;
      }  /* END switch (R->AllocPC) */

    MDB(5,fSCHED) MLog("INFO:     DISA2 rsv '%s' was size '%s' before %ld\n",
      R->Name,
      DISAVMSize[DSize],
      NextHistoryStart);

    MUTimeGetDay(DayEnd,&Month,NULL);

    if ((Month != RsvStartMonth) &&
        (Month != RsvEndMonth))
      {
      /* month is unique so we will charge based Months Days */

      Days = MUGetNumDaysInMonth(DayEnd);
      }
    else
      {
      Days = MUGetNumDaysInMonth(R->StartTime);
      }

    if (DSize == disaSAN)
      {
      /* daily cost of SAN is per gigabyte */

      /* find number of vm rsvs and subtract 60 for each one */
      int TotalSAN = MAX(0,MSNLGetIndexCount(&History->Res.GenericRes,SAN) - (History->ElementCount * 60));

      MDB(5,fSCHED) MLog("INFO:     DISA2 rsv '%s' on %ld using month days %d (day cost '%.04f')\n",
        R->Name,
        NextHistoryStart,
        Days,
        DISASizeCost[DSize] * TotalSAN);

      *Cost += (DISASizeCost[DSize] * TotalSAN) / Days;
      }
    else
      {
      MDB(5,fSCHED) MLog("INFO:     DISA2 rsv '%s' on %ld using month days %d (day cost '%.04f')\n",
        R->Name,
        NextHistoryStart,
        Days,
        DISASizeCost[DSize] / Days);


      *Cost += DISASizeCost[DSize] / Days;
      }
    }    /* END for (dindex) */

  return(SUCCESS);
  }  /* END MDISAGetRsvCost() */


/**
 * DISA charging routine.  This code is EXTREMELY specific to DISA2.  It is being 
 * kept specific to make it as easy as possible to code.  With DISA1 the code was very
 * difficult to maintain.
 *
 * CHARGEPOLICY=DEVITREQUESTEDWC
 * CHARGEOBJECTS=vpc
 * FLUSHINTERVAL=month
 * CHARGERATE=LOCAL:<variates>/MONTH*
 * V cannot be NULL
 *
 * Every reservation charged will fall into one of these categories:
 *
 * 1 CPU, 1 GB RAM, 60 GB Disk $ 500
 * 1 CPU, 2 GB RAM, 60 GB Disk $ 862
 * 1 CPU, 3 GB RAM, 60 GB Disk $1,293
 * 1 CPU, 4 GB RAM, 60 GB Disk $1,724
 * 2 CPU, 2 GB RAM, 60 GB Disk $1,000
 * 2 CPU, 3 GB RAM, 60 GB Disk $1,293
 * 2 CPU, 4 GB RAM, 60 GB Disk $1,724
 * 4 CPU, 4 GB RAM, 60 GB Disk $2,000
 * 4 CPU, 8 GB RAM, 60 GB Disk $3,448
 * 1 GB storage $ 2.82
 *
 * These are the monthly rates.  The daily rate is the size of the reservation at the end
 * of the day divided by the number of days in the month in which the VPC was created if it 
 * is a partial month.  Otherwise divide by the number of days in the current month.
 *
 * For each day of the current billing phase Moab will categorize the Rsv for that day
 * then add up the charge based on the category and the month.
 *
 * Does not charge for the start or end pads.
 *
 * VPCs are valid for 1 month increments but because a reservation can be created and 
 * destroyed at will we cannot assume any 1 month increments for a typical reservation.
 *
 * NOTE: There is a special case that has to be accounted for.  If a reservation is created
 * at the end of a month, and the month in which it ends has fewer days then we still have
 * to charge for a month.  Example: Start is Jan 31, End is Feb 28, must charge for an extra
 * 3 days so that the charge is good for a whole month.  This is done by adding an extra
 * charge to the quote or at the end of the reservation.
 */

int MLocalRsvGetDISAAllocCost(

  mrsv_t  *R,         /* I */
  mvpcp_t *V,         /* I (optional) */
  double  *RCostP,    /* O */
  mbool_t  IsPartial, /* I */
  mbool_t  IsQuote)   /* I */

  {
  int  TotalDays;

  long ChargeStartTime;
  int  ChargeDuration;

  if (RCostP != NULL)
    *RCostP = 0.0;

  if (R == NULL)
    {
    return(FAILURE);
    }

  MDB(5,fSCHED) MLog("INFO:     determining charge for rsv '%s'\n",
    R->Name);

  if (IsQuote == TRUE)
    {
    /* for quotes, charge is full duration of reservation */

    ChargeStartTime = R->StartTime;
    ChargeDuration  = R->EndTime - R->StartTime;

    if (V != NULL)
      {
      ChargeDuration -= V->ReqStartPad;
      ChargeDuration -= V->ReqEndPad;
      }
    }  /* END if (IsQuote == TRUE) */
  else
    {
    /* get the start of the current charge interval */
    if (R->LastChargeTime == 0)
      {
      MDB(5,fSCHED) MLog("INFO:     rsv '%s' has never been charged, starting charge at '%ld'\n",
        R->Name,
        R->StartTime);
   
      /* reservation has never been charged so account for the start pad */
      ChargeStartTime = R->StartTime + ((V != NULL) ? V->ReqStartPad : 0);
      }
    else
      {
      MDB(5,fSCHED) MLog("INFO:     rsv '%s' has been charged before, starting charge at '%ld'\n",
        R->Name,
        R->LastChargeTime);
   
      /* reservation has been charged, no need to account for the startpad */
      ChargeStartTime = R->LastChargeTime;
      }
   
    /* get the duration of the current charge interval */
    if (R->EndTime <= MSched.Time)
      {
      /* reservation is ending so account for the end pad */
      ChargeDuration = MSched.Time - ChargeStartTime - ((V != NULL) ? V->ReqEndPad : 0);
   
      /* charges are per day, but we charge for the first day, no matter when the reservation started,
         so we half to chop off a day at the end to never charge for the extra day */

      /* this could be a problem if the reservation is created right at midnight */

      ChargeDuration -= MCONST_DAYLEN;

      MDB(5,fSCHED) MLog("INFO:     rsv '%s' is ending (charge duration is '%d')\n",
        R->Name,
        ChargeDuration);
      }
    else
      {
      /* reservation is not ending, just charge to NOW */
      ChargeDuration = MSched.Time - ChargeStartTime;
   
      MDB(5,fSCHED) MLog("INFO:     rsv '%s' is not ending (charge duration is '%d')\n",
        R->Name,
        ChargeDuration);
      }
    }    /* END else */

  /* get the total days that we are charging for */
  TotalDays = (ChargeDuration / MCONST_DAYLEN) + ((ChargeDuration % MCONST_DAYLEN != 0) ? 1 : 0);

  MDB(5,fSCHED) MLog("INFO:     rsv '%s' charge will be for %d days\n",
    R->Name,
    TotalDays);

  MDISAGetRsvCost(R,ChargeStartTime,TotalDays,RCostP);

  return(SUCCESS);
  }  /* MLocalRsvGetDISAAllocCost() */





int MDISATest()

  {
  mrsv_t R;

  long   StartTime = 1255984844;

  double Cost = 0.0;

  memset(&R,0,sizeof(R));

  MUStrCpy(R.Name,"test-rsv",sizeof(R.Name));

  R.StartTime = StartTime;

  MSched.Time = StartTime;
  R.AllocPC   = 2;
  R.DRes.Mem  = 2000;

  MHistoryAddEvent(&R,mxoRsv);

  MSched.Time = StartTime + 4 * MCONST_DAYLEN;
  R.AllocPC   = 4;
  R.DRes.Mem  = 4000;

  MHistoryAddEvent(&R,mxoRsv);
  
  MSched.Time = StartTime + 8 * MCONST_DAYLEN;
  R.EndTime   = MSched.Time;

  MSched.Time = StartTime + 34 * MCONST_DAYLEN;

  MLocalRsvGetDISAAllocCost(&R,NULL,&Cost,TRUE,FALSE);

  return(SUCCESS);
  }  /* END MDISATest() */

/* END MLocal.c */
