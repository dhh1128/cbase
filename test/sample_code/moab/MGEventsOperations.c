/* HEADER */


/**
 * @file MGEventsOperations.c
 *
 * This file contains functions that utilize GEvents for moab
 *
 */

#include "moab.h"
#include "moab-proto.h"
#include "moab-const.h"
#include "moab-global.h"


/**
 * Helper function to perform the details on processing a GEvent item
 * 
 * Test if the RecordEventList flag OR Record flag is on, if so
 * record the information on this GEvent
 *
 * @param     OType
 * @param     OID
 * @param     GOAList
 * @param     EType
 * @param     GOName
 * @param     Msg
 */

void __MGEventCheckAndRecordGEvent(
    
  enum MXMLOTypeEnum         OType,      
  char                      *OID,       
  mbitmap_t                 *GOAList,
  enum MRecordEventTypeEnum  EType,      
  char                      *GOName,
  const char                *Msg)

  {
  char            tmpLine[MMAX_LINE];
  void           *O = NULL;

  /* GEvent */

  if (bmisset(&MSched.RecordEventList,mrelGEvent) ||
     (bmisset(GOAList,mgeaRecord)))
    {
    if (Msg != NULL)
      {
      snprintf(tmpLine,sizeof(tmpLine),"%s MSG='%s'",
        GOName,
        (Msg != NULL) ? Msg : "-");
      }
    else
      {
      snprintf(tmpLine,sizeof(tmpLine),"%s -",
        GOName);
      }

    if (MOGetObject(OType,OID,&O,mVerify) == SUCCESS)
      {
      MOSetMessage(O,OType,(void **)Msg,mdfString,mAdd);
      }

    MSysRecordEvent(
      OType,
      OID,
      EType,
      GOName,
      tmpLine,
      NULL);
    }  /* if (bmisset(&MSched.RecordEventList,mrelGEvent) || ...) */
  } /* END __MGEventCheckAndRecordGEvent() */

/**
 * Helper for Notifying someone via EMAIL
 *
 */

void __MGEventCheckAndNofiyViaEmail(

  enum MXMLOTypeEnum         OType,      
  char                      *OID,       
  mbitmap_t                 *GOAList,
  char                      *GOName,
  const char                *Msg)        

  {
  char            tmpLine[MMAX_LINE];
  void           *O = NULL;

  if (bmisset(GOAList,mgeaNotify))
    {
    snprintf(tmpLine,sizeof(tmpLine),"moab event - %s (%s:%s)",
      GOName,
      MXO[OType],
      (OID != NULL) ? OID : "");

    if (MOGetObject(OType,OID,&O,mVerify) == SUCCESS)
      {
      MOSetMessage(O,OType,(void **)Msg,mdfString,mAdd);
      }

    MSysSendMail(
      MSysGetAdminMailList(1),
      NULL,
      tmpLine,
      NULL,
      Msg);
    }  /* END if (bmisset(GOAList,mgeaNotify)) */
  } /* END __MGEventCheckAndNofiyViaEmail() */




/**
 * Helper function to get XML string for a gevent. 
 *
 * @param GEName [I] - The gevent to find
 * @param OType  [I] - Type of object to search for the gevent
 * @param OID    [I] - Name of parent object
 * @param Str    [O] - Output XML string (dup'd)
 */

int __MGEventGetGEventXML(

  enum MXMLOTypeEnum         OType,
  char                      *OID,
  char                      *GEName,
  char                     **Str)

  {
  mgevent_obj_t  *GEvent = NULL;
  mgevent_list_t *GEventList = NULL;

  if ((MGEventGetListFromObject(OType,OID,&GEventList) == SUCCESS) &&
      (MGEventGetItem(GEName,GEventList,&GEvent) == SUCCESS))
    {
    mxml_t      *E = NULL;
    mxml_t      *GEventXML = NULL;
    mstring_t    tmpStr(MMAX_LINE);
    int          rc = FAILURE;

    MXMLCreateE(&E,"temp");
    MGEventItemToXML(E,GEName,GEvent);

    MXMLGetChild(E,"gevent",NULL,&GEventXML);

    if (GEventXML != NULL)
      {
      MXMLToMString(GEventXML,&tmpStr,NULL,TRUE);

      if (MUStrDup(Str,tmpStr.c_str()) == SUCCESS)
        {
        rc = SUCCESS;
        }
      }

    MXMLDestroyE(&E);

    return(rc);
    }

  return(FAILURE);
  }  /* END __MGEventGetGEventXML() */


/**
 * Helper routine to Check and Execute a cmd upon Event
 *
 */

void  __MGEventCheckAndExecute( 

  enum MXMLOTypeEnum         OType,
  char                      *OID,
  mbitmap_t                 *GOAList,
  char                     **GOADetails,
  char                      *GEName         /* If !NULL then a GEvent instance */
  )

  {
  char tmpLine[MMAX_LINE];
  char tmpCmd[MMAX_LINE];

  mln_t *VarList = NULL;

  char  tmpVarValBuf[MDEF_VARVALBUFCOUNT][MMAX_BUFFER3];  /* hold tmp var values */

  int   tmpVarCount;
  char *StdInInput = NULL;

  if ((bmisset(GOAList,mgeaExecute)) && (GOADetails[mgeaExecute] != NULL))
    {
    /* FORMAT:  <EXEC>[?<ARG>[&<ARG>]...] */

    MUStrCpy(tmpLine,GOADetails[mgeaExecute],sizeof(tmpLine));

    MUStrReplaceChar(tmpLine,'?',' ',TRUE);
    MUStrReplaceChar(tmpLine,'&',' ',TRUE);

    if ((GEName != NULL) && (bmisset(GOAList,mgeaObjectXML)))
      {
      /* Need to create XML of gevent and pass to stdin */

      __MGEventGetGEventXML(OType,OID,GEName,&StdInInput);

      }  /* END if ((GEName != NULL) && (bmisset(GOAList,mgeaObjectXML))) */

    /* NOTE:  perform variable substitution */

    tmpVarCount = 0;

    MOGetCommonVarList(
      OType,
      OID,
      NULL,
      &VarList,
      tmpVarValBuf,
      &tmpVarCount);  /* I/O */

    MUInsertVarList(
      tmpLine,
      VarList,
      NULL,
      NULL,
      tmpCmd, /* O */
      sizeof(tmpCmd),
      FALSE);

    MUReadPipe2(
      tmpCmd,
      StdInInput,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL);

    MULLFree(&VarList,MUFREE);
    MUFree(&StdInInput);
    }  /* END ((bmisset(GOAList,mgeaExecute)) && ...) */
  } /* END __MGEventCheckAndExecute() */


  /**
   * Helper routine to Check if GOAList flag (mgeaOff or mgeaOn)
   * are set, if either is on, then we need to Notify the NODE object
   *
   */

void __MGEventCheckAndSetOnOrOffAttr(
    
  enum MXMLOTypeEnum         OType,
  char                      *OID,
  mbitmap_t                 *GOAList)

  {
  void            *O = NULL;

  if (bmisset(GOAList,mgeaOff) || bmisset(GOAList,mgeaOn))
    {
    /* power off resource */

    switch (OType)
      {
      case mxoNode:

        if (MOGetObject(OType,OID,&O,mVerify) == SUCCESS)
          {
          if (MNodeSetAttr(
                (mnode_t *)O,
                mnaPowerIsEnabled,
                (void **)(bmisset(GOAList,mgeaOn) ? "on" : "off"),
                mdfString,
                mVerify) == FAILURE)
            {
            /* NO-OP */
            }
          }

        break;

      default:

        /* NO-OP */

        break;
      }  /* END switch (OType) */
    }    /* END if (bmisset(GOAList,mgeaOff) || ...) */
  } /* END __MGEventCheckAndSetOnOrOffAttr() */


/**
 * Helper function to check and then preempt a job if set
 *
 */

void  __MGEventCheckAndPreemptJobs(

  enum MXMLOTypeEnum         OType,
  char                      *OID,
  mbitmap_t                 *GOAList,
  char                     **GOADetails)

  {
  void            *O = NULL;

  if (bmisset(GOAList,mgeaPreempt))
    {
    /* preempt jobs */

    switch (OType)
      {
      case mxoNode:

        if (MOGetObject(OType,OID,&O,mVerify) == SUCCESS)
          {
          enum MPreemptPolicyEnum PreemptPolicy = mppNONE;

          if (GOADetails[mgeaPreempt] != NULL)
            {
            PreemptPolicy = (enum MPreemptPolicyEnum)MUGetIndexCI(
              GOADetails[mgeaPreempt],
              MPreemptPolicy, 
              FALSE,
              0);
            }

          if (PreemptPolicy == mppNONE)
            {
            if (((mnode_t *)O)->NP != NULL)
              PreemptPolicy = ((mnode_t *)O)->NP->PreemptPolicy;
            }
 
          MNodePreemptJobs(
            (mnode_t *)O,
            "generic event trigger fired",
            PreemptPolicy,
            TRUE);
          }

        break;

      default:

        /* NO-OP */

        break;
      }  /* END switch (OType) */
    }    /* END if (bmisset(GOAList,mgeaPreempt)) */
  } /* END __MGEventCheckAndPreemptJobs() */



/**
 * Helper function to check and reserve if flag is set
 *
 */

void __MGEventCheckAndReserve(
    
  enum MXMLOTypeEnum         OType,
  char                      *OID,
  mbitmap_t                 *GOAList,
  char                     **GOADetails,
  char                      *GOName,
  const char                *Msg)

  {
  void *O = NULL;

  if (bmisset(GOAList,mgeaReserve))
    {
    /* create reservation */

    switch (OType)
      {
      case mxoNode:

        if (MOGetObject(OType,OID,&O,mVerify) == SUCCESS)
          {
          mulong Duration = 0;

          if (GOADetails[mgeaReserve] != NULL)
            {
            Duration = MUTimeFromString(GOADetails[mgeaReserve]);
            }

          if (Duration <= 0)
            Duration = MCONST_DAYLEN;

          MNodeReserve(
            (mnode_t *)O,
            Duration,
            GOName,
            NULL,
            Msg);
          }

        break;

      default:

        /* NO-OP */

        break;
      }  /* END switch (OType) */
    }    /* END if (bmisset(GOAList,mgeaReserve)) */
  } /* END  __MGEventCheckAndReserve() */


/**
 * Helper function to perform a Node reboot as a result
 * of a Event
 */
void __MGEventCheckAndReset(

  enum MXMLOTypeEnum         OType,
  char                      *OID,
  mbitmap_t                 *GOAList)
    
  {
  void *O = NULL;

  if (bmisset(GOAList,mgeaReset))
    {
    /* reset resource immediately */

    switch (OType)
      {
      case mxoNode:

        if (MOGetObject(OType,OID,&O,mVerify) == SUCCESS)
          {
          if (MNodeReboot((mnode_t *)O,NULL,NULL,NULL) == FAILURE)
            {
            /* NO-OP */
            }
          }

        break;

      default:

        /* NO-OP */

        break;
      }  /* END switch (OType) */
    }    /* END if (bmisset(GOAList,mgeaReset)) */
  } /* END __MGEventCheckAndReset() */


/**
 * Helper function to check for Idle and reset if flag set
 *
 */

void __MGEventCheckAndResetWhenIdle(

  enum MXMLOTypeEnum         OType,
  char                      *OID,
  mbitmap_t                 *GOAList)

  {
  void *O = NULL;

  if (bmisset(GOAList,mgeaScheduledReset))
    {
    /* reset resource when idle */

    switch (OType)
      {
      case mxoNode:

        if (MOGetObject(OType,OID,&O,mVerify) == SUCCESS)
          {
          mnl_t tmpNL = {0};

          MNLInit(&tmpNL);

          MNLSetNodeAtIndex(&tmpNL,0,(mnode_t *)O);
          MNLSetTCAtIndex(&tmpNL,0,1);
          MNLTerminateAtIndex(&tmpNL,1);

          /* verify existing reset request not already queued */

          if (MNodeGetSysJob(MNLReturnNodeAtIndex(&tmpNL,0),msjtReset,MBNOTSET,NULL) == FAILURE)
            {
            if (MUGenerateSystemJobs(
                 NULL,
                 NULL,
                 &tmpNL,
                 msjtReset,
                 "reset",
                 -1,
                 NULL,
                 NULL,
                 FALSE,
                 TRUE,
                 NULL,
                 NULL) == FAILURE)
              {
              /* NO-OP */
              }
            }

          MNLFree(&tmpNL);
          }

        break;

      default:

        /* NO-OP */

        break;
      }  /* END switch (OType) */
    }    /* END if (bmisset(GOAList,mgeaScheduledReset)) */
  } /* END  __MGEventCheckAndResetWhenIdle() */



/** 
 * Help function to check flag and if set, signal jobs
 */

void __MGEventCheckAndSignalJobs(

  enum MXMLOTypeEnum         OType,
  char                      *OID,
  mbitmap_t                 *GOAList,
  char                     **GOADetails)

  {
  void *O = NULL;

  if (bmisset(GOAList,mgeaSignal))
    {
    /* signal jobs */

    switch (OType)
      {
      case mxoNode:

        if (MOGetObject(OType,OID,&O,mVerify) == SUCCESS)
          {
          MNodeSignalJobs(
            (mnode_t *)O,
            (GOADetails[mgeaSignal] != NULL) ? GOADetails[mgeaSignal] : (char *)"sigint",
            TRUE);
          }

        break;

      default:

        /* NO-OP */

        break;
      }  /* END switch (OType) */
    }    /* END if (bmisset(GOAList,mgeaSignal)) */
  } /* END __MGEventCheckAndSignalJob() */


/**
 * Helper function to Adjust Object State if flag is set 
 *
 */

void __MGEVentCheckAndAdjustObjectState(

  enum MXMLOTypeEnum         OType,
  char                      *OID,
  mbitmap_t                 *GOAList)

  {
  void  *O = NULL;

  if (bmisset(GOAList,mgeaDisable))
    {
    switch (OType)
      {
      case mxoNode:

        if (MOGetObject(OType,OID,&O,mVerify) == SUCCESS)
          {
          enum MNodeStateEnum state = mnsDrained;

          MRMNodeModify(
            (mnode_t*)O,
            NULL,  /* vm */
            NULL,  /* ext */
            NULL,  /* rm */
            mnaNodeState,
            NULL,
            (void *)MNodeState[mnsDrained],  /* Value */
            TRUE,
            mSet,
            NULL,
            NULL);

          MNodeSetState(
            (mnode_t *)O,
            state,
            FALSE);
          }

        break;

      default:

        /* NO-OP */

        break;
      }  /* END switch (OType) */
    }    /* END if (bmisset(GOAList,mgeaDisable)) */
  } /* END __MGEVentCheckAndAdjustObjectState() */


/**
 * Helper function to Add Message to Parent Object if flag is set
 *
 */

void __MGEventCheckAndAddMessageToParentObject(

  enum MXMLOTypeEnum         OType,
  char                      *OID,
  mbitmap_t                 *GOAList,
  char                      *GEName,
  const char                *Msg)
    
  {
  if (bmisset(GOAList,mgeaFail))
    {
    switch (OType)
      {
      case mxoNode:
      case mxoxVM:

        {
        job_iter JTI;

        mjob_t *J  = NULL;
        mnode_t *N;

        if ((GEName == NULL) ||
            ((OType == mxoNode) &&
             (strcmp(GEName,"nodemodifyfail") != 0) &&
             (strcmp(GEName,"nodepowerfail") != 0) &&
             (strcmp(GEName,"nodecreatefail") != 0)))
          break;

        MJobIterInit(&JTI);

        /* must search for the system job creating this vm */

        while (MJobTableIterate(&JTI,&J) == SUCCESS)
          {
          if (J->System == NULL)
            continue;

          if ((OType == mxoxVM) &&
              ((J->System->VM == NULL) || (strcmp(J->System->VM,OID))))
            continue;

          if ((OType == mxoNode) &&
              ((MNodeFind(OID,&N) == FAILURE) ||
              (MNLFind(&J->NodeList,N,NULL) == FAILURE)))
            continue;


          bmset(&J->IFlags,mjifSystemJobFailure);

          MMBAdd(
            &J->MessageBuffer,
            (Msg != NULL) ? Msg : "event received",
            "N/A",
            mmbtOther,
            0,
            1,
            NULL);

          break;
          }
        }   /* END BLOCK (case mxoxVM|mxoNode) */

        break;

      default:

        /* NO-OP */

        break;
      }  /* END switch (OType) */
    }    /* END if (bmisset(GOAList,mgeaFail)) */
  } /* END __MGEventCheckAndAddMessageToParentObject() */


/**
 * Process generic events for the system.
 * 
 * @see MWikiNodeUpdateAttr() - parent
 * @see MSchedProcessOConfig() - peer - parses GEVENTCFG[] parameter
 *
 * @param GOIndex (I) generic object index - for GMetrics
 * @param OType (I) parent object type
 * @param OID (I)
 * @param GEName (I) gevent name - GEvents only
 * @param AThreshold (I) [not used]
 * @param EType (I)
 * @param Msg (I) [optional]
 */

int MGEventProcessGEvent(

  int                        GOIndex,    /* I generic object index (optional) */
  enum MXMLOTypeEnum         OType,      /* I parent object type */
  char                      *OID,        /* I */
  char                      *GEName,     /* I (optional) If NULL, then GEvent */
  double                     AThreshold, /* I (not used) */
  enum MRecordEventTypeEnum  EType,      /* I */
  const char                *Msg)        /* I (optional) */

  {
  char            *GOName;           /* generic object name */
  char           **GOADetails;       /* generic object action details */
  int              GOECount = 0;     /* generic object event count */
  int              GOEThreshold = 0; /* generic object event count threshold */
  mbitmap_t       *GOAList = NULL;   /* generic object action list */
  mulong           GOReArm = 0;      /* generic object rearm time */
  mulong           GOLastTime = 0;   /* last time object action was taken */
  mgevent_desc_t  *GEventDesc;       /* the GEvent description*/

  /* Determine which Event we are being called to deal with:  GEvent or GMetric
   * then harvest the data from the specific instance item
  */

  if (GEName != NULL)
    {
    /* It is a GEvent */

    if (MGEventGetDescInfo(GEName,&GEventDesc) == FAILURE)
      return(FAILURE);

    GEventDesc->GEventCount++;
    GOAList      = &GEventDesc->GEventAction;
    GOECount     = GEventDesc->GEventCount;
    GOEThreshold = GEventDesc->GEventAThreshold;

    GOName     = GEName;
    GOADetails = GEventDesc->GEventActionDetails;

    GOReArm    = GEventDesc->GEventReArm;
    GOLastTime = GEventDesc->GEventLastFired;
    }
  else
    {
    /* It is a GMetric */

    MGMetric.GMetricCount[GOIndex]++;

    GOAList      = &MGMetric.GMetricAction[GOIndex];
    GOECount     = MGMetric.GMetricCount[GOIndex];
    GOEThreshold = MGMetric.GMetricAThreshold[GOIndex];

    GOName     = MGMetric.Name[GOIndex];
    GOADetails = MGMetric.GMetricActionDetails[GOIndex];

    GOReArm    = MGMetric.GMetricReArm[GOIndex];
    GOLastTime = MGMetric.GMetricLastFired[GOIndex];
    }


  /* Check the various types of 'triggering'
   *
   * If they have NOT been reached, then no action to be taken
   */

  if ((GOEThreshold > 0) && (GOECount % GOEThreshold != 0))
    {
    /* threshold count has not been reached */

    return(SUCCESS);
    }

  /* Test if REARM time has been reached */
  if ((MSched.Time - GOReArm) < GOLastTime)
    {
    /* Rearm time has not past */

    return(SUCCESS);
    }

  /* Set the 'now' time of when this trigger was fired */

  if (GEName != NULL)
    {
    /* Set the last fired time for the GEvent */

    GEventDesc->GEventLastFired = MSched.Time; 
    }
  else
    {
    /* Set the last fired time for the GMetric */

    MGMetric.GMetricLastFired[GOIndex] = MSched.Time;
    }


  /*
   * Now process each of the 'event' flags, one (or two) flag(s) at a time
   * and then proceed to execute the actions for that (those) flag
   */

  /* GEvent: Go check, set attribute and record the event if flag is set */
  __MGEventCheckAndRecordGEvent(OType,OID,GOAList,EType,GOName,Msg);

  /* send mail: Go check and notify via Email if flag is set */
  __MGEventCheckAndNofiyViaEmail(OType,OID,GOAList,GOName,Msg);

  /* Execute: Go check and Execute if flag is set */
  __MGEventCheckAndExecute(OType,OID,GOAList,GOADetails,GEName);

  /* Power OFF or ON: Go check and set attr if flag is set */
  __MGEventCheckAndSetOnOrOffAttr(OType,OID,GOAList);

  /* Preempt: Go check and Preempty if flag is flag is set */
  __MGEventCheckAndPreemptJobs(OType,OID,GOAList,GOADetails);

  /* Reserve: Go check and Reserve if flag is flag is set */
  __MGEventCheckAndReserve(OType,OID,GOAList,GOADetails,GOName,Msg);

  /* Reset resource: Go check and do the reset if flag is set */
  __MGEventCheckAndReset(OType,OID,GOAList);

  /* Reset resource when idle: Go check and reset if flag is set */
  __MGEventCheckAndResetWhenIdle(OType,OID,GOAList);

  /* signal job: Go Check and signal if flag is set */
  __MGEventCheckAndSignalJobs(OType,OID,GOAList,GOADetails);

  /* adjust object state: Go check and adjust object state if flag is set */
  __MGEVentCheckAndAdjustObjectState(OType,OID,GOAList);

  /* Add message to parent : Go check and add message to parent if flag is set  */
  __MGEventCheckAndAddMessageToParentObject(OType,OID,GOAList,GEName,Msg);

  return(SUCCESS);
  }  /* END MGEventProcessGEvent() */




/**
 * Clears out all old gevents from the given List containter.  
 *
 * Old gevents will be freed
 *
 * @see MGEventClearOldGEvents - parent
 *
 * @param List (I) - The collection List to be checked for old gevents
 */

int MGEventClearOldGEventsFromList(

  mgevent_list_t *List)

  {
  char           *GEName;
  mgevent_obj_t  *GEvent;
  mgevent_iter_t  GEIter;

  mgevent_desc_t *GEDesc = NULL;

  if (List == NULL)
    return(FAILURE);

  MGEventIterInit(&GEIter);

  if (MGEventGetItemCount(List) > 0)
    {
    while (MGEventItemIterate(List,&GEName,&GEvent,&GEIter) == SUCCESS)
      {
      /* Get the GEvent Desc information for this instance */

      MGEventGetDescInfo(GEName,&GEDesc);

      /* If GEventReArm is specified, we will use that so that there is no
          chance of destroying a gevent before the rearm time is up */

      if ((GEDesc != NULL) && (GEDesc->GEventReArm > 0))
        {
        if (GEvent->GEventMTime + GEDesc->GEventReArm > MSched.Time)
          continue;
        }
      else
        {
        if (GEvent->GEventMTime + MSched.GEventTimeout > MSched.Time)
          continue;
        }

      /* GEvent is old, remove */

      MGEventRemoveItem(List,GEName);
      MUFree((char **)&GEvent->GEventMsg);
      MUFree((char **)&GEvent->Name);
      MUFree((char **)&GEvent);
      }
    }

  return(SUCCESS);
  } /* MGEventClearOldGEventsFromList() */



/**
 * Clears out any gevents that are older than the rearm time.  If no rearmtime was specified for that
 *  gevent, it will be deleted if it is older than five minutes.
 *
 * @see MServerUpdate - parent 
 */

int MGEventClearOldGEvents()

  {
  int nindex;
  mnode_t *N;

  /* Check scheduler gevents */

  MGEventClearOldGEventsFromList(&MSched.GEventsList);

  /* Check node gevents */

  for (nindex = 0;nindex < MSched.M[mxoNode];nindex++)
    {
    N = MNode[nindex];

    if ((N == NULL) || (N->Name[0] == '\0'))
      {
      break;
      }

    if (MGEventGetItemCount(&N->GEventsList) > 0)
      MGEventClearOldGEventsFromList(&N->GEventsList);

    /* Check VM gevents */

    if (N->VMList != NULL)
      {
      mln_t *Ptr;
      mvm_t *V;

      for (Ptr = N->VMList;Ptr != NULL;Ptr = Ptr->Next)
        {
        V = (mvm_t *)Ptr->Ptr;

        if (MGEventGetItemCount(&V->GEventsList) > 0)
          {
          MGEventClearOldGEventsFromList(&V->GEventsList);
          }
        } /* END for (Ptr = N->VMList;Ptr != NULL;Ptr = Ptr->Next) */
      } /* END if (N->VMList != NULL) */
    }

  return(SUCCESS);
  } /* END MGEventClearOldGEvents() */

