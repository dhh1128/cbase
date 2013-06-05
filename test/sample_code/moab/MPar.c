/* HEADER */

/**
 * @file MPar.c
 *
 * Moab Partitions
 */

/* Contains:                                          
    int MJobGetPAL(X,Y,Z,A,B)                         
    int MParFind(PName,PP)                            
    int MParAdd(PName,PP)                             
    int MParSetDefaults(P)                            
    int MParListBMFromString(PString,BM,Mode)         
    int MParShow(PName,Buffer,BufSize,DFormat,IFlags,ShowDiag) 
    int MParGetTC(P,Avl,Cfg,Ded,Req,Time)             
    char *MParBMToString(BM,Delim,Line)
    int MParProcessOConfig(P,PIndex,IVal,DVal,SVal,SArray) 
    int MParProcessNAvailPolicy() 
    int MParUpdate() 
                                                      */

using namespace std;

#include "moab.h"
#include "moab-proto.h"  
#include "moab-global.h"
#include "moab-const.h"
#include "PluginNodeAllocate.h"
#include "PluginManager.h"


int __MParSortNodeSetListQSortFunc(char **, char **);
void MParSortNodeSetList(mpar_t *);


/**
 * Loads the Partition's node allocation plugin. 
 *
 * Will destroy previously loaded plugin on successfull load.
 *
 * @param Partition (I) The partition to load the plugin on.
 * @param PluginName (I) The name of the libary file -- relative or absolute.
 */

int __MParLoadNodeAllocationPlugin(

  mpar_t     *Partition,
  const char *PluginName)

  {
  MASSERT(Partition != NULL,"null partition when loading node allocation plugin on partition");
  MASSERT(PluginName != NULL,"null plugin name when loading node allocation plugin on partition");
  MASSERT(PluginName[0] != '\0',"null plugin name when loading node allocation plugin on partition");

  try 
    {
    PluginNodeAllocate *tmpPlugin;
    tmpPlugin = PluginManager::CreateNodeAllocationPlugin(Partition->Name,PluginName);

    if (Partition->NodeAllocationPlugin != NULL)
      {
      delete Partition->NodeAllocationPlugin;
      Partition->NodeAllocationPlugin = NULL;
      }

    Partition->NodeAllocationPlugin = tmpPlugin;
    }
  catch (exception& e) 
    {
    string errMsg  = "ERROR:    Error loading node allocation plugin '";
           errMsg += PluginName;
           errMsg += "' for partition '";
           errMsg += Partition->Name;
           errMsg += "', '";
           errMsg += e.what();
           errMsg += "' - will perform normal Moab node allocation";
    MDB(1,fSTRUCT) MLog("%s\n",errMsg.c_str());
    MMBAdd(&MSched.MB,errMsg.c_str(),NULL,mmbtNONE,0,0,NULL);

    return(FAILURE);
    }
  
  string successMsg = string("INFO:     Successfully loaded 'NodeAllocation' plugin '") + 
                              Partition->NodeAllocationPlugin->Name() + "' for partition '" + 
                              Partition->Name + "'\n";
  MDB(1,fSTRUCT) MLog(successMsg.c_str());
                             
  return(SUCCESS);
  } /* __MParLoadNodeAllocationPlugin() */



/**
 * Sets the Partition's node allocatino policy.
 *
 * @param Partition (I) The partition to set.
 * @param Policy The string representing the node allocation policy.
 */

int __MParSetNodeAllocationPolicy(

  mpar_t *Partition,
  char   *Policy)

  {
  MASSERT(Partition != NULL,"null partition when setting partition's node allocation policy");
  MASSERT(Policy != NULL,"null policy when setting partition's node allocation policy");
  MASSERT(Policy[0] != '\0',"empty policy when setting partition's node allocation policy");

  char *allocationPolicy = NULL;
  char *policyValue = NULL;
  enum MNodeAllocPolicyEnum oldPolicy;

  allocationPolicy = MUStrTok(Policy,":",&policyValue);

  oldPolicy = Partition->NAllocPolicy;

  Partition->NAllocPolicy = (enum MNodeAllocPolicyEnum)MUGetIndexCI(
    allocationPolicy,
    MNAllocPolicy,
    FALSE,
    oldPolicy);

  /* convert aliased algorithms to old ones to prevent Moab
   * from showing the new names in showconfig */
  switch (Partition->NAllocPolicy)
    {
    case mnalInReportedOrder:
      Partition->NAllocPolicy = mnalFirstAvailable;
      break;
    case mnalInReverseReportedOrder:
      Partition->NAllocPolicy = mnalLastAvailable;
      break;
    case mnalCustomPriority:
      Partition->NAllocPolicy = mnalMachinePrio;
      break;
    case mnalProcessorLoad:
      Partition->NAllocPolicy = mnalCPULoad;
      break;
    case mnalMinimumConfiguredResources:
      Partition->NAllocPolicy = mnalMinResource;
      break;
    case mnalProcessorSpeedBalance:
      Partition->NAllocPolicy = mnalMaxBalance;
      break;
    case mnalNodeSpeed:
      Partition->NAllocPolicy = mnalFastest;
      break;
    default:
      /* NO-OP */
      break;
    }

  if ((oldPolicy == mnalPlugin) && 
      (Partition->NAllocPolicy != oldPolicy) &&
      (Partition->NodeAllocationPlugin != NULL))
    {
    delete Partition->NodeAllocationPlugin;
    Partition->NodeAllocationPlugin = NULL;
    }

  /* load plugin */
  if (Partition->NAllocPolicy == mnalPlugin)
    {
    if (__MParLoadNodeAllocationPlugin(Partition,policyValue) == FAILURE)
      {
      Partition->NAllocPolicy = oldPolicy;
      return(FAILURE);
      }
    }

  return(SUCCESS);
  } /* END __MParSetNodeAllocationPolicy() */

/**
 * Locate partition with specified name.
 *
 * @see MParAdd() - peer
 *
 * @param PName (I)
 * @param PP (O)
 */

int MParFind(
 
  const char  *PName,
  mpar_t     **PP)
 
  {
  int pindex;

  mpar_t *P;

  if (PP != NULL)
    *PP = NULL;
 
  if (MUStrIsEmpty(PName))
    {
    return(FAILURE);
    }
 
  for (pindex = 0;pindex < MMAX_PAR;pindex++)
    {
    P = &MPar[pindex];

    if (P->Name[0] == '\0')
      {
      return(FAILURE);
      }

    if (P->Name[0] == '\1')
      continue;
 
    if (strcmp(P->Name,PName) != 0)
      continue;
 
    /* partition found */

    if (PP != NULL)
      *PP = P;
 
    return(SUCCESS);
    }  /* END for (pindex) */ 
 
  return(FAILURE);
  }  /* END MParFind() */





/**
 * Add new partition
 *
 * @see MParFind() - peer
 *
 * @param PName (I)
 * @param PP (O) [optional]
 */

int MParAdd(
 
  const char  *PName,  /* I */
  mpar_t     **PP)     /* I */
 
  {
  int pindex;

  mpar_t *P;

  const char *FName = "MParAdd";
 
  MDB(4,fSTRUCT) MLog("%s(%s,PP)\n",
    FName,
    (PName != NULL) ? PName : "NULL");
 
  if (MUStrIsEmpty(PName))
    {
    return(FAILURE);
    }

  if (MParFind(PName,&P) == SUCCESS)
    {
    /* partition already exists */

    if (PP != NULL)
      *PP = P;

    return(SUCCESS);
    }

  if (!strcmp(PName,MCONST_GLOBALPARNAME))
    {
    P = &MPar[0];

    pindex = 0;
    }
  else 
    {
    /* skip ALL partition */

    for (pindex = 1;pindex < MMAX_PAR;pindex++)
      {
      P = &MPar[pindex];

      if (P->Name[0] <= '\1')
        {
        break;
        }
      }    /* END for (pindex) */

    if (pindex >= MMAX_PAR)
      {
      /* partition overflow */

      if (MSched.OFOID[mxoPar] == NULL)
        MUStrDup(&MSched.OFOID[mxoPar],PName);

      return(FAILURE);
      }

    if ((MSched.SharedPtIndex < 0) && !strcasecmp(PName,"shared"))
      {
      MSched.SharedPtIndex = pindex;
      }
    }    /* END else (!strcmp(PName,MCONST_GLOBALPARNAME)) */
   
  /* available partition slot located */

  MUFree((char **)&P->L.IdlePolicy);
  MUFree((char **)&P->L.JobPolicy);  

  if (P->S.IStat != NULL)
    {
    int scount;

    if (P->S.IStat[0] != NULL)
      scount = P->S.IStatCount;
    else
      scount = MStat.P.MaxIStatCount;

    MStatPDestroy((void ***)&P->S.IStat,msoCred,scount);
    }
 
  memset(P,0,sizeof(mpar_t));
 
  P->Index = pindex;
 
  MUStrCpy(P->Name,PName,sizeof(P->Name));

  MPUCreate(&P->L.IdlePolicy);
  MPUCreate(&P->L.JobPolicy);

  MPUInitialize(&P->L.ActivePolicy);

  MSNLInit(&P->CRes.GenericRes);
  MSNLInit(&P->UpRes.GenericRes);
  MSNLInit(&P->ARes.GenericRes);
  MSNLInit(&P->DRes.GenericRes);

  P->S.XLoad = (double *)MUCalloc(1,sizeof(double) * MSched.M[mxoxGMetric]);

  P->S.OType = mxoPar;
  P->S.Index = -1;

  P->VPCMasterRsv = NULL;

  P->QOSBucket = (mrsv_bucket_t *)MUCalloc(1,sizeof(mrsv_bucket_t) * MMAX_QOS);

  MParSetDefaults(P);

  MParLoadConfig(P,NULL);

  if (PP != NULL)
    *PP = P;
 
  return(SUCCESS);
  }  /* END MParAdd() */



/**
 * Set partition attributes to default values.
 *
 * @see MParAdd() - peer
 *
 * @param P (I) [modified]
 */

int MParSetDefaults(

  mpar_t *P)  /* I (modified) */

  {
  int index;

  mfsc_t *F;

  const char *FName = "MParSetDefaults";

  MDB(6,fSTRUCT) MLog("%s(%s)\n",
    FName,
    (P != NULL) ? P->Name : "NULL");

  if (P == NULL)
    {
    return(FAILURE);
    }

  P->BFPolicy             = MDEF_BFPOLICY;
  P->BFDepth              = MDEF_BFDEPTH;
  P->BFProcFactor         = MDEF_BFNODEFACTOR;
  P->BFMaxSchedules       = MDEF_BFMAXSCHEDULES;

  P->JobPrioAccrualPolicy = MDEF_JOBPRIOACCRUALPOLICY;

  P->VWConflictPolicy     = MDEF_VWCONFLICTPOLICY;

  P->NAvailPolicy[0]      = MDEF_RESOURCEAVAILPOLICY;

  for (index = 0;index < mrlLAST;index++)
    {
    P->ResourceLimitPolicy[MRESUTLHARD][index] = 
      MDEF_RESOURCELIMITPOLICY;

    bmset(
      &P->ResourceLimitViolationActionBM[MRESUTLHARD][index],
      MDEF_RESOURCELIMITVIOLATIONACTION);

    P->ResourceLimitMultiplier[index] = MDEF_RESOURCELIMITMULTIPLIER;
    }  /* END for (index) */

  P->UntrackedProcFactor  = MDEF_NODEUNTRACKEDPROCFACTOR;
  P->AdminMinSTime        = MDEF_MINSUSPENDTIME;
  P->JobMinSTime          = 0;

  /* system policies */
 
  P->MaxMetaTasks         = MDEF_MAXMETATASKS;

  P->NodeSetDelay         = MDEF_NSDELAY;
  P->NodeSetPolicy        = mrssNONE;
  P->NodeSetIsOptional    = MDEF_NSISOPTIONAL;
  P->NodeSetAttribute     = MDEF_NSATTRIBUTE;
  P->NodeSetPriorityType  = mrspFirstFit;
 
  P->MaxJobStartTime       = MMAX_JOBSTARTTIME;

  /* partition will inherit node allocation policy of global partition */

  /* NOTE:  this should be changed to a 'not-set' value allowing inheritance
            to dynamically follow global partition */

  /* Late binding of Nodeallocationpolicy. Will be determined by configuration
   * or MJobAdjustNAPolicy in MJobAllocMNL. */
  P->NAllocPolicy         = mnalNONE;

  P->ARFPolicy            = marfNONE;
  P->DistPolicy           = MDEF_TASKDISTRIBUTIONPOLICY;
  P->BFMetric             = MDEF_BFMETRIC;
  P->JobRetryTime         = MDEF_JOBRETRYTIME;
 
  P->RsvRetryTime         = MDEF_RSVRETRYTIME;
  P->RsvTType             = MDEF_RSVTHRESHOLDTYPE;
  P->RsvTValue            = MDEF_RSVTHRESHOLDVALUE;

  P->MigrateTime = MSched.Time;
  P->QTWeight = 1.0;
  P->NPWeight = 1.0;
  P->ATWeight = 0.0;
 
  if (P->QOSBucket != NULL)
    {
    P->QOSBucket[0].RsvDepth = MDEF_RSVDEPTH;
    P->QOSBucket[1].RsvDepth = 0;
 
    P->QOSBucket[0].List[0] = MDEF_RSVQOSLIST;
    P->QOSBucket[0].List[1] = NULL;

    strcpy(P->QOSBucket[0].Name,"DEFAULT");

    for (index = 1;index < MMAX_QOS;index++)
      {
      P->QOSBucket[index].List[0] = NULL;
      P->QOSBucket[index].RsvDepth = 0;
      P->QOSBucket[index].Name[0]  = '\0';
      }  /* END for (index) */
    }
 
  P->NodeBusyStateDelayTime  = MDEF_NODEBUSYSTATEDELAYTIME;
  P->NodeDownStateDelayTime  = MDEF_NODEDOWNSTATEDELAYTIME;
  P->NodeDrainStateDelayTime = MDEF_NODEDRAINSTATEDELAYTIME;
 
  bmsetall(&P->F.PAL,MMAX_PAR);

  P->F.PDef               = &MPar[1];  /* NOTE:  default partition */

  P->F.QDef[0]            = &MQOS[MDEF_SYSQDEF];
 
  bmclear(&P->F.QAL);
 
  bmclear(&P->F.JobFlags);

  F = &P->FSC;

  if (P->Index != 0)
    {
    /* '-1' indicates use global priorities */

    for (index = 0;index < mpcLAST;index++)
      {
      F->PCW[index] = MCONST_PRIO_NOTSET;
      }  /* END for (index) */

    for (index = 0;index < mpsLAST;index++)
      {
      F->PSW[index] = MCONST_PRIO_NOTSET;
      }  /* END for (index) */

    P->RsvPolicy = mrpDefault;
    }
  else
    {
    P->RsvPolicy = MDEF_RSVPOLICY;

    for (index = 0;index < mpcLAST;index++)
      {
      F->PCW[index] = 1;
      }  /* END for (index) */

    for (index = 0;index < mpsLAST;index++)
      {
      F->PSW[index] = 0;
      }  /* END for (index) */

    /* enable service queue time, usage exetime, and cred and fs priority tracking only */

    F->PSW[mpsSQT] = 1;

    F->PSW[mpsCU]  = 1;
    F->PSW[mpsCG]  = 1;
    F->PSW[mpsCA]  = 1;
    F->PSW[mpsCQ]  = 1;
    F->PSW[mpsCC]  = 1;

    F->PSW[mpsFU]  = 1000;
    F->PSW[mpsFG]  = 1000;
    F->PSW[mpsFA]  = 1000;
    F->PSW[mpsFQ]  = 1000;
    F->PSW[mpsFC]  = 1000;

    F->XFMinWCLimit  = MDEF_XFMINWCLIMIT;
    F->FSPolicy      = MDEF_FSPOLICY;
    F->FSInterval    = MDEF_FSINTERVAL;
    F->FSDepth       = MDEF_FSDEPTH;
    F->FSDecay       = MDEF_FSDECAY;
    }  /* END else */

  return(SUCCESS);
  }  /* END MParSetDefaults() */





/**
 * Convert comma-delimited string into partition bitmap.
 *
 * @param PString (I)
 * @param BM (O)
 * @param Mode (I) [0 or mAdd]
 */

int MPALFromRangeString(
 
  char           *PString,  /* I */
  mbitmap_t      *BM,       /* O */
  int             Mode)     /* I (0 or mAdd) */
 
  {
  int   rangestart;
  int   rangeend;
 
  int   rindex;
 
  char  tmpLine[MMAX_LINE];
 
  char *rtok;
  char *tail;
 
  char  tmpName[MMAX_NAME];
 
  char *TokPtr;
 
  mpar_t  *P;
 
  if (BM == NULL)
    {
    return(FAILURE);
    }
 
  bmclear(BM);
 
  if (PString == NULL)
    {
    return(SUCCESS);
    }
 
  MUStrCpy(tmpLine,PString,sizeof(tmpLine));
 
  /* FORMAT:  PSTRING:   <RANGE>[:<RANGE>]... */
  /*          RANGE:     <VALUE>[-<VALUE>]    */
 
  /* NOTE:    The following non-numeric values may appear in the string */
  /*          an should be handled: '&', '^'                            */
 
  rtok = MUStrTok(tmpLine,"[],:",&TokPtr);
 
  while (rtok != NULL)
    {
    while ((*rtok == '&') || (*rtok == '^'))
      rtok++; 
 
    rangestart = strtol(rtok,&tail,10);
 
    if ((rangestart != 0) || (rtok[0] == '0'))
      {
      if (*tail == '-')
        rangeend = strtol(tail + 1,&tail,10);
      else
        rangeend = rangestart;
 
      rangeend = MIN(rangeend,31);
 
      for (rindex = rangestart;rindex <= rangeend;rindex++)
        {
        sprintf(tmpName,"%d",
           rindex);
 
        if (MParFind(tmpName,&P) == SUCCESS)
          {
          bmset(BM,P->Index);
          }
        else if ((Mode == mAdd) && (P != NULL))
          {
          MParAdd(tmpName,&P);
 
          bmset(BM,P->Index);
          }
        }    /* END for (rindex) */
      }
    else
      {
      /* partition name provided */
 
      /* remove meta characters (handled externally) */
 
      if ((tail = strchr(rtok,'&')) != NULL)
        *tail = '\0';
      else if ((tail = strchr(rtok,'^')) != NULL)
        *tail = '\0';
 
      if (MParFind(rtok,&P) == SUCCESS)
        {
        bmset(BM,P->Index); 
        }
      else if ((Mode == mAdd) && (P != NULL))
        {
        MParAdd(rtok,&P);
 
        bmset(BM,P->Index);
        }
      }
 
    rtok = MUStrTok(NULL,"[],:",&TokPtr);
    }  /* END while (rtok) */
 
  return(SUCCESS);
  }  /* END MParListBMFromString() */





/**
 * Report partition state and diagnostics (for 'mdiag -t').
 *
 * @see MUIParDiagnose() - parent
 *
 * @param PName (I) [optional]
 * @param RE (O)
 * @param String (O)
 * @param DFormat (I)
 * @param IFlags (I) [bitmap of enum MClientModeEnum]
 * @param ShowDiag (I)
 */

int MParShow(
 
  char                 *PName,
  mxml_t               *RE,
  mstring_t            *String,
  enum MFormatModeEnum  DFormat,
  mbitmap_t            *IFlags,
  mbool_t               ShowDiag)
 
  {
  int pindex;
  int rindex; 
  int index;
  int aindex;

  int URes;
  int ARes;
  int DRes;
  int CRes;

  mbool_t HeaderDisplayed;
  
  mgcred_t  *U;
  mgcred_t  *G;
  mgcred_t  *A;
  mclass_t  *C;
  mqos_t    *Q;

  mpar_t    *P = NULL;

  mrm_t     *R = NULL;
 
  mxml_t    *DE = NULL;  /* set to avoid compiler warning */
  mxml_t    *PE[MMAX_PAR] = {NULL};
 
  char      Line[MMAX_BUFFER];
  char      tmpLine[MMAX_LINE];
 
  char      PList[MMAX_LINE];
  char      PrioLine[MMAX_NAME];

  const char *Pad = "----------------------------------------------------------------------------";

  int       PCount;

  mbool_t   MessageDetected = FALSE;
  mbool_t   ShowGlobal      = TRUE;

  mhashiter_t Iter;

  const enum MParAttrEnum XMLPAList[] = {
    mpaArch,
    mpaBacklogPS,
    mpaBacklogDuration,
    mpaDefaultNodeActiveWatts,
    mpaDefaultNodeIdleWatts,
    mpaDefaultNodeStandbyWatts,
    mpaGreenBacklogThreshold,
    mpaGreenQTThreshold,
    mpaGreenXFactorThreshold,
    mpaID,
    mpaIsRemote,
    mpaJobCount,
    mpaMaxCPULimit,
    mpaMaxNode,
    mpaMaxProc,
    mpaMaxPS,
    mpaMaxWCLimit,
    mpaMessages,
    mpaMinNode,
    mpaMinProc,
    mpaMinWCLimit,
    mpaNodePowerOffDuration,
    mpaNodePowerOnDuration,
    mpaOS,
    mpaRM,    
    mpaRMType,
    mpaState,
    mpaUpdateTime,
    mpaVMCreationDuration,
    mpaVMDeletionDuration,
    mpaVMMigrationDuration,
    mpaNONE };

  const enum MParAttrEnum HumanPAList[] = {
    mpaArch,
    mpaAllocResFailPolicy,
    mpaIsRemote,
    mpaMaxCPULimit,
    mpaMaxNode,
    mpaMaxProc,
    mpaMaxPS,
    mpaMaxWCLimit,
    mpaMinNode,
    mpaMinProc,
    mpaMinWCLimit,
    mpaNodeAccessPolicy,
    mpaOS,
    mpaRM,
    mpaRMType,
    mpaUpdateTime,
    mpaUseTTC,
    mpaNONE };

  const char *FName = "MParShow";
 
  MDB(2,fUI) MLog("%s(%s,Buffer,BufSize,%s,%s)\n",
    FName,
    (PName != NULL) ? PName : "NULL",
    MFormatMode[DFormat],
    MBool[ShowDiag]);

  /* initialize */

  if (DFormat == mfmXML)
    {
    DE = RE;

    if (DE == NULL)
      {
      return(FAILURE);
      }
    }
  else
    {
    if (String == NULL)
      {
      return(FAILURE);
      }

    MStringSet(String,"\0");
    }

  PCount = 0;

  for (pindex = 1;pindex < MSched.M[mxoPar];pindex++)
    {
    P = &MPar[pindex];

    if (P->Name[0] == '\0')
      break;

    if (P->Name[0] == '\1')
      continue;

    if (P->ConfigNodes > 0)
      PCount++;
    }  /* END for (pindex) */

  if (PCount > 1)
    ShowGlobal = TRUE;

  /* create header */

  if (DFormat != mfmXML)
    {
    MStringAppendF(String,"Partition Status\n\n");
 
    MPALToString(&MPar[0].F.PAL,",",PList);

    MStringAppendF(String,"System Partition Settings:  PList: %s\n\n",
      PList);

    MStringAppendF(String,"%-20s %8s        %16.16s %18.18s\n\n",
      "Name",
      "Procs",
      "NodeAccessPolicy",
      "JobNodeMatchPolicy");
    }

  /* display partition attributes */
 
  for (pindex = 0;pindex < MSched.M[mxoPar];pindex++)
    {
    mhashiter_t HIter;

    P = &MPar[pindex];
 
    if (P->Name[0] == '\0')
      break;

    if (P->Name[0] == '\1')
      continue;

    if ((pindex == 0) && (ShowGlobal == FALSE))
      {
      /* ignore global partition if only one partition exists */

      continue;
      }

    if (!MUStrIsEmpty(PName) && strcmp(PName,P->Name))
      {
      continue;
      }

    tmpLine[0] = '\0';

    mstring_t NodeAccess(MMAX_LINE);
    mstring_t NodeMatch(MMAX_LINE);

    MParAToMString(P,mpaNodeAccessPolicy,&NodeAccess,0);
    MParAToMString(P,mpaJobNodeMatchPolicy,&NodeMatch,0);
 
    if (DFormat == mfmXML)
      {
      PE[pindex] = NULL;

      MXMLCreateE(&PE[pindex],(char *)MXO[mxoPar]);

      MParToXML(P,PE[pindex],(enum MParAttrEnum *)XMLPAList);

      if (!MUStrIsEmpty(NodeAccess.c_str()))
        MXMLSetAttr(PE[pindex],"NodeAccessPolicy",(void *)NodeAccess.c_str(),mdfString);

      if (!MUStrIsEmpty(NodeMatch.c_str()))
        MXMLSetAttr(PE[pindex],"JobNodeMatchPolicy",(void *)NodeMatch.c_str(),mdfString);

      MXMLAddE(DE,PE[pindex]);

      continue;
      }
    
    /* display human-readable access and config */

    tmpLine[0] = '\0';
    MParAToMString(P,mpaNodeAccessPolicy,&NodeAccess,0);
    MParAToMString(P,mpaJobNodeMatchPolicy,&NodeMatch,0);

    if (P->Priority != 0)
      {
      sprintf(PrioLine,"  (priority=%ld)",
        P->Priority);
      }
    else
      {
      PrioLine[0] = '\0';
      }

    MStringAppendF(String,"%-20s %8d        %16.16s %18.18s%s\n",
      (P->Name[0] != '\0') ? P->Name : NONE,
      P->CRes.Procs, 
      (!MUStrIsEmpty(NodeAccess.c_str())) ? NodeAccess.c_str() : "---",
      (!MUStrIsEmpty(NodeMatch.c_str())) ? NodeMatch.c_str() : "---",
      PrioLine);

    /* check user access */
 
    mstring_t CredLine(MMAX_BUFFER);;

    MUHTIterInit(&HIter);
    while (MUHTIterate(&MUserHT,NULL,(void **)&U,NULL,&HIter) == SUCCESS)	
      {
      if (bmisset(&U->F.PAL,pindex))
        {
        MStringAppendF(&CredLine,"%s%s",
          (CredLine.empty()) ? "" : ",",
          U->Name);
        }
      }  /* END for (index) */ 
 
    if (!CredLine.empty())
      {
      MStringAppendF(String,"  ULIST=%s\n",
        CredLine.c_str());
      }

    /* check group access */
 
    CredLine.clear();  /* reset string */

    MUHTIterInit(&Iter);
    while (MUHTIterate(&MGroupHT,NULL,(void **)&G,NULL,&Iter) == SUCCESS)
      {
      if (bmisset(&G->F.PAL,pindex))
        {
        MStringAppendF(&CredLine,"%s%s",
          (CredLine.empty()) ? "" : ",",
          G->Name);
        }
      }  /* END for (index) */
 
    if (!CredLine.empty())
      {
      MStringAppendF(String,"  GLIST=%s\n",
        CredLine.c_str());
      }
 
    /* check account access */
 
    CredLine.clear();  /* reset string */
 
    MUHTIterInit(&Iter);
    while (MUHTIterate(&MAcctHT,NULL,(void **)&A,NULL,&Iter) == SUCCESS)
      {
      if (bmisset(&A->F.PAL,pindex))
        {
         MStringAppendF(&CredLine,"%s%s",
          (CredLine.empty()) ? "" : ",",
          A->Name);
        }
      }  /* END for (index) */
 
    if (!CredLine.empty())
      {
      MStringAppendF(String,"  ALIST=%s\n",
        CredLine.c_str());
      }

    /* check class access */

    CredLine.clear();  /* Reset string */

    for (index = MCLASS_FIRST_INDEX;index < MMAX_CLASS;index++)
      {
      C = &MClass[index];

      if ((C->Name[0] == '\0') || (C->Name[0] == '\1'))
        continue;

      if (bmisset(&C->F.PAL,pindex))
        {
        MStringAppendF(&CredLine,"%s%s",
          (CredLine.empty()) ? "" : ",",
          C->Name);
        }
      }  /* END for (index) */

    if (!CredLine.empty())
      {
      MStringAppendF(String,"  CLIST=%s\n",
        CredLine.c_str());
      }

    /* check QOS access */

    CredLine.clear();  /* Reset string */

    for (index = 1;index < MMAX_QOS;index++)
      {
      Q = &MQOS[index];

      if ((Q == NULL) || (Q->Name[0] == '\0') || (Q->Name[0] == '\1'))
        continue;

      if (bmisset(&Q->F.PAL,pindex))
        {
        MStringAppendF(&CredLine,"%s%s",
          (CredLine.empty()) ? "" : ",",
          Q->Name);
        }
      }  /* END for (index) */

    if (!CredLine.empty())
      {
      MStringAppendF(String,"  QLIST=%s\n",
        CredLine.c_str());
      }

    if (P->Message != NULL)
      {
      MStringAppendF(String,"  MESSAGE=%s\n",
        P->Message);
      }  /* END if (P->Message) */

    /* Better suited for verbose display */

    /* display dedicated suspend resources */

    if (bmisset(IFlags,mcmVerbose))
      {
      mstring_t tmp(MMAX_LINE);

      for (aindex = 0;HumanPAList[aindex] != mpaNONE;aindex++)
        {
        MStringSet(&tmp,"\0");

        if (MParAToMString(P,HumanPAList[aindex],&tmp,0) == SUCCESS)
          {
          MStringAppendF(String,"  %s=%s",
            MParAttr[HumanPAList[aindex]],
            tmp.c_str());
          }
        }    /* END for (aindex) */

      MStringAppendF(String,"\n");
      }
    }      /* END for (pindex) */

  /* show resource state */

  if (DFormat != mfmXML)
    {
    /* display resource state header */

    MStringAppendF(String,"\n%-14s %10s %10s %7s %10s %7s %10s %7s\n\n",
      "Partition",
      "Configured",
      "Up",
      "U/C",
      "Dedicated",
      "D/U",
      "Active",
      "A/U");
    }

  /* display partition resources */

  for (rindex = mrNode;rindex <= mrDisk;rindex++)
    {
    HeaderDisplayed = FALSE;

    for (pindex = 0;pindex < MSched.M[mxoPar];pindex++)
      {
      P = &MPar[pindex];

      if (P->Name[0] == '\0')
        break;

      if (P->Name[0] == '\1')
        continue;
 
      if (P->ConfigNodes == 0)
        continue;

      if ((pindex == 0) && (ShowGlobal == FALSE))
        {
        /* ignore global partition if only one partition exists */

        continue;
        }
 
      if ((PName != NULL) && 
          (PName[0] != '\0') && 
           strcmp(PName,NONE) && 
           strcmp(PName,P->Name))
        {
        continue;
        }

      switch (rindex)
        {
        case mrProc:

          CRes = P->CRes.Procs;
          URes = P->UpRes.Procs;
          DRes = P->DRes.Procs;
          ARes = P->ARes.Procs;

          break;

        case mrMem:
         
          CRes = P->CRes.Mem;
          URes = P->UpRes.Mem;
          DRes = P->DRes.Mem;
          ARes = P->ARes.Mem;
 
          break;

        case mrSwap:

          CRes = P->CRes.Swap;
          URes = P->UpRes.Swap;
          DRes = P->DRes.Swap;
          ARes = P->ARes.Swap;

          break;

        case mrDisk:

          CRes = P->CRes.Disk;
          URes = P->UpRes.Disk;
          DRes = P->DRes.Disk;
          ARes = P->ARes.Disk;

          break;

        case mrNode:

        if (MSched.BluegeneRM == TRUE)
          {
          /* Show how many cnodes there are in a system, not how many midplanes.
          * 1 Midplane = 512 c-nodes. Moab sees 1 node as 512 procs. */

          CRes = P->CRes.Procs  / MSched.BGNodeCPUCnt;
          URes = P->UpRes.Procs / MSched.BGNodeCPUCnt;
          DRes = P->DRes.Procs  / MSched.BGNodeCPUCnt;
          ARes = URes - DRes;
          }
        else
          {
          CRes = P->ConfigNodes;
          URes = P->UpNodes;
          DRes = P->ActiveNodes;
          ARes = P->UpNodes - P->ActiveNodes;
          }

          break;

        default:

          CRes = 0;
          URes = 0;
          DRes = 0;
          ARes = 0;

          break;
        }  /* END switch (rindex) */

      if (CRes <= 0)
        continue;

      if (DFormat == mfmXML)
        {
        sprintf(Line,"%s.cfg",
          MResourceType[rindex]);

        MXMLSetAttr(PE[pindex],Line,(void *)&CRes,mdfInt);

        /* FIXME: URes is not "utilized" it is "up" */

        sprintf(Line,"%s.utl",
          MResourceType[rindex]);

        MXMLSetAttr(PE[pindex],Line,(void *)&URes,mdfInt);

        sprintf(Line,"%s.avl",
          MResourceType[rindex]);

        MXMLSetAttr(PE[pindex],Line,(void *)&ARes,mdfInt);

        sprintf(Line,"%s.ded",
          MResourceType[rindex]);

        MXMLSetAttr(PE[pindex],Line,(void *)&DRes,mdfInt);
        }
      else
        {
        if (HeaderDisplayed == FALSE)
          {
          HeaderDisplayed = TRUE;

          switch (rindex)
            {
            case mrNode:

              sprintf(tmpLine,"Nodes");

              break;

            case mrProc:

              sprintf(tmpLine,"Processors");

              break;

            case mrMem:

              sprintf(tmpLine,"Memory (in MB)");

              break;

            case mrSwap:

              sprintf(tmpLine,"Swap (in MB)");

              break;

            case mrDisk:

              sprintf(tmpLine,"Disk (in MB)");

              break;
       
            default:

              sprintf(tmpLine,"%s",
                MResourceType[rindex]);

              break;
            }  /* END switch (rindex) */

          sprintf(Line,"%s%s %.*s\n",
            (rindex == mrNode) ? "" : "\n",
            tmpLine,
            (int)(79 - strlen(tmpLine)),
            Pad);

          MStringAppend(String,Line);
          }  /* END if (HeaderDisplayed == FALSE) */

        MStringAppendF(String,"%-14.14s %10d %10d %6.2f%c %10d %6.2f%c %10d %6.2f%c\n",
          P->Name,
          CRes,
          URes,
          (double)((CRes > 0) ? URes * 100.0 / CRes : 0.0),
          '%',
          DRes,
          (double)((URes > 0) ? DRes * 100.0 / URes : 0.0),
          '%',
          (URes - ARes),
          (double)((URes > 0) ? (URes - ARes) * 100.0 / URes : 0.0),
          '%');
        }
      }  /* END for (pindex) */
    }    /* END for (rindex) */

  /* display class information */

  if (DFormat != mfmXML)
    {
    MStringAppend(String,"\nClasses/Queues\n\n");
 
    MStringAppendF(String,"%-14.14s [<CLASS>]...\n\n",
      " ");
    }

  for (pindex = 0;pindex < MSched.M[mxoPar];pindex++)
    {
    P = &MPar[pindex];

    if (P->Name[0] == '\0')
      break;

    if (P->Name[0] == '\1')
      continue;
 
    if (P->ConfigNodes == 0)
      continue;

    if ((pindex == 0) && (ShowGlobal == FALSE))
      {
      /* ignore global partition if only one partition exists */

      continue;
      }

    if (!bmisclear(&P->Classes))
      {
      mstring_t Classes(MMAX_LINE);

      MClassListToMString(&P->Classes,&Classes,NULL);

      MUStrCpy(Line,(Classes.empty()) ? NONE : Classes.c_str(),sizeof(Line));

      if (DFormat == mfmXML)
        MXMLSetAttr(PE[pindex],(char *)MParAttr[mpaClasses],(void *)Line,mdfString);
      }
    else
      {
      strcpy(Line,"---");
      }

    if (DFormat != mfmXML)
      {
      MStringAppendF(String,"%-14.14s %s\n",
        P->Name,
        Line);
      }
    }   /* END for (pindex) */

  /* display generic resource information */

  if (MGRes.Name[1][0] != '\0')
    {
    if (DFormat != mfmXML)
      {
      MStringAppend(String,"\nGeneric Resources\n\n");

      MStringAppendF(String,"%14.14s [<GRES> <AVAIL>:<CONFIG>]...\n\n",
        " ");
      }

    mstring_t GResStr(MMAX_LINE);

    for (pindex = 0;pindex < MSched.M[mxoPar];pindex++)
      {
      P = &MPar[pindex];

      if (P->Name[0] == '\0')
        break;

      if (P->Name[0] == '\1')
        continue;

      if (P->ConfigNodes == 0)
        continue;

      if ((pindex == 0) && (ShowGlobal == FALSE))
        {
        /* ignore global partition if only one partition exists */

        continue;
        }

      MStringSet(&GResStr,"\0");

      MSNLToMString(&P->ARes.GenericRes,&P->UpRes.GenericRes,NULL,&GResStr,meGRes);

      if (DFormat == mfmXML)
        {
        if (!MUStrIsEmpty(GResStr.c_str()))
          MXMLSetAttr(PE[pindex],(char *)MParAttr[mpaGRes],(void *)GResStr.c_str(),mdfString);
        }
      else
        {
        MStringAppendF(String,"%-14.14s %s\n",
          P->Name,
          (MUStrIsEmpty(GResStr.c_str())) ? "---" : GResStr.c_str());
        }
      }   /* END for (pindex) */
    }     /* END if (MAttr[meGRes][1][0] != '\0') */

  if (MAList[meNFeature][1][0] != '\0')
    {
    if (DFormat != mfmXML)
      {
      MStringAppend(String,"\nNode Features\n\n");
      }

    for (pindex = 0;pindex < MSched.M[mxoPar];pindex++)
      {
      P = &MPar[pindex];

      if (P->Name[0] == '\0')
        break;

      if (P->Name[0] == '\1')
        continue;

      if (P->ConfigNodes == 0)
        continue;

      if ((pindex == 0) && (ShowGlobal == FALSE))
        {
        /* ignore global partition if only one partition exists */

        continue;
        }

      MUNodeFeaturesToString(',',&P->FBM,Line);

      if (DFormat == mfmXML)
        {
        if ((Line[0] != '\0') && strcmp(Line,NONE))
          MXMLSetAttr(PE[pindex],(char *)MParAttr[mpaNodeFeatures],(void *)Line,mdfString);
        }
      else
        {
        if (!strcmp(Line,NONE))
          strcpy(Line,"---");

        MStringAppendF(String,"%-14.14s %s\n",
          P->Name,
          Line);
        }
      }   /* END for (pindex) */

    if ((DFormat != mfmXML) && bmisset(IFlags,mcmVerbose2))
      {
      mnode_t *N;

      mbitmap_t FBM;

      int findex;
      int nindex;

      int NUC;
      int NConC;
      int PUC;
      int PCC;
   
      /* present feature breakdown */

      MStringAppendF(String,"\n%-14.14s  %11s  %11s\n\n",
        "Feature",
        "Nodes (U:C)",
        "Procs (U:C)");

      for (findex = 1;MAList[meNFeature][findex][0] != '\0';findex++)
        {
        bmclear(&FBM);

        bmset(&FBM,findex);

        NUC = NConC = PUC = PCC = 0;

        for (nindex = 0;nindex < MSched.M[mxoNode];nindex++)
          {
          N = MNode[nindex];

          if ((N == NULL) || (N->Name[0] == '\0'))
            break;

          if (N->Name[0] == '\1')
            continue;

          if (MAttrSubset(&N->FBM,&FBM,mclAND))
            {
            NConC++;
            PCC+=N->CRes.Procs;

            if (MNODEISUP(N))
              {
              NUC++;
              PUC+=N->CRes.Procs;
              }
            }
          }

        MStringAppendF(String,"%-14.14s  %5d:%-5d  %5d:%-5d\n",
          MAList[meNFeature][findex],
          NUC,
          NConC,
          PUC,
          PCC);
        }
      }    /* END if (bmisset(IFlags,mcmVerbose2)) */
    }      /* END if (MAList[meNFeature][1][0] != '\0') */

  if ((ShowDiag == TRUE) && 
      (DFormat != mfmXML) &&
      (bmisset(IFlags,mcmVerbose)) &&
      (R != NULL))
    {
    if ((R->SubType == mrmstXT3) || (R->SubType == mrmstXT4))
      {
      MRMCheckAllocPartition(NULL,String,FALSE);
      }
    }

  if ((ShowDiag == TRUE) && (bmisset(IFlags,mcmVerbose)))
    {
    double PPrio;

    if (DFormat != mfmXML)
      {
      for (pindex = 1;pindex < MSched.M[mxoPar];pindex++)
        {
        P = &MPar[pindex];

        if (P->Name[0] == '\0')
          break;

        if (P->Name[0] == '\1')
          continue;

        if ((P->RM == NULL) || (P->RM->State == mrmsActive))
          continue;

        MStringAppendF(String,"NOTE:  master RM %s for partition %s is in state %s\n",
          P->RM->Name,
          P->Name,
          MRMState[P->RM->State]);
        }  /* END for (pindex) */
      }

    if (DFormat != mfmXML)
      {
      MStringAppend(String,"\nAllocation Priority\n\n");

      MStringAppendF(String,"%14s PRIORITY DETAILS...\n\n",
        " ");
      }

    for (pindex = 1;pindex < MSched.M[mxoPar];pindex++)
      {
      P = &MPar[pindex];

      if (P->Name[0] == '\0')
        break;

      if (P->Name[0] == '\1')
        continue;

      /* NOTE:  report messages even if partition is currently empty */

      if (P->MB != NULL)
        MessageDetected = TRUE;

      if (P->ConfigNodes == 0)
        continue;

      MParGetPriority(P,NULL,MSched.Time,&PPrio,MSched.Time,Line);
     
      if (DFormat == mfmXML)
        {
        /* NYI */
        }
      else
        {
        MStringAppendF(String,"%-14.14s %08.2f %s\n",
          P->Name,
          PPrio,
          Line);
        }
      }    /* END for (pindex) */

    /* NOTE: utilize backlog detected (NYI) */

    if (DFormat != mfmXML)
      {
      char tmpQTLine[MMAX_LINE];
      char tmpXFLine[MMAX_LINE];
  
      char tmpJobLine[MMAX_LINE];
      char TString[MMAX_LINE];
 
      MStringAppend(String,"\nBacklog\n\n");

      MStringAppendF(String,"%14s %4s  %9s  %15s  %10s  %5s\n\n",
        " ",
        "Jobs",
        "BacklogPS",
        "BacklogDuration",
        "AvgQTime",
        "AvgXF");

      for (pindex = 0;pindex < MSched.M[mxoPar];pindex++)
        {
        P = &MPar[pindex];

        if (P->Name[0] == '\0')
          break;

        if (P->Name[0] == '\1')
          continue;

        if (P->ConfigNodes == 0)
          continue;

        if (P->BacklogPS <= 0.0)
          continue;

        R = P->RM;

        if ((R != NULL) && (R->AvgQTime > 0))
          {
          MULToTString(R->AvgQTime,TString);

          strcpy(tmpQTLine,TString);
          }
        else
          strcpy(tmpQTLine,"---");

        if ((R != NULL) && (R->AvgXFactor > 0.0))
          sprintf(tmpXFLine,"%5.2f",
            R->AvgXFactor);
        else
          strcpy(tmpXFLine,"---");

        if (R != NULL) 
          sprintf(tmpJobLine,"%4d",
            R->JobCount);
        else
          strcpy(tmpJobLine,"---");

        MULToTString(P->BacklogDuration,TString);

        MStringAppendF(String,"%-14.14s %4s  %9s  %15s  %10s  %5s\n\n",
          P->Name,
          tmpJobLine,
          MUDToRSpec(P->BacklogPS,mvmByte,NULL), 
          TString,
          tmpQTLine,
          tmpXFLine);
        }  /* END for (pindex) */
      }      /* END if (DFormat != mfmXML) */
    else
      {
      mrm_t *R;

      /* report status in XML */


      for (pindex = 0;pindex < MSched.M[mxoPar];pindex++)
        {
        P = &MPar[pindex];

        if (P->Name[0] == '\0')
          break;

        if (P->Name[0] == '\1')
          continue;

        if (P->ConfigNodes == 0)
          continue;

        if (P->BacklogPS <= 0.0)
          continue;

        R = P->RM;

        if (R != NULL)
          {
          MXMLSetAttr(PE[pindex],(char *)MParAttr[mpaJobCount],(void *)&R->JobCount,mdfInt);
          }
        }
      }    /* END else (DFormat != mfmXML) */

    if ((DFormat != mfmXML) && (MessageDetected == TRUE))
      {
      MStringAppend(String,"\nMessages\n\n");

      for (pindex = 0;pindex < MSched.M[mxoPar];pindex++)
        {
        P = &MPar[pindex];

        if (P->Name[0] == '\0')
          break;

        if (P->Name[0] == '\1')
          continue;

        /* NOTE:  report messages even if partition is currently empty */

        if (P->MB == NULL)
          continue;

        MStringAppendF(String,"Messages for partition %s ------------\n",
          P->Name);

        if (P->MB != NULL)
          {
          char tmpBuf[MMAX_BUFFER];

          MMBPrintMessages(P->MB,mfmHuman,TRUE,-1,tmpBuf,sizeof(tmpBuf));

          MStringAppend(String,tmpBuf);
          }
        }  /* END for (pindex) */
      }    /* END if ((DFormat != mfmXML) && (MessageDetected == TRUE)) */
    }      /* END if ((ShowDiag == TRUE) && ...) */

  return(SUCCESS);
  }  /* END MParShow() */




int MParGetTC( 
 
  mpar_t  *P,     /* I */
  mcres_t *Avl,   /* I */
  mcres_t *Cfg,   /* I */
  mcres_t *Ded,   /* I (optional) */
  mcres_t *Req,   /* I (the req resources) */
  long     Time)  /* I */
 
  {
  mcres_t tmpReq;

  int     NodeIsDedicated = FALSE;
 
  int     TC;

  const char *FName = "MParGetTC";

  MDB(6,fUI) MLog("%s(%s,Avl,Cfg,Ded,Req,%ld)\n",
    FName,
    (P != NULL) ? P->Name : "NULL",
    Time);
 
  MCResInit(&tmpReq);
  MCResCopy(&tmpReq,Req);
 
  if (tmpReq.Procs == -1)
    {
    tmpReq.Procs = 1;
    NodeIsDedicated = TRUE;
    }
 
  if (tmpReq.Mem == -1)
    {
    tmpReq.Mem = 1;
    NodeIsDedicated = TRUE;
    }
 
  if (tmpReq.Swap == -1)
    {
    tmpReq.Swap = 1;
    NodeIsDedicated = TRUE;
    }
 
  if (tmpReq.Disk == -1)
    {
    tmpReq.Disk = 1;
    NodeIsDedicated = TRUE; 
    }
 
  TC = MNodeGetTC(NULL,Avl,Cfg,Ded,&tmpReq,Time,1,NULL);
 
  MCResFree(&tmpReq);

  if (NodeIsDedicated == FALSE)
    {
    return(TC);
    }
 
  return(MIN(TC,P->ConfigNodes));
  }  /* END MParGetTC() */



/**
 * Process 'old-style' partition config parameters.
 *
 * @see MParConfigShow() - peer - report active partition config settings
 *
 * @param P (I)
 * @param PIndex (I)
 * @param IVal (I)
 * @param DVal (I)
 * @param SVal (I)
 * @param SArray (I)
 */

int MParProcessOConfig(

  mpar_t *P,      /* I */
  enum MCfgParamEnum PIndex, /* I */
  int     IVal,   /* I */
  double  DVal,   /* I */
  char   *SVal,   /* I */
  char  **SArray) /* I */

  {
  int tmpI;

  if (P == NULL)
    {
    return(FAILURE);
    }

  switch (PIndex)
    {
    case mcoAdminMinSTime:

      {
      char *ptr;
      char *TokPtr = NULL;

      ptr = MUStrTok(SVal,",",&TokPtr);

      ptr = MUStrTok(NULL,",",&TokPtr);

      P->AdminMinSTime = MUTimeFromString(SVal);

      if (ptr != NULL)
        P->JobMinSTime = MUTimeFromString(ptr);
      }  /* END BLOCK (case mcoAdminMinSTime) */
 
      break;

    case mcoBFDepth:

      P->BFDepth = IVal;

      break;

    case mcoBFMetric:

      P->BFMetric = (enum MBFMetricEnum)MUGetIndexCI(SVal,MBFMPolicy,FALSE,P->BFMetric);

      if (MBFMPolicy[P->BFMetric] == NULL)
        {
        MDB(1,fCONFIG) MLog("ALERT:    invalid %s parameter specified '%s'\n",
          MParam[mcoBFMetric],
          SVal);
        }

      break;

    case mcoBFMinVirtualWallTime:

      P->MinVirtualWallTime = MUTimeFromString(SVal);

      break;

    case mcoBFVirtualWallTimeConflictPolicy:

      P->VWConflictPolicy = (enum MVWConflictPolicyEnum)MUGetIndexCI(
        SVal,
        (const char **)MVWConflictPolicy,
        FALSE,
        mbfNONE);

      break;

    case mcoBFVirtualWallTimeScalingFactor:

      P->VirtualWallTimeScalingFactor = strtod(SVal,NULL);
 
      break;

    case mcoBFPolicy:

      {
      enum MBFPolicyEnum BIndex;

      BIndex = (enum MBFPolicyEnum)MUGetIndexCI(
        SVal,
        (const char **)MBFPolicy,
        FALSE,
        mbfNONE);

      if (BIndex != mbfNONE)
        {
        P->BFPolicy = BIndex;
        }
      else
        {
        /* support deprecated format */

        if ((MUBoolFromString(SVal,TRUE) == FALSE) || !strcmp(SVal,"NONE"))
          P->BFPolicy = mbfNONE;
        }
      }    /* END BLOCK */

      break;

    case mcoBFProcFactor:

      P->BFProcFactor = IVal;

      break;

    case mcoBFMaxSchedules:

      P->BFMaxSchedules = IVal;

      break;

    case mcoDisableMultiReqJobs:

      if (MUBoolFromString(SVal,FALSE) == TRUE)
        bmset(&P->Flags,mpfDisableMultiReqJobs);
      else
        bmunset(&P->Flags,mpfDisableMultiReqJobs);

      break;

    case mcoEnableNegJobPriority:

      if (MUBoolFromString(SVal,FALSE) == TRUE)
        bmset(&P->Flags,mpfEnableNegJobPriority);
      else
        bmunset(&P->Flags,mpfEnableNegJobPriority);

      break;

    case mcoEnablePosUserPriority:

      if (MUBoolFromString(SVal,FALSE) == TRUE)
        bmset(&P->Flags,mpfEnablePosUserPriority);
      else
        bmunset(&P->Flags,mpfEnablePosUserPriority);

      break;

    case mcoJobNodeMatch:

      bmclear(&P->JobNodeMatchBM);

      {
      int index;
      int index2;

      for (index = 0;SArray[index] != NULL;index++)
        {
        for (index2 = 0;MJobNodeMatchType[index2] != NULL;index2++)
          {
          if (strstr(SArray[index],MJobNodeMatchType[index2]))
            bmset(&P->JobNodeMatchBM,index2);
          }
        }  /* END for (index) */
      }    /* END BLOCK */

      break;

    case mcoJobRetryTime:

      P->JobRetryTime = MUTimeFromString(SVal);

      break;

    case mcoMaxJobStartTime:

      P->MaxJobStartTime = MUTimeFromString(SVal);

      break;

    case mcoNodeAllocationPolicy:

      MParSetAttr(P,mpaNodeAllocPolicy,(void **)SVal,mdfString,mSet);

      break;

    case mcoNodeAllocResFailurePolicy:

      P->ARFPolicy = (enum MAllocResFailurePolicyEnum)MUGetIndexCI(
        SVal,
        MARFPolicy,
        FALSE,
        P->ARFPolicy);

      break;

    case mcoNodeAvailPolicy:

      MParProcessNAvailPolicy(P,SArray,FALSE,NULL);

      break;

    case mcoParAdmin:

      MParSetAttr(P,mpaOwner,(void **)SVal,mdfString,mSet);

      break;

    case mcoParAllocationPolicy:

      P->PAllocPolicy = (enum MParAllocPolicyEnum)MUGetIndexCI(
        SVal,
        MPAllocPolicy,
        FALSE,
        P->PAllocPolicy);

      if (P != &MPar[0])
        MPar[0].PAllocPolicy = P->PAllocPolicy;

      if ((MSched.DefaultN.P == NULL) &&
          (MNPrioAlloc(&MSched.DefaultN.P) == FAILURE))
        {
        return(FAILURE);
        }

      /* NOTE:  translate/enforce partition allocation policy */

      switch (P->PAllocPolicy)
        {
        case mpapBestAFit:   /* best absolute fit */

          MPar[0].QTWeight = 1.0;
          MPar[0].NPWeight = 1.0;
          MPar[0].ATWeight = 0.0;

          /* dis-favor partitions with available processors */

          MSched.DefaultN.P->CW[mnpcParAProcs] = -100;
          MSched.DefaultN.P->CW[mnpcPParAProcs] = 0;

          MSched.DefaultN.P->CWIsSet = TRUE;

          break;

        case mpapBestPFit:   /* best percentage fit */

          MPar[0].QTWeight = 1.0;
          MPar[0].NPWeight = 1.0;
          MPar[0].ATWeight = 0.0;

          /* dis-favor partitions with available processors (percent) */

          MSched.DefaultN.P->CW[mnpcParAProcs] = 0;
          MSched.DefaultN.P->CW[mnpcPParAProcs] = -100;

          MSched.DefaultN.P->CWIsSet = TRUE;

          break;

        case mpapFirstCompletion: /* earliest job completion */

          MPar[0].QTWeight = 1.0;
          MPar[0].NPWeight = 0.0;
          MPar[0].ATWeight = 0.0;
          MPar[0].SJWeight = 0.0;

          break;

        case mpapFirstStart:      /* earliest job start */

          MPar[0].QTWeight = 1.0;
          MPar[0].NPWeight = 0.0;
          MPar[0].ATWeight = 0.0;
          MPar[0].SJWeight = 0.0;

          break;

        case mpapRR:              /* round robin */

          MPar[0].QTWeight = 0.0;
          MPar[0].NPWeight = 0.0;
          MPar[0].ATWeight = 1.0;
          MPar[0].SJWeight = 1.0;

          break;

        case mpapLoadBalance:     /* worst absolute fit */

          MPar[0].QTWeight = 1.0;
          MPar[0].NPWeight = 1.0;
          MPar[0].ATWeight = 0.0;
          MPar[0].SJWeight = 0.0;

          /* favor partitions with available processors */

          MSched.DefaultN.P->CW[mnpcParAProcs]  = 100;
          MSched.DefaultN.P->CW[mnpcPParAProcs] = 0;

          MSched.DefaultN.P->CWIsSet = TRUE;

          break;

        case mpapLoadBalanceP:    /* worst percentage fit */
        default:

          MPar[0].QTWeight = 1.0;
          MPar[0].NPWeight = 1.0;
          MPar[0].ATWeight = 0.0;
          MPar[0].SJWeight = 0.0;

          /* favor partitions with available processors (percent) */

          MSched.DefaultN.P->CW[mnpcParAProcs]  = 0;
          MSched.DefaultN.P->CW[mnpcPParAProcs] = 100;

          MSched.DefaultN.P->CWIsSet = TRUE;

          break;
        }  /* END switch (P->PAllocPolicy) */

      break;  /* END switch (mcoParAllocPolicy) */

    case mcoPriorityTargetDuration:

      {
      char *tail;

      /* FORMAT:  [[[DD:]HH:]MM:]SS[FSTARGETMODE] */

      tail = &SVal[strlen(SVal) - 1];

      if (!isdigit(*tail))
        {
        /* determine target type */

        if (*tail == MFSTargetTypeMod[mfstFloor])
          P->PTDurationTargetMode = mfstFloor;
        else if (*tail == MFSTargetTypeMod[mfstCeiling])
          P->PTDurationTargetMode = mfstCeiling;
        else
          P->PTDurationTargetMode = mfstTarget; 
        }
      else
        {
        P->PTDurationTargetMode = mfstTarget;
        }

      P->PriorityTargetDuration = MUTimeFromString(SVal);
      }

      break;

    case mcoPriorityTargetProcCount:

      /* FORMAT:  <PROCCOUNT>[FSTARGETMODE] */

      {
      mfs_t tmpFS;

      MFSTargetFromString(&tmpFS,SVal,TRUE);

      P->PriorityTargetProcCount = (int)tmpFS.FSTarget;
      P->PTProcCountTargetMode   = tmpFS.FSTargetMode;
      }

      break;

    case mcoRangeEvalPolicy:

      /* NOTE:  force to msched for now */

      if (!strcmp(SVal,"LONGEST"))
        {
        MSched.SeekLong = TRUE;
        }
      else
        {
        MSched.SeekLong = FALSE;
        }

      break;

    case mcoRejectNegPrioJobs:

      if (MUBoolFromString(SVal,FALSE) == TRUE)
        bmset(&P->Flags,mpfRejectNegPrioJobs);
      else
        bmunset(&P->Flags,mpfRejectNegPrioJobs);

      break;

    case mcoResourceLimitMultiplier:

      {
      /* FORMAT <RESOURCE>:<MULTIPLIER>[,...] */

      char *ptr;
      char *TokPtr = NULL;

      char *ptr2;
      char *TokPtr2 = NULL;

      int aIndex;
      
      ptr = MUStrTok((char *)SVal,",",&TokPtr);

      while (ptr != NULL)
        {
        ptr2 = MUStrTok(ptr,":",&TokPtr2);
        
        aIndex = MUGetIndexCI(
          ptr2,
          (const char **)MResLimitType,
          FALSE,
          (int)mrNONE);

        P->ResourceLimitMultiplier[aIndex] = MAX(0,strtod(TokPtr2,NULL));
      
        ptr = MUStrTok(NULL,",",&TokPtr);
        }  /* END while (ptr != NULL) */
      }    /* END BLOCK (case mcoResourceLimitMultiplier) */

      break;

    case mcoResourceLimitPolicy:

      MUParseResourceLimitPolicy(P,SArray);

      break;

    case mcoRsvNodeAllocPolicy:

      MParSetAttr(P,mpaRsvNodeAllocPolicy,(void **)SVal,mdfString,mSet);

      break;

    case mcoRsvNodeAllocPriorityF:

      MParSetAttr(P,mpaRsvNodeAllocPriorityF,(void **)SVal,mdfString,mSet);

      break;

    case mcoRsvPolicy:

      tmpI = MUGetIndexCI(SVal,MRsvPolicy,FALSE,0);

      if (tmpI != 0)
        {
        P->RsvPolicy = (enum MRsvPolicyEnum)tmpI;
        }
      else
        {
        MDB(1,fCONFIG) MLog("ALERT:    invalid %s parameter specified '%s'\n",
          MParam[mcoRsvPolicy],
          SVal);
        }

      break;

    case mcoRsvRetryTime:

      if (strchr(SVal,'^'))
        P->RestrictedRsvRetryTime = TRUE;

      P->RsvRetryTime = MUTimeFromString(SVal);

      break;

    case mcoRsvThresholdType:

      tmpI = MUGetIndexCI(SVal,MRsvThresholdType,FALSE,0);

      if (tmpI != 0)
        {
        P->RsvTType = (enum MRsvThresholdEnum)tmpI;
        }
      else
        {
        MDB(1,fCONFIG) MLog("ALERT:    invalid %s parameter specified '%s'\n",
          MParam[mcoRsvThresholdType],
          SVal);
        }
      
      break;

    case mcoRsvThresholdValue:

      P->RsvTValue = IVal;

      break;

    case mcoRsvTimeDepth:

      MSched.RsvTimeDepth = IVal;

      break;

    case mcoRsvSearchAlgo:

      {
      enum MRsvSearchAlgoEnum tmpI;

      tmpI = (enum MRsvSearchAlgoEnum)MUGetIndexCI(SVal,MRsvSearchAlgo,FALSE,0);

      if (tmpI != mrsaNONE)
        MSched.RsvSearchAlgo = tmpI;
      }  /* END BLOCK */

      break;

    case mcoMaxMetaTasks:

      P->MaxMetaTasks = IVal;

      break;

    case mcoSuspendRes:

      MParSetAttr(
        P,
        mpaDedSRes,
        (void **)SVal,
        mdfString,
        mSet);
      
      break;

    case mcoSystemMaxJobProc:

      MPar[0].L.JobPolicy->HLimit[mptMaxProc][0] = IVal;
      MPar[0].L.JobPolicy->SLimit[mptMaxProc][0] = IVal;

      break;

    case mcoSystemMaxJobTime:

      MPar[0].L.JobPolicy->HLimit[mptMaxWC][0] = MUTimeFromString(SVal);
      MPar[0].L.JobPolicy->SLimit[mptMaxWC][0] = MPar[0].L.JobPolicy->HLimit[mptMaxWC][0];

      break;

    case mcoSystemMaxJobPS:

      MPar[0].L.JobPolicy->HLimit[mptMaxPS][0] = IVal;
      MPar[0].L.JobPolicy->SLimit[mptMaxPS][0] = IVal;

      break;

    case mcoTaskDistributionPolicy:

      tmpI = MUGetIndexCI(SVal,MTaskDistributionPolicy,FALSE,0);

      if (tmpI != 0)
        {
        P->DistPolicy = (enum MTaskDistEnum)tmpI;
        }
      else
        {
        MDB(1,fCONFIG) MLog("ALERT:    invalid %s parameter specified '%s'\n",
          MParam[mcoTaskDistributionPolicy],
          SVal);
        }

      break;

    case mcoUseSystemQueueTime:

      if (MUBoolFromString(SVal,FALSE) == TRUE)
        bmset(&P->Flags,mpfUseSystemQueueTime);
      else
        bmunset(&P->Flags,mpfUseSystemQueueTime);

      break;

    case mcoUseCPUTime:

      if (MUBoolFromString(SVal,FALSE) == TRUE)
        bmset(&P->Flags,mpfUseCPUTime);
      else
        bmunset(&P->Flags,mpfUseCPUTime);
 
      break;

    case mcoUseFloatingNodeLimits:

      if (MUBoolFromString(SVal,FALSE) == TRUE)
        bmset(&P->Flags,mpfUseFloatingNodeLimits);
      else
        bmunset(&P->Flags,mpfUseFloatingNodeLimits);

      break;

    case mcoUseMachineSpeed:

      if (MUBoolFromString(SVal,FALSE) == TRUE)
        bmset(&P->Flags,mpfUseMachineSpeed);
      else
        bmunset(&P->Flags,mpfUseMachineSpeed);

      break;

    case mcoUseNodeHash:

      MSched.UseNodeIndex = MUBoolFromString(SVal,FALSE);

      break;

    case mcoUseMoabCTime:

      MSched.UseMoabCTime = MUBoolFromString(SVal,FALSE);

    case mcoJobPrioAccrualPolicy:

      MCfgParsePrioAccrualPolicy(&P->JobPrioAccrualPolicy,&P->JobPrioExceptions,SVal);
      
      break;

    case mcoJobPrioExceptions:

      MCfgParsePrioExceptions(&P->JobPrioExceptions,SVal);

      break;

    case mcoBFPriorityPolicy:

      P->BFPriorityPolicy = (enum MBFPriorityEnum)MUGetIndexCI(
        SVal,
        MBFPriorityPolicyType,
        FALSE,
        P->BFPriorityPolicy);

      break;

    case mcoBFChunkDuration:

      P->BFChunkDuration = MUTimeFromString(SVal);

      if (MSched.Iteration > 0)
        P->BFChunkBlockTime = MSched.Time + P->BFChunkDuration; 

      break;

    case mcoBFChunkSize:

      P->BFChunkSize = IVal;

      break;

    case mcoNodeSetAttribute:

      P->NodeSetAttribute = (enum MResourceSetTypeEnum)MUGetIndexCI(
        SVal,
        (const char **)MResSetAttrType,
        FALSE,
        P->NodeSetAttribute);

      break;

    case mcoNodeSetDelay:

      P->NodeSetDelay = MUTimeFromString(SVal);

      if (P->NodeSetDelay > 0)
        P->NodeSetIsOptional = FALSE;
      else
        P->NodeSetIsOptional = TRUE;

      break;

    case mcoNodeSetForceMinAlloc:

      P->NodeSetForceMinAlloc = MUBoolFromString(SVal,TRUE);

      break;

    case mcoNodeSetIsOptional:

      /* a non-zero nodeset delay will make nodesets mandatory */

      if (MUBoolFromString(SVal,TRUE) == TRUE)
        P->NodeSetIsOptional = TRUE;
      else
        P->NodeSetIsOptional = FALSE;

      break;

    case mcoNodeSetList:

      /* FORMAT:  <ATTR>[{ :,|}<ATTR>]... */

      {
      int index;

      if ((SArray == NULL) || (SArray[0] == NULL))
        {
        /* clear node set list */

        for (index = 0;index < MMAX_ATTR;index++)
          {
          if (P->NodeSetList[index] == NULL)
            break;

          MUFree(&P->NodeSetList[index]);
          }  /* END for (index) */
        }
      else
        {
        char *ptr;
        char *TokPtr;

        int   sindex = 0;

        for (index = 0;index < MMAX_ATTR;index++)
          {
          if (SArray[index] == NULL)
            break;

          ptr = MUStrTok(SArray[index]," :,|",&TokPtr);

          while (ptr != NULL)
            {
            MUStrDup(&P->NodeSetList[sindex],ptr);

            sindex++;

            if (sindex >= MMAX_ATTR)
              break;

            ptr = MUStrTok(NULL," :,|",&TokPtr);
            }

          }  /* END for (index) */
        }    /* END else ((SArray == NULL) || (SArray[0] == NULL)) */
      }      /* END BLOCK (case mcoNodeSetList) */

      break;

    case mcoNodeSetPolicy:

      P->NodeSetPolicy = (enum MResourceSetSelectionEnum)MUGetIndexCI(
        SVal,
        (const char **)MResSetSelectionType,
        FALSE,
        P->NodeSetPolicy);

      break;

    case mcoNodeSetPriorityType:

      P->NodeSetPriorityType = (enum MResourceSetPrioEnum)MUGetIndexCI(
        SVal,
        (const char **)MResSetPrioType,
        FALSE,
        mrspNONE);

      if ((bmisset(&P->Flags,mpfSharedMem)) &&
          (P->NodeSetPriorityType == mrspMinLoss))
        P->NodeSetPlus = mnspDelay;

      break;

    case mcoNodeSetMaxUsage:

      if ((DVal > 1.0) || (DVal < 0.0))
        {
        MDB(1,fSTRUCT) MLog("ERROR:    ignoring NODESETMAXUSAGE because set incorrectly (%f). Valid values are 0.0 - 1.0.\n",
          DVal);
        }
      else
        {
        P->NodeSetMaxUsage = DVal;
        }

      break;

    case mcoNodeSetTolerance:

      P->NodeSetTolerance = DVal;

      break;

    case mcoVPCNodeAllocationPolicy:

      MParSetAttr(P,mpaVPCNodeAllocPolicy,(void **)SVal,mdfString,mSet);

      break;

    case mcoWattChargeRate:

      MParSetAttr(P,mpaWattChargeRate,(void **)SVal,mdfDouble,mSet);

      break;

    default:

      /* NOT HANDLED */

      return(FAILURE);

      /*NOTREACHED*/

      break;
    }  /* END switch (PIndex) */

  return(SUCCESS);
  }  /* END MParProcessOConfig() */


/**
 * Update per-partition state and usage information.
 *
 * @see MSysMainLoop() - parent - update each iteration after scheduling cycle
 * @see MRMUpdate() - parent - update on iteration 0 or if new RM is initialized/loaded
 * @see MJobStart() - parent - update associated partition after job is started
 * @see MSchedProcessJobs() - parent - update if MSysScheduleProvisioning() updates partition
 *
 * NOTE:  evaluate all partitions if SP points to global partition (MPar[0])
 * NOTE:  if partition has dynamic job template, load template on iteration 0, 
 *        schedule template on subsequent iterations
 *
 * @param SP (I) [modified]
 * @param UpdateStatistics (I)
 * @param JobInfoLoaded (I)
 */

int MParUpdate(

  mpar_t  *SP,               /* I (modified) */
  mbool_t  UpdateStatistics, /* I */
  mbool_t  JobInfoLoaded)    /* I */

  {
  int pindex;
  int nindex;

  mnode_t *N;
  mpar_t  *GP;
  mpar_t  *P;

  int EffNC;  /* address pseudo/grid nodes */

  const char *FName = "MParUpdate";

  MDB(4,fSTRUCT) MLog("%s(%s,%s,%s)\n",
    FName,
    (SP != NULL) ? SP->Name : "NULL",
    MBool[UpdateStatistics],
    MBool[JobInfoLoaded]);

  if (SP == NULL)
    {
    return(FAILURE);
    }

  MDB(4,fSTRUCT) MLog("INFO:     P[%s]:  Total %d:%d  Up %d:%d  Idle %d:%d  Active %d:%d\n",
    SP->Name,
    SP->ConfigNodes,
    SP->CRes.Procs,
    SP->UpNodes,
    SP->UpRes.Procs,
    SP->IdleNodes,
    SP->ARes.Procs,
    SP->ActiveNodes,
    SP->DRes.Procs);

  GP = &MPar[0];

  /* clear partition utilization stats */

  for (pindex = 0;pindex < MSched.M[mxoPar];pindex++)
    {
    P = &MPar[pindex];

    if ((SP != GP) && (P != GP) && (P != SP))
      continue;

    if (P->Name[0] == '\1')
      continue;

    if (P->ConfigNodes == 0)
      continue;

    if (SP == GP)
      {
      P->ConfigNodes = 0;

      P->UpNodes     = 0;

      MCResClear(&P->CRes);
      MCResClear(&P->UpRes);
      }

    P->IdleNodes   = 0;

    P->ActiveNodes = 0;

    /* Only clear when the global partition is being looked at. MJobStart
     * calls this routine with a specific partition and clears the value
     * but there are still more jobs to look at. */

    if (SP == GP)
      P->S.ISubmitJC = 0;

    bmclear(&P->FBM);

    MCResClear(&P->ARes);
    MCResClear(&P->DRes);
    }  /* END for (pindex) */

  for (nindex = 0;nindex < MSched.M[mxoNode];nindex++)
    {
    N = MNode[nindex];

    if ((N == NULL) || (N->Name[0] == '\0'))
      break;

    if (N->Name[0] == '\1')
      continue;

    if ((N->RackIndex > 0) &&
         bmisset(&MSys[N->RackIndex][N->SlotIndex].Attributes,mattrSystem))
      {
      continue;
      }

    EffNC = 1;

    /* associate node's classes with partition */

    P = &MPar[N->PtIndex];

    GP->MaxPPN = MAX(GP->MaxPPN,N->CRes.Procs);

    if (P == GP)
      {
      /* NOTE: A storage resource manager is not associated with a partition. */

      if (strcmp(N->Name,"GLOBAL") && 
         (N->RM != NULL) && 
         !bmisset(&N->RM->RTypes,mrmrtStorage))
        {
        MDB(1,fSTRUCT) MLog("ERROR:    node '%s' is not associated with any partition\n",
          N->Name);
        }

      continue;
      }

    if ((P->RM == NULL) && (N->RM != NULL))
      {
      /* update partition */

      P->RM = N->RM;
      }

    /* NOTE:  if node is tied to 'TrackOS' partition and node's OSList allows 
              association with other partitions, these other partitions should
              be updated with reflect all resources which could migrate to
              these partitions (NYI) */

    /* NOTE:  some nodes/partitions (ie, all, shared) will not have associated RM's */

    if (P->RM != NULL)
      {
      P->RM->PtIndex = P->Index;
      }

    P->MaxPPN = MAX(P->MaxPPN,N->CRes.Procs);

    if (SP == GP)
      {
      /* global update */

      /* NOTE:  node up status only changes on once per iteration basis */

      P->ConfigNodes += EffNC;

      MCResAdd(&P->CRes,&N->CRes,&N->CRes,1,FALSE);
      MCResAdd(&GP->CRes,&N->CRes,&N->CRes,1,FALSE);

      GP->ConfigNodes += EffNC;

      if (MNODEISUP(N) == TRUE)
        {
        P->UpNodes += EffNC;

        MCResAdd(&P->UpRes,&N->CRes,&N->CRes,1,FALSE);

        GP->UpNodes += EffNC;

        MCResAdd(&GP->UpRes,&N->CRes,&N->CRes,1,FALSE);

        bmor(&GP->FBM,&N->FBM);
        bmor(&GP->Classes,&N->Classes);
        }
      }   /* END if (SP == GP) */

    if ((SP == GP) || (SP == P))
      {
      if (MNODEISUP(N) == TRUE)
        {
        /* add available node resources to available partition resources */

        MCResAdd(&P->ARes,&N->CRes,&N->ARes,1,FALSE);
        MCResAdd(&GP->ARes,&N->CRes,&N->ARes,1,FALSE);

        /* add dedicated node resources to dedicated partition resources */

        if (N->DRes.Procs >= 0)
          {
          MCResAdd(&P->DRes,&N->CRes,&N->DRes,1,FALSE);
          MCResAdd(&GP->DRes,&N->CRes,&N->DRes,1,FALSE);
          }

        if ((N->ARes.Procs == N->CRes.Procs) && (N->DRes.Procs == 0))
          {
          if ((SP == GP) || (SP == P))
            {
            P->IdleNodes += EffNC;
            }

          GP->IdleNodes += EffNC;
          }

        /* NOTE:  discrepancies occur when background load is applied */

        if ((N->TaskCount > 0) || (N->State == mnsReserved))
          {
          if ((SP == GP) || (SP == P))
            {
            P->ActiveNodes += EffNC;
            }

          GP->ActiveNodes += EffNC;
          }

        bmor(&P->FBM,&N->FBM);
        bmor(&P->Classes,&N->Classes);
        }    /* END if (MNODEISUP(N) == TRUE) */

      MDB(5,fSTRUCT) MLog("INFO:     MNode[%s] added to MPar[%s] (%d:%d)\n",

        N->Name,
        MPar[N->PtIndex].Name,
        N->ARes.Procs,
        N->CRes.Procs);
      }
    }      /* END for (nindex) */

  /* virtual private cluster update */

  MVPCUpdate(NULL);

  /* update global statistics */

  if (UpdateStatistics == TRUE)
    {
    must_t *GSP;

    MStat.NC   = GP->UpNodes;
    MStat.TNC += GP->UpNodes;

    MStat.PC   = GP->UpRes.Procs;
    MStat.TPC += GP->UpRes.Procs;

    if (MPar[0].S.IStatCount > 0)
      GSP = (must_t *)MPar[0].S.IStat[MPar[0].S.IStatCount - 1];
    else
      GSP = NULL;

    if (GSP != NULL)
      {
      GSP->TNC += GP->UpNodes;

      GSP->TPC += GP->UpRes.Procs;
      }

    if (GP->FSC.TargetIsAbsolute == TRUE)
      {
      double interval;

      /* update theoretical fairshare usage */

      interval = (double)MSched.Interval / 100.0;

      switch (GP->FSC.FSPolicy)
        {
        case mfspDPS:
        default:

          GP->F.FSUsage[0] += GP->CRes.Procs * interval;

          break;
        }
      }
    }      /* END if (UpdateStatistics == TRUE) */

  /* remove partition if parent RM has been removed */

  for (pindex = 0;pindex < MSched.M[mxoPar];pindex++)
    {
    P = &MPar[pindex];

    if ((SP != GP) && (P != GP) && (P != SP))
      continue;

    if (P->Name[0] == '\1')
      continue;

    if (P->ConfigNodes != 0)
      continue;

    if (bmisset(&P->Flags,mpfDestroyIfEmpty))
      MParDestroy(&P);
    }  /* END for (pindex) */

  MDB(4,fSTRUCT) MLog("INFO:     P[%s]:  Total %d:%d  Up %d:%d  Idle %d:%d  Active %d:%d\n",
    SP->Name,
    SP->ConfigNodes,
    SP->CRes.Procs,
    SP->UpNodes,
    SP->UpRes.Procs,
    SP->IdleNodes,
    SP->ARes.Procs,
    SP->ActiveNodes,
    SP->DRes.Procs);

  return(SUCCESS);
  }  /* END MParUpdate() */





/**
 * Report current partition config parameter settings.
 *
 * @see MParProcessOConfig() - peer - process partition config settings
 *
 * @param P (I)
 * @param VFlag (I)
 * @param PIndex (I) [parameter index]
 * @param String (O)
 */

int MParConfigShow(

  mpar_t     *P,
  int        VFlag,
  int        PIndex,
  mstring_t *String)

  {
  /* NOTE:  no bounds checking */

  char TString[MMAX_LINE];

  if ((P == NULL) || (String == NULL))
    {
    return(FAILURE);
    }

  MStringSet(String,"\0");

  if (P->Index == 0)
    {
    MStringAppend(String,"# global policies\n\n");
    }
  else
    {
    MStringAppendF(String,"\n# partition %s policies\n\n",
      P->Name);
    }

  if (P->Index == 0)
    {
    if ((bmisset(&P->Flags,mpfRejectNegPrioJobs)) ||
        (VFlag || (PIndex == -1) || (PIndex == mcoRejectNegPrioJobs)))
      {
      MStringAppend(String,
        MUShowString(
          MParam[mcoRejectNegPrioJobs],
          (char *)MBool[bmisset(&P->Flags,mpfRejectNegPrioJobs)]));
      }

    if ((bmisset(&P->Flags,mpfEnableNegJobPriority)) ||
        (VFlag || (PIndex == -1) || (PIndex == mcoEnableNegJobPriority)))
      {
      MStringAppend(String,
        MUShowString(
          MParam[mcoEnableNegJobPriority],
          (char *)MBool[bmisset(&P->Flags,mpfEnableNegJobPriority)]));
      }

    if ((bmisset(&P->Flags,mpfEnablePosUserPriority)) ||
        (VFlag || (PIndex == -1) || (PIndex == mcoEnablePosUserPriority)))
      {
      MStringAppend(String,
        MUShowString(
          MParam[mcoEnablePosUserPriority],
          (char *)MBool[bmisset(&P->Flags,mpfEnablePosUserPriority)]));
      }

    if ((bmisset(&P->Flags,mpfDisableMultiReqJobs)) ||
        (VFlag || (PIndex == -1) || (PIndex == mcoDisableMultiReqJobs)))
      {
      MStringAppend(String,
        MUShowString(
          MParam[mcoDisableMultiReqJobs],
          (char *)MBool[bmisset(&P->Flags,mpfDisableMultiReqJobs)]));
      }

    if ((P->JobPrioAccrualPolicy != mjpapNONE) ||
        (VFlag || (PIndex == -1) || (PIndex == mcoJobPrioAccrualPolicy)))
      {
      if (P->Index == 0)
        {
        MStringAppend(String,
          MUShowString(
            MParam[mcoJobPrioAccrualPolicy],
            (char *)MJobPrioAccrualPolicyType[P->JobPrioAccrualPolicy]));
        }
      }

    if ((!bmisclear(&P->JobPrioExceptions)) ||
        (VFlag || (PIndex == -1) || (PIndex == mcoJobPrioExceptions)))
      {
      if (P->Index == 0)
        {
        char tmpLine[MMAX_LINE];

        bmtostring(&P->JobPrioExceptions,MJobPrioException,tmpLine);

        MStringAppend(String,
          MUShowString(
            MParam[mcoJobPrioExceptions],
            tmpLine));
        }
      }

    if ((P->VWConflictPolicy != MDEF_VWCONFLICTPOLICY) ||
        (VFlag || (PIndex == -1) || (PIndex == mcoBFVirtualWallTimeConflictPolicy)))
      {
      MStringAppend(String,
        MUShowString(
          MParam[mcoBFVirtualWallTimeConflictPolicy],
          (char *)MVWConflictPolicy[P->VWConflictPolicy]));
      }

    if ((P->L.JobPolicy->HLimit[mptMaxProc][0] != 0) ||
       (VFlag || (PIndex == -1) || (PIndex == mcoSystemMaxJobProc)))
      {
      MStringAppendF(String,"%-30s  %d\n",
        MParam[mcoSystemMaxJobProc],
        P->L.JobPolicy->HLimit[mptMaxProc][0]);
      }

     if ((P->L.JobPolicy->HLimit[mptMaxWC][0] != 0) ||
       (VFlag || (PIndex == -1) || (PIndex == mcoSystemMaxJobTime)))
      {
      MULToTString(P->L.JobPolicy->HLimit[mptMaxWC][0],TString);

      MStringAppend(String,
        MUShowString(
          MParam[mcoSystemMaxJobTime],
          TString));
      }

     if ((P->L.JobPolicy->HLimit[mptMaxPS][0] != 0) ||
       (VFlag || (PIndex == -1) || (PIndex == mcoSystemMaxJobPS)))
      {
      MStringAppendF(String,"%-30s  %d\n",
        MParam[mcoSystemMaxJobPS],
        P->L.JobPolicy->HLimit[mptMaxPS][0]);
      }

    if ((P->BFPriorityPolicy != mbfpNONE) ||
        (VFlag || (PIndex == -1) || (PIndex == mcoBFPriorityPolicy)))
      {
      MStringAppend(String,
        MUShowString(
          MParam[mcoBFPriorityPolicy],
          (char *)MBFPriorityPolicyType[P->BFPriorityPolicy]));
      }

    if ((P->BFChunkDuration != 0) ||
        (VFlag || (PIndex == -1) || (PIndex == mcoBFChunkDuration)))
      {
      MULToTString(P->BFChunkDuration,TString);

      MStringAppend(String,
        MUShowString(
          MParam[mcoBFChunkDuration],
          TString));
      }

    if ((P->BFChunkSize != 0) ||
        (VFlag || (PIndex == -1) || (PIndex == mcoBFChunkSize)))
      {
      MStringAppendF(String,"%-30s  %d\n",
        MParam[mcoBFChunkSize],
        P->BFChunkSize);
      }

    if ((bmisset(&P->Flags,mpfUseMachineSpeed)) ||
        (VFlag || (PIndex == -1) || (PIndex == mcoUseMachineSpeed)))
      {
      MStringAppend(String,
        MUShowString(
          MParam[mcoUseMachineSpeed],
          (char *)MBool[bmisset(&P->Flags,mpfUseMachineSpeed)]));
      }

    if ((P->RsvNAllocPolicy != mnalNONE) ||
        (VFlag || (PIndex == -1) || (PIndex == mcoRsvNodeAllocPolicy)))
      {
      MStringAppend(String,
        MUShowString(
          MParam[mcoRsvNodeAllocPolicy],
          (char *)MNAllocPolicy[P->RsvNAllocPolicy]));
      }

    if ((P->RsvP != NULL) ||
        (VFlag || (PIndex == -1) || (PIndex == mcoRsvNodeAllocPriorityF)))
      {
      mstring_t tmp(MMAX_LINE);

      MPrioFToMString(P->RsvP,&tmp);

      MStringAppend(String,
        MUShowString(
          MParam[mcoRsvNodeAllocPriorityF],
          tmp.c_str()));
      }

    if ((MSched.UseNodeIndex == TRUE) ||
        (VFlag || (PIndex == -1) || (PIndex == mcoUseNodeHash)))
      {
      MStringAppend(String,
        MUShowString(
          MParam[mcoUseNodeHash],
          (char *)MBool[MSched.UseNodeIndex]));
      }

    if ((!bmisset(&P->Flags,mpfUseSystemQueueTime)) ||
        (VFlag || (PIndex == -1) || (PIndex == mcoUseSystemQueueTime)))
      {
      MStringAppend(String,
        MUShowString(
          MParam[mcoUseSystemQueueTime],
          (char *)MBool[bmisset(&P->Flags,mpfUseSystemQueueTime)]));
      }

    if ((!bmisset(&P->Flags,mpfUseLocalMachinePriority)) ||
        (VFlag || (PIndex == -1) || (PIndex == mcoUseLocalMachinePriority)))
      {
      MStringAppend(String,
        MUShowString(
          MParam[mcoUseLocalMachinePriority],
          (char *)MBool[bmisset(&P->Flags,mpfUseLocalMachinePriority)]));
      }

    if ((bmisset(&P->Flags,mpfUseFloatingNodeLimits)) ||
        (VFlag || (PIndex == -1) || (PIndex == mcoUseFloatingNodeLimits)))
      {
      MStringAppend(String,
        MUShowString(
          MParam[mcoUseFloatingNodeLimits],
          (char *)MBool[bmisset(&P->Flags,mpfUseFloatingNodeLimits)]));
      }

    if ((P->UntrackedProcFactor > 0.0) ||
        (VFlag || (PIndex == -1) || (PIndex == mcoNodeUntrackedProcFactor)))
      {
      MStringAppendF(String,"%-30s  %.1f\n",
        MParam[mcoNodeUntrackedProcFactor],
        P->UntrackedProcFactor);
      }
    }    /* END if (P->Index == 0) */

  if (P->Index == 0)
    {
    /* job nodematchpolicy only enabled for global partition */

    if ((!bmisclear(&P->JobNodeMatchBM)) ||
        (VFlag || (PIndex == -1) || (PIndex == mcoJobNodeMatch)))
      {
      char tmpLine[MMAX_LINE];

      bmtostring(&MPar[0].JobNodeMatchBM,MJobNodeMatchType,tmpLine,",",NONE);

      MStringAppendF(String,"%-30s  %s\n",
        MParam[mcoJobNodeMatch],
        tmpLine);
      }
    }

  if ((P->MaxJobStartTime != MMAX_TIME) ||
      (VFlag || (PIndex == -1) || (PIndex == mcoMaxJobStartTime)))
    {
    MULToTString(P->MaxJobStartTime,TString);

    MStringAppend(String,
      MUShowSArray(
        MParam[mcoMaxJobStartTime],
        P->Index,
        TString));

    MStringAppend(String,"\n");
    }

  /* system max policies */

  if ((P->MaxMetaTasks != MDEF_MAXMETATASKS) || 
      (VFlag || 
      (PIndex == -1) || 
      (PIndex == mcoMaxMetaTasks)))
    {
    MStringAppend(String,
      MUShowIArray(MParam[mcoMaxMetaTasks],P->Index,P->MaxMetaTasks));
    }

  /* node set policies */

  if ((P->NodeSetPolicy != mrssNONE) || (VFlag || (PIndex == -1) ||
      (PIndex == mcoNodeSetPolicy)))
    {
    MStringAppend(String,
      MUShowSArray(
        MParam[mcoNodeSetPolicy],
        P->Index,
        (char *)MResSetSelectionType[P->NodeSetPolicy]));
    }

  if ((P->NodeSetAttribute != mrstNONE) || (VFlag || (PIndex == -1) ||
      (PIndex == mcoNodeSetAttribute)))
    {
    MStringAppend(String,
      MUShowSArray(
        MParam[mcoNodeSetAttribute],
        P->Index,
        (char *)MResSetAttrType[P->NodeSetAttribute]));
    }

  if ((P->NodeSetList[0] != NULL) || (VFlag || (PIndex == -1) || 
      (PIndex == mcoNodeSetList)))
    {
    char tmpLine[MMAX_LINE];
    int  index;

    char *tBPtr;
    int   tBSpace;

    MUSNInit(&tBPtr,&tBSpace,tmpLine,sizeof(tmpLine));

    for (index = 0;P->NodeSetList[index] != NULL;index++)
      {
      MUSNPrintF(&tBPtr,&tBSpace,"%s%s",
        (index > 0) ? "," : "",
        P->NodeSetList[index]);
      }  /* END for (index) */

    MStringAppend(String,
      MUShowSArray(
        MParam[mcoNodeSetList],
        P->Index,
        tmpLine));
    }

  if ((P->NodeSetDelay != MDEF_NSDELAY) || (VFlag || (PIndex == -1) ||
      (PIndex == mcoNodeSetDelay)))
    {
    MULToTString(P->NodeSetDelay,TString);

    MStringAppend(String,
      MUShowSArray(
        MParam[mcoNodeSetDelay],
        P->Index,
        (char *)TString));
    }

  if (VFlag || (PIndex == -1) ||
     (PIndex == mcoNodeSetIsOptional))
    {
    mbool_t IsOptional;

    IsOptional = (P->NodeSetIsOptional != 0) ? TRUE : FALSE;

    MStringAppend(String,
      MUShowSArray(
        MParam[mcoNodeSetIsOptional],
        P->Index,
        (char *)MBool[IsOptional]));
    }

  if ((P->NodeSetMaxUsage > 0.0) || 
      (P->NodeSetMaxUsageSet == TRUE) ||
      (VFlag || (PIndex == -1) ||
       (PIndex == mcoNodeSetMaxUsage)))
    {
    MStringAppend(String,
      MUShowDouble(MParam[mcoNodeSetMaxUsage],P->NodeSetMaxUsage));

    if (P->NodeSetMaxUsageSet == TRUE)
      {
      int NSIndex = 0;

      for (NSIndex = 0;P->NodeSetList[NSIndex] != NULL;NSIndex++)
        {
        if (P->NodeSetListMaxUsage[NSIndex] > 0.0)
          {
          char MaxUsage[MMAX_LINE];

          sprintf(MaxUsage, "%s[%s]",
            MParam[mcoNodeSetMaxUsage],
            P->NodeSetList[NSIndex]);

          MStringAppend(String,
            MUShowDouble(MaxUsage,P->NodeSetListMaxUsage[NSIndex]));
          }
        }
      }
    } /* END if ((P->NodeSetMaxUsage > 0.0) ... */

  if ((P->NodeSetPriorityType != mrspNONE) || (VFlag || (PIndex == -1) ||
      (PIndex == mcoNodeSetPriorityType)))
    {
    MStringAppend(String,
      MUShowSArray(
        MParam[mcoNodeSetPriorityType],
        P->Index,
        (char *)MResSetPrioType[P->NodeSetPriorityType]));
    }

  if ((P->NodeSetTolerance != MDEF_NSTOLERANCE) || (VFlag || (PIndex == -1) ||
      (PIndex == mcoNodeSetTolerance)))
    {
    MStringAppend(String,
      MUShowFArray(
        MParam[mcoNodeSetTolerance],
        P->Index,
        P->NodeSetTolerance));
    }

  if ((P->WattChargeRate > 0) || (VFlag || (PIndex == -1) ||
      (PIndex == mcoWattChargeRate)))
    {
    MStringAppend(String,
      MUShowDouble(
        MParam[mcoWattChargeRate],P->NodeSetTolerance));
    }

  if (P->O != NULL)
    {
    char tmp[MMAX_NAME << 2];

    snprintf(tmp,sizeof(tmp),"%s:%s",
      MXO[P->OType],
      P->OID);

    MStringAppend(String,
      MUShowSArray(
        MParam[mcoParAdmin],
        P->Index,
        tmp));
    }

  /* node allocation policy */

  if ((PIndex == mcoNodeAllocationPolicy) || VFlag || (PIndex == -1))
    {
    char valueLine[MMAX_LINE];

    string nallocPolicy = (char *)MNAllocPolicy[P->NAllocPolicy];
    if ((P->NAllocPolicy == mnalPlugin) &&
        (P->NodeAllocationPlugin != NULL))
      {
      nallocPolicy += string(":") + P->NodeAllocationPlugin->FileName();
      }

    MUShowSSArray(
      MParam[mcoNodeAllocationPolicy],
      P->Name,
      nallocPolicy.c_str(),
      valueLine,
      MMAX_LINE);
    
    MStringAppend(String,valueLine);
    }

  if (P->Index == 0)
    {
    mfsc_t *F;

    int     index;

    F = &P->FSC;

    /* show master partition config */

    MStringAppend(String,"\n");

    /* backfill policies */

    if ((P->BFPolicy != mbfNONE) ||
        (VFlag || (PIndex == -1) || (PIndex == mcoBFPolicy)))
      {
      if ((PIndex == -1) || (PIndex == mcoBFPolicy) || VFlag)
        {
        MStringAppend(String,
          MUShowString(MParam[mcoBFPolicy],(char *)MBFPolicy[P->BFPolicy]));
        }

      if ((P->BFDepth != MDEF_BFDEPTH) ||
          (VFlag || (PIndex == -1) || (PIndex == mcoBFDepth)))
        {
        MStringAppend(String,
          MUShowInt(MParam[mcoBFDepth],P->BFDepth));
        }

      if ((P->BFProcFactor > 0) ||
          (VFlag || (PIndex == -1) || (PIndex == mcoBFProcFactor)))
        {
        MStringAppend(String,
          MUShowInt(MParam[mcoBFProcFactor],P->BFProcFactor));
        }

      if ((P->BFMaxSchedules != MDEF_BFMAXSCHEDULES) ||
          (VFlag || (PIndex == -1) || (PIndex == mcoBFMaxSchedules)))
        {
        MStringAppend(String,
          MUShowInt(MParam[mcoBFMaxSchedules],P->BFMaxSchedules));
        }

      if ((PIndex == -1) || (PIndex == mcoBFMetric) || VFlag)
        {
        MStringAppend(String,
          MUShowString(MParam[mcoBFMetric],(char *)MBFMPolicy[P->BFMetric]));
        }

      if ((PIndex == -1) || (PIndex == mcoBFMinVirtualWallTime) || VFlag)
        {
        MStringAppend(String,
          MUShowLong(
            MParam[mcoBFMinVirtualWallTime],
            P->MinVirtualWallTime));
        }

      if ((PIndex == -1) || 
          (PIndex == mcoBFVirtualWallTimeScalingFactor) || VFlag)
        {
        MStringAppend(String,
          MUShowDouble(
            MParam[mcoBFVirtualWallTimeScalingFactor],
            P->VirtualWallTimeScalingFactor));
        }

      if (PIndex == -1)
        MStringAppend(String,"\n");
      }  /* END if ((P->BFPolicy != mbfNONE) || ... ) */

    if (VFlag || (PIndex == -1) || (PIndex == mcoJobRejectPolicy))
      {
      char tmpLine[MMAX_LINE];

      bmtostring(&MSched.JobRejectPolicyBM,MJobRejectPolicy,tmpLine);

      MStringAppend(String,
        MUShowSArray(
          MParam[mcoJobRejectPolicy],
          P->Index,
          tmpLine));
      }

    if (VFlag || (PIndex <= 0) || (PIndex == mcoPreemptPolicy))
      {
      MStringAppend(String,
        MUShowSArray(
          MParam[mcoPreemptPolicy],
          P->Index,
          (char *)MPreemptPolicy[MSched.PreemptPolicy]));
      }

    if (VFlag || (PIndex == -1) || (PIndex == mcoPreemptJobCount))
      {
      MStringAppend(String,
        MUShowIArray(
          MParam[mcoPreemptJobCount],
          P->Index,
          MSched.SingleJobMaxPreemptee));
      }

    if (VFlag || (PIndex == -1) || (PIndex == mcoPreemptSearchDepth))
      {
      MStringAppend(String,
        MUShowIArray(
          MParam[mcoPreemptSearchDepth],
          P->Index,
          MSched.SerialJobPreemptSearchDepth));
      }

    if (VFlag || (PIndex == -1) || (PIndex == mcoPreemptPrioJobSelectWeight))
      {
      MStringAppend(String,
        MUShowFArray(
          MParam[mcoPreemptPrioJobSelectWeight],
          P->Index,
          MSched.PreemptPrioWeight));
      }

    if (VFlag || (PIndex == -1) || (PIndex == mcoParAllocationPolicy))
      {
      MStringAppend(String,
        MUShowSArray(
          MParam[mcoParAllocationPolicy],
          P->Index,
          (char *)MPAllocPolicy[P->PAllocPolicy]));
      }

    if (VFlag || (PIndex == -1) || (PIndex == mcoStatProfCount))
      {
      MStringAppend(String,
        MUShowIArray(
          MParam[mcoStatProfCount],
          P->Index,
          MStat.P.MaxIStatCount));
      }

    if (VFlag || (PIndex == -1) || (PIndex == mcoStatProfDuration))
      {
      MULToTString(MStat.P.IStatDuration,TString);

      MStringAppend(String,
        MUShowSArray(
          MParam[mcoStatProfDuration],
          P->Index,
          TString));
      }

    if (VFlag || (PIndex == -1) || (PIndex == mcoStatProfGranularity))
      {
      MStringAppend(String,
        MUShowSArray(
          MParam[mcoStatProfGranularity],
          P->Index,
          "N/A"));
      }

    if (VFlag || (PIndex == -1) || (PIndex == mcoAdminMinSTime))
      {
      MULToTString(P->AdminMinSTime,TString);

      MStringAppend(String,
        MUShowSArray(
          MParam[mcoAdminMinSTime],
          P->Index,
          TString));
      }

    if ((PIndex == mcoResourceLimitPolicy) || VFlag || (PIndex == -1))
      {
      int  index;
      char tmpLine[MMAX_LINE];

      char tmpPolicy[MMAX_LINE];
      char tmpAction[MMAX_LINE];
      char tmpWC[MMAX_LINE];

      char tmpWCS[MMAX_NAME];
      char tmpWCH[MMAX_NAME];

      tmpLine[0] = '\0';

      for (index = 1;MResLimitType[index] != NULL;index++)
        {
        if (P->ResourceLimitPolicy[MRESUTLHARD][index] != mrlpNONE)
          {
          if (P->ResourceLimitMaxViolationTime[MRESUTLHARD][index] > 0)
            {
            MULToTString(P->ResourceLimitMaxViolationTime[MRESUTLSOFT][index],TString);
            MUStrCpy(
              tmpWCS,
              TString,
              sizeof(tmpWCS));

            MULToTString(P->ResourceLimitMaxViolationTime[MRESUTLHARD][index],TString);
            MUStrCpy(
              tmpWCH,
              TString,
              sizeof(tmpWCH));

            sprintf(tmpLine,"%s%s:%s:%s:%s ",
              tmpLine,
              MResLimitType[index],
              MSLimitToString(
                (char *)MResourceLimitPolicyType[P->ResourceLimitPolicy[MRESUTLSOFT][index]],
                (char *)MResourceLimitPolicyType[P->ResourceLimitPolicy[MRESUTLHARD][index]],
                tmpPolicy,
                sizeof(tmpPolicy)),
              MSLimitBMToString(
                MPolicyAction,
                &P->ResourceLimitViolationActionBM[MRESUTLSOFT][index],
                &P->ResourceLimitViolationActionBM[MRESUTLHARD][index],
                tmpAction,
                sizeof(tmpAction)),
              MSLimitToString(
                tmpWCS,
                tmpWCH,
                tmpWC,
                sizeof(tmpWC)));
            }
          else
            {
            sprintf(tmpLine,"%s%s:%s:%s ",
              tmpLine,
              MResLimitType[index],
              MSLimitToString(
                (char *)MResourceLimitPolicyType[P->ResourceLimitPolicy[MRESUTLSOFT][index]],
                (char *)MResourceLimitPolicyType[P->ResourceLimitPolicy[MRESUTLHARD][index]],
                tmpPolicy,
                sizeof(tmpPolicy)),
              MSLimitBMToString(
                MPolicyAction,
                &P->ResourceLimitViolationActionBM[MRESUTLSOFT][index],
                &P->ResourceLimitViolationActionBM[MRESUTLHARD][index],
                tmpAction,
                sizeof(tmpAction)));
            }
          }
        }    /* END for (index) */

      MStringAppend(String,
        MUShowSArray(
          MParam[mcoResourceLimitPolicy],
          P->Index,
          tmpLine));
      }  /* END if ((PIndex == mcoResourceLimitPolicy) || ...) */

    if ((PIndex == mcoNodeAvailPolicy) || VFlag || (PIndex == -1))
      {
      int  policyindex;
      int  rindex;

      char tmpLine[MMAX_LINE];

      tmpLine[0] = '\0';

      for (rindex = 0;MResourceType[rindex] != NULL;rindex++)
        {
        policyindex = P->NAvailPolicy[rindex];

        if (policyindex == 0)
          continue;

        if (tmpLine[0] != '\0')
          strcat(tmpLine," ");

        sprintf(tmpLine,"%s%s:%s",
          tmpLine,
          MNAvailPolicy[policyindex],
          (rindex == 0) ? DEFAULT : MResourceType[rindex]);
        }  /* END for (rindex) */

      MStringAppend(String,
        MUShowSArray(
          MParam[mcoNodeAvailPolicy],
          P->Index,
          tmpLine));
      }  /* END if ((PIndex == mcoNodeAvailPolicy) || ...) */

    if ((PIndex == mcoVPCNodeAllocationPolicy) || VFlag || (PIndex == -1))
      {
      MStringAppend(String,
        MUShowSArray(
          MParam[mcoVPCNodeAllocationPolicy],
          P->Index,
          (char *)MNAllocPolicy[P->VPCNAllocPolicy]));
      }

    /* task distribution policy */

    if (VFlag || (PIndex == -1) || (PIndex == mcoTaskDistributionPolicy))
      {
      MStringAppend(String,
        MUShowSArray(
          MParam[mcoTaskDistributionPolicy],
          P->Index,
          (char *)MTaskDistributionPolicy[P->DistPolicy]));
      }

    /* reservation policy */

    MStringAppend(String,
      MUShowSArray(
        MParam[mcoRsvPolicy],
        P->Index,
        (char *)MRsvPolicy[P->RsvPolicy]));

    for (index = 0;index < MMAX_QOSBUCKET;index++)
      {
      if (!VFlag &&
         ((P->QOSBucket[index].RsvDepth == MDEF_RSVDEPTH) ||
          (P->QOSBucket[index].RsvDepth == 0)) &&
          (P->QOSBucket[index].List[0] == (mqos_t *)MMAX_QOS))
        {
        continue;
        }

      if (VFlag || (PIndex == -1) || (PIndex == mcoRsvBucketDepth))
        {
        MStringAppend(String,
          MUShowIArray(
            MParam[mcoRsvBucketDepth],
            P->Index,
            P->QOSBucket[index].RsvDepth));
        }

      if (VFlag || (P->QOSBucket[index].List[0] != (mqos_t *)MMAX_QOS))
        {
        char tmpLine[MMAX_LINE];

        int  qindex;

        if (P->QOSBucket[index].List[0] == (mqos_t *)MMAX_QOS)
          {
          strcpy(tmpLine,ALL);
          }
        else
          {
          tmpLine[0] = '\0';

          for (qindex = 0;P->QOSBucket[index].List[qindex] != NULL;qindex++)
            {
            if (P->QOSBucket[index].List[qindex] == (mqos_t *)MMAX_QOS)
              {
              sprintf(tmpLine,"%s%s ",
                tmpLine,
                ALL);
              }
            else
              {
              sprintf(tmpLine,"%s%s ",
                tmpLine,
                P->QOSBucket[index].List[qindex]->Name);
              }
            }
          }

        if (VFlag || (PIndex == -1) || (PIndex == mcoRsvBucketQOSList))
          {
          MStringAppend(String,
            MUShowSArray(
              MParam[mcoRsvBucketQOSList],
              index,
              tmpLine));
          }
        }    /* END if (P->QOSBucket[index].List[0] != (mqos_t *)MMAX_QOS)) */
      }      /* END for (index) */

    if ((P->RsvRetryTime != 0) || 
        (VFlag || (PIndex == -1) || 
        (PIndex == mcoRsvRetryTime)))
      {
      MULToTString(P->RsvRetryTime,TString);

      MStringAppend(String,
        MUShowSArray(
          MParam[mcoRsvRetryTime],
          P->Index,
          TString));
      }

    if ((P->JobRetryTime != 0) ||
        (VFlag || (PIndex == -1) ||
        (PIndex == mcoJobRetryTime)))
      {
      MULToTString(P->JobRetryTime,TString);

      MStringAppendF(String,"%-30s  %s\n",
        MParam[mcoJobRetryTime],TString);

/* Uncomment when per partition JOBRETRYTIME is supported
      MULToTString(P->JobRetryTime,TString);

      MStringAppend(String,
        MUShowSArray(
          MParam[mcoJobRetryTime],
          P->Index,
          TString));
*/
      }

    if ((P->RsvTValue != 0) || 
        (VFlag || (PIndex == -1) || 
        (PIndex == mcoRsvThresholdValue)))
      {
      MStringAppend(String,
        MUShowSArray(
          MParam[mcoRsvThresholdType],
          P->Index,
          (char *)MRsvThresholdType[P->RsvTType]));

      MStringAppend(String,
        MUShowIArray(MParam[mcoRsvThresholdValue],P->Index,P->RsvTValue));
      }

    MStringAppend(String,"\n");

    /* FS policies */

    MStringAppendF(String,"%-30s  %s%s\n",
      MParam[mcoFSPolicy],MFSPolicyType[F->FSPolicy],
      (F->TargetIsAbsolute == TRUE) ? "%" : "");

    if ((F->FSPolicy == mfspNONE) || 
        (VFlag || (PIndex == -1) || (PIndex == mcoFSPolicy)))
      {
      MULToTString(F->FSInterval,TString);

      MStringAppendF(String,"%-30s  %s\n",
        MParam[mcoFSInterval],TString);

      MStringAppendF(String,"%-30s  %d\n",
        MParam[mcoFSDepth],F->FSDepth);

      MStringAppendF(String,"%-30s  %-6.2f\n",
        MParam[mcoFSDecay],F->FSDecay);

      MStringAppend(String,"\n");
      }

    MStringAppend(String,"\n");
    }  /* END if (P->Index == 0) */

  return(SUCCESS);
  }  /* END MParConfigShow() */




/**
 * Free memory and destroy partition.
 *
 * @param PP (I) [modified, contents freed]
 */

int MParDestroy(

  mpar_t **PP)  /* I (modified, contents freed) */

  {
  job_iter JTI;

  mpar_t *P;

  mjob_t *J;

  if ((PP == NULL) || (*PP == NULL))
    {
    return(FAILURE);
    }

  P = *PP;

  /* free dynamic attributes */

  MUFree(&P->Message);

  MUFree((char **)&P->CList);

  MUFree((char **)&P->L.IdlePolicy);
  MUFree((char **)&P->L.JobPolicy);

  MUFree((char **)&P->QOSBucket);

  MNPrioFree(&P->RsvP);

  if (P->L.JobMaximums[0] != NULL)
    MJobDestroy(&P->L.JobMaximums[0]);

  if (P->L.JobMinimums[0] != NULL)
    MJobDestroy(&P->L.JobMinimums[0]);

  MUFree((char **)&P->S.XLoad);

  if (P->S.IStat != NULL)
    {
    MStatPDestroy((void ***)&P->S.IStat,msoCred,MStat.P.MaxIStatCount);
    }

  if (P->FSC.ShareTree != NULL)
    {
    MFSTreeFree(&P->FSC.ShareTree);
    }

  /* clear partition name from all PAL's */

  MJobIterInit(&JTI);

  while (MJobTableIterate(&JTI,&J) == SUCCESS)
    {
    bmunset(&J->PAL,P->Index);
    bmunset(&J->CurrentPAL,P->Index);
    bmunset(&J->SpecPAL,P->Index);
    bmunset(&J->SysPAL,P->Index);
    }

  if (P->NodeAllocationPlugin != NULL)
    {
    delete P->NodeAllocationPlugin;
    P->NodeAllocationPlugin = NULL;
    }

  P->Name[0] = '\1';

  return(SUCCESS);
  }  /* END MParDestroy() */





/**
 */

int MParFreeTable()

  {
  int pindex;

  mpar_t *P;

  for (pindex = 0;pindex < MMAX_PAR;pindex++)
    {
    P = &MPar[pindex];

    if (P->Name[0] == '\0')
      break;

    if (P->Name[0] == '\1')
      continue;

    MParDestroy(&P);
    }  /* END for (pindex) */

  return(SUCCESS);
  }  /* END MParFreeTable() */




    
/**
 * Set specified partition attribute.
 *
 * @see MParAToString() - peer
 *
 * @param P (I) [modified]
 * @param AIndex (I)
 * @param Value (I)
 * @param Format (I)
 * @param Mode (I)
 */

int MParSetAttr(

  mpar_t                 *P,      /* I * (modified) */
  enum MParAttrEnum       AIndex, /* I */
  void                  **Value,  /* I */
  enum MDataFormatEnum    Format, /* I */
  enum MObjectSetModeEnum Mode)   /* I */

  {
  mjob_t *J;
  mreq_t *RQ;

  const char *FName = "MParSetAttr";

  MDB(6,fSTRUCT) MLog("%s(%s,%s,Value,Format,%u)\n",
    FName,
    (P != NULL) ? P->Name : "NULL",
    MParAttr[AIndex],
    Mode);

  if (P == NULL)
    {
    return(FAILURE);
    }

  switch (AIndex)
    {
    case mpaACL:

      if (P->IsVPC == TRUE)
        {
        /* modify CL (cred list) - NOT ACL - for master rsv */

        marray_t RList;

        MUArrayListCreate(&RList,sizeof(mrsv_t *),10);

        if ((MRsvGroupGetList(P->RsvGroup,&RList,NULL,0) == SUCCESS) &&
            (RList.NumItems > 0))
          {
          mrsv_t *R = (mrsv_t *)MUArrayListGetPtr(&RList,0);

          MRsvSetAttr(R,mraCL,(void *)Value,Format,Mode);
          }

        MUArrayListFree(&RList);
        }

      break;

    case mpaJobActionOnNodeFailure:
    case mpaAllocResFailPolicy:

      P->ARFPolicy = (enum MAllocResFailurePolicyEnum)MUGetIndexCI(
        (char *)Value,
        MARFPolicy,
        FALSE,
        marfNONE);

      break;

    case mpaCmdLine:

      MUStrDup(&P->MShowCmd,(char *)Value);

      break;

    case mpaConfigFile:

      MUStrDup(&P->ConfigFile,(char *)Value);

      MParLoadConfigFile(P,NULL,NULL);

      break;

    case mpaCost:

      P->Cost = strtod((char *)Value,NULL);

      break;

    case mpaDedSRes:

      /* FORMAT:  <ATTR>[,<ATTR>]... */

      {
      char *ptr;
      char *TokPtr = NULL;

      int aIndex;
      
      ptr = MUStrTok((char *)Value,",",&TokPtr);

      while (ptr != NULL)
        {
        aIndex = (enum MResourceTypeEnum)MUGetIndexCI(
          ptr,
          (const char **)MResourceType,
          FALSE,
          (int)mrNONE);

        P->SuspendRes[aIndex] = TRUE;
      
        ptr = MUStrTok(NULL,",",&TokPtr);
        }  /* END while (ptr != NULL) */
      }    /* END BLOCK (case mpaDedSRes) */ 

      break;

    case mpaDefCPULimit:

      {
      mulong tmpL;

      if ((P->L.JobDefaults == NULL) &&
          (MJobAllocBase(&P->L.JobDefaults,&RQ) == FAILURE))
        {
        MDB(5,fCONFIG) MLog("ALERT:    cannot alloc job structure in %s\n",
          FName);

        break;
        }

      J = P->L.JobDefaults;

      if (Format == mdfString)
        tmpL = MUTimeFromString((char *)Value);
      else
        tmpL = *(long *)Value;
 
      J->CPULimit = tmpL;
      }  /* END BLOCK (case mpaDefCPULimit) */

      break;

    case mpaDefaultNodeActiveWatts:

      if (Format == mdfString)
        P->DefNodeActiveWatts = strtod((char *)Value,NULL);
      else
        P->DefNodeActiveWatts = *(double *)Value;
        
      break;

    case mpaDefaultNodeIdleWatts:

      if (Format == mdfString)
        P->DefNodeIdleWatts = strtod((char *)Value,NULL);
      else
        P->DefNodeIdleWatts = *(double *)Value;

      break;

    case mpaDefaultNodeStandbyWatts:

      if (Format == mdfString)
        P->DefNodeStandbyWatts = strtod((char *)Value,NULL);
      else
        P->DefNodeStandbyWatts = *(double *)Value;
 
      break;

    case mpaDuration:

      if (Format == mdfString)
        {     
        P->ReqDuration = strtol((char *)Value,NULL,0);
        }
      else
        {
        P->ReqDuration = *(mulong *)Value;
        }

      break;

    case mpaFlags:

      if (Format == mdfString)
        {
        bmfromstring((char *)Value,MParFlags,&P->Flags);
        }

      break;

    case mpaID:

      /* NYI */

      break;

    case mpaJobNodeMatchPolicy:

      bmclear(&P->JobNodeMatchBM);

      {
      int index;

      char *ptr;
      char *TokPtr = NULL;

      ptr = MUStrTok((char *)Value,",:",&TokPtr);

      while (ptr != NULL)
        {
        for (index = 0;MJobNodeMatchType[index] != NULL;index++)
          {
          if (strstr(ptr,MJobNodeMatchType[index]))
            bmset(&P->JobNodeMatchBM,index);
          }

        ptr = MUStrTok(NULL,",:",&TokPtr);
        }  /* END for (index) */
      }    /* END BLOCK */

      break;

    case mpaMaxCPULimit:

      {
      mulong tmpL;

      if ((P->L.JobMaximums[0] == NULL) &&
          (MJobAllocBase(&P->L.JobMaximums[0],&RQ) == FAILURE))
        {
        MDB(5,fCONFIG) MLog("ALERT:    cannot alloc job structure in %s\n",
          FName);

        break;
        }

      J = P->L.JobMaximums[0];

      if (Format == mdfString)
        tmpL = MUTimeFromString((char *)Value);
      else
        tmpL = *(long *)Value;
 
      J->CPULimit = tmpL;
      }  /* END BLOCK (case mpaMaxCPULimit) */

      break;

    case mpaMaxProc:

      {
      if ((P->L.JobMaximums[0] == NULL) &&
          (MJobAllocBase(&P->L.JobMaximums[0],&RQ) == FAILURE))
        {
        MDB(5,fCONFIG) MLog("ALERT:    cannot alloc job structure in %s\n",
          FName);

        break;
        }

      J = P->L.JobMaximums[0];
      RQ = J->Req[0];

      if (Format == mdfString)
        J->Request.TC = (int)strtol((char *)Value,NULL,10);
      else
        J->Request.TC = *(int *)Value;

      bmset(&J->IFlags,mjifIsTemplate);

      RQ->DRes.Procs = 1;
      RQ->TaskCount  = J->Request.TC;
      }  /* END BLOCK (case mpaMaxProc) */

      break;

    case mpaMaxPS:

      {
      if ((P->L.JobMaximums[0] == NULL) &&
          (MJobAllocBase(&P->L.JobMaximums[0],&RQ) == FAILURE))
        {
        MDB(5,fCONFIG) MLog("ALERT:    cannot alloc job structure in %s\n",
          FName);

        break;
        }

      J = P->L.JobMaximums[0];

      if (Format == mdfString)
        J->PSDedicated = strtod((char *)Value,NULL);
      else
        J->PSDedicated = *(double *)Value;

      bmset(&J->IFlags,mjifIsTemplate);
      }  /* END BLOCK (case mpaMaxPS) */

      break;

    case mpaMaxNode:

      {
      if ((P->L.JobMaximums[0] == NULL) &&
          (MJobAllocBase(&P->L.JobMaximums[0],&RQ) == FAILURE))
        {
        MDB(5,fCONFIG) MLog("ALERT:    cannot alloc job structure in %s\n",
          FName);

        break;
        }

      J = P->L.JobMaximums[0];
      RQ = J->Req[0];

      bmset(&J->IFlags,mjifIsTemplate);

      if (Format == mdfString)
        J->Request.NC = (int)strtol((char *)Value,NULL,10);
      else
        J->Request.NC = *(int *)Value;

      RQ->NodeCount = J->Request.NC;
      }  /* END BLOCK */

      break;

    case mpaMaxWCLimit:

      {
      mulong tmpL;

      /* FORMAT:  <STRING> */

      if ((P->L.JobMaximums[0] == NULL) &&
          (MJobAllocBase(&P->L.JobMaximums[0],&RQ) == FAILURE))
        {
        MDB(5,fCONFIG) MLog("ALERT:    cannot alloc job structure in %s\n",
          FName);

        break;
        }

      J = P->L.JobMaximums[0];

      bmset(&J->IFlags,mjifIsTemplate);

      if (Format == mdfString)
        tmpL = MUTimeFromString((char *)Value);
      else
        tmpL = *(long *)Value;

      J->WCLimit        = tmpL;
      J->SpecWCLimit[0] = tmpL;
      }  /* END BLOCK (case mpaMaxWC) */

      break;

    case mpaMinProc:

      {
      if ((P->L.JobMinimums[0] == NULL) &&
          (MJobAllocBase(&P->L.JobMinimums[0],&RQ) == FAILURE))
        {
        MDB(5,fCONFIG) MLog("ALERT:    cannot alloc job structure in %s\n",
          FName);

        break;
        }

      J = P->L.JobMinimums[0];
      RQ = J->Req[0];

      if (Format == mdfString)
        J->Request.TC = (int)strtol((char *)Value,NULL,10);
      else
        J->Request.TC = *(int *)Value;

      bmset(&J->IFlags,mjifIsTemplate);

      RQ->DRes.Procs = 1;
      RQ->TaskCount  = J->Request.TC;
      }  /* END BLOCK (case mpaMinProc) */

      break;

    case mpaMinNode:

      {
      if ((P->L.JobMinimums[0] == NULL) &&
          (MJobAllocBase(&P->L.JobMinimums[0],&RQ) == FAILURE))
        {
        MDB(5,fCONFIG) MLog("ALERT:    cannot alloc job structure in %s\n",
          FName);

        break;
        }

      J = P->L.JobMinimums[0];
      RQ = J->Req[0];

      bmset(&J->IFlags,mjifIsTemplate);

      if (Format == mdfString)
        J->Request.NC = (int)strtol((char *)Value,NULL,10);
      else
        J->Request.NC = *(int *)Value;

      RQ->NodeCount = J->Request.NC;
      }  /* END BLOCK */

      break;

    case mpaMinWCLimit:

      {
      mulong tmpL;

      /* FORMAT:  <STRING> */

      if ((P->L.JobMinimums[0] == NULL) &&
          (MJobAllocBase(&P->L.JobMinimums[0],&RQ) == FAILURE))
        {
        MDB(5,fCONFIG) MLog("ALERT:    cannot alloc job structure in %s\n",
          FName);

        break;
        }

      J = P->L.JobMinimums[0];

      bmset(&J->IFlags,mjifIsTemplate);

      if (Format == mdfString)
        tmpL = MUTimeFromString((char *)Value);
      else
        tmpL = *(long *)Value;

      J->WCLimit        = tmpL;
      J->SpecWCLimit[0] = tmpL;
      }  /* END BLOCK (case mpaMinWC) */

      break;

    case mpaNodeAccessPolicy:

      P->NAccessPolicy = (enum MNodeAccessPolicyEnum)MUGetIndexCI((char *)Value,MNAccessPolicy,FALSE,mnacNONE);

      break;

    case mpaNodeAllocPolicy:

      if (__MParSetNodeAllocationPolicy(P,(char *)Value) == FAILURE)
        return(FAILURE);

      break;

    case mpaNodeChargeRate:

      P->DefNodeChargeRate = strtod((char *)Value,NULL);

      break;

    case mpaNodePowerOffDuration:

      P->NodePowerOffDuration = MUTimeFromString((char *)Value);

      break;

    case mpaNodePowerOnDuration:

      P->NodePowerOnDuration = MUTimeFromString((char *)Value);

      break;

    case mpaOwner:

      {
      char OName[MMAX_NAME << 2];

      char *ptr;
      char *TokPtr;

      enum MXMLOTypeEnum OType = mxoNONE;

      void *O = NULL;

      /* FORMAT:  <otype>:<oid> */

      MUStrCpy(OName,(char *)Value,sizeof(OName));

      ptr = MUStrTok(OName,":",&TokPtr);

      OType = (enum MXMLOTypeEnum)MUGetIndexCI(ptr,MXO,FALSE,mxoNONE);

      if (OType == mxoNONE)
        {
        return(FAILURE);
        }

      ptr = MUStrTok(NULL," \n\t",&TokPtr);

      if ((ptr == NULL) || (ptr[0] == '\0'))
        {
        return(FAILURE);
        }

      if (MOGetObject(OType,ptr,&O,mVerify) == FAILURE)
        {
        return(FAILURE);
        }

      P->OType = OType;

      P->O = O;

      MUStrCpy(P->OID,ptr,sizeof(P->OID));

      MUStrCpy(OName,(char *)Value,sizeof(OName));

      if (P->IsVPC == TRUE)
        {
        marray_t RList;

        MUArrayListCreate(&RList,sizeof(mrsv_t *),10);

        if ((MRsvGroupGetList(P->RsvGroup,&RList,NULL,0) == SUCCESS) &&
            (RList.NumItems > 0))
          {
          int rindex;

          for (rindex = 0;rindex < RList.NumItems;rindex++)
            {
            mrsv_t *R = (mrsv_t *)MUArrayListGetPtr(&RList,rindex);
     
            MRsvSetAttr(R,mraOwner,OName,Format,mSet);
            }
          }

        MUArrayListFree(&RList);
        }
      }  /* END BLOCK (case mpaOwner) */

      break;

    case mpaPriority:

      P->Priority = strtol((char *)Value,NULL,10);

      break;

    case mpaProfile:

      MVPCProfileFind((char *)Value,&P->VPCProfile);

      break;

    case mpaProvEndTime:

      if (Format == mdfString)
        {     
        P->ProvEndTime = strtol((char *)Value,NULL,0);
        }
      else
        {
        P->ProvEndTime = *(mulong *)Value;
        }

      break;

    case mpaProvStartTime:

      if (Format == mdfString)
        {     
        P->ProvStartTime = strtol((char *)Value,NULL,0);
        }
      else
        {
        P->ProvStartTime = *(mulong *)Value;
        }

      break;

    case mpaPurgeTime:

      if (Format == mdfString)
        {     
        P->PurgeTime = strtol((char *)Value,NULL,10);
        }
      else
        {
        P->PurgeTime = *(mulong *)Value;
        }

      break;

    case mpaReqOS:

      if (Format == mdfString)
        P->ReqOS = MUMAGetIndex(meOpsys,(char *)Value,mAdd);

      break;

    case mpaReqResources:

      /* NOTE:  P->ReqResources is human readable string describing *
                resources explicitly requested by user */

      MUStrDup(&P->ReqResources,(char *)Value);

      break;

    case mpaReservationDepth:

      if (Value != NULL)
        P->ParBucketRsvDepth = strtol((char *)Value,NULL,10);

      break;

    case mpaRsvGroup:

      /* NOTE:  P->Resource is generally rsv group id */

      MUStrDup(&P->RsvGroup,(char *)Value);

      break;

    case mpaRsvNodeAllocPolicy:

      P->RsvNAllocPolicy = (enum MNodeAllocPolicyEnum)MUGetIndexCI(
        (char *)Value,
        MNAllocPolicy,
        FALSE,
        P->NAllocPolicy);

      break;

    case mpaRsvNodeAllocPriorityF:

      MNPrioFree(&P->RsvP);
      MProcessNAPrioF(&P->RsvP,(char *)Value);

      break;

    case mpaRM:

      if (Format == mdfString)
        {
        MRMAdd((char *)Value,&P->RM);
        }

      break;

    case mpaState:

      P->State = (enum MClusterStateEnum)MUGetIndex((char *)Value,MClusterState,FALSE,0);

      break;

    case mpaStartTime:

      if (Format == mdfString)
        {     
        P->ReqStartTime = strtol((char *)Value,NULL,0);
        }
      else
        {
        P->ReqStartTime = *(mulong *)Value;
        }

      break;

    case mpaUseTTC:

      if (MUBoolFromString((char *)Value,FALSE) == TRUE)
        bmset(&P->Flags,mpfUseTTC);
      else
        bmunset(&P->Flags,mpfUseTTC);

      break;

    case mpaUser:

      {
      mgcred_t *U = NULL;

      if (MUserFind((char *)Value,&U) == FAILURE)
        {
        return(FAILURE);
        }
      else
        {
        P->U = U;
        }

      if (P->IsVPC == TRUE)
        {
        marray_t RList;

        MUArrayListCreate(&RList,sizeof(mrsv_t *),10);

        if ((MRsvGroupGetList(P->RsvGroup,&RList,NULL,0) == SUCCESS) &&
            (RList.NumItems > 0))
          {
          int rindex;

          for (rindex = 0;rindex < RList.NumItems;rindex++)
            {
            mrsv_t *R = (mrsv_t *)MUArrayListGetPtr(&RList,rindex);
     
            MRsvSetAttr(R,mraAUser,(void *)Value,Format,mSet);
            }
          }

        MUArrayListFree(&RList);
        }
      }    /* END case mpaUser */

      break;

    case mpaGroup:

      {
      mgcred_t *G = NULL;

      if (MGroupFind((char *)Value,&G) == FAILURE)
        {
        return(FAILURE);
        }
      else
        {
        P->G = G;
        }

      if (P->IsVPC == TRUE)
        {
        marray_t RList;

        MUArrayListCreate(&RList,sizeof(mrsv_t *),10);

        if ((MRsvGroupGetList(P->RsvGroup,&RList,NULL,0) == SUCCESS) &&
            (RList.NumItems > 0))
          {
          int rindex;

          for (rindex = 0;rindex < RList.NumItems;rindex++)
            {
            mrsv_t *R = (mrsv_t *)MUArrayListGetPtr(&RList,rindex);
     
            MRsvSetAttr(R,mraAGroup,(void *)Value,Format,mSet);
            }
          }

        MUArrayListFree(&RList);
        }
      }  /* END case mpaGroup */

      break;

    case mpaAcct:

      {
      mgcred_t *A = NULL;

      if (MAcctFind((char *)Value,&A) == FAILURE)
        {
        return(FAILURE);
        }
      else
        {
        P->A = A;
        }

      if (P->IsVPC == TRUE)
        {
        marray_t RList;

        MUArrayListCreate(&RList,sizeof(mrsv_t *),10);

        if ((MRsvGroupGetList(P->RsvGroup,&RList,NULL,0) == SUCCESS) &&
            (RList.NumItems > 0))
          {
          int rindex;

          for (rindex = 0;rindex < RList.NumItems;rindex++)
            {
            mrsv_t *R = (mrsv_t *)MUArrayListGetPtr(&RList,rindex);
     
            MRsvSetAttr(R,mraAAccount,(void *)Value,Format,mSet);
            }
          }

        MUArrayListFree(&RList);
        }
      }  /* END case mpaAcct */

      break;

    case mpaQOS:

      {
      mqos_t *Q = NULL;

      if (MQOSFind((char *)Value,&Q) == FAILURE)
        {
        return(FAILURE);
        }
      else
        {
        P->Q = Q;
        }

      if (P->IsVPC == TRUE)
        {
        marray_t RList;

        MUArrayListCreate(&RList,sizeof(mrsv_t *),10);

        if ((MRsvGroupGetList(P->RsvGroup,&RList,NULL,0) == SUCCESS) &&
            (RList.NumItems > 0))
          {
          int rindex;

          for (rindex = 0;rindex < RList.NumItems;rindex++)
            {
            mrsv_t *R = (mrsv_t *)MUArrayListGetPtr(&RList,rindex);
     
            MRsvSetAttr(R,mraAQOS,(void *)Value,Format,mSet);
            }
          }

        MUArrayListFree(&RList);
        }
      }  /* END case mpaQOS */

      break;

    case mpaVariables:

      if (P->IsVPC == TRUE)
        {
        marray_t RList;

        /* extract variables from rsv group leader */

        MUArrayListCreate(&RList,sizeof(mrsv_t *),10);

        if ((MRsvGroupGetList(P->RsvGroup,&RList,NULL,0) == SUCCESS) &&
            (RList.NumItems > 0))
          {
          mrsv_t *R = (mrsv_t *)MUArrayListGetPtr(&RList,0);

          MRsvSetAttr(R,mraVariables,Value,Format,Mode);
          } 

        MUArrayListFree(&RList);
        }  /* END if (P->IsVPC == TRUE) */

      break;

    case mpaVMCreationDuration:

      P->VMCreationDuration = MUTimeFromString((char *)Value);

      break;

    case mpaVMDeletionDuration:

      P->VMDeletionDuration = MUTimeFromString((char *)Value);

      break;

    case mpaVMMigrationDuration:

      P->VMMigrationDuration = MUTimeFromString((char *)Value);

      break;

    case mpaVPCFlags:

      if (Format == mdfString)
        P->VPCFlags = strtol((char *)Value,NULL,10);
      else if (Format == mdfLong)
        P->VPCFlags = *(long *)Value;

      break;

    case mpaVPCNodeAllocPolicy:

      P->VPCNAllocPolicy = (enum MNodeAllocPolicyEnum)MUGetIndexCI(
        (char *)Value,
        MNAllocPolicy,
        FALSE,
        P->VPCNAllocPolicy);

      /* sync GLOBAL and DEFAULT partitions */

      if ((P->Index == 0) &&
         ((MPar[1].VPCNAllocPolicy == mnalNONE) ||
          (MPar[1].VPCNAllocPolicy == MDEF_NALLOCPOLICY)))
        {
        MPar[1].VPCNAllocPolicy = P->VPCNAllocPolicy;
        }
      else if ((P->Index == 1) &&
              ((MPar[0].VPCNAllocPolicy == mnalNONE) ||
               (MPar[0].VPCNAllocPolicy == MDEF_NALLOCPOLICY)))
        {
        MPar[0].VPCNAllocPolicy = P->VPCNAllocPolicy;
        }

      break;

    case mpaWattChargeRate:

      P->WattChargeRate = strtod((char *)Value,NULL);
 
      break;

    default:

      /* NO-OP */

      break;
    }  /* END switch (AIndex) */
  
  return(SUCCESS);
  }  /* END MParSetAttr() */





/**
 * Convert partition attribute to string.
 *
 * @see MParSetAttr() - peer
 * @see MVPCToXML() - parent
 *
 * @param P (I)
 * @param AIndex (I)
 * @param String (O)
 * @param DModeBM (I) [bitmap of enum mcm*]
 */

int MParAToMString(

  mpar_t            *P,         /* I */
  enum MParAttrEnum  AIndex,    /* I */
  mstring_t         *String,    /* O */
  int                DModeBM)   /* I (bitmap of enum mcm*) */

  {
  if (String == NULL)
    {
    return(FAILURE);
    }

  MStringSet(String,"\0");

  if (P == NULL)
    {
    return(FAILURE);
    }

  switch (AIndex)
    {
    case mpaAcct:

      {
      mrsv_t *R;

      if (P->VPCMasterRsv != NULL)
        break;

      R = P->VPCMasterRsv;

      if ((R == (mrsv_t *)1) || (R == NULL))
        break;

      if ((P->RsvGroup == NULL) || strcmp(R->Name,P->RsvGroup))
        break;

      if (R->A != NULL)
        MStringSet(String,R->A->Name);
      }  /* END (case mpaAcct) */

      break;

    case mpaACL:

      if (P->IsVPC == TRUE)
        {
        /* report CL (cred list) - NOT ACL - for master rsv */

        marray_t RList;

        MUArrayListCreate(&RList,sizeof(mrsv_t *),10);

        if ((MRsvGroupGetList(P->RsvGroup,&RList,NULL,0) == SUCCESS) &&
            (RList.NumItems > 0))
          {
          mrsv_t *R = (mrsv_t *)MUArrayListGetPtr(&RList,0);

          MRsvAToMString(R,mraCL,String,DModeBM);
          } 

        MUArrayListFree(&RList);
        }

      break;

    case mpaAllocResFailPolicy:

      if (P->ARFPolicy != marfNONE)
        {
        MStringSetF(String,"%s",(char *)MARFPolicy[P->ARFPolicy]);
        }

      break;

    case mpaArch:

      if ((P->RepN != NULL) && (P->RepN->Arch != 0))
        {
        if (P->RepN->Index > MSched.M[mxoNode])
          {
          /* remove this if statement after ticket 3320 resolved */

          MDB(3,fSTRUCT) MLog("WARNING:  partition %s representative node set to corrupt value\n",
            (P != NULL) ? P->Name : "NULL");
          }

        MStringSetF(String,"%s",MAList[meArch][P->RepN->Arch]);
        }

      break;

    case mpaBacklogDuration:

      if (P->BacklogDuration > 0)
        {
        MStringAppendF(String,"%ld",
          P->BacklogDuration);
        }

      break;

    case mpaBacklogPS:

      if (P->BacklogPS > 0.0)
        {
        MStringAppendF(String,"%f",
          P->BacklogPS);
        }

      break;

    case mpaCmdLine:

      if (P->MShowCmd != NULL)
        {
        MStringAppendF(String,"%s",
          P->MShowCmd);
        }

      break;

    case mpaConfigFile:

      if (P->ConfigFile != NULL)
        {
        MStringAppendF(String,"%s",
          P->ConfigFile);
        }

      break;

    case mpaCost:

      if ((MAM[0].Type != mamtNONE) && (MAM[0].UseDisaChargePolicy == TRUE))
        {
        mrsv_t *tmpR;
   
        int     rindex;
   
        double tmpCost;
        double Cost = 0.0;
   
        marray_t RList;

        if (P->VPCProfile != NULL)
          Cost += P->VPCProfile->SetUpCost;
   
        MUArrayListCreate(&RList,sizeof(mrsv_t *),10);
       
        if (MRsvGroupGetList(P->RsvGroup,&RList,NULL,0) == FAILURE)
          {
          MUArrayListFree(&RList);

          MStringAppendF(String,"%.2f",Cost);

          break;
          }

        for (rindex = 0;rindex < RList.NumItems;rindex++)
          {
          tmpR = (mrsv_t *)MUArrayListGetPtr(&RList,rindex);
   
          MRsvGetCost(tmpR,P->VPCProfile,0,0,0,0,0,NULL,NULL,FALSE,NULL,&tmpCost);
   
          Cost += tmpCost;
          }
   
        MUArrayListFree(&RList);

        MStringAppendF(String,"%.2f",Cost);
        }
      else if (P->Cost > 0)
        {
        MStringAppendF(String,"%.2f",
          P->Cost);
        } 
      else if (P->VPCMasterRsv != NULL)
        {
        mrsv_t  *R;
        double   Cost;

        R = P->VPCMasterRsv;

        if ((P->RsvGroup != NULL) && !strcmp(P->RsvGroup,R->Name))
          {
          if (P->VPCProfile != NULL)
            {
            /* NOTE:  should MRsvGetCost() replace calculation below? */

            Cost = 
              P->VPCProfile->SetUpCost +
              P->VPCProfile->NodeSetupCost * R->AllocTC +
              P->VPCProfile->NodeHourAllocChargeRate * 
                R->AllocTC * 
                MAX(1,R->EndTime - R->StartTime) / MCONST_HOURLEN;

            MStringAppendF(String,"%.2f",
              Cost);
            }
          }
        }    /* END else if (P->VPCMasterRsv != NULL) */

      break;

    case mpaDuration:

      if (P->ReqDuration > 0)
        {
        MStringAppendF(String,"%ld",
          P->ReqDuration);
        }

      break;

    case mpaDefaultNodeActiveWatts:

      if (P->DefNodeActiveWatts != 0.0)
        MStringAppendF(String,"%.2f",
          P->DefNodeActiveWatts);

      break;

    case mpaDefaultNodeIdleWatts:

      if (P->DefNodeIdleWatts != 0.0)
        MStringAppendF(String,"%.2f",
          P->DefNodeIdleWatts);

      break;

    case mpaDefaultNodeStandbyWatts:

      if (P->DefNodeStandbyWatts != 0.0)
        MStringAppendF(String,"%.2f",
          P->DefNodeStandbyWatts);

      break;

    case mpaID:

      MStringSetF(String,"%s",P->Name);

      break;
      
    case mpaIsRemote:

      MStringSetF(String,"%s",MBool[bmisset(&P->Flags,mpfIsRemote)]);

      break;

    case mpaJobCount:

      if (P->RM != NULL)
        MStringAppendF(String,"%d",
          P->RM->JobCount);

      break;

    case mpaJobNodeMatchPolicy:

      if (!bmisclear(&P->JobNodeMatchBM))
        {
        bmtostring(&P->JobNodeMatchBM,MJobNodeMatchType,String);
        }

      break;

    case mpaMaxCPULimit:

      if (P->L.JobMaximums[0] != NULL)
        {
        mjob_t *J = P->L.JobMaximums[0];

        if (J->CPULimit > 0)
          MStringAppendF(String,"%ld",
            J->CPULimit);
        }

      break;

    case mpaMaxNode:

      if (P->L.JobMaximums[0] != NULL)
        {
        mjob_t *J = P->L.JobMaximums[0];

        if (J->Request.NC > 0)
          {
          MStringAppendF(String,"%d",
            J->Request.NC);
          }
        }

      break;

    case mpaMaxProc:

      if (P->L.JobMaximums[0] != NULL)
        {
        mjob_t *J = P->L.JobMaximums[0];

        if (J->Request.TC > 0)
          MStringAppendF(String,"%d",
            J->Request.TC);
        }

      break;

    case mpaMaxPS:

      if (P->L.JobMaximums[0] != NULL)
        {
        mjob_t *J = P->L.JobMaximums[0];

        if (J->PSDedicated > 0.0)
          MStringAppendF(String,"%f",
            J->PSDedicated);
        }

      break;

    case mpaMaxWCLimit:

      if (P->L.JobMaximums[0] != NULL)
        {
        mjob_t *J = P->L.JobMaximums[0];

        if (J->SpecWCLimit[0] > 0)
          {
          MStringAppendF(String,"%ld",
            J->SpecWCLimit[0]);
          }
        }

      break;
 
    case mpaMessages:

      if (P->Message != NULL)
        MStringSetF(String,"%s",P->Message);

      break;

    case mpaMinNode:

      if (P->L.JobMinimums[0] != NULL)
        {
        mjob_t *J = P->L.JobMinimums[0];

        if (J->Request.NC > 0)
          MStringAppendF(String,"%d",
            J->Request.NC);
        }

      break;

    case mpaMinProc:

      if (P->L.JobMinimums[0] != NULL)
        {
        mjob_t *J = P->L.JobMinimums[0];

        if (J->Request.TC > 0)
          MStringAppendF(String,"%d",
            J->Request.TC);
        }

      break;

    case mpaMinWCLimit:

      if (P->L.JobMinimums[0] != NULL)
        {
        mjob_t *J = P->L.JobMinimums[0];

        if (J->SpecWCLimit[0] > 0)
          MStringAppendF(String,"%ld",
            J->SpecWCLimit[0]);
        }

      break;

    case mpaNodeAccessPolicy:

      if (P->NAccessPolicy != mnacNONE)
        MStringAppendF(String,"%s",MNAccessPolicy[P->NAccessPolicy]);

      break;

    case mpaNodeChargeRate:

      if (P->DefNodeChargeRate > 0.0)
        {
        MStringAppendF(String,"%.2f",
          P->DefNodeChargeRate);
        }

      break;

    case mpaNodePowerOffDuration:

      if (P->NodePowerOffDuration > 0)
        {
        MStringAppendF(String,"%ld",
          P->NodePowerOffDuration);
        }

    case mpaNodePowerOnDuration:

      if (P->NodePowerOnDuration > 0)
        {
        MStringAppendF(String,"%ld",
          P->NodePowerOnDuration);
        }

      break;

    case mpaOS:
  
      if ((P->RepN != NULL) && (P->RepN->ActiveOS != 0))
        {
        if (P->RepN->Index > MSched.M[mxoNode])
          {
          /* remove this if statement after ticket 3320 resolved */

          MDB(3,fSTRUCT) MLog("WARNING:  partition %s representative node set to corrupt value\n",
            (P != NULL) ? P->Name : "NULL");
          }

        MStringSetF(String,"%s",MAList[meOpsys][P->RepN->ActiveOS]);
        }

      break;

    case mpaOwner:

      if (P->OType != mxoNONE)
        {
        MStringAppendF(String,"%s:%s",
          MXO[P->OType],
          (P->OID != NULL) ? P->OID : "NULL");
        }

      break;

    case mpaPriority:

      if (P->Priority != 0)
        {
        MStringAppendF(String,"%ld",P->Priority);
        }

      break;

    case mpaProfile:

      if (P->VPCProfile != NULL)
        MStringSetF(String,"%s",P->VPCProfile->Name);

      break;

    case mpaProvEndTime:

      if (P->ProvEndTime > 0)
        {
        MStringAppendF(String,"%ld",
          P->ProvEndTime);
        }

      break;

    case mpaProvStartTime:

      if (P->ProvStartTime > 0)
        {
        MStringAppendF(String,"%ld",
          P->ProvStartTime);
        }

      break;

    case mpaPurgeTime:

      if (P->PurgeTime > 0)
        {
        MStringAppendF(String,"%ld",
          P->PurgeTime);
        }

      break;

    case mpaReqResources:

      if (P->ReqResources != NULL)
        {
        MStringSetF(String,"%s",P->ReqResources);
        }

      break;

    case mpaRsvGroup:

      if (P->RsvGroup != NULL)
        {
        MStringSetF(String,"%s",P->RsvGroup);
        }

      break;

    case mpaRM:

      if (P->RM != NULL)
        {
        MStringSetF(String,"%s",P->RM->Name);
        }

      break;

    case mpaRMType:

      if (P->RM != NULL)
        {
        MStringSetF(String,"%s",(char *)MRMType[P->RM->Type]);
        }

      break;

    case mpaSize:

      if (P->VPCMasterRsv != NULL)
        {
        mrsv_t  *R;

        R = P->VPCMasterRsv;

        if ((P->RsvGroup != NULL) && !strcmp(P->RsvGroup,R->Name))
          {
          MStringAppendF(String,"%d",
            R->AllocTC);
          }
        }

      break;

    case mpaStartTime:

      if (P->ReqStartTime > 0)
        {
        MStringAppendF(String,"%ld",
          P->ReqStartTime);
        }

      break;
    
    case mpaState:

      if (P->State != mvpcsNONE)
        {
        MStringSetF(String,"%s",(char *)MClusterState[P->State]);
        }
      else if ((P->RM != NULL) && (P->RM->State != mrmsNONE))
        {
        MStringSetF(String,"%s",(char *)MRMState[P->RM->State]);
        }

      break;

    case mpaSuspendResources:

      {
      int index;

      mbool_t AttrIsSet = FALSE;

      for (index = 1;index < mrLAST;index++)
        {
        if (P->SuspendRes[index] == FALSE)
          continue;

        if (AttrIsSet == TRUE)
          {
          MStringAppendF(String,",%s",
            MResourceType[index]);
          }
        else
          {
          MStringAppendF(String,"%s",
            MResourceType[index]);

          AttrIsSet = TRUE;
          }
        }    /* END for (index) */
      }      /* END (case mpaSuspendResources) */

      break;

    case mpaUpdateTime:

      if (P->RM != NULL)
        {
        MStringAppendF(String,"%ld",
          P->RM->UTime);
        }

      break;

    case mpaUser:

      {
      mrsv_t *R;
         
      if (P->VPCMasterRsv == NULL)
        break;
        
      R = P->VPCMasterRsv;
       
      if ((R == (mrsv_t *)1) || (R == NULL))
        break;

      if ((P->RsvGroup == NULL) || strcmp(R->Name,P->RsvGroup)) 
        break;
     
      if (R->U != NULL)
        MStringSet(String,R->U->Name);
      }  /* END BLOCK (case mpaUser) */

      break;

    case mpaUseTTC:

      if (bmisset(&P->Flags,mpfUseTTC))
        MStringSetF(String,"%s",MBool[TRUE]);

      break;

    case mpaVariables:

      {
      mrsv_t *R;

      /* extract variables from rsv group leader */

      if (MRsvFind(P->RsvGroup,&R,mraNONE) == FAILURE)
        break;

      MRsvAToMString(R,mraVariables,String,DModeBM);
      }  /* END BLOCK (case mpaVariables) */

      break;

    case mpaVMCreationDuration:

      if (P->VMCreationDuration > 0)
        {
        char TString[MMAX_LINE];

        MULToTString(P->VMCreationDuration,TString);

        MStringSetF(String,"%s",TString);
        }

      break;

    case mpaVMDeletionDuration:

      if (P->VMDeletionDuration > 0)
        {
        char TString[MMAX_LINE];

        MULToTString(P->VMDeletionDuration,TString);

        MStringSetF(String,"%s",TString);
        }

      break;

    case mpaVMMigrationDuration:

      if (P->VMMigrationDuration > 0)
        {
        char TString[MMAX_LINE];

        MULToTString(P->VMMigrationDuration,TString);

        MStringSetF(String,"%s",TString);
        }

      break;

    case mpaVPCFlags:

      MStringSetF(String,"%lu",
        P->VPCFlags);

      break;

    default:

      /* NO-OP */

      break;
    }  /* END switch (AIndex) */

  if (MUStrIsEmpty(String->c_str()))
    {
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MParAToString() */




/**
 * Setup the appropriate BMs for partition configuration processing.
 *
 * NOTE: as each credential parameter is setup for per-partition scheduling
 *       we can uncomment it below.
 *
 * @param Global
 * @param Cred
 * @param QOS
 * @param Group
 * @param User
 * @param Acct
 * @param Class
 */

int MParSetupRestrictedCfgBMs(

  mbitmap_t *Global,
  mbitmap_t *Cred,
  mbitmap_t *QOS,
  mbitmap_t *Group,
  mbitmap_t *User,
  mbitmap_t *Acct,
  mbitmap_t *Class)

  {
  if (Global != NULL)
    {
    bmclear(Global);
   
    bmset(Global,mcoCACap);
    bmset(Global,mcoCAWeight);
    bmset(Global,mcoOLDAFSWeight);
    bmset(Global,mcoAJobAttrCap);
    bmset(Global,mcoAJobAttrWeight);
    bmset(Global,mcoAJobGResCap);
    bmset(Global,mcoAJobGResWeight);
    bmset(Global,mcoAJobStateCap);
    bmset(Global,mcoAJobStateWeight);
    bmset(Global,mcoAttrWeight);
    bmset(Global,mcoSBPCap);
    bmset(Global,mcoSBPWeight);
    bmset(Global,mcoCCCap);
    bmset(Global,mcoCCWeight);
    bmset(Global,mcoCredCap);
    bmset(Global,mcoCredWeight);
    bmset(Global,mcoSDeadlineCap);
    bmset(Global,mcoSDeadlineCap);
    bmset(Global,mcoSDeadlineWeight);
    bmset(Global,mcoOLDDirectSpecWeight);
    bmset(Global,mcoRDiskCap);
    bmset(Global,mcoRDiskWeight);
    bmset(Global,mcoFACap);
    bmset(Global,mcoFAWeight);
    bmset(Global,mcoOLDFSAWeight);
    bmset(Global,mcoFSCap);
    bmset(Global,mcoFCCap);
    bmset(Global,mcoFCWeight);
    bmset(Global,mcoOLDFSCWeight);
    bmset(Global,mcoGFACap);
    bmset(Global,mcoGFAWeight);
    bmset(Global,mcoGFGCap);
    bmset(Global,mcoGFGWeight);
    bmset(Global,mcoFGCap);
    bmset(Global,mcoFGWeight);
    bmset(Global,mcoOLDFSGWeight);
    bmset(Global,mcoGFUCap);
    bmset(Global,mcoGFUWeight);
    bmset(Global,mcoFSInterval);
    bmset(Global,mcoFJPUCap);
    bmset(Global,mcoFJPUWeight);
    bmset(Global,mcoFJRPUCap);
    bmset(Global,mcoFJRPUWeight);
    bmset(Global,mcoFPPUCap);
    bmset(Global,mcoFPPUWeight);
    bmset(Global,mcoFPSPUCap);
    bmset(Global,mcoFPSPUWeight);
    bmset(Global,mcoFQCap);
    bmset(Global,mcoFQWeight);
    bmset(Global,mcoOLDFSQWeight);
    bmset(Global,mcoFUCap);
    bmset(Global,mcoFUWeight);
    bmset(Global,mcoOLDFSUWeight);
    bmset(Global,mcoFSWeight);
    bmset(Global,mcoOLDGFSWeight);
    bmset(Global,mcoCGCap);
    bmset(Global,mcoCGWeight);
    bmset(Global,mcoAJobIDCap);
    bmset(Global,mcoAJobIDWeight);
    bmset(Global,mcoAJobNameCap);
    bmset(Global,mcoAJobNameWeight);
    bmset(Global,mcoRMemCap);
    bmset(Global,mcoRMemWeight);
    bmset(Global,mcoRNodeCap);
    bmset(Global,mcoRNodeWeight);
    bmset(Global,mcoRPECap);
    bmset(Global,mcoRPEWeight);
    bmset(Global,mcoRProcCap);
    bmset(Global,mcoRProcWeight);
    bmset(Global,mcoRPSCap);
    bmset(Global,mcoRPSWeight);
    bmset(Global,mcoCQCap);
    bmset(Global,mcoCQWeight);
    bmset(Global,mcoSQTCap);
    bmset(Global,mcoOLDQTWeight);
    bmset(Global,mcoSQTWeight);
    bmset(Global,mcoResCap);
    bmset(Global,mcoResWeight);
    bmset(Global,mcoOLDServWeight);
    bmset(Global,mcoServCap);
    bmset(Global,mcoServWeight);
    bmset(Global,mcoSSPVCap);
    bmset(Global,mcoSSPVWeight);
    bmset(Global,mcoSStartCountCap);
    bmset(Global,mcoSStartCountWeight);
    bmset(Global,mcoRSwapCap);
    bmset(Global,mcoRSwapWeight);
    bmset(Global,mcoTargCap);
    bmset(Global,mcoTQTCap);
    bmset(Global,mcoTQTWeight);
    bmset(Global,mcoTargWeight);
    bmset(Global,mcoTXFCap);
    bmset(Global,mcoTXFWeight);
    bmset(Global,mcoOLDUFSWeight);
    bmset(Global,mcoUsageCap);
    bmset(Global,mcoUConsCap);
    bmset(Global,mcoUConsWeight);
    bmset(Global,mcoUPerCCap);
    bmset(Global,mcoUPerCWeight);
    bmset(Global,mcoURemCap);
    bmset(Global,mcoURemWeight);
    bmset(Global,mcoUExeTimeCap);
    bmset(Global,mcoUExeTimeWeight);
    bmset(Global,mcoUsageWeight);
    bmset(Global,mcoCUCap);
    bmset(Global,mcoSUPrioCap);
    bmset(Global,mcoSUPrioWeight);
    bmset(Global,mcoCUWeight);
    bmset(Global,mcoRWallTimeCap);
    bmset(Global,mcoRWallTimeWeight);
    bmset(Global,mcoFUWCACap);
    bmset(Global,mcoFUWCAWeight);
    bmset(Global,mcoSXFCap);
    bmset(Global,mcoOLDXFWeight);
    bmset(Global,mcoSXFWeight);
    }  /* END if (Global != NULL) */

  if (QOS != NULL)
    {
    bmclear(QOS);

/*
    bmset(QOS,mqaMaxJob);
    bmset(QOS,mqaMaxProc);
    bmset(QOS,mqaMaxNode);
    bmset(QOS,mqaXFWeight);
    bmset(QOS,mqaQTWeight);
    bmset(QOS,mqaXFTarget);
    bmset(QOS,mqaQTTarget);
    bmset(QOS,mqaFSTarget);
*/
    }  /* END if (QOS != NULL) */

  if (Cred != NULL)
    {
    bmclear(Cred);
   
    bmset(Cred,mcaMaxJob);
    bmset(Cred,mcaMaxNode);
    bmset(Cred,mcaMaxWC);
    bmset(Cred,mcaMaxIJob);
    bmset(Cred,mcaMaxProc);
    bmset(Cred,mcaMaxSubmitJobs);
    bmset(Cred,mcaMinNodePerJob);
    bmset(Cred,mcaMaxNodePerJob);
    bmset(Cred,mcaMaxProcPerJob);
    bmset(Cred,mcaMaxWCLimitPerJob);
    bmset(Cred,mcaFlags);

/*
    bmset(Cred,mcaFSCap);
    bmset(Cred,mcaFSTarget);
    bmset(Cred,mcaGFSTarget);
    bmset(Cred,mcaGFSUsage);
    bmset(Cred,mcaMaxGRes);
    bmset(Cred,mcaMaxPE);
    bmset(Cred,mcaMinProc);
    bmset(Cred,mcaMaxPS);
    bmset(Cred,mcaMaxMem);
    bmset(Cred,mcaMaxIGRes);
    bmset(Cred,mcaMaxINode);
    bmset(Cred,mcaMaxIPE);
    bmset(Cred,mcaMaxIProc);
    bmset(Cred,mcaMaxIPS);
    bmset(Cred,mcaMaxIWC);
    bmset(Cred,mcaMaxIMem);
    bmset(Cred,mcaOMaxJob);
    bmset(Cred,mcaOMaxNode);
    bmset(Cred,mcaOMaxPE);
    bmset(Cred,mcaOMaxProc);
    bmset(Cred,mcaOMaxPS);
    bmset(Cred,mcaOMaxWC);
    bmset(Cred,mcaOMaxMem);
    bmset(Cred,mcaOMaxIJob);
    bmset(Cred,mcaOMaxINode);
    bmset(Cred,mcaOMaxIPE);
    bmset(Cred,mcaOMaxIProc);
    bmset(Cred,mcaOMaxIPS);
    bmset(Cred,mcaOMaxIWC);
    bmset(Cred,mcaOMaxIMem);
    bmset(Cred,mcaOMaxJNode);
    bmset(Cred,mcaOMaxJPE);
    bmset(Cred,mcaOMaxJProc);
    bmset(Cred,mcaOMaxJPS);
    bmset(Cred,mcaOMaxJWC);
    bmset(Cred,mcaOMaxJMem);
    bmset(Cred,mcaMaxJobPerUser);
    bmset(Cred,mcaMaxJobPerGroup);
    bmset(Cred,mcaMaxNodePerUser);
    bmset(Cred,mcaMaxProcPerUser);
    bmset(Cred,mcaMaxProcPerNodePerQueue);
    bmset(Cred,mcaMaxProcPerNode);
*/
    } /* END if (Cred != NULL) */

  if (Class != NULL)
    {
    bmclear(Class);

/*
    bmset(Class,mclaMaxProcPerNode);
    bmset(Class,mclaMaxCPUTime);
    bmset(Class,mclaMaxNode);
    bmset(Class,mclaMaxProc);
    bmset(Class,mclaMaxPS);
    bmset(Class,mclaMinNode);
    bmset(Class,mclaMinProc);
    bmset(Class,mclaMinPS);
    bmset(Class,mclaMinTPN);
*/
    }  /* END if (Class != NULL) */

  if (Group != NULL)
    {
    bmclear(Group);
    }  /* END if (Group != NULL) */

  if (Acct != NULL)
    {
    bmclear(Group);
    }  /* END if (Acct != NULL) */

  if (User != NULL)
    {
    bmclear(Group);
    }  /* END if (User != NULL) */

  return(SUCCESS);
  } /* END MParSetupRestrictedCfgBMs() */


/**
 * Load and process a partition configuration file.
 *
 * @param P
 * @param CfgString
 * @param EMsg
 */

int MParLoadConfigFile(

  mpar_t    *P,
  mstring_t *CfgString,
  char      *EMsg)

  {
  char  *ptr;
 
  char   PVal[MMAX_BUFFER + 1];
  char   IName[MMAX_NAME];
  char   Default[MMAX_NAME];
 
  int    cindex;
 
  enum MCfgParamEnum CIndex;

  mbitmap_t Global;

  char *Buffer = NULL;

  MParSetupRestrictedCfgBMs(&Global,NULL,NULL,NULL,NULL,NULL,NULL);

  if (P == NULL)
    {
    bmclear(&Global);

    return(FAILURE);
    }

  if (CfgString == NULL)
    {
    Buffer = MFULoadNoCache(P->ConfigFile,1,NULL,NULL,NULL,NULL);
    }
  else
    {
    MUStrDup(&Buffer,CfgString->c_str());
    }

  if ((Buffer == NULL) || (Buffer[0] == '\0'))
    {
    /* no buffer to process */

    bmclear(&Global);

    MUFree(&Buffer);

    return(FAILURE);
    }

  /* remove comment lines */

  MCfgAdjustBuffer(&Buffer,FALSE,NULL,FALSE,FALSE,FALSE);

  MUStrCpy(Default,"DEFAULT",sizeof(Default));

  /* look for all defined parameters in buffer */
 
  MCredLoadConfig(mxoQOS,Default,P,Buffer,TRUE,NULL);
  MCredLoadConfig(mxoQOS,NULL,P,Buffer,TRUE,NULL);      

  MCredLoadConfig(mxoUser,Default,P,Buffer,TRUE,NULL);
  MCredLoadConfig(mxoUser,NULL,P,Buffer,TRUE,NULL);      

  MCredLoadConfig(mxoGroup,Default,P,Buffer,TRUE,NULL);
  MCredLoadConfig(mxoGroup,NULL,P,Buffer,TRUE,NULL);

  MCredLoadConfig(mxoAcct,Default,P,Buffer,TRUE,NULL);
  MCredLoadConfig(mxoAcct,NULL,P,Buffer,TRUE,NULL);

  MCredLoadConfig(mxoClass,Default,P,Buffer,TRUE,NULL);
  MCredLoadConfig(mxoClass,NULL,P,Buffer,TRUE,NULL);
 
  /* look for all defined parameters in buffer */
 
  for (cindex = 1;MCfg[cindex].Name != NULL;cindex++)
    {
    if (!bmisset(&Global,MCfg[cindex].PIndex))
      continue;

    CIndex = (enum MCfgParamEnum)cindex;

    /* NOTE:  process all parameters located */

    ptr = Buffer;

    while (MCfgGetParameter(
         ptr,
         MCfg[cindex].Name,
         &CIndex,  /* O */
         IName,    /* O */
         PVal,     /* O */
         sizeof(PVal),
         FALSE,
         &ptr) == SUCCESS)
      {
      MDB(7,fCONFIG) MLog("INFO:     value for parameter '%s': '%s' for partition %s\n",
        MCfg[CIndex].Name,
        PVal,
        P->Name);

      MCfgProcessLine(CIndex,IName,P,PVal,FALSE,NULL);

      IName[0] = '\0';
      }  /* END while (MCfgGetParameter() == SUCCESS) */
    }    /* END for (cindex)  */
 
  bmclear(&Global);

  MUFree(&Buffer);

  return(SUCCESS);
  }  /* END MParLoadConfigFile() */


/**
 * Report partition attributes in XML.
 *
 * @see MParFromXML()
 * @see MVPCToXML() FIXME: should combine MParToXML() and MVPCToXML()
 *
 * @param P (I)
 * @param E (O)
 * @param SAList (I) [optional]
 */

int MParToXML(

  mpar_t            *P,      /* I */
  mxml_t            *E,      /* O */
  enum MParAttrEnum *SAList) /* I (optional) */

  {
  const enum MParAttrEnum DAList[] = {
    mpaAcct,
    mpaACL,
    mpaArch,
    mpaBacklogPS,
    mpaBacklogDuration,
    mpaCmdLine,
    mpaCost,
    mpaDuration,
    mpaID,
    mpaOS,
    mpaOwner,
    mpaPriority,
    mpaProfile,
    mpaProvStartTime,
    mpaProvEndTime,
    mpaPurgeTime,
    mpaReqResources,
    mpaRsvGroup,
    mpaRMType,
    mpaStartTime,
    mpaState,
    mpaUpdateTime,
    mpaUser,
    mpaVariables,
    mpaVMCreationDuration,
    mpaVMDeletionDuration,
    mpaVMMigrationDuration,
    mpaNONE };

  int  aindex;

  enum MParAttrEnum *AList;

  mbool_t ShowACL = FALSE;

  if ((P == NULL) || (E == NULL))
    {
    return(FAILURE);
    }

  if (SAList != NULL)
    AList = SAList;
  else
    AList = (enum MParAttrEnum *)DAList;

  mstring_t tmpString(MMAX_LINE);;

  for (aindex = 0;AList[aindex] != mpaNONE;aindex++)
    {
    if (AList[aindex] == mpaACL)
      {
      ShowACL = TRUE;

      continue;
      }

    MStringSet(&tmpString,"\0");

    if ((MParAToMString(P,AList[aindex],&tmpString,0) == FAILURE) ||
        (MUStrIsEmpty(tmpString.c_str())))
      {
      continue;
      }

    MXMLSetAttr(E,(char *)MParAttr[AList[aindex]],tmpString.c_str(),mdfString);
    }  /* END for (aindex) */

  if (P->MB != NULL)
    {
    mxml_t *ME = NULL;

    MStringSet(&tmpString,"\0");

    MParAToMString(P,mpaMessages,&tmpString,0);

    if (MXMLFromString(&ME,tmpString.c_str(),NULL,NULL) == SUCCESS)
      {
      MXMLAddE(E,ME);
      }
    }

  if (ShowACL == TRUE)
    {
    marray_t RList;

    MUArrayListCreate(&RList,sizeof(mrsv_t *),10);

    if ((MRsvGroupGetList(P->RsvGroup,&RList,NULL,0) == SUCCESS) &&
        (RList.NumItems > 0))
      {
      mxml_t *AE;

      macl_t *tACL;

      mrsv_t *R = (mrsv_t *)MUArrayListGetPtr(&RList,0);

      for (tACL = R->CL;tACL != NULL;tACL = tACL->Next)
        {
        MACLToXML(tACL,&AE,NULL,TRUE);

        MXMLAddE(E,AE);
        }
      }    /* END if ((MRsvGroupGetList(P->RsvGroup,RList,NULL,0) == SUCCESS) && ...) */

    MUArrayListFree(&RList);
    }

  return(SUCCESS);
  }  /* END MParToXML() */





/**
 * Create/update partition using XML description.
 *
 * @see MParToXML()
 *
 * @param P (I) [modified]
 * @param PE (I)
 */

int MParFromXML(

  mpar_t *P,   /* I (modified) */
  mxml_t *PE)  /* I */

  {
  int aindex;

  enum MParAttrEnum paindex;

  mxml_t *ME;

  int CTok;

#ifndef __MOPT

  if ((P == NULL) || (PE == NULL))
    {
    return(FAILURE);
    }

#endif /* !__MOPT */

  /* NOTE:  do not initialize partition.  may be overlaying data */

  for (aindex = 0;aindex < PE->ACount;aindex++)
    {
    paindex = (enum MParAttrEnum)MUGetIndex(PE->AName[aindex],MParAttr,FALSE,0);

    if (paindex == mpaNONE)
      continue;

    MParSetAttr(P,paindex,(void **)PE->AVal[aindex],mdfString,mSet);
    }  /* END for (aindex) */

  if (MXMLGetChild(PE,(char *)MParAttr[mpaMessages],NULL,&ME) == SUCCESS)
    {
    MMBFromXML(&P->MB,ME);
    }

  if (P->IsVPC == TRUE)
    {
    marray_t RList;
    mrsv_t *R;

    CTok = -1;

    aindex = 0;

    MUArrayListCreate(&RList,sizeof(mrsv_t *),10);

    if (MRsvGroupGetList(P->RsvGroup,&RList,NULL,0) == SUCCESS)
      {
      R = (mrsv_t *)MUArrayListGetPtr(&RList,0);

      while (MXMLGetChild(PE,(char *)MParAttr[mpaACL],&CTok,&ME) == SUCCESS)
        {
        MACLFromXML(&R->CL,ME,FALSE);
        }
      }   /* if (MRsvGroupGetList(P->RsvGroup,RList,NULL,0) == SUCCESS) */

    MUArrayListFree(&RList);
    }     /* END if (P->IsVPC == TRUE) */

  return(SUCCESS);
  }  /* END MParFromXML() */



/**
 * Load partition config parameters.
 *
 * @see MParProcessConfig() - child
 *
 * @param P (I)
 * @param Buf (I) [optional]
 */

int MParLoadConfig(

  mpar_t  *P,    /* I */
  char    *Buf)  /* I (optional) */

  {
  char *ptr;
  char *head;

  char  Value[MMAX_LINE];

  const char *FName = "MParLoadConfig";

  MDB(5,fSTRUCT) MLog("%s(%s,%s)\n",
    FName,
    (P != NULL) ? P->Name : "NULL",
    (Buf != NULL) ? Buf : "NULL");

  /* load specified partition config info */

  if ((P == NULL) || (P->Name[0] == '\0'))
    {
    return(FAILURE);
    }

  if (Buf != NULL)
    head = Buf;
  else
    head = MSched.ConfigBuffer;

  if (head == NULL)
    {
    return(FAILURE);
    }

  ptr = head;

  while (MCfgGetSVal(
           head,
           &ptr,
           MCredCfgParm[mxoPar],
           P->Name,
           NULL,
           Value,
           sizeof(Value),
           0,
           NULL) != FAILURE)
    {
    /* partition paramter located */

    /* FORMAT:  <ATTR>=<VALUE>[ <ATTR>=<VALUE>]... */

    if (MParProcessConfig(P,NULL,Value) == FAILURE)
      {
      return(FAILURE);
      }
    }

  return(SUCCESS);
  }  /* END MParLoadConfig() */




/**
 * Process 'PARCFG' parameter.
 *
 * WARNING: ALL partition is loaded after all other partitions 
 *
 * @see MParLoadConfig()
 *
 * @param P (I) [modified]
 * @param R (I) [optional]
 * @param Value (I)
 */

int MParProcessConfig(

  mpar_t  *P,     /* I (modified) */
  mrm_t   *R,     /* I (optional) */
  char    *Value) /* I */

  {
  char  *ptr;
  char  *TokPtr;

  char   ValLine[MMAX_LINE];
  char   AttrArray[MMAX_NAME];

  int    aindex;

  enum   MParAttrEnum AIndex;

  int    tmpI;

  int    rc;

  mbool_t FailureDetected;

  const char *FName = "MParProcessConfig";

  MDB(5,fSTRUCT) MLog("%s(%s,%s,%s)\n",
    FName,
    (P != NULL) ? P->Name : "NULL",
    (R != NULL) ? R->Name : "NULL",
    (Value != NULL) ? Value : "NULL");

  if ((P == NULL) ||
      (Value == NULL) ||
      (Value[0] == '\0'))
    {
    return(FAILURE);
    }

  ptr = MUStrTokE(Value," \t\n",&TokPtr);

  FailureDetected = FALSE;

  while (ptr != NULL)
    {
    /* parse name-value pairs */

    /* FORMAT:  <ATTR>=<VALUE>[,<VALUE>] */

    /* partition attribute processing */

    if (MUGetPair(
          ptr,
          (const char **)MParAttr,
          &aindex,
          AttrArray,
          TRUE,
          &tmpI,
          ValLine,
          MMAX_LINE) == FAILURE)
      {
      char tmpLine[MMAX_LINE];

      /* cannot parse value pair */

      snprintf(tmpLine,sizeof(tmpLine),"unknown attribute '%.128s' specified",
        ptr);

      MDB(3,fSTRUCT) MLog("WARNING:  unknown attribute '%s' specified for partition %s\n",
        ptr,
        (P != NULL) ? P->Name : "NULL");

      MMBAdd(&P->MB,tmpLine,NULL,mmbtNONE,0,0,NULL);

      ptr = MUStrTokE(NULL," \t\n",&TokPtr);

      continue;
      }  /* END if (MUGetPair() == FAILURE) */

    rc = SUCCESS;

    AIndex = (enum MParAttrEnum)aindex;

    switch (aindex)
      {
      case mpaAllocResFailPolicy:
      case mpaDefCPULimit:
      case mpaDefaultNodeActiveWatts:
      case mpaDefaultNodeIdleWatts:
      case mpaDefaultNodeStandbyWatts:
      case mpaFlags:
      case mpaGreenBacklogThreshold:
      case mpaGreenQTThreshold:
      case mpaGreenXFactorThreshold:
      case mpaJobActionOnNodeFailure:
      case mpaJobNodeMatchPolicy:
      case mpaMaxCPULimit:
      case mpaMaxProc:
      case mpaMaxPS:
      case mpaMaxNode:
      case mpaMaxWCLimit:
      case mpaMinProc:
      case mpaMinNode:
      case mpaMinWCLimit:
      case mpaNodeAccessPolicy:
      case mpaNodeAllocPolicy:
      case mpaNodeChargeRate:
      case mpaNodePowerOffDuration:
      case mpaNodePowerOnDuration:
      case mpaPriority:
      case mpaReqOS:
      case mpaReservationDepth:
      case mpaUseTTC:
      case mpaVMCreationDuration:
      case mpaVMDeletionDuration:
      case mpaVMMigrationDuration:
      case mpaRM:
      case mpaConfigFile:

        MParSetAttr(P,AIndex,(void **)ValLine,mdfString,mSet);

        break;

      default:

        /* NOT SUPPORTED */
 
        {
        char tmpLine[MMAX_LINE];
 
        snprintf(tmpLine,sizeof(tmpLine),"attribute %s not configurable",
          MParAttr[AIndex]);

        MDB(3,fSTRUCT) MLog("WARNING:  attribute %s not configurable for partition %s\n",
          MParAttr[AIndex],
          (P != NULL) ? P->Name : "NULL");

        MMBAdd(&P->MB,tmpLine,NULL,mmbtNONE,0,0,NULL);
        }  /* END BLOCK */
 
        break;
      }  /* END switch (aindex) */

    if (rc == FAILURE)
      FailureDetected = TRUE;

    ptr = MUStrTok(NULL," \t\n",&TokPtr);
    }    /* END while (ptr != NULL) */

  if (FailureDetected == TRUE)
    {
    return(FAILURE);
    }

  /* sync 'side-effects' */

  /* NO-OP */

  return(SUCCESS);
  }  /* END MParProcessConfig() */




/**
 * Report partition allocation priority for specified job.
 *
 * @param P (I)
 * @param J (I) [optional]
 * @param EStartTime (I)
 * @param Val (O)
 * @param StartTime (I)
 * @param Msg (O) [optional,minsize=MMAX_LINE]
 */

int MParGetPriority(

  mpar_t   *P,          /* I */
  mjob_t   *J,          /* I (optional)*/
  mulong    EStartTime, /* I */
  double   *Val,        /* O */
  long      StartTime,  /* I */
  char     *Msg)        /* O (optional,minsize=MMAX_LINE) */

  {
  char   Buffer[MMAX_LINE];
  double tmpPrio;

  long   QTime;

  char  *BPtr;
  int    BSpace;

  const char *FName = "MParGetPriority";
 
  MDB(6,fSTRUCT) MLog("%s(%s,%s,%ld,Val,%ld,Msg)\n",
    FName,
    (P != NULL) ? P->Name : "NULL",
    (J != NULL) ? J->Name : "NULL",
    EStartTime,
    StartTime);

  if (Val != NULL)
    *Val = 0.0;

  MUSNInit(&BPtr,&BSpace,Buffer,sizeof(Buffer));

  if ((P == NULL) || (Val == NULL))
    {
    return(FAILURE);
    }

  MUSNPrintF(&BPtr,&BSpace,"P[%s] = ",
    P->Name);

  /* NOTE:  P = (AllocationTime * ATWeight + QTime * QtWeight + NPrio * NPrioWeight) + 1/(SJWeight * NumSubmittedJobs) */

  tmpPrio = 0.0;

  QTime = 0;

  if (MPar[0].PAllocPolicy == mpapRandom)
    {
    *Val = (double) (rand() % 100);

    return(SUCCESS);
    }

  if (EStartTime > MSched.Time)
    {
    long TimeDiff;

    if ((mulong)StartTime > MSched.Time)
      {
      TimeDiff = EStartTime - StartTime;
      QTime = MAX(0,TimeDiff);
      }
    else
      {
      TimeDiff = EStartTime - MSched.Time;
      QTime = MAX(0,TimeDiff);
      }
    }

  if (MPar[0].PAllocPolicy == mpapFirstCompletion)
    {
    if (J != NULL)
      {
      /* NOTE:  determine job-specific partition speed (NYI) */

      QTime += (long)(1.0 * J->SpecWCLimit[0]);
      }
    }

  if (QTime > 0)
    {
    tmpPrio += (double)MPar[0].QTWeight * QTime;

    if (MPar[0].QTWeight != 0.0)
      {
      MUSNPrintF(&BPtr,&BSpace,"%.2f*QTime[%ld]",
        MPar[0].QTWeight,
        QTime);
      }
    }

  if ((P->RepN == NULL) || (P->RepN->PtIndex != P->Index))
    {
    int nindex;

    mnode_t *N;

    /* locate representatitive node */

    P->RepN = NULL;

    for (nindex = 0;nindex < MSched.M[mxoNode];nindex++)
      {
      N = MNode[nindex];

      if (N == NULL)
        break;

      if (N == (mnode_t *)1)
        continue;

      if (N->PtIndex != P->Index)
        continue;

      P->RepN = N;

      /* remove this debug log after ticket 3320 resolved */
      MDB(3,fSTRUCT) MLog("INFO:     partition %s representative node set to node (%s) index (%d) in MParGetPriority\n",
        P->Name,
        N->Name,
        N->Index);

      break;      
      }  /* END for (nindex) */
    }    /* END if ((P->RepN == NULL) || (P->RepP->PtIndex != P->Index)) */

  if (P->RepN != NULL)
    {
    char tmpLine[MMAX_LINE];

    double tmpNPrio;

    MNodeGetPriority(
      P->RepN,
      J,
      NULL,
      NULL,
      0,
      FALSE,
      &tmpNPrio,
      StartTime,
      tmpLine);

    tmpPrio += (double)MPar[0].NPWeight * tmpNPrio;

    if (MPar[0].NPWeight != 0.0) 
      {
      MUSNPrintF(&BPtr,&BSpace," +%.2f*NPrio[%s]",
        MPar[0].QTWeight,
        tmpLine);
      }
    }

  if (P->MigrateTime > 0)
    {
    tmpPrio += (double)MPar[0].ATWeight * (int)(MSched.Time - P->MigrateTime);

    if (MPar[0].ATWeight != 0.0)
      {
      MUSNPrintF(&BPtr,&BSpace," +%.2f*MigrateTime[%ld]",
        MPar[0].ATWeight,
        (MSched.Time - P->MigrateTime));
      }
    }

  if ((P->S.ISubmitJC > 0) && (MPar[0].SJWeight > 0))
    {
    tmpPrio += 1.0f / ((double)MPar[0].SJWeight * P->S.ISubmitJC);

    if (MPar[0].SJWeight != 0.0)
      {
      MUSNPrintF(&BPtr,&BSpace," + 1/(%.2f*NumSubmittedJobs[%d])",
        MPar[0].SJWeight,
        P->S.ISubmitJC);
      }
    }

  *Val = tmpPrio;

  MDB(7,fSTRUCT) MLog("INFO:     calculated partition priority %s\n",
    Buffer);

  if (Msg != NULL)
    {
    MUStrCpy(Msg,Buffer,MMAX_LINE);
    }

  return(SUCCESS);
  }  /* END MParGetPriority() */


/**
 * Parse node availability policy value
 *
 * @param P (I) [modified]
 * @param SArray (I)
 * @param DoEval (I)
 * @param EMsg (O) [minsize=MMAX_LINE,optional]
 */

int MParProcessNAvailPolicy(

  mpar_t   *P,      /* I (modified) */
  char    **SArray, /* I */
  mbool_t   DoEval, /* I */
  char     *EMsg)   /* O (minsize=MMAX_LINE,optional) */

  {
  char *sptr;
  char *STokPtr;

  char *pptr;
  char *rptr;
  char *TokPtr = NULL;
  int   pindex;
  enum  MResourceAvailabilityPolicyEnum policyindex;
  int   rindex;

  char *BPtr;
  int   BSpace;

  char  tmpLine[MMAX_LINE];

  if (EMsg != NULL)
    MUSNInit(&BPtr,&BSpace,EMsg,MMAX_LINE);

  if ((P == NULL) || (SArray == NULL))
    {
    return(FAILURE);
    }

  /* FORMAT:  <POLICY>[:<RESOURCETYPE>] ... */

  for (pindex = 0;SArray[pindex] != NULL;pindex++)
    {
    MUStrCpy(tmpLine,SArray[pindex],sizeof(tmpLine));

    sptr = MUStrTok(tmpLine," \t\n",&STokPtr);

    while (sptr != NULL)
      {
      pptr = MUStrTok(sptr,":",&TokPtr);

      policyindex = (enum MResourceAvailabilityPolicyEnum)MUGetIndexCI(pptr,MNAvailPolicy,FALSE,0);

      if ((rptr = MUStrTok(NULL,":",&TokPtr)) != NULL)
        {
        rindex = MUGetIndexCI(rptr,MResourceType,FALSE,0);
        }
      else
        {
        rindex = 0;
        }

      P->NAvailPolicy[rindex] = policyindex;

      if ((EMsg != NULL) && (EMsg[0] == '\0'))
        {
        if ((rptr != NULL) && (rindex == 0))
          { 
          MUSNPrintF(&BPtr,&BSpace,"invalid resource '%s' specified with policy '%s'",
            rptr,
            pptr);

          if (DoEval == TRUE)
            {
            return(FAILURE);
            }
          }
        else if (policyindex == 0)
          {
          MUSNPrintF(&BPtr,&BSpace,"invalid policy '%s' specified with resource '%s'",
            pptr,
            (rptr != NULL) ? rptr : "NULL");

          if (DoEval == TRUE)
            {
            return(FAILURE);
            }
          }
        }    /* END if ((EMsg != NULL) && (EMsg[0] == '\0')) */

      sptr = MUStrTok(NULL," \t\n",&STokPtr);
      }      /* END while (sptr != NULL) */
    }        /* END for (pindex) */

  return(SUCCESS);
  }  /* END MParProcessNAvailPolicy() */





/**
 * Add node to specified partition.
 *
 * @see MNodeSetPartition() - parent
 *
 * NOTE:  realloc's node structures if needed to match destination partition
 *
 * @param P (I)
 * @param N (I) [modified]
 */

int MParAddNode(

  mpar_t  *P,  /* I */
  mnode_t *N)  /* I (modified) */

  {
  const char *FName = "MParAddNode";

  MDB(6,fSTRUCT) MLog("%s(%s,%s)\n",
    FName,
    (P != NULL) ? P->Name : "NULL",
    (N != NULL) ? N->Name : "NULL");

  if ((P == NULL) || (N == NULL))
    {
    return(FAILURE);
    }

  if (MSched.IsClient == TRUE)
    {
    N->PtIndex = P->Index;

    return(SUCCESS);
    }

  if (bmisset(&N->IFlags,mnifIsTemp))
    {
    return(SUCCESS);
    }

  /* NOTE:  multiple RM's may point to single partition if RMCFG[X] PARTITION=X is used */

   if ((P->RepN == NULL) && (N->Name[0] != '\0')) /* Don't use a temporary node as a rep node (see MWikiClusterQuery) */
    {
    /* if representative node not set, set this node as the node attribute template */

    P->RepN = MNode[N->Index];

    MDB(7,fSTRUCT) MLog("INFO:     partition %s representative node set to node %s index %d in MParAddNode\n",
      P->Name,
      N->Name,
      N->Index);
    }

  if (N->RM != NULL)
    {
    if (P->RM != NULL)
      {
      if (P->RM != N->RM)
        {
        if ((N->RM->PtIndex != 0) && (N->RM->PtIndex == P->Index))
          {
          /* node is already member of specified partition */

          /* NO-OP */
          }
        else
          {
          /* node is already member of different partition */

          if (MParRemoveNode(P,N) == FAILURE)
            {
            return(FAILURE);
            }
          }
        }
      }    /* END if (P->RM != NULL) */
    else
      {
      P->RM = N->RM;
      }
    }      /* END if (N->RM != NULL) */

  N->PtIndex = P->Index;

  return(SUCCESS);
  }  /* END MParAddNode() */



/**
 * Lists PAL in [A][B] format.
 *
 * @param PAL
 * @param Buf (I/O) also returned
 */

char *MPALToString(

  mbitmap_t  *PAL,
  const char *Delim,
  char       *Buf)

  {
  mpar_t *P;

  int   pindex;

  char *BPtr = NULL;
  int   BSpace = 0;

  if (bmisclear(PAL))
    {
    MUStrCpy(Buf,NONE,MMAX_LINE);

    return(Buf);
    }

  MUSNInit(&BPtr,&BSpace,Buf,MMAX_LINE);

  /* start at 1, we don't print out the ALL partition */

  for (pindex = 1;pindex < MMAX_PAR;pindex++)
    {
    P = &MPar[pindex];   

    if (bmisset(PAL,pindex) == FALSE)
      continue;

    if (P->Name[0] == '\0')
      break;

    if (P->Name[0] == '\1')
      continue;

    if (bmisset(PAL,pindex))
      {
      if (Delim == '\0')
        MUSNPrintF(&BPtr,&BSpace,"[%s]",P->Name);
      else
        MUSNPrintF(&BPtr,&BSpace,"%s%s",(Buf[0] != '\0') ? Delim : "",P->Name);
      }
    }    /* for (index) */

  return(Buf);
  }  /* END MPALToString() */



/**
 * Convert a string to an mbitmap_t.
 *
 * @param String
 * @param Mode
 * @param PAL
 */

int MPALFromString(

  char                    *String,
  enum MObjectSetModeEnum  Mode,
  mbitmap_t               *PAL)

  {
  mpar_t *P;

  char *ptr;
  char *TokPtr = NULL;

  char  Line[MMAX_LINE];

  /* FORMAT:  <ATTR>[{:|[] \t}<ATTR>]... */

  const char *FName = "MPALFromString";

  MDB(6,fCONFIG) MLog("%s('%s',AttrMap,%d)\n",
    FName,
    String,
    Mode);

  if (PAL != NULL)
    bmclear(PAL);

  if ((MUStrIsEmpty(String)) || (PAL == NULL))
    {
    return(FAILURE);
    }

  strcpy(Line,String);

  /* terminate at ';', '\n', '#' */

  ptr = MUStrTok(Line,";#\n",&TokPtr);

  ptr = MUStrTok(ptr," &^:,[]\t|",&TokPtr);

  while (ptr != NULL)
    {
    if (((Mode == mAdd) && (MParAdd(ptr,&P) == SUCCESS)) ||
        (MParFind(ptr,&P) == SUCCESS))
      {
      bmset(PAL,P->Index);
      }
    else if ((Mode == mVerify) && (MParFind(ptr,&P) == FAILURE))
      {
      return(FAILURE);
      }

    ptr = MUStrTok(NULL," &^:,[]\t|",&TokPtr);
    }  /* END while (ptr != NULL) */

  return(SUCCESS);
  }  /* END MPALFromString() */



/**
 * Remove node from specified partition.
 *
 * @param P (I)
 * @param N (I) [modified]
 */

int MParRemoveNode(

  mpar_t  *P,  /* I */
  mnode_t *N)  /* I (modified) */

  {
  const char *FName = "MParRemoveNode";

  MDB(6,fSTRUCT) MLog("%s(%s,%s)\n",
    FName,
    (P != NULL) ? P->Name : "NULL",
    (N != NULL) ? N->Name : "NULL");

  if ((P == NULL) || (N == NULL))
    {
    return(FAILURE);
    }

  /* NOTE:  multiple RM's may point to single partition if RMCFG[X] PARTITION=X is used */

  if (N->PtIndex != 0)
    N->PtIndex = 0;

  if (P->RepN == N)
    {
    P->RepN = NULL;
    }

  return(SUCCESS);
  }  /* END MParRemoveNode() */



/**
 * Sets PP to the first valid partition in PAL.
 *
 * @param PAL (I)
 * @param PP  (O)
 */

int MParBMGetFirst(

  mbitmap_t     *PAL,
  mpar_t       **PP)

  {
  int pindex;

  mpar_t *P = NULL;

  for (pindex = 1;pindex < MMAX_PAR;pindex++)
    {
    P = &MPar[pindex];   

    if (bmisset(PAL,pindex) == FAILURE)
      continue;

    if (P->Name[0] == '\0')
      break;

    if (P->Name[0] == '\1')
      continue;

    /* partition found */

    if (PP != NULL)
      *PP = P;

    return(SUCCESS);
    }  /* END for (pindex) */ 

  return(FAILURE);
  }  /* END MParBMGetFirst() */
  

 
/**
 * Debug routine to troubshoot preoblem with P->RepN pointer
 * getting corrupted - ticket 3320
 *
 */
void MParCheckRepN(
  int   CallingId)
  {
  int pindex;
  mpar_t *P;

  for (pindex = 0;pindex < MMAX_PAR;pindex++)
    {
    P = &MPar[pindex];

    if (P->Name[0] == '\1')
      continue;

    if (P->Name[0] == '\0')
      break;

    if ((P->RepN != NULL) && (P->RepN->Index > MSched.M[mxoNode]))
      {
      MDB(1,fSTRUCT) MLog("WARNING:  partition %s ReqN corrupt (%d)\n",
        P->Name,
        CallingId);
      }
    }    /* END for (pindex) */
  } /* END MParCheckRepN */


/**
 * Struct used to prioritize partitions.
 */

typedef struct {
  mpar_t *P;
  double  Priority;
} __mpar_prio_t;


/**
 * Sort partition priorties from high to low.
 * 
 * @param A (I)
 * @param B (I)
 */

int __MParPrioCompLToH(

  __mpar_prio_t *A,
  __mpar_prio_t *B)

  {
  return((int)(B->Priority - A->Priority));
  } /* END int __MParPrioCompLToH() */


/**
 * Prioritizes a list of partitions.
 *
 * Will remove ALL,DEFAULT from list and any other partition that is not 
 * feasible right now.
 *
 * This routine is called in different locations and can have different effects depending on
 * PERPARTITIONSCHEDULING.  When TRUE this routine is called before the call to
 * MQueueScheduleIJobs.  It happens in __MSchedSchedulePerPartitionPerJob.  Each partition
 * in the sorted list will then be passed to MQueueScheduleIJobs.  Thus, this routine is only
 * called once for all jobs (not once per job).
 *
 * When PERPARTITIONSCHEDULING is FALSE this routine is called in MQueueScheduleJobs and the
 * prioritization will be on a per-job basis.  Thus this routine is called for each job each
 * iteration.
 *
 * Randomizing the partition list for every job every iteration with PERPARTITIONSCHEDULING 
 * set to true cannot be done as the priority of the job can be different in each partition.
 *
 * Asserts PList is MMAX_PAR + 1
 *
 * @see MParGetPriority
 *
 * @param PList (I) List of partitions to prioritize. NULL terminated.
 * @param J (I) Job to take into account in prioritization. (optional)
 */

int MParPrioritizeList(

  mpar_t **PList,  /* I/O (will be reordered) */
  mjob_t  *J)      /* I (optional) */

  {
  int pindex;
  int priorityCount = 0;

  mpar_t *P;

  __mpar_prio_t PPriorities[MMAX_PAR + 1];

  double PPrio = 0;

  char tmpMsg[MMAX_LINE];

  if (PList == NULL)
    return(FAILURE);
  
  /* Get priorities of all partitions. */
  for (pindex = 0;PList[pindex] != NULL;pindex++)
    {
    P = PList[pindex];

    /* Strip out unwanted partitions */
    if ((P->Index == 0) ||
        (!strcmp(P->Name,"ALL")) ||
        (!strcmp(P->Name,"DEFAULT")))
      continue;

    if (P->ConfigNodes == 0)
      {
      /* partition is empty */

      continue;
      }

    if ((J != NULL) && 
        (MJobCheckParRes(J,P) == FAILURE))
      {
      /* partition doesn't have enough configured resources to support job */

      continue;
      }
    else if ((J != NULL) &&
             ((MSched.SchedPolicy == mscpEarliestStart) ||
              (MSched.SchedPolicy == mscpEarliestCompletion)))
      {
      long EstStartTime = 0;
      mulong  MinStartDelay = 0;
      mulong  MinWallDuration = 0;;

      if (MJobGetEStartTime(
            J,
            &P,             /* I/O */
            NULL,
            NULL,
            NULL,
            &EstStartTime,  /* O (absolute) */
            NULL,
            TRUE,           /* include remote partitions */
            FALSE,          /* check partition availability */
            tmpMsg) == FAILURE)
        {
        MDB(4,fSCHED) MLog("WARNING:  cannot determine earliest start time for job %s in partition '%s' - %s\n",
          J->Name,
          (P != NULL) ? P->Name : "NULL",
          tmpMsg);
   
        continue;
        }
   
      /* calculate pre-requisite data-staging delays */
   
      MSDGetMinStageInDelay(J,P->RM,P->RM->DataRM,&MinStartDelay,&MinWallDuration);
   
      EstStartTime += MinStartDelay;

      MParGetPriority(P,J,EstStartTime,&PPrio,MSched.Time,NULL);
      }
    else
      {
      MParGetPriority(P,J,MSched.Time,&PPrio,MSched.Time,NULL);
      }

    PPriorities[priorityCount].P = P;
    PPriorities[priorityCount++].Priority = PPrio;
    }  /* END for (pindex) */

  PPriorities[priorityCount].P = NULL;

  /* Sort partitions by priority. */
  if (priorityCount > 1)
    qsort(
      (void *)PPriorities,
      priorityCount,
      sizeof(__mpar_prio_t),
      (int(*)(const void *,const void *))__MParPrioCompLToH);


  /* Reorder partition list */
  for (pindex = 0;pindex < priorityCount;pindex++)
    {
    PList[pindex] = PPriorities[pindex].P;
    }

  PList[pindex] = NULL;

  MDB(4,fSCHED)
    {
    MLog("INFO:     reordered partition priority:\t");

    for (pindex = 0;PList[pindex] != NULL;pindex++)
      {
      MLogNH("%s%s",(pindex > 0) ? ", " : "",PList[pindex]->Name);
      }   

    MLogNH("\n");
    }

  return(SUCCESS);
  } /* END int MParPrioritizeList */


/* END MPar.c */

/* vim: set ts=2 sw=2 expandtab nocindent filetype=c: */
