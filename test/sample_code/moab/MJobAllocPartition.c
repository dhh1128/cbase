/* HEADER */

      
/**
 * @file MJobAllocPartition.c
 *
 * Contains: MJobCreateAllocPartition and MJobDestroyAllocPartition
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


#ifdef __MCPA
#include "cpalib.h"

/* NOTE:  may use CPA_MAX_NODES at some point */

unsigned int MaxListSize = CPA_MAX_PART;
unsigned int MaxNID = CPA_MAX_NID;
int __MCPAFreeNodeReq(cpa_node_req_t *,cpa_nid_list_t);
#endif /* __MCPA */



#define MAX_PCSIZESPERREQ 32  /* max number of proccount sizes within single req nodelist */



/**
 * Qsort comparision algorithm - sort mnalloc_t array lowest node index first.
 *
 * @param A (I)
 * @param B (I)
 */

int __MNLNodeIndexCompLToH(

  mnalloc_old_t *A,  /* I */
  mnalloc_old_t *B)  /* I */

  {
  /* order low to high */

  return(A->N->NodeIndex - B->N->NodeIndex);
  }  /* END __MNLNodeIndexCompLToH() */



/**
 * Create new per-job allocation partition. (used for Cray, BProc, etc systems)
 *
 * @see MJobDestroyAllocPartition() - peer
 * @see MUReadPipe2() - child
 *
 * @param J (I) [modified]
 * @param R (I)
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 * @param SC (O) [optional]
 */

int MJobCreateAllocPartition(

  mjob_t *J,    /* I (modified) */
  mrm_t  *R,    /* I */
  char   *EMsg, /* O (optional,minsize=MMAX_LINE) */
  int    *SC)   /* O (optional) */

  {
  int   rc;

#ifndef __MCPA
  /* ifndef'ed' to prevetn compiler warnings since exitCode 
   * isn't used in __MCPA */

  int   exitCode;
#endif

  int   nindex;

  mreq_t *CRQ;        /* compute request */

  mulong ParID;       /* O - partition id */
  unsigned long long AdminCookie; /* O - admin cookie */
  unsigned long long AllocCookie; /* O - alloc cookie */

  char  tEMsg[MMAX_LINE];
  char  tmpLine[MMAX_LINE];

  static mbool_t  AssignHostList = MBNOTSET;

  mbool_t         UseCPA            = FALSE;
  mbool_t         ApplyAllocJobVars = FALSE;
  mbool_t         IsBProcBased      = FALSE;

  mstring_t NewEnvVariables(MMAX_LINE);;

  int             UID = -1;

#ifdef __MCPA
  int             PCListSize;  /* number of entries in proccount size array */
  int             pcindex;

  mnalloc_t      *NLPtr;

  cpa_node_req_t *NodeReq = NULL;
  cpa_node_req_t *PrevNodeReq;

  cpa_nid_list_t  Wanted = NULL;
#endif /* __MCPA */

  const char *FName = "MJobCreateAllocPartition";

  MDB(4,fSCHED) MLog("%s(%s,%s,EMsg,SC)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (R != NULL) ? R->Name : "NULL");

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (SC != NULL)
    *SC = 0;

  if ((R == NULL) || (J == NULL))
    {
    return(FAILURE);
    }

  if ((MSched.Mode != msmNormal) &&
      (MSched.Mode != msmSlave))
    {
    return(SUCCESS);
    }

  switch (R->SubType)
    {
    case mrmstXT3:
    case mrmstXT4:

      /* NO-OP */

      break;

    default:

      if (!bmisset(&R->IFlags,mrmifSetTaskmapEnv))
        {
        MDB(2,fSCHED) MLog("INFO:     cannot modify environment for rm job %s\n",
          J->Name);

        return(FAILURE);
        }

      break;
    }  /* END switch (R->SubType) */
 
  switch (R->SubType)
    {
    case mrmstXT3:
    case mrmstXT4:

      if (AssignHostList == MBNOTSET)
        {
        if (getenv("MOABDISABLEXT3HOSTASSIGNMENT") == NULL)
          AssignHostList = TRUE;
        else
          AssignHostList = FALSE;
        }
   
      {
      int rqindex;
      int PC;

      /* NOTE:  do not use MJobGetPC() which assumes all jobs have compute component */

      PC = 0;

      for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
        {
        if (J->Req[rqindex] == NULL)
          break;

        PC += J->Req[rqindex]->TaskCount * J->Req[rqindex]->DRes.Procs;
        }

      if (PC == 0)
        {
        /* 'zero proc' job - no CPA partition required */
     
        return(SUCCESS);
        }
      }    /* END BLOCK */
 
      if (bmisset(&J->IFlags,mjifAllocPartitionCreated))
        {
        MJobDestroyAllocPartition(J,R,"orphan");
        }
 
      /* create partition */
     
      if (R->NoAllocMaster == TRUE)
        {
        CRQ = J->Req[0];
        }
      else
        {
        CRQ = J->Req[1];
        }
     
      if ((CRQ == NULL) || (CRQ->TaskCount <= 0))
        {
        /* most jobs contain 'login' resource on Req[0], compute resources on Req[1] */
     
        if (EMsg != NULL)
          {
          sprintf(EMsg,"cannot create alloc partition - job is corrupt, no compute req");
          }
     
        return(FAILURE);
        }
   
#ifdef __MCPA
      {
      int PCCount;                    /* number of procgroups currently tracked */
      int PCList[MAX_PCSIZESPERREQ];  /* proccount size array (terminated w/-1) */
      int rqindex;

      PCCount = 0;
      PCList[PCCount] = -1;

      for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
        {
        CRQ = J->Req[rqindex];

        if (CRQ == NULL)
          break;

        for (nindex = 0;nindex < J->NLSize;nindex++)
          {
          if (CRQ->NodeList[nindex].N == NULL)
            break;

          if (CRQ->NodeList[nindex].TC * CRQ->DRes.Procs != PCList[0])
            {
            /* non-default taskcount size located */

            /* search existing list of taskgroups for match */

            for (pcindex = 0;pcindex < MAX_PCSIZESPERREQ;pcindex++)
              {
              if (PCList[pcindex] == -1)
                {
                /* end of list reached - new taskgroup located - add to list */
  
                PCList[pcindex] = CRQ->NodeList[nindex].TC * CRQ->DRes.Procs;
                PCList[pcindex + 1] = -1;

                PCCount++;

                break;
                }

              if (PCList[pcindex] == CRQ->NodeList[nindex].TC * CRQ->DRes.Procs)
                {
                /* taskgroup match located - end search */

                break;
                }
              }  /* END for (pcindex) */ 

            if (PCCount >= MAX_PCSIZESPERREQ)
              break;
            }  /* END if (CRQ->NodeList[nindex].TC * CRQ->DRes.Procs != PCList[0]) */
          }    /* END for (nindex) */
        }      /* END for (rqindex) */
      }        /* END BLOCK */
#else /* __MCPA */
      /* count # of nodes if compute req nodelist */

      /* NOTE:  use CRQ->TaskCount as upper bound - cannot have more
                nodes than tasks
      */

      for (nindex = 0;nindex < CRQ->TaskCount;nindex++)
        {
        if (MNLGetNodeAtIndex(&CRQ->NodeList,nindex,NULL) == FAILURE)
          break;
        }
#endif /* __MCPA */
 
#ifdef __MCPA
      UseCPA = TRUE;

      ApplyAllocJobVars = TRUE;
#endif /* __MCPA */

      if ((UseCPA == TRUE) || (R->SubType == mrmstXT4))
        {
        /* user required for APBasil, Catnip, and CPA based partitions */

        if (J->Credential.U == NULL)
          {
          if (EMsg != NULL)
            {
            sprintf(EMsg,"job is corrupt, user not specified");
            }
     
          return(FAILURE);
          }

        /* NOTE:  check for valid UID */
     
        UID = J->Credential.U->OID;
     
        if ((UID < 0) || 
           ((UID == 0) && 
            !bmisset(&J->Flags,mjfSystemJob) && 
            (MSched.AllowRootJobs == FALSE)))
          {
          /* determine if user-to-UID mapping is remote and specified within job environment */
     
          if (J->Env.BaseEnv != NULL)
            {
            char *ptr;
     
            ptr = strstr(J->Env.BaseEnv,"PBS_O_UID=");
     
            if (ptr == NULL) 
              {
              if (EMsg != NULL)
                sprintf(EMsg,"job is corrupt, uid not specified");
 
              return(FAILURE);
              }

            ptr += strlen("PBS_O_UID=");
     
            UID = (int)strtol(ptr,NULL,10);
            }
          }    /* END if ((UID < 0) || (UID == 0) && (MSched.AllowRootJobs == FALSE)) */
     
        if ((UID < 0) || ((UID == 0) && (MSched.AllowRootJobs == FALSE)))
          { 
          if (EMsg != NULL)
            {
            sprintf(EMsg,"job is corrupt, invalid user");
            }
     
          return(FAILURE);
          }
        }    /* END if ((UseCPA == TRUE) || (R->SubType == mrmstXT4)) */
      else
        {
        /* assume Moab+PBSPro in which RM will handle all alloc partition management */

        return(SUCCESS);
        }

#ifndef __MCPA
      /* generate hostlist */
     
      if (MNLIsEmpty(&CRQ->NodeList))
        {
        if (EMsg != NULL)
          {
          sprintf(EMsg,"job is corrupt, invalid compute hostlist");
          }
     
        return(FAILURE);
        }
#endif /* !__MCPA */
     
    
      /* assign partition to job */
     
#ifdef __MCPA
      {
      char *AcctID = NULL;

      int   PPN = 1;        /* NOTE: not really supported w/in CPA, always use 1 */
      int   Flags = 0;      /* NOTE: only allocate compute hosts, always use 0 */

      char *Spec = NULL;;

      mnl_t tmpNodeList;

      if (J->Credential.A != NULL)
        AcctID = J->Credential.A->Name;
 
      NodeReq = NULL;

      MNLInit(&tmpNodeList);

      for (pcindex = 0;pcindex < PCCount;pcindex++)
        {
        if (AssignHostList == TRUE)
          {
          char HostBuf[MMAX_BUFFER << 5];  /* ~2MB - must handle ~200,000 hostnames */
        
          int nindex2;

          rc = nid_list_create(
            0,
            MaxListSize,  /* max count */
            0,
            MaxNID,       /* max value */
            &Wanted);     /* O (alloc) */
     
          if (rc != 0)
            {
            /* FAILURE */
     
            sprintf(EMsg,"nid_list_create failed: rc=%d (%s)\n",
              rc,
              cpa_rc2str(rc));
     
            MDB(1,fSCHED) MLog("ALERT:    cannot create CPA NID list - failure in nid_list_create(0,%d,0,%d,&Wanted)  rc=%d (%s)\n",
              MaxListSize,
              MaxNID,
              rc,
              cpa_rc2str(rc));
     
            MNLFree(&tmpNodeList);

            return(FAILURE);
            }
     
          MDB(5,fSCHED) MLog("INFO:     CPA NID list created (%d of %d)\n",
            pcindex + 1,
            PCCount);

          nindex2 = 0;
            
          for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
            {
            CRQ = J->Req[rqindex];

            if (CRQ == NULL)
              break;

            for (nindex = 0;CRQ->NodeList[nindex].N != NULL;nindex++)
              {
              if ((CRQ->NodeList[nindex].TC * CRQ->DRes.Procs) == PCList[pcindex])
                {
                tmpNodeList[nindex2].N = CRQ->NodeList[nindex].N;
                tmpNodeList[nindex2].TC = CRQ->NodeList[nindex].TC;

                nindex2++;
                }
              }
            }    /* END for (rqindex) */

          PCListSize = nindex2;

          tmpNodeList[nindex2].N = NULL;

          NLPtr = tmpNodeList;

          /* sort in order of node index */
    
          qsort(
            (void *)NLPtr,
            PCListSize,
            sizeof(mnalloc_t),
            (int(*)(const void *,const void *))__MNLNodeIndexCompLToH);
     
          if (MNLToString(
                (mnalloc_t *)NLPtr,
                FALSE,  /* do not include task info */
                ",",
                '\0',
                HostBuf,     /* O */
                sizeof(HostBuf)) == FAILURE)
            {
            /* FAILURE */
     
            sprintf(EMsg,"cannot create hostlist for %d nodes - buffer too small",
              PCListSize);
     
            MDB(1,fSCHED) MLog("ALERT:    cannot create hostlist for %d nodes - failure in MNLToString()\n",
              PCListSize);
     
            MNLFree(&tmpNodeList);

            return(FAILURE);
            }
        
          MDB(6,fSCHED) MLog("INFO:     original CPA hostlist: '%s'\n",
            HostBuf);
       
          rc = nid_list_destringify(HostBuf,Wanted);
     
          if (rc != 0)
            {
            /* FAILURE */
     
            sprintf(EMsg,"nid_list_destringify failed: rc=%d (%s)\n",
              rc,
              cpa_rc2str(rc));
     
            MDB(1,fSCHED) MLog("ALERT:    cannot translate tasklist into NID list - failure in nid_list_destringify((%.32s,Wanted)  rc=%d (%s)\n",
              HostBuf,
              rc,
              cpa_rc2str(rc));
    
            __MCPAFreeNodeReq(NodeReq,Wanted);
 
            MNLFree(&tmpNodeList);

            return(FAILURE);
            }
     
          MDB(5,fSCHED)
            {
            char *buf = NULL;
            int   bufsize = 0;
     
            rc = nid_list_stringify(Wanted,&buf,&bufsize);
     
            if (rc == 0)
              {
              MLog("INFO:     NID list '%.1024s'\n",
                buf);
              }
            else
              {
              MLog("WARNING:  cannot convert NID list to string, failure in CPA nid_list_stringify rc=%d\n",
                rc);
              }
     
            free(buf);
            }  /* END MDB(5,fSCHED) */
          }    /* END if (AssignHostList == TRUE) */
        else
          {
          PCListSize = 0;

          Wanted = NULL;
          }

        PrevNodeReq = NodeReq;

        /* NodeReq is allocated and linked to Wanted */

        NodeReq = cpa_new_node_req(
          PCListSize,       /* number of nodes required by job with specific per node proccount */
          PCList[pcindex],
          Flags,
          Spec,
          Wanted);  /* I */

        if (NodeReq == NULL)
          {
          /* FAILURE:  cannot alloc memory for node req */

          sprintf(EMsg,"cpa_new_node_req() failed\n");

          MDB(1,fSCHED) MLog("ALERT:    cannot alloc memory for node req - failure in cpa_new_node_req(%d,%d,%d,%d,Wanted)\n",
            PCListSize,
            PCList[pcindex],
            Flags,
            Spec);

          __MCPAFreeNodeReq(NodeReq,Wanted);

          MNLFree(&tmpNodeList);

          return(FAILURE);
          }

        if (pcindex > 0)
          NodeReq->next = PrevNodeReq;          
        }  /* END for (pcindex) */

      rc = cpa_create_partition(
        NodeReq,
        CPA_BATCH,
        CPA_NOT_SPECIFIED,
        UID,
        (AcctID != NULL) ? AcctID : "DEFAULT",
        (cpa_partition_id_t *)&ParID,   /* O */
        (cpa_cookie_t *)&AdminCookie,   /* O */
        (cpa_cookie_t *)&AllocCookie);  /* O */

      if (rc != 0)
        {
        /* FAILURE */

        sprintf(EMsg,"cpa_create_partition() failed: rc=%d (%s)\n",
          rc,
          cpa_rc2str(rc));

        MDB(1,fSCHED) MLog("ALERT:    cannot create CPA partition for job %s - failure in cpa_create_partition(%p,%d,%d,%d,%s,...): rc=%d (%s)\n",
          J->Name,
          NodeReq,
          CPA_BATCH,
          CPA_NOT_SPECIFIED,
          UID,
          (AcctID != NULL) ? AcctID : "DEFAULT",
          rc,
          cpa_rc2str(rc));

        if (rc == -28)
          {
          /* CPA_RETRY received - transient failure - try scheduling again */

          MDB(1,fSCHED) MLog("INFO:     CPA retry detected - will re-attempt partition creation in 2 seconds\n");

          MSched.NextUIPhaseDuration = 2;
          }

        __MCPAFreeNodeReq(NodeReq,NULL);

        MNLFree(&tmpNodeList);

        return(FAILURE);
        }

      rc = cpa_assign_partition(
        (cpa_partition_id_t)ParID,
        (cpa_cookie_t)AdminCookie,
        J->Name,
        1);     /* NOT CURRENTLY USED - should be set to NID of 'master host' */

      /* free memory, nid list no longer required */

      __MCPAFreeNodeReq(NodeReq,NULL);

      if (rc != 0)
        {
        /* FAILURE */

        sprintf(EMsg,"cpa_assign_partition() failed: rc=%d (%s)\n",
          rc,
          cpa_rc2str(rc));

        MDB(1,fSCHED) MLog("ALERT:    cannot assign job to CPA partition - failure in cpa_assign_partition(%lu,%llu,%s,1): rc=%d (%s)\n",
          ParID,
          AdminCookie,
          J->Name,
          rc,
          cpa_rc2str(rc));

        MJobDestroyAllocPartition(J,R,"create failed");

        MNLFree(&tmpNodeList);

        return(FAILURE);
        }
      }    /* END BLOCK */
#else /* __MCPA */

      if ((R->SubType == mrmstXT4) && (R->ND.Path[mrmXParCreate] != NULL))
        {
        /* assume native interface based specification */
        int reqIndex;

        char    RsvID[MMAX_LINE];

        char    PEMsg[MMAX_LINE];

        mstring_t tmpCmd(MMAX_LINE);
        mstring_t OBuf(MMAX_LINE);
        mstring_t HostBuf(MMAX_BUFFER);  /* must handle ~200,000 hostnames */
        mstring_t tmpHostBuf(MMAX_BUFFER);

        /* sort in order of node index */

        ApplyAllocJobVars = TRUE;

        for (reqIndex = 0;(J->Req[reqIndex] != NULL) && (reqIndex < MMAX_REQ_PER_JOB);reqIndex++)
          {
          if (MNLIsEmpty(&J->Req[reqIndex]->NodeList))
            continue;

          /* Sort nodes one more time according to the priority of the nodes to
           * get the correct nid ordering for mpi. Nodes could be out of order
           * if they were selected through different affinities. */
          if (MPar[0].NAllocPolicy == mnalMachinePrio) 
            MNLSort(&J->Req[reqIndex]->NodeList,J,MSched.Time,mnalMachinePrio,NULL,FALSE);
          
          MStringSet(&tmpHostBuf,"");

          MUNLToRangeString(
            &J->Req[reqIndex]->NodeList,
            NULL,
            -1,
            TRUE,
            FALSE, /* only compress ascending order nodes */
            &tmpHostBuf);
          
          /* If dest not empty, add delimiter then add new item */
          if (!HostBuf.empty())
            {
            HostBuf += '+';
            }
          HostBuf += tmpHostBuf;
          }  /* END for (reqIndex) */

        /* MUTaskMapToString(J->TaskMap,NULL,'\0',HostBuf,sizeof(HostBuf)); */

        MStringSetF(&tmpCmd,"%s --reserve --rm %s -j %s -t %s -u %s",
          R->ND.Path[mrmXParCreate],
          R->Name,
          J->DRMJID != NULL ? J->DRMJID : J->Name,
          HostBuf.c_str(),
          J->Credential.U->Name);

        MUReadPipe2(
          tmpCmd.c_str(),
          NULL,
          &OBuf,
          NULL,
          &R->P[0],
          &exitCode,    /* O */
          PEMsg,        /* O */
          NULL);

        if (exitCode != 0)
          {
          sprintf(tmpLine,"alloc partition command '%s' failed - %s",
            R->ND.Path[mrmXParCreate],
            (PEMsg[0] != '\0') ? PEMsg : "external failure");

          if (!OBuf.empty())
            {
            strcat(tmpLine," with output: ");
            strcat(tmpLine,OBuf.c_str());
            }

          if (EMsg != NULL)
            strcpy(EMsg,tmpLine);

          MDB(1,fNAT) MLog("ALERT:    %s\n", 
            tmpLine);

          if (SC != NULL)
            {
            /* determine if failure is transient (NYI) */

            *SC = 1;
            }

          return(FAILURE);
          }  /* END if (exitCode != 0) */

        MUStrCpy(RsvID,OBuf.c_str(),sizeof(RsvID));

        ParID = strtol(RsvID,NULL,10);

        AdminCookie = 1;
        AllocCookie = 0;
        }  /* END if ((R->SubType == mrmstXT4) && (R->ND.Path[mrmXParCreate] != NULL)) */
      else
        {
        /* FAILURE - no alloc partition interface specified */

        sprintf(EMsg,"no alloc partition interface specified\n");

        return(FAILURE);
        }
 
      /* MDB(1,fSCHED) MLog("ALERT:    CPA module not enabled on XT3 - job partition not created\n"); */
#endif /* else __MCPA */

      /* Store added env variables and push only added vars to torque -- not
       * the whole env. */

      /* save the partition and cookies in job variables */
    
      if (ApplyAllocJobVars == TRUE)
        { 
        char *AllocPar = NULL;

        static mbool_t HaveFixedCrayALPSBug = FALSE;

        if (HaveFixedCrayALPSBug == FALSE)
          {
          /* NOTE:  ALPS can be restarted and lose all info on existing partition ids - it
                    will then re-use these ids causing collisions.  If jobs with partition
                    ids largers than the one we just created exist, they are invalid and partition
                    info for these jobs should be cleared.

                    This only needs to be done once at start-up.
          */

          mjob_t *tmpJ;

          job_iter JTI;

          mulong tmpUL;

          MJobIterInit(&JTI);

          /* walk all jobs - look for jobs with ParID >= X */

          while (MJobTableIterate(&JTI,&tmpJ) == SUCCESS)
            {
            if (MUHTGet(&J->Variables,"ALLOCPARTITION",(void **)&AllocPar,NULL) == FAILURE)
              continue;

            tmpUL = strtoul(AllocPar,NULL,10);

            if (tmpUL >= ParID)
              {
              /* if found, remove variable from job */

              MUHTRemove(&tmpJ->Variables,"ALLOCPARTITION",MUFREE);
              }
            }  /* END for (tmpJ) */

          HaveFixedCrayALPSBug = TRUE;
          }  /* END if (HaveFixedCrayALPSBug == FALSE) */

        sprintf(tmpLine,"%lu",
          ParID);
     
        if (MUHTAdd(&J->Variables,"ALLOCPARTITION",strdup(tmpLine),NULL,MUFREE) == FAILURE)
          {
          MDB(1,fSCHED) MLog("ERROR:    cannot set ALLOCPARTITION variable for job %s (no memory)\n",
            J->Name);

          return(FAILURE);
          }
 
        if ((MUHTGet(&J->Variables,"ALLOCPARTITION",(void **)AllocPar,NULL) == FAILURE))
          {
          /* cannot locate rsv id that we just created */

          MDB(2,fSCHED) MLog("ERROR:    cannot locate BASIL RSVID (job 'ALLOCPARTITION' variable) that we just created\n");
          
          return(FAILURE);
          }

        MJobAddEnvVar(J,"BATCH_PARTITION_ID",tmpLine,',');

        MStringAppendF(&NewEnvVariables,"%s%s=%s",
            (!NewEnvVariables.empty()) ? "," : "",
            "BATCH_PARTITION_ID",
            tmpLine);
     
        sprintf(tmpLine,"%llu",
          AdminCookie);
     
        MUHTAdd(&J->Variables,"ALLOCADMINCOOKIE",strdup(tmpLine),NULL,MUFREE);
     
        sprintf(tmpLine,"%llu",
          AllocCookie);
        }  /* END if (ApplyAllocJobVars == TRUE) */
 
      MJobAddEnvVar(J,"BATCH_ALLOC_COOKIE",tmpLine,',');

      MStringAppendF(&NewEnvVariables,"%s%s=%s",
          (!NewEnvVariables.empty()) ? "," : "",
          "BATCH_ALLOC_COOKIE",
          tmpLine);

      MJobAddEnvVar(J,"BATCH_JOBID",J->Name,',');

      MStringAppendF(&NewEnvVariables,"%s%s=%s",
          (!NewEnvVariables.empty()) ? "," : "",
          "BATCH_JOBID",
          J->Name);

      /* NOTE:  may need to add 'ENVIRONMENT=BATCH' (or 'PBS_ENVIRONMENT=PBS_BATCH') */
     
      /* NOTE:  may need to add 'PBS_PARTITION=x,PBS_ALLOC_COOKIE=x,PBS_JOBID=x' */

      /* push added environment to RM */

      if (R->Version >= 253)
        {
        /* Torque 2.5.3 supports incremental additions. Which is needed if the
         * job wasn't been migrated prior to scheduling because the job's
         * Variable_List will be clobbered. (MOAB-137) */

        rc = MRMJobModify(
              J,
              "Variable_List",
              NULL,
              NewEnvVariables.c_str(),
              mIncr,
              NULL,
              tEMsg,
              NULL);
        }
      else
        {
        rc = MRMJobModify(
              J,
              "Variable_List",
              NULL,
              J->Env.BaseEnv,
              mSet,
              NULL,
              tEMsg,
              NULL);
        }
          
      if (rc == FAILURE)
        {
        /* FAILURE */
     
        MDB(2,fSCHED) MLog("INFO:     cannot modify environment for rm job %s - '%s'\n",
          J->Name,
          tEMsg);
     
        if (EMsg != NULL)
          strcpy(EMsg,tEMsg);
     
        MJobDestroyAllocPartition(J,R,"create failed");
     
        return(FAILURE);
        }  /* END if (rc == FAILURE) */

      bmset(&J->IFlags,mjifAllocPartitionCreated);

      break;

    default:

      {
      mstring_t tmpBuf(MMAX_LINE); 

      /* push environment into job */

      if (IsBProcBased == TRUE)
        {
        MJobAddEnvVar(
          J,
          "BEOWULF_JOB_MAP",
          MUTaskMapToMString(J->TaskMap,":",'.',&tmpBuf),
          ',');
        }

      /* comma is env var delimiter for PBS, must backslash */

      /* push NODES env var down to PBS - thus requires backslash delimiter */

      /* NOTE:  must specify list of allocated nodes, not tasklist */

      MStringSet(&tmpBuf,"");

      MNLToMString(
        &J->NodeList,
        FALSE,
        "\\,",
        '.',
        -1,
        &tmpBuf);

      MJobAddEnvVar(
        J,
        "NODES",
        tmpBuf.c_str(),
        ',');

      rc = MRMJobModify(
             J,
             "Variable_List",
             NULL,
             J->Env.BaseEnv,
             mSet,
             NULL,
             tEMsg,
             NULL);

      if (rc == FAILURE)
        {
        /* FAILURE */

        MDB(2,fSCHED) MLog("INFO:     cannot modify environment for rm job %s - '%s'\n",
          J->Name,
          tEMsg);

        if (EMsg != NULL)
          strcpy(EMsg,tEMsg);

        MJobDestroyAllocPartition(J,R,"create failed");

        return(FAILURE);
        }  /* END if (rc == FAILURE) */
      }    /* END BLOCK (case mrmstTM) */

      break;
    }      /* END switch (R->SubType) */

  return(SUCCESS);
  }  /* END MJobCreateAllocPartition() */





#ifdef __MCPA
int __MCPAFreeNodeReq(

  cpa_node_req_t *NReq,    /* I (free'd,optional) */
  cpa_nid_list_t  Wanted)  /* I (free'd,optional) */

  {
  cpa_node_req_t *NPtr;

  if (Wanted != NULL)
    {
    nid_list_destroy(Wanted);
    }

  if (NReq != NULL)
    {
    for (NPtr = NReq;NPtr != NULL;NPtr = NPtr->next)
      {
      if (NPtr->wanted != NULL)
        nid_list_destroy(NPtr->wanted);
      }  /* END for (NPtr) */

    cpa_free_node_reqs(NReq);
    }

  return(SUCCESS);
  }  /* END __MCPAFreeNodeReq() */
#endif /* __MCPA */






/**
 * Destroy allocation partition associated with job
 *
 * NOTE:  see MJobCreateAllocPartition() - peer - creates new alloc partition
 *
 * @param J (I) [modified]
 * @param R (I)
 * @param Msg (I) [optional]
 */

int MJobDestroyAllocPartition(

  mjob_t     *J,
  mrm_t      *R,
  const char *Msg)

  { 
  int rc;

  const char *FName = "MJobDestroyAllocPartition";

  MDB(3,fSCHED) MLog("%s(%s,%s,%s)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (R != NULL) ? R->Name : "NULL",
    (Msg != NULL) ? Msg : "NULL");

  if ((J == NULL) || (R == NULL))
    {
    return(FAILURE);
    }

  if ((MSched.Mode != msmNormal) &&
      (MSched.Mode != msmSlave))
    {
    return(SUCCESS);
    }

  bmunset(&J->IFlags,mjifAllocPartitionCreated);

#ifdef __MCPA
  {
  mln_t *tmpL;

  int ErrorP = 0;

  mulong             ParID;
  unsigned long long AdminCookie;

  if ((MULLCheck(J->Variables,"ALLOCPARTITION",&tmpL) == FAILURE) ||
      (tmpL->Ptr == NULL))
    {
    /* cannot locate partition */

    MDB(2,fSCHED) MLog("ALERT:    cannot locate alloc partition (%s)\n",
      (tmpL != NULL) ? "empty" : "not found");

    return(FAILURE);
    }

  ParID = strtoul((char *)tmpL->Ptr,NULL,10);

  if ((MULLCheck(J->Variables,"ALLOCADMINCOOKIE",&tmpL) == FAILURE) ||
      (tmpL->Ptr == NULL))
    {
    /* cannot locate partition */

    MDB(2,fSCHED) MLog("ALERT:    cannot locate alloc admin cookie (%s)\n",
      (tmpL != NULL) ? "empty" : "not found");

    return(FAILURE);
    }

  AdminCookie = strtoull((char *)tmpL->Ptr,NULL,10);

  MDB(2,fSCHED) MLog("INFO:     destroying CPA partition %lu for job %s with cookie %llu - %s\n",
    ParID,
    J->Name,
    AdminCookie,
    (Msg != NULL) ? Msg : "no details");

  /* cpa_destroy_partition() will fail if yod is present */

  rc = cpa_destroy_partition(
    ParID,
    AdminCookie,
    &ErrorP);      /* O - if set, destroy failed on one or more tasks */

  if (rc != 0)
    {
    char tmpLine[MMAX_LINE];

    MDB(1,fSCHED) MLog("ALERT:    cannot destroy CPA partition %lu for job %s - cpa_destroy_partition(%lu,%llu,ErrorP) failed: rc=%d (%s)\n",
      ParID,
      J->Name,
      ParID,
      AdminCookie,
      rc,
      cpa_rc2str(rc));

    snprintf(tmpLine,sizeof(tmpLine),"ALERT:    cannot destroy CPA partition %lu for job %s - cpa_destroy_partition(%lu,%llu,ErrorP) failed: rc=%d (%s)\n",
      ParID,
      J->Name,
      ParID,
      AdminCookie,
      rc,
      cpa_rc2str(rc));

    /* send email to admins */
  
    MSysSendMail(
      MSysGetAdminMailList(1),
      NULL,
      "moab cannot remove CPA partition",
      NULL,
      tmpLine);

    bmset(&J->IFlags,mjifStalePartition);

    return(FAILURE);
    }

  MULLRemove(&J->Variables,"ALLOCPARTITION",MUFREE);
  MULLRemove(&J->Variables,"ALLOCADMINCOOKIE",MUFREE);
  }    /* END BLOCK */
#else  /* __MCPA */
  {
  /* assume APBasil interface */

  char  EMsg[MMAX_LINE];

  char  tmpBuf[MMAX_LINE];

  char *RsvID = NULL;

  if (MUHTGet(&J->Variables,"ALLOCPARTITION",(void **)&RsvID,NULL) == FAILURE)
    {
    MDB(2,fSCHED) MLog("ALERT:    cannot locate BASIL RSVID (job 'ALLOCPARTITION' variable)\n");

    return(FAILURE);
    }

  if (MUHTGet(&J->Variables,"ALLOCADMINCOOKIE",NULL,NULL) == FAILURE)
    {
    /* cannot locate cookie */

    /* NOTE:  cookie not required - do not return failure? */

    MDB(2,fSCHED) MLog("ALERT:    cannot locate BASIL COOKIE\n");

    /* return(FAILURE); */
    }

  if (R->ND.Path[mrmXParDestroy] != NULL) 
    {
    mstring_t OBuf(MMAX_LINE);
    mstring_t EBuf(MMAX_LINE);

    snprintf(tmpBuf,sizeof(tmpBuf),"%s --rm %s -p %s",
      R->ND.Path[mrmXParDestroy],
      R->Name,
      RsvID);


    rc = MUReadPipe2(
      tmpBuf,
      NULL,
      &OBuf,
      &EBuf,
      &R->P[0],
      NULL,
      EMsg,
      NULL);

    if (rc == FAILURE)
      {
      MDB(1,fSCHED) MLog("ALERT:    job partition not destroyed - EBuf=%s  EMsg=%s\n",
        EBuf.c_str(),
        EMsg);

      return(FAILURE);
      }
    }  /* END if (R->ND.Path[mrmXParDestroy] != NULL) */
  else
    {
    MDB(1,fSCHED) MLog("ALERT:    job partition not destroyed - partition destroy URL not specified\n");

    return(FAILURE);
    }

  /* NOTE:  check response - NYI */

  MDB(3,fSCHED) MLog("INFO:     alloc partition for job %s successfully destroyed\n",
    J->Name);

  MUHTRemove(&J->Variables,"ALLOCPARTITION",MUFREE);
  MUHTRemove(&J->Variables,"ALLOCADMINCOOKIE",MUFREE);
  }    /* END #else __MCPA */
#endif /* __MCPA */

#ifdef __MCPA
  /* With alps, a busy node shouldn't be put back to idle after destroying a
   * reservation because the Node Health Checker (NHC) can run for a while.
   * Moab shouldn't schedule a new job on the node until it is free. */
   
  if ((MJOBISCOMPLETE(J) == TRUE) && (!MNLIsEmpty(&J->NodeList)))
    {
    int nindex;

    mnode_t *N;

    /* NOTE:  clear allocated nodes from reserved/busy (indicating partition reservation)
              to idle */

    for (nindex = 0;MNLGetNodeAtIndex(&J->NodeList,nindex,&N) == SUCCESS;nindex++)
      {
      if ((N->State == mnsBusy) && 
         ((R->SubType == mrmstXT4) || ((N->SubState != NULL) && (!strcasecmp(N->SubState,"cpabusy")))))
        {
        if (N->ARes.Procs == 0)
          {
          N->ARes.Procs = N->CRes.Procs;

          if (nindex == 0)
            MDB(3,fSCHED) MLog("INFO:     allocation partition for job %s released - adjusting node resources\n",
              J->Name);
          }

        N->State  = mnsIdle;
        N->EState = mnsIdle;
        }
      }    /* END for (nindex) */
    }      /* END if ((MJOBISCOMPLETED(J) == TRUE) && (J->NodeList != NULL)) */
#endif /* __MCPA */

  return(SUCCESS);
  }  /* END MJobDestroyAllocPartition() */
/* END MJobAllocPartition.c */
