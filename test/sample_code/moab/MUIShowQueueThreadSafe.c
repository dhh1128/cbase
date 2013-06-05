/* HEADER */

/**
 * @file MUIShowQueueThreadSafe.c
 *
 * Contains MUI Show Queue function, thread safe
 */



#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  




/**
 * compare job transition objects by remaining time, ordered low to high
 *
 * @param A (I)
 * @param B (I)
 */

int __MTransJobRemainingTimeCompLToH(

  mtransjob_t **A,  /* I */
  mtransjob_t **B)  /* I */

  {
  long ATime = (*A)->RequestedMaxWalltime - (*A)->UsedWalltime;
  long BTime = (*B)->RequestedMaxWalltime - (*B)->UsedWalltime;

  return((int)(ATime - BTime));
  }




/**
 * compare job transition objects by completion time, ordered low to high
 *
 * @param A (I)
 * @param B (I)
 */

int __MTransJobCompletionTimeCompLToH(

  mtransjob_t **A,  /* I */
  mtransjob_t **B)  /* I */

  {
  /* order low to high */

  return((*A)->CompletionTime - (*B)->CompletionTime);
  }  /* END __MTransJobCompletionTimeCompLToH() */




/**
 * compare job transition objects by start priority, ordered high to low
 *
 * @param A (I)
 * @param B (I)
 */

int __MTransJobPriorityCompHToL(

  mtransjob_t **A,  /* I */
  mtransjob_t **B)  /* I */

  {
  /* order high to low */
/*
  if ((*B)->Priority == (*A)->Priority)
    {
    return(__MTransJobNameCompLToH(A,B));
    }
*/

  return((*B)->Priority - (*A)->Priority);
  } /* END __MTransJobPriorityCompHToL() */




/**
 * Process 'mshow/showq' request in a threadsafe way.
 *
 * @see MUIQueueShow() - child - process showq
 * @see MClusterShowARes() - child - process 'mshow -a'
 *
 * @param S (I) [modified]
 * @param CFlagBM (I) enum MRoleEnum bitmap
 * @param Auth (I)
 */

int MUIShowQueueThreadSafe(

  msocket_t *S,       /* I (modified) */
  mbitmap_t *CFlagBM, /* I enum MRoleEnum bitmap */
  char      *Auth)    /* I */

  {
  int rindex;

  mxml_t *DE;
  mxml_t *CE;
  mxml_t *AQE;
  mxml_t *EQE;
  mxml_t *BQE;
  mxml_t *CQE;
  mxml_t *JE;
  mxml_t *RE = NULL;
  mxml_t *WE = NULL;

  char FlagString[MMAX_LINE];
  char ArgString[MMAX_LINE];
  char AttrString[MMAX_LINE];
  char TypeString[MMAX_LINE];
  char OString[MMAX_LINE];

  char tmpName[MMAX_NAME];
  char tmpVal[MMAX_LINE];

  enum MXMLOTypeEnum OIndex;

  int  EQCount;
  int  AQCount;
  int  BQCount;
  int  CQCount;

  int  RemoteActiveProcs = 0;
  int  RemoteIdleProcs   = 0;
  int  RemoteUpProcs     = 0;
  int  RemoteActiveNodes = 0;
  int  RemoteIdleNodes   = 0;
  int  RemoteUpNodes     = 0;
  int  RemoteConfigNodes = 0;

  int  LocalActiveProcs = 0;
  int  LocalIdleProcs   = 0;
  int  LocalUpProcs     = 0;
  int  LocalActiveNodes = 0;
  int  LocalIdleNodes   = 0;
  int  LocalUpNodes     = 0;
  int  LocalConfigNodes = 0;

  /* ptrs to increment remote or local counts */
  int *IdleProcs;
  int *UpProcs;
  int *ActiveNodes;
  int *IdleNodes;
  int *UpNodes;
  int *ConfigNodes;

  mtransjob_t  *J;
  mtransnode_t *N;

  marray_t AJList;
  marray_t BJList;
  marray_t IJList;
  marray_t IRJList;
  marray_t CJList;
  marray_t NList;
  marray_t ArrayMasterList;

  /*mdb_t *MDBInfo;*/

  mrm_t   *tmpRM;

  mbool_t IsAdmin = FALSE;
  mbool_t SkipBlocked = FALSE;

  int   WTok = -1;

  mpar_t *P  = NULL;

  marray_t JConstraintList;
  mjobconstraint_t JConstraint;

  marray_t NConstraintList;
  mnodeconstraint_t NConstraint;

  mbitmap_t JTBM;
  mbitmap_t FlagBM;  /* bitmap of enum MCModeEnum */

  const char *FName = "MUIShowQueueThreadSafe";

  MDB(2,fUI) MLog("%s(S,%s)\n",
    FName,
    Auth);

  memset(&JConstraint.ALong,0,sizeof(JConstraint.ALong));
  memset(&NConstraint.ALong,0,sizeof(NConstraint.ALong));

  FlagString[0] = '\0';
  ArgString[0]  = '\0';
  AttrString[0] = '\0';
  TypeString[0] = '\0';
  OString[0]    = '\0';

  /*MSchedGetAttr(msaDBHandle,mdfNONE,(void **)&MDBInfo,-1);*/

  if (bmisset(CFlagBM,mcalAdmin1) ||
      bmisset(CFlagBM,mcalAdmin2) ||
      bmisset(CFlagBM,mcalAdmin3))
    IsAdmin = TRUE;

  RE = S->RDE;

  if ((MS3GetObject(RE,OString) == FAILURE) ||
     ((OIndex = (enum MXMLOTypeEnum)MUGetIndexCI(OString,MXO,FALSE,mxoNONE)) == mxoNONE))
    {
    MDB(3,fUI) MLog("WARNING:  corrupt command '%100.100s' received\n",
      S->RBuffer);

    MUISAddData(S,"ERROR:    corrupt command received\n");

    return(FAILURE);
    }

  if (MXMLGetAttr(RE,MSAN[msanFlags],NULL,FlagString,0) == SUCCESS)
    {
    bmfromstring(FlagString,MClientMode,&FlagBM);
    }    /* END if (MXMLGetAttr() == SUCCESS) */

  MUArrayListCreate(&JConstraintList,sizeof(mjobconstraint_t),5);
  MUArrayListCreate(&NConstraintList,sizeof(mnodeconstraint_t),5);

  while (MS3GetWhere(
      RE,
      &WE,
      &WTok,
      tmpName,
      sizeof(tmpName),
      tmpVal,
      sizeof(tmpVal)) == SUCCESS)
    {
    /* process where */

    if (!strcmp(tmpName,"partition"))
      {
      if (MParFind(tmpVal,&P) == FAILURE)
        {
        MUISAddData(S,"ERROR:    unknown partition specified\n");

        MUArrayListFree(&JConstraintList);
        return(FAILURE);
        }
      }
    else if (!strcmp(tmpName,"user"))
      {
      mgcred_t *UP;

#ifdef __MDEBUG
      /* secret kill switch for debugging and valgrind purposes */

      if (!strcmp(tmpVal,"MOABKILLSWITCH"))
        {
        fprintf(stderr,"exiting now\n");

        exit(1);
        }
#endif /* __MDEBUG */

      if (MUserFind(tmpVal,&UP) == FAILURE)
        {
        MUISAddData(S,"ERROR:    unknown user specified\n");

        MUArrayListFree(&JConstraintList);
        return(FAILURE);
        }

      JConstraint.AType = mjaUser;
      MUStrCpy(JConstraint.AVal[0],tmpVal,sizeof(JConstraint.AVal[0]));
      JConstraint.ACmp = mcmpSSUB;

      MUArrayListAppend(&JConstraintList,(void *)&JConstraint);
      }
    else if (!strcmp(tmpName,"group"))
      {
      mgcred_t *GP;

      if (MGroupFind(tmpVal,&GP) == FAILURE)
        {
        MUISAddData(S,"ERROR:    unknown group specified\n");

        MUArrayListFree(&JConstraintList);
        return(FAILURE);
        }

      JConstraint.AType = mjaGroup;
      MUStrCpy(JConstraint.AVal[0],tmpVal,sizeof(JConstraint.AVal[0]));
      JConstraint.ACmp = mcmpSSUB;
      
      MUArrayListAppend(&JConstraintList,(void *)&JConstraint);
      }
    else if (!strcmp(tmpName,"acct"))
      {
      mgcred_t *AP;

      if (MAcctFind(tmpVal,&AP) == FAILURE)
        {
        MUISAddData(S,"ERROR:    unknown account specified\n");
 
        MUArrayListFree(&JConstraintList);
        return(FAILURE);
        }

      JConstraint.AType = mjaAccount;
      MUStrCpy(JConstraint.AVal[0],tmpVal,sizeof(JConstraint.AVal[0]));
      JConstraint.ACmp = mcmpSSUB;
      
      MUArrayListAppend(&JConstraintList,(void *)&JConstraint);
      }
    else if (!strcmp(tmpName,"class"))
      {
      mclass_t *CP;

      if (MClassFind(tmpVal,&CP) == FAILURE)
        {
        MUISAddData(S,"ERROR:    unknown class specified\n");
 
        MUArrayListFree(&JConstraintList);
        return(FAILURE);
        }

      JConstraint.AType = mjaClass;
      MUStrCpy(JConstraint.AVal[0],tmpVal,sizeof(JConstraint.AVal[0]));
      JConstraint.ACmp = mcmpSSUB;
      
      MUArrayListAppend(&JConstraintList,(void *)&JConstraint);
      }
    else if (!strcmp(tmpName,"qos"))
      {
      mqos_t *QP;

      if (MQOSFind(tmpVal,&QP) == FAILURE)
        {
        MUISAddData(S,"ERROR:    unknown qos specified\n");

        MUArrayListFree(&JConstraintList);
        return(FAILURE);
        }

      JConstraint.AType = mjaQOS;
      MUStrCpy(JConstraint.AVal[0],tmpVal,sizeof(JConstraint.AVal[0]));
      JConstraint.ACmp = mcmpSSUB;
      
      MUArrayListAppend(&JConstraintList,(void *)&JConstraint);
      }
    } /* END while (MS3GetWhere(...)) */

  MXMLGetAttr(RE,MSAN[msanArgs],NULL,ArgString,sizeof(ArgString));
  MXMLGetAttr(RE,"attr",NULL,AttrString,sizeof(AttrString));
  MXMLGetAttr(RE,MSAN[msanOption],NULL,TypeString,sizeof(TypeString));

  /* check to see if HideBlocked is set */
  if (bmisset(&MSched.DisplayFlags,mdspfHideBlocked) &&
      (IsAdmin == FALSE) &&
      (strcmp(TypeString,"blocked") != 0))
    {
    SkipBlocked = TRUE;
    }

  bmfromstring(TypeString,MJobStateType,&JTBM);

  if (bmisclear(&JTBM))
    {
    bmset(&JTBM,mjstActive);
    bmset(&JTBM,mjstEligible);
    bmset(&JTBM,mjstBlocked);
    }
 
  MUArrayListCreate(&AJList,sizeof(mtransjob_t *),10);
  MUArrayListCreate(&BJList,sizeof(mtransjob_t *),10);
  MUArrayListCreate(&IJList,sizeof(mtransjob_t *),10);
  MUArrayListCreate(&IRJList,sizeof(mtransjob_t *),10);
  MUArrayListCreate(&CJList,sizeof(mtransjob_t *),10);
  MUArrayListCreate(&ArrayMasterList,sizeof(mtransjob_t *),10);

  if ((P != NULL) && (P->Index != 0))
    {
    JConstraint.AType = mjaPAL;
    JConstraint.ALong[0] = P->Index;
    MUArrayListAppend(&JConstraintList,(void *)&JConstraint);

    NConstraint.AType = mnaPartition;
    MUStrCpy(NConstraint.AVal[0],P->Name,sizeof(NConstraint.AVal[0]));
    MUArrayListAppend(&NConstraintList,(void *)&NConstraint);
    }

  MCacheJobLock(TRUE,TRUE);

  MCacheReadJobsForShowq(&JTBM,P,&JConstraintList,&AJList,&BJList,&IJList,&IRJList,&CJList,&ArrayMasterList);

  MUArrayListFree(&JConstraintList);

  if (MXMLCreateE(&S->SDE,(char *)MSON[msonData]) == FAILURE)
    {
    MUISAddData(S,"ERROR:    cannot create response\n");
    MCacheJobLock(FALSE,TRUE);
    MUArrayListFree(&NConstraintList);

    return(FAILURE);
    }

  DE = S->SDE;

  if (MS3GetObject(DE,NULL) == FAILURE)
    MS3SetObject(DE,(char *)MXO[mxoQueue],NULL);

  /* add the cluster element */

  CE = NULL;

  if (MXMLCreateE(&CE,(char *)MXO[mxoCluster]) == FAILURE)
    {
    MUISAddData(S,"ERROR:    internal failure\n");
    MCacheJobLock(FALSE,TRUE);
    MUArrayListFree(&NConstraintList);

    return(FAILURE);
    }

  MXMLAddE(DE,CE);

  /* NOTE: create active queue */

  AQE = NULL;

  if (bmisset(&JTBM,mjstActive))
    {
    if (MXMLCreateE(&AQE,(char *)MXO[mxoQueue]) == FAILURE)
      {
      MUISAddData(S,"ERROR:    internal failure\n");
      MCacheJobLock(FALSE,TRUE);
      MUArrayListFree(&NConstraintList);

      return(FAILURE);
      }

    MXMLSetAttr(
      AQE,
      MSAN[msanOption],
      (void **)MJobStateType[mjstActive],
      mdfString);

    MXMLAddE(DE,AQE);
    } /* END if (...mjstActive...) */

  /* NOTE: create eligible queue */

  EQE = NULL;

  if (bmisset(&JTBM,mjstEligible))
    {
    if (MXMLCreateE(&EQE,(char *)MXO[mxoQueue]) == FAILURE)
      {
      MUISAddData(S,"ERROR:    internal failure\n");
      MCacheJobLock(FALSE,TRUE);
      MUArrayListFree(&NConstraintList);

      return(FAILURE);
      }

    MXMLSetAttr(
      EQE,
      MSAN[msanOption],
      (void **)MJobStateType[mjstEligible],
      mdfString);

    MXMLAddE(DE,EQE);
    } /* END if (...mjstEligible...) */

  /* NOTE: create blocked queue */

  BQE = NULL;

  if (bmisset(&JTBM,mjstBlocked))
    {
    if (MXMLCreateE(&BQE,(char *)MXO[mxoQueue]) == FAILURE)
      {
      MUISAddData(S,"ERROR:    internal failure\n");
      MCacheJobLock(FALSE,TRUE);
      MUArrayListFree(&NConstraintList);

      return(FAILURE);
      }

    MXMLSetAttr(
      BQE,
      MSAN[msanOption],
      (void **)MJobStateType[mjstBlocked],
      mdfString);

    MXMLAddE(DE,BQE);
    } /* END if (...mjstBlocked...) */

  /* NOTE: create completed queue */

  CQE = NULL;

  if (bmisset(&JTBM,mjstCompleted))
    {
    if (MXMLCreateE(&CQE,(char *)MXO[mxoQueue]) == FAILURE)
      {
      MUISAddData(S,"ERROR:    internal failure\n");
      MCacheJobLock(FALSE,TRUE);
      MUArrayListFree(&NConstraintList);

      return(FAILURE);
      }

    MXMLSetAttr(
      CQE,
      MSAN[msanOption],
      (void **)MJobStateType[mjstCompleted],
      mdfString);

    MXMLSetAttr(
        CQE,
        "purgetime",
        (void **)&MSched.JobCPurgeTime,
        mdfLong);

    MXMLAddE(DE,CQE);
    } /* END if (...mjstBlocked...) */


  AQCount = 0;
  EQCount = 0;
  BQCount = 0;
  CQCount = 0;

  LocalActiveProcs = 0;

  /* sort active jobs by time remaining, low to high */

  if ((AJList.NumItems > 1) && (!bmisset(&FlagBM,mcmSummary)))
    {
    /* NYI OPTIMIZATION: only sort the array masters */

    qsort(
      (void *)AJList.Array,
      AJList.NumItems,
      sizeof(mtransjob_t *),
      (int(*)(const void *,const void *))__MTransJobRemainingTimeCompLToH);
    }

  /* add active jobs to the outgoing xml */

  for (rindex = 0; rindex < AJList.NumItems; rindex++)
    {
    J = (mtransjob_t *)MUArrayListGetPtr(&AJList,rindex);

    AQCount++;

    if ((MUStrIsEmpty(J->DestinationRM)) ||
        ((MRMFind(J->DestinationRM,&tmpRM) == SUCCESS) &&
         (tmpRM->Type != mrmtMoab)))
      {
      if (J->State != mjsSuspended)
        {
        LocalActiveProcs += J->MaxProcessorCount;
        }
      }
    else
      {
      RemoteActiveProcs += J->MaxProcessorCount;
      }

    /* never add array sub-jobs to the display list */

    if (bmisset(&J->TFlags,mtransfArraySubJob))
      {
      continue;
      }

    if ((AQE != NULL) && (!bmisset(&FlagBM,mcmSummary)))
      {
      MJobTransitionBaseToXML(J,&JE,P,&FlagString[0]);

      MXMLAddE(AQE,JE);
      }
    } /* END for (AJList...) */

  /* sort idle jobs with reservation by job priority, high to low */
    /* deprecated -- sorting is done on the client. */

  /* add idle jobs with reservation to the outgoing xml */

  for (rindex = 0; rindex < IRJList.NumItems; rindex++)
    {
    J = (mtransjob_t *)MUArrayListGetPtr(&IRJList,rindex);

    EQCount++;

    /* never add array sub-jobs to the display list */

    if (bmisset(&J->TFlags,mtransfArraySubJob))
      {
      continue;
      }

    if ((EQE != NULL) && (!bmisset(&FlagBM,mcmSummary)))
      {
      MJobTransitionBaseToXML(J,&JE,P,&FlagString[0]);

      MXMLAddE(EQE,JE);
      }
    } /* END for (IRJList...) */

  /* sort idle jobs by job priority, high to low */
    /* deprecated -- sorting is done on the client. */

  /* add idle jobs to the outgoing xml */

  for (rindex = 0; rindex < IJList.NumItems; rindex++)
    {
    J = (mtransjob_t *)MUArrayListGetPtr(&IJList,rindex);

    EQCount++;

    /* never add array sub-jobs to the display list */

    if (bmisset(&J->TFlags,mtransfArraySubJob))
      {
      continue;
      }

    if ((EQE != NULL) && (!bmisset(&FlagBM,mcmSummary)))
      {
      MJobTransitionBaseToXML(J,&JE,P,&FlagString[0]);

      MXMLAddE(EQE,JE);
      }
    } /* END for (IJList...) */

  /* sort blocked jobs by name */
    /* deprecated -- sorting is done on the client. */

  /* add blocked jobs to the outgoing xml */

  for (rindex = 0; rindex < BJList.NumItems; rindex++)
    {
    J = (mtransjob_t *)MUArrayListGetPtr(&BJList,rindex);

    /* Don't allow user to see other users blocked jobs */
    if ((SkipBlocked == TRUE) &&
        (strcmp(Auth,J->User) != 0))
      {
      continue;
      }

    BQCount++;

    /* never add array sub-jobs to the display list */

    if (bmisset(&J->TFlags,mtransfArraySubJob))
      {
      continue;
      }

    if ((BQE != NULL) && (!bmisset(&FlagBM,mcmSummary)))
      {
      MJobTransitionBaseToXML(J,&JE,P,&FlagString[0]);

      MXMLAddE(BQE,JE);
      }
    } /* END for (BJList...) */

  /* sort completed jobs by completion time, low to high */

  if (CJList.NumItems > 1)
    {
    /* NYI OPTIMIZATION: only sort the array masters */

    qsort(
      (void *)CJList.Array,
      CJList.NumItems,
      sizeof(mtransjob_t *),
      (int(*)(const void *,const void *))__MTransJobCompletionTimeCompLToH);
    }

  /* add completed jobs to the outgoing xml */

  for (rindex = 0; rindex < CJList.NumItems; rindex++)
    {
    J = (mtransjob_t *)MUArrayListGetPtr(&CJList,rindex);

    /* never add array sub-jobs to the display list */

    if (bmisset(&J->TFlags,mtransfArraySubJob))
      {
      continue;
      }

    if (CQE != NULL)
      {
      MJobTransitionBaseToXML(J,&JE,P,&FlagString[0]);

      MXMLAddE(CQE,JE);

      CQCount++;
      }
    } /* END for (CJList...) */

  /* go through the array master jobs, if any, and determine where they need
   * to be added to the queues. */

  if (!bmisset(&FlagBM,mcmSummary))
    {
    for (rindex = 0; rindex < ArrayMasterList.NumItems; rindex++)
      {
      int ProcCount = 0;
      int JobCount = 0;
      mulong StartTime = 0;
   
      J = (mtransjob_t *)MUArrayListGetPtr(&ArrayMasterList,rindex);   
   
      /* check all the active jobs for sub-jobs of this array to find the total
       * running proc count */
   
      MJobTransitionGetArrayProcCount(J,&AJList,&ProcCount);
      MJobTransitionGetArrayJobCount(J,&AJList,&JobCount);
   
      if ((ProcCount > 0) && (JobCount > 0)) /* if one is > 0, the other had better be... */
        {
        if ((AQE != NULL) &&
            (!bmisset(&FlagBM,mcmSummary)))
          {
          MJobTransitionGetArrayRunningStartTime(J,&AJList,&StartTime);
          MJobTransitionBaseToXML(J,&JE,P,&FlagString[0]);
   
          MXMLSetAttr(JE,(char *)MJobAttr[mjaReqProcs],(void *)&ProcCount,mdfInt);
          MXMLSetAttr(JE,(char *)MJobAttr[mjaState],(void *)MJobState[mjsRunning],mdfString);
          MXMLSetAttr(JE,(char *)MJobAttr[mjaStartTime],(void *)&StartTime,mdfLong);
   
          /* modify the name of the array master to include the number of
           * sub-jobs in this queue */
          MJobTransitionSetXMLArrayNameCount(J,JE,JobCount);
   
          MXMLAddE(AQE,JE);
          }
        } /* END if (ProcCount > 0) */
   
      /* check idle-reserved and idle jobs for sub-jobs of this array */
      ProcCount = 0;
      JobCount = 0;
   
      MJobTransitionGetArrayProcCount(J,&IRJList,&ProcCount);
      MJobTransitionGetArrayJobCount(J,&IRJList,&JobCount);
   
      MJobTransitionGetArrayProcCount(J,&IJList,&ProcCount);
      MJobTransitionGetArrayJobCount(J,&IJList,&JobCount);
   
      if ((ProcCount > 0) && (JobCount > 0)) /* if one is > 0, the other had better be... */
        {
        if ((EQE != NULL) &&
            (!bmisset(&FlagBM,mcmSummary)))
          {
          MJobTransitionBaseToXML(J,&JE,P,&FlagString[0]);
   
          MXMLSetAttr(JE,(char *)MJobAttr[mjaReqProcs],(void *)&ProcCount,mdfInt);
   
          /* modify the name of the array master to include the number of
           * sub-jobs in this queue */
          MJobTransitionSetXMLArrayNameCount(J,JE,JobCount);
   
          MXMLAddE(EQE,JE);
          }
        } /* END if (ProcCount > 0) */
   
      /* check blocked jobs for sub-jobs of this array */
      ProcCount = 0;
      JobCount = 0;
   
      MJobTransitionGetArrayProcCount(J,&BJList,&ProcCount);
      MJobTransitionGetArrayJobCount(J,&BJList,&JobCount);
   
      if ((ProcCount > 0) && (JobCount > 0)) /* if one is > 0, the other had better be... */
        {
        if ((BQE != NULL) &&
            (!bmisset(&FlagBM,mcmSummary)))
          {
          MJobTransitionBaseToXML(J,&JE,P,&FlagString[0]);
   
          MXMLSetAttr(JE,(char *)MJobAttr[mjaReqProcs],(void *)&ProcCount,mdfInt);
          MXMLSetAttr(JE,(char *)MJobAttr[mjaState],(void *)MJobState[mjsBlocked],mdfString);
   
          /* modify the name of the array master to include the number of
           * sub-jobs in this queue */
          MJobTransitionSetXMLArrayNameCount(J,JE,JobCount);
   
          /* if the HideBLocked flag is set don't let other users see blocked jobs */
          if (!bmisset(&MSched.DisplayFlags,mdspfHideBlocked) ||
              (IsAdmin == TRUE) ||
              (strcmp(Auth,J->User) == 0))
            MXMLAddE(BQE,JE);
          }
        } /* END if (ProcCount > 0) */
   
      /* check completed jobs for sub-jobs of this array */
      ProcCount = 0;
      JobCount = 0;
   
      MJobTransitionGetArrayProcCount(J,&CJList,&ProcCount);
      MJobTransitionGetArrayJobCount(J,&CJList,&JobCount);
   
      if ((ProcCount > 0) && (JobCount > 0)) /* if one is > 0, the other had better be... */
        {
        if ((CQE != NULL) &&
            (!bmisset(&FlagBM,mcmSummary)))
          {
          MJobTransitionBaseToXML(J,&JE,P,&FlagString[0]);
   
          MXMLSetAttr(JE,(char *)MJobAttr[mjaReqProcs],(void *)&ProcCount,mdfInt);
          MXMLSetAttr(JE,(char *)MJobAttr[mjaState],(void *)MJobState[mjsCompleted],mdfString);
   
          /* modify the name of the array master to include the number of
           * sub-jobs in this queue */
          MJobTransitionSetXMLArrayNameCount(J,JE,JobCount);
   
          MXMLAddE(CQE,JE);
          }
        } /* END if (ProcCount > 0) */
      }   /* END for (rindex... ) */
    }     /* END if (!bmisset(&FlagBM,mcmSummary)) */

  MUArrayListFree(&AJList);
  MUArrayListFree(&BJList);
  MUArrayListFree(&IJList);
  MUArrayListFree(&IRJList);
  MUArrayListFree(&CJList);
  MUArrayListFree(&ArrayMasterList);
  MCacheJobLock(FALSE,TRUE);

  MXMLSetAttr(AQE,"count",(void **)&AQCount,mdfInt);
  MXMLSetAttr(EQE,"count",(void **)&EQCount,mdfInt);
  MXMLSetAttr(BQE,"count",(void **)&BQCount,mdfInt);
  MXMLSetAttr(CQE,"count",(void **)&CQCount,mdfInt);

  MUArrayListCreate(&NList,sizeof(mtransnode_t *),10);

  MCacheNodeLock(TRUE,TRUE);
  MCacheReadNodes(&NList,&NConstraintList);
 

  for (rindex = 0;rindex < NList.NumItems;rindex++)
    {
    mpar_t *nodeP;

    N = (mtransnode_t *)MUArrayListGetPtr(&NList,rindex);

    if ((MParFind(N->Partition,&nodeP) == SUCCESS) &&
        (nodeP->RM != NULL) &&
        (nodeP->RM->Type == mrmtMoab))
      {
      IdleProcs   = &RemoteIdleProcs;
      UpProcs     = &RemoteUpProcs;   
      ActiveNodes = &RemoteActiveNodes;
      IdleNodes   = &RemoteIdleNodes;
      UpNodes     = &RemoteUpNodes;    
      ConfigNodes = &RemoteConfigNodes;
      }
    else
      {
      IdleProcs   = &LocalIdleProcs;
      UpProcs     = &LocalUpProcs;   
      ActiveNodes = &LocalActiveNodes;
      IdleNodes   = &LocalIdleNodes;    
      UpNodes     = &LocalUpNodes;    
      ConfigNodes = &LocalConfigNodes;
      }

    if (MNSISACTIVE(N->State)) {
      (*ActiveNodes)++;
      (*UpNodes)++;

      (*IdleProcs) += N->AvailableProcessors;
      (*UpProcs)   += N->ConfiguredProcessors;
      }
    else if (MNSISUP(N->State))
      {
      (*IdleNodes)++;
      (*UpNodes)++;

      (*IdleProcs) += N->AvailableProcessors;
      (*UpProcs)   += N->ConfiguredProcessors;
      }

    (*ConfigNodes)++;
    }

  MUArrayListFree(&NList);
  MCacheNodeLock(FALSE,TRUE);

  MUArrayListFree(&NConstraintList);

  if (MSched.BluegeneRM == TRUE)
    {
    int tmpInt;

    tmpInt = LocalUpProcs / MSched.BGNodeCPUCnt;
    MXMLSetAttr(CE,"LocalUpNodes",(void *)&tmpInt,mdfInt);

    tmpInt = LocalActiveProcs / MSched.BGNodeCPUCnt;
    MXMLSetAttr(CE,"LocalActiveNodes",(void *)&tmpInt,mdfInt);

    tmpInt = RemoteUpProcs / MSched.BGNodeCPUCnt;
    MXMLSetAttr(CE,"RemoteUpNodes",(void *)&tmpInt,mdfInt);

    tmpInt = RemoteActiveNodes / MSched.BGNodeCPUCnt;
    MXMLSetAttr(CE,"RemoteActiveNodes",(void *)&tmpInt,mdfInt);
    }
  else
    {
    MXMLSetAttr(CE,"LocalUpNodes",(void *)&LocalUpNodes,mdfInt);
    MXMLSetAttr(CE,"LocalActiveNodes",(void *)&LocalActiveNodes,mdfInt);
    MXMLSetAttr(CE,"RemoteUpNodes",(void *)&RemoteUpNodes,mdfInt);
    MXMLSetAttr(CE,"RemoteActiveNodes",(void *)&RemoteActiveNodes,mdfInt);
    }

  MXMLSetAttr(CE,"RemoteUpProcs",(void *)&RemoteUpProcs,mdfInt);
  MXMLSetAttr(CE,"RemoteIdleProcs",(void *)&RemoteIdleProcs,mdfInt);
  MXMLSetAttr(CE,"RemoteAllocProcs",(void *)&RemoteActiveProcs,mdfInt);
  MXMLSetAttr(CE,"RemoteIdleNodes",(void *)&RemoteIdleNodes,mdfInt);
  MXMLSetAttr(CE,"RemoteConfigNodes",(void *)&RemoteConfigNodes,mdfInt);

  MXMLSetAttr(CE,"LocalUpProcs",(void *)&LocalUpProcs,mdfInt);
  MXMLSetAttr(CE,"LocalIdleProcs",(void *)&LocalIdleProcs,mdfInt);
  MXMLSetAttr(CE,"LocalAllocProcs",(void *)&LocalActiveProcs,mdfInt);
  MXMLSetAttr(CE,"LocalIdleNodes",(void *)&LocalIdleNodes,mdfInt);
  MXMLSetAttr(CE,"LocalConfigNodes",(void *)&LocalConfigNodes,mdfInt);
  MXMLSetAttr(CE,"time",(void *)&MSched.Time,mdfLong);

  /* check scheduler state and add appropriate notes to the cluster element */

  MUIGetShowqNotes(IsAdmin,CE);

  return(SUCCESS);
  }  /* END MUIShowQueueThreadSafe() */
/* END MUIShowQueueThreadSafe.c */
