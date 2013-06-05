/* HEADER */

      
/**
 * @file MJobXML.c
 *
 * Contains:
 *    Various MJob To/From/Support XML functions
 */


#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  




/**
 * Create xml describing the partition priority from a mtransjob_t
 *  
 * @see __MJobTransitionParPriorityToXML() 
 *  
 * @param J (I)  The job providing the information 
 * @param JE (O) The XML structure
 */

int __MJobParPriorityToXML(

  mjob_t      *J,
  mxml_t      *JE)

  {
  int     pindex;
  mxml_t *ParE;
 
  if (FALSE == MSched.PerPartitionScheduling)
    return(FAILURE);

  for (pindex = 1; pindex < MMAX_PAR; pindex++)
    {
    if (MPar[pindex].Name[0] == '\0')
      break;

    if (MPar[pindex].Name[0] == '\1')
      continue;
 
    if (bmisset(&J->PAL,pindex) == FAILURE)
      continue;

    MXMLCreateE(&ParE,(char *)MXO[mxoPar]);

    MXMLSetAttr(ParE,(char *)MJobAttr[mjaStartPriority],(void *)&J->PStartPriority[pindex],mdfLong);
    MXMLSetAttr(ParE,(char *)MParAttr[mpaID],MPar[pindex].Name,mdfString);

    if ((J->Rsv != NULL) && (J->Rsv->PtIndex == pindex))
      {
      MXMLSetAttr(ParE,(char *)MJobAttr[mjaRsvStartTime],(void *)&J->Rsv->StartTime,mdfLong);
      }

    MXMLAddE(JE,ParE);
    }

    return(SUCCESS);
  } /* END __MJobParPriorityToXML() */

/**
 * Translate a given job (mjob_t) to XML format (mxml_t).
 *
 * @see MJobToExportXML() - peer
 * @see MCJobStoreCP() - parent (w/IsCP == TRUE)
 * @see MJobStoreCP() - parent (w/IsCP == TRUE) 
 *
 * @param J (I)
 * @param JEP (O) - NULL or existing mxml_t object [possibly allocated]
 * @param SJAList (I) [optional]
 * @param SRQAList (I) [optional]
 * @param NullVal (I) [optional]
 * @param IsCP (I)
 * @param IsDisplay (I)
 */

int MJobToXML(

  mjob_t  *J,        /* I */
  mxml_t **JEP,      /* O (possibly allocated) - NULL or existing mxml_t object */
  enum MJobAttrEnum *SJAList,  /* I (optional) */
  enum MReqAttrEnum *SRQAList, /* I (optional) */
  char    *NullVal,  /* I (optional) */
  mbool_t  IsCP,     /* I */
  mbool_t  IsDisplay)/* I */

  {
  const enum MJobAttrEnum DJAList[] = {
    mjaAWDuration,
    mjaEEWDuration,   /* eligible job time */
    mjaHold,
    mjaLastChargeTime,
    mjaQOSReq,
    mjaReqReservation,
    mjaStartCount,
    mjaStatMSUtl,
    mjaStatPSDed,
    mjaStatPSUtl,
    mjaSysPrio,
    mjaSysSMinTime,
    mjaUserPrio,
    mjaVariables,
    mjaNONE };

  int  aindex;

  enum MJobAttrEnum *JAList;

  mxml_t *JE;

  mbool_t ShowMB   = FALSE;
  mbool_t ShowVars = FALSE;

  const char *FName = "MJobToXML";

  if ((J == NULL) || (JEP == NULL))
    {
    return(FAILURE);
    }

  if (*JEP == NULL)
    {
    if (MXMLCreateE(JEP,(char *)MXO[mxoJob]) == FAILURE)
      {
      MDB(3,fCKPT) MLog("INFO:     failed to create xml for job '%s'\n",
        J->Name);

      return(FAILURE);
      }
    }

  JE = *JEP;

  if (SJAList != NULL)
    JAList = SJAList;
  else
    JAList = (enum MJobAttrEnum *)DJAList;

  mstring_t tmpString(MMAX_LINE);

  for (aindex = 0;JAList[aindex] != mjaNONE;aindex++)
    {
    switch (JAList[aindex])
      {
      case mjaAllocVMList:

        if (IsCP == TRUE)
          {
          /* J->VMTaskMap was removed */

          /* MJobAToMString() will print either Variables->VMID or VMTaskMap, but when we
             come up from checkpoint we don't want to put Variables->VMID into VMTaskMap
             so for CP don't call MJobAToMString() */

          continue;
          }

        break;

      case mjaBecameEligible:

        if (J->EligibleTime != 0)
          {
          MXMLSetAttr(JE,(char *)MJobAttr[mjaBecameEligible],(void *)&J->EligibleTime,mdfInt);
          }

        continue;

        /*NOTREACHED*/

        break;

      case mjaDepend:

        {
        enum MRMTypeEnum RType;

        if ((IsCP == TRUE) && 
            !bmisset(&J->IFlags,mjifHasInternalDependencies))
          {
          /* only checkpoint if RM does not support depend type */

          if (J->Depend == NULL)
            continue;

          if (J->DestinationRM != NULL)
            RType = J->DestinationRM->Type;
          else if (J->SubmitRM != NULL)
            RType = J->SubmitRM->Type;
          else
            RType = mrmtNONE;
          
          switch (RType)
            {
            case mrmtPBS:
 
              if ((J->Depend->Type == mdJobSyncMaster) || (J->Depend->Type == mdJobSyncSlave))
                {
                /* fall through to write out dependency checkpoint info */
                }
              else
                {
                /* RM will handle dependency - do not checkpoint */

                continue;
                }

              break;

            case mrmtS3:

              /* Internal resource manager - must checkpoint */

              break;

            default:

              /* by default, assume RM will handle dependency - do not checkpoint */

              continue;

              /*NOTREACHED*/

              break;
            }  /* END switch (RType) */
          }    /* END if (IsCP == TRUE) */
        }      /* END BLOCK (case mjaDepend) */

        break;

      case mjaMessages:

        ShowMB = TRUE;

        continue;
       
        /*NOTREACHED*/

        break;

      case mjaReqReservation:

        if (!bmisset(&J->Flags,mjfAdvRsv) || (J->ReqRID == NULL))
          continue;

        break;

      case mjaReqProcs:

        /* For display purposes use total task count, otherwise don't display (obsolete) */

        if ((IsDisplay == TRUE) && (J->TotalTaskCount > 0))
          {
          MXMLSetAttr(JE,(char *)MJobAttr[mjaReqProcs],(void *)&J->TotalTaskCount,mdfInt);
          }

        if (IsDisplay == TRUE)
          continue;

        /* if not doing display, continue normally.
            J->Request.TC is important for checkpointing and when the job
            is first checked in MS3WorkloadQuery */

        break;

      case mjaReqNodes:

        if ((IsDisplay == TRUE) && (MSched.BluegeneRM == TRUE) && (J->Request.NC == 0))
          {
          MXMLSetAttr(JE,(char *)MJobAttr[mjaReqNodes],(void *)&J->Request.TC,mdfInt);
          }

        continue;

        /*NOTREACHED*/

        break;

      case mjaVMCR:

        {
        if (J->VMCreateRequest != NULL)
          {
          mxml_t *VMCRE = NULL;

          MVMCRToXML(J->VMCreateRequest,&VMCRE);

          MXMLAddE(JE,VMCRE);
          }
        } /* END BLOCK mjaVMCR */

        continue;

        /*NOTREACHED*/

        break;

      case mjaTrigger:

        {
        /* Translate the job's triggers to XML, needed for checkpointing.
            Job triggers are stored as sub-elements in the job's XML */

        mtrig_t *T = NULL;
        mxml_t  *TE = NULL;

        int tindex;

        if (J->Triggers == NULL)
          continue;

        for (tindex = 0; tindex < J->Triggers->NumItems;tindex++)
          {
          T = (mtrig_t *)MUArrayListGetPtr(J->Triggers,tindex);

          /* Can't do MTrigIsReal becaue triggers won't show up
              on job templates */

          if ((T == NULL) ||
              (T->TName == NULL) ||
              (T->IsExtant == FALSE))
            continue;

          TE = NULL;

          if (MXMLCreateE(&TE,(char *)MXO[mxoTrig]) == FAILURE)
            {
            continue;
            }

          MTrigToXML(T,TE,NULL);

          MXMLAddE(JE,TE);
          }
        } /* END BLOCK mjaTrigger */

        continue;

        /*NOTREACHED*/

        break;

      case mjaVariables:

        ShowVars = TRUE;

        continue;
       
        /*NOTREACHED*/

        break;

      default:

        /* NO-OP */

        break;
      }  /* END switch (JAList[aindex]) */

    if ((MJobAToMString(
           J,
           JAList[aindex],
           &tmpString) == FAILURE) ||
        (MUStrIsEmpty(tmpString.c_str())))
      {
      if (NullVal == NULL)
        continue;

      MStringSet(&tmpString,NullVal);
      }

    MXMLSetAttr(JE,(char *)MJobAttr[JAList[aindex]],tmpString.c_str(),mdfString);
    }  /* END for (aindex) */

  if (MSched.PerPartitionScheduling == TRUE)
    {
    __MJobParPriorityToXML(J,JE);
    } 

  if ((SRQAList == NULL) || (SRQAList[0] != mrqaNONE))
    {
    int rqindex;

    mxml_t *RQE = NULL;

    /* process req */

    for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
      {
      mreq_t *RQ = J->Req[rqindex];
      mbool_t CPExtra = FALSE;

      if (RQ == NULL)
        break;

      RQE = NULL;

      if (MXMLCreateE(&RQE,(char *)MXO[mxoReq]) == FAILURE)
        continue;

      MReqToXML(J,RQ,RQE,SRQAList,NullVal);

      if ((IsCP == TRUE) && ((RQ->RMIndex < 0 ) || 
            (!strcasecmp(MRM[RQ->RMIndex].Name,"internal"))))
        {
        /* checkpoint extra information if the RM for the Req is internal */
        CPExtra = TRUE;
        }
      else
        {
        mrm_t *RM = MRM + RQ->RMIndex;

        if (MRMFunc[RM->Type].JobSubmit == NULL)
          {
          /* checkpoint extra information if the RM has no 
           * job submit function */
          CPExtra = TRUE;
          }
        else switch (RM->Type) 
          {
          case mrmtNative:

            if ((RM->ND.URL[mrmaJobSubmitURL] == NULL) ||
                (RM->ND.URL[mrmaJobSubmitURL] == (void *)1))
            /* checkpoint extra information if the native RM has no job 
             * submit URL */
              CPExtra = TRUE;

            break;

          default:

            break;
          }
        }

      if (CPExtra == TRUE)
        {
        enum MReqAttrEnum ReqExtra[] = {
          mrqaGRes,
          mrqaCGRes,
          mrqaNONE };

        MReqToXML(J,RQ,RQE,ReqExtra,NullVal);
        }

      MXMLAddE(JE,RQE);
      }  /* END for (rqindex) */
    }    /* END if ((SRQAList == NULL) || (SRQAList[0] != mrqaNONE)) */

  if (J->DataStage != NULL)
    {
    if (J->DataStage->SIData != NULL)
      {
      /* process data-staging information */
   
      mxml_t *SDE;
      msdata_t *Data;
   
      Data = J->DataStage->SIData;
   
      while (Data != NULL)
        {
        SDE = NULL;
   
        if (MXMLCreateE(&SDE,(char *)"datareq") == FAILURE)
          continue;
   
        MSDToXML(Data,SDE,NULL,NullVal);
   
        MXMLAddE(JE,SDE);
   
        Data = Data->Next;
        }
      }  /* END if (J->SIData != NULL) */

    if (J->DataStage->SOData != NULL)
      {    
      /* process data-staging information */
   
      mxml_t *SDE;
      msdata_t *Data;
   
      Data = J->DataStage->SOData;
   
      while (Data != NULL)
        {
        SDE = NULL;
   
        if (MXMLCreateE(&SDE,(char *)"datareq") == FAILURE)
          continue;
   
        MSDToXML(Data,SDE,NULL,NullVal);
   
        MXMLAddE(JE,SDE);
   
        Data = Data->Next;
        }
      }  /* END if (J->SOData != NULL) */
    }  /* END if (J->DS != NULL) */

  if (J->Array != NULL)
    {
    mxml_t *JA;

    MJobArrayToXML(J,&JA);

    MXMLAddE(JE,JA);
    }

  if (ShowVars == TRUE)
    {
    MUAddVarsToXML(JE,&J->Variables);
    }

  if ((ShowMB == TRUE) && (J->MessageBuffer != NULL))
    {
    mxml_t *ME = NULL;
    char   *ptr;
 
    if (MJobAToMString(J,mjaMessages,&tmpString) == SUCCESS)
      {
      char *mutableString = NULL;

      MUStrDup(&mutableString,tmpString.c_str());

      if (((ptr = strchr(mutableString,'\n')) != NULL) && (IsCP == TRUE))
        {
        /* NOTE:  pack string - replace newlines w/'\7' - sync w/MMBAdd() */
 
        for (;ptr != NULL;ptr = strchr(ptr,'\n'))
          {
          *ptr = '\7';
          }
        }

      if (MXMLFromString(&ME,mutableString,NULL,NULL) == SUCCESS)
        {
        MXMLAddE(JE,ME);
        }

      MUFree(&mutableString);
      }
    else
      {
      MDB(2,fSTRUCT) MLog("WARNING: cannot encode job messages in %s\n",
        FName);
      }
    }    /* END if ((ShowMB == TRUE) && (J->MB != NULL)) */

  if ((J->System != NULL))
    {
    mxml_t *JSE = NULL;

    if ((MJobSystemToXML(J->System,&JSE) == SUCCESS))
      {
      MXMLAddE(JE,JSE);
      }
    }  /* END if ((J->System != NULL)) */

  if ((bmisset(&J->IFlags,mjifIsTemplate) ||
       ((J->System != NULL) && (J->System->JobType == msjtGeneric))) && (J->TemplateExtensions != NULL))
    {
    mxml_t *XE = NULL;

    MUIJTXToXML(J,JE);

    MXMLAddE(JE,XE);
    }

  if ((J->ExtensionData != NULL) && (bmisset(&J->IFlags,mjifTemplateStat)))
    {
    mtjobstat_t *JStat = (mtjobstat_t *)J->ExtensionData;
    mxml_t *StatNode;

    if (MTJobStatToXML(JStat,IsDisplay,&StatNode) == SUCCESS)
      {
      MXMLAddE(JE,StatNode);
      }
    }

  return(SUCCESS);
  }  /* END MJobToXML() */





/**
 * Convert a job into XML that is used when a destination peer is reporting back
 * to a source peer.  This function enforces cred mapping, host mapping, etc.
 *
 * @see MJobToXML() - peer 
 *
 * @param J (I)
 * @param JEP (O) - NULL or existing mxml_t object [possibly allocated]
 * @param SJAList (I) [optional - not used]
 * @param SRQAList (I) [optional]
 * @param NullVal (I) [optional]
 * @param IsCP (I)
 * @param R (I)
 */

int MJobToExportXML(

  mjob_t  *J,        /* I */
  mxml_t **JEP,      /* O (possibly allocated) - NULL or existing mxml_t object */
  enum MJobAttrEnum *SJAList,  /* I (optional - not used) */
  enum MReqAttrEnum *SRQAList, /* I (optional) */
  char    *NullVal,  /* I (optional) */
  mbool_t  IsCP,     /* I */
  mrm_t   *R)        /* I */

  {
  mxml_t *JE = *JEP;
  mxml_t *MBE;

  char tmpLine[MMAX_LINE];

  if (R == NULL)
    {
    return(FAILURE);
    }

  mstring_t tmpBuf(MMAX_LINE);

  /* AllocNodeList FORMAT: <Node Name>[:<Task Count>][,...] */
        
  MStringSetF(&tmpBuf,"%s:%d",
    MSched.Name,
    J->Request.TC);              

  MXMLSetAttr(JE,(char *)MJobAttr[mjaAllocNodeList],(void **)tmpBuf.c_str(),mdfString);

  /* check if job was cred-mapped */

  if ((J->Grid != NULL) && (J->Grid->User != NULL))
    {
    MXMLSetAttr(JE,(char *)MJobAttr[mjaUser],(void **)J->Grid->User,mdfString);
    }
  else if (R->OMap != NULL)
    {
    if (J->SubmitRM != R)
      {
      MOMap(R->OMap,mxoUser,J->Credential.U->Name,FALSE,TRUE,tmpLine);
      }
    else
      {
      MOMap(R->OMap,mxoUser,J->Credential.U->Name,FALSE,FALSE,tmpLine);
      }

    MXMLSetAttr(JE,(char *)MJobAttr[mjaUser],(void **)tmpLine,mdfString);
    }
  else
    {
    MXMLSetAttr(JE,(char *)MJobAttr[mjaUser],(void **)J->Credential.U->Name,mdfString);
    }
  
  /* group */
 
  if ((J->Grid != NULL) && (J->Grid->Group != NULL))
    {
    MXMLSetAttr(JE,(char *)MJobAttr[mjaGroup],(void **)J->Grid->Group,mdfString);
    }
  else if (R->OMap != NULL)
    {
    if (J->SubmitRM != R)
      { 
      MOMap(R->OMap,mxoGroup,J->Credential.G->Name,FALSE,TRUE,tmpLine);
      }
    else
      {
      MOMap(R->OMap,mxoGroup,J->Credential.G->Name,FALSE,FALSE,tmpLine);
      }

    MXMLSetAttr(JE,(char *)MJobAttr[mjaGroup],(void **)tmpLine,mdfString);
    }
  else
    {
    MXMLSetAttr(JE,(char *)MJobAttr[mjaGroup],(void **)J->Credential.G->Name,mdfString);
    }

  /* class */

  if ((J->Grid != NULL) && (J->Grid->Class != NULL))
    {
    MXMLSetAttr(JE,(char *)MJobAttr[mjaClass],(void **)J->Grid->Class,mdfString);
    }
  else if (R->OMap != NULL)
    {
    if (J->SubmitRM != R)
      { 
      MOMap(R->OMap,mxoClass,J->Credential.C->Name,FALSE,TRUE,tmpLine);
      }
    else
      {
      MOMap(R->OMap,mxoClass,J->Credential.C->Name,FALSE,FALSE,tmpLine);
      }

    if (tmpLine[0] != '\0')
      MXMLSetAttr(JE,(char *)MJobAttr[mjaClass],(void **)tmpLine,mdfString);
    }
  else if (J->Credential.C != NULL)
    {
    MXMLSetAttr(JE,(char *)MJobAttr[mjaClass],(void **)J->Credential.C->Name,mdfString);
    }

  /* account */

  if ((J->Grid != NULL) && (J->Grid->Account != NULL))
    {
    MXMLSetAttr(JE,(char *)MJobAttr[mjaAccount],(void **)J->Grid->Account,mdfString);
    }
  else if (R->OMap != NULL)
    {  
    if (J->SubmitRM != R)
      { 
      MOMap(R->OMap,mxoAcct,J->Credential.A->Name,FALSE,TRUE,tmpLine);
      }
    else
      {
      MOMap(R->OMap,mxoAcct,J->Credential.A->Name,FALSE,FALSE,tmpLine);
      }

    MXMLSetAttr(JE,(char *)MJobAttr[mjaAccount],(void **)tmpLine,mdfString);
    }
  else if (J->Credential.A != NULL)
    {
    MXMLSetAttr(JE,(char *)MJobAttr[mjaAccount],(void **)J->Credential.A->Name,mdfString);
    }

  /* qos */

  if ((J->Grid != NULL) && (J->Grid->QOS != NULL))
    {
    MXMLSetAttr(JE,(char *)MJobAttr[mjaQOS],(void **)J->Grid->QOS,mdfString);
    }
  else if (R->OMap != NULL)
    {
    if (J->SubmitRM != R)
      { 
      MOMap(R->OMap,mxoQOS,J->Credential.Q->Name,FALSE,TRUE,tmpLine);
      }
    else
      {
      MOMap(R->OMap,mxoQOS,J->Credential.Q->Name,FALSE,FALSE,tmpLine);
      }

    MXMLSetAttr(JE,(char *)MJobAttr[mjaQOS],(void **)tmpLine,mdfString);
    }
  else if (J->Credential.Q != NULL)
    {
    MXMLSetAttr(JE,(char *)MJobAttr[mjaQOS],(void **)J->Credential.Q->Name,mdfString);
    }

  /* qos requested */

  if ((J->Grid != NULL) && (J->Grid->QOSReq != NULL))
    {
    MXMLSetAttr(JE,(char *)MJobAttr[mjaQOSReq],(void **)J->Grid->QOSReq,mdfString);
    }
  else if (R->OMap != NULL)
    {
    if (J->SubmitRM != R)
      { 
      MOMap(R->OMap,mxoQOS,J->QOSRequested->Name,FALSE,TRUE,tmpLine);
      }
    else
      {
      MOMap(R->OMap,mxoQOS,J->QOSRequested->Name,FALSE,FALSE,tmpLine);
      }

    MXMLSetAttr(JE,(char *)MJobAttr[mjaQOSReq],(void **)tmpLine,mdfString);
    }
  else
    {
    MXMLSetAttr(JE,(char *)MJobAttr[mjaQOSReq],(void **)J->QOSRequested->Name,mdfString);
    }

  if ((MJOBISACTIVE(J) == FALSE) && (J->MigrateBlockReason != mjneNONE))
    {
    if (J->MigrateBlockMsg != NULL)
      {
      MStringSetF(&tmpBuf,"%s: %s",
        MJobBlockReason[J->MigrateBlockReason],
        J->MigrateBlockMsg);
      }
    else
      {
      MStringSet(&tmpBuf,(char *)MJobBlockReason[J->MigrateBlockReason]);
      }

    MXMLSetAttr(JE,(char *)MJobAttr[mjaBlockReason],(void **)tmpBuf.c_str(),mdfString);
    }

  /* output and error files */

  if (J->Env.RMOutput != NULL)
    {
    char *ofile;
    char *ptr;

    if ((R->ExtHost != NULL) && ((ptr = strchr(J->Env.RMOutput,':')) != NULL))
      {
      snprintf(tmpLine,sizeof(tmpLine),"%s%s",
        R->ExtHost,
        ptr);

      ofile = tmpLine;
      }
    else
      {
      ofile = J->Env.RMOutput;
      }

    MXMLSetAttr(JE,(char *)MJobAttr[mjaOFile],(void **)ofile,mdfString);
    }    /* END if (J->E.RMOutput != NULL) */

  if (J->Env.RMError != NULL)
    {
    char *efile;
    char *ptr;

    if ((R->ExtHost != NULL) && ((ptr = strchr(J->Env.RMError,':')) != NULL))
      {
      snprintf(tmpLine,sizeof(tmpLine),"%s%s",
        R->ExtHost,
        ptr);

      efile = tmpLine;
      }
    else
      {
      efile = J->Env.RMError;
      }

    MXMLSetAttr(JE,(char *)MJobAttr[mjaEFile],(void **)efile,mdfString);
    }    /* END if (J->E.RMError != NULL) */

  /* determine SRM and make sure XML accurately reports it */

  if (J->SystemID != NULL)
    {
    /* job was staged from another peer */

    MXMLSetAttr(JE,(char *)MJobAttr[mjaRM],(void **)J->SystemID,mdfString);
    MXMLSetAttr(JE,(char *)MJobAttr[mjaSRMJID],(void **)J->SystemJID,mdfString);
    MXMLSetAttr(JE,(char *)MJobAttr[mjaDRMJID],(void **)J->Name,mdfString);
    }
  else
    {
    /* job was locally submitted */

    MXMLSetAttr(JE,(char *)MJobAttr[mjaRM],(void **)MSched.Name,mdfString);
    MXMLSetAttr(JE,(char *)MJobAttr[mjaSRMJID],(void **)J->Name,mdfString);    
    }
        
  if ((J->MessageBuffer != NULL) && 
      (MXMLGetChild(JE,(char *)MJobAttr[mjaMessages],NULL,&MBE) == SUCCESS))
    {
    mxml_t *ME;
    int     MTok;

    MTok = -1;

    while (MXMLGetChild(MBE,NULL,&MTok,&ME) == SUCCESS)
      {
      MXMLSetAttr(ME,(char *)MMBAttr[mmbaSource],(void **)MSched.Name,mdfString);
      }
    }

  /* Variables */

  if ((J->Credential.U != NULL) && (J->Credential.U->HomeDir != NULL))
    {
    /* Grid Data Staging: In the case that the home directories are different between
     * the compute node and the submission node, RHOME can be used to distinguish
     * the remote home directory. */

    sprintf(tmpLine,"RHOME=%s",J->Credential.U->HomeDir);

    MXMLAppendAttr(JE,(char *)MJobAttr[mjaVariables],tmpLine,';');
    }

  return(SUCCESS);
  }  /* END MJobToExportXML() */




/**
 * Converts an XML description into a job object.
 * 
 * @see MJobToXML()
 *
 * @param J (I, Modified) The Job that will be populated 
 * @param E (I) The XML description that will be used to populate the Job
 * @param IsPartial (I) job info is partial - multi-source job
 * @param Mode (I) mode to use when setting job attrs
 */

int MJobFromXML(

  mjob_t  *J,          /* I (modified) */
  mxml_t  *E,          /* I */
  mbool_t  IsPartial,  /* I (job info is partial - multi-source job) */
  enum MObjectSetModeEnum Mode)  /* I (mode to use when setting job attrs) */

  {
  int aindex;

  enum MJobAttrEnum jaindex;

  int CTok;

  mxml_t *CE;

  mreq_t *RQ;

  int     JTC;
  int     JNC;

  const char *FName = "MJobFromXML";

  MDB(4,fSTRUCT) MLog("%s(%s,%s,%s)\n",
    FName,
    (J != NULL) ? "J" : "NULL",
    (E != NULL) ? "E" : "NULL",
    MBool[IsPartial]);

  if ((J == NULL) || (E == NULL))
    {
    return(FAILURE);
    }

  if (Mode == mNONE2)
    {
    Mode = mSet;
    }

  /* NOTE:  do not initialize.  may be overlaying data */

  for (aindex = 0;aindex < E->ACount;aindex++)
    {
    jaindex = (enum MJobAttrEnum)MUGetIndex(E->AName[aindex],MJobAttr,FALSE,0);

    if (jaindex == mjaNONE)
      continue;    

    MDB(8,fSTRUCT) MLog("INFO:     XML job attribute '%s'  value: '%s' \n",
      E->AName[aindex],
      (E->AVal[aindex] != NULL) ? E->AVal[aindex] : "NULL");

    if (jaindex == mjaDependBlock)
      {
      /* This is not in MJobSetAttr because we're actually pulling two items
          and we are using checkpoint syntax */

      char *TmpStr = NULL;
      char *TmpTok = NULL;
      char *TmpPtr = NULL;

      MUStrDup(&TmpStr,E->AVal[aindex]);

      TmpPtr = MUStrTok(TmpStr,":",&TmpTok);

      if (TmpPtr != NULL)
        {
        J->DependBlockReason = (enum MDependEnum)MUGetIndex(TmpPtr,MDependType,FALSE,0);

        TmpPtr = MUStrTok(NULL,":",&TmpTok);

        if (TmpPtr != NULL)
          {
          MUStrDup(&J->DependBlockMsg,TmpPtr);
          }
        }

      MUFree(&TmpStr);

      continue;
      } /* END if (jaindex == mjaDependBlock) */

    MJobSetAttr(J,jaindex,(void **)E->AVal[aindex],mdfString,Mode);
    }  /* END for (aindex) */

  /* NOTE: ignore job level settings if per req node/task info specified */

  JTC = J->Request.TC;
  JNC = J->Request.NC;

  J->Request.TC = 0;
  J->Request.NC = 0;
 
  /* load children */

  CTok = -1;

  while (MXMLGetChild(E,NULL,&CTok,&CE) == SUCCESS)
    {
    int rqindex = 0;

    if (!strcmp(CE->Name,(char *)MJobAttr[mjaMessages]))
      {
      MMBFromXML(&J->MessageBuffer,CE);
      }
    else if (!strcmp(CE->Name,(char *)MXO[mxoReq]))
      {
      if (MXMLGetAttrF(CE,(char *)MReqAttr[mrqaIndex],NULL,(void *)&rqindex,mdfInt,0) == SUCCESS)
        {
        if ((rqindex < 0) || (rqindex >= MMAX_REQ_PER_JOB))
          {
          /* checkpoint file is corrupt - invalid rqindex */

          return(FAILURE);
          }

        /* create reqs from 0 to rqindex */

        if (J->Req[rqindex] == NULL)
          {
          if (MReqCreate(J,NULL,&J->Req[rqindex],FALSE) == FAILURE)
            {
            /* cannot allocate memory or MMAX_REQ reached */

            return(FAILURE);
            }
          }    /* END while (J->Req[rqindex] == NULL) */

        RQ = J->Req[rqindex];

        MReqFromXML(J,RQ,CE);
        }  /* END if (MXMLGetAttrF() == SUCCESS) */
      }
    else if (!strcmp(CE->Name,(char *)MXO[mxoxVMCR]))
      {
      if ((J->VMCreateRequest = (mvm_req_create_t *)MUCalloc(1,sizeof(mvm_req_create_t))) == NULL)
        {
        /* If it has a VMCR, it is a VMTracking job.  No good without VMCR */

        return(FAILURE);
        }

      MVMCRFromXML(J->VMCreateRequest,CE);
      }
    else if (!strcmp(CE->Name,"datareq"))
      {
      /* NOTE: need to support "overwriting" existing datareqs in linked lists */

      msdata_t **DP;
      msdata_t *tmpD = NULL;
      msdata_t *Data = NULL;

      /* load in from XML and check type */

      MSDCreate(&tmpD);
      MSDFromXML(tmpD,CE);

      if (J->DataStage == NULL)
        {
        MDSCreate(&J->DataStage);
        }

      if (tmpD->Type == mdstStageIn)
        {
        DP = &J->DataStage->SIData;
        }
      else if (tmpD->Type == mdstStageOut)
        {
        DP = &J->DataStage->SOData;
        }
      else
        {
        /* not supported */

        MSDDestroy(&tmpD);

        continue;
        }

      if (*DP == NULL)
        {
        /* add to head of list */

        *DP = tmpD;

        continue;
        }

      /* add to end of linked-list */

      Data = *DP;

      while (Data->Next != NULL)
        {
        Data = Data->Next;
        }

      Data->Next = tmpD;
      }    /* END if (!strcmp(CE->Name,"datareq")) */
    else if (!strcmp(CE->Name,"mtjobstat"))
      {
      mtjobstat_t *JStat;

      if (J->ExtensionData == NULL)
        {
        J->ExtensionData = MUCalloc(1,sizeof(JStat[0]));
        }

      JStat = (mtjobstat_t *)J->ExtensionData;

      MTJobStatFromXML(CE,JStat);
      }
    else if (!strcmp(CE->Name,"system"))
      {
      if (J->System == NULL)
        MJobCreateSystemData(J);

      if (MJobSystemFromXML(J->System,CE) == SUCCESS)
        {
        /* Set mjifRunAlways on J->Flags on vmstorage, vmcreate, & vmdestroy jobs */

        switch(J->System->JobType)
          {
          case msjtStorage:
#if 0
          case msjtVMStorage:
          case msjtVMCreate:
          case msjtVMDestroy:
#endif
          case msjtVMMap:

            bmset(&J->IFlags,mjifRunAlways);
						break;            

          case msjtOSProvision:
          case msjtOSProvision2:

            MSysJobAddQOSCredForProvision(J);
            break;

          default:
            break;
          }
        }

      if (J->System->JobType == msjtGeneric)
        {
        bmset(&J->IFlags,mjifGenericSysJob);
        }
      }
    else if (!strcmp(CE->Name,MJobAttr[mjaArrayInfo]))
      {
      MJobArrayFromXML(J,CE);
      }
    else if (!strcmp(CE->Name,"trig"))
      {
      /* Job triggers are stored as sub-elements in the job's XML */

      mtrig_t *tmpT;
      mtrig_t *copyT;

      /* This will always add the trigger (not overwrite) */

      tmpT = (mtrig_t *)MUCalloc(1,sizeof(mtrig_t));

      if ((MTrigCreate(tmpT,mttNONE,mtatNONE,NULL) == FAILURE) ||
          (MTrigFromXML(tmpT,CE) == FAILURE))
        {
        MUFree((char **)&tmpT);

        continue;
        }

      copyT = tmpT;
      MTrigAdd(tmpT->TName,tmpT,&copyT);

      MUFree(&tmpT->TName);
      MUFree((char **)&tmpT);
      tmpT = copyT;

      /* Always set trigger to this job */
      MUStrDup(&tmpT->OID,J->Name);
      tmpT->OType = mxoJob;
      tmpT->O = (void *)J;
      MOAddTrigPtr(&J->Triggers,tmpT);

      if (MTrigInitialize(tmpT) == FAILURE)
        {
        MUFree((char **)&tmpT);

        continue;
        }

      if (bmisset(&J->IFlags,mjifIsTemplate))
        {
        bmset(&tmpT->InternalFlags,mtifIsTemplate);
        }
      }
    else if (!strcmp(CE->Name,"tx"))
      {
      int tmpRC;

      if (J->TemplateExtensions == NULL)
        J->TemplateExtensions = (mjtx_t *)MUCalloc(1,sizeof(mjtx_t));

      tmpRC = MTXFromXML(J->TemplateExtensions,CE,J);

      if (tmpRC == FAILURE)
        {
        MUFree((char **)J->TemplateExtensions);
        }
      }
    else if (!strcmp(CE->Name,"Variables"))
      {
      MUAddVarsFromXML(CE,&J->Variables);
      }
    }  /* END while (MXMLGetChild(E,NULL,&CTok,&CE) == SUCCESS) */

  /* Triggers were already hooked up above */

  if (J->Request.TC == 0)
    J->Request.TC = JTC;

  if (J->Request.NC == 0)
    J->Request.NC = JNC;

  return(SUCCESS);
  }  /* END MJobFromXML() */



/**
 * Report base job attributes via XML.
 *
 * @see MUIQueueShow() - parent
 *
 * @param J              (I)
 * @param JEP            (O) [alloc]
 * @param SType          (I)
 * @param DoShowTaskList (I)
 * @param DoVerbose      (I)
 */

int MJobBaseToXML(

  mjob_t                  *J,
  mxml_t                 **JEP,
  enum MJobStateTypeEnum   SType,
  mbool_t                  DoShowTaskList,
  mbool_t                  DoVerbose)

  {
  int  tmpState;
  char tmpStateName[MMAX_NAME];
  char tmpLine[MMAX_LINE];

  long Duration;
  int  Procs;

  mxml_t *JE;

  mreq_t *RQ;

  if ((J == NULL) || (JEP == NULL))
    {
    return(FAILURE);
    }

  /* job attributes specified:
   *  state, wclimit, jobid, user, starttime, submittime, procs, qos
   *  nodecount, group
   */

  if (bmisset(&J->Hold,mhBatch))
    tmpState = mjsBatchHold;
  else if (bmisset(&J->Hold,mhSystem))
    tmpState = mjsSystemHold;
  else if (bmisset(&J->Hold,mhUser))
    tmpState = mjsUserHold;
  else if (bmisset(&J->Hold,mhDefer))
    tmpState = mjsDeferred;
  else
    tmpState = (J->IState != mjsNONE) ? J->IState : J->State;

  MUStrCpy(tmpStateName,(char *)MJobState[tmpState],sizeof(tmpStateName));

  if (SType != mjstCompleted)
    {
    if (((J->DestinationRM != NULL) || bmisset(&J->IFlags,mjifPurge)) &&
        (bmisset(&J->IFlags,mjifWasCanceled)))
      {
      MUStrCpy(tmpStateName,"Canceling",sizeof(tmpStateName));
      }
    else if (J->SubState == mjsstMigrated)
      {
      MUStrCpy(tmpStateName,"Migrated",sizeof(tmpStateName));
      }
    else if (J->SubState == mjsstProlog)
      {
      MUStrCpy(tmpStateName,"Starting",sizeof(tmpStateName));
      }
    else if (J->SubState == mjsstEpilog)
      {
      MUStrCpy(tmpStateName,"Ending",sizeof(tmpStateName));
      }
    }

  *JEP = NULL;

  if (MXMLCreateE(JEP,(char *)MXO[mxoJob]) == FAILURE)
    {
    return(FAILURE);
    }

  RQ = J->Req[0];

  if (RQ == NULL)
    {
    return(FAILURE);
    }

  JE = *JEP;

  if ((bmisset(&MPar[0].Flags,mpfUseCPUTime)) && (J->CPULimit > 0))
    Duration = J->CPULimit;
  else
    Duration = J->WCLimit;

  /* MJobBaseToXML is called for display purposes only, so if TTC is set, 
   * use that value for ReqProcs */

  if (J->TotalTaskCount > 0)
    Procs = J->TotalTaskCount;
  else
    Procs = J->TotalProcCount;

  MXMLSetAttr(JE,(char *)MJobAttr[mjaJobID],(void *)J->Name,mdfString);

  if (J->AName != NULL)
    MXMLSetAttr(JE,(char *)MJobAttr[mjaJobName],(void *)J->AName,mdfString);

  if (J->SystemJID != NULL)
    MXMLSetAttr(JE,(char *)MJobAttr[mjaGJID],(void *)J->SystemJID,mdfString);

  if (J->DRMJID != NULL)
    MXMLSetAttr(JE,(char *)MJobAttr[mjaDRMJID],(void *)J->DRMJID,mdfString);

  MXMLSetAttr(JE,(char *)MJobAttr[mjaState],(void *)tmpStateName,mdfString);

  if (J->Credential.U != NULL)
    MXMLSetAttr(JE,(char *)MJobAttr[mjaUser],(void *)J->Credential.U->Name,mdfString);

  if (J->Credential.G != NULL)
    MXMLSetAttr(JE,(char *)MJobAttr[mjaGroup],(void *)J->Credential.G->Name,mdfString);

  if (J->Credential.A != NULL)
    MXMLSetAttr(JE,(char *)MJobAttr[mjaAccount],(void *)J->Credential.A->Name,mdfString);

  if ((J->Credential.Q != NULL) && (J->Credential.Q->Index != 0))
    MXMLSetAttr(JE,(char *)MJobAttr[mjaQOS],(void *)J->Credential.Q->Name,mdfString);

  if (RQ->PtIndex != 0)
    {
    MXMLSetAttr(JE,(char *)MJobAttr[mjaPAL],(void *)MPar[RQ->PtIndex].Name,mdfString);
    }
  else if (DoVerbose == TRUE)
    {
    /* Not an active or completed job, show PAL or ALL if not defined */

    char tmpLine[MMAX_LINE];

    if (!bmissetall(&J->PAL,MMAX_PAR))
      MXMLSetAttr(JE,(char *)MJobAttr[mjaPAL],(void *)MPALToString(&J->PAL,",",tmpLine),mdfString);
    else
      MXMLSetAttr(JE,(char *)MJobAttr[mjaPAL],(void *)MPar[0].Name,mdfString);
    }

  if (J->Credential.C != NULL)
    MXMLSetAttr(JE,(char *)MJobAttr[mjaClass],(void *)J->Credential.C->Name,mdfString);

  MXMLSetAttr(JE,(char *)MJobAttr[mjaStartTime],(void *)&J->StartTime,mdfLong);
  MXMLSetAttr(JE,(char *)MJobAttr[mjaSubmitTime],(void *)&J->SubmitTime,mdfLong);

  MXMLSetAttr(JE,(char *)MJobAttr[mjaReqAWDuration],(void *)MULToTStringSeconds(Duration,tmpLine,sizeof(tmpLine)),mdfString);
  MXMLSetAttr(JE,(char *)MJobAttr[mjaSuspendDuration],(void *)&J->SWallTime,mdfLong);

  MXMLSetAttr(JE,(char *)MJobAttr[mjaReqProcs],(void *)&Procs,mdfInt);

  if (MSched.BluegeneRM == TRUE)
    {
    int tmpI = J->Request.TC / MSched.BGNodeCPUCnt;

    MXMLSetAttr(JE,(char *)MJobAttr[mjaReqNodes],(void *)&tmpI,mdfInt);
    }
  else if (J->Request.NC > 0)
    {
    MXMLSetAttr(JE,(char *)MJobAttr[mjaReqNodes],(void *)&J->Request.NC,mdfInt);
    }

  if (J->EffQueueDuration > 0)
    MXMLSetAttr(JE,(char *)MJobAttr[mjaEEWDuration],(void *)&J->EffQueueDuration,mdfLong);

  if (J->CurrentStartPriority != 0)
    MXMLSetAttr(JE,(char *)MJobAttr[mjaStartPriority],(void *)&J->CurrentStartPriority,mdfLong);

  if (J->Rsv != NULL)
    {
    MXMLSetAttr(JE,(char *)MJobAttr[mjaRsvStartTime],(void *)&J->Rsv->StartTime,mdfLong);
    }

  if ((SType == mjstActive) || (SType == mjstCompleted))
    {
    int tmpI;

    MXMLSetAttr(JE,(char *)MJobAttr[mjaRunPriority],(void *)&J->RunPriority,mdfLong);

    MXMLSetAttr(JE,(char *)MJobAttr[mjaStatPSDed],(void *)&J->PSDedicated,mdfDouble);
    MXMLSetAttr(JE,(char *)MJobAttr[mjaStatPSUtl],(void *)&J->PSUtilized,mdfDouble);

    if (SType == mjstCompleted)
      {
      if (bmisset(&J->IFlags,mjifWasCanceled))
        {
        char tmpLine[MMAX_LINE];

        if (J->CompletionCode != 0)
          {
          snprintf(tmpLine,sizeof(tmpLine),"CNCLD(%d)",
            J->CompletionCode);
          }
        else
          {
          snprintf(tmpLine,sizeof(tmpLine),"CNCLD");
          }

        MXMLSetAttr(JE,(char *)MJobAttr[mjaCompletionCode],(void *)tmpLine,mdfString);
        }
      else
        {
        MXMLSetAttr(JE,(char *)MJobAttr[mjaCompletionCode],(void *)&J->CompletionCode,mdfInt);
        }

      MXMLSetAttr(JE,(char *)MJobAttr[mjaCompletionTime],(void *)&J->CompletionTime,mdfLong);
      }

    if (RQ->DRes.Mem > 0)
      {
      tmpI = RQ->TaskCount * RQ->DRes.Mem;

      MXMLSetAttr(JE,(char *)MJobAttr[mjaReqMem],(void *)&tmpI,mdfInt);
      }

    if (!MNLIsEmpty(&J->NodeList))
      {
      mnode_t *tmpN;

      char NameBuf[MMAX_NAME];

      MNLGetNodeAtIndex(&J->NodeList,0,&tmpN);
      MXMLSetAttr(
        JE,
        (char *)MJobAttr[mjaMasterHost],
        (void *)MNodeAdjustName(tmpN->Name,0,NameBuf),
        mdfString);
      }

    MXMLSetAttr(JE,(char *)MJobAttr[mjaAWDuration],(void *)&J->AWallTime,mdfLong);
    }  /* END if ((SType == mjsActive) || (SType == mjstCompleted)) */

  if (DoShowTaskList == TRUE)
    {
    int nindex;

    mnode_t *N;

    char tmpLine[MMAX_BUFFER];

    tmpLine[0] = '\0';

    for (nindex = 0;MNLGetNodeAtIndex(&J->NodeList,nindex,&N) == SUCCESS;nindex++)
      {
      sprintf(tmpLine,"%s%d:%d;",
        tmpLine,
        N->RackIndex,
        N->SlotIndex);

      MDB(4,fSTRUCT) MLog("INFO:     adding node '%s' of job '%s' to buffer\n",
        N->Name,
        J->Name);
      }  /* END for (nindex) */

    if (tmpLine[0] != '\0')
      MXMLSetAttr(JE,(char *)MJobAttr[mjaAllocNodeList],(void *)tmpLine,mdfString);
    }  /* END if (DoShowTaskList == TRUE) */

  /* per partition start priorities */

  if (MSched.PerPartitionScheduling == TRUE)
    {
    __MJobParPriorityToXML(J,JE);
    } 


  /* system job user proxy */

  if ((J->System != NULL) &&
      (J->System->ProxyUser != NULL))
    {
    MXMLSetAttr(JE,(char *)MJobSysAttr[mjsaProxyUser],(void *)J->System->ProxyUser,mdfString);
    }

  return(SUCCESS);
  }  /* END MJobBaseToXML() */
/* END MJobXML.c */
