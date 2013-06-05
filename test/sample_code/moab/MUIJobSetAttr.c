/* HEADER */

/**
 * @file MUIJobSetAttr.c
 *
 * Contains MUI Job Set Attribute function
 */



#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  





/**
 * Remove a job partially submitted through MUIJobSubmit().
 *
 * Jobs submitted through msub may be added to the cache and database before
 * they have been legitimately reviewed.  These jobs have to be properly
 * removed from the cache and database before they are removed.  This routine
 * performs those steps.  It should only be called from MUIJobSubmit().
 *
 * @param JP
 */

int __MUIJobSubmitRemove(

  mjob_t **JP)

  {
  mjob_t *J = NULL;

  if (JP == NULL)
    {
    return(FAILURE);
    }
  
  J = *JP;

  J->State = mjsRemoved;

  MJobTransition(J,TRUE,FALSE);

  MJobRemove(JP);

  return(SUCCESS);
  }  /* END __MUIJobSubmitRemove() */





/**
 * Perform job attribute modification per 'mjobctl -m' request.
 *
 * @see __MUIJobModify() - parent
 *
 * @param J (I)
 * @param AIndex (I)
 * @param SubAttr (I) [required, can be zero-length]
 * @param Val (I) [required]
 * @param CFlags (I) [bitmap of enum MRoleEnum]
 * @param FlagString (I) [one of set, unset, add, incr, or decr]
 * @param ArgString (I)
 * @param AName (I) - requestor
 * @param Msg (O) [required,minsize=MMAX_LINE]
 */

int __MUIJobSetAttr(

  mjob_t    *J,
  enum MJobAttrEnum AIndex,
  char      *SubAttr,
  char      *Val,
  mbitmap_t *CFlags,
  char      *FlagString,
  char      *ArgString,
  char      *AName,
  char      *Msg)

  {
  int MBufSize = MMAX_LINE;

  char WarnBuffer[MMAX_LINE];

  /* NOTE: __MUIJobSetAttr() assumes caller already verified that *
           AName is an admin or owner of the job */
  
  enum MObjectSetModeEnum Mode;
  
  mbool_t JobManager = FALSE;  /* manager (not admin) */
  mbool_t Admin = FALSE;       /* admin */
  mgcred_t *U  = NULL;
  char tEmsg[MMAX_LINE];

  WarnBuffer[0] = '\0';

  if ((J == NULL) || (Msg == NULL))
    {
    if (Msg != NULL)
      sprintf(Msg,"ERROR:  internal command failure");

    return(FAILURE);
    }

  MUserAdd(AName,&U);

  if (MUICheckAuthorization(
        U,
        NULL,
        (void *)J,
        mxoJob,
        mcsMJobCtl,
        mccmModify,
        &Admin,
        tEmsg,  /* O */
        sizeof(tEmsg)) == FAILURE)
    {
    MDB(3,fSTRUCT) MLog("request is not authorized (%s)\n", tEmsg);

    sprintf(Msg, "Error: request is not authorized (%s)\n", tEmsg);

    return(FAILURE);
    }

  if ((J->Credential.A != NULL) && 
      (J->Credential.A->F.ManagerU != NULL) &&
      (MCredFindManager(AName,J->Credential.A->F.ManagerU,NULL) == SUCCESS))
    {
    JobManager = TRUE;
    }
  else if ((J->Credential.C != NULL) &&
           (J->Credential.C->F.ManagerU != NULL) &&
           (MCredFindManager(AName,J->Credential.C->F.ManagerU,NULL) == SUCCESS))
    {
    JobManager = TRUE;
    }
  else if (MVCUserHasAccessToObject(U,(void *)J,mxoJob) == TRUE)
    {
    JobManager = TRUE;
    }

  if ((JobManager == FALSE) && (Admin == FALSE) && strcmp(J->Credential.U->Name,AName))
    {
    sprintf(Msg,"ERROR:  request not authorized - not job owner");

    return(FAILURE);
    }

  if (strstr(FlagString,"decr") || strstr(FlagString,"unset"))
    {
    Mode = mUnset;
    }
  else if (strstr(FlagString,"incr") || strstr(FlagString,"add"))
    {
    Mode = mAdd;
    }
  else
    {
    Mode = mSet;
    }

  Msg[0] = '\0';

  switch (AIndex)
    {
    case mjaAccount:

      {
      int SC;
      mgcred_t *JA = NULL;

      char tmpEMsg[MMAX_LINE] = {0};

      if (MJOBISACTIVE(J) == TRUE)
        {
        sprintf(Msg,"ERROR:  cannot modify active job's account");

        return(FAILURE);
        }

      if ((Admin == FALSE) && strcmp(J->Credential.U->Name,AName))
        {
        sprintf(Msg,"ERROR:  request not authorized");

        return(FAILURE);
        }

      if ((MSched.EnforceAccountAccess == TRUE) && (MAcctFind(Val,&JA) == FAILURE))
        {
        snprintf(Msg,MMAX_LINE,"ERROR:  cannot set job account to %s, account does not exist",
          Val);

        return(FAILURE);
        }

      if ((Admin == FALSE) && MCredCheckAcctAccess(J->Credential.U,J->Credential.G,JA) == FAILURE)
        {
        snprintf(Msg,MMAX_LINE,"ERROR:  access denied to account '%s'",
          Val);

        return(FAILURE);
        }

      if ((Admin == FALSE) && (MAcctFind(Val,&JA) == TRUE))
        {
        /* Check if account has access to user-specified partition list.  If 
           there is at least one partition that is in both the specified
           partition list and the account partition list, then the account
           change will be allowed. */

        int ParIndex;
        mbool_t FSPassed;
        mbitmap_t oldPAL;
        mbitmap_t newPAL;

        bmcopy(&newPAL,&J->SpecPAL);
        bmcopy(&oldPAL,&newPAL);

        if (!bmisclear(&JA->F.PAL))
          {
          /* Account has a PAL, must check partitions */

          bmand(&newPAL,&JA->F.PAL);

          if (bmisclear(&newPAL))
            {
            snprintf(Msg,MMAX_LINE,"ERROR:  account '%s' does not have access to required partition(s)",
              Val);
    
            return(FAILURE);
            }
          }

        /* Also check partition fairshare trees */

        for (ParIndex = 1;ParIndex < MMAX_PAR;ParIndex++)
          {
          FSPassed = FALSE;
 
         if ((MPar[ParIndex].Name == NULL) ||
              (MPar[ParIndex].Name[0] == '\0'))
            break;

          if (bmisset(&oldPAL,MPar[ParIndex].Index))
            {
            if (MJobCheckFSTreeAccess(J,&MPar[ParIndex],JA,NULL,NULL,NULL,NULL) == SUCCESS)
              {
              FSPassed = TRUE;

              if (MSched.FSTreeIsRequired == TRUE)
                {
                mgcred_t *tmpA;
 
                /* save current account information to restore after we check new account */

                tmpA = J->Credential.A;
                J->Credential.A = JA;

                if (MQOSValidateFairshare(J,J->Credential.Q) == FAILURE)
                  {
                  /* looks like we do not have access to this account in the fairshare tree for this partition/qos/user */

                  FSPassed = FALSE;
                  }

                /* put the original account back, note that the new account will be set below */

                J->Credential.A = tmpA;
                } /* END if MSched.FSTreeIsRequired */
              } /* END if (MJobCheckFSTreeAccess() == SUCCESS) */

            if (FSPassed == TRUE)
              {
              /* we found a valid partition so we are done looping */

              break;
              }
            }
          } /* END for (ParIndex) */ 

        if (FSPassed == FALSE)
          {
          snprintf(Msg,MMAX_LINE,"ERROR:  account '%s' does not have access to required partition(s)",
              Val);

          return(FAILURE);
          }
        }

      if (MJobSetAttr(J,mjaAccount,(void **)Val,mdfString,mSet) == FAILURE)
        {
        if (Val[0] == '\0')
          {
          /* disallow the empty string */

          snprintf(Msg,MMAX_LINE,"ERROR:  Job acconts cannot be set with an empty string");
          }
        else
          {
          snprintf(Msg,MMAX_LINE,"ERROR:  cannot set job account to %s",
            Val);
          }

        return(FAILURE);
        }
      else if (!strcmp(Val,"NONE"))
        {
        /* NONE clears the account information */

        snprintf(Msg,MMAX_LINE,"INFO:   Account for job %s has been cleared",
            J->Name);
        }

      if (MRMJobModify(J,(char *)MJobAttr[mjaAccount],NULL,Val,mSet,NULL,tmpEMsg,&SC) == FAILURE)
        {

        snprintf(Msg,MMAX_LINE,"ERROR:  cannot set job account to %s via RM (%s)",Val,tmpEMsg);

        return(FAILURE);
        }

      MJobGetFSTree(J,NULL,TRUE);
      }    /* END BLOCK */

      break;

    case mjaAllocNodeList:

      if (MJOBISACTIVE(J) == FALSE)
        {
        snprintf(Msg,MMAX_LINE,"ERROR:  cannot adjust allocated nodes for non-active jobs");

        return(FAILURE);
        }

      snprintf(Msg,MMAX_LINE,"ERROR:  cannot adjust resources for active jobs");

      return(FAILURE);

      break;

    case mjaArrayLimit: /* change the limit of running jobs on an array */

      if (J->Array != NULL)
        {
        J->Array->Limit = atoi(Val);
        }
      else
        {
        /* If it is an array subjob return now rather than print anything. */

        if (bmisset(&J->Flags,mjfArrayJob))
          return(SUCCESS);

        /* can only modify the limit on the array master */

        sprintf(Msg,"Unable to modify array limit for '%s' -- not an array master.\n",
            J->Name);
        }

      break;

    case mjaClass:

      /* NOTE:  should allow class modification by job owner if all criteria are satisfied */

      {
      mclass_t *C;
      char      tEMsg[MMAX_LINE] = {0};

      if (MClassFind(Val,&C) == FAILURE)
        {
        snprintf(Msg,MMAX_LINE,"ERROR:  cannot locate class '%s'",
          Val);

        return(FAILURE);
        }

      if ((Admin == FALSE) && (C->NoSubmit == TRUE))
        {
        snprintf(Msg,MMAX_LINE,"ERROR:  job submission disabled for class '%s'",
          Val);

        return(FAILURE);
        }

      /* NOTE:  if we reach here, requestor is either job manager or job owner */

      if ((JobManager == FALSE) && (Admin == FALSE))
        {
        if (MJobCheckClassJLimits(
              J,
              C,
              FALSE,
              0,
              tEMsg,  /* O */
              sizeof(tEMsg)) == FAILURE)
          {
          /* job does not meet class constraints */

          sprintf(Msg,"job violates class constraints - %s",
            tEMsg);

          return(FAILURE);
          }
        }

      if (MJobSetClass(J,C,TRUE,tEMsg) == FAILURE)
        {
        snprintf(Msg,MMAX_LINE,"ERROR:  cannot set class to %s - %s",
          C->Name,
          tEMsg);

        return(FAILURE);
        }

      /* NOTE:  should unset attributes associated with previous class (NYI) */

      MJobApplyClassDefaults(J);

      sprintf(Msg,"class/queue set to %s for job %s\n",
        C->Name,
        J->Name);
      }    /* END BLOCK (case mjaClass) */

      break;

    case mjaDepend:

      {
      char *JobIdPtr;
      char *NArg;
      char *TokPtr;
      char  tmpLine[MMAX_LINE];
      char  RMArgs[MMAX_LINE];
      char  CJobBuf[MMAX_LINE];

      enum MDependEnum DependType = mdNONE;
      enum MRMTypeEnum RmType = mrmtNONE;
  
      CJobBuf[0] = '\0';

      if (J->DestinationRM != NULL)
        RmType = J->DestinationRM->Type;
  
      if (MJOBISACTIVE(J) == TRUE)
        {
        snprintf(Msg,MMAX_LINE,"ERROR: job %s is active - cannot modify dependency\n",
          J->Name);

        return(FAILURE);
        }

      if (Mode == mSet)
        {
        /* remove any existing dependencies */
  
        MJobDependDestroy(J);
        } /* END Mode != mAdd */
  
      MUStrCpy(tmpLine,Val,MMAX_LINE);
  
      NArg = MUStrTok(tmpLine,":",&TokPtr);

      RMArgs[0] = '\0';

      while (NArg != NULL)
        {
        mjob_t *tmpJ = NULL;

        /* Check if we are in the format "jobcomplete:<jobid>" get a pointer to the jobid */

        int rc = MJobParseDependType(NArg,RmType,mdJobCompletion,&DependType);

        if (rc == SUCCESS)
          NArg = MUStrTok(NULL,":",&TokPtr);
  
        JobIdPtr = NArg;
  
        if (JobIdPtr == NULL)
          {
          snprintf(Msg,MMAX_LINE,"ERROR:  incorrect dependency format: %s",
              Val);
  
          return(FAILURE);
          }
  
        if (Mode == mDecr || Mode == mUnset)
          {
          rc = MJobRemoveDependency(J,DependType,JobIdPtr,JobIdPtr,0);

          if (rc == SUCCESS)
            {
            snprintf(Msg,MMAX_LINE,"successfully unset dependency on job %s\n",
              J->Name);
            }
          else
            {
            snprintf(Msg,MMAX_LINE,"could not unset dependency on job %s\n",
              J->Name);
            }
          }
        else
          {
          if ((!strcasecmp(JobIdPtr,"none")) ||
              ((RmType == mrmtWiki) && (!strcasecmp(JobIdPtr,"0"))))    /* SLURM jobid "0" is the same as "none" */
            {
            DependType = mdNONE;
            rc = SUCCESS;
      
            if (RmType == mrmtMoab)
              {
              Mode = mSet;
              }
            }
          else 
            {
            bmset(&J->IFlags,mjifSubmitEval);
            rc = MJobSetDependency(J,DependType,JobIdPtr,JobIdPtr,0,TRUE,Msg);
            Mode = mAdd;
            bmunset(&J->IFlags,mjifSubmitEval);
            }
          }

        if (rc == FAILURE)
          {
          if(Msg[0] == '\0')
            {
            snprintf(Msg,MMAX_LINE,"ERROR:  specified job '%s' does not exist/is invalid",
              JobIdPtr);
            }

          return(FAILURE);
          }
        else if ((MUStrStr(FlagString,"warnifcomplete",0,TRUE,FALSE) != NULL) &&
                 (MJobCFind(JobIdPtr,NULL,mjsmJobName) != FAILURE))
          {
          /* if flag string indicates we should warn if there are completed jobs in the 
             dependency list then add this job to list of completed jobs */

          if (CJobBuf[0] != '\0')
            strcat(CJobBuf,",");

          strcat(CJobBuf,JobIdPtr);
          }

        /* If the resource manager is SLURM then modify the job in SLURM */

        if (RmType == mrmtWiki)
          {
          if ((tmpJ != NULL) && (tmpJ->DRMJID != NULL))
            {
            JobIdPtr = tmpJ->DRMJID; /* Use the resource manager Job Id if available */
                                                                            }
            /* ArgString sent to Wiki is in the format "Type:<jobid>[,Type:<jobid>]" */
            if (RMArgs[0] != '\0')
              strcat(RMArgs,",");

            if (!strcasecmp(JobIdPtr,"none")) /* SLURM expects a jobid of 0 to specify none */
              {
              /* no depend type for the "none" case */

              strcat(RMArgs,"0");
              }
            else
              {
              strcat(RMArgs,SDependType[DependType]);
              strcat(RMArgs,":");
              strcat(RMArgs,JobIdPtr);
              }
            }  /* END if (RmType == mrmtWiki) */

        NArg = MUStrTok(NULL,":",&TokPtr);
        }    /* END while (NArg != NULL) */

      /* if there were completed jobs in the dependency list (and flags indicated we
         should warn for this condition) then create the warning string which will 
         be added to the response at the end of this routine. */

      if (CJobBuf[0] != '\0')
        {
        snprintf(WarnBuffer,MMAX_LINE,"WARNING:  job(s) '%s' already completed\n",
            CJobBuf);
        }

      if (((RmType == mrmtPBS) || (RmType == mrmtMoab)) && 
          (J->Depend != NULL))
        {
        mstring_t DependString(MMAX_LINE);

        MDependToString(&DependString,J->Depend,mrmtPBS);

        if (RmType == mrmtPBS)
          {
          MRMJobModify(
            J,
            "Resource_List",
            "depend",
            DependString.c_str(),
            mSet,
            NULL,
            NULL,
            NULL);
          }

        if (RmType == mrmtMoab)
          {
          MRMJobModify(
            J,
            (char *)MJobAttr[mjaDepend],
            "depend",
            DependString.c_str(),
            mSet,
            NULL,
            NULL,
            NULL);
          }
        }
      else if ((RmType == mrmtWiki) && (RMArgs[0] != '\0'))
        {
        MRMJobModify(
          J,
          (char *)MJobAttr[mjaDepend],
          NULL,
          RMArgs,
          mSet,
          NULL,
          NULL,
          NULL);
        }

      /* if the job has a reservation and the dependency is not satified then release the reservation */

      if ((J->Rsv != NULL) &&
          (MJobCheckDependency(J,NULL,FALSE,NULL,NULL) == FALSE))
        {
        MJobReleaseRsv(J,TRUE,TRUE);

        snprintf(Msg,MMAX_LINE,"INFO:   Depend for job %s set to %s - job reservation released\n",
          J->Name,
          Val);
        }

      }  /* END BLOCK (case mjaDepend) */ 

      break;

    case mjaEEWDuration:

      if (Mode != mSet)
        {
        snprintf(Msg,sizeof(Msg),
            "The %s operation is not supported for attribute %s",
            MObjOpType[Mode],
            MJobAttr[AIndex]);
        return(FAILURE);
        }

      if (Admin == FALSE)
        {
        sprintf(Msg,"ERROR:  request not authorized");

        return(FAILURE);
        }

        {
        long tmpL;
   
        tmpL = MUTimeFromString((char *)Val);
   
        if (tmpL < 0)
          {
          /* reset so job submit time is effective time */
   
          J->EffQueueDuration = MSched.Time - J->SubmitTime;
          J->SystemQueueTime  = J->SubmitTime;
   
          sprintf(Msg,"effective queue time reset for job %s\n",
            J->Name);
          }
        else
          {
          J->EffQueueDuration = tmpL;
          J->SystemQueueTime = MAX(J->SubmitTime,MSched.Time - tmpL);
   
          sprintf(Msg,"effective queue time specified for job %s\n",
            J->Name);
          }
        }   /* END BLOCK */

      break;

    /* NOTE: mjaEnv handled in parent */

    case mjaFlags:

      {
      char  tmpRsv[MMAX_NAME];

      char *ptr;

      mbitmap_t tmpL;

      /* NOTE: if flag is ADVRES format is ADVRES:<rsvid> */

      tmpRsv[0] = '\0';

      if ((ptr = strstr((char *)Val,"ADVRES:")) != NULL)
        {
        int index;

        ptr += strlen("ADVRES:");

        for (index = 0;ptr[index] != '\0';index++)
          {
          if (strchr(",;| \t\n",ptr[index]))
            break;

          tmpRsv[index] = ptr[index];
          }

        tmpRsv[index] = '\0';
        }
         
      bmfromstring((char *)Val,MJobFlags,&tmpL," \t\n,:|");

      if (bmisclear(&tmpL))
        {
        sprintf(Msg,"ERROR:  unknown flag '%s'\n",
          (char *)Val);

        return(FAILURE);
        }
      
      if ((JobManager == FALSE) && (Admin == FALSE))
        {
        /* verify flag modify request only impacts non-privileged flags */

        if (MJobApproveFlags(
             J,
             (Mode == mSet) ? TRUE : FALSE,
             &tmpL) == FAILURE)
          {
          sprintf(Msg,"ERROR:  request not authorized");

          return(FAILURE);
          }   /* END if (MJobApproveFlags() == FAILURE) */
        }

      if (MJobSetAttr(J,AIndex,(void **)Val,mdfString,Mode) == SUCCESS)
        {
          if ((Admin == TRUE) &&
              bmisset(&J->SpecFlags,mjfIgnPolicies) &&
              !bmisset(&J->Flags,mjfIgnPolicies))
            {
            /* if admin setting ignpolicies then set a flag to remember admin set the ignpolicies flag */

            bmset(&J->SpecFlags,mjfAdminSetIgnPolicies);
            }

          /* request is approved, set job flags immediately */

          /* only 'OR' flags for mode mAdd */
        if (Mode != mAdd)
          {
          bmcopy(&J->Flags,&J->SpecFlags);
          }
        else
          {
          bmor(&J->Flags,&J->SpecFlags);
          }

        if (tmpRsv[0] == '\0')
          MUFree(&J->ReqRID);
        else 
          MUStrDup(&J->ReqRID,tmpRsv);
        }

      sprintf(Msg,"flags modified for job %s\n",
        J->Name);
      }     /* END (case mjaFlags) */

      break;

    case mjaHold:

      {
      mbitmap_t hbm;
      mbitmap_t OrigHold;

      /* get hold type */

      if (!strcasecmp(Val,"all"))
        {
        /* hold type NOT specified */

        if (Admin == TRUE)
          {
          bmset(&hbm,mhUser);
          bmset(&hbm,mhDefer);
          bmset(&hbm,mhSystem);
          bmset(&hbm,mhBatch);
          }
        else 
          {
          /* non-admin only authorized to adjust user hold on jobs */

          bmset(&hbm,mhUser);

          if ((Mode == mUnset) && !bmisset(&J->Hold,mhUser))
            {
            sprintf(Msg,"ERROR:  only admins can clear non-user job holds");

            return(FAILURE);
            }
          }
        }
      else
        {
        /* hold type(s) explicitly specified */

        if (bmfromstring(Val,MHoldType,&hbm) == FAILURE)
          {
          sprintf(Msg,"ERROR:  invalid hold type specified");

          return(FAILURE);
          }
        }

      /* authorize request */

      if (Admin == TRUE)
        {
        /* admin authority available */
        }
      else if (bmisset(&hbm,mhUser))
        {
        /* jobowner can adjust user holds only */
        }
      else
        {
        sprintf(Msg,"ERROR:  request not authorized");

        return(FAILURE);
        }

      /* adjust holds */

      if ((J->DestinationRM != NULL) && (J->DestinationRM->Type == mrmtMoab))
        {
        /* send job modifications to remote peer */

        /* NYI */
        }

      if (bmisset(&hbm,mhUser) || bmisset(&hbm,mhSystem))
        {
        /* Check to see if the rm type is mrmtPBS. */ 
        /* Note that we only check the SRM type if the DRM is not available. */
        /* XT setup talks directly to torque when modifying a job. */

        if (((J->DestinationRM != NULL) && ((J->DestinationRM->Type == mrmtPBS) || (J->DestinationRM->SubType == mrmstXT4))) ||
            ((J->DestinationRM == NULL) && (J->SubmitRM != NULL) && (J->SubmitRM->Type == mrmtPBS)))
          {
          const char *typeStr = NULL;
  
          if (Mode == mSet)
            {
            if (bmisset(&hbm,mhUser))
              {
              typeStr = "user";
              }
            else /* must be system */
              {
              typeStr = "system";
              }
            }

          if ((J->TemplateExtensions == NULL) ||
              (J->TemplateExtensions->WorkloadRM == NULL) ||
              (J->TemplateExtensions->WorkloadRM->Type == mrmtPBS))
            {
            /* modify if J->TX is NULL or if J->TX->WorkloadRM is NULL or if
             * the TX is type PBS */

            MRMJobModify(J,"hold",NULL,typeStr,mSet,NULL,NULL,NULL);
            }
          } /* END if DRM or SRM type is mrmtPBS */
        } /* END if mhUser or mhSystem*/

      if (bmisset(&hbm,mhBatch) && (strstr(FlagString,"unset") == NULL))
        {
        MOReportEvent(J,J->Name,mxoJob,mttHold,MSched.Time,TRUE);
        }

      bmcopy(&OrigHold,&J->Hold);

      /* NOTE:  user/admin should be able to apply message (NYI) */

      if (strstr(FlagString,"unset") == NULL)
        {
        char tmpLine[MMAX_LINE];
        char HoldLine[MMAX_LINE];

        char emptyString[] = {0};
        char *str;

        /* NOTE:  MJobSetHold allows hold message to be set, adjusts eligible
                  queue time */

        str =  bmtostring(&hbm,MHoldType,HoldLine);
        if (NULL == str)
          str = emptyString;

        snprintf(tmpLine,sizeof(tmpLine),"hold(s) '%s' set by user %s",
          str,
          AName);

        if (bmisset(&hbm,mhUser))
          MJobSetHold(J,mhUser,-1,mhrAdmin,tmpLine);

        if (bmisset(&hbm,mhBatch))
          MJobSetHold(J,mhBatch,-1,mhrAdmin,tmpLine);

        if (bmisset(&hbm,mhSystem))
          MJobSetHold(J,mhSystem,-1,mhrAdmin,tmpLine);

        if (bmisset(&hbm,mhDefer))
          MJobSetHold(J,mhDefer,-1,mhrAdmin,tmpLine);
        }
      else
        {
        MJobSetAttr(J,mjaHold,(void **)&hbm,mdfOther,mUnset);

        if (bmisclear(&J->Hold))
          {
          J->HoldReason = mhrNONE;
          J->HoldTime = 0;

          /* unset hold messages - */

          MMBRemoveMessage(&J->MessageBuffer,NULL,mmbtHold); 
          }
        }

      if (bmcompare(&J->Hold,&OrigHold) != 0)
        {
        if (!bmisclear(&J->Hold))
          {
          char tmpHolds[MMAX_LINE];

          bmtostring(&J->Hold,MHoldType,tmpHolds);

          sprintf(Msg,"holds modified for job %s  (%s hold%s still in place)\n",
            J->Name,
            tmpHolds,
            (strchr(tmpHolds,',') != NULL) ? "s" : "");
          }
        else
          {
          sprintf(Msg,"holds modified for job %s\n",
            J->Name);
          }

        MDB(3,fSTRUCT) MLog("INFO:     hold %s for job %s by admin\n",
          (strstr(FlagString,"unset") != NULL) ? "unset" : "set",
          J->Name);
        }  /* END if (J->Hold != OrigHold) */
      }    /* END (case mjaHold) */

      break;

    case mjaReqHostList:

      MJobSetAttr(J,mjaReqHostList,(void **)Val,mdfString,mSet);

      if (MJOBISACTIVE(J) == TRUE)
        {
        /* checkpoint and requeue */

        MRMJobCheckpoint(J,FALSE,"migrating job hostlist",NULL,NULL);

        MJobRequeue(J,NULL,"migrating job hostlist",NULL,NULL);

        sprintf(Msg,"job %s migrated to new hostlist, will re-start as soon as resources are available\n",
          J->Name);
        }
      else
        {
        sprintf(Msg,"hostlist modified for job %s\n",
          J->Name);
        }

      break;

    case mjaJobName:
      {
      char tmpEMsg[MMAX_LINE] = {0};

      if (MJobSetAttr(J,mjaJobName,(void **)Val,mdfString,mSet) == FAILURE)
        {
        sprintf(Msg,"cannot modifiy jobname for job %s\n",
          J->Name);

        return(FAILURE);
        }

      if (MRMJobModify(J,"jobname",NULL,Val,mSet,NULL,tmpEMsg,NULL) == FAILURE)
        {
        snprintf(Msg,MMAX_LINE,"ERROR:  cannot set job name to %s via RM (%s)",Val,tmpEMsg);

        return(FAILURE);
        }
      
      sprintf(Msg,"user-specified jobname modified for job %s\n",
        J->Name);

      } /* end case mjaJobName */
      break;

    case mjaLogLevel:
    case mjaMessages:

      MJobSetAttr(J,AIndex,(void **)Val,mdfString,mSet);

      sprintf(Msg,"%s modified for job %s\n",
        MJobAttr[AIndex],
        J->Name);

      break;

    case mjaNotificationAddress:

      {
      char tmpEMsg[MMAX_LINE] = {0};

      if (MJobSetAttr(J,AIndex,(void **)Val,mdfString,mSet) == FAILURE)
        {
        sprintf(Msg,"cannot modifiy notification address for job %s\n",J->Name);

        return(FAILURE);
        }

      if (MRMJobModify(
          J,
          (char *)MJobAttr[mjaNotificationAddress],
          NULL,
          Val,
          mSet,
          NULL,
          tmpEMsg,
          NULL) == FAILURE)

        {
        sprintf(Msg,"cannot modifiy notification address for job %s via RM (%s)\n",J->Name,tmpEMsg);

        return(FAILURE);
        }

      sprintf(Msg,"notification address modified for job %s\n",
        J->Name);

      break;
      } /* END case mjaNotificationAddress */

    case mjaPAL:

      {
      mbitmap_t tmpSpec;
      mbitmap_t tmpSys;

      bmcopy(&tmpSpec,&J->SpecPAL);
      bmcopy(&tmpSys,&J->SysPAL);

      MJobSetAttr(J,AIndex,(void **)Val,mdfString,(Mode == mUnset) ? mDecr : Mode);

      if ((Mode == mSet) && (MSched.FSTreeIsRequired == TRUE) &&
          (MQOSValidateFairshare(J,J->Credential.Q) == FAILURE))
        {
        /* fairshare fail, reset to original PAL's */

        bmcopy(&J->SpecPAL,&tmpSpec);
        bmcopy(&J->SysPAL,&tmpSys);

        MJobGetPAL(J,&J->SpecPAL,&J->SysPAL,&J->PAL);

        MDB(2,fUI) MLog("WARNING:  Fairshare does not allow specified PAL (%s)\n",
            (char *)Val);

        sprintf(Msg,"ERROR:    Fairshare does not allow specified partition(s) (%s)",
            (char *)Val);

        return(FAILURE);
        }

      break;
      }

    case mjaReqCMaxTime:

      if (Admin == FALSE)
        {
        sprintf(Msg,"ERROR:  request not authorized");

        return(FAILURE);
        }

      {
      long ReqCTime;

      ReqCTime = strtol(Val,NULL,10);

      if (Mode == mSet)
        {
        long        BestCTime;

        mbool_t     CTimeIsValid = TRUE;

        mnl_t  *MNodeList[MMAX_REQ_PER_JOB];

        long        StartTime;

        mpar_t     *P;

        int         NC;
        int         TC;

        /* validate specified time */

        /* NOTE:  active jobs are valid due to preemption */

        /* NOTE:  job QOS should control which jobs allow CMaxDate (NYI) */

        if (J->StartTime > 0)
          BestCTime = J->StartTime + J->SpecWCLimit[0];
        else
          BestCTime = MSched.Time + J->SpecWCLimit[0];

        MNLMultiInit(MNodeList);

        if (ReqCTime < BestCTime)
          {
          CTimeIsValid = FALSE;
          }
        else
          {
          /* attempt to schedule at requested ctime */

          StartTime = ReqCTime - J->SpecWCLimit[0];
          P         = NULL;

          if (MJobGetEStartTime(
              J,
              &P,
              &NC,
              &TC,
              MNodeList,
              &StartTime,
              NULL,
              TRUE,
              FALSE,
              NULL) == FAILURE)
            {
            /* cannot reserve nodes at specified time */
            /* attempt to reserve job at earlier time */

            StartTime = MSched.Time;
            P         = NULL;

            if (MJobGetEStartTime(
                 J,
                 &P,
                 &NC,
                 &TC,
                 MNodeList,
                 &StartTime,
                 NULL,
                 TRUE,
                 FALSE,
                 NULL) == FAILURE)
              {
              MDB(2,fUI) MLog("WARNING:  cannot find earliest start time for job '%s'\n",
                J->Name);

              sprintf(Msg,"ERROR:    cannot find earliest start time for job '%s'",
                J->Name);

              CTimeIsValid = FALSE;
              }
            }
          }    /* END else (ReqCTime < BestCTime) */

        if (CTimeIsValid == FALSE)
          {
          char DString[MMAX_LINE];

          /* cannot meet requested completion time */

          MULToDString((mulong *)&ReqCTime,DString);

          MDB(2,fUI) MLog("INFO:     requested deadline %s cannot be set for job %s",
            DString,
            J->Name);

          sprintf(Msg,"requested deadline %s cannot be set for job %s",
            DString,
            J->Name);

          MNLMultiFree(MNodeList);

          return(SUCCESS);
          }

        /* create ctime reservation */

        if (MRsvJCreate(J,MNodeList,StartTime,mjrDeadline,NULL,FALSE) == FAILURE)
          {
          char DString[MMAX_LINE];

          MULToDString((mulong *)&ReqCTime,DString);

          sprintf(Msg,"ERROR:  cannot enforce request completion time deadline %s for job %s",
            DString,
            J->Name);

          MNLMultiFree(MNodeList);

          return(SUCCESS);
          }

        MNLMultiFree(MNodeList);
        }    /* END if (Mode == mSet) */

      MJobSetAttr(J,AIndex,(void **)&ReqCTime,mdfLong,(enum MObjectSetModeEnum)Mode);
      }  /* END BLOCK */

      break;

    case mjaReqReservation:

      if ((Admin == FALSE) && (JobManager == FALSE))
        {
        if (strstr(FlagString,"unset") != NULL)
          {
          sprintf(Msg,"ERROR:  request not authorized");

          return(FAILURE);
          }
        }

      MJobSetAttr(J,AIndex,(void **)Val,mdfString,(strstr(FlagString,"unset") != NULL) ? mUnset : mSet);

      sprintf(Msg,"required job reservation for job %s modified\n",
        J->Name);

      break;

    case mjaReqSMinTime:

      /* NOTE:  allow user to adjust 'release time' */

      MJobSetAttr(J,AIndex,(void **)Val,mdfString,(strstr(FlagString,"unset") != NULL) ? mUnset : mSet);

      sprintf(Msg,"required minimum start time for job %s modified\n",
        J->Name);

      break;

    case mjaSize:
      {
      char tmpEMsg[MMAX_LINE] = {0};

      if (MJobSetAttr(J,mjaSize,(void **)Val,mdfString,mSet) == FAILURE)
        {
        sprintf(Msg,"cannot modifiy size for job %s via RM\n", J->Name);

        return(FAILURE);
        }

      if (MRMJobModify(
          J,
          (char *)MJobAttr[mjaSize],
          NULL,
          Val,
          mSet,
          NULL,
          tmpEMsg,
          NULL) == FAILURE)
        {
        sprintf(Msg,"cannot modifiy size for job %s via RM (%s)\n",J->Name,tmpEMsg);

        return(FAILURE);
        }

      MJobUpdateResourceCache(J,NULL,0,NULL); /* update showq proc count */

      break;
      } /* END case mjaSize */

    case mjaState:

      {
      enum MJobStateEnum JState = mjsNONE;

      int  SC = 0;
      char tmpName[MMAX_NAME];

      char EMsg[MMAX_LINE] = {0};

      if ((char *)Val != NULL)
        JState = (enum MJobStateEnum)MUGetIndexCI((char *)Val,MJobState,FALSE,0);

      if ((JState != mjsCompleted) && (JState != mjsRemoved))
        {
        sprintf(Msg,"ERROR:  can only set state to completed or removed");

        return(FAILURE);
        }

      if (Admin == FALSE)
        {
        sprintf(Msg,"ERROR:  request not allowed");

        return(FAILURE);
        }

      if (!bmisset(&J->Flags,mjfNoRMStart))
        {
        sprintf(Msg,"ERROR:  can only set state on system jobs");

        return(FAILURE);
        }

      strcpy(tmpName,J->Name);

      if (JState == mjsCompleted)
        {
        /* adjust job walltime to complete job immediately upon next iteration */

        J->SpecWCLimit[0] = MSched.Time - J->StartTime;
        J->SpecWCLimit[1] = MSched.Time - J->StartTime;

        J->WCLimit = MSched.Time - J->StartTime;

        MRsvJCreate(J,NULL,J->StartTime,mjrActiveJob,NULL,FALSE);
        }
      else if (MJobCancelWithAuth(AName,J,NULL,FALSE,EMsg,&SC) == FAILURE)
        {
        sprintf(Msg,"ERROR:  state modify request failed - %s",
          EMsg);

        return(FAILURE);
        }
      
      MDB(3,fUI) MLog("INFO:     job '%s' state set to %s by user %s\n",
        tmpName,
        MJobState[JState],
        AName);

      if (SC == mscPartial)
        {
        sprintf(Msg,"job %s state set to %s (%s)\n",
          tmpName,
          MJobState[JState],
          EMsg);
        }
      else
        {
        sprintf(Msg,"job %s %s\n",
          tmpName,
          MJobState[JState]);
        }
      }    /* END BLOCK (case mjaState) */

      break;

    case mjaSysPrio:

      if (Admin == FALSE)
        {
        sprintf(Msg,"ERROR:  request not authorized");

        return(FAILURE);
        }

      {
      long tmpPrio;

      mbool_t IsRelative = FALSE;

      tmpPrio = strtol(Val,NULL,0);

      if ((strstr(ArgString,"relative") != NULL) ||
          (strstr(FlagString,"relative") != NULL))
        {
        IsRelative = TRUE;
        }
      else if (Mode == mAdd)
        {
        IsRelative = TRUE;
        }
      else if (Mode == mUnset)
        {
        IsRelative = TRUE;

        tmpPrio *= -1;
        }

      if (IsRelative == FALSE) 
        {
        if (Admin == FALSE)
          {
          /* only admins may specify absolute priority */

          sprintf(Msg,"ERROR:  request not authorized - cannot specify absolute priority");

          return(FAILURE);
          }

        /* enforce sysprio bounds */

        if ((tmpPrio < 0) || (tmpPrio > 1000))
          {
          strcpy(Msg,"ERROR:  system priority must be in the range 0 - 1000");

          return(FAILURE);
          }
        }
      else
        {
        /* HACK:  if priority > (MMAX_PRIO_VAL + 1000), J->SystemPrio is
                  treated as relative prio adjustment */ 

        if ((Admin == FALSE) && (tmpPrio > 0))
          {
          sprintf(Msg,"ERROR:  request not authorized - cannot specify positive relative priority");

          return(FAILURE);
          }

        tmpPrio += (MMAX_PRIO_VAL << 1);
        }

      MJobSetAttr(J,mjaSysPrio,(void **)&tmpPrio,mdfLong,(strstr(FlagString,"unset") != NULL) ? mUnset : mSet);

      snprintf(Msg,MMAX_LINE,"job %s system priority modified\n",
        J->Name);

      if (MSched.Mode == msmNormal)
        {
        MOSSyslog(LOG_INFO,"system prio set to %ld on job %s",
          tmpPrio,
          J->Name);
        }
      }    /* END BLOCK (case mjaSysPrio) */

      break;

    case mjaTrigger:

      /* NOTE:  treat all operations as 'incr' for now */

      {
      marray_t TList;

      MUArrayListCreate(&TList,sizeof(mtrig_t *),10);

      MTrigLoadString(
        &J->Triggers,
        Val,
        TRUE,
        FALSE,
        mxoJob,
        J->Name,
        &TList,
        NULL);

      if (TList.NumItems > 0)
        {
        mtrig_t *T;

        /* initialize new triggers only */

        int tindex;

        for (tindex = 0; tindex < TList.NumItems;tindex++)
          {
          T = (mtrig_t *)MUArrayListGetPtr(&TList,tindex);
 
          if (MTrigIsValid(T) == FALSE)
            continue;

          MTrigInitialize(T);
          }  /* END for (tindex) */

        MOReportEvent((void *)J,J->Name,mxoJob,mttCreate,J->CTime,TRUE);

        if (J->StartTime > 0)
          MOReportEvent(J,J->Name,mxoJob,mttStart,J->StartTime,TRUE);
        }    /* END if (J->T != NULL) */

      MUArrayListFree(&TList);
      }      /* END BLOCK (case mjaTrigger) */

      break;

    case mjaVariables:

      if (MJobSetAttr(J,mjaVariables,(void **)Val,mdfString,Mode) == FAILURE)
        {
        sprintf(Msg,"cannot %s job variable",
          ((Mode == mUnset) || (Mode == mDecr)) ? "clear" : "set");

        return(FAILURE);
        }

      if ((Mode == mUnset) || (Mode == mDecr))
        {
        snprintf(Msg,MMAX_LINE,"successfully unset variables on job %s",
          J->Name);
        }
      else
        {
        snprintf(Msg,MMAX_LINE,"successfully set variables on job %s",
          J->Name);
        }

      break;

    case mjaUserPrio:

      {
      long tmpPrio;
      mbool_t unsetEnableFlag = FALSE;

      /* NOTE: all managers/owners can make changes, no authority check required */
      
      if ((Val == NULL) || (Val[0] == '\0'))
        tmpPrio = 0;
      else
        tmpPrio = strtol(Val,NULL,0);

      if ((tmpPrio == 0) && (Val != NULL) && (Val[0] != '\0'))
        {
        strcpy(Msg,"ERROR:  invalid priority specified");

        return(FAILURE);
        }

      if ((Mode == mAdd) || (strstr(ArgString,"relative") != NULL))
        tmpPrio = J->UPriority + tmpPrio;

      if ((tmpPrio < MDEF_MINUSERPRIO) || (tmpPrio > MDEF_MAXUSERPRIO))
        {
        strcpy(Msg,"ERROR:  invalid value for user priority");

        return(FAILURE);
        } 

      if (tmpPrio > 0)
        {
        if ((!bmisset(&MPar[0].Flags,mpfEnablePosUserPriority)) && 
            (Admin == FALSE))
          {
          strcpy(Msg,"ERROR:  only admins can set positive job priority");

          return(FAILURE);
          }
        else if ((!bmisset(&MPar[0].Flags,mpfEnablePosUserPriority)) && 
                 (Admin == TRUE))
          {
          /* set EnablePosUserPriority flag temporarily so that MJobSetAttr
           * won't set job's user priority to 0 for an admin request. */
        
          if (!bmisset(&MPar[0].Flags,mpfEnablePosUserPriority))
            {
            unsetEnableFlag = TRUE;
            bmset(&MPar[0].Flags,mpfEnablePosUserPriority);
            }
          }
        }

      MJobSetAttr(J,mjaUserPrio,(void **)&tmpPrio,mdfLong,(strstr(FlagString,"unset") != NULL) ? mUnset : mSet);

      if (unsetEnableFlag == TRUE)
        bmunset(&MPar[0].Flags,mpfEnablePosUserPriority);

      sprintf(Msg,"job user priority modified");
      }    /* END BLOCK (case mjaUserPrio) */

      break;

    case mjaQOS:
    case mjaQOSReq:

      {
      char QALLine[MMAX_LINE];

      /* NOTE: all managers/owners can make changes, no authority check required */

      mqos_t *Q;

      mbitmap_t QAL;

      /* reject running jobs */

      if (MJOBISACTIVE(J))
        {
        sprintf(Msg,"ERROR:  cannot modify QOS of active job");

        return(FAILURE);
        }

      if ((Val[0] == '\0') || !strcasecmp(Val,"default") || !strcasecmp(Val,"none"))
        {
        /* admin is unsetting QoS, fallback to default */

        J->QOSRequested = NULL;

        MQOSGetAccess(J,J->QOSRequested,NULL,NULL,&Q);
        }
      else if (MQOSFind(Val,&Q) == FAILURE)
        {
        sprintf(Msg,"ERROR:  invalid QOS specified");

        return(FAILURE);
        }

      /* Check if job has access to QOS or if requestor is admin (level 1 or 2)*/

      if ((MQOSGetAccess(J,Q,NULL,&QAL,NULL) == FAILURE) &&
          (Admin != TRUE))      
        {
        MDB(2,fUI) MLog("INFO:     job %s does not have access to QOS %s (QAL: %s)\n",
          J->Name,
          Q->Name,
          MQOSBMToString(&QAL,QALLine));

        sprintf(Msg,"ERROR:    job %s does not have access to QOS %s (QOSList: %s)",
          J->Name,
          Q->Name,
          MQOSBMToString(&QAL,QALLine));

        return(FAILURE);
        }

      /* Check fairshare access */

      if ((Admin != TRUE) &&
          (MSched.FSTreeIsRequired == TRUE) &&
          (MQOSValidateFairshare(J,Q) == FAILURE))
        {
        MDB(2,fUI) MLog("INFO:     job %s does not have access to QOS %s in fairshare tree (QAL: %s)\n",
          J->Name,
          Q->Name,
          MQOSBMToString(&QAL,QALLine));

        sprintf(Msg,"ERROR:    job %s does not have access to QOS %s in fairshare tree (QOSList: %s)",
          J->Name,
          Q->Name,
          MQOSBMToString(&QAL,QALLine));

        return(FAILURE);
        }

      /* set both requested and effective QOS */

      MJobReleaseRsv(J,TRUE,TRUE);

      MJobSetQOS(J,Q,0);

      MJobSetAttr(J,mjaQOSReq,(void **)Val,mdfString,mSet);

      if (MSched.Mode == msmNormal)
        {
        MOSSyslog(LOG_INFO,"QOS changed from %s to %s on job %s",
          J->Credential.Q->Name,
          Q->Name,
          J->Name);
        }

      sprintf(Msg,"job QOS modified");
      }  /* END BLOCK (case mjaQOS) */

      break;

    case mjaMinWCLimit:

      {
      mutime tmpLimit;

      if (MJOBISACTIVE(J))
        {
        sprintf(Msg,"cannot modify minwclimit on active jobs");

        return(FAILURE);
        }

      if ((Val == NULL) || (Val[0] == '\0'))
        tmpLimit = 0;
      else
        tmpLimit = MUTimeFromString(Val);

      if (tmpLimit > J->WCLimit)
          
        {
        sprintf(Msg,"cannot set minwclimit to more than total walltime limit");

        return(FAILURE);
        }

      if (tmpLimit <= 0)
        {
        sprintf(Msg,"invalid minwclimit value");

        return(FAILURE);
        }

      /* Need to release any reservation, it was based on the minwclimit */
      /* New minwclimit will affect next scheduling iteration */

      MJobReleaseRsv(J,TRUE,TRUE);

      J->MinWCLimit = tmpLimit;
      J->OrigMinWCLimit = tmpLimit;

      sprintf(Msg,"job minwclimit modified");
      }

      break;

    default:

      sprintf(Msg,"ERROR:  attribute '%s' not processed for job %s\n",
        MJobAttr[AIndex],
        J->Name);

      return(FAILURE);

      /*NOTREACHED*/

      break;
    }  /* END switch (AIndex) */

  /* return message if not already specified */

  if ((Msg[0] == '\0') && (AIndex != mjaHold))
    { 
    char *BPtr;
    int   BSpace;

    MUSNInit(&BPtr,&BSpace,Msg,MBufSize);

    MUSNPrintF(&BPtr,&BSpace,"INFO:   %s for job %s",
      MJobAttr[AIndex],
      J->Name);

    if (FlagString != NULL)
      {
      if (!strcmp(FlagString,"set"))
        {
        MUSNPrintF(&BPtr,&BSpace," set to %s\n",
          Val);
        }
      else if (!strcmp(FlagString,"incr"))
        {
        MUSNPrintF(&BPtr,&BSpace," incremented by %s\n",
          Val);
        }
      else if (!strcmp(FlagString,"decr"))
        {
        MUSNPrintF(&BPtr,&BSpace," decremented by %s\n",
          Val);
        }
      else if (!strcmp(FlagString,"unset"))
        {
        MUSNPrintF(&BPtr,&BSpace," unset\n");
        }
      else
        {
        MUSNPrintF(&BPtr,&BSpace," set to %s\n",
          Val);
        }
      }
    else
      {
      MUSNPrintF(&BPtr,&BSpace," updated\n");
      }

    if (WarnBuffer[0] != '\0')
      {
      MUSNPrintF(&BPtr,&BSpace,WarnBuffer);
      }

    }    /* END if (Msg[0] == '\0') */
  
  return(SUCCESS);
  }  /* END __MUIJobSetAttr() */
/* END MUIJobSetAttr.c */
