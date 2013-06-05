/* HEADER */

/**
 * @file MUIJobCtl.c
 *
 * Contains MUI Job Control functions
 */



#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  




/** 
 * Process job template actions.
 *
 * @note handles mjobctl -T
 * @see MUIJobCtl - parent
 *
 * @param S
 * @param AFlags
 * @param Auth
 */

int __MUIJobTemplateCtl(

  msocket_t *S,
  char      *Auth)

  {
  mxml_t *RE = NULL;
  mxml_t *WE = NULL;

  char Command[MMAX_NAME];
  char JobExp[MMAX_LINE];
  char tmpVal[MMAX_BUFFER];
  char tmpName[MMAX_LINE];
  char FlagString[MMAX_LINE];
  char ArgString[MMAX_LINE];
  char TemplateBuffer[MMAX_BUFFER];
  char EMsg[MMAX_LINE] = {0};

  enum MJobCtlCmdEnum CIndex;

  int   WTok;

  mjob_t *TJob;

  mgcred_t *U;

  if (S->WireProtocol != mwpS32)
    {
    return(FAILURE);
    }

  if (S->RDE != NULL)
    {
    RE = S->RDE;
    }
  else
    {
    return(FAILURE);
    }

  if (MXMLGetAttr(RE,MSAN[msanAction],NULL,Command,sizeof(Command)) == FAILURE)
    {
    MUISAddData(S,"ERROR:    corrupt command received\n");

    return(FAILURE);
    }

  if ((CIndex = (enum MJobCtlCmdEnum)MUGetIndexCI(
         Command,
         MJobCtlCmds,
         FALSE,
         mjcmNONE)) == mjcmNONE)
    {
    MDB(3,fUI) MLog("WARNING:  unexpected subcommand '%.32s' received\n",
      Command);

    MUISAddData(S,"ERROR:    unexpected subcommand\n");

    return(FAILURE);
    }

  MXMLGetAttr(RE,(char *)MXO[mxoJob],NULL,JobExp,sizeof(JobExp));

  /* process 'where' constraints */

  WTok = -1;

  TemplateBuffer[0] = '\0';

  while (MS3GetWhere(
      RE,
      &WE,
      &WTok,
      tmpName,
      sizeof(tmpName),
      tmpVal,
      sizeof(tmpVal)) == SUCCESS)
    {
    if ((!strcmp(tmpName,"jobtype")) &&
        (!strcmp(tmpVal,"template")))
      {
      continue;
      }
    else if (!strcmp(tmpName,"template"))
      {
      MUStrCpy(TemplateBuffer,tmpVal,sizeof(TemplateBuffer));
      }
    }      /* END while (MS3GetWhere(RE,&WE,&Tok) == SUCCESS) */

  while (MS3GetSet(
        RE,
        &WE,
        &WTok,
        tmpName,
        sizeof(tmpName),
        tmpVal,
        sizeof(tmpVal)) == SUCCESS)
    {
    if (!strcmp(tmpName,"args"))
      {
      MUStrCpy(TemplateBuffer,tmpVal,sizeof(TemplateBuffer));
      }
    }

  /* load additional command attributes */

  MXMLGetAttr(RE,MSAN[msanFlags],NULL,FlagString,0);

  MXMLGetAttr(RE,MSAN[msanArgs],NULL,ArgString,0);

  TJob = NULL;

  if (MUserAdd(Auth,&U) == FAILURE)
    {
    MDB(3,fUI) MLog("WARNING:  cannot add user '%s'\n",
      Auth);

    return(FAILURE);
    }

  switch (CIndex)
    {
    case mjcmCreate:

      if (MTJobFind(JobExp,NULL) == SUCCESS)
        {
        MUISAddData(S,"ERROR:    a template with that name already exists\n");

        return(FAILURE);
        }

      if (TemplateBuffer[0] == '<')
        {
        mxml_t *tmpE = NULL;

        if (MTJobAdd(JobExp,&TJob) == FAILURE)
          {
          MUISAddData(S,"ERROR:    failed to create job structure\n");

          return(FAILURE);
          }

        /* Convert description to XML */

        if ((MXMLFromString(&tmpE,TemplateBuffer,NULL,EMsg) == FAILURE) ||
            (MJobFromXML(TJob,tmpE,TRUE,mAdd) == FAILURE))
          {
          if (EMsg[0] != '\0')
            MUISAddData(S,EMsg);
          else
            MUISAddData(S,"ERROR:    failed to create job from XML\n");

          bmset(&TJob->IFlags,mjifTemplateIsDeleted);

          MXMLDestroyE(&tmpE);

          return(FAILURE);
          }

        MXMLDestroyE(&tmpE);
        }  /* END if (TemplateBuffer[0] == '<') */
      else if (TemplateBuffer[0] != '\0')
        {
        if (MTJobProcessConfig(JobExp,TemplateBuffer,&TJob,EMsg) == FAILURE)
          {
          if (EMsg[0] != '\0')
            MUISAddData(S,EMsg);
          else
            MUISAddData(S,"ERROR:    failed to create template\n");

          return(FAILURE);
          }
        }
      else
        {
        MUISAddData(S,"ERROR:    template description needed\n");

        return(FAILURE);
        }

#ifdef MNOT /* enable when this becomes an end-user command */
      MUserAdd(Auth,&U);

      TJob->Credential.U = U;

      MACLSet(
        &TJob->RequiredCredList,
        maUser,
        (void *)U,
        mcmpSEQ,
        0,        /* I affinity - not used */
        0,        /* I flags - not used */
        TRUE);
#endif /* MNOT */

      if (TJob->TemplateExtensions == NULL)
        {
        MJobAllocTX(TJob);
        }

      bmset(&TJob->TemplateExtensions->TemplateFlags,mtjfTemplateIsSelectable);
      bmset(&TJob->TemplateExtensions->TemplateFlags,mtjfTemplateIsDynamic);
      bmset(&TJob->IFlags,mjifTemplateIsDynamic);

      MUISAddData(S,"successfully created template\n");

      break;

    case mjcmDelete:

      if ((JobExp[0] == '\0') ||
          (MTJobFind(JobExp,&TJob) == FAILURE))
        {
        MUISAddData(S,"ERROR:      could not access template\n");

        return(FAILURE);
        }

#ifdef MNOT /* enable when this becomes an end user command */
      if (MTJobCheckCredAccess(TJob,mxoUser,Auth) == FAILURE)
        {
        MUISAddData(S,"ERROR:    could not access template specified template\n");

        return(FAILURE);
        }
#endif 

      if (MTJobDestroy(TJob,EMsg,sizeof(EMsg)) == FAILURE)
        {
        MUISAddData(S,EMsg);

        return(FAILURE);
        }

      MUISAddData(S,"successfully deleted template\n");

      break;

    case mjcmModify:

      if ((JobExp[0] == '\0') ||
          (MTJobFind(JobExp,&TJob) == FAILURE))
        {
        MUISAddData(S,"ERROR:      could not access template\n");

        return(FAILURE);
        }

#ifdef MNOT /* enable when this becomes an end user command */
      if (MTJobCheckCredAccess(TJob,mxoUser,Auth) == FAILURE)
        {
        MUISAddData(S,"ERROR:    could not access template specified template\n");

        return(FAILURE);
        }
#endif

      if (TemplateBuffer[0] == '<')
        {
        mxml_t *tmpE = NULL;

        /* Convert description to XML */

        if ((MXMLFromString(&tmpE,TemplateBuffer,NULL,EMsg) == FAILURE) ||
            (MJobFromXML(TJob,tmpE,TRUE,mAdd) == FAILURE))
          {
          if (EMsg[0] != '\0')
            MUISAddData(S,EMsg);
          else
            MUISAddData(S,"ERROR:    failed to modify job from XML\n");

          bmset(&TJob->IFlags,mjifTemplateIsDeleted);

          MXMLDestroyE(&tmpE);

          return(FAILURE);
          }

        MXMLDestroyE(&tmpE);
        }
      else if (TemplateBuffer[0] != '\0')
        {
        if (MTJobProcessConfig(JobExp,TemplateBuffer,&TJob,EMsg) == FAILURE)
          {
          MUISAddData(S,EMsg);

          return(FAILURE);
          }
        }
      else
        {
        MUISAddData(S,"ERROR:    template description needed\n");

        return(FAILURE);
        }

      /* always set, just to make sure user didn't clear these flags */

      bmset(&TJob->TemplateExtensions->TemplateFlags,mtjfTemplateIsSelectable);
      bmset(&TJob->TemplateExtensions->TemplateFlags,mtjfTemplateIsDynamic);

      MUISAddData(S,"successfully modified template\n");

      break;

    case mjcmQuery:

      {
      char *ptrRE;
      mxml_t *RE = NULL;

      bmisset(&S->Flags,msftReadOnlyCommand);

      if (S->RDE != NULL)
        {
        RE = S->RDE;
        }
      else
        {
        if ((S->RPtr == NULL) ||
            ((ptrRE = strchr(S->RPtr,'<')) == NULL) ||
            (MXMLFromString(&RE,ptrRE,NULL,NULL) == FAILURE))
          {
          MUISAddData(S,"ERROR:    cannot read request\n");

          return(FAILURE);
          }

        S->RDE = RE;
        }

      if (MXMLCreateE(&S->SDE,(char *)MSON[msonData]) == FAILURE)
        {
        MUISAddData(S,"ERROR:    cannot create response\n");

        return(FAILURE);
        }

      MUIJobQueryTemplates(S->SDE,RE);
      }    /* END BLOCK (case mjcmQuery) */

      break;

    default:

      /* NYI */

      return(FAILURE);

      /*NOTREACHED*/

      break;
    }  /* END switch (CIndex) */

  return(SUCCESS);
  }  /* END __MUIJobTemplateCtl() */




/**
 * Process job control (mjobctl) requests.
 *
 * The action can be one of
 *   - cancel :   cancels the job. 
 *   - diagnose : currently unsupported
 *   - hold :     put a hold on the job
 *   - modify 
 *   - query
 *   - resume
 *   - signal
 *   - start
 *   - submit
 *   - suspend
 * 
 * This function currently (6/20/2007) handles the following jobctl commands:
 *   - mjcmQuery : calls MUIJobQuery()
 *   - mjcmCancel : 
 *   - mjcmTerminate : Handled with mjcmCancel
 *   - mjcmCheckpoint : Calls __MUIJobPreempt()
 *   - mjcmMigrate : Calls __MUIJobMigrate()
 *   - mjcmModify : Calls MUIJobModify()
 *   - mjcmRequeue : Calls __MUIJobPreempt()
 *   - mjcmResume : Calls __MUIJobResume()
 *   - mjcmShow : NYI, but has case statement
 *   - mjcmSignal : Calls MS3GetSet() and MRMJobSignal()
 *   - mjcmStart : Calls a bunch of things
 *   - mjcmSubmit : Is handled early, but has "case" statement later. Calls MUIJobSubmit()
 *   - mjcmSuspend : Calls __MUIJobPreempt()
 *
 * Following are jobctl cases that are NYI (6/20/2007)
 *   - mjcmDiagnose
 *   - mjcmAdjustHold
 *   - mjcmAdjustPrio
 * 
 * The command can take an expression of jobs. Each job that the expression evaluates to 
 * will have the action executed on it.
 * The action can also take flags like "verbose"
 * 
 * FORMAT:  <jobctl action={cancel,diagnose,hold,modify,query,resume,signal,start,submit,suspend} job={jobexp} flags={VERBOSE}></jobctl> 
 *
 * @param S (I) The socket that the data resides on.
 * @param AFlags (I) [bitmap of enum MRoleEnum]
 * @param Auth (I)
 */

int MUIJobCtl(

  msocket_t *S,      /* I */
  mbitmap_t *AFlags, /* I (bitmap of enum MRoleEnum) */
  char      *Auth)   /* I */

  {
  marray_t JobList;

  char Command[MMAX_NAME];
  char JobExp[MMAX_LINE*8];
  char FlagString[MMAX_LINE];
  char ArgString[MMAX_LINE];
  char Annotation[MMAX_LINE];

  char tmpLine[MMAX_LINE*8];
  char tEMsg[MMAX_LINE] = {0};

  mbool_t JobTypeIsTemplate = FALSE;

  marray_t MasterJobs;

  enum MJobCtlCmdEnum CIndex;
  int  jindex;

  int  rc;

  /* used for determining if charges will be made while preempting/requeueing */
  mbool_t   IsAdmin;

  mjob_t *J;

  mbool_t GroupOperation;

  mxml_t *RE = NULL;
  mxml_t *WE = NULL;

  int     WTok;

  mpar_t *P;

  char    TimeString[MMAX_LINE];

  int     CAIndex;
  int     COIndex;

  char    tmpVal[MMAX_LINE];
  char    tmpName[MMAX_LINE];

  char    tmpMsg[MMAX_LINE];

  mjobconstraint_t JCon[16];

  mbool_t JobExpIsRegEx = FALSE;

  mgcred_t *U  = NULL;
  mpsi_t   *PSI = NULL;
  mrm_t    *PR = NULL;

  mgcred_t *JU = NULL;
  mgcred_t *JG = NULL;
  mgcred_t *JA = NULL;
  mclass_t *JC = NULL;
  mqos_t   *JQ = NULL;
  mrsv_t   *JRes = NULL;
  char     *JName = NULL;
  enum MJobStateEnum JS = mjsNONE;

  mbitmap_t JP;

  char      SubArg[MMAX_LINE];

  mbool_t TransactionSuccessful;
  mbool_t ProcessAll = FALSE;
  mbool_t ProcessAny = FALSE;

  const char *FName = "MUIJobCtl";

  MDB(2,fUI) MLog("%s(S,%s)\n",
    FName,
    (Auth != NULL) ? Auth : "NULL");

  if (S == NULL)
    {
    return(FAILURE);
    }

  Annotation[0] = '\0';
  TimeString[0] = '\0';

  SubArg[0] = '\0';

  GroupOperation = FALSE;

  MUISClear(S);

  {
  char *ptr;
  char *TokPtr;

  if (S->RDE != NULL)
    {
    RE = S->RDE;
    }
  else
    {
    if ((S->RPtr == NULL) ||
       ((ptr = strchr(S->RPtr,'<')) == NULL) ||
        (MXMLFromString(&RE,ptr,NULL,NULL) == FAILURE))
      {
      MDB(3,fUI) MLog("WARNING:  corrupt command '%100.100s' received\n",
        S->RPtr);

      MUISAddData(S,"ERROR:    corrupt command received\n");

      return(FAILURE);
      }

    S->RDE = RE;
    }

  if (MXMLGetAttr(RE,MSAN[msanAction],NULL,Command,sizeof(Command)) == FAILURE)
    {
    MUISAddData(S,"ERROR:    corrupt command received\n");

    return(FAILURE);
    }

  MXMLGetAttr(RE,(char *)MXO[mxoJob],NULL,JobExp,sizeof(JobExp));

  /* process 'set' conditions */

  WTok = -1;

  while (MS3GetSet(
      RE,
      &WE,
      &WTok,
      tmpName,
      sizeof(tmpName),
      tmpVal,
      sizeof(tmpVal)) == SUCCESS)
    {
    if (!strcmp(tmpName,"message"))
      MUStrCpy(Annotation,tmpVal,sizeof(Annotation));
    else if (!strcmp(tmpName,"message"))
      MUStrCpy(ArgString,tmpVal,sizeof(SubArg));
    }

  /* process 'where' constraints */

  WTok = -1;

  CAIndex = 0;

  memset(&JCon,0,sizeof(mjobconstraint_t) * 16);

  while (MS3GetWhere(
      RE,
      &WE,
      &WTok,
      tmpName,
      sizeof(tmpName),
      tmpVal,
      sizeof(tmpVal)) == SUCCESS)
    {
    JCon[CAIndex].AType = (enum MJobAttrEnum)MUGetIndexCI(
      tmpName,
      MJobAttr,
      FALSE,
      mjaNONE);

    if (JCon[CAIndex].AType != mjaNONE)
      {
      COIndex = 0;

      /* load job constraints */

      if (MXMLGetAttr(WE,"comp",NULL,tmpName,sizeof(tmpName)) == SUCCESS)
        {
        JCon[CAIndex].ACmp = (enum MCompEnum)MUGetIndexCI(
          tmpName,
          MComp,
          FALSE,
          mcmpSEQ);
        }

      /* FORMAT:  <VAL>[,<VAL>]... */

      ptr = MUStrTok(tmpVal,", \t\n",&TokPtr);

      while (ptr != NULL)
        {
        switch (JCon[CAIndex].AType)
          {
          case mjaCompletionTime:
          case mjaStartTime:

            JCon[CAIndex].ALong[COIndex] = MUTimeFromString(ptr);

            break;

          case mjaAccount:

            if (MAcctFind(ptr,&JA) == FAILURE)
              {
              MUISAddData(S,"ERROR:    cannot locate requested account\n");

              return(FAILURE);
              }

            MUStrCpy(JCon[CAIndex].AVal[COIndex],ptr,sizeof(JCon[0].AVal[0]));

            break;

          case mjaClass:

            if (MClassFind(ptr,&JC) == FAILURE)
              {
              MUISAddData(S,"ERROR:    cannot locate requested class\n");

              return(FAILURE);
              }

            MUStrCpy(JCon[CAIndex].AVal[COIndex],ptr,sizeof(JCon[0].AVal[0]));

            break;

          case mjaGroup:

            if (MGroupFind(ptr,&JG) == FAILURE)
              {
              MUISAddData(S,"ERROR:    cannot locate requested group\n");

              return(FAILURE);
              }

            MUStrCpy(JCon[CAIndex].AVal[COIndex],ptr,sizeof(JCon[0].AVal[0]));

            break;

          case mjaJobName:

            MUStrCpy(JCon[CAIndex].AVal[COIndex],ptr,sizeof(JCon[0].AVal[0]));

            JName = JCon[CAIndex].AVal[COIndex];

            break;

          case mjaPAL:

            if (MPALFromString(ptr,mVerify,&JP) == FAILURE)
              {
              MUISAddData(S,"ERROR:    cannot locate requested partition\n");

              return(FAILURE);
              }

            MUStrCpy(JCon[CAIndex].AVal[COIndex],ptr,sizeof(JCon[0].AVal[0]));

            break;

          case mjaQOS:

            if (MQOSFind(ptr,&JQ) == FAILURE)
              {
              MUISAddData(S,"ERROR:    cannot locate requested qos\n");

              return(FAILURE);
              }

            MUStrCpy(JCon[CAIndex].AVal[COIndex],ptr,sizeof(JCon[0].AVal[0]));

            break;

          case mjaReqReservation:

            if (MRsvFind(ptr,&JRes,mraNONE) == FAILURE)
              {
              MUISAddData(S,"ERROR:    cannot locate requested reservation\n");

              return(FAILURE);
              }

            MUStrCpy(JCon[CAIndex].AVal[COIndex],ptr,sizeof(JCon[0].AVal[0]));

            break;

          case mjaUser:

            if (MUserFind(ptr,&JU) == FAILURE)
              {
              MUISAddData(S,"ERROR:    cannot locate requested user\n");

              return(FAILURE);
              }

            MUStrCpy(JCon[CAIndex].AVal[COIndex],ptr,sizeof(JCon[0].AVal[0]));

            break;

          case mjaState:

            if ((JS = (enum MJobStateEnum)MUGetIndexCI(ptr,MJobState,FALSE,mjsNONE)) == mjsNONE)
              {
              MUISAddData(S,"ERROR:    cannot locate requested job state\n");

              return(FAILURE);
              }

            MUStrCpy(JCon[CAIndex].AVal[COIndex],ptr,sizeof(JCon[0].AVal[0]));

            break;

          default:

            MUStrCpy(JCon[CAIndex].AVal[COIndex],ptr,sizeof(JCon[0].AVal[0]));

            break;
          }  /* END switch (JCon[CAIndex].AType) */

        COIndex++;

        ptr = MUStrTok(NULL,", \t\n",&TokPtr);
        }  /* END while (ptr != NULL) */

      CAIndex++;
      }    /* END if (JCon[CAIndex].AType != mjaNONE) */
    else if (!strcmp(tmpName,"timeframe"))
      {
      MUStrCpy(TimeString,tmpVal,sizeof(TimeString));
      }
    else if ((!strcmp(tmpName,"jobtype")) &&
             (!strcmp(tmpVal,"template")))
      {
      JobTypeIsTemplate = TRUE;  

      /* stop processing and jump into __MUIJobTemplateCtl */

      break;
      }
    }      /* END while (MS3GetWhere(RE,&WE,&Tok) == SUCCESS) */

  JCon[CAIndex].AType = mjaNONE;

  /* load additional command attributes */

  MXMLGetAttr(RE,MSAN[msanFlags],NULL,FlagString,0);

  if (strstr(FlagString,"group") != NULL)
    {
    GroupOperation = TRUE;
    }

  MXMLGetAttr(RE,MSAN[msanArgs],NULL,ArgString,0);
  }  /* END BLOCK */

  if (JobTypeIsTemplate == TRUE)
    {
    /* DOUG is lazy and didn't want to incorporate all the template stuff into this unwieldy routine */

    return(__MUIJobTemplateCtl(S,Auth));
    }

  /* process data */

  if ((CIndex = (enum MJobCtlCmdEnum)MUGetIndexCI(
         Command,
         MJobCtlCmds,
         FALSE,
         mjcmNONE)) == mjcmNONE)
    {
    MDB(3,fUI) MLog("WARNING:  unexpected subcommand '%s' received\n",
      Command);

    sprintf(tmpLine,"ERROR:    unexpected subcommand '%s'\n",
      Command);

    MUISAddData(S,tmpLine);

    return(FAILURE);
    }

  /* initialize constraints */

  P = &MPar[0];

  /* handle special case subcmds (no jobexp parsing, 
     therefore no authorization check) */
  
  switch (CIndex)
    {
    case mjcmQuery:

      bmset(&S->Flags,msftReadOnlyCommand);

      /* RE->Args = { diag | starttime | timeline | hostlist } */

      return(MUIJobQuery(JobExp,Auth,JCon,P,S,TimeString,RE));

      break;

    case mjcmRerun:

      return(__MUIJobRerun(JobExp,Auth,S,RE));

      break;

    case mjcmSubmit:

      /* process job submission */

      return(MUIJobSubmit(S,Auth,-1,RE,NULL,NULL));

      break;

    default:

      /* carry-on */

      break;
    }  /* END switch (CIndex) */

  if (JobExp[0] == '\0')
    {
    if ((JA == NULL) && 
        (JG == NULL) && 
        (JC == NULL) && 
        (bmisclear(&JP)) &&
        (JQ == NULL) && 
        (JRes == NULL) &&
        (JU == NULL) && 
        (JS == mjsNONE) &&
        (JName == NULL))
      {
      /* no job expression specified */

      MUISAddData(S,"no job expression specified");

      return(FAILURE);
      }

    strcpy(JobExp,"ALL");
    }

  /* translate job expression to list of jobs */

  /* NOTE:  encapsulate job expression '^(<X>)$' */

  /* NOTE: don't try to "test" if the job is a regular expression because
           torque job arrays have "[]" in them */

  if ((MSched.UseJobRegEx == TRUE) || 
      (!strcmp(JobExp,"ALL"))      || 
      (!strcmp(JobExp,ALL)))
    {
    JobExpIsRegEx = TRUE;

    MUStrCpy(tmpLine,JobExp,sizeof(tmpLine));
    }
  else
    {
    if ((strstr(JobExp,"r:")) || 
        (strstr(JobExp,"x:")))
      {
      snprintf(tmpLine,sizeof(tmpLine),"%s",
        JobExp);
      JobExpIsRegEx = TRUE;
      }
    else if (JobExpIsRegEx == TRUE)
      {
      snprintf(tmpLine,sizeof(tmpLine),"^(%s)$",
        JobExp);
      }
    }

  MUArrayListCreate(&JobList,sizeof(mjob_t *),-1);

  tmpMsg[0] = '\0';

  if (JobExpIsRegEx == TRUE)
    {
    if (!strcmp(JobExp,"ALL"))
      {
      if (CIndex == mjcmMigrate)
        {
        ProcessAny = TRUE;
        }
      else
        {
        ProcessAll = TRUE;
        }
      }

    if (ProcessAny == TRUE)
      {
      /* NO-OP */
      }
    else if ((tmpLine[0] == '\0') || (MUREToList(
          tmpLine,
          mxoJob,
          NULL,
          &JobList,           /* O */
          FALSE,
          tmpMsg,
          sizeof(tmpMsg)) == FAILURE) || (JobList.NumItems == 0))
      {
      tmpMsg[0] = '\0';

      /* look for job AName if job ID lookup fails */

      if ((tmpLine[0] == '\0') || (MUREToList(
            tmpLine,
            mxoJob,
            NULL,
            &JobList,           /* O */
            TRUE,
            tmpMsg,
            sizeof(tmpMsg)) == FAILURE) || (JobList.NumItems == 0))
        {
        snprintf(tmpLine,sizeof(tmpLine),"ERROR:    invalid job expression received '%s' : %s\n",
          JobExp,
          (tmpLine[0] == '\0') ? "no job specified" : tmpMsg);

        MUISAddData(S,tmpLine);

        MUArrayListFree(&JobList);

        return(FAILURE);
        }
      }
    }    /* END if (JobExpIsRegEx == TRUE) */
  else
    {
    char SystemJID[MMAX_NAME];
    char *ptr;
    char *TokPtr;
    char *SJIDPtr;
    char *SJIDTokPtr;

    mbool_t IsTemplate;

    MXMLGetAttr(RE,(char *)MJobAttr[mjaGJID],NULL,SystemJID,sizeof(SystemJID));
      
    ptr = MUStrTok(JobExp,", \t\n",&TokPtr);

    SJIDPtr = MUStrTok(SystemJID,", \t\n",&SJIDTokPtr);

    while (ptr != NULL)
      {
      if (strncasecmp(ptr,"template:",strlen("template:")))
        {
        IsTemplate = FALSE;
        }
      else
        {
        ptr += strlen("template:");
        IsTemplate = TRUE;
        }

      /* don't do extended as that will return completed jobs too */

      if ((IsTemplate == FALSE) &&
          (MJobFind(ptr,&J,mjsmExtended) == SUCCESS))
        {
        /* normal batch job located */

        /* NO-OP */
        }
      else if (MTJobFind(ptr,&J) == SUCCESS)
        {
        /* job is template, just add it to the list */

        /* NO-OP */
        }
      else if ((SJIDPtr != NULL) &&
               (MJobFindSystemJID(SJIDPtr,&J,mjsmBasic) == SUCCESS))
        {
        /* job located by system JID */

        /* NO-OP */
        }
      else
        {
        /* spec does not match any job */

        snprintf(tmpLine,sizeof(tmpLine),"ERROR:  invalid job specified (%s)\n",
          ptr);

        MUISAddData(S,tmpLine);

        MUArrayListFree(&JobList);

        return(FAILURE);
        }

      MUArrayListAppendPtr(&JobList,J);

      if (J->Array != NULL)
        {
        GroupOperation = TRUE;
        }

      if (GroupOperation == TRUE)
        {
        job_iter JTI;

        MJobIterInit(&JTI);

        while (MJobTableIterate(&JTI,&J) == SUCCESS)
          {
          if ((J->JGroup != NULL) && (J->JGroup->Name != NULL) && (!strcmp(ptr,J->JGroup->Name)))
            {
            MUArrayListAppendPtr(&JobList,J);
            }
          }
        }

      ptr = MUStrTok(NULL,", \t\n",&TokPtr);

      SJIDPtr = MUStrTok(NULL,", \t\n",&SJIDTokPtr);
      }  /* END while (ptr != NULL) */
    }    /* END else (JobExpIsRegEx == TRUE) */

  /* virtual cluster auth check */

  if (!strncasecmp(Auth,"peer:",strlen("peer:")))
    {
    if (MPeerFind(Auth,FALSE,&PSI,FALSE) == FAILURE)
      {
      /* NYI */
      }
    else
      {
      MRMFindByPeer(PSI->Name,&PR,FALSE);
      }
    }
  else if (MUserAdd(Auth,&U) == FAILURE)
    {
    MDB(3,fUI) MLog("WARNING:  cannot add user '%s'\n",
      Auth);

    MUArrayListFree(&JobList);

    return(FAILURE);
    }

  if (JobList.NumItems == 0)
    {
    MUISAddData(S,"no jobs selected");
 
    MUArrayListFree(&JobList);

    return(SUCCESS);
    }

  MDB(3,fUI) MLog("INFO:     attempting %s operation on %d jobs\n",
    MJobCtlCmds[CIndex],
    JobList.NumItems);

  memset(&MasterJobs,0,sizeof(MasterJobs));

  /* process all specified jobs */
 
  mstring_t Response(MMAX_LINE);

  TransactionSuccessful = FALSE;

  for (jindex = 0;jindex < JobList.NumItems;jindex++)
    {
    enum MJobStateEnum tmpState = mjsNONE;

    J = (mjob_t *)MUArrayListGetPtr(&JobList,jindex);

    if (J == NULL)
      {
      continue;
      }

    if (bmisset(&J->Hold,mhBatch))
      tmpState = mjsBatchHold;
    else if (bmisset(&J->Hold,mhSystem))
      tmpState = mjsSystemHold;
    else if (bmisset(&J->Hold,mhUser))
      tmpState = mjsUserHold;
    else if (bmisset(&J->Hold,mhDefer))
      tmpState = mjsDeferred;
    else
      tmpState = (J->IState != mjsNONE) ? J->IState : J->State;

    if (((JA != NULL) && (J->Credential.A != JA)) ||
        ((JC != NULL) && (J->Credential.C != JC)) ||
        ((JG != NULL) && (J->Credential.G != JG)) ||
        ((JQ != NULL) && (J->Credential.Q != JQ)) ||
        ((JU != NULL) && (J->Credential.U != JU)) ||
        ((JS != mjsNONE) && (tmpState != JS)) ||
        ((JName != NULL) && ((J->AName == NULL) || strcmp(JName,J->AName))))
      {
      /* job does not match specified cred */

      continue;
      }

    if ((JRes != NULL) &&
        (J->Rsv != NULL))
      {
      mnode_t   *N;
      int nindex1;
      mbool_t ResFound = TRUE;

      /* Rsv must be checked node-by-node */

      for (nindex1 = 0;MNLGetNodeAtIndex(&J->Rsv->NL,nindex1,&N) == SUCCESS;nindex1++)
        {
        if (MREFindRsv(N->RE,JRes,NULL) == FAILURE)
          {
          ResFound = FALSE;
          }
        }    /* END for nindex1 */

      if (ResFound == FALSE)
        {
        /* Job has at least one node that lies outside JRes */

        continue;
        }
      }    /* END if (JRes != NULL) */

    if (!bmisclear(&JP))
      {
      /* verify partition */

      if (tmpState == mjsRunning)
        {
        /* since the job is running we expect a single partition and use J->Req[0]->PtIndex */

        if (!bmisset(&JP,J->Req[0]->PtIndex))
          continue;
        }
      else
        {
        /* job not running, see if J->PAL is a subset of the requested partitions */

        mbitmap_t CopyPAL;
        int pindex;

        /* make a temp copy of the bitmap of the partitions where the job could run */

        bmcopy(&CopyPAL,&J->PAL);

        for (pindex = 0; pindex < MMAX_PAR; pindex++)
          {
          if (bmisset(&JP,pindex))
            {
            bmunset(&CopyPAL,pindex);
            }
          } /* END for loop */

        /* if all of the job's partitions were found in the requested partitions 
           then this job can be cancelled */

        if (!bmisclear(&CopyPAL))
          {
          /* valid partitions remain, skip */

          bmclear(&CopyPAL);

          continue;
          }

        bmclear(&CopyPAL);
        } /* END else (tmpState == mjsRunning) */
      } /* END if (!bmisclear(&JP)) */

    /* verify job access */

    if (MUICheckAuthorization(
          U,
          PSI,
          (void *)J,
          mxoJob,
          mcsMJobCtl,
          CIndex,
          &IsAdmin,
          tEMsg,  /* O */
          sizeof(tEMsg)) == FAILURE)
      {
      /* check if user has special access to this job */

      if ((PR != NULL) && !bmisset(&PR->LocalJobExportPolicy,mexpControl))
        {
        if ((J->SystemID == NULL) || strcmp(PR->Name,J->SystemID))
          { 
          /* cannot access/modify local job */

          sprintf(tmpLine,"RM peer '%s' is attempting to modify the job '%s', but it is not allowed (%s)\n",
            PR->Name,
            J->Name,
            tEMsg);

          MDB(2,fUI) MLog("%s",
            tmpLine);

          MStringAppend(&Response,tmpLine);

          continue;
          }
        }
      else
        {
        /* cannot access/modify local job */

        if ((ProcessAll == TRUE) && (JobList.NumItems > 1))
          {
          /* ignore failures - non-admin 'mjobctl ALL' only target accessible jobs */

          continue;
          }

        sprintf(tmpLine,"ERROR:  user '%s' cannot access job '%s' (%s)\n",
          U->Name,
          J->Name,
          tEMsg);

        MDB(3,fUI) MLog("%s",
          tmpLine);

        MStringAppend(&Response,tmpLine);

        continue;
        }
      }  /* END if (MUICheckAuthorization()i == FAILURE) */

    switch (CIndex)
      {
      case mjcmForceCancel:

        MDB(3,fSCHED) MLog("INFO:    force-cancel requested for job '%s'\n",
          J->Name);

        bmset(&J->IFlags,mjifRequestForceCancel);

        /* intentional fall through */

      case mjcmCancel:
      case mjcmTerminate:

        {
        char EMsg[MMAX_LINE] = {0};
        char tmpName[MMAX_NAME];
        int  SC;
        mbool_t VMCreateLog = FALSE;

        if (bmisset(&J->IFlags,mjifNoImplicitCancel) &&
            (ProcessAll == TRUE))
          {
          /* do not cancel jobs w/ implicit protection if this job was covered by "ALL" */

          break;
          }

        /* NOTE:  save job id, internal jobs may be removed immediately */

        if (bmisset(&J->IFlags,mjifIsRsvJob))
          {
          snprintf(tmpLine,sizeof(tmpLine),"cannot cancel rsv job '%s' (try removing reservation associated reservation '%s')\n",
            J->Name,
            (J->JGroup != NULL) ? J->JGroup->Name : "");

          MStringAppend(&Response,tmpLine);

          break;
          }

        if ((!bmisset(&J->IFlags,mjifRequestForceCancel)) && bmisset(&J->Flags,mjfArrayMaster))
          {
          /* don't cancel master array jobs until we know all the subjobs are gone */

          if (MasterJobs.ArraySize == 0)
            MUArrayListCreate(&MasterJobs,sizeof(mjob_t *),10);

          MUArrayListAppendPtr(&MasterJobs,J);

          break;
          }

        if ((J->System != NULL) && (MJOBISACTIVE(J)))
          {
          mbool_t AllowCancel = FALSE;

          switch (J->System->JobType)
            {
            /*case msjtVMCreate:*/
            case msjtVMTracking:
            case msjtGeneric:
            case msjtStorage:
            case msjtVMMigrate:

              AllowCancel = TRUE;

              break;

            /*case msjtVMDestroy:*/
            case msjtOSProvision:
            case msjtPowerOn:
            case msjtPowerOff:

              /* check for "force" flag */

              if (bmisset(&J->IFlags,mjifRequestForceCancel))
                {
                bmset(&J->IFlags,mjifSystemJobFailure);

                bmset(&J->IFlags,mjifWasCanceled);

                snprintf(tmpLine, sizeof(tmpLine),"marking system job '%s' for cancellation\n",
                  J->Name);

                MStringAppend(&Response,tmpLine);

                break;
                }

            default:

              snprintf(tmpLine, sizeof(tmpLine),"cannot cancel running system job '%s'\n",
                J->Name);

              MStringAppend(&Response,tmpLine);

              break;
            }  /* END switch (J->System->JobType) */

          if (AllowCancel == FALSE)
            break;
          }  /* END if ((J->System != NULL) && (MJOBISACTIVE(J))) */

        MUStrCpy(tmpName,J->Name,sizeof(tmpName));

        /* populate tmpLine with cancel message */

        /* NYI */

        /* inform system that admin 'full job cancel' is initiated */
#if 0
        if ((J->System != NULL) && 
            (J->System->JobType == msjtVMCreate))
          {
          /* just in case we need to log after canceling the job below */

          VMCreateLog = TRUE;
          }
#endif

        if (J->SubState == mjsstEpilog)
          {
          MDB(3,fUI) MLog("INFO:     job '%s' completed in RM/cancelled by user %s\n",
            tmpName,
            Auth);

          /* NO-OP */
          }
        else if (MJobCancelWithAuth(Auth,J,Annotation,FALSE,EMsg,&SC) == SUCCESS)
          {
          MDB(3,fUI) MLog("INFO:     job '%s' cancelled by user %s\n",
            tmpName,
            Auth);

          if (VMCreateLog)
            {
            sprintf(tmpLine,"job '%s' will be allowed to run to completion, a vmdestroy job was created\n",
              tmpName);

            MDB(3,fUI) MLog("INFO:     job '%s' will be allowed to run to completion, a vmdestroy job was created\n",
              tmpName);
            }
          else if ((SC == mscPartial) || (SC == mscPending))
            {
            /* Note: not sure if mscPartial should be here, it may be a cut and paste error, but
             * we do need mscPending for pending job cancels, so that has been added and mscPartial
             * is left here just in case it really is needed for something. */

            sprintf(tmpLine,"job '%s' cancelled (%s)\n",
              tmpName,
              EMsg);
            }
          else
            {
            sprintf(tmpLine,"job '%s' cancelled\n",
              tmpName);
            }

          MStringAppend(&Response,tmpLine);

          TransactionSuccessful = TRUE;

          /* MRMJobCancel handles the MJobTransition(), J is deleted by the time we get here */
          }
        else
          {
          MDB(3,fUI) MLog("ALERT:    cannot cancel job '%s' (%s)\n",
            J->Name,
            EMsg);

          sprintf(tmpLine,"ERROR:  cannot cancel job '%s' (%s)\n",
            J->Name,
            EMsg);

          MStringAppend(&Response,tmpLine);
          }

        break;
        }    /* END BLOCK */

        break;

      case mjcmJoin:

        {
        char EMsg[MMAX_LINE] = {0};

        mjob_t *CJ = NULL;

        if (MJobFind(ArgString,&CJ,mjsmBasic) == SUCCESS)
          {
          rc = MJobJoin(J,CJ,EMsg);

          if (rc == FAILURE)
            {
            sprintf(tmpLine,"could not join job '%s' to job '%s' -- %s\n",
              J->Name,
              ArgString,
              EMsg);

            MStringAppend(&Response,tmpLine);
            }
          else
            {
            sprintf(tmpLine,"successfully joined job '%s' to job '%s'\n",
              ArgString,
              J->Name);

            MStringAppend(&Response,tmpLine);
            }
          }
        else
          {
          sprintf(tmpLine,"invalid job '%s'\n",
            ArgString);

          MStringAppend(&Response,tmpLine);
          }
        }   /* END case mjcmJoin */

        break;

      case mjcmMigrate:

        /* NOTE: inside job loop */

        rc = __MUIJobMigrate(J,Auth,S,RE);

        if (rc == SUCCESS)
          {
          TransactionSuccessful = TRUE;

          MJobTransition(J,FALSE,FALSE);
          }

        break;

      case mjcmModify:

        /* NAMI billing - Modify */

        /* The charging has to be done BEFORE the modify happens so that the
            job is charged up to the point that it was modified with the old
            XML.  The next charge duration will reflect the change */

        if (MAMNativeDoCommand(&MAM[0],
              (void *)J,
              mxoJob,
              mamnModify,
              NULL,
              NULL,
              NULL) == FAILURE)
          {
          MDB(3,fSTRUCT) MLog("WARNING:  Accounting event 'modify' failed for job '%s'\n",
            J->Name);
          }

        rc = MUIJobModify(J,Auth,AFlags,S,RE);

        if (rc == SUCCESS)
          {
          if (MSched.AdminEventAggregationTime >= 0)
            MUIDeadLine = MIN(MUIDeadLine,(long)MSched.Time + MSched.AdminEventAggregationTime);

          TransactionSuccessful = TRUE;

          bmset(&J->IFlags,mjifShouldCheckpoint);

          MJobTransition(J,FALSE,FALSE);
          }  /* END if (rc == SUCCESS) */

        break;

      /* NOTE: mjcmQuery handled above via MUIJobQuery() */

      case mjcmCheckpoint:
      case mjcmSuspend:
      case mjcmRequeue:

        {
        enum MPreemptPolicyEnum PPolicy;

        /* NOTE: inside job loop */

        /* figure out the preempt policy based on CIndex so we don't have 3
         * cases using the exact same code */

        switch (CIndex)
          {
          default:
          case mjcmRequeue:    PPolicy = mppRequeue;    break;
          case mjcmSuspend:    PPolicy = mppSuspend;    break;
          case mjcmCheckpoint: PPolicy = mppCheckpoint; break;
          }

        if (strstr(FlagString,"unmigrate"))
          {
          char EMsg[MMAX_LINE];

          rc = MJobUnMigrate(J,TRUE,EMsg,NULL);

          if (rc == SUCCESS)
            {
            snprintf(tmpLine,sizeof(tmpLine),"job '%s' successfully unmigrated\n",J->Name);
            }
          else
            {
            snprintf(tmpLine,sizeof(tmpLine),"job '%s' could not be unmigrated (%s)\n",J->Name,EMsg);
            }
          }
        else
          {
          rc = __MUIJobPreempt(J,ArgString,U,PPolicy,IsAdmin,Annotation,tmpLine);
          }

        MStringAppend(&Response,tmpLine);

        if (tmpLine[MAX(strlen(tmpLine) - 1,0)] != '\n')
          MStringAppend(&Response,"\n");

        if (rc == SUCCESS)
          {
          TransactionSuccessful = TRUE;

          MJobTransition(J,FALSE,FALSE);
          }
        }

        break;

      case mjcmResume:

        /* NOTE:  only preemptor or higher level admin may resume job */
        /*        ie, user may suspend and admin resume */
        /*        ie, admin may suspend and user may not resume */

        rc = __MUIJobResume(J,Auth,tmpLine);

        MStringAppend(&Response,tmpLine);

        if (rc == SUCCESS)
          {
          TransactionSuccessful = TRUE;

          MJobTransition(J,FALSE,FALSE);
          }

        break;

      case mjcmShow:

        /* NYI */

        break;

      case mjcmSignal:

        {
        mxml_t *SE;
        int     STok;

        char tmpAttr[MMAX_NAME];
        char tmpVal[MMAX_LINE];

        char tmpAction[MMAX_LINE];
        char tmpLine[MMAX_LINE];

        int  SigNo;

        tmpAction[0] = '\0';
        SigNo = -1;

        STok = -1;

        while (MS3GetSet(
            RE,
            &SE,
            &STok,
            tmpAttr,
            sizeof(tmpAttr),
            tmpVal,
            sizeof(tmpVal)) == SUCCESS)
          {
          /* FORMAT:  <X>=<Y> */

          if (!strcasecmp(tmpAttr,"signal"))
            {
            if (isdigit(tmpVal[0]))
              SigNo = (int)strtol(tmpVal,NULL,10);
            else
              MUStrCpy(tmpAction,tmpVal,sizeof(tmpAction));
 
            continue;
            }
          }

        if ((rc = MRMJobSignal(J,SigNo,tmpAction,tmpLine,NULL)) == FAILURE)
          {
          MStringAppend(&Response,tmpLine);

          if (tmpLine[MAX(strlen(tmpLine) - 1,0)] != '\n')
            MStringAppend(&Response,"\n");

          break;
          }

        sprintf(tmpLine,"signal successfully sent to job %s\n",
          J->Name);

        MStringAppend(&Response,tmpLine);

        if (rc == SUCCESS)
          {
          TransactionSuccessful = TRUE;

          MJobTransition(J,FALSE,FALSE);
          }
        }  /* END BLOCK (case mjcmSignal) */

        break;

      case mjcmStart:

        /* handle "runjob" */

        {
        mxml_t *SE;
        int     STok;

        int     aindex;

        mpar_t *SP;

        mnl_t *MNodeList[MMAX_REQ_PER_JOB];

        char tmpAttr[MMAX_NAME];
        char tmpVal[MMAX_BUFFER << 3];    /* make dynamic (FIXME) */
                                          /* must be as large as largest possible hostlist */

        char SHostList[MMAX_BUFFER << 3];  /* make dynamic (FIXME) */
        char tmpLine[MMAX_LINE];
        char EMsg[MMAX_LINE] = {0};

        char *NodeMap = NULL;

        int  reqPC;

        enum MPolicyRejectionEnum PReason;

        mreq_t *RQ;

        mbitmap_t EFlags;

        mbool_t DoClear = FALSE;

        int     JobStartSC;

        /* support explicit_partition, explicit_nodelist, ignpolicy, ignrsv, ignstate */

        /* set defaults */

        SHostList[0] = '\0';
        SP           = NULL;

        /* evaluate all 'set' attributes */

        STok = -1;

        while (MS3GetWhere(
            RE,
            &SE,     /* O */
            &STok,
            tmpAttr, /* O */
            sizeof(tmpAttr),
            tmpVal,  /* O */
            sizeof(tmpVal)) == SUCCESS)
          {
          /* FORMAT:  <X>=<Y> */

          aindex = MUGetIndexCI(tmpAttr,MJobAttr,FALSE,mjaNONE);

          if (aindex == mjaNONE)
            {
            MStringAppend(&Response,"cannot parse unknown attribute\n");

            continue;
            }

          if (tmpVal[0] == '\0')
            {
            char tmpLine[MMAX_LINE];

            snprintf(tmpLine,sizeof(tmpLine),"cannot determine value for attribute '%s' when attempting to start job\n",
              tmpAttr);

            MStringAppend(&Response,tmpLine);

            continue;
            }

          switch (aindex)
            {
            case mjaFlags:

              if (!strcmp(tmpVal,"clearnodes"))
                {
                /* clear need nodes */

                DoClear = TRUE;
                }
              else
                {
                bmfromstring(tmpVal,MJobStartFlags,&EFlags);
                }

              break;

            case mjaPAL:

              /* NOTE:  only single partition PAL currently supported */

              if (MParFind(tmpVal,&SP) == FAILURE)
                {
                snprintf(tmpLine,sizeof(tmpLine),"cannot locate requested partition %s'\n",
                  tmpVal);

                MUISAddData(S,tmpLine);

                MUArrayListFree(&JobList);

                return(FAILURE);
                }

              break;
 
            case mjaReqHostList:

              MUStrCpy(SHostList,tmpVal,sizeof(SHostList));

              break;

            default:

              /*NOT SUPPORTED */

              snprintf(tmpLine,sizeof(tmpLine),"cannot support specification of attribute '%s'\n",
                tmpAttr);

              MUISAddData(S,tmpLine);

              MUArrayListFree(&JobList);

              return(FAILURE);

              break;
            }  /* END switch (aindex) */
          }    /* END while (MXMLGetChild(RE,(char *)MSON[msonSet],&STok,&SE) == SUCCESS) */

        /* verify node is suitable to start */

        RQ = J->Req[0];  /* FIXME:  only single req + global req jobs supported */

        /* check job state/estate */

        if (J->State != mjsIdle)
          {
          snprintf(tmpLine,sizeof(tmpLine),"job '%s' is in state '%s'  (state must be idle)\n",
            J->Name,
            MJobState[J->State]);

          MUISAddData(S,tmpLine);

          MUArrayListFree(&JobList);

          return(SUCCESS);
          }

        if (J->EState != mjsIdle)
          {
          snprintf(tmpLine,sizeof(tmpLine),"job '%s' is in expected state '%s'  (expected state must be idle)\n",
            J->Name,
            MJobState[J->EState]);

          MUISAddData(S,tmpLine);

          MUArrayListFree(&JobList);

          return(SUCCESS);
          }

        if (DoClear == TRUE)
          {
          /* clear host info */

          if (!strcmp(SHostList,NONE))
            {
            sprintf(SHostList,"%d",
              J->Request.TC);
            }

          if (MRMJobModify(
                J,
                "Resource_List",
                "neednodes",
                SHostList,
                mSet,
                NULL,
                NULL,
                NULL) == FAILURE)
            {
            sprintf(tmpLine,"ERROR:  cannot set hostlist for job '%s' to '%s'\n",
              J->Name,
              SHostList);

            MUISAddData(S,tmpLine);

            MUArrayListFree(&JobList);

            return(FAILURE);
            }

          /* reset internal hostlist state */

          if (bmisset(&J->IFlags,mjifHostList))
            {
            bmunset(&J->IFlags,mjifHostList);

            MNLClear(&J->ReqHList);
            }

          sprintf(tmpLine,"INFO:  successfully set hostlist for job '%s' to '%s'\n",
            J->Name,
            SHostList);

          MUISAddData(S,tmpLine);

          MJobTransition(J,FALSE,FALSE);

          MUArrayListFree(&JobList);

          return(SUCCESS);
          }  /* END if (bmisset(&EFlags,mcmClear)) */

        if ((!bmisset(&EFlags,mjsfIgnNState)) || (SHostList[0] == '\0'))
          {
          /* check resource availability */

          if (RQ->DRes.Procs * J->Request.TC > MPar[0].ARes.Procs)
            {
            snprintf(tmpLine,sizeof(tmpLine),"ERROR: job cannot run  (insufficient idle procs in cluster:  %d needed  %d available)\n",
              J->Request.TC * RQ->DRes.Procs,
              MPar[0].ARes.Procs);

            MUISAddData(S,tmpLine);

            MUArrayListFree(&JobList);

            return(SUCCESS);
            }
          }

        if ((MSched.Mode != msmSlave) && !bmisset(&EFlags,mjsfIgnPolicies))
          {
          /* check policies */

          if (MJobCheckPolicies(
               J,
               mptHard,
               0,
               (SP != NULL) ? SP : &MPar[0],
               &PReason,
               NULL,
               MSched.Time) == FAILURE)
            {
            snprintf(tmpLine,sizeof(tmpLine),"ERROR: Job rejected by policy %s.  Use 'mjobctl -w flags=ignorepolicies <jobnum>'\n",
              MPolicyRejection[PReason]);

            MStringAppend(&Response,tmpLine);

            continue;
            }
          }    /* END if (!bmisset(&EFlags,mjsfIgnPolicies)) */

        MNodeMapInit(&NodeMap);
        MNLMultiInit(MNodeList);

        if (SHostList[0] == '\0')
          {
          int pindex;
         
          mpar_t *P;

          MDB(3,fUI) MLog("INFO:     no hostlist specified for job '%s'\n",
            J->Name);

          /* check node requirements */

          /* Start with partition 1.  Partition 0 is [ALL], and we don't 
               want to allow jobs to cross partitions */

          for (pindex = 1;pindex < MMAX_PAR;pindex++)
            {
            P = &MPar[pindex];

            if (P->Name[0] == '\1')
              continue;

            if (P->Name[0] == '\0')
              break;

            if ((SP != NULL) && (P != SP))
              continue;

            /* If an override partition was not specified with the runjob, then make
               sure that the job has access to the partition we are checking for nodes. */

            if ((SP == NULL) && 
                (pindex > 0) &&
                (bmisset(&J->PAL,pindex) == FAILURE))
              continue;

            if (!bmisset(&EFlags,mjsfIgnRsv))
              {
              if (MJobSelectMNL(
                    J,
                    P,
                    NULL,
                    MNodeList,
                    NodeMap,
                    FALSE,
                    FALSE,
                    NULL,
                    NULL,
                    NULL) == FAILURE)
                {
                continue;
                }
              }
            else
              {
              mnl_t tmpNodeList;

              int        nindex;
              int        index;

              mnode_t   *N;

              int        FNC;
              int        FTC;

              int        tmpTC;

              /* ignore policies, reservations, and QOS flags (ie, AdvRes) */

              /* get feasible nodes */

              MNLInit(&tmpNodeList);

              if (MReqGetFNL(
                    J,
                    RQ,
                    P,
                    NULL,
                    &tmpNodeList,
                    &FNC,
                    &tmpTC,
                    MMAX_TIME,
                    0,
                    NULL) == FAILURE)
                {
                MNLFree(&tmpNodeList);

                continue;
                }

              memset(NodeMap,mnmNONE,sizeof(char) * MSched.M[mxoNode]);

              nindex = 0;

              FTC = 0;

              /* check node state/estate/resources only, update nodemap */

              for (index = 0;MNLGetNodeAtIndex(&tmpNodeList,index,&N) == SUCCESS;index++)
                {
                if (((N->State != mnsIdle) && (N->State != mnsActive)) ||
                    ((N->EState != mnsIdle) && (N->EState != mnsActive)))
                  {
                  continue;
                  }
  
                tmpTC = MNodeGetTC(
                  N,
                  &N->ARes,
                  &N->CRes,
                  &N->DRes,
                  &RQ->DRes,
                  MSched.Time,
                  1,
                  NULL);

                if (tmpTC < 1)
                  continue;

                FTC += tmpTC;

                /* add node to list */

                MNLSetNodeAtIndex(MNodeList[0],nindex,N);
                MNLSetTCAtIndex(MNodeList[0],nindex,tmpTC);

                nindex++;
                }  /* END for (index) */

              MNLFree(&tmpNodeList);

              MNLTerminateAtIndex(MNodeList[0],nindex);

              if ((FTC < J->Request.TC) || (nindex < J->Request.NC))
                {
                continue;
                }
              }    /* END else (!bmisset(&EFlags,mjsfIgnRsv)) */

            /* adequate resources found, start job in first available partition */

            break;
            }      /* END for (pindex) */

          if ((pindex == MMAX_PAR) || (MPar[pindex].Name[0] == '\0'))
            {
            if (MPar[2].Name[0] == '\0')
              {
              /* only one partition exists */

              sprintf(tmpLine,"ERROR: job cannot run (available nodes do not meet requirements)\n");
              }
            else
              {
              sprintf(tmpLine,"ERROR: job cannot run (available nodes do not meet requirements in any partition)\n");
              }

            MStringAppend(&Response,tmpLine);

            continue;
            }
          }    /* END if (SHostList[0] == '\0') */ 
        else
          {
          int nindex;
          int nindex2;
          int TC;
          int MaxNodeIndex;


          char *ptr;
          char *TokPtr;

          mnode_t *N;

          /* create specified nodelist */

          nindex = 0;
          TC     = 0;
          MaxNodeIndex = -1;

          rc     = SUCCESS;

          /* convert task list to node list */

          /* FORMAT: <NODENAME>[,<NODENAME>]... */

          ptr = MUStrTok(SHostList,"+,: \t\n",&TokPtr);

          while (ptr != NULL)
            {
            if (MNodeFind(ptr,&N) != SUCCESS)
              {
              char tmpName[MMAX_NAME];

              /* append domain and try again */

              sprintf(tmpName,"%s%s",
                tmpName,
                MSched.DefaultDomain);

              if (MNodeFind(tmpName,&N) != SUCCESS)
                {
                snprintf(tmpLine,sizeof(tmpLine),"ERROR:  cannot locate node '%s' in specified nodelist\n",
                  ptr);

                MStringAppend(&Response,tmpLine);

                rc = FAILURE;

                break;
                }
              }    /* END if (MNodeFind() != SUCCESS) */

            if (rc == FAILURE)
              {
              continue;
              }

            /* must check previous nodes added */

            nindex = -1;

            for (nindex2 = 0; nindex2 <= MaxNodeIndex; nindex2++)
              {
              if (N == MNLReturnNodeAtIndex(MNodeList[0],nindex2))
                {
                nindex = nindex2;

                break;
                }
              }

            if (nindex == -1)
              {
              nindex = MaxNodeIndex + 1;
              }

            if (N == MSched.GN)
              {
              MNLSetNodeAtIndex(MNodeList[1],0,N);
              MNLAddTCAtIndex(MNodeList[1],0,1);
              }
            else
              {
              NodeMap[N->Index] = mnmRequired;
              MNLSetNodeAtIndex(MNodeList[0],nindex,N);
              MNLAddTCAtIndex(MNodeList[0],nindex,1);
              }

            TC++;

            MaxNodeIndex = MAX(MaxNodeIndex,nindex);

            ptr = MUStrTok(NULL,"+,: \t\n",&TokPtr);
            }  /* END while (ptr != NULL) */

          if (TC < J->Request.TC)
            {
            if ((J->TotalTaskCount != 0) && (J->Request.TC != J->Request.NC))
              {
              /*
               * In the grid, the Master may send the slave a hostlist with just the nodes
               * to use, but without taking into account the task count. In this case
               * we have to fiddle with the task counts to pass the MJobAllocMNL() routine
               * call below. We also assume that the resource manager will give us back
               * corrected task counts in the next workload query.
               *
               * This situation does not occur when a job is first started since the requested
               * taskcount always matches the node count until the next workload query
               * after the job becomes active. 
               *
               * If a job is deferred and we attempt to start the job again after the job
               * taskcount values have been changed as a result of a workload query we have
               * to readjust the taskcount values to match the incomplete hostlist sent 
               * by the master in MMJobStart(). 
               * 
               * This if statement should be removed when the MMJobStart() code is changed
               * to send a correct hostlist.
               *
               * See Ticket 2595
               */

              J->Request.TC = TC;
              J->Req[0]->TaskCount = TC;
              }
            else
              {
              sprintf(tmpLine,"ERROR:  incorrect number of tasks in hostlist: (%d requested  %d specified)\n",
                J->Request.TC,
                TC);

              MStringAppend(&Response,tmpLine);

              continue;
              }
            }

          MNLTerminateAtIndex(MNodeList[0],nindex + 1);
          }  /* END else (strcmp(SHostList,NONE)) */

        if (ISMASTERSLAVEREQUEST(PR,MSched.Mode))
          {
          /* master has already determined nodelist. */ 
          MNLCopy(&J->Req[0]->NodeList,MNodeList[0]);

          /* Assuming that global req is on Req[1].
           * multi-req grid jobs, other than a second global req, are not
           * supported with grids, especially those those that originiate
           * on the slave. multi-req jobs would work if they orignated on
           * the master as all reqs are condensed into one. */
          if ((J->Req[1] != NULL) && 
              (MReqIsGlobalOnly(J->Req[1]) == TRUE) && 
              (!MNLIsEmpty(MNodeList[1]) == TRUE))
            {
            MNLCopy(&J->Req[1]->NodeList,MNodeList[1]);
            }
          }
        else if (MJobAllocMNL(
                  J,
                  MNodeList,  /* I */
                  NodeMap,
                  NULL,
                  MPar[RQ->PtIndex].NAllocPolicy,
                  MSched.Time,
                  NULL,
                  EMsg) == FAILURE)
          {
          MDB(2,fUI) MLog("INFO:     cannot allocate nodes for job '%s' - %s\n",
            J->Name,
            EMsg);

          snprintf(tmpLine,sizeof(tmpLine),"ERROR:    cannot allocate nodes for job '%s' - %s\n",
            J->Name,
            EMsg);

          MStringAppend(&Response,tmpLine);

          MUFree(&NodeMap);
          MNLMultiFree(MNodeList);

          continue;
          }

        MUFree(&NodeMap);
        MNLMultiFree(MNodeList);

        reqPC = RQ->DRes.Procs * J->Request.TC;

        if (MJobStart(J,EMsg,&JobStartSC,"job started by admin") == FAILURE) 
          {
          MDB(2,fUI) MLog("INFO:     job '%s' cannot be started by user %s - %s\n",
            J->Name,
            Auth,
            EMsg);

          snprintf(tmpLine,sizeof(tmpLine),"job '%s' cannot be started on %d proc%s (%s)\n",
            J->Name,
            reqPC,
            (reqPC == 1) ? "" : "s",
            EMsg);

          MStringAppend(&Response,tmpLine);

          continue;
          }

        MUIQRemoveJob(J);

        MDB(2,fUI) MLog("INFO:     job '%s' started by user %s\n",
          J->Name,
          Auth);

        sprintf(tmpLine,"job '%s' started on %d proc%s\n",
          J->Name,
          reqPC,
          (reqPC == 1) ? "" : "s");

        MStringAppend(&Response,tmpLine);

        TransactionSuccessful = TRUE;

        MJobTransition(J,FALSE,FALSE);

        if (MSched.Mode == msmNormal)
          {
          MOSSyslog(LOG_INFO,"job %s started manually",
            J->Name);
          }
        }          /* END BLOCK */

        break;

      case mjcmSubmit:

        /* NYI */

        break;

      default:

        /* NO-OP */

        break;
      }  /* END switch (CIndex) */
    }    /* END for (jindex) */

  if (((CIndex == mjcmCancel) || (CIndex == mjcmTerminate)) && 
      (MasterJobs.NumItems > 0))
    {
    char EMsg[MMAX_LINE];
    char tmpName[MMAX_NAME];

    for (jindex = 0; jindex < MasterJobs.NumItems; jindex++)
      {
      J = (mjob_t *)MUArrayListGetPtr(&MasterJobs,jindex);

      MUStrCpy(tmpName,J->Name,sizeof(tmpName));

      bmset(&J->IFlags,mjifWasCanceled);

      if (MJobArrayIsFinished(J) == FALSE)
        {
        sprintf(tmpLine,"array master job '%s' will be cleaned up when all subjobs exit\n",
          tmpName);

        MStringAppend(&Response,tmpLine);
        }
      else if (MJobCancelWithAuth(Auth,J,Annotation,FALSE,EMsg,NULL) == SUCCESS)
        {
        MDB(3,fUI) MLog("INFO:     job '%s' cancelled by user %s\n",
          tmpName,
          Auth);

        sprintf(tmpLine,"job '%s' cancelled\n",
          tmpName);

        MStringAppend(&Response,tmpLine);
        }
      else
        {
        MDB(3,fUI) MLog("ALERT:    cannot cancel job '%s' (%s)\n",
          J->Name,
          EMsg);

        sprintf(tmpLine,"ERROR:  cannot cancel job '%s' (%s)\n",
          J->Name,
          EMsg);

        MStringAppend(&Response,tmpLine);
        }
      }  /* END for (jindex) */
    }   /* END if ((cindex == mjcmCancel) && (MasterJobs.NumItems > 0)) */

  MUISAddData(S,Response.c_str());

  MUArrayListFree(&JobList);

  MUArrayListFree(&MasterJobs);

  if (TransactionSuccessful == FALSE)
    {
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MUIJobCtl() */

/* END MUIJobCtl.c */
