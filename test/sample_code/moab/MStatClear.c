/* HEADER */

      
/**
 * @file MStatClear.c
 *
 * Contains: Statistic Clear functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 * Clear active and idle usage (AP and IP) stats for specified object types.
 *
 * NOTE: handle both single and multi-dimensional usage stats.
 *       clear eligible/idle job stats on MStat
 * NOTE: on mxoNONE or mxoALL, clear Hierarchical FS stats.
 * NOTE: if mlSystem set in PTypeBM, clear MStat.x and MPar[0] stats.
 *
 * @param OType (I) [mxoALL = ALL, mxoNONE = ALL but node, mxo* = *]
 * @param PTypeBM (I)    (bitmap of mlActive, mlIdle, and mlSystem) 
 * @param PStatOnly (I)
 * @param PCredTotal (I)
 */

int MStatClearUsage(

  enum MXMLOTypeEnum OType,
  mbool_t            Active,
  mbool_t            Idle,
  mbool_t            System,
  mbool_t            PStatOnly,
  mbool_t            PCredTotal)

  {
  int oindex;

  enum MXMLOTypeEnum OList[] = { 
    mxoUser, 
    mxoGroup, 
    mxoAcct, 
    mxoQOS, 
    mxoClass, 
    mxoPar, 
    mxoNode, 
    mxoSched,
    mxoxTJob,
    mxoNONE };

  const char *FName = "MStatClearUsage";

  MDB(3,fSTAT) MLog("%s(%s,%s,%s)\n",
    FName,
    MXO[OType],
    MBool[PStatOnly],
    MBool[PCredTotal]);

  if (PStatOnly != TRUE)
    {
    MStat.EligibleJobs = 0;
    MStat.IdleJobs     = 0;
    }

  for (oindex = 0;OList[oindex] != mxoNONE;oindex++)
    {
    /* (mxoALL = ALL, mxoNONE = ALL but node, mxo* = *) */

    if (OType != mxoALL)
      {
      if ((OType == mxoNONE) && (OList[oindex] == mxoNode))
        {
        continue;
        }

      if ((OType != mxoNONE) && (OList[oindex] != OType))
        {
        continue;
        }
      }

    MDB(4,fSTAT) MLog("INFO:     clearing usage stats for %s objects\n",
      MXO[OList[oindex]]);

    if ((OList[oindex] == mxoUser) ||
        (OList[oindex] == mxoGroup) ||
        (OList[oindex] == mxoAcct))
      {
      MStatClearGCredUsage(OList[oindex],Active,Idle,System,PCredTotal);
      }
    else
      {
      MStatClearOtherUsage(OList[oindex],Active,Idle,System,PCredTotal);
      }
    }    /* END for (oindex) */

  if ((MSched.FSTreeDepth > 0) &&
     ((OType == mxoNONE) || (OType == mxoALL)))
    {
    MFSTreeStatClearUsage(Active,Idle,PCredTotal);
    }

  MVCStatClearUsage(Active,Idle,PCredTotal);

  if (System == TRUE)
    {
    MStat.SuccessfulPH          = 0.0;
    MStat.TotalPHAvailable      = 0.0;
    MStat.TotalPHBusy           = 0.0;
    MStat.TotalPHDed            = 0.0;

    MStat.SuccessfulJobsCompleted = 0;
 
    memset(MStat.SMatrix,0,sizeof(MStat.SMatrix));
 
    MStat.MinEff          = 100.0;
    MStat.MinEffIteration = 0;
    }  /* END if (System == TRUE) */
 
  return(SUCCESS);
  }  /* END MStatClearUsage() */




/**
 * Call MStatClearObjectUsage() for each object of OType.
 *
 * NOTE: this routine only handles user, class, acct objects
 *
 * @param OType
 * @param PTypeBM
 * @param PCredTotal
 */

int MStatClearGCredUsage(

  enum MXMLOTypeEnum OType,
  mbool_t            Active,
  mbool_t            Idle,
  mbool_t            System,
  mbool_t            PCredTotal)

  {
  mhashiter_t HTIter;

  mhash_t    *HT = NULL;

  char  *NPtr;

  must_t *S;

  mcredl_t *L;

  mgcred_t *O;

  if (OType == mxoUser)
    {
    HT = &MUserHT;
    }
  else if (OType == mxoGroup)
    {
    HT = &MGroupHT;
    }
  else if (OType == mxoAcct)
    {
    HT = &MAcctHT;
    }

  if (HT == NULL)
    {
    return(FAILURE);
    }

  MUHTIterInit(&HTIter);

  while (MUHTIterate(HT,NULL,(void **)&O,NULL,&HTIter) == SUCCESS)
    {
    L    = &O->L;
    NPtr =  O->Name;
    S    = &O->Stat;

    MStatClearObjectUsage(L,S,NPtr,OType,PCredTotal,Active,Idle,System);
    }

  return(SUCCESS);
  }  /* END MStatClearGCredUsage() */



/**
 * Call MStatClearObjectUsage() for each object of OType.
 *
 * NOTE: this routine handles everything but mgcred_t objects (user, class, acct)
 *
 * @param OType
 * @param PTypeBM
 * @param PCredTotal
 */

int MStatClearOtherUsage(

  enum MXMLOTypeEnum OType,
  mbool_t            Active,
  mbool_t            Idle,
  mbool_t            System,
  mbool_t            PCredTotal)

  {
  int cindex;
  int MaxO = MSched.M[OType];

  char     *NPtr;

  must_t   *S;
  mcredl_t *L;

  /* clear usage on all creds of type OType */

  for (cindex = 0;cindex < MaxO;cindex++)
    {
    L    = NULL;
    S    = NULL;
    NPtr = NULL;

    switch (OType)
      {
      case mxoQOS:

        if (MQOS[cindex].Name[0] <= '\1')
          continue;

        L    = &MQOS[cindex].L;

        NPtr = MQOS[cindex].Name;
        S    = &MQOS[cindex].Stat;    

        break;

      case mxoClass:

        if (MClass[cindex].Name[0] <= '\1')
          continue;

        L    = &MClass[cindex].L;

        NPtr = MClass[cindex].Name;
        S    = &MClass[cindex].Stat;    
         
        break;

      case mxoPar:

        if (MPar[cindex].Name[0] <= '\1')
          continue;

        /* NOTE:  partition idle workload stats cleared in MSysScheduleProvisioning() */

        L    = &MPar[cindex].L;

        NPtr = MPar[cindex].Name;
        S    = &MPar[cindex].S;    

        break;

      case mxoNode:

        if ((MNode[cindex] == NULL) || (MNode[cindex]->Name[0] <= '\1'))
          continue;

        NPtr = MNode[cindex]->Name;
        /*AP   = &MNode[cindex]->AP;*/

        /* We only clear AP later, just do it here (different structs, too) */
        memset(MNode[cindex]->AP.Usage,0,sizeof(MNode[cindex]->AP.Usage));

        break;

      case mxoSched:

        L    = &MPar[0].L;

        NPtr = MPar[0].Name;

        /* NOTE:  do not pass 'S' (stats) at this point - do not know all side affects */

        break;

      case mxoxTJob:

        {
        mjob_t *TJ;

        if (MUArrayListGet(&MTJob,cindex) == NULL)
          continue;

        TJ = *(mjob_t **)(MUArrayListGet(&MTJob,cindex));

        if (TJ == NULL)
          continue;

        if (TJ->ExtensionData == NULL)
          continue;

        L = &((mtjobstat_t *)TJ->ExtensionData)->L;

        NPtr = TJ->Name;
        }

        break;

      case mxoUser:
      case mxoGroup:
      case mxoAcct:
      default:

        /* mgcred_t objects handled in MStatClearGCredUsage() */

        /* NO-OP */

        break;
      }  /* END switch (OType[oindex]) */

    MStatClearObjectUsage(L,S,NPtr,OType,PCredTotal,Active,Idle,System);
    }  /* END for (cindex) */

  return(SUCCESS);
  }  /* END MStatClearOtherUsage() */



/**
 * Clear the given structures Usage.
 *
 * @param L
 * @param S
 * @param Name
 * @param OType
 * @param PCredTotal
 * @param PTypeBM  (bitmap of mlActive, mlIdle, and mlSystem) 
 */

int MStatClearObjectUsage(

  mcredl_t          *L,
  must_t            *S,
  char              *Name,
  enum MXMLOTypeEnum OType,
  mbool_t            PCredTotal,
  mbool_t            Active,
  mbool_t            Idle,
  mbool_t            System)

  {
  mhash_t *APA = NULL;
  mhash_t *APU = NULL;
  mhash_t *APG = NULL;
  mhash_t *APC = NULL;
  mhash_t *APQ = NULL;

  mhash_t *IPU = NULL;
  mhash_t *IPQ = NULL;
  mhash_t *IPA = NULL;
  mhash_t *IPC = NULL;
  mhash_t *IPG = NULL;

  mpu_t *AP = NULL;
  mpu_t *IP = NULL;

  if (L != NULL)
    {
    AP  = &L->ActivePolicy;

    APA = L->AcctActivePolicy;
    APU = L->UserActivePolicy;
    APC = L->ClassActivePolicy;
    APG = L->GroupActivePolicy;
    APQ = L->QOSActivePolicy;

    IP  = L->IdlePolicy;

    IPU = L->UserIdlePolicy;
    IPG = L->GroupIdlePolicy;
    IPA = L->AcctIdlePolicy;
    IPC = L->ClassIdlePolicy;
    IPQ = L->QOSIdlePolicy;
    }  /* END if (L != NULL) */
    
  if ((Name == NULL) || (Name[0] == '\0') || (Name[0] == '\1'))
    return(SUCCESS);

  MDB(6,fSTAT) MLog("INFO:     clearing usage stats for %s %s - %s%s%s%s%s\n",
    MXO[OType],
    Name,
    (APA != NULL) ? " APA" : "",
    (APU != NULL) ? " APU" : "",
    (APC != NULL) ? " APC" : "",
    (APG != NULL) ? " APG" : "",
    (APQ != NULL) ? " APQ" : "");

  if ((L != NULL) && (PCredTotal == TRUE))
    {
    int pindex;

    for (pindex = 0; pindex < MMAX_PAR; pindex++)
      {
      if (L->TotalJobs[pindex] != 0)
        {
        MDB(7,fSTAT) MLog("INFO:     clearing total jobs usage stats (partition %s) for %s %s total jobs %d\n",
          MPar[pindex].Name,
          MXO[OType],
          Name,
          L->TotalJobs[pindex]);

        L->TotalJobs[pindex] = 0;
        }

      /* see if we only need to clear pindex 0 */

      if (MSched.PerPartitionScheduling != TRUE)
        break;
      }
    }

  if (Active == TRUE)
    {
    if (AP != NULL)
      {
      if (AP->GLimit != NULL)
        {
        mgpu_t *P = NULL;

        mhashiter_t HTIter;

        MUHTIterInit(&HTIter);

        while (MUHTIterate(AP->GLimit,NULL,(void **)&P,NULL,&HTIter) == SUCCESS)
          {
          MGPUClearUsage(P);
          }
        }

      MPUClearUsage(AP);
      }

    if (APA != NULL)
      {
      mpu_t *P = NULL;

      mhashiter_t HTIter;

      MUHTIterInit(&HTIter);

      while (MUHTIterate(APA,NULL,(void **)&P,NULL,&HTIter) == SUCCESS)
        {
        MPUClearUsage(P);
        }
      }

    if (APU != NULL)
      {
      mpu_t *P = NULL;

      mhashiter_t HTIter;

      MUHTIterInit(&HTIter);

      while (MUHTIterate(APU,NULL,(void **)&P,NULL,&HTIter) == SUCCESS)
        {
        MPUClearUsage(P);
        }
      }

    if (APC != NULL)
      {
      mpu_t *P = NULL;

      mhashiter_t HTIter;

      MUHTIterInit(&HTIter);

      while (MUHTIterate(APC,NULL,(void **)&P,NULL,&HTIter) == SUCCESS)
        {
        MPUClearUsage(P);
        }
      }

    if (APG != NULL)
      {
      mpu_t *P = NULL;

      mhashiter_t HTIter;

      MUHTIterInit(&HTIter);

      while (MUHTIterate(APG,NULL,(void **)&P,NULL,&HTIter) == SUCCESS)
        {
        MPUClearUsage(P);
        }
      }

    if (APQ != NULL)
      {
      mpu_t *P = NULL;

      mhashiter_t HTIter;

      MUHTIterInit(&HTIter);

      while (MUHTIterate(APQ,NULL,(void **)&P,NULL,&HTIter) == SUCCESS)
        {
        MPUClearUsage(P);
        }
      }
    }    /* END if (bmisset(PTypeBM,mlActive)) */

  if (Idle == TRUE)
    {
    if (IP != NULL)
      {
      if (IP->GLimit != NULL)
        {
        mgpu_t *P = NULL;

        mhashiter_t HTIter;

        MUHTIterInit(&HTIter);

        while (MUHTIterate(IP->GLimit,NULL,(void **)&P,NULL,&HTIter) == SUCCESS)
          {
          MGPUClearUsage(P);
          }
        }

      MPUClearUsage(IP);
      }

    if (IPU != NULL)
      {
      mpu_t *P = NULL;

      mhashiter_t HTIter;

      MUHTIterInit(&HTIter);

      while (MUHTIterate(IPU,NULL,(void **)&P,NULL,&HTIter) == SUCCESS)
        {
        MPUClearUsage(P);
        }
      }

    if (IPA != NULL)
      {
      mpu_t *P = NULL;

      mhashiter_t HTIter;

      MUHTIterInit(&HTIter);

      while (MUHTIterate(IPA,NULL,(void **)&P,NULL,&HTIter) == SUCCESS)
        {
        MPUClearUsage(P);
        }
      }

    if (IPG != NULL)
      {
      mpu_t *P = NULL;

      mhashiter_t HTIter;

      MUHTIterInit(&HTIter);

      while (MUHTIterate(IPG,NULL,(void **)&P,NULL,&HTIter) == SUCCESS)
        {
        MPUClearUsage(P);
        }
      }

    if (IPC != NULL)
      {
      mpu_t *P = NULL;

      mhashiter_t HTIter;

      MUHTIterInit(&HTIter);

      while (MUHTIterate(IPC,NULL,(void **)&P,NULL,&HTIter) == SUCCESS)
        {
        MPUClearUsage(P);
        }
      }

    if (IPQ != NULL)
      {
      mpu_t *P = NULL;

      mhashiter_t HTIter;

      MUHTIterInit(&HTIter);

      while (MUHTIterate(IPQ,NULL,(void **)&P,NULL,&HTIter) == SUCCESS)
        {
        MPUClearUsage(P);
        }
      }
    }  /* END if (bmisset(PTypeBM,mlActive)) */

  if ((S != NULL) && (System == TRUE))
    {
    memset(S,0,sizeof(must_t));

    MStatPInitialize((void *)S,FALSE,msoCred);
    }

  return(SUCCESS);
  }  /* END MStatClearObjectUsage() */

/* END MStatClear.c */
