/* HEADER */

/**
 * @file MUIRsvCtlModify.c
 *
 * Contains MUI Rsv Control Modify function
 */



#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 * Modify existing reservation (process 'mrsvctl -m').
 *
 * NOTE:  for multiple -m statements such as:
 *
 *        mrsvctl -m variable+="name=val" -m duration="01:00:00"
 *
 *        the only output will be from the last -m on the request. At some
 *        point we may want to change this by building up the reply/output
 *        string that gets sent back to the client as we go through the
 *        requests.
 * 
 * @see MUIRsvCtl() - parent
 * @see MRsvSetAttr() - child
 *
 * @param R (I) [modified]
 * @param RE (I)
 * @param EMsg (O) [required,minsize=MMAX_LINE,will be reported to requestor]
 */

int MUIRsvCtlModify(

  mrsv_t *R,     /* I (modified) */
  mxml_t *RE,    /* I */
  char   *EMsg)  /* O (required,minsize=MMAX_LINE,will be reported to requestor) */

  {
  int rc;

  char  tmpName[MMAX_NAME];
  char  tmpVal[MMAX_LINE];
  char  tmpOp[MMAX_NAME];
  char  tmpLine[MMAX_LINE];
  char  FlagLine[MMAX_LINE];

  char *ptr;
  char *TokPtr;

  int   Mode;

  mxml_t *SE;

  int   STok;

  mulong tmpTime;

  mbool_t ReqDefChanged = FALSE;

  enum MRsvAttrEnum AIndex = mraNONE;

  enum MObjectSetModeEnum OIndex = mSet;  /* modify operation type */

  char *MPtr;
  int   MSpace;

  if (EMsg == NULL)
    {
    return(FAILURE);
    }

  FlagLine[0] = '\0';
  EMsg[0]     = '\0';

  /* allow modification, but not cancellation of VPC rsvs */

#ifdef MNOT
  if ((bmisset(&R->Flags,mrfStatic)) || (bmisset(&R->Flags,mrfStanding)))
    {
    sprintf(EMsg,"ERROR:  cannot modify 'static/standing' reservation %s\n",
      R->Name);

    return(FAILURE);
    }
#endif /* MNOT */

  rc = FAILURE;

  STok = -1;

  MXMLGetAttr(RE,MSAN[msanFlags],NULL,FlagLine,sizeof(FlagLine));

  while (MS3GetSet(
      RE,
      &SE,
      &STok,
      tmpName,
      sizeof(tmpName),
      tmpVal,
      sizeof(tmpVal)) == SUCCESS)
    {
    MXMLGetAttr(SE,MSAN[msanOp],NULL,tmpOp,sizeof(tmpOp)); 

    /* FORMAT:  <X>=<Y> */

    AIndex = (enum MRsvAttrEnum)MUGetIndexCI(tmpName,MRsvAttr,FALSE,mraNONE);

    /* Do not allow the name to be changed (user should change label instead) */
    if ((AIndex == mraName) ||(AIndex == mraSpecName))
      {
      sprintf(EMsg,"ERROR:  cannot modify name for existing reservations\n");

      return(FAILURE);
      }

    if (MXMLGetAttr(SE,MSAN[msanOp],NULL,tmpOp,sizeof(tmpOp)) == SUCCESS)
      {
      OIndex = (enum MObjectSetModeEnum)MUGetIndexCI(
        tmpOp,
        MObjOpType,
        FALSE,
        mSet);
      }
    else
      {
      OIndex = mSet;
      }

    switch (AIndex)
      {
      case mraACL:

        /* make sure to unset - default is set */
        if (strstr(FlagLine,"unset") != NULL)
          {
          OIndex = mUnset;
          }

        /* check to see if we're really trying to add/increment or decrement
         * here */

        else if (strstr(tmpVal,"+=") != NULL)
          {
          OIndex = mAdd;
          }

        else if (strstr(tmpVal,"-=") != NULL)
          {
          OIndex = mDecr;
          }

        if ((rc |= MRsvSetAttr(
             R,
             mraACL,
             (void *)tmpVal,
             mdfString,
             OIndex)) == FAILURE)
          {
          snprintf(EMsg,MMAX_LINE,"ERROR:  could not change ACL for rsv %s\n",
            R->Name);
          }
        else
          {
          snprintf(EMsg,MMAX_LINE,"successfully changed ACL for rsv %s\n",
            R->Name);

          MRsvCreateCredLock(R);
          }

        break;

      case mraDuration:
      case mraEndTime:

        /* If changing duration or endtime, check if a trigger associated 
           with the endtime has already fired which does not have multifire
           on. */

        if (bmisset(&R->Flags,mrfEndTrigHasFired))
          {
          snprintf(EMsg,MMAX_LINE,"ERROR:  cannot change %s when an endtime non-multifire trigger has fired\n",
            MRsvAttr[AIndex]);

          return(FAILURE); 
          }

        /* Do NOT put a break here -> if there is no trigger problem, use 
           same logic in mraStartTime to change the duration/endtime */

      case mraStartTime:

        {
        long StartTime;
        long EndTime;

        StartTime = R->StartTime;
        EndTime   = R->EndTime;

        switch (OIndex)
          {
          case mAdd:
          case mDecr:

            switch (AIndex)
              {
              case mraEndTime:

                tmpTime = R->EndTime;

                break;

              case mraDuration:

                if (R->Duration > 0)
                  tmpTime = R->Duration;
                else
                  tmpTime = (R->EndTime - R->StartTime);

                break;

              case mraStartTime:
              default:

                tmpTime = R->StartTime;

                break;
              }

            if (OIndex == mIncr)
              tmpTime += MUTimeFromString(tmpVal);
            else
              tmpTime -= MUTimeFromString(tmpVal);

            /* time delta now calculated - set new value */

            OIndex = mSet;

            break;

          case mSet:
          default:

            switch (AIndex)
              {
              case mraDuration:

                tmpTime = MUTimeFromString(tmpVal);

                break;

              case mraEndTime:
              case mraStartTime:
              default:

#ifdef MNOT
                if (strchr(tmpVal,'_') == NULL)
                  {
                  snprintf(EMsg,MMAX_LINE,"ERROR:   please specify an absolute time\n");

                  return(FAILURE);
                  }
#endif /* MNOT */

                if (MUStringToE(tmpVal,(long *)&tmpTime,TRUE) == FAILURE)
                  {
                  snprintf(EMsg,MMAX_LINE,"NOTE:  invalid time format");

                  return(FAILURE);
                  }

                if (strpbrk(tmpVal, "+-_/") == NULL)
                  {
                  /* If the starttime is ambiguous (before now), add 24 hours.
                     If the starttime is supposed to be before the current time 
                     today, the date can be put in to force it.  This is to 
                     get rid of ambiguity.*/

                  if ((tmpTime < MSched.Time) && 
                      (AIndex == mraStartTime))
                    {
                    tmpTime += (MCONST_DAYLEN); 
                    }

                  /* If endtime is ambiguous, move until the first available 
                     after the starttime and currenttime. */

                  while (((tmpTime < MSched.Time) || 
                          (tmpTime < R->StartTime)) && 
                         (AIndex == mraEndTime))
                    {
                    tmpTime += (MCONST_DAYLEN);
                    }
                  }

                break;
              }  /* END switch (AIndex) */

            break;
          }     /* END switch (OIndex) */

        MRsvAdjustTimeframe(R);

        if (strstr(FlagLine,"force") == NULL)
          {
          /* apply policies - do not force change */

          /* Dave, please stop asking questions in the code... */
          /* Doug, do we need both MRsvSetTimeframe() and MRsvConfigure() here? */

          if (MRsvSetTimeframe(R,AIndex,mSet,tmpTime,EMsg) == FAILURE)
            {
            return(FAILURE);
            }

          if (MRsvConfigure(NULL,R,-1,-1,tmpLine,NULL,NULL,TRUE) == FAILURE)
            {
            snprintf(EMsg,MMAX_LINE,"NOTE:  could not change %s rsv %s - %s\n",
              MRsvAttr[AIndex],
              R->Name,
              tmpLine);

            /* restore timeframe to original value */

            R->StartTime = StartTime;
            R->EndTime   = EndTime;

            return(FAILURE);
            }

          MOReportEvent((void *)R,R->Name,mxoRsv,mttCreate,R->CTime,TRUE);
          MOReportEvent((void *)R,R->Name,mxoRsv,mttStart,R->StartTime,TRUE);
          MOReportEvent((void *)R,R->Name,mxoRsv,mttEnd,R->EndTime,TRUE);

          /* call MRsvAllocate() to update timeframe in node RE table */

          MRsvAdjustTimeframe(R);

          snprintf(EMsg,MMAX_LINE,"%s for rsv '%s' changed\n",
            MRsvAttr[AIndex],
            R->Name);
          }    /* END if (strstr(FlagLine,"force") == NULL) */
        else
          {
          /* tmpTime is absolute */

          if (MRsvSetAttr(R,AIndex,(void *)&tmpTime,mdfLong,mSet) == FAILURE)
            {
            snprintf(EMsg,MMAX_LINE,"ERROR:  could not change %s for rsv %s\n",
              MRsvAttr[AIndex],
              R->Name);

            return(FAILURE);
            }

          MOReportEvent((void *)R,R->Name,mxoRsv,mttCreate,R->CTime,TRUE);
          MOReportEvent((void *)R,R->Name,mxoRsv,mttStart,R->StartTime,TRUE);
          MOReportEvent((void *)R,R->Name,mxoRsv,mttEnd,R->EndTime,TRUE);

          MRsvAdjustTimeframe(R);

          snprintf(EMsg,MMAX_LINE,"successfully changed %s for rsv %s\n",
            MRsvAttr[AIndex],
            R->Name);
          }
        }    /* END BLOCK (case mraStartTime) */

        break;

      case mraFlags:

        /* FORMAT:  {+=|=|-=}<FLAG> */

        if ((rc |= MRsvSetAttr(R,mraFlags,(void *)tmpVal,mdfString,OIndex)) == FAILURE)
          {
          snprintf(EMsg,MMAX_LINE,"ERROR:  could not change flags for rsv %s\n",
            R->Name);
          }
        else
          {
          snprintf(EMsg,MMAX_LINE,"successfully changed flags for rsv %s\n",
            R->Name);
          }

        break;

      case mraHostExp:

        {
        int HostCount;
        int TC;
        int NC;
        int nindex;
        mbool_t RESelectedNodes = FALSE;

        marray_t NList;
        mnode_t *N;

        char  Buffer[MMAX_BUFFER];

        /* FORMAT:  {+=|-=}<host>[,<host>]... */

        switch (OIndex)
          {
          case mAdd:

            Mode = 1;

            break;

          case mDecr:

            Mode = -1;

            break;

          case mSet:
          case mSetOnEmpty:

            /* Right now the '=' is using mSetOnEmpty */

            if (strstr(FlagLine,"force") == NULL)
              {
              snprintf(EMsg,MMAX_LINE,"ERROR:  operation on rsv %s not supported (must use --flags=force)\n",
                R->Name);

              return(FAILURE);
              }

            Mode = 0;

            break;

          default:

            snprintf(EMsg,MMAX_LINE,"ERROR:  operation on rsv %s not supported\n",
              R->Name);

            return(FAILURE);

            /* NOTREACHED */

            break;
          }  /* END switch (OIndex) */

        MUSNInit(&MPtr,&MSpace,EMsg,MMAX_LINE);

        HostCount = 0;

        /* NOTE:  convert tmpVal into node expression (NYI) */

        ptr = MUStrTok(tmpVal,"= \t\n",&TokPtr);

        MUArrayListCreate(&NList,sizeof(mnode_t *),-1);

        while (ptr != NULL)
          {
          MUArrayListClear(&NList);

          Buffer[0] = '\0';

          if ((ptr[0] == '\0') || (MUREToList(
                ptr,
                mxoNode,
                NULL,                /* partition constraint */
                &NList,               /* O */
                FALSE,
                Buffer,
                sizeof(Buffer)) == FAILURE) || (NList.NumItems == 0))
            {
            MDB(2,fCONFIG) MLog("ALERT:    cannot expand hostlist '%s'\n",
              ptr);

            MUSNPrintF(&MPtr,&MSpace,"ALERT:  expression '%s' does not match any nodes\n",
              ptr);

            ptr = MUStrTok(NULL,"= \t\n",&TokPtr);

            continue;
            }

          MUSNPrintF(&MPtr,&MSpace,"INFO:   expression '%s' successfully matched %d node(s)\n", 
            ptr,
            NList.NumItems);
          
          RESelectedNodes = TRUE;

          /* only clear the node list when we do a set and we find a RE that matches some nodes */
          /* we do this in case none of the RE's match so we keep our original node list */

          if (HostCount == 0 && Mode == 0)
            {
            /* We've found a match so first clear the current NL */
            MRsvClearNL(R);
            }

          for (nindex = 0;nindex < NList.NumItems;nindex++)
            {
            N = (mnode_t *)MUArrayListGetPtr(&NList,nindex);

            /* By default only add one task when using a rsv modify */

            if (MRsvAdjustNode(R,N,((Mode == 1) ? 1 : -1),Mode,TRUE) == FAILURE)
              {
              MUSNPrintF(&MPtr,&MSpace,"ERROR:  cannot locate node %s in rsv %s\n",
                N->Name,
                R->Name);

              MDB(3,fUI) MLog("INFO:     cannot remove node '%s' from rsv %s\n",
                ptr,
                R->Name);

              ptr = MUStrTok(NULL,", \t\n",&TokPtr);

              continue;
              }

            if (Mode == -1)
              {
              /* remove node from R->NL,R->ReqNL,R->HostExp */

              MNLRemove(&R->NL,N);
              MNLRemove(&R->ReqNL,N);
              }
            else if ((Mode == 1) || (Mode == 0 ))
              {
              if (R->HostExp == NULL)
                {
                MUSNPrintF(&MPtr,&MSpace,"ERROR:  cannot modify hostexp for TASKCOUNT based rsv\n");

                return(FAILURE);
                }

              /* add node to R->NL,R->ReqNL,R->HostExp */

              MNLAdd(&R->NL,N,1);
              MNLAdd(&R->ReqNL,N,1);
              }

            HostCount++;
            }  /* END for (nindex) */

          ptr = MUStrTok(NULL,", \t\n",&TokPtr);
          }    /* END while (ptr != NULL) */

        if (RESelectedNodes == FALSE) 
          {
          MUSNPrintF(&MPtr,&MSpace,"ERROR:  could not match any nodes\n");

          return(FAILURE);
          }

        /* change the reqtc,alloctc,reqnc,allocnc */

        TC = MNLSumTC(&R->NL);
        NC = MNLCount(&R->NL);

        R->AllocTC = TC;
        R->ReqTC   = TC;
        R->AllocNC = NC;
        R->ReqNC   = NC;

        if ((OIndex == mAdd) || (OIndex == mDecr))
          {
          MUSNPrintF(&MPtr,&MSpace,"NOTE:  %d host%s %s rsv %s\n",
            HostCount,
            (HostCount != 1) ? "s" : "",
            (Mode == -1) ? "removed from" : "added to",
            R->Name);
          }
        else
          {
          MUSNPrintF(&MPtr,&MSpace,"NOTE:  hostlist changed for rsv %s\n",
            R->Name);
          }

        /* create a completely new HostExp */

        mstring_t  NewNL(MMAX_LINE);

        MNLToRegExString(&R->NL,&NewNL);

        rc |= MRsvSetAttr(R,mraHostExp,NewNL.c_str(),mdfString,mSet);

        MHistoryAddEvent(R,mxoRsv);

        MUArrayListFree(&NList);
        }  /* END BLOCK (case mraHostExp) */

        break;

      case mraLogLevel:

        rc |= MRsvSetAttr(R,mraLogLevel,(void **)tmpVal,mdfString,mSet);

        MUSNInit(&MPtr,&MSpace,EMsg,MMAX_LINE);

        MUSNPrintF(&MPtr,&MSpace,"NOTE:  loglevel set to %s for rsv %s\n",
          tmpVal,
          R->Name);

        break;

      case mraReqTaskCount:

        {
        int NewTC;
        int OldTC;
        int TC;

        mnl_t *NL;

        TC = (int)strtol(tmpVal,NULL,10);

        if (TC == 0) 
          {
          return(FAILURE);
          }

        OldTC = R->ReqTC;

        switch (OIndex)
          {
          case mAdd:

            NewTC = OldTC + TC;

            break;

          case mDecr:

            NewTC = OldTC - TC;

            break;

          case mSet:
          default:

            NewTC = TC;

            break;
          }  /* END switch (OIndex) */

        if ((NewTC == OldTC) && (ReqDefChanged == FALSE))
          {
          /* no change */

          break;
          }

        if (NewTC <= 0)
          {
          /* illegal request */

          snprintf(EMsg,MMAX_LINE,"NOTE:  illegal task count specified for rsv %s\n",
            R->Name);

          return(FAILURE);
          }

        if ((MNLIsEmpty(&R->NL)) &&
            ((R->AllocNC > 0) ||
            ((R->HostExp != NULL) && (strstr(R->HostExp,"GLOBAL") != NULL))))
          {
          MRsvCreateNL(
            &R->NL,
            R->HostExp,
            0,
            0,
            NULL,
            &R->DRes,
            NULL);

          if (MNLIsEmpty(&R->NL))
            {
            snprintf(EMsg,MMAX_LINE,"NOTE:  cannot create nodelist for %s\n",
              R->Name);

            return(FAILURE);
            }
          }

        NL = &R->NL;

        if (NewTC < OldTC)
          {
          mnode_t *tmpN;

          int nindex;
          int NC;
          int tmpI;

          /* release resources */

          TC = OldTC - NewTC;

          NC = MNLCount(NL);

          /* assume nodes allocated in highest priority first order */
          /* release nodes in last node first order */

          for (nindex = NC - 1;nindex >= 0;nindex--)
            {
            if (MNLGetNodeAtIndex(NL,nindex,&tmpN) == FAILURE)
              {
              break;
              }

            /* walk list */

            tmpI = MIN(MNLGetTCAtIndex(NL,nindex),TC);

            /* adjust node and rsv alloc info */

            /* NOTE:  We'll adjust the rsv stats ourselves after this returns. */

            MRsvAdjustNode(R,tmpN,tmpI,-1,FALSE);

            /* adjust the resources stats on the reservation */

            R->AllocTC -= tmpI;

            /* if the reservation covers all procs on the node, decrement by
             * the node's configured proc count. otherwise decrement by the
             * number of procs per task used by the reservation multiplied by
             * our task count increment */

            if (R->DRes.Procs == -1)
              R->AllocPC -= tmpN->CRes.Procs;
            else
              R->AllocPC -= R->DRes.Procs * tmpI;

            /* adjust rsv nodelist */

            if (tmpI == MNLGetTCAtIndex(NL,nindex))
              {
              MNLTerminateAtIndex(NL,nindex);

              R->AllocNC -= 1; /* only need to adjust if we're removing the node */
              }
            else
              MNLAddTCAtIndex(NL,nindex,-1 * tmpI);

            TC -= tmpI;

            if (TC <= 0)
              break;
            }  /* END for (nindex) */

          MRsvSetAttr(R,mraReqTaskCount,(void **)&NewTC,mdfInt,mSet);

          snprintf(EMsg,MMAX_LINE,"reservation %s taskcount adjusted from %d to %d\n",
            R->Name,
            OldTC,
            NewTC);

          /* does rsv hostexpression need to be adjusted? (NYI) */
          }
        else 
          {
          char tmpLine[MMAX_LINE];

          /* allocate resources */

          MRsvSetAttr(R,mraReqTaskCount,(void **)&NewTC,mdfInt,mSet);

          if ((rc |= MRsvConfigure(NULL,R,-1,-1,tmpLine,NULL,NULL,TRUE)) == FAILURE)
            {
            snprintf(EMsg,MMAX_LINE,"NOTE:  cannot allocate requested resources for rsv %s - %s\n",
              R->Name,
              tmpLine);

            MRsvSetAttr(R,mraReqTaskCount,(void **)&OldTC,mdfInt,mSet);

            return(FAILURE);
            }
          else
            {
            snprintf(EMsg,MMAX_LINE,"reservation %s taskcount adjusted from %d to %d\n",
              R->Name,
              OldTC,
              NewTC);
            }
          }
        }  /* END (case mraReqTaskCount) */

        break;

      case mraResources:

        {
        mnode_t *tmpN;

        int gindex;
        int nindex;

        int TC = MAX(R->ReqTC,R->AllocTC);

        mcres_t tmpRes;

        /* only allow modifying down resources, not modifying TID or modifying
           up resources */

        MCResInit(&tmpRes);

        if (MCResFromString(&tmpRes,tmpVal) == FAILURE)
          {
          MCResFree(&tmpRes);

          return(FAILURE);
          }

        if ((tmpRes.Procs > R->DRes.Procs * TC) ||
            (tmpRes.Mem > R->DRes.Mem * TC) ||
            (tmpRes.Swap > R->DRes.Swap * TC) ||
            (tmpRes.Disk > R->DRes.Disk * TC) ||
            (MSNLGetIndexCount(&tmpRes.GenericRes,0) > MSNLGetIndexCount(&R->DRes.GenericRes,0) * TC))
          {
          snprintf(EMsg,MMAX_LINE,"reservation cannot increase resources\n");

          MCResFree(&tmpRes);

          return(FAILURE);
          }
          
        for (gindex = 1;MGRes.Name[gindex][0] != '\0';gindex++)
          {
          if (MSNLGetIndexCount(&tmpRes.GenericRes,gindex) > (MSNLGetIndexCount(&R->DRes.GenericRes,gindex) * TC))
            {
            snprintf(EMsg,MMAX_LINE,"reservation cannot increase resources\n");

            MCResFree(&tmpRes);

            return(FAILURE);
            }
          }  /* END for (gindex) */

        /* this is only used for DISA right now.  in DISA when they modify the rsv
           they are modifying it down and resizing the reservation to 1 task.  Not
           only that but DISA reservations are only on 1 node so the loop below
           might not work for multi-node reservations, or at least it might not work
           as expected */

        /* MRsvAdjustNode() is broken when you reducing the size and taskcount on
           the same node */

        MCResCopy(&R->DRes,&tmpRes);

        R->ReqTC = 1;

        R->AllocPC = 0;
        R->AllocNC = 0;
        R->AllocTC = 0;

        /* MRsvAdjustNode() and MNodeBuildRE() do not function perfectly when
           modifying a reservation.  The best solution is to clear out the
           reservation and add it back in (expensive) */

        MRsvDeallocateResources(R,&R->NL);

        for (nindex = 0;MNLGetNodeAtIndex(&R->NL,nindex,&tmpN) == SUCCESS;nindex++)
          {
          /* adding 1 task per node and summing the resources here because
             MRsvAdjustNode() doesn't do it right */

          MNLSetTCAtIndex(&R->NL,nindex,1);
          MNLSetTCAtIndex(&R->ReqNL,nindex,1);

          rc |= MRsvAdjustNode(R,tmpN,MNLGetTCAtIndex(&R->NL,nindex),0,FALSE);

          R->AllocPC += R->DRes.Procs;
          R->AllocNC++;
          R->AllocTC++;
          }

        MHistoryAddEvent(R,mxoRsv);

        MCResFree(&tmpRes);
        }  /* END mraResources */

        break;

      case mraComment:
      case mraLabel:
      case mraRsvGroup:

        rc |= MRsvSetAttr(R,AIndex,(void **)tmpVal,mdfString,mSet);

        snprintf(EMsg,MMAX_LINE,"reservation %s attribute %s changed to '%s'\n",
          R->Name,
          MRsvAttr[AIndex],
          tmpVal);

        break;

      case mraTrigger:

        {
        /* currently ignore op */

        snprintf(EMsg,MMAX_LINE,"NOTE:  operation on rsv %s not supported\n",
          R->Name);

        return(FAILURE);
        }  /* END BLOCK (case mraTrigger) */

      case mraVariables:

        /* FORMAT:  ??? */

        /* NOTE:  add support for incr/decr (NYI) */
      
        /* NOTE: mDecr, mAdd, mSet, and mSetOnEmpty are all handled
         * in MRsvSetAttr */

        /* if R->Variables is not really empty, we don't want mSetOnEmpty */

        if ((OIndex == mSetOnEmpty) && (R->Variables != NULL))
          {
          OIndex = mSet;
          }

        rc |= MRsvSetAttr(R,AIndex,(void **)tmpVal,mdfString,OIndex);

        snprintf(EMsg,MMAX_LINE,"reservation '%s' attribute '%s' changed.\n",
          R->Name,
          MRsvAttr[AIndex]);

        break;

      default:

        sprintf(tmpLine,"NOTE:  operation on rsv %s not supported\n",
          R->Name);

        return(FAILURE);

        /*NOTREACHED*/

        break;
      } /* END switch (AIndex) */
    } /* END while (MXMLGetChild(RE,NULL,&STok,&SE) == SUCCESS) */

  /* check rc to see if we need to transition the reservation */

  if (rc == SUCCESS)
    {
    MRsvTransition(R,FALSE);
    }

  if (bmisset(&R->Flags,mrfSystemJob))
    MRsvSyncSystemJob(R);

  /* check for modify trigger */
  if (R->T != NULL)
    {
    MOReportEvent(
      (void *)R,
      R->Name,
      mxoRsv,
      mttModify,
      MSched.Time,
      TRUE);
    }

  /* record modify event for reservation */
  if (bmisset(&MSched.RecordEventList,mrelRsvModify))
    {
    char tmpMsg[MMAX_LINE];

    snprintf(tmpMsg,sizeof(tmpMsg),"%s changed. %s",
      MRsvAttr[AIndex],
      (EMsg != NULL) ? EMsg : "-");

    MOWriteEvent((void *)R,mxoRsv,mrelRsvModify,tmpMsg,MStat.eventfp,NULL);
    }

  return(SUCCESS);
  }      /* END MUIRsvCtlModify() */
/* END MUIRsvCtlModify.c */
