
/* HEADER */

/**
 * @file MUHostList.c
 * 
 * Contains various functions manipulate Host Lists
 *
 */


#include "moab.h"
#include "moab-proto.h"
#include "moab-const.h"
#include "moab-global.h"



/**
 * This function takes a node name, applies node range information (if 
 *   applicable) and produces a host string that can be added to a hostlist.  
 *
 * @param NodeName (I) Node Name
 * @param PrefixLen (I) [optional] Length of the node name prefix (e.g. 4 in the case of odev13), -1 for non-range hostlist
 * @param StartIndexLen [optional] (I) length of the index number (e.g. 3 in the case of odev127), ignored if PrefixLen == -1
 * @param StartIndex (I) [optional] Node index from the node name (e.g. 13 in the case of odev13), ignored if PrefixLen == -1
 * @param EndIndexLen (I) [optional] length of the end index number, ignored if PrefixLen == -1
 * @param EndIndex (I) [optional] Node index from the ending index name, , ignored if PrefixLen == -1
 * @param NodeNameSuffix (I) [optional] suffix following the node digits (e.g. "abc" in the case of odev13abc), ignored if PrefixLen == -1
 * @param StartTC (I) Task Count - Set to -1 to not include the task count in the hostlist string
 * @param HostListElement (O) Location to put the hostlist string element
 */

int MUBuildHostListElement(

  char *NodeName,
  int PrefixLen,
  int StartIndexLen,
  int StartIndex,
  int EndIndexLen,
  int EndIndex,
  char *NodeNameSuffix,
  int StartTC,
  char *HostListElement)

  {
  char TCString[MMAX_LINE];
  char Prefix[MMAX_LINE];
  int SIndex;
  int SIndexLen;
  int EIndex;
  int EIndexLen;

  if (HostListElement == NULL)
    {
    return(FAILURE);
    }

  TCString[0] = '\0';

  if (StartTC != -1)
    {
    /* Note that SLURM requires "*1" if the task count is 1. It cannot be omitted if 1 */

    sprintf(TCString,"*%d",
      StartTC);
    }

  if (PrefixLen < 0)
    {
    /* Build non-range hostlist string  (e.g. odev[14]*13 */

    int tmpPrefixLen = 0;
    int tmpIndexLen = 0;
    int tmpIndex = 0;
    char *NameSuffix;

    MUNodeNameNumberOffset(NodeName, &tmpPrefixLen, &tmpIndexLen);

    if (tmpIndexLen > 0)
      {
      tmpIndex = (int)strtol(NodeName + tmpPrefixLen,&NameSuffix,10);

      sprintf(HostListElement, "%.*s[%0*d]%s%s",
        tmpPrefixLen,
        NodeName,
        tmpIndexLen,
        tmpIndex,
        NameSuffix,
        TCString);
      }
    else
      {
      sprintf(HostListElement, "%s%s",
        NodeName,
        TCString);
      }
    }
  else
    {
    /* Build the hostlist range string (e.g odev[04-23]abc*13) */

    if ((StartIndexLen <= 0) || (StartIndex < 0) || (EndIndexLen <= 0) || (EndIndex < 0))
      {
      strcpy(HostListElement,"");

      return(FAILURE);
      }

    if (strlen(NodeName) < (unsigned int) PrefixLen)
      {
      strcpy(HostListElement,"");

      return(FAILURE);
      }

    Prefix[0] = '\0';
    if (PrefixLen > 0)
      {
      sprintf(Prefix,"%.*s",PrefixLen,NodeName);
      }

    /* If we have a descending range then make it ascending */

    if (StartIndex > EndIndex)
      {
      SIndex = EndIndex;
      SIndexLen = EndIndexLen;
      EIndex = StartIndex;
      EIndexLen = StartIndexLen;
      }
    else
      {
      SIndex = StartIndex;
      SIndexLen = StartIndexLen;
      EIndex = EndIndex;
      EIndexLen = EndIndexLen;
      }

    sprintf(HostListElement, "%s[%0*d-%0*d]%s%s",
      Prefix,
      SIndexLen,
      SIndex,
      EIndexLen,
      EIndex,
      NodeNameSuffix,
      TCString);
    } /*END else PrefixLen == -1 */

  return(SUCCESS);
  } /* MUBuildHostListElement() */






/**
 * This function takes a HostList with ranges "odev[14-16]abc*14,odev[17]abc*14" 
 *   and produces "odev[14-16,17]abc*14"
 *
 * @param HostList (I/O) [Modified] HostList
 */

void MUCollapseHostListRanges(

  char *HostList)

  {
  mbool_t IsMatch = FALSE;
  char *CurrentPtr;
  char *NextPtr = NULL;
  char *TokPtr = NULL;
  char *NextIndex;
  int CurrentPrefixLen;
  int NextPrefixLen;
  char *CurrentSuffix;
  char *NextSuffix;
  char *tmpPtr;
  int OriginalHostListLen;

  if (HostList == NULL)
    {
    return;
    }

  /* There must be at least two ranges in the hostlist in order to do any compression */

  if ((tmpPtr = strchr(HostList,'[')) == NULL)
    return;

  if (strchr(tmpPtr + 1,'[') == NULL)
    return;

  OriginalHostListLen = strlen(HostList);

  /* Make a copy of HostList so that we can tokenize it */

  tmpPtr = NULL;
  MUStrDup(&tmpPtr,HostList);

  CurrentPtr = MUStrTokEArrayPlus(tmpPtr,":",&TokPtr,FALSE);

  /* We are going to rebuild the HostList so copy the first tokenized entry into HostList */

  /* Note that we are writing over top of the current HostList. The resultant compressed
   * Hostlist should be the same size or smaller than the current hostlist so we should not
   * be in danger of copying beyond the end of HostList. */

  strcpy(HostList,CurrentPtr);

  /* Compare current token with the next token. If the next token can be consolidated into the current
   * token then do so in the HostList and repeat the process with the next token. If the next token 
   * cannot be consolidated into the current token, then add it to the end of the hostlist and make 
   * it the current token. */

  while (CurrentPtr != NULL)
    {

    if ((IsMatch != TRUE) && (NextPtr != NULL))
      {
      /* The next token cannot be consolidated into the current token, so add the next token to
       * the end of the HostList and make it the current token */

      if ((strlen(HostList) + strlen(NextPtr) + 1) >  /* add one to take into account the comma */ 
          ((unsigned int)OriginalHostListLen + 1))    /* add one to take into account the NULL terminator */
        {
        /* Safety check to make sure we don't overrun our HostList buffer. Should never happen. */

        break;
        }

      MUStrCat(HostList,":",OriginalHostListLen + 1); /* MUStrCat assumes the length includes the NULL terminator so we need + 1 */
      MUStrCat(HostList,NextPtr,OriginalHostListLen + 1);
      CurrentPtr = NextPtr;
      }

    /* Get the next token */

    NextPtr = MUStrTokEArrayPlus(NULL,":",&TokPtr,FALSE);
    if (NextPtr == NULL)
      break;

    /* Make sure that both the current and next elements have ranges */

    if (strchr(CurrentPtr,'[') == NULL)
      {
      IsMatch = FALSE;
      continue;
      }

    if ((NextIndex = strchr(NextPtr,'[')) == NULL)
      {
      IsMatch = FALSE;
      continue;
      }

    /* Make sure the node name prefixes are the same */

    CurrentPrefixLen = strcspn(CurrentPtr,"[");
    NextPrefixLen = strcspn(NextPtr,"[");

    if ((CurrentPrefixLen != NextPrefixLen) ||
        (strncmp(CurrentPtr,NextPtr,CurrentPrefixLen) != 0))
      {
      IsMatch = FALSE;
      continue;
      }

    /* Make sure the suffixes are the same */

    CurrentSuffix = strchr(CurrentPtr,']');
    NextSuffix = strchr(NextPtr,']');

    if ((CurrentSuffix == NULL) || (NextSuffix == NULL))
      {
      /* possible overflow occurred and encountered garbage. */

      MUStrCat(HostList,"...",OriginalHostListLen + 1); /* MUStrCat assumes the length includes the NULL terminator so we need + 1 */

      MUFree(&tmpPtr);

      return;
      }

    if (strcmp(CurrentSuffix,NextSuffix) != 0)
      {
      IsMatch = FALSE;
      continue;
      }

    /* Current and next have the same node name, and suffix, so combine them in the hostlist */

    CurrentSuffix = strrchr(HostList,']'); /* find the last ']' at the end of the HostList */

    /* odev[2-4,13]abc*13 and odev[18-19]abc*13 will be consolidated to odev[2-4,13,18-19]abc*13 */

    snprintf(CurrentSuffix, OriginalHostListLen - (CurrentSuffix - HostList), ",%s", NextPtr + NextPrefixLen + 1);
    IsMatch = TRUE;
    } /* END While CurrentPtr != NULL */

  MUFree(&tmpPtr);

  return;
  }  /* END MUOptimizeHostListRanges() */




/**
 * Builds a Bluegene/L hostlist element (single and range).
 *
 * Assumes the prefixes of the host names are the same @see MUNodeNameAdjacent().
 * Ex. kecheA00 and kecheA03 converts to keche[A00xA03]
 *
 * @param StartHost (I) The begginging of the host list range. (ex. kecheA00)
 * @param EndHost The (I) Ending of the host list range. (ex. kecheA03) [optional]
 * @param HostListElement Returns the hostlistelement in this.
 */

int MUBuildBGLHostListElement(

  char *StartHost,  /* I */
  char *EndHost,    /* I (optional)*/
  char *HostListElement) /* O */

  {
  int PrefixLen;
  int StartIndexLen;

  if ((StartHost == NULL) || (HostListElement == NULL))
    {
    /* Need at least StartHost and HostListElement */

    return(FAILURE);
    }

  /* Prefix is length of the node name -3 because bg/l nodes are define in 3 dimensions */

  PrefixLen = strlen(StartHost) - 3;
  StartIndexLen = 3; 
    
  if((EndHost == NULL) || (strcmp(StartHost,EndHost) == 0))
    {
    /* Single node element */

    /*sprintf(HostListElement,"%s",StartHost);*/
    sprintf(HostListElement,"%.*s[%.*s]",
        PrefixLen,
        StartHost,
        StartIndexLen,
        StartHost + PrefixLen);
    }
  else
    {
    sprintf(HostListElement,"%.*s[%.*s-%.*s]",
      PrefixLen,
      StartHost,
      StartIndexLen,
      StartHost + PrefixLen,
      StartIndexLen,
      EndHost + PrefixLen);
    }

  return(SUCCESS);
  } /* END int MUBuildBGLHostListElement() */



/**
 * Create null-terminated hostlist from tasklist.
 *
 * @param TM (I)
 * @param HostList (O)
 * @param R (I)
 */

int MUTMToHostList(

  int     *TM,       /* I */
  char   **HostList, /* O */
  mrm_t   *R)        /* I */

  {
  int index;
  int tindex;

  const char *FName = "MUTMToHostList";

  MDB(4,fLL) MLog("%s(TM,HostList,%s)\n",
    FName,
    (R != NULL) ? R->Name : "NULL");

  if ((TM == NULL) || (HostList == NULL) || (R == NULL))
    {
    return(FAILURE);
    }

  tindex = 0;

  for (index = 0;TM[index] != -1;index++)
    {
    if (MNode[TM[index]]->RM == R)
      {
      HostList[tindex] = MNode[TM[index]]->Name;

      MDB(6,fLL) MLog("INFO:     TM[%02d] %s\n",
        tindex,
        HostList[tindex]);

      tindex++;
      }
    }    /* END for (index) */

  HostList[tindex] = NULL;

  return(SUCCESS);
  }  /* END MUTMToHostList() */


