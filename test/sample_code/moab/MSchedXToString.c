/* HEADER */

      
/**
 * @file MSchedXToString.c
 *
 * Contains: Various MSched X ToString functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 * Convert scheduler object to string
 *
 * @see MCPCreate() - parent
 * @see MSchedAToString() - child
 *
 * @param S      (I)
 * @param Format (I)
 * @param IsCP   (I)
 * @param Buf    (O)
 */

int MSchedToString(

  msched_t             *S,
  enum MFormatModeEnum  Format,
  mbool_t               IsCP,
  mstring_t            *Buf)

  {
  int aindex;
  int index;

  int rc;

  const enum MSchedAttrEnum CPAList[] = {
    msaCPVersion,
    msaEventCounter,
    msaMessage,
    msaRsvCounter,
    msaRsvGroupCounter,
    msaTrigCounter,
    msaLastCPTime,
    msaLastTransCommitted,
    msaLocalQueueIsActive,
    msaTransCounter,
    msaRestartState,
    msaStateMAdmin,
    msaStateMTime,
    msaNONE };

  const enum MSchedAttrEnum DAList[] = {
    msaCPVersion,
    msaIterationCount,
    msaLastCPTime,
    msaLastTransCommitted,
    msaLocalQueueIsActive,
    msaMessage,
    msaRsvCounter,
    msaRsvGroupCounter,
    msaTrigCounter,
    msaTransCounter,
    msaNONE };

  const enum MAdminAttrEnum AAList[] = {
    madmaName,
    madmaServiceList,
    madmaUserList,
    madmaNONE };

  enum MSchedAttrEnum *AList;
 
  mxml_t *E = NULL;
  mxml_t *AE;
 
  if ((S == NULL) || (Buf == NULL))
    {
    return(FAILURE);
    }

  AList = (enum MSchedAttrEnum *)((IsCP == TRUE) ? CPAList : DAList);
 
  MXMLCreateE(&E,(char *)MXO[mxoSched]);

  /* add attributes */

  mstring_t tmpString(MMAX_LINE);

  for (aindex = 0;AList[aindex] != msaNONE;aindex++)
    {
    MSchedAToString(
      &MSched,
      AList[aindex],
      &tmpString,
      Format);

    if (MUStrIsEmpty(tmpString.c_str()))
      continue; 

    MXMLSetAttr(E,(char *)MSchedAttr[AList[aindex]],tmpString.c_str(),mdfString);    
    }    /* END for (aindex) */

  if (IsCP == FALSE)
    {
    /* add children */

    for (aindex = 1;aindex <= MMAX_ADMINTYPE;aindex++)
      {
      AE = NULL;

      MXMLCreateE(&E,"admin");

      for (index = 0;AAList[index] != madmaNONE;index++)
        {
        MAdminAToString(
          &MSched.Admin[aindex],
          AAList[index],
          &tmpString,
          Format);

        if (MUStrIsEmpty(tmpString.c_str()))
          continue;

        MXMLSetAttr(AE,(char *)MAdminAttr[AList[index]],tmpString.c_str(),mdfString);
        }    /* END for (aindex) */

      MXMLAddE(E,AE);
      }  /* END for (aindex) */
    }    /* END if (IsCP == FALSE) */

  rc = MXMLToMString(E,Buf,NULL,TRUE);
 
  MXMLDestroyE(&E);
 
  return(rc);
  }  /* END MSchedToString() */





/**
 * Load <ATTR>=<VAL> pairs from scheduler checkpoint record.
 * 
 * @see MCPLoadSched() - parent
 * @see MXMLFromString() - child
 *
 * @param S (I) [modified]
 * @param Buf (I)
 */

int MSchedFromString(

  msched_t *S,    /* I (modified) */
  const char     *Buf)  /* I */

  {
  int aindex;

  enum MSchedAttrEnum saindex;

  mxml_t *E;

  const char *FName = "MSchedFromString";

  MDB(4,fSCHED) MLog("%s(S,%s)\n",
    FName,
    (Buf != NULL) ? Buf : "NULL");

  if ((S == NULL) || (Buf == NULL))
    {
    return(FAILURE);
    }

  E = NULL;

  if (MXMLFromString(&E,Buf,NULL,NULL) == FAILURE)
    {
    return(FAILURE);
    }

  for (aindex = 0;aindex < E->ACount;aindex++)
    {
    saindex = (enum MSchedAttrEnum)MUGetIndex(E->AName[aindex],MSchedAttr,FALSE,0);

    if (saindex == msaNONE)
      continue;

    MSchedSetAttr(&MSched,saindex,(void **)E->AVal[aindex],mdfString,mSet);
    }  /* END for (aindex) */

  MXMLDestroyE(&E);

  return(SUCCESS);
  }  /* END MSchedFromString() */





/**
 * Report scheduler stats to XML.
 *
 * NOTE:  this routine should only be used by client commands (see
 *        mcoReportProfileStatsAsChild
 *
 * NOTE:  used by 'showstats -v'
 *
 * @param S (I)
 * @param DisplayMode (I)
 * @param Buf (O)
 * @param BufSize (I)
 * @param OutputIsXML (I) [NOTE:  only valid for DisplayMode == XML]
 */

int MSchedStatToString(

  msched_t               *S,           /* I */
  enum MWireProtocolEnum  DisplayMode, /* I */
  void                   *Buf,         /* O */
  int                     BufSize,     /* I */
  mbool_t                 OutputIsXML) /* I (NOTE:  only valid for DisplayMode == XML) */

  {
  const char *FName = "MSchedStatToString";

  MDB(2,fUI) MLog("%s(S,%s,Buf,BufSize,%s)\n",
    FName,
    MWireProtocol[DisplayMode],
    MBool[OutputIsXML]);

  if ((S == NULL) || (Buf == NULL))
    {
    return(FAILURE);
    }

  /* NOTE: MSchedStatToString() assumes authorization check is performed by the caller */

  switch (DisplayMode)
    { 
    case mwpXML:

      {
      mxml_t *DE = NULL;
      mxml_t *E  = NULL;

      /* stat attributes */

      enum MStatAttrEnum AList[] = {
        mstaStartTime,
        mstaSchedDuration,
        mstaGCIJobs,
        mstaGCEJobs,
        mstaGCAJobs,
        mstaTJobCount,
        mstaTCapacity,
        mstaDuration,
        mstaGPHAvl,
        mstaGPHUtl,
        mstaGPHDed,
        mstaGPHSuc,
        mstaTDSAvl,
        mstaTDSUtl,
        mstaIDSUtl,
        mstaTMSAvl,
        mstaTMSDed,
        mstaTMSUtl,
        mstaTNJobCount,
        mstaTNJobAcc,
        mstaTJobAcc,
        mstaAvgXFactor,
        mstaGMinEff,
        mstaGMinEffIteration,
        mstaMXF,
        mstaMBypass,
        mstaMQueueTime,
        mstaTPSDed,
        mstaTPSExe,
        mstaTPSReq,
        mstaTPSUtl,
        mstaTJEval,
        mstaTJPreempt,
        mstaTPHPreempt,
        mstaTSubmitJC,     /* S->SubmitJC */
        mstaTSubmitPH,     /* S->SubmitPH */
        mstaTStartJC,      /* S->StartJC (field 50) */
        mstaTStartPC,      /* S->StartPC */
        mstaTStartQT,      /* S->StartQT */
        mstaTStartXF,      /* S->StartXF */
        mstaAvgBypass,
        mstaAvgQueueTime,
        mstaAvgXFactor,
        mstaNONE };

      /* sys attributes */

      enum MSysAttrEnum BList[] = {
        msysaStatInitTime,
        msysaUpNodes,
        msysaUpProcs,
        msysaUpMem,
        msysaIdleNodeCount,
        msysaIdleProcCount,
        msysaIdleMem,
        msysaIterationCount,
        msysaSCompleteJobCount,
        msysaActivePS,
        msysaQueuePS,
        msysaAvgTActivePH,
        msysaAvgTQueuedPH,
        msysaRMPollInterval,
        msysaPresentTime,
        msysaNONE };

      /* do not display any stat attrs in MSysToXML() */

      enum MXMLOTypeEnum CList[] = {
        mxoNONE };

      if ((OutputIsXML == TRUE) && (Buf != NULL))
        DE = (mxml_t *)Buf;
      else
        MXMLCreateE(&DE,(char*)MSON[msonData]);

      if (MStatToXML((void *)&MPar[0].S,msoCred,&E,TRUE,AList) == SUCCESS)
        {
        if (MStat.P.IStatDuration > 0)
          {
          MXMLSetAttr(E,"SpecDuration",(void *)&MStat.P.IStatDuration,mdfLong);
          }
        else
          {
          int tmpI = MDEF_PSTEPDURATION;

          MXMLSetAttr(E,"SpecDuration",(void *)&tmpI,mdfLong);
          }

        /* show rates for job submission, starts, completion */ 

        if (MPar[0].S.IStatCount > 0)
          {
          double tmpD;

          int sindex;

          int TotalTime       = 0;
          int TotalSubmitJC   = 0;
          int TotalStartJC    = 0;
          int TotalSuccessJC  = 0;

          sindex = MPar[0].S.IStatCount;

          while ((MPar[0].S.IStatCount - sindex < 5) && (sindex > 0))
            {
            if ((MSched.Time - MPar[0].S.IStat[sindex - 1]->StartTime) < (mulong)MStat.P.IStatDuration)
               TotalTime += MSched.Time - MPar[0].S.IStat[sindex - 1]->StartTime;
            else
               TotalTime += MStat.P.IStatDuration;

            TotalSubmitJC   += MPar[0].S.IStat[sindex - 1]->SubmitJC;
            TotalStartJC    += MPar[0].S.IStat[sindex - 1]->StartJC;
            TotalSuccessJC  += MPar[0].S.IStat[sindex - 1]->SuccessJC;

            sindex--;
            }

          if (TotalTime > 0)
            {
            MXMLSetAttr(E,"ThroughputTime",&TotalTime,mdfLong);

            tmpD = ((double)TotalSubmitJC)/(TotalTime/60.0);
            MXMLSetAttr(E,"JSubmitRate",&tmpD,mdfDouble);

            tmpD = ((double)TotalSuccessJC)/(TotalTime/60.0);
            MXMLSetAttr(E,"JSuccessRate",&tmpD,mdfDouble);

            tmpD = ((double)TotalStartJC)/(TotalTime/60.0);
            MXMLSetAttr(E,"JStartRate",&tmpD,mdfDouble);
            }
          }

        MXMLAddE(DE,E);
        }

      E = NULL;

      if (MSysToXML(S,&E,BList,CList,TRUE,0) == SUCCESS)
        {
        MXMLAddE(DE,E);
        }

      if (OutputIsXML == FALSE)
        MXMLToString(DE,(char *)Buf,MMAX_BUFFER,NULL,TRUE);

      if ((OutputIsXML != TRUE) || (Buf == NULL))
        {
        MXMLDestroyE(&DE);
        } 
      }  /* END BLOCK (case mwpXML) */

      break;

    default:

      {
      long     RunTime;

      char    *BPtr;
      int      BSpace;

      must_t  *T;

      mpar_t  *GP;

      BPtr = (char *)Buf;
      BSpace = BufSize;

      BPtr[0] = '\0';

      /* build stat buffer */

      MUSNPrintF(&BPtr,&BSpace,"%ld\n",
        S->Time);

      /* set up scheduler run time */

      RunTime = MStat.SchedRunTime;

      GP = &MPar[0];
      T  = &GP->S;

      /*                        STM ITM RTM IJ EJ AJ UN UP UM IN IP IM CT SJ TPA TPB SPH TMA TMD QP AQP NJA JAC PSX IT RPI WEF WI MXF ABP MBP AQT MQT PSR PSD PSU MSA MSD JE */

      MUSNPrintF(&BPtr,&BSpace,"%ld %ld %ld %d %d %d %d %d %d %d %d %d %d %d %f %f %f %f %f %ld %lu %f %f %f %d %ld %f %d %f %f %d %f %lu %f %f %f %f %f %d %d %f\n",
        S->StartTime,
        MStat.InitTime,
        RunTime,
        MStat.EligibleJobs,
        MStat.IdleJobs,
        MStat.ActiveJobs,
        GP->UpNodes,
        GP->UpRes.Procs,
        GP->UpRes.Mem,
        GP->IdleNodes,
        MIN(GP->ARes.Procs,(GP->UpRes.Procs - GP->DRes.Procs)),
        MIN(GP->ARes.Mem  ,(GP->UpRes.Mem   - GP->DRes.Mem  )),
        T->JC,
        MStat.SuccessfulJobsCompleted,
        MStat.TotalPHAvailable,
        MStat.TotalPHBusy,
        MStat.SuccessfulPH,
        T->MSAvail,
        T->MSDedicated,
        GP->L.IdlePolicy->Usage[mptMaxPS][0],
        (S->Iteration > 0) ? MStat.TQueuedPH / S->Iteration : 0,
        T->NJobAcc,
        T->JobAcc,
        (double)((T->PSRun > 0.0) ? T->PSXFactor / T->PSRun : 0.0),
        S->Iteration,
        S->MaxRMPollInterval,
        MStat.MinEff,
        MStat.MinEffIteration,
        T->MaxXFactor,
        (double)((T->JC > 0) ? (double)T->Bypass / T->JC : 0.0),
        T->MaxBypass,
        (double)((T->JC > 0) ? (double)T->TotalQTS / T->JC : 0.0),
        T->MaxQTS,
        T->PSRun,
        T->PSDedicated,
        T->PSUtilized,
        T->MSDedicated,
        T->MSUtilized,
        MStat.EvalJC,
        MStat.TotalPreemptJobs,
        (double)MStat.TotalPHPreempted / 3600.0);
      }  /* END BLOCK */
    }    /* END switch (Mode) */

  return(SUCCESS);
  }  /* END MSchedStatToString() */
/* END MSchedXToString.c */
