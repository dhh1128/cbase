/* HEADER */

/**
 * @file MXML.c
 *
 * Contains all XML related functions. 
 * Includes XML creation, destruction, manipulation, conversion to/from strings, etc.
 */

/*                                          *
 * Contains:                                *
 *                                          *
 * int MXMLExtractE(E,C,CP)                 *
 * int MXMLMergeE(Dst,Src,Delim)            *
 * int MXMLSetChild(E,CName,CE)             *
 * int MXMLCreateE(EP,Name)                 *
 * int MXMLDestroyE(EP)                     *
 * int MXMLSetAttr(E,A,V,Format)            *
 * int MXMLRemoveAttr(A,AName)              *
 * int MXMLAppendAttr(E,AName,AVal,Delim)   *
 * int MXMLSetVal(E,V,Format)               *
 * int MXMLAddE(E,C)                        *
 * int MXMLToXString(E,Buf,BufSize,MaxBufSize,Tail,NoCompress) *
 * int MXMLToString(E,Buf,BufSize,TailP,NoCompress) *
 * int MXMLGetAttrF(E,AName,ATok,AVal,DFormat,VSize) *
 * int MXMLGetAttr(E,AName,ATok,AVal,VSize) *
 * int MXMLGetChild(E,CName,CTok,C)         *
 * int MXMLGetChildCI(E,CName,CTok,CP)      *
 * int MXMLFromString(EP,XMLString,Tail,EMsg) *
 * int MXMLDupE(SrcE,DstEP)                 *
 * int MXMLFind(RootE,Name,Type,EP)         *
 *                                          */


#include "moab.h"
#include "moab-const.h"
#include "moab-proto.h"
#include "moab-global.h"


/**
 * Called when a source of data is too big for a destination
 * buffer. The buffer start is 'BufStart' and current position
 * pointer is 'BPtr', with 'BSpace' remaining in the buffer.
 *
 * It is possible for:
 *   a) no space left in destination
 *   b) some space left in destionation
 *   c) current position is AT the buffer start
 *
 * @param  BPtr       [I]  current buffer position pointer
 * @param  BSpace     [I]  remaining space of buffer before full
 * @param  BufStart   [I]  start of buffer
 */

int __MXMLTerminateBufferNoSpace(
    
  char **BPtr,
  int   *BSpace,
  char  *BufStart)

  {
  /* If there is absolutely NO space left in the buffer, then we
   * try to null termiante at the just previous byte location 
   * BUT......
   */
  if (*BSpace == 0)
    {
    /* Only place the null byte if AFTER start of buffer */
    if (*BPtr > BufStart)
      {
      (*BPtr)[-1] = '\0'; 
      }
    else
      {
      /* no room for terminator AT ALL so .... */

      /* NO-OP */
      }
    }
  else
    {
    /* Here there is some room in the buffer, so just place
     * the null byte at current position
     */
    *BPtr[0] = '\0';
    }

  return(SUCCESS);
  } /* END __MXMLTerminateBufferNoSpace() */


/* Ensure the following is set to the largest encoding possible
 * in __MXMLEncodeEscapeChars()
 */

#define  MXML_BIGGEST_ENCODING_SIZE    6

/**
 * Map a 'special' character to an encoded char sequence and put
 * that sequence into the destination buffer OR
 * just copy the non-special character to the destination buffer
 *
 * Caller must ensure there is enough space for at least 6 characters
 * of encoded sequence
 *
 * @param AValue        [I]   [start of input buffer]
 * @param Dest          [I]   [start of output buffer]
 * @param DestCurrPtr   [I/O] [Index of current position into dest buffer]
 */
int __MXMLEncodeEscapeChars(
  
  char AValue,
  char *Dest,
  int  *DestCurrPtr)

  {
  int  index = *DestCurrPtr;

  if (AValue == '<')
    {
    Dest[index++] = '&';
    Dest[index++] = 'l';
    Dest[index++] = 't';
    Dest[index++] = ';';
    }
  else if (AValue == '>')
    {
    Dest[index++] = '&';
    Dest[index++] = 'g';
    Dest[index++] = 't';
    Dest[index++] = ';';
    }
  else if (AValue == '&')
    {
    Dest[index++] = '&';
    Dest[index++] = 'a';
    Dest[index++] = 'm';
    Dest[index++] = 'p';
    Dest[index++] = ';';
    }
  else if (AValue == '"')
    {
    Dest[index++] = '&';
    Dest[index++] = 'q';
    Dest[index++] = 'u';
    Dest[index++] = 'o';
    Dest[index++] = 't';
    Dest[index++] = ';';
    }
  else
    {
    Dest[index++] = AValue;
    }

  *DestCurrPtr = index;

  return(SUCCESS);
  } /* __MXMLEncodeEscapeChars() */


/**
 * Check for escaped/encoded sequence of characters ('&...;') like
 * and if so, decode them to the proper original character
 *
 * Returns:
 *   SUCCESS if encoding map
 *
 * @see   __MXMLEncodeEscapeChars - to keep in sync
 *
 * @param Pptr       [I/O]   pointer to buffer char pointer, update consumed pointer
 * @param mappedChar [O]     Original character with NULL terminated (Expects a size of 2 chars)
 */

int __MXMLDecodeEscapeForBaseSet(
  
  char **Pptr,
  char   mappedChar[2])

  {
  char *ptr;
  int   rc = SUCCESS;

  ptr = *Pptr;   /* Grab the buffer pointer */

  mappedChar[1] = '\0';   /* NULL terminate the returned value */

  /* map the input character (*ptr) if encoded or just a string copy */

  if (0 == strncmp(ptr,"&lt;",4))
    {
    mappedChar[0] = '<';

    ptr += 4;
    }
  else if (0 == strncmp(ptr,"&gt;",4))
    {
    mappedChar[0] = '>';

    ptr += 4;
    }
  else if (0 == strncmp(ptr,"&amp;",5))
    {
    mappedChar[0] = '&';

    ptr += 5;
    }
  else if (0 == strncmp(ptr,"&apos;",6))
    {
    mappedChar[0] = '\'';

    ptr += 6;
    }
  else  
    {
    rc = FAILURE;
    }

  /* Return the updated the input pointer */

  *Pptr = ptr;

  return(rc);
  } /* __MXMLDecodeEscapeForBaseSet() */


/**
 * Check for escaped/encoded sequence of for 'quote' (")
 * and if so, decode them to the proper original character
 *
 * Returns:
 *   SUCCESS if encoding map
 *
 * @see   __MXMLEncodeEscapeChars - to keep in sync
 *
 * @param Pptr       [I/O]   pointer to buffer char pointer, update consumed pointer
 * @param mappedChar [O]     Original character with NULL terminated (Expects a size of 2 chars)
 */
int __MXMLDecodeEscapeForQuote(
  
  char **Pptr,
  char   mappedChar[2])

  {
  char *ptr;
  int   rc = SUCCESS;

  ptr = *Pptr;   /* Grab the buffer pointer */

  mappedChar[1] = '\0';   /* NULL terminate the returned value */

  /* map the input character (*ptr) if encoded or just a string copy */

  if (0 == strncmp(ptr,"&quot;",6))
    {
    mappedChar[0] = '"';

    ptr += 6;
    }
  else  
    {
    rc = FAILURE;
    }

  /* Return the updated the input pointer */

  *Pptr = ptr;

  return(rc);
  } /* __MXMLDecodeEscapeForQuote() */


/**
 * Check for escaped/encoded sequence of for ENVRS character
 * and if so, decode them to the proper original character
 *
 * Returns:
 *   SUCCESS if encoding map
 *   FAILURE if char was NOT encoded
 *
 * @see   __MXMLEncodeEscapeChars - to keep in sync
 *
 * @param Pptr       [I/O]   pointer to buffer char pointer, update consumed pointer
 * @param mappedChar [O]     Original character with NULL terminated (Expects a size of 2 chars)
 */
int __MXMLDecodeEscapeForENVRS(
  
  char **Pptr,
  char   mappedChar[2])

  {
  char *ptr;
  int   rc = SUCCESS;

  ptr = *Pptr;   /* Grab the buffer pointer */

  mappedChar[1] = '\0';   /* NULL terminate the returned value */

  /* map the input character (*ptr) if encoded or just a string copy */

  if (0 == strncmp(ptr,"~rs;",4))
    {
    mappedChar[0] = ENVRS_ENCODED_CHAR;

    ptr += 4;
    }
  else  
    {
    rc = FAILURE;
    }

  /* Return the updated the input pointer */

  *Pptr = ptr;

  return(rc);
  } /* __MXMLDecodeEscapeForENVRS */


/**
 * Convert encoded string from an ampersand (&) encoding
 *
 * Map the following char ESC sequences to their respective char:
 *
 * '&lt;'   ->    '<'
 * '&gt;'   ->    '>'
 * '&amp;'  ->    '&'
 * '&quot;' ->    '"'
 * '&apos;' ->    '''
 *
 * @see   __MXMLEncodeEscapeChars - to keep in sync
 *
 * @param Buf (I) [modified]
 */

int __MXMLStringConvertFromAmp(

  char *Buf)  

  {  
  char *ptr;
  int   index;
  char   mappedChar[2];  /* place to put original mapped char */

  /* NOTE:  output string should be guaranteed to be smaller and can 
            thus fit in original buffer */

  ptr = Buf;

  index = 0;

  /* FORMAT:  <STRING> */

  while (*ptr != '\0')
    {
    /* If the client is requesting XML do not translate any of the encoded
       special characters or it will produce invalid xml */
    if (((MSched.IsClient) && (C.Format == mfmXML)) || 
        (*ptr != '&'))
      {
      Buf[index++] = *ptr++;  /* copy straight across */
      }
    else if (__MXMLDecodeEscapeForBaseSet(&ptr,mappedChar) == SUCCESS)
      {
      /* Encoding found and mapped: Extract it and put in destination buffer */

      Buf[index++] = mappedChar[0];
      }
    else if (__MXMLDecodeEscapeForQuote(&ptr,mappedChar) == SUCCESS)
      {
      /* Encoding found and mapped: Extract it and put in destination buffer */

      Buf[index++] = mappedChar[0];
      }
    else
      {
      Buf[index++] = *ptr++;  /* copy straight across */
      }
    }    /* END while () */

  Buf[index] = '\0';

  return(SUCCESS);
  }  /* END __MXMLStringConvertFromAmp() */




/**
 * Extracts (removes) the given child from the XML structure.
 * Only removes the reference to the child from the XML tree
 * (recursively walks the child subtrees looking for the
 * specified 'child')
 *
 * Call will be responsible to 'destroy' the removed child
 *
 * @param E (I)              top tree container to search
 * @param C (I)              child reference to extract
 * @param CP (O) [optional]  pointer to extracted child
 */

int MXMLExtractE(

  mxml_t  *E,  
  mxml_t  *C, 
  mxml_t **CP) 

  {
  int cindex;

  /* Parameter check */
  if ((E == NULL) || (C == NULL))
    {
    return(FAILURE);
    }

  /* Search the child list for a match */
  for (cindex = 0;cindex < E->CCount;cindex++)
    {
    if (C != E->C[cindex])
      {
      /* If not in upper container, try lower nodes' list */
      if (MXMLExtractE(E->C[cindex],C,CP) == SUCCESS)
        {
        return(SUCCESS);  /* Found it */
        }

      continue;   /* iterate to next entry */
      }

    /* We have a match, so if caller asked for info, return it */
    if (CP != NULL)
      *CP = E->C[cindex];

    E->C[cindex] = NULL;   /* Remove reference to the child */

    return(SUCCESS);
    }  /* END for (cindex) */

  /* Didn't find the child in this sub-tree */
  return(FAILURE);
  }  /* MXMLExtractE() */



#if 0

Code not used in moab AND it has bugs in that it is difficult
to write a correct unittest for it

Keep, but define it out  30 Jan 2012  dougbert


/**
 * Append all the Attributes from Src and put them
 * into the Dest tree
 *
 * Src and Dest must have identical tree structure in
 * order for this to work
 *
 * @param Dst   (I)
 * @param Src   (I)
 * @param Delim (I)   Name<delim>Value format
 */

int MXMLMergeE(

  mxml_t  *Dst,   
  mxml_t  *Src,  
  char     Delim) 

  {
  int cindex;
  int aindex;

  /* parent and child must have identical element structure */

  if ((Dst == NULL) || (Src == NULL))
    {
    return(FAILURE);
    }

  /* Iterate the child list container */
  for (cindex = 0;cindex < Dst->CCount;cindex++)
    {
    /* recursively call all children in the list */

    if (MXMLMergeE(Dst->C[cindex],Src->C[cindex],Delim) == FAILURE)
      {
      return(FAILURE);
      }

    /* iterate over Src attributes and merge the attributes to dest */

    for (aindex = 0;aindex < Src->ACount;aindex++)
      {
      MXMLAppendAttr(
        Dst,
        Src->AName[aindex],
        (char *)Src->AVal[aindex],
        Delim);
      }
    }  /* END for (cindex) */

  return(SUCCESS);
  }  /* MXMLMergeE() */

#endif  




/**
 * Add XML element C (child) to XML element E (parent).
 *
 * @param E (I) [modified]
 * @param C (I) [required]
 */

int MXMLAddE(

  mxml_t *E, 
  mxml_t *C)

  {
  if ((E == NULL) || (C == NULL))
    {
    return(FAILURE);
    }

  /* Check if will fit into existing container or if need to grow */
  if (E->CCount >= E->CSize)
    {
    if (E->C == NULL)
      {
      E->C = (mxml_t **)MUCalloc(1,sizeof(mxml_t *) * MDEF_XMLICCOUNT);
      if (NULL == E->C)
        return(FAILURE);

      E->CSize = MDEF_XMLICCOUNT;
      }
    else
      {
      mxml_t      **NewPtr;

      NewPtr = (mxml_t **)MOSrealloc(E->C,sizeof(mxml_t *) * MAX(16,E->CSize << 1));

      if (NULL == NewPtr)
        return(FAILURE);

      E->C = NewPtr;
      E->CSize <<= 1;
      }
    }    /* END if (E->CCount >= E->CSize) */

  E->C[E->CCount] = C;

  E->CCount++;

  return(SUCCESS);
  }  /* END MXMLAddE() */



/**
 * Create/locate child of XML element.
 *
 * @param E     (I)
 * @param CName (I)
 * @param CEP   (O) [optional]
 */

int MXMLSetChild(

  mxml_t  *E,  
  char    *CName, 
  mxml_t **CEP)  

  {
  mxml_t *CE;

  if (CEP != NULL)
    *CEP = NULL;

  if ((E == NULL) || (CName == NULL))
    {
    return(FAILURE);
    }

  if (MXMLGetChild(E,CName,NULL,&CE) == SUCCESS)
    {
    /* located existing child */

    if (CEP != NULL)
      *CEP = CE;

    return(SUCCESS);
    }

  /* create new child */
  
  if ((CE = (mxml_t *)MUCalloc(1,sizeof(mxml_t))) == NULL)
    {
    return(FAILURE);
    }

  if (CName != NULL)
    {
    if (MUStrDup(&CE->Name,CName) == FAILURE)
      {
      MUFree((char **)&CE);

      return(FAILURE);
      }
    }

  if (CEP != NULL)
    *CEP = CE;

  MXMLAddE(E,CE);

  return(SUCCESS);
  }  /* END MXMLSetChild() */





/**
 * create/alloc an elment, regardless of value of EP 
 * (ie if already created, it 'drops' prior reference - leak)
 *
 * @param EP    (O)
 * @param Name  (I) [optional]
 */

int MXMLCreateE(

  mxml_t **EP,   
  const char    *Name)

  {
  /* NOTE:  should 'Name' be mandatory? (Make Mandatory in 5.0 - NYI) */

  if (EP == NULL)
    {
    return(FAILURE);
    }

  if ((*EP = (mxml_t *)MUCalloc(1,(sizeof(mxml_t)+1))) == NULL)
    {
    return(FAILURE);
    }

  if ((Name != NULL) && (Name[0] != '\0'))
    {
    if (MUStrDup(&(*EP)->Name,Name) == FAILURE)
      {
      MUFree((char **)EP);

      return(FAILURE);
      }
    }

  return(SUCCESS);
  }  /* END MXMLCreateE() */





/**
 * Recursively destroy xml element structure and its children.
 *
 * NOTE:  does not touch XD (and SHOULD NOT touch XD) 
 *
 * @param EP (I) [modified]
 */

int MXMLDestroyE(

  mxml_t **EP)  /* I (modified) */

  {
  int index;

  mxml_t *E;

  if (EP == NULL)
    {
    return(FAILURE);
    }

  E = *EP;

  if (E == NULL)
    {
    return(SUCCESS);
    }

  /* If any children, clean them up as well */

  if (E->C != NULL)
    {
    /* destroy children */

    for (index = 0;index < E->CCount;index++)
      {
      if (E->C[index] == NULL)  /* If empty slot, continue on */
        continue;

      MXMLDestroyE(&E->C[index]);  /* Destroy the child element */
      }  /* END for (index) */

    MUFree((char **)&E->C);   /* Free the children array container */
    }  /* END if (E->C != NULL) */

  /* free attributes */

  if (E->AName != NULL)
    {
    for (index = 0;index < E->ACount;index++)
      {
      if (E->AName[index] == NULL)   /* If end-of-list done */
        break;

      MUFree(&E->AName[index]);      /* Free this Name instance */

      /* If there is a Value, then free it as well */
      if ((E->AVal != NULL) && (E->AVal[index] != NULL))
        MUFree(&E->AVal[index]);
      }  /* END for (index) */

    /* Free the Value array container */
    if (E->AVal != NULL)
      {
      MUFree((char **)&E->AVal);
      }

    /* Finally free the Name array container */
    if (E->AName != NULL)
      {
      MUFree((char **)&E->AName);
      }
    }    /* END if (E->AName != NULL) */

  /* free CData */

  if (E->CData != NULL)
    {
    for (index = 0;index < E->CDataCount;index++)
      {
      if (E->CData[index] == NULL)  /* end of list */
        break;

      MUFree(&E->CData[index]);
      }

    MUFree((char **)&E->CData);
    }  /* END if (E->CData != NULL) */

  /* free name */

  if (E->Name != NULL)
    MUFree(&E->Name);

  if (E->Val != NULL)
    MUFree(&E->Val);

  MUFree((char **)EP);

  return(SUCCESS);
  }  /* END MXMLDestroyE() */





/**
 * Set an attribute within XML object.
 * 
 * Note: Will overwrite an existing attribute if found.
 *
 * @param E       (I) [modified] The element whose attribute will be set
 * @param A       (I) The name of the attribute to set
 * @param V       (I) The value that the attribute should be set to.
 * @param Format  (I) The format of "V". Can be a string, int, etc. See MDataFormatEnum in mcom.h.
 */

int MXMLSetAttr(

  mxml_t              *E,       
  const char          *A,      
  const void          *V,     
  enum MDataFormatEnum Format) 

  {
  int  aindex;
  int  iindex;

  int  rc;

  char tmpLine[MMAX_LINE];

  char *ptr;

  /* NOTE:  overwrite existing attr if found */

  if ((E == NULL) || (A == NULL))
    {
    return(FAILURE);
    }

  if (V != NULL)
    {
    switch (Format)
      {
      case mdfString:
      default:

        ptr = (char *)V;

        break;

      case mdfInt:

        sprintf(tmpLine,"%d",
          *(int *)V);

        ptr = tmpLine;

        break;

      case mdfLong:

        sprintf(tmpLine,"%ld",
          *(long *)V);

        ptr = tmpLine;

        break;

      case mdfDouble:

        sprintf(tmpLine,"%f",
          *(double *)V);

        ptr = tmpLine;

        break;
      }  /* END switch (Format) */
    }
  else
    {
    tmpLine[0] = '\0';

    ptr = tmpLine;
    }

  /* initialize attribute table */

  if (E->AName == NULL)
    {
    E->AName = (char **)MUCalloc(1,sizeof(char *) * MMAX_XMLATTR);
    E->AVal  = (char **)MUCalloc(1,sizeof(char *) * MMAX_XMLATTR);

    if ((E->AName == NULL) || (E->AVal == NULL))
      {
      return(FAILURE);
      }

    E->ASize = MMAX_XMLATTR;
    E->ACount = 0;
    }

  /* insert in alphabetical order */

  /* overwrite existing attribute if found (NOTE: case sensitive strcmp used) */

  aindex = 0;
  iindex = 0;
  rc     = 0;

  for (aindex = 0;aindex < E->ACount;aindex++)
    {
    rc = strcmp(E->AName[aindex],A);

    if (rc > 0)
      {
      /* have found entry which is beyond new attribute alphabetically */

      /* break out of loop and insert new entry below */

      break;
      }

    if (rc == 0)
      {
      /* match located - save index of match and break out */

      /* set insertion index to current index */

      iindex = aindex;

      break;
      }

    iindex = aindex + 1;
    }  /* END for (aindex) */

  if (rc != 0) /* one extra slot for terminator and one because of the 0 offset */
    {
    /* we did not find match */

    iindex = aindex;

    /* Check for full table condition */
    if (E->ACount >= E->ASize - 2) /* one extra slot for terminator and one because of the 0 offset */
      {
      char **NewAName;
      char **NewAVal;
      int    NewASize;

      /* allocate memory */
      NewASize = E->ASize << 1;
      NewAName = (char **)MOSrealloc(E->AName,sizeof(char *) * MAX(16,NewASize));
      if (NewAName == NULL)
        {
        /* FAILURE - cannot allocate memory, orginal memory is intact */

        return(FAILURE);
        }

      NewAVal  = (char **)MOSrealloc(E->AVal,sizeof(char *) * MAX(16,NewASize));

      if (NewAVal == NULL)
        {
        /* FAILURE - cannot allocate memory, orginal memory is intact */

        free(NewAName);
        return(FAILURE);
        }

      /* Alloc was successful, so update fields */
      E->AName = NewAName;
      E->AVal = NewAVal;
      E->ASize  = NewASize;
      }
    }    /* END if (aindex >= E->ACount) */

  if ((ptr[0] == '\0') && (aindex >= E->ACount))
    {
    /* no action required for empty attribute */

    return(SUCCESS);
    }

  /* prepare insertion point */

  if (rc != 0)
    {
    /* push all existing attribute fields out one more index */

    for (aindex = E->ACount - 1;aindex >= iindex;aindex--)
      {
      E->AVal[aindex + 1]  = E->AVal[aindex];
      E->AName[aindex + 1] = E->AName[aindex];
      }  /* END for (aindex) */

    E->AVal[aindex + 1]  = NULL;
    E->AName[aindex + 1] = NULL;
    }  /* END if (rc != 0) */

  if ((iindex < E->ACount) && (E->AVal[iindex] != NULL))
    MUFree(&E->AVal[iindex]);

  if (MUStrDup(&E->AVal[iindex],(char *)((ptr != NULL) ? ptr : "")) == FAILURE)
    {
    return(FAILURE);
    }

  if ((rc != 0) || (E->AName[iindex] == NULL))
    {
    MUStrDup(&E->AName[iindex],A);

    E->ACount++;
    }

  return(SUCCESS);
  }  /* END MXMLSetAttr() */




/**
 * Remove a specific attribute from an Element
 *
 * @param E (I) [modified]
 * @param AName (I)
 */

int MXMLRemoveAttr(

  mxml_t        *E, 
  const char    *AName) 

  {
  int aindex;

  if ((E == NULL) || (AName == NULL) || (AName[0] == '\0'))
    {
    return(FAILURE);
    }

  if ((E->ACount <= 0) || (E->AName == NULL))
    {
    return(SUCCESS);
    }

  for (aindex = 0;aindex < E->ACount;aindex++)
    {
    if ((E->AName[aindex] != NULL) && !strcmp(E->AName[aindex],AName))
      {
      /* match located */

      MUFree(&E->AVal[aindex]);
      MUFree(&E->AName[aindex]);

      for (;aindex < E->ACount;aindex++)
        {
        E->AVal[aindex]  = E->AVal[aindex + 1];
        E->AName[aindex] = E->AName[aindex + 1];
        }  /* END for (aindex) */

      E->ACount--;

      return(SUCCESS);
      }
    }    /* END for (aindex) */

  /* attribute not located */

  return(SUCCESS);
  }  /* END MXMLRemoveAttr() */




/**
 * Given an existing attribute in an Element, 'Append' a new
 * 'Value' to that same named attribute
 *
 * @param E       (I) root element
 * @param AName   (I) Attribute name
 * @param AVal    (I) new value to be appended
 * @param Delim   (I) Delimiter to use between the old value and the appended value
 */

int MXMLAppendAttr(

  mxml_t      *E,      
  const char  *AName, 
  const char  *AVal, 
  char         Delim)

  {
  int   ATok;
  int   len;

  char  VBuf[MMAX_LINE];

  if ((E == NULL) || (AName == NULL) || (AVal == NULL))
    {
    return(FAILURE);
    }

  ATok = -1;

  /* Get the named attribute, if successful, append to its existing value,
   * if not found, then simply ADD this attribute with the specified value
   */
  if (MXMLGetAttr(E,AName,&ATok,VBuf,sizeof(VBuf)) == SUCCESS)
    {
    char DString[2];
    char *NewString;

    /* allocate NEW space for old value, delimiter, new value and NULL */
    len = strlen(E->AVal[ATok]) + strlen(AVal) + 2;

    NewString = (char *)MOSrealloc(E->AVal[ATok], len);

    /* Check for failed realloc */
    if (NULL == NewString)
      {
      /* FAILURE - cannot allocate memory */

      return(FAILURE);
      }

    E->AVal[ATok] = NewString;
  
    DString[0] = Delim;
    DString[1] = '\0';

    /* just allocated adequate space, should not be able to overflow
     * and then concatenate the delim and new value to end of old value
     */
    strcat(E->AVal[ATok],DString);
    strcat(E->AVal[ATok],AVal);
    }
  else if (MXMLSetAttr(E,AName,AVal,mdfString) == FAILURE)
    {
    return(FAILURE); 
    }
 
  return(SUCCESS);
  }  /* END MXMLAppendAttr() */





/**
 * Set an attribute value by specified enumerated type
 *
 * @param E       (I) [modified]
 * @param V       (I)
 * @param Format  (I)
 */

int MXMLSetVal(

  mxml_t              *E,      
  const void          *V,     
  enum MDataFormatEnum Format)

  {
  char  tmpLine[MMAX_LINE];
  char *ptr;

  if ((E == NULL) || (V == NULL))
    {
    return(FAILURE);
    }

  if (E->Val != NULL)
    {
    MUFree(&E->Val);
    }

  switch (Format)
    {
    case mdfString:
    default:

      ptr = (char *)V;

      break;

    case mdfInt:

      sprintf(tmpLine,"%d",
        *(int *)V);

      ptr = tmpLine;

      break;

    case mdfLong:

      sprintf(tmpLine,"%ld",
        *(long *)V);

      ptr = tmpLine;

      break;

    case mdfDouble:

      sprintf(tmpLine,"%f",
        *(double *)V);

      ptr = tmpLine;

      break;
    }  /* END switch (Format) */
  
  if (MUStrDup(&E->Val,ptr) == FAILURE)
    {
    return(FAILURE);
    }

  /* strip '<' symbols  NOTE:  ignore '<' symbol in attrs */

  /* NOTE:  must replace temp hack 14 w/ &lt; */

  if (E->Val != NULL)
    {
    for (ptr = strchr(E->Val,'<');ptr != NULL;ptr = strchr(ptr,'<'))
      *ptr = (char)14;
    }

  return(SUCCESS);
  }  /* END MXMLSetVal() */





/**
 * Add child element to existing XML structure.
 *
 * @param E         (I)
 * @param CName     (I)
 * @param CVal      (I) [optional]
 * @param CEP       (O) [optional]
 */

int MXMLAddChild(

  mxml_t  *E,     
  char const *CName,
  char const *CVal,
  mxml_t **CEP)   

  {
  mxml_t *CE;

  if (CEP != NULL)
    *CEP = NULL;

  if ((E == NULL) || (CName == NULL))
    {
    return(FAILURE);
    }

  /* create new child */

  if ((CE = (mxml_t *)MUCalloc(1,sizeof(mxml_t))) == NULL)
    {
    return(FAILURE);
    }

  if (CName != NULL)
    {
    if (MUStrDup(&CE->Name,CName) == FAILURE)
      {
      MUFree((char **)&CE);

      return(FAILURE);
      }
    }

  if (CEP != NULL)
    *CEP = CE;

  MXMLAddE(E,CE);

  if (CVal != NULL)
    MXMLSetVal(CE,(void *)CVal,mdfString);

  return(SUCCESS);
  }  /* END MXMLAddChild() */



/**
 * Give input data, determine if that data indicates we should compress the data in the buffer
 *
 * @param    NoCompress     [I]
 * @param    Buf            [I/O]
 * @param    BufLen         [I]
 * @param    BufMax         [I]
 * @param    Name           [I]      'Name' of this 'attribute' buffer
 * @param    DataMatch      [I]      if 'DataMatch' equals 'Name' we proceed
 */

int __MXMLCompressDataAttribute(

  mbool_t     NoCompress,
  char       *Buf,
  int         BufLen,
  int         BufMax,
  char       *Name,
  const char *DataMatch)

  {
  /* If compression is requested and data is too big and the name matches perform the compression */

  if ((NoCompress == FALSE) && (BufLen > BufMax))
    {
    if ((Name != NULL) && !strcmp(Name,DataMatch))
      {
      /* attempt to compress in place */

      if (MSecCompress((unsigned char *)Buf,BufLen,NULL,MCONST_CKEY) == FAILURE)
        {
        return(FAILURE);
        }
      }
    }
  return(SUCCESS);
  }



/**
 * Convert an mxml_t structure tree to a valid XML String object
 *
 * This function will grow the given buffer (MOSrealloc)
 * on a per need basis.
 *
 * NOTE:  Will fail if XML is corrupt or memory cannot be allocated.
 *
 * @param E           (I)
 * @param Buf         (O) [populated/modified]
 * @param BufSizeP    (I/O)
 * @param MaxBufSize  (I)
 * @param Tail        (O)
 * @param NoCompress  (I)
 */

int MXMLToXString(

  mxml_t   *E,       
  char    **Buf,    
  int      *BufSizeP, 
  int       MaxBufSize, 
  char  const  **Tail,      
  mbool_t   NoCompress) 

  {
  int NewSize;
  char *tmpPtr;  /* need this so caller can free Buf if needs be */
  mstring_t tmpBuf(MMAX_LINE);

  /* NOTE:  MXMLToString() only fails due to lack of space */

  if ((E == NULL) || (Buf == NULL))
    {
    return(FAILURE);
    }

  /* allocate initial memory if required */

  if (*Buf == NULL)
    {
    NewSize = MMAX_BUFFER;

    if ((tmpPtr = (char *)MUMalloc(NewSize)) == NULL)
      {
      /* cannot allocate buffer */

      return(FAILURE);
      }

    *Buf = tmpPtr;

    if (BufSizeP != NULL)
      *BufSizeP = MMAX_BUFFER;
    }
  else
    {
    if (BufSizeP != NULL)
      {
      NewSize = *BufSizeP;
      }
    else
      {
      return(FAILURE);
      }
    }

  if (MXMLToMString(
      E,
      &tmpBuf,
      Tail, /* O */
      NoCompress) == FAILURE)
    {
    return(FAILURE);
    }

  int bufSize = (int) tmpBuf.size();
  if (bufSize >= NewSize)
    {
    NewSize = bufSize + 1;

    if ((*Buf = (char *)MOSrealloc(*Buf,NewSize)) == NULL)
      {
      /* FAILURE - cannot allocate buffer */
    
      return(FAILURE);
      }

    if (BufSizeP != NULL)
      *BufSizeP = NewSize;
    }    /* END if (strlen(tmpBuf.c_str()) >= NewSize) */

  strncpy(*Buf,tmpBuf.c_str(),NewSize);

  return(SUCCESS);
  }  /* END MXMLToXString() */


/**
 * convert the 'AName' attribute and its 'AValue' value to XML output
 *
 * @param   AName        (I)   Attrib name
 * @param   AValue       (I)   Attrib value
 * @param   BufStart     (I/O) Buffer start address
 * @param   BPtr         (I/O) Current position pointer into buffer 
 * @param   BSpace       (I/O) Remaining space in buffer
 */

int __MXMLToStringOneAttribute(
  
  char   *AName,
  char   *AValue,
  char   *BufStart,
  char  **BPtr,
  int    *BSpace)

  {
  char    NewLine[MMAX_BUFFER * 4];
  int     len;

  /* FORMAT:  <NAME>="<VAL>" */

  *BPtr[0] = ' ';
 
  *BPtr   += 1;
  *BSpace -= 1;

  len = strlen(AName);

  /* must account for '=' and '"' */

  if (len + 2 >= *BSpace)
    {
    /* insufficient space, so try to terminate with NULL byte if possible */

    __MXMLTerminateBufferNoSpace(BPtr,BSpace,BufStart);

    return(FAILURE);
    }

  /* Set the AName */
  strcpy(*BPtr,AName);

  *BSpace -= len;
  *BPtr   += len;

  /* Place a '=' */
  *BPtr[0] = '=';

  *BPtr   += 1;
  *BSpace -= 1;

  /* Place a '"' */
  *BPtr[0] = '"';

  *BPtr   += 1;
  *BSpace -= 1;

  /* Now place the Attribute Value */

  if ((AValue != NULL) &&
     ((strchr(AValue,'<') != NULL) ||
      (strchr(AValue,'>') != NULL) ||
      (strchr(AValue,'"') != NULL) ||
      (strchr(AValue,'&') != NULL)))
    {
    /* must replace '<' with '&lt;' and '>' with '&gt;' */

    int   index1;
    int   index2 = 0;

    len = strlen(AValue);

    if ((len >= (int)sizeof(NewLine)) || (len >= *BSpace))
      {
      /* insufficient space, so try to terminate with NULL byte if possible */

      __MXMLTerminateBufferNoSpace(BPtr,BSpace,BufStart);

      return(FAILURE);
      }

    /* Scan the AValue looking for special characters that need to be
     * encoded
     */
    for (index1 = 0;index1 < len;index1++)
      {
      /* Make sure that there is enough space in the destination buffer
       * for the encoded char sequence
       */
      if ((index2 + MXML_BIGGEST_ENCODING_SIZE) > (int)sizeof(NewLine))
        {
        /* insufficient space */

        return(FAILURE);
        }

      /* Examine character at AValue[index1] for encoding */
      __MXMLEncodeEscapeChars(AValue[index1],NewLine,&index2);
      }   /* END for (index1) */

    NewLine[index2] = '\0';

    len = strlen(NewLine);

    /* must account for closing '"' */

    if ((len + 1) >= *BSpace)
      {
      /* insufficient space, so try to terminate with NULL byte if possible */

      __MXMLTerminateBufferNoSpace(BPtr,BSpace,BufStart);

      return(FAILURE);
      }

    strcpy(*BPtr,NewLine);
    } /* END if (strchr(AValue,'<')...) */
  else
    {
    if (AValue != NULL)
      {
      len = strlen(AValue);
      }
    else
      {
      len = 0;
      }

    /* must account for closing '"' */

    if ((len + 1) >= *BSpace)
      {
      /* insufficient space, so try to terminate with NULL byte if possible */

      __MXMLTerminateBufferNoSpace(BPtr,BSpace,BufStart);

      return(FAILURE);
      }

    if (AValue != NULL)
      strcpy(*BPtr,AValue);
    }    /* END else */

  *BSpace -= len;
  *BPtr   += len;

  /* Terminate the Attribute Value */
  *BPtr[0] = '"';

  *BPtr   += 1;
  *BSpace -= 1;

  return(SUCCESS);
  }  /* END __MXMLToStringOneAttribute() */


/**
 * Given a char determine if it is 'special' char that must be encoded
 * with a 'encoded string'
 *
 * Single function to perform the encoding if needed
 *
 * @param chr   [I]    input char
 * @param NEptr [I/O]  pointer to char pointer to return encoded string [NULL if not encoded]
 * @param NElen [I/O]  pointer to int to return length of encoded string [0 if NEPtr is NULL]
 */

void __MXMLEncodeSpecialChar(

  const char   chr,
  const char **NEptr,
  int         *NElen)

  {
  switch (chr)
    {
    case '<':

      *NElen = 4;
      *NEptr = "&lt;";

      break;

    case '>':

      *NElen = 4;
      *NEptr = "&gt;";

      break;

    case '&':

      *NElen = 5;
      *NEptr = "&amp;";

      break;

    case ENVRS_ENCODED_CHAR:

      *NElen = 4;
      *NEptr = ENVRS_SYMBOLIC_STR;

     break;

    /* '"' not required to be parsed in node data per XML specification */
    /*
    case '"':

      NElen = 6;
      NEptr = "&quot;";

      break;
    */

    default:

      *NElen = 0;
      *NEptr = NULL;

      break;
    }
  } /* END __MXMLEncodeSpecialChar() */



/**
 * For given 'Value' generate XML output into the buffer 
 *
 * @param Val     (I)
 * @param Buf     (I/O) Buffer start address
 * @param BPtr    (O)
 * @param BSpace  (O)
 */

int __MXMLValueToString(
    
  char   *Val,
  char   *Buf,
  char  **BPtr,
  int    *BSpace)

  {
  int         len;
  char       *ptr;
  const char *NEptr;
  int         NElen;

  /* Do we have a Value ?? */

  if (Val != NULL)
    {
    len = strlen(Val);

    if (len >= *BSpace)
      {
      /* insufficient space, so try to terminate with NULL byte if possible */

      __MXMLTerminateBufferNoSpace(BPtr,BSpace,Buf);

      return(FAILURE);
      }

    NEptr = NULL;
    NElen = 0;

    ptr = Val;

    while (*ptr != '\0')
      {
      /* Check current character if 'special' and return its escaped string and length */

      __MXMLEncodeSpecialChar(*ptr,&NEptr,&NElen);

      /* If Len is 0, then it is NOT encoded, so simply store it straight thru */
      if (NElen == 0)
        {
        (*BPtr)[0] = *ptr;
        (*BPtr)++;

        BSpace--;
        }
      else
        {
        /* Special character and we have an encoded sequence to save */

        len += NElen - 1;

        if (len >= *BSpace)
          {
          /* insufficient space, so try to terminate with NULL byte if possible */

          __MXMLTerminateBufferNoSpace(BPtr,BSpace,Buf);

          return(FAILURE);
          }
        else
          {
          strncpy(*BPtr,NEptr,NElen);

          *BPtr += NElen;
          *BSpace -= NElen;
          }
        } /* END else */

      ptr++;  /* Iterator bump */

      } /* END while (*ptr != '\0') */
    }  /* END if (Val != NULL) */

  return(SUCCESS);
  } /* END __MXMLValueToString() */


/**
 * For the given Name string, generate XML into buffer
 *
 * @param   Name    (I)
 * @param Buf     (I/O) Buffer start address
 * @param BPtr    (O)
 * @param BSpace  (O)
 *
 */

int __MXMLFooterToString(

  char   *Name,
  char   *Buf,
  char  **BPtr,
  int    *BSpace)

  {
  int   len;

  if (Name != NULL)
    {
    len = strlen(Name);
    }
  else
    {
    len = strlen("NA");
    }

  if (*BSpace < len + 4)
    {
    /* insufficient space, so try to terminate with NULL byte if possible */

    __MXMLTerminateBufferNoSpace(BPtr,BSpace,Buf);

    return(FAILURE);
    }

  (*BPtr)[0] = '<';

  *BPtr   += 1;
  *BSpace -= 1;;

  (*BPtr)[0] = '/';

  *BPtr   += 1;
  *BSpace -= 1;;

  if (Name != NULL)
    {
    strcpy(*BPtr,Name);
    }
  else
    {
    strcpy(*BPtr,"NA");
    }
 
  *BSpace -= len;
  *BPtr   += len;

  (*BPtr)[0] = '>';   /* Terminate the phrase */

  *BPtr   += 1;
  *BSpace -= 1;;
  
  return(SUCCESS);
  }


/**
 * Helper function to print CData to a string
 *
 * No parameter checking
 *
 * @param CDataVal [I] - The value to print
 * @param Buf      [I] - The buffer (used to get length)
 * @param BPtr     [O] - Buffer to print to
 * @param BSpace   [I] - Space left in buffer
 */

int __MXMLCDataToString(

  const char *CDataVal,
  char       *Buf,
  char      **BPtr,
  int        *BSpace)

  {
  if (CDataVal != NULL)
    {
    int len;

    len = strlen(CDataVal) + strlen("<![CDATA[]]>");

    if (len >= *BSpace)
      {
      /* insufficient space, so try to terminate with NULL byte if possible */

      __MXMLTerminateBufferNoSpace(BPtr,BSpace,Buf);

      return(FAILURE);
      }

    MUSNPrintF(BPtr,BSpace,"<![CDATA[%s]]>",
      CDataVal);
    }

  return(SUCCESS);
  }  /* END __MXMLCDataToString() */




/**
 * Convert an mxml_t structure tree to a valid XML String object
 * 
 * @see MXMLToMString() for more dynamic mstring_t based routine
 *
 * @param E           (I) The mxml_t struct to convert
 * @param Buf         (O) The buffer that the XML string will be stored in
 * @param BufSize     (I) The size of the buffer (important for eliminating overflow)
 * @param TailP       (O) [optional]
 * @param NoCompress  (I) [boolean] Compress Data element.
 */

int MXMLToString(

  mxml_t   *E,       
  char     *Buf,    
  int       BufSize,
  char    **TailP, 
  mbool_t   NoCompress) 

  {
  int index;
  int BSpace;

  char *BPtr;
  char *tail;

  int   rc;
  int   len;

  /* Check parameters for validity */
 
  if (Buf != NULL)
    {
    if (BufSize > 0)
      Buf[0] = '\0';
    }

  if ((E == NULL) || (Buf == NULL))
    {
    return(FAILURE);
    }

  if (BufSize < MMAX_NAME)
    {
    return(FAILURE);
    }

  /* Initialize destination pointer and remaining space */

  BPtr   = Buf;
  BSpace = BufSize;

  /* display header */

  BPtr[0] = '<';

  BPtr++;
  BSpace--;

  if (E->Name != NULL)
    {
    len = strlen(E->Name);

    if (len >= BSpace)
      {
      /* insufficient space */

      BPtr[0] = '\0';

      return(FAILURE);
      }

    strcpy(BPtr,E->Name);

    BSpace -= len;
    BPtr   += len;
    }
  else
    {
    strcpy(BPtr,"NA");

    len = strlen("NA");

    BPtr   += len;
    BSpace -= len;
    }

  index = 0; /* So valgrind won't complain */

  /* iterate over and display all attributes of this element */

  for (index = 0;index < E->ACount;index++)
    {
    rc = __MXMLToStringOneAttribute(E->AName[index],E->AVal[index],Buf,&BPtr,&BSpace);
    if (FAILURE == rc)
      {
      return(FAILURE);
      }
    }  /* END for (index) */

  BPtr[0] = '>';      /* terminate the output with trailing angle bracket */
  BPtr++;
  BSpace--;

  /* ToString the element's 'Value' if any */

  rc = __MXMLValueToString(E->Val,Buf,&BPtr,&BSpace);
  if (FAILURE == rc)
    {
    return(FAILURE);
    }

  /* display CData */

  if (E->CData != NULL)
    {
    for (index = 0;index < E->CDataCount;index++)
      {
      if (E->CData[index] == NULL)
        break;

      rc = __MXMLCDataToString(E->CData[index],Buf,&BPtr,&BSpace);

      if (FAILURE == rc)
        {
        return(FAILURE);
        }
      }
    }

  /* iterate over and display children of this element */

  for (index = 0;index < E->CCount;index++)
    {
    if (E->C[index] == NULL)
      continue;

    if (MXMLToString(E->C[index],BPtr,BSpace,&tail,NoCompress) == FAILURE)
      {
      return(FAILURE);
      }

    len = strlen(BPtr);

    BSpace -= len;
    BPtr   += len;
    }  /* END for (index) */

  /* display footer (element Name) */

  rc = __MXMLFooterToString(E->Name,Buf,&BPtr,&BSpace);
  if (FAILURE == rc)
    {
    return(FAILURE);
    }

  /* terminate string */

  BPtr[0] = '\0';

  /* Wrap up */

  if (TailP != NULL)    /* Return pointer to any 'tail' of the input string */
    *TailP = BPtr;

  /* Check if compression is requested, data is too big, item is "Data" */

  if (__MXMLCompressDataAttribute(NoCompress,Buf,strlen(Buf),(MMAX_BUFFER >> 1),E->Name,"Data") == FAILURE)
    return(FAILURE);

  return(SUCCESS);
  }  /* END MXMLToString() */


/**
 * Given one Attribute value, toString it to a mstring_t
 *
 * @param  MString    [I/O]     destination buffer to store output string
 * @param  AName      [I]       Attribute Name
 * @param  AValue     [I]       Atrribute Value
 * @param  NewLine    [I]       already initialized temp buffer
 */

int __MXMLToMStringOneAttribute(
    
  mstring_t   *MString,
  char        *AName,
  char        *AValue,
  mstring_t   *NewLine)

  {
  int           RegCharCount;
  int           len;
  const char   *Encoded;

  /* RegCharCount -
      Before we were checking each character, and appending regular characters
      (not '<', '>', '"', '&') one at a time via MStringAppendF because
      MStringAppend only accepts strings.  This was very slow, since we were
      calling a large function for every char.  RegCharCount is a count of 
      the number of consecutive regular characters that we've had.  When we
      hit an irregular character, we will append the entire string of regular
      characters at once, and then append the irregular stuff.  This cuts
      down calls to MStringAppendF significantly, and speeds up this function.
      MStringAppendCount is used to append the regular chars, using
      (<CurPtr> - RegCharCount) as the pointer. */

  /* FORMAT:  <NAME>="<VAL>" */

  /* Compose a simply string from the AName in the form:  AName="
   * then set that to the MString
   */
  MStringAppendF(MString," %s=\"",AName);

  RegCharCount = 0;

  /* If the AValue is set and if any specials are in the AValue input string,
   * then perform per character processing on the AValue for special encoding
   */
  if ((AValue != NULL) && (strpbrk(AValue,"<>\"&") != NULL))
    {
    /* must replace '<' with '&lt;' 
     *          and '>' with '&gt;' 
     *          and '&' with '&amp;' 
     *          and '"' with '&quot;' 
     */

    int   index1;

    MStringSet(NewLine,"");   /* Init the temp MString */

    len = strlen(AValue);

    for (index1 = 0;index1 < len;index1++)
      {
      switch (AValue[index1])
        {
        case '<':
          Encoded = "&lt;";
          break;

        case '>':
          Encoded = "&gt;";
          break;

        case '&':
          Encoded = "&amp;";
          break;

        case '"':
          Encoded = "&quot;";
          break;

        default:
          Encoded = NULL;
          break;
        }

      /* If no special characters then just bump the Reg char count */

      if (NULL == Encoded)
        {
        /* Regular character, keep track of how many we've had in a row */

        RegCharCount++;
        }
      else
        {
        /* We have a special char, so post any regular chars outstanding */
        if (RegCharCount > 0)
          {
          MStringAppendCount(NewLine,&AValue[index1 - RegCharCount],RegCharCount);
          }

        /* Append the special character encoding string */
        MStringAppend(NewLine,Encoded);
        RegCharCount = 0;
        }
      } /* END for(index1...) */

    if (RegCharCount > 0)
      {
      /* Regular chars until end of string */

      MStringAppend(NewLine,&AValue[index1 - RegCharCount]);
      }

    /* Transfer the temp MString to the destination MString */

    MStringAppend(MString,NewLine->c_str());

    } /* END if (strchr(AValue,'<')...) */
  else if (AValue != NULL)
    {
    MStringAppend(MString,AValue);
    }    /* END else */

  MStringAppend(MString,"\"");

  return(SUCCESS);
  } /* END __MXMLToMStringOneAttribute() */


/**
 * ToMString a Value item
 *
 */

int __MXMLValueToMString(
  
  mstring_t   *MString, 
  char        *Val)
  {
  const char  *NEptr;
  int          NElen;
  char        *ptr;
  int          RegCharCount; /* Num of consecutive regular chars we've had */

  /* If there is an Element Value, toMString it */

  if (Val != NULL)
    {
    NEptr = NULL;
    NElen = 0;

    ptr = Val;
    RegCharCount = 0;

    while (*ptr != '\0')
      {
      /* Check current character if 'special' and return its escaped string and length */
  
      __MXMLEncodeSpecialChar(*ptr,&NEptr,&NElen);
  
      /* If not special, then copy character straight through */
  
      if (NEptr == NULL)
        {
        /* Regular character, keep track of how many we've had in a row */
        RegCharCount++;
        }
      else
        {
        /* Irregular character, copy all of the past regular characters */
  
        if (RegCharCount > 0)
          MStringAppendCount(MString,ptr - RegCharCount,RegCharCount);
  
        /* Now add the irregular stuff */
  
        MStringAppend(MString,NEptr);
  
        RegCharCount = 0;   /* reset the Regular character count outstanding */
        }
  
      ptr++; /* Iterator bump */
  
      } /* END while (*ptr != '\0') */
  
    /* If any regular characters, append them to the buffer as well */
    if (RegCharCount > 0)
      {
      /* Reg chars until end of string */
  
      MStringAppend(MString,ptr - RegCharCount);
      }
    }

  return(SUCCESS);
  } /* END __MXMLValueToMString() */


/**
 *  Helper function to print CData to an MString.
 *  No error checking of the mstring
 *
 * @param MString  [O] - The mstring to print to
 * @param CDataVal [I] - The CData string to print
 */

int __MXMLCDataToMString(

  mstring_t *MString,
  char      *CDataVal)

  {
  if (CDataVal != NULL)
    {
    MStringAppendF(MString,"<![CDATA[%s]]>",
      CDataVal);
    }

  return(SUCCESS);
  }  /* END __MXMLCDataToMString() */


/**
 * Convert an mxml_t structure tree to a valid XML MString object
 * 
 * @param E           (I) The mxml_t struct to convert
 * @param MString     (O) The buffer that the XML string will be stored in (should already be initialized) 
 * @param TailP       (O) [optional]
 * @param NoCompress  (I) [boolean] Compress Data element.
 */

int MXMLToMString(

  mxml_t     *E,           
  mstring_t  *MString,    
  char const **TailP,     
  mbool_t     NoCompress)

  {
  int       index;
  int       len;

  char const *tail;

  if ((E == NULL) || (MString == NULL))
    {
    return(FAILURE);
    }

  /* start with: displaying the header */

  MStringAppend(MString,"<");

  if (E->Name != NULL)
    {
    MStringAppend(MString,E->Name);
    }
  else
    {
    MStringAppend(MString,"NA");
    }

  /* display the element's attributes if any */

  if (E->ACount > 0)
    {
    mstring_t NewLine(MMAX_LINE);

    /* Init temp Mstring one for all attributes and that save init/destroy on each one */

    for (index = 0;index < E->ACount;index++)
      {
      __MXMLToMStringOneAttribute(MString,E->AName[index],E->AVal[index],&NewLine);
      }  /* END for (index) */
    }

  MStringAppend(MString,">");   /* add the trailing attribute bracket */


  /* If there is an Element Value, then output its contents into the MString */

  __MXMLValueToMString(MString, E->Val);

  /* display CData */

  if (E->CData != NULL)
    {
    for (index = 0;index < E->CDataCount;index++)
      {
      if (E->CData[index] == NULL)
        break;

      __MXMLCDataToMString(MString,E->CData[index]);
      }
    }

  /* recursively display children of this element */

  for (index = 0;index < E->CCount;index++)
    {
    if (E->C[index] == NULL)
      continue;

    if (MXMLToMString(E->C[index],MString,&tail,NoCompress) == FAILURE)
      {
      return(FAILURE);
      }
    }  /* END for (index) */


  /* display final footer */

  if (E->Name != NULL)
    {
    MStringAppend(MString,"</");
    MStringAppend(MString,E->Name);
    MStringAppend(MString,">");
    }
  else
    {
    MStringAppend(MString,"</NA>");
    }


  /* How big is the buffer to encode */

  len = MString->length();

  /* Return pointer to any trailing data stream */
  if (TailP != NULL)
    *TailP = MString->c_str() + len;

  /* Need a mutable string since mstring cannot be alter via c_str() method */

  char *mutableString = NULL;
  MUStrDup(&mutableString,MString->c_str());

  /* Check if compression is requested, data is too big, item is "Data" */

  int rc = __MXMLCompressDataAttribute(NoCompress,mutableString,len,(MMAX_BUFFER >> 1),E->Name,"Data");

  *MString = mutableString;   /* replace the mstring with the new compressed mutable string */

  MUFree(&mutableString);
 
  if (rc == FAILURE)
    return(FAILURE);

  return(SUCCESS);
  }  /* END MXMLToMString() */



/**
 * Get an attribute from an element according to a specific format
 *
 * @param E       (I)
 * @param AName   (I/O)
 * @param ATok    (I) [optional]
 * @param AVal    (O) [optional]
 * @param DFormat (I) object type pointed by *AVal
 * @param VSize   (I) AVal size, if of type string, in byte count
 */

int MXMLGetAttrF(

  mxml_t        *E,     
  const char    *AName, 
  int           *ATok,  
  void          *AVal, 
  enum MDataFormatEnum DFormat, 
  int            VSize)

  {
  char tmpLine[MMAX_LINE];
  int  rc;

  if (AVal == NULL)
    {
    return(FAILURE);
    }

  /* filtering switch statement */
  switch (DFormat)
    {
    case mdfString:

      rc = MXMLGetAttr(
        E,
        AName,
        ATok,
        (char *)AVal,
        VSize);

      return(rc);

      /* NOTREACHED*/

      break;

    case mdfInt:
    case mdfLong:
    case mdfDouble:

      if (MXMLGetAttr(
            E,
            AName,
            ATok,
            tmpLine, /* O */
            sizeof(tmpLine)) == FAILURE)
        {
        /* clear response */

        switch (DFormat)
          {
          case mdfInt:    *(int *)AVal    = 0; break;
          case mdfLong:   *(long *)AVal   = 0; break;
          case mdfDouble: *(double *)AVal = 0.0; break;
          default: break;
          }  /* END switch (DFormat) */

        return(FAILURE);
        }

      break;

    default:

      return(FAILURE);

      /*NOTREACHED*/

      break;
    }  /* END switch (DFormat) */

  /* Final switch statement to return data */
  switch (DFormat)
    {
    case mdfInt:

      *(int *)AVal = (int)strtol(tmpLine,NULL,10);

      break;

    case mdfLong:

      *(long *)AVal = strtol(tmpLine,NULL,10);

      break;

    case mdfDouble:

      *(double *)AVal = strtod(tmpLine,NULL);

      break;

    default:

      /* IMPOSSIBLE to EVER reach this code due to
       * 1st switch statement above

      *(char **)AVal = NULL;

      return(FAILURE);

      NOTREACHED 

      */
      break;
    }  /* END switch (DFormat) */

  return(SUCCESS);
  }  /* END MXMLGetAttrF() */






/**
 * String Duplicates the <value> of the <attr>=<value> XML pair into pointer AVal.
 *
 * Iterator capable to allow walking the entire list
 *
 * NOTE: Returned AVal must be freed after use
 * 
 * @param E       (I)
 * @param AName   (I/O) [case sensitive, if empty, will be populated with name of next attr]
 * @param ATok    (I) [optional: iterator -1 to start] 
 * @param AVal    (O) [optional,minsize=VSize]
 */

int MXMLDupAttr(

  mxml_t *E,     
  char   *AName, 
  int    *ATok,  
  char  **AVal)  

  {
  /* NOTE:  set AName to empty string to get Name */

  int aindex;
  int astart;

  if (AVal != NULL)
    AVal[0] = '\0';

  if (E == NULL)
    {
    return(FAILURE);
    }

  if (ATok != NULL)
    astart = *ATok;
  else
    astart = -1;

  for (aindex = astart + 1;aindex < E->ACount;aindex++)
    {
    if ((AName == NULL) || 
        (AName[0] == '\0') || 
        !strcmp(AName,E->AName[aindex]))
      {
      /* If AName is an empty string, then initialize it with current entry */
      if ((AName != NULL) && (AName[0] == '\0'))
        {
        strncpy(AName,E->AName[aindex],MMAX_NAME);
        AName[MMAX_NAME - 1] = '\0';
        }

      /* return value to caller */
      if ((AVal != NULL) && (E->AVal[aindex] != NULL))
        {
        if (MUStrDup(AVal,E->AVal[aindex]) == FAILURE)
          {
          return(FAILURE);
          }
        }

      if (ATok != NULL)
        *ATok = aindex;

      return(SUCCESS);
      }  /* END if ((AName == NULL) || ...) */
    }    /* END for (aindex) */

  return(FAILURE);
  }  /* END MXMLDupAttr() */



/**
 * Returns attribute names/values for XML elements.
 *
 * @param E       (I) The element to extract attributes from.
 * @param AName   (O) [case sensitive will be populated with name of next attr]
 * @param ATok    (I) [optional] Used to make iterating through an element multiple times efficient.
 * @param AVal    (O) [minsize=VSize] The buffer to save the value of the attribute in.
 * @param VSize   (I) The size of the value buffer (AVal) being passed in.
 */

int MXMLGetAnyAttr(

  mxml_t     *E,     
  char       *AName, 
  int        *ATok, 
  char       *AVal,
  int         VSize)

  {
  /* NOTE:  set AName to empty string to get Name */

  int aindex;
  int astart;

  int EVSize;

  if (AVal != NULL)
    AVal[0] = '\0';

  if ((E == NULL) || (AName == NULL) || (AVal == NULL))
    {
    return(FAILURE);
    }

  EVSize = (VSize > 0) ? VSize : MMAX_LINE;

  if (ATok != NULL)
    astart = *ATok;
  else
    astart = -1;

  for (aindex = astart + 1;aindex < E->ACount;aindex++)
    {
    strncpy(AName,E->AName[aindex],MMAX_NAME);

    AName[MMAX_NAME - 1] = '\0';

    strncpy(AVal,E->AVal[aindex],EVSize);

    AVal[EVSize - 1] = '\0';

    if (ATok != NULL)
      *ATok = aindex;

    return(SUCCESS);
    }    /* END for (aindex) */

  return(FAILURE);
  }  /* END MXMLGetAttr() */




/**
 * Returns attribute names/values for XML elements.
 *
 * @param E       (I) The element to extract attributes from.
 * @param AName   (I/O) [case sensitive, if empty, will be populated with name of next attr]
 * @param ATok    (I) [optional] Used to make iterating through an element multiple times efficient.
 * @param AVal    (O) [optional,minsize=VSize] The buffer to save the value of the attribute in.
 * @param VSize   (I) The size of the value buffer (AVal) being passed in.
 */

int MXMLGetAttr(

  mxml_t      *E,     
  const char  *AName, 
  int         *ATok, 
  char        *AVal,
  int          VSize)

  {
  /* NOTE:  set AName to empty string to get Name */

  int aindex;
  int astart;

  int EVSize;

  if (AVal != NULL)
    AVal[0] = '\0';

  if (E == NULL)
    {
    return(FAILURE);
    }

  EVSize = (VSize > 0) ? VSize : MMAX_LINE;

  if (ATok != NULL)
    astart = *ATok;
  else
    astart = -1;

  for (aindex = astart + 1;aindex < E->ACount;aindex++)
    {
    if (!strcmp(AName,E->AName[aindex]))
      {
      if ((AVal != NULL) && (E->AVal[aindex] != NULL))
        {
        strncpy(AVal,E->AVal[aindex],EVSize);
        AVal[EVSize - 1] = '\0';
        }

      if (ATok != NULL)
        *ATok = aindex;

      return(SUCCESS);
      }
    }    /* END for (aindex) */

  return(FAILURE);
  }  /* END MXMLGetAttr() */




/**
 * Return the requested xml attribute if it exists, in a MString object
 * 
 * Same thing as MXMLGetAttr but uses an mstring_t instead.
 *
 * @param E     (I)
 * @param AName (I/O) [case sensitive, if empty, will be populated with name of next attr]
 * @param ATok  (I) [optional]
 * @param AVal  (O) Expected to be initialzied beforehand.[optional]
 */

int MXMLGetAttrMString(

  mxml_t          *E,     
  const char      *AName,
  int             *ATok,
  mstring_t       *AVal)

  {
  /* NOTE:  set AName to empty string to get Name */

  int aindex;
  int astart;

  if (E == NULL)
    {
    return(FAILURE);
    }

  if (ATok != NULL)
    astart = *ATok;
  else
    astart = -1;

  for (aindex = astart + 1;aindex < E->ACount;aindex++)
    {
    if (!strcmp(AName,E->AName[aindex]))
      {
      if ((AVal != NULL) && (E->AVal[aindex] != NULL))
        {
        MStringSet(AVal,E->AVal[aindex]);
        }

      if (ATok != NULL)
        *ATok = aindex;

      return(SUCCESS);
      }
    }    /* END for (aindex) */

  return(FAILURE);
  }  /* END MXMLGetAttrMString() */





/**
 * In a given element: look for and return a child of the element
 *
 * @param E     (I)
 * @param CName (I) [optional]
 * @param CTok  (I) [optional]
 * @param CP    (O) [optional]
 */

int MXMLGetChild(

  mxml_t const *E,
  char const *CName,
  int     *CTok,  
  mxml_t **CP)   

  {
  int cindex;
  int cstart;

  if (CP != NULL)
    *CP = NULL;

  if (E == NULL)
    {
    return(FAILURE);
    }

  /* Determine start of this iteration */
  if (CTok != NULL)
    cstart = *CTok;
  else
    cstart = -1;

  /* Startthe search */
  for (cindex = cstart + 1;cindex < E->CCount;cindex++)
    {
    if (E->C[cindex] == NULL)
      continue;

    if ((CName == NULL) || !strcmp(CName,E->C[cindex]->Name))
      {
      if (CP != NULL)
        *CP = E->C[cindex];

      if (CTok != NULL)
        *CTok = cindex;

      return(SUCCESS);
      }
    }    /* END for (cindex) */

  return(FAILURE);
  }  /* END MXMLGetChild() */




/**
 * In a given element: look for and return a child of the element, but case insensitive
 *
 * @param E       (I)
 * @param CName   (I) [optional]
 * @param CTok    (I) [optional]
 * @param CP      (O) [optional]
 */

int MXMLGetChildCI(

  mxml_t        *E,     
  const char    *CName,
  int           *CTok,
  mxml_t       **CP) 

  {
  int cindex;
  int cstart;

  int SLen;

  if (CP != NULL)
    *CP = NULL;

#ifndef __MOPT
  if (E == NULL) 
    {
    return(FAILURE);
    }
#endif /* __MOPT */

  if (CTok != NULL)
    cstart = *CTok;
  else
    cstart = -1;

  if (CName != NULL)
    SLen = strlen(CName) + 1;
  else
    SLen = 0;

  for (cindex = cstart + 1;cindex < E->CCount;cindex++)
    {
    if (E->C[cindex] == NULL)
      continue;

    if ((CName == NULL) || !strncasecmp(CName,E->C[cindex]->Name,SLen))
      {
      if (CP != NULL)
        *CP = E->C[cindex];

      if (CTok != NULL)
        *CTok = cindex;

      return(SUCCESS);
      }
    }    /* END for (cindex) */

  return(FAILURE);
  }  /* END MXMLGetChildCI() */


/**
 * Function to set the Error Msg buffer and cleanup the element
 *
 * @param ErrorMessage  (I)    [error message to store into EMsg if present]
 * @param EP            (I)    [pointer to a mxml_t node * to be freed]
 * @param EMsg          (O)    [optional:  buffer pointer to put an error message]
 */

int __MXMLParseStringErrorCleanup(
    
  const char *ErrorMessage,
  mxml_t    **EP,
  char       *EMsg)

  {
  if ((EMsg != NULL))
    strcpy(EMsg,ErrorMessage);

  return(MXMLDestroyE(EP));
  }



/**
 * Extract the root name of the element in the input stream 
 *
 * @param  EP         (I)]    XML Element node to add to EP
 * @param  ptrP       (I)]    pointer to current char * pointer into input stream
 * @param  DoAppend   (I/O)   ptr to return mbool_t to indicate whether append or not
 * @param  EMsg       (O)]    [optional Error message buffer]
 */

int __MXML_ExtractRootElement(

  mxml_t  **EP,
  char    **ptrP,
  mbool_t  *DoAppend,
  char     *EMsg)

  {
  int         index;
  char       *ptr;
  char        tmpNLine[MMAX_LINE + 1]; /* this needs to be dynamic to avoid errors */
  mbool_t     ElementIsClosed = FALSE;

  ptr = *ptrP;

  /* Ready to begin parsing:   extract root element token */

  /* FORMAT:  '<name>...' */

  /* remove whitespace */

  while (isspace(*ptr))
    ptr++;

  if (*ptr != '<')
    {
    /* cannot located start of element */
    __MXMLParseStringErrorCleanup("cannot locate start of root element",EP,EMsg);
    return(FAILURE);
    }

  ptr++;  /* skip the '<' of the root element */

  /* harvest the element's name tag */

  index = 0;

  while ((*ptr != ' ') && (*ptr != '>'))
    {
    if ((ptr[0] == '/') && (ptr[1] == '>'))
      {
      ElementIsClosed = TRUE;

      break;
      }

    tmpNLine[index++] = *(ptr++);

    if ((index >= MMAX_LINE) || (*ptr == '\0'))
      {
      char   tmpEMsg[MMAX_LINE] = "";

      tmpNLine[MIN(index,MMAX_LINE - 1)] = '\0';

      if (index >= MMAX_LINE)
        {
        sprintf(tmpEMsg,"element name is too long - '%.12s...'",
          tmpNLine);
        }
      else
        {
        sprintf(tmpEMsg,"element is malformed - no terminator");
        }

      __MXMLParseStringErrorCleanup(tmpEMsg,EP,EMsg);
      return(FAILURE);
      }
    }    /* END while ((*ptr != ' ') && (*ptr != '>')) */

  tmpNLine[index] = '\0';

  if (tmpNLine[0] == '\0')
    {
    __MXMLParseStringErrorCleanup("element name is empty",EP,EMsg);
    return(FAILURE);
    }

  /* Now check if we need to create the XML elment container */

  if ((*EP == NULL) && (MXMLCreateE(EP,tmpNLine) == FAILURE))
    {
    char   tmpEMsg[MMAX_LINE] = "";

    sprintf(tmpEMsg,"cannot create XML element '%.32s'",
        tmpNLine);

    __MXMLParseStringErrorCleanup(tmpEMsg,EP,EMsg);
    return(FAILURE);
    }

  if (((*EP)->ACount > 0) || ((*EP)->CCount > 0))
    {
    *DoAppend = TRUE;
    }

  if (ElementIsClosed == TRUE)
    {
    ptr += 2; /* skip '/>' */

    *ptrP = ptr;
    return(SUCCESS);
    }

  *ptrP = ptr;

  return(CONTINUE);
  }



/**
 * Parse input string ('ptr') for meta elements and ignore them
 *
 * What is known at call time:
 *    'ptr' is pointing at the Leading '<'
 *    'ptr[1]' is NOT a trailing '/' char
 *
 *
 * @param  EP      [I]  XML Element node to add to EP
 * @param  BufP    [I]  pointer to current char * pointer into input stream
 * @param  EMsg    [O]  [optional Error message buffer]
 */

int __MXMLIgnoreMetaElements(

  mxml_t **EP,     
  char   **BufP,
  char    *EMsg)

  {
  char *ptr = *BufP;

  while ((ptr[1] == '?') || (ptr[1] == '!'))
    {
    ptr++;

    /* ignore 'meta' elements */

    if (*ptr == '?')
      {
      ptr++;

      if ((ptr = strstr(ptr,"?>")) == NULL)
        {
        /* cannot locate end of meta element */

        return(FAILURE);
        }

      if ((ptr = strchr(ptr,'<')) == NULL)
        {
        /* cannot locate next element */
        __MXMLParseStringErrorCleanup("cannot locate post-meta XML",EP,EMsg);
        return(FAILURE);
        }
      }    /* END if (*ptr == '?') */

    /* ignore 'comment' element */

    if (!strncmp(ptr,"!--",3))
      {
      ptr += 3;

      if ((ptr = strstr(ptr,"-->")) == NULL)
        {
        /* cannot locate end of comment element */
        __MXMLParseStringErrorCleanup("cannot locate comment termination marker",EP,EMsg);
        return(FAILURE);
        }

      if ((ptr = strchr(ptr,'<')) == NULL)
        {
        /* cannot locate next element */
        __MXMLParseStringErrorCleanup("cannot locate post-comment XML",EP,EMsg);
        return(FAILURE);
        }
      }    /* END if (!strncmp(ptr,"!--",3)) */
    else if (ptr[1] == '!')
      {
      const char* ptr2;

      ptr++;
      ptr++;

      /* walk the string until End-Of-String looking for a XML token */
      while (*ptr != '\0')
        {
        /* If we found a token, then break */
        ptr2 = strchr("<[>",*ptr);
        if (ptr2 != NULL)
          {
          break;
          }

        ptr++;
        }

      /* Possible at EOS or at next XML token:
       *   If '*ptr' is a NULL byte, then
       *   strchr WILL return a pointer to the NULL byte - NOT a NULL ptr
       */
      if ('\0' == *ptr)
        {
        /* cannot locate end element */
        __MXMLParseStringErrorCleanup("cannot locate post-comment XML",EP,EMsg);
        return(FAILURE);
        }

      /* *ptr is NOT NULL byte, so safe to use it for a char to search for */
      ptr2 = strchr((char *)"<[>",*ptr);

      switch (*ptr2)
        {
        case '[':
 
          /* check for valid end-token, if found continue on */
          ptr = strstr(ptr,"]>");

          break;

        default:

          /* NYI */

          if (*EP != NULL)
            MXMLDestroyE(EP);

          return(FAILURE);

          /* NOTREACHED */

          break;
        }

      if ((NULL == ptr) || ((ptr = strchr(ptr,'<')) == NULL))
        {
        /* cannot locate next element */
        __MXMLParseStringErrorCleanup("cannot locate post-comment XML",EP,EMsg);
        return(FAILURE);
        }
      }    /* END if (*ptr == '!') */
    }      /* END while ((ptr[1] == '?') || (ptr[1] == '!')) */

  /* return updated pointer */
  *BufP = ptr;

  return(SUCCESS);
  } /* END  __MXMLIgnoreMetaElements() */



/**
 * Compressed data was detected in the stream, 
 * so uncompress that section
 *
 * @param  E       [I]  XML Element node to add to EP
 * @param  ptrP    [I]  pointer to current char * pointer into input stream
 * @param  EMsg    [O]  [optional Error message buffer]
 */

int __MXML_DecompressData(

  mxml_t      *E,
  char       **ptrP,
  char        *EMsg)

  {
  char *ptr;

  char *tail;
  int   len;

  mxml_t *CE;

  char *tmpBuf;

  ptr = *ptrP;

  /* compressed data detected, '<' symbol guaranteed to not be part of string */

  tail = strchr(ptr,'<');
 
  /* determine size of value */

  len = tail - ptr;

  /* uncompress data */

  tmpBuf = NULL;

  /* NOTE:  CRYPTHEAD header indicates encryption/compression */

  if (MSecDecompress(
        (unsigned char *)ptr,
        (unsigned int)len,
        NULL,
        (unsigned int)0,
        (unsigned char **)&tmpBuf,
        NULL) == FAILURE)
        {
        if ((EMsg != NULL) && (EMsg[0] == '\0'))
          strcpy(EMsg,"cannot decompress value");

        return(FAILURE);
        }

  /* process expanded buffer (guaranteed to be a single element */

  CE = NULL;

  if ((MXMLFromString(&CE,tmpBuf,NULL,EMsg) == FAILURE) ||
      (MXMLAddE(E,CE) == FAILURE))
    {
    if ((EMsg != NULL) && (EMsg[0] == '\0'))
      strcpy(EMsg,"cannot add child element");
    return(FAILURE);
    }

  tmpBuf = NULL;

  /* move pointer to end of compressed data, and update caller's pointer */

  *ptrP = tail;

  return(SUCCESS);
  } /* END __MXML_DecompressData() */


/**
 * Extract the attributes for this element
 *
 * @param  EP             (I/O) pointer to ptr to xml element to populate
 * @param  XMLStringStart (I)   start of original XML string buffer
 * @param  ptrP           (I)   ptr to buffer pointer of current char being processed
 * @param  EMsg           (O)   [optional error buffer]
 *
 * Return:
 *    SUCCESS  consumption of attributes is complete AND the caller can return to its caller
 *    FAILURE  error
 *    CONTINUE caller can continue normally after calling this function
 */

int __MXML_ExtractNodeAttributes(
    
  mxml_t      **EP,
  const char   *XMLStringStart,
  char        **ptrP,
  char         *EMsg)

  {
  int         index;
  mstring_t   tmpVLine(MMAX_LINE);
  char        tmpNLine[MMAX_LINE + 1]; /* this needs to be dynamic to avoid errors */
  char       *ptr;

  ptr = *ptrP;

  /* Init the mstring that we will use for each attribute=value pair */

  /* remove whitespace */

  while (isspace(*ptr))
    ptr++;

  /* Scan the input link until ending '>' or null byte */

  while (*ptr != '>')
    {
    /* extract attributes */

    /* FORMAT:  <ATTR>="<VAL>" ... */

    index = 0;

    /* Harvest the attribute or null byte */

    while ((*ptr != '=') && (*ptr != '\0'))
      {
      tmpNLine[index++] = *(ptr++);

      if (index >= MMAX_LINE)
         break;
      }

    tmpNLine[index] = '\0';   /* NULL terminate the string */

    if (*ptr != '\0')
      ptr++;  /* skip '=' */

    if (*ptr != '\0')
      ptr++;  /* skip '"' */

    /* Check for end of string */

    if (*ptr == '\0')
      {
      __MXMLParseStringErrorCleanup("string is corrupt - early termination",EP,EMsg);
      return(FAILURE);
      }

    /* Reset the tmp string container for the next attribute */
    tmpVLine.clear();

    /* now harvest the "value" for the attribute, iterate til closing quote OR null byte */

    while (((*ptr != '"') && (*ptr != '\0')) ||
          ((ptr > XMLStringStart) && (*(ptr - 1) == '\\')))
      {
      char  mappedChar[2];

      /* Check if base set of xml encodings are encountered */
      
      if (__MXMLDecodeEscapeForBaseSet(&ptr,mappedChar) == SUCCESS)
        {
        MStringAppend(&tmpVLine,mappedChar);
        }
      else if (__MXMLDecodeEscapeForQuote(&ptr,mappedChar) == SUCCESS)
        {
        MStringAppend(&tmpVLine,mappedChar);
        }
      else if (__MXMLDecodeEscapeForENVRS(&ptr,mappedChar) == SUCCESS)
        {
        MStringAppend(&tmpVLine,mappedChar);
        }
      else
        {
        /* no encoding, just copy the plain character itself */

        MStringAppendF(&tmpVLine,"%c",*(ptr++));
        }
      }    /* END while ((*ptr != '"') || ...) */

    /* we have now constructed a single 'attribute="value"' set, 
     * so set it to the parent xml node
     */
    MXMLSetAttr(*EP,tmpNLine,(void *)tmpVLine.c_str(),mdfString);

    /* we got here because *ptr is a null byte OR the matching '"'.
     * If a null byte, things are bad
     */
    if (*ptr == '\0')
      {
      __MXMLParseStringErrorCleanup("string is corrupt - early termination",EP,EMsg);
      return(FAILURE);
      }

    ptr++; /* ignore the '"' */

    while (*ptr == ' ')    /* skip white space */
      ptr++;

    /* check for element terminator */

    if ((ptr[0] == '/') && (ptr[1] == '>'))
      {
      /* element terminator reached */

      ptr += 2; /* skip '/>' */

      *ptrP = ptr;

      return(SUCCESS);
      }
    }  /* END while (*ptr != '>') */

  ptr++;                    /* Skip the '>' */

  *ptrP = ptr;              /* return the current pointer back to the caller */

  return(CONTINUE);  /* Tell caller to NOT return to its caller, but to continue processing */
  } /* END __MXML_ExtractNodeAttributes */


/**
 * Extract Values from the input string associated with current node 
 *
 * @param  EP             (I/O) pointer to ptr to xml element to populate
 * @param  ptrP           (I)   ptr to buffer pointer of current char being processed
 * @param  EMsg           (O)   [optional error buffer]
 */

int __MXML_ExtractNodeValue(
    
  mxml_t  **EP,
  char    **ptrP,
  char     *EMsg)

  {
  char        *ptr;

  ptr = *ptrP;

  /* extract the optional element 'value' */

  /* Ex:   'Value    <' */

  /* skip whitespace */

  while (isspace(*ptr))
    ptr++;

  /* When we reached the next element's start angle bracket, we are done with value field */

  if (*ptr != '<')
    {
    mstring_t    tmpVLine(MMAX_LINE);

    /* Check for optional Compress action needed */

    if ((MSysUICompressionIsDisabled() != TRUE) && 
        (!strncmp(ptr,CRYPTHEAD,strlen(CRYPTHEAD))))
      {
      char tmpEMsg[MMAX_LINE];
      tmpEMsg[0] = '\0';

      /* Go perform the uncompression on the current data stream */

      if (__MXML_DecompressData(*EP,&ptr,tmpEMsg) == FAILURE)
        {
        __MXMLParseStringErrorCleanup(tmpEMsg,EP,EMsg);

        return(FAILURE);
        } /* END __MXML_DecompressData(EP,*EP,&ptr,EMsg) == FAILURE */
      }  /* END if (!strncmp(ptr,CRYPTHEAD,strlen(CRYPTHEAD))) */

    /* read element value */

    tmpVLine.clear();  /* reset string */

    /* Scan the input buffer and process */

    while ((*ptr != '\0') && (*ptr != '<'))
      {
      char  mappedChar[2];

      /* Check and decode any base set encodings */

      if (__MXMLDecodeEscapeForBaseSet(&ptr,mappedChar) == SUCCESS)
        {
        MStringAppend(&tmpVLine,mappedChar);
        }
      else if (__MXMLDecodeEscapeForENVRS(&ptr,mappedChar) == SUCCESS)
        {
        MStringAppend(&tmpVLine,mappedChar);
        }
      /* Not required to parse (quote) '"' per XML spec in node value */
      else
        {
        /* copy the char across w/o mod */

        MStringAppendF(&tmpVLine,"%c",*(ptr++));
        }
      }  /* END while (*ptr != '<') */

    if (strchr(tmpVLine.c_str(),'&'))
      {
      /* convert value to only contain XML-friendly characters */

      __MXMLStringConvertFromAmp(tmpVLine.c_str());
      }  /* END if (strchr(tmpVLine.c_str(),'&')) */

    if (MUStrDup(&(*EP)->Val,tmpVLine.c_str()) == FAILURE)
      {
      __MXMLParseStringErrorCleanup("ERROR: internal error occurred in memory allocation",EP,EMsg);

      return(FAILURE);
      }

    /* map any 'shift out' characters back to a '<' symbol */

    if ((*EP)->Val != NULL)
      {
      char *ptr2;

      for (ptr2 = strchr((*EP)->Val,(char)XML_VALUE_ANGLE_BRACKET_INT);
           ptr2 != NULL;
           ptr2 = strchr(ptr2,(char)XML_VALUE_ANGLE_BRACKET_INT))
        *ptr2 = '<';
      }
    }  /* END if (*ptr != '<') */

  *ptrP = ptr;

  return(CONTINUE);
  } /* END __MXML_ExtractNodeValue() */


/**
 * extract children from the input string and form in memory element node
 *
 * @param  EP        (I/O) pointer to ptr to xml element to populate
 * @param  ptrP      (I)   ptr to buffer pointer of current char being processed
 * @param  Tail     
 * @param  DoAppend  
 * @param  EMsg      (O)   [optional error buffer]
 */

int __MXML_ExtractChildren(

  mxml_t  **EP,
  char    **ptrP,
  char    **Tail,     
  mbool_t   DoAppend,
  char     *EMsg)

  {
  char    *ptr;
  char    *tail;

  ptr = *ptrP;

  mstring_t    tmpVLine(MMAX_LINE);

  while (ptr[1] != '/')
    {
    mxml_t *CE;

    CE = NULL;

    if (DoAppend == TRUE)
      {
      char *ptr2;
      char  tmpCName[MMAX_LINE];

      int   index;

      /* FORMAT:  <NAME>... */

      /* locate name */

      ptr2 = ptr + 1;  /* ignore '<' */

      index = 0;

      while ((*ptr2 != ' ') && (*ptr2 != '>'))
        {
        if ((ptr2[0] == '/') && (ptr2[1] == '>'))
          {
          break;
          }

        tmpCName[index++] = *(ptr2++);

        if ((index >= MMAX_LINE) || (*ptr2 == '\0'))
          {
          char tmpEMsg[MMAX_LINE];

          tmpCName[MIN(index,MMAX_LINE - 1)] = '\0';

          if (index >= MMAX_LINE)
            {
            sprintf(tmpEMsg,"element name is too long - '%.12s...'",
              tmpCName);
            }
          else
            {
            sprintf(tmpEMsg,"element is malformed - no terminator");
            }

          __MXMLParseStringErrorCleanup(tmpEMsg,EP,EMsg);

          return(FAILURE);
          }
        }

      tmpCName[index] = '\0';

      /* NOTE: Duplicate elements get overwritten - sorry, W3C */
      MXMLGetChild(*EP,tmpCName,NULL,&CE);
      }  /* END if (DoAppend == TRUE) */

    /* Call recursively to parse the entire child and then add that new
     * child to our current element node. This is how we build the
     * tree of inmemory linked nodes
     */
    if ((MXMLFromString(&CE,ptr,&tail,EMsg) == FAILURE) ||
        (MXMLAddE(*EP,CE) == FAILURE))
      {
      break;
      }

    ptr = tail;

    if ((ptr == NULL) || (ptr[0] == '\0'))
      {
      /* XML is corrupt */

      if (Tail != NULL)
        *Tail = ptr;

      __MXMLParseStringErrorCleanup("cannot extract child",EP,EMsg);

      return(FAILURE);
      }
    }  /* END while (ptr[1] != '/') */

  /* ignore whitespace */

  while (isspace(*ptr))
    ptr++;

  /* value may follow children */

  tmpVLine.clear();  /* Reset string */

  if ((*EP)->Val == NULL)
    {
    /* NOTE:  application of CRYPTHEAD marker is side-affect of compression
              algorithm - THIS IS DANGEROUS as it is only 4 characters long
              and may be randomly generated within client signature values 
    */

    if ((MSysUICompressionIsDisabled() != TRUE) &&
       (!strncmp(ptr,CRYPTHEAD,strlen(CRYPTHEAD))))
      {
      char tmpEMsg[MMAX_LINE] = "\0";

      if (__MXML_DecompressData(*EP,&ptr,tmpEMsg) == FAILURE)
        {
        __MXMLParseStringErrorCleanup(tmpEMsg,EP,EMsg);

        return(FAILURE);
        } /* END __MXML_DecompressData(*EP,&ptr,EMsg) == FAILURE */
      } /* END !strncmp(ptr,CRYPTHEAD,strlen(CRYPTHEAD)) */


    tmpVLine.clear();  /* Reset string */

    while (*ptr != '<')
      {
      MStringAppendF(&tmpVLine,"%c",*(ptr++));

      if (*ptr == '\0')
        {
        char tmpEMsg[MMAX_LINE] = "\0";

        sprintf(tmpEMsg,"cannot load value line - %.10s (corrupt)",
            tmpVLine.c_str());

        __MXMLParseStringErrorCleanup(tmpEMsg,EP,EMsg);

        return(FAILURE);
        }

      if ((*ptr == '/') && (ptr[1] == '>'))
        break;
      }  /* END while (*ptr != '<') */

    if (!tmpVLine.empty())
      {
      if (MUStrDup(&(*EP)->Val,tmpVLine.c_str()) == FAILURE)
        {
        __MXMLParseStringErrorCleanup("no memory for value",EP,EMsg);

        return(FAILURE);
        }
      }
    }  /* END if (E->Val == NULL) */

  *ptrP = ptr;

  return(CONTINUE);
  }


/**
 * Parse the XML tail from the input stream:
 *     the 'ending and matching tag for a given element'
 *     like:
 *         '/Tag>'
 *
 * @param  EP             (I/O) pointer to ptr to xml element to populate
 * @param  ptrP           (I)   ptr to buffer pointer of current char being processed
 * @param  Tail           (I/O) returned pointer to rest of input stream 
 * @param  EMsg           (O)   [optional error buffer]
 */

int __MXML_ParseXMLTail(

  mxml_t  **EP,
  char    **ptrP,
  char    **Tail,     
  char     *EMsg)

  {
  char *ptr;

  ptr = *ptrP;

  /*  FORMAT:  '/Tag>'  */

  if (*ptr == '/')
    {
    /* process '/>' */

    ptr++; /* ignore '/' */
    }
  else if (*ptr != '\0')
    {
    /* We should encounter the end matching tag of the element */

    /* NOTE: corrupt XML string may move ptr beyond string terminator */

    ptr++; /* ignore '<' */

    /* What is this?? ASSUMPTIONS! */

    ptr++; /* ignore '/' of '/Tag>'*/

    /* If we have a tag name already saved, bump the ptr
     * past the ASSUMED ending matching tag
     */
    if ((*EP)->Name != NULL)
      ptr += strlen((*EP)->Name);
    }

  /* Next character 'should' be a '>' byte, enforce that here */

  if (*ptr == '\0')
    {
    __MXMLParseStringErrorCleanup("xml tail is corrupt",EP,EMsg);
    return(FAILURE);
    }

  ptr++; /* ASSUMPTION: skip over last '>' */

  if (Tail != NULL)
    *Tail = ptr;

  *ptrP = ptr;

  return(CONTINUE);
  }



/**
 * Parse XML input and generates a mxml_t tree structure
 *   (Takes a XML endocded string and returns it in XML structure tree)
 *
 * @param EP        (O) [populate or create - will be freed on failure]
 * @param XMLString (I)
 * @param Tail      (O) [optional] returned pointer to rest of input stream 
 * @param EMsg      (O) [optional,minsize=MMAX_LINE]
 */

int MXMLFromString(

  mxml_t       **EP,     
  const char    *XMLString, 
  char         **Tail,     
  char          *EMsg)    

  {
  int      rc;

  char    *ptr;

  mbool_t  DoAppend = FALSE;

  /* NOTE:  only support up to two distinct threads */                         

  /* NOTE:  do not initialize EP, it may already be set */

  if (EMsg != NULL)
    EMsg[0] = '\0';

  /* Check for 'bad' parameter */

  if ((XMLString == NULL) || (EP == NULL))
    {
    __MXMLParseStringErrorCleanup("invalid arguments",EP,EMsg);
    return(FAILURE);
    }

  /* Check for the presence of the less-than token in the string */

  if ((ptr = (char *)strchr(XMLString,'<')) == NULL)
    {
    __MXMLParseStringErrorCleanup("no XML in string",EP,EMsg);
    return(FAILURE);
    }

  /* Check for tail marker */

  if (ptr[1] == '/')
    {
    /* located tail marker */
    __MXMLParseStringErrorCleanup("premature termination marker",EP,EMsg);
    return(FAILURE);
    }

  /* NOTE:  should support append/overlay parameter (NYI) */

  /* ignore 'meta' elements */

  rc = __MXMLIgnoreMetaElements(EP,&ptr,EMsg);
  if (FAILURE == rc)
    return(FAILURE);

  /* Extract the element's root info */

  rc = __MXML_ExtractRootElement(EP,&ptr,&DoAppend,EMsg);
  if (CONTINUE != rc)
    {
    if ((rc == SUCCESS) && (Tail != NULL))
      *Tail = ptr;

    return(rc);
    }

  /* Now Extract attributes for this element */

  /* __MXML_ExtractNodeAttributes function can return status to proceed in 1 of 3 paths:
   *    1) Return NOW with FAILURE         Problem with incoming syntax
   *    2) Return NOW with SUCCESS         Syntax good, string has been processed
   *    3) This function should CONTINUE on:
   *       which means do not return here, but proceed with further parsing
   */

  rc = __MXML_ExtractNodeAttributes(EP,XMLString,&ptr,EMsg);
  if (CONTINUE != rc)
    {
    if (Tail != NULL)
      *Tail = ptr;

    return(rc);
    }

  /* Now extract node value
   * NOTE:  value can occur before, after, or spread amongst children
   */
  rc = __MXML_ExtractNodeValue(EP,&ptr,EMsg);
  if (CONTINUE != rc)
    {
    return(rc);
    }

  /* Now extract any children in the input stream for the current element */

  rc = __MXML_ExtractChildren(EP,&ptr,Tail,DoAppend,EMsg);
  if (CONTINUE != rc)
    {
    return(rc);
    }

  /* process the XML tail of the input stream */

  rc = __MXML_ParseXMLTail(EP,&ptr,Tail,EMsg);
  if (CONTINUE != rc)
    {
    return(rc);
    }

  return(SUCCESS);
  }  /* END MXMLFromString() */



/**
 * Duplicate an xml structure tree.
 *
 * Return pointer to new xml structure tree
 *
 * NOTE: does NOT (and SHOULD NOT) duplicate XD 
 *
 * @param SrcE  (I)
 * @param DstEP (O)
 */

int MXMLDupE(

  mxml_t  *SrcE, 
  mxml_t **DstEP)

  {
  int       rc = SUCCESS;

  if (DstEP == NULL)
    {
    return(FAILURE);
    }

  *DstEP = NULL;

  if (SrcE == NULL)
    {
    return(FAILURE);
    }
 
  /* Create a MString */
  mstring_t Buffer(MMAX_LINE);

  /* Convert model to a MString */
  if (MXMLToMString(SrcE,&Buffer,(char const **) NULL,TRUE) == SUCCESS)
    {
    /* Parse the MString into the NEW model tree */
    if (MXMLFromString(DstEP,Buffer.c_str(),NULL,NULL) == FAILURE)
      {
      rc = FAILURE;
      }
    }
  else
    {
    rc = FAILURE;
    }

  return(rc);
  }  /* END MXMLDupE() */




/* recursively locate node in XML tree */

/* NOTE: should be extended to support node 'type' concept and match only if 
         name and type match (NYI) */

/**
 * Find an element by name and optionally by type
 * return a pointer to the element
 *
 * Recursive
 *
 * @param RootE   (I)
 * @param Name    (I)
 * @param Type    (I) [optional,-1 = not set]
 * @param EP      (O) [optional]
 */

int MXMLFind(

  mxml_t  *RootE,  
  char    *Name,  
  int      Type, 
  mxml_t **EP)  

  {
  int cindex;

  if (EP != NULL)
    *EP = NULL;

  /* Check parameters */
  if ((RootE == NULL) || (Name == NULL) || (Name[0] == '\0'))
    {
    return(FAILURE);
    }

  /* Check for match: selected type (or don't care) and then by string match */
  if ((Type == -1) || (Type == RootE->Type))
    {
    if ((RootE->Name != NULL) && !strcasecmp(Name,RootE->Name))
      {
      if (EP != NULL)
        *EP = RootE;

      return(SUCCESS);
      }
    }

  /* recursively search all children of this element */
  for (cindex = 0;cindex < RootE->CCount;cindex++)
    {
    if (MXMLFind(RootE->C[cindex],Name,Type,EP) == SUCCESS)
      {
      return(SUCCESS);
      }
    }  /* END for (cindex) */

  /* cannot locate requested node in this XML tree */

  return(FAILURE);
  }  /* END MXMLFind() */




#if 0

Code not used in moab AND it has bugs in that it is difficult
to write a correct unittest for it

Keep, but define it out  12 Feb 2012  dougbert


/*
 * Duplicates the XML structure without first converting to a string
 * NOTE: void *XD is NOT copied
 *  
 * @param SrcE   (I) Source
 * @param DestEP (O) Output destination pointer [alloc]
 *
 */
 
int MXMLDupENoString(

  mxml_t  *SrcE,  
  mxml_t **DstE) 

  {
  int tmpIndex;
  mxml_t *DstEP;

  if (DstE == NULL)
    {
    return(FAILURE);
    }

  if (SrcE == NULL)
    {
    return(FAILURE);
    }

  DstEP = (mxml_t *)MUCalloc(1,sizeof(mxml_t));

  /* Manually copy everything over */
  
  /* Name */
  DstEP->Name = NULL;
  if (SrcE->Name != NULL)
    {
    if (MUStrDup(&DstEP->Name,SrcE->Name) == FAILURE)
      {
      MUFree(&DstEP->Name);
      MUFree((char **)&DstEP);

      return(FAILURE);
      }
    }

  /* Val */
  DstEP->Val = NULL;
  if (SrcE->Val != NULL)
    {
    if (MUStrDup(&DstEP->Val,SrcE->Val) == FAILURE)
      {
      MUFree(&DstEP->Name);
      MUFree(&DstEP->Val);
      MUFree((char **)&DstEP);

      return(FAILURE);
      }
    }

  /* ACount, ASize */
  DstEP->ACount = SrcE->ACount;
  DstEP->ASize = SrcE->ASize;

  /* CCount, CSize */ 
  DstEP->CCount = SrcE->CCount;
  DstEP->CSize = SrcE->CSize;
  
  /* Attribute names */  
  if (SrcE->AName != NULL)
    {
    DstEP->AName = (char **)MUCalloc(1,SrcE->ACount * sizeof(char *));

    for (tmpIndex = 0;tmpIndex < SrcE->ACount;tmpIndex++)
      {
      DstEP->AName[tmpIndex] = NULL;

      if (MUStrDup(&DstEP->AName[tmpIndex],SrcE->AName[tmpIndex]) == FAILURE)
        {
        MUFree(&DstEP->Name);
        MUFree(&DstEP->Val);
        MUFree(DstEP->AName);
        MUFree((char **)&DstEP);

        return(FAILURE);
        }
      }
    }

  /* Attribute values */

  if (SrcE->AVal != NULL)
    {
    DstEP->AVal = (char **)MUCalloc(1,SrcE->ACount * sizeof(char *));

    for (tmpIndex = 0;tmpIndex < SrcE->ACount;tmpIndex++)
      {
      DstEP->AVal[tmpIndex] = NULL;

      if (MUStrDup(&DstEP->AVal[tmpIndex],SrcE->AVal[tmpIndex]) == FAILURE)
        {
        MUFree(&DstEP->Name);
        MUFree(&DstEP->Val);
        MUFree(DstEP->AName); 
        MUFree(DstEP->AVal);
        MUFree((char **)&DstEP);

        return(FAILURE);
        }
      }
    }

  /* Type */
  DstEP->Type = SrcE->Type;
  
  /* Children */
  if (SrcE->C != NULL)
    {
    DstEP->C = (mxml_t **)MUCalloc(1,sizeof(mxml_t *) * SrcE->CCount);

    for (tmpIndex = 0;tmpIndex < SrcE->CCount;tmpIndex++)
      {
      if (MXMLDupENoString(SrcE->C[tmpIndex],&DstEP->C[tmpIndex]) == FAILURE)
        {
        int destroyIndex;
        MUFree(&DstEP->Name);
        MUFree(&DstEP->Val);
        MUFree(DstEP->AName);
        MUFree(DstEP->AVal);

        for (destroyIndex = 0;destroyIndex < tmpIndex;destroyIndex++)
          {
          MXMLDestroyE(&(DstEP->C[destroyIndex]));
          }
        MUFree((char **)&DstEP);

        return(FAILURE);
        } 
      }
    }
  else
    {
    DstEP->C = NULL;
    }

  /* XD is not copied into the duplicate! */
  DstEP->XD = NULL;

  *DstE = DstEP;

  return(SUCCESS);
  }  /* END MXMLDupENoString() */
#endif


/**
 * Add a CData to an XML element.
 *
 * @param E     [I] - (modified) The XML element to add the CData to
 * @param CData [I] - The CData to add (copied)
 */

int MXMLAddCData(
  mxml_t     *E,
  const char *CData)

  {
  if ((E == NULL) || (CData == NULL))
    return(FAILURE);

  if (E->CDataCount >= E->CDataSize)
    {
    if (E->CData == NULL)
      {
      E->CData = (char **)MUCalloc(1,sizeof(char *) * MDEF_XMLICCOUNT);

      if (NULL == E->CData)
        return(FAILURE);

      E->CDataSize = MDEF_XMLICCOUNT;
      }
    else
      {
      char **NewPtr = NULL;

      NewPtr = (char **)MOSrealloc(E->CData,sizeof(char *) * (E->CDataSize << 1));

      if (NULL == NewPtr)
        return(FAILURE);

      E->CData = NewPtr;
      E->CDataSize <<= 1;
      }
    }  /* END if (E->CDataCount >= E->CDataSize) */

  MUStrDup(&E->CData[E->CDataCount],CData);

  E->CDataCount++;

  return(SUCCESS);
  }  /* END MXMLAddCData() */


/**
 * optionally filter XML before sending it to the server
 *
 * @param SDEP (I/O) pointer to XML structure that may optionally be destroyed and replaced
 * @param CIndex (I) The type of the request
 * @param IsServer (I) Whether this is being called by the server or the client
 * @param EMsg     (O)
 */

int MFilterXML(

  mxml_t **SDEP,
  int      CIndex,
  mbool_t  IsServer,
  char    *EMsg)

  {
  int rc = SUCCESS;

  mxml_t *SDE = *SDEP;

  if ((SDEP == NULL) || (EMsg == NULL))
    {
    return(FAILURE);
    }

  if (CIndex == mcsMSubmit)
    {
    /* send XML to a filter command and use the output of the command as the final XML
     * to send to the server
     */
    mxml_t    *JE = NULL;
    mxml_t    *EE = NULL;
    int        ReturnCode;
    char       tmpEMsg[MMAX_LINE];
    int        ChildIndex = 0;
    char      *Filter = NULL;
    mpsi_t     tmpP;
    mrm_t      tmpRM;

    if ((IsServer == FALSE) && (MUStrIsEmpty(MSched.ClientSubmitFilter)))
      {
      return(SUCCESS);
      }
    else if ((IsServer == TRUE) && (MUStrIsEmpty(MSched.ServerSubmitFilter)))
      {
      return(SUCCESS);
      }

    Filter = (IsServer == TRUE) ? MSched.ServerSubmitFilter : MSched.ClientSubmitFilter;
    
    if (MXMLGetChild(SDE,(char *)MXO[mxoJob],&ChildIndex,&JE) == FAILURE)
      {
      snprintf(EMsg,MMAX_LINE,"internal XML processing error (could not locate job XML structure)");

      return(FAILURE);
      }

    mstring_t  MBuffer(MMAX_BUFFER);

    if (MXMLToMString(JE,&MBuffer,(char const **) NULL,TRUE) == FAILURE)
      {
      snprintf(EMsg,MMAX_LINE,"error converting request to XML");

      return(FAILURE);
      }

    /* make a buffer at least twice as big as needed to hold the current XML
     * structure, and at least as big as MMAX_BUFFER
     */

    mstring_t  EnvString(MMAX_BUFFER);

    memset(&tmpP,0,sizeof(tmpP));

    if (C.Timeout > 0)
      tmpP.Timeout = C.Timeout; 


    if (C.EnvP != NULL)
      {
      /* Add full user env into the filter's execution env. */

      int eindex = 0;

      MRMCreate("envrm",&tmpRM);

      for (eindex = 0;C.EnvP[eindex] != NULL;eindex++)
        {
        MStringAppendF(&EnvString,"%s" ENVRS_ENCODED_STR,C.EnvP[eindex]);
        }

      tmpRM.Env = EnvString.c_str(); 

      tmpP.R = &tmpRM;
      }  /* END if (C.EnvP != NULL) */

    mstring_t  Output(MMAX_LINE);
    mstring_t  ErrBuf(MMAX_LINE);

    MUReadPipe2(
      Filter,
      MBuffer.c_str(),
      &Output,
      &ErrBuf,
      &tmpP,            /* I - contains timeout */
      &ReturnCode,
      tmpEMsg,
      NULL);

    snprintf(EMsg,MMAX_LINE,"%s",ErrBuf.c_str());

    if (!MUStrIsEmpty(EMsg) && (IsServer == FALSE))
      fprintf(stderr,"%s",EMsg);

    switch(ReturnCode)
      {
      case 0:

        if (MXMLFromString(&EE,Output.c_str(),NULL,tmpEMsg) == FAILURE)
          {
          snprintf(EMsg,MMAX_LINE,"failed to parse the XML output of Filter Command %s: (%s)\n",
            Filter,
            tmpEMsg);

          rc = FAILURE;
          }

        rc = SUCCESS;

        SDE->C[ChildIndex] = EE;

        break;

      case mscTimeout:

        rc = FAILURE;

        snprintf(EMsg,MMAX_LINE,"%s",tmpEMsg);

        break;

      default:

        rc = FAILURE;

        if (ErrBuf.empty())
          {
          snprintf(EMsg,MMAX_LINE,"\nFilter Command '%s' failed with status code %d\n",
              Filter,
              ReturnCode);
          }

        /*NOTREACHED*/

        break;
      }  /* END switch (ReturnCode) */
    }  /* if ((CIndex == mcsMSubmit) && ... */

  return(rc);
  }  /* END MFilterXML() */




/**
 * Sort an XML structure based on compare.
 *
 * @param E (I) [modified] Tree to sort
 * @param compare (I) function used to compare to mxml_t* nodes
 */

int MXMLSort(

  mxml_t *E,
  int     (*compare)(const void *,const void*))

  {
  int cindex;

  qsort(E->C,E->CCount,sizeof(mxml_t *),compare);

  for (cindex = 0;cindex < E->CCount;cindex++) 
    {
    MXMLSort(E->C[cindex], compare);
    }
  
  return(SUCCESS);
  }  /* END MXMLSort() */


/**
 * Perform a shallow copy of the XD mxml_t member.
 * 
 * WARNING: freeing one tree could lead to corrupt data in the other
 *
 * @param Src (I)
 * @param Dst (I)
 */

int MXMLShallowCopyXD(

  mxml_t *Src,
  mxml_t *Dst)

  {
  int cindex;

  Dst->TData = Src->TData;

  for (cindex = 0;((cindex < Src->CCount) && (cindex < Dst->CCount));cindex++)
    {
    MXMLShallowCopyXD(Src->C[cindex], Dst->C[cindex]);
    }

  return(SUCCESS);
  }  /* END MXMLShallowCopyXD() */


/**
 * Compare two mxml_t* structs by their Name attribute.
 * Case sensitive
 *
 * @param A
 * @param B
 */

int MXMLCompareByName(

  mxml_t **A, 
  mxml_t **B)

  {
  return(strcasecmp((*A)->Name,(*B)->Name));
  }  /* END MXMLCompareByName() */

/* END MXML.c */
