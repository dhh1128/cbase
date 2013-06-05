/* HEADER */

/**
 * @file MJobCred.c
 *
 * Contains Credential functions on jobs
 *
 */



#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 * Assigns the given credentials to a job. Access policies are respected (see MCredCheckAcctAccess()),
 *
 * @param J (I)
 * @param UName (I)
 * @param GName (I)
 * @param AName (I) [optional]
 * @param ForceAcct (I) force account
 * acct credential assignment should be forced, regardless of access violations, etc. A force will, however
 * result in an Idle job to have a hold placed on it and an Active job will have a warning message attached
 * to it.
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 */

int MJobSetCreds(
 
  mjob_t     *J,
  const char *UName,
  const char *GName,
  const char *AName,
  mbool_t     ForceAcct,
  char       *EMsg)
 
  {
  mgcred_t *tmpAcct = NULL;

  const char *FName = "MJobSetCreds";

  MDB(3,fSTRUCT) MLog("%s(%s,%s,%s,%s,EMsg)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    UName,
    GName,
    (AName != NULL) ? AName : "NULL");

  if (EMsg != NULL)
    { 
    EMsg[0] = '\0';
    }
  
  if ((J == NULL) || (UName == NULL) || (GName == NULL))
    {
    if (UName == NULL)
      {
      if (EMsg != NULL)
        strcpy(EMsg,"user not specified");
      }
    else if (GName == NULL)
      {
      if (EMsg != NULL)
        strcpy(EMsg,"group not specified");
      }

    return(FAILURE);
    }
 
  /* locate/add user record */
 
  if (MUserAdd(UName,&J->Credential.U) == FAILURE)
    {
    MDB(1,fSTRUCT) MLog("ERROR:    user hash table overflow on job '%s' with user %s\n",
      J->Name,
      UName);

    if (EMsg != NULL) 
      strcpy(EMsg,"cannot add user");

    return(FAILURE);
    }
 
  /* locate/add group record */
 
  if (MGroupAdd(GName,&J->Credential.G) == FAILURE) 
    {
    MDB(1,fSTRUCT) MLog("ERROR:    group hash table overflow on job '%s' with group %s\n",
      J->Name,
      GName);

    if (EMsg != NULL)
      strcpy(EMsg,"cannot add user");
 
    return(FAILURE);
    }
 
  /* locate/add account record */
 
  tmpAcct = J->Credential.A;

  if ((AName == NULL) || (AName[0] == '\0'))
    {
    if (MJobGetAccount(J,&J->Credential.A) == FAILURE)
      {
      char Message[MMAX_LINE];

      if (EMsg != NULL)
        strcpy(EMsg,"Unable to determine default account for job");

      if (MAM[0].Type != mamtNONE)
        {
        MDB(1,fSTRUCT) MLog("ERROR:    Unable to determine default account for job '%s', user '%s'\n",
          J->Name,
          J->Credential.U->Name);
 
        sprintf(Message,"AMFAILURE:  Unable to determine default account for job '%s', user '%s'\n",
          J->Name,
          J->Credential.U->Name);
 
        MSysRegEvent(Message,mactNONE,1);

        if (MAM[0].StartFailureAction == mamjfaHold)
          {
          MJobSetHold(J,mhDefer,MSched.DeferTime,mhrNoFunds,"Unable to determine default account");
          }
        else if (MAM[0].StartFailureAction == mamjfaCancel)
          {
          MJobCancel(
            J,
            "MOAB_INFO:  Unable to determine default account\n",
            FALSE,
            NULL,
            NULL);

          return(FAILURE);
          }
        }    /* END if (MAM[0].Type != mamtNONE) */
      }
    }    /* END if ((AName == NULL) || (AName[0] == '\0')) */
  else if (MAcctAdd(AName,&J->Credential.A) == FAILURE)
    {
    MDB(1,fSTRUCT) MLog("ERROR:    account table overflow on job '%s' with account %s\n",
      J->Name,
      AName);

    if (EMsg != NULL)
      strcpy(EMsg,"cannot add account");
 
    return(FAILURE);
    }

  if ((J->Credential.A != NULL) && 
       strcmp(J->Credential.A->Name,ALL) &&
       strcmp(J->Credential.A->Name,NONE))
    {
    if (MCredCheckAcctAccess(J->Credential.U,J->Credential.G,J->Credential.A) == FAILURE)
      {
      if (ForceAcct == TRUE)
        {
        if (MJOBISACTIVE(J))
          {
          MMBAdd(
            &J->MessageBuffer,
            "account access violation detected--please check",
            NULL,
            mmbtOther,
            MSched.Time + MCONST_DAYLEN,
            0,
            NULL);
          }
        else
          {
          MJobSetHold(J,mhBatch,0,mhrCredAccess,"account access violation--user cannot run jobs in given account");
          }
        }
      else
        {
        MDB(1,fSTRUCT) MLog("ERROR:    account '%s' is not accessible by job %s\n",
          J->Credential.A->Name,
          J->Name);

        if (EMsg != NULL)
          snprintf(EMsg,MMAX_LINE,"job not authorized to use requested account '%s'",
            J->Credential.A->Name);

        return(FAILURE);
        }
      }
    }    /* END if ((J->Cred.A != NULL) && ...) */

  /* If moab learned of this job through the resource manager and subsequently 
     added the account to the job then notify the RM of the account change. 
     Also check to make sure that we do not modify a pseudo job. */

  if ((!bmisset(&J->IFlags,mjifTemporaryJob)) &&
      (J->SRMJID == NULL) && 
      (tmpAcct != J->Credential.A) && 
      (J->Credential.A != NULL))
    {
    if ((J->SubmitRM != NULL) && (J->SubmitRM->NoUpdateJobRMCreds != TRUE))
      {
      if (MRMJobModify(J,"Account_Name",NULL,J->Credential.A->Name,mSet,NULL,NULL,NULL) == FAILURE)
        {
        MDB(5,fSTRUCT) MLog("INFO:     job %s account '%s' cannot be modified in RM\n",
          J->Name,
          J->Credential.A->Name);
        }
      }
    }
 
  MJobUpdateFlags(J);

  return(SUCCESS);
  }  /* END MJobSetCreds() */





/**
 * Check a given credential's access to a job template.
 *
 * @see __MUIJobTemplateCtl - parent
 *
 * @param TJob
 * @param OType
 * @param OName
 */

int MTJobCheckCredAccess(

  mjob_t             *TJob,
  enum MXMLOTypeEnum  OType,
  char               *OName)

  {
  macl_t *CList;

  int cindex;

  enum MAttrEnum AIndex;

  if ((TJob == NULL) || 
      (OType == mxoNONE) ||
      (OName == NULL) ||
      (OName[0] == '\0'))
    {
    return(FAILURE);
    }

  CList = TJob->RequiredCredList;

  switch (OType)
    {
    case mxoUser:  AIndex = maUser;  break;
    case mxoGroup: AIndex = maGroup; break;
    case mxoAcct:  AIndex = maAcct;  break;
    case mxoQOS:   AIndex = maQOS;   break;
    case mxoClass: AIndex = maClass; break;
    case mxoJob:   AIndex = maJob;   break;
    default: return(FAILURE); /*NOTREACHED*/ break;
    }

  /* check required user list */

  for (cindex = 0;CList[cindex].Type != maNONE;cindex++)
    {
    if (CList[cindex].Type != AIndex)
      continue;

    if (!strcmp(CList[cindex].Name,OName))
      break;

    if (!strcmp(CList[cindex].Name,"ALL"))
      break;
    }  /* END for (cindex) */

  if (CList[cindex].Type == maNONE)
    {
    return(FAILURE);
    }  /* END for (cindex) */

  return(SUCCESS);
  }  /* END MTJobCheckCredAccess() */





/**
 * Checks to see if creds were set by default on the job an if so, changes the default 
 * cred to the default cred according to the specified partition. 
 * Only applicable for per partition scheduling. 
 *
 * @param J (I/O) (creds on this job may be modified)
 * @param P (I) (partition)
 */

int MJobPartitionTransformCreds(

  mjob_t *J,
  mpar_t *P) 

  {

  if ((J == NULL) || (P == NULL))
    {
    return(FAILURE);
    }

  if (MSched.PerPartitionScheduling != TRUE)
    return(SUCCESS);

  if (!strcmp(P->Name,"ALL"))
    return (SUCCESS);

#if 0
  /* Originally we were thinking that we should not change the
     creds for a job with a reservation - however, in per partition
     scheduling we check the job in each partition and if it gets a
     better reservation on a different partition then the reservation
     is replaced. We must change the creds for each partition even
     if the job has a reservation. Not sure if this will cause problems
     if the job has different creds than when it got its reservation.
     If this is a problem we would have to somehow remember which
     partition the job was being scheduled for when it got its reservation
     after we are done scheduling and reset the creds back to that state. */

  if (J->R != NULL)
    return(SUCCESS);
#endif

  /* if the job is active or complete we do not need to transform the creds */

  if (MJOBISACTIVE(J))
    return(SUCCESS);

  if (MJOBISCOMPLETE(J))
    return(SUCCESS);

  /* if the class was set by default then we can change it to the
     default class for this partition */

  if ((bmisset(&J->IFlags,mjifClassSetByDefault) == TRUE) &&
      (J->Credential.C != NULL) &&
      (P->RM != NULL) &&
      (P->RM->DefaultC != NULL) &&
      (J->Credential.C != P->RM->DefaultC))
    {
    J->Credential.C = P->RM->DefaultC;
    }

  /* if the class has not been set and there is a default class for this
     partition then we can set the class to the partition default */

  if ((J->Credential.C == NULL) &&
      (P->RM != NULL) &&
      (P->RM->DefaultC != NULL))
    {
    bmset(&J->IFlags,mjifClassSetByDefault);
    J->Credential.C = P->RM->DefaultC;
    }


  /* if the QOS was set by default (from the fairshare tree) then
     we can set it to the default for this user */

  if ((bmisset(&J->IFlags,mjifQOSSetByDefault) == TRUE) &&
      (J->QOSRequested == NULL))
    {
    mqos_t *QDef = NULL;

    MQOSGetAccess(J,NULL,P,NULL,&QDef);

    if (QDef != NULL)
      MJobSetQOS(J,QDef,0);
    }

  return(SUCCESS);
  } /* END MJobPartitionTransformCreds() */




/**
 * Copy mcred_t structure.
 *
 * @param Dst
 * @param Src
 */

int MCredCopy(

  mcred_t *Dst,
  mcred_t *Src)

  {
  Dst->U = Src->U;
  Dst->G = Src->G;
  Dst->A = Src->A;
  Dst->C = Src->C;
  Dst->Q = Src->Q;

  Dst->CredType = Src->CredType;

  MACLCopy(&Dst->ACL,Src->ACL);
  MACLCopy(&Dst->CL,Src->CL);

  Dst->MTime = MSched.Time;

  /* not handled: Templates */

  return(SUCCESS);
  }  /* END MCredCopy() */


