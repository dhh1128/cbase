/* HEADER */

/**
 * @file MUtil.c
 * 
 * Contains various functions that simplify common operations in Moab.
 *
 * String manipulation, linked list manipulation, searching, sorting, and 
 * sleeping are a few of these operations.
 */

/*                                           *
 * Contains:                                 *
 *                                           *
 *  int MUGetPair(String,AttrName,...)       *
 *                                           */

#include "moab.h"
#include "moab-proto.h"
#include "moab-const.h"
#include "moab-global.h"



/* END prototypes */

/**
 * Checks to see if a given array is filled entirely with a given byte/char.
 *
 * @param Data (I) The array to check.
 * @param Value (I) The byte/char that should be present in the entire array.
 * @param Size (I) The size of the array to check.
 * @return FAILURE if any character in the given array does not match expected 
 *         value.
 */

int MUMemCCmp(

  char *Data,  /* I */
  char  Value, /* I */
  int   Size)  /* I */

  {
  int index;

  if (Data == NULL)
    {
    return(FAILURE);
    }

  for (index = 0;index < Size;index++)
    {
    if (Data[index] != Value)
      {
      return(FAILURE);
      }
    }  /* END for (index) */

  return(SUCCESS);
  }  /* END MUMemCCmp() */

/**
 * Generate tasklist from comma-delimited string.
 *
 * @param Buffer (I) [modified as side-affect]
 * @param TL (O) [minsize=TLSize]
 * @param TLSize (I)
 * @param AddNode (I)
 */

int MTLFromString(

  char      *Buffer,   /* I (modified as side-affect) */
  int       *TL,       /* O (minsize=TLSize) */
  int        TLSize,   /* I */
  mbool_t    AddNode)  /* I */

  {
  int tindex;

  char *TokPtr;

  char *nodeid;

  int   rc;

  mnode_t *N;

  /* FORMAT:  <NODEID>[,<NODEID>]... */

  if (TL != NULL)
    TL[0] = -1;

  /* NOTE:  If we got a Buffer that only has a '-' in it, the job is a VM
   *        tracking job with no nodes associated with it. We need to not add
   *        a node for it. */

  if ((Buffer == NULL) || (TL == NULL) || (strcmp(Buffer,"-") == 0))
    {
    return(FAILURE);
    }

  tindex = 0;

  nodeid = MUStrTok(Buffer,", \t\n",&TokPtr);

  while (nodeid != NULL)
    {
    if (AddNode == TRUE)
      rc = MNodeAdd(nodeid,&N);
    else
      rc = MNodeFind(nodeid,&N);

    if (rc == SUCCESS)
      {
      TL[tindex] = N->Index;

      tindex++;

      if (tindex >= TLSize)
        break;
      }  /* END if (rc == SUCCESS) */

    nodeid = MUStrTok(NULL,", \t\n",&TokPtr);
    }  /* END while (ptr != NULL) */

  TL[MIN(tindex,TLSize - 1)] = -1;

  return(SUCCESS);
  }  /* END MTLFromString() */





/**
 *
 *
 * @param JPFList (I) [modified]
 * @param Type (I)
 * @param AVal (I)
 * @param Priority (I)
 */

int MJPrioFAdd(

  mjpriof_t                 **JPFList,  /* I (modified) */
  enum MJobPrioAttrTypeEnum   Type,     /* I */
  char                       *AVal,     /* I */
  long                        Priority) /* I */

  {
  mjpriof_t *JPFPtr;

  mbool_t    FExists = FALSE;

  int        AIndex;

  if ((JPFList == NULL) || (AVal == NULL))
    {
    return(FAILURE);
    }

  switch (Type)
    { 
    case mjpatState:

      AIndex = MUGetIndexCI(AVal,MJobState,FALSE,0);

      break;

    case mjpatGRes:

      AIndex = MUMAGetIndex(meGRes,AVal,mAdd);

      break;

    case mjpatGAttr:
    default:

      AIndex = MUMAGetIndex(meJFeature,AVal,mAdd);

      break;
    }  /* END switch (Type) */

  if (*JPFList == NULL)
    {
    *JPFList = (mjpriof_t *)MUCalloc(1,sizeof(mjpriof_t));

    JPFPtr = *JPFList;
    }
  else
    {  
    /* locate existing/new slot */

    for (JPFPtr = *JPFList;JPFPtr->Next != NULL;JPFPtr = JPFPtr->Next)
      {
      /* check for match */

      if ((JPFPtr->Type == Type) && (JPFPtr->AIndex == AIndex))
        {
        FExists = TRUE;

        break;
        }
      }    /* END for (JPFPtr) */

    if (FExists == FALSE)
      {
      /* add new job priority function node */

      JPFPtr->Next = (mjpriof_t *)MUCalloc(1,sizeof(mjpriof_t));

      JPFPtr = JPFPtr->Next;
      }
    }    /* END else (*JPFList == NULL) */

  /* populate JPFPtr */

  JPFPtr->Priority = Priority;
  JPFPtr->Type     = Type;

  JPFPtr->AIndex   = AIndex;

  return(SUCCESS);
  }  /* END MJPrioFAdd() */




/**
 *
 *
 * @param JPFList (I)
 * @param J (I)
 * @param Type (I)
 * @param Priority (O)
 */

int MJPrioFGetPrio(

  mjpriof_t                 *JPFList,  /* I */
  mjob_t                    *J,        /* I */
  enum MJobPrioAttrTypeEnum  Type,     /* I */
  long                      *Priority) /* O */

  {
  mjpriof_t *JPFPtr;

  if (Priority != NULL)
    *Priority = 0;

  if ((JPFList == NULL) || (J == NULL) || (Priority == NULL))
    {
    return(FAILURE);
    }

  for (JPFPtr = JPFList;JPFPtr != NULL;JPFPtr = JPFPtr->Next)
    {
    if (JPFPtr->Type != Type)
      continue;

    switch (JPFPtr->Type)
      {
      case mjpatState:

        if (J->State == (enum MJobStateEnum) JPFPtr->AIndex)
          *Priority += JPFPtr->Priority;

        /* NOTE:  only one matching priority attribute per function */

/*
        return(SUCCESS);
*/
        /*NOTREACHED*/

        break;

      case mjpatGRes:

        if (MSNLGetIndexCount(&J->Req[0]->DRes.GenericRes,JPFPtr->AIndex) > 0)
          *Priority += JPFPtr->Priority;

        break;

      case mjpatGAttr:
      default:

        if (bmisset(&J->AttrBM,JPFPtr->AIndex) == TRUE)
          *Priority += JPFPtr->Priority;

        break;
      }

    MDB(8,fSTRUCT) MLog("INFO:     (Type:%d) Priority:%ld JPFPtr->Priority:%ld\n",
      JPFPtr->Type,
      *Priority,
      JPFPtr->Priority);
    }  /* END for (JPFPtr) */

  return(SUCCESS);
  }  /* END MJPrioFGetPrio() */




/**
 *
 * NOTE:  MUBufGetMatchToken() does not modify Head buffer *
 * NOTE:  Head buffer should be doubly terminated with two '\0' chars *
 *
 * @param Head (I) [optional,only specified on initial call]
 * @param MString (I) string to match [optional]
 * @param SDelim (I) [optional]
 * @param TokPtr (I/O) [optional]
 * @param OBuf (O) [optional]
 * @param OBufSize (O) [optional]
 */

char const *MUBufGetMatchToken(

  char const  *Head,     /* I string start (optional,only specified on initial call) */
  char        *MString,  /* I (optional) string to match */
  char const  *SDelim,   /* I specified record delimiter (optional) */
  char const **TokPtr,   /* I/O (optional) */
  char        *OBuf,     /* O (optional) */
  int          OBufSize) /* O (optional) */

  {
  const char *ptr;
  const char *tail;

  static const char *DDelim = "\n";  /* default delimiter */

  const char *Delim;

  int   Len;

  /* locate tokens bounded by Delim which begin with MString (if specified) 
   * where MString is white space delimited */

  if (OBuf != NULL)
    OBuf[0] = '\0';

  if (Head != NULL)
    {
    ptr = Head;
    }
  else if (TokPtr != NULL)
    {
    ptr = *TokPtr;
    }
  else
    {
    return(FAILURE);
    }

  Delim = (SDelim != NULL) ? SDelim : DDelim;

  /* FORMAT:  <MATCH>{ \0 | <DELIM> | <WS> } <DATA>... */

  if (MString != NULL)
    {
    Len = strlen(MString);

    while ((ptr = strstr(ptr,MString)) != NULL)
      {
      if ((ptr > Head) && (*(ptr - 1) != Delim[0]))
        {
        /* not at record start */

        if (*ptr != '\0')
          ptr++;

        continue;
        }

      if ((ptr[Len] != '\0') && (ptr[Len] != Delim[0]) && !isspace(ptr[Len]))
        {
        /* match found is not correctly terminated */

        if (*ptr != '\0')
          ptr++;

        continue;
        }

      if (TokPtr != NULL)
        *TokPtr = ptr + Len;

      /* locate token tail */

      tail = strstr(ptr,Delim);

      if (tail == NULL)
        tail = ptr + strlen(ptr);

      if (OBuf != NULL)
        {
        MUStrCpy(OBuf,ptr,MIN(OBufSize,tail - ptr + 1));
        }

      return(ptr);
      }  /* END while (ptr != NULL) */

    return(NULL);
    }
  else
    {
    /* advance to first non-whitespace character */

    while (isspace(*ptr))
      ptr++;
    }

  /* FORMAT:  XXX[<DELIM>][XXX[<DELIM>]]... */

  while (ptr != NULL)
    {
    if (*ptr == '\0')
      {  
      /* string is empty */

      /* NO-OP ??? */ 
      }

    /* valid delimiters located */

    /* locate token tail */

    tail = strstr(ptr,Delim);

    if (tail == NULL)
      tail = ptr + strlen(ptr);

    if (OBuf != NULL)
      {
      MUStrCpy(OBuf,ptr,MIN(OBufSize,tail - ptr + 1));
      }

    if (TokPtr != NULL)
      *TokPtr = tail + strlen(Delim);

    return(ptr);
    }  /* END while (ptr != NULL) */

  return(Head);
  }  /* END MUBufGetMatchToken() */




 
/**
 *
 *
 * @param S1 (I)
 * @param S2 (I)
 * @param D (O)
 * @param BufSize (I)
 */

int MTLMerge(

  mtl_t *S1,       /* I */
  mtl_t *S2,       /* I */
  mtl_t *D,        /* O */
  int    BufSize)  /* I */ 

  {
  int tlindex;

  mulong ETime = 0;

  int    s1index;
  int    s2index;

#ifndef MOPT
  if ((S1 == NULL) || (S2 == NULL) || (D == NULL))
    {
    return(FAILURE);
    }
#endif /* MOPT */

  s1index = 0;
  s2index = 0;

  /* NOTE:  lists terminated w/Time=MMAX_TIME */
  
  for (tlindex = 0;tlindex < BufSize;tlindex++)
    {
    ETime = MIN(S1[s1index].Time,S2[s2index].Time);

    D[tlindex].Time = ETime;
    D[tlindex].X1 = S1[s1index].X1;
    D[tlindex].X2 = S2[s2index].X1;

    if (ETime == MMAX_TIME)
      break;

    if (S1[s1index].Time == ETime)
      s1index++;

    if (S2[s2index].Time == ETime)
      s2index++;
    }  /* END for (tlindex) */

  D[tlindex].Time = ETime;

  if (tlindex > 0)
    {
    D[tlindex].X1 = D[tlindex - 1].X1;
    D[tlindex].X2 = D[tlindex - 1].X2;
    }
  else
    {
    D[tlindex].X1 = 0;
    D[tlindex].X2 = 0;
    }

  return(SUCCESS);
  }  /* END MTLMerge() */




/**
 * Determine if substring, Val, is contained within string using delimiter, Delim.
 * 
 * @see MUStrAppend()
 *
 * @param ListString (I)
 * @param Val (I)
 * @param Delim (I)
 */

int MUCheckStringList(

  char *ListString,  /* I */
  char *Val,         /* I */
  char  Delim)       /* I */

  {
  char *ptr;

  int   len;

  if ((ListString == NULL) || (Val == NULL) || (Delim <= '\0'))
    {
    return(FAILURE);
    }

  ptr = strstr(ListString,Val);

  len = strlen(Val);

  while (ptr != NULL)
    {
    if ((ptr > ListString) && (*(ptr - 1) != Delim))
      {
      ptr = strstr(ptr + 1,Val);

      continue;
      }

    if ((*(ptr + len) != '\0') && (*(ptr + len) != Delim))
      {
      ptr = strstr(ptr + 1,Val);

      continue;
      }

    return(SUCCESS);
    }  /* END while (ptr != NULL) */

  return(FAILURE);
  }  /* END MUCheckStringList() */







/**
 * Checks for provided characters in the given string.
 *
 * @param SrcString (I)
 * @param CharList (I)
 * @return Returns SUCCESS if SrcString contains only characters from CharList 
 */

int MUCheckString(

  const char *SrcString,  /* I */
  char *CharList)   /* I */

  {
  int sindex;

  if ((SrcString == NULL) || (SrcString[0] == '\0'))
    {
    return(SUCCESS);
    }

  if ((CharList == NULL) || (CharList[0] == '\0'))
    {
    return(FAILURE);
    }

  for (sindex = 0;SrcString[sindex] != '\0';sindex++)
    {
    if (!strchr(CharList,SrcString[sindex]))
      {
      /* invalid character located */

      return(FAILURE);
      }
    }    /* END for (sindex) */

  return(SUCCESS);
  }  /* END MUCheckString() */


/**
 * Checks the credentials from an unmunge result
 *
 * This function makes sure that the Username, UID, Group Name, and GID of the 
 * user that created the munge credential exists on the server side (in moab) as well.
 *
 * @param UnMungeCreds (I) The results of the unmunge output 
 */

int MUCheckUnMungeCreds(

  char *UnMungeCreds)  /* I */

  {
  mgcred_t *tmpUser;
  char GrpName[MMAX_NAME];
  int  tmpGID;
  char *tmpPtr;
  char *tmpPtr2;

  if ((tmpPtr = MUStrStr(UnMungeCreds,"UID:",0,FALSE,FALSE)) == NULL)
    {
    MDB(5,fSOCK) MLog("ALERT:    UID label not found in unmunge results\n");

    return(FAILURE);
    }

  tmpPtr += strlen("UID:");

  while (isspace(*tmpPtr))
    tmpPtr++;

  /* Find space after user name */

  tmpPtr2 = MUStrChr(tmpPtr,' ');  /* Format = "UID:    name (uid)" */

  *tmpPtr2++ = '\0';  /* also increment to left paren of uid */

  if (MUserFind(tmpPtr,&tmpUser) == FAILURE) 
    {
    MDB(5,fSOCK) MLog("ALERT:    User %s of munged credential doesn't exist in MOAB\n",
      tmpPtr);

    return(FAILURE);
    }

  /* Get uid of user */

  tmpPtr = ++tmpPtr2; /* tmpPtr points to the begining of uid */ 

  /* Find right paren aft uid */

  tmpPtr2 = MUStrChr(tmpPtr,')');  /* Format = "UID:    name (uid)" */

  *tmpPtr2++ = '\0'; /* increment ptr past current position to get GID*/

  /* OID is UID in the mgcred_t */

  if (tmpUser->OID != strtol(tmpPtr,NULL,10))
    {
    MDB(5,fSOCK) MLog("ALERT:    UID %s for user %s of munged credential doesn't exist in MOAB\n",
      tmpPtr,
      tmpUser->Name);

    return(FAILURE);
    }

  /* Get group name and GID from unmunge result */

  if ((tmpPtr = MUStrStr(tmpPtr2,"GID:",0,FALSE,FALSE)) == NULL)
    {
    MDB(5,fSOCK) MLog("ALERT:    GID label not found in unmunge results\n");
    
    return(FAILURE);
    }

  tmpPtr += strlen("GID:");

  while (isspace(*tmpPtr))
    tmpPtr++;

  /* tmpPtr is now pointing at the group name */

  /* Find space after group name */

  tmpPtr2 = MUStrChr(tmpPtr,' ');  /* Format = "GID:    name (gid)" */

  *tmpPtr2++ = '\0';  /* also increment to left paren of gid */

  if (MUGNameFromUName(tmpUser->Name,GrpName) == FAILURE)
    {
    MDB(5,fSOCK) MLog("ALERT:    Group name %s for user %s not found in system\n",
      tmpPtr,
      tmpUser->Name);
    
    return(FAILURE);
    }

  if (strcmp(tmpPtr,GrpName))
    {
    MDB(5,fSOCK) MLog("ALERT:    Munge Cred Group name %s for user %s in moab does not match group name %s found in system\n",
      tmpPtr,
      tmpUser->Name,
      GrpName);
    
    return(FAILURE);
    }

  /* Get gid of group name */

  tmpPtr = ++tmpPtr2;  /* tmpPtr points to gid */

  /* Find right paren after gid */

  tmpPtr2 = MUStrChr(tmpPtr,')');  /* Format = "GID:    name (gid)" */

  *tmpPtr2 = '\0';

  if ((tmpGID = MUGIDFromUser(-1,tmpUser->Name,NULL)) < 0)
    {
    MDB(5,fSOCK) MLog("ALERT:    GID  %s for user %s was not found in system\n",
      tmpPtr,
      tmpUser->Name);
    
    return(FAILURE);
    }

  if (tmpGID != strtol(tmpPtr,NULL,10))
    {
    MDB(5,fSOCK) MLog("ALERT:    UnMunge Cred GID %s for user %s in moab does not match the gid %d found in system\n",
      tmpPtr,
      tmpUser->Name,
      tmpGID);
      
    return(FAILURE);
    }

  return(SUCCESS);
  } /* END MUCheckUnMungeCreds */





/**
 * Checks the UnMunge credentials for clock skew
 *
 * Looks at the returned unmunge creds for clock skew
 *
 * @param UnMungeCreds (I) Credentials from unmunge
 * @return Returns success if Munge clock skew was found, FAILURE otherwise
 */

int MUCheckUnMungeSkew(

  char *UnMungeCreds) /* I */

  {
  static int Timesync = 0;

  if (UnMungeCreds == NULL)
    {
    return(FAILURE);
    }

  /* If "Rewound" or "Expired" found, nodes could be out of sync */

  if (MUStrStr(UnMungeCreds,"Rewound",0,FALSE,FALSE)) 
    {
    Timesync++;
   
    /* Log first 10 times and every 5 after that that. */

    if(Timesync <= 10 || (Timesync % 5) == 0)
      {
      MDB(1,fSOCK) MLog("ALERT:    UNMUNGE: Rewound credential found, node times need to be synced\n");

      fprintf(stderr,"UNMUNGE ERROR: Rewound credential found, node times need to be synced\n");

      return(SUCCESS);
      }
    }
  else if (MUStrStr(UnMungeCreds,"Expired",0,FALSE,FALSE)) 
    {
    Timesync++;
   
    /* Log first 10 times and every 5 after that that. */

    if(Timesync <= 10 || (Timesync % 5) == 0)
      {
      MDB(1,fSOCK) MLog("ALERT:    UNMUNGE: Expired Credential found, node times may need to be synced\n");

      fprintf(stderr,"UNMUNGE ERROR: Expired credential found, node times may need to be synced\n");

      return(SUCCESS);
      }
    }

  return(FAILURE);
  }  /* MUCheckMungeSkew() */





/**
 * Checks the munge output for prefix and suffix errors.
 *
 * If "MUNGE:" isn't on the front of the output, this is a prefix error. <br>
 * If ":" isn't on the end of the output, this is a suffix error
 *
 * @param MungeOutput (I) Munge output string
 * @param EMsg (O) The error message to pass back [optional]
 */

int MUCheckMungeOutput(

  char* MungeOutput,  /* I */
  char* EMsg)         /* I (optional) */

  {
  char *tmpPtr;
  char *tmpPrefix;

  /* Check for prefix error */

  if ((tmpPrefix = MUStrStr(MungeOutput,"MUNGE:",0,FALSE,FALSE)) == NULL)
    {
    MDB(5,fSOCK) MLog("ALERT:    MUNGE: Invalid Prefix (MUNGE: not found in output)\n");

    if(EMsg != NULL)
      strcpy(EMsg,"Invalid Munge Output - Prefix Error");

    return(FAILURE);
    }

  if (tmpPrefix != MungeOutput) 
    {
    /* MUNGE: should come at the begging of the munge output */

    MDB(5,fSOCK) MLog("ALERT:    MUNGE: Invalid Prefix - MUNGE: not found at front of output\n");

    if(EMsg != NULL)
      strcpy(EMsg,"Invalid Munge Output - Prefix Error");

    return(FAILURE);
    }

  /* Check for suffix error */

  tmpPrefix += strlen("MUNGE:");  /* step past the first : to find : at end of output */

  if((tmpPtr = MUStrChr(tmpPrefix,'\n')) != NULL) /* null out lingering new line */
    {
    *tmpPtr = '\0';
    }

  if (MUStrChr(tmpPrefix,':') == NULL)
    {
    MDB(5,fSOCK) MLog("ALERT:    MUNGE: Invalid Suffix (: not found in output)\n");

    if(EMsg != NULL)
      strcpy(EMsg,"Invalid Munge Output - Suffix Error");

    return(FAILURE);
    }
  else if (tmpPrefix[strlen(tmpPrefix) - 1] != ':')
    {
    /* : should come at the end. */

    MDB(5,fSOCK) MLog("ALERT:    MUNGE: Invalid Suffix (: not found at end of output\n");

    if(EMsg != NULL)
      strcpy(EMsg,"Invalid Munge Output - Suffix Error");

    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MUCheckMungeOutput() */

/* END MUtilMunge.c */



/* overlap == 0 indicates no overlap */

/**
 *
 *
 * @param S1Start
 * @param S1End
 * @param S2Start
 * @param S2End
 */

long MUGetOverlap(

  mulong S1Start,
  mulong S1End,
  mulong S2Start,
  mulong S2End)

  {
  long Overlap;

  Overlap = (long)MIN(S1End,S2End) - MAX(S1Start,S2Start);

  Overlap = MAX(0,Overlap);
 
  return(Overlap);
  }  /* END MUGetOverlap() */





/**
 *
 *
 * @param Key (I)
 * @param ReqList (I) [optional,NULL terminated]
 * @param IgnList (I) [optional,NULL terminated]
 * @return Return FALSE if not in req list or in ign list 
 */

mbool_t MUCheckList(

  char  *Key,     /* I */
  char **ReqList, /* I (optional,NULL terminated) */
  char **IgnList) /* I (optional,NULL terminated) */

  {
  int lindex;

  if ((Key == NULL) || (Key[0] == '\0'))
    {
    return(FALSE);
    }

  if ((ReqList != NULL) && (ReqList[0] != NULL))
    {
    for (lindex = 0;ReqList[lindex] != NULL;lindex++)
      {
      if (!strcasecmp(ReqList[lindex],Key))
        {
        return(TRUE);
        }
      }

    return(FALSE);
    }

  if (IgnList != NULL)
    {
    for (lindex = 0;IgnList[lindex] != NULL;lindex++)
      {
      if (!strcasecmp(IgnList[lindex],Key))
        {
        return(FALSE);
        }
      }

    return(TRUE);
    }

  return(TRUE);
  }  /* END MUCheckList() */





/**
 *
 *
 * @param Key (I)
 * @param ReqList (I) [optional]
 * @param IgnList (I) [optional]
 */

mbool_t MUCheckListR(

  const char *Key,
  mlr_t      *ReqList,
  mlr_t      *IgnList)

  {
  int lindex;
  int kindex;

  if ((Key == NULL) || (Key[0] == '\0'))
    {
    return(FALSE);
    }

  if ((ReqList != NULL) && (ReqList->List != NULL))
    {
    if (ReqList->Prefix != NULL)
      {
      if (!strncasecmp(ReqList->Prefix,Key,ReqList->PrefixLen))
        {
        kindex = strtol(Key + ReqList->PrefixLen,NULL,10);

        if ((kindex >= ReqList->MinIndex) && (kindex <= ReqList->MaxIndex))
          {
          return(TRUE);
          }
        }
      }
    else if (ReqList->List != NULL)
      { 
      for (lindex = 0;ReqList->List[lindex] != NULL;lindex++)
        {
        if (!strcasecmp(ReqList->List[lindex],Key))
          {
          return(TRUE);
          }
        }
      }

    return(FALSE);
    }

  if ((IgnList != NULL) && (IgnList->List != NULL))
    {
    if (IgnList->Prefix != NULL)
      {
      if (!strncasecmp(IgnList->Prefix,Key,IgnList->PrefixLen))
        {
        kindex = strtol(Key + IgnList->PrefixLen,NULL,10);

        if ((kindex >= IgnList->MinIndex) && (kindex <= IgnList->MaxIndex))
          {
          return(FALSE);
          }
        }
      }
    else if (IgnList->List != NULL)
      {
      for (lindex = 0;IgnList->List[lindex] != NULL;lindex++)
        {
        if (!strcasecmp(IgnList->List[lindex],Key))
          {
          return(FALSE);
          }
        }
      }

    return(TRUE);
    }

  return(TRUE);
  }  /* END MUCheckList() */



/**
 *
 *
 * @param SrcS (I) [optional,maxsize=MMAX_LINE]
 * @param SrcI (I) [optional, -1 -> not set]
 * @param RC (O)
 * @param RCLType (I)
 */

int MUGetRC(

  char            *SrcS,    /* I (optional,maxsize=MMAX_LINE) */
  int              SrcI,    /* I (optional, -1 -> not set) */
  int             *RC,      /* O */
  enum MRCListEnum RCLType) /* I */

  {
  int tmpI = 999;
  int cindex;

  char *ptr = NULL;

  if (RC == NULL)
    {
    return(FAILURE);
    }

  if ((SrcS == NULL) && (SrcI < 0))
    {
    *RC = tmpI;

    return(FAILURE);
    }

  /* extract rc */

  if (SrcS != NULL)
    {
    /* FORMAT:  [ERROR] <VAL>: ... */

    ptr = SrcS;

    /* skip leading whitespace */

    for (cindex = 0;cindex < MMAX_LINE;cindex++)
      {
      if (!isspace(ptr[cindex]))
        break;
      }  /* END for (cindex) */

    ptr += cindex;

    if (!strncmp(ptr,"ERROR",strlen("ERROR")))
      {
      /* skip optional 'ERROR' header */

      ptr += strlen("ERROR");

      /* skip trailing whitespace */

      for (cindex = 0;cindex < MMAX_LINE;cindex++)
        {
        if (!isspace(ptr[cindex]))
          break;
        }  /* END for (cindex) */

      ptr += cindex;
      }

    /* allow hex/octal/decimal */

    tmpI = (int)strtol(ptr,NULL,0);

    if ((tmpI == 0) && (ptr[0] != '\0'))
      tmpI = 999;
    }
  else
    {
    tmpI = SrcI;
    }

  if (tmpI < 0)
    {
    /* what is the logic here? */

    tmpI = 999;
    }

  switch (RCLType)
    {
    case mrclCentury:

      *RC = (tmpI / 100) * 100;

      break;

    case mrclDecade:

      *RC = (tmpI / 10) * 10;

      break;

    case mrclYear:
    default:

      *RC = tmpI;

      break;
    }  /* END switch (RCLType) */

  return(SUCCESS);
  }  /* END MUGetRC() */




/**
 * Checks if the given string is a range
 *
 * Checks for [ -,x ] in the following order.
 * name[01] is not a range, but is the whole name.
 *
 * <p> Handles bgl range string as well (name[ABCxXYZ]).
 *
 * @param RString (I) The string to check for a range
 */

mbool_t MUStringIsRange(

  char  *RString) /*I*/

  {
  char *ptr;

  if ((ptr = strchr(RString,'[')) != NULL)
    {
    if (((strchr(ptr,'-')) == NULL) &&
        ((strchr(ptr,',')) == NULL) &&
        ((strchr(ptr,'x')) == NULL)) /* bluegene */
      {
      return(FALSE);
      }

    if ((strchr(ptr,']')) != NULL)
      {
      return(TRUE);
      }
    }

  return(FALSE);
  }  /* END MUStringIsRange() */

/**
 * Prepares a string for SQL insertion.
 *
 * @param String   (I) original string
 * @param SafeMode (I) sanitizes string to avoid SQL injection attacks
 */

int MUStringToSQLString(

  mstring_t *String,
  mbool_t    SafeMode)

  {
  int Length;

  int sindex;  /* string index */
  int iindex;  /* insert index */

  /* NOT THREAD SAFE!! */
  static int   Size = 0;
  static char *ptr = NULL;

  int StringSize = strlen(String->c_str()) << 1;

  if (String == NULL)
    {
    return(FAILURE);
    }

  if (Size == 0)
    {
    Size = StringSize;

    ptr = (char *)MUMalloc(StringSize);
    }
  else if (Size < StringSize)
    {
    ptr = (char *)realloc(ptr,StringSize);

    Size = StringSize;
    }

  Length = strlen(String->c_str());

  ptr[0] = '\0';

  iindex = 0;

  for (sindex = 0;sindex < Length;sindex++)
    {
    if ((*String)[sindex] == '\\')  /* escape backslashes */
      {
      ptr[iindex++] = '\\';
      ptr[iindex++] = '\\';
      }
    else if ((*String)[sindex] == '\'')  /* escape single ticks */
      {
      ptr[iindex++] = '\\';
      ptr[iindex++] = '\'';
      }
    else
      {
      ptr[iindex++] = (*String)[sindex];
      }
    }

  ptr[iindex] = '\0';

  MStringSet(String,ptr);

  return(SUCCESS);
  }  /* END MUStringToSQLString() */






/**
 * Apply a generic resources to a generic resources array.
 *
 * @param     *GResName (I)
 * @param     *CGRes (O)
 * @param     *DGRes (O)
 * @param     *AGRes (O)
 * @param      CGResVal (I)
 * @param      DGResVal (I)
 * @param      AGResVal (I)
 */

int MUInsertGenericResource(

  char    *GResName,
  msnl_t  *CGRes,
  msnl_t  *DGRes,
  msnl_t  *AGRes,
  int      CGResVal,
  int      DGResVal,
  int      AGResVal)

  {
  int GResIndex;

  GResIndex = MUMAGetIndex(meGRes,GResName,mAdd);

  if (GResIndex == 0)
    {
    return(FAILURE);
    }

  MSNLSetCount(CGRes,GResIndex,CGResVal);
  MSNLSetCount(DGRes,GResIndex,DGResVal);
  MSNLSetCount(AGRes,GResIndex,AGResVal);

  return(SUCCESS);
  } /* END MUInsertGenericResource() */




/**
 * Find a name in an array of names and return its index or -1  if not found.
 * The search ends when MaxSize has been exceeded or an empty string is encountered.
 * If the name "ALL" is encountered, then return the index of "ALL"
 */

int MUFindName(

  name_t *Names,    /* I */
  char   *ToFind,   /* I */
  int     MaxSize)  /* I */

  {
  int index;

  for (index = 0;index < MaxSize;index++)
    {
    if (Names[index][0] == '\0')
      break;

    if (!strcmp(Names[index],ToFind))
      {
      return(SUCCESS);

      break;
      }

    if (!strcmp(Names[index],"ALL"))
      {
      return(SUCCESS);
      }
    }    /* END for (index) */

  return(FAILURE);
  }  /* END MUFindName() */




/**
 * Replace the shell/torque variable $PBS_JOBID with the actual job name.
 *
 * TODO: may want to make this more general (replace a shell var with a given
 * string)
 */

int MUExpandPBSJobIDToJobName(

  char      *Buf,                /* I/O (modified) string we're searching in */
  int        BufSize,            /* I/O (modified) */
  mjob_t    *JP)                 /* I job whose name we want */

  {
  char JobName[MMAX_NAME];      /* for the real job name */

  char *ptr;                    /* pointer to $PBS_JOBID */

  int StrIndex;                 /* index of $PBS_JOBID in Buf */

  const char *Var = "$PBS_JOBID";

  if ((JP == NULL) || (Buf == NULL) || (Buf[0] == '\0') || (BufSize == 0))
    {
    return(FAILURE);
    }

  ptr = strstr(Buf, Var);

  if (ptr == NULL)
    {
    return(FAILURE);
    }

  /* find the full name of the job */

  if (MJobGetName(JP,NULL,JP->SubmitRM,JobName,MMAX_NAME,mjnFullName) == NULL)
    {
    return(FAILURE);
    }

  /* find the index of $PBS_JOBID in Buf... */

  StrIndex = ptr - Buf;

  if (StrIndex < 0)
    {
    return(FAILURE);
    }

  /* make the replacement */
  if (MUBufReplaceString(
        &Buf,
        &BufSize,
        StrIndex,
        strlen(Var),
        JobName,
        FALSE) == FAILURE)
    {
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MUExpandPBSJobIDToJobName() */




/**
 * Check to see if this moab is running on the grid head
 *
 */
  
mbool_t MUIsGridHead()

  {
  mrm_t *R;
  int    rindex;
  
  for (rindex = 0;rindex < MSched.M[mxoRM];rindex++)
    {
    R = &MRM[rindex];

    if (R->Name[0] == '\0')   
      break;
  
    if (MRMIsReal(R) == FALSE)
      continue;

    /* don't count moab peers as slaves*/
  
    if (bmisset(&R->Flags,mrmfClient))
      continue;
  
    /* if this moab has a slave then it is a grid head */
  
    if (R->Type == mrmtMoab)
      {      
      return(TRUE);
      }
    }

  return(FALSE);
  } /* END MUIsGridHead */
 
 
 
/**
 *
 * @param String     (I) modified(tokenized) on success
 * @param OutName    (O) on success, points to a tokenized string inside of String
 * @param OutSubName (O) on success, points to a tokenized string inside of String
 *
 * @return SUCCESS if String is of the form $NAME[$SUBNAME], FAILURE otherwise
 */

int MUParseNewStyleName(

  char  *String,
  char **OutName,
  char **OutSubName)

  {
  char *OpenBracket;
  char *CloseBracket;
  if (((OpenBracket = strchr(String,'[')) != NULL) &&
      ((CloseBracket = strchr(OpenBracket,']')) != NULL))
    {
    OpenBracket[0] = 0;
    CloseBracket[0] = 0;

    *OutName = String;
    *OutSubName = OpenBracket + 1;

    return(SUCCESS);
    }
  else
    {
    return(FAILURE);
    }
  }  /* END MUParseNewStyleName() */



int MUParseResourceLimitPolicy(

  mpar_t *P,
  char  **SArray)

  {
  /* FORMAT:  <RESOURCE>[:[<SPOLICY>,]<HPOLICY>[:[<SACTION>,]<HACTION[:[<SVTIME>,]<HVTIME>]]] */
 
  char *ptr;
  char *TokPtr = NULL;

  char *hptr;
  char *HTokPtr = NULL;

  char *APtr;
  char *ATokPtr = NULL;

  mbool_t OnePolicy = FALSE;

  enum MResLimitPolicyEnum RLPolicy[2] = { mrlpNONE, mrlpNONE };

  mbitmap_t RLActionBM[2];  /* (bitmap of enum MResLimitVActionEnum) */

  enum MResLimitVActionEnum tRLAction;

  int  index;
  int  vindex;

  for (vindex = 0;SArray[vindex] != NULL;vindex++)
    {
    /* determine resource */

    if ((ptr = MUStrTok(SArray[vindex],":",&TokPtr)) == NULL)
      {
      MDB(1,fCONFIG) MLog("ALERT:    invalid %s parameter specified '%s'\n",
        MParam[mcoResourceLimitPolicy],
        SArray[vindex]);

      continue;
      }

    if ((index = MUGetIndexCI(ptr,MResLimitType,FALSE,mrNONE)) == mrNONE)
      {
      MDB(1,fCONFIG) MLog("ALERT:    invalid %s parameter specified '%s'\n",
        MParam[mcoResourceLimitPolicy],
        SArray[vindex]);

      continue;
      }

    /* set defaults */

    P->ResourceLimitPolicy[MRESUTLHARD][index] = mrlpAlways;

    bmset(
      &P->ResourceLimitViolationActionBM[MRESUTLHARD][index],
      MDEF_RESOURCELIMITVIOLATIONACTION);

    P->ResourceLimitPolicy[MRESUTLSOFT][index] = mrlpAlways;

    bmset(
      &P->ResourceLimitViolationActionBM[MRESUTLSOFT][index],
      MDEF_RESOURCELIMITVIOLATIONACTION);

    /* determine policy */

    /* FORMAT:  [<SPOLICY>,]<HPOLICY> */

    if ((ptr = MUStrTok(NULL,":",&TokPtr)) == NULL)
      {
      /* no policy specified */

      continue;
      }

    /* set hptr to second arg */

    hptr = MUStrTok(ptr,",",&HTokPtr);
    hptr = MUStrTok(NULL,",",&HTokPtr);

    if ((RLPolicy[MRESUTLHARD] = (enum MResLimitPolicyEnum)MUGetIndexCI(
           ptr,
           MResourceLimitPolicyType,
           FALSE,
           mrlpNONE)) == mrlpNONE)
      {
      /* invalid policy */

      MDB(1,fCONFIG) MLog("ALERT:    invalid policy %s specified with parameter %s\n",
        ptr,
        MParam[mcoResourceLimitPolicy]);

      P->ResourceLimitPolicy[MRESUTLHARD][index] = mrlpNONE;

      continue;
      }

    if (hptr != NULL)
      {
      RLPolicy[MRESUTLSOFT] = RLPolicy[MRESUTLHARD];
      
      RLPolicy[MRESUTLHARD] = (enum MResLimitPolicyEnum)MUGetIndexCI(
        hptr,
        MResourceLimitPolicyType,
        FALSE,
        mrlpNONE);
      }
    else
      {
      OnePolicy = TRUE;
      }

    if (RLPolicy[MRESUTLHARD] == mrlpNever)
      RLPolicy[MRESUTLHARD] = mrlpNONE;

    if (RLPolicy[MRESUTLSOFT] == mrlpNever)
      RLPolicy[MRESUTLSOFT] = mrlpNONE;

    P->ResourceLimitPolicy[MRESUTLHARD][index] = RLPolicy[MRESUTLHARD];
    P->ResourceLimitPolicy[MRESUTLSOFT][index] = RLPolicy[MRESUTLSOFT];

    /* determine action */

    /* FORMAT:  [<SACTION>,]<HACTION> */

    if ((ptr = MUStrTok(NULL,":",&TokPtr)) == NULL)
      {
      /* no action specified */

      MDB(1,fCONFIG) MLog("ALERT:    no action specified with parameter %s\n",
        MParam[mcoResourceLimitPolicy]);

      continue;
      }

    hptr = MUStrTok(ptr,",",&HTokPtr);
    hptr = MUStrTok(NULL,",",&HTokPtr);

    APtr = MUStrTok(ptr,"+",&ATokPtr);

    bmclear(&RLActionBM[MRESUTLHARD]);

    while (APtr != NULL)
      {
      if ((tRLAction = (enum MResLimitVActionEnum)MUGetIndexCI(
             APtr,
             MPolicyAction,
             FALSE,
             mrlaNONE)) == mrlaNONE)
        {
        /* invalid action */

        MDB(1,fCONFIG) MLog("ALERT:    invalid action %s specified with parameter %s\n",
          APtr,
          MParam[mcoResourceLimitPolicy]);
        }
      else
        {
        bmset(&RLActionBM[MRESUTLHARD],tRLAction);
        }

      APtr = MUStrTok(NULL,"+",&ATokPtr);
      }

    if (bmisclear(&P->ResourceLimitViolationActionBM[MRESUTLHARD][index]))
      {
      /* no valid action specified */

      MDB(1,fCONFIG) MLog("ALERT:    no valid action specified with parameter %s\n",
        MParam[mcoResourceLimitPolicy]);

      continue;
      }

    if (hptr != NULL)
      {
      /* hard limit specified */
      
      if (OnePolicy == TRUE)
        {
        /* one policy and two actions specified */

        MDB(3,fCONFIG) MLog("WARNING:  insufficient policies specified; hpolicy=%s,spolicy=%s",
          ((P->ResourceLimitPolicy[MRESUTLHARD][index] != 0)
              ? MResourceLimitPolicyType[P->ResourceLimitPolicy[MRESUTLHARD][index]] : "NONE"),
          ((P->ResourceLimitPolicy[MRESUTLSOFT][index] != 0)
              ? MResourceLimitPolicyType[P->ResourceLimitPolicy[MRESUTLHARD][index]] : "NONE"));

        }

      bmcopy(&RLActionBM[MRESUTLSOFT],&RLActionBM[MRESUTLHARD]);

      bmclear(&RLActionBM[MRESUTLHARD]);

      APtr = MUStrTok(hptr,"+",&ATokPtr);

      while (APtr != NULL)
        {
        if ((tRLAction = (enum MResLimitVActionEnum)MUGetIndexCI(
               APtr,
               MPolicyAction,
               FALSE,
               mrlaNONE)) == mrlaNONE)
          {
          /* invalid action */

          MDB(1,fCONFIG) MLog("ALERT:    invalid action %s specified with parameter %s\n",
            APtr,
            MParam[mcoResourceLimitPolicy]);
          }
        else
          {
          bmset(&RLActionBM[MRESUTLHARD],tRLAction);
          }

        APtr = MUStrTok(NULL,"+",&ATokPtr);
        }
      }    /* END if (hptr != NULL) */
    else
      {
      bmclear(&RLActionBM[MRESUTLSOFT]);
      }

    P->ResourceLimitViolationActionBM[MRESUTLHARD][index] = RLActionBM[MRESUTLHARD];
    P->ResourceLimitViolationActionBM[MRESUTLSOFT][index] = RLActionBM[MRESUTLSOFT];

    /* determine violation time */

    if ((ptr = MUStrTok(NULL," \t\n",&TokPtr)) == NULL)
      {
      /* no vtime specified */

      continue;
      }

    hptr = MUStrTok(ptr,",",&HTokPtr);
    hptr = MUStrTok(NULL,",",&HTokPtr);

    if (hptr != NULL)
      {
      P->ResourceLimitMaxViolationTime[MRESUTLSOFT][index] = MUTimeFromString(ptr);
      P->ResourceLimitMaxViolationTime[MRESUTLHARD][index] = MUTimeFromString(hptr);
      }
    else
      {
      P->ResourceLimitMaxViolationTime[MRESUTLSOFT][index] = 0;
      P->ResourceLimitMaxViolationTime[MRESUTLHARD][index] = MUTimeFromString(ptr);
      }
    }  /* END for (vindex) */

  return(SUCCESS);
  }  /* END MUParseResourceLimitPolicy() */






/**
 * Enforce DNS Servername standard
 */

mbool_t MUIsValidHostName(

  char *Name)

  {
  int Len;   /* length of full server name (may be FQDN) */
  int SLen;  /* length of non-fully qualified server name */

  int cindex;

  char *ptr;

  if ((Name == NULL) || (Name[0] == '\0'))
    {
    return(FALSE);
    }

  Len = strlen(Name);

  if ((ptr = strchr(Name,'.')) != NULL)
    SLen = ptr - Name;
  else 
    SLen = Len;

  /* constraints:

       name must start with alnum
       name must end with alnum
       name must contain only alnum + hyphen
  
       NOTE:  allow '.' for FQDN specification 
   */

  if (!isalnum(Name[0]) || !isalnum(Name[SLen - 1]))
    {
    return(FALSE);
    }
  
  for (cindex = 1;cindex < Len - 1;cindex++)
    {
    if (!isalnum(Name[cindex]) && 
       (Name[cindex] != '-') && 
       (Name[cindex] != '.'))
      {
      return(FALSE);
      }
    }    /* END for (cindex) */

  return(TRUE);
  }  /* END MUIsValidHostName() */





/**
 * returns TRUE if List is non-NULL and contains OSIndex. FALSE otherwise.
 *
 * if the OSPtr argument is supplied the function will set it to the OS
 * structure if a match is found
 *
 * @param List    (I) can be NULL
 * @param OSIndex (I) should be > 0
 * @param OSPtr   (O) optional, modified
 */

mbool_t MUOSListContains(

  mnodea_t  *List,
  int        OSIndex,
  mnodea_t **OSPtr)    /* O (optional) */

  {
  int aindex;

  if (List == NULL)
    {
    return(FALSE);
    }

  assert(OSIndex > 0);

  if (OSPtr != NULL)
    *OSPtr = NULL;

  for (aindex = 0;aindex < MMAX_NODEOSTYPE;aindex++)
    {
    if (List[aindex].AIndex <= 0)
      {
      /* end of list reached */

      break;
      }

    if (List[aindex].AIndex == OSIndex)
      {
      if (OSPtr != NULL)
        *OSPtr = &List[aindex];

      return(TRUE);
      }
    }  /* END for (aindex) */

  return(FALSE);
  }  /* END MUOSListContains() */

/* END MUtil.c */
