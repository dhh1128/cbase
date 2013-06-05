/* HEADER */

      
/**
 * @file MStatAToString.c
 *
 * Contains: MStatAToString
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 * Report stat attribute as string.
 * 
 * @see MStatToXML() - parent
 *
 * @param Stat (I)
 * @param StatType (I)
 * @param Index (I)
 * @param SIndex (I) [only used for indexed stats like GRes]
 * @param Buf (O)
 * @param Format (I)
 */

int MStatAToString(
 
  void                  *Stat,      /* I */
  enum MStatObjectEnum   StatType,  /* I */
  int                    Index,     /* I */
  int                    SIndex,    /* I (only used for indexed stats like GRes) */
  char                  *Buf,       /* O */
  enum MFormatModeEnum   Format)    /* I */
 
  {
  must_t *GS;

  /* int BufSize = MMAX_LINE; */

  if (Buf != NULL)
    Buf[0] = '\0';
 
  if ((Stat == NULL) || (Buf == NULL))
    {
    return(FAILURE);
    }

  if(MStatIsProfileStat(Stat,StatType) && !MStatIsProfileAttr(StatType,Index))
    return(SUCCESS);

  switch (StatType)
    {
    case msoCred:

      {
      must_t *S = (must_t *)Stat;
 
      enum MStatAttrEnum AIndex = (enum MStatAttrEnum)Index;

      if ((S->Index != -1) && 
          (S->Index < MStat.P.MaxIStatCount) && 
          (MPar[0].S.IStat != NULL))
        GS = MPar[0].S.IStat[S->Index];
      else
        GS = &MPar[0].S;

      switch (AIndex)
        {
        case mstaAvgBypass:
          
          {
          double tmpD;

          /* bypass not tracked in object stats */

          tmpD = (double)((GS->JC > 0) ? (double)GS->Bypass / GS->JC : 0.0);

          if (tmpD > 0.0)
            {
            sprintf(Buf,"%.2f",
              tmpD);
            }
          } /* END BLOCK */

          break;

        case mstaAvgQueueTime:
     
          {
          /* avg qtime not tracked in object stats */

          double tmpD = (double)((GS->JC > 0) ? (double)GS->TotalQTS / GS->JC : 0.0);

          if (tmpD > 0.0)
            sprintf(Buf,"%.2f",tmpD);
          } /* END BLOCK */

          break;

        case mstaAvgXFactor:

          {
          double tmpD = (double)((GS->PSRun > 0.0) ? GS->PSXFactor / GS->PSRun : 0.0);

          if (tmpD > 0.0)
            sprintf(Buf,"%.2f",tmpD);
          } /* END BLOCK */

          break;

        case mstaTCapacity:

          if (S->TCapacity > 0)
            {
            sprintf(Buf,"%ld",
              S->TCapacity);
            }

          break;

        case mstaDuration:

          if (S->Duration > 0)
            {
            sprintf(Buf,"%ld",
              S->Duration);
            }
          else if ((GS != NULL) && (GS->Duration > 0))
            {
            sprintf(Buf,"%ld",
              GS->Duration);
            }
          else 
            {
            sprintf(Buf,"%ld",
              MStat.P.IStatDuration - 
              (MStat.P.IStatEnd - MSched.Time)); 
            }

          break;

        case mstaIterationCount:

          /* number of iterations recorded in profile */

          if ((S->IterationCount > 0) || (S->IsProfile == TRUE))
            {
            sprintf(Buf,"%d",
              S->IterationCount);
            }
          else if ((GS != NULL) && (GS->IterationCount > 0) && (GS->StartTime == S->StartTime))
            {
            sprintf(Buf,"%d",
              GS->IterationCount);
            }

          break;

        case mstaQOSCredits:

          if (S->QOSCredits > 0.0)
            {
            sprintf(Buf,"%.2f",
              S->QOSCredits);
            }

          break;

        case mstaTNL:

          if (S->TNL > 0.0)
            {
            sprintf(Buf,"%.2f",
              S->TNL);
            }

          break;

        case mstaTNM:

          if (S->TNM > 0.0)
            {
            sprintf(Buf,"%.2f",
              S->TNM);
            }

          break;

        case mstaTANodeCount:

          if (S->ActiveNC > 0)
            {
            sprintf(Buf,"%d",
              S->ActiveNC);
            }

          break;

        case mstaTAProcCount:

          if (S->ActivePC > 0)
            {
            sprintf(Buf,"%d",
              S->ActivePC);
            }

          break;

        case mstaTNC:

          /* return global information if local override not specified */

          if (S->TNC > 0)
            {
            sprintf(Buf,"%d",
              S->TNC);
            }
          else if ((GS != NULL) && (GS->TNC > 0) && (GS->StartTime == S->StartTime))
            {
            sprintf(Buf,"%d",
              GS->TNC);
            }

          break;

        case mstaTPC:

          /* return global information if local override not specified */

          if (S->TPC > 0)
            {
            sprintf(Buf,"%d",
              S->TPC);
            }
          else if ((GS != NULL) && (GS->TPC > 0) && (GS->StartTime == S->StartTime))
            {
            sprintf(Buf,"%d",
              GS->TPC);
            }
          else
            {
            sprintf(Buf,"%d",
              (int)(MPar[0].UpRes.Procs * (MStat.P.IStatDuration)/ MAX(1,MSched.MaxRMPollInterval)));
            }

          break;

        case mstaTJobCount:

          if (S->JC > 0)
            { 
            sprintf(Buf,"%d",
              S->JC);
            }
     
          break;
     
        case mstaTNJobCount:

          if (S->JAllocNC > 0) 
            {
            sprintf(Buf,"%d",
              S->JAllocNC); 
            }
     
          break;
     
        case mstaTQueueTime:

          if (S->TotalQTS > 0)
            { 
            sprintf(Buf,"%ld",
              S->TotalQTS);
            }

          break;
     
        case mstaMQueueTime:

          if (S->MaxQTS > 0)
            { 
            sprintf(Buf,"%ld",
              S->MaxQTS);
            }
     
          break;

        case mstaTDSAvl:

          if (S->DSAvail > 0.0)
            {
            sprintf(Buf,"%.2f",
              S->DSAvail);
            }

          break;

        case mstaTDSDed:

          /* NYI */

          break;

        case mstaTDSUtl:

          if (S->DSUtilized > 0.0)
            {
            sprintf(Buf,"%.2f",
              S->DSUtilized);
            }

          break;

        case mstaIDSUtl:

          if (S->IDSUtilized > 0.0)
            {
            sprintf(Buf,"%.2f",
              S->IDSUtilized);
            }

          break;

        case mstaTReqWTime:

          if (S->TotalRequestTime > 0.0)
            { 
            sprintf(Buf,"%.2f",
              S->TotalRequestTime);
            }

          break;
     
        case mstaTExeWTime:

          if (S->TotalRunTime > 0.0)
            { 
            sprintf(Buf,"%.2f",
              S->TotalRunTime);
            }

          break;

        case mstaTMSAvl:

          if (S->MSAvail > 0.0)
            {
            sprintf(Buf,"%.2f",
              S->MSAvail);
            }

          break;

        case mstaTMSDed:

          if (S->MSDedicated > 0.0)
            {
            sprintf(Buf,"%.2f",
              S->MSDedicated);
            }

          break;

        case mstaTMSUtl:

          if (S->MSUtilized > 0.0)
            {
            sprintf(Buf,"%.2f",
              S->MSUtilized);
            }

          break;

        case mstaTPSReq:

          if (S->PSRequest > 0.0)
            { 
            sprintf(Buf,"%.2f",
              S->PSRequest);
            }

          break; 
     
        case mstaTPSExe:

          if (S->PSRun > 0.0)
            { 
            sprintf(Buf,"%.2f",
              S->PSRun);
            }

          break;
     
        case mstaTPSDed:

          if (S->PSDedicated > 0.0) 
            {
            sprintf(Buf,"%.2f",
              S->PSDedicated);
            }
     
          break;
     
        case mstaTPSUtl:

          if (S->PSUtilized > 0.0)
            { 
            sprintf(Buf,"%.2f",
              S->PSUtilized);
            }

          break;
     
        case mstaTJobAcc:

          if (S->JobAcc > 0.0)
            { 
            sprintf(Buf,"%.2f",
              S->JobAcc);
            }

          break;
     
        case mstaTNJobAcc:

          if (S->NJobAcc > 0.0)
            { 
            sprintf(Buf,"%.2f",
              S->NJobAcc);
            }

          break;
     
        case mstaTXF: 

          if (S->XFactor > 0.0)
            { 
            sprintf(Buf,"%.2f",
              S->XFactor);
            }

          break;
     
        case mstaTNXF:

          if (S->NXFactor > 0.0)
            { 
            sprintf(Buf,"%.2f",
              S->NXFactor);
            }

          break;
     
        case mstaMXF:

          if (S->MaxXFactor > 0.0)
            { 
            sprintf(Buf,"%.2f",
              S->MaxXFactor);
            }

          break;
     
        case mstaTBypass:

          if (S->Bypass > 0)
            { 
            sprintf(Buf,"%d",
              S->Bypass);
            }

          break;
     
        case mstaMBypass:

          if (S->MaxBypass > 0)
            { 
            sprintf(Buf,"%d",
              S->MaxBypass);
            }

          break;
     
        case mstaTQOSAchieved:

          if (S->QOSSatJC > 0)
            { 
            sprintf(Buf,"%d", 
              S->QOSSatJC);
            }

          break;

        case mstaTSubmitJC:

          if (S->SubmitJC > 0)
            {
            sprintf(Buf,"%d",
              S->SubmitJC);
            }

          break;

        case mstaTSubmitPH:

          if (S->SubmitPHRequest > 0)
            {
            sprintf(Buf,"%.2f",
              S->SubmitPHRequest);
            }

          break;

        case mstaTStartJC:

          if (S->StartJC > 0)
            {
            sprintf(Buf,"%d",
              S->StartJC);
            }

          break;

        case mstaTStartPC:

          if (S->StartPC > 0)
            {
            sprintf(Buf,"%d",
              S->StartPC);
            }

          break;

        case mstaTStartQT:

          if (S->StartQT > 0)
            {
            sprintf(Buf,"%ld",
              S->StartQT);
            }

          break;

        case mstaTStartXF:

          if (S->StartXF > 0.0)
            {
            sprintf(Buf,"%.2f",
              S->StartXF);
            }

          break;

        case mstaStartTime:

          /* NOTE:  report context sensitve data, if pointing to global partition,
                    report time when scheduler started, otherwise report when stat
                    profile started or when stats were initialized. */

          /* NOTE:  reporting MSched.StartTime required by StattTime field in 
                    'showstats -v' */

          if ((S == GS) && (S->IsProfile == FALSE))
            {
            sprintf(Buf,"%ld",
              MSched.StartTime);
            }
          else 
            {
            /* S cannot be NULL here, alias checked at top of block */

            sprintf(Buf,"%ld",
              S->StartTime);
            }

          break;

        case mstaIStatCount:

          if ((MStat.P.MaxIStatCount > 0))
            {
            sprintf(Buf,"%d",
              MStat.P.MaxIStatCount);
            }

          break;

        case mstaIStatDuration:

          if ((MStat.P.IStatDuration > 0))
            {
            sprintf(Buf,"%d",
              MStat.P.IStatDuration);
            }

          break;

        case mstaGCEJobs:      /* current eligible jobs */

          if ((MStat.EligibleJobs > 0))
            {
            sprintf(Buf,"%d",
              MStat.EligibleJobs);
            }

          break;

        case mstaGCIJobs:      /* current idle jobs */

          if ((MStat.IdleJobs > 0))
            {
            sprintf(Buf,"%d",
              MStat.IdleJobs);
            }

          break;

        case mstaGCAJobs:      /* current active jobs */

          if ((MStat.ActiveJobs > 0))
            {
            sprintf(Buf,"%d",
              MStat.ActiveJobs);
            }
     
          break;

        case mstaGMetric:

          /* see mstaXLoad */

          if ((S->XLoad != NULL) /* FIXME: this is a temporary segfault avoider */
              && (S->XLoad[SIndex] != 0.0) &&
              (S->XLoad[SIndex] != MDEF_GMETRIC_VALUE))
            {
            sprintf(Buf,"%.2f",
              S->XLoad[SIndex]);
            }

          break;

        case mstaGPHAvl:

          if ((MStat.TotalPHAvailable > 0.0))
            {
            sprintf(Buf,"%.2f",
              MStat.TotalPHAvailable);
            }

          break;

        case mstaGPHUtl:

          if ((MStat.TotalPHBusy > 0.0))
            {
            sprintf(Buf,"%.2f",
              MStat.TotalPHBusy);
            }

          break;

        case mstaGPHDed:

          if ((MStat.TotalPHDed > 0.0))
            {
            sprintf(Buf,"%.2f",
              MStat.TotalPHDed);
            }

          break;

        case mstaGPHSuc:

          if ((MStat.SuccessfulPH > 0.0))
            {
            sprintf(Buf,"%.2f",
              MStat.SuccessfulPH);
            }

          break;

        case mstaGMinEff:

          if ((MStat.MinEff > 0.0))
            {
            sprintf(Buf,"%.2f",
              MStat.MinEff);
            }

          break;

        case mstaGMinEffIteration:

          if ((MStat.MinEffIteration > 0))
            {
            sprintf(Buf,"%d",
              MStat.MinEffIteration);
            }

          break;

        case mstaTPHPreempt:

          if ((MStat.TotalPHPreempted > 0.0))
            {
            sprintf(Buf,"%.2f",
              MStat.TotalPHPreempted);
            }

          break;

        case mstaTJPreempt:

          if ((MStat.TotalPreemptJobs > 0))
            {
            sprintf(Buf,"%d",
              MStat.TotalPreemptJobs);
            }

          break;

        case mstaTJEval:

          if ((MStat.EvalJC > 0))
            {
            sprintf(Buf,"%d",
              MStat.EvalJC);
            }

          break;

        case mstaTQueuedPH:

          if (S->TQueuedPH > 0.0)
            {
            sprintf(Buf,"%.2f",
              S->TQueuedPH);
            }

          break;

        case mstaTQueuedJC:

          if (S->TQueuedJC > 0)
            {
            sprintf(Buf,"%d",
              S->TQueuedJC);
            }

          break;

        case mstaSchedDuration:

          if ((MStat.SchedRunTime > 0))
            {
            sprintf(Buf,"%ld",
              MStat.SchedRunTime);
            }

          break;

        case mstaSchedCount:

          sprintf(Buf,"%d",
            MSched.Iteration);

          break;

        case mstaXLoad:

          if ((S->XLoad[SIndex] != 0.0) && (S->XLoad[SIndex] != MDEF_GMETRIC_VALUE))
            {
            sprintf(Buf,"%.2f",
              S->XLoad[SIndex]);
            }

          break;

        default:

          /* not handled */

          return(FAILURE);

          /*NOTREACHED*/
     
          break;
        }  /* END switch (AIndex) */
      }    /* END BLOCK (case msoCred) */

      break;

    case msoNode:
 
      {
      mnust_t *S = (mnust_t *)Stat;
 
      enum MNodeStatTypeEnum AIndex = (enum MNodeStatTypeEnum)Index;

      if ((S->Index != -1) && 
          (S->Index < MStat.P.MaxIStatCount) && 
          (MPar[0].S.IStat != NULL))
        GS = MPar[0].S.IStat[S->Index];
      else
        GS = &MPar[0].S;

      switch (AIndex)
        {
        case mnstAGRes:

          if ((SIndex < MSched.M[mxoxGRes]) && 
              (MSNLGetIndexCount(&S->AGRes,SIndex) > 0))
            {
            sprintf(Buf,"%d",
              MSNLGetIndexCount(&S->AGRes,SIndex));
            }
     
          break;

        case mnstAProc:

          if (S->AProc > 0)
            {
            sprintf(Buf,"%d",
              S->AProc);
            }

          break;
 
        case mnstCat:

          if (S->Cat != mncLAST)
            {
            sprintf(Buf,"%s",
              MNodeCat[S->Cat]);
            }

          break;

        case mnstCGRes:

          if ((SIndex < MSched.M[mxoxGRes]) &&
              (MSNLGetIndexCount(&S->CGRes,SIndex) > 0))
            {
            sprintf(Buf,"%d",
              MSNLGetIndexCount(&S->CGRes,SIndex)); 
            }
     
          break;

        case mnstCProc:

          if (S->CProc > 0)
            {
            sprintf(Buf,"%d",
              S->CProc);
            }

          break;

        case mnstCPULoad:

          if (S->CPULoad > 0.0)
            {
            sprintf(Buf,"%.2f",
              S->CPULoad);
            }

          break;
 
        case mnstDGRes:

          if ((SIndex < MSched.M[mxoxGRes]) &&
              (MSNLGetIndexCount(&S->DGRes,SIndex) > 0))
            {
            sprintf(Buf,"%d",
              MSNLGetIndexCount(&S->DGRes,SIndex)); 
            }
     
          break;

        case mnstDProc:

          if (S->DProc > 0)
            {
            sprintf(Buf,"%d",
              S->DProc);
            }

          break;

        case mnstFailureJC:

          if (S->FailureJC > 0)
            {
            sprintf(Buf,"%d",
              S->FailureJC);
            }
 
          break;

        case mnstGMetric:

          /* see mnstXLoad */

          if ((S->XLoad[SIndex] != 0.0) && (S->XLoad[SIndex] != MDEF_GMETRIC_VALUE))
            {
            sprintf(Buf,"%.2f",
              S->XLoad[SIndex]);
            }

          break;
 
        case mnstIStatCount:

          if (MStat.P.MaxIStatCount > 0)
            {
            sprintf(Buf,"%d",
              MStat.P.MaxIStatCount);
            }

          break;

        case mnstIStatDuration:

          if ((MStat.P.IStatDuration > 0))
            {
            sprintf(Buf,"%d",
              MStat.P.IStatDuration);
            }

          break;

        case mnstMaxCPULoad:

          sprintf(Buf,"%.2f",
            S->MaxCPULoad);

          break;

        case mnstMinCPULoad:

          sprintf(Buf,"%.2f",
            S->MinCPULoad);

          break;

        case mnstMaxMem:

          sprintf(Buf,"%d",
            S->MaxMem);

          break;

        case mnstMinMem:

          sprintf(Buf,"%d",
            S->MinMem);

          break; 

        case mnstQueuetime:

          if (S->Queuetime > 0)
            {
            sprintf(Buf,"%ld",
              S->Queuetime);
            }

          break;

        case mnstStartJC:

          if (S->StartJC > 0)
            {
            sprintf(Buf,"%d",
              S->StartJC);
            }
 
          break;

        case mnstStartTime:

          /* At this point S cannot be NULL. Check at top of function  */

          sprintf(Buf,"%ld", S->StartTime);

          break;

        case mnstIterationCount:

          if (S->IterationCount >= 0)
            {
            sprintf(Buf,"%d",
              S->IterationCount);
            }

          break;

        case mnstNodeState:

          sprintf(Buf,"%s",
            MNodeState[S->NState]);

          break;

        case mnstRCat:

          if (S->RCat != mncLAST)
            {
            sprintf(Buf,"%s",
              MNodeCat[S->RCat]);
            }

          break;

        case mnstSuccessiveJobFailures:

          sprintf(Buf,"%d",
            S->SuccessiveJobFailures);

          break;

        case mnstUMem:

          sprintf(Buf,"%d",
            S->UMem);

          break;

        case mnstXLoad:

          if ((S->XLoad[SIndex] != 0.0) && (S->XLoad[SIndex] != MDEF_GMETRIC_VALUE))
            {
            sprintf(Buf,"%.2f",
              S->XLoad[SIndex]); 
            }
     
          break;

        case mnstXFactor:

          if (S->XFactor != 0.0)
            {
            sprintf(Buf,"%.2f",
              S->XFactor);
            }

        default:

          /* NO-OP */

          break;
        }  /* END switch (AIndex) */
      }    /* END case msoNode */

      break;

    default:

      /* NO-OP */

      break;
    }   /* END switch (StatType) */
 
  return(SUCCESS);
  }  /* END MStatAToString() */

/* END MStatAToString.c */
