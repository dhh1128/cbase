/* HEADER */

/**
 * @file MUIRsvCtl.c
 *
 * Contains MUI Rsv function
 */



#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 * Process mrsvctl (Reservation Control) client requests.
 *
 * NOTE:  also handles deprecated commands setres and releaseres
 *
 * @see MUIRsvCreate() - child - create new reservation
 * @see MUIRsvCtlModify() - child - modifies existing rsvs
 *
 * @param S (I)
 * @param AFlags (I) [bitmap of enum MRoleEnum]
 * @param Auth (I)
 */

int MUIRsvCtl(

  msocket_t *S,       /* I */
  mbitmap_t *AFlags,  /* I credential flags (bitmap of enum MRoleEnum) */
  char      *Auth)    /* I */

  {
  char Command[MMAX_NAME];
  char RsvExp[MMAX_LINE];

  char FlagString[MMAX_LINE];
  char ArgString[MMAX_LINE];

  marray_t RsvList;

  char tmpLine[MMAX_LINE << 3];

  char tmpName[MMAX_LINE];
  char tmpVal[MMAX_LINE];

  enum MRsvCtlCmdEnum CIndex;

  enum MRsvAttrEnum AIndex;

  int  rc;

  mrsv_t *R;

  mrm_t  *RM;

  mgcred_t *U = NULL;

  mbool_t IsAdmin;

  mxml_t *RE = NULL;
  mxml_t *WE = NULL;
  mxml_t *DE = NULL;

  int     rindex;

  mbitmap_t  Flags;

  mbool_t UseUIndex = FALSE;
  mbool_t UseGName  = FALSE;
  mbool_t AppendRsvExp = FALSE;

  mgcred_t *RU = NULL;
  mgcred_t *RG = NULL;
  mgcred_t *RA = NULL;
  mqos_t   *RQ = NULL;

  enum MFormatModeEnum DFormat = mfmNONE;

  const char *FName = "MUIRsvCtl";

  MDB(2,fUI) MLog("%s(S,%s)\n",
    FName,
    (Auth != NULL) ? Auth : "NULL");

  if (S == NULL)
    {
    return(FAILURE);
    }

  /* NOTE:  support create, destroy, list, modify */

  /* initialize values */

  S->WireProtocol = mwpS32;

  RsvExp[0] = '\0';

  /* process request */

  switch (S->WireProtocol)
    {
    case mwpXML:
    case mwpS32:

      {
      int WTok;
   
      /* FORMAT:  <Request action="X" object="Y"><Where/></Request> */

      char *ptr;

      if (S->RDE != NULL)
        {
        RE = S->RDE;
        }
      else if ((S->RPtr != NULL) &&
              ((ptr = strchr(S->RPtr,'<')) != NULL) &&
               (MXMLFromString(&RE,ptr,NULL,NULL) == FAILURE))
        {
        MDB(3,fUI) MLog("WARNING:  corrupt command '%100.100s' received\n",
          S->RBuffer);

        MUISAddData(S,"ERROR:    corrupt command received\n");

        return(FAILURE);
        }

      S->RDE = RE;

      if (MXMLGetAttr(RE,MSAN[msanAction],NULL,Command,sizeof(Command)) == FAILURE)
        {
        MDB(3,fUI) MLog("WARNING:  cannot locate command '%100.100s'\n",
          S->RBuffer);

        MUISAddData(S,"ERROR:    cannot locate command\n");

        return(FAILURE);
        }

      WTok = -1;

      while (MS3GetWhere(
          RE,
          &WE,
          &WTok,
          tmpName,          /* O */
          sizeof(tmpName),
          tmpVal,           /* O */
          sizeof(tmpVal)) == SUCCESS)
        {
        /* process 'where' constraints */

        AIndex = (enum MRsvAttrEnum)MUGetIndexCI(tmpName,MRsvAttr,FALSE,mraNONE);

        if ((AIndex == mraName) || (AIndex == mraUIndex) || (AIndex == mraRsvGroup))
          {
          if (AppendRsvExp == TRUE)
            {
            MUStrCat(RsvExp,",",sizeof(RsvExp));
            MUStrCat(RsvExp,tmpVal,sizeof(RsvExp));
            }
          else
            {
            MUStrCpy(RsvExp,tmpVal,sizeof(RsvExp));

            AppendRsvExp = TRUE;
            }

          if (AIndex == mraUIndex)
            UseUIndex = TRUE;
          else if (AIndex == mraRsvGroup)
            UseGName = TRUE;
          } 
        else
          {
          switch (AIndex)
            {
            case mraAAccount:

              if (MAcctFind(tmpVal,&RA) == FAILURE)
                {
                MUISAddData(S,"ERROR:    cannot locate requested account\n");

                return(FAILURE);
                }

              break;

            case mraAGroup:

              if (MGroupFind(tmpVal,&RG) == FAILURE)
                {
                MUISAddData(S,"ERROR:    cannot locate requested group\n");

                return(FAILURE);
                }

              break;

            case mraAQOS:

              if (MQOSFind(tmpVal,&RQ) == FAILURE)
                {
                MUISAddData(S,"ERROR:    cannot locate requested qos\n");

                return(FAILURE);
                }

              break;

            case mraAUser:

              if (MUserFind(tmpVal,&RU) == FAILURE)
                {
                MUISAddData(S,"ERROR:    cannot locate requested user\n");

                return(FAILURE);
                }

              break;

            default:

              MUISAddData(S,"ERROR:   incorrect reservation attribute\n");

              return(FAILURE);

              /* NOTREACHED */

              break;
            }   /* END switch (AIndex) */
          }     /* END else */
        }       /* END while (MXMLGetChild() == SUCCESS) */

      if (MXMLGetAttr(RE,MSAN[msanFlags],NULL,FlagString,sizeof(FlagString)) == SUCCESS)
        {
        bmfromstring(FlagString,MClientMode,&Flags);

        if (bmisset(&Flags,mcmXML))
          {
          DFormat = mfmXML;
          }
        else
          {
          DFormat = mfmNONE;
          }
        }

      MXMLGetAttr(RE,MSAN[msanArgs],NULL,ArgString,sizeof(ArgString));
      }  /* END BLOCK (case mwpXML/mwpS3) */

      break;

    default:

      /* not supported */

      MUISAddData(S,"ERROR:    corrupt command received\n");

      return(FAILURE);

      /*NOTREACHED*/

      break;
    }  /* END switch (S->WireProtocol) */

  /* process data */

  if ((CIndex = (enum MRsvCtlCmdEnum)MUGetIndexCI(
         Command,
         MRsvCtlCmds,
         FALSE,
         mrcmNONE)) == mrcmNONE)
    {
    MDB(3,fUI) MLog("WARNING:  unexpected subcommand '%s' received\n",
      Command);

    sprintf(tmpLine,"ERROR:    unexpected subcommand '%s'\n",
      Command);

    MUISAddData(S,tmpLine);

    return(FAILURE);
    }

  MUserAdd(Auth,&U);

  S->SBuffer[0] = '\0';

  switch (CIndex)
    {
      case mrcmCreate:
        {
        marray_t RList;

        char Buffer[MMAX_BUFFER];
        int  rc;

        Buffer[0] = 0;

        /* check for unauthorized peers exporting rsv */

        if ((Auth != NULL) && 
            (!strncasecmp(Auth,"peer:",strlen("peer:"))))
          {
          if (MRMFind(&Auth[strlen("peer:")],&RM) == FAILURE)
            {
            /* NYI */
            }
          else if (!bmisset(&RM->Flags,mrmfRsvImport))
            {
            MUISAddData(S,"rsv import not enabled for peer");
    
            return(FAILURE);
            }
          }

        MUArrayListCreate(&RList,sizeof(mrsv_t *),50);

        rc = MUIRsvCreate(
              RE,
              Auth,
              Buffer,          /* O */
              sizeof(Buffer),
              TRUE,
              FALSE,
              FALSE,
              &RList);
    
        if (DFormat == mfmXML)
          {
          if (MXMLCreateE(&S->SDE,(char *)MSON[msonData]) == FAILURE)
            {
            rc = FAILURE;
    
            MUISAddData(S,"ERROR:    internal error, cannot create response\n");
            }
          else if (rc == SUCCESS)
            {
            mrsv_t *R;
    
            int rindex;
    
            mxml_t *RE;
    
            DE = S->SDE;
    
            for (rindex = 0;rindex < RList.NumItems;rindex++)
              {
              R = (mrsv_t *)MUArrayListGetPtr(&RList,rindex);

              RE = NULL;

              MXMLCreateE(&RE,(char *)MXO[mxoRsv]);

              MXMLSetVal(RE,R->Name,mdfString);

              MXMLAddE(DE,RE);
              }
            }
          else
            {
            MUISAddData(S,Buffer);
            }
          } /* END if (DFormat == mfmXML) */
        else
          {
          MUISAddData(S,Buffer);
          }

        MUArrayListFree(&RList);

        return(rc);
        }    /* END case mrcmCreate: */

      case mrcmFlush:
        {
        msrsv_t *SR;
    
        if (MSRFind(RsvExp,&SR,NULL) == SUCCESS)
          {
          int sindex;
    
          for (sindex = 0;sindex < MMAX_SRSV_DEPTH;sindex++)
            {
            SR->DisabledTimes[sindex] = 0;
            } 

          snprintf(tmpLine,sizeof(tmpLine),"SR[%s] successfully cleared disabled times\n",
            SR->Name);

          MUISAddData(S,tmpLine);

          return(SUCCESS);
          }
        }

      case mrcmQuery:

        bmset(&S->Flags,msftReadOnlyCommand);

        if (!strcasecmp(ArgString,"profile"))
          {
          mxml_t *DE;

          if ((S->SDE == NULL) && 
              (MXMLCreateE(&S->SDE,(char *)MSON[msonData]) == FAILURE))
            {
            MUISAddData(S,"ERROR:    cannot create response\n");

            return(FAILURE);
            }

          DE = S->SDE;

          return(__MUIRsvProfShow(S,U,RsvExp,&DE,NULL));
          }

      case mrcmList:

        bmset(&S->Flags,msftReadOnlyCommand);

        break;

      default:

        break;

    } /* end switch(CIndex) */

  MUArrayListCreate(&RsvList,sizeof(mrsv_t *),1);

  if (UseUIndex == TRUE)
    {
    char *ptr;
    char *TokPtr;

    /* locate by unique index */

    /* FORMAT:  <ID>[,<ID>]... */

    /* NOTE:  rsv id in format '<NAME>.<UID>' */

    ptr = MUStrTok(RsvExp,", \t\n:",&TokPtr);

    while (ptr != NULL)
      {
      if (MRsvFind(ptr,&R,mraUIndex) == SUCCESS)
        {
        MUArrayListAppendPtr(&RsvList,R);

        MDB(6,fUI) MLog("INFO:     reservation '%s' located by index\n",
          R->Name);
        }
      else
        {
        snprintf(tmpLine,sizeof(tmpLine),"ERROR:    cannot locate reservation %s\n",
          ptr);

        MUISAddData(S,tmpLine);
        }

      ptr = MUStrTok(NULL,", \t\n:",&TokPtr);
      }  /* END while (ptr != NULL) */
    }    /* END if (UseUIndex == TRUE) */
  else if (UseGName == TRUE)
    {
    rsv_iter RTI;

    char *ptr;
    char *TokPtr;

    /* allow multiple group selection */
    /* FORMAT:  <GID>[,<GID>]... */

    ptr = MUStrTok(RsvExp,", \t:",&TokPtr);

    while (ptr != NULL)
      {
      MRsvIterInit(&RTI);

      while (MRsvTableIterate(&RTI,&R) == SUCCESS)
        {
        if (R->RsvGroup == NULL)
          continue;

        if (strcmp(R->RsvGroup,ptr))
          continue;

        MUArrayListAppendPtr(&RsvList,R);

        MDB(6,fUI) MLog("INFO:     reservation '%s' located by group '%s'\n",
          R->Name,
          ptr);
        }  /* END while (MRsvTableIterate()) */

      ptr = MUStrTok(NULL,", \t:",&TokPtr);
      }    /* END while (ptr != NULL) */
    }    /* END if (UseGName == TRUE) */
  else
    {
    if (RsvExp[0] == '\0')
      {
      /* no reservation name received */

      snprintf(tmpLine,sizeof(tmpLine),"ERROR:    no reservation specified\n");

      MUISAddData(S,tmpLine);

      MUArrayListFree(&RsvList);

      return(FAILURE);
      }
    else if ((MUStringIsRE(RsvExp) == TRUE) || (!strcmp(RsvExp,"ALL")))
      {
      /* regular expression or special expression received */

      char tmpBuf[MMAX_BUFFER];
 
      tmpBuf[0] = '\0';

      if (MUREToList(
            RsvExp,
            mxoRsv,
            NULL,
            &RsvList,
            FALSE,
            tmpBuf,
            sizeof(tmpBuf)) == FAILURE)
        {
        snprintf(tmpLine,sizeof(tmpLine),"ERROR:    invalid expression '%s' : %s\n",
          RsvExp,
          tmpBuf);

        MUISAddData(S,tmpLine);

        MUArrayListFree(&RsvList);

        return(FAILURE);
        }
      }
    else
      {
      /* single reservation specified */

      mrsv_t *tmpR = NULL;

      if (MRsvFind(RsvExp,&tmpR,mraNONE) == FAILURE)
        {
        snprintf(tmpLine,sizeof(tmpLine),"ERROR:    invalid reservation specified '%s'\n",
          RsvExp);

        MUISAddData(S,tmpLine);

        MUArrayListFree(&RsvList);

        return(FAILURE);
        }
      else
        {
        MUArrayListAppendPtr(&RsvList,tmpR);
        }
      }   /* END else */

    if (RsvList.NumItems == 0)
      {
      if (!strcmp(RsvExp,"ALL"))
        {
        MUArrayListFree(&RsvList);

        return(SUCCESS);
        }
      else
        {
        char tmpLine[MMAX_LINE];

        snprintf(tmpLine,sizeof(tmpLine),"ERROR:  '%s' does not match any reservations\n",
          RsvExp);

        MUISAddData(S,tmpLine);

        MUArrayListFree(&RsvList);

        return(FAILURE);
        }
      }  /* END if (RsvList.NumItems == 0) */
    }    /* END else (UseUIndex == TRUE) */

  MDB(2,fUI) MLog("INFO:     performing '%s' operation on rsv expression '%s' (%d matches)\n",
    MRsvCtlCmds[CIndex],
    RsvExp,
    RsvList.NumItems);

  if (RsvList.NumItems <= 0)
    {
    /* no reservations found */

    sprintf(tmpLine,"ERROR:  could not locate any matching reservations\n");

    MUISAddData(S,tmpLine);

    MUArrayListFree(&RsvList);

    return(FAILURE);
    }
  
  rc = SUCCESS;

  for (rindex = 0;rindex < RsvList.NumItems;rindex++)
    {
    R = (mrsv_t *)MUArrayListGetPtr(&RsvList,rindex);
     
    /*
    if (((RA != NULL) && (J->Cred.A != RA)) ||
        ((RG != NULL) && (J->Cred.G != RG)) ||
        ((RQ != NULL) && (J->Cred.Q != RQ)) ||
        ((RU != NULL) && (J->Cred.U != RU)))
      {

      continue;
      }
    */
     
    /* get authorization */

    if (MUICheckAuthorization(
          U,
          NULL,
          (void *)R,
          mxoRsv,
          mcsMRsvCtl,
          CIndex,
          &IsAdmin,
          tmpLine,
          sizeof(tmpLine)) == FAILURE)
      {
      /* NOTE:  if rsv list is requested, do not report - 'not authorized' message
                for each rsv. */

      if ((CIndex != mrcmList) && (CIndex != mrcmQuery))
        {
        MUISAddData(S,tmpLine);
        MUISAddData(S,"\n");

        /* allow errno to be set on the command line to indicate error */

        rc = FAILURE;
        }

      /* If the final request failed, return failure otherwise continue
         to the next request */
      if (RsvList.NumItems - 1 == rindex)
        {
        MUArrayListFree(&RsvList);

        /* last request failed */

        if (CIndex == mrcmQuery)
          {
          /* Failed just means no reservations reported */

          return(SUCCESS);
          }

        return(FAILURE);
        }
      else
        {
        /* failure is recorded but entire request hasn't failed */

        continue;
        }
      }

    switch (CIndex)
      {
      case mrcmAlloc:

        {
        char EMsg[MMAX_LINE] = {0};
        char tmpName[MMAX_NAME];
        char tmpVal[MMAX_NAME];

        char GResType[MMAX_NAME];

        int len;

        GResType[0] = '\0';

        /* allocate additional requested resources to specified rsv */

        /* FORMAT:  mrsvctl -A gres.type=dvd vpc.3 */   

        /* parse request */

        if (MS3GetSet(
             RE,
             NULL,
             NULL,
             tmpName,
             sizeof(tmpName),
             tmpVal,
             sizeof(tmpVal)) == FAILURE)
          {
          sprintf(tmpLine,"ERROR:  no allocation resource specified\n");

          MUISAddData(S,tmpLine);

          MUArrayListFree(&RsvList);

          return(FAILURE);
          }
     
        if (!strcasecmp(tmpName,"resource"))
          {
          len = strlen("gres.type=");

          if (!strncasecmp(tmpVal,"gres.type=",len))
            {
            MUStrCpy(GResType,&tmpVal[len],sizeof(GResType));
            }
          }

        if (GResType[0] == '\0')
          {
          sprintf(tmpLine,"ERROR:  no allocation resource specified\n");

          MUISAddData(S,tmpLine);

          MUArrayListFree(&RsvList);

          return(FAILURE);
          }
 
        /* create pseudo job */
        rc = MRsvAllocateGResType(R,GResType,EMsg);

        MUISAddData(S,EMsg);

        MUArrayListFree(&RsvList);

        return(rc);
        }  /* END case mrcmAlloc */

        /*NOTREACHED*/

        break;

      case mrcmCreate:

        {
        /* should not be reached (handled above) */

        rc = FAILURE;

        sprintf(tmpLine,"ERROR:  create failed - internal error\n");

        MUISAddData(S,tmpLine);

        MUArrayListFree(&RsvList);

        return(rc);
        }  /* END case mrcmCreate */

        /*NOTREACHED*/

        break;

      case mrcmDestroy:

        {
        char tmpName[MMAX_NAME];

        if (bmisset(&R->Flags,mrfStatic))
          {
          sprintf(tmpLine,"ERROR:  cannot release 'static' reservation %s\n",
            R->Name);

          MUISAddData(S,tmpLine);

          continue;
          }

        if (bmisset(&R->Flags,mrfParentLock))
          {
          sprintf(tmpLine,"ERROR:  cannot release 'locked' reservation %s - must remove parent object\n",
            R->Name);

          MUISAddData(S,tmpLine);

          continue;
          }

        MUStrCpy(tmpName,R->Name,sizeof(tmpName));

        R->CancelIsPending = TRUE;

        if (bmisset(&R->Flags,mrfStanding))
          {
          int sindex;

          msrsv_t *SR = NULL;

          /* set enabletime to the end of this reservation */

          if (MSRFind(R->RsvParent,&SR,NULL) == SUCCESS)
            {
            for (sindex = 0;sindex < MMAX_SRSV_DEPTH;sindex++)
              {
              if ((SR->DisabledTimes[sindex] == 0) ||
                  (SR->DisabledTimes[sindex] == 1))
                {
                if (R->EndTime == MMAX_TIME - 1)
                  SR->DisabledTimes[sindex] = MMAX_TIME;
                else
                  SR->DisabledTimes[sindex] = R->EndTime;

                break;
                }
              }    /* END for (sindex) */
            }      /* END if (MSRFind(R->RsvGroup,&SR) == SUCCESS) */
          else
            {
            sprintf(tmpLine,"WARNING:  cannot locate rsv parent\n");

            MUISAddData(S,tmpLine);

            MDB(3,fUI) MLog("WARNING:  cannot locate parent '%s' for rsv '%s'\n",
              (R->RsvGroup != NULL) ? R->RsvGroup : "NULL",
              R->Name);
            }
          }        /* END if (bmisset(&R->Flags,mrfStanding)) */

        R->EndTime = MSched.Time - 1;
        R->ExpireTime = 0;

        snprintf(tmpLine,sizeof(tmpLine),"reservation %s destroyed by %s",
          tmpName,
          Auth);
          
        MOWriteEvent(R,mxoRsv,mrelRsvCancel,tmpLine,MStat.eventfp,NULL);

        if ((long)R->StartTime <= MUIDeadLine)
          {
          /* adjust UI phase wake up time */

          if (MSched.AdminEventAggregationTime >= 0)
            MUIDeadLine = MIN(MUIDeadLine,(long)MSched.Time + MSched.AdminEventAggregationTime);
          }

        /* remove reservation */

        if ((R->Type = mrtJob) && (R->J != NULL))
          {
          /* Expiring job reservations are not released in MRsvCheckStatus(), they are extended.
             So to remove the reservation you must do it outside MRsvCheckStatus() */

          MJobReleaseRsv(R->J,TRUE,TRUE);

          R = NULL;
          }
        else
          {
          MRsvCheckStatus(NULL);
          }

        MOSSyslog(LOG_INFO,tmpLine);

        sprintf(tmpLine,"reservation %s successfully released\n",
          tmpName);
        }    /* END case mrcmDestroy */

        MUISAddData(S,tmpLine);

        continue;

        /*NOTREACHED*/

        break;

      case mrcmFlush:

        {
        msrsv_t *SR;

        int sindex;

        if (MSRFind(R->RsvGroup,&SR,NULL) == FAILURE)
          {
          sprintf(tmpLine,"reservation '%s' does not belong to a standing reservation\n",
            R->Name);

          MUISAddData(S,tmpLine);

          rc = FAILURE;

          break;
          }

        for (sindex = 0;sindex < MMAX_SRSV_DEPTH;sindex++)
          {
          SR->DisabledTimes[0] = 0;
          } 

        sprintf(tmpLine,"SR[%s] successfully cleared disabled times\n",
          SR->Name);

        MUISAddData(S,tmpLine);
        }

        break;

      case mrcmJoin:

        {
        char EMsg[MMAX_LINE] = {0};

        mrsv_t *CR = NULL;

        if (MRsvFind(ArgString,&CR,mraNONE) == SUCCESS)
          {
          rc = MRsvJoin(R,CR,EMsg);

          if (rc == FAILURE)
            {
            sprintf(tmpLine,"could not join rsv '%s' to rsv '%s' -- %s\n",
              R->Name,
              ArgString,
              EMsg);

            MUISAddData(S,tmpLine);
            }
          else
            {
            sprintf(tmpLine,"successfully joined rsv '%s' to rsv '%s'\n",
              ArgString,
              R->Name);

            MHistoryAddEvent(R,mxoRsv);

            MUISAddData(S,tmpLine);
            }
          }
        else
          {
          sprintf(tmpLine,"invalid reservation '%s'\n",
            ArgString);

          MUISAddData(S,tmpLine);
          }
        }   /* END case mrcmJoin */

        break;

      case mrcmSignal:

        {
        enum MTrigTypeEnum Signal = (enum MTrigTypeEnum)MUGetIndexCI(ArgString,MTrigType,FALSE,mttNONE);

        if (Signal == mttNONE)
          {
          sprintf(tmpLine,"invalid signal '%s' for rsv '%s'\n",
            ArgString,
            R->Name);

          MUISAddData(S,tmpLine);
          }
        else
          {
          MOReportEvent((void *)R,NULL,mxoRsv,Signal,MSched.Time,TRUE);

          sprintf(tmpLine,"signal '%s' successfully sent to rsv '%s'\n",
            MTrigType[Signal],
            R->Name);

          MUISAddData(S,tmpLine);
          }
        }  /* END BLOCK (case mrcmSignal) */

        break;

      case mrcmList:

        /* NOTE:  only add matching reservations */

        /* NOTE:  global list handled outside of loop */

        MUISAddData(S,R->Name);
        MUISAddData(S," ");

        break;

      case mrcmMigrate:

        {
        char EMsg[MMAX_LINE] = {0};

        mrsv_t *CR = NULL;

        if (MRsvFind(ArgString,&CR,mraNONE) == SUCCESS)
          {
          rc = MRsvMigrate(R,CR,EMsg);

          if (rc == FAILURE)
            {
            sprintf(tmpLine,"could not migrate rsv '%s' to rsv '%s' -- %s\n",
              R->Name,
              ArgString,
              EMsg);

            MUISAddData(S,tmpLine);
            }
          else
            {
            sprintf(tmpLine,"successfully migrated rsv '%s' to rsv '%s'\n",
              ArgString,
              R->Name);

            MHistoryAddEvent(R,mxoRsv);

            MUISAddData(S,tmpLine);
            }
          }
        else
          {
          sprintf(tmpLine,"invalid reservation '%s'\n",
            ArgString);

          MUISAddData(S,tmpLine);
          }
        }   /* END case mrcmJoin */

        break;


        break;

      case mrcmModify:

        {
        char EMsg[MMAX_LINE] = {0};

        rc = MUIRsvCtlModify(R,RE,EMsg);

        MUISAddData(S,EMsg);

        if (rc == FAILURE)
          {
          MUArrayListFree(&RsvList);

          return rc;
          }
        }   /* END BLOCK (case mrcmModify) */

        break;

      case mrcmQuery:

        /* NOTE: "profile" handled above */

        bmset(&S->Flags,msftReadOnlyCommand);

        if (!strcmp(ArgString,"resources"))
          {
          /* list allocated resources */

          if (!MNLIsEmpty(&R->NL))
            {
            MNLToString(
              &R->NL,
              bmisset(&Flags,mcmVerbose),
              ",",
              '\0',
              tmpLine,
              sizeof(tmpLine));

            MUISAddData(S,tmpLine);
            }

          MUArrayListFree(&RsvList);

          return(SUCCESS);
          }  /* END if (!strcmp(ArgString,"resources")) */
        else if (!strcmp(ArgString,"export"))
          {
          mxml_t *DE;

          if ((S->SDE == NULL) && 
              (MXMLCreateE(&S->SDE,(char *)MSON[msonData]) == FAILURE))
            {
            MUISAddData(S,"ERROR:    cannot create response\n");

            MUArrayListFree(&RsvList);

            return(FAILURE);
            }

          DE = S->SDE;
 
          if (MUIExportRsvQuery(
                R,
                NULL,
                Auth,
                DE,
                0) == FAILURE)
            {
            MUArrayListFree(&RsvList);

            return(FAILURE);
            }
          }
        else if (!strcmp(ArgString,"wiki"))
          {
          mstring_t String(MMAX_LINE);

          MRsvToWikiString(R,NULL,&String);

          MUISAddData(S,String.c_str());
          } /* END if (!strcmp(ArgString,"wiki")) */
        else
          {
          /* diagnose/display rsv state/config */

          char    DiagOpt[MMAX_NAME];

          mxml_t *DE = NULL;
          mxml_t *RE = NULL;

          const enum MRsvAttrEnum RAList[] = {
            mraName,
            mraAAccount,
            mraAGroup,
            mraAUser,
            mraAQOS,
            mraAllocNodeCount,
            mraAllocNodeList,
            mraAllocProcCount,
            mraAllocTaskCount,
            mraComment,
            mraEndTime,
            mraExpireTime,
            mraFlags,
            mraGlobalID,
            mraHostExp,
            mraLabel,
            mraLogLevel,
            mraOwner,
            mraPartition,
            mraProfile,
            mraReqArch,
            mraReqFeatureList,
            mraReqMemory,
            mraReqNodeCount,
            mraReqNodeList,
            mraReqOS,
            mraReqTaskCount,
            mraResources,
            mraRsvGroup,
            mraRsvParent,
            mraSID,
            mraStartTime,
            mraStatCAPS,
            mraStatCIPS,
            mraStatTAPS,
            mraStatTIPS,
            mraSubType,
            mraTrigger,
            mraType,
            mraVariables,
            mraVMList,
            mraNONE };

          MUStrCpy(DiagOpt,R->Name,sizeof(DiagOpt));

          /* check to see if the socket's return data xml is initialized */

          if (S->SDE == NULL)
            {
            if (MXMLCreateE(&S->SDE,(char *)MSON[msonData]) == FAILURE)
              {
              MUISAddData(S,"ERROR:    cannot create response\n");

              MUArrayListFree(&RsvList);

              return(FAILURE);
              }
            }

          DE = S->SDE;

          MXMLCreateE(&RE,(char *)MXO[mxoRsv]);

          MXMLAddE(DE,RE);

          MRsvToXML(R,&RE,(enum MRsvAttrEnum *)RAList,NULL,TRUE,mcmNONE);

          /* NOTE:  do not return, append data to buffer and continue */
          }  /* END else() */

        break;

      default:

        /* NO-OP */

        break;
      }        /* END switch (CIndex) */
    }          /* END for (rindex) */

  MUArrayListFree(&RsvList);

  return(rc);
  }  /* END MUIRsvCtl() */
/* END MUIRsvCtl.c */
