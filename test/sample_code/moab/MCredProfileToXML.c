/* HEADER */

      
/**
 * @file MCredProfileToXML.c
 *
 * Contains: MCredProfileToXML
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 * Report credential profile stats as XML.
 * Caller may specify specific stat attributes.
 *
 * NOTE:  process 'mcredctl -q profile <CREDTYPE> --xml'
 * NOTE:  process 'showstats -acqgu -t <time>[<time>]
 *
 * @see MNodeProfileToXML() - sync
 * @see MTJobStatProfileToXML() - sync
 * @see MStatPToString() - child
 * @see MStatPToSummaryString() - child
 * @see __MUICredCtlQuery() - parent
 *
 * @param OIndex       (I)
 * @param OName        (I) [optional]
 * @param SName        (I) [optional]
 * @param StartTime    (I)
 * @param EndTime      (I)
 * @param Time         (I)
 * @param MStatAttrsBM (I) [optional]
 * @param DoSummary    (I)
 * @param ClientOutput 
 * @param E            (O) [modified]
 */

int MCredProfileToXML(

  enum MXMLOTypeEnum  OIndex,
  char               *OName,
  const char         *SName,
  mulong              StartTime,
  mulong              EndTime,
  long                Time,
  mbitmap_t          *MStatAttrsBM,
  mbool_t             DoSummary,
  mbool_t             ClientOutput,
  mxml_t             *E)

  {
  mxml_t *OE;
  mxml_t *PE;

  must_t *SPtr;
  must_t  tmpStat; /* this needs to be initialized to zeros! */

  mhashiter_t HTI;

  enum MStatAttrEnum SIndex;

  int IStatDuration;
  int sindex;

  char tmpBuf[MMAX_BUFFER];
  char tmpName[MMAX_LINE];

  void   *O;
  void   *OP;

  void   *OEnd;

  char   *NameP;

  must_t  **NewS = NULL;

  int     PIndex;
  int     PDuration;

  /* number of profiles to print out (calculated) */
  int     Iterations;

  mdb_t *MDBInfo;

  const enum MStatAttrEnum AList[] = {
    mstaDuration,
    mstaTANodeCount,
    mstaTAProcCount,
    mstaTJobCount,
    mstaTNJobCount,
    mstaTQueueTime,
    mstaMQueueTime,
    mstaTReqWTime,
    mstaTExeWTime,
    mstaTDSAvl,
    mstaTDSUtl,
    mstaIDSUtl,
    mstaTMSAvl,
    mstaTMSDed,
    mstaTMSUtl,
    mstaTPSReq,
    mstaTPSExe,
    mstaTPSDed,
    mstaTPSUtl,
    mstaTJobAcc,
    mstaTNJobAcc,
    mstaTXF,
    mstaTNXF,
    mstaMXF,
    mstaTBypass,
    mstaMBypass,
    mstaTQOSAchieved,
    mstaStartTime,
    mstaIStatCount,
    mstaIStatDuration,
    mstaGCEJobs,        /* current eligible jobs */
    mstaGCIJobs,        /* current idle jobs */
    mstaGCAJobs,        /* current active jobs */
    mstaGPHAvl,
    mstaGPHUtl,
    mstaGPHDed,
    mstaGPHSuc,
    mstaGMinEff,
    mstaGMinEffIteration,
    mstaTNC,
    mstaTPC,
    mstaTQueuedJC,
    mstaTQueuedPH,
    mstaTSubmitJC,      /* S->SubmitJC */
    mstaTSubmitPH,      /* S->SubmitPH */
    mstaTStartJC,       /* S->StartJC (field 50) */
    mstaTStartPC,       /* S->StartPC */
    mstaTStartQT,       /* S->StartQT */
    mstaTStartXF,       /* S->StartXF */
    mstaIterationCount,
    mstaQOSCredits,
    mstaTNL,
    mstaTNM,
    mstaGMetric,
    mstaNONE };

  if ((E == NULL) || (MStat.P.IStatDuration <= 0))
    {
    return(FAILURE);
    }

  /* enforce time constraints */

  PIndex    = 0;
  PDuration = MStat.P.IStatDuration;

  if (MStat.P.IStatDuration == 0)
    {
    return(SUCCESS);
    }

  memset(&tmpStat,0,sizeof(tmpStat));

  Iterations = (EndTime - StartTime) / PDuration;

  /* FIXME:  why do we need to do this?  */

  Iterations++;

  /**/

  /* get all profile data for credential type within timeframe */

  SIndex = (enum MStatAttrEnum)MUGetIndexCI(SName,MStatAttr,FALSE,mstaNONE);

  if (MSched.CredDiscovery == TRUE)
    {
    MStatValidatePData(
      NULL,
      OIndex,
      NULL,
      NULL,
      NULL,
      StartTime,
      EndTime,
      Time,
      TRUE,
      1,
      NULL);
    }

  memset(&tmpStat,0,sizeof(tmpStat));

  MOINITLOOP(&OP,OIndex,&OEnd,&HTI);

  while ((O = MOGetNextObject(&OP,OIndex,OEnd,&HTI,&NameP)) != NULL)
    {
    if ((NameP == NULL) ||
        !strcmp(NameP,NONE) ||
        !strcmp(NameP,ALL) ||
        !strcmp(NameP,"ALL") ||
        !strcmp(NameP,"NOGROUP"))
      {
      continue;
      }

    if (!strcmp(NameP,"DEFAULT"))
      {
      /* NOTE:  currently display DEFAULT only for QOS credtype */

      if (OIndex != mxoQOS)
        continue;

      /* should credtype defaults be displayed only if usage is accrued? */

      /* NYI */
      }

    if ((OName != NULL) && (OName[0] != '\0') && (strcmp(OName,NameP)))
      continue;

    if (MOGetComponent(O,OIndex,(void **)&SPtr,mxoxStats) == FAILURE)
      {
      /* NOTE:  SPtr->IStat can be NULL if there are no current stats
                even if historical stats exist (FIXME?) */

      if (SPtr->IStat == NULL)
        {
        /* comment out - DAVE - TEMP */

        /* continue; */
        }
      }

    OE = NULL;

    MXMLCreateE(&OE,(char *)MXO[OIndex]);

    MXMLSetAttr(
      OE,
      (char *)MCredAttr[mcaID],
      (void **)((NameP != NULL) ? NameP : "NONE"),
      mdfString);

    PE = NULL;

    if (DoSummary == TRUE)
      {
      MXMLCreateE(&PE,(char *)MXO[mxoxStats]);
      }
    else
      {
      MXMLCreateE(&PE,(char *)MStatAttr[mstaProfile]);
      }

    MXMLSetAttr(PE,(char *)MProfAttr[mpraCount],(void *)&SPtr->IStatCount,mdfInt);
    MXMLSetAttr(PE,(char *)MProfAttr[mpraMaxCount],(void *)&MStat.P.MaxIStatCount,mdfInt);

    if (MStat.P.IStatDuration > 0)
      {
      IStatDuration = MStat.P.IStatDuration;
      }
    else
      {
      IStatDuration = MDEF_PSTEPDURATION;
      }

    MXMLSetAttr(PE,"SpecDuration",(void *)&IStatDuration,mdfInt);

    MXMLAddE(OE,PE);

    /* need stat type */

    MSchedGetAttr(msaDBHandle,mdfNONE,(void **)&MDBInfo,-1);

    if (MDBInfo->DBType != mdbNONE)
      {
      if (MDBReadGeneralStatsRange(
            MDBInfo,
            &tmpStat, 
            (OIndex == mxoSched) ? mxoPar : OIndex,
            (OIndex == mxoSched) ? (char *)"ALL" : NameP,
            IStatDuration,
            StartTime,
            EndTime,
           NULL) == FAILURE)
        {
        /* failure in database query */

        MDB(3,fALL) MLog("WARNING:  could not retrieve statistics from database\n");

        MStatPDestroy((void ***)&tmpStat.IStat,msoCred,Iterations);

        MXMLDestroyE(&OE);
  
        return(FAILURE);
        }

      NewS = tmpStat.IStat;

      tmpStat.IStat      = NULL;
      tmpStat.IStatCount = 0;
      }
    else
      {
      NewS = (must_t **)MUCalloc(1,sizeof(must_t *) * Iterations);

      if (MStatValidatePData(
            O,
            OIndex,
            NameP,
            SPtr,        /* I */
            NewS,        /* O */
            StartTime,
            EndTime,
            Time,
            FALSE,
            Iterations,
            MStatAttrsBM) == FAILURE)
        {
        /* cannot generate valid data */

        MDB(3,fALL) MLog("WARNING:  cannot generate valid stat data for external query\n");

        MStatPDestroy((void ***)&tmpStat.IStat,msoCred,Iterations);

        MXMLDestroyE(&OE);

        return(FAILURE);
        }
      }  /* END else */

    for (sindex = 0;AList[sindex] != mstaNONE;sindex++)
      {
      if ((SIndex != mstaNONE) && (AList[sindex] != SIndex))
        continue;

      if (NewS[PIndex] == NULL)
        continue;

      if (!MStatIsProfileAttr(msoCred,AList[sindex]))
        continue;

      if ((MStatAttrsBM != NULL) && 
         (!bmisset(MStatAttrsBM,AList[sindex])) && 
         (AList[sindex] != mstaStartTime))
        {
        /* attribute not included in user-specified attribute list */

        continue;
        }

      if (AList[sindex] == mstaGMetric)
        {
        int xindex;

        /* loop through all generic metrics */

        for (xindex = 1;xindex < MSched.M[mxoxGMetric];xindex++)
          {
          if (MGMetric.Name[xindex][0] == '\0')
            break;

          /* NOTE:  cannot test S->XLoad[xindex] because XLoad may only be set in
                    a subset of S->IStat[] stats */

          if (DoSummary == FALSE)
            {
            MStatPToString(
              (void **)NewS,
              msoCred,
              Iterations,
              (int)AList[sindex],
              xindex,
              tmpBuf,          /* O:  comma delimited list of values */
              sizeof(tmpBuf));
            }
          else
            {
            MStatPToSummaryString(
              (void **)NewS,
              msoCred,
              Iterations,
              (int)AList[sindex],
              xindex,
              tmpBuf,          /* O */
              sizeof(tmpBuf));
            }

          if (tmpBuf[0] == '\0')
            continue;

          snprintf(tmpName,sizeof(tmpName),"GMetric.%s",
            MGMetric.Name[xindex]);

          if ((ClientOutput == TRUE) && (MSched.ReportProfileStatsAsChild == TRUE))
            MXMLAddChild(PE,tmpName,tmpBuf,NULL);
          else
            MXMLSetAttr(PE,tmpName,(void *)tmpBuf,mdfString);
          }  /* END for (gindex) */

        continue;
        }    /* END else if (AList[sindex] = mstaGMetric) */

      if (DoSummary == FALSE)
        {
        MStatPToString(
          (void **)NewS,
          msoCred,
          Iterations,
          AList[sindex],
          0,
          tmpBuf,          /* O:  comma delimited list of values */
          sizeof(tmpBuf));
        }
      else
        {
        MStatPToSummaryString(
          (void **)NewS,
          msoCred,
          Iterations,
          AList[sindex],
          0,
          tmpBuf,          /* O */
          sizeof(tmpBuf));
        }

      if (tmpBuf[0] == '\0')
        continue;

      if ((ClientOutput == TRUE) && (MSched.ReportProfileStatsAsChild == TRUE))
        MXMLAddChild(PE,(char *)MStatAttr[AList[sindex]],tmpBuf,NULL);
      else
        MXMLSetAttr(PE,(char *)MStatAttr[AList[sindex]],(void *)tmpBuf,mdfString);
      }  /* END for (sindex) */

    MStatPDestroy((void ***)&NewS,msoCred,Iterations);

    MXMLAddE(E,OE);
    }  /* END while ((O = MOGetNextObject(&OP,OIndex,OEnd,&NameP)) != NULL) */

  return(SUCCESS);
  }  /* END MCredProfileToXML() */
/* END MCredProfileToXML.c */
