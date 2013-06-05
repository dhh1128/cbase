/* HEADER */

      
/**
 * @file MJobCheckLimits.c
 *
 * Contains:
 *    MJobCheckLimits()
 */


#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 * Verify job currently satisfies all limits associated w/partition P and
 * all job credentials.
 *
 * @see MPolicyCheckLimit() - child
 *
 * @param J (I)
 * @param PLevel (I)
 * @param P (I)
 * @param MLimitBM (I) [bitmap of enum ml*]
 * @param Remaining (O) (optional) number of jobs just like this one that can be run - only valid this instant
 * @param Msg (O) [optional,minsize=MMAX_LINE]
 */

int MJobCheckLimits(

  const mjob_t  *J,
  enum MPolicyTypeEnum PLevel,
  const mpar_t  *P,
  mbitmap_t     *MLimitBM,
  long          *Remaining,
  char          *Msg)

  {
  /* MLimitBM:  active, system, queue */

  const char *FName = "MJobCheckLimits";

  MDB(7,fSTRUCT) MLog("%s(%s,%s,%s,Msg)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (P != NULL) ? P->Name : "NULL",
    MPolicyMode[PLevel]);

  if (Msg != NULL)
    Msg[0] = '\0';

  if ((J == NULL) || (J->Req[0] == NULL) || (P == NULL))
    {
    return(FAILURE);
    }

  if ((P->RM != NULL) && 
      (P->RM->MaxJobPerMinute > 0) &&
      (P->RM->IntervalStart + MCONST_MINUTELEN > MSched.Time) &&
      ((int)P->RM->IntervalJobCount >= P->RM->MaxJobPerMinute))
    {
    /* job violates RM max job per minute policy */

    if (Msg != NULL)
      {
      sprintf(Msg,"job %s violates maxjobperminute policy for RM %s (limit: %d)",
        J->Name,
        P->RM->Name,
        P->RM->MaxJobPerMinute);
      }

    return(FAILURE);
    }

  if ((PLevel == mptOff) || (bmisset(&J->Flags,mjfIgnPolicies)))
    {
    /* ignore policies */

    return(SUCCESS);
    }

  if (Remaining != NULL)
    {
    *Remaining = MMAX_JOBARRAYSIZE;
    }

  if ((bmisset(&J->Flags,mjfArrayJob) && (J->JGroup != NULL)) && bmisset(MLimitBM,mlActive))
    {
    mjob_t *JA = NULL;

    if (MJobFind(J->JGroup->Name,&JA,mjsmBasic) == FAILURE)
      {
      /* no-op */
      }
    else if ((JA->Array != NULL) && 
             (JA->Array->Limit > 0) &&
             (JA->Array->Active >= JA->Array->Limit))
      {
      if (Msg != NULL)
        {
        snprintf(Msg,MMAX_LINE,"job '%s' violates array '%s' slot limit of %d",
          J->Name,
          JA->Array->Name,
          JA->Array->Limit);
        }
       
      return(FAILURE);
      }
    }    /* END if (bmisset(&J->Flags,mjfArrayJob) && (J->JGroup != NULL)) */

  /* determine resource consumption */

  if (MJobCheckCredentialLimits(J,PLevel,P,MLimitBM,Remaining,Msg) == FAILURE)
    {
    return(FAILURE);
    }

  if (MJobCheckFairshareTreeLimits(J,PLevel,P,MLimitBM,Remaining,Msg) == FAILURE)
    {
    return(FAILURE);
    }

  if ((J->ParentVCs != NULL) && 
      (MJobCheckVCLimits(J,PLevel,P,MLimitBM,Remaining,Msg) == FAILURE))
    {
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MJobCheckLimits() */





/**
 * Check credential (U,G,Q,C,A) limits.
 *
 * @see MJobCheckLimits - parent
 *
 * @param J (I)
 * @param PLevel (I)
 * @param P (I)
 * @param MLimitBM (I) [bitmap of enum ml*]
 * @param Remaining (O) (optional) number of jobs just like this one that can be run - only valid this instant
 * @param Msg (O) [optional,minsize=MMAX_LINE]
 */

int MJobCheckCredentialLimits(

  const mjob_t  *J,
  enum MPolicyTypeEnum PLevel,
  const mpar_t  *P,
  mbitmap_t     *MLimitBM,
  long          *Remaining,
  char          *Msg)

  {
  const char *MType[] = {
    "active",
    "idle",
    "system",
    NULL };

  enum MXMLOTypeEnum OList1[] = { 
    mxoNONE, 
    mxoUser, 
    mxoGroup, 
    mxoAcct,
    mxoClass, 
    mxoQOS,
    mxoALL };       

  enum MXMLOTypeEnum OList2[] = { 
    mxoUser, 
    mxoGroup, 
    mxoAcct, 
    mxoClass, 
    mxoQOS, 
    mxoPar, 
    mxoALL };

  enum MActivePolicyTypeEnum PList[]  = { 
    mptMaxJob, 
    mptMaxProc, 
    mptMaxNode, 
    mptMaxPS, 
    mptMaxPE, 
    mptMaxWC, 
    mptMinProc,
    mptMaxMem,
    mptMaxGRes,
    mptMaxArrayJob,
    mptNONE };

  enum MLimitTypeEnum MList[] = { 
    mlActive, 
    mlIdle, 
    mlSystem, 
    mlNONE };

  int oindex1;
  int oindex2;

  int ptindex; /* policy type index */
  int mindex;

  long tmpL;

  const mpu_t *OP;  /* object policies */
  const mpu_t *DP;  /* default policies */
  const mpu_t *QP;  /* QoS specified override policies */

  mbitmap_t EffOverrideCredBM;  /* bitmap of cred types to which active 
                                         policy overrides should be applied */
  const char *OID;

  mbool_t UsedOverride;

  int parindex = (MSched.PerPartitionScheduling == TRUE) ? P->Index : 0;

  mpc_t JUsage;

  memset(&JUsage,0,sizeof(JUsage));
 
  MSNLInit(&JUsage.GRes);

  MPCFromJob(J,&JUsage);

  oindex1 = 0; 
  oindex2 = 0;
 
  bmsetall(&EffOverrideCredBM,mxoLAST);

  for (mindex = 0;MList[mindex] != mlNONE;mindex++)
    {
    if (!bmisset(MLimitBM,MList[mindex]))
      continue;

    /* limits: user, group, account, class, override w/QOS */

    /* locate override limits */

    if (J->Credential.Q != NULL)
      {
      if (MList[mindex] == mlActive)
        {
        if (!bmisclear(&J->Credential.Q->L.OverrideActiveCredBM))
          bmcopy(&EffOverrideCredBM,&J->Credential.Q->L.OverrideActiveCredBM);
       
        QP = J->Credential.Q->L.OverrideActivePolicy;
        }
      else if (MList[mindex] == mlIdle)
        {
        QP = J->Credential.Q->L.OverrideIdlePolicy;
        }
      else if (MList[mindex] == mlSystem)
        {
        QP = J->Credential.Q->L.OverriceJobPolicy;
        }
      else
        {
        QP = NULL;
        }
      }
    else
      {
      QP = NULL;
      }

    for (oindex1 = 0;OList1[oindex1] != mxoALL;oindex1++)
      {
      for (oindex2 = 0;OList2[oindex2] != mxoALL;oindex2++)
        {
        if (((OList2[oindex2] == mxoPar) && (MList[mindex] != mlSystem)) ||
            ((OList2[oindex2] != mxoPar) && (MList[mindex] == mlSystem)))
          {
          continue;
          }    

        OP = NULL;
        DP = NULL;

        OID = NULL;

        switch (OList2[oindex2])
          {
          case mxoUser:

            if (J->Credential.U != NULL)
              {
              switch (OList1[oindex1])
                {
                case mxoNONE:

                  if (MList[mindex] == mlActive)
                    OP = &J->Credential.U->L.ActivePolicy;
                  else if (MList[mindex] == mlIdle)
                    OP = J->Credential.U->L.IdlePolicy;

                  break;

                case mxoAcct:

                  if (J->Credential.A != NULL)
                    {
                    if (MList[mindex] == mlActive)
                      MUHTGet(J->Credential.U->L.AcctActivePolicy,J->Credential.A->Name,(void **)&OP,NULL);
                    else if (MList[mindex] == mlIdle)
                      MUHTGet(J->Credential.U->L.AcctIdlePolicy,J->Credential.A->Name,(void **)&OP,NULL);
                    }

                  break;

                case mxoQOS:

                  if (J->Credential.Q != NULL)
                    {
                    if (MList[mindex] == mlActive)
                      MUHTGet(J->Credential.U->L.QOSActivePolicy,J->Credential.Q->Name,(void **)&OP,NULL);
                    else if (MList[mindex] == mlIdle)
                      MUHTGet(J->Credential.U->L.QOSIdlePolicy,J->Credential.Q->Name,(void **)&OP,NULL);
                    }

                  break;

                case mxoGroup:

                  if (J->Credential.G != NULL)
                    {
                    if (MList[mindex] == mlActive)
                      MUHTGet(J->Credential.U->L.GroupActivePolicy,J->Credential.G->Name,(void **)&OP,NULL);
                    else if (MList[mindex] == mlIdle)
                      MUHTGet(J->Credential.U->L.GroupIdlePolicy,J->Credential.G->Name,(void **)&OP,NULL);
                    }

                  break;

                case mxoClass:

                  if (J->Credential.C != NULL)
                    {
                    if (MList[mindex] == mlActive)
                      MUHTGet(J->Credential.U->L.ClassActivePolicy,J->Credential.C->Name,(void **)&OP,NULL);
                    else if (MList[mindex] == mlIdle)
                      MUHTGet(J->Credential.U->L.ClassIdlePolicy,J->Credential.C->Name,(void **)&OP,NULL);
                    }

                  break;

                default:

                  /* NO-OP */

                  break;
                }  /* END switch (OList1[oindex1]) */

              OID = J->Credential.U->Name;
              }  /* END if (J->Cred.U != NULL) */

            if (MSched.DefaultU != NULL)
              {
              if (MList[mindex] == mlActive)
                DP = &MSched.DefaultU->L.ActivePolicy;
              else if (MList[mindex] == mlIdle)
                DP = MSched.DefaultU->L.IdlePolicy;
              }

            break;

          case mxoGroup:

            if (J->Credential.G != NULL)
              {
              switch (OList1[oindex1])
                {
                case mxoNONE:

                  if (MList[mindex] == mlActive)
                    OP = &J->Credential.G->L.ActivePolicy;
                  else if (MList[mindex] == mlIdle)
                    OP = J->Credential.G->L.IdlePolicy;

                  break;

                case mxoAcct:

                  if (J->Credential.A != NULL)
                    {
                    if (MList[mindex] == mlActive)
                      MUHTGet(J->Credential.G->L.AcctActivePolicy,J->Credential.A->Name,(void **)&OP,NULL);
                    else if (MList[mindex] == mlIdle)
                      MUHTGet(J->Credential.G->L.AcctIdlePolicy,J->Credential.A->Name,(void **)&OP,NULL);
                    }

                  break;

                case mxoClass:

                  if (J->Credential.C != NULL)
                    {
                    if (MList[mindex] == mlActive)
                      MUHTGet(J->Credential.G->L.ClassActivePolicy,J->Credential.C->Name,(void **)&OP,NULL);
                    else if (MList[mindex] == mlIdle)
                      MUHTGet(J->Credential.G->L.ClassIdlePolicy,J->Credential.C->Name,(void **)&OP,NULL);
                    }

                  break;

                case mxoQOS:

                  if (J->Credential.Q != NULL)
                    {
                    if (MList[mindex] == mlActive)
                      MUHTGet(J->Credential.G->L.QOSActivePolicy,J->Credential.Q->Name,(void **)OP,NULL);
                    else if (MList[mindex] == mlIdle)
                      MUHTGet(J->Credential.G->L.QOSIdlePolicy,J->Credential.Q->Name,(void **)OP,NULL);
                    }

                  break;

                case mxoUser:

                  if (J->Credential.U != NULL)
                    {
                    if (MList[mindex] == mlActive)
                      MUHTGet(J->Credential.G->L.UserActivePolicy,J->Credential.U->Name,(void **)&OP,NULL);
                    else if (MList[mindex] == mlIdle)
                      MUHTGet(J->Credential.G->L.UserIdlePolicy,J->Credential.U->Name,(void **)&OP,NULL);
                    }

                  break;

                default:

                  /* NO-OP */

                  break;
                }  /* END switch (OList1[oindex1]) */

              OID = J->Credential.G->Name;
              }  /* END if (J->Cred.G != NULL) */

            if (MSched.DefaultG != NULL)
              {
              if (MList[mindex] == mlActive)
                DP = &MSched.DefaultG->L.ActivePolicy;
              else if (MList[mindex] == mlIdle)
                DP = MSched.DefaultG->L.IdlePolicy;
              }
 
            break;
 
          case mxoAcct:

            if (J->Credential.A != NULL)
              {
              switch (OList1[oindex1])
                {
                case mxoNONE:

                  if (MList[mindex] == mlActive)
                    OP = &J->Credential.A->L.ActivePolicy;
                  else if (MList[mindex] == mlIdle)
                    OP = J->Credential.A->L.IdlePolicy;

                  break;

                case mxoUser:

                  if ((J->Credential.U != NULL))
                    {
                    if (MList[mindex] == mlActive)
                      MUHTGet(J->Credential.A->L.UserActivePolicy,J->Credential.U->Name,(void **)&OP,NULL);
                    else if (MList[mindex] == mlIdle)
                      MUHTGet(J->Credential.A->L.UserIdlePolicy,J->Credential.U->Name,(void **)&OP,NULL);
                    }

                  break;
             
                case mxoGroup:

                  if ((J->Credential.G != NULL))
                    {
                    if (MList[mindex] == mlActive)
                      MUHTGet(J->Credential.A->L.GroupActivePolicy,J->Credential.G->Name,(void **)&OP,NULL);
                    else if (MList[mindex] == mlIdle)
                      MUHTGet(J->Credential.A->L.GroupIdlePolicy,J->Credential.G->Name,(void **)&OP,NULL);
                    }

                  break;
 
                case mxoQOS:

                  if (J->Credential.Q != NULL)
                    {
                    if (MList[mindex] == mlActive)
                      MUHTGet(J->Credential.A->L.QOSActivePolicy,J->Credential.Q->Name,(void **)&OP,NULL);
                    else if (MList[mindex] == mlIdle)
                      MUHTGet(J->Credential.A->L.QOSIdlePolicy,J->Credential.Q->Name,(void **)&OP,NULL);
                    }

                  break;
 
                case mxoClass:

                  if (J->Credential.C != NULL)
                    {
                    if (MList[mindex] == mlActive)
                      MUHTGet(J->Credential.A->L.ClassActivePolicy,J->Credential.C->Name,(void **)&OP,NULL);
                    else if (MList[mindex] == mlIdle)
                      MUHTGet(J->Credential.A->L.ClassIdlePolicy,J->Credential.C->Name,(void **)&OP,NULL);
                    }

                  break;
 
                default:

                  /* NO-OP */

                  break;
                }  /* END switch (OList1[oindex1]) */
 
              OID = J->Credential.A->Name;
              }  /* END if (J->Cred.A != NULL) */
 
            if (MSched.DefaultA != NULL)
              {
              if (MList[mindex] == mlActive)
                DP = &MSched.DefaultA->L.ActivePolicy;
              else if (MList[mindex] == mlIdle)
                DP = MSched.DefaultA->L.IdlePolicy;
              }

            break;

          case mxoQOS:
 
            if (J->Credential.Q != NULL)
              {
              switch (OList1[oindex1])
                {
                case mxoNONE:

                  if (MList[mindex] == mlActive)
                    OP = &J->Credential.Q->L.ActivePolicy;
                  else if (MList[mindex] == mlIdle)
                    OP = J->Credential.Q->L.IdlePolicy;
                  else if (MList[mindex] == mlSystem)
                    OP = J->Credential.Q->L.JobPolicy;

                  break;

                case mxoUser:

                  if (J->Credential.U != NULL)
                    {
                    if (MList[mindex] == mlActive)
                      MUHTGet(J->Credential.Q->L.UserActivePolicy,J->Credential.U->Name,(void **)&OP,NULL);
                    else if (MList[mindex] == mlIdle)
                      MUHTGet(J->Credential.Q->L.UserIdlePolicy,J->Credential.U->Name,(void **)&OP,NULL);
                    }

                  break;

                case mxoGroup:

                  if (J->Credential.G != NULL)
                    {
                    if (MList[mindex] == mlActive)
                      MUHTGet(J->Credential.Q->L.GroupActivePolicy,J->Credential.G->Name,(void **)&OP,NULL);
                    else if (MList[mindex] == mlIdle)
                      MUHTGet(J->Credential.Q->L.GroupIdlePolicy,J->Credential.G->Name,(void **)&OP,NULL);
                    }
 
                  break;

                case mxoClass:

                  if (J->Credential.C != NULL)
                    {
                    if (MList[mindex] == mlActive)
                      MUHTGet(J->Credential.Q->L.ClassActivePolicy,J->Credential.C->Name,(void **)&OP,NULL);
                    else if (MList[mindex] == mlIdle)
                      MUHTGet(J->Credential.Q->L.ClassIdlePolicy,J->Credential.C->Name,(void **)&OP,NULL);
                    }

                  break;

                case mxoAcct:

                  if (J->Credential.A != NULL)
                    {
                    if (MList[mindex] == mlActive)
                      MUHTGet(J->Credential.Q->L.AcctActivePolicy,J->Credential.A->Name,(void **)&OP,NULL);
                    else if (MList[mindex] == mlIdle)
                      MUHTGet(J->Credential.Q->L.AcctIdlePolicy,J->Credential.A->Name,(void **)&OP,NULL);
                    }

                  break;


                default:

                  /* NO-OP */

                  break;
                }  /* END switch (OList1[oindex1]) */

              OID = J->Credential.Q->Name;
              }
 
            if (MSched.DefaultQ != NULL)
              {
              if (MList[mindex] == mlActive)
                DP = &MSched.DefaultQ->L.ActivePolicy;
              else if (MList[mindex] == mlIdle)
                DP = MSched.DefaultQ->L.IdlePolicy;
              }
 
            break;

          case mxoClass:

            if (J->Credential.C != NULL)
              {
              switch (OList1[oindex1])
                {
                case mxoNONE:

                  if (MList[mindex] == mlActive)
                    OP = &J->Credential.C->L.ActivePolicy;
                  else if (MList[mindex] == mlIdle)
                    OP = J->Credential.C->L.IdlePolicy;

                  break;

                case mxoUser:

                  if (J->Credential.U != NULL)
                    {
                    if (MList[mindex] == mlActive)
                      MUHTGet(J->Credential.C->L.UserActivePolicy,J->Credential.U->Name,(void **)&OP,NULL);
                    else if (MList[mindex] == mlIdle)
                      MUHTGet(J->Credential.C->L.UserIdlePolicy,J->Credential.U->Name,(void **)&OP,NULL);
                    }

                  break;

                case mxoGroup:

                  if (J->Credential.G != NULL)
                    {
                    if (MList[mindex] == mlActive)
                      MUHTGet(J->Credential.C->L.GroupActivePolicy,J->Credential.G->Name,(void **)&OP,NULL);
                    else if (MList[mindex] == mlIdle)
                      MUHTGet(J->Credential.C->L.GroupIdlePolicy,J->Credential.G->Name,(void **)&OP,NULL);
                    }

                  break;

                case mxoAcct:

                  if (J->Credential.A != NULL)
                    {
                    if (MList[mindex] == mlActive)
                      MUHTGet(J->Credential.C->L.AcctActivePolicy,J->Credential.A->Name,(void **)&OP,NULL);
                    else if (MList[mindex] == mlIdle)
                      MUHTGet(J->Credential.C->L.AcctIdlePolicy,J->Credential.A->Name,(void **)&OP,NULL);
                    }

                  break;

                case mxoQOS:

                  if (J->Credential.Q != NULL)
                    {
                    if (MList[mindex] == mlActive)
                      MUHTGet(J->Credential.C->L.QOSActivePolicy,J->Credential.Q->Name,(void **)&OP,NULL);
                    else if (MList[mindex] == mlIdle)
                      MUHTGet(J->Credential.C->L.QOSIdlePolicy,J->Credential.Q->Name,(void **)&OP,NULL);
                    }

                  break;

                default:

                  /* NO-OP */

                  break;
                }  /* END switch (OList1[oindex1]) */
 
              OID = J->Credential.C->Name;
              }  /* END if (J->Cred.C != NULL) */
 
            if (MSched.DefaultC != NULL)
              {
              if (MList[mindex] == mlActive)
                DP = &MSched.DefaultC->L.ActivePolicy;
              else if (MList[mindex] == mlIdle)
                DP = MSched.DefaultC->L.IdlePolicy;
              }

            break;

          case mxoPar:

            if (P != NULL)
              {
              if (MList[mindex] == mlActive)
                OP = &P->L.ActivePolicy;
              else if (MList[mindex] == mlIdle)
                OP = P->L.IdlePolicy;
              else if (MList[mindex] == mlSystem)
                OP = P->L.JobPolicy;
 
              OID = P->Name;
              }
 
            if (MList[mindex] == mlActive)
              DP = &MPar[0].L.ActivePolicy;
            else if (MList[mindex] == mlIdle)
              DP = MPar[0].L.IdlePolicy;
            else if (MList[mindex] == mlSystem)
              DP = MPar[0].L.JobPolicy;

            break;         

          default:

            /* NOT HANDLED */

            break;
          }  /* END switch (OList2[oindex2]) */

        if (OP == NULL)
          continue;
 
        /* check usage against the limit for each policy type MaxProc,MaxNode,MaxMem, etc. */

        for (ptindex = 0;PList[ptindex] != mptNONE;ptindex++)
          {
          mbool_t PolicyCheckFail = FALSE;
          int     FailPartitionIndex = 0;

          if ((J->Credential.Q != NULL) &&
              (bmisset(&J->Credential.Q->ExemptFromLimits,PList[ptindex])))
            continue;

          /* check the ALL partition (and the requested partition if per partition scheduling) */

          if (MPolicyCheckLimit(
                &JUsage,
                FALSE,
                PList[ptindex],
                PLevel,
                0,   /* ALL partition */
                OP,  /* object pointer */
                DP,  /* default object pointer */
                bmisset(&EffOverrideCredBM,OList1[oindex1]) ? QP : NULL,
                &tmpL,
                Remaining,
                &UsedOverride) == FAILURE)
            {
            PolicyCheckFail = TRUE;
            FailPartitionIndex = 0;
            }
          else if ((parindex > 0) &&
                   (MPolicyCheckLimit(
                      &JUsage,
                      FALSE,
                      PList[ptindex],
                      PLevel,
                      parindex,
                      OP,  /* object pointer */
                      DP,  /* default object pointer */
                      bmisset(&EffOverrideCredBM,OList1[oindex1]) ? QP : NULL,
                      &tmpL,
                      NULL, /* Remaining if non-null was already decremented in the MPolicyCheckLimit() call above */
                      &UsedOverride) == FAILURE))
            {
            PolicyCheckFail = TRUE;
            FailPartitionIndex = parindex;
            }

          if (PolicyCheckFail == TRUE)
            {
            int AdjustVal;

            switch (PList[ptindex])
              {
              case mptMaxJob:       AdjustVal = JUsage.Job;       break;
              case mptMaxArrayJob:  AdjustVal = JUsage.ArrayJob;  break;
              case mptMaxNode:      AdjustVal = JUsage.Node;      break;
              case mptMaxPE:        AdjustVal = JUsage.PE;        break;
              case mptMaxProc:      AdjustVal = JUsage.Proc;      break;
              case mptMaxPS:        AdjustVal = JUsage.PS;        break;
              case mptMaxWC:        AdjustVal = JUsage.WC;        break;
              case mptMaxMem:       AdjustVal = JUsage.Mem;       break;
              case mptMinProc:      AdjustVal = JUsage.Proc;      break;
              case mptMaxGRes:      AdjustVal = MSNLGetIndexCount(&JUsage.GRes,0); break;
              default: AdjustVal = 0; break;
              }

            if (Msg != NULL)
              {
              snprintf(Msg,MMAX_LINE,"job %s violates %s%s %s %s limit of %ld for %s %s %s partition %s (Req: %d  InUse: %lu)",
                J->Name,
                (UsedOverride == TRUE) ? "QoS override " : "",
                MType[mindex],
                MPolicyMode[PLevel],   
                (MList[mindex] == mlIdle) ? 
                  MIdlePolicyType[PList[ptindex]] : 
                  MPolicyType[PList[ptindex]],
                tmpL,
                MXO[OList2[oindex2]],
                ((OID != NULL) ? OID : NONE),
                ((oindex1 != mxoNONE) ? MXO[OList1[oindex1]] : ""),
                MPar[FailPartitionIndex].Name,
                AdjustVal,
                OP->Usage[PList[ptindex]][FailPartitionIndex]);
              }  /* END if (Msg != NULL) */

            MDB(3,fSCHED) MLog("INFO:     job %s violates %s %s %s limit of %ld for %s %s %s partition %s (Req: %d, InUse: %lu)\n",
              J->Name,
              MType[mindex],
              MPolicyMode[PLevel],
              (MList[mindex] == mlIdle) ? MIdlePolicyType[PList[ptindex]] : MPolicyType[PList[ptindex]],
              tmpL,
              (UsedOverride == TRUE) ? MXO[mxoQOS] : MXO[OList2[oindex2]],
              (UsedOverride == TRUE) ? J->Credential.Q->Name : ((OID != NULL) ? OID : NONE),
              (UsedOverride == TRUE) ? MXO[mxoQOS] : ((oindex1 != mxoNONE) ? MXO[OList1[oindex1]] : ""),
              MPar[FailPartitionIndex].Name,
              AdjustVal,
              OP->Usage[PList[ptindex]][FailPartitionIndex]);

            MSNLFree(&JUsage.GRes);

            return(FAILURE);
            }  /* END if (PolicyCheckFail == TRUE) */
          }    /* END for (ptindex) */
        }      /* END for (oindex2) */ 
      }        /* END for (oindex1) */
    }          /* END for (mindex) */

  MSNLFree(&JUsage.GRes);

  return(SUCCESS);
  }  /* END MJobCheckCredentialLimits() */


/**
 * Check fairshare limits.
 * 
 * @see MJobCheckLimits - parent
 *
 * @param J (I)
 * @param PLevel (I)
 * @param P (I)
 * @param MLimitBM (I) [bitmap of enum ml*]
 * @param Remaining (O) (optional) number of jobs just like this one that can be run - only valid this instant
 * @param Msg (O) [optional,minsize=MMAX_LINE]
 */

int MJobCheckFairshareTreeLimits(

  const mjob_t  *J,
  enum MPolicyTypeEnum PLevel,
  const mpar_t  *P,
  mbitmap_t     *MLimitBM,
  long          *Remaining,
  char          *Msg)

  {
  int PolicyChecked[mptLAST];

  const char *MType[] = {
    "active",
    "idle",
    "system",
    NULL };

  enum MActivePolicyTypeEnum PList[]  = { 
    mptMaxJob, 
    mptMaxProc, 
    mptMaxNode, 
    mptMaxPS, 
    mptMaxPE, 
    mptMaxWC, 
    mptMinProc,
    mptMaxMem,
    mptMaxGRes,
    mptMaxArrayJob,
    mptNONE };

  enum MLimitTypeEnum MList[] = { 
    mlActive, 
    mlIdle, 
    mlSystem, 
    mlNONE };

  int ptindex; /* policy type index */
  int mindex;
  int tindex;

  long tmpL;

  mpu_t *OP;  /* object policies */
  mpu_t *QP;  /* QoS specified override policies */

  static mpu_t **fstPU = NULL;

  mpc_t JUsage;

  memset(&JUsage,0,sizeof(JUsage));
 
  MSNLInit(&JUsage.GRes);

  MPCFromJob(J,&JUsage);

  /* fairshare tree processing */

  if (MSched.FSTreeDepth <= 0)
    {
    MSNLFree(&JUsage.GRes);

    return(SUCCESS);
    }

  /* malloc the fairshare search array if not already done */

  if (fstPU == NULL)
    {
    fstPU = (mpu_t **)MUCalloc(1,sizeof(mpu_t *) * (MSched.FSTreeDepth + 1));
    }
  else
    {
    fstPU[0] = NULL;         
    }

  /* populate the fairshare search array with pointers to the fairshare configuration 
   * for active limits, then walk through the array checking fairshare policy against usage.
   * Repeat the process for idle limits, and then system limits */

  for (mindex = 0;MList[mindex] != mlNONE;mindex++)
    {
    mfst_t *TData = NULL;
    int NumFSEntries = 0;

    if (!bmisset(MLimitBM,MList[mindex]))
      continue;

    /* Note that the fstPU indexes start at the leaf nodes of the fairshare tree which are the
     * most specific (e.g. user) and as the index increments it points to the parent data
     * which is more general. (e.g. class and then account) */

    if (J->FairShareTree != NULL)
      {
      if (J->FairShareTree[P->Index] != NULL)
        {
        TData = J->FairShareTree[P->Index]->TData;
        }
      else if (J->FairShareTree[J->Req[0]->PtIndex] != NULL)
        {
        TData = J->FairShareTree[J->Req[0]->PtIndex]->TData;
        }
      else if (J->FairShareTree[0] != NULL)
        {
        TData = J->FairShareTree[0]->TData;
        }
      }

    /* Populate the fstPU array. Note this can be a sparse array so NULL entries are OK */
    /* We keep track of the number of entries to search later in NumFSEntries */

    while ((TData != NULL) && (NumFSEntries < MSched.FSTreeDepth))
      {
      if (MList[mindex] == mlActive)
        fstPU[NumFSEntries++] = &TData->L->ActivePolicy;
      else if (MList[mindex] == mlIdle)
        fstPU[NumFSEntries++] = TData->L->IdlePolicy;
      else
        fstPU[NumFSEntries++] = NULL;
   
      if (TData->Parent == NULL)
        break;
   
      /* move to the next higher leaf in the tree */

      TData = TData->Parent->TData;
      }

    /* fairshare tree limits: user, account, class, override w/QOS */

    /* locate override limits */

    if (J->Credential.Q != NULL)
      {
      if (MList[mindex] == mlActive)
        QP = J->Credential.Q->L.OverrideActivePolicy;
      else if (MList[mindex] == mlIdle)
        QP = J->Credential.Q->L.OverrideIdlePolicy;
      else if (MList[mindex] == mlSystem)
        QP = J->Credential.Q->L.OverriceJobPolicy;
      else
        QP = NULL;
      }
    else
      {
      QP = NULL;
      }

    memset(PolicyChecked,0,sizeof(PolicyChecked));

    /* Check each non NULL entry in the fairshare search array against all 
       applicable policies.  Once a policy has passed a check then do not check
       it again (e.g. if the user MAXJOB policy was found to be set in the 
       fairshare tree and it has been checked and passed then do not check 
       MAXJOB again for the remaining indexes (acct , class, etc). */

    for (tindex = 0;tindex < NumFSEntries;tindex++)
      {
      OP = fstPU[tindex];

      if (OP == NULL)
        continue;

      /* check usage against the limit for each policy type MaxProc,MaxNode,MaxMem, etc. */

      for (ptindex = 0;PList[ptindex] != mptNONE;ptindex++)
        {

        /* See if this policy type has already been checked and succeeded for a previous index */

        if ((MSched.FSMostSpecificLimit == TRUE) && (PolicyChecked[ptindex] == TRUE))
          continue;

        if (MPolicyCheckLimit(
              &JUsage,
              FALSE,
              PList[ptindex],
              PLevel,
              0, /* NOTE:  change to P->Index for per partition limits */
              OP,
              NULL,
              QP,
              &tmpL,
              Remaining,
              NULL) == FAILURE)
          {
          int AdjustVal;

          switch (PList[ptindex])
            {
            case mptMaxJob:  AdjustVal = JUsage.Job;  break;
            case mptMaxNode: AdjustVal = JUsage.Node; break;
            case mptMaxPE:   AdjustVal = JUsage.PE;   break;
            case mptMaxProc: AdjustVal = JUsage.Proc; break;
            case mptMaxPS:   AdjustVal = JUsage.PS;   break;
            case mptMaxWC:   AdjustVal = JUsage.WC;   break;
            case mptMaxMem:  AdjustVal = JUsage.Mem;  break;
            case mptMinProc: AdjustVal = JUsage.Proc; break;
            case mptMaxGRes: AdjustVal = MSNLGetIndexCount(&JUsage.GRes,0); break;
            default: AdjustVal = 0; break;
            }

          if (Msg != NULL)
            {
            sprintf(Msg,"job %s violates %s %s %s limit of %ld (Req: %d  InUse: %ld)",
              J->Name,
              MType[mindex],
              MPolicyMode[PLevel],   
              MPolicyType[PList[ptindex]],
              tmpL,
              AdjustVal,
              OP->Usage[PList[ptindex]][0]);
            }  /* END if (Msg != NULL) */

          MDB(3,fSCHED) MLog("INFO:     job %s violates %s %s %s limit of %d (Req: %d, InUse: %lu)\n",
            J->Name,
            MType[mindex],
            MPolicyMode[PLevel],
            MPolicyType[PList[ptindex]],
            tmpL,
            AdjustVal,
            OP->Usage[PList[ptindex]][0]);

          MSNLFree(&JUsage.GRes);

          return(FAILURE);
          }  /* END if (MPolicyCheckLmit() == FAILURE) */

        /* if there was a limit value for this policy type and the check succeeded
         * then we do not need to check this policy again for subsequent tindex entries */

        if (tmpL != 0)
          {
          PolicyChecked[ptindex] = TRUE;
          }
        }    /* END for (ptindex) */
      }      /* END for (tindex) */ 
    }        /* END for (mindex) */

  MSNLFree(&JUsage.GRes);

  return(SUCCESS);
  }  /* END MJobCheckFairshareTreeLimits() */





/**
 * Check vc limits.
 * 
 * @see MJobCheckLimits - parent
 *
 * @param J (I)
 * @param PLevel (I)
 * @param P (I)
 * @param MLimitBM (I) [bitmap of enum ml*]
 * @param Remaining (O) (optional) number of jobs just like this one that can be run - only valid this instant
 * @param Msg (O) [optional,minsize=MMAX_LINE]
 */

int MJobCheckVCLimits(

  const mjob_t  *J,
  enum MPolicyTypeEnum PLevel,
  const mpar_t  *P,
  mbitmap_t     *MLimitBM,
  long          *Remaining,
  char          *Msg)

  {
  int VCCount;

  const char *MType[] = {
    "active",
    "idle",
    "system",
    NULL };

  enum MActivePolicyTypeEnum PList[]  = { 
    mptMaxJob, 
    mptMaxProc, 
    mptMaxNode, 
    mptMaxPS, 
    mptMaxPE, 
    mptMaxWC, 
    mptMinProc,
    mptMaxMem,
    mptMaxGRes,
    mptMaxArrayJob,
    mptNONE };

  enum MLimitTypeEnum MList[] = { 
    mlActive, 
    mlIdle, 
    mlSystem, 
    mlNONE };

  int ptindex; /* policy type index */
  int mindex;

  long Limit;

  mpu_t *OP;  /* object policies */
  mpu_t *QP;  /* QoS specified override policies */

  mpu_t **VCPU = NULL;

  mpc_t JUsage;

  if ((MJobGetVCThrottleCount(J,&VCCount) == FAILURE) ||
      (VCCount == 0))
    {
    return(SUCCESS);
    }

  memset(&JUsage,0,sizeof(JUsage));
 
  MSNLInit(&JUsage.GRes);

  MPCFromJob(J,&JUsage);

  VCPU = (mpu_t **)MUCalloc(1,sizeof(mpu_t *) * (VCCount + 1));

  /* populate the fairshare search array with pointers to the fairshare configuration 
   * for active limits, then walk through the array checking fairshare policy against usage.
   * Repeat the process for idle limits, and then system limits */

  for (mindex = 0;MList[mindex] != mlNONE;mindex++)
    {
    mln_t *tmpL;
    mvc_t *tmpVC;

    void *Ptr;

    int VCCount = 0;

    if (!bmisset(MLimitBM,MList[mindex]))
      continue;

    /* populate VCPU with relevant VCs */

    for (tmpL = J->ParentVCs;tmpL != NULL;tmpL = tmpL->Next)
      {
      tmpVC = (mvc_t *)tmpL->Ptr;

      if (tmpVC == NULL)
        continue;
       
      Ptr = NULL;
 
      if (MVCGetDynamicAttr(tmpVC,mvcaThrottlePolicies,&Ptr,mdfOther) == SUCCESS)
        {
        if (MList[mindex] == mlActive)
          VCPU[VCCount++] = &((mcredl_t *)Ptr)->ActivePolicy;
        else if (MList[mindex] == mlIdle)
          VCPU[VCCount++] = ((mcredl_t *)Ptr)->IdlePolicy;
        else
          VCPU[VCCount++] = NULL;
        }
      }  /* END for (ParentVCs) */

    /* locate override limits */

    if (J->Credential.Q != NULL)
      {
      if (MList[mindex] == mlActive)
        QP = J->Credential.Q->L.OverrideActivePolicy;
      else if (MList[mindex] == mlIdle)
        QP = J->Credential.Q->L.OverrideIdlePolicy;
      else if (MList[mindex] == mlSystem)
        QP = J->Credential.Q->L.OverriceJobPolicy;
      else
        QP = NULL;
      }
    else
      {
      QP = NULL;
      }

    /* Check each non NULL entry in the fairshare search array against all 
       applicable policies.  Once a policy has passed a check then do not check
       it again (e.g. if the user MAXJOB policy was found to be set in the 
       fairshare tree and it has been checked and passed then do not check 
       MAXJOB again for the remaining indexes (acct , class, etc). */

    for (VCCount = 0;VCPU[VCCount] != NULL;VCCount++)
      {
      OP = VCPU[VCCount];

      if (OP == NULL)
        continue;

      /* check usage against the limit for each policy type MaxProc,MaxNode,MaxMem, etc. */

      for (ptindex = 0;PList[ptindex] != mptNONE;ptindex++)
        {
        if (MPolicyCheckLimit(
              &JUsage,
              FALSE,
              PList[ptindex],
              PLevel,
              0, /* NOTE:  change to P->Index for per partition limits */
              OP,
              NULL,
              QP,
              &Limit,
              Remaining,
              NULL) == FAILURE)
          {
          int AdjustVal;

          switch (PList[ptindex])
            {
            case mptMaxJob:  AdjustVal = JUsage.Job;  break;
            case mptMaxNode: AdjustVal = JUsage.Node; break;
            case mptMaxPE:   AdjustVal = JUsage.PE;   break;
            case mptMaxProc: AdjustVal = JUsage.Proc; break;
            case mptMaxPS:   AdjustVal = JUsage.PS;   break;
            case mptMaxWC:   AdjustVal = JUsage.WC;   break;
            case mptMaxMem:  AdjustVal = JUsage.Mem;  break;
            case mptMinProc: AdjustVal = JUsage.Proc; break;
            case mptMaxGRes: AdjustVal = MSNLGetIndexCount(&JUsage.GRes,0); break;
            default: AdjustVal = 0; break;
            }

          if (Msg != NULL)
            {
            sprintf(Msg,"job %s violates %s %s %s limit of %ld (Req: %d  InUse: %ld)",
              J->Name,
              MType[mindex],
              MPolicyMode[PLevel],   
              MPolicyType[PList[ptindex]],
              Limit,
              AdjustVal,
              OP->Usage[PList[ptindex]][0]);
            }  /* END if (Msg != NULL) */

          MDB(3,fSCHED) MLog("INFO:     job %s violates %s %s %s limit of %d (Req: %d, InUse: %lu)\n",
            J->Name,
            MType[mindex],
            MPolicyMode[PLevel],
            MPolicyType[PList[ptindex]],
            Limit,
            AdjustVal,
            OP->Usage[PList[ptindex]][0]);

          MSNLFree(&JUsage.GRes);
          MUFree((char **)&VCPU);

          return(FAILURE);
          }  /* END if (MPolicyCheckLmit() == FAILURE) */
        }    /* END for (ptindex) */
      }      /* END for (tindex) */ 
    }        /* END for (mindex) */

  MSNLFree(&JUsage.GRes);
  MUFree((char **)&VCPU);

  return(SUCCESS);
  }  /* END MJobCheckVCLimits() */

/* END MJobCheckLimits.c */
