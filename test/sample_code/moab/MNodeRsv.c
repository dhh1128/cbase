/* HEADER */

      
/**
 * @file MNodeRsv.c
 *
 * Contains: Node and Reservations
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 * Finds all jobs from the NodeList that have the IgnIdleJobRsv flag.
 *
 * @param NodeList (I)
 * @param JobList (I/O) [modified]
 */

int MNodeListGetIgnIdleJobRsvJobs(

  mnl_t    *NodeList, /* I */
  mjob_t  **JobList)  /* I/O modified */

  {
  int jindex,nindex;

  mnode_t *tmpN;

  if((JobList == NULL) || (NodeList == NULL))
    return(FAILURE);

  JobList[0] = NULL;

  for (nindex = 0;MNLGetNodeAtIndex(NodeList,nindex,&tmpN) == SUCCESS;nindex++)
    {
    for (jindex = 0;tmpN->JList[jindex] != NULL;jindex++)
      {
      mjob_t *tmpJ = tmpN->JList[jindex];

      if (bmisset(&tmpJ->Flags,mjfIgnIdleJobRsv))
        {
        int tmpindex = 0;

        while ((JobList[tmpindex] != NULL) && /* not at end of current list */
               (JobList[tmpindex] != tmpJ) && /* job already in current list */
               (tmpindex < MMAX_JOB))
          tmpindex++;

        if (JobList[tmpindex] == NULL)
          {
          JobList[tmpindex] = tmpJ;
          JobList[++tmpindex] = NULL;
          }
        }
      }   /* END for jindex */
    }     /* END for nindex */

  return(SUCCESS);
  }  /* END MNodeListGetIgnIdleJobRsvJobs() */




/**
 * Report number of reservations on specified node which meet JobOnly and ActiveOnly criteria.
 *
 * @see MNodeGetJob() - peer
 * @see MNodeGetRsv() - peer
 *
 * @param N (I)
 * @param JobOnly (I)
 * @param ActiveOnly (I)
 */

int MNodeGetRsvCount(

  mnode_t  *N,          /* I */
  mbool_t   JobOnly,    /* I */
  mbool_t   ActiveOnly) /* I */

  {
  int RC = 0;

  mre_t  *RE;
  mrsv_t *R;

  if ((N == NULL) || (N->RE == NULL))
    {
    return(0);
    }

  /* determine reservation count */

  for (RE = N->RE;RE != NULL;MREGetNext(RE,&RE))
    {
    if (RE->Type != mreEnd)
      continue;

    R = MREGetRsv(RE);

    if ((JobOnly == TRUE) && (R->Type != mrtJob))
      continue;

    if ((ActiveOnly == TRUE) && (R->StartTime > MSched.Time))
      continue;

    RC++;
    }  /* END for (rindex) */

  return(RC);
  }  /* END MNodeGetRsvCount() */



/**
 * Reserve node for specific purpose (typically failure or trigger based rsv).
 *
 * @see MNLReserve()
 *
 * @param N (I)
 * @param Duration (I) [if <= 0, take no action]
 * @param Name (I) [optional]
 * @param RsvProfile (I) [optional]
 * @param Msg (I) [optional]
 */

int MNodeReserve(

  mnode_t    *N,
  mulong      Duration,
  const char *Name,
  char       *RsvProfile,
  const char *Msg)

  {
  char    RName[MMAX_LINE];

  mrsv_t *R;
  mrsv_t *RProf = NULL;

  mnl_t   NL = {0};
 
  mulong    EndTime;

  mbitmap_t tmpFlags;

  if (Duration <= 0)
    {
    return(SUCCESS);
    }

  MNLInit(&NL);

  MNLSetNodeAtIndex(&NL,0,N);
  MNLSetTCAtIndex(&NL,0,1);

  if ((Name != NULL) && (Name[0] != '\0'))
    {
    snprintf(RName,sizeof(RName),"%s.%d",
      Name,
      MSched.RsvCounter++);
    }
  else
    {
    snprintf(RName,sizeof(RName),"maintenance.%d",
      MSched.RsvCounter++);
    }

  if (MRsvFind(RName,&R,mraNONE) == SUCCESS)
    {
    MRsvDestroy(&R,TRUE,TRUE);
    }

  if (RsvProfile != NULL)
    MRsvProfileFind(RsvProfile,&RProf);

  EndTime = MSched.Time + (((RProf != NULL) && (RProf->Duration > 0)) ? (mulong)RProf->Duration : Duration);

  if (MRsvCreate(
        mrtUser,                                /*  Type         */
        (RProf != NULL) ? RProf->ACL : NULL,    /*  ACL          */
        NULL,                                   /*  CL           */
        (RProf != NULL) ? &RProf->Flags : &tmpFlags, /* Flags      */
        &NL,                                    /*  NodeList     */
        MSched.Time,                            /*  StartTime    */
        EndTime,                                /*  EndTime      */
        1,                                      /*  NC           */
        -1,                                     /*  PC           */
        RName,                                  /*  RBaseName    */
        &R,                                     /*  RsvP         */
        N->Name,                                   /*  HostExp      */
        NULL,                                   /*  DRes         */
        NULL,                                   /*  EMsg         */
        TRUE,                                   /*  DoRemote     */
        TRUE) == FAILURE)                       /*  DoAlloc      */
    {
    MNLFree(&NL);
    return(FAILURE);
    }

  if (R->HostExp != NULL)
    {
    R->HostExpIsSpecified = TRUE;
    }

  if (Msg != NULL)
    {
    MRsvSetAttr(R,mraComment,(void *)Msg,mdfString,mSet);
    }

  MNLFree(&NL);

  return(SUCCESS);
  }  /* END MNodeReserve() */




/**
 *
 *
 * @param N (I)
 * @param J () [optional]
 * @param StartTime (I)
 * @param Duration (I)
 */

mbool_t MNodeRangeIsExclusive(

  mnode_t *N,          /* I */
  mjob_t  *J,          /* (optional) */
  mulong   StartTime,  /* I */
  mulong   Duration)   /* I */

  {
  mre_t *RE;

  mulong  Overlap;

  mrsv_t *R;

  if ((N == NULL) || (StartTime <= 0) || (N->RE == NULL))
    {
    return(FALSE);
    }

  for (RE = N->RE;RE != NULL;MREGetNext(RE,&RE))
    {
    if ((mulong)RE->Time < StartTime)
      continue;

    if ((mulong)RE->Time > StartTime + Duration)
      break;

    /* if start or end event detected, rsv is active during range */

    R = MREGetRsv(RE);

    if (J == NULL)
      {
      if (bmisset(&R->Flags,mrfStanding))
        continue;
      }
    else
      {
      if (RE->Type == 1)
        {
        Overlap = StartTime + Duration - RE->Time;
        }
      else
        {
        Overlap = RE->Time - StartTime;
        }
 
      if (MRsvCheckJAccess(
           R,
           J,
           Overlap,
           NULL,
           FALSE,
           NULL,
           NULL,
           NULL,
           NULL) == TRUE)
        {
        continue;
        }
      }

    return(TRUE);
    }  /* END for (MREGetNext) */
   
  return(FALSE);
  }  /* END MNodeRangeIsExclusive() */


    



/**
 * Locate specified reservation on specified node.
 *
 * @see MRsvFind() - peer
 * @see MRsvHasOverlap() - peer
 *
 * @param N          (I)
 * @param RID        (I) rsvid or rsvgroup id [optional]
 * @param ActiveOnly (I) if set, only report active reservations
 * @param RP         (O) [optional]
 * @param JP         (O) [optional]
 */

int MNodeRsvFind(

  const mnode_t  *N,
  const char     *RID,
  mbool_t         ActiveOnly,
  mrsv_t        **RP,
  mjob_t        **JP)

  {
  mre_t  *RE;
  mrsv_t *R;

  if (RP != NULL)
    *RP = NULL;

  if (JP != NULL)
    *JP = NULL;

  if ((N == NULL) || (RID == NULL) || (RID[0] == '\0'))
    {
    return(FAILURE);
    }

  for (RE = N->RE;RE != NULL;MREGetNext(RE,&RE))
    {
    R = MREGetRsv(RE);

    if (strcmp(RID,R->Name) && ((R->RsvGroup == NULL) || strcmp(RID,R->RsvGroup)))
      {
      /* rsv does not match RID */

      continue;
      }

    if ((ActiveOnly == TRUE) && (R->StartTime > MSched.Time))
      continue;

    if ((RP == NULL) && (JP != NULL))
      {
      if (R->J == NULL)
        continue;
      }

    if (RP != NULL)
      *RP = R;

    if (JP != NULL)
      *JP = R->J;

    return(SUCCESS);
    }  /* END for (rindex) */

  return(FAILURE);
  }  /* END MNodeRsvFind() */
/* END MNodeRsv.c */
