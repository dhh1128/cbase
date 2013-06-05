/* HEADER */

      
/**
 * @file MRM_XML.c
 *
 * Contains: MRMToXML
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 * Report resource manager attributes as XML.
 *
 * @see MRMToString() - parent
 * @see MRMStoreCP() - parent
 *
 * @param R (I)
 * @param RMEP (O)
 * @param SAList (I) [optional]
 */

int MRMToXML(

  mrm_t             *R,      /* I */
  mxml_t           **RMEP,   /* O */
  enum MRMAttrEnum  *SAList) /* I (optional) */

  {
  enum MRMAttrEnum DAList[] = {
    mrmaAuthType,
    mrmaAuthAList,
    mrmaAuthCList,
    mrmaAuthGList,
    mrmaAuthQList,
    mrmaAuthUList,
    mrmaCheckpointSig,
    mrmaCheckpointTimeout,
    mrmaClusterQueryURL,
    mrmaConfigFile,
    mrmaCredCtlURL,
    mrmaCSAlgo,
    mrmaDataRM,
    mrmaDefDStageInTime,
    mrmaDefHighSpeedAdapter,
    mrmaDataRM,
    mrmaEPort,
    mrmaFlags,
    mrmaHost,
    mrmaJobCount,
    mrmaJobCounter,
    mrmaJobStartCount,
    mrmaLocalDiskFS,
    /* mrmaMessages, - msgs are currently not safe for XML (need to update like job msgs) */
    mrmaMinETime,
    mrmaName,
    mrmaNC,
    mrmaNMPort,
    mrmaNMServer,
    mrmaNodeFailureRsvProfile,
    mrmaNodeList,
    mrmaOMap,
    mrmaPartition,
    mrmaPort,
    mrmaProcs,
    mrmaProfile,
    mrmaResourceType,
    mrmaSocketProtocol,
    mrmaSQLData,
    mrmaState,
    mrmaStats,
    mrmaSubmitCmd,
    mrmaSuspendSig,
    mrmaTargetUsage,
    mrmaTimeout,
    mrmaType,
    mrmaUpdateTime,
    mrmaVersion,
    mrmaVMOwnerRM,
    mrmaWireProtocol,
    mrmaWorkloadQueryURL,
    mrmaNONE };

  int  aindex;
  int  pindex;

  enum MRMAttrEnum *AList;

  char tmpBuffer[MMAX_BUFFER];

  mxml_t *RME;

  if ((R == NULL) || (RMEP == NULL))
    {
    return(FAILURE);
    }

  if ((*RMEP == NULL) && (MXMLCreateE(RMEP,(char *)MXO[mxoRM]) == FAILURE))
    {
    return(FAILURE);
    }

  RME = *RMEP;

  if (SAList != NULL)
    AList = SAList;
  else
    AList = DAList;

  for (aindex = 0;AList[aindex] != mrmaNONE;aindex++)
    {
    if ((MRMAToString(R,AList[aindex],tmpBuffer,sizeof(tmpBuffer),0) == FAILURE) ||
        (tmpBuffer[0] == '\0'))
      {
      continue;
      }

    MXMLSetAttr(
      RME,
      (char *)MRMAttr[AList[aindex]],
      (void *)tmpBuffer,
      mdfString);
    }  /* END for (aindex) */

  /* checkpoint local queue information */

  if (bmisset(&R->IFlags,mrmifLocalQueue) &&
      (MSched.LoadAllJobCP == FALSE) &&
      (MUHTIsInitialized(&R->U.S3.JobListIndex) == SUCCESS))
    {
    char *tmpBuf;
    int   tmpBufLen;

    mhashiter_t JobListIter;
    char *JobName = NULL;

    mjob_t *J;

    mxml_t *QE = NULL;

    MUHTIterInit(&JobListIter);

    MXMLCreateE(&QE,(char *)MXO[mxoQueue]);

    tmpBuf = NULL;
    tmpBufLen = 0;

    while (MUHTIterate(&R->U.S3.JobListIndex,&JobName,NULL,NULL,&JobListIter) == SUCCESS)
      {
      if (MJobFind(JobName,&J,mjsmBasic) == SUCCESS)
        {
        if (MJobShouldCheckpoint(J,R) == FALSE)
          continue;
        }

      MUStrAppend(&tmpBuf,&tmpBufLen,JobName,',');  /* consider using MUSNXPrintF to decrease heap fragmentation */
      }

    MXMLSetAttr(QE,(char *)MXO[mxoJob],tmpBuf,mdfString);

    MXMLAddE(RME,QE);

    MUFree(&tmpBuf);
    }  /* END if ((bmisset(&R->IFlags,mrmifLocalQueue)) && ...) */

  /* create peer interface child */

  for (pindex = 0;pindex < MMAX_RMSUBTYPE;pindex++)
    {
    mxml_t *PE;

    if (bmisset(&R->RTypes,mrmrtLicense) ||
        bmisset(&R->RTypes,mrmrtNetwork) ||
        bmisset(&R->RTypes,mrmrtProvisioning) ||
        bmisset(&R->RTypes,mrmrtStorage))
      {
      /* non-compute RM's may not have interface type specified since they
         may route directly through file or CLI interfaces */

      if (R->P[pindex].RespTotalCount[0] <= 0)
        continue;
      }
    else
      { 
      if (R->P[pindex].Type == mpstNONE)
        continue;
      }

    PE = NULL;

    MXMLCreateE(&PE,"psi");

    MPSIToXML(&R->P[pindex],PE);

    MXMLAddE(RME,PE);
    }  /* END for (pindex) */

  if (bmisset(&R->RTypes,mrmrtLicense))
    {
    int rindex;
    int IC;

    double tmpD;

    double totalCfg;
    double totalDed;

    mxml_t *GRE;

    IC = MAX(1,R->U.Nat.ICount);

    totalCfg = 0.0;
    totalDed = 0.0;

    for (rindex = 0;rindex < MSched.M[mxoxGRes];rindex++)
      {
      if ((R->U.Nat.ResName == NULL) ||
          (R->U.Nat.ResName[rindex][0] == '\0'))
        break;

      /* Only include license information that applies to this resource manager */

      if (MSNLGetIndexCount(&R->U.Nat.ResConfigCount,rindex) == 0)
        continue;

      GRE = NULL;

      MXMLCreateE(&GRE,"license");

      MXMLSetAttr(GRE,"name",(void *)R->U.Nat.ResName[rindex],mdfString);

      MXMLSetAttr(GRE,"ic",(void *)&IC,mdfInt);

      tmpD = (double)(MSNLGetIndexCount(&R->U.Nat.ResAvailCount,rindex)) / IC;

      MXMLSetAttr(GRE,"averageAvail",(void *)&tmpD,mdfDouble);

      tmpD = (double)(MSNLGetIndexCount(&R->U.Nat.ResConfigCount,rindex)) / IC;

      MXMLSetAttr(GRE,"averageConfig",(void *)&tmpD,mdfDouble);

      /* NOTE: percentages below are over all iterations since Moab started */

      tmpD = (double)(MSNLGetIndexCount(&R->U.Nat.ResFreeICount,rindex)) / IC;

      MXMLSetAttr(GRE,"percentIdle",(void *)&tmpD,mdfDouble);

      tmpD = (double)(MSNLGetIndexCount(&R->U.Nat.ResAvailICount,rindex) - MSNLGetIndexCount(&R->U.Nat.ResFreeICount,rindex)) / IC;

      MXMLSetAttr(GRE,"percentActive",(void *)&tmpD,mdfDouble);

      tmpD = (double)(R->U.Nat.ICount - MSNLGetIndexCount(&R->U.Nat.ResAvailICount,rindex)) / IC;

      MXMLSetAttr(GRE,"percentBusy",(void *)&tmpD,mdfDouble);

      if (R->U.Nat.N != NULL)
        {
        mnode_t *GN;

        GN = (mnode_t *)R->U.Nat.N;
          
        tmpD = (double)(MSNLGetIndexCount(&GN->CRes.GenericRes,rindex));

        MXMLSetAttr(GRE,"currentConfig",(void *)&tmpD,mdfDouble);

        tmpD = (double)(MSNLGetIndexCount(&GN->ARes.GenericRes,rindex));

        totalCfg += tmpD;

        MXMLSetAttr(GRE,"currentAvail",(void *)&tmpD,mdfDouble);

        tmpD = (double)(MSNLGetIndexCount(&GN->DRes.GenericRes,rindex));

        MXMLSetAttr(GRE,"currentDedicated",(void *)&tmpD,mdfDouble);

        tmpD = (double)(MSNLGetIndexCount(&GN->DRes.GenericRes,rindex));

        totalDed += tmpD;
        }

      MXMLAddE(RME,GRE);
      }  /* END for (rindex) */

    tmpD = totalDed / MAX(1.0,totalCfg);

    MXMLSetAttr(RME,"LOAD",(void *)&tmpD,mdfDouble);
    }    /* END if (bmisset(&R->RTypes,mrmrtLicense)) */

  if (bmisset(&R->RTypes,mrmrtNetwork))
    {
    if (!MNLIsEmpty(&R->NL))
      {
      double   tmpD;
      mnode_t *N;
      int      gindex;

      MNLGetNodeAtIndex(&R->NL,0,&N);

      gindex = MUMAGetIndex(
        meGRes,
        "bandwidth",
        mVerify);

      if (gindex > 0)
        {
        tmpD = MSNLGetIndexCount(&N->ARes.GenericRes,gindex) / MAX(1.0,MSNLGetIndexCount(&N->CRes.GenericRes,gindex));

        tmpD = 1.0 - tmpD;

        MXMLSetAttr(RME,"LOAD",(void *)&tmpD,mdfDouble);
        } 
      }
    }    /* END if (bmisset(&R->RTypes,mrmrtNetwork)) */

  if (bmisset(&R->RTypes,mrmrtProvisioning))
    {
    /* report ratio of I/O (ie, install) capacity currently in use */

    /* NYI */
    }

  if (bmisset(&R->RTypes,mrmrtStorage))
    {
    if (!MNLIsEmpty(&R->NL))
      {
      double   tmpD;
      mnode_t *N;

      MNLGetNodeAtIndex(&R->NL,0,&N);

      tmpD = N->ARes.Disk / MAX(1.0,N->CRes.Disk);

      tmpD = 1.0 - tmpD;

      MXMLSetAttr(RME,"LOAD",(void *)&tmpD,mdfDouble);
      }  /* END if (bmisset(&R->RTypes,mrmrtStorage)) */
    }

  return(SUCCESS);
  }  /* END MRMToXML() */



/**
 * Create/update RM object from XML that represents an RM.
 *
 * @param R (I) A valid pointer to an RM that will accept the loaded attributes. [modified]
 * @param E (I) The parsed XML that the RM should be loaded from.
 */

int MRMFromXML(

  mrm_t  *R,   /* I (modified) */
  mxml_t *E)   /* I */

  {
  int aindex;

  enum MRMAttrEnum raindex;

  int pindex;
  int CTok;

  mxml_t *CE;

  const char *FName = "MRMFromXML";

  MDB(3,fSTRUCT) MLog("%s(%s,%s)\n",
    FName,
    (R != NULL) ? "R" : "NULL",
    (E != NULL) ? "E" : "NULL");

  if ((R == NULL) || (E == NULL))
    {
    return(FAILURE);
    }

  /* NOTE:  do not initialize--may be overlaying existing data */

  for (aindex = 0;aindex < E->ACount;aindex++)
    {
    raindex = (enum MRMAttrEnum)MUGetIndex(E->AName[aindex],MRMAttr,FALSE,0);

    if (raindex == mrmaNONE)
      continue;

    MRMSetAttr(R,raindex,(void **)E->AVal[aindex],mdfString,mSet);
    }  /* END for (aindex) */

  if (MXMLGetChild(E,(char *)MXO[mxoQueue],NULL,&CE) == SUCCESS)
    {
	/* NOTE: We may want to look at making this dynamic. It's only a matter of time
			 before somebody else blows this buffer again. */

    char Buffer[MMAX_BUFFER<<4];

    /* load jobs from the internal queue */

    if (MXMLGetAttr(CE,(char *)MXO[mxoJob],NULL,Buffer,sizeof(Buffer)) == SUCCESS)
      {
      MS3InitializeLocalQueue(R,Buffer);
      }
    }

  CTok = -1;
  pindex = 0;

  while (MXMLGetChild(E,"psi",&CTok,&CE) == SUCCESS)
    {
    /* set default */

    if (R->P[pindex].Type == mpstNONE)
      R->P[pindex].Type = mpstQM;

    MPSIFromXML(&R->P[pindex++],CE);
    }

  return(SUCCESS);
  }  /* END MRMFromXML() */



/**
 *
 *
 * @param E (I/O) [modified]
 */

int MRMAddLangToXML(
  
  mxml_t *E)  /* I/O (modified) */

  {
  char Languages[MMAX_LINE];
  char tmpName[MMAX_NAME];
  mrm_t *R;
  int rmindex;

  if (E == NULL)
    {
    return(FAILURE);
    }

  /* add languages that this virtual node supports */

  Languages[0] = '\0';

  for (rmindex = 0;rmindex < MSched.M[mxoRM];rmindex++)
    {
    R = &MRM[rmindex];

    if (R->Name[0] == '\0')
      break;

    if (MRMIsReal(R) == FALSE)
      continue;

    if (MRMFunc[R->Type].JobSubmit == NULL)
      continue;

    snprintf(tmpName,sizeof(tmpName),"%s%s%s,",
      MRMType[R->Type],
      (R->SubType != mrmstNONE) ? ":" : "",
      (R->SubType != mrmstNONE) ? MRMSubType[R->SubType] : "");

    /* make sure we don't already have this subtype */

    if (strstr(Languages,tmpName) != NULL)
      {
      continue;
      }

    MUStrCat(Languages,tmpName,sizeof(Languages));
    }  /* END for (rmindex) */

  if (MXMLSetAttr(E,"rmlanguages",(void **)&Languages,mdfString) == FAILURE)
    {
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MRMAddLangToXML() */



/* END MRM_XML.c */
