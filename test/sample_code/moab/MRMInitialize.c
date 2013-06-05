/* HEADER */

      
/**
 * @file MRMInitialize.c
 *
 * Contains:
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  




/**
 * Issue RM-specific call to initialize communication with RM.
 *
 * @param SR (I) [optional]
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MRMInitialize(

  mrm_t                *SR,   /* I (optional) */
  char                 *EMsg, /* O (optional,minsize=MMAX_LINE) */
  enum MStatusCodeEnum *SC)   /* O (optional) */

  {
  int rmindex;

  mbool_t Failure;  

  int rc;
  
  enum MStatusCodeEnum tSC;
  enum MStatusCodeEnum *SCP;

  mrm_t *R;

  char tEMsg[MMAX_LINE];
  char *EMsgP;

  const char *FName = "MRMInitialize";
 
  MDB(2,fRM) MLog("%s(%s,EMsg,SC)\n",
    FName,
    (SR != NULL) ? SR->Name : "NULL");

  if (EMsg != NULL)
    {
    EMsg[0] = '\0';

    EMsgP = EMsg;
    }
  else
    {
    EMsgP = tEMsg;
    }

  if (SC != NULL)
    {
    *SC = mscNoError;

    SCP = &tSC;
    }
  else
    {
    SCP = &tSC;
    }

  Failure = FALSE;

  for (rmindex = 0;rmindex < MSched.M[mxoRM];rmindex++)
    {
    R = &MRM[rmindex];
  
    if (R->Name[0] == '\0')
      break;

    if (MRMIsReal(R) == FALSE)
      continue;

    if ((SR != NULL) && (R != SR))
      continue;

    /* the below code will be used for both slave peers and normal peers */

    if (!bmisset(&R->Flags,mrmfNoCreateAll) && 
        !bmisset(&R->Flags,mrmfNoCreateResource) && 
        !bmisset(&R->IFlags,mrmifLocalQueue) &&
        bmisset(&R->RTypes,mrmrtCompute))
      {
      if (R->PtIndex <= 0)
        {
        mpar_t *P;

        /* create matching partition for compute RM's which can discover/create
           nodes */

        if (MParAdd(R->Name,&P) == FAILURE)
          {
          MDB(5,fSTRUCT) MLog("ERROR:    cannot create partition for RM %s\n",
            R->Name);

          MSched.ParBufOverflow = TRUE;

          MRMDestroy(&R,NULL);

          continue;
          }

        R->PtIndex = P->Index;

        P->RM = R;

        if (R->Type == mrmtMoab)
          {
          bmset(&P->Flags,mpfIsRemote);
          }
        }
      }    /* END (!bmisset(&R->Flags,mrmfNoCreateAll) && ...) */
    else if (!bmisset(&R->Flags,mrmfNoCreateAll) &&
             !bmisset(&R->Flags,mrmfNoCreateResource) &&
            (!bmisclear(&R->RTypes)))
      {
      /* RM is special resource type RM, ie network, storage, license, provisioning, etc */

      if (R->PtIndex <= 0)
        {
        mpar_t *P;

        if (MParAdd("SHARED",&P) == FAILURE)
          {
          MDB(5,fSTRUCT) MLog("ERROR:    cannot create partition for RM %s\n",
            R->Name);

          MSched.ParBufOverflow = TRUE;

          MRMDestroy(&R,NULL);

          continue;
          }

        R->PtIndex = P->Index;

        MSched.SharedPtIndex = P->Index;
        }
      }

    if (R->Name[0] == '\0')
      {
      sprintf(R->Name,"%s.%d",
        MRMType[R->Type],
        R->Index);
      }

    /* load in CLIENTCFG information */

    if (R->ClientName[0] != '\0')
      {
      MOLoadPvtConfig((void **)R,mxoRM,R->ClientName,NULL,NULL,NULL);
      }
    else
      {
      MOLoadPvtConfig((void **)R,mxoRM,R->Name,NULL,NULL,NULL);
      }

    if ((R->P[0].CSKey == NULL) && (R->P[0].HostName != NULL))
      {
      /* attempt to look up peer by host */

      char tmpName[MMAX_NAME];
      mpsi_t *P;

      snprintf(tmpName,sizeof(tmpName),"HOST:%s",
        R->P[0].HostName);

      P = &R->P[0];

      MOLoadPvtConfig((void **)R,mxoNONE,tmpName,&P,NULL,NULL);
      }

    if ((R->ClientName[0] == '\0') &&
        (R->P[0].CSKey != NULL))
      {
      /* RMCFG doesn't specify ClientName - set it to <SCHEDNAME> */

      MRMSetAttr(R,mrmaClient,(void **)R->Name,mdfString,mSet);
      }

    if (bmisset(&R->Flags,mrmfClient))
      {
      /* no local initialization required */

      continue;
      }

    /* handle failures of Moab peers intelligently */

    if (R->Type == mrmtMoab)
      {
      if (R->State == mrmsDisabled)
        {
        /* admin has disabled interface */

        /* do not re-enable */

        continue;
        }

      if (R->State == mrmsCorrupt)
        {
        /* NOTE:  Down -> no contact  Corrupt -> hung / bad data */
        /* NOTE:  for now we won't check for Downed RM's - usually the cost to query a downed RM is minimal */

        if ((R->State == mrmsCorrupt) && (MSched.Iteration <= 0))
          {
          /* NOTE:  do not retry corrupt interface on first iteration, rm connection may be hung */
          
          /* NOTE:  RestoreFailureCount block below will handle subsequent restore attempts */

          Failure = TRUE;

          continue;
          }

        if (R->RestoreFailureCount > 0)
          {
          /* previous restore attempt failed */

          mulong UpdateTime = R->StateMTime +
                              MAX(MSched.MaxRMPollInterval,
                                  MIN((MCONST_MINUTELEN << (R->RestoreFailureCount - 1)),MSched.RMRetryTimeCap));

          if (UpdateTime > MSched.Time)
            {
            /* exponential backoff prevents attempted RM restore at this time */

            Failure = TRUE;

             continue;
            }
          }
        }    /* END if ((R->State == mrmsNONE) || ...) */
      }      /* END if (R->Type == mrmtMoab) */

    if (MRMFunc[R->Type].JobStart != NULL)
      bmset(&R->Flags,mrmfExecutionServer);

    /* NOTE: we should move the below code into a separate routine (something like MRMDoLocalInit())
     * so we can use it in specific M***Initalize() routines */
 
    if (MRMFunc[R->Type].RMInitialize == NULL)
      {
      MDB(7,fRM) MLog("ALERT:    cannot initialize RM (RM '%s' does not support function '%s')\n",
        R->Name,
        MRMFuncType[mrmRMInitialize]);
 
      continue;
      }

    MRMStartFunc(R,NULL,mrmRMInitialize);
 
    MUThread(
      (int (*)(void))MRMFunc[R->Type].RMInitialize,
      R->P[0].Timeout,
      &rc,
      NULL,
      R,
      3,
      R,
      EMsgP,
      SCP);

    MRMEndFunc(R,NULL,mrmRMInitialize,NULL);
 
    if (rc != SUCCESS)
      {
      MDB(3,fRM) MLog("ALERT:    cannot initialize RM (RM '%s' failed in function '%s')\n",
        R->Name,
        MRMFuncType[mrmRMInitialize]);

      if (*SCP == mscNoError)
        *SCP = mscRemoteFailure;

      if (EMsgP[0] != '\0')
        {
        char tmpLine[MMAX_LINE];

        snprintf(tmpLine,sizeof(tmpLine),"cannot initialize interface - %s",
          EMsgP);

        MRMSetFailure(R,mrmRMInitialize,*SCP,tmpLine);
        }
      else if (rc == mscTimeout)
        {
        MRMSetFailure(R,mrmRMInitialize,*SCP,"initialization timeout");
        }
      else
        {
        MRMSetFailure(R,mrmRMInitialize,*SCP,"cannot initialize interface");
        }

      Failure = TRUE;

      if (R->Type == mrmtMoab)
        {
        /* keep track of the number of failures */

        R->RestoreFailureCount++;
        }                

      continue;
      }  /* END if (rc != SUCCESS) */

    R->LastInitFailureTime = 0;
    R->RMNI          = DEFAULT_MRMNETINTERFACE; 

    R->InitIteration = MSched.Iteration;
    R->FailIteration = -1;
    R->RestoreFailureCount = 0;

    if (R->MaxFailCount == 0)
      R->MaxFailCount = MMAX_RMFAILCOUNT;
    
    MRMSetState(R,mrmsActive);

    R->FirstContact = TRUE;
    }    /* END for (rmindex) */
 
  if (Failure == TRUE)
    {
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MRMInitialize() */
/* END MRMInitialize.c */ 
