/* HEADER */

      
/**
 * @file MNodeProfileToXML.c
 *
 * Contains: MNodeProfileToXML
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 * Report node profile to XML (supports 'mnodectl -q profile'.)
 *
 * @see MCredProfileToXML() - sync
 * @see MStatPToSummaryString() - child
 * @see MStatPToString() - child
 * @see MTJobStatProfileToxML() - sync
 * @see MUINodeCtl() - parent
 *
 * @param Name         (I) [optional]
 * @param N            (I) [optional]
 * @param SName        (I) [optional]
 * @param NAList       (I) [optional]
 * @param StartTime    (I)
 * @param EndTime      (I)
 * @param Time         (I)
 * @param E            (O) [modified]
 * @param ClientOutput (I)
 * @param DoSummary    (I)
 */

int MNodeProfileToXML(

  char               *Name,
  mnode_t            *N,
  char               *SName,
  enum MNodeAttrEnum *NAList,
  long                StartTime,
  long                EndTime,
  long                Time,
  mxml_t             *E,
  mbool_t             ClientOutput,
  mbool_t             DoSummary,
  mbool_t             ClientOnly)

  {
  mxml_t *NE;
  mxml_t *PE;
  mxml_t *SE;

  mnust_t *SPtr;

  marray_t NodeList;
  
  enum MNodeStatTypeEnum SIndex;

  int  nindex;
  int  sindex;

  char tmpBuf[MMAX_BUFFER];
  char tmpName[MMAX_LINE];

  mnust_t  **NewS = NULL;

  int     PIndex;
  int     PDuration;

  /* number of profiles to print out (calculated) */
  int     Iterations;

  mdb_t *MDBInfo;

  const enum MNodeStatTypeEnum AList[] = {
    mnstAGRes,
    mnstAProc,
    mnstCat,
    mnstCGRes,
    mnstCProc,
    mnstCPULoad,
    mnstDGRes,
    mnstDProc,
    mnstIterationCount,
    mnstMaxCPULoad,
    mnstMinCPULoad,
    mnstMaxMem,
    mnstMinMem,
    mnstNodeState,
    mnstQueuetime,
    mnstRCat,
    mnstStartTime,
    mnstUMem,
    mnstXLoad,
    mnstXFactor,
    mnstNONE };

  if (E == NULL)
    {
    return(FAILURE);
    }

  /* enforce time constraints */

  PIndex    = 0;
  PDuration = MStat.P.IStatDuration;

  if (EndTime < (long)MSched.IntervalStart)
    {
    Iterations = (EndTime - StartTime) / PDuration;

    Iterations++;
    }
  else
    {
    /* don't allow the current interval to be included, since it has no stats */

    Iterations = ((long)MSched.IntervalStart - StartTime) / PDuration;
    }

  if (Iterations <= 0)
    {
    return(FAILURE);
    }

  /* get all profile data for credential type within timeframe */

  SIndex = (enum MNodeStatTypeEnum)MUGetIndexCI(SName,MNodeStatType,FALSE,mstaNONE);

  MUArrayListCreate(&NodeList,sizeof(mnode_t *),-1);

  if (N == NULL)
    {
    char tmpMsg[MMAX_LINE];

    tmpMsg[0] = '\0';

    if (MUREToList(
          Name,
          mxoNode,
          NULL,
          &NodeList,
          FALSE,
          tmpMsg,
          sizeof(tmpMsg)) == FAILURE)
      {
      MUArrayListFree(&NodeList);

      return(FAILURE);
      }
    }   /* END if (N == NULL) */
  else
    {
    MUArrayListAppendPtr(&NodeList,N);
    }

  for (nindex = 0;nindex < NodeList.NumItems;nindex++)
    {
    N = (mnode_t *)MUArrayListGetPtr(&NodeList,nindex);

    if ((MOGetComponent((void *)N,mxoNode,(void **)&SPtr,mxoxStats) == FAILURE) ||
        (SPtr->IStat == NULL))
      {
      continue;
      }

    NE = NULL;

    MXMLCreateE(&NE,(char *)MXO[mxoNode]);

    MXMLSetAttr(
      NE,
      (char *)MNodeAttr[mnaNodeID],
      N->Name,
      mdfString);

    SE = NULL;

    MXMLCreateE(&SE,(char *)MXO[mxoxStats]);

    MXMLAddE(NE,SE);

    PE = NULL;

    MXMLCreateE(&PE,(char *)MStatAttr[mstaProfile]);

    MXMLSetAttr(PE,(char *)MProfAttr[mpraCount],(void *)&SPtr->IStatCount,mdfInt);
    MXMLSetAttr(PE,(char *)MProfAttr[mpraMaxCount],(void *)&MStat.P.MaxIStatCount,mdfInt);

    if (MStat.P.IStatDuration > 0)
      {
      MXMLSetAttr(PE,"SpecDuration",(void *)&MStat.P.IStatDuration,mdfLong);
      }
    else
      {
      int tmpI = MDEF_PSTEPDURATION;

      MXMLSetAttr(PE,"SpecDuration",(void *)&tmpI,mdfLong);
      }

    MXMLAddE(SE,PE);

    /* need stat type */

    MSchedGetAttr(msaDBHandle,mdfNONE,(void **)&MDBInfo,-1);

    if (MDBInfo->DBType != mdbNONE)
      {
      mnust_t tmpStat;
      
      memset(&tmpStat,0,sizeof(tmpStat));
      
      memset(&tmpStat,0,sizeof(tmpStat));

      PROP_FAIL(MDBReadNodeStatsRange(
        MDBInfo,
        &tmpStat,
        N->Name,
        MStat.P.IStatDuration,
        StartTime,
        EndTime,
        NULL));

      NewS = tmpStat.IStat;

      tmpStat.IStat      = NULL; 
      tmpStat.IStatCount = 0;
      }
    else 
      {
      NewS = (mnust_t **)MUCalloc(1,sizeof(mnust_t *) * Iterations);

      MStatValidateNPData(N,SPtr,NewS,StartTime,EndTime,Time,Iterations);
      }

    for (sindex = 0;AList[sindex] != mnstNONE;sindex++)
      {
      if ((SIndex != mnstNONE) && (AList[sindex] != SIndex))
        continue;

      if (NewS[PIndex] == NULL)
        continue;

      if (!MStatIsProfileAttr(msoNode,AList[sindex]))
        continue;

      if ((AList[sindex] == mnstCGRes) || 
          (AList[sindex] == mnstAGRes) || 
          (AList[sindex] == mnstDGRes))
        {
        char ResTypeChar;
        int gindex;

        switch (AList[sindex])
          {
          case mnstAGRes:
            ResTypeChar = 'A';
            break;

          case mnstCGRes:
            ResTypeChar = 'C';
            break;

          case mnstDGRes:
            ResTypeChar = 'D';
            break;

          default:
            assert(FALSE);
            ResTypeChar = 'C';

            break;
          }

        /* loop through all generic resources */

        for (gindex = 1;gindex < MSched.M[mxoxGRes];gindex++)
          {
          if (MGRes.Name[gindex][0] == '\0')
            break;

          if (DoSummary == FALSE)
            {
            MStatPToString(
              (void **)NewS,
              msoNode,
              Iterations,
              (int)AList[sindex],
              gindex,
              tmpBuf,          /* O:  comma delimited list of values */
              sizeof(tmpBuf));
            }
          else
            {
            MStatPToSummaryString(
              (void **)NewS,
              msoNode,
              Iterations,
              (int)AList[sindex],
              gindex,
              tmpBuf,          /* O:  comma delimited list of values */
              sizeof(tmpBuf));
            }
   
          if (tmpBuf[0] == '\0')
            continue;

          snprintf(tmpName,sizeof(tmpName),"%cGRes.%s",
            ResTypeChar,
            MGRes.Name[gindex]);

          if (MSched.ReportProfileStatsAsChild == TRUE)
            MXMLAddChild(PE,tmpName,tmpBuf,NULL);
          else
            MXMLSetAttr(PE,tmpName,(void *)tmpBuf,mdfString);
          }  /* END for (gindex) */
        }    /* END if ((AList[sindex] == mnstCGRes) || ...) */
      else if (AList[sindex] == mnstXLoad)
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
              msoNode,
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
              msoNode,
              Iterations,
              (int)AList[sindex],
              xindex,
              tmpBuf,          /* O:  comma delimited list of values */
              sizeof(tmpBuf));
            }

          if (tmpBuf[0] == '\0')
            continue;

          snprintf(tmpName,sizeof(tmpName),"GMetric.%s",
            MGMetric.Name[xindex]);

          if (MSched.ReportProfileStatsAsChild == TRUE)
            MXMLAddChild(PE,tmpName,tmpBuf,NULL);
          else
            MXMLSetAttr(PE,tmpName,(void *)tmpBuf,mdfString);
          }  /* END for (xindex) */
        }    /* END else if (AList[sindex] = mnstXLoad) */
      else
        {
        if (DoSummary == FALSE)
          {
          MStatPToString(
            (void **)NewS,
            msoNode,
            Iterations,
            (int)AList[sindex],
            0,
            tmpBuf,          /* O:  comma delimited list of values */
            sizeof(tmpBuf));
          }
        else
          {
          MStatPToSummaryString(
            (void **)NewS,
            msoNode,
            Iterations,
            (int)AList[sindex],
            0,
            tmpBuf,          /* O:  comma delimited list of values */
            sizeof(tmpBuf));
          }

        if (tmpBuf[0] == '\0')
          continue;

        if (MSched.ReportProfileStatsAsChild == TRUE)
          MXMLAddChild(PE,(char *)MNodeStatType[AList[sindex]],tmpBuf,NULL);
        else
          MXMLSetAttr(PE,(char *)MNodeStatType[AList[sindex]],(void *)tmpBuf,mdfString);
        }
      }      /* END for (sindex) */

    MStatPDestroy((void ***)&NewS,msoNode,Iterations);

    if (NAList != NULL)
      {
      mbitmap_t BM;

      int aindex;

      mstring_t tmpString(MMAX_LINE);;

      bmset(&BM,mcmXML);

      for (aindex = 0;NAList[aindex] != mnaNONE;aindex++)
        {
        if ((MNodeAToString(N,NAList[aindex],&tmpString,&BM) == FAILURE) ||
            (MUStrIsEmpty(tmpString.c_str())))
          {
          continue;
          }
 
        MXMLSetAttr(NE,(char *)MNodeAttr[NAList[aindex]],tmpString.c_str(),mdfString);
        }  /* END for (aindex) */
      }    /* END if (AList != NULL) */

    MXMLAddE(E,NE);
    }  /* END for (nindex) */

  MUArrayListFree(&NodeList);

  return(SUCCESS);
  }  /* END MNodeProfileToXML() */
/* END MNodeProfileToXML.c */
