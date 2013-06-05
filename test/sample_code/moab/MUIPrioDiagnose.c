/* HEADER */

/**
 * @file MUIPrioDiagnose.c
 *
 * Contains MUI Priority Diagnose
 */



#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


#define MMAX_JOBPRIOXMLSTRINGSIZE 2048

/**
 * Report and analyze job priority (mdiag -p).
 *
 * @see MJobCalcStartPriority() - child
 *
 * @param Auth (I) (FIXME: not used but should be)
 * @param String (O)
 * @param P (I)
 * @param ReqE (I) 
 * @param DFlags (I) [bitmap of enum mcm*]
 * @param DFormat (I)
 */

int MUIPrioDiagnose(

  char                *Auth,
  mstring_t           *String,
  mpar_t              *P,
  mxml_t              *ReqE,
  mbitmap_t           *DFlags,
  enum MFormatModeEnum DFormat)

  {
  int index;
  int JobCount;

  double tmpD;

  mjob_t *J;

  mxml_t *E = NULL;

  mbool_t UseJobName = FALSE;

  mgcred_t *JU = NULL;
  mgcred_t *JG = NULL;
  mgcred_t *JA = NULL;
  mclass_t *JC = NULL;
  mqos_t   *JQ = NULL;

  void     *O;

  int       CTok;
  enum MXMLOTypeEnum OIndex;

  char tmpSVal[MMAX_LINE];
  char tmpDVal[MMAX_LINE];

  const char *FName = "MUIPrioDiagnose";

  MDB(2,fUI) MLog("%s(Buffer,BufSize,%s,ReqE,%s)\n",
    FName,
    (P != NULL) ? P->Name : "NULL",
    MFormatMode[DFormat]);

  if (String == NULL)
    {
    return(FAILURE);
    }

  /* process all constraints */

  CTok = -1;

  UseJobName = bmisset(DFlags,mcmJobName);

  MStringSet(String,"\0");

  while (MS3GetWhere(
      ReqE,
      NULL,
      &CTok,
      tmpSVal,         /* O */
      sizeof(tmpSVal),
      tmpDVal,         /* O */
      sizeof(tmpDVal)) == SUCCESS)
    {
    OIndex = (enum MXMLOTypeEnum)MUGetIndexCI(tmpSVal,MXO,FALSE,0);

    switch (OIndex)
      {
      case mxoUser:
      case mxoGroup:
      case mxoAcct:
      case mxoClass:
      case mxoQOS:

        if (MOGetObject(OIndex,tmpDVal,(void **)&O,mVerify) == FAILURE)
          {
          MDB(5,fUI) MLog("INFO:     cannot locate %s %s for constraint in %s\n",
            MXO[OIndex],
            tmpDVal,
            FName);

          if (DFormat != mfmXML)
            {
            MStringAppendF(String,"ERROR:    cannot locate %s %s in Moab's historical data\n",
              MXO[OIndex],
              tmpDVal);
            }

          return(FAILURE);
          }

        if (OIndex == mxoUser)
          JU = (mgcred_t *)O;
        else if (OIndex == mxoGroup)
          JG = (mgcred_t *)O;
        else if (OIndex == mxoAcct)
          JA = (mgcred_t *)O;
        else if (OIndex == mxoClass)
          JC = (mclass_t *)O;
        else if (OIndex == mxoQOS)
          JQ = (mqos_t *)O;

        break;

      default:

        /* NO-OP */

        break;
      }  /* END switch (OIndex) */
    }    /* END while (MS3GetWhere() == SUCCESS) */

  if (DFormat != mfmXML)
    {
    MStringAppendF(String,"diagnosing job priority information (partition: %s)\n\n",
      P->Name);

    E = NULL;
    }

  /* initialize priority statistics and build header */

  MJobCalcStartPriority(
    NULL,
    P,
    NULL,
    mpdHeader,
    (DFormat == mfmXML) ? &E : NULL,
    (DFormat == mfmXML) ? NULL : String,
    DFormat,
    UseJobName);

  JobCount = 0;

  /* NOTE:  MUIQ[] only contains jobs which do not violate any 
            policies (soft/hard policies, holds, etc) */

  if (bmisset(DFlags,mcmVerbose2))
    {
    job_iter JTI;

    mjob_t **tmpJobs = NULL;
    int JIndex = 0;
    int JCount = 0;

    MJobListAlloc(&tmpJobs);

    MJobIterInit(&JTI);

    while (MJobTableIterate(&JTI,&J) == SUCCESS)
      {
      tmpJobs[JIndex++] = J;
      }

    JCount = JIndex;

    tmpJobs[JIndex] = NULL; /* Make sure the last one is NULL */

    qsort(
      (void *)tmpJobs,
      JIndex,
      sizeof(mjob_t *),
      (int(*)(const void *,const void *))MJobStartPrioComp);

    JCount = JIndex;

    for (JIndex = 0; JIndex < JCount; JIndex++)
      {
      J = tmpJobs[JIndex];

      if (bmisset(&J->IFlags,mjifIsHidden))
        {
        continue;
        }

      if (((JU != NULL) && (J->Credential.U != JU)) ||
          ((JG != NULL) && (J->Credential.G != JG)) ||
          ((JA != NULL) && (J->Credential.A != JA)) ||
          ((JQ != NULL) && (J->Credential.Q != JQ)) ||
          ((JC != NULL) && (J->Credential.C != JC)))
        {
        /* job rejected by request constraints */

        continue;
        }

      if ((P->Index != 0) &&
          (((bmisset(&J->PAL,P->Index)) == 0) ||
           ((J->Req[0]->PtIndex != 0) &&
            (J->Req[0]->PtIndex != P->Index))))
        {
        /* job rejected by partition request */

        continue;
        }

      MDB(5,fUI) MLog("INFO:     diagnosing priority for job '%s'\n",
        J->Name);

      MJobCalcStartPriority(
        J,
        P,
        &tmpD,    /* O */
        mpdJob,
        (DFormat == mfmXML) ? &E : NULL,  /* O */
        (DFormat == mfmXML) ? NULL : String,  /* O */
        DFormat,
        UseJobName);

      JobCount++;
      }  /* END for (index) */

    MUFree((char **)&tmpJobs);
    }    /* END else (bmisset(DFlags,mcmVerbose2)) */
  else
    {
    /* report eligible jobs */

    for (index = 0;MUIQ[index] != NULL;index++)
      {
      J = MUIQ[index];

      if (!MJobPtrIsValid(J))
        continue;

      if (bmisset(&J->IFlags,mjifIsHidden))
        {
        continue;
        }

      if (bmisset(DFlags,mcmVerbose))
        {
        /* NOTE:  active, eligible jobs will be reported in the verbose output section below */

        if (MJOBISACTIVE(J) == TRUE)
          continue;
        }

      if (((JU != NULL) && (J->Credential.U != JU)) ||
          ((JG != NULL) && (J->Credential.G != JG)) ||
          ((JA != NULL) && (J->Credential.A != JA)) ||
          ((JQ != NULL) && (J->Credential.Q != JQ)) ||
          ((JC != NULL) && (J->Credential.C != JC)))
        {
        /* job rejected by request constraints */

        continue;
        }

      if ((P->Index != 0) && 
          (((bmisset(&J->PAL,P->Index)) == 0) ||
           ((J->Req[0]->PtIndex != 0) &&
            (J->Req[0]->PtIndex != P->Index))))
        {
        /* job rejected by partition request */

        continue;
        }

      MDB(5,fUI) MLog("INFO:     diagnosing priority for job '%s'\n",
        J->Name);

      MJobCalcStartPriority(
        J,
        P,
        &tmpD,    /* O */
        mpdJob,
        (DFormat == mfmXML) ? &E : NULL,  /* O */
        (DFormat == mfmXML) ? NULL : String,  /* O */
        DFormat,
        UseJobName);

      MJobSetStartPriority(
        J,
        (P == NULL) ? 0 : P->Index,
        (long)tmpD);

      JobCount++;
      }  /* END for (index) */
    }    /* END else (bmisset(DFlags,mcmVerbose2)) */

  if (bmisset(DFlags,mcmVerbose) && 
      (MAQ.NumItems > 0) &&
      (!bmisset(DFlags,mcmVerbose2)))
    {
    mhashiter_t HTI;

    mjob_t **tmpMAQ = (mjob_t **)MUCalloc(1,sizeof(mjob_t *) * (MAQ.NumItems + 1));

    /* Only if mcmVerbose2 is not set because that will already show active jobs */

    if (DFormat != mfmXML)
      {
      MStringAppend(String,"---Active Jobs---\n"); 
      }

    MUHTIterInit(&HTI);

    index = 0;

    while (MUHTIterate(&MAQ,NULL,(void **)&J,NULL,&HTI) == SUCCESS)
      {
      tmpMAQ[index++] = J;
      }

    tmpMAQ[index] = NULL;

    qsort(
      (void *)tmpMAQ,
      index,
      sizeof(mjob_t *),
      (int(*)(const void *,const void *))MJobStartPrioComp);

    /* report active jobs */

    for (index = 0;tmpMAQ[index] != NULL;index++)
      {
      J = tmpMAQ[index];

      if (!MJobPtrIsValid(J))
        continue;

      if (((JU != NULL) && (J->Credential.U != JU)) ||
          ((JG != NULL) && (J->Credential.G != JG)) ||
          ((JA != NULL) && (J->Credential.A != JA)) ||
          ((JQ != NULL) && (J->Credential.Q != JQ)) ||
          ((JC != NULL) && (J->Credential.C != JC)))
        {
        /* job rejected by request constraints */

        continue;
        }

      if ((P->Index != 0) &&
          (((bmisset(&J->PAL,P->Index)) == 0) ||
           ((J->Req[0]->PtIndex != 0) &&
            (J->Req[0]->PtIndex != P->Index))))
        {
        /* job rejected by partition request */

        continue;
        }

      MDB(5,fUI) MLog("INFO:     diagnosing priority for job '%s'\n",
        J->Name);

      MJobCalcStartPriority(
        J,
        P,
        &tmpD,  /* O */
        mpdJob,
        (DFormat == mfmXML) ? &E : NULL,
        (DFormat == mfmXML) ? NULL : String,
        DFormat,
        UseJobName);

      JobCount++;
      }  /* END for (index) */

    MUFree((char **)&tmpMAQ);
    }    /* END if (bmisset(DFlags,mcmVerbose)) */

  if (JobCount > 0)
    {
    /* generate summary info */

    MJobCalcStartPriority(
      NULL,
      P,
      NULL,
      mpdFooter,
      (DFormat == mfmXML) ? &E : NULL,
      (DFormat == mfmXML) ? NULL : String,
      DFormat,
      UseJobName);
    }
  else
    {
    if (DFormat != mfmXML)
      {
      MStringSet(String,"no eligible jobs in queue\n");

      return(SUCCESS);
      }
    }

  if (DFormat == mfmXML)
    {
    if (E != NULL)
      MXMLToMString(E,String,NULL,TRUE);

    MXMLDestroyE(&E);

    return(SUCCESS);
    }

  if (bmisset(DFlags,mcmVerbose))
    {
    mhashiter_t HTI;

    /* sync w/OCWeight, OCWParam, OFWeight */

    const enum MXMLOTypeEnum OList[] = {
      mxoUser, 
      mxoGroup, 
      mxoAcct, 
      mxoQOS, 
      mxoClass, 
      mxoNONE };

    const enum MPrioSubComponentEnum OCWeight[] = {
      mpsCU, 
      mpsCG, 
      mpsCA, 
      mpsCQ, 
      mpsCC, 
      mpsNONE };

    const enum MCfgParamEnum OCWParam[] = {
      mcoCUWeight, 
      mcoCGWeight, 
      mcoCAWeight, 
      mcoCQWeight, 
      mcoCCWeight, 
      mcoNONE };

    const enum MPrioSubComponentEnum OFWeight[] = {
      mpsFU, 
      mpsFG, 
      mpsFA, 
      mpsFQ, 
      mpsFC, 
      mpsNONE };

    const enum MCfgParamEnum OFWParam[] = {
      mcoFUWeight, 
      mcoFGWeight, 
      mcoFAWeight, 
      mcoFQWeight, 
      mcoFCWeight, 
      mcoFJPUWeight, 
      mcoFPPUWeight,
      mcoFPSPUWeight, 
      mcoNONE };

    mfs_t  *F;
    mfs_t  *DF;

    void   *OE;
    void   *O;
    void   *OP;

    int     oindex;

    char *OName;
    char *CName;

    mbool_t CPrioritySet;
    mbool_t CFSTargetSet;

    /* diagnose priority configuration */

    /* check credential and fairshare based priority */

    for (oindex = 0;OList[oindex] != mxoNONE;oindex++)
      {
      MOINITLOOP(&OP,OList[oindex],&OE,&HTI);

      MDB(4,fFS) MLog("INFO:     checking %s priority\n",
        MXO[OList[oindex]]);

      if ((P->FSC.PCW[mpcCred] != 0) && (P->FSC.PSW[OCWeight[oindex]] != 0) &&
          (P->FSC.PCW[mpcFS] != 0) && (P->FSC.PSW[OFWeight[oindex]] != 0))
        {
        continue;
        }

      DF = NULL;

      switch (OList[oindex])
        {
        case mxoUser:

          if (MSched.DefaultU != NULL)
            DF = &MSched.DefaultU->F;

          break;

        case mxoGroup:

          if (MSched.DefaultG != NULL)
            DF = &MSched.DefaultG->F;

          break;

        case mxoAcct:

          if (MSched.DefaultA != NULL)
            DF = &MSched.DefaultA->F;

          break;

        case mxoClass:

          if (MSched.DefaultC != NULL)
            DF = &MSched.DefaultC->F;

          break;

        case mxoQOS:

          if (MSched.DefaultQ != NULL)
            DF = &MSched.DefaultQ->F;

          break;

        default:

          /* NO-OP */

          break;
        }  /* END switch (OList[oindex]) */

      CName = NULL;

      CPrioritySet = FALSE;
      CFSTargetSet = FALSE;

      if ((DF != NULL) && (DF->Priority != 0))
        {
        CPrioritySet = TRUE;
        }

      if ((DF != NULL) && (DF->FSTarget != 0))
        {
        CFSTargetSet = TRUE;
        }

      if ((CPrioritySet == FALSE) || (CFSTargetSet == FALSE))
        {
        while ((O = MOGetNextObject(&OP,OList[oindex],OE,&HTI,&OName)) != NULL)
          {
          if (MOGetComponent(O,OList[oindex],(void **)&F,mxoxFS) == FAILURE)
            continue;

          if (F->Priority != 0)
            {
            CName = OName;

            CPrioritySet = TRUE;
            }

          if (F->FSTarget != 0)
            {
            CFSTargetSet = TRUE;
            }

          if ((CPrioritySet == TRUE) && (CFSTargetSet == TRUE))
            break;
          }  /* END while ((O = MOGetNextObject()) != NULL) */
        }    /* END if ((CPrioritySet == FALSE) || (CFSTargetSet == FALSE)) */

      if ((CPrioritySet == TRUE) &&
         ((P->FSC.PCW[mpcCred] == 0) || (P->FSC.PSW[OCWeight[oindex]] == 0)))
        {
        MStringAppendF(String,"NOTE:  priority set for %s '%s' but priority weights not specified: %s %s\n",
          MXO[OList[oindex]],
          (CName != NULL) ? CName : "DEFAULT",
          (P->FSC.PCW[mpcCred] == 0) ? MParam[mcoCredWeight] : "",
          (P->FSC.PSW[OCWeight[oindex]] == 0) ? MParam[OCWParam[oindex]] : "");
        }

      if ((CFSTargetSet == TRUE) && (MPar[0].FSC.FSPolicy != mfspNONE) &&
         ((P->FSC.PCW[mpcFS] == 0) || (P->FSC.PSW[OFWeight[oindex]] == 0)))
        {
        MStringAppendF(String,"NOTE:  FS target set for %s '%s' but priority weights not specified: %s %s\n",
          MXO[OList[oindex]],
          (CName != NULL) ? CName : "DEFAULT",
          (P->FSC.PCW[mpcFS] == 0) ? MParam[mcoFSWeight] : "",
          (P->FSC.PSW[OFWeight[oindex]] == 0) ? MParam[OFWParam[oindex]] : "");
        }
      }    /* END for (oindex) */
    }      /* END if (bmisset(DFlags,mcmVerbose)) */

  if (!bmisset(&MPar[0].Flags,mpfEnableNegJobPriority))
    {
    MStringAppendF(String,"\nNOTE:  minimum allowed job priority is 1, set %s to TRUE to allow negative priority jobs - any negative priority is shown for reference\n",
      MParam[mcoEnableNegJobPriority]);
    }
  else if ((bmisset(&MPar[0].Flags,mpfEnableNegJobPriority)) && (bmisset(&MPar[0].Flags,mpfRejectNegPrioJobs)))
    {
    MStringAppendF(String,"NOTE:  negative priority jobs will be blocked (%s=TRUE  %s=TRUE)\n",
      MParam[mcoEnableNegJobPriority],
      MParam[mcoRejectNegPrioJobs]);
    }

  return(SUCCESS);
  }  /* END MUIPrioDiagnose() */
/* END MUIPrioDiagnose.c */
