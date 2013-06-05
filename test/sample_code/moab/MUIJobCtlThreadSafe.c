/* HEADER */

/**
 * @file MUIJobCtlThreadSafe.c
 *
 * Contains Thread Safe MUI Job Control functions
 */



#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  




/**
 * Process 'mdiag -j' request in a threadsafe way.
 *
 * @param S (I) [modified]
 * @param CFlagBM (I) enum MRoleEnum bitmap
 * @param Auth (I)
 */

int MUIJobCtlThreadSafe(

  msocket_t *S,       /* I (modified) */
  mbitmap_t *CFlagBM, /* I enum MRoleEnum bitmap */
  char      *Auth)    /* I */

  {
  int rindex;
  int CTok;

  enum MJobAttrEnum AIndex;

  char DiagOpt[MMAX_LINE];
  char FlagString[MMAX_LINE];
  char tmpVal[MMAX_LINE];
  char tmpName[MMAX_LINE];

  marray_t JConstraintList;
  mjobconstraint_t JConstraint;

  mxml_t *DE;
  mxml_t *JE;

  mtransjob_t *J;

  marray_t JList;

  mdb_t *MDBInfo;

  mbitmap_t AuthBM;  /* bitmap of enum MRoleEnum */

  mbool_t ShowArraySubJobs = FALSE;

  mbool_t ExcludeCompleted = TRUE;  /* whether to show completed jobs */

  mbitmap_t CFlags;  /* bitmap of enum MCModeEnum */

  /* check to see of user authorized to run this command */

  MSysGetAuth(Auth,mcsDiagnose,0,&AuthBM);

  if (!bmisset(&AuthBM,mcalOwner) && !bmisset(&AuthBM,mcalGranted))
    {
    char tmpLine[MMAX_LINE];

    snprintf(tmpLine,sizeof(tmpLine),"ERROR:    user '%s' is not authorized to run command '%s'\n",
      Auth,
      MUI[mcsDiagnose].SName);

    MUISAddData(S,tmpLine);

    return(FAILURE);
    }

  MUArrayListCreate(&JList,sizeof(mtransjob_t *),10);

  MSchedGetAttr(msaDBHandle,mdfNONE,(void **)&MDBInfo,-1);

  if (MXMLGetAttr(S->RDE,MSAN[msanFlags],NULL,FlagString,sizeof(FlagString)) == SUCCESS)
    bmfromstring(FlagString,MClientMode,&CFlags);

  /* Get job name provided by user */

  MUArrayListCreate(&JConstraintList,sizeof(mjobconstraint_t),10);  

  if (MXMLGetAttr(S->RDE,MSAN[msanOp],NULL,DiagOpt,sizeof(DiagOpt)) == SUCCESS)
    {

    /* Build job constraint list to only get job[s] specified */
    /* DiagOpt might be a comma-separated list of jobs or "ALL"  */
          
    JConstraint.AType = mjaJobID;
    JConstraint.ACmp = mcmpEQ2;
    MUStrCpy(JConstraint.AVal[0],DiagOpt,sizeof(JConstraint.AVal[0]));
    MUArrayListAppend(&JConstraintList,(void *)&JConstraint);

    /* because the job is explicitly listed, then Completed jobs _should_ be shown */

    ExcludeCompleted = FALSE;

    /* if the user specified a job name of an array sub-job, enable showing it */

    ShowArraySubJobs = TRUE;


    }

  CTok = -1;

  while (MS3GetWhere(
      S->RDE,
      NULL,
      &CTok,
      tmpName,         /* O */
      sizeof(tmpName),
      tmpVal,         /* O */
      sizeof(tmpVal)) == SUCCESS)
    {

    AIndex = (enum MJobAttrEnum)MUGetIndexCI(tmpName,MJobAttr,FALSE,mjaNONE);

    switch (AIndex)
      {
      case mjaNONE:
        /* MOAB-3215  LLNL is relying on the fact that "completed" is a 
         * constraint type that will show completed jobs.  Here we 
         * handle that special case by creating an artificial constraint 
         * for that purpose. 
         *  
         * MOAB-3315 has added "system" & "ALL" to the list.  So, we'll add 
         * them to the constraint list and let MJobTransitionMatchesConstraints make 
         * the decision.  */ 

        if ((!strcmp(tmpName,"completed")) || (!strcmp(tmpName,"ALL")))
          ExcludeCompleted = FALSE;

        JConstraint.AType = mjaState;
        JConstraint.ACmp = mcmpEQ2;
        MUStrCpy(JConstraint.AVal[0],tmpName,sizeof(JConstraint.AVal[0]));
        MUArrayListAppend(&JConstraintList,(void *)&JConstraint);

        break;

      case mjaState:

        /* if the state is being specified, do NOT exclude completed jobs--
         * let the explicitly specified state flags speak for themselves... */
        ExcludeCompleted = FALSE;

        /* fall through... */

      case mjaJobID:
      case mjaUser:
      case mjaGroup:
      case mjaAccount:
      case mjaClass:
      case mjaQOS:
      default:
      
        JConstraint.AType = AIndex;
        JConstraint.ACmp  = mcmpEQ2;
        MUStrCpy(JConstraint.AVal[0],tmpVal,sizeof(JConstraint.AVal[0]));
        MUArrayListAppend(&JConstraintList,(void *)&JConstraint);
        break;

      case mjaArrayMasterID:

        JConstraint.AType = AIndex;
        JConstraint.ACmp  = mcmpEQ;
        JConstraint.ALong[0] = strtol(tmpVal,NULL,10);
        MUArrayListAppend(&JConstraintList,(void *)&JConstraint);
        break;

      }  /* END switch (AIndex) */
    }   /* END MS3GetWhere() */

  if (ExcludeCompleted == TRUE)
    {
    /* This is the default if there are no other specifications. */
    /* don't get jobs that are in a completed state */

    JConstraint.AType = mjaState;
    JConstraint.ACmp = mcmpNE;

    JConstraint.ALong[0] = mjsCompleted;
    JConstraint.ALong[1] = mjsRemoved;
    JConstraint.ALong[2] = mjsVacated;
    JConstraint.ALong[3] = mjsNONE;

    MUArrayListAppend(&JConstraintList,(void *)&JConstraint);
    } /* END else */

  MCacheJobLock(TRUE,TRUE);
  MCacheReadJobs(&JList,&JConstraintList);

  MUArrayListFree(&JConstraintList);

  if (MXMLCreateE(&S->SDE,(char *)MSON[msonData]) == FAILURE)
    {
    MUISAddData(S,"ERROR:    cannot create response\n");

    MCacheJobLock(FALSE,TRUE);
    MUArrayListFree(&JList);
    return(FAILURE);
    }

  DE = S->SDE;

  /* only sort if the SORT flag is used */

  /* NYI OPTIMIZATION: only sort the array masters */

  if (bmisset(&CFlags,mcmSort) && (JList.NumItems > 1))
    {
    /* if there's more than one job in the list, sort the list by JobID */

    qsort(
      (void *)JList.Array,
      JList.NumItems,
      sizeof(mtransjob_t *),
      (int(*)(const void *,const void *))__MTransJobNameCompLToH);
    }

  for (rindex = 0;rindex < JList.NumItems;rindex++)
    {
    J = (mtransjob_t *)MUArrayListGetPtr(&JList,rindex);

    if ((bmisset(&J->TFlags,mtransfArraySubJob)) && (!ShowArraySubJobs))
      continue;

    if(J->Requirements[0] == NULL)
       {
       MDB(0,fSCHED) MLog("ALERT:    Job %s has no valid request attached.\n",
         (J->Name != NULL) ? J->Name : "NULL");
       continue;
       }

    __MUIJobTransitionToXML(J,&CFlags,&JE);

    if (bmisset(&J->TFlags,mtransfArrayMasterJob))
      {
      mxml_t *SJE = NULL;
      mxml_t *RE = NULL;

      int ProcCount = 0;

      marray_t AJList;

      mjobconstraint_t AJConstraint;
      mjobconstraint_t VerboseConstraint;

      marray_t AJConstraintList;

      MUArrayListCreate(&AJList,sizeof(mtransjob_t *),10);
      MUArrayListCreate(&AJConstraintList,sizeof(mjobconstraint_t),10);

      memset(&AJConstraint,0,sizeof(AJConstraint));
      memset(&VerboseConstraint,0,sizeof(VerboseConstraint));

      /* get the list of sub-jobs of this array */

      AJConstraint.AType = mjaArrayMasterID;
      AJConstraint.ACmp = mcmpEQ;
      AJConstraint.ALong[0] = J->ArrayMasterID;

      MUArrayListAppend(&AJConstraintList,(void *)&AJConstraint);

      if (!bmisset(&CFlags,mcmVerbose))
        {
        VerboseConstraint.AType = mjaState;
        VerboseConstraint.ACmp = mcmpNE;
        VerboseConstraint.ALong[0] = mjsCompleted;

        MUArrayListAppend(&AJConstraintList,(void *)&VerboseConstraint);
        }

      MCacheReadJobs(&AJList,&AJConstraintList);

      /* get the proc count of the array jobs */

      MJobTransitionGetArrayProcCount(J,&AJList,&ProcCount);

      MXMLGetChild(JE,"req",NULL,&RE);

      MXMLSetAttr(RE,(char *)MReqAttr[mrqaTCReqMin],(void *)&ProcCount,mdfInt);

      if (bmisset(&CFlags,mcmXML))
        {
        /* create the array sub-job list element */

        MJobTransitionArraySubjobsToXML(J,&AJList,&SJE);

        MXMLAddE(JE,SJE);
        }

      MUArrayListFree(&AJList);
      MUArrayListFree(&AJConstraintList);
      } /* END if (bmisset(&J->TFlags,mtransfArrayMasterJob)) */

    /* NOTE:  non-xml client requests need some special handling */

    if (!bmisset(&CFlags,mcmXML))
      {
      /* it only wants the request's "active" partition */

      MXMLSetAttr(JE,(char *)MJobAttr[mjaPAL],(void *)J->Requirements[0]->Partition,mdfString);

      /* we need the rsv access for non-xml... */

      MJobTransitionAToString(J,mjaRsvAccess,tmpVal,MMAX_LINE,mfmNONE);
      MXMLSetAttr(JE,(char *)MJobAttr[mjaRsvAccess],(void *)tmpVal,mdfString);
      } /* END if (!bmisset(CFlags, mcmXML)) */

    MXMLAddE(DE,JE);
    } /* END for (rindex) */

  MCacheJobLock(FALSE,TRUE);
  MUArrayListFree(&JList);

  return(SUCCESS);
  }  /* END MUIJobCtlThreadSafe() */

 /*  END MUIJobCtlThreadSafe.c */
