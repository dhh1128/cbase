/* HEADER */

/**
 * @file MVMVlan.c
 * 
 * Contains various functions VLAN with VM
 *
 */


#include "moab.h"
#include "moab-proto.h"
#include "moab-const.h"
#include "moab-global.h"



/*
 * If MSched.UseVLANFeatures is TRUE, will take the given job and
 *  do an mshow -a to find the first node that supports the proc count.
 *  The job (which has the VM) will then request the feature of that node.
 *  If the job already requested a VLAN feature, than that feature will be
 *  used.
 *
 * @param VMJob (I) [modified] - The VM job to assign to a VLAN 
 */

int MVMChooseVLANFeature(

  mjob_t *VMJob)

  {
  char  tmpXML[MMAX_LINE];
  char *BPtr;
  int   BSpace;
  int   TmpRQD;

  mrange_t RRange[MMAX_REQ_PER_JOB];
  mbitmap_t   QueryFlags;

  mxml_t  *AE = NULL;
  mxml_t  *RE = NULL;
  mxml_t  *PE = NULL;
  mxml_t  *CE = NULL;

  int      CTok;
  mrsv_t  *TmpRsv = NULL;
  mrsv_t  *R = NULL;

  char  AdminName[MMAX_NAME];

  int      RQIndex;
  int      FIndex;
  int      NSIndex; /* NodeSet index */

  char     Feature[MMAX_NAME] = {0};
  int      FeatureIndex;

  char     TmpEMsg[MMAX_LINE];

  if (VMJob == NULL)
    {
    return(FAILURE);
    }

  if (VMJob->VMUsagePolicy != mvmupVMCreate)
    {
    /* This job does not create VMs */

    return(SUCCESS);
    }

  if (MSched.AssignVLANFeatures != TRUE)
    {
    return(SUCCESS);
    }

  if ((VMJob->VMCreateRequest != NULL) &&
      (VMJob->VMCreateRequest->N != NULL))
    {
    /* Job already requested a specific hypervisor, we'll use that VLAN */

    MNodeGetVLAN(VMJob->VMCreateRequest->N,Feature);
    FeatureIndex =  MUMAGetIndex(meNFeature,Feature,mVerify);

    if (FeatureIndex == 0)
      {
      MDB(7,fSTRUCT) MLog("ERROR:    could not find VLAN feature '%s'\n",
        Feature);

      return(FAILURE);
      }

    /* Need to put the feature on all reqs */

    for (RQIndex = 0;VMJob->Req[RQIndex] != NULL;RQIndex++)
      {
      bmset(&VMJob->Req[RQIndex]->ReqFBM,FeatureIndex);
      }

    return(SUCCESS);
    }  /* END if ((VMJob->VMCR != NULL) && (VMJob->VMCR->N != NULL)) */

  /* Check to see if VMJob already has a VLAN feature */

  for (RQIndex = 0;VMJob->Req[RQIndex] != NULL;RQIndex++)
    {
    for (FIndex = 0;FIndex < MDEF_ATTRBMSIZE;FIndex++)
      {
      /* Check Feature Bit Map for feature bit */

      if (!bmisset(&VMJob->Req[RQIndex]->ReqFBM,FIndex))
        continue;

      /* Search the nodeset list that contains all vlan features against the feature bit map. */

      for (NSIndex = 0;NSIndex < MMAX_ATTR;NSIndex++)
        {
        if (MPar[0].NodeSetList[NSIndex] == NULL)
          break;

        if (!strcmp(MPar[0].NodeSetList[NSIndex],MAList[meNFeature][FIndex]))
          {
          /* Job already has a requested VLAN feature, use that */
          
          /* Force feature set on each req -- This is redundant, but ensures its set on each req. */

          for (RQIndex = 0;VMJob->Req[RQIndex] != NULL;RQIndex++)
            bmset(&VMJob->Req[RQIndex]->ReqFBM,FIndex);

          return(SUCCESS);
          }
        }  /* END for (NSIndex) */
      }  /* END for (FIndex) */
    }  /* END for (RQIndex = 0;VMJob->Req[RQIndex] != NULL;RQIndex++) */

  /* No VLAN feature requested */

  bmset(&QueryFlags,mcmVerbose);
  bmset(&QueryFlags,mcmFuture);
  bmset(&QueryFlags,mcmUseTID);
  bmset(&QueryFlags,mcmSummary);

  MUSNInit(&BPtr,&BSpace,tmpXML,sizeof(tmpXML));

  MUSNPrintF(&BPtr,&BSpace,"<Request action=\"query\"");
  MUSNPrintF(&BPtr,&BSpace," flags=\"summary,future,tid,policy\"");
  MUSNPrintF(&BPtr,&BSpace," option=\"intersection,noaggregate\"><Object>");
  MUSNPrintF(&BPtr,&BSpace,"cluster</Object>");

  /* Just need a feature, put 1:00:00 as default so that mshow -a will work */

  /* NOTE: we used to perform a different mshow request if storage was requested, but
   * we are no longer doing this as it has proven difficult to get a PRIORITY node
   * allocation policy to work well with both storage and compute resources at this 
   * early stage. Because we are just selecting a feature (for VLAN determination),
   * we do not need to get fancy with storage--a single request for compute resources
   * will suffice for now. */

  /* Proc based request */

  MUSNPrintF(&BPtr,&BSpace,"<Set><Where name=\"mintasks\">1@procs:%d</Where>",
    VMJob->VMCreateRequest->CRes.Procs);

  if ((VMJob->VMCreateRequest->Walltime > 0) && (VMJob->VMCreateRequest->Walltime < MCONST_DAYLEN * 50))
    {
    char    *VTString = NULL;

    if (MULToVTString(VMJob->VMCreateRequest->Walltime,&VTString) == SUCCESS)
      MUSNPrintF(&BPtr,&BSpace,"<Where name=\"duration\">%s</Where>",VTString);

    MUFree(&VTString);
    }
  else
    {
    MUSNPrintF(&BPtr,&BSpace,"<Where name=\"duration\">%s</Where>","1:00:00");
    }

  MUSNPrintF(&BPtr,&BSpace,"<Where name=\"qos\">vmtracking</Where>");

  /* Add features */

  if (!bmisclear(&VMJob->VMCreateRequest->FBM))
    {
    char  Line[MMAX_LINE];

    MUNodeFeaturesToString(',',&VMJob->VMCreateRequest->FBM,Line);
    if (!MUStrIsEmpty(Line))
      MUSNPrintF(&BPtr,&BSpace,"<Where name=\"nodefeature\">%s</Where>",Line);
    }

  MUSNPrintF(&BPtr,&BSpace,"<Where name=\"vmusage\">requirepm</Where></Set>");
  MUSNPrintF(&BPtr,&BSpace,"</Request>");

  /* Create output XML structure */
  MXMLCreateE(&AE,(char *)MSON[msonData]);
  MXMLAddChild(AE,"Object","cluster",NULL);

  /* Convert XML string to XML structure */
  MXMLFromString(&RE,tmpXML,NULL,NULL);

  /* We want MClusterShowARes to give us just the first option */
  TmpRQD = MSched.ResourceQueryDepth;
  MSched.ResourceQueryDepth = 1;

  if (MClusterShowARes(
      MSched.Admin[1].UName[0],
      mwpXML,          /* response format */
      &QueryFlags,     /* query flags */
      RRange,          /* I (modified) */
      mwpXML,          /* source format */
      (void *)RE,      /* source data */
      (void **)&AE,    /* response data */
      0,
      0,
      NULL,
      TmpEMsg) == FAILURE)
    {
    MSched.ResourceQueryDepth = TmpRQD;

    MDB(7,fSTRUCT) MLog("ERROR:    failed to find VLAN feature - '%s'\n",
      TmpEMsg);

    MXMLDestroyE(&RE);
    MXMLDestroyE(&AE);

    return(FAILURE);
    }

  MSched.ResourceQueryDepth = TmpRQD;

  MDB(7,fSTRUCT)
    {
    mstring_t Result(MMAX_LINE);

    if (MXMLToMString(AE,&Result,NULL,TRUE) == SUCCESS)
      {
      MLog("INFO:     result from 'mshow -a' for VM Storage request -- %s\n",Result.c_str());
      }
    }

  CTok = -1;

  if ((MXMLGetChild(AE,(char *)MXO[mxoPar],&CTok,&PE) == FAILURE) ||
      (MXMLGetChild(AE,(char *)MXO[mxoPar],&CTok,&PE) == FAILURE))
    {
    MXMLDestroyE(&AE);
    MXMLDestroyE(&RE);

    MDB(7,fSTRUCT) MLog("ERROR:    could not find suitable hypervisor feature (no valid results)\n");

    return(FAILURE);
    }

  CTok = -1;

  if (MXMLGetChild(PE,"range",&CTok,&CE) == SUCCESS)
    {
    char TIDStr[MMAX_NAME];
    int TID;
    long tmpStartTime;
    long tmpEndTime;

    mrsv_t *OrigTmpRsv = NULL;

    if (MXMLGetAttr(CE,"tid",NULL,TIDStr,sizeof(TIDStr)) == FAILURE)
      {
      return(FAILURE);
      }

    TID = (int)strtol(TIDStr,NULL,10);

    MRsvInitialize(&TmpRsv);

    MRsvSetAttr(TmpRsv,mraResources,(void *)&TID,mdfInt,mSet);

    snprintf(AdminName,sizeof(AdminName),"USER==%s",
      MSched.Admin[1].UName[0]);
    MRsvSetAttr(TmpRsv,mraACL,(void *)AdminName,mdfString,mSet);

    tmpStartTime = TmpRsv->StartTime;
    tmpEndTime = TmpRsv->EndTime;

    if ((mulong)tmpStartTime < MSched.Time)
      {
      tmpStartTime = MAX(MSched.Time,(mulong)tmpStartTime);
      }

    MRsvSetTimeframe(TmpRsv,mraStartTime,mSet,tmpStartTime,NULL);
    MRsvSetAttr(TmpRsv,mraEndTime,(void *)&tmpEndTime,mdfLong,mSet);

    if ((mulong)tmpStartTime < MSched.Time)
      {
      tmpStartTime = MAX(MSched.Time,(mulong)tmpStartTime);
      }

    /* Need to save the pointer because MRsvDestroy will set the pointer
       to NULL, but doesn't actually do a MUFree */

    OrigTmpRsv = TmpRsv;

    /* Create reservation */

    if (MRsvConfigure(
          NULL,
          TmpRsv,
          0,
          0,
          NULL,
          &R,
          NULL,
          FALSE) == FAILURE)
      {
      MDB(7,fSTRUCT) MLog("ERROR:    could not find VLAN feature\n");

      MXMLDestroyE(&AE);
      MXMLDestroyE(&RE);

      MRsvDestroy(&TmpRsv,FALSE,FALSE);
      MUFree((char **)&OrigTmpRsv);

      return(FAILURE);
      }

    MRsvDestroy(&TmpRsv,FALSE,FALSE);
    MUFree((char **)&OrigTmpRsv);
    } /* END if (MXMLGetChild(PE,"range",&CTok,&CE) == SUCCESS) */

  if ((R == NULL) || 
      (MNLIsEmpty(&R->NL)))
    {
    MDB(7,fSTRUCT) MLog("ERROR:    could not find VLAN feature (no reservation)\n");

    MXMLDestroyE(&AE);
    MXMLDestroyE(&RE);

    MRsvDestroy(&R,TRUE,TRUE);

    return(FAILURE);
    }

  MNodeGetVLAN(MNLReturnNodeAtIndex(&R->NL,0),Feature);
  FeatureIndex =  MUMAGetIndex(meNFeature,Feature,mVerify);

  if (FeatureIndex == 0)
    {
    MDB(7,fSTRUCT) MLog("ERROR:    could not find VLAN feature '%s'\n",
      Feature);

    MXMLDestroyE(&AE);
    MXMLDestroyE(&RE);

    MRsvDestroy(&R,TRUE,TRUE);

    return(FAILURE);
    }

  /* Need to put the feature on all reqs */

  for (RQIndex = 0;VMJob->Req[RQIndex] != NULL;RQIndex++)
    {
    bmset(&VMJob->Req[RQIndex]->ReqFBM,FeatureIndex);
    }

  MXMLDestroyE(&AE);
  MXMLDestroyE(&RE);

  MRsvDestroy(&R,TRUE,TRUE);

  return(SUCCESS);
  } /* END MVMChooseVLANFeature */




/**
 * Uses nodeset functionality to determine if migration source and target 
 * are in the same VLAN
 * 
 * @see MUSubmitVMMigrationJob - parent
 * 
 * @param SrcNode (I)
 * @param DstNode (I)
 */

int MVMCheckVLANMigrationBoundaries(
      
  mnode_t   *SrcNode,
  mnode_t   *DstNode)

  {
  int rc = 0;

  mnl_t nodeSetConstraints = {0};

  mjob_t FakeJ;
  msysjob_t FakeSysJ;

  /* cook up a job with appropriate requirements */
  memset(&FakeJ,0,sizeof(mjob_t));    
  memset(&FakeSysJ,0,sizeof(msysjob_t));

  strcpy(FakeJ.Name,"premigrate.test");
  FakeJ.System = &FakeSysJ;
  FakeJ.Request.NC = 1;

  /* cook up a node list with the source and destination nodes */

  MNLInit(&nodeSetConstraints);

  /* Setup the SrcNode & DstNode in constraint list for MJobSelectResourceSet */

  MNLSetNodeAtIndex(&nodeSetConstraints,0,SrcNode);
  MNLSetNodeAtIndex(&nodeSetConstraints,1,DstNode);

  /* Set TaskCount=1 for both the SrcNode & DstNode, otherwise MJobSelectResourceSet will fail. */

  MNLSetTCAtIndex(&nodeSetConstraints,0,1);
  MNLSetTCAtIndex(&nodeSetConstraints,1,1);

  rc = MJobSelectResourceSet(
        &FakeJ,
        (mreq_t*) NULL,
        MSched.Time,
        MPar[0].NodeSetAttribute,
        MPar[0].NodeSetPolicy,
        MPar[0].NodeSetList,
        &nodeSetConstraints,    /* I/O */
        NULL, /* node map */
        -1,
        NULL,
        NULL,
        NULL);

  MNLFree(&nodeSetConstraints);

  return(rc);
  }  /* END MVMCheckVLANMigrationBoundaries() */


/* END MVMVlan.c */
