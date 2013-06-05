
/* HEADER */

      
/**
 * @file MJobTemp.c
 *
 * Contains: MJobFreeTemp and MJobMakeTemp
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


int MJobFreeTemp(

  mjob_t **JP)

  {
  mjob_t *J = NULL;

  if ((JP == NULL) || (*JP == NULL))
    return(FAILURE);

  J = *JP;

  MUStrCpy(J->Name,"temp",MMAX_NAME);

  bmset(&J->IFlags,mjifTemporaryJob);

  MJobDestroy(JP);

  MUFree((char **)JP);

  *JP = NULL;

  return(SUCCESS);
  }  /* END MJobFreeTemp() */



/**
 * Create temporary pseudo-job.
 *
 * NOTE: should ALWAYS call "MJobFreeTemp()" after using the temporary job.
 * 
 * @see MJobFreeTemp() - peer
 *
 * @param JP (I/O) [alloced/modified]
 *
 * NOTE:  CHANGE in v6_1: this routine will almost always dynamically allocate memory,
 *                        calling function must free this memory.
 */

int MJobMakeTemp(

  mjob_t   **JP)           /* I/O (alloc) */

  {
  mjob_t *J = NULL;

  /* NOTE:  
       - will allocate ACL/CL for job if not specified 
       - associates jobs with MRM[MSched.DefaultPseudoJobRMIndex] 
       - creates single req jobs */

  if (JP == NULL)
    {
    return(FAILURE);
    }

  MJobCreate("temporary_job",FALSE,&J,FALSE);

  MReqCreate(J,NULL,NULL,TRUE);

  J->Req[0]->DRes.Procs = 1;

  J->Req[0]->SetBlock   = MBNOTSET;

  J->Req[0]->RMIndex    = 0;

  J->State       = mjsIdle;

  J->CMaxDate    = MMAX_TIME;
  J->TermTime    = MMAX_TIME;

  J->SubmitRM         = &MRM[MSched.DefaultPseudoJobRMIndex];
  J->DestinationRM         = &MRM[MSched.DefaultPseudoJobRMIndex];

  MNLInit(&J->NodeList);
  MNLInit(&J->Req[0]->NodeList);
 
  J->TaskMap     = (int *)MUMalloc(sizeof(int) * (MSched.JobMaxTaskCount));
  J->TaskMap[0]  = -1;

  J->TaskMapSize = MSched.JobMaxTaskCount;

  /* by default temp jobs can run on every partition */

  bmsetall(&J->SysPAL,MMAX_PAR);
  bmsetall(&J->SpecPAL,MMAX_PAR);
  bmsetall(&J->PAL,MMAX_PAR);
#if 0
  /* none of these are set and the RMs from above may be wrong so we can't 
     make this call */

  MJobGetPAL(J,J->SpecPAL,J->SysPAL,J->PAL);
#endif

  J->Credential.Q = NULL;

  bmset(&J->IFlags,mjifTemporaryJob);

  MJobBuildCL(J);       

  *JP = J;

  return(SUCCESS);
  }  /* END MJobMakeTemp() */
/* END MJobTemp.c */
