
/* HEADER */

      
/**
 * @file MReq.c
 *
 * Contains:
 */


#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 * Create/allocate new req structure.
 *
 * NOTE:  all dynamic req attributes must be explicitly handled inside of 
 *            DoAllocate == TRUE block
 *
 * @see MJobCreate() - peer
 * @see MReqDestroy() - peer - free/destroy req
 *
 * @param J (I)
 * @param SrcRQ (I) source requirements [optional]
 * @param DstRQ (O) new requirement [optional]
 * @param DoAllocate (I) allocate new dynamic memory
 */

int MReqCreate(
 
  mjob_t        *J,
  const mreq_t  *SrcRQ,
  mreq_t       **DstRQ,
  mbool_t        DoAllocate)

  {
  mreq_t *RQ;
  int     rqindex;

  const char *FName = "MReqCreate";

  MDB(2,fSTRUCT) MLog("%s(%s,SrcRQ,DstRQ,%s)\n",
    FName,
    (J != NULL) ? J->Name : NULL,
    MBool[DoAllocate]);

  if (J == NULL)
    {
    return(FAILURE);
    }
 
  for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
    {
    if (J->Req[rqindex] == NULL)
      break;
    }  /* END for (rqindex) */

  if (rqindex >= MMAX_REQ_PER_JOB)
    {
    MDB(4,fSTRUCT) MLog("ALERT:    failed attempt to add req to job %s (rqindex = %d)\n",
       J->Name,
       rqindex);

    return(FAILURE);
    }
 
  MDB(4,fSTRUCT) MLog("INFO:     adding requirement at slot %d\n",
    rqindex);
 
  if ((RQ = (mreq_t *)MUCalloc(1,sizeof(mreq_t))) == NULL)
    {
    MDB(1,fSTRUCT) MLog("ALERT:    cannot allocate memory for requirement %s:%d, errno: %d (%s)\n",
      J->Name,
      rqindex,
      errno, 
      strerror(errno));
 
    return(FAILURE);
    }

  RQ->SetBlock = MBNOTSET;

  RQ->NodeSetCount = 0;

  if (!bmisset(&MSched.Flags,mschedfMinimalJobSize))
    {
    RQ->URes = (mcres_t *)MUCalloc(1,sizeof(mcres_t));
    RQ->LURes = (mcres_t *)MUCalloc(1,sizeof(mcres_t));
    RQ->MURes = (mcres_t *)MUCalloc(1,sizeof(mcres_t));

    MCResInit(RQ->URes);
    MCResInit(RQ->LURes);
    MCResInit(RQ->MURes);
    }

  RQ->Pref = NULL;

  if (SrcRQ != NULL)
    {
    /* this used to be a memcpy, but that's just an incredibly stupid thing to do */

    MReqDuplicate(RQ,SrcRQ);
    }
  else
    {
    MSNLInit(&RQ->DRes.GenericRes);
    MSNLInit(&RQ->CGRes);
    MSNLInit(&RQ->AGRes);
    }
 
  if ((SrcRQ != NULL) && (DoAllocate == TRUE))
    {
    if (!MNLIsEmpty(&SrcRQ->NodeList))
      {
      MNLCopy(&RQ->NodeList,&SrcRQ->NodeList);
      }
    else
      { 
      MNLClear(&RQ->NodeList);
      }

    if (SrcRQ->NAllocPolicy != mnalNONE)
      {
      RQ->NAllocPolicy = SrcRQ->NAllocPolicy;
      }
    }    /* END if ((SrcRQ != NULL) && (DoAllocate == TRUE)) */

  J->Req[rqindex]     = RQ;
  J->Req[rqindex + 1] = NULL;
 
  RQ->Index = rqindex;

  bmset(&J->IFlags,mjifReqIsAllocated);
 
  if (DstRQ != NULL)
    *DstRQ = RQ;
 
  return(SUCCESS);
  }  /* END MReqCreate() */



/**
 * Compare to mreq_t structures.
 *
 * @param R1
 * @param R2
 */

int MReqCmp(

  const mreq_t *R1,
  const mreq_t *R2)

  {
  if ((R1 != NULL) && (R2 == NULL))
    return(FAILURE);

  if ((R1 == NULL) && (R2 != NULL))
    return(FAILURE);

  if (MCResCmp(&R1->DRes,&R2->DRes) == FAILURE)
    return(FAILURE);

  if (bmcompare(&R1->ReqFBM,&R2->ReqFBM) != 0)
    return(FAILURE);

  return(SUCCESS);
  }  /* END MReqCmp() */



/**
 * Free the NestedNodeSet structure.
 *
 */

int MReqFreeNestedNodeSet(

  mln_t **NestedNodeSet)

  {
  int index;

  mln_t *tmpL;

  char **List;

  if (NestedNodeSet == NULL)
    {
    return(SUCCESS);
    }

  tmpL = *NestedNodeSet;

  while (tmpL != NULL)
    {
    List = (char **)tmpL->Ptr;
    
    index = 0;

    while (List[index] != NULL)
      {
      MUFree(&List[index]);

      index++;
      }

    MUFree((char **)&tmpL->Ptr);

    tmpL = tmpL->Next;
    }  /* END while (tmpL != NULL) */

  MULLFree(NestedNodeSet,NULL);

  return(SUCCESS);
  }  /* END MReqFreeNestedNodeSet() */



/**
 * Initializes the XLoad structure on the node.  Also creates
 *  the gevent arrays.
 */

int MReqInitXLoad(

  mreq_t *RQ)    /* I */

  {
  if (RQ->XURes != NULL)
    {
    return(SUCCESS);
    }

  if ((RQ->XURes = (mxload_t *)MUCalloc(1,sizeof(mxload_t))) == NULL)
    {
    return(FAILURE);
    }

  MGMetricCreate(&RQ->XURes->GMetric);

  return(SUCCESS);
  } /* END MReqInitXLoad */

/**
 * Destroy/Free job req.
 *
 * @see MReqCreate() - peer
 * @see MJobDestroy() - parent
 *
 * @param RQP (I)
 */

int MReqDestroy(

  mreq_t **RQP)         /* I */

  {
  int index;

  mreq_t *RQ;

  if ((RQP == NULL) || (*RQP == NULL))
    {
    return(SUCCESS);
    }

  RQ = *RQP;

  if (RQ->SetList != NULL)
    {
    for (index = 0;RQ->SetList[index] != NULL;index++)
      {
      if (RQ->SetList[index] == NULL)
        break;

      MUFree((char **)&RQ->SetList[index]);
      }  /* END for (index) */

    MUFree((char **)&RQ->SetList);
    }

  MNLFree(&RQ->NodeList);

  if (RQ->Pref != NULL)
    {
    /* perform deep free */

    if (RQ->Pref->Variables)
      MULLFree((mln_t **)&RQ->Pref->Variables,MUFREE);

    MUFree((char **)&RQ->Pref);
    }

  if (RQ->OptReq != NULL)
    MReqOptReqFree(&RQ->OptReq);

  if (RQ->XURes != NULL)
    {
    MXLoadDestroy(&RQ->XURes);
    }  /* END if (RQ->XUREs != NULL) */

  RQ->ReqAttr.clear();

  MCResFree(&RQ->DRes);
  MCResFree(RQ->CRes);
  MCResFree(RQ->URes);
  MCResFree(RQ->MURes);
  MCResFree(RQ->LURes);

  MSNLFree(&RQ->CGRes);
  MSNLFree(&RQ->AGRes);

  MUFree((char **)&RQ->CRes);
  MUFree((char **)&RQ->URes);
  MUFree((char **)&RQ->LURes);
  MUFree((char **)&RQ->MURes);

  MUFree(&RQ->ParentJobName);

  bmclear(&RQ->ReqFBM);
  bmclear(&RQ->SpecFBM);
  bmclear(&RQ->ExclFBM);

  /* free dynamically allocated req structure */

  MUFree((char **)RQP);

  return(SUCCESS);
  }  /* END MReqDestroy() */




/**
 * Destroy/deep free moptreq_t structure.
 */

int MReqOptReqFree(

  moptreq_t **OP)  /* I (freed) */

  {
  moptreq_t *O;

  if ((OP == NULL) || (*OP == NULL))
    {
    return(SUCCESS);
    }

  O = *OP;

  MUFree(&O->ReqJID);

  MUFree(&O->ReqService);

  if (O->GMReq != NULL)
    MULLFree((mln_t **)&O->GMReq,MUFREE);

  MUFree((char **)OP);

  return(SUCCESS);
  }  /* END MReqOptReqFree() */


/**
 *
 *
 * @param J (I)
 * @param RQ (I)
 */

int MReqGetPC(

  mjob_t *J,  /* I */
  mreq_t *RQ) /* I */

  {
  int PC = 0;

  MJobUpdateResourceCache(J,RQ,0,&PC);

  return(PC);
  }  /* END MReqGetPC() */


/**
 * Parse a feature string and set the appropriate BM.
 *
 * @param J      (I)
 * @param RQ     (I)
 * @param Mode   (I)
 * @param String (I)
 */

int MReqSetFBM(

  mjob_t                 *J,
  mreq_t                 *RQ,
  enum MObjectSetModeEnum Mode,
  const char             *String)

  {
  /* NOTE:  only set feature list on primary req (FIXME) */

  /* determine feature logic modes
   * NOTE:  the mode will be set to the first operand, 
   * i.e. aix|hpux&bigmem will become aix|hpux|bigmem */

  /* NOTE: currently you can ask for the following:
       {A|B|!C|!D}
       {A&B&!C&!D}
       but you can't mix modes */

  int rqcounter;

  char *Value = NULL;
  char *ptr;
  char *OrPtr;
  char *AndPtr;
  char *TokPtr = NULL;

  enum MCLogicEnum FMode = mclAND;

  if ((J == NULL) || (RQ == NULL) || (MUStrIsEmpty(String)))
    {
    return(FAILURE);
    }

  MUStrDup(&Value,String);

  OrPtr = strchr(Value,'|');
  AndPtr = strchr(Value,'&');

  /* look for OR, otherwise, default to AND */

  if ((OrPtr != NULL) && (AndPtr != NULL))
    {
    /* use whichever came first */

    if (OrPtr > AndPtr)
      FMode = mclOR;
    else
      FMode = mclAND;
    }
  else if (OrPtr != NULL)
    {
    FMode = mclOR;
    }

  /* build feature strings and set feature bitmaps */

  mstring_t FeatureList(MMAX_LINE);

  ptr = MUStrTok(Value,":&|",&TokPtr);

  while (ptr != NULL)
    {
    if (ptr[0] != '!')
      {
      MStringAppendF(&FeatureList,"%s%c",ptr,(FMode == mclOR) ? '|' : '&');
      }

    ptr = MUStrTok(NULL,":&|",&TokPtr);
    }  /* END if ((ptr != NULL) && (ptr[0] != '!')) */

  if (!FeatureList.empty())
    {
    for (rqcounter = 0;J->Req[rqcounter] != NULL;rqcounter++)
      {
      if ((RQ != NULL) && (J->Req[rqcounter] != RQ))
        continue;

      if (J->Req[rqcounter]->DRes.Procs != 0)
        {
        MReqSetAttr(J,J->Req[rqcounter],mrqaSpecNodeFeature,(void **)FeatureList.c_str(),mdfString,Mode);
        }
      }
    }  /* END if (!MUStrIsEmpty(FeatureList.c_str())) */

  MUFree(&Value);
  MUStrDup(&Value,String);

  FeatureList.clear();  /* reset string */

  ptr = MUStrTok(Value,":&|",&TokPtr);

  while (ptr != NULL)
    {
    if (ptr[0] == '!')
      {
      MStringAppendF(&FeatureList,"%s%c",&ptr[1],(FMode == mclOR) ? '|' : '&');
      }

    ptr = MUStrTok(NULL,":&|",&TokPtr);
    }

  if (!MUStrIsEmpty(FeatureList.c_str()))
    {
    for (rqcounter = 0;J->Req[rqcounter] != NULL;rqcounter++)
      {
      if ((RQ != NULL) && (J->Req[rqcounter] != RQ))
        continue;

      if (J->Req[rqcounter]->DRes.Procs != 0)
        {
        MReqSetAttr(J,J->Req[rqcounter],mrqaExclNodeFeature,(void **)FeatureList.c_str(),mdfString,Mode);
        }
      }
    }  /* END if (!MUStrIsEmpty(FeatureList.c_str())) */

  MUFree(&Value);

  return(SUCCESS);
  }   /* END MReqSetFBM() */






/**
 * Set job 'req' (requirement) attribute.
 *
 * @see MJobSetAttr() - peer - set job attribute
 * @see MReqAToMString() - peer - converts req attribute to string
 *
 * @param J (I)
 * @param RQ (I) [modified]
 * @param AIndex (I)
 * @param Value (I)
 * @param Format (I)
 * @param Mode (I)
 */

int MReqSetAttr(

  mjob_t                 *J,
  mreq_t                 *RQ,
  enum MReqAttrEnum       AIndex,
  void                  **Value,
  enum MDataFormatEnum    Format,
  enum MObjectSetModeEnum Mode)
 
  {
  const char *FName = "MReqSetAttr";

  MDB(7,fSCHED) MLog("%s(%s,%d,%s,%s,%u,%s)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (RQ != NULL) ? RQ->Index : -1,
    (Value != NULL) ? "Value" : "NULL",
    MReqAttr[AIndex],
    Format,
    MObjOpType[Mode]);

  if ((J == NULL) || (RQ == NULL))
    {
    return(FAILURE);
    }

  switch (AIndex)
    {
    case mrqaAllocNodeList:

      /* supports string or other (mnl_t    ) */

      {
      int ReqTC;
      int ReqNC;

      int nindex;

      if (Value == NULL)
        {
        if (!MNLIsEmpty(&RQ->NodeList))
          MNLClear(&RQ->NodeList);

        break;
        }

      if (Format == mdfString)
        {
        char *ptr;
        char *TokPtr;
       
        char *tail;

        int TC;

        mnode_t *N;

        /* FORMAT:  <HOST>[:<X>|:ppn=<X>][[{+ \t\n;,}<HOST>[:<X>|:ppn=<X>]]... */

        ptr = MUStrTok((char *)Value,"+ \t\n;,",&TokPtr);

        nindex = 0;

        while (ptr != NULL)
          {
          tail = strchr(ptr,':');

          if (tail != NULL)
            *tail = '\0';

          if (MUHTIsInitialized(&MSched.VMTable) == TRUE)
            {
            if (MVMFind(ptr,NULL) == SUCCESS)
              {
              /* checkpoint file is corrupt - VM's listed as physical servers
                 ignore data */

              MDB(1,fSCHED) MLog("ALERT:    alloc list for job %s is corrupt - ignoring (VM '%s' reported as physical server\n",
                J->Name,
                ptr);

              return(FAILURE);
              }
            }

          if (MNodeAdd(ptr,&N) == FAILURE)
            {
            /* error:  cannot add allocated node */
 
            return(FAILURE);
            }

          MNLSetNodeAtIndex(&RQ->NodeList,nindex,N);

          TC = 1;

          if (tail != NULL)
            {
            if (!strcmp(tail + 1,"ppn="))
              {
              tail += strlen("ppn=") + 1;
              }
            else
              {
              tail ++;
              }
 
            TC = (int)strtol(tail,NULL,10);
            }

          MNLSetTCAtIndex(&RQ->NodeList,nindex,TC);

          nindex++;

          ptr = MUStrTok(NULL,"+ \t\n;,",&TokPtr);
          }  /* END while (ptr != NULL) */

        MNLTerminateAtIndex(&RQ->NodeList,nindex);
        }
      else 
        {
        mnl_t *NL;
       
        NL = (mnl_t *)Value;

        if (NL == NULL)
          {
          MNLClear(&RQ->NodeList);
 
          break;
          }

        MNLCopy(&RQ->NodeList,NL);
        }  /* END else (Format == mdfString) */

      ReqTC = 0;
      ReqNC = 0;

      for (nindex = 0;MNLGetNodeAtIndex(&RQ->NodeList,nindex,NULL) == SUCCESS;nindex++)
        {
        ReqTC += MNLGetTCAtIndex(&RQ->NodeList,nindex);
        ReqNC ++;
        }  /* END for (nindex) */      

      RQ->TaskCount = ReqTC;
      RQ->NodeCount = ReqNC;  
      }  /* END BLOCK (case mrqaAllocNodeList) */

      break;

    case mrqaGRes:

      {
      int gindex;
      char *GResPtr = NULL;
      char *GTokPtr = NULL;

      /* FORMAT:  <GRES>[:<COUNT>][,<GRES>[:<COUNT>]]... */

      GResPtr = MUStrTok((char *)Value," ,",&GTokPtr);

      /* remove any existing GRes since this is overriding any previous GRes settings */
         
      if ((GResPtr != NULL) && (!MSNLIsEmpty(&RQ->DRes.GenericRes)))
        {
        /* GRes specified and job's Req[0] is populated with one or more GRes */

        MSNLClear(&RQ->DRes.GenericRes);

        MDB(7,fSCHED) MLog("INFO:     job %s removing all GRes on req %d\n",
          J->Name,
          RQ->Index);
        }

      /* parse each GRes attribute and associated count and add it to the job */

      while (GResPtr != NULL)
        {
        int Count = 1;
        char *CPtr;

        CPtr = strchr((char *)GResPtr,':');

        if (CPtr != NULL)
          {
          *CPtr++ = 0;

          Count = (unsigned int)strtol(CPtr,NULL,10);
          }

        if ((gindex = MUMAGetIndex(
            meGRes,
            GResPtr,
            (MSched.EnforceGRESAccess == TRUE) ? mVerify : mAdd)) == 0)
          {
          MDB(1,fSTRUCT) MLog("ALERT:    cannot add requested generic resource '%s' to job %s\n",
            GResPtr,
            J->Name);

          GResPtr = MUStrTok(NULL," ,",&GTokPtr);

          continue;
          }

        if (MGRes.GRes[gindex]->FeatureGRes == TRUE)
          {
          int sindex = MUGetNodeFeature(GResPtr,(MSched.EnforceGRESAccess == TRUE) ?  mVerify : mAdd,NULL);

          if (sindex == 0)
            {
            MDB(1,fSTRUCT) MLog("ALERT:    cannot add requested feature gres '%s' to job %s\n",
              GResPtr,
              J->Name);

            return(FAILURE);
            }
          else
            {
            bmset(&J->ReqFeatureGResBM,sindex);

            return(SUCCESS);
            }
          }

        MDB(7,fSCHED) MLog("INFO:     job '%s:%d' adding GRes '%s' with count %d\n",
          J->Name,
          RQ->Index,
          MGRes.Name[gindex],
          Count);

        MSNLSetCount(&RQ->DRes.GenericRes,gindex,MSNLGetIndexCount(&RQ->DRes.GenericRes,gindex) + Count);

        GResPtr = MUStrTok(NULL," ,",&GTokPtr);
        } /* END while (GResPtr != NULL) */
      }   /* END BLOCK (case mrqaGRes) */

      break;

    case mrqaRMName:

      {
      mrm_t *R;

      RQ->RMIndex = (MRMFind((char*)Value,&R) == SUCCESS) ? R->Index : -1;
      }

      break;

    case mrqaReqArch:

      if ((RQ->Arch = MUMAGetIndex(meArch,(char *)Value,mAdd)) == FAILURE)
        {
        return(FAILURE);
        }

      break;

    case mrqaReqDiskPerTask:

      {
      int tmpI;

      if (NULL == Value)
        return(FAILURE);

      if (Format == mdfInt)
        tmpI = *(int *)Value;
      else
        tmpI = (int)strtol((char *)Value,NULL,0);

      RQ->DRes.Disk = tmpI;
      }  /* END BLOCK */

      break;

    case mrqaReqImage:

      if ((RQ->OptReq != NULL) || 
         ((RQ->OptReq = (moptreq_t *)MUCalloc(1,sizeof(moptreq_t))) != NULL))
        {
        MUStrDup(&RQ->OptReq->ReqImage,(char *)Value);
        }

      break;

    case mrqaReqMemPerTask:

      {
      long tmpL;

      if (NULL == Value)
        return(FAILURE);

      if (Format == mdfInt)
        tmpL = (long) *(int *)Value;
      else
        tmpL = MURSpecToL((char*)Value,mvmMega,mvmMega);

      RQ->DRes.Mem = tmpL;
      }  /* END BLOCK */

      break;

    case mrqaReqSwapPerTask:

      {
      long tmpL;

      if (NULL == Value)
        return(FAILURE);

      if (Format == mdfInt)
        tmpL = (long) *(int *)Value;
      else
        tmpL = MURSpecToL((char*)Value,mvmMega,mvmMega);

      RQ->DRes.Swap = tmpL;
      }  /* END BLOCK */

      break;

    case mrqaReqNodeDisk:

      if (NULL == Value)
        return(FAILURE);

      if (Format == mdfInt)
        RQ->ReqNR[mrDisk] = *(int *)Value;
      else
        RQ->ReqNR[mrDisk] = (int)strtol((char *)Value,NULL,0);

      break;
 
    case mrqaNodeAccess:

      {
      enum MNodeAccessPolicyEnum tmpI;

      tmpI = (enum MNodeAccessPolicyEnum)MUGetIndexCI(
        (char *)Value,
        MNAccessPolicy,
        FALSE,
        mnacNONE);

      if (tmpI != mnacNONE)
        {
        RQ->NAccessPolicy = tmpI;

        MDB(7,fCONFIG) MLog("INFO:     job %s node access policy set to %s\n",
          J->Name,
          MNAccessPolicy[RQ->NAccessPolicy]);
        }
      }  /* END BLOCK */

      break;

    case mrqaReqNodeFeature:
    case mrqaSpecNodeFeature:
    case mrqaExclNodeFeature:

      /* NOTE: SpecFBM changes are reflected in ReqFBM */

      {
      char *ptr;
      char *TokPtr;

      /* assume string */

      /* FORMAT:  <FEATURE>[{|:, \t\n[}<FEATURE>{]}]... */

      if (Mode == mSet)
        {
        if ((AIndex == mrqaReqNodeFeature) || (AIndex == mrqaSpecNodeFeature))
          bmclear(&RQ->ReqFBM);

        if (AIndex == mrqaSpecNodeFeature)
          bmclear(&RQ->SpecFBM);

        if (AIndex == mrqaExclNodeFeature)
          bmclear(&RQ->ExclFBM);

        /* set Mode to mAdd so we don't clobber each individual entry */

        Mode = mAdd;
        }

      if (Format != mdfString)
        {
        /* assume we were handed another FBM */

        bmor(&RQ->ReqFBM,(mbitmap_t *)Value);
        }
      else if (Value != NULL)
        {
        if (strchr((char *)Value,'|'))
          {
          if (AIndex != mrqaExclNodeFeature) 
            {
            RQ->ReqFMode = mclOR;
            }
          }

        if (!strchr((char *)Value,'&')) 
          {
          if (AIndex == mrqaExclNodeFeature)
            {
            RQ->ExclFMode = mclOR;   
            }
          }

        MDB(7,fSTRUCT) MLog("INFO:     setting job %s:%d %snode feature to %s (op=%s)\n",
          J->Name,
          RQ->Index,
          (AIndex == mrqaExclNodeFeature ? "excluded " : ""),
          (char *)Value,
          MObjOpType[Mode]);

        ptr = MUStrTok((char *)Value,"&|:, []\t\n",&TokPtr);

        while (ptr != NULL)
          {
          if (AIndex == mrqaExclNodeFeature) 
            {
            MUGetNodeFeature(ptr,Mode,&RQ->ExclFBM);
            }
          else if ((AIndex == mrqaReqNodeFeature) || (AIndex == mrqaSpecNodeFeature))
            {
            MUGetNodeFeature(ptr,Mode,&RQ->ReqFBM); 
            }

          if (AIndex == mrqaSpecNodeFeature)
            MUGetNodeFeature(ptr,Mode,&RQ->SpecFBM); 
      
          ptr = MUStrTok(NULL,"&|:, []\t\n",&TokPtr);
          }
        }    /* END if (Value != NULL) */

      if (MSched.NodeToJobAttrMap[0][0] != '\0')
        {
        MJobUpdateFlags(J);
  
        MJobBuildCL(J);
        }
      }    /* END BLOCK (case mrqaReqNodeFeature) */
 
      break;

    case mrqaPref:

      if (Value != NULL)
        {
        enum MNodeAttrEnum PrefNAttr = mnaNONE;

        char *ptr;
        char *TokPtr = NULL;

        MDB(7,fSTRUCT) MLog("INFO:     setting job %s:%d node pref feature to %s (op=%s)\n",
          J->Name,
          RQ->Index,
          (char *)Value,
          MObjOpType[Mode]);

        /* FORMAT:  [{feature|variable|gres}:]<ATTR>[=<VAL>] */

        ptr = MUStrTok((char *)Value,"|:, []\t\n",&TokPtr);

        if (RQ->Pref == NULL)
          {
          if ((RQ->Pref = (mpref_t *)MUCalloc(1,sizeof(mpref_t))) == NULL)
            {
            return(FAILURE);
            }
          }

        if (!strcasecmp(ptr,"variable"))
          {
          PrefNAttr = mnaVariables;

          ptr = MUStrTok(NULL,"|:, []\t\n",&TokPtr);
          }
        else if (!strcasecmp(ptr,"gres"))
          {
          PrefNAttr = mnaAvlGRes;

          ptr = MUStrTok(NULL,"|:, []\t\n",&TokPtr);
          }
        else if (!strcasecmp(ptr,"feature"))
          {
          PrefNAttr = mnaFeatures;

          ptr = MUStrTok(NULL,"|:, []\t\n",&TokPtr);
          }
        else
          {
          PrefNAttr = mnaFeatures;
          }

        while (ptr != NULL)
          {
          switch (PrefNAttr)
            {
            case mnaFeatures:

              MUGetNodeFeature(ptr,Mode,&RQ->Pref->FeatureBM);

              break;

            case mnaAvlGRes:

              /* NYI */

              break;

            case mnaVariables:

              if (MULLAdd(&RQ->Pref->Variables,ptr,NULL,NULL,MUFREE) == FAILURE)
                {
                MDB(1,fSCHED) MLog("ERROR:    cannot set pref variable for job %s (no memory)\n",
                  J->Name);

                return(FAILURE);
                }

              break;
           
            default:

              /* NOT SUPPORTED */

              /* NO-OP */

              break;
            }  /* END switch (PrefNAttr) */

          ptr = MUStrTok(NULL,"|:, []\t\n",&TokPtr);
          }  /* END while (ptr != NULL) */
        }    /* END if (Value != NULL) */

      break;

    case mrqaReqNodeProc:

      if (NULL == Value)
        return(FAILURE);

      if (Format == mdfInt)
        RQ->ReqNR[mrProc] = *(int *)Value;
      else
        MReqResourceFromString(RQ,mrProc,(char *)Value);

      break;

    case mrqaReqNodeMem:

      if (NULL == Value)
        return(FAILURE);

      if (Format == mdfInt)
        RQ->ReqNR[mrMem] = *(int *)Value;
      else
        MReqResourceFromString(RQ,mrMem,(char *)Value);

      break;

    case mrqaReqNodeSwap:

      if (NULL == Value)
        return(FAILURE);

      if (Format == mdfInt)
        RQ->ReqNR[mrSwap] = *(int *)Value;
      else
        MReqResourceFromString(RQ,mrSwap,(char *)Value);

      break;

    case mrqaReqNodeAMem:

      if (NULL == Value)
        return(FAILURE);

      if (Format == mdfInt)
        RQ->ReqNR[mrAMem] = *(int *)Value;
      else
        MReqResourceFromString(RQ,mrAMem,(char *)Value);

      break;

    case mrqaReqNodeASwap:

      if (NULL == Value)
        return(FAILURE);

      if (Format == mdfInt)
        RQ->ReqNR[mrASwap] = *(int *)Value;
      else
        MReqResourceFromString(RQ,mrASwap,(char *)Value);

      break;

    case mrqaReqProcPerTask:

      if (NULL == Value)
        return(FAILURE);

      if (Format == mdfInt)
        RQ->DRes.Procs = *(int *)Value;
      else
        {
        char *tail;

        RQ->DRes.Procs = (int)strtol((char *)Value,&tail,10);

        if ((tail != NULL) && (tail[0] == '*')) 
          {
          /* needed for xt sizezero jobs to show correctly when exported to the grid head */
          bmset(&J->IFlags,mjifDProcsSpecified);
          }
        }

      break;

    case mrqaNCReqMin:

      if (NULL == Value)
        return(FAILURE);

      {
      int tmpI;
      int Delta;

      if (Format == mdfInt)
        tmpI = *(int *)Value;
      else if (Format == mdfLong)
        tmpI = (int)(*(long *)Value);
      else 
        tmpI = (int)strtol((char *)Value,NULL,10);

      if (tmpI > 0)
        {
        Delta = tmpI - RQ->NodeCount;

        RQ->NodeCount = tmpI;

        if (Delta != 0)
          J->Request.NC = MAX(0,J->Request.NC + Delta);
        else
          J->Request.NC = tmpI;

        }
      }    /* END BLOCK (case mrqaNCReqMin) */

      break;

    case mrqaReqOpsys:

      {
      int tmpI;

      if (NULL == Value)
        return(FAILURE);

      if (Format == mdfInt)
        {
        tmpI = *(int *)Value;
        }
      else
        {
        /* assume string format */

        tmpI = MUMAGetIndex(meOpsys,(char *)Value,mVerify);
        }

      if ((tmpI <= 0) || (tmpI > MMAX_ATTR))
        {
        return(FAILURE);
        }

      RQ->Opsys = tmpI;
      }  /* END BLOCK (case mrqaReqOpsys) */

      break;

    case mrqaReqPartition:

      {
      mpar_t *tmpP;

      if (NULL == Value)
        return(FAILURE);

      if (MParFind((char *)Value,&tmpP) == FAILURE)
        {
        return(FAILURE);                
        }

      RQ->PtIndex = tmpP->Index;
      }  /* END BLOCK */

      break;

    case mrqaReqSoftware:

      if (Format == mdfString)
        {
        /* add new req */

        int   rqindex;
        int   index;
        int   pindex;

        char tmpLine[MMAX_LINE];
  
        char *ptr;
        char *ptr2;
        char *TokPtr;

        long  TimeFrame;
        int   tmpS;

        mreq_t *tmpRQPtr;

        /* NOTE:  RQ->Appl supports provisioning, GRES does not */

        MUStrCpy(tmpLine,(char *)Value,sizeof(tmpLine));

        /* NOTE:  RQ->Appl supports provisioning, GRES does not */

        ptr = MUStrTok(tmpLine,",%",&TokPtr);

        while (ptr != NULL)
          {
          if ((ptr2 = strchr(ptr,'+')) != NULL)
            {
            tmpS = (unsigned int)strtol(&ptr2[1],NULL,10);
            }
          else if ((ptr2 = strchr(ptr,':')) != NULL)
            {
            tmpS = (unsigned int)strtol(&ptr2[1],NULL,10); 
            }
          else
            {
            tmpS = 1;
            }

          /* check to see if we need to put a trigger on this resource */
          if ((ptr2 = strchr(ptr,'@')) != NULL)
            {
            TimeFrame = MUTimeFromString(&ptr2[1]);
            }
          else
            {
            TimeFrame = -1;
            }

          /* remove end of ptr so MUMAGetIndex will work */
          for (pindex=0; pindex<MMAX_LINE; pindex++)
            {
            if (ptr[pindex] == '+' || ptr[pindex] == ':')
              {
              ptr[pindex] = '\0';
              break;
              }
            }

          if ((index = MUMAGetIndex(meGRes,ptr,(MSched.EnforceGRESAccess == TRUE) ? mVerify : mAdd)) == 0)
            {
            MDB(1,fSCHED) MLog("ALERT:    cannot add support for GRes software '%s'\n",
              ptr);

            continue;
            }

          /* verify resource not already added */

          for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
            {
            if (rqindex == MMAX_REQ_PER_JOB)
              break;

            if (MSNLGetIndexCount(&J->Req[rqindex]->DRes.GenericRes,index) == tmpS)
              {
              break;
              }
            }    /* END for (rqindex) */

          if ((rqindex != MMAX_REQ_PER_JOB) && 
              (J->Req[rqindex] != NULL) &&
              (MSNLGetIndexCount(&J->Req[rqindex]->DRes.GenericRes,index) == tmpS))
            {
            break;
            }

          /* trigger checking */

          if (TimeFrame > 0)
            {
            if (MJOBISACTIVE(J) &&
                (MSched.Time - J->StartTime > (unsigned long)TimeFrame))
              {
              /* ignore this request */
              continue;
              }
            else
              {
              mtrig_t *T;
              marray_t TList;
              char tmpStr[MMAX_LINE];

              snprintf(tmpStr,sizeof(tmpLine),"atype=internal,etype=start,offset=%ld,action=\"job:-:modify:gres-=%s\"",
                TimeFrame,
                ptr);

              MUArrayListCreate(&TList,sizeof(mtrig_t *),10);

              if (MTrigLoadString(
                    &J->Triggers,
                    tmpStr,
                    TRUE,
                    FALSE,
                    mxoJob,
                    J->Name,
                    &TList,
                    NULL) == FAILURE)
                {
                continue;
                }

              T = (mtrig_t *)MUArrayListGetPtr(&TList,0);
              T->O = (void *)J;
              MUStrDup(&T->OID,J->Name);
              MTrigInitialize(T);
              MUArrayListFree(&TList);
              } /* END else (TimeFrame > 0) */
            } /* END if ((TimeFrame > 0) && MJOBISACTIVE(J)) */

          /* check if resource exists on the global node */

          if ((MSched.GN != NULL) && (MSNLGetIndexCount(&MSched.GN->CRes.GenericRes,index) > 0))
            {
            if (MReqCreate(J,NULL,&tmpRQPtr,FALSE) == FAILURE)
              {
              continue;
              }

            MSNLSetCount(&tmpRQPtr->DRes.GenericRes,index,tmpS);

            tmpRQPtr->TaskCount          = 1;
            tmpRQPtr->TaskRequestList[0] = 1;
            tmpRQPtr->TaskRequestList[1] = 1;

            /* represents all non-compute nodes */

            J->FloatingNodeCount++;
            }
          else
            {
            MSNLSetCount(&RQ->DRes.GenericRes,index,tmpS);
            } 

          ptr = MUStrTok(NULL,",%",&TokPtr);

          } /* end while (ptr != NULL)*/ 
        } /* END else if (Format == mdfString) */

      break;

    case mrqaTCReqMin:

      {
      int tmpI;
      int Delta;

      if (NULL == Value)
        return(FAILURE);

      if (Format == mdfInt)
        tmpI = *(int *)Value;
      else if (Format == mdfLong)
        tmpI = (int)(*(long *)Value);
      else
        {
        char *tail;

        tmpI = (int)strtol((char *)Value,&tail,10);

        if ((tail != NULL) && (tail[0] == '*'))
          {
          /* needed for xt sizezero jobs to show correctly when exported to the grid head */
          bmset(&J->IFlags,mjifTasksSpecified);
          }
        }

      if (tmpI > 0)
        {
        if ((bmisset(&J->IFlags,mjifTaskCountIsDefault)) && (RQ->Index == 0))
          {
          J->Request.TC = 0;

          bmunset(&J->IFlags,mjifTaskCountIsDefault);
          }
            
        Delta = tmpI - RQ->TaskCount;

        RQ->TaskRequestList[0] = tmpI;
        RQ->TaskRequestList[1] = tmpI;
        RQ->TaskRequestList[2] = 0;

        RQ->TaskCount = tmpI;

        J->Request.TC = MAX(0,J->Request.TC + Delta);
        }
      }    /* END BLOCK (case mrqaTCReqMin) */

      break;

    case mrqaTPN:

      {
      int tmpI;

      if (Value == NULL)
        {
        RQ->TasksPerNode = 0;

        break;
        }

      if (Format == mdfInt)
        tmpI = *(int *)Value;
      else if (Format == mdfLong)
        tmpI = (int)(*(long *)Value);
      else
        tmpI = (int)strtol((char *)Value,NULL,10);

      if (tmpI > 0)
        {
        RQ->TasksPerNode = tmpI;
        }
      }    /* END BLOCK */

      break;

    default:

      /* attribute not supported */

      return(FAILURE);
   
      /*NOTREACHED*/

      break;
    }  /* END switch (AIndex) */

  return(SUCCESS);
  }  /* END MReqSetAttr() */





/**
 * Report req attribute as string.
 *
 * @see MReqSetAttr() - peer
 * @see MJobAToMString()
 *
 * @param J (I)
 * @param RQ (I)
 * @param AIndex (I)
 * @param Buf (O)
 * @param Mode (I)
 */

int MReqAToMString(

  mjob_t              *J,       /* I */
  mreq_t              *RQ,      /* I */
  enum MReqAttrEnum    AIndex,  /* I */
  mstring_t           *Buf,     /* O */
  enum MFormatModeEnum Mode)    /* I */

  {
  if (Buf == NULL)
    {
    return(FAILURE);
    }

  if ((J == NULL) || (RQ == NULL))
    {
    return(FAILURE);
    }

  Buf->clear();

  switch (AIndex)
    {
    case mrqaAllocNodeList:

      if (MJOBISACTIVE(J) || MJOBISCOMPLETE(J))
        {
        MNLToMString(&RQ->NodeList,MBNOTSET,",",'\0',-1,Buf);
        }    /* END BLOCK (case mrqaAllocNodeList) */

      break;

    case mrqaAllocPartition:

      {
      mnode_t *N;

      if (!MJOBISACTIVE(J) && !MJOBISCOMPLETE(J))
        break;

      if (!MNLIsEmpty(&RQ->NodeList))
        {
        MNLGetNodeAtIndex(&RQ->NodeList,0,&N);

        MStringSet(Buf,MPar[N->PtIndex].Name);
        }
      else if (!MNLIsEmpty(&J->NodeList))
        {
        MNLGetNodeAtIndex(&J->NodeList,0,&N);

        MStringSet(Buf,MPar[N->PtIndex].Name);
        }
      }  /* END BLOCK */

      break;

    case mrqaIndex:

      MStringSetF(Buf,"%d",
        RQ->Index);

      break;

    case mrqaRMName:

      if (RQ->RMIndex >= 0)
        {
        MStringSetF(Buf,"%s",MRM[RQ->RMIndex].Name);
        }

      break;

    case mrqaNCReqMin:

      if (RQ->NodeCount > 0)
        {
        MStringSetF(Buf,"%d",
          RQ->NodeCount);
        }

      break;

    case mrqaNodeAccess:

      if (RQ->NAccessPolicy != mnacNONE)
        MStringSet(Buf,(char *)MNAccessPolicy[RQ->NAccessPolicy]);

      break;

    case mrqaNodeSet:

      {
      int sindex;

      if ((RQ->SetList != NULL) && 
          ((RQ->SetSelection != mrssNONE) ||
           (RQ->SetType != mrstNONE) ||
           (RQ->SetList[0] != NULL)))
        {
        MStringSetF(Buf,"%s:%s:",
          MResSetSelectionType[RQ->SetSelection],
          MResSetAttrType[RQ->SetType]);

        for (sindex = 0;RQ->SetList[sindex] != NULL;sindex++)
          {
          if (sindex > 0)
            MStringAppend(Buf,",");

          MStringAppend(Buf,RQ->SetList[sindex]);
          }  /* END for (sindex) */

        if (sindex == 0)
          MStringAppend(Buf,NONE);
        }  /* END if ((RQ->SetSelection != mrssNONE) || ...) */
      }    /* END BLOCK */

      break;

    case mrqaPref:

      if (RQ->Pref != NULL)
        {
        char tmpLine[MMAX_LINE];

        if (!bmisclear(&RQ->Pref->FeatureBM))
          {
          MUNodeFeaturesToString(',',&RQ->Pref->FeatureBM,tmpLine);
   
          MStringAppendF(Buf,"feature:%s",
            tmpLine);
          }

        if (RQ->Pref->Variables != NULL)
          {
          tmpLine[0] = '\0';

          MULLToString(
            RQ->Pref->Variables,
            FALSE,
            NULL,
            tmpLine,
            sizeof(tmpLine));

          if (!MUStrIsEmpty(tmpLine))
            {
            MStringAppendF(Buf,"%svariable:%s",
              (!Buf->empty()) ? "," : "",
              tmpLine);
            }
          }
        }    /* END if (RQ->Pref != NULL) */

      break;

    case mrqaReqArch:

      if ((RQ->Arch != 0) && strcmp(MAList[meArch][RQ->Arch],NONE))
        MStringSet(Buf,MAList[meArch][RQ->Arch]);

      break;

    case mrqaReqAttr:

      /* Not-Supported, this attribute prints out in XML only */

      break;

    case mrqaReqDiskPerTask:

      if (RQ->DRes.Disk > 0)
        {
        MStringSetF(Buf,"%d",
          RQ->DRes.Disk);
        }

      break;

    case mrqaReqImage:

      /* NYI */

      break;

    case mrqaReqMemPerTask:

      if (RQ->DRes.Mem > 0)
        {
        MStringSetF(Buf,"%d",
          RQ->DRes.Mem);
        }

      break;

    case mrqaReqNodeDisk:

      if (RQ->ReqNR[mrDisk] != 0)
        {
        MStringSetF(Buf,"%s%d",
          (RQ->ReqNRC[mrDisk] != 0) ? MComp[(int)RQ->ReqNRC[mrDisk]] : "",
          RQ->ReqNR[mrDisk]);
        }

      break;

    case mrqaReqNodeFeature:

      if (!bmisclear(&RQ->ReqFBM))
        {
        char tmpLine[MMAX_LINE];
        char Delim = '\0';

        if (RQ->ReqFMode == mclOR)
          {
          Delim = '|';
          }

        MUNodeFeaturesToString(Delim,&RQ->ReqFBM,tmpLine);

        MStringSet(Buf,tmpLine);
        }   /* END BLOCK (case mrqaReqNodeFeature) */

      break;

    case mrqaExclNodeFeature:

      if (!bmisclear(&RQ->ExclFBM))
        {
        char tmpLine[MMAX_LINE];
        char Delim = '\0';

        if (RQ->ExclFMode == mclAND)
          {
          Delim = '&';
          }

        MUNodeFeaturesToString(Delim,&RQ->ExclFBM,tmpLine);

        MStringSet(Buf,tmpLine);
        }  /* END case mrqaExclNodeFeature */

      break;

    case mrqaReqNodeMem:

      if ((RQ->ReqNR[mrMem] != 0) || (RQ->ReqNRC[mrMem] != mcmpNONE))
        {
        MStringSetF(Buf,"%s%d",
          (RQ->ReqNRC[mrMem] != 0) ? MComp[(int)RQ->ReqNRC[mrMem]] : "",
          RQ->ReqNR[mrMem]);
        }

      break;

    case mrqaReqNodeAMem:

      if ((RQ->ReqNR[mrAMem] != 0) || (RQ->ReqNRC[mrAMem] != mcmpNONE))
        {
        MStringSetF(Buf,"%s%d",
          (RQ->ReqNRC[mrAMem] != 0) ? MComp[(int)RQ->ReqNRC[mrAMem]] : "",
          RQ->ReqNR[mrAMem]);
        }

      break;

    case mrqaReqNodeProc:

      if ((RQ->ReqNR[mrProc] != 0) || (RQ->ReqNRC[mrProc] != mcmpNONE))
        {
        MStringSetF(Buf,"%s%d",
          (RQ->ReqNRC[mrProc] != 0) ? MComp[(int)RQ->ReqNRC[mrProc]] : "",
          RQ->ReqNR[mrProc]);
        }

      break;

    case mrqaReqNodeSwap:

      if ((RQ->ReqNR[mrSwap] != 0) || (RQ->ReqNRC[mrSwap] != mcmpNONE))
        {
        MStringSetF(Buf,"%s%d",
          (RQ->ReqNRC[mrSwap] != 0) ? MComp[(int)RQ->ReqNRC[mrSwap]] : "",
          RQ->ReqNR[mrSwap]);
        }

      break;

    case mrqaReqNodeASwap:

      if ((RQ->ReqNR[mrASwap] != 0) || (RQ->ReqNRC[mrASwap] != mcmpNONE))
        {
        MStringSetF(Buf,"%s%d",
          (RQ->ReqNRC[mrASwap] != 0) ? MComp[(int)RQ->ReqNRC[mrASwap]] : "",
          RQ->ReqNR[mrASwap]);
        }

      break;

    case mrqaReqOpsys:

      if (RQ->Opsys != 0)
        {
        MStringSet(Buf,MAList[meOpsys][RQ->Opsys]);
        }

      break;

    case mrqaReqPartition:

      if (RQ->PtIndex > 0)
        {
        MStringSet(Buf,MPar[RQ->PtIndex].Name);
        }

      break;

    case mrqaReqProcPerTask:

      if ((RQ->DRes.Procs != 0) ||
          bmisset(&J->IFlags,mjifDProcsSpecified)) 
        {
        /* mjifDProcsSpecified is needed for xt sizezero jobs to show
         * correctly when exported to the grid head */

        MStringSetF(Buf,"%d%s",
          RQ->DRes.Procs,
          bmisset(&J->IFlags,mjifDProcsSpecified) ? "*" : "");
        }

      break;

    case mrqaReqSoftware:

      /* NYI */

      break;

    case mrqaReqSwapPerTask:

      if (RQ->DRes.Swap > 0)
        {
        MStringSetF(Buf,"%d",
          RQ->DRes.Swap);
        }

      break;

    case mrqaTCReqMin:

      if (RQ->TaskRequestList[0] != 0)
        {
        /* mjifTasksSpecified is needed for xt sizezero jobs to show
         * correctly when exported to the grid head */

        MStringSetF(Buf,"%d%s",
          RQ->TaskRequestList[0],
          bmisset(&J->IFlags,mjifTasksSpecified) ? "*" : "");
        }

      break;

    case mrqaTPN:

      if (RQ->TasksPerNode > 0)
        {
        MStringSetF(Buf,"%d",
          RQ->TasksPerNode);
        }

      break;

    case mrqaGRes:
    case mrqaCGRes:

      {
      int Index;      
      int Count;     

      msnl_t* GRes;

      mbool_t First = TRUE;

      if (AIndex == mrqaCGRes)
        {
        if (RQ->CRes == NULL)
          break;

        GRes = &RQ->CRes->GenericRes;
        }
      else
        {
        GRes = &RQ->DRes.GenericRes;
        }

      Count = MSNLGetIndexCount(GRes,0);

      for (Index = 1; Index < MSched.M[mxoxGRes] && Count > 0; Index++)
        {
        if (MGRes.Name[Index][0] == '\0')
          break;

        if (MSNLGetIndexCount(GRes,Index) == 0)
          continue;

        MStringAppendF(Buf,"%s%s:%d",
          (First ? "" : ","),
          MGRes.Name[Index],
          MSNLGetIndexCount(GRes,Index));

        Count -= MSNLGetIndexCount(GRes,Index);

        First = FALSE;
        }

      if (!bmisclear(&J->ReqFeatureGResBM))
        {
        char  Line[MMAX_LINE];

        MUNodeFeaturesToString(',',&J->ReqFeatureGResBM,Line);

        MStringAppendF(Buf,"%s%s",
          (First ? "" : ","),
          Line);
        }
      }    /* END BLOCK (case mrqaGRes) */

      break;

    case mrqaDepend:
    case mrqaSpecNodeFeature:

      /* currently not required to report as string */

      /* NO-OP */

      break;

    case mrqaUtilMem:

      if ((RQ->URes != NULL) && (RQ->URes->Mem != 0))
        {
        MStringAppendF(Buf,"%d",RQ->URes->Mem);
        }

      break;

    case mrqaUtilProc:

      if ((RQ->URes != NULL) && (RQ->URes->Procs != 0))
        {
        MStringAppendF(Buf,"%d",RQ->URes->Procs);
        }

      break;

    case mrqaUtilSwap:

      if ((RQ->URes != NULL) && (RQ->URes->Swap != 0))
        {
        MStringAppendF(Buf,"%d",RQ->URes->Swap);
        }

      break;

    default:

      /* not handled */

      MDB(7,fSTRUCT) MLog("WARNING:  req attribute %d %s cannot be converted to string\n",
        AIndex,
        (MReqAttr[AIndex] != NULL) ? MReqAttr[AIndex] : "NULL");

      return(FAILURE);

      /*NOTREACHED*/

      break;
    }  /* END switch (AIndex) */

  return(SUCCESS);
  }  /* END MReqAToMString() */


/**
 * Determine if job Req 'prefers' specified node.
 *
 * @see MSched.ResourcePrefIsActive - routine only called if value is set
 * @see MJobAllocMNL() - parent
 * @see MNodeGetPriority() - peer
 *
 * @param RQ (I)
 * @param N (I)
 * @param IsPref (O)
 */

int MReqGetPref(

  mreq_t  *RQ,      /* I */
  mnode_t *N,       /* I */
  mbool_t *IsPref)  /* O */

  {
  if ((RQ == NULL) || (N == NULL) || (IsPref == NULL))
    {
    return(FAILURE);
    }

  *IsPref = (char)FALSE;

  if (RQ->Pref == NULL)
    {
    return(SUCCESS);
    }

  /* node is 'pref' if 'any' pref features are present in N->FBM */

  if (MAttrSubset(&N->FBM,&RQ->Pref->FeatureBM,mclOR) == SUCCESS)
    {
    *IsPref = (char)TRUE;

    return(SUCCESS);
    }

  if (RQ->Pref->Variables != NULL)
    {
    mln_t *ptr;

    for (ptr = RQ->Pref->Variables;ptr != NULL;ptr = ptr->Next)
      {
      if (MUHTGet(&N->Variables,ptr->Name,NULL,NULL) == TRUE)
        {
        *IsPref = (char)TRUE;

        return(SUCCESS);
        }
      }
    }

  /* check GRes - NYI */

  return(SUCCESS);
  }  /* END MReqGetPref() */


/**
 * Check if node resources match req constraints.
 *
 * NOTE:  if DoFeasibleCheck is FALSE, assume node already checked for feasibility.
 *
 * @see MReqGetFNL() - peer
 * @see MReqCheckResourceMatch() - child
 * @see MNodeDiagnoseEligibility() - parent
 * @see MNodeGetCfgTime() - child
 *
 * @param J (I) job
 * @param N (I) node
 * @param RQ (I) req
 * @param StartTime (I) time job must start
 * @param TCAvail (O) [optional] tasks allowed at StartTime (optional)
 * @param MinSpeed (I)
 * @param MinTC (I) [optional, <= 0 for not set]
 * @param RIndex (O) [required] node rejection index
 * @param Affinity (O) affinity status
 * @param AvailTime (O) [optional]
 * @param DoFeasibleCheck (I)
 * @param ForceVMEval     (I) Force evaluation of VM resources on Node instead of Node resources
 * @param BMsg (O) [optional,minsize=MMAX_NAME] block msg
 */

int MReqCheckNRes(

  const mjob_t       *J,
  const mnode_t      *N,
  const mreq_t       *RQ,
  long                StartTime,
  int                *TCAvail,
  double              MinSpeed,
  int                 MinTC,
  enum MAllocRejEnum *RIndex,
  char               *Affinity,
  long               *AvailTime,
  mbool_t             DoFeasibleCheck,
  mbool_t             ForceVMEval,
  char               *BMsg)

  {
  char AttrAffinity = mnmNONE;
  int MinTPN;

  int TC;

  const mcres_t *NDPtr = NULL;
  mcres_t  tmpNDRes;

  mbool_t  UseLocalRes = FALSE;

  char TString[MMAX_LINE];
  const char *FName = "MReqCheckNRes";

  MULToTString(StartTime - MSched.Time,TString);

  MDB(4,fSCHED) MLog("%s(%s,%s,RQ[%d],%s,TCAvail,%.3f,%d,RIndex,%s,AvailTime,%s,%s,BMsg)\n",
    FName,
    J->Name,
    N->Name,
    RQ->Index,
    TString,
    MinSpeed,
    MinTC,
    (Affinity == NULL) ? "NULL" : "Affinity",
    MBool[DoFeasibleCheck],
    MBool[ForceVMEval]);

  if (RIndex != NULL)
    *RIndex = marNONE;

  if (TCAvail != NULL)
    *TCAvail = 0;

  if (Affinity != NULL)
    *Affinity = mnmNONE;

  if (AvailTime != NULL)
    *AvailTime = 0;

  if (BMsg != NULL)
    BMsg[0] = '\0';

  if (DoFeasibleCheck == TRUE)
    {
    if (MReqCheckResourceMatch(J,RQ,N,RIndex,StartTime,ForceVMEval,NULL,&AttrAffinity,BMsg) == FAILURE)
      {
      if ((BMsg != NULL) && (RIndex != NULL))
        {
        switch (*RIndex)
          {
          case marOpsys:

            if (((N->OSList != NULL) && (N->OSList[0].AIndex > 0)) ||    
                ((MSched.DefaultN.OSList != NULL) && (MSched.DefaultN.OSList[0].AIndex > 0)))
              {
              int   oindex;

              char *BPtr;
              int   BSpace;

              mnodea_t *OSList = ((N->OSList != NULL) ? N->OSList : MSched.DefaultN.OSList);

              MUSNInit(&BPtr,&BSpace,BMsg,MMAX_LINE);

              MUSNCat(&BPtr,&BSpace,MAList[meOpsys][OSList[0].AIndex]);

              for (oindex = 1;OSList[oindex].AIndex != 0;oindex++)
                {
                if (OSList[oindex].CfgTime > 0)
                  {
                  MUSNPrintF(&BPtr,&BSpace,",%s",
                    MAList[meOpsys][OSList[oindex].AIndex]);
                  }
                }    /* END for (oindex) */
              }      /* END if (OSList != NULL) */
            else
              {
              strcpy(BMsg,MAList[meOpsys][N->ActiveOS]);
              }

            break;

          case marArch:

            strcpy(BMsg,MAList[meArch][N->Arch]);

            break;

          case marVM:

            strcpy(BMsg,"no vmcreate");

            break;

          default:

            /* NO-OP */

            break;
          }  /* END switch (*RIndex) */
        }    /* END if ((BMsg != NULL) && (RIndex != NULL)) */

      return(FAILURE);
      }  /* END if (MReqCheckResourceMatch(...) == FAILURE) */
    }    /* END if (DoFeasibleCheck == TRUE) */
  else if (RQ->ReqAttr.size() != 0)
    {
    MNodeCheckReqAttr(N,RQ,&AttrAffinity,NULL);
    }

  if (StartTime == (long)MSched.Time)
    {
    /* job is attempting to start now */

    const mcres_t *NA;
    const mcres_t *NC;
    mulong   ATime;

    mcres_t vmCRes;
    mcres_t vmARes;

    mbool_t  UsageIsExclusive;

    if (MNODEISUP(N) == FALSE)
      {
      /* node is unavailable */

      if (RIndex != NULL)
        *RIndex = marState;

      return(FAILURE);
      }

    if ((MNodeGetCfgTime(N,J,RQ,&ATime) == FAILURE) ||
       ((long)ATime > StartTime))
      {
      if (RIndex != NULL)
        *RIndex = marDynCfg;

      return(FAILURE);
      }

    /* NOTE:  J may be reservation pseudo-job or real job */

    if (MReqIsGlobalOnly(RQ))
      {
      /* if the job is only requesting global resources (licenses) then always use N->ARes */

      UsageIsExclusive = TRUE;
      }
    else if (!bmisset(&J->Flags,mjfPreemptor) && !bmisset(&J->Flags,mjfRsvMap))
      {
      UsageIsExclusive = TRUE;
      }
    else
      {
      /* FIXME: there is a problem here: MOAB-3903, we will use N->CRes instead of N->ARes
                because this job is a preemptor, but what about licenses? */

      /* preemptors may preempt active jobs */
      /* rsv's may overlay jobs with matching ACL's */

      UsageIsExclusive = FALSE;
      }

    /* check dynamic node attributes */

    if (UsageIsExclusive == TRUE)
      {
      mbool_t EStateIsUnavail;
      mbool_t StateIsUnavail;

      StateIsUnavail = ((N->State != mnsIdle) && (N->State != mnsActive));
      EStateIsUnavail = ((N->EState != mnsIdle) && (N->EState != mnsActive));

      if ((StateIsUnavail == TRUE) || (EStateIsUnavail == TRUE))
        {
        MDB(5,fSCHED) MLog("INFO:     node is in %s state '%s'\n",
          (StateIsUnavail == TRUE) ? "" : "expected",
          (StateIsUnavail == TRUE) ? MNodeState[N->State] : MNodeState[N->EState]);

        if (RIndex != NULL)
          *RIndex = marState;

        if (BMsg != NULL)
          {
          if (StateIsUnavail == TRUE)
            strcpy(BMsg,MNodeState[N->State]);
          else
            strcpy(BMsg,MNodeState[N->EState]);
          }

        return(FAILURE);
        }

      if ((N->RM == NULL) || 
          (N->RM->Type != mrmtMoab))
        {
        /* only following enforce policies for non-virtual nodes */

        if (((RQ->NAccessPolicy == mnacSingleJob) ||
            (RQ->NAccessPolicy == mnacSingleTask)) &&
            (MNodePolicyAllowsJAccess(N,J,MSched.Time,RQ->NAccessPolicy,NULL) == FALSE))
          {
          if ((N->State != mnsIdle) || (N->EState != mnsIdle))
            {
            MDB(5,fSCHED) MLog("INFO:     node is in%s state '%s' (dedicated access required)\n",
              (N->State != mnsIdle) ? "" : " expected",
              (N->State != mnsIdle) ? MNodeState[N->State] : MNodeState[N->EState]);

            if (RIndex != NULL)
              *RIndex = marState;

            if (BMsg != NULL)
              {
              if (N->State != mnsIdle)
                {
                sprintf(BMsg,"%s - dedicated access required",
                  MNodeState[N->State]);
                }
              else
                {
                sprintf(BMsg,"ESTATE:%s - dedicated access required",
                  MNodeState[N->EState]);
                }
              }

            return(FAILURE);
            }

          /* redundant test */

          if (N->ARes.Procs < MNodeGetBaseValue(N,mrlProc)) 
            {
            MDB(5,fSCHED) MLog("INFO:     inadequate procs on node '%s' (%d available)\n",
              N->Name,
              N->ARes.Procs);

            if (RIndex != NULL)
              *RIndex = marCPU;

            if (BMsg != NULL)
              {
              sprintf(BMsg,"%d procs utilized - dedicated access required",
                N->CRes.Procs - N->ARes.Procs);
              }

            return(FAILURE);
            }
          }    /* END if ((RQ->NAccessPolicy == mnacSingleJob) || ...) */
        }      /* END if ((N->RM == NULL) || (N->RM->Type != mrmtMoab)) */

      NA = &N->ARes;
      }      /* END if (!bmisset(&J->Flags,mjfPreemptor)) */
    else
      {
      /* job is preemptor or rsv map, all node resources may be accessible */

      /* MNodeGetTC() check will ignore node state and current node allocation */

      NA = &N->CRes;
      }

    /* NOTE:  resources already dedicated on node for suspended job */
    /* should subtract RQ->DRes*RQ->NodeList[x].TC from N->DRes */

    if ((MSched.TrackSuspendedJobUsage == TRUE) && 
        (MinTC > 0) && 
        (J->State == mjsSuspended))
      {
      UseLocalRes = TRUE;

      MCResInit(&tmpNDRes);

      MCResCopy(&tmpNDRes,&N->DRes);

      MCResRemove(&tmpNDRes,&N->CRes,&RQ->DRes,MinTC,FALSE);
      }
    else
      {
      NDPtr = &N->DRes;
      }
     
    NC = &N->CRes;

    if ((((MSched.VMsAreStatic == TRUE) && (MJOBREQUIRESVMS(J))) ||
         (ForceVMEval == TRUE)) && (!bmisset(&MSched.Flags,mschedfOutOfBandVMRsv)))
      {
      MNodeGetVMRes(N,J,TRUE,&vmCRes,NULL,&vmARes);

      NC = &vmCRes;
      NA = &vmARes;
      }

    TC = MNodeGetTC(
      N,
      NA,
      NC,
      (UseLocalRes == FALSE) ? NDPtr : &tmpNDRes,
      &RQ->DRes,
      (UsageIsExclusive == TRUE) ? StartTime : MMAX_TIME,
      1,
      RIndex);  /* O */

    if (UseLocalRes == TRUE)
      {
      MCResFree(&tmpNDRes);
      }

    if (MSched.ResOvercommitSpecified[0] == TRUE)
      {
      int tmpTC;

      MNodeGetAvailTC(N,J,RQ,TRUE,&tmpTC);

      TC = MAX(TC,tmpTC);
      }

    if ((TC <= 0) || ((MinTC >= 0) && (TC < MinTC)))
      {
      MDB(4,fSCHED) MLog("INFO:     node supports inadequate tasks (%d) for job %s:%d(%s)\n",
        TC,
        J->Name,
        RQ->Index,
        (RIndex != NULL) ? MAllocRejType[*RIndex] : "???");

      return(FAILURE);
      }

    if (TC < MAX(1,RQ->TasksPerNode))
      {
      MDB(4,fSCHED) MLog("INFO:     node supports inadequate tasks per node %d task%c (%d tasks/node required)\n",
        TC,
        (TC == 1) ? ' ' : 's',
        MAX(1,RQ->TasksPerNode));

      if (RIndex != NULL)
        *RIndex = marCPU;

      if (BMsg != NULL)
        {
        sprintf(BMsg,"inadequate tasks per node available");
        }

      return(FAILURE);
      }

    MDB(4,fSCHED) MLog("INFO:     node supports %d task%c (%d tasks/node required)\n",
      TC,
      (TC == 1) ? ' ' : 's',
      MAX(1,RQ->TasksPerNode));

    /* check classes */

    if ((N != MSched.GN) &&
        (J->Credential.C != NULL) && !bmisset(&N->Classes,J->Credential.C->Index))
      {
      MDB(5,fSCHED) MLog("INFO:     node is missing class %s\n",
        J->Credential.C->Name);

      if (RIndex != NULL)
        *RIndex = marClass;
 
      if (BMsg != NULL)
        {
        sprintf(BMsg,"missing %s",J->Credential.C->Name );
        }

      return(FAILURE);
      }

    if ((N->RM != NULL) && (N->RM->Type == mrmtLL))
      {
      /* handle LL geometry */

      MinTPN = (RQ->TasksPerNode > 0) ? RQ->TasksPerNode : 1;

      if (RQ->NodeCount > 0)
        {
        MinTPN = RQ->TaskCount / RQ->NodeCount;
        }

      /* handle pre-LL22 geometries */

      if ((RQ->BlockingFactor != 1) &&
         ((TC < (MinTPN - 1)) ||
         ((TC < MinTPN) && (RQ->TaskCount % RQ->NodeCount == 0))))
        {
        MDB(5,fSCHED) MLog("INFO:     node supports %d task%c (%d tasks/node required:LL)\n",
          TC,
          (TC == 1) ? ' ' : 's',
          MinTPN);

        if (RIndex != NULL)
          *RIndex = marCPU;

        return(FAILURE);
        }
      }    /* END if (RM[N->RMIndex].Type == mrmtLL) */

    /* check local disk space */

    if (N->CRes.Disk > 0)
      {
      /* NOTE:  only check disk space if disk space is reported */
#ifdef MNOT
      if (N->CRes.Disk <= (2 * J->ExecSize))
        {
        MDB(5,fSCHED) MLog("INFO:     inadequate free space in local disk for executable on node '%s' (%d MB available)\n",
          N->Name,
          N->CRes.Disk);

        if (RIndex != NULL)
          *RIndex = marDisk;

        return(FAILURE);
        }
#endif /* MNOT */
      }    /* END if (N->CRes.Disk > 0) */

    if (!MSNLIsEmpty(&RQ->DRes.GenericRes))
      {
      if (MSNLGetCount(
            J->PStartPriority[N->PtIndex],
            &RQ->DRes.GenericRes,
            &NA->GenericRes,
            NULL) == FAILURE)
        {
        MDB(3,fSCHED) MLog("INFO:     generic resources not found (%d needed)\n",
          MSNLGetIndexCount(&RQ->DRes.GenericRes,0));

        if (RIndex != NULL)
          *RIndex = marFeatures;

        return(FAILURE);
        }
      }
    }   /* END if (StartTime == MSched.Time) */

  if (StartTime < MMAX_TIME)
    {
    /* check dynamic node attributes */

    /* check swap space */

    if ((MPar[0].NAvailPolicy[mrSwap] != mrapDedicated) && 
        (N->CRes.Swap > 0) && 
        (N->ARes.Swap < MDEF_MINSWAP))
      {
      MDB(5,fSCHED) MLog("INFO:     inadequate swap on node '%s' (%d MB available)\n",
        N->Name,
        N->ARes.Swap);

      if (RIndex != NULL)
        *RIndex = marSwap;

      return(FAILURE);
      }

    /* check local disk */

    if ((MPar[0].NAvailPolicy[mrDisk] != mrapDedicated) && 
        (N->CRes.Disk > 0) &&
        !MUCompare(N->CRes.Disk,RQ->ReqNRC[mrDisk],RQ->ReqNR[mrDisk]))
      {
      MDB(5,fSCHED) MLog("INFO:     inadequate disk (%s %d requested  %d found)\n",
        MComp[(int)RQ->ReqNRC[mrDisk]],
        RQ->ReqNR[mrDisk],
        N->CRes.Disk);

      if (RIndex != NULL)
        *RIndex = marDisk;

      return(FAILURE);
      }

    /* FIXME: add check for GRes to filter out nodes that cannot start the job now */

    if (MJobCheckNStartTime(
          J,
          RQ,
          N,
          StartTime,
          TCAvail,   /* O */
          MinSpeed,
          RIndex,    /* O */
          Affinity,
          AvailTime, /* O */
          BMsg) == FAILURE)
      {
      MDB(4,fSCHED) MLog("INFO:     adequate time not available on node %s to run job %s\n",
        N->Name,
        J->Name);

      if (BMsg != NULL)
        {
        if ((RIndex != NULL) && (*RIndex == marTime))
          {
          if ((BMsg[0] == '\0') && (J->ReqRID != NULL))
            snprintf(BMsg,MMAX_NAME,"rsv %s required",
              J->ReqRID);
          }
        }    /* END if (BMsg != NULL) */

      return(FAILURE);
      }      /* END if (MJobCheckNStartTime() == FAILURE) */
    }        /* END if (StartTime < MMAX_TIME) */
  else
    {
    /* NOTE:  Node is being considered for the future - DRes should be NULL, right? */

    if (TCAvail != NULL)
      {
      *TCAvail = MNodeGetTC(N,&N->CRes,&N->CRes,&N->DRes,&RQ->DRes,MMAX_TIME,1,RIndex);

      if (RQ->TasksPerNode > 0)
        {
        if (bmisset(&J->IFlags,mjifExactTPN) || bmisset(&J->IFlags,mjifMaxTPN))
          *TCAvail = MIN(RQ->TasksPerNode,*TCAvail);

        if (*TCAvail < RQ->TasksPerNode)
          *TCAvail = 0;
        }
      }
    }

  if ((TCAvail != NULL) && (*TCAvail == 0))
    {
    return(FAILURE);
    }

  if ((AttrAffinity != mnmNONE) && (Affinity != NULL))
    {
    /* only override rsv affinity in certain cases */

    switch (*Affinity)
      {
      default:
      case mnmNONE:

        *Affinity = AttrAffinity;

        break;

      case mnmPositiveAffinity:
      case mnmNegativeAffinity:
      case mnmRequired:

        /* NO-OP */

        break;
      }
    }
 
  if (MLocalJobCheckNRes(J,N,StartTime) == FAILURE)
    {
    MDB(5,fSCHED) MLog("INFO:     failed local check\n");

    if (RIndex != NULL)
      *RIndex = marPolicy;

    if (BMsg != NULL)
      strcpy(BMsg,"local");

    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MReqCheckNRes() */




/**
 * Convert resource-manager specific resource specification string to internal 
 * req structure.
 *
 * @see MJobProcessExtensionString() - parent
 *
 * @param J (I)
 * @param RQ (I) [modified]
 * @param ResString (I)
 * @param RMType (I)
 * @param IsPref (I)
 */

int MReqRResFromString(

  mjob_t *J,         /* I */
  mreq_t *RQ,        /* I (modified) */
  char   *ResString, /* I */
  int     RMType,    /* I */
  int     IsPref)    /* I */

  {
  unsigned int   index;
  int   sindex;

  enum MCompEnum   CIndex;

  int   RMIndex;

  char *ptr;
  char *TokPtr;

  char  AName[MMAX_LINE];
  char  ValLine[MMAX_LINE];

  enum MReqRsrcEnum {
    mrrNONE = 0,
    mrrArch,
    mrrADisk,
    mrrAMem,
    mrrAProcs,
    mrrASwap,
    mrrCDisk,
    mrrCMem,
    mrrCProcs,
    mrrCSwap,
    mrrFeature,
    mrrHost,
    mrrOpSys,
    mrrVariable,
    mrrLAST };

  int RMList[] = { mrmtLL, mrmtWiki, -1 };

  #define MMAX_RMRRESTYPE  (sizeof(RMList) / sizeof(int))

  typedef struct {
    int   RRIndex;
    const char *RName[MMAX_RMRRESTYPE];
    } mrrlist_t;

  /*                  LL              Wiki        Other1 */

  const mrrlist_t  MRMRRes[] = {
    { mrrNONE,      { NULL,      NULL,       NULL }},
    { mrrArch,      { "Arch",    NULL,       NULL }},
    { mrrADisk,     { NULL,      NULL,       NULL }},
    { mrrAMem,      { NULL,      NULL,       NULL }},
    { mrrAProcs,    { NULL,      NULL,       NULL }},
    { mrrASwap,     { "Swap",    NULL,       NULL }},
    { mrrASwap,     { NULL,      NULL,       NULL }},
    { mrrCDisk,     { "Disk",    "DISK",     NULL }},
    { mrrCMem,      { "Memory",  "MEM",      NULL }},
    { mrrCProcs,    { NULL,      "PROC",     NULL }},
    { mrrCSwap,     { NULL,      "SWAP",     NULL }},
    { mrrFeature,   { "Feature", "FEATURE",  NULL }},
    { mrrHost,      { "Machine", NULL,       NULL }},
    { mrrOpSys,     { "OpSys",   NULL,       NULL }},
    { mrrVariable,  { NULL,      "VARIABLE", NULL }},
    { 0,            { NULL,      NULL,       NULL }},
    { 0,            { NULL,      NULL,       NULL }},
    { 0,            { NULL,      NULL,       NULL }},
    { 0,            { NULL,      NULL,       NULL }},
    { 0,            { NULL,      NULL,       NULL }},
    { 0,            { NULL,      NULL,       NULL }},
    { 0,            { NULL,      NULL,       NULL }},
    { 0,            { NULL,      NULL,       NULL }},
    { 0,            { NULL,      NULL,       NULL }},
    { 0,            { NULL,      NULL,       NULL }},
    { 0,            { NULL,      NULL,       NULL }},
    { 0,            { NULL,      NULL,       NULL }},
    { 0,            { NULL,      NULL,       NULL }},
    { 0,            { NULL,      NULL,       NULL }},
    { -1,           { NULL,      NULL,       NULL }}};

  const char *FName = "MReqRResFromString";

  MDB(5,fSCHED) MLog("%s(%s,RQ,%s,%s,%s)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    MRMType[RMType],
    (ResString != NULL) ? ResString : "NULL",
    MBool[IsPref]);

  if ((J == NULL) || (RQ == NULL) || (ResString == NULL))
    {
    return(FAILURE);
    }

  /* locate RM type */

  RMIndex = -1;

  for (index = 0;index < MMAX_RMRRESTYPE;index++)
    {
    if (RMList[index] == RMType)
      {
      RMIndex = index;

      break;
      }
    }  /* END for (index) */

  if (RMIndex == -1)
    {
    /* cannot locate requested RM type */

    RMIndex = mrmtWiki;
    }

  if (IsPref == FALSE)
    {
    /* set defaults */

    bmclear(&RQ->ReqFBM);

    memset(RQ->ReqNR,0,sizeof(RQ->ReqNR));

    memset(RQ->ReqNRC,MDEF_RESCMP,sizeof(RQ->ReqNRC));

    RQ->Opsys          = 0;
    RQ->Arch           = 0;
    }  /* END is (IsPref == FALSE) */

  /* parse string by boolean operators */

  /* FORMAT: <REQ>[{ && | || } <REQ> ] ... */

  /* FORMAT: REQ -> [(]<ATTR>[<WS>]<CMP>[<WS>]["]<VAL>["]<WS>[)] */
  /* FORMAT: REQ -> <FEATURE> */

  /* NOTE:  currently do not support 'or' booleans.  all req's 'and'd */

  ptr = MUStrTok(ResString,"&|",&TokPtr);

  while (ptr != NULL)
    {
    if (MUParseComp(ptr,AName,&CIndex,ValLine,sizeof(ValLine),TRUE) == FAILURE)
      {
      MDB(0,fSCHED) MLog("ALERT:    cannot parse req line for job '%s'\n",
        J->Name);

      ptr = MUStrTok(NULL,"&|",&TokPtr);

      continue;
      }

    /* determine attr type */

    for (index = 0;MRMRRes[index].RRIndex != -1;index++)
      {
      if ((MRMRRes[index].RName[RMIndex] != NULL) &&
          !strcasecmp(MRMRRes[index].RName[RMIndex],AName))
        {
        break;
        }
      }  /* END for (index) */

    if (MRMRRes[index].RRIndex == 0)
      {
      /* ignore resource request */
 
      continue;
      }

    if (MRMRRes[index].RRIndex == -1)
      {
      if (MUGetNodeFeature(AName,mVerify,&RQ->ReqFBM) > 0)
        {
        MDB(0,fSCHED) MLog("ALERT:    cannot identify resource type '%s' for job '%s'\n",
          AName,
          J->Name);
        }
      else
        {
        MDB(3,fSCHED) MLog("INFO:     feature request %s added to job %s resource requirements\n",
          AName,
          J->Name);
        }

      ptr = MUStrTok(NULL,"&|",&TokPtr);

      continue;
      }  /* END if (MRMRRes[index].RRIndex == -1) */

    if (IsPref == TRUE)
      {
      MDB(6,fSCHED) MLog("INFO:     job %s requests resource preference %s %s %s\n",
        J->Name,
        AName,
        MComp[CIndex],
        ValLine);

      switch (MRMRRes[index].RRIndex)
        {
        case mrrFeature:

          if (CIndex == mcmpEQ)
            {
            if (RQ->Pref == NULL)
              {
              if ((RQ->Pref = (mpref_t *)MUCalloc(1,sizeof(mpref_t))) == NULL)
                {
                return(FAILURE);
                }
              }

            MUGetNodeFeature(ValLine,mAdd,&RQ->Pref->FeatureBM);
            }

          break;

        case mrrVariable:
 
          if (CIndex == mcmpEQ)
            {
            if (RQ->Pref == NULL)
              {
              if ((RQ->Pref = (mpref_t *)MUCalloc(1,sizeof(mpref_t))) == NULL)
                {
                return(FAILURE);
                }
              }

            if (MULLAdd(
                &RQ->Pref->Variables,
                ValLine,
                NULL,
                NULL,
                MUFREE) == FAILURE)
              {
              MDB(1,fSCHED) MLog("ERROR:    cannot set pref variable for job %s\n",
                J->Name);

              return(FAILURE);
              }
            }

          break;

        default:

          /* NO-OP */

          break;
        }  /* END switch (MRMRRes[index].RRIndex) */

      ptr = MUStrTok(NULL,"&|",&TokPtr);

      continue;
      }  /* END if (IsPref == TRUE) */

    MDB(6,fSCHED) MLog("INFO:     job %s requests resource constraints %s %s %s\n",
      J->Name,
      AName,
      MComp[CIndex],
      ValLine);

    switch (MRMRRes[index].RRIndex)
      {
      case mrrArch:

        if ((ValLine[0] != '\0') && strcmp(ValLine,"any") && (CIndex == mcmpEQ))
          {
          if ((RQ->Arch = MUMAGetIndex(meArch,ValLine,mAdd)) == FAILURE)
            {
            MDB(1,fSCHED) MLog("WARNING:  job '%s' does not have valid Arch value '%s'\n",
              J->Name,
              ValLine);
            }
          }

        break;

      case mrrADisk:

        RQ->DRes.Disk = (int)strtol(ValLine,NULL,10);

        break;

      case mrrAMem:

        RQ->DRes.Mem = (int)strtol(ValLine,NULL,10);

        break;

      case mrrAProcs:

        RQ->DRes.Procs = (int)strtol(ValLine,NULL,10);

        break;

      case mrrASwap:

        RQ->DRes.Swap = (int)strtol(ValLine,NULL,10);

        break;

      case mrrCDisk:

        /* NOTE: rm specific unit defaults not handled */

        RQ->ReqNRC[mrMem] = CIndex;
        RQ->ReqNR[mrMem]  = (int)strtol(ValLine,NULL,10) / 1024;  

        break;

      case mrrCMem:

        RQ->ReqNRC[mrMem] = CIndex;
        RQ->ReqNR[mrMem]  = (int)strtol(ValLine,NULL,10);

        break;

      case mrrCProcs:

        RQ->ReqNRC[mrProc] = CIndex;
        RQ->ReqNR[mrProc]  = (int)strtol(ValLine,NULL,10);

        break;

      case mrrCSwap:

        RQ->ReqNRC[mrSwap] = CIndex;
        RQ->ReqNR[mrSwap]  = (int)strtol(ValLine,NULL,10);

        break;

      case mrrFeature:

        if (CIndex == mcmpEQ)
          {
          MUGetNodeFeature(ValLine,mAdd,&RQ->ReqFBM);
          }

        break;

      case mrrHost:

        if (CIndex == mcmpEQ)
          {
          for (sindex = 0;sindex < (int)strlen(ValLine);sindex++)
            {
            if (ValLine[sindex] == '\"')
              ValLine[sindex] = ' ';
            }

          if (MJobSetAttr(J,mjaReqHostList,(void **)ValLine,mdfString,mAdd) == FAILURE)
            {
            char tmpLine[MMAX_LINE];

            /* invalid hostlist required */

            sprintf(tmpLine,"invalid hostlist requested '%.64s'",
              ValLine);

            /* set job hold */

            MJobSetHold(J,mhDefer,MSched.DeferTime,mhrNoResources,tmpLine);
            }

          J->ReqHLMode = mhlmSubset;
          }
        else if (CIndex == mcmpNE)
          {
          MJobSetAttr(J,mjaExcHList,(void **)ValLine,mdfString,mAdd);
          }
        else
          {
          /* invalid hostlist requirement */
 
          MDB(1,fSCHED) MLog("WARNING:  job '%s' detected with invalid host requirment '%s'\n",
            J->Name,
            ValLine);
          }

        break;

      case mrrOpSys:

        if ((ValLine[0] != '\0') && strcmp(ValLine,"any") && (CIndex == mcmpEQ))
          {
          if ((RQ->Opsys = MUMAGetIndex(meOpsys,ValLine,mAdd)) == FAILURE)
            {
            MDB(1,fSCHED) MLog("WARNING:  job '%s' does not have valid Opsys value '%s'\n",
              J->Name,
              ValLine);
            }
          }

        break;

      default:

        /* NO-OP */

        MDB(3,fSCHED) MLog("WARNING:  resource req type %s not supported in %s()\n",
          AName,
          FName);

        break;
      }  /* END switch (MRMRRes[index].RRIndex) */

    ptr = MUStrTok(NULL,"&|",&TokPtr);
    }  /* END while (ptr != NULL) */

  return(SUCCESS);
  }  /* END MReqRResFromString() */
 


/**
 * Handle allocation for global gres, licenses, networks, storage, etc.
 *
 * @param J (I)
 * @param RQ (I)
 */

int MReqAllocateLocalRes(

  mjob_t *J,  /* I */
  mreq_t *RQ) /* I */

  {
  mnl_t     tmpNL;

  char       tmpLine[MMAX_LINE];

  const char *FName = "MReqAllocateLocalRes";

  MDB(7,fSTRUCT) MLog("%s(%s,%d)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (RQ != NULL) ? RQ->Index : -1);

  if ((J == NULL) || (RQ == NULL))
    {
    return(FAILURE);
    }

  if (RQ->DRes.Procs != 0)
    { 
    /* no local resources to allocated */

    return(SUCCESS);
    }

  if (!MNLIsEmpty(&RQ->NodeList))
    {
    /* local resources previously allocated */

    return(SUCCESS);
    }

  /* must allocate local resources */

  MNLInit(&tmpNL);

  /* NOTE:  currently allocate first feasible pseudo resource */

  if (MReqGetFNL(
       J,
       RQ,
       &MPar[0],
       NULL, /* I */
       &tmpNL,
       NULL,
       NULL,
       MMAX_TIME,
       0,
       NULL) == FAILURE)
    {
    MNLFree(&tmpNL);

    MDB(2,fRM) MLog("ALERT:    cannot locate feasible virtual resources for %s:%d  (gres: %d)\n",
      J->Name,
      RQ->Index,
      MSNLGetIndexCount(&RQ->DRes.GenericRes,0));

    snprintf(tmpLine,sizeof(tmpLine),"cannot locate virtual resources for req %d",
      RQ->Index);

    MMBAdd(&J->MessageBuffer,tmpLine,NULL,mmbtNONE,0,0,NULL);

    return(FAILURE);
    }  /* END if (MReqGetFNL() == FAILURE) */

  /* NOTE:  only allocate single node per req */

  MNLSetTCAtIndex(&tmpNL,0,MIN(MNLGetTCAtIndex(&tmpNL,0),RQ->TaskCount));
  MNLTerminateAtIndex(&tmpNL,1);

  MReqSetAttr(
    J,
    RQ,
    mrqaAllocNodeList,
    (void **)&tmpNL,
    mdfOther,
    mSet);

  J->Request.NC += 1;
  J->Request.TC += MNLGetTCAtIndex(&tmpNL,0);

  MDB(2,fRM) MLog("INFO:     %d of %d tasks allocated for %s:%d\n",
    MNLGetTCAtIndex(&RQ->NodeList,0),
    RQ->TaskCount,
    J->Name,
    RQ->Index);

  MNLFree(&tmpNL);

  return(SUCCESS);
  }  /* END MReqAllocateLocalRes() */


/**
 * Removes the req at rqindex from the job.
 *
 * @param J
 * @param rqindex
 */

int MReqRemoveFromJob(

  mjob_t *J,
  int     rqindex)

  {
  if ((J == NULL) || (rqindex >= MMAX_REQ_PER_JOB) || (J->Req[rqindex] == NULL))
    {
    return(FAILURE);
    }

  MReqDestroy(&J->Req[rqindex]);

  for (;rqindex < MMAX_REQ_PER_JOB;rqindex++)
    {
    J->Req[rqindex] = J->Req[rqindex + 1];
    J->Req[rqindex + 1] = NULL;
    }

  for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
    {
    if (J->Req[rqindex] == NULL)
      break;

    J->Req[rqindex]->Index = rqindex;
    }

  return(SUCCESS);
  }  /* END MReqRemoveFromJob() */


/**
 * Ensures that the given node satisfies a job's required node features.
 *
 * @param AvlMap (I)
 * @param RQ (I)
 */

int MReqCheckFeatures(

  const mbitmap_t  *AvlMap,
  const mreq_t     *RQ)

  {
  const mbitmap_t *ReqMap;

  int rc;

  mbool_t DoNone = FALSE;

  if ((RQ == NULL) ||
      (AvlMap == NULL))
    {
    return(FAILURE);
    }

  ReqMap = &RQ->ReqFBM;

#ifdef MYAHOO
  {
  int None = 0;
  int index;

  /* check for special 'NONE' feature! */

  None = MUGetNodeFeature("NONE",mVerify,NULL);

  for (index = 0;index < MMAX_ATTR;index++)
    {
    if (MAList[meNFeature][index][0] == '\0')
      break;

    if (bmisset(&ReqMap,None))
      {
      DoNone = TRUE;

      break;
      }
    }  /* END for (index) */

  if (DoNone == TRUE)
    {
    for (index = 0;index < MMAX_ATTR;index++)
      {
      if (MAList[meNFeature][index][0] == '\0')
        break;

      if (bmisset(&AvlMap,index))
        {
        return(FAILURE);
        }
      }  /* END for (index) */
    }
  }  /* END BLOCK */
#endif /* MYAHOO */

  if (DoNone == TRUE)
    {
    /* if we got this far, then the node has no features on it
     * and we don't need to call MAttrSubset() to see if our
     * requested features match those on the node (because there
     * are none) */

    return(SUCCESS);
    }

  /* now check to see if features are actually satisfied */

  rc = MAttrSubset(AvlMap,ReqMap,RQ->ReqFMode);

  return(rc);
  }  /* END MReqCheckFeatures() */




/**
 * Duplicates the mreq_t structure. 
 *
 * @see MJobDuplicate() - parent
 *
 * NOTE: will alloc new memory on Dst structure.
 * NOTE: this routine only works for jobs that have just been loaded (ie. not 
 *       running, no stats, no reservations) 
 *
 * @param Dst (O)
 * @param Src (I)
 */

int MReqDuplicate(

  mreq_t       *Dst,
  const mreq_t *Src)

  {
  MUStrDup(&Dst->ReqJID,Src->ReqJID);             /**< if set, indicates req is associated with co-alloc job (alloc) */
  memcpy(&Dst->ReqNR,&Src->ReqNR,sizeof(Src->ReqNR)); /**< required resource attributes (ie, cfg mem >= X) */
  MUStrCpy(Dst->ReqNRC,Src->ReqNRC,sizeof(Src->ReqNRC));     /**< required resource attribute comparison */

  bmcopy(&Dst->ReqFBM,&Src->ReqFBM);    /**< required feature (BM)                */
  bmcopy(&Dst->SpecFBM,&Src->SpecFBM); /**< specified feature (BM)               */

  if (Src->Pref != NULL)
    {
    if (Dst->Pref == NULL)
      {
      if ((Dst->Pref = (mpref_t *)MUCalloc(1,sizeof(mpref_t))) == NULL)
        {
        return(FAILURE);
        }
      }

    /**< preferred feature (BM) */
                                                                   
    bmcopy(&Dst->Pref->FeatureBM,&Src->Pref->FeatureBM); 

    /**< preferred variables */

    if (Src->Pref->Variables != NULL)
      MULLDup(&Dst->Pref->Variables,Src->Pref->Variables);
    }  /* END if (Src->Pref != NULL) */

  Dst->ReqFMode = Src->ReqFMode; /**< required feature logic (and or or)          */

  MNSCopy(Dst,Src);

  if (Src->NAllocPolicy != mnalNONE)
    {
    Dst->NAllocPolicy= Src->NAllocPolicy;
    }

  /* NOTE:  locations still reference Network as an index, not a BM - FIXME */

  Dst->Opsys = Src->Opsys;              /**< required OS (BM of MAList[meOpsys])         */
  Dst->Arch = Src->Arch;               /**< HW arch (BM of MAList[meArch])              */
  
/*  moptreq_t *OptReq;         < optional extension requirements (alloc) */

  Dst->PtIndex = Src->PtIndex;            /**< assigned partition index                    */

  MSNLFree(&Dst->CGRes);
  MSNLFree(&Dst->AGRes);
  MSNLFree(&Dst->DRes.GenericRes);

  MSNLInit(&Dst->DRes.GenericRes);

  MCResCopy(&Dst->DRes,&Src->DRes);
/*  mcres_t *SDRes;            < dedicated resources per task (suspended)    */
  if ((Src->MURes != NULL) && (Dst->MURes != NULL))
    MCResCopy(Dst->MURes,Src->MURes);

  /* initialize CGRes and AGres */
  MSNLInit(&Dst->CGRes);
  MSNLInit(&Dst->AGRes);

/*  mxload_t *XURes;           < utilized extension resources (alloc)        */
/*  mxload_t *XRRes;           < required extension resources (alloc)        */

  Dst->RMWTime = Src->RMWTime;            /**< req walltime as reported by RM              */

  /* index '0' is active, indices 1 through MMAX_SHAPE contain requests */

  memcpy(&Dst->TaskRequestList,&Src->TaskRequestList,sizeof(Src->TaskRequestList));

  Dst->NodeCount = Src->NodeCount;          /**< nodes allocated */
  Dst->TaskCount = Src->TaskCount;          /**< tasks allocated */

/*  int   *SGCounter[MMAX_GCOUNTER];  < generic counter start value (alloc) */
/*  int   *EGCounter[MMAX_GCOUNTER];  < generic counter end value (alloc) */

  Dst->TasksPerNode = Src->TasksPerNode;       /**< (0 indicates TPN not specified) */
  Dst->BlockingFactor = Src->BlockingFactor;

  Dst->MinProcSpeed = Src->MinProcSpeed;       /**< minimum proc speed for this req */

  Dst->NAccessPolicy = Src->NAccessPolicy; /**< node access policy */

/*mnallocp_t *NAllocPolicy;   ??? */

  Dst->RMIndex = Src->RMIndex;       /**< index of DRM if set, otherwise index of SRM (-1 if not set) */

  /* used with co-alloc jobs */

  Dst->State = Src->State;   /**< state of job-part associated with this req */
  Dst->EState = Src->EState;  /**< expected state of job-part associated with this req */
  
/*  mrsv_t    *R;         < req specific reservation (only used w/multi-rm jobs) */
/*  mrrsv_t   *RemoteR;   < req specific remote reservation (only used w/multi-rm jobs) */

/*  mnalloc_t *NodeList;  < (alloc,minsize=MSched.JobMaxNodeCount + 1,terminated w/N==NULL) */

  Dst->SetSelection = Src->SetSelection;
  Dst->SetType = Src->SetType;

  if ((Src->SetList != NULL) && (Src->SetList[0] != NULL))
    {
    int index;

    if (Dst->SetList == NULL)
      {
      Dst->SetList = (char **)MUCalloc(1,sizeof(char *) * MMAX_ATTR);
      }

    for (index = 0;Src->SetList[index] != NULL;index++)
      {
      MUStrDup(&Dst->SetList[index],Src->SetList[index]);
      }
    }

  Dst->Index = Src->Index;

  return(SUCCESS);
  } /* END int MReqDuplicate() */



/**
 * Gets the number of nodeboards a req would need to run on.
 *
 * Determine number of nodeboards by number or requested procs and memory.
 *
 * Request: 2proc,8mem NodeA=2proc,4mem = needs 2 of NodeA to fullfill
 * Request: 4proc,4mem NodeA=2proc,4mem = needs 2 of NodeA to fullfill
 *
 * @param J (I)
 * @param RQ (I)
 * @param N (I)
 * @return Returns the number of nodeboards needed.
 */

int MReqGetNBCount(
   
  const mjob_t  *J,
  const mreq_t  *RQ,
  const mnode_t *N)

  {
  int         ReqProcs = 0;
  int         ReqMem = 0;
  int         Result;
  double      NumNodes;  /* number of nodes required to satisfy job */

  const char *FName = "MReqGetNBCount";

  MDB(5,fSTRUCT) MLog("%s(%s,%d,%s)\n",
    FName,
    (J != NULL)  ? J->Name : "NULL",
    (RQ != NULL) ? RQ->Index : 0,
    (N != NULL)  ? N->Name : "NULL");

  if ((J == NULL) || (RQ == NULL) || (N == NULL))
    { 
    MDB(1,fSCHED) MLog("ERROR:    invalid parameters passed to MReqGetNBCount\n");

    return(FAILURE);
    }

  ReqProcs += RQ->DRes.Procs * RQ->TaskCount;
  ReqMem += RQ->ReqMem;

  /* calculate number of nodes required to satisfy job */
  NumNodes = MAX(
    (double)ReqMem / MAX(1,N->CRes.Mem),
    (double)ReqProcs / MAX(1,N->CRes.Procs));

  /* total lost resources is effectively the number of nodes consumed */

  if ((NumNodes - (int)NumNodes) > 0.0) /* round up */
    Result = (int)(NumNodes + 1);
  else
    Result = (int)NumNodes;

  MDB(7,fSTRUCT) MLog("INFO:     %d nodeboards needed for job %s:%d (cpus=%d mem=%d) using node %s (cprocs=%d cmem=%d)\n",
    Result,
    J->Name,
    RQ->Index,
    ReqProcs,
    ReqMem,
    N->Name,
    N->CRes.Procs,
    N->CRes.Mem);

  return(Result);
  } /* END int MReqGetNBCount() */



/*  determines the required operating system for a 
 *  given job, which can be on the job's own req, or
 *  in some other structure with which the job is 
 *  associated.
 * 
 *  @param J      (I)
 *  @param reqos  (O, optional)
 */

#define MRGRO_ACTUAL_OS(X) ((X > 0) && (strcmp(MAList[meOpsys][X], ANY) != 0))

int MReqGetReqOpsys(

  const mjob_t  *J,
  int           *ReqOS)

  {
  if ((J == NULL) || (ReqOS == NULL))
    {
    return (FAILURE);
    }

  *ReqOS = 0;

  /* job req */
  if ((J->Req[0] != NULL) && 
      MRGRO_ACTUAL_OS(J->Req[0]->Opsys))
    {
    *ReqOS = J->Req[0]->Opsys;
  
    return (SUCCESS);
    }

  /* job class */
  if ((J->Credential.C != NULL) && 
      (J->Credential.C->L.JobDefaults != NULL))
    {
    J = J->Credential.C->L.JobDefaults;

    if ((J->Req[0] != NULL) && 
        (MRGRO_ACTUAL_OS(J->Req[0]->Opsys)))
      {
      *ReqOS = J->Req[0]->Opsys;
      
      return (SUCCESS);
      }
    }

  /* default class */
  if ((MSched.DefaultC != NULL) && 
      (MSched.DefaultC->L.JobDefaults != NULL))
    {
    J = MSched.DefaultC->L.JobDefaults;

    if ((J->Req[0] != NULL) && 
        (MRGRO_ACTUAL_OS(J->Req[0]->Opsys)))
      {
      *ReqOS = J->Req[0]->Opsys;
      
      return (SUCCESS);
      }
    }

  /* default job template */
  if ((MSched.DefaultJ != NULL) && 
      (MSched.DefaultJ->Req[0] != NULL))
    {
    J = MSched.DefaultJ;
  
    if ((J->Req[0] != NULL) && 
        (MRGRO_ACTUAL_OS(J->Req[0]->Opsys)))
      {
      *ReqOS = J->Req[0]->Opsys;
      
      return (SUCCESS);
      }
    }

  return (FAILURE);
  } /* END MReqGetReqOpsys() */



int MReqSetImage(

  mjob_t *J,
  mreq_t *RQ,
  int     OSIndex)

  {
  if ((J == NULL) || (RQ == NULL))
    { 
    return(FAILURE);
    }

  /* NOTE:  must know if image access is dedicated */

  /* NOTE:  all createvm images will be dedicated */

  /* NOTE:  all requirevm images will be dedicated */

  /* NOTE:  requirepm images may/may not be dedicatd */

  /* should apply reqmem to job */

  if (J->VMUsagePolicy == mvmupVMCreate)
    {
    if ((RQ->Opsys != 0) && (MSched.VMOSSize[RQ->Opsys] > 0))
      RQ->DRes.Mem = MAX(RQ->DRes.Mem,MSched.VMOSSize[RQ->Opsys]);
    else
      RQ->DRes.Mem = MAX(RQ->DRes.Mem,MSched.VMOSSize[0]);
    }

  return(SUCCESS);
  }  /* END MReqSetImage() */



/**
 * Set the resource requirements on a req from a string
 * of the form '[cmp]value', e.g. '>=100', '==10'
 * 
 * @see MReqSetAttr() - parent
 *
 * @param RQ (I) [modified]
 * @param Resource (I)
 * @param str (I)
 */

void MReqResourceFromString(

  mreq_t                 *RQ,        /* (I) modified */
  enum MResourceTypeEnum  Resource,  /* (I) */
  char                   *str)       /* (I) */

  {
  if ((RQ != NULL) && (str != NULL) && (Resource > mrNONE) && (Resource < mrLAST))  
    {
    int ComparatorLen;
    int Comparator;

    Comparator = ComparatorLen = 0;

    if ((Comparator = MUCmpFromString(str, &ComparatorLen)) > 0)
      {
      RQ->ReqNRC[Resource] = Comparator;
      }

    RQ->ReqNR[Resource] = (int)strtol((char *)(str + ComparatorLen), NULL, 10);
    }

  return;
  }



/**
 * Determines if the req is a size zero job req.
 *
 * Size zero jobs are used on Crays and run only on the login/pbs_mom nodes and
 * don't run on compute nodes with ALPs reservations
 *
 * @param J (I) Used to call MJobHasGRes
 * @param Req (I) Req to check.
 *
 * @pre Req should be a member of J->Req.
 *
 * @return True if the req is size zero, false otherwise.
 */

mbool_t MReqIsSizeZero(

  mjob_t *J,
  mreq_t *Req)

  {
  int    rc    = FAILURE;
  mrm_t *ReqRM = NULL;

  MASSERT(J != NULL,"null job pointer when check size zero req.");
  MASSERT(Req != NULL,"null req pointer when check size zero req.");

  ReqRM = &MRM[J->Req[Req->Index]->RMIndex];

  MASSERT(ReqRM != NULL,"req has a null rm pointer.");

  if (((ReqRM->SubType == mrmstXT3) || (ReqRM->SubType == mrmstXT4)) &&
      (Req->DRes.Procs == 0) &&
      (MJobHasGRes(J,"master",Req->Index) == TRUE))
    {
    rc = SUCCESS;
    }

  return(rc);
  } /* END MReqIsSizeZero() */


