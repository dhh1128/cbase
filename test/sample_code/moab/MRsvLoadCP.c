/* HEADER */

      
/**
 * @file MRsvLoadCP.c
 *
 * Contains: MRsvLoadCP
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  
#include "MEventLog.h"

/**
 * Load and create reservation objects from checkpoint repository.
 *
 * NOTE: rsv attributes loaded from checkpoint files must be explicitly
 *       handled within this routine.

 * @see MCPStoreRsvList()
 * @see MRsvCreateFromXML() - peer - loads rsv attributes from peer RMs.
 *
 * @param RS (I) [optional]
 * @param Buf (I)
 */

int MRsvLoadCP(

  mrsv_t *RS,  /* I (optional) */
  char   *Buf) /* I */

  {
  char     tmpName[MMAX_NAME];
  char     RsvID[MMAX_NAME];
  char     tmpHostExpBuf[MMAX_BUFFER];
  char    *HostExpBufPtr = NULL;

  long     CkTime;

  char    *ptr;

  mxml_t *E = NULL;

  mrsv_t  *R;
  mrsv_t  *RsvP;
  mrsv_t   tmpR;

  int      rc;

  char     EMsg[MMAX_LINE];

  mbool_t  RIsLocal = FALSE;

  const char *FName = "MRsvLoadCP";

  MDB(4,fCKPT) MLog("%s(%s,%s)\n",
    FName,
    (RS != NULL) ? RS->Name : "NULL",
    (Buf != NULL) ? Buf : "NULL");

  if (Buf == NULL)
    {
    return(FAILURE);
    }

  /* if RS is specified, update RS.  if not, load rsv ckpt line into new rsv */

  /* FORMAT:  <RSVHEADER> <RSVID> <CKTIME> <RSVSTRING> */

  /* determine rsv ID */

  sscanf(Buf,"%64s %64s %ld",
    tmpName,
    RsvID,
    &CkTime);

  if (((long)MSched.Time - CkTime) > (long)MCP.CPExpirationTime)
    {
    MDB(2,fCKPT) MLog("WARNING:  checkpoint file has expired\n");

    return(SUCCESS);
    }

  if ((ptr = strchr(Buf,'<')) == NULL)
    {
    MDB(2,fCKPT) MLog("WARNING:  cannot locate checkpoint XML for reservation %s\n",
      (RS != NULL) ? RS->Name : "");

    return(FAILURE);
    }

  if (MXMLFromString(&E,ptr,NULL,EMsg) == FAILURE)
    {
    MDB(2,fCKPT) MLog("WARNING:  cannot process checkpoint XML for reservation %s - %s\n",
      (RS != NULL) ? RS->Name : "",
      EMsg);

    return(FAILURE);
    }

  if (RS == NULL)
    {
    if (MRsvFind(RsvID,&R,mraNONE) != SUCCESS)
      {
      /* cannot locate reservation - create new reservation */

      RIsLocal = TRUE;

      R = &tmpR;

      MRsvInitialize(&R);
      }
    }
  else
    {
    R = RS;
    }

  /* update original reservation */

  rc = MRsvFromXML(R,E,NULL);

  MXMLDestroyE(&E);

  if (rc == FAILURE)
    {
    MDB(2,fCKPT) MLog("WARNING:  cannot update reservation %s with checkpoint XML\n",
      (RS != NULL) ? RS->Name : "");

    if (RIsLocal == TRUE)
      MRsvDestroy(&R,FALSE,FALSE);

    return(FAILURE);
    }

  if ((R->RsvParent != NULL) &&
      bmisset(&R->Flags,mrfStanding) &&
      (MSRFind(R->RsvParent,NULL,NULL) != SUCCESS))
    {
    /* cannot locate parent standing reservation - assume parent reservation
     * has been removed from config file */

    /* DOUG:  should we destroy/free R? */
    /* DOUG:  should we purge entry from checkpoint? */

    MDB(5,fCKPT) MLog("INFO:     parent reservation not located - destroying reservation %s\n",
      R->RsvParent);

    if (RIsLocal == TRUE)
      MRsvDestroy(&R,FALSE,FALSE);

    return(FAILURE);
    }

  if ((R->DRes.Procs == 0) &&
      (R->DRes.Mem == 0) &&
      (R->DRes.Swap == 0) &&
      (R->DRes.Disk == 0) &&
      (MSNLIsEmpty(&R->DRes.GenericRes)))
    {
    /* reservations should reserve at least processors */

    R->DRes.Procs = -1;
    }

  if (R->Type == mrtNONE)
    R->Type = mrtUser;

  if (RIsLocal == TRUE)
    {
    if (bmisset(&R->Flags,mrfIsActive))
      {
      bmunset(&R->Flags,mrfIsActive);
      bmset(&R->Flags,mrfWasActive);
      }

    /* use the correct host expression */ 

    HostExpBufPtr = R->HostExp;

    if (!MNLIsEmpty(&R->NL))
      {
      if (MNLToString(&R->NL,FALSE,NULL,'\0',tmpHostExpBuf,sizeof(tmpHostExpBuf)) == FAILURE)
        {
        return(FAILURE);
        }

      HostExpBufPtr = tmpHostExpBuf;
      }

    if ((R->SubType == mrsvstVPC) &&
        (R->EndTime < MSched.Time))
      {
      /* this reservation is already over, but it's part of a VPC, we need to go through
         all the motions associated with removing a reservation (triggers, charging, logging).
         To address this set the reservation endtime to one second in the future (RT8384) */

      R->EndTime = MSched.Time + 1;
      }

    /* create new rsv */

    if (MRsvCreate(
         R->Type,
         R->ACL,
         R->CL,
         &R->Flags,
         &R->ReqNL,     /* ReqNL is NULL for most reservations */
         R->StartTime,
         R->EndTime,
         0,
         0,
         R->Name,
         &RsvP,       /* O - pointer to newly created reservation */
         HostExpBufPtr,  /* I */
         &R->DRes,
         NULL,
         TRUE,
         FALSE) == FAILURE)
      {
      /* cannot create reservation */

      MDB(2,fCKPT) MLog("WARNING:  cannot create new reservation from checkpoint XML for reservation %s\n",
        (RS != NULL) ? RS->Name : "");

      /* did the rsv expire while moab was down? */
      if ((R->StartTime >= R->EndTime) || (MSched.Time >= R->EndTime))
        {
        /* record rsv end event */
        MOWriteEvent((void *)R,mxoRsv,mrelRsvEnd,NULL,MStat.eventfp,NULL);

        CreateAndSendEventLog(meltRsvEnd,R->Name,mxoRsv,(void *)R);
        }

      if (RIsLocal == TRUE)
        MRsvDestroy(&R,FALSE,FALSE);

      return(FAILURE);
      }

    /* Rsv created using allocated node list, now we can restore original hostexp */

    if (R->HostExp != NULL)
      {
      MRsvSetAttr(RsvP,mraHostExp,(void *)R->HostExp,mdfString,mSet);
      }

    if (R->Label != NULL)
      {
      MRsvSetAttr(RsvP,mraLabel,(void **)R->Label,mdfString,mSet);
      }

    if ((R->O != NULL) && (R->OType != mxoNONE))
      {
      MMovePtr((char **)&R->O,(char **)&RsvP->O);

      RsvP->OType = R->OType;
      }

    RsvP->CAPS = R->CAPS;
    RsvP->CIPS = R->CIPS;

    RsvP->TAPS = R->TAPS;
    RsvP->TIPS = R->TIPS;

    /* NOTE:  do not load R->AllocTC and R->AllocNC from checkpoint, these
              should be regenerated */

    if (!bmisclear(&R->ReqFBM))
      MRsvSetAttr(RsvP,mraReqFeatureList,&R->ReqFBM,mdfOther,mSet);

    RsvP->ReqFMode   = R->ReqFMode;

    RsvP->ReqArch    = R->ReqArch;
    RsvP->ReqMemory  = R->ReqMemory;
    RsvP->ReqOS      = R->ReqOS;

    RsvP->CTime      = R->CTime;

    RsvP->CIteration = MSched.Iteration;

    RsvP->LastChargeTime = R->LastChargeTime;

    RsvP->IsTracked  = R->IsTracked;

    RsvP->PtIndex    = R->PtIndex;

    RsvP->LogLevel   = R->LogLevel;

    RsvP->SubType    = R->SubType;

    RsvP->Priority   = R->Priority;
    
    RsvP->LienCost   = R->LienCost;

    if (R->A != NULL)
      MRsvSetAttr(RsvP,mraAAccount,(void *)R->A,mdfOther,mSet);

    if (R->G != NULL)
      MRsvSetAttr(RsvP,mraAGroup,(void *)R->G,mdfOther,mSet);

    if (R->U != NULL)
      MRsvSetAttr(RsvP,mraAUser,(void *)R->U,mdfOther,mSet);

    if (R->Q != NULL)
      MRsvSetAttr(RsvP,mraAQOS,(void *)R->Q,mdfOther,mSet);

    if (R->ReqTC > 0)
      MRsvSetAttr(RsvP,mraReqTaskCount,(void *)&R->ReqTC,mdfInt,mSet);

    if (R->HostExpIsSpecified == TRUE)
      RsvP->HostExpIsSpecified = TRUE;

    if (R->MB != NULL)
      {
      MMovePtr((char **)&R->MB,(char **)&RsvP->MB);
      }

    MMovePtr(&R->SpecName,&RsvP->SpecName);

    if (R->SystemID != NULL)
      {
      MMovePtr(&R->SystemID,&RsvP->SystemID);
      }

    if (R->SystemRID != NULL)
      {
      MMovePtr(&R->SystemRID,&RsvP->SystemRID);
      }

    if (R->Comment != NULL)
      {
      MMovePtr(&R->Comment,&RsvP->Comment);
      }

    if (R->History != NULL)
      {
      MMovePtr((char **)&R->History,(char **)&RsvP->History);
      }

    if (R->RsvGroup != NULL)
      {
      MMovePtr(&R->RsvGroup,&RsvP->RsvGroup);
      }

    if ((!MNLIsEmpty(&R->ReqNL)) && (MNLIsEmpty(&RsvP->ReqNL)))
      {
      /* must preserve ReqNL because it holds the TaskCount information
       * (6-15-05) */

      MNLCopy(&RsvP->ReqNL,&R->ReqNL);
      }

    if (R->T != NULL)
      {
      MMovePtr((char **)&R->T,(char **)&RsvP->T);
      }

    if (R->Variables != NULL)
      {
      MMovePtr((char **)&R->Variables,(char **)&RsvP->Variables);
      }

    if (!bmisclear(&R->ReqFBM))
      {
      bmcopy(&RsvP->ReqFBM,&R->ReqFBM);
      }

    if (R->RsvParent != NULL)
      {
      /* NOTE:  this section should sync checkpointed rsvs against the
                parent SR */

      msrsv_t *SR;

      MMovePtr(&R->RsvParent,&RsvP->RsvParent);

      /* If RsvGroup not explicitly specified, RsvGroup should be set
       * to RsvParent */

      if (RsvP->RsvGroup == NULL)
        MUStrDup(&RsvP->RsvGroup,RsvP->RsvParent);

      if (MSRFind(RsvP->RsvParent,&SR,NULL) == SUCCESS)
        {
        long SRPeriodStart;
        long SRPeriodEnd;
        int SRPeriodIndex;

        long PeriodStart;
        long PeriodEnd;
        int PeriodIndex;

        mulong StartTime;
        mulong EndTime;

        long Offset;
        int  dindex;

        /* reservation is SR child, link to parent */

        /* determine time slot */

        MUGetPeriodRange(RsvP->StartTime,0,0,SR->Period,&SRPeriodStart,&SRPeriodEnd,&SRPeriodIndex);
        MUGetPeriodRange(MSched.Time,0,0,SR->Period,&PeriodStart,&PeriodEnd,&PeriodIndex);

        Offset = SRPeriodStart - PeriodStart;

        if ((Offset < 0) && (RsvP->StartTime < MSched.Time))
          {
          /* too old */

          MRsvDestroy(&RsvP,TRUE,TRUE);

          MDB(5,fCKPT) MLog("INFO:     reservation for specified period no longer needed - destroying reservation %s\n",
            (RS != NULL) ? RS->Name : "");

          if (RIsLocal == TRUE)
            MRsvDestroy(&R,FALSE,FALSE);

          return(FAILURE);
          }

        switch (SR->Period)
          {
          case mpHour:
  
            dindex = Offset / MCONST_HOURLEN;
  
            break;
  
          case mpDay:

            dindex = Offset / MCONST_DAYLEN;

            break;

          case mpWeek:
  
            dindex = Offset / MCONST_WEEKLEN;
  
            break;
  
          case mpMonth:
  
            dindex = Offset / MCONST_MONTHLEN;
  
            break;
  
          default:
          case mpInfinity:
  
            dindex = 0;
  
            break;
          }

        /* NOTE:  if the SR has changed ANY aspect of time
                  (period,starttime,endtime,days) delete this child rsv
                  and rebuild from scratch */

        if (dindex >= (SR->Depth + 1))
          {
          /* this depth is not needed */

          MRsvDestroy(&RsvP,TRUE,TRUE);

          MDB(5,fCKPT) MLog("INFO:     reservation for specified period no longer needed - destroying reservation %s\n",
            (RS != NULL) ? RS->Name : "");

          if (RIsLocal == TRUE)
            MRsvDestroy(&R,FALSE,FALSE);

          return(FAILURE);
          }

        MSRGetAttributes(SR,dindex,&StartTime,&EndTime);

        /* logic changed 3-18-08 by DRW, could result in unnecessarily
         * destroying rsvs */

        if (SR->Period == mpInfinity)
          {
          /* NO-OP */
          /* SR is still an infinity reservation */
          }
        else if (((RsvP->StartTime != StartTime) &&
                  (StartTime != MSched.Time) &&
                  (RsvP->StartTime != MSched.Time)) ||
                 (RsvP->EndTime != StartTime + EndTime))
          {
          /* time settings on SR have changed, delete CP rsv and start
           * from scratch */

          MRsvDestroy(&RsvP,TRUE,TRUE);

          MDB(5,fCKPT) MLog("INFO:     reservation time constraints have changed - destroying reservation %s\n",
            (RS != NULL) ? RS->Name : "");

          if (RIsLocal == TRUE)
            MRsvDestroy(&R,FALSE,FALSE);

          return(FAILURE);
          }

        /* NOTE:

        For RT4472 this was changed by changeset 7517:540282cf58ee:

        if ((MSched.UseCPRsvNodeList == FALSE) &&
            (RsvP->HostExp != NULL) &&
            (SR->HostExp != NULL) &&
            (strcmp(RsvP->HostExp,SR->HostExp)))

        But this means that any change to the hostlist is ignored for standing
        reservations.  I am reverting the behavior back to pre-4472 so that
        changes to the hostexp are caught and dealt with. */

        if ((RsvP->HostExp != NULL) &&
            (SR->HostExp != NULL) &&
            (RsvP->HostExpIsSpecified == TRUE) &&
            (strcmp(RsvP->HostExp,SR->HostExp)))
          {
          /* hostexp settings on SR have changed, delete CP rsv and start
           * from scratch */

          MRsvDestroy(&RsvP,TRUE,TRUE);

          MDB(5,fCKPT) MLog("INFO:     reservation host expression/constraints have changed - destroying reservation %s\n",
            (RS != NULL) ? RS->Name : "");

          if (RIsLocal == TRUE)
            MRsvDestroy(&R,FALSE,FALSE);

          return(FAILURE);
          }

        if (bmcompare(&SR->FBM,&RsvP->ReqFBM) != 0)
          {
          MRsvDestroy(&RsvP,TRUE,TRUE);

          if (RIsLocal == TRUE)
            MRsvDestroy(&R,FALSE,FALSE);

          return(FAILURE);
          }

        if (SR->LogLevel != RsvP->LogLevel)
          {
          /* DOUG:  why destroy reservation with different loglevel? */

          MRsvDestroy(&RsvP,TRUE,TRUE);

          if (RIsLocal == TRUE)
            MRsvDestroy(&R,FALSE,FALSE);

          return(FAILURE);
          }

        if ((RsvP->MB == NULL) && (SR->MB != NULL))
          {
          MMBCopyAll(&RsvP->MB,SR->MB);
          }

        /* NOTE:  the +1 is needed because MSRUpdate ALWAYS brings
                  reservations forward so we need to offset that with a +1 */

        SR->R[dindex + 1] = RsvP;

        if (SR->ReqTC != RsvP->ReqTC)
          RsvP->ReqTC = SR->ReqTC;

        if (SR->DRes.Procs != RsvP->DRes.Procs)
          RsvP->DRes.Procs = SR->DRes.Procs;
                
        if (SR->DRes.Mem != RsvP->DRes.Mem)
          RsvP->DRes.Mem = SR->DRes.Mem;
        } /* END if (MSRFind() == SUCCESS) */
      }

    /* DOUG:  Should parameter 3 be FALSE? */

    MRsvDestroy(&R,FALSE,TRUE); /* done needing local R */

    R = RsvP;

    /* determine what to do with nodes */

    if ((R->HostExp == NULL) &&
        (!MNLIsEmpty(&R->ReqNL)))
      {
      mstring_t String(MMAX_LINE);

      if (MNLToRegExString(&R->ReqNL,&String) == FAILURE)
        {
        return(FAILURE);
        }

      MUStrDup(&R->HostExp,String.c_str());

      R->HostExpIsSpecified = MBNOTSET;

      if (R->RsvParent != NULL)
        {
        /* NOTE standing reservations never have ReqNL populated */

        MNLFree(&R->ReqNL);
        }
      }
    }  /* END if (RIsLocal == TRUE) */

  MRsvCreateCredLock(R);

  MRsvTransition(R,FALSE);
 
  return(SUCCESS);
  }  /* END MRsvLoadCP() */
/* END MRsvLoadCP.c */
