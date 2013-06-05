/* HEADER */

/**
 * @file MHashTable.c
 *
 * Contains moab hash table (HT) object functions
 *
 */

#include "moab.h"
#include "moab-proto.h"


/**
 * Based on the endian-ness of the machine, it will
 * generate a bit-sensitive hash.
 *
 * @param Name (I) The string to hash.
 */

int MUGenHash(

  char const *Name)  /* I */

  {
  if (MLITTLE_ENDIAN)
    {
    return(hashlittle(Name,strlen(Name),0));
    }
  else
    {
    return(hashbig(Name,strlen(Name),0));
    }
  }  /* END MUGenHash() */


/**
 * Creates a hashtable by allocating its memory and creating a hash table array
 * based on HT->HashSize. This function will NOT free an already full 
 * hashtable -- make sure you call MUHTFree() on a hash table before trying to 
 * "re-create" it.
 *
 * NOTE: You don't have to call MUHTCreate() on a mhash_t structure -- you 
 * can use it immediately with MUHTAdd() and it will be automatically created.
 *
 * @param HT (I)
 * @param HashSize (I) [optional]
 */


int MUHTCreate(

  mhash_t *HT,
  int      HashSize)

  {
  if (HT == NULL)
    {
    return(FAILURE);
    }

  memset(HT,0x0,sizeof(mhash_t));

  if (HashSize > -1)
    HT->HashSize = HashSize;
  else
    HT->HashSize = MDEF_HASHTABLESIZE;

  /* Allocate memory, verify we got it 
   * Pointer size * the size of hashtable size (which is a shift factor on a 1)
   */
  HT->Table = (mln_t **)MUCalloc(1,MHASHSIZE(HT->HashSize) * sizeof(mln_t *));
  if (NULL == HT->Table)
    {
    HT->HashSize = 0;
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* MUHTCreate() */




/**
 * Checks whether the given HT is empty.
 *
 * @param HT (I)
 */

mbool_t MUHTIsEmpty(

  mhash_t *HT) /* I */

  {
  if (MUHTIsInitialized(HT) == FALSE)
    return(TRUE);

  if (HT->NumItems == 0)
    return(TRUE);

  return(FALSE);
  }  /* END MUHTIsEmpty() */





/**
 * Destroys and empties the given hashtable. It will free all of the objects in the hashtable
 * using the given free function, Fn.
 *
 * @param HT (I)
 * @param FreeObjects (I)
 * @param Fn (I) [optional]
 */

int MUHTFree(

  mhash_t *HT,           /* I */
  mbool_t  FreeObjects,  /* I */
  int   (*Fn)(void **))  /* I (optional) */

  {
  int hindex;
  int MaxSize;

  if (HT == NULL)
    {
    return(FAILURE);
    }

  if (HT->Table == NULL)
    {
    return(SUCCESS);
    }

  HT->NumCollisions = 0;
  HT->NumItems = 0;
  HT->LongestChain = 0;

  MaxSize = MHASHSIZE(HT->HashSize);

  /* Scan list for items in the table */
  for (hindex = 0;hindex < MaxSize;hindex++)
    {
    if (HT->Table[hindex] != NULL)
      {
      if (FreeObjects == TRUE)
        MULLFree(&HT->Table[hindex],Fn);
      else
        MULLFree(&HT->Table[hindex],NULL);
      }
    }

  MUFree((char **)&HT->Table);
  HT->HashSize = 0;

  return(SUCCESS);
  }  /* MUHTFree() */

/**
 * Empties the given hashtable BUT does not free the actual table!
 * It will free all of the objects in the hashtable
 * using the given free function, Fn.
 *
 * @param HT (I)
 * @param FreeObjects (I)
 * @param Fn (I) [optional]
 */

int MUHTClear(

  mhash_t *HT,           /* I */
  mbool_t  FreeObjects,  /* I */
  int   (*Fn)(void **))  /* I (optional) */

  {
  int hindex;
  int MaxSize;

  if (HT == NULL)
    {
    return(FAILURE);
    }

  if (HT->Table == NULL)
    {
    return(SUCCESS);
    }

  HT->NumCollisions = 0;
  HT->NumItems = 0;
  HT->LongestChain = 0;

  MaxSize = MHASHSIZE(HT->HashSize);

  for (hindex = 0;hindex < MaxSize;hindex++)
    {
    if (HT->Table[hindex] != NULL)
      {
      if (FreeObjects == TRUE)
        MULLFree(&HT->Table[hindex],Fn);
      else
        MULLFree(&HT->Table[hindex],NULL);
      }
    }

  return(SUCCESS);
  }  /* MUHTClear() */

/**
 * Adds the given object to the hashtable using Name as the key. A
 * general-purpose bitmap (BM) can also be passed in. If an object
 * with the same key already exists, the object will be overriden.
 * This function will automatically allocate and grow a hashtable--no
 * extra function or work is needed.
 *
 * NOTE: if you are passing in objects that you want to be freed when
 *   they are removed from the table, you should pass in Fn.  This is
 *   also used when growing the table, so if you don't pass it in the
 *   add, you may get a memory leak (if you are passing in a char *,
 *   for example).
 *
 * @param HT (I)
 * @param Name (I)
 * @param Object (I) [optional]
 * @param BM (I/O) [optional]
 * @param Fn (I) [optional]
 *
 */

int MUHTAdd(

  mhash_t    *HT,           /* I */
  char const *Name,         /* I */
  void       *Object,       /* I (optional) */
  mbitmap_t  *BM,           /* I/O (optional) */
  int       (*Fn)(void **)) /* I (optional) */

  {
  int Hash = 0;
  int Bucket = 0;
  int OldLSize = 0;
  mln_t *LLPtr;

  if ((HT == NULL) || (Name == NULL))
    {
    return(FAILURE);
    }

  if (HT->Table == NULL)
    {
    /* create table on-demand */

    if (MUHTCreate(HT,-1) == FAILURE)
      {
      return(FAILURE);
      }
    }

  /* rehash if longest chain grows too large! */

  if (HT->LongestChain >= MDEF_HASHCHAINMAX)
    {
    mhash_t NewHash;
    mhashiter_t HTIter;
    char *VarName;
    void *VarVal;
    mbitmap_t VarBM;

    /* need to grow table and rehash */

    MUHTCreate(&NewHash,HT->HashSize+1);  /* makes table 2x larger */

    MUHTIterInit(&HTIter);

    while (MUHTIterate(HT,&VarName,&VarVal,&VarBM,&HTIter) == SUCCESS)
      {
      MUHTAdd(&NewHash,VarName,VarVal,&VarBM,Fn);
      }

    /* free old table */

    MUHTFree(HT,FALSE,Fn);

    /* copy over new table */

    memcpy(HT,&NewHash,sizeof(mhash_t));
    memset(&NewHash,0x0,sizeof(mhash_t));
    }

  /* The best hash table sizes are powers of 2.  There is no need to do
   * mod a prime (mod is sooo slow!).  If you need less than 32 bits,
   * use a bitmask.  For example, if you need only 10 bits, do
   *   h = (h & MHASHMASK(10));
   *   In which case, the hash table should have MHASHSIZE(10) elements. */

  Hash = MUGenHash(Name);

  Bucket = (Hash & MHASHMASK(HT->HashSize));

  if (HT->Table[Bucket] == NULL)
    {
    MULLCreate(&HT->Table[Bucket]);
    }

  OldLSize = HT->Table[Bucket]->ListSize;

  MULLAdd(&HT->Table[Bucket],Name,Object,&LLPtr,Fn);

  if (BM != NULL)
    LLPtr->BM = *BM;

  if (OldLSize < HT->Table[Bucket]->ListSize)
    {
    if (OldLSize > 0)
      {
      /* bucket/hash collision detected! */

      HT->NumCollisions++;
      }

    HT->NumItems++;  /* mark that we've added an item */
    }

  HT->LongestChain = MAX(HT->Table[Bucket]->ListSize,HT->LongestChain);
    
  return(SUCCESS);
  }  /* END MUHTAdd() */





/**
 * Copy a hash object.
 *
 * Assumes both Src and Dst are initialized, and Src may have data items in it
 *
 * NOTE: Is actually more of a MUHTOr.
 * NOTE: Performs a DEEP copy and strdups VarVal
 *
 * NOTE: only works on J->TVariables (ie data is null terminated string)
 * 
 * @param Dst (O)
 * @param Src (I)
 */

int MUHTCopy(

  mhash_t       *Dst,
  const mhash_t *Src)

  {
  mhashiter_t HTIter;
  char       *VarName;
  void       *VarVal;
  mbitmap_t   VarBM;

  MUHTIterInit(&HTIter);

  while (MUHTIterate(Src,&VarName,&VarVal,&VarBM,&HTIter) == SUCCESS)
    {
    /* note that VarVal is optional and may be NULL */

    MUHTAdd(Dst,VarName,(VarVal != NULL) ? strdup((char *)VarVal) : NULL,&VarBM,MUFREE);
    }

  return(SUCCESS);
  }  /* END MUHTCopy() */





/**
 * Works exactly the same as MUHTAdd() except that it is case insensitive.
 *
 * @see MUHTAdd()
 *
 * @param HT     (I)
 * @param Name   (I)
 * @param Object (I) [optional]
 * @param BM     (I/O) [optional]
 * @param Fn     (I) [optional]
 *
 */

int MUHTAddCI(
 
  mhash_t     *HT,
  const char  *Name,
  void        *Object,
  mbitmap_t   *BM,
  int        (*Fn)(void **))

  {
  char tmpName[MMAX_NAME];

  MUStrCpy(tmpName,Name,sizeof(tmpName));

  /* make sure the name is lower case */
  MUStrToLower(tmpName);

  /* now that it is lower case call the normal function */

  return(MUHTAdd(HT,tmpName,Object,BM,Fn));
  }  /* END MUHTAddCI() */





/**
 * Adds the given object to the hashtable using an integer as the key. A
 * general-purpose bitmap (BM) can also be passed in. If an object
 * with the same key already exists, the object will be overriden.
 * This function will automatically allocate and grow a hashtable--no
 * extra function or work is needed.
 *
 * @see MUHTAdd()
 *
 * @param HT (I)
 * @param Key (I)
 * @param Object (I)
 * @param BM (I/O) [optional]
 * @param Fn (I) [optional]
 *
 */

int MUHTAddInt(

  mhash_t    *HT,
  int         Key,
  void       *Object,
  mbitmap_t  *BM,
  int   (*Fn)(void **))

  {
  char tmpKey[MMAX_LINE];

  snprintf(tmpKey,sizeof(tmpKey),"%d",
    Key);

  return(MUHTAdd(HT,tmpKey,Object,BM,Fn));
  }  /* END MUHTAddInt() */




/**
 * Removes an object from a hash table based on the given key (Name).
 *
 * @param HT (I)
 * @param Name (I)
 * @param Fn (I) [optional]
 *
 * @return SUCCESS if the key is found and succesfully removed, FAILURE otherwise.
 */

int MUHTRemove(

  mhash_t    *HT,
  const char *Name,
  int        (*Fn)(void **))

  {
  int Hash = 0;
  int Bucket = 0;

  if ((HT == NULL) ||
      (HT->Table == NULL) ||
      (Name == NULL))
    {
    return(FAILURE);
    }

  Hash = MUGenHash(Name);

  Bucket = (Hash & MHASHMASK(HT->HashSize));

  if (HT->Table[Bucket] == NULL)
    {
    return(FAILURE);
    }
  else
    {
    int OldLSize;

    OldLSize = HT->Table[Bucket]->ListSize;

    if (MULLRemove(&HT->Table[Bucket],Name,Fn) == SUCCESS)
      {
      if ((HT->Table[Bucket] == NULL) || (OldLSize > HT->Table[Bucket]->ListSize))
        HT->NumItems--;
      }
    else
      {
      return(FAILURE);
      }
    }

  return(SUCCESS);
  }  /* END MUHTRemove() */





/**
 * Works exactly like MUHTRemove() except it is case insensitive.
 *
 * @see MUHTRemove()
 *
 * @param HT   (I)
 * @param Name (I)
 * @param Fn   (I) [optional]
 */

int MUHTRemoveCI(

  mhash_t     *HT,           
  const char  *Name,        
  int        (*Fn)(void **))

  {
  char tmpName[MMAX_NAME];

  MUStrCpy(tmpName,Name,sizeof(tmpName));

  MUStrToLower(tmpName);

  return(MUHTRemove(HT,tmpName,Fn));
  } /* END MUHTRemoveCI() */




/**
 * Removes an object from a hash table based on the given key (which is an integer).
 *
 * @param HT (I)
 * @param Key (I)
 * @param Fn (I) [optional]
 */

int MUHTRemoveInt(

  mhash_t *HT,           /* I */
  int      Key,          /* I */
  int    (*Fn)(void **)) /* I (optional) */

  {
  char tmpKey[MMAX_LINE];

  snprintf(tmpKey,sizeof(tmpKey),"%d",
    Key);

  return(MUHTRemove(HT,tmpKey,Fn));
  }  /* END MUHTRemoveInt() */





/**
 * Return an object in the hashtable for the given Name (which is a string 
 * and is acting as the key).
 *
 * @see MUHTGetInt() - peer
 *
 * @param HT (I)
 * @param Name (I)
 * @param Object (O) [optional]
 * @param BM (O) [optional]
 * @return FAILURE if the given key is not in the hashtable.
 */

int MUHTGet(

  mhash_t    *HT,     /* I */
  char const *Name,   /* I */
  void      **Object, /* O (optional) */
  mbitmap_t  *BM)     /* O (optional) */

  {
  int Hash = 0;
  int Bucket = 0;
  mln_t *LLPtr;
  int rc;

  if (Object != NULL)
    *Object = NULL;

  if ((Name == NULL) || (HT == NULL))
    {
    return(FAILURE);
    }

  if (HT->Table == NULL)
    {
    return(FAILURE);
    }

  Hash = MUGenHash(Name);

  Bucket = (Hash & MHASHMASK(HT->HashSize));

  if (HT->Table[Bucket] == NULL)
    {
    return(FAILURE);
    }

  rc = MULLCheckCS(HT->Table[Bucket],Name,&LLPtr);

  if (rc == FAILURE)
    {
    return(FAILURE);
    }

  if (Object != NULL)
    *Object = LLPtr->Ptr;

  if (BM != NULL)
    bmcopy(BM,&LLPtr->BM);

  return(SUCCESS);
  }  /* END MUHTGet() */





/**
 * Functions exactly the same as MUHTGet() except it is case insensitive.
 *
 * @see MUHTGet()
 *
 * @param HT (I)
 * @param Name (I)
 * @param Object (O) [optional]
 * @param BM (O) [optional]
 * @return FAILURE if the given key is not in the hashtable.
 */
 
int MUHTGetCI(
 
  mhash_t    *HT,     /* I */
  char const *Name,   /* I */
  void      **Object, /* O (optional) */
  mbitmap_t  *BM)     /* O (optional) */

  {
  char tmpName[MMAX_NAME];

  MUStrCpy(tmpName,Name,sizeof(tmpName));

  /* make name all lower case */

  MUStrToLower(tmpName);

  /* now that the string is all lowercase, call the normal function */

  return(MUHTGet(HT,tmpName,Object,BM));
  } /* END MUHTGetCI() */





/**
 * Gets the object in the hashtable for the given Key (which is an integer).
 *
 * @see MUHTGet()
 *
 * @param HT (I)
 * @param Key (I)
 * @param Object (O) [optional]
 * @param BM (O) [optional]
 *
 * @return FAILURE if the given key is not in the hashtable.
 */

int MUHTGetInt(

  mhash_t   *HT,
  int        Key,
  void     **Object,
  mbitmap_t *BM)

  {
  char tmpKey[MMAX_LINE];

  snprintf(tmpKey,sizeof(tmpKey),"%d",
    Key);

  return(MUHTGet(HT,tmpKey,Object,BM));
  }  /* END MUHTGetInt() */





/**
 * Initializes the hashtable iterate object. Do this before iterating
 * over a hashtable.
 *
 * @see MUHTIterate() - peer
 *
 * @param Iter (I)
 */

int MUHTIterInit(

  mhashiter_t *Iter) /* I */

  {
  if (Iter == NULL)
    {
    return(FAILURE);
    }

  memset(Iter,0x0,sizeof(mhashiter_t));

  return(SUCCESS);
  }  /* END MUHTIterInit() */





/**
 * Iterates over the given hashtable, returning each item the hashtable contains.
 * This makes it easy to treat a hashtable like an unsorted "list".
 *
 * WARNING: Make sure Iter object is initialized with MUHTIterInit() before
 * calling this function.
 *
 * @param HT (I)
 * @param Name (O) optional
 * @param Object (O) optional
 * @param BM (O) optional
 * @param Iter (I/O) modified
 */

int MUHTIterate(

  const mhash_t   *HT,
  char           **Name,
  void           **Object,
  mbitmap_t       *BM,
  mhashiter_t     *Iter)

  {
  mln_t *tmpLL;

  if (Name != NULL)
    *Name = NULL;

  if (Object != NULL)
    *Object = NULL;

  if (BM != NULL)
    bmclear(BM);

  if ((HT == NULL) || (Iter == NULL))
    {
    return(FAILURE);
    }

  if (HT->Table == NULL)
    {
    return(FAILURE);
    }

  /* find the next full bucket if we are not already in one */

  if (Iter->LLNode == NULL)
    {
    while (Iter->Bucket < MHASHSIZE(HT->HashSize))
      {
      if (HT->Table[Iter->Bucket] == NULL)
        {
        Iter->Bucket++;

        continue;
        }

      Iter->LLNode = HT->Table[Iter->Bucket];
      Iter->Bucket++;

      break;
      }
    }

  /* find the next item in the bucket, if we are already in a valid bucket */

  while (Iter->LLNode != NULL)
    {
    tmpLL = Iter->LLNode;

    if (Name != NULL)
      *Name = tmpLL->Name;

    if (Object != NULL)
      *Object = tmpLL->Ptr;

    if (BM != NULL)
      *BM = tmpLL->BM;

    Iter->LLNode = tmpLL->Next;

    return(SUCCESS);
    }  /* END while (Iter->LLNode != NULL) */

  return(FAILURE);
  }  /* END MUHTIterate() */




/**
 * Creates an mstring representation of the hash table.
 * (Assumes that the objects held in the hash table are strings.)
 *
 * To specify the delimiter, use MUHTToMStringDelim()
 *
 * @param HT   (I) A pointer to the hashtable.
 * @param OBuf (O)
 */

int MUHTToMString(

  mhash_t   *HT,
  mstring_t *OBuf)

  {
  mhashiter_t HTIter;

  char *Name;
  char *Val;

  if ((HT == NULL) || (OBuf == NULL))
    {
    return(FAILURE);
    }


  *OBuf = "";

  MUHTIterInit(&HTIter);

  while (MUHTIterate(HT,&Name,(void **)&Val,NULL,&HTIter) == SUCCESS)
    {
    MStringAppendF(OBuf,"%s%s%s%s",
      (OBuf->empty()) ? "" : ",",
      Name,
      (Val != NULL) ? "=" : "",
      (Val != NULL) ? Val : "");
    }

  return(SUCCESS);
  }  /* END MUHTToMString() */


/**
 * Creates an mstring representation of the hash table.
 * (Assumes that the objects held in the hash table are strings.)
 *
 * To specify the delimiter, use MUHTToMStringDelim()
 *
 * @param HT    (I) A pointer to the hashtable.
 * @param OBuf  (O)
 * @param Delim (I)
 */

int MUHTToMStringDelim(

  mhash_t     *HT,
  mstring_t   *OBuf,
  const char  *Delim)

  {
  mhashiter_t HTIter;
                     
  char *Name;
  char *Val;
  char tmpLine[MMAX_LINE];
    
  if ((HT == NULL) || (OBuf == NULL) || 
      (Delim == NULL) || (Delim[0] == '\0'))
    {
    return(FAILURE);
    }
    
  *OBuf = "";

  MUHTIterInit(&HTIter);

  while (MUHTIterate(HT,&Name,(void **)&Val,NULL,&HTIter) == SUCCESS)
    {
    snprintf(tmpLine,sizeof(tmpLine),"%s%s%s",
      Name,
      (Val != NULL) ? "=" : "",
      (Val != NULL) ? Val : "");

    MStringAppendF(OBuf,"%s%s",
      (OBuf->empty()) ? "" : Delim,
      tmpLine);
    }

  return(SUCCESS); 
  }  /* END MUHTToMStringDelim() */


/**
 * This function will check if a hash table has been initialized.
 * 
 * WARNING: This function could return SUCCESS for hashtables that
 * have not been properly memset to 0!
 *
 * @param HT (I)
 */

int MUHTIsInitialized(

  mhash_t *HT)

  {
  if (HT == NULL)
    {
    return(FAILURE);
    }

  if ((HT->Table == NULL) || (HT->HashSize <= 0))
    {
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MUHTIsInitialized() */

