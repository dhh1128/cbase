/* HEADER */

      
/**
 * @file MRsvAdjust.c
 *
 * Contains: Rsv Adjust attribute functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 * Adjust rsv resource allocation in space/time.
 *
 * NOTE:  Perform analysis by converting rsv to job and scheduling job.
 *
 * @see MSysScheduleVPC() - parent - adjust VPC's
 * @see MJobGetRange() - child
 * @see MJobAllocMNL() - child
 * @see MRsvToJob() - child
 *
 * @param RS (I) [optional]
 * @param StartTime (I) [optional, -1 = not specified]
 * @param TC (I) [optional, -1 = not specified]
 * @param TimeLock (I) [rsv time may not be adjusted]
 */

int MRsvAdjust(

  mrsv_t *RS,         /* I (optional) */
  long    StartTime,  /* I earliest allowed starttime (optional, -1 = not specified) */
  int     TC,         /* I (optional, -1 = not specified) */
  mbool_t TimeLock)   /* I (rsv time may not be adjusted) */

  {
  mrsv_t  *R;

  rsv_iter RTI;

  long     BestStartTime;

  long     Delta;

  char    *NodeMap = NULL;

  mrange_t GRange[MMAX_RANGE];

  mnl_t  *MNodeList[MMAX_REQ_PER_JOB];

  mjob_t *J = NULL;

  enum MNodeAllocPolicyEnum NAPolicy;

  const char *FName = "MRsvAdjust";

  MDB(2,fSCHED) MLog("%s(%s,%ld,%d,%s)\n",
    FName,
    (RS != NULL) ? RS->Name : "NULL",
    StartTime,
    TC,
    MBool[TimeLock]);

  /* initialize 'rsv-map' pseudo-job */

  MNodeMapInit(&NodeMap);

  MNLMultiInit(MNodeList);

  MJobMakeTemp(&J);

  J->RsvAList = (char **)MUCalloc(1,sizeof(char *) * MMAX_PJOB);

  MRsvIterInit(&RTI);

  if (RS != NULL)
    {
    /* target reservation explicitly specified - go directly to operation */

    R = RS;

    goto RShortCut;
    }

  /* locate rsvs to migrate */

  while (MRsvTableIterate(&RTI,&R) == SUCCESS)
    {
    if (bmisset(&R->Flags,mrfIsVPC))
      {
      /* VPCs are adjusted only when directly called with RS */

      continue;
      }

    if (RS == NULL)
      {
      if (!bmisset(&R->Flags,mrfTimeFlex))
        {
        /* reservation is not adjustable */

        continue;
        }

      if (R->StartTime <= MSched.Time)
        {
        /* reservation is already active */

        continue;
        }
      }
    else
      {
      if (R != RS)
        {
        /* reservation not specified */

        continue;
        }
      }    /* END else (RS == NULL) */

RShortCut:

    MDB(1,fCORE) MLog("INFO:     evaluating reservation %s\n",
      R->Name);

    /* maintain existing reservation */

    if (MRsvToJob(R,J) == FAILURE)
      {
      continue;
      }

    MJobAddAccess(J,R->Name,FALSE);

    if (TC > 0)
      MReqSetAttr(J,J->Req[0],mrqaTCReqMin,(void **)&TC,mdfInt,mSet);

    if (TimeLock == FALSE)
      {
      mbitmap_t BM;

      if (StartTime > 0)
        {
        BestStartTime = StartTime;
        }
      else
        {
        BestStartTime = R->StartTime;
        }

      bmset(&BM,mrtcStartEarliest);
      if (MJobGetRange(
           J,
           J->Req[0],
           NULL,
           &MPar[0],
           MSched.Time,
           FALSE,
           GRange,       /* O */
           NULL,
           NULL,
           NodeMap,
           &BM,
           MSched.SeekLong,
           NULL,
           NULL) == FAILURE)
        {
        /* cannot locate available resources */

        if (RS != NULL)
          break;

        continue;
        }

      if (StartTime > 0)
        {
        if ((long)GRange[0].STime != StartTime)
          {
          MDB(2,fSCHED) MLog("INFO:     cannot obtain desired starttime (%ld != %ld)\n",
            GRange[0].STime,
            StartTime);

          if (RS != NULL)
            break;

          continue;
          }
        }
      else if (bmisset(&R->Flags,mrfIsVPC))
        {
        /* always move vpcs? */

        /* NO-OP */
        }
      else if ((long)(GRange[0].STime + 60) >= BestStartTime)
        {
        /* new reservation is not significantly better */

        if (RS != NULL)
          break;

        continue;
        }

      BestStartTime = GRange[0].STime;
      }  /* END if (TimeLock == FALSE) */
    else
      {
      BestStartTime = R->StartTime;
      }  /* END else (TimeLock == FALSE) */

    /* select resources */

    if (MJobGetRange(
         J,
         J->Req[0],
         NULL,
         &MPar[0],
         BestStartTime,
         FALSE,
         GRange,
         MNodeList, /* O */
         NULL,
         NodeMap,
         0,
         MSched.SeekLong,
         NULL,
         NULL) == FAILURE)
      {
      /* cannot locate new resources */

      if (RS != NULL)
        break;

      continue;
      }

    NAPolicy = (J->Req[0]->NAllocPolicy != mnalNONE) ?
      J->Req[0]->NAllocPolicy :
      MPar[0].NAllocPolicy;

    if (MJobAllocMNL(
          J,
          MNodeList,  /* I */
          NodeMap,
          MNodeList,  /* O */
          NAPolicy, 
          MSched.Time,
          NULL,
          NULL) == FAILURE)
      {
      MDB(2,fSCHED) MLog("INFO:     cannot allocate nodes for rsv '%s'\n",
        R->Name);

      if (RS != NULL)
        break;

      continue;
      }

    MRsvDeallocateResources(R,&R->NL);

    if (TimeLock == FALSE)
      {
      char TString[MMAX_LINE];

      Delta = R->StartTime - BestStartTime;

      R->StartTime -= Delta;
      R->EndTime   -= Delta;

      MULToTString(Delta,TString);

      MDB(1,fCORE) MLog("INFO:     timeframe for reservation %s adjusted forward by %s seconds\n",
        R->Name,
        TString);
      }  /* END if (TimeLock == FALSE) */

    if (MRsvAllocate(R,MNodeList[0]) == FAILURE)
      {
      MDB(1,fCORE) MLog("ALERT:    cannot adjust reservation %s (reservation empty)\n",
        R->Name);

      if (RS != NULL)
        break;

      continue;
      }

    /* reservation adjustment was successful */

    if (RS != NULL)
      break;
    }  /* END for (rindex) */

  MJobFreeTemp(&J);

  MUFree(&NodeMap);
  MNLMultiFree(MNodeList);

  return(SUCCESS);
  }  /* END MRsvAdjust() */





/**
 * Adjust start and end times for all reservations and reservation events by 
 * Delta seconds.
 *
 * NOTE:  only used for simulation when adjusting simulation clock (MSched.Time)
 *
 * @see MRsvAdjustTimeFrame() - peer
 *
 * @param Delta (I) [time change in seconds]
 */

int MRsvAdjustTime(

  long Delta)  /* I (time change in seconds) */

  {
  rsv_iter RTI;

  int nindex;

  mnode_t *N;
  mrsv_t  *R;
  mre_t   *RE;

  const char *FName = "MRsvAdjustTime";

  MDB(4,fSTRUCT) MLog("%s(%ld)\n",
    FName,
    Delta);

  MRsvIterInit(&RTI);

  while (MRsvTableIterate(&RTI,&R) == SUCCESS)
    {
    R->StartTime = MIN(R->StartTime + Delta,MMAX_TIME);
    R->EndTime   = MIN(R->EndTime + Delta,MMAX_TIME);
    }

  for (nindex = 0;nindex < MSched.M[mxoNode];nindex++)
    {
    N = MNode[nindex];

    if ((N == NULL) || (N->Name[0] == '\0'))
      break;

    if (N->Name[0] == '\1')
      continue;

    for (RE = N->RE;RE != NULL;MREGetNext(RE,&RE))
      {
      RE->Time = MIN(RE->Time + Delta,MMAX_TIME);
      }  /* END for (MREGetNext) */
    }    /* END for (nindex) */

  return(SUCCESS);
  }  /* END MRsvAdjustTime() */





/**
 * Adjust N->R, N->RC, and N->RETable tables
 *
 * If AdjustStats == TRUE, adjust R->AllocNC, R->AllocPC, and R->AllocTC
 * NOTE:  does NOT adjust R->NL
 *
 * @see MNodeBuildRE() - child
 * @see MRsvAllocate() - parent
 *
 * @param R (I) [modified]
 * @param N (I)
 * @param STC (I) [specified task count, -1 for all]
 * @param Mode (I) [set=0, incr=+1, decr=-1]
 * @param AdjustStats (I)
 */

int MRsvAdjustNode(

  mrsv_t  *R,           /* I (modified) */
  mnode_t *N,           /* I */
  int      STC,         /* I (specified task count, -1 for all) */
  int      Mode,        /* I (set=0, incr=+1, decr=-1) */
  mbool_t  AdjustStats) /* I */

  {
  mre_t *RE;

  int AllocTC = 0;
  int AllocPC = 0;

  int AdjustTC = 0;
  int AdjustPC = 0;

  const char *FName = "MRsvAdjustNode";

  MDB(5,fSTRUCT) MLog("%s(%s,%s,%d,%d,%s)\n",
    FName,
    (R != NULL) ? R->Name : "NULL",
    (N != NULL) ? N->Name : "NULL",
    STC,
    Mode,
    MBool[AdjustStats]);

  if ((R == NULL) || (N == NULL))
    {
    return(FAILURE);
    }

  /* NOTE:  user reservations have RE->DRes indicating total reserved resources */
  /*        user reservations have RC indicating max total tasks to block */

  R->MTime    = MSched.Time;
  N->RsvMTime = MSched.Time;

  /* find matching/available reservation slot */

  for (RE = N->RE;RE != NULL;MREGetNext(RE,&RE))
    {
    if (MREGetRsv(RE) == R)
      {
      /* specified reservation located */

      AllocTC = RE->TC;

      if (R->DRes.Procs > 0)
        AllocPC = MIN(N->CRes.Procs,AllocTC * R->DRes.Procs);
      else
        AllocPC = N->CRes.Procs;

      break;
      }
    }    /* END for (sindex) */

  if ((Mode != 0) && (RE == NULL))
    {
    /* no reservation to remove */

    return(FAILURE);
    }

  if (STC < 0)
    {
    if (R->DRes.Procs == -1)
      {
      AdjustTC = 1;
      }
    else
      {
      AdjustTC = N->CRes.Procs;
      }

    AdjustPC = N->CRes.Procs;
    }
  else
    {
    /* task count specified */

    if (R->DRes.Procs == -1)
      AdjustPC = N->CRes.Procs;
    else
      AdjustPC = MIN(N->CRes.Procs,STC * R->DRes.Procs);

    AdjustTC = STC;
    }

  switch (Mode)
    {
    case 0:  /* set tasks */

      if (AdjustStats == TRUE)
        {
        R->AllocTC += AdjustTC - AllocTC;
        R->AllocPC += AdjustPC - AllocPC;

        if (AllocTC == 0)
          {
          R->AllocNC += 1;
          }
        }

      MREInsert(&N->RE,R->StartTime,R->EndTime,R,&R->DRes,AdjustTC);

      break;

    case -1:  /* remove tasks */

      {
      int TC = MAX(0,RE->TC - AdjustTC);

      /* remove the old one with its incorrect TC */

      MRERelease(&N->RE,R);

      /* add it back in if it still belongs on the node */

      if (TC > 0)
        MREInsert(&N->RE,R->StartTime,R->EndTime,R,&R->DRes,TC);

      if (AdjustStats == TRUE)
        {
        R->AllocTC -= MIN(AllocTC,AdjustTC);
        R->AllocPC -= MIN(AllocPC,AdjustPC);

        if (TC <= 0)
          {
          R->AllocNC -= 1;
          }
        }
      }   /* END BLOCK */

      break;

    case 1:  /* add tasks */
    default:

      {
      int TC = RE->TC + AdjustTC;

      if (AdjustStats == TRUE)
        {
        /* CHANGED: 3/31/09 by Jason G. to address RT 4443 */
        /* AllocTC/AllocPC can only be non-zero if the rsv was already on the node */

        R->AllocTC += AdjustTC - AllocTC;
        R->AllocPC += AdjustPC - AllocPC;

        if (R->AllocTC > R->ReqTC)
          {
          MDB(3,fSCHED) MLog("ALERT:    reservation %s AllocTC > ReqTC (%d > %d) at %s:%d\n",R->Name,R->AllocTC,R->ReqTC,__FILE__,__LINE__);
          }

        if (AllocTC == 0)
          {
          R->AllocNC += 1;
          }
        }

      MREInsert(&N->RE,R->StartTime,R->EndTime,R,&R->DRes,TC);
      }   /* END BLOCK */

      break;
    }  /* END switch (Mode) */

  MNodeBuildRE(N);

  return(SUCCESS);
  }  /* END MRsvAdjustNode() */





/**
 * Adjust timeframes of all N->RETable[] tables on allocated nodes to match modified 
 * reservation start/end values.
 *
 * @see MRsvAdjustTime() - peer
 *
 * @param R (I)
 */

int MRsvAdjustTimeframe(

  mrsv_t *R)   /* I */

  {
  int nindex;

  mnode_t *N;

  if (R == NULL)
    {
    return(FAILURE);
    }

  if (!MNLIsEmpty(&R->NL))
    {
    for (nindex = 0;MNLGetNodeAtIndex(&R->NL,nindex,&N) == SUCCESS;nindex++)
      {
      /* release/rebuild existing RE entries */

      MNodeBuildRE(N);
      }    /* END for (nindex) */
    }      /* END if (R->NL != NULL) */
  else
    {
    /* locate nodes containing rsv */

    for (nindex = 0;nindex < MSched.M[mxoNode];nindex++)
      {
      N = MNode[nindex];

      if ((N == NULL) || (N->Name[0] == '\0'))
        break;

      if (N->Name[0] == '\1')
        continue;

      if (N->RE == NULL)
        continue;

      MNodeBuildRE(N);
      }    /* END for (nindex) */
    }      /* END else ((R->NL != NULL) && (N->R != NULL)) */

  return(SUCCESS);
  }  /* END MRsvAdjustTimeframe() */

/* END MRsvAdjust.c */
