/* HEADER */

/**
 * @file MUIVMCtlThreadSafe.c
 *
 * Contains MUI VM Control function, thread safe operation
 */



#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 * process mvmctl -q request in a threadsafe way.
 *
 * @param S       (I) [modified]
 * @param CFlagBM (I) enum MRoleEnum bitmap
 * @param Auth    (I) authorization credentials for request
 */

int MUIVMCtlThreadSafe(

  msocket_t *S,       /* I (modified) */
  mbitmap_t *CFlagBM, /* I enum MRoleEnum bitmap */
  char      *Auth)    /* I */

  {
  int rindex;
  int WTok;

  char DiagOpt[MMAX_LINE];
  char FlagString[MMAX_LINE];
  char VMID[MMAX_LINE];
  char tmpName[MMAX_LINE];
  char tmpVal[MMAX_LINE];

  marray_t VMConstraintList;
  mvmconstraint_t VMConstraint;

  mxml_t *RDE;
  mxml_t *DE;
  mxml_t *VME;
  mxml_t *WE;

  mtransvm_t *V;

  marray_t VMList;

  mdb_t *MDBInfo;

  mgcred_t *U;

  mbitmap_t CFlags;  /* bitmap of enum MCModeEnum */

  const enum MVMAttrEnum DAList[] = {
    mvmaActiveOS,
    mvmaADisk,
    mvmaAlias,
    mvmaAMem,
    mvmaAProcs,
    mvmaCDisk,
    mvmaCMem,
    mvmaCProcs,
    mvmaCPULoad,
    mvmaContainerNode,
    mvmaDDisk,
    mvmaDMem,
    mvmaDProcs,
    mvmaDescription,
    mvmaEffectiveTTL,
    mvmaFlags,
    mvmaGMetric,
    mvmaID,
    mvmaJobID,
    mvmaLastMigrateTime,
    mvmaLastSubState,
    mvmaLastSubStateMTime,
    mvmaLastUpdateTime,
    mvmaMigrateCount,
    mvmaNetAddr,
    mvmaNextOS,
    mvmaOSList,
    mvmaPowerIsEnabled,
    mvmaPowerState,
    mvmaProvData,
    mvmaRackIndex,
    mvmaSlotIndex,
    mvmaSovereign,
    mvmaSpecifiedTTL,
    mvmaStartTime,
    mvmaState,
    mvmaSubState,
    mvmaStorageRsvNames,
    mvmaTrackingJob,
    mvmaVariables,
    mvmaLAST };

  VMID[0] = '\0';

  if (S->RDE != NULL)
    {
    RDE = S->RDE;
    }
  else
    {
    RDE = NULL;

    if (MXMLFromString(&RDE,S->RPtr,NULL,NULL) == FAILURE)
      {
      MUISAddData(S,"ERROR:  corrupt command received\n");

      return(FAILURE);
      }
    }

  MUArrayListCreate(&VMList,sizeof(mtransvm_t *),10);

  MSchedGetAttr(msaDBHandle,mdfNONE,(void **)&MDBInfo,-1);

  if (MXMLGetAttr(RDE,MSAN[msanFlags],NULL,FlagString,sizeof(FlagString)) == SUCCESS)
    bmfromstring(FlagString,MClientMode,&CFlags);

  MUArrayListCreate(&VMConstraintList,sizeof(mvmconstraint_t),10);

  WE = NULL;
  WTok = -1;

  /* Parse any 'where' clauses for this command */
  while (MS3GetWhere(
          RDE,
          &WE,
          &WTok,
          tmpName,
          sizeof(tmpName),
          tmpVal,
          sizeof(tmpVal)) == SUCCESS)
    {
    /* Check the keyword (tmpName) if it is a 'msanName' string [indexed into MSAN] */
    if (!strcmp(tmpName,MSAN[msanName]))
      {
      VMConstraint.AType = mvmaID;
      VMConstraint.ACmp = mcmpEQ2;

      MUStrCpy(VMConstraint.AVal[0],tmpVal,sizeof(VMConstraint.AVal[0]));

      MUArrayListAppend(&VMConstraintList,(void *)&VMConstraint);
      }
    else
      {
      enum MVMAttrEnum AIndex;

      /* Then check for a VM Attribute keywords in a 'where' clause */

      AIndex = (enum MVMAttrEnum) MUGetIndexCI(tmpName,MVMAttr,FALSE,mvmaNONE);

      /* Now process the attribute of a VM, if any found */
      switch(AIndex)
        {
          case mvmaState:
            VMConstraint.AType = mvmaState;
            VMConstraint.ACmp = mcmpEQ;
            MUStrCpy(VMConstraint.AVal[0],tmpVal,sizeof(VMConstraint.AVal[0]));
            MUArrayListAppend(&VMConstraintList,(void*)&VMConstraint);
            break;

          default:
            /* not found, skip on */
            break;
         } /* End switch(AIndex) */
      }
    } /* END while(MS3GetWhere(....) */

  /* The constraint list is populated now, so time to apply the constraint list
   * against the nodes specified by the database, and generate a
   * list that matches the constraints
   */

  /* Lock the list, then apply the constraint list: 
   *  Resultant list back into 'VMList'
   */
  MCacheVMLock(TRUE,TRUE);

  /* Apply the list of VMs against the newly built VM ConstraintList as per the user request */
  MCacheReadVMs(&VMList,&VMConstraintList);

  /* Done with the constraint list, free it */
  MUArrayListFree(&VMConstraintList);

  if (MXMLCreateE(&S->SDE,(char *)MSON[msonData]) == FAILURE)
    {
    MUISAddData(S,"ERROR:    cannot create response\n");

    MCacheVMLock(FALSE,TRUE);
    MUArrayListFree(&VMList);
    return(FAILURE);
    }

  DE = S->SDE;

  /* find the user who issued the request */

  MUserAdd(Auth,&U);

  /* get the argument flags for the request */

  if (MXMLGetAttr(RDE,MSAN[msanFlags],NULL,FlagString,sizeof(FlagString)) == SUCCESS)
    bmfromstring(FlagString,MClientMode,&CFlags);

  /* look for a specified VMID */

  if (MXMLGetAttr(RDE,MSAN[msanOp],NULL,DiagOpt,sizeof(DiagOpt)) == SUCCESS)
    {
    MUStrCpy(VMID,DiagOpt,sizeof(VMID));
    }

  /* loop through all vm's and create an xml element for each, adding
   * it to the return data element */

  for (rindex = 0;rindex < VMList.NumItems;rindex++)
    {
    V = (mtransvm_t *)MUArrayListGetPtr(&VMList,rindex);

    if ((V == NULL) || (MUStrIsEmpty(V->VMID)))
      break;

    if (V->VMID[0] == '\1')
      continue;

    if (MUICheckAuthorization(
          U,
          NULL,
          NULL,
          mxoxVM,
          mcsDiagnose,
          mrcmQuery,
          NULL,
          NULL,
          0) == FAILURE)
      {
      /* no authority to diagnose reservation */

      continue;
      } /* END if (MUICheckAuthorization...) */

    MDB(6,fUI) MLog("INFO:     evaluating vm[%d] '%s'\n",
      rindex,
      V->VMID);

    /* XML will be alloc'd inside of MVMTransitionToXML() */

    VME = NULL;

    MVMTransitionToXML(
      V,
      &VME,
      (enum MVMAttrEnum *)DAList);

    MXMLAddE(DE,VME);
    }  /* END for (rindex) */

  /* Release the VM Cache lock */
  MCacheVMLock(FALSE,TRUE);
  
  /* Free the temp VM list that was sent back to client */
  MUArrayListFree(&VMList);

  return(SUCCESS);
  } /* end MUIVMCtlThreadSafe() */
/* END MUIVMCtlThreadSafe.c */
