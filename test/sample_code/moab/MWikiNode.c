/* HEADER */

      
/**
 * @file MWikiNode.c
 *
 * Contains: MWikiNode
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  

#include "MWikiInternal.h"

 
/**
 * Process WIKI node string and update associated node.
 *
 * Assumes attribute string starts at first attribtue value pair, not at jobid 
 * (ex. ClusterQuery format: jobid:<field>=<value>;[<field>=<value>;])
 *
 * @param AString (I)
 * @param N (I) [modified]
 * @param R (I)
 * @param AList (O) [optional,minsize=mwnaLAST]
 */

int MWikiNodeLoad(
 
  char                *AString,  /* I */
  mnode_t             *N,        /* I (modified) */
  mrm_t               *R,        /* I */
  enum MWNodeAttrEnum *AList)    /* O (optional,minsize=mwnaLAST) */

  {
  char *ptr;
  char *tail;

  char  NodeAttr[MMAX_BUFFER];

  char  EMsg[MMAX_LINE];
  char  tmpLine[MMAX_LINE];

  time_t tmpTime;

  enum MWNodeAttrEnum AIndex;

  int   acount;

  const char *FName = "MWikiNodeLoad";

  MDB(2,fWIKI) MLog("%s(AString,%s,%s,AList)\n",
    FName,
    (N != NULL) ? N->Name : "NULL",
    (R != NULL) ? R->Name : "NULL");

  if ((N == NULL) || (AString == NULL))
    {
    return(FAILURE);
    }

  if (AList != NULL)
    AList[0] = mwnaLAST;

  time(&tmpTime);

  N->CTime = (long)tmpTime;
  N->MTime = (long)tmpTime;
  N->ATime = (long)tmpTime;

  N->RM = R;

  N->TaskCount = 0;

  ptr = AString;

  acount = 0;

  /* FORMAT:  <FIELD>=<VALUE>;[<FIELD>=<VALUE>;]... */

  while (ptr[0] != '\0')
    {
    if ((tail = MUStrChr(ptr,';')) != NULL)
      {
      strncpy(NodeAttr,ptr,MIN(MMAX_BUFFER - 1,tail - ptr));
      NodeAttr[tail - ptr] = '\0';
      }
    else
      {
      MUStrCpy(NodeAttr,ptr,MMAX_BUFFER);
      }

    if (!strstr(NodeAttr,"AFS=") && !strstr(NodeAttr,"CFS="))
      {
      /* NOTE: MUPurgeEscape() conflicts w/XML */

      MUPurgeEscape(NodeAttr); 
      }

    if (MWikiNodeUpdateAttr(NodeAttr,-1,N,R,&AIndex,EMsg) == FAILURE)
      {
      snprintf(tmpLine,sizeof(tmpLine),"info corrupt for node %s - %s",
        N->Name,
        EMsg);

      MMBAdd(&R->MB,tmpLine,NULL,mmbtNONE,0,0,NULL);
      }

    if ((AList != NULL) && (AIndex != mwnaNONE) && (acount < mwnaLAST))
      {
      AList[acount] = AIndex;

      acount++; 
      }

    if (tail == NULL)
      break;

    ptr = tail + 1;
    }  /* END while ((ptr[0] != '\0') */

  if (AList != NULL)
    AList[MIN(mwnaLAST,acount)] = mwnaLAST;

  /* NOTE:  if node reports in state running and nodeaccesspolicy is not shared, effective node
            state should be busy (NYI) */

  N->EState       = N->State;

  N->SyncDeadLine = MMAX_TIME;
  N->StateMTime   = MSched.Time;

  N->STTime       = 0;
  N->SUTime       = 0;
  N->SATime       = 0;

  if ((N->State != mnsActive) && 
      (N->State != mnsIdle) && 
      (N->State != mnsUnknown) &&
      (N->State != mnsUp))
    {
    N->ARes.Procs = 0;
    }

  MDB(6,fWIKI)
    {
    char Line[MMAX_LINE];

    mstring_t Classes(MMAX_LINE);
  
    MClassListToMString(&N->Classes,&Classes,NULL);
  
    MLog("INFO:     MNode[%03d] '%18s' %9s VM: %8d Mem: %5d Dk: %5d Cl: %s %s\n",
      N->Index,
      N->Name,
      MNodeState[N->State],
      N->CRes.Swap,
      N->CRes.Mem,
      N->CRes.Disk,
      (Classes.empty()) ? NONE : Classes.c_str(),
      MUNodeFeaturesToString(',',&N->FBM,Line));
    }

  return(SUCCESS);
  }  /* END MWikiNodeLoad() */



/**
 * Update node attributes from WIKI text string.
 *
 * @see MWikiNodeUpdateAttr() - child
 *
 * @param AList (I)
 * @param N (I) [modified]
 * @param R (I) [optional]
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 */

int MWikiNodeUpdate(

  char    *AList,  /* I */
  mnode_t *N,      /* I (modified) */
  mrm_t   *R,      /* I (optional) */
  char    *EMsg)   /* O (optional,minsize=MMAX_LINE) */

  {
  char  NodeAttr[MMAX_BUFFER];
  char  DelayAttrs[MMAX_BUFFER];
  char *ptr;
  char *tail;

  char  tEMsg[MMAX_LINE];

  const char *FName = "MWikiNodeUpdate";

  MDB(4,fWIKI) MLog("%s(%.16s,%s,%s,EMsg)\n",
    FName,
    (AList != NULL) ? AList : "NULL",
    (N != NULL) ? N->Name : "NULL",
    (R != NULL) ? R->Name : "NULL");

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if ((AList == NULL) || (N == NULL))
    {
    return(FAILURE);
    }

  tEMsg[0] = '\0';

  ptr = AList;

  DelayAttrs[0] = '\0';

  /* FORMAT:  <FIELD>=<VALUE>[:<FIELD>=<VALUE>]... */
 
  while ((tail = MUStrChr(ptr,';')) != NULL)
    {
    if (tail > ptr)
      {
      strncpy(NodeAttr,ptr,tail - ptr);
      NodeAttr[tail - ptr] = '\0';

      if ((strstr(NodeAttr,"AFS=") == NULL) && (strstr(NodeAttr,"CFS=") == NULL))
        {
        /* NOTE: MUPurgeEscape() conflicts w/XML */

        MUPurgeEscape(NodeAttr);
        }

      if ((strstr(NodeAttr,MWikiNodeAttr[mwnaAClass]) != NULL) ||
          (strstr(NodeAttr,MWikiNodeAttr[mwnaCClass]) != NULL))
        {
        /* These attributes may be dependent on other attributes, delay processing */

        MUStrCat(DelayAttrs,NodeAttr,sizeof(DelayAttrs));
        MUStrCat(DelayAttrs,";",sizeof(DelayAttrs));
        }
      else
        {
        MWikiNodeUpdateAttr(
          NodeAttr,
          -1,
          N,
          R,
          NULL,
          (EMsg != NULL) ? tEMsg : NULL);

        if ((EMsg != NULL) && (tEMsg[0] != '\0') && (EMsg[0] == '\0'))
          {
          MUStrCpy(EMsg,tEMsg,MMAX_LINE);
          }
        }
      }

    ptr = tail + 1;
    }  /* END while ((tail = MUStrChr(ptr,';')) != NULL) */

  /* Process delayed attrs */

  ptr = DelayAttrs;

  while ((tail = MUStrChr(ptr,';')) != NULL)
    {
    if (tail > ptr)
      {
      strncpy(NodeAttr,ptr,tail - ptr);
      NodeAttr[tail - ptr] = '\0';

      MWikiNodeUpdateAttr(
        NodeAttr,
        -1,
        N,
        R,
        NULL,
        (EMsg != NULL) ? tEMsg : NULL);

      if ((EMsg != NULL) && (tEMsg[0] != '\0') && (EMsg[0] == '\0'))
        {
        MUStrCpy(EMsg,tEMsg,MMAX_LINE);
        }
      }

    ptr = tail + 1;
    }

  return(SUCCESS);
  }  /* END MWikiNodeUpdate() */



/**
 * Update single node attribute from WIKI text string.
 *
 * @see MWikiNodeUpdate() - parent
 * @see MWikiVMUpdate() - parent
 *
 * @see MWikiNodeAttrEnum
 *
 * @param AttrString (I)
 * @param SAIndex    (I) specified attribute index [optional]
 * @param N          (I) [modified]
 * @param R          (I) [optional]
 * @param AIndexP    (O) [optional]
 * @param EMsg       (O) [optional,minsize=MMAX_LINE]
 *
 * Step 1.0  Initialize
 * Step 2.0  Update Attribute
 */

int MWikiNodeUpdateAttr(

  char                *AttrString,
  int                  SAIndex,
  mnode_t             *N,
  mrm_t               *R,
  enum MWNodeAttrEnum *AIndexP,
  char                *EMsg)

  {
  char *ptr;
  char *Value;
  char *tail;

  char  Buf[MMAX_BUFFER];
  char  IName[MMAX_NAME];

  int   aindex;

  char *TokPtr;

  int   index;
  int   tmpInt;
 
  mpar_t *P;

  const char *FName = "MWikiNodeUpdateAttr";

  MDB(6,fWIKI) MLog("%s(%.32s,%d,%s,R,AIndexP,EMsg)\n",
    FName,
    (AttrString != NULL) ? AttrString : "NULL",
    SAIndex,
    (N != NULL) ? N->Name : "NULL");

  /* Step 1.0  Initialize */

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (AIndexP != NULL)
    *AIndexP = mwnaNONE;

  if ((N == NULL) || (AttrString == NULL))
    {
    return(FAILURE);
    }

  if ((ptr = MUStrChr(AttrString,'=')) == NULL)
    {
    if (EMsg != NULL)
      {
      snprintf(EMsg,MMAX_LINE,"attribute '%s' malformed - no '='",
        AttrString);
      }

    return(FAILURE);
    }

  /* FORMAT:  <ATTR>[[INAME]]{ \t\n=}*{<ALNUM_STRING>|<XML>} */

  IName[0] = '\0';

  if (SAIndex > 0)
    {
    /* node attribute index specified by caller */

    aindex = SAIndex;
    }
  else 
    {
    MWikiGetAttrIndex(AttrString,(char **)MWikiNodeAttr,&aindex,IName);
    }

  if ((aindex == 0) || (MWikiNodeAttr[aindex] == NULL))
    {
    if (EMsg != NULL)
      {
      snprintf(EMsg,MMAX_LINE,"invalid attribute '%s' specified",
        AttrString);
      }

    return(FAILURE);
    }

  if (AIndexP != NULL)
    *AIndexP = (enum MWNodeAttrEnum)aindex;

  ptr++;

  /* locate value */

  while (isspace(*ptr) || (*ptr == '='))
    ptr++;

  Value = ptr;

  /* Step 2.0  Update Attribute */

  switch (aindex)
    {
    case mwnaAClass:

      /* FORMAT: <CLASS>[:<COUNT>][,<CLASS>[:<COUNT>]]... */

      bmset(&N->IFlags,mnifRMClassUpdate);

      if (strcmp(Value,NONE))
        {
        MClassListFromString(&N->Classes,Value,N->RM);
        }    /* END if (strcmp(Value,NONE)) */

      break;

    case mwnaADisk:

      N->ARes.Disk = (int)strtol(Value,NULL,10);

      break;

    case mwnaAFS:

      {
      mxml_t *E;
      char   *tail;

      int     fsindex;

      char    tmpLine[MMAX_LINE];

      /* FORMAT:  <fs id="X" size="X" io="Y" rcount="X" wcount="X" ocount="X"></fs>... */

      if (N->FSys == NULL)
        {
        N->FSys = (mfsys_t *)MUCalloc(1,sizeof(mfsys_t));
        }

      E = NULL;

      tail = Value;

      while (MXMLFromString(&E,tail,&tail,NULL) == SUCCESS)
        {
        if (MXMLGetAttr(E,"id",NULL,tmpLine,0) == FAILURE)
          {
          continue;
          }

        for (fsindex = 0;fsindex < MMAX_FSYS;fsindex++)
          {
          if (N->FSys->Name[fsindex] == NULL)
            {
            /* add new filesystem */

            MUStrDup(&N->FSys->Name[fsindex],tmpLine);

            break;
            }

          if (!strcmp(N->FSys->Name[fsindex],tmpLine))
            {
            /* filesystem located */

            break;
            }
          }    /* END for (fsindex) */

        if (fsindex >= MMAX_FSYS)
          {
          /* max filesystem count exceeded */

          /* ignore current filesystem */

          continue;
          }

        if (MXMLGetAttr(E,"size",NULL,tmpLine,0) == SUCCESS)
          {
          N->FSys->ASize[fsindex] = (int)MURSpecToL(tmpLine,mvmMega,mvmKilo);
          }

        if (MXMLGetAttr(E,"io",NULL,tmpLine,0) == SUCCESS)
          {
          N->FSys->AIO[fsindex] = strtod(tmpLine,NULL);
          }

        if (MXMLGetAttr(E,"rcount",NULL,tmpLine,0) == SUCCESS)
          {
          N->FSys->RJCount[fsindex] = (int)strtol(tmpLine,NULL,10);
          }

        if (MXMLGetAttr(E,"wcount",NULL,tmpLine,0) == SUCCESS)
          {
          N->FSys->WJCount[fsindex] = (int)strtol(tmpLine,NULL,10);
          }

        if (MXMLGetAttr(E,"ocount",NULL,tmpLine,0) == SUCCESS)
          {
          N->FSys->OJCount[fsindex] = (int)strtol(tmpLine,NULL,10);
          }

        fsindex++;

        MXMLDestroyE(&E);
        }  /* END while (MXMLFromString() == SUCCESS) */
      }    /* END BLOCK (case mwnaAFS) */

      break;

    case mwnaAMemory:

      {
      int tmpI;

      tmpI = (int)strtol(Value,NULL,10);

      /* NOTE:  do not scale available memory - only configured procs */

      N->ARes.Mem = tmpI;
      }

      break;

    case mwnaAProc:

      {
      int tmpI;

      tmpI = (int)strtol(Value,NULL,10);

      /* NOTE:  do not scale available procs - only configured procs */

      N->ARes.Procs = tmpI;
      }

      break;

    case mwnaArch:

      N->Arch = MUMAGetIndex(meArch,Value,mAdd);

      break;

    case mwnaARes:

      {
      int tmpI;

      /* FORMAT:  <NAME>[:<COUNT>][,<NAME>[:<COUNT>]]...   */

      strncpy(Buf,Value,sizeof(Buf));

      ptr = MUStrTok(Buf,",",&TokPtr);

      while (ptr != NULL)
        {
        if ((tail = strchr(ptr,':')) != NULL)
          {
          *tail = '\0';

          index = MUMAGetIndex(meGRes,ptr,mAdd);

          tmpI = (int)strtol(tail + 1,NULL,10);
          }
        else
          {
          index = MUMAGetIndex(meGRes,ptr,mAdd);

          tmpI = 1;
          }

        if ((index == FAILURE) || (index >= MSched.M[mxoxGRes]))
          {
          if (MSched.OFOID[mxoxGRes] == NULL)
            MUStrDup(&MSched.OFOID[mxoxGRes],ptr);

          MDB(1,fWIKI) MLog("ALERT:    gres '%s' cannot be added to node %s\n",
            ptr,   
            N->Name);

          return(FAILURE);
          }
 
        MSNLSetCount(&N->ARes.GenericRes,index,tmpI);

        if (MSNLGetIndexCount(&N->CRes.GenericRes,index) < MSNLGetIndexCount(&N->ARes.GenericRes,index))
          {
          /* config GRES not explicitly specified - available gres CANNOT exceed
             configured gres */

          MSNLSetCount(&N->CRes.GenericRes,index,tmpI);
          }

        if ((R != NULL) && bmisset(&R->RTypes,mrmrtLicense))
          {
          /* update license RM stats */

          R->U.Nat.CurrentResAvailCount += tmpI;

          MSNLSetCount(&R->U.Nat.ResAvailCount,index,MSNLGetIndexCount(&R->U.Nat.ResAvailCount,index) + tmpI);

          MGRes.GRes[index]->RM = R;
          }

        if (MSched.AResInfoAvail == NULL)
          {
          MSched.AResInfoAvail = (mbool_t **)MUCalloc(1,sizeof(void *) * MSched.M[mxoNode]);

          MSched.AResInfoAvail[N->Index] = (mbool_t *)MUCalloc(1,sizeof(mbool_t) * (MSched.M[mxoxGRes] + 1));
          }
        else if (MSched.AResInfoAvail[N->Index] == NULL)
          {
          MSched.AResInfoAvail[N->Index] = (mbool_t *)MUCalloc(1,sizeof(mbool_t) * (MSched.M[mxoxGRes] + 1));
          }

        MSched.AResInfoAvail[N->Index][index] = TRUE;

        ptr = MUStrTok(NULL,",",&TokPtr);
        }  /* END while (ptr != NULL) */
      }    /* END BLOCK (mwnaARes) */

      break;

    case mwnaASwap:

      tmpInt = (int)strtol(Value,NULL,10);
      MNodeSetAttr(N,mnaRASwap,(void **)&tmpInt,mdfInt,mSet);

      break;

    case mwnaAttr:

      {
      char *ptr;
      char *ptr2;

      char *TokPtr;
      char *TokPtr2;

      /* FORMAT:  <NAME>{=}<VALUE>[;<NAME>{=}<VALUE>]... */
      /* below code only recognizes ONE value and doesn't treat '+' as a delimeter */

      ptr = MUStrTok(Value,";",&TokPtr);

      while (ptr != NULL)
        {
        ptr2 = MUStrTok(ptr,"=",&TokPtr2);

        N->AttrList.insert(std::pair<std::string,std::string>(ptr2,TokPtr2));

        ptr = MUStrTok(NULL,";",&TokPtr);
        }  /* END while (ptr != NULL) */
      }    /* END BLOCK (case mwnaAttr) */

      break;

    case mwnaCat:

      /* node failure reason */

      MNodeSetRMMessage(N,Value,R);

      break;

    case mwnaCClass:

      /* FORMAT:  <CLASS>[:<COUNT>][,<CLASS>[:<COUNT>]]... */

      bmset(&N->IFlags,mnifRMClassUpdate);

      if (strcmp(Value,NONE))
        {
        MClassListFromString(&N->Classes,Value,N->RM);
        }    /* END if (strcmp(Value,NONE)) */

      break;

    case mwnaCDisk:

      N->CRes.Disk = (int)strtol(Value,NULL,10);
      MNodeCreateBaseValue(N,mrlDisk,(int)strtol(Value,NULL,10));

      break;

    case mwnaCFS:

      {
      mxml_t *E;
      char   *tail;

      int     fsindex;

      char    tmpLine[MMAX_LINE];

      /* NOTE:  order and quantity of fs's must not change over time */

      /* FORMAT:  <fs id="X" size="X" io="Y"></fs>... */

      if (N->FSys == NULL)
        {
        N->FSys = (mfsys_t *)MUCalloc(1,sizeof(mfsys_t));
        }

      E = NULL;

      tail = Value;

      while (MXMLFromString(&E,tail,&tail,NULL) == SUCCESS)
        {
        if (MXMLGetAttr(E,"id",NULL,tmpLine,0) == FAILURE)
          {
          continue;
          }

        for (fsindex = 0;fsindex < MMAX_FSYS;fsindex++)
          {
          if (N->FSys->Name[fsindex] == NULL)
            {
            /* add new filesystem */

            MUStrDup(&N->FSys->Name[fsindex],tmpLine);

            break;
            }

          if (!strcmp(N->FSys->Name[fsindex],tmpLine))
            {
            /* filesystem located */

            break;
            }
          }    /* END for (fsindex) */

        if (fsindex >= MMAX_FSYS)
          {
          /* max filesystem count exceeded */

          /* ignore current filesystem */

          continue;
          }

        if (MXMLGetAttr(E,"size",NULL,tmpLine,0) == SUCCESS)
          {
          N->FSys->CSize[fsindex] = (int)MURSpecToL(tmpLine,mvmMega,mvmKilo);
          }

        if (MXMLGetAttr(E,"io",NULL,tmpLine,0) == SUCCESS)
          {
          N->FSys->CMaxIO[fsindex] = strtod(tmpLine,NULL);
          }

        MXMLDestroyE(&E);
        }  /* END while (MXMLFromString() == SUCCESS) */
      }    /* END BLOCK */

      break;

    case mwnaChargeRate:

      MNodeSetAttr(N,mnaChargeRate,(void **)Value,mdfString,mSet);

      break;

    case mwnaCMemory:

      {
      int tmpI;

      tmpI = (int)strtol(Value,NULL,10);

      if (MSched.ResOvercommitSpecified[mrlMem] == TRUE)
        {
        N->CRes.Mem = MNodeGetOCValue(N,tmpI,mrlMem);
        MNodeCreateBaseValue(N,mrlMem,tmpI);
        }
      else if (!bmisset(&N->IFlags,mnifMemOverride))
        N->CRes.Mem = tmpI;
      }

      break;

    case mwnaCProc:

      {
      int tmpI;

      tmpI = (int)strtol(Value,NULL,10);

      if (MSched.ResOvercommitSpecified[mrlProc] == TRUE)
        {
        N->CRes.Procs = MNodeGetOCValue(N,tmpI,mrlProc);
        MNodeCreateBaseValue(N,mrlProc,tmpI);
        }
      else
        N->CRes.Procs = tmpI;
      }

      break;

    case mwnaCRes:

      { 
      int tmpI;

      /* FORMAT:  <NAME>[:<COUNT>][,<NAME>[:<COUNT>]]...   */

      strncpy(Buf,Value,sizeof(Buf));

      ptr = MUStrTok(Buf,",",&TokPtr);

      while (ptr != NULL)
        {
        if ((tail = strchr(ptr,':')) != NULL)
          {
          *tail = '\0';

          if (ptr[0] == '\0')
            {
            ptr = MUStrTok(NULL,",",&TokPtr);

            continue;
            }

          index = MUMAGetIndex(meGRes,ptr,mAdd);

          tmpI = (int)strtol(tail + 1,NULL,10);
          }
        else
          {
          index = MUGenericGetIndex(meGRes,ptr,mAdd);

          tmpI = 1;
          }

        if ((index == FAILURE) || (index >= MSched.M[mxoxGRes]))
          {
          MDB(1,fWIKI) MLog("ALERT:    gres '%s' cannot be added to node %s\n",
            ptr,
            N->Name);

          if (EMsg != NULL)
            snprintf(EMsg,MMAX_LINE,"gres '%s' cannot be added to node",
              ptr);             

          return(FAILURE);
          }

        if (MGRes.GRes[index]->FeatureGRes == TRUE)
          {
          int sindex = MUGetNodeFeature(ptr,mAdd,NULL);

          if ((sindex == 0) || (sindex >= MMAX_ATTR))
            {
            MDB(2,fSTRUCT) MLog("ALERT:    cannot add FeatureGRes '%s'\n",
              ptr);

            break;
            }

          if (tmpI > 0)
            {
            bmset(&MSched.FeatureGRes,sindex);
            }
          else if (tmpI == 0)
            {
            bmunset(&MSched.FeatureGRes,sindex);
            }

          ptr = MUStrTok(NULL,", \t\n",&TokPtr);

          continue;
          }

        MSNLSetCount(&N->CRes.GenericRes,index,tmpI);

        if ((R != NULL) && bmisset(&R->RTypes,mrmrtLicense))
          {
          /* update license rm stats */

          R->U.Nat.CurrentUResCount++;
          R->U.Nat.CurrentResConfigCount += tmpI;

          MSNLSetCount(&R->U.Nat.ResConfigCount,index,MSNLGetIndexCount(&R->U.Nat.ResConfigCount,index) + tmpI);

          MGRes.GRes[index]->RM = R;
          }

        ptr = MUStrTok(NULL,",",&TokPtr);
        }  /* END while (ptr != NULL) */
      }    /* END (case mwnaCRes) */

      break;

    case mwnaCPULoad:

      N->Load = (double)atof(Value);

      break;

    case mwnaCPUSpeed:

      N->ProcSpeed = MAX(0,(int)strtol(Value,NULL,10));

      break;

    case mwnaCSwap:

      tmpInt = (int)strtol(Value,NULL,10);
      MNodeCreateBaseValue(N,mrlSwap,tmpInt);
      MNodeSetAttr(N,mnaRCSwap,(void **)&tmpInt,mdfInt,mSet);

      break;

    case mwnaFeature:

      /* FORMAT: <FEATURE>[{: ,}<FEATURE>]... */

      {
      char  tmpLine[MMAX_LINE];

      MUStrCpy(tmpLine,Value,sizeof(tmpLine));

      if (bmisclear(&N->FBM) || bmisset(&N->RMAOBM,mnaFeatures))
        {
        MNodeSetAttr(N,mnaFeatures,(void **)tmpLine,mdfString,
          ((MSched.NodeFeaturesSpecifiedInConfig == TRUE) || (bmisset(&MSched.Flags,mschedfAggregateNodeFeatures))) ? mAdd : mSet);
  
        /* Mark bit indicating RM feature ownership */

        bmset(&N->RMAOBM,mnaFeatures);
        }
      }    /* END BLOCK (case mwnaFeature) */

      break;

    case mwnaGEvent:

      {
      char *ptr;
      char *TokPtr;
      char *MTime;

      char UnquotedValue[MMAX_BUFFER];

      mgevent_obj_t  *GEvent;
      mgevent_desc_t *GEDesc;
      mulong NewMTime = MSched.Time;
      /* generic event */

      /* FORMAT:  GEVENT[hitemp]='xxx' */
      /*  With mtime: GEVENT[hitemp:408174934]='xxx' */

      if ((IName[0] == '\0') || (Value[0] == '\0'))
        {
        /* mal-formed value, ignore */

        break;
        }

      ptr = MUStrTok(IName,":",&TokPtr);

      /* Add the GEvent Description and Item Object */
      MGEventAddDesc(ptr);
      MGEventGetOrAddItem(ptr,&N->GEventsList,&GEvent);

      /* check if event is associated with a VM */


      if ((TokPtr != NULL) && (strlen(TokPtr) > 0))
        {
        /* Still something in TokPtr after ':', it's severity */

        MTime = MUStrTok(NULL,":",&TokPtr);

        if (MTime != NULL)
          {
          NewMTime = (int)strtol(MTime,NULL,10);

          if (NewMTime <= 0)
            {
            NewMTime = MSched.Time;
            }
          }
        }

      GEvent->Severity = 0;

      MUStrCpyQuoted(UnquotedValue,Value,sizeof(UnquotedValue));

      MUStrDup(&GEvent->GEventMsg,UnquotedValue);

      MGEventGetDescInfo(ptr,&GEDesc);

      if (!strncasecmp(UnquotedValue,"'vm:",strlen("vm:")))
        {
        char *Message = NULL;

        char *ptr2;
        char *TokPtr2;

        MUStrDup(&Message,UnquotedValue);

        /* parse off vm ID */

        ptr2 = MUStrTok(Message,"':",&TokPtr2);

        ptr2 = MUStrTok(NULL,":",&TokPtr2);

        if ((ptr2 != NULL) && (ptr2[0] != '\0'))
          {
          if (GEvent->GEventMTime + GEDesc->GEventReArm <= MSched.Time)
            {
            GEvent->GEventMTime = NewMTime;
            /* process event as new */

            MGEventProcessGEvent(
              -1,
              mxoxVM,
              ptr2,
              ptr,
              0.0,
              mrelGEvent,
              UnquotedValue);

            MUFree(&Message);

            break;
            }
          }  /* END if ((ptr2 != NULL) && (ptr2[0] != '\0')) */

        MUFree(&Message);
        }    /* END if (!strncasecmp(Value,"vm:",strlen("vm:"))) */

      /* always update message */

      if (GEvent->GEventMTime + GEDesc->GEventReArm <= MSched.Time)
        {
        GEvent->GEventMTime = MSched.Time;

        MGEventProcessGEvent(
          -1,
          mxoNode,
          N->Name,
          ptr,
          0.0,
          mrelGEvent,
          UnquotedValue);
        }
      }  /* END BLOCK (case mwnaGEvent) */

      break;

    case mwnaGMetric:

      {
      int gmindex;
      double DVal;

      /* generic metrics */

      /* FORMAT:  GMETRIC[temp]=113.2 */

      if (MNodeInitXLoad(N) == FAILURE)
        {
        /* cannot allocate memory */

        break;
        }

      if ((IName[0] == '\0') || (Value[0] == '\0'))
        {
        /* malformed value, ignore */

        if (EMsg != NULL)
          {
          if (IName[0] == '\0')
            {
            snprintf(EMsg,MMAX_LINE,"gmetric attribute malformed - no label");
            }
          else
            {
            snprintf(EMsg,MMAX_LINE,"gmetric attribute '%s' malformed - no value", 
              IName);
            }
          }

        return(FAILURE);
        }

      gmindex = MUMAGetIndex(meGMetrics,IName,mAdd);

      if ((gmindex <= 0) || (gmindex >= MSched.M[mxoxGMetric]))
        {
        if (EMsg != NULL)
          {
          snprintf(EMsg,MMAX_LINE,"cannot add gmetric '%s' - buffer full",
            IName);
          }

        return(FAILURE);
        }

      DVal = strtod(Value,NULL);

      if (DVal != MDEF_GMETRIC_VALUE)
        { 
        N->XLoad->GMetric[gmindex]->IntervalLoad = DVal;
        N->XLoad->GMetric[gmindex]->SampleCount++;
        }

      bmset(&N->XLoad->RMSetBM,gmindex);

      /* check if gmetric has event handler (NYI) */

      if ((MGMetric.GMetricThreshold[gmindex] != 0.0) &&
          (MUDCompare(
             DVal,
             MGMetric.GMetricCmp[gmindex],
             MGMetric.GMetricThreshold[gmindex])) &&
          (N->XLoad->GMetric[gmindex]->EMTime + MGMetric.GMetricReArm[gmindex] <= MSched.Time))
        {
        N->XLoad->GMetric[gmindex]->EMTime = MSched.Time;

        MGEventProcessGEvent(
          gmindex,
          mxoNode,
          N->Name,
          NULL,
          MGMetric.GMetricThreshold[gmindex],
          mrelGEvent,
          Value);
        }
      }  /* END BLOCK (case mwnaGMetric) */
 
      break;

    case mwnaIdleTime:

      MNodeSetAttr(N,mnaKbdIdleTime,(void **)Value,mdfString,mSet);

      break;

    case mwnaMaxTask:

      /* NYI */

      break;

    case mwnaMessage:

      /* FORMAT: [<LABEL>:]<MESSAGE> */

      MMBAdd(&N->MB,(char *)Value,NULL,mmbtNONE,0,0,NULL);

      break;

/*
    case mwnaName:

      MUStrCpy(N->Name,(char *)Value,sizeof(N->Name));

      break;
*/

    case mwnaNodeIndex:

      MNodeSetAttr(N,mnaNodeIndex,(void **)Value,mdfString,mSet);

      break;

    case mwnaOS:

      N->ActiveOS = MUMAGetIndex(meOpsys,Value,mAdd);

      break;

    case mwnaOSList:

      /* FORMAT:  [<APPLICATION>@]<OS>[:<PROVDURATION>][,[<APPLICATION>@]<OS>[:<PROVDURATION>]... */

      MNodeSetAttr(N,mnaOSList,(void **)Value,mdfString,mSet);

      if ((N->ActiveOS == 0) && (MOLDISSET(MSched.LicensedFeatures,mlfProvision)))
        {
        if ((N->OSList != NULL) && (N->OSList[1].AIndex <= 0))
          MNodeSetAttr(N,mnaOS,(void **)MAList[meOpsys][N->OSList[0].AIndex],mdfString,mSet);
        else
          MNodeSetAttr(N,mnaOS,(void **)ANY,mdfString,mSet);
        }
      else if (!MOLDISSET(MSched.LicensedFeatures,mlfProvision))
        {
        MMBAdd(&MSched.MB,"ERROR:  license does not allow OS provisioning, please contact Adaptive Computing\n",NULL,mmbtNONE,0,0,NULL);
        }

      break;

    case mwnaOther:

      MNodeSetAttr(N,mnaVariables,(void **)Value,mdfString,mSet);

      break;

    case mwnaPartition:

      if (MParAdd(Value,&P) == SUCCESS)
        {
        MParAddNode(P,N);
        }

      break;

    case mwnaPowerIsEnabled:

      MNodeSetAttrOwner(N,R,mnaPowerIsEnabled);

      MNodeSetAttr(N,mnaPowerIsEnabled,(void **)Value,mdfString,mSet);

      break;

    case mwnaPriority:

      MNodeSetAttr(N,mnaPriority,(void **)Value,mdfString,mSet);
      N->PrioritySet = TRUE;

      break;

    case mwnaRack:

      MNodeSetAttr(N,mnaRack,(void **)Value,mdfString,mSet);

      break;

    case mwnaSlot:

      MNodeSetAttr(N,mnaSlot,(void **)Value,mdfString,mSet);

      break;

    case mwnaSpeed:

      N->Speed = MIN(MMAX_NODESPEED,strtod(Value,NULL));

      N->Speed = MAX(0.0,N->Speed);

      break;

    case mwnaState:

      {
      char *ptr;
      char *TokPtr;

      enum MNodeStateEnum OldState = N->State;
      enum MNodeStateEnum OldRMState = mnsNONE;
      enum MNodeStateEnum NState;

      int  RIndex;

      ptr = MUStrTok(Value,":",&TokPtr);
       
      if (ptr == NULL)
        break;

      if (N->SpecState != mnsNONE)
        break;  /* Moab's internal state is overriding RM */

      if (!strcmp(ptr,"Draining"))
        {
        /* NOTE:  normally, DRAIN maps to mnsDraining */

        NState = mnsDraining;
        }
      else
        {
        NState = (enum MNodeStateEnum)MUGetIndexCI(ptr,MNodeState,TRUE,mnsNONE);
        }

      /* set substate, if one exists */
      /* NOTE:  substate only supported w/one RM per node (FIXME) */

      ptr = MUStrTok(NULL,":",&TokPtr);

      MUStrDup(&N->SubState,ptr);

      if ((N->SubState != NULL) &&
          (N->SubState[0] != '\0') &&
          ((N->LastSubState == NULL) ||
           (strcmp(N->LastSubState,N->SubState))))
        {
        N->LastSubStateMTime = MSched.Time;
        MUStrDup(&N->LastSubState,N->SubState);
        }

      if ((R != NULL) && (MRMIgnoreNodeStateInfo(R) == TRUE))
        {
        /* only set RM-specific node state and exit */

        if (bmisset(&R->RTypes,mrmrtProvisioning))
          {
          mjob_t *J;

          MDB(7,fRM) MLog("INFO:     state '%s' reported for node '%s' by prov rm '%s'\n",
            MNodeState[NState],
            N->Name,
            R->Name);
 
          if ((NState == mnsDown) && (MNodeGetSysJob(N,msjtOSProvision,MBNOTSET,&J) == SUCCESS))
            {
            if (MJOBISACTIVE(J) == TRUE)
              {
              /* if provisioning RM, and provisioning job is active, mask state to mnsRunning */

              NState = mnsActive;
              }
            } 
          }

        if ((bmisset(&R->Flags,mrmfNoCreateResource)) || 
            (N->RM != NULL))
          {
          N->RMState[R->Index] = NState;
          }

        break;  
        }  /* END if ((R != NULL) && (MRMIgnoreNodeStateInfo(R) == TRUE)) */

      RIndex = -1;

      if (R != NULL)
        {
        /* slave RM's should not override info from primary RM */

        RIndex = R->Index;

        OldRMState = N->RMState[RIndex];

        if (bmisset(&R->RTypes,mrmrtProvisioning))
          {
          mjob_t *J;

          if ((NState == mnsDown) && 
              (MNodeGetSysJob(N,msjtOSProvision,MBNOTSET,&J) == SUCCESS))
            {
            if (MJOBISACTIVE(J) == TRUE)
              {
              /* if provisioning RM, and provisioning job is active, mask state 
                 to mnsRunning */

              NState = mnsActive;
              }
            } 
          }

        N->RMState[RIndex] = NState;

        /* code below breaks multi-rm setups where both RM's can create resources */

        /*
        if (!bmisset(&R->Flags,mrmfNoCreateResource))
          N->State = NState;
        */
        }
      else if (N->RM != NULL)
        {
        /* NOTE:  only set N->RMState[N->RM->Index] to NState if R is not set */

        /* master RM will update self */

        RIndex = N->RM->Index;

        OldRMState = N->RMState[RIndex];

        /* NOTE:  if state is mnsUnknown, should we update both N->State and 
                  N->RMState values when actual state is determined in 
                  MNodeAdjustAvailResources()? */

        N->RMState[RIndex] = NState;

        N->State = NState;
        }
      else
        {
        OldRMState = N->State;

        N->State = NState;
        }

      if ((OldState != mnsNONE) && 
          (N->State != OldState) &&
          (R != NULL) &&
          !bmisset(&R->Flags,mrmfNoCreateResource) &&
         ((OldRMState == mnsNONE) || (OldRMState != N->RMState[R->Index])))
        {
        /* if per-RM node state is maintained, only report updated state if 
           effective node state has changed AND reported RM node state has 
           changed */

        MDB(2,fRM) MLog("INFO:     node '%s' changed states from %s to %s\n",
          N->Name,
          MNodeState[OldState],
          MNodeState[N->State]);
   
        MSched.EnvChanged = TRUE;
    
        N->StateMTime = MSched.Time;

        /* handle <ANYSTATE> to 'Down/Drain/Flush' state transitions */

        if ((N->State == mnsDown) ||
            (N->State == mnsDraining) ||
            (N->State == mnsDrained) ||
            (N->State == mnsFlush))
          {
          N->EState       = N->State;
          N->SyncDeadLine = MMAX_TIME;

          /* only report event for single node, not default */

/*
          MOReportEvent((void *)&MSched.DefaultN,
            MSched.DefaultN.Name,
            mxoNode,
            mttFailure,
            MSched.Time,
            TRUE);
*/

          MOReportEvent((void *)N,
            N->Name,
            mxoNode,
            mttFailure,
            MSched.Time,
            TRUE);
          }  /* END if ((N->State == mnsDown) || ...) */

        /* handle Down/Drain/Flush state to <any state> transitions */

        if ((OldState == mnsDown) ||
            (OldState == mnsDraining) ||
            (OldState == mnsDrained) ||
            (OldState == mnsFlush) ||
            (OldState == mnsReserved))
          {
          N->EState       = N->State;
          N->SyncDeadLine = MMAX_TIME;
          }

        if ((OldState == mnsIdle) && (N->State == mnsBusy))
          {
          if (MSched.Mode != msmMonitor)
            {
            MDB(1,fRM) MLog("ALERT:    unexpected node transition on node '%s'  Idle -> Busy\n",
              N->Name);
            }

          N->EState       = N->State;
          N->SyncDeadLine = MMAX_TIME;
          }
        }   /* END if (N->State != OldState) */
      }     /* END BLOCK (case mwnaState) */

      break;

    case mwnaType:

      /* NYI */

      break;

    case mwnaUpdateTime:

      /* this is the RM's update time (not Moab's) */

      N->ATime = strtol(Value,NULL,10);

      break;

    case mwnaVarAttr:

      {
      char *ptr;
      char *ptr2;

      char *TokPtr;
      char *TokPtr2;

      enum MHVTypeEnum NewHVType = mhvtNONE;
  
      /* FORMAT:  <NAME>{=:}<VERSION>[+<NAME>{=:}<VERSION>]... */

      /* set NewHVType in case it isn't specified in the wiki--this way we don't override existing HVType */

      NewHVType = N->HVType;

      ptr = MUStrTok(Value,"+",&TokPtr);

      while (ptr != NULL)
        {
        ptr2 = MUStrTok(ptr,":=",&TokPtr2);

        if ((ptr2 == NULL) || (TokPtr2 == NULL))
          {
          ptr = MUStrTok(NULL,"+",&TokPtr);

          continue;
          }
        
        if (MSched.ManageTransparentVMs == TRUE)
          {
          if (!strcasecmp(ptr2,"hvtype"))
            {
            /* maintain per-node hypervisor type (move to MAList[] index - NYI) */

            ptr2 = MUStrTok(NULL,":=",&TokPtr2);

            NewHVType = (enum MHVTypeEnum)MUGetIndexCI(ptr2,MHVType,FALSE,0);
            }
          else if (!strcasecmp(ptr2,"novmmigrations"))
            {
            /* exclude this node from any VM migration policies */

            bmset(&N->Flags,mnfNoVMMigrations);
            }
          else if (!strcasecmp(ptr2,"allowvmmigrations"))
            {
            bmunset(&N->Flags,mnfNoVMMigrations);
            }
          }
        else
          {
          N->AttrList.insert(std::pair<const char *,const char *>(ptr2,TokPtr2));
          }

        ptr = MUStrTok(NULL,"+",&TokPtr);
        }  /* END while (ptr != NULL) */

      if (N->HVType != NewHVType)
        {
        /* hypervisor type has changed */

        N->HVType = NewHVType;

        if (N->HVType == mhvtNONE)
          {
          /* clear */

          bmunset(&N->IFlags,mnifCanCreateVM);
          }
        else
          {
          /* set */

          bmset(&N->IFlags,mnifCanCreateVM);
          }

        MNodeUpdateOperations(N);
        }
      }    /* END BLOCK (case mwnaVarAttr) */

      break;

    case mwnaVariables:

      /* FORMAT:  '<VAR>=<VAL>[,<VAR>=<VAL>]...' */

      MNodeSetAttr(N,mnaVariables,(void **)Value,mdfString,mAdd);

      break;

    case mwnaVMOSList:

      /* FORMAT:  [<APPLICATION>@]<OS>[:<PROVDURATION>][,[<APPLICATION>@]<OS>[:<PROVDURATION>]... */

      MNodeSetAttr(N,mnaVMOSList,(void **)Value,mdfString,mSet);

      break;

    case mwnaNetAddr:

      MUStrDup(&N->NetAddr,Value);

      break;

    case mwnaXRes:

      /* external license usage, licenses used externally, that moab cannot track */

      {
      int tmpI;

      /* FORMAT:  <NAME>[:<COUNT>][,<NAME>[:<COUNT>]]...   */

      strncpy(Buf,Value,sizeof(Buf));

      ptr = MUStrTok(Buf,",",&TokPtr);

      while (ptr != NULL)
        {
        if ((tail = strchr(ptr,':')) != NULL)
          {
          *tail = '\0';

          index = MUMAGetIndex(meGRes,ptr,mAdd);

          tmpI = (int)strtol(tail + 1,NULL,10);
          }
        else
          {
          index = MUMAGetIndex(meGRes,ptr,mAdd);

          tmpI = 1;
          }

        if ((index == FAILURE) || (index >= MSched.M[mxoxGRes]))
          {
          if (MSched.OFOID[mxoxGRes] == NULL)
            MUStrDup(&MSched.OFOID[mxoxGRes],ptr);

          MDB(1,fWIKI) MLog("ALERT:    gres '%s' cannot be added to node %s\n",
            ptr,   
            N->Name);

          return(FAILURE);
          }
 
        if (N->XRes == NULL)
          {
          N->XRes = (mcres_t *)MUMalloc(sizeof(mcres_t));

          MCResInit(N->XRes);
          }

        MSNLSetCount(&N->XRes->GenericRes,index,tmpI);

        ptr = MUStrTok(NULL,",",&TokPtr);
        }  /* END while (ptr != NULL) */
      }    /* END BLOCK (mwnaARes) */

      break;
 
    default:

      /* unexpected node attribute */

      /* NO-OP */

      break;
    }  /* END switch (aindex) */

  return(SUCCESS);
  }  /* END MWikiNodeUpdateAttr() */




/**
 * Apply attributes set in the src node to the dst node.
 * 
 * This function mimics what MWikiNodeUpdateAttr in how it applys the values
 * of the source node to the destination node. 
 *
 * This function is used in node ranges. A template node is created and then
 * all of it's applied values are applied to the nodes in the range.
 *
 * NOTE:  used to apply node templates.
 *
 * @see MWikiClusterQuery() - parent
 *
 * @param DstN (I) Destination Node [modified]
 * @param SrcN (I) Source Node
 * @param R (I)
 * @param AList (I) List of attributes to copy from the src to dst
 */

int MWikiNodeApplyAttr(

  mnode_t             *DstN,  /* I (modified) */ 
  mnode_t             *SrcN,  /* I */
  mrm_t               *R,     /* I */
  enum MWNodeAttrEnum *AList) /* I */

  {
  int AIndex;
  int tmpInt;

  const char *FName = "MWikiNodeApplyAttr";

  MDB(2,fWIKI) MLog("%s(DstN,SrcN,%s,AList)\n",
    FName,
    (R != NULL) ? R->Name : "NULL");

  for (AIndex = 0;AList[AIndex] != mwnaLAST;AIndex++)
    {
    switch (AList[AIndex])
      {
      case mwnaAClass:

        bmset(&DstN->IFlags,mnifRMClassUpdate);
        
        bmcopy(&DstN->Classes,&SrcN->Classes);

        break;

      case mwnaADisk:

        DstN->ARes.Disk = SrcN->ARes.Disk;

        break;

      case mwnaAFS:

        {
        int fsindex;
      
        /* FORMAT:  <fs id="X" size="X" io="Y" rcount="X" wcount="X" ocount="X"></fs>... */
      
        if (DstN->FSys == NULL)
          {
          DstN->FSys = (mfsys_t *)MUCalloc(1,sizeof(mfsys_t));
          }

        DstN->FSys->BestFS = SrcN->FSys->BestFS;

        for (fsindex = 0;fsindex < MMAX_FSYS;fsindex++)
          {
          if (SrcN->FSys->Name[fsindex] == NULL)
            break;

          MUStrDup(&DstN->FSys->Name[fsindex],SrcN->FSys->Name[fsindex]);

          DstN->FSys->RJCount[fsindex] = SrcN->FSys->RJCount[fsindex];
          DstN->FSys->WJCount[fsindex] = SrcN->FSys->WJCount[fsindex];
          DstN->FSys->OJCount[fsindex] = SrcN->FSys->OJCount[fsindex];

          DstN->FSys->ASize[fsindex] = SrcN->FSys->ASize[fsindex];
          DstN->FSys->AIO[fsindex]   = SrcN->FSys->AIO[fsindex];
          }
        } /* END mwnaAFS block */

        break;

      case mwnaAMemory:

        tmpInt = SrcN->ARes.Mem;
        MNodeSetAttr(DstN,mnaRAMem,(void **)&tmpInt,mdfInt,mSet);

        break;

      case mwnaAProc:
      
        DstN->ARes.Procs = SrcN->ARes.Procs;
        
        break;
        
      case mwnaArch:
        
        DstN->Arch = SrcN->Arch;

        break;
       
      case mwnaARes:      /* available generic resources */

        {
        int gindex;

        MSNLCopy(&DstN->ARes.GenericRes,&SrcN->ARes.GenericRes);

        for (gindex = 1;gindex < MSched.M[mxoxGRes];gindex++)
          {
          if (MGRes.Name[gindex][0] == '\0')
            break;

          if ((R != NULL) && bmisset(&R->RTypes,mrmrtLicense))
            {
            /* update license RM stats */

            R->U.Nat.CurrentResAvailCount  += MSNLGetIndexCount(&DstN->ARes.GenericRes,gindex);

            MSNLSetCount(&R->U.Nat.ResAvailCount,gindex,MSNLGetIndexCount(&R->U.Nat.ResAvailCount,gindex) + MSNLGetIndexCount(&DstN->ARes.GenericRes,gindex));
            }

          if (MSched.AResInfoAvail == NULL)
            {
            MSched.AResInfoAvail = (mbool_t **)MUCalloc(1,sizeof(void *) * MSched.M[mxoNode]);

            MSched.AResInfoAvail[DstN->Index] = (mbool_t *)MUCalloc(1,sizeof(mbool_t) * (MSched.M[mxoxGRes] + 1));
            }
          else if (MSched.AResInfoAvail[DstN->Index] == NULL)
            {
            MSched.AResInfoAvail[DstN->Index] = (mbool_t *)MUCalloc(1,sizeof(mbool_t) * (MSched.M[mxoxGRes] + 1));
            }

          MSched.AResInfoAvail[DstN->Index][gindex] = TRUE;
          }
        }   /* END BLOCK (case mwnaARes) */

        break;

      case mwnaASwap:
      
        tmpInt = SrcN->ARes.Swap;
        MNodeSetAttr(DstN,mnaRASwap,(void **)&tmpInt,mdfInt,mSet);
        
        break;

      case mwnaCClass:
        
        bmset(&DstN->IFlags,mnifRMClassUpdate);
        
        bmcopy(&DstN->Classes,&SrcN->Classes);

        break;

      case mwnaCDisk:
        
        DstN->CRes.Disk = SrcN->CRes.Disk;
        
        break;

      case mwnaCFS:

        {
        int     fsindex;
      
        /* FORMAT:  <fs id="X" size="X" io="Y"></fs>... */

        if (DstN->FSys == NULL)
          {
          DstN->FSys = (mfsys_t *)MUCalloc(1,sizeof(mfsys_t));
          }

        for (fsindex = 0;fsindex < MMAX_FSYS;fsindex++)
          {
          if (SrcN->FSys->Name[fsindex] == NULL)
            break;

          DstN->FSys->CSize[fsindex]  = SrcN->FSys->CSize[fsindex];
          DstN->FSys->CMaxIO[fsindex] = SrcN->FSys->CMaxIO[fsindex]; 
          } 
        } /* END mwnaCFS block */

        break;

      case mwnaCMemory:
      
        tmpInt = SrcN->CRes.Mem;
        MNodeSetAttr(DstN,mnaRCMem,(void **)&tmpInt,mdfInt,mSet);
        
        break;

      case mwnaCProc:
      
        DstN->CRes.Procs = SrcN->CRes.Procs;
        
        break;

      case mwnaCPULoad:
      
        DstN->Load = SrcN->Load;
        
        break;

      case mwnaCRes:      /* configured generic resources */

        { 
        int gindex;

        MSNLCopy(&DstN->CRes.GenericRes,&SrcN->CRes.GenericRes);

        for (gindex = 1;gindex < MSched.M[mxoxGRes];gindex++)
          {
          if (MGRes.Name[gindex][0] == '\0')
            break;

          if ((R != NULL) && bmisset(&R->RTypes,mrmrtLicense))
            {
            /* update license rm stats */

            R->U.Nat.CurrentUResCount++;
            R->U.Nat.CurrentResConfigCount  += MSNLGetIndexCount(&SrcN->CRes.GenericRes,gindex);

            MSNLSetCount(&R->U.Nat.ResConfigCount,gindex,MSNLGetIndexCount(&R->U.Nat.ResConfigCount,gindex) + MSNLGetIndexCount(&SrcN->CRes.GenericRes,gindex));
            }
          } 
        } /* END mwnaCRES block */

        break;

      case mwnaCSwap:
       
        tmpInt = SrcN->CRes.Swap;
        MNodeSetAttr(DstN,mnaRCSwap,(void **)&tmpInt,mdfInt,mSet);

        break;
        
      case mwnaCurrentTask:
        
        /* NO-OP */

        break;
        
      case mwnaFeature:
        
        bmor(&DstN->FBM,&SrcN->FBM);

        break;

      case mwnaGEvent:    /* generic event */

        /* NYI */

        break;

      case mwnaGMetric:   /* generic metric */
        
        {
        int    gmindex;
        double DVal;
        char   strVal[MMAX_NAME];

        /* FORMAT:  GMETRIC[temp]=113.2 */

        if (MNodeInitXLoad(DstN) == FAILURE)
          {
          /* cannot allocate memory */

          break;
          }

        for (gmindex = 1;gmindex < MSched.M[mxoxGMetric];gmindex++)
          {
          DVal = SrcN->XLoad->GMetric[gmindex]->IntervalLoad;

          if (DVal != MDEF_GMETRIC_VALUE)
            {
            DstN->XLoad->GMetric[gmindex]->IntervalLoad = DVal;
            DstN->XLoad->GMetric[gmindex]->SampleCount++;
            }

          strVal[0] = '\0';

          sprintf(strVal,"%f",
            DVal);

          /* check if gmetric has event handler (NYI) */

          if ((MGMetric.GMetricThreshold[gmindex] != 0.0) &&
              (MUDCompare(
                DVal,
                MGMetric.GMetricCmp[gmindex],
                MGMetric.GMetricThreshold[gmindex])))
            {
            MGEventProcessGEvent(
              gmindex,
              mxoNode,
              DstN->Name,
              NULL,
              MGMetric.GMetricThreshold[gmindex],
              mrelGEvent,
              strVal);
            }
          }
        }  /* END BLOCK (case mwnaGMetric) */

        break;

      case mwnaIdleTime:  /* duration since last local mouse or keyboard activity */
        
        DstN->KbdIdleTime = SrcN->KbdIdleTime;
        
        break;
        
      case mwnaMaxTask:
        
        /* NYI */
        
        break;

      case mwnaMessage:
        
        /* NYI */

        break;
        
      case mwnaName:
        
        /* NYI */

        break;
        
      case mwnaOSList:
        
        {
        int osindex;
        char osList[MMAX_NAME];

        osList[0] = '\0';

        /* Create oslist string (linux,osx,vista) for MNodeSetAttr */

        for (osindex = 0;SrcN->OSList[osindex].AIndex > 0;osindex++)
          {
          if (osindex != 0) /* separate OS's with comma */
            MUStrCat(osList,",",MMAX_NAME);

          MUStrCat(osList,MAList[meOpsys][SrcN->OSList[osindex].AIndex],MMAX_NAME);
          }

        MNodeSetAttr(DstN,mnaOSList,(void **)osList,mdfString,mSet);

        if (DstN->ActiveOS == 0)
          {
          if ((DstN->OSList != NULL) && (DstN->OSList[1].AIndex <= 0))
            MNodeSetAttr(DstN,mnaOS,(void **)MAList[meOpsys][DstN->OSList[0].AIndex],mdfString,mSet);
          else
            MNodeSetAttr(DstN,mnaOS,(void **)ANY,mdfString,mSet);
          }
        } /* END mwnaOSList block */

        break;

      case mwnaOS:

        DstN->ActiveOS = SrcN->ActiveOS;

        break;

      case mwnaOther:

        /* NYI */

        break;

      case mwnaPartition:

        MParAddNode(&MPar[SrcN->PtIndex],DstN);

        break;

      case mwnaPowerIsEnabled:
        
        /* NYI */

        break;

      case mwnaRack:

        MRackAddNode(&MRack[SrcN->RackIndex],DstN,-1);

        break;

      case mwnaSlot:
        
        DstN->SlotIndex = SrcN->SlotIndex;

        break;

      case mwnaSpeed:
        
        DstN->Speed = SrcN->Speed;

        break;
        
      case mwnaState:

        {
        enum MNodeStateEnum OldState   = DstN->State;
        enum MNodeStateEnum OldRMState = mnsNONE;
        enum MNodeStateEnum NState     = mnsNone;

        int  RIndex;

        if (R != NULL)
          NState = SrcN->RMState[R->Index];

        /* set substate, if one exists */
        /* NOTE:  substate only supported w/one RM per node (FIXME) */

        MUStrDup(&DstN->SubState,SrcN->SubState);

        if ((R != NULL) && (MRMIgnoreNodeStateInfo(R) == TRUE))
          {
          /* only set RM-specific node state and exit */

          if (bmisset(&R->RTypes,mrmrtProvisioning))
            {
            mjob_t *J;

            MDB(7,fRM) MLog("INFO:     state '%s' reported for node '%s' by prov rm '%s'\n",
              MNodeState[NState],
              DstN->Name,
              R->Name);
  
            if ((NState == mnsDown) && (MNodeGetSysJob(DstN,msjtOSProvision,MBNOTSET,&J) == SUCCESS))
              {
              if (MJOBISACTIVE(J) == TRUE)
                {
                /* if provisioning RM, and provisioning job is active, mask state to mnsRunning */

                NState = mnsActive;
                }
              } 
            }

          if ((bmisset(&R->Flags,mrmfNoCreateResource)) || 
              (DstN->RM != NULL))
            {
            DstN->RMState[R->Index] = NState;
            }

          break;  
          }  /* END if ((R != NULL) && (MRMIgnoreNodeStateInfo(R) == TRUE)) */

        RIndex = -1;

        if (R != NULL)
          {
          /* slave RM's should not override info from primary RM */

          RIndex = R->Index;

          OldRMState = DstN->RMState[RIndex];

          if (bmisset(&R->RTypes,mrmrtProvisioning))
            {
            mjob_t *J;

            if ((NState == mnsDown) && 
                (MNodeGetSysJob(DstN,msjtOSProvision,MBNOTSET,&J) == SUCCESS))
              {
              if (MJOBISACTIVE(J) == TRUE)
                {
                /* if provisioning RM, and provisioning job is active, mask state 
                  to mnsRunning */

                NState = mnsActive;
                }
              } 
            }

          DstN->RMState[RIndex] = NState;
          }
        else if (DstN->RM != NULL)
          {
          /* NOTE:  only set N->RMState[N->RM->Index] to NState if R is not set */

          /* master RM will update self */

          RIndex = DstN->RM->Index;

          OldRMState = DstN->RMState[RIndex];

          /* NOTE:  if state is mnsUnknown, should we update both N->State and 
                    N->RMState values when actual state is determined in 
                    MNodeAdjustAvailResources()? */

          DstN->RMState[RIndex] = NState;

          DstN->State = NState;
          }
        else
          {
          OldRMState = DstN->State;

          DstN->State = NState;
          }

        if ((OldState != mnsNONE) && 
            (DstN->State != OldState) &&
            (R != NULL) &&
            !bmisset(&R->Flags,mrmfNoCreateResource) &&
          ((OldRMState == mnsNONE) || (OldRMState != DstN->RMState[R->Index])))
          {
          /* if per-RM node state is maintained, only report updated state if 
            effective node state has changed AND reported RM node state has 
            changed */

          MDB(2,fRM) MLog("INFO:     node '%s' changed states from %s to %s\n",
            DstN->Name,
            MNodeState[OldState],
            MNodeState[DstN->State]);
    
          MSched.EnvChanged = TRUE;
      
          DstN->StateMTime = MSched.Time;

          /* handle <ANYSTATE> to 'Down/Drain/Flush' state transitions */

          if ((DstN->State == mnsDown) ||
              (DstN->State == mnsDraining) ||
              (DstN->State == mnsDrained) ||
              (DstN->State == mnsFlush))
            {
            DstN->EState       = DstN->State;
            DstN->SyncDeadLine = MMAX_TIME;

            /* only report event for single node, not default */

  /*
            MOReportEvent((void *)&MSched.DefaultN,
              MSched.DefaultN.Name,
              mxoNode,
              mttFailure,
              MSched.Time,
              TRUE);
  */

            MOReportEvent((void *)DstN,
              DstN->Name,
              mxoNode,
              mttFailure,
              MSched.Time,
              TRUE);
            }  /* END if ((DstN->State == mnsDown) || ...) */

          /* handle Down/Drain/Flush state to <any state> transitions */

          if ((OldState == mnsDown) ||
              (OldState == mnsDraining) ||
              (OldState == mnsDrained) ||
              (OldState == mnsFlush) ||
              (OldState == mnsReserved))
            {
            DstN->EState       = DstN->State;
            DstN->SyncDeadLine = MMAX_TIME;
            }

          if ((OldState == mnsIdle) && (DstN->State == mnsBusy))
            {
            if (MSched.Mode != msmMonitor)
              {
              MDB(1,fRM) MLog("ALERT:    unexpected node transition on node '%s'  Idle -> Busy\n",
                DstN->Name);
              }

            DstN->EState       = DstN->State;
            DstN->SyncDeadLine = MMAX_TIME;
            }
          }   /* END if (N->State != OldState) */
        }     /* END BLOCK (case mwnaState) */

        break;

      case mwnaTransferRate:
      case mwnaType:
        
        /* NYI */

        break;

      case mwnaUpdateTime:
      
        DstN->ATime = SrcN->ATime;
        
        break;
        
      case mwnaVarAttr:
        
        /* NYI */

        break;

      case mwnaVariables:

        MUHTCopy(&DstN->Variables,&SrcN->Variables);
        
        break;

      default:

        /* NOT HANDLED */

        /* NO-OP */

        break;
      }  /* switch (AList[AIndex]) */
    }    /* END for (AIndex) */

  return(SUCCESS);
  }  /* END MWikiNodeApplyAttr() */


/* END MWikiNode.c */
