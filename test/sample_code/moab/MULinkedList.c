/* HEADER */


/**
 * @file MULinkedList.c
 *
 * moab linked list object functions
 *
 * Singularly linked list. 
 *
 * mln_t * HeadPtr  ==>  mln_s *
 *
 * mln_s *Next points to next node. List is terminated with NULL pointer
 *
 */

#include "moab.h"
#include "moab-proto.h"
#include "moab-const.h"


/**
 * Creates a new head for a linked-list.
 *
 * @see MULLAdd() - peer
 *
 * @param LHead (O) [alloc] The new linked-list head.
 */

int MULLCreate(

  mln_t **LHead) /* O (alloc) */

  {
  if (LHead == NULL)
    {
    return(FAILURE);
    }

  if (*LHead != NULL)
    {
    /* don't overwrite existing memory */

    return(FAILURE);
    }

  *LHead = (mln_t *)MUCalloc(1,sizeof(mln_t));

  return(SUCCESS);
  }  /* END MULLCreate() */


/**
 * get the number of items in a linked list
 *
 * @param  LHead  (*)
 */

int MULLGetCount(

  mln_t    *LHead)

  {
  /* Map a bad pointer to no items */
  if (NULL == LHead)
    return(0);

  /* and return the number of items officially on the list */
  return(LHead->ListSize);
  }



/**
 * Prepends to a list. Does not traverse the list searching for duplicate
 * name values.
 *
 * @param LHead (I/O)
 * @param Name  (I)
 * @param DPtr  (I) [optional]
 * @return
 */

int MULLPrepend(

  mln_t     **LHead, /* I/O (modified) */
  char const *Name,  /* I */
  void       *DPtr)  /* I opaque data ptr (optional) */

  {
  mln_t *NewLink;
  mln_t *OldHead;

  if (NULL == LHead)
    {
    return(FAILURE);
    }

  if ((NewLink = (mln_t *)MUCalloc(1,sizeof(mln_t))) == NULL)
    {
    return(FAILURE);
    }

  OldHead = *LHead;

  MUStrDup(&NewLink->Name,Name);
  NewLink->Ptr = DPtr;
  NewLink->Next = OldHead;

  if (OldHead != NULL)
    NewLink->ListSize = OldHead->ListSize + 1;
  else
    NewLink->ListSize = 1;

  *LHead = NewLink;

  return(SUCCESS);
  }





/**
 * Add an element to the end of a linked list OR
 *
 * The old element will then be replaced with the new element Name given to 
 *   this function upon match constraints
 *
 * @see MULLCreate() - peer
 * @see MULLToString() - peer
 * @see MULLDup() - peer
 * @see MULLRemove() - peer
 * @see MULLUpdateFromString() - peer
 * @see MULLCheck() - peer
 *
 * NOTE:  DPtr is not allocated when attached to object
 * NOTE:  will override matching node
 * NOTE:  matching is case sensitive
 * 
 * @param LHead (I/O) [modified] The head of the list--a new list is created 
 *    if *LHead == NULL.
 * @param Name (I) The name of element to add
 * @param DPtr (I) [optional] The destination pointer. Will be put in the "ptr"
 *    field of the linked list.
 * @param LP (O) [optional] pointer to resulting linked list object 
 * @param Fn (I) [Optional] If Name is already in the list, this function 
 *    pointer will free the old element. 
 */

int MULLAdd(

  mln_t     **LHead, /* I/O (modified) */
  char const *Name,  /* I */
  void       *DPtr,  /* I opaque data ptr (optional) */
  mln_t     **LP,    /* O pointer to resulting linked-list object (optional) */
  int       (*Fn)(void **))  /* I free function (optional) */

  {
  mln_t *ptr;
  mln_t *prev;

  const char *FName = "MULLAdd";

  MDB(8,fSTRUCT) MLog("%s(%s,%s,%s,LP,%s)\n",
    FName,
    (LHead != NULL) ? "LHead" : "NULL",
    (Name != NULL) ? Name : "NULL",
    (DPtr != NULL) ? "DPtr" : "NULL",
    (Fn != NULL) ? "Fn" : "NULL");

  if (LP != NULL)
    *LP = NULL;

  if ((LHead == NULL) || (Name == NULL))
    {
    return(FAILURE);
    }

  /* initial setup of pointers */
  prev = NULL;

  ptr = *LHead;

  /* walk the list looking for:
   *  1) last node: then break out of loop
   *  2) If found a matching name, or an empty node, use it and return now
   *     An empty node is pre-allocated after a call to MULLCreate()
   *     has occurred, so this code handles this.
   */
  while (ptr != NULL)
    {
    /* If we encounter a control node w/o a name OR the new node name matches 
     * an existing node name, then set the new node's info into this existing 
     * control node on the list and done.
     */
    if ((ptr->Name == NULL) || !strcmp(ptr->Name,Name))
      {
      /* We found a control node we will use for the new data, 
       * if requested, return ptr to that control node
       */
      if (LP != NULL)
        *LP = ptr;

      /* if no name field on this item, then it is the 1st node after MULLCreate(),
       * so reuse it as a new entry for the incoming data
       */
      if (ptr->Name == NULL)
        {
        MUStrDup(&ptr->Name,Name);

        ptr->NameLen = strlen(Name);

        (*LHead)->ListSize++;  /* we've added a new item to this list */
        }

      /* Check for an existing payload pointer, if none, set with new payload ptr */
      if (ptr->Ptr == NULL)
        {
        ptr->Ptr = DPtr;
        }
      else if ((DPtr != NULL) && (DPtr != ptr->Ptr))
        {
        /* new payload pointer does NOT this control node's payload
         * ptr, so need to deconstruct the old payload ptr THEN
         * we set the new payload ptr into the existing control node
         */

        /* Check if payload pointer is present, if so go deconstruct it */
        if ((ptr->Ptr != NULL) && (Fn != NULL))
          {
          (*Fn)(&ptr->Ptr);   /* call payload deconstructor w/payload ptr */
          }

        ptr->Ptr = DPtr;      /* set the new payload ptr into control node */
        }

      return(SUCCESS);
      }  /* END if (!strcmp(ptr->Name,Name)) */

    prev = ptr;   /* Save a pointer to the current node, as we walk the list */

    ptr = ptr->Next;
    }  /* END while (ptr != NULL) */

  /* reached end of list so we are adding new element to linked list */

  /* Get a new control structure */
  if ((ptr = (mln_t *)MUCalloc(1,sizeof(mln_t))) == NULL)
    {
    return(FAILURE);
    }

  /* Check if list has nodes or is empty, and adjust ptrs accordingly */
  if (prev != NULL)
    {
    prev->Next = ptr;
    }
  else
    {
    /* This case is generated by a NULL mln_t head pointer in which
     * NO MULLCreate() has been called on that head pointer
     */
    *LHead = ptr;
    }

  /* fill in the new control node:
   *   create the name buffer and its length 
   *   have control node point to the caller's payload
   */
  MUStrDup(&ptr->Name,Name);

  ptr->NameLen = strlen(Name);

  ptr->Ptr = DPtr;

  /* increment size of list, and return pointer to control node */

  (*LHead)->ListSize++;

  if (LP != NULL)
    *LP = ptr;

  return(SUCCESS);
  }  /* END MULLAdd() */





/**
 * Duplicate all elements in a linked list.
 *
 * @see MULLAdd() - peer
 *
 * NOTE:  This routine assumes D->Ptr is a string and will alloc memory for 
 *        the new D->Ptr using MUStrDup() 
 *
 * NOTE:  If any memory allocator fails, the Destination list will
 *        be free'd and the function will return FAILURE
 *
 * @param DstLLP (O) [alloc]
 * @param SrcLL (I)
 */

int MULLDup(

  mln_t **DstLLP,  /* O (alloc) */
  mln_t  *SrcLL)   /* I */

  {
  int rc;
  mln_t *sptr;
  mln_t *dptr;

  mln_t *ptr;

  const char *FName = "MULLDup";

  MDB(5,fSTRUCT) MLog("%s(DstLLP,SrcLL)\n",
    FName);

  if ((DstLLP == NULL) || (SrcLL == NULL))
    {
    return(FAILURE);
    }

  MUFree((char **)DstLLP);

  dptr = NULL;

  for (sptr = SrcLL;sptr != NULL;sptr = sptr->Next)
    {
    ptr = (mln_t *)MUCalloc(1,sizeof(mln_t));

    /* No memory failure, need to tear down what we have built up
     * and return FAILURE
     */
    if (NULL == ptr)
      {
      MULLFree(DstLLP,MUFREE);
      return(FAILURE);
      }

    if (sptr->Ptr != NULL)
      {
      rc = MUStrDup((char **)&ptr->Ptr,(char *)sptr->Ptr);
      if (FAILURE == rc)
        {
        MULLFree(DstLLP,MUFREE);
        return(FAILURE);
        }
      }

    ptr->BM = sptr->BM;
    ptr->ListSize = sptr->ListSize;
    ptr->NameLen = sptr->NameLen;

    /* Dup the name into the destination, error check */
    rc = MUStrDup(&ptr->Name,sptr->Name);
    if (FAILURE == rc)
      {
      MULLFree(DstLLP,MUFREE);
      return(FAILURE);
      }

    if (dptr == NULL)
      {
      *DstLLP = ptr;
      }
    else
      {
      dptr->Next = ptr;
      }

    dptr = ptr;
    }  /* END for (sptr) */

  return(SUCCESS);
  }  /* END MULLDup() */





/**
 * Remove linked list node and free associated dynamically allocated memory.
 *
 * NOTE: Relink tree and adjust head pointer as needed.
 *
 * NOTE: this routine is dangerous and should not be placed in a "for" loop.
 *
 * @param LHead (I) [modified]
 * @param Name (I)
 * @param Fn (I) [optional]
 *
 * @return SUCCESS if Name was successfully found and removed from the list, FAILURE
 * otherwise.
 */

int MULLRemove(

  mln_t      **LHead,
  const char  *Name,
  int        (*Fn)(void **))

  {
  char *tName = NULL;  /* temporary local copy of Name parameter */

  /* NOTE:  assume local copy made to address threading issues/race conditions */

  mln_t *Prev;
  mln_t *ptr;
  mln_t *next;

  mbool_t FoundName = FALSE;

  const char *FName = "MULLRemove";

  MDB(9,fSTRUCT) MLog("%s(%s,%s,%s)\n",
    FName,
    (LHead != NULL) ? "LHead" : "NULL",
    (Name != NULL) ? Name : "NULL",
    (Fn != NULL) ? "Fn" : "NULL");

  if ((LHead == NULL) || (MUStrIsEmpty(Name)) || (*LHead == NULL))
    {
    return(FAILURE);
    }

  Prev = NULL;

  next = (*LHead)->Next;

  MUStrDup(&tName,Name);

  for (ptr = *LHead;ptr != NULL;ptr = next)
    {
    if (ptr->Name == NULL)
      break;

    if (strcmp(ptr->Name,tName))
      {
      /* current node does not match */

      Prev = ptr;

      next = ptr->Next;

      continue;
      }

    /* relink tree */

    if (Prev == NULL)
      {
      /* new head */

      *LHead = ptr->Next;

      if (*LHead != NULL)
        {
        (*LHead)->ListSize = ptr->ListSize;
        }
      }
    else
      {
      Prev->Next = ptr->Next;
      }

    /* free object */

    if (ptr->Name != NULL)
      MUFree(&ptr->Name);

    next = ptr->Next;

    if ((ptr->Ptr != NULL) && (Fn != NULL))
      {
      (*Fn)(&ptr->Ptr);
      }

    /* decrement list size */

    if (*LHead != NULL)
      (*LHead)->ListSize--;

    MUFree((char **)&ptr);

    FoundName = TRUE;
    }  /* END for (ptr) */

  MUFree(&tName);

  if (FoundName == TRUE)
    return(SUCCESS);
  else
    return(FAILURE);
  }  /* END MULLRemove() */





/**
 * Update an existing linked list with new values parsed from string.
 *
 * @see MULLToString() - peer
 *
 * @param HeadP (I) [modified]
 * @param Buf (I)
 * @param SDelim (I) [optional]
 *
 */

int MULLUpdateFromString(

  mln_t     **HeadP,   /* I (modified) */
  char       *Buf,     /* I */
  const char *SDelim)  /* I (optional) */

  {
  /* FORMAT:  <name>[=<value>][[;<name[=<value]]...] */

  mln_t *lptr;

  char *ptr;
  char *TokPtr = NULL;

  char *attr;
  char *val;

  char *TokPtr2 = NULL;

  char *tmpLine = NULL;

  const char *DDelim = ";";

  const char *Delim;
 
  mln_t *Head;

  const char *FName = "MULLUpdateFromString";

  MDB(5,fSTRUCT) MLog("%s(HeadP,%s,%s)\n",
    FName,
    (Buf != NULL) ? Buf : "NULL",
    (SDelim != NULL) ? SDelim : "NULL");
 
  if (HeadP == NULL)
    {
    return(FAILURE);
    }

  if (Buf == NULL)
    {
    return(SUCCESS);
    }

  if (SDelim != NULL)
    Delim = SDelim;
  else
    Delim = DDelim;

  /* add specified data */

  MUStrDup(&tmpLine,Buf);

  ptr = MUStrTok(tmpLine,Delim,&TokPtr);

  Head = *HeadP;

  while (ptr != NULL)
    {
    attr = MUStrTok(ptr,"=",&TokPtr2);
    val  = MUStrTok(NULL,"=",&TokPtr2);

    /* determine if value already exists */

    for (lptr = Head;lptr != NULL;lptr = lptr->Next)
      {
      if (lptr->Name == NULL)
        continue;

      if (!strcmp(attr,lptr->Name)) 
        {
        /* match located:  MUStrDup() will free lptr->Ptr */

        /* should lptr be removed if val == NULL? (NYI) */

        MUStrDup((char **)&lptr->Ptr,val);

        break;
        }
      }    /* END for (lptr) */

    if (lptr == NULL)
      {
      /* match not located - create new object */

      char *valp;

      if (val != NULL)
        valp = strdup(val);
      else
        valp = NULL;

      MULLAdd(HeadP,attr,(void *)valp,NULL,MUFREE); 

      Head = *HeadP;
      }  /* END if (lptr == NULL) */

    ptr = MUStrTok(NULL,Delim,&TokPtr);
    }  /* END while (ptr != NULL) */

  MUFree(&tmpLine);

  return(SUCCESS);
  }  /* END MULLUpdateFromString() */





/**
 * Report linked-list as delimited attr-value string.
 *
 * FORMAT:  <NAME>[=<VAL>][,<NAME>[=<VAL>]]...
 *
 * NOTE:  default attr-value delimiter is comma.
 *
 * NOTE:  There is also an MULLToMString()
 *
 * @see MULLUpdateFromString() - peer
 *
 * @param Head (I)
 * @param ShowExtended (I) [show extended object info]
 * @param SDelimStr (I) [optional]
 * @param Buf (O)
 * @param BufSize (I)
 */

int MULLToString(

  mln_t      *Head,          /* I */
  mbool_t     ShowExtended,  /* I (show extended object info) */
  const char *SDelimStr,     /* I (optional) */
  char       *Buf,           /* O */
  int         BufSize)       /* I */

  {
  char  *BPtr = NULL;
  int    BSpace = 0;

  mln_t *ptr;

  const char  *DefDelim = ",";

  const char  *DelimStr;  /* variable delimiter */

  const char *FName = "MULLToString";

  MDB(8,fSTRUCT) MLog("%s(Head,%s,%s,Buf,%d)\n",
    FName,
    MBool[ShowExtended],
    (SDelimStr != NULL) ? SDelimStr : "",
    BufSize);

  if (Buf != NULL)
    MUSNInit(&BPtr,&BSpace,Buf,BufSize);

  if ((Head == NULL) || (Buf == NULL))
    {
    return(FAILURE);
    }

  DelimStr = (SDelimStr != NULL) ? SDelimStr : DefDelim;

  for (ptr = Head;ptr != NULL;ptr = ptr->Next)
    {
    if ((ShowExtended == TRUE) && (ptr->Ptr != NULL))
      {
      MUSNPrintF(&BPtr,&BSpace,"%s%s=%s",
        (BPtr != Buf) ? DelimStr : "",
        ptr->Name,
        (char *)ptr->Ptr);
      }
    else
      {
      if (BPtr != Buf)
        {
        MUSNPrintF(&BPtr,&BSpace,",%s",
          ptr->Name);
        }
      else
        {
        MUSNPrintF(&BPtr,&BSpace,"%s",
          ptr->Name);
        }
      }
    }  /* END for (ptr = Head) */

  if (BSpace <= 0)
    {
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MULLToString() */


 /**
 * Report linked-list as delimited attr-value mstring_t.
 *
 * FORMAT:  <NAME>[=<VAL>][,<NAME>[=<VAL>]]...
 *
 * NOTE:  default attr-value delimiter is comma.
 *
 * @see MULLUpdateFromString() - peer
 *
 * @param Head (I)
 * @param ShowExtended (I) [show extended object info]
 * @param SDelimStr (I) [optional]
 * @param Buf (O)
 */

int MULLToMString(

  mln_t      *Head,          /* I */
  mbool_t     ShowExtended,  /* I (show extended object info) */
  const char *SDelimStr,     /* I (optional) */
  mstring_t  *Buf)         /* O */

  {
  mln_t *ptr;

  const char  *DefDelim = ",";

  const char  *DelimStr;  /* variable delimiter */

  const char *FName = "MULLToString";

  MDB(8,fSTRUCT) MLog("%s(Head,%s,%s,Buf)\n",
    FName,
    MBool[ShowExtended],
    (SDelimStr != NULL) ? SDelimStr : "");

  if ((Head == NULL) || (Buf == NULL))
    {
    return(FAILURE);
    }

  Buf->clear();

  DelimStr = (SDelimStr != NULL) ? SDelimStr : DefDelim;

  for (ptr = Head;ptr != NULL;ptr = ptr->Next)
    {
    if ((ShowExtended == TRUE) && (ptr->Ptr != NULL))
      {
      MStringAppendF(Buf,"%s%s=%s",
        (!Buf->empty()) ? DelimStr : "",
        ptr->Name,
        (char *)ptr->Ptr);
      }
    else
      {
      if (!Buf->empty())
        {
        MStringAppendF(Buf,",%s",
          ptr->Name);
        }
      else
        {
        MStringAppend(Buf,ptr->Name);
        }
      }
    }  /* END for (ptr = Head) */

  return(SUCCESS);
  }  /* END MULLToMString() */



/**
 * Free/destroy linked-list.
 *
 * @see MULLAdd()
 * @see MULLRemove()
 *
 * @param Head (I) [modified]
 * @param Fn   (I) [optional]
 */

int MULLFree(

  mln_t  **Head,
  int   (*Fn)(void **))

  {
  mln_t *ptr;
  mln_t *next;

  /* free linked list */

  if ((Head == NULL) || (*Head == NULL))
    {
    return(SUCCESS);
    }

  /* Walk the entire list, cleaning each node on the list */
  for (ptr = *Head;ptr != NULL;ptr = next)
    {
    /* If the name was allocated, free it */
    if (ptr->Name != NULL)
      {
      free(ptr->Name);
      ptr->Name = NULL;
      }

    next = ptr->Next;   /* Note next node after current one */

    /* If payload is non-NULL, then call a destructor on that payload */
    if (ptr->Ptr != NULL) 
      {
      /* Only if there is a 'destructor', call it */
      if (Fn != NULL)
        {
        (*Fn)(&ptr->Ptr);
        }
      else
        {
        /* payload ptr is not allocated, just clear it */

        ptr->Ptr = NULL;
        }
      }

    bmclear(&ptr->BM);

    free(ptr);    /* Finally, free the node's control structure */
    }  /* END for (ptr) */

  *Head = NULL;   /* Clear the head pointer when all is said and done */

  return(SUCCESS);
  }  /* END MULLFree() */





/**
 * Moab Utility Linked List Check - Find the node in the linked list with a 
 *   matching string value.
 * 
 * @see MULLCheckP() - peer - check for matching pointers
 * @see MULLCheckCI() - peer - case-sensitive check for matching string value
 *
 * NOTE:  search is case-insensitive.
 *
 * @param Head (I) The head of the linked list
 * @param Value (I) The value that we want to find in the list
 * @param LP (O) [Optional] Holds the first element in the linked list that 
 *   has a name matching Value or the string "ALL"
 */

int MULLCheck(

  mln_t  *Head,   /* I */
  char const *Value,  /* I */
  mln_t **LP)     /* O (optional) */

  {
  mln_t *ptr;
  int ValueLen;

  if (LP != NULL)
    *LP = NULL;

  if ((Head == NULL) || (Value == NULL))
    {
    return(FAILURE);
    }

  ValueLen = strlen(Value);

  for (ptr = Head;ptr != NULL;ptr = ptr->Next)
    {
    if (ptr->Name == NULL)
      continue;

    /* use strlen comparison to avoid strcasecmp */

    if (ptr->NameLen == ValueLen)
      {
      if (!strcasecmp(Value,ptr->Name))
        {
        if (LP != NULL)
          *LP = ptr;

        return(SUCCESS);
        }
      }

    if ((ptr->Name[0] == 'A') || (ptr->Name[0] == 'a')) 
      {
      if (!strcasecmp(ptr->Name,"ALL"))
        {
        if (LP != NULL)
          *LP = ptr;

        return(SUCCESS);
        }
      }
    }    /* END for (ptr) */

  return(FAILURE);
  }  /* END MULLCheck() */




/**
 * Moab Utility Linked List Check - Find the node in the linked list with a
 *   matching string value.
 *
 * @see MULLCheck() - peer - check for matching string values
 *
 * @param Head (I) The head of the linked list
 * @param Ptr (I) The pointer that we want to find in the list
 * @param LP (O) [optional] Holds the first element in the linked list that
 *   has a matching value 
 */

int MULLCheckP(

  mln_t  *Head,   /* I */
  void   *Ptr,    /* I */
  mln_t **LP)     /* O (optional) */

  {
  mln_t *ptr;

  if (LP != NULL)
    *LP = NULL;

  if ((Head == NULL) || (Ptr == NULL))
    {
    return(FAILURE);
    }

  for (ptr = Head;ptr != NULL;ptr = ptr->Next)
    {
    if (ptr->Name == NULL)
      continue;

    if (ptr->Ptr == Ptr)
      {
      if (LP != NULL)
        *LP = ptr;

      return(SUCCESS);
      }
    }    /* END for (ptr) */

  return(FAILURE);
  }  /* END MULLCheckP() */






/**
 * Moab Utility Linked List Check - Find the node in the linked list with a matching value (case-sensitive).
 * 
 * NOTE:  search is case-sensitive!
 * 
 * @see MULLCheck() - peer - case insensitive string value check
 *
 * @param Head (I) The head of the linked list
 * @param Value (I) The value that we want to find in the list
 * @param LP (O) [Optional] Holds the first element in the linked list that has a name matching Value
 */

int MULLCheckCS(

  mln_t      *Head,   /* I */
  char const *Value,  /* I */
  mln_t     **LP)     /* O (optional) */

  {
  mln_t *ptr;
  int ValueLen;

  if (LP != NULL)
    *LP = NULL;

  if ((Head == NULL) || (Value == NULL))
    {
    return(FAILURE);
    }

  ValueLen = strlen(Value);

  for (ptr = Head;ptr != NULL;ptr = ptr->Next)
    {
    if (ptr->Name == NULL)
      continue;

    /* use strlen comparison to avoid strcmp */

    if (ptr->NameLen == ValueLen)
      {
      if (!strcmp(Value,ptr->Name))
        {
        if (LP != NULL)
          *LP = ptr;

        return(SUCCESS);
        }
      }
    }    /* END for (ptr) */

  return(FAILURE);
  }  /* END MULLCheckCS() */





/**
 * Convienent function that iterates through a linked list--cuts down on
 * cut and paste code that is EVERYWHERE in Moab!
 *
 * @param Head (I)
 * @param Iter (I/O) [modified]
 */

int MULLIterate(

  mln_t  *Head,
  mln_t **Iter)

  {
  if ((Head == NULL) || (Iter == NULL))
    {
    return(FAILURE);
    }

  if (*Iter == NULL)
    {
    *Iter = Head;
    }
  else if ((*Iter)->Next != NULL)
    {
    *Iter = (*Iter)->Next;
    }
  else
    {
    return(FAILURE);  /* hit end of list */
    }

  if ((*Iter)->Name == NULL)
    {
    /* list is empty */

    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MULLIterate() */

/**
 * Function that iterates through a linked list--cuts down on
 * cut and paste code that is EVERYWHERE in Moab!
 *
 * If the payload pointer in the control structure (mln_t) is NULL, then
 * continue to next control structure. 
 *
 * With this iterator, no need to check for Iter->Ptr is NULL after each
 * SUCCESSful return.
 *
 * @param Head    (I)
 * @param Iter    (I/O) [modified]
 * @param Payload (O) [optional]
 */

int MULLIteratePayload(

    mln_t  *Head,
    mln_t **Iter,
    void  **Payload)

  {
  if ((NULL == Head) || (NULL == Iter))
    {
    return(FAILURE);
    }

  /* Check for first time call, and set the Iter state to Head */
  if (NULL == *Iter)
    *Iter = Head;
  else
    *Iter = (*Iter)->Next;

  for(;;)
    {
    /* When Iter becomes NULL, we are done w/failure */
    if (NULL == *Iter)
      return(FAILURE);

    /* If there is a Payload Ptr, then stop the iteration, otherwise skip this node */
    if (NULL != (*Iter)->Ptr)
      {
      if (NULL != Payload)
        *Payload = (*Iter)->Ptr;
      break;
      }

    *Iter = (*Iter)->Next;   /* move to next control node */
    }

  /* if no name on the control structure, treat as end of list */
  if (NULL == (*Iter)->Name)
    return(FAILURE);

  return(SUCCESS);
  }  /* END MULLIterate() */


/** Helper function for adding a variable to the variable linked list
 *
 * @see MTrigInitiateAction() - parent
 * @see MVCAddVarsToLLRecursive() - parent
 * @see MOGetCommonVarList() - parent
 * @see MUInsertVarList() - parent
 * @see MUJobGetVarList() - parent
 * @see MSysJobStar() - parent
 *
 * @param VarList [O] - Linked list to add variable to
 * @param Name    [I] - The variable name
 * @param Value   [I] - The variable value
 */

int MUAddVarLL(

    mln_t **VarList,
    const char *Name,
    char  *Value)

  {
  char *tmpS = NULL;
  int rc;

  /* tmpS is passed into the list, will be freed by the list */

  MUStrDup(&tmpS,Value);

  rc = MULLAdd(VarList,Name,(void *)tmpS,NULL,MUFREE);

  if (rc != SUCCESS)
    MUFree(&tmpS);

  return(rc);
  }  /* END MUAddVarLL() */


/* END MULinkedList.c */
