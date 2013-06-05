/* HEADER */

      
/**
 * @file MObject.c
 *
 * Contains: Object functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 * Write object event record/stats to specified file pointer.
 *
 * NOTE:  In later versions, a unique event id should be associated with each event
 *        record and written to the event database.  Objects can then maintain a
 *        history by simply including a list of event identifiers. (NYI)
 *
 * NOTE:  in most cases, FP should be MStat.eventfp which points to X 
 *
 * @see MSysRecordEvent() - child
 *
 * @param O (I)
 * @param OType (I)
 * @param EType (I)
 * @param Msg (I) [optional]
 * @param FP (I) [optional]
 * @param UserName (I) [optional]
 */

int MOWriteEvent(

  void                      *O,      /* I */
  enum MXMLOTypeEnum         OType,  /* I */
  enum MRecordEventTypeEnum  EType,  /* I */
  const char                *Msg,    /* I (optional) */
  FILE                      *FP,     /* I (optional) */
  char                      *UserName)/* I (optional) User responsible for event */
 
  {
  int rc;

  char *ONameP = NULL;

  char *UName = NULL;

  const char *FName = "MOWriteEvent";
 
  MDB(4,fSTAT) MLog("%s(%s,%s,%s,%s,%s,%s)\n",
    FName,
    (O != NULL) ?  "O" : "NULL",
    MXO[OType],
    MRecordEventType[EType],
    (Msg != NULL) ? Msg : "NULL",
    (FP != NULL) ? "FP" : "NULL",
    (UserName != NULL) ? UserName : "NULL");

  if ((O == NULL) &&
      ((EType != mrelNote) || (OType != mxoNONE)))
    {
    return(FAILURE);
    }

  if (UserName != NULL)
    {
    UName = UserName;
    }
 
  if (FP == stderr)
    {
    fprintf(FP,"STATS:  ");
    }

  /* this allows us to no longer check in MSched.RecordEventList before
   * calling this function! */

  if (!bmisset(&MSched.RecordEventList,EType))
    {
    return(FAILURE);
    }

  if (OType != mxoNONE)
    MOGetName(O,OType,&ONameP);
  else
    ONameP = (char *)MXO[mxoNONE];

  mstring_t JobString(MMAX_LINE);
  mstring_t Buf(MMAX_LINE);

  switch (OType)
    { 
    mjob_t *J;

    case mxoJob:

      J = (mjob_t *)O;

      if (bmisset(&J->IFlags,mjifNoEventLogs))
        {
        rc = FAILURE;

        break;
        }

      UName = ((J->System != NULL) && (J->System->ProxyUser)) ? J->System->ProxyUser : J->Credential.U->Name;

      if (MSched.WikiEvents == TRUE)
        { 
        rc = MJobToWikiString(
          (mjob_t *)O,
          Msg,
          &JobString);
        }
      else
        {
        rc = MJobToTString(
          (mjob_t *)O,
          EType,
          Msg,
          &JobString);
        }

      break;

    case mxoNode:

      rc = MNodeToWikiString(
        (mnode_t *)O,
        NULL,
        &Buf);

      break;

    case mxoRsv:

      rc = MRsvToWikiString(
        (mrsv_t *)O,
        NULL,
        &Buf);

      break;

    case mxoSched:

      /* no extra output required */

      rc = SUCCESS;

      break;

    case mxoTrig:

      rc = MTrigToTString(
        (mtrig_t *)O,
        EType,
        &Buf);

      break;

    case mxoNONE:

      if (EType != mrelNote)
        rc = FAILURE;
      else
        rc = SUCCESS;

      break;

    case mxoxVM:
      {
      rc = MTraceBuildVMResource(
        (mvm_t *)O,
        &Buf);
      }

      break;


    default:

      rc = FAILURE;

      break;
    }  /* END switch (OType) */

  if (rc == SUCCESS)
    {
    if ((Msg != NULL) && (OType != mxoJob))
      {
      char tmpMsg[MMAX_BUFFER];
      char tmpLine[MMAX_LINE * 4];

      MUStrCpy(tmpMsg,Msg,sizeof(tmpMsg));

      /* FORMAT:  <EVENTHEADER> <JOBATTRS> MSG='MSG' */

      /* remove newline at end of message */
      MUStringChop(tmpMsg);

      snprintf(tmpLine,sizeof(tmpLine)," MSG='%s'",
        tmpMsg);

      if (!Buf.empty())
        {
        int blen = strlen(Buf.c_str());
        /* appending 'MSG' string - remove mid string newlines if present */

        if (Buf[blen - 1] == '\n')
          MStringStripFromIndex(&Buf,-1);

        MStringAppend(&Buf,tmpLine);
        }
      else
        {
        MStringAppend(&Buf,tmpLine);
        }
      }    /* END if (Msg != NULL) */

    MSysRecordEvent(
      OType,
      ONameP,
      EType,
      UName,
      (OType == mxoJob) ? JobString.c_str() : Buf.c_str(),
      NULL);

#ifdef MYAHOO
    /* Yahoo has asked for some custom event logs.
     * Is there a better way to facilitate their requests without
     * changing the very brittle nature of the event file format? */

    if (OType == mxoJob)
      {
      mjob_t *J = (mjob_t *)O;
      char  AllocHosts[MMAX_LINE*2];  /* this is plenty large for Yahoo jobs (they typically only
                                         run on one host) */

      char *EventComment;
      char *AHPtr;
      int   AHSpace;
      int   index;

      char *BPtr;
      int   BSpace;

      MUSNInit(&AHPtr,&AHSpace,AllocHosts,sizeof(AllocHosts));

      if (J->TaskMap != NULL)
        {
        for (index = 0;J->TaskMap[index] != -1;index++)
          {
          MUSNPrintF(&AHPtr,&AHSpace,"%s%s",
            (AllocHosts[0] != '\0') ? "," : "",
            MNode[J->TaskMap[index]]->Name);
          }  /* END for (index) */
        }

      /* FORMAT: <ANAME> <EXECUTION HOSTS> <CPU TIME> (<MSG>)*/

      MStringAppendF(&JobString,"%s %s %d",
        (J->AName != NULL) ? J->AName : "NULL",
        AllocHosts,
        J->CPUTime);

      if (Msg != NULL)
        {
        MStringAppendF(&JobString," MSG='%s'",
          Msg);
        }

      if (MUHTGet(&J->Variables,"ecomment",(void **)&EventComment,NULL) == SUCCESS)
        {
        MStringAppendF(&JobString," COMMENT='%s'",
          EventComment);
        }

      MSysRecordEvent(
        OType,
        ONameP, 
        EType,
        UName,
        JobString.c_str(),
        NULL);
      }
#endif /* MYAHOO */

    MDB(4,fSTAT) MLog("INFO:     stats written for '%s' object\n",
      MXO[OType]);
    }  /* END if (rc == SUCCESS) */
 
  return(SUCCESS);
  }  /* END MOWriteEvent() */




/**
 * Remove checkpoint line for specified object in the database.
 *
 * @see MOCheckpoint - peer
 *
 * @param OType 
 * @param Name
 * @param Timestamp
 * @param EMsg
 */

int MORemoveCheckpoint(

  enum MXMLOTypeEnum OType,
  char              *Name,
  long unsigned int  Timestamp,
  char              *EMsg)

  {
  enum MCkptTypeEnum CPType;

  mdb_t *MDBInfo = NULL;

  MSchedGetAttr(msaDBHandle,mdfNONE,(void **)&MDBInfo,-1);

  if ((MDBInfo == NULL) || (MCP.UseDatabase == FALSE) || (MDBInfo->DBType == mdbNONE))
    return(FAILURE);

  switch (OType)
    {
    case mxoJob:   CPType = mcpJob; break;
    case mxoxCJob: CPType = mcpCJob; break;
    case mxoPar:   CPType = mcpPar; break;
    case mxoRsv:   CPType = mcpRsv; break;
    default:       CPType = mcpLAST; break;
    }

  if (CPType == mcpLAST)
    {
    return(FAILURE);
    }

  return(MDBDeleteCP(MDBInfo,(char *)MCPType[CPType],Name,Timestamp,EMsg));
  }  /* END MORemoveCheckpoint() */
  




/**
 * Checkpoint specified object to the database.
 *
 * @see MORemoveCheckpoint - peer
 *
 * @param OType 
 * @param O
 * @param FullCP (I) Whether to store a full chkpt (only applicable to jobs at this point)
 * @param EMsg
 *
 * @see MJobStoreCP() - child
 */

int MOCheckpoint(

  enum MXMLOTypeEnum  OType,  /* I */
  void               *O,      /* I */
  mbool_t             FullCP, /* I */
  char               *EMsg)   /* I */

  {
  enum MStatusCodeEnum SC;

  char   tmpEMsg[MMAX_LINE];
  int    rc = 0;

  mdb_t *MDBInfo;


  MSchedGetAttr(msaDBHandle,mdfNONE,(void **)&MDBInfo,-1);

  if ((MCP.UseDatabase == FALSE) || ((MDBInfo->DBType == mdbNONE)) || (O == NULL))
    {
    return(FAILURE);
    }
  
  mstring_t Buffer(MMAX_BUFFER);

  switch (OType)
    {
    case mxoxCJob:
 
      if (MJobStoreCP((mjob_t *)O,&Buffer,FullCP) == SUCCESS)
        {
        MUStringToSQLString(&Buffer,TRUE);

        rc = MDBInsertCP(
          MDBInfo,
          (char *)MCPType[mcpCJob],
          ((mjob_t *)O)->Name,
          MSched.Time,
          Buffer.c_str(),
          EMsg,
          &SC);
        }
      else
        {
        rc = FAILURE;
        }

      /*NOTREACHED*/

      break;

    case mxoJob:

      if (MJobStoreCP((mjob_t *)O,&Buffer,FullCP) == SUCCESS)
        {
        MUStringToSQLString(&Buffer,TRUE);

        rc = MDBInsertCP(MDBInfo,(char *)MCPType[mcpJob],((mjob_t *)O)->Name,MSched.Time,Buffer.c_str(),tmpEMsg,&SC);

        if (rc == FAILURE)
          {
          char tmpLine[MMAX_LINE];

          snprintf(tmpLine,sizeof(tmpLine),"error writing to checkpoint DB (%s)",
            tmpEMsg);

          MDB(1,fCKPT) MLog("ERROR:  %s\n",
            tmpLine);

          MMBAdd(
            &MSched.MB,
            tmpLine,
            NULL,
            mmbtOther,
            0,
            0,
            NULL);
          }
        }
      else
        {
        rc = FAILURE;
        }

      break;

    case mxoNode:

      if (MNodeToString((mnode_t *)O,&Buffer) == SUCCESS)
        {
        rc = MDBInsertCP(MDBInfo,(char *)MCPType[mcpNode],((mnode_t *)O)->Name,MSched.Time,Buffer.c_str(),EMsg,&SC);
        }
      else
        {
        rc = FAILURE;
        }

      break;
 
    case mxoPar:

      break;
 
    case mxoRsv:

      {
      mrsv_t *R = (mrsv_t *)O;
      mxml_t *E=NULL;

      if ((MRsvToXML(R,&E,NULL,NULL,FALSE,mcmCP) == SUCCESS) &&
          (MXMLToMString(E,&Buffer,NULL,TRUE) == SUCCESS))
        {
        MXMLDestroyE(&E);

        rc = MDBInsertCP(MDBInfo,(char *)MCPType[mcpRsv],R->Name,MSched.Time,Buffer.c_str(),EMsg,&SC);
        }
      else
        {
        rc = FAILURE;
        }
      }
 
      break;

    case mxoSRsv:

      if (MSRToString((msrsv_t *)O,Buffer.c_str()) == SUCCESS)
        {
        rc = MDBInsertCP(MDBInfo,(char *)MCPType[mcpSRsv],((msrsv_t *)O)->Name,MSched.Time,Buffer.c_str(),EMsg,&SC);
        }
      else
        {
        rc = FAILURE;
        }

      break;

    case mxoxTJob:

      if (MTJobStoreCP((mjob_t *)O,Buffer.c_str(),Buffer.capacity(),TRUE) == SUCCESS)
        {
        rc = MDBInsertCP(MDBInfo,(char *)MCPType[mcpTJob],((mjob_t *)O)->Name,MSched.Time,Buffer.c_str(),EMsg,&SC);
        }
      else
        {
        rc = FAILURE;
        }

    case mxoTrig:

      {
      mtrig_t *T = (mtrig_t *)O;
      mxml_t  *TE = NULL;

      if ((MTrigToXML(T,TE,NULL) == SUCCESS) &&
          (MXMLToMString(TE,&Buffer,NULL,TRUE) == SUCCESS))
        {
        MXMLDestroyE(&TE);

        rc = MDBInsertCP(MDBInfo,(char *)MCPType[mcpTrigger],T->TName,MSched.Time,Buffer.c_str(),EMsg,&SC);
        }
      else
        {
        MXMLDestroyE(&TE);

        rc = FAILURE;
        }
      }   /* END case mxoTrig */

      break;

    case mxoUser:
    case mxoGroup:
    case mxoAcct:
    case mxoClass:
    case mxoQOS:

      {
      char *OID = NULL;

      enum MCkptTypeEnum CPType = mcpLAST;

      switch (OType)
        {
        case mxoUser:  OID = ((mgcred_t *)O)->Name; CPType = mcpUser;  break;
        case mxoGroup: OID = ((mgcred_t *)O)->Name; CPType = mcpGroup; break;
        case mxoAcct:  OID = ((mgcred_t *)O)->Name; CPType = mcpAcct;  break;
        case mxoClass: OID = ((mclass_t *)O)->Name; CPType = mcpClass; break;
        case mxoQOS:   OID = ((mqos_t *)O)->Name;   CPType = mcpQOS;   break;
     
        default:  rc = FAILURE; break;
        }  /* END switch (OIndex) */
 
      if (MCredToString(O,OType,TRUE,&Buffer) == SUCCESS)
        {
        rc = MDBInsertCP(MDBInfo,(char *)MCPType[CPType],OID,MSched.Time,Buffer.c_str(),EMsg,&SC);
        }
      else
        {
        rc = FAILURE;
        }
      }  /* END BLOCK mxo<CRED> */
 
      break;

    case mxoRM:

      if (MRMStoreCP((mrm_t *)O,Buffer.c_str(),Buffer.capacity()) == SUCCESS)
        {
        rc = MDBInsertCP(MDBInfo,(char *)MCPType[mcpRM],((mrm_t *)O)->Name,MSched.Time,Buffer.c_str(),EMsg,&SC);
        }
      else
        {
        rc = FAILURE;
        }
          
      break;

    case mxoSched:

      if (MSchedToString((msched_t *)O,mfmAVP,TRUE,&Buffer) == SUCCESS)
        {
        rc = MDBInsertCP(MDBInfo,(char *)MCPType[mcpSched],(char *)MXO[mxoSched],MSched.Time,Buffer.c_str(),EMsg,&SC);
        }
      else
        {
        rc = FAILURE;
        }

      break;

    case mxoSys:
 
      if (MSysToString(&MSched,&Buffer,TRUE) == SUCCESS)
        {
        rc = MDBInsertCP(MDBInfo,(char *)MCPType[mcpSys],(char *)MXO[mxoSys],MSched.Time,Buffer.c_str(),EMsg,&SC);
        }
      else
        {
        rc = FAILURE;
        }

      break;

    case mxoxVPC:

      {
      mxml_t *PE;
      mpar_t *P = (mpar_t *)O;

      PE = NULL;

      rc = FAILURE;

      MXMLCreateE(&PE,(char *)MXO[mxoPar]);
    
      MParToXML(P,PE,NULL);

      if (MXMLToMString(PE,&Buffer,NULL,TRUE) == SUCCESS)
        {
        rc = MDBInsertCP(MDBInfo,(char *)MCPType[mcpPar],(char *)MXO[mxoxVPC],MSched.Time,Buffer.c_str(),EMsg,&SC);
        }
    
      MXMLDestroyE(&PE);
      }

      break;

    default:

      rc = FAILURE;

      break;
    }  /* END switch (OType) */

  return(rc);
  }  /* END MOCheckpoint() */


/**
 *
 *
 * @param O (I) [modified]
 * @param OType (I)
 * @param Value (I)
 * @param Format (I)
 * @param Mode (I) [mSet/mAdd/mClear]
 */

int MOSetMessage(

  void                   *O,      /* I (modified) */
  enum MXMLOTypeEnum      OType,  /* I */
  void                  **Value,  /* I */
  enum MDataFormatEnum    Format, /* I */
  enum MObjectSetModeEnum Mode)   /* I (mSet/mAdd/mClear) */

  {
  if (O == NULL)
    {
    return(FAILURE);
    }

  switch (OType)
    {
    case mxoJob:

      MJobSetAttr((mjob_t *)O,mjaMessages,Value,Format,Mode);

      break;

    case mxoNode:

      MNodeSetAttr((mnode_t *)O,mnaMessages,Value,Format,Mode);

      break;

    case mxoRsv:

      MRsvSetAttr((mrsv_t *)O,mraMessages,Value,Format,Mode);

      break;

    case mxoSched:

      MSchedSetAttr((msched_t *)O,msaMessage,Value,Format,Mode);

      break;

    case mxoClass:
    case mxoUser:
    case mxoGroup:
    case mxoQOS:
    case mxoAcct:

      MCredSetAttr((void *)O,OType,mcaMessages,NULL,Value,NULL,Format,Mode);

      break;

    default:

      return(FAILURE);

      /*NOTREACHED*/

      break;
    }  /* END switch (OType) */

  return(SUCCESS);
  }  /* END MOSetAttr() */


/**
 *
 *
 * @param O (I) [consumer]
 * @param OIndex (I) [consumer type]
 * @param OType (I)
 * @param OP (O) [object policy usage]
 * @param DP (O) [default policy usage]
 * @param QP (O) qos policy usage [optional]
 */

int MOGetCredUsage(

  void                *O,      /* I (consumer) */
  enum MXMLOTypeEnum   OIndex, /* I (consumer type) */
  enum MXMLOTypeEnum   OType,  /* I */
  mpu_t              **OP,     /* O (object policy usage) */
  mpu_t              **DP,     /* O (default policy usage) */
  mpu_t              **QP)     /* O (optional) qos policy usage */

  {
  mjob_t *J;
  mrsv_t *R;

  /* return object, default, and QoS pointers for requested objects */

  if (OP != NULL)
    *OP = NULL;
  else
    return(FAILURE);

  if (DP != NULL)
    *DP = NULL;
  else
    return(FAILURE);

  if (QP != NULL)
    *QP = NULL;

  switch (OIndex)
    {
    case mxoJob:
    default:

      {
      J = (mjob_t *)O;
    
      if (QP != NULL)
        {
        if (J->Credential.Q != NULL)
          *QP = J->Credential.Q->L.OverrideActivePolicy;
        }

      switch (OType)
        {
        case mxoUser:

          if (J->Credential.U != NULL)
            *OP = &J->Credential.U->L.ActivePolicy;

          if (MSched.DefaultU != NULL)
            *DP = &MSched.DefaultU->L.ActivePolicy;

          break;

        case mxoGroup:

          if (J->Credential.G != NULL)
            *OP = &J->Credential.G->L.ActivePolicy;

          if (MSched.DefaultG != NULL)
            *DP = &MSched.DefaultG->L.ActivePolicy;

          break;

        case mxoAcct:

          if (J->Credential.A != NULL)
            *OP = &J->Credential.A->L.ActivePolicy;

          if (MSched.DefaultA != NULL)
            *DP = &MSched.DefaultA->L.ActivePolicy;

          break;

        case mxoQOS:

          if (J->Credential.Q != NULL)
            *OP = &J->Credential.Q->L.ActivePolicy;
  
          if (MSched.DefaultQ != NULL)
            *DP = &MSched.DefaultQ->L.ActivePolicy;

          break;

        case mxoClass:

          if (J->Credential.C != NULL)
            *OP = &J->Credential.C->L.ActivePolicy;

          if (MSched.DefaultC != NULL)
            *DP = &MSched.DefaultC->L.ActivePolicy;

          break;

        default:

          /* NOT HANDLED */

          break;
        }  /* END switch (OType) */
      }    /* END BLOCK (case mxoJob) */

      break;

    case mxoRsv:

      {
      R = (mrsv_t *)O;

      if (QP != NULL)
        {
        if (R->Q != NULL)
          *QP = R->Q->L.OverrideActivePolicy;
        }

      switch (OType)
        {
        case mxoUser:

          if (R->U != NULL)
            *OP = R->U->L.RsvActivePolicy;

          if (MSched.DefaultU != NULL)
            *DP = MSched.DefaultU->L.RsvActivePolicy;

          break;

        case mxoGroup:

          if (R->G != NULL)
            *OP = R->G->L.RsvActivePolicy;

          if (MSched.DefaultG != NULL)
            *DP = MSched.DefaultG->L.RsvActivePolicy;

          break;

        case mxoAcct:

          if (R->A != NULL)
            *OP = R->A->L.RsvActivePolicy;

          if (MSched.DefaultA != NULL)
            *DP = MSched.DefaultA->L.RsvActivePolicy;

          break;

        case mxoQOS:

          if (R->Q != NULL)
            *OP = R->Q->L.RsvActivePolicy;

          if (MSched.DefaultQ != NULL)
            *DP = MSched.DefaultQ->L.RsvActivePolicy;

          break;

        case mxoClass:

          /* NOT SUPPORTED */

          /* NO-OP */

          break;

        default:

          /* NOT HANDLED */

          break;
        }  /* END switch (OType) */

      /* NYI */
      }  /* END BLOCK (case mxoRsv) */
    }    /* END switch (OIndex) */

  return(SUCCESS);
  }  /* END int MOGetCredUsage() */


/**
 * Finds and returns the name of the next available object.
 *
 * NOTE:  Multi-byte character names may include negative characters in Name[0] position.
 *
 * @param O      (I)
 * @param OIndex (I)
 * @param OEnd   (I) [modified]
 * @param HTI    (I) [modified] - not used for all OTypes
 * @param NameP  (O)
 */

void *MOGetNextObject(

  void             **O,
  enum MXMLOTypeEnum OIndex,
  void              *OEnd,
  mhashiter_t       *HTI,
  char             **NameP)

  { 
  *NameP = NULL;

  if (OIndex != mxoUser)
    {
    if ((O == NULL) || (OEnd == NULL))
      {
      return(NULL);
      }
  
    if ((*O > OEnd) && (OIndex != mxoSched))
      {
      *O = NULL;
    
      return(*O);
      }
    }
 
  switch (OIndex) 
    {
    case mxoUser:  
    case mxoGroup:
    case mxoAcct:
      {
      mgcred_t *O;
      mhash_t *hashTable;
 
      hashTable = (mhash_t *)MSched.T[OIndex];

      if (HTI == NULL)
        return(NULL);

      if ((hashTable != NULL) && (MUHTIterate(hashTable,NULL,(void **)&O,NULL,HTI) == SUCCESS))
        {
        if (NameP != NULL)
          *NameP = O->Name;

        return((void*)O);
        }  /* END for (O) */
      }
 
      break;

    case mxoQOS:

      {
      mqos_t *Q;
 
      for (Q = *(mqos_t **)O + 1;Q <= (mqos_t *)OEnd;Q += 1)
        {
        if ((Q == NULL) || (Q->Name[0] == '\0'))
          continue;

        if ((Q->Name[0] != '\1'))
          {
          *O = (void *)Q;
 
          if (NameP != NULL)
            *NameP = Q->Name;
 
          return(*O);
          }
        }  /* END for (Q) */
      }

      break;

    case mxoClass:

      {
      mclass_t *C;
 
      for (C = *(mclass_t **)O + 1;C <= (mclass_t *)OEnd;C += 1)
        {
        if ((C == NULL) || (C->Name[0] == '\0'))
          continue;
 
        if ((C->Name[0] != '\1'))
          {
          *O = (void *)C;
 
          if (NameP != NULL)
            *NameP = C->Name;
 
          return(*O);
          }
        }  /* END for (C) */
      }    /* END BLOCK */

      break;

    case mxoPar:

      {
      mpar_t *P;

      for (P = *(mpar_t **)O + 1;P <= (mpar_t *)OEnd;P += 1)
        {
        if ((P == NULL) || (P->Name[0] == '\0'))
          continue;

        if ((P->Name[0] != '\1'))
          {
          *O = (void *)P;

          if (NameP != NULL)
            *NameP = P->Name;

          return(*O);
          }
        }  /* END for (P) */
      }    /* END BLOCK */

      break;

    case mxoSched:

      {
      *NameP = MSched.Name;

      if (*O == &MSched)
        {
        /* initial pass */

        *O = NULL;
    
        return(&MSched);
        }
      else
        {
        *O = NULL;

        return(NULL);
        }

      return(*O);
      }  /* END BLOCK */

      break;

    default: 

      *NameP = NULL; 

      break; 
    }  /* END switch (OIndex) */

  *O = NULL;

  return(*O);
  }  /* END MOGetNextObject() */



/* END MObject.c */
