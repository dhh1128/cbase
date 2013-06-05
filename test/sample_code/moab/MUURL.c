
/* HEADER */

/**
 * @file MUURL.c
 *
 * Contains Functions for parsing URL strings
 */

#include "moab.h"
#include "moab-proto.h"
#include "moab-const.h"
#include "moab-global.h"




/**
 * Creates a URL string with the format: [<PROTO>://]<HOST>[:<PORT>][/<DIR>]. 
 *
 * @param Protocol (I) [optional]
 * @param HostName (I)
 * @param Directory (I) [optional]
 * @param Port (I) [optional]
 * @param String (O) Holds created URL string
 */

int MUURLCreate(

  char *Protocol,
  char *HostName,
  char *Directory,
  int   Port,
  mstring_t *String)

  {
  if (HostName == NULL)
    {
    return(FAILURE);
    }

  MStringSet(String,"\0");

  /* FORMAT:  [<PROTO>://]<HOST>[:<PORT>][/<DIR>] */

  if ((Protocol != NULL) && 
      (Protocol[0] != '\0') &&
      (Protocol[0] != '['))
    {
    MStringAppendF(String,"%s://%s",
      Protocol,
      HostName);
    }
  else
    {
    MStringAppendF(String,"%s",
      HostName);
    }

  if (Port > 0)
    {
    MStringAppendF(String,":%d",
      Port);
    }

  if ((Directory != NULL) && (Directory[0] != '\0'))
    {
    MStringAppendF(String,"%s%s",
      (Directory[0] != '/') ? "/" : "",
      Directory);
    }

  return(SUCCESS);
  }  /* END MUURLCreate() */


/**
 * Perform URL-compliant character encoding.
 *
 * (hide ' ' '&', '$', '@', etc)
 *
 * NOTE:  see http://www.blooberry.com/indexdot/html/topics/urlencoding.htm
 * NOTE:  return NULL on FAILURE
 *
 * @param SString (I)
 * @param DString (O)
 * @param DStringLen (I)
 */

char *MUURLEncode(

  const char *SString,     /* I */
  char *DString,     /* O */
  int   DStringLen)  /* I */

  {
  const char *ptr;

  const char *SPtr;

  char *BPtr = NULL;
  int   BSpace = 0;


  mstring_t tmpString(MMAX_LINE);

  const char *FName = "MUURLEncode";

  MDB(5,fSTRUCT) MLog("%s(%.16s...,DString,%d)\n",
    FName,
    (SString != NULL) ? SString : "NULL",
    DStringLen);

#ifndef __MOPT
  if ((SString == NULL) || (DString == NULL))
    {
    return(NULL);
    }
#endif /* !__MOPT */

  if (SString == DString)
    {
    tmpString = SString;

    SPtr = tmpString.c_str();
    }
  else
    {
    SPtr = SString;
    }

  MUSNInit(&BPtr,&BSpace,DString,DStringLen);

  for (ptr = SPtr;*ptr != '\0';ptr++)
    {
    /* characters to encode:  0-30, 32, 34-38, 43, 44, 47, 58-64, 91-94, 96, 123-255 */

    if ((*ptr <= 30) || 
        (*ptr == 32) || 
       ((*ptr >= 34) && (*ptr <= 38)) ||
        (*ptr == 43) ||
        (*ptr == 44) ||
        (*ptr == 47) ||
       ((*ptr >= 58) && (*ptr <= 64)) ||
       ((*ptr >= 91) && (*ptr <= 94)) ||
        (*ptr == 96) ||
        (*ptr >= 123))
      {
      MUSNPrintF(&BPtr,&BSpace,"%%%02x",
        (int)*ptr);
      }
    else
      {
      if (BSpace > 0)
        {
        /* direct copy new character into destination string */

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
      DStringLen);

    return(NULL);
    }

  return(DString);
  }  /* END MUURLEncode() */



/**
 * Parse URL string into protocol, host, path, and port components.
 *
 * @see MUURLQueryParse()
 * @see MUInsertVarList() - child
 *
 * @param URL (I)
 * @param Protocol (O) [optional,minsize=MMAX_NAME]
 * @param HostName (O) [optional,minsize=MMAX_NAME]
 * @param Directory (O) [optional]
 * @param DirSize (I)
 * @param Port (O) [optional]
 * @param DoInitialize (I) Initialize all of the output parameters to the empty string before modifying them.
 * @param DoInsertVar (I)
 */

int MUURLParse(

  char    *URL,           /* I */
  char    *Protocol,      /* O (optional,minsize=MMAX_NAME) */
  char    *HostName,      /* O (optional,minsize=MMAX_NAME) */
  char    *Directory,     /* O (optional) */
  int      DirSize,       /* I */
  int     *Port,          /* O (optional) */
  mbool_t  DoInitialize,  /* I */
  mbool_t  DoInsertVar)   /* I */

  {
  char *head;
  char *tail;

  char  tmpLine[MMAX_LINE];

  if (URL == NULL)
    {
    return(FAILURE);
    }

  if (DoInitialize == TRUE)
    {
    if (Protocol != NULL)
      Protocol[0] = '\0';

    if (HostName != NULL)
      HostName[0] = '\0';

    if (Directory != NULL)
      Directory[0] = '\0';

    if (Port != NULL)
      *Port = 0;
    }  /* END if (DoInitialize == TRUE) */

  if ((DoInsertVar == TRUE) && (strchr(URL,'$') != NULL))
    {
    MUInsertVarList(
      URL,             /* I */
      NULL,
      NULL,
      NULL,
      tmpLine,         /* O */
      sizeof(tmpLine),
      FALSE);
    }
  else
    {
    MUStrCpy(tmpLine,URL,sizeof(tmpLine));
    }

  head = tmpLine;

  /* FORMAT:  [<PROTO>://][<HOST>[:<PORT>]][/<DIR>] */

  if ((tail = strstr(head,"://")) != NULL)
    {
    /* locate/copy protocol */
    /* tail marks end of protocol */

    if (Protocol != NULL)
      MUStrCpy(Protocol,head,MIN(tail - head + 1,MMAX_NAME));

    /* advance head to end of protocol */

    head = tail + strlen("://");
    }

  /* locate hostname terminator */

  if (!(tail = strchr(head,':')) &&
      !(tail = strchr(head,'/')))
    {
    tail = head + strlen(head);
    }

  if (*head != '/')
    {
    /* FORMAT:  <HOST>:<PORT> */

    if (HostName != NULL)
      MUStrCpy(HostName,head,MIN(tail - head + 1,MMAX_NAME));
 
    head = tail;
  
    if (*head == ':')
      {
      /* extract port */
   
      if (Port != NULL)
        *Port = (int)strtol(head + 1,&tail,0);
    
      head = tail;
      }
    }    /* END if (*head != '/') */
    
  if (*head == '/')
    {
    /* extract directory */

    tail = head + strlen(head);

    if (Directory != NULL)
      MUStrCpy(Directory,head,MIN(tail - head + 1,DirSize));
    
    head = tail;
    }

  return(SUCCESS);
  }  /* END MUURLParse() */





/**
 *
 *
 * @param URL (I)
 * @param Protocol (O) [optional,minsize=MMAX_NAME]
 * @param HostName (O) [optional,minsize=MMAX_NAME]
 * @param Port (O) [optional]
 * @param Directory (O) [optional]
 * @param DirSize (I)
 * @param Query (O) [optional,minsize=MMAX_NAME]
 * @param DoInitialize (I)
 */

int MUURLQueryParse(

  char    *URL,           /* I */
  char    *Protocol,      /* O (optional,minsize=MMAX_NAME) */
  char    *HostName,      /* O (optional,minsize=MMAX_NAME) */
  int     *Port,          /* O (optional) */
  char    *Directory,     /* O (optional) */
  int      DirSize,       /* I */
  char    *Query,         /* O (optional,minsize=MMAX_NAME) */
  mbool_t  DoInitialize)  /* I */

  {
  char *ptr;

  if (URL == NULL)
    {
    return(FAILURE);
    }

  if (DoInitialize == TRUE)
    {
    if (Query != NULL)
      Query[0] = '\0';
    }  /* END if (DoInitialize == TRUE) */

  if (MUURLParse(URL,Protocol,HostName,Directory,DirSize,Port,DoInitialize,FALSE) == FAILURE)
    {
    return(FAILURE);
    }

  if ((Directory != NULL) && ((ptr = strstr(Directory,"?")) != NULL))
    {
    MUStrCpy(Query,ptr + 1,MIN(sizeof(ptr),MMAX_NAME));

    *ptr = '\0';
    }

  return(SUCCESS);
  }  /* END MUURLQueryParse() */
/* END MUURL.c */
