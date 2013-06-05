/* HEADER */

/**
 * @file MUICredCtl.c
 *
 * Contains MUI Credential function
 */



#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/** 
 * This routine adds Per Job Limits as attributes to a XML element, 
 *  
 * @param CE       (I/O) pointer to an XML element 
 * @param P        (I) Policy and Usage Limits pointer 
 * @param LType    (I) Policy Type (idle, active, etc) 
 * @param pindex   (I) parameter index 
 * @param APString (I) active policy string [optional] 
 */

int __MUIXMLPolicyLimits(

  mxml_t    *CE,
  mpu_t     *P,
  enum MLimitTypeEnum LType,
  int        pindex,
  char      *APString)

  {
  int         index;
  const char *PType;

  if ((P == NULL) || (CE == NULL))
    return(FAILURE);

  /* check the limits and if there is one defined then build an
     XML element, set its attribute to contain the limit and usage info
     and add it as a child to the XML element passed into this routine */

  for (index = 0;index < mptLAST;index++)    
    {
    if (index == mptMaxGRes)
      {
      char *GResName;

      mhashiter_t HTI;

      mgpu_t *GP;

      if (P->GLimit == NULL)
        continue;

      MUHTIterInit(&HTI);

      while (MUHTIterate(P->GLimit,&GResName,(void **)&GP,NULL,&HTI) == SUCCESS)
        {
        char tmpName[MMAX_LINE];

        mxml_t *LE;
   
        if ((GP->SLimit[pindex] < 0) && (GP->HLimit[pindex] < 0))
          continue;

        PType = (LType == mlIdle) ? MIdlePolicyType[index] : MPolicyType[index];
   
        MXMLCreateE(&LE,(char *)MXO[mxoxLimits]);

        snprintf(tmpName,sizeof(tmpName),"%s:%s",PType,GResName);

        MXMLSetAttr(LE,"NAME",(void *)tmpName,mdfString);
   
        MXMLSetAttr(LE,"SLIMIT",(void *)&GP->SLimit[pindex],mdfLong);
   
        /* only send a hard limit if different than the soft limit */
   
        if (GP->SLimit[pindex] != GP->HLimit[pindex])
          MXMLSetAttr(LE,"HLIMIT",(void *)&GP->HLimit[pindex],mdfLong);
   
        /* only send a usage if non-zero */
   
        if (GP->Usage[pindex] != 0)
          MXMLSetAttr(LE,"USAGE",(void *)&GP->Usage[pindex],mdfLong);
   
        /* add active policy info as an xml attribute (from APU, APC, etc.) as in "user:roger" */
   
        if (APString != NULL)
          MXMLSetAttr(LE,"aptype",(void *)APString,mdfString);
   
        MXMLAddE(CE,LE);
        }

      continue;
      }
    else if ((P->SLimit[index][pindex] >= 0) || (P->HLimit[index][pindex] >= 0))
      {
      mxml_t *LE;

      PType = (LType == mlIdle) ? MIdlePolicyType[index] : MPolicyType[index];

      MXMLCreateE(&LE,(char *)MXO[mxoxLimits]);
      MXMLSetAttr(LE,"NAME",(void *)PType,mdfString);

      MXMLSetAttr(LE,"SLIMIT",(void *)&P->SLimit[index][pindex],mdfLong);

      /* only send a hard limit if different than the soft limit */

      if (P->SLimit[index][pindex] != P->HLimit[index][pindex])
        MXMLSetAttr(LE,"HLIMIT",(void *)&P->HLimit[index][pindex],mdfLong);

      /* only send a usage if non-zero */

      if (P->Usage[index][pindex] != 0)
        MXMLSetAttr(LE,"USAGE",(void *)&P->Usage[index][pindex],mdfLong);

      /* add active policy info as an xml attribute (from APU, APC, etc.) as in "user:roger" */

      if (APString != NULL)
        MXMLSetAttr(LE,"aptype",(void *)APString,mdfString);

      MXMLAddE(CE,LE);
      }
    }  /* END for (index) */

  return(SUCCESS);
  } /*END __MUIXMLPolicyLimits() */


/**  
 * This routine adds Per Job Limits as attributes to a XML element, 
 *  
 * @param LE (I/O) pointer to an XML element 
 * @param L (I) pointer to per cred limits 
 * @param pindex (I) parameter index 
 */

int __MUIXMLPerJobLimits(

  mxml_t    *LE,
  mcredl_t  *L,
  int        pindex)

  {
  mjob_t *JMax;
  mjob_t *JMin;

  if ((L == NULL) || (LE == NULL))
    {
    return(FALSE);
    }

  if ((JMax = L->JobMaximums[pindex]) != NULL)
    {
    mjob_t *JMax = L->JobMaximums[pindex];

    if (JMax->CPULimit > 0)
      MXMLSetAttr(LE,(char *)MParAttr[mpaMaxCPULimit],(void *)&JMax->CPULimit,mdfLong);

    if (JMax->PSDedicated > 0)
      MXMLSetAttr(LE,(char *)MParAttr[mpaMaxPS],(void *)&JMax->PSDedicated,mdfDouble);

    if (JMax->Request.TC > 0)
      MXMLSetAttr(LE,(char *)MParAttr[mpaMaxProc],(void *)&JMax->Request.TC,mdfInt);

    if (JMax->Request.NC > 0)
      MXMLSetAttr(LE,(char *)MParAttr[mpaMaxNode],(void *)&JMax->Request.NC,mdfInt);

    if (JMax->SpecWCLimit[0] > 0)
      MXMLSetAttr(LE,(char *)MParAttr[mpaMaxWCLimit],(void *)&JMax->SpecWCLimit[0],mdfLong);
    }

  if ((JMin = L->JobMinimums[pindex]) != NULL)
    {
    if (JMin->Request.TC > 0)
      MXMLSetAttr(LE,(char *)MParAttr[mpaMinProc],(void *)&JMin->Request.TC,mdfInt);

    if (JMin->Request.NC > 0)
      MXMLSetAttr(LE,(char *)MParAttr[mpaMinNode],(void *)&JMin->Request.NC,mdfInt);

    if (JMin->SpecWCLimit[0] > 0)
      MXMLSetAttr(LE,(char *)MParAttr[mpaMinWCLimit],(void *)JMin->SpecWCLimit[0],mdfLong);
    }

  return(SUCCESS);
  } /* END __MUIXMLPerJobLimits() */




/** 
 * This routine adds Active Policy Limits as attributes to a XML element, 
 *  
 * @param CE (I/O) pointer to an XML element 
 * @param L (I) pointer to per cred limits 
 * @param pindex (I) parameter index 
 */

int __MUIXMLIdlePolicyLimits(

  mxml_t   *CE,
  mcredl_t *L,
  int       pindex)

  {
  int ipindex;

  enum MAttrEnum IdlePolicyCredList[] = {
    maUser,
    maGroup,
    maAcct,
    maQOS,
    maClass,
    maNONE };

    if ((CE == NULL) || (L == NULL))
      {
      return(FAILURE);
      }

  for (ipindex = 0;IdlePolicyCredList[ipindex] != maNONE;ipindex++)
    {
    mhashiter_t IPxIter;
    mhash_t    *IPx = NULL;
    mpu_t      *mpuPtr;

    switch(IdlePolicyCredList[ipindex])
      {
      case maUser:
        IPx = L->UserIdlePolicy;
        break;

      case maGroup:
        IPx = L->GroupIdlePolicy;
        break;

      case maAcct:
        IPx = L->AcctIdlePolicy;
        break;

      case maQOS:
        IPx = L->QOSIdlePolicy;
        break;

      case maClass:
        IPx = L->ClassIdlePolicy;
        break;

      default:
        break;

      } /* END switch */

    if (IPx == NULL)
      continue;

    /* active policies are stored as hash table entries in L */

    MUHTIterInit(&IPxIter);

    while (MUHTIterate(IPx,NULL,(void **)&mpuPtr,NULL,&IPxIter) == SUCCESS)
      {
      char  tmpBuf[MMAX_LINE];
      char *tmpIPName;

      tmpIPName = ((mgcred_t *)mpuPtr->Ptr)->Name;

      if ((tmpIPName != NULL) && (strlen(tmpIPName) == 0))
        tmpIPName = NULL;

      /* it may or may not have a name "user" or "user:sam" */

      sprintf(tmpBuf,"%s%s%s",
        MHRObj[IdlePolicyCredList[ipindex]],
        (tmpIPName != NULL) ? ":" : "",
        (tmpIPName != NULL) ? tmpIPName : "");

      __MUIXMLPolicyLimits(CE,mpuPtr,mlIdle,pindex,tmpBuf);
      }

    } /* END for ipindex */

  return(SUCCESS);

  } /*END __MUIXMLIdlePolicyLimits() */









/** 
 * This routine adds Active Policy Limits as attributes to a XML element, 
 *  
 * @param CE (I/O) pointer to an XML element 
 * @param L (I) pointer to per cred limits 
 * @param pindex (I) parameter index 
 */

int __MUIXMLActivePolicyLimits(

  mxml_t   *CE,
  mcredl_t *L,
  int       pindex)

  {
  int apindex;

  enum MAttrEnum ActivePolicyCredList[] = {
    maUser,
    maGroup,
    maAcct,
    maQOS,
    maClass,
    maNONE };

    if ((CE == NULL) || (L == NULL))
      {
      return(FAILURE);
      }

  for (apindex = 0;ActivePolicyCredList[apindex] != maNONE;apindex++)
    {
    mhashiter_t APxIter;
    mhash_t    *APx = NULL;
    mpu_t      *mpuPtr;

    switch(ActivePolicyCredList[apindex])
      {
      case maUser:
        APx = L->UserActivePolicy;
        break;

      case maGroup:
        APx = L->GroupActivePolicy;
        break;

      case maAcct:
        APx = L->AcctActivePolicy;
        break;

      case maQOS:
        APx = L->QOSActivePolicy;
        break;

      case maClass:
        APx = L->ClassActivePolicy;
        break;

      default:
        break;

      } /* END switch */

    if (APx == NULL)
      continue;

    /* active policies are stored as hash table entries in L */

    MUHTIterInit(&APxIter);

    while (MUHTIterate(APx,NULL,(void **)&mpuPtr,NULL,&APxIter) == SUCCESS)
      {
      char  tmpBuf[MMAX_LINE];
      char *tmpAPName;

      tmpAPName = ((mgcred_t *)mpuPtr->Ptr)->Name;

      if ((tmpAPName != NULL) && (strlen(tmpAPName) == 0))
        tmpAPName = NULL;

      /* it may or may not have a name "user" or "user:sam" */

      sprintf(tmpBuf,"%s%s%s",
        MHRObj[ActivePolicyCredList[apindex]],
        (tmpAPName != NULL) ? ":" : "",
        (tmpAPName != NULL) ? tmpAPName : "");

      __MUIXMLPolicyLimits(CE,mpuPtr,mlActive,pindex,tmpBuf);
      }

    } /* END for apindex */

  return(SUCCESS);

  } /*END __MUIXMLActivePolicyLimits() */





/**  
 * This routine adds Per Job Limits as attributes to a XML element, 
 * and Policy limits as child elements to the supplied XML element. 
 *  
 * @param CE (I/O) pointer to an XML element (alloc'ed in this routine) 
 * @param EName (I) xml element name 
 * @param AName (I) xml element attr name 
 * @param Name (I) name of the xml element attr 
 * @param L (I) pointer to per cred limits 
 * @param pindex (I) parameter index 
 */

int __MUIXMLCredLimits(
  mxml_t    **CE,
  char       *EName,
  const char *AName,
  char       *Name,
  mcredl_t   *L,
  int         pindex)

  {

  if ((CE == NULL) || (EName == NULL) || 
      (AName == NULL) || (L == NULL))
    {
    return(FAILURE);
    }

  /* create an xml element */

  MXMLCreateE(CE,EName);
  MXMLSetAttr(*CE,AName,Name,mdfString);

  /* add maxsubmit limit (and total job count) as attrs to the xml element */

  if (L->MaxSubmitJobs[pindex] > 0)
    {
    MXMLSetAttr(*CE,"MAXSUBMITJOBS",(void *)&L->MaxSubmitJobs[pindex],mdfInt);

    if (L->TotalJobs[pindex] > 0)
      MXMLSetAttr(*CE,"TOTALJOBS",(void *)&L->TotalJobs[pindex],mdfInt);
    }

  /* add per job limits as attributes to the xml element */

  __MUIXMLPerJobLimits(*CE,L,pindex);

  /* add policy limits as child elements to the xml element */

  __MUIXMLPolicyLimits(*CE,&L->ActivePolicy,mlActive,pindex,NULL);
  __MUIXMLPolicyLimits(*CE,L->IdlePolicy,mlIdle,pindex,NULL);

  /* add active policy limits (APU, APC, etc. ) as child elements to the xml element */

  __MUIXMLActivePolicyLimits(*CE,L,pindex);
  __MUIXMLIdlePolicyLimits(*CE,L,pindex);

  return(SUCCESS);
  } /* END __MUIXMLCredLimits() */



/** 
 * This routine generates an XML structure containing RM, Partition, cred limits. 
 *  
 * @param SEPtr (I/O) 
 */

int __MUIGetLimitsXMLMString(

  mxml_t **SEPtr)

  {
  int     pindex;
  int     rmindex;

  mgcred_t *Cred;

  mhashiter_t HTIter;

  /* the root of the policies XML structure */

  if (MXMLCreateE(SEPtr,"policies") == FAILURE)
    {
    return(FAILURE);
    }

#if 0
  MXMLSetAttr(*SEPtr,(char *)MSchedAttr[msaName],(void *)MSched.Name,mdfString);
#endif

  /* put any RM limits inside the scheduler XML element */

  for (rmindex = 0;rmindex < MSched.M[mxoRM];rmindex++)
    {
    mrm_t *R = &MRM[rmindex];
    mxml_t *RE = NULL;


    if (R->Name[0] == '\0')
      break;

    if (MRMIsReal(R) == FALSE)
      continue;

    if (R->MaxJobPerMinute > 0)
      {
      MXMLCreateE(&RE,(char *)MXO[mxoRM]);
      MXMLSetAttr(RE,(char *)MRMAttr[mrmaMaxJobPerMinute],(void *)&R->MaxJobPerMinute,mdfInt);
      }

    if (RE != NULL)
      {
      MXMLSetAttr(RE,(char *)MRMAttr[mrmaName],(void *)R->Name,mdfString);
      MXMLAddE(*SEPtr,RE);
      }
    }

  /* if any partitions (or creds within a partition) have limits, add a partition element
     to the scheduler element */

  for (pindex = 0;pindex < MMAX_PAR;pindex++)
    {
    mxml_t *ParE = NULL;
    mpar_t *P = &MPar[pindex];
    int  qindex;
    int  cindex;

    if (P->Name[0] == '\0')
      break;

    if (P->Name[0] == '\1')
      continue;

    /* create a partition element and add any partition per job limits as attributes
       and add any policy limits as child elements */

    __MUIXMLCredLimits(&ParE,(char *)MXO[mxoPar],(char *)MParAttr[mpaID],P->Name,&P->L,pindex);

#if 1
    if (P->MaxPPN > 0)
      MXMLSetAttr(ParE,"MaxPPN",(void *)&P->MaxPPN,mdfInt);
#endif

    /* create cred child elements (class, user, etc.) and add any per job limits as attributes
       and add any policy limits as child elements. If no limits found then
       destroy the element and do not add it to the partition element, otherwise,
       add it to the partition element */

    /* class elements */

    for (cindex = MCLASS_DEFAULT_INDEX;cindex < MSched.M[mxoClass];cindex++)
      {
      mxml_t *CE = NULL;
      mclass_t *C = &MClass[cindex];

      if ((C->Name[0] == '\0') || (C->Name[1] == '\1'))
        continue;

      __MUIXMLCredLimits(&CE,(char *)MXO[mxoClass],(char *)MClassAttr[mclaName],C->Name,&C->L,pindex);

      /* if limit(s) found then add the child element to the partition element, otherwise, destroy it */

      if ((CE->CCount > 0) || (CE->ACount > 1))
        MXMLAddE(ParE,CE);
      else
        MXMLDestroyE(&CE);

      } /* END for cindex */

    /* qos elements */

    for (qindex = 0;qindex < MSched.M[mxoQOS];qindex++)
      {
      mxml_t *QE = NULL;
      mqos_t *Q = &MQOS[qindex];

      if (Q->Name[0] == '\0')
        break;

      __MUIXMLCredLimits(&QE,(char *)MXO[mxoQOS],"QOS",Q->Name,&Q->L,pindex);

      /* if limit(s) found then add the child element to the partition element, otherwise, destroy it */

      if ((QE->CCount > 0) || (QE->ACount > 1))
        MXMLAddE(ParE,QE);
      else
        MXMLDestroyE(&QE);

      }  /* END for (qindex) */

    /* user elements */

    MUHTIterInit(&HTIter);

    while (MUHTIterate(&MUserHT,NULL,(void **)&Cred,NULL,&HTIter) == SUCCESS)
      {
      mxml_t   *UE;

      __MUIXMLCredLimits(&UE,(char *)MXO[mxoUser],"User",Cred->Name,&Cred->L,pindex);

      /* if limit(s) found then add the child element to the partition element, otherwise, destroy it */

      if ((UE->CCount > 0) || (UE->ACount > 1))
        MXMLAddE(ParE,UE);
      else
        MXMLDestroyE(&UE);
      }  /* END for (uindex) */

    /* group elements */

    MUHTIterInit(&HTIter);

    while (MUHTIterate(&MGroupHT,NULL,(void **)&Cred,NULL,&HTIter) == SUCCESS)
      {
      mxml_t   *GE;

      __MUIXMLCredLimits(&GE,(char *)MXO[mxoGroup],"Group",Cred->Name,&Cred->L,pindex);

      /* if limit(s) found then add the child element to the partition element, otherwise, destroy it */

      if ((GE->CCount > 0) || (GE->ACount > 1))
        MXMLAddE(ParE,GE);
      else
        MXMLDestroyE(&GE);
      }  /* END for (gindex) */

    /* account elements */

    MUHTIterInit(&HTIter);

    while (MUHTIterate(&MAcctHT,NULL,(void **)&Cred,NULL,&HTIter) == SUCCESS)
      {
      mxml_t   *AE;

      __MUIXMLCredLimits(&AE,(char *)MXO[mxoAcct],"Account",Cred->Name,&Cred->L,pindex);

      /* if limit(s) found then add the child element to the partition element, otherwise, destroy it */

      if ((AE->CCount > 0) || (AE->ACount > 1))
        MXMLAddE(ParE,AE);
      else
        MXMLDestroyE(&AE);
      }  /* END for (aindex) */

    /* if child elements were added to the partition element or if attributes
       were added to the partition element then add the partition element to
       the scheduler element, otherwise, destroy the partition element. */

    if ((ParE->CCount > 0) || (ParE->ACount > 1))
      {
      MXMLAddE(*SEPtr,ParE);
      }
    else
      {
      MXMLDestroyE(&ParE);
      }
    }   /* END for (pindex) */

  return(SUCCESS);
  }  /* END __MUIGetLimitsXMLMString() */



/**
 * Process credential query ('mcredctl -q') request.
 *
 * @see MCredProfileToXML() - child
 * @see MUICredCtl() - parent
 * @see MUICredDiagnose() - child
 * @see MQOSShow() - child
 * @see MCredProfileToXML() - child - process 'mcredctl -q profile'
 * @see MUIShowCConfig() - child
 *
 * @param S (I) [modified]
 * @param Auth (I)
 * @param AuthU (I)
 * @param OIndex (I)
 * @param OName (I)
 * @param ArgString (I)
 * @param OptString (I)
 * @param SName (I) [optional]
 * @param WhereValue (I)
 * @param DE (O) [modified]
 * @param DFlags (I) [bitmap of enum MCModeEnum]
 */

int __MUICredCtlQuery(

  msocket_t          *S,
  char               *Auth,
  mgcred_t           *AuthU,
  enum MXMLOTypeEnum  OIndex,
  char               *OName,
  char               *ArgString,
  char               *OptString,
  char               *SName,
  char               *WhereValue,
  mxml_t             *DE,
  mbitmap_t          *DFlags)

  {
  mgcred_t *U;
  void     *O;

  mbool_t IsAdmin;

  mhashiter_t HTI;

  char    tmpLine[MMAX_LINE];

  const char *FName = "__MUICredCtlQuery";

  MDB(4,fUI) MLog("%s(S,Auth,AuthU,OIndex,OName,ArgString,%s,SName,WhereValue,DE,%ld)\n",
    FName,
    OptString,
    DFlags);

  MOGetObject(OIndex,OName,&O,mVerify);

  if (MUStrNCmpCI(ArgString,"policies",strlen("policies")) == SUCCESS)
    {
    /*Don't bother checking for admin authorization for policy queries.*/
    }
  else
    {

    if (MUICheckAuthorization(
          AuthU,
          NULL,
          (void *)O,
          OIndex,
          mcsCredCtl,
          mccmQuery,
          &IsAdmin,
          NULL,
          0) == FAILURE)
      {
      sprintf(tmpLine,"ERROR:    user '%s' is not authorized to run command\n",
        Auth);
  
      MUISAddData(S,tmpLine);
  
      return(FAILURE);
      }

    }

  if (!strcasecmp(ArgString,"role"))
    {
    int aindex;

    mbitmap_t AuthBM;

    char AuthString[MMAX_LINE];

    char  *APtr;
    int    ASpace;

    int    cindex;

    mxml_t *UE;

    /* determine role */

    if (OName[0] != '\0')
      {
      void   *O;
      void   *OP;

      void   *OE;

      char   *NameP;

      char    tmpLine[MMAX_LINE];

      mbool_t DoAll;

      /* Return data in XML where the client will format the data by setting the mcmXML bit. */
      /* NOTE:  The non-xml code should be removed later. */

      bmset(DFlags,mcmXML);    

      if (!strcasecmp(OName,"ALL"))
        {
        DoAll = TRUE;
        }
      else
        {
        DoAll = FALSE; 

        if (MUserFind(OName,NULL) == FAILURE)
          {
          /* no record exists for requestor, get default access */

          MSysGetAuth(OName,mcsNONE,0,&AuthBM);

          MUSNInit(&APtr,&ASpace,AuthString,sizeof(AuthString));

          for (aindex = mcalAdmin1;aindex <= mcalAdmin5;aindex++)
            {
            if (!bmisset(&AuthBM,aindex))
              continue;

            MUSNPrintF(&APtr,&ASpace,"%sadmin%d",
              (AuthString[0] != '\0') ? "," : "",
              aindex - mcalAdmin1 + 1);
            }  /* END for (aindex) */

          /* Using MCLASS_FIRST_INDEX to skip DEFAULT class */
          for (cindex = MCLASS_FIRST_INDEX;cindex < MMAX_CLASS;cindex++)
            {
            if (MClass[cindex].Name[0] == '\0')
              break;

            if ((MClass[cindex].Name[0] == '\1') || (MClass[cindex].F.ManagerU == NULL))
              continue;

            if (MCredFindManager(OName,MClass[cindex].F.ManagerU,NULL) == SUCCESS)
              {
              if (AuthString[0] != '\0')
                MUSNCat(&APtr,&ASpace,",");

              MUSNPrintF(&APtr,&ASpace,"manager:class:%s",
                MClass[cindex].Name);
              }
            }  /* END for (cindex) */

          if (bmisset(DFlags,mcmXML))
            {
            UE = NULL;

            MXMLCreateE(&UE,(char *)MXO[mxoUser]);

            MXMLSetAttr(UE,(char *)MCredAttr[mcaID],(void *)OName,mdfString);
            MXMLSetAttr(UE,"role",(void *)AuthString,mdfString);

            MXMLAddE(DE,UE);
            }
          else
            {
            snprintf(tmpLine,sizeof(tmpLine),"%-12.12s   role=%s\n",
              OName,
              AuthString);

            MUISAddData(S,tmpLine);
            }
          }    /* END if (MUserFind(OName,NULL) == FAILURE) */
        }      /* END else (!strcasecmp(OName,"ALL")) */

      MOINITLOOP(&OP,mxoUser,&OE,&HTI);

      while ((O = MOGetNextObject(&OP,mxoUser,OE,&HTI,&NameP)) != NULL)
        {
        if ((NameP == NULL) ||
            !strcmp(NameP,NONE) ||
            !strcmp(NameP,"NOGROUP"))
          {
          continue;
          }

        if ((DoAll == FALSE) && strcmp(OName,NameP))
          continue;

        U = (mgcred_t *)O;

        MSysGetAuth(U->Name,mcsNONE,0,&AuthBM);

        MUSNInit(&APtr,&ASpace,AuthString,sizeof(AuthString));

        for (aindex = mcalAdmin1;aindex <= mcalAdmin5;aindex++)
          {
          if (!bmisset(&AuthBM,aindex))
            continue;

          MUSNPrintF(&APtr,&ASpace,"%sadmin%d",
            (AuthString[0] != '\0') ? "," : "",
            aindex - mcalAdmin1 + 1);
          }  /* END for (aindex) */ 

        /* Using MCLASS_FIRST_INDEX to skip DEFAULT class */
        for (cindex = MCLASS_FIRST_INDEX;cindex < MMAX_CLASS;cindex++)
          {
          if (MClass[cindex].Name[0] == '\0')
            break;

          if ((MClass[cindex].Name[0] == '\1') || (MClass[cindex].F.ManagerU == NULL))
            continue;

          if (MCredFindManager(U->Name,MClass[cindex].F.ManagerU,NULL) == SUCCESS)
            {
            if (AuthString[0] != '\0')
              MUSNCat(&APtr,&ASpace,",");

            MUSNPrintF(&APtr,&ASpace,"manager:class:%s",
              MClass[cindex].Name);
            }
          }  /* END for (cindex) */

        if (bmisset(DFlags,mcmXML))
          {
          UE = NULL;

          MXMLCreateE(&UE,(char *)MXO[mxoUser]);

          MXMLSetAttr(UE,(char *)MCredAttr[mcaID],(void *)U->Name,mdfString);
          MXMLSetAttr(UE,"role",(void *)AuthString,mdfString);
   
          MXMLAddE(DE,UE);
          }
        else
          {
          snprintf(tmpLine,sizeof(tmpLine),"%-12.12s   role=%s\n",
            U->Name,
            AuthString);

          MUISAddData(S,tmpLine);
          }
        }  /* END if ((OName[0] != '\0') && ...) */
      }    /* END if (OName[0] != '\0') */

    if (bmisset(DFlags,mcmVerbose))
      {
      int sindex;

      char tmpLine[MMAX_LINE];

      char *BPtr;
      int   BSpace;

      /* display role breakdown */

      for (aindex = mcalAdmin1;aindex <= mcalAdmin5;aindex++)
        {
        UE = NULL;

        MXMLCreateE(&UE,"admin");
 
        MXMLSetAttr(
          UE,
          (char *)MCredAttr[mcaID],
          (void *)MRoleType[aindex - mcalAdmin1 + 1],
          mdfString);
 
        MXMLSetAttr(
          UE,
          (char *)MAdminAttr[madmaName],
          MSched.Admin[aindex - mcalAdmin1 + 1].Name,
          mdfString);
 
        MUSNInit(&BPtr,&BSpace,tmpLine,sizeof(tmpLine));

        for (sindex = 1;MUI[sindex].SName != NULL;sindex++)
          {
          if (MUI[sindex].AdminAccess[aindex - mcalAdmin1 + 1] == FALSE)
            continue;

          if (tmpLine[0] != '\0')
            MUSNCat(&BPtr,&BSpace,",");

          MUSNCat(&BPtr,&BSpace,MUI[sindex].SName);
          }  /* END for (sindex) */

        MXMLSetAttr(UE,"services",(void *)tmpLine,mdfString);

        MXMLAddE(DE,UE);
        }  /* END for (aindex) */ 
      }    /* END if (bmisset(DFlags,mcmVerbose)) */
    }      /* END if (!strcmp(ArgString,"role")) */
  else if (!strcmp(ArgString,"config")) 
    {
    if (bmisset(DFlags,mcmXML))
      {
      MUIShowCConfig(OName,OIndex,DE,NULL,NULL);
      }
    else
      {
      mstring_t tmpBuf(MMAX_LINE);

      switch (OIndex)
        {
        case mxoAcct:
        case mxoGroup:
        case mxoUser:

          MUICredDiagnose(
            Auth,
            NULL,
            &tmpBuf,
            OIndex,
            OName,
            (bmisset(DFlags,mcmVerbose)) ? TRUE : FALSE);

          break;

        case mxoClass:

          MClassShow(Auth,OName,NULL,&tmpBuf,DFlags,FALSE,mfmHuman);

          break;

        case mxoQOS:

          MQOSShow(Auth,OName,&tmpBuf,0,mfmHuman);

          break;

        default:

          /* NO-OP */

          break;
        } /* END switch (OIndex) */

      if (!MUStrIsEmpty(tmpBuf.c_str()))
        MUISAddData(S,tmpBuf.c_str());
      }   /* END else */
    }     /* END if (!strcmp(ArgString,"config")) */
  else if (!strcmp(ArgString,"profile"))
    {
    long STime = 0;
    long ETime = MMAX_TIME;

    mbitmap_t MStatAttrsBM;

    if (strstr(OptString,"time:"))
      {
      MStatGetRange(&OptString[strlen("time:")],&STime,&ETime);
      }
    else
      {
      MUGetPeriodRange(MSched.Time,0,0,mpDay,&STime,NULL,NULL);
      }

    /* Check for user-specified attribute types list (Optional)*/

    if (strstr(OptString,"types:"))  /* User has specified a specific list, parse and create bitmap */
      {
      char  tmpLine[MMAX_LINE];
      char *ptr;
      char *TokPtr = NULL;
      enum  MStatAttrEnum sindex;

      MUStrCpy(tmpLine,strstr(OptString,"types:"),sizeof(tmpLine));

      ptr = MUStrTok(tmpLine,":",&TokPtr);
      ptr = MUStrTok(NULL,",",&TokPtr);

      while (ptr != NULL)
        {
        sindex = (enum MStatAttrEnum)MUGetIndexCI(ptr,MStatAttr,FALSE,mstaNONE);
        bmset(&MStatAttrsBM,sindex);

        ptr = MUStrTok(NULL,",",&TokPtr);
        } /* END while (ptr != null) */
      } /* END if types: specified */
    else 
      {
      int aindex;
      /* Add all statistic attributes to bitmap */
      for (aindex = 0;aindex != mstaLAST;aindex++)
        {
        bmset(&MStatAttrsBM,aindex);
        }
      } /* END else (no user specified attribute list */

    /* never allow query beyond now */

    ETime = MIN((mulong)ETime,MIN((mulong)(MStat.P.IStatEnd - 2),MSched.Time));

    if (ETime < STime)
      {
      /* bad times */

      return(SUCCESS);
      }

    MCredProfileToXML(
      OIndex,
      OName,
      SName,
      STime,
      ETime,
      -1,
      &MStatAttrsBM,
      FALSE,
      TRUE,
      DE);
    }    /* END else if (!strcmp(ArgString,"profile")) */
  else if (!strcmp(ArgString,"limit") || !strcmp(ArgString,"all"))
    {
    mhashiter_t HTI;

    void   *O;
    void   *OP;

    void   *OE;

    char   *NameP;

    enum MLimitAttrEnum AList[] = {
      mlaAJobs,
      mlaANodes,
      mlaAProcs,
      mlaAPS,
      mlaHLAJobs,
      mlaHLANodes,
      mlaHLAProcs,
      mlaHLAPS, 
      mlaHLIJobs,
      mlaHLINodes,
      mlaSLAJobs,
      mlaSLANodes,
      mlaSLAProcs,
      mlaSLAPS,
      mlaSLIJobs,
      mlaSLINodes,
      mlaNONE };

    mxml_t *CE;
    mxml_t *LE;

    mcredl_t *LPtr;

    if (OName[0] == '\0')
      {
      MOINITLOOP(&OP,OIndex,&OE,&HTI);

      while ((O = MOGetNextObject(&OP,OIndex,OE,&HTI,&NameP)) != NULL)
        {
        if ((NameP == NULL) ||
            !strcmp(NameP,NONE) ||
            !strcmp(NameP,ALL) ||
            !strcmp(NameP,"ALL") ||
            !strcmp(NameP,"NOGROUP"))
          {
          continue;
          }

        if (!strcmp(NameP,"DEFAULT"))
          {
          /* NOTE:  currently display DEFAULT for all credtypes */

          /* should credtype defaults be displayed only if usage is accrued? */

          /* NYI */
          }

        if (MOGetComponent(O,OIndex,(void **)&LPtr,mxoxLimits) == FAILURE)
          {
          continue;
          }

        if (!strcmp(ArgString,"all"))
          {
          /* don't show limits */

          MUIShowCConfig(NameP,OIndex,DE,&CE,AList);
          }
        else
          {
          CE = NULL;
          LE = NULL;

          MXMLCreateE(&CE,(char *)MXO[OIndex]);

          MXMLSetAttr(CE,(char *)MCredAttr[mcaID],(void *)NameP,mdfString);

          MXMLAddE(DE,CE);

          /* collect hard and soft limits and resource usage */

          MLimitToXML(LPtr,&LE,(enum MLimitAttrEnum *)AList);

          MXMLAddE(CE,LE);
          }  /* END while ((O = MOGetNextObject() != NULL) */
        }
      }    /* END if (OName[0] == '\0') */
    else
      {
      if (!strcmp(ArgString,"all"))
        {
        /* FIXME: don't show limits */

        MUIShowCConfig(OName,OIndex,DE,NULL,AList);
        }
      else
        {
        CE = NULL;

        MXMLCreateE(&CE,(char *)MXO[OIndex]);

        MXMLSetAttr(CE,(char *)MCredAttr[mcaID],(void *)OName,mdfString);

        MXMLAddE(DE,CE);

        if ((MOGetObject(OIndex,OName,&O,mVerify) == SUCCESS) &&
            (MOGetComponent(O,OIndex,(void **)&LPtr,mxoxLimits) == SUCCESS))
          {
          LE = NULL;

          /* collect hard and soft limits and resource usage */

          MLimitToXML(LPtr,&LE,(enum MLimitAttrEnum *)AList);

          MXMLAddE(CE,LE);
          }
        }
      }
    }      /* END else if (!strcmp(ArgString,"limit")) */
  else if (!strcmp(ArgString,"accessto") ||
           !strcmp(ArgString,"accessfrom"))
    {
    mxtree_t CTree;

    mtree_t  tmpTBuf[MMAX_CRED];

    void   *O;
    void   *OP;

    void   *OE;

    int     Depth;

    char   *NameP;

    char   *ptr;

    mbool_t AccessTo;

    mhashiter_t HTI;

    /* NOTE:  accessto: who can access this credential
              accessfrom: what this credential has access to  */

    if ((ptr = strstr(OptString,"depth=")) != NULL)
      {
      ptr += strlen("depth=");

      Depth = (int)strtol(ptr,NULL,10);
      }
    else
      {
      Depth = 1;
      }

    if (!strcmp(ArgString,"accessto"))
      {
      AccessTo = TRUE;
      }
    else
      {
      AccessTo = FALSE;
      }

    if (OName[0] == '\0')
      {
      switch (OIndex)
        {
        case mxoUser:
        case mxoGroup:
        case mxoAcct:
       
          {
          mhash_t *HT;
          mhashiter_t HTIter;

          MUHTIterInit(&HTIter);

          if (OIndex == mxoUser)
            HT = &MUserHT;
          else if (OIndex == mxoGroup)
            HT = &MGroupHT;
          else
            HT = &MAcctHT;

          while (MUHTIterate(HT,NULL,&O,NULL,&HTIter) == SUCCESS)
            {
            NameP = ((mgcred_t *)O)->Name;

            if ((NameP == NULL) ||
                !strcmp(NameP,NONE) ||
                !strcmp(NameP,ALL) ||
                !strcmp(NameP,"ALL") ||
                !strcmp(NameP,"DEFAULT") ||
                !strcmp(NameP,"NOGROUP"))
              {
              continue;
              }
     
            CTree.Root     = NULL;
            CTree.Buf      = tmpTBuf;
            CTree.BufSize  = MMAX_CRED;
            CTree.BufIndex = 0;
     
            MUIOShowAccess(O,OIndex,NameP,AccessTo,Depth,&CTree,DE);
            }  /* END while (MUHTIterate()) */
          }  /* END case */

          break;

        default:

          MOINITLOOP(&OP,OIndex,&OE,&HTI);
     
          while ((O = MOGetNextObject(&OP,OIndex,OE,&HTI,&NameP)) != NULL)
            {
            if ((NameP == NULL) ||
                !strcmp(NameP,NONE) ||
                !strcmp(NameP,ALL) ||
                !strcmp(NameP,"ALL") ||
                !strcmp(NameP,"DEFAULT") ||
                !strcmp(NameP,"NOGROUP"))
              {
              continue;
              }
     
            CTree.Root     = NULL;
            CTree.Buf      = tmpTBuf;
            CTree.BufSize  = MMAX_CRED;
            CTree.BufIndex = 0;
     
            MUIOShowAccess(O,OIndex,NameP,AccessTo,Depth,&CTree,DE);
            }  /* END while ((O = MOGetNextObject() != NULL) */

          break;
 
        }  /* END switch (OIndex) */
      }    /* END if (OName[0] == '\0') */
    else
      {
      if (MOGetObject(OIndex,OName,&O,mVerify) == SUCCESS)
        {
        CTree.Root     = NULL;
        CTree.Buf      = tmpTBuf;
        CTree.BufSize  = MMAX_CRED;
        CTree.BufIndex = 0;

        MUIOShowAccess(O,OIndex,OName,AccessTo,Depth,&CTree,DE);
        }
      }    /* END else (OName[0] == '\0') */
    }      /* END else if (!strcmp(ArgString,"access")) */
  else if (!strcmp(ArgString,"policies"))
    {
    mxml_t *CE;

    __MUIGetLimitsXMLMString(&CE);

    MXMLAddE(DE,CE);
    } /* END else if (!strcmp(ArgString,"policies")) */

  return(SUCCESS);
  }        /* END __MUICredCtlQuery() */


/**
 * Process cred control (mcredctl) request.
 *
 * @see __MUICredCtlQuery() - child
 * @see MCredCreate() - child  - create new object
 *
 * @param S (I) [modified]
 * @param CFlags (I) [bitmap of enum MRoleEnum]
 * @param Auth (I)
 */

int MUICredCtl(

  msocket_t *S,       /* I (modified) */
  mbitmap_t *CFlags,  /* I (bitmap of enum MRoleEnum) */
  char      *Auth)    /* I */

  {
  char OString[MMAX_LINE];
  char AString[MMAX_LINE];
  char tmpLine[MMAX_LINE];

  char FlagString[MMAX_LINE];
  char ArgString[MMAX_LINE];
  char OptString[MMAX_LINE];

  enum MXMLOTypeEnum   OIndex[mxoALL + 1] = {mxoNONE};
  enum MCredCtlCmdEnum AIndex;

  char tmpName[MMAX_LINE];
  char tmpValue[MMAX_LINE];

  char OName[MMAX_LINE];
  char SName[MMAX_NAME];
  char PName[MMAX_NAME];

  char User[MMAX_NAME];

  double Allocation;
 
  mbitmap_t DFlags;   /* bitmap of enum MCM* */

  int rc = SUCCESS; /* return code */

  mgcred_t *AuthU;

  mxml_t *RE = NULL;
  mxml_t *DE;

  const char *FName = "MUICredCtl";

  MDB(2,fUI) MLog("%s(S,%ld,%s)\n",
    FName,
    CFlags,
    (Auth != NULL) ? Auth : "NULL");

  if (S == NULL)
    {
    return(FAILURE);
    }

  /* get authorization */

  MUserAdd(Auth,&AuthU);

  MUISClear(S);

  OName[0] = '\0';
  SName[0] = '\0';
  PName[0] = '\0';
  User[0]  = '\0';

  Allocation = -1.0;

  /* TEMP:  force to XML */

  S->WireProtocol = mwpS32;

  switch (S->WireProtocol)
    {
    case mwpS32:

      {
      int STok;
      int WTok;
      int rc;

      char *ptr;
      char *TokPtr;

      int oindex;

      /* Request Format:  <Request object="Queue"/>... */
      /* Response Format: <Data msg="output truncated\nRM unavailable\n"><queue type="active"><job name="X" state="X" ...></job></queue></Data> */

      RE = S->RDE;

      rc = MS3GetObject(RE,OString);

      if (rc == SUCCESS)
        {
        ptr = MUStrTok(OString,",",&TokPtr);

        oindex = 0;

        while (ptr != NULL)
          {
          if ((OIndex[oindex] = (enum MXMLOTypeEnum)MUGetIndexCI(
                 ptr,
                 MXO,
                 FALSE,
                 mxoNONE)) == mxoNONE)
            {
            rc = FAILURE;
 
            break;
            }

          if (oindex >= mxoALL)
            break;

          ptr = MUStrTok(NULL,",",&TokPtr);

          oindex++;
          }

        OIndex[oindex] = mxoNONE;
        }

      if (rc == FAILURE) 
        {
        MDB(3,fUI) MLog("WARNING:  corrupt command '%.128s' received\n",
          S->RBuffer);

        MUISAddData(S,"ERROR:    corrupt command received\n");

        return(FAILURE);
        }

      if ((MXMLGetAttr(RE,MSAN[msanAction],NULL,AString,0) == FAILURE) ||
         ((AIndex = (enum MCredCtlCmdEnum)MUGetIndexCI(
             AString,
             MCredCtlCmds,
             FALSE,
             mccmNONE)) == mccmNONE))
        {
        MDB(3,fUI) MLog("WARNING:  corrupt command '%100.100s' received\n",
          S->RBuffer);

        MUISAddData(S,"ERROR:    corrupt command received\n");

        return(FAILURE);
        }

      MXMLGetAttr(RE,MSAN[msanFlags],NULL,FlagString,sizeof(FlagString));
      MXMLGetAttr(RE,MSAN[msanArgs],NULL,ArgString,sizeof(ArgString));
      MXMLGetAttr(RE,MSAN[msanOption],NULL,OptString,sizeof(OptString));
      MXMLGetAttr(RE,"partition",NULL,PName,sizeof(PName));

      bmfromstring(FlagString,MClientMode,&DFlags);

      WTok = -1;

      while (MS3GetWhere(
          S->RDE,
          NULL,
          &WTok,
          tmpName,
          sizeof(tmpName),
          tmpValue,
          sizeof(tmpValue)) == SUCCESS)
        {
        if (!strcmp(tmpName,"Id"))
          strcpy(OName,tmpValue);
        else if (!strcmp(tmpName,"Stat"))
          strcpy(SName,tmpValue);
        }

      STok = -1;

      while (MS3GetSet(
          S->RDE,
          NULL,
          &STok,
          tmpName,
          sizeof(tmpName),
          tmpValue,
          sizeof(tmpValue)) == SUCCESS)
        {
        if (!strcmp(tmpName,"user"))
          strcpy(User,tmpValue);
        else if (!strcmp(tmpName,"allocation"))
          Allocation = strtod(tmpValue,NULL);
        }
      }  /* END BLOCK */

      break;

    default:

      MUISAddData(S,"ERROR:    format not supported\n");

      return(FAILURE);

      break;
    }  /* END switch (S->WireProtocol) */

  /* create parent element */

  if (MXMLCreateE(&S->SDE,(char *)MSON[msonData]) == FAILURE)
    {
    MUISAddData(S,"ERROR:    cannot create response\n");

    return(FAILURE);
    }

  DE = S->SDE;

  /* process request */

  switch (AIndex)
    {
    case mccmCreate:

      if (MUICheckAuthorization(
          AuthU,
          NULL,
          NULL,
          OIndex[0],
          mcsCredCtl,
          mccmDestroy,
          NULL,
          NULL,
          0) == FAILURE)
        {
        sprintf(tmpLine,"ERROR:    user '%s' is not authorized to run command\n",
          Auth);

        MUISAddData(S,tmpLine);

        return(FAILURE);
        }

      if (MCredCreate(OIndex[0],OName,User,Allocation,tmpLine) == FAILURE)
        {
        sprintf(tmpLine,"ERROR:    unable to create %s %s\n",
         MXO[OIndex[0]],
         OName);

        MUISAddData(S,tmpLine);

        return(FAILURE);
        }

      sprintf(tmpLine,"successfully created %s %s\n",
        MXO[OIndex[0]],
        OName);

      MUISAddData(S,tmpLine);
 
      break;

    case mccmDestroy:

      {
      mhashiter_t HTI;

      char tmpLine[MMAX_LINE];
      char tmpMsg[MMAX_LINE];

      char *BPtr;
      int   BSpace;

      void *O;
      void *OP;

      void *OE;

      char *NameP;

      MUSNInit(&BPtr,&BSpace,tmpLine,sizeof(tmpLine)); 

      if (OName[0] == '\0')
        O = NULL;
      else
        MOGetObject(OIndex[0],OName,&O,mVerify);

      if (MUICheckAuthorization(
          AuthU,
          NULL,
          (void *)O,
          OIndex[0],
          mcsCredCtl,
          mccmDestroy,
          NULL,
          NULL,
          0) == FAILURE)
        {
        sprintf(tmpLine,"ERROR:    user '%s' is not authorized to run command\n",
          Auth);

        MUISAddData(S,tmpLine);

        return(FAILURE);
        }

      MOINITLOOP(&OP,OIndex[0],&OE,&HTI);

      while ((O = MOGetNextObject(&OP,OIndex[0],OE,&HTI,&NameP)) != NULL)
        {
        if ((NameP == NULL) ||
            !strcmp(NameP,NONE) ||
            !strcmp(NameP,ALL) ||
            !strcmp(NameP,"ALL") ||
            !strcmp(NameP,"DEFAULT") ||
            !strcmp(NameP,"NOGROUP"))
          {
          continue;
          }

        if ((OName[0] != '\0') &&
            (strcmp(OName,ALL)) &&
            (strcmp(OName,"ALL")) &&
            (strcmp(OName,NameP)))
          continue;

        if (MCredPurge(OIndex[0],O,tmpMsg) == SUCCESS)
          {
          MUSNPrintF(&BPtr,&BSpace,"%s:%s successfully purged\n",
            MXO[OIndex[0]],
            NameP);
          }
        else
          {
          MUSNPrintF(&BPtr,&BSpace,"%s:%s could not be purged (%s)\n",
            MXO[OIndex[0]],
            NameP,
            tmpMsg);
          }
        }

      MUSNCat(&BPtr,&BSpace,"please restart for changes to take effect\n");

      MUISAddData(S,tmpLine);
      }  /* END BLOCK (case mccmDestroy) */

      break;

    case mccmList:

      {
      mbitmap_t BM;

      void   *O;
      void   *OP;

      void   *OE;

      char   *NameP;

      mxml_t *CE;

      int     oindex;

      mhashiter_t HTI;

      const enum MCredAttrEnum CAList[] = {
        mcaID,
        mcaNONE };

      const enum MXMLOTypeEnum CCList[] = {
        mxoNONE };

      bmset(&BM,mcmVerbose);

      for (oindex = 0;oindex < mxoALL;oindex++)
        {
        if (OIndex[oindex] == mxoNONE)
          break;

        MOINITLOOP(&OP,OIndex[oindex],&OE,&HTI);

        while ((O = MOGetNextObject(&OP,OIndex[oindex],OE,&HTI,&NameP)) != NULL)
          {
          if ((NameP == NULL) ||
              !strcmp(NameP,NONE) ||
              !strcmp(NameP,ALL) ||
              !strcmp(NameP,"ALL") ||
              !strcmp(NameP,"NOGROUP"))
            {
            continue;
            }

          if ((OIndex[oindex] != mxoPar) &&
              (OIndex[oindex] != mxoUser) &&
              (OIndex[oindex] != mxoGroup) &&
              (OIndex[oindex] != mxoAcct) &&
              (OIndex[oindex] != mxoClass) &&
              (OIndex[oindex] != mxoQOS) &&
              !strcmp(NameP,"DEFAULT"))
            {
            continue;
            }

          if ((ArgString[0] != '\0') && strcmp(ArgString,NONE))
            {
            if (strcmp(ArgString,NameP))
              continue;
            }

          if (MUICheckAuthorization(
                AuthU,
                NULL,
                (void *)O,
                OIndex[oindex],
                mcsCredCtl,
                mccmList,
                NULL,
                NULL,
                0) == FAILURE)
            {
            sprintf(tmpLine,"ERROR:    user '%s' is not authorized to use command\n",
              Auth);

            MUISAddData(S,tmpLine);
            /* Set command line errno for failure */
            rc = FAILURE;
            continue;
            }

          CE = NULL;

          MCOToXML(
            O,
            OIndex[oindex],
            &CE,
            (void *)CAList,
            (enum MXMLOTypeEnum *)CCList,
            NULL,
            TRUE,
            &BM);

          MXMLAddE(DE,CE);
          }  /* END while() */
        }    /* END for (oindex) */
      }      /* END BLOCK (case mccmList) */

      break;

    case mccmModify:

      {
      /* process 'mcredctl -m' request */

      void *O;

      must_t *SPtr;

      char tmpLine[MMAX_LINE];
      char tmpSNLine[MMAX_LINE];
      char tmpSVLine[MMAX_LINE];

      if (OName[0] == '\0')
        O = NULL;
      else
        MOGetObject(OIndex[0],OName,&O,mVerify);

      if (MUICheckAuthorization(
            AuthU,
            NULL,
            (void *)O,
            OIndex[0],
            mcsCredCtl,
            mccmModify,
            NULL,
            NULL,
            0) == FAILURE)
        {
        snprintf(tmpLine,sizeof(tmpLine),"ERROR:    user '%s' is not authorized to modify %s %s\n",
          Auth,
          MXO[OIndex[0]],
          OName);

        MUISAddData(S,tmpLine);

        return(FAILURE);
        }

      /* extract set */

      MS3GetSet(
        RE,
        NULL,
        NULL,
        tmpSNLine,
        sizeof(tmpSNLine),
        tmpSVLine,
        sizeof(tmpSVLine));

      if ((MOGetObject(OIndex[0],OName,&O,mVerify) == FAILURE) ||
          (MOGetComponent(O,OIndex[0],(void **)&SPtr,mxoxStats) == FAILURE))
        {
        /* credential id not specified */

        MUISAddData(S,"credential not specified");

        return(FAILURE);
        }

      /* make change */

      /* support profile, hold, message, credits, etc */

      if (!strcasecmp(tmpSNLine,"profile"))
        {
        if (O == NULL)
          {
          return(FAILURE);
          }

        if (MUBoolFromString(tmpSVLine,FALSE) == FALSE)
          {
          /* disable profiling */

          if (SPtr->IStat != NULL)
            MStatPDestroy((void ***)&SPtr->IStat,msoCred,MStat.P.MaxIStatCount);
          }
        else
          {
          if (SPtr->IStat == NULL)
            MStatPInitialize((void *)SPtr,FALSE,msoCred);
          }
        }    /* END if (!strcasecmp(tmpSNLine,"profile")) */
      else if (!strcasecmp(tmpSNLine,"hold"))
        {
        if (O == NULL)
          {
          return(FAILURE);
          }  

        MCredSetAttr(O,OIndex[0],mcaHold,NULL,(void **)tmpSVLine,NULL,mdfString,mSet);

        snprintf(tmpLine,sizeof(tmpLine),"holds adjusted for %s %s\n",
          MXO[OIndex[0]],
          OName);

        MUISAddData(S,tmpLine);
        }    /* END else if (!strcasecmp(tmpSNLine,"hold")) */
      else if (!strcasecmp(tmpSNLine,"message"))
        {
        if (O == NULL)
          {
          return(FAILURE);
          }

        MCredSetAttr(O,OIndex[0],mcaMessages,NULL,(void **)tmpSVLine,NULL,mdfString,mSet);

        snprintf(tmpLine,sizeof(tmpLine),"message adjusted for %s %s\n",
          MXO[OIndex[0]],
          OName);

        MUISAddData(S,tmpLine);
        }
      else if (!strcasecmp(tmpSNLine,"credits"))
        {
        /* NYI */ 
        }
      else if (!strcasecmp(tmpSNLine,"config"))
        {
        char *BPtr;
        int   BSpace;

        char  ConfigBuf[MMAX_BUFFER];

        char *ptr;
        char *TokPtr;

        char  Msg[MMAX_LINE];

        if (MCredCfgParm[OIndex[0]] == NULL)
          {
          char tmpLine[MMAX_LINE];

          snprintf(tmpLine,sizeof(tmpLine),"cannot modify object type %s\n",
            MXO[OIndex[0]]);

          MUISAddData(S,tmpLine);

          break;
          }

        MUSNInit(&BPtr,&BSpace,ConfigBuf,sizeof(ConfigBuf));

        MUSNPrintF(&BPtr,&BSpace,"%s[%s] %s\n",
          MCredCfgParm[OIndex[0]],
          OName,
          tmpSVLine);

        ptr = ConfigBuf;

        MCfgAdjustBuffer(&ptr,FALSE,NULL,FALSE,TRUE,FALSE);

        ptr = MUStrTok(ptr,"\n",&TokPtr);

        if (MSysReconfigure(
               ptr,              /* I */
               TRUE,             /* I */
               FALSE,            /* I - eval only */
               FALSE,            /* I - modify master */
               1,                /* I */
               Msg) == SUCCESS)  /* O */
          {
          MDB(2,fUI) MLog("INFO:     config line '%.100s' successfully processed\n",
            ptr);

          /* NYI */
          }
        else
          {
          char tmpLine[MMAX_LINE];

          snprintf(tmpLine,sizeof(tmpLine),"config adjusted for %s %s\n",
            MXO[OIndex[0]],
            OName);

          MUISAddData(S,tmpLine);
          }
        }    /* END else if (!strcasecmp(tmpSNLine,"config")) */
      else if (!strcasecmp(tmpSNLine,"trigger"))
        {
        /* NYI */
        }
      }      /* END BLOCK (case mccmModify) */

      break;

    case mccmQuery:

      {
      int oindex;

      for (oindex = 0;oindex < mxoALL;oindex++)
        {
        if (OIndex[oindex] == mxoNONE)
          break;

        if (__MUICredCtlQuery(
            S,
            Auth,
            AuthU,
            OIndex[oindex],
            OName,
            ArgString,
            OptString,
            SName,
            NULL,                 /* where value - not used */
            DE,                   /* O */
            &DFlags) == FAILURE)
          {
            rc = FAILURE;
          }
        }    /* END for (oindex) */
      }      /* END BLOCK (case mccmQuery) */

      break;

    case mccmReset:

      {
      void   *OE;
      void   *OP;
      void   *O;

      char   *NameP;
      char    tmpLine[MMAX_LINE];

      int     oindex;

      mhashiter_t HTI;

      enum mccmResetCmdEnum {
        mcrcALL,
        mcrcStats,
        mcrcFairShare,
        mcrcNONE };

      enum mccmResetCmdEnum CmdMode;

      if (strstr(ArgString,"stat") != NULL)
        {
        CmdMode = mcrcStats;
        }
      else if (strstr(ArgString,"fairshare") != NULL)
        {
        CmdMode = mcrcFairShare;
        }
      else if (strstr(ArgString,"all") != NULL)
        {
        CmdMode = mcrcALL;
        }
      else
        {
        CmdMode = mcrcNONE;
        }

      for (oindex = 0;OIndex[oindex] != mxoNONE;oindex++)
        {
        MOINITLOOP(&OP,OIndex[oindex],&OE,&HTI);

        while ((O = MOGetNextObject(&OP,OIndex[oindex],OE,&HTI,&NameP)) != NULL)
          {
          if ((NameP == NULL) ||
              !strcmp(NameP,NONE) ||
              !strcmp(NameP,ALL) ||
              !strcmp(NameP,"ALL") ||
              !strcmp(NameP,"DEFAULT") ||
              !strcmp(NameP,"NOGROUP"))
            {
            continue;
            }

          if ((OName != NULL) &&
              (strcmp(OName,ALL)) &&
              (strcmp(OName,"ALL")) &&
              (strcmp(OName,NameP)))
            continue;

          switch (CmdMode)
            {
            case mcrcALL:
            case mcrcStats:

              switch (OIndex[oindex])
                {
                default:

                  {
                  must_t *SPtr;

                  if (MOGetComponent(O,OIndex[oindex],(void **)&SPtr,mxoxStats) == FAILURE)
                    {
                    continue;
                    }

                  if ((SPtr->IStat != NULL) &&
                      (MStatPDestroy((void ***)&SPtr->IStat,msoCred,MStat.P.MaxIStatCount) != FAILURE))
                    {
                    MStatPInitialize((void *)SPtr,FALSE,msoCred);

                    snprintf(tmpLine,sizeof(tmpLine),"INFO:  profiling statistics reset for %s:%s\n",
                      MXO[OIndex[oindex]],
                      NameP);

                    MUISAddData(S,tmpLine);
                    }
                  }  /* END BLOCK default */

                  break;
                }  /* END switch (OIndex[oindex]) */

              break;

            case mcrcFairShare:

              switch (OIndex[oindex])
                {
                case mxoUser:
                case mxoAcct:

                  /* note that the client makes sure that a partition name is supplied  in the case of fairshare */

                  if (MFSCredReset(OIndex[oindex],NameP,PName) == SUCCESS)
                    {
                    snprintf(tmpLine,sizeof(tmpLine),"INFO:  fairshare reset for %s:%s in partition %s\n",
                      MXO[OIndex[oindex]],
                      NameP,
                      PName);
                    }
                  else
                    {
                    snprintf(tmpLine,sizeof(tmpLine),"INFO:  fairshare not reset for %s:%s in partition %s\n",
                      MXO[OIndex[oindex]],
                      NameP,
                      PName);
                    }

                  MUISAddData(S,tmpLine);

                  break;

                default:

                  snprintf(tmpLine,sizeof(tmpLine),"INFO:  cannot reset fairshare info for %s:%s in partition %s\n",
                    MXO[OIndex[oindex]],
                    NameP,
                    PName);

                  MUISAddData(S,tmpLine);

                  break;
                }  /* END switch (OIndex[oindex]) */

              if (CmdMode != mcrcALL)
                break;

            default:

              /* NO-OP */

              break;
            } /* END switch (CmdMode) */
          }   /* END while (MOGetNextObject() != NULL) */
        }     /* END for (oindex) */
      }       /* END BLOCK (case mccmReset) */

      break;

    default:

      MUISAddData(S,"ERROR:    action not handled\n");

      return(FAILURE);

      break;
    }  /* END switch (OIndex) */

  return(rc);
  }  /* END MUICredCtl() */
/* END MUICredCtl.c */
