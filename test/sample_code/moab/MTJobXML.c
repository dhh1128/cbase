/* HEADER */

/**
 * @file MTJobXML.c
 *
 * New routines to do XML to/from template jobs
 */

 
#include "moab.h"
#include "moab-proto.h"
#include "moab-global.h"
#include "moab-const.h"



/**
 * convert an mtjobstat_t struct to XML. Currently converts F, S, and learning
 * data
 *
 * @param JStat        (I)
 * @param ClientOutput (I)
 * @param Out          (O) will contain newly-created XML element
 */

int MTJobStatToXML(

  mtjobstat_t const *JStat,
  mbool_t            ClientOutput,
  mxml_t           **Out)

  {
  mxml_t *RootNode;
  mxml_t *Node;

  *Out = NULL;

  MXMLCreateE(&RootNode,"mtjobstat");

  if (MFSToXML((mfs_t *)&JStat->F,&Node,NULL) == SUCCESS)
    {
    if ((Node->ACount == 0) && (Node->CCount == 0))
      /* do not use empty element */
      MXMLDestroyE(&Node);
    else
      MXMLAddE(RootNode,Node);
    }

  if (MStatToXML(
       (void *)&JStat->S,
       msoCred,
       &Node,
       ClientOutput,
       NULL) != FAILURE)
    {
    if ((Node->ACount == 0) && (Node->CCount == 0))
      /* do not use empty element */
      MXMLDestroyE(&Node);
    else
      MXMLAddE(RootNode,Node);
    }

  *Out = RootNode;
  return (SUCCESS);
  } /* END MJobStatToXML() */



/**
 * loads the XML in JStatNode into OutJStat
 *
 * @see MJobStatToXML()
 *
 * @param JStatNode (I)
 * @param OutJStat  (O)
 */

int MTJobStatFromXML(

  mxml_t const *JStatNode,
  mtjobstat_t   *OutJStat)

  {
  int CTok = -1;
  mxml_t *ChildNode;

  while (MXMLGetChild(JStatNode,NULL,&CTok,&ChildNode) == SUCCESS)
     {
     if (!strcmp(ChildNode->Name,"fs"))
       {
       MFSFromXML(&OutJStat->F,ChildNode);
       }
     else if (!strcmp(ChildNode->Name,"stats"))
       {
       MStatFromXML(&OutJStat->S,msoCred,ChildNode);
       }
     else
       {
       MDB(3,fSTRUCT) MLog("ERROR:    %s is not a valid template job stat child "
           "element\n",
           ChildNode->Name);
       }
     }

  if (OutJStat->S.IStat == NULL)
    {
    MStatPInitialize(((void *)&OutJStat->S),TRUE,msoCred);
    }

  return (SUCCESS);
  } /* END MTJobStatFromXML() */

/* END MTJob.c */  

/* vim: set ts=2 sw=2 expandtab nocindent filetype=c: */
/**
 * Report template job profile to XML.
 *
 * @see MCredProfileToXML() - sync
 * @see MStatPToSummaryString() - child
 * @see MStatPToString() - child 
 * @see MStatValidatePData() - child
 * @see MNodeProfileToxML() - sync
 * @see MStatWritePeriodPStats() - parent
 * @see MDBReadGeneralStatsRange() - child
 *
 * @param J            (I) [optional]
 * @param JName        (I) [optional]
 * @param SName        (I) [optional]
 * @param StartTime    (I)
 * @param EndTime      (I)
 * @param Time         (I)
 * @param DoSummary    (I)
 * @param ClientOutput (I)
 * @param E            (O) [modified]
 */

int MTJobStatProfileToXML(

  mjob_t   *J,
  char     *JName,
  char     *SName,
  mulong    StartTime,
  mulong    EndTime,
  long      Time,
  mbool_t   DoSummary,
  mbool_t   ClientOutput,
  mxml_t   *E)

  {
  mxml_t *OE;
  mxml_t *PE;

  must_t *SPtr;

  enum MStatAttrEnum SIndex;

  int sindex;
  int tjindex;

  char tmpBuf[MMAX_BUFFER];

  mjob_t *TJ;

  must_t  **NewS = NULL;

  int     PIndex;
  int     PDuration;

  /* number of profiles to print out (calculated) */
  int     Iterations;

  mdb_t * MDBInfo;

  const enum MStatAttrEnum AList[] = {
    mstaDuration,
    mstaTNL,
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
    mstaNONE };

  const char *FName = "MTJobStatProfileToXML";

  MDB(3,fSCHED) MLog("%s(%s,%s,%s,%lu,%lu,%ld,%s,E)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (JName != NULL) ? JName : "",
    (SName != NULL) ? SName : "",
    StartTime,
    EndTime,
    Time,
    MBool[DoSummary]);

  if ((E == NULL) || (MStat.P.IStatDuration <= 0) || (StartTime > MSched.Time))
    {
    return(FAILURE);
    }

  /* enforce time constraints */

  PIndex    = 0;
  PDuration = MStat.P.IStatDuration;

  if (MStat.P.IStatDuration == 0)
    {
    /* no information collected */

    return(SUCCESS);
    }

  Iterations = (EndTime - StartTime) / PDuration;

  /* FIXME:  why do we need to do this, round up?  */

  Iterations++;

  /* get all profile data for credential type within timeframe */

  SIndex = (enum MStatAttrEnum)MUGetIndexCI(SName,MStatAttr,FALSE,mstaNONE);

  if (MSched.CredDiscovery == TRUE)
    {
    MStatValidatePData(
      NULL,
      mxoJob,
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

  for (tjindex = 0;MUArrayListGet(&MTJob,tjindex) != NULL;tjindex++)
    {
    TJ = *(mjob_t **)(MUArrayListGet(&MTJob,tjindex));

    if (TJ == NULL)
      break;

    if (TJ->ExtensionData == NULL)
      continue;

    if ((J != NULL) && (TJ != J))
      continue;

    if ((JName != NULL) && strcmp(TJ->Name,JName))
      continue;

    SPtr = &((mtjobstat_t *)TJ->ExtensionData)->S;

    if (SPtr->IStat == NULL)
      {
      /* comment out - DAVE - TEMP */

      MDB(4,fSCHED) MLog("INFO:     NULL IStat pointer detected for template job '%s'\n",
        TJ->Name);

      /* continue; */
      }

    OE = NULL;

    MXMLCreateE(&OE,(char *)MXO[mxoJob]);

    MXMLSetAttr(
      OE,
      (char *)MJobAttr[mjaJobID],
      (void **)TJ->Name,
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

    MXMLSetAttr(
      PE,
      (char *)MProfAttr[mpraCount],
      (void *)&SPtr->IStatCount,
      mdfInt);

    MXMLSetAttr(
      PE,
      (char *)MProfAttr[mpraMaxCount],
      (void *)&MStat.P.MaxIStatCount,
      mdfInt);

    if (MStat.P.IStatDuration > 0)
      {
      MXMLSetAttr(PE,"SpecDuration",(void *)&MStat.P.IStatDuration,mdfLong);
      }
    else
      {
      int tmpI = MDEF_PSTEPDURATION;

      MXMLSetAttr(PE,"SpecDuration",(void *)&tmpI,mdfLong);
      }

    MXMLAddE(OE,PE);

    /* need stat type */

    MSchedGetAttr(msaDBHandle,mdfNONE,(void **)&MDBInfo,-1);

    if ((MCP.UseDatabase == TRUE) && (MDBInfo->DBType != mdbNONE))
      {
      must_t tmpStat;
      int IStatDuration;

      if (MStat.P.IStatDuration > 0)
        {
        IStatDuration = MStat.P.IStatDuration;
        }
      else
        {
        IStatDuration = MDEF_PSTEPDURATION;
        }

      memset(&tmpStat,0,sizeof(tmpStat));

      if (MDBReadGeneralStatsRange(
            MDBInfo,
            &tmpStat,
            mxoJob,
            TJ->Name,
            IStatDuration,
            StartTime,
            EndTime,
            NULL) == FAILURE)
        {
        /* failure in database query */

        MDB(3,fALL) MLog("WARNING:  could not retrieve statistics from database\n");

        return(FAILURE);
        }

      NewS = tmpStat.IStat;
      }
    else
      {
      NewS = (must_t **)MUCalloc(1,sizeof(must_t *) * Iterations);

      if (MStatValidatePData(
            (void *)TJ,
            mxoJob,
            TJ->Name,
            SPtr,        /* I */
            NewS,        /* O */
            StartTime,
            EndTime,
            Time,
            FALSE,
            Iterations,
            NULL) == FAILURE)
        {
        /* cannot generate valid data */

        MDB(3,fALL) MLog("WARNING:  cannot generate valid stat data for external query\n");

        return(FAILURE);
        }
      }

    for (sindex = 0;AList[sindex] != mstaNONE;sindex++)
      {
      if ((SIndex != mstaNONE) && (AList[sindex] != SIndex))
        continue;

      if (NewS[PIndex] == NULL)
        continue;

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
          tmpBuf,
          sizeof(tmpBuf));
        }

      if (tmpBuf[0] == '\0')
        continue;

      if ((MSched.ReportProfileStatsAsChild == TRUE) && (ClientOutput == TRUE))
        MXMLAddChild(PE,(char *)MStatAttr[AList[sindex]],tmpBuf,NULL);
      else
        MXMLSetAttr(PE,(char *)MStatAttr[AList[sindex]],tmpBuf,mdfString);
      }  /* END for (sindex) */

    MStatPDestroy((void ***)&NewS,msoCred,Iterations);

    MXMLAddE(E,OE);
    }  /* END for (tjindex) */

  return(SUCCESS);
  }  /* END MTJobStatProfileToXML() */


/**
 * Translate a given template job (mjob_t) to XML format (mxml_t).
 *
 * @see MTJobStoreCP() - parent
 *
 * @param J       (I)
 * @param JEP     (O) - NULL or existing mxml_t object [possibly allocated]
 * @param NullVal (I) [optional]
 * @param IsCP    (I)
 */

int MTJobToXML(

  mjob_t   *J,
  mxml_t  **JEP,
  char     *NullVal,
  mbool_t   IsCP)

  {
  mxml_t *JE;

  if (*JEP == NULL)
    {
    if (MXMLCreateE(JEP,(char *)MXO[mxoJob]) == FAILURE)
      {
      return(FAILURE);
      }
    }

  JE = *JEP;

  MXMLSetAttr(JE,(char *)MJobAttr[mjaJobID],J->Name,mdfString);

  if (J->TemplateExtensions != NULL)
    {
    mxml_t *XE = NULL;

    MXMLAddE(JE,XE);
    }

  if ((J->ExtensionData != NULL) && (bmisset(&J->IFlags,mjifTemplateStat)))
    {
    mtjobstat_t *JStat = (mtjobstat_t *)J->ExtensionData;
    mxml_t *StatNode;

    /* allocate memory for XLoad before it is used */

    if (JStat->S.XLoad == NULL)
      JStat->S.XLoad = (double *)MUCalloc(1,sizeof(double) * MSched.M[mxoxGMetric]);

    if (MTJobStatToXML(JStat,(IsCP == TRUE) ? FALSE : TRUE,&StatNode) == SUCCESS)
      {
      MXMLAddE(JE,StatNode);
      }
    }

  return(SUCCESS);
  }  /* END MTJobToXML() */



int MJobTMatchToXML(
  
  mjtmatch_t  *JT,      /* I */
  mxml_t     **JEP,     /* O */
  char        *NullVal) /* I */

  {
  int jindex;

  mxml_t *JE;

  if ((JT == NULL) || (JEP == NULL))
    {
    return(FAILURE);
    }

  if (*JEP == NULL)
    {
    if (MXMLCreateE(JEP,(char *)"jobmatch") == FAILURE)
      {
      return(FAILURE);
      }
    }

  JE = *JEP;

  MXMLSetAttr(JE,(char *)MJobAttr[mjaJobID],JT->Name,mdfString);

  if (JT->JMax != NULL)
    MXMLSetAttr(JE,(char *)MJTMatchAttr[mjtmaJMax],JT->JMax->Name,mdfString);

  if (JT->JMin != NULL)
    MXMLSetAttr(JE,(char *)MJTMatchAttr[mjtmaJMin],JT->JMin->Name,mdfString);

  if (JT->JDef != NULL)
    MXMLSetAttr(JE,(char *)MJTMatchAttr[mjtmaJDef],JT->JDef->Name,mdfString);

  if (JT->JSet != NULL)
    MXMLSetAttr(JE,(char *)MJTMatchAttr[mjtmaJSet],JT->JSet->Name,mdfString);

  jindex = 0;

  while (jindex < MMAX_JTSTAT)
    {
    if (JT->JStat[jindex] == NULL)
      break;

    MXMLSetAttr(JE,(char *)MJTMatchAttr[mjtmaJStat],JT->JStat[jindex]->Name,mdfString);

    jindex++;
    }

  return(SUCCESS);
  }  /* END MJobTMatchToXML() */

