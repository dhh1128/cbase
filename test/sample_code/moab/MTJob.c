/* HEADER */

/**
 * @file MTJob.c
 *
 * Contains Job Template functions
 */


#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  




/**
 * Takes a job structure and expresses its data in an XML format in preparation for
 * saving it in a persistent checkpoint file. 
 *
 * This routine is customized to accept Template type jobs.
 *
 * @param J (I) The job to convert to XML for checkpointing.
 * @param Buf (O) The buffer where the XML will be saved.
 * @param BufSize (I) The size of the passed in buffer.
 * @param StoreComplete (I) Whether or not a complete job XML string should be created or a smaller, more
 * brief version should be output.
 */

int MTJobStoreCP(

  mjob_t *J,
  char   *Buf,
  int     BufSize,
  mbool_t StoreComplete)

  {
  mxml_t *JE = NULL;

  if (Buf != NULL)
    {
    Buf[0] = '\0';
    }

  if ((J == NULL) || (Buf == NULL))
    {
    return(FAILURE);
    }

  MTJobToXML(
    J,
    &JE,
    NULL,
    TRUE);

  if (MXMLToString(JE,Buf,BufSize,NULL,TRUE) == FAILURE)
    {
    Buf[0] = '\0';
  
    MXMLDestroyE(&JE);
                                                                                
    return(FAILURE);
    }

  MXMLDestroyE(&JE);

  return(SUCCESS);
  }  /* END MTJobStoreCP() */








/**
 * Load checkpointed job template attributes.
 *
 * @see MCPRestore()
 *
 * @param JP          (I) [optional]
 * @param SBuf        (I) [optional]
 * @param DynamicTJob (I)
 */

int MTJobLoadCP(

  mjob_t  *JP,
  const char    *SBuf,
  mbool_t  DynamicTJob)
 
  {
  char    tmpName[MMAX_NAME + 1];
  char    TJobName[MMAX_NAME + 1];

  const char   *ptr;
 
  mjob_t *J=NULL;
 
  long    CkTime;

  const char   *Buf;

  mxml_t *E = NULL;

  const char *FName = "MTJobLoadCP";
 
  MDB(4,fCKPT) MLog("%s(JP,%s)\n",
    FName,
    (SBuf != NULL) ? SBuf : "NULL");

  Buf = (SBuf != NULL) ? SBuf : MCP.Buffer;

  if (Buf == NULL)
    {
    return(FAILURE);
    }
 
  /* FORMAT:  <JOBHEADER> <JOBID> <CKTIME> <JOBSTRING> */
 
  /* determine job name */
 
  sscanf(Buf,"%64s %64s %ld",
    tmpName,
    TJobName,
    &CkTime);
 
  if (((long)MSched.Time - CkTime) > (long)MCP.CPExpirationTime)
    {
    return(SUCCESS);
    }

  if (JP == NULL)
    {
    if (DynamicTJob == TRUE)
      {
      MTJobAdd(TJobName,&J);

      bmset(&J->IFlags,mjifTemplateIsDynamic);
      }
    else if (MTJobFind(TJobName,&J) != SUCCESS)
      {
      MDB(5,fCKPT) MLog("INFO:     job '%s' no longer detected\n",
        TJobName);
 
      return(SUCCESS);
      }
    }
  else
    {
    J = JP;
    }

  if ((ptr = strchr(Buf,'<')) == NULL)
    {
    return(FAILURE);
    }
 
  MXMLFromString(&E,ptr,NULL,NULL);

  MJobFromXML(J,E,FALSE,mSet);

  if (bmisset(&J->IFlags,mjifTemplateIsDynamic))
    {
    bmcopy(&J->Flags,&J->SpecFlags);
    }
 
  MXMLDestroyE(&E);
 
  return(SUCCESS);
  }  /* END MTJobLoadCP() */





/**
 * Convert string to corresponding job flag bitmap.
 *
 * @see MJobFlagsToString()
 *
 * @param J (I) [optional]
 * @param RQ (I) [optional]
 * @param SFlagsP (O) [optional]
 * @param ReqRID (O) [optional]
 * @param SDelim (I) [optional]
 * @param FString (I)
 * @param DoInitialize (I)
 */

int MJobFlagsFromString(

  mjob_t         *J,            /* I (optional,modified) */
  mreq_t         *RQ,           /* I (optional,modified) */
  mbitmap_t      *SFlagsP,      /* O (optional) */
  char          **ReqRID,       /* O (optional) */
  const char     *SDelim,       /* I (optional) */
  char           *FString,      /* I */
  mbool_t         DoInitialize) /* I */

  {
  char *ptr;
  char *TokPtr;

  char  TokChar;

  const char *Delim;

  const char *DDelim = ",:";

  char tmpLine[MMAX_LINE];

  enum MJobFlagEnum FIndex;

  mbitmap_t *FlagsP;

  if (FString == NULL)
    {
    return(FAILURE);
    }

  if (SFlagsP != NULL)
    {
    FlagsP = SFlagsP;
    }
  else if (J != NULL)
    {
    FlagsP = &J->SpecFlags;
    }
  else
    {
    return(FAILURE);
    }

  if (DoInitialize == TRUE)
    {
    bmclear(FlagsP);
    }

  MUStrCpy(tmpLine,FString,sizeof(tmpLine));

  Delim = (SDelim != NULL) ? SDelim : DDelim;

  ptr = MUStrTok(tmpLine,Delim,&TokPtr);

  while (ptr != NULL)
    {
    FIndex = (enum MJobFlagEnum)MUGetIndexCI(ptr,MJobFlags,MBNOTSET,mjfNONE);

    if (FIndex == mjfNONE)
      {
      /* allow specification of internal flags */

      if (J != NULL)
        {
        if (!strcasecmp(ptr,"EXITONTRIGCOMPLETION"))
          bmset(&J->IFlags,mjifExitOnTrigCompletion);
        else if (!strcasecmp(ptr,"NOIMPLICITCANCEL"))
          bmset(&J->IFlags,mjifNoImplicitCancel);
        }

      ptr = MUStrTok(NULL,Delim,&TokPtr);

      continue;
      }

    bmset(FlagsP,FIndex);

    MDB(5,fCONFIG) MLog("INFO:     flag %s set for job %s\n",
      MJobFlags[FIndex],
      (J != NULL) ? J->Name : "");

    TokChar = FString[ptr + strlen(MJobFlags[FIndex]) - tmpLine];

    switch (FIndex)
      {
      case mjfAdvRsv:

        if (TokChar == ':')
          {
          if ((TokPtr == NULL) || (TokPtr[0] == '\0'))
            break;

          /* verify next token is not flag */

          if (MUGetIndexCI(TokPtr,MJobFlags,MBNOTSET,mjfNONE) == mjfNONE)
            {
            /* HACK:  flags=advres:rsvid overloads ':' delimiter */

            /* advance to next token */

            ptr = MUStrTok(NULL,Delim,&TokPtr);

            if ((J != NULL) && (ptr[0] == '!'))
              {
              bmset(&J->IFlags,mjifNotAdvres);

              ptr++;
              }

            if (ReqRID != NULL)
              MUStrDup(ReqRID,ptr);
            else if (J != NULL)
              MUStrDup(&J->ReqRID,ptr);

            MDB(5,fCONFIG) MLog("INFO:     reservation %s selected\n",
              ptr);
            }
          }

        break;

      case mjfCancelOnExitCode:

        if (TokChar == ':')
          {
          if ((TokPtr == NULL) || (TokPtr[0] == '\0'))
            break;

          /* verify next token is not flag */

          if (MUGetIndexCI(TokPtr,MJobFlags,MBNOTSET,mjfNONE) == mjfNONE)
            {     

            /* advance to next token */
            ptr = MUStrTok(NULL,Delim,&TokPtr);

            if (J != NULL)
              {
              char *tmpS = NULL;

              bmset(&J->Flags,mjfCancelOnExitCode);

              MDB(5,fCONFIG) MLog("INFO:     setting array exit codes %s for array job %s\n",
                ptr,
                J->Name);

              /* check to see if the dynamic attribute has already been set, if not set it */

              if (MJobGetDynamicAttr(J,mjaCancelExitCodes,(void **)tmpS,mdfString) == FAILURE)
                {
                if (MJobSetDynamicAttr(J,mjaCancelExitCodes,(void **)ptr,mdfString) == FAILURE)
                  return(FAILURE);
                }

              MUFree(&tmpS);
              }
            }
          }
        else
          return(FAILURE);

        break;

      case mjfNoResources:

        /* race condition?  will resources be set elsewhere? */

        if ((J != NULL) && (RQ != NULL))
          {
          /* clear job resource consumption */

          RQ->DRes.Procs = 0;

          J->Request.NC = 0;
          J->Request.TC = 0;

          RQ->TaskCount = 0;
          RQ->NodeCount = 0;

          RQ->DRes.Mem  = 0;
          RQ->DRes.Swap = 0;
          RQ->DRes.Disk = 0;

          RQ->TaskRequestList[0] = 0;
          RQ->TaskRequestList[1] = 0;

          J->TotalProcCount = 0;
          
          bmset(&J->SpecFlags,mjfNoRMStart);
          }

        break;

      default:

        /* NO-OP */

        break;
      }  /* END switch (findex) */

    ptr = MUStrTok(NULL,Delim,&TokPtr);
    }    /* END while (ptr != NULL) */

  return(SUCCESS);
  }  /* END MJobFlagsFromString() */





/**
 * Allocate job stat sub-structures.
 *
 * @param JSD (I) [modified,alloc]
 */

int MTJobStatAlloc(

  mtjobstat_t *JSD)  /* I (modified,alloc) */

  {
  if (JSD == NULL)
    {
    return(FAILURE);
    }

  /* NOTE:  structures below currently static */

  /*
  if (JSD->HostFail == NULL)
    JSD->HostFail = (int *)MUCalloc(1,sizeof(int) * MMAX_NODE * (MMAX_LEARNDEPTH + 1));

  if (JSD->HostPerf == NULL)
    JSD->HostPerf = (double *)MUCalloc(1,sizeof(double) * MMAX_NODE * (MMAX_LEARNDEPTH + 1));

  if (JSD->HostPower == NULL)
    JSD->HostPower = (double *)MUCalloc(1,sizeof(double) * MMAX_NODE * (MMAX_LEARNDEPTH + 1));

  if (JSD->HostSample == NULL)
    JSD->HostSample = (int *)MUCalloc(1,sizeof(int) * MMAX_NODE * (MMAX_LEARNDEPTH + 1));

  if (JSD->HostReject == NULL)
    JSD->HostReject = (mbool_t *)MUCalloc(1,sizeof(int) * MMAX_NODE);
  */

  return(SUCCESS);
  }  /* END MTJobStatAlloc() */




/**
 * Load job template config parameters.
 *
 * @see MTJobProcessConfig() - child
 * @see MSysLoadConfig() - parent - load initial config parameters
 * @see MSysReconfigure() - parent - dynamically modify existing parameters
 * @see MTJobLoad() - peer - process all job template attributes
 *
 * @param STJID (I) [optional]
 * @param Buf (I) [optional]
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 */

int MTJobLoadConfig(

  char *STJID,  /* I (optional) specified template job id */
  char *Buf,    /* I (optional) config buffer to process */
  char *EMsg)   /* O (optional,minsize=MMAX_LINE) */

  {
  char  tmpVal[MMAX_BUFFER];

  char  IndexName[MMAX_NAME];

  int   rc = SUCCESS;

  char *ptr;
  char *head;

  const char *FName = "MTJobLoadConfig";

  MDB(5,fSTRUCT) MLog("%s(%s,%.32s...,EMsg)\n",
    FName,
    (STJID != NULL) ? STJID : "NULL",
    (Buf != NULL) ? Buf : "NULL");

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if ((STJID == NULL) || (STJID[0] == '\0'))
    {
    head = (Buf != NULL) ? Buf : MSched.ConfigBuffer;
    ptr  = head;

    IndexName[0] = '\0';

    while (MCfgGetSVal(
             head,
             &ptr,  /* I/O */
             MCredCfgParm[mxoJob],
             IndexName,
             NULL,
             tmpVal,
             sizeof(tmpVal),
             0,
             NULL) != FAILURE)
      {
      rc &= MTJobProcessConfig(IndexName,tmpVal,NULL,EMsg);

      IndexName[0] = '\0';
      }
    }

  return(rc);
  }  /* END MTJobLoadConfig() */





/**
 * Process job template config parameters.
 *
 * @see MTJobAdd() - child
 * @see MTJobLoad() - child (parses job attributes)
 * @see MTJobLoadConfig() - parent
 *
 * All JOBCFG[] parameters processed in MTJobLoad()
 *
 * @param TID (I) template job id
 * @param Val (I)
 * @param TJP (O) [optional]
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 */

int MTJobProcessConfig(

  char    *TID,  /* I template job id */
  char    *Val,  /* I */
  mjob_t **TJP,  /* O (optional) */ 
  char    *EMsg) /* O (optional,minsize=MMAX_LINE) */

  {
  mjob_t *TJ;

  const char *FName = "MTJobProcessConfig";

  MDB(5,fSTRUCT) MLog("%s(%s,%s,TJP,EMsg)\n",
    FName,
    (TID != NULL) ? TID : "NULL",
    (Val != NULL) ? Val : "NULL");

  if (TJP != NULL)
    *TJP = NULL;

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if ((TID == NULL) || (Val == NULL))
    {
    return(FAILURE);
    }

  if ((TID[0] == '\0') || (Val[0] == '\0'))
    {
    return(FAILURE);
    }

  /* templates only added, never destroyed */

  if (MTJobAdd(TID,&TJ) == FAILURE)
    {
    return(FAILURE);
    }

  if (!strcasecmp(TJ->Name,"default"))
    {
    MSched.DefaultJ = TJ; 
    }

  if (bmisset(&TJ->IFlags,mjifTemplateIsDeleted))
    {
    MDB(3,fSTRUCT) MLog("INFO:     attempted operation on deleted template '%s'\n",
      TID);
   
    if (EMsg != NULL)
      MUStrCpy(EMsg,"could not perform operation on deleted template\n",MMAX_LINE);

    return(FAILURE);
    }

  /* FORMAT:  <ATTR>=<VAL> where <ATTR> is wiki compatible */

  mstring_t tmpBuf(MMAX_LINE);

  MWikiFromAVP(NULL,Val,&tmpBuf);

  if (MTJobLoad(
       TJ,
       tmpBuf.c_str(),
       NULL,
       EMsg) == FAILURE)
    {
    MDB(3,fSTRUCT) MLog("INFO:     cannot parse template job %s, ignoring\n",
      TID);

    return(FAILURE);
    }

  TJ->MTime = MSched.Time;

  if (TJP != NULL)
    *TJP = TJ;

  return(SUCCESS);
  }  /* END MTJobProcessConfig() */





/**
 * Load job match config parameter.
 *
 * @see MTJobMatchProcessConfig() - child
 *
 * @param STJMID (I) [optional]
 * @param Buf (I) [optional]
 */

int MTJobMatchLoadConfig(

  char *STJMID,  /* I (optional) */
  char *Buf)     /* I (optional) */

  {
  char  tmpVal[MMAX_BUFFER];

  char  IndexName[MMAX_NAME];

  char *ptr;

  if ((STJMID == NULL) || (STJMID[0] == '\0'))
    {
    ptr = (Buf != NULL) ? Buf : MSched.ConfigBuffer;

    IndexName[0] = '\0';

    while (MCfgGetSVal(
             MSched.ConfigBuffer,
             &ptr,
             MParam[mcoJobMatchCfg],
             IndexName,
             NULL,
             tmpVal,
             sizeof(tmpVal),
             0,
             NULL) != FAILURE)
      {
      MTJobMatchProcessConfig(IndexName,tmpVal,NULL);

      IndexName[0] = '\0';
      }
    }

  return(SUCCESS);
  }  /* END MTJobMatchLoadConfig() */





/**
 * Process job match (aka JOBMATCHCFG) parameter attributes.
 *
 * @see MTJobMatchLoadConfig() - parent
 *
 * @param TID (I) template job match id
 * @param Val (I)
 * @param JTMP (O) [optional]
 */

int MTJobMatchProcessConfig(

  char        *TID,  /* I template job match id */
  char        *Val,  /* I */
  mjtmatch_t **JTMP) /* O (optional) */

  {
  char *ptr;
  char *TokPtr;

  mjtmatch_t *JTM;

  int     tindex;

  int     aindex;

  enum MJTMatchAttrEnum AIndex;

  char    ValBuf[MMAX_LINE << 2];

  int     tmpI;  /* for attr comparison - not used */

  int     rc;

  const char *FName = "MTJobMatchProcessConfig";

  MDB(5,fSTRUCT) MLog("%s(%s,%s,TJP)\n",
    FName,
    (TID != NULL) ? TID : "NULL",
    (Val != NULL) ? Val : "NULL");

  if (JTMP != NULL)
    *JTMP = NULL;

  if ((TID == NULL) || (Val == NULL))
    {
    return(FAILURE);
    }

  if ((TID[0] == '\0') || (Val[0] == '\0'))
    {
    return(FAILURE);
    }

  /* templates only added, never destroyed */

  for (tindex = 0;tindex < MMAX_TJOB;tindex++)
    { 
    JTM = MJTMatch[tindex];

    if (JTM != NULL)
      {
      if (!strcasecmp(JTM->Name,TID))
        break;

      /* no match - continue search */

      continue;
      }

    /* end of list located - add structure to end of list */

    MJTMatch[tindex] = (mjtmatch_t *)MUCalloc(1,sizeof(mjtmatch_t));

    JTM = MJTMatch[tindex];

    if (JTM == NULL)
      {
      /* cannot alloc memory */

      return(FAILURE);
      }

    MUStrCpy(JTM->Name,TID,sizeof(JTM->Name));

    break;
    }

  if (tindex >= MMAX_TJOB)
    {
    /* table full */

    return(FAILURE);
    }

  /* FORMAT:  <ATTR>=<VAL> where <ATTR> is MTJobMatchAttr[] */

  ptr = MUStrTokE(Val," \t\n",&TokPtr);

  while (ptr != NULL)
    {
    /* parse name-value pairs */

    /* FORMAT:  <ATTR>=<VALUE>[,<VALUE>] */

    if (MUGetPair(
          ptr,
          (const char **)MJTMatchAttr,
          &aindex,   /* O */
          NULL,
          TRUE,
          &tmpI,     /* O */
          ValBuf,    /* O */
          sizeof(ValBuf)) == FAILURE)
      {
      /* cannot parse value pair */

      ptr = MUStrTokE(NULL," \t\n",&TokPtr);

      continue;
      }

    rc = SUCCESS;

    AIndex = (enum MJTMatchAttrEnum)aindex;

    switch (AIndex)
      {
      case mjtmaFlags:

        /* NYI */

        break;

      case mjtmaJMax:

        rc = MTJobFind(ValBuf,&JTM->JMax);

        break;

      case mjtmaJMin:

        rc = MTJobFind(ValBuf,&JTM->JMin);

        break;

      case mjtmaJDef:

        rc = MTJobFind(ValBuf,&JTM->JDef);

        break;

      case mjtmaJSet:

        rc = MTJobFind(ValBuf,&JTM->JSet);

        break;

      case mjtmaJStat:

        {
        /* FORMAT: <STAT>[,<STAT>]... */

        char *ptr;
        char *TokPtr;

        int   sindex;

        sindex = 0;

        ptr = MUStrTok(ValBuf,",",&TokPtr);

        while (ptr != NULL)
          {
          /* NOTE:  allow creation of stat object */

          /* NYI */

          rc = MTJobAdd(ValBuf,&JTM->JStat[sindex]);

          if (rc == SUCCESS)
            {
            /* job stats can be allocated when loaded from the ck file, do not overwrite */

            bmset(&JTM->JStat[sindex]->IFlags,mjifTemplateStat);

            if (JTM->JStat[sindex]->ExtensionData == NULL)
              {
              JTM->JStat[sindex]->ExtensionData = MUCalloc(1,sizeof(mtjobstat_t));

              /* mark template as data or stat template */

              MStatPInitialize(((void *)&((mtjobstat_t *)JTM->JStat[sindex]->ExtensionData)->S),TRUE,msoCred);
              }

            if (((mtjobstat_t *)JTM->JStat[sindex]->ExtensionData)->S.XLoad == NULL)
              {
              ((mtjobstat_t *)JTM->JStat[sindex]->ExtensionData)->S.XLoad = 
                (double *)MUCalloc(1,sizeof(double) * MSched.M[mxoxGMetric]);
              }

            MPUInitialize(&((mtjobstat_t *)JTM->JStat[sindex]->ExtensionData)->L.ActivePolicy);

            if (((mtjobstat_t *)JTM->JStat[sindex]->ExtensionData)->L.IdlePolicy == NULL)
              MPUCreate(&((mtjobstat_t *)JTM->JStat[sindex]->ExtensionData)->L.IdlePolicy);

            bmset(&JTM->JStat[sindex]->IFlags,mjifDataOnly);

            sindex++;

            if (sindex >= MMAX_JTSTAT)
              break;
            }  /* END if (rc == SUCCESS) */

          ptr = MUStrTok(NULL,",",&TokPtr);
          }  /* END while (ptr != NULL) */
        }    /* END BLOCK (case mjtmaJStat) */

        break;

      case mjtmaJUnset:

        rc = MTJobFind(ValBuf,&JTM->JUnset);

        break;

      default:

        /* NOT HANDLED */

        return(FAILURE);

        /*NOTREACHED*/

        break;
      }  /* END switch (AIndex) */

    if (rc == FAILURE)
      {
      MDB(2,fSTRUCT) MLog("WARNING:  cannot set attribute '%s' to value '%s' on jobmatch '%s'\n",
        MJTMatchAttr[AIndex],
        ValBuf,
        TID);
      }

    ptr = MUStrTokE(NULL," \t\n",&TokPtr);
    }    /* END while (ptr != NULL) */

  if (JTMP != NULL)
    *JTMP = JTM;

  return(SUCCESS);
  }  /* END MTJobProcessConfig() */





/**
 * Process Job Template attribute.
 *
 * @see MWikiJobUpdateAttr() - parent
 * @see MTJobLoad() - process non-wiki extensions
 * @see MJTXAttr[] - table of parameter names
 *
 * @param J (I) [modified]
 * @param AVP (I) [FORMAT:  <ATTR>=<VAL>]
 */

int MTJobProcessAttr(

  mjob_t     *J,
  const char *AVP)

  {
  char tmpLine[MMAX_LINE];

  char *Attr;
  char *Val;
  char *TokPtr = NULL;

  enum MJTXEnum aindex;

  if ((J == NULL) || (AVP == NULL))
    {
    return(FAILURE);
    }

  if (J->TemplateExtensions == NULL)
    {
    MJobAllocTX(J);
    }

  MUStrCpy(tmpLine,AVP,sizeof(tmpLine));

  Attr = MUStrTok(tmpLine,"=",&TokPtr);
  Val  = TokPtr;

  aindex = (enum MJTXEnum)MUGetIndexCI(Attr,MJTXAttr,FALSE,mjtxaNONE);

  switch (aindex)
    {
    case mjtxaTemplateDepend:

      {
      mln_t *tmpL = NULL;

      char *DependType;
      char *TokPtr = Val;

      while( (DependType = MUStrTok(NULL,",",&TokPtr)) != NULL)
        {
        char *DependValue;
        char *TokPtr2 = DependType;

        DependValue = MUStrTok(NULL,":",&TokPtr2);
        DependValue = MUStrTok(NULL,":",&TokPtr2);

        if (DependValue == NULL)
          break;

        MULLAdd(&J->TemplateExtensions->Depends,DependValue,NULL,&tmpL,NULL);

        /* NOTE:  template dependencies activated in MJobApplySetTemplate() */

        /* HACK:  before operations add 1024 to differentiate from after operations */

        if (!strcasecmp(DependType,"afterok"))
          bmset(&tmpL->BM,mdJobSuccessfulCompletion);
        else if (!strcasecmp(DependType,"afterany") || !strcasecmp(DependType,"after"))
          bmset(&tmpL->BM,mdJobCompletion);
        else if (!strcasecmp(DependType,"afterfail") || !strcasecmp(DependType,"afternotok"))
          bmset(&tmpL->BM,mdJobFailedCompletion);
        else if (!strcasecmp(DependType,"beforeok"))
          bmset(&tmpL->BM,mdJobBeforeSuccessful);
        else if (!strcasecmp(DependType,"beforeany") || !strcasecmp(DependType,"before"))
          bmset(&tmpL->BM,mdJobBefore);
        else if (!strcasecmp(DependType,"beforefail") || !strcasecmp(DependType,"beforenotok"))
          bmset(&tmpL->BM,mdJobBeforeFailed);
        else if (!strcasecmp(DependType,"syncwith"))
          bmset(&tmpL->BM,mdJobSyncMaster);
        }
      }  /* END BLOCK (case mjtxaTemplateDepend) */

      break;

    case mjtxaGRes:

      {
      char *ptr;
      char *TokPtr = NULL;

      char *ptr2;
      char *TokPtr2 = NULL;

      int   tmpS;

      /* generic node resource (required/consumed by job) */

      /* FORMAT:  <ATTR>[+<COUNT>][,<ATTR>[+<COUNT>]]... */

      ptr = MUStrTok(Val,",%",&TokPtr);

      J->Req[0]->DRes.GResSpecified = TRUE;

      while (ptr != NULL)
        {
        ptr2 = MUStrTok(ptr,":+",&TokPtr2);

        if ((ptr2 = MUStrTok(NULL,":+",&TokPtr2)) != NULL)
          tmpS = (unsigned int)strtol(ptr2,NULL,10);
        else 
          tmpS = 1;

        /* Add now in case EnforceGResAccess is on */

        MUMAGetIndex(meGRes,ptr,mAdd);

        if (MJobAddRequiredGRes(
              J,
              ptr,
              tmpS,
              mxaGRes, /* Force to GRes so that it will go onto DRes.GRes */
              FALSE,
              FALSE) == FAILURE)
          {
          snprintf(tmpLine,sizeof(tmpLine),"cannot locate required resource '%s'",
            ptr);

          MJobSetHold(J,mhBatch,0,mhrNoResources,tmpLine);

          break;
          }

        ptr = MUStrTok(NULL,",",&TokPtr);
        }  /* END while (ptr != NULL) */
      }    /* END BLOCK */

      break;

    case mjtxaReqFeatures:

      MUNodeFeaturesFromString(Val,mAdd,&J->Req[0]->ReqFBM);

      break;

    case mjtxaVariables:

      MJobSetAttr(J,mjaVariables,(void **)Val,mdfString,mSet);

      break;

    default:

      return(FAILURE);

      /*NOTREACHED*/

      break;
    }  /* END switch (aindex) */  

  bmset(&J->TemplateExtensions->SetAttrs,aindex);
 
  return(SUCCESS);
  }  /* END MTJobProcessAttr() */





/**
 * Update J to reflect contents of specified attribute list.
 *
 * NOTE:  Should handle job template exceptions and pass remaining attributes 
 *        to MWikiJobUpdateAttr()
 *
 * NOTE:  Attributes supported:  SOFTWARE, WORK, NODEMOD, PREF, PARTITION, 
 *                               IFLAGS, DESCRIPTION, SERVICE, MEM, TEMPLATEFLAGS,
 *                               NODEACCESSPOLICY, SELECT, USER, GROUP, QOS, 
 *                               CLASS, ACCOUNT, RM, SRM, TEMPLIMIT, VMUSAGE,
 *                               AUTOSIZE
 *
 * @see MWikiJobUpdateAttr() - child
 * @see MTJobProcessConfig() - parent
 *
 * @param J (I) [modified]
 * @param AList (I) [optional]
 * @param R (I) [optional]
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 */

int MTJobLoad(

  mjob_t *J,         /* I (modified) */
  char   *AList,     /* I (optional) */
  mrm_t  *R,         /* I (optional) */
  char   *EMsg)      /* O (optional,minsize=MMAX_LINE) */

  {
  mjob_t *JT;

  char   *TokPtr;
  char   *JobAttr;

  char   *ptr2;

  int     rc = SUCCESS;

  const char *FName = "MTJobLoad";

  MDB(5,fSTRUCT) MLog("%s(%s,%s,%s,EMsg)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (AList != NULL) ? AList : "NULL",
    (R != NULL) ? R->Name : "NULL");

  if (J == NULL)
    return(FAILURE);

  /* mark job as template */

  bmset(&J->IFlags,mjifIsTemplate);

  if ((MTJobFind(J->Name,&JT) == SUCCESS) && (J != JT))
    {
    /* apply job template info */

    if (JT->Credential.U != NULL)
      J->Credential.U = JT->Credential.U;

    if (JT->Credential.G != NULL)
      J->Credential.G = JT->Credential.G;

    if (JT->QOSRequested != NULL)
      {
      /* in all cases, grant access to template QOS */

      J->QOSRequested = JT->QOSRequested;
      J->Credential.Q = JT->QOSRequested;
      }

    if (JT->TemplateExtensions != NULL)
      {
      if (J->TemplateExtensions == NULL)
        {
        MJobAllocTX(J);
        }
      }  /* END if (JT->TX != NULL) */

    if (AList == NULL)
      MJobUpdateFlags(J);
    }  /* END if (MTJobFind(JobID,&JT) == SUCCESS) */

  if (AList == NULL)
    {
    return(SUCCESS);
    }

  /* FORMAT:  <FIELD>=<VALUE>;[<FIELD>=<VALUE>;]... */

  JobAttr = MUStrTokE(AList,";\n",&TokPtr);
  while (JobAttr != NULL)
    {
    enum MJobCfgAttrEnum aindex = mjcaNONE;
    char   ValLine[MMAX_LINE];

    /* parse name-value pairs */

    /* FORMAT:  <VALUE>[,<VALUE>] */

    if (MUGetPair(
        JobAttr,
        (const char **)MJobCfgAttr,
        (int*)&aindex,  /* O */
        NULL,
        TRUE,
        NULL,
        ValLine,
        sizeof(ValLine)) == FAILURE)
      {
      aindex = mjcaNONE;
      }

    switch(aindex)
      {
      /* STAGEIN=<value> */

      case mjcaStageIn:
        {
        if (MUBoolFromString(ValLine,FALSE) == TRUE)
          bmset(&J->IFlags,mjifStageInExec);
  
        break;
        }
  
      case mjcaSystemJobType:
        {
        if (J->System == NULL)
          {
          MJobCreateSystemData(J);
          }

        J->System->JobType = (enum MSystemJobTypeEnum)MUGetIndexCI(ValLine,MSysJobType,FALSE,msjtNONE);

        break;
        }
  
      /* FAILUREPOLICY=<value> */

      case mjcaFailurePolicy:
        {
        if (strlen(ValLine) > 0)
          MJobSetAttr(J,mjaTemplateFailurePolicy,(void **)&ValLine,mdfString,mSet);
  
        break;
        }
  
      case mjcaNodeSet:

        MJobSetNodeSet(J,ValLine,TRUE);

        break;
  
      /* PREF=<value> */

      case mjcaPref:
        {
        if (strlen(ValLine) > 0) 
          {
          mreq_t *RQ;
  
          RQ = J->Req[0];
          MReqSetAttr(J,RQ,mrqaPref,(void **)&ValLine,mdfString,mSet);
          }

        break;
        }
  
      /* PARTITION=<Value> */

      case mjcaPartition:
        {
        char *ptr;
        char *tTokPtr;
  
        char tmpLine[MMAX_LINE];
  
        MUStrCpy(tmpLine,ValLine,sizeof(tmpLine));
  
        ptr = MUStrTok(tmpLine,":,",&tTokPtr);
  
        /* need to add all partitions here, otherwise there is a race condition */
  
        while (ptr != NULL)
          {
          MParAdd(ptr,NULL);
  
          ptr = MUStrTok(NULL,",",&tTokPtr);
          }
  
        if (strlen(ValLine) > 0)
          MJobSetAttr(J,mjaPAL,(void **)ValLine,mdfString,mSet);
  
        break;
        }
  
      /* TFLAGS=<Value> */

      case mjcaTFlags:
        {
        char *ptr;
        char *tTokPtr;        
        enum MTJobFlagEnum findex;  
        char tmpLine[MMAX_LINE];
        mbool_t Selectable;
    
        MUStrCpy(tmpLine,ValLine,sizeof(tmpLine));
    
        ptr = MUStrTok(tmpLine,",",&tTokPtr);
  
        if (J->TemplateExtensions == NULL)
          {
          MJobAllocTX(J);
          }
  
        /* Keep/set SELECT, but do not take out here, use SELECT=FALSE */
  
        Selectable = bmisset(&J->TemplateExtensions->TemplateFlags,mtjfTemplateIsSelectable);
  
        bmclear(&J->TemplateExtensions->TemplateFlags);
  
        while (ptr != NULL)
          {
          findex = (enum MTJobFlagEnum)MUGetIndexCI(ptr,MTJobFlags,FALSE,mtjfNONE);
  
          if (findex != mtjfNONE)
            {
            bmset(&J->TemplateExtensions->TemplateFlags,findex);
            }
  
          ptr = MUStrTok(NULL,",",&tTokPtr);
          }
  
        /* Reset SELECT to TRUE if it was set before */
  
        if (Selectable == TRUE)
          bmset(&J->TemplateExtensions->TemplateFlags,mtjfTemplateIsSelectable);

        break;
        }
  
      case mjcaDescription:
        {
        char *tPtr;
        char *tTokPtr;
  
        char  tmpLine[MMAX_LINE];
  
        MUStrCpy(tmpLine,ValLine,sizeof(tmpLine));
  
        if ((tPtr = MUStrTok(tmpLine,"'\"",&tTokPtr)) != NULL)
          {
          if (strcmp(tPtr,"NONE"))
            {
            MJobSetAttr(J,mjaDescription,(void **)tPtr,mdfString,mSet);
            }
          else
            {
            /* Set the description to NONE -> unset */
  
            MUFree(&J->Description);
            }
          }
  
        break;
        }
  
  #ifdef INDEV
      /* SERVICE=<Value>*/

      case mjcaService:
        {
  
        J->TemplateExtensions->SuppliedGRes = X;
  
        break;
        }
  #endif
  
      /* MEM=<Value> */

      case mjcaMem:
        {
        MReqSetAttr(
          J,
          J->Req[0],
          mrqaReqMemPerTask,
          (void **)ValLine,
          mdfString,
          mSet);
  
        break;
        }
  
      /* CPULIMIT=<Value> */

      case mjcaCPULimit:
        {
  
        J->CPULimit = MUTimeFromString((char *)ValLine);
  
        break;
        }
  
      /* NODEACCESSPOLICY=<Value> */

      case mjcaNodeAccessPolicy:
        {
        if (strncmp(ValLine,"NONE",strlen("NONE")) != 0)
          {
          MReqSetAttr(
            J,
            J->Req[0],
            mrqaNodeAccess,
            (void **)ValLine,
            mdfString,
            mSet);
          }
        else
          {
          J->Req[0]->NAccessPolicy = mnacNONE;
          }
  
        break;
        }
  
      /* JOBSCRIPT=<Value> */

      case mjcaJobScript:
        {
        char *FileData;
  
        if ((FileData = MFULoadNoCache(ValLine,1,NULL,NULL,NULL,NULL)) != NULL)
          {
          if (J->RMSubmitString != NULL)
            MUFree(&J->RMSubmitString);
  
          J->RMSubmitString = FileData;
          }
        else
          {
          MDB(1,fWIKI) MLog("ERROR:    job template %s cannot load job script file %s\n",
            J->Name,
            JobAttr);
  
          MMBAdd(&J->MessageBuffer,"cannot load requested job script file",NULL,mmbtAnnotation,0,0,NULL);
          }
  
        break;
        }
  
      /* RESTARTABLE=<VALUE> */

      case mjcaRestartable:
        {

        MJobSetAttr(J,mjaIsRestartable,(void **)ValLine,mdfString,mSet);
  
        break;
        }
  
      /* RMSERVICEJOB=<VALUE> */

      case mjcaRMServiceJob: 
        {
  
        if (MUBoolFromString(ValLine,FALSE) == TRUE)
          {
          bmset(&J->IFlags,mjifIsServiceJob);
          }
  
        break;
        }
  
      /* SELECT=<VALUE> */

      case mjcaSelect:
        {
        if (J->TemplateExtensions == NULL)
          {
          MJobAllocTX(J);
          }
  
        if (MUBoolFromString(ValLine,FALSE) == TRUE)
          bmset(&J->TemplateExtensions->TemplateFlags,mtjfTemplateIsSelectable);
        else
          bmunset(&J->TemplateExtensions->TemplateFlags,mtjfTemplateIsSelectable);
  
        break;
        }
  
      /* USER=<VALUE> */

      case mjcaUser:
        {
        char *TokPtr2;
  
        mgcred_t *U;
  
        /* FORMAT:  <USER>[,<USER>]... */
  
        ptr2 = MUStrTok(ValLine,",",&TokPtr2);
  
        while (ptr2 != NULL)
          {
          if (MUserAdd(ptr2,&U) == FAILURE)
            {
            MDB(1,fWIKI) MLog("ERROR:    job template %s requests non-existent user %s\n",
              J->Name,
              ptr2);
  
            MMBAdd(&J->MessageBuffer,"cannot add requested user",NULL,mmbtAnnotation,0,0,NULL);
            }
          else
            {
            if (J->Credential.U == NULL)
              J->Credential.U = U;
  
            MACLSet(
              &J->RequiredCredList,
              maUser,
              (void *)U,
              mcmpSEQ,
              0,        /* I affinity - not used */
              0,        /* I flags - not used */
              TRUE);
            }
  
          ptr2 = MUStrTok(NULL,",",&TokPtr2);
          }  /* END while (ptr2 != NULL) */
        
        break;
        }  /* END case mjcaUser */
  
      /* GROUP=<VALUE> */

      case mjcaGroup:
        {
        char *TokPtr2;
  
        mgcred_t *G;
  
        /* FORMAT:  <GROUP>[,<GROUP>]... */
  
        ptr2 = MUStrTok(ValLine,",",&TokPtr2);
  
        while (ptr2 != NULL)
          {
          if (MGroupAdd(ptr2,&G) == FAILURE)
            {
            MDB(1,fWIKI) MLog("ERROR:    job template %s requests non-existent group %s\n",
              J->Name,
              ptr2);
  
            MMBAdd(&J->MessageBuffer,"cannot add requested group",NULL,mmbtAnnotation,0,0,NULL);
            }
          else
            {
            if (J->Credential.G == NULL)
              J->Credential.G = G;
  
            MACLSet(
              &J->RequiredCredList,
              maGroup,
              (void *)G,
              mcmpSEQ,
              0,        /* I affinity - not used */
              0,        /* I flags - not used */
              TRUE);
            }
  
          ptr2 = MUStrTok(NULL,",",&TokPtr2);
          }  /* END while (ptr2 != NULL) */
  
        break;
        }  /* END "GROUP=" */

      /* QOS=<VALUE */
  
      case mjcaQOS:
        {
        char *TokPtr2;
  
        mqos_t *Q;
  
        /* FORMAT:  <QOS>[,<QOS>]... */
  
        ptr2 = MUStrTok(ValLine,",",&TokPtr2);
  
        while (ptr2 != NULL)
          {
          if (!strcmp(ptr2,"NONE"))
            {
            /* unset QOS */
  
            J->QOSRequested = NULL;
            J->Credential.Q = NULL;
            }
          else if (MQOSAdd(ptr2,&Q) == FAILURE)
            {
            MDB(1,fWIKI) MLog("ERROR:    job template %s requests non-existent QoS %s\n",
              J->Name,
              ptr2);
  
            MMBAdd(&J->MessageBuffer,"cannot add requested QoS",NULL,mmbtAnnotation,0,0,NULL);
            }
          else
            {
            /* Always set */
  
            J->QOSRequested = Q;
            J->Credential.Q = Q;
  
            MACLSet(
              &J->RequiredCredList,
              maQOS,
              (void *)Q,
              mcmpSEQ,
              0,        /* I affinity - not used */
              0,        /* I flags - not used */
              TRUE);
            }
  
          ptr2 = MUStrTok(NULL,",",&TokPtr2);
          }  /* END while (ptr2 != NULL) */
  
        break;
        }  /* END "QOS=" */
  
      /* CLASS=<VALUE> */

      case mjcaClass:
        {
        char *TokPtr2;
  
        mclass_t *C;
  
        /* FORMAT:  <CLASS>[,<CLASS>]... */
  
        ptr2 = MUStrTok(ValLine,",",&TokPtr2);
  
        while (ptr2 != NULL)
          {
          if (!strcmp(ptr2,"NONE"))
            {
            /* unset class */
  
            J->Credential.C = NULL;
            }
          else if (MClassAdd(ptr2,&C) == FAILURE)
            {
            MDB(1,fWIKI) MLog("ERROR:    unable to create class %s for job template %s\n",
              ptr2,
              J->Name);
  
            MMBAdd(&J->MessageBuffer,"cannot add requested class",NULL,mmbtAnnotation,0,0,NULL);
            }
          else
            {
            /* Always set */
  
            J->Credential.C = C;
  
            /* push class into RCL to allow multiple classes per template (for matching) */
  
            MACLSet(
              &J->RequiredCredList,
              maClass,
              (void *)C,
              mcmpSEQ,
              0,        /* I affinity - not used */
              0,        /* I flags - not used */
              TRUE);
            }
  
          ptr2 = MUStrTok(NULL,",",&TokPtr2);
          }  /* END while (ptr2 != NULL) */
  
        break;
        }  /* END "CLASS=" */
  
      /* ACCOUNT=<VALUE> */

      case mjcaAccount:
        {
        char *TokPtr2;
  
        mgcred_t *A;
  
        /* FORMAT:  <ACCOUNT>[,<ACCOUNT>]... */
  
        ptr2 = MUStrTok(ValLine,",",&TokPtr2);
  
        while (ptr2 != NULL)
          {
          if (!strcmp(ptr2,"NONE"))
            {
            /* unset the account */
  
            J->Credential.A = NULL;
            }
          else if (MAcctAdd(ptr2,&A) == FAILURE)
            {
            MDB(1,fWIKI) MLog("ERROR:    job template %s requests non-existent account %s\n",
              J->Name,
              ptr2);
  
            MMBAdd(&J->MessageBuffer,"cannot add requested account",NULL,mmbtAnnotation,0,0,NULL);
            }
          else
            {
            /* Always set */
  
            J->Credential.A = A;
  
            MACLSet(
              &J->RequiredCredList,
              maAcct,
              (void *)A,
              mcmpSEQ,
              0,        /* I affinity - not used */
              0,        /* I flags - not used */
              TRUE);
            }
  
          ptr2 = MUStrTok(NULL,",",&TokPtr2);
          }  /* END while (ptr2 != NULL) */
  
        break;
        }  /* "ACCOUNT=" */
  
      /* ACTIONS=<VALUE> */

      case mjcaAction:

        J->TemplateExtensions->SetAction = (enum MJTSetActionEnum)MUGetIndexCI((char *)ValLine,MJTSetAction,FALSE,mjtsaNONE);

        break;

      /* GENSYSJOB=TRUE */
 
      case mjcaGenSysJob:

        {
        marray_t  TList;

        MUArrayListCreate(&TList,sizeof(mtrig_t *),1);
 
        if (MTrigLoadString(&J->System->TList,ValLine,TRUE,FALSE,mxoJob,J->Name,&TList,NULL) == SUCCESS)
          {
          bmset(&J->IFlags,mjifGenericSysJob);
          }
        else if (MUBoolFromString(ValLine,FALSE) == TRUE)
          {
          bmset(&J->IFlags,mjifGenericSysJob);
          }
        else
          {
          bmunset(&J->IFlags,mjifGenericSysJob);
          }

        MUArrayListFree(&TList);
        }  /* END case mjcaGenSysJob */

        break;

      /* INHERITRES=TRUE */

      case mjcaInheritResources:
        {
        if (MUBoolFromString(ValLine,FALSE) == TRUE)
          bmset(&J->IFlags,mjifInheritResources);
        else
          bmunset(&J->IFlags,mjifInheritResources);

        break;
        }
 
      /* RM=<VALUE> */

      case mjcaRM:
        {
        MJobSetAttr(J,mjaWorkloadRMID,(void **)ValLine,mdfString,mSet);
  
        break;
        }  /* "RM=" */
  
      /* SRM=<VALUE> */

      case mjcaSRM:
        {
        MRMFind(ValLine,&J->SubmitRM);
  
        break;
        }  /* END "SRM=" */
  
 
      case mjcaWalltime:

#if 0   /* What is this learning stuff? :) */
        /* WALLTIME=$LEARN */
        /* FORMAT:  <ATTR>=<VAL> | <ATTR>=$LEARN*<X> */
 
        if ((ptr2 = MUStrStr(ValLine,"$LEARN",0,TRUE,FALSE)) != NULL)
          {

          /* If JobAttr = "WALLTIME=$LEARN*1.2", then ptr2 = "$LEARN*1.2"*/
          /*ptr2 += strlen("WALLTIME="); */ 
  
          if (J->TX->LMultiplier == NULL)
            J->TX->LMultiplier = (double *)MUCalloc(1,sizeof(double) * mjaLAST);
  
          ptr2 = strchr(ptr2,'*');
  
          if (ptr2 != NULL)
            {
            ptr2++;
  
            J->TX->LMultiplier[mjaReqAWDuration] = strtod(ptr2,NULL); 
            }
          }
        else
          {
          MDB(1,fWIKI) MLog("ERROR:    LEARN value '%s' not supported for job '%s'\n",
            JobAttr,
            J->Name);
          }
          break;
#endif

        {
        mutime tmpL = MUTimeFromString(ValLine);

        if (tmpL <= 0)
          {
          MDB(1,fWIKI) MLog("ERROR:    invalid walltime specification '%s'\n",
            ValLine); 
          }
        else
          {
          J->SpecWCLimit[0] = tmpL;
          J->SpecWCLimit[1] = tmpL;
          }
        }  /* END case mjcaWalltime */

        break;

      /* VMUSAGE=<VALUE> */

      case mjcaVMUsage:
        {
  
        /* FORMAT:  <double> */
  
        MJobSetAttr(J,mjaVMUsagePolicy,(void **)ValLine,mdfString,mSet);
  
        break;
        }  /* END "VMUSAGE=" */

      /* DESTROYTEMPLATE=<NAME> */

      case mjcaDestroyTemplate:
        {
        MUStrDup(&J->TemplateExtensions->DestroyTemplate,ValLine);
        }

        break;

      /* MIGRATETEMPLATE=<NAME> */

      case mjcaMigrateTemplate:
        {
        MUStrDup(&J->TemplateExtensions->MigrateTemplate,ValLine);
        }

        break;

      /* MODIFYTEMPLATE=<NAME> */

      case mjcaModifyTemplate:
        {
        MUStrDup(&J->TemplateExtensions->ModifyTemplate,ValLine);
        }

        break;
 
      /* DGRES=<VALUE> */

      case mjcaDGres:
        {
        int     aindex;
        msnl_t *GRes;
        mreq_t *RQ;
  
        RQ = J->Req[0];
  
        aindex = MUGetIndexCI(JobAttr,MWikiJobAttr,MBNOTSET,0);
  
        if (aindex == mwjaCGRes)
          {
          GRes = &RQ->CGRes;
          }
        else if (aindex == mwjaAGRes)
          {
          GRes = &RQ->AGRes;
          }
        else
          {
          GRes = &RQ->DRes.GenericRes;
          }
  
        /* clear the old GRes attributes, this is a one way set */
  
        MSNLClear(GRes);

        if (strncmp(ValLine,"NONE",strlen("NONE")) != 0)
          {
          MJobUpdateGRes(J,RQ,GRes,aindex,ValLine);
          }
  
        break;
        } /* END "DGRES=" */
  
      /* If the attribute is not part of the MJobCfgAttr list, try it against
       * the Wiki list. Only process the attributes specified by the case statements */

      case mjcaArgs:
      case mjcaDMem: 
      case mjcaDDisk:
      case mjcaDSwap:
      case mjcaError:
      case mjcaExec:
      case mjcaExitCode:
      case mjcaFlags:
      case mjcaGAttr:
      case mjcaGEvent:
      case mjcaIWD:
      case mjcaJName:
      case mjcaName:
      case mjcaPartitionMask:
      case mjcaPriorityF:
      case mjcaRDisk:
      case mjcaRSwap:
      case mjcaRAGRes:
      case mjcaRCGRes:
      case mjcaTaskPerNode:
      case mjcaTrigger:
      case mjcaVariables:

      /* Wiki Attributes documented in Job Templates */

      case mjcaDProcs:
      case mjcaEUser:
      case mjcaGname:
      case mjcaGres:
      case mjcaNodes:
      case mjcaPriority:
      case mjcaRArch:
      case mjcaRFeatures:
      case mjcaROpsys:
      case mjcaTasks:
      case mjcaTemplateDepend:
      case mjcaUName:
      case mjcaWClimit:

        {
        if (MWikiJobUpdateAttr(JobAttr,J,J->Req[0],NULL,EMsg) == FAILURE)
          {
          char tmpLine[MMAX_LINE];
    
          rc &= FAILURE;
    
          MDB(1,fWIKI) MLog("ERROR:    cannot parse wiki string for job '%s'\n",
            J->Name);
    
          snprintf(tmpLine,sizeof(tmpLine),"cannot process attribute '%s'",
            JobAttr);
    
          MMBAdd(&J->MessageBuffer,tmpLine,NULL,mmbtAnnotation,0,0,NULL);
          }    /* END if (MWikiJobUpdateAttr() == FAILURE) */ 

        break;
        }

      default:
        {
        MDB(1,fWIKI) MLog("WARNING:  JobAttr not supported.  '%s'\n",
              JobAttr);

        break;
        }
      }

    JobAttr = MUStrTokE(NULL,";\n",&TokPtr);
    }  /* END while (JobAttr != NULL) */

  return(rc);
  }  /* END MTJobLoad() */



/**
 * Find matching job template or create new template if not found.
 *
 * @see MTJobFind() - peer
 *
 * NOTE:  case insensitive in Moab 5.4 and higher
 *
 * @param TJName (I)
 * @param TJP (O)
 */

int MTJobAdd(

  const char    *TJName,
  mjob_t       **TJP)

  {
  int     jindex;
  mjob_t *TJ;

  const char *FName = "MTJobAdd";

  MDB(7,fSCHED) MLog("%s(%s,TJP)\n",
    FName,
    (TJName != NULL) ? TJName : "NULL");

  if ((TJP == NULL) || (TJName == NULL))
    {
    return(FAILURE);
    }

  /* search job template table for match */
  
  TJ = NULL;
  
  for (jindex = 0;MUArrayListGet(&MTJob,jindex);jindex++)
    {
    TJ = *(mjob_t **)(MUArrayListGet(&MTJob,jindex));

    if (TJ == NULL)
      break;

    if (!strcasecmp(TJ->Name,TJName))
      {
      break;
      }
    else
      {
      TJ = NULL;
      }
    }

  if (TJ == NULL)
    {
    /* allocate new template */
    mjob_t *NewTJob = NULL;

    if ((MJobCreate(TJName,FALSE,&NewTJob,NULL) == FAILURE) ||
        (MReqCreate(NewTJob,NULL,&NewTJob->Req[0],FALSE) == FAILURE) ||
        (MUArrayListAppendPtr(&MTJob,(void *)NewTJob) == FAILURE))
      {
      MDB(7,fSCHED) MLog("INFO:     failed to add/find job template '%s'\n",
        TJName);

      if (NewTJob != NULL)
        MJobDestroy(&NewTJob);

      return(FAILURE);
      }

    TJ = NewTJob;

    bmset(&TJ->IFlags,mjifIsTemplate);

    MJobAllocTX(TJ);

    MCPRestore(mcpTJob,TJName,TJ,NULL);
    }  /* END TJ == NULL */
  
  *TJP = TJ;

  return(SUCCESS);  
  }  /* END int MTJobAdd() */



int MTJobFreeXD(

  mjob_t *J)

  {
  mtjobstat_t *Stat;

  if ((J == NULL) || (J->ExtensionData == NULL))
    {
    return(FAILURE);
    }

  Stat = (mtjobstat_t *)J->ExtensionData;

  MUFree((char **)&Stat->S.XLoad);

  MStatPDestroy((void ***)&Stat->S.IStat,msoCred,-1);

  MUFree((char **)&Stat->L.IdlePolicy);

  MUFree((char **)&J->ExtensionData);

  Stat = NULL;

  return(SUCCESS);
  }  /* END MTJobFreeXD() */


/**
 * Finds a template job based on the action happening to another template job
 *  Returns SUCCESS if the a template job is found, FAILURE otherwise.
 *
 * NewTJob is a template job, not a real job (you create the job from NewTJob)
 *
 * @see MTJobFind()
 *
 * @param TJob    [I] - The job that the action is happening to
 * @param Action  [I] - The action
 * @param NewTJob [O] - The resulting job
 */

int MTJobFindByAction(

  mjob_t *TJob,
  enum MTJobActionTemplateEnum Action,
  mjob_t **NewTJob)

  {
  mjob_t *tmpJ = NULL;

  if ((TJob == NULL) || (TJob->TemplateExtensions == NULL))
    {
    return(FAILURE);
    }

  if (NewTJob != NULL)
    *NewTJob = NULL;

  switch (Action)
    {
    case mtjatDestroy:

      MTJobFind(TJob->TemplateExtensions->DestroyTemplate,&tmpJ);

      break;

    case mtjatMigrate:

      MTJobFind(TJob->TemplateExtensions->MigrateTemplate,&tmpJ);

      break;

    case mtjatModify:

      MTJobFind(TJob->TemplateExtensions->ModifyTemplate,&tmpJ);

      break;

    default:

      return(FAILURE);

      /*NOTREACHED*/
      break;
    }  /* END switch (Action) */

  if (tmpJ == NULL)
    return(FAILURE);

  if (NewTJob != NULL)
    *NewTJob = tmpJ;

  return(SUCCESS);
  }  /* END MTJobFindByAction() */





/**
 * Creates a real job based on the action being performed on the job.
 *  Checks if the job has a template, and if that template has a job for
 *   that action.
 *  Returns SUCCESS if a job is created, FAILURE otherwise.
 *
 * @see MTJobFindByAction() - child
 *
 * @param CurrentJob[I] - The job having the action performed
 * @param Action    [I] - The action
 * @param NodeString[I] - Node to submit the job to (optional)
 * @param ActionJob [O] - Resulting job from the action (real job)
 * @param User      [I] - (optional) Name of the user who will own the job
 */

int MTJobCreateByAction(

  mjob_t      *CurrentJob,
  enum MTJobActionTemplateEnum Action,
  char        *NodeString,
  mjob_t     **ActionJob,
  const char  *User)

  {
  mjob_t *CurrentTemplate = NULL;    /* Template job that CurrentJob inherits */
  mjob_t *tmpJ = NULL;  /* Temp job to return */
  mjob_t *tmpTemplate = NULL; /* The template to create tmpJ off of */
  mln_t  *tmpL = NULL;
  mln_t  *vcL = NULL;
  mbool_t JobCreated = FALSE;
  char JName[MMAX_NAME];
  mrm_t *R;
  mbitmap_t SubmitFlags;

  if (CurrentJob == NULL)
    return(FAILURE);

  if (ActionJob != NULL)
    *ActionJob = NULL;

  mstring_t NodeStr(MMAX_LINE);
  mstring_t Buf(MMAX_LINE);

  if ((NodeString != NULL) && (NodeString[0] != '\0'))
    {
    MStringSetF(&NodeStr,"<Nodes><Node>%s</Node></Nodes>",
      NodeString);
    }
  else
    {
    NodeStr.clear();
    }

  /* Find template that Job uses */

  for (tmpL = CurrentJob->TSets;tmpL != NULL;tmpL = tmpL->Next)
    {
    JName[0] = '\0';

    MTJobFind(tmpL->Name,&CurrentTemplate);

    if ((CurrentTemplate == NULL) ||
        (MTJobFindByAction(CurrentTemplate,Action,&tmpTemplate) == FAILURE))
      {
      /* No associated job template for this action */

      continue;
      }

    if (!bmisset(&tmpTemplate->IFlags,mjifGenericSysJob))
      continue;

    /* Create the new job */

    if (CurrentJob->JGroup != NULL)
      {
      snprintf(JName,sizeof(JName),"%s.%s",
        CurrentJob->JGroup->Name,
        tmpTemplate->Name);
      }
    else
      {
      snprintf(JName,sizeof(JName),"%s.%s",
        CurrentJob->Name,
        tmpTemplate->Name);
      }

    tmpJ = NULL;

    MRMAddInternal(&R);

    Buf.clear();

    MStringSet(&Buf,"<job><Extension></Extension>");

    MStringAppendF(&Buf,"<JobName>%s</JobName>",
      JName);

    MStringAppendF(&Buf,"%s",
      NodeStr.c_str());

    MStringAppendF(&Buf,"<UserId>%s</UserId>",
      (!MUStrIsEmpty(User)) ? User : CurrentJob->Credential.U->Name);

    MStringAppendF(&Buf,"<GroupId>%s</GroupId>",
      ((mgcred_t *)CurrentJob->Credential.U->F.GDef)->Name);

    MStringAppendF(&Buf,"<ProjectId>%s</ProjectId>",
      (CurrentJob->Credential.A != NULL) ? CurrentJob->Credential.A->Name : "");

    MStringAppendF(&Buf,"<QualityOfService>%s</QualityOfService><SubmitLanguage>PBS</SubmitLanguage>",
      (CurrentJob->Credential.Q != NULL) ? CurrentJob->Credential.Q->Name : "");

    MStringAppendF(&Buf,"<Requested><Processors>%d</Processors><WallclockDuration>%ld</WallclockDuration></Requested></job>",
      1,
      1);

    /* Set the NoEventLogs flag so that the JOBSUBMIT event will not be written
        out.  When MS3 has created the job, it still will not be the full job
        because we need to apply templates and such later.  This means the
        event log will be incomplete or incorrect.  We will clear the flag
        and write out the JOBSUBMIT event manually before exiting */

    bmset(&SubmitFlags,mjsufNoEventLogs);

    MS3JobSubmit(Buf.c_str(),R,&tmpJ,&SubmitFlags,JName,NULL,NULL);

    /* Check if job was created */

    if (tmpJ == NULL)
      continue;

    /* Clear the NoEventLogs flag so that future events can be written out */

    bmunset(&tmpJ->IFlags,mjifNoEventLogs);

    /* Clear info that may have been on CurrentJob */

    bmclear(&tmpJ->Flags);
    bmclear(&tmpJ->SpecFlags);

    tmpJ->VMUsagePolicy = mvmupNONE;

    /* Copy variables */

    MUHTCopy(&CurrentJob->Variables,&tmpJ->Variables);

    /* Apply Template */

    MJobApplySetTemplate(tmpJ,tmpTemplate,NULL);

    bmset(&tmpJ->Flags,mjfSystemJob);
    bmset(&tmpJ->SpecFlags,mjfSystemJob);
    bmset(&tmpJ->Flags,mjfNoRMStart);
    bmset(&tmpJ->SpecFlags,mjfNoRMStart);
    bmset(&tmpJ->IFlags,mjifReserveAlways);

    if (Action == mtjatDestroy)
      {
      mjob_t *TJ = NULL;
      mvm_t *VM = NULL;

      /* Set the Job to have no resources.  */

      bmset(&tmpJ->Flags,mjfNoResources);
      bmset(&tmpJ->SpecFlags,mjfNoResources);
      
      bmunset(&tmpJ->IFlags,mjifReserveAlways);

      /* Clear the Procs to zero, because the reservation code doesn't look at the no resources bit. */
      
      tmpJ->Request.TC = 0;
      tmpJ->Req[0]->TaskCount = 0;
      tmpJ->Req[0]->DRes.Procs = 0;
      tmpJ->Req[0]->TaskRequestList[0] = 0;
      tmpJ->Req[0]->TaskRequestList[1] = 0;

      if ((!bmisset(&MSched.Flags,mschedfNoVMDestroyDependencies)) && MJobWorkflowHasStarted(CurrentJob))
        {
        mstring_t DependStr(MMAX_LINE);

        MStringSetF(&DependStr,"after:%s",
          CurrentJob->Name);

        MJobAddDependency(tmpJ,DependStr.c_str());
        }

      if ((CurrentJob->System != NULL) &&
          (CurrentJob->System->JobType == msjtVMTracking) &&
          (CurrentJob->System->VM != NULL) &&
          (MVMFind(CurrentJob->System->VM,&VM) == SUCCESS) &&
          (MUCheckAction(&VM->Action,msjtGeneric,&TJ) == TRUE) &&
          (MVMJobIsVMMigrate(TJ) == TRUE))
        {
        /* CurrentJob is VMTracking, currently being migrated. */
        /* Destroy after the migrate finishes */

        mstring_t DependStr(MMAX_LINE);

        MStringSetF(&DependStr,"afterany:%s",
          TJ->Name);

        MJobAddDependency(tmpJ,DependStr.c_str());
        }
      }  /* END if (Action == mtjatDestroy) */

    /* Place into TJob's parent VCs */
    for (vcL = CurrentJob->ParentVCs;vcL != NULL;vcL = vcL->Next)
      {
      mvc_t *VC = (mvc_t *)vcL->Ptr;

      if (VC == NULL)
        continue;

      MVCAddObject(VC,tmpJ,mxoJob);
      }  /* END for (vcL) */

    if (tmpJ->TemplateExtensions == NULL)
      {
      /* Have an error */

      MJobCancel(tmpJ,NULL,FALSE,NULL,NULL);

      continue;
      }

    /* Point this job back to CurrentJob */

    tmpJ->TemplateExtensions->TJobAction = Action;
    MUStrDup(&tmpJ->TemplateExtensions->JobReceivingAction,CurrentJob->Name);

    if (Action == mtjatDestroy)
      {
      bmset(&CurrentJob->Flags,mjfDestroyTemplateSubmitted);
      bmset(&CurrentJob->SpecFlags,mjfDestroyTemplateSubmitted);
      }

    if (Action == mtjatMigrate)
      {
      char *tmpVMID = NULL;

      /* Do some setup so that MOWriteEvent will register this job as a
          migration job */

      if (NodeString != NULL)
        MUStrDup(&tmpJ->System->VMDestinationNode,NodeString);

      /* Get VMID from CurrentJob (the VMTracking job) */

      if (MUHTGet(&CurrentJob->Variables,"VMID",(void **)&tmpVMID,NULL) == SUCCESS)
        {
        MUStrDup(&tmpJ->System->VM,tmpVMID);
        } 
      }  /* END if (Action ==mtjatMigrate) */

    MOWriteEvent((void *)tmpJ,mxoJob,mrelJobSubmit,"checkpoint record not found",MStat.eventfp,NULL);

    /* We will return the first created job */

    if ((JobCreated == FALSE) && (ActionJob != NULL))
      *ActionJob = tmpJ;

    JobCreated = TRUE;
    }  /* END for (tmpL = CurrentJob->TSets) */

  if (JobCreated == TRUE)
    return(SUCCESS);

  return(FAILURE);
  }  /* END MTJobCreateByAction() */



/**
 * Destroy (or mark deleted) a job template.
 *
 * This function is only for use by user commands.  User commands can only
 *  delete dynamic job templates (not ones defined in moab.cfg).
 *
 * @see __MUIJobTemplateCtl - parent
 *
 * @param TJob     (I) [modified]
 * @param EMsg     (O)
 * @param EMsgSize (I)
 */

int MTJobDestroy(

  mjob_t  *TJob,      /* I (modified) */
  char    *EMsg,      /* O */
  int      EMsgSize)  /* I */

  {
  if (TJob == NULL)
    return(FAILURE);

  if (bmisset(&TJob->IFlags,mjifTemplateIsDynamic))
    {
    bmset(&TJob->IFlags,mjifTemplateIsDeleted);
    }
  else
    {
    if (EMsg != NULL)
      {
      snprintf(EMsg,EMsgSize,"ERROR:    job template '%s' is not dynamic\n",TJob->Name);
      }

    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MTJobDestroy() */


