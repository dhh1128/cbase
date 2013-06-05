/* HEADER */

/**
 * @file MStats.c
 *
 * Moab Statistics
 */

/*  Contains:                                *
 *  double MStatGetCom(nindex1,nindex2)      *
 *  int MStatInitializeActiveSysUsage()      *
 *  int MStatBuildMatrix(SIndex,SBuffer,BufSize,SDFlagBM,DFormat,MEP) *
 *  char *MStatGetGrid(sindex,G,R,C,T,TV,TV,Mode)  *
 *                                           */



#include <assert.h>

#include "moab.h"
#include "moab-proto.h"
#include "moab-global.h"
#include "moab-const.h"
#include "MEventLog.h"

extern const char *MCredCfgParm[];

/**
 * Create statistic info
 *
 *
 */

int MStatCreate(

  void    **O,
  enum MStatObjectEnum OType)

  {
  if ((O == NULL) || (OType == msoNONE))
    {
    return(FAILURE);
    }

  switch (OType)
    {
    case msoCred:

      {
      must_t *tmpS = (must_t *)MUCalloc(1,sizeof(must_t));

      tmpS->XLoad = (double *)MUCalloc(1,sizeof(double) * MSched.M[mxoxGMetric]);

      *O = tmpS;
      }  /* END case msoCred */

      break;

    case msoNode:

      {
      mnust_t *tmpS = (mnust_t *)MUCalloc(1,sizeof(mnust_t));

      tmpS->XLoad = (double *)MUCalloc(1,sizeof(double) * MSched.M[mxoxGMetric]);

      MSNLInit(&tmpS->CGRes);
      MSNLInit(&tmpS->AGRes);
      MSNLInit(&tmpS->DGRes);

      *O = tmpS;
      }  /* END case msoNode */

      break;

    default:

      break;
    }

  return(SUCCESS);
  }  /* END MStatCreate() */


/**
 * Free statistic resources
 *
 */

int MStatFree(

  void                **O,
  enum MStatObjectEnum  OType)

  {
  if ((O == NULL) || (OType == msoNONE))
    {
    return(FAILURE);
    }

  switch (OType)
    {
    case msoCred:

      {
      must_t *tmpS = (must_t *)*O;

      if (tmpS != NULL)
        MUFree((char **)&tmpS->XLoad);

      MUFree((char **)O);
      }  /* END case msoCred */

      break;

    case msoNode:

      {
      mnust_t *tmpS = (mnust_t *)*O;

      if (tmpS != NULL)
        MUFree((char **)&tmpS->XLoad);

      MSNLFree(&tmpS->CGRes);
      MSNLFree(&tmpS->AGRes);
      MSNLFree(&tmpS->DGRes);

      MUFree((char **)O);
      }  /* END case msoNode */

      break;

    default:

      break;
    }

  return(SUCCESS);
  }  /* END MStatFree() */

  


/**
 * Initialize matrix statistics.
 *
 * @param P (I)
 */

int MStatProfInitialize(

  mprofcfg_t *P)  /* I */

  {
  int    index;
  int    distance;

  double tmp;
  double gstep;

  int    ScaleModFactor;

  long   MinTime;

  const char *FName = "MStatProfInitialize";

  MDB(2,fSTAT) MLog("%s(P)\n",
    FName);

  if (P == NULL)
    {
    return(FAILURE);
    }

  MinTime = MAX(P->MinTime,1);

  /* bound profile statistics */

  P->MinProc = MAX(1,P->MinProc);

  if (P->TimeStepSize > 0)
    P->MaxTime = (unsigned long)(MinTime * pow((double)P->TimeStepSize,(double)P->TimeStepCount));

  if (P->ProcStepSize > 0)
    P->MaxProc = (unsigned long)(P->MinProc * pow((double)P->ProcStepSize,(double)P->ProcStepCount));

  /* set up time scale */

  distance = P->MaxTime / MinTime;
  tmp      = (double)1.0 / P->TimeStepCount;

  gstep = pow((double)distance,tmp);

  tmp = (double)1.0;

  MDB(4,fSTAT) MLog("INFO:     time min value: %4ld  distance: %4d  step: %6.2f\n",
    MinTime,
    distance,
    gstep);

  P->TimeStep[0] = MinTime;

  ScaleModFactor = 0;

  for (index = 1;index <= (int)P->TimeStepCount;index++)
    {
    tmp *= gstep;

    P->TimeStep[index] = (int)(tmp * MinTime + 0.5);

    /* skip previously used values */
 
    if ((index != 0) && (P->TimeStep[index - ScaleModFactor] ==
                         P->TimeStep[index - ScaleModFactor - 1]))
      {
      ScaleModFactor++;
      }

    MDB(5,fSTAT) MLog("INFO:     TimeStep[%02d]: %8ld\n",
      index,
      P->TimeStep[index]);
    }

  P->TimeStep[P->TimeStepCount + 1] = 999999999;

  MDB(4,fSTAT) MLog("INFO:     time steps eliminated: %3d\n",
    ScaleModFactor);
  
  P->TimeStepCount -= ScaleModFactor;

  /* set up node scale */

  distance = P->MaxProc  / P->MinProc;
  tmp      = (double)1.0 / P->ProcStepCount;

  gstep = pow((double)distance,tmp);

  tmp = (double)1.0;

  MDB(4,fSTAT) MLog("INFO:     node min value: %3ld  distance: %3d  step: %4.2f\n",
    P->MinProc,
    distance,
    gstep);

  P->ProcStep[0] = P->MinProc;

  ScaleModFactor = 0;

  for (index = 1;index <= (int)P->ProcStepCount;index++)
    {
    tmp *= gstep;

    P->ProcStep[index - ScaleModFactor] = (int)(tmp * P->MinProc + .5);

    /* if value is used previously, skip it */
  
    if ((index != 0) && 
        (P->ProcStep[index - ScaleModFactor] == P->ProcStep[index - ScaleModFactor - 1]))
      {
      ScaleModFactor++;
      }

    MDB(5,fSTAT) MLog("INFO:     ProcStep[%02d]: %4ld\n",
      index,
      P->ProcStep[index]);
    }

  P->ProcStep[P->ProcStepCount + 1] = 999999999;

  MDB(4,fSTAT) MLog("INFO:     node steps eliminated: %d\n",
    ScaleModFactor);

  P->ProcStepCount -= ScaleModFactor;

  /* set up accuracy scale */

  gstep = 0.0;

  for (index = 0;index < (int)P->AccuracyScale;index++)
    {
    gstep += (double)100 / P->AccuracyScale;

    P->AccuracyStep[index] = (int)gstep;

    MDB(5,fSTAT) MLog("INFO:     AccuracyStep[%02d]: %4ld\n",
      index,
      P->AccuracyStep[index]);
    }

  return(SUCCESS);
  }  /* END MStatProfInitialize() */





/**
 * Increment total FairShare credential usage counts 
 *
 * @param J (I)
 */

int MStatIncrementFSCredUsage(

  mjob_t *J)  /* I */

  {
    mfst_t *TData = NULL;
    int pindex;

    const char *FName = "MStatIncrementFSCredUsage";

    MDB(3,fSTAT) MLog("%s(%s)\n",
      FName,
      (J != NULL) ? J->Name : "NULL");

    if (J == NULL)
      {
      return(FAILURE);
      }

    /* Note that the fstPU indexes start at the leaf nodes of the fairshare tree which are the
     * most specific (e.g. user) and as the index increments it points to the parent data
     * which is more general. (e.g. class and then account) */

    for (pindex = 0; pindex < MMAX_PAR; pindex++)
      {
      /* only check limit for requested partitions */

      if ((MSched.PerPartitionScheduling == TRUE) &&
          (pindex > 0) &&
          (bmisset(&J->SpecPAL,pindex) == FAILURE))
        continue;

      TData = NULL;
      if (J->FairShareTree != NULL)
        {
        if (J->FairShareTree[pindex] != NULL)
          {
          TData = J->FairShareTree[pindex]->TData;
          }
        }
      if (TData == NULL)
        continue;

      while (TData != NULL)
        {
        TData->L->TotalJobs[0]++;

        MDB(7,fSTAT) MLog("INFO:     job '%s' increment total job count to %d for %s usage\n",
          J->Name,
          TData->L->TotalJobs[0],
          MCredCfgParm[TData->OType]);

        /* move to the next higher leaf in the tree */

        if (TData->Parent != NULL)
          {
          TData = TData->Parent->TData;
          }
        else
          break;
        }
      }

    return(SUCCESS);
  }  /* END MStatIncrementFSCredUsage() */



/**
 * Increment total credential usage counts 
 *
 * @param J (I)
 */

int MStatIncrementCredUsage(

  mjob_t *J)  /* I */

  {
  int OTIndex;          /* Object Type Index for OTList */ 
  mcredl_t *L  = NULL;
  void *CTO;            /* Cred Type Object e.g. U,A,G,C,Q */
  void *DefaultCTO;     /* Cred Type Object e.g. MSched.DefaultU, etc */

  /* Object Type list */

  const enum MXMLOTypeEnum OTList[] = {
    mxoUser,
    mxoGroup,
    mxoAcct,
    mxoClass,
    mxoQOS,
    mxoNONE };

  const char *FName = "MStatIncrementCredUsage";

  MDB(4,fSTAT) MLog("%s(%s)\n",
    FName,
    (J != NULL) ? J->Name : "NULL");

  if (J == NULL)
    {
    return(FAILURE);
    }

  for (OTIndex = 0;OTList[OTIndex] != mxoNONE;OTIndex++)
    {
    /* Get a pointer to the U,A,G,C, or Q pointer in the mcred_t structure */

    MCredGetCredTypeObject(&J->Credential,OTList[OTIndex],&CTO,&DefaultCTO);

    /* Get a pointer to the current resource usage L pointer */

    if ((MOGetComponent(CTO,OTList[OTIndex],(void **)&L,mxoxLimits) == SUCCESS) &&
        (L != NULL))
      {
      if (MSched.PerPartitionScheduling != TRUE)
        {
        L->TotalJobs[0]++;

        MDB(7,fSTAT) MLog("INFO:     job '%s' increment total job count to %d for %s usage\n",
          J->Name,
          L->TotalJobs[0],
          MXO[OTList[OTIndex]]);
        }
      else
        {  
        int pindex;
        mbitmap_t tmpPAL;

        L->TotalJobs[0]++;

        bmcopy(&tmpPAL,&J->SpecPAL);

        /* increment cred usage across all requested partitions */

        for (pindex = 1; pindex < MMAX_PAR; pindex++)
          {
          /* quit if we have handled all requested partitions */

          if (bmisclear(&tmpPAL))
            break;
          
          /* skip unaffected partitions */
 
          if (!bmisset(&tmpPAL,pindex))
            continue;

          /* take this prtition out of the bit map of partitions that we still need to process */
  
          bmunset(&tmpPAL,pindex);
  
          L->TotalJobs[pindex]++;
          
          MDB(7,fSTAT) MLog("INFO:     job '%s' (partition %s) increment total job count to %d for %s usage\n",
            J->Name,
            MPar[pindex].Name,
            L->TotalJobs[pindex],
            MXO[OTList[OTIndex]]);
          } /* END for pindex */

        bmclear(&tmpPAL);
        } /* END else */
      } /* END if ((MOGetComponent(CTO,OTList[OTIndex] ... */
    }  /* END for (OTIndex) */

  MStatIncrementFSCredUsage(J);
  return(SUCCESS);
  }  /* END MStatIncrementCredUsage() */



/**
 * Set default attributes for global stat object (MStat).
 *
 * NOTE:  primarily sets default values for matrix statistics.
 *
 */

int MStatSetDefaults(void)

  {
  const char *FName = "MStatSetDefaults";

  MDB(3,fSTAT) MLog("%s()\n",
    FName);

  memset(&MStat,0,sizeof(MStat));       

  /* set default stat values */
 
  MStat.SchedRunTime    = 0;
 
  MStat.MinEff          = 100.0;
  MStat.MinEffIteration = 0;

  MStat.P.IStatDuration = MDEF_PSTEPDURATION;
  MStat.P.MaxIStatCount = MDEF_PSTEPCOUNT;

  MStat.EventFileRollDepth = -1;

#ifdef MYAHOO
  /* Yahoo wants default set to 30 for now */
  MStat.EventFileRollDepth = 30;
#endif /* MYAHOO */
 
  /* set stats directory */
 
  if (!strstr(MSched.VarDir,MDEF_STATDIR))
    {
    sprintf(MStat.StatDir,"%s%s",
      MSched.VarDir,
      MDEF_STATDIR);
    }
  else
    {
    strcpy(MStat.StatDir,MDEF_STATDIR);
    }

  /* initialize statistics matrix bounds */

  MStat.P.MinTime       = MDEF_STIMEMIN;
  MStat.P.MaxTime       = MDEF_STIMEMAX;
  MStat.P.TimeStepCount = MDEF_STIMESTEPCOUNT;
  MStat.P.TimeStepSize  = MDEF_STIMESTEPSIZE;

  MStat.P.MinProc       = MDEF_SPROCMIN;
  MStat.P.MaxProc       = MDEF_SPROCMAX;
  MStat.P.ProcStepCount = MDEF_SPROCSTEPCOUNT;
  MStat.P.ProcStepSize  = MDEF_SPROCSTEPSIZE;
 
  MStat.P.AccuracyScale = MDEF_ACCURACYSCALE;
 
  return(SUCCESS);
  }  /* END MStatSetDefaults() */




/**
 * Load/initialize stat object at scheduler start-up.
 *
 * @param P (I)
 */

int MStatPreInitialize(

  mprofcfg_t *P)  /* I */

  {
  const char *FName = "MStatPreInitialize";

  MDB(3,fSTAT) MLog("%s(P)\n",
    FName);

  MStatProfInitialize(P);

  MStatOpenFile(MSched.Time,NULL);

  if (MSched.PBSAccountingDir != NULL)
    {
    MStatOpenCompatFile(MSched.Time);
    }

  return(SUCCESS);
  }  /* END MStatPreInitialize() */




/**
 * Initialize stat objects.
 *
 */

int MStatInitialize(void)

  {
  int gindex;
  int tindex;

  const char *FName = "MStatInitialize";

  MDB(3,fSTAT) MLog("%s()\n",
    FName);

  /*record SCHEDSTART in events file */
  if (MSched.Iteration <= 0)
    {
    char tmpEventMsg[MMAX_LINE];

    tmpEventMsg[0] = '\0';

    if (MSched.UseCPRestartState == TRUE)
      {
      MUStrCpy(tmpEventMsg,"scheduler starting with CP restart state",sizeof(tmpEventMsg));
      }

    MSysRecordEvent(mxoSched,MSched.Name,mrelSchedStart,NULL,tmpEventMsg,NULL);

    if (MSched.PushEventsToWebService == TRUE)
      {
      MEventLog *Log = new MEventLog(meltSchedStart);
      Log->SetCategory(melcStart);
      Log->SetFacility(melfScheduler);
      Log->SetPrimaryObject(MSched.Name,mxoSched,(void *)&MSched);
      if (tmpEventMsg[0] != '\0')
        Log->AddDetail("msg",tmpEventMsg);

      MEventLogExportToWebServices(Log);

      delete Log;
      }

    if (MSched.State == mssmPaused)
      {
      MSysRecordEvent(
        mxoSched,
        MSched.Name,
        mrelSchedPause,
        NULL,
        (char *)"scheduler paused at startup",
        NULL);

      CreateAndSendEventLogWithMsg(meltSchedPause,MSched.Name,mxoSched,(void *)&MSched,"scheduler paused at startup");
      }
    else if (MSched.State == mssmStopped)
      {
      MSysRecordEvent(
        mxoSched,
        MSched.Name,
        mrelSchedStop,
        NULL,
        (char *)"scheduler stopped at startup",
        NULL);

      CreateAndSendEventLogWithMsg(meltSchedStop,MSched.Name,mxoSched,(void *)&MSched,"scheduler stopped at startup");
      }  /* END else if (MSched.State == mssmStopped) */
    }  /* END if (MSched.Iteration <= 0) */

  if (MStat.InitTime == 0)
    MStat.InitTime = MSched.Time;

  /* called after initial job load */

  if (MSched.ProfilingIsEnabled == TRUE)
    {
    MStatPInitialize(NULL,FALSE,msoCred);
    }

  for (tindex = 0;tindex < MMAX_GRIDTIMECOUNT;tindex++)
    {
    for (gindex = 0;gindex < MMAX_GRIDSIZECOUNT;gindex++)
      {
      MStat.SMatrix[0].Grid[tindex][gindex].XLoad = (double *)MUCalloc(1,sizeof(double) * MSched.M[mxoxGMetric]);
      MStat.SMatrix[1].Grid[tindex][gindex].XLoad = (double *)MUCalloc(1,sizeof(double) * MSched.M[mxoxGMetric]);
      }
    }  /* END for (tindex) */

  for (gindex = 0;gindex < MMAX_GRIDSIZECOUNT;gindex++)
    {
    MStat.SMatrix[0].RTotal[gindex].XLoad = (double *)MUCalloc(1,sizeof(double) * MSched.M[mxoxGMetric]);
    MStat.SMatrix[1].RTotal[gindex].XLoad = (double *)MUCalloc(1,sizeof(double) * MSched.M[mxoxGMetric]);
    MStat.SMatrix[0].CTotal[gindex].XLoad = (double *)MUCalloc(1,sizeof(double) * MSched.M[mxoxGMetric]);
    MStat.SMatrix[1].CTotal[gindex].XLoad = (double *)MUCalloc(1,sizeof(double) * MSched.M[mxoxGMetric]);
    }

  return(SUCCESS);
  }  /* END MStatInitialize() */





/**
 * Clear system 'per iteration' counters and usage stats 
 *
 * NOTE: run once at the beginning of each iteration
 *
 */

int MStatInitializeActiveSysUsage(void)

  {
  const char *FName = "MStatInitializeActiveSysUsage";

  MDB(2,fSTAT) MLog("%s()\n",
    FName);

  MSched.TotalJobStartCounter     += MSched.JobStartPerIterationCounter;
  MSched.TotalJobPreempteeCounter += MSched.JobPreempteePerIterationCounter;
  MSched.TotalJobPreemptorCounter += MSched.JobPreemptorPerIterationCounter;

  MSched.JobStartPerIterationCounter = 0;
  MSched.JobPreempteePerIterationCounter = 0;
  MSched.JobPreemptorPerIterationCounter = 0;

  MStatClearUsage(mxoNONE,TRUE,TRUE,FALSE,FALSE,TRUE);

  return(SUCCESS);
  }  /* END MStatInitializeActiveSysUsage() */





/**
 * Generate report for matrix statistics (for 'showstats -f').
 *
 * @param SIndex (I)
 * @param SBuffer (O) [optional]
 * @param BufSize (I)
 * @param SDFlagBM (I) [bitmap of enum MProfModeEnum]
 * @param DFormat (I) [mfmXML or ???]
 * @param MEP (O) [optional]
 */

int MStatBuildMatrix(

  enum MMStatTypeEnum    SIndex,
  char                  *SBuffer,
  int                    BufSize,
  mbitmap_t             *SDFlagBM, 
  enum MFormatModeEnum   DFormat,
  mxml_t               **MEP)

  {
  int   timeindex;
  int   procindex;

  must_t *G;
  must_t *C;
  must_t *R;
  must_t *T;

  mxml_t *ME;
  mxml_t *RE;
  mxml_t *CE;

  char   *BPtr;
  int     BSpace;

  int     mindex = 0;  /* use long-term matrix */

  char   *ptr;

  char    tmpLine[MMAX_LINE];

  const char *FName = "MStatBuildMatrix";

  MDB(3,fSTAT) MLog("%s(%s,SBuffer,%d,%d,MEP)\n",
    FName,
    MStatType[SIndex],
    BufSize,
    DFormat);

  if (DFormat == mfmXML)
    {
    bmset(SDFlagBM,mXML);

    ME = NULL;

    MXMLCreateE(&ME,"matrix");
 
    MXMLSetAttr(ME,"InitTime",(void *)&MStat.InitTime,mdfLong);

    MXMLSetAttr(ME,"Type",(char *)MStatType[SIndex],mdfString);

    MXMLSetAttr(ME,(char *)MSchedAttr[msaName],MSched.Name,mdfString);
    }
  else
    {
    char DString[MMAX_LINE];

    MUSNInit(&BPtr,&BSpace,SBuffer,BufSize);

    MULToDString((mulong *)&MStat.InitTime,DString);

    MUSNPrintF(&BPtr,&BSpace,"statistics since %s\n",
      DString);
    }

  switch (SIndex)
    {
    case mgstAvgXFactor:

      strcpy(tmpLine,"Average XFactor Grid");

      break;

    case mgstMaxXFactor:

      strcpy(tmpLine,"Maximum XFactor (hours)");

      break;

    case mgstAvgQTime:

      strcpy(tmpLine,"Average QueueTime (hours)");

      break;

    case mgstAvgBypass:

      strcpy(tmpLine,"Average Bypass (bypass count)");

      break;

    case mgstMaxBypass:

      strcpy(tmpLine,"Maximum Bypass (bypass count)");

      break;

    case mgstJobCount:

      strcpy(tmpLine,"Job Count (jobs)");

      break;

    case mgstPSRequest:

      strcpy(tmpLine,"ProcHour Request (percent of total)");

      break;

    case mgstPSRun:

      strcpy(tmpLine,"ProcHour Run (percent of total)");

      break;

    case mgstWCAccuracy:

      strcpy(tmpLine,"WallClock Accuracy (percent)");

      break;

    case mgstBFCount:

      strcpy(tmpLine,"BackFill (percent of jobs run)");

      break;

    case mgstBFPSRun:

      strcpy(tmpLine,"BackFill (percent of prochours delivered)");

      break;

    case mgstQOSDelivered:

      strcpy(tmpLine,"Quality of Service Delivered (percent of jobs meeting QOS)");

      break;

    case mgstEstStartTime:

      strcpy(tmpLine,"Estimated StartTime (HH:MM:SS)");

      break;

    default:

      strcpy(tmpLine,"Unknown Statistics Type");

      MDB(1,fSTAT) MLog("ERROR:    unexpected statistics type, %d\n",
        SIndex);

      break;
    }  /* END switch (SIndex) */

  if (DFormat == mfmXML)
    {
    MXMLSetAttr(ME,"label",(void *)tmpLine,mdfString);
    }
  else
    {
    MUSNPrintF(&BPtr,&BSpace,"%s\n\n",
      tmpLine);
    }

  if (DFormat != mfmXML)
    {
    /* label columns */

    if (bmisset(SDFlagBM,mMatrix))
      {
      MUSNCat(&BPtr,&BSpace,"         ");
      }
    else
      {
      MUSNPrintF(&BPtr,&BSpace,"[ %5s ]",
        "PROCS");
      }

    for (timeindex = 0;timeindex <= (int)MStat.P.TimeStepCount;timeindex++)
      {
      char BString[MMAX_LINE];

      if (bmisset(SDFlagBM,mMatrix))
        {
        MUBStringTime(MStat.P.TimeStep[timeindex],BString);

        MUSNPrintF(&BPtr,&BSpace,"  %8s   ",
          BString);
        }
      else
        {
        MUBStringTime(MStat.P.TimeStep[timeindex],BString);

        MUSNPrintF(&BPtr,&BSpace,"[ %8s  ]",
          BString);
        }
      }    /* END for (timeindex) */

    if (bmisset(SDFlagBM,mMatrix))
      {
      MUSNCat(&BPtr,&BSpace,"              \n");
      }
    else
      {
      if (SIndex != mgstEstStartTime)
        {
        MUSNPrintF(&BPtr,&BSpace,"[  %8s  ]\n",
          "TOTAL");
        }
      else
        {
        MUSNCat(&BPtr,&BSpace,"\n");
        }
      }
    }    /* END if (DFormat != mfmXML) */

  MDB(3,fSTAT) MLog("INFO:     stat header created\n");

  T = &MPar[0].S;

  R = &MStat.SMatrix[mindex].RTotal[0];

  for (procindex = 0;procindex <= (int)MStat.P.ProcStepCount;procindex++)
    {
    /* create row label */

    if (DFormat == mfmXML)
      {
      RE = NULL;

      MXMLCreateE(&RE,"row");

      MXMLAddE(ME,RE);

      MXMLSetAttr(RE,"label",(void *)&MStat.P.ProcStep[procindex],mdfInt);
      }
    else
      { 
      if (bmisset(SDFlagBM,mMatrix))
        {
        MUSNPrintF(&BPtr,&BSpace,"  %4ld   ",
          MStat.P.ProcStep[procindex]);
        }
      else
        {
        MUSNPrintF(&BPtr,&BSpace,"[ %4ld  ]",
          MStat.P.ProcStep[procindex]); 
        }
      }

    R = &MStat.SMatrix[mindex].RTotal[procindex];

    C = &MStat.SMatrix[mindex].CTotal[0];
    
    for (timeindex = 0;timeindex <= (int)MStat.P.TimeStepCount;timeindex++)
      {
      G = &MStat.SMatrix[mindex].Grid[timeindex][procindex];
      C = &MStat.SMatrix[mindex].CTotal[timeindex];

      ptr = MStatGetGrid(
        SIndex,
        G,
        R,
        C,
        T,
        MStat.P.ProcStep[procindex],
        MStat.P.TimeStep[timeindex],
        SDFlagBM);

      if (DFormat == mfmXML)
        {
        CE = NULL;

        MXMLCreateE(&CE,"col");

        MXMLSetAttr(CE,"label",(void *)&MStat.P.TimeStep[timeindex],mdfInt);

        MXMLSetAttr(
          CE,
          "value",
          (void *)ptr,
          mdfString);

        MXMLAddE(RE,CE);
        }
      else
        {
        MUSNCat(&BPtr,&BSpace,ptr);

        if (G->JC != 0)
          {
          switch (SIndex)
            {
            default:

              /* NO-OP */

              break;
            }
          }
        }
      }      /* END for (timeindex = 0;...) */

    /* display column totals */

    ptr = MStatGetGrid(
      SIndex,
      R,
      R,
      C,
      T,
      MStat.P.ProcStep[procindex],
      MStat.P.TimeStep[timeindex],
      SDFlagBM);

    if (DFormat != mfmXML)
      {
      if (SIndex != mgstEstStartTime)
        {
        MUSNCat(&BPtr,&BSpace,ptr);
        }

      MUSNCat(&BPtr,&BSpace,"\n");
      }
    else
      {
      CE = NULL;

      if (SIndex != mgstEstStartTime)
        {
        MXMLCreateE(&CE,"col");
 
        MXMLSetAttr(CE,"label",(void *)"total",mdfString);

        MXMLSetAttr(CE,"value",(void *)ptr,mdfString);

        MXMLAddE(RE,CE);
        }
      }

    MDB(3,fSTAT) MLog("INFO:     stat row[%02d] created\n",
      procindex);
    }  /* END for (procindex) */

  if (SIndex != mgstEstStartTime)
    {
    /* calculate column totals */

    if (DFormat != mfmXML)
      {
      if (bmisset(SDFlagBM,mMatrix))
        {
        MUSNCat(&BPtr,&BSpace,"  TOTAL  ");
        }
      else
        {
        MUSNCat(&BPtr,&BSpace,"[ TOTAL ]");
        }

      for (timeindex = 0;timeindex <= (int)MStat.P.TimeStepCount;timeindex++)
        {
        C = &MStat.SMatrix[mindex].CTotal[timeindex];

        ptr = MStatGetGrid(
          SIndex,
          C,
          R,
          C,
          T,
          MStat.P.ProcStep[procindex],
          MStat.P.TimeStep[timeindex],
          SDFlagBM);

        MUSNCat(&BPtr,&BSpace,ptr);
        }  /* END for (timeindex) */
      }    /* END if (DFormat != mfmXML) */
    else
      {
      char tmpVal[MMAX_NAME];

      RE = NULL;

      MXMLCreateE(&RE,"row");

      MXMLSetAttr(RE,"label",(void *)"total",mdfString);

      for (timeindex = 0;timeindex <= (int)MStat.P.TimeStepCount;timeindex++)
        {
        CE = NULL;

        MXMLCreateE(&CE,"col");

        MXMLSetAttr(CE,"label",(void *)&MStat.P.TimeStep[timeindex],mdfInt);

        C = &MStat.SMatrix[mindex].CTotal[timeindex];

        ptr = MStatGetGrid(
          SIndex,
          C,
          R,
          C,
          T,
          MStat.P.ProcStep[procindex],
          MStat.P.TimeStep[timeindex],
          SDFlagBM);

        MXMLSetAttr(CE,"value",(void *)ptr,mdfString);

        MXMLAddE(RE,CE);
        }  /* END for (timeindex) */

      /* add total value */

      tmpVal[0] = '\0';

      switch (SIndex)
        {
        case mgstAvgXFactor:

          sprintf(tmpVal,"%.2f",
            (T->JC > 0) ? (double)T->XFactor / T->JC : 0.0);

          break;

        case mgstAvgQTime:

          sprintf(tmpVal,"%.2f",
            (T->JC > 0) ? (double)T->TotalQTS / T->JC / 3600.0 : 0.0);

          break;

        default:

          /* NO-OP */

          break;
        }  /* END switch (X) */

      if (tmpVal[0] != '\0')
        {
        MXMLCreateE(&CE,"col");

        MXMLSetAttr(CE,"label",(void *)"total",mdfString);

        MXMLSetAttr(CE,"value",(void *)tmpVal,mdfString);

        MXMLAddE(RE,CE);
        }  /* END if (tmpVal[0] != '\0') */

      MXMLAddE(ME,RE);
      }

    MDB(3,fSTAT) MLog("INFO:     stat column totals created\n");
    }  /* END if (SIndex != mgstEstStartTime) */

  if (DFormat != mfmXML)
    {
    MUSNCat(&BPtr,&BSpace,"\n");

    /* calculate overall totals */

    switch (SIndex)
      {
      case mgstAvgXFactor:

        MUSNPrintF(&BPtr,&BSpace,"%-26s %8.4f\n",
          "Job Weighted XFactor:",
          (T->JC > 0) ? (double)T->XFactor / T->JC : 0.0);

        MUSNPrintF(&BPtr,&BSpace,"%-26s %8.4f\n",
          "Proc Weighted XFactor:",
          (T->JAllocNC > 0) ? T->NXFactor / T->JAllocNC : 0.0);

        MUSNPrintF(&BPtr,&BSpace,"%-26s %8.4f\n",
          "PS Weighted XFactor:",
          (T->PSRun > 0.0) ? T->PSXFactor / T->PSRun : 0.0);

        break;

      case mgstMaxXFactor:

        MUSNPrintF(&BPtr,&BSpace,"%-26s %8.4f\n",
          "Overall Max XFactor:",
          (double)T->MaxXFactor);

        break;

      case mgstAvgQTime:

        MUSNPrintF(&BPtr,&BSpace,"%-26s %8.4f\n",
          "Job Weighted QueueTime:",
          (T->JC > 0) ? (double)T->TotalQTS / T->JC / 3600.0 : 0.0);

        break;

      case mgstAvgBypass:

        MUSNPrintF(&BPtr,&BSpace,"%-26s %8.4f\n",
          "Job Weighted X Bypass:",
          (T->JC > 0) ? (double)T->Bypass / T->JC : 0.0);

        break;

      case mgstMaxBypass:

        MUSNPrintF(&BPtr,&BSpace,"%-26s %8d\n",
          "Overall Max Bypass:",
          T->MaxBypass);

        break;

      case mgstJobCount:

        MUSNPrintF(&BPtr,&BSpace,"%-26s %8d\n",
          "Total Jobs:",
          T->JC);

        break;

      case mgstPSRequest:

        MUSNPrintF(&BPtr,&BSpace,"%-26s %8.2f\n",
          "Total PH Requested:",
          (double)T->PSRequest / 3600.0);
    
        break;

      case mgstPSRun:

        MUSNPrintF(&BPtr,&BSpace,"%-26s %8.2f\n",
          "Total PH Run",
          (double)T->PSRun / 3600.0);
    
        break;

      case mgstWCAccuracy:

        MUSNPrintF(&BPtr,&BSpace,"%-26s %8.3f\n",
          "Overall WallClock Accuracy:",
          (T->PSRequest > 0.0) ? (double)T->PSRun / T->PSRequest * 100.0 : 0.0);

        break;

      case mgstBFCount:

        MUSNPrintF(&BPtr,&BSpace,"%-26s %8.4f (%d / %d)\n",
          "Job Weighted BackFill Job Percent:",
          (T->JC > 0) ? (double)T->BFCount / T->JC * 100.0 : 0.0,
          T->BFCount,
          T->JC);

        break;

      case mgstBFPSRun:

        MUSNPrintF(&BPtr,&BSpace,"%-26s %8.4f (%6.2f / %6.2f)\n",
          "PS Weighted BackFill PS Percent:",
          (T->PSRun > 0) ? (double)T->BFPSRun / T->PSRun * 100.0 : 0.0,
          T->BFPSRun,
          T->PSRun);

        break;

      case mgstQOSDelivered:

        MUSNPrintF(&BPtr,&BSpace,"%-26s %8.4f (%d / %d)\n",
          "Job Weighted QOS Success Rate:",
          (T->JC > 0) ? (double)T->QOSSatJC / T->JC * 100.0 : 0.0,
          T->QOSSatJC,
          T->JC);

        break;

      case mgstEstStartTime:

        /* NO-OP */

        break;

      default:

        MUSNPrintF(&BPtr,&BSpace,"ERROR:  stat type %d totals not handled\n",
          SIndex);

        break;
      }  /* END switch (SIndex) */

    MUSNPrintF(&BPtr,&BSpace,"%-26s %8d\n\n",
      "Total Samples:",
      T->JC);
    }    /* END if (DFormat != mfmXML) */
  else
    {
    int index;

    char  tmpBuf[MMAX_BUFFER];

    MXMLSetAttr(ME,"Samples",(void *)&T->JC,mdfInt);

    if (SBuffer != NULL)
      {
      MUSNInit(&BPtr,&BSpace,SBuffer,BufSize);

      MXMLToString(ME,BPtr,BSpace,NULL,TRUE);

      MUSNUpdate(&BPtr,&BSpace);

      if (bmisset(SDFlagBM,mcmPeer))
        {
        MUSNInit(&BPtr,&BSpace,tmpBuf,sizeof(tmpBuf));

        MUSNPrintF(&BPtr,&BSpace,"<Data>%s",
          SBuffer);

        for (index = 0;index < MSched.M[mxoRM];index++)
          {
          if (MRM[index].Name[0] == '\0')
            break;

          if (MRMIsReal(&MRM[index]) == FALSE)
            continue;

          if (!bmisset(&MRM[index].Flags,mrmifLoadBalanceQuery))
            continue;

          /* NOTE:  who populates 'S'?  What format is it in? */

          MUSNCat(&BPtr,&BSpace,(char *)MRM[index].S);
          }

        MUSNCat(&BPtr,&BSpace,"</Data>");

        MUStrCpy(SBuffer,tmpBuf,BufSize);
        }  /* END if (bmisset(SDFlagBM,mcmPeer)) */
      }    /* END if (SBuffer != NULL) */

    if (MEP != NULL)
      *MEP = ME;
    else
      MXMLDestroyE(&ME);
    }  /* END else (DFormat != mfmXML) */ 

  return(SUCCESS);
  }  /* END MStatBuildMatrix() */





/**
 * Generate data for cell in stats matrix.
 *
 * @see MStatBuildMatrix()
 *
 * @param SIndex (I) stat type
 * @param G (I) grid/cell
 * @param R (I) row
 * @param C (I) column
 * @param T (I) total
 * @param TaskVal (I)
 * @param TimeVal (I)
 * @param DFlagBM (I) [bitmap of mXML,mMatrix]
 */

char *MStatGetGrid(

  enum MMStatTypeEnum SIndex,
  must_t *G,
  must_t *R,
  must_t *C,
  must_t *T,
  int     TaskVal,
  long    TimeVal,
  mbitmap_t *DFlagBM)

  {
  static char Line[MMAX_NAME];

  char doubleline[MMAX_LINE];
  char intline[MMAX_LINE];
  char intonly[MMAX_LINE];
  char defline[MMAX_LINE];
  char nullline[MMAX_LINE];
  char stringonly[MMAX_LINE];

  const char *FName = "MStatGetGrid";

  MDB(5,fSTAT) MLog("%s(%d,G,R,C,T,%d,%ld,%d)\n",
    FName,
    SIndex,
    TaskVal,
    TimeVal,
    DFlagBM);

  if (bmisset(DFlagBM,mXML))
    {
    strcpy(doubleline,"%.2f");
    strcpy(intline,   "%d");
    strcpy(intonly,   "%d");
    strcpy(stringonly,"%s");
    strcpy(defline,   "0");
    strcpy(nullline,  "0");
    }
  else if (bmisset(DFlagBM,mMatrix))
    {
    strcpy(doubleline,"    %7.2f   ");
    strcpy(intline,   "    %7d  ");
    strcpy(intonly,   "   %8d   ");
    strcpy(stringonly,"   %8s   ");
    strcpy(defline,   "          0   ");
    strcpy(nullline,  "          0   ");
    }
  else
    {
    strcpy(doubleline,"[%7.2f %4d]");
    strcpy(intline,   "[%7d %4d]");
    strcpy(intonly,   "[  %8d  ]");
    strcpy(stringonly,"[  %8s  ]");
    strcpy(defline,   "[  ????????  ]");
    strcpy(nullline,  "[------------]");
    }

  if ((G->JC == 0) && (SIndex != mgstEstStartTime))
    {
    strcpy(Line,nullline);

    return(Line);
    }

  switch (SIndex)
    {
    case mgstAvgXFactor:

      sprintf(Line,(const char *)doubleline,
        G->XFactor / G->JC,
        G->JC);

      break;

    case mgstMaxXFactor:

      sprintf(Line,(const char *)doubleline,
        G->MaxXFactor,
        G->JC);

      break;

    case mgstAvgQTime:

      sprintf(Line,(const char *)doubleline,
        (double)(G->TotalQTS) / G->JC / 3600.0,
        G->JC);

      break;

    case mgstAvgBypass:

      sprintf(Line,doubleline,
        (double)G->Bypass / G->JC,
        G->JC);

      break;

    case mgstMaxBypass:

      sprintf(Line,intline,
        G->MaxBypass,
        G->JC);

      break;

    case mgstJobCount:

      sprintf(Line,intonly,
        G->JC);

      break;

    case mgstPSRequest:

      sprintf(Line,doubleline,
        G->PSRequest / T->PSRequest * 100.0,
        G->JC);

      break;

    case mgstPSRun:

      sprintf(Line,doubleline,
        G->PSRun / T->PSRun * 100.0,
        G->JC);

      break;

    case mgstWCAccuracy:

      sprintf(Line,doubleline,
        G->PSRun / G->PSRequest * 100,
        G->JC);

      break;

    case mgstBFCount:

      sprintf(Line,doubleline,
        (double)G->BFCount / G->JC * 100.0,
        G->JC);

      break;

    case mgstBFPSRun:

      sprintf(Line,doubleline,
        G->BFPSRun / G->PSRun * 100.0,
        G->JC);

      break;

    case mgstQOSDelivered:

      sprintf(Line,doubleline,
        (double)G->QOSSatJC / G->JC * 100.0,
        G->JC);

      break;

    case mgstEstStartTime:

      {
      long StartTime;

      mjob_t *J = NULL;

      MJobMakeTemp(&J);

      strcpy(J->Name,"startestimate");

      J->Request.TC     = TaskVal;
      J->SpecWCLimit[0] = TimeVal;

      J->Req[0]->TaskCount = TaskVal;

      StartTime = MSched.Time;

      if (MJobGetEStartTime(
           J,
           NULL,
           NULL,
           NULL,
           NULL,
           &StartTime,
           NULL,
           TRUE,
           FALSE,
           NULL) == FAILURE)
        {
        StartTime = MMAX_TIME;
        }

     if (!bmisset(DFlagBM,mXML) && (StartTime == MMAX_TIME))
        {
        strcpy(Line,nullline);
        }
      else if (bmisset(DFlagBM,mXML))
        {
        sprintf(Line,intonly,
          (int)StartTime);
        }
      else
        {
        char TString[MMAX_LINE];

        MULToTString(StartTime - MSched.Time,TString);

        sprintf(Line,stringonly,
          TString);
        }

      MJobFreeTemp(&J); 
      }  /* END BLOCK */

      break;

    default:

      strcpy(Line,defline);

      MDB(3,fSTAT) MLog("ALERT:    stat type %d not handled\n",
        SIndex);

      break;
    }  /* END switch (SIndex) */

  return(Line);
  }  /* END MStatGetGrid() */
 




/**
 * Adjust stats/usage associated with addition/removal of specified job to 
 * the eligible job list.
 *
 * NOTE:  adjust global eligible job count and idle job limits (IP) 
 *
 * jobs are given the eligible flag in MStatUpdateBacklog(), however, for iteration 0
 * MStatUpdateBacklog() is not called and the flag must be set here when count is 1
 *
 * @see MJobStart() - parent - remove job from eligible list as it is started
 * @see MJobSetHold() - parent - remove job from eligible list
 * @see MQueueSelectAllJobs() - parent - add job to eligible list
 * @see MQueueAddAJob() - peer - add job to active list
 *
 * @param J (I)
 * @param P (I) [optional]
 * @param Count (I) [1 or -1]
 */

int MStatAdjustEJob(

  mjob_t *J,     /* I */
  mpar_t *P,     /* I (optional) */
  int     Count) /* I (1 or -1) */

  {
  /* NOTE: DRW removed double-counting checks 5/6/08 */
  /*       double-counting checks prevented jobs from being counted as eligible */
  /*       and idle job usage limits were broken as well as idle stats */

  if (bmisset(&J->IFlags,mjifIsEligible) && (Count < 0))
    {
    bmunset(&J->IFlags,mjifIsEligible);
    }
  else if (!bmisset(&J->IFlags,mjifIsEligible) && (Count > 0)) 
    {
    bmset(&J->IFlags,mjifIsEligible);
    }

  MDB(3,fSTAT) MLog("INFO:     adjusting usage stats for job '%s' (%s)\n",
    J->Name,
    (Count > 0) ? "add" : "remove");

  MStat.EligibleJobs += Count;

  /* global partition idle stats */

  MPolicyAdjustUsage(J,NULL,NULL,mlIdle,MPar[0].L.IdlePolicy,mxoALL,Count,NULL,NULL);

  /* job idle stats */

  MPolicyAdjustUsage(J,NULL,P,mlIdle,NULL,mxoALL,Count,NULL,NULL);

  if (J->Credential.Q != NULL)
    {
    J->Credential.Q->MaxXF = MAX(J->Credential.Q->MaxXF,J->EffXFactor);
    J->Credential.Q->MaxQT = MAX(J->Credential.Q->MaxQT,J->EffQueueDuration);
 
    /* NOTE: QoS backlog maintained w/in Q->L.IP->Usage[mptMaxPS][0] */
    }  /* END if (J->Cred.Q != NULL) */

  return(SUCCESS);
  }  /* END MStatAdjustEJob() */




/**
 * Complete stats records and close stats files.
 *
 */

int MStatShutdown(void)

  {
  if ((MStat.eventfp != NULL) &&
      (MStat.eventfp != stderr) &&
      (MStat.eventfp != mlog.logfp))
    {
    fclose(MStat.eventfp);
    }

  MStat.eventfp = NULL;

  return(SUCCESS);
  }  /* END MStatShutdown() */





/**
 * Report Scheduler/System attributes via XML string.
 *
 * @see MSysToXML()
 *
 * @param S (I)
 * @param Buf (O)
 * @param IsCP (I)
 */

int  MSysToString(

  msched_t  *S,    /* I */
  mstring_t *Buf,  /* I/O */
  mbool_t    IsCP) /* I */

  {
  enum MSysAttrEnum CPAList[] = {
    msysaAPServerHost,
    msysaAPServerTime,
    /* msysaAvgTQueuedPH, */   /* should not be checkpointed - startup stat only */
    msysaCPInitTime,
    msysaFSInitTime,
    msysaStatInitTime,
    msysaVersion,
    msysaNONE };

  enum MSysAttrEnum AList[] = {
    msysaIterationCount,
    msysaPresentTime,
    msysaStatInitTime,
    msysaVersion,
    msysaNONE };

  mxml_t *E = NULL;

  if ((S == NULL) || (Buf == NULL))
    {
    return(FAILURE);
    }

  MSysToXML(
    S,
    &E,
    (IsCP == TRUE) ? (enum MSysAttrEnum *)CPAList : (enum MSysAttrEnum *)AList,
    NULL,
    FALSE,
    0);

  MXMLToMString(E,Buf,NULL,TRUE);
 
  MXMLDestroyE(&E);
 
  return(SUCCESS);
  }  /* END MSysToString() */





/**
 * Report Scheduler/System attributes via XML.
 *
 * @see MSysToString()
 *
 * @param S            (I)
 * @param EP           (O)
 * @param SAList       (I)
 * @param SCList       (I)
 * @param ClientOutput (I)
 * @param Mode         (I)
 */

int MSysToXML(

  msched_t           *S,
  mxml_t            **EP,
  enum MSysAttrEnum  *SAList,
  enum MXMLOTypeEnum *SCList,
  mbool_t             ClientOutput,
  int                 Mode)

  {
  enum MSysAttrEnum  *AList;
  enum MXMLOTypeEnum *CList;

  int  aindex;
  int  cindex;

  void *C;

  mxml_t *XC = NULL;

  char tmpString[MMAX_LINE];

  enum MSysAttrEnum DAList[] = {
    msysaVersion,
    msysaNONE };

  enum MXMLOTypeEnum DCList[] = {
    mxoxStats,
    mxoxLimits,
    mxoxFS,
    mxoNONE };

  if (SAList != NULL)
    AList = SAList;
  else
    AList = DAList;
 
  if (SCList != NULL)
    CList = SCList;
  else
    CList = DCList;
 
  MXMLCreateE(EP,(char *)MXO[mxoSys]);
 
  for (aindex = 0;AList[aindex] != msysaNONE;aindex++)
    {
    if ((MSysAToString(S,AList[aindex],tmpString,sizeof(tmpString),mfmXML) == FAILURE) ||
        (tmpString[0] == '\0'))
      {
      continue;
      }
 
    MXMLSetAttr(*EP,(char *)MSysAttr[AList[aindex]],tmpString,mdfString);
    }  /* END for (aindex) */
 
  for (cindex = 0;CList[cindex] != mxoNONE;cindex++)
    {
    XC = NULL;
 
    if ((MOGetComponent((void *)S,mxoSched,&C,CList[cindex]) == FAILURE) ||
        (MOToXML(C,CList[cindex],NULL,ClientOutput,&XC) == FAILURE))
      {
      continue;
      }
 
    MXMLAddE(*EP,XC);
    }  /* END for (cindex) */

  return(SUCCESS);
  }  /* END MSysToXML() */




/**
 * Report sys object attribute as string.
 *
 * @param S (I)
 * @param AIndex (I)
 * @param Buf (O)
 * @param BufSize (I)
 * @param Mode (I)
 */

int MSysAToString(

  msched_t            *S,       /* I */
  enum MSysAttrEnum    AIndex,  /* I */
  char                *Buf,     /* O */
  int                  BufSize, /* I */       
  enum MFormatModeEnum Mode)    /* I */
 
  {
  if ((S == NULL) || (Buf == NULL))
    {
    return(FAILURE);
    }

  Buf[0] = '\0';
 
  switch (AIndex)
    {
    case msysaActivePS:

      if (MPar[0].L.ActivePolicy.Usage[mptMaxPS][0] > 0)
        {
        sprintf(Buf,"%lu",
          MPar[0].L.ActivePolicy.Usage[mptMaxPS][0]);
        }

      break;

    case msysaAPServerHost:

      if (MSched.MAPServerHost[0][0] == '\0')
        break;

      {
      int aindex;

      char   *BPtr = NULL;
      int     BSpace = 0;

      MUSNInit(&BPtr,&BSpace,Buf,BufSize);

      for (aindex = 0;aindex < MMAX_APSERVER;aindex++)
        {
        if (MSched.MAPServerHost[aindex][0] == '\0')
          break;

        if (aindex > 0)
          {
          MUSNPrintF(&BPtr,&BSpace,",%s",
            MSched.MAPServerHost[aindex]);
          }
        else
          {
          MUSNPrintF(&BPtr,&BSpace,"%s",
            MSched.MAPServerHost[aindex]);
          }
        }  /* END for (aindex) */
      }    /* END BLOCK */

      break;

    case msysaAPServerTime:

      if (MSched.MAPServerTime[0] == 0)
        break;

      {
      int aindex;

      char   *BPtr = NULL;
      int     BSpace = 0;

      MUSNInit(&BPtr,&BSpace,Buf,BufSize);

      for (aindex = 0;aindex < MMAX_APSERVER;aindex++)
        {
        if (MSched.MAPServerHost[aindex][0] == '\0')
          break;

        if (aindex > 0)
          {
          MUSNPrintF(&BPtr,&BSpace,",%ld",
            MSched.MAPServerTime[aindex]);
          }
        else
          {
          MUSNPrintF(&BPtr,&BSpace,"%ld",
            MSched.MAPServerTime[aindex]);
          }
        }  /* END for (aindex) */
      }    /* END BLOCK */

      break;

    case msysaAvgTActivePH:

      sprintf(Buf,"%lu",
        (MSched.Iteration > 0) ? MStat.TActivePH / MSched.Iteration : 0);

      break;

    case msysaAvgTQueuedPH:

      sprintf(Buf,"%lu",
        (MSched.Iteration > 0) ? MStat.TQueuedPH / MSched.Iteration : 0);

      break;

    case msysaCPInitTime:

      sprintf(Buf,"%ld",
        MCP.CPInitTime);

      break;

    case msysaFSInitTime:

      sprintf(Buf,"%ld",
        MPar[0].FSC.FSInitTime);

      break;

    case msysaIdleMem:

      sprintf(Buf,"%d",
        MIN(MPar[0].ARes.Mem,(MPar[0].UpRes.Mem - MPar[0].DRes.Mem)));

      break;

    case msysaIdleNodeCount:

      if (MPar[0].IdleNodes > 0)
        {
        sprintf(Buf,"%d",
          MPar[0].IdleNodes);
        }

      break;

    case msysaIdleProcCount:

      sprintf(Buf,"%d",
        MIN(MPar[0].ARes.Procs,(MPar[0].UpRes.Procs - MPar[0].DRes.Procs)));

      break;

    case msysaIterationCount:

      sprintf(Buf,"%d",
        MSched.Iteration);

      break;

    case msysaQueuePS:

      if (MPar[0].L.IdlePolicy->Usage[mptMaxPS][0] > 0)
        {
        sprintf(Buf,"%lu",
          MPar[0].L.IdlePolicy->Usage[mptMaxPS][0]);
        }

      break;

    case msysaPresentTime:

      sprintf(Buf,"%ld",
        MSched.Time);

      break;

    case msysaRMPollInterval:

      if (MSched.MaxRMPollInterval > 0)
        {
        sprintf(Buf,"%ld",
          MSched.MaxRMPollInterval);
        }

      break;

    case msysaSCompleteJobCount:

      if (MStat.SuccessfulJobsCompleted > 0)
        {
        sprintf(Buf,"%d",
          MStat.SuccessfulJobsCompleted);
        }

      break;

    case msysaStatInitTime:

      sprintf(Buf,"%ld",
        MStat.InitTime);

      break;

    case msysaUpNodes:

      if (MPar[0].UpNodes > 0)
        {
        sprintf(Buf,"%d",
          MPar[0].UpNodes);
        }

      break;

    case msysaUpProcs:

      if (MPar[0].UpRes.Procs > 0)
        {
        sprintf(Buf,"%d",
          MPar[0].UpRes.Procs);
        }

      break;

    case msysaUpMem:

      if (MPar[0].UpRes.Mem > 0)
        {
        sprintf(Buf,"%d",
          MPar[0].UpRes.Mem);
        }

      break;

    case msysaVersion:

      if (S->Version != NULL) 
        {
        sprintf(Buf,"%s",
          S->Version);
        }

      break;

    default:

      /* NOT HANDLED */

      return(FAILURE);

      /*NOTREACHED*/

      break;
    }  /* END switch (AIndex) */
  
  return(SUCCESS);
  }  /* END MSysAToString() */




/**
 * Set system attribute.
 *
 * @param S (I) [modified]
 * @param AIndex (I)
 * @param Value (I)
 * @param Format (I)
 * @param Mode (I)
 */

int MSysSetAttr(

  msched_t                *S,      /* I (modified) */
  enum MSysAttrEnum        AIndex, /* I */
  void                   **Value,  /* I */
  enum MDataFormatEnum     Format, /* I */
  enum MObjectSetModeEnum  Mode)   /* I */
 
  {
  if (S == NULL)
    {
    return(FAILURE);
    }

  switch (AIndex)
    {
    case msysaAPServerHost:

      if (Format == mdfString)
        {
        char *ptr;
        char *TokPtr = NULL;

        int   hindex;

        /* FORMAT:  HOST[,HOST]... */

        hindex = 0;

        ptr = MUStrTok((char *)Value,", \t\n",&TokPtr);

        while (ptr != NULL)
          {
          if (hindex >= MMAX_APSERVER)
            break;

          MUStrCpy(MSched.MAPServerHost[hindex],ptr,sizeof(MSched.MAPServerHost[0]));

          hindex++;

          ptr = MUStrTok(NULL,", \t\n",&TokPtr);
          }
        }    /* END if (Format == mdfString) */

      break;

    case msysaAPServerTime:

      if (Format == mdfString)
        {
        char *ptr;
        char *TokPtr = NULL;

        int   hindex;

        /* FORMAT:  HOST[,HOST]... */

        hindex = 0;

        ptr = MUStrTok((char *)Value,", \t\n",&TokPtr);

        while (ptr != NULL)
          {
          if (hindex >= MMAX_APSERVER)
            break;

          MSched.MAPServerTime[hindex] = strtol(ptr,NULL,10);

          hindex++;

          ptr = MUStrTok(NULL,", \t\n",&TokPtr);
          }
        }    /* END if (Format == mdfString) */

      break;

    case msysaAvgTQueuedPH:

      /* NYI - should not be implemented */

      break;

    case msysaCPInitTime:

      MCP.CPInitTime = strtol((char *)Value,NULL,10);

      break;

    case msysaPresentTime:

      S->Time = strtol((char *)Value,NULL,10);

      break;

    case msysaStatInitTime:

      MStat.InitTime = strtol((char *)Value,NULL,10);      

      break;

    case msysaFSInitTime:

      MPar[0].FSC.FSInitTime = strtol((char *)Value,NULL,10);

      break;

    case msysaVersion:

      /* NOTE:  checkpoint version ignored.  use built-in version only */

      /* NO-OP */

      break;

    default:

      /* NOT HANDLED */

      return(FAILURE);

      /*NOTREACHED*/

      break;
    }  /* END switch (AIndex) */

  return(SUCCESS);
  }  /* END MSysSetAttr() */





/**
 *
 *
 * @param L (I)
 * @param EP (O)
 * @param SAList (I) [optional]
 */

int MLimitToXML(

  mcredl_t             *L,      /* I */
  mxml_t              **EP,     /* O */
  enum MLimitAttrEnum  *SAList) /* I (optional) */

  {
  const enum MLimitAttrEnum DAList[] = {
    mlaAJobs,
    mlaAMem,
    mlaAProcs,
    mlaAPS,
    mlaNONE };

  int  aindex;
 
  const enum MLimitAttrEnum *AList;
 
  char tmpString[MMAX_LINE];
 
  if ((L == NULL) || (EP == NULL))
    {
    return(FAILURE);
    }
 
  if (SAList != NULL)
    AList = SAList;
  else
    AList = (enum MLimitAttrEnum *)DAList;

  MXMLCreateE(EP,(char *)MXO[mxoxLimits]);
 
  for (aindex = 0;AList[aindex] != mlaNONE;aindex++)
    {
    if ((MLimitAToString(L,AList[aindex],tmpString,0) == FAILURE) ||
        (tmpString[0] == '\0'))
      {
      continue;
      }
 
    MXMLSetAttr(
      *EP,
      (char *)MLimitAttr[AList[aindex]],
      tmpString,
      mdfString);
    }  /* END for (aindex) */

  return(SUCCESS);
  }  /* END MLimitToXML() */




/**
 *
 *
 * @param L
 * @param AIndex
 * @param Buf
 * @param Format
 */

int MLimitAToString(

  mcredl_t *L,
  enum MLimitAttrEnum AIndex,
  char     *Buf,
  int       Format)

  {
  if ((L == NULL) || (Buf == NULL))
    {
    return(FAILURE);
    }

  Buf[0] = '\0';
 
  switch (AIndex)
    {
    case mlaAJobs:

      if (L->ActivePolicy.Usage[mptMaxJob][0] > 0)
        { 
        sprintf(Buf,"%lu",
          L->ActivePolicy.Usage[mptMaxJob][0]);
        }
 
      break;

    case mlaAMem:

      if (L->ActivePolicy.Usage[mptMaxMem][0] > 0)
        {
        sprintf(Buf,"%lu",
          L->ActivePolicy.Usage[mptMaxMem][0]);
        }

      break;

    case mlaANodes:

      if (L->ActivePolicy.Usage[mptMaxNode][0] > 0)
        {
        sprintf(Buf,"%lu",
          L->ActivePolicy.Usage[mptMaxNode][0]);
        }

      break;
 
    case mlaAProcs:

      if (L->ActivePolicy.Usage[mptMaxProc][0] > 0)
        {
        sprintf(Buf,"%lu",
          L->ActivePolicy.Usage[mptMaxProc][0]);
        }
 
      break;
 
    case mlaAPS:

      if (L->ActivePolicy.Usage[mptMaxPS][0] > 0)
        {
        sprintf(Buf,"%lu",
          L->ActivePolicy.Usage[mptMaxPS][0]);
        }
 
      break;

    case mlaSLAJobs:

      if (L->ActivePolicy.SLimit[mptMaxJob][0] > 0)
        {
        sprintf(Buf,"%ld",
          L->ActivePolicy.SLimit[mptMaxJob][0]);
        }

      break;

    case mlaSLANodes:

      if (L->ActivePolicy.SLimit[mptMaxNode][0] > 0)
        {
        sprintf(Buf,"%ld",
          L->ActivePolicy.SLimit[mptMaxNode][0]);
        }

      break;

    case mlaSLAProcs:

      if (L->ActivePolicy.SLimit[mptMaxProc][0] > 0)
        {
        sprintf(Buf,"%ld",
          L->ActivePolicy.SLimit[mptMaxProc][0]);
        }

      break;

    case mlaSLAPS:

      if (L->ActivePolicy.SLimit[mptMaxPS][0] > 0)
        {
        sprintf(Buf,"%ld",
          L->ActivePolicy.SLimit[mptMaxPS][0]);
        }

      break;

    case mlaSLIJobs:

      if ((L->IdlePolicy != NULL) && (L->IdlePolicy->SLimit[mptMaxJob][0] > 0))
        {
        sprintf(Buf,"%ld",
          L->IdlePolicy->SLimit[mptMaxJob][0]);
        }

      break;

    case mlaSLINodes:

      if ((L->IdlePolicy != NULL) && (L->IdlePolicy->SLimit[mptMaxNode][0] > 0))
        {
        sprintf(Buf,"%ld",
          L->IdlePolicy->SLimit[mptMaxNode][0]);
        }

      break;

    case mlaHLAJobs:

      if (L->ActivePolicy.HLimit[mptMaxJob][0] > 0)
        {
        sprintf(Buf,"%ld",
          L->ActivePolicy.HLimit[mptMaxJob][0]);
        }

      break;

    case mlaHLANodes:

      if (L->ActivePolicy.HLimit[mptMaxNode][0] > 0)
        {
        sprintf(Buf,"%ld",
          L->ActivePolicy.HLimit[mptMaxNode][0]);
        }

      break;

    case mlaHLAProcs:

      if (L->ActivePolicy.HLimit[mptMaxProc][0] > 0)
        {
        sprintf(Buf,"%ld",
          L->ActivePolicy.HLimit[mptMaxProc][0]);
        }

      break;

    case mlaHLAPS:

      if (L->ActivePolicy.HLimit[mptMaxPS][0] > 0)
        {
        sprintf(Buf,"%ld",
          L->ActivePolicy.HLimit[mptMaxPS][0]);
        }

      break;

    case mlaHLIJobs:

      if ((L->IdlePolicy != NULL) && (L->IdlePolicy->HLimit[mptMaxJob][0] > 0))
        {
        sprintf(Buf,"%ld",
          L->IdlePolicy->HLimit[mptMaxJob][0]);
        }

      break;

    case mlaHLINodes:

      if ((L->IdlePolicy != NULL) && (L->IdlePolicy->HLimit[mptMaxNode][0] > 0))
        {
        sprintf(Buf,"%ld",
          L->IdlePolicy->HLimit[mptMaxNode][0]);
        }

      break;

    default:

      /* NOT HANDLED */

      return(FAILURE);

      /*NOTREACHED*/
 
      break;
    }  /* END switch (AIndex) */
 
  return(SUCCESS);
  }  /* END MLimitAToString() */


 
 
 
/**
 *
 *
 * @param L (I) [modified]
 * @param E (I)
 */

int MLimitFromXML(
 
  mcredl_t *L,  /* I (modified) */
  mxml_t   *E)  /* I */
 
  {
  int aindex;

  enum MLimitAttrEnum saindex;
 
  if ((L == NULL) || (E == NULL))
    {
    return(FAILURE);
    }
 
  /* NOTE:  do not initialize.  may be overlaying data */
 
  for (aindex = 0;aindex < E->ACount;aindex++)
    {
    saindex = (enum MLimitAttrEnum)MUGetIndex(E->AName[aindex],MLimitAttr,FALSE,0);
 
    if (saindex == mlaNONE)
      continue;
 
    MLimitSetAttr(L,saindex,(void **)E->AVal[aindex],mdfString,mSet);
    }  /* END for (aindex) */
 
  return(SUCCESS);
  }  /* END MLimitFromXML() */

 


/**
 *
 *
 * @param L (I) [modified]
 * @param AIndex (I)
 * @param Value (I)
 * @param Format (I)
 * @param Mode (I)
 */

int MLimitSetAttr(
 
  mcredl_t  *L,      /* I (modified) */
  enum MLimitAttrEnum AIndex, /* I */
  void     **Value,  /* I */
  int        Format, /* I */
  int        Mode)   /* I */
 
  {
  if (L == NULL)
    {
    return(FAILURE);
    }
 
  switch (AIndex)
    {
    case mlaAJobs:
 
      L->ActivePolicy.Usage[mptMaxJob][0] = (int)strtol((char *)Value,NULL,10);
 
      break;

    case mlaAMem:

      L->ActivePolicy.Usage[mptMaxMem][0] = (int)strtol((char *)Value,NULL,10);

      break;

    case mlaAProcs:

      L->ActivePolicy.Usage[mptMaxProc][0] = (int)strtol((char *)Value,NULL,10);        
 
      break;
 
    case mlaAPS:

      L->ActivePolicy.Usage[mptMaxPS][0] = strtoul((char *)Value,NULL,10);     
 
      break;

    default:

      /* not handled */

      return(FAILURE);

      /*NOTREACHED*/

      break;
    }  /* switch (AIndex) */

  return(SUCCESS);
  }  /* MLimitSetAttr() */




/**
 *
 * @param ObjectType (I)
 * @param AttrIndex  (I)
 */

mbool_t MStatIsProfileAttr(

  enum MStatObjectEnum ObjectType,
  int                  AttrIndex)

  {

  switch (ObjectType)
    {
    case msoCred:

      switch ((enum MStatAttrEnum)AttrIndex)
        {

        case mstaIStatCount:
        case mstaIStatDuration:
        case mstaGCEJobs:
        case mstaGCIJobs:
        case mstaGCAJobs:
        case mstaGPHAvl:
        case mstaGPHUtl:
        case mstaGPHDed:
        case mstaGPHSuc:
        case mstaGMinEff:
        case mstaGMinEffIteration:
        case mstaTPHPreempt:
        case mstaTJPreempt:
        case mstaTJEval:
        case mstaSchedDuration:
          return(FALSE);

        default:
          return(TRUE);

          break;
        }
      break;

    case msoNode:

      switch ((enum MNodeStatTypeEnum)AttrIndex)
      {

        case mnstIStatCount:
        case mnstIStatDuration:

          return(FALSE);

        default:

          return (TRUE);
      }
      break;

    default:
      return(FALSE);

    }
  } /* END MStatIsProfileAttr() */



/**
 *
 */

int MStatIsProfileStat(

  void *Stat,
  enum MStatObjectEnum ObjectType)

  {

  switch (ObjectType)
    {
    case msoCred:

      return ((must_t *)Stat)->IsProfile;

      break;

    case msoNode:

      return ((mnust_t *)Stat)->IsProfile;

      break;

    default:
      return(FALSE);
    }

  return (SUCCESS);
  } /* END MStatIsProfileStat() */






/**
 * Report cred stat structure as string.
 *
 * NOTE: this routine should not be called by client commands, if
 *       it is then it needs to add a parameter to display the XML
 *       correctly for statistics (mcoReportProfileStatsAsChild)
 *
 * @see MStatToXML() - child
 * @see MStatFromString() - peer
 *
 * @param S (I)
 * @param Buf (O) [minsize=MMAX_BUFFER]
 * @param SAList (I) [optional]
 */

int MStatToString(

  must_t             *S,      /* I */
  char               *Buf,    /* O (minsize=MMAX_BUFFER) */
  enum MStatAttrEnum *SAList) /* I (optional) */

  {
  mxml_t *E = NULL;

  if (Buf != NULL)
    Buf[0] = '\0';

  if ((S == NULL) || (Buf == NULL))
    {
    return(FAILURE);
    }

  MStatToXML((void *)S,msoCred,&E,FALSE,SAList);

  MXMLToString(E,Buf,MMAX_BUFFER,NULL,TRUE);

  MXMLDestroyE(&E);

  return(SUCCESS);
  }  /* END MStatToString() */





/**
 * Update stat structure from XML string.
 *
 * @see MStatFromXML()
 *
 * @param Buf (I)
 * @param S (I) [modified]
 * @param SType (I)
 */

int MStatFromString(

  char                 *Buf, /* I */
  void                 *S,   /* I (modified) */
  enum MStatObjectEnum  SType)  /* I */

  {
  mxml_t *E = NULL;

  int rc;

  if ((Buf == NULL) || (S == NULL))
    {
    return(FAILURE);
    }

  rc = MXMLFromString(&E,Buf,NULL,NULL);

  if (rc == SUCCESS)
    {
    rc = MStatFromXML(S,SType,E);
    }

  MXMLDestroyE(&E);

  if (rc == FAILURE)
    { 
    return(FAILURE);
    }
  
  return(SUCCESS);
  }  /* END MStatFromString() */


/**
 * Translate range string to effective start and end time.
 *
 * NOTE:  supports both absolute and relative time specification.
 *
 * @see MUGetStarttime() - child
 * @see MUTimeFromString() - child
 *
 * @param Range  (I)
 * @param STime  (O)
 * @param ETime  (O)
 */

int MStatGetRange(

  char *Range, /* I */
  long *STime, /* O */
  long *ETime) /* O */

  {
  char tmpLine[MMAX_LINE];

  char *ptr;
  char *TokPtr;

  mbool_t EndTimeIsNow = TRUE;

  if ((Range == NULL) || 
      (Range[0] == '\0') ||
      (STime == NULL) ||
      (ETime == NULL))
    {
    return(FAILURE);
    }

  *STime = -1;
  *ETime = -1;

  MUGetPeriodRange(MSched.Time,0,0,mpDay,STime,NULL,NULL);

  *ETime = (long)MSched.Time;

  /* OptString Format:  time:<STIME>[,<ETIME>] */

  MUStrCpy(tmpLine,Range,sizeof(tmpLine));

  ptr = MUStrTok(tmpLine,",",&TokPtr);

  if (ptr != NULL)
    {
    *STime = MUTimeFromString(ptr);

    if (*STime < (10*MCONST_YEARLEN))
      {
      /* Assume STime is relative (either negative or in the 1970s) */
      /* convert relative time to absolute time */

      *STime = MSched.Time + *STime;
      }
    }
   
  if (*STime > (long)MSched.Time)
    *STime = 0;
  else
    *STime = MUGetStarttime(MStat.P.IStatDuration,*STime);

  if (*STime <= 0)
    {
    MUGetPeriodRange(MSched.Time,0,0,mpDay,STime,NULL,NULL);
    }

  ptr = MUStrTok(NULL,",",&TokPtr);

  if (ptr != NULL)
    {
    EndTimeIsNow = FALSE;

    *ETime = MUTimeFromString(ptr);

    if (*ETime < (10*MCONST_YEARLEN))
      {
      /* Assume ETime is relative (either negative or in the 1970s) */
      /* convert relative time to absolute time */

      *ETime = MSched.Time + *ETime;
      }
    }

  if ((*ETime > (long)MSched.Time) || (*ETime < 0))
    {
    EndTimeIsNow = TRUE;

    *ETime = MSched.Time;
    }

  *ETime = MUGetStarttime(MStat.P.IStatDuration,*ETime);

  if ((EndTimeIsNow == TRUE) && (*ETime < (long)MSched.Time))
    *ETime += MStat.P.IStatDuration - 1;

  return(SUCCESS);
  }  /* END MStatGetRange() */





/**
 * Merge two must_t structures (used for stat profiles).
 *
 * @see MStatWritePeriodPStats()
 * @see MStatMergeXML()
 *
 * @param SS1 (I)
 * @param SS2 (I)
 * @param SSD (O)
 * @param StatType (I)
 */

int MStatMerge(

  void                  *SS1,       /* I */
  void                  *SS2,       /* I */
  void                  *SSD,       /* O */
  enum MStatObjectEnum   StatType)  /* I */

  {
  if ((SS1 == NULL) || (SS2 == NULL) || (SSD == NULL))
    {
    return(FAILURE);
    }

  switch (StatType)
    {
    case msoCred:

      {
      int aindex;

      must_t SD;

      must_t *S1 = (must_t *)SS1;
      must_t *S2 = (must_t *)SS2;

      memset(&SD,0,sizeof(must_t));

/*
      long  StartTime;              
      long  Duration;
      int   IterationCount;
*/

      SD.Duration       = S2->Duration;
      SD.IterationCount = S1->IterationCount;
      
      SD.JC        = S1->JC + S2->JC;                  
      SD.SubmitJC  = S1->SubmitJC + S2->SubmitJC;      /* jobs submitted                                */
      SD.SuccessJC = S1->SuccessJC + S2->SuccessJC;    /* jobs successfully completed                   */
      SD.RejectJC  = S1->RejectJC + S2->RejectJC;      /* jobs submitted that have been rejected        */
      SD.QOSSatJC  = S1->QOSSatJC + S2->QOSSatJC;      /* jobs that met requested QOS                   */
      SD.JAllocNC  = S1->JAllocNC + S2->JAllocNC;      /* total nodes allocated to jobs                 */

      SD.TotalQTS = S1->TotalQTS + S2->TotalQTS;       /* job queue time                                */
      SD.MaxQTS   = MAX(S1->MaxQTS,S2->MaxQTS);

      SD.TQueuedJC = S1->TQueuedJC + S2->TQueuedJC;
      SD.TQueuedPH = S1->TQueuedPH + S2->TQueuedPH;

      SD.TPC = S1->TPC + S2->TPC;
      SD.TNC = S1->TNC + S1->TNC;

      SD.PSRequest = S1->PSRequest + S2->PSRequest;                        /* requested procseconds completed               */
      SD.SubmitPHRequest = S1->SubmitPHRequest + S2->SubmitPHRequest;      /* requested prochours submitted                 */
      SD.PSRun = S1->PSRun + S2->PSRun;                                    /* executed procsecond                           */
      SD.PSRunSuccessful = S1->PSRunSuccessful + S2->PSRunSuccessful;      /* successfully completed procseconds            */
      SD.TQueuedPH   = S1->TQueuedPH + S2->TQueuedPH;          /* total queued prochours                        */
      SD.TotalRequestTime = S1->TotalRequestTime + S2->TotalRequestTime;   /* total requested job walltime                  */
      SD.TotalRunTime = S1->TotalRunTime + S2->TotalRunTime;               /* total execution job walltime                  */
      SD.PSDedicated  = S1->PSDedicated + S2->PSDedicated;                 /* procseconds dedicated                         */
      SD.ActiveNC     = S1->ActiveNC + S2->ActiveNC;                       /* total active nodes                            */
      SD.ActivePC     = S1->ActivePC + S2->ActivePC;                       /* total active procs                            */
      SD.PSUtilized   = S1->PSUtilized  + S2->PSUtilized;                  /* procseconds actually used                     */
      SD.DSAvail      = S1->DSAvail + S2->DSAvail;                         /* disk-seconds available                        */
      SD.DSUtilized   = S1->DSUtilized + S2->DSUtilized;                                                                         
      SD.IDSUtilized  = S1->IDSUtilized + S2->IDSUtilized;                 /* disk-seconds (instant) utilized               */
      SD.MSAvail      = S1->MSAvail + S2->MSAvail;                         /* memoryseconds available                       */
      SD.MSUtilized   = S1->MSUtilized + S2->MSUtilized;                                                                        
      SD.MSDedicated  = S1->MSDedicated + S2->MSDedicated;                 /* memoryseconds dedicated                       */
      SD.StartJC      = S1->StartJC + S2->StartJC;                         /* started jobs                                  */
      SD.StartPC      = S1->StartPC + S2->StartPC;                         /* started proc-weighted jobs                    */
      SD.StartXF      = S1->StartXF + S2->StartXF;                         /* total XF at job start                         */
      SD.StartQT      = S1->StartQT + S2->StartQT;                         /* total QT at job start                         */

      if (SD.JC != 0)
        {
        SD.AvgXFactor = ((S1->AvgXFactor * S1->JC) + (S2->AvgXFactor * S2->JC)) / SD.JC;
        SD.AvgQTime   = (S1->TotalQTS + S2->TotalQTS) / SD.JC;
        }

      SD.XFactor    = S1->XFactor + S2->XFactor;                           /* xfactor sum                                   */
      SD.JobAcc     = S1->JobAcc + S2->JobAcc;                             /* job accuracy sum                              */

#ifdef __STATTYPES
      SD.NJobAcc     = S1->NJobAcc + S2->NJobAcc;                          /* node weighed job accuracy sum                 */
      SD.NXFactor    = S1->NXFactor + S2->NXFactor;                        /* node weighed xfactor sum                      */
      SD.PSXFactor   = S1->PSXFactor + S2->PSXFactor;                      /* ps weighted xfactor sum                       */
#endif /* __MSTATTYPES */

      for (aindex = 0;aindex < MMAX_ACCURACY;aindex++)
        {
        SD.Accuracy[aindex] = S1->Accuracy[aindex] + S2->Accuracy[aindex]; /* wallclock accuracy distribution */
        }

      SD.MaxXFactor  = MAX(S1->MaxXFactor,S2->MaxXFactor);                 /* max xfactor detected                          */

      SD.Bypass      = S1->Bypass + S2->Bypass;                            /* number of times job was bypassed              */
      SD.MaxBypass   = MAX(S1->MaxBypass,S2->MaxBypass);                   /* max number of times job was not scheduled     */
      SD.BFCount     = S1->BFCount + S2->BFCount;                          /* number of jobs backfilled                     */
      SD.BFPSRun     = S1->BFPSRun + S2->BFPSRun;                          /* procseconds backfilled                        */
                                                                           /* number of credits used                        */
      SD.QOSCredits  = S1->QOSCredits + S2->QOSCredits;                    /* number of credits used                        */

      SD.XLoad = (double *)MUCalloc(1,sizeof(double) * MSched.M[mxoxGMetric]);

      for (aindex = 0;aindex < MSched.M[mxoxGMetric];aindex++)
        {
        SD.XLoad[aindex] = S1->XLoad[aindex] + S2->XLoad[aindex];
        }

      MStatPCopy2(S1,&SD,msoCred);

      MUFree((char **)&SD.XLoad);
      }  /* END BLOCK (case msoCred) */

      break;

    case msoNode:

      break;

    default:

      break;
    }

  return(SUCCESS);
  }  /* END MStatMerge() */




/**
 *Get the XLoad member out of stats structures 
 */

double *MStatExtractXLoad(

  void              *Stat,
  enum MXMLOTypeEnum OType)

  {
  switch(OType)
    {
    case mxoNode: return ((mnust_t* )Stat)->XLoad;
    default: return ((must_t *)Stat)->XLoad;
    }
  }  /* END MUExtractXLoad() */





/**
 * Apply a generic metric to a generic metric array.
 *
 * @param XLoad (O)
 * @param GMetricName
 * @param GMetricValue)
 */

int MStatInsertGenericMetric(

  double *XLoad,         /* O */ 
  char   *GMetricName,
  double  GMetricValue)

  {
  int GMetricIndex;

  GMetricIndex = MUMAGetIndex(meGMetrics,GMetricName,mAdd);

  if (GMetricIndex == 0)
    {
    return(FAILURE);
    }

  if (GMetricValue != MDEF_GMETRIC_VALUE)
    XLoad[GMetricIndex] = GMetricValue;

  return(SUCCESS);
  } /* END MUInsertGenericMetric() */





/**
 * Add the last profile that is in memory to the end of the arary.
 *
 * NOTE: Because the database only has past iterations we need to add the
 *       current iteration in memory to the array.
 *
 * @param OType
 * @param OID
 * @param SAP
 * @param EndTime
 */

int MStatAddLastFromMemory(

  enum MXMLOTypeEnum OType,
  char              *OID,
  void              *SAP,
  mulong             EndTime)

  {
  void *SPtr = NULL;
  void *O = NULL;

  if ((long)EndTime < (MStat.P.IStatEnd - MStat.P.IStatDuration))
    {
    return(SUCCESS);
    }

  if (MOGetObject(OType,OID,&O,mVerify) == FAILURE)
    {
    return(FAILURE);
    }

  if (MOGetComponent(O,OType,&SPtr,mxoxStats) == FAILURE)
    {
    return(FAILURE);
    }

  switch (OType)
    {
    case mxoUser:
    case mxoGroup:
    case mxoAcct:
    case mxoClass:
    case mxoQOS:

      {
      must_t *S = (must_t *)SPtr;
      must_t *Stats = (must_t *)SAP;

      /* Prevent segfault by ensuring IStatCount is greater than zero. */

      if ((S->IStatCount > 0) && (Stats->IStatCount > 0))
        {
        if (S->IStat[S->IStatCount - 1]->StartTime == Stats->IStat[Stats->IStatCount - 1]->StartTime)
          {
          MStatPCopy((void **)&Stats->IStat[Stats->IStatCount - 1],
                     S->IStat[S->IStatCount - 1],
                     msoCred);
          }
        }
      } /* END case msoCred */

      break;

    case mxoNode:

      {
      mnust_t *S = (mnust_t *)SPtr;
      mnust_t *Stats = (mnust_t *)SAP;

      if (S->IStat[S->IStatCount - 1]->StartTime == Stats->IStat[Stats->IStatCount - 1]->StartTime)
        {
        MStatPCopy((void **)&Stats->IStat[Stats->IStatCount - 1],
                   S->IStat[S->IStatCount - 1],
                   msoNode);
        }
      } /* END case msoNode */

      break;

    default:

      return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MStatAddLastFromMemory() */

/* END MStats.c */
