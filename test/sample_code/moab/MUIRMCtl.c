/* HEADER */

/**
 * @file MUIRMCtl.c
 *
 * Contains MUI Resource Manager Control
 */



#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 * Handle 'mrmctl' (Resource Manager Control) client requests.
 *
 * @see MCRMCtlCreateRequest() - generates request XML 
 * @see MCRMCtl() - handles response w/in client
 * @see MUIRMCtlModifyDynamicResources() - child - process 'mrmctl -x'
 *
 * @param S (I)
 * @param AFlags (I) [bitmap of enum MRoleEnum]
 * @param Auth (I)
 */

int MUIRMCtl(

  msocket_t *S,
  mbitmap_t *AFlags,
  char      *Auth)

  {
  char Command[MMAX_NAME];

  char FlagString[MMAX_NAME];
  char ArgString[MMAX_NAME];
  char NotifyString[MMAX_LINE];

  char TimeString[MMAX_NAME];

  char SubOID[MMAX_NAME];
  char SubOType[MMAX_NAME];

  char tmpLine[MMAX_LINE << 3];
  char tmpMsg[MMAX_LINE];

  char tmpName[MMAX_LINE];
  char tmpVal[MMAX_LINE];

  enum MRMCtlCmdEnum CIndex;

  mbitmap_t Flags;

  int  rc;

  mrm_t *R;
  mam_t *A  = NULL;
  midm_t *I = NULL;
  mpar_t *P = NULL;

  mxml_t *RE = NULL;
  mxml_t *SE = NULL;
  mxml_t *WE = NULL;

  enum MStatusCodeEnum SC;

  enum MFormatModeEnum DFormat;

  mbool_t specifiedRM = FALSE;

  mrm_t *RMList[MMAX_RM + 1];  /* list of RM's to act upon (NULL terminated) */

  mgcred_t *U = NULL;
  
  const char *FName = "MUIRMCtl";

  MDB(2,fUI) MLog("%s(S,%s)\n",
    FName,
    (Auth != NULL) ? Auth : "NULL");

  if (S == NULL)
    {
    return(FAILURE);
    }

  /* NOTE:  support create, destroy, modify */

  S->WireProtocol = mwpS32;

  /* set defaults */

  RMList[0] = &MRM[0];
  RMList[1] = NULL;

  A = NULL;

  SubOType[0] = '\0';
  NotifyString[0] = '\0';

  DFormat = mfmNONE;

  /* evaluate request */

  switch (S->WireProtocol)
    {
    case mwpXML:
    case mwpS32:

      {
      int WTok;

      /* FORMAT:  <Request action="X" rsv="X"></Request> */

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

        MUISAddData(S,"ERROR:    cannot located command\n");

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
        /* process 'where' constraints */

        if (!strcmp(tmpName,MSAN[msanName]) ||
            !strcmp(tmpName,MXO[mxoRM]))
          {
          if (!strncasecmp(tmpVal,"am",strlen("am")))
            {
            char *ptr;

            ptr = tmpVal + strlen("am");

            if (ptr[0] == '\0')
              {
              A = &MAM[0];
              }
            else if (MAMFind(ptr + 1,&A) == FAILURE)
              {
              MDB(3,fUI) MLog("WARNING:  cannot locate rm '%.100s'\n",
                tmpVal);

              MUISAddData(S,"ERROR:    cannot locate am\n");

              return(FAILURE);
              }
            }
          else if (!strncasecmp(tmpVal,"id",strlen("id")))
            {
            /* user can specify ID or ID:rmid */

            char *ptr;

            ptr = tmpVal + strlen("id");

            if (ptr[0] == '\0') /* no rmid specified after ID */
              {
              I = &MID[0];
              }
            else if (MIDFind(ptr + 1,&I) == FAILURE) /* look for rmid after the ':' */
              {
              MDB(3,fUI) MLog("WARNING:  cannot locate rm '%.100s'\n",
                tmpVal);

              MUISAddData(S,"ERROR:    cannot locate id\n");

              return(FAILURE);
              }
            }
          else if (!strncasecmp(tmpVal,"par:",strlen("par:")))
            {
            char *ptr;

            ptr = tmpVal + strlen("par:");

            if ((ptr[0] == '\0') || (MParFind(ptr,&P) == FAILURE))
              {
              MDB(3,fUI) MLog("WARNING:  cannot locate partition '%.100s'\n",
                tmpVal);

              MUISAddData(S,"ERROR:    cannot locate partition\n");

              return(FAILURE);
              }
            }
          else 
            {
            if (!strcmp(tmpVal,"ALL"))
              {
              int rmindex;
              int rmcount = 0;

              for (rmindex = 0;rmindex < MMAX_RM;rmindex++)
                {
                if (MRM[rmindex].Name[0] == '\0')
                  break;
           
                if ((MRM[rmindex].Name[0] == '\1') || 
                    bmisset(&MRM[rmindex].Flags,mrmfTemplate))
                  continue;
          
                RMList[rmcount] = &MRM[rmindex];

                rmcount++;
                }  /* END for (rmindex) */

              /* terminate list */

              RMList[rmcount] = NULL;
              }
            else
              {
              if (MRMFind(tmpVal,&R) == FAILURE)
                {
                MDB(3,fUI) MLog("WARNING:  cannot locate rm '%.100s'\n",
                  tmpVal);
 
                MUISAddData(S,"ERROR:    cannot locate rm\n");

                return(FAILURE);
                }

              RMList[0] = R;
              }

            specifiedRM = TRUE;
            }
          }
        else if (!strcmp(tmpName,"queue"))
          {
          MUStrCpy(SubOType,tmpName,sizeof(SubOType));

          MUStrCpy(SubOID,tmpVal,sizeof(SubOID));
          }
        else if (!strcasecmp(tmpName,"starttime"))
          {
          MUStrCpy(TimeString,tmpVal,sizeof(TimeString));
          }
        else if (!strcasecmp(tmpName,"notify"))
          {
          MUStrCpy(NotifyString,tmpVal,sizeof(NotifyString));
          }
        }    /* END while (MS3GetWhere(RE) == SUCCESS) */

      MXMLGetAttr(RE,MSAN[msanFlags],NULL,FlagString,sizeof(FlagString));
      MXMLGetAttr(RE,MSAN[msanArgs],NULL,ArgString,sizeof(ArgString));
      }  /* END BLOCK */

      break;

    default:

      /* not supported */

      MUISAddData(S,"ERROR:    corrupt command received\n");

      return(FAILURE);

      /*NOTREACHED*/

      break;
    }  /* END switch (S->WireProtocol) */

  bmfromstring(FlagString,MClientMode,&Flags);

  if (bmisset(&Flags,mcmXML))
    {
    DFormat = mfmXML;
    }
  else
    {
    DFormat = mfmNONE;
    }

  /* process data */

  if ((CIndex = (enum MRMCtlCmdEnum)MUGetIndexCI(
         Command,
         MRMCtlCmds,
         FALSE,
         mrmctlNONE)) == mrmctlNONE)
    {
    MDB(3,fUI) MLog("WARNING:  unexpected subcommand '%s' received\n",
      Command);

    sprintf(tmpLine,"ERROR:    unexpected subcommand '%s'\n",
      Command);

    MUISAddData(S,tmpLine);

    return(FAILURE);
    }

  S->SBuffer[0] = '\0';


  /* get cred structure */
  MUserAdd(Auth,&U);

  /* verify rm access */
  if (MUICheckAuthorization(
        U,
        NULL,
        NULL,
        mxoRM,
        mcsRMCtl,
        mgscModify,
        NULL,
        NULL,
        0) == FAILURE)
    {
    MDB(2,fUI) MLog("INFO:     user %s is not authorized to perform operation on rm %s\n",  
      Auth, 
      RMList[0]->Name);

    sprintf(tmpLine,"NOTE:  user %s is not authorized to perform operation on rm %s\n", Auth, RMList[0]->Name);

    MUISAddData(S,tmpLine);

    return(FAILURE);
    }

  /* NOTE:  should loop through all RM's in RMList[] (NYI) */
  /* NOTE:  RMList only supported with mrmctlClear */

  R = RMList[0];

  switch (CIndex)
    {
    case mrmctlClear:

      /* handle 'mrmctl -f' request */

      {
      int rmindex;

      const char *fobject = "statistics";

      MXMLGetAttr(RE,MSAN[msanOp],NULL,tmpVal,sizeof(tmpVal));

      for (rmindex = 0;RMList[rmindex] != NULL;rmindex++)
        {
        R = RMList[rmindex];

        if (!strcasecmp(tmpVal,"messages"))
          {
          /* remove messages/clear failures */

          MMBFree(&R->MB,TRUE);

          fobject = "statistics/messages";
          }
        else if (strcasecmp(tmpVal,"stats"))
          {
          /* ERROR, fobject is neither 'messages' nor 'stats' */
          sprintf(tmpLine,"ERROR: '%s' is neither 'messages' or 'stats'."
              ,tmpVal);

          MUISAddData(S,tmpLine);

          return(FAILURE);
          }
 
        MPSIClearStats(&R->P[0]);

        MPSIClearStats(&R->P[1]);

        sprintf(tmpLine,"%s cleared for RM '%s'\n",
          fobject,R->Name);

        MUISAddData(S,tmpLine);
        }  /* END for (rmindex) */
      }    /* END BLOCK (case mrmctlClear) */
 
      break;

    case mrmctlCreate:

      {
      int STok = -1;

      mbool_t  ArgsLocated = FALSE;

      while (MXMLGetChild(RE,(char *)MSON[msonSet],&STok,&SE) == SUCCESS)
        {
        /* process 'set' request */

        if (MXMLGetAttr(SE,MSAN[msanName],NULL,tmpName,sizeof(tmpName)) == FAILURE)
          {
          continue;
          }

        if (MXMLGetAttr(SE,MSAN[msanValue],NULL,tmpVal,sizeof(tmpVal)) == FAILURE)
          {
          continue;
          }

        if (!strcmp(tmpName,"queue"))
          {
          mclass_t *C;

          ArgsLocated = TRUE;

          if (MClassAdd(tmpVal,&C) == FAILURE)
            {
            sprintf(tmpLine,"class/queue '%s cannot be created via RM %s\n",
              tmpVal,
              R->Name);

            MUISAddData(S,tmpLine);

            return(FAILURE);
            }

          rc = MRMQueueCreate(
            C,
            R,
            C->Name,
            TRUE,
            tmpMsg,
            &SC);

          if (rc == FAILURE)
            {
            MDB(3,fUI) MLog("WARNING:  rm queue create command failed - '%s'\n",
              tmpMsg);

            sprintf(tmpLine,"ERROR:    rm queue create command failed - '%s'\n",
              tmpMsg);

            MUISAddData(S,tmpLine);

            continue;
            }

          sprintf(tmpLine,"queue '%s' created on RM '%s'\n",
            tmpVal,
            R->Name);
 
          MUISAddData(S,tmpLine);
          }    /* END if (!strcmp(tmpName,"queue")) */
        }      /* END while (MXMLGetChild() == FAILURE) */

      if (ArgsLocated == FALSE)
        {
        MDB(3,fUI) MLog("WARNING:  rm queue create command failed - insufficient arguments\n");
  
        sprintf(tmpLine,"WARNING:  rm queue create command failed - insufficient arguments\n");
 
        MUISAddData(S,tmpLine);
        }
      }        /* END BLOCK (case mrmctlCreate) */

      break;

    case mrmctlDestroy:

      MDB(3,fUI) MLog("WARNING:  cannot destroy static RM\n");
                                                                                
      sprintf(tmpLine,"WARNING:  cannot destroy static RM\n");
                                                                                
      MUISAddData(S,tmpLine);

      return(FAILURE);

      /*NOTREACHED*/

      break;

    case mrmctlList:

      {
      int rmindex;
      int amindex;

      mbool_t foundRM = FALSE;

      char *BPtr;
      int   BSpace;

      MUSNInit(&BPtr,&BSpace,tmpLine,sizeof(tmpLine));
 
      for (rmindex = 0;rmindex < MSched.M[mxoRM];rmindex++)
        {
        if (MRM[rmindex].Name[0] == '\0')
          break;

        if (MRMIsReal(R) == FALSE)
          continue;

        if ((specifiedRM == TRUE) &&
          (((R != NULL) && (R != &MRM[rmindex])) || (A != NULL)))
          {
          continue;
          }

        if (DFormat == mfmXML)
          {
          MUSNPrintF(&BPtr,&BSpace,"<rm>%s</rm>",
            MRM[rmindex].Name);
          }
        else
          { 
          /* display human readable text */

          MUSNPrintF(&BPtr,&BSpace,"RM[%s]  type: '%s'  state: '%s'\n",
            MRM[rmindex].Name,
            MRMType[MRM[rmindex].Type],
            MRMState[MRM[rmindex].State]);
          }

        foundRM = TRUE;
        }  /* END for (rmindex) */
 
      for (amindex = 0;MAM[amindex].Type != mamtNONE;amindex++)
        {
        if ((A != NULL) && (A != &MAM[amindex]))
          continue; 

        if (DFormat == mfmXML)
          {
          MUSNPrintF(&BPtr,&BSpace,"<am>%s</am>",
            MAM[amindex].Name);
          }
        else
          {
          /* display human readable text */

          MUSNPrintF(&BPtr,&BSpace,"AM[%s]  type: '%s'  state: '%s'\n",
            MAM[amindex].Name,
            MAMType[MAM[amindex].Type],
            MAMState[MAM[amindex].State]);
          }
 
        foundRM = TRUE;
        }  /* END for (amindex) */
 
      if (foundRM == FALSE)
        {
        if (DFormat == mfmXML)
          {
          /* NO-OP */
          }
        else
          {
          sprintf(tmpLine,"NOTE:     RM or AM '%s' not found\n",
            tmpName);
          }
        }
         
      MUISAddData(S,tmpLine);
      }  /* END BLOCK (case mrmctlList) */

      break;
    
    case mrmctlModify:

      {
      char *TokPtr;
      char *SOType;
      char *ANamePtr;

      int   AIndex;
      int   STok;

      char *vptr;
      char  SubAttr[MMAX_NAME];

      mclass_t *C = NULL;

      if (!strcasecmp(SubOType,"queue") ||
          !strcasecmp(SubOType,"class"))
        {
        if (MClassFind(SubOID,&C) == FAILURE)
          {
          return(FAILURE);
          }
        }

      STok = -1;

      while (MS3GetSet(
          RE,
          &SE,
          &STok,
          tmpName,
          sizeof(tmpName),
          tmpVal,
          sizeof(tmpVal)) == SUCCESS)
        {
        /* FORMAT:  <NAME>[.<ATTR>]=<VAL> */

        if (strchr(tmpName,'.') != NULL)
          {
          if ((SOType = MUStrTok(tmpName,".",&TokPtr)) != NULL)
            {
            ANamePtr = MUStrTok(NULL,"\n",&TokPtr);
            }
          else
            {
            /* should never be reached - malformed value */

            break;
            }
          }
        else
          {
          SOType   = NULL;

          ANamePtr = tmpName;
          }

        if (C != NULL)
          {
          SubAttr[0] = '\0';
          vptr = NULL;

          AIndex = (enum MClassAttrEnum)MUGetIndexCI(
            ANamePtr,
            MClassAttr,
            FALSE,
            0);

          switch (AIndex)
            {
            case mclaReqUserList:

              vptr = tmpVal;

              break;

            default:

              MUStrCpy(SubAttr,ANamePtr,sizeof(SubAttr));

              AIndex = mclaOther;

              break;
            }  /* END switch (AIndex) */

          /* modify attribute */

          rc = MRMQueueModify(
            C,
            R,
            (enum MClassAttrEnum)AIndex,
            SubAttr,
            (vptr != NULL) ? vptr : tmpVal,  /* I - attribute value */
            TRUE,
            tmpMsg,  /* O */
            &SC);    /* O */

          if (rc == FAILURE)
            {
            MDB(3,fUI) MLog("WARNING:  rm queue modify command failed on RM %s - '%s'\n",
              R->Name,
              tmpMsg);

            sprintf(tmpLine,"ERROR:    rm queue modify command failed on RM %s - '%s'\n",
              R->Name,
              tmpMsg);

            MUISAddData(S,tmpLine);

            return(FAILURE);
            }  /* END if (rc == FAILURE) */
          }    /* END if (C != NULL) */
        else if (!strcasecmp(ANamePtr,"state"))
          {
          if (!strcasecmp(tmpVal,"disabled"))
            {
            MRMSetState(R,mrmsDisabled);
            }
          else if (!strcasecmp(tmpVal,"enabled"))
            {
            MRMSetState(R,mrmsConfigured);

            R->RestoreFailureCount = 0;
            }
          }  /* END else if (!strcmp(ANamePtr,"state")) */
        else
          {
          /* object type not supported */

          return(FAILURE);
          }
        }    /* END while (MS3GetSet(RE) == FAILURE) */
 
      sprintf(tmpLine,"RM '%s' successfully modified\n",
        R->Name);

      MUISAddData(S,tmpLine);

      /* check for modify trigger */

      if (R->T != NULL)
        {
        MOReportEvent((void *)R,R->Name,mxoRM,mttModify,MSched.Time,TRUE);
        }
      }      /* END BLOCK (case mrmctlModify) */
 
      break;

    case mrmctlPeerCopy:

      {
      char tmpName[MMAX_LINE];
      char FileName[MMAX_NAME];
      char EMsg[MMAX_LINE] = {0};
      int SC;

      /* only supported for moab peer types */

      if (R->Type != mrmtMoab)
        {
        MDB(3,fUI) MLog("ERROR:    RM '%s' is not a Moab RM--cannot copy file",
          R->Name);

        snprintf(tmpLine,sizeof(tmpLine),"ERROR:   RM '%s' is not a Moab RM--cannot copy file\n",
          R->Name);

        MUISAddData(S,tmpLine);

        return(FAILURE);
        }

      if (MS3GetWhere(
          RE,
          NULL,
          NULL,
          tmpName,
          sizeof(tmpName),
          FileName,
          sizeof(FileName)) == FAILURE)
        {
        return(FAILURE);
        }

      if (strcmp(tmpName,"file"))
        {
        return(FAILURE);
        }

      if (MMPeerCopy(FileName,R,NULL,EMsg,&SC) == FAILURE)
        {
        snprintf(tmpLine,sizeof(tmpLine),"ERROR:    %s\n",
          EMsg);

        MUISAddData(S,tmpLine);

        return(FAILURE);
        }

      snprintf(tmpLine,sizeof(tmpLine),"copy of file '%s' was successful\n",
        FileName);

      MUISAddData(S,tmpLine);

      return(SUCCESS);
      }  /* END (case mrmctlPeerCopy) */

      break;

    case mrmctlPing:

      {
      enum MStatusCodeEnum SC;

      int rc;

      if (A != NULL)
        {
        /* ping accounting manager */

        rc = MAMPing(A,&SC);        
        }
      else
        {
        /* ping resource manager */

        rc = MRMPing(R,&SC);
        }

      if (rc == FAILURE)
        {
        sprintf(tmpLine,"failure - cannot ping RM %s",
          R->Name);
        }
      else
        {
        sprintf(tmpLine,"success - response received from RM %s",
          R->Name);
        }

      MUISAddData(S,tmpLine);
      }  /* END BLOCK (case mrmctlPing) */

      break;

    case mrmctlPurge:

      sprintf(tmpLine,"NOTE:  operation on RM %s not supported\n",R->Name);

      MUISAddData(S,tmpLine);

      return(FAILURE);

      break;

    case mrmctlQuery:

      {
      /* NYI */

      MUISAddData(S,"not enabled");
      }  /* END BLOCK (case mrmctlQuery) */

      break;

    case mrmctlReconfig:

      {
      /* reload the AM or ID RM */

      char *RMName;

      if (I != NULL) 
        RMName = I->Name;
      else if (A != NULL) 
        RMName = A->Name;
      else
        RMName = tmpVal; /* should never get here */

      if ((I != NULL) && (I->State == mrmsActive))
        {
        MIDLoadCred(
          I,
          mxoNONE,
          NULL);

        sprintf(tmpLine,"ID RM %s reloaded",
          I->Name);

        MUISAddData(S,tmpLine);  
        }
      else if ((A != NULL) && (A->Type != mamtNONE) && (A->State == mrmsActive))
        {
        /* Currently nothing needs to be done to reload the AM */

        sprintf(tmpLine,"AM RM %s reloaded",
          A->Name);

        MUISAddData(S,tmpLine);  
        }
      else if ((P != NULL) && (P->ConfigFile != NULL))
        {
        MParLoadConfigFile(P,NULL,NULL);

        sprintf(tmpLine,"partition %s configfile reloaded",P->Name);

        MUISAddData(S,tmpLine);
        }
      else
        {
        sprintf(tmpLine,"Unable to reload RM %s",
          RMName);

        MUISAddData(S,tmpLine);  
        }
      }  /* END BLOCK (case mrmctlReconfig) */

      break;

    default:

      sprintf(tmpLine,"NOTE:  operation on RM %s not supported\n",
        R->Name);

      MUISAddData(S,tmpLine);

      return(FAILURE);

      /*NOTREACHED*/

      break;
    }  /* END switch (CIndex) */

  return(SUCCESS);
  }  /* END MUIRMCtl() */
/* END MUIRMCtl.c */
