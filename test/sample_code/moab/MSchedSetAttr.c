/* HEADER */

      
/**
 * @file MSchedSetAttr.c
 *
 * Contains: MSchedSetAttr
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 * Set scheduler attribute.
 *
 * @see MSchedAToString() - peer
 *
 * @param S (I) [modified]
 * @param AIndex (I)
 * @param AVal (I)
 * @param Format (I)
 * @param Mode (I)
 */

int MSchedSetAttr(

  msched_t               *S,      /* I (modified) */
  enum MSchedAttrEnum     AIndex, /* I */
  void                  **AVal,   /* I */
  enum MDataFormatEnum    Format, /* I */
  enum MObjectSetModeEnum Mode)   /* I */

  {
  if ((S == NULL) || (AVal == NULL))
    {
    return(FAILURE);
    }

  switch (AIndex)
    {
    case msaChargeMetricPolicy:

      S->ChargeMetric = (enum MAMConsumptionPolicyEnum)MUGetIndexCI(
        (char *)AVal,
        MAMChargeMetricPolicy,
        FALSE,
        S->ChargeMetric);

      break;

    case msaChargeRatePolicy:

      S->ChargeRatePolicy = (enum MAMConsumptionRatePolicyEnum)MUGetIndexCI(
        (char *)AVal,
        MAMChargeRatePolicy,
        FALSE,
        S->ChargeRatePolicy);

      break;

    case msaCheckpointDir:

      MUStrCpy(S->CheckpointDir,(char *)AVal,sizeof(S->CheckpointDir));

      break;

    case msaCPVersion:

      {
      char *ptr;

      if ((ptr = strrchr((char *)AVal,'.')) != NULL)
        {
        *ptr = '\0';
        }

      strcpy(MCP.DVersion,(char *)AVal);
      }
 
      break;

    case msaEventCounter:

      S->EventCounter = (int)strtol((char *)AVal,NULL,10);

      break;

    case msaHALockCheckTime:

      S->HALockCheckTime = MUTimeFromString((char *)AVal);

      break;

    case msaHALockFile:

      MUStrCpy(S->HALockFile,(char *)AVal,sizeof(S->HALockFile));

      break;

    case msaHALockUpdateTime:

      S->HALockUpdateTime = MUTimeFromString((char *)AVal);

      break;

    case msaHomeDir:

      MUStrCpy(S->CfgDir,(char *)AVal,sizeof(S->CfgDir));

      /* append '/' if necessary */

      if (S->CfgDir[strlen(S->CfgDir) - 1] != '/')
        MUStrCat(S->CfgDir,"/",sizeof(S->CfgDir));

      /* set CWD to home directory */

      if (chdir(S->CfgDir) == -1)
        {
        fprintf(stderr,"cannot enter home directory '%s', %s\n",
          S->CfgDir,
          strerror(errno));

        MDB(0,fALL) MLog("ERROR:    cannot change directory to '%s', errno: %d (%s)\n",
          S->CfgDir,
          errno,
          strerror(errno));
        }
 
      break;

    case msaHTTPServerPort:

      S->HTTPServerPort = (int)strtol((char *)AVal,NULL,10);

      break;

    case msaIgnoreNodes:

      switch (Mode)
        {
        case mSet:
        case mAdd:
        default:

          {
          char *ptr;
          char *TokPtr = NULL;

          mlr_t *LPtr;

          int    oindex;

          if (strchr((char *)AVal,'!') != NULL)
            {
            if ((S->SpecNodes.List == NULL) &&
               ((S->SpecNodes.List = (char **)MUCalloc(1,sizeof(char *) * (MSched.M[mxoNode] + 1))) == NULL))
              {
              return(FAILURE);
              }

           
            LPtr = &S->SpecNodes;
            }
          else
            {
            if ((S->IgnoreNodes.List == NULL) &&
               ((S->IgnoreNodes.List = (char **)MUCalloc(1,sizeof(char *) * (MSched.M[mxoNode] + 1))) == NULL))
              {
              return(FAILURE);
              }

            LPtr = &S->IgnoreNodes;
            }

          if ((Mode == mSet) || (Mode == mNONE2))
            {
            oindex = 0;
            }
          else
            {
            /* locate end of list */

            for (oindex = 0;oindex < MSched.M[mxoNode];oindex++)
              {
              if (LPtr->List[oindex] == NULL)
                break;
              }

            if (oindex >= MSched.M[mxoNode])
              break;
            }

          ptr = strchr((char *)AVal,'[');

          if (ptr != NULL)
            {
            *ptr = '\0';

            MUStrDup(&LPtr->Prefix,(char *)AVal);
            LPtr->PrefixLen = strlen((char *)AVal);

            ptr++;

            ptr = MUStrTok(ptr,"[-]",&TokPtr);

            LPtr->MinIndex = (int)strtol(ptr,NULL,10);

            ptr = MUStrTok(NULL,"[-]",&TokPtr);

            LPtr->MaxIndex = (int)strtol(ptr,NULL,10);
            }
          else
            {
            ptr = MUStrTok((char *)AVal," \t\n,!:",&TokPtr);

            while (ptr != NULL)
              {
              MUStrDup(&LPtr->List[oindex],ptr);

              oindex++;

              if (oindex >= MSched.M[mxoNode])
                break;

              ptr = MUStrTok(NULL," \t\n,!:",&TokPtr);
              }  /* END while (ptr) */
            }

          LPtr->List[oindex] = NULL;
          }  /* END case */

          break;

        case mDecr:

          {
          int oindex;

          char **LPtr = S->IgnoreNodes.List;

          if (LPtr == NULL)
            break;

          for (oindex = 0;LPtr[oindex] != NULL;oindex++)
            {
            if (!strcasecmp(LPtr[oindex],(char *)AVal))
              {
              int nindex = oindex;

              while (LPtr[oindex++] != NULL)
                {
                LPtr[nindex] = LPtr[oindex];
                }

              LPtr[oindex] = NULL;
              }
            }
          }   /* END case mDecr */

          break;
        }  /* END switch (Mode) */

      break;

    case msaLastTransCommitted:

      S->LastTransCommitted = (int)strtol((char *)AVal,NULL,0);

      break;

    case msaLocalQueueIsActive:

      S->LocalQueueIsActive = MUBoolFromString((char *)AVal,FALSE);

      break;

    case msaJobMaxNodeCount:

      if (Format == mdfString)
        {
        char *ptr1;
        char *ptr2;
 
        char *TokPtr = NULL;

        /* FORMAT:  [<DEF>,]<MAX> */

        ptr1 = MUStrTok((char *)AVal,",",&TokPtr);
        ptr2 = MUStrTok(NULL,",",&TokPtr);

        if (ptr2 == NULL)
          {
          MSched.JobMaxNodeCount = strtol(ptr1,NULL,10);
          MSched.JobDefNodeCount = MSched.JobMaxNodeCount;
          }
        else
          {
          MSched.JobDefNodeCount = strtol(ptr1,NULL,10);
          MSched.JobMaxNodeCount = strtol(ptr2,NULL,10);
          }
        }

      break;

    case msaMessage:

      if (Format == mdfOther)
        {
        mmb_t *MP = (mmb_t *)AVal;

        MMBAdd(&S->MB,MP->Data,MP->Owner,MP->Type,MP->ExpireTime,MP->Priority,NULL);
        }
      else
        {
        char *mptr;

        char tmpLine[MMAX_LINE << 2];

        /* append message to message buffer */

        mptr = (char *)AVal;

        if (mptr == NULL)
          break;

        if (MUStringIsPacked(mptr) == TRUE)
          {
          MUStringUnpack(mptr,tmpLine,sizeof(tmpLine),NULL);

          mptr = tmpLine;
          }

        if (mptr[0] == '<')
          {
          /* extract XML */

          MMBFromString(mptr,&S->MB);
          }      /* END if (mptr[0] == '<') */
        else
          {
          MMBAdd(
            &S->MB,
            mptr,
            NULL,
            mmbtOther,
            MSched.Time + MCONST_DAYLEN,
            0,
            NULL);
          }
        }  /* END else if (Format == mdfOther) */

      break;

    case msaName:

      MUStrCpy(S->Name,(char *)AVal,sizeof(S->Name));

      break;

    case msaRsvCounter:

      S->RsvCounter = (int)strtol((char *)AVal,NULL,0);

      break;

    case msaRsvGroupCounter:

      S->RsvGroupCounter = (int)strtol((char *)AVal,NULL,0);

      break;

    case msaSchedLockFile:

      MUStrCpy(S->LockFile,(char *)AVal,sizeof(S->LockFile));

      break;

    case msaSpoolDir:

      MUStrCpy(S->SpoolDir,(char *)AVal,sizeof(S->SpoolDir));

      if (S->CheckpointDir[0] == '\0')
        MUStrCpy(S->CheckpointDir,(char *)AVal,sizeof(S->CheckpointDir));

      break;

    case msaStateMAdmin:

        MUStrDup(&S->StateMAdmin,(char *)AVal);
      
      break;

    case msaStateMTime:
      {
      long newTime;

      newTime = strtol((char *)AVal,NULL,10);

      /* Don't allow the state time to be older than the last, which may occur
         when restoring data from checkpoint file */

      if (newTime > S->StateMTime)
        S->StateMTime = newTime;
      }

      break;

    case msaRestartState:

      if (S->UseCPRestartState) /* set when starting moab with the "moab -s" command */
        {

        /* We only set the restart state from an attribute when we read in the checkpoint file
         * and we only use the value in the checkpoint file when moab is started with the
         * "moab -s" command.
         */

        S->RestartState = (enum MSchedStateEnum)MUGetIndexCI(
          (char *)AVal,
          MSchedState,
          FALSE,
          mssmNONE);
        }

      break;

    case msaTrigCounter:

      S->TrigCounter = (int)strtol((char *)AVal,NULL,10);

      break;

    case msaTransCounter:

      S->TransCounter = (int)strtol((char *)AVal,NULL,10);

      break;

    default:

      /* NO-OP */

      break;
    }  /* END switch (AIndex) */

  return(SUCCESS);
  }  /* END MSchedSetAttr() */
/* END MSchedSetAttr.c */
