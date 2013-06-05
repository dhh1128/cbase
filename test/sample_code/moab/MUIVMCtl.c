/* HEADER */

/**
 * @file MUIVMCtl.c
 *
 *
 * Contains MUI VM Control:  int MUIVMCtl()
 *
 */


#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 * Perform the VM Command: Destroy 
 *
 * @param     S                 (I) The socket
 * @param     Auth              (I)
 * @param     VMExp             (I) [VM name]
 * @param     CFlags            (I) 
 */

int __mvmcmDestroy(

  msocket_t  *S,
  char       *Auth,
  char        VMExp[],
  mbitmap_t  *CFlags)

  {
  char        Buf[MMAX_LINE];
  char        EMsg[MMAX_LINE] = {0};
  char        tmpLine[MMAX_LINE << 2];

  msysjob_t  *SysJob = NULL;
  mvm_t      *VM;

  tmpLine[0] = '\0';

  if (MVMFind(VMExp,&VM) == FAILURE)
    {
    snprintf(tmpLine,sizeof(tmpLine),"ERROR:  VM '%s' not found\n",
      VMExp);

    MUISAddData(S,tmpLine);

    return(FAILURE);
    }

  if (MUIVMDestroy(VM,VM->N,Auth,CFlags,Buf,EMsg,&SysJob) == FAILURE)
    {
    MMBAdd(
      &VM->N->MB,
      (char *)tmpLine,
      MSCHED_SNAME,
      mmbtAnnotation,
      MMAX_TIME,
      0,
      NULL);

    MUISAddData(S,EMsg);

    return(FAILURE);
    }

  if (SysJob != NULL) 
    {
    char proxy[MMAX_NAME];

    if (MXMLGetAttr(S->RDE,"proxy",NULL,proxy,sizeof(proxy)))
      {
      MUStrDup(&SysJob->ProxyUser, proxy);
      }
    }

  MUISAddData(S,Buf);

  return(SUCCESS);
  } /* END case mvmcmDestroy */



/**
 * Perform the VM Command: Migrate 
 *
 * @param     S                 (I) The socket
 * @param     Auth              (I)
 * @param     VMExp             (I) [VM tag]
 * @param     CFlags
 * @param     EvalOnly
 */

int __mvmcmForceMigration(

  msocket_t  *S,
  char       *Auth,
  char        VMExp[],
  mbitmap_t  *CFlags,
  mbool_t     EvalOnly)

  {
  /* NOTE:  'mvmctl -f [ALL,green,overcommit,rsv]' will force a migration to occur.
  **   There are 3 types of migrations that are 1) Reservation, Green, and Overcommit.
  **   For now, this is an internal command and hasn't been documented for the general public. */

  int     ccode = TRUE;
  char    tmpLine[MMAX_LINE << 2];
  enum    MVMMigrationPolicyEnum MigrationType;
  mbitmap_t    AuthBM;

  if (!MSched.AllowVMMigration)
    {
    /* MOAB-1662  regardless of the '-f' flag, ALLOWVMMIGRATION overrides any
       types of migration.  */
    snprintf(tmpLine,sizeof(tmpLine),"VM Migrations disabled by administrator. (ALLOWVMMIGRATION)");

    MUISAddData(S,tmpLine);
    return(SUCCESS);
    }

  MSysGetAuth(Auth,mcsMVMCtl,0,&AuthBM);

  if (!bmisset(&AuthBM,mcalAdmin1))
    {
    MUISAddData(S,"ERROR:  request not authorized");

    return(FAILURE);
    }

  MigrationType = (enum MVMMigrationPolicyEnum)MUGetIndexCI(VMExp,MVMMigrationPolicy,FALSE,mvmmpNONE);

  ccode = MUIVMCtlPlanMigrations(MigrationType,EvalOnly,bmisset(CFlags,mcmXML),S);

  return(ccode);
  } /* END __mvmcmForceMigration() */



/**
 * Perform the VM Command: Migrate 
 *
 * @param     S                 (I) The socket
 * @param     Auth              (I)
 * @param     VMExp             (I) [VM name]
 * @param     ArgString         (I) 
 */

int __mvmcmMigrate(

  msocket_t  *S,
  char       *Auth,
  char        VMExp[],
  char        ArgString[])

  {
  char        tmpLine[MMAX_LINE << 2];
  char        tmpMsg[MMAX_LINE] = {0};
  char        EMsg[MMAX_LINE] = {0};
  char       *Arg;
  char       *TokPtr;
  char       *ptr;
  mjob_t     *J;
  mjob_t     *PowerJ = NULL;
  mnode_t    *DstN = NULL;
  mbitmap_t   AuthBM;
  mvm_t      *VM;

  MSysGetAuth(Auth,mcsMVMCtl,0,&AuthBM);
 
  if (!bmisset(&AuthBM,mcalAdmin1))
    {
    MUISAddData(S,"ERROR:  request not authorized");
 
    return(FAILURE);
    }

  if (MVMFind(VMExp,&VM) == FAILURE)
    {
    snprintf(tmpLine,sizeof(tmpLine),"ERROR:  VM '%s' not found\n",
      VMExp);

    MUISAddData(S,tmpLine);

    return(FAILURE);
    }

  /* Verify VM is migratable */

  if (!VMIsEligibleForMigration(VM,TRUE))
    {
    snprintf(tmpLine,sizeof(tmpLine),"ERROR:    VM %s can not be migrated.\n",
      VM->VMID);

    MUISAddData(S,tmpLine);

    return(FAILURE);
    }

  if (VM->N == NULL)
    {
    snprintf(tmpLine,sizeof(tmpLine),"ERROR:    VM %s has no parent.  It may not have been created yet.\n",
      VM->VMID);

    MUISAddData(S,tmpLine);

    return(FAILURE);
    }

  Arg = MUStrTok(ArgString,",",&TokPtr);

  if ((Arg == NULL) || (Arg[0] == '\0'))
    {
    snprintf(tmpLine,sizeof(tmpLine),"ERROR:    argument 'dsthost' is required\n");

    MUISAddData(S,tmpLine);

    return(FAILURE);
    }

  if ((ptr = strstr(Arg,"dsthost=")) == NULL)
    {
    snprintf(tmpLine,sizeof(tmpLine),"ERROR:    argument 'dsthost' is required\n");

    MUISAddData(S,tmpLine);

    return(FAILURE);
    }

  ptr += strlen("dsthost=");

  if (!strncasecmp(ptr,"any",strlen("any")))
    {
    DstN = NULL;
    }
  else if (MNodeFind(ptr,&DstN) == FAILURE)
    {
    snprintf(tmpLine,sizeof(tmpLine),"ERROR:    specified destination host is invalid\n");

    MUISAddData(S,tmpLine);

    return(FAILURE);
    }
  else if (MVMNodeIsValidDestination(VM,DstN,TRUE,EMsg) == FALSE)
    {
    snprintf(tmpLine,sizeof(tmpLine),"ERROR:    %s\n",
      EMsg);

    MUISAddData(S,tmpLine);

    return(FAILURE);
    }

  if (DstN == NULL)
    {
    mstring_t EMsgMString(MMAX_LINE);

    if ((VMFindDestinationSingleRun(VM,&EMsgMString,&DstN) == FAILURE) ||
        (DstN == NULL))
      {
      MDB(2,fALL) MLog("WARNING:  vm %s should migrate from node %s but cannot locate valid destination - %s (policy)\n",
        VM->VMID,
        VM->N->Name,
        EMsgMString.c_str());
 
      snprintf(tmpLine,sizeof(tmpLine),"ERROR:  cannot select destination host to migrate VM %s - %s\n",
        VM->VMID,
        EMsgMString.c_str());
    
      MUISAddData(S,tmpLine);

      return(FAILURE);
      }  /* END if (VMFindDestinationSingleRun(VM,&DstN) == FAILURE) */
    } /* END if (DstN == NULL) */

  /* migrate VM to specified node */

  if (DstN->PowerSelectState == mpowOff)
    {
    snprintf(tmpLine,sizeof(tmpLine),"ERROR:  cannot power-on destination host %s to allow migration of vm %s\n",
      DstN->Name,
      VM->VMID);

    MUISAddData(S,tmpLine);

    return(FAILURE);
    }

  snprintf(tmpMsg,sizeof(tmpMsg),"migration from %s to %s requested by admin %s",
    VM->N->Name,
    DstN->Name,
    (Auth != NULL) ? Auth : "");

  MUStrCpy(tmpLine,"user_migrate",sizeof(tmpLine));

  MVMAddGEvent(VM,tmpLine,0,tmpMsg);

  if (MUSubmitVMMigrationJob(VM,DstN,Auth,&J,tmpMsg,EMsg) == FAILURE)
    {
    snprintf(tmpLine,sizeof(tmpLine),"ERROR:  cannot migrate VM %s to host %s - %s\n",
      VM->VMID,
      DstN->Name,
      EMsg);

    MUISAddData(S,tmpLine);

    return(FAILURE);
    }

  if (PowerJ != NULL)
    {
    /* migration cannot execute until power on is complete */

    MJobSetAttr(J,mjaDepend,(void **)PowerJ->Name,mdfString,mSet);
    }

  if (J->System != NULL)
    {        
    char proxy[MMAX_NAME];

    if (MXMLGetAttr(S->RDE,"proxy",NULL,proxy,sizeof(proxy)))
      {
      MUStrDup(&J->System->ProxyUser, proxy);
      }
    }

  snprintf(tmpLine,sizeof(tmpLine),"VM migrate job '%s' submitted\n",
    J->Name);

  MUISAddData(S,tmpLine);

  return(SUCCESS);
  } /* END __ mvmcmMigrate() */




/**
 * Perform the VM Command: Modify 
 *
 * @param     S                 (I) The socket
 * @param     Auth              (I)
 * @param     VMExp             (I) [VM name]
 * @param     AName             (I) [Attribute Name array]
 * @param     AVal              (I) [Attribute Value array]
 * @param     OperationString   (I) [Operation array]
 */

int __mvmcmModify(
    
  msocket_t  *S,
  char       *Auth,
  char        VMExp[],
  char        AName[][MMAX_LINE],
  char        AVal[][MMAX_LINE << 2],
  char        OperationString[][MMAX_NAME])

  {
  mvm_t            *VM;
  char              tmpLine[MMAX_LINE << 2];
  enum MVMAttrEnum  AIndex;
  int               sindex;
  mbitmap_t         AuthBM;

  tmpLine[0] = '\0';

  /* Map the Auth to an Auth Bit Map */
  MSysGetAuth(Auth,mcsMVMCtl,0,&AuthBM);
 
  /* Authenticate and error return if cannot */
  if (!bmisset(&AuthBM,mcalAdmin1))
    {
    MUISAddData(S,"ERROR:  request not authorized");
 
    return(FAILURE);
    }

  /* Look for the VM specified */
  if (MVMFind(VMExp,&VM) == FAILURE)
    {
    snprintf(tmpLine,sizeof(tmpLine),"ERROR:  VM '%s' not found\n",
      VMExp);

    MUISAddData(S,tmpLine);

    return(FAILURE);
    }

  /* Iterate over all incoming Attribute names */
  for (sindex = 0;AName[sindex][0] != '\0';sindex++)
    {
    /* Reverse looking string to Attribute Name enum index */

    AIndex = (enum MVMAttrEnum)MUGetIndexCI(
      AName[sindex],
      MVMAttr,
      FALSE,
      mvmaNONE);

    /* Process Attribute selected */
    switch (AIndex)
      {
      case mvmaNONE:

        snprintf(tmpLine,sizeof(tmpLine),"ERROR:  cannot modify attribute '%s' on VM %s to '%s' - unknown attribute\n",
          AName[sindex],
          VM->VMID,
          AVal[sindex]);

        MUISAddData(S,tmpLine);

        return(FAILURE);

        break;

      case mvmaAlias:

        if (MVMSetAttr(VM,mvmaAlias,AVal[sindex],mdfString,mAdd) == SUCCESS)
          {
          snprintf(tmpLine,sizeof(tmpLine),"alias '%s' added to VM '%s'\n",
            AVal[sindex],
            VM->VMID);
          }
        else
          {
          snprintf(tmpLine,sizeof(tmpLine),"ERROR:  cannot add alias '%s' to VM '%s' (alias already in use?)\n",
            AVal[sindex],
            VM->VMID);

          MUISAddData(S,tmpLine);

          return(FAILURE);
          }

        break;

      case mvmaFlags:

        {
        if (MUStrStr(AVal[sindex],"cannotmigrate",0,TRUE,FALSE))
          {
          bmunset(&VM->Flags,mvmfCanMigrate);
          snprintf(tmpLine,sizeof(tmpLine),"Flag modified");
          }
        else if (MUStrStr(AVal[sindex],"canmigrate",0,TRUE,FALSE))
          {
          bmset(&VM->Flags,mvmfCanMigrate);
          snprintf(tmpLine,sizeof(tmpLine),"Flag modified");
          }
        else
          {
          snprintf(tmpLine,sizeof(tmpLine),"Flag not recognized");
          }

        MUISAddData(S,tmpLine);

        return(SUCCESS);
        }

        break;

      case mvmaGEvent:

        {
        char *ptr;
        char *GName;
        char *TokPtr;
        int severity = 0;

        char tmpLine2[MMAX_LINE];
        char *tmpMsg;
        mgevent_obj_t  *GEvent;

        /* generic event */

        MUStrCpy(tmpLine2,AVal[sindex],sizeof(tmpLine2));

        ptr = MUStrTok(tmpLine2,":",&TokPtr);

        if (ptr == NULL)
          {
          break;
          }

        while(ptr[0] == '=')
          {
          ptr++;
          }

        /* Match the requested operation against the DECREMENT operation string */
        if (!strcmp(OperationString[sindex],MObjOpType[mDecr]))
          {
          /* Remove the given gevent */

          if (MGEventGetItem(ptr,&VM->GEventsList,&GEvent) == SUCCESS)
            {
            MGEventRemoveItem(&VM->GEventsList,ptr);
            MUFree((char **)&GEvent->GEventMsg);
            MUFree((char **)&GEvent->Name);
            MUFree((char **)&GEvent);
            }

          snprintf(tmpLine,sizeof(tmpLine),"gevent removed\n");

          MUISAddData(S,tmpLine);

          tmpLine[0] = '\0';

          continue;
          }

        /* Not a decr operation, set the gevent */

        GName = ptr;

        if (strchr(TokPtr,':') != NULL)
          {
          ptr = MUStrTok(NULL,":",&TokPtr);

          if (ptr != NULL)
            severity = (int)strtol(ptr,NULL,10);
          }

        tmpMsg = MUStrTok(NULL,"\n",&TokPtr);

        if (MVMAddGEvent(VM,GName,severity,tmpMsg) == FAILURE)
          {
          snprintf(tmpLine,sizeof(tmpLine),"ERROR:  could not process event %s\n",GName);

          MUISAddData(S,tmpLine);

          tmpLine[0] = '\0';

          continue;
          }

        snprintf(tmpLine,sizeof(tmpLine),"gevent processed\n");

        MUISAddData(S,tmpLine);

        tmpLine[0] = '\0';

        continue;
        } /* END case mvmaGEvent: */

        break;

      case mvmaGMetric:

        if (MVMSetAttr(
             VM,
             AIndex,
             (void **)AVal[sindex],
             mdfString,
             mSet) == SUCCESS)
          {
          snprintf(tmpLine,sizeof(tmpLine),"gmetric set on VM '%s'\n",
            VM->VMID);

          MUISAddData(S,tmpLine);

          return(SUCCESS);
          }

        break;

      case mvmaVariables:

        /* right now we add variables instead of "setting" and overwriting them */

        if (MVMSetAttr(VM,AIndex,AVal[sindex],mdfString,mAdd) == FAILURE)
          {
          snprintf(tmpLine,sizeof(tmpLine),"ERROR:  could not set attributes on VM %s\n",
            VM->VMID);

          MUISAddData(S,tmpLine);

          return(FAILURE);
          }

        MUISAddData(S,"successfully updated VM variables");

        return(SUCCESS);

        break;

      case mvmaPowerSelectState:
      case mvmaPowerState:

        {
        char EMsg[MMAX_LINE] = {0};
        enum MPowerStateEnum NextPower = mpowNONE;

        if ((!strcasecmp(AVal[sindex],"start")) || 
            (!strcasecmp(AVal[sindex],"on")))
          {
          NextPower = mpowOn;
          }
        else if ((!strcasecmp(AVal[sindex],"stop")) ||
                 (!strcasecmp(AVal[sindex],"off")))
          {
          NextPower = mpowOff;
          }

        if (NextPower != mpowNONE)
          {
          char proxy[MMAX_NAME];
          char *username;

          if (MRMNodeSetPowerState(VM->N,NULL,VM,NextPower,EMsg) == FAILURE)
            {
            snprintf(tmpLine,sizeof(tmpLine),"ERROR:  cannot power on VM %s - %s\n",
              VM->VMID,
              EMsg);

            MUISAddData(S,tmpLine);

            return(FAILURE);
            }

          if (MXMLGetAttr(S->RDE,"proxy",NULL,proxy,sizeof(proxy)))
            {
            username = proxy;
            }
          else
            {
            username = Auth;
            }

          MOWriteEvent((void *)VM,mxoxVM,(NextPower == mpowOn) ? mrelVMPowerOn : mrelVMPowerOff,NULL,NULL,username);

          MUISAddData(S,"successfully updated VM power state");
          }
        else
          {
          MUISAddData(S,"ERROR:  specified power state not supported\n");

          return(FAILURE);
          }

        return(SUCCESS);
        } /* END case mvmaPower... */

        break;

      /* Put attrs that will just use MVMSetAttr with strings here */

      case mvmaDescription:

        if (MVMSetAttr(VM,AIndex,AVal[sindex],mdfString,mSet) == FAILURE)
          {
          snprintf(tmpLine,sizeof(tmpLine),"ERROR:  could not modify attribute '%s' on VM '%s'\n",
            MVMAttr[AIndex],
            VM->VMID);

          MUISAddData(S,tmpLine);

          return(FAILURE);
          }

        snprintf(tmpLine,sizeof(tmpLine),"VM attribute '%s' successfully modified on VM '%s'\n",
          MVMAttr[AIndex],
          VM->VMID);

        MUISAddData(S,tmpLine);

        return(SUCCESS);

        /*NOTREACHED*/

        break;

      case mvmaActiveOS:

        {
        char EMsg[MMAX_LINE] = {0};

        if (MNodeProvision(VM->N,VM,NULL,AVal[sindex],MBNOTSET,EMsg,NULL) == FAILURE)
          {
          snprintf(tmpLine,sizeof(tmpLine),"ERROR:  cannot change os on VM %s:%s to '%s' - %s\n",
            VM->N->Name,
            VM->VMID,
            AVal[sindex],
            EMsg);

          MUISAddData(S,tmpLine);

          break;
          }

        /* Add response string into the tmpLine, allowing for more entries to be added (boundary check?) */
        snprintf(tmpLine,sizeof(tmpLine),"set attribute %s on VM %s to '%s'%s%s\n",
          MVMAttr[mvmaActiveOS],
          VM->VMID,
          AVal[sindex],
          (EMsg != NULL) ? " - " : "",
          (EMsg != NULL) ? EMsg : "");
        }

        break;

      case mvmaState:

        if (MVMSetState(VM,mnsNONE,AVal[sindex]) == FAILURE)
          snprintf(tmpLine,sizeof(tmpLine),"ERROR:  cannot change State on VM %s to %s\n",VM->VMID,AVal[sindex]);
        else
          snprintf(tmpLine,sizeof(tmpLine),"State changed on VM %s to %s\n",VM->VMID,AVal[sindex]);

        break;

      case mvmaTrigger:

        {
        if (MVMSetAttr(VM,mvmaTrigger,AVal[sindex],mdfString,mAdd) == FAILURE)
          {
          snprintf(tmpLine,sizeof(tmpLine),"ERROR:  cannot set trigger on VM %s\n",
            VM->VMID);
          }
        else
          {
          snprintf(tmpLine,sizeof(tmpLine),"trigger set on VM %s\n",
            VM->VMID);
          }
        }

        break;

      default:

        snprintf(tmpLine,sizeof(tmpLine),"ERROR:  cannot modify attribute '%s' for VMs - operation not supported\n",
          MVMAttr[AIndex]);

        MUISAddData(S,tmpLine);

        return(FAILURE);

        /*NOTREACHED*/

        break;
      } /* END switch (AIndex) */
    } /* END for (sindex = 0;AName[sindex][0] != '\0';sindex++) */
      
  /* If there is any data in the tmpLine buffer, send back to client */
  if (tmpLine[0] != '\0')
    MUISAddData(S,tmpLine);

  return(SUCCESS);
  } /* END __mvmcmModify() */


/**
 * Perform the VM Command: Destroy 
 *
 * @param     S                 (I) The socket
 * @param     VMExp             (I) [VM name]
 * @param     CFlags            (I) 
 */
int __mvmcmQuery(

  msocket_t  *S,
  char        VMExp[],
  mbitmap_t  *CFlags)
    
  {
  char tmpLine[MMAX_LINE];

  mxml_t *DE = NULL;
  mstring_t VMShowStr(MMAX_LINE);
  mvm_t  *VM;

  bmset(&S->Flags,msftReadOnlyCommand);

  if (bmisset(CFlags,mcmXML))
    {
    MXMLCreateE(&DE,(char *)MSON[msonData]);

    S->SDE = DE;
    }

  if (!strcmp(VMExp,"ALL"))
    {
    mhashiter_t VMIter;

    MUHTIterInit(&VMIter);

    while (MUHTIterate(&MSched.VMTable,NULL,(void **)&VM,NULL,&VMIter) == SUCCESS)
      {
      if (bmisset(CFlags,mcmXML))                                                                       
        {
        mxml_t *VE;

        MXMLCreateE(&VE,(char *)MXO[mxoxVM]);                                                          

        if (MVMToXML(VM,VE,NULL) == FAILURE)                                                           
          {
          snprintf(tmpLine,sizeof(tmpLine),"ERROR:  cannot convert VM '%s' to XML\n",                  
            VMExp);                                                                                    

          MUISAddData(S,tmpLine);                                                                      

          return(FAILURE);
          }                                                                                            

        MXMLAddE(DE,VE);
        } /* END if (bmisset(CFlags,mcmXML)) */
      else
        {
        MVMShow(&VMShowStr,VM,TRUE,FALSE);
        }
      } /* END while (MUHTIterate(&MSched.VMTable ...) == SUCCESS) */

    if (!bmisset(CFlags,mcmXML))
      {
      /* Can't print tmpLine in the loop because then we'll get repeats */

      if (MSched.VMTable.NumItems <= 0)
        {
        snprintf(tmpLine,sizeof(tmpLine),"no VMs found\n");

        MUISAddData(S,tmpLine);
        }
      else
        {
        MUISAddData(S,VMShowStr.c_str());
        }
      }

    return(SUCCESS);
    } /* END if (!strcmp(VMExp,ALL)) */
  else if (MVMFind(VMExp,&VM) == FAILURE)
    {
    snprintf(tmpLine,sizeof(tmpLine),"ERROR:  VM '%s' not found\n",
      VMExp);

    MUISAddData(S,tmpLine);

    return(FAILURE);
    }

  /* Just print one VM */

  if (bmisset(CFlags,mcmXML))
    {
    mxml_t *VE;

    MXMLCreateE(&VE,(char *)MXO[mxoxVM]);

    if (MVMToXML(VM,VE,NULL) == FAILURE)
      {
      snprintf(tmpLine,sizeof(tmpLine),"ERROR:  cannot convert VM '%s' to XML\n",
        VMExp);

      MUISAddData(S,tmpLine);

      return(FAILURE);
      }

    MXMLAddE(DE,VE); /* Added new VE element to the DE element */

    } /* END if (bmisset(CFlags,mcmXML)) */
  else
    {
    MVMShow(&VMShowStr,VM,TRUE,FALSE);

    MUISAddData(S,VMShowStr.c_str());
    }

  return(SUCCESS);  /* since we aren't using tmpLine, exit early! */
  }  /* END __mvmcmQuery() */


/**
 * Parse the "Where" Expression for the VM object
 *
 * @param   RDE       (I)
 * @param   UseRegex  (I)
 * @param   VMExp     (O)
 */

int __MUIVMParseWhereExpression(

  mxml_t    *RDE,
  mbool_t    UseRegex,
  char       VMExp[])

  {
  char    tmpName[MMAX_LINE];
  char    tmpVal[MMAX_LINE << 2];
  int     WTok;
  mxml_t *WE;

  /* Initialize VM Identifier buffer to empty string */
  VMExp[0] = '\0';

  WTok = -1;

  /* This function was based on MUINodeCtl, which had a UseRegex 
      is that needed for vms? */

  /* extract the "Where" expression */
  if (UseRegex != TRUE)
    {
    while (MS3GetWhere(
           RDE,
           &WE,
           &WTok,
           tmpName,
           sizeof(tmpName),
           tmpVal,
           sizeof(tmpVal)) == SUCCESS)
      {
      if (!strcmp(tmpName,MSAN[msanName]))
        {
        if (!strcmp(tmpVal,"ALL"))
          {
          strcpy(VMExp,"ALL");
          }
        else
          {
          MUStrCpy(VMExp,tmpVal,sizeof(tmpVal));
          }
        } /* END if (!strcmp(tmpName,MSAN[msanName])) */
      } /* END while (MS3GetWhere == SUCCESS) */
    } /* END if (UseRegex != TRUE) */
  else
    {
    while (MS3GetWhere(
           RDE,
           &WE,
           &WTok,
           tmpName,
           sizeof(tmpName),
           tmpVal,
           sizeof(tmpVal)) == SUCCESS)
      {
      if (!strcmp(tmpName,MSAN[msanName]))
        {
        snprintf(VMExp,sizeof(VMExp),"%s",
          tmpVal);
        }
      }
    } /* END else (UseRegex != TRUE) */

  return(SUCCESS);
  }



/**
 * Process VM control requests ('mvmctl').
 *
 * @param S - The socket
 * @param AFlags
 * @param Auth
 */

int MUIVMCtl(

  msocket_t *S,
  mbitmap_t *AFlags,
  char      *Auth)

  {
#define MMAX_VMCATTR 8
  char    SubCommand[MMAX_NAME];
  char    AName[MMAX_VMCATTR + 1][MMAX_LINE];
  char    AVal[MMAX_VMCATTR + 1][MMAX_LINE << 2];
  char    VMExp[MMAX_LINE << 2];
  char    OptString[MMAX_VMCATTR + 1][MMAX_NAME];
  char    OperationString[MMAX_VMCATTR + 1][MMAX_NAME];
  char    ArgString[MMAX_LINE * 6];
  char    OptArgString[MMAX_LINE];

  char    FlagString[MMAX_LINE];

  mbool_t UseRegex = FALSE;

  enum MVMCtlCmdEnum SCIndex;

  int     sindex;

  char    tmpLine[MMAX_LINE];

  mxml_t *RDE;
  mxml_t *SE;

  int     CTok;

  mbitmap_t  CFlags;

  mbool_t   EvalOnly = FALSE;  /* For -f migration - if TRUE, migrations will only be planned, not performed */
  mgcred_t *U = NULL;

  const char *FName = "MUIVMCtl";

  MDB(2,fUI) MLog("%s(S,%ld,%s)\n",
    FName,
    AFlags,
    (Auth != NULL) ? Auth : "NULL");

  /* No socket, no work-y */
  if (S == NULL)
    {
    return(FAILURE);
    }

  /* Clear socket Sending Element and Message containters to a known state, 
   * then set the wire protocol for the response message
   */
  MUISClear(S);

  S->WireProtocol = mwpS32;


  /* If socket has Read XML element already initialized, use it */
  if (S->RDE != NULL)
    {
    RDE = S->RDE;
    }
  else
    {
    /* Socket does not have the Read XML element yet, so construct a local
     * XML element pointer here. Will be destructed later after parsing
    */
    RDE = NULL;

    /* Parse the XML string into the local RDE mxml_t tree */
    if (MXMLFromString(&RDE,S->RPtr,NULL,NULL) == FAILURE)
      {
      MUISAddData(S,"ERROR:  corrupt command received\n");

      snprintf(tmpLine,sizeof(tmpLine),"ERROR:  invalid request received by command '%s'\n",
        S->RBuffer);

      MUISAddData(S,tmpLine);

      return(FAILURE);
      }
    }

  /* Using the Attribute Name 'Action' string, get its value - SubCommand */
  MXMLGetAttr(RDE,MSAN[msanAction],NULL,SubCommand,sizeof(SubCommand));

  /* Get the 'Flags' string */
  if (MXMLGetAttr(RDE,MSAN[msanFlags],NULL,FlagString,sizeof(FlagString)) == SUCCESS)
    {
    if (strstr(FlagString,"regex") != NULL)
      UseRegex = TRUE;

    if (strstr(FlagString,"eval") != NULL)
      EvalOnly = TRUE;

    bmfromstring(FlagString,MClientMode,&CFlags);
    }

  /* Initialize the socket SEND buffer */
  S->SBuffer[0] = '\0';

  /* Map the MVMCtl subcommand to its enum index for easier dispatch */
  SCIndex = (enum MVMCtlCmdEnum)MUGetIndexCI(
    SubCommand,
    MVMCtlCmds,
    FALSE,
    mvmcmNONE);

  /* Error check the result */
  if (SCIndex == mvmcmNONE)
    {
    sprintf(tmpLine,"ERROR:  invalid subcommand received (%s)\n",
      SubCommand);

    MUISAddData(S,tmpLine);

    return(FAILURE);
    }

  /* Perform user authentification */
  if ((MUserAdd(Auth,&U) == FAILURE) ||
      (MUICheckAuthorization(
        U,
        NULL,
        NULL,
        mxoxVM,
        mcsMVMCtl,
        SCIndex,      /* Sub-Command for authentication. */
        NULL,
        NULL,
        0) == FAILURE))
    {
    snprintf(tmpLine,sizeof(tmpLine),"ERROR:  user not authorized to run this command\n");

    MUISAddData(S,tmpLine);

    return(FAILURE);
    }


  /* Parse the "Where" expression for a VM tag */
  __MUIVMParseWhereExpression(RDE,UseRegex,VMExp);

  /* Did we find a VM Object Specified? (returned in VMExp) */
  if (VMExp[0] == '\0')
    {
    /* VMs must be created by job templates */

    MUISAddData(S,"ERROR:  no VM expression received\n");

    return(FAILURE);

#if 0
    if (SCIndex != mvmcmCreate)
      {
      MUISAddData(S,"ERROR:  no VM expression received\n");

      return(FAILURE);
      }
    else
      {
      /* Creating a VM, pick the next unique name */

      MVMGetUniqueID(VMExp);
      }
#endif
    } /* END if (VMExp[0] == '\0') */


  /* We have a VM name, so parse out 'args', 'option', 'name', 'value' fields */
  MXMLGetAttr(RDE,MSAN[msanArgs],NULL,ArgString,sizeof(ArgString));
  MXMLGetAttr(RDE,MSAN[msanOption],NULL,OptArgString,sizeof(OptArgString));

  /* Build up an array of AName:AVal pairs from what we find in the input stream */
  sindex = 0;      /* First Attribute Name instance */

  MXMLGetAttr(RDE,MSAN[msanName],NULL,AName[sindex],sizeof(AName[0]));
  MXMLGetAttr(RDE,MSAN[msanValue],NULL,AVal[sindex],sizeof(AVal[0]));

  /* If there was an Attribute Name Instance, bump for next instance */
  if (AName[0][0] != '\0')
    sindex++;


  /* iterate over this element's childern looking for the 'msonSet' strings
   * and when found extract its: Attr Name, Value, Opt, Operations 
   */
  CTok = -1;

  while (MXMLGetChild(RDE,(char *)MSON[msonSet],&CTok,&SE) == SUCCESS)
    {
    MXMLGetAttr(SE,MSAN[msanName],NULL,AName[sindex],sizeof(AName[0]));
    MXMLGetAttr(SE,MSAN[msanValue],NULL,AVal[sindex],sizeof(AVal[0]));
    MXMLGetAttr(SE,MSAN[msanOption],NULL,OptString[sindex],sizeof(OptString[0]));
    MXMLGetAttr(SE,MSAN[msanOp],NULL,OperationString[sindex],sizeof(OperationString[0]));

    sindex++;

    if (sindex >= MMAX_VMCATTR)
      break;
    }

  AName[sindex][0] = '\0';   /* NULL string ending entry for a END of List marker */
  AVal[sindex][0] = '\0';


  /* If the socket Read Element is NULL, that means we used a local element
   * therefore we need to destruct the local one before continuing
   */
  if (S->RDE == NULL)
    {
    MXMLDestroyE(&RDE);
    }


  /* now displatch the VM Action command we received */

  switch (SCIndex)
    {
#if 0
    case mvmcmCreate:

      return(MUIVMCreate(S,Auth,NULL,VMExp,ArgString));  /* node name is passed in via ArgString */

      break;
#endif

    case mvmcmDestroy:

      /* Go and process the Destroy operation */
      return(__mvmcmDestroy(S,Auth,VMExp,&CFlags));

      break;

    case mvmcmMigrate:

      /* Go and process the Migrate operation */
      return(__mvmcmMigrate(S,Auth,VMExp,ArgString));

      break;

    case mvmcmModify:

      /* Go and process the Modify operation */
      return(__mvmcmModify(S,Auth,VMExp,AName,AVal,OperationString));

      break;

    case mvmcmQuery:

      /* Go and process the Query operation */
      return(__mvmcmQuery(S,VMExp,&CFlags));

      break;

    case mvmcmForceMigration:

      /* Go and process the ForceMigration operation */
      return(__mvmcmForceMigration(S,Auth,VMExp,&CFlags,EvalOnly));

      break;

    default:

      snprintf(tmpLine,sizeof(tmpLine),"ERROR:  invalid command for mvmctl");
      MUISAddData(S,tmpLine);
      return(FAILURE);

      break;
    }

  /* NOTREACHED */

  return(SUCCESS);
  } /* END MUIVMCtl */


/**
 * Helper function to print out a decision list to the socket
 *
 * No error checking.
 *
 * @see MUIVMCtlPlanMigrations() - parent
 *
 * @param XML                [I] - If TRUE, return format is XML, string otherwise
 * @param IntendedMigrations [I] - The list of intended migrations (not empty)
 * @param S                  [O] - The socket to print information to
 */

int __EvalOnlyPrintDecisions(

  mbool_t XML,
  mln_t  *IntendedMigrations,
  msocket_t *S)

  {
  mln_t *tmpL = NULL;
  mvm_migrate_decision_t *Decision = NULL;

  if (XML)
    {
    if (MXMLCreateE(&S->SDE,(char *)MSON[msonData]) == FAILURE)
      {
      MUISAddData(S,"ERROR:    cannot create response\n");

      return(FAILURE);
      }

    for (tmpL = IntendedMigrations;tmpL != NULL;tmpL = tmpL->Next)
      {
      mxml_t *E = NULL;

      Decision = (mvm_migrate_decision_t *)tmpL->Ptr;

      if (MXMLCreateE(&E,"Migration") == FAILURE)
        {
        MXMLDestroyE(&S->SDE);
        MUISAddData(S,"ERROR:    cannot create response\n");

        return(FAILURE);
        }

      MXMLSetAttr(E,"VM",Decision->VM->VMID,mdfString);
      MXMLSetAttr(E,"Dst",Decision->DstNode->Name,mdfString);
      MXMLSetAttr(E,"Src",Decision->SrcNode->Name,mdfString);

      MXMLAddE(S->SDE,E);
      }  /* END for (tmpL = IntendedMigrations) */
    }  /* END if (XML) */
  else
    {
    mstring_t Result(MMAX_LINE);
    int count = 1;

    for (tmpL = IntendedMigrations;tmpL != NULL;tmpL = tmpL->Next)
      {
      Decision = (mvm_migrate_decision_t *)tmpL->Ptr;

      MStringAppendF(&Result,"%d:  VM '%s' from '%s' to '%s'\n",
        count,
        Decision->VM->VMID,
        Decision->SrcNode->Name,
        Decision->DstNode->Name);

      count++;
      }

    MUISAddData(S,Result.c_str());
    }  /* END else (!XML) */

  FreeVMMigrationDecisions(&IntendedMigrations);

  return(SUCCESS);
  }  /* END __EvalOnlyPrintDecisions() */



/**
 * Helper function to take care of performing migrations from a client request
 *  No error checking.
 *
 * @see MUIVMCtl() - parent
 *
 * @param MigrationType [I] - The migration type to perform
 * @param EvalOnly      [I] - If TRUE, just return XML list of all of the decisions (don't actually migrate)
 * @param XML           [I] - If TRUE, return format is XML (if EvalOnly is on)
 * @param S             [O] - (optional) Output socket
 */

int MUIVMCtlPlanMigrations(
  enum MVMMigrationPolicyEnum MigrationType,
  mbool_t                     EvalOnly,
  mbool_t                     XML,
  msocket_t                  *S)

  {
  int ccode = SUCCESS;
  int NumberPerformed = 0;
  mln_t *IntendedMigrations = NULL;
  int MigrationsQueued = 0;
  char tmpLine[MMAX_LINE];
  int  tmpSize = MMAX_LINE;

  ccode = PlanVMMigrations(MigrationType,TRUE,&IntendedMigrations);

  if (ccode == FAILURE)
    {
    snprintf(tmpLine,tmpSize,"Failed to plan migrations");

    MUISAddData(S,tmpLine);

    return(FAILURE);
    }

  MigrationsQueued = MULLGetCount(IntendedMigrations);

  if (MigrationsQueued == 0)
    {
    snprintf(tmpLine,tmpSize,"No migrations to perform");

    MUISAddData(S,tmpLine);

    return(ccode);
    }

  if (EvalOnly == TRUE)
    {
    /* Evaluating only, print decisions that were made */

    if (S == NULL)
      return(ccode);

    ccode = __EvalOnlyPrintDecisions(XML,IntendedMigrations,S);

    return(ccode);
    }  /* END if (EvalOnly == TRUE) */

  /* Perform migrations */

  ccode = PerformVMMigrations(&IntendedMigrations,&NumberPerformed,(char *)MVMMigrationPolicy[MigrationType]);

  if (ccode == SUCCESS)
    {
    snprintf(tmpLine,tmpSize,"%d of %d migrations performed",
      NumberPerformed,
      MigrationsQueued);
    }
  else
    {
    snprintf(tmpLine,tmpSize,"Failed to perform migrations");
    }

  if (tmpLine[0] != '\0')
    MUISAddData(S,tmpLine);

  return(ccode);
  }  /* END MUIVMCtlPlanMigrations() */

/* END MUIMVCtl.c */
