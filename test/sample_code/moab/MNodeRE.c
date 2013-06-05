/* HEADER */

      
/**
 * @file MNodeRE.c
 *
 * Contains: Node and RE functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 * Locate reservation on node which exists before MaxETime.
 * 
 * @see MNodeGetSysJob() - peer
 *
 * @param N (I)
 * @param JobOnly (I)
 * @param MaxETime (I)
 * @param RP (O) [optional]
 */

int MNodeGetRsv(

  const mnode_t *N,
  mbool_t        JobOnly,
  mulong         MaxETime,
  mrsv_t       **RP)

  {
  mre_t  *RE;
  mrsv_t *R;

  if (RP != NULL)
    *RP = NULL;

  if ((N == NULL) || (N->RE == NULL))
    {
    return(FAILURE);
    }

  for (RE = N->RE;RE != NULL;MREGetNext(RE,&RE))
    {
    R = MREGetRsv(RE);

    if ((JobOnly == TRUE) && (R->Type != mrtJob))
      {
      continue;
      }

    if ((mulong)RE->Time > MaxETime)
      break;

    if (RP != NULL)
      *RP = R;

    /* matching RE found */

    return(SUCCESS);
    }  /* END for (reindex) */

  /* matching RE not located */

  return(FAILURE);
  }  /* END MNodeGetRsv() */




/* NOTE:  some variables moved outside of MNodeBuildRE() to avoid stack overflow
          issues under gcc */

mrsv_t *CRI[MMAX_RSV_GDEPTH];   /* container reservation index */
int     CRC[MMAX_RSV_GDEPTH];
int     CRESI[MMAX_RSV_GDEPTH];
int     CREEI[MMAX_RSV_GDEPTH];

/**
 * Update RE (reservation event) table on node N.
 *
 * @see MRsvCheckJAccess() - child
 * @see MREInsert() - child
 *
 * NOTE:  MNodeBuildRE() need only be called if non-job rsv is present on node.
 *
 * NOTE:  if a job reservation is being removed then MNodeBuildRE()
 *        may not compensate for other reservations needing to expand onto
 *        those reservations
 *
 * @param N (I) [required,modified]
 */

int MNodeBuildRE(

  mnode_t *N)        /* I (required,modified) */
 
  {
  mrsv_t  *JR;
  mrsv_t  *R;
 
  mjob_t  *J;
 
  int     JRC;
  long    Overlap;
 
  int     crindex;
 
  int     jrindex1;
  int     jrindex2;
 
  int     oreindex;
  int     nreindex;
  int     creindex;
  int     jreindex;
  int     treindex;
 
  long    InitialJStartTime;
  long    CRStartTime;
  long    CREndTime;
  long    CROverlap;
  long    JRETime;

  /* NOTE:  should allow MMAX_RSV_GDEPTH to be smaller than MMAX_RSV_DEPTH, ie, alloc to MAX of both (NYI) */
 
  mre_t  *tRE;
  mrsv_t *tR;

  mre_t  *NodeRE = NULL;
  int     NodeRESize;

  static mre_t  *CRE  = NULL;  /* container reservation events */
  static mre_t  *ORE  = NULL;  /* original reservation events (from N->RE[]) */
  static mre_t  *NRE  = NULL;  /* new reservation events */
  static mre_t  *JRE  = NULL;  /* job reservation events */
  static mre_t  *mCRE = NULL;
  static int    *AdjustedRC = NULL;

  static int     MaxTRsv;     /* size of re table */
 
  mcres_t BR;
  mcres_t IBR;  /* blocked resources */
  static mcres_t *ZRes = NULL; /* empty CRes structure */
 
  int     OPC;
  int     NPC;

  const char *FName = "MNodeBuildRE";

  MDB(4,fCORE) MLog("%s(%s)\n",
    FName,
    (N != NULL) ? N->Name : "NULL");

  if ((N == NULL) || (N->RE == NULL))
    {
    return(FAILURE);
    }

  N->RE = MRESort(N->RE);

  if ((MNodeHasPartialUserRsv(N) == FALSE) || (MNodeGetJobCount(N) == 0))
    {
    return(SUCCESS);
    }

  /* must calculate blocked res for each 'container' reservation at  */
  /* each rsv event boundary.  collapse container reservations       */
  /* where possible using memcmp.  Insert all container reservation  */
  /* events at blocked res calculation completion                    */

  /* NOTE:  new RE table consists of                                 *
   *          job and container rsv events                           *
   *        in case of multiple containers                           *
   *          jobs extract resources from all inclusive reservations *
   *        in case of multiple competing containers                 *
   *          available resources assigned to rsvs in priority order */

  /* if Rebuild is set, rebuild full RE table                        *
   * otherwise, build only events occuring in SR timeframe           */

  if (ZRes == NULL)
    {
    ZRes = (mcres_t *)MUCalloc(1,sizeof(mcres_t));
    MCResInit(ZRes);
    }

  MaxTRsv = MAX(MMAX_RSV_DEPTH,MMAX_RSV_GDEPTH);

  MaxTRsv <<= 1;

  if (JRE == NULL)
    {
    /* NOTE:  realloc JRE,... if MSched.MaxRsvPerNode changes (NYI) */

    MREAlloc(&CRE,MaxTRsv);
    MREAlloc(&ORE,MaxTRsv);
    MREAlloc(&NRE,MaxTRsv);
    MREAlloc(&JRE,MaxTRsv);
    MREAlloc(&mCRE,MaxTRsv);
    
    AdjustedRC = (int *)MUCalloc(1,sizeof(int) * MaxTRsv);

    if (JRE == NULL)
      {
      return(FAILURE);
      }
    }    /* END if (JRE == NULL) */

  /* clear CR index table */

  CRI[0] = NULL;
 
  jreindex = 0;
 
  JRETime  = 0;

  /* locate specified SR entries */

  MREListToArray(N->RE,&NodeRE,&NodeRESize);

  if (NodeRESize == 0)
    return(SUCCESS);

  for (nreindex = 0;NodeRE[nreindex].Type != mreNONE;nreindex++)
    {
    /* loop through all reservation events on node */

    tRE = &NodeRE[nreindex];
    tR  = MREGetRsv(tRE);

    if (tR == NULL)
      continue;

    if (tR->Type == mrtJob)
      {
      JRETime = tRE->Time;
 
      /* job reservation located */
 
      MRECopyEvent(&JRE[jreindex],tRE,TRUE);
 
      jreindex++;
      }  /* END if (tR->Type == mrtJob) */
    else
      {
      /* determine if specified container rsv is already in CRI list - if not, 
         record container reservation associated with specified rsv event */ 
 
      for (crindex = 0;crindex < MMAX_RSV_GDEPTH;crindex++)
        {
        if (tRE->R == CRI[crindex])
          {
          /* container rsv already processed - eval and break out of loop */

          /* verify job rsv event matches CR split start time */
          /* if not, both events should block same (MAX) resources */
 
          if (JRETime != tRE->Time)
            {
            /* look for matching job re entries */
 
            for (treindex = nreindex + 1;NodeRE[treindex].Type != mreNONE;treindex++)
              {
              if (NodeRE[treindex].Time != tRE->Time)
                break;
 
              if (tR->Type == mrtJob)
                {
                JRETime = tRE->Time;
 
                break;
                }
              }    /* END for (treindex) */
            }      /* END if (JRETime != tRE->Time) */
 
          if (JRETime != tRE->Time)
            {
            /* no matching job res event found */
 
            /* must update both start and end events */
 
            if (tRE->Type == mreStart)
              {
              OPC = (mCRE[crindex].BRes.Procs != -1) ? 
                mCRE[crindex].BRes.Procs : N->CRes.Procs;
              NPC = (tRE->BRes.Procs != -1) ? 
                tRE->BRes.Procs : N->CRes.Procs;
 
              if (OPC > NPC)
                {
                /* if previous event tuple is larger, locate outstanding end event */
 
                for (treindex = nreindex + 1;NodeRE[treindex].Type != mreNONE;treindex++)
                  {
                  if ((NodeRE[treindex].R != tRE->R) ||
                      (NodeRE[treindex].Type != mreEnd))
                    {
                    continue;
                    }
 
                  /* end event found */

                  break;
                  }  /* END for (treindex) */
 
                MCResCopy(&tRE->BRes,&mCRE[crindex].BRes);
                MCResCopy(&NodeRE[treindex].BRes,&mCRE[crindex].BRes);
 
                if ((NodeRE[CRESI[crindex]].BRes.Procs == -1) || 
                    (NodeRE[CREEI[crindex]].BRes.Procs == -1))
                  {
                  fprintf(stderr,"ERROR:  negative blocked resources detected\n");
                  }
                }
              else if (OPC < NPC)
                {
                MCResCopy(&mCRE[crindex].BRes,&tRE->BRes);

                MCResCopy(&NodeRE[CRESI[crindex]].BRes,&mCRE[crindex].BRes);

                MCResCopy(&NodeRE[CREEI[crindex]].BRes,&mCRE[crindex].BRes);
 
                if ((NodeRE[CRESI[crindex]].BRes.Procs == -1) || 
                    (NodeRE[CREEI[crindex]].BRes.Procs == -1))
                  {
                  fprintf(stderr,"ERROR:  negative blocked resources detected\n");
                  }
                }
 
              CRESI[crindex] = nreindex;
              }
            else
              {
              CREEI[crindex] = nreindex;
              }
            }    /* END if (JRETime != tRE->Time) */
 
          break;
          }      /* END if (tRE->Index == CRI[crindex]) */

        /* unique rsv located, add to end of list */
 
        if (CRI[crindex] == NULL)
          {
          /* add new rsv event */

          CRI[crindex]   = tRE->R;
          CRC[crindex]   = tRE->TC;
          CRESI[crindex] = nreindex;
 
          MRECopyEvent(&mCRE[crindex],tRE,TRUE);
 
          CRI[crindex + 1] = NULL;
 
          break;
          }
        }    /* END for (crindex)             */
      }      /* END else (tR->Type == mrtJob) */
    }        /* END for (nreindex)            */

  JRE[jreindex].Type = mreNONE;
  JRE[jreindex].R = NULL;
 
  InitialJStartTime = (jreindex > 0) ?
    JRE[0].R->StartTime :
    MMAX_TIME;

  if (jreindex == 0)
    {
    /* no job rsvs, no reason to rebuild */
 
    for (nreindex = 0;nreindex < NodeRESize;nreindex++)
      {
      MCResFree(&NodeRE[nreindex].BRes);
      }
   
    MUFree((char **)&NodeRE);

    return(SUCCESS);
    }
 
  MRECopy(ORE,JRE,jreindex + 1,TRUE);

  nreindex = jreindex;
 
  MDB(6,fSTRUCT) MLog("INFO:     updating container reservations on node '%s'\n",N->Name);

  /* CR: container reservation */
 
  MCResInit(&IBR);
  MCResInit(&BR);

  for (jrindex1 = 0;JRE[jrindex1].Type != mreNONE;jrindex1++)
    {
    AdjustedRC[jrindex1] = JRE[jrindex1].TC;
    }

  for (crindex = 0;CRI[crindex] != NULL;crindex++)
    {
    creindex = 0;

    /* NOTE:  memset below is very expensive - consider reducing size of 
              memset or replacing with end of list variable */ 

    /* NOTE:  CRE memset below should not be required - CRE structure should be properly populated and terminated
              below - if not, fix code below */

    /* memset(CRE,0,sizeof(mre_t) * MaxTRsv); */
 
    R = CRI[crindex];

    CRStartTime = MAX(MSched.Time,R->StartTime);
 
    CREndTime   = R->EndTime;
 
    /* blocked resources =~ dedicated resource per task * task count */
 
    MCResClear(&IBR);

    /* IBR is total blocked resources */
 
    MCResAdd(&IBR,&N->CRes,&R->DRes,CRC[crindex],FALSE);

    if (CRStartTime < InitialJStartTime)
      {
      CRE[creindex].R     = CRI[crindex];
      CRE[creindex].TC    = CRC[crindex];
      CRE[creindex].Time  = CRStartTime;
      CRE[creindex].Type  = mreStart;
 
      MCResCopy(&CRE[creindex].BRes,&IBR);

      creindex++;
 
      CRE[creindex].R     = CRI[crindex];
      CRE[creindex].TC    = CRC[crindex];
      CRE[creindex].Time  = MIN(CREndTime,InitialJStartTime);
      CRE[creindex].Type  = mreEnd;
 
      MCResCopy(&CRE[creindex].BRes,&IBR);

      CRStartTime = InitialJStartTime;
 
      creindex++;
      }  /* END if (CRStartTime < InitialJStartTime) */
 
    for (jrindex1 = 0;JRE[jrindex1].Type != mreNONE;jrindex1++)
      {
      /* locate smallest event interval */
 
      for (jrindex2 = 0;JRE[jrindex2].Type != mreNONE;jrindex2++)
        {
        if (JRE[jrindex2].Time > CRStartTime)
          {
          CREndTime = MIN(CREndTime,JRE[jrindex2].Time);
 
          break;
          }
        }    /* END for (jrindex2) */ 
 
      Overlap =
        MIN((mulong)CREndTime,R->EndTime) -
        MAX((mulong)CRStartTime,R->StartTime);
 
      if (Overlap <= 0)
        {
        if ((mulong)CRStartTime >= R->EndTime)
          break;

        continue;
        }
 
      MCResCopy(&BR,&IBR);

      for (jrindex2 = 0;JRE[jrindex2].Type != mreNONE;jrindex2++)
        {
        if (JRE[jrindex2].Type != mreStart)
          continue;
 
        if (JRE[jrindex2].Time >= CREndTime)
          break;

        JR = JRE[jrindex2].R;
 
        Overlap =
          MIN((mulong)CREndTime,JR->EndTime) -
          MAX((mulong)CRStartTime,JR->StartTime);
 
        if (Overlap <= 0)
          {
          if (JR->StartTime > (mulong)CREndTime)
            break;

          continue;
          }
 
/*
        JRC = JRE[jrindex2].TC;
*/
        JRC = AdjustedRC[jrindex2];
 
        J = JR->J;
 
        /* if job reservation overlaps container reservation component */
 
        switch (R->Type)
          {
          case mrtUser:
 
            CROverlap =
              MIN(R->EndTime,JR->EndTime) -
              MAX(R->StartTime,JR->StartTime);
 
            if (MRsvCheckJAccess(
                 R,
                 J,
                 CROverlap,
                 NULL,
                 FALSE,
                 NULL,
                 NULL,
                 NULL,
                 NULL) == TRUE)
              {
              if ((JRC > 0) && (JRE[jrindex2].Type == mreStart))
                { 
                /* remove dedicated job resources from container blocked resources */

                MCResRemove(&BR,&N->CRes,&JR->DRes,JRC,FALSE);

                AdjustedRC[jrindex2] -= JRC;
                }
              else if ((JRC > 0) && (JRE[jrindex2].Type == mreEnd))
                {
                /* remove dedicated job resources from container blocked resources */
 
                MCResAdd(&BR,&N->CRes,&JR->DRes,JRC,FALSE);

                AdjustedRC[jrindex2] -= JRC;
                }
              }
 
            break;

          default:

            /* NO-OP */
 
            break;
          }     /* END switch (CR->Type) */
        }       /* END for (jrindex2)    */
 
      if ((mulong)CRStartTime == R->EndTime)
        break;
 
      MCResGetMax(&BR,&BR,ZRes);
 
      /* create new range */
 
      CRE[creindex].R     = CRI[crindex];
      CRE[creindex].TC    = CRC[crindex];
      CRE[creindex].Time  = CRStartTime;
      CRE[creindex].Type  = mreStart;
 
      MCResCopy(&CRE[creindex].BRes,&BR);

      creindex++;

      CRE[creindex].R     = CRI[crindex];
      CRE[creindex].TC    = CRC[crindex];
      CRE[creindex].Time  = CREndTime;
      CRE[creindex].Type  = mreEnd;
 
      MCResCopy(&CRE[creindex].BRes,&BR);

      creindex++;
 
      if ((mulong)CREndTime >= R->EndTime)
        break;
 
      /* advance start/end time for new CRes */
 
      CRStartTime = CREndTime;
      CREndTime   = R->EndTime;
      }         /* END for (jrindex1)    */ 
 
    if (creindex == 0)
      {
      /* no job overlap, use original CR events */
 
      CRE[creindex].R     = CRI[crindex];
      CRE[creindex].TC    = CRC[crindex];
      CRE[creindex].Time  = CRStartTime;
      CRE[creindex].Type  = mreStart;
 
      MCResCopy(&CRE[creindex].BRes,&IBR);

      creindex++;
 
      CRE[creindex].R     = CRI[crindex];
      CRE[creindex].TC    = CRC[crindex];
      CRE[creindex].Time  = CREndTime;
      CRE[creindex].Type  = mreEnd;
 
      MCResCopy(&CRE[creindex].BRes,&IBR);

      creindex++;
      }
 
    /* merge container rsv (CR) events with job rsv (JR) events */
 
    CRE[creindex].Type = mreNONE;
 
    creindex = 0;
    oreindex = 0;
    nreindex = 0;
 
    while ((CRE[creindex].Type != mreNONE) ||
           (ORE[oreindex].Type != mreNONE))
      {
      if ((creindex >= NodeRESize) ||
          (oreindex >= NodeRESize) ||
          (nreindex >= NodeRESize))
        {
        MDB(1,fSTRUCT) MLog("ALERT:    node reservation event overflow (N: %d  C: %d  O: %d) - increase %s (MAX=%d)\n",
          nreindex,
          creindex,
          oreindex,
          MParam[mcoMaxRsvPerNode],
          NodeRESize);

        MMBAdd(
          &MSched.MB,
          "maximum rsv events per node exceeded - set 'MAXRSVPERNODE'",
          NULL,
          mmbtOther,
          MSched.Time + MCONST_DAYLEN,
          0,
          NULL);

        MSched.MaxRsvPerNodeReached = TRUE;

        break;
        }
 
      if ((ORE[oreindex].Type == mreNONE) ||
         ((CRE[creindex].Type != mreNONE) &&
          (CRE[creindex].Time < ORE[oreindex].Time)))
        {
        MRECopyEvent(
          &NRE[nreindex],
          &CRE[creindex],
          TRUE);
 
        nreindex++;
 
        creindex++;
        }
      else
        {
        MRECopyEvent(
          &NRE[nreindex],
          &ORE[oreindex],
          TRUE);
 
        nreindex++;

        oreindex++;
        }
      }   /* END while ((CRE[creindex].Type != mreNONE) || (ORE[oreindex].Type != mreNONE)) */ 
 
    MRECopy(
      ORE,
      NRE,
      nreindex,
      TRUE);
 
    ORE[nreindex].Type = mreNONE;
    }     /* END for (crindex)     */

  /* perform sanity check on RE table */
 
  CRStartTime = 0;
 
  for (nreindex = 0;ORE[nreindex].Type != mreNONE;nreindex++)
    {
    char TString[MMAX_LINE];

    if (nreindex >= NodeRESize)
      break;

    tRE = &ORE[nreindex];
    tR  = MREGetRsv(tRE);

    MULToTString(tRE->Time - MSched.Time,TString);
 
    MDB(8,fSTRUCT) MLog("INFO:     N[%s]->RE[%02d] (%s %s %s)\n",
      N->Name,
      nreindex,
      tR->Name,
      TString,
      MREType[tRE->Type]);
 
    if (CRStartTime > tRE->Time)
      {
      MDB(2,fSTRUCT) MLog("ALERT:    node %s RE table is corrupt.  RE[%d] '%s' at %s is out of time order\n",
        N->Name,
        nreindex,
        tR->Name,
        TString);
      }
 
    CRStartTime = tRE->Time;
    }   /* END for (reindex) */

  MRECopy(
    NodeRE,
    ORE,
    nreindex + 1,
    TRUE);

  MCResFree(&IBR);
  MCResFree(&BR);

  MREFree(&N->RE);

  for (nreindex = 0;NodeRE[nreindex].Type != mreNONE;nreindex++)
    {
    MREAddSingleEvent(&N->RE,&NodeRE[nreindex]);
    }

  for (nreindex = 0;nreindex < NodeRESize;nreindex++)
    {
    MCResFree(&NodeRE[nreindex].BRes);
    }

  MUFree((char **)&NodeRE);

  return(SUCCESS);
  }  /* END MNodeBuildRE() */

/* END MNodeRE.c */
