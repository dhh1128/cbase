/* HEADER */

      
/**
 * @file MSysDoTest.c
 *
 * Contains: Internal test functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  




int __MSysTestGeo(

  char *Geometry)

  {
  if ((Geometry == NULL) || (Geometry[0] == '\0'))
    {
    const char *String = "{(0,1) (3) (5,4) (2)}";

    MJobParseGeometry(NULL,String);
    }
  else
    {
    MJobParseGeometry(NULL,Geometry);
    }

  exit(0);
  }  /* END __MSysTestGeo() */

/**
 *
 *
 * @param XMLString (I)
 */

int __MSysTestXML(
 
  char *XMLString)  /* I */

  {
  char *tail;

  FILE *fp;

  mxml_t *E = NULL;

  char EMsg[MMAX_LINE];

  char buffer[MMAX_BUFFER << 3];

  if ((XMLString != NULL) && (XMLString[0] != '\0'))
    {
    if (XMLString[0] == '/')
      {
      int rc;

      fp = fopen(XMLString,"r");

      rc = fread((void *)buffer,sizeof(buffer),1,fp);

      XMLString = buffer;

      fprintf(stdout,"read in %d\n",rc);
      }

    fprintf(stdout,"XMLString: '%s'\n\n",
      XMLString);
  
    if (MXMLFromString(&E,XMLString,&tail,EMsg) == FAILURE)
      {
      fprintf(stderr,"ERROR: '%s'\n",
        EMsg);
  
      exit(1);
      }
  
    if (tail != NULL)
      {
      fprintf(stdout,"tail: '%s'\n\n",
        XMLString);
      }

    exit(0);
    }

  MXMLCreateE(&E,"Data");
  MXMLSetAttr(E,"Value",(void *)"this >> that",mdfString);

  MXMLToString(E,EMsg,sizeof(EMsg),NULL,TRUE);

  fprintf(stdout,"XML = '%s'\n\n",
    EMsg);

  MXMLDestroyE(&E);

  MXMLFromString(&E,EMsg,NULL,NULL);

  fprintf(stdout,"After MXMLFromString: <Data %s=\"%s\"></Data>\n\n",
    E->AName[0],
    E->AVal[0]);

  EMsg[0] = '\0';

  MXMLToString(E,EMsg,sizeof(EMsg),NULL,TRUE);

  fprintf(stdout,"XML = '%s'\n\n",
    EMsg);

  exit(1);

  /* NOTREACHED */

  return(SUCCESS);
  }  /* END __MSysTestXML() */


  


int __MSysTestRLMerge()
 
  {
#define TEST2 1

#ifdef TEST1
  mrange_t R1[] = {
    {1136915988,1136923188,792,792,0,0,0,NULL,NULL},
    {1136923188,1136930602,793,793,0,0,0,NULL,NULL},
    {1136930602,1136942562,794,794,0,0,0,NULL,NULL},
    {1136942562,1136945883,795,795,0,0,0,NULL,NULL},
    {1136945883,1136955162,797,8,0,0,0,NULL,NULL},
    {1136955162,1136955164,798,8,0,0,0,NULL,NULL},
    {1136955164,1136984381,799,8,0,0,0,NULL,NULL},
    {1136984381,2139992800,800,8,0,0,0,NULL,NULL},
    {0,0,0,0,0,0,0,NULL,NULL} };

  mrange_t R2[] = {
    {888736699,888763698,1,1,0,0,0,NULL,NULL},
    {888763709,888763709,1,1,0,0,0,NULL,NULL},
    {888763711,2139999999,1,1,0,0,0,NULL,NULL},
    {0,0,0,0,0,0,0,NULL,NULL} };

  MRLMerge(R1,R2,1,FALSE,NULL); 
#endif

#ifdef TEST2old
  mrange_t R1[] = {
    {120,120,2,2,0,0,0,NULL,NULL},
    {120,420,1,1,0,0,0,NULL,NULL},
    {420,420,2,2,0,0,0,NULL,NULL},
    {420,600,1,1,0,0,0,NULL,NULL},
    {660,720,1,1,0,0,0,NULL,NULL},
    {0,0,0,0,0,0,0,NULL,NULL} };

  mrange_t R2[] = {
    {60,300,1,1,0,0,0,NULL,NULL},
    {300,420,2,2,0,0,0,NULL,NULL},
    {0,0,0,0,0,0,0,NULL,NULL} };

  MSched.Time = 0;

  fprintf(stderr,"R1:\n%s\n",
    MRLShow(R1,FALSE,FALSE));

  fprintf(stderr,"R2:\n%s\n",
    MRLShow(R2,FALSE,FALSE));

  MRLMerge(R1,R2,1,FALSE,NULL);

  fprintf(stderr,"Range:\n%s\n",
    MRLShow(R1,FALSE,FALSE));
#endif

#ifdef TEST2
  mrange_t R1[] = {
/*
    {120,180,1,1,0,0,0,NULL,NULL},
    {180,240,2,2,0,0,0,NULL,NULL},
    {240,360,3,3,0,0,0,NULL,NULL},
*/
    {360,420,1,1,0,0,0,NULL,NULL},
    {420,420,4,4,0,0,0,NULL,NULL},
/*
    {420,480,1,1,0,0,0,NULL,NULL},
*/
    {0,0,0,0,0,0,0,NULL,NULL} };

  mrange_t R2[] = {
    {60,3000,1,1,0,0,0,NULL,NULL},
    {0,0,0,0,0,0,0,NULL,NULL} };

  MSched.Time = 0;

  fprintf(stderr,"R1:\n%s\n",
    MRLShow(R1,FALSE,FALSE));

  fprintf(stderr,"R2:\n%s\n",
    MRLShow(R2,FALSE,FALSE));

  MRLMerge(R1,R2,1,FALSE,NULL);

  fprintf(stderr,"Range:\n%s\n",
    MRLShow(R1,FALSE,FALSE));
#endif

#ifdef TEST3
  mrange_t RRange[] = {
    {0,MMAX_TIME,1,1,0,0,0,NULL,NULL},
    {0,0,0,0,0,0,0,NULL,NULL} };

  mrange_t R1[] = {
    {1136915988,1136923188,792,792,0,0,0,NULL,NULL},
    {1136923188,1136930602,793,793,0,0,0,NULL,NULL},
    {1136930602,1136942562,794,794,0,0,0,NULL,NULL},
    {1136942562,1136945883,795,795,0,0,0,NULL,NULL},
    {1136945883,1136955162,797,8,0,0,0,NULL,NULL},
    {1136955162,1136955164,798,8,0,0,0,NULL,NULL},
    {1136955164,1136984381,799,8,0,0,0,NULL,NULL},
    {1136984381,2139992800,800,8,0,0,0,NULL,NULL},
    {0,0,0,0,0,0,0,NULL,NULL} };

  MRLMergeTime(
    R1,
    RRange,
    TRUE,
    -1,             /* maximize range merging */
    0);
#endif

/*
  mrange_t R1[] = {
    {1000000000,1000010000,22,22},
    {1000010000,1000030000,30,30},
    {1000030000,1000035000,33,33},
    {1000045000,2140000000,17,17},
    {0,0,0,0} };
*/

/*
  mrange_t R1[] = {
    {956601443,956612113,22,22},
    {956612113,956631180,30,30},
    {956631180,956635459,33,33},
    {956712600,956766449,17,17},
    {956766449,956770944,119,119},
    {956770944,956810762,131,131},
    {956810762,956981059,133,133},
    {956981059,957198515,183,183},
    {957198515,2139740800,187,187}, 
    {0,0,0,0}
    };
*/

  /* 
  mrange_t R2[] = {
    {956635459,956635459,1,1},
    {956981059,2139740800,1,1},
    {0,0,0,0}
    };
  */
 
  /*
  mrange_t R1[] = {
    {888709099,888736699,4,4},
    {888736699,888741199,7,7},
    {888741199,888763698,19,19},
    {888763709,888763709,19,19},
    {888763709,888763711,171,171},
    {888763711,2139999999,190,190},
    {0,0,0,0}
    };
 
  mrange_t R2[] = {
    {888736699,888763698,1,1},
    {888763709,888763709,1,1},
    {888763711,2139999999,1,1},
    {0,0,0,0}
    };
  */
 
  exit(0);

  /*NOTREACHED*/

  return(SUCCESS);
  }  /* END __MSysTestRLMerge() */





int __MSysTestJobSelectFRL()

  {
#ifdef MNOT /* to avoid compile warnings */
  mjob_t tmpJ;
  mreq_t tmpRQ;

  int    RCount;

  mrange_t R1[] = {
    { 1022172521, 1022173705, 476, 119 },
    { 1022173705, 1022176944, 496, 124 },
    { 1022176944, 1022179074, 500, 125 },
    { 1022179074, 1022182393, 516, 129 },
    { 1022182393, 1022185130, 520, 130 },
    { 1022185130, 1022188330, 536, 134 },
    { 1022188330, 1022189431, 600, 150 },
    { 1022189431, 1022208469, 664, 166 },
    { 1022208469, 1022211377, 680, 170 },
    { 1022211377, 1022215080, 728, 182 },
    { 1022215080, 1022250175, 744, 186 },
    { 1022250175, 1022252002, 748, 187 },
    { 1022252002, 1022252096, 752, 188 },
    { 1022252096, 1022253053, 756, 189 },
    { 1022253053, 1022255920, 804, 201 },
    { 1022255920, 2139992800, 808, 202 },
    { 0, 0, 0, 0 } };

  memset(&tmpJ,0,sizeof(tmpJ));
  memset(&tmpRQ,0,sizeof(tmpRQ));

  strcpy(tmpJ.Name,"test");

  tmpJ.Request.TC = 512;
  tmpJ.Request.NC = 128;
 
  tmpRQ.TaskCount = 512;
  tmpRQ.NodeCount = 128;
 
  MJobSelectFRL(&tmpJ,&tmpRQ,R1,1,&RCount);

  exit(0);

  /*NOTREACHED*/
#endif /* MNOT */
  return(SUCCESS);
  }  /* END __MSysTestJobSelectFRL() */




int __MSysTestNPrioF()

  {
  mnode_t tmpN;

  memset(&tmpN,0,sizeof(tmpN));

  MProcessNAPrioF(&tmpN.P,"6*LOAD + -.01 * CMEM - JOBCOUNT");

  exit(0);
 
  /*NOTREACHED*/
  
  return(SUCCESS);
  }  /* END __MSysTestNPrioF() */




int __MSysTestRLAND()

  {
  mrange_t R1[] = {
    { 1016704575, 2139856000, 4, 1, 0, 0, 0, NULL, NULL },
/*
    {1,3,1,1},
    {5,6,1,1},
    {6,7,2,2},
    {11,13,2,2},
    {13,14,1,1},
    {15,16,1,1},
    {17,18,1,1},
*/
    {0,0,0,0,0,0,0,NULL,NULL }
    };
 
  mrange_t R2[] = {
    {1016704575, 2139856000, 4, 1, 0, 0, 0, NULL, NULL },
/*
    {0,2,1,1},
    {4,6,1,1},
    {8,9,1,1},
    {10,11,1,1},
    {13,14,1,1},
    {14,15,1,1},
    {15,16,1,1},
    {16,17,1,1},
*/
    {0,0,0,0,0,0,0,NULL,NULL }
    };

  mrange_t C[MMAX_RANGE];
 
  /*
  mrange_t R1[] = {
    {888709099,888736699,4,4},
    {888736699,888741199,7,7},
    {888741199,888763698,19,19},
    {888763709,888763709,19,19},
    {888763709,888763711,171,171},
    {888763711,2139999999,190,190},
    {0,0,0,0}
    };
 
  mrange_t R2[] = {
    {888736699,888763698,1,1},
    {888763709,888763709,1,1},
    {888763711,2139999999,1,1},
    {0,0,0,0}
    };
  */ 
 
  MRLAND(C,R1,R2,FALSE);
 
  exit(0);

  /*NOTREACHED*/
 
  return(SUCCESS);
  }  /* END __MSysTestRLAND() */





int __MSysTestMNodeBuildRE()

  {
  /* NYI */

  return(SUCCESS);
  }  /* END __MSysTestMNodeBuildRE() */




int __MSysTestJobGetSNRange()
 
  {
#if 0
  mjob_t   tmpJ;
  mnode_t  tmpN;
 
  mre_t   RE[MMAX_RSV_DEPTH << 1];
  mrsv_t *R[MMAX_RSV_DEPTH];
  int     RC[MMAX_RSV_DEPTH];
 
  mreq_t  tmpRQ;
 
  char tmpLine[MMAX_LINE];

  mrange_t GRange[MMAX_RANGE];
  mrange_t ARange[MMAX_RANGE];
 
  char    Affinity;

  enum MResourceAvailabilityPolicyEnum NAvailPolicy[mrLAST];
 
  memset(&tmpJ,0,sizeof(tmpJ));
  memset(&tmpN,0,sizeof(tmpN));
  memset(&tmpRQ,0,sizeof(tmpRQ));
  memset(NAvailPolicy,0,sizeof(NAvailPolicy));
 
  /* configure general */
 
  MSched.Time     = 1213931845;
  MSched.MaxRsvPerNode = 8;
/*
  MSched.RsvSearchAlgo = mrsaWide;
*/

  mlog.logfp     = stderr;
  mlog.Threshold = 8;
 
  /* configure reservation */

  /* now = 1213931845 */
  /* wclimit = 1440000 */
     
  MRsvInitialize(&MRsv[0],NULL,"430806",FALSE);

  MRsv[0]->Type       = mrtJob;
  MRsv[0]->StartTime  = 1213931844;
  MRsv[0]->EndTime    = 1215371844;
  MRsv[0]->DRes.Procs = 1;
  bmclear(&MRsv[0]->Flags);
  MRsv[0]->J          = &tmpJ;

  MRsvInitialize(&MRsv[1],NULL,"elston.437",FALSE);

  MRsv[1]->Type       = mrtUser;
  MRsv[1]->StartTime  = 0;
  MRsv[1]->EndTime    = MMAX_TIME - 1;
  MRsv[1]->DRes.Procs = 8;
  bmclear(&MRsv[1]->Flags);
  MUStrDup(&MRsv[1]->RsvGroup,"elston");

  MUStrCpy(tmpLine,"USER==wightman",sizeof(tmpLine));

  MACLLoadConfigLine(&MRsv[1]->ACL,tmpLine,mSet,NULL,FALSE);

  /* configure node */

  strcpy(tmpN.Name,"tango065");

  tmpN.CRes.Procs = 8;
  tmpN.ARes.Procs = 8;

  memcpy(tmpN.NAvailPolicy,NAvailPolicy,sizeof(tmpN.NAvailPolicy));

  tmpN.NAvailPolicy[mrProc] = mrapDedicated;

  /* link node */

  tmpN.R  = R;
  tmpN.RE = RE;
  tmpN.RC = RC;

  tmpN.R[0]  = MRsv[1];
  tmpN.RC[0] = 1;

  tmpN.R[1]  = MRsv[0];
  tmpN.RC[1] = 8;

  tmpN.RE[0].Type = mreStart;
  tmpN.RE[0].BRes.Procs = 1;
  tmpN.RE[0].Time = 1213931845 - 1;
  tmpN.RE[0].Index = 1;

  tmpN.RE[1].Type = mreStart;
  tmpN.RE[1].BRes.Procs = 0;
  tmpN.RE[1].Time = 1213931845;
  tmpN.RE[1].Index = 0;

  tmpN.RE[2].Type = mreEnd;
  tmpN.RE[2].BRes.Procs = 8;
  tmpN.RE[2].Time = 1213931845 - 1 + 1440000;
  tmpN.RE[2].Index = 1;

  tmpN.RE[3].Type = mreEnd;
  tmpN.RE[3].BRes.Procs = 0;
  tmpN.RE[3].Time = 1213931845 - 1 + 1440000;
  tmpN.RE[3].Index = 0;

  tmpN.RE[4].Type = mreStart;
  tmpN.RE[4].BRes.Procs = 8;
  tmpN.RE[4].Time = 1213931845 - 1 + 1440000;
  tmpN.RE[4].Index = 0;

  tmpN.RE[5].Type = mreEnd;
  tmpN.RE[5].BRes.Procs = 8;
  tmpN.RE[5].Time = MMAX_TIME - 1;
  tmpN.RE[5].Index = 0;

  tmpN.RE[6].Type = mreNONE;

  MNode[0] = &tmpN;

  /* configure job */

  strcpy(tmpJ.Name,"430806");

  tmpJ.Req[0]         = &tmpRQ;
  tmpJ.WCLimit        = 1440000;
  tmpJ.SpecWCLimit[0] = 1440000;

  tmpJ.Request.TC = 8;

  MACLSet(&tmpJ.Cred.CL,maUser,(void *)"wightman",mcmpSEQ,mnmPositiveAffinity,0,FALSE);

  bmset(&tmpJ.Flags,mjfAdvRsv);

  MUStrDup(&tmpJ.ReqRID,"elston");

  tmpRQ.DRes.Procs = 1;

  /* configure range requirements */

  GRange[0].TC = 8;
  GRange[0].NC = 0;

  GRange[0].STime = 1213931845;
  GRange[0].ETime = 1213931845 + 1440000;

  GRange[1].ETime = 0;


  MJobGetSNRange(
    &tmpJ,
    &tmpRQ, 
    &tmpN,
    GRange,
    MMAX_RANGE,
    &Affinity,
    NULL,
    ARange,
    NULL,
    FALSE,
    NULL);

  exit(0);

  /*NOTREACHED*/
 
#endif /* 0 */

  return(SUCCESS);
  }  /* __MSysTestJobGetSNRange() */



int __MSysTestMacAddress()

  {
  char MacAddress[MMAX_LINE];

  if (MGetMacAddress(MacAddress) == SUCCESS)
    {
    fprintf(stdout,"Mac: %s\n",MacAddress);
    }
  else
    {
    fprintf(stderr,"could not get mac address\n");
    }

  exit(1);
  }



int __MSysTestSpawn()

  {
  char *OBuf;
  char *EBuf;

  int  PID;
  int  SC;

  int  rc;

  rc = MUSpawnChild(
    "/tmp/test.cmd",
    "testprog",
    "-lnodes=1:ppn=2",
    "/bin/ksh",
    508,
    508,
    "$HOME/tmp",
    "XXX=YYY;ABC=GHI",
    NULL,
    NULL,
    "#PBS -l walltime=1:00:00\n/bin/sleep 600\n",
    NULL,
    &OBuf,
    &EBuf,
    &PID,
    &SC,
    20000,
    0,
    NULL,
    mxoNONE,
    FALSE,
    FALSE,
    NULL);
 
  if (rc == FAILURE)
    {
    fprintf(stdout,"NOTE:  child launch failed\n");

    exit(1);
    }

  fprintf(stdout,"NOTE:  child launched (O: '%s'  E: '%s'  PID: %d  SC: %d)\n",
    OBuf,
    EBuf,
    PID,
    SC);

  exit(0);

  return(SUCCESS);
  }  /* END __MSysTestSpawn() */


/**
 * Perform internal unit testing.
 */

int MSysDoTest()
 
  {
  char *tptr;
  char *aptr;
  char *ptr;

  int   aindex;
  int   rc;

  char  tmpLine[MMAX_LINE];

  const char *TName[] = {
    NONE,
    "COMPRESS",
    "SCHED",
    "XML",
    "RANGEAND",
    "RANGECOLLAPSE",
    "RANGEMERGE",
    "GETSNRANGE",
    "NODEPRIO",
    "SPAWN",
    "WIKINODE",
    "WIKIJOB",
    "WIKICLUSTER",
    "RMX",
    "JOBNAME",
    "JOBDIST",
    "CHECKSUM",
    "LLSUBMIT",
    "LLQUERY",
    "MAIL",
    "PBSPARSE",
    "PBSPARSE2",
    "NODERANGE",
    "BGRANGE",
    "ODBCTEST",
    "SQLITE3TEST",
    "STRINGTEST",
    "RMXTEST",
    "STATCONVTEST",
    "GEOTEST",
    "DISATEST",
    "MACADDRESS",
    NULL };

  enum {
    mirtNONE = 0,
    mirtCompress,
    mirtSched,
    mirtXML,
    mirtRLAND,
    mirtJobSelectFRL,
    mirtRLMerge,
    mirtJobGetSNRange,
    mirtNodePrio,
    mirtSpawn,
    mirtWikiNode,
    mirtWikiJob,
    mirtWikiCluster,
    mirtRMExtension,
    mirtJobName,
    mirtJobDist,
    mirtChecksum,
    mirtLLSubmit,
    mirtLLQuery,
    mirtMail,
    mirtPBSParse,
    mirtPBSParse2,
    mirtNodeRange,
    mirtBGRange,
    mirtODBCTest,
    mirtSQLite3Test,
    mirtStringTest,
    mirtRMX,
    mirtStatConvTest,
    mirtGeoTest,
    mirtDisaTest,
    mirtMacAddress,
    mirtLAST };
 
  if ((tptr = getenv(MSCHED_ENVTESTVAR)) == NULL)
    {
    return(SUCCESS);
    }

  MUStrCpy(tmpLine,tptr,sizeof(tmpLine));

  aindex = MUGetIndexCI(tmpLine,TName,TRUE,0);

  aptr = NULL;

  if ((ptr = strchr(tmpLine,':')) != NULL)
    {
    aptr = ptr + 1;
    }

  fprintf(stderr,"INFO:     running test %s, data='%s'\n",
    TName[aindex],
    (aptr != NULL) ? aptr : "");

  switch (aindex)
    {
    case mirtCompress:

      MUCompressTest();

      break;

    case mirtXML:

      __MSysTestXML(aptr);

      break;

    case mirtRLAND:

      __MSysTestRLAND(); 

      break;

    case mirtGeoTest:

      __MSysTestGeo(aptr);

      break;

    case mirtJobSelectFRL:

      __MSysTestJobSelectFRL();

      break;

    case mirtRLMerge:
 
      __MSysTestRLMerge(); 

      break;

    case mirtJobGetSNRange:

      __MSysTestJobGetSNRange();

      break;

    case mirtMacAddress:

      __MSysTestMacAddress();

      break;

    case mirtNodePrio:

      __MSysTestNPrioF();

      break;

    case mirtSpawn:

      __MSysTestSpawn();

      break;

    case mirtWikiNode:

      MWikiTestNode(aptr);

      break;

    case mirtWikiJob:

      MWikiTestJob(aptr);

      break;

    case mirtWikiCluster:

      MWikiTestCluster(aptr);

      break;

    case mirtRMExtension:

      MJobTestRMExtension(aptr);

      break;

    case mirtJobName:

      MJobTestName(aptr);

      break;

    case mirtChecksum:

      MSecTestChecksum(aptr);

      break;

    case mirtLLSubmit:

      {
      int JC;

      char *ptr;
      char *TokPtr = NULL;

      mjob_t tmpJ;
      mreq_t tmpRQ;

      mnode_t *N;

      int    tmpTM[4];

      MRM[0].Type = mrmtLL;

      MRM[0].U.LL.ProcVersionNumber = 9;

      strcpy(tmpJ.Name,"test");

      /* FORMAT:  <FROMHOST>.<CLUSTER>,<NODEID> */

      ptr = MUStrTok(aptr,".,",&TokPtr);

      tmpJ.SubmitHost = NULL;
      MUStrDup(&tmpJ.SubmitHost,ptr);

      ptr = MUStrTok(NULL,".,",&TokPtr);

      ptr = MUStrTok(NULL,",",&TokPtr);

      if (MNodeAdd(ptr,&N) == FAILURE)
        {
        exit(1);
        }

      N->RM = &MRM[0];

      tmpJ.TaskMap = tmpTM;
      tmpJ.TaskMapSize = 4;

      tmpJ.TaskMap[0] = N->Index;
      tmpJ.TaskMap[1] = -1;

      tmpJ.Req[0] = &tmpRQ;
      tmpJ.Req[1] = NULL;

      tmpRQ.RMIndex = 0;

      MRMWorkloadQuery(&JC,NULL,NULL,NULL);

      MRMJobStart(&tmpJ,NULL,NULL);
  
      exit(1);
      }  /* END BLOCK (case mirtLLSubmit) */

      break;

    case mirtLLQuery:

      /* NYI */

      exit(1);

      break;

    case mirtMail:

      rc = MSysSendMail(
        MSysGetAdminMailList(1),
        NULL,
        "moab test",
        NULL,
        "test mail");

      if (rc == FAILURE)
        exit(1);

      exit(0);
      
      break;

    case mirtPBSParse:
    case mirtPBSParse2:
 
      {
      /* NOTE:  select must be translated different than "neednodes", if select is specified,
                memory is assigned as a per task resource and arch as a per req constraint */

      /*
      > qsub -l select=4 (it requests 4 default chunks)
      >
      > or
      >
      > qsub -l select=2:ncpus=1:mem=10GB+3:ncpus=2:mem=8GB:arch=linux
      > (requests 2 chunks of 1 cpu,10gb of mem and 3 chunks of
      > 2 cpus, 8gb of mem, arch=linux).
      */

      int     rc;

      mjob_t *J = NULL;

      int *TaskList = NULL;

      MRM[0].Type = mrmtPBS;

      MJobMakeTemp(&J);

      MRMJobPreLoad(J,"PBSParse",&MRM[0]);

      TaskList = (int *)MUMalloc(sizeof(int) * MSched.JobMaxTaskCount);

      if (aindex == mirtPBSParse)
        rc = MJobGetPBSTaskList(J,aptr,TaskList,FALSE,TRUE,FALSE,FALSE,NULL,NULL);
      else
        rc = MJobGetSelectPBSTaskList(J,aptr,TaskList,FALSE,FALSE,NULL);

      MUFree((char **)TaskList);

      if (rc == FAILURE)
        {
        exit(1);
        }

      exit(0);
      }  /* END BLOCK (case mirtPBSParse) */

      break;

    case mirtNodeRange:

      {
      int       nindex;

      mnode_t  *N;

      mnl_t     tmpNL;

      MNLInit(&tmpNL);

      nindex = 0;

      MNodeAdd("node12",&N);
      MNLSetNodeAtIndex(&tmpNL,nindex,N);
      MNLSetTCAtIndex(&tmpNL,nindex++, 4);

      MNodeAdd("node11",&N);
      MNLSetNodeAtIndex(&tmpNL,nindex,N);
      MNLSetTCAtIndex(&tmpNL,nindex++, 4);

      MNodeAdd("node03",&N);
      MNLSetNodeAtIndex(&tmpNL,nindex,N);
      MNLSetTCAtIndex(&tmpNL,nindex++, 4);

      MNodeAdd("node04",&N);
      MNLSetNodeAtIndex(&tmpNL,nindex,N);
      MNLSetTCAtIndex(&tmpNL,nindex++, 2);

      MNodeAdd("node05",&N);
      MNLSetNodeAtIndex(&tmpNL,nindex,N);
      MNLSetTCAtIndex(&tmpNL,nindex++, 2);

      MNodeAdd("node06",&N);
      MNLSetNodeAtIndex(&tmpNL,nindex,N);
      MNLSetTCAtIndex(&tmpNL,nindex++, 1);

      MNodeAdd("node09",&N);
      MNLSetNodeAtIndex(&tmpNL,nindex,N);
      MNLSetTCAtIndex(&tmpNL,nindex++, 4);

      MNodeAdd("node08",&N);
      MNLSetNodeAtIndex(&tmpNL,nindex,N);
      MNLSetTCAtIndex(&tmpNL,nindex++, 4);

      MNodeAdd("node07",&N);
      MNLSetNodeAtIndex(&tmpNL,nindex,N);
      MNLSetTCAtIndex(&tmpNL,nindex++, 4);

      MNodeAdd("node10",&N);
      MNLSetNodeAtIndex(&tmpNL,nindex,N);
      MNLSetTCAtIndex(&tmpNL,nindex++, 3);

      MNodeAdd("node01",&N);
      MNLSetNodeAtIndex(&tmpNL,nindex,N);
      MNLSetTCAtIndex(&tmpNL,nindex++, 4);

      MNodeAdd("node02",&N);
      MNLSetNodeAtIndex(&tmpNL,nindex,N);
      MNLSetTCAtIndex(&tmpNL,nindex++, 3);

      MNLTerminateAtIndex(&tmpNL,nindex);

      mstring_t TaskBuf(MMAX_BUFFER);

      MUNLToRangeString(
        &tmpNL,
        NULL,
        -1,
        TRUE,
        TRUE,
        &TaskBuf);

      fprintf(stderr,"Range:  '%s'\n",
        TaskBuf.c_str());

      exit(0);
      }  /* END BLOCK */

      break;

    case mirtBGRange:

      {
      char tmpLine[MMAX_LINE];

      int NCount;
      int NListIndex = 0;

      char **NCharList = NULL; /* make dynamic */

      NCharList = (char **)MUCalloc(1,sizeof(char *) * MSched.M[mxoNode]);

      snprintf(tmpLine,sizeof(tmpLine),"002x233,702x733");

      MUParseBGRangeString(tmpLine,NCharList,MSched.M[mxoNode] - NListIndex,&NCount);

      exit(0);
      }

      break;

    case mirtDisaTest:

      rc = MDISATest();

      exit((rc == SUCCESS) ? 0 : 1);

      break;

    default:
     
      {
      int tindex;
 
      /* cannot determine test */

      fprintf(stderr,"ERROR:    invalid test specified (%s) - use ",
        tptr);

      for (tindex = 1;tindex < mirtLAST;tindex++)
        {
        fprintf(stderr,"%s%s",
          (tindex > 1) ? "," : "",
          TName[tindex]);
        }

      fprintf(stderr,"\n");
      }  /* END BLOCK */

      exit(1);

      /*NOTREACHED*/

      break;
    }  /* END switch (aindex) */
 
  return(SUCCESS);
  }  /* END MSysDoTest() */

/* END MSysDoTest.c */
