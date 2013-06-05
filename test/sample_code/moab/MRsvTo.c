/* HEADER */

      
/**
 * @file MRsvTo.c
 *
 * Contains: Rsv To various other structs/objects/formats
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 * Convert reservation to string.
 *
 * NOTE:  currently not called by any routine.
 *
 * @param R (I)
 * @param RC (I) [optional]
 * @param SBuf (O)
 * @param SBufSize (I)
 * @param RAList (I) [optional]
 */

int MRsvToString(

  mrsv_t *R,        /* I */
  int    *RC,       /* I (optional) */
  char   *SBuf,     /* O */
  int     SBufSize, /* I */
  enum MRsvAttrEnum *RAList) /* I (optional) */

  {
  mxml_t *E = NULL;

  if (SBuf != NULL)
    SBuf[0] = '\0';

  if ((R == NULL) || (SBuf == NULL))
    {
    return(FAILURE);
    }

  if (MRsvToXML(R,&E,RAList,NULL,FALSE,mcmNONE) == FAILURE)
    {
    return(FAILURE);
    }

  MXMLToString(E,SBuf,SBufSize,NULL,TRUE);

  MXMLDestroyE(&E);

  return(SUCCESS);
  }  /* END MRsvToString() */





/**
 *
 *
 * @param N (I)
 * @param R (I) - show only specified rsv [optional]
 * @param ShowFree (I) - show free resources as rsv
 * @param EP (O) [optional]
 * @param SBuf (O) [optional]
 * @param SBufSize (I)
 */

int MNRsvToString(

  mnode_t  *N,        /* I */
  mrsv_t   *R,        /* I (optional) - show only specified rsv */
  mbool_t   ShowFree, /* I - show free resources as rsv */
  mxml_t  **EP,       /* O (optional) */
  char     *SBuf,     /* O (optional) */
  int       SBufSize) /* I */

  {
  static int DNRAList[] = {
    mnraDRes,
    mnraEnd,
    mnraName,
    mnraState,
    mnraStart,
    mnraTC,
    mnraType,
    mnraNONE };

  static mrsv_t tR;
  static mbool_t tRInitialized = FALSE;

  mxml_t *NE  = NULL;
  mxml_t *NRE = NULL;

  if (EP != NULL)
    *EP = NULL;

  if ((N == NULL) ||
     ((SBuf == NULL) && (EP == NULL)))
    {
    return(FAILURE);
    }

  if (ShowFree == TRUE)
    {
    if (tRInitialized == FALSE)
      {
      strcpy(tR.Name,"free");
      tR.StartTime  = MSched.Time;
      tR.EndTime    = MMAX_TIME;
      tR.Type       = mrtUser;
      tR.DRes.Procs = -1;

      tRInitialized = TRUE;
      }

    tR.StartTime  = MSched.Time;

    if ((N->RE != NULL) && (N->RE->Type == mreStart))
      {
      tR.EndTime = N->RE->Time;
      }
    else
      {
      tR.EndTime = MMAX_TIME;
      }

    if (tR.EndTime <= MSched.Time)
      {
      return(FAILURE);
      }
    }    /* END if (ShowFree == TRUE) */

  if (MXMLCreateE(&NE,"node") == FAILURE)
    {
    return(FAILURE);
    }

  MXMLSetAttr(NE,"name",(void *)N->Name,mdfString);

  MXMLSetAttr(NE,"partition",(void *)MPar[N->PtIndex].Name,mdfString);

  if (ShowFree == TRUE)
    {
    NRE = NULL;

    if (MXMLCreateE(&NRE,"nres") == FAILURE)
      {
      return(FAILURE);
      }

    MNRsvToXML(N,&tR,NRE,(int *)DNRAList);

    MXMLAddE(NE,NRE);
    }
  else
    {
    mrsv_t *NR = NULL;

    RsvTable printedRsvs; /* map to keep track of which rsvs we've printed */

    mre_t *RE;

    for (RE = N->RE;RE != NULL;MREGetNext(RE,&RE))
      {
      if (RE->Type != mreStart)
        continue;

      NR = MREGetRsv(RE);

      if ((R != NULL) && (NR != R))
        continue;

      if (printedRsvs.find(NR->Name) != printedRsvs.end())
        continue;

      /* create XML */

      NRE = NULL;

      if (MXMLCreateE(&NRE,"nres") == FAILURE)
        {
        break;
        }

      MNRsvToXML(N,MREGetRsv(RE),NRE,(int *)DNRAList);

      printedRsvs.insert(std::pair<std::string,mrsv_t *>(NR->Name,NR));

      MXMLAddE(NE,NRE);
      }  /* END for (rindex) */
    }

  if (SBuf != NULL)
    {
    MXMLToString(NE,SBuf,SBufSize,NULL,TRUE);
    }

  if (EP != NULL)
    {
    *EP = NE;
    }
  else
    {
    MXMLDestroyE(&NE);
    }

  return(SUCCESS);
  }  /* END MNRsvToString() */




/**
 * Convert the reservation SR on node N to xml and add it to E.
 *
 * @param N (I)
 * @param SR (I) [optional]
 * @param E (O)
 * @param AList (I) [optional]
 */

int MNRsvToXML(

  mnode_t *N,       /* I */
  mrsv_t  *SR,      /* I */
  mxml_t  *E,       /* O */
  int     *AList)   /* I (optional) */

  {
  int  tmpTC = 0;

  char    tmpState[MMAX_NAME];

  char tmpLine[MMAX_LINE];

  if ((N == NULL) || (E == NULL) || (SR == NULL))
    {
    return(FAILURE);
    }

  MXMLSetAttr(E,(char *)MNRsvAttr[mnraName],SR->Name,mdfString);

  sprintf(tmpLine,"%ld",
    SR->StartTime);

  MXMLSetAttr(E,(char *)MNRsvAttr[mnraStart],tmpLine,mdfString);

  sprintf(tmpLine,"%ld",
    SR->EndTime);

  MXMLSetAttr(E,(char *)MNRsvAttr[mnraEnd],tmpLine,mdfString);

  switch (SR->Type)
    {
    case mrtJob:

      {
      mjob_t *J;

      strcpy(tmpLine,"Job");

      if ((J = SR->J) == NULL)
        {
        MDB(2,fUI) MLog("ERROR:    cannot locate host job '%s'\n",
          SR->Name);

        strcpy(tmpState,MJobState[mjsUnknown]);
        }
      else
        {
        strcpy(tmpState,MJobState[J->State]);
        }
      }    /* END BLOCK */

      break;

    case mrtUser:

      strcpy(tmpLine,"User");

      strcpy(tmpState,"N/A");

      break;

    default:

      strcpy(tmpLine,"?");

      strcpy(tmpState,"N/A");

      break;
    }  /* END switch (SR->Type) */

  MXMLSetAttr(E,(char *)MNRsvAttr[mnraState],tmpState,mdfString);

  MXMLSetAttr(E,(char *)MNRsvAttr[mnraType],tmpLine,mdfString);

  MREFindRsv(N->RE,SR,&tmpTC);

  sprintf(tmpLine,"%d",tmpTC);

  MXMLSetAttr(E,(char *)MNRsvAttr[mnraTC],tmpLine,mdfString);

  MXMLSetAttr(
    E,
    (char *)MNRsvAttr[mnraDRes],
    MCResToString(&SR->DRes,0,mfmHuman,tmpLine),
    mdfString);

  return(SUCCESS);
  }  /* END MNRsvToXML() */





/**
 * Create a VPC from an existing reservation.
 *
 * @param R
 * @param VPC
 */


int MRsvToVPC(

  mrsv_t *R,    /* I */
  mpar_t *VPC)  /* O */

  {
  char    EMsg[MMAX_LINE];

  mpar_t *tmpV = NULL;

  marray_t RList;

  mulong Duration;

  if (R == NULL)
    {
    return(FAILURE);
    }

  if (R->VPC != NULL)
    {
    /* Reservation already has VPC */

    return(SUCCESS);
    }
    
  if (MVPCAdd(R->Name,&tmpV) == FAILURE)
    {
    return(FAILURE);
    }

  bmset(&tmpV->Flags,mpfIsDynamic);

  if (R->U != NULL)
    {
    tmpV->O     = R->U;
    tmpV->OType = mxoUser;
    }
  else if (R->A != NULL)
    {
    tmpV->O     = R->A;
    tmpV->OType = mxoAcct;
    }
  else if (R->G != NULL)
    {
    tmpV->O     = R->G;
    tmpV->OType = mxoGroup;
    }

  if (R->U != NULL)
    {
    tmpV->U = R->U;
    }
 
  if (R->G != NULL)
    {
    tmpV->G = R->G;
    }

  if (R->A != NULL)
    {
    tmpV->A = R->A;
    }

  MParSetAttr(tmpV,mpaStartTime,(void **)&R->StartTime,mdfLong,mSet);

  Duration = R->EndTime - R->StartTime;

  MParSetAttr(tmpV,mpaDuration,(void **)&Duration,mdfLong,mSet);

  if (R->RsvGroup == NULL)
    MUStrDup(&R->RsvGroup,R->Name);

  MUStrDup(&tmpV->RsvGroup,R->RsvGroup);

  MUArrayListCreate(&RList,sizeof(mrsv_t *),10);

  MRsvGroupGetList(R->RsvGroup,&RList,NULL,0);

  if (MVPCUpdateReservations(tmpV,&RList,TRUE,EMsg) == FAILURE)
    {
    /* cannot charge for rsv allocation */

    MRsvDestroyGroup(R->RsvGroup,FALSE);

    /* destroy VPC */

    MVPCFree(&tmpV);

    MUArrayListFree(&RList);

    return(FAILURE);
    }

  MUArrayListFree(&RList);

  return(SUCCESS);
  }  /* END MRsvToVPC() */



/**
 * Create pseudo-job to represent reservation.
 *
 * @see MRsvSyncSystemJob() - parent
 * @see MRsvAdjust() - parent
 *
 * @param R (I)
 * @param J (O) [modified]
 */

int MRsvToJob(

  mrsv_t *R,  /* I */
  mjob_t *J)  /* O (modified) */

  {
  mnode_t *N;

  int nindex;

  int ProcCount;
  int NodeCount;

  if ((R == NULL) || (J == NULL) || (MNLIsEmpty(&R->NL)))
    {
    return(FAILURE);
    }

  /* NOTE:  is J freed afterwards?  if not, J->MasterJobName will be memory leak below */

  /* QUESTION:  Why is mjfRsvMap not always set here? */

  /* strcpy(J->Name,R->Name); */

  /* copy creds */

  MACLCopy(&J->Credential.CL,R->CL);

  J->Credential.U = R->U;

  if (R->A != NULL)
    J->Credential.A = R->A;

  if (R->Q != NULL)
    J->Credential.Q = R->Q;

  if (R->G != NULL)
    J->Credential.G = R->G;

  /* clear rsv access list */

  if (J->RsvAList != NULL)
    {
    int rindex;

    /* NOTE:  must know if J->RsvAList values are dynamically or statically allocated.
              If dynamic, below will cause mem leak */

    for (rindex = 0;J->RsvAList[rindex] != NULL;rindex++)
      {
      MUFree(&J->RsvAList[rindex]);
      }

    memset(J->RsvAList,0,sizeof(char *) * MMAX_PJOB);
    }

  /* clear rsv exclude list */

  if (J->RsvExcludeList != NULL)
    {
    int rindex;

    /* NOTE:  must know if J->RsvExcludeList values are dynamically or statically allocated.
              If dynamic, below will cause mem leak */

    for (rindex = 0;J->RsvExcludeList[rindex] != NULL;rindex++)
      {
      MUFree(&J->RsvExcludeList[rindex]);
      }

    memset(J->RsvExcludeList,0,sizeof(char *) * MMAX_PJOB);
    }

  J->WCLimit        = R->EndTime - R->StartTime;
  J->SpecWCLimit[0] = J->WCLimit;
  J->SpecWCLimit[1] = J->WCLimit;

  /* NOTE:  support single req reservations */

  /* NOTE: there isn't a clear translation between DRes.Procs == -1
           for reservations and DRes.Procs for J->Req.  The sum of the 
           allocated tasks must be used and DRes.Procs must be 1 */

  MNLCopy(&J->ReqHList,&R->NL);

  bmset(&J->IFlags,mjifHostList);

  if (J->Req[0] != NULL)
    J->HLIndex = J->Req[0]->Index;

  ProcCount = 0;
  NodeCount = 0;

  for (nindex = 0;MNLGetNodeAtIndex(&R->NL,nindex,NULL) == SUCCESS;nindex++)
    {
    /* if the reservation reserves all procs then we can't copy the task
       structure, we have to convert it, otherwise copy the task structure */

    if (R->DRes.Procs == -1)
      {
      MNLGetNodeAtIndex(&J->ReqHList,nindex,&N);

      MNLSetTCAtIndex(&J->ReqHList,nindex,N->CRes.Procs);

      ProcCount += MNLGetTCAtIndex(&J->ReqHList,nindex);

      NodeCount++;
      }
    else
      {
      MNLSetTCAtIndex(&J->ReqHList,nindex,MNLGetTCAtIndex(&R->NL,nindex));

      ProcCount += MNLGetTCAtIndex(&R->NL,nindex);

      NodeCount++;
      }
    }   /* END for (nindex) */

  J->Request.TC = ProcCount;
  J->Request.NC = NodeCount;

  if (J->Req[0] != NULL)
    {
    mreq_t *RQ;

    RQ = J->Req[0];

    RQ->TaskCount = J->Request.TC;

    RQ->NodeCount = J->Request.NC;

    RQ->TaskRequestList[0] = RQ->TaskCount;
    RQ->TaskRequestList[1] = RQ->TaskCount;
    RQ->TaskRequestList[2] = 0;

    MSNLInit(&RQ->DRes.GenericRes);

    MCResCopy(&RQ->DRes,&R->DRes);

    if (R->DRes.Procs == -1)
      RQ->DRes.Procs = 1;

    if (R->SysJobTemplate != NULL)
      {
      mjob_t *TJ = NULL;

      MTJobFind(R->SysJobTemplate,&TJ);

      if ((TJ != NULL) &&
          (TJ->Req[0] != NULL) &&
          (TJ->Req[0]->NAccessPolicy != mnacNONE))
        {
        J->Req[0]->NAccessPolicy = TJ->Req[0]->NAccessPolicy;
        }
      }
    }  /* END if (J->Req[0] != NULL) */

  if (bmisset(&R->Flags,mrfIsVPC))
    {
    mln_t *tmpL = NULL;

    bmset(&J->Flags,mjfRsvMap);

    bmset(&J->IFlags,mjifVPCMap);

    if (MULLCheck(R->Variables,"VPCID",&tmpL) == SUCCESS)
      {
      /* set J->AName to VPC name */

      MJobSetAttr(J,mjaJobName,(void **)tmpL->Ptr,mdfString,mSet);
      }
    }

  /* NOTE:  Cal, MasterJobName are pointers to memory allocated on R */
  /*          DO NOT CALL MJobDestroy() ON JOB */

  MACLSet(&R->ACL,maJob,J->Name,mcmpSEQ,mnmNeutralAffinity,0,0);

  if (J->JGroup == NULL)
    {
    MJGroupInit(J,(R->RsvGroup != NULL) ? R->RsvGroup : R->Name,-1,0);
    }
  else
    {
    MUStrCpy(J->JGroup->Name,(R->RsvGroup != NULL) ? R->RsvGroup : R->Name,sizeof(J->JGroup->Name));

    J->JGroup->RealJob = NULL;
    }

  return(SUCCESS);
  }  /* END MRsvToJob() */




/**
 * Reservation to a Wiki string
 */

int MRsvToWikiString(
  mrsv_t    *R, /* I */
  char      *Msg,
  mstring_t *String)
  {
  if ((R == NULL) || (String == NULL))
    {
    return(FALSE);
    }

  /* NAME */

  MStringAppendF(String," %s=%s",
    MWikiRsvAttr[mwraName],
    R->Name);

  /* CREATION TIME */

  if (R->CTime > 0)
    {
    MStringAppendF(String," %s=%lu",
      MWikiRsvAttr[mwraCreationTime],
      R->CTime);
    }

  /* START TIME */

  if (R->StartTime > 0)
    {
    MStringAppendF(String," %s=%lu",
      MWikiRsvAttr[mwraStartTime],
      R->StartTime);
    }

  /* END TIME */

  if (R->EndTime > 0)
    {
    MStringAppendF(String," %s=%lu",
      MWikiRsvAttr[mwraEndTime],
      R->EndTime);
    }

  /* ALLOC TC */

  if (R->AllocTC > 0)
    {
    MStringAppendF(String," %s=%d",
      MWikiRsvAttr[mwraAllocTC],
      R->AllocTC);
    }

  /* ALLOC NC */

  if (R->AllocNC > 0)
    {
    MStringAppendF(String," %s=%d",
      MWikiRsvAttr[mwraAllocNC],
      R->AllocNC);
    }

  /* TOTAL ACTIVE PROC_SECONDS */

  if (R->TAPS > 0.0)
    {
    MStringAppendF(String," %s=%.2f",
      MWikiRsvAttr[mwraTAPS],
      R->TAPS);
    }
  else
    {
    MStringAppendF(String," %s=%.2f",
      MWikiRsvAttr[mwraTAPS],
      R->CAPS);
    }

  /* TOTAL IDLE PROC-SECONDS */

  if (R->TIPS > 0.0)
    {
    MStringAppendF(String," %s=%.2f",
      MWikiRsvAttr[mwraTIPS],
      R->TIPS);
    }
  else
    {
    MStringAppendF(String," %s=%.2f",
      MWikiRsvAttr[mwraTIPS],
      R->CIPS);
    }

  /* NODELIST */

  mstring_t tmpString(MMAX_LINE);


  if (!MNLIsEmpty(&R->NL))
    {
    MStringSet(&tmpString,"");
    MRsvAToMString(R,mraAllocNodeList,&tmpString,0);

    MStringAppendF(String," %s=%s",
      MWikiRsvAttr[mwraAllocNodeList],
      tmpString.c_str());
    }

  /* OWNER */

  if (R->O != NULL)
    {
    MStringSet(&tmpString,"");
    MRsvAToMString(R,mraOwner,&tmpString,0);

    MStringAppendF(String," %s=%s",
      MWikiRsvAttr[mwraOwner],
      tmpString.c_str());
    }

  /* ACL */

  MStringSet(&tmpString,"");
  MRsvAToMString(R,mraACL,&tmpString,0);

  MStringAppendF(String," %s=%s",
    MWikiRsvAttr[mwraACL],
    tmpString.c_str());

  /* FLAGS */

  MStringSet(&tmpString,"");
  MRsvAToMString(R,mraFlags,&tmpString,0);

  if ((!tmpString.empty()) &&
      (strcmp(tmpString.c_str(),NONE)))
    {
    MStringAppendF(String," %s=%s",
      MWikiRsvAttr[mwraFlags],
      tmpString.c_str());
    }

  /* MESSAGES */

  MStringSet(&tmpString,"");
  MWikiMBToWikiString(R->MB,Msg,&tmpString);

  if (!tmpString.empty())
    {
    MStringAppendF(String," %s=\"%s\"",
      MWikiRsvAttr[mwraMessage],
      tmpString.c_str());
    }

  /* PRIORITY */

  if (R->Priority > 0)
    {
    MStringAppendF(String," %s=%d",
      MWikiRsvAttr[mwraPriority],
      R->Priority);
    }

  /* TYPE */

  if (R->Type != mrtNONE)
    {
    MStringAppendF(String," %s=%s",
      MWikiRsvAttr[mwraType],
      MRsvType[R->Type]);
    } 

  /* SUBTYPE */

  if ((R->SubType != mrsvstNONE) && (R->SubType != mrsvstAdmin_Other))
    {
    MStringAppendF(String," %s=%s",
      MWikiRsvAttr[mwraSubType],
      MRsvSubType[R->SubType]);
    }

  /* LABEL */

  if (R->Label != NULL)
    {
    MStringAppendF(String," %s=%s",
      MWikiRsvAttr[mwraLabel],
      R->Label);
    }

  /* RSVGROUP */

  if (R->RsvGroup != NULL)
    {
    MStringAppendF(String," %s=%s",
      MWikiRsvAttr[mwraRsvGroup],
      R->RsvGroup);
    }

  MStringAppend(String,"\n");

  return(SUCCESS);
  }  /* END MRsvToWikiString() */

/* END MRsvTo.c */
