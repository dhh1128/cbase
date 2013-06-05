/* HEADER */

/**
 * @file MUINodeCtl.c
 *
 * Contains Job Control function
 */



#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


#define MMAX_NCATTR 8



int __MUINodeModifyState(

  char      *Auth,
  marray_t  *NodeList,
  char      *StateString,
  char      *FlagString,
  mstring_t *Response)

  {
  int nindex;

  mnode_t *N;

  char EMsg[MMAX_LINE];

  enum MNodeStateEnum State;

  if ((NodeList == NULL) || (MUStrIsEmpty(StateString)) || (Response == NULL))
    {
    return(FAILURE);
    }

  State = (enum MNodeStateEnum)MUGetIndexCI(StateString,MNodeState,FALSE,mnsNONE);

  if (State == mnsNONE)
    {
    MStringAppendF(Response,"ERROR:  invalid node state specified (%s)\n",
      StateString);

    return(FAILURE);
    }

  switch (State)
    {
    case mnsDrained: break;
    case mnsIdle: break;
    default:

      MStringAppendF(Response,"ERROR: unsupported state specified (%s) - use 'drained' or 'idle'\n",
        StateString);

      return(FAILURE);
    }

  for (nindex = 0;nindex < NodeList->NumItems;nindex++)
    {
    N = (mnode_t *)MUArrayListGetPtr(NodeList,nindex);

    if (MRMNodeModify(
          N,
          NULL,
          NULL,
          NULL,
          mnaNodeState,
          NULL,
          StateString,
          TRUE,
          mSet,
          EMsg,  /* O */
          NULL) == FAILURE)
      {
      MStringAppendF(Response,"ERROR:  could not modify node state on RM (%s)\n",
        EMsg);

      continue;
      }  /* END if (strcasestr(FlagString,"force")) */

    MNodeSetState(N,State,TRUE);

    MStringAppendF(Response,"state on node '%s' set to '%s'\n",
      N->Name,
      StateString);
    }

  return(SUCCESS);
  }  /* END __MUINodeModifyState() */


/**
 * Process the 'mnodectl -m power=<value>' command
 *
 */

int  __MUINodeCtlModifyPower(
    
  char       *Auth,
  char       *Proxy,
  marray_t   *NodeList,
  const char *AVal,
  mbitmap_t  *CFlags,
  mstring_t  *Response)

  {
  enum MPowerStateEnum RequestedState;

  msysjob_t **SysJobArray = NULL;

  int nindex;

  if ((NodeList == NULL) || (NodeList->NumItems <= 0))
    {
    return(FAILURE);
    }

  /* FORMAT:  power = { on | off | reset } */

  /* If 'reset' requested, then reset the node(s) by rebooting */
  if (!strcasecmp(AVal,"reset"))
    {
    mnode_t *N;
    int nindex;

    for (nindex = 0;nindex < NodeList->NumItems;nindex++)
      {
      N = (mnode_t *)MUArrayListGetPtr(NodeList,nindex);
    
      if (MNodeReboot(N,NULL,NULL,NULL) == SUCCESS)
        {
        MStringAppendF(Response,"node '%s' was reset\n",N->Name);
        }
      else
        {
        MStringAppendF(Response,"node '%s' could not be reset\n",N->Name);
        }
      }

    return(SUCCESS);
    }  /* END if (!strcasecmp(AVal,"reset")) */
  else if (strcasecmp(AVal,"on") && strcasecmp(AVal,"off"))
    {
    /* invalid action state value specified */

    MStringAppendF(Response,"invalid power value specified");

    return(FAILURE);
    }

  SysJobArray = (msysjob_t **)MUCalloc(1,sizeof(msysjob_t *) * MSched.M[mxoNode]);

  /* Now parse actual action verb: The requested state that was specified */
  RequestedState = (MUBoolFromString(AVal,TRUE) == TRUE) ?  mpowOn : mpowOff;

  /* We have a valid 'requested state', so go and perform the poweron | poweroff action */
  if (MNLSetPower(Auth,NodeList,RequestedState,CFlags,Response,SysJobArray,MSched.M[mxoNode]) == FAILURE)
    {
    /* attempt failed, report the error reason and the returned EMsg value */

    MUFree((char **)&SysJobArray);

    return(FAILURE);
    }

  if (!MUStrIsEmpty(Proxy))
    {
    /* Iterate over the list of SysJobArray's ProxyUsers and
     * for each one, set them to the new proxy name
     * (Why does this occur on a POWER cmd?)
     */
    for (nindex = 0; nindex < MSched.M[mxoNode] && SysJobArray[nindex] != NULL; nindex++) 
      {
      MUStrDup(&SysJobArray[nindex]->ProxyUser,Proxy);
      }
    }

  MUFree((char **)&SysJobArray);

  return(SUCCESS);
  } /* END __MUINodeCtlModifyPower() */


/**
 * Handle 'mnodectl -m' requests.
 *
 * @see MUINodeCtl() - parent
 *
 */
 
int __MUINodeCtlModify(

  char      *Auth,                    /* I */
  char      *Proxy,                   /* I (optional) */
  mnode_t   *N,                       /* I (modified) */
  marray_t  *NodeList,                /* I (modified) */
  char       AName[][MMAX_LINE],      /* I */
  char       AVal[][MMAX_LINE << 2],  /* I */
  mbitmap_t *CFlags,                  /* I */
  char      *FlagString,              /* I */
  char       OptString[][MMAX_NAME],  /* I */
  char       OperationString[][MMAX_NAME],  /* I */
  mstring_t *Response)

  {
  int   sindex;
  int   aindex;

  enum MObjectSetModeEnum Mode;

  char  tmpLine[MMAX_LINE];
  char  tmpAttr[MMAX_NAME];

  enum MNodeAttrEnum AIndex;

  char *ptr;

  mgcred_t *AuthU;

  /* AName -> {asw|csw|gevent|gres|gmetric|nodetype|os|partition|power|
               resource|state|trace|variable} */

  /* nodetype and partition should work! */

  /* check authorization of user to execute node control commands */

  if (Response == NULL)
    {
    return(FAILURE);
    }

  MUserAdd(Auth,&AuthU);

  if (MUICheckAuthorization(
        AuthU,
        NULL,
        (void *)N,
        mxoNode,
        mcsMNodeCtl,
        mncmModify,
        NULL,
        tmpLine,
        sizeof(tmpLine)) == FAILURE)
    {
    MStringAppend(Response,tmpLine);

    return(FAILURE);
    }

  for (sindex = 0;sindex < MMAX_NCATTR;sindex++)
    {
    if (AName[sindex][0] == '\0')
      break;

    /* FORMAT:  <ATTR>=<VAL> */

    MUStrCpy(tmpAttr,AName[sindex],sizeof(tmpAttr));

    if ((ptr = strchr(tmpAttr,'=')) != NULL)
      *ptr = '\0';
    
    if ((aindex = MUGetIndexCI(
        tmpAttr,
        MNCMArgs,
        FALSE,
        mncmaNONE)) == mncmaNONE)
      {
      /* handle standard node attributes */

      AIndex = (enum MNodeAttrEnum)MUGetIndexCI(
        AName[sindex],
        MNodeAttr,
        FALSE,
        mnaNONE);

      if (AIndex == mnaNONE)
        {
        /* remap attributes */

        if (!strcasecmp(AName[sindex],"message"))
          AIndex = mnaMessages;
        }

      switch (AIndex)
        {
        case mnaMessages:

          /* FORMAT:  [<LABEL>:]<MESSAGE> */

          {
          char *head;
          char *ptr;
          char *cp;
          char *label = NULL;

          mmb_t *MB;

          head = AVal[sindex];

          if ((cp = strchr(AVal[sindex],':')) != NULL)
            {
            for (ptr = head;ptr < cp;ptr ++)
              {
              if (isspace(*ptr))
                break;
              }

            if (ptr == cp)
              {
              if (!strncasecmp(head,"error",strlen("error")) ||
                  !strncasecmp(head,"alert",strlen("alert")) ||
                  !strncasecmp(head,"info",strlen("info")))
                {
                /* no label specified */

                /* NO-OP */ 
                }
              else
                {
                label = head;
                *cp = '\0';

                head = cp + 1;
                }
              }
            }    /* END if ((cp = strchr(AVal[sindex],':')) != NULL) */

          MMBAdd(
            &N->MB,
            head,
            (Auth != NULL) ? Auth : (char *)"N/A",
            mmbtOther,
            MSched.Time + MCONST_DAYLEN,
            10,
            &MB);

          if ((MB != NULL) && (label != NULL))
            MUStrDup(&MB->Label,label);

          MStringAppendF(Response,"attribute %s set on node %s\n",
            MNodeAttr[AIndex],
            N->Name);

          continue;
          }       /* END BLOCK */

          break;

        case mnaFeatures:
        case mnaGMetric:
        case mnaVariables:
        case mnaPowerSelectState:

          if (MNodeSetAttr(N,AIndex,(void **)AVal[sindex],mdfString,mSet) == SUCCESS)
            {
            MStringAppendF(Response,"attribute %s set on node %s\n",
              MNodeAttr[AIndex],
              N->Name);
            }
          else
            {
            MStringAppendF(Response,"ERROR:  unable to set attribute %s to '%s' on node %s\n",
              MNodeAttr[AIndex],
              AVal[sindex],
              N->Name);
            }

          continue;

          /*NOTREACHED*/

          break;

        default:

          MStringAppendF(Response,"ERROR:  invalid argument received (%s)\n",
            AName[sindex]);

          return(FAILURE);

          /*NOTREACHED*/

          break;
        }  /* END switch (AIndex) */
      }    /* END if ((AIndex = MUGetIndexCI()) == mncmaNONE) */

    /* handle command-specific node attributes */

    Mode = (enum MObjectSetModeEnum)MUGetIndexCI(
      OperationString[sindex],
      MObjOpType,
      FALSE,
      mSet);

    switch (aindex)
      {
      case mncmaGEvent:

        {
        char *ptr;
        char *TokPtr;

        char tmpLine[MMAX_LINE];
        char *tmpMsg;
        mgevent_obj_t  *GEvent;
        mgevent_desc_t *GEDesc;

        /* generic event */
   
        MUStrCpy(tmpLine,AVal[sindex],sizeof(tmpLine));

        ptr = MUStrTok(tmpLine,":",&TokPtr);

        /* FORMAT */
        /* AName[sindex] = gevent */
        /* AVal[sindex] = hitemp:[severity:]'xxx' */

        if (ptr == NULL)
          {
          break;
          }

        while (ptr[0] == '=')
          {
          ptr++;
          }

        if (!strcmp(OperationString[sindex],MObjOpType[mDecr]))
          {
          /* Remove the given gevent */

          if (MGEventGetItem(ptr,&N->GEventsList,&GEvent) == SUCCESS)
            {
            MGEventRemoveItem(&N->GEventsList,ptr);
            MUFree((char **)&GEvent->GEventMsg);
            MUFree((char **)&GEvent->Name);
            MUFree((char **)&GEvent);
            }

          break;
          }

        /* Add the GEvent description and Item */

        MGEventAddDesc(ptr);
        MGEventGetOrAddItem(ptr,&N->GEventsList,&GEvent);
        MGEventGetDescInfo(ptr,&GEDesc);

        if (strchr(TokPtr,':') != NULL)
          {
          ptr = MUStrTok(NULL,":",&TokPtr);

          if (ptr != NULL)
            {
            GEvent->Severity = (int)strtol(ptr,NULL,10);

            if ((GEvent->Severity > MMAX_GEVENTSEVERITY) || (GEvent->Severity < 0))
              GEvent->Severity = 0;
            }
          }
        else
          {
          GEvent->Severity = 0;
          }

        tmpMsg = MUStrTok(NULL,"\n",&TokPtr);

        MUStrDup(&GEvent->GEventMsg,tmpMsg);

        if (GEvent->GEventMTime + GEDesc->GEventReArm <= MSched.Time)
          {
          /* process event as new */

          GEvent->GEventMTime = MSched.Time;
   
          MGEventProcessGEvent(
            -1,
            mxoNode,
            N->Name,
            ptr,
            0.0,
            mrelGEvent,
            tmpMsg);
          }

        MStringAppendF(Response,"set attribute %s on node %s\n",
          MNodeAttr[mnaGEvent],
          N->Name);
        }  /* END BLOCK (case mwnaGEvent) */
   
        break;

      case mncmaOS:

        {
        char EMsg[MMAX_LINE] = {0};

        marray_t JArray;

        int  OSIndex = MUMAGetIndex(meOpsys,AVal[sindex],mVerify);                           

        if (OSIndex <= 0)
          {
          MStringAppendF(Response,"ERROR:  unknown OS specified '%s'\n",
            AVal[sindex]);

          break;
          }

        if (N->VMList != NULL)
          {
          MStringAppendF(Response,"ERROR:  Can not reprovision node with existing VM's.  VM's may be migrated using 'mvmctl -M'.\n");
          break;
          }

        if (bmisset(CFlags,mcmForce))
          {
          if (MNodeProvision(N,NULL,NULL,AVal[sindex],MBNOTSET,EMsg,NULL) == FAILURE)
            {
            MStringAppendF(Response,"ERROR:  cannot change os on node %s to '%s' - %s\n",
              N->Name,
              AVal[sindex],
              EMsg);

            break;
            }
          else
            {
            MStringAppendF(Response,"node %s provisioning to OS '%s'\n",
              N->Name,
              MAList[meOpsys][OSIndex]);
            }
          }
        else
          {
          int nindex;

          mnl_t filteredNL;
          mnl_t NL;

          char VMString[MMAX_LINE];

          MNLInit(&NL);

          for (nindex = 0;nindex < NodeList->NumItems;nindex++)
            {
            MNLAddNode(&NL,(mnode_t *)MUArrayListGetPtr(NodeList,nindex),1,FALSE,NULL);
            }

          if (!strncasecmp(FlagString,"vm:",strlen("vm:")))
            {
            MUStrCpy(VMString,&FlagString[strlen("vm:")],sizeof(VMString));
            }
          else
            {
            VMString[0] = '\0';
            }

          MUArrayListCreate(&JArray,sizeof(mjob_t *),1);

          MNLInit(&filteredNL);

      	  /* Check if we need to provision nodes based on srcOS and destOS */
      	  /* unless MSched.ForceNodeReprovision is true */

      	  if (MSched.ForceNodeReprovision == FALSE)
            {
            mnode_t *tmpN;

            for (nindex = 0;MNLGetNodeAtIndex(&NL,nindex,&tmpN) == SUCCESS;nindex++)
              {
              if (tmpN->ActiveOS != OSIndex)
                {
                MNLAddNode(&filteredNL,tmpN,1,TRUE,NULL);
                }
              }

            if (MNLIsEmpty(&filteredNL))
              {
              MNLFree(&filteredNL);
              MNLFree(&NL);

              MStringAppendF(Response,"ERROR:  the host(s) are already running the requested OS\n");

              MUArrayListFree(&JArray);

              return(FAILURE);
              }
            }
          else
            {
            MNLCopy(&filteredNL, &NL);
            }

          /* END of check */

          if (MUGenerateSystemJobs(
                (AuthU != NULL) ? AuthU->Name : NULL,
                NULL,
                &filteredNL,
                msjtOSProvision,
                (VMString[0] == '\0') ? "provision" : "vmprovision",
                OSIndex,
                VMString,
                NULL,
                FALSE,
                FALSE,
                NULL,
                &JArray) == SUCCESS)
            {
            int nindex;

            mjob_t *J;

            for (nindex = 0;nindex < JArray.NumItems;nindex++)
              {
              J = *(mjob_t **)MUArrayListGet(&JArray,nindex);

              MStringAppendF(Response,"provisioning job '%s' submitted\n\n",
                J->Name);

              if ((J->System != NULL) && (!MUStrIsEmpty(Proxy)))
                {
                MUStrDup(&J->System->ProxyUser,Proxy);
                }
              }  /* END for (nindex) */
            }    /* END if (MUGenerateSystemJobs) */

          MNLFree(&filteredNL);
          MNLFree(&NL);

          MUArrayListFree(&JArray);
          }
        }    /* END BLOCK (case mncmaOS) */

        break;

      case mncmaPartition:

        if (MNodeSetAttr(N,mnaPartition,(void **)AVal[sindex],mdfString,mSet) == FAILURE)
          {
          MStringAppendF(Response,"ERROR:  cannot set attribute %s on node %s to '%s'\n",
            MNodeAttr[mnaPartition],
            N->Name,
            AVal[sindex]);
          }
        else
          {
          MStringAppendF(Response,"set attribute %s on node %s to '%s'\n",
            MNodeAttr[mnaPartition],
            N->Name,
            AVal[sindex]);
          }

        break;

      case mncmaPower:

        /* Call the helper function to perform the Power action (AVal[sindex]) processing */
        if (__MUINodeCtlModifyPower(Auth,Proxy,NodeList,AVal[sindex],CFlags,Response) == FAILURE)
          {
          return(FAILURE);
          }

        break;

      case mncmaState:

        if (__MUINodeModifyState(Auth,NodeList,AVal[sindex],FlagString,Response) == FAILURE)
          {
          return(FAILURE);
          }

        break;

      case mncmaVariables:
        {
        int       slen = strlen(AVal[sindex]);

        const char *Word = NULL;

        if (slen == 0)
          break;

        mstring_t tmpString(AVal[sindex]);;

        if (Mode == mAdd)
          Word = "added";
        else if ((Mode == mDecr) || (Mode == mUnset))
          Word = "removed";
        else
          Word = "set";

        /* MNodeSetAttr tokenizes AVal[sindex] so we save off a copy beforehand for MUSNPrintf() calls afterwards */

        MStringAppend(&tmpString,AVal[sindex]);

        if (MNodeSetAttr(N,mnaVariables,(void **)AVal[sindex],mdfString,Mode) == SUCCESS)
          {
          /* variable <var_name> <set,decr,unset> on node <node_name> */

          MStringAppendF(Response,"variable '%s' %s on node %s\n",
            tmpString.c_str(),
            Word,
            N->Name);
          }
        else
          {
          MStringAppendF(Response,"FAILURE: variable '%s' NOT %s on node %s\n",
            tmpString.c_str(),
            Word,
            N->Name);
          }
        }

        break;

      default:

        MStringAppendF(Response,"ALERT:  invalid argument received (%s)\n",
          AName[sindex]);

        break;
      }  /* END switch (aindex) */
    }    /* END for (sindex) */

  /* node modify was successful */

  if (MSched.AdminEventAggregationTime >= 0)
    {
    MUIDeadLine = MIN(
      MUIDeadLine,
      (long)MSched.Time + MSched.AdminEventAggregationTime);
    }

  /* check for modify trigger */

  if (N->T != NULL)
    {
    MOReportEvent((void *)N,N->Name,mxoNode,mttModify,MSched.Time,TRUE);
    }

  return(SUCCESS);
  }      /* END __MUINodeCtlModify() */
 

#define MAX_VM_PER_COMMAND 100

/**
 * Process node control ('mnodectl') requests.
 *
 * @see __MUINodeCtlModify() - child - modify nodes
 * @see MNodeRemove() - child - remove/destroy nodes
 * @see MUINodeDiagnose() - child - 'mnodectl -q'
 * @see MNodeProfileToXML() - child - 'mnodectl -q profile'
 * @see MUIVMCreate() - child - 'mnodectl -c vm:<X>'
 *
 * @param S (I/O)
 * @param AFlags (I) [bitmap of MRoleEnum]
 * @param Auth (I)
 */

int MUINodeCtl(

  msocket_t *S,      /* I/O */
  mbitmap_t *AFlags, /* I (bitmap of MRoleEnum) */
  char      *Auth)   /* I */

  {
  char     SubCommand[MMAX_NAME];
  char     AName[MMAX_NCATTR + 1][MMAX_LINE];
  char     AVal[MMAX_NCATTR + 1][MMAX_LINE << 2];
  char     NodeExp[MMAX_LINE << 2];
  char     OptString[MMAX_NCATTR + 1][MMAX_NAME];
  char     OperationString[MMAX_NCATTR + 1][MMAX_NAME];
  char     ArgString[MMAX_LINE];
  char     tmpName[MMAX_LINE];
  char     tmpVal[MMAX_LINE];
  char     OptArgString[MMAX_LINE];

  enum MNodeCtlCmdEnum SCIndex;

  int      sindex;
  int      NIndex;

  mnode_t *N;

  marray_t NodeList;

  mvm_t   *V[MAX_VM_PER_COMMAND];

  char     tmpLine[MMAX_LINE << 2];
  char    *BPtr;
  int      BSpace;

  char     FlagString[MMAX_LINE];

  mbool_t  UseRegex = FALSE;

  mxml_t  *RDE;
  mxml_t  *SE;
  mxml_t  *WE;

  int      CTok;
  int      WTok;

  mbitmap_t CFlags;

  const char *FName = "MUINodeCtl";

  MDB(2,fUI) MLog("%s(S,%ld,%s)\n",
    FName,
    AFlags,
    (Auth != NULL) ? Auth : "NULL");

  if (S == NULL)
    {
    return(FAILURE);
    }

  /* FORMAT:  <CMD> <SUBCOMMAND> <ARG> <NODEEXP>       
   *                ^                                 
   *     ie:  mnodectl modify state=down node001     
   *          mnodectl create trace="X" alpha321    
   *          mnodectl modify resource=dog.set:4       
   *          mnodectl modify resource=dog.inc:1       
   *          mnodectl modify resource=cat.dec:2      
   *          mnodectl query diag node001            
   *          mnodectl list                         
   *  <NODEEXP> = <NODEID>[,<NODEID>]... or
   *              <NODE_REGEX> or
   *              <NODEID>:<VMID>
   */

  MUSNInit(&BPtr,&BSpace,tmpLine,sizeof(tmpLine));

  MUISClear(S);

  memset(V,0,sizeof(V));

  /* TEMP:  force to XML */

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

  if (MXMLGetAttr(RDE,MSAN[msanFlags],NULL,FlagString,sizeof(FlagString)) == SUCCESS)
    {
    if (strstr(FlagString,"regex") != NULL)
      UseRegex = TRUE;

    bmfromstring(FlagString,MClientMode,&CFlags);
    }

  /* look-up the subcommand */

  S->SBuffer[0] = '\0';

  SCIndex = (enum MNodeCtlCmdEnum)MUGetIndexCI(
    SubCommand,
    MNodeCtlCmds,
    FALSE,
    mncmNONE);

  if (SCIndex == mncmNONE)
    {
    sprintf(tmpLine,"ERROR:  invalid subcommand received (%s)\n",
      SubCommand);

    MUISAddData(S,tmpLine);

    return(FAILURE);
    }

  NodeExp[0] = '\0';

  WTok = -1;

  if (UseRegex != TRUE)
    {
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
        char *ptr;

        ptr = strchr(tmpVal,':');

        if (ptr != NULL)
          {
          char tmpBuffer[MMAX_BUFFER];

          char *TokPtr;
          char *NPtr;

          char *BPtr;
          int   BSpace;

          int   vindex;

          /* FORMAT:  <NODEID>:<VMID>[,<NODEID>:<VMID>]... expression specified */

          vindex = 0;


          MUSNInit(&BPtr,&BSpace,tmpBuffer,sizeof(tmpBuffer));

          NPtr = MUStrTok(tmpVal,",",&TokPtr);

          while (NPtr != NULL)
            {
            ptr = strchr(NPtr,':');

            if (ptr != NULL)
              {
              *ptr = '\0';

              if (MVMFind(ptr + 1,&V[vindex]) == FAILURE)
                {
                snprintf(tmpLine,sizeof(tmpLine),"ERROR:  cannot locate VM '%s'\n",
                  ptr + 1);

                MUISAddData(S,tmpLine);

                return(FAILURE);
                }
              }
            else
              {
              V[vindex] = NULL;
              }

            MUSNPrintF(&BPtr,&BSpace,"%s%s",
              (vindex > 0) ? "," : "",
              NPtr);

            vindex++;

            NPtr = MUStrTok(NULL,",",&TokPtr);
            }  /* END while (NPtr != NULL) */

          MUStrCpy(NodeExp,tmpBuffer,sizeof(NodeExp));
          }
        else if (!strcmp(tmpVal,"ALL"))
          {
          strcpy(NodeExp,"ALL");
          }
        else if (SCIndex == mncmQuery)
          {
          MUStrCpy(NodeExp,tmpVal,sizeof(NodeExp));
          }
        else
          {
          snprintf(NodeExp,sizeof(NodeExp),"^%s$",
            tmpVal);
          }

        break;
        }
      }    /* END while (MS3GetWhere() == SUCCESS) */
    }      /* END if (UseRegex != TRUE) */
  else
    {
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
        snprintf(NodeExp,sizeof(NodeExp),"%s",
          tmpVal);
        }
      else if (!strcmp(tmpName,"jobspec"))
        {
        /* NYI */
        }
      }
    }   /* END else (UseRegex != TRUE) */

  if (NodeExp[0] == '\0')
    {
    if (SCIndex != mncmQuery)
      {
      MUISAddData(S,"ERROR:  no node expression received\n");

      return(FAILURE);
      }
    }

  MXMLGetAttr(RDE,MSAN[msanArgs],NULL,ArgString,sizeof(ArgString));
  MXMLGetAttr(RDE,MSAN[msanOption],NULL,OptArgString,sizeof(OptArgString));

  /* NOTE:  old format allows AName/AVal from root element */

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

    if (sindex >= MMAX_NCATTR)
      break;
    }  /* END while (MXMLGetChild() == SUCCESS) */

  /* terminate lists */

  AName[sindex][0] = '\0';
  AVal[sindex][0]  = '\0';

  if (S->RDE == NULL)
    {
    MXMLDestroyE(&RDE);
    }

  MUArrayListCreate(&NodeList,sizeof(mnode_t *),-1);

  switch (SCIndex)
    {
    case mncmQuery:

      if (strstr(ArgString,"diag"))
        {
        MUINodeDiagnose(
          Auth,
          NULL,
          mxoNONE,
          NULL,
          &S->SDE,  /* O */
          NULL,
          NULL,
          NULL,
          NodeExp,
          &CFlags);

        return(SUCCESS);
        }
   
      /* fall-through */

    default:

      /* determine node expression list */

      {
      char *ptr;

      char tmpMsg[MMAX_LINE];

      tmpMsg[0] = '\0';

      /* FORMAT:  <NODEID>[,<NODEID>]... or
                  <NODEID>:<VMID> or
                  <NODEEXP>
      */

      if (NodeExp[0] == '\0')
        {
        snprintf(tmpLine,sizeof(tmpLine),"ERROR:  no node expression specified\n");

        MUISAddData(S,tmpLine);

        return(FAILURE);
        }

      if (V[0] != NULL)
        {
        char *Ptr;
        char *TokPtr;

        mnode_t *N;

        Ptr = MUStrTok(NodeExp,",",&TokPtr);

        while (Ptr != NULL)
          {
          if (MNodeFind(Ptr,&N) == FAILURE)
            {          
            snprintf(tmpLine,sizeof(tmpLine),"ERROR:  invalid node '%s' specified\n",
              Ptr);

            MUISAddData(S,tmpLine);

            return(FAILURE);
            }

          MUArrayListAppendPtr(&NodeList,N);

          Ptr = MUStrTok(NULL,",",&TokPtr);
          }  /* END while (Ptr != NULL) */
        }
      else if ((ptr = strchr(NodeExp,':')) != NULL) 
        {
        if (MNodeFind(NodeExp,&N) == FAILURE)
          {
          snprintf(tmpLine,sizeof(tmpLine),"ERROR:  cannot locate node '%s'\n",
            NodeExp);

          MUISAddData(S,tmpLine);

          return(FAILURE);
          }

        if (V[0] == NULL)
          {
          *ptr = '\0';

          if (MVMFind(ptr + 1,&V[0]) == FAILURE)
            {
            snprintf(tmpLine,sizeof(tmpLine),"ERROR:  cannot locate VM '%s'\n",
              ptr + 1);

            MUISAddData(S,tmpLine);

            return(FAILURE);
            }
          }

        MUArrayListAppendPtr(&NodeList,N);
        }    /* END if (ptr != NULL) */
      else if ((MUREToList(
            NodeExp,
            mxoNode,
            NULL,
            &NodeList,
            FALSE,
            tmpMsg,
            sizeof(tmpMsg)) == FAILURE) || (NodeList.NumItems == 0))
        {
        snprintf(tmpLine,sizeof(tmpLine),"ERROR:  invalid node expression received '%s' : %s\n",
          NodeExp,
          tmpMsg);

        MUISAddData(S,tmpLine);

        return(FAILURE);
        }
      }    /* END BLOCK (case default) */

      break;
    }  /* END switch (SCIndex) */

  sindex = 0;

  for (NIndex = 0;NIndex < NodeList.NumItems;NIndex++)
    {
    N = (mnode_t *)MUArrayListGetPtr(&NodeList,NIndex);

    MUSNInit(&BPtr,&BSpace,tmpLine,sizeof(tmpLine));

    switch (SCIndex)
      {
      case mncmDestroy:

        if (!strcasecmp(AName[0],"message"))
          {
          int Index;

          /* NOTE:  are we certain message label/index is in set index '0'? */

          MMBGetIndex(N->MB,AVal[0],NULL,mmbtNONE,&Index,NULL);

          if (MMBRemove(Index,&N->MB) == FAILURE)
            {
            MUSNPrintF(&BPtr,&BSpace,"ERROR:  cannot remove message %d from node %s\n",
              Index,
              N->Name);
            }
          else
            {
            MUSNPrintF(&BPtr,&BSpace,"successfully removed message %d from node %s\n",
              Index,
              N->Name);
            }
          }    /* END if (!strcasecmp(AName[0],"message")) */

        break;

      case mncmList:

        /* list all nodes */

        {
        mxml_t *RE = NULL;
        mxml_t *NE = NULL;

        char tmpBuf[MMAX_BUFFER];

        if (MXMLCreateE(&RE,(char *)MSON[msonData]) == FAILURE)
          {
          MUISAddData(S,"ERROR:  could not create xml structure\n");

          return(FAILURE);
          }

        NE = NULL;

        MXMLCreateE(&NE,(char *)MXO[mxoNode]);

        MXMLSetAttr(
          NE,
          (char *)MNodeAttr[mnaNodeID],
          (void *)N->Name,
          mdfString);

        MXMLAddE(RE,NE);

        MXMLToString(RE,tmpBuf,sizeof(tmpBuf),NULL,TRUE);

        MUISAddData(S,tmpBuf);

        MXMLDestroyE(&RE);
        }    /* END BLOCK (case mncmList) */

        break;

      case mncmMigrate:

        /* migrating VM from one physical node to another */

        {
        char GEventID[MMAX_NAME];

        mjob_t *J;
        mjob_t *PowerJ = NULL;

        char *Arg;
        char *TokPtr;

        char *BPtr;
        int   BSpace;
        char *ptr;
        char  tmpMsg[MMAX_LINE];
        char  EMsg[MMAX_LINE] = {0};

        mnode_t *DstN = NULL;

        if (V[NIndex] == NULL)
          {
          MUSNPrintF(&BPtr,&BSpace,"ERROR:    argument 'rm' or 'vm' is required\n");

          MUISAddData(S,tmpLine);

          return(FAILURE);
          }

        /* ARG FORMAT: vm=<vmid>,dsthost=<nodeid> */
        
        MUSNInit(&BPtr,&BSpace,tmpLine,sizeof(tmpLine));

        Arg = MUStrTok(ArgString,",",&TokPtr);

        if (V[NIndex] != NULL)
          {
          char tmpEMsg[MMAX_LINE] = {0};

          /* check for required 'dsthost' parameter */

          if (V[NIndex]->N == NULL)
            {
            MUSNPrintF(&BPtr,&BSpace,"ERROR:    VM %s has no parent. It may not yet be created.\n",
              V[NIndex]->VMID);

            MUISAddData(S,tmpLine);

            return(FAILURE);
            }

          if ((Arg == NULL) || (Arg[0] == '\0'))
            {
            MUSNPrintF(&BPtr,&BSpace,"ERROR:    argument 'dsthost is required\n");

            MUISAddData(S,tmpLine);

            return(FAILURE);
            }

          if ((ptr = strstr(Arg,"dsthost=")) == NULL)
            {
            MUSNPrintF(&BPtr,&BSpace,"ERROR:    argument 'dsthost' is required\n");

            MUISAddData(S,tmpLine);

            return(FAILURE);
            }

          ptr += strlen("dsthost=");

          if (!strncasecmp(ptr,"any",strlen("any")))
            {
            DstN = NULL;
            }
          else if (MNodeFind(ptr,&DstN) == FAILURE)
            {
            MUSNPrintF(&BPtr,&BSpace,"ERROR:    specified destination host is invalid\n");

            MUISAddData(S,tmpLine);

            return(FAILURE);
            }
          else if (MVMNodeIsValidDestination(V[NIndex],DstN,TRUE,tmpEMsg) == FALSE)
            {
            MUSNPrintF(&BPtr,&BSpace,"ERROR:    %s\n",
              tmpEMsg);

            MUISAddData(S,tmpLine);

            return(FAILURE);
            }
          }  /* END if ((ptr = strstr(Arg,"vm=")) != NULL) */

        /* perform migration of VM to new node/hypervisor */

        MUStrCpy(GEventID,"user_migrate",sizeof(GEventID));

        /* NOTE:  see __MUIJobMigrate() - allows job-centric VM migration */

        if (DstN == NULL)
          {
          mstring_t EMsgMString(MMAX_LINE);

          if ((VMFindDestinationSingleRun(V[NIndex],&EMsgMString,&DstN) == FAILURE) ||
              (DstN == NULL))
            {
            char tmpLine[MMAX_LINE];

            MDB(2,fALL) MLog("WARNING:  vm %s should migrate from node %s but cannot locate valid destination - %s (policy)\n",
              V[NIndex]->VMID,
              N->Name,
              EMsgMString.c_str());

            snprintf(tmpLine,sizeof(tmpLine),"ERROR:  cannot select destination host to migrate VM %s - %s\n",
              V[NIndex]->VMID,
              EMsgMString.c_str());

            MUISAddData(S,tmpLine);

            return(FAILURE);
            }  /* END if (VMFindDestinationSingleRun(V[NIndex],&DstN) == FAILURE) */
          }  /* END if (DstN == NULL) */

        /* migrate VM to specified node */

        if (DstN->PowerSelectState == mpowOff)
          {
          MUSNPrintF(&BPtr,&BSpace,"ERROR:  cannot power-on destination host %s to allow migration of vm %s\n",
            DstN->Name,
            V[NIndex]->VMID);

          MUISAddData(S,tmpLine);

          return(FAILURE);
          }  /* END if (DstN->PowerSelectState == mpowOff) */

        /* use V and DstN */

        snprintf(tmpMsg,sizeof(tmpMsg),"migration from %s to %s requested by admin %s",
          N->Name,
          DstN->Name,
          (Auth != NULL) ? Auth : "");

        MVMAddGEvent(V[NIndex],GEventID,0,tmpMsg);

        if (MUSubmitVMMigrationJob(
            V[NIndex],
            DstN,
            (Auth != NULL) ? Auth : NULL,
            &J,
            tmpMsg,
            EMsg) == FAILURE)
          {
          MUSNPrintF(&BPtr,&BSpace,"ERROR:  cannot migrate VM %s to host %s - %s\n",
            V[NIndex]->VMID,
            DstN->Name,
            EMsg);

          MUISAddData(S,tmpLine);

          return(FAILURE);
          }

        if (PowerJ != NULL)
          {
          /* migration cannot execute until power on is complete */

          MJobSetAttr(J,mjaDepend,(void **)PowerJ->Name,mdfString,mSet);
          }

        /* NOTE:  update system job once RM has reported migration is complete */

        MUSNPrintF(&BPtr,&BSpace,"VM migrate job '%s' submitted\n",
          J->Name);

        MUISAddData(S,tmpLine);

        return(SUCCESS);
        }    /* END BLOCK (case mncmMigrate) */

        break;

      case mncmModify:

        if (V[NIndex] != NULL)
          {
          enum MNodeAttrEnum AIndex;

          /* NOTE:  modifying VM */

          /* NOTE:  at some point, consider merging code below into 
                    __MUINodeCtlModify() - may or may not make sense */

          for (sindex = 0;AName[sindex][0] != '\0';sindex++)
            {
            /* enable 'aliases' */

            AIndex = (enum MNodeAttrEnum)MUGetIndexCI(
              AName[sindex],
              MNodeAttr,
              FALSE,
              mnaNONE);

            if (AIndex == mnaNONE)
              {
              /* enable 'aliases' */

              if (!strcasecmp(AName[sindex],"state"))
                AIndex = mnaNodeState;
              }

            switch (AIndex)
              {
              case mnaNONE:

                MUSNPrintF(&BPtr,&BSpace,"ERROR:  cannot modify attribute '%s' on VM %s:%s to '%s' - unknown attribute\n",
                  AName[sindex],
                  N->Name,
                  V[NIndex]->VMID,
                  AVal[sindex]);

                MUISAddData(S,tmpLine);

                break;

              case mnaOS:

                {
                /* NOTE:  modify active OS within VM */

                /* NYI */

                char EMsg[MMAX_LINE] = {0};

                if (MNodeProvision(N,V[NIndex],NULL,AVal[sindex],MBNOTSET,EMsg,NULL) == FAILURE)
                  {
                  MUSNPrintF(&BPtr,&BSpace,"ERROR:  cannot change os on VM %s:%s to '%s' - %s\n",
                    N->Name,
                    V[NIndex]->VMID,
                    AVal[sindex],
                    EMsg);

                  MUISAddData(S,tmpLine);

                  break;
                  }

                MUSNPrintF(&BPtr,&BSpace,"set attribute %s on VM %s:%s to '%s' - %s\n",
                  MNodeAttr[mnaOS],
                  N->Name,
                  V[NIndex]->VMID,
                  AVal[sindex],
                  EMsg);

                MUISAddData(S,tmpLine);
                }  /* END BLOCK (case mnaOS) */

                break;

              case mnaNodeState:

                if (!strcasecmp(AVal[sindex],"start"))
                  {
                  char EMsg[MMAX_LINE] = {0};

                  /* start VM */

                  if (MRMNodeSetPowerState(N,NULL,V[NIndex],mpowOn,EMsg) == FAILURE)
                    {
                    MUSNPrintF(&BPtr,&BSpace,"ERROR:  cannot power on VM %s:%s - %s\n",
                      N->Name,
                      V[NIndex]->VMID,
                      EMsg);

                    MUISAddData(S,tmpLine);

                    return(FAILURE);
                    }
                  }
                else if (!strcasecmp(AVal[sindex],"stop"))
                  {
                  char EMsg[MMAX_LINE] = {0};

                  /* stop VM */

                  if (MRMNodeSetPowerState(N,NULL,V[NIndex],mpowOff,EMsg) == FAILURE)
                    {
                    MUSNPrintF(&BPtr,&BSpace,"ERROR:  cannot power off VM %s:%s - %s\n",
                      N->Name,
                      V[NIndex]->VMID,
                      EMsg);

                    MUISAddData(S,tmpLine);

                    return(FAILURE);
                    }
                  }

                MUISAddData(S,"successfully updated VM power state");

                break;

              default:

                /* modification of this attribute not support for VMs */

                MUSNPrintF(&BPtr,&BSpace,"ERROR:  cannot modify attribute %s for VMs - operation not supported\n",
                  MNodeAttr[AIndex]);

                MUISAddData(S,tmpLine);

                return(FAILURE);

                /*NOTREACHED*/

                break;
              }  /* END switch (AIndex) */
            }    /* END for (sindex) */

          return(SUCCESS);
          }  /* END if (V[NIndex] != NULL) */
        else
          {
          mstring_t Response(MMAX_LINE);
 
          char Proxy[MMAX_NAME];

          MXMLGetAttr(S->RDE,"proxy",NULL,Proxy,sizeof(Proxy));

          if (__MUINodeCtlModify(
                Auth,
                Proxy,
                N,
                &NodeList,
                AName,
                AVal,
                &CFlags,
                FlagString,
                OptString,
                OperationString,
                &Response) == FAILURE)
            {
            MUISAddData(S,Response.c_str());

            /* NOTE:  should we continue or abort on failure? */

            MUArrayListFree(&NodeList);
            return(FAILURE);
            }

          MUISAddData(S,Response.c_str());
          }

        break;

      case mncmQuery:

        if (strstr(ArgString,"profile"))
          {
          /* show node profile statistics */

          long STime;
          long ETime;
   
          mxml_t *DE;

          if ((S->SDE == NULL) && 
              (MXMLCreateE(&S->SDE,(char *)MSON[msonData]) == FAILURE))
            {
            MUISAddData(S,"ERROR:  could not create xml structure\n");

            MUArrayListFree(&NodeList);
            return(FAILURE);
            }

          DE = S->SDE;

          if (strstr(OptArgString,"time:"))
            {
            MStatGetRange(&OptArgString[strlen("time:")],&STime,&ETime);
            }
          else
            {
            MUGetPeriodRange(MSched.Time,0,0,mpDay,&STime,NULL,NULL);
 
            ETime = MSched.Time;
            }

          if (ETime < STime)
            {
            /* bad times */

            MUArrayListFree(&NodeList);
            return(SUCCESS);
            }

          MNodeProfileToXML(
            NodeExp,
            N,
            NULL,
            NULL,
            STime,
            ETime,
            -1,
            DE,
            TRUE,
            !bmisset(&CFlags,mcmVerbose),  /* DoSummary is TRUE if Vervose is not set */
            TRUE);  
          }  /* END if (strstr(ArgString,"profile")) */
        else if (strstr(ArgString,"cat"))
          {
          /* show node profile statistics */

          long STime;
          long ETime;
   
          mxml_t *DE;

          if ((S->SDE == NULL) && 
              (MXMLCreateE(&S->SDE,(char *)MSON[msonData]) == FAILURE))
            {
            MUISAddData(S,"ERROR:  could not create xml structure\n");

            MUArrayListFree(&NodeList);
            return(FAILURE);
            }

          DE = S->SDE;

          if (strstr(OptArgString,"time:"))
            {
            MStatGetRange(&OptArgString[strlen("time:")],&STime,&ETime);
            }
          else
            {
            MUGetPeriodRange(MSched.Time,0,0,mpDay,&STime,NULL,NULL);
 
            ETime = MSched.Time;
            }

          if (ETime < STime)
            {
            /* bad times */

            MUArrayListFree(&NodeList);
            return(SUCCESS);
            }

          MNodeProfileToXML(NodeExp,N,NULL,NULL,STime,ETime,-1,DE,TRUE,TRUE,TRUE);
          }  /* END else if (strstr(ArgString,"cat")) */
        else if (strstr(ArgString,"diag"))
          {
          /* generate report similar to mdiag -n */

          /* NYI */
          }
        else if (strstr(ArgString,"wiki"))
          {
          mstring_t tmpString(MMAX_LINE);

          /* Don't need to check for ALL, that's done by NodeExp above.
              This will already be called for each node */

          MNodeToWikiString(N,NULL,&tmpString);

          MUISAddData(S,tmpString.c_str());
          } /* END else if (strstr(ArgString,"wiki")) */

        break;

      case mncmReinitialize:

        {
        char EMsg[MMAX_LINE] = {0};
        char NodeName[MMAX_LINE];

        /* if successful, node will be deleted */

        MUStrCpy(NodeName,N->Name,sizeof(NodeName));

        /* restore node to base unprovisioned state */

        if (MNodeReinitialize(N,NULL,EMsg,NULL) == FAILURE)
          {
          MUSNPrintF(&BPtr,&BSpace,"ERROR:  cannot reinitialized node '%s' (%s)\n",
            NodeName,
            EMsg);
          }
        else
          {
          MUSNPrintF(&BPtr,&BSpace,"successfully reinitialized node '%s'\n",
            NodeName);
          }

        N = NULL;

        MUISAddData(S,tmpLine);

        /* clear tmplLine to prevent duplicate reporting */

        tmpLine[0] = '\0';
        }  /* END BLOCK (case mncmReinitialize) */

        break;

      default:

        /* NO-OP */

        break;
      }  /* END switch (SCIndex) */

    if (SCIndex == mncmModify)
      /*
       * __MUICtlNodeModify operates on all of NodeList,
       * so there is no need to loop through all the nodes
       */
      break;

    if (tmpLine[0] != '\0')
      MUISAddData(S,tmpLine);
    }    /* END for (NIndex) */

  MUArrayListFree(&NodeList);

  return(SUCCESS);
  }  /* END MUINodeCtl() */
/* END MUINodeCtl.c */
