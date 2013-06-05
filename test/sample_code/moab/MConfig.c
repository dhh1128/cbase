/* HEADER */

/**
 * @file MConfig.c
 *
 * Moab Configuration
 */
        
/* Contains:                                              *
 * int MCfgAdjustBuffer(Buf,AllowExtension,BaseTail,AllowRMDirectives,ReplaceEscape,CutComments) *
 * int MCfgGetVal(Buf,Param,Index,Value,SysTable)          *
 * int MCfgGetIVal(Buf,CurPtr,Param,IName,Index,Value,SymTable) *
 * int MCfgExtractVal(Buf,Param,Index,Attr)               *
 * int MCfgEvalOConfig(OType,SCString,SPName,SAList,Buf,BufSize)   *
 *                                                        */



using namespace std;

#include "moab.h"
#include "moab-proto.h"
#include "moab-global.h"
#include "moab-const.h"
#include "PluginManager.h"
#include "PluginNodeAllocate.h"


/**
 *
 *
 * @param Buf (I)
 */

mbool_t MCfgLineIsEmpty(

  char *Buf)

  {
  int cindex;
  int len;

  if ((Buf == NULL) || (Buf[0] == '\0'))
    {
    return(TRUE);
    }

  len = strlen(Buf);

  for (cindex = 0;cindex < len;cindex++)
    {
    if (isspace(Buf[cindex]))
      continue;

    if (Buf[cindex] == '#')
      break;

    return(FALSE);
    }  /* END for (cindex) */

  return(TRUE);
  }  /* END MCfgLineIsEmpty() */





/**
 * Replace all comments with spaces     
 * Replace tabs with space              
 * Replace '"' with space               
 * Replace unprintable chars with space 
 * Extend '\' lines                     
 *
 * NOTE:  if AllowRMDirectives, also allow #! shell magic
 *        and script-embedded '#' chars 
 *
 * @param Buf (I) Only modified if AllowExtension == TRUE [modified]
 * @param AllowExtension (I) Specifies whether Buf can be extended (realloc'd).
 *        If this is set to TRUE, then Buf must point to memory allocated on the heap
 * @param BaseTail (O) [optional]
 * @param AllowRMDirectives (I)
 * @param ReplaceEscape (I)
 * @param CutComments (I) If TRUE, a comment is replaced with \n\0.
 */

int MCfgAdjustBuffer(

  char     **Buf,                /* I (modified) */
  mbool_t    AllowExtension,     /* I */
  char     **BaseTail,           /* O (optional) */
  mbool_t    AllowRMDirectives,  /* I */
  mbool_t    ReplaceEscape,      /* I */
  mbool_t    CutComments)        /* I */

  {
  char *ptr;
  char *IBuf;

  int   blen;
  int   offset;

  char  IFile[MMAX_LINE];
  char  Params[MMAX_LINE];

  enum MCfgStateEnum { 
    mcbPreParam = 0, 
    mcbOnParam, 
    mcbPreVal, 
    mcbOnVal, 
    mcbComment };

  enum MCfgStateEnum State;

  mbool_t DirectiveFound = FALSE;
  char DirectiveToUse[MMAX_LINE];

  int LineNumber;

  /* replace all comments with spaces     */
  /* replace tabs with space              */
  /* replace '"' with space               */
  /* replace unprintable chars with space */
  /* extend '\' lines                     */

  const char *FName = "MCfgAdjustBuffer";
 
  MDB(3,fCONFIG) MLog("%s(Buf,%s,BaseTail)\n",
    FName,
    (AllowExtension == TRUE) ? "TRUE" : "FALSE");

  if (BaseTail != NULL)
    *BaseTail = NULL;

  if ((Buf == NULL) || (*Buf == NULL) || (strlen(*Buf) < 1))
    {
    return(SUCCESS);
    }

  Params[0] = '\0';
  DirectiveToUse[0] = '\0';

  /* include '.dat' file */

  /* load file */

  ptr = *Buf;

  if (AllowExtension == TRUE)
    {
    char tmpLine[MMAX_LINE];

    sprintf(tmpLine,"%s%s",
      MSched.CfgDir,
      MDEF_DATFILE);

    if (((IBuf = MFULoad(MDEF_DATFILE,1,macmWrite,NULL,NULL,NULL)) != NULL) ||
        ((IBuf = MFULoad(tmpLine,1,macmWrite,NULL,NULL,NULL)) != NULL)) 
      {
      blen = strlen(*Buf);

      if ((IBuf = (char *)realloc(IBuf,blen + 2 + strlen(IBuf))) == NULL)
        {
        /* NOTE:  memory failure */

        MUFree(&IBuf);

        return(FAILURE);
        }

      /* append file */

      strcat(IBuf,"\n");

      strcat(IBuf,*Buf);

      MUFree(Buf);

      *Buf = IBuf;

      ptr = *Buf;
      }  /* END if (IBuf = MFULoad(MDEF_DATFILE) != NULL) */
    }    /* END if (AllowExtension == TRUE) */
 
  State = mcbPreParam;

  IFile[0] = '\0';
  Params[0] = '\0';

  LineNumber = 0;

  while (*ptr != '\0')
    {
    /* remove comments */

    if (*ptr == '#')
      {
      if (AllowExtension == TRUE)
        {
        /* look for include at start of line */

        if (!strncasecmp(ptr,"#INCLUDE",strlen("#INCLUDE")) && 
          ((ptr == *Buf) || (ptr[-1] == '\n')))
          {
          char *tPtr;

          char  Line[MMAX_LINE << 2];

          int   len = strlen("#INCLUDE");

          /* FORMAT:  #INCLUDE <FILENAME> [<args>]*/

          tPtr = strchr(ptr,'\n');

          if (tPtr == NULL)
            {
            MUStrCpy(Line,ptr,sizeof(Line));
            }
          else
            {
            strncpy(Line,ptr + len,tPtr - (ptr + len));

            Line[tPtr - (ptr + len)] = '\0';
            }

          MUSScanF(Line,"%x%s%x%s",
            MMAX_LINE,
            IFile,
            MMAX_LINE,
            Params);

          State = mcbComment;
  
          if (CutComments == TRUE)
            {
            *(ptr++) = '\n';
            *ptr = '\0';
            }
          else
            {
            *ptr = ' ';
            }
          }
        else
          {
          State = mcbComment;

          if (CutComments == TRUE)
            {
            *(ptr++) = '\n';
            *ptr = '\0';
            }
          else
            {
            *ptr = ' ';
            }
          }
        }  /* END if (AllowExtension == TRUE) */
      else if ((AllowRMDirectives == TRUE) &&
               (UserDirectiveSpecified == FALSE) &&
               (MUIsRMDirective(ptr,mrmtPBS) || 
                MUIsRMDirective(ptr,mrmtLL) ||
                (MUIsRMDirective(ptr,mrmtOther) &&
                (LineNumber > 0))))
        {
        /* FIXME: DO NOT ALLOW THIS:   #PBS -l nodes=1 #PBS -l walltime=600 */

        if (DirectiveFound == TRUE)
          {
          if (strncasecmp(ptr,DirectiveToUse,strlen(DirectiveToUse)))
            {
            State = mcbComment;

            if (CutComments == TRUE)
              {
              *(ptr++) = '\n';
              *ptr = '\0';
              }
            else
              {
              *ptr = ' ';
              }
            }
          }
        else
          {
          /* FORMAT:  {DIRECTIVE}[ [<VALUE>]] */

          /* we just came across our first directive--remember it! - why? */

          char *FirstSpace;

          for (FirstSpace = ptr;*FirstSpace != '\0';FirstSpace++)
            {
            if (!isspace(*FirstSpace))
              break;
            }

          if (FirstSpace != NULL)
            {
            /* store the name of the directive */   

            strncpy(DirectiveToUse,ptr,MIN((int)sizeof(DirectiveToUse),(int)(FirstSpace - ptr))); 

            DirectiveFound = TRUE;
            }
          }
        }  /* END else if ((AllowRMDirectives == TRUE) ... ) */
      else if ((AllowRMDirectives == TRUE) &&
               (ptr > *Buf) &&
               (ptr[-1] == '$'))
        {
        /* NOTE: $# is valid for shell scripts */

        /* NO-OP */
        }
      else if ((AllowRMDirectives == TRUE) &&
               (ptr == *Buf) &&
               (ptr[1] == '!') &&
               (LineNumber == 0))
        {
        /* FORMAT:  '#!<PATH>' */

        /* NOTE:   do not remove shell magic if on the first line */

        /* NO-OP */
        } 
      else if ((AllowRMDirectives == TRUE) && 
               (State != mcbComment) &&
               (ptr > *Buf) && 
               (ptr[-1] != '\n'))
        {
        /* NOTE:  do not remove # characters embedded into script */

        /* NO-OP */
        }
      else if ((UserDirective != NULL) &&
               (UserDirectiveSpecified == TRUE) &&
               (AllowRMDirectives == TRUE) &&
               (*ptr != '\n') &&
               (*ptr != '\r'))
        {
        /* If -C argument is present and this line does not match, comment */
          if ((UserDirective[0] == '\0') || 
            strncasecmp(ptr,UserDirective,strlen(UserDirective)))
          {
          State = mcbComment;

          if (CutComments == TRUE)
            {
            *(ptr++) = '\n';
            *ptr = '\0';
            }
          else
            {
            *ptr = ' ';
            }
          }
        else
          {
          DirectiveFound = TRUE;
          }
        }
      else
        {
        State = mcbComment;
  
        if (CutComments == TRUE)
          {
          *(ptr++) = '\n';
          *ptr = '\0';
          }
        else
          {
          *ptr = ' ';
          }
        }
      }  /* END if (*ptr == '#') */
    else if ((*ptr == '\\') && 
             (State != mcbComment) && 
             (ReplaceEscape == TRUE))
      {
      *ptr = ' ';

      while (isspace(*ptr))
        {
        *ptr = ' ';

        ptr++;
        }
      }
    else if ((*ptr == '\n') || (*ptr == '\r'))
      {
      if (*ptr == '\r')
        *ptr = '\n';

      LineNumber++;

      if ((State == mcbComment) && (IFile[0] != '\0'))
        {
        char tmpIFile[MMAX_LINE];

        /* include file at end */

        if (IFile[0] != '/')
          snprintf(tmpIFile,sizeof(tmpIFile),"%s/%s",MSched.CfgDir,IFile);
        else
          MUStrCpy(tmpIFile,IFile,sizeof(tmpIFile));

        /* load file */

        if ((IBuf = MFULoad(tmpIFile,1,macmWrite,NULL,NULL,NULL)) == NULL)
          {
          /* cannot load include file */
          }
        else
          {
          blen = strlen(*Buf);

          offset = ptr - *Buf;

          if ((*Buf = (char *)realloc(*Buf,blen + 2 + strlen(IBuf))) == NULL)
            {
            /* NOTE:  memory failure */

            MUFree(&IBuf);

            return(FAILURE);
            }

#define MMAX_ALLOWEDINCLUDESTRINGS 10

          if (Params[0] != '\0')
            { 
            char *ptr;
            char *TokPtr;

            char *AllowedStrings[MMAX_ALLOWEDINCLUDESTRINGS + 1];

            int   ASIndex; 

            ptr = MUStrTok(Params,",\t\n ",&TokPtr);

            ASIndex = 0;

            while ((ptr != NULL) && (ASIndex < MMAX_ALLOWEDINCLUDESTRINGS))
              {
              if (ptr[0] == '#')
                {
                /* ignore comments after #INCLUDE */

                break;
                }

              AllowedStrings[ASIndex] = ptr;

              ASIndex++;

              ptr = MUStrTok(NULL,",\t\n ",&TokPtr);
              }

            AllowedStrings[ASIndex] = NULL;

            if (ASIndex > 0)
              MCfgPurgeBuffer(IBuf,AllowedStrings);
            }  /* END if (Params[0] != '\0') */

          /* append file */

          strcat(*Buf,"\n");
          strcat(*Buf,IBuf); 

          MUFree(&IBuf);

          ptr = *Buf + offset;
          }  /* END else ((IBuf = MFULoad(IFile,1,macmWrite,NULL,NULL,NULL)) == NULL) */

        IFile[0] = '\0';
        }  /* END if ((State == mcbComment) && (IFile[0] != '\0')) */

      State = mcbPreParam;
      }  /* END else if ((*ptr == '\n') || (*ptr == '\r')) */
    else if ((State == mcbComment) || 
            ((*ptr == '\\') && (ReplaceEscape == TRUE)))
      {
      *ptr = ' ';
      }
    else if (isspace(*ptr) || (*ptr == '='))
      {
      if (*ptr == '=')
        {
        if ((AllowRMDirectives == FALSE) && (State != mcbOnVal))
          *ptr = ' ';
        }
      else
        { 
        if (State == mcbOnParam)
          State = mcbPreVal;

        if (*ptr != '\t')
          *ptr = ' ';
        }
      }
    else if (isprint(*ptr))
      {
      if (State == mcbPreParam)
        {
        State = mcbOnParam;
        }
      else if (State == mcbPreVal)
        {
        State = mcbOnVal;
        }
      } 
    else
      {
      /* remove comment */

      *ptr = ' ';
      }

    ptr++;
    }  /* END while (ptr != '\0') */ 

  if (BaseTail != NULL)
    *BaseTail = ptr;

  /* include '.dat' file */

  /* load file */

  if (AllowExtension == TRUE)
    {
    char tmpLine[MMAX_LINE];

    sprintf(tmpLine,"%s.%s",
      MSched.CfgDir,
      MDEF_DATFILE);

    if ((IBuf = MFULoad(tmpLine,1,macmWrite,NULL,NULL,NULL)) != NULL)
      {
      blen = strlen(*Buf);

      offset = ptr - *Buf;

      if ((*Buf = (char *)realloc(*Buf,blen + 2 + strlen(IBuf))) == NULL)
        {
        /* NOTE:  memory failure */

        MUFree(&IBuf);

        return(FAILURE);
        }

      /* append file */

      strcat(*Buf,"\n");

      MSched.DatStart = *Buf + strlen(*Buf);

      strcat(*Buf,IBuf);

      MUFree(&IBuf);

      ptr = *Buf + offset;
      }  /* END if (IBuf = MFULoad(MDEF_DATFILE) != NULL) */
    }    /* END if (AllowExtension == TRUE) */

  MDB(5,fCONFIG) MLog("INFO:     adjusted config Buffer ------\n%s\n------\n",
    *Buf);

  return(SUCCESS);
  }  /* END MCfgAdjustBuffer() */





/**
 *
 *
 * @param Buf (I) [modified]
 */

int MCfgCompressBuf(

  char *Buf)  /* I (modified) */

  {
  int   blen;
  int   offset;
  int   index;

  mbool_t EraseChar;

  if (Buf == NULL)
    {
    return(FAILURE);
    }

  blen = strlen(Buf);

  offset = 0;

  EraseChar = TRUE;

  for (index = 0;index < blen;index++)
    {
    if ((EraseChar == TRUE) && !strchr(" \t\n",Buf[index]))
      EraseChar = FALSE;     

    if (EraseChar == TRUE)
      {
      offset++;
      }
    else if (offset > 0)
      {
      Buf[index - offset] = Buf[index];
      }
      
    if (Buf[index] == '\n')
      EraseChar = TRUE;
    }  /* END for (index) */

  Buf[index - offset] = '\0';

  return(SUCCESS);
  }  /* END MCfgCompressBuf() */





/**
 * NOTE:  There is also an MCfgGetValMString()
 *
 * @param Buf (I) [modified]
 * @param Param (I)
 * @param IName (O) [minsize=MMAX_NAME]
 * @param Index (I/O) [optional]
 * @param Value (O)
 * @param ValSize (I)
 * @param SymTable (I) [optional]
 */

int MCfgGetVal(

  char  **Buf,       /* I (modified) */
  const char *Param, /* I */
  char   *IName,     /* O (minsize=MMAX_NAME) */
  int    *Index,     /* I/O (optional) */
  char   *Value,     /* O */
  int     ValSize,   /* I */
  char  **SymTable)  /* I (optional) */
   
  {
  char *ptr;
  char *tmp;
  char *head;
  char *tail;

  char  IndexName[MMAX_NAME];

  int   iindex;

  const char *FName = "MCfgGetVal";

  MDB(7,fCONFIG) MLog("%s(Buf,%s,%s,Index,Value,%d,SymTable)\n",
    FName,
    Param,
    (IName != NULL) ? IName : "NULL",
    ValSize);

  /* NOTE:  allow 'double quote' bounded values */

  if (Value != NULL)
    Value[0] = '\0';

  if ((Param == NULL) || (Param[0] == '\0'))
    {
    return(FAILURE);
    }

  memset(IndexName, 0, MMAX_NAME);

  /* FORMAT:  { '\0' || '\n' }[<WS>]<VAR>[<[><WS><INDEX><WS><]>]<WS><VALUE> */

  ptr = *Buf;

  while (ptr != NULL)
    {
    if ((head = strcasestr(ptr,Param)) == NULL)
      {
      /* cannot locate parameter */

      return(FAILURE);
      }

    ptr = head + strlen(Param);

    /* look backwards for newline or start of buffer */

    if (head > *Buf)
      tmp = head - 1;
    else
      tmp = *Buf;

    while ((tmp > *Buf) && ((*tmp == ' ') || (*tmp == '\t')))
      tmp--;

    if ((tmp != *Buf) && (*tmp != '\n'))
      continue;

    if ((IName != NULL) && (IName[0] != '\0'))
      {
      /* requested index name specified */

      if (*ptr != '[')
        continue;

      ptr++;

      while (isspace(*ptr))
        ptr++;

      if (strncmp(IName,ptr,strlen(IName)) != 0)
        continue;

      ptr += strlen(IName);

      while (isspace(*ptr))
        ptr++;
       
      if (*ptr != ']')
        continue; 

      ptr++;

      /* requested index found */
      }
    else if (isspace(*ptr))
      {
      /* no index specified */

      if (Index != NULL)
        *Index = 0;
      }
    else
      {
      /* index specified, no specific index requested */
      char *indexptr;
      int bracketcount = 1;

      if (*ptr != '[')
        continue;

      ptr++;

      head = ptr;
      indexptr = IndexName;

      /* Allow for nested square brackets (i.e. NODECFG[node[01]]) */

      while ((*ptr != '\n') && (*ptr != '\0'))
        {
        if(*ptr == '[')
          bracketcount++;
        else if (*ptr == ']')
          bracketcount--;

        if (bracketcount == 0)
          break;

        /* copy char into the new string */

        if (!isspace(*ptr))
          {
          *indexptr++ = *ptr;
          }
        
        ptr++;
        }

      while (isspace(*ptr))
        ptr++;

      if (*ptr != ']')
        continue;

      ptr++;

      if (Index != NULL)
        {
        *Index = (int)strtol(IndexName,&tail,10);

        if (*tail != '\0')
          {
          /* index is symbolic */
  
          if (SymTable == NULL)
            {
            return(FAILURE);
            }

          for (iindex = 0;SymTable[iindex] != NULL;iindex++)
            {
            if (!strcmp(SymTable[iindex],IndexName))
              {
              *Index = iindex;

              break;
              }
            }

          if (SymTable[iindex] == NULL)
            {
            MUStrDup(&SymTable[iindex],IndexName);

            *Index = iindex;
            }
          }
        }
      }    /* END else ... */

    /* matching string located */

    break;
    }  /* END while(ptr != NULL) */

  if (ptr == NULL)
    {
    /* parameter not located */

    return(FAILURE);
    }

  /* process value */
 
  /* FORMAT:  <ATTR>{= | <WS>}["]<VAL>["] */
  /*                ^                     */

  if (*ptr == '=')
    ptr++;
 
  while ((*ptr == ' ') || (*ptr == '\t'))
    ptr++;

  if (*ptr == '"')
    {
    ptr++;

    if ((tail = strchr(ptr,'"')) == NULL)
      tail = *Buf + strlen(*Buf);
    }
  else
    {  
    if ((tail = strchr(ptr,'\n')) == NULL)
      tail = *Buf + strlen(*Buf);
    }
 
  MUStrCpy(Value,ptr,MIN(tail - ptr + 1,ValSize));

  if ((tail - ptr) < ValSize)
    Value[tail - ptr] = '\0';
  else
    Value[ValSize] = '\0';

  /* update indexname */

  if ((IName != NULL) && (IName[0] == '\0'))
    MUStrCpy(IName,IndexName,MMAX_NAME);

  *Buf = tail;

  return(SUCCESS);
  }  /* END MCfgGetVal() */


/**
 *
 *
 * @param Buf (I) [modified]
 * @param Param (I)
 * @param IName (O) [minsize=MMAX_NAME]
 * @param Index (I/O) [optional]
 * @param Value (O)
 * @param SymTable (I) [optional]
 */

int MCfgGetValMString(

  char     **Buf,       /* I (modified) */
  const char *Param,    /* I */
  char      *IName,     /* O (minsize=MMAX_NAME) */
  int       *Index,     /* I/O (optional) */
  mstring_t *Value,     /* O */
  char     **SymTable)  /* I (optional) */
   
  {
  char *ptr;
  char *tmp;
  char *head;
  char *tail;

  char  IndexName[MMAX_NAME];

  int   iindex;

  const char *FName = "MCfgGetVal";

  MDB(7,fCONFIG) MLog("%s(Buf,%s,%s,Index,Value,SymTable)\n",
    FName,
    Param,
    (IName != NULL) ? IName : "NULL");

  /* NOTE:  allow 'double quote' bounded values */

  if (Value != NULL)
    {
    Value->clear();
    }

  if ((Param == NULL) || (Param[0] == '\0'))
    {
    return(FAILURE);
    }

  IndexName[0] = '\0';

  /* FORMAT:  { '\0' || '\n' }[<WS>]<VAR>[<[><WS><INDEX><WS><]>]<WS><VALUE> */

  ptr = *Buf;

  while (ptr != NULL)
    {
#ifdef MYAHOO  /* remove from Yahoo after we are sure there are no negative side-effects */
    if ((head = MUStrStr(ptr,(char *)Param,0,TRUE,FALSE)) == NULL)
#else
    if ((head = strstr(ptr,Param)) == NULL)
#endif /* MYAHOO */
      {
      /* cannot locate parameter */

      return(FAILURE);
      }

    ptr = head + strlen(Param);

    /* look backwards for newline or start of buffer */

    if (head > *Buf)
      tmp = head - 1;
    else
      tmp = *Buf;

    while ((tmp > *Buf) && ((*tmp == ' ') || (*tmp == '\t')))
      tmp--;

    if ((tmp != *Buf) && (*tmp != '\n'))
      continue;

    if ((IName != NULL) && (IName[0] != '\0'))
      {
      /* requested index name specified */

      if (*ptr != '[')
        continue;

      ptr++;

      while (isspace(*ptr))
        ptr++;

      if (strncmp(IName,ptr,strlen(IName)) != 0)
        continue;

      ptr += strlen(IName);

      while (isspace(*ptr))
        ptr++;
       
      if (*ptr != ']')
        continue; 

      ptr++;

      /* requested index found */
      }
    else if (isspace(*ptr))
      {
      /* no index specified */

      if (Index != NULL)
        *Index = 0;
      }
    else
      {
      /* index specified, no specific index requested */

      if (*ptr != '[')
        continue;

      ptr++;

      while (isspace(*ptr))
        ptr++;

      head = ptr;

      while ((!isspace(*ptr)) && (*ptr != ']'))
        ptr++;

      MUStrCpy(IndexName,head,MIN(ptr - head + 1,MMAX_NAME));

      while (isspace(*ptr))
        ptr++;

      if (*ptr != ']')
        continue;

      ptr++;

      if (Index != NULL)
        {
        *Index = (int)strtol(IndexName,&tail,10);

        if (*tail != '\0')
          {
          /* index is symbolic */
  
          if (SymTable == NULL)
            {
            return(FAILURE);
            }

          for (iindex = 0;SymTable[iindex] != NULL;iindex++)
            {
            if (!strcmp(SymTable[iindex],IndexName))
              {
              *Index = iindex;

              break;
              }
            }

          if (SymTable[iindex] == NULL)
            {
            MUStrDup(&SymTable[iindex],IndexName);

            *Index = iindex;
            }
          }
        }
      }    /* END else ... */

    /* matching string located */

    break;
    }  /* END while(ptr != NULL) */

  if (ptr == NULL)
    {
    /* parameter not located */

    return(FAILURE);
    }

  /* process value */
 
  /* FORMAT:  <ATTR>{= | <WS>}["]<VAL>["] */
  /*                ^                     */

  if (*ptr == '=')
    ptr++;
 
  while ((*ptr == ' ') || (*ptr == '\t'))
    ptr++;

  if (*ptr == '"')
    {
    ptr++;

    if ((tail = strchr(ptr,'"')) == NULL)
      tail = *Buf + strlen(*Buf);
    }
  else
    {  
    if ((tail = strchr(ptr,'\n')) == NULL)
      tail = *Buf + strlen(*Buf);
    }

  if (Value != NULL)
    {
    /* MString doesn't have a way to add N characters yet, so we have to terminate the string */

    char tmpChar = *tail;
    *tail = '\0';
 
    MStringSet(Value,ptr);
    MStringAppend(Value,"\0");

    *tail = tmpChar;
    }

  /* update indexname */

  if ((IName != NULL) && (IName[0] == '\0'))
    MUStrCpy(IName,IndexName,MMAX_NAME);

  *Buf = tail;

  return(SUCCESS);
  }  /* END MCfgGetValMString() */



 
/**
 *
 *
 * @param Buf (I)
 * @param CurPtr (I) [modified]
 * @param Param (I)
 * @param IndexName (I)
 * @param Index (I)
 * @param Value (O)
 * @param SymTable (I) [optional]
 */

int MCfgGetIVal(

  char  *Buf,        /* I */
  char **CurPtr,     /* I (modified) */
  const char *Param, /* I */
  char  *IndexName,  /* I */
  int   *Index,      /* I */
  int   *Value,      /* O */
  char **SymTable)   /* I (optional) */

  {
  char  ValLine[MMAX_LINE];
  char *ptr;

  int   rc;

  const char *FName = "MCfgGetIVal";

  MDB(7,fCONFIG) MLog("%s(Buf,CurPtr,%s,%s,Index,Value,SymTable)\n",
    FName,
    Param,
    (IndexName != NULL) ? IndexName : "NULL");

  ptr = Buf;

  if (CurPtr != NULL)
    ptr = MAX(ptr,*CurPtr);

  rc = MCfgGetVal(
    &ptr,
    Param,
    IndexName,
    Index,
    ValLine,
    sizeof(ValLine),
    SymTable);

  if (CurPtr != NULL)
    *CurPtr = ptr;

  if (rc == FAILURE)
    {
    return(FAILURE);
    }

  *Value = (int)strtol(ValLine,NULL,0);

  MDB(4,fCONFIG) MLog("INFO:     %s[%s] set to %d\n",
    Param,
    (IndexName != NULL) ? IndexName : "",
    *Value);

  return(SUCCESS);
  }  /* END MCfgGetIVal() */





/**
 * Load string value/indexname for config parameter.
 *
 * NOTE:  There is also an MCfgGetSValMString()
 *
 * @see MCfgEvalOConfig() - parent
 *
 * @param Buf (I)
 * @param CurPtr (I/O) [optional] pointer to last processed character
 * @param Param (I)
 * @param IndexName (I/O) [minsize=MMAX_NAME]
 * @param Index (O) [optional]
 * @param Value (O)
 * @param ValSize (I)
 * @param Mode (I) [if 1, only process first whitespace delimited value]
 * @param SymTable (I) [optional,not used]
 */

int MCfgGetSVal(

  char       *Buf,
  char      **CurPtr,
  const char *Param,
  char       *IndexName,
  int        *Index,
  char       *Value,
  int         ValSize,
  int         Mode,
  char      **SymTable)

  {
  char *ptr;

  int   rc;
  int   index;

  const char *FName = "MCfgGetSVal";

  MDB(7,fCONFIG) MLog("%s(Buf,CurPtr,%s,%s,Index,Value,%d,%d,SymTable)\n",
    FName,
    Param,
    (IndexName != NULL) ? IndexName : "NULL",
    ValSize,
    Mode);

  if (Value == NULL)
    {
    return(FAILURE);
    }

  Value[0] = '\0';

  if (Param == NULL)
    {
    return(FAILURE);
    }

  ptr = Buf;

  if (CurPtr != NULL)
    ptr = MAX(ptr,*CurPtr);

  rc = MCfgGetVal(&ptr,Param,IndexName,Index,Value,ValSize,SymTable);

  if (CurPtr != NULL)
    *CurPtr = ptr;

  if (rc == FAILURE)
    {
    return(FAILURE);
    }

  if (Mode == 1)
    {
    /* process only first white space delimited string */

    for (index = 0;Value[index] != '\0';index++)
      {
      if (isspace(Value[index]))
        {
        Value[index] = '\0';

        break;
        }
      }
    }
  else
    {
    /* remove trailing whitespace */
 
    for (index = strlen(Value) - 1;index > 0;index--)
      {
      if (isspace(Value[index]))
        Value[index] = '\0';
      else
        break;
      }  /* END for (index) */
    }

  if ((IndexName != NULL) && (IndexName[0] != '\0'))
    {
    MDB(4,fCONFIG) MLog("INFO:     %s[%s] set to %s\n",
      Param,
      (IndexName != NULL) ? IndexName : "NULL",
      Value);
    }
  else
    {
    MDB(4,fCONFIG) MLog("INFO:     %s set to %s\n",
      Param,
      Value);
    }

  return(SUCCESS);
  }  /* END MCfgGetSVal() */


/**
 * Load string value/indexname for config parameter into an mstring_t.
 *
 * @see MCfgEvalOConfig() - parent
 *
 * @param Buf (I)
 * @param CurPtr (I/O) [optional] pointer to last processed character
 * @param Param (I)
 * @param IndexName (I/O) [minsize=MMAX_NAME]
 * @param Index (O) [optional]
 * @param Value (O)
 * @param Mode (I) [if 1, only process first whitespace delimited value]
 * @param SymTable (I) [optional,not used]
 */

int MCfgGetSValMString(

  char   *Buf,       /* I */
  char  **CurPtr,    /* I/O (optional) */
  const char *Param, /* I */
  char   *IndexName, /* I/O (minsize=MMAX_NAME) */
  int    *Index,     /* O (optional) */
  mstring_t *Value,  /* O */
  int     Mode,      /* I (if 1, only process first whitespace delimited value) */
  char  **SymTable)  /* I (optional,not used) */

  {
  char *ptr;

  int   rc;
  int   index;

  const char *FName = "MCfgGetSVal";

  MDB(7,fCONFIG) MLog("%s(Buf,CurPtr,%s,%s,Index,Value,%d,SymTable)\n",
    FName,
    Param,
    (IndexName != NULL) ? IndexName : "NULL",
    Mode);

  /* MCfgGetValMString will take care of initializing Value */

  if (Param == NULL)
    {
    return(FAILURE);
    }

  ptr = Buf;

  if (CurPtr != NULL)
    ptr = MAX(ptr,*CurPtr);

  rc = MCfgGetValMString(&ptr,Param,IndexName,Index,Value,SymTable);

  if (CurPtr != NULL)
    *CurPtr = ptr;

  if (rc == FAILURE)
    {
    return(FAILURE);
    }

  if (Mode == 1)
    {
    /* process only first white space delimited string */

    for (index = 0;(*Value)[index] != '\0';index++)
      {
      if (isspace((*Value)[index]))
        {
        MStringStripFromIndex(Value,index);

        break;
        }
      }
    }
  else
    {
    /* remove trailing whitespace */
 
    for (index = strlen(Value->c_str()) - 1;index > 0;index--)
      {
      if (isspace((*Value)[index]))
        MStringStripFromIndex(Value,-1);
      else
        break;
      }  /* END for (index) */
    }

  if ((IndexName != NULL) && (IndexName[0] != '\0'))
    {
    MDB(4,fCONFIG) MLog("INFO:     %s[%s] set to %s\n",
      Param,
      (IndexName != NULL) ? IndexName : "NULL",
      Value->c_str());
    }
  else
    {
    MDB(4,fCONFIG) MLog("INFO:     %s set to %s\n",
      Param,
      Value->c_str());
    }

  return(SUCCESS);
  }  /* END MCfgGetSValMString() */





/**
 *
 *
 * @param DoCheckPolicy (I)
 */

int MCfgEnforceConstraints(

  mbool_t DoCheckPolicy)  /* I */

  {
  int        pindex;

  enum MXMLOTypeEnum OList[] = { 
    mxoUser, 
    mxoGroup, 
    mxoAcct, 
    mxoClass, 
    mxoQOS, 
    mxoNONE };

  int        oindex;

  mpu_t     *AP;
  mpu_t     *IP;
  mpu_t     *JP;

  int        PtIndex = 0;

  msched_t  *S;

  const char *FName = "MCfgEnforceConstraints";

  MDB(4,fCONFIG) MLog("%s(%s)\n",
    FName,
    MBool[DoCheckPolicy]);

  if (DoCheckPolicy == TRUE)
    {
    for (oindex = 0;OList[oindex] != mxoNONE;oindex++)
      {
      AP = NULL;
      IP = NULL;

      switch (OList[oindex])
        {
        case mxoUser:

          if (MSched.DefaultU != NULL)
            {
            AP = &MSched.DefaultU->L.ActivePolicy;
            IP = MSched.DefaultU->L.IdlePolicy;
            }

          break;

        case mxoGroup:

          if (MSched.DefaultG != NULL)
            {
            AP = &MSched.DefaultG->L.ActivePolicy;
            IP = MSched.DefaultG->L.IdlePolicy;
            }

          break;

        case mxoAcct:

          if (MSched.DefaultA != NULL)
            {
            AP = &MSched.DefaultA->L.ActivePolicy;
            IP = MSched.DefaultA->L.IdlePolicy;
            }

          break;

        default:

          /* NOT HANDLED */

          break;
        }  /* END switch (OList[oindex]) */

      if (AP == NULL)
        continue;

      for (pindex = 0;pindex < mptLAST;pindex++)
        {
        if (AP != NULL)
          {
          if (AP->SLimit[pindex][PtIndex] == 0)
            {
            /* set soft limits if only hard limit specified */

            AP->SLimit[pindex][PtIndex] = AP->HLimit[pindex][PtIndex];
            }
          else if ((AP->SLimit[pindex][PtIndex] > 0)  &&
                  ((AP->HLimit[pindex][PtIndex] < AP->SLimit[pindex][PtIndex]) ||
                   (AP->HLimit[pindex][PtIndex] <= 0)))
            {
            /* set hard limits if only soft limit specified */

            AP->HLimit[pindex][PtIndex] = AP->SLimit[pindex][PtIndex];
            }
          }

        if (IP != NULL)
          {
          if (IP->SLimit[pindex][PtIndex] == 0)
            {
            IP->SLimit[pindex][PtIndex] = IP->HLimit[pindex][PtIndex];
            }
          else if ((IP->SLimit[pindex][PtIndex] > 0)  &&
                  ((IP->HLimit[pindex][PtIndex] < IP->SLimit[pindex][PtIndex]) ||
                   (IP->HLimit[pindex][PtIndex] <= 0)))
            {
            IP->HLimit[pindex][PtIndex] = IP->SLimit[pindex][PtIndex];
            }
          }
        }    /* END for (pindex) */
      }      /* END for (oindex) */

    PtIndex = 0;

    JP      = MPar[PtIndex].L.JobPolicy;

    for (pindex = 0;pindex < mptLAST;pindex++)
      {
      if (JP != NULL)
        {
        if (JP->SLimit[pindex][PtIndex] == 0)
          {
          JP->SLimit[pindex][PtIndex] = JP->HLimit[pindex][PtIndex];
          }
        else if ((JP->SLimit[pindex][PtIndex] > 0)  &&
                ((JP->HLimit[pindex][PtIndex] < JP->SLimit[pindex][PtIndex]) ||
                 (JP->HLimit[pindex][PtIndex] <= 0)))
          {
          /* set hard limit if only soft limit specified */

          JP->HLimit[pindex][PtIndex] = JP->SLimit[pindex][PtIndex];
          }
        }
      }    /* END for (pindex) */
    }      /* END if (DoCheckPolicy == TRUE) */
  else
    {
    if (MSched.MaxJobCounter < MSched.MinJobCounter)
      {

      MDB(3,fCONFIG) MLog("ERROR:    Min Job ID '%d' must be less than Max Job ID '%d'\n",
        MSched.MinJobCounter,
        MSched.MaxJobCounter);

      MSched.MaxJobCounter = 0;
      MSched.MaxJobCounter = MMAX_INT;
      }
    }     /* END else (DoCheckPolicy == FALSE) */  

  /* verify server status */

  S = &MSched;

  if (S->IsServer == TRUE)
    {
    if (S->ServerHost[0] == '\0')
      {
      MDB(0,fCONFIG) MLog("ERROR:    attribute '%s' of parameter %s must be specified\n",
        MSchedAttr[msaServer],
        MParam[mcoSchedCfg]);

      fprintf(stderr,"ERROR:    attribute '%s' of parameter %s must be specified\n",
        MSchedAttr[msaServer],
        MParam[mcoSchedCfg]);

      exit(1);
      }
    }

  if (MOSHostIsLocal(S->ServerHost,FALSE) == TRUE)
    {
    MDB(2,fCONFIG) MLog("INFO:     server started on host '%s' (Primary Server)\n",
      S->ServerHost);
    }
  else if (MOSHostIsLocal(S->AltServerHost,FALSE) == TRUE)
    {
    MDB(2,fCONFIG) MLog("INFO:     server started on host '%s' (Fallback Server)\n",
      S->AltServerHost);
    }
  else if (S->IsServer == TRUE)
    {
    if (S->AltServerHost[0] != '\0')
      {
      MDB(2,fCONFIG) MLog("INFO:     server must be started on host '%s' or '%s' (currently on '%s')\n",
        S->ServerHost,
        S->AltServerHost,
        S->LocalHost);

      fprintf(stderr,"ERROR:    server must be started on host '%s' or '%s' (currently on '%s') - see SCHEDCFG parameter\n",
        S->ServerHost,
        S->AltServerHost,
        S->LocalHost);
      }
    else
      {
      MDB(2,fCONFIG) MLog("INFO:     server must be started on host '%s' (currently on '%s')\n",
        S->ServerHost,
        S->LocalHost);

      fprintf(stderr,"ERROR:    server must be started on host '%s' (currently on '%s') - see SCHEDCFG parameter\n",
        S->ServerHost,
        S->LocalHost);
      }

    exit(1);
    }

  return(SUCCESS);
  }  /* END MCfgEnforceConstraints() */




/**
 * Process single line from Moab config file for specified parameter.
 *
 * @see MCfgProcessBuffer() - parent
 *
 * @param CIndex    (I)
 * @param IndexName (I) [optional]
 * @param PPtr      (I) [optional]
 * @param Line      (I)
 * @param EvalOnly  (I)
 * @param Msg       (O) [optional,minsize=MMAX_LINE]
 */

int MCfgProcessLine(

  int      CIndex,
  char    *IndexName,
  mpar_t  *PPtr,
  char    *Line,
  mbool_t  EvalOnly,
  char    *Msg)

  {
  int    IVal;
  double DVal;
  char  *SVal;
  char  *SArray[MMAX_CONFIGATTR];

  char  *ptr;

  int    MIndex;

  char  *TokPtr = NULL;

  enum MCfgParamEnum PIndex;

  mpar_t      *P  = NULL;
  mam_t       *A  = NULL;
  mrm_t       *R  = NULL;
  msrsv_t     *SR = NULL;
  mqos_t      *Q  = NULL;

  mbool_t      IsInvalid = FALSE;

  const char *FName = "MCfgProcessLine";

  MDB(3,fCONFIG) MLog("%s(%s,%s,%s,%s,Msg)\n",
    FName,
    MParam[MCfg[CIndex].PIndex],
    (IndexName != NULL) ? IndexName : "NULL",
    Line,
    MBool[EvalOnly]);

  if (Msg != NULL)
    Msg[0] = '\0';

  PIndex = MCfg[CIndex].PIndex;

  /* set defaults */

  if (PPtr != NULL)
    {
    P = PPtr;
    }
  else
    {
    P = &MPar[0];
    }

  /* load parameter index */

  switch (MCfg[CIndex].OType)
    {
    case mxoSRsv:

      /* verify SR exists */

      /* NOTE: no default SR */

      if ((EvalOnly == TRUE) && (MSRFind(IndexName,&SR,NULL) == FAILURE))
        {
        MDB(2,fCONFIG) MLog("ALERT:    cannot configure SR[%s]\n",
          IndexName);

        return(FAILURE);
        }

      if (MSRAdd(IndexName,&SR,NULL) == FAILURE)
        {
        MDB(2,fCONFIG) MLog("ALERT:    cannot configure SR[%s]\n",
          IndexName);

        return(FAILURE);
        }

      break;

    case mxoPar:

      if (PPtr != NULL)
        {
        /* NO-OP */
        }
      else if ((IndexName == NULL) || 
               (IndexName[0] == '\0'))
        {
        /* default index selects global partition */

        P = &MPar[0];
        }
      else if ((EvalOnly == TRUE) && (MParFind(IndexName,&P) == FAILURE))
        {
        MDB(2,fCONFIG) MLog("ALERT:    cannot configure MPar[%s]\n",
          IndexName);

        return(FAILURE);
        }
      else if (MParAdd(IndexName,&P) == FAILURE)
        {
        MDB(2,fCONFIG) MLog("ALERT:    cannot configure MPar[%s]\n",
          IndexName);

        return(FAILURE);
        }

      break;

    case mxoQOS:

      /* NOTE: no default QOS */

      if ((EvalOnly == TRUE) && (MQOSFind(IndexName,&Q) == FAILURE))
        {
        MDB(2,fCONFIG) MLog("ALERT:    cannot configure MQOS[%s]\n",
          IndexName);

        return(FAILURE);
        }

      if ((IndexName != NULL) &&
          strcmp(IndexName,"DEFAULT") &&
          strcmp(IndexName,DEFAULT))
        {
        if (MQOSAdd(IndexName,&Q) == FAILURE)
          {
          MDB(2,fCONFIG) MLog("ALERT:    cannot configure MQOS[%s]\n",
            IndexName);

          return(FAILURE);
          }
        }
      else
        {
        MOGetObject(mxoQOS,"DEFAULT",(void **)&Q,mVerify);
        }

      break;

    case mxoAM:

      if (MAM[0].Name[0] == '\0')
        {
        /* NOTE:  base config only enables primary AM */

        if (MAMAdd(IndexName,&A) == FAILURE)
          {
          MDB(2,fCONFIG) MLog("ALERT:    cannot configure MAM[%s]\n",
            IndexName);

          return(FAILURE);
          }
        }

      break;

    case mxoRM:

      if ((IndexName != NULL) &&
          (IndexName[0] == '\0'))
        {
        /* default index selects base RM */

        R = &MRM[0];
        }
      else if ((EvalOnly == TRUE) && (MRMFind(IndexName,&R) == FAILURE))
        {
        MDB(2,fCONFIG) MLog("ALERT:    cannot configure MRM[%s]\n",
          IndexName);

        return(FAILURE);
        }

      if (MRMAdd(IndexName,&R) == FAILURE)
        {
        MDB(2,fCONFIG) MLog("ALERT:    cannot configure MRM[%s]\n",
          IndexName);

        return(FAILURE);
        }

      MRMSetDefaults(R);

      MOLoadPvtConfig((void **)R,mxoRM,NULL,NULL,NULL,NULL);

      break;

    default:

      /* NO-OP */

      break;
    }  /* END switch (MCfg[CIndex].OType) */

  /* initialize values */

  IVal   = 0;
  DVal   = 0.0;
  SVal   = NULL;
  MIndex = 0;

  SArray[0] = NULL;

  /* read config values */

  switch (MCfg[CIndex].Format)
    {
    case mdfStringArray:

      /* process string array */

      /* check for first string value */

      if ((SArray[MIndex] = MUStrTok(Line," \t\n",&TokPtr)) != NULL)
        {
        MDB(7,fCONFIG) MLog("INFO:     adding value[%d] '%s' to string array\n",
          MIndex,
          SArray[MIndex]);

        MIndex++;

        /* load remaining string values */

        while ((SArray[MIndex] = MUStrTok(NULL," \t\n",&TokPtr)) != NULL)
          {
          MDB(7,fCONFIG) MLog("INFO:     adding value[%d] '%s' to string array\n",
            MIndex,
            SArray[MIndex]);

          MIndex++;
          }
        }
      else
        {
        /* no values located */

        MDB(1,fCONFIG) MLog("WARNING:  parameter '%s[%s]' is not assigned a value.  using default value\n",
          MParam[PIndex],
          IndexName);

        if (Msg != NULL)
          {
          strcpy(Msg,"parameter value is not specified");
          }

        if (EvalOnly == TRUE)
          {
          return(SUCCESS);
          }

        IsInvalid = TRUE;
        }

      break;

    case mdfIntArray:

      /* extract integer array */

      /* NYI */

      break;

    case mdfInt:

      MDB(7,fCONFIG) MLog("INFO:     parsing values for integer array parameter '%s'\n",
        MParam[PIndex]);

      if ((ptr = MUStrTok(Line," \t\n",&TokPtr)) == NULL)
        {
        MDB(1,fCONFIG) MLog("WARNING:  parameter '%s' is not assigned a value (using default value)\n",
          MParam[PIndex]);

        if (Msg != NULL)
          {
          strcpy(Msg,"parameter value is not in required format (integer)");
          }

        if (EvalOnly == TRUE)
          {
          return(SUCCESS);
          }

        IsInvalid = TRUE;
        }
      else
        {
        IVal = (int)strtol(ptr,NULL,0);

        if ((IVal == 0) && (ptr[0] != '0'))
          {
          MDB(1,fCONFIG) MLog("WARNING:  parameter[%u] '%s' value (%s) cannot be read as an integer  (using default value)\n",
            PIndex,
            MParam[PIndex],
            ptr);

          if (Msg != NULL)
            {
            strcpy(Msg,"parameter value is not in required format (integer)");
            }

          if (EvalOnly == TRUE)
            {
            return(SUCCESS);
            }

          IsInvalid = TRUE;
          }
        }

      break;

    case mdfDouble:

      /* extract double array */

      if ((ptr = MUStrTok(Line," \t\n",&TokPtr)) == NULL)
        {
        MDB(1,fCONFIG) MLog("WARNING:  parameter '%s' is not assigned a value  (using default value)\n",
          MParam[PIndex]);

        if (Msg != NULL)
          {
          strcpy(Msg,"parameter value is not in required format (double)");
          }

        if (EvalOnly == TRUE)
          {
          return(SUCCESS);
          }

        IsInvalid = TRUE;
        }
      else
        {
        DVal = strtod(ptr,NULL);

        if ((DVal == 0.0) && (ptr[0] != '0'))
          {
          MDB(1,fCONFIG) MLog("WARNING:  Parameter[%u] '%s' value (%s) cannot be read as a double  (using default value)\n",
            PIndex,
            MParam[PIndex],
            ptr);

          if (Msg != NULL)
            {
            strcpy(Msg,"parameter value is not in required format (double)");
            }

          if (EvalOnly == TRUE)
            {
            return(SUCCESS);
            }

          IsInvalid = TRUE;
          }
        }

      break;

    case mdfDoubleArray:

      /* NYI */

      break;

    case mdfString:

      /* process string value */

      MDB(7,fCONFIG) MLog("INFO:     parsing value for single string parameter '%s'\n",
        MParam[PIndex]);

      /* changed to MUStrTokEPlus() 11/21/07 by DRW to correctly parse user specified error messages */

      if ((SVal = MUStrTokEPlus(Line," \t\n",&TokPtr)) == NULL)
        {
        MDB(1,fCONFIG) MLog("WARNING:  NULL value specified for parameter[%u] '%s'\n",
          PIndex,
          MParam[PIndex]);

        IsInvalid = TRUE;
        }

      break;

    default:

      MDB(0,fCONFIG) MLog("ERROR:    parameter[%u] '%s' not handled\n",
        PIndex,
        MParam[PIndex]);

      if (EvalOnly == TRUE)
        {
        if (Msg != NULL)
          {
          strcpy(Msg,"parameter not handled");
          }

        return(SUCCESS);
        }

      break;
    }  /* END switch (PIndex) */

  if (IsInvalid == TRUE) 
    {
    /* invalid parameter specified */

    MDB(0,fCONFIG) MLog("ALERT:    parameter '%s' has invalid value\n",
      MCfg[CIndex].Name);

    if (Msg != NULL)
      {
      strcpy(Msg,"parameter has invalid value");
      }

    if (EvalOnly == TRUE)
      {
      return(SUCCESS);
      }

    return(SUCCESS);
    }

  /* assign values to parameters */

  switch (MCfg[CIndex].OType)
    {
    case mxoSRsv:

      /* NOTE:  all SR attributes processed via SRCFG */

      /* NO-OP */

      break;

    case mxoAM:

      /* NOTE:  all AM attributes processed via AMCFG */

      /* NO-OP */

      break;

    case mxoRM:

      /* NOTE:  all RM attributes processed via RMCFG */

      /* NO-OP */

      break;

    case mxoQOS:

      /* NOTE:  all QOS attributes processed via QOSCFG */

      /* NO-OP */

      break;

    case mxoxSim:

      if (EvalOnly == FALSE)
        MSimProcessOConfig(&MSim,PIndex,IVal,DVal,SVal,SArray);

      break;

    case mxoPar:
    case mxoSched:
    default:
   
      MCfgSetVal(
        PIndex,
        IVal,
        DVal,
        SVal,
        SArray,
        P,
        IndexName,
        EvalOnly,
        Msg);     /* O */

      break;
    }  /* END switch (MCfg[CIndex].Type) */

  if (EvalOnly == TRUE)
    {
    /* NOTE:  currently, EvalOnly not processed within object processconfig routines */

    return(SUCCESS);
    }

  /* log parameter setting */

  switch (MCfg[CIndex].Format)
    {
    case mdfInt:

      MDB(4,fCONFIG) MLog("INFO:     parameter '%s' assigned int value %d\n",
        MParam[PIndex],
        IVal);

      break;

    case mdfDouble:

      MDB(4,fCONFIG) MLog("INFO:     parameter '%s' assigned int value %f\n",
        MParam[PIndex],
        DVal);

      break;

    case mdfString:

      MDB(4,fCONFIG) MLog("INFO:     parameter '%s' assigned string value '%s'\n",
        MParam[PIndex],
        SVal);

      break;

    case mdfStringArray:

      {
      char tmpLine[MMAX_LINE];

      int  index;

      tmpLine[0] = '\0';

      for (index = 0;index < MIndex;index++)
        {
        MUStrCat(tmpLine,SArray[index],sizeof(tmpLine));
        MUStrCat(tmpLine," ",sizeof(tmpLine));
        }  /* END for (index) */

      MDB(4,fCONFIG) MLog("INFO:     parameter '%s' assigned string array '%s'\n",
        MParam[PIndex],
        tmpLine);
      }  /* END BLOCK */

      break;

    default:

      /* NO-OP */

      break;
    }  /* END switch (MCfg[CIndex].Format) */

  return(SUCCESS);
  }  /* END MCfgProcessLine() */






/**
 * Process Moab config buffer extracting and processing individual parameters.
 *
 * NOTE:  only handles old-style parameters
 *
 * @see MCfgProcessLine() - child
 *
 * @param Buffer (I)
 */

int MCfgProcessBuffer(
 
  char *Buffer)  /* I */
 
  {
  char  *ptr;
 
  char   PVal[MMAX_BUFFER + 1];
  char   IName[MMAX_NAME];
 
  int    cindex;
 
  enum MCfgParamEnum CIndex;

  const char *FName = "MCfgProcessBuffer";
 
  MDB(5,fCONFIG) MLog("%s(%s)\n",
    FName,
    Buffer);
 
  if ((Buffer == NULL) || (Buffer[0] == '\0'))
    {
    return(FAILURE);
    }
 
  /* look for all defined parameters in buffer */
 
  for (cindex = 1;MCfg[cindex].Name != NULL;cindex++)
    {
    MDB(5,fCONFIG) MLog("INFO:     checking parameter '%s'\n",
      MCfg[cindex].Name);

    CIndex = (enum MCfgParamEnum)cindex;

    /* NOTE:  process all parameters located */

    ptr = Buffer;

    while (MCfgGetParameter(
         ptr,
         MCfg[cindex].Name,
         &CIndex,  /* O */
         IName,    /* O */
         PVal,     /* O */
         sizeof(PVal),
         FALSE,
         &ptr) == SUCCESS)
      {
      MDB(7,fCONFIG) MLog("INFO:     value for parameter '%s': '%s'\n",
        MCfg[CIndex].Name,
        PVal);

      MCfgProcessLine(CIndex,IName,NULL,PVal,FALSE,NULL);

      IName[0] = '\0';
      }  /* END while (MCfgGetParameter() == SUCCESS) */
    }    /* END for (cindex)  */
 
  return(SUCCESS);
  }  /* END MCfgProcessBuffer() */





/**
 * Translate deprecated parameters into current equivalents.
 *
 * @param CIndex (I)
 */

int MCfgTranslateBackLevel(
 
  enum MCfgParamEnum *CIndex)  /* I */
 
  {
  if (CIndex == NULL)
    {
    return(FAILURE);
    }
 
  switch (MCfg[*CIndex].PIndex)
    {
    case mcoOLDUFSWeight:
    case mcoOLDFSUWeight:
    
      MCfgGetIndex(mcoFUWeight,NULL,TRUE,CIndex);
 
      break;
 
    case mcoOLDGFSWeight:
    case mcoOLDFSGWeight:

      MCfgGetIndex(mcoFGWeight,NULL,TRUE,CIndex);     
 
      break;
 
    case mcoOLDAFSWeight:
    case mcoOLDFSAWeight:

      MCfgGetIndex(mcoFAWeight,NULL,TRUE,CIndex);    
 
      break;
 
    case mcoOLDFSQWeight:

      MCfgGetIndex(mcoFQWeight,NULL,TRUE,CIndex);      
 
      break;
 
    case mcoOLDFSCWeight: 

      MCfgGetIndex(mcoFCWeight,NULL,TRUE,CIndex);       
 
      break;
 
    case mcoOLDServWeight:

      MCfgGetIndex(mcoServWeight,NULL,TRUE,CIndex);      
 
      break;
 
    case mcoOLDDirectSpecWeight:

      MCfgGetIndex(mcoCredWeight,NULL,TRUE,CIndex);      
 
      break;
 
    case mcoBFType:

      MCfgGetIndex(mcoBFPolicy,NULL,TRUE,CIndex);      

      break;

    case mcoOLDMaxRsvPerNode:

      MCfgGetIndex(mcoMaxRsvPerNode,NULL,TRUE,CIndex);

      break;

    default:

      if (MCfg[*CIndex].NewPIndex != mcoNONE)
        {
        /* NOTE:  cannot do direct translation for all paramaters */

        /* MCfgGetIndex(MCfg[*CIndex].NewPIndex,NULL,TRUE,CIndex); */
        }
 
      break;
    }  /* END switch (*CIndex) */
 
  return(SUCCESS);
  }  /* END MCfgTranslateBackLevel() */





/**
 *
 *
 * @param PIndex (I) [optional]
 * @param PName (I) [optional]
 * @param ExactMatch (I)
 * @param CIndex (O)
 */

int MCfgGetIndex(

  enum MCfgParamEnum  PIndex,
  const char         *PName,
  mbool_t             ExactMatch,
  enum MCfgParamEnum *CIndex)

  { 
  int cindex;
  int len;

  char tmpLine[MMAX_LINE + 1] = {0};

  if (CIndex == NULL)
    {
    return(FAILURE);
    }

  /* must specify either PIndex or PName */

  if (PName != NULL)
    {
    /* support case insensitive match */

    MUStrToUpper(PName,tmpLine,sizeof(tmpLine));
    }  /* END if (PName != NULL) */

  for (cindex = 0;MCfg[cindex].Name != NULL;cindex++)
    {
    if (PName != NULL)
      {
      len = strlen(MCfg[cindex].Name);

      if ((ExactMatch == FALSE) && !strncmp(MCfg[cindex].Name,tmpLine,len))
        {
        if (!isalnum(tmpLine[len]))
          {
          *CIndex = (enum MCfgParamEnum)cindex;

          return(SUCCESS);
          }
        }
      else if (!strcmp(MCfg[cindex].Name,tmpLine))
        {
        *CIndex = (enum MCfgParamEnum)cindex;

        return(SUCCESS);
        }
      }
    else if (MCfg[cindex].PIndex == PIndex)
      {
      *CIndex = (enum MCfgParamEnum)cindex;

      return(SUCCESS);
      }
    }    /* END for (cindex) */
 
  return(FAILURE);
  }  /* END MCfgGetIndex() */





/**
 * Process non-object based parameters.
 *
 * @see MSchedProcessOConfig() - child
 *
 * @param PIndex (I)
 * @param IVal (I)
 * @param DVal (I)
 * @param SVal (I)
 * @param SArray (I)
 * @param P (I)
 * @param IndexName (I)
 * @param Eval (I)
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 */

int MCfgSetVal(

  enum MCfgParamEnum PIndex, /* I */
  int      IVal,      /* I */
  double   DVal,      /* I */
  char    *SVal,      /* I */
  char   **SArray,    /* I */
  mpar_t  *P,         /* I */
  char    *IndexName, /* I */
  mbool_t  Eval,      /* I */
  char    *EMsg)      /* O (optional,minsize=MMAX_LINE) */
  
  {
  int    MIndex;

  int      val;
  double   valf;
  char    *valp;
  char   **valpa;

  mfsc_t  *F  = NULL;

  const char *FName = "MCfgSetVal";

  MDB(3,fCONFIG) MLog("%s(%s,IVal,DVal,SVal,SArray,P,IName,Eval,EMsg)\n",
    FName,
    MParam[PIndex]);

  if (P != NULL)
    F = &P->FSC;
  else
    F = &MPar[0].FSC;

  val    = IVal;
  valf   = DVal;
  valp   = SVal;
  valpa  = SArray;

  if (SArray != NULL)
    {
    for (MIndex = 0;SArray[MIndex] != NULL;MIndex++);
    }
  else
    {
    MIndex = 0;
    }

  if (Eval == TRUE)
    {
    switch (PIndex)
      {
      case mcoFSPolicy:

        /* NO-OP */

        break;

      default:

        return(SUCCESS);

        /* NO-OP */
 
        break;
      }
    }

  /* assign values to parameters */        

  switch (PIndex)
    {
    case mcoAccountingInterfaceURL:
    case mcoAdminEventAggregationTime:
    case mcoHAPollInterval:
    case mcoMaxRsvPerNode:
    case mcoMaxDepends:
    case mcoParIgnQList:
    case mcoJobActionOnNodeFailure:
    case mcoARFDuration:
    case mcoJobAggregationTime:
    case mcoLogLevel:
    case mcoLogPermissions:
    case mcoLogLevelOverride:
    case mcoLogRollAction:
    case mcoJobStartLogLevel:
    case mcoRMMsgIgnore:
    case mcoRMPollInterval:
    case mcoRMRetryTimeCap:
    case mcoMinDispatchTime:
    case mcoNodePollFrequency:
    case mcoMaxSleepIteration:
    case mcoJobFBAction:
    case mcoMailAction:
    case mcoAdminEAction:
    case mcoAdminEInterval:
    case mcoDontCancelInteractiveHJobs:
    case mcoCheckPointFile:
    case mcoCheckPointInterval:
    case mcoCheckPointWithDatabase:
    case mcoCheckSuspendedJobPriority:
    case mcoChildStdErrCheck:
    case mcoCheckPointAllAccountLists:
    case mcoCheckPointExpirationTime:
    case mcoClientUIPort:
    case mcoDeadlinePolicy:
    case mcoMissingDependencyAction:
    case mcoMongoServer:
    case mcoDefaultDomain:
    case mcoDefaultJobOS:
    case mcoDefaultClassList:
    case mcoDefaultQMHost:
    case mcoDeleteStageOutFiles:
    case mcoGEventCfg:            /* hack - move to own routine */
    case mcoGMetricCfg:           /* hack - move to own routine */
    case mcoGResCfg:              /* hack - move to own routine */
    case mcoGResToJobAttrMap:     /* hack - move to own routine */
    case mcoServerName:
    case mcoLogFacility:
    case mcoLogFileMaxSize:
    case mcoLogFileRollDepth:
    case mcoSpoolDir:
    case mcoCheckPointDir:
    case mcoMountPointCheckInterval:
    case mcoMountPointCheckThreshold:
    case mcoNAPolicy:
    case mcoNodeFailureReserveTime:
    case mcoNodeTypeFeatureHeader:
    case mcoNodeToJobFeatureMap:
    case mcoNodeActiveList:
    case mcoNonBlockingCacheInterval:
    case mcoNodeCatCredList:
    case mcoPartitionFeatureHeader:
    case mcoProcSpeedFeatureHeader:
    case mcoRackFeatureHeader:
    case mcoSchedMode:
    case mcoSchedPolicy:
    case mcoSlotFeatureHeader:
    case mcoClientTimeout:
    case mcoClientMaxConnections:
    case mcoPushCacheToWebService:
    case mcoPushEventsToWebService:
    case mcoRecordEventList:
    case mcoReportProfileStatsAsChild:
    case mcoDisableScheduling:
    case mcoMCSocketProtocol:
    case mcoServerHost:
    case mcoServerPort:
    case mcoRsvReallocPolicy:
    case mcoJobRejectPolicy:
    case mcoJobRemoveEnvVarList:
    case mcoQOSIsOptional:
    case mcoQOSRejectPolicy:
    case mcoPreemptJobCount:
    case mcoPreemptPolicy:
    case mcoPreemptSearchDepth:
    case mcoPreemptPrioJobSelectWeight:
    case mcoPreemptRTimeWeight:
    case mcoStatProfCount:
    case mcoStatProfDuration:
    case mcoStatProfGranularity:
    case mcoDisplayFlags:
    case mcoEmulationMode:
    case mcoEnableClusterMonitor:
    case mcoEnableCPJournal:
    case mcoEnableJobArrays:
    case mcoEnableEncryption:
    case mcoEnableFailureForPurgedJob:
    case mcoEnableFastSpawn:
    case mcoEnableHighThroughput:
    case mcoEnableFastNodeRsv:
    case mcoEnableImplicitStageOut:
    case mcoEnableNSNodeAddrLookup:
    case mcoEnableVMDestroy:
    case mcoEnableVMSwap:
    case mcoEnforceAccountAccess:
    case mcoEnforceGRESAccess:
    case mcoDontEnforcePeerJobLimits:
    case mcoDisplayProxyUserAsUser:
    case mcoCalculateLoadByVMSum:
    case mcoMigrateToZeroLoadNodes:
    case mcoExtendedJobTracking:
    case mcoUseSyslog:
    case mcoDataStageHoldType:
    case mcoDefaultTaskMigrationTime:
    case mcoDeferTime:
    case mcoDeferCount:
    case mcoDeferStartCount:
    case mcoDependFailurePolicy:
    case mcoJobCPurgeTime:
    case mcoVMCPurgeTime:
    case mcoJobPurgeTime:
    case mcoJobRsvRetryInterval:
    case mcoNodeDownTime:
    case mcoNodePurgeTime:
    case mcoAPIFailureThreshold:
    case mcoNodeSyncDeadline:
    case mcoJobSyncDeadline:
    case mcoJobMaxOverrun:
    case mcoMServerHomeDir:
    case mcoNodeMaxLoad:
    case mcoNodeBusyStateDelayTime:
    case mcoNodeCPUOverCommitFactor:
    case mcoNodeDownStateDelayTime:
    case mcoNodeDrainStateDelayTime:
    case mcoNodeMemOverCommitFactor:
    case mcoNodeNameCaseInsensitive:
    case mcoNoLocalUserEnv:
    case mcoMaxGreenStandbyPoolSize:
    case mcoMaxJobHoldTime:
    case mcoMaxJobPreemptPerIteration:
    case mcoMaxJobStartPerIteration:
    case mcoMaxSuspendTime:
    case mcoJobPreemptCompletionTime:
    case mcoJobPreemptMaxActiveTime:
    case mcoJobPreemptMinActiveTime:
    case mcoPBSAccountingDir:
    case mcoEvalMetrics:
    case mcoEventFileRollDepth:
    case mcoEventLogWSURL:
    case mcoEventLogWSUser:
    case mcoEventLogWSPassword:
    case mcoSideBySide:
    case mcoSimTimePolicy:
    case mcoSimTimeRatio:
    case mcoStopIteration:
    case mcoLogDir:
    case mcoLogFile:
    case mcoSchedStepCount:
    case mcoUseJobRegEx:
    case mcoAdminUsers:
    case mcoAdmin1Users:
    case mcoAdmin2Users:
    case mcoAdmin3Users:
    case mcoAdmin4Users:
    case mcoRegistrationURL:
    case mcoRsvBucketDepth:
    case mcoRsvBucketQOSList:
    case mcoSchedToolsDir:
    case mcoSchedLockFile:
    case mcoSpoolDirKeepTime:
    case mcoGEventTimeout:
    case mcoStatDir:
    case mcoStatProcMax:
    case mcoStatProcMin:
    case mcoStatProcStepCount:
    case mcoStatProcStepSize:
    case mcoStatTimeMax:
    case mcoStatTimeMin:
    case mcoStatTimeStepCount:
    case mcoStatTimeStepSize:
    case mcoStoreSubmission:
    case mcoAdjUtlRes:
    case mcoFallBackClass:
    case mcoFileRequestIsJobCentric:
    case mcoForceNodeReprovision:
    case mcoForceRsvSubType:
    case mcoForceTrigJobsIdle:
    case mcoForkIterations:
    case mcoFSChargeRate:
    case mcoJobMaxNodeCount:
    case mcoJobMaxTaskCount:
    case mcoLicenseChargeRate:
    case mcoFullJobTrigCP:
    case mcoLimitedJobCP:
    case mcoLimitedNodeCP:
    case mcoLoadAllJobCP:
    case mcoNetworkChargeRate:
    case mcoNodeUntrackedProcFactor:
    case mcoNodeUntrackedResDelayTime:
    case mcoRealTimeDBObjects:
    case mcoRemapClass:
    case mcoRemapClassList:
    case mcoResourceQueryDepth:
    case mcoRestartInterval:
    case mcoVPCFlags:
    case mcoVPCMigrationPolicy:
    case mcoHideVirtualNodes:
    case mcoIgnoreClasses:
    case mcoIgnoreJobs:
    case mcoIgnoreLLMaxStarters:
    case mcoIgnoreNodes:
    case mcoIgnorePreempteePriority:
    case mcoIgnoreUsers:
    case mcoInstantStage:
    case mcoInvalidFSTreeMsg:
    case mcoJobMigratePolicy:
    case mcoJobCCodeFailSampleSize:
    case mcoMaxGRes:
    case mcoMaxJob:
    case mcoMaxNode:
    case mcoNodeIDFormat:
    case mcoObjectEList:
    case mcoOptimizedJobArrays:
    case mcoOSCredLookup:
    case mcoCredDiscovery:
    case mcoDefaultRackSize:
    case mcoDefaultVMSovereign:
    case mcoDisableSameCredPreemption:
    case mcoDisableSameQOSPreemption:
    case mcoEnableFSViolationPreemption:
    case mcoEnableSPViolationPreemption:
    case mcoMemRefreshInterval:
    case mcoFSTreeACLPolicy:
    case mcoReportPeerStartInfo:
    case mcoRequeueRecoveryRatio:
    case mcoUMask:
    case mcoVAddressList:
    case mcoArrayJobParLock:
    case mcoTrigFlags:
    case mcoTrackSuspendedJobUsage:
    case mcoTrackUserIO:
    case mcoWCViolAction:
    case mcoWCViolCCode:
    case mcoWebServicesUrl:
    case mcoWikiEvents:
    case mcoFSTreeIsRequired:
    case mcoFSTreeUserIsRequired:
    case mcoAllocResFailPolicy:
    case mcoUseAnyPartitionPrio:
    case mcoStrictProtocolCheck:
    case mcoUseUserHash:
    case mcoUseCPRsvNodeList:
    case mcoAllowRootJobs:
    case mcoAllowVMMigration:
    case mcoAlwaysApplyQOSLimitOverride:
    case mcoAlwaysEvaluateAllJobs:
    case mcoAssignVLANFeatures:
    case mcoUseMoabJobID:
    case mcoTrigCheckTime:
    case mcoTrigEvalLimit:
    case mcoDisableBatchHolds:
    case mcoDisableExcHList:
    case mcoDisableInteractiveJobs:
    case mcoIgnoreRMDataStaging:
    case mcoDisableVMDecisions:
    case mcoDisableThresholdTriggers:
    case mcoDisableUICompression:
    case mcoDisableSlaveJobSubmit:
    case mcoEnableJobEvalCheckStart:
    case mcoNoWaitPreemption:
    case mcoMapFeatureToPartition:
    case mcoRejectDosScripts:
    case mcoFSRelativeUsage:
    case mcoRemoveTrigOutputFiles:
    case mcoSubmitHosts:
    case mcoEnableVPCPreemption:
    case mcoJobCTruncateNLCP:
    case mcoNodeSetPlus:
    case mcoSubmitFilter:
    case mcoServerSubmitFilter:
    case mcoUnsupportedDependencies:
    case mcoFilterCmdFile:
    case mcoUseDatabase:
    case mcoRemoteFailTransient:
    case mcoRejectInfeasibleJobs:
    case mcoRejectDuplicateJobs:
    case mcoRelToAbsPaths:
    case mcoFSMostSpecificLimit:
    case mcoAuthTimeout:
    case mcoThreadPoolSize:
    case mcoGreenPoolEvalInterval:
    case mcoEnableHPAffinityScheduling:
    case mcoSocketLingerVal:
    case mcoSocketWaitTime:
    case mcoSocketExplicitNonBlock:
    case mcoEnableUnMungeCredCheck:
    case mcoResendCancelCommand:
    case mcoMDiagXmlJobNotes:
    case mcoPerPartitionScheduling:
    case mcoVMCreateThrottle:
    case mcoVMMigrateThrottle:
    case mcoVMGResMap:
    case mcoVMMigrationPolicy:
    case mcoVMMinOpDelay:
    case mcoVMOvercommitThreshold:
    case mcoVMsAreStatic:
    case mcoVMTracking:
    case mcoMaxACL:
    case mcoAggregateNodeActions:
    case mcoAggregateNodeActionsTime:
    case mcoNodeIdlePowerThreshold:
    case mcoAllowMultiReqNodeUse:
    case mcoVMStorageNodeThreshold:
    case mcoShowMigratedJobsAsIdle:
    case mcoBGPreemption:
    case mcoGuaranteedPreemption:
    case mcoDisableRegExCaching:
    case mcoJobFailRetryCount:
    case mcoMWikiSubmitTimeout:
    case mcoNodeSetMaxUsage:
    case mcoInternalTVarPrefix:
    case mcoVMProvisionStatusReady:
    case mcoVMStorageMountDir:
    case mcoJobExtendStartWallTime:
    case mcoOptimizedCheckpointing:
    case mcoQueueNAMIActions:
    case mcoNoJobHoldNoResources:
    case mcoMaxGMetric:
    case mcoNAMITransVarName:
    case mcoNestedNodeSet:
    case mcoDisableRequiredGResNone:
    case mcoQOSDefaultOrder:
    case mcoSubmitEnvFileLocation:
    case mcoFreeTimeLookAheadDuration:

      if (Eval == FALSE)
        MSchedProcessOConfig(&MSched,PIndex,val,valf,valp,valpa,IndexName);
      
      break;

    case mcoAdminMinSTime:
    case mcoBFChunkDuration:
    case mcoBFChunkSize:
    case mcoBFDepth:
    case mcoBFMaxSchedules:
    case mcoBFMetric:
    case mcoBFMinVirtualWallTime:
    case mcoBFPolicy:
    case mcoBFPriorityPolicy:
    case mcoBFProcFactor:
    case mcoBFVirtualWallTimeConflictPolicy:
    case mcoBFVirtualWallTimeScalingFactor:
    case mcoBlockList:
    case mcoDisableMultiReqJobs:
    case mcoEnableNegJobPriority:
    case mcoEnablePosUserPriority:
    case mcoJobNodeMatch:
    case mcoJobPrioAccrualPolicy:
    case mcoJobPrioExceptions:
    case mcoJobRetryTime:
    case mcoMaxJobStartTime:
    case mcoMaxMetaTasks:
    case mcoNodeAllocationPolicy:
    case mcoNodeAllocResFailurePolicy:
    case mcoNodeAvailPolicy:
    case mcoNodeSetAttribute:
    case mcoNodeSetDelay:
    case mcoNodeSetForceMinAlloc:
    case mcoNodeSetIsOptional:
    case mcoNodeSetList:
    case mcoNodeSetPolicy:
    case mcoNodeSetPriorityType:
    case mcoNodeSetTolerance:
    case mcoParAllocationPolicy:
    case mcoPriorityTargetDuration:
    case mcoPriorityTargetProcCount:
    case mcoRangeEvalPolicy:
    case mcoRejectNegPrioJobs:
    case mcoResourceLimitMultiplier:
    case mcoResourceLimitPolicy:
    case mcoRsvPolicy:
    case mcoRsvRetryTime:
    case mcoRsvSearchAlgo:
    case mcoRsvThresholdType:
    case mcoRsvThresholdValue:
    case mcoRsvTimeDepth:
    case mcoSuspendRes:
    case mcoSystemMaxJobProc:
    case mcoSystemMaxJobTime:
    case mcoSystemMaxJobPS:
    case mcoTaskDistributionPolicy:
    case mcoUseMachineSpeed:
    case mcoUseNodeHash:
    case mcoUseMoabCTime:
    case mcoUseSystemQueueTime:
    case mcoVPCNodeAllocationPolicy:
    case mcoRsvNodeAllocPolicy:
    case mcoRsvNodeAllocPriorityF:
    case mcoWattChargeRate:

      if (Eval == FALSE)
        MParProcessOConfig(P,PIndex,val,valf,valp,valpa);
 
      break;

    case mcoServWeight:
    case mcoTargWeight:
    case mcoCredWeight:
    case mcoAttrWeight:
    case mcoFSWeight:
    case mcoResWeight:
    case mcoUsageWeight:

    case mcoSQTWeight:
    case mcoSXFWeight:
    case mcoSDeadlineWeight:
    case mcoSSPVWeight:
    case mcoSUPrioWeight:
    case mcoSStartCountWeight:
    case mcoSBPWeight:
    case mcoTQTWeight:
    case mcoTXFWeight:
    case mcoCUWeight:
    case mcoCGWeight:
    case mcoCAWeight:
    case mcoCQWeight:
    case mcoCCWeight:
    case mcoFUWeight:
    case mcoFGWeight:
    case mcoFAWeight:
    case mcoFQWeight:
    case mcoFCWeight:
    case mcoGFUWeight:
    case mcoGFGWeight:
    case mcoGFAWeight:
    case mcoFUWCAWeight:
    case mcoFJPUWeight:
    case mcoFPPUWeight:
    case mcoFPSPUWeight:
    case mcoAJobAttrWeight:
    case mcoAJobGResWeight:
    case mcoAJobIDWeight:
    case mcoAJobNameWeight:
    case mcoAJobStateWeight:
    case mcoRNodeWeight:
    case mcoRProcWeight:
    case mcoRMemWeight:
    case mcoRSwapWeight:
    case mcoRDiskWeight:
    case mcoRPSWeight:
    case mcoRPEWeight:
    case mcoRWallTimeWeight:
    case mcoUConsWeight:
    case mcoURemWeight:
    case mcoUPerCWeight:
    case mcoUExeTimeWeight:

    case mcoServCap:
    case mcoTargCap:
    case mcoCredCap:
    case mcoAttrCap:
    case mcoFSCap:
    case mcoResCap:
    case mcoUsageCap:

    case mcoSQTCap:
    case mcoSXFCap:
    case mcoSDeadlineCap:
    case mcoSSPVCap:
    case mcoSUPrioCap:
    case mcoSStartCountCap:
    case mcoSBPCap:
    case mcoTQTCap:
    case mcoTXFCap:
    case mcoCUCap:
    case mcoCGCap:
    case mcoCACap:
    case mcoCQCap:
    case mcoCCCap:
    case mcoFUCap:
    case mcoFGCap:
    case mcoFACap:
    case mcoFQCap:
    case mcoFCCap:
    case mcoGFUCap:
    case mcoGFGCap:
    case mcoGFACap:
    case mcoFUWCACap:
    case mcoFJPUCap:
    case mcoFPPUCap:
    case mcoFPSPUCap:
    case mcoAJobAttrCap:
    case mcoAJobGResCap:
    case mcoAJobIDCap:
    case mcoAJobNameCap:
    case mcoAJobStateCap:
    case mcoRNodeCap:
    case mcoRProcCap:
    case mcoRMemCap:
    case mcoRSwapCap:
    case mcoRDiskCap:
    case mcoRPSCap:
    case mcoRPECap:
    case mcoRWallTimeCap:
    case mcoUConsCap:
    case mcoURemCap:
    case mcoUPerCCap:
    case mcoUExeTimeCap:

    case mcoXFMinWCLimit:
    case mcoFSPolicy:
    case mcoFSInterval:
    case mcoFSIsAbsolute:
    case mcoFSDepth:
    case mcoFSDecay:
    case mcoFSEnableCapPriority:
    case mcoFSTreeIsProportional:
    case mcoFSTreeShareNormalize:
    case mcoFSTreeAcctShallowSearch:
    case mcoFSTreeCapTiers:
    case mcoFSTreeTierMultiplier:
    case mcoJobAttrPrioF:
 
      MFSProcessOConfig(
        F,
        PIndex,
        val,
        valf,
        valp,
        valpa,
        Eval,
        EMsg);  /* O */
 
      break;

    case mcoFSTree:

      /* FSTree processing requires two passes, will be handled after remaining parameters loaded */

      MSched.FSTreeConfigDetected = TRUE;

      break;
      
    case mcoNodeCfg:
    case mcoSchedCfg:
    case mcoUserCfg:
    case mcoGroupCfg:
    case mcoAcctCfg:
    case mcoClassCfg:
    case mcoJobCfg:
    case mcoJobMatchCfg:
    case mcoParCfg:
    case mcoQOSCfg:
    case mcoRMCfg:
    case mcoSRCfg:
    case mcoAdminCfg:
    case mcoSysCfg:
    case mcoVCProfile:
    case mcoVMCfg:
    case mcoClientCfg:

      /* ignore config */

      MDB(8,fCONFIG) MLog("INFO:     parameter '%s' will be processed later\n",
        MParam[PIndex]);

      break;

    default:

      MDB(1,fCONFIG) MLog("WARNING:    unexpected parameter[%u] '%s' detected\n",
        PIndex,
        MParam[PIndex]);

      return(FAILURE);

      /*NOTREACHED*/

      break;
    }  /* END switch (PIndex) */

  return(SUCCESS);
  }  /* END MCfgSetVal() */




/**
 * Gets the index name and parameter value from the configuration.
 *
 * @param Buffer (I)
 * @param PName (I)
 * @param CIndexP (I/O) [optional]
 * @param IName (O) Index name ex. USERCFG[DEFAULT] [minsize=MMAX_NAME]
 * @param ValBuf (O) Parameter Value
 * @param ValBufSize (I)
 * @param GetLast (I)
 * @param TailP (O) [optional]
 */

int MCfgGetParameter(

  char        *Buffer,
  const char  *PName,
  enum MCfgParamEnum *CIndexP,
  char        *IName,
  char        *ValBuf,
  int          ValBufSize,
  mbool_t      GetLast,
  char       **TailP)

  {
  char *ptr;
  char *iptr;
  char *tail;
  char *wptr;
 
  int   index;

  int   TIndex;

  mbool_t IsValid;

  const char *FName = "MCfgGetParameter";

  MDB(7,fCONFIG) MLog("%s(Buffer,%s,%d,IName,ValBuf,%d,%s,TailP)\n",
    FName,
    (PName != NULL) ? PName : "NULL",
    (CIndexP != NULL) ? *CIndexP : 0,
    ValBufSize,
    MBool[GetLast]);

  if ((Buffer == NULL) || (PName == NULL) || (IName == NULL))
    {
    return(FAILURE);
    }

  /* NOTE:  if GetLast == TRUE, TailP points to start of parameter. */
  /*        if GetLast == FALSE, TailP points to start of parameter + 1  */

  /* NOTE:  ValBuf will contain full <ATTR>=<VAL>[ <ATTR>=<VAL>]... string */

  TIndex = 0;

  if (GetLast == TRUE)
    {
    if ((TailP != NULL) && (*TailP != NULL))
      TIndex = *TailP - Buffer;
    }

  ptr = Buffer;

  IName[0] = '\0';

  /* search until match is located */

  while ((ptr = MUStrStr(ptr,PName,TIndex,TRUE,GetLast)) != NULL)
    {
    TIndex = ptr - Buffer;

    IsValid = FALSE;

    MDB(7,fCONFIG) MLog("INFO:     checking parameter '%s' (loop)\n",
      PName);

    /* FORMAT:  {\n|<START>}[<WS>]<PARAMETER> */

    /* verify ptr at start of line */

    for (wptr = ptr - 1;wptr > Buffer;wptr--)
      {
      if (!isspace(*wptr) || (*wptr == '\n'))
        break;
      }

    if ((wptr <= Buffer) || (*wptr == '\n'))
      {
      iptr = ptr + strlen(PName);

      switch (*iptr)
        {
        case '[':

          /* determine array parameter */

          for (index = 1;index < MMAX_NAME;index++)
            {
            if (isspace(iptr[index]) || (iptr[index] == ']'))
              break;

            IName[index - 1] = iptr[index];
            }

          IName[index - 1] = '\0';

          MDB(3,fCONFIG) MLog("INFO:     detected array index '%s'\n",
            IName);

          IsValid = TRUE;

          break;

        case ' ':
        case '\t':
        case '\n':
        case '\0':

          IsValid = TRUE;

          break;

        default:

          /* NO-OP */

          break;
        }    /* END switch (*iptr) */
      }      /* if ((ptr == Buffer) || (*(ptr - 1) == '\n')) */

    if (IsValid == FALSE)
      {
      /* advance pointer, continue search */

      if (GetLast == TRUE)
        ptr = Buffer;
      else
        ptr++;

      continue;
      }

    MDB(4,fCONFIG) MLog("INFO:     located parameter '%s'\n",
      PName);

    MUGetWord(ptr,NULL,&tail);

    /* advance pointer */

    if (TailP != NULL)
      *TailP = ptr + 1;

    ptr = tail;

    if ((tail = strchr(ptr,'\n')) == NULL)
      {
      tail = ptr + strlen(ptr);
      }

    if (ValBuf != NULL)
      MUStrCpy(ValBuf,ptr,MIN(tail - ptr + 1,ValBufSize));

    MCfgTranslateBackLevel(CIndexP);

    return(SUCCESS);
    }  /* END while ((ptr = strstr(ptr,PName)) != NULL) */

  /* no match located */
 
  return(FAILURE);
  }  /* END MCfgGetParameter() */




/**
 *
 *
 * @param PBuf (I) processed buffer
 * @param OBuf (I) [modified/alloc]
 * @param SCIndex (I)
 * @param IName (I) [optional]
 * @param AName (I) [optional]
 * @param DoComment (I)
 * @param BufModified (I) [optional]
 */

int MCfgExtractAttr(

  char    *PBuf,              /* I processed buffer */
  char   **OBuf,              /* I original buffer (modified/alloc) */
  enum MCfgParamEnum SCIndex, /* I */
  char    *IName,             /* I index name (optional) */
  char    *AName,             /* I attribute name (optional) */
  mbool_t  DoComment,         /* I */
  mbool_t *BufModified)       /* I (optional) */

  {
  char  PVal[MMAX_LINE << 3];

  char  LIName[MMAX_NAME];

  enum MCfgParamEnum CIndex;

  char *ptr;

  char *tail;
  char *btail;
  char *ptail;

  char *AHead;
  int   ACount;

  int   blen;
 
  int   rlen;
  char  rbuf[MMAX_LINE];

  char *BPtr;
  int   BSpace;

  const char *FName = "MCfgExtractAttr";

  MDB(7,fCONFIG) MLog("%s(%s,%s,%d,%s,%s,BufModified)\n",
    FName,
    (PBuf != NULL) ? "PBuf" : "NULL",
    (OBuf != NULL) ? "OBuf" : "NULL",
    SCIndex,
    (IName != NULL) ? IName : "NULL",
    (AName != NULL) ? AName : "NULL");

  if ((PBuf == NULL) || 
      (OBuf == NULL) || 
      (*OBuf == NULL) ||
      (SCIndex <= mcoNONE))
    {
    return(FAILURE);
    }

  MUSNInit(&BPtr,&BSpace,rbuf,sizeof(rbuf));

  if (BufModified != NULL)
    *BufModified = FALSE;

  btail = PBuf + strlen(PBuf);

  /* locate each line which sets attribute AName in parameter Param[IName] */
  /* if line sets multiple attributes, remove only specified attribute     */
  /* if line sets only specified attribute, remove full parameter line     */

  /* locate existing parameter settings */

  ptail = btail;

  CIndex = SCIndex;

  while (MCfgGetParameter(
          PBuf,
          MCfg[CIndex].Name,
          &CIndex,
          LIName,              /* O */
          PVal,                /* O */
          sizeof(PVal),
          TRUE,                /* get last */
          &ptail) == SUCCESS)  /* I/O - ptail points one char beyond start of parameter */
    {
    /* check parameter index */

    if ((IName == NULL) ||
        (strcmp(LIName,IName) != 0))
      {
      continue;
      }

    AHead = NULL;

    /* PVal reported in format <ATTR>=<VAL>[ <ATTR>=<VAL>]... */

    /* NOTE:  PVal is local copy, ptail points into PBuf */

    if (strchr(PVal,'='))
      {
      /* evaluating multi-attribute or 'attr=val' parameter */

      if ((AName != NULL) && 
          (AName[0] != '\0'))
        {
        char *NewLinePtr;

        NewLinePtr = strchr(ptail - 1,'\n');

        /* NOTE:  AHead must point into PBuf, not local copy PVal */

        AHead = strstr(ptail - 1,AName);

        if ((AHead == NULL) || ((NewLinePtr != NULL) && (AHead > NewLinePtr)))
          {
          /* NOTE:  specified attribute not located in parameter line */

          continue;
          }

        ptr = AHead + strlen(AName);

        if (ptr[0] != '=')
          {
          /* NOTE:  matching string is attr/val substring - ignore */

          continue;
          }

        /* required attribute is specified */
        }  /* END if ((AName != NULL) && ...) */

      /* determine attribute count */

      ACount = 0;

      for (ptr = PVal;(ptr = strchr(ptr,'=')) != NULL;ptr++)
        {
        ACount++;
        }
      }    /* END if (strchr(PVal,'=')) */
    else
      {
      ACount = 1;  
      }

    tail = ptail;

    if ((AName != NULL) && (ACount > 1))
      {
      /* remove only specified attribute */

      tail = AHead;

      if (AHead != NULL)
        {
        for (rlen = 0;AHead[rlen] != '\0';rlen++)
          {
          if (isspace(AHead[rlen]))
            break;
          }  /* END for (rlen) */
        }
      else
        {
        rlen = 0;
        }

      MUSNCat(&BPtr,&BSpace,"");
      }
    else if (DoComment == TRUE)
      {
      /* comment entire parameter line */

      /* NOTE:  tail points to second character of parameter name */
  
      if ((tail != NULL) && (tail < btail))
        {
        if (isalnum(*tail) && (tail > PBuf))
          tail--;
        }  /* END if ((tail != NULL) && (tail < btail)) */

      rlen = 0;
     
      MUSNInit(&BPtr,&BSpace,rbuf,sizeof(rbuf));

      MUSNCat(&BPtr,&BSpace,"#");
      }
    else
      {
      /* remove entire line */

      tail = ptail - 1;

      MUSNInit(&BPtr,&BSpace,rbuf,sizeof(rbuf));

      for (rlen = 0;tail[rlen] != '\0';rlen++)
        {
        if (tail[rlen] == '\n')
          {
          break;
          }

        MUSNCat(&BPtr,&BSpace," ");
        }  /* END for (rlen) */
      }    /* END else */

    blen = strlen(*OBuf);

    /* NOTE:  must maintain synchronization between *OBuf and PBuf (NYI) */

    /* remove string */

    if (MUBufReplaceString(
          OBuf,
          &blen,
          tail - PBuf, /* offset */
          rlen,
          rbuf,
          TRUE) == FAILURE)
      {
      /* replace failed */

      MDB(2,fCONFIG) MLog("WARNING:  cannot extract parameter %s[%s] attribute '%s'\n",
        MCfg[CIndex].Name,
        IName,
        AName);

      return(FAILURE);
      }

    if (BufModified != NULL)
      *BufModified = TRUE;
    }    /* END while (MCfgGetParameter() == SUCCESS) */

  return(SUCCESS);
  }  /* END MCfgExtractAttr() */




#define MMAX_VALIST 16


/**
 *
 *
 * @param OType (I)
 * @param SCString (I)
 * @param SPName (I) [optional]
 * @param SAList (I) [optional]
 * @param Buf (O)
 * @param BufSize (I)
 */

int MCfgEvalOConfig(

  enum MXMLOTypeEnum  OType,
  char               *SCString,
  const char         *SPName,
  char              **SAList,
  char               *Buf,
  int                 BufSize)

  {
  char **AList;
  char **OAList;  /* object specific attr list */

  char   PName[MMAX_NAME];

  char   IndexName[MMAX_NAME];

  char   Value[MMAX_LINE];

  char  *CPtr;

  char  *ptr;
  char  *TokPtr;

  int    aindex;
  int    oaindex;

  char   ValLine[MMAX_LINE];

  char  *BPtr;
  int    BSpace;

  char  *tmpString;

  int    VAList[MMAX_VALIST];
  int    vaindex;

  char   EMsg[MMAX_LINE];

  mbool_t CmpRelative = TRUE;

  mbool_t IsValid;

  if (SCString == NULL)
    {
    return(FAILURE);
    }

  if (Buf != NULL)
    {
    MUSNInit(&BPtr,&BSpace,Buf,BufSize);
    }

  if (SPName != NULL)
    strcpy(PName,SPName);
  else if (MCredCfgParm[OType] != NULL)
    strcpy(PName,MCredCfgParm[OType]);
  else
    PName[0] = '\0';

  OAList = NULL;

  VAList[0] = 0;

  switch (OType)
    {
    case mxoAcct:

      AList = (SAList != NULL) ? SAList : (char **)MCredAttr;
      OAList = (char **)MAcctAttr;

      break;

    case mxoAM:

      AList = (SAList != NULL) ? SAList : (char **)MAMAttr;

      break;

    case mxoClass:

      AList = (SAList != NULL) ? SAList : (char **)MCredAttr;
      OAList = (char **)MClassAttr;

      VAList[0] = mclaNAPrioF;
      VAList[1] = 0;

      break;

    case mxoCluster:

      AList = (SAList != NULL) ? SAList : (char **)MVPCPAttr;

      break;

    case mxoGroup:

      AList = (SAList != NULL) ? SAList : (char **)MCredAttr;
      OAList = (char **)MGroupAttr;

      break;

    case mxoJob:

      AList = (SAList != NULL) ? SAList : (char **)MJobCfgAttr;

      break;

    case mxoNode:

      AList = (SAList != NULL) ? SAList : (char **)MNodeAttr;

      VAList[0] = mnaPrioF;
      VAList[1] = 0;

      break;

    case mxoPar:

      AList = (SAList != NULL) ? SAList : (char **)MParAttr;

      VAList[0] = mpaNodeAllocPolicy;
      VAList[1] = 0;

      break;

    case mxoQOS:

      AList = (SAList != NULL) ? SAList : (char **)MCredAttr;
      OAList = (char **)MQOSAttr;

      break;

    case mxoRM:

      AList = (SAList != NULL) ? SAList : (char **)MRMAttr;

      break;

    case mxoSched:

      AList = (SAList != NULL) ? SAList : (char **)MSchedAttr;

      break;

    case mxoSRsv:

      AList = (SAList != NULL) ? SAList : (char **)MSRsvAttr;

      VAList[0] = msraTrigger;
      VAList[1] = msraPeriod;
      VAList[2] = 0;

      CmpRelative = FALSE;

      break;

    case mxoSys:

      AList = (SAList != NULL) ? SAList : (char **)MCredAttr;

      break;

    case mxoUser:

      AList = (SAList != NULL) ? SAList : (char **)MCredAttr;
      OAList = (char **)MUserAttr;

      break;

    default:

      return(FAILURE);
  
      /*NOTREACHED*/

      break;
    }  /* END switch (OType) */

  if ((tmpString = (char *)MUCalloc(1,strlen(SCString) + 1)) == NULL)
    {
    return(FAILURE);
    }

  if (PName[0] == '\0')
    {
    return(FAILURE);
    }

  strcpy(tmpString,SCString);

  IsValid = TRUE;

  /* load ALL matching parameters */

  IndexName[0] = '\0';

  CPtr = tmpString;

  while (MCfgGetSVal(
           tmpString,
           &CPtr,
           PName,
           IndexName,
           NULL,
           Value,
           sizeof(Value),
           0,
           NULL) != FAILURE)
    {
    /* process value line */

    ptr = MUStrTokE(Value," \t\n",&TokPtr);

    while (ptr != NULL)
      {
      /* parse name-value pairs */

      /* FORMAT:  <VALUE>[,<VALUE>] */

      oaindex = 0;

      if (MUGetPair(
          ptr,
          (const char **)AList,
          &aindex,  /* O */
          NULL,
          CmpRelative,
          NULL,
          ValLine,
          sizeof(ValLine)) == FAILURE)
        {
        /* cannot parse value pair in primary attr list */

        if ((OAList == NULL) || (MUGetPair(
            ptr,
            (const char **)OAList,
            &oaindex,  /* O */
            NULL,
            CmpRelative,
            NULL,
            ValLine,
            sizeof(ValLine)) == FAILURE))
          {
          /* cannot parse value pair */

          IsValid = FALSE;

          if (Buf != NULL)
            {
            if (IndexName[0] != '\0')
              {
              MUSNPrintF(&BPtr,&BSpace,"cannot process %s[%s] attribute '%s'\n",
                PName,
                IndexName,
                ptr);
              }
            else
              {
              MUSNPrintF(&BPtr,&BSpace,"cannot process %s attribute '%s'\n",
                PName,
                ptr);
              }
            }

          /* attribute-value is invalid */

          ptr = MUStrTokE(NULL," \t\n",&TokPtr);

          continue;
          }
        }    /* END if (MUGetPair()) */

      /* attribute-value syntax is valid */

      for (vaindex = 0;vaindex < MMAX_VALIST;vaindex++)
        {
        if (VAList[vaindex] == 0)
          break;

        if ((VAList[vaindex] == aindex) || (VAList[vaindex] == oaindex))
          break;
        }  /* END for (vaindex) */

      if ((VAList[vaindex] == aindex) || (VAList[vaindex] == oaindex))
        {
        /* perform limited parameter value analysis */

        if (OType == mxoSRsv)
          {
          switch (aindex)
            {
            case msraTrigger:
              if (MTrigLoadString(NULL,ValLine,FALSE,TRUE,mxoRsv,NULL,NULL,EMsg) == FAILURE)
                {
                IsValid = FALSE;
                if (IndexName[0] != '\0')
                  {
                  MUSNPrintF(&BPtr,&BSpace,"cannot process value for %s[%s] attribute '%s' - %s\n",
                    PName,
                    IndexName,
                    ptr,
                    EMsg);
                  }
                else
                  {
                  MUSNPrintF(&BPtr,&BSpace,"cannot process value for %s attribute '%s' - %s\n",
                    PName,
                    ptr,
                    EMsg);
                  }
                }
              break;

            case msraPeriod:
              if (MUGetIndexCI(ValLine,MCalPeriodType,FALSE,0) == msraNONE)
                {
                IsValid = FALSE;
                if (IndexName[0] != '\0')
                  {
                  MUSNPrintF(&BPtr,&BSpace,"Invalid value for %s[%s] attribute '%s'\n",
                    PName,
                    IndexName,
                    ptr);
                  }
                else
                  {
                  MUSNPrintF(&BPtr,&BSpace,"cannot process value for %s attribute '%s'\n",
                    PName,
                    ptr);
                  }
  
                }
              break;

            default:
              break;
            }
          }
        else if (((OType == mxoClass) && (oaindex == mclaNAPrioF)) ||
                 ((OType == mxoNode) && (aindex == mnaPrioF)))
          {
          mnprio_t *P = NULL;

          int       rc;

          rc = MProcessNAPrioF(&P,ValLine);

          MNPrioFree(&P);

          if (rc == FAILURE)
            {
            IsValid = FALSE;
            if (IndexName[0] != '\0')
              {
              MUSNPrintF(&BPtr,&BSpace,"cannot process value for %s[%s] attribute '%s' - %s\n",
                PName,
                IndexName,
                ptr,
                "invalid function specified");
              }
            else
              {
              MUSNPrintF(&BPtr,&BSpace,"cannot process value for %s attribute '%s' - %s\n",
                PName,
                ptr,
                "invalid function specified");
              }
            }
          }    /* END else if ... */
        else if ((OType == mxoPar) && (aindex == mpaNodeAllocPolicy))
          {
          enum MNodeAllocPolicyEnum policyType;
          char *allocationPolicy = NULL;
          char *policyValue = NULL;

          allocationPolicy = MUStrTok(ValLine,":",&policyValue);

          policyType = (enum MNodeAllocPolicyEnum)MUGetIndexCI(allocationPolicy,MNAllocPolicy,FALSE,mnalNONE);
          if (policyType == mnalNONE)
            {
            IsValid = FALSE;
            MUSNPrintF(&BPtr,&BSpace,"cannot process value for %s attribute '%s' - %s\n",
              PName,
              ptr,
              "invalid NODEALLOCATIONPOLICY");
            }

          /* validate plugin path */
          if (policyType == mnalPlugin)
            {
            try 
              {
              PluginManager::ValidateNodeAllocationPlugin("Eval Plugin",policyValue);
              }
            catch (exception& e) 
              {
              IsValid = FALSE;
              MUSNPrintF(&BPtr,&BSpace,"cannot process value for %s attribute '%s' - invalid plugin: %s\n",
                PName,
                ptr,
                e.what());
              } /* END catch () */
            }
          }    /* END else if ... */
        }      /* END if (VAList[vaindex] == aindex) */

      ptr = MUStrTokE(NULL," \t\n",&TokPtr);
      }    /* END while (ptr != NULL) */
    }      /* END while (MCfgGetSVal() != NULL) */

  MUFree(&tmpString);

  if (IsValid == FALSE)
    {
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MCfgEvalOConfig() */





/**
 * Strip all non-authorized strings from Buffer.
 *
 * @param Buffer
 * @param AllowedStrings
 */

int MCfgPurgeBuffer(

  char  *Buffer,          /* I (modified) */
  char **AllowedStrings)  /* I */

  {
  char *ptr;
  char *ptr2;

  char  tmpBuf[MMAX_LINE];

  int   tindex;

  if ((Buffer == NULL) || (AllowedStrings == NULL) || (AllowedStrings[0] == NULL))
    {
    return(FAILURE);
    }

  ptr = Buffer;

  while (*ptr != '\0')
    {
    /* remove comments */

    while (isspace(*ptr))
      {
      ptr++;
      }

    if (isalpha(*ptr))
      {
      tindex = 0;

      ptr2 = ptr;

      while (isalpha(*ptr) && (tindex < MMAX_LINE - 1))
        {
        tmpBuf[tindex++] = *ptr++;
        }

      tmpBuf[tindex] = '\0';

      tindex = 0;

      while (AllowedStrings[tindex] != NULL)
        {
        if (strcmp(AllowedStrings[tindex],tmpBuf))
          {
          tindex++;

          continue;
          }

        break;
        }

      if (AllowedStrings[tindex] == NULL)
        {
        ptr = ptr2;

        while ((*ptr != '\n') && (*ptr != '\0'))
          {
          *ptr = ' ';

          ptr++;
          }
        }
      else
        {
        while (*ptr != '\n')
          {
          if (*ptr == '\0')
            break;

          ptr++;
          }

        if (*ptr == '\n')
          ptr++;
        }
      }    /* END if (isalpha(*ptr)) */
    else
      {
      ptr++;
      }
    }      /* END while (ptr != '\0') */ 

  return(SUCCESS);
  }  /* END MCfgPurgeBuffer() */




/**
 * Parse Prioity Accrual Policy.
 *
 * @param JobPrioAccrualPolicy
 * @param JobPrioExceptions 
 * @param Buffer 
 */

int MCfgParsePrioAccrualPolicy(

  enum MJobPrioAccrualPolicyEnum *JobPrioAccrualPolicy,  /* I */
  mbitmap_t *JobPrioExceptions, /* I */
  char      *Buffer )          /* I */

  {
  mbitmap_t tmpL;

  if ((JobPrioExceptions == NULL) || (Buffer == NULL))
    {
    return(FAILURE);
    }

  *JobPrioAccrualPolicy = (enum MJobPrioAccrualPolicyEnum)MUGetIndexCI(
    Buffer,
    MJobPrioAccrualPolicyType,
    FALSE,
    *JobPrioAccrualPolicy);

  switch (*JobPrioAccrualPolicy)
    {
    case mjpapAlways:

      MDB(0,fCONFIG) MLog("WARNING:  deprecated value '%s' specified for parameter '%s' (use JOBPRIOACCRUALPOLICY ACCRUE and JOBPRIOEXCEPTIONS ALL)\n",
        Buffer,
        MParam[mcoJobPrioAccrualPolicy]);

      bmset(&tmpL,mjpeSoftPolicy);
      bmset(&tmpL,mjpeHardPolicy);
      bmset(&tmpL,mjpeUserhold);
      bmset(&tmpL,mjpeBatchhold);
      bmset(&tmpL,mjpeSystemhold);
      bmset(&tmpL,mjpeIdlePolicy);

      *JobPrioAccrualPolicy = mjpapAccrue;
      bmcopy(JobPrioExceptions,&tmpL);

      break;

    case mjpapFullPolicy:

      MDB(0,fCONFIG) MLog("WARNING:  deprecated value '%s' specified for parameter '%s' (use JOBPRIOEXCEPTIONS NONE))\n",
        Buffer,
        MParam[mcoJobPrioAccrualPolicy]);

      *JobPrioAccrualPolicy = mjpapAccrue;
      bmclear(JobPrioExceptions);

      break;

    case mjpapFullPolicyReset:

      MDB(0,fCONFIG) MLog("WARNING:  deprecated value '%s' specified for parameter '%s' (use JOBPRIOACCRUALPOLICY RESET and JOBPRIOEXCEPTIONS NONE))\n",
        Buffer,
        MParam[mcoJobPrioAccrualPolicy]);

      *JobPrioAccrualPolicy = mjpapReset;
      bmclear(JobPrioExceptions);

      break;

    case mjpapQueuePolicy:

      MDB(0,fCONFIG) MLog("WARNING:  deprecated value '%s' specified for parameter '%s' (use JOBPRIOEXCEPTIONS SOFTPOLICY,HARDPOLICY))\n",
        Buffer,
        MParam[mcoJobPrioAccrualPolicy]);

      bmset(&tmpL,mjpeSoftPolicy);
      bmset(&tmpL,mjpeHardPolicy);

      *JobPrioAccrualPolicy = mjpapAccrue;
      bmcopy(JobPrioExceptions,&tmpL);

      break;

    case mjpapQueuePolicyReset:

      MDB(0,fCONFIG) MLog("WARNING:  deprecated value '%s' specified for parameter '%s' (use JOBPRIOACCRUALPOLICY RESET and JOBPRIOEXCEPTIONS SOFTPOLICY,HARDPOLICY))\n",
        Buffer,
        MParam[mcoJobPrioAccrualPolicy]);

      *JobPrioAccrualPolicy = mjpapReset;
      bmcopy(JobPrioExceptions,&tmpL);

      break;

    case mjpapAccrue:
    case mjpapReset:
    default:

      /* NO-OP */

      break;
    }  /* END switch (JobPrioAccrualPolicy) */ 

  return(SUCCESS);
  }  /* END MCfgParsePrioAccrualPolicy() */
  



/**
 * Parse Prioity Exceptions.
 *
 * @param JobPrioExceptions 
 * @param Buffer 
 */

int MCfgParsePrioExceptions(

  mbitmap_t *JobPrioExceptions,
  char      *Buffer)  /* I */

  {
  if ((JobPrioExceptions == NULL) || (Buffer == NULL))
    {
    return(FAILURE);
    }

  if (!strcasecmp(Buffer,"all"))
    {
    bmset(JobPrioExceptions,mjpeBatchhold);
    bmset(JobPrioExceptions,mjpeDefer);
    bmset(JobPrioExceptions,mjpeDepends);
    bmset(JobPrioExceptions,mjpeHardPolicy);
    bmset(JobPrioExceptions,mjpeIdlePolicy);
    bmset(JobPrioExceptions,mjpeSoftPolicy);
    bmset(JobPrioExceptions,mjpeSystemhold);
    bmset(JobPrioExceptions,mjpeUserhold);
    }
  else if (!strcasecmp(Buffer,"none"))
    {
    bmclear(JobPrioExceptions);
    }
  else
    {
    bmfromstring(Buffer,MJobPrioException,JobPrioExceptions);
    } /* END if (!strcasecmp(Buffer,"all")) */

  return(SUCCESS);
  }  /* END MCfgParsePrioExceptions */

/* END MConfig.c */
