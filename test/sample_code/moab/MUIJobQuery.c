/* HEADER */

/**
 * @file MUIJobQuery.c
 *
 * Contains MUI Job Query
 */



#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



#define MMAX_COMPLETEJOBCOUNT     128

/**
 * This routine will go through each job with a required rsv and find
 * those that do not have an existing reservation to match the requirement.
 *
 * @param S (I)
 * @param RsvName (I) [optional]
 */

int __MUIJobQueryRsvOrphans(

  msocket_t *S,
  char      *RsvName)

  {
  job_iter JTI;

  char     *BPtr;
  int       BSpace;
  char      OrphanBuf[MMAX_BUFFER];

  mbool_t   NoJobs = TRUE;

  mjob_t   *J;
  mrsv_t   *R = NULL;
  mbool_t   RsvNameSpecified = FALSE;

  if ((RsvName != NULL) &&
      (RsvName[0] != '\0') &&
      strcasecmp(RsvName,"ALL"))
    {
    RsvNameSpecified = TRUE;

    if (MRsvFind(RsvName,&R,mraNONE) == FAILURE)
      {
      MRsvFind(RsvName,&R,mraRsvGroup);
      }
    }

  MUSNInit(&BPtr,&BSpace,OrphanBuf,sizeof(OrphanBuf));

  /* create header */

  MUSNPrintF(&BPtr,&BSpace,"Evaluating reservation job orphans\n\n");

  MUSNPrintF(&BPtr,&BSpace,"%-12s %12s %24s %12s\n\n",
    "State",
    "JobID",
    "JobName",
    "ReqRsv");

  MJobIterInit(&JTI);

  while (MJobTableIterate(&JTI,&J) == SUCCESS)
    {
    if ((J->ReqRID == NULL) ||
        (J->ReqRID[0] <= '\1')) 
      {
      /* skip jobs with no required reservations */

      continue;
      }

    NoJobs = FALSE;

    if (RsvNameSpecified == FALSE)
      {
      /* check for orphans of any reservation */

      if ((MRsvFind(J->ReqRID,NULL,mraNONE) == FAILURE) &&
          (MRsvFind(J->ReqRID,NULL,mraRsvGroup) == FAILURE))
        {
        /* tell the user that the reservation is dead */
        /* FORMAT: <JOB STATE>  <JOBID> <JOBNAME> <REQRID> */
        /* %-12s %12s %8s %12s */
        if (MJOBISACTIVE(J))
          MUSNPrintF(&BPtr,&BSpace,"%-12.12s  ",MJobState[J->State]);
        else
          MUSNPrintF(&BPtr,&BSpace,"%-12.12s  ",MJobState[J->State]);

        MUSNPrintF(&BPtr,&BSpace,"%12s %24s %12s\n",
          J->Name,
          MPRINTNULL(J->AName),
          J->ReqRID); 
        }
      } /* END if (RsvNameSpecified == FALSE) */
    else
      {
      /* just check orphans of the specified reservation */

      if (strcmp(J->ReqRID,RsvName))
        {
        /* skip jobs with a different reservation name */

        continue;
        }

      if (R == NULL)
        {
        /* reservation has ended */
        /* FORMAT: <JOB STATE>  <JOBID> <JOBNAME> <REQRID> */
        /* %-12s %12s %8s %12s */

        if (MJOBISACTIVE(J))
          MUSNPrintF(&BPtr,&BSpace,"%-12.12s  ",MJobState[J->State]);
        else
          MUSNPrintF(&BPtr,&BSpace,"%-12.12s  ",MJobState[J->State]);

        MUSNPrintF(&BPtr,&BSpace,"%12s %24s %12s\n",
          J->Name,
          MPRINTNULL(J->AName),
          J->ReqRID);
        }
      else
        {
        /* found the reservation */ 

        MUSNPrintF(&BPtr,&BSpace,"There are no orphans (reservation '%s' still exists for job '%s')\n",
          J->ReqRID,
          J->Name); 
        }
      } /* END else i.e. (RsvNameSpecified == FALSE) */
    }   /* END for (J = MJob[0]->Next) */

  if (NoJobs == TRUE)
    {
    MUSNPrintF(&BPtr,&BSpace,"No jobs currently require reservations!\n");
    }

  MUISAddData(S,OrphanBuf);

  return(SUCCESS);
  }  /* END __MUIJobQueryRsvOrphans() */




/**
 * Return job template information.
 *
 * @see __MUIJobQuery() - parent
 *
 * NOTE:  Uses MJobToXML() for reporting template attributes.
 *
 * @param E (I/O) - The XML element to write to
 * @param RE (I)  - The request XML element (to find the job expression)
 */

int MUIJobQueryTemplates(

  mxml_t *E,  /* I/O */
  mxml_t *RE) /* I */

  {
  int jindex;

  mjob_t     *TJ;
  mjtmatch_t *MJ;

  mxml_t *JE;
  char JobExpr[MMAX_LINE];

  enum MJobAttrEnum JAList[] = {
    mjaJobID,
    mjaAccount,
    mjaArgs,
    mjaClass,
    mjaCmdFile,
    mjaDescription,
    mjaFlags,
    mjaGroup,
    mjaPAL,
    mjaQOS,
    mjaRCL,
    mjaReqAWDuration,
    mjaReqHostList,    /**< requested hosts */
    mjaReqNodes,
    mjaReqReservation,
    mjaRM,
    mjaStatPSDed,
    mjaSysPrio,
    mjaTemplateFlags,
    mjaTrigger,
    mjaUser,
    mjaVMUsagePolicy,
    mjaVariables,
    mjaNONE };

  enum MReqAttrEnum RQAList[] = {
    mrqaDepend,           /* dependent job */
    mrqaGRes,          /* required GRes */
    mrqaNodeAccess,
    mrqaNodeSet,
    mrqaPref,
    mrqaReqArch, 
    mrqaReqDiskPerTask,
    mrqaReqMemPerTask,
    mrqaReqProcPerTask,
    mrqaReqNodeDisk,
    mrqaReqNodeFeature,
    mrqaReqNodeSwap,
    mrqaReqOpsys,
    mrqaReqSwapPerTask,
    mrqaTCReqMin,
    mrqaTPN,
    mrqaNONE };

  if ((E == NULL) || (RE == NULL))
    {
    return(FAILURE);
    }

  MXMLGetAttr(RE,"job",NULL,JobExpr,sizeof(JobExpr));

  /* report job templates */

  for (jindex = 0;MUArrayListGet(&MTJob,jindex);jindex++)
    {
    TJ = *(mjob_t **)(MUArrayListGet(&MTJob,jindex));

    if (TJ == NULL)
      break;

    if (bmisset(&TJ->IFlags,mjifTemplateIsDeleted))
      continue;

    if ((strcmp(JobExpr,"ALL")) &&
        (strcmp(JobExpr,"")) &&
        (strcmp(JobExpr,TJ->Name)))
      continue;

    JE = NULL;

    /* MJobToXML has the stuff to print the template stuff */

    MJobToXML(TJ,&JE,JAList,RQAList,NULL,FALSE,FALSE);

    MXMLAddE(E,JE);
    }  /* END for (jindex) */

  for (jindex = 0;jindex < MMAX_TJOB;jindex++)
    {
    if (MJTMatch[jindex] == NULL)
      break;

    MJ = MJTMatch[jindex];

    JE = NULL;

    MJobTMatchToXML(MJ,&JE,NULL);

    MXMLAddE(E,JE);
    }

  return(SUCCESS);
  }  /* END MUIJobQueryTemplates() */




#define MWF_PROCESSING_DEFFERED ((SUCCESS) << 1)

/**
 * Report job workflow information. (processes 'mjobctl -q workflow <JOBID>')
 *
 * Delegates the dependency graph calculation to
 * __MUIJobQueryWorkflowRecursive() and then reports the results in XML.
 *
 * @see Moab Workflow graphs in moabdocs.h
 * @see MUIJobQuery() - parent
 * @see MJobFindDependent() - child 
 *
 * @param J (I)
 * @param E (O)
 * @param EStartTime (I) [optional] earliest starttime for job - only used w/Mode==1
 * @param MaxDepth (I) maximum depth of nodes relative to root to print out in 
 *   the results, where directly linked nodes have a depth of 1, and nodes 
 *   linked to those have a depth of 2, etc.
 */

int __MUIJobQueryWorkflow(

  mjob_t *J,          /* I */
  mxml_t *E,          /* O */
  long    EStartTime, /* I (optional) - earliest starttime for job - only used w/Mode==1 */
  int     MaxDepth)   /* I maximum search depth */

  {
  mjworkflow_t  WList[MMAX_WORKFLOWJOBCOUNT];

  mxml_t *JE;
  mxml_t *DE;

  mulong tmpL;

  int dindex;
  int jindex;

  /* report XML in time sorted order */

  if ((J == NULL) || (E == NULL))
    {
    return(FAILURE);
    }

  /* FORMAT:  <job id="X" starttime="X" endtime="X" state="X"><depend type="X" target="X"></depend></job> */

  /* Step 1.0 Initialize */

  memset(WList,0,sizeof(WList));

  /* set all child indeces to -1 */

  for (jindex = 0;jindex < MMAX_WORKFLOWJOBCOUNT;jindex++)
    {
    for (dindex = 0;dindex < MMAX_WCHILD;dindex++)
      {
      WList[jindex].ChildIndex[dindex] = -1;
      }
    }

  __MUIJobQueryWorkflowRecursive(
    J,
    EStartTime,
    WList,
    NULL,
    MaxDepth,
    E);

  /* Report Results */

  for (jindex = 0;jindex < MMAX_WORKFLOWJOBCOUNT;jindex++)
    {
    if (WList[jindex].J == NULL)
      break;
    }

  /* Sort List */

  /* FORMAT:  <job id="X" starttime="X" endtime="X" state="X"><depend type="X" target="X"></depend></job> */

  /* Report list as XML */

  for (jindex = 0;jindex < MMAX_WORKFLOWJOBCOUNT;jindex++)
    {
    if (WList[jindex].J == NULL)
      break;

    if (WList[jindex].DepthIndex < 0)
      continue;

    J = WList[jindex].J;

    JE = NULL;

    MXMLCreateE(&JE,"job");

    MXMLSetAttr(JE,(char *)MJobAttr[mjaJobID],(void *)J->Name,mdfString);

    switch (J->State)
      {
      case mjsStarting:
      case mjsRunning:

        MXMLSetAttr(JE,(char *)MJobAttr[mjaState],(void *)"active",mdfString);

        break;

      case mjsCompleted:

        {
        char const * const Status = (J->CompletionCode == 0) ? "success" : "failure";

        MXMLSetAttr(JE,(char *)MJobAttr[mjaState],(void *)Status,mdfString);
        }

        break;

      case mjsRemoved:

        MXMLSetAttr(JE,(char *)MJobAttr[mjaState],(void *)"failure",mdfString);

        break;

      case mjsIdle:

        if (J->DependBlockReason == mdNONE)
          {
          MXMLSetAttr(JE,(char *)MJobAttr[mjaState],(void *)"idle",mdfString);
          }
        else
          {
          MXMLSetAttr(JE,(char *)MJobAttr[mjaState],(void *)"blocked",mdfString);
          }

        break;

      default:

        MXMLSetAttr(JE,(char *)MJobAttr[mjaState],(void *)"blocked",mdfString);

        break;
      }  /* END switch (J->State) */

    MXMLSetAttr(
      JE,
      (char *)MJobAttr[mjaStartTime],
      (void *)&WList[jindex].StartTime,
      mdfLong);

    if (MJOBISCOMPLETE(J))
      tmpL = J->CompletionTime;
    else
      tmpL = WList[jindex].StartTime + J->WCLimit;

    MXMLSetAttr(JE,(char *)MJobAttr[mjaCompletionTime],(void *)&tmpL,mdfLong);

    MXMLAddE(E,JE);

    /* add child dependencies */

    for (dindex = 0;dindex < MMAX_WCHILD;dindex++)
      {
      int ChildIndex = WList[jindex].ChildIndex[dindex];

      if (ChildIndex < 0)
        {
        /* end of list reached */

        break;
        }

      if (WList[ChildIndex].DepthIndex < 0)
        continue;

      DE = NULL;

      MXMLCreateE(&DE,"depend");

      MXMLSetAttr(DE,(char *)MJobAttr[mjaJobID],(void *)WList[ChildIndex].J->Name,mdfString);

      MXMLSetAttr(DE,(char *)"type",(void *)MDependType[WList[jindex].ChildType[dindex]],mdfString);

      MXMLAddE(JE,DE);
      }  /* END for (cindex) */
    }    /* END for (jindex) */

  return(SUCCESS);
  }  /* END __MUIJobQueryWorkflow() */



/**
 * Recursive portion of __MUIJobQueryWorkflow.
 * This function actually computes the dependency graph
 * and stores the results in WList
 *
 * @see Moab Workflow graphs in moabdocs.h
 * @see __MUIJobQueryWorkflow - parent
 *
 * @param J              (I)
 * @param EStartTime     (I)
 * @param WList          (I/O) [passed workflow list]
 * @param Parents        (I) null-terminated linked list of parents
 * @param DepthRemaining (I) Number of remaining allowable hops. Terminates at 0
 * @param ErrElement     (I) Element to store error messages on
 *
 * @return SUCCESS, FAILURE, or MWF_PROCESSING_DEFFERED 
 */

int __MUIJobQueryWorkflowRecursive(

  mjob_t       *J,
  long          EStartTime,
  mjworkflow_t *WList,
  mln_t const  *Parents,
  int           DepthRemaining,
  mxml_t       *ErrElement)

  {
  mdepend_t    *D;
  mln_t         SelfLink; /* store self as a parent for further recursive calls */

  mjob_t       *JParent = NULL;
  mjob_t       *JChild;

  job_iter JTI;
  job_iter CJTI;

  int           jindex;

  int           MyJIndex;

  mbool_t       PreExisting = FALSE;

  mulong        MaxParentEndTime;
  mulong        MyEndTime;

  /* routine is recursive */

  /* report XML in time sorted order */

  if (J == NULL)
    {
    return(FAILURE);
    }

  memset(&SelfLink,0,sizeof(SelfLink));
  SelfLink.Ptr = J;
  SelfLink.Next = (mln_t *)Parents;

  /* FORMAT:  <job id="X" starttime="X" endtime="X" state="X"><depend type="X" target="X"></depend></job> */

  /* Step 1.0 Initialize */

  /* Step 2.0 Add self to list */

  for (jindex = 0;jindex < MMAX_WORKFLOWJOBCOUNT;jindex++)
    {
    if (WList[jindex].J == NULL)
      {
      WList[jindex].J = J;

      break;
      }

    if (WList[jindex].J == J)
      {
      /* self already added */

      PreExisting = TRUE;

      break;
      }
    }  /* END for (jindex) */

  if (jindex >= MMAX_WORKFLOWJOBCOUNT)
    {
    return(FAILURE);
    }

  MyJIndex = jindex;

  WList[MyJIndex].DepthIndex = DepthRemaining;

  /* Step 3.0 Determine All Parents */

  MaxParentEndTime = 0;

  D = J->Depend;

  while ((PreExisting == FALSE) && (D != NULL))
    {
    JParent = NULL;

    switch (D->Type)
      {
      case mdJobStart:                /* job must have previously started */
      case mdJobCompletion:           /* job must have previously completed */
      case mdJobSuccessfulCompletion: /* job must have previously completed successfully */
      case mdJobFailedCompletion:     /* job must have previously completed unsuccessfully */
      case mdJobSyncSlave:            /* master job must start at same time */

        if ((MJobFind(D->Value,&JParent,mjsmExtended) == FAILURE) &&
            (MJobCFind(D->Value,&JParent,mjsmBasic) == FAILURE))
          {
          /* cannot locate depend parent */

          D = D->NextAnd;

          JParent = NULL;
          }

        break;

      case mdJobSyncMaster:
      default:

        /* ignore dependency */

        break;
      }  /* END switch (D->Type) */

    if (JParent == NULL)
      {
      continue;
      }

    /* Step 3.1 For Each New Parent, Recursively Call Parent */

    for (jindex = 0;jindex < MMAX_WORKFLOWJOBCOUNT;jindex++)
      {
      if ((WList[jindex].J == JParent) && (!WList[jindex].StartTimeIsComputed))
        {
        mln_t const *Node;
        
        /* check for cycle in parents list */

        for (Node = &SelfLink;Node != NULL;Node = Node->Next)
          {
          if (Node->Ptr == JParent)
            {
            /* cycle detected. Report as error and return */

            mxml_t *ErrChild = NULL;
            mln_t *Node;
            mstring_t MStr(MMAX_LINE);

            MStringAppend(&MStr,JParent->Name);

            for (Node = &SelfLink;Node != NULL;Node = Node->Next)
              {
              MStringAppendF(&MStr,",%s",((mjob_t *)Node->Ptr)->Name);
              }

            MXMLAddChild(ErrElement,"Error",NULL,&ErrChild);

            MXMLSetAttr(ErrChild,"Type",(void *)"Cycle",mdfString);
            MXMLSetAttr(ErrChild,"Cycle",MStr.c_str(),mdfString);

            WList[MyJIndex].StartTime = MMAX_TIME;
            WList[MyJIndex].StartTimeIsComputed = TRUE;

            return(FAILURE);
            }
          }    /* END for (Node) */

        /* No cycle detected, but the current parent has already started to be processed
         * and we cannot process the current node yet */

        WList[MyJIndex].J = NULL;

        return(MWF_PROCESSING_DEFFERED);
        }  /* END if ((WList[jindex].J == JParent) && ...) */

      if (WList[jindex].J == NULL)
        {
        int cindex;
        int rc;

        /* end of list reached - evaluate parent */

        rc = __MUIJobQueryWorkflowRecursive(JParent,-1,WList,&SelfLink,DepthRemaining - 1,ErrElement);
 
        if (rc != SUCCESS)
          {
          WList[MyJIndex].J = NULL;

          return(rc);
          }

        /* NOTE:  recurvsive call is guaranteed to populate next available slot, 
                  ie, WList[jindex] */

        /* Step 3.1.1 Add self as a child to parent */

        for (cindex = 0;cindex < MMAX_WCHILD;cindex++)
          {
          int ChildIndex = WList[jindex].ChildIndex[cindex];

          if (ChildIndex < 0)
            {
            /* add dependency */

            if ((jindex >= MMAX_WORKFLOWJOBCOUNT) || 
                (MyJIndex >= MMAX_WORKFLOWJOBCOUNT))
              {
              /* this should never actually happen in practice. If it does then anything goes, including future SIGSEGVs */

              return(FAILURE);
              }

            WList[jindex].ChildIndex[cindex] = MyJIndex;

            WList[jindex].ChildType[cindex] = D->Type;

            break;
            }

          if (WList[ChildIndex].J == J)
            {
            /* dependency already added */

            break;
            }
          }  /* END for (cindex) */

        /* Step 3.1.2 Update parent end time */

        MaxParentEndTime = MAX(MaxParentEndTime,WList[jindex].StartTime + JParent->WCLimit);

        break;
        }  /* END if (WList[jindex].J == NULL) */

      if (WList[jindex].J == JParent)
        {
        int cindex;
        mjworkflow_t * const Parent = WList + jindex;

        /* parent already located - take no action */

        Parent->DepthIndex = MAX(DepthRemaining - 1,Parent->DepthIndex);
 
        MaxParentEndTime = MAX(MaxParentEndTime,Parent->StartTime + JParent->WCLimit);
 
        /* not done computing parent */

        if (!WList[jindex].StartTimeIsComputed)
          break;

        /* add self to parent if parent was unable to add self earlier */

        for (cindex = 0;cindex < MMAX_WCHILD; cindex++)
          {
          if ((Parent->ChildIndex[cindex] == MyJIndex) && 
              (Parent->ChildType[cindex] == D->Type))
            {
            break;
            }

          if (Parent->ChildIndex[cindex] == -1)
            {
            Parent->ChildIndex[cindex] = MyJIndex;
            Parent->ChildType[cindex] = D->Type;

            break;
            }
          }    /* END for (cindex) */

        break;
        }  /* END if (WList[jindex].J == JParent) */
      }    /* END for (jindex) */

    D = D->NextAnd;
    }  /* END while ((PreExisting == FALSE) && ...) */

  /* Step 4.0 Determine StartTime */

  if (MJOBISACTIVE(J) || MJOBISCOMPLETE(J))
    {
    WList[MyJIndex].StartTime = J->StartTime;
    }
  else if (J->Rsv != NULL)
    {
    WList[MyJIndex].StartTime = J->Rsv->StartTime;
    }
  else
    {
    /* job is idle/blocked */

    if (EStartTime > 0)
      {
      /* exploring child and already know my end time */

      WList[MyJIndex].StartTime = MAX(J->StartTime,(mulong)EStartTime);
      }

    WList[MyJIndex].StartTime = MAX(WList[MyJIndex].StartTime,MSched.Time);
    WList[MyJIndex].StartTime = MAX(WList[MyJIndex].StartTime,MaxParentEndTime);
    WList[MyJIndex].StartTime = MAX(WList[MyJIndex].StartTime,WList[MyJIndex].J->SpecSMinTime);
    }

  /* Step 5.0 Traverse All Children */

  MJobIterInit(&JTI);
  MCJobIterInit(&CJTI);

  WList[MyJIndex].StartTimeIsComputed = TRUE;

  while ((PreExisting == FALSE) && 
         (MJobFindDependent(J,&JChild,&JTI,&CJTI) == SUCCESS))
    {
    int cindex;
    int rc;
    int ChildIndex;

    for (jindex = 0;jindex < MMAX_WORKFLOWJOBCOUNT;jindex++)
      {
      if (WList[jindex].J == NULL)
        {
        /* end of list reached - child not located - child is new */

        break;
        }

      if (WList[jindex].J == JChild)
        {
        /* child already located - take no action */

        break;
        }
      }  /* END for (jindex) */

    if (jindex >= MMAX_WORKFLOWJOBCOUNT)
      break;

    /* Step 5.1 Add Child Dependency Info */

    for (cindex = 0;cindex < MMAX_WCHILD;cindex++)
      {
      ChildIndex = WList[MyJIndex].ChildIndex[cindex];

      if (ChildIndex < 0)
        {
        mdepend_t *DChild;

        /* add dependency */

        if ((jindex >= MMAX_WORKFLOWJOBCOUNT) ||
            (MyJIndex >= MMAX_WORKFLOWJOBCOUNT))
          {
          /* this should never actually happen in practice. If it does then anything goes, including future SIGSEGVs */

          return(FAILURE);
          }

        WList[MyJIndex].ChildIndex[cindex] = jindex;

        DChild = JChild->Depend;

        while (DChild != NULL)
          {
          if (!strcmp(DChild->Value,J->Name))
            break;

          DChild = DChild->NextAnd; 
          }  /* END while (DChild != NULL) */

        if (DChild != NULL)
          WList[MyJIndex].ChildType[cindex] = DChild->Type; /* mdJobCompletion; */
        else
          {
          /* TODO: Do we (who?) need a value in there if the DChild wasn't found? */
          }

        break;
        }

      if (WList[ChildIndex].J == JChild)
        {
        /* dependency already added */

        break;
        }
      }  /* END for (cindex) */

    if (WList[jindex].J != NULL)
      {
      WList[jindex].DepthIndex = MAX(DepthRemaining -1,WList[jindex].DepthIndex);

      /* child already processed - ignore */

      continue;
      }

    /* Step 5.2 Determine Child StartTime */

    MyEndTime = WList[MyJIndex].StartTime + J->WCLimit;

    /* Step 5.3 If Child is New, Call Child */

    rc = __MUIJobQueryWorkflowRecursive(
        JChild,
        MyEndTime,
        WList,
        NULL,
        DepthRemaining - 1,
        ErrElement);

    if (rc != SUCCESS)
      {
      WList[MyJIndex].ChildType[cindex]  =  mdNONE;
      WList[MyJIndex].ChildIndex[cindex] = -1;

      continue;
      }
    }    /* END while ((PreExisting == FALSE) && (MJobFindDependent() == SUCCESS)) */

  return(SUCCESS);
  }  /* END __MUIJobQueryWorkflow() */





/**
 * Handle 'mjobctl -q' command.
 *
 * NOTE:  Also handles 'showstart' command (@see __MUIJobGetEStartTime()).
 *
 * @see MUIJobCtl() - parent
 * @see __MUIJobToXML()
 * @see MUIJobQueryTemplates() - child
 * @see __MUIJobQueryWorkflow() - child
 *
 * @param JSpec      (I) [format: <JOBID>|???]
 * @param Auth       (I)
 * @param JC         (I) [optional - only used to query completed jobs]
 * @param P          (I)
 * @param S          (I/O)
 * @param TimeString (I)
 * @param RE         (I) request
 */

int MUIJobQuery(

  char             *JSpec,
  char             *Auth,
  mjobconstraint_t *JC,
  mpar_t           *P,
  msocket_t        *S,
  char             *TimeString,
  mxml_t           *RE)

  {
  char FlagString[MMAX_LINE];
  char ArgString[MMAX_LINE];

  char QueryAttribute[MMAX_LINE];
  char tmpName[MMAX_NAME];
  
  mclass_t *tC = NULL;

  enum MJobAttrEnum AIndex;
  int  jindex;

  mjob_t *J;

  long StartTime;
  long EndTime;

  mxml_t *JE;
  mxml_t *DE;

  mbool_t DoQueryCompleted = FALSE;

  mgcred_t *U;

  char tmpLine[MMAX_LINE];

  int  CTok;
 
  char DelimBuf[2];

  const char *FName = "MUIJobQuery";

  MDB(2,fUI) MLog("%s(%s,%s,JC,%s,S,RE)\n",
    FName,
    (JSpec != NULL) ? JSpec : "NULL",
    (Auth != NULL) ? Auth : "NULL",
    (P != NULL) ? P->Name : "NULL");

  if (JSpec == NULL)
    {
    return(FAILURE);
    }

  strcpy(DelimBuf,",");
  
  MXMLGetAttr(RE,MSAN[msanFlags],NULL,FlagString,0);

  MXMLGetAttr(RE,MSAN[msanArgs],NULL,ArgString,0);

  MUStrToLower(FlagString);

  if (strstr(FlagString,"completed") != NULL)
    DoQueryCompleted = TRUE;

  /* create response */

  if (MXMLCreateE(&S->SDE,(char *)MSON[msonData]) == FAILURE)
    {
    MUISAddData(S,"ERROR:    cannot create response\n");

    return(FAILURE);
    }

  /* parse request */

  DE = S->SDE;

  /* determine query attribute type */

  MXMLGetAttr(RE,MSAN[msanArgs],NULL,QueryAttribute,sizeof(QueryAttribute));

  CTok = -1;

  while (MS3GetWhere(
        RE,
        NULL,
        &CTok,
        tmpName,            /* O */
        sizeof(tmpName),
        tmpLine,            /* O */
        sizeof(tmpLine)) == SUCCESS)
    {
    if (!strcasecmp(tmpName,"delim"))
      {
      DelimBuf[0] = tmpLine[0];
      }
    else if (!strcasecmp(tmpName,"class") && strcasecmp(tmpLine,NONE))
      {
      if (MClassFind(tmpLine,&tC) == FAILURE)
        {
        MUISAddData(S,"invalid class specified");

        return(FAILURE);
        }
      }
    }    /* END while (MS3GetWhere() == SUCCESS) */

  if (!strcasecmp(QueryAttribute,MJobAttr[mjaStartTime]))
    {
    /* handle 'showstart' command */

    /* only show start time for specific jobs or job description */    

    mxml_t *JE;

    char PeerName[MMAX_NAME];
    char EString[MMAX_LINE];

    PeerName[0] = '\0';

    MXMLGetAttr(RE,"peer",NULL,PeerName,sizeof(PeerName));

    /* __MUIJobGetEStartTime() may return info for both local resources and peers */

#if 0  /* there's no reason for this call that I can see */
>>>>>>> other
    /* NOTE:  issue MJobFromJSpec() to populate EString */

    MJobMakeTemp(&J);

    if (MJobFromJSpec(
          JSpec,            /* I */
          &J,               /* I/O */
          S->RDE,           /* I */
          Auth,             /* I */
          NULL,
          EString,          /* O */
          sizeof(EString),
          NULL) == FAILURE)
      {
      MJobFreeTemp(&J);

      MUISAddData(S,EString);

      return(FAILURE);
      }

    MJobFreeTemp(&J);
#endif

    /* NOTE:  class/queue info should be routed in to __MUIJobGetEStartTime() */

    if (__MUIJobGetEStartTime(JSpec,Auth,S,FlagString,PeerName,tC,&JE) == FAILURE)
      {
      sprintf(EString,"\nINFO:  cannot determine start time for job %s\n\n",
        JSpec);

      MUISAddData(S,EString);

      return(SUCCESS);
      }
  
    /* job start time information available */

    MXMLAddE(DE,JE);

    return(SUCCESS);
    }  /* END if (strcasecmp(QueryAttribute,MJobAttr[mjaStartTime])) */
  else if (!strcasecmp(QueryAttribute,MJobAttr[mjaReqHostList]))
    {
    char tmpBuffer[MMAX_BUFFER << 2];  /* FIXME: make dynamic */

    if (MJobFind(JSpec,&J,mjsmExtended) == FAILURE)
      {
      if (!strcmp(JSpec,"ALL"))
        {
        snprintf(tmpBuffer,sizeof(tmpBuffer),"Currently there are no active jobs\n");
        }
      else
        {
        snprintf(tmpBuffer,sizeof(tmpBuffer),"ERROR:  cannot locate job '%s'\n",
          JSpec);
        }

      MUISAddData(S,tmpBuffer);

      return(FAILURE);
      }

    /* by default, display comma delimited */

    /* look at allocated host list, reserved host list, or required hostlist */

    if (!MNLIsEmpty(&J->NodeList))
      {
      MNLToString(&J->NodeList,FALSE,DelimBuf,'\0',tmpBuffer,sizeof(tmpBuffer));
      }
    else if ((J->Rsv != NULL) && 
             (!MNLIsEmpty(&J->Rsv->NL)))
      {
      MNLToString(&J->Rsv->NL,FALSE,DelimBuf,'\0',tmpBuffer,sizeof(tmpBuffer));
      }
    else
      {
      sprintf(tmpBuffer,"ERROR:  job has no hostlist\n");
      }

    MUISAddData(S,tmpBuffer);

    return(SUCCESS);
    }  /* END else if (strcasecmp(QueryAttribute,MJobAttr[mjaReqHostList])) */
  else if (!strcasecmp(QueryAttribute,"stats"))
    {
    char EString[MMAX_LINE];

    long ETime = MMAX_TIME;
    long STime;

    /* query job template statistics */

    if (!strcmp(JSpec,"ALL"))
      {
      J = NULL;
      }
    else if (MTJobFind(JSpec,&J) == SUCCESS)
      {
      /* NO-OP */
      }
    else if (MJobFromJSpec(
          JSpec,
          &J,  /* O */
          S->RDE,
          Auth,
          NULL,
          EString,
          sizeof(EString),
          NULL) == FAILURE)
      {
      MUISAddData(S,EString);

      return(FAILURE);
      }

    if ((TimeString == NULL) || (TimeString[0] == '\0'))
      {
      MUGetPeriodRange(MSched.Time,0,0,mpDay,&STime,NULL,NULL);

      ETime = MSched.Time;
      }
    else
      {
      MStatGetRange(TimeString,&STime,&ETime);
      }

    MTJobStatProfileToXML(
      J,
      NULL,
      NULL,
      STime,
      ETime,
      -1,
      FALSE,
      TRUE,
      S->SDE);

    return(SUCCESS);
    }
  else if (!strcasecmp(QueryAttribute,"template"))
    {
    MUIJobQueryTemplates(S->SDE,RE);

    return(SUCCESS);
    }
  else if (!strcasecmp(QueryAttribute,"workflow"))
    {
    char tmpMsg[MMAX_LINE];
    int const DefaultMaxDepth = 128;
    int MaxDepth = DefaultMaxDepth;
    char WhereName[MMAX_NAME];
    char WhereValue[MMAX_NAME];
    mxml_t *WE = NULL;
    int WTok = 0;

    if ((MJobFind(JSpec,&J,mjsmExtended)  == FAILURE) &&
        (MJobCFind(JSpec,&J,mjsmBasic) == FAILURE))
      {
      snprintf(tmpMsg,sizeof(tmpMsg),"ERROR:  cannot locate job '%s'\n",
        JSpec);

      MUISAddData(S,tmpMsg);

      return(FAILURE);
      }

    /* search for the max-depth where attribute */

    while (MS3GetWhere(
        RE,
        &WE,
        &WTok,
        WhereName,
        sizeof(WhereValue),
        WhereValue,
        sizeof(WhereValue)) == SUCCESS)
      {
      if (!strcmp(WhereName,"max-depth"))
        {
        char *EndPtr;
        MaxDepth = strtol(WhereValue,&EndPtr,10);

        if (MaxDepth <= 0 || EndPtr < WhereValue + strlen(WhereValue) )
          {
          MDB(3,fUI) MLog("ALERT:    invalid max depth value '%s'\n",
            WhereValue);

          MaxDepth = DefaultMaxDepth;
          }
        }
      }

    __MUIJobQueryWorkflow(J,S->SDE,-1,MaxDepth);

    return(SUCCESS);
    }
  else if ((!strncasecmp(QueryAttribute,"rsvorphans",strlen("rsvorphans"))))
    {
    char RsvName[MMAX_NAME];

    /* reservation names look similar to job names and are stored in the job attribute when parsed into xml */
    MXMLGetAttr(RE,"job",NULL,RsvName,sizeof(RsvName));

    __MUIJobQueryRsvOrphans(S,RsvName);

    return(SUCCESS);
    }
  else if (!strcasecmp(QueryAttribute,"wiki"))
    {
    if (JSpec == NULL)
      {
      MUISAddData(S,"ERROR:  no job specified\n");

      return(FAILURE);
      }

    mstring_t WikiString(MMAX_LINE);

    MUserFind(Auth,&U);

    if (strcmp(JSpec,"ALL"))
      {
      /* one specific job */

      if (MJobFind(JSpec,&J,mjsmExtended) == FAILURE)
        {
        MStringSetF(&WikiString,"ERROR:  could not find job '%s'\n",
          JSpec);

        MUISAddData(S,WikiString.c_str());

        return(FAILURE);
        }

      if (MUICheckAuthorization(
              U,
              NULL,
              (void *)J,
              mxoJob,
              mcsMJobCtl,
              mjcmQuery,
              NULL,
              tmpLine,
              sizeof(tmpLine)) == FAILURE)
          {
          MUISAddData(S,"ERROR:  User does not have access to requested job");

          return(FAILURE);
          }

      MJobToWikiString(J,NULL,&WikiString);
      }
    else
      {
      job_iter JTI;

      /* All jobs */

      mstring_t tmpString(MMAX_LINE);

      MJobIterInit(&JTI);

      /* only support full job query with ALL */

      while (MJobTableIterate(&JTI,&J) == SUCCESS)
        {
        if (MUICheckAuthorization(
              U,
              NULL,
              (void *)J,
              mxoJob,
              mcsMJobCtl,
              mjcmQuery,
              NULL,
              tmpLine,
              sizeof(tmpLine)) == FAILURE)
          {
          continue;
          }

        tmpString.clear();

        MJobToWikiString(J,NULL,&tmpString);

        MStringAppend(&WikiString,tmpString.c_str());
        } /* END for (J = MJob[0]->Next; ...) */
      } /* END else (!strcmp(JSpec,"ALL")) */

    MUISAddData(S,WikiString.c_str());

    return(SUCCESS);
    }
  else if ((!strcasecmp(QueryAttribute,"diag")) ||
           (QueryAttribute[0] == '\0'))
    {
    /* NO-OP */

    /* default is diag */
    }

  if (MUserFind(Auth,&U) == FAILURE)
    {
    /* HACK:  removed to allow DEFAULT user to perform actions */

    /* return(FAILURE); */
    }
  
  /* NOTE:  process job attributes (ie, uname, starttime, diag, etc) */

  if (DoQueryCompleted == TRUE)
    {
    int jcindex;

    mjob_t *tmpCJList = NULL;

    /* default query examines last 24 hours */

    StartTime = MSched.Time - MCONST_DAYLEN;
    EndTime   = MMAX_TIME;

    for (jcindex = 0;JC[jcindex].AType != mjaNONE;jcindex++)
      {
      if (JC[jcindex].AType == mjaStartTime)
        {
        StartTime = JC[jcindex].ALong[0];
        }
      else if (JC[jcindex].AType == mjaCompletionTime)
        {
        if (JC[jcindex].ACmp == mcmpLE)
          {
          /* by default, look 2 weeks back */

          StartTime = JC[jcindex].ALong[0] - (MCONST_WEEKLEN * 2);
          }
        else if (JC[jcindex].ACmp == mcmpGE)
          {
          StartTime = JC[jcindex].ALong[0];
          }
        }
      }     /* END for (jcindex) */

    /* below line used to be if (StartTime >= (long)MSched.Time - MSched.JobCPurgeTime) - didn't work! */
      
    if ((long)MSched.Time >= MAX(StartTime,(long)MSched.Time - MSched.JobCPurgeTime))
      {
      job_iter JTI;

      /* load jobs from internal cache */

      MDB(6,fUI) MLog("INFO:     reporting completed jobs\n");

      MCJobIterInit(&JTI);

      while (MCJobTableIterate(&JTI,&J) == SUCCESS)
        {
        if (strcmp(JSpec,"ALL") && strcmp(JSpec,J->Name))
          continue;

        if (MUICheckAuthorization(
              U,
              NULL,
              (void *)J,
              mxoJob,
              mcsMJobCtl,
              mjcmQuery,
              NULL,
              tmpLine,
              sizeof(tmpLine)) == FAILURE)
         {
         MDB(2,fUI) MLog("%s",tmpLine);

         continue;
         }

        if ((J->CompletionTime < (mulong)StartTime) ||
            (J->CompletionTime > (mulong)EndTime))
          {
          /* we need to make sure that jobs in the internal CJob queue are
           * within the user's specificed time frame */

          continue;
          }

        if (MJobCheckConstraintMatch(J,JC) == SUCCESS)
          {
          __MUIJobToXML(J,0,&JE);  /* inside '(DoQueryCompleted == TRUE)' */

          MXMLAddE(DE,JE);
          }  /* END for (jindex) */
        }

      /* return(SUCCESS); - we want to see ALL jobs in the specified time window, not just completed jobs in mem. */
      } /* END if ((long)MSched.Time >= MAX(StartTime,(long)MSched.Time - MSched.JobCPurgeTime) */

    /* load jobs from stats files */

    if (MSched.WikiEvents == TRUE)
      {
      /* NOTE:  MTrace reads the events file using the old format.  Wiki format in MTrace has not been inplemented. (NYI).
       *        This code has been put here to stop error message like "ALERT: workload trace is corrupt" so customers don't worry. MOAB-4430  */

      return(SUCCESS);
      }

    MDB(6,fUI) MLog("INFO:     reporting completed jobs (stats)\n");

    tmpCJList = (mjob_t *)MUCalloc(1,sizeof(mjob_t) * MMAX_COMPLETEJOBCOUNT);

    if (tmpCJList == NULL)
      {
      MUISAddData(S,"internal failure locating matching jobs");

      return(FAILURE);
      }

    if (MTraceQueryWorkload(
          StartTime,
          EndTime,
          JC,
          tmpCJList,  /* O */
          MMAX_COMPLETEJOBCOUNT) == FAILURE)
      {
      MUISAddData(S,"cannot locate matching jobs");

      MJobDestroyList(tmpCJList,MMAX_COMPLETEJOBCOUNT);

      MUFree((char **)&tmpCJList);

      return(FAILURE);
      }

    for (jindex = 0;jindex < MMAX_COMPLETEJOBCOUNT;jindex++)
      {
      if (tmpCJList[jindex].Name[0] == '\0')
        break;

      if (MJobCFind(tmpCJList[jindex].Name,NULL,mjsmBasic) == SUCCESS)
        {
        /* check to make sure that we haven't already found and reported this job
         * from the internal MCJob (see code above) */

        continue;
        }

      J = &tmpCJList[jindex];

      if (MUICheckAuthorization(
            U,
            NULL,
            (void *)J,
            mxoJob,
            mcsMJobCtl,
            mjcmQuery,
            NULL,
            tmpLine,
            sizeof(tmpLine)) == FAILURE)
       {
       MDB(2,fUI) MLog("%s",tmpLine);

       continue;
       }

      __MUIJobToXML(J,0,&JE);  /* inside (DoQueryCompleted == TRUE) */

      MXMLAddE(DE,JE);
      }  /* END for (jindex) */

    MJobDestroyList(tmpCJList,MMAX_COMPLETEJOBCOUNT);

    MUFree((char **)&tmpCJList);

    return(SUCCESS);
    }  /* END if (DoQueryCompleted == TRUE) */

  if (!strcmp(JSpec,"ALL"))
    {
    job_iter JTI;

    /* process 'mjobctl -q diag ALL' */

    MJobIterInit(&JTI);

    /* only support full job query with ALL */

    while (MJobTableIterate(&JTI,&J) == SUCCESS)
      {
      if (MUICheckAuthorization(
            U,
            NULL,
            (void *)J,
            mxoJob,
            mcsMJobCtl,
            mjcmQuery,
            NULL,
            tmpLine,
            sizeof(tmpLine)) == FAILURE)
       {
       MDB(2,fUI) MLog("%s",tmpLine);

       continue;
       }

      __MUIJobToXML(J,0,&JE);

      if (J->TemplateExtensions != NULL)
        {
        /* add dynamic job attributes which do not have match job attr enums */

        MUIJTXToXML(J,JE);
        }  /* END if (J->TX != NULL) */

      MXMLAddE(DE,JE);
      }  /* END for (jindex) */

    return(SUCCESS);
    }  /* END if (!strcmp(JSpec,"ALL")) */

  /* at this point, JSpec must be individual job */

  MJobFind(JSpec,&J,mjsmExtended);

  if ((J == NULL) || (S->SDE == NULL))
    {
    /* cannot locate job */

    if (J == NULL)
      MUISAddData(S,"cannot locate job");
    else
      MUISAddData(S,"internal error");

    return(FAILURE);
    }

  if (MUICheckAuthorization(
        U,
        NULL,
        (void *)J,
        mxoJob,
        mcsMJobCtl,
        mjcmQuery,
        NULL,
        tmpLine,
        sizeof(tmpLine)) == FAILURE)
   {

   MDB(2,fUI) MLog("%s",
     tmpLine);

   /* display the failure on the command line */

   MUISAddData(S,tmpLine);

   return(FAILURE);
   }

  JE = NULL;

  MXMLCreateE(&JE,(char *)MXO[mxoJob]);

  MXMLSetAttr(
    JE,
    (char *)MJobAttr[mjaJobID],
    (void **)J->Name,
    mdfString);

  AIndex = (enum MJobAttrEnum)MUGetIndexCI(QueryAttribute,MJobAttr,FALSE,0);

  switch (AIndex)
    {
    case mjaStartTime:

      /* handled previously */

      /* NO-OP */

      break;

    case mjaNONE:

      /* display all basic job attributes */

      __MUIJobToXML(J,NULL,&JE);

      if (J->TemplateExtensions != NULL)
        {
        /* add dynamic job attributes which do not have match job attr enums */

        MUIJTXToXML(J,JE);
        }  /* END if (J->TX != NULL) */

      break;

    default:

      /* unexpected attribute */

      /* NO-OP */

      break;
    }  /* END switch (AIndex) */

  MXMLAddE(DE,JE);

  return(SUCCESS);
  }  /* END MUIJobQuery() */


