/* HEADER */

      
/**
 * @file MWikiSLURM.c
 *
 * Contains: MWiki SLURM Functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  

#include "MWikiInternal.h"


/**
 * Load SLURM-specific partition attributes.
 *
 * @param R (I) [modified]
 */

int MSLURMLoadParInfo(

  mrm_t *R)  /* I (modified) */

  {
  char Buf[MMAX_BUFFER << 2];
  char tmpLine[MMAX_LINE];

  char *PPtr;
  char *PNext;
  char *TokPtr;
  char *NListPtr;
  char *NIListPtr = NULL;
  char *SharedPtr;
  char *DefPtr;

  char *ptr;
  char *tmpPtr;

  mpar_t   *P;
  mclass_t *C;

  long      MaxTime;
  int       MaxNC;
  int       MinNC;

  unsigned long    BoundedLimit;

  mbool_t MapParToClass;
  mbool_t DefaultNADedicated;

  enum MNodeAccessPolicyEnum NAPolicy = mnacNONE;

  mbool_t  IsDefaultPartition = MBNOTSET;
 
  mpar_t  *MoabPartition;  /* the partition managed by this rm (as defined by moab--not slurm) */
  int      pindex;

  const char *FName = "MSLURMLoadParInfo";

  MDB(2,fWIKI) MLog("%s(%s)\n",
    FName,
    (R != NULL) ? R->Name : "NULL");

  if (R == NULL)
    {
    return(FAILURE);
    }

  MoabPartition = (MSched.PerPartitionScheduling == TRUE) ? &MPar[R->PtIndex] : NULL;
  pindex = (MSched.PerPartitionScheduling == TRUE) ? R->PtIndex : 0;

  /* if there are multiple SLURM RMs, we need to be sure to access the correct config via scontrol */
  MUSetEnv("SLURM_CONF",R->ConfigFile);

  if (MUReadPipe("scontrol show partition --oneliner",NULL,Buf,sizeof(Buf),NULL) == FAILURE)
    {
    MMBAdd(
      &R->MB,
      "cannot execute scontrol - check path",
      NULL,
      mmbtNONE,
      0,
      0,
      NULL);

    return(FAILURE);
    }

  if (R->Version >= 10000)
    MapParToClass = TRUE;
  else
    MapParToClass = FALSE;

  /* FORMAT:  "PartitionName=<X> ...
                 Default={YES|NO} Shared={YES|NO|FORCED|EXCLUSIVE} ...
                 ...
                 Nodes=<N1>,<N2>... NodeIndicies=0,1,...,-1
  */
  
  PPtr = strstr(Buf,"PartitionName=");
  
  while (PPtr != NULL)
    {
    PNext = strstr(PPtr + 1,"PartitionName=");
  
    /* NOTE:  use whitespace in search to avoid MinNodes=, MaxNodes=, and TotalNodes= */

    if (R->IsBluegene)
      {
      NListPtr = strstr(PPtr," BasePartitions=");

      if (R->Version < 20000)
        {
        NIListPtr = strstr(PPtr,"BPIndices=");
        }
      }
    else
      {
      NListPtr = strstr(PPtr," Nodes=");
      NIListPtr = strstr(PPtr,"NodeIndices=");
      }

    if (R->Version < 20000)
      {
      NIListPtr = strstr(PPtr,"NodeIndices=");
      }
  
    /* Note that slurm releases after 2.0 no longer send the NodeIndices= */

    if ((NListPtr == NULL) || ((R->Version < 20000) && (NIListPtr == NULL)) || 
       ((PNext != NULL) && ((NListPtr > PNext) || ((R->Version < 20000) && (NIListPtr > PNext)))))
      {
      /* partition is corrupt - ignore partition */

      PPtr = PNext;

      continue;
      }

    /* determine partition max time, FORMAT:  UNLIMITED||<MINUTES> */

    MaxTime = -1;

    ptr = strstr(PPtr," MaxTime=");

    if (ptr != NULL)
      {
      if (R->Version >= 10300)
        {
        /* v1.3 FORMAT: min:sec | hour:min:sec | day-hour:min:sec | UNLIMITED */

        ptr += strlen(" MaxTime=");

        tmpPtr = MUStrChr(ptr,' ');

        MUStrCpy(tmpLine,ptr,(tmpPtr + 1) - ptr);
        
        if (!strncmp(tmpLine,"UNLIMITED",sizeof("UNLIMITED")))
          MaxTime = 0;
        else
          MaxTime = MWikiParseTimeLimit(tmpLine);
        }
      else
        {
        MaxTime = strtol(ptr + strlen(" MaxTime="),NULL,10);
        }
      }
#if 0 /* getting rid of this, it effectively makes it so that you can't set
         MAX.WCLIMIT in Moab as setting MaxTime to 0 removes the setting in 
         moab  (RT7065) */
    else
      MaxTime = 0;  /* reset WCLimit */
#endif

    /* determine partition min nodes, FORMAT:  UNLIMITED||<INTEGER> */

    ptr = strstr(PPtr," MinNodes=");

    if (ptr != NULL)
      {
      ptr += strlen(" MinNodes=");

      tmpPtr = MUStrChr(ptr,' ');

      MUStrCpy(tmpLine,ptr,(tmpPtr + 1) - ptr);

      MinNC = MUExpandKNumber(tmpLine);
      }
    else
      MinNC = 1;

    /* determine partition max nodes, FORMAT:  UNLIMITED||<INTEGER> */

    ptr = strstr(PPtr," MaxNodes=");

    if (ptr != NULL)
      {
      ptr += strlen(" MaxNodes=");

      tmpPtr = MUStrChr(ptr,' ');

      MUStrCpy(tmpLine,ptr,(tmpPtr + 1) - ptr);

      if (!strncmp(tmpLine,"UNLIMITED",sizeof("UNLIMITED")))
        MaxNC = 0;
      else
        MaxNC = MUExpandKNumber(tmpLine);
      }
    else
      MaxNC = 0;

    DefPtr = strstr(PPtr,"Default=YES");

    if ((DefPtr != NULL) && ((PNext == NULL) || (DefPtr < PNext)))
      IsDefaultPartition = TRUE;
    else
      IsDefaultPartition = FALSE;

    DefaultNADedicated = FALSE;

    SharedPtr = strstr(PPtr,"Shared=");

    if ((SharedPtr != NULL) && ((PNext == NULL) || (SharedPtr < PNext)))
      {
      /* The SLURM SelectType is checked in MSLURMInitialize() and the MPar[0].SLURMISShared is set accordingly.
       *
       * ====================================================================
       * |              |              SelectType =                         |
       * |              |                                                   | 
       * | shared=      | linear          select/cons_res   select/bluegene |
       * ====================================================================
       * | Exclusive    | singlejob      | singlejob       | singlejob      |
       * --------------------------------------------------------------------
       * | No           | singlejob      | shared          | singlejob      |
       * --------------------------------------------------------------------
       * | Yes          | singlejob (1)  | shared          | singlejob (1)  |
       * --------------------------------------------------------------------
       * | Force        | shared (2)     | shared          | shared         |
       * ====================================================================
       *
       * (1) - This will be correct unless users requests include the
       * "--exclusive" option, which should be rare in practice
       * (2) - While Slurm will be scheduling nodes, Moab will schedule the
       * individual processors and avoid over-subscribing them
       */

      SharedPtr += strlen("Shared=");

      if (!strncasecmp(SharedPtr,"EXCLUSIVE",strlen("EXCLUSIVE")))
        {
        NAPolicy = mnacSingleJob;
        }
      else if (bmisset(&MPar[0].Flags,mpfSLURMIsShared))
        {
        NAPolicy = mnacShared;
        }
      else if (!strncasecmp(SharedPtr,"FORCE",strlen("FORCE")))
        {
        NAPolicy = mnacShared;
        }
      else if (!strncasecmp(SharedPtr,"YES",strlen("YES")))
        {
        /* NOTE:  with YES, node is only shared if job explicitly states it is
                  shared using '-s' or '--shared'.  Currently, SLURM 1.2.1 and 
                  lower do not provide this info to Moab.  For now, mark all 
                  nodes as single job.  Change when SLURM passes this info via
                  the WIKI interface */

        if (R->Version >= 10202)
          {
          DefaultNADedicated = TRUE;
          }
        else
          {
          NAPolicy = mnacSingleJob;
          }
        }
      else
        {
        /* sharing is not allowed regardless of per-job specification */

        NAPolicy = mnacSingleJob;

        /* NOTE:  should set RM flag 'nosharing' to prevent user specified 
                  sharing (NYI) */
        }
      }    /* END if ((SharedPtr != NULL) && ...) */

    /* extract partition name */

    ptr = MUStrTok(PPtr + strlen("PartitionName=")," \t\n",&TokPtr);

    if (ptr == NULL)
      {
      /* output is corrupt */

      break;
      }      

    if (MapParToClass == TRUE)
      {
      if (MClassAdd(ptr,&C) == FAILURE)
        {
        /* cannot add new class */

        MDB(1,fWIKI) MLog("ALERT:    cannot add SLURM partition '%s' as class\n",
          ptr);

        break;
        }

      if (DefaultNADedicated == TRUE)
        C->NAIsDefaultDedicated = TRUE;
     
      if (IsDefaultPartition == TRUE)
        {
        mrm_t *IRM;
        int rmcount;

        /* only apply defaults if there is a single SLURM RM */
        /* FIXME: we really should be applying the DefaultC on a per RM basis,
         * NOT on the internal RM! */

        MRMGetComputeRMCount(&rmcount);

        if (rmcount == 1)
          {
          if ((MRMGetInternal(&IRM) == SUCCESS) &&
              (IRM->DefaultC == NULL))
            {
            IRM->DefaultC = C;
            }
          }
        }

      if (NAPolicy != mnacNONE)
        {
        mreq_t *RQ;

        if (C->L.JobSet != NULL)
          {
          RQ = C->L.JobSet->Req[0];

          MDB(7,fWIKI) MLog("INFO:     class %s node access policy already set to %s\n",
            C->Name,
            MNAccessPolicy[RQ->NAccessPolicy]);
          }
        else
          {
          RQ = NULL;
          }

        if ((RQ == NULL) ||
           ((RQ->NAccessPolicy == mnacNONE) ||
            (RQ->NAccessPolicy == mnacShared)) )
          {
          MDB(7,fWIKI) MLog("INFO:     setting class %s node access policy to %s in partition load\n",
            C->Name,
            MNAccessPolicy[NAPolicy]);

          MClassSetAttr(
            C,
            MoabPartition,
            mclaNAPolicy,
            (void **)MNAccessPolicy[NAPolicy],
            mdfString,
            mSet);
          }
        }

      /* Truncate the RM MAX.NODE count if it is outside of RMLimit bounds */
      if ((MSched.Mode != msmSlave) &&                     /* slaves accept all changes */
          (MClassGetEffectiveRMLimit(C,pindex,mcaMaxNodePerJob,&BoundedLimit) == SUCCESS) && 
          (MaxNC > (int)BoundedLimit))   /* bounds checking */
        {
        MDB(3,fRM) MLog("WARNING:  truncating MAX.NODE for class:%s, (rm reports:%d  Moab enforces:%d)\n",
                        C->Name,
                        MaxNC,
                        BoundedLimit);
        MaxNC = BoundedLimit;
        }

      if (MaxNC > 0)
        {
        MClassSetAttr(
          C,
          MoabPartition,
          mclaMaxNode,
          (void **)&MaxNC,
          mdfInt,
          mSet);
        }

      /* Truncate the RM MIN.NODE count if it is outside of RMLimit bounds */
      if ((MSched.Mode != msmSlave) &&                     /* slaves accept all changes */
          (MClassGetEffectiveRMLimit(C,pindex,mcaMinNodePerJob,&BoundedLimit) == SUCCESS) && 
          (MinNC < (int)BoundedLimit))   /* bounds checking */
        {
        MDB(3,fRM) MLog("WARNING:  truncating MIN.NODE for class:%s, (rm reports:%d  Moab enforces:%d)\n",
                        C->Name,
                        MinNC,
                        BoundedLimit);
        MinNC = BoundedLimit;
        }

      if (MinNC >= 0)
 
        {
        if (MinNC == 0)
          MinNC = 1;  /* a min of 0 is a silly value */

        MClassSetAttr(
          C,
          MoabPartition,
          mclaMinNode,
          (void **)&MinNC,
          mdfInt,
          mSet);
        }

      /* convert to seconds */

      /* Truncate the RM max time count if it is outside of RMLimit bounds */
      if ((MSched.Mode != msmSlave) &&                            /* slaves accept all changes */
          (MClassGetEffectiveRMLimit(C,pindex,mcaMaxWCLimitPerJob,&BoundedLimit) == SUCCESS) && 
          ((mutime)MaxTime > BoundedLimit))
        {
        MDB(3,fRM) MLog("WARNING:  truncating max time for class:%s, (rm reports:%d  Moab enforces:%d)\n",
                        C->Name,
                        (mutime)MaxTime,
                        BoundedLimit);
        MaxTime = BoundedLimit;
        }

      if (MaxTime > 0)
        {
        if ((R->Version < 10300) && (MaxTime < MCONST_EFFINF))
          MaxTime *= MCONST_MINUTELEN;

        /* NOTE: if MaxTime is zero, then we will be effectively reseting the MaxWCLimit */

        MCredSetAttr(
          (void *)C,
          mxoClass,
          mcaMaxWCLimitPerJob,
          MoabPartition,
          (void **)&MaxTime,
          NULL,
          mdfLong,
          mSet);
        }

      bmunset(&C->Flags,mcfRemote);
      }  /* END else if (MapParToClass == TRUE) */
    else
      {
      if (DefaultNADedicated == TRUE)
        bmset(&R->IFlags,mrmifDefaultNADedicated);

      if (MParAdd(ptr,&P) == FAILURE)
        {
        /* cannot add new partition */

        MDB(1,fWIKI) MLog("ALERT:    cannot add SLURM partition '%s'\n",
          ptr);

        break;
        }

      if (IsDefaultPartition == TRUE)
        {
        if ((NAPolicy != mnacNONE) &&
           ((MSched.DefaultNAccessPolicy == mnacNONE) ||
            (MSched.DefaultNAccessPolicy == mnacShared)))
          {
          MSched.DefaultNAccessPolicy = NAPolicy;
          }
        }    /* END if (IsDefaultPartition == TRUE) */
      }      /* END else ... */

    PPtr = PNext;

    }    /* END while (PPtr != NULL) */

  return(SUCCESS);
  }  /* END MSLURMLoadParInfo() */





/**
 * Gets Slurm's Version
 *
 * @param R (I) Resource Manager Object
 *
 * @return Returns SUCCESS Regardless
 */

int MSLURMGetVersion(

    mrm_t *R) /*I*/

  {
  char Buf[MMAX_BUFFER << 4];

  /* if there are multiple SLURM RMs, we need to be sure to access the correct config via scontrol */
  MUSetEnv("SLURM_CONF",R->ConfigFile);

  /* determine version - NOTE:  MUReadPipe() can succeed even if scontrol fails or is not found */

  if ((MUReadPipe("scontrol --version",NULL,Buf,sizeof(Buf),NULL) == SUCCESS) && (Buf[0] != '\0'))
    {
    char *ptr;
    char *TokPtr;

    /* FORMAT:  slurm <MajorVersion>.<MinorVersion>.<SubVersion> */

    /* remove terminating '\n' */

    MUStrTok(Buf,"\n",&TokPtr);

    MUStrCpy(R->APIVersion,Buf,sizeof(R->APIVersion));

    /* remove 'slurm' */

    ptr = MUStrTok(Buf," \t\n.",&TokPtr);

    /* get major, minor, and sub version numbers */

    ptr = MUStrTok(NULL," \t\n.",&TokPtr);

    if (ptr != NULL)
      {
      R->Version += (int)strtol(ptr,NULL,10) * 10000;
      }

    ptr = MUStrTok(NULL," \t\n.",&TokPtr);

    if (ptr != NULL)
      {
      R->Version += MIN(9,(int)strtol(ptr,NULL,10)) * 100;
      }

    ptr = MUStrTok(NULL," \t\n.",&TokPtr);

    /* NOTE:  slurm version X.Y.Z is translated to Wiki version XXYYZZ */

    if (ptr != NULL)
      {
      R->Version += MIN(99,(int)strtol(ptr,NULL,10));
      }
    }    /* END if (MUReadPipe("scontrol --version") == SUCCESS)  */
  else
    {
    char tmpLine[MMAX_LINE];

    /* version not specified and version not detected */

    R->Version = MDEF_SLURMVERSION;

    snprintf(tmpLine,sizeof(tmpLine),"SLURM version not specified/detected, assuming %s",
      MDEF_SLURMVERSIONSTRING);

    MMBAdd(
      &R->MB,
      tmpLine,
      NULL,
      mmbtNONE,
      0,
      0,
      NULL);
    }

  return(SUCCESS);
  } /* END MSLURMGetVersion */





/**
 * Initializes SLURM RM
 *
 * @see MWikiInitialize() - parent
 *
 * @param R (I) Resource Manager Object
 * @return Returns Success Regardless
 */

int MSLURMInitialize(

  mrm_t *R) /*I*/

  {
  char Buf[MMAX_BUFFER << 4];
  char tmpLine[MMAX_LINE];

  /* if there are multiple SLURM RMs, we need to be sure to access the correct config via scontrol */
  MUSetEnv("SLURM_CONF",R->ConfigFile);

  if (MUReadPipe("scontrol show config --oneliner",NULL,Buf,sizeof(Buf),NULL) == SUCCESS)
    {
    char *ptr;
    char *TokPtr;

    /* look of SelectType = select/bluegene */

    if ((ptr = strstr(Buf,"SelectType")) != NULL)
      {
      if (strstr(ptr,"select/bluegene") != NULL) 
      /* if ((strstr(Buf,"select/bluegene") != NULL) &&
          (strstr(Buf,"HostFormat=2") != NULL)) */
        {
        R->IsBluegene = TRUE; 

        MSched.BluegeneRM = TRUE;

        MSched.BGNodeCPUCnt = 1;           /* Default */
        MSched.BGBasePartitionNodeCnt = 1; /* Default */

        /* Get extra information to be able to print out correct cnode counts. */

        if ((ptr = strstr(Buf,"NodeCPUCnt")) != NULL) 
          {
          /* FORMAT: NodeCPUCnt\s+= # */
          
          ptr += strlen("NodeCPUCnt");

          MUStrCpy(tmpLine,ptr,sizeof(tmpLine));

          /* select '=' */

          ptr = MUStrTok(tmpLine," \t\n",&TokPtr);

          /* select value */

          ptr = MUStrTok(NULL," \t\n",&TokPtr);

          MSched.BGNodeCPUCnt = (int)strtol(ptr,NULL,10);
          } /* END if ((ptr = strstr(Buf,"NodeCPUCnt")) != NULL) */

        if ((ptr = strstr(Buf,"BasePartitionNodeCnt")) != NULL) 
          {
          /* FORMAT: BasePartitionNodeCnt\s+= # */

          ptr += strlen("BasePartitionNodeCnt");

          MUStrCpy(tmpLine,ptr,sizeof(tmpLine));

          /* select '=' */

          ptr = MUStrTok(tmpLine," \t\n",&TokPtr);

          /* select value */

          ptr = MUStrTok(NULL," \t\n",&TokPtr);

          MSched.BGBasePartitionNodeCnt = (int)strtol(ptr,NULL,10);
          } /* END if ((ptr = strstr(Buf,"NodeCPUCnt")) != NULL) */
        } /* END if (strstr(ptr,"select/bluegene") != NULL) */
      }  /* END if (ptr != NULL) */

    /* look for BackupController w/Format 'BackupController     = <HOSTNAME>|<IPADDR>' */

    ptr = strstr(Buf,"BackupController");

    if (ptr != NULL)
      {
      ptr += strlen("BackupController");

      MUStrCpy(tmpLine,ptr,sizeof(tmpLine));

      /* select '=' */

      ptr = MUStrTok(tmpLine," \t\n",&TokPtr);

      /* select value */

      ptr = MUStrTok(NULL," \t\n",&TokPtr);

      if ((strcmp(ptr,"(null)")) && (R->P[1].HostName == NULL))
        MUStrDup(&R->P[1].HostName,ptr);
      }  /* END if (ptr != NULL) */

    /* look for SchedulerPort w/Format 'SchedulerPort     = 7321' */

    ptr = strstr(Buf,"SchedulerPort");

    if (ptr != NULL)
      {
      ptr += strlen("SchedulerPort");

      MUStrCpy(tmpLine,ptr,sizeof(tmpLine));

      /* select '=' */

      ptr = MUStrTok(tmpLine," \t\n",&TokPtr);

      /* select value */

      ptr = MUStrTok(NULL," \t\n",&TokPtr);

      R->P[0].Port = (int)strtol(ptr,NULL,10);

      if ((R->P[1].HostName != NULL) && (R->P[1].Port == 0))
        R->P[1].Port = R->P[0].Port;
      }  /* END if (ptr != NULL) */

    /* look for SelectType w/Format 'SelectType     = select/cons_res' */

    ptr = strstr(Buf,"SelectType");

    if (ptr != NULL)
      {
      ptr += strlen("SelectType");

      MUStrCpy(tmpLine,ptr,sizeof(tmpLine));

      /* select '=' */

      ptr = MUStrTok(tmpLine," \t\n",&TokPtr);

      /* select value */

      ptr = MUStrTok(NULL," \t\n",&TokPtr);

      if (!strcasecmp(ptr,"select/cons_res"))
        {
        bmset(&MPar[0].Flags,mpfSLURMIsShared);

        MDB(7,fWIKI) MLog("INFO:     scontrol show config has SelectType = select/cons_res\n");
        }
      }  /* END if (ptr != NULL) */
    }    /* END if (MUReadPipe("scontrol show config",...) == SUCCESS) */

  if (R->Version > 10109)
    {
    bmset(&R->Wiki.Flags,mslfNodeDeltaQuery);

    if (R->Version >= 10400)
      {
      bmset(&R->Wiki.Flags,mslfJobDeltaQuery);
      }
    }
  else
    {
    char tmpLine[MMAX_LINE];

    /* in SLURM 0.x and pre-1.1.10, checksum based authorization via the wiki interface is broken */

    sprintf(tmpLine,"SLURM version %d does not support checksum - command authorization disabled",
      R->Version);

    MMBAdd(
      &R->MB,
      tmpLine,
      NULL,
      mmbtNONE,
      0,
      0,
      NULL);

    R->AuthType = mrmapNONE;
    }

  return(SUCCESS);
  }  /* END MSLURMInitialize */






/**
 * Determine if specified job will be allowed to run by SLURM.
 *
 * @param JW       (I)
 * @param R        (I)
 * @param Response (O)
 * @param EMsg     (O) [optional,minsize=MMAX_LINE]
 * @param SC       (O) [optional]
 */

int MSLURMJobWillRun(

  mjwrs_t               *JW,
  mrm_t                 *R,
  char                 **Response,
  char                  *EMsg,
  enum MStatusCodeEnum  *SC)
  
  {
  mjob_t    *J;
  mnl_t     *NL;

  char *MPtr;

  char  tmpLine[MMAX_LINE];

  int rc;

  int   tmpSC;
  char  tEMsg[MMAX_LINE];

  const char *FName = "MSLURMJobWillRun";

  MDB(2,fWIKI) MLog("%s(%s,%s,NL,EMsg,SC)\n",
    FName,
    ((JW != NULL) && (JW->J != NULL)) ? JW->J->Name : "NULL",
    (R != NULL) ? R->Name : "NULL");

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if (SC != NULL)
    *SC = mscNoError;

  if (JW == NULL)
    {
    return(FAILURE);
    }

  NL = &JW->NL;
  J  = JW->J;

  if ((J == NULL) || (R == NULL) || (NL == NULL))
    {
    return(FAILURE);
    }

  if (EMsg != NULL)
    MPtr = EMsg;
  else
    MPtr = tmpLine;

  MPtr[0] = '\0';

  mstring_t NodeBuf(MMAX_BUFFER);

  /* Sort the node list by name before printing to range because bgp[001x000]
   * is invalid */
  /* populate feasible node list as compressed node-range */

  if (MUNLToRangeString(
       NL,
       NULL,
       0,
       FALSE,
       TRUE,  /* sort */
       &NodeBuf) == FAILURE)
    {
    MDB(1,fWIKI) MLog("ALERT:    cannot convert nodelist into range string for job %s\n",
      J->Name);

    return(FAILURE);
    }

  mstring_t CommandBuf(MMAX_BUFFER);
 
  if (R->Version >= 20100)
    {
    /* FORMAT: CMD=JOBWILLRUN ARG=<jobid> [STARTTIME=<time>] NODES=<node_range> [PREEMPT=<jobid1>[,<jobid2>...] */

    MStringAppendF(&CommandBuf,"%s%s %s%s STARTTIME=%lu NODES=%s",
      MCKeyword[mckCommand],
      MWMCommandList[mwcmdCheckJob],
      MCKeyword[mckArgs],
      (J->DRMJID != NULL) ? J->DRMJID : J->Name,
      (JW->StartDate > 0) ? JW->StartDate : 0, 
      NodeBuf.c_str());

    if ((JW->Preemptees != NULL) &&
        (JW->Preemptees[0] != NULL))
      {
      int jIndex;
      mjob_t *pJ = NULL;

      MStringAppend(&CommandBuf," PREEMPT=");

      for (jIndex = 0;JW->Preemptees[jIndex] != NULL;jIndex++)
        {
        pJ = JW->Preemptees[jIndex];

        MStringAppendF(&CommandBuf,"%s%s",
            (jIndex > 0) ? "," : "",
            (pJ->DRMJID != NULL) ? pJ->DRMJID : pJ->Name);
        }
      }
    }
  else
    {
    /* FORMAT: CMD=JOBWILLRUN ARG=<jobid>[@<time>],<node_range>[JOBID=<jobid>[@<time>],<node_range>]... */

    MStringAppendF(&CommandBuf,"%s%s %sJOBID=%s@%lu,%s",
      MCKeyword[mckCommand],
      MWMCommandList[mwcmdCheckJob],
      MCKeyword[mckArgs],
      (J->DRMJID != NULL) ? J->DRMJID : J->Name,
      (JW->StartDate > 0) ? JW->StartDate : 0, 
      NodeBuf.c_str());
    }

  rc = MWikiDoCommand(
          R,
          mrmJobStart,
          &R->P[R->ActingMaster],
          FALSE,
          R->AuthType,
          CommandBuf.c_str(),
          Response,  /* O (alloc,populated on SUCCESS) */
          NULL,
          FALSE,
          tEMsg,      /* O (populated on FAILURE) */
          &tmpSC);

  if (rc == FAILURE)
    {
    MDB(2,fWIKI) MLog("ALERT:    cannot validate resource allocation for job %s through %s RM\n",
      J->Name,
      MRMType[R->Type]);

    if ((EMsg != NULL) && (EMsg[0] == '\0'))
      {
      strcpy(EMsg,"rm failure");
      }

    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MSLURMJobWillRun() */





/**
 * Parses slurm will run response.
 *
 * @param JW           (I) Willrun stucture to populate nodelist and taskcount.
 * @param R            (I)
 * @param Response     (I) The will run response.
 * @param ReqStartTime (I)
 * @param EMsg         (I/O)
 */

int MSLURMParseWillRunResponse(

  mjwrs_t   *JW,
  mrm_t     *R,
  char      *Response,
  mulong     ReqStartTime,
  char      *EMsg)

  {
  int rc;
  char *resPtr;
  int tmpTC;
  mulong tmpStartDate;
  char *tmpNodeStr = NULL;

  int numTokens;
  int tokenArraySize = 6;
  char *tokenArray[tokenArraySize];
  int tIndex;
  
  /* v1.3: STARTINFO=JOBID=<jobid>:<taskcount>@<startdate>,<node_range> [JOBID=... */
  /* v2.1: STARTINFO=<jobid> TASKS=<tc> STARTTIME=<time> NODES=<nodes> [PREEMPT=<jobid>[,<jobid>] */

  if ((JW == NULL) || (Response == NULL))
    {
    return(FAILURE);
    }

  if (strstr(Response,"Jobs not runable") != NULL)
    {
    /* try all nodes */

    return(FAILURE);
    }

  if (R->Version >= 20100)
    {
    resPtr = strstr(Response,"STARTINFO=");
    numTokens = MUStrSplit(resPtr," ",4,tokenArray,tokenArraySize);
    }
  else
    {
    resPtr = strstr(Response,"JOBID=");
    resPtr += strlen("JOBID=");
    numTokens = MUStrSplit(resPtr,":@,",3,tokenArray,tokenArraySize);
    }

  if (numTokens < 4)
    {
    for (tIndex = 0;tIndex < numTokens;tIndex++)
      MUFree(&tokenArray[tIndex]);

    MDB(1,fWIKI) MLog("ERROR:    failed to parse response string\n");

    if ((EMsg != NULL) && (EMsg[0] == '\0'))
      strcpy(EMsg,"failed to parse response string");

    return(FAILURE);
    }

  if (R->Version >= 20100)
    {
    tmpTC = strtol(tokenArray[1] + strlen("TASKS="),0,10);
    tmpStartDate = strtol(tokenArray[2] + strlen("STARTTIME="),0,10);
    tmpNodeStr = tokenArray[3] + strlen("NODES=");

    if ((numTokens == 5) &&
        (MWillRunParsePreemptees(tokenArray[4],JW->Preemptees) == FAILURE))
      {
      for (tIndex = 0;tIndex < numTokens;tIndex++)
        MUFree(&tokenArray[tIndex]);

      MDB(1,fWIKI) MLog("ERROR:    failed to parse preempt string\n");

      if ((EMsg != NULL) && (EMsg[0] == '\0'))
        strcpy(EMsg,"failed to parse preempt string");

      return(FAILURE);
      }
    }
  else
    {
    tmpTC = strtol(tokenArray[1],0,10);
    tmpStartDate = strtol(tokenArray[2],0,10);
    tmpNodeStr = tokenArray[3];
    }

  JW->TC = tmpTC;
  JW->StartDate = MWillRunAdjustStartTime(tmpStartDate,ReqStartTime);

  rc = MUNLFromRangeString(
        tmpNodeStr,   
        NULL,
        &JW->NL,
        NULL,
        0,
        -1,          /* I Set TC to what Node has as TaskCount */
        FALSE,
        (R->IsBluegene) ? TRUE : FALSE);

  /* free allocated tokens */

  for (tIndex = 0;tIndex < numTokens;tIndex++)
    MUFree(&tokenArray[tIndex]);

  if (rc == FAILURE)
    {
    MDB(7,fWIKI) MLog("ALERT:    cannot parse reported range string\n");

    if ((EMsg != NULL) && (EMsg[0] == '\0'))
      strcpy(EMsg,"cannot parse range string");

    return(FAILURE);
    }

  return(SUCCESS);
  } /* END int MSLURMParseWillRunResponse() */




/**
 * Intercepts the call to modify a bg job's nodecount.
 *
 * Translates 1k to 1024 and modifies job's taskcount since
 * slurm thinks in c-nodes and moab thinks in tasks.
 *
 * @param J
 * @param NCValue The node count value to modify the job to.
 * @param Msg
 */

int MSLURMModifyBGJobNodeCount(

  mjob_t *J,       /*I*/
  char   *NCValue, /*I*/
  char   *Msg)     /*O*/

  {
  int   tmpNC;
  int   tmpTC;
  char  tmpNCStr[MMAX_LINE];
  char *tail;

  if ((J == NULL) || (NCValue == NULL))
    return(FAILURE);

  /* map node requests to proc requests on bluegene */

  tmpNC = strtol((char *)NCValue,&tail,10);

  if ((tail != NULL) && (tail[0] == 'k'))
    tmpNC *= 1024;
  
  tmpTC = tmpNC * MSched.BGNodeCPUCnt;

  /* Unset TaskCountIsDefault to not get an invalid count. */

  bmunset(&J->IFlags,mjifTaskCountIsDefault); 

  MReqSetAttr(J,J->Req[0],mrqaTCReqMin,(void **)&tmpTC,mdfInt,mSet);

  snprintf(tmpNCStr,sizeof(tmpNCStr),"%d",tmpNC);

  if (MRMJobModify(
        J,
        "Resource_List",
        "nodes",
        tmpNCStr,
        mSet,
        NULL,
        Msg,  /* O */
        NULL) == FAILURE)
    {
    return(FAILURE);
    }

  return(SUCCESS);
  } /* MSLURMModifyBGJobNodeCount */
/* END MWikiSLURM.c */
