/* HEADER */

/**
 * @file MClass.c
 *
 * Moab Classes
 */

/* Contains:                                         *
 *  int MClassFind(CName,C)                          *
 *  int MClassShow(SC,SBuffer,SBufSize,DFlags,DFormat) */


#include "moab.h"
#include "moab-proto.h"
#include "moab-const.h"
#include "moab-global.h"

/**
 * Find class structure based on name.
 *
 * Returns: FAILURE or SUCCESS with a entry pointer
 *
 * @see MClassAdd()
 *
 * @param CName (I)
 * @param CP (O) [optional]
 */

int MClassFind(

  const char  *CName,
  mclass_t   **CP)

  {
  int cindex;

  const char *FName = "MClassFind";

  MDB(5,fSTRUCT) MLog("%s(%s,CP)\n",
    FName,
    (CName != NULL) ? CName : "NULL");

  if (CP != NULL)
    *CP = NULL;

  if ((CName == NULL) || 
      (CName[0] == '\0') || 
      (CP == NULL))
    {
    return(FAILURE);
    }

  /* Scan the list and compare each entry with the name given
   * This code assumes all entries are at the beginning and
   * the first 'empty' entry indicates End Of List 
   *  
   * The default class should be found with MSched.DefaultC but 
   * since the client doesn't have a default class, we will start 
   * our search here with MCLASS_DEFAULT_INDEX.
   */
  for (cindex = MCLASS_DEFAULT_INDEX;MClass[cindex].Name[0] != '\0';cindex++)
    {
    if (!strcmp(MClass[cindex].Name,CName))
      {
      /* We have a match: Note the addresss and return good  */
      *CP = &MClass[cindex];
 
      return(SUCCESS);
      }
    }    /* END for (cindex) */

  return(FAILURE);
  }  /* END MClassFind() */




/**
 * Add new class object to class list or return
 * existing class if one by the given name already
 * exists.
 * 
 * @see MClassFind()
 *
 * @param CName (I)
 * @param CP (O) [optional]
 */

int MClassAdd(

  const char  *CName,
  mclass_t   **CP)
 
  {
  int cindex;

  const char *FName = "MClassAdd";

  MDB(5,fSTRUCT) MLog("%s(%s,CP)\n",
    FName,
    (CName != NULL) ? CName : "NULL");

  if ((CName == NULL) ||
      (CName[0] == '\0') ||
      (strcmp(CName,NONE) == 0))   /* Fulton no longer uses the "[NONE]" class. */
    {
    return(FAILURE);
    }

  /* Starting at DEFAULT INDEX so the "DEFAULT" class can be added at 
   * index MCLASS_DEFAULT_INDEX. */

  for (cindex = MCLASS_DEFAULT_INDEX;cindex < MMAX_CLASS;cindex++)
    {
    if (MClass[cindex].Name[0] == '\0')
      {
      /* empty class slot located */
 
      memset(&MClass[cindex],0,sizeof(mclass_t));

      MClass[cindex].Index = cindex;

      MClass[cindex].MTime = MSched.Time;
 
      bmset(&MClass[cindex].Flags,mcfRemote);

      MUStrCpy(MClass[cindex].Name,CName,sizeof(MClass[cindex].Name));

      MPUInitialize(&MClass[cindex].L.ActivePolicy);

      MClass[cindex].Stat.XLoad = (double *)MUCalloc(1,sizeof(double) * MSched.M[mxoxGMetric]);

      MCredAdjustConfig(mxoClass,&MClass[cindex],FALSE);
 
      if (CP != NULL)
        *CP = &MClass[cindex];

      MDB(4,fSTRUCT) MLog("INFO:     new class '%s' added\n",
        CName);
 
      return(SUCCESS);
      }
 
    if (!strcmp(MClass[cindex].Name,CName))
      {
      /* existing class located */

      if (CP != NULL)
        *CP = &MClass[cindex];
 
      return(SUCCESS);
      }
    }    /* END for (cindex) */

  /* cannot add class, buffer is full */
 
  if (CP != NULL)
    *CP = NULL;

  if (MSched.OFOID[mxoClass] == NULL)
    MUStrDup(&MSched.OFOID[mxoClass],CName);

  return(FAILURE);
  }  /* END MClassAdd() */


/**
 * For all Nodes gather some node stats
 *
 * @param classNodes (O)
 */

void __MClassCalcNodeStats(
        
  int classNodes[][3])
      
  {
  int nodeIndex;
  int classIndex;

  /* Iterate over entire MNode array, looking for valid nodes,
   * then gather statistics from those valid nodes
   */
  for (nodeIndex = 0;nodeIndex < MSched.M[mxoNode]; nodeIndex++)
    {
    if (MNode[nodeIndex] == NULL)
      break;

    for (classIndex = MCLASS_FIRST_INDEX;classIndex < MMAX_CLASS; classIndex++)
      {
      /* Check for end of valid entries */
      if (MClass[classIndex].Name[0] == '\0')
        break;

      /* Nodes are initialized if slot count is > 0 */

      if ((MNode[nodeIndex] != NULL) &&
          (bmisset(&MNode[nodeIndex]->Classes,MClass[classIndex].Index)))
        {
        switch (MNode[nodeIndex]->State)
          {
          case mnsActive:
          case mnsBusy:
          case mnsDraining:

            classNodes[MClass[classIndex].Index][0]++;

            break;

          case mnsIdle:

            classNodes[MClass[classIndex].Index][1]++;

            break;
            
          case mnsDown:
          case mnsDrained:
          default:

            classNodes[MClass[classIndex].Index][2]++;

            break;
          } /* END switch (MNode[nodeIndex]->State) */
        } 
      }   /* END for(classIndex = MCLASS_FIRST_INDEX;classIndex < MMAX_CLASS; classIndex++) */
    }     /* for (nodeIndex = 0;nodeIndex < MSched.M[mxoNode]; nodeIndex++) */
  }


/**
 * Report class diagnostics, state, and configuration.
 * 
 * handle "mdiag -c"
 *
 * @see MUIDiagnose() - parent
 * @see __MUICredCtlQuery() - parent
 * @see MUIPeerSchedQuery() - parent
 *
 * @param Auth (I) (FIXME: not used but should be)
 * @param SpecClass (I) [optional]
 * @param RE (O)
 * @param Response (O)
 * @param DFlags (I) [bitmap of mcm, NOTE: mcmSummary is for GRID mode reporting]
 * @param IsPeer (I) Query is coming from peer, don't return stats.
 * @param DFormat (I) (XML or plain text)
 *
 * NOTE:  To add new class or cred attributes, insert associated enum into 
 *        ClAList[] and CAList[]
 */

int MClassShow(

  char                 *Auth,
  char                 *SpecClass,
  mxml_t               *RE,
  mstring_t            *Response,
  mbitmap_t            *DFlags,
  mbool_t               IsPeer,
  enum MFormatModeEnum  DFormat)
 
  {
  int       cindex;
  int       aindex;

  mstring_t FlagLine(MMAX_LINE);

  char      tmpQALString[MMAX_LINE];

  char      ParLine[MMAX_LINE];
 
  char      QALChar;

  mxml_t   *DE = NULL;
  mxml_t   *CE;
 
  mclass_t *C;
  mclass_t *SpecC;

  int       classNodes[MMAX_CLASS][3]; /* calcuate nodes stats for each class */
                                       /* 3 = active, idle, and down ndoe states */
  int nodeCount = 0;    /* begin: used for nodes per class calculations */
  int idleCount = 0;
  int activeCount = 0;
  int downCount = 0;

  enum MClassAttrEnum ClAList[] = {
    mclaAllocResFailPolicy,
    mclaCancelFailedJobs,
    mclaOCNode,
    mclaOCDProcFactor,
    mclaDefJAttr,
    mclaDefProc,
    mclaDefReqDisk,
    mclaDefReqFeature,
    mclaDefReqGRes,
    mclaDefReqMem,
    mclaDefNodeSet,
    mclaExclFeatures,
    mclaExclFlags,
    mclaHostList,
    mclaIgnNodeSetMaxUsage,
    mclaJobEpilog,
    mclaJobProlog,
    mclaLogLevel,
    mclaMaxCPUTime,
    mclaMaxProcPerNode,
    mclaMaxArraySubJobs,
    mclaMaxPS,
    mclaMaxGPU,
    mclaMinNode,
    mclaMinProc,
    mclaMinPS,
    mclaMinTPN,
    mclaMinGPU,
    mclaForceNAPolicy,
    mclaNAPolicy,
    mclaNAPrioF,
    mclaReqFeatures,
    mclaReqFlags,
    mclaReqImage,
    mclaReqUserList,
    mclaReqAccountList,
    mclaRM,
    mclaRMAList,
    mclaWCOverrun,
    mclaNONE };

  enum MCredAttrEnum CAList[] = {
    mcaADef,
    mcaFlags,
    mcaHold,
    mcaJobFlags,
    mcaManagers,
    mcaMaxIJob,
    mcaMaxIProc,
    mcaDefTPN,
    mcaDefWCLimit,
    mcaMinWCLimitPerJob,
    mcaMaxWCLimitPerJob,
    mcaMaxNodePerJob,
    mcaMaxProcPerJob,
    mcaMaxProcPerNode,
    mcaVariables,
    mcaMaxMem,
    mcaNONE };

  enum MCredAttrEnum PerList[] = {
    mcaMaxProcPerUser,
    mcaMaxNodePerUser,
    mcaMaxJobPerUser,
    mcaMaxJobPerGroup,
    mcaNONE };

  mbool_t   AIsSet;

  char convertBuffer[MMAX_LINE];


  const char *FName = "MClassShow";

  MDB(3,fUI) MLog("%s(%s,%s,SBuffer,SBufSize,%d)\n",
    Auth,
    FName,
    (SpecClass != NULL) ? SpecClass : "NULL",
    DFormat);

  if (DFormat == mfmXML)
    {
    /* XML Format section */
    DE = RE;

    if (DE == NULL)
      {
      return(FAILURE);
      }
    }
  else
    {
    if (Response == NULL)
      {
      return(FAILURE);
      }

    /* create human readable header for non-XML output */

    *Response += "Class/Queue Status\n\n";
 
    /*                        NAME PRI  FLAG  QDEF  QLST M PLST  FST  Limits */

    snprintf(convertBuffer,sizeof(convertBuffer),
      "%-14s %-8s %-12s %-12s %12s%c %-20s %-6s %s\n\n",
      "ClassID",
      "Priority",
      "Flags",
      "QDef",
      "QOSList",
      '*',
      "PartitionList",
      "Target",
      "Limits");

    *Response += convertBuffer;
    }

  /* calculate node stats for classes only on verbose */

  if (bmisset(DFlags,mcmVerbose))
    {
    memset(classNodes,0,sizeof(classNodes));  /* clear all previous results */

    __MClassCalcNodeStats(classNodes);        /* Go gather the node stats */
    }       /* END if (bmisset(&DFlags,mcmVerbose)) */

  if (SpecClass != NULL)
    {
    MClassFind(SpecClass,&SpecC);
    }

  /* Now scan/iterate the entire MClass array looking for valid entries to scan and show */
  /* Scan starting at DEFAULT to show DEFAULT class. */

  mstring_t tmpLString("");

  for (cindex = MCLASS_DEFAULT_INDEX;cindex < MMAX_CLASS;cindex++)
    {
    C = &MClass[cindex];
 
    MDB(8,fUI) MLog("INFO:     checking MClass[%02d]: %s\n",
      cindex,
      C->Name);
 
    /* validate the arguments */
    if (C->Name[0] == '\0')
      continue;

    /* If Specific Class (SpecC) was specified, this entry is not it, continue */
    if ((!MUStrIsEmpty(SpecClass)) && (C != SpecC))
      continue;
 
    FlagLine.clear();

    MJobFlagsToMString(NULL,&C->F.JobFlags,&FlagLine);
 
    if (C->F.QALType == malAND)
      QALChar = '&';
    else if (C->F.QALType == malOnly)
      QALChar = '^';
    else
      QALChar = ' ';
 

    if (DFormat == mfmXML)
      {
      enum MCredAttrEnum BCAList[] = {
        mcaJobFlags,
        mcaFSTarget,
        mcaID,
        mcaPriority,
        mcaMaxJob,
        mcaMaxProc,
        mcaMaxProcPerNode,
        mcaNONE };

      enum MClassAttrEnum MCLAList[] = {
        mclaHostList,
        mclaExclUserList,
        mclaReqUserList,
        mclaReqAccountList,
        mclaMaxProc,
        mclaComment,      /* probably should stay near the bottom */
        mclaNONE };

      /* create XML object */

      CE = NULL;

      MXMLCreateE(&CE,(char *)MXO[mxoClass]);

      MXMLAddE(DE,CE);
    
      for (aindex = 0;BCAList[aindex] != mcaNONE;aindex++)
        {
        tmpLString = "";

        if ((MCredAToMString(
               C,
               mxoClass,
               BCAList[aindex],
               &tmpLString,      /* O */
               mfmNONE,
               0) == SUCCESS) &&
            (!MUStrIsEmpty(tmpLString.c_str())) &&
             strcmp(tmpLString.c_str(),NONE))
          {
          MXMLSetAttr(CE,(char *)MCredAttr[BCAList[aindex]],(void *)tmpLString.c_str(),mdfString);
          }
        }    /* END for (cindex) */
    
      for (aindex = 0;MCLAList[aindex] != mclaNONE; aindex++)
        {
        tmpLString = "";

        if ((MClassAToMString(
              C,
              MCLAList[aindex],
              &tmpLString) == SUCCESS) && (!MUStrIsEmpty(tmpLString.c_str())))
          {
          MXMLSetAttr(CE,(char *)MClassAttr[MCLAList[aindex]],(void *)tmpLString.c_str(),mdfString);
          }
        }       /* END for (aindex) */
      }      /* END if (DFormat == mfmXML) */
    else
      {
      char Line[MMAX_LINE];

      MCredShowAttrs(
        &C->L,
        &C->L.ActivePolicy,
        C->L.IdlePolicy,
        C->L.OverrideActivePolicy,
        C->L.OverrideIdlePolicy,
        C->L.OverriceJobPolicy,
        C->L.RsvPolicy,
        C->L.RsvActivePolicy,
        &C->F,
        0,
        TRUE,
        TRUE,
        Line);

      tmpLString = Line;

      /*                         NAME PRIO FLAG  QDEF  QLST M PLST  FSTARG Limits */
 
      snprintf(convertBuffer,sizeof(convertBuffer),
        "%-14s %8ld %-12s %-12s %12s%c %-20s %5.2f %7s\n",
        C->Name,
        C->F.Priority,
        (!FlagLine.empty()) ? FlagLine.c_str() : "---",
        (C->F.QDef[0]) != NULL ?  (C->F.QDef[0])->Name : "---",
        (!bmisclear(&C->F.QAL)) ? MQOSBMToString(&C->F.QAL,tmpQALString) : "---",
        QALChar,
        (bmisclear(&C->F.PAL)) ?  "---" : MPALToString(&C->F.PAL,NULL,ParLine),
        C->F.FSTarget,
        (!tmpLString.empty()) ? tmpLString.c_str() : "---");

      *Response += convertBuffer;

      if (bmisset(DFlags,mcmVerbose) && (C->MB != NULL))
        {
        char tmpLine[MMAX_LINE];

        MMBPrintMessages(
          C->MB,
          mfmHuman,
          FALSE,
          -1,
          tmpLine,
          sizeof(tmpLine));

        if (tmpLine[0] != '\0')
          {
          /* add ' %s'  tmpLine */
          *Response += "  ";
          *Response += tmpLine;
          }
        }

      if (bmisset(DFlags,mcmVerbose))
        {
        /* display usage information */

        /* NYI */
        }
      }    /* END else (DFormat == mfmXML) */

    if ((bmisset(DFlags,mcmVerbose)) && 
        ((classNodes[C->Index][0] > 0) ||
         (classNodes[C->Index][1] > 0) ||
         (classNodes[C->Index][2] > 0)))
      {
      /* total      active                    idle                      down */

      nodeCount   = classNodes[C->Index][0] + classNodes[C->Index][1] + classNodes[C->Index][2];
      activeCount = classNodes[C->Index][0];
      idleCount   = classNodes[C->Index][1];
      downCount   = classNodes[C->Index][2];

      if (DFormat != mfmXML)
        {
        snprintf(convertBuffer,sizeof(convertBuffer),
            "  Nodes:  Active=%d (%.2f%%)  Idle=%d (%.2f%%)  Down=%d (%.2f%%)  Total=%d\n",
            activeCount,
            (nodeCount < 1) ? 0.0 : ((double)activeCount / (double)nodeCount) * 100.0,
            idleCount,
            (nodeCount < 1) ? 0.0 : ((double)idleCount / (double)nodeCount) * 100.0,
            downCount,
            (nodeCount < 1) ? 0.0 : ((double)downCount / (double)nodeCount) * 100.0,
            nodeCount);

        *Response += convertBuffer;
        }
      } /* END if (nodeList != NULL) */

    /* list extended class attributes */

    AIsSet = FALSE;

    for (aindex = 0;ClAList[aindex] != mclaNONE;aindex++)
      {
      if (!bmisset(DFlags,mcmVerbose) && 
        ((ClAList[aindex] == mclaHostList) || (ClAList[aindex] == mclaReqUserList) || (ClAList[aindex] == mclaReqAccountList)))
        {
        continue;
        }

      if (bmisset(DFlags,mcmSummary) &&
        ((ClAList[aindex] == mclaHostList) || (ClAList[aindex] == mclaReqUserList) || (ClAList[aindex] == mclaReqAccountList)))
        {
        /* don't report such specific info when reporting for peers in GRID mode */

        continue;
        }

      tmpLString = "";

      if ((MClassAToMString(
             C,
             ClAList[aindex],
             &tmpLString) == SUCCESS) && 
          (!MUStrIsEmpty(tmpLString.c_str())) && 
           strcmp(tmpLString.c_str(),NONE))
        {
        if (DFormat == mfmXML)
          {
          MXMLSetAttr(CE,(char *)MClassAttr[ClAList[aindex]],(void *)tmpLString.c_str(),mdfString);
          }
        else
          {
          /* Add '  %s=%s' */
          *Response += "  ";
          *Response += MClassAttr[ClAList[aindex]];
          *Response += '=';
          *Response += tmpLString;
          }

        AIsSet = TRUE;
        }
      }    /* END for (aindex) */

    /* list extended cred attributes */

    for (aindex = 0;CAList[aindex] != mcaNONE;aindex++)
      {
      tmpLString = "";

      if ((MCredAToMString(
             (void *)C,
             mxoClass,
             CAList[aindex],
             &tmpLString,     /* O */
             mfmNONE,
             0) == SUCCESS) &&
          (!tmpLString.empty()))
        {
        if (DFormat == mfmXML)
          {
          MXMLSetAttr(CE,(char *)MCredAttr[CAList[aindex]],(void *)tmpLString.c_str(),mdfString);
          }
        else
          {
          /* Add '  %s=%s' */
          *Response += "  ";
          *Response += MCredAttr[CAList[aindex]];
          *Response += '=';
          *Response += tmpLString;
          }
       
        AIsSet = TRUE;
        }
      }    /* END for (aindex) */

    for (aindex = 0;PerList[aindex] != mcaNONE;aindex++)
      {
      tmpLString = "";

      if ((MCredPerLimitsToMString(
             (void *)C,
             mxoClass,
             PerList[aindex],
             &tmpLString,    /* O */
             mfmNONE,
             0) == SUCCESS) &&
          (!MUStrIsEmpty(tmpLString.c_str())))
        {
        if (DFormat == mfmXML)
          {
          MXMLSetAttr(CE,(char *)MCredAttr[PerList[aindex]],(void *)tmpLString.c_str(),mdfString);
          }
        else
          {
          *Response += ' ';
          *Response += tmpLString;
          }
        }
      }

      /* disable stats on a peer query */

    if ((DFormat == mfmXML) && (IsPeer == FALSE))
      {
      must_t *Stats;
      mxml_t *SE;

      /* add stats information */

      if ((MOGetComponent(C,mxoClass,(void **)&Stats,mxoxStats) == SUCCESS) &&
          (MOToXML(Stats,mxoxStats,NULL,TRUE,&SE) == SUCCESS))
        {
        MXMLAddE(CE,SE);
        }
      }

    if (DFormat != mfmXML)
      {
      if (AIsSet == TRUE)
        {
        *Response += '\n';
        }

      if (C == MSched.RemapC)
        {
        *Response += "  NOTE:  class ";
        *Response +=   C->Name;
        *Response += " is remap class\n";
        }

      if (bmisset(&C->Flags,mcfRemote))
        {
        *Response += "  NOTE:  class ";
        *Response += C->Name;
        *Response += " is remote\n";
        }

      if (C->FType == mqftRouting)
        {
        *Response += "  NOTE:  class ";
        *Response += C->Name;
        *Response += " is routing queue\n";
        }

      if (C->F.ReqRsv != NULL)
        {
        *Response += "  NOTE:  class ";
        *Response += C->Name;
        *Response += " is tied to rsv '";
        *Response += C->F.ReqRsv;
        *Response += "'\n";
        }
      }    /* END if (DFormat != mfmXML) */
    }      /* END for (cindex) */

  return(SUCCESS);
  }  /* END MClassShow() */



/**
 * Populate a class bitmap from a class style string.
 *
 * FORMAT: [<CLASS>[:<COUNT>]]... or <CLASS>[:<COUNT>][,<CLASS>[:<COUNT>]]... 
 *         delimiter is "[]" by default but can also be "," or ";" if detected in the string
 *
 * NOTE: that <COUNT> is parsed but ignored for backwards compatibility
 *
 * @param ClassBM (O)
 * @param String (I)
 * @param SrcR (I) [optional]
 */

int MClassListFromString(

  mbitmap_t     *ClassBM, /* O */
  char          *String,  /* I */
  mrm_t         *SrcR)    /* I (optional) */

  {
  char *cptr;
  char *tail;
  mclass_t *C;

  char *TokPtr;

  const char *DPtr;

  const char *DefDelim = "[]";

  char  tmpBuf[2];

  char  Buffer[MMAX_LINE];

  if (ClassBM != NULL)
    {
    bmclear(ClassBM);
    }

  if ((String == NULL) || (ClassBM == NULL))
    {
    return(FAILURE);
    }

  if (strstr(String,NONE) != NULL)
    {
    return(FAILURE);
    }

  if (strchr(String,',') != NULL)
    {
    tmpBuf[0] = ',';
    tmpBuf[1] = '\0';

    DPtr = tmpBuf;
    }
  else if (strchr(String,';') != NULL)
    {
    tmpBuf[0] = ';';
    tmpBuf[1] = '\0';

    DPtr = tmpBuf;
    }
  else
    { 
    DPtr = DefDelim;
    }
 
  strcpy(Buffer,String);

  cptr = MUStrTok(Buffer,DPtr,&TokPtr);

  if (cptr == NULL)
    {
    /* empty string pased in */

    return(SUCCESS);
    }

  do
    {
    if (((tail = strchr(cptr,':')) != NULL) ||
        ((tail = strchr(cptr,' ')) != NULL))
      {
      *tail = '\0';
      }

    if ((MClassFind(cptr,&C)) == FAILURE)
      {
 
      if (MClassAdd(cptr,&C) == SUCCESS)
        { 
        if (SrcR != NULL)
          {
          if (C->RM == NULL)
            C->RM = SrcR;

          if (SrcR->Type == mrmtNative)
            bmunset(&C->Flags,mcfRemote);
          }

        bmset(ClassBM,C->Index);
        }
      }     /* END if ((CIndex = MUMAGetIndex()) == FAILURE) */
    else
      {
      bmset(ClassBM,C->Index);
      }
    }
  while ((cptr = MUStrTok(NULL,DPtr,&TokPtr)) != NULL);

  return(SUCCESS);
  }  /* END MClassListFromString() */




/**
 * Converts a ClassBM to a [] formated string.
 * 
 * @param ClassBM
 * @param String
 * @param RM
 */

int MClassListToMString(

  mbitmap_t     *ClassBM,
  mstring_t     *String,
  mrm_t         *RM)

  {
  int   cindex;

  if ((ClassBM == NULL) || (String == NULL))
    {
    return(FAILURE);
    }

  *String = "";

  /* The class bitmap should never contain the DEFAULT class
   * on the server, but the client doesn't have a DEFAULT class
   * and therefore starts at index 0. */ 
  for (cindex = MCLASS_DEFAULT_INDEX;cindex < MMAX_CLASS;cindex++)
    {
    if (MClass[cindex].Name[0] == '\0')
      {
      /* end of list */

      break;
      }

    if ((RM != NULL) && (!bmisclear(&MClass[cindex].RMAList)) && !bmisset(&MClass[cindex].RMAList,RM->Index))
      {
      continue;
      }

    if (!bmisset(ClassBM,cindex))
      continue;

    *String += '[';
    *String += MClass[cindex].Name;
    *String += ']';
    }  /* END for (cindex) */

  return(SUCCESS);
  } /* END MClassListToMString() */







/**
 * Process class-specific config attributes.
 *
 * @param C (I) [modified]
 * @param Value (I)
 * @param IsPar (I)
 */

int MClassProcessConfig(

  mclass_t *C,
  char     *Value,
  mbool_t   IsPar)

  {
  mbitmap_t ValidBM;

  char *ptr;
  char *TokPtr;

  int   aindex;

  int   tmpI;

  char  ValBuf[MMAX_BUFFER];
  char  tmpLine[MMAX_LINE];

  mbool_t AttrIsValid;

  enum MObjectSetModeEnum ValCmp;

  const char *FName = "MClassProcessConfig";

  MDB(5,fCONFIG) MLog("%s(%s,%s)\n",
    FName,
    (C != NULL) ? C->Name : "NULL",
    (Value != NULL) ? Value : "NULL");

  if ((C == NULL) ||
      (Value == NULL) ||
      (Value[0] == '\0'))
    {
    return(FAILURE);
    }

  if (IsPar == TRUE)
    {
    MParSetupRestrictedCfgBMs(NULL,NULL,NULL,NULL,NULL,NULL,&ValidBM);
    }

  AttrIsValid = MBNOTSET;

  /* process value line */

  ptr = MUStrTokE(Value," \t\n",&TokPtr);

  while (ptr != NULL)
    {
    /* parse name-value pairs */

    if (MUGetPair(
          ptr,
          (const char **)MClassAttr,
          &aindex,
          NULL,
          TRUE,
          &tmpI,
          ValBuf,  /* O */
          sizeof(ValBuf)) == FAILURE)

      {
      mmb_t *M;

      /* cannot parse value pair */

      snprintf(tmpLine,sizeof(tmpLine),"unknown attribute '%.128s' specified",
        ptr);

      MDB(3,fSTRUCT) MLog("WARNING:  unknown attribute '%s' specified for class %s\n",
        ptr,
        (C != NULL) ? C->Name : "NULL");

      if (MMBAdd(&C->MB,tmpLine,NULL,mmbtNONE,0,0,&M) == SUCCESS)
        {
        M->Priority = -1;  /* annotation, do not checkpoint */
        }

      ptr = MUStrTokE(NULL," \t\n",&TokPtr);

      if (AttrIsValid == MBNOTSET)
        AttrIsValid = FALSE;

      continue;
      }

    switch (tmpI)
      {
      case 1:

        ValCmp = mIncr;

        break;

      case -1:

        ValCmp = mDecr;

        break;

      default:

        ValCmp = mSet;

        break;
      }  /* END switch (ValCmp) */

    if ((IsPar == TRUE) && (!bmisset(&ValidBM,aindex)))
      {
      ptr = MUStrTokE(NULL," \t\n",&TokPtr);

      continue;
      }

    /* NOTE:  only allow certain attributes to be set via config */

    switch (aindex)
      {
      case mclaAllocResFailPolicy:
      case mclaOCNode:
      case mclaDefJAttr:
      case mclaDefJExt:
      case mclaDefReqDisk:
      case mclaDefReqFeature:
      case mclaDefReqGRes:
      case mclaDefReqMem:
      case mclaDefNode:
      case mclaDefNodeSet:
      case mclaDefOS:
      case mclaDefProc:
      case mclaDisableAM:
      case mclaExclFeatures:
      case mclaExclFlags:
      case mclaHostList:
      case mclaIgnNodeSetMaxUsage:
      case mclaJobProlog:
      case mclaJobEpilog:
      case mclaJobTermSig:
      case mclaJobTrigger:
      case mclaLogLevel:
      case mclaMaxProcPerNode:
      case mclaMaxArraySubJobs:
      case mclaWCOverrun:
      case mclaMaxCPUTime:
      case mclaMaxNode:
      case mclaMaxProc:
      case mclaMaxPS:
      case mclaMaxGPU:
      case mclaMinNode:
      case mclaMinProc:
      case mclaMinPS:
      case mclaMinTPN:
      case mclaMinGPU:
      case mclaPartition:
      case mclaPreTerminationSignal:
      case mclaReqFeatures:
      case mclaReqFlags:
      case mclaReqImage:
      case mclaReqQOSList:
      case mclaReqUserList:
      case mclaReqAccountList:
      case mclaExclUserList:
      case mclaRMAList:
      case mclaSysPrio:
      case mclaCancelFailedJobs:

        AttrIsValid = TRUE;

        MClassSetAttr(
          C,
          NULL,
          (enum MClassAttrEnum)aindex,
          (void **)ValBuf,
          mdfString,
          ValCmp);

        break;

      case mclaForceNAPolicy:
      case mclaNAPolicy:

        AttrIsValid = TRUE;

        if (!strcasecmp(ValBuf,"SERIALJOBSHARED"))
          {
          C->NAIsSharedOnSerialJob = TRUE;
          }
        else
          {
          MClassSetAttr(
            C,
            NULL,
            (enum MClassAttrEnum)aindex,
            (void **)ValBuf,
            mdfString,
            ValCmp);
          }

        break;

      case mclaNAPrioF:

        /* first free any existing priority function */
        MNPrioFree(&C->P);

        if (MProcessNAPrioF(&C->P,ValBuf) == SUCCESS)
          {
          AttrIsValid = TRUE;
          }

        break;

      default:

        /* NO-OP */

        break;
      }  /* END switch (AIndex) */

    ptr = MUStrTokE(NULL," \t\n",&TokPtr);
    }  /* END while (ptr != NULL) */

  bmclear(&ValidBM);

  if (AttrIsValid == FALSE)
    {
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MClassProcessConfig() */




/**
 *
 *
 * NOTE: does NOT clear contents before populating
 *
 * @param C
 * @param VFlag
 * @param PIndex
 * @param String (O)
 */

int MClassConfigLShow(

  mclass_t  *C,
  int        VFlag,
  int        PIndex,
  mstring_t *String)

  {
  enum MClassAttrEnum AList[] = {
    mclaOCNode,
    mclaDefReqDisk,
    mclaDefReqFeature,
    mclaDefReqMem,
    mclaDefNodeSet,
    mclaExclFeatures,
    mclaExclFlags,
    mclaReqFeatures,
    mclaReqFlags,
    mclaReqImage,
    mclaNONE };

  int aindex;
 
  if ((C == NULL) || (String == NULL))
    {
    return(FAILURE);
    }

  /* don't clear buffer */

  mstring_t tmpString("");

  for (aindex = 0;AList[aindex] != mclaNONE;aindex++)
    {
    if ((MClassAToMString(C,AList[aindex],&tmpString) == FAILURE) ||
        (MUStrIsEmpty(tmpString.c_str())))
      {
      continue;
      }

    /* ' %s=%s' */
    *String += ' ';
    *String += MClassAttr[AList[aindex]];
    *String += '=';
    *String += tmpString;
    }  /* END for (aindex) */

  return(SUCCESS);
  }  /* END MClassConfigLShow() */





/**
 * Set specified class attribute.
 *
 * @param C (I) [modified]
 * @param P (I) [optional]
 * @param AIndex (I)
 * @param Value (I) [can be modified - side affect]
 * @param Format (I)
 * @param Mode (I)
 */

int MClassSetAttr(

  mclass_t               *C,      /* I (modified) */
  mpar_t                 *P,      /* I (optional) */
  enum MClassAttrEnum     AIndex, /* I */
  void                  **Value,  /* I (can be modified - side affect) */
  enum MDataFormatEnum    Format, /* I */
  enum MObjectSetModeEnum Mode)   /* I */

  {
  char *ptr;
  char *TokPtr = NULL;

  mjob_t *J  = NULL;
  mreq_t *RQ = NULL;

  int pindex = 0;

  const char *FName = "MClassSetAttr";

  MDB(5,fCONFIG) MLog("%s(%s,%s,Value,%d,Mode)\n",
    FName,
    (C != NULL) ? C->Name : "NULL",
    MClassAttr[AIndex],
    Format);

  if (C == NULL)
    {
    return(FAILURE);
    }

  if (P != NULL)
    {
    pindex = P->Index;
    }

  switch (AIndex)
    {
    case mclaAllocResFailPolicy:

      {
      char *ptr;

      /* FORMAT:  <POLICY>[*] */

      ptr = strchr((char *)Value,'*');

      if (ptr != NULL)
        {
        *ptr = '\0';

        C->ARFOnMasterTaskOnly = TRUE;
        }

      C->ARFPolicy = (enum MAllocResFailurePolicyEnum)MUGetIndexCI(
        (char *)Value,
        MARFPolicy,
        FALSE,
        0);
      }

      break;

    case mclaOCNode:

      MUStrDup(&C->OCNodeName,(char *)Value);

      break;

    case mclaCancelFailedJobs:

      C->CancelFailedJobs = MUBoolFromString((char *)Value,FALSE);

      break;

    case mclaDefJAttr:

      if (C->L.JobDefaults == NULL)
        {
        if (MJobAllocBase(&C->L.JobDefaults,&RQ) == FAILURE)
          {
          MDB(5,fCONFIG) MLog("ALERT:    cannot alloc job structure in %s\n",
            FName);

          break;
          }
        }
      else
        {
        RQ = C->L.JobDefaults->Req[0];
        }

      J = C->L.JobDefaults;

      MJobSetAttr(J,mjaGAttr,Value,Format,Mode);

      break;

    case mclaDefJExt:

       if (C->L.JobDefaults == NULL)
        {
        if (MJobAllocBase(&C->L.JobDefaults,&RQ) == FAILURE)
          {
          MDB(5,fCONFIG) MLog("ALERT:    cannot alloc job structure in %s\n",
            FName);

          break;
          }
        }

      J = C->L.JobDefaults;

      MJobSetAttr(J,mjaRMXString,Value,Format,Mode);

      break;

    case mclaDefReqDisk:

      {
      /* FORMAT: <VAL>[<MOD>] */

      long tmpL;

      tmpL = MURSpecToL((char *)Value,mvmMega,mvmMega);

      if (C->L.JobDefaults == NULL)
        {
        if (MJobAllocBase(&C->L.JobDefaults,&RQ) == FAILURE)
          {
          MDB(5,fCONFIG) MLog("ALERT:    cannot alloc job structure in %s\n",
            FName);

          break;
          }
        }
      else
        {
        RQ = (C->L.JobDefaults)->Req[0];
        }

      J = C->L.JobDefaults;

      RQ->DRes.Disk = tmpL;
      } /* END BLOCK mclaDefReqDisk */

      break;

    case mclaDefReqFeature:

      {
      char tmpLine[MMAX_LINE];

      /* FORMAT:  <FEATURE>[{ \t:,}<FEATURE>]... */

      /* default feature requirements */

      MUStrCpy(tmpLine,(char *)Value,sizeof(tmpLine));

      ptr = MUStrTok(tmpLine," \t,:[]",&TokPtr);

      while (ptr != NULL)
        {
        MUGetNodeFeature(ptr,mAdd,&C->DefFBM);

        ptr = MUStrTok(NULL," \t,:[]",&TokPtr);
        }  /* END while (ptr != NULL) */
      }    /* END BLOCK */

      break;

    case mclaDefReqGRes:

      {
      int GResCount;

      char *ptr2;
      char *TokPtr2 = NULL;

      /* FORMAT:  <GRES>[{ \t:,}<GRES>]... */

      /* default generic resource requirements */

      if (C->L.JobDefaults == NULL)
        {
        if (MJobAllocBase(&C->L.JobDefaults,&RQ) == FAILURE)
          {
          MDB(5,fCONFIG) MLog("ALERT:    cannot alloc job structure in %s\n",
            FName);

          break;
          }
        }
      else
        {
        RQ = (C->L.JobDefaults)->Req[0];
        }

      J = C->L.JobDefaults;

      ptr = MUStrTok((char *)Value,",",&TokPtr);

      while (ptr != NULL)
        {
        ptr2 = MUStrTok(ptr,"+:",&TokPtr2);

        if ((TokPtr2 != NULL) && (TokPtr2[0] != '\0'))
          GResCount = (int)strtol(TokPtr2,NULL,10);
        else
          GResCount = 1;

        /* Add the GRes manually here - job cannot add gres by itself if
           MSched.EnforceGRESAccess is TRUE */

        MUMAGetIndex(meGRes,ptr2,mAdd);

        if (MJobAddRequiredGRes(J,ptr2,GResCount,mxaGRes,FALSE,FALSE) == FAILURE)
          {
          char tmpLine[MMAX_LINE];

          snprintf(tmpLine,sizeof(tmpLine),"cannot locate required resource '%s'",
            ptr);

          MJobSetHold(J,mhBatch,0,mhrNoResources,tmpLine);

          break;
          }

        ptr = MUStrTok(NULL,",",&TokPtr);
        }  /* END while (ptr != NULL) */
      }    /* END BLOCK */

      break;

    case mclaDefReqMem:

      {
      /* FORMAT: <VAL>[<MOD>] */

      long tmpL;

      tmpL = MURSpecToL((char *)Value,mvmMega,mvmMega);

      if (C->L.JobDefaults == NULL)
        {
        if (MJobAllocBase(&C->L.JobDefaults,&RQ) == FAILURE)
          {
          MDB(5,fCONFIG) MLog("ALERT:    cannot alloc job structure in %s\n",
            FName);

          break;
          }
        }
      else
        {
        RQ = C->L.JobDefaults->Req[0];
        }

      J = C->L.JobDefaults;

      RQ->DRes.Mem = tmpL;
      }  /* END BLOCK (case mclaDefReqMem) */

      break;

    case mclaDefNodeSet:

      {
      if (C->L.JobDefaults == NULL)
        {
        if (MJobAllocBase(&C->L.JobDefaults,&RQ) == FAILURE)
          {
          MDB(5,fCONFIG) MLog("ALERT:    cannot alloc job structure in %s\n",
            FName);

          break;
          }
        }
      else
        {
        RQ = C->L.JobDefaults->Req[0];
        }

      RQ->SetList = (char **)MUCalloc(1,sizeof(char *) * MMAX_ATTR);

      MNSFromString(
        (char *)Value,
        &RQ->SetSelection,
        &RQ->SetType,
        RQ->SetList);
      }    /* END BLOCK */

      break;

    case mclaDefNode:

      {
      int Count;

      if (C->L.JobDefaults == NULL)
        {
        if (MJobAllocBase(&C->L.JobDefaults,&RQ) == FAILURE)
          {
          MDB(5,fCONFIG) MLog("ALERT:    cannot alloc job structure in %s\n",
            FName);

          break;
          }
        }
      else
        {
        RQ = C->L.JobDefaults->Req[0];
        }

      Count = strtol((char*)Value, NULL, 10);

      C->L.JobDefaults->Request.TC = Count;
      C->L.JobDefaults->Request.NC = Count;
      }    /* END BLOCK */

      break;

    case mclaDefOS:

      if (C->L.JobDefaults == NULL)
        {
        MJobAllocBase(&C->L.JobDefaults,&RQ);
        }

      if ((C->L.JobDefaults == NULL) || (C->L.JobDefaults->Req[0] == NULL))
        {
        MDB(1,fCONFIG) MLog("ALERT:    cannot alloc job structure in %s\n",
          FName);
  
        return (FAILURE);
        }
      else 
        {
        RQ = C->L.JobDefaults->Req[0];
        }

      RQ->Opsys = MUMAGetIndex(meOpsys,(char *)Value,mAdd);

      break;  /* END case mclaDefOS */

    case mclaDefProc:

      {
      if (C->L.JobDefaults == NULL)
        {
        if (MJobAllocBase(&C->L.JobDefaults,&RQ) == FAILURE)
          {
          MDB(1,fCONFIG) MLog("ALERT:    cannot alloc job structure in %s\n",
            FName);

          return(FAILURE);
          }
        }
      else
        {
        RQ = C->L.JobDefaults->Req[0];
        }

      J = C->L.JobDefaults;

      J->Request.TC = (int)strtol((char *)Value,NULL,10);

      RQ->DRes.Procs = 1;
      RQ->TaskCount  = J->Request.TC;
      }  /* END BLOCK case mclaDefProc */

      break;

    case mclaDisableAM:

      C->DisableAM = MUBoolFromString((char *)Value,FALSE);

      break;

    case mclaHostList:

      /* FORMAT: <HOSTEXP> */

      if (strncmp(C->Name,"DEFAULT",strlen(C->Name)) == 0)
        {
        MDB(7,fSTRUCT) MLog("INFO:     hostlist not supported for class %s\n",C->Name);

        break;
        }

      MUStrDup(&C->HostExp,(char *)Value);

      if (MNode[0] == NULL)
        {
        /* Prevent ERROR message from nodes not being loaded yet. */

        MDB(4,fSTRUCT) MLog("INFO:     nodes haven't been read in yet for class hostlist, will get after cluster query.\n");
        }
      else
        {
        MClassUpdateHostListRE(C,(char *)Value);
        }

      break;

    case mclaIgnNodeSetMaxUsage:

      C->IgnNodeSetMaxUsage = MUBoolFromString((char *)Value,FALSE);

      break;

    case mclaJobEpilog:

      {
      char tmpLine[MMAX_LINE << 2];

      if (Format == mdfString)
        {
        if ((C->L.JobDefaults == NULL) &&
            (MJobAllocBase(&C->L.JobDefaults,&RQ) == FAILURE))
          {
          MDB(5,fCONFIG) MLog("ALERT:    cannot alloc job structure in %s\n",
            FName);

          break;
          }

        MUStrDup(&C->JobEpilog,(char *)Value);

        snprintf(tmpLine,sizeof(tmpLine),"atype=exec,etype=end,action='%s',flags=attacherror",
          (char *)Value);

        MTrigLoadString(
          &C->L.JobDefaults->EpilogTList,
          tmpLine,
          TRUE,
          FALSE,
          mxoJob,
          NULL,
          NULL,
          NULL);
        }
      }   /* END case mclaJobEpilog */

      break;

    case mclaJobProlog:

      {
      char tmpLine[MMAX_LINE << 2];

      if (Format == mdfString)
        {
        if ((C->L.JobDefaults == NULL) &&
            (MJobAllocBase(&C->L.JobDefaults,&RQ) == FAILURE))
          {
          MDB(5,fCONFIG) MLog("ALERT:    cannot alloc job structure in %s\n",
            FName);

          break;
          }

        snprintf(tmpLine,sizeof(tmpLine),"atype=exec,etype=create,action='%s',sets=finished.!failure,flags=attacherror,flags=multifire",
          (char *)Value);

        MUStrDup(&C->JobProlog,(char *)Value);

        MTrigLoadString(
          &C->L.JobDefaults->PrologTList,
          tmpLine,
          TRUE,
          FALSE,
          mxoJob,
          NULL,
          NULL,
          NULL);

        snprintf(tmpLine,sizeof(tmpLine),"atype=internal,etype=create,action=job:-:start,requires=finished,sets=!failure,unsets=finished.!finished,flags=multifire");

        MTrigLoadString(
          &C->L.JobDefaults->PrologTList,
          tmpLine,
          TRUE,
          FALSE,
          mxoJob,
          NULL,
          NULL,
          NULL);

        snprintf(tmpLine,sizeof(tmpLine),"atype=internal,etype=create,action=job:-:modify:hold,requires=failure,unsets=failure.!failure,flags=multifire");

        MTrigLoadString(
          &C->L.JobDefaults->PrologTList,
          tmpLine,
          TRUE,
          FALSE,
          mxoJob,
          NULL,
          NULL,
          NULL);
        }  /* END if (Format == mdfString) */
      }    /* END BLOCK (case mclaJobProlog) */

      break;

    case mclaJobTermSig:

      MUStrDup(&C->TermSig,(char *)Value);

      break;

    case mclaJobTrigger:

      if (C->L.JobDefaults == NULL)
        {
        if (MJobAllocBase(&C->L.JobDefaults,&RQ) == FAILURE)
          {
          MDB(5,fCONFIG) MLog("ALERT:    cannot alloc job structure in %s\n",
            FName);

          break;
          }
        }

      J = C->L.JobDefaults;

      MTrigLoadString(
        &J->Triggers,
        (char *)Value,
        TRUE,
        FALSE,
        mxoClass,
        C->Name,
        NULL,
        NULL);

      break;

    case mclaLogLevel:

      C->LogLevel = (int)strtol((char *)Value,NULL,10);

      break;

    case mclaMaxCPUTime:

      {
      if ((C->L.JobMaximums[pindex] == NULL) &&
          (MJobAllocBase(&C->L.JobMaximums[pindex],&RQ) == FAILURE))
        {
        MDB(5,fCONFIG) MLog("ALERT:    cannot alloc job structure in %s\n",
          FName);

        break;
        }

      J = C->L.JobMaximums[pindex];
      RQ = J->Req[0];

      if (Format == mdfString)
        J->CPULimit = (mulong)strtol((char *)Value,NULL,10);
      else if (Format == mdfLong)
        J->CPULimit = *(mulong *)Value;
      }  /* END BLOCK */

      break;

    case mclaMaxNode:

      {
      if ((C->L.JobMaximums[pindex] == NULL) &&
          (MJobAllocBase(&C->L.JobMaximums[pindex],&RQ) == FAILURE))
        {
        MDB(5,fCONFIG) MLog("ALERT:    cannot alloc job structure in %s\n",
          FName);

        break;
        }

      J = C->L.JobMaximums[pindex];
      RQ = J->Req[0];

      if (Format == mdfString)
        J->Request.NC = (int)strtol((char *)Value,NULL,10);
      else
        J->Request.NC = *(int *)Value;

      RQ->NodeCount = J->Request.NC;
      }  /* END BLOCK */

      break;

    case mclaMaxProc:

      {
      if ((C->L.JobMaximums[pindex] == NULL) &&
          (MJobAllocBase(&C->L.JobMaximums[pindex],&RQ) == FAILURE))
        {
        MDB(5,fCONFIG) MLog("ALERT:    cannot alloc job structure in %s\n",
          FName);

        break;
        }

      J = C->L.JobMaximums[pindex];
      RQ = J->Req[0];

      if (Format == mdfString)
        J->Request.TC = (int)strtol((char *)Value,NULL,10);
      else
        J->Request.TC = *(int *)Value;

      bmset(&J->IFlags,mjifTasksSpecified);

      RQ->DRes.Procs = 1;
      RQ->TaskCount  = J->Request.TC;
      }  /* END BLOCK */

      break;

    case mclaMaxProcPerNode:

      {
      int tmpI;

      if (Format == mdfString)
        tmpI = (int)strtol((char *)Value,NULL,10);
      else
        tmpI = *(int *)Value;

      C->MaxProcPerNode = tmpI;
      }  /* END BLOCK */

      break;

    case mclaMaxArraySubJobs:

      {
      int tmpI;

      if (Format == mdfString)
        tmpI = (int)strtol((char *)Value,NULL,10);
      else
        tmpI = *(int *)Value;

      C->MaxArraySubJobs = tmpI;
      }  /* END BLOCK */

      break;

    case mclaMaxPS:

      if ((C->L.JobMaximums[pindex] == NULL) &&
          (MJobAllocBase(&C->L.JobMaximums[pindex],&RQ) == FAILURE))
        {
        MDB(5,fCONFIG) MLog("ALERT:    cannot alloc job structure in %s\n",
          FName);

        break;
        }

      J = C->L.JobMaximums[pindex];

      if (Format == mdfString)
        J->PSDedicated = strtod((char *)Value,NULL);
      else
        J->PSDedicated = *(double *)Value;

      bmset(&J->IFlags,mjifIsTemplate);

      break;

    case mclaMaxGPU:

      {
      int count;
      int gindex;

      if ((C->L.JobMaximums[pindex] == NULL) &&
          (MJobAllocBase(&C->L.JobMaximums[pindex],&RQ) == FAILURE))
        {
        MDB(5,fCONFIG) MLog("ALERT:    cannot alloc job structure in %s\n",
          FName);

        break;
        }

      J = C->L.JobMaximums[pindex];
      RQ = J->Req[0];

      gindex = MUMAGetIndex(meGRes,MCONST_GPUS,mAdd);

      if (Format == mdfString)
        count = (int)strtol((char *)Value,NULL,10);
      else
        count = *(int *)Value;

      bmset(&J->IFlags,mjifPBSGPUSSpecified);
      bmset(&J->IFlags,mjifIsTemplate);

      MSNLSetCount(&RQ->DRes.GenericRes,gindex,count);
      }
 
      break;

    case mclaMinNode:

      {
      if ((C->L.JobMinimums[pindex] == NULL) &&
          (MJobAllocBase(&C->L.JobMinimums[pindex],&RQ) == FAILURE))
        {
        MDB(5,fCONFIG) MLog("ALERT:    cannot alloc job structure in %s\n",
          FName);

        break;
        }

      J = C->L.JobMinimums[pindex];
      RQ = J->Req[0];

      bmset(&J->IFlags,mjifIsTemplate);

      if (Format == mdfString)
        J->Request.NC = (int)strtol((char *)Value,NULL,10);
      else
        J->Request.NC = *(int *)Value;

      RQ->NodeCount = J->Request.NC;
      }  /* END BLOCK */
     
      break;

    case mclaMinProc:

      {
      if ((C->L.JobMinimums[pindex] == NULL) &&
          (MJobAllocBase(&C->L.JobMinimums[pindex],&RQ) == FAILURE))
        {
        MDB(5,fCONFIG) MLog("ALERT:    cannot alloc job structure in %s\n",
          FName);

        break;
        }

      J = C->L.JobMinimums[pindex];
      RQ = J->Req[0];

      if (Format == mdfString)
        J->Request.TC = (int)strtol((char *)Value,NULL,10);
      else
        J->Request.TC = *(int *)Value;

      bmset(&J->IFlags,mjifIsTemplate);
      bmset(&J->IFlags,mjifTasksSpecified);

      RQ->DRes.Procs = 1;
      RQ->TaskCount  = J->Request.TC;
      }  /* END BLOCK (case mclaMinProc) */

      break;

    case mclaMinPS:

      if ((C->L.JobMinimums[pindex] == NULL) &&
          (MJobAllocBase(&C->L.JobMinimums[pindex],&RQ) == FAILURE))
        {
        MDB(5,fCONFIG) MLog("ALERT:    cannot alloc job structure in %s\n",
          FName);

        break;
        }

      J = C->L.JobMinimums[pindex];

      if (Format == mdfString)
        J->PSDedicated = strtod((char *)Value,NULL);
      else
        J->PSDedicated = *(double *)Value;

      bmset(&J->IFlags,mjifIsTemplate);

      break;

    case mclaMinGPU:

      {
      int count;
      int gindex;

      if ((C->L.JobMinimums[pindex] == NULL) &&
          (MJobAllocBase(&C->L.JobMinimums[pindex],&RQ) == FAILURE))
        {
        MDB(5,fCONFIG) MLog("ALERT:    cannot alloc job structure in %s\n",
          FName);

        break;
        }

      J = C->L.JobMinimums[pindex];
      RQ = J->Req[0];

      gindex = MUMAGetIndex(meGRes,MCONST_GPUS,mAdd);

      if (Format == mdfString)
        count = (int)strtol((char *)Value,NULL,10);
      else
        count = *(int *)Value;

      bmset(&J->IFlags,mjifPBSGPUSSpecified);
      bmset(&J->IFlags,mjifIsTemplate);

      MSNLSetCount(&RQ->DRes.GenericRes,gindex,count);
      }
 
      break;

    case mclaReqFeatures:
    case mclaExclFeatures:

      {
      char tmpLine[MMAX_LINE];

      /* minimum class feature requirements */

      /* FORMAT:  <FEATURE>[{ \t:,}<FEATURE>]... */

      if (AIndex == mclaReqFeatures)
        {
        if ((C->L.JobMinimums[pindex] == NULL) &&
            (MJobAllocBase(&C->L.JobMinimums[pindex],NULL) == FAILURE))
          {
          MDB(5,fCONFIG) MLog("ALERT:    cannot alloc job structure in %s\n",
            FName);

          break;
          }

        J = C->L.JobMinimums[pindex];
        }
      else
        {
        if ((C->L.JobMaximums[pindex] == NULL) &&
            (MJobAllocBase(&C->L.JobMaximums[pindex],NULL) == FAILURE))
          {
          MDB(5,fCONFIG) MLog("ALERT:    cannot alloc job structure in %s\n",
            FName);

          break;
          }

        J = C->L.JobMaximums[pindex];
        }

      RQ = J->Req[0];

      bmset(&J->IFlags,mjifIsTemplate);

      if (strchr((char *)Value,'|') != NULL)
        {
        RQ->ReqFMode = mclOR;
        }
      else
        {
        RQ->ReqFMode = mclAND;
        }

      MUStrCpy(tmpLine,(char *)Value,sizeof(tmpLine));

      ptr = MUStrTok(tmpLine,":,| \t\n",&TokPtr);

      while (ptr != NULL)
        {
        MUGetNodeFeature(ptr,mAdd,&RQ->ReqFBM);

        ptr = MUStrTok(NULL,":,| \t\n",&TokPtr);
        }  /* END while (ptr != NULL) */
      }    /* END (case mclaReqFeatures) */

      break;

    case mclaExclFlags:
    case mclaReqFlags:

      {
      /* minimum class feature requirements */

      /* FORMAT:  <FLAG>[{ \t:,}<FLAG>]... */

      if (AIndex == mclaReqFlags)
        {
        if ((C->L.JobMinimums[pindex] == NULL) &&
            (MJobAllocBase(&C->L.JobMinimums[pindex],NULL) == FAILURE))
          {
          MDB(5,fCONFIG) MLog("ALERT:    cannot alloc job structure in %s\n",
            FName);

          break;
          }

        J = C->L.JobMinimums[pindex];
        }
      else
        {
        if ((C->L.JobMaximums[pindex] == NULL) &&
            (MJobAllocBase(&C->L.JobMaximums[pindex],NULL) == FAILURE))
          {
          MDB(5,fCONFIG) MLog("ALERT:    cannot alloc job structure in %s\n",
            FName);

          break;
          }

        J = C->L.JobMaximums[pindex];
        }

      RQ = J->Req[0];

      bmset(&J->IFlags,mjifIsTemplate);

      bmfromstring((char *)Value,MJobFlags,&J->Flags);
      }    /* END BLOCK (case mclaExclFlags) */

      break;

    case mclaReqImage:

      {
      mreq_t *RQ;

      /* minimum class feature requirements */

      /* FORMAT:  <IMAGENAME> */

      if ((C->L.JobSet == NULL) &&
          (MJobAllocBase(&C->L.JobSet,NULL) == FAILURE))
        {
        MDB(5,fCONFIG) MLog("ALERT:    cannot alloc job structure in %s\n",
          FName);

        break;
        }

      RQ = C->L.JobSet->Req[0];

      if (RQ == NULL)
        break;

      if ((RQ->OptReq == NULL) &&
         ((RQ->OptReq = (moptreq_t *)MUCalloc(1,sizeof(moptreq_t))) == NULL))
        break;

      MUStrDup(&RQ->OptReq->ReqImage,(char *)Value);
      }  /* END BLOCK */

      break;

    case mclaMinTPN:

      {
      mreq_t *RQ;

      /* FORMAT:  <INT> */

      if ((C->L.JobMinimums[pindex] == NULL) &&
          (MJobAllocBase(&C->L.JobMinimums[pindex],&RQ) == FAILURE))
        {
        MDB(5,fCONFIG) MLog("ALERT:    cannot alloc job structure in %s\n",
          FName);

        break;
        }

      J = C->L.JobMinimums[pindex];
      RQ = J->Req[0];
    
      bmset(&J->IFlags,mjifIsTemplate);
  
      RQ->TasksPerNode = (int)strtol((char *)Value,NULL,10);
      }  /* END BLOCK */

      break;

      break;

    case mclaForceNAPolicy:

      {
      mreq_t *RQ;

      if (C->L.JobSet == NULL)
        {
        if (MJobAllocBase(&C->L.JobSet,&RQ) == FAILURE)
          {
          MDB(5,fCONFIG) MLog("ALERT:    cannot alloc job structure in %s\n",
            FName);

          break;
          }
        }
      else
        {
        RQ = C->L.JobSet->Req[0];
        }

      RQ->NAccessPolicy = (enum MNodeAccessPolicyEnum)MUGetIndexCI(
        (char *)Value,
        MNAccessPolicy,  
        FALSE,
        mnacNONE);
      }  /* END BLOCK (case mclaFNAPolicy) */

      break;

    case mclaNAPolicy:

      /* only supports mdfString format */

      {
      mreq_t *RQ;

      if (C->L.JobDefaults == NULL)
        {
        if (MJobAllocBase(&C->L.JobDefaults,&RQ) == FAILURE)
          {
          MDB(5,fCONFIG) MLog("ALERT:    cannot alloc job structure in %s\n",
            FName);

          break;
          }
        }
      else
        {
        RQ = C->L.JobDefaults->Req[0];
        }

      RQ->NAccessPolicy = (enum MNodeAccessPolicyEnum)MUGetIndexCI(
        (char *)Value,
        MNAccessPolicy,  
        FALSE,
        mnacNONE);
      }  /* END BLOCK (case mclaNAPolicy) */

      break;

    case mclaPartition:

      /* NOTE: partition may not exist yet */

      MUStrDup(&C->Partition,(char *)Value);

      break;

    case mclaPreTerminationSignal:

      {
      char *ptr;
      char *TokPtr = NULL;

      /* FORMAT:  [<SIGNAL>][@<OFFSET>] */

      ptr = (char *)Value;

      if (strchr(ptr,'@') == NULL)
        {
        C->PreTerminationSignal = (int)strtol(ptr,NULL,10);
        C->PreTerminationSignalOffset = MDEF_PRETERMINATIONSIGNALOFFSET;
        }
      else if (ptr[0] == '@')
        {
        C->PreTerminationSignal = MDEF_PRETERMINATIONSIGNAL;
        C->PreTerminationSignalOffset = MUTimeFromString(ptr + 1);
        }
      else
        {
        ptr = MUStrTok(ptr,"@",&TokPtr);
     
        C->PreTerminationSignal = (int)strtol(ptr,NULL,10);
 
        ptr = MUStrTok(NULL,"@",&TokPtr);

        C->PreTerminationSignalOffset = MUTimeFromString(ptr + 1);
        }
      }  /* END BLOCK (case mclaPreTerminationSignal) */

      break;

    case mclaOCDProcFactor:

      {
      double tmpD;

      if (Format == mdfString)
        tmpD = (double)strtod((char *)Value,NULL);
      else
        tmpD = *(double *)Value;

      C->OCDProcFactor = tmpD;
      }  /* END BLOCK */

      break;

    case mclaReqAccountList:
    case mclaReqQOSList:

      {
      char    tmpLine[MMAX_LINE];

      enum MAttrEnum attrType;

      attrType = (AIndex == mclaReqQOSList) ? maQOS : maAcct;

      /* FORMAT:  <USER>[{@,:+ \t\n}<USER>]... */

      /* NOTE:  FORMAT may include <USER>[@<HOST>] */

      /* NOTE:  currently do not support host specification */

      if ((C->L.JobMinimums[pindex] == NULL) &&
          (MJobAllocBase(&C->L.JobMinimums[pindex],&RQ) == FAILURE))
        {
        MDB(5,fCONFIG) MLog("ALERT:    cannot alloc job structure in %s\n",
          FName);

        break;
        }

      J = C->L.JobMinimums[pindex];

      bmset(&J->IFlags,mjifIsTemplate);

      MUStrCpy(tmpLine,(char *)Value,sizeof(tmpLine));

      ptr = MUStrTok(tmpLine,"@,:+ \t\n",&TokPtr);

      if (Mode == mSet)
        {
        /* clear current setting */

        MACLRemove(&J->RequiredCredList,attrType,NULL);
        }    /* END if (Mode == mSet) */

      while (ptr != NULL)
        {
        /* locate first available slot */

        switch (Mode)
          {
          case mIncr:
          case mSet:

            MACLSet(&J->RequiredCredList,attrType,ptr,mcmpSEQ,mnmNeutralAffinity,0,TRUE);
            
            break;

          case mDecr:

            MACLRemove(&J->RequiredCredList,attrType,ptr);

            break;

          default:
         
            /* NO-OP */

            break;
          }

        ptr = MUStrTok(NULL,"@,:+ \t\n",&TokPtr);
        }  /* END while (ptr != NULL) */
      }    /* END BLOCK */

      break;

    case mclaReqUserList:
    case mclaExclUserList:

      {
      char    *tmpLine = NULL;

      mjob_t ** JP;

      /* FORMAT:  <USER>[{@,:+ \t\n}<USER>]... */

      /* NOTE:  FORMAT may include <USER>[@<HOST>] */

      /* NOTE:  currently do not support host specification */

      if (AIndex == mclaReqUserList)
        JP = &C->L.JobMinimums[pindex];
      else 
        JP = &C->L.JobMaximums[pindex];

      if ((*JP == NULL) &&
          (MJobAllocBase(JP,&RQ) == FAILURE))
        {
        MDB(5,fCONFIG) MLog("ALERT:    cannot alloc job structure in %s\n",
          FName);

        break;
        }

      J = *JP;

      bmset(&J->IFlags,mjifIsTemplate);

      MUStrDup(&tmpLine,(char *)Value);

      ptr = MUStrTok(tmpLine,"@,:+ \t\n",&TokPtr);

      if (Mode == mSet)
        {
        /* clear current setting */

        MACLRemove(&J->RequiredCredList,maUser,NULL);
        }    /* END if (Mode == mSet) */

      while (ptr != NULL)
        {
        /* locate first available slot */

        switch (Mode)
          {
          case mIncr:
          case mSet:

            MACLSet(&J->RequiredCredList,maUser,ptr,mcmpSEQ,mnmNeutralAffinity,0,TRUE);
            
            break;

          case mDecr:

            MACLRemove(&J->RequiredCredList,maUser,ptr);

            break;

          default:
         
            /* NO-OP */

            break;
          }

        ptr = MUStrTok(NULL,"@,:+ \t\n",&TokPtr);
        }  /* END while (ptr != NULL) */

      MUFree(&tmpLine);
      }    /* END BLOCK */

      break;

    case mclaRM:

      /* NYI */

      break;

    case mclaRMAList:

      {
      mrm_t *R;

      mbool_t IsExcl = FALSE;

      char *ptr;
      char *TokPtr = NULL;

      /* FORMAT:  {!}<RMID>[,{!}<RMID>]... */

      if (strchr((char *)Value,'!'))
        IsExcl = TRUE;

      bmclear(&C->RMAList);

      ptr = MUStrTok((char *)Value,", \t\n:!",&TokPtr);

      while (ptr != NULL)
        {
        /* NOTE:  move from MRMFind to MRMAdd to avoid config race condition */

        if (MRMAdd(ptr,&R) == SUCCESS)
          bmset(&C->RMAList,R->Index);         

        ptr = MUStrTok(NULL,", \t\n:!",&TokPtr);
        }  /* END while (ptr != NULL) */

      if (IsExcl == TRUE)
        {
        bmnot(&C->RMAList,MSched.M[mxoRM]);
        }
      }    /* END BLOCK */

      break;

    case mclaSysPrio:

      if (C->L.JobDefaults == NULL)
        {
        if (MJobAllocBase(&C->L.JobDefaults,&RQ) == FAILURE)
          {
          MDB(5,fCONFIG) MLog("ALERT:    cannot alloc job structure in %s\n",
            FName);

          break;
          }
        }

      J = C->L.JobDefaults;

      J->SystemPrio = (long)strtol((char *)Value,NULL,10);

      break;

    case mclaWCOverrun:

      {
      int tmpL;

      if (Format == mdfString)
        tmpL = MUTimeFromString((char *)Value);
      else
        tmpL = *(long *)Value;

      C->F.Overrun = tmpL;
      }  /* END BLOCK */
     
    default:

      /* NO-OP */

      return(FAILURE);
   
      /*NOTREACHED*/

      break;
    }  /* END switch (AIndex) */

  return(SUCCESS);
  }  /* END MClassSetAttr() */





/**
 * Report class attribute as string.
 *
 * @see MClassSetAttr() - peer
 *
 * @param C (I)
 * @param AIndex (I)
 * @param String (O)
 */

int MClassAToMString(

  mclass_t            *C,       /* I */
  enum MClassAttrEnum  AIndex,  /* I */
  mstring_t           *String)  /* O */ 

  {
  char convertBuffer[64];

  if (String != NULL)
    *String = "";

  if ((C == NULL) || (String == NULL))
    {
    return(FAILURE);
    }

  switch (AIndex)
    {
    case mclaAllocResFailPolicy:

      if (C->ARFPolicy != marfNONE)
        *String = (char *)MARFPolicy[C->ARFPolicy];

      break;

    case mclaMaxArraySubJobs:

      if (C->MaxArraySubJobs > 0)
        {
        snprintf(convertBuffer,sizeof(convertBuffer),"%d",C->MaxArraySubJobs);
        *String += convertBuffer;
        }

      break;

    case mclaComment:

      {
      if (C->MB != NULL && C->MB->Data != NULL)
        {
        *String += C->MB->Data;
        }
      }

      break;

    case mclaCancelFailedJobs:

      if (C->CancelFailedJobs == TRUE)
        *String += MBool[C->CancelFailedJobs];

      break;

    case mclaOCNode:

      if (C->OCNodeName != NULL)
        *String += C->OCNodeName;

      break;

    case mclaOCDProcFactor:

      if (C->OCDProcFactor > 0.0)
        {
        snprintf(convertBuffer,sizeof(convertBuffer),"%0.2f", C->OCDProcFactor);
        *String += convertBuffer;
        }

      break;

    case mclaDefJAttr:

      if (C->L.JobDefaults != NULL)
        {
        MJobAToMString(C->L.JobDefaults,mjaGAttr,String);
        }

      break;

    case mclaDefNodeSet:

      if (C->L.JobDefaults != NULL)
        {
        mjob_t *J;
        mreq_t *RQ;
        int sindex = 0;
 
        J = C->L.JobDefaults;
        RQ = J->Req[0];
 
        if ((RQ != NULL) &&
            (RQ->SetList != NULL) &&
            (RQ->SetSelection != 0))
          {
          /* '%s:%s:' */
          *String += MResSetSelectionType[RQ->SetSelection];
          *String += ':';
          *String += MResSetAttrType[RQ->SetType];
          *String += ':';
 
          for (sindex = 0;RQ->SetList[sindex] != NULL;sindex++)
            {
            if (sindex > 0)
              *String += ':';
 
            *String += RQ->SetList[sindex];
            }
 
          if (sindex == 0)
            *String += NONE;
          }  /* END if ((RQ != NULL) && ... */
        }    /* END if (C->L.JDef != NULL) */
 
      break;

    case mclaDefOS: 

      if (C->L.JobDefaults != NULL)
        {
        mjob_t *J;
        mreq_t *RQ;
  
        J = C->L.JobDefaults;
        RQ = J->Req[0];

        if ((RQ != NULL) &&
            (RQ->Opsys != 0))
          {
          *String += MAList[meOpsys][RQ->Opsys];
          } 
        } /* END if C->L.JDef != NULL */
  
      break;

    case mclaDefProc:

      if (C->L.JobDefaults != NULL)
        {
        mjob_t *J;

        J = C->L.JobDefaults;

        if (J->Request.TC > 0)
          {
          snprintf(convertBuffer,sizeof(convertBuffer),"%d", J->Request.TC);
          *String += convertBuffer;
          }
        }

      break;
 
    case mclaDefReqFeature:

      {
      char tmpLine[MMAX_LINE];

      *String += MUNodeFeaturesToString(',',&C->DefFBM,tmpLine);
      }

      break;

    case mclaDefReqGRes:

      if (C->L.JobDefaults != NULL)
        {
        mjob_t *J;
        mreq_t *RQ;

        J = C->L.JobDefaults;
        RQ = J->Req[0];

        if ((RQ != NULL) &&
            (!MSNLIsEmpty(&RQ->DRes.GenericRes)))
          {
          MSNLToMString(&RQ->DRes.GenericRes,NULL,",",String,meGRes);
          }
        }

      break;

    case mclaDisableAM:

      if (C->DisableAM == TRUE)
        *String += MBool[C->DisableAM];

      break;

    case mclaHostList:

      if (C->HostExp != NULL)
        {
        *String += C->HostExp;
        }

      break;

    case mclaIgnNodeSetMaxUsage:

      if (C->IgnNodeSetMaxUsage == TRUE)
        *String += (char *)MBool[C->IgnNodeSetMaxUsage];

      break;

    case mclaExclFeatures:
    case mclaReqFeatures:

      {
      char Features[MMAX_LINE];

      mjob_t *J;

      if (AIndex == mclaExclFeatures)
        J = C->L.JobMaximums[0];
      else
        J = C->L.JobMinimums[0];
    
      if (J != NULL)
        {
        if ((J->Req[0] != NULL) &&
            (!bmisclear(&J->Req[0]->ReqFBM)))
          {
          *String += MUNodeFeaturesToString((J->Req[0]->ReqFMode == mclOR) ? '|' : ',',&J->Req[0]->ReqFBM,Features);
          }
        }   /* END if (J != NULL) */
      }     /* END BLOCK (case mclaExclFeatures) */
 
      break;

    case mclaExclFlags:
    case mclaReqFlags:

      {
      mjob_t *J;

      if (AIndex == mclaExclFlags)
        J = C->L.JobMaximums[0];
      else
        J = C->L.JobMinimums[0];

      if (J != NULL)
        {
        MJobFlagsToMString(NULL,&J->Flags,String);
        }
      }  /* END BLOCK (case mclaExclFlags) */

      break;

    case mclaReqAccountList:
    case mclaExclUserList:
    case mclaReqUserList:

      {
      macl_t *tACL;

      mjob_t *J;

      enum MAttrEnum attrType;

      attrType = (AIndex == mclaReqAccountList) ? maAcct : maUser;

      if ((AIndex == mclaReqUserList) || (AIndex == mclaReqAccountList))
        J = C->L.JobMinimums[0];
      else
        J = C->L.JobMaximums[0];

      if ((J == NULL) || (J->RequiredCredList == NULL))
        break;

      for (tACL = J->RequiredCredList;tACL != NULL;tACL = tACL->Next)
        {
        if (tACL->Type != attrType)
          continue;

        *String += (String->empty()) ? "" : ",";
        *String += tACL->Name;
        }
      }    /* END BLOCK (case mclaReqUserList) */

      break;

    case mclaReqImage:

      {
      mjob_t *J;

      J = C->L.JobSet;

      if ((J != NULL) && 
          (J->Req[0] != NULL) && 
          (J->Req[0]->OptReq != NULL) && 
          (J->Req[0]->OptReq->ReqImage != NULL))
        {
        *String += J->Req[0]->OptReq->ReqImage;
        }
      }    /* END BLOCK */

      break;

    case mclaJobEpilog:

      if (C->JobEpilog != NULL)
        *String += C->JobEpilog;

      break;

    case mclaJobProlog:

      if (C->JobProlog != NULL)
        *String += C->JobProlog;

      break;

    case mclaLogLevel:

      if (C->LogLevel != 0)
        {
        snprintf(convertBuffer,sizeof(convertBuffer),"%d", C->LogLevel);
        *String += convertBuffer;
        } 

      break;

    case mclaMaxCPUTime:

      {
      mjob_t *J;

      J = C->L.JobMaximums[0];

      if (J == NULL)
        break;

      if (J->CPULimit <= 0.0)
        break;

      snprintf(convertBuffer,sizeof(convertBuffer),"%ld", J->CPULimit);
      *String += convertBuffer;
      }

      break;

    case mclaMaxPS:

      {
      mjob_t *J;

      J = C->L.JobMaximums[0];

      if (J == NULL)
        break;

      if (J->PSDedicated <= 0.0)
        break;

      snprintf(convertBuffer,sizeof(convertBuffer),"%f", J->PSDedicated);
      *String += convertBuffer;
      }  /* END BLOCK (case mclaMaxPS) */

      break;

    case mclaMaxProc:
      
      {
      mjob_t *J;

      J = C->L.JobMaximums[0];

      if ((J != NULL) && (J->Request.TC != 0))
        {
        snprintf(convertBuffer,sizeof(convertBuffer),"%d", J->Request.TC);
        *String += convertBuffer;
        }
      }

      break;

    case mclaMinNode:

      {
      mjob_t *J;
      
      J = C->L.JobMinimums[0];
      
      if ((J != NULL) && (J->Request.NC > 0))
        {
        snprintf(convertBuffer,sizeof(convertBuffer),"%d", J->Request.NC);
        *String += convertBuffer;
        }
      }  /* END BLOCK (case mclaMinNode) */

     break;

    case mclaMinPS:

      {
      mjob_t *J;

      J = C->L.JobMinimums[0];

      if (J == NULL)
        break;

      if (J->PSDedicated <= 0.0)
        break;

      snprintf(convertBuffer,sizeof(convertBuffer),"%f", J->PSDedicated);
      *String += convertBuffer;
      }  /* END BLOCK (case mclaMinPS) */

      break;

    case mclaMinTPN:

      {
      mjob_t *J;

      J = C->L.JobMinimums[0];

      if ((J == NULL) || (J->Req[0] == NULL))
        break;

      if (J->Req[0]->TasksPerNode <= 0)
        break;

      snprintf(convertBuffer,sizeof(convertBuffer),"%d", J->Req[0]->TasksPerNode);
      *String += convertBuffer;
      }  /* END BLOCK (case mclaMinTPN) */

      break;

    case mclaMinProc:

      {
      mjob_t *J;

      J = C->L.JobMinimums[0];

      if ((J != NULL) && (J->Request.TC > 0))
        {
        snprintf(convertBuffer,sizeof(convertBuffer),"%d", J->Request.TC);
        *String += convertBuffer;
        }
      }  /* END BLOCK (case mclaMinProc) */

      break;

      break;

    case mclaForceNAPolicy:

      {
      mjob_t *J;
      mreq_t *RQ;

      J = C->L.JobSet;

      if (J == NULL)
        break;

      RQ = J->Req[0];

      if (RQ->NAccessPolicy == mnacNONE)
        break;

      *String += MNAccessPolicy[RQ->NAccessPolicy];
      }  /* END BLOCK (case mclaNAPolicy) */

      break;

    case mclaNAPolicy:

      {
      mjob_t *J;
      mreq_t *RQ;

      J = C->L.JobDefaults;

      if (J == NULL)
        break;

      RQ = J->Req[0];

      if (RQ->NAccessPolicy == mnacNONE)
        break;

      *String += MNAccessPolicy[RQ->NAccessPolicy];
      }  /* END BLOCK (case mclaNAPolicy) */

      break;

    case mclaNAPrioF:

      if (MPrioFToMString(C->P,String) == FAILURE)
        {
        return(FAILURE);
        }

      break;

    case mclaRM:

      if (C->RM == NULL)
        break;

      *String += ((mrm_t *)C->RM)->Name;

      break;

    case mclaRMAList:

      if (!bmisclear(&C->RMAList))
        {
        int rmindex;

        for (rmindex = 0;rmindex < MMAX_RM;rmindex++)
          {
          if (!bmisset(&C->RMAList,rmindex))
            continue;

          *String += (String->empty()) ? "" : ",";
          *String += MRM[rmindex].Name;
          }  /* END for (rmindex) */
        }

      break;

    case mclaSysPrio:

      {
      mjob_t *J;

      J = C->L.JobDefaults;

      if ((J != NULL) && (J->SystemPrio > 0))
        {
        snprintf(convertBuffer,sizeof(convertBuffer),"%ld", J->SystemPrio);
        *String += convertBuffer;
        }
      } 

      break;
  
    case mclaWCOverrun:

      if (C->F.Overrun != 0)
        {
        char TString[MMAX_LINE];

        MULToTString(C->F.Overrun,TString);
        *String += TString;
        }

      break;

    default:

      /* NO-OP */

      return(FAILURE);

      /*NOTREACHED*/

      break;
    }  /* END switch (AIndex) */

  return(SUCCESS);
  }  /* END MClassAToString() */




/**
 *
 *
 * @param CS (I)
 * @param VFlag
 * @param String (O)
 */

int MClassConfigShow(

  mclass_t  *CS,
  int        VFlag,  
  mstring_t *String)

  {
  int cindex;
  int aindex;

  mclass_t *C;

  if (String == NULL)
    {
    return(FAILURE);
    }

  *String = "";  /* Init the return string */

  mstring_t tmpString("");
  mstring_t FullString("");

  for (cindex = MCLASS_DEFAULT_INDEX;cindex < MMAX_CLASS;cindex++)
    {
    C = &MClass[cindex];

    if ((CS != NULL) && (CS != C))
      continue;

    if (C->Name[0] == '\1')
      continue;

    if (C->Name[0] == '\0')
      break;

    FullString = MCredCfgParm[mxoClass];
    FullString += '[';
    FullString += C->Name;
    FullString += "] ";

    for (aindex = 0;MClassAttr[aindex] != NULL;aindex++)
      {
      tmpString = "";

      MClassAToMString(C,(enum MClassAttrEnum)aindex,&tmpString);

      if (!tmpString.empty())
        {
        FullString += ' ';
        FullString += MClassAttr[aindex];
        FullString += '=';
        FullString += tmpString;
        }
      }    /* END for (aindex) */

    *String += FullString;
    *String += '\n';
    }  /* END for (cindex) */

  return(SUCCESS);
  }  /* END MClassConfigShow() */





/**
 *
 *
 * @param C (I)
 * @param U (I)
 */

int MClassCheckAccess(

  mclass_t *C,   /* I */
  mgcred_t *U)   /* I */

  { 
  macl_t *tACL;

  mjob_t *J;

  if ((C == NULL) || (U == NULL))
    {
    return(FAILURE);
    }

  J = C->L.JobMinimums[0];

  if ((J == NULL) || (J->RequiredCredList == NULL))
    {
    /* no access constraints */

    return(SUCCESS);
    }

  for (tACL = J->RequiredCredList;tACL != NULL;tACL = tACL->Next)
    {
    if (tACL->Type != maUser)
      continue;

    if (!strcmp(U->Name,tACL->Name))
      {
      /* specified user name in required user list */

      return(SUCCESS);
      }
    }    /* END for (cindex) */

  /* NOTE:  check class exclude user list */

  /* NYI */

  return(FAILURE);
  }  /* END MClassCheckAccess() */




/**
 * Expands the class's HostExp to a HostList.
 *
 * @param C (I) [modified]
 * @param Value (I) [modified]
 */

int MClassUpdateHostListRE(

  mclass_t *C,      /* I (modified) */
  char     *Value)  /* I (modified) */

  {
  char tmpMsg[MMAX_LINE];

  int  nindex;

  marray_t NodeList;

  mnl_t *NL;

  mnode_t *N;

  const char *FName = "MClassUpdateHostListRE";

  MDB(4,fSCHED) MLog("%s(%s,%s)\n",
    FName,
    C->Name,
    (Value == NULL) ? "NULL" : Value);

  /* hostlist is regular expression (or range) */

  /* NOTE:  potential race condition: class hostlist loaded
            requires nodes be preloaded */

  tmpMsg[0] = '\0';

  MUArrayListCreate(&NodeList,sizeof(mnode_t *),1);

  if (MUREToList(
       Value,
       mxoNode,
       NULL,
       &NodeList,
       FALSE,
       tmpMsg,  /* O */
       sizeof(tmpMsg)) == FAILURE)
    {
    MDB(2,fCONFIG) MLog("ERROR:    invalid class host expression received (%s) : %s\n",
      (char *)Value,
      tmpMsg);

    return(FAILURE);
    }

  NL = &C->NodeList;

  for (nindex = 0;nindex < NodeList.NumItems;nindex++)
    {
    N = (mnode_t *)MUArrayListGetPtr(&NodeList,nindex);

    MNLSetNodeAtIndex(NL,nindex,N);

    MNLSetTCAtIndex(NL,nindex,1);

    bmset(&N->Classes,C->Index);
    }  /* END for (nindex) */

  MNLTerminateAtIndex(NL,nindex);

  MUArrayListFree(&NodeList);

  return(SUCCESS);
  }  /* END MClassUpdateHostListRE() */

/**
 * Gets the effective Resource Manager limit for a class.  
 * This can be modified by the presence of limits on the default partition.
 * 
 * @return SUCCESS if there is a limit, FAILURE otherwise.
 *
 * @param C (I) 
 * @param ParIndex (I)  Partition Index
 * @param type (I)      Attribute type of limit
 * @param BoundedLimit (0)
 */

int MClassGetEffectiveRMLimit(

  mclass_t            *C,
  int                  ParIndex,
  enum MCredAttrEnum   type,
  unsigned long       *BoundedLimit)
 
  {
  MASSERT((C != NULL),"Unexpected NULL value for mclass_t C:");
  MASSERT((BoundedLimit != NULL),"Unexpected NULL value for unsigned long BoundedLimit:");

  switch (type)
    {
    case mcaMaxWCLimitPerJob:
      if ((C->RMLimit.JobMaximums[ParIndex]) &&             /* allocated, so bounds are present */
          (C->RMLimit.JobMaximums[ParIndex]->WCLimit != 0)) /* zero indicates no limit */
        {   
        *BoundedLimit = C->RMLimit.JobMaximums[ParIndex]->WCLimit;
        return(SUCCESS);
        }   
      break;

    case mcaMinNodePerJob:
      if ((C->RMLimit.JobMinimums[ParIndex]) &&             /* allocated, so bounds are present */
          (C->RMLimit.JobMinimums[ParIndex]->Request.NC != 0)) /* zero indicates no limit */
        {   
        *BoundedLimit = C->RMLimit.JobMinimums[ParIndex]->Request.NC;
        return(SUCCESS);
        }   
      break;

    case mcaMaxNodePerJob:
      if ((C->RMLimit.JobMaximums[ParIndex]) &&             /* allocated, so bounds are present */
          (C->RMLimit.JobMaximums[ParIndex]->Request.NC != 0)) /* zero indicates no limit */
        {   
        *BoundedLimit = C->RMLimit.JobMaximums[ParIndex]->Request.NC;
        return(SUCCESS);
        }   
      break;

    default:
        return(FAILURE);
    }   

  if (ParIndex != 0)
    {   
    /* if we have made it this far, there is not a limit on our specific partition... */
    /* recursively call ourself with the default partition index. */
    return MClassGetEffectiveRMLimit(C, 0, type, BoundedLimit);
    }   

  return(FAILURE);

  }  /* END for MClassGetEffectiveRMLimit*/

/* END MClass.c */
