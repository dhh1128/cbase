/* HEADER */

/**
 * @file MUIVPCModify.c
 *
 * Contains MUI VPC Modify function
 */



#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 * Modify a vpc based on command from user.
 *
 */

int MUIVPCModify(

  char                 *Auth,   /* I */
  mxml_t               *RE,     /* I */
  char                 *OID,    /* I */
  enum MXMLOTypeEnum    OType,  /* I */
  mstring_t            *String) /* O */
 
  {
  mrsv_t  *MasterR;

  marray_t RList;

  mpar_t *C;  /* VPC */

  int     STok;

  mxml_t *SE;
  mxml_t *WE;

  char    tmpName[MMAX_NAME];
  char    tmpVal[MMAX_BUFFER];
  char    tmpOp[MMAX_NAME];

  mbool_t IsAdmin      = FALSE;

  mpsi_t   *P;
  mgcred_t *U;

  enum MParAttrEnum AIndex;

  enum MObjectSetModeEnum OIndex;

  int  rc;

  if (!strncasecmp(Auth,"peer:",strlen("peer:")))
    {
    if (MPeerFind(Auth,FALSE,&P,FALSE) == FAILURE)
      {
      /* NYI */
      }      

    U = NULL;
    }
  else
    {
    MUserAdd(Auth,&U);
    P = NULL;
    }

  if (MS3GetWhere(
        RE,
        &WE,
        NULL,
        tmpName,         /* NOTE:  ignore name, use only value */
        sizeof(tmpName),
        tmpName,
        sizeof(tmpName)) == FAILURE)
    {
    return(FAILURE);
    }

  if (MVPCFind(tmpName,&C,FALSE) == FAILURE)
    {
    MStringAppendF(String,"cannot locate vpc '%s'\n",
      tmpName);

    return(FAILURE);
    }

  if (MUICheckAuthorization(
        U,
        P,
        C,
        mxoPar,
        mcsMSchedCtl,
        msctlModify,
        &IsAdmin,
        NULL,
        0) == FAILURE)
    {
    MStringAppendF(String,"ERROR:    user '%s' is not authorized to run command\n",
      Auth);

    return(FAILURE);
    }

  STok = -1;

  MUArrayListCreate(&RList,sizeof(mrsv_t *),10);

  MRsvGroupGetList(C->RsvGroup,&RList,NULL,0);

  MasterR = (mrsv_t *)MUArrayListGetPtr(&RList,0);

  while (MS3GetSet(
      RE,
      &SE,
      &STok,
      tmpName,
      sizeof(tmpName),
      tmpVal,
      sizeof(tmpVal)) == SUCCESS)
    {
    rc = MBNOTSET;

    AIndex = (enum MParAttrEnum)MUGetIndexCI(tmpName,MParAttr,FALSE,0);

    MXMLGetAttr(SE,MSAN[msanOp],NULL,tmpOp,sizeof(tmpOp)); 

    /* FORMAT:  <X>=<Y> */

    OIndex = (enum MObjectSetModeEnum)MUGetIndexCI(
      tmpOp,
      MObjOpType,
      FALSE,
      mNONE2);

    switch (AIndex)
      {
      case mpaACL:

        if (MParSetAttr(C,mpaACL,(void **)tmpVal,mdfString,OIndex) == FAILURE)
          {
          /* NYI */
          }
        else
          {
          mln_t  *tmpL;

          if (RList.NumItems > 0)
            {
            MULLAdd(&MasterR->Variables,"LASTRESULT",NULL,&tmpL,MUFREE);

            MUStrDup((char **)&tmpL->Ptr,tmpVal);

            MULLAdd(&MasterR->Variables,"LASTOP",NULL,&tmpL,MUFREE);

            if ((OIndex == mAdd) || (OIndex == mIncr))
              MUStrDup((char **)&tmpL->Ptr,"ADD");
            else if (OIndex == mDecr)
              MUStrDup((char **)&tmpL->Ptr,"REMOVE");
            else
              MUStrDup((char **)&tmpL->Ptr,"SET");

            MULLAdd(&MasterR->Variables,"LASTATTR",NULL,&tmpL,MUFREE);

            MUStrDup((char **)&tmpL->Ptr,"ACL");

            MOReportEvent((void *)MasterR,MasterR->Name,mxoRsv,mttModify,MSched.Time,TRUE);
            }
          }    /* END else (MParSetAttr() == FAILURE) */

        break;

      case mpaDuration:

        {
        char   Msg[MMAX_LINE];
        char   FlagLine[MMAX_LINE];

        mbitmap_t FlagBM;

        MXMLGetAttr(RE,MSAN[msanFlags],NULL,FlagLine,sizeof(FlagLine));

        bmfromstring(FlagLine,MClientMode,&FlagBM);

        if (MVPCModify(
              C,
              mpaDuration,
              tmpVal,
              mdfString,
              OIndex,
              &FlagBM,
              Msg) == FAILURE)  /* O */
          {
          MDB(3,fUI) MLog("INFO:     cannot modify duration for vpc %s '%s'\n",
            C->Name,
            Msg);

          MStringAppendF(String,"cannot modify duration for vpc %s '%s'",
            C->Name,
            Msg);
          }
        else
          {
          char TString[MMAX_LINE];

          MVPCUpdateCharge(C,NULL);

          MULToTString(C->ReqDuration,TString);
  
          MStringAppendF(String,"walltime set to %s",
            TString);
          }
        }    /* END BLOCK (case mpaDuration) */

        break;

      case mpaMessages:

        /* FORMAT:  message='<MESSAGE TEXT>' */

        {
        char *ptr;
        char *TokPtr;

        ptr = MUStrTokE(tmpVal,"\n",&TokPtr);

        MMBAdd(
          &C->MB,
          ptr,
          (Auth != NULL) ? Auth : (char *)"N/A",
          mmbtOther,
          MSched.Time + MCONST_DAYLEN * 100,
          10,
          NULL);
        }  /* END BLOCK (case mpaMessages) */

        break;

      case mpaProvStartTime:

        {
        char Msg[MMAX_LINE];

        MVPCModify(C,mpaProvStartTime,tmpVal,mdfString,OIndex,0,Msg);

        MStringAppend(String,Msg);
        }

        break;

      case mpaReqResources:

        /* change resources on the reservation */

        {
        int NewTC;
        int OldTC;
        int TC;

        mbool_t ReqDefChanged = FALSE;  /* task resource definition has changed */

        mnl_t *NL;

        if (isdigit(tmpVal[0]))
          {
          char *tail;

          /* FORMAT:  <TC>[@<RESTYPE>:<COUNT>[+<RESTYPE>:<COUNT>]...] where <RESTYPE> is one of *procs*, *mem*, *disk*, or *swap*  */

          /* change VPC taskcount */

          TC = (int)strtol(tmpVal,&tail,10);

          if (TC == 0) 
            {
            rc = FAILURE;

            break;
            }

          OldTC = MasterR->ReqTC;

          if (OldTC == 0)
            {
            OldTC = MNLSumTC(&MasterR->NL);
            }

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

          if ((tail != NULL) && (*tail == '@'))
            {
            mrsv_t *tmpR;

            char *ptr;
            char *TokPtr;

            char *RName;
            char *RCount;
            char *TokPtr2;
     
            int   rindex;
            int   rlindex;

            /* modify R task description */

            ReqDefChanged = TRUE;

            ptr = MUStrTok(tail + 1,"+",&TokPtr);

            while (ptr != NULL)
              {
              RName  = MUStrTok(ptr,":",&TokPtr2);
              RCount = MUStrTok(NULL,":",&TokPtr2);

              if ((RName == NULL) || (RCount == NULL))
                {
                ptr = MUStrTok(NULL,"+",&TokPtr);
    
                continue;
                }

              rindex = MUGetIndexCI(RName,MResourceType,TRUE,0);

              for (rlindex = 0;rlindex < RList.NumItems;rlindex++)
                {
                tmpR = (mrsv_t *)MUArrayListGetPtr(&RList,rlindex);

                switch (rindex)
                  {
                  case mrProc:

                    tmpR->DRes.Procs = (int)strtol(RCount,NULL,10);

                    break;

                  case mrMem:
                  case mrDisk:
                  case mrSwap:

                    /* NYI */

                    break;

                  default:

                    /* NO-OP */

                    break;
                  }  /* END switch (rindex) */

                MRsvAllocate(tmpR,&tmpR->NL);
                }    /* END for (rlindex) */

              ptr = MUStrTok(NULL,"+",&TokPtr);
              }      /* END while (ptr != NULL) */
            }        /* END if ((tail != NULL) && (*tail = '@')) */

          if ((NewTC == OldTC) && (ReqDefChanged == FALSE))
            {
            /* no change */

            break;
            }

          if (NewTC <= 0)
            {
            /* illegal request */

            MStringAppendF(String,"NOTE:  illegal resources specified for vpc %s\n",
              C->Name);

            rc = FAILURE;

            break;
            }

          NL = &MasterR->NL;

          if (NewTC < OldTC)
            {
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
              /* walk list */

              tmpI = MIN(MNLGetTCAtIndex(NL,nindex),TC);
 
              /* adjust node and rsv alloc info */

              MRsvAdjustNode(MasterR,MNLReturnNodeAtIndex(NL,nindex),tmpI,-1,TRUE);

              /* adjust rsv nodelist */

              if (tmpI == MNLGetTCAtIndex(NL,nindex))
                MNLTerminateAtIndex(NL,nindex);
              else
                MNLAddTCAtIndex(NL,nindex,-1 * tmpI);

              TC -= tmpI;

              if (TC <= 0)
                break;
              }  /* END for (nindex) */

            MRsvSetAttr(MasterR,mraReqTaskCount,(void **)&NewTC,mdfInt,mSet);

            MVPCUpdateCharge(C,NULL);

            MRsvSyncSystemJob(MasterR);

            MStringAppendF(String,"reservation %s taskcount decreased from %d to %d\n",
              MasterR->Name,
              OldTC,
              NewTC);

            /* does rsv hostexpression need to be adjusted? (NYI) */
            }  /* END if (NewTC < OldTC) */
          else 
            {
            mrsv_t *tmpR;
            mln_t  *tmpL;

            int     rindex;

            char tmpLine[MMAX_LINE];
            char tmpBuf[MMAX_BUFFER];

            mnl_t DiffNL;
            mnl_t OldNL;

            /* allocate resources */

            MNLInit(&OldNL);

            MNLCopy(&OldNL,&MasterR->NL);

            MRsvSetAttr(MasterR,mraReqTaskCount,(void **)&NewTC,mdfInt,mSet);

            if (MRsvConfigure(NULL,MasterR,-1,-1,tmpLine,NULL,NULL,TRUE) == FAILURE)
              {
              for (rindex = 1;rindex < RList.NumItems;rindex++)
                {
                tmpR = (mrsv_t *)MUArrayListGetPtr(&RList,rindex);

                if (tmpR->RsvGroup[0] == '\0')
                  {
                  strcpy(tmpR->RsvGroup,C->RsvGroup);
                  }
                }

              MStringAppendF(String,"NOTE:  cannot allocate requested resources for vpc rsv %s - %s\n",
                MasterR->Name,
                tmpLine);

              MRsvSetAttr(MasterR,mraReqTaskCount,(void **)&OldTC,mdfInt,mSet);

              rc = FAILURE;

              break;
              }  /* END if (MRsvConfigure() == FAILURE) */

            MVPCUpdateCharge(C,NULL);

            if (ReqDefChanged == TRUE)
              {
              MStringAppendF(String,"reservation %s task definitions changed\n",
                MasterR->Name);
              }
            else
              {
              MStringAppendF(String,"reservation %s taskcount increased from %d to %d\n",
                MasterR->Name,
                OldTC,
                NewTC);
              }

            /* rebuild HostExp */

            if (MNLToString(
                 &MasterR->NL,
                 FALSE,
                 NULL,
                 '\0',
                 tmpBuf,
                 sizeof(tmpBuf)) == SUCCESS)
              {
              MUStrDup(&MasterR->HostExp,tmpBuf);
              }

            MNLInit(&DiffNL);

            MNLDiff(&MasterR->NL,&OldNL,&DiffNL,NULL,NULL);

            MNLToString(&DiffNL,FALSE,",",'\0',tmpBuf,sizeof(tmpBuf));

            MNLFree(&OldNL);
            MNLFree(&DiffNL);

            MULLAdd(&MasterR->Variables,"LASTRESULT",NULL,&tmpL,MUFREE);

            MUStrDup((char **)&tmpL->Ptr,tmpBuf);

            MULLAdd(&MasterR->Variables,"LASTOP",NULL,&tmpL,MUFREE);

            MUStrDup((char **)&tmpL->Ptr,"ADD");

            MULLAdd(&MasterR->Variables,"LASTATTR",NULL,&tmpL,MUFREE);

            MUStrDup((char **)&tmpL->Ptr,"NODES");

            MOReportEvent((void *)MasterR,MasterR->Name,mxoRsv,mttModify,MSched.Time,TRUE);

            MRsvSyncSystemJob(MasterR);
            }    /* END else (NewTC < OldTC) */
          }      /* END  if (isdigit(tmpVal[0])) */
        else
          {
          char *ptr;
          char *TokPtr;

          /* modify host exp */

          char tmpName[MMAX_NAME];

          int   Mode = 0;

          mnode_t *N;

          /* FORMAT:  {+=|-=}<host>[,<host>]... */
  
          switch (OIndex)
            {
            case mAdd:
  
              Mode = 1;
      
              break;

            case mDecr:
  
              Mode = -1;
  
              break;
  
            default:

              MStringAppendF(String,"NOTE:  operation on rsv %s not supported\n",
                (C->RsvGroup != NULL) ? C->RsvGroup : "NULL");

              rc = SUCCESS;
  
              /* NOTREACHED */

              break;
            }  /* END switch (OIndex) */

          ptr = MUStrTok(tmpVal,"=, \t\n",&TokPtr);

          while (ptr != NULL)
            {
            if (MNodeFind(ptr,&N) == SUCCESS)
              {
              if (MRsvAdjustNode(MasterR,N,-1,Mode,TRUE) == FAILURE)
                {
                MDB(3,fUI) MLog("INFO:     cannot remove node '%s' from rsv %s\n",
                  ptr,
                  MasterR->Name);
                }
  
              if (Mode == -1)
                {
                sprintf(tmpName,"^%s$",
                  N->Name);

                /* remove node from MasterR->NL,MasterR->ReqNL,MasterR->HostExp */

                MNLRemove(&MasterR->NL,N);
                MNLRemove(&MasterR->ReqNL,N);

                if (MUStrRemoveFromList(MasterR->HostExp,tmpName,'|',FALSE) == FAILURE)
                  {
                  /* try it without the '^' and '$' */
  
                  sprintf(tmpName,"%s",
                    N->Name);
  
                  MUStrRemoveFromList(MasterR->HostExp,tmpName,',',FALSE);
                  }

                if (MasterR->ReqTC > 0)
                  MasterR->ReqTC -= N->CRes.Procs;

                if (MasterR->ReqNC > 0)
                  MasterR->ReqNC -= 1;
                }
              else if (Mode == 1)
                {
                if (strchr(MasterR->HostExp,'|') != NULL)
                  {
                  sprintf(tmpName,"^%s$",
                    N->Name);

                  MUStrAppend(&MasterR->HostExp,NULL,tmpName,'|');
                  }
                else
                  {
                  sprintf(tmpName,"%s",
                    N->Name);

                  MUStrAppend(&MasterR->HostExp,NULL,tmpName,',');
                  }
                /* add node to MasterR->NL,MasterR->ReqNL,MasterR->HostExp */

                MNLAdd(&MasterR->NL,N,0);
                MNLAdd(&MasterR->ReqNL,N,0);
                }
              else
                {
                /* set MasterR->HostExp */
  
                /* NYI */
                }
              }
            else
              {
              MDB(3,fUI) MLog("INFO:     cannot find node '%s' for rsv %s\n",
                ptr,
                MasterR->Name);
              }

            ptr = MUStrTok(NULL,", \t\n",&TokPtr);
            }  /* END while (ptr != NULL) */

          MVPCUpdateCharge(C,NULL);

          MRsvSyncSystemJob(MasterR);

          MStringAppendF(String,"NOTE:  hosts %s rsv %s\n",
            (Mode == -1) ? "removed from" : "added to",
            MasterR->Name);
          }  /* END else (isdigit(tmpVal[0])) */
        }    /* END BLOCK (case mpaReqResources) */

        break;

      case mpaVariables:

        {
        mrsv_t *R;

        int rindex;

        /* adjust vpc variables -> rsvgroup master variables */

        for (rindex = 0;rindex < RList.NumItems;rindex++)
          {
          R = (mrsv_t *)MUArrayListGetPtr(&RList,rindex);

          if (strcmp(C->RsvGroup,R->Name))
            continue;

          /* NOTE:  add/modify only specified variable values */

          MULLUpdateFromString(&R->Variables,tmpVal,NULL); 

          break;
          }  /* END for (rindex) */
        }

        break;

      case mpaRsvGroup:

        {
        char    Msg[MMAX_LINE];

        mrsv_t *R;

        int rindex;
        int TID;

        marray_t LocalRList;

        mtrans_t *Trans = NULL;

        MUArrayListCreate(&LocalRList,sizeof(mrsv_t *),10);

        /* only used to add a TID to an existing VPC */

        if (MVPCAddTID(
              C,
              NULL,
              Auth,
              tmpVal,
              &LocalRList,
              Msg) == FAILURE)
          {
          MUArrayListFree(&LocalRList);

          rc = FAILURE;

          MStringAppend(String,Msg);

          break;
          }

        if (MVPCUpdateReservations(C,&LocalRList,TRUE,Msg) == FAILURE)
          {
          /* cannot charge for rsv allocation */

          /* destroy reservations in LocalRList */

          for (rindex = 0;rindex < LocalRList.NumItems;rindex++)
            {
            R = (mrsv_t *)MUArrayListGetPtr(&LocalRList,rindex);

            MRsvDestroy(&R,TRUE,TRUE);
            }  /* END for (rindex) */
     
          MUArrayListFree(&LocalRList);

          MStringAppend(String,Msg);

          rc = FAILURE;

          break;
          }

        TID = strtol(tmpVal,NULL,10);

        if ((MTransFind(TID,NULL,&Trans) == SUCCESS) &&
            (Trans->RsvID != NULL))
          {
          MRsvDestroyGroup(Trans->RsvID,FALSE);

          MTransInvalidate(Trans);
          }

        for (rindex = 0;rindex < LocalRList.NumItems;rindex++)
          {
          R = (mrsv_t *)MUArrayListGetPtr(&LocalRList,rindex);

          MStringAppendF(String,"NOTE:  reservation %s added to VPC\n",
            R->Name);
          }  /* END for (rindex) */

        MUArrayListFree(&LocalRList);
        }  /* END case mpaRsvGroup */

        break;

      case mpaUser:
      case mpaGroup:
      case mpaAcct:
      case mpaQOS:
      case mpaOwner:

        if (OIndex != mSet)
          {
          MStringAppendF(String,"operation %s not supported",MObjOpType[OIndex]);

          rc = FAILURE;
          }
        else if (MParSetAttr(C,AIndex,(void **)tmpVal,mdfString,mSet) == FAILURE)
          {
          MStringAppendF(String,"could not modify %s to %s",MParAttr[AIndex],tmpVal);

          rc = FAILURE;
          }
        else
          {
          MStringAppendF(String,"vpc %s set to %s",MParAttr[AIndex],tmpVal);
          }

        break;
 
      default:

        /* NOT HANDLED */

        MStringAppend(String,"specified attribute not handled");

        rc = FAILURE;

        /*NOTREACHED*/

        break;
      }  /* END switch (AIndex) */

    if ((rc == SUCCESS) || (rc == FAILURE))
      break;
    }    /* END while (MS3GetSet() == SUCCESS) */

  MUArrayListFree(&RList);

  return(SUCCESS);
  }  /* END MUIVPCModify() */
/* END MUIVPCModify.c */
