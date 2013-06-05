/* HEADER */

/**
 * @file MVariables.c
 * 
 * Contains various functions For VARIABLES
 *
 */


#include "moab.h"
#include "moab-proto.h"
#include "moab-const.h"
#include "moab-global.h"


enum MOVarNameEnum {
  movnETime,
  movnEType,
  movnHome,
  movnOID,
  movnOType,
  movnOwnerMail,
  movnTime,
  movnUser,
  movnToolsDir,
  movnLAST };

const char *MOVarName[] = {
  "ETIME",
  "ETYPE",
  "HOME",
  "OID",
  "OTYPE",
  "OWNERMAIL",
  "TIME",
  "USER",
  "TOOLSDIR",
  NULL };





/**
 * Takes a hash table of variables and converts it to comma delimeted string.
 *
 * @param HT  (I) Hash-table to convert.
 * @param Buf (O) [modified] The buffer that the resulting string is written to.
 */

int MVarToMString(

  mhash_t   *HT,
  mstring_t *Buf)

  {
  /* it's possible MUHTToMString() may not work correctly on string hashes in the future so just wrap it */

  return(MUHTToMString(HT,Buf));
  }  /* END MVarToString() */




/**
 * Takes a linked list of variables and converts it to comma delimeted string.
 *
 * @param V (I) Linked-list to convert.
 * @param Buf (O) [modified] The buffer that the resulting string is written to.
 * @param BufSize (I) The size of the buffer.
 */

char *MVarLLToString(

  mln_t *V,       /* I */
  char  *Buf,     /* O (modified) */
  int    BufSize) /* I */

  {
  char *BPtr = NULL;
  int   BSpace = 0;

  mln_t *tmpL;

  if (Buf != NULL)
    MUSNInit(&BPtr,&BSpace,Buf,BufSize);

  if ((V == NULL) || (Buf == NULL))
    {
    return(Buf);
    }

  tmpL = V;

  while (tmpL != NULL)
    {
    if (Buf[0] != '\0')
      MUSNCat(&BPtr,&BSpace,",");

    MUSNCat(&BPtr,&BSpace,tmpL->Name);

    if (tmpL->Ptr != NULL)
      {
      MUSNPrintF(&BPtr,&BSpace,"=%s",
        (char *)tmpL->Ptr);
      }

    tmpL = tmpL->Next;
    }  /* END while (tmpL != NULL) */

  return(Buf);
  }  /* END MVarLLToString() */





#if 0
/**
 * Takes a linked list of variables and converts it to "+" delimeted string with
 * quotes around the variable values. This is a format that is used in VMCR structures
 * for VM creation.
 *
 * @param V (I) Linked-list to convert.
 * @param Buf (O) [modified] The buffer that the resulting string is written to.
 * @param BufSize (I) The size of the buffer.
 */

char *MVarToQuotedString(

  mhash_t *V,       /* I */
  char    *Buf,     /* O (modified) */
  int      BufSize) /* I */

  {
  char *BPtr;
  int   BSpace;

  char *VarName;
  char *VarVal;

  mhashiter_t HTI;

  if (Buf != NULL)
    MUSNInit(&BPtr,&BSpace,Buf,BufSize);

  if ((V == NULL) || (Buf == NULL))
    {
    return(Buf);
    }

  MUHTIterInit(&HTI);

  while (MUHTIterate(V,&VarName,(void **)&VarVal,NULL,&HTI) == SUCCESS)
    {
    if (Buf[0] != '\0')
      MUSNCat(&BPtr,&BSpace,"+");

    MUSNCat(&BPtr,&BSpace,VarName);

    if (VarVal != NULL)
      {
      MUSNPrintF(&BPtr,&BSpace,"=\"%s\"",
        VarVal);
      }
    }  /* END while (MUHTIterate) */

  return(Buf);
  }  /* END MVarToQuotedString() */
#endif
  


/**
 * Return variables common to all objects of specified type.
 *
 * @see MTrigInitiateAction() - parent
 * @see __MTrigGetVarList() - peer
 *
 * NOTE:  populates VarName[]/VarVal[] from beginning of list overriding any 
 *        previous contents.
 * 
 * @param OType     (I)
 * @param OID       (I) [optional]
 * @param U         (I) [optional] user associated with object
 * @param VarList   (I)
 * @param tmpBuf    (O)
 * @param BufCountP (I/O)
 */

int MOGetCommonVarList(

  enum MXMLOTypeEnum   OType,
  char                *OID,
  mgcred_t            *U,
  mln_t              **VarList,
  char                 tmpBuf[][MMAX_BUFFER3],
  int                 *BufCountP)

  {
  if (VarList == NULL)
    {
    return(FAILURE);
    }

  /* populate variable translation tables */

  MUAddVarLL(VarList,(char *)MOVarName[movnOID],(OID != NULL) ? OID : (char *)NullP);

  if ((tmpBuf != NULL) && (*BufCountP > 0))
    {
    char DString[MMAX_LINE];

    MULToDString(&MSched.Time,DString);
    strcpy(tmpBuf[*BufCountP],DString);

    MUAddVarLL(VarList,(char *)MOVarName[movnTime],tmpBuf[*BufCountP]);
    (*BufCountP)--;

    if (*BufCountP > 0)
      {
      sprintf(tmpBuf[*BufCountP],"%ld",MSched.Time);

      MUAddVarLL(VarList,(char *)MOVarName[movnETime],tmpBuf[*BufCountP]);
      (*BufCountP)--;
      }
    }

  MUAddVarLL(VarList,(char *)MOVarName[movnOType],(char *)MXO[OType]);

  MUAddVarLL(VarList,(char *)MOVarName[movnHome],(char *)MSched.CfgDir);

  MUAddVarLL(VarList,(char *)MOVarName[movnUser],(U != NULL) ? U->Name : (char *)NullP);

  if ((U != NULL) && (U->EMailAddress != NULL))
    {
    MUAddVarLL(VarList,(char *)MOVarName[movnOwnerMail],(U != NULL) ? U->EMailAddress : (char *)NullP);
    }

  MUAddVarLL(VarList,(char *)MOVarName[movnToolsDir],MSched.ToolsDir);

  return(SUCCESS);
  }  /* END MOGetCommonVarList() */




/**
 * Perform variable substituiton in specified string.
 *
 * NOTE:  Replace specified and well-known variables with variable values.
 *
 * @param Msg          (I) original string containing the variables
 * @param SVarList     (I) [optional,linked list]
 * @param HashList     (I) list of hash tables to use for resolution [optional] 
 * @param EString      (I) [optional]
 * @param Buffer       (O)
 * @param BufSize      (I)
 * @param DoubleDollar (I)
 */

int MUInsertVarList(

  char     *Msg,
  mln_t    *SVarList,
  mhash_t **HashList,
  char     *EString,
  char     *Buffer,
  int       BufSize,
  mbool_t   DoubleDollar)

  {
#define MUINSERTVARLIST_MAXSIZE 64
#define MUINSERTVARLIST_DEFAULTSIZE 4

  int   len;

  int   mindex;
  int   bindex;
  int   index;

  mbool_t BufferFull = FALSE;

  char  tmpName[MMAX_BUFFER];
  char  tmpVal[MMAX_BUFFER];

  mln_t *DVarList = NULL;
  mln_t *VarList = NULL;

  mbool_t  VarAdded = FALSE;

  if ((Msg == NULL) || (Buffer == NULL))
    {
    return(FAILURE);
    }

  Buffer[0] = '\0';

  if (SVarList != NULL)
    {
    VarList = SVarList;
    }
  else
    {
    MUAddVarLL(&DVarList,"HOME",getcwd(tmpVal,sizeof(tmpVal)));

    MUAddVarLL(&DVarList,"TOOLSDIR",MSched.ToolsDir);

    VarList = DVarList;
    }

  len = strlen(Msg);

  bindex = 0;

  /* FORMAT:  ...$<X>{ /}... or ...$(<X>)... */

  for (mindex = 0;mindex < len;mindex++)
    {
    if ((Msg[mindex] != '$') || 
       ((DoubleDollar == TRUE) && (Msg[mindex + 1] != '$')))
      {
      /* regular character, copy */

      /* NOTE:  prevent adding '//' into file names - NOTE this behavior
                should only occur if result is known to be a path (NYI) */

      if ((VarAdded != TRUE) || (Msg[mindex] != '/'))
        {
        Buffer[bindex] = Msg[mindex];

        bindex++;
        }

      VarAdded = FALSE;
      }
    else if ((DoubleDollar == FALSE) || 
            ((mindex > 1) && (Msg[mindex - 1] == '$')))
      {
      /* variable detected */

      mindex++;

      /* load full variable */

      tmpName[0] = '\0';
      tmpVal[0]  = '\0';

      index = 0;

      if (Msg[mindex] == '(')
        {
        mindex++;

        for (;mindex < len;mindex++)
          {
          if (Msg[mindex] == ')')
            {
            /* END of var name located */

            tmpName[index] = '\0';

            break;
            }

          tmpName[index] = Msg[mindex];

          index++;

          if (index >= (int)sizeof(tmpName) - 1)
            {
            tmpName[0] = '\0';

            break;
            }
          }

        if (mindex >= len)
          tmpName[0] = '\0';
        }  /* END if (Msg[mindex] == '(') */
      else
        {
        for (;mindex <= len;mindex++)
          {
          if (isspace(Msg[mindex]) || (Msg[mindex] == '\0') || (Msg[mindex] == '/'))
            {
            /* END of var name located */

            tmpName[index] = '\0';

            mindex--;

            break;
            }

          tmpName[index] = Msg[mindex];

          index++;

          if (index >= (int)sizeof(tmpName) - 1)
            {
            tmpName[0] = '\0';

            break;
            }
          }
        }  /* END else () */

      /* variable located, look up value */

      if (tmpName[0] != '\0')
        {
        mln_t *tmpL = NULL;
        /* first search through hash tables--they are much faster than lists! */

        if ((HashList != NULL) && (HashList[0] != NULL))
          {
          mhash_t *tmpHT;
          int hindex = 0;
          char *Result = NULL;

          tmpHT = HashList[hindex];

          while (tmpHT != NULL)
            {
            if (MUHTGet(tmpHT,tmpName,(void **)&Result,NULL) == SUCCESS)
              {
              MUStrCpy(tmpVal,Result,sizeof(tmpVal));

              break;
              }

            hindex++;
            tmpHT = HashList[hindex];
            }
          }

        if (tmpVal[0] == '\0')
          {
          if ((strlen(tmpName) < 2) ||
              (tmpName[0] != '*') ||
              (tmpName[1] != '.'))
            {
            /* Not a wildcard variable (no namespaces) */

            for (tmpL = VarList;tmpL != NULL;tmpL = tmpL->Next)
              {
              if (tmpL->Ptr == NULL)
                continue;

              if (strcmp(tmpL->Name,tmpName))
                  continue;

              MUStrCpy(tmpVal,(char *)tmpL->Ptr,sizeof(tmpVal));

              break;
              }  /* END for (tmpL) */
            }  /* END if (strlen(tmpName < 2) ...) */
          else
            {
            char *BPtr;
            int   BSpace;
            char *VarNameRoot;

            /* Wildcard variable (use namespaces) */

            /* Skip the "*." part */

            VarNameRoot = tmpName + 2;

            MUSNInit(&BPtr,&BSpace,tmpVal,sizeof(tmpVal));

            for (tmpL = VarList;tmpL != NULL;tmpL = tmpL->Next)
              {
              if (tmpL->Ptr == NULL)
                continue;

              if (MVCVarNameFits(tmpL->Name,VarNameRoot) == FAILURE)
                continue;

              /* Found a valid name, append onto list */
              /*  Will be <VarName>=<VarVal> on the command line */

              if (tmpVal[0] == '\0')
                {
                MUSNPrintF(&BPtr,&BSpace,"%s=%s",
                  tmpL->Name,
                  (char *)tmpL->Ptr);
                }
              else
                {
                MUSNPrintF(&BPtr,&BSpace," %s=%s",
                  tmpL->Name,
                  (char *)tmpL->Ptr);
                }
              }  /* END for (tmpL) */
            }  /* END else (strlen(tmpName < 2) ... */

          if ((VarList != DVarList) && (tmpVal[0] == '\0'))
            {
            /* value not located in specified list, use default list */

            /* No wildcards for the default list */

            for (tmpL = DVarList;tmpL != NULL;tmpL = tmpL->Next)
              {
              if (tmpL->Ptr == NULL)
                break;

              if (strcmp(tmpL->Name,tmpName))
                continue;

              MUStrCpy(tmpVal,(char *)tmpL->Ptr,sizeof(tmpVal));

              break;
              }
            }  /* END if ((VarList != DVarList) && (tmpVal[0] == '\0')) */
          }

        if ((tmpVal[0] == '\0') && (EString != NULL))
          {
          /* value not found for variable--use specified default */

          MUStrCpy(tmpVal,EString,sizeof(tmpVal));
          }
        }    /* END if (tmpName[0] != '\0') */

      if (BufSize - bindex > (int)strlen(tmpVal))
        {
        /* copy value into string */

        if (!strcmp(tmpName,"HOME") &&
           (bindex > 3) &&
           !strncmp(&Buffer[bindex - 3],"///",3))
          {
          /* special case:  adjust for file expansion in URLs */

          bindex--;
          }

        Buffer[bindex] = '\0';

        strcat(&Buffer[bindex],tmpVal);

        bindex += strlen(tmpVal);

        if (tmpVal[strlen(tmpVal) - 1] == '/')
          VarAdded = TRUE;
        }
      else
        {
        /* buffer is full */

        BufferFull = TRUE;

        break;
        }
      }    /* END else () */

    if (bindex >= BufSize)
      break;
    }  /* END for (mindex) */

  /* terminate string */

  Buffer[MIN(bindex,BufSize - 1)] = '\0';

  if (DVarList != NULL)
    MULLFree(&DVarList,MUFREE);

  if ((BufferFull == TRUE) || (bindex >= BufSize))
    {
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MUInsertVarList() */


