/* HEADER */

      
/**
 * @file MCredShowAttrs.c
 *
 * Contains: MCredShowAttrs
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 *
 *
 * @param P (I)
 * @param PlIndex (I)
 * @param PtIndex (I)
 * @param ShowUsage (I) - display current usage of limited resource in format U:S[,H]
 */

char *__MCredShowOLimit(

  mpu_t    *P,         /* I */
  long      PlIndex,   /* I */
  long      PtIndex,   /* I */
  mbool_t   ShowUsage) /* I - display current usage of limited resource in format U:S[,H] */

  {
  static char Line[MMAX_NAME];

  if (P == NULL)
    {
    return(NULL);
    }

  if ((P->HLimit[PlIndex][PtIndex] == 0) &&
      (P->SLimit[PlIndex][PtIndex] == 0))
    {
    return(NULL);
    }

  if (ShowUsage == TRUE)
    {
    sprintf(Line,"%lu:",
      P->Usage[PlIndex][PtIndex]);
    }
  else
    {
    Line[0] = '\0';
    }

  if ((P->SLimit[PlIndex][PtIndex] == 0) || 
      (P->SLimit[PlIndex][PtIndex] == P->HLimit[PlIndex][PtIndex]))
    {
    sprintf(Line,"%s%ld",
      Line,
      P->HLimit[PlIndex][PtIndex]);
    }
  else
    {
    sprintf(Line,"%s%ld,%ld",
      Line,
      P->SLimit[PlIndex][PtIndex],
      P->HLimit[PlIndex][PtIndex]);
    }

  return(Line);
  }  /* END __MCredShowOLimit() */




/**
 *
 *
 * @param P (I)
 * @param PlIndex (I)
 * @param PtIndex (I)
 * @param ShowUsage (I) - display current usage of limited resource in format U:S[,H]
 */

char *MCredShowLimit(

  mpu_t    *P,         /* I */
  long      PlIndex,   /* I */
  long      PtIndex,   /* I */
  mbool_t   ShowUsage) /* I - display current usage of limited resource in format U:S[,H] */

  {
  static char Line[MMAX_NAME];

  if (P == NULL)
    {
    return(NULL);
    }

  if ((P->HLimit[PlIndex][PtIndex] < 0) &&
      (P->SLimit[PlIndex][PtIndex] < 0))
    {
    return(NULL);
    }

  if (ShowUsage == TRUE)
    {
    sprintf(Line,"%lu:",
      P->Usage[PlIndex][PtIndex]);
    }
  else
    {
    Line[0] = '\0';
    }

  if ((P->SLimit[PlIndex][PtIndex] < 0) || 
      (P->SLimit[PlIndex][PtIndex] == P->HLimit[PlIndex][PtIndex]))
    {
    sprintf(Line,"%s%ld",
      Line,
      P->HLimit[PlIndex][PtIndex]);
    }
  else
    {
    sprintf(Line,"%s%ld,%ld",
      Line,
      P->SLimit[PlIndex][PtIndex],
      P->HLimit[PlIndex][PtIndex]);
    }

  return(Line);
  }  /* END MCredShowLimit() */



/**
 * Report credential attributes in human-readable string.
 *
 * @param L (I) [optional]
 * @param AP (I)
 * @param IP (I) [optional]
 * @param OAP (I) [optional]
 * @param OIP (I) [optional]
 * @param OJP (I) [optional]
 * @param RP (I) [optional]
 * @param RAP (I) [optional]
 * @param FS (I) [optional,not used]
 * @param Priority (I)
 * @param Mode (I) [bitmap of enum mcs*]
 */

char *MCredShowAttrs(

  mcredl_t  *L,
  mpu_t     *AP,
  mpu_t     *IP,
  mpu_t     *OAP,
  mpu_t     *OIP,
  mpu_t     *OJP,
  mpu_t     *RP,
  mpu_t     *RAP,
  mfs_t     *FS,
  long       Priority,
  mbool_t    Usage,
  mbool_t    Limits,
  char      *Line)

  {
  mpu_t *JP = NULL;  /* NOTE:  not yet supported */

  char *ptr;

  char *BPtr;
  int   BSpace;

  int pindex;

  mbool_t ShowUsage;

  MUSNInit(&BPtr,&BSpace,Line,MMAX_LINE);

  if (Usage == TRUE)
    ShowUsage = TRUE;
  else
    ShowUsage = FALSE;

  for (pindex = mcaPriority;pindex < mcaLAST;pindex++)
    {
    switch (pindex)
      {
      case mcaPriority:

        if (Priority != 0)
          {
          MUSNPrintF(&BPtr,&BSpace," %s=%ld",
            MCredAttr[pindex],
            Priority);
          }

        break;

      case mcaMaxGRes:

        if ((Limits == TRUE) &&
           ((ptr = MCredShowLimit(AP,mptMaxGRes,0,ShowUsage)) != NULL))
          {
          MUSNPrintF(&BPtr,&BSpace," %s=%s",
            MCredAttr[pindex],
            ptr);
          }

        break;

      case mcaMaxJob:

        if ((Limits == TRUE))
          {
          if ((ptr = MCredShowLimit(AP,mptMaxJob,0,ShowUsage)) != NULL)
            {
            MUSNPrintF(&BPtr,&BSpace," %s=%s",
              MCredAttr[pindex],
              ptr);
            }

          if (L != NULL)
            {
            char tmpLine[MMAX_LINE];

            if (L->UserActivePolicy != NULL)
              {
              MCredShow2DLimit(
                L,
                mlActive,
                mptMaxJob,
                mcaMaxJob,
                mxoUser,
                ShowUsage,
                tmpLine,    /* O */
                sizeof(tmpLine));

              MUSNPrintF(&BPtr,&BSpace," %s",
                tmpLine);
              }

            if (L->AcctActivePolicy != NULL)
              {
              MCredShow2DLimit(
                L,
                mlActive,
                mptMaxJob,
                mcaMaxJob,
                mxoAcct,
                ShowUsage,
                tmpLine,    /* O */
                sizeof(tmpLine));

              MUSNPrintF(&BPtr,&BSpace," %s",
                tmpLine);
              }

            if (L->QOSActivePolicy != NULL)
              {
              MCredShow2DLimit(
                L,
                mlActive,
                mptMaxJob,
                mcaMaxJob,
                mxoQOS,
                ShowUsage,
                tmpLine,    /* O */
                sizeof(tmpLine));

              MUSNPrintF(&BPtr,&BSpace," %s",
                tmpLine);
              }

            if (L->ClassActivePolicy != NULL)
              {
              MCredShow2DLimit(
                L,
                mlActive,
                mptMaxJob,
                mcaMaxJob,
                mxoClass,
                ShowUsage,
                tmpLine,    /* O */
                sizeof(tmpLine));

              MUSNPrintF(&BPtr,&BSpace," %s",
                tmpLine);
              }

            if (L->GroupActivePolicy != NULL)
              {
              MCredShow2DLimit(
                L,
                mlActive,
                mptMaxJob,
                mcaMaxJob,
                mxoGroup,
                ShowUsage,
                tmpLine,    /* O */
                sizeof(tmpLine));

              MUSNPrintF(&BPtr,&BSpace," %s",
                tmpLine);
              }
            }  /* if (L != NULL) */
          }

        break;

      case mcaMaxNode:

        if ((Limits == TRUE) &&
           ((ptr = MCredShowLimit(AP,mptMaxNode,0,ShowUsage)) != NULL))
          {
          MUSNPrintF(&BPtr,&BSpace," %s=%s",
            MCredAttr[pindex],
            ptr);
          }

        break;

      case mcaMaxPE:

        if ((Limits == TRUE) &&
           ((ptr = MCredShowLimit(AP,mptMaxPE,0,ShowUsage)) != NULL))
          {
          MUSNPrintF(&BPtr,&BSpace," %s=%s",
            MCredAttr[pindex],
            ptr);
          }

        break;

      case mcaMaxProc:

        if ((Limits == TRUE))
          {
          if ((ptr = MCredShowLimit(AP,mptMaxProc,0,ShowUsage)) != NULL)
            {
            MUSNPrintF(&BPtr,&BSpace," %s=%s",
              MCredAttr[pindex],
              ptr);
            }

          if (L != NULL)
            {
            char tmpLine[MMAX_LINE];

            if (L->UserActivePolicy != NULL)
              {
              MCredShow2DLimit(
                L,
                mlActive,
                mptMaxProc,
                mcaMaxProc,
                mxoUser,
                ShowUsage,
                tmpLine,    /* O */
                sizeof(tmpLine));

              MUSNPrintF(&BPtr,&BSpace," %s",
                tmpLine);
              }

            if (L->AcctActivePolicy != NULL)
              {
              MCredShow2DLimit(
                L,
                mlActive,
                mptMaxProc,
                mcaMaxProc,
                mxoAcct,
                ShowUsage,
                tmpLine,    /* O */
                sizeof(tmpLine));

              MUSNPrintF(&BPtr,&BSpace," %s",
                tmpLine);
              }

            if (L->QOSActivePolicy != NULL)
              {
              MCredShow2DLimit(
                L,
                mlActive,
                mptMaxProc,
                mcaMaxProc,
                mxoQOS,
                ShowUsage,
                tmpLine,    /* O */
                sizeof(tmpLine));

              MUSNPrintF(&BPtr,&BSpace," %s",
                tmpLine);
              }

            if (L->ClassActivePolicy != NULL)
              {
              MCredShow2DLimit(
                L,
                mlActive,
                mptMaxProc,
                mcaMaxProc,
                mxoClass,
                ShowUsage,
                tmpLine,    /* O */
                sizeof(tmpLine));

              MUSNPrintF(&BPtr,&BSpace," %s",
                tmpLine);
              }

            if (L->GroupActivePolicy != NULL)
              {
              MCredShow2DLimit(
                L,
                mlActive,
                mptMaxProc,
                mcaMaxProc,
                mxoGroup,
                ShowUsage,
                tmpLine,    /* O */
                sizeof(tmpLine));

              MUSNPrintF(&BPtr,&BSpace," %s",
                tmpLine);
              }
            }  /* if (L != NULL) */

          }

        break;

      case mcaMinProc:

        if ((Limits == TRUE) &&
          ((ptr = MCredShowLimit(JP,mptMinProc,0,ShowUsage)) != NULL))
          {
          MUSNPrintF(&BPtr,&BSpace," %s=%s",
            MCredAttr[pindex],
            ptr);
          }

        break;

      case mcaMaxPS:

        if ((Limits == TRUE))
          {
          if ((ptr = MCredShowLimit(AP,mptMaxPS,0,ShowUsage)) != NULL)
            {
            MUSNPrintF(&BPtr,&BSpace," %s=%s",
              MCredAttr[pindex],
              ptr);
            }

          if (L != NULL)
            {
            char tmpLine[MMAX_LINE];

            if (L->UserActivePolicy != NULL)
              {
              MCredShow2DLimit(
                L,
                mlActive,
                mptMaxPS,
                mcaMaxPS,
                mxoUser,
                ShowUsage,
                tmpLine,    /* O */
                sizeof(tmpLine));

              MUSNPrintF(&BPtr,&BSpace," %s",
                tmpLine);
              }

            if (L->AcctActivePolicy != NULL)
              {
              MCredShow2DLimit(
                L,
                mlActive,
                mptMaxPS,
                mcaMaxPS,
                mxoAcct,
                ShowUsage,
                tmpLine,    /* O */
                sizeof(tmpLine));

              MUSNPrintF(&BPtr,&BSpace," %s",
                tmpLine);
              }

            if (L->QOSActivePolicy != NULL)
              {
              MCredShow2DLimit(
                L,
                mlActive,
                mptMaxPS,
                mcaMaxPS,
                mxoQOS,
                ShowUsage,
                tmpLine,    /* O */
                sizeof(tmpLine));

              MUSNPrintF(&BPtr,&BSpace," %s",
                tmpLine);
              }

            if (L->ClassActivePolicy != NULL)
              {
              MCredShow2DLimit(
                L,
                mlActive,
                mptMaxPS,
                mcaMaxPS,
                mxoClass,
                ShowUsage,
                tmpLine,    /* O */
                sizeof(tmpLine));

              MUSNPrintF(&BPtr,&BSpace," %s",
                tmpLine);
              }

            if (L->GroupActivePolicy != NULL)
              {
              MCredShow2DLimit(
                L,
                mlActive,
                mptMaxPS,
                mcaMaxPS,
                mxoGroup,
                ShowUsage,
                tmpLine,    /* O */
                sizeof(tmpLine));

              MUSNPrintF(&BPtr,&BSpace," %s",
                tmpLine);
              }
            }  /* if (L != NULL) */
          }

        break;

      case mcaMaxWC:

        if ((Limits == TRUE) &&
           ((ptr = MCredShowLimit(AP,mptMaxWC,0,ShowUsage)) != NULL))
          {
          MUSNPrintF(&BPtr,&BSpace," %s=%s",
            MCredAttr[pindex],
            ptr);
          }

        break;

      case mcaMaxMem:

        if ((Limits == TRUE) &&
           ((ptr = MCredShowLimit(AP,mptMaxMem,0,ShowUsage)) != NULL))
          {
          MUSNPrintF(&BPtr,&BSpace," %s=%s",
            MCredAttr[pindex],
            ptr);
          }

        break;

      case mcaMaxIJob:

        if ((Limits == TRUE))
          {
          if ((ptr = MCredShowLimit(IP,mptMaxJob,0,ShowUsage)) != NULL)
            {
            MUSNPrintF(&BPtr,&BSpace," %s=%s",
              MCredAttr[pindex],
              ptr);
            }
   
          if (L != NULL)
            {
            char tmpLine[MMAX_LINE];

            if (L->UserIdlePolicy != NULL)
              {
              MCredShow2DLimit(
                L,
                mlIdle,
                mptMaxJob,
                mcaMaxIJob,
                mxoUser,
                ShowUsage,
                tmpLine,    /* O */
                sizeof(tmpLine));

              MUSNPrintF(&BPtr,&BSpace," %s",
                tmpLine);
              }

            if (L->AcctIdlePolicy != NULL)
              {
              MCredShow2DLimit(
                L,
                mlIdle,
                mptMaxJob,
                mcaMaxIJob,
                mxoAcct,
                ShowUsage,
                tmpLine,    /* O */
                sizeof(tmpLine));

              MUSNPrintF(&BPtr,&BSpace," %s",
                tmpLine);
              }

            if (L->QOSIdlePolicy != NULL)
              {
              MCredShow2DLimit(
                L,
                mlIdle,
                mptMaxJob,
                mcaMaxIJob,
                mxoQOS,
                ShowUsage,
                tmpLine,    /* O */
                sizeof(tmpLine));

              MUSNPrintF(&BPtr,&BSpace," %s",
                tmpLine);
              }

            if (L->ClassIdlePolicy != NULL)
              {
              MCredShow2DLimit(
                L,
                mlIdle,
                mptMaxJob,
                mcaMaxIJob,
                mxoClass,
                ShowUsage,
                tmpLine,    /* O */
                sizeof(tmpLine));

              MUSNPrintF(&BPtr,&BSpace," %s",
                tmpLine);
              }

            if (L->GroupIdlePolicy != NULL)
              {
              MCredShow2DLimit(
                L,
                mlIdle,
                mptMaxJob,
                mcaMaxIJob,
                mxoGroup,
                ShowUsage,
                tmpLine,    /* O */
                sizeof(tmpLine));

              MUSNPrintF(&BPtr,&BSpace," %s",
                tmpLine);
              }
            }  /* if (L != NULL) */
          }    /* END if ((Limits == TRUE)) */

        break;

      case mcaMaxINode:

        if ((Limits == TRUE) &&
           ((ptr = MCredShowLimit(IP,mptMaxNode,0,ShowUsage)) != NULL))
          {
          MUSNPrintF(&BPtr,&BSpace," %s=%s",
            MCredAttr[pindex],
            ptr);
          }

        break;

      case mcaMaxIPE:

        if ((Limits == TRUE) &&
           ((ptr = MCredShowLimit(IP,mptMaxPE,0,ShowUsage)) != NULL))
          {
          MUSNPrintF(&BPtr,&BSpace," %s=%s",
            MCredAttr[pindex],
            ptr);
          }

        break;

      case mcaMaxIProc:

        if ((Limits == TRUE))
          {
          if ((ptr = MCredShowLimit(IP,mptMaxProc,0,ShowUsage)) != NULL)
            {
            MUSNPrintF(&BPtr,&BSpace," %s=%s",
              MCredAttr[pindex],
              ptr);
            }

          if (L != NULL)
            {
            char tmpLine[MMAX_LINE];

            if (L->UserIdlePolicy != NULL)
              {
              MCredShow2DLimit(
                L,
                mlIdle,
                mptMaxProc,
                mcaMaxIProc,
                mxoUser,
                ShowUsage,
                tmpLine,    /* O */
                sizeof(tmpLine));

              MUSNPrintF(&BPtr,&BSpace," %s",
                tmpLine);
              }

            if (L->AcctIdlePolicy != NULL)
              {
              MCredShow2DLimit(
                L,
                mlIdle,
                mptMaxProc,
                mcaMaxIProc,
                mxoAcct,
                ShowUsage,
                tmpLine,    /* O */
                sizeof(tmpLine));

              MUSNPrintF(&BPtr,&BSpace," %s",
                tmpLine);
              }

            if (L->QOSIdlePolicy != NULL)
              {
              MCredShow2DLimit(
                L,
                mlIdle,
                mptMaxProc,
                mcaMaxIProc,
                mxoQOS,
                ShowUsage,
                tmpLine,    /* O */
                sizeof(tmpLine));

              MUSNPrintF(&BPtr,&BSpace," %s",
                tmpLine);
              }

            if (L->ClassIdlePolicy != NULL)
              {
              MCredShow2DLimit(
                L,
                mlIdle,
                mptMaxProc,
                mcaMaxIProc,
                mxoClass,
                ShowUsage,
                tmpLine,    /* O */
                sizeof(tmpLine));

              MUSNPrintF(&BPtr,&BSpace," %s",
                tmpLine);
              }

            if (L->GroupIdlePolicy != NULL)
              {
              MCredShow2DLimit(
                L,
                mlIdle,
                mptMaxProc,
                mcaMaxIProc,
                mxoGroup,
                ShowUsage,
                tmpLine,    /* O */
                sizeof(tmpLine));

              MUSNPrintF(&BPtr,&BSpace," %s",
                tmpLine);
              }
            }  /* if (L != NULL) */
          }

        break;

      case mcaMaxIPS:

        if ((Limits == TRUE) &&
           ((ptr = MCredShowLimit(IP,mptMaxPS,0,ShowUsage)) != NULL))
          {
          MUSNPrintF(&BPtr,&BSpace," %s=%s",
            MCredAttr[pindex],
            ptr);
          }

        break;

      case mcaMaxIWC:

        if ((Limits == TRUE) &&
           ((ptr = MCredShowLimit(IP,mptMaxWC,0,ShowUsage)) != NULL))
          {
          MUSNPrintF(&BPtr,&BSpace," %s=%s",
            MCredAttr[pindex],
            ptr);
          }

        break;

      case mcaMaxIMem:

        if ((Limits == TRUE) &&
           ((ptr = MCredShowLimit(IP,mptMaxMem,0,ShowUsage)) != NULL))
          {
          MUSNPrintF(&BPtr,&BSpace," %s=%s",
            MCredAttr[pindex],
            ptr);
          }

        break;

      case mcaOMaxJob:

        if ((Limits == TRUE) &&
           ((ptr = __MCredShowOLimit(OAP,mptMaxJob,0,ShowUsage)) != NULL))
          {
          MUSNPrintF(&BPtr,&BSpace," %s=%s",
            MCredAttr[pindex],
            ptr);
          }

        break;

      case mcaOMaxNode:

        if ((Limits == TRUE) &&
           ((ptr = __MCredShowOLimit(OAP,mptMaxNode,0,ShowUsage)) != NULL))
          {
          MUSNPrintF(&BPtr,&BSpace," %s=%s",
            MCredAttr[pindex],
            ptr);
          }

        break;

      case mcaOMaxPE:

        if ((Limits == TRUE) &&
           ((ptr = __MCredShowOLimit(OAP,mptMaxPE,0,ShowUsage)) != NULL))
          {
          MUSNPrintF(&BPtr,&BSpace," %s=%s",
            MCredAttr[pindex],
            ptr);
          }

        break;

      case mcaOMaxProc:

        if ((Limits == TRUE) &&
           ((ptr = __MCredShowOLimit(OAP,mptMaxProc,0,ShowUsage)) != NULL))
          {
          MUSNPrintF(&BPtr,&BSpace," %s=%s",
            MCredAttr[pindex],
            ptr);
          }

        break;

      case mcaOMaxPS:

        if ((Limits == TRUE) &&
           ((ptr = __MCredShowOLimit(OAP,mptMaxPS,0,ShowUsage)) != NULL))
          {
          MUSNPrintF(&BPtr,&BSpace," %s=%s",
            MCredAttr[pindex],
            ptr);
          }

        break;

      case mcaOMaxWC:

        if ((Limits == TRUE) &&
           ((ptr = __MCredShowOLimit(OAP,mptMaxWC,0,ShowUsage)) != NULL))
          {
          MUSNPrintF(&BPtr,&BSpace," %s=%s",
            MCredAttr[pindex],
            ptr);
          }

        break;

      case mcaOMaxMem:

        if ((Limits == TRUE) &&
           ((ptr = __MCredShowOLimit(OAP,mptMaxMem,0,ShowUsage)) != NULL))
          {
          MUSNPrintF(&BPtr,&BSpace," %s=%s",
            MCredAttr[pindex],
            ptr);
          }

        break;

      case mcaOMaxIJob:

        if ((Limits == TRUE) &&
           ((ptr = __MCredShowOLimit(OIP,mptMaxJob,0,ShowUsage)) != NULL))
          {
          MUSNPrintF(&BPtr,&BSpace," %s=%s",
            MCredAttr[pindex],
            ptr);
          }

        break;

      case mcaOMaxINode:

        if ((Limits == TRUE) &&
           ((ptr = __MCredShowOLimit(OIP,mptMaxNode,0,ShowUsage)) != NULL))
          {
          MUSNPrintF(&BPtr,&BSpace," %s=%s",
            MCredAttr[pindex],
            ptr);
          }

        break;

      case mcaOMaxIPE:

        if ((Limits == TRUE) &&
           ((ptr = __MCredShowOLimit(OIP,mptMaxPE,0,ShowUsage)) != NULL))
          {
          MUSNPrintF(&BPtr,&BSpace," %s=%s",
            MCredAttr[pindex],
            ptr);
          }

        break;

      case mcaOMaxIProc:

        if ((Limits == TRUE) &&
           ((ptr = __MCredShowOLimit(OIP,mptMaxProc,0,ShowUsage)) != NULL))
          {
          MUSNPrintF(&BPtr,&BSpace," %s=%s",
            MCredAttr[pindex],
            ptr);
          }

        break;

      case mcaOMaxIPS:

        if ((Limits == TRUE) &&
           ((ptr = __MCredShowOLimit(OIP,mptMaxPS,0,ShowUsage)) != NULL))
          {
          MUSNPrintF(&BPtr,&BSpace," %s=%s",
            MCredAttr[pindex],
            ptr);
          }

        break;

      case mcaOMaxIWC:

        if ((Limits == TRUE) &&
           ((ptr = __MCredShowOLimit(OIP,mptMaxWC,0,ShowUsage)) != NULL))
          {
          MUSNPrintF(&BPtr,&BSpace," %s=%s",
            MCredAttr[pindex],
            ptr);
          }

        break;

      case mcaOMaxIMem:

        if ((Limits == TRUE) &&
           ((ptr = __MCredShowOLimit(OIP,mptMaxMem,0,ShowUsage)) != NULL))
          {
          MUSNPrintF(&BPtr,&BSpace," %s=%s",
            MCredAttr[pindex],
            ptr);
          }

        break;

      case mcaOMaxJNode:

        if ((Limits == TRUE) &&
           ((ptr = __MCredShowOLimit(OJP,mptMaxNode,0,ShowUsage)) != NULL))
          {
          MUSNPrintF(&BPtr,&BSpace," %s=%s",
            MCredAttr[pindex],
            ptr);
          }

        break;

      case mcaOMaxJPE:

        if ((Limits == TRUE) &&
           ((ptr = __MCredShowOLimit(OJP,mptMaxPE,0,ShowUsage)) != NULL))
          {
          MUSNPrintF(&BPtr,&BSpace," %s=%s",
            MCredAttr[pindex],
            ptr);
          }

        break;

      case mcaOMaxJProc:

        if ((Limits == TRUE) &&
           ((ptr = __MCredShowOLimit(OJP,mptMaxProc,0,ShowUsage)) != NULL))
          {
          MUSNPrintF(&BPtr,&BSpace," %s=%s",
            MCredAttr[pindex],
            ptr);
          }

        break;

      case mcaOMaxJPS:

        if ((Limits == TRUE) &&
           ((ptr = __MCredShowOLimit(OJP,mptMaxPS,0,ShowUsage)) != NULL))
          {
          MUSNPrintF(&BPtr,&BSpace," %s=%s",
            MCredAttr[pindex],
            ptr);
          }

        break;

      case mcaOMaxJWC:

        if ((Limits == TRUE) &&
           ((ptr = __MCredShowOLimit(OJP,mptMaxWC,0,ShowUsage)) != NULL))
          {
          MUSNPrintF(&BPtr,&BSpace," %s=%s",
            MCredAttr[pindex],
            ptr);
          }

        break;

      case mcaOMaxJMem:

        if ((Limits == TRUE) &&
           ((ptr = __MCredShowOLimit(OJP,mptMaxMem,0,ShowUsage)) != NULL))
          {
          MUSNPrintF(&BPtr,&BSpace," %s=%s",
            MCredAttr[pindex],
            ptr);
          }

        break;

      case mcaFSCap:

        /* NYI */

        break;

      case mcaFSTarget:

        /* NYI */

        break;

      case mcaQList:

        /* NYI */

        break;

      case mcaQDef:

        /* NYI */

        break;

      case mcaPList:

        /* NYI */

        break;

      case mcaPDef:

        /* NYI */

        break;

      case mcaJobFlags:

        /* NO-OP */

        break;

      case mcaRMaxDuration:  /* per rsv reservation limits */

        if ((Limits == TRUE) &&
           ((ptr = MCredShowLimit(RP,mptMaxWC,0,ShowUsage)) != NULL))
          {
          MUSNPrintF(&BPtr,&BSpace," %s=%s",
            MCredAttr[pindex],
            ptr);
          }

        break;

      case mcaRMaxProc:

        if ((Limits == TRUE) &&
           ((ptr = MCredShowLimit(RP,mptMaxProc,0,ShowUsage)) != NULL))
          {
          MUSNPrintF(&BPtr,&BSpace," %s=%s",
            MCredAttr[pindex],
            ptr);
          }

        break;

      case mcaRMaxPS:

        if ((Limits == TRUE) &&
           ((ptr = MCredShowLimit(RP,mptMaxPS,0,ShowUsage)) != NULL))
          {
          MUSNPrintF(&BPtr,&BSpace," %s=%s",
            MCredAttr[pindex],
            ptr);
          }

        break;

      case mcaRMaxCount:     /* aggregate reservation limits */

        if ((Limits == TRUE) &&
           ((ptr = MCredShowLimit(RP,mptMaxJob,0,ShowUsage)) != NULL))
          {
          MUSNPrintF(&BPtr,&BSpace," %s=%s",
            MCredAttr[pindex],
            ptr);
          }

        break;

      case mcaRMaxTotalDuration:

        if ((Limits == TRUE) &&
           ((ptr = MCredShowLimit(RAP,mptMaxWC,0,ShowUsage)) != NULL))
          {
          MUSNPrintF(&BPtr,&BSpace," %s=%s",
            MCredAttr[pindex],
            ptr);
          }

        break;

      case mcaRMaxTotalProc:

        if ((Limits == TRUE) &&
           ((ptr = MCredShowLimit(RAP,mptMaxProc,0,ShowUsage)) != NULL))
          {
          MUSNPrintF(&BPtr,&BSpace," %s=%s",
            MCredAttr[pindex],
            ptr);
          }

        break;

      case mcaRMaxTotalPS:

        if ((Limits == TRUE) &&
           ((ptr = MCredShowLimit(RAP,mptMaxPS,0,ShowUsage)) != NULL))
          {
          MUSNPrintF(&BPtr,&BSpace," %s=%s",
            MCredAttr[pindex],
            ptr);
          }

        break;

      default:

        /* NO-OP */

        break;
      }  /* END switch (pindex) */
    }    /* END for (pindex) */

  return(Line);
  }  /* END MCredShowAttrs() */

/* END MCredShowAttrs.c */
