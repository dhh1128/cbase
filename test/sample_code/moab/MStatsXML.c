/* HEADER */

      
/**
 * @file MStatsXML.c
 *
 * Contains: MStatXML functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 * Convert stat (must_t or mnust_t) structure to corresponding XML.
 *
 * @see MStatToString() - parent
 * @see MStatFromXML() - peer
 * @see MNodeToXML() - parent
 * @see MNodeProfileToXML()/MCredProfileToXML() - peer - report object profile stats
 *
 * NOTE:  reports results for 'mcredctl -q profile <CREDTYPE>[:<CREDID>] --format=xml'
 *
 * NOTE:  called w/no SSAList for checkpointing 
 *
 * @param SS           (I)
 * @param SType        (I)
 * @param EP           (O) [allocated]
 * @param ClientOutput (I)
 * @param SSAList      (I) [optional]
 */

int MStatToXML(

  void                  *SS,
  enum MStatObjectEnum   SType,
  mxml_t               **EP,
  mbool_t                ClientOutput,
  void                  *SSAList)

  {
  if (EP != NULL)
    *EP = NULL;

  if ((SS == NULL) || (EP == NULL))
    {
    return(FAILURE);
    }
    
  switch (SType)
    {
    case msoCred:

      {
      must_t *S = (must_t *)SS;

      enum MStatAttrEnum *SAList = (enum MStatAttrEnum *)SSAList;

      enum MStatAttrEnum DAList[] = {
        mstaTANodeCount,
        mstaTAProcCount,
        mstaTNC,
        mstaTPC,
        mstaTQueuedJC,
        mstaTQueuedPH,
        mstaTJobCount,
        mstaTNJobCount,
        mstaTQueueTime,
        mstaMQueueTime,
        mstaTReqWTime,
        mstaTExeWTime,
        mstaTMSAvl,
        mstaTMSDed,
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
        mstaTSubmitJC,
        mstaTSubmitPH,
        mstaTStartJC,
        mstaTStartPC,
        mstaTStartQT,
        mstaTStartXF,
/*
        mstaTAppBacklog,
        mstaTAppLoad,
        mstaTAppQueueTime,
        mstaTAppResponseTime,
*/
        mstaStartTime,
        mstaIStatDuration,
        mstaIterationCount,
        mstaIStatCount,
        mstaProfile,
        mstaQOSCredits,
        mstaTNL,
        mstaTNM,
        mstaGMetric,  /* NOTE:  report GMetric only - no mstaXLoad */
        mstaNONE };
     
      int  aindex;
      int  rc;
 
      mxml_t *PE = NULL;
     
      enum MStatAttrEnum *AList;
     
      char tmpString[MMAX_BUFFER];  /* must be large enough to handle any single must_t attribute */
    
      char tmpName[MMAX_LINE];
 
      if ((S == NULL) || (EP == NULL))
        {
        return(FAILURE);
        }
     
      if (SAList != NULL)
        AList = SAList;
      else
        AList = DAList;
     
      MXMLCreateE(EP,(char *)MXO[mxoxStats]);
     
      /* locate profile request */
     
      if (S->IStat != NULL)
        {
        for (aindex = 0;AList[aindex] != mstaNONE;aindex++)
          {
          if (AList[aindex] != mstaProfile)
            continue;
     
          MXMLCreateE(&PE,(char *)"Profile");
     
          MXMLAddE(*EP,PE);
     
          break;
          }
        }    /* END if (S->IStat != NULL) */
     
      for (aindex = 0;AList[aindex] != mstaNONE;aindex++)
        {
        if (AList[aindex] == mstaGMetric)
          {
          int gindex;

          for (gindex = 1;gindex < MSched.M[mxoxGMetric];gindex++)
            {
            if (MGMetric.Name[gindex][0] == '\0')
              break;

            /* NOTE:  cannot test S->XLoad[gindex] because XLoad may only 
                      be set in a subset of S->IStat[] stats */

            MStatAToString(
              (void **)S,
              msoCred,
              AList[aindex],
              gindex,
              tmpString,
              mfmNONE);

            if (tmpString[0] == '\0')
              continue;

            snprintf(tmpName,sizeof(tmpName),"%s.%s",
              MStatAttr[AList[aindex]],
              MGMetric.Name[gindex]);

            if ((MSched.ReportProfileStatsAsChild == TRUE) && (ClientOutput == TRUE))
              MXMLAddChild(*EP,tmpName,tmpString,NULL);
            else
              MXMLSetAttr(*EP,tmpName,tmpString,mdfString);
            }  /* END for (gindex) */
          }
        else
          {
          rc = MStatAToString(
                (void *)S,
                msoCred,
                AList[aindex],
                0,
                tmpString,  /* O */
                mfmNONE);

          if ((rc == FAILURE) || (tmpString[0] == '\0'))
            {
            continue;
            }

          if ((MSched.ReportProfileStatsAsChild == TRUE) && (ClientOutput == TRUE))
            MXMLAddChild(*EP,(char *)MStatAttr[AList[aindex]],tmpString,NULL);
          else
            MXMLSetAttr(*EP,(char *)MStatAttr[AList[aindex]],tmpString,mdfString);
          }

        if (PE != NULL)
          {
          switch (AList[aindex])
            {
            case mstaIStatCount:
            case mstaIStatDuration:
     
              /* NO-OP */
     
              break;
    
            case mstaGMetric:

              {
              int gindex;

              for (gindex = 1;gindex < MSched.M[mxoxGMetric];gindex++)
                {
                if (MGMetric.Name[gindex][0] == '\0')
                  break;

                /* NOTE:  cannot test S->XLoad[gindex] because XLoad may only 
                          be set in a subset of S->IStat[] stats */

                MStatPToString(
                  (void **)S->IStat,
                  msoCred,
                  S->IStatCount,
                  AList[aindex],
                  gindex,
                  tmpString,
                  sizeof(tmpString));

                if (tmpString[0] == '\0')
                  continue;

                snprintf(tmpName,sizeof(tmpName),"%s.%s",
                  MStatAttr[AList[aindex]],
                  MGMetric.Name[gindex]);

                if ((MSched.ReportProfileStatsAsChild == TRUE) && (ClientOutput == TRUE))
                  MXMLAddChild(PE,tmpName,tmpString,NULL);
                else
                  MXMLSetAttr(PE,tmpName,tmpString,mdfString);
                }  /* END for (gindex) */
              }    /* END BLOCK (case mstaGMetric) */

              break;
 
            default:
     
              MStatPToString(
                (void **)S->IStat,
                msoCred,
                S->IStatCount,
                AList[aindex],
                0,
                tmpString,
                sizeof(tmpString));
    
              if ((MSched.ReportProfileStatsAsChild == TRUE) && (ClientOutput == TRUE))
                MXMLAddChild(PE,(char *)MStatAttr[AList[aindex]],tmpString,NULL);
              else
                MXMLSetAttr(PE,(char *)MStatAttr[AList[aindex]],tmpString,mdfString);
     
              break;
            }  /* END switch (AList[aindex]) */
          }    /* END if (PE != NULL) */
        }      /* END for (aindex) */
      }        /* END BLOCK (case msoCred) */
    
      break;

    case msoNode:

      {
      mnust_t *S = (mnust_t *)SS;

      enum MNodeStatTypeEnum DAList[] = {
        mnstAGRes,
        mnstAProc,
        mnstCat,
        mnstCGRes,
        mnstCProc,
        mnstCPULoad,
        mnstDGRes,
        mnstDProc,
        mnstFailureJC,
        mnstGMetric,
        mnstIStatCount,
        mnstIStatDuration,
        mnstIterationCount,
        mnstMaxCPULoad,
        mnstMinCPULoad,
        mnstMaxMem,
        mnstMinMem,
        mnstNodeState,
        mnstQueuetime,
        /* mnstProfile, */  /* do not report profile stats by default */
        mnstRCat,
        mnstStartJC,
        mnstStartTime,
        mnstUMem,
        mnstXLoad,    /* should this be removed since move info is reported via mnstGMetric? */
        mnstXFactor,
        mnstNONE };

      int  aindex;
      int  rc;
 
      mxml_t *PE = NULL;
     
      enum MNodeStatTypeEnum *AList;
      enum MNodeStatTypeEnum *SAList = (enum MNodeStatTypeEnum *)SSAList;
     
      char tmpString[MMAX_BUFFER];
      char tmpName[MMAX_LINE];
     
      if ((S == NULL) || (EP == NULL))
        {
        return(FAILURE);
        }
     
      if (SAList != NULL)
        AList = SAList;
      else
        AList = DAList;
     
      MXMLCreateE(EP,(char *)MXO[mxoxStats]);
     
      /* locate profile request */
     
      if (S->IStat != NULL)
        {
        for (aindex = 0;AList[aindex] != mnstNONE;aindex++)
          {
          if (AList[aindex] != mnstProfile)
            continue;
     
          MXMLCreateE(&PE,(char *)"Profile");
     
          MXMLAddE(*EP,PE);
     
          break;
          }
        }    /* END if (S->IStat != NULL) */
     
      for (aindex = 0;AList[aindex] != mnstNONE;aindex++)
        {
        if (AList[aindex] == mnstGMetric)
          {
          int gindex;

          for (gindex = 1;gindex < MSched.M[mxoxGMetric];gindex++)
            {
            if (MGMetric.Name[gindex][0] == '\0')
              break;

            /* NOTE:  cannot test S->XLoad[gindex] because XLoad may only 
                      be set in a subset of S->IStat[] stats */

            MStatAToString(
              (void **)S,
              msoNode,
              AList[aindex],
              gindex,
              tmpString,
              mfmNONE);

            if (tmpString[0] == '\0')
              continue;

            snprintf(tmpName,sizeof(tmpName),"%s.%s",
              MNodeStatType[AList[aindex]],
              MGMetric.Name[gindex]);

            MXMLSetAttr(*EP,tmpName,tmpString,mdfString);
            }  /* END for (gindex) */
          }
        else
          {
          rc = MStatAToString(
                  (void *)S,
                  msoNode,
                  AList[aindex],
                  0,
                  tmpString,  /* O */
                  mfmNONE);

          if (((rc == FAILURE) || (tmpString[0] == '\0')) &&
               (AList[aindex] != mnstAGRes) && 
               (AList[aindex] != mnstCGRes) && 
               (AList[aindex] != mnstDGRes))
            {
            /* The GRes fails because there is no data in the 0th location.
               We can avoid this "if" statement by processing them in their own for loop.
               The same goes for GMetric as it is also indexed */

            continue;
            }
     
          MXMLSetAttr(*EP,(char *)MNodeStatType[AList[aindex]],tmpString,mdfString);
          }
     
        if (PE != NULL)
          {
          switch (AList[aindex])
            {
            case mnstIStatCount:
            case mnstIStatDuration:
     
              /* NO-OP */
     
              break;

            case mnstAGRes:
            case mnstCGRes:
            case mnstDGRes:
           
              {
              int gindex;

              for (gindex = 1;gindex < MSched.M[mxoxGRes];gindex++)
                {
                if (MGRes.Name[gindex][0] == '\0')
                  break;

                MStatPToString(
                  (void **)S->IStat,
                  msoNode,
                  S->IStatCount,
                  AList[aindex],
                  gindex,
                  tmpString,  /* O */
                  sizeof(tmpString));
 
                if (tmpString[0] == '\0')
                  continue;

                snprintf(tmpName,sizeof(tmpName),"%s.%s",
                  MNodeStatType[AList[aindex]],
                  MGRes.Name[gindex]);

                if ((MSched.ReportProfileStatsAsChild == TRUE) && (ClientOutput == TRUE))
                  MXMLAddChild(PE,tmpName,tmpString,NULL);
                else
                  MXMLSetAttr(PE,tmpName,tmpString,mdfString);
                }  /* END for (gindex) */
              }    /* END BLOCK (case mnstGRes) */

              break;

            case mnstGMetric:

              {
              int gindex;

              for (gindex = 1;gindex < MSched.M[mxoxGMetric];gindex++)
                {
                if (MGMetric.Name[gindex][0] == '\0')
                  break;

                /* NOTE:  cannot test S->XLoad[gindex] because XLoad may only be set in
                          a subset of S->IStat[] stats */

                MStatPToString(
                  (void **)S->IStat,
                  msoNode,
                  S->IStatCount,
                  AList[aindex],
                  gindex,
                  tmpString,
                  sizeof(tmpString));

                if (tmpString[0] == '\0')
                  continue;

                snprintf(tmpName,sizeof(tmpName),"%s.%s",
                  MNodeStatType[AList[aindex]],
                  MGMetric.Name[gindex]);

                if ((MSched.ReportProfileStatsAsChild == TRUE) && (ClientOutput == TRUE))
                  MXMLAddChild(PE,tmpName,tmpString,NULL);
                else
                  MXMLSetAttr(PE,tmpName,tmpString,mdfString);
                }  /* END for (gindex) */
              }    /* END BLOCK (case mnstGMetric) */

              break;
     
            default:
     
              MStatPToString(
                (void **)S->IStat,
                msoNode,
                S->IStatCount,
                AList[aindex],
                0,
                tmpString,
                sizeof(tmpString));
    
              if ((MSched.ReportProfileStatsAsChild == TRUE) && (ClientOutput == TRUE))
                MXMLAddChild(PE,(char *)MStatAttr[AList[aindex]],tmpString,NULL);
              else
                MXMLSetAttr(PE,(char *)MStatAttr[AList[aindex]],tmpString,mdfString);
     
              break;
            }  /* END switch (AList[aindex]) */
          }    /* END if (PE != NULL) */
        }      /* END for (aindex) */
      }        /* END BLOCK (case msoNode) */

      break;

    default:

      /* NO-OP */

      break;
    }  /* END switch (SType) */

  return(SUCCESS);
  }  /* END MStatToXML() */


/**
 * Update stat structure from XML.
 *
 * NOTE: this routine is called by server and clients
 *
 * @see MStatToXML() - peer
 *
 * @param SS (I) [modified]
 * @param SType (I)
 * @param E (I)
 */

int MStatFromXML(

  void                 *SS,     /* I (modified) */
  enum MStatObjectEnum  SType,  /* I */
  mxml_t               *E)      /* I */

  {
  switch (SType)
    {
    case msoCred:
      {
      must_t *S = (must_t *)SS;

      int aindex;
     
      char tmpVal[MMAX_BUFFER];
      char tmpLine[MMAX_BUFFER];
     
      enum MStatAttrEnum saindex;
     
      int tmpI;
     
      mxml_t *PE;
     
      if ((S == NULL) || (E == NULL))
        {
        return(FAILURE);
        }
     
      /* NOTE:  do not initialize.  may be overlaying data */
     
      for (aindex = 0;aindex < E->ACount;aindex++)
        {
        saindex = (enum MStatAttrEnum)MUGetIndex(E->AName[aindex],MStatAttr,MBNOTSET,0);
     
        if (saindex == mstaNONE)
          {
          continue;
          }

        if (saindex == mstaIStatCount)
          {
          MUStrCpy(tmpVal,E->AVal[aindex],sizeof(tmpVal));
     
          tmpI = (int)strtol(tmpVal,NULL,10);
     
          if ((MStat.P.MaxIStatCount != 0) && (tmpI != MStat.P.MaxIStatCount))
            {
            return(FAILURE);
            }
     
          continue;
          }

        if (saindex == mstaIStatDuration)
          {
          MUStrCpy(tmpVal,E->AVal[aindex],sizeof(tmpVal));
     
          tmpI = (int)strtol(tmpVal,NULL,10);
     
          /* NOTE:  if MStat.P.IStatDuration == 0 we are calling this from the client (showstats) */
     
          if ((MStat.P.IStatDuration != 0) && (tmpI != MStat.P.IStatDuration))
            {
            return(FAILURE);
            }
          }
     
        if (saindex == mstaGMetric)
          {
          int gindex;

          /* FORMAT: gmetric[.|:]<GMETRICID>=<VAL> */

          if ((E->AName[aindex][strlen(MStatAttr[mstaGMetric])] == ':') ||
              (E->AName[aindex][strlen(MStatAttr[mstaGMetric])] == '.'))
            {
            gindex = MUMAGetIndex(meGMetrics,E->AName[aindex] + strlen("GMetric:"),mAdd);
            
            /* gindex will be greater than zero on success.  Zero on failure */

            if (gindex > 0)
              {
              MStatSetAttr(
                (void *)S,
                msoCred,
                (int)saindex,
                (void **)E->AVal[aindex],
                gindex - 1,
                mdfString,
                mSet);
              }
            }

          continue;
          }  /* END if (saindex == mstaGMetric) */

        MStatSetAttr((void *)S,msoCred,(int)saindex,(void **)E->AVal[aindex],0,mdfString,mSet);
        }  /* END for (aindex) */
     
      /* check for profile child and load those in from xml */
     
      if ((MStat.P.IStatDuration != 0) && (MXMLGetChild(E,"Profile",NULL,&PE) != FAILURE))
        {
        long  EarliestPTime;
        long  tmpL;
       
        int  tmpCount;
        int  SkipCount = -1;
        int  TotalCount = 0;
     
        char *ptr;
        char *TokPtr;
     
        MStatPInitialize((void *)S,FALSE,msoCred);
     
        EarliestPTime = 
          MUGetStarttime(MStat.P.IStatDuration,MSched.Time) - 
          (MStat.P.IStatDuration * MStat.P.MaxIStatCount);
     
        for (aindex = 0;aindex < PE->ACount;aindex++)
          {
          saindex = (enum MStatAttrEnum)MUGetIndex(PE->AName[aindex],MStatAttr,FALSE,0);
     
          if (saindex != mstaStartTime)
            continue;
     
          MUStrCpy(tmpVal,PE->AVal[aindex],sizeof(tmpVal));
     
          MUStrCpy(tmpLine,tmpVal,sizeof(tmpLine));
     
          MStatPStarttimeExpand(tmpLine,sizeof(tmpLine),MStat.P.IStatDuration);
     
          /* count up total number of samples */
     
          SkipCount = 0;
     
          ptr = MUStrTok(tmpLine,",",&TokPtr);
     
          while (ptr != NULL)
            {
            tmpL = strtol(ptr,NULL,10);
     
            if (tmpL < EarliestPTime)
              SkipCount++;
            else
              break;
     
            ptr = MUStrTok(NULL,",",&TokPtr);
            }
     
          break;
          }  /* END for (aindex) */
     
        for (aindex = 0;aindex < PE->ACount;aindex++)
          {
          /* allow matching to extended values, ie gres:x,gmetric:y, etc */

          saindex = (enum MStatAttrEnum)MUGetIndex(
            PE->AName[aindex],
            MStatAttr,
            FALSE,
            0);
     
          if (saindex == mstaNONE)
            {
            /* should log error - NYI */

            continue;
            }

          MUStrCpy(tmpVal,PE->AVal[aindex],sizeof(tmpVal));
     
          MUStrCpy(tmpLine,tmpVal,sizeof(tmpLine));
     
          if (strchr(tmpLine,'*') != NULL)
            {
            MStatPExpand(tmpLine,sizeof(tmpLine));
            MStatPExpand(tmpVal,sizeof(tmpVal));
            }
          else if (saindex == mstaStartTime)
            {
            MStatPStarttimeExpand(tmpLine,sizeof(tmpLine),MStat.P.IStatDuration);
            MStatPStarttimeExpand(tmpVal,sizeof(tmpVal),MStat.P.IStatDuration);
            }
     
          /* count up total number of samples */
     
          if (SkipCount == -1)
            {
            SkipCount = 0;
     
            ptr = MUStrTok(tmpLine,",",&TokPtr);
     
            while (ptr != NULL)
              {
              TotalCount++;
     
              ptr = MUStrTok(NULL,",",&TokPtr);
              }
     
            SkipCount = MAX(0,TotalCount - MStat.P.MaxIStatCount);
            }
    
          tmpCount = SkipCount;
     
          ptr = MUStrTok(tmpVal,",",&TokPtr);
     
          while (tmpCount > 0)
            {
            tmpCount--;
     
            ptr = MUStrTok(NULL,",",&TokPtr);
            }
     
          S->IStatCount = 0;
     
          while (ptr != NULL)
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
            else if (saindex == mstaGMetric)
              {
              int gindex;

              /* FORMAT: gmetric[.|:]<GMETRICID>=<VAL> */

              if ((PE->AName[aindex][strlen(MStatAttr[mstaGMetric])] == ':') ||
                  (PE->AName[aindex][strlen(MStatAttr[mstaGMetric])] == '.'))
                {
                gindex = MUMAGetIndex(meGMetrics,PE->AName[aindex] + strlen("GMetric:"),mAdd);

                MStatSetAttr(
                  (void *)S->IStat[S->IStatCount],
                  msoCred,
                  (int)saindex,
                  (void **)ptr,
                  gindex - 1,
                  mdfString,
                  mSet);
                }
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
     
            if (S->IStatCount >= MStat.P.MaxIStatCount)
              break;
            }
          }   /* END for (aindex) */
        }     /* END if (MXMLGetChild(E,"Profile",NULL,&PE) == FAILURE) */
      }       /* END BLOCK (case msoCred) */

      break;

    case msoNode:
      {
      mnust_t *S = (mnust_t *)SS;

      int aindex;
     
      char tmpVal[MMAX_BUFFER];
      char tmpLine[MMAX_BUFFER];
     
      enum MNodeStatTypeEnum saindex;
     
      int tmpI;
      int StatCount = 0;
     
      mxml_t *PE;
     
      if ((S == NULL) || (E == NULL))
        {
        return(FAILURE);
        }
     
      /* NOTE:  do not initialize.  may be overlaying data */
     
      for (aindex = 0;aindex < E->ACount;aindex++)
        {
        saindex = (enum MNodeStatTypeEnum)MUGetIndex(E->AName[aindex],MNodeStatType,FALSE,0);
     
        if (saindex == mnstNONE)
          {
          continue;
          }
        else if (saindex == mnstIStatCount)
          {
          MUStrCpy(tmpVal,E->AVal[aindex],sizeof(tmpVal));
     
          tmpI = (int)strtol(tmpVal,NULL,10);
     
          if ((MStat.P.MaxIStatCount != 0) && (tmpI != MStat.P.MaxIStatCount))
            {
            return(FAILURE);
            }
     
          continue;
          }
        else if (saindex == mnstIStatDuration)
          {
          MUStrCpy(tmpVal,E->AVal[aindex],sizeof(tmpVal));
     
          tmpI = (int)strtol(tmpVal,NULL,10);
     
          /* NOTE:  if MStat.P.IStatDuration == 0 we are calling this from the client (showstats) */
     
          if ((MStat.P.IStatDuration != 0) && (tmpI != MStat.P.IStatDuration))
            {
            return(FAILURE);
            }
          }
        else if ((saindex == mnstAGRes) ||
                 (saindex == mnstCGRes) ||
                 (saindex == mnstDGRes))
          {
          int gindex;

          /* FORMAT: {ACD}GRes[:|.]<GRESID>=<VAL> */

          if ((E->AName[aindex][strlen(MNodeStatType[mnstAGRes])] == ':') ||
              (E->AName[aindex][strlen(MNodeStatType[mnstAGRes])] == '.'))
            {
            gindex = MUMAGetIndex(meGRes,E->AName[aindex] + strlen("xGRes:"),mAdd);

            MStatSetAttr(
              (void *)S,
              msoCred,
              (int)saindex,
              (void **)E->AVal[aindex],
              gindex - 1,
              mdfString,
              mSet);
            }

          continue;
          }
     
        MStatSetAttr((void *)S,msoNode,(int)saindex,(void **)E->AVal[aindex],0,mdfString,mSet);
        }  /* END for (aindex) */
     
      /* check for profile child and load those in from xml */
     
      if ((MStat.P.IStatDuration != 0) && (MXMLGetChild(E,"Profile",NULL,&PE) != FAILURE))
        {
        long  EarliestPTime;
        long  tmpL;
       
        int  tmpCount;
        int  SkipCount = -1;
        int  TotalCount = 0;
     
        char *ptr;
        char *TokPtr;
     
        MStatPInitialize((void *)S,FALSE,msoNode);
     
        EarliestPTime = 
          MUGetStarttime(MStat.P.IStatDuration,MSched.Time) - 
          (MStat.P.IStatDuration * MStat.P.MaxIStatCount);
     
        for (aindex = 0;aindex < PE->ACount;aindex++)
          {
          saindex = (enum MNodeStatTypeEnum)MUGetIndex(PE->AName[aindex],MNodeStatType,FALSE,0);
     
          if (saindex != mnstStartTime)
            continue;
     
          MUStrCpy(tmpVal,PE->AVal[aindex],sizeof(tmpVal));
     
          MUStrCpy(tmpLine,tmpVal,sizeof(tmpLine));
     
          MStatPStarttimeExpand(tmpLine,sizeof(tmpLine),MStat.P.IStatDuration);
     
          /* count up total number of samples */
     
          SkipCount = 0;
     
          ptr = MUStrTok(tmpLine,",",&TokPtr);
     
          while (ptr != NULL)
            {
            tmpL = strtol(ptr,NULL,10);
     
            if (tmpL < EarliestPTime)
              SkipCount++;
            else
              break;
     
            ptr = MUStrTok(NULL,",",&TokPtr);
            }
     
          break;
          }  /* END for (aindex) */
     
        for (aindex = 0;aindex < PE->ACount;aindex++)
          {
          /* allow matching to extended values, ie gres:x,gmetric:y, etc */

          saindex = (enum MNodeStatTypeEnum)MUGetIndex(
            PE->AName[aindex],
            MNodeStatType,
            TRUE,
            0);
     
          if (saindex == mnstNONE)
            {
            /* should log error - NYI */

            continue;
            }

          MUStrCpy(tmpVal,PE->AVal[aindex],sizeof(tmpVal));
     
          MUStrCpy(tmpLine,tmpVal,sizeof(tmpLine));
     
          if (strchr(tmpLine,'*') != NULL)
            {
            MStatPExpand(tmpLine,sizeof(tmpLine));
            MStatPExpand(tmpVal,sizeof(tmpVal));
            }
          else if (saindex == mnstStartTime)
            {
            MStatPStarttimeExpand(tmpLine,sizeof(tmpLine),MStat.P.IStatDuration);
            MStatPStarttimeExpand(tmpVal,sizeof(tmpVal),MStat.P.IStatDuration);
            }
     
          /* count up total number of samples */
     
          if (SkipCount == -1)
            {
            SkipCount = 0;
     
            ptr = MUStrTok(tmpLine,",",&TokPtr);
     
            while (ptr != NULL)
              {
              TotalCount++;
     
              ptr = MUStrTok(NULL,",",&TokPtr);
              }
     
            SkipCount = MAX(0,TotalCount - MStat.P.MaxIStatCount);
            }

          tmpCount = SkipCount;
     
          ptr = MUStrTok(tmpVal,",",&TokPtr);
     
          while (tmpCount > 0)
            {
            tmpCount--;
     
            ptr = MUStrTok(NULL,",",&TokPtr);
            }
     
          S->IStatCount = 0;
     
          while (ptr != NULL)
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
            else if ((saindex == mnstAGRes) ||
                     (saindex == mnstCGRes) ||
                     (saindex == mnstDGRes))
              {
              int gindex;

              /* FORMAT: {ACD}GRes[:|.]<GRESID>=<VAL> */

              if ((PE->AName[aindex][strlen(MNodeStatType[mnstAGRes])] == ':') ||
                  (PE->AName[aindex][strlen(MNodeStatType[mnstAGRes])] == '.'))
                {
                gindex = MUMAGetIndex(meGRes,PE->AName[aindex] + strlen("xGRes:"),mAdd);
 
                MStatSetAttr(
                  (void *)S->IStat[S->IStatCount],
                  msoNode,
                  (int)saindex,
                  (void **)ptr,
                  gindex,
                  mdfString,
                  mSet);
                }
              }
            else if (saindex == mnstGMetric)
              {
              int gindex;

              /* FORMAT: gmetric[.|:]<GMETRICID>=<VAL> */

              if ((PE->AName[aindex][strlen(MNodeStatType[mnstGMetric])] == ':') ||
                  (PE->AName[aindex][strlen(MNodeStatType[mnstGMetric])] == '.'))
                {
                gindex = MUMAGetIndex(meGMetrics,PE->AName[aindex] + strlen("GMetric:"),mAdd);

                MStatSetAttr(
                  (void *)S->IStat[S->IStatCount],
                  msoNode,
                  (int)saindex,
                  (void **)ptr,
                  gindex - 1,
                  mdfString,
                  mSet);
                }
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
                (int)saindex,
                (void **)ptr,
                0,
                mdfString,
                mSet);
              }
     
            S->IStatCount++;
     
            ptr = MUStrTok(NULL,",",&TokPtr);
     
            if (S->IStatCount >= MStat.P.MaxIStatCount)
              break;
            }

          if (S->IStatCount > StatCount)
            StatCount = S->IStatCount;
          }   /* END for (aindex) */

        S->IStatCount = StatCount;
        }     /* END if (MXMLGetChild(E,"Profile",NULL,&PE) == FAILURE) */
      }       /* END case msoNode */

      break;

    default:

      /* not handled */

      return(FAILURE);

      /*NOTREACHED*/

      break;
    }  /* END switch (SType) */

  return(SUCCESS);
  }  /* END MStatFromXML() */


/**
 * Merge stat XML.
 *
 * @see MStatMerge()
 *
 * @param Dst (I)
 * @param Src (I) [may be NULL]
 * @param Delim (I)
 */

int MStatMergeXML(

  mxml_t  *Dst,   /* I */
  mxml_t  *Src,   /* I (may be NULL) */
  char     Delim) /* I */

  {
  int cindex;
  int cindex2;

  int aindex;

  mxml_t *CE = NULL;

  const char *FName = "MStatMergeXML";

  MDB(7,fSCHED) MLog("%s(Dst,Src,%c)\n",
    FName,
    Delim);

  /* parent and child must have identical element structure */

  if ((Dst == NULL) || (Src == NULL))
    {
    /* do not modify destination */

    return(FAILURE);
    }

  /* NOTE:  Dst and Src should have same structure */

  /* NOTE:  Dst and Src may have different numbers of children */

  /*        children may include multiple instance of users, groups, 
            accounts, etc where each child may have a unique 'ID' attribute */

  for (cindex = 0;cindex < Src->CCount;cindex++)
    {
    /* locate match - must be profile object or have ID (AVal[0]) match */

    if ((Dst->C != NULL) && 
        (Dst->CCount > cindex) && 
        (Dst->C[cindex]->Name != NULL) &&
        !strcmp(Dst->C[cindex]->Name,Src->C[cindex]->Name) &&
        (!strcmp(Dst->C[cindex]->Name,"Profile") ||
       ((Src->C[cindex]->ACount >= 1) &&
        (Dst->C[cindex]->ACount >= 1) &&
        !strcmp(Dst->C[cindex]->AVal[0],Src->C[cindex]->AVal[0]))))
      {
      /* matching children located at same offset */
 
      if (MStatMergeXML(
           Dst->C[cindex],    /* I (modified) */
           Src->C[cindex],    /* I */
           Delim) == FAILURE)
        {
        return(FAILURE);
        }

      continue;
      }

    /* attempt to locate matching child */

    for (cindex2 = 0;cindex2 < Dst->CCount;cindex2++)
      {
      if ((Dst->C != NULL) &&
          (Dst->CCount > cindex2) &&
          (Dst->C[cindex2]->Name != NULL) &&
          !strcmp(Dst->C[cindex2]->Name,Src->C[cindex]->Name) &&
          (!strcmp(Dst->C[cindex2]->Name,"Profile") ||
         ((Src->C[cindex]->ACount >= 1) &&
          (Dst->C[cindex2]->ACount >= 1) &&
          !strcmp(Dst->C[cindex2]->AVal[0],Src->C[cindex]->AVal[0]))))
        {
        /* matching children located at different offset */

        if (MStatMergeXML(
            Dst->C[cindex2],    /* I (modified) */
            Src->C[cindex],    /* I */
            Delim) == FAILURE)
          {
          return(FAILURE);
          }

        break;
        }
      }    /* END for (cindex2) */

    if (cindex2 < Dst->CCount)
      {
      /* matching child located */

      continue;
      }

    /* matching destination child not located, add source */

    if (MXMLDupE(Src->C[cindex],&CE) == FAILURE)
      {
      return(FAILURE);
      }

    MXMLAddE(Dst,CE);
    }  /* END for (cindex) */

  /* merge all attributes */

  for (aindex = 0;aindex < Dst->ACount;aindex++)
    {
    if (aindex >= Src->ACount)
      {
      /* XML version mismatch */

      break;
      }

    if (!strcmp(Src->AName[aindex],MProfAttr[mpraCount]))
      {
      int tmpI1;
      int tmpI2;

      tmpI1 = (int)strtol(Src->AVal[aindex],NULL,10);
      tmpI2 = (int)strtol(Dst->AVal[aindex],NULL,10);

      tmpI1 += tmpI2;

      MXMLSetAttr(Dst,(char *)MProfAttr[mpraCount],(void *)&tmpI1,mdfInt);
      }
    else if (!strcmp(Src->AName[aindex],MProfAttr[mpraMaxCount]))
      {
      int tmpI1;
      int tmpI2;

      tmpI1 = (int)strtol(Src->AVal[aindex],NULL,10);
      tmpI2 = (int)strtol(Dst->AVal[aindex],NULL,10);

      tmpI1 = MAX(tmpI1,tmpI2);

      MXMLSetAttr(Dst,(char *)MProfAttr[mpraMaxCount],(void *)&tmpI1,mdfInt);
      }
    else if (!strcmp(Src->AName[aindex],"ID"))
      {
      /* NO-OP */
      }
    else
      {
      MXMLAppendAttr(
        Dst,
        Src->AName[aindex],
        (char *)Src->AVal[aindex],
        Delim);
      }
    }    /* END for (aindex) */

  return(SUCCESS);
  }  /* MStatMergeXML() */


/* END MStatsXML.c */
