/* HEADER */

/**
 * @file MUIVCCtl.c
 *
 * Contains MUI VC Control
 */



#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 * Helper function to lock down certain VC flags (like Workflow, Deleting, etc.)
 *  that should not be set by user or admin.
 *
 * Returns SUCCESS if the given flags can be set, FAILURE otherwise.
 *
 * @param ReqFlags     [I] - The string of flags to set
 * @param InvalidFlags [O] - Output comma-delimited string of flags that can't be set by user
 *                              Must already be init'd
 */

int MUIVCCanSetFlags(
  char *ReqFlags,
  mstring_t *InvalidFlags)

  {
  mbitmap_t tmpL;

  int aindex;
  int rc = SUCCESS;

  /* Place illegal flags here */
  const enum MVCFlagEnum BadFlagList[] = {
    mvcfDeleting,
    mvcfHasStarted,
    mvcfWorkflow,
    mvcfNONE };

  if ((ReqFlags == NULL) ||
      (InvalidFlags == NULL))
    {
    return(FAILURE);
    }

  bmfromstring((char *)ReqFlags,MVCFlags,&tmpL,":,");

  for (aindex = 0;BadFlagList[aindex] != mvcfNONE;aindex++)
    {
    if (bmisset(&tmpL,BadFlagList[aindex]))
      {
      /* Unsettable flag found */

      rc = FAILURE;

      if (!InvalidFlags->empty())
        MStringAppend(InvalidFlags,",");

      MStringAppend(InvalidFlags,MVCFlags[BadFlagList[aindex]]);
      }
    }  /* END for (aindex) */

  bmclear(&tmpL);

  return(rc);
  }  /* END MUIVCCanSetFlags() */

/**
 * Process VC control requests ('mvcctl')
 *
 * @param S - The socket
 * @param AFlags
 * @param Auth
 */

int MUIVCCtl(

  msocket_t *S,
  mbitmap_t *AFlags,
  char      *Auth)

  {
  #define MMAX_VCCATTR 8
  char  SubCommand[MMAX_NAME];
  char  AName[MMAX_VCCATTR + 1][MMAX_LINE];
  char  AVal[MMAX_VCCATTR + 1][MMAX_LINE << 2];
  char  VCExp[MMAX_LINE << 2];
  char  OptString[MMAX_VCCATTR + 1][MMAX_NAME];
  char  OperationString[MMAX_VCCATTR + 1][MMAX_NAME];
  char  ArgString[MMAX_LINE * 6];
  char  tmpName[MMAX_LINE];
  char  tmpVal[MMAX_LINE];
  char  OptArgString[MMAX_LINE];
  char  FlagString[MMAX_LINE];

  enum MFormatModeEnum Format;

  mvc_t  *VC;

  mxml_t *RDE;
  mxml_t *DE;
  mxml_t *SE;
  mxml_t *WE;

  enum MVCCtlCmdEnum SCIndex;
  enum MVCExecuteEnum EIndex = mvceNONE;  /* type of VC action (SCIndex will be set to mvccmExecute */

  int sindex;
  int CTok;
  int WTok;

  mbitmap_t CFlags;
  mbool_t Force = FALSE;  /* For force-deletion (don't wait for sub-objects) */

  mgcred_t *U = NULL;

  char  tmpLine[MMAX_LINE << 2];
  char *BPtr;
  int   BSpace; 

  const char *FName = "MUIVCCtl";

  MDB(2,fUI) MLog("%s(S,%s)\n",
    FName,
    (Auth != NULL) ? Auth : "NULL");

  if (S == NULL)
    {
    return(FAILURE);
    }

  MUSNInit(&BPtr,&BSpace,tmpLine,sizeof(tmpLine));
  MUISClear(S);

  S->WireProtocol = mwpS32;

  if (S->RDE != NULL)
    {
    RDE = S->RDE;
    }
  else
    {
    RDE = NULL;

    if (MXMLFromString(&RDE,S->RPtr,NULL,NULL) == FAILURE)
      {
      MUISAddData(S,"ERROR:  corrupt command received\n");

      MUSNPrintF(&BPtr,&BSpace,"ERROR:  invalid request received by command '%s'\n",
        S->RBuffer);

      MUISAddData(S,tmpLine);

      return(FAILURE);
      }
    }

  MXMLGetAttr(RDE,MSAN[msanAction],NULL,SubCommand,sizeof(SubCommand));

  Format = mfmHuman;

  if (MXMLGetAttr(RDE,MSAN[msanFlags],NULL,FlagString,sizeof(FlagString)) == SUCCESS)
    {
    bmfromstring(FlagString,MClientMode,&CFlags);

    /* if (strstr(FlagString,"XML") != NULL) */
    if (bmisset(&CFlags,mcmXML) == TRUE)
      Format = mfmXML;

    if (bmisset(&CFlags,mcmForce) == TRUE)
      Force = TRUE;
    }

  S->SBuffer[0] = '\0';

  SCIndex = (enum MVCCtlCmdEnum)MUGetIndexCI(
    SubCommand,
    MVCCtlCmds,
    FALSE,
    mvccmNONE);

  if (SCIndex == mvccmNONE)
    {
    /* Not a normal command, check if it is a VC action (execute) */

    EIndex = (enum MVCExecuteEnum)MUGetIndexCI(
      SubCommand,
      MVCExecute,
      FALSE,
      mvceNONE);

    /* If the subcommand was an action, we need to set it to Execute */

    if ((EIndex != mvceNONE) && (EIndex != mvceLAST))
      {
      /* The reason that execute is being set here instead of on the client side
          is because in MUISProcessRequest(), "execute" as an action is caught
          and will make the command go to mccBal, not mccVCCtl.  We send across
          instead the actual VC action (MVCExecute), and set mvccmExecute here */

      SCIndex = mvccmExecute;
      }
    else
      {
      sprintf(tmpLine,"ERROR:  invalid subcommand received (%s)\n",
        SubCommand);

      MUISAddData(S,tmpLine);

      return(FAILURE);
      }
    }  /* END if (SCIndex == mvccmNONE) */

  MUserAdd(Auth,&U);

  if (U == NULL)
    {
    MUSNPrintF(&BPtr,&BSpace,"ERROR:  user not authorized to run this command\n");

    MUISAddData(S,tmpLine);

    return(FAILURE);
    }

  VCExp[0] = '\0';
  WTok = -1;

  while (MS3GetWhere(
          RDE,
          &WE,
          &WTok,
          tmpName,
          sizeof(tmpName),
          tmpVal,
          sizeof(tmpVal)) == SUCCESS)
    {
    if (!strcmp(tmpName,MSAN[msanName]))
      {
      snprintf(VCExp,sizeof(VCExp),"%s",
        tmpVal);

      break;
      }
    }

  if ((SCIndex != mvccmCreate) && (VCExp[0] == '\0'))
    {
    MUISAddData(S,"ERROR:  no VC expression received\n");

    return(FAILURE);
    }

  MXMLGetAttr(RDE,MSAN[msanArgs],NULL,ArgString,sizeof(ArgString));
  MXMLGetAttr(RDE,MSAN[msanOption],NULL,OptArgString,sizeof(OptArgString));

  sindex = 0;

  MXMLGetAttr(RDE,MSAN[msanName],NULL,AName[sindex],sizeof(AName[0]));
  MXMLGetAttr(RDE,MSAN[msanValue],NULL,AVal[sindex],sizeof(AVal[0]));

  if (AName[0][0] != '\0')
    sindex++;

  CTok = -1;

  while (MXMLGetChild(RDE,(char *)MSON[msonSet],&CTok,&SE) == SUCCESS)
    {
    MXMLGetAttr(SE,MSAN[msanName],NULL,AName[sindex],sizeof(AName[0]));
    MXMLGetAttr(SE,MSAN[msanValue],NULL,AVal[sindex],sizeof(AVal[0]));
    MXMLGetAttr(SE,MSAN[msanOption],NULL,OptString[sindex],sizeof(OptString[0]));
    MXMLGetAttr(SE,MSAN[msanOp],NULL,OperationString[sindex],sizeof(OperationString[0]));

    sindex++;

    if (sindex >= MMAX_VCCATTR)
      break;
    }

  AName[sindex][0] = '\0';
  AVal[sindex][0] = '\0';

  if (S->RDE == NULL)
    {
    MXMLDestroyE(&RDE);
    }

  if ((SCIndex != mvccmCreate) &&
      (SCIndex != mvccmQuery) &&
      (MVCFind(VCExp,&VC) == FAILURE))
    {
    snprintf(tmpLine,sizeof(tmpLine),"ERROR:  VC '%s' not found\n",
      VCExp);

    MUISAddData(S,tmpLine);

    return(FAILURE);
    }

  /* Create parent element */

  if (MXMLCreateE(&S->SDE,(char *)MSON[msonData]) == FAILURE)
    {
    MUISAddData(S,"ERROR:    cannot create response\n");

    return(FAILURE);
    }

  DE = S->SDE;

  switch (SCIndex)
    {
    case mvccmCreate:

      {
      enum MVCAttrEnum aindex;

      if (MVCAllocate(&VC,VCExp,NULL) == FAILURE)
        {
        MUISAddData(S,"ERROR:  failed to create VC\n");

        return(FAILURE);
        }

      /* Creation successful set user to owner */

      mstring_t UStr(MMAX_NAME);

      MStringSetF(&UStr,"USER:%s",U->Name);

      MVCSetAttr(VC,mvcaOwner,(void **)UStr.c_str(),mdfString,mSet);

      MVCSetAttr(VC,mvcaCreator,(void **)U->Name,mdfString,mSet);

      WTok = -1;
     
      while (MS3GetWhere(
              RDE,
              &WE,
              &WTok,
              tmpName,
              sizeof(tmpName),
              tmpVal,
              sizeof(tmpVal)) == SUCCESS)
        {
        aindex = (enum MVCAttrEnum)MUGetIndexCI(
                  tmpName,
                  MVCAttr,
                  FALSE,
                  mvcaNONE);

        if (aindex == mvcaName)
          {
          continue;
          }
        else if (aindex == mvcaFlags)
          {
          mstring_t InvalidFlags(MMAX_LINE);

          if (MUIVCCanSetFlags(AVal[sindex],&InvalidFlags) == FAILURE)
            {
            /* Unsettable flag found, don't set and return */

            snprintf(tmpLine,sizeof(tmpLine),"WARNING:  cannot set %s (display flags only)\n",
              InvalidFlags.c_str());

            MUISAddData(S,tmpLine);
            }

          /* All flags good, continue */

          }  /* END if (aindex == mvcaFlags) */

        if ((aindex == mvcaNONE) && (!strcasecmp(tmpName,MXO[mxoxVC])))
          {
          mvc_t *VC2 = NULL;

          if (MVCFind(tmpVal,&VC2) == SUCCESS)
            {
            if (MVCUserHasAccess(VC2,U) == FALSE)
              {
              snprintf(tmpLine,sizeof(tmpLine),"ERROR:  user does not have access to requested VC\n");

              MUISAddData(S,tmpLine);
              }
            else if (MVCAddToVC(VC,VC2,tmpLine) == FAILURE)
              {
              MUISAddData(S,tmpLine);
              }
            }
          }
        else if (MVCSetAttr(VC,aindex,(void **)tmpVal,mdfString,mSet) == FAILURE)
          {
          snprintf(tmpLine,sizeof(tmpLine),"WARNING:  failed to Set attribute '%s'\n",
            AName[sindex]);

          MUISAddData(S,tmpLine);
          }
        }   /* END while (MS3GetWhere()) */

      /* Tell user of the name (and possible label) */

      snprintf(tmpLine,sizeof(tmpLine),"VC '%s' created\n",
        VC->Name);

      MUISAddData(S,tmpLine);

      return(SUCCESS);
      }  /* END case mvccmCreate */

      break;

    case mvccmDestroy:

      {
      if (MVCUserHasAccess(VC,U) == FALSE)
        {
        MUISAddData(S,"ERROR:  user does not have access to requested VC\n");

        return(FAILURE);
        }

      /* If Force == TRUE, WaitForEmpty = FALSE */

      if (MVCRemove(&VC,FALSE,!Force,Auth) == SUCCESS)
        {
        if (VC == NULL)
          {
          snprintf(tmpLine,sizeof(tmpLine),"VC '%s' destroyed\n",
            VCExp);
          }
        else
          {
          /* MVCRemove was successful, but VC is waiting on sub-objects to be destroyed */

          snprintf(tmpLine,sizeof(tmpLine),"VC '%s' pending destruction (waiting for sub-objects to be released)\n",
            VCExp);
          }

        MUISAddData(S,tmpLine);

        return(SUCCESS);
        }
      else
        {
        MUISAddData(S,"ERROR:  failed to destroy VC\n");

        return(FAILURE);
        }
      }

      break;

    case mvccmModify:

      if (MVCUserHasAccess(VC,U) == FALSE)
        {
        MUISAddData(S,"ERROR:  user does not have access to requested VC\n");

        return(FAILURE);
        }

      for (sindex = 0;AName[sindex][0] != '\0';sindex++)
        {
        enum MVCAttrEnum aindex;
        enum MObjectSetModeEnum Op;

        aindex = (enum MVCAttrEnum)MUGetIndexCI(
                  AName[sindex],
                  MVCAttr,
                  FALSE,
                  mvcaNONE);

        if ((aindex != mvcaFlags) &&
            (aindex != mvcaVariables) &&
            (aindex != mvcaDescription) &&
            (aindex != mvcaACL) &&
            (aindex != mvcaOwner) &&
            (aindex != mvcaReqNodeSet) &&
            (aindex != mvcaThrottlePolicies) &&
            (aindex != mvcaReqStartTime))
          {
          snprintf(tmpLine,sizeof(tmpLine),"ERROR:  invalid attribute '%s'\n",
            AName[sindex]);

          MUISAddData(S,tmpLine);

          return(FAILURE);
          }

        if (aindex == mvcaOwner)
          {
          /* Only the owner can change the owner */

          if (MVCCredIsOwner(VC,NULL,U) == FALSE)
            {
            snprintf(tmpLine,sizeof(tmpLine),"ERROR:  only the owner can change ownership on a VC\n");

            MUISAddData(S,tmpLine);

            return(FAILURE);
            }
          }  /* END if (aindex == mvcaOwner) */

        if (aindex == mvcaFlags)
          {
          mstring_t InvalidFlags(MMAX_LINE);

          if (MUIVCCanSetFlags(AVal[sindex],&InvalidFlags) == FAILURE)
            {
            /* Unsettable flag found, don't set and return */

            snprintf(tmpLine,sizeof(tmpLine),"ERROR:  cannot set %s (display flags only)\n",
              InvalidFlags.c_str());

            MUISAddData(S,tmpLine);

            return(FAILURE);
            }

          /* All flags good, continue */

          }  /* END if (aindex == mvcaFlags) */

        Op = (enum MObjectSetModeEnum)MUGetIndexCI(
                OperationString[sindex],
                MObjOpType,
                FALSE,
                mSet);

        if (MVCSetAttr(VC,aindex,(void **)AVal[sindex],mdfString,Op) == FAILURE)
          {
          snprintf(tmpLine,sizeof(tmpLine),"ERROR:  failed to modify attribute '%s'\n",
            AName[sindex]);

          MUISAddData(S,tmpLine);

          return(FAILURE);
          }
        }  /* END for (sindex = 0;AName[sindex][0] != '\0';sindex++) */

      snprintf(tmpLine,sizeof(tmpLine),"VC '%s' successfully modified\n",
        VC->Name);

      MUISAddData(S,tmpLine);

      break;

    case mvccmQuery:

      {
      mstring_t tmpStr(MMAX_LINE);
      mbool_t FullXML = FALSE;

      bmset(&S->Flags,msftReadOnlyCommand);

      if (MUStrStr(FlagString,"fullXML",0,TRUE,FALSE) != NULL)
        FullXML = TRUE;

      if (MVCShowVCs(&tmpStr,VCExp,U,Format,FullXML) == FAILURE)
        {
        snprintf(tmpLine,sizeof(tmpLine),"ERROR:  VC '%s' not found\n",
          VCExp);

        MUISAddData(S,tmpLine);

        return(FAILURE);
        }

      if (tmpStr.empty())
        {
        if (Format == mfmHuman)
          MUISAddData(S,"no VCs found\n");

        return(SUCCESS);
        }

      if (Format == mfmHuman)
        {
        MUISAddData(S,tmpStr.c_str());
        }
      else if (Format == mfmXML)
        {
        char *Tail = NULL;
        char *Tail2 = NULL;
        mxml_t *VCE = NULL;

        MXMLFromString(&VCE,tmpStr.c_str(),&Tail,NULL);

        MXMLAddE(DE,VCE);

        while ((Tail != NULL) && (Tail[0] != '\0'))
          {
          VCE = NULL;

          Tail2 = Tail;
          Tail = NULL;

          MXMLFromString(&VCE,Tail2,&Tail,NULL);

          MXMLAddE(DE,VCE);
          }
        }  /* END else if (Format == mfmXML) */
      }

      break;

    case mvccmAdd:

      if (MVCUserHasAccess(VC,U) == FALSE)
        {
        MUISAddData(S,"ERROR:  user does not have access to requested VC\n");

        return(FAILURE);
        }

      if (bmisset(&VC->Flags,mvcfDeleting))
        {
        MUISAddData(S,"ERROR:  cannot add objects to a VC that is being deleted\n");

        return(FAILURE);
        }

      for (sindex = 0;AName[sindex][0] != '\0';sindex++)
        {
        /* "Name" here is the object type - User, Node, etc. */

        enum MXMLOTypeEnum OType;

        OType = (enum MXMLOTypeEnum)MUGetIndexCI(
                  AName[sindex],
                  MXO,
                  FALSE,
                  mxoNONE);

        if (!MVCIsSupportedOType(OType))
          {
          snprintf(tmpLine,sizeof(tmpLine),"ERROR:  type '%s' is not supported\n",
            AName[sindex]);

          MUISAddData(S,tmpLine);

          return(FAILURE);
          }

        if (MVCHasObject(VC,AVal[sindex],OType))
          {
          snprintf(tmpLine,sizeof(tmpLine),"%s '%s' already in VC '%s'\n",
            MXO[OType],
            AVal[sindex],
            VCExp);

          MUISAddData(S,tmpLine);
          continue;
          }

        if (MVCAddObjectByName(VC,AVal[sindex],OType,U) == FAILURE)
          {
          snprintf(tmpLine,sizeof(tmpLine),"ERROR:  failed to add %s '%s'\n",
            MXO[OType],
            AVal[sindex]);

          MUISAddData(S,tmpLine);

          return(FAILURE);
          }

        snprintf(tmpLine,sizeof(tmpLine),"%s '%s' added to VC '%s'\n",
          MXO[OType],
          AVal[sindex],
          VCExp);

        MUISAddData(S,tmpLine);
        }  /* END for (sindex) */

      /* END BLOCK Add */

      break;

    case mvccmRemove:

      if (MVCUserHasAccess(VC,U) == FALSE)
        {
        MUISAddData(S,"ERROR:  user does not have access to requested VC\n");

        return(FAILURE);
        }

      for (sindex = 0;AName[sindex][0] != '\0';sindex++)
        {
        /* "Name" here is the object type - User, Node, etc. */

        enum MXMLOTypeEnum OType;

        OType = (enum MXMLOTypeEnum)MUGetIndexCI(
                  AName[sindex],
                  MXO,
                  FALSE,
                  mxoNONE);

        if (!MVCIsSupportedOType(OType))
          {
          snprintf(tmpLine,sizeof(tmpLine),"ERROR:  type '%s' is not supported\n",
            AName[sindex]);

          MUISAddData(S,tmpLine);

          return(FAILURE);
          }

        if (!MVCHasObject(VC,AVal[sindex],OType))
          {
          snprintf(tmpLine,sizeof(tmpLine),"ERROR:  %s '%s' was not found in VC '%s'\n",
            MXO[OType],
            AVal[sindex],
            VCExp);

          MUISAddData(S,tmpLine);
          continue;
          }

        if (MVCRemoveObjectByName(VC,AVal[sindex],OType) == FAILURE)
          {
          snprintf(tmpLine,sizeof(tmpLine),"ERROR:  failed to remove %s '%s'\n",
            MXO[OType],
            AVal[sindex]);

          MUISAddData(S,tmpLine);

          return(FAILURE);
          }

        snprintf(tmpLine,sizeof(tmpLine),"%s '%s' removed from VC '%s'\n",
          MXO[OType],
          AVal[sindex],
          VCExp);

        MUISAddData(S,tmpLine);
        }  /* END for (sindex) */

      /* END BLOCK Remove */

      break;

    case mvccmExecute:

      {
      /* EIndex was checked above - EIndex is the action to execute */

      int tmpRC = SUCCESS;
      mstring_t tmpMsg(MMAX_LINE);

      tmpRC = MVCExecuteAction(VC,EIndex,&tmpMsg,Format);

      MUISAddData(S,tmpMsg.c_str());

      return(tmpRC);
      }  /* END BLOCK Execute */

      break;

    default:

      MUISAddData(S,"ERROR:  unknown command\n");

      return(FAILURE);

      /* NOTREACHED */

      break;
    }  /* END switch (SCIndex) */ 

  return(SUCCESS);
  }  /* END MUIVCCtl() */
/* END MUIVCCtl.c */
