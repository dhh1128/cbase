/* HEADER */

/**
 * @file MUIEventQueryThreadSafe.c
 *
 * Contains MUI Event Query function, threadsafe
 */



#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 * Process 'mdiag -e' request in a threadsafe way.
 *
 * @param S (I) [modified]
 * @param CFlagBM (I) enum MRoleEnum bitmap
 * @param Auth (I)
 */

int MUIEventQueryThreadSafe(

  msocket_t *S,       /* I (modified) */
  mbitmap_t *CFlagBM, /* I enum MRoleEnum bitmap */
  char      *Auth)    /* I */

  {
  #define MMAX_EVENTQUERY   32

  mxml_t *RE = S->RDE;
  mxml_t *DE = NULL;

  mulong StartTime;
  mulong EndTime;

  char   tmpLine[MMAX_LINE];
  char   tmpName[MMAX_LINE];

  int    CTok;

  enum MRecordEventTypeEnum EType[MMAX_EVENTQUERY + 1];
  enum MXMLOTypeEnum        OType[MMAX_EVENTQUERY + 1];

  char   OID[MMAX_EVENTQUERY + 1][MMAX_LINE];
  int    EID[MMAX_EVENTQUERY + 1];  /* specified event id's */

  mdb_t *MDBInfo;

  if (RE == NULL)
    {
    return(FAILURE);
    }

  if (MXMLCreateE(&S->SDE,(char *)MSON[msonData]) == FAILURE)
    {
    MUISAddData(S,"ERROR:    cannot create response\n");

    return(FAILURE);
    }

  StartTime = 0;
  EndTime = MMAX_TIME;

  /* select all object id's */

  OID[0][0] = '\0';

  /* select all event types */

  EType[0] = mrelLAST;

  /* select all event instances */

  EID[0] = -1;

  /* select all object types */

  OType[0] = mxoLAST;

  CTok = -1;

  MSchedGetAttr(msaDBHandle,mdfNONE,(void **)&MDBInfo,-1);

  if (MDBInfo->DBType != mdbODBC)
    {
    MDB(3,fUI) MLog("ERROR:  event querying only supported with ODBC (See USEDATABASE)\n");

    MUISAddData(S,"ERROR:    event querying only supported with ODBC (See USEDATABASE)\n");

    return(FAILURE);
    }

  if (MXMLCreateE(&S->SDE,(char *)MSON[msonData]) == FAILURE)
    {
    MUISAddData(S,"ERROR:    cannot create response\n");

    return(FAILURE);
    }

  DE = S->SDE;

  while (MS3GetWhere(
      RE,
      NULL,
      &CTok,
      tmpName,                     /* O */
      sizeof(tmpName),             
      tmpLine,                     /* O */
      sizeof(tmpLine)) == SUCCESS) 
    {
    if (!strcasecmp(tmpName,"starttime"))
      {
      StartTime = MUTimeFromString(tmpLine);

      continue;
      }

    if (!strcasecmp(tmpName,"endtime"))
      {
      EndTime = MUTimeFromString(tmpLine);

      continue;
      }

    if (!strcasecmp(tmpName,"eventtypes"))
      {
      int   eindex;

      char *ptr;
      char *TokPtr;

      int   tmpI;

      eindex = 0;

      ptr = MUStrTok(tmpLine,",:",&TokPtr);

      while (ptr != NULL)
        {
        tmpI = MUGetIndexCI(ptr,MRecordEventType,FALSE,0);

        if (tmpI != 0)
          EType[eindex++] = (enum MRecordEventTypeEnum)tmpI;

        if (eindex >= MMAX_EVENTQUERY)
          break;

        ptr = MUStrTok(NULL,",:",&TokPtr);
        }  /* END while (ptr != NULL) */

      EType[eindex] = mrelLAST;

      continue;
      }  /* END if (!strcasecmp(tmpName,"eventtypes")) */

    if (!strcasecmp(tmpName,"oidlist"))
      {
      int   oindex;

      char *ptr;
      char *TokPtr;

      oindex = 0;

      ptr = MUStrTok(tmpLine,",:",&TokPtr);

      while (ptr != NULL)
        {
        MUStrCpy(OID[oindex++],ptr,sizeof(OID[0]));

        if (oindex >= MMAX_EVENTQUERY)
          break;

        ptr = MUStrTok(NULL,",:",&TokPtr);
        }  /* END while (ptr != NULL) */

      OID[oindex][0] = '\0';

      continue;
      }  /* END if (!strcasecmp(tmpName,"oidlist")) */

    if (!strcasecmp(tmpName,"eidlist"))
      {
      int   oindex;

      char *ptr;
      char *TokPtr;

      oindex = 0;

      ptr = MUStrTok(tmpLine,",:",&TokPtr);

      while (ptr != NULL)
        {
        EID[oindex++] = (int)strtol(ptr,NULL,10);

        if (oindex >= MMAX_EVENTQUERY)
          break;

        ptr = MUStrTok(NULL,",:",&TokPtr);
        }  /* END while (ptr != NULL) */

      EID[oindex] = -1;

      continue;
      }  /* END if (!strcasecmp(tmpName,"eidlist")) */

    if (!strcasecmp(tmpName,"objectlist"))
      {
      int   oindex;

      char *ptr;
      char *TokPtr;

      int   tmpI;

      oindex = 0;

      ptr = MUStrTok(tmpLine,",:",&TokPtr);

      while (ptr != NULL)
        {
        tmpI = MUGetIndexCI(ptr,MXO,FALSE,0);

        if (tmpI != 0)
          OType[oindex++] = (enum MXMLOTypeEnum)tmpI;

        if (oindex >= MMAX_EVENTQUERY)
          break;

        ptr = MUStrTok(NULL,",:",&TokPtr);
        }  /* END while (ptr != NULL) */

      OType[oindex] = mxoLAST;

      continue;
      }  /* END if (!strcasecmp(tmpName,"objectlist")) */
    }    /* END while (MS3GetWhere() == SUCCESS) */

  MSysQueryEventsToXML(
    StartTime,
    EndTime,
    EID,
    EType,
    OType,
    OID,
    DE);

  return(SUCCESS);
  }  /* END MUIEventQueryThreadSafe() */
/* END MUIEventQueryThreadSafe.c */
