/* HEADER */

      
/**
 * @file MStatSetAttr.c
 *
 * Contains: MStatSetAttr
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 * Set specified stat attribute.
 *
 * @param Stat (I) [modified]
 * @param StatType (I)
 * @param SIndex (I)
 * @param Value (I)
 * @param StatIndex (I) [used for indexed stats like GRes]
 * @param Format (I) [not used]
 * @param Mode (I) [not used]
 */

int MStatSetAttr(

  void                  *Stat,      /* I (modified) */
  enum MStatObjectEnum   StatType,  /* I */
  int                    SIndex,    /* I */
  void                 **Value,     /* I */
  int                    StatIndex, /* I (used for indexed stats like GRes) */
  enum MDataFormatEnum   Format,    /* I (not used) */
  int                    Mode)      /* I (not used) */
 
  {
  const char *FName = "MStatSetAttr";

  MDB(7,fSCHED) MLog("%s(Stat,%s,%d,Value,%d,%d,%d)\n",
    FName,
    MStatObject[StatType],
    SIndex,
    StatIndex,
    Format,
    Mode);

  if ((Stat == NULL) || (Value == NULL))
    {
    return(FAILURE);
    }

  switch (StatType)
    {
    case msoCred:

      {
      must_t *S = (must_t *)Stat;

      enum MStatAttrEnum AIndex = (enum MStatAttrEnum)SIndex;

      switch (AIndex) 
        {
        case mstaTNL:

          S->TNL = strtod((char *)Value,NULL);

          break;

        case mstaTNM:

          S->TNM = strtod((char *)Value,NULL);

          break;

        case mstaTCapacity:

          S->TCapacity = strtol((char *)Value,NULL,10);

          break;

        case mstaTMSUtl:

          S->MSUtilized = strtod((char *)Value,NULL);

          break;

        case mstaDuration:

          S->Duration = strtol((char *)Value,NULL,10);

          break;

        case mstaIterationCount:

          S->IterationCount = (int)strtol((char *)Value,NULL,10);

          break;

        case mstaQOSCredits:

          S->QOSCredits = strtod((char *)Value,NULL);

          break;

        case mstaTANodeCount:

          S->ActiveNC = (int)strtol((char *)Value,NULL,10);

          break;

        case mstaTAProcCount:

          S->ActivePC = (int)strtol((char *)Value,NULL,10);

          break;

        case mstaTJobCount:
     
          S->JC = (int)strtol((char *)Value,NULL,10);
     
          break;
     
        case mstaTNJobCount:
     
          S->JAllocNC = (int)strtol((char *)Value,NULL,10);
     
          break;
     
        case mstaTQueueTime:
     
          S->TotalQTS = strtol((char *)Value,NULL,10);
     
          break;
     
        case mstaMQueueTime:
     
          S->MaxQTS = strtol((char *)Value,NULL,10);
     
          break;

        case mstaTDSAvl:

          S->DSAvail = strtod((char *)Value,NULL);

          break;

        case mstaTDSDed:

          /* NYI */

          break;

        case mstaTDSUtl:

          S->DSUtilized = strtod((char *)Value,NULL);

          break;

        case mstaIDSUtl:

          S->IDSUtilized = strtod((char *)Value,NULL);

          break;

        case mstaTNC:

          S->TNC = (int)strtol((char *)Value,NULL,10);

          break;

        case mstaTPC:

          S->TPC = (int)strtol((char *)Value,NULL,10);

          break;
     
        case mstaTReqWTime:
     
          S->TotalRequestTime = strtod((char *)Value,NULL);
     
          break;

        case mstaTExeWTime:
     
          S->TotalRunTime = strtod((char *)Value,NULL);
     
          break;

        case mstaTMSAvl:

          S->MSAvail = strtod((char *)Value,NULL);  

          break;

        case mstaTMSDed:

          S->MSDedicated = strtod((char *)Value,NULL);
     
          break;
          
        case mstaTPSReq:
     
          S->PSRequest = strtod((char *)Value,NULL);
     
          break;
     
        case mstaTPSExe:
     
          S->PSRun = strtod((char *)Value,NULL);
     
          break;
     
        case mstaTPSDed:
     
          S->PSDedicated = strtod((char *)Value,NULL);
     
          break;
     
        case mstaTPSUtl:
     
          S->PSUtilized = strtod((char *)Value,NULL);
     
          break;
     
        case mstaTJobAcc:
     
          S->JobAcc = strtod((char *)Value,NULL);
     
          break;
     
        case mstaTNJobAcc: 
     
          S->NJobAcc = strtod((char *)Value,NULL);
     
          break;
     
        case mstaTXF:
     
          S->XFactor = strtod((char *)Value,NULL);
     
          break;
     
        case mstaTNXF:
     
          S->NXFactor = strtod((char *)Value,NULL);
     
          break;
     
        case mstaMXF:
     
          S->MaxXFactor = strtod((char *)Value,NULL);
     
          break;
     
        case mstaTBypass:
     
          S->Bypass = (int)strtol((char *)Value,NULL,10);
     
          break;
     
        case mstaMBypass:
     
          S->MaxBypass = (int)strtol((char *)Value,NULL,10);
     
          break;
     
        case mstaTQOSAchieved: 
     
          S->QOSSatJC = (int)strtol((char *)Value,NULL,10);
     
          break;

        case mstaStartTime:

          MStat.InitTime = strtol((char *)Value,NULL,10);

          break;

        case mstaTSubmitJC:

          S->SubmitJC = (int)strtol((char *)Value,NULL,10);

          break;

        case mstaTSubmitPH:

          S->SubmitPHRequest = strtod((char *)Value,NULL);

          break;

        case mstaTStartJC:

          S->StartJC = (int)strtol((char *)Value,NULL,10);

          break;

        case mstaTStartPC:

          S->StartPC = (int)strtol((char *)Value,NULL,10);

          break;

        case mstaTStartQT:

          S->StartQT = strtol((char *)Value,NULL,10);

          break;

        case mstaTStartXF:

          S->StartXF = strtod((char *)Value,NULL);

          break;

        case mstaIStatCount:

          MStat.P.MaxIStatCount = (int)strtol((char *)Value,NULL,10);

          break;

        case mstaTQueuedJC:

          S->TQueuedJC = (int)strtol((char *)Value,NULL,10);

          break;

        case mstaTQueuedPH:

          S->TQueuedPH = strtod((char *)Value,NULL);

          break;

        case mstaGMetric:

          if (StatIndex >= 0)
            {
            double tmpD = strtod((char *)Value,NULL);

            if ((tmpD != MDEF_GMETRIC_VALUE) && (S->XLoad != NULL))
              S->XLoad[StatIndex] = tmpD;
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

      enum MNodeStatTypeEnum AIndex = (enum MNodeStatTypeEnum)SIndex;
 
      switch (AIndex)
        {
        case mnstAProc:

          S->AProc = (int)strtol((char *)Value,NULL,10);

          break;

        case mnstCat:

          S->Cat = (enum MNodeCatEnum)MUGetIndex((char *)Value,MNodeCat,FALSE,mncNONE);

          break;

        case mnstAGRes:
        case mnstCGRes:
        case mnstDGRes:

          {
          msnl_t *GRes;

          int NewVal;

          MASSERT(StatIndex > 0,"Invalid index when setting stat attribute");

          switch (AIndex)
            {
            case mnstAGRes:

              GRes = &S->AGRes;

              break;

            case mnstCGRes:

              GRes = &S->CGRes;

              break;

            case mnstDGRes:

              GRes = &S->DGRes;

              break;

            default:

              assert(FALSE);

              /* impossible to reach, but just in case! */

              GRes = &S->CGRes;

              break;
            }   /* END switch (AIndex) */

          NewVal = (int)strtol((char *)Value,NULL,10);

          MSNLSetCount(GRes,StatIndex,NewVal);
          }
     
          break;

        case mnstCProc:

          S->CProc = (int)strtol((char *)Value,NULL,10);

          break;

        case mnstCPULoad:

          S->CPULoad = strtod((char *)Value,NULL);

          break;

        case mnstDProc:

          S->DProc = (int)strtol((char *)Value,NULL,10);

          break;

        case mnstFailureJC:

          S->FailureJC = (int)strtol((char *)Value,NULL,10);

          break;

        case mnstGMetric:

          if (StatIndex > 0)
            {
            double tmpD = strtod((char *)Value,NULL);

            if (tmpD != MDEF_GMETRIC_VALUE)
              S->XLoad[StatIndex] = tmpD;
            }

          break;

        case mnstIterationCount:

          S->IterationCount = (int)strtol((char *)Value,NULL,10);

          break;

        case mnstMaxCPULoad:

          S->MaxCPULoad = strtod((char *)Value,NULL);
 
          break;

        case mnstMinCPULoad:

          S->MinCPULoad = strtod((char *)Value,NULL);

          break;

        case mnstMaxMem:

          S->MaxMem = (int)strtol((char *)Value,NULL,10);

          break;

        case mnstMinMem:

          S->MinMem = (int)strtol((char *)Value,NULL,10);

          break;

        case mnstQueuetime:

          S->Queuetime = strtol((char *)Value,NULL,10);

          break;
 
        case mnstRCat:

          S->RCat = (enum MNodeCatEnum)MUGetIndex((char *)Value,MNodeCat,FALSE,mncNONE);

          break;

        case mnstStartJC:

          S->StartJC = (int)strtol((char *)Value,NULL,10);

          break;
 
        case mnstStartTime:

          MStat.InitTime = strtol((char *)Value,NULL,10);

          break;

        case mnstIStatCount:

          MStat.P.MaxIStatCount = (int)strtol((char *)Value,NULL,10);

          break;

        case mnstNodeState:

          S->NState = (enum MNodeStateEnum)MUGetIndex((char *)Value,MNodeState,FALSE,mnsNONE);

          break;

        case mnstSuccessiveJobFailures:

          S->SuccessiveJobFailures = (int)strtol((char *)Value,NULL,10);

          break;

        case mnstUMem:

          S->UMem = (int)strtol((char *)Value,NULL,10);

          break;

        case mnstXLoad:

          {
          double tmpD = strtod((char *)Value,NULL);

          if (tmpD != MDEF_GMETRIC_VALUE)
            S->XLoad[StatIndex] = tmpD;
          }

          break;

        case mnstXFactor:

          S->XFactor = strtod((char *)Value,NULL);

          break;

        default:

          /* NYI */

          break;
        }  /* END switch (AIndex) */
      }    /* END BLOCK (case msoNode) */

      break;

    default:

      /* NO-OP */

      break;
    }  /* END switch (StatType) */
 
  return(SUCCESS);
  }  /* END MStatSetAttr() */

/* END MStatSetAttr.c */
