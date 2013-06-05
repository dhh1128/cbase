/* HEADER */

      
/**
 * @file MUtilXML.c
 *
 * Contains: XML Utility functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  

/**
 * Encodes a string so it is ready to be placed in XML.
 *
 * WARNING: This function is not thread-safe, however, it is currently
 * only used in the client commands (single-threaded programs).
 *
 * @param SString (I)
 * @param DString (O) [optional]
 * @param SCList (I) List of special XML characters to translate [optional]
 * @param QuotedOnly (I) if TRUE, only encode double-quoted portions of string
 * @param DStringLen (I)
 */

char *MUXMLEncode(

  char *SString,
  char *DString,
  const char *SCList,
  mbool_t QuotedOnly,
  int   DStringLen)

  {
  char *ptr;

  char *SPtr;

  char *DPtr;
  int   DLen;

  char *BPtr = NULL;
  int   BSpace = 0;

  const char *DCList = "<>";

  const char *CList;
 
  mbool_t InQuote;
 

  mstring_t tmpString(MMAX_LINE);

  static char *tmpDString = NULL;
  static int   tmpDStringLen;

  const char *FName = "MUXMLEncode";

  MDB(5,fSTRUCT) MLog("%s(%.16s...,DString,%d)\n",
    FName,
    (SString != NULL) ? SString : "NULL",
    DStringLen);

  if (SString == NULL)
    {
    return(NULL);
    }

  /* IMPORTANT NOTE:  Routine only supports '<' and '>' chars in CList!!! */

  CList = (SCList != NULL) ? SCList : DCList;

  if (SString == DString)
    {
    tmpString = SString;

    SPtr = tmpString.c_str();
    }
  else
    {
    SPtr = SString;
    }

  if (DString == NULL)
    {
    if (tmpDString == NULL)
      {
      /* alloc generous space in case string has lots of substitutions */

      tmpDString    = (char *)MUMalloc(strlen(SString) * 2);
      tmpDStringLen = strlen(SString) * 2;
      }
    else if (tmpDStringLen < (int)(strlen(SString) * 2))
      {
      /* need to alloc more space */

      tmpDString    = (char *)realloc(tmpDString,strlen(SString) * 2);
      tmpDStringLen = strlen(SString) * 2;
      }
    else
      {
      /* buffer is sufficient */

      tmpDString[0] = '\0';
      }

    DPtr = tmpDString;
    DLen = tmpDStringLen;
    }
  else
    {
    DPtr = DString;
    DLen = DStringLen;
    }

  MUSNInit(&BPtr,&BSpace,DPtr,DLen);

  InQuote = FALSE;

  for (ptr = SPtr;*ptr != '\0';ptr++)
    {
    if (*ptr == '\"')
      InQuote = 1 - InQuote;

    /* characters to encode:  ??? */

    if (((QuotedOnly == FALSE) || (InQuote == TRUE)) && (strchr(CList,*ptr)))
      {
      switch (*ptr)
        {
        case '<':

          MUSNPrintF(&BPtr,&BSpace,"&lt;"); break;

        case '>':

          MUSNPrintF(&BPtr,&BSpace,"&gt;"); break;

        default:

          break;
        }
      }
    else
      {
      /* direct copy new character into destination string */

      if (BSpace > 0)
        {
        *BPtr = *ptr;

        BPtr++;
        BSpace--;

        *BPtr = '\0';
        }
      }
    }  /* END for (ptr) */

  if (BSpace <= 0)
    {
    MDB(0,fSTRUCT) MLog("ALERT:    destination string too small for expanded URL ('%.32s' > %d)\n",
      SString,
      DLen);

    return(NULL);
    }

  return(DPtr);
  }  /* END MUXMLEncode() */



/**
 *
 *
 * @param E (I) [modified]
 * @param Status (I)
 * @param Msg (I)
 * @param Code (I)
 */

int MUIXMLSetStatus(

  mxml_t     *E,
  int         Status,
  const char *Msg,
  int         Code)

  {
  mxml_t *C = NULL;

  if (E == NULL)
    {
    return(FAILURE);
    }

  if ((MXMLGetChild(E,"status",NULL,&C) == FAILURE) &&
     ((MXMLCreateE(&C,"status") == FAILURE) ||
      (MXMLAddE(E,C) == FAILURE)))
    {
    /* cannot add status child */

    return(FAILURE);
    }

  if (Status == SUCCESS)
    MXMLSetVal(C,(void *)"success",mdfString);
  else
    MXMLSetVal(C,(void *)"failure",mdfString);

  if (Msg != NULL)
    MXMLSetAttr(C,"message",(void **)Msg,mdfString);

  MXMLSetAttr(C,"code",(void **)&Code,mdfInt);
    
  return(SUCCESS);
  }  /* END MUIXMLSetStatus() */

/* END MUtilXML.c */
