/* HEADER */

/**
 * @file MUICheckNode.c
 *
 * Contains MUI Check Node function
 */



#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 * Report detailed node information (handle 'checknode').
 *
 * @see MNodeShowState() - child 
 * @see MNodeShowReservations() - child
 * @see MNodeDiagnoseState() - child
 * @see MNodeDiagnoseRsv() - child
 * @see MNodeToXML() - child - report node attributes via XML
 *
 * @param S (I)
 * @param CFlags (I) (not used)
 * @param Auth (I)
 */

#define MOABGLOBALNODEINDEX  -2
#define MOABDEFAULTNODEINDEX -3

int MUICheckNode(

  msocket_t *S,      /* I */
  mbitmap_t *CFlags, /* I (not used) */
  char      *Auth)   /* I */

  {
  int  nindex;

  char  NodeExp[MMAX_LINE];
  char  EMsg[MMAX_LINE];
  char  tmpName[MMAX_LINE];
  char  tmpVal[MMAX_LINE];

  mnode_t  *N;

  marray_t  NodeList;

  int   WTok;

  mbitmap_t Flags;

  mxml_t *RE;
  mxml_t *DE = NULL;

  mgcred_t *U;

  const char *FName = "MUICheckNode";

  MDB(2,fUI) MLog("%s(RBuffer,SBuffer,%d,%s,SBufSize)\n",
    FName,
    CFlags,
    Auth);

  /* initialize values */

  RE = S->RDE;

  NodeExp[0] = '\0';
  EMsg[0] = '\0';

  WTok = -1;

  while (MS3GetWhere(
        RE,
        NULL,
        &WTok,
        tmpName,
        sizeof(tmpName),
        tmpVal,
        sizeof(tmpVal)) == SUCCESS)
    {
    if (!strcmp(tmpName,MNodeAttr[mnaNodeID]))
      {
      MUStrCpy(NodeExp,tmpVal,sizeof(NodeExp));
      }
    }

  if (MXMLGetAttr(RE,MSAN[msanFlags],NULL,tmpVal,sizeof(tmpVal)) == SUCCESS)
    {
    bmfromstring(tmpVal,MClientMode,&Flags);
    }

  if (MUserAdd(Auth,&U) == FAILURE)
    {
    MUISAddData(S,"ERROR:    cannot authenticate user\n");

    return(FAILURE);
    }

  /* for now, either a user can run checknode or she cannot */

  if (MUICheckAuthorization(
       U,
       NULL,
       (void *)NULL,
       mxoNode,
       mcsCheckNode,
       mgscList,
       NULL,
       EMsg,
       sizeof(EMsg)) == FAILURE)
    {
    MUISAddData(S,"ERROR:    user is not authorized to run checknode\n");

    return(FAILURE);
    }

  mstring_t Buffer(MMAX_LINE);

  if (bmisset(&Flags,mcmXML))
    {
    if (MXMLCreateE(&S->SDE,(char *)MSON[msonData]) == FAILURE)
      {
      MUISAddData(S,"ERROR:    cannot create response\n");

      return(FAILURE);
      }

    DE = S->SDE;
    }
 

  if (NodeExp[0] == '\0')
    {
    MUISAddData(S,"ERROR:   no node expression received\n");

    return(FAILURE);
    }

  EMsg[0] = '\0';

  MUArrayListCreate(&NodeList,sizeof(mnode_t *),1);

  if ((!strcasecmp(NodeExp,"global")) && (MSched.GN != NULL))
    {
    MUArrayListAppendPtr(&NodeList,(void *)MSched.GN);
    }
  else if (!strcasecmp(NodeExp,"default"))
    {
    MUArrayListAppendPtr(&NodeList,(void *)&MSched.DefaultN);
    }
  else if (!strcasecmp(NodeExp,"ALL"))
    {
    if (MUREToList(
          NodeExp,
          mxoNode,
          NULL,
          &NodeList,
          FALSE,
          EMsg,
          sizeof(EMsg)) == FAILURE)
      {
      char tmpLine[MMAX_LINE];

      snprintf(tmpLine,sizeof(tmpLine),"ERROR:    invalid node expression received '%s' : %s\n",
        NodeExp,
        EMsg);

      MUISAddData(S,tmpLine);

      return(FAILURE);
      }
    }
  else
    {
    mnode_t *tmpNode;

    if (MNodeFind(NodeExp,&tmpNode) == FAILURE)
      {
      char tmpLine[MMAX_LINE];
      snprintf(tmpLine,sizeof(tmpLine),"ERROR:    cannot locate node '%s'\n",NodeExp);
      MUISAddData(S,tmpLine);

      return(FAILURE);
      }
    
    MUArrayListAppendPtr(&NodeList,(void *)tmpNode);
    }

  for (nindex = 0;nindex < NodeList.NumItems;nindex++)
    {
    N = (mnode_t *)MUArrayListGetPtr(&NodeList,nindex);

    if (bmisset(&Flags,mcmXML))
      {
      MUINodeDiagnose(
            Auth,
            NULL,
            mxoNONE,
            NULL,
            &DE,
            NULL,
            NULL,
            NULL,
            N->Name,   /* I */
            &Flags);
      }
    else
      {
      MCheckNode(N,&Flags,&Buffer);
   
      if (N != &MSched.DefaultN)
        {
        MNodeShowReservations(N,&Buffer);
   
        MNodeDiagnoseState(N,&Buffer);
   
        MNodeDiagnoseRsv(N,&Flags,&Buffer);

        if (N->T != NULL)
          {
          MTrigDiagnose(NULL,NULL,NULL,&Buffer,mfmHuman,&Flags,N->T,mAdd);
          }
        }  /* END if (N != &MSched.DefaultN) */
   
      if (bmisset(&Flags,mcmVerbose) && (N->MB != NULL))
        {
        char SBuffer[MMAX_BUFFER];
   
        int  SBufSize = sizeof(SBuffer);
   
        /* generate header */
   
        char *ptr;
   
        ptr = SBuffer;;
   
        MMBPrintMessages(
          NULL,
          mfmHuman,
          TRUE,
          -1,
          ptr,
          SBufSize);
   
        ptr = SBuffer + strlen(SBuffer);
   
        /* display data */
   
        MMBPrintMessages(
          N->MB,
          mfmHuman,
          TRUE,
          -1,
          ptr,
          SBufSize);
   
        /* Add header to the outter buffer */
        Buffer += SBuffer;
        }
      }    /* END else */
    }  /* END for (nindex) */

  MUArrayListFree(&NodeList);
   
  if (!bmisset(&Flags,mcmXML))
    {
    MUISAddData(S,Buffer.c_str());
    }

  return(SUCCESS);
  }  /* END MUICheckNode() */
/* END MUICheckNode.c */
