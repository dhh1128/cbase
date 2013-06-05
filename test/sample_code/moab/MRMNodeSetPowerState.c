/* HEADER */

      
/**
 * @file MRMNodeSetPowerState.c
 *
 * Contains: MRMNodeSetPowerState
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 * Adjust node power state
 *
 * NOTE:  requires 'exec' type native RM action
 *
 * @see MRMNodeModify() - peer
 * @see MUReadPipe() - child - launch power modify action
 * @see MSysSchedulePower() - parent
 * @see MNodeSetAttr() - parent - used to modify node power via 'mnodectl'
 *
 * @param N          (I) [modified]
 * @param NL         (I) [modified]
 * @param V          (I) [optional,modified]
 * @param PowerState (I)
 * @param EMsg       (O) [optional,minsize=MMAX_LINE] 
 */

int MRMNodeSetPowerState(

  mnode_t              *N,
  mnl_t                *NL,
  mvm_t                *V,
  enum MPowerStateEnum  PowerState,
  char                 *EMsg)

  {
  char    *ptr;

  int      nindex;
  int      rc;

  mnode_t *tmpN = NULL;

  mnat_t *ND = NULL;

  mpsi_t *P = NULL;  

  char CmdString[MMAX_LINE];

  long RMTime;

  const char *FName = "MRMNodeSetPowerState";

  MDB(2,fRM) MLog("%s(%s,%s,%s,EMsg)\n",
    FName,
    (N != NULL) ? N->Name : "NULL",
    (V != NULL) ? V->VMID : "",
    MPowerState[PowerState]);

  if (EMsg != NULL)
    EMsg[0] = '\0';

  tmpN = (NL != NULL) ? MNLReturnNodeAtIndex(NL,0) : N;

  if ((tmpN == NULL) || (PowerState == mpowNONE))
    {
    return(FAILURE);
    }

  if ((MSched.ProvRM != NULL) && (MSched.ProvRM->ND.Path[mrmXNodePower] != NULL))
    {
    ND = &MSched.ProvRM->ND;
    P = MSched.ProvRM->P;
    }
  else if ((tmpN->RM != NULL) && (tmpN->RM->ND.Path[mrmXNodePower] != NULL))
    {
    ND = &tmpN->RM->ND;
    P = tmpN->RM->P;
    }

  /* perform variable-substitution in URL */

  if (ND != NULL)
    {
    MUInsertVarList(
      ND->Path[mrmXNodePower],
      NULL,
      NULL,
      NULL,
      CmdString,         /* O */
      sizeof(CmdString),
      FALSE);
    }

  if ((V != NULL) && (V->PowerState == PowerState))
    {
    /* requested state already active */

    MDB(3,fSTRUCT) MLog("INFO:     powerselectstate (%s) matches requested powerstate on vm %s\n",
      MPowerState[PowerState],
      V->VMID);

    /* need a better way to handle this */
    /* return(SUCCESS); */
    }

  if (tmpN->PowerSelectState == PowerState)
    {
    /* requested state already active */

    MDB(3,fSTRUCT) MLog("INFO:     powerselectstate (%s) matches requested powerstate on node %s\n",
      MPowerState[PowerState],
      tmpN->Name);

    /* need a better way to handle this */
    /* return(SUCCESS); */
    }

  switch (PowerState)
    {
    case mpowOn:
    case mpowOff:

      if (V != NULL)
        {
        V->PowerSelectState = PowerState;
        V->PowerState       = PowerState;
        V->PowerMTime       = MSched.Time;
        }
      else
        {
        for (nindex = 0;nindex < MSched.M[mxoNode];nindex++)
          {
          if (NL != NULL)
            {
            tmpN = MNLReturnNodeAtIndex(NL,nindex);
            }
          else if (nindex > 0)
            {
            tmpN = NULL;
            }
          else
            {
            tmpN = N;
            }
 
          if (tmpN == NULL)
            break;

          /* power node on */
     
          if ((ND == NULL) || (ND->Path[mrmXNodePower] == NULL))
            {
            if (MSched.Mode != msmTest)
              {
              MDB(3,fSTRUCT) MLog("WARNING:  no external power interface - cannot power %s node %s%s%s\n",
                MPowerState[PowerState],
                tmpN->Name,
                (V != NULL) ? ":" : "",
                (V != NULL) ? V->VMID : "");
     
              if (PowerState == mpowOn)
                {
                MMBAdd(&tmpN->MB,"no external power interface - cannot power on node",NULL,mmbtNONE,0,0,NULL);
          
                if (tmpN->RM != NULL)
                  MMBAdd(&tmpN->RM->MB,"no external power interface - cannot power on node",NULL,mmbtNONE,0,0,NULL);
                }
              else
                {
                MMBAdd(&tmpN->MB,"no external power interface - cannot power off node",NULL,mmbtNONE,0,0,NULL);
          
                if (tmpN->RM != NULL)
                  MMBAdd(&tmpN->RM->MB,"no external power interface - cannot power off node",NULL,mmbtNONE,0,0,NULL);
                }
    
              if (EMsg != NULL)
                snprintf(EMsg,MMAX_LINE,"no external power interface - cannot modify power state");
     
              return(FAILURE);
              }
     
            /* no external action specified, make local changes only */
     
            MDB(3,fSTRUCT) MLog("INFO:     no external power interface - internally powering %s node %s\n",
              MPowerState[PowerState],
              tmpN->Name);
     
            tmpN->PowerSelectState = PowerState;
            tmpN->PowerIsEnabled = TRUE;
     
            return(SUCCESS);
            } /* END if ((ND == NULL) || (ND->Path[mrmXNodePower] == NULL)) */
          }   /* END for (nindex) */
        }     /* END else */

      {
      mstring_t NodeList(MMAX_LINE);

      if (NL != NULL)
        {
        MNLToMString(NL,FALSE,",",'\0',1,&NodeList);
  
        ptr = NodeList.c_str();

        tmpN = MNLReturnNodeAtIndex(NL,0);
        }
      else if (V != NULL)
        {
        ptr = V->VMID;

        tmpN = N;
        }
      else
        {
        ptr =  N->Name;

        tmpN = N;
        }

      mstring_t tmpString(MMAX_LINE);

      /* execute script */

      MStringAppendF(&tmpString,"%s %s %s",
        CmdString,
        ptr,
        (PowerState == mpowOn) ? "ON" : "OFF");

      MRMStartFunc(tmpN->RM,NULL,mrmNodeModify);

      /* switched to MUReadPipe2 4/3/09 DRW */

      mstring_t Response(MMAX_LINE);
      mstring_t StdErr(MMAX_LINE);

      if (MUReadPipe2(
            tmpString.c_str(),          /* COMMAND */
            NULL,                       /* stdin */
            &Response,                  /* stdout */
            &StdErr,                    /* stderr */
            P,                          /* PSI */
            &rc,                        /* return code */
            EMsg,                       /* duh */
            NULL) == FAILURE)           /* SC */
        {
        MDB(1,fSTRUCT) MLog("ALERT:    cannot read output of command '%s'\n",
          CmdString);

        return(FAILURE);
        }

      MRMEndFunc(tmpN->RM,NULL,mrmNodeModify,&RMTime);

      /* remove time from sched load and add to rm action load */

      MSched.LoadEndTime[mschpRMAction] += RMTime;
      MSched.LoadStartTime[mschpSched]  += RMTime;

      if (strstr(Response.c_str(),"ERROR") != NULL)
        {
        MDB(1,fSTRUCT) MLog("ALERT:    unable to set power to %s for node %s with command '%s'\n",
          MPowerState[PowerState],
          tmpN->Name,
          CmdString);

        if (EMsg != NULL)
          snprintf(EMsg,MMAX_LINE,"error reported from power command - %s",
            Response.c_str());

        return(FAILURE);
        }

      if (V != NULL)
        {
        V->PowerSelectState = PowerState;
        V->PowerState       = PowerState;
        }
      else
        {
        char tmpLine[MMAX_LINE];

        snprintf(tmpLine,sizeof(tmpLine),"node powered %s",
          MPowerState[PowerState]);

        for (nindex = 0;nindex < MSched.M[mxoNode];nindex++)
          {
          if (NL != NULL)
            {
            tmpN = MNLReturnNodeAtIndex(NL,nindex);
            }
          else if (nindex > 0)
            {
            tmpN = NULL;
            }
          else
            {
            tmpN = N;
            }
 
          if (tmpN == NULL)
            break;

          tmpN->PowerSelectState = PowerState;
          tmpN->PowerIsEnabled   = TRUE;

          MOWriteEvent((void *)tmpN,mxoNode,mrelNodeModify,tmpLine,MStat.eventfp,NULL);
     
          if ((PowerState == mpowOn) &&
              (V == NULL) &&
              (MPOWPISGREEN(tmpN->PowerPolicy) || (MPOWPISGREEN(MSched.DefaultN.PowerPolicy))))
            {
            double IdleWatts;
     
            /* NOTE:  node is transitioning from standby to idle */
     
            IdleWatts = MPar[tmpN->PtIndex].DefNodeIdleWatts;
     
            if ((MSched.WattsGMetricIndex > 0) &&
               ((tmpN->XLoad == NULL) || !bmisset(&tmpN->XLoad->RMSetBM,MSched.WattsGMetricIndex)))
              {
              MNodeSetGMetric(
                tmpN,
                MSched.WattsGMetricIndex,
                MPar[tmpN->PtIndex].DefNodeIdleWatts);
     
              tmpN->XLoad->GMetric[MSched.WattsGMetricIndex]->IntervalLoad = MPar[tmpN->PtIndex].DefNodeIdleWatts;
              tmpN->XLoad->GMetric[MSched.WattsGMetricIndex]->SampleCount++;
              }
     
            if (MSched.PWattsGMetricIndex > 0)
              {
              MNodeSetGMetric(
                tmpN,
                MSched.PWattsGMetricIndex,
                IdleWatts);
              }
            }
          else if ((PowerState == mpowOff) &&
                   (V == NULL) &&
                   (MPOWPISGREEN(tmpN->PowerPolicy) || (MPOWPISGREEN(MSched.DefaultN.PowerPolicy))))
            {
            /* node is transitioning from idle to standby */
     
            if ((MSched.WattsGMetricIndex > 0) &&
               ((tmpN->XLoad == NULL) || !bmisset(&tmpN->XLoad->RMSetBM,MSched.WattsGMetricIndex)))
              {
              /* RM is not reporting wattage, usage defaults */
     
              MNodeSetGMetric(tmpN,MSched.WattsGMetricIndex,MPar[tmpN->PtIndex].DefNodeStandbyWatts);
     
              /* NOTE:  pwatts should maintain 'idle' wattage. */
     
              tmpN->XLoad->GMetric[MSched.WattsGMetricIndex]->IntervalLoad = MPar[tmpN->PtIndex].DefNodeStandbyWatts;
              tmpN->XLoad->GMetric[MSched.WattsGMetricIndex]->SampleCount++;
              }
            }
          }    /* END for (nindex) */
        }      /* END else */
      }

      break;

    default:

      /* NYI */

      return(FAILURE);

      /*NOTREACHED*/

      break;
    }  /* END switch (PowerState) */

  return(SUCCESS);
  }  /* END MRMNodeSetPowerState() */
/* END MRMNodeSetPowerState.c */
