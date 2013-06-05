/* HEADER */

/**
 * @file MJobGRes.c
 *
 * Contains Job GRes functions
 */


#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 * Adds a new Req to the Job in order to add a sole GRes to it.
 *
 * @param J (I)
 * @param RQ (O)
 * @param MakeMaster (I)
 */

int __AddPrivateGResReq(

  mjob_t  *J,
  mreq_t **Req,
  mbool_t  MakeMaster)

  {
  MASSERT(J != NULL,"null job when adding private req.");

  if (MReqCreate(J,NULL,Req,FALSE) == FAILURE)
    {
    return(FAILURE);
    }
  
  mreq_t *reqPtr = *Req;

  MRMReqPreLoad(reqPtr,J->SubmitRM);

  /* unset Procs set by default in MRMReqPreLoad() */

  reqPtr->DRes.Procs = 0;

  if (MakeMaster == TRUE)
    {
    /* NOTE:  master req must be req 0 */
    /*        (ie, master task must show up first in task list */

    J->Req[reqPtr->Index]        = J->Req[0];
    J->Req[reqPtr->Index]->Index = reqPtr->Index;

    J->Req[0] = reqPtr;
    reqPtr->Index = 0;
    }

  reqPtr->TaskCount          = 1;
  reqPtr->TaskRequestList[0] = 1;
  reqPtr->TaskRequestList[1] = 1;

  reqPtr->NAccessPolicy = mnacShared;

  MDB(7,fCONFIG) MLog("INFO:     job %s node access policy set to %s from GRES\n",
    J->Name,
    MNAccessPolicy[reqPtr->NAccessPolicy]);

  return(SUCCESS);
  } /* END void __AddPrivateGResReq() */



/**
 * Adds generic resource requests to the given job.
 *
 * NOTE:  will merge multiple floating gres requests into single req
 *
 * @param J (I) [modified]
 * @param RName (I)
 * @param Count (I)
 * @param Type (I)
 * @param MakeMaster (I) - make gres req master job req
 * @param MakeNew (I) - make new req to hold gres request
 * 
 */

int MJobAddRequiredGRes(

  mjob_t         *J,
  const char     *RName,
  int             Count,
  enum MXAttrType Type,
  mbool_t         MakeMaster,
  mbool_t         MakeNew)

  {
  int rqindex;
  int gindex;
  int sindex;

  mreq_t *RQ;

  mnode_t *VNode;

  mbool_t  NewGRes = FALSE;

  const char *FName = "MJobAddRequiredGRes";

  MDB(5,fSTRUCT) MLog("%s(%s,%s,%d,%s,%s,%s)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (RName != NULL) ? RName : "NULL",
    Count,
    MRMXAttr[Type],
    MBool[MakeMaster],
    MBool[MakeNew]);

  if ((J == NULL) || (RName == NULL) || (RName[0] == '\0') || (Count <= 0))
    {
    return(FAILURE);
    }

  if (!strncasecmp(RName,"none",strlen("none")))
    {

    if ((MSched.EnforceGRESAccess == TRUE) && 
        (MSched.DisableRequiredGResNone == TRUE))
      {

      /* if a GRes of "none" is not allowed (by configuration) then reject it */

      MDB(3,fSTRUCT) MLog("ALERT:    '%s' cannot be requested requested as a generic resource for job %s\n",
        RName,
        J->Name);

      return(FAILURE);
      }

    /* job requires node with no gres present */

    bmset(&J->IFlags,mjifNoGRes);

    return(SUCCESS);
    }

  if (MUMAGetIndex(meGRes,RName,mVerify) == 0)
    {
    NewGRes = TRUE;
    }

  if ((gindex = MUMAGetIndex(
         meGRes,
         RName,
         (MSched.EnforceGRESAccess == TRUE) ?  mVerify : mAdd)) == 0)
    {
    if (MSched.OFOID[mxoxGRes] == NULL)
      MUStrDup(&MSched.OFOID[mxoxGRes],RName);

    MDB(1,fSTRUCT) MLog("ALERT:    cannot add requested generic resource '%s' to job %s\n",
      RName,
      J->Name);

    return(FAILURE);
    }

  if (MGRes.GRes[gindex]->FeatureGRes == TRUE)
    {
    sindex = MUGetNodeFeature(RName,(MSched.EnforceGRESAccess == TRUE) ?  mVerify : mAdd,NULL);

    if (sindex == 0)
      {
      MDB(1,fSTRUCT) MLog("ALERT:    cannot add requested feature gres '%s' to job %s\n",
        RName,
        J->Name);

      return(FAILURE);
      }
    else
      {
      bmset(&J->ReqFeatureGResBM,sindex);

      return(SUCCESS);
      }
    }

  if (MGRes.GRes[gindex]->IsPrivate == TRUE)
    MakeNew = TRUE;

  /* verify resource not already added */

  for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
    {
    if ((((Type == mxaGRes) || (Type == mxaVRes))) && 
          (MSNLGetIndexCount(&J->Req[rqindex]->DRes.GenericRes,gindex) == Count))
      {
      /* request already added */

      return(SUCCESS);
      }

    if ((Type == mxaCGRes) && 
        (J->Req[rqindex]->CRes != NULL) &&
        (MSNLGetIndexCount(&J->Req[rqindex]->CRes->GenericRes,gindex) == Count))
      {
      /* request already added */

      return(SUCCESS);
      }
    }    /* END for (rqindex) */

  if (NewGRes == TRUE)
    {
    /* 9/9/11 DRW: this is a big change in behavior.  If we are adding a GRES in this routine then
                   we default to putting that GRES on its own Req.  This assumes the GRES we just
                   added is a floating resource and not node-locked.  Almost all GRES are floating
                   so this seems like the right behavior */

    MGRes.GRes[gindex]->NotCompute = TRUE;
    }

  /* check if resource exists on the global node or other logical node */

  VNode = NULL;

  if (((Type == mxaVRes) || 
       (MGRes.GRes[gindex]->NotCompute == TRUE)) ||
       (MSchedLocateVNode(gindex,&VNode) == SUCCESS))
    {
    int rqindex;

    /* VRes resources are always located within a virtual resource manager */
    /*  thus, always create an independent req for VRes based gres requests */

    /* check if global req already exists */

    if (VNode == NULL)
      MSchedLocateVNode(gindex,&VNode);

    RQ = NULL;

    for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
      {
      RQ = J->Req[rqindex];

      if (RQ == NULL)
        break;

      /* global req already exists - verify located VNode contains all needed GRes types */

      if (MReqIsGlobalOnly(RQ) == TRUE)
        {
        /* global req already exists - verify located VNode contains all needed GRes types */

        if ((VNode != NULL) && (MSNLGetCount(
             0,
             &RQ->DRes.GenericRes,
             &VNode->CRes.GenericRes,
             NULL) == SUCCESS))
          {
          /* all needed GRes's are located on VNode, use this req */

          break;
          }

        /* this VNode contains GRes we are looking for but not what is needed
           for this req - keep looking for matching req */
        }

      if (MSNLGetIndexCount(&RQ->DRes.GenericRes,gindex) > 0)
        {
        /* req already references gres - use it */

        break;
        }

      if (MakeNew == FALSE)
        {
        if (MReqIsGlobalOnly(RQ) == TRUE)
          {
          int gindex;

          /* verify req does not require existing 'private' gres */

          for (gindex = 1;gindex < MSched.M[mxoxGRes];gindex++)
            {
            if (MGRes.Name[gindex][0] == '\0')
              {
              /* end of list reached */

              break;
              }

            if ((MSNLGetIndexCount(&RQ->DRes.GenericRes,gindex) > 0) &&
                (MGRes.GRes[gindex]->IsPrivate == TRUE))
              {
              /* cannot share req with private gres */

              break;
              }
            }  /* END for (gindex) */

          /* don't try to access an invalid GRes */
          if (gindex == MSched.M[mxoxGRes])
            gindex--;

          if (MGRes.GRes[gindex]->IsPrivate == TRUE)
            continue;

          break;
          }
        }    /* END if (MakeNew == FALSE) */
      
      if ((MSNLIsEmpty(&RQ->DRes.GenericRes) == TRUE) && 
          (RQ->DRes.Procs == 0))
        {
        /* RQ is empty - so use it */

        break;
        }
      }    /* END for (rqindex) */
     
    if (RQ == NULL)
      { 
      if (__AddPrivateGResReq(J,&RQ,MakeMaster) == FAILURE)
        return(FAILURE);

      J->FloatingNodeCount++;
      }  /* END if (RQ == NULL) */

    MSNLSetCount(&RQ->DRes.GenericRes,gindex,Count);
    }
  else if (Type == mxaCGRes)
    {
    /* Assign configured generic resource requirements */

    RQ = J->Req[0];

    MSNLSetCount(&RQ->DRes.GenericRes,gindex,Count);
    }
  else
    {
    /* GRes is located on compute node, assign GRes to primary req 
     * or private req if gres is private */

    /* If GRes is private (ie. is on it's own req), then find and update the private req. */

    if (MakeNew == TRUE)
      {
      RQ = NULL;
      for (int rqindex = 0;(rqindex < MMAX_REQ_PER_JOB) && (J->Req[rqindex] != NULL);rqindex++)
        {
        if (MSNLGetIndexCount(&J->Req[rqindex]->DRes.GenericRes,gindex) > 0)
          {
          RQ = J->Req[rqindex];
          break;
          }
        }
      }
    else
      RQ = J->Req[0];

    if (RQ == NULL)
      {
      if (__AddPrivateGResReq(J,&RQ,MakeMaster) == FAILURE)
        return(FAILURE);
      }

    if ((MGRes.GRes[gindex]->IsPrivate == TRUE) &&
        (MGRes.GRes[gindex]->InvertTaskCount == TRUE))
      { 
      /* Invert taskcount so that it is 1 task with <Count> gres' per task. */
      MSNLSetCount(&RQ->DRes.GenericRes,gindex,1);
      RQ->TaskCount = Count;
      RQ->TaskRequestList[0] = Count;
      RQ->TaskRequestList[1] = Count;
      }
    else
      {
      MSNLSetCount(&RQ->DRes.GenericRes,gindex,Count);
      }

    if ((J->SubmitRM != NULL) &&
        (J->SubmitRM->ApplyGResToAllReqs == TRUE))
      {
      int rqindex;

      for (rqindex = 1;J->Req[rqindex] != NULL;rqindex++)
        {
        RQ = J->Req[rqindex];

        MSNLSetCount(&RQ->DRes.GenericRes,gindex,Count);
        }
      }
    }    /* END else */

  return(SUCCESS);
  }  /* END MJobAddRequiredGRes() */





/**
 * report TRUE if job requires specified generic resource
 *
 * @param J (I)
 * @param GRIndex (I)
 */

mbool_t MJobGResIsRequired(

  mjob_t *J,
  int     GRIndex)

  {
  int rqindex;

  if ((J == NULL) || (GRIndex <= 0))
    {
    return(FALSE);
    }

  for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
    {
    if (J->Req[rqindex] == NULL)
      break;

    if (MSNLGetIndexCount(&J->Req[rqindex]->DRes.GenericRes,GRIndex) > 0) 
      {
      return(TRUE);
      }
    }    /* END for (rqindex) */

  return(FALSE);
  }  /* END MJobGResIsRequired() */







/**
 * Determines if the job has the optional gres or if any job has a gres.
 *
 * @param J (I)
 * @param GResName (I) If null then the function returns true if any gres is found.  [optional]
 * @param ReqIndex (I) Only check the given req if >=0. [optional]
 */

mbool_t MJobHasGRes(

  mjob_t     *J,
  const char *GResName,
  int         ReqIndex)

  {
  int gindex;
  int rqindex;
  int endIndex;

  if (J == NULL)
    {
    return(FALSE);
    }

  if ((GResName == NULL) || (GResName[0] == '\0'))
    {
    gindex = -1;
    }
  else if ((gindex = MUMAGetIndex(meGRes,GResName,mVerify)) == 0)
    {
    return(FALSE);
    }

  if (ReqIndex >= 0)
    {
    rqindex = ReqIndex;
    endIndex = ReqIndex + 1;
    }
  else
    {
    rqindex = 0;
    endIndex = MMAX_REQ_PER_JOB;
    }

  for (rqindex = 0;rqindex < endIndex;rqindex++)
    {
    if (J->Req[rqindex] == NULL)
      break;

    if (MSNLIsEmpty(&J->Req[rqindex]->DRes.GenericRes))
      continue;

    if ((gindex == -1) || (MSNLGetIndexCount(&J->Req[rqindex]->DRes.GenericRes,gindex) > 0))
      {
      return(TRUE);
      }
    }  /* END for (rqindex) */

  return(FALSE);
  }  /* END MJobHasGRes() */





/**
 * Remove generic resource requirements from job.
 *
 * 6-2-09 BOC: 
 *   RemoveReq was added to prevent reqs from being destroyed. Destroying the req causes
 *   problems when destroying gres only reqs in a grid. Starting at MJobFromXML, an RQ 
 *   pointer, pointing at J->Req[index], is passed down the path: 
 *   MJobFromXML -> MReqFromXML -> MReqSetAttr. If RQ points to the gres only req and is
 *   the only req, then when it is destroyed RQ will be pointing to free'd' memory the next 
 *   call to MReqSetAttr. The solution is to not destroy the req and just let it be cleared.
 *   The side effect is that there are empty reqs. MJobAddRequiredGRes adds the req to the 
 *   global req if it is floating or adds it to Req[0] if it is node locked.  
 *             
 * @param J (I) [modified]
 * @param GResName (I)
 * @param RemoveReq (I) If true, will remove gres only reqs.
 */

int MJobRemoveGRes(

  mjob_t *J,         /* I (modified) */
  char   *GResName,  /* I */
  mbool_t RemoveReq) /* I */

  {
  int gindex;
  int rqindex;

  if ((J == NULL) || (GResName == NULL) || (GResName[0] == '\0'))
    {
    return(SUCCESS);
    }

  if ((gindex = MUMAGetIndex(meGRes,GResName,mVerify)) == 0)
    {
    /* GRes does not exist so job cannot have it */

    return(SUCCESS);
    }

  for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
    {
    if (J->Req[rqindex] == NULL)
      break;

    if (MSNLGetIndexCount(&J->Req[rqindex]->DRes.GenericRes,gindex) == 0)
      continue;

    /* if req has procs, only remove gres reference, otherwise, remove 
       full req */

    if ((J->Req[rqindex]->DRes.Procs != 0) ||
        (J->Req[rqindex]->DRes.Disk != 0) ||
        (RemoveReq == FALSE))
      {
      /* req possesses non-gres resources */

      MSNLSetCount(&J->Req[rqindex]->DRes.GenericRes,gindex,0);

      continue;
      }

    /* gres-only req located */

    if (RemoveReq == TRUE)
      {
      if (J->Req[rqindex]->R != NULL)
        {
        MRsvDestroy(&J->Req[rqindex]->R,TRUE,TRUE);
        }

      MReqRemoveFromJob(J,rqindex);

      if ((J->Rsv == NULL) && 
          (J->Req[0] != NULL) &&
          (J->Req[0]->R != NULL))
        {
        J->Rsv = J->Req[0]->R;
        }
      }   /* END if (RemoveReq == TRUE) */

    return(SUCCESS);
    }  /* END for (rqindex) */
 
  return(SUCCESS);
  }  /* END MJobRemoveGRes() */





/**
 * Remove all GRes (and associated reservations) for job.
 *
 * @param J (I) [modified]
 */

int MJobRemoveAllGRes(

  mjob_t *J)         /* I (modified) */

  {
  int gindex;
  int rqindex;

  for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
    {
    if (J->Req[rqindex] == NULL)
      break;

    if (MSNLIsEmpty(&J->Req[rqindex]->DRes.GenericRes))
      continue;

    for (gindex = 1;gindex < MSched.M[mxoxGRes];gindex++)
      {
      if (MGRes.Name[gindex][0] == '\0')
        break;

      if (MSNLGetIndexCount(&J->Req[rqindex]->DRes.GenericRes,gindex) == 0)
        continue;

      MJobRemoveGRes(J,MGRes.Name[gindex],FALSE); /* don't destroy req */

      /* if removing the GRes also caused the Req to be removed then we are done */

      if (J->Req[rqindex] == NULL)
        break;

      if (MSNLIsEmpty(&J->Req[rqindex]->DRes.GenericRes))
        break;
      } /* END for (gindex) */
    }  /* END for (rqindex) */

  return(SUCCESS);
  } /* END MJobRemoveAllGRes() */





/**
 * Sum all job's gres into single msnl_t structure.
 *
 * @param J
 * @param GRes
 */

int MJobGetTotalGRes(

  const mjob_t *J,
  msnl_t       *GRes)

  {
  int rqindex;
  int gindex;

  mreq_t *RQ;

  int Count;

  if ((J == NULL) || (GRes == NULL))
    {
    return(FAILURE);
    }

  MSNLClear(GRes);

  for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
    {
    RQ = J->Req[rqindex];

    if (RQ == NULL)
      break;

    if (MSNLGetIndexCount(&RQ->DRes.GenericRes,0) == 0)
      continue;

    for (gindex = 1;gindex < MSched.M[mxoxGRes];gindex++)
      {
      if (MGRes.Name[gindex][0] == '\0')
        break;

      /* sum whatever we already had plus the new resource times the number of tasks */

      Count = MSNLGetIndexCount(GRes,gindex) + (MSNLGetIndexCount(&RQ->DRes.GenericRes,gindex) * RQ->TaskCount);

      MSNLSetCount(GRes,gindex,Count);
      }  /* END for (gindex) */
    }    /* END for (rqindex) */

  return(SUCCESS);
  }  /* END MJobGetTotalGRes() */






/**
 * Updates the given generic resources based on the value
 * coming back from the wiki interface.
 *
 * @param J (I/O) the job that owns the RQ and the GRes
 * @param RQ (I/O) the owner of the GRes
 * @param GRes (O) the GRes to be updated
 * @param AIndex (I) attribute index (GRes type)
 * @param Value (I) string specifying the updates to the GRes
 *
 * @return SUCCESS if at least one good GRes value is parsed from Valu*
 */

int MJobUpdateGRes(

  mjob_t *J,
  mreq_t *RQ,
  msnl_t *GRes,
  int     AIndex,
  char   *Value)

  {
  char *pairPtr;
  const char *StringDelim = ",";
  const char *PairDelim = ":";
  char *TokPtr;
  char  tmpLine[MMAX_LINE];
  int   rc = FAILURE;

  if ((J == NULL) ||
      (RQ == NULL) ||
      (GRes == NULL) ||
      (Value == NULL))
    {
    return(FAILURE);
    }

  strncpy(tmpLine,Value,sizeof(tmpLine));

  TokPtr = tmpLine;

  while ((pairPtr = MUStrTok(NULL,StringDelim,&TokPtr)) != NULL)
    {
    char *strtolEndPtr;
    int   GResValue;
    int   GResIndex;
    char *keyPtr;
    char *valPtr;
    char *TokPtr2 = pairPtr;
    int OldValue;

    keyPtr = MUStrTok(NULL,PairDelim,&TokPtr2);
    valPtr = MUStrTok(NULL,PairDelim,&TokPtr2);

    if (keyPtr == NULL)
      {
      MDB(5,fWIKI) MLog("INFO:     parsed GRES wiki string with empty pair\n");
      
      continue;
      }

    if (valPtr == NULL)
      {
      MDB(2,fWIKI) MLog("INFO:     GRes keyword '%s' passed in with no value\n",
        keyPtr);

      GResValue = 1;
      }
    else
      {
      GResValue = strtol(valPtr,&strtolEndPtr,10);

      if (strtolEndPtr == valPtr)
        {
        MDB(2,fWIKI) MLog("INFO:     value '%s' for GRes '%s' is not a number",
          Value,
          keyPtr);

        continue;
        }

      if (GResValue <= 0)
        {
        MDB(2,fWIKI) MLog("INFO:     value '%s' for GRes '%s' is not positive",
          Value,
          keyPtr);

        continue;
        }
      }

    rc = SUCCESS;

    GResIndex = MUMAGetIndex(meGRes,keyPtr,mAdd);

    if (GResIndex == 0)
      {
      continue;
      }

    /* NOTE:  if job is newly submitted, must associate GRes with correct req */

    /* determine correct req to associate with gres */

    switch (AIndex)
      {
      case mwjaCGRes:

        MSNLSetCount(&RQ->CGRes,GResIndex,GResValue);

        break;

      case mwjaAGRes:

        MSNLSetCount(&RQ->AGRes,GResIndex,GResValue);

        break;

      case mwjaDGRes:
      default:

        OldValue = 0;

        if (bmisset(&J->IFlags,mjifVPCMap))
          {
          int rqindex;

          for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
            {
            if (J->Req[rqindex] == NULL)
              break;

            if (MSNLGetIndexCount(&J->Req[rqindex]->DRes.GenericRes,GResIndex) > 0)
              {
              OldValue = MSNLGetIndexCount(&J->Req[rqindex]->DRes.GenericRes,GResIndex);

              break;
              }
            }  
          }
 
        MJobAddRequiredGRes(J,keyPtr,GResValue,mxaGRes,FALSE,FALSE);

        if ((OldValue != GResValue) && 
            (bmisset(&J->IFlags,mjifVPCMap)))
          {
          mpar_t *VPC = NULL;

          /* new GRes value needs to be placed onto VPC and RSV */

          MVPCFind(J->AName,&VPC,FALSE);

          MVPCUpdateResourceUsage(VPC,&RQ->DRes);
          }

        break;
      }  /* END switch (AIndex) */
    }    /* END while (pairPtr != NULL) */ 

  return(rc);
  }  /* END MJobUpdateGRes() */





/**
 * Allocate a gres type to a job.
 *
 */

int MJobAllocateGResType(

  mjob_t *J,
  char   *EMsg)

  {
#if 0 /* needs to be updated for new GRes structures */
  int rqindex;

  int gindex;
  int gtindex;

  /* find the req and the gres which need allocating */

  for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
    {
    for (gindex = 1;MGRes.Name[gindex][0] != '\0';gindex++)
      {
      if (MGRes.Name[gindex][0] == '\0')
        break;

      if (MSNLGetIndexCount(&J->Req[rqindex]->DRes.GenericRes,gindex) == 0)
        continue;

      for (gtindex = 1;gtindex < J->Req[rqindex]->DRes.GRes.Size;gtindex++)
        {
        if (MGRes.Name[gtindex][0] == '\0')
          break;

        if (strcmp(MGRes[gtindex].Type,MGRes.Name[gindex]))
          continue;

        break;
        }

      if (MGRes.Name[gtindex][0] != '\0')
        break;
      }
 
    if ((MSNLGetIndexCount(&J->Req[rqindex]->DRes.GenericRes,gindex) != 0) ||
        (MGRes.Name[gindex][0] != '\0'))
      {
      break;
      }
    }

  if ((J->Req[rqindex] == NULL) ||
      (MGRes.Name[gindex][0] == '\0'))
    {
    sprintf(EMsg,"job '%s' did not allocate gres of type\n",
      J->Name);

    return(FAILURE);
    }

  if (MRsvAllocateGResType(
        (J->Req[rqindex]->R == NULL) ? J->R : J->Req[rqindex]->R,
        MGRes.Name[gindex],
        EMsg) == FAILURE)
    {
    return(FAILURE);
    }

  MJobAddRequiredGRes(J,EMsg,1,mxaGRes,FALSE,FALSE);

#endif /* ! */

  return(SUCCESS);
  }  /* END MJobAllocateGResType() */





/**
 * Remove non-Gres reqs from jobs
 *
 */

int MJobRemoveNonGResReqs(

  mjob_t *J,
  char   *EMsg)

  {
  int gresrqindex;
  
  /* check for invalid use of this flag */

  for (gresrqindex = 0;gresrqindex < MMAX_REQ_PER_JOB;gresrqindex++)
    {
    if (J->Req[gresrqindex] == NULL)
      break;
  
    if (MSNLGetIndexCount(&J->Req[gresrqindex]->DRes.GenericRes,0) <= 0)
      {
      int localrqindex = gresrqindex + 1;
  
      if ((gresrqindex == 0) && (J->Req[1] == NULL))
        {
        MDB(1,fRM) MLog("ALERT:    gresonly flag used on non-gres job\n",
          J->Name);
   
        if (EMsg != NULL)
          strcpy(EMsg,"gresonly flag used on non-gres job");
   
        MJobReject(&J,mhBatch,MSched.DeferTime,mhrRMReject,"gresonly flag used on non-gres job");
   
        return(FAILURE);
        }
      
      /* this req has no gres requirement so it's probably just a leftover, delete it */
  
      /* Copy features from this req first onto other reqs so that features are preserved */
  
      for (;localrqindex < MMAX_REQ_PER_JOB && J->Req[localrqindex] != NULL;localrqindex++)
        {
        MReqSetAttr(J,J->Req[localrqindex],mrqaReqNodeFeature,(void **)&J->Req[gresrqindex]->ReqFBM,mdfOther,mAdd);
        }
  
      MReqRemoveFromJob(J,gresrqindex);
      }
    else
      {
      /* zero out the other requested resources */
  
      J->Req[gresrqindex]->DRes.Procs = 0;
      J->Req[gresrqindex]->DRes.Mem   = 0;
      J->Req[gresrqindex]->DRes.Disk  = 0;
      J->Req[gresrqindex]->DRes.Swap  = 0;
      }
    }

  return(SUCCESS);
  }
