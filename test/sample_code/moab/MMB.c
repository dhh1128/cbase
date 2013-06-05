/* HEADER */

/**
 * @file MMB.c
 *
 *
 */

/* Contains:                                 *
 *   int MMBAdd(Head,Message,Owner,Type,ExpireTime,Urgency,MBPtr) *
 *   int MMBRemove(Index,Head)               *
 *   int MMBRemoveMessage(Head,Msg,MType)    *
 *   int MMBGetNext(MBPtr)                   *
 *   int MMBPurge(Head,Data,MinPriority,Now) *
 *   char *MMBPrintMessages(Head,DFormat,MinPrio,Buf,BufSize) *
 *   int MMBCopyAll(DstP,Src)                *
 *                                           */

#include "moab.h"
#include "moab-proto.h"
#include "moab-const.h"
#include "moab-global.h"

/* capabilities:

  provide object specific event log 
  messages time ordered or priority ordered
  self cleaning (expires old, or low priority messages)
  self updating (replaces out-of date data)
  trigger enabled?

*/

int __MMBFindMessage(mmb_t *,const char *,mmb_t **);





/* linked list of messages */




/**
 * Create deep copy of source message buffer array.
 *
 * @param DstHead (O) [modified/alloc]
 * @param SrcHead (I)
 */

int MMBCopyAll(

  mmb_t **DstHead,  /* O (modified/alloc) */
  mmb_t  *SrcHead)  /* I */

  {
  mmb_t *mptr;

  mmb_t *MB;

  const char *FName = "MMBCopyAll";

  MDB(7,fSTRUCT) MLog("%s(DstHead,SrcHead)\n",
    FName);

  if (DstHead == NULL)
    {
    return(FAILURE);
    }

  if (SrcHead == NULL)
    {
    return(SUCCESS);
    }

  for (mptr = SrcHead;mptr != NULL;mptr = mptr->Next)
    {
    if (MMBAdd(
          DstHead,
          mptr->Data,
          mptr->Owner,
          mptr->Type,
          mptr->ExpireTime,
          mptr->Priority,
          &MB) == FAILURE)
      {
      continue;
      }

    MB->Count     = mptr->Count;
    MB->IsPrivate = mptr->IsPrivate;
    MB->CTime     = mptr->CTime;

    if (mptr->Label != NULL)
      MUStrDup(&MB->Label,mptr->Label);

    if (mptr->Source != NULL)
      MUStrDup(&MB->Source,mptr->Source); 
    }  /* END for (mptr) */

  return(SUCCESS);
  }  /* END MMBCopyAll() */





/**
 * Remove all messages which match string or type.
 *
 * @see MMBRemove() - removes/frees entire message buffer at specified index.
 *
 * @param Head (I) [possibly modified]
 * @param Msg (I) [optional]
 * @param MType (I) [optional,mmbtNONE=notset,mmbtLAST=all(NYI)]
 */

int MMBRemoveMessage(

  mmb_t            **Head,
  const char        *Msg,
  enum MMBTypeEnum   MType)

  {
  int Index;

  const char *FName = "MMBRemoveMessage";

  MDB(7,fSTRUCT) MLog("%s(Head,%.32s...,%s)\n",
    FName,
    (Msg != NULL) ? Msg : "",
    (MType <= mmbtLAST) ? MMBType[MType] : "???");

  if ((Head == NULL) || ((Msg == NULL) && (MType == mmbtNONE)))
    {
    return(SUCCESS);
    }

  if (Msg != NULL)
    {
    /* locate messages which match string */

    if (MMBGetIndex(*Head,NULL,Msg,mmbtNONE,&Index,NULL) == SUCCESS)
      {
      /* remove specified message */

      MMBRemove(Index,Head);
      }
    }
  else
    {
    /* locate messages which match type */

    while (MMBGetIndex(*Head,NULL,NULL,MType,&Index,NULL) == SUCCESS)
      MMBRemove(Index,Head);
    }

  return(SUCCESS);
  }  /* END MMBRemoveMessage() */





/**
 * Add a new message to parent object.
 * 
 * NOTE:  replace double quotes with single quotes for ease of XML
 *        encapsulation
 *
 * @param Head (I) [modified]
 * @param MString (I)
 * @param Owner (I) [optional]
 * @param Type (I)
 * @param ExpireTime (I) [optional, set to <= 0 for default]
 * @param Priority (I)
 * @param MMBP (O) [optional]
 */

int MMBAdd(

  mmb_t  **Head,         /* I (modified) */
  char const *MString,      /* I */
  char const *Owner,        /* I (optional) */
  enum MMBTypeEnum Type, /* I */
  long     ExpireTime,   /* I (optional, set to <= 0 for default) */
  int      Priority,     /* I */
  mmb_t  **MMBP)         /* O (optional) */

  {
  mmb_t **Tail;
  mmb_t  *MBPtr;

  char  tmpLine[MMAX_LINE << 1];

  char *Msg;
  char *ptr;
  char *tail;
  char *Label = NULL;

  int   Index;

  const char *FName = "MMBAdd";

  MDB(7,fSTRUCT) MLog("%s(Head,%.32s...,%s,%s,%ld,%d,MMBP)\n",
    FName,
    (MString != NULL) ? MString : "",
    (Owner != NULL) ? Owner : "NULL",
    MMBType[Type],
    ExpireTime,
    Priority);

  if (Head == NULL)
    {
    return(FAILURE);
    }

  if ((MString == NULL) || (MString[0] == '\0'))
    {
    return(FAILURE);
    }

  /* FORMAT:  [<LABEL>:]<MESSAGE> */

  /* if label is specified and message is empty, destroy existing label messages */

  strncpy(tmpLine,MString,sizeof(tmpLine));

  Msg = tmpLine;

  if ((ptr = strchr(Msg,'\"')) != NULL)
    {
    MUStrReplaceChar(ptr,'\"','\'',FALSE);
    }

  if ((tail = strchr(Msg,':')) != NULL)
    {
    /* label must not contain whitespace and cannot be reserved strings error, alert, or info */

    for (ptr = Msg;ptr < tail;ptr++)
      {
      if (isspace(*ptr))
        break;
      }

    if (ptr == tail)
      {
      /* label located */

      Label = Msg;
      *tail = '\0';

      Msg = tail + 1;
      }
    }    /* END if ((cp = strchr(AVal[sindex],':')) != NULL) */

  if ((Label != NULL) && 
      (MMBGetIndex(*Head,Label,NULL,mmbtNONE,&Index,&MBPtr) == SUCCESS))
    {
    /* messages may be different */

    if (Msg[0] == '\0')
      {
      /* clear message */

      MMBRemove(Index,Head);

      if (MMBP != NULL)
        *MMBP = NULL;

      return(SUCCESS);
      }
    else if ((MBPtr->Data == NULL) || strcmp(MBPtr->Data,Msg))
      {
      /* update message - messages are different */

      MUStrDup(&MBPtr->Data,Msg);
      }
    }  
  else
    {
    /* locate existing message or find next available message slot */

    if (__MMBFindMessage(*Head,Msg,&MBPtr) != SUCCESS)
      { 
      MMBGetTail(Head,&Tail);

      *Tail = (mmb_t *)MUCalloc(1,sizeof(mmb_t));

      MBPtr = *Tail;

      if (Label != NULL)
        MUStrDup(&MBPtr->Label,Label);

      MUStrDup(&MBPtr->Data,Msg);
      }
    }

  MBPtr->Count++;

  if (ExpireTime <= 0)
    MBPtr->ExpireTime = MSched.Time + MCONST_DAYLEN;
  else
    MBPtr->ExpireTime = ExpireTime;

  MBPtr->Priority = Priority;

  if (Owner != NULL)
    MUStrDup(&MBPtr->Owner,Owner);

  MBPtr->Type = Type;

  MUGetTime((mulong *)&MBPtr->CTime,mtmNONE,&MSched);

  if (MMBP != NULL)
    *MMBP = MBPtr;

  return(SUCCESS);
  }  /* END MMBAdd() */





/**
 * Remove message located at Index.
 *
 * @see MMBRemoveMessage() - removes message with specified name/type.
 *
 * @param Index (I)
 * @param Head (I) [modified/removed]
 */

int MMBRemove(

  int     Index, /* I */
  mmb_t **Head)  /* I (modified/removed) */

  {
  mmb_t *ptr;
  mmb_t *prev;

  int    count;
  int    NumRemoved;

  if (Head == NULL)
    {
    return(FAILURE);
    }

  if (Index < 0)
    {
    return(FAILURE);
    }

  count = 0;
  NumRemoved = 0;
  prev  = NULL;

  for (ptr = *Head;ptr != NULL;ptr = ptr->Next)
    {
    if (count == Index)
      {
      /* uncomment below to remove one message instance at a time */

      /*
      if (ptr->Count > 1)
        {
        ptr->Count --;

        break;
        }
      */

      if (prev != NULL)
        {
        prev->Next = ptr->Next;

        MMBFree(&ptr,FALSE);

        NumRemoved++;

        ptr = prev;

        break;
        }
      else
        {
        *Head = ptr->Next;

        MMBFree(&ptr,FALSE);

        NumRemoved++;

        ptr = *Head;

        prev = ptr;

        break;
        }
      }    /* END if (count == Index) */

    prev = ptr;

    count++;
    }    /* END for (ptr) */

  if (count < Index)
    {
    return(FAILURE);
    }

  if (NumRemoved <= 0)
    {
    return(FAILURE);
    }

  return(SUCCESS);
  } /* END MMBRemove() */





/**
 * Free specified element or entire message buffer list.
 * 
 * @see MMBRemove()
 *
 * @param MBP (I) [freed]
 * @param FreeList (I) - free entire list, not just element
 */

int MMBFree(

  mmb_t   **MBP,      /* I (freed) */
  mbool_t   FreeList) /* I - free entire list, not just element */

  {
  mmb_t *MB;
  mmb_t *MBN;

  if ((MBP == NULL) || (*MBP == NULL))
    {
    return(SUCCESS);
    }

  for (MB = *MBP;MB != NULL;MB = MBN)
    {
    MBN = MB->Next;

    MUFree(&MB->Owner);
    MUFree(&MB->Source);
    MUFree(&MB->Label);

    MUFree(&MB->Data);

    MUFree((char **)&MB);

    if (FreeList == FALSE)
      break;
    }  /* END for (MB) */

  *MBP = NULL;

  return(SUCCESS);
  }  /* END MMBFree() */





/**
 * Purge expired/stale messages from message buffer.
 *
 * @param HP (I) [modified]
 * @param Data (I) [optional]
 * @param MinPriority (I) [optional] (-1 is not set)
 * @param Now (I) [optional]
 */

int MMBPurge(

  mmb_t  **HP,          /* I (modified) */
  char    *Data,        /* I (optional) */
  int      MinPriority, /* I (optional) */
  long     Now)         /* I (optional) */

  {
  mmb_t *ptr;
  mmb_t *prev;

  mmb_t *MNext;

  if ((HP == NULL) || (*HP == NULL))
    {
    return(SUCCESS);
    }

  prev = NULL;

  for (ptr = *HP;ptr != NULL;ptr = MNext)
    {
    MNext = ptr->Next;

    if (((Data == NULL) || strcmp(Data,ptr->Data)) &&
         (Now < ptr->ExpireTime) &&
        ((MinPriority < 0) || (ptr->Priority > MinPriority)))
      {
      prev = ptr;

      continue;
      }

    /* remove node */

    MDB(5,fUI) MLog("INFO:     purging message '%.100s'\n",
      (ptr->Data != NULL) ? ptr->Data : "NULL");

    if (prev != NULL)
      {
      /* remove internal node */

      prev->Next = ptr->Next;

      MMBFree(&ptr,FALSE);

      ptr = prev;
      }
    else
      {
      /* remove head node */

      *HP = ptr->Next;

      MMBFree(&ptr,FALSE);

      ptr = *HP;
      }
    }    /* END for (ptr) */

  return(SUCCESS);
  }  /* END MMBPurge() */




 
/**
 * find message in message buffer with matching data string
 *
 * @param Head (I)
 * @param Message (I)
 * @param Ptr (O)
 */

int __MMBFindMessage(

  mmb_t       *Head,
  const char  *Message,
  mmb_t      **Ptr)

  {
  mmb_t *ptr;

  for (ptr = Head;ptr != NULL;ptr = ptr->Next)
    {
    if (ptr->Data != NULL)
      {
      if (!strcmp(ptr->Data,Message))
        {
        *Ptr = ptr;

        return(SUCCESS);
        }
      }
    }    /* END for (ptr) */

  return(FAILURE);
  }  /* END __MMBFindMessage() */


  


/**
 * Find next message in message buffer with matching label.
 *
 * @param Head (I)
 * @param Label (I) [optional]
 * @param Data (I) [optional]
 * @param Type (I) [optional] Specify a type other than mmbtNONE to have the type checked.
 * @param IndexP (O) [optional]
 * @param MP (O) [optional]
 */

int MMBGetIndex(

  mmb_t      *Head,
  const char *Label,
  const char *Data,
  enum MMBTypeEnum Type,
  int        *IndexP,
  mmb_t     **MP)

  {
  mmb_t *ptr;

  int    count;

  if (IndexP != NULL)
    *IndexP = 0;
  
  if (MP != NULL)
    *MP = NULL;

  if ((Head == NULL) || 
      ((Label == NULL) && (Data == NULL) && (Type == mmbtNONE)))
    {
    return(FAILURE);
    }

  count = 0;

  for (ptr = Head;ptr != NULL;ptr = ptr->Next)
    {
    if ((Label != NULL) && (ptr->Label != NULL))
      {
      if (!strcasecmp(ptr->Label,Label))
        {
        if (IndexP != NULL)
          *IndexP = count;

        if (MP != NULL)
          *MP = ptr;

        return(SUCCESS);
        }
      }    /* END if ((Label != NULL) && (ptr->Label != NULL)) */
   
    if ((Data != NULL) && (ptr->Data != NULL))
      {
      if (!strcasecmp(ptr->Data,Data))
        {
        if (IndexP != NULL)
          *IndexP = count;

        if (MP != NULL)
          *MP = ptr;

        return(SUCCESS);
        }
      }    /* END if ((Data != NULL) && (ptr->Data != NULL)) */

    if ((Type != mmbtNONE) && (ptr->Type == Type))
      {
      if (IndexP != NULL)
        *IndexP = count;

      if (MP != NULL)
        *MP = ptr;

      return(SUCCESS);
      } /* END if ((Type != mmbtNONE) && (ptr->Type == Type)) */

    count++;
    }      /* END for (ptr) */

  return(FAILURE);
  }  /* END MMBGetIndex() */




/**
 * get last message pointer in message buffer (will point to NULL)
 *
 * For example, if you have one message (just Head), it will be the
 *  same as &(Head->Next), so it is for adding an element and already
 *  having the link from the current last element set up.
 *
 * @param Head (I)
 * @param Tail (O)
 */

int MMBGetTail(

  mmb_t  **Head,  /* I */
  mmb_t ***Tail)  /* O */

  {
  mmb_t *ptr;

  if (Head == NULL)
    {
    return(FAILURE);
    }

  if (Tail == NULL)
    {
    return(FAILURE);
    }

  if (*Head != NULL)
    {
    for (ptr = *Head;ptr->Next != NULL;ptr = ptr->Next);

    *Tail = &ptr->Next;
    }
  else
    {
    *Tail = Head;
    }

  return(SUCCESS);
  }  /* END MMBGetTail() */





/**
 * Report object messages in specified (Human/XML) format.
 *
 * @param Head (I) [optional]
 * @param DFormat (I)
 * @param ShowVerbose (I)
 * @param MinPrio (I) [only display messages of this priority or higher]
 * @param Buf (O) [optional,char * or mxml_t *]
 * @param BufSize (I)
 */

char *MMBPrintMessages(

  mmb_t               *Head,        /* I (optional) */
  enum MFormatModeEnum DFormat,     /* I */
  mbool_t              ShowVerbose, /* I */
  int                  MinPrio,     /* I (only display messages of this priority or higher) */
  char                *Buf,         /* O (optional,char * or mxml_t *) */
  int                  BufSize)     /* I */

  {
  mmb_t *mptr;

  char *BPtr = NULL;
  int   BSpace;

  char *hptr = NULL;

  static char tmpMBuf[MMAX_BUFFER];

  char  tmpLine[MMAX_LINE << 2];
  char  tmpLine2[MMAX_LINE << 2];

  if (Buf != NULL)
    {
    BPtr = Buf;
    BSpace = BufSize;
    }
  else
    {
    BPtr = tmpMBuf;
    BSpace = sizeof(tmpMBuf);
    }

  hptr = BPtr;

  switch (DFormat)
    {
    case mfmXML:
    case mfmAVP:  /* KLUDGE - use AVP to create packed XML data */

      {
      mxml_t *DE;
      mxml_t *CE;
      mxml_t *UE;

      int     count = 0;

      DE = (mxml_t *)Buf;

      if (DE == NULL)
        break;

      if (Head == NULL)
        {
        /* header not required in XML */

        break;
        }  /* END if (Head == NULL) */

      if (DFormat == mfmAVP)
        {
        /* initialize temporary parent XML element */

        UE = NULL;
        }
    
      for (mptr = Head;mptr != NULL;mptr = mptr->Next)
        {
        if (mptr->Priority < MinPrio)
          continue;

        CE = NULL;

        MXMLCreateE(&CE,"message");

        MXMLSetAttr(CE,"index",(void *)&count,mdfInt);
        MXMLSetAttr(CE,(char *)MMBAttr[mmbaCTime],(void *)&mptr->CTime,mdfLong);

        if (mptr->Owner != NULL)
          MXMLSetAttr(CE,(char *)MMBAttr[mmbaOwner],(void *)mptr->Owner,mdfString);

        MXMLSetAttr(CE,(char *)MMBAttr[mmbaPriority],(void *)&mptr->Priority,mdfInt);
        MXMLSetAttr(CE,(char *)MMBAttr[mmbaCount],(void *)&mptr->Count,mdfInt);
        MXMLSetAttr(CE,(char *)MMBAttr[mmbaExpireTime],(void *)&mptr->ExpireTime,mdfLong);
        MXMLSetAttr(CE,(char *)MMBAttr[mmbaType],(void *)MMBType[mptr->Type],mdfString);

        if (mptr->Data != NULL)
          {
          if (strchr(mptr->Data,'\n') != NULL)
            {
            char tmpData[MMAX_BUFFER];

            MUStrCpy(tmpData,mptr->Data,sizeof(tmpData));

            MUStrReplaceChar(tmpData,'\n',' ',FALSE);

            MXMLSetAttr(CE,(char *)MMBAttr[mmbaData],(void *)tmpData,mdfString);
            }
          else
            { 
            MXMLSetAttr(CE,(char *)MMBAttr[mmbaData],(void *)mptr->Data,mdfString);
            }
          }

        if (mptr->Label != NULL)
          MXMLSetAttr(CE,(char *)MMBAttr[mmbaLabel],(void *)mptr->Label,mdfString);

        if (DFormat == mfmAVP)
          {
          /* create/update temporary XML */

          if (UE == NULL)
            MXMLCreateE(&UE,(char*) MSON[msonData]);

          MXMLAddE(UE,CE);
          }
        else
          {
          MXMLAddE(DE,CE);
          }

        count++;
        }  /* END for (mptr) */

      if ((DFormat == mfmAVP) && (UE != NULL))
        {
        /* NOTE:  Buf is mxml_t *, do not populate */

        MUSNInit(&BPtr,&BSpace,tmpMBuf,sizeof(tmpMBuf));

        hptr = BPtr;

        MXMLToString(UE,tmpLine,sizeof(tmpLine),NULL,TRUE);

        MXMLDestroyE(&UE);

        MUStringPack(tmpLine,tmpLine2,sizeof(tmpLine2));

        MUSNCat(&BPtr,&BSpace,tmpLine2);
        }
      }    /* END BLOCK */

      break;

    case mfmHuman:
    default:

      {
      int count = 0;

      char tmpBuf[MMAX_NAME];
      char label[MMAX_NAME];

      if (Buf != NULL)
        MUSNInit(&BPtr,&BSpace,Buf,BufSize);
      else
        MUSNInit(&BPtr,&BSpace,tmpMBuf,sizeof(tmpMBuf));

      hptr = BPtr;

      if (Head == NULL)
        {
        if (ShowVerbose == TRUE)
          {
          MUSNPrintF(&BPtr,&BSpace,"%-7s %10s %10s %8.8s %4.4s %3.3s %s\n",
            "Label",
            "CreateTime",
            "ExpireTime",
            "Owner",
            "Prio",
            "Num",
            "Message");
          }

        break;
        }  /* END if (Head == NULL) */

      for (mptr = Head;mptr != NULL;mptr = mptr->Next)
        {
        if (mptr->Priority < MinPrio)
          continue;

        if (mptr->Label != NULL)
          {
          strncpy(label,mptr->Label,sizeof(label));
          }
        else
          {
          sprintf(label,"%d",
            count);
          }

        count++;

        if (ShowVerbose == TRUE)
          {
          char TString[MMAX_LINE];
          /* display w/relative time */

          MULToTString(mptr->CTime - MSched.Time,TString);

          strcpy(tmpBuf,TString);

          MULToTString(mptr->ExpireTime - MSched.Time,TString);

          MUSNPrintF(&BPtr,&BSpace,"%-8.8s %10s %10s %8.8s %4d %3d %s\n",
            label,
            tmpBuf,
            TString,
            (mptr->Owner != NULL) ? mptr->Owner : "N/A",
            mptr->Priority,
            mptr->Count,
            (mptr->Data != NULL) ? mptr->Data : "-");
          }
        else
          {
          if (mptr->Source != NULL)
            {
            MUSNPrintF(&BPtr,&BSpace,"Message[%.8s] (from %s) %s\n",
              label,
              mptr->Source,
              (mptr->Data != NULL) ? mptr->Data : "-");
            }
          else
            {
            MUSNPrintF(&BPtr,&BSpace,"Message[%.8s] %s\n",
              label,
              (mptr->Data != NULL) ? mptr->Data : "-");
            }
          }
        }  /* END for (mptr) */
      }    /* END BLOCK (case mfmHuman) */

      break;
    }  /* END switch (DFormat) */

  return(hptr);
  }  /* END MMBPrintMessages() */




/**
 * Translate string to object message buffer.
 *
 * @param Buf (I)
 * @param MBP (O) [alloc]
 */

int MMBFromString(

  char   *Buf,  /* I */
  mmb_t **MBP)  /* O (alloc) */

  {
  int CTok;

  mxml_t *E = NULL;
  mxml_t *ME;

  char tmpLine[MMAX_LINE << 1];
  char Owner[MMAX_NAME];
  char Label[MMAX_NAME];

  int count;
  int priority;

  mmb_t *tmpMB;

  enum MMBTypeEnum type;

  mulong etime;

  char *ptr;

  /* NOTE:  sync w/MMBFromXML() */
 
  if ((Buf == NULL) || (MBP == NULL))
    {
    return(FAILURE);
    }

  if (MXMLFromString(&E,Buf,NULL,NULL) == FAILURE)
    {
    return(FAILURE);
    }

  CTok = -1;

  while (MXMLGetChild(E,"message",&CTok,&ME) == SUCCESS)
    {
    if (MXMLGetAttr(ME,(char *)MMBAttr[mmbaCount],NULL,tmpLine,sizeof(tmpLine)) == SUCCESS)
      {
      count = (int)strtol(tmpLine,NULL,10);
      }
    else
      {
      count = 1;
      }

    if (MXMLGetAttr(ME,(char *)MMBAttr[mmbaPriority],NULL,tmpLine,sizeof(tmpLine)) == SUCCESS)
      {
      priority = (int)strtol(tmpLine,NULL,10);
      }
    else
      {
      priority = 1;
      }

    MXMLGetAttr(ME,(char *)MMBAttr[mmbaType],NULL,tmpLine,sizeof(tmpLine));

    type = (enum MMBTypeEnum)MUGetIndexCI(tmpLine,MMBType,FALSE,mmbtNONE);

    if (MXMLGetAttr(ME,(char *)MMBAttr[mmbaExpireTime],NULL,tmpLine,sizeof(tmpLine)) == SUCCESS)
      {
      etime = strtol(tmpLine,NULL,10);
      }
    else
      {
      etime = MSched.Time + MCONST_DAYLEN;
      }

    if (MXMLGetAttr(ME,(char *)MMBAttr[mmbaOwner],NULL,Owner,sizeof(Owner)) == FAILURE)
      strcpy(Owner,"N/A");

    MXMLGetAttr(ME,(char *)MMBAttr[mmbaLabel],NULL,Label,sizeof(Label));

    if (MXMLGetAttr(ME,(char *)MMBAttr[mmbaData],NULL,tmpLine,sizeof(tmpLine)) == FAILURE)
      {
      MXMLDestroyE(&E);

      return(FAILURE);
      }

    ptr = tmpLine;

    /* NOTE:  restore checkpoint newlines - sync w/MJobToXML() */

    while ((ptr = strchr(ptr,'\7')) != NULL)
      {
      *ptr = '\n';
      }

    MXMLDestroyE(&E);

    if (MMBAdd(
          MBP,
          tmpLine,  /* I */
          Owner,
          type,
          etime,
          priority,
          &tmpMB) == FAILURE)
      {
      return(FAILURE);
      }

    if (count > 1)
      MMBSetAttr(tmpMB,mmbaCount,(void *)&count,mdfInt);

    if (Label[0] != '\0')
      MUStrDup(&tmpMB->Label,Label);
    }    /* END while (MXMLGetChild() == SUCCESS) */

  return(SUCCESS);
  }  /* END MMBFromString() */




/**
 * Set specific message buffer attribute.
 *
 * @param MB (I) [modified]
 * @param AIndex (I)
 * @param Val (I)
 * @param DIndex (I)
 */

int MMBSetAttr(

  mmb_t                *MB,      /* I (modified) */
  enum MMBAttrEnum      AIndex,  /* I */
  void                 *Val,     /* I */
  enum MDataFormatEnum  DIndex)  /* I */

  {
  if (MB == NULL)
    {
    return(FAILURE);
    }

  switch (AIndex)
    {
    case mmbaCount:

      MB->Count = *(int *)Val;

      break;

    case mmbaLabel:

      /* NYI */

      break;

    case mmbaSource:

      MUStrDup(&MB->Source,(char *)Val);

      break;

    default:

      /* not supported */

      return(FAILURE);
    
      /*NOTREACHED*/

      break;
    }  /* END switch (AIndex) */

  return(SUCCESS);
  }  /* END MMBSetAttr() */





/**
 *
 *
 * @param MBP (I) [modified]
 * @param E (I)
 */

int MMBFromXML(

  mmb_t   **MBP,  /* I (modified) */
  mxml_t   *E)    /* I */

  {
  int     CTok;
  mxml_t *ME;

  char    tmpLine[MMAX_LINE << 1];
  char    Owner[MMAX_NAME];
  char    Source[MMAX_NAME];
  char    Label[MMAX_NAME];

  int     count;
  int     priority;

  enum MMBTypeEnum type;

  char   *ptr;

  mulong  etime;

  mmb_t  *tmpMB;

  CTok = -1;

  while (MXMLGetChild(E,"message",&CTok,&ME) == SUCCESS)
    {
    if (MXMLGetAttr(ME,(char *)MMBAttr[mmbaCount],NULL,tmpLine,sizeof(tmpLine)) == SUCCESS)
      {
      count = (int)strtol(tmpLine,NULL,10);
      }
    else
      {
      count = 1;
      }

    if (MXMLGetAttr(ME,(char *)MMBAttr[mmbaPriority],NULL,tmpLine,sizeof(tmpLine)) == SUCCESS)
      {
      priority = (int)strtol(tmpLine,NULL,10);
      }
    else
      {
      priority = 1;
      }

    MXMLGetAttr(ME,(char *)MMBAttr[mmbaType],NULL,tmpLine,sizeof(tmpLine));

    type = (enum MMBTypeEnum)MUGetIndexCI(tmpLine,MMBType,FALSE,mmbtNONE);

    if (MXMLGetAttr(ME,(char *)MMBAttr[mmbaExpireTime],NULL,tmpLine,sizeof(tmpLine)) == SUCCESS)
      {
      etime = strtol(tmpLine,NULL,10);
      }
    else
      {
      etime = MSched.Time + MCONST_DAYLEN;
      }

    if (MXMLGetAttr(ME,(char *)MMBAttr[mmbaOwner],NULL,Owner,sizeof(Owner)) == FAILURE)
      strcpy(Owner,"N/A");

    MXMLGetAttr(ME,(char *)MMBAttr[mmbaLabel],NULL,Label,sizeof(Label));

    MXMLGetAttr(ME,(char *)MMBAttr[mmbaSource],NULL,Source,sizeof(Source));

    if (MXMLGetAttr(
         ME,
         (char *)MMBAttr[mmbaData],
         NULL,
         tmpLine,
         sizeof(tmpLine)) == FAILURE)
      {
      return(FAILURE);
      }

    /* NOTE:  should 'pack/unpack' messages to handle '<' and '\n' characters (NYI) */ 

    if ((ptr = strchr(tmpLine,'\7')) != NULL)
      {
      /* unpack ckpt string - replace '\7' w/newlines */

      for (;ptr != NULL;ptr = strchr(ptr,'\7'))
        {
        *ptr = '\n';
        }
      }

    if (MMBAdd(
          MBP,
          tmpLine,
          Owner,
          type,
          etime,
          priority,
          &tmpMB) == FAILURE)
      {
      return(FAILURE);
      }

    if (count > 1)
      MMBSetAttr(tmpMB,mmbaCount,(void *)&count,mdfInt);

    if (Source[0] != '\0')
      MMBSetAttr(tmpMB,mmbaSource,(void *)Source,mdfString); 

    if (Label[0] != '\0')
      MUStrDup(&tmpMB->Label,Label);
    }    /* END while (MXMLGetChild() == SUCCESS) */

  return(SUCCESS);
  }   /* END MMBFromXML() */



/* END MMB.c */

