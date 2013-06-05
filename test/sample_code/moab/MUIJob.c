/* HEADER */

      
/**
 * @file MUIJob.c
 *
 * Contains: Misc MUI Job functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



int __MUIJobTemplateCtl(msocket_t *,char *);
int __MUIJobGetStart(char *,mpar_t **,mclass_t *,mulong *,mxml_t **,char *,int,msocket_t *,char *);

/**
 * This MUI internal function is a handler for job migrate requests. This
 * function SHOULD be invoked after the caller has already verified that Auth
 * is an admin or owner of the job. (It is usually called by MUIJobCtl().)
 * Based on the flags and XML inputs, the given job will be appropriately
 * migrated. If a socket object is given, then an appropriate success/error
 * message will be attached to the socket.
 *
 * @see MUIJobCtl() - parent
 *
 * @param J (I) Job to migrate. [modified]
 * @param Auth Name/identifier of the user/peer making this request. (I)
 * @param S (I) Socket on which to return success/error messages. [optional,modified]
 * @param RE XML describing the requested modification. (I)
 *
 * @return Will return SUCCESS if no Set requests detected.
 */

int __MUIJobMigrate(

  mjob_t    *J,
  char      *Auth,
  msocket_t *S,
  mxml_t    *RE)

  {
  int        rc;

  char     **VMList = NULL;
  mnl_t      NL = {0};

  enum MStatusCodeEnum tmpSC;

  char       EMsg[MMAX_LINE] = {0};

  int        tindex;
  int        nindex;

  int        WTok;

  mxml_t    *WE;

  char       tmpName[MMAX_NAME];
  char       tmpVal[MMAX_LINE];

  mnode_t   *SrcN[64];
  mnode_t   *DstN[64];

  mnode_t   *N;

  mbool_t    SwapVMs    = TRUE;

  mvm_t     *SV;  /* source vm data */
  mvm_t     *DV;  /* destination vm data */

  mln_t     *svptr;
  mln_t     *dvptr;

  /* initialize */

  SrcN[0] = NULL;
  SrcN[1] = NULL;

  DstN[0] = NULL;
  DstN[1] = NULL;

  /* process 'where' constraints */

  WTok = -1;

  while (MS3GetWhere(
      RE,
      &WE,
      &WTok,
      tmpName,          /* O */
      sizeof(tmpName),
      tmpVal,           /* O */
      sizeof(tmpVal)) == SUCCESS)
    {
    if (!strcasecmp(tmpName,"srcnode"))
      {
      if (MNodeFind(tmpVal,&SrcN[0]) == FAILURE)
        {
        snprintf(EMsg,sizeof(EMsg),"cannot locate source node '%s'",
          tmpVal);

        MUISAddData(S,EMsg);

        return(FAILURE);
        }
      }
    else if (!strcasecmp(tmpName,"dstnode"))
      {
      if (MNodeFind(tmpVal,&DstN[0]) == FAILURE)
        {
        snprintf(EMsg,sizeof(EMsg),"cannot locate destination node '%s'",
          tmpVal);

        MUISAddData(S,EMsg);

        return(FAILURE);
        }
      }
    else if (!strcasecmp(tmpName,"swap"))
      {
      SwapVMs = MUBoolFromString(tmpVal,FALSE);
      }
    }    /* END while (MS3GetWhere() == SUCCESS) */


  /* NOTE:  swap vm's unless noswap is true */

  VMList = (char **)MUCalloc(1,sizeof(char *) * MSched.M[mxoNode]);

  tindex = 0;

  MNLInit(&NL);

  for (nindex = 0;SrcN[nindex] != NULL;nindex++)
    {
    if (DstN[nindex] == NULL)
      {
      strcpy(EMsg,"Destination node must be specified");

      MUISAddData(S,EMsg);

      MUFree((char **)&VMList);
      MNLFree(&NL);

      return(FAILURE);
      }

    /* determine VM's located on source */
    N = SrcN[nindex];

    if (N->VMList == NULL) 
      continue;

    dvptr = DstN[nindex]->VMList;

    for (svptr = N->VMList;svptr != NULL;svptr = svptr->Next)
      {
      SV = (mvm_t *)svptr->Ptr;

      if ((SV == NULL) || (SV->ARes.Procs == SV->CRes.Procs))
        {
        /* vm is idle - continue search */

        continue;
        }

      /* verify VM is associated with job if specified */

      /* NYI */

      DV = NULL;

      if (SwapVMs != FALSE)
        {
        /* locate 'swap' target - idle vm on destination node to swap with active vm */

        /* search for idle VM amongst remaining VM's on dst host */

        for (;dvptr != NULL;dvptr = dvptr->Next)
          {
          DV = (mvm_t *)dvptr->Ptr;

          if ((DV == NULL) || (DV->ARes.Procs != DV->CRes.Procs))
            {
            /* VM on dst host is active - continue search */

            continue;
            }

          break;
          }
        }    /* END if (SwapVMs != FALSE) */

      if (SV == NULL)
        {
        /* no valid source located */

        continue;
        }

      if ((DV == NULL) && (SwapVMs == TRUE))
        {
        /* no valid swap target located */

        continue;
        }

      /* valid VM's located - register operation */

      if (SV != NULL)
        {
        VMList[tindex] = SV->VMID;
        MNLSetNodeAtIndex(&NL,tindex,DstN[nindex]);
        MNLSetTCAtIndex(&NL,tindex,1);

        tindex++;
        }

      if (DV != NULL)
        {
        VMList[tindex] = DV->VMID;
        MNLSetNodeAtIndex(&NL,tindex,SrcN[nindex]);
        MNLSetTCAtIndex(&NL,tindex,1);

        tindex++;
        }  /* END if (DV != NULL) */
      }    /* END for (svptr) */
    }      /* END for (tindex) */

  /* terminate list */

  VMList[tindex] = NULL;

  MNLTerminateAtIndex(&NL,tindex);

  /* NOTE:  job taskmap info updated in MRMJobMigrate() below */

  rc = MRMJobMigrate(
      NULL,
      VMList,
      &NL,
      EMsg,  /* O */
      &tmpSC);

  MUFree((char **)&VMList);

  MNLFree(&NL);

  if (rc == FAILURE)
    {
    snprintf(EMsg,sizeof(EMsg),"cannot migrate VM - %s",
      EMsg);

    MUISAddData(S,EMsg);

    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END __MUIJobMigrate() */


/**
 * Process job preempt request.
 *
 * @see MUIJobCtl() - parent
 *
 * NOTE:  authorization must be handled at higher layers
 *
 * @param J       (I)
 * @param Args    (I) [optional]
 * @param U       (I)
 * @param PPolicy (I)
 * @param IsAdmin (I)
 * @param Message (I) specified action annotation
 * @param Msg     (O) [required,maxsize=MMAX_LINE]
 */

int __MUIJobPreempt(

  mjob_t                  *J,
  char                    *Args,
  mgcred_t                *U,
  enum MPreemptPolicyEnum  PPolicy,
  mbool_t                  IsAdmin,
  char                    *Message,
  char                    *Msg)

  {
  int rc;
  int SC = mscNoError;
  char tmpName[MMAX_NAME];
  char tmpLine[MMAX_NAME];

  const char *FName = "__MUIJobPreempt";

  MDB(2,fUI) MLog("%s(%s,%s,%s,%d,%s,Msg)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (Args != NULL) ? Args : "NULL",
    (U->Name != NULL) ? U->Name : "NULL",
    PPolicy,
    (Message != NULL) ? Message : "NULL");

  if (Msg == NULL)
    {
    return(FAILURE);
    }

  Msg[0] = '\0';

  if (J == NULL)
    {
    sprintf(Msg,"ERROR:    command failed\n");

    return(FAILURE);
    }

  /* NOTE: MJobPreempt could destroy J! */

  MUStrCpy(tmpName,J->Name,sizeof(tmpName));

  if ((Message != NULL) && (Message[0] != '\0'))
    rc = MJobPreempt(J,NULL,NULL,NULL,Message,PPolicy,TRUE,Args,Msg,&SC);
  else
    rc = MJobPreempt(J,NULL,NULL,NULL,"admin request",PPolicy,TRUE,Args,Msg,&SC);

  /* create an allocation charge for the job if the auth request comes from
     the job's owner 
     
     NOTE: This is to prevent users from requeueing their own jobs to run
     multiple times without getting charged... */

  /* If we get an SC of mscPartial then the preemption policy was changed
   * and Msg contains a message describing the change. */ 

  if ((SC == mscPartial) && (Msg[0] != '\0'))
    {
    /* Note that the job name should already be in the Msg from MJobPreempt */

    sprintf(tmpLine,"job %s preempted\n",
      (rc == SUCCESS) ? "successfully" : "cannot be");

    MUStrCat(Msg,tmpLine,MMAX_LINE);
    }

  if (Msg[0] == '\0')
    {
    sprintf(Msg,"job %s %s preempted\n",
      tmpName,
      (rc == SUCCESS) ? "successfully" : "cannot be");
    }

  return(rc);
  }  /* END __MUIJobPreempt() */





/**
 *
 *
 * @param J (I)
 * @param Args
 * @param Msg (O) [minsize=MMAX_LINE]
 */

int __MUIJobResume(

  mjob_t *J,    /* I */
  char   *Args,
  char   *Msg)  /* O (minsize=MMAX_LINE) */

  {
  mjob_t *Q[2];   /* job list */
  int RJ = 0; /* number of resumed jobs - we always expect this to be 1 or 0 */
  char EMsg[MMAX_LINE] = {0};
  char *Errors[] = {EMsg};

  if (Msg == NULL)
    {
    return(FAILURE);
    }

  Msg[0] = '\0';

  if (J == NULL)
    {
    sprintf(Msg,"ERROR:    command failed\n");

    return(FAILURE);
    }

  if (J->State == mjsRunning)
    {
    sprintf(Msg, "cannot resume active job %s \n", J->Name);
    return(FAILURE);
    }

  Q[0] = J;
  Q[1] = NULL;       /* terminate the job index list with -1 */

  MQueueScheduleSJobs(Q,&RJ,Errors);

  sprintf(Msg,"job %s %s resumed %s\n",
    J->Name,
    (RJ > 0) ? "successfully" : "cannot be",
    EMsg);
 
  return((RJ > 0) ? SUCCESS : FAILURE);
  }  /* END __MUIJobResume() */





/**
 * Report detailed job information.  Handles the 'checkjob' client request.
 *
 * @see MCCheckJob() - client-side reporting of job info
 * @see __MUIJobToXML() - child - report XML state
 * @see MJobShowTXAttrs() - child 
 * @see MJobShowSystemAttrs() - child
 * @see MJobDiagnoseState() - child
 *
 * @param S      (O)
 * @param CFlags (I)
 * @param Auth   (I)
 */

int MUICheckJobWrapper(

  msocket_t *S,
  mbitmap_t *CFlags,
  char      *Auth)

  {
  job_iter JTI;

  mbitmap_t      Flags;  /* bitmap of enum mcm* */

  enum MPolicyTypeEnum PLevel;

  mjob_t   *J;

  marray_t JobNames;
  marray_t Jobs;
  char   **JNameList;

  mjob_t **JList = NULL;

  int      jindex;

  char     QOSList[MMAX_NAME];
  char     RsvList[MMAX_LINE];
  char     tmpVal[MMAX_NAME];
  char     tmpName[MMAX_NAME];
  char     NodeName[MMAX_LINE];
  char     tmpBuffer[MMAX_BUFFER];
  char     Line[MMAX_LINE];

  mgcred_t *U = NULL;

  int      WTok;

  mxml_t *DE;
  mxml_t *JE;
  mxml_t *RE;

  mbitmap_t IgnBM;

  mbool_t ShowAll = FALSE;
  mbool_t IsAdmin = FALSE;

  mbool_t JobDisplayed = FALSE;

  const char *FName = "MUICheckJobWrapper";

  MDB(2,fUI) MLog("%s(%s)\n",
    FName,
    Auth);

  /* initialize values */

  RE = S->RDE;

  PLevel = mptHard;

  MUArrayListCreate(&JobNames,sizeof(char *),10);
  MUArrayListCreate(&Jobs,sizeof(mjob_t *),10);

  RsvList[0]  = '\0';
  QOSList[0]  = '\0';
  NodeName[0] = '\0';

  WTok = -1;

  while (MS3GetWhere(
        RE,
        NULL,
        &WTok,
        tmpName,
        sizeof(tmpName),
        tmpVal,
        sizeof(tmpVal)) == SUCCESS)
    {
    if (!strcmp(tmpName,MJobAttr[mjaJobID]))
      {
      MUArrayListAppendPtr(&JobNames,strdup(tmpVal));
      }
    else if (!strcmp(tmpName,MJobAttr[mjaEnv]))
      {
      /* NOTE:  hack, use env to route policy level */

      PLevel = (enum MPolicyTypeEnum)MUGetIndexCI(tmpVal,MPolicyMode,FALSE,mptSoft);
      }
    else if (!strcmp(tmpName,MJobAttr[mjaRsvAccess]))
      {
      MUStrCpy(RsvList,tmpVal,sizeof(RsvList));

      if (strcmp(RsvList,NONE) && (RsvList[0] != '\0'))
        {
        char *ptr;
        char *TokPtr;
        char *RName;

        ptr = MUStrTok(RsvList,",",&TokPtr);

        while (ptr != NULL)
          {
          RName = ptr;

          ptr = MUStrTok(NULL,",",&TokPtr);

          if (MRsvFind(RName,NULL,mraNONE) == FAILURE)
            {
            char tmpLine[MMAX_LINE];

            snprintf(tmpLine,sizeof(tmpLine),"ERROR:  invalid reservation specified: %s\n",
              RName);

            MUISAddData(S,tmpLine);

            MUArrayListFreePtrElements(&JobNames);
            MUArrayListFree(&JobNames);
            MUArrayListFree(&Jobs);

            return(FAILURE);
            }
          }
        } /* END if (strcmp(RsvList,NONE) && (RsvList[0] != '\0')) */

      MUStrCpy(RsvList,tmpVal,sizeof(RsvList));
      }
    else if (!strcmp(tmpName,MJobAttr[mjaQOSReq]))
      {
      MUStrCpy(QOSList,tmpVal,sizeof(QOSList));

      if ((QOSList[0] != '\0') && (strcmp(QOSList,NONE)))
        {
        char *ptr;
        char *TokPtr;
        char *QName;

        ptr = MUStrTok(QOSList,",",&TokPtr);

        while (ptr != NULL)
          {
          QName = ptr;

          ptr = MUStrTok(NULL,",",&TokPtr);

          if (MQOSFind(QName,NULL) == FAILURE)
            {
            char tmpLine[MMAX_LINE];

            snprintf(tmpLine,sizeof(tmpLine),"ERROR:  invalid qos specified: %s\n",
              QName);

            MUISAddData(S,tmpLine);

            MUArrayListFreePtrElements(&JobNames);
            MUArrayListFree(&JobNames);
            MUArrayListFree(&Jobs);

            return(FAILURE);
            }
          }
        }     /* END if ((QOSList[0] != '\0') && (strcmp(QOSList,NONE))) */

      MUStrCpy(QOSList,tmpVal,sizeof(QOSList));
      }
    else if (!strcmp(tmpName,MJobAttr[mjaReqHostList]))
      {
      if (MNodeFind(tmpVal,NULL) == FAILURE)
        {
        char tmpLine[MMAX_LINE];

        snprintf(tmpLine,sizeof(tmpLine),"ERROR:  invalid node specified: %s\n",
          tmpVal);

        MUISAddData(S,tmpLine);

        MUArrayListFreePtrElements(&JobNames);
        MUArrayListFree(&JobNames);
        MUArrayListFree(&Jobs);

        return(FAILURE);
        }

      MUStrCpy(NodeName,tmpVal,sizeof(NodeName));
      }
    else if (!strcmp(tmpName,"ignlist"))
      {
      if (bmfromstring(tmpVal,MAllocRejType,&IgnBM," \t\n,:|") == FAILURE)
        {
        char tmpLine[MMAX_LINE];

        /* NOTE:  pass array[1] to skip '[NONE]' entry */

        snprintf(tmpLine,sizeof(tmpLine),"ERROR:  cannot process ignlist, use one or more of the following: '%s'\n",
          MUArrayToString(&MAllocRejType[1],",",tmpBuffer,sizeof(tmpBuffer)));

        MUISAddData(S,tmpLine);

        MUArrayListFreePtrElements(&JobNames);
        MUArrayListFree(&JobNames);
        MUArrayListFree(&Jobs);
        return(FAILURE);
        }
      } 
    }    /* END while (MS3GetWhere() == SUCCESS) */

  if (JobNames.NumItems <= 0)
    {
    MUISAddData(S,"ERROR:  no jobs specified\n");

    MUArrayListFreePtrElements(&JobNames);
    MUArrayListFree(&JobNames);
    MUArrayListFree(&Jobs);
    return(FAILURE);
    }

  JNameList = (char **)JobNames.Array;

  if (MXMLGetAttr(RE,MSAN[msanFlags],NULL,tmpVal,sizeof(tmpVal)) == SUCCESS)
    {
    bmfromstring(tmpVal,MClientMode,&Flags);
    }

  if ((JobNames.NumItems == 1) && !strcasecmp(JNameList[0],"ALL"))
    {
    ShowAll = TRUE;
    }
  else 
    {
    int rc;

    for (jindex = 0;jindex < JobNames.NumItems;jindex++)
      {
      char *JobName = JNameList[jindex];
      /* Allow format 'template:<JOBID>' or '<JOBID>' */

      if (!strncasecmp(JobName,"template:",strlen("template:")))
        {
        rc = MTJobFind(JobName + strlen("template:"),&J);
        }
      else if (!strncasecmp(JobName,"jobname:",strlen("jobname:")))
        {
        J = NULL;

        if (MJobFind(JobName + strlen("jobname:"),&J,mjsmJobName) == SUCCESS)
          {
          rc = SUCCESS;
          }
        else
          {
          rc = FAILURE;
          }
        }
      else
        {
        J = NULL;

        if ((MJobFind(JobName,&J,mjsmExtended) == FAILURE) &&
            (MJobCFind(JobName,&J,mjsmBasic) == FAILURE) &&
           ((MTJobFind(JobName,&J) == FAILURE)))
          {
          rc = FAILURE;
          }
        else
          {
          rc = SUCCESS;
          }
        }

      if (rc == FAILURE)
        {
        char tmpLine[MMAX_LINE];

        snprintf(tmpLine,sizeof(tmpLine),"ERROR:  invalid job specified: %s\n",
          JobName);

        MUISAddData(S,tmpLine);

        MDB(3,fUI) MLog("INFO:     cannot locate job '%s' in %s()\n",
          JobName,
          FName);

        MUArrayListFreePtrElements(&JobNames);
        MUArrayListFree(&JobNames);
        MUArrayListFree(&Jobs);
        return(FAILURE);
        }

      MUArrayListAppendPtr(&Jobs,J);
      }

    JList = (mjob_t **)Jobs.Array;

    jindex = 0;

    J = JList[0];
    }  /* END else (!strcasecmp(JobName,"ALL")) */

  if ((bmisset(&Flags,mcmXML)) && (S->SDE == NULL))
    {
    if (MXMLCreateE(&S->SDE,(char *)MSON[msonData]) == FAILURE)
      {
      MUISAddData(S,"ERROR:    cannot create response\n");

      MUArrayListFree(&Jobs);
      return(FAILURE);
      }
    }

  DE = S->SDE;

  mstring_t Buffer(MMAX_LINE);


  MJobIterInit(&JTI);

  while (TRUE)
    {
    if (ShowAll == TRUE)
      {
      /* NOTE:  do not display all completed jobs */

      while (MJobTableIterate(&JTI,&J) == SUCCESS)
        {
        if ((bmisset(&J->Flags, mjfArrayJob)) &&
            !(bmisset(&J->Flags,mjfArrayMaster)))
          continue;

        /* immediately break out */

        break;
        }

      if (J == NULL)
        {
        /* END of job list located */

        break;
        }
      }    /* END if (ShowAll == TRUE) */
    else if (Jobs.NumItems > 1)
      {
      if (jindex >= Jobs.NumItems)
        {
        /* end of list reached */

        break;
        }

      /* locate next job */

      J = JList[jindex++];
      }

    /* security check (allow A1-3 or job owner) */

    /* currently only users call checkjob, no peers */

    if (((U == NULL) && (MUserAdd(Auth,&U) == FAILURE)) ||
        (MUICheckAuthorization(
           U,
           NULL,
           (void *)J,
           mxoJob,
           mcsCheckJob,
           mjcmQuery,
           &IsAdmin,                  /* O */
           Line,                      /* O - not used */
           sizeof(Line)) == FAILURE))
      {
      if ((J->Credential.U != NULL) && (strcmp(J->Credential.U->Name,Auth) != 0))
        {
        /* account manager check */

        if (((J->Credential.A != NULL) && (J->Credential.A->F.ManagerU != NULL)) &&
            (MCredFindManager(Auth,J->Credential.A->F.ManagerU,NULL) == SUCCESS))
          {
          /* NO-OP */

          /* user is account manager */
          }
        else if (((J->Credential.C != NULL) && (J->Credential.C->F.ManagerU != NULL)) &&
            (MCredFindManager(Auth,J->Credential.C->F.ManagerU,NULL) == SUCCESS))
          {
          /* NO-OP */

          /* user is class/queue manager */
          }
        else
          {
          char tmpLine[MMAX_LINE];

          if ((ShowAll == TRUE) || (Jobs.NumItems > 1))
            continue;

          MDB(2,fUI) MLog("INFO:     user %s is not authorized to check status of job '%s'\n",
              Auth,
              J->Name);

          snprintf(tmpLine,sizeof(tmpLine),"user %s is not authorized to check status of job '%s'\n",
              Auth,
              J->Name);

          MUISAddData(S,tmpLine);

          MUArrayListFreePtrElements(&JobNames);
          MUArrayListFree(&JobNames);
          MUArrayListFree(&Jobs);
          return(FAILURE);
          }
        }    /* END if (strcmp(J->Cred.U->Name,Auth) != 0) */
      }      /* END if ((MUserAdd() == FAILURE) || MUICheckAuthorization() == FAILURE) */
    else if ((ShowAll == TRUE) || (Jobs.NumItems > 1))
      {
      /* NOTE:  MUICheckAuthorization() allows all users to query all jobs - 
         see MAP comment in routine */

      if ((IsAdmin == FALSE) && (J->Credential.U != U))
        {
        continue;
        }
      }

    /* __MUIJobToXML() handles the --format=xml output */

    if (bmisset(&Flags,mcmXML))
      {
      __MUIJobToXML(J,&Flags,&JE);

      MXMLAddE(DE,JE);

      if ((ShowAll == FALSE) && (Jobs.NumItems <= 1))
        {
        MUArrayListFreePtrElements(&JobNames);
        MUArrayListFree(&JobNames);
        MUArrayListFree(&Jobs);

        MUISAddData(S,Buffer.c_str());

        return(SUCCESS);
        }

      continue;
      }

    /* display job state */

    if (JobDisplayed == TRUE)
      {
      /* at least one job displayed */

      MStringAppendF(&Buffer,"\n\n============================\n\n");
      }
    else
      {
      JobDisplayed = TRUE;
      }

    MUICheckJob(J,&Buffer,PLevel,&IgnBM,RsvList,QOSList,NodeName,&Flags);

    if (bmisset(&J->Flags,mjfArrayMaster))
      {
      if (ShowAll == TRUE)
        {
        continue;
        }
      else
        {
        MUISAddData(S,Buffer.c_str());
   
        MUArrayListFree(&Jobs);
        return(SUCCESS);
        }
      }

    if ((ShowAll == FALSE) && (Jobs.NumItems <= 1))
      break;
    }  /* END while (TRUE) */

  if ((bmisset(&Flags,mcmXML)) && ((ShowAll == TRUE) || (Jobs.NumItems > 1)))
    {
    /* NO-OP */
    }

  MUArrayListFreePtrElements(&JobNames);
  MUArrayListFree(&JobNames);
  MUArrayListFree(&Jobs);

  MUISAddData(S,Buffer.c_str());

  return(SUCCESS);
  }  /* END MUICheckJobWrapper() */



/**
 * Translate job to XML for 'mdiag -j', 'mjobctl -q', etc.
 *
 * @see MJobBaseToXML()
 * @see MJobToXML() - child
 * @see MJobDiagnoseEligibility() - child
 *
 * @param J (I)
 * @param CFlags (I) bitmap of enum MCModeEnum
 * @param JEP (O) [alloc]
 */

int __MUIJobToXML(

  mjob_t     *J,
  mbitmap_t  *CFlags,
  mxml_t    **JEP)

  {
  mxml_t *BE;

  /* see JVerboseList below to list of 'verbose' job attributes */

  const enum MJobAttrEnum JList[] = {
    mjaAccount,
    mjaAllocVMList,
    mjaAWDuration,
    mjaBecameEligible,
    mjaBlockReason,
    mjaBypass,
    mjaClass,
    mjaCmdFile,
    mjaCompletionCode,
    mjaCompletionTime,
    mjaDescription,
    mjaEEWDuration,   /* eligible job time */
    mjaEFile,
    mjaEffPAL,
    mjaEState,
    mjaFlags,
    mjaGAttr,
    mjaGJID,
    mjaGroup,
    mjaHold,
    mjaReqHostList,
    mjaIWD,
    mjaJobID,
    mjaJobName,
    mjaMessages,
    mjaMinWCLimit,
    mjaOFile,
    mjaPAL,
    mjaParentVCs,
    mjaStartPriority,
    mjaQueueStatus,
    mjaQOS,
    mjaQOSReq,
    mjaReqAWDuration,
    mjaReqCMaxTime,
    mjaReqProcs,
    mjaReqNodes,
    mjaReqReservation,
    mjaReqSMinTime,
    mjaRM,
    mjaRMError,
    mjaRMOutput,
    mjaSRMJID,
    mjaDRMJID,
    mjaPrologueScript,
    mjaEpilogueScript,
    mjaServiceJob,
    mjaSessionID,
    mjaStartCount,
    mjaStartPriority,
    mjaStartTime,
    mjaState,
    mjaStatMSUtl,
    mjaStatPSDed,
    mjaStatPSUtl,
    mjaStdErr,
    mjaStdOut,
    mjaStorage,
    mjaSubmitTime,
    mjaSuspendDuration,
    mjaSysPrio,
    mjaSysSMinTime,
    mjaTemplateList,  /* NOTE:  remove attribute once verbose flag routed via 'mjobctl -q' */
    mjaTemplateSetList,
    mjaTemplateFlags,
    mjaTTC,
    mjaUMask,
    mjaUser,
    mjaUserPrio,
    mjaVariables,
    mjaVM,
    mjaVMUsagePolicy,
    mjaNONE };

  const enum MReqAttrEnum RQList[] = {
    mrqaAllocNodeList,
    mrqaAllocPartition,
    mrqaExclNodeFeature,
    mrqaNCReqMin,
    mrqaNodeAccess,
    mrqaReqArch,          /* architecture */
    mrqaReqAttr,
    mrqaReqDiskPerTask,
    mrqaReqMemPerTask,
    mrqaReqNodeDisk,
    mrqaReqNodeFeature,
    mrqaReqNodeMem,
    mrqaReqNodeProc,
    mrqaReqNodeSwap,
    mrqaReqOpsys,
    mrqaReqPartition,
    mrqaReqProcPerTask,
    mrqaReqSwapPerTask,
    mrqaTCReqMin,
    mrqaTPN,
    mrqaGRes,
    mrqaCGRes,
    mrqaNONE };

  /* the following arrays are used when the "verbose" flag is specified */

  const enum MJobAttrEnum JVerboseList[] = {
    mjaAccount,
    mjaAllocVMList,
    mjaAWDuration,
    mjaBecameEligible,
    mjaBlockReason,
    mjaBypass,
    mjaClass,
    mjaCmdFile,
    mjaCompletionCode,
    mjaCompletionTime,
    mjaCPULimit,
    mjaDepend,
    mjaDependBlock,
    mjaDescription,
    mjaEEWDuration,   /* eligible job time */
    mjaEffPAL,
    mjaEFile,
    mjaEligibilityNotes,
    mjaEState,
    mjaFlags,
    mjaGAttr,
    mjaGJID,
    mjaGroup,
    mjaHold,
    mjaIWD,
    mjaJobID,
    mjaJobName,
    mjaMessages,
    mjaNodeSelection,
    mjaNotification,
    mjaNotificationAddress,
    mjaOFile,
    mjaPAL,
    mjaParentVCs,
    mjaStartPriority,
    mjaQueueStatus,
    mjaQOS,
    mjaQOSReq,
    mjaReqAWDuration,
    mjaReqCMaxTime,
    mjaReqHostList,
    mjaReqProcs,
    mjaReqNodes,
    mjaReqReservation,
    mjaReqSMinTime,
    mjaRM,
    mjaRMError,
    mjaRMOutput,
    mjaSRMJID,
    mjaDRMJID,
    mjaPrologueScript,
    mjaEpilogueScript,
    mjaSessionID,
    mjaShell,
    mjaStartCount,
    mjaStartPriority,
    mjaStartTime,
    mjaState,
    mjaStatMSUtl,
    mjaStatPSDed,
    mjaStatPSUtl,
    mjaSubmitHost,  /* for LSF compat command job query */
    mjaStdErr,
    mjaStdOut,
    mjaSubmitArgs,
    mjaSubmitTime,
    mjaSubState,
    mjaSuspendDuration,
    mjaSysPrio,
    mjaSysSMinTime,
    mjaTemplateList,
    mjaTemplateSetList,
    mjaTemplateFlags,
    mjaUMask,
    mjaUser,
    mjaUserPrio,
    mjaVariables,
    mjaVMUsagePolicy,
    mjaNONE };

  const enum MReqAttrEnum RQVerboseList[] = {
    mrqaAllocNodeList,
    mrqaAllocPartition,
    mrqaNCReqMin,
    mrqaNodeAccess,
    mrqaReqArch,          /* architecture */
    mrqaReqAttr,
    mrqaReqDiskPerTask,
    mrqaReqMemPerTask,
    mrqaReqNodeDisk,
    mrqaReqNodeFeature,
    mrqaExclNodeFeature,
    mrqaReqNodeMem,
    mrqaReqNodeProc,
    mrqaReqNodeSwap,
    mrqaReqOpsys,
    mrqaReqPartition,
    mrqaReqProcPerTask,
    mrqaReqSwapPerTask,
    mrqaTCReqMin,
    mrqaTPN,
    mrqaGRes,
    mrqaCGRes,
    mrqaUtilMem,
    mrqaUtilProc,
    mrqaUtilSwap,
    mrqaNONE };

  if ((J == NULL) || (JEP == NULL))
    {
    return(FAILURE);
    }

  *JEP = NULL;

  if (!bmisset(CFlags,mcmVerbose))
    {
    MJobToXML(
      J,
      JEP,
      (enum MJobAttrEnum *)JList,
      (enum MReqAttrEnum *)RQList,
      NULL,
      FALSE,
      TRUE);
    }
   else
    {
    MJobToXML(
      J,
      JEP,
      (enum MJobAttrEnum *)JVerboseList,
      (enum MReqAttrEnum *)RQVerboseList,
      NULL,
      FALSE,
      TRUE);
    }

  if (bmisset(CFlags,mcmVerbose2))
    {
    BE = NULL;

    MXMLCreateE(&BE,"ELIGIBILITY");

    MJobDiagnoseEligibility(
      J,
      *JEP,
      NULL,
      mptSoft,
      mfmXML,
      CFlags,
      NULL,
      (char *)BE,
      0,
      TRUE);

    MXMLAddE(*JEP,BE);
    }

  return(SUCCESS);
  }  /* END __MUIJobToXML() */




/**
 * Translate job to XML for 'mdiag -j'
 *
 * @see MJobTransitionBaseToXML()
 * @see MJobTransitionToXML() - child
 *
 * @param J       (I) the job we're translating to XML
 * @param CFlags  (I) bitmap of enum MCModeEnum (for command args/flags)
 * @param JEP     (O) the xml element we're using [alloc]
 */

int __MUIJobTransitionToXML(

  mtransjob_t  *J,
  mbitmap_t    *CFlags,
  mxml_t      **JEP)

  {
  /* "normal" list of job/request attributes */

  const enum MJobAttrEnum JList[] = {
    mjaAccount,
    mjaAllocVMList,
    mjaAWDuration,
    mjaBecameEligible,
    mjaBlockReason,
    mjaBypass,
    mjaClass,
    mjaCmdFile,
    mjaCompletionCode,
    mjaCompletionTime,
    mjaDescription,
    mjaEEWDuration,   /* eligible job time */
    mjaEFile,
    mjaEffPAL,
    mjaEState,
    mjaFlags,
    mjaGAttr,
    mjaGJID,
    mjaGroup,
    mjaHold,
    mjaReqHostList,
    mjaIWD,
    mjaJobID,
    mjaJobName,
    mjaMessages,
    mjaMinWCLimit,
    mjaOFile,
    mjaPAL,
    mjaParentVCs,
    mjaQOS,
    mjaQOSReq,
    mjaQueueStatus,
    mjaReqAWDuration,
    mjaReqReservation,
    mjaRM,
    mjaRMError,
    mjaRMOutput,
    mjaSessionID,
    mjaSRMJID,
    mjaDRMJID,
    mjaStartCount,
    mjaStartPriority,
    mjaStartTime,
    mjaState,
    mjaStatMSUtl,
    mjaStatPSDed,
    mjaStatPSUtl,
    mjaStdErr,
    mjaStdOut,
    mjaSubmitTime,
    mjaSuspendDuration,
    mjaSysPrio,
    mjaSysSMinTime,
    mjaTemplateSetList,
    mjaUMask,
    mjaUser,
    mjaUserPrio,
    mjaUtlMem,
    mjaVariables,
    mjaNONE };

  const enum MReqAttrEnum RQList[] = {
    mrqaAllocNodeList,
    mrqaAllocPartition,
    mrqaExclNodeFeature,
    mrqaNCReqMin,
    mrqaNodeAccess,
    mrqaReqArch,          /* architecture */
    mrqaReqAttr,
    mrqaReqDiskPerTask,
    mrqaReqMemPerTask,
    mrqaReqNodeDisk,
    mrqaReqNodeFeature,
    mrqaReqNodeMem,
    mrqaReqNodeProc,
    mrqaReqNodeSwap,
    mrqaReqOpsys,
    mrqaReqPartition,
    mrqaReqProcPerTask,
    mrqaReqSwapPerTask,
    mrqaTCReqMin,
    mrqaTPN,
    mrqaGRes,
    mrqaCGRes,
    mrqaUtilSwap,
    mrqaNONE };

  /* "verbose" list of job/request attributes */

  const enum MJobAttrEnum JVerboseList[] = {
    mjaAccount,
    mjaAllocVMList,
    mjaAWDuration,
    mjaBecameEligible,
    mjaBlockReason,
    mjaBypass,
    mjaClass,
    mjaCmdFile,
    mjaCompletionCode,
    mjaCompletionTime,
    mjaCPULimit,
    mjaDepend,
    mjaDependBlock,
    mjaDescription,
    mjaEEWDuration,   /* eligible job time */
    mjaEffPAL,
    mjaEFile,
    mjaEligibilityNotes,
    mjaEState,
    mjaFlags,
    mjaGAttr,
    mjaGJID,
    mjaGroup,
    mjaHold,
    mjaIWD,
    mjaJobID,
    mjaJobName,
    mjaMessages,
    mjaMinWCLimit,
    mjaNodeSelection,
    mjaNotification,
    mjaNotificationAddress,
    mjaOFile,
    mjaPAL,
    mjaParentVCs,
    mjaQueueStatus,
    mjaQOS,
    mjaQOSReq,
    mjaReqAWDuration,
    mjaReqCMaxTime,
    mjaReqHostList,
    mjaReqReservation,
    mjaReqSMinTime,
    mjaRM,
    mjaRMError,
    mjaRMOutput,
    mjaSessionID,
    mjaSRMJID,
    mjaDRMJID,
    mjaPrologueScript,
    mjaEpilogueScript,
    mjaShell,
    mjaStartCount,
    mjaStartPriority,
    mjaStartTime,
    mjaState,
    mjaStatMSUtl,
    mjaStatPSDed,
    mjaStatPSUtl,
    mjaSubmitHost,  /* for LSF compat command job query */
    mjaStdErr,
    mjaStdOut,
    mjaSubmitTime,
    mjaSubState,
    mjaSuspendDuration,
    mjaSysPrio,
    mjaSysSMinTime,
    mjaTemplateList,
    mjaTemplateSetList,
    mjaTemplateFlags,
    mjaUMask,
    mjaUser,
    mjaUserPrio,
    mjaUtlMem,
    mjaVariables,
    mjaVMUsagePolicy,
    mjaNONE };

  const enum MReqAttrEnum RQVerboseList[] = {
    mrqaAllocNodeList,
    mrqaAllocPartition,
    mrqaExclNodeFeature,
    mrqaNCReqMin,
    mrqaNodeAccess,
    mrqaReqArch,          /* architecture */
    mrqaReqAttr,
    mrqaReqDiskPerTask,
    mrqaReqMemPerTask,
    mrqaReqNodeDisk,
    mrqaReqNodeFeature,
    mrqaReqNodeMem,
    mrqaReqNodeProc,
    mrqaReqNodeSwap,
    mrqaReqOpsys,
    mrqaReqPartition,
    mrqaReqProcPerTask,
    mrqaReqSwapPerTask,
    mrqaTCReqMin,
    mrqaTPN,
    mrqaGRes,
    mrqaCGRes,
    mrqaNONE };

  if ((J == NULL) || (JEP == NULL))
    {
    return(FAILURE);
    }

  *JEP = NULL;

  if (!bmisset(CFlags,mcmVerbose))
    {
    MJobTransitionToXML(
      J,
      JEP,
      (enum MJobAttrEnum *)JList,
      (enum MReqAttrEnum *)RQList,
      mfmNONE);
    }
  else
    {
    MJobTransitionToXML(
      J,
      JEP,
      (enum MJobAttrEnum *)JVerboseList,
      (enum MReqAttrEnum *)RQVerboseList,
      mfmNONE);
    }

  return(SUCCESS);
  } /* END __MUIJobTransitionToXML() */






/**
 * Handle 'mjobctl -R' command.
 *
 * @see MUIJobCtl() - parent
 *
 * @param JobExp (I) [format: <JOBID>|???]
 * @param Auth   (I)
 * @param S      (I/O)
 * @param RE     (I) request
 */

int __MUIJobRerun(

  char             *JobExp,
  char             *Auth,
  msocket_t        *S,
  mxml_t           *RE)

  {
  char tmpLine[MMAX_LINE];
  char EMsg[MMAX_LINE] = {0};

  mjob_t *CJ = NULL;
  int SC = 0;

  if (MJobCFind(JobExp,&CJ,mjsmBasic) == FAILURE)
    {
    snprintf(tmpLine,sizeof(tmpLine),"ERROR:    could not find completed job '%s'\n",
      JobExp);

    MUISAddData(S,tmpLine);

    return(FAILURE);
    }

  if (!bmisset(&CJ->Flags,mjfRestartable))
    {
    snprintf(tmpLine,sizeof(tmpLine),"ERROR:    job does not have restartable flag\n");
   
    MUISAddData(S,tmpLine);

    return(FAILURE);
    }
  else if (bmisset(&CJ->IFlags,mjifIsArrayJob) ||
           bmisset(&CJ->Flags,mjfArrayMaster))
    {
    snprintf(tmpLine,sizeof(tmpLine),"ERROR:    cannot rerun job arrays\n");
   
    MUISAddData(S,tmpLine);

    return(FAILURE);
    }

  if ((CJ->Req[0]->RMIndex >= 0) &&
      (MRM[CJ->Req[0]->RMIndex].Type == mrmtPBS))
    {
    if (MJobRequeue(CJ,NULL,"manual requeue",EMsg,&SC) == FAILURE)
      {
      snprintf(tmpLine,sizeof(tmpLine),"ERROR:    could not requeue completed job '%s', rm failure '%s'\n",
        JobExp,
        EMsg);
   
      MUISAddData(S,tmpLine);
   
      return(FAILURE);
      }
    }
  else
    {
    mjob_t *NewJ = NULL;

    mrm_t  *R = MSched.InternalRM;

    int rqindex;

    if ((MJobCreate(CJ->Name,TRUE,&NewJ,NULL) == FAILURE) ||
        (MJobDuplicate(NewJ,CJ,FALSE) == FAILURE))
      {
      snprintf(tmpLine,sizeof(tmpLine),"ERROR:    could not requeue completed job '%s', internal failure\n",
        JobExp);
   
      MUISAddData(S,tmpLine);
   
      return(FAILURE);
      }
   
    MJobToIdle(NewJ,TRUE,TRUE);

    MUFree((char **)&NewJ->TaskMap);
    NewJ->TaskMapSize = 0;
   
    NewJ->SubmitRM = R;
   
    for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
      {
      if (NewJ->Req[rqindex] == NULL)
        break;
   
      NewJ->Req[rqindex]->RMIndex = R->Index;
      }  /* END for (rqindex) */
    }

  snprintf(tmpLine,sizeof(tmpLine),"requeued completed job '%s'\n",
    JobExp);
   
  MUISAddData(S,tmpLine);
 
  return(SUCCESS);
  }  /* END __MUIJobRerun() */






/**
 * Get Earliest job start time.
 *
 * @see MUIJobQuery() - parent
 * @see __MUIJobGetEStartTime() - child
 *
 * NOTE:  Handle 'showstart' command.
 *
 * @param JSpec     (I) [jobid|jobdesc]
 * @param SP        (I/O) [O-optional-best partition] [I-optional-required partition]
 * @param C         (I) [optional]
 * @param StartTime (O) [optional]
 * @param JEP       (O) [optional]
 * @param EMsg      (O) [optional]
 * @param EMsgSize  (I)
 * @param S         (I) [optional]
 * @param Auth      (I)
 */

int __MUIJobGetStart(
 
  char      *JSpec,
  mpar_t   **SP,
  mclass_t  *C,
  mulong    *StartTime,
  mxml_t   **JEP,
  char      *EMsg,
  int        EMsgSize,
  msocket_t *S,
  char      *Auth)

  {
  long   Start;
  long   DeadLine;
 
  mpar_t *P;
  mpar_t *BP;
 
  int    NodeCount;
  int    TaskCount;

  mjob_t *OrigJP = NULL;

  char   DString[MMAX_LINE];
 
  mjob_t *J;
 
  mxml_t *JE;

  mrsv_t *R = NULL;

  mgcred_t *U;

  mbool_t   IsAdmin;

  char     TString[MMAX_LINE];

  const char *FName = "__MUIJobGetStart";
 
  MDB(2,fUI) MLog("%s(%s,SP,%s,StartTime,JEP,EMsg,%d,S,%s)\n",
    FName,
    (JSpec != NULL) ? JSpec : "NULL",
    (C != NULL) ? C->Name : "NULL",
    EMsgSize,
    (Auth != NULL) ? Auth : "NULL");
                  
  if (JEP != NULL)
    *JEP = NULL;

  if (StartTime != NULL)
    *StartTime = 0;

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (JSpec == NULL)
    {
    return(FAILURE);
    }

  if ((SP != NULL) && (*SP != NULL))
    P = *SP;
  else 
    P = &MPar[0]; 

  MJobMakeTemp(&J);

  if (MJobFromJSpec(
        JSpec,
        &J,        /* I/O */
        (S != NULL) ? S->RDE : NULL,
        Auth,
        &OrigJP,   /* O */
        EMsg,      /* O */
        EMsgSize,
        NULL) == FAILURE)
    {
    MJobFreeTemp(&J);

    return(FAILURE);
    }

  if ((strchr(JSpec,'@') != NULL) && (strchr(JSpec,'<') == NULL))
    {
    /* This is not a real job (e.g shaowstart 5@30) with specified partition(s) so reset PALs to allow consideration of all partitions */
  
    MJobSetAttr(J,mjaPAL,NULL,mdfInt,mSet);
    }

  MUserFind(Auth,&U);
  
  if (MUICheckAuthorization(
        U,
        NULL,
        (void *)J,
        mxoJob,
        mcsShowEstimatedStartTime,
        mjcmQuery,
        &IsAdmin,
        EMsg,
        EMsgSize) == FAILURE)
    {
    MDB(2,fUI) MLog("%s",
      EMsg);

    if (S != NULL)
      MUISAddData(S,EMsg);

    MJobFreeTemp(&J);

    return(FAILURE);
    }

  Start = MSched.Time;

  if (J->SpecSMinTime > MSched.Time)
    {
    Start = J->SpecSMinTime;
    }

  if (bmisset(&J->Hold,mhDefer))
    {
    Start = MAX(Start,(long)J->HoldTime + MSched.DeferTime);
    }
 
  if ((J->Rsv == NULL) && (OrigJP != NULL))
    {
    /* MJobFromJSpec() does not fill in J->R in the duplicate job
       but it does give us the job index so that we can look up the job reservation */

    R = OrigJP->Rsv;

/*
    J->FSTree = MJob[JIndex]->FSTree;
*/
    }
  else
    {
    R = J->Rsv;
    }

  if (R != NULL)
    {
    MDB(2,fUI) MLog("INFO:     reservation found for job '%s' in showstart (%s; R: %s)\n", 
      J->Name,
      (OrigJP != NULL) ? OrigJP->Name : "NULL",
      ((OrigJP != NULL) && (OrigJP->Rsv != NULL)) ? OrigJP->Rsv->Name : "NULL");

    Start = R->StartTime;
    BP    = &MPar[MAX(0,R->PtIndex)];
    }
  else
    {
    mnl_t *MNodeList[MMAX_REQ_PER_JOB];

    int RetCode;

    mpar_t *tmpP;
  
    MDB(2,fUI) MLog("INFO:     reservation not found for job '%s' in showstart (%s; R: %s)\n", 
      J->Name,
      (OrigJP != NULL) ? OrigJP->Name : "NULL",
      ((OrigJP != NULL) && (OrigJP->Rsv != NULL)) ? OrigJP->Rsv->Name : "NULL");

    tmpP = (P->Index > 0) ? P : NULL;

    /* Since J was not deep copied from the original job it may not have the FSTree information needed by MJobGetEStartTime.
       Temporarily have J point to the original job FSTree info */

    if ((MSched.FSTreeIsRequired == TRUE) && (OrigJP != NULL) && (OrigJP->FairShareTree != NULL))
      J->FairShareTree = OrigJP->FairShareTree;

    MNLMultiInit(MNodeList);

    RetCode = MJobGetEStartTime(
         J,
         &tmpP,
         &NodeCount,
         &TaskCount,
         MNodeList,
         &Start,       /* I/O */
         NULL,
         TRUE,
         FALSE,
         EMsg);

    MNLMultiFree(MNodeList);

    J->FairShareTree = NULL;

    if (RetCode == FAILURE)  /* O */
      {
      MDB(2,fUI) MLog("WARNING:  cannot determine earliest start time for job '%s'\n", 
        J->Name);

      if ((EMsg != NULL) && (EMsg[0] == '\0'))
        { 
        snprintf(EMsg,EMsgSize,"ERROR:    cannot determine earliest start time for job '%s'\n",
          J->Name);
        }

      MSNLFree(&J->Req[0]->CGRes);
      MSNLFree(&J->Req[0]->AGRes);

      MJobFreeTemp(&J);

      return(FAILURE);
      }

    BP = tmpP;
    }  /* END else (J->R != NULL) */

  /* start time located */

  DeadLine = Start + J->SpecWCLimit[0];

  MULToTString(Start - MSched.Time,TString);
  MULToDString((mulong *)&DeadLine,DString);

  MDB(3,fUI) MLog("INFO:     earliest completion located for job %s in %s on partition %s at %s",
    J->Name,
    TString,
    BP->Name,
    DString);

  if (StartTime != NULL)
    *StartTime = Start;

  if (SP != NULL)
    *SP = BP;

  if (JEP != NULL)
    {
    int tmpI;

    /* generate xml response */

    JE = NULL;

    MXMLCreateE(&JE,(char *)MXO[mxoJob]);

    MXMLSetAttr(JE,(char *)MJobAttr[mjaJobID],(void *)J->Name,mdfString);

    MXMLSetAttr(JE,"now",(void *)&MSched.Time,mdfLong);

    tmpI = J->Req[0]->DRes.Procs * J->Request.TC;

    MXMLSetAttr(JE,(char *)MJobAttr[mjaReqProcs],(void *)&tmpI,mdfInt);
    MXMLSetAttr(JE,(char *)MJobAttr[mjaStartTime],(void *)StartTime,mdfLong);
    MXMLSetAttr(JE,(char *)MJobAttr[mjaReqAWDuration],(void *)&J->SpecWCLimit[0],mdfLong);
    MXMLSetAttr(JE,(char *)MJobAttr[mjaPAL],(void *)BP->Name,mdfString);

    *JEP = JE;
    }  /* END if (JEP != NULL) */

  MSNLFree(&J->Req[0]->CGRes);
  MSNLFree(&J->Req[0]->AGRes);

  MJobFreeTemp(&J);
 
  return(SUCCESS);
  }  /* END __MUIJobGetStart() */




/**
 * Determine estimated start time for job matching specified job description.
 * 
 * @see MUIJobQuery() - parent
 * @see __MUIJobGetStart() - child
 *
 * NOTE:  Process 'showstart' command.
 *
 * @param JSpec (I) [format???]
 * @param Auth (I)
 * @param S (I/O) [optional]
 * @param FlagString (I)
 * @param PeerName (I) [optional]
 * @param C (I) [optional]
 * @param PE (O) [optional,alloc]
 */

int __MUIJobGetEStartTime(

  char             *JSpec,      /* I (format???) */
  char             *Auth,       /* I */
  msocket_t        *S,          /* I/O (optional) */
  char             *FlagString, /* I */
  char             *PeerName,   /* I (optional) */
  mclass_t         *C,          /* I (optional) */
  mxml_t          **PE)         /* O (optional,alloc) */
  
  {
  int  rc;
  char EMsg[MMAX_BUFFER] = {0};
  
  mulong StartTime;

  mbool_t  IsVerbose;
  mbool_t  ShowPeer;

  mbool_t  ShowPrio;
  mbool_t  ShowRsv;
  mbool_t  ShowHist;

  mjob_t *tmpJ = NULL;

  mxml_t *JE;
  
  mpar_t *BP;

  const char *FName = "__MUIJobGetEStartTime";

  if ((JSpec == NULL) ||
      (Auth == NULL) ||
      (FlagString == NULL) ||
      (PE == NULL))
    {
    MDB(3,fUI) MLog("ALERT:    invalid parameters passed to %s\n",
      FName);

    return(FAILURE);
    }

  IsVerbose = FALSE;

  ShowPeer = FALSE;
  ShowPrio = FALSE;
  ShowRsv  = FALSE;
  ShowHist = FALSE;

  if (FlagString != NULL)
    {
    if ((strstr(FlagString,"VERBOSE") != NULL) ||
        (strstr(FlagString,"verbose") != NULL))
      {
      IsVerbose = TRUE;
      } 

    if ((strstr(FlagString,"GRID") != NULL) ||
        (strstr(FlagString,"grid") != NULL))
      {
      ShowPeer = TRUE;
      }

    if (strstr(FlagString,"hist") != NULL) 
      {
      ShowHist = TRUE;
      }
    else if ((strstr(FlagString,"rsv") != NULL) ||
             (strstr(FlagString,"res") != NULL))
      {
      ShowRsv = TRUE;
      }
    else if (strstr(FlagString,"prio") != NULL)
      {
      ShowPrio = TRUE;
      }
    else if (strstr(FlagString,"all") != NULL)
      {
      ShowPrio = TRUE;
      ShowRsv  = TRUE;
      ShowHist = TRUE;
      }
    else
      {
      /* default behavior (for non-remote queries only) */

      if (ShowPeer == FALSE)
        {
        ShowRsv = TRUE;
        }
      }
    }
  else
    {
    /* default behavior (for non-remote queries only).
     * Remote queries will be handled in MMJobQuery. */

    if (ShowPeer == FALSE)
      {
      ShowRsv = TRUE;
      }
    }

  /* look for partition constraint */

  /* NYI */

  *PE = NULL;

  BP = NULL;

  JE = NULL;

  if ((ShowPeer == FALSE) && 
      (MJobFind(JSpec,&tmpJ,mjsmExtended) == SUCCESS) &&
      (MJOBWASMIGRATEDTOREMOTEMOAB(tmpJ)))
    {
    /* If a job is already migrated to a remote peer, show when the job can 
     * start on the machine that it will actually run on. */

    ShowPeer = TRUE;

    PeerName = tmpJ->DestinationRM->Name;
    }

  /* auth check handled previously by __MUIJobGetStart() */

  if (ShowPeer == FALSE)
    {
    mjob_t *J = NULL;

    MJobMakeTemp(&J);

    if (ShowRsv == TRUE)
      {
      rc = __MUIJobGetStart(
        JSpec,         /* I */
        &BP,           /* O */
        C,             /* I */
        &StartTime,    /* O */
        &JE,           /* O */
        EMsg,          /* O - start time description */
        sizeof(EMsg),
        S,
        Auth);

      if (rc == FAILURE)
        {
        if ((IsVerbose == TRUE) && (S != NULL))
          {
          MUISAddData(S,EMsg); 
          }
        }
      }

    if (ShowPrio == TRUE)
      {
      rc = MJobGetEstStartTime(
        JSpec,       /* I */
        BP,          /* I */
        &StartTime,  /* O */
        &JE,         /* O */
        FALSE,       /* I - don't use hist */
        (S != NULL) ? S->RDE : NULL,
        Auth,
        EMsg,        /* O */
        sizeof(EMsg));

      if (rc == FAILURE)
        {
        if ((IsVerbose == TRUE) && (S != NULL))
          {
          MUISAddData(S,EMsg);
          }
        }
      }

    if (ShowHist == TRUE) 
      {
      /* must pass in UseFeedback (NYI) */

      rc = MJobGetEstStartTime(
        JSpec,       /* I */
        BP,          /* I */
        &StartTime,  /* O */
        &JE,         /* O */
        TRUE,        /* I - use hist */
        (S != NULL) ? S->RDE : NULL,
        Auth,
        EMsg,        /* O */
        sizeof(EMsg));

      if (rc == FAILURE)
        {
        if ((IsVerbose == TRUE) && (S != NULL))
          {
          MUISAddData(S,EMsg);
          }
        }
      }

    /* put flag on xml if job is blocked */

    if (MJobFromJSpec(
          JSpec,
          &J,    /* O */
          (S != NULL) ? S->RDE : NULL,
          Auth,
          NULL,
          NULL,
          0,
          NULL) == FAILURE)
      {
      return(FAILURE);
      }

    if (!bmisset(&J->IFlags,mjifUIQ))
      {
      char bMsg[MMAX_LINE];

      /* job is blocked */

      if (J->SpecSMinTime > MSched.Time)
        {
        sprintf(bMsg,"user-specified startdate not reached for %ld seconds",
          J->SpecSMinTime - MSched.Time);
        }
      else if (bmisset(&J->Hold,mhDefer))
        {
        char bMsg[MMAX_LINE];

        /* job is blocked */

        if (J->SpecSMinTime > MSched.Time)
          {
          sprintf(bMsg,"user-specified startdate not reached for %ld seconds",
            J->SpecSMinTime - MSched.Time);
          }
        else if (bmisset(&J->Hold,mhDefer))
          {
          sprintf(bMsg,"job deferred for %ld seconds",
            MAX(1,J->HoldTime + MSched.DeferTime - MSched.Time));
          }
        else if (MJOBISACTIVE(J) == TRUE)
          {
          sprintf(bMsg,"job is running");
          }
        else
          {
          sprintf(bMsg,"job is blocked");
          }

        MXMLSetAttr(JE,"blockmsg",(char *)bMsg,mdfString);
        }
      else if (MJOBISACTIVE(J) == TRUE)
        {
        sprintf(bMsg,"job is running");
        }
      else
        {
        sprintf(bMsg,"job is blocked");
        }

      MXMLSetAttr(JE,"blockmsg",(char *)bMsg,mdfString);
      }

    MJobFreeTemp(&J);
    }    /* END if (ShowPeer == FALSE) */
  else
    {
    int rmindex;
    char RAQMethod[MMAX_NAME];

    mrm_t  *R;
    mjob_t *J;    
    mxml_t *RE;

    MJobMakeTemp(&J);

    /* generate peer request */

    if (MJobFromJSpec(
          JSpec,
          &J,
          (S != NULL) ? S->RDE : NULL,
          Auth,
          NULL,
          EMsg,
          MMAX_LINE,
          NULL) == FAILURE)
      {
      MDB(3,fUI) MLog("INFO:     cannot translate job specification to job in %s\n",
        FName);

      MJobFreeTemp(&J);

      return(FAILURE);
      }

    MXMLCreateE(&JE,(char *)MXO[mxoJob]);

    RAQMethod[0] = '\0';   

    /* determine RAQMethod */

    if ((ShowRsv == TRUE) &&
        (ShowPrio == TRUE) &&
        (ShowHist == TRUE))
      {
      strcpy(RAQMethod,MResAvailQueryMethod[mraqmAll]);
      }
    else if (ShowRsv == TRUE)
      {
      strcpy(RAQMethod,MResAvailQueryMethod[mraqmReservation]);
      }
    else if (ShowPrio == TRUE)
      {
      strcpy(RAQMethod,MResAvailQueryMethod[mraqmPriority]);
      }
    else if (ShowHist == TRUE)
      {
      strcpy(RAQMethod,MResAvailQueryMethod[mraqmHistorical]);
      }

    /* must pass in UseFeedback (NYI) */

    /* send request to each peer */

    for (rmindex = 0;rmindex < MSched.M[mxoRM];rmindex++)
      {
      R = &MRM[rmindex];

      if (R->Name[0] == '\0')
        break;

      if (MRMIsReal(R) == FALSE)
        continue;

      if (R->Type != mrmtMoab)
        continue;

      if (R->State != mrmsActive)
        continue;

      if ((PeerName != NULL) &&
          (PeerName[0] != '\0') &&
          ((strcasecmp(PeerName,"all")) &&
           (strcmp(PeerName,R->Name))))
        {
        continue;
        }

      /* issue query to remote partition */

      if (MMJobQuery(
            R,
            J,
            (RAQMethod[0] != '\0') ? RAQMethod : NULL,
            &RE,
            NULL,
            NULL) == FAILURE)
        {
        continue;
        }

      /* append result to JE */

      MXMLSetAttr(RE,"peername",(void *)R->Name,mdfString);

      /* Set the job's jobid because MS3JobToXML() doesn't pass the jid
       * to the peer; which behavior can't change as this would change behavior
       * in the grid as the peer normally gets it's own jid. If changed
       * the peer would get the same jid as the grid head. */

      MXMLSetAttr(RE,(char *)MJobAttr[mjaJobID],(void *)J->Name,mdfString);

      MXMLAddE(JE,RE);      
      }  /* END for (rmindex) */

    MJobFreeTemp(&J);

    if (JE->CCount == 0)
      {
      /* error occured */

      MDB(3,fUI) MLog("INFO:     no children in peer response in %s\n",
        FName);

      MJobFreeTemp(&J);

      return(FAILURE);
      }
    }    /* END else (ShowPeer == FALSE) */

  if (PE != NULL)
    { 
    if (JE == NULL)
      {
      MDB(3,fUI) MLog("ALERT:    no job element in %s\n",
        FName);

      return(FAILURE);
      }

    *PE = JE;
    }

  return(SUCCESS);
  }  /* END __MUIJobGetEStartTime() */





/**
 * @param JobSpec (I)
 * @param U (I) [optional]
 * @param P (I) [optional]
 * @param NL (O) [minsize=MMAX_NODE+1]
 */

int MUIJobSpecGetNL(

  char      *JobSpec,  /* I */
  mgcred_t  *U,        /* I (optional) */
  mpar_t    *P,        /* I (optional) */
  mnl_t     *NL)       /* O (minsize=MMAX_NODE+1) */

  {
  mjob_t *J;

  char   FString[MMAX_LINE];  /* required node feature string */

  char  *ptr;

  if ((JobSpec == NULL) || (NL == NULL))
    {
    return(FAILURE);
    }

  MJobMakeTemp(&J);
  bmset(&J->IFlags,mjifIsTemplate);

  J->Credential.U = U;

  MJobBuildCL(J);

  /* parse/apply jobspec string */

  FString[0] = '\0';

  if (!strncasecmp(JobSpec,"wiki:",strlen("wiki:")))
    {
    ptr = JobSpec + strlen("wiki:");

    if (MWikiJobLoad(
        "select_template",     /* I */
        ptr,     /* I attribute list */
        J,       /* I (modified) */
        NULL,    /* I default req (optional) */
        NULL,    /* O (optional,minsize=MSched.JobMaxTaskCount) */
        NULL,    /* I (optional) */
        NULL) == FAILURE) /* O (optional,minsize=MMAX_LINE) */
      {
      return(FAILURE);
      }
    }
  else
    {
    /* NO-OP */
    }

  if (FString[0] != '\0')
    MReqSetAttr(J,J->Req[0],mrqaReqNodeFeature,(void **)FString,mdfString,mAdd);

  if (MReqGetFNL(
        J,
        J->Req[0],
        P,
        NULL,
        NL,     /* O */
        NULL,
        NULL,
        MMAX_TIME,
        0,
        NULL) == FAILURE)
    {
    MJobFreeTemp(&J);

    return(FAILURE);
    }

  if (J->NodePriority != NULL)
    {
    MNLSort(NL,J,MSched.Time,mnalMachinePrio,NULL,FALSE);
    }

  MJobFreeTemp(&J);

  return(SUCCESS);
  }  /* END MUIJobSpecGetNL() */

