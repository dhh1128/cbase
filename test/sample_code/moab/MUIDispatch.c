/* HEADER */

/**
 * @file MUIDispatch.c
 *
 * Contains MUI Request dispatch code and its intialization code
 */



#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/* Private data:
 *
 * The MCRequest Function pointer table
 */
int (*MCRequestF[mccLAST])(msocket_t *,mbitmap_t *,char *);



/**
 * Initialize function handlers for high level user interface requests.
 *
 * MC Functions will be displatched via function pointers in the MCRequestF
 * table.
 *
 */

int MUIInitMUIFuncHandlers(void)

  {
  /* NOTE:  @see MUI[] */

  MCRequestF[mccBal]       = MUIBal;
  MCRequestF[mccJobCtl]    = MUIJobCtl;
  MCRequestF[mccNodeCtl]   = MUINodeCtl;
  MCRequestF[mccRsvCtl]    = MUIRsvCtl;
  MCRequestF[mccShow]      = MUIShow;
  MCRequestF[mccCredCtl]   = MUICredCtl;
  MCRequestF[mccRMCtl]     = MUIRMCtl;
  MCRequestF[mccSchedCtl]  = MUISchedCtl;
  MCRequestF[mccStat]      = MUIShowStats;
  MCRequestF[mccDiagnose]  = MUIDiagnose;
  MCRequestF[mccVCCtl]     = MUIVCCtl;
  MCRequestF[mccVMCtl]     = MUIVMCtl;
  MCRequestF[mccCheckJob]  = MUICheckJobWrapper;
  MCRequestF[mccCheckNode] = MUICheckNode;
  MCRequestF[mccRsvShow]   = MUIShowRes;
  MCRequestF[mccShowState] = MUIShowState;

  /* NOTE: threadsafe routines */

  MCRequestF[mccShowThreadSafe] = MUIShowQueueThreadSafe;
  MCRequestF[mccJobCtlThreadSafe] = MUIJobCtlThreadSafe;
  MCRequestF[mccNodeCtlThreadSafe] = MUINodeCtlThreadSafe;
  MCRequestF[mccRsvDiagnoseThreadSafe] = MUIRsvDiagnoseThreadSafe;
  MCRequestF[mccTrigDiagnoseThreadSafe] = MUITrigDiagnoseThreadSafe;
  MCRequestF[mccVCCtlThreadSafe] = MUIVCCtlThreadSafe;
  MCRequestF[mccVMCtlThreadSafe] = MUIVMCtlThreadSafe;
  MCRequestF[mccPeerResourcesQuery] = MUIPeerResourcesQueryThreadSafe;
  MCRequestF[mccEventThreadSafe] = MUIEventQueryThreadSafe;
  MCRequestF[mccJobSubmitThreadSafe] = MUIJobSubmitThreadSafe;

  /* NOTE:  submit routes into jobctl */

  MCRequestF[mccSubmit]   = MUIJobCtl;
 
  return(SUCCESS);
  }  /* END MUIInitMUIFuncHandlers() */

/**
 * Report detailed job information.  Handles the 'checkjob' client request. 
 *  
 *  
 * WARNING!! 
 * This routine is a compatibility interface between the old style (6.1.x) 
 * and the new style (7.x.x) checkjob.  It only needs to be included until 
 * We release version 8.x.x, at which time it can be deprecated. 
 *
 * @param RBuffer (I)
 * @param SBuffer (O)
 * @param AFlags (I)    Unused
 * @param Auth (I)
 * @param SBufSize (I/O)
 */

int MUIJobShow(

  const char *RBuffer,
  char       *SBuffer,
  mbitmap_t  *AFlags,
  char       *Auth,
  long       *SBufSize)

  {
  mxml_t *ReqE;
  msocket_t S;
  int rc;

  const char *FName = "MUIJobShow";
  MDB(2,fUI) MLog("%s(RBuffer,SBuffer,%d,%s,BufSize)\n",
    FName,
    AFlags,
    Auth);

  /* initialize values */

  ReqE = NULL;

  if (MXMLFromString(&ReqE,RBuffer,NULL,NULL) == FAILURE)
    {
    sprintf(SBuffer,"ERROR:  cannot process request\n");

    MDB(3,fUI) MLog("INFO:     cannot parse request '%s' in %s()\n",
      RBuffer,
      FName);

    return(FAILURE);
    }

  memset(&S, 0, sizeof(S));

  MUStrDup(&S.RBuffer,RBuffer);
  S.RE = ReqE;
  S.RDE = ReqE;

  rc = MUICheckJobWrapper( &S, 0, Auth);
  
  /* If the results came back as a string in S.SBuffer,
     save them.  Otherwise assume they are in the S.SDE XML
     structure and convert that. */
  if (S.SBuffer != NULL)
    {
    strncpy(SBuffer, S.SBuffer, *SBufSize);
    }
  else if (S.SDE != NULL)
    {
    MXMLToString(S.SDE,SBuffer,*SBufSize,NULL,TRUE);
    }

  MUFree(&S.RBuffer);
  MXMLDestroyE(&ReqE);
  
  return(rc);
  } /* End MUIJobShow */
  
  
  
  
/**
 * Report detailed node information (handle 'checknode').
 *  
 *  
 * WARNING!! 
 * This routine is a compatibility interface between the old style (6.1.x) 
 * and the new style (7.x.x) checknode.  It only needs to be included until 
 * We release version 8.x.x, at which time it can be deprecated. 
 *
 * @param RBuffer (I)
 * @param SBuffer (I/O) output buffer
 * @param CFlags (I) command flags
 * @param Auth (I)
 * @param SBufSize (I)
 */

int MUINodeShow(

  const char *RBuffer,
  char       *SBuffer,
  mbitmap_t  *CFlags,
  char       *Auth,
  long       *SBufSize)

  {
  char *Tokenized = NULL;

  mxml_t *RE;
  msocket_t S;
  mulong Flags;
  int rc;
  int numTokens;
  int tokenArraySize = 2;

  char *tokenArray[tokenArraySize];

  const char *FName = "MUINodeShow";

  MDB(2,fUI) MLog("%s(RBuffer,SBuffer,%d,%s,SBufSize)\n",
    FName,
    CFlags,
    Auth);

  /* Format:  <nodelist> <Flags> */
  /* Parse into nodelist and flags */

  MUStrDup(&Tokenized,RBuffer);

  if ((numTokens = MUStrSplit(Tokenized," ",tokenArraySize-1,tokenArray,tokenArraySize)) != 2)
    {
    sprintf(SBuffer,"ERROR:  cannot process request\n");

    MDB(3,fUI) MLog("INFO:     cannot parse request '%s' in %s()\n",
      RBuffer,
      FName);

    MUFree(&Tokenized);

    return(FAILURE);
    }

  /* Create the XML object */
  RE = NULL;
  MXMLCreateE(&RE,(char *)MSON[msonRequest]);
  MS3SetObject(RE,(char *)MXO[mxoNode],NULL);

  Flags = strtol(tokenArray[1],NULL,10);
  MXMLSetAttr(RE,MSAN[msanFlags],(void *)&Flags,mdfLong);
  
  MS3AddWhere(RE,(char *)MNodeAttr[mnaNodeID],tokenArray[0],NULL);

  memset(&S, 0, sizeof(S));

  MUStrDup(&S.RBuffer,RBuffer);
  S.RE = RE;
  S.RDE = RE;

  if ((rc = MUICheckNode( &S, 0, Auth)) == SUCCESS)
    {
    strncpy(SBuffer, S.SBuffer, *SBufSize);
    }

  MUFree(&S.RBuffer);
  MUFree(&Tokenized);
  MXMLDestroyE(&RE);
  
  return(rc);
  }




/**
 * Report reservation status (process 'showres' request)
 *
 * @see MUIRsvCtl() - control/modify/create reservations.
 *
 * @see MNodeShowRsv() - child - process 'showres -n'
 * @see MUIRsvList() - child - process 'showres'
 *
 * @param RBuffer (I)
 * @param Buffer (O)
 * @param CFlags (I)
 * @param Auth (I)
 * @param BufSize (I)
 */

int MUIRsvShow(

  const char *RBuffer,  /* I */
  char       *Buffer,   /* O */
  mbitmap_t  *CFlags,   /* I */
  char       *Auth,     /* I */
  long       *BufSize)  /* I */

  {
  mxml_t *DE = NULL;

  enum MXMLOTypeEnum ObjectType;

  char Name[MMAX_NAME];
  char PName[MMAX_NAME];
  char EMsg[MMAX_LINE] = {0};

  mpar_t *P;

  mbitmap_t DFlags;

  /* imported from 6.1 for backwards compatibility */

  enum MRsvFlagEnum {
    rnone = 0,
    rBlock,
    rClear,
    rComplete,
    rCP,
    rExclusive,
    rForce,
    rForce2,
    rFuture,
    rGrep,
    rJobName,
    rLocal,
    rNonBlock,
    rOverlap,
    rParse,
    rPeer,
    rPersistent,
    rPolicy,
    rRelative,
    rSort,
    rSummary,
    rTimeFlex,
    rUser,
    rUseTID,
    rVerbose,
    rVerbose2,
    rXML,
    rlast };

  int  Flags;

  const char *FName = "MUIRsvShow";

  MDB(3,fUI) MLog("%s(RBuffer,Buffer,%s,BufSize)\n",
    FName,
    Auth);

  MUSScanF(RBuffer,"%u %x%s %d %x%s",
    &ObjectType,
    sizeof(PName),
    PName,
    &Flags,
    sizeof(Name),
    Name);

  if (MParFind(PName,&P) == FAILURE)
    {
    sprintf(Buffer,"ERROR:  invalid partition, %s, specified\n",
      PName);

    return(FAILURE);
    }

  if (MXMLCreateE(&DE,MSON[msonData]) == FAILURE)
    {
    sprintf(Buffer,"ERROR:    cannot create response\n");

    return(FAILURE);
    }

  /* possible flags are mcmNonBlock,mcmVerbose,mcmOverlap */

  if (MOLDISSET(Flags,rNonBlock))
    {
    bmset(&DFlags,mcmNonBlock);
    }

  if (MOLDISSET(Flags,rVerbose))
    {
    bmset(&DFlags,mcmVerbose);
    }

  if (MOLDISSET(Flags,rOverlap))
    {
    bmset(&DFlags,mcmOverlap);
    }

  switch (ObjectType)
    {
    case mxoNode:

      if (MNodeShowRsv(Auth,Name,P,&DFlags,DE,EMsg) == FAILURE)
        {
        MUStrCpy(Buffer,EMsg,*BufSize);

        return(FAILURE);
        }

      break;

    case mxoJob:

      if (MUIRsvList(Auth,Name,P,&DFlags,DE,EMsg) == FAILURE)
        {
        MUStrCpy(Buffer,EMsg,*BufSize);

        return(FAILURE);
        }

      break;

    default:

      MDB(0,fUI) MLog("ERROR:    reservation type '%d' not handled\n",
        ObjectType);

      sprintf(Buffer,"ERROR:    reservation type '%d' not handled\n",
        ObjectType);

      return(FAILURE);

      break;
    }  /* END switch (ObjectType) */

  MXMLToString(DE,Buffer,*BufSize,NULL,TRUE);

  MXMLDestroyE(&DE);

  return(SUCCESS);
  }  /* END MUIRsvShow() */










/**
 * MUI Dispatch initialization function (constructor) to
 * setup the MUI Function dispatch function pointer table
 */

int MUIInitialize()

  {
  const char *FName = "MUIInitialize";

  MDB(3,fUI) MLog("%s()\n",
    FName);

  /* Initialize Function pointers of the MUI ADMIN matrix */

  /* NOTE:  these routines have 5 parameter function prototypes - 
   *        see MCRequestF[] for handling 3 parameter UI functions
   */

  MUI[mcsShowState].Func              = NULL;                /* deprecated */
  MUI[mcsShowQueue].Func              = NULL;                /* deprecated */
  MUI[mcsStatShow].Func               = NULL;                /* deprecated */
  MUI[mcsRsvCreate].Func              = NULL;                /* deprecated */
  MUI[mcsRsvDestroy].Func             = NULL;                /* deprecated */
  MUI[mcsRsvShow].Func                = MUIRsvShow;          /* deprecated */
  MUI[mcsDiagnose].Func               = NULL;                /* deprecated */
  MUI[mcsShowConfig].Func             = NULL;                /* deprecated */
  MUI[mcsCheckJob].Func               = MUIJobShow;          /* deprecated */
  MUI[mcsCheckNode].Func              = MUINodeShow;         /* deprecated */
  MUI[mcsJobStart].Func               = NULL;                /* deprecated */
  MUI[mcsCancelJob].Func              = NULL;                /* deprecated */
  MUI[mcsConfigure].Func              = NULL;                /* deprecated */
  MUI[mcsShowEstimatedStartTime].Func = MUIShowEstStartTime;
  MUI[mcsShowResAvail].Func           = MUIOClusterShowARes;


  /* Setup the MCRequestF table */
  /* @see MCRequestF[] */
  MUIInitMUIFuncHandlers();

  return(SUCCESS);
  }  /* END MUIInitialize() */


/**
 * Actual MUI Dispatch function
 * Uses a FuncIndex select value to determine which function to call
 * and returns the selected function's return code
 */

int MDispatchMCRequestFunction(
    
  long         FuncIndex,
  msocket_t   *S,
  mbitmap_t   *AFlags,
  name_t       tmpAuth)

  {
  /* Ensure function index resides with the table and the table entry is valid */
  if ((FuncIndex <= mccNONE) || (FuncIndex >= mccLAST) || (NULL == MCRequestF[FuncIndex]))
    return(FAILURE);

  return(MCRequestF[FuncIndex](S,AFlags,tmpAuth));
  } /* END DispatchMCRequestFunction() */



/**
 * Validate a Call function index 
 */
int MCheckMCRequestFunction(

  long         FuncIndex)

  {
  /* Ensure function index resides with the table and the table entry is valid */
  if ((FuncIndex <= mccNONE) || (FuncIndex >= mccLAST) || (NULL == MCRequestF[FuncIndex]))
    return(FAILURE);

  return(SUCCESS);
  }

/* END MUIDispatch */
