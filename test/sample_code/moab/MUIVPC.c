/* HEADER */

/**
 * @file MUIVPC.c
 *
 * Contains MUI VPC Create and Destroy
 */



#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 * Create virtual private cluster (VPC).
 *
 * @see __MUISchedCtlCreate() - parent
 * @see MUIRsvCreate() - child - create rsv
 *
 * @param RE (I)
 * @param Auth (I)
 * @param Flags (I)
 * @param EMsg (O) [optional,minsize=MMAX_BUFFER]
 */

int MUIVPCCreate(

  mxml_t    *RE,
  char      *Auth,
  mbitmap_t *Flags,
  char      *EMsg)

  {
  /* create new vpc */

  char   tmpLine[MMAX_LINE];

  char  tmpBuf[MMAX_BUFFER];

  char *BPtr;
  int   BSpace;

  char *ptr;
  char *TokPtr;

  enum MParAttrEnum AIndex;

  mtrans_t *Trans = NULL;

  char VPCName[MMAX_NAME];
  char SpecName[MMAX_NAME];

  mulong StartTime;
  mulong Duration;

  char   tmpName[MMAX_NAME];
  char   tmpVal[MMAX_LINE];

  char   ResList[MMAX_LINE];
  char   ReqResList[MMAX_LINE];
  char   RsvGroup[MMAX_NAME];
  char   RsvProfile[MMAX_NAME];
  char   VarList[MMAX_BUFFER];

  /* eventually remove UName */

  char   UName[MMAX_NAME];
  char   AName[MMAX_NAME];
  char   GName[MMAX_NAME];
  char   QName[MMAX_NAME];
  char   OName[MMAX_NAME];
  char   Msg[MMAX_LINE];
  char   VPCProfile[MMAX_NAME];

  char  *QueryData = NULL;

  mulong VPCFlags = 0;

  int    STok;
  int    TID = 0;  /* set to avoid compiler warning */

  void *O;
  enum MXMLOTypeEnum OIndex;

  double Cost;

  mbool_t TransLocated = FALSE;

  mpar_t *C;
  mvpcp_t *CProf = NULL;

  mrsv_t *R;

  marray_t RList;

  /* FORMAT:  -s duration=X -s starttime=X -s profile=X <TID>[,<TID>]... */

  /* initialize */

  MUSNInit(&BPtr,&BSpace,tmpBuf,sizeof(tmpBuf));

  VPCName[0] = '\0';
  SpecName[0] = '\0';

  ResList[0] = '\0';
  VPCProfile[0] = '\0';
  RsvProfile[0] = '\0';
  VarList[0] = '\0';

  ReqResList[0] = '\0';

  UName[0] = '\0';
  AName[0] = '\0';
  GName[0] = '\0';
  QName[0] = '\0';
  OName[0] = '\0';

  Msg[0]   = '\0';

  StartTime = 0;
  Duration  = 0;

  Cost     = 0.0;

  OIndex   = mxoNONE;
  O        = NULL;

  /* load VPC attributes */

  STok = -1;

  while (MS3GetSet(
      RE,
      NULL,
      &STok,
      tmpName,
      sizeof(tmpName),
      tmpVal,
      sizeof(tmpVal)) == SUCCESS)
    {
    AIndex = (enum MParAttrEnum)MUGetIndexCI(tmpName,MParAttr,FALSE,0);

    switch (AIndex)
      {
      case mpaAcct:

        MUStrCpy(AName,tmpVal,sizeof(AName));

        break;

      case mpaGroup:

        MUStrCpy(GName,tmpVal,sizeof(GName));

        break;

      case mpaQOS:

        MUStrCpy(QName,tmpVal,sizeof(QName));

        break;

      case mpaDuration:

        Duration = MUTimeFromString(tmpVal);

        break;

      case mpaMessages:

        MUStrCpy(Msg,tmpVal,sizeof(Msg));

        break;

      case mpaID:

        MUStrCpy(SpecName,tmpVal,sizeof(SpecName));
 
        break;

      case mpaOwner:

        MUStrCpy(OName,tmpVal,sizeof(OName));

        break;

      case mpaProfile:

        MUStrCpy(VPCProfile,tmpVal,sizeof(VPCProfile));

        break;

      case mpaReqResources:

        MUStrCpy(ReqResList,tmpVal,sizeof(ReqResList));

        break;

      case mpaRsvGroup:

        MUStrCpy(ResList,tmpVal,sizeof(ResList));

        break;

      case mpaRsvProfile:

        MUStrCpy(RsvProfile,tmpVal,sizeof(RsvProfile));

        break;

      case mpaStartTime:

        StartTime = MUTimeFromString(tmpVal);

        break;
   
      case mpaUser:

        MUStrCpy(UName,tmpVal,sizeof(UName));

        break;

      case mpaVariables:

        /* FORMAT:  <name>[=<value>][[;<name[=<value]]...] */

        MUStrCpy(VarList,tmpVal,sizeof(VarList));

        break;

      default:

        MUSNPrintF(&BPtr,&BSpace,"ERROR:  unexpected attribute '%s' specified",
          tmpName);

        snprintf(EMsg,MMAX_BUFFER,"%s",
          tmpBuf);

        return(FAILURE);

        /*NOTREACHED*/

        break; 
      }  /* END switch (AIndex) */
    }    /* END while (MS3GetSet() == SUCCESS) */

  if (UName[0] == '\0')
    {
    MUStrCpy(UName,Auth,sizeof(UName));
    }

  if ((StartTime <= 0) || (Duration <= 0))
    {
    char     *tail;

    /* extract starttime from TID */

    TID = (int)strtol(ResList,&tail,10);

    if ((TID > 0) && 
        (MTransFind(TID,Auth,&Trans) == SUCCESS))
      {
      TransLocated = TRUE;

      if (StartTime <= 0)
        {
        if (Trans->Val[mtransaStartTime] != NULL)
          StartTime = strtol(Trans->Val[mtransaStartTime],NULL,10);
        else
          StartTime = MSched.Time;
        }

      if (Duration <= 0)
        {
        /* NOTE:  should use VPC Profile MinDuration instead of MCONST_DAYLEN (NYI) */

        if (Trans->Val[mtransaDuration] != NULL)
          Duration = strtol(Trans->Val[mtransaDuration],NULL,10);
        else
          Duration = MCONST_DAYLEN;
        }

      if ((Cost <= 0) && (Trans->Val[mtransaCost] != NULL))
        Cost = strtod(Trans->Val[mtransaCost],NULL);

      if (Trans->Val[mtransaVPCProfile] != NULL)
        {
        if ((VPCProfile[0] != '\0') && 
            strcasecmp(VPCProfile,Trans->Val[mtransaVPCProfile]))
          {
          MUSNPrintF(&BPtr,&BSpace,"ERROR:  cannot commit transaction id %d - requires profile '%s'\n",
            TID,
            Trans->Val[mtransaVPCProfile]);

          snprintf(EMsg,MMAX_BUFFER,"%s",
            tmpBuf);

          return(FAILURE);
          }

        MUStrCpy(VPCProfile,Trans->Val[mtransaVPCProfile],sizeof(VPCProfile));
        }
 
      QueryData = Trans->Val[mtransaMShowCmd];

      if (Trans->Val[mtransaFlags] != NULL)
        VPCFlags = strtol(Trans->Val[mtransaFlags],NULL,10);
      }  /* END if ((TID > 0) && ...) */
    }    /* END if ((StartTime <= 0) || (Duration <= 0)) */

  /* sanity check VPC config */

  if ((StartTime <= 0) || 
      (Duration <= 0) || 
      (ResList[0] == '\0'))
    {
    if ((TID > 0) && (TransLocated == FALSE))
      {
      MUSNPrintF(&BPtr,&BSpace,"ERROR:  cannot locate transaction id %d\n",
        TID);
      }
    else
      {
      MUSNCat(&BPtr,&BSpace,"ERROR:  invalid/incomplete vpc configuration");
      }

    snprintf(EMsg,MMAX_BUFFER,"%s",
      tmpBuf);

    return(FAILURE);
    }

  /* locate profile */

  if ((VPCProfile[0] != '\0') && 
      (MVPCProfileFind(VPCProfile,&CProf) == FAILURE))
    {
    /* cannot locate specified VPC profile */

    sprintf(tmpLine,"ERROR:  cannot locate vpc profile '%.32s'",
      VPCProfile);

    MUSNCat(&BPtr,&BSpace,tmpLine);

    snprintf(EMsg,MMAX_BUFFER,"%s",tmpBuf);

    return(FAILURE);
    }

  /* find out the owner and create rsv, systemjob, vpc under that owner */

  if ((OName[0] == '\0') && (UName[0] != '\0'))
    {
    snprintf(OName,sizeof(OName),"user:%s",
      UName);
    }
  else if ((OName[0] != '\0') && (UName[0] != '\0'))
    {
    MUSNCat(&BPtr,&BSpace,"WARNING:  both owner and user specified, using user\n");

    snprintf(OName,sizeof(OName),"user:%s",UName);
    }

  /* FORMAT:  <otype>:<oid> */

  MUStrCpy(tmpLine,OName,sizeof(tmpLine));

  ptr = MUStrTok((char *)tmpLine,":",&TokPtr);

  OIndex = (enum MXMLOTypeEnum)MUGetIndexCI(ptr,MXO,FALSE,mxoNONE);

  if ((OIndex == mxoNONE) ||
     ((OIndex != mxoUser) &&
      (OIndex != mxoGroup) &&
      (OIndex != mxoAcct)))
    {
    MUSNCat(&BPtr,&BSpace,"ERROR:  Owner must be a user, group or account.\n");

    snprintf(EMsg,MMAX_BUFFER,"%s",tmpBuf);

    return(FAILURE);
    }

  ptr = MUStrTok(NULL," \n\t",&TokPtr);

  if ((ptr == NULL) || (ptr[0] == '\0'))
    {
    MUSNCat(&BPtr,&BSpace,"ERROR:  no owner id specified.\n");

    snprintf(EMsg,MMAX_BUFFER,"%s",tmpBuf);

    return(FAILURE);
    }

  if (MOGetObject(OIndex,ptr,&O,mAdd) == FAILURE)
    {
    MUSNPrintF(&BPtr,&BSpace,"ERROR:  cannot add owner '%s' '%s'\n",
      MXO[OIndex],
      ptr);

    snprintf(EMsg,MMAX_BUFFER,"%s",tmpBuf);

    return(FAILURE);
    }

  /* locate unique name */

  if (SpecName[0] == '\0')
    {
    snprintf(VPCName,sizeof(VPCName),"vpc.%d",MSched.RsvGroupCounter++);
    }
  else
    {
    snprintf(VPCName,sizeof(VPCName),"%s.%d",SpecName,MSched.RsvGroupCounter++);
    }

  if (MVPCAdd(VPCName,&C) == FAILURE)
    {
    /* cannot add new VPC */

    /* license message was already added to scheduler in MVPCAdd */

    if (MOLDISSET(MSched.LicensedFeatures,mlfVPC))
      {
      MUSNCat(&BPtr,&BSpace,"ERROR:  cannot add vpc");
      }
    else
      {
      MUSNCat(&BPtr,&BSpace,"ERROR:  VPC creation only enabled in Hosting/Utility Computing release");
      }

    snprintf(EMsg,MMAX_BUFFER,"%s",tmpBuf);

    /* destroy VPC */

    MVPCFree(&C);

    return(FAILURE);
    }

  if (CProf != NULL)
    C->VPCProfile = CProf;

  if (AName[0] != '\0')
    MAcctFind(AName,&C->A);

  if (GName[0] != '\0')
    MGroupFind(GName,&C->G);

  if (QName[0] != '\0')
    MQOSFind(QName,&C->Q);

  C->O = O;
  C->OType = OIndex;

  switch (OIndex)
    {
    case mxoUser:

      C->U = (mgcred_t *)O;

      break;

    case mxoGroup:

      C->G = (mgcred_t *)O;

      break;

    case mxoAcct:

      C->A = (mgcred_t *)O;

      break;

    case mxoQOS:

      C->Q = (mqos_t *)O;

      break;

    default:
 
      /* NO-OP */

      break;
    }

  /* create resource reservation */

  C->O     = O;
  C->OType = OIndex;

  C->Cost  = Cost;

  if (O == NULL)
    {
    MUSNPrintF(&BPtr,&BSpace,"ERROR:  could not add owner '%s' '%s'\n",
      MXO[C->OType],
      ptr);

    snprintf(EMsg,MMAX_BUFFER,"%s",tmpBuf);

    MRsvDestroyGroup(RsvGroup,FALSE);

    /* destroy VPC */

    MVPCFree(&C);

    return(FAILURE);
    }
  
  if ((MParSetAttr(C,mpaOwner,(void **)OName,mdfString,mSet) == FAILURE) ||
      (C->OType == mxoNONE))
    {
    MUSNCat(&BPtr,&BSpace,"ERROR:  valid owner not specified\n");

    snprintf(EMsg,MMAX_BUFFER,"%s",
      tmpBuf);

    MRsvDestroyGroup(RsvGroup,FALSE);

    /* destroy VPC */

    MVPCFree(&C);

    return(FAILURE);
    }

  bmset(&C->Flags,mpfIsDynamic);

  C->State = mvpcsPending;

  MPeerFind(Auth,FALSE,&C->P,FALSE);

  MParSetAttr(C,mpaStartTime,(void **)&StartTime,mdfLong,mSet);

  MParSetAttr(C,mpaDuration,(void **)&Duration,mdfLong,mSet);

  if (ReqResList[0] != '\0')
    MParSetAttr(C,mpaReqResources,(void **)ReqResList,mdfString,mSet);

  MParSetAttr(C,mpaProfile,(void **)VPCProfile,mdfString,mSet);

  MParSetAttr(C,mpaCmdLine,(void **)QueryData,mdfString,mSet);

  MParSetAttr(C,mpaVPCFlags,(void **)&VPCFlags,mdfLong,mSet);

  if (Msg[0] != '\0')
    {
    /* add message */

    /* NYI */
    }

  MUArrayListCreate(&RList,sizeof(mrsv_t *),10);

  if (MVPCAddTID(C,RsvProfile,Auth,ResList,&RList,EMsg) == FAILURE)
    {
    MUArrayListFree(&RList);

    return(FAILURE);
    }

  if (Trans->RsvID != NULL)
    {
    MRsvDestroyGroup(Trans->RsvID,FALSE);

    /* remove the RsvIDs for this Trans and all dependents */

    MTransInvalidate(Trans);
    }

  R = (mrsv_t *)MUArrayListGetPtr(&RList,0);

  MUStrCpy(RsvGroup,R->RsvGroup,MMAX_NAME);

  if (MVPCUpdateReservations(C,&RList,TRUE,EMsg) == FAILURE)
    {
    /* cannot charge for rsv allocation */

    MRsvDestroyGroup(RsvGroup,FALSE);

    /* destroy VPC */

    MVPCFree(&C);

    MUArrayListFree(&RList);

    return(FAILURE);
    }

  MRsvSetAttr(
    R,
    mraVariables,
    (void *)VarList,
    mdfString,
    mAdd);

  C->PurgeTime = C->ProvEndTime + MDEF_VPCPURGETIME;

  if (C->VPCProfile != NULL)
    {
    C->Priority = C->VPCProfile->Priority;
    }

  if (bmisset(Flags,mcmXML))
    {
    mxml_t *VE = NULL;

    MXMLCreateE(&VE,"vpc");

    MXMLSetVal(VE,(void *)VPCName,mdfString);

    if (bmisset(Flags,mcmVerbose))
      {
      mrsv_t *R;

      int   NC;

      char *ptr;
      char *TokPtr;

      char tmpBuf[MMAX_BUFFER];
      char MasterHost[MMAX_NAME];

      char Port[MMAX_NAME];

      mln_t *tmpL;

      mnode_t *tmpN;

      mbitmap_t IFlags;

      bmset(&IFlags,mcmXML);
      bmset(&IFlags,mcmExclusive);

      MRsvFind(RsvGroup,&R,mraNONE);

      ptr = NULL;

      if ((R != NULL) && (!MNLIsEmpty(&R->NL)))
        {
        MNLToString(&R->NL,FALSE,",",'\0',tmpBuf,sizeof(tmpBuf));
        }

      MXMLSetAttr(VE,"starttime",(void *)&StartTime,mdfLong);
      MXMLSetAttr(VE,"duration",(void *)&Duration,mdfLong);

      /* FIXME: MasterHost can either be the first node of the Rsv within RList
                with the label "head" (NYI) or just the first node in the list */

      MNLGetNodeAtIndex(&R->NL,0,&tmpN);

      MUStrCpy(MasterHost,tmpN->Name,sizeof(MasterHost));

      MUStrTok(MasterHost,"= \t\n;:",&TokPtr);

      ptr = MUStrTok(NULL,", \t\n;:",&TokPtr);

      MXMLSetAttr(VE,"master",(void *)ptr,mdfString);

      MUStrCpy(MasterHost,ptr,sizeof(MasterHost));

      if (R != NULL)
        {
        /* triggers may populate variables that need to be returned (ie. port) */

        MSchedCheckTriggers(NULL,-1,NULL);

        if (MULLCheck(R->Variables,"port",&tmpL) == SUCCESS)
          {
          MXMLSetAttr(VE,"port",tmpL->Ptr,mdfString);

          MUStrCpy(Port,(char *)tmpL->Ptr,sizeof(Port));
          }

        if (MULLCheck(R->Variables,"master",&tmpL) == SUCCESS)
          {
          MXMLSetAttr(VE,"master",tmpL->Ptr,mdfString);

          MUStrCpy(MasterHost,(char *)tmpL->Ptr,sizeof(MasterHost));
          }
        }   /* END  if (R != NULL) */

      snprintf(tmpLine,sizeof(tmpLine),"master=%s",MasterHost);

      MRsvSetAttr(R,mraVariables,(void **)tmpLine,mdfString,mAdd);

      snprintf(tmpLine,sizeof(tmpLine),"port=%s",Port);

      MRsvSetAttr(R,mraVariables,(void **)tmpLine,mdfString,mAdd);

      if (VPCProfile[0] != '\0')
        MXMLSetAttr(VE,(char *)MParAttr[mpaProfile],VPCProfile,mdfString);

      if ((R != NULL) && (MULLCheck(R->Variables,"hostlist",&tmpL) == SUCCESS))
        {
        /* NYI - send across information about possible virtual machines */
        }
      else
        {
        /* send across node information */
   
        /* NOTE:  insert node releasepriority information */
   
        bmset(&IFlags,mcmOverlap);
   
        ptr = MUStrTok(tmpBuf,", \t\n;:",&TokPtr);
   
        NC = 0;
   
        while (ptr != NULL)
          {
          MUINodeDiagnose(
            Auth,
            NULL,
            mxoNONE,
            NULL,
            &VE,    /* O */
            NULL,
            NULL,
            NULL,
            ptr,
            &IFlags);
   
          ptr = MUStrTok(NULL,", \t\n;:",&TokPtr);
   
          NC += 1;
          }
   
        MXMLSetAttr(VE,"NC",(void *)&NC,mdfInt);
        }
      }     /* END if (IsVerbose == TRUE) */

    MXMLToString(VE,EMsg,MMAX_BUFFER,NULL,TRUE);

    MXMLDestroyE(&VE);
    }  /* END if (bmisset(Flags,mcmXML)) */
  else
    {
    MUSNCat(&BPtr,&BSpace,VPCName);

    snprintf(EMsg,MMAX_BUFFER,"%s",
      tmpBuf);
    }

  /* force checkpoint this iteration */

  MCP.LastCPTime = 0;

  MUArrayListFree(&RList);

  return(SUCCESS);
  }  /* END MUIVPCCreate() */
/* END MUIVPC.c */
