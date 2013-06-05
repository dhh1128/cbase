/* HEADER */

      
/**
 * @file MWikiJobUpdateAttr.c
 *
 * Contains: MWikiJobUpdateAttr
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  

#include "MWikiInternal.h"


/**
 * Process Wiki attr-val pair and apply appropiate attribute to job.
 *
 * @see MWikiJobUpdate() - parent
 *
 * @param Tok (I) [format=<ATTR>=<VAL>]
 * @param J (I) [modified]
 * @param DRQ (I) [optional]
 * @param R (I) [optional]
 * @param EMsg (O) [optional,minsize=MMAX_LINE] 
 */

int MWikiJobUpdateAttr(

  const char *Tok,
  mjob_t     *J,
  mreq_t     *DRQ,
  mrm_t      *R,
  char       *EMsg)

  {
  char  *ptr;

  char  Value[MMAX_BUFFER] = {0};

  mreq_t  *RQ;
  mnode_t *N;
  mqos_t  *QDef;

  int    nindex;
  int    aindex;

  char   SubAttr[MMAX_LINE] = {0};
  char   HostName[MMAX_NAME];

  char  *TokPtr = NULL;

  int    tmpI;

  const char *FName = "MWikiJobUpdateAttr";

  MDB(6,fWIKI) MLog("%s(%.256s,%s,DRQ,EMsg)\n",
    FName,
    Tok,
    (J != NULL) ? J->Name : "NULL");

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (J == NULL)
    {
    return(FAILURE);
    }

  if ((Tok == NULL) || (Tok[0] == '\0'))
    {
    /* Luis is seeing a problem with empty Tok, nothing else is wrong with the job,
       just ignore the empty token and return success for now */

    return(SUCCESS);
    }

  if ((ptr = MUStrChr(Tok,'=')) == NULL)
    {
    MDB(3,fWIKI) MLog("WARNING:  malformed WIKI string '%s' - no '='\n",
      Tok);

    if (EMsg != NULL)
      strcpy(EMsg,"invalid string - no '='");

    return(FAILURE);
    }

  RQ = (DRQ != NULL) ? DRQ : J->Req[0]; /* FIXME */

  if (RQ == NULL)
    {
    MDB(3,fWIKI) MLog("WARNING:  invalid job '%s' - no req\n",
      J->Name);

    if (EMsg != NULL)
      strcpy(EMsg,"invalid job - no req");

    return(FAILURE);
    }

  if (*ptr == '\0')
    {
    MDB(3,fWIKI) MLog("WARNING:  malformed WIKI string '%s' - EOF\n",
      Tok);

    if (EMsg != NULL)
      strcpy(EMsg,"invalid string - empty");

    return(FAILURE);
    }


  /* Need to Map string key=value to individual componets: 'aindex', 'SubName' and 'Value' */

  {
  char *Name;
  char *SubName;
  char tmpTok[MMAX_LINE];

  /* avoid modifying Tok, so get a local copy */
  MUStrCpy(tmpTok,Tok,sizeof(tmpTok));

  /* Parse if in 'ATTRIBUTE[SUBATTRIBUTE]=' format */

  if (MUParseNewStyleName(tmpTok,&Name,&SubName) == SUCCESS)
    {
    /* This has a SUBATTRIBUTE, save that as SubAttr */
    MUStrCpy(SubAttr,SubName,sizeof(SubAttr));

    /* Make the main attribute to an 'aindex' */
    aindex = MUGetIndexCI(Name,MWikiJobAttr,MBNOTSET,0);
    }
  else
    {
    /* Make the main attribute to an 'aindex' */
    aindex = MUGetIndexCI(Tok,MWikiJobAttr,MBNOTSET,0);
    }
  }

  /* validate the 'aindex', handle if not found */
  if (aindex == 0)
    {
    char tmpLine[MMAX_LINE];

    if (J->TemplateExtensions != NULL)
      {
      /* process profile extensions */

      if (MTJobProcessAttr(J,Tok) == SUCCESS)
        {
        return(SUCCESS);
        }
      }

    /* unexpected attribute specified */

    if (EMsg != NULL)
      {
      snprintf(EMsg,MMAX_LINE,"attribute '%s' not supported",
        Tok);
      }

    snprintf(tmpLine,sizeof(tmpLine),"unexpected attribute '%s' specified via WIKI interface",
      Tok);

    MMBAdd(
      &J->MessageBuffer,
      tmpLine,
      NULL,
      mmbtNONE,
      0,
      0,
      NULL);
 
    return(FAILURE);
    }  /* END if (aindex == 0) */

  ptr++;   /* Skip the '=' character */

  /* comparators can include ==, = and need them to survive, so skip them */

  if ((aindex == mwjaCProcsCmp) || 
      (aindex == mwjaRMemCmp)  || 
      (aindex == mwjaRAMemCmp) ||  
      (aindex == mwjaRDiskCmp) || 
      (aindex == mwjaRSwapCmp) || 
      (aindex == mwjaRASwapCmp) ||
      (aindex == mwjaAccount))
    {
    while (isspace(*ptr))
      ptr++;
    }
  else 
    {
    while (isspace(*ptr) || (*ptr == '=')) 
      ptr++;
    }

  /* NOTE:  should process empty fields to reset previously set attributes */

#ifdef MNOT
  if (*ptr == '\0')
    {
    /* empty value specified */

    return(SUCCESS);
    }
#endif /* MNOT */

  /* Now copy the 'value' of the 'attribute=value' to its own variable */

  MUStrCpyQuoted(Value,ptr,sizeof(Value));

  /* Key switch here, where the real work gets done: 
   *     we have now mapped the 'attribute' to an enum, so now dispatch to process
   *     the attribute according to its own semantics
   *
   *     Components available for use are:  int:aindex, string:SubAttr and string:Value
   */
  switch (aindex)
    {
    case mwjaCheckpointFile:

      /* NOTE:  required hostlist should be checkpointed or this constraint will be lost */

      if ((J->DestinationRM != NULL) && 
           bmisset(&J->DestinationRM->IFlags,mrmifNoMigrateCkpt))
        {
        if ((MJOBISACTIVE(J) || MJOBISCOMPLETE(J)) &&
            !bmisset(&J->IFlags,mjifHostList) &&
            (!MNLIsEmpty(&J->NodeList)))
          {
          /* lock job to currently allocated nodes */

          MJobSetAttr(J,mjaReqHostList,(void **)MNLGetNodeName(&J->NodeList,0,NULL),mdfString,mSet);
          }
        }

      break;

    case mwjaComment:

      {
      /* NOTE:  Wiki comment maps to RM extension string */

      char *tmpPtr;

      /* remove trailing '\"' if present */

      if (Value[strlen(Value) - 1] == '\"')
        Value[strlen(Value) - 1] = '\0';

      /* remove embedded '\'' characters from the string */

      while ((tmpPtr = strchr(Value,'\'')) != NULL)
        {

        /* shift the rest of the string over one byte */

        while (*tmpPtr != '\0')
          {
          *tmpPtr = *(tmpPtr + 1);
          tmpPtr++;
          }
        }

      /* remove beginning '\"' from the string */

      if (Value[0] == '\"')
        {
        tmpPtr = Value;

        while (*tmpPtr != '\0')
          {
          *tmpPtr = *(tmpPtr + 1);
          tmpPtr++;
          }
        }

      /* Change: changed mIncr to mSet.  When changing dependencies, SLURM
         updates its comment string.  With mIncr, RMXString would have the 
         entire old RMXString (including old dependencies), and the entire
         new RMXString.  This would mean old dependencies would be re-read
         and everything else was repeated */

      MJobSetAttr(J,mjaRMXString,(void **)Value,mdfString,mSet);

      if ((J->RMXString != NULL) &&
          (MJobProcessExtensionString(J,J->RMXString,mxaNONE,NULL,EMsg) == FAILURE))
        {
        MDB(3,fWIKI) MLog("ALERT:    cannot process extension line '%s' for job %s\n",
          (J->RMXString != NULL) ? J->RMXString : "NULL",
           J->Name);

        return(FAILURE);
        }
      }  /* END BLOCK (case mwjaComment) */

      break;

    case mwjaExitCode:

      MJobSetAttr(J,mjaCompletionCode,(void **)Value,mdfString,mSet);

      break;

    case mwjaFlags:

      {
      mbitmap_t Flags;

      /* includes interactive settings */

      MDB(7,fWIKI) MLog("INFO:     WIKI flags '%s' set for job %s\n",
        Value,
        J->Name);

      /* special handling for internal flags */

      MJobFlagsFromString(J,RQ,NULL,NULL,NULL,Value,FALSE);

      bmfromstring(Value,MJobFlags,&Flags);
      
      bmor(&J->SpecFlags,&Flags);
      bmor(&J->Flags,&J->SpecFlags);

      bmclear(&Flags);
      }

      break;

    case mwjaGAttr:

      MJobSetAttr(J,mjaGAttr,(void **)Value,mdfString,mIncr);

      break;

    case mwjaGEvent:

      /* This is currently used for pending action jobs.  These are placed in 
          the job's messages, with an error type */

      bmset(&J->IFlags,mjifSystemJobFailure);

      MMBAdd(
        &J->MessageBuffer,
        (Value != NULL) ? Value : "event received",
        "N/A",
        mmbtPendActionError,
        0,
        1,
        NULL);

      break;

    case mwjaGMetric:

      /* Update this Job GMetric[SubAttr] on this job with 'Value' */

      if (FAILURE == MJobGMetricUpdateAttr(J,SubAttr,Value))
        {
        if (NULL != EMsg)
          sprintf(EMsg,"Could not add add GMETRIC[%s]=%s to Job\n",SubAttr,Value);

        return(FAILURE);
        }

      break;

    case mwjaHoldType:

      {
      mbitmap_t tmpL;

      /* see MHoldType[] */

      /* NOTE:  should only allow set/unset of User/System holds via WIKI */

      if ((Value[0] == '\0') || !strcmp(Value,"0"))
        {
        /* empty hold list specified, clear all holds */

        /* NO-OP */
        }
      else if (!isdigit(((char *)Value)[0]))
        {
        bmfromstring((char *)Value,MHoldType,&tmpL);
        }

      if (!bmisset(&tmpL,mhUser))
        bmunset(&J->Hold,mhUser);
      else
        bmset(&J->Hold,mhUser);

      if (!bmisset(&tmpL,mhSystem))
        bmunset(&J->Hold,mhSystem);
      else
        bmset(&J->Hold,mhSystem);
      }  /* END BLOCK (case mwjaHoldType) */

      break;

    case mwjaJName:

      MUStrDup(&J->AName,Value);

      break;

    case mwjaTemplate:

      {
      mjob_t* JobTemplate;

      if (MTJobFind((char*)Value,&JobTemplate) == FAILURE)
        {
        MDB(3,fWIKI) MLog("ERROR:  template %s not found for job %s\n",
          Value,
          J->Name);

        if (EMsg != NULL)
          strcpy(EMsg,"template not found");

        return(FAILURE);
        }  

#if 0 /* this never worked before because J->TX->TIndex was just the LAST applied template
         we could search J->CL as templates are added as credentials to jobs. NYI */
      if ((J->TemplateExtensions != NULL) &&
          (J->TemplateExtensions->TIndex != JobTemplate->Index))
        {
        if (MJobApplySetTemplate(J,JobTemplate,NULL) == FAILURE)
          {
          MDB(3,fWIKI) MLog("ERROR:  template %s not applied to job %s\n",
            JobTemplate->Name,
            J->Name);

          if (EMsg != NULL)
            strcpy(EMsg,"cannot apply template");
          
          return(FAILURE);
          }
        }
#endif
      }  /* END case mwjaTemplate */

      break;

    case mwjaName:

      {
      /* user-specified job name */

      char *VPtr = NULL;

      VPtr = Value;
      
      if ((VPtr[0] == '\"') && (VPtr[strlen(VPtr)-1] == '\"'))
        {
        /* Slurm reports back the job name quoted. One issue with this is in the 
         * grid, the slave will report back the quoted job name and it will replace
         * the non-quoted job name in the hash table. Any dependencies relying on 
         * the non-quoted job name are then broken. */

        VPtr++;
        VPtr[strlen(VPtr)-1] = '\0';
        }

      MUStrDup(&J->AName,VPtr);

      break;
      }  /* END BLOCK */

    case mwjaPriority:

      /* see mwjaUPriority */

      {
      long tmpL;

      if (!bmisset(&J->IFlags,mjifIsTemplate))
        break;

      tmpL = strtol(Value,NULL,10);

      if (strchr(Value,'+'))
        {
        /* priority is relative */

        J->SystemPrio = (MMAX_PRIO_VAL << 1) + tmpL;
        }
      else if (strchr(Value,'-'))
        {
        /* priority is relative */

        J->SystemPrio = (MMAX_PRIO_VAL << 1) + tmpL;
        }
      else
        {
        /* priority is absolute */

        J->SystemPrio = tmpL;
        }
      }    /* END BLOCK (case mwjaPriority) */
  
      break;

    case mwjaPriorityF: 

      MNPrioFree(&(J->NodePriority));
      if (MProcessNAPrioF(&(J->NodePriority),Value) == FAILURE)
        {
        MDB(3,fCONFIG) MLog("WARNING:  invalid priority function '%s' on job '%s'\n",
          Value,
          J->Name);
        }

      break;

    case mwjaSlaveScript:

      /* NYI */

      break;

    case mwjaUpdateTime:

      /* this is the RM's update time (not Moab's) */

      /* J->MTime = strtol(Value,NULL,10);
      J->ATime = J->MTime; */

      break;

    case mwjaEstWCTime:

      J->EstWCTime = strtol(Value,NULL,10);

      break;

    case mwjaState:

      {
      enum MJobStateEnum JState;

      if ((R != NULL) && 
          (bmisset(&R->Flags,mrmfIgnWorkloadState)))
        {
        /* ignore state coming from this RM */

        break;
        }

      if (strcasecmp(MJobState[J->State],Value))
        {
        if (MSched.Iteration > 0)
          {
          MDB(1,fWIKI) MLog("INFO:     job '%s' changed states from %s to %s\n",
            J->Name,
            MJobState[J->State],
            Value);
          }

        J->MTime = MSched.Time;
        }

      JState = (enum MJobStateEnum)MUGetIndexCI(Value,MJobState,FALSE,mjsNONE);

      switch (JState)
        {
        case mjsHold:

          /* STATE=hold is deprecated, assume user hold 
             instead, should set STATE=idle,HOLDTYPE=<X> */

          bmset(&J->Hold,mhUser);

          J->State = mjsIdle;

          break;

        case mjsUnknown:

          if ((J->EState == mjsSuspended) || 
              (J->EState == mjsIdle) || 
              (J->EState == mjsNONE))
            {
            /* NOTE:  slurm wiki interface does not properly report suspended */

            J->State = mjsSuspended;
            }
          else
            {
            J->State = JState;
            }

          break;

        case mjsIdle:

          if (MJOBISACTIVE(J) && (J->Rsv != NULL))
            {
            /* job is rejected/requeued */

            MDB(1,fWIKI) MLog("INFO:     job %s requeued/rejected - releasing reservation\n",
              J->Name);

            MJobReleaseRsv(J,TRUE,TRUE);
            }

          J->State = JState;

          break;

        default:

          J->State = JState;

          break;
        }  /* END switch (JState) */
      }    /* END BLOCK (case mwjaState) */

      break;

    case mwjaWCLimit:

      {
      long  val; 

      /* FORMAT:  [[HH:]MM:]SS */

      val =  MUTimeFromString(Value);

      if (val < 0)
        {
        /* unlimited walltime requested */

        bmset(&J->IFlags,mjifWCNotSpecified);

        if (MPar[0].L.JobPolicy->HLimit[mptMaxWC][0] > 0)
          val = MPar[0].L.JobPolicy->HLimit[mptMaxWC][0];
        else
          val = MDEF_SYSMAXJOBWCLIMIT;
        }

      J->SpecWCLimit[1] = (mutime)val;
      J->SpecWCLimit[0] = J->SpecWCLimit[1];
     
      if (!bmisset(&MPar[0].Flags,mpfUseMachineSpeed))
        {
        J->WCLimit = J->SpecWCLimit[0];
        }
      }   /* END BLOCK (case mwjaWCLimit) */

      break;

    case mwjaWCKey:
      
      {
      char  tmpLine[MMAX_LINE];

      /* store the wckey as a variable job attribute */

      snprintf(tmpLine,sizeof(tmpLine),"wckey=%s",Value);
      MJobSetAttr(J,mjaVariables,(void **)tmpLine,mdfString,mSet);
      }
      
      break;
      
    case mwjaTasks:

      {
      int rqindex;

      int TC;

      /* requested job taskcount */

      /* NOTE:  must handle multi-req jobs */

      /* NOTE:  assume tasks reference single req compute tasks only */

      bmset(&J->IFlags,mjifTasksSpecified);

      if (!strncasecmp(Value,"select=",strlen("select=")))
        {
        char EMsg[MMAX_LINE];

        if (MJobGetSelectPBSTaskList(J,Value,NULL,TRUE,FALSE,EMsg) == FAILURE)
          {
          if (EMsg[0] != '\0')
            {
            MMBAdd(
              &J->MessageBuffer,
              EMsg,
              NULL,
              mmbtNONE,
              0,
              0,
              NULL);
            }

          return(FAILURE);
          }
        }
      else if (strchr(Value,'+') || strchr(Value,':') || strchr(Value,'#'))
        {
        /* parse as PBS/TORQUE node resource description */

        MJobGetPBSTaskList(J,Value,NULL,FALSE,TRUE,TRUE,FALSE,R,NULL);

        break;
        }

      tmpI = (int)strtol(Value,NULL,10);

      if ((tmpI == 0) && (RQ->DRes.Procs == 0))  
        {
        /* no compute tasks specified, do not modify local resource req */

        /* NO-OP */
        }
      else
        {  
        if (MJOBISACTIVE(J) == FALSE)
          {
          /* NOTE:  job is idle, must take into account multi-req jobs with virtual resources (NYI) */

          if (J->TotalTaskCount != 0)
            {
            if (tmpI != J->TotalTaskCount)
              {
              MDB(7,fWIKI) MLog("ALERT:    job %s RM task count %d does not match requested ttc %d)\n",
                J->Name,
                tmpI,
                J->TotalTaskCount);
              } 
            }
          if (J->Geometry != NULL)
            {
            MJobParseGeometry(J,J->Geometry);
            }
          else
            {
            J->Request.TC = tmpI;
            RQ->TaskCount = tmpI;

            RQ->TaskRequestList[0] = tmpI;
            RQ->TaskRequestList[1] = tmpI;      
            RQ->TaskRequestList[2] = 0;

            if (bmisset(&J->IFlags,mjifMaxNodes)) /* slurm maxnodes reported */
              RQ->TasksPerNode = RQ->TaskCount;
            }
          }   /* END if (MJOBISACTIVE(J) == FALSE) */
        else
          {
          /* NOTE:  job is active */

          /* task count only applies to non-virtual resources */

          TC = tmpI;

          for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
            {
            if (J->Req[rqindex]->DRes.Procs == 0)
              continue;

            /* remove tasks associated with other compute reqs */

            if (rqindex == RQ->Index)
              continue;

            TC -= J->Req[rqindex]->TaskCount;
            }

          if (bmisset(&J->IFlags,mjifIsNew))
            {
            /* job is new */

            RQ->TaskCount = tmpI;

            RQ->TaskRequestList[0] = tmpI;
            RQ->TaskRequestList[1] = tmpI;
            RQ->TaskRequestList[2] = 0;

            if (J->Req[1] != NULL)
              J->Request.TC += TC;
            else
              J->Request.TC = TC;

            break;
            }  /* END if (bmisset(&J->IFlags,mjifIsNew)) */

          if ((TC != 0) && (tmpI != J->Request.TC))
            {
            if (J->Req[1] == NULL)
              {
              /* single req job has taskcount specified which does not match job level task request */

              /* only alert if pseudo-resources not associated with job */

              MDBO(3,J,fRM) MLog("ALERT:    job '%s' TC (%d) does not match TC from RM (%d)\n",
                J->Name,
                J->Request.TC,
                tmpI);
              }
            }
          }    /* END else (MJOBISACTIVE(J) == FALSE) */
        }      /* END else ((tmpI == 0) && (RQ->DRes.Procs == 0)) */
      }        /* END BLOCK (case mwjaTasks) */

      break;

    case mwjaNodes:

      /* TODO: add user friendly template masks */

      if (bmisset(&J->IFlags,mjifMaxNodes)) /* NodeCount already set in mwjaMaxNodes */
        break;

      tmpI = (int)strtol(Value,NULL,10);

      /* NOTE:  if SLURM, info is reported as minimum nodes, not required nodes */

      if (((J->DestinationRM != NULL) && (J->DestinationRM->SubType == mrmstSLURM)) ||
          ((J->SubmitRM != NULL) && (J->SubmitRM->SubType == mrmstSLURM)))
        {
        /* NOTE:  if SLURM/BG, info is reported as minimum nodes, not required nodes */

        if ((tmpI > 1) || (J->Request.TC <= 0))
          {
          J->Request.NC = tmpI;
          RQ->NodeCount = tmpI;
          }
        }
      else
        {
        J->Request.NC = tmpI;
        RQ->NodeCount = tmpI;
        }

      break;

    case mwjaMaxNodes:

      /* Because slurm reports nodes=1 for the two cases, -N1 -n3 and -n3, moab can't determine 
       * whether to let the job span nodes or not. Slurm now reports MAXNODES=0 for -n3 and 
       * MAXNODES=1 for -N1 -n3. For right now, only the case where MAXNODES=1 is considered.
       * Moab sets RQ->TasksPerNode to RQ->TaskCount to keep job on one node.
       * 10/08 BOC */

      tmpI = (int)strtol(Value,NULL,10);

      /* Don't adjust TasksPerNode if using TTC - TaskCount may not be correct at job start on the grid slave.
         See ticket RT2595 and code in MUIJobCtl for job start to see why TaskCount may not be correct
         on a moab slave. */

      if ((tmpI == 1) &&
          (J->TotalTaskCount == 0))  
        {
        J->Request.NC = tmpI;
        RQ->NodeCount = tmpI;

        RQ->TasksPerNode = RQ->TaskCount;

        bmset(&J->IFlags,mjifMaxNodes);
        }
      else if (bmisset(&J->IFlags,mjifMaxNodes) == TRUE)
        {
        /* If the job is modified, MAXNODES can change to something other than
         * one so forcing to one node isn't need anymore. Previously, if 
         * MAXNODES changed from 1 to 2, the RQ->TasksPerNode would be set
         * to the new TaskCount which would be greater than MaxPPN and 
         * wouldn't run. */
        
        RQ->TasksPerNode = 0;

        bmunset(&J->IFlags,mjifMaxNodes);
        }

      break;

    case mwjaGeometry:

      /* NYI */
 
      break;

    case mwjaQueueTime:

      J->SubmitTime = strtol(Value,NULL,10);

      if (J->SubmitTime > MSched.Time)
        {
        char TString[MMAX_LINE];

        MULToTString(J->SubmitTime - MSched.Time,TString);

        MDB(2,fWIKI) MLog("WARNING:  clock skew detected (queue time for job %s in %s)\n",
          J->Name,
          TString);

        J->SubmitTime = MSched.Time;
        }

      break;

    case mwjaStartDate:

      {
      long tmpL;

      tmpL = strtol(Value,NULL,10);

      MJobSetAttr(J,mjaReqSMinTime,(void **)&tmpL,mdfLong,mSet);
      }  /* END BLOCK */

      break;

    case mwjaEndDate:

      {
      long tmpL;

      tmpL = strtol(Value,NULL,10);

      MJobSetAttr(J,mjaReqCMaxTime,(void **)&tmpL,mdfLong,mSet);
      }  /* END BLOCK */
 
      break;

    case mwjaStartTime:

      {
      mulong tmpStartTime;

      tmpStartTime = strtol(Value,NULL,10);

      if (tmpStartTime > MSched.Time)
        {
        char TString[MMAX_LINE];

        MULToTString(tmpStartTime - MSched.Time,TString);

        MDB(2,fWIKI) MLog("WARNING:  clock skew detected (start time for job %s in %s)\n",
          J->Name,
          TString);

        tmpStartTime = MSched.Time;
        }

      if ((J->StartTime > 0) && (tmpStartTime != J->StartTime))
        {
        MDB(2,fWIKI) MLog("INFO:     start time changed from %ld to %ld on job %s\n",
          J->StartTime,
          tmpStartTime,
          J->Name);
        }

      J->StartTime = tmpStartTime;

      if (J->DispatchTime <= 0)
        J->DispatchTime = tmpStartTime;
      }  /* END BLOCK (case mwjaStartTime) */

      break;

    case mwjaCheckpointStartTime:

      J->CheckpointStartTime = strtol(Value,NULL,10);

      if (J->CheckpointStartTime > MSched.Time)
        {
        char TString[MMAX_LINE];

        MULToTString(J->CheckpointStartTime - MSched.Time,TString);

        MDB(2,fWIKI) MLog("WARNING:  clock skew detected (checkpoint start time for job %s in %s)\n",
          J->Name,
          TString);

        J->CheckpointStartTime = MSched.Time;
        }

      break;

    case mwjaCompletionTime:

      J->CompletionTime = strtol(Value,NULL,10);

      if (J->CompletionTime > MSched.Time)
        {
        char TString[MMAX_LINE];

        MULToTString(J->CompletionTime - MSched.Time,TString);

        MDB(2,fWIKI) MLog("WARNING:  clock skew detected (completion time for job %s in %s)\n",
          J->Name,
          TString);

        J->CompletionTime = MSched.Time;
        }

      break;

    case mwjaUName:

      if (MUserAdd(Value,&J->Credential.U) == FAILURE)
        {
        MDB(1,fWIKI) MLog("ERROR:    cannot add user for job %s (UserName: %s)\n",
          J->Name,
          Value);
        }

     if (J->EUser != NULL)
       {
       if (strcmp(J->Credential.U->Name,J->EUser))
         {
         mbool_t AllowProxy;

         /* effective user requested which does not match job owner */

         AllowProxy = MJobAllowProxy(J,J->EUser);

         if (AllowProxy == FALSE)
           {
           MJobSetHold(J,mhBatch,0,mhrCredAccess,"job not authorized to use proxy credentials");
           }
         }
       else
         {
         MUFree(&J->EUser);
         }
       }    /* END if ((J->EUser != NULL) || strcmp(J->Cred.U->Name,J->EUser)) */
  
      break;

    case mwjaEUser:

      /* if user is set and user does not match euser */

      if (bmisset(&J->IFlags,mjifIsTemplate))
        {
        MUStrDup(&J->EUser,Value);
        }
      else if ((J->Credential.U != NULL) && strcmp(J->Credential.U->Name,Value))
        {
        mbool_t AllowProxy;

        /* effective user requested which does not match job owner */

        AllowProxy = MJobAllowProxy(J,Value);

        if (AllowProxy == FALSE)
          {
          MJobSetHold(J,mhBatch,0,mhrCredAccess,"job not authorized to use proxy credentials");
          }
        else
          {
          MUStrDup(&J->EUser,Value);
          }
        }    /* END if ((J->Cred.U != NULL) || strcmp(J->Cred.U->Name,Value)) */
      else if (J->Credential.U == NULL)
        {
        /* process later in MWikiJobSetAttr(mwjaUName) */

        MUStrDup(&J->EUser,Value);
        }

      break;

    case mwjaGName:

      if (Value[0] == '$')
        {
        /* NOTE:  map group to local group */

        /* NOTE:  should differentiate $LOCALGROUP from other variables (NYI) */

        bmset(&J->IFlags,mjifUseLocalGroup);
        }
      else if (MGroupAdd(Value,&J->Credential.G) == FAILURE)
        {
        MDB(1,fWIKI) MLog("ERROR:    cannot add group for job %s (GroupName: %s)\n",
          J->Name,
          Value);
        }

      break;

    case mwjaAccount:

      if (MAcctAdd(Value,&J->Credential.A) == FAILURE)
        {
        MDB(1,fWIKI) MLog("ERROR:    cannot add account for job %s (Account: %s)\n",
          J->Name,
          Value);
        }

      break;

    case mwjaNodeSet:

      {
      char   *TokPtr2;
      int     index;
      char   *ptr2;

      MDB(6,fWIKI) MLog("INFO:     nodeset '%s' requested\n",
        Value);

      if ((ptr2 = MUStrTok(Value,":",&TokPtr2)) != NULL)
        {
        RQ->SetSelection = (enum MResourceSetSelectionEnum)MUGetIndexCI(
          ptr2,
          MResSetSelectionType,
          FALSE,
          mrssNONE);

        if ((ptr2 = MUStrTok(NULL,":",&TokPtr2)) != NULL)
          {
          RQ->SetType = (enum MResourceSetTypeEnum)MUGetIndexCI(
            ptr2,
            MResSetAttrType,
            FALSE,
            mrstNONE);

          index = 0;

          while ((ptr2 = MUStrTok(NULL,":,",&TokPtr2)) != NULL)
            {
            MUStrDup(&RQ->SetList[index],ptr2);

            index++;
            }  /* END while (ptr2) */
          }
        }
      }      /* END BLOCK */

      break;

    case mwjaNodeAllocationPolicy:

      MJobSetAttr(J,mjaNodeAllocationPolicy,(void **)Value,mdfString,mSet);

      break;

    case mwjaPref:

      MReqSetAttr(J,RQ,mrqaPref,(void **)Value,mdfString,mSet);

      break;

    case mwjaRFeatures:

      MReqSetAttr(J,RQ,mrqaSpecNodeFeature,(void **)Value,mdfString,mSet);

      break;

    case mwjaXFeatures:

      {
      static char const *const modeChars = "&|!";
      char const *modeIndex;
      mbitmap_t *FBM;
      enum MCLogicEnum *FMode;
      char *ValPtr;

      FBM = &RQ->ExclFBM;
      FMode = &RQ->ExclFMode;
      *FMode = mclOR;

      /* MUMAMAttrFromLine() doesn't allow comma delimited */

      MUStrReplaceChar(Value,',',':',FALSE);
      ValPtr = Value;

      if ((modeIndex = strchr(modeChars,Value[0])) != NULL)
        {
        *FMode = (enum MCLogicEnum)(modeIndex - modeChars);

        ValPtr += 1;
        }

      MUNodeFeaturesFromString(ValPtr,mAdd,FBM);
      }  /* END BLOCK (case mwjaXFeatures) */

      break;

    case mwjaRClass:

      MJobSetAttr(J,mjaClass,(void **)Value,mdfString,mSet);

      break;

    case mwjaROpsys:

      if (!strcmp(Value,"NONE"))
        RQ->Opsys = 0;
      else
        RQ->Opsys = MUMAGetIndex(meOpsys,Value,mAdd);

      break;

    case mwjaRArch:

      if (!strcmp(Value,"NONE"))
        RQ->Arch = 0;
      else
        RQ->Arch = MUMAGetIndex(meArch,Value,mAdd);

      break;

    case mwjaRMem:

      RQ->ReqNR[mrMem] = (int)strtol(Value,NULL,10);

      break;

    case mwjaRAMem:

      RQ->ReqNR[mrAMem] = (int)strtol(Value,NULL,10);

      break;

    case mwjaRMemCmp:

      RQ->ReqNRC[mrMem] = MUCmpFromString(Value,NULL);

      break;

    case mwjaRAMemCmp:

      RQ->ReqNRC[mrAMem] = MUCmpFromString(Value,NULL);

      break;

    case mwjaDMem:

      RQ->DRes.Mem = (int)strtol(Value,NULL,10);

      break;

    case mwjaRDisk:

      RQ->ReqNR[mrDisk] = (int)strtol(Value,NULL,10);

      break;

    case mwjaRDiskCmp:

      RQ->ReqNRC[mrDisk] = MUCmpFromString(Value,NULL);

      break;

    case mwjaDDisk:

      RQ->DRes.Disk = (int)strtol(Value,NULL,10); 

      break;

    case mwjaRSwap:

      RQ->ReqNR[mrSwap] = (int)strtol(Value,NULL,10);

      break;

    case mwjaRASwap:

      RQ->ReqNR[mrASwap] = (int)strtol(Value,NULL,10);

      break;

    case mwjaRSwapCmp:

      RQ->ReqNRC[mrSwap] = MUCmpFromString(Value,NULL);

      break;

    case mwjaRASwapCmp:

      RQ->ReqNRC[mrASwap] = MUCmpFromString(Value,NULL);

      break;

    case mwjaDSwap:

      RQ->DRes.Swap = (int)strtol(Value,NULL,10);

      break;

    case mwjaCProcsCmp:

      RQ->ReqNRC[mrProc] = MUCmpFromString(Value,NULL);

      break;

    case mwjaCProcs:
  
      RQ->ReqNR[mrProc] = (int)strtol(Value,NULL,10);
  
      break;

    case mwjaPartitionList:

      {
      mrm_t *R;

      R = (J->DestinationRM != NULL) ? J->DestinationRM : J->SubmitRM;

      if ((R != NULL) && 
          (R->SubType == mrmstSLURM) &&
          (R->Version >= 10000))
        {
        /* map to class */

        if (MClassFind(Value,&J->Credential.C) == FAILURE)
          {
          MDB(1,fWIKI) MLog("ALERT:    cannot locate class '%s' for job %s\n",
            Value,
            J->Name);

          MJobSetHold(J,mhBatch,MSched.DeferTime,mhrRMReject,"cannot locate requested class");

          return(FAILURE);
          }
        }    /* END if ((R != NULL) && ...) */
      else
        {
        mbitmap_t tmpI;

        MPALFromString(Value,mAdd,&tmpI);

        if ((bmisclear(&tmpI)) && (!MUStrIsEmpty(Value)))
          {
          bmclear(&tmpI);

          MJobSetHold(J,mhBatch,MSched.DeferTime,mhrRMReject,"requested partition does not exist");

          return(FAILURE);
          }

        bmcopy(&J->SpecPAL,&tmpI);

        bmclear(&tmpI);
        }  /* END else (R != NULL) && ...) */
      }    /* END BLOCK */

      break;

    case mwjaExec:

      if (!strcmp(Value,"NONE") && (J->Env.Cmd != NULL))
        MUFree(&J->Env.Cmd);
      else
        MUStrDup(&J->Env.Cmd,Value);
 
      break;      

    case mwjaArgs:

      if (!strcmp(Value,"NONE") && (J->Env.Args != NULL))
        MUFree(&J->Env.Args);
      else
        MUStrDup(&J->Env.Args,Value);
 
      break;      

    case mwjaIWD:
 
      MUStrDup(&J->Env.IWD,Value);

      break;

    case mwjaRejectionCount:

      /* NYI */

      break;

    case mwjaRejectionMessage:

      /* a slurm requeued job will have a REJMESSAGE with the 
       * following message, which doesn't require a hold, or if
       the state of the job is Completed, don't put a hold on it.*/

      if (((Value[0] != '\0') &&
          (strstr(Value,"Job requeued by user/admin") != NULL)) ||
          (MJOBISCOMPLETE(J)))
        break;

      if (Value[0] != '\0')
        {
        MJobSetHold(J,mhDefer,MSched.DeferTime,mhrRMReject,Value);
        }

      break;

    case mwjaRejectionCode:

      /* NYI */

      break;

    case mwjaEvent:

      /* NYI */

      break;

    case mwjaTaskPerNode:

      RQ->TasksPerNode = (int)strtol(Value,NULL,10);

      break;

    /* 
    case mwjaTaskList:

      not called, handled via MWikiAttrToTaskList() 

      break;
    */

    case mwjaMasterHost:

      /** @see case mwjaHostList */

      if (!bmisset(&J->IFlags,mjifHostList))
        MJobSetAttr(J,mjaMasterHost,(void **)Value,mdfString,mSet);

      break;

    case mwjaMasterReqAllocTaskList:

      if ((J->SubmitRM == NULL) || (J->SubmitRM->NoAllocMaster == FALSE))
        {
        int  *TaskList = NULL;
        int   tindex;
        int   TCount;

        int   rqindex;

        mreq_t *MRQ;    /* matching req for tasklist */

        TaskList = (int *)MUMalloc(sizeof(int) * MSched.JobMaxTaskCount);

        /* how should task-to-req association be specified? */

        if (MWikiAttrToTaskList(
              J,
              TaskList,             /* O */
              Value,                /* I */
              &TCount) == FAILURE)  /* O */
          {
          MDB(1,fWIKI) MLog("ALERT:    cannot locate tasklist '%s' for job %s\n",
            Value,
            J->Name);
 
          MUFree((char **)&TaskList);

          break;
          }

        /* by default, modify master req */

        MRQ = J->Req[0];

        /* attempt to locate req which matches allocated resources */

        for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
          {
          if (J->Req[rqindex] == NULL)
            break;

          if ((J->Req[rqindex]->DRes.Procs != 0) && 
              (MNode[TaskList[0]]->CRes.Procs != 0))
            {
            /* matching compute node req located */

            MRQ = J->Req[rqindex];

            break; 
            }

          if ((!MSNLIsEmpty(&J->Req[rqindex]->DRes.GenericRes)) && 
              (!MSNLIsEmpty(&MNode[TaskList[0]]->CRes.GenericRes)))
            {
            /* matching generic resource node req located */

            MRQ = J->Req[rqindex];

            break;
            }
          }    /* END for (rqindex) */

        /* NOTE:  should adjust job total task/node count (NYI) */

        if (!MNLIsEmpty(&MRQ->NodeList))
          {
          for (tindex = 0;tindex < TCount;tindex++)
            {
            MNLSetNodeAtIndex(&MRQ->NodeList,tindex,MNode[TaskList[tindex]]);
            MNLSetTCAtIndex(&MRQ->NodeList,tindex,1);

            MNLTerminateAtIndex(&MRQ->NodeList,tindex + 1);
            }
          }

        MUFree((char **)&TaskList);
        }  /* END if ((J->SRM == NULL) || (J->SRM->NoAllocMaster == FALSE)) */

      break;

    case mwjaQOS:

      MQOSFind(Value,&J->QOSRequested);

      if ((MQOSGetAccess(J,J->QOSRequested,NULL,NULL,&QDef) == FAILURE) ||
          (J->QOSRequested == NULL))
        {
        MJobSetQOS(J,QDef,0);
        }
      else
        {
        MJobSetQOS(J,J->QOSRequested,0);
        }

      break;

    case mwjaDProcs:

      /* specify procs per task */

      if (bmisset(&J->IFlags,mjifIsTemplate))
        {
        J->Request.TC = (int)strtol(Value,NULL,10);
        RQ->DRes.Procs = (int)strtol(Value,NULL,10);
        }
      else
        {
        RQ->DRes.Procs = (int)strtol(Value,NULL,10);
        }

      bmset(&J->IFlags,mjifDProcsSpecified);

      break;

    case mwjaHostList:
 
      /* required host list */

      /* FORMAT:  <HOST>[{,:+}<HOST>]...[{*^}] */
 
      {
      mstring_t HostLine(MMAX_LINE);
      int rqindex;

      mrm_t *R;

      R = (J->DestinationRM != NULL) ? J->DestinationRM : J->SubmitRM;

      bmset(&J->IFlags,mjifHostList);

      HostLine = Value;

      nindex = 0;

      MNLClear(&J->ReqHList);

      if ((R != NULL) && bmisset(&R->Wiki.Flags,mslfCompressOutput))
        {
        int tc;
        int *TaskList = NULL;

        TaskList = (int *)MUMalloc(sizeof(int) * MSched.JobMaxTaskCount);

        MWikiAttrToTaskList(J,TaskList,HostLine.c_str(),&tc);

        MJobNLFromTL(J,&J->ReqHList,TaskList,NULL);

        MUFree((char **)&TaskList);
        }
      else
        {
        if ((ptr = strchr(HostLine.c_str(),'^')) != NULL)
          {
          /* superset mode forced */

          /* Determine index of the carat, and replace */
          HostLine[(int) (ptr - HostLine.c_str())] = ':';

          J->ReqHLMode = mhlmSuperset;
          }
        else if ((ptr = strchr(HostLine.c_str(),'*')) != NULL)
          {
          /* subset mode forced */

          /* Determine index of the asterick, and replace */
          HostLine[(int) (ptr - HostLine.c_str())] = ':';

          J->ReqHLMode = mhlmSubset;

          MDB(3,fWIKI) MLog("INFO:     non-compressed format hostlist '%s' contains compressed format task character '*'\n",
            HostLine.c_str());
          }

        if (MSched.ManageTransparentVMs == TRUE)
          {
          /* translate VM's to physical nodes */

          MVMStringToPNString(
            &HostLine,  /* O */
            &HostLine); /* I */
          }

        char *mutableHostLine = NULL;
        MUStrDup(&mutableHostLine,HostLine.c_str());

        ptr = MUStrTok(mutableHostLine,",:+",&TokPtr);

        while (ptr != NULL)
          {
          if (MNodeFind(ptr,&N) == FAILURE)
            {
            /* try full hostname */

            sprintf(HostName,"%s.%s",
              ptr,
              MSched.DefaultDomain);

            if (MNodeFind(HostName,&N) != SUCCESS)
              {
              MDB(1,fWIKI) MLog("ALERT:    cannot locate node '%s' for job '%s' hostlist\n",
                ptr,
                J->Name);

              N = NULL;
              }
            }

          if (N != NULL)
            {
            mnode_t *tmpN;

            MNLGetNodeAtIndex(&J->ReqHList,nindex,&tmpN);

            if (tmpN == N)
              {
              MNLAddTCAtIndex(&J->ReqHList,nindex,1);
              }
            else
              {
              if (tmpN != NULL)
                nindex++;

              MNLSetNodeAtIndex(&J->ReqHList,nindex,N);
  
              MNLSetTCAtIndex(&J->ReqHList,nindex,1);
              }
            }

          ptr = MUStrTok(NULL,",:+",&TokPtr);
          }    /* END while (ptr != NULL) */

        MUFree(&mutableHostLine);

        MNLTerminateAtIndex(&J->ReqHList,nindex + 1);
        }  /* END else ((R != NULL) && bmisset(&R->U.Wiki.Flags,mslfCompressOutput)) */


      /* assign hostlist to first compute req */

      J->HLIndex = 0;

      for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
        {
        if (J->Req[rqindex] == NULL)
          break;

        if (J->Req[rqindex]->DRes.Procs != 0)
          {
          J->HLIndex = rqindex;

          break;
          }
        }    /* END for (rqindex) */

      /* NOTE:  for SLURM systems, hostlist is generally subset */

      if ((J->SubmitRM != NULL) && (J->SubmitRM->SubType == mrmstSLURM))
        {
        J->ReqHLMode = mhlmSubset;
        }    /* END if ((J->SRM != NULL) && (J->SRM->SubType == mrmstSLURM)) */
      }      /* END BLOCK (case mwjaHostList) */

      break;

    case mwjaExcludeHostList:

      /* FORMAT: <node>[[:|,]<node>] where <node> can be <node_name>|<range> 
       * ex. k1:k2:k3 or k[1-3] or k[1,3-4] or k[1-2],j[1-2] */

      MJobSetAttr(J,mjaExcHList,(void **)ptr,mdfString,mSet);

      break;

    case mwjaSID:

      J->SessionID = (int)strtol(Value,NULL,10);

      break;

    case mwjaSuspendTime:

      /* NYI */

      break;

    case mwjaReqRsv:

      MJobSetAttr(J,mjaReqReservation,(void **)Value,mdfString,mSet);

      break;

    case mwjaRsvAccess:

      MJobSetAttr(J,mjaRsvAccess,(void **)Value,mdfString,mSet);

      break;

    case mwjaEnv:

      {
      char *eptr;
      char EBuf[MMAX_BUFFER << 2];

      if (MUStringIsPacked(Value) == TRUE)
        {
        /* must 'unpack' env value */

        MUStringUnpack(Value,EBuf,sizeof(EBuf),NULL);

        eptr = EBuf;
        }
      else
        {
        eptr = Value;
        }

      MUStrDup(&J->Env.BaseEnv,eptr);
      }  /* END BLOCK */

      break;          

    case mwjaInput:

      /* requested stdin file */

      MUStrDup(&J->Env.Input,Value);
 
      break;          

    case mwjaOutput:

      {
      char HostName[MMAX_NAME];
      char tmpLine[MMAX_LINE];

      /* requested stdout file */

      MUStrDup(&J->Env.Output,Value);

      MSchedGetAttr(msaServer,mdfString,(void **)HostName,sizeof(HostName));

      snprintf(tmpLine,sizeof(tmpLine),"%s:%s",
        HostName,
        Value);

      MUStrDup(&J->Env.RMOutput,tmpLine);
      }  /* END BLOCK */

      break;          

    case mwjaError:

      {
      char HostName[MMAX_NAME];
      char tmpLine[MMAX_LINE];

      /* requested stderr file */

      MUStrDup(&J->Env.Error,Value);

      MSchedGetAttr(msaServer,mdfString,(void **)HostName,sizeof(HostName));

      snprintf(tmpLine,sizeof(tmpLine),"%s:%s",
        HostName,
        Value);

      MUStrDup(&J->Env.RMError,tmpLine);
      }  /* END BLOCK */
 
      break;          

    case mwjaSubmitDir:

      /* ignore submit directory */

      /* NO-OP */

      break;

    case mwjaSubmitHost:

      MUStrDup(&J->SubmitHost,Value);

      break;

    case mwjaSubmitString:

      {
      char tmpBuffer[MMAX_BUFFER];
      int  SC;

      /* NOTE;  unpack string */

      MUStringUnpack(Value,tmpBuffer,sizeof(tmpBuffer),&SC);
      
      if (SC == FAILURE)
        {
        MDB(2,fWIKI) MLog("ALERT:    buffer too small to store job submitstring '%.64s'\n",
          Value);

        return(FAILURE);
        }

      MUStrDup(&J->RMSubmitString,tmpBuffer);
      }  /* END BLOCK */

      break;

    case mwjaSubmitType:

      J->RMSubmitType = (enum MRMTypeEnum) MUGetIndex(Value,MRMType,FALSE,0);

      break;

    case mwjaTrigger:

      if (strstr((char *)Value,"$$") != NULL)
        {
        int tindex;

        /* delay variable expansion until after variables loaded */

        if (J->TemplateExtensions == NULL)
          {
          MJobAllocTX(J);
          }

        for (tindex = 0;tindex < MMAX_SPECTRIG;tindex++)
          {
          if (J->TemplateExtensions->SpecTrig[tindex] == NULL)
            MUStrDup(&J->TemplateExtensions->SpecTrig[tindex],Value);
          }
        }
      else
        {
        MTrigLoadString(&J->Triggers,(char *)Value,TRUE,FALSE,mxoJob,J->Name,NULL,NULL);
        }

      break;

    case mwjaUPriority:

      /* see mwjaPriority */

      MJobSetAttr(J,mjaUserPrio,(void **)Value,mdfString,mSet);

      break;

    case mwjaUProcs:

      if (RQ->URes != NULL)
        RQ->URes->Procs = (int)strtol(Value,NULL,10);

      break;

    case mwjaVariables:

      MJobSetAttr(J,mjaVariables,(void **)Value,mdfString,mSet);

      break;

    case mwjaCGRes:
    case mwjaAGRes:
    case mwjaDGRes:

      {
      msnl_t *GRes;

      GRes = (aindex == mwjaCGRes) ? &RQ->CGRes : (aindex == mwjaAGRes) ? 
        &RQ->AGRes : &RQ->DRes.GenericRes;

      MJobUpdateGRes(J,RQ,GRes,aindex,Value); 
      }    /* END BLOCK (case mwjaCGRes,...) */

      break;

    default:

      MDB(2,fWIKI) MLog("INFO:     WIKI keyword '%s'(%d) not handled\n",
        MWikiJobAttr[aindex],
        aindex);

      if (EMsg != NULL)
        strcpy(EMsg,"attribute not handled");

      return(FAILURE);

      /*NOTREACHED*/
           
      break;
    }  /* END switch (aindex) */

  return(SUCCESS);
  }  /* END MWikiJobUpdateAttr() */

/* END MWikiJobUpdateAttr.c */
