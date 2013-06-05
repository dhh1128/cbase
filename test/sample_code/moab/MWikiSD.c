/* HEADER */

      
/**
 * @file MWikiSD.c
 *
 * Contains: MWikiSD
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  

#include "MWikiInternal.h"


/**
 *
 *
 * @param AString (I)
 * @param SD (I) [modified]
 * @param R (I)
 */

int MWikiSDUpdate(

  char        *AString,  /* I */
  msdata_t    *SD,       /* I (modified) */
  mrm_t       *R)        /* I */

  {
  char *ptr;
  char *tail;

  char  SDAttr[MMAX_BUFFER];

  char  EMsg[MMAX_LINE];
  char  tmpLine[MMAX_LINE];

  const char *FName = "MWikiSDUpdate";

  MDB(2,fWIKI) MLog("%s(AString,SD,%s)\n",
    FName,
    (R != NULL) ? R->Name : "NULL");

  if ((SD == NULL) || (AString == NULL))
    {
    return(FAILURE);
    }

  ptr = AString;

  /* FORMAT:  <FIELD>=<VALUE> [<FIELD>=<VALUE>]... */

  while (ptr[0] != '\0')
    {
    if ((tail = MUStrChr(ptr,' ')) != NULL)
      {
      strncpy(SDAttr,ptr,MIN(MMAX_BUFFER - 1,tail - ptr));
      SDAttr[tail - ptr] = '\0';
      }
    else
      {
      MUStrCpy(SDAttr,ptr,MMAX_BUFFER);
      }

    if (MWikiSDUpdateAttr(SDAttr,SD,R,EMsg) == FAILURE)
      {
      snprintf(tmpLine,sizeof(tmpLine),"info corrupt for staging-data '%s -> %s' - %s",
        SD->SrcLocation,
        SD->DstLocation,
        EMsg);

      MMBAdd(&R->MB,tmpLine,NULL,mmbtNONE,0,0,NULL);
      }

    if (tail == NULL)
      break;

    ptr = tail + 1;
    }  /* END while ((tail = MUStrChr(ptr,';')) != NULL) */

  return(SUCCESS);
  }  /* END MWikiSDUpdate() */




/**
 *
 *
 * @param AttrString (I)
 * @param SD (I) [modified]
 * @param R (I) [optional]
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 */

int MWikiSDUpdateAttr(

  char     *AttrString,  /* I */
  msdata_t *SD,          /* I (modified) */
  mrm_t    *R,           /* I (optional) */
  char     *EMsg)        /* O (optional,minsize=MMAX_LINE) */

  {
  char *ptr;
  char *Value;
  char  tmpLine[MMAX_LINE];
  char *TokPtr;

  int   aindex;

  const char *FName = "MWikiSDUpdateAttr";

  MDB(6,fWIKI) MLog("%s(%.32s,SD,R,EMsg)\n",
    FName,
    (AttrString != NULL) ? AttrString : "NULL");

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if ((SD == NULL) || (AttrString == NULL))
    {
    return(FAILURE);
    }

  if ((ptr = MUStrChr(AttrString,'=')) == NULL)
    {
    if (EMsg != NULL)
      {
      snprintf(EMsg,MMAX_LINE,"attribute '%s' malformed - no '='",
        AttrString);
      }

    return(FAILURE);
    }

  /* FORMAT:  <ATTR>={"<ALNUM_STRING>"|<ALNUM_STRING>} */

  /* make copy of attribute string */

  MUStrCpy(tmpLine,AttrString,sizeof(tmpLine));

  ptr = MUStrTokE(tmpLine,"=",&TokPtr);

  aindex = MUGetIndex(ptr,MWikiSDAttr,FALSE,0);

  if (aindex == 0)
    {
    if (EMsg != NULL)
      {
      snprintf(EMsg,MMAX_LINE,"invalid attribute '%s' specified",
        AttrString);
      }

    return(FAILURE);
    }

  Value = MUStrTokE(NULL,"=",&TokPtr);

  if (Value == NULL)
    {
    snprintf(EMsg,MMAX_LINE,"missing value for attribute '%s'",
      AttrString);

    return(FAILURE);
    }

  switch (aindex)
    {
    case mwsdaBytesTransferred:
        
      {
      mulong NewSize;

      NewSize = strtol(Value,NULL,10);

      if (NewSize != SD->DstFileSize)
        {
        /* mark that the staging operating is not stalled */

        SD->UTime = MSched.Time;
        }

      MSDSetAttr(SD,msdaDstFileSize,(void **)&NewSize,mdfLong,mSet);
      }  /* END BLOCK */

      break;
      
    case mwsdaEndTime:

      /* data staging operation has finished? */

      /* NYI */
      
      break;

    case mwsdaError:

      MUStrDup(&SD->EMsg,Value);

      break;
      
    case mwsdaFileSize:

      {
      mulong tmpL = strtol(Value,NULL,10);

      MSDSetAttr(SD,msdaSrcFileSize,(void **)&tmpL,mdfLong,mSet);
      }  /* END BLOCK */
      
      break;
      
    case mwsdaRemoveTime:

      /* NYI */

      break;
      
    case mwsdaSourceURL:
      
      /* verify that this URL matches what we expect */

      if (strcmp(SD->SrcLocation,Value))
        {
        /* URL's do not match - mismatched record! */

        MDB(2,fWIKI) MLog("WARNING:  stage-data src location is being incorrectly reported via WIKI '%s' != '%s'\n",
          Value,
          SD->SrcLocation);

        return(FAILURE);
        }
            
      break;
      
    case mwsdaStartTime:

      {
      mulong tmpL = strtol(Value,NULL,10);

      MSDSetAttr(SD,msdaTStartDate,(void **)&tmpL,mdfLong,mSet);
      }  /* END BLOCK */

      break;
      
    case mwsdaState:

      MSDSetAttr(SD,msdaState,(void **)Value,mdfString,mSet);
      
      break;
      
    case mwsdaUser:

      /* NYI */
      
      break;
      
    default:

      MDB(2,fWIKI) MLog("INFO:     WIKI keyword '%s'(%d) not handled\n",
        MWikiSDAttr[aindex],
        aindex);

      return(FAILURE);

      /*NOTREACHED*/
 
      break;
    }  /* END switch (aindex) */

  return(SUCCESS);
  }  /* END MWikiSDUpdateAttr() */

/* END MWikiSD.c */
