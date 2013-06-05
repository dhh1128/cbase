/* HEADER */

      
/**
 * @file MJobToTString.c
 *
 * Contains:
 *    MJobToTString()
 */


#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 * Translate job structure to trace string for event file.
 *
 * NOTE:  Used to create job event records.
 * NOTE:  sync w/MTraceLoadWorkload()
 *
 * @see XXX() for job checkpointing
 * @see MOWriteEvent() - parent
 * @see MTraceLoadWorkload() - peer - load/parse job trace
 *
 * @param J (I)
 * @param EType (I) [not used]
 * @param Msg (I) optional
 * @param String (I)
 */

int MJobToTString(

  mjob_t     *J,
  enum MRecordEventTypeEnum EType,
  const char *Msg,
  mstring_t  *String)

  {
  int    index;

  char  *ptr;
  char   Features[MMAX_LINE];

  char   QOSString[MMAX_LINE];

  char   ResString[MMAX_LINE];
  char   CmdString[MMAX_LINE];

  char   ASBuf[MMAX_LINE];              /* job template/application simulator buffer */
  char   UList[MMAX_LINE];              /* utilization line */
  char   ELine[MMAX_LINE];              /* estimation line */
  char   RMName[MMAX_NAME];
  char   tmpReqRID[MMAX_LINE];

  mreq_t *RQ[MMAX_REQ_PER_JOB];

  long   tmpStartTime;
  mulong CompletionTime;


  /* generate job trace string */

  const char *FName = "MJobToTString";

  MDB(4,fSTRUCT) MLog("%s(%s,%s,String)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    MRecordEventType[EType]);

  if ((J == NULL) || (String == NULL))
    {
    return(FAILURE);
    }

  ASBuf[0] = '\0';

  RQ[0] = J->Req[0];  /* FIXME */

  if (RQ[0] == NULL)
    {
    return(FAILURE);
    }

  mstring_t ReqHList(MMAX_LINE);
  mstring_t RMXString(MMAX_LINE);
  mstring_t AllocHosts(MMAX_BUFFER);


  MUNodeFeaturesToString('\0',&RQ[0]->ReqFBM,Features);

  if (MUStrIsEmpty(Features))
    strcpy(Features,"-");  /* NOTE:  was '[-]' */

  /* initialize trace data */

  sprintf(ResString,"%7.2f",
    J->PSUtilized);

  tmpStartTime = J->StartTime;

  if (J->Ckpt != NULL)
    tmpStartTime -= J->Ckpt->StoredCPDuration;

 
  CmdString[0] = '\0';

  if (J->Env.Cmd != NULL)
    {
    MUStrCpy(CmdString,J->Env.Cmd,sizeof(CmdString));

    while ((ptr = strchr(CmdString,' ')) != NULL)
      {
      *ptr = '_';
      }
    }

  if (bmisset(&J->IFlags,mjifHostList) &&
      (!MNLIsEmpty(&J->ReqHList)))
    {
    MNLToMString(&J->ReqHList,MBNOTSET,",",'\0',-1,&ReqHList);
    }
  else
    {
    MStringSet(&ReqHList,"-");
    }

  /* NOTE:  for workload version 510 and higher, report as taskmap, otherwise report as 
            list of allocated nodes */

  if (((J->DestinationRM != NULL) && bmisset(&J->DestinationRM->Flags,mrmfCompressHostList)) ||
           ((J->SubmitRM != NULL) && bmisset(&J->SubmitRM->Flags,mrmfCompressHostList)))
    {
    MUNLToRangeString(
      &J->NodeList,
      NULL,
      -1,
      TRUE,
      TRUE, /* sort node list */
      &AllocHosts);
    }
  else if (J->TaskMapSize > 0)
    {
    for (index = 0;J->TaskMap[index] != -1;index++)
      {
      MStringAppendF(&AllocHosts,"%s%s",
        (!AllocHosts.empty()) ? "," : "",
        MNode[J->TaskMap[index]]->Name);
      }  /* END for (index) */
    }

  if (J->Credential.Templates != NULL)
    {
    int jindex;

    char *tBPtr;
    int   tBSpace;

    tBPtr = ASBuf;
    tBSpace = sizeof(ASBuf);

    MUSNUpdate(&tBPtr,&tBSpace);

    MUSNPrintF(&tBPtr,&tBSpace,"%stemplates=",
      (tBPtr != ASBuf) ? "," : "");

    for (jindex = 0;J->Credential.Templates[jindex] != NULL;jindex++)
      {
      if (jindex >= MSched.M[mxoxTJob])
        break;

      MUSNPrintF(&tBPtr,&tBSpace,"%s%s",
        (jindex > 0) ? "," : "",
        J->Credential.Templates[jindex]);
      }  /* END for (jindex) */
    }    /* END if (J->Cred.Templates != NULL) */
 
  if (ASBuf[0] == '\0')
    strcpy(ASBuf,"-");

  if (AllocHosts.empty())
    MStringSet(&AllocHosts,"-");

  sprintf(QOSString,"%s:%s",
    (J->QOSRequested != NULL) ? J->QOSRequested->Name : "-",
    (J->Credential.Q != NULL) ? J->Credential.Q->Name : "-");

  if (J->SubmitRM != NULL)
    strcpy(RMName,J->SubmitRM->Name);
  else
    strcpy(RMName,"internal");

  if (J->CompletionTime > 0)
    CompletionTime = J->CompletionTime;
  else
    CompletionTime = MSched.Time;

  MRMXToMString(J,&RMXString,&J->RMXSetAttrs);

  if (!RMXString.empty())
    {
    char *mutableString = NULL;

    MUStrDup(&mutableString,RMXString.c_str());

    MUStrReplaceChar(mutableString,'\0','_',FALSE);

    RMXString = mutableString;

    MUFree(&mutableString);
    } 
  else
    {
    MStringSet(&RMXString,"-");
    }

  if (J->ReqRID != NULL)
    {
    sprintf(tmpReqRID,"%s%s",
      (bmisset(&J->IFlags,mjifNotAdvres)) ? "!" : "",
      J->ReqRID);
    }

  mstring_t FlagLine(MMAX_LINE);

  MJobFlagsToMString(NULL,&J->SpecFlags,&FlagLine);

  /* report attributes common to all trace formats */

  /*                     NRQ TRQ UID GID CPU STA  CLAS QUET DIST STAT COMT ARC OPS MCMP MEM  DCMP DSK  FEAT SQUT TAS TPN QREQ FLG ACC CMD  CC  BYP NSUTL PAR DPROCS DMEM DDISK DSWAP STARTDATE ENDDATE MNODE RMINDEX HOSTLIST RSVID ASNAME RES1 RES2 */
  MStringAppendF(String,"%3d %3d %8s %9s %7ld %9s %11s %9lu %9lu %9lu %9lu %6s %6s %-2s %4dM %-2s %6dM %10s %9lu %3d %4d %s %5s %9s %15s %4s %3d %s %9s %6d %6dM %6dM %6dM %9lu %9lu %s %s %s %s %s",
    J->Request.NC,                                         /* %3d NRQ */
    J->Request.TC,                                         /* %3d TRQ */
    (J->Credential.U != NULL) ? J->Credential.U->Name : "-",           /* %8s UID */
    (J->Credential.G != NULL) ? J->Credential.G->Name : "-",           /* %9s GID */
    J->SpecWCLimit[0],                                     /* %7ld CPU */
    MJobState[J->State],                                   /* %9s STA */
    (J->Credential.C != NULL) ? J->Credential.C->Name : "-",           /* %11s CLAS */
    (unsigned long)J->SubmitTime,                          /* %9lu QUET */
    (unsigned long)J->DispatchTime,                        /* %9lu DIST */
    (unsigned long)tmpStartTime,                           /* %9lu STAT */
    CompletionTime,                                        /* %9lu COMT */
    (RQ[0]->Arch > 0) ? MAList[meArch][RQ[0]->Arch] : "-", /* %6s ARC */
    (RQ[0]->Opsys > 0) ? MAList[meOpsys][RQ[0]->Opsys] : "-", /* %6s OPS */
    MComp[(int)RQ[0]->ReqNRC[mrMem]],                      /* %-2s MCMP */
    RQ[0]->ReqNR[mrMem],                                   /* %4dM MEM */
    MComp[(int)RQ[0]->ReqNRC[mrDisk]],                     /* %-2s DCMP */
    RQ[0]->ReqNR[mrDisk],                                  /* %6dM DSK */
    Features,                                              /* %10s FEAT */
    (unsigned long)J->SystemQueueTime,                     /* %9lu SQUT */
    J->Alloc.TC,                                           /* %3d TAS */
    RQ[0]->TasksPerNode,                                   /* %4d TPN */
    QOSString,                                             /* %s QREQ */
    (!MUStrIsEmpty(FlagLine.c_str())) ? FlagLine.c_str() : "-",                /* %5s FLG */
    (J->Credential.A != NULL) ? J->Credential.A->Name : "-",           /* %9s ACC */
    (CmdString[0] != '\0') ? CmdString : "-",              /* %15s CMD */
    RMXString.c_str(),                                     /* %4s RMXString */
    J->BypassCount,                                        /* %3d BYP */
    ResString,                                             /* %s PSUTL */
    MPar[RQ[0]->PtIndex].Name,                             /* %9s PAR */
    RQ[0]->DRes.Procs,                                     /* %6d DPROCS */
    RQ[0]->DRes.Mem,                                       /* %6dM DMEM */
    RQ[0]->DRes.Disk,                                      /* %6dM DDISK */
    RQ[0]->DRes.Swap,                                      /* %6dM DSWAP */
    (unsigned long)J->SpecSMinTime,                        /* %9lu STARTDATE*/
    (unsigned long)J->CMaxDate,                            /* %9lu ENDDATE */
    AllocHosts.c_str(),                                    /* %s MNODE */
    (RMName[0] != '\0') ? RMName : "-",                    /* %s RMID */
    ReqHList.c_str(),                                      /* %s HOSTLIST */
    (J->ReqRID != NULL) ? tmpReqRID : "-",                 /* %s RSVID */
    ASBuf);                                                /* %s ASBUF */


  {
  char HistLine[MMAX_LINE];
  char tmpMsg[MMAX_BUFFER];  /* must be large enough to handle all messages concatenated */
  char ExtMem[MMAX_LINE];
  char ExtCPU[MMAX_LINE];

  char OLine[MMAX_LINE];   /* other:  completion code, session id, alloc partition id, etc */
  char GMetric[MMAX_LINE];
  char EffQueueDuration[MMAX_LINE];

  /* NOTE:  switch to reporting non-verbose J->MB (NYI) */

  if (J->MessageBuffer != NULL)
    {
    /* report job messages */

    char tmpLine[MMAX_LINE << 2];

    MMBPrintMessages(
      J->MessageBuffer,
      mfmNONE,
      TRUE,             /* verbose */
      -1,
      tmpLine,          /* O */
      sizeof(tmpLine));

    MUStringPack(tmpLine,tmpMsg,sizeof(tmpMsg));
    }
  else
    {
    strcpy(tmpMsg,"-");
    }

  UList[0] = '\0';
  ELine[0] = '\0';

  MUStrCpy(ExtMem,"-",sizeof(ExtMem));
  MUStrCpy(ExtCPU,"-",sizeof(ExtCPU));
  MUStrCpy(GMetric,"-",sizeof(GMetric));

  MUStrCpy(HistLine,"-",sizeof(HistLine));

  sprintf(OLine,"%d",
    J->CompletionCode);

  {
  char *AllocPar;
  char *UPtr;
  int   USpace;

  enum MNodeAccessPolicyEnum NAccessPolicy;

  MUSNInit(&UPtr,&USpace,UList,sizeof(UList));

  UPtr = OLine;
  USpace = sizeof(OLine);

  MUSNUpdate(&UPtr,&USpace);

  if (J->SessionID > 0)
    {
    MUSNPrintF(&UPtr,&USpace,",SID=%d",
      J->SessionID);
    }

  if (MUHTGet(&J->Variables,"ALLOCPARTITION",(void **)&AllocPar,NULL) == SUCCESS)
    {
    MUSNPrintF(&UPtr,&USpace,",ALLOCPAR=%s",
      AllocPar);
    }

  if (MSched.StoreSubmission == TRUE)
    {
    char tmpPath[MMAX_PATH_LEN];

    MJobGetIWD(J,tmpPath,sizeof(tmpPath),TRUE);

    MUSNPrintF(&UPtr,&USpace,",IWD=%s",
      tmpPath);

    if (J->Env.Output != NULL)
      {
      MJobResolveHome(J,J->Env.Output,tmpPath,sizeof(tmpPath));

      MUSNPrintF(&UPtr,&USpace,",OutputFile=%s",
        tmpPath);
      }
    }

  if ((J->Req[0] != NULL) &&
      (J->Req[0]->PtIndex >= 0) &&
      (J->TaskMapSize > 0) &&
      (J->TaskMap != NULL) &&
      (J->TaskMap[0] >= 0))
    {
    MJobGetNAPolicy(J,NULL,&MPar[J->Req[0]->PtIndex],MNode[J->TaskMap[0]],&NAccessPolicy);
    }
  else
    {
    NAccessPolicy = mnacNONE;
    }

  if (NAccessPolicy != mnacNONE)
    {
    MUSNPrintF(&UPtr,&USpace,",NAccessPolicy=%s",
      MNAccessPolicy[NAccessPolicy]); 
    }
    
  if (RQ[0]->XURes != NULL)
    {
    char *BPtr;
    int   BSpace;

    int gmindex;

    double AvgFactor;

    MUSNInit(&BPtr,&BSpace,GMetric,sizeof(GMetric));

    for (gmindex = 1;gmindex < MSched.M[mxoxGMetric];gmindex++)
      {
      if (RQ[0]->XURes->GMetric[gmindex] == NULL)
        continue;

      if ((RQ[0]->XURes->GMetric[gmindex]->Min <= 0.0) ||
          (RQ[0]->XURes->GMetric[gmindex]->Max <= 0.0) ||
          (RQ[0]->XURes->GMetric[gmindex]->Avg <= 0.0))
        continue;

      /* NOTE:  GMetric Avg is SUM(<GMETRICVAL>*<MSched.Interval>/J->Alloc.NC) */

      AvgFactor = (double)(RQ[0]->XURes->GMetric[gmindex]->Avg * J->TotalProcCount / 
                             MAX(1,J->PSDedicated));  

      MUSNPrintF(&BPtr,&BSpace,"%sgmetric[%s]=%.2f",
        (GMetric[0] != '\0') ? "," : "",
        MGMetric.Name[gmindex + 1],
        AvgFactor / 100.0);
      }  /* END for (gmindex) */
    }    /* END if (RQ[0]->XURes != NULL) */
  }      /* END BLOCK */

  if (UList[0] == '\0')
    strcpy(UList,"-");

  if (ELine[0] == '\0')
    strcpy(ELine,"-");

  if (GMetric[0] == '\0')
    strcpy(GMetric,"-");

  if (J->EffQueueDuration > 0)
    snprintf(EffQueueDuration,sizeof(EffQueueDuration),"%ld",J->EffQueueDuration);
  else
    snprintf(EffQueueDuration,sizeof(EffQueueDuration),"0");
  
  /* FORMAT:  nodeset msg cost utl est ccode xmem xcpu */

  MStringAppendF(String," %s %s %.2f %s %s %s %s %s %s %s %s",
    "-",
    tmpMsg,
    J->Cost,
    HistLine,
    UList,
    ELine,
    OLine,
    ExtMem,
    ExtCPU,
    GMetric,
    EffQueueDuration);

  if ((MSched.StoreSubmission == TRUE) && (J->RMSubmitString != NULL))
    {
    mstring_t tmpSubmitString(MMAX_LINE);
    
    MUMStringPack(J->RMSubmitString,&tmpSubmitString);

    MStringAppendF(String," %s",
      tmpSubmitString.c_str());
    }
  }

  if (Msg != NULL)
    {
    MStringAppendF(String," MSG='%s'",
      Msg);
    }

  /* Display 'VMID=<vm name>' from J->System->VM */

  if ((J->System != NULL) && (J->System->VM != NULL))
    {
    MStringAppendF(String," %s=%s",
      MWikiJobAttr[mwjaVMID],
      J->System->VM);
    }

  /* terminate record */

  MStringAppend(String,"\n");

  return(SUCCESS);
  }  /* END MJobToTString() */



/* END MJobToTString.c */
