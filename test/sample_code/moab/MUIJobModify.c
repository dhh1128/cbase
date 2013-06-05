/* HEADER */

/**
 * @file MUIJobModify.c
 *
 * Contains MUI Job Modify function
 */



#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


int __MUIJobModifyGRes(mjob_t *,char *,enum MObjectSetModeEnum,char *);


/**
 * This MUI internal function is a handler for job modification requests. This
 * function SHOULD be invoked after the caller has already verified that Auth
 * is an admin or owner of the job. (It is usually called by MUIJobCtl().)
 * Based on the flags and XML inputs, the given job will be appropriately
 * modified. If a socket object is given, then an appropriate success/error
 * message will be attached to the socket.
 *
 * @see __MUIJobSetAttr() - child
 * @see MUIJobCtl() - parent
 *
 * @param J (I) Job to modify. [modified]
 * @param Auth Name/identifier of the user/peer making this request. (I)
 * @param CFlags (I) Specifies the ADMIN-level of the incoming request [bitmap of MRoleEnum]
 * @param S (I) Socket on which to return success/error messages. [optional,modified]
 * @param RE XML describing the requested modification. (I)
 *
 * @return Will return SUCCESS if no Set requests detected.
 */

int MUIJobModify(

  mjob_t    *J,      /* I (modified) */
  char      *Auth,   /* I */
  mbitmap_t *CFlags, /* I (bitmap of MRoleEnum) */
  msocket_t *S,      /* I (optional,modified) */
  mxml_t    *RE)     /* I */

  {
  mxml_t *SE;

  char tmpAttr[MMAX_LINE];
  char tmpVal[MMAX_LINE * 4];
  char tmpOp[MMAX_NAME];

  char ArgString[MMAX_LINE];
  char FlagString[MMAX_LINE];
  char SubAttr[MMAX_LINE];

  char Msg[MMAX_LINE];

  enum MJobAttrEnum jaindex;
  enum MReqAttrEnum rqaindex;

  int  STok;

  enum MObjectSetModeEnum Op;
  int  tmpI = 0;

  int  rc = SUCCESS;

  mbool_t SetFound = FALSE;
  mbool_t JobManager = FALSE;

  mreq_t *RQ;

  const char *FName = "MUIJobModify";

  MDB(2,fUI) MLog("%s(%s,%s,S,RE)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (Auth != NULL) ? Auth : "NULL");

  if ((J == NULL) || (J->Req[0] == NULL))
    {
    return(FAILURE);
    }

  RQ = J->Req[0];  /* FIXME */

  MXMLGetAttr(RE,MSAN[msanFlags],NULL,FlagString,sizeof(FlagString));

  MXMLGetAttr(RE,MSAN[msanArgs],NULL,ArgString,sizeof(ArgString));

  /* evaluate all 'set' attributes */

  STok = -1;

  while (MS3GetSet(
      RE,
      &SE,
      &STok,
      tmpAttr,          /* O */
      sizeof(tmpAttr),
      tmpVal,           /* O */
      sizeof(tmpVal)) == SUCCESS)
    {
    char *Name;
    char *SubName;
    SetFound = TRUE;

    if (MUParseNewStyleName(tmpAttr,&Name,&SubName) == SUCCESS)
      {
      MUStrCpy(SubAttr,SubName,sizeof(SubAttr));
      }
    else
      {
      Name = tmpAttr;
      SubAttr[0] = 0;
      }


    rqaindex = mrqaNONE;

    if ((jaindex = (enum MJobAttrEnum)MUGetIndexCI(
           Name,
           MJobAttr,
           FALSE,
           mjaNONE)) == mjaNONE)
      {
      if ((rqaindex = (enum MReqAttrEnum)MUGetIndexCI(
             Name,
             MJobAttr,
             FALSE,
             mrqaNONE)) == mrqaNONE)
        {
        /* enable job/req attr 'aliases' */

        if (!strcasecmp(Name,"partition"))
          {
          rqaindex = mrqaAllocPartition;
          }
        else if (!strcasecmp(Name,"priority"))
          {
          jaindex = mjaSysPrio;
          }
        else if (!strcasecmp(Name,"queue"))
          {
          jaindex = mjaClass;
          }
        else if (!strcasecmp(Name,"nodes"))
          {
          if (J->TotalTaskCount != 0)
            {
            /* job was not created with the TTC parameter */
            
            MUISAddData(S,"ERROR:  Cannot modify job nodes if ttc was specified at job submission.");

            return (FAILURE);
            }

          rqaindex = mrqaNCReqMin;
          }
        else if (!strcasecmp(Name,"wclimit"))
          {
          jaindex = mjaReqAWDuration;
          }
        else if (!strcasecmp(Name,"jobdisk"))
          {
          rqaindex = mrqaReqDiskPerTask;

          tmpI = (int)strtol(tmpVal,NULL,10);

          if ((tmpI > 0) || (tmpVal[0] == '\0'))
            tmpI /= MAX(1,RQ->TaskCount);
          }
        else if (!strcasecmp(Name,"jobmem"))
          {
          rqaindex = mrqaReqMemPerTask;

          tmpI = (int)strtol(tmpVal,NULL,10);

          if ((tmpI > 0) || (tmpVal[0] == '\0'))
            tmpI /= MAX(1,RQ->TaskCount);
          }
        else if (!strcasecmp(Name,"jobswap"))
          {
          rqaindex = mrqaReqSwapPerTask;

          tmpI = (int)strtol(tmpVal,NULL,10);

          if ((tmpI > 0) || (tmpVal[0] == '\0'))
            tmpI /= MAX(1,RQ->TaskCount);
          }
        else if (!strcasecmp(Name,"nodecount"))
          {
          rqaindex = mrqaNCReqMin;
          }
        else if (!strcasecmp(Name,"proccount"))
          {
          rqaindex = mrqaTCReqMin;
          }
        else if (!strcasecmp(Name,"arraylimit"))
          {
          jaindex = mjaArrayLimit;
          }
        else if (!strcasecmp(Name,"minstarttime"))
          {
          jaindex = mjaReqSMinTime;
          }
        else if (!strcasecmp(Name,"trig"))
          {
          jaindex = mjaTrigger;
          }
        else if (!strcasecmp(Name,"var"))
          {
          jaindex = mjaVariables;
          }
        else if (!strcasecmp(Name,"allocnodelist"))
          {
          jaindex = mjaAllocNodeList;
          }
        else if(!strcasecmp(Name,"features"))
          {
          rqaindex = mrqaReqNodeFeature;
          }
        else if(!strcasecmp(Name,"feature"))
          {
          rqaindex = mrqaReqNodeFeature;
          }
        else if(!strcasecmp(Name,"gres"))
          {
          rqaindex = mrqaGRes;
          }
        else if (!strcasecmp(Name,"notificationaddress"))
          {
          jaindex = mjaNotificationAddress;
          }
        else
          {
          snprintf(Msg,sizeof(Msg),"unknown attribute '%s' specified",
            Name);

          MUISAddData(S,Msg);

          continue;
          }
        }    /* END if ((rqaindex = (enum MReqAttrEnum)MUGetIndexCI() == mrqaNONE) */
      }      /* END if ((jaindex = (enum MJobAttrEnum)MUGetIndexCI() == mjaNONE) */

    Op = mNONE2;

    if (MXMLGetAttr(SE,MSAN[msanOp],NULL,tmpOp,sizeof(tmpOp)) == SUCCESS)
      {
      if (!strcmp(tmpOp,"decr"))
        {
        /* why is decr handled differently? */

        Op = mDecr;

        strncat(FlagString," unset",sizeof(FlagString) - strlen(FlagString));
        }
      else if ((Op = (enum MObjectSetModeEnum)MUGetIndexCI(tmpOp,MObjOpType,FALSE,mNONE2)) != mNONE2)
        {
        strncat(FlagString," ",sizeof(FlagString) - strlen(FlagString));
        strncat(FlagString,MObjOpType[Op],sizeof(FlagString) - strlen(FlagString));
        }
      else if (!strcmp(tmpOp,"incr"))
        {
        Op = mAdd;

        strncat(FlagString," incr",sizeof(FlagString) - strlen(FlagString));
        }
      }

    if ((ArgString[0] == '\0') && (SE->Val != NULL))
      {
      MUStrCpy(ArgString,SE->Val,sizeof(ArgString));
      }

    /* set job attributes */

    if (bmisset(CFlags,mcalAdmin1) ||
        bmisset(CFlags,mcalAdmin2))
      {
      JobManager = TRUE;
      }
    else if ((J->Credential.A != NULL) &&
             (J->Credential.A->F.ManagerU != NULL) &&
             (MCredFindManager(Auth,J->Credential.A->F.ManagerU,NULL) == SUCCESS))
      {
      JobManager = TRUE;
      }
    else if ((J->Credential.C != NULL) &&
             (J->Credential.C->F.ManagerU != NULL) &&
             (MCredFindManager(Auth,J->Credential.C->F.ManagerU,NULL) == SUCCESS))
      {
      JobManager = TRUE;
      }

    MDB(4,fUI) MLog("INFO:     requestor %s attempting to %s attribute %s of job %s (%s)\n",
      (Auth != NULL) ? Auth : "NULL",
      (tmpOp[0] != '\0') ? tmpOp : "modify",
      MJobAttr[jaindex],
      (J != NULL) ? J->Name : "NULL",
      ArgString);

    rc = FAILURE;

    switch (jaindex)
      {
      case mjaFlags:

        rc = __MUIJobSetAttr(
          J,
          jaindex,
          SubAttr,
          tmpVal,
          CFlags,
          tmpOp,
          ArgString,
          Auth,
          Msg);

        MUISAddData(S,Msg);

        break;

      case mjaReqReservation:
      case mjaAllocNodeList:
      case mjaArrayLimit:
      case mjaClass:
      case mjaDepend:
      case mjaEEWDuration:
      case mjaHold:
      case mjaReqHostList:
      case mjaLogLevel: 
      case mjaMessages:
      case mjaMinWCLimit:
      case mjaNotificationAddress:
      case mjaPAL:
      case mjaQOS:
      case mjaQOSReq:
      case mjaReqCMaxTime:
      case mjaSize:
      case mjaState:
      case mjaSysPrio:
      case mjaJobName:
      case mjaTrigger:
      case mjaVariables:

        /* __MUIJobSetAttr enforces access policies */

        rc = __MUIJobSetAttr(
          J,
          jaindex,
          SubAttr,
          tmpVal,
          CFlags,
          FlagString,
          ArgString,
          Auth,
          Msg);

        MUISAddData(S,Msg);

        break;

      /* NOTE:  we want to check to see if we need to use MMJobModify here. do
                we also want to move some of the other attributes so they
                fall through to be handled the same? */

      case mjaAccount:
          {
          if ((MJOBWASMIGRATEDTOREMOTEMOAB(J)) ||
              ((J->SystemID == NULL) && (bmisset(&J->Flags,mjfClusterLock))))
            {
            rc = MMJobModify(
                J,
                J->DestinationRM,
                "Account",
                "",
                tmpVal,
                Op,
                Msg,
                NULL);
            }/* END if(job is on a remote peer in the grid) */
          else
            {
            rc = __MUIJobSetAttr(
              J,
              jaindex,
              SubAttr,
              tmpVal,
              CFlags,
              FlagString,
              ArgString,
              Auth,
              Msg);
            } /* END else (job is local) */

          MUISAddData(S,Msg);

          break;
          }

      case mjaEnv:

        {
        char tmpMsg[MMAX_LINE];
        char tmpBuffer[MMAX_LINE];

        /* append or overwrite? */

        if (Op == mDecr)
          {
          /* decr not supported */

          /* NYI */

          MUISAddData(S,"ERROR:  job environment reduction not supported");

          break;
          }

        if (Op == mAdd)
          {
          char *VName;
          char *VVal;

          char *ptr;

          char *TokPtr;
          char *TokPtr2;

          /* append/incr existing env */

          ptr = MUStrTok(tmpVal,",",&TokPtr);

          rc = -1;

          while (ptr != NULL)
            { 
            VName = MUStrTok(ptr,"=",&TokPtr2);

            VVal = MUStrTok(NULL,"=",&TokPtr2);

            rc = MJobAddEnvVar(
                  J,
                  VName,
                  VVal,
                  ((J->DestinationRM != NULL) && (J->DestinationRM->Type == mrmtPBS)) ? ',' : ENVRS_ENCODED_CHAR);
      
            if (rc == FAILURE)
              break;
 
            ptr = MUStrTok(NULL,",",&TokPtr);
            }  /* END while (ptr != NULL) */

          if (rc == SUCCESS)
            { 
            rc = MRMJobModify(
                   J,
                   "Variable_List",
                   NULL,
                   J->Env.BaseEnv,
                   mSet,
                   NULL,
                   tmpMsg,  /* O */
                   NULL);
            }
          }    /* END if (Op == mAdd) */
        else
          {
          rc = MRMJobModify(
               J,
               "Variable_List",
               NULL,
               tmpVal,
               mSet,
               NULL,
               tmpMsg,  /* O */
               NULL);
          }    /* END else (Op == mAdd) */
     
        if (rc == FAILURE)
          {
          MDB(2,fSCHED) MLog("INFO:     cannot modify environment for rm job %s - '%s'\n",
            J->Name,
            tmpMsg);

          snprintf(tmpBuffer,sizeof(tmpBuffer),"cannot modify environment for rm job %s - '%s'\n",
            J->Name,
            tmpMsg);

          MUISAddData(S,tmpBuffer);
          }
        else if (rc == -1)
          {
          MUISAddData(S,"no changes made to job environment");
          }
        else
          {
          MUISAddData(S,"job environment successfully modified");
          }
        }    /* END BLOCK (case mjaEnv) */

        break;

      case mjaReqAWDuration:

        {
        long WallTime;
        long tmpL;

        char tmpMsg[MMAX_LINE];

        char tmpLine[MMAX_LINE];
        char TString[MMAX_LINE];

        mbool_t IsLonger;

        mbool_t FailOnConflict = TRUE;

        mbool_t ReservationReleased = FALSE;

        WallTime = J->SpecWCLimit[0];

        tmpL = MUTimeFromString(tmpVal);

        switch (Op)
          {
          case mAdd:

            WallTime += tmpL;

            break;

          case mDecr:

            WallTime -= tmpL;

            break;

          default:

            WallTime = tmpL;

            break;
          }  /* END switch (Op) */

        if (MJOBISACTIVE(J) == TRUE)
          {
          if (WallTime <= (J->AWallTime + MSched.MaxRMPollInterval))
            {
            /* don't allow them to set the walltime to a value that will cause moab to cancel
               the job, make them run canceljob instead */

            char tmpLine[MMAX_LINE];
            char tmpLine2[MMAX_LINE];
            char TString[MMAX_LINE];
   
            MULToTString(J->SpecWCLimit[0],TString);
   
            strcpy(tmpLine2,TString);
   
            MULToTString(WallTime,TString);
   
            snprintf(tmpLine,sizeof(tmpLine),"cannot reduce walltime for active job %s from %s to %s (cancel job instead?)",
              J->Name,
              tmpLine2,
              TString);
   
            MUISAddData(S,tmpLine);
   
            /* modification failed */
   
            continue;
            }

          WallTime = MAX(WallTime,J->AWallTime);
          }

        IsLonger = ((mulong)WallTime > J->SpecWCLimit[0]) ? TRUE : FALSE;

        if ((IsLonger == TRUE) && 
            (JobManager == FALSE) && 
           ((J->Credential.Q == NULL) || !bmisset(&J->Credential.Q->DynAttrs,mjdaWCLimit)))
          {
          /* requestor is only authorized to reduce active/reserved job walltime */

          if (MJOBISACTIVE(J) == TRUE)
            {
            char tmpLine[MMAX_LINE];
            char TString[MMAX_LINE];

            MULToTString(WallTime,TString);

            snprintf(tmpLine,sizeof(tmpLine),"not authorized to increase walltime attribute for job %s to '%s' (job is active)",
              J->Name,
              TString);

            rc = FAILURE;

            MUISAddData(S,tmpLine);

            continue;
            }

          if (J->Rsv != NULL)
            {
            /* if the user makes the walltime longer then the reservation is forfeit */

            MJobReleaseRsv(J,TRUE,TRUE);

            ReservationReleased = TRUE; /* we will let the user know that they lost their reservation */
            }
          }    /* END if ((IsLonger == TRUE) && ...) */

        /* MJobAdjustWallTime() calls MJobGetSNRange() which for some reason
           blocks the adjustment of the walltime for a job running in a
           INFINITE reservation. Doug says this is a known problem. As
           a workaround, if we pass in a FailOnConflict as FALSE
           to MJobAdjustWallTime() it will skip the checks and just adjust the
           wall time. */

        if ((MSched.Mode == msmSlave) && (JobManager == TRUE)) 
          {
          /* If this is a slave and the request came from the master or an
           * admin then always adjust the wall time */

          FailOnConflict = FALSE;
          }
        else if (strstr(FlagString,"force"))
          {
          /* If the job modify was done by the user with the --flag=force
           * then force the wall time adjustment */

          FailOnConflict = FALSE;
          }
          
        /* MJobAdjustWallTime Will call MRMJobModify */

        rc = MJobAdjustWallTime(J,WallTime,WallTime,TRUE,FALSE,FailOnConflict,TRUE,tmpMsg);

        if (rc == FAILURE)
          {
          char tmpLine[MMAX_LINE];
          char tmpLine2[MMAX_LINE];
          char TString[MMAX_LINE];

          MULToTString(J->SpecWCLimit[0],TString);

          strcpy(tmpLine2,TString);

          MULToTString(WallTime,TString);

          snprintf(tmpLine,sizeof(tmpLine),"cannot change walltime attribute for job %s from %s to %s (%s)",
            J->Name,
            tmpLine2,
            TString,
            tmpMsg);

          MUISAddData(S,tmpLine);

          /* modification failed */

          continue;
          }

        if (J->Triggers != NULL)
          {
          /* job triggers exist - update end event */

          MOReportEvent((void *)J,J->Name,mxoJob,mttEnd,J->StartTime + J->SpecWCLimit[0],TRUE);
          }

        if (J->DestinationRM == NULL)
          {
          char SubmitScript[MMAX_SCRIPT];

          /* if job has not been migrated to an RM (DRM not assigned) then modify the RMSubmitString */

          tmpLine[0] = '\0';
          SubmitScript[0] = '\0';

          MUStrCpy(SubmitScript,J->RMSubmitString,sizeof(SubmitScript));

          /* first, remove the original walltime */

          if (MJobRemovePBSDirective(
              NULL,
              "walltime",
              SubmitScript,
              tmpLine,
              sizeof(tmpLine)) == SUCCESS)
            {

            /* then, add the new walltime */

            sprintf(tmpLine,"walltime=%s",tmpVal);

            MJobPBSInsertArg(
              "-l",
              tmpLine,
              SubmitScript,
              SubmitScript,
              sizeof(SubmitScript));
            }

          /* save it back to RMSubmitString */

          MUStrDup(&J->RMSubmitString,SubmitScript);

          } /* END if (J->DRM == NULL) */

        MULToTString(WallTime,TString);

        snprintf(tmpLine,sizeof(tmpLine),"set walltime attribute for job %s to '%s' %s\n",
          J->Name,
          TString,
          (ReservationReleased == TRUE) ? "(Reservation Released)": "");

        MUISAddData(S,tmpLine);
        }    /* END BLOCK (case mjaReqAWDuration) */

        /* job walltime successfully modified */

        break;

      case mjaReqSMinTime:

        {
        long tmpL;

        MUStringToE(tmpVal,&tmpL,TRUE);

        MJobSetAttr(J,mjaReqSMinTime,(void **)&tmpL,mdfLong,mSet);

        /* NOTE:  may require RM job modification */

        /*  If the resource manager is PBS or SLURM */
        if ((J->SubmitRM != NULL) &&
           ((J->SubmitRM->Type == mrmtPBS) || (J->SubmitRM->Type == mrmtS3)))          
          {
          char tmpName[MMAX_NAME];
          char tmpMsg[MMAX_LINE];

          sprintf(tmpName,"%ld",
            tmpL);

          if (J->SubmitRM->Type == mrmtPBS)
            {
            /* Command for PBS/TORQUE */
            rc = MRMJobModify(
                 J,
                 "Execution_Time",
                 NULL,
                 tmpName,
                 mSet,
                 NULL,
                 tmpMsg,
                 NULL);
            }
          else
            {
            /* Command for SLURM */
            rc = MRMJobModify(
                 J,
                 "Resource_List",
                 "minstarttime",
                 tmpName,
                 mSet,
                 NULL,
                 tmpMsg,
                 NULL);
            }

          if (rc == FAILURE)
            {
            MDB(2,fSCHED) MLog("INFO:     cannot modify minstarttime for rm job %s - '%s'\n",
              J->Name,
              tmpMsg);

            MUISAddData(S,"job minstarttime cannot be modified");
            }
          else
            {
            MUISAddData(S,"job minstarttime modified");
            }
          }
        else
          {
          MUISAddData(S,"job minstarttime modified");

          rc = SUCCESS;
          }
        }  /* END (case mjaReqSMinTime) */

        break;

      case mjaUserPrio:

        rc = __MUIJobSetAttr(
          J,
          jaindex,
          SubAttr,
          tmpVal,  /* I - priority value */
          CFlags,
          FlagString,
          ArgString,
          Auth,
          Msg);  /* O */

        if (rc == FAILURE)
          {
          MUISAddData(S,Msg);
 
          break;
          }

        rc = MRMJobModify(
          J,
          (char *)MJobAttr[mjaUserPrio],
          NULL,
          tmpVal,  /* I - priority value */
          mSet,
          NULL,
          Msg,     /* O */
          NULL);

        if (rc == FAILURE)
          {
          MDB(2,fSCHED) MLog("INFO:     cannot modify minstarttime for rm job %s - '%s'\n",
            J->Name,
            Msg);

          MUISAddData(S,Msg);

          break;
          }

        MUISAddData(S,"priority modified");

        break;

      default:

        switch (rqaindex)
          {
          case mrqaNONE:

            sprintf(Msg,"modification of job attribute '%s' not supported\n",
              MJobAttr[jaindex]);

            MUISAddData(S,Msg);

            break;

          case mrqaAllocPartition:
            
            {
            char tmpMsg[MMAX_LINE];
            mpar_t *tmpPar;
            char *ptr;
            char *TokPtr;
            char tmpLine[MMAX_LINE];

            /* Make sure that the requested partition exists before we modify
             * the job to use this partition */

            if (MJOBISACTIVE(J))
              {
              MDB(2,fSCHED) MLog("INFO:     cannot modify partition of running job %s\n",
                J->Name);

              sprintf(tmpMsg,"cannot modify partition for a running job\n");

              MUISAddData(S,tmpMsg);

              break;
              }

            /* Multiple partitions can be specified delimited by ":" */

            MUStrCpy(tmpLine,tmpVal,sizeof(tmpLine));

            ptr = MUStrTok(tmpLine,":",&TokPtr);

            /* check each partition to make sure it exists */
            do
              {
              if ((rc = MParFind(ptr,&tmpPar)) == FAILURE)
                  break;

              ptr = MUStrTok(NULL,":",&TokPtr);
              } while (ptr != NULL);

            if (rc == FAILURE)
              {
              MDB(2,fSCHED) MLog("INFO:     partition (%s) does not exist, cannot modify partition for RM job %s\n",
                ptr,
                J->Name);

              sprintf(tmpMsg,"partition (%s) does not exist, cannot modify partition for job\n",
                ptr);

              MUISAddData(S,tmpMsg);

              break;
              }

            if ((J->DestinationRM != NULL) &&
                (J->DestinationRM->Type == mrmtMoab))
              {
              /* job has already been migrated--we must unmigrate job before
               * we can change the partition */

              /* llnl reports that unless the job can run immediately after
               * the unmigrate that moab usually ends up cancelling the job. 
               * they originally asked for the unmigrate functionality before
               * we supported just in time migration so this is no longer
               * an issue.  now they would like us to fail the job
               * modification if the job has been migrated rt2160
               
              rc = MJobUnMigrate(J,FALSE,tmpMsg,NULL);

              if (rc == FAILURE) 
              */ 
                {
                MDB(2,fSCHED) MLog("INFO:     cannot modify partition for migrated RM job %s\n",
                  J->Name);

                MUISAddData(S,"cannot modify partition for migrated RM job");

                return(FAILURE);
                }
              }

            if ((J->SubmitRM != NULL) && bmisset(&J->SubmitRM->IFlags,mrmifLocalQueue))
              {
              /* grid job has not yet been migrated */

              /* need to get the proper mode for MJobSetAttr, not just
               * hard-code to MSet */

              rc = __MUIJobSetAttr(
                J,
                mjaPAL,
                SubAttr,
                tmpVal,
                CFlags,
                FlagString,
                ArgString,
                Auth,
                Msg);

              if (rc == FAILURE)
                {
                  MUISAddData(S,"partition not modified\n");
                  MUISAddData(S,Msg);
                }
              else
                MUISAddData(S,"partition modified");

              break;
              }

            rc = MRMJobModify(
                 J,
                 "Resource_List",
                 "partition",
                 tmpVal,
                 mSet,
                 NULL,
                 tmpMsg,  /* O */
                 NULL);  

            if (rc == FAILURE)
              {
              MDB(2,fSCHED) MLog("INFO:     cannot modify partition for RM job %s - '%s'\n",
                J->Name,
                tmpMsg);

              MUISAddData(S,tmpMsg);

              break;
              }
            }  /* END BLOCK */

            MUISAddData(S,"partition modified");

            break;  

          case mrqaNCReqMin:

            if (MJOBISACTIVE(J) == TRUE)
              {
              snprintf(Msg,MMAX_LINE,"ERROR:  cannot modify node count for a running or starting job");

              MUISAddData(S,Msg);

              return(FAILURE);
              }

            if (MSched.BluegeneRM == TRUE)
              {
              char tmpMsg[MMAX_LINE];

              rc = SUCCESS;

              if (MSLURMModifyBGJobNodeCount(J,tmpVal,tmpMsg) == FAILURE)
                {
                MDB(2,fSCHED) MLog("INFO:     cannot modify requested node count for rm job %s - '%s'\n",
                  J->Name,
                  tmpMsg);

                MUISAddData(S,tmpMsg);

                return(FAILURE);
                }
              }
            else
              {
              char tmpMsg[MMAX_LINE];

              if (MUStrHasAlpha(tmpVal) && (MSched.BluegeneRM == FALSE))
                { 
                snprintf(Msg,MMAX_LINE,"ERROR:  Node count must be integer.");

                MUISAddData(S,Msg);

                return(FAILURE);
                }
  
              if (J->Request.NC == 0)
                {
                snprintf(Msg,MMAX_LINE,"ERROR:  Cannot modify node count unless node match policy is exact node.  Refer to NMATCHPOLICY.");

                MUISAddData(S,Msg);

                return(FAILURE);
                }

              if ((MSched.BluegeneRM == FALSE) && (strtol(tmpVal,NULL,0) <= 0))
                {
                /* NOTE: Bluegene may contain an alpha numeric that would cause strtol to fail. */

                snprintf(Msg,MMAX_LINE,"ERROR:  Nodes value must be greater than zero.");

                MUISAddData(S,Msg);

                return(FAILURE);
                }

              if (J->Request.NC == J->Request.TC)
                {
                /* there is a one to one relationship between nodes and tasks so modify tasks as well to maintain the correlation.*/

                MDB(7,fSCHED) MLog("INFO:     job %s - modifying node count and task count from %d to %s\n",
                  J->Name,
                  J->Request.NC,
                  tmpVal);

                MReqSetAttr(J,RQ,mrqaNCReqMin,(void **)tmpVal,mdfString,mSet);  /* nodes */
                MReqSetAttr(J,RQ,mrqaTCReqMin,(void **)tmpVal,mdfString,mSet);  /* tasks */
                }
              else if (J->Request.TC > J->Request.NC)
                {
                /* Assume job was created with PPN parameter */

                int ppn;
                int taskcount;

                MDB(7,fSCHED) MLog("INFO:     job %s - modifying node count from %d to %s\n",
                  J->Name,
                  J->Request.NC,
                  tmpVal);

                ppn = (J->Request.TC / J->Request.NC);

                MReqSetAttr(J,RQ,mrqaNCReqMin,(void **)tmpVal,mdfString,mSet);  /* nodes */

                taskcount = ppn * J->Request.NC; /* Procs per node times new node count */

                MDB(7,fSCHED) MLog("INFO:     job %s - modifying task count from %d to %d\n",
                    J->Name,
                    J->Request.TC,
                    taskcount);

                MReqSetAttr(J,RQ,mrqaTCReqMin,(void **)&taskcount,mdfInt,mSet);  /* tasks */
                }
              else
                {
                /* NYI - if nodes and tasks do not have a one to one relationship, how should modifying the nodes affect the task count? */

                MDB(7,fSCHED) MLog("INFO:     job %s - modifying node count from %d to %s and task count stays at %d\n",
                  J->Name,
                  J->Request.NC,
                  tmpVal,
                  J->Request.TC);

                MReqSetAttr(J,RQ,mrqaNCReqMin,(void **)tmpVal,mdfString,mSet);  /* nodes */
                }

              rc = MRMJobModify(
                   J,
                   "Resource_List",
                   "nodes",
                   tmpVal,
                   mSet,
                   NULL,
                   tmpMsg,  /* O */
                   NULL);

              if (rc == FAILURE)
                {
                MDB(2,fSCHED) MLog("INFO:     cannot modify requested node count for rm job %s - '%s'\n",
                  J->Name,
                  tmpMsg);

                MUISAddData(S,tmpMsg);

                return(FAILURE);
                }
              }    /* END BLOCK */

            MUISAddData(S,"node count modified");

            break;

          case mrqaReqDiskPerTask:

            /* NOTE:  should job reservation be destroyed and recreated? */

            RQ->DRes.Disk = tmpI;

            if (J->Rsv != NULL)
              {
              MRsvDestroy(&J->Rsv,TRUE,TRUE);

              MRsvJCreate(J,NULL,J->StartTime,J->RType,NULL,FALSE);
              }

            rc = SUCCESS;

            break;

          case mrqaReqAttr:

            MReqAttrFromString(&RQ->ReqAttr,tmpVal);

            break;

          case mrqaReqMemPerTask:

            {
            char tmpVal[MMAX_NAME];
            char tmpMsg[MMAX_LINE];

            RQ->DRes.Mem = tmpI;

            sprintf(tmpVal,"%dmb",
              tmpI);

            rc = MRMJobModify(
                 J,
                 "Resource_List",
                 "mem",
                 tmpVal,
                 mSet,
                 NULL,
                 tmpMsg,
                 NULL);

            if (rc == FAILURE)
              {
              MDB(2,fSCHED) MLog("INFO:     cannot modify memory for rm job %s - '%s'\n",
                J->Name,
                tmpMsg);

              MUISAddData(S,tmpMsg);
              }
        
            /* NOTE:  continue even if RM modify fails (is this best?) */
 
            if (J->Rsv != NULL)
              {
              MRsvDestroy(&J->Rsv,TRUE,TRUE);

              MRsvJCreate(J,NULL,J->StartTime,J->RType,NULL,FALSE);
              }
            }    /* END BLOCK */

            break;

          case mrqaReqSwapPerTask:

            RQ->DRes.Swap = tmpI;

            if (J->Rsv != NULL)
              {
              MRsvDestroy(&J->Rsv,TRUE,TRUE);

              MRsvJCreate(J,NULL,J->StartTime,J->RType,NULL,FALSE);
              }

            rc = SUCCESS;

            break;

          case mrqaReqNodeFeature:
          case mrqaSpecNodeFeature:
            {
            char tmpMsg[MMAX_LINE];

            rc = SUCCESS;

            if (MJOBISACTIVE(J))
              {
              sprintf(Msg,"cannot modify features on active job\n");

              MUISAddData(S,Msg);
              rc = FAILURE;
              }
            else if (MRMJobModify(J,Name,NULL,tmpVal,Op,NULL,tmpMsg,NULL) == FAILURE)
              {
              sprintf(Msg,"Resource manager rejected feature modification request: %s\n",
                tmpMsg);

              MUISAddData(S,Msg);
              rc = FAILURE;
              }
            else 
              {
              MReqSetAttr(J,RQ,mrqaSpecNodeFeature,(void **)tmpVal,mdfString,Op);
              }

            if (rc == SUCCESS)
              {
              sprintf(Msg,"job features modified\n");

              MUISAddData(S,Msg);
              }
            }

            break;

          case mrqaGRes:

            {
            char tmpMsg[MMAX_LINE];

            if (Op == mSet)
              MJobRemoveAllGRes(J);

            if (__MUIJobModifyGRes(J,tmpVal,Op,tmpMsg) == SUCCESS)
              {
              rc = SUCCESS;
              }
            else
              {
              rc = FAILURE;
              }

            MUISAddData(S,tmpMsg);
            }

            break;

          default:

            /* NOTE:  should job reservation be destroyed and recreated? */

            sprintf(Msg,"modification of job req attribute '%s' not supported\n",
              MReqAttr[rqaindex]);

            MUISAddData(S,Msg);

            break;
          }  /* END switch (rqaindex) */

        break;
      }  /* END switch (jaindex) */
    }    /* END while (MS3GetSet() == SUCCESS) */

  if (SetFound == FALSE)
    {
    snprintf(Msg,sizeof(Msg),"no valid attributes specified");

    MUISAddData(S,Msg);

    return(FAILURE);
    }

  /* Rebuild the Credential List and save off any changes made to the job */

  MJobBuildCL(J);
  MOCheckpoint(mxoJob,(void *)J,FALSE,NULL);

  if (rc == FAILURE)
    {
    return(FAILURE);
    }

  /* check for modify trigger */

  if (J->Triggers != NULL)
    {
    MOReportEvent((void *)J,J->Name,mxoJob,mttModify,MSched.Time,TRUE);
    }

  MJobTransition(J,FALSE,FALSE);

  return(SUCCESS);
  }  /* END MUIJobModify() */



/**
 * Modify the gres of an existing job.
 *
 * This routine does NOT let you increase the gres count of an active job
 * This routine will fail if the RM doesn't allow the modify (this currently applies to native)
 * This routine won't let you set it to 0 if it's the only thing the job wants
 * This routine won't let you set it to a negative value
 * This routine will update the reservation if it exists
 *
 * @param J     (I) modified
 * @param Value (I)
 * @param Op
 * @param Msg
 */

int __MUIJobModifyGRes(

  mjob_t                 *J,
  char                   *Value,
  enum MObjectSetModeEnum Op,
  char                    *Msg)

  {
  long Count = 1;

  char *GResPtr;
  char *GTokPtr;

  char  tmpMsg[MMAX_LINE];

  int  gindex;
  int  NewGCount;
  int  OrigGCount = 0;

  int  rqindex;

  mreq_t  *RQ;

  mbool_t IsActive;

  if (Msg != NULL)
    Msg[0] = '\0';

  if ((J == NULL) || (MUStrIsEmpty(Value)) || (Msg == NULL))
    {
    return(FAILURE);
    }

  if (strchr(Value,',') != NULL)
    {
    snprintf(Msg,MMAX_LINE,"command only supports modifying single gres\n");

    return(FAILURE);
    }

  /* NOTE:  only allow modification of single gres */

  /* FORMAT:  <GRES>:<COUNT> */

  GResPtr = MUStrTok(Value,":",&GTokPtr);

  if (MUStrIsEmpty(GResPtr))
    {
    snprintf(Msg,MMAX_LINE,"empty gres specified\n");

    return(FAILURE);
    }

  if (!MUStrIsEmpty(GTokPtr))
    {
    /* get the optional count if specified */
    Count = strtol(GTokPtr,NULL,10);
    }

  gindex = MUMAGetIndex(meGRes,GResPtr,(MSched.EnforceGRESAccess == TRUE) ?  mVerify : mAdd);

  if (gindex == 0)
    {
    snprintf(Msg,MMAX_LINE,"Unknown GRes '%s'\n",GResPtr);

    return(FAILURE);
    }

  /* find out if the gres was already requested */

  for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
    {
    RQ = J->Req[rqindex];

    if (RQ == NULL)
      break;

    if (MSNLGetIndexCount(&RQ->DRes.GenericRes,gindex) > 0)
      {
      OrigGCount = MSNLGetIndexCount(&RQ->DRes.GenericRes,gindex);

      break;
      }
    }

  IsActive = MJOBISACTIVE(J);

  switch (Op)
    {
    case mSet:

      NewGCount = Count;

      break;

    case mDecr:

      NewGCount = OrigGCount - Count;

      break;

    case mAdd:
    default:

      NewGCount = OrigGCount + Count;

      break;
    }  /* END switch (Op) */


  if (NewGCount < 0)
    {
    snprintf(Msg,MMAX_LINE,"cannot request negative gres count\n");

    return(FAILURE);
    }
  else if (NewGCount == 0)
    {
    if (MJobOnlyWantsThisGRes(J,gindex) == TRUE)
      {
      snprintf(Msg,MMAX_LINE,"cannot remove only requested resource on the job (cancel job instead?)\n");

      return(FAILURE);
      }

    Op = mSet;
    }
  else if (NewGCount > OrigGCount)
    {
    if (IsActive == TRUE)
      {
      snprintf(Msg,MMAX_LINE,"cannot increase gres count on active job\n");

      return(FAILURE);
      }

    Op = mAdd;
    }
  else if (NewGCount == OrigGCount)
    {
    snprintf(Msg,MMAX_LINE,"job already has requested GRes count\n");

    return(FAILURE);
    }
  else
    {
    Op = mDecr;
    }

  /* NOTE:  only contact the RM if we're modifying down, because active jobs can't modify up */

  switch (Op)
    {
    case mDecr:

      {
      char strCount[MMAX_NAME];

      snprintf(strCount,sizeof(strCount),"%d",NewGCount);

      if (MRMJobModify(J,"gres",GResPtr,GTokPtr,mSet,NULL,tmpMsg,NULL) == FAILURE)
        {
        snprintf(Msg,MMAX_LINE,"Resource manager rejected gres modification request: %s\n",
          tmpMsg);

        return(FAILURE);
        }

      /* NOTE:  RM modify successful, set new gres gres value */

      MSNLSetCount(&RQ->DRes.GenericRes,gindex,NewGCount);
      }  /* case mDecr */

      break;

    case mAdd:

      if (MJobAddRequiredGRes(
            J,
            MGRes.Name[gindex],
            NewGCount,
            mxaGRes,
            FALSE,
            FALSE) == FAILURE)
        {
        snprintf(Msg,MMAX_LINE,"modification of job GRes '%s' failed\n",
          GResPtr);

        return(FAILURE);
        }

      break;

    default:

      /* if we are sure that the above code guarantees that removing this req won't
         result in a job with no Reqs then we could change this FALSE to TRUE */

      MJobRemoveGRes(J,MGRes.Name[gindex],FALSE);

      break;
    }  /* END switch (Op) */

  if (J->Rsv != NULL)
    {
    MJobReleaseRsv(J,TRUE,FALSE);

    MRsvJCreate(J,NULL,J->StartTime,mjrActiveJob,NULL,FALSE);
    }    /* END if (R != NULL) */

  snprintf(Msg,MMAX_LINE,"job GRes modified\n");

  return(SUCCESS);
  }  /* END MUIJobModifyGRes() */

/* END MUIJobModify.c */
