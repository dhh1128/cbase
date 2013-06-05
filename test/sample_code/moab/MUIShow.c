/* HEADER */

/**
 * @file MUIShow.c
 *
 * Contains MUI Show function
 */



#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/* __MUIShowRsv() is necessary for valgrind to work with MUIShow() */

/**
 *
 *
 * @param S (I) [modified]
 * @param CFlagBM (I) enum MRoleEnum bitmap
 * @param Auth (I)
 * @param FlagBM ()
 * @param FlagString ()
 * @param TypeString ()
 * @param ArgString ()
 * @param RE ()
 * @param OExp ()
 */

int __MUIShowRsv(

  msocket_t *S,
  mbitmap_t *CFlagBM,
  char      *Auth,
  mbitmap_t *FlagBM,
  char      *FlagString,
  char      *TypeString,
  char      *ArgString,
  mxml_t    *RE,
  char      *OExp)

  {
  if ((FlagString == NULL) ||
      (TypeString == NULL) ||
      (ArgString  == NULL) ||
      (RE         == NULL) ||
      (OExp       == NULL))
    {
    return(FAILURE);
    }
  
  if (strstr(FlagString,"profile"))
    {
    /* display reservation profiles */

    __MUIRsvProfShow(S,NULL,NULL,&S->SDE,TypeString);
    }
  else if (strstr(ArgString,"resources"))
    {
    char tmpBuf[MMAX_BUFFER] = {0};

    mrsv_t *R;

    if (MRsvFind(OExp,&R,mraNONE) == FAILURE)
      {
      MUISAddData(S,"cannot locate reservation");

      return(FAILURE);
      }
  
    /* display allocated resources */

    if (!MNLIsEmpty(&R->NL))
      {
      MNLToString(
        &R->NL,
        bmisset(FlagBM,mcmVerbose),
        ",",
        '\0',
        tmpBuf,
        sizeof(tmpBuf));

      MUISAddData(S,tmpBuf);
      }
    }
  else
    {
    mstring_t String(MMAX_BUFFER);

    mpar_t *P;
    char    DiagOpt[MMAX_NAME];
    mbitmap_t IFlags;

    enum MFormatModeEnum DFormat;

    sprintf(DiagOpt,NONE);

    if (strstr(FlagString,"XML"))
      {
      DFormat = mfmXML;
      }
    else
      {
      DFormat = mfmHuman;
      }

    P = &MPar[0];

    MUIRsvDiagnose(
      CFlagBM,
      Auth,
      NULL,
      &String,
      P,
      DiagOpt,
      DFormat,
      &IFlags,
      mrtNONE); /* mrtNONE means do not filter on type */
 
    MUISAddData(S,String.c_str());
    }  /* END else (strstr(FlagString,"profile")) */

  return(SUCCESS);
  }  /* END __MUIShowRsv() */




/**
 * Process 'mshow/showq' request.
 *
 * @see MUIQueueShow() - child - process showq
 * @see MClusterShowARes() - child - process 'mshow -a'
 *
 * @param S (I) [modified]
 * @param CFlagBM (I) enum MRoleEnum bitmap
 * @param Auth (I)
 */

int MUIShow(

  msocket_t *S,       /* I (modified) */
  mbitmap_t *CFlagBM, /* I enum MRoleEnum bitmap */
  char      *Auth)    /* I */

  {
  char OString[MMAX_LINE];
  char FlagString[MMAX_LINE];
  char ArgString[MMAX_LINE];
  char AttrString[MMAX_LINE];
  char TypeString[MMAX_LINE];

  char OExp[MMAX_LINE];
 
  char *CmdPtr;

  enum MXMLOTypeEnum OIndex;

  mbitmap_t FlagBM;  /* bitmap of enum MCModeEnum */

  mxml_t *E = NULL;
  mxml_t *RE = NULL;
  mxml_t *WE = NULL;

  mpar_t *P  = NULL;

  mrange_t RRange[MMAX_REQ_PER_JOB];

  const char *FName = "MUIShow";

  MDB(2,fUI) MLog("%s(S,%ld,%s)\n",
    FName,
    CFlagBM,
    (Auth != NULL) ? Auth : "NULL");

  if (S == NULL)
    {
    return(FAILURE);
    }

  MUISClear(S);

  ArgString[0]  = '\0';
  AttrString[0] = '\0';
  FlagString[0] = '\0';

  OExp[0]       = '\0';

  /* TEMP:  force to XML */

  S->WireProtocol = mwpS32;

  CmdPtr = strstr(S->RBuffer,"showq");

  if (CmdPtr == NULL)
    {
    char tmpLine[MMAX_LINE];
  
    mgcred_t *AuthU;

    /* check authorization in case SERVICES=NONE is set in config file,
       but always allow showq */

    MUserAdd(Auth,&AuthU);

    if (MUICheckAuthorization(
          AuthU,
          NULL,
          NULL,
          mxoQueue,
          mcsMShow,
          mgscList,
          NULL,
          NULL,
          0) == FAILURE)
      {
      MDB(2,fUI) MLog("INFO:     user %s is not authorized to perform task\n", 
        Auth);

      sprintf(tmpLine,"NOTE:  user %s is not authorized to perform task \n",
        Auth);

      MUISAddData(S,tmpLine);

      return(FAILURE);
      }
    }  /* END if (CmdPtr == NULL) */

  memset(RRange,0,sizeof(RRange));

  switch (S->WireProtocol)
    {
    case mwpXML:

      {
      char *ptr;

      char tmpName[MMAX_NAME];
      char tmpVal[MMAX_LINE];

      int   WTok;

      /* FORMAT:  <Message><Request object="queue"/>... */
      /* FORMAT:  <Reply><Response>msg="output truncated\nRM unavailable\n"><queue type="active"><job name="X" state="X" ...></job></queue></Response></Reply> */

      if (S->RDE != NULL)
        {
        E = S->RDE;
        }
      else if (((ptr = strchr(S->RPtr,'<')) == NULL) ||
                (MXMLFromString(&E,ptr,NULL,NULL) == FAILURE))
        {
        MDB(3,fUI) MLog("WARNING:  corrupt command '%.128s' received\n",
          S->RBuffer);

        MUISAddData(S,"ERROR:    corrupt command received\n");

        return(FAILURE);
        }

      S->RDE = E;

      if ((MXMLGetChild(E,(char *)MSON[msonRequest],NULL,&RE) == FAILURE) ||
          (MS3GetObject(RE,OString) == FAILURE) ||
         ((OIndex = (enum MXMLOTypeEnum)MUGetIndexCI(OString,MXO,FALSE,mxoNONE)) == mxoNONE))
        {
        MDB(3,fUI) MLog("WARNING:  corrupt command '%.128s' received\n",
          S->RBuffer);

        MUISAddData(S,"ERROR:    corrupt command received\n");

        return(FAILURE);
        }

      WTok = -1;

      while (MS3GetWhere(
          RE,
          &WE,
          &WTok,
          tmpName,
          sizeof(tmpName),
          tmpVal,
          sizeof(tmpVal)) == SUCCESS)
        {
        /* process where */

        if (!strcmp(tmpName,"starttime"))
          {
          RRange[0].STime = strtol(tmpVal,NULL,0);
          }
        else if (!strcmp(tmpName,"name"))
          {
          MUStrCpy(OExp,tmpVal,sizeof(OExp));
          }
        else if (!strcmp(tmpName,"partition"))
          {
          MParFind(tmpVal,&P);
          }
        }

      if (MXMLGetAttr(RE,MSAN[msanFlags],NULL,FlagString,sizeof(FlagString)) == SUCCESS)
        {
        /* FORMAT:  <X>[,<Y>]... */

        bmfromstring(FlagString,MClientMode,&FlagBM);
        }

      MXMLGetAttr(RE,MSAN[msanArgs],NULL,ArgString,sizeof(ArgString));
      MXMLGetAttr(RE,"attr",NULL,AttrString,sizeof(AttrString));
      }  /* END BLOCK (case mwpXML) */

      break;
  
    case mwpS32:

      {
      int WTok;

      char tmpName[MMAX_NAME];
      char tmpVal[MMAX_LINE];

      /* Request Format:  <Data object="queue"/>... */
      /* Response Format: <Data>msg="output truncated\nRM unavailable\n"><queue type="active"><job name="X" state="X" ...></job></queue></Response></Reply> */

      RE = S->RDE;

      if ((MS3GetObject(RE,OString) == FAILURE) ||
         ((OIndex = (enum MXMLOTypeEnum)MUGetIndexCI(OString,MXO,FALSE,mxoNONE)) == mxoNONE))
        {
        MDB(3,fUI) MLog("WARNING:  corrupt command '%100.100s' received\n",
          S->RBuffer);

        MUISAddData(S,"ERROR:    corrupt command received\n");

        return(FAILURE);
        }

      if (MXMLGetAttr(RE,MSAN[msanFlags],NULL,FlagString,0) == SUCCESS)
        {
        bmfromstring(FlagString,MClientMode,&FlagBM);
        }    /* END if (MXMLGetAttr() == SUCCESS) */

      WTok = -1;

      while (MS3GetWhere(
          RE,
          &WE,
          &WTok,
          tmpName,
          sizeof(tmpName),
          tmpVal,
          sizeof(tmpVal)) == SUCCESS)
        {
        /* process where */

        if (MUStrNCmpCI(tmpName,"starttime",strlen("starttime") + 1) == SUCCESS)
          {
          RRange[0].STime = strtol(tmpVal,NULL,0);
          }
        else if (MUStrNCmpCI(tmpName,"name",strlen("name") + 1) == SUCCESS)
          {
          MUStrCpy(OExp,tmpVal,sizeof(OExp));
          }
        else if (!strcmp(tmpName,"partition"))
          {
          if (MParFind(tmpVal,&P) == FAILURE)
            {
            MUISAddData(S,"ERROR:    unknown partition specified\n");

            return(FAILURE);
            }
          }
        }

      MXMLGetAttr(RE,MSAN[msanArgs],NULL,ArgString,sizeof(ArgString));
      MXMLGetAttr(RE,"attr",NULL,AttrString,sizeof(AttrString));
      MXMLGetAttr(RE,MSAN[msanOption],NULL,TypeString,sizeof(TypeString));
      }  /* END BLOCK (case mwpS3) */

      break;

    default:

      MUISAddData(S,"ERROR:    format not supported\n");

      return(FAILURE);

      break;
    }  /* END switch (S->WireProtocol) */

  /* create parent element */

  if (MXMLCreateE(&S->SDE,(char *)MSON[msonData]) == FAILURE)
    {
    MUISAddData(S,"ERROR:    cannot create response\n");

    return(FAILURE);
    }

  MS3SetObject(
    S->SDE,
    (char *)MXO[OIndex],
    NULL);

  /* process request */

  switch (OIndex)
    {
    case mxoQueue:

      MUIQueueShow(
        S,
        RE,
        &S->SDE,
        P,
        TypeString,
        Auth,
        CFlagBM,
        &FlagBM);

      break;

    case mxoxCP:

      /* display list of experienced values */

      MUIMiscShow(S,RE,&S->SDE,TypeString);

      break;

    case mxoRsv:

      {
      int rc;
      
      rc = __MUIShowRsv(S,CFlagBM,Auth,&FlagBM,FlagString,TypeString,ArgString,RE,OExp);

      return(rc);
      }  /* END (case mxoRsv) */

      break;

    case mxoSRsv:

      {
      /* int rc; */

      /* rc = __MUIShowSRsv(S,CFlagBM,Auth,FlagBM,FlagString,TypeString,ArgString,RE,OExp); */

      /* return(rc); */
      }  /* END (case mxoSRsv) */

      break;

    case mxoCluster:

      {
      int NSIndex;

      char EMsg[MMAX_LINE] = {0};
    
      mbool_t IsComplete;

      /* NOTE:  support query set grouping by looping thru set attribute list */

      for (NSIndex = 0;NSIndex < MMAX_ATTR;NSIndex++)
        {
        switch (S->WireProtocol)
          {
          case mwpHTML:

             if (MClusterShowARes(
                   Auth,
                   mwpHTML,              /* request format */
                   &FlagBM,
                   RRange,
                   mwpAVP,               /* response format */
                   (void *)S->RPtr,      /* I */
                   (void **)S->SBuffer,  /* O */
                   MMAX_BUFFER,
                   NSIndex,
                   &IsComplete,
                   EMsg) == FAILURE)
               {
               MUISAddData(S,EMsg);

               return(FAILURE);
               }

             break;

          default:

             if (MClusterShowAResWrapper(
                   Auth,
                   mwpXML,            /* request format */
                   &FlagBM,
                   RRange,
                   mwpXML,            /* response format */
                   (void *)RE,
                   (void **)&S->SDE,  /* O (populated) */
                   MMAX_BUFFER,
                   NSIndex,
                   &IsComplete,
                   EMsg) == FAILURE)
               {
               MUISAddData(S,EMsg);

               return(FAILURE);
               }

             break;
          }    /* END switch (S->WireProtocol) */

        if (IsComplete == TRUE)
          {
          break;
          }
        }      /* END for (NSIndex = 0) */
      }        /* END (case mxoCluster) */

      break;

    case mxoxStats:

      {
      enum MXMLOTypeEnum OIndex;

      char *ptr;
      char *TokPtr;

      enum MStatAttrEnum AList[3];

      AList[0] = (enum MStatAttrEnum)MUGetIndexCI(
        TypeString,
        MStatAttr,
        FALSE,
        mstaNONE);

      AList[1] = mstaStartTime;
      AList[2] = mstaNONE;
      
      /* FORMAT:  <CREDTYPE>[:<CREDID>] */

      if ((ptr = MUStrTok(ArgString," :\n\t",&TokPtr)) != NULL)
        {
        OIndex = (enum MXMLOTypeEnum)MUGetIndexCI(ptr,MXOC,FALSE,mxoNONE);

        ptr = MUStrTok(NULL," :\n\t",&TokPtr);
        }  /* END if ((ptr = MUStrTok() != NULL)) */
      else
        {
        /* attempt to extract object index from 'attr' attribute */

        OIndex = (enum MXMLOTypeEnum)MUGetIndexCI(AttrString,MXOC,FALSE,mxoNONE);
        }

      if (strstr(FlagString,"time"))
        {
        /* display interval statistics */

        switch (S->WireProtocol)
          {
          default:

            /* must pass specified credtype */

            MCredShowStats(
              OIndex,
              ptr,
              (AList[0] != mstaNONE) ? AList : NULL,
              TRUE,
              mfmXML,
              (void **)&S->SDE,
              TRUE,
              0);

            break;
          }  /* END switch (S->WireProtocol) */
        }
      else
        {
        /* display current statistics */
       
        switch (S->WireProtocol)
          {
          default:

            /* must pass specified credtype */

            MCredShowStats(
              OIndex,
              ptr,
              (AList[0] != mstaNONE) ? AList : NULL,
              FALSE,
              mfmXML,
              (void **)&S->SDE,
              TRUE,
              0);

            break;
          }  /* END switch (S->WireProtocol) */
        }
      }    /* END BLOCK */

      break;

    case mxoxLimits:

      /* NYI */

      break;

    default:

      MUISAddData(S,"ERROR:    object not handled\n");

      return(FAILURE);

      /*NOTREACHED*/

      break;
    }  /* END switch (OIndex) */

  return(SUCCESS);
  }  /* END MUIShow() */
/* END MUIShow.c */
