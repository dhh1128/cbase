/* HEADER */

/**
 * @file MURE.c
 *
 * Contains: 
 *    MUREToList function
 *    MURangeToList function
 *
 */


#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  




/**
 * Given a 'specification pattern: SPattern' or a Job Expression
 * look for said pattern in the list of Current Jobs
 *
 * Only Moab.<number> and <number>[<number>] name patterns are parsed
 *
 * Moab subjobs are not parsed in 'r:' range listing, because
 * that will require to iterator specifiers. Possible future feature.
 *
 * @param SPattern    (I) [modified,maxsize=MMAX_BUFFER]
 * @param OType       (I)
 * @param P           (I) [Partition optional]
 * @param Array       (O) (optional) should already be created
 * @param Buffer      (I) [minsize=BufSize] I - match field or O message string
 * @param BufSize     (I) [size of above Buffer]
 */

int MURangeToList(

  char              *SPattern,
  enum MXMLOTypeEnum OType,
  mpar_t            *P,
  marray_t          *Array,
  char              *Buffer,
  int                BufSize)

  {
  mbool_t MatchFound = FALSE;

  int   index;

  char *ptr;
  char *TokPtr;

  char *Pattern;

  char  tmpBuf[MMAX_BUFFER];
  char  Header[MMAX_NAME];

  int   StartIndex;
  int   EndIndex;

  char *tail;

  char  Match[MMAX_LINE];

  mnode_t *N;

  mnode_t  tmpNode;

  mjob_t *J;
  char JName[MMAX_NAME];
  int jindex;

  char *BPtr;
  int   BSpace;

  /* Validate that we have a 'SPattern' and an 'OList' */
  if (NULL == SPattern)
    {
    if (NULL != Buffer)
      strcpy(Buffer,"no pattern specified");

    return(FAILURE);
    }

  /* Check for Range (r:) or Regex (x:) tag before the specification string */
  if (!strncasecmp(SPattern,"r:",strlen("r:")))
    {
    Pattern = SPattern + strlen("r:");
    }
  else if (!strncasecmp(SPattern,"x:",strlen("x:")))
    {
    /* regex not processed locally */

    return(FAILURE);
    }
  else
    {
    Pattern = SPattern;
    }

  if (Buffer != NULL)
    {
    if (strlen(Buffer) >= sizeof(Match))
      {
      /* specified buffer is too large */

      if (Buffer != NULL)
        {
        sprintf(Buffer,"pattern too long (exceeds %d character max)",
          (int)sizeof(Match));
        }

      return(FAILURE);
      }

    MUStrCpy(Match,Buffer,sizeof(Match));

    if (Match[0] != '\0')
      {
      MUStrCpy(tmpNode.Name,Match,sizeof(tmpNode.Name));

      MNodeProcessName(&tmpNode,Match);
      }

    MUSNInit(&BPtr,&BSpace,Buffer,BufSize);
    }
  else
    {
    Match[0] = '\0';
    }

  /* translate pattern into range list */

  if (strlen(Pattern) >= sizeof(tmpBuf))
    {
    /* specified pattern is too large */

    if (Buffer != NULL)
      {
      sprintf(Buffer,"pattern too long (exceeds %d character max)",
        (int)sizeof(tmpBuf));
      }

    return(FAILURE);
    }

  MUStrCpy(tmpBuf,Pattern,sizeof(tmpBuf));

  /* FORMAT:  <START>[-<END>][,<START>[-<END>]]... */

  ptr = MUStrTok(tmpBuf," \t\n,",&TokPtr);

  Header[0] = '\0';  /* start with empty 'Header' */

  /* Scan for one or more JobExpressions */
  while (ptr != NULL)
    {
    /* NOTE: if Header is not explicitly specified, search will inherit previous Header */

    if (isalpha(ptr[0]) || (OType == mxoJob))
      {
      char *vptr;

      /* save header */

      /* FORMAT: tg-c[7-897] */

      if ((vptr = strchr(ptr,'[')) == NULL)
        { 
        /* invalid format specified */

        if (Buffer != NULL)
          strcpy(Buffer,"range requires square bracket delimiters");

        return(FAILURE);
        }

      /* Extract just the Header of the name: stuff before the '['  */
      MUStrCpy(Header,ptr,MIN((int)sizeof(Header),vptr - ptr + 1));

      ptr = vptr + 1;  
      }

    StartIndex = (int)strtol(ptr,&tail,10);

    if ((tail != NULL) && (*tail == '-'))
      EndIndex = (int)strtol(tail + 1,NULL,10);
    else
      EndIndex = StartIndex; 
    
    switch (OType)
      {
      case mxoJob:

        /* Iterate through jobs from startindex to endindex and test 
         * validity of a match
         *
         * Valid names:
         *    PATTERN                   TYPE             EXAMPLE
         *    <string>.<number>         moab             Moab.51
         *    <number>[index]           torque subjob    536[1]
         */
        for (jindex = StartIndex;jindex < EndIndex + 1;jindex++)
          {
          /* Header + index is job name */
          if (isalpha(Header[0]))
            {
            sprintf(JName,"%s%d",Header,jindex);
            }
          else
            {
            /* torque job array starts with number ex: 536[1]*/
            sprintf(JName,"%s%s%d%s",Header,"[",jindex,"]");
            }

          /* find the job and add it to the list to deal with */
          if (MJobFind(JName,&J,mjsmBasic) == SUCCESS)
            {
            /* Add the hit to the Output List */

            if (Array == NULL)
              {
              return(SUCCESS);
              }

            MUArrayListAppendPtr(Array,J);

            MatchFound = TRUE;
            }
          }

        break;

      case mxoNode:

        for (index = 0;index < MSched.M[mxoNode];index++)
          {
          if (Match[0] != '\0')
            N = &tmpNode;
          else
            N = MNode[index];

          if ((N == NULL) || (N->Name[0] == '\0'))
            break;

          if (N->Name[0] == '\1')
            continue;

          if ((N != &tmpNode) && (P != NULL) && (P->Index > 0) && (N->PtIndex > 0) && (N->PtIndex != P->Index))
            {
            /* ignore partition check if node has no partition or P is MPar[0] */

            continue;
            }

          if ((N->NodeIndex < StartIndex) || (N->NodeIndex > EndIndex))
            continue;

          if (Header[0] != '\0')
            {
            /* FORMAT:  <HEADER><INDEX> */

            if (strncasecmp(N->Name,Header,strlen(Header)))
              continue;

            if (!isdigit(N->Name[strlen(Header)]))
              continue;
            }  /* END if (Header[0] != '\0') */

          if (Array == NULL)
            {
            /* match located */

            return(SUCCESS);
            }

          /* add to list */

          MUArrayListAppendPtr(Array,N);
              
          MatchFound = TRUE;

          if (Match[0] != '\0')
            break;
          }  /* END for (index) */

        break;

      case mxoRsv:

        /* NYI */

        break;

      default:

        /* NOT-SUPPORTED */

        return(FAILURE);
   
        /*NOTREACHED*/

        break;
      }  /* END switch (OType) */

    ptr = MUStrTok(NULL," \t\n,",&TokPtr);
    }  /* END while (ptr != NULL) */

  if (MatchFound == FALSE)
    {
    if (Buffer != NULL)
      strcpy(Buffer,"no matches found");

    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MURangeToList() */



/* POSIX support is being used */

typedef struct {
    mstring_t *Pattern;   /* Pointer to a mstring, if NULL, mstring is not yet allocated */
    regex_t   re;
    mbool_t   IsAlloc;
    } mregex_t;

/**
 *
 *
 * @param Pattern (I)
 * @param RE (I) [modified]
 */

int __MURECompile(

  char *Pattern, /* I */
  void *RE)      /* I (modified) */

  {
  int   rc;

  if (RE == NULL)
    {
    return(FAILURE);
    }

  {
  regex_t *R;

  R = (regex_t *)&((mregex_t *)RE)->re;

  if ((RE != NULL) && (((mregex_t *)RE)->IsAlloc == TRUE))
    {
    regfree(R);

    ((mregex_t *)RE)->IsAlloc = FALSE;
    }

  rc = regcomp(
    (regex_t *)R,
    Pattern,
    REG_EXTENDED|REG_ICASE|REG_NEWLINE|REG_NOSUB);

  ((mregex_t *)RE)->IsAlloc = TRUE;

  if (rc != 0)
    {
    MDB(1,fSTRUCT) MLog("ALERT:    cannot compile regular expression '%s', rc: %d\n",
      Pattern,
      rc);

    return(FAILURE);
    }
  }    /* END BLOCK */

  return(SUCCESS);
  }  /* END __MURECompile() */





#define MMAX_RXCACHE   32         
 
/**
 * Expands a regular expression pattern into a list of object indexes.
 *
 * NOTE: This function is *NOT* threadsafe!
 *
 * @param SPattern    (I/O) [modified]
 * @param OType       (I)
 * @param P           (I) [optional]
 * @param Array       (O) [optional] only supported for Rsvs, will populate if present
 * @param DoAlternate (I) - lookup alternet (use job AName not job ID)
 * @param Buffer      (I) match field or O - human readable message string for success or failure (minsize=BufSize) [maxsize=MMAX_LINE]
 * @param BufSize     (I)
 */

int MUREToList(
 
  char              *SPattern,
  enum MXMLOTypeEnum OType,
  mpar_t            *P,
  marray_t          *Array,
  mbool_t            DoAlternate,
  char              *Buffer,
  int                BufSize)

  {
  job_iter JTI;
  rsv_iter RTI;

  static mregex_t rxcache[MMAX_RXCACHE + 1]; /* system will clean up 
                                                alloced mstring mem. */
  static mbool_t InitRequired = TRUE;

  mbool_t MatchFound = FALSE;
 
  int   index;
  int   rxindex;

  char *ptr;
  char *nptr;
  char *TokPtr;

  char *Pattern;
 
  char  Match[MMAX_LINE];
 
  mjob_t  *J; 
  mnode_t *N;
  mrsv_t  *R;

  char    *BPtr = NULL;
  int      BSpace = 0;

  int      rc;

  mbool_t  SearchRegEx = TRUE;

  enum MOExpSearchOrderEnum SearchOrder;
 
  const char *FName = "MUREToList";

  MDB(3,fSTRUCT) MLog("%s(%s,%s,%s,Array,Count,Buffer,%d)\n",
    FName,
    (SPattern != NULL) ? SPattern : "NULL",
    MXO[OType],
    (P != NULL) ? P->Name : "NULL",
    BufSize);

  if (SPattern == NULL)
    {
    return(FAILURE);
    }
 
  if (InitRequired == TRUE)
    {
    memset(rxcache,0,sizeof(rxcache));
 
    InitRequired = FALSE;
    }

  if (!strncasecmp(SPattern,"r:",strlen("r:")))
    {
    SearchOrder = moesoRange;

    Pattern = SPattern + strlen("r:");
    }
  else if (!strncasecmp(SPattern,"x:",strlen("x:")))
    {
    SearchOrder = moesoRE;

    Pattern = SPattern + strlen("x:");
    }
  else if (!strncasecmp(SPattern,"l:",strlen("l:")))
    {
    SearchOrder = moesoExact;

    Pattern = SPattern + strlen("l:");
    }
  else
    {
    SearchOrder = MSched.OESearchOrder;

    Pattern = SPattern;
    }

  MDB(7,fSCHED) MLog("%s - Search order: %s\n",
    FName,
    MOExpSearchOrder[SearchOrder]);

  switch (SearchOrder)
    {
    case moesoExact:

      if (OType == mxoRsv)
        {
        MDB(3,fSTRUCT) MLog("INFO:     l: not supported for reservations\n");
 
        if (Buffer != NULL)
          {
          snprintf(Buffer,BufSize,"\"l:\" not supported for reservations\n");
          }

        return(FAILURE);
        }

      /* fall through */

    case moesoNONE:

      /* search exact match only */

      SearchRegEx = FALSE;

      break;

    case moesoRange:

      rc = MURangeToList(
        SPattern,
        OType, 
        P,
        Array,
        Buffer,
        BufSize);

      MDB(7,fSCHED) MLog("%s returning %s with search order %s\n",
        FName,
        MBool[rc],
        MOExpSearchOrder[SearchOrder]);

      return(rc);

      /*NOTREACHED*/

      break;

    case moesoRE:

      /* process expression as regex */

      /* NO-OP */

      break;
   
    case moesoRangeRE:

      rc = MURangeToList(
        SPattern,
        OType,
        P,
        Array,
        Buffer,
        BufSize);

      if (rc == SUCCESS)
        {
        MDB(7,fSCHED) MLog("%s returning %s with search order %s\n",
          FName,
          MBool[rc],
          MOExpSearchOrder[SearchOrder]);

        return(SUCCESS);
        }

      /* expression is not valid range */

      break;
  
    default:

      /* NO-OP */

      break;
    }  /* END switch (MSched.OESearchOrder) */

  if ((Buffer != NULL) && (Buffer[0] != '\0'))
    {
    MUStrCpy(Match,Buffer,sizeof(Match));

    MDB(4,fSTRUCT) MLog("INFO:     checking for expression match with '%s'\n",
      Match);
    }
  else
    {
    Match[0] = '\0';
    }

  if (Buffer != NULL)
    MUSNInit(&BPtr,&BSpace,Buffer,BufSize);
  else
    BPtr = NULL;

  /* truncate at newline */
 
  if ((ptr = strchr(Pattern,'\n')) != NULL)
    *ptr = '\0';

  if ((SearchRegEx == TRUE) && 
      ((OType == mxoRsv) || (OType == mxoJob) || (OType == mxoNode)))
    {
    char *ptr;

    /* replace ',' delimited list with '|' delimited list */

    for (ptr = strchr(Pattern,',');ptr != NULL;ptr = strchr(ptr,','))
      {
      if (ptr[1] == '\0')
        { 
        /* strip off the trailing ',' (causes quirky behavior) */

        ptr[0] = '\0';
        ptr    = NULL;

        break;
        }

      ptr[0] = '|';

      ptr++;      
      }  /* END for (ptr) */
    }
 
  /* Find a slot that either is:
   *   empty
   *   or has a pattern match with argument
   */
  if ((MSched.DisableRegExCaching == FALSE) &&
      (OType == mxoNode) && (SearchRegEx == TRUE))
    {
    for (rxindex = 0;rxindex < MMAX_RXCACHE;rxindex++)
      {
      /* if entry pointer is NULL, end of list - found an open slot */
      if (rxcache[rxindex].Pattern == NULL)
        break; 
 
      if (!strcmp(rxcache[rxindex].Pattern->c_str(),Pattern))
        break;
      }  /* END for (rxindex) */
    }
  else
    {
    /* use the overflow bin and always compile the RE */
    rxindex = MMAX_RXCACHE;
    }

  /* Don't compile the re yet if mxoNode and rxindex is MMAX_RXCACHE because
   * it will be freed and alloc'ed again in the mxoNode loop. */

  if ((SearchRegEx == TRUE) && (rxcache[rxindex].Pattern == NULL) &&
      ((OType != mxoNode) || (rxindex != MMAX_RXCACHE)))
    {
    /* compile new pattern */

    if (__MURECompile(Pattern,&rxcache[rxindex]) == FAILURE)
      {
      if (Buffer != NULL)
        {
        MUSNPrintF(&BPtr,&BSpace,"ERROR:    cannot interpret regular expression '%s'\n",
          Pattern);
        }

      MDB(7,fSCHED) MLog("%s returning %s, failed to compile pattern (%s)\n",
        FName,
        MBool[FAILURE],
        Pattern);
 
      return(FAILURE);
      }
 
    /* NEW entry to be initialized */
    if (rxindex < MMAX_RXCACHE)
      {
      /* create a new mstring with 'Pattern' 
       * only MMAX_RXCACHE entries are ever allocated for mstring
       * entry MMAX_RXCACHE is never allocated mstring, but it's 
       * 're' compiled entry as the overflow 'bin' (when the cache is full
       */
      rxcache[rxindex].Pattern = new mstring_t(Pattern);
      }
    }   /* END if (rxcache[rxindex].Pattern[0] == '\0') */
 

  /* NOW do 'object' specific operations */

  switch (OType)
    {
    case mxoNode: 
      {    
      /* FORMAT:  <HOSTEXP>[{,:+ \t\n}<HOSTEXP>]... */

      nptr = MUStrTok(Pattern,",:+ \t\n",&TokPtr);

      while (nptr != NULL)
        {
        ptr = nptr;

        nptr = MUStrTok(NULL,",:+ \t\n",&TokPtr);

        if (SearchRegEx == TRUE) 
          {
          /* Overflow 'bin' */
          if (rxcache[MMAX_RXCACHE].IsAlloc == TRUE)
            {
            regfree(&rxcache[MMAX_RXCACHE].re);

            rxcache[MMAX_RXCACHE].IsAlloc = FALSE;
            }

          if ((rxindex >= MMAX_RXCACHE) || (rxcache[rxindex].IsAlloc == FALSE))
            {
            if (__MURECompile(ptr,&rxcache[rxindex]) == FAILURE)
              {
              if (Buffer != NULL)
                {
                MUSNPrintF(&BPtr,&BSpace,"ERROR:    cannot interpret regular expression '%s'\n",
                  ptr);
                }

              MDB(7,fSCHED) MLog("%s returning %s - failed to compile node pattern (%s)\n",
                FName,
                MBool[FAILURE],
                ptr);

              return(FAILURE);
              }
            }     /* END if ((rxindex >= MMAX_RXCACHE)...) */

          if ((Match[0] != '\0') && !regexec((regex_t *)&rxcache[rxindex].re,Match,0,NULL,0))
            {
            /* regex match located */
 
            MatchFound = TRUE;

            break;
            }
          }       /* END if (SearchRegEx == TRUE) */

        for (index = 0;index < MSched.M[mxoNode];index++)
          {
          N = MNode[index];
 
          if ((N == NULL) || (N->Name[0] == '\0'))
            break;
 
          if (N->Name[0] == '\1')
            continue;
 
          if ((P != NULL) && (P->Index > 0) && (N->PtIndex != P->Index))
            continue;

          if ((SearchRegEx == FALSE) && 
               strcasecmp(ptr,N->Name) &&
             ((N->FullName == NULL) || strcasecmp(ptr,N->FullName)))
            {
            continue;
            }

          if (SearchRegEx == FALSE)
            {
            /* match already found */

            /* NO-OP */
            }
          else if (!strcasecmp(ptr,N->Name) || 
                 ((N->FullName != NULL) && !strcasecmp(ptr,N->FullName)))
            {
            /* node name match located */

            /* NO-OP */
            }
          else if ((!strcmp(ptr,"ALL") || !strcmp(ptr,ALL)) && 
                 ((N->RM == NULL) ||
                  (bmisclear(&N->RM->RTypes)) ||
                  bmisset(&N->RM->RTypes,mrmrtCompute) ||
                  (N->CRes.Procs > 0)))
            {
            /* 'ALL' expression matches to compute node */

            /* NO-OP */
            }
          else if (!regexec((regex_t *)&rxcache[rxindex].re,N->Name,0,NULL,0) ||
           ((N->FullName != NULL) && !regexec((regex_t *)&rxcache[rxindex].re,N->FullName,0,NULL,0)))
            {
            /* regex match located */
            }
          else
            {
            /* no match found */

            continue;
            }

          /* match located */

          if (Match[0] == '\0')
            {
            MUArrayListAppendPtr(Array,N);

            MatchFound = TRUE;
 
            MDB(3,fSTRUCT) MLog("INFO:     '%s' added to regex list\n",
              N->Name);
 
            if (Buffer != NULL)
              {
              MUSNPrintF(&BPtr,&BSpace,"node '%s' found\n",
                N->Name);
              }
            }
          else if (!strcmp(N->Name,Match))
            {
            MUArrayListAppendPtr(Array,N);

            MatchFound = TRUE;
 
            if (Buffer != NULL)
              {
              MUStrCpy(Buffer,N->Name,BufSize);
              }
 
            break;
            }
          }      /* END for (index = 0;index < MSched.M[mxoNode];index++) */ 
        }        /* END while (ptr != NULL) */
 
      if ((rxindex == MMAX_RXCACHE) && (rxcache[MMAX_RXCACHE].IsAlloc == TRUE))
        {
        /* free memory in temporary cache space */

        regfree(&rxcache[MMAX_RXCACHE].re);

        rxcache[MMAX_RXCACHE].IsAlloc = FALSE;
        }  /* END if (rxindex == MMAX_RXCACHE) */
 
      if ((MatchFound == FALSE) && (Match[0] != '\0'))
        {
        MDB(3,fSTRUCT) MLog("INFO:     no matches found for node expression\n");
 
        if (Buffer != NULL)
          {
          MUSNPrintF(&BPtr,&BSpace,"no matches found for expression\n");
          }
 
        return(FAILURE);
        }
      }    /* END BLOCK mxoNode */
 
      break;
 
    case mxoJob:

      MJobIterInit(&JTI);

      while (MJobTableIterate(&JTI,&J) == SUCCESS)
        {
        MDB(6,fSTRUCT) MLog("INFO:     comparing job '%s' against regex\n",
          J->Name);

        if (DoAlternate == FALSE)
          {
          if (!strcmp(Pattern,"ALL") ||
              !strcmp(Pattern,"^(ALL)$") ||
             !regexec((regex_t *)&rxcache[rxindex].re,J->Name,0,NULL,0))
            {
            if (Match[0] == '\0')
              {
              MDB(3,fSTRUCT) MLog("INFO:     job '%s' added to regex list\n",
                J->Name);
   
              MUArrayListAppendPtr(Array,J);

              MatchFound = TRUE;
   
              if (Buffer != NULL)
                {
                MUSNPrintF(&BPtr,&BSpace,"job '%s' found\n",
                  J->Name);
                }
              }
            else if (!strcmp(J->Name,Match))
              {
              if (Buffer != NULL)
                MUStrCpy(Buffer,J->Name,BufSize);
   
              MUArrayListAppendPtr(Array,J);

              MatchFound = TRUE;
   
              break;
              }
            }  /* END if (!strcmp(Pattern,"ALL") || ... */
          }
        else if (J->AName != NULL)
          {
          if (!strcmp(Pattern,"ALL") ||
              !strcmp(Pattern,"^(ALL)$") ||
             !regexec((regex_t *)&rxcache[rxindex].re,J->AName,0,NULL,0))
            {
            if (Match[0] == '\0')
              {
              MDB(3,fSTRUCT) MLog("INFO:     job '%s' added to regex list\n",
                J->Name);
   
              MUArrayListAppendPtr(Array,J);

              MatchFound = TRUE;
   
              if (Buffer != NULL)
                {
                MUSNPrintF(&BPtr,&BSpace,"job '%s' found\n",
                  J->AName);
                }
              }
            else if (!strcmp(J->AName,Match))
              {
              if (Buffer != NULL)
                MUStrCpy(Buffer,J->AName,BufSize);
   
              MUArrayListAppendPtr(Array,J);

              MatchFound = TRUE;
   
              break;
              }
            }  /* END if (!strcmp(Pattern,"ALL") || ... */
          }    /* END if (J->AName != NULL) */
        }      /* END for (J) */
 
      if ((rxindex == MMAX_RXCACHE) && (rxcache[MMAX_RXCACHE].IsAlloc == TRUE))
        { 
        /* free memory */

        regfree(&rxcache[MMAX_RXCACHE].re);

        rxcache[MMAX_RXCACHE].IsAlloc = FALSE;
        }
 
      if ((MatchFound == FALSE) && (Match[0] != '\0'))
        {
        MDB(3,fSTRUCT) MLog("INFO:     no matches found for job expression\n");
 
        if (Buffer != NULL)
          {
          MUSNPrintF(&BPtr,&BSpace,"no matches found for expression\n");
          }
 
        return(FAILURE);
        }
 
      break;

    case mxoRsv:

      MRsvIterInit(&RTI);

      while (MRsvTableIterate(&RTI,&R) == SUCCESS)
        {
        if ((P != NULL) && (P->Index > 0) && (R->PtIndex != P->Index))
          continue;

        if (Array == NULL)
          {
          MDB(3,fSTRUCT) MLog("ALERT:    regex buffer is full\n");

          break;
          }

        if (!strcmp(Pattern,"ALL") || (regexec((regex_t *)&rxcache[rxindex].re,R->Name,0,NULL,0) == 0) || 
           ((R->Label != NULL) && (regexec((regex_t *)&rxcache[rxindex].re,R->Label,0,NULL,0) == 0)))
          {
          if (Match[0] == '\0')
            {
            MUArrayListAppendPtr(Array,R);

            MatchFound = TRUE;

            MDB(3,fSTRUCT) MLog("INFO:     rsv '%s' added to regex list\n",
              R->Name);

            if (Buffer != NULL)
              {
              MUSNPrintF(&BPtr,&BSpace,"rsv '%s' found\n",
                R->Name);
              }
            }
          else if (!strcmp(R->Name,Match))
            {
            if (Buffer != NULL)
              MUStrCpy(Buffer,R->Name,BufSize);

            MUArrayListAppendPtr(Array,R);

            MatchFound = TRUE;

            break;
            }
          }  /* END if (!strcmp(Pattern,"ALL") || ... */
        }    /* END while (MRsvTableIterate()) */

      if ((rxindex == MMAX_RXCACHE) && (rxcache[MMAX_RXCACHE].IsAlloc == TRUE))
        {
        /* free memory */

        regfree(&rxcache[MMAX_RXCACHE].re);

        rxcache[MMAX_RXCACHE].IsAlloc = FALSE;
        }

      if ((MatchFound == FALSE) && (Match[0] != '\0'))
        {
        MDB(3,fSTRUCT) MLog("INFO:     no matches found for rsv expression\n");

        if (Buffer != NULL)
          {
          MUSNPrintF(&BPtr,&BSpace,"no matches found for expression\n");
          }

        /* don't return failure because Dan doesn't like it for gmoab */

        return(FAILURE);
        }

      break;
 
    default:
 
      MDB(0,fSTRUCT) MLog("ERROR:    unexpected object type '%d' passed to %s()\n",
        OType,
        FName);
 
      if ((rxindex == MMAX_RXCACHE) && (rxcache[MMAX_RXCACHE].IsAlloc == TRUE))
        {
        regfree(&rxcache[MMAX_RXCACHE].re);

        rxcache[MMAX_RXCACHE].IsAlloc = FALSE;
        }
 
      return(FAILURE);

      /*NOTREACHED*/
 
      break; 
    }  /* END switch (OType) */
 
  if ((rxindex == MMAX_RXCACHE) && (rxcache[MMAX_RXCACHE].IsAlloc == TRUE))
    {
    /* free memory */

    regfree(&rxcache[rxindex].re);

    rxcache[MMAX_RXCACHE].IsAlloc = FALSE;
    }
 
  return(SUCCESS);
  }  /* MUREToList() */

/* END MURE.c */
