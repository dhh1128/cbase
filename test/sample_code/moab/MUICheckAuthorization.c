/* HEADER */

/**
 * @file MUICheckAuthorization.c
 *
 * Contains MUI Check Authorization function
 */



#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 * Verify user/peer can perform requested operation on specified object.
 *
 * @param U (I) [optional]
 * @param P (I) [optional]
 * @param O (I) [optional]
 * @param OType (I) [optional]
 * @param Cmd (I)
 * @param CmdSubType (I) [optional,uses different enum per object]
 * @param IsAdmin (O) [optional]
 * @param EMsg (O) [optional]
 * @param EMsgSize (I)
 */

int MUICheckAuthorization(

  mgcred_t            *U,          /* I (optional) */
  mpsi_t              *P,          /* I (optional) */
  void                *O,          /* I pointer to object requested (optional) */
  enum MXMLOTypeEnum   OType,      /* I object type requested (optional) */
  enum MSvcEnum        Cmd,        /* I */
  int                  CmdSubType, /* I (optional,uses different enum per object) */
  mbool_t             *IsAdmin,    /* O (optional) */
  char                *EMsg,       /* O (optional) */
  int                  EMsgSize)   /* I */

  {
  mbitmap_t  AuthBM;

  mqos_t *Q = NULL;

  mbool_t ReqAuthorized;
 
  /* const char FName = "MUICheckAuthorization"; */

  if (IsAdmin != NULL)
    *IsAdmin = FALSE;

  if (EMsg != NULL)
    EMsg[0] = '\0';

  /* check general admin access */

  if (U != NULL)
    {
    MSysGetAuth(U->Name,Cmd,CmdSubType,&AuthBM);
    }
  else if (P != NULL)
    {
    MSysGetAuth(P->Name,Cmd,CmdSubType,&AuthBM);
    }
  else
    {
    MSysGetAuth("DEFAULT",Cmd,CmdSubType,&AuthBM);
    }

  if (bmisset(&AuthBM,mcalGranted))
    {
    if (IsAdmin != NULL)
      *IsAdmin = TRUE;

    return(SUCCESS);
    }  /* END if (bmisset(&AuthBM,mcalGranted)) */

  if ((Cmd == mcsDiagnose) &&
      (MUserHasPrivilege(U,OType,moaDiagnose) == TRUE))
    {
    return(SUCCESS);
    }

  /* check object ownership */
 
  ReqAuthorized = FALSE;

  switch (OType)
    {
    case mxoJob:
      {
      /* use MJobCtlCmdEnum for CmdSubType */

      mjob_t *J;

      if (O == NULL)
        {
        return(FAILURE);
        }

      J = (mjob_t *)O;

      switch (CmdSubType)
        {
        case mjcmCancel:
        case mjcmTerminate:

          if ((U != NULL) &&
              (MSched.Admin[U->Role].AllowCancelJobs == TRUE))
            {
            ReqAuthorized = TRUE;
            }
          else if ((U != NULL) && (MUserHasPrivilege(U,mxoJob,moaModify) == TRUE))
            {
            ReqAuthorized = TRUE;
            }
          else
            {
            mfst_t *TData;

            if ((J->FairShareTree != NULL) && (J->FairShareTree[J->Req[0]->PtIndex] != NULL))
              TData = J->FairShareTree[J->Req[0]->PtIndex]->TData;
            else if ((J->FairShareTree != NULL) && (J->FairShareTree[0] != NULL))
              TData = J->FairShareTree[0]->TData;
            else
              TData = NULL;

            if (MUserIsOwner(U,J->Credential.U,J->Credential.A,J->Credential.C,J->Credential.C,&J->PAL,mxoClass,TData) == TRUE)
              ReqAuthorized = TRUE;

            if (MVCUserHasAccessToObject(U,(void *)J,mxoJob) == TRUE)
              {
              /* User was granted access by VC */

              ReqAuthorized = TRUE;
              }
            }

          break;

        case mjcmStart:

          /* Unauthorized users should not be able to run this command */

          break;

        case mjcmQuery:

          if (Cmd != mcsCheckJob) /* this is checkjob, handled differently */
            {
            mbitmap_t BM;   /* bitmap of enum MRoleEnum */

            /* changed to be more flexible.  This is going off of mdiag rather than mjobctl
               as the mjobctl command can do a lot more and mdiag is safer.  This is also to
               address MAP issues */

            MSysGetAuth(U->Name,mcsDiagnose,0,&BM);

            if (bmisset(&BM,mcalOwner) || bmisset(&BM,mcalGranted))
              {
              ReqAuthorized = TRUE;
              }
            }

          if ((CmdSubType == mjcmQuery) &&
              (U != NULL) &&
              (U->ViewpointProxy == TRUE))
            {
            ReqAuthorized = TRUE;
            }

          if (U == NULL)
            {
            U = MSched.DefaultU;
            }

          if (MUserIsOwner(U,J->Credential.U,J->Credential.A,J->Credential.C,O,&J->PAL,mxoJob,NULL) == TRUE)
            {
            ReqAuthorized = TRUE;
            }

          if (MVCUserHasAccessToObject(U,(void *)J,mxoJob) == TRUE)
            {
            /* User was granted access by VC */

            ReqAuthorized = TRUE;
            }

          break;

        case mjcmRequeue:
        default:
          
          /* users can requeue their own jobs */

          if (MUserIsOwner(U,
                J->Credential.U,
                NULL,
                NULL,
                NULL,
                NULL,
                mxoJob,
                NULL) == TRUE)
            {
            ReqAuthorized = TRUE;
            }

          if (MVCUserHasAccessToObject(U,(void *)J,mxoJob) == TRUE)
            {
            /* User was granted access by VC */

            ReqAuthorized = TRUE;
            }

          if (MUserHasPrivilege(U,mxoJob,moaModify) == TRUE)
            {
            ReqAuthorized = TRUE;
            }



          break;  
        }  /* END switch (CmdSubType) */
      }     /* END BLOCK (case mxoJob) */

      break;

    case mxoNode:
      {
      /* use MSubCmdEnum for CmdSubType */

      mnode_t *N;

      switch (CmdSubType)
        {
        case mncmQuery:
        case mncmModify:

          if ((U != NULL) &&
              (U->ViewpointProxy == TRUE))
            {
            ReqAuthorized = TRUE;

            return(SUCCESS);
            }

          /* fallthrough */

        default:

          if (O == NULL)
            {
            if (EMsg != NULL)
              strcpy(EMsg,"object not specified");

            return(FAILURE);
            }

          N = (mnode_t *)O;

          if (MUserIsOwner(U,N->Cred.U,N->Cred.A,N->Cred.C,NULL,NULL,mxoNONE,NULL) == TRUE)
            {
            ReqAuthorized = TRUE;
            }
          else if (MUserHasPrivilege(U,mxoNode,(CmdSubType == mncmModify) ? moaModify : moaDiagnose) == TRUE)
            {
            ReqAuthorized = TRUE;
            }
          else
            {
            if (EMsg != NULL)
              {
              snprintf(EMsg,EMsgSize, "%suser %s%s%snot authorized for node '%s'\n",
                EMsg,
                (U == NULL) ? "" : "'",
                (U == NULL) ? "" : U->Name,
                (U == NULL) ? "" : "' ",
                N->Name);
              }     /* END if (MNodeFindVPCPar(N,&NodeVPCP) == SUCCESS) */
            }       /* END else (MUserIsOwner(U,N->Cred.U,N->Cred.A,N->Cred.C,NULL,NULL,mxoNONE) == TRUE) */

          break;
        }  /* END switch (CmdSubType) */
      }    /* END case mxoNode */

      break;

    case mxoPar:

      /* use MSubCmdEnum for CmdSubType */

      switch (CmdSubType)
        {
        case msctlCreate:

          MUserGetCreds(U,NULL,NULL,NULL,&Q);

          if (Q == NULL)
            Q = MSched.DefaultQ;

          if ((Q == NULL) || !bmisset(&Q->Flags,mqfEnableUserRsv))
            {
            if (EMsg != NULL)
              {
              /* NOTE:  remove all snprintf(EMsg,"%s",EMsg) calls */

              snprintf(EMsg,EMsgSize,"%sVPC creation not enabled for QOS '%s'\n",
                EMsg,
                Q->Name);
              }

            return(FAILURE);
            }

          /* user's default qos allows PRsv */

          ReqAuthorized = TRUE;

          break;

        case msctlList:

          {
          mpar_t *P;

          if (O == NULL)
            {
            /* list vpcprofiles */

            ReqAuthorized = TRUE;

            break;
            }

          P = (mpar_t *)O;

          if ((U == P->O) || ((U != NULL) && (U->ViewpointProxy == TRUE)))
            {
            ReqAuthorized = TRUE;
            }
          }

          break;

        default:

          {
          mpar_t *P;

          if (O == NULL)
            {
            if (EMsg != NULL)
              strcpy(EMsg,"object not specified");

            return(FAILURE);
            }

          P = (mpar_t *)O;

          if (U == P->O)
            {
            ReqAuthorized = TRUE;
            }
          }

          break;
        }  /* END switch (CmdSubType) */

      break;

    case mxoRsv:

      {
      /* use MRsvCtlCmdEnum for CmdSubType */
      
      mrsv_t *R;

      if (O == NULL)
        {
        if (EMsg != NULL)
          strcpy(EMsg,"object not specified");

        return(FAILURE);
        }

      R = (mrsv_t *)O;

      switch (CmdSubType)
        {
        case mrcmCreate:
          {
          mjob_t tmpJ;

          /* check if non-admin is authorized to create reservations */

          if (R->Q != NULL)
            {
            if (!bmisset(&R->Q->Flags,mqfEnableUserRsv))
              {
              if (EMsg != NULL)
                {
                snprintf(EMsg,EMsgSize,"personal reservations not enabled for QOS '%s'\n",
                  R->Q->Name);
                }

              return(FAILURE);
              }

            MJobInitialize(&tmpJ);

            tmpJ.Credential.U = U;
            tmpJ.Credential.G = (U != NULL) ? (mgcred_t *)U->F.GDef : NULL;

            if (MQOSGetAccess(&tmpJ,R->Q,NULL,NULL,NULL) == FAILURE)
              {
              snprintf(EMsg,EMsgSize,"access denied to QOS '%s'\n",
                R->Q->Name);

              return(FAILURE);
              }

            /* requested qos allows PRsv */

            ReqAuthorized = TRUE;
            }
          else if (MUserHasPrivilege(U,mxoRsv,moaCreate) == TRUE)
            {
            ReqAuthorized = TRUE;
            }
          else
            {
            MUserGetCreds(U,NULL,NULL,NULL,&Q);

            if (Q == NULL)
              MUserGetCreds(MSched.DefaultU,NULL,NULL,NULL,&Q);

            if (Q == NULL)
              {
              if (!bmisset(&MSched.DefaultQ->Flags,mqfEnableUserRsv))
                {
                if (EMsg != NULL)
                  {
                  char *ptr;

                  ptr = EMsg + strlen(EMsg);

                  snprintf(ptr,EMsgSize - (ptr - EMsg),"personal reservations not enabled for QOS '%s'\n",
                    R->Q->Name);
                  }
          
                return(FAILURE);
                } 

              ReqAuthorized = TRUE;

              Q = MSched.DefaultQ;
              }
            else if (!bmisset(&Q->Flags,mqfEnableUserRsv))
              {
              if (EMsg != NULL)
                {
                char *ptr;

                ptr = EMsg + strlen(EMsg);

                snprintf(EMsg,EMsgSize - (ptr - EMsg),"personal reservations not enabled for QOS '%s'\n",
                  R->Q->Name);
                }

              return(FAILURE);
              }

            /* user's default qos allows PRsv */

            R->Q = Q;

            ReqAuthorized = TRUE;
            }
          }       /* END (case mrcmCreate) */

          break;

        case mrcmDestroy:
        case mrcmList:
        case mrcmQuery:

          /* check if user has access to this reservation */

          if (((CmdSubType == mrcmQuery) || (CmdSubType == mrcmList)) &&
              ((U != NULL) && (U->ViewpointProxy == TRUE)))
            {
            ReqAuthorized = TRUE;
            }

          if ((R->OType == mxoUser) &&
              (R->O != NULL) &&
              (R->O == U))
            {
            /* user marked as owner so she can destroy or query as much
               she likes */

            ReqAuthorized = TRUE;

            return(SUCCESS);
            }

          if (R->Type == mrtJob)
            {
            mjob_t *J;

            if (R->J == NULL)
              {
              snprintf(EMsg,EMsgSize,"user '%s'\n",
                R->Q->Name);

              return(FAILURE);
              }
  
            J = R->J;
  
            if (MUserIsOwner(U,J->Credential.U,J->Credential.A,J->Credential.C,NULL,&J->PAL,mxoNONE,NULL) == TRUE)
              {
              ReqAuthorized = TRUE;
              }
            }  /* END R->Type == mrtJob */
          else if (MUserIsOwner(U,R->U,R->A,NULL,R->O,NULL,R->OType,NULL) == TRUE)
            {
            ReqAuthorized = TRUE;
            }

          if (MUserHasPrivilege(U,mxoRsv,(CmdSubType == mrcmDestroy) ? moaDestroy : moaDiagnose) == TRUE)
            {
            ReqAuthorized = TRUE;
            }

          if (ReqAuthorized == FALSE)
            {
            mjob_t *tmpJ = NULL;

            MJobMakeTemp(&tmpJ);

            MUStrCpy(tmpJ->Name,"temporary-authorization-check",MMAX_NAME);

            tmpJ->Credential.U = U;

            MJobBuildCL(tmpJ);

            MJobSetAttr(tmpJ,mjaReqReservation,(void **)R->Name,mdfString,mSet);

            if (MRsvCheckJAccess(R,tmpJ,0,NULL,FALSE,NULL,NULL,NULL,NULL) == SUCCESS)
              {
              ReqAuthorized = TRUE;
              }

            MJobFreeTemp(&tmpJ);
            }

          break;

        case mrcmModify:
   
          /* for now, don't allow end-users to modify reservations, even if they own them,
             the reason for this is because we don't know if they are trying to grow it or
             shrink it or anything else */

          if (MUserHasPrivilege(U,mxoRsv,moaModify) == TRUE)
            {
            ReqAuthorized = TRUE;
            }

          break;

        default:

          if (EMsg != NULL)
            strcpy(EMsg,"invalid subcommand");

          return(FAILURE);

          /* NOTREACHED */

          break;
        }  /* END switch (CmdSubType) */
      }     /* END BLOCK (case mxoRsv) */

      break;

    case mxoSched:

      {
      switch (CmdSubType)
        {
        case msctlList:
        case msctlLog:
        case msctlPause:
        case msctlQuery:
        case msctlResume:
        case msctlStep:
        case msctlStop:

          {
          int index = 0;

          if ((CmdSubType == msctlQuery) &&
              (U != NULL) &&
              (U->ViewpointProxy == TRUE))
            {
            ReqAuthorized = TRUE;
            }

          if (bmisset(&AuthBM,mcalAdmin1))
            {
            index = 1;
            }
          else if (bmisset(&AuthBM,mcalAdmin2))
            {
            index = 2;
            }
          else if (bmisset(&AuthBM,mcalAdmin3))
            {
            index = 3;
            }
          else if (bmisset(&AuthBM,mcalAdmin4))
            {
            index = 4;
            }
          else if (bmisset(&AuthBM,mcalAdmin5))
            {
            index = 5;
            }
            
          if (index > 0)
            {
            /* NOTE:  if 'showconfig' is allowed, 'mschedctl -l' is allowed */

            if (MUI[mcsMSchedCtl].AdminAccess[index] == TRUE)
              ReqAuthorized = TRUE;
            else if ((CmdSubType == msctlList) && (MUI[mcsShowConfig].AdminAccess[index] == TRUE))
              ReqAuthorized = TRUE;
            }

          if (((CmdSubType == msctlPause) ||
               (CmdSubType == msctlResume) ||
               (CmdSubType == msctlStop) ||
               (CmdSubType == msctlStep)) &&
              (MUserHasPrivilege(U,mxoSched,moaModify) == TRUE))
            {
            ReqAuthorized = TRUE;
            }
          else if (MUserHasPrivilege(U,mxoSched,moaDiagnose) == TRUE)
            {
            ReqAuthorized = TRUE;
            }
          }    /* END BLOCK (case msctlList) */

          break;

        case msctlModify:
        case msctlReconfig:

          if (bmisset(&AuthBM,mcalAdmin1))
            {
            if (IsAdmin != NULL)
              *IsAdmin = TRUE;

            return(SUCCESS);
            }

          ReqAuthorized = FALSE;

          if (MUserHasPrivilege(U,mxoSched,moaModify) == TRUE)
            {
            ReqAuthorized = TRUE;
            }

          /* non-admins not allowed access to query */

          if (EMsg != NULL)
            strcpy(EMsg,"admin authority required to run command");

          return(FAILURE);
                                                                                           
          /*NOTREACHED*/

          break;

        case msctlCreate:
        case msctlDestroy:
        case msctlFailure:
        case msctlInit:
        case msctlKill:
        default:

          if (EMsg != NULL)
            strcpy(EMsg,"admin authority required to run command");

          return(FAILURE);

          /*NOTREACHED*/

          break;
        }  /* END switch (CmdSubType) */
      }    /* END BLOCK (case mxoSched) */

      break;

    case mxoUser:
    case mxoGroup:
    case mxoClass:
    case mxoAcct:
    case mxoQOS:

      {
      if ((Cmd == mcsDiagnose) &&
          (MUserHasPrivilege(U,OType,moaDiagnose) == TRUE))
        {
        ReqAuthorized = TRUE;

        break;
        }

      /* use MCredCtlCmdEnum for CmdSubType */

      if (O == NULL)
        {
        if (EMsg != NULL)
          strcpy(EMsg,"object not specified");

        return(FAILURE);
        }

      switch (Cmd)
        {
        case mcsStatShow:

          CmdSubType = mccmQuery;

          break;

        default:

          /* NO-OP */

          break;
        }  /* END switch (CmdType) */

      switch (CmdSubType)
        {
        case mccmQuery:

          if (OType == mxoUser)
            {
            mgcred_t *User;
            mgcred_t *UserA = NULL;
  
            User = (mgcred_t *)O;
  
            if (U == User)
              {
              ReqAuthorized = TRUE;
              }
            else
              {
              MUserGetCreds(
                U,
                NULL,
                NULL,
                &UserA,
                NULL);

              if ((UserA != NULL) &&
                  MUserIsOwner(U,NULL,UserA,NULL,NULL,&MPar[0].F.PAL,mxoNONE,NULL))
                {
                ReqAuthorized = TRUE;
                }
              }
            }  /* END if (OType == mxoUser) */
          else if (OType == mxoAcct)
            {
            if (MUserIsOwner(U,NULL,(mgcred_t *)O,NULL,NULL,&MPar[0].F.PAL,mxoNONE,NULL))
              {
              ReqAuthorized = TRUE;
              }
            }
          else if (OType == mxoClass)
            {
            if (MUserIsOwner(U,NULL,NULL,(mclass_t *)O,NULL,NULL,mxoNONE,NULL))
              {
              ReqAuthorized = TRUE;
              }
            }
          else
            {
            if (MUserIsOwner(U,NULL,(mgcred_t *)O,NULL,NULL,NULL,mxoNONE,NULL))
              {
              ReqAuthorized = TRUE;
              }
            }    /* END else (OType == mxoUser) */

          break;

        case mccmDestroy:
        case mccmList:
        case mccmModify:
        case mccmReset:
        default:

          /* NOTE:  only admins and managers allowed to destroy, list, modify and reset creds */

          if (O == NULL)
            {
            ReqAuthorized = FALSE;
            }
          else if (OType == mxoAcct)
            {
            if (MUserIsOwner(U,NULL,(mgcred_t *)O,NULL,NULL,&MPar[0].F.PAL,mxoNONE,NULL))
              {
              ReqAuthorized = TRUE;
              }
            }
          else if (OType == mxoClass)
            {
            if (MUserIsOwner(U,NULL,NULL,(mclass_t *)O,NULL,NULL,mxoNONE,NULL))
              {
              ReqAuthorized = TRUE;
              }
            }
          else if (OType == mxoUser)
            {
            /* request is authorized if requestor is user manager or if
               requestor is queue manager and C->F.AllowUserManagement==TRUE */

            if (MCredFindManager(U->Name,U->F.ManagerU,NULL) == SUCCESS)
              ReqAuthorized = TRUE;

            /* NYI */
            }
          else if (MUserIsOwner(U,NULL,(mgcred_t *)O,NULL,NULL,NULL,mxoNONE,NULL))
            {
            ReqAuthorized = TRUE;
            }    /* END else if (MUserIsOwner()) */

          break;
        }  /* END switch (CmdSubType) */
      }    /* END BLOCK (case mxoQOS) */

      break;

    case mxoxVM:

      /* Only grant rights for create & query. */

      switch (CmdSubType)
        {
        case mvmcmModify:
        case mvmcmDestroy:
        case mvmcmMigrate:
        case mvmcmForceMigration:

          if ((U != NULL) &&
              (U->ViewpointProxy == TRUE))
            {
            ReqAuthorized = TRUE;
            }

          break;

#if 0
        case mvmcmCreate:
#endif
        case mvmcmQuery:
          ReqAuthorized = TRUE;
          break;

        /* Destroy, migrate, modify require admin rights that has already
           been checked in MSysGetAuth(),
           ie:  mvmcmDestroy, mvmcmMigrate, mvmcmModify */

        default:
          ReqAuthorized = FALSE;
          break;
        }
      break;

    case mxoRM:
    case mxoxFS:
    case mxoxPriority:
    case mxoQueue:
    case mxoxGreen:
    case mxoxLimits:
    case mxoSRsv:
    case mxoTrig:

      if ((Cmd == mcsDiagnose) &&
          (MUserHasPrivilege(U,OType,moaDiagnose) == TRUE))
        {
        ReqAuthorized = TRUE;
        }

      break;

    case mxoNONE:
    default:

      /* NOT SUPPORTED */

      if (EMsg != NULL)
        strcpy(EMsg,"request not supported");

      return(FAILURE);

      /* NOTREACHED */

      break;
    }  /* END switch (OType) */

  if (ReqAuthorized == FALSE)
    {
    if (EMsg != NULL)
      strcpy(EMsg,"request is not authorized\n"); /* add newline */

    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MUICheckAuthorization() */
/* END MUICheckAuthorization.c */
