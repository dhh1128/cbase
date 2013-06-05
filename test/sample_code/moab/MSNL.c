/* HEADER */

/**
 * @file MSNL.c
 *
 * Contains various helper functions for the msnl_t object.
 *
 */

#include "moab.h"
#include "moab-proto.h"
#include "moab-global.h"


/**
 * Recalculates the total stored in S-count[0]
 *
 * @param S (I)
 */

int MSNLRefreshTotal(

  msnl_t *S)

  {

  MASSERT((S != NULL),"Unexpected NULL value for msnl_t *S.");

  return S->RefreshTotal();

  }  /* END MSNLRefreshTotal() */


/**
 * Recalculates the total stored in sum
 *
 */

int MSNL::RefreshTotal(

  void)

  {
  std::map<short,int>::iterator iter;

  sum = 0;

  if (data.empty())
    return(SUCCESS);

  for(iter = data.begin(); iter != data.end(); iter++)
    {
    sum += iter->second;
    }

  return(SUCCESS);
  }  /* END MSNL::RefreshTotal() */


/**
 * Clear the msnl_t structure
 *
 * @param S (I)
 */

int MSNLClear(

  msnl_t *S)

  {

  MASSERT((S != NULL),"Unexpected NULL value for msnl_t *S.");

  return S->Clear();

  }  /* END MSNLClear() */



/**
 * Clears an msnl_t class. 
 *
 */

int MSNL::Clear(

  void)

  {
  data.clear();
  sum = 0;

  return(SUCCESS);
  }  /* END MSNL::Clear() */

/**
 * Only used for gres currently.
 *
 * @param NumList (O)
 * @param String (I)
 * @param Delim (I) [optional]
 * @param SrcR (I) [optional] -- unused
 */

int MSNLFromString(

  msnl_t     *NumList,
  const char *String,
  const char *Delim,
  mrm_t      *SrcR)

  {
  char *cptr;
  char *tail;
  int   Count;
  int   CIndex;

  char *TokPtr = NULL;

  const char *DPtr;

  const char *DefDelim = "[]";

  char  tmpBuf[2];

  const char *FName = "MUNumListFromString";

  MASSERT((String != NULL),"Unexpected NULL value for char *String.");
  MASSERT((NumList != NULL),"Unexpected NULL value for msnl_t *NumList.");

  if (Delim != NULL)
    {
    DPtr = Delim;
    }
  else
    {
    if (strchr(String,',') != NULL)
      {
      tmpBuf[0] = ',';
      tmpBuf[1] = '\0';

      DPtr = tmpBuf;
      }
    else if (strchr(String,';') != NULL)
      {
      tmpBuf[0] = ';';
      tmpBuf[1] = '\0';

      DPtr = tmpBuf;
      }
    else
      { 
      DPtr = DefDelim;
      }
    }

  /* FORMAT:  [<ATTR>[<:|=><COUNT>]]... or <ATTR>[:<COUNT>][,<ATTR>[:<COUNT>]]... */

  if (MSNLClear(NumList) != SUCCESS)
    {
    return(FAILURE);
    }

  if (strstr(String,NONE) != NULL)
    {
    return(FAILURE);
    }

  /* create a mutable string from the String argument */
  char *mutableString = NULL;
  MUStrDup(&mutableString,String);

  cptr = MUStrTok(mutableString,DPtr,&TokPtr);

  if (cptr == NULL)
    {
    /* empty string pased in */

    MUFree(&mutableString);
    return(SUCCESS);
    }

  do
    {
    if (((tail = strchr(cptr,':')) != NULL) ||
        ((tail = strchr(cptr,'=')) != NULL) ||
        ((tail = strchr(cptr,' ')) != NULL))
      {
      *tail = '\0';

      Count = (int)strtol(tail + 1,NULL,0);
      }
    else
      {
      Count = 1;
      }

    if ((CIndex = MUMAGetIndex(meGRes,cptr,mVerify)) == FAILURE)
      {
      CIndex = MUMAGetIndex(meGRes,cptr,mAdd);

      if ((CIndex >= MSched.M[mxoxGRes]) || (CIndex <= 0))
        {
        MDB(3,fLL) MLog("WARNING:  GRES overflow in %s\n",
          FName);

        MUFree(&mutableString);
        return(FAILURE);
        }
      }     /* END if ((CIndex = MUMAGetIndex()) == FAILURE) */

    MSNLSetCount(NumList,CIndex,Count);
    }
  while ((cptr = MUStrTok(NULL,DPtr,&TokPtr)) != NULL);

  MUFree(&mutableString);
  return(SUCCESS);
  }  /* END MSNLFromString() */





/**
 * Convert msnl_t structure into human-readable string.
 *
 * NOTE: only works with meGRes currently
 * NOTE: does NOT return "NONE"
 *
 * @param ANL (I)
 * @param CNL (I) [optional]
 * @param DString (I) [optional]
 * @param String (O)
 * @param SLIndex (I) [optional]
 */

int MSNLToMString(

  msnl_t      *ANL,
  msnl_t      *CNL,
  const char  *DString,
  mstring_t   *String,
  int          SLIndex)

  {

  MASSERT((ANL != NULL),"Unexpected NULL value for msnl_t *ANL.");
  MASSERT((String != NULL),"Unexpected NULL value for mstring_t *String.");
  MASSERT((SLIndex == meGRes),"Unexpected non-meGRes value for int SLIndex.");

  return ANL->ToMString(CNL,DString,String,SLIndex);

  }  /* END MSNLToMString() */





/**
 * Convert msnl_t structure into human-readable string.
 *
 * NOTE: only works with meGRes currently
 * NOTE: does NOT return "NONE"
 *
 * @param CNL (I) [optional]
 * @param DString (I) [optional]
 * @param String (O)
 * @param SLIndex (I) [optional]
 */

int MSNL::ToMString(

  msnl_t      *CNL,
  const char  *DString,
  mstring_t   *String,
  int          SLIndex)

  {
  int         cindex;

  MASSERT((String != NULL),"Unexpected NULL value for mstring_t *String.");
  MASSERT((SLIndex == meGRes),"Unexpected non-meGRes value for int SLIndex.");

  MStringSet(String,"\0");

  if (this->IsEmpty())
    return(SUCCESS);

  /* NOTE:  if delimiter specified, only show class name, not count */
  /* NOTE:  DString only enabled for 'no CNL specification' */

  std::map<short,int>::const_iterator iter;
  for(iter = data.begin(); iter != data.end(); iter++)
    {
    cindex = iter->first;
    // TODO: what about array bounds errors since indicies are now free to go anywhere?
    if ((MGRes.Name[cindex] == NULL) ||
        (MGRes.Name[cindex][0] == '\0'))
      break;

    if (CNL != NULL)
      {
      if ((CNL->GetIndexCount(cindex) > 0) || 
          (this->GetIndexCount(cindex) > 0)) 
        {
        /* NOTE:  DString is ignored */

        MStringAppendF(String,"[%s %d:%d]",
          MGRes.Name[cindex],
          this->GetIndexCount(cindex),
          CNL->GetIndexCount(cindex)); 
        }
      }
    else
      {
      if (this->GetIndexCount(cindex) > 0) 
        {
        if (DString != NULL)
          {
          /* always print DString, this is needed to correctly parse nodes
             in mdiag -n */
          
          MStringAppendF(String,"%s=%d%s",
            MGRes.Name[cindex],
            this->GetIndexCount(cindex),
            DString);
          }
        else
          {
          MStringAppendF(String,"[%s %d]",
            MGRes.Name[cindex],
            this->GetIndexCount(cindex));
          }
        }
      }
    }   /* END for (cindex) */

  return(SUCCESS);
  }  /* END MSNL::ToMString() */



/**
 * Frees an msnl_t structure.
 *
 * @param S
 *
 */

int MSNLFree(

  msnl_t *S)

  {
  MASSERT((S != NULL),"Unexpected NULL value for msnl_t *S.");

  return S->Free();

  }  /* END MSNLFree() */



/**
 * Frees an msnl_t structure.
 *
 *
 */

int MSNL::Free(

  void)

  {
  /* generally a no-op  destructors of individual members clean themselves. */

  /* however, if created via malloc/free destructors won't be called, so.... */
  MSNL::Clear();

  return(SUCCESS);
  }  /* END MSNL::Free() */



/**
 * Initializes an msnl_t structure. (must be called before using any MSNL* functions)
 *
 * @param S
 */

int MSNLInit(

  msnl_t *S)

  {
  MASSERT((S != NULL),"Unexpected NULL value for msnl_t *S.");

  return S->Init();
  }  /* END MSNLInit() */



/**
 * Initializes an msnl_t class. (must be called before using any MSNL* functions)
 *
 */

int MSNL::Init(

  void)

  {
  MSNL::Clear();

  return(SUCCESS);
  }  /* END MSNL::Init() */



/**
 * Returns the count at the specified index.
 *
 * @param NumL (I)
 * @param Index (I)
 *
 */

int MSNLGetIndexCount(

  const msnl_t  *NumL,
  int            Index)

  {
  MASSERT((NumL != NULL),"Unexpected NULL value for msnl_t *NUML.");
  MASSERT((Index >= 0), "Index out of range (< 0).");

  if (Index == 0)
    return NumL->GetSum();
  else
    return NumL->GetIndexCount(Index);

  }  /* END MSNLGetIndexCount() */



/**
 * Returns the count at the specified index.
 *
 * @param Index (I)
 *
 */

int MSNL::GetIndexCount(

  int            Index) const

  {
  MASSERT((Index >= 0), "Index out of range (< 0).");

  std::map<short,int>::const_iterator iter = data.find(Index);
  if (iter == data.end())
    return 0;
  else
    return iter->second;

  }  /* END MSNL::GetIndexCount() */



/**
 * Returns the sum of all counts.
 *
 */

int MSNL::GetSum(

  void) const

  {
  return sum;
  }  /* END MSNL::GetSum() */



/**
 * Returns the sum of all counts.
 *
 */

int MSNL::Size(

  void) const

  {
  return data.size();
  }  /* END MSNL::Size() */



/**
 * Set the corresponding value in the SNL structure, update total.
 *
 * This routine sets the value and updates the total.
 *
 * @param NumL (I, modified)
 * @param Index (I)
 * @param Count (I)
 */

int MSNLSetCount(

  msnl_t  *NumL,
  int      Index,
  int      Count)

  {

  MASSERT((NumL != NULL),"Unexpected NULL value for msnl_t NumL.");
  MASSERT((Index >= 1), "Index out of range (< 1).");

  return NumL->SetCount(Index, Count);

  }  /* END MSNLSetCount() */



/**
 * Set the corresponding value in the SNL structure, update total.
 *
 * This routine sets the value and updates the total.
 *
 * @param Index (I)
 * @param Count (I)
 */

int MSNL::SetCount(

  int      Index,
  int      Count)

  {
  long Delta;

  MASSERT((Index >= 1), "Index out of range (< 1).");

  Delta = Count - data[Index];

  data[Index] = Count;

  /* Check for positive overflows */

  if ((Delta > 0) && (sum > INT_MAX - Delta))
    {
    /* positive overflow */

    sum = INT_MAX;
    }
  else
    {
    /* Check for negative overflows */

    if ((Delta < 0) && (sum < INT_MIN - Delta))
      {
      /* negative overflow condition */

      sum = INT_MIN;
      }
    else
      {
      /* No overflows -- Sum the total gres's */
  
      sum += Delta;
      }
    }

  return(SUCCESS);
  }  /* END MSNL::SetCount() */


/**
 * Copy SrcN to DstN
 *
 * @param DstN (O)
 * @param SrcN (I)
 */

int MSNLCopy(

  msnl_t        *DstN,
  const msnl_t  *SrcN)
 
  {

  MASSERT((DstN != NULL),"Unexpected NULL value for msnl_t *DstN.");
  MASSERT((SrcN != NULL),"Unexpected NULL value for msnl_t *SrcN.");

  return DstN->Copy(SrcN);

  }  /* END MSNLCopy() */





/**
 * Copy SrcN to this
 *
 * @param SrcN (I)
 */

int MSNL::Copy(

  const msnl_t  *SrcN)
 
  {
  MASSERT((SrcN != NULL),"Unexpected NULL value for msnl_t *SrcN.");

  /* are we copying to ourself?  no-op success */
  if (SrcN == this)
    return(SUCCESS);

  data.clear();

  data = SrcN->data;
  sum = SrcN->sum;

  return(SUCCESS);
  }  /* END MSNL::Copy() */


/**
 * Checks to see if msnl_t structure is empty
 *
 * @param N
 */

mbool_t MSNLIsEmpty(

  const msnl_t  *N)
 
  {
  MASSERT((N != NULL),"Unexpected NULL value for msnl_t *N.");
  
  return N->IsEmpty();

  }  /* END MSNLIsEmpty() */


/**
 * Checks to see if msnl_t structure is empty
 *
 */

mbool_t MSNL::IsEmpty(

  void) const
 
  {
  return data.empty(); 
  }  /* END MSNLIsEmpty() */


/**
 * Report instances of 'Req' which can be satisfied by 'Avail' msnl_t  
 * If SUCCESS returned, then *CPtr is set to the count (if non-null)
 *
 * @param Priority (I) -- unused
 * @param Req (I)
 * @param Avail (I)
 * @param CPtr (O)
 */

int MSNLGetCount(
 
  long          Priority,
  const msnl_t *Req,
  const msnl_t *Avail,
  int          *CPtr)
 
  {
 
  MASSERT((Req != NULL),"Unexpected NULL value for msnl_t *Req.");
  MASSERT((Avail != NULL),"Unexpected NULL value for msnl_t *Avail.");

  return Req->HowManyFit(Avail, CPtr);

  }  /* END MSNLGetCount() */      


/**
 * Report number of instances of this ('Req') which can be satisfied by 'Avail' msnl_t  
 * If SUCCESS returned, then *CPtr is set to the count (if non-null)
 *
 * @param Avail (I)
 * @param CPtr (O)
 */

int MSNL::HowManyFit(
 
  const msnl_t *Avail,
  int          *CPtr) const
 
  {
  int rindex;
  int Count;
 
  MASSERT((Avail != NULL),"Unexpected NULL value for msnl_t *Avail.");

  if (CPtr != NULL)
    *CPtr = 0;

  if (IsEmpty())
    {
    /* nothing required, Avail can provide unlimited instances */

    if (CPtr != NULL)
      *CPtr = MMAX_INT;
 
    return(SUCCESS);
    }
 
  if (MSNLIsEmpty(Avail))
    {
    /* something required but nothing available - no instances can be provided */

    return(FAILURE);
    }

  /* max available count cannot exceed global avail / global req */
 
  Count = Avail->GetSum() / this->GetSum();

  std::map<short,int>::const_iterator iter;
  for(iter = data.begin(); iter != data.end(); iter++)
    {
    rindex = iter->first;

    // TODO: what about array bounds errors since indicies are now free to go anywhere?
    if (MGRes.Name[rindex][0] == '\0')
      break;

    if (iter->second <= 0)
      continue;

    Count = MIN(Count,Avail->GetIndexCount(rindex) / iter->second);

    if (Count == 0)
      {
      return(FAILURE);
      }
    }      /* END for (rindex) */
 
  if (CPtr != NULL)
    *CPtr = Count;
 
  return(SUCCESS);
  }  /* END MSNL::HowManyFit() */      

/* END MSNL.c */
