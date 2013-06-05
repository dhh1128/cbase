/* HEADER */

      
/**
 * @file MSchedStats.c
 *
 * Contains: MSched status functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 * Update high-level scheduler statistics for all jobs processed.
 *
 */

int MSchedUpdateStats(void)

  {
  struct timeval   tvp;
  long             schedtime;

  double           efficiency;
  double           SchedTime;

  mpar_t          *GP;
  must_t          *S;

  const char *FName = "MSchedUpdateStats";

  MDB(2,fCORE) MLog("%s()\n",
    FName);

  gettimeofday(&tvp,NULL);

  schedtime = (tvp.tv_sec  - MSched.SchedTime.tv_sec) * 1000 +
              (tvp.tv_usec - MSched.SchedTime.tv_usec) / 1000;

  MDB(2,fCORE) MLog("INFO:     iteration: %4d   scheduling time: %6.3f seconds\n",
    MSched.Iteration,
    (double)schedtime / 1000.0);

  if (MSched.Mode == msmSim)
    {
    SchedTime = (double)MSched.MaxRMPollInterval / (double)MCONST_HOURLEN;
    }
  else
    {
    SchedTime = (double)MSched.Interval / 100.0 / (double)MCONST_HOURLEN;
    }

  GP = &MPar[0];
  S  = &GP->S;

  if ((GP->DRes.Procs == 0) && (GP->UpRes.Procs != 0) && (MAQ.NumItems == 0))
    {
    int rmindex;

    mbool_t RMIsDown = FALSE;
    
    mrm_t *R;

    /* NOTE:  if up procs specified and no dedicated procs detected and active
              jobs are detected and at least one RM is down/corrupt, assume RM 
              failure and assume all nodes are allocated */

    /* NOTE:  This may introduce a bug for stats on truly idle clusters with a
              down non-compute RM - improve stat detection and RM dependencies (FIXME) */

    /* NOTE:  preferred fix is detect if transient/timeout RM failure occured and
              if active jobs exist and dedicated procs are 0, apply same usage as
              detected on previous iteration (NYI) */

    for (rmindex = 0;rmindex < MMAX_RM;rmindex++)
      {
      R = &MRM[rmindex];

      if (R->Name[0] == '\0')
        break;
 
      if (R->Name[0] == '\1')
        continue;

      if ((R->State == mrmsDown) || (R->State == mrmsCorrupt))
        {
        RMIsDown = TRUE;

        break;
        }
      }    /* END for (rmindex) */

    if (RMIsDown == TRUE)
      MStat.TotalPHDed += (double)GP->UpRes.Procs * SchedTime;
    }
  else
    {
    MStat.TotalPHDed += (double)GP->DRes.Procs * SchedTime;
    }

  MStat.TotalPHAvailable   += (double)GP->UpRes.Procs * SchedTime;
  MStat.TotalPHBusy        += (double)(GP->UpRes.Procs - GP->ARes.Procs) *
                               SchedTime;

  if (GP->FSC.TargetIsAbsolute == TRUE)
    {
    /* update theoretical fairshare usage */

    switch (GP->FSC.FSPolicy)
      {
      case mfspDPS:
      default:

        GP->F.FSUsage[0] += GP->UpRes.Procs * SchedTime;

        break;
      }
    }    /* END if (GP->FSC.TargetIsAbsolute == TRUE) */

  /* NOTE:  S->PSDedicated is updated in MStatUpdateActiveJobUsage() */
  /* NOTE:  S->MSDedicated is updated in MStatUpdateActiveJobUsage() */

  S->MSAvail += (double)GP->UpRes.Mem * SchedTime * (double)MCONST_HOURLEN;

  if (GP->UpNodes > 0)
    efficiency = (double)GP->DRes.Procs * 100.0 / GP->UpRes.Procs;
  else
    efficiency = 0.0;

  if (MSched.Iteration > 5)
    {
    if (efficiency < MStat.MinEff)
      {
      MStat.MinEff          = efficiency;
      MStat.MinEffIteration = MSched.Iteration;

      MDB(2,fCORE) MLog("INFO:     minimum efficiency reached (%8.2f percent) on iteration %d\n",
        MStat.MinEff,
        MStat.MinEffIteration);
      }
    }

  MRsvUpdateStats();

  MSchedUpdateNodeCatCredStats();

  MDB(1,fCORE) MLog("INFO:     current util[%d]:  %d/%-d (%.2f%c)  PH: %.2f%c  active jobs: %d of %d (completed: %d)\n",
    MSched.Iteration,
    (GP->UpNodes - GP->IdleNodes),
    GP->UpNodes,
    (GP->UpNodes > 0) ?
      (double)(GP->UpNodes - GP->IdleNodes) * 100.0 / GP->UpNodes :
      (double)0.0,
    '%',
    (MStat.TotalPHAvailable > 0.0) ?
      MStat.TotalPHBusy * 100.0 / MStat.TotalPHAvailable :
      0.0,
    '%',
    MStat.ActiveJobs,
    MStat.EligibleJobs + MStat.ActiveJobs,
    S->JC);

  return(SUCCESS);
  }  /* END MSchedUpdateStats() */





int MSchedUpdateNodeCatCredStats()

  {
  int nindex;               /* node index */
  int ngindex;              /* nodecat group index */
  int cindex;              /* nodecat group index */
  int NIndex[mncLAST];      /* index to next node in list */

  mjob_t   *J;

  mnl_t    *NList[mncLAST];

  int       TCList[mncLAST];
  int       NCList[mncLAST];

  mnode_t *N;

  if (MSched.NodeCatCredEnabled == FALSE)
    {
    return(SUCCESS);
    }

  /* loop through all nodes, assign all nodes w/cred cats to temp jobs */

  memset(NIndex,0,sizeof(NIndex));
  memset(NCList,0,sizeof(NCList));
  memset(TCList,0,sizeof(TCList));

  /* get the number of node categories to allocate */

  for (cindex = 1;cindex < mncLAST;cindex++)
    {
    if (MSched.NodeCatGroup[cindex] == NULL)  /* node categories from the config file */
      {
      break;
      }
    }    /* END for (cindex) */

  /* allocate mnl_t structs */

  MNLMultiInitCount(NList,cindex);

  for (nindex = 0;nindex < MSched.M[mxoNode];nindex++)
    {
    N = MNode[nindex];

    if ((N == NULL) || (N->Name[0] == '\0'))
      break;

    ngindex = MSched.NodeCatGroupIndex[N->Stat.Cat];

    if ((N->Name[0] == '\1') || (ngindex == 0))
      continue;

    /* add node to list associated with cat cred */

    MNLSetNodeAtIndex(NList[ngindex],NIndex[ngindex],N);
    MNLSetTCAtIndex(NList[ngindex],NIndex[ngindex],N->CRes.Procs);

    TCList[ngindex] += N->CRes.Procs;
    NCList[ngindex] += 1;

    NIndex[ngindex]++;
    }  /* END for (nindex) */

  /* create pseudo-job for each node cat grouping */

  MJobMakeTemp(&J);

  J->State = mjsRunning;

  for (ngindex = 0;ngindex < mncLAST;ngindex++)
    {
    if ((MSched.NodeCatGroup[ngindex] == NULL) || (TCList[ngindex] == 0))
      {
      continue;
      }

    MUStrCpy(J->Name,MSched.NodeCatGroup[ngindex],sizeof(J->Name));

    /* terminate nodelist */

    MNLTerminateAtIndex(NList[ngindex],NIndex[ngindex]);

    /* assign allocated nodes to job */

    MNLCopy(&J->NodeList,NList[ngindex]);
    MNLCopy(&J->Req[0]->NodeList,NList[ngindex]);
    J->Req[0]->TaskCount = TCList[ngindex];
    J->TotalProcCount = TCList[ngindex];
    J->Req[0]->NodeCount = NCList[ngindex];

    /* assign credentials to job */

    J->Credential.U = MSched.NodeCatU[ngindex];
    J->Credential.G = MSched.NodeCatG[ngindex];
    J->Credential.A = MSched.NodeCatA[ngindex];
    J->Credential.Q = MSched.NodeCatQ[ngindex];
    J->Credential.C = MSched.NodeCatC[ngindex];

    /* set additional job attributes */

    bmset(&J->IFlags,mjifIsTemplate);

    J->WCLimit = MCONST_DAYLEN;

    MStatUpdateActiveJobUsage(J);
    }  /* END for (ngindex) */

  MNLMultiFreeCount(NList,cindex);

  MJobFreeTemp(&J);

  return(SUCCESS);
  }  /* END MSchedUpdateNodeCatCredStats() */
/* END MSchedStats.c */
