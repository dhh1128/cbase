/* HEADER */

/**
 * @file MFS.c
 * 
 * Moab Fairshare
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-global.h"  
#include "moab-const.h"  
 
/* local prototypes */

int __MFSShowCred(enum MXMLOTypeEnum,void *,char *,mfs_t *,enum MFormatModeEnum,mbool_t,mbool_t,double,double *,mxml_t *,mstring_t *);
int __MFSTreeBuildFromXML(mxml_t **,mxml_t *,mpar_t *,mxml_t *);
int __MFSTreeBuildCredAccess(mxml_t *,mxml_t *);
mbool_t __MFSTreeNodeIsCred(mjob_t *,mfst_t *);
int __MFSTreeCheckCapUsage(mjob_t *,mpar_t *,mfst_t *,mstring_t *);


#define MFSTREE_TREESPACE 4

/* END local prototypes */

/**
 * Converts Moab's fairshare usage info into an XML format.
 *
 * @see MFSFromXML()
 *
 * @param F      (I)
 * @param EP     (O)
 * @param SAList (I,optional)
 */

int MFSToXML(

  mfs_t             *F,       /* I */
  mxml_t           **EP,      /* O */
  enum MFSAttrEnum  *SAList)  /* I (optional) */
 
  {
  const enum MFSAttrEnum DAList[] = {
    mfsaTarget, 
    mfsaCap,
    mfsaNONE };

  int  aindex;
 
  enum MFSAttrEnum *AList;
 
  char tmpString[MMAX_LINE];

  if (EP != NULL)
    *EP = NULL;
 
  if ((F == NULL) || (EP == NULL))
    {
    return(FAILURE);
    }
 
  if (SAList != NULL)
    AList = SAList;
  else
    AList = (enum MFSAttrEnum *)DAList;
 
  MXMLCreateE(EP,(char *)MXO[mxoxFS]);
 
  for (aindex = 0;AList[aindex] != mfsaNONE;aindex++)
    {
    if ((MFSAToString(F,AList[aindex],tmpString,0) == FAILURE) ||
        (tmpString[0] == '\0'))
      {
      continue;
      }
 
    MXMLSetAttr(*EP,(char *)MFSAttr[AList[aindex]],tmpString,mdfString);
    }  /* END for (aindex) */

  return(SUCCESS);
  }  /* END MFSToXML() */




/**
 * Report Moab fairshare configuration/policy information as XML.
 *
 * @param FSC (I)
 * @param FS (I)
 * @param EP (O)
 * @param SAList (I) [optional]
 */

int MFSCToXML(

  mfsc_t             *FSC,    /* I */
  mfs_t              *FS,     /* I */
  mxml_t            **EP,     /* O */
  enum MFSCAttrEnum  *SAList) /* I (optional) */

  {
  const enum MFSCAttrEnum DAList[] = {
    mfscaDecay,
    mfscaDefTarget,
    mfscaDepth,
    mfscaInterval,
    mfscaPolicy,
    mfscaUsageList,
    mfscaNONE };

  int  aindex;

  enum MFSCAttrEnum *AList;

  char tmpString[MMAX_LINE];

  if ((FSC == NULL) || (EP == NULL))
    {
    return(FAILURE);
    }

  if (SAList != NULL)
    AList = SAList;
  else
    AList = (enum MFSCAttrEnum *)DAList;

  MXMLCreateE(EP,(char *)MXO[mxoxFSC]);

  for (aindex = 0;AList[aindex] != mfscaNONE;aindex++)
    {
    if ((MFSCAToString(FSC,FS,AList[aindex],tmpString,0) == FAILURE) ||
        (tmpString[0] == '\0'))
      {
      continue;
      }

    MXMLSetAttr(*EP,(char *)MFSCAttr[AList[aindex]],tmpString,mdfString);
    }  /* END for (aindex) */

  return(SUCCESS);
  }  /* END MFSCToXML() */




/**
 * Convert XML to fairshare usage info.
 *
 * @see MFSToXML()
 *
 * @param F (I) [modified]
 * @param E (I)
 */

int MFSFromXML(
 
  mfs_t  *F,  /* I (modified) */
  mxml_t *E)  /* I */
 
  {
  int aindex;
  
  enum MFSAttrEnum saindex;
 
  if ((F == NULL) || (E == NULL))
    {
    return(FAILURE);
    }
 
  /* NOTE:  do not initialize.  may be overlaying data */
 
  for (aindex = 0;aindex < E->ACount;aindex++)
    {
    saindex = (enum MFSAttrEnum)MUGetIndex(E->AName[aindex],MFSAttr,FALSE,0);
 
    if (saindex == mfsaNONE)
      continue;
 
    MFSSetAttr(F,saindex,(void **)E->AVal[aindex],mdfString,mSet);
    }  /* END for (aindex) */
 
  return(SUCCESS);
  }  /* END MFSFromXML() */
 
 
 
 
/**
 * Set specified fairshare attribute.
 *
 * @param F (I) [modified]
 * @param AIndex (I)
 * @param Value (I)
 * @param Format (I)
 * @param Mode (I)
 */

int MFSSetAttr(
 
  mfs_t             *F,      /* I (modified) */
  enum MFSAttrEnum   AIndex, /* I */
  void             **Value,  /* I */
  int                Format, /* I */
  int                Mode)   /* I */
 
  { 
  if (F == NULL)
    {
    return(FAILURE);
    }

  switch (AIndex)
    {
    case mfsaTarget:

      MFSTargetFromString(F,(char *)Value,TRUE);
 
      break;

    case mfsaCap:

      MFSTargetFromString(F,(char *)Value,FALSE);

      break;

    case mfsaUsageList:

      /* NYI */

      return(FAILURE);

      /*NOTREACHED*/

      break;
 
    default:

      /* not handled */

      return(FAILURE);
   
      /*NOTREACHED*/
 
      break;
    }  /* switch (AIndex) */
 
  return(SUCCESS);
  }  /* MFSSetAttr() */





/**
 *
 *
 * @param F (I) [modified]
 * @param Buf (I)
 * @param IsTarget (I)
 */

int MFSTargetFromString(

  mfs_t   *F,        /* I (modified) */
  char    *Buf,      /* I */
  mbool_t  IsTarget) /* I */

  {
  double Target;
  enum   MFSTargetEnum Mode;

  char *tail;

  if ((F == NULL) || (Buf == NULL))
    {
    return(FAILURE);
    }

  /* FORMAT:  <TARGET>[<MOD>] */

  Target = strtod(Buf,&tail);
 
  if (tail != NULL)
    {
    if (*tail == MFSTargetTypeMod[mfstFloor])
      Mode = mfstFloor;
    else if (*tail == MFSTargetTypeMod[mfstCeiling])
      Mode = mfstCeiling;
    else if (*tail == MFSTargetTypeMod[mfstCapAbs])
      Mode = mfstCapAbs;
    else if (*tail == MFSTargetTypeMod[mfstCapRel])
      Mode = mfstCapRel;
    else
      Mode = mfstTarget;
    }
  else
    {
    Mode = mfstTarget;
    }

  if (IsTarget == TRUE)
    {
    if ((Mode != mfstCeiling) && (Mode != mfstFloor) && (Mode != mfstTarget))
      {
      /* invalid type */

      MDB(1,fFS) MLog("ERROR:    invalid type specified for FSTarget");

      return(FAILURE);
      }

    F->FSTargetMode = Mode;
    F->FSTarget     = Target;
    }
  else
    {
    if ((Mode != mfstCapAbs) && (Mode != mfstCapRel))
      {
      /* invalid type */

      MDB(3,fFS) MLog("INFO:     No type specified for FSCap, defaulting to Relative\n");

      Mode = mfstCapRel;
      }

    F->FSCapMode = Mode;
    F->FSCap     = Target;
    }

  return(SUCCESS);
  }  /* END MFSTargetFromString() */




/**
 * Report fairshare config/policy attribute as string.
 *
 * @see MFSAToString()
 *
 * @param FSC (I)
 * @param FS (I)
 * @param AIndex (I)
 * @param Buf (O) [minsize=MMAX_LINE]
 * @param Format (I)
 */

int MFSCAToString(

  mfsc_t *FSC,    /* I */
  mfs_t  *FS,     /* I */
  enum MFSCAttrEnum AIndex, /* I */
  char   *Buf,    /* O (minsize=MMAX_LINE) */
  int     Format) /* I */

  {
  int BufSize = MMAX_LINE;

  /* NOTE:  full FS info distributed amongst global FS object and FSC object */

  if ((FSC == NULL) || (Buf == NULL))
    {
    return(FAILURE);
    }

  Buf[0] = '\0';

  switch (AIndex)
    {
    case mfscaDecay:

      if (FSC->FSDecay != 1.0)
        {
        sprintf(Buf,"%.2f",
          FSC->FSDecay);
        }

      break;

    case mfscaDefTarget:

      if ((FS != NULL) && (FS->FSTarget > 0.0))
        {
        strcpy(Buf,MFSTargetToString(FS->FSTarget,FS->FSTargetMode));
        }

      break;

    case mfscaDepth:

      if (FSC->FSDepth > 0)
        {
        sprintf(Buf,"%d",
          FSC->FSDepth);
        }

      break;

    case mfscaInterval:

      if (FSC->FSInterval > 0)
        {
        sprintf(Buf,"%ld",
          FSC->FSInterval);
        }

      break;

    case mfscaPolicy:

      if (FSC->FSPolicy != mfspNONE)
        {
        sprintf(Buf,"%s",
          MFSPolicyType[FSC->FSPolicy]);
        }

      break;

    case mfscaUsageList:

      if (FS != NULL)
        {
        int fsindex;

        char *BPtr;
        int   BSpace;

        MUSNInit(&BPtr,&BSpace,Buf,BufSize);

        for (fsindex = 0;fsindex < FSC->FSDepth;fsindex++)
          {
          MUSNPrintF(&BPtr,&BSpace,"%s%.2f",
            (fsindex > 0) ? "," : "",
            FS->FSUsage[fsindex]);
          }    /* END for (fsindex) */
        }      /* END if (FS != NULL) */

      break;

    default:

      /* not supported */

      return(FAILURE);

      /*NOTREACHED*/

      break;
    }  /* END switch (AIndex) */

  return(SUCCESS);
  }  /* END MFSCAToString() */




/**
 * Report fairshare attribute as string.
 *
 * @see MFSCAToString()
 *
 * @param F (I)
 * @param AIndex (I)
 * @param Buf (O)
 * @param Format (I)
 */

int MFSAToString(
 
  mfs_t *F,      /* I */
  enum MFSAttrEnum AIndex, /* I */
  char  *Buf,    /* O (minsize=MMAX_LINE) */
  int    Format) /* I */
 
  {
  int BufSize = MMAX_LINE;

  if ((F == NULL) || (Buf == NULL))
    {
    return(FAILURE);
    }

  Buf[0] = '\0';
 
  switch (AIndex)
    {
    case mfsaTarget:

      if (F->FSTarget > 0.0) 
        strcpy(Buf,MFSTargetToString(F->FSTarget,F->FSTargetMode));

      break;

    case mfsaCap:

      if (F->FSCap > 0.0)
        strcpy(Buf,MFSTargetToString(F->FSCap,F->FSCapMode));

      break;

    case mfsaUsageList:

      {
      int fsindex;

      char *BPtr;
      int   BSpace;

      MUSNInit(&BPtr,&BSpace,Buf,BufSize);

      for (fsindex = 0;fsindex < MPar[0].FSC.FSDepth;fsindex++)
        {
        MUSNPrintF(&BPtr,&BSpace,"%s%.2f",
          (fsindex > 0) ? "," : "",
          F->FSUsage[fsindex]);
        }    /* END for (fsindex) */
      }      /* END BLOCK */

      break;

    default:

      /* not supported */

      return(FAILURE);

      /*NOTREACHED*/

      break;
    }  /* END switch (AIndex) */

  return(SUCCESS);
  }  /* END MFSAToString() */




#define MMIN_GFSCAPUSAGE 1000.0

/**
 * Verify job J can run without violating FS Cap.
 *
 * @param FSPtr (I)
 * @param J (I)
 * @param P (I)
 * @param OIPtr (O) object type which violates FSCap [optional] 
 */

int MFSCheckCap(

  mfs_t   *FSPtr,   /* I */
  mjob_t  *J,       /* I */
  mpar_t  *P,       /* I */
  int     *OIPtr)   /* O (optional) object type which violates FSCap */

  {
  enum MXMLOTypeEnum OList[] = { 
    mxoUser, 
    mxoGroup, 
    mxoAcct, 
    mxoClass, 
    mxoQOS, 
    mxoNONE };

  int oindex;

  double FSUsage;
  double FSReq;

  double GFSUsage;

  mfs_t *F;

  mfsc_t *FC = &MPar[0].FSC;

  double tmpD;

  mpar_t *GP = &MPar[0];

  if (OIPtr != NULL)
    *OIPtr = 0;

  switch (FC->FSPolicy)
    {
    case mfspDPES:

      MJobGetPE(J,P,&tmpD);
 
      FSReq = (long)tmpD;
 
      break;
 
    case mfspUPS:
    case mfspDPS:
    default:
 
      FSReq = J->TotalProcCount * J->SpecWCLimit[0];
 
      break;
    }  /* END switch (FC->FSPolicy) */

  GFSUsage = GP->F.FSUsage[0] + GP->F.FSFactor;

  if (FSPtr == NULL)
    {
    if ((J == NULL) || (P == NULL))
      {
      return(FAILURE);
      }

    for (oindex = 0;OList[oindex] != mxoNONE;oindex++)
      {
      F = NULL;
 
      switch (OList[oindex])
        {
        case mxoUser:
 
          if (J->Credential.U != NULL)
            F = &J->Credential.U->F;
 
          break;
 
        case mxoGroup:
 
          if (J->Credential.G != NULL)
            F = &J->Credential.G->F;
 
          break;
 
        case mxoAcct:

          if (J->Credential.A != NULL)
            F = &J->Credential.A->F;
 
          break;
 
        case mxoClass:
 
          if (J->Credential.C != NULL)
            F = &J->Credential.C->F;
 
          break;
 
        case mxoQOS:
 
          if (J->Credential.Q != NULL)
            F = &J->Credential.Q->F;
 
          break;

        default:

          /* NO-OP */
 
          break;
        }  /* END switch (OList[oindex]) */
 
      if (F == NULL)
        {
        /* no fairshare object defined for cred */

        continue;
        }

      FSUsage = 0.0;

      switch (F->FSCapMode)
        {
        case mfstCapAbs:

          FSUsage = F->FSUsage[0] + F->FSFactor + FSReq;
 
          break;
 
        case mfstCapRel:
 
          if (GFSUsage > MMIN_GFSCAPUSAGE)
            {
            FSUsage = ((F->FSUsage[0] + F->FSFactor + FSReq) / GFSUsage) * 100.0;
            
            /* FSUsage should be a percentage of total usage and should not exceed 100 % */

            if (FSUsage > 100) 
              FSUsage = 100;
            }
 
          break;
 
        default:

          /* NO-OP */
 
          break;
        }  /* END switch (F->FSCapMode) */
 
      if (FSUsage > F->FSCap)
        {
        if (OIPtr != NULL)
          *OIPtr = OList[oindex];

        return(FAILURE);
        }
      }    /* END for (oindex) */
    }      /* END if (FSPtr == NULL) */
  else
    {
    F = FSPtr;

    FSUsage = 0.0;

    switch (F->FSCapMode)
      {
      case mfstCapAbs:

        FSUsage = F->FSUsage[0] + F->FSFactor + FSReq;
 
        break;
 
      case mfstCapRel:
 
        if (GFSUsage > MMIN_GFSCAPUSAGE)
          {
          FSUsage = (F->FSUsage[0] + F->FSFactor + FSReq) /
                    GFSUsage * 100.0;
          }
 
        break;

      default:

        /* NO-OP */

        break;
      }  /* END switch (F->FSCapMode) */

    if (FSUsage > F->FSCap)
      {
      return(FAILURE);
      }
    }    /* END else (FSPtr == NULL) */

  return(SUCCESS);
  }  /* END MFSCheckCap() */


/**
 * Checks whether or not the current tree node is part of the job's credentials. 
 *
 * @param J (I)
 * @param TData (I)
 */

mbool_t __MFSTreeNodeIsCred(

  mjob_t *J,     /*I*/
  mfst_t *TData) /*I*/

  {
  mbool_t FoundCred = FALSE;

  /* When tree is created through xml and not FSTREE[] TData->O is populated,
   * which is used to determine if the job is violating a fscap. */

  if ((J == NULL) || (TData == NULL) || (TData->O == NULL))
    return(FALSE);

  if (strcasecmp(((mgcred_t *)TData->O)->Name,"root") == 0)
    return(TRUE);

  switch (TData->OType)
    {
    case mxoAcct:
     
      if (TData->O == J->Credential.A) 
        FoundCred = TRUE;

      break;

    case mxoClass:
      
      if (TData->O == J->Credential.C)
        FoundCred = TRUE;

      break;

    case mxoGroup:
     
      if (TData->O == J->Credential.G)
        FoundCred = TRUE;

      break;

    case mxoQOS:
     
      if (TData->O == J->Credential.Q)
        FoundCred = TRUE;

      break;

    case mxoUser:
     
      if (TData->O == J->Credential.U)
        FoundCred = TRUE;

      break;

    default:

      /* NO-OP */

      break;
    }

  return(FoundCred);
  }  /* END __MFSTreeNodeIsCred */



/**
 * Reports if the job is actually in violation of the cap usage.
 *
 * @param J (I)
 * @param P (I)
 * @param TData (I)
 * @param EMsg (O)
 */

int __MFSTreeCheckCapUsage(

  mjob_t    *J,     /*I*/
  mpar_t    *P,     /*I*/
  mfst_t    *TData, /*I*/
  mstring_t *EMsg)  /*O*/

  {
  mfs_t  *F  = TData->F;

  double  FSUsage  = 0.0;

  if (TData->F->FSCap == 0.0)
    return(SUCCESS); /* No FSCAP */

  switch (F->FSCapMode)
    {
    case mfstCapAbs:

      FSUsage = TData->F->FSUsage[0] + F->FSFactor;

      break;

    case mfstCapRel:

      /* Usage[N]Shares calulated in MFSTreeUpdateTNodeUsage */

      if (MPar[0].FSC.FSTreeShareNormalize == TRUE)
        FSUsage = (TData->UsageNShares / TData->NShares) * 100;
      else
        FSUsage = (TData->UsageShares / TData->Shares) * 100;

      break;

    default:

      /* NO-OP */

      break;
    }  /* END switch (F->FSCapMode) */

  if (FSUsage >= F->FSCap)
    {
    if (EMsg != NULL)
      {
      *EMsg = "job '";
      *EMsg += J->Name;
      *EMsg += "' violates fscap";
      }

    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END __MFSTreeCheckCapUsage */



/**
 * Verify job J can run without violating any FS cap the given partitions fs tree.
 *
 * Recurisely searches fs tree and returns failure if the job violates any fscap.
 *
 * @param Tree (I) Current node in the tree.
 * @param J (I)
 * @param P (I)
 * @param EMsg (O)
 */

int MFSTreeCheckCap(

  mxml_t    *Tree,  /*I*/
  mjob_t    *J,     /*I*/
  mpar_t    *P,     /*I*/
  mstring_t *EMsg)  /*O*/

  {
  mfst_t *TData = NULL; /* current tree node */

  int cindex = 0;
  int rc     = 0;

  if (Tree == NULL) /* Base Case */
    return(SUCCESS);

  TData = Tree->TData;

  /* all levels of the tree are assumed to be valid credentials (TData->O != NULL), 
   * the nodes that aren't won't be traversed. ex. FSTREE[] in moab.cfg doesn't populate
   * TData->O but the xml fstree does. */

  if (__MFSTreeNodeIsCred(J,TData) == TRUE)  /* check fscap and follow tree */
    {
    if (__MFSTreeCheckCapUsage(J,P,TData,EMsg) == FAILURE)
      {
      if (EMsg != NULL)
        MStringAppendF(EMsg,"%s:%s",MXO[TData->OType],Tree->Name);

      return(FAILURE);
      }

    /* recurse through rest of tree */

    for (cindex = 0;cindex < Tree->CCount;cindex++)
      {
      rc = MFSTreeCheckCap(Tree->C[cindex],J,P,EMsg);

      if (rc == FAILURE)
        {
        if (EMsg != NULL)
          {
          MStringAppendF(EMsg,"->%s:%s",MXO[TData->OType],Tree->Name);
          
          if (strcasecmp(Tree->Name,"root") == 0)
            MStringAppendF(EMsg,"(par:%s)",P->Name);
          }

        return(FAILURE);
        }
      }
    } /* END if (__MFSTreeNodeIsCred(J,TData) == TRUE) */

  return(SUCCESS);
  } /* END MFSTreeCheckCap */




/**
 *
 *
 * @param F (I)
 */

int MFSShutdown(

  mfsc_t *F)  /* I */

  {
  long NewFSInterval;

  if ((F == NULL) || (F->FSPolicy == mfspNONE))
    {
    return(SUCCESS);
    }

  /* if FS interval reached */
 
  NewFSInterval = MSched.Time - (MSched.Time % MAX(1,F->FSInterval));
 
  if (NewFSInterval != MSched.CurrentFSInterval)
    {
    if (MSched.CurrentFSInterval != 0)
      MFSUpdateData(F,MSched.CurrentFSInterval,FALSE,FALSE,TRUE);
 
    MSched.CurrentFSInterval = NewFSInterval;
 
    MDB(1,fFS) MLog("INFO:     FS interval rolled to interval %ld\n",
      MSched.CurrentFSInterval);
    }
  else
    {
    MFSUpdateData(F,MSched.CurrentFSInterval,TRUE,FALSE,FALSE);
    }

  return(SUCCESS);
  }  /* END MFSShutdown() */



/**
 *
 *
 * @param FSTarget (I)
 * @param FSMode (I)
 */

char *MFSTargetToString(
 
  double             FSTarget,  /* I */
  enum MFSTargetEnum FSMode)    /* I */
 
  {
  static char Line[MMAX_LINE];
 
  sprintf(Line,"%.2f%c",
    FSTarget,
    MFSTargetTypeMod[FSMode]);
 
  return(Line);
  }  /* END MFSTargetToString() */




/**
 *
 * @param FileName (I)
 * @param FSSlot (I)
 * @param DoXML (I)
 * @param pindex (I) [-1 = not set ]
 */

int MFSLoadDataFile(
 
  char   *FileName,  /* I */
  int     FSSlot,    /* I */
  mbool_t DoXML,     /* I */
  int     pindex)    /* I (-1 = not set ) */
 
  {
  int      count;
 
  char    *buf;
  char     Name[MMAX_NAME + 1];
  char     Type[MMAX_NAME + 1];
 
  double   Value;
  char    *ptr;
  char    *tmp;
 
  char    *head;
  char    *TokPtr;
 
  int      buflen;
 
  int      SC;

  void    *O;

  mfs_t   *F;

  enum MXMLOTypeEnum OIndex;

  int      rc;

  const char *FName = "MFSLoadDataFile";
 
  MDB(2,fFS) MLog("%s(%s,%d)\n", 
    FName,
    FileName,
    FSSlot);
 
  if ((buf = MFULoad(FileName,1,macmWrite,&count,NULL,&SC)) == NULL)
    {
    MDB(6,fFS) MLog("INFO:  cannot open FS data file '%s'\n",
      FileName);
 
    return(FAILURE);
    }
 
  buflen = strlen(buf);
  head   = buf;
 
  ptr = MUStrTok(buf,"\n",&TokPtr);
 
  while ((ptr = MUStrTok(head,"\n",&TokPtr)) != NULL)
    {
    head = ptr + strlen(ptr) + 1;
 
    if ((head - buf) >= buflen)
      {
      /* point head to end of buffer */
 
      head = &buf[buflen];
      }
 
    /* ignore comments */
 
    if ((tmp = strchr(ptr,'#')) != NULL)
      *tmp = '\0';
 
    MDB(6,fFS) MLog("INFO:     parsing FS data line '%s'\n",
      ptr); 
 
    /* load value */
 
    if ((DoXML == TRUE) && (ptr[0] != '<'))
      {
      /* only process XML */

      continue;
      }
    
    if ((DoXML == FALSE) && (ptr[0] == '<'))
      {
      /* ignore XML */

      continue;
      }

    if (DoXML == FALSE)
      {
      rc = sscanf(ptr,"%32s %32s %lf",
        Type,
        Name,
        &Value);

      if (rc != 3)
        {
        /* ignore line */

        continue;
        }

      if (!strcmp(Type,"TOTAL"))
        {
        OIndex = mxoSched;
        }
      else if ((OIndex = (enum MXMLOTypeEnum)MUGetIndex(Type,MXO,FALSE,mxoNONE)) == mxoNONE)
        {
        continue;
        }

      if ((MOGetObject(OIndex,Name,&O,mAdd) == FAILURE) ||
          (MOGetComponent(O,OIndex,(void **)&F,mxoxFS) == FAILURE))
        {
        continue;
        }

      /* update value */

      MDB(6,fFS) MLog("INFO:     %s '%s' FSUsage[%d]: %f\n",
        MXO[OIndex],
        (Name[0] != '\0') ? Name : "NONE",
        FSSlot,
        Value);
 
      if (pindex == -1)
        {
        F->FSUsage[FSSlot] = Value;
        }
      } /* END if (DoXML == FALSE) */
    else
      {
      mxml_t *Tree = NULL;

      /* Load the XML format fairshare data read from the file into a temporary 
         fairshare tree structure. Then call MFSTreeLoadFSData() to load the temporary fs tree
         into a slot in the fairshare tree for the partition specified by pindex. */ 

      if (MFSTreeLoadXML(ptr,&Tree,NULL) == SUCCESS)
        {
        MFSTreeLoadFSData(MPar[pindex].FSC.ShareTree,Tree,FSSlot,FALSE);

        MFSTreeFree(&Tree);
        }
      }
    }  /* END  while ((ptr = MUStrTok(head,"\n",&TokPtr)) != NULL) */
 
  free(buf);
 
  return(SUCCESS);
  }  /* END MFSLoadDataFile() */





/**
 * Calculate, rotate, and write FS status data.
 *
 * @param FC (I) [modified]
 * @param FSInterval (I) FS data interval index
 * @param ABM (I) [bitmap of enum MFSActionEnum]
 * @param Write (I) write out to file
 * @param Calc (I) calculate a new time slot
 * @param Rotate (I) rotate a new time slot
 */

int MFSUpdateData(

  mfsc_t    *FC,
  int        FSInterval,
  mbool_t    Write,
  mbool_t    Calc,
  mbool_t    Rotate)

  {
  int      pindex;
  int      fsindex;

  char     FSFile[MMAX_PAR + 1][MMAX_LINE];  /* need MMAX_PAR+1 for extra all-purpose file */
  char     DString[MMAX_LINE];

  char    *NameP;

  void    *O;
  void    *OP;

  FILE    *fsfp = NULL;
  FILE    *fspfp[MMAX_PAR] = {0};

  mbitmap_t FlagBM;

  mxml_t  *E;

  const enum MXMLOTypeEnum OList[] = { 
    mxoUser, 
    mxoGroup, 
    mxoAcct, 
    mxoQOS, 
    mxoClass, 
    mxoSched, 
    mxoNONE };
 
  int oindex;
  int findex;

  mfs_t *F;

  const char *FName = "MFSUpdateData";

  MDB(2,fFS) MLog("%s(FC,%d)\n",
    FName,
    FSInterval);

  MDB(6,fFS) MLog("INFO:     mode: %s:%s:%s\n",
    (Calc == TRUE)   ? "calc"  : "",
    (Rotate == TRUE) ? "rotate"  : "",
    (Write == TRUE)  ? "write" : "");

  findex = 0;

  FSFile[0][0] = '\0';

  mstring_t tmpBuffer(MMAX_LINE);

  for (pindex = 0;pindex < MMAX_PAR;pindex++)
    {
    if (MPar[pindex].Name[0] == '\1')
      continue;

    if (MPar[pindex].Name[0] == '\0')
      break;

    fspfp[pindex] = NULL;
    FSFile[pindex + 1][0] = '\0';
    }

  if ((Write == TRUE) || (Rotate == TRUE))
    {
    /* open FS data file */

    if (MStat.StatDir[strlen(MStat.StatDir) - 1] == '/')
      {
      sprintf(FSFile[findex],"%sFS.%d",
        MStat.StatDir,
        FSInterval);
      }
    else
      {
      sprintf(FSFile[findex],"%s/FS.%d",
        MStat.StatDir,
        FSInterval);
      }
      
    if ((fsfp = fopen(FSFile[findex],"w+")) == NULL)
      {
      MDB(0,fFS) MLog("WARNING:  cannot open FS data file '%s', errno: %d (%s)\n",
        FSFile[findex],
        errno,
        strerror(errno));

      return(FAILURE);
      }

    MULToDString(&MSched.Time,DString);

    /* write FS file header */

    fprintf(fsfp,"# FS Data File (Duration: %6ld seconds)  Starting: %s\n",
      FC->FSInterval,
      DString);

    for (pindex = 0;pindex < MMAX_PAR;pindex++)
      {
      if (MPar[pindex].Name[0] == '\1')
        continue;

      if (MPar[pindex].Name[0] == '\0')
        break;

      findex++;

      if ((MPar[pindex].FSC.ShareTree != NULL) &&
          (MPar[pindex].FSC.ShareTree->TData->CShares != 0))
        {
        if (MStat.StatDir[strlen(MStat.StatDir) - 1] == '/')
          {
          sprintf(FSFile[findex],"%sFS-%s.%d",
            MStat.StatDir,
            MPar[pindex].Name,
            FSInterval);
          }
        else
          {
          sprintf(FSFile[findex],"%s/FS-%s.%d",
            MStat.StatDir,
            MPar[pindex].Name,
            FSInterval);
          }
        }
      else
        {
        FSFile[findex][0] = '\0';
        }
      
      if ((FSFile[findex][0] != '\0') && (fspfp[pindex] = fopen(FSFile[findex],"w+")) == NULL)
        {
        MDB(0,fFS) MLog("WARNING:  cannot open FS data file '%s', errno: %d (%s)\n",
          FSFile[findex],
          errno,
          strerror(errno));

        fclose(fsfp);
        return(FAILURE);
        }
    
      /* write FS file header */

      if (FSFile[findex][0] != '\0')
        {
        MULToDString(&MSched.Time,DString);

        fprintf(fspfp[pindex],"# FS Data File (Duration: %6ld seconds)  Starting: %s\n",
          FC->FSInterval,
          DString);
        }
      } /* END for (pindex) */
    }  /* END if (bmisset(&ABM,mfsactWrite) || bmisset(&ABM,mfsactRotate)) */

  for (oindex = 0;OList[oindex] != mxoNONE;oindex++)
    {
    mhashiter_t HTI;

    void *OE;

    /* step through all objects */

    MOINITLOOP(&OP,OList[oindex],&OE,&HTI);

    MDB(4,fFS) MLog("INFO:     updating %s fairshare\n",
      MXO[OList[oindex]]);    

    while ((O = MOGetNextObject(&OP,OList[oindex],OE,&HTI,&NameP)) != NULL)
      {
      if (MOGetComponent(O,OList[oindex],(void **)&F,mxoxFS) == FAILURE)
        continue;

      MDB(7,fFS) MLog("INFO:     updating %s %s\n",
        MXO[OList[oindex]],
        (NameP != NULL) ? NameP : "NONE");

      if (((Write == TRUE) || (Rotate == TRUE)) && 
          (F->FSUsage[0] != 0.0))
        {
        fprintf(fsfp,"%-10s %15s %12.3f\n",
          MXO[OList[oindex]],
          ((NameP != NULL) && (NameP[0] != '\0')) ? NameP : "NONE",
          F->FSUsage[0]);
        }

      if (Rotate == TRUE)
        {
        for (fsindex = 1;fsindex < MMAX_FSDEPTH;fsindex++)
          F->FSUsage[MMAX_FSDEPTH - fsindex] = F->FSUsage[MMAX_FSDEPTH - fsindex - 1];

        F->FSUsage[0] = 0.0;
        }

      if ((Calc == TRUE) || (Rotate == TRUE))
        {
        F->FSFactor = MFSCalcFactor(FC,F->FSUsage);
        }
      }    /* END while ((O = MOGetNextObj()) != NULL) */
    }      /* END for (oindex) */

  bmset(&FlagBM,mfstaUsage);

  for (pindex = 0;pindex < MMAX_PAR;pindex++)
    {
    if (MPar[pindex].Name[0] == '\1')
      continue;

    if (MPar[pindex].Name[0] == '\0')
      break;

    if ((fspfp[pindex] == NULL) && 
         ((Rotate == TRUE) || (Write == TRUE)))
      continue;

    if (MPar[pindex].FSC.ShareTree != NULL)
      {
      if ((fspfp[pindex] != NULL) && 
           ((Rotate == TRUE) || (Write == TRUE)))
        {
        E = NULL;

        MFSTreeToXML(MPar[pindex].FSC.ShareTree,&E,&FlagBM);

        tmpBuffer = "";
        MXMLToMString(E,&tmpBuffer,NULL,TRUE);

        fprintf(fspfp[pindex],"%s\n",
          tmpBuffer.c_str());

        MXMLDestroyE(&E);
        }

      MFSTreeUpdateFSData(MPar[pindex].FSC.ShareTree,FC,Write,Calc,Rotate);
      }

    if (fspfp[pindex] != NULL)
      fclose(fspfp[pindex]);
    }

  if (fsfp != NULL)
    fclose(fsfp);

  return(SUCCESS);
  }  /* END MFSUpdateData() */





/**
 *
 *
 * @param FC (I) [modified]
 */

int MFSInitialize(

  mfsc_t *FC)  /* I (modified) */

  {
  long StartFSInterval;
  long CurrentFSInterval;

  char FileName[MMAX_LINE];
  
  int  interval;

  const char *FName = "MFSInitialize";

  MDB(3,fFS) MLog("%s(FC)\n",
    FName);

  if ((FC == NULL) || (FC->FSPolicy == mfspNONE))
    {
    return(SUCCESS);
    }

  CurrentFSInterval = MSched.Time - (MSched.Time % MAX(1,FC->FSInterval));
  StartFSInterval   = CurrentFSInterval - FC->FSDepth * FC->FSInterval;

  for (interval = 0;interval <= MMAX_FSDEPTH;interval++)
    {
    if (interval > FC->FSDepth)
      break;

    if (MPar[0].FSC.FSInitTime > StartFSInterval + interval * FC->FSInterval)
      {
      /* fs state initialized after specified interval */

      continue;
      }
    
    if (MStat.StatDir[strlen(MStat.StatDir) - 1] == '/')
      {
      sprintf(FileName,"%sFS.%ld",
        MStat.StatDir,
        StartFSInterval + interval * FC->FSInterval);
      }
    else
      {
      sprintf(FileName,"%s/FS.%ld",
        MStat.StatDir,
        StartFSInterval + interval * FC->FSInterval);
      }
 
    if (MFSLoadDataFile(FileName,FC->FSDepth - interval,FALSE,-1) == FAILURE)
      {
      MDB(3,fFS) MLog("WARNING:  cannot load FS file '%s' for slot %d\n",
        FileName,
        FC->FSDepth - interval);
      }
    }    /* END for (interval) */

  MFSUpdateData(FC,0,FALSE,TRUE,FALSE);

  return(SUCCESS);
  }  /* END MFSInitialize() */




/**
 *
 *
 * @param FC (I) [modified]
 */

int MFSInitializeFSTreeData(

  mfsc_t *FC)  /* I (modified) */

  {
  long StartFSInterval;
  long CurrentFSInterval;

  char FileName[MMAX_LINE];
  
  int  interval;
  int  pindex;

  const char *FName = "MFSInitialize";

  MDB(3,fFS) MLog("%s(FC)\n",
    FName);

  if ((FC == NULL) || (FC->FSPolicy == mfspNONE))
    {
    return(SUCCESS);
    }

  CurrentFSInterval = MSched.Time - (MSched.Time % MAX(1,FC->FSInterval));
  StartFSInterval   = CurrentFSInterval - FC->FSDepth * FC->FSInterval;

  for (interval = 0;interval <= MMAX_FSDEPTH;interval++)
    {
    if (interval > FC->FSDepth)
      break;

    if (MPar[0].FSC.FSInitTime > StartFSInterval + interval * FC->FSInterval)
      {
      /* fs state initialized after specified interval */

      continue;
      }
 
    if (MSched.FSTreeDepth == 0)
      {
      continue;
      }

    for (pindex = 0;pindex < MMAX_PAR;pindex++)
      {
      if (MPar[pindex].Name[0] == '\0')
        break;
 
      if (MPar[pindex].Name[0] == '\1')
        continue;

      if (MPar[pindex].FSC.ShareTree == NULL)
        continue;
     
      if (MStat.StatDir[strlen(MStat.StatDir) - 1] == '/')
        {
        sprintf(FileName,"%sFS-%s.%ld",
          MStat.StatDir,
          MPar[pindex].Name, 
          StartFSInterval + interval * FC->FSInterval);
        }
      else
        {
        sprintf(FileName,"%s/FS-%s.%ld",
          MStat.StatDir,
          MPar[pindex].Name,
          StartFSInterval + interval * FC->FSInterval);
        }
  
      if (MFSLoadDataFile(FileName,FC->FSDepth - interval,TRUE,pindex) == FAILURE)
        {
        MDB(3,fFS) MLog("WARNING:  cannot load FS file '%s' for slot %d\n",
        FileName,
        FC->FSDepth - interval);
        }
      }
    }    /* END for (interval) */

  return(SUCCESS);
  }  /* END MFSInitializeFSTreeData() */




/**
 *
 *
 * @param F       (I)
 * @param FSUsage (I)  FS usage history
 */

double MFSCalcFactor(

  mfsc_t *F,
  double  FSUsage[MMAX_FSDEPTH])

  {
  int    cindex;

  static double fsfactor;

  const char *FName = "MFSCalcFactor";

  MDB(7,fFS) MLog("%s(F,FSUsage)\n",
    FName);

  if ((F == NULL) || (FSUsage == NULL))
    {
    return(FAILURE);
    }

  fsfactor = 0.0;

  for (cindex = 1;cindex < F->FSDepth;cindex++)
    {
    fsfactor += (FSUsage[cindex] * pow(F->FSDecay,cindex));

    MDB(7,fFS) MLog("INFO:  FSUsage[%d]  %0.2f FSDecay %0.2f \n",
      cindex,
      FSUsage[cindex],
      pow(F->FSDecay,cindex));
    }  /* END for (cindex) */

  MDB(7,fFS) MLog("INFO:  FSFactor: %0.2f\n",
    fsfactor);

  return(fsfactor);
  }  /* END MFSCalcFactor() */





/**
 * Process priority and fairshare parameters.
 *
 * @param F (I) [modified]
 * @param PIndex (I)
 * @param IVal (I)
 * @param DVal (I)
 * @param SVal (I)
 * @param SArray (I)
 * @param Eval (I)
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 */

int MFSProcessOConfig(
 
  mfsc_t  *F,       /* I (modified) */
  enum MCfgParamEnum PIndex, /* I */
  int      IVal,    /* I */
  double   DVal,    /* I */
  char    *SVal,    /* I */
  char   **SArray,  /* I */
  mbool_t  Eval,    /* I */
  char    *EMsg)    /* O (optional,minsize=MMAX_LINE) */
 
  {
  if (F == NULL)
    {
    return(FAILURE);
    }

  if (Eval == TRUE)
    {
    if (PIndex != mcoFSPolicy)
      {
      return(SUCCESS);
      }
    }
 
  switch (PIndex)
    {
    case mcoFSDecay:
 
      F->FSDecay = DVal;
 
      break;

    case mcoFSDepth:

      F->FSDepth = MIN(IVal,MMAX_FSDEPTH);
 
      MDB(1,fCONFIG) MLog("INFO:     %s set to %d\n",
        MParam[PIndex],
        F->FSDepth);
 
      break;

    case mcoFSEnableCapPriority:

      F->EnableCapPriority = MUBoolFromString(SVal,FALSE);

      break;

    case mcoFSInterval:
 
      F->FSInterval = MUTimeFromString(SVal);
 
      break;

    case mcoFSIsAbsolute:

      F->TargetIsAbsolute = MUBoolFromString(SVal,FALSE);

      break;

    case mcoFSPolicy:

      {
      char *ptr;
      char *TokPtr = NULL;

      if (strchr(SVal,'%'))
        {
        ptr = MUStrTok(SVal,"%",&TokPtr);

        F->TargetIsAbsolute = TRUE;
        }
      else
        {
        ptr = SVal;
        }

      F->FSPolicySpecified = TRUE;

      if (MUBoolFromString(ptr,FALSE) == TRUE)
        {
        /* enable backlevel support */ 

        if (EMsg != NULL)
          {
          sprintf(EMsg,"deprecated 'boolean' use of parameter '%s'",
            MParam[PIndex]);
          }

        if (Eval != TRUE)
          F->FSPolicy = mfspDPS;   
        }
      else
        { 
        enum MFSPolicyEnum tFSP;

        if ((tFSP = (enum MFSPolicyEnum)MUGetIndexCI(
               ptr,
               MFSPolicyType,
               TRUE,
               mfspNONE)) != mfspNONE)
          {
          if (Eval != TRUE)
            F->FSPolicy = tFSP;
          }
        else
          {
          if (EMsg != NULL)
            {
            sprintf(EMsg,"invalid value specified for parameter '%s'",
              MParam[PIndex]);
            }
          }
        }
      }   /* END case mcoFSPolicy */

      break;

    case mcoFSTreeCapTiers:

      F->FSTreeCapTiers = MUBoolFromString(SVal,FALSE);

      break;

    case mcoFSTreeIsProportional:

      F->FSTreeIsProportional = MUBoolFromString(SVal,FALSE);

      break;

    case mcoFSTreeShareNormalize:

      F->FSTreeShareNormalize = MUBoolFromString(SVal,FALSE);

      break;

    case mcoFSTreeAcctShallowSearch:

      F->FSTreeAcctShallowSearch = MUBoolFromString(SVal,FALSE);

      break;

    case mcoFSTreeTierMultiplier:

      if (SVal != NULL)
        F->FSTreeTierMultiplier = strtod(SVal,NULL);
      else
        F->FSTreeTierMultiplier = DVal;

      break;

    case mcoJobAttrPrioF:

      {
      char *ptr;

      char IName[MMAX_NAME];
      char Value[MMAX_NAME];

      long Prio;

      int  aindex;

      /* FORMAT:  STATE[Running]=100  STATE[Suspended]=1000  ATTR[PREEMPTEE]=200 GRES[matlab]=200 */

      for (aindex = 0;SArray[aindex] != NULL;aindex++)
        {
        if (SArray[aindex][0] == '\0')
          break;

        ptr = SArray[aindex];

        IName[0] = '\0';

        while (MCfgGetVal(&ptr,"STATE",IName,NULL,Value,sizeof(Value),NULL) == SUCCESS)
          {
          Prio = strtol(Value,NULL,10);

          MJPrioFAdd(&MSched.JPrioF,mjpatState,IName,Prio);
 
          IName[0] = '\0';
          }  /* END while (MCfgGetVal() == SUCCESS) */

        ptr = SArray[aindex];

        IName[0] = '\0';

        while (MCfgGetVal(&ptr,"ATTR",IName,NULL,Value,sizeof(Value),NULL) == SUCCESS)
          {
          Prio = strtol(Value,NULL,10);

          MJPrioFAdd(&MSched.JPrioF,mjpatGAttr,IName,Prio);

          IName[0] = '\0';
          }  /* END while (MCfgGetVal() == SUCCESS) */

        ptr = SArray[aindex];

        IName[0] = '\0';

        while (MCfgGetVal(&ptr,"GRES",IName,NULL,Value,sizeof(Value),NULL) == SUCCESS)
          {
          Prio = strtol(Value,NULL,10);

          MJPrioFAdd(&MSched.JPrioF,mjpatGRes,IName,Prio);

          IName[0] = '\0';
          }  /* END while (MCfgGetVal() == SUCCESS) */
        }    /* END for (aindex) */
      }      /* END BLOCK (case case mcoJobAttrPrioF) */

      break;

    case mcoServWeight:
    case mcoTargWeight:
    case mcoCredWeight:
    case mcoAttrWeight:
    case mcoFSWeight:
    case mcoResWeight:
    case mcoUsageWeight:
 
      {
      long  tmpL;
      char *tail;
 
      tmpL = strtol(SVal,&tail,10);
 
      if (*tail == '%')
        {
        F->PCW[PIndex - mcoServWeight + 1] = 10;
        F->PCP[PIndex - mcoServWeight + 1] = tmpL;
        
        MDB(8,fSTRUCT) MLog("INFO:     F->PCW[%d] = %ld (tmpL = %ld)\n",
          PIndex - mcoServWeight + 1,
          F->PCW[PIndex - mcoServWeight + 1],
          tmpL);

        MDB(8,fSTRUCT) MLog("INFO:     F->PCP[%d] = %ld (tmpL = %ld)\n",
          PIndex - mcoServWeight + 1,
          F->PCP[PIndex - mcoServWeight + 1],
          tmpL);
        }
      else
        {
        F->PCW[PIndex - mcoServWeight + 1] = MAX(MCONST_PRIO_NOTSET + 1,tmpL);

        MDB(8,fSTRUCT) MLog("INFO:     F->PCW[%d] = %ld (tmpL = %ld)\n",
          PIndex - mcoServWeight + 1,
          F->PCW[PIndex - mcoServWeight + 1],
          tmpL);
        }
      }    /* END BLOCK */
 
      break;

    case mcoSQTWeight:
    case mcoSXFWeight:
    case mcoSDeadlineWeight:
    case mcoSSPVWeight:
    case mcoSUPrioWeight:
    case mcoSStartCountWeight:
    case mcoSBPWeight:
    case mcoTQTWeight:
    case mcoTXFWeight:
    case mcoCUWeight:
    case mcoCGWeight:
    case mcoCAWeight:
    case mcoCQWeight:
    case mcoCCWeight:
    case mcoFUWeight:
    case mcoFGWeight:
    case mcoFAWeight:
    case mcoFQWeight:
    case mcoFCWeight:
    case mcoGFUWeight:
    case mcoGFGWeight:
    case mcoGFAWeight:
    case mcoFUWCAWeight:
    case mcoFJPUWeight:
    case mcoFPPUWeight:
    case mcoFPSPUWeight:
    case mcoAJobAttrWeight:
    case mcoAJobGResWeight:
    case mcoAJobIDWeight:
    case mcoAJobNameWeight:
    case mcoAJobStateWeight:
    case mcoRNodeWeight:
    case mcoRProcWeight:
    case mcoRMemWeight:
    case mcoRSwapWeight:
    case mcoRDiskWeight:
    case mcoRPSWeight:
    case mcoRPEWeight:
    case mcoRWallTimeWeight:
    case mcoUConsWeight:
    case mcoURemWeight:
    case mcoUPerCWeight:
    case mcoUExeTimeWeight:
 
      {
      long tmpL = IVal;

      F->PSW[PIndex - mcoSQTWeight + 1] = MAX(MCONST_PRIO_NOTSET + 1,tmpL);

      MDB(8,fSTRUCT) MLog("INFO:     F->PSW[%d] = %ld (IVal = %ld)\n",
        PIndex - mcoSQTWeight + 1,
        F->PSW[PIndex - mcoSQTWeight + 1],
        tmpL);

      MPar[0].FSC.PSCIsActive[PIndex - mcoSQTWeight + 1] = TRUE; /* set this flag on the global FSC */
      }

      break;
 
    case mcoServCap:
    case mcoTargCap:
    case mcoCredCap:
    case mcoAttrCap:
    case mcoFSCap:
    case mcoResCap:
    case mcoUsageCap: 
 
      F->PCC[PIndex - mcoServCap + 1] = (long)IVal;
      
      MDB(8,fSTRUCT) MLog("INFO:     F->PCC[%d] = %ld (IVal = %ld)\n",
        PIndex - mcoServCap + 1,
        F->PCC[PIndex - mcoServCap + 1],
        (long)IVal);
 
      break;
 
    case mcoSQTCap:
    case mcoSXFCap:
    case mcoSDeadlineCap:
    case mcoSSPVCap:
    case mcoSUPrioCap:
    case mcoSStartCountCap:
    case mcoSBPCap:
    case mcoTQTCap:
    case mcoTXFCap:
    case mcoCUCap:
    case mcoCGCap:
    case mcoCACap:
    case mcoCQCap:
    case mcoCCCap:
    case mcoFUCap:
    case mcoFGCap:
    case mcoFACap:
    case mcoFQCap:
    case mcoFCCap:
    case mcoGFUCap:
    case mcoGFGCap:
    case mcoGFACap:
    case mcoFUWCACap:
    case mcoFJPUCap:
    case mcoFPPUCap:
    case mcoFPSPUCap:
    case mcoAJobAttrCap:
    case mcoAJobGResCap:
    case mcoAJobIDCap:
    case mcoAJobNameCap:
    case mcoAJobStateCap:
    case mcoRNodeCap:
    case mcoRProcCap:
    case mcoRMemCap:
    case mcoRSwapCap:
    case mcoRDiskCap:
    case mcoRPSCap:
    case mcoRPECap:
    case mcoRWallTimeCap:
    case mcoUConsCap:
    case mcoURemCap:
    case mcoUPerCCap:
    case mcoUExeTimeCap:
 
      F->PSC[PIndex - mcoSQTCap + 1] = (long)IVal;

      MPar[0].FSC.PSCIsActive[PIndex - mcoSQTCap + 1] = TRUE; /* set this flag on the global FSC */

      MDB(8,fSTRUCT) MLog("INFO:     F->PSC[%d] = %ld (IVal = %ld)\n",
        PIndex - mcoSQTCap + 1,
        F->PSC[PIndex - mcoSQTCap + 1],
        (long)IVal);

      break;
 
    case mcoXFMinWCLimit:
 
      F->XFMinWCLimit = MUTimeFromString(SVal); 
 
      break;

    default:

      /* NO-OP */

      break;
    }  /* END switch (PIndex) */

  return(SUCCESS);
  }  /* END MFSProcessOConfig() */





/**
 * Prints fairshare stats for credentials.
 *
 * Adds a * to the credential if _____, prints % used
 * Target and for each fairshare interval.
 *
 * @param OType (I)
 * @param O (I)
 * @param OName (I)
 * @param F (I)
 * @param DFormat (I)
 * @param Verbose (I)
 * @param ShowFSCap (I) Show FSCap stats or not
 * @param RFSFactor (I) relative FS factor [optional]
 * @param RFSUsage (I) relative usage [optional]
 * @param RE (O)
 * @param String (O) 
 */

int __MFSShowCred(

  enum MXMLOTypeEnum    OType,
  void                 *O,
  char                 *OName,
  mfs_t                *F,
  enum MFormatModeEnum  DFormat,
  mbool_t               Verbose,
  mbool_t               ShowFSCap,
  double                RFSFactor,
  double               *RFSUsage,
  mxml_t               *RE,
  mstring_t            *String)

  {
  mfs_t *DF;            /* Default scheduler fairshare */

  mxml_t *E = NULL;     /* set to avoid compiler warnings */

  double             FSTarget;
  enum MFSTargetEnum FSMode;
  double             FSPercent = 0.0;

  double             FSCap     = 0.0;
  enum MFSTargetEnum FSCapMode = mfstNONE;
 
  int                fsindex;

  double            *FSUsage;
  double             FSFactor;

  static mpar_t *GP = &MPar[0];

  static char tmpName[MMAX_NAME];
  static char tmpString[MMAX_NAME];
  static char tmpFSCap[MMAX_NAME];

  mbool_t IsRelative;

  DF = NULL;

  if (RFSUsage != NULL)
    {
    IsRelative = TRUE;

    FSUsage  = RFSUsage;
    FSFactor = RFSFactor;
    }
  else
    {
    IsRelative = FALSE;

    FSUsage  = GP->F.FSUsage;
    FSFactor = GP->F.FSFactor;
    }

  if (DFormat == mfmXML)
    {
    if (RE == NULL)
      {
      return(FAILURE);
      }

    E = RE;
    }
  else
    {
    if (String == NULL)
      {
      return(FAILURE);
      }

    /* NOTE:  do not initialize OBuf */
    }

  /* System default fairshare */

  switch (OType)
    {
    case mxoUser:

      if (MSched.DefaultU != NULL)
        DF = &MSched.DefaultU->F;

      break;

    case mxoGroup:

      if (MSched.DefaultG != NULL)
        DF = &MSched.DefaultG->F;

      break;

    case mxoAcct:

      if (MSched.DefaultA != NULL)
        DF = &MSched.DefaultA->F;

      break;

    case mxoClass:

      if (MSched.DefaultC != NULL)
        DF = &MSched.DefaultC->F;

      break;

    case mxoQOS:

      if (MSched.DefaultQ != NULL)
        DF = &MSched.DefaultQ->F;

      break;

    default:

      /* not supported */

      break;
    }  /* END switch (OType) */

  if ((OName == NULL) ||
      (OName[0] == '\0') ||
       !strcmp(OName,ALL))
    {
    /* invalid name specified */

    return(FAILURE);
    }

  /* Grab FSTarget values */

  if (F->FSTarget > 0.0)
    {
    FSTarget = F->FSTarget;
    FSMode   = F->FSTargetMode;
    }
  else if (DF != NULL)
    {
    FSTarget = DF->FSTarget;
    FSMode   = DF->FSTargetMode;
    }
  else
    {
    FSTarget = 0.0;
    FSMode   = mfstNONE;
    }

  /* Grab FSCap values */
  if (ShowFSCap == TRUE)
    {
    if (F->FSCap > 0.0)
      {
      FSCap     = F->FSCap;
      FSCapMode = F->FSCapMode;
      }
    else if (DF != NULL)
      {
      FSCap     = DF->FSCap;
      FSCapMode = DF->FSCapMode;
      }
    else
      {
      FSCap     = 0.0;
      FSCapMode = mfstNONE;
      }
    }

  if ((FSTarget == 0.0) &&
      (F->FSFactor == 0.0) &&
      (F->FSUsage[0] == 0.0) &&
      (Verbose == FALSE))
    {
    /* no information to report */

    return(FAILURE);
    }

  switch (DFormat)
    {
    case mfmXML:

      {
      mbitmap_t BM;

      mxml_t *CE = NULL;
      mxml_t *FSE = NULL;

      enum MFSAttrEnum FSAList[] = {
        mfsaCap,
        mfsaTarget,
        mfsaUsageList,
        mfsaNONE };

      enum MCredAttrEnum CAList[] = {
        mcaID,
        mcaNONE };

      enum MXMLOTypeEnum CCList[] = {
        mxoNONE };

      bmset(&BM,mcmVerbose);

      MCOToXML(
        O,
        OType,
        &CE,
        (void *)CAList,
        CCList,
        NULL,
        TRUE,
        &BM);

      MXMLAddE(E,CE);

      MFSToXML(F,&FSE,FSAList);

      MXMLAddE(CE,FSE);
      }  /* END BLOCK */

      break;

    case mfmHuman:
    default:

      strcpy(tmpName,OName);

      if ((FSFactor + FSUsage[0]) > 0.0)
        {
        FSPercent = (F->FSFactor + F->FSUsage[0]) /
                    (FSFactor + FSUsage[0]) * 100.0;

        if ((((FSMode == mfstFloor)   && (FSPercent < FSTarget)) ||
             ((FSMode == mfstCeiling) && (FSPercent > FSTarget)) ||
             ((FSMode == mfstTarget)  && (fabs(FSPercent - FSTarget) > 5.0))) &&
              (FSTarget > 0.0))
          {
          strcat(tmpName,"*");
          }
        }
      else
        {
        FSPercent = 0.0;
        }

      if ((FSTarget > 0.0) && (IsRelative == FALSE))
        {
        sprintf(tmpString,"%7.7s",
          MFSTargetToString(FSTarget,FSMode));
        }
      else
        {
        strcpy(tmpString,"-------");
        }

      if (IsRelative == TRUE)
        {
        MStringAppendF(String,"  %-12s %7.2f %7s",
          tmpName,
          FSPercent,
          tmpString);
        }
      else
        {
        MStringAppendF(String,"%-14s %7.2f %7s",
          tmpName,
          FSPercent,
          tmpString);
        }

      /* FSCap stats */

      if (ShowFSCap == TRUE)
        {
        if (FSCap > 0.0)
          {
          sprintf(tmpFSCap,"%7.7s",
            MFSTargetToString(FSCap,FSCapMode)); /* Format: fscap[fscapmode]  ex. 50.0^ */
          }
        else
          {
          strcpy(tmpFSCap,"-------");
          }

        /* Print FSCap Targets.  Format: FSCap */

        MStringAppendF(String," %7s",tmpFSCap);
        }

      for (fsindex = 0;fsindex < GP->FSC.FSDepth;fsindex++)
        {
        if (FSUsage[fsindex] > 0.0)
          {
          if (F->FSUsage[fsindex] > 0.0)
            {
            MStringAppendF(String," %7.2f",
              F->FSUsage[fsindex] / FSUsage[fsindex] * 100.0);
            }
          else
            {
            MStringAppendF(String," -------");
            }
          }
        }    /* END for (fsindex) */

      MStringAppendF(String,"\n");

      break;
    }  /* END switch (DFormat) */

  MDB(6,fFS) MLog("INFO:     %s '%s'  FSFactor: %8.2f  FSUsage[0]: %8.2f  FSPercent: %8.2f\n",
    MXO[OType],
    OName,
    F->FSFactor,
    F->FSUsage[0],
    FSPercent);

  return(SUCCESS);
  }  /* END __MFSShowCred() */





/**
 * Report fairshare status (mdiag -f).
 *
 * @param Auth (I) (FIXME: not used but should be)
 * @param String (O)
 * @param OType (I)
 * @param P (I) [optional]
 * @param Flags (I) [bitmap of enum MClientModeEnum - mcmRelative = Hierarchical]
 * @param DFormat (I)
 * @param Verbose (I)
 */

int MFSShow(

  char                 *Auth,
  mstring_t            *String,
  enum MXMLOTypeEnum    OType,
  mpar_t               *P,
  mbitmap_t            *Flags,
  enum MFormatModeEnum  DFormat,
  mbool_t               Verbose)

  {
  mstring_t JobFlags(MMAX_LINE);

  static mbool_t ShowFSCap = MBNOTSET;

  int   fsindex;

  const enum MXMLOTypeEnum OList[] = { 
    mxoUser, 
    mxoGroup, 
    mxoAcct, 
    mxoQOS, 
    mxoClass, 
    mxoNONE };

  int   oindex;

  mfs_t  *F;
  mfsc_t *FC;

  void   *OE;
  void   *O;
  void   *OP;

  char   *NameP;
  char    TString[MMAX_LINE];

  mpar_t *GP = NULL;

  mxml_t *E = NULL;

  mhashiter_t HTI;

  const char *FName = "MFSShow";

  MDB(3,fFS) MLog("%s(String,%s,Flags,%s,%s)\n",
    FName,
    MXO[OType],
    MFormatMode[DFormat],
    MBool[Verbose]);

  if (String == NULL)
    {
    return(FAILURE);
    }

  GP = &MPar[0];

  FC = &GP->FSC;

  *String = "";

  switch (DFormat)
    {
    case mfmXML:

      {
      /* report system level information */

      mxml_t *SE   = NULL;
      mxml_t *FSCE = NULL;

      MXMLCreateE(&E,(char *)MSON[msonData]);

      MXMLCreateE(&SE,(char *)MXO[mxoSched]);

      MXMLAddE(E,SE);

      MFSCToXML(FC,&GP->F,&FSCE,NULL);

      MXMLAddE(SE,FSCE);
      }  /* END BLOCK */

      break;

    case mfmHuman:
    default:

      /* determin if fscap has been speicified for any cred.  only check once */

      if (ShowFSCap == MBNOTSET)
        {
        ShowFSCap = FALSE;  /* trigger so only checks once */

        for (oindex = 0;OList[oindex] != mxoNONE;oindex++)
          {
          if ((OType != mxoNONE) && (OType != OList[oindex]))
            continue;

          MOINITLOOP(&OP,OList[oindex],&OE,&HTI);

          while ((O = MOGetNextObject(&OP,OList[oindex],OE,&HTI,&NameP)) != NULL)
            {
            if(MOGetComponent(O,OList[oindex],(void **)&F,mxoxFS) == FAILURE)
                continue;

            if (F->FSCap > 0.0)
              ShowFSCap = TRUE;
            }
          } /* END for(oindex */
        } /* END if(MBNOTSET == ShowFSCap) */

      /* display global config */

      MStringAppendF(String,"FairShare Information\n\n");

      MULToTString(FC->FSInterval,TString);

      MStringAppendF(String,"Depth: %d intervals   Interval Length: %s   Decay Rate: %.2f\n\n",
        FC->FSDepth,
        TString,
        FC->FSDecay);

      MStringAppendF(String,"FS Policy: %s%s %s\n",
        (FC->FSPolicy != mfspNONE) ? MFSPolicyType[FC->FSPolicy] : "---",
        (FC->TargetIsAbsolute == TRUE) ? "%" : "",
        (FC->EnableCapPriority == TRUE) ? "(Cap Priority Enabled)" : "");

      MStringAppendF(String,"System FS Settings:  Target Usage: %s\n\n",
        MFSTargetToString(GP->F.FSTarget,GP->F.FSTargetMode));

      /* display header */

      MStringAppendF(String,"%-14s %7s %7s",
        "FSInterval",
        "  %   ",
        "Target");
      
      if (ShowFSCap == TRUE)
        {
        MStringAppendF(String," %7s","FSCap"); /* FSCap Column */
        }

      for (fsindex = 0;fsindex < FC->FSDepth;fsindex++)
        {
        if (GP->F.FSUsage[fsindex] > 0.0)
          {
          MStringAppendF(String," %7d",
            fsindex);
          }
        }    /* END for (fsindex) */

      MStringAppendF(String,"\n");

      /* display weight line */

      MStringAppendF(String,"%-14s %7s %7s",
        "FSWeight",
        "-------",
        "-------");

      if (ShowFSCap == TRUE)
        {
        MStringAppendF(String," %7s","-------"); /* FSCap Column */
        }

      for (fsindex = 0;fsindex < FC->FSDepth;fsindex++)
        {
        if (GP->F.FSUsage[fsindex] > 0.0)
          {
          MStringAppendF(String," %7.4f",
            pow(FC->FSDecay,fsindex));
          }
        }  /* END for (fsindex) */

      MStringAppendF(String,"\n");

      /* display total usage line */

      MStringAppendF(String,"%-14s %7.2f %7s",
        "TotalUsage",
        100.0,
        "-------");
      
      if (ShowFSCap == TRUE)
        {
        MStringAppendF(String," %7s","-------"); /* FSCap Column */
        }

      for (fsindex = 0;fsindex < FC->FSDepth;fsindex++)
        {
        if (GP->F.FSUsage[fsindex] > 0.0)
          {
          MStringAppendF(String," %7.1f",
            GP->F.FSUsage[fsindex] / (float)MCONST_HOURLEN);
          }
        }    /* END for (fsindex) */

      MStringAppendF(String,"\n");

      MDB(6,fFS) MLog("INFO:     Total FSFactor: %8.2f  FSUsage[0]: %8.2f\n",
        GP->F.FSFactor,
        GP->F.FSUsage[0]);
      
      break;
    }  /* END switch (DFormat) */

  /* display all credentials */

  for (oindex = 0;OList[oindex] != mxoNONE;oindex++)
    {
    mbool_t HeaderDisplayed = FALSE;

    if ((OType != mxoNONE) && (OType != OList[oindex]))
      continue;

    MOINITLOOP(&OP,OList[oindex],&OE,&HTI);

    MDB(4,fFS) MLog("INFO:     updating %s fairshare\n",
      MXO[OList[oindex]]);

    while ((O = MOGetNextObject(&OP,OList[oindex],OE,&HTI,&NameP)) != NULL)
      {
      if (MOGetComponent(O,OList[oindex],(void **)&F,mxoxFS) == FAILURE)
        continue;

      if (DFormat == mfmXML)
        {
        __MFSShowCred(
          OList[oindex],
          O,
          NameP,
          F,
          DFormat,
          Verbose,
          ShowFSCap,
          0.0,
          NULL,
          E,    
          NULL);
        }
      else
        {
        if (HeaderDisplayed == FALSE)
          {
          MStringAppendF(String,"\n%s\n-------------\n",
            MXOC[OList[oindex]]);

          HeaderDisplayed = TRUE;
          }

        __MFSShowCred(
          OList[oindex],
          O,
          NameP,
          F,
          DFormat,
          Verbose,
          ShowFSCap,
          0.0,
          NULL,
          NULL,
          String);
        }

      if (bmisset(Flags,mcmRelative) && (OList[oindex] != mxoUser))
        {
        mgcred_t *U;
        mhashiter_t HIter;

        /* display hierarchical FS info for users belonging to object */

        MUHTIterInit(&HIter);
        while (MUHTIterate(&MUserHT,NULL,(void **)&U,NULL,&HIter) == SUCCESS)
          {
          switch (OList[oindex]) 
            {
            case mxoGroup:

              if (MULLCheck(U->F.GAL,NameP,NULL) == FAILURE)
                continue;

              break;

            case mxoAcct:

              if (MULLCheck(U->F.AAL,NameP,NULL) == FAILURE)
                continue;

              break;

            default:

              /* NOT SUPPORTED */

              continue;

              /*NOTREACHED*/

              break;
            }  /* END switch (OType) */

          /* display relative FS user */

          if (DFormat == mfmXML)
            {
            __MFSShowCred(
              OList[oindex],
              O,
              U->Name,
              &U->F,
              DFormat,
              Verbose,
              ShowFSCap,
              F->FSFactor,
              F->FSUsage,
              E,
              NULL);
            }
          else
            {
            __MFSShowCred(
              OList[oindex],
              O,
              U->Name,
              &U->F,
              DFormat,
              Verbose,
              ShowFSCap,
              F->FSFactor,
              F->FSUsage,
              NULL,
              String);
            }
          } 
        }    /* END if (bmisset(Flags,mcmRelative) && ...) */
      }      /* END while ((O = MOGetNextObject()) != NULL) */
    }        /* END for (oindex) */

  switch (DFormat)
    {
    case mfmXML:
      {
      mbitmap_t FlagBM;

      mxml_t *FE     = NULL;

      bmset(&FlagBM,mfstaUsage);
      bmset(&FlagBM,mfstaFSFactor);
      bmset(&FlagBM,mfstaLimits);
      bmset(&FlagBM,mfstaShare);
      bmset(&FlagBM,mfstaType);
      bmset(&FlagBM,mfstaPartition);
      bmset(&FlagBM,mfstaTotalShare);
      bmset(&FlagBM,mfstaQList);

      if (Verbose == TRUE)
        {
        bmset(&FlagBM,mfstaFSCap);
        bmset(&FlagBM,mfstaComment);
        }

      if ((P != NULL) && 
          (P->FSC.ShareTree != NULL) &&
          (P->FSC.ShareTree->TData->CShares != 0))
        {
        MFSTreeToXML(P->FSC.ShareTree,&FE,&FlagBM);

        MXMLAddE(E,FE);
        }
      else
        {
        int pindex;
 
        for (pindex = 0;pindex < MMAX_PAR;pindex++)
          {
          if (MPar[pindex].Name[0] == '\1')
            continue;

          if (MPar[pindex].Name[0] == '\0')
            break;

          if (MPar[pindex].FSC.ShareTree != NULL)
            {
            FE = NULL;
    
            MFSTreeToXML(MPar[pindex].FSC.ShareTree,&FE,&FlagBM);
     
            MXMLAddE(E,FE);
            }
          }
        }
      
      MXMLToMString(E,String,NULL,TRUE);

      MXMLDestroyE(&E);
      }
      break;

    case mfmHuman:
    default:

      if ((P != NULL) && 
          (P->FSC.ShareTree != NULL) &&
          (P->FSC.ShareTree->TData->CShares != 0))
        {
        MStringAppendF(String,"\n\nShare Tree Overview for partition '%s'\n",P->Name);

        MFSShowTreeWithFlags(P->FSC.ShareTree,Flags,String,Verbose);
        }
      else
        {
        int pindex;

        for (pindex = 0;pindex < MMAX_PAR;pindex++)
          {
          if (MPar[pindex].Name[0] == '\1')
            continue;

          if (MPar[pindex].Name[0] == '\0')
            break;

          if (MPar[pindex].FSC.ShareTree != NULL)
            {
            MStringAppendF(String,"\n\nShare Tree Overview for partition '%s'\n",MPar[pindex].Name);

            MFSShowTreeWithFlags(MPar[pindex].FSC.ShareTree,Flags,String,Verbose);

            MStringAppendF(String,"\n End of Share Tree for partition '%s\n",MPar[pindex].Name);
            }
          }    /* END for (pindex) */
        }      /* END if (Verbose == TRUE) */

      if ((FC->FSPolicy == mfspNONE) && (FC->ShareTree != NULL))
        {
        MStringAppendF(String,"WARNING:  share tree configured but FSPOLICY not set (tree usage info will not be updated)\n");
        }

      break;
    }  /* END switch (DFormat) */
 
  return(SUCCESS);
  }  /* END MFSShow() */







/**
 * Load FS Tree config XML
 *
 * @see MIDLoadCred() - parent - load config via identity manager
 * @see MFSTreeProcessConfig() - child - process individual config lines
 *
 * @param CfgBuf (I) [optional]
 */

int MFSTreeLoadConfig(

  char *CfgBuf)  /* I (optional) */

  {
  char *BufTok;

  char  IndexName[MMAX_NAME];

  char *ValArray[MMAX_CONFIGATTR];

  int   index;
  int   pindex;

  char *ttokptr=NULL;


  if (MSched.FSTreeDepth > 0)
    {
    /* This routine assumes that MPar[x].FSC fairshare tree pointers are NULL */

    return(SUCCESS);
    }

  mstring_t Value(MMAX_BUFFER);
  mstring_t FSBuf(MMAX_BUFFER);

  BufTok = NULL;

  IndexName[0] = '\0';

  while (MCfgGetSValMString(
          (CfgBuf != NULL) ? CfgBuf : MSched.ConfigBuffer,
          &BufTok,
          MParam[mcoFSTree],
          IndexName,  /* O */
          NULL,
          &Value,
          0,
          NULL) != FAILURE)
    {
    /* copy all FSTree parameters to single buffer */

    /* append string as "%s[%s] %s\n" */

    FSBuf += MParam[mcoFSTree];
    FSBuf += '[';
    FSBuf += IndexName;
    FSBuf += ']';
    FSBuf += ' ';
    FSBuf += Value;
    FSBuf += '\n';

    index = 0;
 
    /* make a mutable char buffer */
    char *mutableValue = strdup(Value.c_str());

    /* check for beginning of element */

    if (Value[0] == '<')
      {
      /* XML style FS Tree format - XML string together as one value in the array */

      ValArray[0] = mutableValue;
      ValArray[1] = NULL;
      }
    else
      {
      /* old style FSTREE[xxx] <ATTR>=<VALUE> defined in a moab.cfg file */
 
        ValArray[index] = MUStrTokE(mutableValue," \t\n",&ttokptr);
    
      /* NOTE:  loop automatically terminates array */

      while (ValArray[index] != NULL)
        {
        index++;

        ValArray[index] = MUStrTokE(NULL," \t\n",&ttokptr);

        if (index >= MMAX_CONFIGATTR - 1)
          break;
        }
      }

    /* process parameter */

    MFSTreeProcessConfig(
      ValArray,
      IndexName,
      NULL,
      FALSE,
      TRUE,
      NULL);  /* O */

    IndexName[0] = '\0';

    /* free the mutable char buffer */
    free(mutableValue);
    mutableValue = NULL;

    }  /* END while (MCfgGetSVal() != FAILURE) */

  if ((MSched.ParHasFSTree != NULL) && (MPar[0].FSC.ShareTree != NULL))
    {
    int pindex;
    mpar_t *P;
    char *tmpBuf = NULL;

    for (pindex = 1;pindex < MMAX_PAR;pindex++)
      {
      if (MPar[pindex].Name[0] == '\1')
        continue;

      if (MPar[pindex].Name[0] == '\0')
        break;

      if (MSched.ParHasFSTree[pindex] == FALSE)
        continue;

      if (MPar[pindex].FSC.ShareTree != NULL)
        continue;

      P = &MPar[pindex];

      /* replicate FS Tree */

      MFSTreeReplicate(
        MPar[0].FSC.ShareTree,  /* I */
        &P->FSC.ShareTree,
        P->Index);     /* O */

      /* copy FSTree config data for per partition processing */

      MUStrDup(&tmpBuf,FSBuf.c_str());

      BufTok = NULL;

      IndexName[0] = '\0';

      /* iterate over values */

      while (MCfgGetSValMString(
              tmpBuf,
              &BufTok,
              MParam[mcoFSTree],
              IndexName,  /* O */
              NULL,
              &Value,
              0,
              NULL) != FAILURE)
        {
        index = 0;

        /* make a mutable char buffer */
        char *mutableValue = strdup(Value.c_str());

        ValArray[index] = MUStrTok(mutableValue," \t\n",&ttokptr);

        /* NOTE:  loop automatically terminates array */

        while (ValArray[index] != NULL)
          {
          index++;

          ValArray[index] = MUStrTok(NULL," \t\n",&ttokptr);

          if (index >= MMAX_CONFIGATTR - 1)
            break;
          }

        /* process parameter */

        MFSTreeProcessConfig(
          ValArray,
          IndexName,
          P,
          FALSE,
          FALSE,  /* do not allow node creation */
          NULL);  /* O */

        IndexName[0] = '\0';

        /* free the mutable char buffer */
        free(mutableValue);
        mutableValue = NULL;
        }  /* END while(MCfgGetSValMstring() */
      }    /* END for (pindex) */

    MUFree(&tmpBuf);
    }      /* END if (MSched.ParHasFSTree != NULL) */

  MFSTreeGetDepth();

  for (pindex = 0;pindex < MMAX_PAR;pindex++)
    {
    if (MPar[pindex].Name[0] == '\1')
      continue;

    if (MPar[pindex].Name[0] == '\0')
      break;

    if (MPar[pindex].FSC.ShareTree == NULL)
      continue;

    if (MPar[0].FSC.FSTreeShareNormalize == TRUE)
      {
      mxml_t *E = MPar[pindex].FSC.ShareTree;
      double TotalNormalizedTreeShares;

      /* Assume the tree is not in a normalized format (e.g. sum of child shares for a given node is not equal to the parent shares.)
         In that case, then we can traverse the tree and calculate the normalized shares for each node in the tree, using the
         number of shares assigned to the partition (or 100 whichever is greater) as the total number of shares available. */

      TotalNormalizedTreeShares = MAX(100,E->TData->Shares);

      /* Traverse the tree and determine and set the normalized share count for each node in the tree. */

      MFSTreeShareNormalize(E,TotalNormalizedTreeShares,E->TData->Shares);
      }

    if (MFSTreeValidateConfig(MPar[pindex].FSC.ShareTree) == FAILURE)
      {
      MDB(1,fFS) MLog("ALERT:    fstree for partition %s is corrupt\n",
        MPar[pindex].Name);
      }
    }  /* END for (pindex) */

  return(SUCCESS);
  }  /* END MFSTreeLoadConfig() */





/**
 * Evaluate fair share tree configuration.
 *
 * @see MFSTreeLoadConfig() - parent
 *
 * @param AttrList (I)
 * @param IndexName (I)
 * @param SP (I) [optional]
 * @param EVal (I)
 * @param AddNode (I) - allow addition of new nodes
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 */

int MFSTreeProcessConfig(

  char    **AttrList,  /* I */
  char     *IndexName, /* I */
  mpar_t   *SP,        /* I (optional) */
  mbool_t   EVal,      /* I */
  mbool_t   AddNode,   /* I - allow addition of new nodes */
  char     *EMsg)      /* O (optional,minsize=MMAX_LINE) */

  {
  int aindex;
  int cindex;

  enum MFSTreeAttrEnum AttrType;


  char *optr;
  char *vptr;
  char *TokPtr = NULL;

  mfst_t *TData;

  mxml_t *E;

  void *O;
  void *F;

  mpar_t *P;

  int   OType;

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if ((AttrList == NULL) || 
      (AttrList[0] == NULL) || 
      (IndexName == NULL) || 
      (IndexName[0] == '\0'))
    {
    return(FAILURE);
    }

  if (EVal == TRUE)
    {
    /* NOTE:  eval not yet enabled */

    return(SUCCESS);
    }

  TData = NULL;

  mstring_t tmpBuffer(MMAX_LINE);

  for (aindex = 0;AttrList[aindex] != NULL;aindex++)
    {
    tmpBuffer = AttrList[aindex];

    if (tmpBuffer[0] == '<')
      {
      MFSTreeLoadXML(tmpBuffer.c_str(),NULL,NULL);

      continue;
      }

    if (TData == NULL)
      {
      if (SP != NULL)
        {
        if (SP->FSC.ShareTree == NULL)
          {
          if (AddNode == FALSE)
            {
            return(FAILURE);
            }
     
          MXMLCreateE(&SP->FSC.ShareTree,"root");
     
          SP->FSC.ShareTree->TData = (mfst_t *)MUCalloc(1,sizeof(mfst_t));

          TData = SP->FSC.ShareTree->TData;

          TData->L     = (mcredl_t *)MUCalloc(1,sizeof(mcredl_t));
          TData->F     = (mfs_t *)MUCalloc(1,sizeof(mfs_t));

          MPUInitialize(&TData->L->ActivePolicy);
          MPUCreate(&TData->L->IdlePolicy);

          MStatCreate((void **)&TData->S,msoCred);
          }
     
        P = SP;
        }   /* END if (SP != NULL) */
      else
        {
        if (MPar[0].FSC.ShareTree == NULL)
          {
          MXMLCreateE(&MPar[0].FSC.ShareTree,"root");
     
          MPar[0].FSC.ShareTree->TData = (mfst_t *)MUCalloc(1,sizeof(mfst_t));

          TData = MPar[0].FSC.ShareTree->TData;

          TData->L     = (mcredl_t *)MUCalloc(1,sizeof(mcredl_t));
          TData->F     = (mfs_t *)MUCalloc(1,sizeof(mfs_t));

          MPUInitialize(&TData->L->ActivePolicy);
          MPUCreate(&TData->L->IdlePolicy);

          MStatCreate((void **)&TData->S,msoCred);
          }
     
        P = &MPar[0];
        }  /* END else */
     
      /* FORMAT:  [<OTYPE>:]<OID> */
     
      if ((optr = strchr(IndexName,':')) != NULL)
        {
        *optr = '\0';
     
        OType = MUGetIndexCI(IndexName,MXO,FALSE,0);
     
        optr++;
        }
      else
        {
        optr = IndexName;
     
        OType = -1;
        } 
        
      if (MXMLFind(P->FSC.ShareTree,optr,OType,&E) == FAILURE)
        {
        if (AddNode == FALSE)
          {
          return(FAILURE);
          }
     
        /* cannot locate node, add to root */
     
        E = NULL;
     
        MXMLCreateE(&E,optr);
     
        MXMLAddE(P->FSC.ShareTree,E);
     
        E->TData = (mfst_t *)MUCalloc(1,sizeof(mfst_t));

        TData = E->TData;

        TData->L     = (mcredl_t *)MUCalloc(1,sizeof(mcredl_t));
        TData->F     = (mfs_t *)MUCalloc(1,sizeof(mfs_t));

        MPUInitialize(&TData->L->ActivePolicy);
        MPUCreate(&TData->L->IdlePolicy);

        MStatCreate((void **)&TData->S,msoCred);
        }
     
      TData = E->TData;
     
      if ((E == NULL) || (TData == NULL))
        {
        return(FAILURE);
        }
      }   /* END if (TData == NULL) */

    AttrType = (enum MFSTreeAttrEnum)MUGetIndexCI(tmpBuffer.c_str(),MFSTreeAttr,TRUE,0);

    if (AttrType == mfstaNONE)
      {
      /* FSTree-specific keyword not handled, try general cred attr */

      cindex = MUGetIndexCI(tmpBuffer.c_str(),MCredAttr,TRUE,0);

      if (cindex == 0)
        {
        /* invalid keyword specified */

        /* silently ignore (FIXME) */

        /* NO-OP */

        continue;
        }

      if (TData->L == NULL)
        {
        if ((TData->L = (mcredl_t *)MUCalloc(1,sizeof(mcredl_t))) == NULL)
          {
          /* cannot allocate memory for limit structure */

          /* silently ignore (FIXME) */

          /* NO-OP */

          break;
          }

        MPUInitialize(&TData->L->ActivePolicy);
        }

      if (TData->L->IdlePolicy == NULL)
        {
        MPUCreate(&TData->L->IdlePolicy);
        }

      if (TData->F == NULL)
        {
        TData->F = (mfs_t *)MUCalloc(1,sizeof(mfs_t));
        }

      MCredProcessConfig(TData,mxoxFSC,NULL,tmpBuffer.c_str(),(mcredl_t *)TData->L,(mfs_t *)TData->F,FALSE,NULL);
 
      continue;
      }  /* END if (AttrType == mfstaNONE) */
 
    /* process FSTree specific attribute */
 
    /* make a copy of the mstring to a mutable char array for processing */
    char *mutableString=NULL;
    MUStrDup(&mutableString,tmpBuffer.c_str());

    MUStrTok(mutableString,"=",&TokPtr);
    vptr = MUStrTok(NULL,"=",&TokPtr);

    switch (AttrType)
      {
      case mfstaShare:

        {
        char *tail;

        char *sptr;
        char *STokPtr = NULL;

        char *ptr;
        char *TokPtr = NULL;

        double tmpShares;

        /* FORMAT:  <TARGET>[<MOD>][@<PAR>][,<TARGET>[<MOD>][@<PAR>]]... */

        sptr = MUStrTok(vptr,",",&STokPtr);

        while (sptr != NULL)
          {
          ptr = MUStrTok(sptr,"@",&TokPtr);

          tmpShares = strtod(ptr,&tail);

          if (*tail == MFSTargetTypeMod[mfstTarget]) 
            TData->ShareTargetType = mfstTarget;
          else if (*tail == MFSTargetTypeMod[mfstFloor]) 
            TData->ShareTargetType = mfstFloor;
          else if (*tail == MFSTargetTypeMod[mfstCeiling]) 
            TData->ShareTargetType = mfstCeiling;
          else if (*tail == MFSTargetTypeMod[mfstCapRel]) 
            TData->ShareTargetType = mfstCapRel;
          else if (*tail == MFSTargetTypeMod[mfstCapAbs]) 
            TData->ShareTargetType = mfstCapAbs;

          ptr = MUStrTok(NULL,"@",&TokPtr);

          if (ptr != NULL)
            {
            mpar_t *P;

            if (SP != NULL)
              {
              if (!strcasecmp(SP->Name,ptr))
                {
                TData->Shares = tmpShares;
                }
              else
                {
                /* ignore config for non-specified partition */

                /* NO-OP */
                }
              }
            else if (MParAdd(ptr,&P) == SUCCESS)
              {
              if (MSched.ParHasFSTree == NULL)
                MSched.ParHasFSTree = (unsigned char *)MUCalloc(1,sizeof(mbool_t) * MMAX_PAR);

              MSched.ParHasFSTree[P->Index] = TRUE;

              /* do not process share info at this point */
              }
            else
              {
              /* warn that invalid parition specified */

              /* NYI */
              }
            }    /* END if (ptr != NULL) */
          else if (SP == NULL)
            {
            /* add share data to global partiiton */

            TData->Shares = tmpShares;
            } 

          sptr = MUStrTok(NULL,",",&STokPtr);
          }    /* END while (sptr != NULL) */
        }      /* END BLOCK */

        break;

      case mfstaChildList:

        {
        char *DPtr;
        char *TokPtr2 = NULL;
  
        char *OPtr;
        char *OID;
        char *TokPtr3 = NULL;

        mxml_t *CE;

        mfst_t *TData;

        enum MXMLOTypeEnum OType;

        if (AddNode == FALSE)
          {
          /* ignore child list */

          break;
          }

        DPtr = MUStrTok(vptr,",",&TokPtr2);

        while (DPtr != NULL)
          {
          CE = NULL;

          /* FORMAT:  [<OTYPE>:]<OID> */

          OPtr = MUStrTok(DPtr,":",&TokPtr3);
          OID  = MUStrTok(NULL,":",&TokPtr3);

          DPtr = MUStrTok(NULL,",",&TokPtr2);

          if (OID != NULL)
            {
            /* cred specified */

            OType = (enum MXMLOTypeEnum)MUGetIndexCI(OPtr,MXO,FALSE,0);

            if (OType == mxoNONE)
              {
              /* specification is invalid */

              continue;
              }
            }
          else
            {
            OType = mxoNONE;

            OID = OPtr;
            }

          if (MXMLCreateE(&CE,OID) == FAILURE)
            {
            /* cannot allocate memory for XML */

            continue;
            }

          CE->Type = OType;

          MXMLAddE(E,CE);

          if (CE->TData == NULL)
            {
            CE->TData = (mfst_t *)MUCalloc(1,sizeof(mfst_t));
            }

          TData = CE->TData;

          TData->Parent = E;

          if (OType != mxoNONE)
            {
            if ((MOGetObject((enum MXMLOTypeEnum)OType,OID,&O,mAdd) == FAILURE) ||
                (MOGetComponent(O,(enum MXMLOTypeEnum)OType,(void **)&F,mxoxFS) == FAILURE))
              {
              /* print error message and remove node */

              MDB(1,fFS) MLog("ERROR:    could not add fstree node '%s'\n",
                           OID);

              /* NYI */

              continue;
              }

            TData->O     = O;
            TData->OType = OType;
            TData->F     = (mfs_t *)MUCalloc(1,sizeof(mfs_t));

            if (TData->Shares == 0.0)
              {
              /* NOTE:  default to using *CFG[X] FSTARGET value */

              TData->Shares = ((mfs_t *)F)->FSTarget;
              }
            }
          else
            {
            TData->F     = (mfs_t *)MUCalloc(1,sizeof(mfs_t));
            }

          MStatCreate((void **)&TData->S,msoCred);

          TData->L     = (mcredl_t *)MUCalloc(1,sizeof(mcredl_t));
          MPUInitialize(&TData->L->ActivePolicy);
          MPUCreate(&TData->L->IdlePolicy);

          if (MSched.FSTreeACLPolicy != mfsapOff)
            {
            __MFSTreeBuildCredAccess(CE,E);
            }
          }  /* END while (DPtr != NULL) */
        }    /* END BLOCK */

        break;

      case mfstaManagers:
           
        if (TokPtr != NULL)
          {
          char *ManagerStrPtr; /* individual manager */
          char *ManagerValues; /* points to values of managers */

          int   mindex;

          mgcred_t** fManagers;

          /* FORMAT:  <USER>[,<USER>]... */

          mindex = 0;
            
          ManagerValues = vptr;

          fManagers = TData->F->ManagerU;

          if (fManagers[0] == NULL)
            {
            memset(fManagers,0,sizeof(fManagers));
            }

          ManagerStrPtr = MUStrTok(NULL,",",&ManagerValues);

          while (ManagerStrPtr != NULL)
            {
            if (mindex > MMAX_CREDMANAGER)
              {
              MDB(1,fFS) MLog("ERROR:    CredManager overflow, manager %s not added\n",
                ManagerStrPtr);

              /* Free the tmp mutable string */
              MUFree(&mutableString);
              return(FAILURE);
              }
             
            if (fManagers[mindex] == NULL)
              {
              fManagers[mindex] = (mgcred_t *)MUCalloc(1,sizeof(mgcred_t));

              if (MUserAdd(ManagerStrPtr,&fManagers[mindex]) == FAILURE)
                {
                MDB(1,fFS) MLog("ERROR:    could not add manager %s to fstree\n",
                  ManagerStrPtr);

                /* Free the tmp mutable string */
                MUFree(&mutableString);
                return(FAILURE);
                }

              ManagerStrPtr = MUStrTok(NULL,",",&ManagerValues);
              }
            else if (strcmp(fManagers[mindex]->Name,ManagerStrPtr) == 0)
              {
              /* manager already in the list */
              break;
              }

            mindex++;
            }
          }  /* END if (Value != NULL) */

        break;

      default:

        /* silently ignore non-handled fstree attribute (FIXME) */

        /* NO-OP */

        break;
      }  /* END switch (index) */

    /* Free the tmp mutable string */
    MUFree(&mutableString);
    }    /* END for (aindex) */ 

  return(SUCCESS);
  }  /* END MFSTreeProcessConfig() */





/**
 * Report FS Tree state.
 *
 * @param RootE (I)
 * @param PE () [optional]
 * @param FlagBM (I) [bitmap of ???]
 * @param Depth (I) current tree depth
 * @param String (O)
 */

int MFSTreeShow(

  mxml_t    *RootE,
  mxml_t    *PE,
  mbitmap_t *FlagBM,
  int        Depth,
  mstring_t *String)

  {
  mbitmap_t BM;
  char QALLine[MMAX_LINE];
  char Limits[MMAX_LINE];

  int   cindex;
  int   fsindex;

  mfst_t *TData;

  char   *qptr;  /* qoslist string pointer */

  char  tmpName[MMAX_NAME];

  static mpar_t *GP = &MPar[0];

  if (String == NULL)
    {
    return(FAILURE);
    }

  /* don't initialize */

  if ((RootE == NULL) || (RootE->Name == NULL))
    {
    return(FAILURE);
    }

  TData = RootE->TData;

  if (TData == NULL)
    {
    return(FAILURE);
    }

  if (Depth > 0)
    {
    sprintf(tmpName,"%*.*s- %.*s",
      (Depth - 1) * 2,
      (Depth - 1) * 2,
      "                                      ",
      MSched.FSTreeDepth * MFSTREE_TREESPACE,
      RootE->Name);
    }
  else
    {
    sprintf(tmpName,"%.16s",
      RootE->Name);
    }

  if (TData->L != NULL)
    {
    mcredl_t *L;

    L = TData->L;

    MCredShowAttrs(
      L,
      &L->ActivePolicy,
      L->IdlePolicy,
      L->OverrideActivePolicy,
      L->OverrideIdlePolicy,
      L->OverriceJobPolicy,
      L->RsvPolicy,
      L->RsvActivePolicy,
      NULL,
      0,
      FALSE,
      TRUE,
      Limits);
    }    /* END if (TData->L != NULL) */
  else
    {
    Limits[0] = '\0';
    }

  if (!bmisclear(&TData->F->QAL))
    {
    qptr = MQOSBMToString(&TData->F->QAL,QALLine);
    }
  else
    {
    qptr = NULL;
    }

  if (TData->F != NULL)
    {
    double Shares;
    double TotalChildShares;
    double UsageShares;

    if (MPar[0].FSC.FSTreeShareNormalize == TRUE)
      {
      /* use normalized shares */

      Shares = TData->NShares;
      TotalChildShares = TData->NShares; /* since shares are normalized the sum of the child shares is the same as the parent shares */
      UsageShares = TData->UsageNShares;
      }
    else
      {
      Shares = TData->Shares;
      UsageShares = fabs(((double)Shares) + (TData->Offset / MPar[0].FSC.FSTreeConstant));

      if (PE != NULL) 
        {
        TotalChildShares = PE->TData->CShares;
        }
      else
        {
        TotalChildShares = TData->Shares;
        }
      }

    if (!bmisset(FlagBM,mcmVerbose2))
      {
      MStringAppendF(String,"%-*.*s  %8.2f  %8.2f%c of %8.2f (%s: %.2f) (%.2f) %s %s%s\n",
        MSched.FSTreeDepth * MFSTREE_TREESPACE,
        MSched.FSTreeDepth * MFSTREE_TREESPACE,
        tmpName,
        UsageShares,
        Shares,
        MFSTargetTypeMod[TData->ShareTargetType],
        TotalChildShares,
        (RootE->Type > 0) ? MXO[RootE->Type] : "node",
        TData->F->FSFactor + TData->F->FSUsage[0], 
        (TData->OType == mxoNONE) ? 0.0 : ((mfs_t *)TData->F)->FSTreeFactor,
        (!MUStrIsEmpty(Limits)) ? Limits : "",
        (qptr != NULL) ? "QLIST=" : "",
        (qptr != NULL) ? qptr : "");

      MDB(7,fFS) MLog("INFO:     TData Shares %8.2f Usage Shares %8.2f TData Offset %8.2f FSTree Constant %8.2f, Factor %8.2f, Usage %8.2f \n",
        Shares,
        UsageShares,
        TData->Offset,
        MPar[0].FSC.FSTreeConstant,
        TData->F->FSFactor,
        TData->F->FSUsage[0]);
      }  /* END if (!bmisset(FlagBM,mcmVerbose2)) */
    else
      {
      char tmpLine[MMAX_LINE];
      char tmpFSCap[MMAX_LINE];

      char *tBPtr;
      int   tBSpace;

      MUSNInit(&tBPtr,&tBSpace,tmpLine,sizeof(tmpLine));

      cindex = 0;

      while (TData->F->ManagerU[cindex] != NULL)
        {
        MUSNPrintF(&tBPtr,&tBSpace,"%s%s",
          (cindex > 0) ? "," : "",
          TData->F->ManagerU[cindex]->Name);

        cindex++;
        }   /* END if (Name != NULL) */

      if (TData->F->FSCap > 0.0)
        sprintf(tmpFSCap,"%.1f%c",TData->F->FSCap,(TData->F->FSCapMode == mfstNONE) ? '%' : MFSTargetTypeMod[TData->F->FSCapMode]);
      else
        sprintf(tmpFSCap,"-----");

      MStringAppendF(String,"%-*.*s  %8.2f  %8.2f%c of %8.2f %8.8s  (%s: %.2f) (%.2f) %s %s (%.2f)",
        MSched.FSTreeDepth * MFSTREE_TREESPACE,
        MSched.FSTreeDepth * MFSTREE_TREESPACE,
        tmpName,
        UsageShares,
        Shares,
        MFSTargetTypeMod[TData->ShareTargetType],                          
        TotalChildShares,
        tmpFSCap,
        (RootE->Type > 0) ? MXO[RootE->Type] : "node",
        TData->F->FSFactor + TData->F->FSUsage[0], 
        (TData->OType == mxoNONE) ? 0.0 : ((mfs_t *)TData->F)->FSTreeFactor,
        (!MUStrIsEmpty(Limits)) ? Limits : "",
        tmpLine,
        TData->Offset);

      MDB(7,fFS) MLog("INFO:     TData Shares %8.2f UsageShares %8.2f TData Offset %8.2f FSTree Constant %8.2f\n",
        Shares,
        UsageShares,
        TData->Offset,
        MPar[0].FSC.FSTreeConstant);

      for (fsindex = 0;fsindex < GP->FSC.FSDepth;fsindex++)
        {
        if (TData->F->FSUsage[fsindex] > 0.0)
          {
          if (TData->F->FSUsage[fsindex] > 0.0)
            {
            MStringAppendF(String," %7.2f",
              TData->F->FSUsage[fsindex]);
/*
              TData->F->FSUsage[fsindex] / TData->F->FSUsage[fsindex] * 100.0);
*/
            }
          else
            {
            MStringAppendF(String," -------");
            }
          }
        }    /* END for (fsindex) */

      if (TData->Comment != NULL)
        {
        MStringAppendF(String,"\n %s",
          TData->Comment);
        }

      MStringAppendF(String,"\n");
      }
    }
  else
    {
    MStringAppendF(String,"%-16.16s  %8.2f  %8.2f%c of %8.2f --- CORRUPT\n",
      tmpName,
      TData->Shares + TData->Offset,
      TData->Shares,
      MFSTargetTypeMod[TData->ShareTargetType],
      (PE != NULL) ? PE->TData->CShares : TData->Shares);
    }

  for (cindex = 0;cindex < RootE->CCount;cindex++)
    {
    MFSTreeShow(
      RootE->C[cindex],
      RootE,
      FlagBM,
      Depth + 1,
      String);
    }  /* END for (cindex) */

  return(SUCCESS);
  }  /* END MFSTreeShow() */




/**
 *
 *
 * @param E (I) [modified]
 * @param TotalFSFactor (I) 
 * @param TotalFSUsage (I) 
 * @param TotalNormalizedShares (I) 
 */

int MFSTreeUpdateTNodeUsage(

  mxml_t *E,  /* I (modified) */
  double  TotalFSFactor,
  double  TotalFSUsage,
  double  TotalNormalizedShares)

  {
  int cindex;

  mfst_t *TData;
  mfst_t *CTData;

  if ((E == NULL) || (E->TData == NULL))
    {
    return(FAILURE);
    }

  TData = E->TData;

  if (TData == NULL)
    {
    return(FAILURE);
    }

  TData->CShares = 0.0;

  if (E->CCount != 0)
    {
    /* this is gather from children */

    if (TData->F != NULL)
      TData->F->FSUsage[0] = 0.0;
    }

  /* get total usage */

  for (cindex = 0;cindex < E->CCount;cindex++)
    {
    /* recursively update children */

    MFSTreeUpdateTNodeUsage(E->C[cindex],TotalFSFactor,TotalFSUsage,TotalNormalizedShares);

    CTData = E->C[cindex]->TData;

    if (CTData == NULL)
      continue;

    if ((TData->F != NULL) && (CTData->F != NULL))
      TData->F->FSUsage[0] += CTData->F->FSUsage[0];

    TData->CShares += CTData->Shares;
    }  /* END for (cindex) */

  if (TData->CShares == 0.0)
    {
    /* if child shares not specified, base usage on target shares */

    TData->CShares = TData->Shares;
    }

  TData->UsageNShares = 0;

  if ((TData->F != NULL) && ((TotalFSUsage + TotalFSFactor) != 0))
    {
    TData->UsageNShares = (TData->F->FSFactor + TData->F->FSUsage[0])/(TotalFSUsage + TotalFSFactor) * TotalNormalizedShares;
    }

  /* determine effective offset for each child */

  for (cindex = 0;cindex < E->CCount;cindex++)
    {
    /* recursively update children */

    CTData = E->C[cindex]->TData;

    if ((CTData == NULL) || (TData->F == NULL) || (CTData->F == NULL))
      continue;

    /* NOTE:  ShareUsage = ShareTarget + ShareOffset */

    /* NOTE:  offset > 0, usage exceeds target, offset < 0, target exceeds usage */

    if (MPar[0].FSC.FSTreeShareNormalize == FALSE)
      {
      double  UsageShares = 0;
      double  TargetChildShares;
      double  ParentShares;

      TargetChildShares = CTData->Shares; /* child target */
      ParentShares = TData->CShares;/* total of all the children shares */
   
      if ((TData->F->FSUsage[0] + TData->F->FSFactor) != 0.0)
        {
        double PercentOfParentUsage = (CTData->F->FSUsage[0] + CTData->F->FSFactor) / (TData->F->FSUsage[0] + TData->F->FSFactor);
  
        /* calculate the number of shares used by this child as a fraction of the parent shares based on the
           percentage of the parent usage by this child. For example, if the child usage is 50% of the parent usage
           then the child effectively used 50% of the parents shares. */
  
        UsageShares  = PercentOfParentUsage * ParentShares;
        }

      CTData->UsageShares = UsageShares; 
  
      /* Note that this only works if the tree was originaly setup in a normalized format (the sum of the children shares are equal to the parent shares).
         If a parent has 100 shares, but the children have a total of 10000 shares then we are subtracting apples from oranges. */
  
      CTData->Offset = (UsageShares - TargetChildShares) * MPar[0].FSC.FSTreeConstant;
      }
    else
      {
      /* use a different formula to callculate the offset for a normalized tree */

      double tmpOffset = 0;

      if (CTData->NShares > 0)
        {
        double ChildNUsageShares = CTData->UsageNShares;

        if (ChildNUsageShares == 0)
          {
          /* give the child a miniscule artificial usage so that we can calculate a fairshare usage value for children with 0 usage,
             otherwise, we get a situation where children with some usage end up with a higher priority than those with no usage .*/

          ChildNUsageShares = .001; 
          }

        tmpOffset  = ChildNUsageShares/CTData->NShares; /* usage divided by target */

        if (tmpOffset < 1)
          {
          /* usage was less than the target, make tmpOffset larger than 1, then make it a negative number */

          tmpOffset = 1/tmpOffset * -1;
          }
        }

      CTData->Offset = tmpOffset * MPar[0].FSC.FSTreeConstant;
      } /* END else (MPar[0].FSC.FSTreeShareNormalize == FALSE) */

    }  /* END for (cindex) */

  return(SUCCESS);
  }  /* END MFSTreeUpdateTNodeUsage() */



/**
 *
 *
 */

int MFSTreeUpdateUsage()

  {
  int pindex;
  int Children = 0;

  mfst_t *TData;

  for (pindex = 0;pindex < MMAX_PAR;pindex++)
    {
    if (MPar[pindex].Name[0] == '\1')
      continue;

    if (MPar[pindex].Name[0] == '\0')
      break;

    if (MPar[pindex].FSC.ShareTree != NULL)
      {
      TData = MPar[pindex].FSC.ShareTree->TData;

      MFSTreeUpdateTNodeUsage(MPar[pindex].FSC.ShareTree,TData->F->FSFactor,TData->F->FSUsage[0],TData->NShares);

/*
      if (TData->F->FSUsage[0] == 0.0)
        TData->Offset = TData->F->FSFactor;
      else
        TData->Offset = (TData->F->FSUsage[0] / TData->F->FSUsage[0]) * TData->CShares - TData->Shares;
*/

      MFSTreeAssignPriority(MPar[pindex].FSC.ShareTree,0.0,0,pindex,&Children);
      }
    }

  return(SUCCESS);
  }  /* END MFSTreeUpdateUsage() */





/**
 *
 *
 * @param E (I)
 * @param ParentFactor (I)
 * @param Depth (I)
 * @param PtIndex (I)
 * @param ChildrenFound (I/O)
 */

int MFSTreeAssignPriority(

  mxml_t *E,             /* I */
  double  ParentFactor,  /* I */
  int     Depth,         /* I */
  int     PtIndex,       /* I */
  int    *ChildrenFound) /* I/O */

  {
  int cindex;
  double Factor;

  mfst_t *TData;

  mxml_t *C;

  double tmpD;

  double Penalty = (MSched.FSTreeIsRequired ? MMAX_PRIO_VAL : -1);

  if ((E == NULL) || (ChildrenFound == NULL))
    {
    return(FAILURE);
    }

  if (MPar[0].FSC.FSTreeTierMultiplier < 0)
    tmpD = pow(MPar[0].FSC.FSTreeTierMultiplier * -1,(double)(MSched.FSTreeDepth - Depth));
  else
    tmpD = pow(MPar[0].FSC.FSTreeTierMultiplier,(double)Depth);

  for (cindex = 0;cindex < E->CCount;cindex++)
    {
    C = E->C[cindex];        

    TData = C->TData;

    Factor = ParentFactor + 
             ((MPar[0].FSC.FSTreeTierMultiplier == 0) ? 1 : tmpD) * 
             ((TData->Shares != 0) ? (-1 * TData->Offset) : (-1 * Penalty));

    MFSTreeAssignPriority(C,Factor,Depth + 1,PtIndex,ChildrenFound);

    TData->F->FSTreeFactor = Factor;

    /* NOTE: non-proportional tree */

    *ChildrenFound += 1;
    }  /* END for (cindex) */



  return(SUCCESS);
  }  /* END MFSTreeAssignPriority() */





/**
 *
 *
 * @param RootE
 * @param PE
 * @param OType
 * @param FlagBM
 * @param Depth
 * @param E
 */

int MFSTreeStatShow(

  mxml_t            *RootE,
  mxml_t            *PE,
  enum MXMLOTypeEnum OType,
  long               FlagBM,
  int                Depth,
  mxml_t           **E)

  {
  if ((RootE == NULL) || (E == NULL))
    {
    return(FAILURE);
    }

  MXMLCreateE(E,"Data");
  MFSTreeUpdateStats(RootE);
  MFSTreeGetStatXML(RootE,PE,OType,FlagBM,Depth,*E);

  return(SUCCESS);
  }





/**
 *
 *
 * @param RootE
 * @param PE () [optional]
 * @param OType
 * @param FlagBM (I) [bitmap of ???]
 * @param Depth (I) current tree depth
 * @param E (O)
 */

int MFSTreeGetStatXML(

  mxml_t             *RootE,
  mxml_t             *PE,      /* parent element (optional) */
  enum MXMLOTypeEnum  OType,
  long                FlagBM,  /* I (bitmap of ???) */
  int                 Depth,   /* I current tree depth */
  mxml_t             *E)       /* O */

  {
  int   cindex;

  mfst_t *TData;

  must_t *S;
  
  mxml_t *CE;
  mxml_t *SE;

  if (E == NULL)
    {
    return(FAILURE);
    }

  MXMLCreateE(&CE,(char *)MXO[OType]);
  MXMLSetAttr(CE,(char *)MCredAttr[mcaID],RootE->Name,mdfString);

  /* HACK: put depth info into index info */

  MXMLSetAttr(CE,(char *)MCredAttr[mcaIndex],(void *)&Depth,mdfInt);

  if ((RootE == NULL) || (RootE->Name == NULL))
    {
    return(FAILURE);
    }

  TData = RootE->TData;

  if (TData == NULL)
    {
    return(FAILURE);
    }

  S = (must_t *)TData->S;

  MStatToXML((void *)S,msoCred,&SE,TRUE,NULL);
  
  MXMLAddE(CE,SE);
  MXMLAddE(E,CE);

  for (cindex = 0;cindex < RootE->CCount;cindex++)
    {
    MFSTreeGetStatXML(
      RootE->C[cindex],
      RootE,
      OType,
      FlagBM,
      Depth + 1,
      E);
    }  /* END for (cindex) */

  return(SUCCESS);
  }  /* END MFSTreeShowStats() */





/**
 *
 *
 * @param E (I) [modified]
 */

int MFSTreeUpdateStats(

  mxml_t *E)  /* I (modified) */

  {
  int cindex;

  mfst_t *TData;
  mfst_t *CTData;

  must_t *S;
  must_t *CS;

  if ((E == NULL) || (E->TData == NULL))
    {
    return(FAILURE);
    }

  TData = E->TData;

  if (TData == NULL)
    {
    return(FAILURE);
    }

  if (TData->S == NULL)
    {
    MStatCreate((void **)&TData->S,msoCred);
    }

  S = (must_t *)TData->S;

  if (S == NULL)
    return(FAILURE);

  switch (TData->OType)
    {
    case mxoUser:
    case mxoAcct:
    case mxoGroup:

      MStatPCopy2(S,&((mgcred_t *)TData->O)->Stat,msoCred);

      break;

    case mxoClass:

      MStatPCopy2(S,&((mclass_t *)TData->O)->Stat,msoCred);
 
      break;

    case mxoQOS:

      MStatPCopy2(S,&((mqos_t *)TData->O)->Stat,msoCred);

      break;

    default:
 
      MStatPClear(S,msoCred);

      break;
    }

  TData->S = S;

  /* get total usage */

  for (cindex = 0;cindex < E->CCount;cindex++)
    {
    /* recursively update children */

    MFSTreeUpdateStats(E->C[cindex]);

    CTData = E->C[cindex]->TData;

    if (CTData == NULL)
      continue;

    CS = (must_t *)CTData->S;

    MStatMerge(S,CS,S,msoCred);
    }  /* END for (cindex) */

  return(SUCCESS);
  }  /* END MFSTreeUpdateStats() */




/**
 *
 *
 * @param E
 */

int MFSTreeFreeXD(
 
  mxml_t *E)

  {
  mfst_t *TData;

  /* NOTE: there are cases where TData->F and TData->L may be NULL (only failure cases) */

  /* NOTE: does not free children of E */

  TData = E->TData;

  if (TData == NULL)
    return(SUCCESS);

  if (TData->F != NULL)
    {
    if (TData->F->AAL != NULL)
      MULLFree(&TData->F->AAL,NULL);

    if (TData->F->GAL != NULL)
      MULLFree(&TData->F->GAL,NULL);

    if (TData->F->CAL != NULL)
      MULLFree(&TData->F->CAL,NULL);

    bmclear(&TData->F->Flags);

    MUFree((char **)&TData->F);
    }

  MStatFree((void **)&TData->S,msoCred);

  if (TData->L != NULL)
    {
    MUFree((char **)&TData->L->IdlePolicy);
    MUFree((char **)&TData->L);
    }
  
  MUFree((char **)&TData->Comment);

  MUFree((char **)&TData);

  E->TData = NULL;
 
  return(SUCCESS); 
  }  /* END MFSTreeFreeXD() */





/**
 *
 *
 * @param EP
 */

int MFSTreeFree(
 
  mxml_t **EP)

  {
  int index;

  mxml_t *E;

  if (EP == NULL)
    {
    return(FAILURE);
    }

  E = *EP;

  if (E == NULL)
    {
    return(SUCCESS);
    }

  if (E->C != NULL)
    {
    /* destroy children */

    for (index = 0;index < E->CCount;index++)
      {
      if (E->C[index] == NULL)
        continue;

      MFSTreeFree(&E->C[index]);
      }  /* END for (index) */

    MUFree((char **)&E->C);
    }  /* END if (E->C != NULL) */

  /* free attributes */

  if (E->AName != NULL)
    {
    for (index = 0;index < E->ACount;index++)
      {
      if (E->AName[index] == NULL)
        break;

      MUFree(&E->AName[index]);

      if ((E->AVal != NULL) && (E->AVal[index] != NULL))
        MUFree(&E->AVal[index]);
      }  /* END for (index) */

    if (E->AVal != NULL)
      {
      MUFree((char **)&E->AVal);
      }

    if (E->AName != NULL)
      {
      MUFree((char **)&E->AName);
      }
    }    /* END if (E->AName != NULL) */

  /* free name */

  if (E->Name != NULL)
    MUFree(&E->Name);

  if (E->Val != NULL)
    MUFree(&E->Val);

  MFSTreeFreeXD(E);

  MUFree((char **)EP);

  return(SUCCESS);
  }  /* END MFSTreeFree() */





/**
 *
 *
 * @param E
 * @param Depth
 */

int MFSTreeTraverse(

  mxml_t *E,
  int     Depth)

  {
  int cindex;

  if (E == NULL)
    {
    return(FAILURE);
    }

  if (Depth > MSched.FSTreeDepth)
    MSched.FSTreeDepth = Depth;

  for (cindex = 0;cindex < E->CCount;cindex++)
    {
    MFSTreeTraverse(E->C[cindex],Depth + 1);
    }

  return(SUCCESS);
  }  /* END MFSTreeTraverse() */




int MFSTreeGetDepth()

  {
  mpar_t *P;

  int Depth;
  int pindex;

  for (pindex = 0;pindex < MSched.M[mxoPar];pindex++)
    {
    P = &MPar[pindex];

    if (P->Name[0] == '\0')
      break;

    if (P->Name[0] == '\1')
      continue;

    if (P->FSC.ShareTree == NULL)
      continue;

    Depth = 0;

    MFSTreeTraverse(P->FSC.ShareTree,Depth + 1);
    }  /* END for (pindex) */

  return(SUCCESS);
  }  /* END MFSTreeGetDepth() */




/**
 * Clear specified usage from FS tree.
 *
 * @param E (I) [modified]
 * @param Active - clear active usage
 * @param Idle - clear idle usage
 * @param PTypeBM (I) - clear other usage
 */

int MFSTreeStatClearTNodeUsage(

  mxml_t    *E,
  mbool_t    Active,
  mbool_t    Idle,
  mbool_t    PCredTotal)

  {
  int cindex;
 
  mcredl_t *L;

  /* TData and L should never be NULL */

  if ((E == NULL) || (E->TData == NULL))
    {
    return(FAILURE);
    }

  L = E->TData->L;

  if (L != NULL)
    {
    if (Active == TRUE)
      MPUClearUsage(&L->ActivePolicy);

    if ((L->IdlePolicy != NULL) && (Idle == TRUE))
      MPUClearUsage(L->IdlePolicy);

    if (PCredTotal)
      memset(L->TotalJobs,0,sizeof(L->TotalJobs));
    }  /* END if (L != NULL) */

  for (cindex = 0;cindex < E->CCount;cindex++)
    {
    MFSTreeStatClearTNodeUsage(E->C[cindex],Active,Idle,PCredTotal);
    }  /* END for (cindex) */

  return(SUCCESS);
  }  /* END MFSTreeStatClearNodeUsage; */






/**
 *
 *
 * @param PTypeBM 
 * @param PCredTotal (I) 
 */

int MFSTreeStatClearUsage(

  mbool_t    Active,
  mbool_t    Idle,
  mbool_t    PCredTotal)

  {
  mpar_t *P;

  int pindex;

  for (pindex = 0;pindex < MSched.M[mxoPar];pindex++)
    {
    P = &MPar[pindex];

    if (P->Name[0] == '\0')
      break;

    if (P->Name[0] == '\1')
      continue;

    if (P->FSC.ShareTree == NULL)
      continue;

    MFSTreeStatClearTNodeUsage(P->FSC.ShareTree,Active,Idle,PCredTotal);
    }  /* END for (pindex) */

  return(SUCCESS);
  }   /* END MFSTreeStatClearUsage() */




/**
 *
 *
 * @param SrcE
 * @param DstE
 * @param PE
 * @param PtIndex
 */

int MFSTreeReplicateTNode(

  mxml_t *SrcE,
  mxml_t *DstE,
  mxml_t *PE,
  int     PtIndex)

  {
  int cindex;

  mfst_t *DTData;

  /* duplicate TData->L, TData->S */

  /* point Cred->FSTreeNode[PtIndex] to this node */

  if (DstE == NULL)
    {
    return(FAILURE);
    }

  if (DstE->TData != NULL)
    {
    MFSTreeFreeXD(DstE);
    }

  DstE->TData = (mfst_t *)MUCalloc(1,sizeof(mfst_t));

  DTData = DstE->TData;
 
  memcpy(DstE->TData,SrcE->TData,sizeof(mfst_t));

  DTData->PtIndex = PtIndex;

  if (PE != NULL)
    DTData->Parent = PE;

  DTData->L     = (mcredl_t *)MUCalloc(1,sizeof(mcredl_t));
  MPUInitialize(&DTData->L->ActivePolicy);
  MPUCreate(&DTData->L->IdlePolicy);
  DTData->F     = (mfs_t *)MUCalloc(1,sizeof(mfs_t));

  MStatCreate((void **)&DTData->S,msoCred);

  for (cindex = 0;cindex < SrcE->CCount;cindex++)
    {
    MFSTreeReplicateTNode(SrcE->C[cindex],DstE->C[cindex],DstE,PtIndex);
    }

  return(TRUE);
  }  /* END MFSTreeReplicateTNode() */




/**
 *
 *
 * @param SrcFSTree (I)
 * @param DstFSTree (O) [allocated]
 * @param PtIndex
 */

int MFSTreeReplicate(

  mxml_t  *SrcFSTree,  /* I */
  mxml_t **DstFSTree,  /* O (allocated) */
  int      PtIndex) 

  {
  if ((SrcFSTree == NULL) || (DstFSTree == NULL))
    {
    return(FAILURE);
    }

  MXMLDupE(SrcFSTree,DstFSTree);

  MFSTreeReplicateTNode(SrcFSTree,*DstFSTree,NULL,PtIndex);

  return(SUCCESS);
  }  /* END MFSTreeReplicate() */





/**
 *
 *
 * @param E (I)
 */

int MFSTreeValidateConfig(

  mxml_t *E)  /* I */

  {
  int cindex;
  
  mfst_t *TData;
  mfst_t *CTData;

  mfs_t *F;
  mfs_t *CF;

  mbool_t FailureDetected = FALSE;

  if ((E == NULL) || (E->TData == NULL))
    {
    return(FAILURE);
    }

  TData = E->TData;
  F = TData->F;

  for (cindex = 0;cindex < E->CCount;cindex++)
    {
    CTData = E->C[cindex]->TData;

    if ((CTData == NULL) || (CTData->F == NULL) || (F == NULL))
      { 
      FailureDetected = TRUE;
      }
    else if (MSched.FSTreeACLPolicy == mfsapFull)
      {
      CF = (mfs_t *)CTData->F;

      /* This copies the QOS from E to its children */
      /* Make sure that FSTREEACLPOLICY is not OFF */

      if (CF->QDef[0] == NULL)
        memcpy(CF->QDef,F->QDef,sizeof(F->QDef));

      bmor(&CF->QAL,&F->QAL);
      }

    if (MFSTreeValidateConfig(E->C[cindex]) == FAILURE)
      FailureDetected = TRUE;
    }  /* END for (cindex) */

  if (FailureDetected == TRUE)
    {
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MFSTreeValidateConfig() */



/**
 *
 *
 * @param Buf (I)
 * @param TreeP (O) [optional - no MPar.FSC.ShareTree will be populated]
 * @param EMsg (O) [optional minsize=MMAX_LINE]
 */

int MFSTreeLoadXML(

  const char    *Buf,   /* I */ 
  mxml_t **TreeP, /* O (optional - no MPar.FSC.ShareTree will be populated) */
  char    *EMsg)  /* O (optional minsize=MMAX_LINE) */

  {
  mxml_t *E;
  mxml_t *Tree;
  mxml_t *CE;

  mpar_t *P;

  int CTok;

  char Partition[MMAX_NAME];

  E = NULL;

  if (MXMLFromString(&E,Buf,NULL,NULL) == FAILURE)
    {
    if (EMsg != NULL)
      MUStrCpy(EMsg,"Invalid XML specified\n",MMAX_LINE);

    return(FAILURE);
    }

  if (strcasecmp(E->Name,MParam[mcoFSTree]))
    {
    if (EMsg != NULL)
      MUStrCpy(EMsg,"XML is not for fstree\n",MMAX_LINE);

    MXMLDestroyE(&E);

    return(FAILURE);
    }

  CTok = -1;

  while (MXMLGetChild(E,(char *)MXO[mxoTNode],&CTok,&CE) == SUCCESS)
    {
    Tree = NULL;

    if (TreeP == NULL)
      {
      if (MXMLGetAttr(CE,"partition",NULL,Partition,sizeof(Partition)) == SUCCESS)
        {
        /* changed to MParAdd() 7/10/09 by DRW for RT 5355 */
        /* change fixes race condition where partitions are added after
           the trees are created */

        if (MParAdd(Partition,&P) == FAILURE)
          continue;

        if (P->FSC.ShareTree != NULL)
          {
          /* FIXME:  no support for dynamic changes */

          continue;
          }

        if (MSched.ParHasFSTree == NULL)
          MSched.ParHasFSTree = (unsigned char *)MUCalloc(1,sizeof(mbool_t) * MMAX_PAR);

        MSched.ParHasFSTree[P->Index] = TRUE;
        }
      else
        {
        P = &MPar[0];

        if (P->FSC.ShareTree != NULL)
          {
          /* FIXME:  no support for dynamic changes */

          continue;
          }
        }

      if (__MFSTreeBuildFromXML(&P->FSC.ShareTree,CE,P,NULL) == FAILURE)
        {
        MDB(1,fFS) MLog("ALERT:    could not build fstree\n");

        MFSTreeFree(&P->FSC.ShareTree);

        return(FAILURE);
        }

      MSched.FSTreeConfigDetected = TRUE;
      }
    else
      {
      /* only grab the first tree in the xml */

      if (__MFSTreeBuildFromXML(&Tree,CE,&MPar[0],NULL) == FAILURE)
        {
        return(FAILURE);
        }

      *TreeP = Tree;

      break;
      }
    }    /* END while (MXMLGetChild(E,(char *)MXO[mxoTNode],&CTok,&CE) == SUCCESS) */

  MXMLDestroyE(&E);

  return(SUCCESS);
  }  /* END MFSTreeLoadXML() */





/**
 *
 *
 * @param Tree (O)
 * @param E (I)
 * @param P (I)
 * @param Parent (I)
 */

int __MFSTreeBuildFromXML(

  mxml_t **Tree,    /* O */
  mxml_t  *E,       /* I */
  mpar_t  *P,       /* I */
  mxml_t  *Parent)  /* I */

  {
  char tmpLine[MMAX_LINE];

  enum MXMLOTypeEnum OType;

  void *O;
  mfs_t *F;

  mxml_t *EP;
  mxml_t *CE;
  mxml_t *NE;

  int CTok;
  
  mfst_t *TData;

  if (Tree != NULL)
    *Tree = NULL;

  if ((Tree == NULL) || (E == NULL) || (P == NULL))
    {
    return(FAILURE);
    }

  if (MXMLGetAttr(E,"name",NULL,tmpLine,sizeof(tmpLine)) == FAILURE)
    {
    return(FAILURE);
    }

  MXMLCreateE(Tree,tmpLine);

  EP = *Tree;

  EP->TData = (mfst_t *)MUCalloc(1,sizeof(mfst_t));

  TData = EP->TData;

  TData->PtIndex = P->Index;

  TData->Parent  = Parent;

  OType = mxoNONE;

  if (MXMLGetAttr(E,(char *)MFSTreeAttr[mfstaType],NULL,tmpLine,sizeof(tmpLine)) != FAILURE)
    {
    OType = (enum MXMLOTypeEnum)MUGetIndexCI(tmpLine,MXO,FALSE,0);
    }
  
  if (OType != mxoNONE)
    {
    if ((MOGetObject((enum MXMLOTypeEnum)OType,EP->Name,&O,mAdd) == FAILURE) ||
        (MOGetComponent(O,(enum MXMLOTypeEnum)OType,(void **)&F,mxoxFS) == FAILURE))
      {
      /* print error message and remove node */

      /* NYI */

      return(FAILURE);
      }

    TData->O     = O;
    TData->OType = OType;
    EP->Type     = OType;
    }

  TData->F     = (mfs_t *)MUCalloc(1,sizeof(mfs_t));
  TData->L     = (mcredl_t *)MUCalloc(1,sizeof(mcredl_t));

  MPUInitialize(&TData->L->ActivePolicy);
  MPUCreate(&TData->L->IdlePolicy);

  if (MXMLGetAttr(E,(char *)MFSTreeAttr[mfstaShare],NULL,tmpLine,sizeof(tmpLine)) != FAILURE)
    {
    char *tail;

    char *sptr;
    char *STokPtr;

    double tmpShares;

    /* FORMAT:  <TARGET>[<MOD>][,<TARGET>[<MOD>]]... */

    sptr = MUStrTok(tmpLine,",",&STokPtr);

    while (sptr != NULL)
      {
      tmpShares = strtod(sptr,&tail);

      if (*tail == MFSTargetTypeMod[mfstTarget]) 
        TData->ShareTargetType = mfstTarget;
      else if (*tail == MFSTargetTypeMod[mfstFloor]) 
        TData->ShareTargetType = mfstFloor;
      else if (*tail == MFSTargetTypeMod[mfstCeiling]) 
        TData->ShareTargetType = mfstCeiling;
      else if (*tail == MFSTargetTypeMod[mfstCapRel]) 
        TData->ShareTargetType = mfstCapRel;
      else if (*tail == MFSTargetTypeMod[mfstCapAbs]) 
        TData->ShareTargetType = mfstCapAbs;

      TData->Shares = tmpShares;

      sptr = MUStrTok(NULL,",",&STokPtr);
      }    /* END while (sptr != NULL) */
    }      /* END if (MXMLGetAttr() != FAILURE) */


  if (MXMLGetAttr(E,(char *)MFSTreeAttr[mfstaFSCap],NULL,tmpLine,sizeof(tmpLine)) != FAILURE)
    {
    char   *tail;
    double  tmpCap;

    /* FORMAT: CAP[^|%] */

    tmpCap = strtod(tmpLine,&tail);

    if (*tail == MFSTargetTypeMod[mfstCapAbs]) 
      TData->F->FSCapMode = mfstCapAbs;
    else
      TData->F->FSCapMode = mfstCapRel;

    TData->F->FSCap = tmpCap;
    }

  if (MXMLGetAttr(E,(char *)MFSTreeAttr[mfstaComment],NULL,tmpLine,sizeof(tmpLine)) != FAILURE)
    {
    TData->Comment = (char *)MUMalloc(sizeof(char) * (strlen(tmpLine) + 1));

    snprintf(TData->Comment,strlen(tmpLine) + 1,"%s",tmpLine);
    }

  if (MXMLGetAttr(E,(char *)MFSTreeAttr[mfstaLimits],NULL,tmpLine,sizeof(tmpLine)) != FAILURE)
    {
    MCredProcessConfig(
      TData,
      mxoxFSC,
      NULL,
      tmpLine,
      (mcredl_t *)TData->L,
      (mfs_t *)TData->F,
      FALSE,
      NULL);
    }

  if (MXMLGetAttr(E,(char *)MFSTreeAttr[mfstaUsage],NULL,tmpLine,sizeof(tmpLine)) != FAILURE)
    {
    TData->F->FSUsage[0] = strtod(tmpLine,NULL);
    }

  if (MXMLGetAttr(E,(char *)MFSTreeAttr[mfstaQList],NULL,tmpLine,sizeof(tmpLine)) != FAILURE)
    {
    char Line[MMAX_LINE];

    snprintf(Line,sizeof(Line),"QLIST=%s",tmpLine);

    MCredProcessConfig(NULL,OType,NULL,Line,TData->L,TData->F,FALSE,NULL);
    }

  if (MXMLGetAttr(E,(char *)MFSTreeAttr[mfstaQDef],NULL,tmpLine,sizeof(tmpLine)) != FAILURE)
    {
    char Line[MMAX_LINE];

    snprintf(Line,sizeof(Line),"QDEF=%s",tmpLine);

    MCredProcessConfig(NULL,OType,NULL,Line,TData->L,TData->F,FALSE,NULL);
    }

  /* build up credential information on leaf nodes */

  if ((Parent != NULL) && (MSched.FSTreeACLPolicy != mfsapOff))
    {
    __MFSTreeBuildCredAccess(EP,Parent);
    }

  CTok = -1;

  while (MXMLGetChild(E,(char *)MXO[mxoTNode],&CTok,&CE) == SUCCESS)
    {
    NE = NULL;

    if (__MFSTreeBuildFromXML(&NE,CE,P,EP) == SUCCESS)
      {
      MXMLAddE(EP,NE);
      }
    else
      {
      MDB(1,fFS) MLog("ALERT:    could not build fstree\n");

      return(FAILURE);
      }
    }
  
  return(SUCCESS);
  }  /* END __MFSTreeBuildFromXML() */




/**
 *
 *
 * @param Tree
 * @param PE
 * @param E
 * @param FlagBM
 */

int __MFSTreeToXML(

  mxml_t    *Tree,
  mxml_t    *PE,
  mxml_t    *E,
  mbitmap_t *FlagBM)

  {
  int CTok;

  mxml_t *CE;
  mxml_t *TE;

  mfst_t *TData;

  if ((E == NULL) || (Tree == NULL))
    {
    return(FAILURE);
    }

  MXMLCreateE(&CE,(char *)MXO[mxoTNode]);

  MXMLSetAttr(CE,(char *)MFSTreeAttr[mfstaName],Tree->Name,mdfString);

  TData = Tree->TData;

  if (bmisset(FlagBM,mfstaLimits))
    {
    mcredl_t *L;
    char tmpln[MMAX_LINE];

    L = TData->L;

    MCredShowAttrs(L,&L->ActivePolicy,L->IdlePolicy,L->OverrideActivePolicy,L->OverrideIdlePolicy,L->OverriceJobPolicy,L->RsvPolicy,L->RsvActivePolicy,NULL,0,FALSE,TRUE,tmpln);

    if (!MUStrIsEmpty(tmpln))
      {
      char *ptr = tmpln;

      while(isspace(*ptr))
        {
        ptr++;
        }

      MXMLSetAttr(CE,(char *)MFSTreeAttr[mfstaLimits],ptr,mdfString);
      }
    }

  if (bmisset(FlagBM,mfstaUsage) && 
      (TData->F != NULL))
    {
    MXMLSetAttr(CE,(char  *)MFSTreeAttr[mfstaUsage],&TData->F->FSUsage[0],mdfDouble);
    }

  if (bmisset(FlagBM,mfstaFSFactor) &&
      (TData->F != NULL))
    {
    double Usage = fabs(((double)TData->Shares) + (TData->Offset / MPar[0].FSC.FSTreeConstant));

    MXMLSetAttr(CE,(char *)MFSTreeAttr[mfstaFSFactor],(void *)&Usage,mdfDouble);
    }

  if (bmisset(FlagBM,mfstaType) && (TData->OType != mxoNONE))
    {
    MXMLSetAttr(CE,(char *)MFSTreeAttr[mfstaType],(void *)MXO[TData->OType],mdfString);
    }

  if (bmisset(FlagBM,mfstaShare) && (TData->Shares != 0.0))
    {
    MXMLSetAttr(CE,(char *)MFSTreeAttr[mfstaShare],&TData->Shares,mdfDouble);
    }

  if (bmisset(FlagBM,mfstaCShare))
    {
    MXMLSetAttr(CE,(char *)MFSTreeAttr[mfstaCShare],&TData->CShares,mdfDouble);
    }

  if (bmisset(FlagBM,mfstaFSCap) && (TData->F->FSCap > 0.0))
    {
    char tmpFSCap[MMAX_LINE];

    sprintf(tmpFSCap,"%.1f%c",TData->F->FSCap,(TData->F->FSCapMode == mfstNONE) ? '%' : MFSTargetTypeMod[TData->F->FSCapMode]);

    MXMLSetAttr(CE,(char *)MFSTreeAttr[mfstaFSCap],tmpFSCap,mdfString);
    }

  if (bmisset(FlagBM,mfstaComment) && (TData->Comment != NULL))
    {
    MXMLSetAttr(CE,(char *)MFSTreeAttr[mfstaComment],TData->Comment,mdfString);
    }

  if ((bmisset(FlagBM,mfstaTotalShare)) && 
      (PE != NULL) &&
      (PE->TData->CShares != 0.0))
    {
    MXMLSetAttr(CE,(char *)MFSTreeAttr[mfstaTotalShare],&PE->TData->CShares,mdfDouble);
    }

  if ((bmisset(FlagBM,mfstaQList)) && 
      (TData->F != NULL) && 
      !bmisclear(&TData->F->QAL))
    {
    char tmpLine[MMAX_LINE];

    MXMLSetAttr(CE,(char *)MFSTreeAttr[mfstaQList],(void *)MQOSBMToString(&TData->F->QAL,tmpLine),mdfString);
    }

  MXMLAddE(E,CE);

  CTok = -1;

  while (MXMLGetChild(Tree,NULL,&CTok,&TE) == SUCCESS)
    {
    __MFSTreeToXML(TE,Tree,CE,FlagBM);
    }

  return(SUCCESS);
  }  /* END __MFSTreeToXML() */



/**
 *
 *
 * @param Tree
 * @param E
 * @param FlagBM
 */

int MFSTreeToXML(

  mxml_t    *Tree,
  mxml_t   **E,
  mbitmap_t *FlagBM)

  {
  int CTok;

  mxml_t *EP;
  mxml_t *CE;
  mxml_t *TE;

  mfst_t *TData;

  if ((Tree == NULL) || (E == NULL))
    {
    return(FAILURE);
    }

  *E = NULL;

  MXMLCreateE(&EP,MParam[mcoFSTree]);

  MXMLCreateE(&CE,(char *)MXO[mxoTNode]);

  MXMLSetAttr(CE,(char *)MFSTreeAttr[mfstaName],Tree->Name,mdfString);

  TData = Tree->TData;

  if (bmisset(FlagBM,mfstaLimits))
    {
    char tmpln[MMAX_LINE];

    mcredl_t *L = (mcredl_t *)TData->L;

    MCredShowAttrs(L,&L->ActivePolicy,L->IdlePolicy,L->OverrideActivePolicy,L->OverrideIdlePolicy,L->OverriceJobPolicy,L->RsvPolicy,L->RsvActivePolicy,NULL,0,FALSE,TRUE,tmpln);

    if (!MUStrIsEmpty(tmpln))
      {
      char *ptr = tmpln;

      while(isspace(*ptr))
        {
        ptr++;
        }

      MXMLSetAttr(CE,(char *)MFSTreeAttr[mfstaLimits],ptr,mdfString);
      }
    }
 
  if (bmisset(FlagBM,mfstaPartition))
    {
    MXMLSetAttr(CE,(char *)MFSTreeAttr[mfstaPartition],MPar[TData->PtIndex].Name,mdfString);
    }
  if (bmisset(FlagBM,mfstaUsage) && 
      (TData->F != NULL))
    {
    MXMLSetAttr(CE,(char  *)MFSTreeAttr[mfstaUsage],&TData->F->FSUsage[0],mdfDouble);
    }

  if (bmisset(FlagBM,mfstaFSFactor) &&
      (TData->F != NULL))
    {
    double Usage = fabs(((double)TData->Shares) + (TData->Offset / MPar[0].FSC.FSTreeConstant));

    MXMLSetAttr(CE,(char *)MFSTreeAttr[mfstaFSFactor],(void *)&Usage,mdfDouble);
    }

  if (bmisset(FlagBM,mfstaType) && (TData->OType != mxoNONE))
    {
    MXMLSetAttr(CE,(char *)MFSTreeAttr[mfstaType],(void *)MXO[TData->OType],mdfString);
    }

  if (bmisset(FlagBM,mfstaShare) && (TData->Shares != 0.0))
    {
    MXMLSetAttr(CE,(char *)MFSTreeAttr[mfstaShare],&TData->Shares,mdfDouble);
    }

  if ((bmisset(FlagBM,mfstaQList)) && 
      (TData->F != NULL) && 
      !bmisclear(&TData->F->QAL))
    {
    char tmpLine[MMAX_LINE];

    MXMLSetAttr(CE,(char *)MFSTreeAttr[mfstaQList],(void *)MQOSBMToString(&TData->F->QAL,tmpLine),mdfString);
    }

  MXMLAddE(EP,CE);

  CTok = -1;

  while (MXMLGetChild(Tree,NULL,&CTok,&TE) == SUCCESS)
    {
    __MFSTreeToXML(TE,Tree,CE,FlagBM);
    }

  *E = EP;

  return(SUCCESS);
  }  /* END MFSTreeToXML() */




/**
 *
 *
 * @param Tree (I) - Tree
 * @param FC (I) - Data
 * @param Write (I) - Write the data to file
 * @param Calc (I) - Calculate new windows data
 * @param Rotate (I) - Rotate the data to file
 */

int MFSTreeUpdateFSData(

  mxml_t    *Tree,
  mfsc_t    *FC,
  mbool_t    Write,
  mbool_t    Calc,
  mbool_t    Rotate)

  {
  mxml_t *CE;

  mfs_t  *F;
  mfst_t *TData;

  int fsindex;
  int CTok;

  if (Tree == NULL)
    {
    return(FAILURE);
    }

  TData = Tree->TData; 
  F     = TData->F;
  
  if (Rotate == TRUE)
    {
    for (fsindex = 1;fsindex < MMAX_FSDEPTH;fsindex++)
      F->FSUsage[MMAX_FSDEPTH - fsindex] = F->FSUsage[MMAX_FSDEPTH - fsindex - 1];

    F->FSUsage[0] = 0.0;
    }

  if ((Calc == TRUE) || (Rotate == TRUE))
    {
    F->FSFactor     = MFSCalcFactor(FC,F->FSUsage);
    F->FSTreeFactor = F->FSFactor;

    MDB(7,fFS) MLog("INFO:  fairshare object %s fsfactor set to %0.2f\n",
      Tree->Name,
      F->FSFactor);
    }

  CTok = -1;

  while (MXMLGetChild(Tree,NULL,&CTok,&CE) == SUCCESS)
    {
    MFSTreeUpdateFSData(CE,FC,Write,Calc,Rotate);
    }

  return(SUCCESS);
  }  /* END MFSTreeUpdateFSData() */




/**
 *
 *
 * @param Tree (I) [modified]
 * @param FSTree (I)
 * @param FSSlot (I) 
 * @param Reload (I) 
 */

int MFSTreeLoadFSData(

  mxml_t *Tree,   /* I (modified) */
  mxml_t *FSTree, /* I */
  int     FSSlot, /* I */
  mbool_t Reload) /* I */

  {
  mxml_t *CE;
  mxml_t *FSCE;

  mfst_t *TData;
  mfst_t *FSTData;

  int     CTok;

  if ((Tree == NULL) || (FSTree == NULL))
    {
    return(FAILURE);
    }

  if (strcmp(Tree->Name,FSTree->Name))
    {
    return(FAILURE);
    }

  TData   = Tree->TData;
  FSTData = FSTree->TData;

  if (Reload == TRUE)
    {
    int index;

    /* in the case of a reload we need the factor imformation 
       and all of the usage data from the old tree */

    TData->F->FSFactor = FSTData->F->FSFactor;
    TData->F->FSTreeFactor = FSTData->F->FSTreeFactor;

    for (index = 0; index < MMAX_FSDEPTH; index++)
      {
      TData->F->FSUsage[index] = FSTData->F->FSUsage[index];
      }
    }
  else
    {
    TData->F->FSUsage[FSSlot] = FSTData->F->FSUsage[0];
    }

  CTok = -1;

  while (MXMLGetChild(FSTree,NULL,&CTok,&FSCE) == SUCCESS)
    {
    if (MXMLGetChild(Tree,FSCE->Name,NULL,&CE) == SUCCESS)
      {
      MFSTreeLoadFSData(CE,FSCE,FSSlot,Reload);
      }
    }

  return(SUCCESS);
  }  /* END MFSTreeLoadFSData() */




/**
 *
 *
 * @param Tree
 * @param Parent
 */

int __MFSTreeBuildCredAccess(

  mxml_t  *Tree,
  mxml_t  *Parent)

  {
  int    cindex;
  int    pindex;

  mfs_t *F;

  mfst_t *TData;

  mln_t *tmpL;

  mgcred_t** PManagers = Parent->TData->F->ManagerU;  /* ptr to parent's managers */
  mgcred_t** CManagers = Tree->TData->F->ManagerU;    /* ptr to child's managers */

  TData = Tree->TData;

  F = TData->F;  /* child data */

  if (MSched.FSTreeACLPolicy == mfsapFull)
    {
    tmpL = Parent->TData->F->AAL;

    while (tmpL != NULL)
      {
      MULLAdd(&F->AAL,tmpL->Name,tmpL->Ptr,NULL,NULL);  /* no free routine */

      tmpL = tmpL->Next;
      }

    tmpL = Parent->TData->F->GAL;

    while (tmpL != NULL)
      {
      MULLAdd(&F->GAL,tmpL->Name,tmpL->Ptr,NULL,NULL);  /* no free routine */

     tmpL = tmpL->Next;
      }

    tmpL = Parent->TData->F->CAL;

    while (tmpL != NULL)
      {
      MULLAdd(&F->CAL,tmpL->Name,tmpL->Ptr,NULL,NULL);  /* no free routine */

      tmpL = tmpL->Next;
      }

    bmor(&F->QAL,&Parent->TData->F->QAL);

    /* add managers parent's managers to child's list */
   
    cindex = 0;
    pindex = 0;

    while (PManagers[pindex] != NULL)
      {
      if (cindex >= MMAX_CREDMANAGER)
        {
        MDB(1,fFS) MLog("ERROR:    CredManager overflow while adding managers to child in fstree\n");

        return(FAILURE);
        } 

      if (CManagers[cindex] == NULL)
        {
        CManagers[cindex] = PManagers[pindex];
        }
      else if (CManagers[cindex] == PManagers[pindex])
        {
        pindex++;

        continue;
        }
     
      cindex++;
      pindex++;
      }
    }

  /* add parent object */

  switch (Parent->TData->OType)
    {
    case mxoGroup:

      MULLAdd(&F->GAL,Parent->Name,Parent->TData->O,NULL,NULL);  /* no free routine */

      break;

    case mxoClass:

      MULLAdd(&F->CAL,Parent->Name,Parent->TData->O,NULL,NULL);  /* no free routine */

      break;

    case mxoQOS:
  
      {
      mbitmap_t tmpQAL;

      if (MQOSListBMFromString(Parent->Name,&tmpQAL,NULL,mAdd) == FAILURE)
        {
        bmclear(&tmpQAL);
        break;
        }
        
      bmor(&F->QAL,&tmpQAL);
      }

      break;

    case mxoAcct:

      MULLAdd(&F->AAL,Parent->Name,Parent->TData->O,NULL,NULL);  /* no free routine */

      break;

    default:

      /* NO-OP */

      break;
    }

/* Taken out for RT3103 */
/*
  if (MSched.FSTreeACLPolicy == mfsapParent)
    {
    return(SUCCESS);
    }
*/

  switch (TData->OType)
    {
    case mxoGroup:
    case mxoUser:
    case mxoAcct:

      F = &((mgcred_t *)TData->O)->F;

      break;

    case mxoClass:

      F = &((mclass_t *)TData->O)->F;

      break;

    case mxoQOS:

      F = &((mqos_t *)TData->O)->F;

      break;

    default:

      F = NULL;

      break;
    }

  if (F != NULL)
    {
    tmpL = TData->F->AAL;

    if ((F->ADef == NULL) && (tmpL != NULL))
      F->ADef = (mgcred_t *)tmpL->Ptr;

    while (tmpL != NULL)
      {
      MULLAdd(&F->AAL,tmpL->Name,tmpL->Ptr,NULL,NULL);  /* no free routine */

      tmpL = tmpL->Next;
      }

    tmpL = TData->F->GAL;

    if ((F->GDef == NULL) && (tmpL != NULL))
      F->GDef = (mgcred_t *)tmpL->Ptr;

    while (tmpL != NULL)
      {
      MULLAdd(&F->GAL,tmpL->Name,tmpL->Ptr,NULL,NULL);  /* no free routine */

      tmpL = tmpL->Next;
      }

    tmpL = TData->F->CAL;

    if ((F->CDef == NULL) && (tmpL != NULL))
      F->CDef = (mclass_t *)tmpL->Ptr;

    while (tmpL != NULL)
      {
      MULLAdd(&F->CAL,tmpL->Name,tmpL->Ptr,NULL,NULL);  /* no free routine */

      tmpL = tmpL->Next;
      }

    bmor(&F->QAL,&TData->F->QAL);

    if ((F->QDef[0] == NULL) && (tmpL != NULL))
      F->QDef[0] = (mqos_t *)tmpL->Ptr;
    }  /* END if (F != NULL) */

  return(SUCCESS);
  }  /* END MFSTreeBuildCredAccess() */





/**
 * Show a fairshare tree based on flags.
 *
 * @see MFSTreeShow()
 *
 * @param Tree
 * @param Flags
 * @param String
 * @param Verbose
 */

int MFSShowTreeWithFlags(

  mxml_t     *Tree, 
  mbitmap_t  *Flags, 
  mstring_t  *String,
  mbool_t     Verbose)

  {
  mxml_t *ShareTree;

  if (Verbose == TRUE)
    {
    MStringAppendF(String,"%-*.*s      Usage    Target             %s   (FSFACTOR)\n",MSched.FSTreeDepth * MFSTREE_TREESPACE,MSched.FSTreeDepth * MFSTREE_TREESPACE,"Name",bmisset(Flags,mcmVerbose2) ? "   FSCAP " : "");
    MStringAppendF(String,"%-*.*s      -----    ------             %s ------------\n",MSched.FSTreeDepth * MFSTREE_TREESPACE,MSched.FSTreeDepth * MFSTREE_TREESPACE,"----",bmisset(Flags,mcmVerbose2) ? "   ----- " : "");
    }
  else
    {
    MStringAppendF(String,"%-*.*s      Usage    Target                (FSFACTOR)\n",MSched.FSTreeDepth * MFSTREE_TREESPACE,MSched.FSTreeDepth * MFSTREE_TREESPACE,"Name");
    MStringAppendF(String,"%-*.*s      -----    ------              ------------\n",MSched.FSTreeDepth * MFSTREE_TREESPACE,MSched.FSTreeDepth * MFSTREE_TREESPACE,"----");
    }

  /* NOTE: If sorting is requested, copy the XML Fair share tree and perform an in-place sort on it,
   *       and print that tree instead 
   */

  if (bmisset(Flags,mcmSort)) 
    {
    MXMLDupE(Tree,&ShareTree);
    MXMLShallowCopyXD(Tree,ShareTree);
    MXMLSort(ShareTree,(int(*)(const void *,const void *))MXMLCompareByName);
    }
  else 
    {
    ShareTree = Tree;
    }

  MFSTreeShow(
    ShareTree,
    NULL,
    Flags,      /* flagbm */
    0,          /* depth */
    String);

  if (bmisset(Flags,mcmSort))
    {
    MXMLDestroyE(&ShareTree);
    }

  return(SUCCESS);
  }  /* END MFSShowTreeWithFlags() */
  
  
  
  
/**
 * Checks a fairshare tree element to see if it matches the 
 * requested object type and object name. If so it resets the 
 * fairshare usage for that element, sets the boolean to 
 * indicate it was reset and returns. If this element does not 
 * match, it recursively calls itself to walk through each 
 * element of the tree until a match is found or all elements in 
 * the tree are checked. 
 *
 * @param E     (I) fairshare tree element
 * @param OType (I) fairshare tree object type 
 * @param OName (I) fairshare tree object name 
 * @param Reset (O) reset boolean set if usage reset 
 *              
 */
  
  
int MFSCredResetElement(

  mxml_t               *E,         /* I */
  enum MXMLOTypeEnum    OType,     /* I */
  char                 *OName,     /* I */
  mbool_t              *Reset)     /* O */

  {
  int cindex;

  if ((E == NULL) || (E->TData == NULL))
    {
    return(FAILURE);
    }

  if ((E->Type == (int)OType) && (!strcmp(E->Name,OName)))
    {
    mfst_t *FStats = E->TData;
  
    if (FStats->F != NULL)
      {
      MDB(3,fSCHED) MLog("INFO:    reset %s usage %f to 0\n",
        E->Name,
        FStats->F->FSUsage[0]);

      FStats->F->FSUsage[0] = 0;

      /* we found the requested element and reset the usage so set the boolean so that we quit looking */

      *Reset = TRUE;

      return(SUCCESS);
      }  
    }

  for (cindex = 0;cindex < E->CCount;cindex++)
    {
    /* recursively traverse the tree elements */

    MFSCredResetElement(E->C[cindex],OType,OName,Reset);

    /* if reset is TRUE then specified object was found in the specified partition and the fairshare usage was set to zero
       so no need to continue traversing the tree */

    if (*Reset == TRUE)
      return(SUCCESS);

    }  /* END for (cindex) */

  return(SUCCESS);
  } /* END MFSCredResetElement() */



/**
 * Reset the fairshare usage value to 0 for the specified object 
 * with the specified partition 
 *
 * @param OType (I) fairshare tree object type (user, acct ,...)
 * @param OName (I) fairshare tree object name
 * @param PName (I) partition
 */


int MFSCredReset(

  enum MXMLOTypeEnum    OType,     /* I */
  char                 *OName,     /* I */
  char                 *PName)     /* I */

  {
  mpar_t *P;
  mbool_t Reset = FALSE;

  int pindex;

  for (pindex = 0;pindex < MSched.M[mxoPar];pindex++)
    {
    P = &MPar[pindex];

    if (P->Name[0] == '\0')
      break;

    if (P->Name[0] == '\1')
      continue;

    /* only check specified partition */

    if (strcmp(PName,P->Name))
      continue;

    if (P->FSC.ShareTree == NULL)
      continue;

    MFSCredResetElement(P->FSC.ShareTree,OType,OName,&Reset);
    }  /* END for (pindex) */

  if (Reset == FALSE)
    {
    return(FAILURE);
    }

  return(SUCCESS);
  } /* END MFSCredReset() */


  
/**
 ** Checks a fairshare tree element to see if it matches the
 ** requested object type and object name. If so it sets the
 ** boolean to indicate it was found and returns. If this element
 ** does not match, it recursively calls itself to walk through each
 ** element of the tree until a match is found or all elements in
 ** the tree are checked.
 **
 ** @param E     (I) fairshare tree element
 ** @param OType (I) fairshare tree object type
 ** @param OName (I) fairshare tree object name
 ** @param Depth (I) number of levels below current level to search (-1 == infinity)
 ** @param Found (O) found boolean set if element found
 **            
 **/
 
 
mxml_t *MFSCredFindElement(
    mxml_t               *E,         /* I */
    enum MXMLOTypeEnum    OType,     /* I */
    char                 *OName,     /* I */
    int                   Depth,     /* I */
    mbool_t              *Found)     /* O */

  {
  int cindex;
  mxml_t *Elt;

  if ((E == NULL) || (E->TData == NULL) || (Found == NULL) || (OName == NULL))
    {
    return(NULL);
    }

  *Found = FALSE;

  /* if we found a matching object type and object name then we are done */

  if ((E->Type == (int)OType) && (!strcmp(E->Name,OName)))
    {
    *Found = TRUE;

    return(E);
    }

  if(Depth == 0) 
    {
    /* no more levels to search, return immediately */

    return(NULL);
    }

  if(Depth > 0) 
    {
    /* a level count was given, decrement for the next level */

    Depth -= 1;
    }

  for (cindex = 0;cindex < E->CCount;cindex++)
    {
    /* recursively traverse the tree elements */

    Elt = MFSCredFindElement(E->C[cindex],OType,OName,Depth,Found);

    /* specified object was found in the specified partition */

    if (*Found == TRUE)
      return(Elt);

    }  /* END for (cindex) */

  return(NULL);
  } /* END MFSCredFindElement() */





/**
 ** Normalizes the number of shares in a fairshare tree.  Upon completion,
 ** the normalized shares for each set of 'sibling' nodes should sum up to
 ** the number of normalized shares the parent has.
 **
 ** @param E                (I) the fairshare tree element
 ** @param ParentNormShares (I) normalized shares in the parent
 ** @param SiblingShares    (I) un-normalized share total in the siblings
 **/

int MFSTreeShareNormalize(

  mxml_t *E, 
  double  ParentNormShares, 
  double  SiblingShares)

  {
  int cindex;
  mfst_t *TObject = E->TData;

  TObject->CShares = 0;
  TObject->NShares = ((ParentNormShares * TObject->Shares)/SiblingShares);

  /* recalculate total number of shares among direct children */
  for(cindex = 0; cindex < E->CCount; cindex++)
    {
    TObject->CShares += E->C[cindex]->TData->Shares;
    }

  /* recursively call on all children */
  for(cindex = 0; cindex < E->CCount; cindex++)
    {
    MFSTreeShareNormalize(E->C[cindex],TObject->NShares,TObject->CShares);
    }

  return(SUCCESS);

  } /* END MFSTreeShareNormalize() */

/**
 * Finds the full QOS bitmap from the given element down to the corresponding
 * user under the given account.  This will find multiple instances of the user
 * as long as they are all under subaccounts of the correct account.  Returns
 * success if a match is found.
 *
 * @param E            (I) the fairshare tree element
 * @param AccountName  (I) the name of the account to search for
 * @param UserName     (I) the name of the user to search for
 * @param FoundAccount (I) have we found the correct account yet?
 * @param QAL          (O) QOS bitmap for the matching account/user
 */

int MFSTreeGetQOSBitmap(

  mxml_t        *E, 
  char          *AccountName, 
  char          *UserName, 
  mbool_t        FoundAccount, 
  mbitmap_t     *QAL)

  {
  int cindex;
  mbool_t FoundUser = FALSE;

  if (!FoundAccount && (E->Type == mxoAcct) && !strcmp(E->Name,AccountName))
    {
    /* found the correct account, now we start looking for the correct user */

    FoundAccount = TRUE;
    }
  else if (FoundAccount && (E->Type == mxoUser) && !strcmp(E->Name,UserName))
    {
    /* found the correct user, union in the qlist and return success */

    bmor(QAL,&E->TData->F->QAL);

    return(SUCCESS);
    }

  /* if this is a user, it is not the correct user, return failure */

  if(E->Type == mxoUser)
    {
    return(FAILURE);
    }

  /* we still haven't found the correct user, recursively check all children */

  for (cindex = 0; cindex < E->CCount; cindex++)
    {
    if (MFSTreeGetQOSBitmap(E->C[cindex],AccountName,UserName,FoundAccount,QAL) == SUCCESS)
        {
        /* this child found a match */

        FoundUser = TRUE;
        }
    }

  /* if this node is an ancestor of a matching node, union in the qlist and return success */

  if (FoundUser)
    {
    bmor(QAL,&E->TData->F->QAL);

    return(SUCCESS);
    }

  return(FAILURE);
  }/* END MFSTreeGetQOSBitmap() */

/**
 * Returns TRUE if U is a manager of account tmpA or any of its parent accounts.
 * NOTE: This function is recursive
 *
 * @param ReqU  (I) User to check manager status
 * @param E     (I) Element to start at
 * @param U     (I) User credential to check
 * @param A     (I) Account credential to check
 * @param Found (O) Whether the user/account has been found yet
 */

mbool_t MFSIsAcctManager(
    mgcred_t *ReqU,     /* I */
    mxml_t   *E,        /* I */
    mgcred_t *U,        /* I */
    mgcred_t *A,        /* I */
    mbool_t  *Found)    /* O */
  {
  int cindex;

  if ((E == NULL) || (A == NULL))
    return (FALSE);

  if ((E->Type == mxoAcct) && (!strcmp(A->Name,E->Name)) && (U != NULL))
    {
    /* found the right account, now check if the user is here */

    if (MFSCredFindElement(E,mxoUser,U->Name,-1,Found) == NULL)
      {
      /* user doesn't have access to this account */

      return (FALSE);
      }
    *Found = TRUE;
    }

  /* recursively search children until we find the correct account */

  for (cindex = 0; cindex < E->CCount; cindex++)
    {
    if (*Found == TRUE)
      break;

    if (MFSIsAcctManager(ReqU,E->C[cindex],U,A,Found) == TRUE)
      return (TRUE);
    }

  if ((E->Type == mxoAcct) && (*Found == TRUE))
    {
    int mindex;
    mgcred_t *currA;

    /* this is (or is a parent of) the correct account */

    /* find Account that matches this element */

    if (MAcctFind(E->Name,&currA))
      {
      /* check if ReqU is a manager of this account */

      for (mindex = 0; mindex < MMAX_CREDMANAGER; mindex++)
        {
        if (currA->F.ManagerU[mindex] == ReqU)
          return (TRUE);
        } /* END for mindex */
      } /* END if MAcctFind */
    } /* END if ((E->Type == mxoAcct) && (*Found == TRUE)) */
  
  return (FALSE);
  }/* END MFSIsAcctManager() */




/**
 * Reformats old style XML Fairshare Tree to new the new style (e.g. tidy xml) with line feeds 
 * and removal of old style single quotes and beginning and end of the buffer.
 *
 * @param FSTreeXMLBuf (I)
 */

char *MFSTreeReformatXML(

  char *FSTreeXMLBuf)  /* I (buffer contents modified) */

  {

  char *StartFSTree = NULL;
  char *StartXML = NULL;
  char *EndXML = NULL;

  if (((StartFSTree = MUStrStr(FSTreeXMLBuf,"FSTREE[",0,FALSE,FALSE)) != NULL) &&
      ((StartXML = MUStrStr(StartFSTree,"<fstree>",0,FALSE,FALSE)) != NULL) &&
      ((EndXML = MUStrStr(StartXML,"</fstree>",0,FALSE,FALSE)) != NULL))
    {
    char *CPtr = StartFSTree;

    /* the tree is in XML format - we now support a new format (e.g. tidy xml) so do some
       processing to handle tree if it is in the old format (single quotes) */

    EndXML += strlen("</fstree>"); /* go to the end of the XML string*/

    *EndXML = '\0'; /* terminate the tree config xml string - no need to do any FS tree processing beyond the end of the xml string */

    /* We used to require a "'" character at the start of the XML and another at the end of the xml, but with the new format
       we do not want them. We just nuked the trailing single quote if there was one so go ahead and remove the starting quote if it exists. */

    StartXML--; /* move start pointer to the character just before the first '<'*/

    if (*StartXML == '\'')
      *StartXML = *EndXML = ' ';

    /* newlines and tabs (allowed in our new format) are not supported in the FS tree load processing routine so replace them with spaces */

    while ((CPtr = strpbrk(CPtr,"\n\t")) != NULL)
      *CPtr++ = ' ';
    } /* END if StartFSTree ...*/

  return(StartFSTree);
  } /* END MFSTreeReformatXML() */

/**
 * Searches the Fairshare tree for a job's qdef.  It starts at the leaf node (user node)
 * and walks up the tree to find the 'nearest' QDef for the Partition/Account/User
 * combination.
 *
 * @param J (I) Desired job
 * @param P (I) Desired partition
 * @param Q (O)
 */

int MFSFindQDef(

  mjob_t  *J,
  mpar_t  *P,
  mqos_t **Q) /* O */

  {
  mxml_t *E;

  if ((J == NULL) || (P == NULL) || (Q == NULL))
    return(FAILURE);

  if (J->FairShareTree == NULL)
    return(FAILURE);

  /* E points to the user node for this job in the fairshare tree 
     under the requested account and given partition. */

  E = J->FairShareTree[P->Index];

  if (E == NULL)
    return(FAILURE);

  *Q = NULL;

  /* walk up the tree looking for a QDef */

  while (E != NULL)
    {
    mfst_t *TData = E->TData;

    if (TData == NULL || TData->F == NULL)
      break;

    *Q = TData->F->QDef[0];

    if (*Q != NULL)
      return(SUCCESS);

    E = TData->Parent;
    }

  return(FAILURE);

  } /* END MFSFindQDef() */

/* END MFS.c */
