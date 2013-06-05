/* HEADER */

      
/**
 * @file MObjectLoadPvtConfig.c
 *
 * Contains: MOLoadPvtConfig 
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 * Load config info for private/secure CLIENTCFG parameter.
 *
 * @see MSysLoadConfig() - parent - general config load
 * @see MRMInitialize() - parent - RM-specific config load
 *
 * @param OP (I) [optional]
 * @param OIndex (I) [optional,mxoNONE=notset]
 * @param OName (I) [optional]
 * @param PP (I/O) [optional - if *PP non-NULL, populate, else return pointer]
 * @param ConfigPathName (I) [optional]
 * @param ConfigBuf (I) [optional]
 */

int MOLoadPvtConfig(

  void   **OP,               /* I (optional) */
  enum MXMLOTypeEnum OIndex, /* I (optional,mxoNONE=notset) */
  char    *OName,            /* I (optional) */
  mpsi_t **PP,               /* I/O (optional - if *PP non-NULL, populate, else return pointer) */
  char    *ConfigPathName,   /* I (optional) */
  char    *ConfigBuf)        /* I (optional) */

  {
  char *ptr;
  char *curptr;
  char *TokPtr;

  char  IndexName[MMAX_NAME];
  char  tmpLine[MMAX_LINE];
  char  ValLine[MMAX_LINE];

  int   aindex;

  mpsi_t *P = NULL;

  if (ConfigBuf != NULL)
    {
    curptr = ConfigBuf;
    }
  else 
    {
    if (MSched.PvtConfigBuffer == NULL)
      {
      int count;
      int SC;

      if (ConfigPathName == NULL)
        {
        return(FAILURE);
        }

      /* load/cache private config data */

      if ((MSched.PvtConfigBuffer = MFULoad(
            ConfigPathName,
            1,
            macmWrite,
            &count,
            NULL,
            &SC)) == NULL)
        {
        MDB(6,fCONFIG) MLog("WARNING:  cannot load file '%s'\n",
          ConfigPathName);

        return(FAILURE);
        }
      else if (MSched.PvtConfigFile == NULL)
        {
        MUStrDup(&MSched.PvtConfigFile,ConfigPathName);
        }
 
      MCfgAdjustBuffer(&MSched.PvtConfigBuffer,TRUE,NULL,FALSE,TRUE,FALSE);
      }  /* END if (MSched.PvtConfigBuffer == NULL) */

    curptr = MSched.PvtConfigBuffer;
    }  /* END else (ConfigBuf != NULL) */

  /* FORMAT:  [CSKEY=<X>][CSALGO=<X>][HOST=<X>][PORT=<X>][PROTOCOL=<X>][VERSION=<X>] */
  
  if ((OIndex != mxoNONE) && (OP != NULL))
    {
    /* obtain object name */

    switch (OIndex)
      {
      case mxoAM:

        sprintf(IndexName,"AM:%s",
          (OName != NULL) ? OName : ((mam_t *)OP)->Name);

        break;

      case mxoRM:

        sprintf(IndexName,"RM:%s",
          (OName != NULL) ? OName : ((mrm_t *)OP)->Name);

        break;

      case mxoSched:

        sprintf(IndexName,"SCHED:%s",
          (OName != NULL) ? OName : ((mrm_t *)OP)->Name);

        break;

      case mxoNONE:
      default:

        /* not handled */

        return(FAILURE);

        /*NOTREACHED*/

        break;
      }  /* END switch (OIndex) */

    /* load config info */

    if (MCfgGetSVal(
          MSched.PvtConfigBuffer,
          &curptr,
          MParam[mcoClientCfg],
          IndexName,
          NULL,
          tmpLine,
          sizeof(tmpLine),
          0,
          NULL) == FAILURE)
      {
      /* cannot locate client config info */

      return(SUCCESS);
      }

    /* process value/set attributes */
   
    ptr = MUStrTokE(tmpLine," \t\n",&TokPtr);

    while (ptr != NULL)
      {
      /* parse name-value pairs */

      /* FORMAT:  <ATTR>=<VALUE>[,<VALUE>] */

      if (MUGetPair(
            ptr,
            (const char **)MOServAttr,
            &aindex,
            NULL,
            TRUE,
            NULL,
            ValLine,
            sizeof(ValLine)) == FAILURE)
        {
        ptr = MUStrTokE(NULL," \t\n",&TokPtr);

        continue;
        }  /* END if (MUGetPair() == FAILURE) */

      switch (aindex)
        {
        case mosaCSKey:  /* deprecated */
        case mosaKey:

          switch (OIndex)
            {
            case mxoRM:

              MRMSetAttr((mrm_t *)OP,mrmaCSKey,(void **)ValLine,mdfString,mSet);

              break;

            case mxoAM:

              MAMSetAttr((mam_t *)OP,mamaCSKey,(void **)ValLine,mdfString,mSet);

              break;

            case mxoNONE:
            default:

              /* NO-OP */

              break;
            }  /* END switch (OIndex) */

          break;

        case mosaAuthType:
        case mosaCSAlgo:  /* deprecated */

          switch (OIndex)
            {
            case mxoRM:

              MRMSetAttr((mrm_t *)OP,mrmaCSAlgo,(void **)ValLine,mdfString,mSet);

              break;

            case mxoAM:

              MAMSetAttr((mam_t *)OP,mamaCSAlgo,(void **)ValLine,mdfString,mSet);

              break;

            default:

              /* NO-OP */

              break;
            }  /* END switch (OIndex) */

          break;
    
        default:

          /* NO-OP */

          break;
        }  /* END switch (AIndex) */

      ptr = MUStrTokE(NULL," \t\n",&TokPtr);
      }    /* END while (ptr != NULL) */
    }      /* END if (OIndex != mxoNONE) */
  else
    {
    /* load general client private attributes */

    if (OName != NULL)
      {
      switch (OIndex)
        {
        case mxoAM:

          sprintf(IndexName,"AM:%s",
            OName);

          break;

        case mxoRM:

          sprintf(IndexName,"RM:%s",
            OName);

          break;

        default:

          strcpy(IndexName,OName);

          break;
        }  /* END switch (OIndex) */
      }    /* END if (OName != NULL) */
    else
      {
      IndexName[0] = '\0';
      }

    while (MCfgGetSVal(
        MSched.PvtConfigBuffer,
        &curptr,
        MParam[mcoClientCfg],
        IndexName,
        NULL,
        tmpLine,
        sizeof(tmpLine),
        0,
        NULL) == SUCCESS)
      {
      /* FORMAT:  INDEXNAME: <CLIENT_NAME>|HOST:<TNAME>|RM:<NAME>|AM:<NAME>|SCHED:<NAME>  */

      if (OP != NULL)
        {
        if (!strncmp("RM:",IndexName,strlen("RM:")) ||
            !strncmp("AM:",IndexName,strlen("AM:")))
          {
          if (OName == NULL)
            {
            IndexName[0] = '\0';
            }
  
          continue;
          }
        }

      if ((PP != NULL) && (*PP != NULL))
        {
        P = *PP;

        if (!strncasecmp(IndexName,"HOST:",strlen("HOST:")))
          {
          char *IPtr;

          IPtr = IndexName + strlen("HOST:");

          MUStrDup(&P->HostName,IPtr);

          MUGetIPAddr(P->HostName,&P->IPAddr);
          }

        /* process value/set attributes */

        ptr = MUStrTokE(tmpLine," \t\n",&TokPtr);

        while (ptr != NULL)
          {
          /* parse name-value pairs */

          /* FORMAT:  <ATTR>=<VALUE>[,<VALUE>] */

          if (MUGetPair(
                ptr,
                (const char **)MOServAttr,
                &aindex,
                NULL,
                TRUE,
                NULL,
                ValLine,
                sizeof(ValLine)) == FAILURE)
            {
            ptr = MUStrTokE(NULL," \t\n",&TokPtr);

            continue;
            }  /* END if (MUGetPair() == FAILURE) */

          switch (aindex)
            {
            case mosaAuthType:
            case mosaCSAlgo:  /* deprecated */

              {
              char *ptr;
              char *TokPtr;

              /* NOTE: allow AuthType but not CSAlgo, see comment above */

              ptr = MUStrTok(ValLine,":'\"",&TokPtr);

              P->CSAlgo = (enum MChecksumAlgoEnum)MUGetIndexCI(ptr,MCSAlgoType,FALSE,mcsaNONE);

              ptr = MUStrTok(NULL,":'\"",&TokPtr);

              MUStrDup(&P->DstAuthID,ptr);
              }  /* END BLOCK */

              break;

            case mosaDropBadRequest:

              P->DropBadRequest = MUBoolFromString(ValLine,FALSE);

              if (!strcasecmp(P->Name,"DEFAULT"))
                MSched.DefaultDropBadRequest = P->DropBadRequest;

              break;

            case mosaCSKey:  /* deprecated */
            case mosaKey:

              MUStrDup(&P->CSKey,ValLine);

              break;

            case mosaHost:

              MUStrDup(&P->HostName,ValLine);
              MUGetIPAddr(P->HostName,&P->IPAddr);

              break;

            case mosaPort:

              P->Port = (int)strtol(ValLine,NULL,10);

              break;

            case mosaProtocol:

              P->SocketProtocol = (enum MSocketProtocolEnum)MUGetIndexCI(
                ValLine,
                MSockProtocol,
                FALSE,
                0);

              break;

            case mosaVersion:

              MUStrDup(&P->Version,ValLine);

              break;

            default:

              /* not handled */

              break;
            }  /* END switch (aindex) */

          ptr = MUStrTokE(NULL," \t\n",&TokPtr);
          }    /* END while (ptr != NULL) */
        }      /* END if (P != NULL) */
      else
        {
        char *IPtr;

        enum MPeerServiceEnum PType;

        /* cache general client info */

        PType = mpstNONE;

        IPtr = IndexName;

        if (!strncasecmp("SCHED:",IndexName,strlen("SCHED:")))
          {
          PType = mpstSC;
          IPtr  = IndexName + strlen("SCHED:");
          }
        else if (!strncasecmp("RM:",IndexName,strlen("RM:")))
          {
          PType = mpstNM;
          IPtr  = IndexName + strlen("RM:");
          }
        else if (!strncasecmp("AM:",IndexName,strlen("AM:")))
          {
          IPtr  = IndexName + strlen("AM:");
          }
        else if (!strncasecmp("HOST:",IndexName,strlen("HOST:")))
          {
          IPtr = IndexName + strlen("HOST:");
          }

        if (MPeerAdd(IPtr,&P) == FAILURE)
          {
          /* cannot add new peer */

          continue;
          }

        if (!strncasecmp(IndexName,"HOST:",strlen("HOST:")))
          {
          MUStrDup(&P->HostName,IPtr);
          MUGetIPAddr(P->HostName,&P->IPAddr);
          }

        if (PType != mpstNONE)
          P->Type = PType;

        /* process value/set attributes */

        ptr = MUStrTokE(tmpLine," \t\n",&TokPtr);

        while (ptr != NULL)
          {
          /* parse name-value pairs */

          /* FORMAT:  <ATTR>=<VALUE>[,<VALUE>] */

          if (MUGetPair(
                ptr,
                (const char **)MOServAttr,
                &aindex,
                NULL,
                TRUE,
                NULL,
                ValLine,
                sizeof(ValLine)) == FAILURE)
            {
            ptr = MUStrTokE(NULL," \t\n",&TokPtr);

            continue;
            }  /* END if (MUGetPair() == FAILURE) */

          /* NOTE:  do not support algo specification (why are we saying this?) */

          switch (aindex)
            {
            case mosaAuth:

              {
              char tmpLine[MMAX_LINE];

              /* add peer to admin list */

              P->RIndex = (enum MRoleEnum)MUGetIndexCI(ValLine,MRoleType,FALSE,0);

              sprintf(tmpLine,"PEER:%s",
                P->Name);

              MSchedAddAdmin(tmpLine,P->RIndex);

              /* disable mrsvctl (only enabled when RsvImport is set) */

              /* MUI[mcsMRsvCtl].AdminAccess[P->RIndex] = FALSE; */
              }  /* END (case mosaAuth) */

              break;

            case mosaAuthType:
            /* case mosaCSAlgo: */

              {
              char *ptr;
              char *TokPtr;

              /* NOTE: allow AuthType but not CSAlgo, see comment above */

              ptr = MUStrTok(ValLine,":'\"",&TokPtr);

              P->CSAlgo = (enum MChecksumAlgoEnum)MUGetIndexCI(
                ptr,
                MCSAlgoType,
                FALSE,
                mcsaNONE);

              ptr = MUStrTok(NULL,":'\"",&TokPtr);

              MUStrDup(&P->DstAuthID,ptr);
              }  /* END BLOCK */

              break;

            case mosaDropBadRequest:

              P->DropBadRequest = MUBoolFromString(ValLine,FALSE);

              if (!strcasecmp(P->Name,"DEFAULT"))
                MSched.DefaultDropBadRequest = P->DropBadRequest;

              break;

            case mosaCSKey: /* deprecated */
            case mosaKey:

              MUStrDup(&P->CSKey,ValLine);

              break;

            case mosaHost:

              MUStrDup(&P->HostName,ValLine);
              MUGetIPAddr(P->HostName,&P->IPAddr);

              break;

            case mosaPort:

              P->Port = (int)strtol(ValLine,NULL,10);

              break;

            default:

              /* NO-OP */

              break;
            }  /* END switch (aindex) */

          ptr = MUStrTokE(NULL," \t\n",&TokPtr);
          }  /* END while (ptr != NULL) */
        }    /* END else (PP != NULL) */

      /* reset IndexName */

      if (OName == NULL)
        {
        IndexName[0] = '\0';
        }
      }      /* END while (MCfgGetSVal() == SUCCESS) */

    /* Event log web service params */

    if (MCfgGetSVal(
        MSched.PvtConfigBuffer,
        &curptr,
        MParam[mcoEventLogWSUser],
        IndexName,
        NULL,
        tmpLine,
        sizeof(tmpLine),
        0,
        NULL) == SUCCESS)
      {
      if (MSched.WebServicesConfigured == TRUE)
        MUStrDup(&MSched.EventLogWSUser,tmpLine);
      else
        MDB(2,fCONFIG) MLog("ERROR:     Web Services is not enabled in this build of moab, cannot set event log web service URL\n");
      }

    if (MCfgGetSVal(
        MSched.PvtConfigBuffer,
        &curptr,
        MParam[mcoEventLogWSPassword],
        IndexName,
        NULL,
        tmpLine,
        sizeof(tmpLine),
        0,
        NULL) == SUCCESS)
      {
      if (MSched.WebServicesConfigured == TRUE)
        MUStrDup(&MSched.EventLogWSPassword,tmpLine);
      else
        MDB(2,fCONFIG) MLog("ERROR:     Web Services is not enabled in this build of moab, cannot set event log web service URL\n");
      }

    if (MCfgGetSVal(
        MSched.PvtConfigBuffer,
        &curptr,
        MParam[mcoMongoUser],
        IndexName,
        NULL,
        tmpLine,
        sizeof(tmpLine),
        0,
        NULL) == SUCCESS)
      {
      MUStrDup(&MSched.MongoUser,tmpLine);
      }

    if (MCfgGetSVal(
        MSched.PvtConfigBuffer,
        &curptr,
        MParam[mcoMongoPassword],
        IndexName,
        NULL,
        tmpLine,
        sizeof(tmpLine),
        0,
        NULL) == SUCCESS)
      {
      MUStrDup(&MSched.MongoPassword,tmpLine);
      }

    }        /* END else (OIndex != mxoNONE) */

  if ((PP != NULL) && (*PP == NULL))
    {
    *PP = P;
    }

  return(SUCCESS);
  }  /* END MOLoadPvtConfig() */

/* END MObjectLoadPvtConfig.c */
