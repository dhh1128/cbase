
/* HEADER */

/**
 * @file MBitMaps.c
 *
 * Contains Bit Map operation functions
 */


#include <string>

#include "moab.h"
#include "moab-proto.h"
#include "moab-const.h"
#include "moab-global.h"


using namespace std;

/**
 *    the BM attribute is an array of unsigned chars, which grows depending on
 *    the bit to be set/unset.  
 *    the Size attribute is the number of chars in the BM array
 */

#define MDEF_BMSIZE 128

/* isolation macros */

#define OFFSET(bit)     ((bit) >> 3)
#define OFFSETBIT(bit)  (1 << ((bit) % 8))


/* Object methods */


/* default constructor */
mbitmap_t::mbitmap_t()
  {
  this->Size = 0;
  this->BM = NULL;
  } /* END mbitmap_t */

/* destructor */
mbitmap_t::~mbitmap_t()
  {
  if (BM != NULL)
    free(BM);
  BM = NULL;
  Size = 0;
  } /* END ~mbitmap_t */




/* assignment operator */
mbitmap_t & mbitmap_t::operator= (const mbitmap_t &rhs) 
  {
  /* Clear the Dest object */
  clear();

  /* check for NULL rhs - yeah, moab does that */
  if (NULL == &rhs)
    return(*this);

  /* check for self reference */
  if (this == &rhs)
    return(*this);
    
  /* if RHS has data, copy it to the Dest */
  if ((rhs.BM != NULL) && (rhs.Size > 0 ))
    {
    Size = rhs.Size;
    
    BM = (unsigned char *) calloc(1,Size);

    /* Copy the existing BM from RHS to Dest */
    std::copy(rhs.BM, rhs.BM + rhs.Size, BM);
    }

  return *this;
  } /* END operator= */

/* Copy constructor */
mbitmap_t::mbitmap_t(const mbitmap_t& rhs) 
        : BM( (unsigned char *) calloc(1,rhs.Size)), Size(rhs.Size)
  {
  std::copy(rhs.BM, rhs.BM + rhs.Size, BM);
  }  /* END mbitmap_t copy constructor */

/* bmclear method */
void mbitmap_t::clear(void)
  {
  if (BM == NULL)
    return;

  free(BM);

  BM = NULL;
  Size = 0;
  }  /* END clear() */


/**
 * Return a binary 'string' containing the output of the BitMap
 *
 * Format:  "00011010_11011010_" ...
 *
 */
std::string mbitmap_t::ToString(void)
  
  {
  string        retString("<empty>");

  if ((0 == Size) || (NULL == BM))    /* if no bits, then it is empty */
    return retString;

  retString = "";      /* Init to NULL string */

  /* Calc the number of bits contained in this BM, based on Size bytes of BM */
  int   bitCount = Size * 8;

  /* iterate over the bitmap, "toString" each bit */
  for (int bit = 0; bit < bitCount; bit++)
    {
    unsigned int Offset = OFFSET(bit);
    unsigned int OffsetBit = OFFSETBIT(bit);

    /* Check for a spacer ('_') is needed now in the sequence */
    if ((bit > 0) && ((bit % 8) == 0))
      {
        retString += "_";
      }

    /* Determine if bit SET or ZERO */
    if (((*(BM + Offset)) & OffsetBit) != 0)
      {
        retString += "1";
      }
    else
      {
        retString += "0";
      }
    }

  return(retString);
  } /* END ToString() */


/**
 * Convert bitmap to string.
 * 
 * NOTE:  There is another bmtostring that uses "mstring_t"
 * NOTE: default delimiter is ',' 
 *
 * @see bmfromstring()
 *
 * @param BM (I) [bitmap]
 * @param AList (I)
 * @param Buffer (O) [minsize=MMAX_LINE]
 * @param Delim (I) [optional, default is ',']
 * @param EString (I) Empty String [optional]
 */

char *bmtostring(
 
  const mbitmap_t  *BM,
  const char      **StringList,
  char             *Buffer,
  const char       *SDelim,
  const char       *EString)
 
  {
  char *BPtr;
  int   BSpace;

  const char *Delim = (SDelim == NULL) ? "," : SDelim;

  int   sindex;

  if ((BM == NULL) || (StringList == NULL) || (Buffer == NULL))
    {
    return(NULL);
    }

  MUSNInit(&BPtr,&BSpace,Buffer,MMAX_LINE);

  for (sindex = 1;StringList[sindex] != NULL;sindex++)
    {
    if (!bmisset(BM,sindex))
      continue;

    if (Delim[0] == '[')
      {
      MUSNPrintF(&BPtr,&BSpace,"[%s]",
        StringList[sindex]);
      }
    else
      {
      MUSNPrintF(&BPtr,&BSpace,"%s%s",
        (Buffer[0] != '\0') ? Delim : "",
        StringList[sindex]);
      }
    }  /* END for (sindex) */

  if (MUStrIsEmpty(Buffer) && (!MUStrIsEmpty(EString)))
    MUStrCpy(Buffer,EString,MMAX_LINE);

  return(Buffer);
  }  /* END bmtostring() */





/**
 * Convert multi-int bitmap to comma-delimited mstring_t.
 *
 * @param BMArray (I)
 * @param StringList (I)
 * @param OBuf (O)
 * @param SDelim (I) (optional) - "," if not specified
 * @param EString (I) (optional) - empty string value, NULL if not specified
 */

int bmtostring(

  const mbitmap_t  *BMArray,
  const char      **StringList,
  mstring_t        *OBuf,
  const char       *SDelim,
  const char       *EString)

  {
  const char *Delim = (SDelim == NULL) ? "," : SDelim;

  int   sindex;

  if ((BMArray == NULL) || (StringList == NULL) || (OBuf == NULL))
    {
    return(FAILURE);
    }

  MStringSet(OBuf,"");

  for (sindex = 1;StringList[sindex] != NULL;sindex++)
    {
    if (!bmisset(BMArray,sindex))
      continue;

    if (Delim[0] == '[')
      {
      MStringAppendF(OBuf,"[%s]",
        StringList[sindex]);
      }
    else
      {
      MStringAppendF(OBuf,"%s%s",
        (!OBuf->empty()) ? Delim : "",
        StringList[sindex]);
      }
    }  /* END for (sindex) */

  if ((OBuf->empty()) && (!MUStrIsEmpty(EString)))
    MStringSet(OBuf,EString);

  return(SUCCESS);
  }  /* END bmtostring() */





/**
 * Free the malloc'd memory for the bitmap.
 *
 * @param FlagBM (I/O,modified) Flag bitmap to be freed 
 */

void bmclear(
    
  mbitmap_t *BM)

  {
  /* map this function call to a object method */

  BM->clear();
  } /* END bmclear() */




/**
 * This routine copies a bitmap
 *
 * WARNING!!!!:  This routine assumes that the LastFlagEnum is 0 based (like mjifLAST) and not
 *               1 based liked MMAX_QOS.  For that reason any bitmap declared based on a 1 based
 *               integer should be 1 less (127 instead of 128) to avoid going off the end. This
 *               currently applies to MMAX_QOS, MMAX_ATTR, MMAX_PAR
 *
 * @param DstFlagBM (I/O,modified) Pointer to destination flag bitmap
 * @param SrcFlagBM (I) Pointer to flag bitmap to be cleared
 * @param LastFlagEnum (I) Last flag enum which gives the number of bits to be copied
 */

void bmcopy(
    
  mbitmap_t       *DstFlagBM,
  const mbitmap_t *SrcFlagBM)

  {
  if (DstFlagBM != NULL)
    DstFlagBM->clear();


  if (SrcFlagBM == NULL)
    return;

  /* Use the assignment operator override to do this */

  *DstFlagBM = *SrcFlagBM;

  }  /* END bmcopy() */






/*
 * Helper function to copy a flag from one bitmap to the other.
 *  NOTE:  This function does NOT do a check to make sure the bitmaps are
 *         of the same type.  It just copies from one index to the other.
 */

void bmcopybit(

  mbitmap_t       *BMTo,
  const mbitmap_t *BMFrom,
  unsigned int     Flag)

  {
  if (bmisset(BMFrom,Flag))
    {
    bmset(BMTo,Flag);
    }
  else
    {
    bmunset(BMTo,Flag);
    }

  return;
  } /* END bmcopybit() */


/**
 * Sets the bit depending on the boolean value.
 *
 * @param BM (I)
 * @param Flag (I)
 * @param SetTo (I)
 */

void bmsetbool(

  mbitmap_t    *BM,
  unsigned int  Flag,
  mbool_t       SetTo)

  {
  if (SetTo == TRUE)
    {
    bmset(BM,Flag);
    }
  else
    {
    bmunset(BM,Flag);
    }

  return;
  } /* END bmsetbool() */


/**
 * Report TRUE if bitmap has no values set.
 *
 * NOTE:  broken!  does not check last entry in array!
 *
 * @param Map (I)
 * @param Size (I) [in bits] size of bitmap
 */

mbool_t bmisclear(

  const mbitmap_t *Map)

  {
  unsigned int index;

  if ((Map == NULL) || (Map->Size == 0))
    return(TRUE);
    
  for (index = 0;index < Map->Size;index++)
    {
    if (Map->BM[index] != '\0')
      return(FALSE);
    } 

  return(TRUE);
  }  /* END bmisclear() */


/**
 * Set all the flags in a bitmap.
 *
 * NOTE: useful for partition bitmaps
 *
 * @param FlagBM (modified)
 * @param LastEnumOrMaxSize
 *
 */

void bmsetall(

  mbitmap_t     *FlagBM,
  unsigned int   LastEnumOrMaxSize)  /* ie: MMAX_PAR or mjfLAST */

  {
  unsigned int index;

  for (index = 0;index < LastEnumOrMaxSize;index++)
    {
    bmset(FlagBM,index);
    }

  return;
  }  /* END bmsetall() */





/**
 * Return TRUE if all the flags in a bitmap are set.
 *
 * NOTE: useful for partition bitmaps
 *
 * @param FlagBM (modified)
 * @param LastEnumOrMaxSize
 *
 */

mbool_t bmissetall(

  const mbitmap_t *FlagBM,
  unsigned int     LastEnumOrMaxSize)  /* ie: MMAX_PAR or mjfLAST */

  {
  unsigned int index;

  for (index = 0;index < LastEnumOrMaxSize;index++)
    {
    if (!bmisset(FlagBM,index))
      return(FALSE);
    }

  return(TRUE);
  }  /* END bmissetall() */


/**
 * Return 0 if the BMs are equal.
 *
 * @param BM1
 * @param BM2
 * @param LastEnumOrMaxSize
 */

int bmcompare(

  const mbitmap_t *BM1,
  const mbitmap_t *BM2)

  {
  unsigned int Max;
  unsigned int index;

  if ((BM1 == NULL) && (BM2 == NULL))
    {
    return(0);
    }

  if ((BM1 == NULL) && (BM2 != NULL))
    {
    return(1);
    }

  if ((BM1 != NULL) && (BM2 == NULL))
    {
    return(1);
    }

  Max = MAX(BM1->Size,BM2->Size);

  for (index = 0;index < Max;index++)
    {
    if ((index < BM1->Size) && (index < BM2->Size) &&
        (BM1->BM[index] != BM2->BM[index]))
      {
      return(1);
      }

    if ((index > BM1->Size) && (BM2->BM[index] != '\0'))
      {
      return(1);
      }

    if ((index > BM2->Size) && (BM1->BM[index] != '\0'))
      {
      return(1);
      }
    } /* END for (index) */

  return(0);
  }  /* END bmcompare() */



/**
 * Converts a string to a bitmap of the specified array.
 *
 * @param String (I)
 * @param Array (I)
 * @param BM (O)
 * @param SDelim (I) optional - string to parse on, otherwise ",[] \t\n"
 */

int bmfromstring(

  const char  *String,
  const char **Array,
  mbitmap_t   *BM,
  const char  *SDelim)

  {
  char *Line = NULL;
  const char *Delim;
  int index;

  char *TokPtr=NULL;
  char *ptr;

  if (BM !=  NULL)
    bmclear(BM);

  if ((MUStrIsEmpty(String)) || 
      (BM == NULL) || 
      (Array == NULL))
    {
    return(FAILURE);
    }

  if (SDelim != NULL)
    Delim = SDelim;
  else
    Delim = ",[] \t\n";

  MUStrDup(&Line,String);

  /* FORMAT:  <VAL>[,<VAL>]... */
  /* or       [<VAL>][,[<VAL>]]... */

  ptr = MUStrTok(Line,Delim,&TokPtr);

  while (ptr != NULL)
    {
    if ((index = MUGetIndexCI(ptr,Array,MBNOTSET,0)) > 0)
      {
      /* ignore attributes not listed */

      bmset(BM,index);
      }

    ptr = MUStrTok(NULL,Delim,&TokPtr);   
    }  /* END while (ptr != NULL) */

  MUFree(&Line);

  return(SUCCESS);
  }  /* END bmfromstring() */
 

/**
 * Returns the state of the requested flag bit in the bit map.
 *
 * @param FlagBM (I) Flag bitmap pointer 
 * @param FlagEnum (I) flag enum to be tested 
 */

mbool_t bmisset(
    
  const mbitmap_t  *FlagBM,
  unsigned int      Bit)

  {
  unsigned int Offset = OFFSET(Bit);
  unsigned int OffsetBit = OFFSETBIT(Bit);

  if (FlagBM == NULL)
    return(FALSE);

  if (Offset >= FlagBM->Size)
    return(FALSE);

  if (((*(FlagBM->BM + Offset)) & OffsetBit) != 0)
    return(TRUE);

  return(FALSE);
  }  /* END bmisset() */




/**
 * Sets a flag bit in the flag bitmap.
 *
 * @param FlagBM (I/O,modified) specified flag bit set 
 * @param FlagEnum (I) flag enum to be set
 */

/* coverity[+alloc : arg-*0] */
void bmset(
    
  mbitmap_t     *FlagBM,
  unsigned int   Bit)

  {
  if (FlagBM == NULL)
    return;

  unsigned char *tmp;
  unsigned int Offset = OFFSET(Bit);
  unsigned int OffsetBit = OFFSETBIT(Bit);
  unsigned int newSize = Offset + 1;

  /* Check if the BM is currently empty: BM == NULL, then empty */
  if (FlagBM->BM == NULL)
    {
    /* calloc the memory and check it BEFORE storing it */
    tmp = (unsigned char *) calloc(1,newSize);
    if (NULL == tmp)
      return;

    FlagBM->BM = tmp;
    FlagBM->Size = newSize;
    }
  else if (Offset >= FlagBM->Size)
    {
    unsigned int index;

    /* resize to larger buffer */

    /* calloc the memory and check it BEFORE storing it */
    tmp = (unsigned char *)realloc(FlagBM->BM,newSize);
    if (NULL == tmp)
      return;

    FlagBM->BM = tmp;

    /* Ensure the fresh bits are cleared first */
    for (index = FlagBM->Size;index < newSize;index++)
      {
      FlagBM->BM[index] = 0;
      }

    FlagBM->Size = newSize;
    }

  /* set the bit now */
  *(FlagBM->BM + Offset) |= OffsetBit;

  }  /* END bmset() */
    


/**
 *
 * Clears a flag bit in the flag bitmap.
 *
 * @param FlagBM (I/O,modified) specified flag bit set 
 * @param FlagEnum (I) flag enum to be set
 */

void bmunset(
    
  mbitmap_t     *FlagBM,
  unsigned int   Bit)

  {
  unsigned int Offset = OFFSET(Bit);
  unsigned int OffsetBit = OFFSETBIT(Bit);

  if (Offset >= FlagBM->Size)
    return;

  (*(FlagBM->BM + Offset)) &= ~(OffsetBit);
  }  /* END bmunset() */


/**
 * Logically OR Src and Dst maps with result in Dst.
 *
 * @param DstFlagBM (I/O)
 * @param SrcFlagBM (I)
 * @param MapSize   (I)  Number of bits in bitmap
 */

void bmor(

  mbitmap_t       *DstFlagBM,
  const mbitmap_t *SrcFlagBM)

  {
  unsigned int NumFlagBits;
  unsigned int index;

  if ((SrcFlagBM == NULL) || (DstFlagBM == NULL))
    {
    return;
    }

  NumFlagBits = SrcFlagBM->Size * 8;

  for (index = 0;index < NumFlagBits;index++)
    {
    if (bmisset(SrcFlagBM,index))
      bmset(DstFlagBM,index);
    }  /* END for (index) */
  }  /* END bmor() */





/**
 * Logically AND Src and Dst maps with result in Dst.
 *
 * @param DstFlagBM (I/O)
 * @param SrcFlagBM (I)
 * @param MapSize   (I)  Number of bits in bitmap
 */

void bmand(
 
  mbitmap_t       *DstFlagBM,
  const mbitmap_t *SrcFlagBM)

  {
  unsigned int NumFlagBits;
  unsigned int index;

  if ((SrcFlagBM == NULL) || (DstFlagBM == NULL))
    {
    return;
    }


  NumFlagBits = SrcFlagBM->Size * 8;

  for (index = 0;index < NumFlagBits;index++)
    {
    if (bmisset(SrcFlagBM,index) && bmisset(DstFlagBM,index))
      bmset(DstFlagBM,index);
    else
      bmunset(DstFlagBM,index);
    }  /* END for (index) */
  }  /* END bmand() */





/**
 * Logically NOT Dst map.
 *
 * @param DstFlagBM (I/O)
 * @param MapSize   (I)  Number of bits in bitmap
 */

void bmnot(

  mbitmap_t     *DstFlagBM,
  unsigned int   MapSize)

  {
  unsigned int NumFlagBits;
  unsigned int index;

  if (DstFlagBM == NULL)
    {
    return;
    }

  NumFlagBits = MapSize * 8;

  for (index = 0;index < NumFlagBits;index++)
    {
    if (bmisset(DstFlagBM,index))
     bmunset(DstFlagBM,index);
    }  /* END for (index) */
  }  /* END bmnot() */

/* END MBitMaps.c */

