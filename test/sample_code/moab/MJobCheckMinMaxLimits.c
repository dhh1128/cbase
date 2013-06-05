/* HEADER */

      
/**
 * @file MJobCheckMinMaxLimits.c
 *
 * Contains:
 *     MjobCheckJMinLimits
 *     MjobCheckJMaxLimits
 */


#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 * Check job against 'max' job template contraints.
 *
 * NOTE:  also enforces user/group WCLimits with any OIndex - FIXME 
 *
 * Supported Attributes:
 * - WCLIMIT
 * - NODES
 * - TASKS/PROCS
 * - tmpTPN
 * - tmpPS
 * - RMEM
 * - FLAGS
 * - user, group, account, class, qos
 * - exec
 *
 * @see MJobCheckJMinLimits()
 * @see MJobGetTemplate()
 *
 * @param J (I)
 * @param OIndex (I)
 * @param OName (I)
 * @param JT (I) [optional]
 * @param DefaultMaxJob (I) [optional]
 * @param AdjustJob (I)
 * @param EMsg (O) [optional]
 * @param EMsgSize (I) [optional]
 */

int MJobCheckJMaxLimits(

  mjob_t     *J,
  enum MXMLOTypeEnum OIndex,
  const char *OName,
  mjob_t     *JT,
  mjob_t     *DefaultMaxJob,
  mbool_t     AdjustJob,
  char       *EMsg,
  int         EMsgSize)

  {
  int GPUIndex;

  /* limits */

  long tmpWCLimit;
  mutime tmpCPULimit;
  int  tmpGPU;
  int  tmpNode;
  int  tmpProc;
  int  tmpTPN;
  int  tmpPS;
  int  tmpMem;

  enum MXMLOTypeEnum  WCLimitOType;
  char                WCLimitOID[MMAX_NAME];

  char tmpOID[MMAX_NAME];

  mbitmap_t tmpFlags;

  int  MinReqNC;  /* nodes requested by job - only count compute nodes */
  int  MinReqTC;  /* tasks requested by job - only count compute tasks */
  int  MinReqMem; /* mem requested by job - only check Req[0] */
 
  macl_t *CList;
  macl_t *maclPtr;

  mcres_t *tmpDRes;

  char *tmpCmd;

  const char *ODesc;

  mjob_t *JL;

  const char *FName = "MJobCheckJMaxLimits";

  MDB(7,fSCHED) MLog("%s(%s,%s,%s,JT,DefaultMaxJob,%s,EMsg,EMsgSize)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    MXO[OIndex],
    (OName != NULL) ? OName : "NULL",
    MBool[AdjustJob]);

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (J == NULL)
    {
    return(SUCCESS);
    }

  if (OIndex == mxoClass)
    {
    ODesc = "class";
    }
  else if (OIndex == mxoUser)
    {
    ODesc = "user";
    }
  else
    {
    /* support other types? */

    ODesc = "job template";
    }

  bmclear(&tmpFlags);

  GPUIndex = MUMAGetIndex(meGRes,MCONST_GPUS,mAdd);

  tmpWCLimit  = 0;
  tmpCPULimit = 0;
  tmpGPU      = -1;
  tmpNode     = 0;  /* nodes requested - only count compute nodes */
  tmpProc     = -1;
  tmpTPN      = 0;
  tmpPS       = 0;
  tmpMem      = -1;

  tmpCmd     = NULL;
  CList      = NULL;
  tmpDRes    = NULL;

  /* if AdjustJob == TRUE, fix job violations */
  /* if AdjustJob == FALSE, return failure on violation */  

  /* extract node count extrema from job */

  if (MSched.BluegeneRM == TRUE)
    {
    MinReqNC = J->Request.TC / MSched.BGNodeCPUCnt;
    }
  else if (J->Request.NC != 0)
    {
    if (J->Req[1] != NULL)
      {
      int rqindex;

      MinReqNC = 0;

      for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
        {
        if (J->Req[rqindex] == NULL)
          break;

        if (J->Req[rqindex]->DRes.Procs != 0)
          MinReqNC += J->Req[rqindex]->NodeCount;
        }  /* END for (rqindex) */
      }
    else
      {
      MinReqNC = J->Request.NC;
      }
    }
  else
    {
    /* NOTE:  should be per req based */

    MJobGetEstNC(J,NULL,&MinReqNC,TRUE);
    }

  MinReqMem = 0;
  MinReqTC  = 0;

  if (J->Req[1] != NULL)
    {
    int rqindex;

    for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
      {
      if (J->Req[rqindex] == NULL)
        break;

      if (J->Req[rqindex]->DRes.Procs != 0)
        MinReqTC += J->Req[rqindex]->TaskCount;

      if (J->Req[rqindex]->DRes.Mem != 0)
        MinReqMem = MAX(MinReqMem,J->Req[rqindex]->DRes.Mem);
      }  /* END for (rqindex) */
    }    /* END if (J->Req[1] != NULL) */
  else if (J->Req[0]->DRes.Procs != 0)
    {
    MinReqTC = J->Request.TC;

    if (J->Req[0]->DRes.Mem != 0)
      MinReqMem = J->Req[0]->DRes.Mem;
    }

  WCLimitOType = OIndex;
  strcpy(WCLimitOID,(OName != NULL)?OName:"");

  if (JT != NULL)
    {
    /* check max limits */

    tmpWCLimit = JT->SpecWCLimit[0];

    tmpCPULimit = JT->CPULimit;

    tmpCmd = JT->Env.Cmd;

    if (JT->Req[0] != NULL)
      {
      tmpMem = JT->Req[0]->DRes.Mem;
      tmpTPN = JT->Req[0]->TasksPerNode;

      if (bmisset(&JT->IFlags,mjifPBSGPUSSpecified))
        tmpGPU = MSNLGetIndexCount(&JT->Req[0]->DRes.GenericRes,GPUIndex);
      }
 
    if ((MinReqTC > 0) && (JT->Req[0] != NULL) && (JT->Req[0]->TaskRequestList[1] > 0))
      {
      int sindex;

      for (sindex = 1;sindex < MMAX_SHAPE;sindex++)
        {
        if (JT->Req[0]->TaskRequestList[sindex] == 0)
          {
          /* end of list reached */

          break;
          }

        if (JT->Req[0]->TaskRequestList[sindex] > MinReqTC)
          {
          /* skipped past applicable values - list is in low to high taskcount order */
 
          break;
          }

        tmpWCLimit = JT->SpecWCLimit[sindex];
        }    /* END for (sindex) */
      }      /* END if ((MinReqTC > 0) && ...) */

    tmpNode    = JT->Request.NC;

    if (bmisset(&JT->IFlags,mjifTasksSpecified) ||
        bmisset(&JT->IFlags,mjifDProcsSpecified))
      {
      tmpProc  = JT->Request.TC;
      }

    tmpPS      = (int)JT->PSDedicated;

    bmcopy(&tmpFlags,&JT->Flags);

    CList      = JT->RequiredCredList;

    if ((JT->Req[0] != NULL) && (OIndex == mxoJob))
      {
      /* This routine is called for class limits after each workload query, but we only want to 
       * check the GRES against the job template once at job submit time. */

      tmpDRes = &JT->Req[0]->DRes;
      }

    if ((JT->Req[0] != NULL) &&
        !bmisclear(&JT->Req[0]->ReqFBM) &&
        MAttrSubset(&J->Req[0]->ReqFBM,&JT->Req[0]->ReqFBM,JT->Req[0]->ReqFMode))
      {
      return(FAILURE);
      }
    }  /* END if (JT != NULL) */

  if ((JT != NULL) && (JT->Variables.Table != NULL) && (J->Variables.Table != NULL))
    {
    char *VarName;
    char *VarVal;

    char *ReqName;
    char *ReqVal;

    mhashiter_t HTI;
    mhashiter_t ReqHTI;

    mbool_t Found;

    /* match variables */

    MUHTIterInit(&ReqHTI);

    while (MUHTIterate(&JT->Variables,&ReqName,(void **)&ReqVal,NULL,&ReqHTI) == SUCCESS)
      {
      Found = FALSE;

      MUHTIterInit(&HTI);

      while (MUHTIterate(&J->Variables,&VarName,(void **)&VarVal,NULL,&HTI) == SUCCESS)
        {
        if (!strcasecmp(VarName,ReqName))
          {
          if ((ReqVal == NULL) ||
                ((VarVal != NULL) &&
                 (!strcasecmp(VarVal,ReqVal))))
            {
            Found = TRUE;

            break;
            }
          }
        }

      if (Found == TRUE)
        {
        /* required variable not found */

        return(FAILURE);
        }
      }  /* END while (MUHTIterate(ReqHTI)) */
    }  /* END if ((JT != NULL) && (JT->Variables.Table != NULL) ... ) */


  /* NOTE:  do not adjust limits if evaluating job again job template */

  if (OIndex != mxoJob)
    {
    /* check user wc limits */

    if ((J->Credential.U != NULL) &&
        (J->Credential.U->L.JobMaximums[0] != NULL))
      {
      JL = J->Credential.U->L.JobMaximums[0];

      strcpy(tmpOID,J->Credential.U->Name);
      }
    else if ((MSched.DefaultU != NULL) &&
             (MSched.DefaultU->L.JobMaximums[0] != NULL))
      {
      JL = MSched.DefaultU->L.JobMaximums[0];

      strcpy(tmpOID,"DEFAULT");
      }
    else
      {
      JL = NULL;
      }

    if (JL != NULL)
      {
      if (tmpWCLimit <= 0)
        {
        /* non-infinite user limit specified */

        WCLimitOType = mxoUser;
        strcpy(WCLimitOID,tmpOID);
 
        tmpWCLimit = (long)JL->SpecWCLimit[0];
        }
      else if (JL->SpecWCLimit[0] > 0)
        {
        if ((long)JL->SpecWCLimit[0] < tmpWCLimit)
          {
          /* user limit is more constraining */

          WCLimitOType = mxoUser;
          strcpy(WCLimitOID,tmpOID);

          tmpWCLimit = (long)JL->SpecWCLimit[0];
          }
        }
      }  /* END if (JL != NULL) */

    /* check group wc limits */

    if ((J->Credential.G != NULL) &&
        (J->Credential.G->L.JobMaximums[0] != NULL))
      {
      JL = J->Credential.G->L.JobMaximums[0];

      strcpy(tmpOID,J->Credential.G->Name);
      }
    else if ((MSched.DefaultG != NULL) &&
             (MSched.DefaultG->L.JobMaximums[0] != NULL))
      {
      JL = MSched.DefaultG->L.JobMaximums[0];

      strcpy(tmpOID,"DEFAULT");
      }
    else
      {
      JL = NULL;
      }

    if (JL != NULL)
      {
      if (tmpWCLimit <= 0)
        {
        /* non-infinite group limit specified */
 
        WCLimitOType = mxoGroup;
        strcpy(WCLimitOID,tmpOID);
  
        tmpWCLimit = (long)JL->SpecWCLimit[0];
        }
      else if (JL->SpecWCLimit[0] > 0)
        {
        if ((long)JL->SpecWCLimit[0] < tmpWCLimit)
          {
          /* group limit is more constraining */

          WCLimitOType = mxoGroup;
          strcpy(WCLimitOID,tmpOID);

          tmpWCLimit = (long)JL->SpecWCLimit[0];
          }
        }
      }  /* END if (JL != NULL) */

    /* apply QOS overrides if specified */

    if ((J->Credential.Q != NULL) &&
        (J->Credential.Q->L.OverriceJobPolicy != NULL))
      {
      if (J->Credential.Q->L.OverriceJobPolicy->HLimit[mptMaxWC][0] != 0)
        {
        if (J->Credential.Q->L.OverriceJobPolicy->HLimit[mptMaxWC][0] == -1)
          {
          tmpWCLimit = 0;
          }
        else
          {
          WCLimitOType = mxoQOS;
          strcpy(WCLimitOID,J->Credential.Q->Name);

          tmpWCLimit = J->Credential.Q->L.OverriceJobPolicy->HLimit[mptMaxWC][0];
          }
        }

      if (J->Credential.Q->L.OverriceJobPolicy->HLimit[mptMaxNode][0] != 0)
        {
        if (J->Credential.Q->L.OverriceJobPolicy->HLimit[mptMaxNode][0] == -1)
          tmpNode = 0;
        else
          tmpNode = J->Credential.Q->L.OverriceJobPolicy->HLimit[mptMaxNode][0];
        }

      if (J->Credential.Q->L.OverriceJobPolicy->HLimit[mptMaxProc][0] != 0)
        {
        if (J->Credential.Q->L.OverriceJobPolicy->HLimit[mptMaxProc][0] == -1)
          tmpProc = 0;
        else
          tmpProc = J->Credential.Q->L.OverriceJobPolicy->HLimit[mptMaxProc][0];
        }

      if (J->Credential.Q->L.OverriceJobPolicy->HLimit[mptMaxPS][0] != 0)
        {
        if (J->Credential.Q->L.OverriceJobPolicy->HLimit[mptMaxPS][0] == -1)
          tmpPS = 0;
        else
          tmpPS = J->Credential.Q->L.OverriceJobPolicy->HLimit[mptMaxPS][0];
        }
      }    /* END if ((J->Cred.Q != NULL) && ...) */
    }      /* END if (OIndex != mxoJob) */

  if (DefaultMaxJob != NULL)
    {
    if (tmpWCLimit == 0)
      tmpWCLimit = DefaultMaxJob->SpecWCLimit[0];

    if (tmpNode == 0)
      tmpNode = DefaultMaxJob->Request.NC;

    if (tmpProc == 0)
      tmpProc = DefaultMaxJob->Request.TC;

    if (tmpPS == 0)
      tmpPS = (int)DefaultMaxJob->PSDedicated;

    if (DefaultMaxJob->Req[0] != NULL)
      {
      if (tmpTPN == 0)
        {
        tmpTPN = DefaultMaxJob->Req[0]->TasksPerNode;
        }
      }

    if (bmisclear(&tmpFlags))
      bmcopy(&tmpFlags,&DefaultMaxJob->Flags);

    if (tmpCmd == NULL)
      tmpCmd = DefaultMaxJob->Env.Cmd;

    if (DefaultMaxJob->Req[0] != NULL)
      tmpMem = DefaultMaxJob->Req[0]->DRes.Mem;

    }  /* END if (DefaultMaxJob != NULL) */

  /* NOTE:  need complete method of obtaining 'most constrained' 
            cred limit (NYI) */

  if (CList != NULL)
    {

    if (J->Credential.U != NULL)
      {
      /* check exclude user list */

      for (maclPtr = CList;maclPtr != NULL;maclPtr = maclPtr->Next)
        {
        if (maclPtr->Type != maUser)
          continue;

        if (!strcmp(maclPtr->Name,J->Credential.U->Name))
          {
          if (EMsg != NULL)
            {
            sprintf(EMsg,"user %s excluded from %s",
              J->Credential.U->Name,
              ODesc);
            }

          return(FAILURE);
          }  /* END if (!strcmp(CList[cindex].Name,J->Cred.U->Name)) */
        }    /* END for maclPtr != NULL */
      }      /* END if (J->Cred.U != NULL) */

    if (J->Credential.G != NULL)
      {
      /* check exclude group list */

      for (maclPtr = CList;maclPtr != NULL;maclPtr = maclPtr->Next)
        {
        if (maclPtr->Type != maGroup)
          continue;

        if (!strcmp(maclPtr->Name,J->Credential.G->Name))
          {
          if (EMsg != NULL)
            {
            sprintf(EMsg,"group %s excluded from %s",
              J->Credential.G->Name,
              ODesc);
            }

          return(FAILURE);
          } /* END if (strcmp(CList[cindex].Name,J->Cred.G->Name)) */
        }  /* END for maclPtr */
      }    /* END if (J->Cred.G != NULL) */

    if (J->Credential.A != NULL)
      {
      /* check exclude account list */

      for (maclPtr = CList;maclPtr != NULL;maclPtr = maclPtr->Next)
        {
        if (maclPtr->Type != maAcct)
          continue;

        if (!strcmp(maclPtr->Name,J->Credential.A->Name))
          {
          if (EMsg != NULL)
           {
           sprintf(EMsg,"account %s excluded from %s",
             J->Credential.A->Name,
             ODesc);
           }

          return(FAILURE);
          } /* END if  (!strcmp(CList[cindex].Name,J->Cred.A->Name)) */
        }  /* END for maclPtr */
      }    /* END if (J->Cred.A != NULL) */

    if (J->Credential.C != NULL)
      {
      /* check exclude class list */

      for (maclPtr = CList;maclPtr != NULL;maclPtr = maclPtr->Next)
        {
        if (maclPtr->Type != maClass)
          continue;

        if (!strcmp(maclPtr->Name,J->Credential.C->Name))
          {
          if (EMsg != NULL)
            {
            sprintf(EMsg,"class %s excluded from %s",
              J->Credential.C->Name,
              ODesc);
            }

          return(FAILURE);
          } /* END if (!strcmp(CList[cindex].Name,J->Cred.C->Name)) */
        }  /* END for maclPtr */
      }    /* END if (J->Cred.C != NULL) */

    if (J->Credential.Q != NULL)
      {
      /* check exclude qos list */

      for (maclPtr = CList;maclPtr != NULL;maclPtr = maclPtr->Next)
        {
        if (maclPtr->Type != maQOS)
          continue;

        if (!strcmp(maclPtr->Name,J->Credential.Q->Name))
          {
          if (EMsg != NULL)
            {
            sprintf(EMsg,"QoS %s excluded from %s",
              J->Credential.Q->Name,
              ODesc);
            }

          return(FAILURE);
          } /* END if (!strcmp(CList[cindex].Name,J->Cred.Q->Name)) */
        }   /* END for maclPtr */
      }     /* END if (J->Cred.Q != NULL) */
    }       /* END if (CList != NULL) */

  if (tmpCmd != NULL)
    {
    if ((J->Env.Cmd != NULL) && !strncmp(tmpCmd,J->Env.Cmd,strlen(tmpCmd)))
      {
      if (EMsg != NULL)
        {
        sprintf(EMsg,"required cmd matches (%s == %s) for %s '%s'",
          J->Env.Cmd,
          tmpCmd,
          ODesc,
          OName);
        }

      return(FAILURE);
      }
    }  /* END if (tmpCmd != NULL) */

  if (tmpWCLimit > 0)
    {
    /* check max WC limit */

    if (((long)J->SpecWCLimit[1] > tmpWCLimit) || 
        ((long)J->SpecWCLimit[1] == -1))
      {
      if ((AdjustJob == TRUE) || (bmisset(&J->IFlags,mjifWCNotSpecified)))
        {
        MJobSetAttr(J,mjaReqAWDuration,(void **)&tmpWCLimit,mdfLong,mSet);
        }
      else
        {
        if (EMsg != NULL)
          {
          sprintf(EMsg,"wclimit too high for %s '%s' (%ld > %ld)",
            MXO[WCLimitOType],
            WCLimitOID,
            J->SpecWCLimit[1],
            tmpWCLimit);
          }

        return(FAILURE);
        }
      }
    }    /* END if (tmpWCLimit > 0) */

  if (tmpNode > 0)
    {
    /* check max node */

    if (MinReqNC > tmpNode)
      { 
      if (AdjustJob == TRUE)
        {
        MJobSetAttr(J,mjaReqNodes,(void **)&tmpNode,mdfInt,mSet);
        }
      else
        {
        if (EMsg != NULL)
          {
          sprintf(EMsg,"nodes too high for %s '%s' (%d > %d)",
            ODesc,
            OName,
            MinReqNC,
            tmpNode);
          }

        return(FAILURE);
        }
      } 
    }    /* END if (tmpNode > 0) */

  if (tmpMem > 0)
    {
    /* check max mem */

    if (MinReqMem > tmpMem)
      { 
      if (AdjustJob == TRUE)
        {
        MReqSetAttr(
          J,
          J->Req[0],
          mrqaReqMemPerTask,
          (void **)&tmpMem,
          mdfInt,
          mSet);
        }
      else
        {
        if (EMsg != NULL)
          {
          sprintf(EMsg,"mem too high for %s '%s' (%d > %d)",
            ODesc,
            OName,
            MinReqMem,
            tmpMem);
          }

        return(FAILURE);
        }
      } 
    }

  if (tmpGPU > -1)
    {
    if (MSNLGetIndexCount(&J->Req[0]->DRes.GenericRes,GPUIndex) > tmpGPU)
      {
      if (AdjustJob == TRUE)
        {
        MJobAddRequiredGRes(
          J,
          MCONST_GPUS,
          tmpGPU,
          mxaGRes,
          FALSE,
          FALSE);
        }
      else
        {
        if (EMsg != NULL)
          {
          sprintf(EMsg,"gpu too high for %s '%s' (%d > %d)",
            ODesc,
            OName,
            MSNLGetIndexCount(&J->Req[0]->DRes.GenericRes,GPUIndex),
            tmpGPU);
          }

        return(FAILURE);
        }
      }
    }

  if (tmpCPULimit > 0)
    {
    if (J->CPULimit > tmpCPULimit)
      {
      if (AdjustJob == TRUE)
        {
        MJobSetAttr(J,mjaCPULimit,(void **)&tmpCPULimit,mdfLong,mSet);
        }
      else
        {
        if (EMsg != NULL)
          {
          sprintf(EMsg,"cpulimit too high for %s '%s' (%ld > %ld)",
            ODesc,
            OName,
            J->CPULimit,
            tmpCPULimit);
          }

        return(FAILURE);
        }
      }
    }    /* END if (tmpCPULimit > 0) */

  if ((tmpProc > 0) || 
     ((JT != NULL) && 
      (bmisset(&JT->IFlags,mjifTasksSpecified) ||
       bmisset(&JT->IFlags,mjifDProcsSpecified))))
    {
    /* check max proc */

    if (MinReqTC > tmpProc)
      {
      if (AdjustJob == TRUE)
        {
        MJobSetAttr(J,mjaReqProcs,(void **)&tmpProc,mdfInt,mSet);
        }
      else
        {
        if (EMsg != NULL)
          {
          sprintf(EMsg,"procs too high for %s '%s' (%d > %d)",
            ODesc,
            OName,
            MinReqTC,
            tmpProc);
          }

        return(FAILURE);
        }
      }
    }    /* END if (tmpProc > 0) */
 
  if (tmpPS > 0)
    {
    int PC = MAX(J->TotalProcCount,J->Request.TC);

    /* check max proc */

    if (PC * (long)J->SpecWCLimit[1] > tmpPS)
      {
      if (AdjustJob == TRUE)
        {
        /* NYI */
        }
      else
        {
        if (EMsg != NULL)
          {
          sprintf(EMsg,"proc-seconds too high for %s '%s' (%d > %d)",
            ODesc,
            OName,
            (int)(PC * (long)J->SpecWCLimit[1]),
            tmpPS);
          }

        return(FAILURE);
        }
      }
    }    /* END if (tmpPS > 0) */

  if (tmpTPN > 0)
    {
    /* check max taskspernode */

    if ((J->Req[0] != NULL) && (J->Req[0]->TasksPerNode > tmpTPN))
      {
      if (AdjustJob == TRUE)
        {

        return(FAILURE);
        }
      else
        {
        if (EMsg != NULL)
          {
          sprintf(EMsg,"TPN too high for %s '%s' (%d > %d)",
            ODesc,
            OName,
            J->Req[0]->TasksPerNode,
            tmpTPN);
          }

        return(FAILURE);
        }
      }
    }  /* END if (tmpTPN > 0) */

  /* NOTE:  where do we check/enforce excluded job features??? */
 
  if (!bmisclear(&tmpFlags))
    {
    mbitmap_t NotFlags;

    /* check excluded flags */

    if (bmisset(&tmpFlags,mjfInteractive) && bmisset(&J->Flags,mjfInteractive))
      {
      if (EMsg != NULL)
        {
        sprintf(EMsg,"flag %s not allowed",
          MJobFlags[mjfInteractive]);
        }

      return(FAILURE);
      }

    bmcopy(&NotFlags,&J->Flags);

    bmand(&NotFlags,&JT->Flags);

    /* Check if the job requests any flags disallowed by the job template */

    if (!bmisclear(&NotFlags))
      {
      return(FAILURE);
      }
    }

  /* job template GRes checks */ 

  if ((tmpDRes != NULL) && (J->Req[0] != NULL))
    {
    /* Functionality Change (9/23/08)
       If the job does not ask for a GRES or the template does not specify
       one, use the template.  If the template specifies GRES restrictions
       and the job requests a GRES, the job will not be able to use the 
       template if it requests more than the template specifies and the 
       template specifies a positive limit.  If the job requests a GRES 
       not specified by the template, it will be ignored.*/

    if (MSNLIsEmpty(&J->Req[0]->DRes.GenericRes))
      {
      /* apply the template */
      }
    else if ((tmpDRes->GResSpecified == TRUE) && (MSNLIsEmpty(&tmpDRes->GenericRes)))
      {
      /* do not apply template */

      return(FAILURE);
      }
    else
      {
      int index;

      for (index = 1;index < MSched.M[mxoxGRes];index++)
        {
        if (MGRes.Name[index][0] == '\0')
          break;

        if ((MSNLGetIndexCount(&tmpDRes->GenericRes,index) > 0) && 
            (MSNLGetIndexCount(&J->Req[0]->DRes.GenericRes,index) > MSNLGetIndexCount(&tmpDRes->GenericRes,index)))
          {
          return(FAILURE);
          }
        }  /* END for (index) */
      }    /* END else */
    }      /* END if ((tmpDRes != NULL) && (J->Req[0] != NULL)) */ 

  if (JT != NULL)
    {
    if (JT->SubmitRM != NULL)
      {
      if (JT->SubmitRM == J->SubmitRM)
        {
        if (EMsg != NULL)
          {
          sprintf(EMsg,"rm %s excluded by %s '%s'",
            JT->SubmitRM->Name,
            ODesc,
            OName);
          }

        return(FAILURE);
        }
      }
    }      /* END if (JT != NULL) */

  return(SUCCESS);
  }  /* END MJobCheckJMaxLimits() */





/**
 * Check job against minimum/required job template contraints.
 *
 * @see MJobCheckJMaxLimits()
 * @see MJobGetTemplate()
 *
 * NOTE:  also enforces user/group WCLimits with any OIndex - FIXME
 *
 * Supported Attributes:
 *   WCLimit
 *   cpulimit
 *   Nodes
 *   Tasks/Procs 
 *   features
 *   exec
 *   RMem
 *   FLAGS
 *   PS
 *   TPN
 *   Cmd
 *   AName
 *   OpSys
 *   RM
 *
 * @param J (I)
 * @param OIndex (I)
 * @param OName (I)
 * @param JT (I) job template against which job is evaluated 
 * @param DefaultMinJob (I) [optional]
 * @param AdjustJob (I)
 * @param EMsg (O) [optional]
 * @param EMsgSize (I) [optional]
 */

int MJobCheckJMinLimits(

  mjob_t     *J,
  enum MXMLOTypeEnum OIndex,
  const char *OName,
  mjob_t     *JT,
  mjob_t     *DefaultMinJob,
  mbool_t     AdjustJob,
  char       *EMsg,
  int         EMsgSize)

  {
  int GPUIndex;

  long tmpWCLimit;
  mutime tmpCPULimit;
  int  tmpGPU;
  int  tmpNode;
  int  tmpProc;     /* effective template min procs to test against */
  int  tmpPS;
  int  tmpTPN;
  int  tmpMem;

  mbitmap_t *tmpPAL = NULL;
  mbitmap_t  tmpFlags;

  char *tmpCmd;
  char *tmpAName;

  int  MaxReqNC;
  int  MaxReqMem;

  macl_t *CList;

  const char *ODesc;

  mreq_t *JTR;  /* job template 'min' req */

  const char *FName = "MJobCheckJMinLimits";

  MDB(7,fSCHED) MLog("%s(%s,%s,%s,JT,DefaultMinJob,%s,EMsg,%d)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    MXO[OIndex],
    (OName != NULL) ? OName : "",
    MBool[AdjustJob],
    EMsgSize);

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (J == NULL)
    {
    return(SUCCESS);
    }

  GPUIndex = MUMAGetIndex(meGRes,MCONST_GPUS,mAdd);

  bmclear(&tmpFlags);

  tmpWCLimit = 0;
  tmpCPULimit = 0;
  tmpGPU     = 0;
  tmpNode    = 0;
  tmpProc    = -1;
  tmpPS      = 0;
  tmpTPN     = 0;
  tmpMem     = -1;

  tmpCmd     = NULL;
  tmpAName   = NULL;

  CList      = NULL;

  /* if AdjustJob == TRUE, fix job violations */
  /* if AdjustJob == FALSE, return failure on violation */  

  if (OIndex == mxoClass)
    {
    ODesc = "class";
    }
  else
    {
    /* support other types? */

    ODesc = "job template";
    }

  /* NOTE:  if OIndex == mxoJob, do not load user, group, etc limits */

  /* extract node count extrema from job */

  if (MSched.BluegeneRM == TRUE)
    {
    MaxReqNC = J->Request.TC / MSched.BGNodeCPUCnt;
    }
  else if (J->Request.NC != 0)
    {
    MaxReqNC = J->Request.NC;
    }
  else
    {
    /* NOTE:  should be per req based */

    MJobGetEstNC(J,&MaxReqNC,NULL,TRUE);
    }

  MaxReqMem = 0;

  if (J->Req[1] != NULL)
    {
    int rqindex;

    for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
      {
      if (J->Req[rqindex] == NULL)
        break;

      if (J->Req[rqindex]->DRes.Mem != 0)
        MaxReqMem = MAX(MaxReqMem,J->Req[rqindex]->DRes.Mem);
      }  /* END for (rqindex) */
    }    /* END if (J->Req[1] != NULL) */
  else if (J->Req[0]->DRes.Procs != 0)
    {
    if (J->Req[0]->DRes.Mem != 0)
      MaxReqMem = J->Req[0]->DRes.Mem;
    }

  if (JT != NULL)
    {
    JTR = JT->Req[0];

    /* check min limits */

    tmpWCLimit = JT->SpecWCLimit[0];
    tmpCPULimit = JT->CPULimit;
    tmpNode    = JT->Request.NC;

    if (bmisset(&JT->IFlags,mjifTasksSpecified) ||
        bmisset(&JT->IFlags,mjifDProcsSpecified))
      {
      tmpProc = JT->Request.TC;
      }

    tmpPS      = (int)JT->PSDedicated;

    bmcopy(&tmpFlags,&JT->Flags);

    tmpCmd     = JT->Env.Cmd;
    tmpAName   = JT->AName;
    CList      = JT->RequiredCredList;

    /* MTJobLoad loads partitions into PAL */

    if (!bmisclear(&JT->PAL))
      tmpPAL = &JT->PAL;

    if (JTR != NULL)
      {
      tmpTPN = JTR->TasksPerNode;
      tmpMem = JTR->DRes.Mem;

      if (bmisset(&JT->IFlags,mjifPBSGPUSSpecified))
        tmpGPU = MSNLGetIndexCount(&JTR->DRes.GenericRes,GPUIndex);

      /* SUCCESS if JMin features is subset of J features */
      /* if JMin ReqFMode == OR, SUCCESS if any JMin features found */

      if (MAttrSubset(&J->Req[0]->ReqFBM,&JTR->ReqFBM,JTR->ReqFMode) == FAILURE)
        {
        /* job is missing required feature */

        if (EMsg != NULL)
          {
          sprintf(EMsg,"%s '%s' MIN.FEATURES not satisfied",
            ODesc,
            OName);
          }

        return(FAILURE);
        }

      if ((JTR->Opsys != 0) && 
          (J->Req[0] != NULL) &&
          (JTR->Opsys != J->Req[0]->Opsys))
        return(FAILURE);

      }  /* END if (JTR != NULL) */

    /* Check System Job Type */

    if ((JT->System != NULL) && (JT->System->JobType != msjtNONE))
      {
      if ((J->System == NULL) || /* Must be a systemjob to match */
          (J->System->JobType != JT->System->JobType))
        return(FAILURE);
      }
    }    /* END if (JT != NULL) */      
  
  /* support default job min limits */

  if (DefaultMinJob != NULL)
    {
    if (tmpWCLimit == 0)
      tmpWCLimit = DefaultMinJob->SpecWCLimit[0];

    if (tmpNode == 0)
      tmpNode = DefaultMinJob->Request.NC;

    if (tmpProc == -1)
      tmpProc = DefaultMinJob->Request.TC;

    if (tmpPS == 0)
      tmpPS = (int)DefaultMinJob->PSDedicated;

    if (DefaultMinJob->Req[0] != NULL)
      {
      if (tmpTPN == 0)
        tmpTPN = DefaultMinJob->Req[0]->TasksPerNode;
      }

    if (bmisclear(&tmpFlags))
      bmcopy(&tmpFlags,&DefaultMinJob->Flags);

    if (tmpCmd == NULL)
      tmpCmd = DefaultMinJob->Env.Cmd;

    if (tmpAName == NULL)
      tmpAName = DefaultMinJob->AName;

    if (DefaultMinJob->Req[0] != NULL)
      tmpMem = DefaultMinJob->Req[0]->DRes.Mem;

    if ((tmpPAL == NULL) &&
        (!bmisclear(&DefaultMinJob->PAL)))
      tmpPAL = &DefaultMinJob->PAL;
    }  /* END if (DefaultMinJob != NULL) */

  /* Requested partitions - check if user requested template partition. */

  if (tmpPAL != NULL)
    {
    int pindex; /* template partition index */
    mbool_t partitionFound = FALSE;

    /* SpecPAL is intialized with ALL partitions. If ALL, then
     * assume that a specific partition (-l partition) wasn't requested. */

    if (bmissetall(&J->SpecPAL,MMAX_PAR))
      return(FAILURE);

    for (pindex = 0;pindex < MMAX_PAR;pindex++)
      {
      if (MPar[pindex].Name[0] == '\0')
        break;

      if ((bmisset(tmpPAL,pindex)) &&   /* template requires? */
          (bmisset(&J->SpecPAL,pindex))) /* job requested? */
        {
        partitionFound = TRUE;

        break;
        }
      }

    if (partitionFound == FALSE)
      return(FAILURE);
    }

  if (tmpCmd != NULL)
    {
    if ((J->Env.Cmd == NULL) || (strncmp(tmpCmd,J->Env.Cmd,strlen(tmpCmd))))
      {
      if (EMsg != NULL)
        {
        sprintf(EMsg,"required cmd does not match (%s != %s) for %s '%s'",
          (J->Env.Cmd != NULL) ? J->Env.Cmd : "",
          tmpCmd,
          ODesc,
          OName);
        }

      return(FAILURE);
      }
    }  /* END if (tmpCmd != NULL) */

  if (tmpAName != NULL)
    {
    if ((J->AName == NULL) || (strncmp(tmpAName,J->AName,strlen(tmpAName))))
      {
      if (EMsg != NULL)
        {
        sprintf(EMsg,"required aname does not match (%s != %s) for %s '%s'",
          (J->AName != NULL) ? J->AName : "",
          tmpAName,
          ODesc,
          OName);
        }

      return(FAILURE);
      }
    }  /* END if (tmpAName != NULL) */

  if (tmpWCLimit > 0)
    {
    /* check min WC limit */

    if (((long)J->SpecWCLimit[1] < tmpWCLimit) && 
        ((long)J->SpecWCLimit[1] != -1))
      {
      if (AdjustJob == TRUE)
        {
        J->SpecWCLimit[1] = tmpWCLimit;
        }
      else
        {
        if (EMsg != NULL)
          {
          sprintf(EMsg,"wclimit too low (%ld < %ld) for %s '%s'",
            J->SpecWCLimit[1],
            tmpWCLimit,
            ODesc,
            OName);
          }

        return(FAILURE);
        }
      }
    }    /* END if (tmpWCLimit > 0) */

  if (tmpGPU > 0)
    {
    if (MSNLGetIndexCount(&J->Req[0]->DRes.GenericRes,GPUIndex) < tmpGPU)
      {
      if (AdjustJob == TRUE)
        {
        MJobAddRequiredGRes(
          J,
          MCONST_GPUS,
          tmpGPU,
          mxaGRes,
          FALSE,
          FALSE);
        }
      else
        {
        if (EMsg != NULL)
          {
          sprintf(EMsg,"gpu too low for %s '%s' (%d > %d)",
            ODesc,
            OName,
            MSNLGetIndexCount(&J->Req[0]->DRes.GenericRes,GPUIndex),
            tmpGPU);
          }

        return(FAILURE);
        }
      }
    }

  if (tmpCPULimit > 0)
    {
    /* check min CPU limit */

    if (J->CPULimit < tmpCPULimit)
      {
      if (AdjustJob == TRUE)
        {
        J->CPULimit = tmpCPULimit;
        }
      else
        {
        if (EMsg != NULL)
          {
          sprintf(EMsg,"cpulimit too low (%ld < %ld) for %s '%s'",
            J->CPULimit,
            tmpCPULimit,
            ODesc,
            OName);
          }

        return(FAILURE);
        }
      }
    }    /* END if (tmpCPULimit > 0) */

  if ((tmpNode > 0) && (MaxReqNC > 0))
    {
    /* check min node */

    if (MaxReqNC < tmpNode)
      { 
      if (AdjustJob == TRUE)
        {
        MJobSetAttr(J,mjaReqNodes,(void **)&tmpNode,mdfInt,mSet);
        }
      else
        {
        if (EMsg != NULL)
          {
          sprintf(EMsg,"nodes too low (%d < %d) for %s '%s'",
            MaxReqNC,
            tmpNode,
            ODesc,
            OName);
          }

        return(FAILURE);
        }
      } 
    }    /* END if (tmpNode > 0) */

  if (tmpProc >= 0)
    {
    int PC = MAX(J->TotalProcCount,J->Request.TC);

    /* check min proc */

    if (PC < tmpProc)
      {
      if (AdjustJob == TRUE)
        {
        MJobSetAttr(J,mjaReqProcs,(void **)&tmpProc,mdfInt,mSet);
        }
      else
        {
        if (EMsg != NULL)
          {
          sprintf(EMsg,"procs too low for %s '%s' (%d < %d)",
            ODesc,
            OName,
            J->Request.TC,
            tmpProc);
          }

        return(FAILURE);
        }
      }
    }    /* END if (tmpProc > 0) */

  if (tmpMem > 0)
    {
    /* check min mem */

    if (MaxReqMem < tmpMem)
      {
      if (AdjustJob == TRUE)
        {
        MReqSetAttr(
          J,
          J->Req[0],
          mrqaReqMemPerTask,
          (void **)&tmpMem,
          mdfInt,
          mSet);
        }
      else
        {
        if (EMsg != NULL)
          {
          sprintf(EMsg,"mem too low for %s '%s' (%d < %d)",
            ODesc,
            OName,
            MaxReqMem,
            tmpMem);
          }

        return(FAILURE);
        }
      }
    }    /* END if (tmpMem > 0) */

  if (tmpPS > 0)
    {
    int PC = MAX(J->TotalProcCount,J->Request.TC);

    /* check min proc */

    if (PC * (long)J->SpecWCLimit[1] < tmpPS)
      {
      if (AdjustJob == TRUE)
        {
        /* NO-OP */
        }
      else
        {
        if (EMsg != NULL)
          {
          sprintf(EMsg,"proc-seconds too low for %s '%s' (%d < %d)",
            ODesc,
            OName,
            (int)(PC * (long)J->SpecWCLimit[1] < tmpPS),
            tmpPS);
          }

        return(FAILURE);
        }
      }
    }    /* END if (tmpPS > 0) */

  if (tmpTPN > 0)
    {
    if (J->Req[0]->TasksPerNode < tmpTPN)
      {
      if (AdjustJob == TRUE)
        {
        /* NYI */

        return(FAILURE);
        }
      else
        {
        if (EMsg != NULL)
          {
          sprintf(EMsg,"TPN too low for %s '%s' (%d < %d)",
            ODesc,
            OName,
            J->Req[0]->TasksPerNode,
            tmpTPN);
          }

        return(FAILURE);
        }
      }
    }  /* END if (tmpTPN > 0) */

  if (!bmisclear(&tmpFlags))
    {
    int tmpI;

    /* required flags */

    for (tmpI = mjfNONE;tmpI != mjfLAST;tmpI++)
      {
      if (bmisset(&tmpFlags,tmpI) && !bmisset(&J->Flags,tmpI))
        {
        if (EMsg != NULL)
          {
          sprintf(EMsg,"flag %s required",
            MJobFlags[tmpI]);
          }

        return(FAILURE);
        }
      }
    }    /* END if (bmisclear(&tmpFlags)) */

  if (CList != NULL)
    {
    macl_t *tACL;

    mbool_t UserACLFound    = FALSE;
    mbool_t GroupACLFound   = FALSE;
    mbool_t AccountACLFound = FALSE;
    mbool_t ClassACLFound   = FALSE;
    mbool_t QOSACLFound     = FALSE;

    /* determine which creds are required */

    for (tACL = CList;tACL != NULL;tACL = tACL->Next)
      {
      if ((tACL->Type == maUser) && (tACL->Name[0] != '\1'))
        UserACLFound = TRUE;
      else if (tACL->Type == maGroup)
        GroupACLFound = TRUE;
      else if (tACL->Type == maAcct)
        AccountACLFound = TRUE;
      else if (tACL->Type == maQOS)
        QOSACLFound = TRUE;
      else if (tACL->Type == maClass)
        ClassACLFound = TRUE;
      }  /* END for (cindex) */

    if ((UserACLFound == TRUE) && (J->Credential.U != NULL))
      {
      /* check required user list */

      for (tACL = CList;tACL != NULL;tACL = tACL->Next)
        {
        if (tACL->Type != maUser)
          continue;

        if (!strcmp(tACL->Name,J->Credential.U->Name))
          break;

        if (!strcmp(tACL->Name,"ALL"))
          break;
        }  /* END for (cindex) */

      if (tACL == NULL)
        {
        if (EMsg != NULL)
          {
          sprintf(EMsg,"user required by %s '%s'",              
            ODesc,
            OName);
          }

        return(FAILURE);
        }  /* END for (cindex) */
      }    /* END if (J->Cred.U != NULL) */
    else if (UserACLFound == TRUE)
      {
      /* user required but no user associated with job */

      if (EMsg != NULL)
        {
        sprintf(EMsg,"user %s not in %s '%s' user list",
          J->Credential.U->Name,
          ODesc,
          OName);
        }

      return(FAILURE);
      }

    if ((GroupACLFound == TRUE) && (J->Credential.G != NULL))
      {
      /* check required group list */

      for (tACL = CList;tACL != NULL;tACL = tACL->Next)
        {
        if (tACL->Type != maGroup)
          continue;

        if (!strcmp(tACL->Name,J->Credential.G->Name))
          break;

        if (!strcmp(tACL->Name,"ALL"))
          break;
        }  /* END for (cindex) */

      if (tACL == NULL)
        {
        if (EMsg != NULL)
          {
          sprintf(EMsg,"group %s not in %s '%s' group list",
            J->Credential.G->Name,
            ODesc,
            OName);
          }

        return(FAILURE);
        }  /* END for (cindex) */
      }    /* END if (J->Cred.G != NULL) */
    else if (GroupACLFound == TRUE)
      {
      /* group required but no group associated with job */

      if (EMsg != NULL)
        {
        sprintf(EMsg,"group required by %s '%s'",
          ODesc,
          OName);
        }

      return(FAILURE);
      }

    /* NOTE: requires that J->Cred.A be set earlier */

    if ((AccountACLFound == TRUE) && (J->Credential.A != NULL))
      {
      /* check required account list */

      for (tACL = CList;tACL != NULL;tACL = tACL->Next)
        {
        if (tACL->Type != maAcct)
          continue;

        if (!strcmp(tACL->Name,J->Credential.A->Name))
          break;

        if (!strcmp(tACL->Name,"ALL"))
          break;
        }  /* END for (cindex) */

      if (tACL == NULL)
        {
        if (EMsg != NULL)
          {
          sprintf(EMsg,"account %s not in %s '%s' account list",
            J->Credential.A->Name,
            ODesc,
            OName);
          }

        return(FAILURE);
        }  /* END for (cindex) */
      }    /* END if (J->Cred.A != NULL) */
    else if (AccountACLFound == TRUE)
      {
      /* account required but no account associated with job */

      if (EMsg != NULL)
        {
        sprintf(EMsg,"account required by %s '%s'",
          ODesc,
          OName);
        }

      return(FAILURE);
      }

    /* NOTE: requires that J->Cred.C be set earlier */

    if ((ClassACLFound == TRUE) && (J->Credential.C != NULL))
      {
      /* check required class/queue list */

      for (tACL = CList;tACL != NULL;tACL = tACL->Next)
        {
        if (tACL->Type != maClass)
          continue;

        if (!strcmp(tACL->Name,J->Credential.C->Name))
          break;

        if (!strcmp(tACL->Name,"ALL"))
          break;
        }  /* END for (cindex) */

      if (tACL == NULL)
        {
        if (EMsg != NULL)
          {
          sprintf(EMsg,"class %s not in %s '%s' class list",
            J->Credential.C->Name,
            ODesc,
            OName);
          }

        return(FAILURE);
        }  /* END for (cindex) */
      }    /* END if (J->Cred.C != NULL) */
    else if (ClassACLFound == TRUE)
      {
      /* class required but no class associated with job */

      if (EMsg != NULL)
        {
        sprintf(EMsg,"class required by %s '%s'",
          ODesc,
          OName);
        }

      return(FAILURE);
      }

    /* NOTE: requires that J->Cred.Q be set earlier */

    if ((QOSACLFound == TRUE) && (J->Credential.Q != NULL))
      {
      /* check required user list */

      for (tACL = CList;tACL != NULL;tACL = tACL->Next)
        {
        if (tACL->Type != maQOS)
          continue;

        if (!strcmp(tACL->Name,J->Credential.Q->Name))
          break;
        }  /* END for (cindex) */

      if (tACL == NULL)
        {
        if (EMsg != NULL)
          {
          sprintf(EMsg,"QOS %s not in %s '%s' QOS list",              
            J->Credential.Q->Name,
            ODesc,
            OName);
          }

        return(FAILURE);
        }  /* END for (cindex) */
      }    /* END if (J->Cred.Q != NULL) */
    else if (QOSACLFound == TRUE)
      {
      /* NOTE: if J->Cred.Q is NULL and Class has a REQQOSLIST then MJobGetQOSAccess found that
               this user and this class don't work */

      if (QOSACLFound == TRUE)
        {
        if (EMsg != NULL)
          {        
          sprintf(EMsg,"QOS required by %s '%s'",
            ODesc,
            OName);
          }

        return(FAILURE);
        }  /* END for (cindex) */
      }    /* END else if (QOSACLFound == TRUE) */
    }      /* END if (CList != NULL) */

  if (JT != NULL) 
    {
    if (JT->SubmitRM != NULL)
      {
      if (JT->SubmitRM != J->SubmitRM)
        {
        if (EMsg != NULL)
          {
          sprintf(EMsg,"rm %s required by %s '%s'",
            JT->SubmitRM->Name,
            ODesc,
            OName);
          }

        return(FAILURE);
        }
      }
    }    /* END if (JT != NULL) */

  if ((JT != NULL) && (JT->Variables.Table != NULL))
    {
    char *VarName;
    char *VarVal;

    char *ReqName;
    char *ReqVal;

    mhashiter_t HTI;
    mhashiter_t ReqHTI;

    mbool_t Found;

    /* match variables */

    if (J->Variables.Table == NULL)
      {
      if (EMsg != NULL)
        {
        sprintf(EMsg,"variables required by %s, job has not variables",
          JT->SubmitRM->Name);
        }

      return(FAILURE);
      }

    MUHTIterInit(&ReqHTI);

    while (MUHTIterate(&JT->Variables,&ReqName,(void **)&ReqVal,NULL,&ReqHTI) == SUCCESS)
      {
      Found = FALSE;

      MUHTIterInit(&HTI);

      while (MUHTIterate(&J->Variables,&VarName,(void **)&VarVal,NULL,&HTI) == SUCCESS)
        {
        if (!strcasecmp(VarName,ReqName) &&
            ((ReqVal == NULL) ||
              ((VarVal != NULL) && (!strcasecmp(VarVal,ReqVal)))))
          {
          Found = TRUE;

          break;
          }
        }

      if (Found == FALSE)
        {
        /* required variable not found */

        return(FAILURE);
        }
      }  /* END while (MUHTIterate(ReqHTI)) */
    }

  return(SUCCESS);
  }  /* END MJobCheckJMinLimits() */


