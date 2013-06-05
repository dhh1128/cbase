/* HEADER */

      
/**
 * @file MJobCL.c
 *
 * Contains: Job Control List functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 * Build job cred list (J->Cred.CL) from existing attributes and credentials.
 *
 * NOTE: this routine will free the old CL and create a new one
 *
 * @param J (I) [modified,memory alloc'd if required]
 */

int MJobBuildCL(
 
  mjob_t *J)  /* I (modified,memory alloc'd if required) */
 
  {
  int cindex;
 
  if (!MJobPtrIsValid(J))
    {
    return(FAILURE);
    }

  if (J->Credential.CL != NULL)
    MACLFreeList(&J->Credential.CL);

  /* NOTE:  ACL info used by both job and job reservations */
 
  if (J->ReqRID != NULL)
    {
    mbitmap_t BM;

    bmset(&BM,maclRequired);

    MACLSet(
      &J->RequiredCredList,
      maRsv,
      J->ReqRID,
      mcmpSEQ,
      mnmPositiveAffinity,
      &BM,
      FALSE);
    }  /* END if (J->ReqRID != NULL) */

  /* insert user, group, account, QOS, and class cred info */

  MACLSet(&J->Credential.CL,maJob,J->Name,mcmpSEQ,mnmPositiveAffinity,0,FALSE);
 
  if (J->Credential.U != NULL)
    {
    MACLSet(&J->Credential.CL,maUser,J->Credential.U->Name,mcmpSEQ,mnmPositiveAffinity,0,FALSE);
    }
 
  if (J->Credential.G != NULL)
    {
    MACLSet(&J->Credential.CL,maGroup,J->Credential.G->Name,mcmpSEQ,mnmPositiveAffinity,0,FALSE);
    }

  if ((bmisset(&MSched.Flags,mschedfExtendedGroupSupport)) &&
      (J->Credential.U != NULL))
    {
    mln_t *tmpL = J->Credential.U->F.GAL;

    while (tmpL != NULL)
      {
      MACLSet(&J->Credential.CL,maGroup,tmpL->Name,mcmpSEQ,mnmPositiveAffinity,NULL,FALSE);

      tmpL = tmpL->Next;
      }  /* END while (tmpL != NULL) */
    }    /* END if (bmisset(&MSched.Flags,mschedfExtendedGroupSupport)) */
 
  if (J->Credential.A != NULL)
    {
    MACLSet(&J->Credential.CL,maAcct,J->Credential.A->Name,mcmpSEQ,mnmPositiveAffinity,0,FALSE);
    }
 
  if ((J->Credential.Q != NULL) && (J->Credential.Q->Name[0] != '\0') && (strcmp(J->Credential.Q->Name,DEFAULT)))
    { 
    MACLSet(&J->Credential.CL,maQOS,J->Credential.Q->Name,mcmpSEQ,mnmPositiveAffinity,0,FALSE);
    }
 
  if (J->Credential.C != NULL)
    { 
    MACLSet(&J->Credential.CL,maClass,J->Credential.C->Name,mcmpSEQ,mnmPositiveAffinity,0,FALSE);
    }

  /* add reservation access info */

  if (J->RsvAList != NULL)
    {
    for (cindex = 0;cindex < MMAX_PJOB;cindex++)
      {
      if (J->RsvAList[cindex] == NULL)
        break;

      MACLSet(&J->Credential.CL,maRsv,J->RsvAList[cindex],mcmpSEQ,mnmPositiveAffinity,0,TRUE);
      }
    }

  if (J->SpecWCLimit[0] > 0)
    {
    MACLSet(&J->Credential.CL,maDuration,&J->SpecWCLimit[0],mcmpEQ,mnmPositiveAffinity,0,FALSE);
    }

  if (J->Request.TC > 0)
    {
    long PC = J->TotalProcCount;

    MACLSet(&J->Credential.CL,maProc,&PC,mcmpEQ,mnmPositiveAffinity,0,FALSE);
    }

  if ((J->Req[0] != NULL) && (J->Req[0]->DRes.Mem > 0))
    {
    MACLSet(&J->Credential.CL,maMemory,&J->Req[0]->DRes.Mem,mcmpEQ,mnmPositiveAffinity,0,FALSE);
    }

  if ((J->Request.TC > 0) && (J->SpecWCLimit[0] > 0))
    {
    long PS = J->TotalProcCount * J->SpecWCLimit[0];
 
    MACLSet(&J->Credential.CL,maPS,&PS,mcmpEQ,mnmPositiveAffinity,0,FALSE);
    }

  if (J->SystemID != NULL)
    {
    MACLSet(&J->Credential.CL,maCluster,J->SystemID,mcmpSEQ,mnmPositiveAffinity,0,FALSE);
    }

  if (!bmisclear(&J->AttrBM))
    {
    /* add job attr info */

    for (cindex = 0;cindex < MMAX_ATTR;cindex++)
      {
      if (MAList[meJFeature][cindex][0] == '\0')
        break;

      if (bmisset(&J->AttrBM,cindex))
        {
        MACLSet(&J->Credential.CL,maJAttr,MAList[meJFeature][cindex],mcmpSEQ,mnmPositiveAffinity,0,TRUE);
        }
      }    /* END for (cindex) */
    }      /* END if (!bmisclear(&J->AttrBM)) */

  if (J->Credential.Templates != NULL)
    {
    int jindex;

    /* add job template info */

    for (jindex = 0;J->Credential.Templates[jindex] != NULL;jindex++)
      {
      if (jindex >= MSched.M[mxoxTJob])
        break;

      MACLSet(&J->Credential.CL,maJTemplate,J->Credential.Templates[jindex],mcmpSEQ,mnmPositiveAffinity,0,TRUE);
      }
    }

  if (J->ParentVCs != NULL)
    {
    mln_t *tmpL = NULL;
    mvc_t *tmpVC = NULL;

    for (tmpL = J->ParentVCs;tmpL != NULL;tmpL = tmpL->Next)
      {
      tmpVC = (mvc_t *)tmpL->Ptr;

      if (tmpVC == NULL)
        continue;

      MACLSet(&J->Credential.CL,maVC,(void *)tmpVC->Name,mcmpSEQ,mnmPositiveAffinity,0,TRUE);
      }
    }  /* END if (J->ParentVCs != NULL) */

  /* add start priority info */

  MACLSet(&J->Credential.CL,maJPriority,&J->CurrentStartPriority,mcmpLE,mnmPositiveAffinity,0,FALSE);

  return(SUCCESS);
  }  /* MJobBuildCL() */ 




/**
 * calls MACLCopy to copy access control lists from destination to source job
 *
 * @param Dst (O)
 * @param Src (I)
 */

int MJobCopyCL(

  mjob_t       *Dst,
  const mjob_t *Src)

  {
  if (Src == NULL || Dst == NULL)
    {
    return(FAILURE);
    }

  /* allocate storage for acl */

  MACLCopy(&Dst->Credential.ACL,Src->Credential.ACL);
  MACLCopy(&Dst->Credential.CL,Src->Credential.CL);

  Dst->Credential.U = Src->Credential.U;
  Dst->Credential.G = Src->Credential.G;
  Dst->Credential.A = Src->Credential.A;
  Dst->Credential.C = Src->Credential.C;
  Dst->Credential.Q = Src->Credential.Q;

  if (Src->Credential.Templates != NULL)
    {
    if (Dst->Credential.Templates == NULL)
      {
      Dst->Credential.Templates = (char **)MUCalloc(1,sizeof(char *) * MSched.M[mxoxTJob]);
      }

    memcpy(Dst->Credential.Templates,Src->Credential.Templates,sizeof(char *) * MSched.M[mxoxTJob]);
    }  /* END if (Src->Cred.Templates != NULL) */

  return(SUCCESS);
  } /* END MJobCopyCL() */
/* END MJobCL.c */
