/* HEADER */

      
/**
 * @file MWikiVMUpdate.c
 *
 * Contains: MWikiVMUpdate
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  

#include "MWikiInternal.h"



/**
 * Update VM resource information as reported via WIKI interface 
 * 
 * @see __MNatClusterProcessData() - parent
 * @see MWikiNodeUpdate() - peer
 * @see MWikiNodeUpdateAttr() - child
 * @see MWikiUpdate() - child
 *
 * NOTE:  support for Wiki-reported VM attributes must be explicitly coded up 
 *        here.
 *
 * @param V (I) [modified]
 * @param String (I) [modified]
 * @param RM (I) [optional]
 * @param EMsg (O) [optional,minsize=MMAX_LINE] 
 */

int MWikiVMUpdate(

  mvm_t   *V,      /* I (modified) */
  char    *String, /* I (modified as side-effect) */
  mrm_t   *RM,     /* I (optional) */
  char    *EMsg)   /* O (optional,minsize=MMAX_LINE) */

  {
  char *ptr;
  char *TokPtr;

  enum MWNodeAttrEnum AIndex;

  mnode_t tmpN;

  if ((V == NULL) || (String == NULL))
    {
    return(FAILURE);
    }

  /* initialize() */

  memset(&tmpN,0,sizeof(tmpN));

  bmset(&tmpN.IFlags,mnifIsVM);

  /* NOTE:  update V if VM is reported either in VARATTRS or as a resource */

  V->UpdateIteration = MSched.Iteration;
  V->UpdateTime      = MSched.Time;
  bmunset(&V->IFlags,mvmifUnreported);

  if ((RM != NULL) && (!bmisset(&V->Flags,mvmfCreationCompleted) || (V->RM == NULL)))
    {
    MVMAddRM(V,RM);
    }

  bmset(&V->Flags,mvmfCreationCompleted);

  /* tokenize on white space and semi-colon */

  ptr = MUStrTokE(String," ;",&TokPtr);

  while (ptr != NULL)
    {
    /* locate attribute */

    AIndex = (enum MWNodeAttrEnum)MUGetIndexCI(ptr,MWikiNodeAttr,TRUE,0);

    /* update attribute */

    switch (AIndex)
      {
      case mwnaCClass:
      case mwnaCDisk:
      case mwnaCFS:
      case mwnaCMemory:
      case mwnaCProc:
      case mwnaCRes:      /* configured generic resources */
      case mwnaCSwap:

        /* NOTE:  code below is inefficient - improve */

        MCResInit(&tmpN.CRes);
        MCResCopy(&tmpN.CRes,&V->CRes);

        MWikiNodeUpdateAttr(ptr,AIndex,&tmpN,NULL,NULL,EMsg);

        MCResCopy(&V->CRes,&tmpN.CRes);
        MCResFree(&tmpN.CRes);
        if (tmpN.RealRes != NULL)
          {
          MCResFree(tmpN.RealRes);
          MUFree((char **)&tmpN.RealRes);
          }

        break;

      case mwnaAClass:
      case mwnaADisk:
      case mwnaAFS:
      case mwnaAMemory:
      case mwnaAProc:
      case mwnaARes:
      case mwnaASwap:

        MCResInit(&tmpN.ARes);
        MCResCopy(&tmpN.ARes,&V->ARes);

        MWikiNodeUpdateAttr(ptr,AIndex,&tmpN,NULL,NULL,EMsg);

        MCResCopy(&V->ARes,&tmpN.ARes);
        MCResFree(&tmpN.ARes);

        break;

      case mwnaCPULoad:
  
        tmpN.Load = 0.0;

        MWikiNodeUpdateAttr(ptr,AIndex,&tmpN,NULL,NULL,EMsg);

        V->CPULoad = tmpN.Load;

        break;

      case mwnaContainerNode:

        {
        char tmpLine[MMAX_LINE];
        char *TokPtr = tmpLine;
        char *AttrName;
        char *NodeName;
        mnode_t *VMParentNode;

        MUStrCpy(tmpLine,ptr,sizeof(tmpLine));

        AttrName = MUStrTok(NULL,"=",&TokPtr);
        NodeName = MUStrTok(NULL,"=",&TokPtr);

        if ((AttrName == NULL) || (NodeName == NULL))
          {
          MDB(2,fWIKI) MLog("ERROR:    cannot process VM attribute %s for VM %s\n",
            ptr,
            V->VMID);

          break;
          }

        if (MNodeFind(NodeName,&VMParentNode) == FAILURE)
          {
          MDB(2,fWIKI) MLog("ERROR:    cannot find or add node %s for VM %s\n",
            NodeName,
            V->VMID);

          break;
          }

        if (!bmisset(&V->Flags,mvmfCompleted))
          MVMSetParent(V,VMParentNode);
        }  /* END BLOCK (case mwnaContainerNode) */

        break;

      case mwnaGEvent:
   
        {
        char INameArray[MMAX_NAME * 2]; /* *2 to account for mtime */

        char  Value[MMAX_BUFFER];
        char *TokPtr;
        char *tmpPtr;
        char *IName;

        mgevent_obj_t  *GEvent;
        mgevent_desc_t *GEDesc;

        mulong MTime = MSched.Time;
        int len = strlen(MWikiNodeAttr[AIndex]);

        /* generic event */
   
        /* FORMAT:  GEVENT[hitemp]='xxx' */
        /*  With severity: GEVENT[hitemp:3]='xxx' */

        if (ptr[len] == '[')
          {
          char *head;
          char *tail;
 
          head = &ptr[len + 1];
          tail = strchr(head,']');
 
          if (tail != NULL)
            {
            /*len = MIN((int)sizeof(IName) - 1,tail - head);*/
            len = tail - head;
 
            MUStrCpy(INameArray,head,len + 1);
 
            INameArray[len] = '\0';

            ptr = tail + strlen("]=");
            }
          }  /* END if (AttrString[len] == '[') */

        IName = MUStrTok(INameArray,":",&TokPtr);

        /* Add the Event Desc and Item information */

        MGEventAddDesc(IName);
        MGEventGetOrAddItem(IName,&V->GEventsList,&GEvent);

        if ((TokPtr != NULL) && (strlen(TokPtr) > 0))
          {
          /* Still something in TokPtr after ':', it's mtime */

          tmpPtr = MUStrTok(NULL,":",&TokPtr);

          if (tmpPtr != NULL)
            {
            MTime = (int)strtol(tmpPtr,NULL,10);

            if (MTime <= 0)
              {
              MTime = MSched.Time;
              }
            }
          }

        GEvent->Severity = 0;

        MGEventGetDescInfo(IName,&GEDesc);
 
        /* check if event is associated with a VM */

        MUStrCpyQuoted(Value,ptr,sizeof(Value));

        MUStrDup(&GEvent->GEventMsg,Value); 

        if (GEvent->GEventMTime + GEDesc->GEventReArm <= MSched.Time)
          {
          /* process event as new */

          GEvent->GEventMTime = MTime;

          MGEventProcessGEvent(
            -1,
            mxoxVM,
            V->VMID,
            IName,
            0.0,
            mrelGEvent,
            Value);
          }
        }  /* END BLOCK (case mwnaGEvent) */
   
        break;

      case mwnaGMetric:

        {
        int gmindex;

        char IName[MMAX_NAME] = {0};
        char Value[MMAX_BUFFER];

        int len = strlen(MWikiNodeAttr[AIndex]);

        /* generic metrics */

        /* FORMAT:  GMETRIC[temp]=113.2 */

        if (ptr[len] == '[')
          {
          char *head;
          char *tail;

          head = &ptr[len + 1];
          tail = strchr(head,']');

          if (tail != NULL)
            {
            len = MIN((int)sizeof(IName) - 1,tail - head);

            MUStrCpy(IName,head,len + 1);

            IName[len] = '\0';

            ptr = tail + strlen("]=");
            }
          } /* END if (ptr[len] == '[') */

        MUStrCpyQuoted(Value,ptr,sizeof(Value));

        if ((IName[0] == '\0') || (Value[0] == '\0'))
          {
          /* malformed value, ignore */

          if (EMsg != NULL)
            {
            if (IName[0] == '\0')
              {
              snprintf(EMsg,MMAX_LINE,"gmetric attribute malformed - no label");
              }
            else
              {
              snprintf(EMsg,MMAX_LINE,"gmetric attribute '%s' malformed - no value",
                IName);
              }
            }

          return(FAILURE);
          }

        gmindex = MUMAGetIndex(meGMetrics,IName,mAdd);

        if ((gmindex <= 0) || (gmindex >= MSched.M[mxoxGMetric]))
          {
          if (EMsg != NULL)
            {
            snprintf(EMsg,MMAX_LINE,"cannot add gmetric '%s' - buffer full",
              IName);
            }

          return(FAILURE);
          }

        if (MVMSetGMetric(V,gmindex,Value) == FAILURE)
          {
          if (EMsg != NULL)
            {
            snprintf(EMsg,MMAX_LINE,"cannot add gmetric '%s'",
              IName);
            }

          return(FAILURE);
          }
        }  /* END BLOCK (case mwnaGMetric) */

        break;

      case mwnaOS:

        tmpN.ActiveOS = 0;

        MWikiNodeUpdateAttr(ptr,AIndex,&tmpN,NULL,NULL,EMsg);

        V->ActiveOS = tmpN.ActiveOS;

        break;

      case mwnaOSList:

        tmpN.OSList = V->OSList;

        MWikiNodeUpdateAttr(ptr,AIndex,&tmpN,NULL,NULL,EMsg);

        V->OSList = tmpN.OSList;

        break;

      case mwnaPowerIsEnabled:

        tmpN.PowerIsEnabled = (V->PowerState == mpowOff) ? FALSE : TRUE;

        MWikiNodeUpdateAttr(ptr,AIndex,&tmpN,NULL,NULL,EMsg);

        V->PowerState = (tmpN.PowerIsEnabled == TRUE) ? mpowOn : mpowOff;

        break;

      case mwnaRack:

        {
        char *vptr;

        /* NOTE:  do not use MWikiNodeUpdateAttr() which will validate VM
                  location against physical resource locations which may result
                  in conflict.
        */

        /* FORMAT:  rack=<RINDEX> */

        vptr = strrchr(ptr,'=');

        if (vptr != NULL)
          V->Rack = (int)strtol(vptr + 1,NULL,10);
        }  /* END BLOCK (case mwnaRack) */

        break;

      case mwnaSlot:

        {
        /* NOTE:  do not use MWikiNodeUpdateAttr() which will validate VM
                  location against physical resources which may result in 
                  conflict.
        */

        /* FORMAT:  slot=<SINDEX> */
        }  /* END BLOCK (case mwnaSlot) */

        break;

      case mwnaState:

        /* FORMAT:  <STATE>[:<SUBSTATE>] */

        tmpN.State    = V->State;
        tmpN.SubState = V->SubState;

        if (V->StateMTime == 0)
          V->StateMTime = MSched.Time;

        MWikiNodeUpdateAttr(ptr,AIndex,&tmpN,NULL,NULL,EMsg);

        if ((V->State == mnsDown) &&
            (tmpN.State == mnsUnknown))
          {
          /* old state was down, if new state is unknown that's not good enough */
          /* this is a hack because MSM can report a down node in Unknown and StateMTime
             gets updated each iteration, ARFDuration will never be reached */

          tmpN.State = mnsDown;
          }
        else if (V->State != tmpN.State)
          {
          V->StateMTime = MSched.Time;
          }

        V->SubState   = tmpN.SubState;

        if ((tmpN.SubState != NULL) && (tmpN.SubState[0] != '\0'))
          {
          if ((V->LastSubState == NULL) || (strcmp(V->SubState,V->LastSubState)))
            {
            /* Leave the last known substate, so only change if moving to a
               real value */

            V->LastSubStateMTime = MSched.Time;
            MUStrDup(&V->LastSubState,V->SubState);
            }
          }

        if (RM != NULL)
          V->RMState[RM->Index] = tmpN.State;
        else
          V->State = tmpN.State;

        if ((V->SubState != NULL) && !strcasecmp(V->SubState,"destroyed"))
          {
          /* VM has been destroyed - remove internal object */

          MDB(2,fWIKI) MLog("INFO:     VM '%s' reported destroyed via RM - removing VM\n",
            V->VMID);

          /* NOTE:  VM will be removed in MRMUpdate() when VM is no longer 
                    reported by RM */

          bmset(&V->Flags,mvmfDestroyed);
          bmunset(&V->Flags,mvmfCanMigrate);
          }

        /* LastSubState may have been alloc'd in MWikiNodeUpdateAttr */

        MUFree(&tmpN.LastSubState);

        break;

      case mwnaVarAttr:

        {
        if (MUStrStr(ptr,"sovereign",0,TRUE,FALSE))
          bmset(&V->Flags,mvmfSovereign);
 
        if (MUStrStr(ptr,"cannotmigrate",0,TRUE,FALSE))
          bmunset(&V->Flags,mvmfCanMigrate);

        if (MUStrStr(ptr,"canmigrate",0,TRUE,FALSE))
          bmset(&V->Flags,mvmfCanMigrate);
        }  /* END BLOCK (case mwnaVarAttr) */

        break;

      case mwnaVariables:

        {
        /* FORMAT:  VARIABLE=<VAR LIST> */
        /* strip off the "VARIABLE=" */

        char *vptr = strrchr(ptr,'=');

        if ((vptr != NULL) && (vptr[1] != '\0'))
          {
          vptr = &vptr[1];
          MVMSetAttr(V,mvmaVariables,vptr,mdfString,mAdd);
          }
        }

        break;

      case mwnaNetAddr:
        {
        char *vptr = strrchr(ptr,'=');
        if (vptr != NULL)
          MUStrDup(&V->NetAddr,vptr+1);
        }

        break;

      default:

        /* attribute not supported for VM's */

        /* NO-OP */

        break;
      }  /* END switch (AIndex) */

    ptr = MUStrTokE(NULL," ;",&TokPtr);
    }    /* END while (ptr != NULL) */

  if (bmisset(&V->Flags,mvmfSovereign) &&
      !bmisset(&V->Flags,mvmfDestroyed))
    {
    /* NOTE:  if V is sovereign, create VM system job to track/consume
              VM resources */

    if ((V->J == NULL) && (V->N != NULL))
      {
      char     tmpName[MMAX_NAME];
      marray_t JArray;
      mnl_t  tmpNL;

      MUArrayListCreate(&JArray,sizeof(mjob_t *),1);

      MNLInit(&tmpNL);

      MNLSetNodeAtIndex(&tmpNL,0,V->N);
      MNLSetTCAtIndex(&tmpNL,0,1);
      MNLTerminateAtIndex(&tmpNL,1);

      snprintf(tmpName,sizeof(tmpName),"vmmap-%s",
        V->VMID);

      MUGenerateSystemJobs(
        NULL,
        NULL,
        &tmpNL,
        msjtVMMap,
        tmpName,
        -1,
        NULL,
        (void *)V,
        FALSE,
        FALSE,
        NULL,
        &JArray);  /* O */

      MNLFree(&tmpNL);

      V->J = *(mjob_t **)MUArrayListGet(&JArray,0);

      MUArrayListFree(&JArray);
      }  /* END if (V->J == NULL) */
    }    /* END if (V->IsConsumer == TRUE) */

  MVMUpdate(V);
  MVMUpdateVMTracking(V);
  MVMUpdateOutOfBandRsv(V); /* Will exit if not an out-of-band VM */

  bmset(&V->IFlags,mvmifReadyForUse);

  MVMTransition(V);

  return(SUCCESS);
  }  /* END MWikiVMUpdate() */

/* END MWikiVMUpdate.c */
