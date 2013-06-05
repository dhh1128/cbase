/* HEADER */

/**
 * @file MACL.c
 *
 * Moab Access Control Lists 
 */

/* contains:
 *
 * char *MACLListShow(ACL,OType,Mode,Buffer,BufSize) 
 * char *MACLShow(ACL,MFormatBM,ShowObject)
 * int MACLLoadConfigLine(ACL,ACLLine,Op,OMap,DoReverse) 
 *
 */

 
#include "moab.h"
#include "moab-proto.h"
#include "moab-global.h"
#include "moab-const.h"
 


/**
 * Initialize an macl_t structure.
 *
 * @param ACL
 */

int MACLInit(

  macl_t *ACL)

  {
  if (ACL == NULL)
    {
    return(FAILURE);
    }

  memset(ACL,0,sizeof(macl_t));

  return(SUCCESS);
  }  /* END MACLInit() */


/**
 * Free a list of ACLs.
 *
 * @param ACLList [I] - The list to free
 */

int MACLFreeList(

  macl_t **ACLList)

  {
  macl_t *tmpACL;
  macl_t *nextACL;

  if (ACLList == NULL)
    {
    return(SUCCESS);
    }

  tmpACL = *ACLList;

  while (tmpACL != NULL)
    {
    nextACL = tmpACL->Next;

    MACLFree(&tmpACL);

    tmpACL = nextACL;
    }

  *ACLList = NULL;

  return(SUCCESS);
  } /* END MACLFreeList */


/**
 * Free a single ACL.
 *
 * @param ACL
 */

int MACLFree(

  macl_t **ACL)

  {
  macl_t *tACL;

  if ((ACL == NULL) || (*ACL == NULL))
    {
    return(SUCCESS);
    }

  tACL = *ACL;

  if (tACL->Name != NULL)
    MUFree(&tACL->Name);

  bmclear(&tACL->Flags);

  MUFree((char **)ACL);

  return(SUCCESS);
  }  /* END MACLFree() */


/**
 * Test if an ACL is empty.
 *
 * @param ACL
 */

mbool_t MACLIsEmpty(

  macl_t *ACL)

  {
  if (ACL == NULL)
    return(TRUE);

  if (ACL->Type == maNONE)
    return(TRUE);

  return(FALSE);
  }  /* END MACLIsEmpty() */


/**
 * Report ACL list as string.
 *
 * NOTE:  There is also an MACLListShowMString()
 *
 * @param ACL (I) [modified]
 * @param OType (I)
 * @param MFormatBM (I)
 * @param Buffer (O) [required]
 * @param BufSize (I)
 */

int MACLListShow(

  macl_t        *ACL,
  enum MAttrEnum OType,
  mbitmap_t     *MFormatBM,
  char          *Buffer,
  int            BufSize)
 
  {
  char ACLLine[MMAX_LINE];

  int oindex;

  char *ptr;

  char *BPtr = NULL;
  int   BSpace = 0;

  macl_t *tACL;

  mbool_t ACLFound = FALSE;
 
  /* OType != maNONE Format:  <A>[:<B>]... */
  /*   or                     <OBJECT>=<A> <OBJECT>=<B>... */

  if (Buffer != NULL)
    Buffer[0] = '\0';

  if ((ACL == NULL) || (Buffer == NULL))
    {
    return(FAILURE);
    }

  MUSNInit(&BPtr,&BSpace,Buffer,BufSize);

  mstring_t tmpLine(MMAX_LINE);  /* Construct a mstring_t instance */

  for (oindex = maNONE + 1;oindex < maLAST;oindex++)
    {
    if ((OType != maNONE) && (OType != (enum MAttrEnum)oindex))
      continue;

    /* Start off with a cleared string instance on every interation */
    tmpLine.clear();

    tACL = ACL;

    while (tACL != NULL)
      {
      if (tACL->Type != (enum MAttrEnum)oindex)
        {
        tACL = tACL->Next;

        continue;
        }

      ACLFound = TRUE;

      ptr = MACLShow(tACL,MFormatBM,FALSE,NULL,FALSE,ACLLine);

      if (!tmpLine.empty())
        {
        tmpLine += ':';
        }

      tmpLine += ptr;

      tACL = tACL->Next;
      }  /* END while() */
 
    if (!tmpLine.empty())
      {
      if ((OType == maNONE) || bmisset(MFormatBM,mfmVerbose))
        {
        MUSNCat(&BPtr,&BSpace,(char *)MAttrO[oindex]);

        if ((tmpLine[0] != '=') &&
            (tmpLine[0] != '<') &&
            (tmpLine[0] != '>'))
          {
          MUSNCat(&BPtr,&BSpace,"=");
          }

        MUSNCat(&BPtr,&BSpace,tmpLine.c_str());
        }
      else
        {
        if (!strncmp(tmpLine.c_str(),"==",2))
          MUSNCat(&BPtr,&BSpace,&tmpLine.c_str()[2]); 
        else
          MUSNCat(&BPtr,&BSpace,tmpLine.c_str());
        }

      if (bmisset(MFormatBM,mfmAVP))
        MUSNCat(&BPtr,&BSpace,";");
      else
        MUSNCat(&BPtr,&BSpace," ");
      }
    }    /* END for (oindex) */

  if (ACLFound == FALSE)
    {
    if (bmisset(MFormatBM,mfmAVP))
      MUSNCat(&BPtr,&BSpace,NONE);
    }

  return(SUCCESS);
  }  /* END MACLListShow() */



 /**
 * Report ACL list as mstring_t.
 *
 * @param ACL (I) [modified]
 * @param OType (I)
 * @param MFormatBM (I)
 * @param Buffer (O) [required]
 */

int MACLListShowMString(

  macl_t        *ACL,       /* I */
  enum MAttrEnum OType,     /* I (maNONE -> ALL) */
  mbitmap_t     *MFormatBM, /* I */
  mstring_t     *Buffer)    /* O (required) */
 
  {
  char ACLLine[MMAX_LINE];

  int oindex;

  char *ptr;


  macl_t *tACL;

  mbool_t ACLFound = FALSE;
 
  /* OType != maNONE Format:  <A>[:<B>]... */
  /*   or                     <OBJECT>=<A> <OBJECT>=<B>... */

  if ((ACL == NULL) || (Buffer == NULL))
    {
    return(FAILURE);
    }


  /* Default constructor will init the memory */
  mstring_t tmpLine(MMAX_LINE);

  for (oindex = maNONE + 1;oindex < maLAST;oindex++)
    {
    if ((OType != maNONE) && (OType != (enum MAttrEnum)oindex))
      continue;

    tmpLine.clear();   /* reset the string to a NULL string */

    for (tACL = ACL;tACL != NULL;tACL = tACL->Next)
      {
      if (tACL->Type != (enum MAttrEnum)oindex)
        {
        continue;
        }

      ACLFound = TRUE;

      ptr = MACLShow(tACL,MFormatBM,FALSE,NULL,FALSE,ACLLine);

      if (!tmpLine.empty())
        {
        tmpLine += ':';
        }

      tmpLine += ptr;
      }  /* END for (tACL...) */
 
    if (!tmpLine.empty())
      {
      if ((OType == maNONE) || bmisset(MFormatBM,mfmVerbose))
        {
        MStringAppend(Buffer,(char *)MAttrO[oindex]);

        if ((tmpLine[0] != '=') &&
            (tmpLine[0] != '<') &&
            (tmpLine[0] != '>'))
          {
          MStringAppend(Buffer,"=");
          }

        MStringAppend(Buffer,tmpLine.c_str());
        }
      else
        {
        if (!strncmp(tmpLine.c_str(),"==",2))
          MStringAppend(Buffer,&tmpLine.c_str()[2]); 
        else
          MStringAppend(Buffer,tmpLine.c_str());
        }

      if (bmisset(MFormatBM,mfmAVP))
        MStringAppend(Buffer,";");
      else
        MStringAppend(Buffer," ");
      }
    }    /* END for (oindex) */

  if (ACLFound == FALSE)
    {
    if (bmisset(MFormatBM,mfmAVP))
      MStringAppend(Buffer,NONE);
    }

  return(SUCCESS);
  }  /* END MACLListShowMString() */

 
/**
 * Report a single ACL entry as string.
 *
 * @param ACL (I) [modified]
 * @param MFormatBM (I)
 * @param ShowObject (I)
 * @param OMap (I) [optional] - everything mapped when present
 * @param DoReverse (I)
 * @param Line (O) - the string that is returned
 */
     
char *MACLShow(     
 
  macl_t       *ACL,
  mbitmap_t    *MFormatBM,
  mbool_t       ShowObject,
  momap_t      *OMap,
  mbool_t       DoReverse,
  char         *Line)
 
  {
  int  cindex;
 
  char ACLString[MMAX_NAME << 1];
  char ModString[MMAX_NAME];
 
  if (Line != NULL)
    Line[0] = '\0';
 
  if (Line == NULL)
    {
    return(FAILURE);
    }

  if (ShowObject == TRUE)
    strcpy(ACLString,MHRObj[(int)ACL->Type]);
  else
    ACLString[0] = '\0';

  if (bmisset(MFormatBM,mfmVerbose))
    {
    cindex = 0;
 
    switch ((int)ACL->Affinity)
      {
      case mnmPositiveAffinity:
 
        /* default */
 
        ModString[cindex++] = '+';

        break;

      case mnmNeutralAffinity:
 
        ModString[cindex++] = '=';

        break;
 
      case mnmNegativeAffinity:
 
        ModString[cindex++] = '-';
 
        break;

      case mnmNONE:

        /* NOTE:  ACL affinity not specified in J->RCL (required cred list) */

        /* NO-OP */

        break;
 
      case mnmUnavailable:
      case mnmRequired:
      default:
 
        ModString[cindex++] = '?';

        break;
      }  /* END switch ((int)ACL.Affinity) */
 
    if (bmisset(&ACL->Flags,maclRequired))
      ModString[cindex++] = '*';
 
    if (bmisset(&ACL->Flags,maclDeny))
      ModString[cindex++] = '!';
 
    if (bmisset(&ACL->Flags,maclXOR))
      ModString[cindex++] = '^';

    if (bmisset(&ACL->Flags,maclCredLock))
      ModString[cindex++] = '&';

    if (bmisset(&ACL->Flags,maclHPEnable))
      ModString[cindex++] = '~';
 
    ModString[cindex] = '\0';
    }
  else
    {
    ModString[0] = '\0';
    }

  cindex = ACL->Cmp;

  if (bmisset(MFormatBM,mfmHuman))
    {
    if (cindex == mcmpSEQ)
      cindex = mcmpEQ;
    else if (cindex == mcmpSNE)
      cindex = mcmpNE;
    }

  switch (ACL->Cmp)
    {
    case mcmpSEQ:
    case mcmpSSUB:
    case mcmpSNE:
    default:

      if (OMap != NULL)
        {
        char tmpLine[MMAX_LINE];

        enum MXMLOTypeEnum OIndex;

        switch (ACL->Type)
          {
          case maUser:  OIndex = mxoUser;  break;
          case maGroup: OIndex = mxoGroup; break;
          case maAcct:  OIndex = mxoAcct;  break;
          case maQOS:   OIndex = mxoQOS;   break;
          case maClass: OIndex = mxoClass; break;
          case maJob:   OIndex = mxoJob;   break;
          case maRsv:   OIndex = mxoRsv;   break;
          case maVC:    OIndex = mxoxVC;   break;

          default:

            OIndex = mxoNONE;

            break;
          }  /* END switch (ObjType) */

        if ((OIndex != mxoNONE) &&
            (MOMap(OMap,OIndex,ACL->Name,DoReverse,FALSE,tmpLine) == SUCCESS))
          {
          sprintf(ACLString,"%s%s%s%s",
            ACLString,
            MComp[cindex],
            tmpLine,
            ModString);
          } 
        else
          {
          sprintf(ACLString,"%s%s%s%s",
            ACLString,
            MComp[cindex],
            ACL->Name,
            ModString);
          }
        }
      else
        {
        sprintf(ACLString,"%s%s%s%s",
          ACLString,
          MComp[cindex],
          ACL->Name,
          ModString);
        }
 
      break;
 
    case mcmpLT:
    case mcmpLE:
    case mcmpEQ:
    case mcmpNE:
    case mcmpGE:
    case mcmpGT: 

      if ((ACL->Type == maDuration) && bmisset(MFormatBM,mfmHuman))
        {
        /* human readable */

        char TString[MMAX_LINE];

        MULToTString(ACL->LVal,TString);

        sprintf(ACLString,"%s%s%s%s",
          ACLString,
          MComp[cindex],
          TString,
          ModString);
        }
      else
        { 
        sprintf(ACLString,"%s%s%ld%s",
          ACLString,
          MComp[cindex],
          ACL->LVal,
          ModString);
        }

      break;
    }   /* END switch (ACL->Cmp) */

  if (bmisset(MFormatBM,mfmHuman))
    { 
    if (Line[0] != '\0')
      strcat(Line," ");
    }
  else if (bmisset(MFormatBM,mfmAVP))
    {
    if (Line[0] != '\0')
      strcat(Line,":");
    }
 
  strcat(Line,ACLString);

  if (bmisset(MFormatBM,mfmAVP) && (Line[0] == '\0'))
    strcpy(Line,NONE);
     
  return(Line);
  }  /* END MACLShow() */ 
 


 
 
/**
 * Parse ACL config line.
 *
 * @see MACLFromString() - child
 *
 * @param ACL (I) [modified]
 * @param ACLLine (I)
 * @param Op (I)
 * @param OMap (I) [optional]
 * @param DoReverse (I)
 */

int MACLLoadConfigLine(

  macl_t                 **ACL,       /* I (modified) */
  char                    *ACLLine,   /* I */
  enum MObjectSetModeEnum  Op,        /* I */
  momap_t                 *OMap,      /* I (optional) */
  mbool_t                  DoReverse) /* I */

  {
  char *ptr;
  char *TokPtr = NULL;

  int   oindex;

  macl_t *tmpACL = NULL;

  const char *ACLDelim = " \t\n;,";

  /* FORMAT:  <OBJTYPE><CMP><VAL>[<MODIFIER>][{<WS>:;,}<OBJTYPE><CMP><VAL>[<MODIFIER>]] */

  /* ie, USER==john+:==steve+;,... */

  ptr = MUStrTok(ACLLine,ACLDelim,&TokPtr);

  while (ptr != NULL)
    {
    if ((oindex = MUGetIndexCI(ptr,MAttrO,TRUE,0)) != 0)
      {
      if (MACLFromString(&tmpACL,ptr + strlen(MAttrO[oindex]),1,(enum MAttrEnum)oindex,mAdd,OMap,DoReverse) == FAILURE)
        {
        /* Free all of the ACLs */

        MACLFreeList(&tmpACL);

        return(FAILURE);
        }
      }
    else
      {
      /* Free all of the ACLs - bad type given */

      MACLFreeList(&tmpACL);

      return(FAILURE);
      }

    ptr = MUStrTok(NULL,ACLDelim,&TokPtr);        
    }  /* END while (ptr != NULL) */

  switch (Op)
    {
    case mAdd:

      MACLMerge(ACL,tmpACL);

      break;

    case mDecr:

      MACLSubtract(ACL,tmpACL);

      break;

    default:

      /* mSet */

      MACLCopy(ACL,tmpACL);

      break;
    }  /* END switch (Op) */

  /* Free the temporary ACLs */

  MACLFreeList(&tmpACL);

  return(SUCCESS);
  }  /* END MACLLoadConfigLine() */





/**
 * Report ACL modifiers as a comma delimited string.
 *
 * NOTE:  Will initialize string to empty.  May return empty string if no flags set.
 *
 *
 * @param ACL     (I) [optional]
 * @param SFlags  (I) [optional, -1 = not set]
 * @param Buf     (I)
 * @param BufSize (I)
 */
int MACLFlagsToString(

  macl_t     *ACL,
  mbitmap_t  *SFlags,
  char       *Buf,
  int         BufSize)

  {
  
  char *BPtr = NULL;
  int   BSpace = 0;

  int   index;

  mbitmap_t *Flags=NULL;

  if (SFlags != NULL)
    {
    Flags = SFlags;
    }
  else if (ACL != NULL)
    {
    Flags = &ACL->Flags;
    }

  MUSNInit(&BPtr,&BSpace,Buf,BufSize);

  for (index = 0;MACLFlags[index] != NULL;index++)
    {
    if (!bmisset(Flags,index))
      continue;

    if (Buf[0] != '\0')
      MUSNCat(&BPtr,&BSpace,",");

    MUSNCat(&BPtr,&BSpace,(char *)MACLFlags[index]);
    }    /* END for (index) */

  return(SUCCESS);
  }      /* END MJobFlagsToString() */





/**
 * Process single ACL expression.
 *
 * NOTE: process single <CREDTYPE> clause: ie "USER==a:b"
 *       but not "USER==a:b,GROUP==c"
 *
 * NOTE: appends new ACL's to ACL linked list
 * NOTE: will clear other ACL's of same object type if '==' operator is 
 *       specified *UNLESS* Op==mAdd (like in MSRProcessConfig)
 *
 * @see MACLLoadConfigLine() - parent
 * @see MRsvSetAttr() - parent
 *
 * If the given command is "mrsvctl -m -a USER==a:b,GROUP==c rsvName",
 * Then this function will be called twice, the first with ACLList containing
 *  "==a:b" and ObjType as maUser, then with ACLList containing "==c" and 
 *  ObjType as maGroup.
 *
 * @param ACL (O)
 * @param ACLList (I)
 * @param ACLCount (I)
 * @param ObjType (I)
 * @param Op (I)
 * @param OMap (I) [optional]
 * @param DoReverse (I)
 */

int MACLFromString(
 
  macl_t                 **ACL,       /* O */
  char                    *ACLList,   /* I */
  int                      ACLCount,  /* I */
  enum MAttrEnum           ObjType,   /* I */
  enum MObjectSetModeEnum  Op,        /* I (one of mIncr, mSet, mDecr) */
  momap_t                 *OMap,      /* I (optional) */
  mbool_t                  DoReverse) /* I */ 

  {
  int index;
  int len;

  char tmpLine[MMAX_LINE];

  enum MXMLOTypeEnum OIndex;
 
  char *ptr;
  char *ptr2;
  char *tail;

  int   PtrCount;
 
  char *TokPtr;
  
  macl_t  tACL;

  const char  *DelimList;

  const char  *FullDelim = "|,:";
  const char  *PartDelim = "|,";

  const char *FName = "MACLFromString";

  MDB(7,fCONFIG) MLog("%s(ACL,%s,%d,%s,Op,OMap,%s)\n",
    FName,
    (ACLList != NULL) ? ACLList : "NULL",
    ACLCount,
    MAttrO[ObjType],
    MBool[DoReverse]);
 
  /* FORMAT: [<CMP>]{[<MODIFIER>]<OBJECTVAL>[<AFFINITY>]}[{:,}{[<MODIFIER>]<OBJECTVAL>[<AFFINITY>]}]... */
 
  if ((ACL == NULL) || (ACLList == NULL))
    {
    return(FAILURE);
    }

  if ((ACLList == NULL) || 
      (ACLList[0] == '\0') || 
      strstr(ACLList,NONE))
    {
    /* ACL string is empty */

    return(SUCCESS);
    }

  PtrCount = 0;

  if ((ObjType == maDuration) || (MUStrStr(ACLList,"peer:",0,TRUE,FALSE) != NULL))
    DelimList = PartDelim;
  else
    DelimList = FullDelim;

  ptr = MUStrTok(ACLList,DelimList,&TokPtr);

  if ((ptr != NULL) && (ptr[0] == ':'))
    {
    /* remove ACL type-value separator, ie user:steve */

    ptr++;
    }

  while (ptr != NULL)
    {
    memset(&tACL,0,sizeof(tACL));

    tACL.Type = (enum MAttrEnum)ObjType;

    /* extract affinity */

    tACL.Affinity = mnmPositiveAffinity;

    switch (ObjType)
      {
      case maUser:  OIndex = mxoUser;  break;
      case maGroup: OIndex = mxoGroup; break;
      case maAcct:  OIndex = mxoAcct;  break;
      case maRsv:   OIndex = mxoRsv;   break;
      case maQOS:   OIndex = mxoQOS;   break;
      case maClass: OIndex = mxoClass; break;
      case maJob:   OIndex = mxoJob;   break;
      case maVC:    OIndex = mxoxVC;   break;

      default:

        OIndex = mxoNONE;

        break;
      }  /* END switch (ObjType) */

    /* exmaine ACL suffix */

    /* NOTE: ! and & are mutually exclusive.  Setting one will unset the other. */

    for (tail = ptr + strlen(ptr) - 1;tail > ptr;tail--)
      {
      if ((*tail == '\0') || (strchr("-&+=%^!*~",*tail) == NULL))
        break;

      switch (*tail)
        {
        case '-':

          *tail = '\0'; 

          tACL.Affinity = mnmNegativeAffinity;

          break;

        case '=':

          *tail = '\0';

          tACL.Affinity = mnmNeutralAffinity;

          break;

        case '+':

          *tail = '\0';

          tACL.Affinity = mnmPositiveAffinity;

          break;

        case '!':

          *tail = '\0';

          bmset(&tACL.Flags,maclDeny);

          bmunset(&tACL.Flags,maclCredLock);

          break;

        case '^':

          *tail = '\0';

          bmset(&tACL.Flags,maclXOR);

          bmunset(&tACL.Flags,maclCredLock);

          break;

        case '*':

          *tail = '\0';

          bmset(&tACL.Flags,maclRequired);

          break;

        case '&':

          *tail = '\0';

          bmset(&tACL.Flags,maclCredLock);

          bmunset(&tACL.Flags,maclDeny);
          bmunset(&tACL.Flags,maclXOR);

          break;

        case '~':

          *tail = '\0';

          bmset(&tACL.Flags,maclHPEnable);

          MSched.HPEnabledRsvExists = TRUE;

          break;

        default: 

          /* Ignore other characters */
           
          /* NO-OP */

          break;
        }  /* END switch (*tail) */
      }    /* END for (tail) */

    /* extract modifier */

    /* FORMAT:  [<CMD>][!^*&][<CMP>]<ACLVAL> */

    if ((ptr[0] == '=') && (strchr("<>!=%",ptr[1])))
      ptr++;

    ptr2 = ptr;             

    if ((index = MUCmpFromString(ptr2,&len)) != 0)
      {
      tACL.Cmp = index;

      ptr2 += len; 
      }
    else
      {
      switch (ObjType)
        {
        case maDuration:
        case maTask:
        case maProc:
        case maPS:

          tACL.Cmp = mcmpLE;

          break;

        case maMemory:

          tACL.Cmp = mcmpGE;

          break;

        default:

          tACL.Cmp = mcmpSEQ;

          break;
        }  /* END switch (ObjType) */
      }    /* END else ((index = MUCmpFromString(ptr2,&len)) != 0) */

    if (tACL.Cmp == mcmpDECR)
      {
      Op = mDecr;
      }

    while ((strchr("-&+=^!*~",ptr2[0]) != NULL) && (ptr2[0] != '\0'))
      {
      if (ptr2[0] == '-')
        {
        tACL.Affinity = mnmNegativeAffinity;

        ptr2++;
        }
      else if (ptr2[0] == '+')
        {
        tACL.Affinity = mnmPositiveAffinity;

        ptr2++;
        }
      else if (ptr2[0] == '=')
        {
        tACL.Affinity = mnmNeutralAffinity;

        ptr2++;
        }
      else if (ptr2[0] == '!')
        {
        bmset(&tACL.Flags,maclDeny);
 
        bmunset(&tACL.Flags,maclCredLock);  

        ptr2++;
        }
      else if (ptr2[0] == '^')
        {
        bmset(&tACL.Flags,maclXOR);

        bmunset(&tACL.Flags,maclCredLock);
 
        ptr2++;
        }
      else if (ptr2[0] == '*')
        {
        bmset(&tACL.Flags,maclRequired);
 
        ptr2++;
        }
      else if (ptr2[0] == '&')
        {
        bmset(&tACL.Flags,maclCredLock);
 
        bmunset(&tACL.Flags,maclDeny);
        bmunset(&tACL.Flags,maclXOR);

        ptr2++;
        }
      else if (ptr2[0] == '~')
        {
        bmset(&tACL.Flags,maclHPEnable);

        MSched.HPEnabledRsvExists = TRUE;
 
        ptr2++;
        }
      }

    switch (ObjType)
      {
      case maDuration:
      case maMemory:
      case maProc:
      case maPS:
      case maTask:

        /* do nothing */

        break;

      default:

        if (tACL.Cmp == mcmpEQ)
          tACL.Cmp = mcmpSEQ;
        else if (tACL.Cmp == mcmpNE)
          tACL.Cmp = mcmpSNE;

        break;       
      }  /* END switch (ObjType) */

    if ((OMap != NULL) && 
        (OIndex != mxoNONE) &&
        (MOMap(OMap,OIndex,ptr2,DoReverse,FALSE,tmpLine) == SUCCESS))
      {
      /* NO-OP */
      }
    else
      {
      MUStrCpy(tmpLine,ptr2,sizeof(tmpLine));
      }

    switch (tACL.Cmp)
      {
      case mcmpSEQ:
      case mcmpSNE:
      case mcmpSSUB:

        MUStrDup(&tACL.Name,tmpLine);

        break;

      default:

        MUStrDup(&tACL.Name,tmpLine);

        if (ObjType == maDuration)
          tACL.LVal = MUTimeFromString(ptr2);
        else 
          tACL.LVal = strtol(tmpLine,NULL,0);      

        break;
      }  /* END switch (ObjType) */

    if (OIndex != mxoNONE)
      {
      if (MOGetObject(OIndex,tmpLine,&tACL.OPtr,mAdd) == SUCCESS)
        {
        if (OIndex == mxoQOS)
          {
          mqos_t *Q = (mqos_t *)tACL.OPtr;

          /* check if we should set conditional flag on tACL */

          if ((Q->QTACLThreshold > 0) || 
              (Q->XFACLThreshold > 0.0) ||
              (Q->BLACLThreshold > 0.0))
            {
            bmset(&tACL.Flags,maclConditional);
            }
          }
        }
      else
        {
        MDB(3,fCONFIG) MLog("ALERT:    cannot add %s '%s' in %s\n",
          MXO[OIndex],
          tmpLine,
          FName);
        }
      }  /* END if (OIndex != mxoNONE) */

    /* If tACL.Cmp is "==" and it is the first time in the while loop
       (the first if in a list), clear the ACL of all credentials
       of the same type */

    if ((tACL.Cmp == mcmpSEQ) && (PtrCount == 0) && (Op != mAdd))
      {
      MACLRemove(ACL,tACL.Type,NULL);  
      }
    else if (tACL.Type == maDuration)
      {
      MACLRemove(ACL,tACL.Type,NULL);  
      }
    else if ((tACL.Cmp == mcmpINCR) && (OIndex != mxoNONE))
      {
      tACL.Cmp = mcmpSEQ; /* Replace the += with == for looks n reports.*/
      }

    MACLSetACL(ACL,&tACL,TRUE);

    MDB(7,fCONFIG) MLog("INFO:     ACL loaded with %s %s (Affinity: %d)\n",
      MAttrO[ObjType],
      (OIndex == mxoNONE) ? ptr2 : tmpLine,
      tACL.Affinity);

    MUFree(&tACL.Name);

    ptr = MUStrTok(NULL,DelimList,&TokPtr);

    PtrCount++;
    }  /* END while (ptr != NULL) */
 
  return(SUCCESS);
  }  /* END MACLFromString() */




/**
 * Set an individual ACL attribute.
 *
 * @param A      (I)
 * @param AIndex (I)
 * @param Value  (I)
 * @param Format (I)
 * @param Mode   (I)
 */

int MACLSetAttr(

  macl_t                 *A,
  enum MACLAttrEnum       AIndex,
  void                  **Value,
  enum MDataFormatEnum    Format,
  enum MObjectSetModeEnum Mode)

  {
  switch (AIndex)
    {
    case maclaAffinity:

      if ((Format == mdfString) && ((char *)Value != NULL))
        {
        A->Affinity = (enum MNodeAffinityTypeEnum)MUGetIndexCI(
          (char *)Value,
          MAffinityType,
          FALSE,
          mnmNONE);
        }

      break;

    case maclaComparison:

      if ((Format == mdfString) && ((char *)Value != NULL))
        {
        A->Cmp = (enum MCompEnum)MUGetIndexCI(
          (char *)Value,
          MComp,
          FALSE,
          mcmpNONE);
        }

      break;

    case maclaDValue:      /* conditional value (double) */

      if ((Format == mdfString) && ((char *)Value != NULL))
        {
        A->DVal = strtod((char *)Value,NULL);
        }

      break;

    case maclaFlags:

      if ((Format == mdfString) && ((char *)Value != NULL))
        {
        bmfromstring((char *)Value,MACLFlags,&A->Flags);
        }

      break;

    case maclaName:

      if ((Format == mdfString) && ((char *)Value != NULL))
        {
        MUStrDup(&A->Name,(char *)Value);
        }
    
      break;

    case maclaType:

      if ((Format == mdfString) && ((char *)Value != NULL))
        {
        A->Type = (enum MAttrEnum)MUGetIndexCI(
          (char *)Value,
          MAttrO,
          FALSE,
          maNONE);
        }

      break;

    case maclaValue:

      if ((Format == mdfString) && ((char *)Value != NULL))
        {
        A->LVal = strtol((char *)Value,NULL,10);
        }

      break;

    default:

      /* NO-OP */

      break;
    }  /* END switch (AIndex) */

  return(SUCCESS);
  }  /* END MACLAToString() */





/**
 * Convert ACL attribute to string.
 *
 * @param A (I)
 * @param AIndex (I)
 * @param Buf (O)
 * @param Mode (I)
 */

int MACLAToString(

  macl_t              *A,      /* I */
  enum MACLAttrEnum    AIndex, /* I */
  char                *Buf,    /* O */
  enum MFormatModeEnum Mode)   /* I */

  {
  /* int BufSize = MMAX_LINE; */
  char FlagBuf[MMAX_LINE];

  if ((A == NULL) || (Buf == NULL))
    {
    return(FAILURE);
    }

  Buf[0] = '\0';

  switch (AIndex)
    {
    case maclaAffinity:

      /* Always print affinity so reading in doesn't require assumptions */

      strcpy(Buf,MAffinityType[(int)A->Affinity]);

      break;

    case maclaComparison:

      if (A->Cmp != '\0')
        {
        strcpy(Buf,MComp[(int)A->Cmp]);
        }

      break;

    case maclaDValue:      /* conditional value (double) */

      if (A->DVal != 0)
        {
        sprintf(Buf,"%f",
          A->DVal);
        }

      break;

    case maclaFlags:

      if (!bmisclear(&A->Flags))
        {
        MACLFlagsToString(A,NULL,FlagBuf,sizeof(FlagBuf));
        strcpy(Buf,FlagBuf);
        }

      break;

    case maclaName:

      if ((A->Name != NULL) && (A->Name[0] != '\0'))
        strcpy(Buf,A->Name);
    
      break;

    case maclaType:

      if (A->Type != maNONE)
        strcpy(Buf,MAttrO[(int)A->Type]);

      break;

    case maclaValue:

      if (A->LVal != 0)
        {
        sprintf(Buf,"%ld",
          A->LVal);
        }

      break;

    default:

      /* NO-OP */

      break;
    }  /* END switch (AIndex) */

  return(SUCCESS);
  }  /* END MACLAToString() */




/**
 * Convert a single macl_t to XML
 *
 * NOTE: EP must be freed by caller
 *
 * @param ACL     (I)
 * @param EP      (O)
 * @param SAList  (I) [optional]
 * @param IsCL    (I)
 */

int MACLToXML(
 
  macl_t             *ACL,    /* I */
  mxml_t            **EP,     /* O */
  enum MACLAttrEnum  *SAList, /* I (optional) */
  mbool_t             IsCL)   /* I */

  {
  mxml_t *AE = NULL;

  enum MACLAttrEnum DAList[] = {
    maclaAffinity,
    maclaComparison,
    maclaDValue,     
    maclaFlags,
    maclaName,
    maclaType,
    maclaValue,
    maclaNONE };

  int  aindex;

  enum MACLAttrEnum *AList;

  char tmpString[MMAX_BUFFER];

  if ((ACL == NULL) || (EP == NULL))
    {
    return(FAILURE);
    }

  if (SAList != NULL)
    AList = SAList;
  else
    AList = DAList;

  if (IsCL)
    {
    MXMLCreateE(&AE,(char *)"CL");
    }
  else
    {
    MXMLCreateE(&AE,(char *)"ACL");
    }

  for (aindex = 0;AList[aindex] != maclaNONE;aindex++)
    {
    if ((MACLAToString(ACL,AList[aindex],tmpString,mfmNONE) == FAILURE) ||
        (tmpString[0] == '\0'))
      {
      continue;
      }

    MXMLSetAttr(AE,(char *)MACLAttr[AList[aindex]],tmpString,mdfString);
    }

  *EP = AE;

  return(SUCCESS);
  }  /* END MACLToXML() */





/*
 * Reads a single <ACL></ACL> xml entry and adds it to the list.
 *
 */

int MACLFromXML(
  
  macl_t     **A,
  mxml_t      *E,
  mbool_t      CreateCreds)

  {
  int aindex;

  enum MACLAttrEnum pindex;

  macl_t ACL;

  MACLInit(&ACL);

  for (aindex = 0;aindex < E->ACount;aindex++)
    {
    pindex = (enum MACLAttrEnum)MUGetIndex(E->AName[aindex],MACLAttr,FALSE,0);

    if (pindex == maclaNONE)
      continue;

    MACLSetAttr(&ACL,pindex,(void **)E->AVal[aindex],mdfString,mSet);
    }  /* END for (aindex) */

  if ((CreateCreds == TRUE) && (ACL.OPtr == NULL))
    {
    switch (ACL.Type)
      {
      case maGroup: MOGetObject(mxoGroup,ACL.Name,&ACL.OPtr,mAdd); break;
      case maUser:  MOGetObject(mxoUser,ACL.Name,&ACL.OPtr,mAdd); break;
      case maAcct:  MOGetObject(mxoAcct,ACL.Name,&ACL.OPtr,mAdd); break; 
      case maQOS:   MOGetObject(mxoQOS,ACL.Name,&ACL.OPtr,mAdd); break; 
      case maClass: MOGetObject(mxoClass,ACL.Name,&ACL.OPtr,mAdd); break; 
      default: /* no need to create the credential */ break;
      }
    }  /* END if ((CreateCreds == TRUE) && (ACL.OPtr == NULL)) */

  MACLSetACL(A,&ACL,TRUE);

  if (ACL.Name != NULL)
    MUFree(&ACL.Name);

  return(SUCCESS);
  }  /* END MACLFromXML() */



/* access granted if:  */
/*   R1 CL matches R2 ACL (R2 grants access to R1) */
/*   R2 CL matches R1 ACL (R1 grants X) */
/*   R1 CL matches R2 RCL */
/*   R2 CL matches R1 RCL */

/**
 * check ACL access of a specified cred list against a specified access list
 *
 * @param ACL (I) access control list
 * @param CL (I) cred list
 * @param AC (I) [optional]
 * @param Affinity (O) [optional]
 * @param IsInclusive (O)
 */

int MACLCheckAccess(
 
  macl_t    *ACL,         /* I access control list */
  macl_t    *CL,          /* I cred list */
  maclcon_t *AC,          /* I (optional) */
  char      *Affinity,    /* O (optional) */
  mbool_t   *IsInclusive) /* O */
 
  {
  int oindex;
 
  mbool_t Inclusive[maLAST];
 
  mbool_t tmpIsInclusive = FALSE;

  mbool_t NonReqNeeded = FALSE;
  mbool_t NonReqFound  = FALSE;

  mbool_t GlobalACLMatch = FALSE;

  macl_t *tACL;
  macl_t *tCL;

  const char *FName = "MACLCheckAccess";

  MDB(8,fCONFIG) MLog("%s(ACL,CL,AC,Affinity,IsInclusive)\n",
    FName);

  if (ACL == NULL)
    {
    return(FAILURE);
    }

  if (IsInclusive == NULL)
    {
    return(FAILURE);
    }
 
  memset(Inclusive,FALSE,sizeof(Inclusive));

  /* NOTE:  require all 'required' creds + at least one non-required 
            cred or resource acl if specified */
 
  /* initialize ACL data */
 
  for (tACL = ACL;tACL != NULL;tACL = tACL->Next)
    {
    if (bmisset(&tACL->Flags,maclDeny))
      {
      /* deny ACL's are inclusive until proven otherwise */

      Inclusive[(int)tACL->Type] = TRUE;
      }
    else if (!bmisset(&tACL->Flags,maclDeny) &&
             !bmisset(&tACL->Flags,maclRequired) &&
             !bmisset(&tACL->Flags,maclXOR))
      {
      if ((tACL->Type != maRsv) && (tACL->Type != maJob))
        {
        /* only explicitly set acl's required */

        NonReqNeeded = TRUE;
        }
      }
    }  /* END for (tACL) */
 
  /* check ACLs */
 
  for (tACL = ACL;tACL != NULL;tACL = tACL->Next)
    {
    mbool_t ACLMatch = FALSE;

    if ((bmisset(&tACL->Flags,maclHPEnable)) &&
        (InHardPolicySchedulingCycle == FALSE))
      {
      if (!bmisset(&tACL->Flags,maclDeny))
        {
        continue;
        }
      }
 
    for (tCL = CL;tCL != NULL;tCL = tCL->Next)
      {
      mbool_t ValMatch = TRUE;
 
      if (tCL->Type != tACL->Type)
        continue;
 
      switch (tACL->Cmp)
        {
        case mcmpEQ:

          if (tCL->LVal != tACL->LVal)
            ValMatch = FALSE;
 
          break;
 
        case mcmpGT:

          if (tCL->LVal <= tACL->LVal)
            ValMatch = FALSE;

          break;

        case mcmpGE:
 
          if (tCL->LVal < tACL->LVal)
            ValMatch = FALSE;
 
          break;
 
        case mcmpLT:

          if (tCL->LVal >= tACL->LVal)
            ValMatch = FALSE;

        case mcmpLE:
 
          if (tCL->LVal > tACL->LVal)
            ValMatch = FALSE;
 
          break;
 
        case mcmpNE:
 
          if (tCL->LVal == tACL->LVal)
            ValMatch = FALSE;
 
          break;
 
        case mcmpSEQ:
        default:

          if (tACL->Name == NULL) /* no access control list */
            {
            ValMatch = FALSE;
            }
          else if (!strcmp(tACL->Name,"ALL"))
            {
            /* allow all when ACL == ALL */

            /* NO-OP */
            }
          else if ((tCL->Name[0] != tACL->Name[0]) &&
              (tCL->Name[0] != '['))
            {
            ValMatch = FALSE;
            }
          else if ((strcmp(tCL->Name,tACL->Name)) &&
                   (strcmp(tCL->Name,ALL)))
            {
            ValMatch = FALSE;
            }
 
          break; 
        }  /* END switch (tACL->Type) */
 
      if (ValMatch != TRUE)
        {
        if (bmisset(&tACL->Flags,maclRequired))
          {
          /* required credential request not matched */
 
          Inclusive[(int)tACL->Type] = FALSE;
          }
        else if (bmisset(&tACL->Flags,maclXOR))
          {
          Inclusive[(int)tACL->Type] = TRUE;
          }

        continue;
        }
 
      /* match found */
 
      if (bmisset(&tACL->Flags,maclDeny))
        {
        if (!bmisset(&tACL->Flags,maclHPEnable) ||
           (InHardPolicySchedulingCycle == TRUE))
          {
          /* deny match found, reservation is exclusive */
 
          *IsInclusive = FALSE;

          MDB(8,fCONFIG) MLog("INFO:     deny match found, reservation is exclusive\n");
 
          return(FAILURE);
          }
        }

      if (bmisset(&tACL->Flags,maclXOR))
        {
        Inclusive[(int)tACL->Type] = FALSE;

        /* Was an XOR, doesn't grant access, so continue */

        continue;
        }

      if (bmisset(&tACL->Flags,maclConditional) && 
         (tACL->OPtr != NULL))
        {
        if (tACL->Type == maQOS)
          {
          mqos_t *Q = (mqos_t *)tACL->OPtr;

          if ((Q->BLACLThreshold > 0) &&
             ((AC == NULL) || (AC->BL < Q->BLACLThreshold)))
            {
            /* conditional not satisfied */

            continue;
            }

          if ((Q->QTACLThreshold > 0) && 
             ((AC == NULL) || (AC->QT < Q->QTACLThreshold)))
            {
            /* conditional not satisfied */

            continue;
            }

          if ((Q->XFACLThreshold > 0.0) && 
             ((AC == NULL) || (AC->XF < Q->XFACLThreshold)))
            {
            /* conditional not satisfied */

            continue;
            }
          }    /* END if (tACL->Type == maQOS) */
        }      /* END if (bmisset(&tACL->Flags,maclConditional) && ...) */

      if (bmisset(&tACL->Flags,maclRequired))
        {
        /* NO-OP */
        }
      else
        {
        NonReqFound = TRUE;
        }
 
      if (Affinity != NULL)
        *Affinity = tACL->Affinity;
 
      ACLMatch = TRUE;
     
      MDB(8,fCONFIG) MLog("INFO:     ACL match located\n");
 
      tmpIsInclusive = TRUE;
      }  /* for (tCL) */

    if (bmisset(&tACL->Flags,maclRequired) && (ACLMatch != TRUE))
      {
      /* cannot locate 'required' ACL match */
 
      *IsInclusive = FALSE;

      MDB(8,fCONFIG) MLog("INFO:     cannot locate 'required' ACL match\n");
 
      return(FAILURE);
      }

    if (ACLMatch == TRUE)
      GlobalACLMatch = TRUE;
    }    /* END for (tACL) */

  if ((NonReqNeeded == TRUE) && (NonReqFound == FALSE))
    {
    /* cannot locate any non-'required' ACL match */

    *IsInclusive = FALSE;

    if (GlobalACLMatch == TRUE)
      {
      MDB(8,fCONFIG) MLog("INFO:     cannot locate any non-'required' ACL match\n");
      }
    else
      {
      MDB(8,fCONFIG) MLog("INFO:     cannot locate any ACL match\n");
      }

    return(FAILURE);
    }  /* END ((NonReqNeeded == TRUE) && (NonReqFound == FALSE)) */
 
  if (tmpIsInclusive == FALSE)
    {
    for (oindex = 1;oindex < maLAST;oindex++)
      {
      if (Inclusive[oindex] == TRUE)
        {
        tmpIsInclusive = TRUE;
 
        break;
        }
      }    /* END for (oindex) */
    }      /* END if (tmpIsInclusive == FALSE) */
 
  *IsInclusive = tmpIsInclusive;
 
  return(SUCCESS);
  }  /* END MACLCheckAccess() */



/**
 * Replace or create new access option in access list depending on Duplicate Type.
 *
 * @param ACL (I)
 * @param NewACL (I)
 * @param DuplicateType (I)
 */

int MACLSetACL(

  macl_t       **ACL,           /* I */
  macl_t        *NewACL,        /* I */
  mbool_t        DuplicateType) /* I */

  {
  macl_t *tACL;

  if (ACL == NULL)
    {
    return(FAILURE);
    }

  tACL = *ACL;

  if (DuplicateType == FALSE)
    {
    MACLRemove(ACL,NewACL->Type,NULL);

    /* Need to reset in case tACL (the first object) was removed */

    tACL = *ACL;
    }

  if (*ACL == NULL)
    {
    /* Empty list, allocate (do after MACLRemove - MACLRemove would remove the
         ACL we just created)) */

    if ((*ACL = (macl_t *)MUCalloc(1,sizeof(macl_t))) == NULL)
      return(FAILURE);

    tACL = *ACL;

    tACL->Type     = NewACL->Type;
    tACL->Cmp      = NewACL->Cmp;
    tACL->Affinity = NewACL->Affinity;
    bmcopy(&tACL->Flags,&NewACL->Flags);
    tACL->OPtr     = NewACL->OPtr;

    switch (NewACL->Type)
      {
      case maUser:
      case maGroup:
      case maClass:
      case maJob:
      case maQOS:
      case maAcct:
      case maRsv:
      case maVC:
      case maJAttr:
      case maCluster:
      case maJTemplate:
     
        MUStrDup(&tACL->Name,NewACL->Name);

        break;

      default:

        tACL->LVal = NewACL->LVal;

        break;
      }

    return(SUCCESS);
    }  /* END if (*ACL == NULL) */

  while (tACL != NULL)
    {
    if (tACL->Type == NewACL->Type)
      {
      /* must determine if we're updating an existing record or adding a new one */

      switch (tACL->Type)
        {
        case maUser:
        case maGroup:
        case maClass:
        case maJob:
        case maQOS:
        case maAcct:
        case maRsv:
        case maVC:
        case maJAttr:
        case maCluster:
        case maJTemplate:

          if (!strcmp(tACL->Name,NewACL->Name))
            {
            /* update and return */

            tACL->Cmp      = NewACL->Cmp;
            tACL->Affinity = NewACL->Affinity;
            bmcopy(&tACL->Flags,&NewACL->Flags);
            tACL->OPtr     = NewACL->OPtr;

            return (SUCCESS);
            }

          break;

        default:

          if (tACL->LVal == NewACL->LVal)
            {
            tACL->Cmp      = NewACL->Cmp;
            tACL->Affinity = NewACL->Affinity;
            bmcopy(&tACL->Flags,&NewACL->Flags);
            tACL->OPtr     = NewACL->OPtr;

            return(SUCCESS);
            }

          break;
        }  /* END switch (Type) */
      }    /* END if (tACL->Type == Type) */ 

    if (tACL->Next == NULL)
      {
      /* add the new element here */

      tACL->Next = (macl_t *)MUCalloc(1,sizeof(macl_t));

      tACL->Next->Type     = NewACL->Type;
      tACL->Next->Cmp      = NewACL->Cmp;
      tACL->Next->Affinity = NewACL->Affinity;
      bmcopy(&tACL->Next->Flags,&NewACL->Flags);
      tACL->Next->OPtr     = NewACL->OPtr;

      switch (tACL->Next->Type)
        {
        case maUser:
        case maGroup:
        case maClass:
        case maJob:
        case maQOS:
        case maAcct:
        case maRsv:
        case maVC:
        case maJAttr:
        case maCluster:
        case maJTemplate:

          MUStrDup(&tACL->Next->Name,NewACL->Name);
          
          return(SUCCESS);

          break;

        default:

          tACL->Next->LVal = NewACL->LVal;
     
          return(SUCCESS);

          break;
        }  /* END switch (Type) */
      }    /* END if (tACL->Type == Type) */ 

    tACL = tACL->Next;
    }      /* END while() */

  return(FAILURE);
  }  /* END MACLSetACL() */


/**
 * Replace or create new access option in access list depending on Duplicate Type.
 *
 * @param ACL (I)
 * @param Type (I)
 * @param Val (I) [optional]
 * @param Cmp (I) [optional]
 * @param Affinity (I) [optional]
 * @param Flags (I) [bitmap of enum macl*]
 * @param DuplicateType (I)
 */

int MACLSet(

  macl_t       **ACL,           /* I */
  enum MAttrEnum Type,          /* I */
  void          *Val,           /* I (optional) */
  int            Cmp,           /* I (optional) */
  int            Affinity,      /* I (optional) */
  mbitmap_t     *Flags,         /* I (bitmap of enum macl*) */
  mbool_t        DuplicateType) /* I */

  {
  macl_t *tACL;

  const char *FName = "MACLSet";

  char *tmpVal = NULL;

  switch (Type)
    {
    case maUser:
    case maGroup:
    case maClass:
    case maJob:
    case maQOS:
    case maAcct:
    case maRsv:
    case maVC:
    case maJAttr:
    case maCluster:
    case maJTemplate:

      tmpVal = (char *)Val;

      break;

    default:

      break;
    }

  MDB(5,fSTRUCT) MLog("%s(ACL,%s,%s,Cmp,Affinity,Flags,%s)\n",
    FName,
    MAttrO[Type],
    (Val != NULL) ? ((tmpVal != NULL) ? tmpVal : "Long") : "NULL",
    MBool[DuplicateType]); 

  if (ACL == NULL)
    {
    return(FAILURE);
    }

  switch (Type)
    {
    case maUser:
    case maGroup:
    case maClass:
    case maJob:
    case maQOS:
    case maAcct:
    case maRsv:
    case maVC:
    case maJAttr:
    case maCluster:
    case maJTemplate:

      if ((Val == NULL) || (((char *)Val)[0] == '\0'))
        return(FAILURE);

      break;

    default:

      break;
    }

  tACL = *ACL;

  if ((DuplicateType == FALSE) && (tACL != NULL))
    {
    MACLRemove(ACL,Type,NULL);

    /* Need to reset in case tACL (the first object) was removed */

    tACL = *ACL;
    }

  if (*ACL == NULL)
    {
    /* Empty list, allocate (do after MACLRemove - MACLRemove would remove the
         ACL we just created)) */

    if ((*ACL = (macl_t *)MUCalloc(1,sizeof(macl_t))) == NULL)
      return(FAILURE);

    tACL = *ACL;

    tACL->Type     = Type;
    tACL->Cmp      = Cmp;
    tACL->Affinity = Affinity;
    bmcopy(&tACL->Flags,Flags);

    switch (Type)
      {
      case maUser:
      case maGroup:
      case maClass:
      case maJob:
      case maQOS:
      case maAcct:
      case maRsv:
      case maVC:
      case maJAttr:
      case maCluster:
      case maJTemplate:

        MUStrDup(&tACL->Name,(char *)Val);

        break;

      default:

        tACL->LVal = (long)(*(long *)Val);

        break;
      }

    return(SUCCESS);
    }  /* END if (*ACL == NULL) */

  while (tACL != NULL)
    {
    if (tACL->Type == Type)
      {
      /* must determine if we're updating an existing record or adding a new one */

      switch (tACL->Type)
        {
        case maUser:
        case maGroup:
        case maClass:
        case maJob:
        case maQOS:
        case maAcct:
        case maRsv:
        case maVC:
        case maJAttr:
        case maCluster:
        case maJTemplate:

          if ((Val != NULL) && (!strcmp(tACL->Name,(char *)Val)))
            {
            /* update and return */

            tACL->Cmp = Cmp;
            tACL->Affinity = Affinity;
            bmcopy(&tACL->Flags,Flags);

            return (SUCCESS);
            }

          break;

        default:

          if (tACL->LVal == (long)(*(long *)Val))
            {
            tACL->Cmp = Cmp;
            tACL->Affinity = Affinity;
            bmcopy(&tACL->Flags,Flags);

            return(SUCCESS);
            }

          break;
        }  /* END switch (Type) */
      }    /* END if (tACL->Type == Type) */ 

    if (tACL->Next == NULL)
      {
      /* add the new element here */

      tACL->Next = (macl_t *)MUCalloc(1,sizeof(macl_t));

      tACL->Next->Type     = Type;
      tACL->Next->Cmp      = Cmp;
      tACL->Next->Affinity = Affinity;
      bmcopy(&tACL->Next->Flags,Flags);

      switch (tACL->Next->Type)
        {
        case maUser:
        case maGroup:
        case maClass:
        case maJob:
        case maQOS:
        case maAcct:
        case maRsv:
        case maVC:
        case maJAttr:
        case maCluster:
        case maJTemplate:

          MUStrDup(&tACL->Next->Name,(char *)Val);
          
          return(SUCCESS);

          break;

        default:

          tACL->Next->LVal     = (long)(*(long *)Val);
     
          return(SUCCESS);

          break;
        }  /* END switch (Type) */
      }    /* END if (tACL->Type == Type) */ 

    tACL = tACL->Next;
    }      /* END while() */

  return(FAILURE);
  }  /* END MACLSet() */




/**
 * remove access/cred option 
 *
 * @param ACL (I) [modified]
 * @param Type (I)
 * @param Val (I) [optional]
 */

int MACLRemove(

  macl_t        **ACL,  /* I (modified) */
  enum MAttrEnum  Type, /* I */
  void           *Val)  /* I (optional) */

  {
  mbool_t MatchLocated = FALSE;

  macl_t *Prev = NULL;
  macl_t *Next = NULL;
  macl_t *tACL = NULL;

  if (ACL == NULL)
    {
    return(FAILURE);
    }

  for (tACL = *ACL;tACL != NULL;tACL = Next)
    {
    MatchLocated = FALSE;

    if (tACL->Type != Type)
      {
      Prev = tACL;

      Next = tACL->Next;

      continue;
      }

    switch (Type)
      {
      case maUser:
      case maGroup:
      case maClass:
      case maJob:
      case maQOS:
      case maAcct:
      case maRsv:
      case maVC:
      case maJAttr:
      case maCluster:
      case maJTemplate:

        if ((Val == NULL) || (!strcmp(tACL->Name,(char *)Val)))
          MatchLocated = TRUE;

        break;

      default:

        if ((Val == NULL) || (tACL->LVal == (long)(*(long *)Val)))
          MatchLocated = TRUE;

        break;
      }  /* END switch (Cmp) */

    if (MatchLocated == TRUE)
      {
      /* remove this element */

      if (Prev != NULL)
        {
        Prev->Next = tACL->Next;
        }
      else
        {
        *ACL = tACL->Next;
        }

      Next = tACL->Next;

      MACLFree(&tACL);
      }
    else
      {
      Prev = tACL;

      Next = tACL->Next;
      }
    }  /* END for (aindex) */

  return(SUCCESS);
  }  /* END MACLRemove() */



/**
 * Merge ACL2 into ACL1.
 *
 * @param ACL1
 * @param ACL2
 */

int MACLMerge(

  macl_t **ACL1,
  macl_t  *ACL2)

  {
  macl_t *tACL;

  for (tACL = ACL2;tACL != NULL;tACL = tACL->Next)
    {
    MACLSetACL(ACL1,tACL,TRUE);
    }  /* END for (tACL) */

  return(SUCCESS);
  }  /* END MACLMerge() */


/**
 * Remove every entry in ACL2 from ACL1;
 *
 * @param ACL1
 * @param ACL2
 */

int MACLSubtract(

  macl_t **ACL1,
  macl_t  *ACL2)

  {
  macl_t *tACL;

  for (tACL = ACL2;tACL != NULL;tACL = tACL->Next)
    {
    switch (tACL->Type)
      {
      case maUser:
      case maGroup:
      case maClass:
      case maJob:
      case maQOS:
      case maAcct:
      case maRsv:
      case maVC:
      case maJAttr:
      case maCluster:
      case maJTemplate:

        MACLRemove(ACL1,tACL->Type,tACL->Name);

        break;

      default:

        MACLRemove(ACL1,tACL->Type,&tACL->LVal);

        break;
      }  /* END switch (tACL->Type) */
    }  /* END for (tACL) */

  return(SUCCESS);
  }  /* END MACLSubtract() */


/**
 * Return the number of ACLS in the list.
 *
 * @param ACL
 */

int MACLCount(

  macl_t *ACL)

  {
  int Count = 0;

  macl_t *tACL = ACL;

  while (tACL != NULL)
    {
    Count++;

    tACL = tACL->Next;
    }

  return(Count);
  }  /* END MACLCount() */

/**
 * Copies an individual macl_t entry.
 *
 * @param Dst
 * @param Src
 */

int MACLCopyEntry(

  macl_t *Dst,
  macl_t *Src)

  {
  if ((Dst == NULL) || (Src == NULL))
    {
    return(FAILURE);
    }

  if (Src->Name != NULL)
    MUStrDup(&Dst->Name,Src->Name);

  Dst->Type     = Src->Type;
  Dst->Cmp      = Src->Cmp;
  Dst->Affinity = Src->Affinity;
  Dst->LVal     = Src->LVal;
  Dst->DVal     = Src->DVal;
  Dst->Flags    = Src->Flags;
  Dst->OPtr     = Src->OPtr;

  Dst->Next     = NULL;

  return(SUCCESS);
  }  /* END MACLCopy() */


/**
 * Copy MACL to ACL but preserve the first entry of ACL.
 *
 * @param ACL (I/O) [modified]
 * @param MACL (I)
 */

int MACLMergeMaster(

  macl_t **ACL,        /* I/O (modified) */
  macl_t  *MACL)       /* I */

  {
  macl_t tACL;

  if ((ACL == NULL) || (MACL == NULL))
    {
    return(FAILURE);
    }

  memset(&tACL,0,sizeof(tACL));

  MACLCopyEntry(&tACL,MACL);

  MACLCopy(ACL,MACL);

  /* set to TRUE so that acl entries after the first entry won't be deleted. see moab-2841 */

  MACLSetACL(ACL,&tACL,TRUE);

  MUFree(&tACL.Name);

  return(SUCCESS);
  }  /* END MACLMerge() */




/**
 * Return a matching ACL entry.
 *
 * @param ACL (I)
 * @param OType
 * @param Out   (O) [required]
 */

int MACLGet(

  macl_t          *ACL,
  enum MAttrEnum   OType,
  macl_t         **Out)

  {
  macl_t *tACL;

  if (Out != NULL)
    *Out = NULL;

  if ((ACL == NULL) || (Out == NULL))
    {
    return(FAILURE);
    }

  for (tACL = ACL;tACL != NULL;tACL = tACL->Next)
    {
    if (tACL->Type == OType)
      {
      *Out = tACL;

      return(SUCCESS);
      }
   }

  /* matching ACL not located */

  return(FAILURE);
  }  /* END MACLGet() */


/* returns TRUE on mVerify if given PR is in ACL (the RM has access to the sandbox) */
/**
 *
 *
 * @param AList (I)
 * @param SandboxIsGlobal (O) [optional]
 * @param PR (I) [optional]
 * @param Mode (I) [mSet or mVerify]
 */

int MACLCheckGridSandbox(

  macl_t  *AList,                /* I */
  mbool_t *SandboxIsGlobal,      /* O (optional) */
  mrm_t   *PR,                   /* I (optional) */
  enum MObjectSetModeEnum Mode)  /* I (mSet or mVerify) */

  {
  mrm_t *R;

  int    rmindex;

  macl_t *A;

  mbool_t HasGlobalSandbox;

  if ((Mode != mSet) && (PR == NULL))
    {
    return(FAILURE);
    }

  /* determine which RMs are impacted */

  HasGlobalSandbox = TRUE;

  /* check all ACL's for a cluster match */

  for (A = AList;A != NULL;A = A->Next)
    {
    if (A->Type != maCluster)
      {
      continue;
      }

    if (Mode == mSet)
      {
      if (!strcasecmp(A->Name,"ALL"))
        {
        int rmindex;

        for (rmindex = 0;rmindex < MSched.M[mxoRM];rmindex++)
          {
          if (MRM[rmindex].Name[0] == '\0')
            break;

          if (MRMIsReal(&MRM[rmindex]) == FALSE)
            continue;

          if (!bmisset(&MRM[rmindex].Flags,mrmfClient))
            continue;

          MRM[rmindex].GridSandboxExists = TRUE;
          }
        }
      else if ((MRMFind(A->Name,&R) == SUCCESS) ||
               (MRMFindByPeer(A->Name,&R,FALSE) == SUCCESS))
        {
        R->GridSandboxExists = TRUE;
        }
      else
        {
        HasGlobalSandbox = FALSE;
        }
      }
    else
      {
      char *ptr;
      char  tmpName[MMAX_LINE];

      /* mVerify logic */

      if (!strcasecmp(A->Name,"ALL"))
        {
        return(SUCCESS);
        }

      MUStrCpy(tmpName,PR->Name,sizeof(tmpName));

      ptr = strstr(tmpName,".INBOUND");

      if (ptr != NULL)
        {
        *ptr = '\0';  /* cut tmpName short, removing the .INBOUND */
        } 

      if (!strcmp(tmpName,A->Name))
        {
        return(SUCCESS);
        }
      }
   }  /* END for (A) */

  if (Mode != mSet)
    {
    /* if PR was in cluster ACL, we should have returned SUCCESS by now--so FAIL */
    /* OR no cluster ACL was found, therefore NO RMs are granted access--so FAIL */

    return(FAILURE);
    }

  /* sandbox applies to all peers */

  if (HasGlobalSandbox != TRUE)
    {
    return(SUCCESS);
    }

  if (SandboxIsGlobal != NULL)
    *SandboxIsGlobal = TRUE;

  /* set global sandbox on all rms */

  for (rmindex = 0;rmindex < MSched.M[mxoRM];rmindex++)
    {
    if (MRM[rmindex].Name[0] == '\0')
      break;

    if (MRMIsReal(&MRM[rmindex]) == FALSE)
      continue;

    MRM[rmindex].GridSandboxExists = TRUE;
    }  /* END for (rmindex) */

  return(SUCCESS);
  }  /* END MACLCheckGridSandbox() */




/**
 *
 *
 * @param CL (I)
 * @param AType (I)
 * @param OP (O)
 */

int MACLGetCred(

  macl_t          *CL,    /* I */
  enum MAttrEnum   AType, /* I */
  void           **OP)    /* O */

  {
  macl_t *tACL;

  if (OP != NULL)
    *OP = NULL;

  if ((CL == NULL) || (OP == NULL))
    {
    return(FAILURE);
    }

  for (tACL = CL;tACL != NULL;tACL = tACL->Next)
    {
    if (tACL->Type != AType)
      continue;

    *OP = tACL->OPtr;

    return(SUCCESS); 
    }  /* END for (aindex) */

  return(FAILURE);
  }  /* END MACLGetCred() */




/**
 * Remove the cred locks from the given ACL
 *
 * @param ACL       (I) the ACL from which to remove cred locks
 * @param RsvName   (I) the name of the reservation whose ACL we're clearing
 */

int MACLRemoveCredLock(

  macl_t *ACL,      /* I */
  char   *RsvName)  /* I */

  {
  char CredName[MMAX_LINE];

  mgcred_t *TmpCred   = NULL;
  mqos_t   *TmpQOS    = NULL;
  mclass_t  *TmpClass  = NULL;

  if (ACL == NULL)
    {
    return(FAILURE);
    }

  /* If there's a credential attached to the ACL, clear it first */

  if (ACL->OPtr != NULL)
    {
    switch(ACL->Type)
      {
      case maClass:

        TmpClass = (mclass_t *)ACL->OPtr;

        break;

      case maQOS:

        TmpQOS = (mqos_t *)ACL->OPtr;

        break;

      case maJob:

        return(FAILURE);

      default:

        TmpCred = (mgcred_t *)ACL->OPtr;

        break;
      } /* END switch (ACL->Type) */
    }
  else
    {
    /* otherwise get the appropriate cred and check the
     * ReqRsv for the name of my parent reservation */

    switch (ACL->Type)
      {
      case maUser:

        if (ACL->Name == NULL)
          {
          break;
          }

        MUStrCpy(CredName,ACL->Name,MMAX_LINE);

        MUHTGet(&MUserHT,CredName,(void **)&TmpCred,NULL);

        break;

      case maGroup:

        if (ACL->Name == NULL)
          {
          break;
          }

        MUStrCpy(CredName,ACL->Name,MMAX_LINE);

        MUHTGet(&MGroupHT,CredName,(void **)&TmpCred,NULL);

        break;

      case maAcct:

        if (ACL->Name == NULL)
          {
          break;
          }

        MUStrCpy(CredName,ACL->Name,MMAX_LINE);

        MUHTGet(&MAcctHT,CredName,(void **)&TmpCred,NULL);

        break;

      case maQOS:

        if (ACL->Name == NULL)
          {
          break;
          }

        MUStrCpy(CredName,ACL->Name,MMAX_LINE);

        MQOSFind(CredName,&TmpQOS);

        break;

      case maClass:

        if (ACL->Name == NULL)
          {
          break;
          }

        MUStrCpy(CredName,ACL->Name,MMAX_LINE);

        MClassFind(CredName,&TmpClass);

        break;

      default:

        /* NO-OP */

        break;
      } /* END switch (ACL->Type) */
    }

  if (TmpCred != NULL)
    {
    MUFree(&TmpCred->F.ReqRsv);
    }
  else if (TmpClass != NULL)
    {
    MUFree(&TmpClass->F.ReqRsv);
    }
  else if (TmpQOS != NULL)
    {
    MUFree(&TmpQOS->F.ReqRsv);
    }

  return(SUCCESS);
  } /* END MACLRemoveCredLock() */



/**
 * MACLCopy copy acl lists from source to destination cred structs
 *
 * @param Dst (I)
 * @param Src (I)
 */

int MACLCopy(

  macl_t **Dst, /* I Destination */
  macl_t  *Src) /* I Source */

  {
  macl_t *ACL = Src;

  MACLFreeList(Dst);

  for (ACL = Src;ACL != NULL;ACL = ACL->Next)
    {
    MACLSetACL(Dst,ACL,TRUE);
    }

  return(SUCCESS);
  }  /* END MACLCopy() */

/* END MACL.c */

