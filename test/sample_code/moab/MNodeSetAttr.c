/* HEADER */

      
/**
 * @file MNodeSetAttr.c
 *
 * Contains: MNodeSetAttr
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 * Set specified node attribute.
 *
 * @see MNodeAToString() - report node attribute as string
 * @see MNodeProcessConfig() - process NODECFG config line
 * @see MNLSetAttr() - set an attribute on an NL in one operation
 * @see enum MNodeAttrEnum
 *
 * @param N (I) [modified]
 * @param AIndex (I)
 * @param Value (I) [modified as side-effect]
 * @param Format (I)
 * @param Mode (I)
 */

int MNodeSetAttr(

  mnode_t                *N,      /* I * (modified) */
  enum MNodeAttrEnum      AIndex, /* I */
  void                  **Value,  /* I (modified as side-effect) */
  enum MDataFormatEnum    Format, /* I */
  enum MObjectSetModeEnum Mode)   /* I */

  {
  const char *FName = "MNodeSetAttr";

  MDB(6,fSTRUCT) MLog("%s(%s,%s,Value,%s,%s)\n",
    FName,
    (N != NULL) ? N->Name : "NULL",
    MNodeAttr[AIndex],
    MFormatMode[Format],
    MObjOpType[Mode]);

  if (N == NULL)
    {
    return(FAILURE);
    }

  switch (AIndex)
    {
    case mnaAccess:

      N->SpecNAPolicy = (enum MNodeAccessPolicyEnum)MUGetIndexCI(
          (char *)Value,
          MNAccessPolicy,
          FALSE,
          mnacNONE);

      N->EffNAPolicy = N->SpecNAPolicy;

      break;

    case mnaAccountList:

      MACLFromString(&N->ACL,(char *)Value,1,maAcct,mSet,NULL,FALSE);

      break;

    case mnaAvlGRes:

      if (Format == mdfString)
        {
        int   RIndex;

        char *TokPtr = NULL;
        char *Name;
        char *Count;

        char *ptr;
        char *TokPtr2;

        int   tmpI;

        /* available generic resources */

        /* support multiple formats */

        if ((strchr((char *)Value,'[') != NULL) ||
            (strchr((char *)Value,';') != NULL))
          {
          if (MSNLFromString(
                &N->ARes.GenericRes,
                (char *)Value,
                NULL,
                N->RM) == FAILURE)
            {
            return(FAILURE);
            }

          break;
          }

        /* FORMAT:  <RESNAME>[:<RESCOUNT>][{+,}<RESNAME>[:<RESCOUNT>]]... */
        /*  or */
        /* FORMAT:  <RESNAME>=<RESCOUNT>[,<RESNAME>=<RESCOUNT>]... */

        if (strchr((char *)Value,'=') != NULL)
          {
          ptr = MUStrTok((char *)Value,", \t\n",&TokPtr);

          while (ptr != NULL)
            {
            Name = MUStrTok(ptr,"=",&TokPtr2);

            RIndex = MUMAGetIndex(meGRes,Name,mAdd);

            if ((RIndex == 0) || (RIndex >= MSched.M[mxoxGRes]))
              {
              /* cannot add gres */

              MDB(2,fSTRUCT) MLog("ALERT:    cannot add GRes '%s' in %s()\n",
                Name,
                FName);

              break;
              }

            Count = MUStrTok(NULL,"=",&TokPtr2);
      
            if (Count == NULL)
              {
              /* count not specified */
              tmpI = 9999;
              }
            else
              {
              tmpI = (int)strtol(Count,NULL,10);
              }

            switch(Mode)
              {
              case mDecr:

                {
                /* do not decrement more than we have */
                int actualDec = MIN(MSNLGetIndexCount(&N->CRes.GenericRes,RIndex),tmpI);

                MSNLSetCount(&N->CRes.GenericRes,RIndex,MSNLGetIndexCount(&N->CRes.GenericRes,RIndex) - actualDec);
                }

                break;

              case mAdd:
   
                MSNLSetCount(&N->CRes.GenericRes,RIndex,MSNLGetIndexCount(&N->CRes.GenericRes,RIndex) + tmpI);

                break;

              case mSet:
              default:

                /* not really the default, but this function is broken in that all it did previously was set.  */

                MSNLSetCount(&N->CRes.GenericRes,RIndex,tmpI);

                break;
              }

            ptr = MUStrTok(NULL,", \t\n",&TokPtr);
            }  /* END while (ptr != NULL) */

          break;
          }    /* END if (strchr('=') != NULL) */

        ptr = MUStrTok((char *)Value,"+, \t\n",&TokPtr);

        while (ptr != NULL)
          { 
          Name = MUStrTok(ptr,":",&TokPtr2);

          RIndex = MUMAGetIndex(meGRes,Name,mAdd);

          if ((RIndex == 0) || (RIndex >= MSched.M[mxoxGRes]))
            {
            /* cannot add gres */

            MDB(2,fSTRUCT) MLog("ALERT:    cannot add GRes '%s' in %s()\n",
              Name,
              FName);

            return(FAILURE);
            }
       
          Count = MUStrTok(NULL,":",&TokPtr2);
 
          if (Count != NULL)
            {
            tmpI = (int)strtol(Count,NULL,10);
            }
          else
            {
            tmpI = 9999;
            }
     
          if (MSNLGetIndexCount(&N->ARes.GenericRes,RIndex) != tmpI) 
            {
            MSNLSetCount(&N->ARes.GenericRes,RIndex,tmpI);
            }

          ptr = MUStrTok(NULL,"+, \t\n",&TokPtr);
          }  /* END while (ptr != NULL) */
        }    /* END if (Format == mdfString) */

      break;

    case mnaAllocRes:

      {
      char *ptr;
      char *TokPtr = NULL;

      /* FORMAT:  <RESID>[,<RESID>]... */

      ptr = MUStrTok((char *)Value,",: \t\n",&TokPtr);

      while (ptr != NULL)
        {
        MULLAdd(&N->AllocRes,ptr,NULL,NULL,NULL); /* no free routine */

        ptr = MUStrTok(NULL,",: \t\n",&TokPtr);
        }
      }  /* END BLOCK (case mnaAllocRes) */

      break;

    case mnaArch:

      N->Arch = MUMAGetIndex(meArch,(char *)Value,mAdd);

      break;

    case mnaAvlClass:
    case mnaCfgClass:

      {
      /* available/configured classes */

      /* NOTE:  do not differentiate between mSet and mAdd */

      /* support multiple formats */

      /* FORMAT:  [<CLASSNAME>{: }<COUNT>][{+,}<CLASSNAME>[{: }<COUNT>]]... */

      if ((Format != mdfString) || (Value == NULL))
        {
        break;
        }

      if (N == &MSched.DefaultN)
        {
        int cindex;

        MUStrCpy(
          MSched.DefaultClassList,
          (char *)Value,
          sizeof(MSched.DefaultClassList));

        MClassListFromString(&N->Classes,(char *)Value,N->RM);

        for (cindex = MCLASS_FIRST_INDEX;cindex < MSched.M[mxoClass];cindex++)
          {
          if (bmisset(&N->Classes,cindex))
            {
            /* load config file based default classes */
    
            MClassAdd(MClass[cindex].Name,NULL);

            bmset(&N->Classes,cindex);
            }
          }

        break;
        }

      MClassListFromString(&N->Classes,(char *)Value,N->RM);
      }    /* END BLOCK (case mnaCfgClass,...) */

      break;

    case mnaChargeRate:

      {
      double tmpD = 0.0;

      if (Value != NULL)
        {
        if (Format == mdfDouble)
          tmpD = *(double *)Value;
        else
          tmpD = strtod((char *)Value,NULL);

        N->ChargeRate = tmpD;
        }
      }

      break;

    case mnaClassList:

      MACLFromString(&N->ACL,(char *)Value,1,maClass,mSet,NULL,FALSE);

      break;

    case mnaComment:

      /* NYI - do we route into persistent node message? */

      break;

    case mnaEnableProfiling:

      N->ProfilingEnabled = MUBoolFromString((char *)Value,TRUE);

      if (N->ProfilingEnabled == TRUE)
        {
        mnust_t *SPtr = NULL;

        if ((MOGetComponent((void *)N,mxoNode,(void **)&SPtr,mxoxStats) == SUCCESS) &&
            (SPtr->IStat == NULL))
          {
          MStatPInitialize((void *)SPtr,FALSE,msoNode);
          }

        MSched.ProfilingIsEnabled = TRUE;
        }
      else if ((N->ProfilingEnabled == FALSE) && (N->Stat.IStat != NULL))
        {
        /* race condition turned profiling on when node was created */

        MStatPDestroy((void ***)&N->Stat.IStat,msoNode,N->Stat.IStatCount);
        }

      break;

    case mnaFeatures:

      {
      char *ptr;
      char *TokPtr;

      if (Mode == mSet)
        bmclear(&N->FBM);
   
      if (Value == NULL)
        {
        break;
        }

      /* FORMAT:  <FEATURE>[{, ;:\n\t}<FEATURE>]... */

      ptr = MUStrTok((char *)Value,":;, \t\n",&TokPtr);
   
      while (ptr != NULL)
        {
        MNodeProcessFeature(N,ptr);
   
        ptr = MUStrTok(NULL,":;, \t\n",&TokPtr);
        }  /* END while (ptr != NULL) */
      }    /* END BLOCK (case mnaFeatures) */

      break;

    case mnaFlags:

      if (Value != NULL)
        {
        mbitmap_t tmpL;

        if (Format == mdfLong)
          bmcopy(&tmpL,(mbitmap_t *)Value);
        else
          bmfromstring((char *)Value,MNodeFlags,&tmpL);

        if (Mode == mSet)
          {
          bmcopy(&N->Flags,&tmpL);
          }
        else if ((Mode == mAdd) || (Mode == mIncr))
          {
          bmor(&N->Flags,&tmpL);
          }
        else if ((Mode == mDecr) || (Mode == mUnset))
          {
          unsigned int index;

          for (index = 0;index < mnfLAST;index++)
            {
            if (bmisset(&tmpL,index))
              bmunset(&N->Flags,index);
            }
          }
        }  /* END BLOCK (case mnaFlags) */

      break;

    case mnaCfgGRes:

      if (Format == mdfString)
        {
        int   RIndex;

        char *TokPtr = NULL;
        char *Name;
        char *Count;
        char *Val;

        char *ptr;
        char *TokPtr2 = NULL;

        char *Label;

        int   tmpI = 0;

        /* configured generic resources */

        /* NOTE:  do not differentiate between mSet and mAdd */

        /* support multiple formats */

        if ((strchr((char *)Value,'[') != NULL) ||
            (strchr((char *)Value,';') != NULL))
          {
          if (MSNLFromString(
                &N->CRes.GenericRes,
                (char *)Value,
                NULL,
                N->RM) == FAILURE)
            {
            return(FAILURE);
            }

          break;
          }

        /* FORMAT:  <RESNAME>[:<RESCOUNT>][{+,}<RESNAME>:[:<RESCOUNT>]]... */
        /*  or */
        /* FORMAT:  <RESNAME>:<LABEL1>[:<LABEL2>][{+,}<RESNAME>[:<LABEL1>]]... */
        /*  or */
        /* FORMAT:  <RESNAME>=<RESCOUNT>[,<RESNAME>=<RESCOUNT>]... */

        Count = NULL;

        if (strchr((char *)Value,'=') != NULL)
          {
          ptr = MUStrTok((char *)Value,", \t\n",&TokPtr);

          while (ptr != NULL)
            {
            Name = MUStrTok(ptr,"=",&TokPtr2);

            RIndex = MUMAGetIndex(meGRes,Name,mAdd);

            if ((RIndex == 0) || (RIndex >= MSched.M[mxoxGRes]))
              {
              /* cannot add gres */

              MDB(2,fSTRUCT) MLog("ALERT:    cannot add GRes '%s' in %s()\n",
                Name,
                FName);

              break;
              }

            Count = MUStrTok(NULL,"=",&TokPtr2);
        
            if (Count == NULL)
              {
              /* count not specified */

              tmpI = 9999;
              }
            else
              {
              tmpI = (int)strtol(Count,NULL,10);
              }

            if (MGRes.GRes[RIndex]->FeatureGRes == TRUE)
              {
              int sindex = MUGetNodeFeature(Name,mAdd,NULL);

              if ((sindex == 0) || (sindex >= MMAX_ATTR))
                {
                MDB(2,fSTRUCT) MLog("ALERT:    cannot add feature gres '%s' in %s()\n",
                  Name,
                  FName);

                break;
                }

              if (tmpI > 0)
                {
                bmset(&MSched.FeatureGRes,sindex);
                }

              ptr = MUStrTok(NULL,", \t\n",&TokPtr);

              continue;
              }

            MSNLSetCount(&N->CRes.GenericRes,RIndex,tmpI);

            ptr = MUStrTok(NULL,", \t\n",&TokPtr);
            }  /* END while (ptr != NULL) */
          }    /* END if (strchr((char *)Value,'=') != NULL) */
        else
          {
          /* FORMAT:  <ATTR>:<COUNT>[:<LABEL>][{+ }<ATTR>:<COUNT>[:<LABEL>]]... */

          ptr = MUStrTok((char *)Value,"+, \t\n",&TokPtr);

          while (ptr != NULL)
            {
            Name  = MUStrTok(ptr,":",&TokPtr2);

            Val   = MUStrTok(NULL,":",&TokPtr2);

            Label = MUStrTok(NULL,":",&TokPtr2);

            /* NOTE:  only add as generic resource if Val is not specified or Val is integer */
            /*        otherwise, add as node attribute */

            if ((Val != NULL) && isalpha(Val[0]))
              {
              char *tmpVal;

              tmpVal = strdup(Val);

              /* NOTE:  node attribute specified */

              N->AttrList.insert(std::pair<const char *,const char *>(Name,tmpVal));

#if 0
              mln_t *L;
              if (MULLAdd(&N->AttrList,Name,(void *)tmpVal,&L,MUFREE) == FAILURE)
                {
                MDB(2,fSTRUCT) MLog("ALERT:    cannot add node attribute %s='%s' to node %s in %s()\n",
                  Name,
                  Val,
                  N->Name,
                  FName);
                }
              else
                {
                /* HACK: indicate rm-specified node attribute which should be cleared each iteration */

                if (Mode == mSetOnEmpty)
                  bmset(&L->BM,1);
                }
#endif

              ptr = MUStrTok(NULL,"+, \t\n",&TokPtr);

              continue;
              }
            else if ((Val != NULL) && !isalpha(Val[0]))
              {
              Count = Val;
              }
   
            RIndex = MUMAGetIndex(meGRes,Name,mAdd);

            if ((RIndex == 0) || (RIndex >= MSched.M[mxoxGRes]))
              {
              /* cannot add gres */

              MDB(2,fSTRUCT) MLog("ALERT:    cannot add GRes '%s' in %s()\n",
                Name,
                FName);

              break;
              }

            if (MGRes.GRes[RIndex]->FeatureGRes == TRUE)
              {
              int sindex = MUGetNodeFeature(Name,mAdd,NULL);

              if ((sindex == 0) || (sindex >= MMAX_ATTR))
                {
                MDB(2,fSTRUCT) MLog("ALERT:    cannot add feature gres '%s' in %s()\n",
                  Name,
                  FName);

                break;
                }

              if ((Count == NULL) || (strtol(Count,NULL,10) > 0))
                {
                /* if they didn't specify count then default to ON, 0=off */

                bmset(&MSched.FeatureGRes,sindex);
                }

              ptr = MUStrTok(NULL,"+, \t\n",&TokPtr);

              continue;
              }
         
            if (Count == NULL)
              {
              /* count not specified */

              tmpI = 9999;

              MSNLSetCount(&N->CRes.GenericRes,RIndex,tmpI);
              }
            else if (isdigit(Count[0]) && (Label == NULL))
              {
              tmpI = (int)strtol(Count,NULL,10);
     
              MSNLSetCount(&N->CRes.GenericRes,RIndex,tmpI);
              }
            else
              {
              char tmpName[MMAX_NAME];

              /* labelled resources detected */

              /* clear old resources */

              MSNLClear(&N->CRes.GenericRes);

              while (Label != NULL)
                {
                char *DPtr = NULL;

                sprintf(tmpName,"%s:%s",
                  Name,
                  Label);

                MUStrDup(&DPtr,tmpName);

                N->AttrList.insert(std::pair<const char *,const char *>(Name,NULL));

                MSNLSetCount(&N->CRes.GenericRes,RIndex,MSNLGetIndexCount(&N->CRes.GenericRes,RIndex) + 1);

#if 0
                if (MULLAdd(&N->AttrList,tmpName,NULL,NULL,NULL) == SUCCESS) /* no free routine */
                  {
                  MSNLSetCount(&N->CRes.GenericRes,RIndex,MSNLGetIndexCount(&N->CRes.GenericRes,RIndex) + 1);
                  }
                else
                  {
                  MUFree(&DPtr);
                  }
#endif

                Label = MUStrTok(NULL,":",&TokPtr2);
                }  /* END while (Label != NULL) */

              MSNLRefreshTotal(&N->CRes.GenericRes);
              }  /* END else */

            ptr = MUStrTok(NULL,"+, \t\n",&TokPtr);
            }  /* END while (ptr != NULL) */
          }    /* END else (strchr((char *)Value,'=') != NULL) */
        }      /* END if (Format == mdfString) */

      break;

    case mnaDedGRes:

      if (MSched.IsClient == FALSE)
        break;
          
      /* Dedicated resources can only be explicitly set in client commands (ie. mdiag -n) */
        
      if ((strchr((char *)Value,'[') != NULL) ||
          (strchr((char *)Value,';') != NULL))
        {
        if (MSNLFromString(
              &N->DRes.GenericRes,
              (char *)Value,
              NULL,
              N->RM) == FAILURE)
          {
          return(FAILURE);
          } 
        }

      break;

    case mnaGEvent:

      {
      char *ptr;
      char *TokPtr = NULL;

      char *GEName;
      mulong GERearm = 0;
      mgevent_obj_t   *GEvent;
      mgevent_desc_t  *GEDesc;

      /* generic event */

      /* FORMAT:  <ename>[{:=}'<msg>'] */

      ptr = (char *)*Value;

      if (ptr[0] == '\0')
        {
        /* mal-formed value, ignore */

        break;
        }

      ptr = MUStrTokE(ptr,"=:",&TokPtr);

      GEName = ptr;
      MGEventAddDesc(GEName);

      MGEventGetOrAddItem(GEName,&N->GEventsList,&GEvent);

      if (MGEventGetDescInfo(GEName,&GEDesc) == SUCCESS)
        {
        GERearm = GEDesc->GEventReArm;
        }

      ptr = MUStrTokE(NULL,"=:",&TokPtr);

      MUStrDup(&GEvent->GEventMsg,ptr);

      if (GEvent->GEventMTime + GERearm <= MSched.Time)
        {
        GEvent->GEventMTime = MSched.Time;

        MGEventProcessGEvent(
          -1,
          mxoNode,
          N->Name,
          GEName,
          0.0,
          mrelGEvent,
          ptr);
        }
      }  /* END BLOCK (case mnaGEvent) */

      break;

    case mnaGMetric:

      {
      char *GMTok;
      char *GMName;
      char *GMVal;
      char *TokPtr = NULL;

      char *TokPtr2 = NULL;
 
      int   gmindex;

      /* generic metrics */

      /* FORMAT:  <GMETRIC>{:=}<VAL>[,<GMETRIC>{:=}<VAL>]... */

      GMTok = MUStrTok((char *)Value,", \t\n",&TokPtr);

      while (GMTok != NULL)
        {
        GMName = MUStrTok(GMTok,"=:",&TokPtr2);
        GMVal  = MUStrTok(NULL,"=:",&TokPtr2);

        GMTok = MUStrTok(NULL,", \t\n",&TokPtr);

        if ((GMName == NULL) || (GMVal == NULL))
          {
          /* mal-formed value, ignore */

          continue;
          }

        gmindex = MUMAGetIndex(meGMetrics,GMName,mAdd);

        if ((gmindex <= 0) || (gmindex >= MSched.M[mxoxGMetric]))
          continue;

        /* valid gmetric located */

        MNodeSetGMetric(N,gmindex,strtod(GMVal,NULL));
        }  /* END while (GMTok != NULL) */
      }    /* END BLOCK (case mnaGMetric) */

      break;

    case mnaGroupList:

      MACLFromString(&N->ACL,(char *)Value,1,maGroup,mSet,NULL,FALSE);

      break;

    case mnaHopCount:

      if (Value == NULL)
        break;

      if (Format == mdfInt)
        {
        N->HopCount = *(int *)Value;

        break;
        }
      
      if (Format == mdfString)
        {
        N->HopCount = (int)strtol((char *)Value,NULL,10);

        break;
        }

      break;

    case mnaJPriorityAccess:

      MACLFromString(&N->ACL,(char *)Value,1,maJPriority,mSet,NULL,FALSE);

    case mnaJTemplateList:

      MACLFromString(&N->ACL,(char *)Value,1,maJTemplate,mSet,NULL,FALSE);

      break;

    case mnaKbdDetectPolicy:

      {
      if (Value == NULL)
        {
        N->KbdDetectPolicy = mnkdpNONE;

        break;
        }

      if (Format == mdfString)
        {
        /* FORMAT:  <STRING> */

        N->KbdDetectPolicy = (enum MNodeKbdDetectPolicyEnum)MUGetIndexCI(
          (char *)Value,
          MNodeKbdDetectPolicy,
          FALSE,
          mnkdpNONE);

        if (N->KbdDetectPolicy != mnkdpNONE)
          MSched.DesktopHarvestingEnabled = TRUE;
        }
      }  /* END BLOCK */

      break;

    case mnaKbdIdleTime:

      if (Format == mdfString)
        {
        N->KbdIdleTime = MUTimeFromString((char *)Value);
        }

      break;

    case mnaLoad:

      {
      double tmpD = 0.0;

      if (Value != NULL)
        {
        if (Format == mdfDouble)
          tmpD = *(double *)Value;
        else
          tmpD = strtod((char *)Value,NULL);
        }

      N->Load = tmpD;
      }  /* END BLOCK */

      break;

    case mnaMaxJob:

       /* NYI */

       break;

    case mnaMaxJobPerUser:

      /* NYI */

      break;

    case mnaMaxLoad:

      if (Value == NULL)
        {
        break;
        }

      if (Format == mdfString)
        {
        N->MaxLoad = strtod((char *)Value,NULL);
        }

      break;

    case mnaMaxPE:

      {
      int tmpI;

      if (Format == mdfString)
        {
        /* FORMAT:  <VAL> */

        tmpI = (int)strtol((char *)Value,NULL,10);

        N->AP.HLimit[mptMaxPE] = tmpI;
        }
      else
        {
        return(FAILURE);
        }
      }  /* END BLOCK */

      break;

    case mnaMaxPEPerJob:

      {
      if (Value == NULL)
        {
        break;
        }

      if (Format == mdfString)
        {
        /* FORMAT:  <VAL>% | <VAL> */

        if (N->NP == NULL)
          {
          if ((N->NP = (mnpolicy_t *)MUCalloc(1,sizeof(mnpolicy_t))) == NULL)
            break;
          }

        if (strchr((char *)Value,'%'))
          N->NP->MaxPEPerJob = strtod((char *)Value,NULL) / 100.0;
        else
          N->NP->MaxPEPerJob = strtod((char *)Value,NULL);
        }
      else
        {
        return(FAILURE);
        }
      }  /* END BLOCK */

      break;

    case mnaMaxProcPerClass:

      {
      int tmpI;

      tmpI = (int)strtol((char *)Value,NULL,10);

      if (N->NP == NULL)
        {
        if ((N->NP = (mnpolicy_t *)MUCalloc(1,sizeof(mnpolicy_t))) == NULL)
          break;
        } 

      N->NP->MaxProcPerClass = tmpI;
      }   /* END BLOCK */

      break;

    case mnaMaxWCLimitPerJob:

      {
      int tmpI;

      if (Format == mdfString)
        {
        /* FORMAT:  <VAL> */

        tmpI = MUTimeFromString((char *)Value);

        N->AP.HLimit[mptMaxWC] = tmpI;
        }
      else
        {
        return(FAILURE);
        }
      }  /* END case mnaMaxWCLimitPerJob */

      break;

    case mnaMessages:

      if (Format == mdfOther)
        {
        mmb_t *MP = (mmb_t *)Value;

        MMBAdd(&N->MB,MP->Data,MP->Owner,MP->Type,MP->ExpireTime,MP->Priority,NULL);
        }
      else if (Format == mdfString)
        {
        /* FORMAT: [<LABEL>:]<MESSAGE> */

        MMBAdd(&N->MB,(char *)Value,NULL,mmbtNONE,0,0,NULL);
        }

      break;

    case mnaOldMessages:

      if (Value == NULL)
        {
        MUFree(&N->Message);
        }
      else
        {
        /* NOTE:  only override, no support for add */

        MUStrDup(&N->Message,(char *)Value);
        }
 
      break;

    case mnaMinPreemptLoad:

      {
      if (Value == NULL)
        {
        N->MinPreemptLoad = 0.0;

        break;
        }

      if (Format == mdfString)
        {
        /* FORMAT:  <FLOAT> */

        N->MinPreemptLoad = strtod((char *)Value,NULL);
        }
      }  /* END BLOCK */

      break;

    case mnaMinResumeKbdIdleTime:

      {
      if (Value == NULL)
        {
        N->MinResumeKbdIdleTime = 0;

        break;
        }

      if (Format == mdfString)
        {
        /* FORMAT:  <INTEGER> */

        N->MinResumeKbdIdleTime = strtol((char *)Value,NULL,10);
        }
      }  /* END BLOCK */

      break;

    case mnaNodeID:

      MUStrCpy(N->Name,(char *)Value,sizeof(N->Name));

      break;

    case mnaNodeIndex:

      N->NodeIndex = (int)strtol((char *)Value,NULL,10);

      break;

    case mnaNodeState:

      if (Format == mdfString)
        {
        N->State = (enum MNodeStateEnum)MUGetIndex((char *)Value,MNodeState,FALSE,0);
        }
      else
        {
        int tmpI;

        tmpI = *(int *)Value;

        N->State = (enum MNodeStateEnum)tmpI;
        }

      N->EState = N->State;

      break;

    case mnaNodeType:

      if (Value != NULL)
        MUStrDup(&N->NodeType,(char *)Value);
      else if (N->NodeType != NULL)
        MUFree(&N->NodeType);

      break;

    case mnaOS:

      {
      int tmpOS;

      tmpOS = MUMAGetIndex(meOpsys,(char *)Value,mAdd);

      if ((N->OSList == NULL) && (MSched.DefaultN.OSList == NULL))
        {
        /* allow request */

        N->ActiveOS = MUMAGetIndex(meOpsys,(char *)Value,mAdd);

        break;
        }

      /* check if request is in OSList */

      if (!strcmp(MAList[meOpsys][tmpOS],ANY))
        {
        N->ActiveOS = tmpOS;

        break;
        }

      if (MNodeIsValidOS(N,tmpOS,NULL) == FALSE)
        {
        return(FAILURE);
        }

      N->ActiveOS = tmpOS;
      }  /* END BLOCK (case mnaOS) */

      break;

    case mnaOSList:
    case mnaVMOSList:

      {
      mnodea_t **List;

      if (AIndex == mnaOSList)
        {
        List = &N->OSList;
        }
      else
        {
        List = &N->VMOSList;
        }

      if (Format != mdfString)
        {
        /* NOTE:  only accepts string format */

        return(FAILURE);
        }

      if (!MOLDISSET(MSched.LicensedFeatures,mlfProvision))
        {
        MMBAdd(&MSched.MB,"ERROR:  license does not allow OS provisioning, please contact Adaptive Computing\n",NULL,mmbtNONE,0,0,NULL);

        return(SUCCESS);
        }

      if (MNodeParseOSList(
            N,
            List,
            (char *)Value,
            Mode,
            (AIndex == mnaVMOSList)) == FAILURE)
        {
        return(FAILURE);
        }
      }

      break;
 
    case mnaOwner:

      if (Value == NULL)
        {
        N->Cred.U = NULL;
        N->Cred.G = NULL;
        N->Cred.A = NULL;
        N->Cred.Q = NULL;
        N->Cred.C = NULL;

        break;
        }

      {
      char *ptr;
      char *TokPtr = NULL;

      char *ptr2;
      char *TokPtr2 = NULL;

      enum MXMLOTypeEnum OType;

      void *O;

      /* FORMAT:  <CREDTYPE>:<CREDID>[,<CREDTYPE>:<CREDID>]... */

      ptr = MUStrTok((char *)Value,",",&TokPtr);

      while (ptr != NULL)
        {
        ptr2 = MUStrTok(ptr,":",&TokPtr2);

        OType = (enum MXMLOTypeEnum)MUGetIndexCI(ptr2,MXO,FALSE,mxoNONE);

        ptr2 = MUStrTok(NULL,":",&TokPtr2);

        if ((OType == mxoNONE) || (MOGetObject(OType,ptr2,&O,mAdd) == FAILURE))
          {
          ptr = MUStrTok(NULL,":",&TokPtr);

          continue;
          }
 
        switch (OType)
          {
          case mxoUser:  N->Cred.U = (mgcred_t *)O; break;
          case mxoGroup: N->Cred.G = (mgcred_t *)O; break;
          case mxoAcct:  N->Cred.A = (mgcred_t *)O; break;
          case mxoQOS:   N->Cred.Q = (mqos_t *)O;   break;
          case mxoClass: N->Cred.C = (mclass_t *)O; break;
          default:  break;
          }  /* END switch (OType) */
    
        ptr = MUStrTok(NULL,",",&TokPtr);
        }  /* END while (ptr != NULL) */
      }    /* END BLOCK */

      break;

    case mnaPartition:

      {
      mpar_t *P;

      /* NOTE:  MNodeSetPartition() calls MParAddNode() */

      MNodeSetPartition(N,NULL,(char *)Value);

      if ((Value == NULL) || (MParAdd((char *)Value,&P) != SUCCESS))
        {
        MDB(3,fSTRUCT) MLog("ALERT:    cannot add node '%s' to partition '%s' (insufficient partitions?)\n",
          (N != NULL) ? N->Name : "NULL",
          (Value != NULL) ? (char *)Value : "NULL");

        /* assign to default partition */

        P = &MPar[MCONST_DEFAULTPARINDEX];

        /* the partition add failed - only set the partition to default
         * if the node did not already have a valid partition. */

        if (N->PtIndex <= 0) 
          {
          MParAddNode(P,N);
          }
        }
      }  /* END BLOCK */

      break;

    case mnaPowerIsEnabled:

      {
      /* synchronize with MNLSetAttr(mnaPowerIsEnabled) */
      /* FORMAT:  <BOOLEAN> */

      mbool_t PowerIsEnabled;

      enum MPowerStateEnum PowerState;

      PowerIsEnabled = MUBoolFromString((char *)Value,TRUE);
   
      if (Mode != mVerify)
        {
        /* no external action specified, make local changes only */

        N->PowerIsEnabled = PowerIsEnabled;

        if (PowerIsEnabled == TRUE)
          {
          if ((N->PowerState == mpowNONE) || (N->PowerState == mpowOff))
            N->PowerState = mpowOn;
          }
        else if (PowerIsEnabled == FALSE)
          {
          if ((N->PowerState == mpowNONE) || (N->PowerState == mpowOn))
            N->PowerState = mpowOff;
          }

        return(SUCCESS);
        }

      switch (PowerIsEnabled)
        {
        case TRUE:

          PowerState = mpowOn;

          break;

        case FALSE:

          PowerState = mpowOff;

          break;

        default:

          PowerState = mpowOn;

          break;
        }

      /* NOTE: cannot check MSched.DisableVMDecisions here (may be used by
                a user).  Must check before calling MNodeSetAttr. */

      MRMNodeSetPowerState(N,NULL,NULL,PowerState,NULL);
      }    /* END BLOCK (case mnaPowerIsEnabled) */

      break;

    case mnaPowerPolicy:

      if (!MOLDISSET(MSched.LicensedFeatures,mlfGreen))
        {
        MMBAdd(&MSched.MB,"ERROR:   power policy not enabled with current license, please contact Adaptive Computing\n",NULL,mmbtNONE,0,0,NULL);

        return(SUCCESS);
        }

      N->PowerPolicy = (enum MPowerPolicyEnum)MUGetIndexCI((char *)Value,MPowerPolicy,FALSE,mpowpNONE);

      if (N->PowerPolicy == mpowpOnDemand)
        MSched.OnDemandGreenEnabled = TRUE;

      break;

    case mnaPowerSelectState:

      N->PowerSelectState = (enum MPowerStateEnum)MUGetIndexCI((char *)Value,MPowerState,FALSE,mpowNONE);

      break;

    case mnaPowerState:

      N->PowerState = (enum MPowerStateEnum)MUGetIndexCI((char *)Value,MPowerState,FALSE,mpowNONE);

      break;

    case mnaPreemptMaxCPULoad:
   
      if (Value == NULL)
        {
        break;
        }

      if (N->NP == NULL)
        {
        if ((N->NP = (mnpolicy_t *)MUCalloc(1,sizeof(mnpolicy_t))) == NULL)
          break;
        }
 
      if (Format == mdfString)
        N->NP->PreemptMaxCPULoad = strtod((char *)Value,NULL);
      else
        N->NP->PreemptMaxCPULoad = *(double *)Value;

      break;

    case mnaPreemptMinMemAvail:

      if (Value == NULL)
        {
        break;
        }

      if (N->NP == NULL)
        {
        if ((N->NP = (mnpolicy_t *)MUCalloc(1,sizeof(mnpolicy_t))) == NULL)
          break;
        }

      if (Format == mdfString)
        N->NP->PreemptMinMemAvail = strtod((char *)Value,NULL);
      else
        N->NP->PreemptMinMemAvail = *(double *)Value;

      break;

    case mnaPreemptPolicy:

      if (N->NP == NULL)
        {
        if ((N->NP = (mnpolicy_t *)MUCalloc(1,sizeof(mnpolicy_t))) == NULL)
          break;
        }

      N->NP->PreemptPolicy = (enum MPreemptPolicyEnum)MUGetIndexCI((char *)Value,MPreemptPolicy,FALSE,mppNONE);

      break;

    case mnaPriority:

      if (Format == mdfString)
        N->Priority = (int)strtol((char *)Value,NULL,10);
      else if (Format == mdfInt)
        N->Priority = *(int *)Value;
      else
        N->Priority = *(long *)Value;
 
      N->PrioritySet = TRUE;

      break;

    case mnaProcSpeed:

      /* NYI */

      break;

    case mnaProvRM:

      MRMAdd((char *)Value,&N->ProvRM);

      break;

    case mnaQOSList:

      MACLFromString(&N->ACL,(char *)Value,1,maQOS,mSet,NULL,FALSE);  

      break;

    case mnaRack:

      {
      int tmpI;

      if (Format == mdfInt)
        {
        tmpI = *(int *)Value;
        }
      else
        {
        sscanf((char *)Value,"%d",
          &tmpI);
        }

      if ((tmpI > 0) &&
          (N->SlotIndex > 0) &&
          (MSys[tmpI][N->SlotIndex].RMName[0] != '\0'))
        {
        return(FAILURE);
        }

      if ((tmpI <= 0) || (tmpI > MMAX_RACK))
        {
        return(FAILURE);
        }

      if (N->RackIndex == tmpI)
        {
        /* no-change */

        return(SUCCESS);
        }

      /* NOTE:  should we create a MRackRemoveNode() ? */

      if (N->RackIndex > 0)
        MRack[N->RackIndex].NodeCount--;

      if ((N->RackIndex > 0) &&
          (N->SlotIndex > 0) &&
          (MSys[N->RackIndex][N->SlotIndex].RMName[0] != '\0'))
        {
        MSys[N->RackIndex][N->SlotIndex].RMName[0] = '\0';
        MSys[N->RackIndex][N->SlotIndex].N         = NULL;
        }

      MRackAddNode(&MRack[tmpI],N,-1);
      }  /* END BLOCK (case mnaRack) */

      break;

    case mnaRADisk:

      if (Format == mdfString)
        N->ARes.Disk = (int)MURSpecToL((char *)Value,mvmMega,mvmMega);
      else if (Format == mdfInt)
        N->ARes.Disk = *(int *)Value;
      else
        N->ARes.Disk = *(long *)Value;

      N->CRes.Disk = MAX(N->CRes.Disk,N->ARes.Disk);

      break;

    case mnaRAMem:

      {
      long AvailMem;

      if (bmisset(&N->IFlags,mnifMemOverride) == FALSE) 
        {
        if (Format == mdfString)
          {
          AvailMem = MURSpecToL((char *)Value,mvmMega,mvmMega);
           
          N->ARes.Mem = AvailMem; /* convert from long to int */
           
          if (N->ARes.Mem < 0)
            {
            MDB(7,fNAT) MLog("INFO:     setting negative RAMEM value '%s' (%ld) (%d)\n",
              (char *)Value,
              AvailMem,
              N->ARes.Mem);
            }
          }
        else
          {
          AvailMem = *(int *)Value;
  
          N->ARes.Mem = AvailMem; /* convert from long to int */
  
          if (N->ARes.Mem < 0)
            {
            MDB(7,fNAT) MLog("INFO:     setting negative RAMEM value '%ld' (%d)\n",
              AvailMem,
              N->ARes.Mem);
            }
          }

        N->CRes.Mem  = MAX(N->CRes.Mem,N->ARes.Mem);
        }
      else /* let moab determine available mem */
        {
        /* N->ARes is memsetted each iteration so ignorning setting N->ARes.Mem will cause
         * problems since ARes.Mem will be 0. Setting to CRes.Mem will let moab determine 
         * the actual available mem. */

        N->ARes.Mem = N->CRes.Mem; 

        if (N->ARes.Mem < 0)
          {
          MDB(7,fNAT) MLog("INFO:     setting negative RAMEM '%d' from CRes\n",
            N->ARes.Mem);
          }
        }
      }

      break;

    case mnaRAProc:
 
      if (Format == mdfString)
        N->ARes.Procs = (int)strtol((char *)Value,NULL,10);
      else if (Format == mdfInt)
        N->ARes.Procs = *(int *)Value;
      else
        N->ARes.Procs = *(long *)Value;

      N->CRes.Procs = MAX(N->CRes.Procs,N->ARes.Procs);

      break;

    case mnaRASwap:
 
      if (bmisset(&N->IFlags,mnifSwapOverride) == FALSE) 
        {
        if (Format == mdfString)
          N->ARes.Swap = (int)MURSpecToL((char *)Value,mvmMega,mvmMega);
        else if (Format == mdfInt)
          N->ARes.Swap = *(int *)Value;
        else
          N->ARes.Swap = *(long *)Value;

        N->CRes.Swap = MAX(N->CRes.Swap,N->ARes.Swap);
        }
      else /* let moab determine available swap */
        {
        /* N->ARes is memsetted each iteration so ignorning setting N->ARes.Swap will cause
         * problems since ARes.Swap will be 0. Setting to CRes.Swap will let moab determine 
         * the actual available swap. */

        N->ARes.Swap = N->CRes.Swap; 
        }

      break;

    case mnaRCDisk:
 
      if (Format == mdfString)
        N->CRes.Disk = (int)MURSpecToL((char *)Value,mvmMega,mvmMega);
      else if (Format == mdfInt)
        N->CRes.Disk = *(int *)Value;
      else
        N->CRes.Disk = *(long *)Value;

      break;
 
    case mnaRCMem:
 
      if (bmisset(&N->IFlags,mnifMemOverride) == FALSE)
        {
        if (Format == mdfString)
          N->CRes.Mem = (int)MURSpecToL((char *)Value,mvmMega,mvmMega);
        else if (Format == mdfInt)
          N->CRes.Mem = *(int *)Value;
        else
          N->CRes.Mem = *(long *)Value;
        }

      break;

    case mnaRCProc:
 
      if (Value == NULL)
        break;

      if (Format == mdfString)
        N->CRes.Procs = (int)strtol((char *)Value,NULL,10);
      else if (Format == mdfInt)
        N->CRes.Procs = *(int *)Value;
      else
        N->CRes.Procs = *(long *)Value;

      break;
 
    case mnaRCSwap:

      if (bmisset(&N->IFlags,mnifSwapOverride) == FALSE)
        { 
        if (Format == mdfString)
          N->CRes.Swap = (int)MURSpecToL((char *)Value,mvmMega,mvmMega);
        else if (Format == mdfInt)
          N->CRes.Swap = *(int *)Value;
        else
          N->CRes.Swap = *(long *)Value;
        }

      break;

    case mnaReleasePriority:

      N->ReleasePriority = (int)strtol((char *)Value,NULL,10);

      break;

    case mnaRMList:

      {
      char *ptr;
      char *TokPtr = NULL;

      int   rmindex;

      mrm_t *R;

      /* FORMAT:  <RM>[,<RM>]... */

      ptr = MUStrTok((char *)Value,",",&TokPtr);

      if ((N->RMList == NULL) &&
         ((N->RMList = (mrm_t **)MUCalloc(1,sizeof(mrm_t *) * (MMAX_RM + 1))) == NULL))
        {
        break;
        }

      ptr = MUStrTok((char *)Value,",",&TokPtr);

      rmindex = 0;

      while (ptr != NULL)
        {
        if (MRMFind(ptr,&R) == SUCCESS)
          {
          N->RMList[rmindex] = R;

          rmindex++;

          if (rmindex >= MMAX_RM)
            break;
          }
        else
          {
          /* cannot locate specified RM */
          }

        ptr = MUStrTok(NULL,",",&TokPtr);
        }  /* END while (ptr != NULL) */

      /* terminate list */

      N->RMList[rmindex] = NULL;
      }  /* END BLOCK (case RMList) */

      break;

    case mnaRsvList:

      {
      mxml_t *RLE = NULL;
      mxml_t *RE;

      int     RTok;

      /* sync local rsv's with remote node rsv's */

      if (MXMLFromString(&RLE,(char *)Value,NULL,NULL) == FAILURE)
        {
        return(FAILURE);
        }

      /* FORMAT:  <Rsv StartTime=X EndTime=X ACL=X></Rsv> */

      RTok = -1;

      while (MXMLGetChild(RLE,NULL,&RTok,&RE) == SUCCESS)
        {
        /* NYI */
        }  /* END MXMLGetChild(RLE) */
      }    /* END BLOCK (case RsvList) */

      break;

    case mnaSlot:

      {
      int SIndex;
      int RIndex;

      char *ptr;

      /* NOTE:  validates slot availability! */

      RIndex = N->RackIndex;

      if (Format == mdfInt)
        {
        SIndex = *(int *)Value;
        }
      else if (Format == mdfString)
        {
        /* FORMAT:  [<RACKINDEX>:]<SLOTINDEX> */
        
        ptr = strchr((char *)Value,':');

        if (ptr != NULL)
          {
          RIndex = (int)strtol((char *)Value,NULL,10);
          SIndex = (int)strtol(ptr + 1,NULL,10);
          }
        else
          {
          SIndex = (int)strtol((char *)Value,NULL,10);
          }
        }
      else 
        {
        /* unknown format */

        SIndex = -1;
        }

      if (((RIndex != N->RackIndex) && (RIndex < 0)) ||
           (RIndex > MMAX_RACK) ||
           (SIndex < 0) ||
           (SIndex > MMAX_RACKSIZE))
        {
        /* invalid location specified */

        return(FAILURE);
        }

      if (((RIndex == 0) || (RIndex == N->RackIndex)) && 
          ((SIndex == 0) || (SIndex == N->SlotIndex)))
        {
        /* no change */

        return(SUCCESS);
        }

      if ((RIndex > 0) &&
          (SIndex > 0) &&
          (MSys[RIndex][SIndex].RMName[0] != '\0'))
        {
        /* location already taken */

        return(FAILURE);
        }
      
      if ((N->SlotIndex > 0) && (N->RackIndex > 0))
        {
        /* remove node from existing location */
          
        if (MRack[N->RackIndex].NodeCount > 0)
          MRack[N->RackIndex].NodeCount--;
  
        if (MSys[N->RackIndex][N->SlotIndex].RMName[0] != '\0')
          {
          MSys[N->RackIndex][N->SlotIndex].RMName[0] = '\0';
          MSys[N->RackIndex][N->SlotIndex].N         = NULL;
          }
        }    /* END if ((N->SlotIndex > 0) && (N->RackIndex > 0)) */
        
      if (RIndex >= 0)
        MRackAddNode(&MRack[RIndex],N,SIndex);
      else
        N->SlotIndex = SIndex;
      }  /* END BLOCK */

      break;

    case mnaSpeed:

      {
      double tmpD = 0.0;

      if (Value != NULL)
        tmpD = strtod((char *)Value,NULL);

      if (tmpD == 0.0)
        tmpD = 0.01;

      N->Speed = tmpD;
      }

      break;

    case mnaStatATime:

      if (Value == NULL)
        break;

      if (Format == mdfString)
        {
        N->SATime = (mulong)strtol((char *)Value,NULL,10);
        }
      else
        {
        N->SATime = *(mulong *)Value;
        }

      break;

    case mnaStatTTime:

      if (Value == NULL)
        break;

      if (Format == mdfString)
        {
        N->STTime = (mulong)strtol((char *)Value,NULL,10);
        }
      else
        {
        N->STTime = *(mulong *)Value;
        }

      break;

    case mnaStatUTime:

      if (Value == NULL)
        break;

      if (Format == mdfString)
        {
        N->SUTime = (mulong)strtol((char *)Value,NULL,10);
        }
      else
        {
        N->SUTime = *(mulong *)Value;
        }

      break;

    case mnaUserList:

      MACLFromString(&N->ACL,(char *)Value,1,maUser,mSet,NULL,FALSE);

      break;

    case mnaVariables:

      /* FORMAT:  <name>[=<value>][,<name>[=<value>]]... */

      {
      char *ptr;
      char *TokPtr;

      char *TokPtr2;

      ptr = MUStrTokEPlus((char *)Value,",",&TokPtr);

      while (ptr != NULL)
        {
        char *cptr;
        int  rc = FAILURE;

        cptr = MUStrTok(ptr,"=",&TokPtr2);

        /* NOTE:  MOAB-3236 requested functionality to erase variables.
         *          For example: "mnodectl -m variable-=abc <node>"
         *        In order to preserve backwards compatibility, the mAdd is the
         *        same as mSet and mDecr is the same as mUnset.
         *  
         *        The '-=' removes the variable.  */ 

        switch(Mode)
          {
          case mAdd:
          case mSet:

            /* Add the variable to the Nodes->Variables link list. */

            rc = MUHTAdd(&N->Variables,cptr,strdup(TokPtr2),NULL,MUFREE);

            break;

          case mDecr:
          case mUnset:

            rc = MUHTRemove(&N->Variables,cptr,MUFREE);

            break;

          default:

            break;
          }

        MDB(7,fSCHED) MLog("%s:     variable '%s' %s on node %s\n",
            (rc == SUCCESS) ? "INFO" : "FAILURE",
            cptr,
            MObjOpType[Mode],
            N->Name);

        if (rc == FAILURE)
          {
            /* NOTE:  this interrupts the parsing of a comma delimted string.  This may 
               result in some of the attributes having been processed and some not, but it is
               The only way to let the calling function know that there is a problem with
               a part of the command.
            */
          return(FAILURE);
          }

        ptr = MUStrTokEPlus(NULL,",",&TokPtr);
        }  /* END while (ptr != NULL) */
      }    /* END BLOCK (case mnaVariables) */

      break;

    default:

      /* NO-OP */

      break;
    }  /* END switch (AIndex) */

  return(SUCCESS);
  }  /* END MNodeSetAttr() */
/* END MNodeSetAttr */
