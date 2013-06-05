/* HEADER */

/**
 * @file MUIBal.c
 *
 * Contains mbal CLient request
 */



#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 * Process 'mbal' client request.
 *
 * @param S (I/O)
 * @param Flags (I)
 * @param Auth (I)
 */

int MUIBal(

  msocket_t *S,     /* I/O */
  mbitmap_t *CFlags, /* I */
  char      *Auth)  /* I */

  {
  char tmpLine[MMAX_LINE];

  int  OIndex;

  int nindex;

  mnode_t *N;
  mnode_t *BestN;

  char *ptr;

  mxml_t *E  = NULL;
  mxml_t *RE = NULL;

  mxml_t *WE;
  int     WTok;

  mnl_t   ReqNL;

  char    tmpName[MMAX_LINE];
  char    tmpVal[MMAX_BUFFER << 4];

  const char *FName = "MUIBal";

  MDB(2,fUI) MLog("%s(S,%s)\n",
    FName,
    (Auth != NULL) ? Auth : "NULL");

  if (S == NULL)
    {
    return(FAILURE);
    }

  /* extract request */

  if (((ptr = strstr(S->RBuffer,"<Body")) == NULL) ||
       (MXMLFromString(&E,ptr,NULL,NULL) == FAILURE))
    {
    /* cannot parse response */

    return(FAILURE);
    }

  if (MXMLGetChild(E,(char *)MSON[msonRequest],NULL,&RE) == FAILURE)
    {
    /* cannot parse response */

    MXMLDestroyE(&E);

    return(FAILURE);
    }

  if (MS3GetObject(RE,tmpLine) == FAILURE)
    {
    /* cannot parse response */

    MXMLDestroyE(&E);

    return(FAILURE);
    }

  if ((OIndex = MUGetIndexCI(tmpLine,MXO,FALSE,mxoNONE)) == mxoNONE)
    {
    /* unexpected object */

    MXMLDestroyE(&E);

    return(FAILURE);
    }

  if (MXMLGetAttr(RE,MSAN[msanAction],NULL,tmpLine,sizeof(tmpLine)) == FAILURE)
    {
    /* cannot parse response */

    MXMLDestroyE(&E);

    return(FAILURE);
    }

  MNLInit(&ReqNL);

  WTok = -1;

  nindex = 0;

  while (MS3GetWhere(
          RE,
          &WE,
          &WTok,
          tmpName,  /* O */
          sizeof(tmpName),
          tmpVal,   /* O */
          sizeof(tmpVal)) == SUCCESS)
    {
    if (!strcmp(tmpName,"hostlist"))
      {
      char *ptr;
      char *TokPtr;

      ptr = MUStrTok(tmpVal,", \t\n",&TokPtr);

      while (ptr != NULL)
        {
        if (MNodeFind(ptr,&N) == SUCCESS)
          { 
          MNLSetNodeAtIndex(&ReqNL,nindex,N);

          nindex++;

          if (nindex >= MSched.M[mxoNode])
            break;
          }
        else
          {
          char tmpLine[MMAX_LINE];

          snprintf(tmpLine,sizeof(tmpLine),"unknown host specified '%s'\n",ptr);

          MUISAddData(S,tmpLine);

          MNLFree(&ReqNL);

          return(FAILURE);
          }

        ptr = MUStrTok(NULL,", \t\n",&TokPtr);
        }  /* END while (ptr != NULL) */

      MNLTerminateAtIndex(&ReqNL,nindex);
      }  /* END if (!strcmp(tmpName,"hostlist")) */
    }    /* END while (MS3GetWhere() == SUCCESS) */

  MXMLDestroyE(&E);

  /* determine best host */

  BestN = NULL;

  for (nindex = 0;nindex < MSched.M[mxoNode];nindex++)
    {
    N = MNode[nindex];

    if (N == NULL)
      break;

    if (N == (mnode_t *)1)
      continue;

    if ((!MNLIsEmpty(&ReqNL)) &&
        (MNLFind(&ReqNL,N,NULL) == FAILURE))
      {
      continue;
      }

    if (MNODEISUP(N) == FALSE)
      continue;

    if ((BestN != NULL) && (N->Load >= BestN->Load))
      continue;

    if (N->CRes.Procs <= 0)
      continue;

    BestN = N;
    }  /* END for (nindex) */

  MNLFree(&ReqNL);

  /* return best host */

  MUISAddData(S,BestN->Name);

  return(SUCCESS);
  }  /* END MUIBal() */

/* END MUIBal.c */
