/* HEADER */

/**
 * @file MQOS.c
 *
 * Moab QOS
 */
 
/* Contains:                                    *
 *  int MQOSShow(Buffer,BufSize,Flags,DFormat)  *
 *                                              */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-global.h"  
#include "moab-const.h"  


#define MPARM_QOSCFG "QOSCFG"

/* internal prototypes */

int __MQOSSetOverrideLimits(mqos_t *);    




/**
 * Search for a named QOS.
 *
 * If found, return success with Q pointing to QOS.
 * If not found, return failure with Q pointing to
 * first free QOS if available, Q set to NULL otherwise
 *
 * NOTE:  QOS's only added, never removed
 *
 * @param QName (I)
 * @param QP (O) [optional]
 */

int MQOSFind(

  const char  *QName,
  mqos_t     **QP)

  {

  int qindex;

  if (QP != NULL)
    *QP = NULL;

  if ((QName == NULL) || 
      (QName[0] == '\0'))
    {
    return(FAILURE);
    }

  /* index 0 not used */

  for (qindex = 1;qindex < MMAX_QOS;qindex++)
    {
    if (MQOS[qindex].Name[0] == '\0')
      {
      /* 'free' QOS slot found */

      if (QP != NULL)
        *QP = &MQOS[qindex];

      break;
      }

    if (strcmp(MQOS[qindex].Name,QName) != 0)
      continue;

    /* QOS found */

    if (QP != NULL)
      *QP = &MQOS[qindex];

    return(SUCCESS);
    }  /* END for (qindex) */

  return(FAILURE);
  }  /* END MQOSFind() */





/**
 * add QoS to MQOS table 
 *
 * @param QName (I)
 * @param QP (O) [optional]
 */

int MQOSAdd(

  const char  *QName,
  mqos_t     **QP)

  {
  int qindex;

  mqos_t *Q;

  const char *FName = "MQOSAdd";

  MDB(3,fCONFIG) MLog("%s(%s,QP)\n",
    FName,
    (QName != NULL) ? QName : "NULL");

  if ((QName == NULL) ||
      (QName[0] == '\0'))
    {
    return(FAILURE);
    }

  /* do not use QOS index 0 */

  for (qindex = 1;qindex < MMAX_QOS;qindex++)
    {
    Q = &MQOS[qindex];

    if (Q->Name[0] == '\0')
      {
      /* empty QOS slot located */

      MQOSInitialize(Q,QName);

      Q->Index = qindex;

      if (QP != NULL)
        *QP = Q;

      return(SUCCESS);
      }

    if (!strcmp(Q->Name,QName))
      {
      if (QP != NULL)
        *QP = Q;
  
      return(SUCCESS);
      }
    }    /* END for (qindex) */

  if (QP != NULL)
    *QP = NULL;  
 
  return(FAILURE);
  }  /* END MQOSAdd() */





/**
 *
 *
 * @param Q (I) [modified]
 * @param QName (I)
 */

int MQOSInitialize(

  mqos_t     *Q,
  const char *QName)

  {
  int index;

  const char *FName = "MQOSInitialize";

  MDB(4,fCONFIG) MLog("%s(Q,%s)\n",
    FName,
    (QName != NULL) ? QName : "NULL");

  if ((Q == NULL) || 
      (QName == NULL) || 
      (QName[0] == '\0'))
    {
    return(FAILURE);
    }

  memset(Q,0,sizeof(mqos_t));

  MUStrCpy(Q->Name,QName,sizeof(Q->Name));

  Q->QTTarget   = MDEF_TARGETQT;
  Q->XFTarget   = MDEF_TARGETXF;
 
  Q->QTSWeight  = MDEF_QOSQTWEIGHT;
  Q->XFSWeight  = MDEF_QOSXFWEIGHT;
 
  Q->F.Priority = MDEF_QOSPRIORITY;

  Q->DedResCost = MDEF_QOSDEDRESCOST;

  Q->SystemJobPriority = -1;

  Q->JobPrioAccrualPolicy = MDEF_QOSPRIOACCRUALPOLICY;

  MPUInitialize(&Q->L.ActivePolicy);

  if (Q->L.OverrideActivePolicy == NULL)
    {
    MPUCreate(&Q->L.OverrideActivePolicy);

    MPUInitializeOverride(Q->L.OverrideActivePolicy);
    }

  if (Q->L.OverrideIdlePolicy == NULL)
    {
    MPUCreate(&Q->L.OverrideIdlePolicy);

    MPUInitializeOverride(Q->L.OverrideIdlePolicy);
    }

  if (Q->L.OverriceJobPolicy == NULL)
    {
    MPUCreate(&Q->L.OverriceJobPolicy);

    MPUInitializeOverride(Q->L.OverriceJobPolicy);
    }

  Q->Stat.XLoad = (double *)MUCalloc(1,sizeof(double) * MSched.M[mxoxGMetric]);

  MCredAdjustConfig(mxoQOS,Q,FALSE);

  for (index = 1;index < MMAX_QOS;index++)
    {
    if (&MQOS[index] == Q)
      {
      Q->Index = index;
 
      break;
      }
    }  /* END for (index) */

  return(SUCCESS);
  }  /* END MQOSInitialize() */




/**
 * Get the configured default QOS order.
 *
 * @param QAL (I)
 * @param QP  (O)
 */

int MQOSGetQosOrder(

  mbitmap_t *QAL,
  mqos_t   **QP)

  {
  mqos_t *Q;
  int qindex;

  MASSERT(QAL != NULL, "Unexpected NULL QAL getting default QOS ORDER");
  MASSERT(QP != NULL, "Unexpected NULL QP getting default QOS ORDER");

  *QP = NULL;

  /* Locate the first QOS that's in the available bitmap */
  for (qindex = 0;qindex < MMAX_QOS;qindex++)
    {
    Q = MSched.QOSDefaultOrder[qindex];

    if (Q == NULL)
      break;

    if (bmisset(QAL,Q->Index))
      {
      *QP = Q;
      return(SUCCESS);
      }
    }

  return(FAILURE);
  }  /* END MQOSGetQOSORDER() */




/**
 * Only review QList and QDef credential attributes, ignore limits, resource requests, etc.
 *
 * @param J     (I)
 * @param QReq  (I) [optional - NULL if no request made]
 * @param P     (I) [optional - NULL if no request made]
 * @param QAL   (O) [BM/optional] QOS Access List
 * @param QDefP (O) [optional] Default QOS
 */

int MQOSGetAccess(
 
  mjob_t        *J,
  mqos_t        *QReq,
  mpar_t        *P,
  mbitmap_t     *QAL,
  mqos_t       **QDefP)
 
  {
  mbitmap_t tmpQAL;
 
  mbool_t AndMask;

  int  qindex;
  int  tindex = 0;

  mfst_t *TData = NULL;

  mclass_t *C = NULL;

  const char *FName = "MQOSGetAccess";
 
  MDB(7,fSTRUCT) MLog("%s(%s,%s,QAL,%s)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (QReq != NULL) ? QReq->Name : "NULL",
    (QDefP != NULL) ? "QDefP" : "NULL");
 
  if (J == NULL) 
    {
    MDB(7,fSTRUCT) MLog("WARNING:  invalid job pointer detected\n");

    return(FAILURE);
    }

  /* NOTE:  job user cred may not be specified for template jobs */

  if (J->Credential.C != NULL)
    {
    C = J->Credential.C;
    }
 
  AndMask = FALSE;
 
  /* determine level 1 (direct access) QAL */

  bmcopy(&tmpQAL,&MPar[0].F.QAL); 
 
  if (MPar[0].F.QALType == malAND)
    {
    AndMask = TRUE;
    }
  else
    {
    /* obtain 'or' list */

    if (J->Credential.U != NULL)
      { 
      if ((J->Credential.U->F.QALType == malOR) &&
          (!bmisclear(&J->Credential.U->F.QAL)))
        {
        bmor(&tmpQAL,&J->Credential.U->F.QAL); 
        }
      else if ((MSched.DefaultU != NULL) &&
               (MSched.DefaultU->F.QALType == malOR))
        {
        bmor(&tmpQAL,&MSched.DefaultU->F.QAL);
        }
      }

    if (J->Credential.G != NULL)
      {
      if ((J->Credential.G->F.QALType == malOR) &&
          (!bmisclear(&J->Credential.G->F.QAL)))
        {
        bmor(&tmpQAL,&J->Credential.G->F.QAL);
        }
      else if ((MSched.DefaultG != NULL) &&
               (MSched.DefaultG->F.QALType == malOR))
        {
        bmor(&tmpQAL,&MSched.DefaultG->F.QAL);
        }
      }

    if (J->Credential.A != NULL)
      {
      if ((J->Credential.A->F.QALType == malOR) &&
          (!bmisclear(&J->Credential.A->F.QAL)))
        {
        bmor(&tmpQAL,&J->Credential.A->F.QAL);
        }
      else if ((MSched.DefaultA != NULL) &&
               (MSched.DefaultA->F.QALType == malOR))
        {
        bmor(&tmpQAL,&MSched.DefaultA->F.QAL);
        }
      }

    if (C != NULL)
      {
      if ((C->F.QALType == malOR) &&
          (!bmisclear(&C->F.QAL)))
        {
        bmor(&tmpQAL,&C->F.QAL);
        }
      else if ((MSched.DefaultC != NULL) &&
               (MSched.DefaultC->F.QALType == malOR))
        {
        bmor(&tmpQAL,&MSched.DefaultC->F.QAL);
        }
      }

    tindex = 0;

    while ((J->FairShareTree != NULL) && (tindex < MMAX_PAR))
      {
      if ((J->FairShareTree[tindex] != NULL) &&
          (J->FairShareTree[tindex]->TData != NULL))
        {
        /* use J->CPAL if it is set, otherwise use J->PAL */

        if (!bmisclear(&J->CurrentPAL))
          {
          if (!bmisset(&J->SpecPAL,tindex))
            {
            tindex++;
    
            continue;
            }

          TData = J->FairShareTree[tindex]->TData;

          break;
          }
        else if (bmisset(&J->PAL,tindex) == FALSE)
          {
          tindex++;

          continue;
          }

        /* first valid tree found */

        TData = J->FairShareTree[tindex]->TData;

        break;
        }

      tindex++;
      }  /* END while ((J->FSTree != NULL) && ...) */

    if ((TData != NULL) &&
        (TData->F->QALType == malOR) &&
        (!bmisclear(&TData->F->QAL)))
      {
      MDB(7,fSTRUCT) MLog("INFO:     getting QOS access from FS tree\n");

      bmor(&tmpQAL,&TData->F->QAL);
      }
    }  /* END else (Policy[0].F.QALType == malAND) */
 
  /* obtain 'exclusive' list */ 

  if ((TData != NULL) && (TData->F->QALType == malAND))
    {
    if (AndMask != TRUE)
      {
      bmcopy(&tmpQAL,&TData->F->QAL);
 
      AndMask = TRUE;
      }
    else
      {
      bmand(&tmpQAL,&TData->F->QAL);
      }
    }

  if ((C != NULL) && (C->F.QALType == malAND))
    {
    if (AndMask != TRUE)
      {
      bmcopy(&tmpQAL,&C->F.QAL);
 
      AndMask = TRUE;
      }
    else
      {
      bmand(&tmpQAL,&C->F.QAL);
      }
    }
 
  if ((J->Credential.A != NULL) && (J->Credential.A->F.QALType == malAND))
    {
    if (AndMask != TRUE)
      {
      bmcopy(&tmpQAL,&J->Credential.A->F.QAL);    
 
      AndMask = TRUE;
      }
    else
      {
      bmand(&tmpQAL,&J->Credential.A->F.QAL);     
      }
    }
 
  if ((J->Credential.G != NULL) && (J->Credential.G->F.QALType == malAND))
    {
    if (AndMask != TRUE)
      {
      bmcopy(&tmpQAL,&J->Credential.G->F.QAL);    
 
      AndMask = TRUE;
      }
    else
      {
      bmand(&tmpQAL,&J->Credential.G->F.QAL);      
      }
    }
 
  if ((J->Credential.U != NULL) && (J->Credential.U->F.QALType == malAND))
    {
    if (AndMask != TRUE)
      {
      bmcopy(&tmpQAL,&J->Credential.U->F.QAL);    
 
      AndMask = TRUE;
      }
    else
      {
      bmand(&tmpQAL,&J->Credential.U->F.QAL);     
      }
    } 
 
  if ((J->Credential.U != NULL) && (J->Credential.U->F.QALType == malOnly))
    {
    bmcopy(&tmpQAL,&J->Credential.U->F.QAL);    
    }
  else if ((J->Credential.G != NULL) && (J->Credential.G->F.QALType == malOnly))
    {
    bmcopy(&tmpQAL,&J->Credential.G->F.QAL);       
    }
  else if ((J->Credential.A != NULL) && (J->Credential.A->F.QALType == malOnly))
    {
    bmcopy(&tmpQAL,&J->Credential.A->F.QAL);       
    }
  else if ((C != NULL) && (C->F.QALType == malOnly))
    {
    bmcopy(&tmpQAL,&C->F.QAL);
    }
  else if ((TData != NULL) && (TData->F->QALType == malOnly))
    {
    bmcopy(&tmpQAL,&TData->F->QAL);
    }

  /* NOTE:  level 2 eval should not be conditional */

  if ((QReq != NULL) && !bmisset(&tmpQAL,QReq->Index))
    {
    /* level one access inadequate, attempt level two */

    /* NOTE:  incomplete, assume only account based level 2 */

    if ((J->Credential.U != NULL) && (J->Credential.U->F.AAL != NULL))
      {
      mgcred_t *A;
      mln_t    *L;

      /* walk U->AAL */

      for (L = J->Credential.U->F.AAL;L != NULL;L = L->Next)
        {
        A = (mgcred_t *)L->Ptr;

        if (A == NULL)
          continue;

        if ((A->F.QALType == malOR) &&
            (!bmisclear(&A->F.QAL)))
          {
          bmor(&tmpQAL,&A->F.QAL);
          }
        }
      }    /* END if ((J->Cred.U != NULL) && (J->Cred.U->AAL != NULL)) */
    }      /* END if (!bmisset(&tmpQAL,QReq->Index)) */
 
  if (QAL != NULL)
    bmcopy(QAL,&tmpQAL);       
 
  /* determine allowed QOS default (precedence: U,G,A,C,S,0) */
 
  if (QDefP != NULL)
    {
    mqos_t *AQD = NULL;
    mqos_t *CQD = NULL;
    mqos_t *FQD = NULL;

    *QDefP = NULL;

    /* check order:
         explicitly specified QOSORDER in config file
         specified in fairshare tree if per partition scheduling 
         explicitly specified cred QDef
         implicit cred QDef 
         explicitly specified default cred QDef
     */
       
    if (MQOSGetQosOrder(
          (TData != NULL) ? &TData->F->QAL : &tmpQAL, /* FS QAL takes precedence over global QAL. */
          QDefP) == SUCCESS)
      {
        /* Got a QOS from QOSORDER in the config file */
      }
    else if ((MSched.PerPartitionScheduling == TRUE) &&
        (J->QOSRequested == NULL) &&
        (MFSFindQDef(J,P,&FQD) == SUCCESS) &&
        /* BMCheck(FQD->Index,tmpQAL)*/ TRUE)
      {
      *QDefP = FQD;
      }
    else if ((J->Credential.U != NULL) &&
        (J->Credential.U->F.QDef[0] != NULL) &&
        (J->Credential.U->F.QDef[0] != &MQOS[0]) &&
         bmisset(&J->Credential.U->F.Flags,mffQDefSpecified) &&
         bmisset(&tmpQAL,J->Credential.U->F.QDef[0]->Index))
      {
      *QDefP = J->Credential.U->F.QDef[0];
      }
    else if ((J->Credential.G != NULL) &&
             (J->Credential.G->F.QDef[0] != NULL) &&
             (J->Credential.G->F.QDef[0] != &MQOS[0]) &&
              bmisset(&J->Credential.G->F.Flags,mffQDefSpecified) &&
              bmisset(&tmpQAL,J->Credential.G->F.QDef[0]->Index))
      {
      *QDefP = J->Credential.G->F.QDef[0];
      }
    else if ((J->Credential.A != NULL) &&
             (J->Credential.A->F.QDef[0] != NULL) &&
             (J->Credential.A->F.QDef[0] != &MQOS[0]) &&
              bmisset(&J->Credential.A->F.Flags,mffQDefSpecified) &&
              bmisset(&tmpQAL,J->Credential.A->F.QDef[0]->Index))
      {
      *QDefP = J->Credential.A->F.QDef[0];
      }
    else if ((C != NULL) &&
             (C->F.QDef[0] != NULL) &&
             (C->F.QDef[0] != &MQOS[0]) &&
              bmisset(&C->F.Flags,mffQDefSpecified) &&
              bmisset(&tmpQAL,C->F.QDef[0]->Index))
      {
      *QDefP = C->F.QDef[0];
      }
    else if ((J->Credential.U != NULL) &&
        (J->Credential.U->F.QDef[0] != NULL) && 
        (J->Credential.U->F.QDef[0] != &MQOS[0]) &&
         bmisset(&tmpQAL,J->Credential.U->F.QDef[0]->Index))
      {
      *QDefP = J->Credential.U->F.QDef[0];
      }
    else if ((J->Credential.G != NULL) &&
             (J->Credential.G->F.QDef[0] != NULL) &&
             (J->Credential.G->F.QDef[0] != &MQOS[0]) &&     
              bmisset(&tmpQAL,J->Credential.G->F.QDef[0]->Index))
      {
      *QDefP = J->Credential.G->F.QDef[0];
      }
    else if ((J->Credential.A != NULL) &&
             (J->Credential.A->F.QDef[0] != NULL) &&
             (J->Credential.A->F.QDef[0] != &MQOS[0]) &&       
              bmisset(&tmpQAL,J->Credential.A->F.QDef[0]->Index))
      {
      *QDefP = J->Credential.A->F.QDef[0];
      }
    else if ((C != NULL) &&
             (C->F.QDef[0] != NULL) &&
             (C->F.QDef[0] != &MQOS[0]) &&
              bmisset(&tmpQAL,C->F.QDef[0]->Index))
      {
      *QDefP = C->F.QDef[0];
      }
    else if ((MSched.DefaultU != NULL) &&
             (MSched.DefaultU->F.QDef[0] != NULL) &&
             (MSched.DefaultU->F.QDef[0] != &MQOS[0]) &&
              bmisset(&tmpQAL,MSched.DefaultU->F.QDef[0]->Index))
      {
      *QDefP = MSched.DefaultU->F.QDef[0];
      }
    else if ((MSched.DefaultG != NULL) &&
             (MSched.DefaultG->F.QDef[0] != NULL) &&
             (MSched.DefaultG->F.QDef[0] != &MQOS[0]) &&
              bmisset(&tmpQAL,MSched.DefaultG->F.QDef[0]->Index))
      {
      *QDefP = MSched.DefaultG->F.QDef[0];
      }
    else if ((MSched.DefaultA != NULL) &&
             (MSched.DefaultA->F.QDef[0] != NULL) &&
             (MSched.DefaultA->F.QDef[0] != &MQOS[0]) &&
              bmisset(&tmpQAL,MSched.DefaultA->F.QDef[0]->Index))
      {
      *QDefP = MSched.DefaultA->F.QDef[0];
      }
    else if ((MSched.DefaultC != NULL) &&
             (MSched.DefaultC->F.QDef[0] != NULL) &&
             (MSched.DefaultC->F.QDef[0] != &MQOS[0]) &&
              bmisset(&tmpQAL,MSched.DefaultC->F.QDef[0]->Index))
      {
      *QDefP = MSched.DefaultC->F.QDef[0];
      }
    else if ((MPar[0].F.QDef[0] != NULL) &&
             (MPar[0].F.QDef[0] != &MQOS[0]))
      {
      *QDefP = MPar[0].F.QDef[0];
      }
    else if ((TData != NULL) &&
             !bmisclear(&TData->F->QAL))
      {
      *QDefP = TData->F->QDef[0];
      }

    if (*QDefP == NULL)
      {
      /* NOTE:  new change in Moab 4.0.4p6, SYSDEF set if no qos specified OR if
                default qos access not available */

      *QDefP = &MQOS[MDEF_SYSQDEF];
      }
 
    /* verify access to default QOS */
 
    if ((*QDefP != NULL) && !bmisset(&tmpQAL,(*QDefP)->Index))
      {
      *QDefP = &MQOS[MDEF_SYSQDEF];  
 
      /* locate first legal QOS */
 
      for (qindex = 1;qindex < MMAX_QOS;qindex++)
        {
        if (bmisset(&tmpQAL,qindex))
          {
          *QDefP = &MQOS[qindex];
 
          break;
          }
        }    /* END for (qindex) */
      }      /* END if (!bmisset(&tmpQAL,(*QDefP)->Index)) */

    if (*QDefP == &MQOS[0])
      {
      /* NOTE:  do not allow jobs to access the 'DEFAULT' QOS template located at MQOS[0] */

      *QDefP = NULL;
      }

    if ((J->Credential.A != NULL) && (J->Credential.A->F.QDef[0] != NULL))
      AQD = (mqos_t *)J->Credential.A->F.QDef[0];
 
    if ((C != NULL) && (C->F.QDef[0] != NULL))
      CQD = (mqos_t *)C->F.QDef[0];
 
    MDB(3,fSTRUCT) MLog("INFO:     default QOS for job %s set to %s(%d) (P:%s,U:%s,G:%s,A:%s,C:%s)\n",
      J->Name,
      (*QDefP != NULL) ? (*QDefP)->Name : "NULL",
      (*QDefP != NULL) ? (*QDefP)->Index : -1,
      (MPar[0].F.QDef[0] != NULL)  ? 
        ((mqos_t *)MPar[0].F.QDef[0])->Name : 
        NONE,
      ((J->Credential.U != NULL) && (J->Credential.U->F.QDef[0] != NULL)) ? 
        ((mqos_t *)J->Credential.U->F.QDef[0])->Name : 
        NONE,
      ((J->Credential.G != NULL) && (J->Credential.G->F.QDef[0] != NULL)) ? 
        ((mqos_t *)J->Credential.G->F.QDef[0])->Name : 
        NONE,
      (AQD != NULL) ? AQD->Name : NONE,
      (CQD != NULL) ? CQD->Name : NONE);
    }  /* END if (QDefP != NULL) */
 
  if (QReq != NULL)
    {
    if (!bmisset(&tmpQAL,QReq->Index))
      {
      MDB(2,fSTRUCT) MLog("WARNING:  job %s cannot access requested QOS %s\n",
        J->Name,
        QReq->Name);
 
      return(FAILURE);
      }
    } 
 
  bmclear(&tmpQAL);
  return(SUCCESS);
  }  /* END MQOSGetAccess() */





/**
 *
 *
 * @param QOSString (I)
 * @param BM (O)
 * @param FQP (O) first qos referenced in list [optional]
 * @param Mode (I) [add unknown QOS's to list]
 */

int MQOSListBMFromString(
 
  char                    *QOSString, /* I */
  mbitmap_t               *BM,        /* O */
  mqos_t                 **FQP,       /* O (optional) first qos referenced in list */
  enum MObjectSetModeEnum  Mode)      /* I (add unknown QOS's to list) */
 
  {
  int   rangestart;
  int   rangeend;
 
  int   rindex;
 
  char  tmpLine[MMAX_LINE];
 
  char *rtok;
  char *tail;

  char  tmpName[MMAX_NAME];
 
  char *TokPtr;

  mqos_t *Q;

  if (FQP != NULL)
    *FQP = NULL;
 
  if (BM == NULL)
    {
    return(FAILURE);
    }

  bmclear(BM); 
 
  if (QOSString == NULL)
    {
    return(SUCCESS);
    }
 
  MUStrCpy(tmpLine,QOSString,sizeof(tmpLine));
 
  /* FORMAT:  QOSSTRING:   <RANGE>[:<RANGE>]... */
  /*          RANGE:       <VALUE>[-<VALUE>]    */
 
  /* NOTE:    The following non-numeric values may appear in the string */
  /*          an should be handled: '&', '^'                            */
 
  rtok = MUStrTok(tmpLine,",:",&TokPtr);
 
  while (rtok != NULL)
    {
    while ((*rtok == '&') || (*rtok == '^'))
      rtok++;
 
    rangestart = strtol(rtok,&tail,10);

    if ((rangestart != 0) || (rtok[0] == '0'))
      { 
      if (*tail == '-')
        rangeend = strtol(tail + 1,&tail,10);
      else
        rangeend = rangestart;

      rangeend = MIN(rangeend,31);
 
      for (rindex = rangestart;rindex <= rangeend;rindex++)
        {
        sprintf(tmpName,"%d",
           rindex);

        if (MQOSFind(tmpName,&Q) == SUCCESS)
          {
          bmset(BM,Q->Index);
          }
        else if ((Mode == mAdd) && (Q != NULL))
          {
          MQOSAdd(tmpName,&Q);

          bmset(BM,Q->Index); 
          }

        if ((FQP != NULL) && (*FQP == NULL) && (Q != NULL))
          *FQP = Q;
        }    /* END for (rindex) */
      }
    else
      {
      /* QOS name provided */

      /* remove meta characters (handled externally) */

      if ((tail = strchr(rtok,'&')) != NULL)
        *tail = '\0';
      else if ((tail = strchr(rtok,'^')) != NULL)
        *tail = '\0';

      if (strcmp(rtok,DEFAULT) && strcmp(rtok,"DEFAULT"))
        {
        /* do not allow assignment to default QOS */

        if (MQOSFind(rtok,&Q) == SUCCESS)
          {
          bmset(BM,Q->Index);  
          }
        else if ((Mode == mAdd) && (Q != NULL))
          {
          MQOSAdd(rtok,&Q);

          bmset(BM,Q->Index);  
          }

        if ((FQP != NULL) && (*FQP == NULL) && (Q != NULL))
          *FQP = Q;
        }
      }    /* END else ((rangestart != 0) || (rtok[0] == '0')) */
       
    rtok = MUStrTok(NULL,",:",&TokPtr);
    }  /* END while (rtok) */
 
  return(SUCCESS);
  }  /* END MQOSListBMFromString() */




/**
 *
 *
 * @param BM (I)
 * @param Delim (I) [optional]
 * @param String (O)
 */

char *MQOSBMToString(
 
  mbitmap_t *BM,     /* I */
  char      *String) /* O */

  {
  char *BPtr = NULL;
  int   BSpace;

  int   bindex;

  mqos_t *Q;

  if (String != NULL)
    String[0] = '\0';

  if ((BM == NULL) || (String == NULL))
    {
    return(String);
    }

  MUSNInit(&BPtr,&BSpace,String,MMAX_LINE);

  for (bindex = 1;bindex < MMAX_QOS;bindex++)
    {
    if (!bmisset(BM,bindex))
      continue;

    Q = &MQOS[bindex];

    if (Q->Name[0] == '\1')
      continue;

    if (Q->Name[0] == '\0')
      break;

    if (!MUStrIsEmpty(String))
      {
      MUSNPrintF(&BPtr,&BSpace,",%s",
        Q->Name);
      }
    else
      {
      MUSNCat(&BPtr,&BSpace,Q->Name);
      }
    }  /* END for (bindex) */
 
  return(String);
  }  /* END MQOSBMToString() */





/**
 * Report QoS diagnostics and state
 *
 * handles "mdiag -q"
 *
 * @see MUIDiagnose() - parent
 *
 * @param Auth   (I) (FIXME: not used but should be)
 * @param QName  (I) [optional]
 * @param Buffer (O)
 * @param FlagBM (I) bitmap of enum MCModeEnum
 * @param Format (I)
 */

int MQOSShow(

  char                *Auth,
  char                *QName,
  mstring_t           *Buffer,
  mbitmap_t           *FlagBM,
  enum MFormatModeEnum Format)
 
  {
  mstring_t JobFlags(MMAX_LINE);

  int qindex;
 
  int index;
  int aindex;
 
  mqos_t   *Q;
  mgcred_t *U;
  mgcred_t *G;
  mgcred_t *A;
  mclass_t *C;
 
  char      QALChar;
  char      QALLine[MMAX_LINE];     
  char      Limits[MMAX_LINE];

  mhashiter_t Iter;

  const enum MQOSAttrEnum QList[] = {
    mqaDedResCost,
    mqaFSScalingFactor,
    mqaUtlResCost,
    mqaBLPowerThreshold,
    mqaQTPowerThreshold,
    mqaXFPowerThreshold,
    mqaBLPreemptThreshold,
    mqaQTPreemptThreshold,
    mqaXFPreemptThreshold,
    mqaBLACLThreshold,
    mqaQTACLThreshold,
    mqaXFACLThreshold,
    mqaBLRsvThreshold,
    mqaQTRsvThreshold,
    mqaXFRsvThreshold,
    mqaBLTriggerThreshold,
    mqaQTTriggerThreshold,
    mqaXFTriggerThreshold,
    mqaGreenBacklogThreshold,
    mqaGreenQTThreshold,
    mqaGreenXFactorThreshold,
    mqaOnDemand,
    mqaPreemptMaxTime,
    mqaPreemptMinTime,
    mqaBacklog,
    mqaDynAttrs,
    mqaSpecAttrs,
    mqaRsvTimeDepth,
    mqaNONE };

  const enum MCredAttrEnum CList[] = { 
    mcaPList,
    mcaHold,
    mcaNONE };

  const char *FName = "MQOSShow";
 
  MDB(2,fUI) MLog("%s(QName,SBuffer,SBufSize,%ld)\n",
    FName,
    FlagBM);

  if (Buffer == NULL)
    {
    return(FAILURE);
    }

  /* do not initialize string, append to end */

  if (Format == mfmXML)
    {
    mstring_t QLine(MMAX_LINE);

    mxml_t *DE = NULL;

    MXMLCreateE(&DE,(char*)MSON[msonData]);

    for (qindex = 0;qindex < MMAX_QOS;qindex++)
      {
      Q = &MQOS[qindex];

      if (Q->Name[0] == '\0')
        break;

      if ((QName != NULL) && 
          (QName[0] != '\0') && 
          strcmp(QName,Q->Name))
        continue;

      if (!strcmp(Q->Name,ALL))
        continue;

      MUIShowCConfig(Q->Name,mxoQOS,DE,NULL,NULL);
      } /* END for (qindex) */

    MXMLToMString(DE,&QLine,NULL,TRUE);

    MStringAppendF(Buffer,"%s",QLine.c_str());

    MXMLDestroyE(&DE);

    return(SUCCESS);
    }

  MStringAppendF(Buffer,"QOS Status\n\n");
 
  MQOSBMToString(&MPar[0].F.QAL,QALLine);
 
  MJobFlagsToMString(NULL,&MPar[0].F.JobFlags,&JobFlags);

  MStringAppendF(Buffer,"System QOS Settings:  QList: %s (Def: %s)  Flags: %s\n\n",
    (QALLine[0] != '\0') ? QALLine : "---",
    ((mqos_t *)MPar[0].F.QDef[0])->Name,
    (JobFlags.empty()) ? "-" : JobFlags.c_str());
 
  MStringAppendF(Buffer,"%-20s%c %8s %8s %8s %8s %8s %10s %12s %s\n\n",
    "Name",
    '*',
    "Priority",
    "QTWeight",
    "QTTarget",
    "XFWeight", 
    "XFTarget",
    "QFlags",
    "JobFlags",
    "Limits");

  /* NOTE:  report DEFAULT qos located at index 0 */
 
  mstring_t JFlags(MMAX_LINE);
  mstring_t String(MMAX_LINE);

  for (qindex = 0;qindex < MMAX_QOS;qindex++)
    {
    Q = &MQOS[qindex];

    if (Q->Name[0] == '\0')
      break;

    if ((QName != NULL) && 
        (QName[0] != '\0') && 
        strcmp(QName,Q->Name))
      continue;

    if (!strcmp(Q->Name,ALL))
      continue;

    JFlags.clear();
    String.clear();

    MQOSFlagsToMString(Q,&String,1); 
 
    if (Q->F.QALType == malAND)
      QALChar = '&';
    else if (Q->F.QALType == malOnly)
      QALChar = '^';
    else
      QALChar = ' ';

    MCredShowAttrs(
      &Q->L,
      &Q->L.ActivePolicy,
      Q->L.IdlePolicy,
      Q->L.OverrideActivePolicy,
      Q->L.OverrideIdlePolicy,
      Q->L.OverriceJobPolicy,
      Q->L.RsvPolicy,
      Q->L.RsvActivePolicy,
      &Q->F,
      0,
      FALSE,
      TRUE,
      Limits);

    MJobFlagsToMString(NULL,&Q->F.JobFlags,&JFlags);

    MStringAppendF(Buffer,"%-20s%c %8ld %8ld %8ld %8ld %8.2f %10s %12s %s\n",
      (Q->Name[0] != '\0') ? Q->Name : "-",
      QALChar,
      Q->F.Priority,
      Q->QTSWeight,
      Q->QTTarget,
      Q->XFSWeight,
      Q->XFTarget,
      (!String.empty()) ? String.c_str() : "-",
      (!JFlags.empty()) ? JFlags.c_str() : "-",
      (MUStrIsEmpty(Limits)) ? "-" : Limits);

    /* we're printing the FSCWeight (fsweight) if it's set */

    if (Q->FSCWeight != 0)
      {
      MStringAppendF(Buffer,"  FSWeight=%ld\n", 
        Q->FSCWeight);
      }

    for (aindex = 0;QList[aindex] != mqaNONE;aindex++)
      {
      String.clear();  /* Reset strring */

      MQOSAToMString(Q,QList[aindex],&String,0);

      if (MUStrIsEmpty(String.c_str()))
        continue;

      MStringAppendF(Buffer,"  %s=%s\n",
        MQOSAttr[QList[aindex]],
        String.c_str());
      }  /* END for (aindex) */

    for (aindex = 0;CList[aindex] != mcaNONE;aindex++)
      {
      String.clear();  /* Reset strring */

      MCredAToMString((void *)Q,mxoQOS,CList[aindex],&String,mfmNONE,0);

      if (MUStrIsEmpty(String.c_str()))
        continue;

      MStringAppendF(Buffer,"  %s=%s\n",
        MCredAttr[CList[aindex]],
        String.c_str());
      }  /* END for (aindex) */

    if (Q->F.ReqRsv != NULL)
      {
      MStringAppendF(Buffer,"  NOTE:  qos %s is tied to rsv '%s'\n",
        Q->Name,
        Q->F.ReqRsv);
      }

    String.clear();  /* Reset strring */

    /* check and display user access */

    MUHTIterInit(&Iter);
    while (MUHTIterate(&MUserHT,NULL,(void **)&U,NULL,&Iter) == SUCCESS)
      {
      if (bmisset(&U->F.QAL,qindex))
        {
        MStringAppendF(&String,"%s%s%s",
          (!String.empty()) ? "," : "",
          U->Name,
          MALType[U->F.QALType]);
        }
      }  /* END for (index) */
 
    if (!String.empty()) 
       {
       MStringAppendF(Buffer,"  Users:    %s\n",
         String.c_str());
       }
 
    /* display group access */
 
    String.clear();  /* Reset strring */

    MUHTIterInit(&Iter);
    while (MUHTIterate(&MGroupHT,NULL,(void **)&G,NULL,&Iter) == SUCCESS)
      {
      if (bmisset(&G->F.QAL,qindex))
        {
        MStringAppendF(&String,"%s%s%s",
          (!String.empty()) ? "," : "",
          G->Name,
          MALType[G->F.QALType]);
        }
      }  /* END for (index) */
 
    if (!String.empty())
      {
      MStringAppendF(Buffer,"  Groups:   %s\n",
        String.c_str());
      }
 
    /* display account access */
 
    String.clear();  /* Reset strring */
 
    MUHTIterInit(&Iter);
    while (MUHTIterate(&MAcctHT,NULL,(void **)&A,NULL,&Iter) == SUCCESS)
      {
      if (bmisset(&A->F.QAL,qindex))
        {
        MStringAppendF(&String,"%s%s%s",
          (!String.empty()) ? "," : "",
          A->Name,
          MALType[A->F.QALType]);
        }
      }  /* END for (index) */
 
    if (!String.empty())
      {
      MStringAppendF(Buffer,"  Accounts: %s\n",
        String.c_str());
      }

    /* display class access */

    String.clear();  /* Reset strring */

    for (index = MCLASS_FIRST_INDEX;index < MMAX_CLASS;index++)
      {
      C = &MClass[index];

      if ((C->Name[0] == '\0') || (C->Name[0] == '\1'))
        continue;

      if (bmisset(&C->F.QAL,qindex))
        {
        MStringAppendF(&String,"%s%s%s",
          (!String.empty()) ? "," : "",
          C->Name,
          MALType[C->F.QALType]);
        }
      }  /* END for (index) */

    if (!String.empty())
      {
      MStringAppendF(Buffer,"  Classes: %s\n",
        String.c_str());
      }

    if (Q->Variables != NULL)
      {
      /* display variables */

      MStringAppendF(Buffer,"  VARIABLE=%s\n",
        Q->Variables->Name);
      MStringAppend(Buffer,"\n");
      }

    if (Q->MB != NULL)
      {
      char *ptr = NULL;

      /* display messages */

      ptr = MMBPrintMessages(
        Q->MB,
        mfmHuman,
        FALSE,
        -1,
        NULL,
        0);

      MStringAppendF(Buffer,"%s",ptr);
      }

    if (bmisset(FlagBM,mcmVerbose))
      {
      char TString[MMAX_LINE];
      /* report QoS performance */

      MULToTString(Q->MaxQT,TString);

      MStringAppendF(Buffer,"Performance:  MaxQT:  %s    MaxXF:  %.2f  Backlog:  %.2f proc-seconds\n",
        TString,
        Q->MaxXF,
        (Q->L.IdlePolicy != NULL) ? (double)Q->L.IdlePolicy->Usage[mptMaxPS][0] : 0.0);
      }
    }    /* END for (qindex) */
 
  return(SUCCESS);
  }  /* END MQOSShow() */





/**
 * Report QoS configuration
 *
 * @see MUIConfigShow() - parent
 * @see MCredConfigShow() - child
 *
 * @param Q (I)
 * @param VFlag (I)
 * @param PIndex (I)
 * @param String (O)
 */

int MQOSConfigShow(

  mqos_t    *Q,
  int        VFlag,
  int        PIndex,
  mstring_t *String)

  {
  if ((Q == NULL) || (String == NULL))
    {
    return(FAILURE);
    }

  /* general credential config */

  MStringSet(String,"\0");

  if ((Q->Name[0] != '\0') && (Q->Name[1] != '\1'))
      MCredConfigShow((void *)Q,mxoQOS,NULL,VFlag,PIndex,String);

  /* QOS specific config */

  /* NYI */

  return(SUCCESS);
  }  /* END MQOSConfigShow() */





/**
 *
 *
 * @param Q (I)
 */

int __MQOSSetOverrideLimits(
 
  mqos_t *Q) /* I */
 
  {
  int index;
 
  if (Q == NULL)
    {
    return(FAILURE);
    }
 
  for (index = 0;MQOSFlags[index] != NULL;index++)
    {
    if (!bmisset(&Q->Flags,index))
      continue;
 
    switch (index)
      {
      case mqfignJobPerUser:
 
        Q->L.OverriceJobPolicy->SLimit[mptMaxJob][0] = -1;
        Q->L.OverriceJobPolicy->HLimit[mptMaxJob][0] = -1;

        Q->L.OverrideActivePolicy->SLimit[mptMaxJob][0] = -1;
        Q->L.OverrideActivePolicy->HLimit[mptMaxJob][0] = -1;
 
        break;
 
      case mqfignMaxPE:
    
        Q->L.OverriceJobPolicy->SLimit[mptMaxPE][0] = -1;
        Q->L.OverriceJobPolicy->HLimit[mptMaxPE][0] = -1;

        Q->L.OverrideActivePolicy->SLimit[mptMaxPE][0] = -1;
        Q->L.OverrideActivePolicy->HLimit[mptMaxPE][0] = -1;

        break;

      case mqfignMaxProc:
 
        Q->L.OverriceJobPolicy->SLimit[mptMaxNode][0] = -1;
        Q->L.OverriceJobPolicy->HLimit[mptMaxNode][0] = -1;
 
        Q->L.OverriceJobPolicy->SLimit[mptMaxProc][0] = -1;
        Q->L.OverriceJobPolicy->HLimit[mptMaxProc][0] = -1;

        Q->L.OverrideActivePolicy->SLimit[mptMaxNode][0] = -1;
        Q->L.OverrideActivePolicy->HLimit[mptMaxNode][0] = -1;
 
        Q->L.OverrideActivePolicy->SLimit[mptMaxProc][0] = -1;
        Q->L.OverrideActivePolicy->HLimit[mptMaxProc][0] = -1;
 
        break;
 
      case mqfignMaxPS: 
 
        Q->L.OverriceJobPolicy->SLimit[mptMaxPS][0] = -1;
        Q->L.OverriceJobPolicy->HLimit[mptMaxPS][0] = -1;

        Q->L.OverrideActivePolicy->SLimit[mptMaxPS][0] = -1;
        Q->L.OverrideActivePolicy->HLimit[mptMaxPS][0] = -1;
 
        break;
 
      case mqfignJobQueuedPerUser:
 
        Q->L.OverrideIdlePolicy->SLimit[mptMaxJob][0] = -1;
        Q->L.OverrideIdlePolicy->HLimit[mptMaxJob][0] = -1;
 
        break;
      }  /* END switch (index) */
    }    /* END for (index) */
 
  return(SUCCESS);
  }  /* END __MQOSSetOverrideLimits() */





/**
 * Report QoS attribute as string.
 *
 * @see MQOSSetAttr() - peer
 *
 * @param Q (I)
 * @param AIndex (I)
 * @param String (O)
 * @param Mode (I)
 */

int MQOSAToMString(

  mqos_t    *Q,
  enum MQOSAttrEnum AIndex,
  mstring_t *String,
  int        Mode)

  {
  if ((Q == NULL) || (String == NULL))
    {
    return(FAILURE);
    }

  MStringSet(String,"\0");

  switch (AIndex)
    {
    case mqaBacklog:

      /* current total backlog of jobs requesting specified QoS */

      if ((Q->L.IdlePolicy != NULL) &&
          (Q->L.IdlePolicy->Usage[mptMaxPS][0] > 0))
        {
        MStringAppendF(String,"%lu",
          Q->L.IdlePolicy->Usage[mptMaxPS][0]);
        }

      break;

    case mqaBLACLThreshold:

      if (Q->BLACLThreshold > 0.0)
        {
        MStringAppendF(String,"%f",
          Q->BLACLThreshold);
        }

      break;

    case mqaBLPowerThreshold:

      if (Q->BLPowerThreshold > 0.0)
        {
        MStringAppendF(String,"%f",
          Q->BLPowerThreshold);
        }

      break;

    case mqaBLPreemptThreshold:

      if (Q->BLPreemptThreshold > 0.0)
        {
        MStringAppendF(String,"%f",
          Q->BLPreemptThreshold);
        }

      break;

    case mqaBLRsvThreshold:

      if (Q->BLRsvThreshold > 0.0)
        {
        MStringAppendF(String,"%f",
          Q->BLRsvThreshold);
        }

      break;

    case mqaBLTriggerThreshold:

      if (Q->BLTriggerThreshold > 0.0)
        {
        MStringAppendF(String,"%f",
          Q->BLTriggerThreshold);
        }

      break;

    case mqaDedResCost:

      if (Q->DedResCost >= 0.0)
        {
        MStringAppendF(String,"%f",
          Q->DedResCost);
        }

      break;

    case mqaDynAttrs:

      bmtostring(&Q->DynAttrs,MJobDynAttr,String);

      break;

    case mqaGreenBacklogThreshold:

      if (Q->GreenBacklogThreshold != 0.0)
        {
        MStringAppendF(String,"%.2f",
          Q->GreenBacklogThreshold);
        }

      break;

    case mqaGreenQTThreshold:

      if (Q->GreenQTThreshold != 0.0)
        {
        MStringAppendF(String,"%ld",
          Q->GreenQTThreshold);
        }

      break;

    case mqaGreenXFactorThreshold:

      if (Q->GreenXFactorThreshold != 0.0)
        {
        MStringAppendF(String,"%.2f",
          Q->GreenXFactorThreshold);
        }

      break;


    case mqaSpecAttrs:

      bmtostring(&Q->SpecAttrs,MJobDynAttr,String);

      break;

    case mqaFlags:

      MQOSFlagsToMString(Q,String,0);

      break;

    case mqaFSScalingFactor:

      if (Q->FSScalingFactor != 0.0)
        MStringAppendF(String,"%f",
          Q->FSScalingFactor);

      break;

    case mqaOnDemand:

      if (Q->OnDemand == TRUE)
        {
        MStringAppendF(String,"%s",
          MBool[TRUE]);
        }

      break;

    case mqaPreemptMaxTime:

      if (Q->PreemptMaxTime > 0)
        {
        char TString[MMAX_LINE];

        MULToTString(Q->PreemptMaxTime,TString);

        MStringAppendF(String,"%s",TString);
        }

      break;

    case mqaPreemptMinTime:

      if (Q->PreemptMinTime > 0)
        {
        char TString[MMAX_LINE];

        MULToTString(Q->PreemptMinTime,TString);

        MStringAppendF(String,"%s",TString);
        }

      break;

    case mqaQTACLThreshold:

      if (Q->QTACLThreshold > 0)
        {
        MStringAppendF(String,"%ld",
          Q->QTACLThreshold);
        }

      break;

    case mqaQTPowerThreshold:

      if (Q->QTPowerThreshold > 0)
        {
        MStringAppendF(String,"%ld",
          Q->QTPowerThreshold);
        }

      break;

    case mqaQTPreemptThreshold:

      if (Q->QTPreemptThreshold > 0)
        {
        MStringAppendF(String,"%ld",
          Q->QTPreemptThreshold);
        }

      break;

    case mqaQTRsvThreshold:

      if (Q->QTRsvThreshold > 0)
        {
        MStringAppendF(String,"%ld",
          Q->QTRsvThreshold);
        }

      break;

    case mqaQTTriggerThreshold:

      if (Q->QTTriggerThreshold > 0)
        { 
        MStringAppendF(String,"%ld",
          Q->QTTriggerThreshold);
        }

      break;

    case mqaQTTarget:

      if (Q->QTTarget > 0)
        {
        MStringAppendF(String,"%ld",
          Q->QTTarget);
        }

      break;

    case mqaQTWeight:

      if (Q->QTSWeight != 0)
        {
        MStringAppendF(String,"%ld",
          Q->QTSWeight);
        }

      break;

    case mqaRsvTimeDepth:

      if (Q->RsvTimeDepth > 0)
        {
        MStringAppendF(String,"%lu",
          Q->RsvTimeDepth);
        }

      break;

    case mqaUtlResCost:

      if (Q->UtlResCost > 0.0)
        {
        MStringAppendF(String,"%f",
          Q->UtlResCost);
        }

      break;

    case mqaXFACLThreshold:

      if (Q->XFACLThreshold > 0.0)
        {
        MStringAppendF(String,"%f",
          Q->XFACLThreshold);
        }

      break;

    case mqaXFPowerThreshold:

      if (Q->XFPowerThreshold > 0.0)
        {
        MStringAppendF(String,"%f",
          Q->XFPowerThreshold);
        }

      break;

    case mqaXFPreemptThreshold:

      if (Q->XFPreemptThreshold > 0.0)
        {
        MStringAppendF(String,"%f",
          Q->XFPreemptThreshold);
        }

      break;

    case mqaXFRsvThreshold:

      if (Q->XFRsvThreshold > 0.0)
        {
        MStringAppendF(String,"%f",
          Q->XFRsvThreshold);
        }

      break;

    case mqaXFTriggerThreshold:

      if (Q->XFTriggerThreshold > 0.0)
        {
        MStringAppendF(String,"%f",
          Q->XFTriggerThreshold);
        }

      break;

    case mqaXFTarget:

      if (Q->XFTarget > 0.0)
        {
        MStringAppendF(String,"%f",
          Q->XFTarget);
        }

      break;

    case mqaXFWeight:

      if (Q->XFSWeight != 0)
        {
        MStringAppendF(String,"%ld",
          Q->XFSWeight);
        }

      break;

    default:

      return(FAILURE);

      /*NOTREACHED*/

      break;
    }  /* END switch (AIndex) */

  return(SUCCESS);
  }  /* END MQOSAToString() */





/**
 * Set specified QoS attribute.
 * 
 * @see MQOSAToString() - peer
 * @see MQOSProcessConfig() - parent
 * @see MCredSetAttr() - peer
 *
 * @param Q (I) [modified]
 * @param AIndex (I)
 * @param Value (I)
 * @param Format (I)
 * @param Mode (I) [not used]
 */

int MQOSSetAttr(

  mqos_t              *Q,       /* I *(modified) */
  enum MQOSAttrEnum    AIndex,  /* I */
  void               **Value,   /* I */
  enum MDataFormatEnum Format,  /* I */
  int                  Mode)    /* I (not used) */

  { 
  if (Q == NULL)
    {
    return(FAILURE);
    }

  switch (AIndex)
    {
    case mqaBLACLThreshold:

      Q->BLACLThreshold = strtod((char *)Value,NULL);

      break;

    case mqaBLPowerThreshold:

      Q->BLPowerThreshold = strtod((char *)Value,NULL);

      break;

    case mqaBLPreemptThreshold:

      Q->BLPreemptThreshold = strtod((char *)Value,NULL);

      break;

    case mqaBLRsvThreshold:

      Q->BLRsvThreshold = strtod((char *)Value,NULL);

      break;

    case mqaBLTriggerThreshold:

      Q->BLTriggerThreshold = strtod((char *)Value,NULL);

      break;

    case mqaDedResCost:

      Q->DedResCost = strtod((char *)Value,NULL);

      break;

    case mqaFlags:

      MQOSFlagsFromString(Q,(char *)Value);

      break;

    case mqaFSScalingFactor:

      if (Value == NULL) 
        break;

      Q->FSScalingFactor = strtod((char *)Value,NULL);

      break;

    case mqaGreenBacklogThreshold:

      Q->GreenBacklogThreshold = strtod((char *)Value,NULL);

      break;

    case mqaGreenQTThreshold:

      Q->GreenQTThreshold = strtol((char *)Value,NULL,10);

      break;

    case mqaGreenXFactorThreshold:

      Q->GreenXFactorThreshold = strtod((char *)Value,NULL);

      break;

    case mqaOnDemand:

      Q->OnDemand = MUBoolFromString((char *)Value,FALSE);

      break;

    case mqaPreemptMaxTime:

      if (Value != NULL)
        Q->PreemptMaxTime = MUTimeFromString((char *)Value);
      else
        Q->PreemptMaxTime = 0;

      break;
 
    case mqaPreemptMinTime:

      if (Value != NULL)
        Q->PreemptMinTime = MUTimeFromString((char *)Value);
      else
        Q->PreemptMinTime = 0;

      break;
 
    case mqaPreemptPolicy:

      Q->PreemptPolicy = (enum MPreemptPolicyEnum)MUGetIndex(
        (char *)Value,
        (const char **)MPreemptPolicy,
         FALSE,
        0);

      break;

    case mqaQTACLThreshold:

      Q->QTACLThreshold = MUTimeFromString((char *)Value);

      break;

    case mqaQTPowerThreshold:

      Q->QTPowerThreshold = MUTimeFromString((char *)Value);

      break;

    case mqaQTPreemptThreshold:

      Q->QTPreemptThreshold = MUTimeFromString((char *)Value);

      break;

    case mqaQTRsvThreshold:

      Q->QTRsvThreshold = MUTimeFromString((char *)Value);

      break;

    case mqaQTTriggerThreshold:

      Q->QTTriggerThreshold = MUTimeFromString((char *)Value);

      break;

    case mqaUtlResCost:

      Q->UtlResCost = strtod((char *)Value,NULL);

      break;

    case mqaXFACLThreshold:

      Q->XFACLThreshold = strtod((char *)Value,NULL);

      break;

    case mqaXFPowerThreshold:

      Q->XFPowerThreshold = strtod((char *)Value,NULL);

      break;

    case mqaXFPreemptThreshold:

      Q->XFPreemptThreshold = strtod((char *)Value,NULL);

      break;

    case mqaXFRsvThreshold:

      Q->XFRsvThreshold = strtod((char *)Value,NULL);

      break;

    case mqaXFTriggerThreshold:

      Q->XFTriggerThreshold = strtod((char *)Value,NULL);

      break;

    case mqaExemptLimit:

      bmfromstring((char *)Value,MPolicyType,&Q->ExemptFromLimits);

      break;

    case mqaSystemPrio:

      {
      long tmpPrio = strtol((char *)Value,NULL,10);

      if ((tmpPrio >= 0) && (tmpPrio <= 1000))
        {
        Q->SystemJobPriority = tmpPrio;
        }
      else
        {
        return(FAILURE);
        }
      }

      break;

    default:

      /* not handled */

      return(FAILURE);
 
      /*NOTREACHED*/

      break;
    }  /* END switch (AIndex) */

  return(SUCCESS);
  }  /* END MQOSSetAttr() */





/**
 * Convert string to QoS flag bitmap.
 *
 * @param Q (I) [modified]
 * @param Buf (I)
 */

int MQOSFlagsFromString(

  mqos_t *Q,   /* I (modified) */
  char   *Buf) /* I */

  {
  int               findex;

  enum MQOSFlagEnum vindex;

  char  UBuf[MMAX_LINE];
  char *ptr;

  if ((Q == NULL) || (Buf == NULL))
    {
    return(FAILURE);
    }

  MUStrToUpper(Buf,UBuf,sizeof(UBuf));
 
  for (findex = 0;MQOSFlags[findex] != NULL;findex++)
    {
    ptr = strstr(UBuf,MQOSFlags[findex]);

    if (ptr == NULL)
      continue;

    if (isalpha(ptr[strlen(MQOSFlags[findex])]))
      continue;

    vindex = (enum MQOSFlagEnum)findex;

    switch (vindex)
      {
      case mqfignJobPerUser:

        Q->L.OverrideActivePolicy->SLimit[mptMaxJob][0] = -1;
        Q->L.OverrideActivePolicy->HLimit[mptMaxJob][0] = -1;

        bmset(&Q->Flags,vindex);

        break;

      case mqfignProcPerUser:

        Q->L.OverrideActivePolicy->SLimit[mptMaxProc][0] = -1;
        Q->L.OverrideActivePolicy->HLimit[mptMaxProc][0] = -1;

        bmset(&Q->Flags,vindex);

        break;

      case mqfignMaxPE: 

        Q->L.OverriceJobPolicy->SLimit[mptMaxPE][0] = -1;
        Q->L.OverriceJobPolicy->HLimit[mptMaxPE][0] = -1;

        bmset(&Q->Flags,vindex);

        break;

      case mqfignMaxProc: 

        Q->L.OverriceJobPolicy->SLimit[mptMaxNode][0] = -1;
        Q->L.OverriceJobPolicy->HLimit[mptMaxNode][0] = -1;

        Q->L.OverriceJobPolicy->SLimit[mptMaxProc][0] = -1;
        Q->L.OverriceJobPolicy->HLimit[mptMaxProc][0] = -1;

        bmset(&Q->Flags,vindex);

        break;

      case mqfignMaxPS:

        Q->L.OverriceJobPolicy->SLimit[mptMaxPS][0] = -1;
        Q->L.OverriceJobPolicy->HLimit[mptMaxPS][0] = -1;

        bmset(&Q->Flags,vindex);

        break;

      case mqfignJobQueuedPerUser:

        Q->L.OverrideIdlePolicy->SLimit[mptMaxJob][0] = -1;
        Q->L.OverrideIdlePolicy->HLimit[mptMaxJob][0] = -1;

        bmset(&Q->Flags,vindex);

        break;

      case mqfCoAlloc:

        MSched.CoAllocationAllowed = TRUE;

        /* fall-through */

      case mqfignMaxTime:
      case mqfpreemptor:
      case mqfpreemptee:
      case mqfDedicated:
      case mqfReserved:
      case mqfNoBF:
      case mqfNTR:
      case mqfRunNow:
      case mqfPreemptSPV:
      case mqfIgnHostList:
      case mqfDeadline:
      case mqfEnableUserRsv:
      case mqfNoReservation:
      case mqfPreemptFSV:
      case mqfTrigger:
      case mqfPreemptorX:

        bmset(&Q->Flags,vindex);

        break;

      case mqfProvision:

        if (MOLDISSET(MSched.LicensedFeatures,mlfProvision))
          {
          bmset(&Q->Flags,vindex);
          MSched.OnDemandProvisioningEnabled = TRUE;
          }
        else
          {
          MMBAdd(
            &MSched.MB,
            "ERROR:  license does not allow dynamic provisioning, please contact Adaptive Computing\n",
            NULL,
            mmbtOther,
            0,
            0,
            NULL);
          }

        break;

      case mqfUseReservation:

        {
        char tmpLine[MMAX_LINE + 1];

        char *ptr;
        char *TokPtr;

        /* FORMAT:  USERESERVED[=<X>[:<X>]...] */

        /* FIXME:  local parsing creating higher layer conflicts */

        bmset(&Q->Flags,vindex);

        sscanf(Buf,"%1024s ",
          tmpLine);

        if (((ptr = MUStrTok(tmpLine,":",&TokPtr)) != NULL) &&
           (((ptr = MUStrTok(NULL,":",&TokPtr)) != NULL)))
          {
          int rindex;

          /* rsv access list specified */

          /* NYI: add support for multiple reservations (using J->RAList) */

          MUStrDup(&Q->F.ReqRsv,ptr);

          bmset(&Q->F.JobFlags,mjfAdvRsv);

          break;

          /* if only one specified put in J->ReqRID, otherwise J->RsvAccess */

          for (rindex = 0;rindex < MMAX_QOSRSV;rindex++)
            {
            MUStrDup(&Q->ReqRsvID[rindex],ptr);

            if ((ptr = MUStrTok(NULL,":",&TokPtr)) == NULL)
              break;
            }  /* END for (rindex) */
          }
        }      /* END BLOCK */

        break; 

      case mqfignUser:

        {
        mbitmap_t tmpI;

        bmfromstring(MQFUSER,MQOSFlags,&tmpI);

        bmor(&Q->Flags,&tmpI);
        }

        break;

      case mqfignSystem:

        {
        mbitmap_t tmpI;

        bmfromstring(MQFSYSTEM,MQOSFlags,&tmpI);

        bmor(&Q->Flags,&tmpI);
        }

        break;

      case mqfignAll:

        {
        mbitmap_t tmpI;

        bmset(&tmpI,mqfignJobPerUser);
        bmset(&tmpI,mqfignJobQueuedPerUser);
        bmset(&tmpI,mqfignMaxPE);
        bmset(&tmpI,mqfignMaxProc);
        bmset(&tmpI,mqfignMaxTime);
        bmset(&tmpI,mqfignMaxPS);

        bmor(&Q->Flags,&tmpI);
        }

        break;

      default:

        bmset(&Q->Flags,vindex);

        break;
      }  /* END switch (vindex) */
    }    /* END for (findex)    */
 
  __MQOSSetOverrideLimits(Q);
 
  mstring_t Flags(MMAX_LINE);

  MQOSFlagsToMString(Q,&Flags,1);

  MDB(2,fCONFIG) MLog("INFO:     QOS '%s' flags set to %s\n",
    Q->Name,
    Flags.c_str());

  return(SUCCESS);
  }  /* END MQOSFlagsFromString() */




/**
 * Convert QOS flag bitmap array to human-readable string.
 *
 * @see MQOSFlagsFromString()
 *
 * @param Q (I)
 * @param String (O)
 * @param Mode (I) [0 -> delim w/' ', else delim w/',']
 */

int MQOSFlagsToMString(

  mqos_t    *Q,
  mstring_t *String,
  int        Mode)

  {
  char  SepString[2];

  int   findex;

  if ((Q == NULL) || (String == NULL))
    {
    return(FAILURE);
    }

  if (Mode == 0)
    strcpy(SepString," ");
  else
    strcpy(SepString,",");

  MStringSet(String,"\0");

  /* NOTE:  should this be '0', not '1'? */

  for (findex = 1;findex < mqfLAST;findex++)
    {
    if (findex == mqfUseReservation)
      continue;

    if (bmisset(&Q->Flags,findex))
      {
      if (!MUStrIsEmpty(String->c_str()))
        MStringAppend(String,SepString);

      MStringAppend(String,(char *)MQOSFlags[findex]);
      } 
    }    /* END for (findex) */

  if (bmisset(&Q->Flags,mqfUseReservation))
    {
    if (!MUStrIsEmpty(String->c_str()))
      MStringAppend(String,SepString);

    MStringAppend(String,(char *)MQOSFlags[mqfUseReservation]);

    if (Q->F.ReqRsv != NULL)
      MStringAppendF(String,":%s",Q->F.ReqRsv);
    }  /* END if (bmisset(&Q->Flags,mqfUseReservation)) */

  return(SUCCESS);
  }  /* END MQOSFlagsToString() */




/**
 *
 *
 * NOTE: does NOT clear contents before populating
 *
 * @param Q (I)
 * @param VFlag (I)
 * @param PIndex (I)
 * @param String (O)
 */

int MQOSConfigLShow(

  mqos_t    *Q,
  int        VFlag,
  int        PIndex,
  mstring_t *String)

  {
  enum MQOSAttrEnum AList[] = {
    mqaNONE };

  int aindex;


  if ((Q == NULL) || (String == NULL))
    {
    return(FAILURE);
    }

  /* don't clear buffer */

  mstring_t tmpString(MMAX_LINE);

  for (aindex = 0;AList[aindex] != mqaNONE;aindex++)
    {
    if ((MQOSAToMString(Q,AList[aindex],&tmpString,0) == FAILURE) ||
        (MUStrIsEmpty(tmpString.c_str())))
      {
      continue;
      }

    MStringAppendF(String,"%s=%s ",
      MQOSAttr[AList[aindex]],
      tmpString.c_str());
    }  /* END for (aindex) */

  return(SUCCESS);
  }  /* END MQOSConfigLShow() */




/**
 *
 *
 * @param QP (I) [modified]
 */

int MQOSDestroy(

  mqos_t **QP)  /* I (modified) */

  {
  mqos_t *Q;

  int     rindex;

  if ((QP == NULL) || (*QP == NULL))
    {
    return(SUCCESS);
    }

  Q = *QP;

  MUFree((char **)&Q->L.OverrideActivePolicy);
  MUFree((char **)&Q->L.OverrideIdlePolicy);
  MUFree((char **)&Q->L.OverriceJobPolicy);

  MUFree((char **)&Q->Stat.XLoad);

  if (Q->F.ReqRsv != NULL)
    MUFree(&Q->F.ReqRsv);

  for (rindex = 0;rindex < MMAX_QOSRSV;rindex++)
    {
    if (Q->ReqRsvID[rindex] != NULL)
      MUFree(&Q->ReqRsvID[rindex]);

    if (Q->RsvAccessList[rindex] != NULL)
      MUFree(&Q->RsvAccessList[rindex]);

    if (Q->RsvPreemptList[rindex] != NULL)
      MUFree(&Q->RsvPreemptList[rindex]);
    }  /* END for (rindex) */

  return(SUCCESS);
  }  /* END MQOSDestroy() */




int MQOSFreeTable()

  {
  int qindex;

  mqos_t *Q;

  for (qindex = 1;qindex < MMAX_QOS;qindex++)
    {
    Q = &MQOS[qindex];

    MQOSDestroy(&Q);
    }  /* END for (qindex) */

  return(SUCCESS);
  }  /* END MQOSFreeTable() */




/**
 *
 *
 * @param J (I) [modified, may be cancelled]
 * @param RejType (I)
 * @param RejDuration (I)
 * @param RejMsg (I)
 */

int MQOSReject(

  mjob_t            *J,
  enum MHoldTypeEnum RejType,
  long               RejDuration,
  const char        *RejMsg)

  {
  char tmpLine[MMAX_LINE];
  mbool_t Hold = FALSE;

  char TString[MMAX_LINE];
  const char *FName = "MQOSReject";

  MULToTString(RejDuration,TString);

  MDB(3,fSCHED) MLog("%s(%s,%s,%s)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    TString,
    (RejMsg != NULL) ? RejMsg : "NULL");

  if ((J == NULL) || (MJOBISACTIVE(J) == TRUE))
    {
    /* only reject idle jobs */

    return(FAILURE);
    }

  /* check to see if we need to default to hold for the reject policy (e.g. handle case where only mail is set )*/

  if (bmisset(&MSched.QOSRejectPolicyBM,mjrpIgnore) == TRUE)
    {
    MDB(7,fSTRUCT) MLog("INFO:     job %s sched qos reject policy of ignore\n",
      J->Name);

    /* NO-OP */
    return (SUCCESS);
    }
  else if (bmisset(&MSched.QOSRejectPolicyBM,mjrpCancel) == TRUE)
    {
    MDB(7,fSTRUCT) MLog("INFO:     job %s sched qos reject policy of cancel\n",
      J->Name);

    Hold = FALSE;
    }
  else if (bmisset(&MSched.QOSRejectPolicyBM,mjrpHold) == TRUE)
    {
    MDB(7,fSTRUCT) MLog("INFO:     job %s sched qos reject policy of hold\n",
      J->Name);

    Hold = TRUE;
    }
  else 
    {
    MDB(7,fSTRUCT) MLog("INFO:     job %s unspecified qos reject policy, defaulting to hold\n",
      J->Name);

    Hold = TRUE;
    }

  if ((bmisset(&MSched.QOSRejectPolicyBM,mjrpMail) == TRUE) &&
      (J->Credential.U->NoEmail != TRUE))
    {
    char tmpSubj[MMAX_LINE];
    char userEmailAddr[MMAX_LINE];
    char ToList[MMAX_LINE];
 
    /* create job reject message */

    snprintf(tmpLine,sizeof(tmpLine),"job %s has been %s for invalid QOS access\n",
      J->Name,
      (Hold == TRUE) ? "placed on hold" : "cancelled");

    sprintf(tmpSubj,"job %s rejected (%s)",
      J->Name,
      (Hold == TRUE) ? "job placed on hold" : "job cancelled");

    if ((J->NotifyAddress != NULL) && (J->NotifyAddress[0] != '\0'))
      {
      strcpy(userEmailAddr,J->NotifyAddress);
      }
    else if ((J->Credential.U->EMailAddress != NULL) && (J->Credential.U->EMailAddress[0] != '\0'))
      {
      strcpy(userEmailAddr,J->Credential.U->EMailAddress);
      }
    else
      {
      sprintf(userEmailAddr,"%s@%s",J->Credential.U->Name,
        (MSched.DefaultMailDomain != NULL) ? MSched.DefaultMailDomain : "localhost");
      }

    sprintf(ToList,"%s,%s",MSysGetAdminMailList(1),userEmailAddr);

    MSysSendMail(
      ToList,
      NULL,
      tmpSubj,
      NULL,
      tmpLine);
    }  /* END if (bmisset(&MSched.QOSRejectPolicy,mjrpMail) == TRUE) */

  if (Hold == TRUE)
    {
    MJobSetHold(J,RejType,RejDuration,mhrCredAccess,RejMsg);
    }
  else  /* must be a cancel since everything else defaults to hold (and we already returned if ignore) */
    {
    mjblockinfo_t *BlockInfoPtr;

    if (RejMsg != NULL)
      {
      sprintf(tmpLine,"MOAB_INFO:  job was rejected by requested QOS- %s\n",
        RejMsg);
      }
    else
      {
      sprintf(tmpLine,"MOAB_INFO:  job was rejected by requested QOS\n");
      }

    /* alloc and add a block record to this job */

    BlockInfoPtr = (mjblockinfo_t *)MUCalloc(1,sizeof(mjblockinfo_t));

    BlockInfoPtr->PIndex = 0;

    /* note that the previous code was not setting the block reason - should it be? */

    BlockInfoPtr->BlockReason = MJobGetBlockReason(J,NULL);
    MUStrDup(&BlockInfoPtr->BlockMsg,tmpLine);

    if (MJobAddBlockInfo(J,NULL,BlockInfoPtr) != SUCCESS)
      MJobBlockInfoFree(&BlockInfoPtr);

    bmset(&J->IFlags,mjifCancel);
    }

  return(SUCCESS);
  }  /* END MQOSReject() */




/**
 * Checks if a job can run for the current user under the
 * current account (J->Cred.A) with the given QOS (QReq).
 * All valid partitions remain in J->SysPAL, invalid partitions
 * get removed.  Returns SUCCESS if any partitions remain,
 * false otherwise.
 *
 * This function only considers all the partitions when all of the 
 * flags are initially set within J->SysPAL.  Otherwise, it only 
 * considers partitions that were previously in the bitmap.
 * The J->SysPAL will never increase as a result of this function.
 *
 * Before successfully returning the J->SysPAL is set to the 
 * value calculated in this function.  (it is not set on a FAILURE)
 *
 *
 * @param J    (I) Job to check [J->SysPAL modified]
 * @param QReq (I) QOS to look for
 */

int MQOSValidateFairshare(

  mjob_t *J,
  mqos_t *QReq)

  {
  char Line[MMAX_LINE];

  int pindex;

  mbitmap_t tmpQAL;
  mbitmap_t tmpPAL;
  mbitmap_t tmpSysPAL;

  mxml_t *E;

  mbool_t Found;

  int Depth;

  const char *FName = "MQOSValidateFairshare";

  MDB(3,fSCHED) MLog("%s(%s,%s)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (QReq != NULL) ? QReq->Name : "NULL");

  /* if no acl policy, nothing to check */

  if (MSched.FSTreeACLPolicy == mfsapOff)
    {
    return(SUCCESS);
    }

  /* must have a qos to check against */

  if (QReq == NULL)
    {
    return(FAILURE);
    }

  /* must have an account */

  if (J->Credential.A == NULL)
    {
    return(FAILURE);
    }

  /* if all partitions are set, only take the usable partitions */

  if (bmissetall(&J->SysPAL,MMAX_PAR))
    {
    bmclear(&J->SysPAL);

    /* start at index 1 to skip over the "ALL" partition */

    for (pindex = 1; pindex < MMAX_PAR; pindex++)
      {
      if (MPar[pindex].Name[0] == '\1')
        continue;

      if (MPar[pindex].Name[0] == '\0')
        break;

      bmset(&J->SysPAL,pindex);
      }
    }

  bmcopy(&tmpSysPAL,&J->SysPAL);

  bmand(&tmpSysPAL,&J->SpecPAL);

  MDB(7,fSTRUCT) MLog("INFO:     job %s starting system partition access list (%s)\n",
    J->Name,
    MPALToString(&tmpSysPAL,NULL,Line));

  for (pindex = 1; pindex < MMAX_PAR; pindex++)
    {
    E = MPar[pindex].FSC.ShareTree;
    Found = FALSE;
    Depth = -1; /* search entire tree */

    if (MPar[pindex].Name[0] == '\1')
      continue;

    if (MPar[pindex].Name[0] == '\0')
      break;

    /* if MPar[pindex] not in J->SysPAL, continue */

    if (!bmisset(&J->SysPAL,pindex))
      continue;

    /* reset tmpQAL for each iteration */

    bmcopy(&tmpQAL,&MPar[0].F.QAL);

    MDB(7,fSTRUCT) MLog("INFO:     job %s global partition default QAL (%s)\n",
      J->Name,
      MQOSBMToString(&tmpQAL,Line));

    /* find specified account in the fairshare tree */

    if (J->Credential.A != NULL)
      E = MFSCredFindElement(E,mxoAcct,J->Credential.A->Name,Depth,&Found);

    /* if account does not exist under the partition, remove it from the PAL */

    if (!Found)
      {
      bmunset(&tmpSysPAL,pindex);

      MDB(7,fSTRUCT) MLog("INFO:     job %s account not found, removing partition index %d from temporary partition access list (%s)\n",
        J->Name,
        pindex,
        MPALToString(&tmpSysPAL,NULL,Line));

      continue;
      }

    /* parent: union in the QAL for the found account */

    if (MSched.FSTreeACLPolicy == mfsapParent)
      {
      bmor(&tmpQAL,&E->TData->F->QAL);
      }

    /* reset searching variables */

    Depth = 1; /* user must be the immediate child of this account */
    Found = FALSE;

    /* find specified user below the found account in the fairshare tree */

    if (J->Credential.U != NULL)
      E = MFSCredFindElement(E,mxoUser,J->Credential.U->Name,Depth,&Found);

    /* if user does not exist under the given account, remove partition from the PAL */

    if (!Found)
      {
      bmunset(&tmpSysPAL,pindex);

      MDB(7,fSTRUCT) MLog("INFO:     job %s user not found, removing partition index %d from temporary partition access list (%s)\n",
        J->Name,
        pindex,
        MPALToString(&tmpSysPAL,NULL,Line));

      continue;
      }

    /* parent/self: union in the QAL for the found user */
    /* full: get full qos bitmap for found user/account */

    if ((MSched.FSTreeACLPolicy == mfsapParent) ||
        (MSched.FSTreeACLPolicy == mfsapSelf))
      {
      bmor(&tmpQAL,&E->TData->F->QAL);

      MDB(7,fSTRUCT) MLog("INFO:     job %s account/user found, add fstree QAL to list (%s)\n",
        J->Name,
        MQOSBMToString(&tmpQAL,Line));
      }
    else /* if (MSched.FSTreeACLPolicy == mfsapFull) */
      {
      mbool_t FoundAccount = FALSE;

      MFSTreeGetQOSBitmap(
        MPar[pindex].FSC.ShareTree,
        J->Credential.A->Name,
        J->Credential.U->Name,
        FoundAccount,
        &tmpQAL);
      }

    /* if requested QOS not in union of qlists, remove partition from PAL */

    if (!bmisset(&tmpQAL,QReq->Index))
      {
      bmunset(&tmpSysPAL,pindex);

      MDB(7,fSTRUCT) MLog("INFO:     job %s requested QOS %s, not in QAL list (%s) remove partition index %d from tmpSysPAL\n",
        J->Name,
        QReq->Name,
        MQOSBMToString(&tmpQAL,Line),
        pindex);
      }
    }/* END for(pindex) */

  /* Take the user specified PAL and combine it with the newly created tmpSysPAL */
  /* if any partition remains, return success */

  bmcopy(&tmpPAL,&tmpSysPAL);
  bmand(&tmpPAL,&J->SpecPAL);

  if (bmisclear(&tmpPAL))
    {
    return(FAILURE);
    }

  /* Save the tmpSysPAL onto the job itself */

  bmcopy(&J->SysPAL,&tmpSysPAL);

  MDB(7,fSTRUCT) MLog("INFO:     job %s final system partition access list (%s)\n",
    J->Name,
    MPALToString(&J->SysPAL,NULL,Line));

  return(SUCCESS);
  }  /* END MQOSValidateFairshare() */

/* END MQOS.c */
