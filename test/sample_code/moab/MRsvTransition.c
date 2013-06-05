/* HEADER */

      
/**
 * @file MRsvTransition.c
 *
 * Contains: MRsvTransition functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 * Store reservation in transition structure for storage in 
 * databse. 
 *
 * @see MReqToTransitionStruct (child)
 * 
 * @param   SR (I) the reservation to store
 * @param   DR (O) the transition object to store it in
 */

int MRsvToTransitionStruct(

  mrsv_t      *SR,
  mtransrsv_t *DR)

  {
  mxml_t *AE;

  mstring_t tmpString(MMAX_LINE);

  char tmpLine[MMAX_LINE];

  MRsvAToMString(SR,mraName,&tmpString,mfmNONE);
  MUStrCpy(DR->Name,tmpString.c_str(),sizeof(DR->Name));

  /* convert the acl to xml and store it in the transition struct */

  if (!MACLIsEmpty(SR->ACL))
    {
    macl_t *tACL;
    mstring_t ACLString(MMAX_LINE);

    for (tACL = SR->ACL;tACL != NULL;tACL = tACL->Next)
      {
      AE = NULL;
      MACLToXML(tACL,&AE,NULL,FALSE);

      MXMLToString(AE,tmpLine,sizeof(tmpLine),NULL,TRUE);
      MXMLDestroyE(&AE);
      MStringAppend(&ACLString,tmpLine);
      }
    MUStrDup(&DR->ACL,ACLString.c_str());
    } /* END if (!MACLIsEmpty(SR->ACL)) */

  MRsvAToMString(SR,mraAAccount,&tmpString,mfmNONE);
  MUStrDup(&DR->AAccount,tmpString.c_str());
  MRsvAToMString(SR,mraAGroup,&tmpString,mfmNONE);
  MUStrDup(&DR->AGroup,tmpString.c_str());
  MRsvAToMString(SR,mraAUser,&tmpString,mfmNONE);
  MUStrDup(&DR->AUser,tmpString.c_str());
  MRsvAToMString(SR,mraAQOS,&tmpString,mfmNONE);
  MUStrDup(&DR->AQOS,tmpString.c_str());

  DR->AllocNodeCount = SR->AllocNC;

  /* Ensure AllocNodeList mstring has been allocated */
  if (DR->AllocNodeList == NULL)
    DR->AllocNodeList = new mstring_t(MMAX_NAME);

  MRsvAToMString(SR,mraAllocNodeList,DR->AllocNodeList,mfmNONE);

  DR->AllocProcCount = SR->AllocPC;
  DR->AllocTaskCount = SR->AllocTC;

  DR->Priority = SR->Priority;

  DR->MaxJob = SR->MaxJob;

  /* convert the cl to xml and store it in the transition struct */
  if (!MACLIsEmpty(SR->CL))
    {
    macl_t *tempCL;
    mstring_t CLString(MMAX_LINE);

    for (tempCL = SR->CL;tempCL != NULL;tempCL = tempCL->Next)
      {
      AE = NULL;
      MACLToXML(tempCL,&AE,NULL,TRUE);

      MXMLToString(AE,tmpLine,sizeof(tmpLine),NULL,TRUE);
      MXMLDestroyE(&AE);
      MStringAppend(&CLString,tmpLine);
      }
    MUStrDup(&DR->CL,CLString.c_str());
    } /* END if (!MACLIsEmpty(SR->CL)) */

  MRsvAToMString(SR,mraComment,&tmpString,mfmNONE);
  MUStrDup(&DR->Comment,tmpString.c_str());

  DR->Cost = SR->LienCost;

  DR->CTime = SR->CTime;
  DR->Duration = (SR->EndTime - SR->StartTime);
  DR->EndTime = SR->EndTime;

  MRsvAToMString(SR,mraExcludeRsv,&tmpString,mfmNONE);
  MUStrDup(&DR->ExcludeRsv,tmpString.c_str());

  DR->ExpireTime = SR->ExpireTime;

  MRsvAToMString(SR,mraFlags,&tmpString,mfmNONE);
  MUStrDup(&DR->Flags,tmpString.c_str());
  MRsvAToMString(SR,mraGlobalID,&tmpString,mfmNONE);
  MUStrDup(&DR->GlobalID,tmpString.c_str());
  MRsvAToMString(SR,mraHostExp,&tmpString,mfmNONE);
  MUStrDup(&DR->HostExp,tmpString.c_str());

  /* convert history to xml and then to a string */
  AE = NULL;
  MHistoryToXML(SR,mxoRsv,&AE);
  MXMLToString(AE,tmpLine,sizeof(tmpLine),NULL,TRUE);
  MUStrDup(&DR->History,tmpLine);
  MXMLDestroyE(&AE);

  MRsvAToMString(SR,mraLabel,&tmpString,mfmNONE);
  MUStrDup(&DR->Label,tmpString.c_str());

  DR->LastChargeTime = SR->LastChargeTime;

  MRsvAToMString(SR,mraLogLevel,&tmpString,mfmNONE);
  MUStrDup(&DR->LogLevel,tmpString.c_str());
  MRsvAToMString(SR,mraMessages,&tmpString,mfmNONE);
  MUStrDup(&DR->Messages,tmpString.c_str());
  MRsvAToMString(SR,mraOwner,&tmpString,mfmNONE);
  MUStrDup(&DR->Owner,tmpString.c_str());
  MRsvAToMString(SR,mraPartition,&tmpString,mfmNONE);
  MUStrDup(&DR->Partition,tmpString.c_str());
  MRsvAToMString(SR,mraProfile,&tmpString,mfmNONE);
  MUStrDup(&DR->Profile,tmpString.c_str());
  MRsvAToMString(SR,mraReqArch,&tmpString,mfmNONE);
  MUStrDup(&DR->ReqArch,tmpString.c_str());
  MRsvAToMString(SR,mraReqFeatureList,&tmpString,mfmNONE);
  MUStrDup(&DR->ReqFeatureList,tmpString.c_str());
  MRsvAToMString(SR,mraReqMemory,&tmpString,mfmNONE);
  MUStrDup(&DR->ReqMemory,tmpString.c_str());
  MRsvAToMString(SR,mraReqNodeCount,&tmpString,mfmNONE);
  MUStrDup(&DR->ReqNodeCount,tmpString.c_str());
  MRsvAToMString(SR,mraReqNodeList,&tmpString,mfmNONE);
  MUStrDup(&DR->ReqNodeList,tmpString.c_str());
  MRsvAToMString(SR,mraReqOS,&tmpString,mfmNONE);
  MUStrDup(&DR->ReqOS,tmpString.c_str());
  MRsvAToMString(SR,mraReqTaskCount,&tmpString,mfmNONE);
  MUStrDup(&DR->ReqTaskCount,tmpString.c_str());
  MRsvAToMString(SR,mraReqTPN,&tmpString,mfmNONE);
  MUStrDup(&DR->ReqTPN,tmpString.c_str());
  MRsvAToMString(SR,mraResources,&tmpString,mfmNONE);
  MUStrDup(&DR->Resources,tmpString.c_str());
  MRsvAToMString(SR,mraRsvAccessList,&tmpString,mfmNONE);
  MUStrDup(&DR->RsvAccessList,tmpString.c_str());
  MRsvAToMString(SR,mraRsvGroup,&tmpString,mfmNONE);
  MUStrDup(&DR->RsvGroup,tmpString.c_str());
  MRsvAToMString(SR,mraRsvParent,&tmpString,mfmNONE);
  MUStrDup(&DR->RsvParent,tmpString.c_str());
  MRsvAToMString(SR,mraSID,&tmpString,mfmNONE);
  MUStrDup(&DR->SID,tmpString.c_str());

  DR->StartTime = SR->StartTime;
  DR->StatCAPS = SR->CAPS;
  DR->StatCIPS = SR->CIPS;
  DR->StatTAPS = SR->TAPS;
  DR->StatTIPS = SR->TIPS;

  MRsvAToMString(SR,mraSubType,&tmpString,mfmNONE);
  MUStrDup(&DR->SubType,tmpString.c_str());
  MRsvAToMString(SR,mraTrigger,&tmpString,mfmNONE);
  MUStrDup(&DR->Trigger,tmpString.c_str());
  MRsvAToMString(SR,mraType,&tmpString,mfmNONE);
  MUStrDup(&DR->Type,tmpString.c_str());

  /* Ensure Variables mstring has been allocated */
  if (DR->Variables == NULL)
    DR->Variables = new mstring_t(MMAX_NAME);

  MRsvAToMString(SR,mraVariables,DR->Variables,mfmNONE);

  MRsvAToMString(SR,mraVMList,&tmpString,mfmNONE);
  MUStrDup(&DR->VMList,tmpString.c_str());

  return(SUCCESS);
  }  /* END MJobToTransitionStruct() */



/**
 * @param SR            (I)
 * @param DR            (O)
 */

int MRsvTransitionCopy(

  mtransrsv_t *SR,         /* I */
  mtransrsv_t *DR)         /* O */

  {

  if ((DR == NULL) || (SR == NULL))
    return(FAILURE);

  memcpy(DR,SR,sizeof(mtransrsv_t));

  return(SUCCESS);
  } /* END MRsvTransitionCopy */ 




/**
 * check to see if a given reservation matches a list of 
 * constraints 
 *
 * @param R               (I)
 * @param RConstraintList (I)
 */
mbool_t MRsvTransitionMatchesConstraints(

    mtransrsv_t *R,
    marray_t    *RConstraintList)

  {
  int cindex;
  mrsvconstraint_t *RConstraint;

  if (R == NULL)
    {
    return(FALSE);
    }

  if ((RConstraintList == NULL) || (RConstraintList->NumItems == 0))
    {
    return(TRUE);
    }

  MDB(7,fSCHED) MLog("INFO:     Checking rsv %s against constraint list.\n",
    R->Name);

  for (cindex = 0; cindex < RConstraintList->NumItems; cindex++)
    {
    RConstraint = (mrsvconstraint_t *)MUArrayListGet(RConstraintList,cindex);

    if (strcmp(R->Name,RConstraint->AVal[0]) != 0)
      {
      return(FALSE);
      }
    }

  return(TRUE);
  }




/**
 * Store a reservation transition object in the given XML object.
 *
 * @param   R        (I) the reservation transition object
 * @param   REP      (O) the XML object to store it in
 * @param   SRAList  (I) [optional]
 * @param   SRQAList (I) [optional]
 * @param   CFlags   (I)
 */

int MRsvTransitionToXML(

  mtransrsv_t        *R,
  mxml_t            **REP,
  enum MRsvAttrEnum  *SRAList,
  enum MReqAttrEnum  *SRQAList,
  long                CFlags)

  {
  mxml_t *RE;     /* element for the reservation */

  int aindex;

  char *startptr;
  char *endptr;

  char aclstr[MMAX_LINE];
  char clstr[MMAX_LINE];

  enum MRsvAttrEnum *RAList;

  mxml_t *CE;

  const enum MRsvAttrEnum DRAList[] = {
    mraName,
    mraACL,       /**< @see also mraCL */
    mraAAccount,
    mraAGroup,
    mraAUser,
    mraAQOS,
    mraAllocNodeCount,
    mraAllocNodeList,
    mraAllocProcCount,
    mraAllocTaskCount,
    mraCL,        /**< credential list */
    mraComment,
    mraCost,      /**< rsv AM lien/charge */
    mraCTime,     /**< creation time */
    mraDuration,
    mraEndTime,
    mraExcludeRsv,
    mraExpireTime,
    mraFlags,
    mraGlobalID,
    mraHostExp,
    mraHistory,
    mraLabel,
    mraLastChargeTime, /* time rsv charge was last flushed */
    mraLogLevel,
    mraMaxJob,
    mraMessages,
    mraOwner,
    mraPartition,
    mraPriority,
    mraProfile,
    mraReqArch,
    mraReqFeatureList,
    mraReqMemory,
    mraReqNodeCount,
    mraReqNodeList,
    mraReqOS,
    mraReqTaskCount,
    mraReqTPN,
    mraResources,
    mraRsvAccessList, /* list of rsv's and rsv groups which can be accessed */
    mraRsvGroup,
    mraRsvParent,
    mraSID,
    mraStartTime,
    mraStatCAPS,
    mraStatCIPS,
    mraStatTAPS,
    mraStatTIPS,
    mraSubType,
    mraTrigger,
    mraType,
    mraVariables,
    mraVMList,
    mraNONE };

  *REP = NULL;

  if (MXMLCreateE(REP,(char *)MXO[mxoRsv]) == FAILURE)
    {
    return(FAILURE);
    }

  RE = *REP;

  if (SRAList != NULL)
    RAList = SRAList;
  else
    RAList = (enum MRsvAttrEnum *)DRAList;

  mstring_t tmpBuf(MMAX_LINE);;

  for (aindex = 0; RAList[aindex] != mraNONE; aindex++)
    {
    switch (RAList[aindex])
      {
      case mraACL:
     
        if (MUStrIsEmpty(R->ACL))
          break;

        /* iterate through each ACL entry in the XML string and turn it into and XML struct */
        startptr = R->ACL;

        while (*startptr != '\0')
          {
          /* skip to the end of the closing ACL tag */
          endptr = strchr(startptr,'>');
          endptr = strchr(endptr+1,'>');

          /* Add 2 to the length to get the full length of the string and to
             compensate for MUStrCpy taking off one char */
          MUStrCpy(aclstr,startptr,(endptr - startptr + 2));

          CE = NULL;
    
          MXMLFromString(&CE,aclstr,NULL,NULL);
          MXMLAddE(RE,CE);

          startptr = endptr + 1;
          }  

        break;

      case mraAllocNodeList:

        if (!R->AllocNodeList->empty())
          MXMLSetAttr(RE,(char *)MRsvAttr[RAList[aindex]],(void *)R->AllocNodeList->c_str(),mdfString);
          
        break;

      case mraCL:

        if (MUStrIsEmpty(R->CL))
          break;

        /* iterate through each CL entry in the XML string and turn it into and XML struct */
        startptr = R->CL;

        while (*startptr != '\0')
          {
          /* skip to the end of the closing ACL tag */
          endptr = strchr(startptr,'>');
          endptr = strchr(endptr+1,'>');

          /* Add 2 to the length to get the full length of the string and to
             compensate for MUStrCpy taking off one char */
          MUStrCpy(clstr,startptr,(endptr-startptr+2));

          CE = NULL;
    
          MXMLFromString(&CE,clstr,NULL,NULL);
          MXMLAddE(RE,CE);

          startptr = endptr + 1;
          }
        break;

      case mraHistory:

        if (MUStrIsEmpty(R->History))
          break;

        CE = NULL;
        
        MXMLFromString(&CE,R->History,NULL,NULL);
        MXMLAddE(RE,CE);

        break;

      case mraExpireTime:

        if (R->ExpireTime != 0)
          {
          MRsvTransitionAToString(R,RAList[aindex],&tmpBuf,mfmNONE);
      
          if (!MUStrIsEmpty(tmpBuf.c_str()))
            MXMLSetAttr(RE,(char *)MRsvAttr[RAList[aindex]],(void *)tmpBuf.c_str(),mdfString);
            
          tmpBuf = "";
          }

        break;

      case mraVariables:

        if (!R->Variables->empty())
          MXMLSetAttr(RE,(char *)MRsvAttr[RAList[aindex]],(void *)R->Variables->c_str(),mdfString);
          
        break;

      case mraMaxJob:

        if (R->MaxJob > 0)
          MXMLSetAttr(RE,(char *)MRsvAttr[mraMaxJob],(void *)&R->MaxJob,mdfInt);

        break;

      case mraMessages:

        if (!MUStrIsEmpty(R->Messages))
          {
          mxml_t *ME = NULL;

          MRsvTransitionAToString(R,mraMessages,&tmpBuf,mfmNONE);

          if ((!MUStrIsEmpty(tmpBuf.c_str())) &&
              (MXMLFromString(&ME,tmpBuf.c_str(),NULL,NULL) == SUCCESS))
            {
            MXMLAddE(RE,ME);
            }
          } /* END if (!MUStrIsEmpty(R->Messages)) */

        tmpBuf = "";

        break;
      
      default:

        MRsvTransitionAToString(R,RAList[aindex],&tmpBuf,mfmNONE);
    
        if (!MUStrIsEmpty(tmpBuf.c_str()))
          MXMLSetAttr(RE,(char *)MRsvAttr[RAList[aindex]],(void *)tmpBuf.c_str(),mdfString);
          
        tmpBuf = "";

        break;
      }   /* END switch (RAList[aindex]) */
    } /* END for (AIndex...) */

  return(SUCCESS);
  }  /* END MRsvTransitionToXML() */



/**
 * Report reservation attribute as string.
 *
 *
 * @param R      (I)
 * @param AIndex (I)
 * @param Buf    (O)
 * @param Mode   (I) [bitmap of enum MCModeEnum]
 */

int MRsvTransitionAToString(

  mtransrsv_t       *R,
  enum MRsvAttrEnum  AIndex,
  mstring_t         *Buf,
  int                Mode)

  {
  char tmpLine[MMAX_LINE];

  if (R == NULL)
    {
    return(FAILURE);
    }

  switch (AIndex)
    {
    case mraAAccount:

      if (!MUStrIsEmpty(R->AAccount))
        MStringSet(Buf,R->AAccount);

      break;

    case mraAGroup:

      if (!MUStrIsEmpty(R->AGroup))
        MStringSet(Buf,R->AGroup);

      break;

    case mraAQOS:

      if (!MUStrIsEmpty(R->AQOS))
        MStringSet(Buf,R->AQOS);

      break;

    case mraAUser:

      if (!MUStrIsEmpty(R->AUser))
        MStringSet(Buf,R->AUser);

      break;

    case mraAllocNodeCount:

      MStringSetF(Buf,"%d",R->AllocNodeCount);

      break;

    case mraAllocProcCount:

      MStringSetF(Buf,"%d",R->AllocProcCount);

      break;

    case mraAllocTaskCount:

      MStringSetF(Buf,"%d",R->AllocTaskCount);

      break;

    case mraComment:

      if (!MUStrIsEmpty(R->Comment))
        MStringSet(Buf,R->Comment);

      break;

    case mraCost:

      MStringSetF(Buf,"%f",R->Cost);

      break;

    case mraCTime:

      MStringSetF(Buf,"%ld",R->CTime);

      break;

    case mraDuration:

      if (R->EndTime < MMAX_TIME - 1)
        MStringSetF(Buf,"%ld",R->Duration);
      else
        MStringSet(Buf,"INFINITY");

      break;

    case mraEndTime:

      MStringSetF(Buf,"%s",MULToTStringSeconds(R->EndTime,tmpLine,sizeof(tmpLine)));

      break;

    case mraExcludeRsv:

      if (!MUStrIsEmpty(R->ExcludeRsv))
        MStringSet(Buf,R->ExcludeRsv);

      break;

    case mraExpireTime:

      MStringSetF(Buf,"%ld",R->ExpireTime);

      break;

    case mraFlags:

      if (!MUStrIsEmpty(R->Flags))
        MStringSet(Buf,R->Flags);

      break;

    case mraGlobalID:

      if (!MUStrIsEmpty(R->GlobalID))
        MStringSet(Buf,R->GlobalID);

      break;

    case mraHostExp:

      if (!MUStrIsEmpty(R->HostExp))
        MStringSet(Buf,R->HostExp);

      break;

    case mraHistory:

      if (!MUStrIsEmpty(R->History))
        MStringSet(Buf,R->History);

      break;

    case mraLabel:

      if (!MUStrIsEmpty(R->Label))
        MStringSet(Buf,R->Label);

      break;

    case mraLastChargeTime:

      MStringSetF(Buf,"%ld",R->LastChargeTime);

      break;

    case mraLogLevel:

      if (!MUStrIsEmpty(R->LogLevel))
        MStringSet(Buf,R->LogLevel);

      break;

    case mraMaxJob:

      if (R->MaxJob > 0)
        MStringSetF(Buf,"%d",R->MaxJob);

      break;

    case mraMessages:

      if (!MUStrIsEmpty(R->Messages))
        MStringSet(Buf,R->Messages);

      break;

    case mraName:

      if (!MUStrIsEmpty(R->Name))
        MStringSet(Buf,R->Name);

      break;

    case mraOwner:

      if (!MUStrIsEmpty(R->Owner))
        MStringSet(Buf,R->Owner);

      break;

    case mraPartition:

      if (!MUStrIsEmpty(R->Partition))
        MStringSet(Buf,R->Partition);

      break;

    case mraPriority:

        MStringSetF(Buf,"%ld",R->Priority);

      break;

    case mraProfile:

      if (!MUStrIsEmpty(R->Profile))
        MStringSet(Buf,R->Profile);

      break;

    case mraReqArch:

      if (!MUStrIsEmpty(R->ReqArch))
        MStringSet(Buf,R->ReqArch);

      break;

    case mraReqFeatureList:

      if (!MUStrIsEmpty(R->ReqFeatureList))
        MStringSet(Buf,R->ReqFeatureList);

      break;

    case mraReqMemory:

      if (!MUStrIsEmpty(R->ReqMemory))
        MStringSet(Buf,R->ReqMemory);

      break;

    case mraReqNodeCount:

      if (!MUStrIsEmpty(R->ReqNodeCount))
        MStringSet(Buf,R->ReqNodeCount);

      break;

    case mraReqNodeList:

      if (!MUStrIsEmpty(R->ReqNodeList))
        MStringSet(Buf,R->ReqNodeList);

      break;

    case mraReqOS:

      if (!MUStrIsEmpty(R->ReqOS))
        MStringSet(Buf,R->ReqOS);

      break;

    case mraReqTaskCount:

      if (!MUStrIsEmpty(R->ReqTaskCount))
        MStringSet(Buf,R->ReqTaskCount);

      break;

    case mraReqTPN:

      if (!MUStrIsEmpty(R->ReqTPN))
        MStringSet(Buf,R->ReqTPN);

      break;

    case mraResources:

      if (!MUStrIsEmpty(R->Resources))
        MStringSet(Buf,R->Resources);

      break;

    case mraRsvAccessList:

      if (!MUStrIsEmpty(R->RsvAccessList))
        MStringSet(Buf,R->RsvAccessList);

      break;

    case mraRsvGroup:

      if (!MUStrIsEmpty(R->RsvGroup))
        MStringSet(Buf,R->RsvGroup);

      break;

    case mraRsvParent:

      if (!MUStrIsEmpty(R->RsvParent))
        MStringSet(Buf,R->RsvParent);

      break;

    case mraSID:

      if (!MUStrIsEmpty(R->SID))
        MStringSet(Buf,R->SID);

      break;

    case mraStartTime:

      MStringSetF(Buf,"%ld",R->StartTime);

      break;

    case mraStatCAPS:

      MStringSetF(Buf,"%.2f",R->StatCAPS);

      break;

    case mraStatCIPS:

      MStringSetF(Buf,"%.2f",R->StatCIPS);

      break;

    case mraStatTAPS:

      MStringSetF(Buf,"%.2f",R->StatTAPS);

      break;

    case mraStatTIPS:

      MStringSetF(Buf,"%.2f",R->StatTIPS);

      break;

    case mraSubType:

      if (!MUStrIsEmpty(R->SubType))
        MStringSet(Buf,R->SubType);

      break;

    case mraTrigger:

      if (!MUStrIsEmpty(R->Trigger))
        MStringSet(Buf,R->Trigger);

      break;

    case mraType:

      if (!MUStrIsEmpty(R->Type))
        MStringSet(Buf,R->Type);

      break;

    case mraVMList:

      if (!MUStrIsEmpty(R->VMList))
        MStringSet(Buf,R->VMList);

      break;

    default:

      return(FAILURE);

      /*NOTREACHED*/

      break;
    }  /* END switch (AIndex) */

  return(SUCCESS);
  }  /* END MRsvTransitionAToString() */




/**
 * Allocate storage for the given reservation transition object
 *
 * @param   RP (O) [alloc-ed] pointer to the newly allocated 
 *             reservation transition object
 */

int MRsvTransitionAllocate(

  mtransrsv_t **RP)

  {
  mtransrsv_t *R;

  if (RP == NULL)
    {
    return(FAILURE);
    }

  R = (mtransrsv_t *)MUCalloc(1,sizeof(mtransrsv_t));

  *RP = R;

  return(SUCCESS);
  }  /* END MRsvTransitionAllocate() */




/**
 * Free the storage for the given reservation transtion object
 *
 * @param   RP (I) [freed] pointer to the reservation transition
 *             object
 */

int MRsvTransitionFree(

  void **RP)

  {
  mtransrsv_t *R;

  if (RP == NULL)
    {
    return(FAILURE);
    }

  R = (mtransrsv_t *)*RP;

  MUFree(&R->CL);
  MUFree(&R->Comment);
  MUFree(&R->ExcludeRsv);
  MUFree(&R->Flags);
  MUFree(&R->GlobalID);
  MUFree(&R->HostExp);
  MUFree(&R->History);
  MUFree(&R->Label);
  MUFree(&R->ACL);
  MUFree(&R->AAccount);
  MUFree(&R->AGroup);
  MUFree(&R->AUser);
  MUFree(&R->AQOS);
  MUFree(&R->LogLevel);
  MUFree(&R->Messages);
  MUFree(&R->Owner);
  MUFree(&R->Partition);
  MUFree(&R->Profile);
  MUFree(&R->ReqArch);
  MUFree(&R->ReqFeatureList);
  MUFree(&R->ReqMemory);
  MUFree(&R->ReqNodeCount);
  MUFree(&R->ReqNodeList);
  MUFree(&R->ReqOS);
  MUFree(&R->ReqTaskCount);
  MUFree(&R->ReqTPN);
  MUFree(&R->Resources);
  MUFree(&R->RsvAccessList);
  MUFree(&R->RsvGroup);
  MUFree(&R->RsvParent);
  MUFree(&R->SID);
  MUFree(&R->SubType);
  MUFree(&R->Trigger);
  MUFree(&R->Type);
  MUFree(&R->VMList);

  /* destruct the AllocNodeList mstring */
  if (R->AllocNodeList != NULL)
    {
    delete R->AllocNodeList;
    R->AllocNodeList = NULL;
    }

  /* destruct the Variables  mstring */
  if (R->Variables != NULL)
    {
    delete R->Variables;
    R->Variables = NULL;
    }

  bmclear(&R->TFlags);

  /* free the actual reservation transition */
  MUFree((char **)RP);

  return(SUCCESS);
  }




/**
 * transition a reservation to the queue to be written to the 
 * database 
 *
 * @see MRsvToTransitionStruct (child) 
 *      MReqToTransitionStruct (child)
 *
 * @param   R (I) the reservation to be transitioned
 * @param   DeleteExisting (I) queue up a delete request
 */

int MRsvTransition(

  mrsv_t *R,
  mbool_t DeleteExisting)

  {
  mtransrsv_t *TR;

  if (R == NULL)
    return(SUCCESS);

  if (bmisset(&MSched.DisplayFlags,mdspfUseBlocking))
    return(SUCCESS);

#if defined (__NOMCOMMTHREAD)
  return(SUCCESS);
#endif

  MRsvTransitionAllocate(&TR);
  
  if (DeleteExisting == TRUE)
    {
    MUStrCpy(TR->Name,R->Name,sizeof(TR->Name));

    bmset(&TR->TFlags,mtransfDeleteExisting);
    }
  else
    {
    MRsvToTransitionStruct(R,TR);
    }

  MOExportTransition(mxoRsv,TR);

  return(SUCCESS);
  }  /* END MRsvTransition() */
/* END MRsvTransition.c */
