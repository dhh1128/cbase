
/* HEADER */

      
/**
 * @file MReqXML.c
 *
 * Contains:
 */


#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


int MReqAttrToXML(

  mxml_t       *E,
  ReqAttrArray &Req)

  {
  unsigned int jindex;

  mxml_t *Rules = NULL;
  mxml_t *RE = NULL;

  if ((E == NULL) || (Req.size() <= 0))
    {
    return(FAILURE);
    }

  MXMLCreateE(&Rules,"WorkloadProximityRules");

  for (jindex = 0;jindex < Req.size();jindex++)
    {
    RE = NULL;

    MXMLCreateE(&RE,"Rule");
    MXMLSetAttr(RE,"attribute",Req[jindex].Name.c_str(),mdfString);
    MXMLSetAttr(RE,"restriction",MAttrAffinityType[Req[jindex].Affinity],mdfString);
    MXMLSetAttr(RE,"operator",MComp[Req[jindex].Comparison],mdfString);
    MXMLSetAttr(RE,"value",Req[jindex].Value.c_str(),mdfString);
    MXMLAddE(Rules,RE);
    }

  MXMLAddE(E,Rules);

  return(SUCCESS);
  }  /* END MReqAttrToXML() */


int MReqAttrToString(

  ReqAttrArray &Req,
  mstring_t    *String)

  {
  unsigned int jindex;

  if ((String == NULL) || (Req.size() <= 0))
    {
    return(FAILURE);
    }

  for (jindex = 0;jindex < Req.size();jindex++)
    {
    if (jindex > 0)
      MStringAppend(String,",");

    MStringAppendF(String,"%s:%s%s%s",
      MAttrAffinityType[Req[jindex].Affinity],
      Req[jindex].Name.c_str(),
      (Req[jindex].Comparison != mcmpNONE) ? MComp[Req[jindex].Comparison] : ":",
      MUStrIsEmpty(Req[jindex].Value.c_str()) ? "" : Req[jindex].Value.c_str());
    }

  return(SUCCESS);
  }  /* END MReqAttrToString() */


int MReqAttrFromString(

  ReqAttrArray *ReqAttrArray,
  const char   *LineP)

  {
  char *Value = NULL;

  RequestedAttribute_t ReqAttr;
  
  char *ptr;
  char *TokPtr = NULL;
  
  char  AName[MMAX_NAME];
  char  AVal[MMAX_NAME];
  char *APtr;
  enum MCompEnum tmpCmp;
  
  if ((ReqAttrArray == NULL) || (MUStrIsEmpty(LineP)))
    {
    return(FAILURE);
    }

  MUStrDup(&Value,LineP);

  /* FORMAT:  REQATTR=[MUST:|SHOULD:|SHOULDNOT:|MUSTNOT:]X{<OP>}Y[,[MUST:|SHOULD:|SHOULDNOT:|MUSTNOT:]X{<OP>}Y]... */
  
  ptr = MUStrTok(Value,",",&TokPtr);
  
  while (ptr != NULL)
    {
    ReqAttr.Affinity = maatMust;
    ReqAttr.Comparison = mcmpNONE;
    ReqAttr.Name.clear();
    ReqAttr.Value.clear();
  
    if (!strncasecmp(ptr,"must:",strlen("must:")))
      {
      ReqAttr.Affinity = maatMust;
  
      ptr += strlen("must:");
      }
    else if (!strncasecmp(ptr,"should:",strlen("should:")))
      {
      ReqAttr.Affinity = maatShould;
  
      ptr += strlen("should:");
      }
    else if (!strncasecmp(ptr,"shouldnot:",strlen("shouldnot:")))
      {
      ReqAttr.Affinity = maatShouldNot;
  
      ptr += strlen("shouldnot:");
      }
    else if (!strncasecmp(ptr,"mustnot:",strlen("mustnot:")))
      {
      ReqAttr.Affinity = maatMustNot;
  
      ptr += strlen("mustnot:");
      }
  
    if (MUParseComp(ptr,AName,&tmpCmp,AVal,sizeof(AVal),TRUE) == SUCCESS)
      {
      if (MUStrIsEmpty(AName))
        {
        ptr = MUStrTok(NULL,",",&TokPtr);
  
        continue;
        }
  
      APtr = NULL;
  
      if (MUStrIsEmpty(AVal))
        {
        APtr = NULL;
        }
      else if (AVal[0] == ':')
        {
        /* value not specified as comparison */
  
        APtr = &AVal[1];
  
        if (tmpCmp == mcmpNONE)
          tmpCmp = mcmpSEQ;
        }
      else
        {
        APtr = AVal;
  
        if (tmpCmp == mcmpNONE)
          tmpCmp = mcmpSEQ;
        }

      if (tmpCmp == mcmpEQ)
        tmpCmp = mcmpSEQ;

      if (tmpCmp == mcmpNE)
        tmpCmp = mcmpSNE;
  
      ReqAttr.Name = AName;

      if (MUStrIsEmpty(APtr))
        ReqAttr.Value = "";
      else
        ReqAttr.Value = APtr;

      ReqAttr.Comparison = tmpCmp;
      
      ReqAttrArray->push_back(ReqAttr);
      }    /* END if (MUParseComp() == SUCCESS) */
  
    ptr = MUStrTok(NULL,",",&TokPtr);
    }  /* END while (ptr != NULL) */

  MUFree(&Value);

  return(SUCCESS);
  }  /* END MReqAttrFromString() */



int MReqAttrFromXML(

  mjob_t *J,
  mreq_t *RQ,
  mxml_t *RulesXML)

  {
  RequestedAttribute_t ReqAttr;

  mxml_t *CE = NULL;
  int     cindex = 0;

  if (RQ == NULL)
    RQ = J->Req[0];

  if ((J == NULL) ||
      (RQ == NULL) ||
      (RulesXML == NULL))
    return(FAILURE);

  if (strcmp(RulesXML->Name,"WorkloadProximityRules"))
    return(FAILURE);

  for (cindex = 0;cindex < RulesXML->CCount;cindex++)
    {
    char  Name[MMAX_LINE];
    char  Value[MMAX_LINE];
    char  Op[MMAX_LINE];
    char  Mode[MMAX_LINE];

    CE = RulesXML->C[cindex];

    ReqAttr.Affinity = maatMust;
    ReqAttr.Comparison = mcmpSEQ;
    ReqAttr.Name.clear();
    ReqAttr.Value.clear();

    if (CE == NULL)
      break;

    /* Get variable name */

    if ((MXMLGetAttr(CE,"attribute",NULL,Name,sizeof(Name)) == FAILURE) ||
        (MUStrIsEmpty(Name)))
      continue;

    ReqAttr.Name = Name;

    if ((MXMLGetAttr(CE,"value",NULL,Value,sizeof(Value)) == FAILURE) ||
        (MUStrIsEmpty(Value)))
      continue;

    ReqAttr.Value = Value;

    if (MXMLGetAttr(CE,"restriction",NULL,Mode,sizeof(Mode)) == SUCCESS)
      {
      ReqAttr.Affinity = (enum MAttrAffinityTypeEnum) MUGetIndexCI(Mode,MAttrAffinityType,FALSE,maatMust);
      }

    if (MXMLGetAttr(CE,"operator",NULL,Op,sizeof(Op)) == SUCCESS)
      {
      ReqAttr.Comparison = (enum MCompEnum)MUCmpFromString2(Op);
      }

    if (ReqAttr.Comparison == mcmpEQ)
      ReqAttr.Comparison = mcmpSEQ;
    else if (ReqAttr.Comparison == mcmpNE)
      ReqAttr.Comparison = mcmpSNE;

    RQ->ReqAttr.push_back(ReqAttr);
    }  /* END for (cindex) */

  return(SUCCESS);
  }  /* END MReqAttrFromXML() */







/**
 * Creates an XML representation of the given job requirement object.
 *
 * @see MJobToXML()
 * @see MReqFromXML()
 *
 * @param J (I)
 * @param RQ (I)
 * @param E (O)
 * @param SRQAList (I) [optional]
 * @param NullVal (I) [optional]
 */

int MReqToXML(

  mjob_t *J,        /* I */
  mreq_t *RQ,       /* I */
  mxml_t *E,        /* O */
  enum MReqAttrEnum *SRQAList, /* I (optional) */
  char   *NullVal)  /* I (optional) */

  {
  mbool_t DoReqAttr = FALSE;

  const enum MReqAttrEnum DRQAList[] = {
    mrqaIndex,
    mrqaNONE };

  int  aindex;

  int  rc;

  enum MReqAttrEnum *RQAList;

  const char *FName = "MReqToXML";

  MDB(4,fSTRUCT) MLog("%s(%s,%d,E,%s,%s)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (RQ != NULL) ? RQ->Index : -1,
    (SRQAList != NULL) ? "SRQAList" : "NULL",
    (NullVal != NULL) ? "NullVal" : "NULL");

  if ((J == NULL) || (RQ == NULL) || (E == NULL))
    {
    return(FAILURE);
    }

  mstring_t tmpString(MMAX_BUFFER);  /* must support large alloc node lists */

  if (SRQAList != NULL)
    RQAList = SRQAList;
  else
    RQAList = (enum MReqAttrEnum *)DRQAList;

  for (aindex = 0;RQAList[aindex] != mrqaNONE;aindex++)
    {
    if (RQAList[aindex] == mrqaLAST)
      continue;

    if (RQAList[aindex] == mrqaReqAttr)
      {
      DoReqAttr = TRUE;

      continue;
      }

    rc = MReqAToMString(
      J,
      RQ,
      RQAList[aindex],
      &tmpString,
      mfmXML);

    if ((rc == FAILURE) || (tmpString.empty()))
      {
      if (NullVal == NULL)
        continue;

      MStringSet(&tmpString,NullVal);
      }

    MXMLSetAttr(E,(char *)MReqAttr[RQAList[aindex]],tmpString.c_str(),mdfString);
    }  /* END for (aindex) */

  if (DoReqAttr == TRUE)
    {
    MReqAttrToXML(E,RQ->ReqAttr);
    }

  return(SUCCESS);
  }  /* END MReqToXML() */


/**
 * Extract Req from XML.
 *
 * @see MReqToXML()
 * @see MJobFromXML()
 *
 * @param J (I) [modified]
 * @param RQ (I) [modified]
 * @param E (I)
 */

int MReqFromXML(

  mjob_t *J,  /* I (modified) */
  mreq_t *RQ, /* I (modified) */
  mxml_t *E)  /* I */

  {
  mxml_t *CE = NULL;

  int CTok;
  int aindex;

  enum MReqAttrEnum rqaindex;

  const char *FName = "MReqFromXML";

  MDB(4,fSTRUCT) MLog("%s(%s,%s,%s)\n",
    FName,
    (J != NULL) ? "J" : "NULL",
    (RQ != NULL) ? "RQ" : "NULL",
    (E != NULL) ? "E" : "NULL");

  if ((J == NULL) || (RQ == NULL) || (E == NULL))
    {
    return(FAILURE);
    }

  /* NOTE:  do not initialize.  may be overlaying data */

  for (aindex = 0;aindex < E->ACount;aindex++)
    {
    rqaindex = (enum MReqAttrEnum)MUGetIndex(E->AName[aindex],MReqAttr,FALSE,0);

    if (rqaindex == mrqaNONE)
      continue;

    MReqSetAttr(J,RQ,rqaindex,(void **)E->AVal[aindex],mdfString,mSet);
    }  /* END for (aindex) */

  /* load children */

  CTok = -1;

  while (MXMLGetChild(E,NULL,&CTok,&CE) == SUCCESS)
    {
    if (!strcasecmp(CE->Name,"WorkloadProximityRules"))
      {
      MReqAttrFromXML(J,RQ,CE);
      }
    }


  return(SUCCESS);
  }  /* END MReqFromXML() */

