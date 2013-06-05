/**
 * @file MJobPBS.c
 *
 * Declares the APIs for job functions that deal with pbs jobs.
 */

#include "moab.h"
#include "moab-proto.h"
#include "moab-const.h"
#include "moab-global.h"
#include <sstream>
#include <vector>
#include <string>

using namespace std;


/**
 * Return full tasklist for GPU exec lists.
 * 
 * @see MJobTranslateNCPUTaskList (translates the full tasklist) 
 *
 * @param J (I) [modified]
 * @param TaskString (I) [modified]
 * @param TaskList (I) [minsize=MSched.JobMaxTaskCount]
 */

int MPBSJobGetAcceleratorExecHList(

  mjob_t  *J,
  char    *TaskString,
  int     *TaskList)

  {
  char *ptr;
  char *TokPtr;
  char *TokPtr2;
  char *tail;
  char  tmpHostName[MMAX_NAME];

  mnode_t *N;

  int tindex = 0;

  if ((J == NULL) || (TaskList == NULL) || (TaskString == NULL))
    {
    return(FAILURE);
    }

  if (!MJOBISACTIVE(J))
    return(SUCCESS);

  if (!bmisset(&J->IFlags,mjifPBSGPUSSpecified))
    return(SUCCESS);

  ptr = MUStrTok(TaskString,"+ \t",&TokPtr);

  /* ignore for now */

  TaskList[0] = -1;

  while (ptr != NULL)
    {
    MUStrCpy(tmpHostName,ptr,sizeof(tmpHostName));

    MUStrTok(tmpHostName,":",&TokPtr2);

    /* remove virtual host id */

    if ((tail = strchr(tmpHostName,'/')) != NULL)
      *tail = '\0';

    if (MNodeFind(tmpHostName,&N) == SUCCESS)
      {
      TaskList[tindex] = N->Index;

      tindex++;
      }

    ptr = MUStrTok(NULL,"+ \t",&TokPtr);
    }  /* END while (ptr != NULL) */

  TaskList[tindex] = -1;

  return(SUCCESS);
  }  /* END MPBSJobGetAcceleratorExecHList() */



/**
 * Insert PBS directive into string buffer.
 *
 * @see MJobPBSRemoveArg() - peer
 *
 * @param AName (I)
 * @param AVal (I) [optional, FORMAT:  <ATTR>[=<VAL>]]
 * @param SrcBuf (I)
 * @param DstBuf (O)
 * @param DstBufSize (I)
 */

int MJobPBSInsertArg(

  const char *AName,
  const char *AVal,
  char       *SrcBuf,
  char       *DstBuf,
  int         DstBufSize)

  {
  int  rc;

  char tmpLine[MMAX_BUFFER];

  char NewBuffer[MMAX_SCRIPT];

  const char *FName = "MJobPBSInsertArg";

  MDB(3,fRM) MLog("%s(%s,%s,SrcBuf,DstBuf,%d)\n",
    FName,
    AName,
    (AVal == NULL) ? "" : AVal,
    DstBufSize);

  if ((AName == NULL) || (SrcBuf == NULL) || (DstBuf == NULL))
    {
    return(FAILURE);
    }

  if ((AVal != NULL) && (MUStrIsEmpty(AVal)))
    {
    /* If an AVal was specified then make sure that it isn't empty. */

    return(SUCCESS);
    }

  if (DstBuf[0] != '#')
    {
    rc = snprintf(tmpLine,sizeof(tmpLine),"#PBS %s %s\n",
      AName,
      (AVal == NULL) ? "" : AVal);
    }
  else
    {
    rc = snprintf(tmpLine,sizeof(tmpLine),"\n#PBS %s %s\n",
      AName,
      (AVal == NULL) ? "" : AVal);
    }

  if (rc >= (int)sizeof(tmpLine))
    {
    MDB(3,fRM) MLog("ALERT:    PBS directive '%.32s...' is too long in %s\n",
      tmpLine,
      FName);

    return(FAILURE);
    }

  /* NOTE:  PBS attr comments must occur before any non-comment lines */

  if (DstBuf == SrcBuf)
    {
    char *ptr;
    char *TokPtr;

    char *BPtr = NULL;
    int   BSpace = 0;

    /* FIXME: put PBS attr comment as last comment line */

    MUStrCpy(NewBuffer,SrcBuf,sizeof(NewBuffer));

    ptr = MUStrTok(NewBuffer,"\n",&TokPtr);

    MUSNInit(&BPtr,&BSpace,DstBuf,DstBufSize);

    while ((ptr != NULL) && (ptr[0] == '#'))
      {
      MUSNPrintF(&BPtr,&BSpace,"%s\n",
        ptr);

      ptr = MUStrTok(NULL,"\n",&TokPtr);
      }

    MUSNPrintF(&BPtr,&BSpace,"%s%s\n%s",
      tmpLine,
      (ptr == NULL) ? " " : ptr,
      (TokPtr == NULL) ? " " : TokPtr);

    if (BSpace <= 0)
      {
      MDB(3,fRM) MLog("ALERT:    cannot insert new PBS directive '%.32s...' into buffer in %s, buffer too small\n",
        tmpLine,
        FName);

      return(FAILURE);
      }

    return(SUCCESS);
    }

  rc = snprintf(DstBuf,DstBufSize,"%s%s",
    tmpLine,
    SrcBuf);

  return(SUCCESS);
  }  /* END MJobPBSInsertArg() */


/**
 * @see MJobPBSInserArg() - peer
 * @see MJobPBSExtractArg() - peer
 *
 * FORMAT:  #PBS -<FLAG> [<ATTR>=<VAL>]
 * NOTE: this function removes everything after AVal. Don't this to remove
 *       nested AVals. For example, removing -l walltime from "-l walltime=<time>,size=<#>"
 *       will get rid of size as well. Use MJobPBSExtractArg for these situations.
 *
 * @param AName (I)
 * @param AVal (I)
 * @param SrcBuf (I)
 * @param DstBuf (O)
 * @param DstBufSize (I)
 */

int MJobPBSRemoveArg(

  const char  *AName,       /* I */
  const char  *AVal,        /* I */
  char  *SrcBuf,      /* I */
  char  *DstBuf,      /* O */
  int    DstBufSize)  /* I */

  {
  char *ptr;
  char *head;
  char *tail;

  const char *FName = "MJobPBSRemoveArg";

  MDB(1,fRM) MLog("%s(%s,%s,SrcBuf,DstBuf,%d)\n",
    FName,
    AName,
    AVal,
    DstBufSize);

  if ((AName == NULL) || (SrcBuf == NULL) || (DstBuf == NULL))
    {
    return(FAILURE);
    }

  if (SrcBuf != DstBuf)
    strcpy(DstBuf,SrcBuf);

  /* FORMAT:  #PBS -<FLAG> [<ATTR>=<VAL>] */

  head = strstr(DstBuf,"#PBS ");

  while (head != NULL)
    {
    ptr = head + strlen("#PBS ");

    if (strncmp(ptr,AName,strlen(AName)))
      {
      head = strstr(ptr,"#PBS ");

      continue;
      }

    if ((AVal != NULL) && (AVal[0] != '\0'))
      {
      /* skip arg name plus white space */

      ptr += strlen(AName) + 1;

      if (strncmp(ptr,AVal,strlen(AVal)))
        {
        head = strstr(ptr,"#PBS ");

        continue;
        }
      }    /* END if ((AVal != NULL) && (AVal[0] != '\0')) */

    head[1] = '#'; 

    tail = head + strlen(head);

    for (ptr = head + 2;ptr < tail;ptr++)
      {
      if (*ptr == '\n')
        break;

      *ptr = ' ';
      }  /* END for (ptr) */

    head = strstr(ptr,"#PBS ");
    }    /* END while (ptr != NULL) */

  return(SUCCESS);
  }  /* END MJobPBSRemoveArg() */



/**
 * Check for the existance of a RM directive at the start of a Line. 
 * return 0 if none, else return the length of the directive. 
 *
 * @see MJobPBSExtractArg()
 *
 * @param Line (I)
 */

int IsRMDirective(

  char   *Line)
  {
  
  if (Line == NULL)
    return(0);
  else if (strncmp(Line, "#PBS", strlen("#PBS"))==0)
    {
    return strlen("#PBS");
    }
  else if (strncmp(Line, "#MSUB", strlen("#MSUB"))==0)
    {
    return strlen("#MSUB");
    }
  else
    return(0);
  }





/**
 * Extract PBS directive values and optionally remove args from command buffer.
 *
 * Only handles lines that have one AName. For example searching for -N in 
 * #PBS -l walltime=# -N name will fail. Also, searching for a SAName of walltime in 
 * #PBS -l nodes=1 -l walltime=# will fail as well.
 *
 * Handles the case where removing -l walltime from -l walltime=<time>,size=<#>.
 * -l size=<#> will remain after the extraction. 
 *  
 * NOTE: This function will modify the SrcBuf if DoRemoveArg is TRUE.  It does 
 *       modify the buffer always, but returns it to it's original state if
 *       DoRemoveArg is FALSE.
 *
 * NOTE: can return SUCCESS with empty AVal
 *
 * @see MJobPBSRemoveArg() - peer
 *
 * @param AName (I) [set to '\0' to remove first attribute] attribute name
 * @param SAName (I) [optional] subattribute name
 * @param SrcBuf (I) [modified]
 * @param AVal (O) [optional,minsize=AValSize]
 * @param AValSize (I)
 * @param DoRemoveArg (I)
 */

int MJobPBSExtractArg(

  const char *AName,
  const char *SAName,
  char       *SrcBuf,
  char       *AVal,
  int         AValSize,
  mbool_t     DoRemoveArg)

  {
  char *ptr=NULL, *nextline=NULL;  /* beginning line pointer - points to first character in 
                             RM directive line which should be removed */

  char *tail = NULL;

  mbool_t ArgFound = FALSE;

  int DirectiveLen = 0;
  
  const char *FName = "MJobPBSExtractArg";

  MDB(1,fRM) MLog("%s(%s,%s,SrcBuf,AVal,%d,%s)\n",
    FName,
    (AName != NULL) ? AName : "NULL",
    (SAName != NULL) ? SAName : "NULL",
    AValSize,
    MBool[DoRemoveArg]);

  if ((AName == NULL) || (SrcBuf == NULL))
    {
    return(FAILURE);
    }

  /* return SUCCESS if arg found and if arg not found */

  if (AVal != NULL)
    AVal[0] = '\0';

  /* locate arg */
  /* ptr is working variable which moves through SrcBuf as it is processed */

  ptr = SrcBuf;

  while(*ptr)
    {
    char *subAttr, *subAttrVal;  /* line pointer - will point to first character in sub-
                                   attribute list */

    /* Point to the beginning of the line.  If a comma separated list of subattributes
       is found later, this will be reset to point to the subattribute.*/

    subAttr = ptr;

    /* Locate the end of the line */
    tail = strchr(ptr,'\n');

    if (tail != NULL)
      {
      /* terminate line temporarily */
      *tail = '\0';
      nextline = tail+1;
      }
    else
      {
      nextline = ptr + strlen(ptr);
      }

    /* Get the Directive if any.  Skip lines with no directive. */
    if ((DirectiveLen = IsRMDirective(ptr)) == 0)
      {
      if (tail)
        {
        *tail = '\n';
        }
      ptr = nextline;
      continue;
      }
    
    /* Step past the Directive and any white space following it. */
    ptr += DirectiveLen;
    while(isspace(*ptr))
      ptr++;

    /* FORMAT:  <ANAME> [...,]<SANAME>=<VAL>[,...] */

    /* Is this the attribute we are looking for? If not, skip the line. */
    if (strncmp(ptr,AName,strlen(AName))!=0)
      {
      if (tail)
        {
        *tail = '\n';
        }
      ptr = nextline;
      continue;
      }

    /* Step past the attribute and following whitespace. */
    ptr += strlen(AName);
    while (isspace(*ptr))
      ptr++;

    ArgFound = TRUE;

    /* ptr points to first sub-attribute, at this point, line is temporarily terminated */

    if (SAName != NULL)
      {
      /* sub-attribute specified */

      char *lptr;  /* list pointer - points to first character in sub-
                                     attribute list */

      char *cptr;  /* comma pointer - set if comma exists w/in directive */

      lptr = ptr;

      if ((ptr = strstr(ptr,SAName)) == NULL)
        {
        /* subattr not found, continue with next line  */

        if (tail != NULL)
          {
          *tail = '\n';
          }
        ptr = nextline;
        continue;
        }

      /* subattr located, subAttr points to first character of sub-attribute name */

      if (strchr(ptr,','))
        {
        /* attribute comma located */

        subAttr = ptr;
        }

      cptr = strchr(ptr,',');

      if (cptr != NULL)
        {
        *cptr = '\0';
        }

      ptr += strlen(SAName);

      if (ptr[0] == '=')
        ptr++;

      /* subAttrVal points to start of Sub Attribute Value */
      subAttrVal = ptr;
      
      /* arg located, copy to AVal */

      if (AVal != NULL)
        {
        MUStrCpy(AVal,subAttrVal,AValSize);
        }

      if (DoRemoveArg == TRUE)
        {
        int subAttrLen;

        /* NOTE:  clearing attribute by inserting spaces will break TORQUE, ie 
             '-l nodes=1,walltime=2,mem=3' works, but
             '-l nodes=1,           mem=3' does not. */

        subAttrLen = strlen(subAttr);

        memset(subAttr,' ',subAttrLen);

        if (cptr != NULL)
          {
          /* clear post-attribute comma */

          *cptr = ' ';

          MUBufRemoveChar(
            subAttr,
            ' ',
            '\n',
            '\n');
          }
        else if ((subAttr > lptr) && (*(subAttr - 1) == ','))
          {
          /* clear pre-attribute comma located without post-attribute 
             attribute, ie ',<ATTR>=<VAL>' */

          *(subAttr - 1) = ' ';

          MUBufRemoveChar(
            subAttr - 1,
            ' ',
            '\n',
            '\n');
          }
        }
      else if (cptr != NULL)
        {
        /* restore comma which was temporarily removed for parsing */

        *cptr = ',';
        }

      break;
      }    /* END if (SAName != NULL) */

    /* ptr points to start of SAList */

    /* arg located, copy to AVal */

    if (AVal != NULL)
      {
      MUStrCpy(AVal,ptr,AValSize);
      }

    if (DoRemoveArg == TRUE)
      {
      memset(subAttr,' ',strlen(subAttr));
      }

    /* We've handled the directive and its attribute. We don't need to process
       any more lines. */
    break;
    
    }    /* END while ((ptr = strstr(ptr,"#PBS")) != NULL) */

  if (tail != NULL)
    {
    *tail = '\n';
    }

  if (ArgFound == FALSE)
    {
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MJobPBSExtractArg() */



/**
 * Extracts resource manager extension attributes.
 * 
 * ex. -W x=size:1
 *
 * @param SAName      (I) subattribute name (optional)
 * @param SrcBuf      (I) (modified)
 * @param AVal        (O) (optional,minsize=AValSize)
 * @param AValSize    (I)
 * @param DoRemoveArg (I)
 */

int MJobPBSExtractExtensionArg(

  const char *SAName,
  char       *SrcBuf,
  char       *AVal,
  int         AValSize,
  mbool_t     DoRemoveArg)

  {
  char *ptr;

  char *bptr;          /* beginning line pointer - points to first character in 
                          RM directive line which should be removed */

  char *tail = NULL;

  mbool_t ArgFound = FALSE;

  const char *AName = "-W";
  const char *Extension = "x=";

  const char *FName = "MJobPBSExtractExtensionArg";

  MDB(1,fRM) MLog("%s(%s,SrcBuf,AVal,%d,%s)\n",
    FName,
    (SAName != NULL) ? SAName : "NULL",
    AValSize,
    MBool[DoRemoveArg]);

  if ((SrcBuf == NULL) || (SAName == NULL))
    {
    return(FAILURE);
    }

  /* return SUCCESS if arg found and if arg not found */

  if (AVal != NULL)
    AVal[0] = '\0';

  /* locate arg */

  ptr = SrcBuf;

  while ((ptr = strstr(ptr,"#PBS")) != NULL)
    {
    /* bptr points to start of new directive line */
    /* ptr is working variable which moves along line as line is processed */

    bptr = ptr;

    ptr += strlen("#PBS");

    while (isspace(*ptr))
      ptr++;

    if (strncmp(ptr,AName,strlen(AName)))
      {
      continue;
      }

    /* FORMAT:  -W x=[;]<SANAME>[=:]<VAL>[;] */

    tail = strchr(ptr,'\n');

    if (tail != NULL)
      {
      /* terminate line */

      *tail = '\0';
      }

    ptr += strlen(AName);

    while (isspace(*ptr))
      ptr++;

    /* Check for x= */

    if (strncmp(ptr,Extension,strlen(Extension)))
      {
      continue;
      }

    ptr += strlen(Extension);
  
    ArgFound = TRUE;

    /* ptr points to first sub-attribute, at this point, line is temporarily terminated */

    if (SAName != NULL)
      {
      /* sub-attribute specified */

      char *lptr;  /* line pointer - points to first character in sub-
                                     attribute list */

      char *cptr;  /* comma pointer - set if comma exists w/in directive */

      lptr = ptr;

      if ((ptr = strstr(ptr,SAName)) == NULL)
        {
        /* subattr not found */

        if (tail != NULL)
          {
          *tail = '\n';
          }

        ptr = lptr;

        continue;
        }

      /* subattr located, ptr points to first character of sub-attribute name */

      if (strchr(bptr,';'))
        {
        /* attribute comma located */

        bptr = ptr;
        }

      cptr = strchr(ptr,';');

      if (cptr != NULL)
        {
        *cptr = '\0';
        }

      ptr += strlen(SAName);

      if ((ptr[0] == '=') || (ptr[0] == ':'))
        ptr++;

      /* ptr points to start of SAVal */

      /* arg located, copy to AVal */

      if (AVal != NULL)
        {
        MUStrCpy(AVal,ptr,AValSize);
        }

      if (DoRemoveArg == TRUE)
        {
        int BLen;

        /* NOTE:  clearing attribute by inserting spaces will break TORQUE, ie 
             '-l nodes=1,walltime=2,mem=3' works, but
             '-l nodes=1,           mem=3' does not. */

        BLen = strlen(bptr);

        memset(bptr,' ',BLen);

        if (cptr != NULL)
          {
          /* clear post-attribute comma */

          *cptr = ' ';

          MUBufRemoveChar(
            bptr,
            ' ',
            '\n',
            '\n');
          }
        else if ((bptr > lptr) && (*(bptr - 1) == ';'))
          {
          /* clear pre-attribute delimiter located without post-attribute 
             attribute, ie '[;]<ATTR>=<VAL>' */

          *(bptr - 1) = ' ';

          MUBufRemoveChar(
            bptr - 1,
            ' ',
            '\n',
            '\n');
          }
        }
      else if (cptr != NULL)
        {
        /* restore ';' which was temporarily removed for parsing */

        *cptr = ';';
        }

      break;
      }    /* END if (SAName != NULL) */

    /* ptr points to start of SAList */

    /* arg located, copy to AVal */

    if (AVal != NULL)
      {
      MUStrCpy(AVal,ptr,AValSize);
      }

    if (DoRemoveArg == TRUE)
      {
      memset(bptr,' ',strlen(bptr));
      }

    break;
    }    /* END while ((ptr = strstr(ptr,"#PBS")) != NULL) */

  if (tail != NULL)
    {
    *tail = '\n';
    }

  if (ArgFound == FALSE)
    {
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MJobPBSExtractExtensionArgArg() */



/**
 * Processes a colon delimited list of pbs resources.
 *
 * This is a helper function to MJobGetSelectPBSTaskList() and MJobSelectPBSVTaskList(). 
 * 
 * ex. ncpus=1:mem=10GB
 * @see MJobGetSelectPBSTaskList()
 * @see MJobSelectPBSVTaskList()
 *
 * @param J (I) Job associated with resources.
 * @param RQ (I) The req to assign info to.
 * @param RList (I) Colon delimited list of resources.
 * @param N (O) return found host or vnode [possibly modified] 
 * @param PPN (O) to return found ppn [optional,possibly modified]
 * @param HostSpecified (O) host= was specified. 
 */

int MJobPBSProcessResources(

  mjob_t   *J,             /* I */
  mreq_t   *RQ,            /* I */
  char     *RList,         /* I */
  mnode_t **N,             /* O (possibly modified)*/
  int      *PPN,           /* O (optional,possibly modified)*/
  mbool_t  *HostSpecified) /* O */

  {
  char    *ptr;
  char    *TokPtr;
  int      ppn;
  
  const char *FName = "MJobPBSProcessResources";

  if (RList == NULL)
    {
    MDB(1,fPBS) MLog("ERROR:    RList was passed as NULL into %s\n",FName);

    return(FAILURE);
    }

  if (RQ == NULL)
    {
    MDB(1,fPBS) MLog("ERROR:    RQ was passed as NULL into %s\n",FName);

    return(FAILURE);
    }

  if (N == NULL)
    {
    MDB(1,fPBS) MLog("ERROR:    N was passed as NULL into %s\n",FName);

    return(FAILURE);
    }

  if (HostSpecified == NULL)
    {
    MDB(1,fPBS) MLog("ERROR:    HostSpecified was passed as NULL into %s\n",FName);

    return(FAILURE);
    }

  *HostSpecified = FALSE;

  ppn = 1;

  ptr = MUStrTok(RList,":",&TokPtr);

  while (ptr != NULL)
    {
    if (isdigit(ptr[0]))
      {
      /* NO-OP */
      }
    else if (!strncmp(ptr,"ncpus=",strlen("ncpus=")))
      {
      ppn = (int)strtol(ptr + strlen("ncpus="),NULL,10);
 
      if (MJobWantsSharedMem(J))
        {
        /* Map ncpus to task count */

        RQ->DRes.Procs = ppn;
        }
      else if (bmisset(&J->IFlags,mjifPBSPPNSpecified))
        {
        /* ncpus translated from ppn spec - set as TPN */

        RQ->TaskCount *= ppn;
        }
      }
    else if (!strncmp(ptr,"mem=",strlen("mem=")))
      {
      /* SharedMem: don't process mem since it is per task/node memory. */

      if (MJobWantsSharedMem(J))
        {
        RQ->ReqMem = (int)MURSpecToL(ptr + strlen("mem="),mvmMega,mvmMega);

        if (RQ->TaskCount > 0)
          RQ->ReqMem *= RQ->TaskCount;
        }
      }
    else if (!strncmp(ptr,"pmem=",strlen("pmem=")))
      {
      RQ->DRes.Mem = (int)MURSpecToL(ptr + strlen("pmem="),mvmMega,mvmMega);
      }
    else if (!strncmp(ptr,"pvmem=",strlen("pvmem=")))
      {
      RQ->DRes.Swap = (int)MURSpecToL(ptr + strlen("pvmem="),mvmMega,mvmMega);
      }
    else if (!strncmp(ptr,"file=",strlen("file=")))
      {
      RQ->DRes.Disk = (int)MURSpecToL(ptr + strlen("file="),mvmMega,mvmMega);
      }
    else if (!strncmp(ptr,"arch=",strlen("arch=")))
      {
      RQ->Arch = MUMAGetIndex(meArch,ptr + strlen("arch="),mAdd);
      }
	else if (!strncmp(ptr,"opsys=",strlen("opsys=")))
      {
      RQ->Opsys = MUMAGetIndex(meOpsys,ptr + strlen("opsys="),mAdd);
      }
    else if (!strncmp(ptr,"qlist=",strlen("qlist=")))
      {
      /* NO-OP */
      }
    else if ((!strncmp(ptr,"host=",strlen("host="))) ||
             (!strncmp(ptr,"vnode=",strlen("vnode="))))
      {
      /* load task list info */

      /* NYI - there could be a conflict if someone was trying to schedule both
         hosts and vnodes */

      if ((!strncmp(ptr,"host=",strlen("host="))) && 
          (MNodeFind(ptr + strlen("host="),N) == FAILURE))
        {
        MDB(1,fPBS) MLog("ALERT:    cannot locate host '%s' for job hostlist\n",
          ptr + strlen("host="));

        return(FAILURE);
        }
      else if ((!strncmp(ptr,"vnode=",strlen("vnode="))) && 
               (MNodeFind(ptr + strlen("vnode="),N) == FAILURE))
        {
        MDB(1,fPBS) MLog("ALERT:    cannot locate vnode '%s' for job hostlist\n",
          ptr + strlen("vnode="));

        return(FAILURE);
        }

      *HostSpecified = TRUE;
      }
    else
      {
      /* feature requirement specified */

#ifdef __STEST
      if (TRUE)
#else
      if ((J->SubmitRM != NULL) &&
          (J->SubmitRM->Version >= 710) &&
          (strchr(ptr,'=') != NULL)) /* ReqFBM can be set via Resources_List.<boolean> in PBSPro > 7.1 */
#endif
        {
        char *ptr3;
        char *TokPtr3 = NULL;

        ptr3 = MUStrTok(ptr,"=",&TokPtr3);

        if (MUGetIndexCI(TokPtr3,MBoolString,FALSE,-1) == -1)
          {
          MDB(1,fPBS) MLog("ALERT:    invalid node feature requested '%s=%s', ignoring\n",
            ptr3,
            TokPtr3);
          }
        else
          {
          MReqSetAttr(J,RQ,mrqaReqNodeFeature,(void **)ptr3,mdfString,mAdd);
          }
        }
      else
        {
        MReqSetAttr(J,RQ,mrqaReqNodeFeature,(void **)ptr,mdfString,mAdd);
        }
      }

    ptr = MUStrTok(NULL,":",&TokPtr);
    }  /* END while (ptr != NULL) */    

  if (PPN != NULL)
    *PPN = ppn; /* pass ppn back */

  return(SUCCESS);
  } /* END int MJobPBSProcessResources() */



/**
 * Inserts the job's MOAB environment variables in pbs RM.
 *
 * @param J (I)
 * @param R (R)
 */

int MJobPBSInsertEnvVarsIntoRM(

  mjob_t *J,
  mrm_t  *R)

  {
  int rc = SUCCESS;

  MASSERT(J != NULL,"bad job pointer when adding env vars to rm\n");
  MASSERT(R != NULL,"bad rm pointer when adding env vars to rm\n");

  if (bmisset(&J->IFlags,mjifEnvRequested) == TRUE) /* if -E was specified */
    {
    char   Message[MMAX_LINE];

    /* Append moab job environment variables on job. */

    mstring_t tmpEnvVars(MMAX_BUFFER);

    if (MJobGetEnvVars(J,',',&tmpEnvVars) == FAILURE)
      {
      MDB(7,fPBS) MLog("ERROR:    failed getting job's environment variables\n");
      
      rc = FAILURE;
      }
    else
      {
      MDB(7,fPBS) MLog("INFO:     setting job's moab env variable list with: %s\n",tmpEnvVars.c_str());

      if ((R->Version < 253) && (J->Env.BaseEnv != NULL))
        MStringAppendF(&tmpEnvVars,",%s",J->Env.BaseEnv);

      rc = MRMJobModify(J,"Variable_List",NULL,tmpEnvVars.c_str(),mIncr,NULL,Message,NULL);

      if (rc == FAILURE)
        MDB(7,fPBS) MLog("ERROR:    failed setting job's env variables\n");
      }
    }

  return(rc);
  } /* MJobPBSInsertEnvVarsIntoRM */



/**
 * Remove all instances of the given attribute from the command buffer.
 *
 * Will remove cases of "-l attribute" as well as "-W x=attribute".
 *
 * optionally returns the last removed instance in OBuf
 *
 * @param AName     (I) '-l' or '-W x=' (if set only removes the given instances)
 * @param SAName    (I) the attribute-name to remove
 * @param SrcBuf    (I) the command buffer [modified]
 * @param OBuf      (O) if not null, returns the last instance removed
 * @param OBufSize  (I) the length of the output buffer
 */

int MJobRemovePBSDirective(

  char       *AName,
  const char *SAName,
  char       *SrcBuf,
  char       *OBuf,
  int         OBufSize)

  {
  char LastArg[MMAX_LINE];
  char tmpVal[MMAX_LINE];

  tmpVal[0] = '\0';
  LastArg[0] = '\0';

  if ((SrcBuf == NULL) || (SrcBuf[0] == '\0'))
    {
    return(FAILURE);
    }

  if ((SAName == NULL) || (SAName[0] == '\0'))
    {
    return(FAILURE);
    }

  MDB(7,fSCHED) MLog("INFO:     removing all instances of PBS attribute request '%s' from command buffer\n",
    SAName);

  /* if we were given a type (-l/-W x=) look for it */

  if ((AName != NULL) && (AName[0] != '\0'))
    {
    MJobPBSExtractArg(
      AName,
      SAName,
      SrcBuf,
      tmpVal,
      MMAX_LINE,
      TRUE);

    while (tmpVal[0] != '\0')
      {
      MUStrCpy(LastArg,tmpVal,MMAX_LINE);

      MJobPBSExtractArg(
        AName,
        SAName,
        SrcBuf,
        tmpVal,
        MMAX_LINE,
        TRUE);
      } /* END while (LastArg[0])... */
    } /* END if (AName != NULL)... */
  else
    {
    /* look for -l SAName */

    MJobPBSExtractArg(
      "-l",
      SAName,
      SrcBuf,
      tmpVal,
      MMAX_LINE,
      TRUE);

    while (tmpVal[0] != '\0')
      {
      MUStrCpy(LastArg,tmpVal,MMAX_LINE);

      MJobPBSExtractArg(
        "-l",
        SAName,
        SrcBuf,
        tmpVal,
        MMAX_LINE,
        TRUE);
      } /* END while (LastArg[0])... */

    if (OBuf != NULL)
      {
      MUStrCpy(OBuf,LastArg,OBufSize);
      }

    /* remove -W x=...;arg:value;... */
    do
      {
      MJobPBSExtractExtensionArg(SAName,SrcBuf,tmpVal,sizeof(tmpVal),TRUE);
      } while (tmpVal[0] != '\0');

    } /* END else (removing both -l and -W x=) */

  return(SUCCESS);
  } /* int MJobRemovePBSDirective() */



/**
 * Parses the PBS style taskstring and populates the job structure.
 *  
 * @see MJobGetSelectPBSTaskList() - peer
 * @see MPBSJobUpdate() - parent
 * @see MPBSJobSetAttr() - parent
 *
 * for neednodes, IsExecList==FALSE,IsNodes==FALSE,DoVerfiy==TRUE, action is ???
 * when IsExecList==FALSE sometimes TaskList is populated - FIXME ???
 *
 * NOTE:  does not set J->Request correctly for multi-req jobs
 * FIXME: the return code is not checked - result: when this routine fails 
 *        there are no constraints on the job
 *
 * @param J (I) [modified]
 * @param TaskString (I) (modified as side-affect) 
 * @param TaskList (O, Optional, minsize=MSched.JobMaxTaskCount) populated when IsExecList == TRUE
 * @param IsExecList (I)
 * @param IsNodes (I) string is 'nodes' specification
 * @param DoVerify (I) verify hostlist is valid
 * @param IsTaskCentric (I) format is <TaskCount>[:<TASKSPEC>]...[[+<TaskCount>][:<TASKSPEC>]...]...
 * @param RM (I) optional
 * @param EMsg (O) optional,minsize=MMAX_LINE
 */

int MJobGetPBSTaskList(

  mjob_t  *J,
  char    *TaskString,
  int     *TaskList,
  mbool_t  IsExecList,
  mbool_t  IsNodes,
  mbool_t  DoVerify,
  mbool_t  IsTaskCentric,
  mrm_t   *RM,
  char    *EMsg)

  {
  int     index;
  int     tindex;

  char   *ptr;
  char   *ptr2;

  char   *TokPtr;
  char   *TokPtr2=NULL;

  mnode_t *N;
  char    tmpHostName[MMAX_NAME];
  char    tmpName[MMAX_NAME];

  int     ppn;
  int     gpus;

  int     tmpTaskCount = 0;
  char   *tail;

  mreq_t *RQ;
  int     rqindex;

  int     TCCount;

  int     TC;

  int     OldTC;

  int     HLIndex;

  int tmpHostNameLength;

  mbool_t RequestIsProcCentric;
  mbool_t ReqIsLoaded;

  mbool_t IsFeature = FALSE;
  mbool_t NodeLocated = FALSE;

  int    *TLPtr;

  int    *tmpTaskList = NULL;

  /* FORMAT:  <HOSTNAME>[:{ppn|cpp}=<X>][<HOSTNAME>[:{ppn|cpp}=<X>]][#excl]... */
  /* or       <COUNT>[:{ppn|cpp}=<X>][#excl]                                   */

  const char *FName = "MJobGetPBSTaskList";

  MDB(5,fPBS) MLog("%s(%s,%s,%s,%s,%s,%s,%s,EMsg)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (TaskString != NULL) ? TaskString : "NULL",
    (TaskList != NULL) ? "TaskList" : "NULL",
    MBool[IsExecList],
    MBool[IsNodes],
    MBool[DoVerify],
    MBool[IsTaskCentric]);

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if ((J == NULL) || (TaskString == NULL))
    {
    if (EMsg != NULL)
      strcpy(EMsg,"invalid taskstring specified");

    return(FAILURE);
    }

  if ((bmisset(&J->IFlags,mjifPBSGPUSSpecified | mjifPBSMICSSpecified) && (IsExecList == TRUE)))
    {
    /* this code sucks reading in DRes.Procs>1 type requests */

    MPBSJobGetAcceleratorExecHList(J,TaskString,TaskList);

    return(SUCCESS);
    }

  tindex = 0;

  RequestIsProcCentric = MJobReqIsProcCentric(J,TaskString);

  /* NOTE:  ppn is 'processes per node' -> number of threads launched per node/virtual node allocated */
  /* NOTE:  cpp is 'cpus per processor' -> task's DRes.Procs */
  /* NOTE:  gpus is 'gpus per node'     -> number of gpus per node/virtual node allocated */
  /* NOTE:  mics is 'mics per node'     -> number of mics per node/virtual node allocated */

  /* NOTE:  only handles one 'ppn' distribution per job (same for gpus) */

  ppn = 1;
  gpus = 0;
  rqindex = 0;

  if (TaskList != NULL)
    {
    TLPtr = TaskList;
    }
  else if (MSched.IsClient == TRUE)
    {
    /* we're on the client so we can't read the moab.cfg */

    tmpTaskList = (int *)MUMalloc(sizeof(int) * MMAX_TASK);

    TLPtr = tmpTaskList;
    }
  else
    {
    tmpTaskList = (int *)MUMalloc(sizeof(int) * MSched.JobMaxTaskCount);

    /* initialize */
    memset(tmpTaskList,0,sizeof(tmpTaskList));

    TLPtr = tmpTaskList;
    }

  /* NOTE:  initial support enabled for multiple reqs */

  RQ = J->Req[0];

  OldTC = RQ->TaskCount;

  RQ->TaskCount = 0;

  if (RequestIsProcCentric == FALSE)
    {
    RQ->DRes.Procs = 1;
    }

  TCCount  = 0;
  HLIndex  = -1;

  if (IsNodes == TRUE)
    {
    bmset(&J->IFlags,mjifPBSNodesSpecified);

    /* should only be set for 'nodes' (not exec_hosts, or need_nodes) */

    /* process before TaskString parsed/fragmented */

    if (strstr(TaskString,"ppn="))
      {
      bmset(&J->IFlags,mjifPBSPPNSpecified);
      }

    if (strstr(TaskString,"cpp="))
      {
      bmset(&J->IFlags,mjifPBSCPPSpecified);
      }

    if (strstr(TaskString,"gpus="))
      {
      bmset(&J->IFlags,mjifPBSGPUSSpecified);

      RequestIsProcCentric = FALSE;
      }

    if (strstr(TaskString,"mics="))
      {
      bmset(&J->IFlags,mjifPBSMICSSpecified);

      RequestIsProcCentric = FALSE;
      }
    }    /* END if (IsNodes == TRUE) */

  if (IsExecList == FALSE)
    {
    if (MJOBISACTIVE(J) == FALSE)
      {
      /* clear existing features in case feature was externally modified within job - 
           if feature still set it will be restored later in this routine */

      /* NOTE:  existing features may be inherited from class cred */
      /* NOTE:  existing features may be specified via RMXString, ie '-l nodes=2,features=X' */

      if ((J->SubmitRM != NULL) && 
          (J->SubmitRM->Version >= 710)) /* ReqFBM can be set via Resources_List.<boolean> in PBSPro > 7.1 */
        {
        /* NOTE:  existing features may be inherited from parent class */

        MReqSetAttr(
          J,
          RQ,
          mrqaReqNodeFeature,
          (void **)NULL,
          mdfString,
          mSet);

        bmor(&RQ->ReqFBM,&RQ->SpecFBM);

        if ((J->Credential.C != NULL) && (bmisclear(&RQ->ReqFBM)))
          bmor(&RQ->ReqFBM,&J->Credential.C->DefFBM);

        if ((J->SubmitRM->JDef != NULL) && 
            (J->SubmitRM->JDef->Req[0] != NULL))
          {
          bmor(&RQ->ReqFBM,&J->SubmitRM->JDef->Req[0]->ReqFBM);
          }
        }
      }
    }    /* END if (IsExecList == FALSE) */

  ReqIsLoaded = FALSE;

  /* NOTE:  collect all host requests into single req */
  /* NOTE:  PPNSet is 'per req' variable */

  /* job is multi-req if
        TCCount > 1

     or

        TCCount == 1 and HLIndex > -1 */

  ptr = MUStrTok(TaskString,"+ \t",&TokPtr);

  while (ptr != NULL)
    {
    /* FORMAT:  <COUNT>[ :{[ppn=<X>] | [cpp=<X>] | [gpus=<X>] | [mics=<X>] | [<FEATURE>]}] */
    /*       or <HOSTNAME>[:ppn=<X>][:cpp=<X>][:gpus=<X>][:mics=<X>] */
    /* PBSPro 9  hosta/J1+hostb/J2*P+...
       where J1 and J2 are an index of the job on the named host 
       and P is the number  of  processors  allocated  from
       that host to this job.   P does not appear if it is 1. */

    ppn = MAX(1,RQ->TasksPerNode);

    MUStrCpy(tmpHostName,ptr,sizeof(tmpHostName));

    MUStrTok(tmpHostName,":",&TokPtr2);

    if ((J->DestinationRM != NULL) && (J->DestinationRM->Version >= 900))
      {
      char *tmpPtr;

      /* if '*' exists, then number is ncpus on host, else ncpu=1 */

      if ((tmpPtr = MUStrChr(tmpHostName,'*')) != NULL)
        {
        ppn = (int)strtol(++tmpPtr,NULL,10); 
        }
      }

    /* remove virtual host id */

    if ((tail = strchr(tmpHostName,'/')) != NULL)
      *tail = '\0';

    tmpTaskCount = (int)strtol(tmpHostName,&tail,10);

    /* on bgl/p systems you can submit job with a 'k', 
     * ex. -l nodes=2k == -l nodes=2048. Client code can't
     * determine if this is a bgl/p request or not so 'k'
     * check is always done. */

    tmpHostNameLength = strlen(tmpHostName);

    /* if k is at end of string and string is all digits multiply taskcount by 1024 */

    if ((tmpHostName[tmpHostNameLength - 1] == 'k') ||
        (tmpHostName[tmpHostNameLength - 1] == 'K'))
      {
      if ((int)strspn(tmpHostName,"0123456789") == (tmpHostNameLength - 1))
        {
        tmpHostName[tmpHostNameLength -1] = '\0';

        tmpTaskCount *= 1024;
        }
      }

    if (((tmpTaskCount != 0) || (tmpHostName[0] == '0')) && (*tail == '\0'))
      {
      /* TC request may be '0' */

      /* TC request detected */

      if (ReqIsLoaded == TRUE)
        {
        /* create new req if current req is populated with host or TC request */

        rqindex++;

        if (J->Req[rqindex] == NULL)
          {
          if (MReqCreate(J,NULL,NULL,FALSE) == FAILURE)
            {
            MDB(1,fPBS) MLog("ALERT:    task description for job %s requests too many job reqs\n",
              J->Name);

            if (EMsg != NULL)
              MUStrCpy(EMsg,"ERROR:   job requests too many reqs\n",MMAX_LINE);

            MUFree((char **)&tmpTaskList);

            return(FAILURE);
            }

          MRMReqPreLoad(J->Req[rqindex],&MRM[J->Req[0]->RMIndex]);
          }

        RQ = J->Req[rqindex];

        RQ->TaskCount = 0;

        ReqIsLoaded = TRUE;
        }
      else
        {
        /* populate primary req */

        ReqIsLoaded = TRUE;
        }

      TCCount++;
      }    /* END if (((tmpTaskCount != 0) || (tmpHostName[0] == '0')) && (*tail == '\0')) */

    /* check for ppn/gpus/mics/feature info */

    ptr2 = MUStrTok(NULL,"#:",&TokPtr2);

    while (ptr2 != NULL)
      {
      MDB(7,fSTRUCT) MLog("INFO:     processing resource token '%s' in %s\n",
        ptr2,
        FName);

      if (!strncmp(ptr2,"ppn=",strlen("ppn=")))
        {
        /* strtol converts digits up to the first non digit in base 10 */

        ppn = (int)strtol(ptr2 + strlen("ppn="),NULL,10); 

        /* if k is at end of string multiply ppn by 1024 */

        if (ptr2[strlen(ptr2) -1] == 'k')
          {
          ptr2[strlen(ptr2) -1] = '\0';

          ppn *= 1024;
          }

        if (ppn >= 1)
          {
          if (RequestIsProcCentric == TRUE)
            {
            if (RQ->Index != HLIndex)
              {
              /* do not set TasksPerNode for HostList Req */

              RQ->TasksPerNode = ppn;
              }
            }
          else
            {
            RQ->DRes.Procs = ppn;

            ppn = 1;
            }
          }
        }
      else if (!strncmp(ptr2,"gpus=",strlen("gpus=")))
        {
        int GPUIndex;

        gpus = (int)strtol(ptr2 + strlen("gpus="),NULL,10);

        GPUIndex = MUMAGetIndex(meGRes,MCONST_GPUS,mAdd);

        if ((GPUIndex == 0) || (GPUIndex >= MMAX_GRES))
          {
          MUFree((char **)&tmpTaskList);

          return(FAILURE);
          }        

        if (MSNLGetIndexCount(&RQ->DRes.GenericRes,GPUIndex) == 0)
          {
          /* Only add if gpus were not already set */

          MSNLSetCount(&RQ->DRes.GenericRes,GPUIndex,MSNLGetIndexCount(&RQ->DRes.GenericRes,GPUIndex) + gpus);
          }
        } /* END if (!strncmp(ptr2,"gpus=",strlen("gpus="))) */
      else if (!strncmp(ptr2,"mics=",strlen("mics=")))
        {
        int mics = (int)strtol(ptr2 + strlen("mics="),NULL,10);

        int MICIndex = MUMAGetIndex(meGRes,MCONST_MICS,mAdd);

        if ((MICIndex == 0) || (MICIndex >= MMAX_GRES))
          {
          MUFree((char **)tmpTaskList);

          return(FAILURE);
          }        

        if (MSNLGetIndexCount(&RQ->DRes.GenericRes,MICIndex) == 0)
          {
          /* Only add if mics were not already set */

          MSNLSetCount(&RQ->DRes.GenericRes,MICIndex,MSNLGetIndexCount(&RQ->DRes.GenericRes,MICIndex) + mics);
          }
        } /* END if (!strncmp(ptr2,"mics=",strlen("mics="))) */
      else if (!strncmp(ptr2,"cpp=",strlen("cpp=")))
        {
        /* load cpp, (ignore ncpus) */

        RQ->DRes.Procs = (int)strtol(ptr2 + strlen("cpp="),NULL,10);
        }
      else if (!strncmp(ptr2,"ncpus=",strlen("ncpus=")) ||
               !strncmp(ptr2,"mem=",strlen("mem=")) ||
               !strncmp(ptr2,"pmem=",strlen("pmem=")) ||
               !strncmp(ptr2,"pvmem=",strlen("pvmem=")) ||
               !strncmp(ptr2,"file=",strlen("file=")))
        {
        /* NO-OP */

        /* do not process ncpus,mem, etc as node feature */
        }
      else if ((!strncmp(ptr2,"reseterr",strlen("reseterr"))) ||
               (!strncmp(ptr2,"exclusive",strlen("exclusive"))) ||
               (!strncmp(ptr2,"default",strlen("default"))) ||
               (!strncmp(ptr2,"shared",strlen("shared"))) ||
               (!strncmp(ptr2,"exclusive_thread",strlen("exclusive_thread"))) ||
               (!strncmp(ptr2,"exclusive_process",strlen("exclusive_proces)s"))))
        {
        /* NO-OP */

        /* do not process gpu modes and reseterr directives, we pass these
         * directly to Torque. */
        }
      else if (!strncmp(ptr2,"excl",strlen("excl")))
        {
        /* #excl */

        /* job requests exclusive resources */

        RQ->NAccessPolicy = mnacSingleJob;
  
        MDB(7,fCONFIG) MLog("INFO:     job %s node access policy set to %s from pbs task string\n",
          J->Name,
          MNAccessPolicy[RQ->NAccessPolicy]);
        }
      else
        {
        /* feature requirement specified */

        if ((J->SubmitRM != NULL) &&
            (J->SubmitRM->Version >= 710) &&
            (strchr(ptr2,'=') != NULL)) /* ReqFBM can be set via Resources_List.<boolean> in PBSPro > 7.1 */
          {
          char *ptr3;
          char *TokPtr3;

          ptr3 = MUStrTok(ptr2,"=",&TokPtr3);

          if (MUGetIndexCI(TokPtr3,MBoolString,FALSE,-1) == -1)
            {
            MDB(1,fPBS) MLog("ALERT:    invalid node feature requested '%s=%s', ignoring\n",
              ptr3,
              TokPtr3);
            }
          else
            {
            MReqSetAttr(J,RQ,mrqaReqNodeFeature,(void **)ptr3,mdfString,mAdd);
            }
          }
        else
          {
          MReqSetAttr(
            J,
            RQ,
            mrqaReqNodeFeature,
            (void **)ptr2,
            mdfString,
#ifdef MYAHOO
            mSet);
#else
            mAdd);
#endif /* MYAHOO */
          }
        }

      ptr2 = MUStrTok(NULL,"#:",&TokPtr2);
      }  /* END while (ptr2 != NULL) */

    /* determine hostname/nodecount */

    if (((tmpTaskCount != 0) || (ptr[0] == '0')) && (*tail == '\0'))
      {
      /* TC request specified */

      /* Shouldn't multiply if tpn specified */

      /* FIXME: race condition as qsub jobs hit this section of code before TPN is processed,
       *        msub jobs hit this section of code AFTER TPN is process 
       */

      if (!bmisset(&J->IFlags,mjifExactTPN) && !bmisset(&J->IFlags,mjifMaxTPN))
        RQ->TaskCount += tmpTaskCount * MAX(1,RQ->TasksPerNode);
      else
        RQ->TaskCount += tmpTaskCount;
      }  /* END if (((tmpTaskCount != 0) || ...) */
    else
      {
      /* HL request specified */

      IsFeature = FALSE;
      NodeLocated = FALSE;

      if (DoVerify == TRUE)
        {
        if ((MUMAGetIndex(meNFeature,tmpHostName,mVerify) != 0) && 
            (IsExecList == FALSE))
          {
          IsFeature = TRUE;
          }

        if ((MSched.ManageTransparentVMs == TRUE) && (IsExecList == TRUE))
          {
          mvm_t *V;

          if (MUHTGet(&MSched.VMTable,tmpHostName,(void **)&V,NULL) == SUCCESS)
            {
            /* virtual machine allocated */

            N = V->N;

            V->ARes.Procs = 0;
           
            NodeLocated = TRUE;
            }
          }

        if ((NodeLocated == FALSE) && (RM != NULL) &&
            (MOMap(RM->OMap,mxoNode,tmpHostName,TRUE,FALSE,tmpName) == SUCCESS))
          {
          MUStrCpy(tmpHostName,tmpName,sizeof(tmpHostName));
          }

        if ((NodeLocated == FALSE) &&
            (MNodeFind(tmpHostName,&N) == FAILURE) &&
            (IsFeature == FALSE))
          {
          if (MSched.SideBySide == TRUE)
            {
            /* job was started by another scheduler on unknown nodes */

            bmset(&J->SpecFlags,mjfNoResources);
            bmset(&J->Flags,mjfNoResources);

            if (EMsg != NULL)
              MUStrCpy(EMsg,"job was started by another scheduler on unknown nodes\n",MMAX_LINE);

            MUFree((char **)&tmpTaskList);

            return(FAILURE);
            }

          if (TRUE) /* (MSched.VMGResMap == TRUE) */
            {
            mvm_t *V;

            if (MVMFind(tmpHostName,&V) == FAILURE)
              {
              MDB(1,fPBS) MLog("ALERT:    cannot locate host '%s' for job hostlist\n",
                tmpHostName);

              if (EMsg != NULL)
                MUStrCpy(EMsg,"cannot locate host for job hostlist\n",MMAX_LINE);

              MUFree((char **)&tmpTaskList);

              return(FAILURE);
              }

            /* use virtual node's physical node */

            N = V->N;

            if (J->ReqVMList == NULL)
              {
              J->ReqVMList = (mvm_t **)MUCalloc(1,sizeof(mvm_t *) * (MSched.JobMaxTaskCount));
              }

            /* NOTE:  only single req VM/job supported (FIXME) */

            J->ReqVMList[0] = V;
            J->ReqVMList[1] = NULL;
            }
          }  /* END if ((NodeLocated == FALSE) && ...) */
        }    /* END if (DoVerify == TRUE) */
      else
        {
        if (MNodeAdd(tmpHostName,&N) == FAILURE)
          {
          MDB(1,fPBS) MLog("ALERT:    cannot add host '%s' to job hostlist\n",
            tmpHostName);

          if (EMsg != NULL)
            MUStrCpy(EMsg,"cannot add host to job hostlist\n",MMAX_LINE);

          MUFree((char **)&tmpTaskList);

          return(FAILURE);
          }
        }

      if (IsFeature == FALSE)
        {
        mbool_t NoLoad = FALSE;

        /* load task list info */

        if (RequestIsProcCentric == FALSE)
          {
          for (index = 0;index < tindex;index++)
            {
            if (TLPtr[index] == N->Index)
              {
              NoLoad = TRUE;

              break;
              }
            }

          for (index = 0;(index < ppn) && (NoLoad == FALSE);index++)
            {
            /* FIXME: for RequestIsProcCentric == FALSE same hosts belong in the same task */

            TLPtr[tindex] = N->Index;

            tindex++;

            if (IsExecList == TRUE)
              {
              /* exec lists do not contain ppn info, but are listed with one entry per vnode */

              break;
              }
            }
          }    /* END if (RequestIsProcCentric == FALSE) */
        else if (N != NULL)
          {
          for (index = 0;index < ppn;index++)
            {
            /* FIXME: for RequestIsProcCentric == FALSE same hosts belong in the same task */

            TLPtr[tindex] = N->Index;

            tindex++;

            /* PPBPro 9 condenses exec_hosts, so need to continue */

            if ((J->SubmitRM != NULL) && 
                /*(J->SRM->Version < 900) && */
                /* This is the correct solution but it breaks the reported
                 * procs in showq because it builds up a tasklist with a node
                 * index for each proc. Without it, Allocated Nodes in
                 * checkjob only shows 1 proc in use on the node. In
                 * MRMJobPostUpdate, the tasklist is walked and each task is
                 * counted and then assigned to RQ->TaskCount. ex. with a
                 * 1node,3cpus job, RQ->TaskCount is 1 and there are 3 node
                 * indecies in the TaskList. Then in MRMJobPostUpdate,
                 * RQ->TaskCount gets set to 3 because it calulated 3 node indecies. */
                (IsExecList == TRUE))
              {
              /* exec lists do not contain ppn info, but are listed with one entry per vnode */

              break;
              }
            }
          }    /* END else if (N != NULL) */

        if (NoLoad == FALSE)
          {
          if ((HLIndex == -1) && (ReqIsLoaded == TRUE))
            {
            /* create new HL req */
 
            rqindex++;
 
            if (J->Req[rqindex] == NULL)
              {
              MReqCreate(J,NULL,NULL,FALSE);
 
              MRMReqPreLoad(J->Req[rqindex],&MRM[J->Req[0]->RMIndex]);
              }
 
            RQ = J->Req[rqindex];
 
            RQ->TaskCount = 0;
            }  /* END if ((HLIndex == -1) && (ReqIsLoaded == TRUE)) */
 
          /* exec_host list one entry per proc. If 24 procs and 8 ppn and +=
           * ppn, then taskcount will be 192 so increment by 1 */

          if (IsExecList == TRUE)
            RQ->TaskCount += 1;
          else
            RQ->TaskCount += ppn;
 
          ReqIsLoaded = TRUE;
 
          HLIndex = rqindex;
          }
        else
          {
          RQ->DRes.Procs += 1;
          }
        }  /* END if (IsFeature == FALSE) */
      else
        {
        MReqSetAttr(
          J,
          RQ,
          mrqaReqNodeFeature,
          (void **)tmpHostName,
          mdfString,
          mSet);
        }
      }    /* END else ((tmpTaskCount != 0) && (*tail == '\0')) */

    ptr = MUStrTok(NULL,"+ \t",&TokPtr);
    }    /* END while (ptr != NULL) */

  TLPtr[tindex] = -1;

  if ((tindex > 0) && (HLIndex > -1) && (IsExecList == FALSE))
    {
    /* host list specified */

    if (MJobNLFromTL(J,&J->ReqHList,TLPtr,NULL) == FAILURE)
      {
      MDB(7,fPBS) MLog("ALERT:    cannot allocate hostlist for job %s\n",
        J->Name);

      if (EMsg != NULL)
        MUStrCpy(EMsg,"could not allocate hostlist for job\n",MMAX_LINE);

      MUFree((char **)&tmpTaskList);

      return(FAILURE);
      }

    bmset(&J->IFlags,mjifHostList);

    MDB(7,fPBS) MLog("INFO:     required hostlist set for job %s\n",
      J->Name);
    }

  MDB(7,fPBS) MLog("INFO:     %d host task(s) located for job\n",
    tindex);

  if (rqindex > 0)
    {
    MDB(7,fPBS) MLog("INFO:     %d task(s) located for job req %d\n",
      J->Req[rqindex]->TaskCount,
      rqindex);
    }

  if (IsExecList == FALSE)
    {
    static mjob_t  *OldJ = NULL;
    static int      OldJHLIndex = -1;

    if (HLIndex == -1)
      {
      /* NOTE:  must handle 'neednodes' specification of hostlist
       *  (ie, neednodes specifies hostlist, nodes does not */

      if ((J == OldJ) && (OldJHLIndex == -1))
        {
        bmunset(&J->IFlags,mjifHostList);

        MNLFree(&J->ReqHList);
        }
      }  /* END if (HLIndex == -1) */

    if (J == OldJ)
      {
      if (HLIndex > -1)
        OldJHLIndex = HLIndex;
      }
    else
      {
      OldJ = J;
      OldJHLIndex = HLIndex;
      }

    if (HLIndex > -1)
      J->HLIndex = HLIndex;
    }    /* END if (IsExecList == FALSE) */

  if ((RQ->TaskCount == 0) && (OldTC != 0))
    RQ->TaskCount = OldTC;

  /* sync Req TaskCounts (NOTE: Req nodecounts not modified) */

  TC = 0;

  for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
    {
    RQ = J->Req[rqindex];

    if (RQ == NULL)
      break;

    RQ->TaskRequestList[0] = RQ->TaskCount;

    /* Why do we overwrite what is at [1]? 
     * For a trl of 4:8:12 and 12 is being used, 
     * the trl will get set to 12:8:12 */
    
    if (!bmisset(&J->IFlags,mjifTRLRequest))
      RQ->TaskRequestList[1] = RQ->TaskCount; 

    if (RQ->DRes.Procs > 0) /* exclude generic nodes */
      TC += RQ->TaskCount;
    }    /* END for (rqindex) */

  J->Request.TC = MAX(J->Request.TC,TC);

  MUFree((char **)&tmpTaskList);

  return(SUCCESS);
  }    /* END MJobGetPBSTaskList() */




/**
 * Processes the pbs exec_vnode line.
 *
 * PBSPro 9.x: now uses vnodes. In the case that there are multiple vnodes per host, then we will want to 
 * to read in this attribute because it will tell us which vnodes the job is running on. exec_host will 
 * only says which hosts the job is running on, not which vnodes under a host.
 *
 * <p>(man qstat) The exec_vnode string has the format:(vnodeA:ncpus=N1:mem=M1)+(vnodeB:ncpus=N2:mem=M2)+...
 * where  N1  and  N2 are the number of CPUs allocated to that job on that vnode, and M1 and M2 are the amount 
 * f memory allocated to that job on that vnode. 
 *
 * another ex: (vnodeA:ncpus=N:mem=X)+(nodeB:ncpus=P:mem=Y+nodeC:mem=Z) 
 *
 * @param J (I) [modified]
 * @param TaskString (I) [modified as side-affect]
 * @param TaskList (O) [optional,minsize=MSched.JobMaxTaskCount]
 * @param DoVerify (I) verify hostlist is valid
 */

int MJobGetPBSVTaskList(

  mjob_t  *J,              /* I */
  char    *TaskString,     /* I (modified as side-affect) */
  int     *TaskList,       /* O (optional,minsize=MSched.JobMaxTaskCount) */
  mbool_t  DoVerify)       /* I verify hostlist is valid */

  {
  const char *FName = "MJobGetPBSVTaskList";

  MDB(5,fPBS) MLog("%s(%s,%s,%s,%s)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (TaskString != NULL) ? TaskString : "NULL",
    (TaskList != NULL) ? "TaskList" : "NULL",
    MBool[DoVerify]);

  if (NULL == TaskString)
    return(FAILURE);

  mstring_t VStr(TaskString);
  size_t pos;

  /* remove characters '(' and ')' from the string, check for case of not found  */
  pos = VStr.find('(');
  if (pos != mstring_t::npos)
    VStr.erase(pos,1);

  pos = VStr.find(')');
  if (pos != mstring_t::npos)
    VStr.erase(pos,1);

  MJobGetSelectPBSTaskList(J,VStr.c_str(),TaskList,TRUE,TRUE,NULL);

  return(SUCCESS);
  } /* END int MJobGetPBSVTaskList() */




/**
 * Process 'select' resource description.
 *
 * NOTE:  This routine will populate J with only one Req.
 *
 * @see MJobGetPBSTaskList() - peer
 *
 * @param J (I) [modified]
 * @param TaskString (I) [modified as side-affect]
 * @param TaskList (O) [optional,minsize=MSched.JobMaxTaskCount]
 * @param DoVerify (I) verify hostlist is valid
 * @param IsVNodeList Specifies wether the tasklist is the exec_vnode list or not. This function has 
                      alls the functionality to read in the exec_vnode list. The only difference is that 
                      the first arg in the TaskString is the name of the host, not the number of chunks.
 * @param EMsg (O) [optional,minsize=MMAX_LINE)
 */

int MJobGetSelectPBSTaskList(
  
  mjob_t  *J,              /* I */
  const char  *TaskString,     /* I (modified as side-affect) */
  int     *TaskList,       /* O (optional,minsize=MSched.JobMaxTaskCount) */
  mbool_t  DoVerify,       /* I verify hostlist is valid */
  mbool_t  IsVNodeList,    /* I If the tasklist is the exec_vnode list or not */
  char    *EMsg)           /* O (optional,minsize=MMAX_LINE) */

  {
  char *ptr; 
  char *ptr2;
  
  char *TokPtr = NULL;
  char *TokPtr2 = NULL;

  int rqindex;
  int index;
  int ppn;
  int tindex;
  int HLIndex;

  mnode_t *N;

  int    *TLPtr;

  int    *tmpTaskList = NULL;

  mreq_t *RQ = NULL;

  mbool_t HostSpecified;

  mrm_t *R = NULL;

  char *tmpTaskStr = NULL;

  /* NOTE:  select must be translated different than "neednodes", if select is specified,
            memory is assigned as a per task resource and arch as a per req constraint */

  /*
  > qsub -l select=4 (it requests 4 default chunks)
  >
  > or
  >
  > qsub -l select=2:ncpus=1:mem=10GB+3:ncpus=2:mem=8GB:arch=linux
  > (requests 2 chunks of 1 cpu,10gb of mem and 3 chunks of
  > 2 cpus, 8gb of mem, arch=linux).
  */

  const char *FName = "MJobGetSelectPBSTaskList";

  MDB(5,fPBS) MLog("%s(%s,%s,%s,%s)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (TaskString != NULL) ? TaskString : "NULL",
    (TaskList != NULL) ? "TaskList" : "NULL",
    MBool[DoVerify]);

  if (NULL == J)
    return(FAILURE);

  rqindex = 0;
  tindex  = 0;
  HLIndex = -1;  /* set a NOT-FOUND marker of -1 */

  tmpTaskList = (int *)MUCalloc(1,sizeof(int) * MSched.M[mxoNode]);

  if (TaskList != NULL)
    {
    TLPtr = TaskList;
    }
  else
    {
    TLPtr = tmpTaskList;
    }

  MUStrDup(&tmpTaskStr, TaskString);

  ptr = MUStrTok(tmpTaskStr,"+ \n\t",&TokPtr);

  RQ = NULL;

  R = &MRM[J->Req[0]->RMIndex];

  while (ptr != NULL)
    {
    N = NULL;

    if (J->Req[rqindex] == NULL)
      {
      MReqCreate(J,NULL,NULL,TRUE);

      MRMReqPreLoad(J->Req[rqindex],R);
      }

    RQ = J->Req[rqindex];

    if (RQ == NULL)
      {
      MUFree(&tmpTaskStr);
      MUFree((char **)&tmpTaskList);

      return(FAILURE);
      }

    ptr2 = MUStrTok(ptr,":",&TokPtr2);

    if (IsVNodeList == TRUE)
      {
      if (MNodeFind(ptr2,&N) == FAILURE)
        {
        MDB(1,fPBS) MLog("ALERT:    cannot locate host '%s' for job %s exec_vnode list.\n",
          ptr,
          J->Name);

        MUFree((char **)&tmpTaskList);
        MUFree(&tmpTaskStr);

        return(FAILURE);
        }
      } /* END if (IsVNodeList == TRUE) */
    else 
      {
      int tmpI;

      /* PBSPro: It's not always nescessary to have number of chunks in the front of the select  
         -l select=[N:]chunk[+[N:]chunk ...] */
    
      RQ->TaskCount = 0;

      if (MUStrChr(ptr2,'=') != NULL) 
        {
        if ((TokPtr2 != NULL) && (*TokPtr2 != '\0'))
          *(--TokPtr2) = ':'; /* Replace ':' */

        TokPtr2 = ptr2;

        tmpI = 1; /* If # number of chunks not specified, defaults to 1 */
        }
      else
        {
        tmpI = (int)strtol(ptr2,NULL,10);
        }

      if (RQ->TaskCount != tmpI)
        {
        MReqSetAttr(
          J,
          RQ,
          mrqaTCReqMin,
          (void **)&tmpI,
          mdfInt,
          mSet);
        }
      }

    if (MJobPBSProcessResources(
         J,
         RQ,
         TokPtr2,
         &N,
         &ppn,
         &HostSpecified) == FAILURE)
      {
      MDB(4,fPBS) MLog("WARNING:  Job %s failed while processing PBS resources\n",
        J->Name);

      MUFree((char **)&tmpTaskList);
      MUFree(&tmpTaskStr);

      return(FAILURE);
      }

    if (N != NULL)
      {
      /* defer adding tasks to TL because ncpus could have been read in later */ 

      for (index = 0;index < ppn;index++)
        {
        TLPtr[tindex] = N->Index;

        tindex++;
        }
      }

    RQ->TaskRequestList[0] = RQ->TaskCount;
    RQ->TaskRequestList[1] = RQ->TaskCount;

    /* We found an entry at 'rqindex', note its value now */
    HLIndex = rqindex;

    ptr = MUStrTok(NULL,"+ \n\t",&TokPtr);

    rqindex++;
    }  /* END while (ptr != NULL) */

  MUFree((char **)&tmpTaskList);
  MUFree(&tmpTaskStr);

  TLPtr[tindex] = -1;

  if ((HLIndex >= 0) && (tindex > 0) && (IsVNodeList == FALSE)) /* don't set reqhlist for exec_vnode */
    {
    /* host list specified */

    if (MJobNLFromTL(J,&J->ReqHList,TLPtr,NULL) == FAILURE)
      {
      MDB(7,fPBS) MLog("ALERT:    cannot allocate hostlist for job %s\n",
        J->Name);

      return(FAILURE);
      }

    bmset(&J->IFlags,mjifHostList);

    MDB(7,fPBS) MLog("INFO:     required hostlist set for job %s\n",
      J->Name);
    }

  return(SUCCESS);
  }  /* END MJobGetSelectPBSTaskList() */



/**
 * Apply resource-manager specific characteristics to massage job attributes. 
 *
 * NOTE:  only handles single req jobs.
 *
 * @see MPBSJobSetAttr() - parent
 * @see MPBSJobLoad() - parent
 *
 * @param J [modified]
 * @param TA
 * @param R
 */

int MPBSJobAdjustResources(

  mjob_t   *J,  /* I/O (modified) */
  mtrma_t  *TA, /* I */
  mrm_t    *R)  /* I */

  {
  mreq_t *RQ;
  int     rqindex;

  mbool_t RequestIsProcCentric;

  if ((J == NULL) || (J->Req[0] == NULL) || (R == NULL))
    {
    return(FAILURE);
    }

  if (J->RMXString != NULL)
    {
    mbitmap_t RMXBM;

    bmset(&RMXBM,mxaNMatchPolicy);
    bmset(&RMXBM,mxaPMask);

    MJobProcessExtensionString(J,J->RMXString,mxaNONE,&RMXBM,NULL);
    }  /* END if (J->RMXString != NULL) */

  MJobGetNodeMatchPolicy(J,&J->NodeMatchBM);

  RQ = J->Req[0];

  RequestIsProcCentric = MJobReqIsProcCentric(J,NULL);

  if ((J->Credential.G == NULL) || !strcmp(J->Credential.G->Name,DEFAULT))
    {
    if (J->Credential.U->F.GDef != NULL)
      J->Credential.G = (mgcred_t *)J->Credential.U->F.GDef;
    }

  /* handle ncpus/nodect adjustment */

  /* NOTE:  PBSPro 5.2.2+ maps 'ncpus' directly to 'ppn' */

  if (TA != NULL)
    {
    if (RequestIsProcCentric == TRUE)
      {
      if (R->PBS.PBS5IsEnabled == TRUE)
        {
        /* only executed on job load */

        if ((J->SubmitRM != NULL) && (J->SubmitRM->Version > 520))
          {
          if (TA->NCPUs > 1)
            {
            if (bmisset(&J->IFlags,mjifPBSPPNSpecified) ||
                bmisset(&J->IFlags,mjifPBSCPPSpecified))
              {
              /* ignore NCPU's if ppn or cpp is specified */

              TA->NCPUs = 1;
              }

            if (!bmisset(&J->IFlags,mjifPBSCPPSpecified) &&
                 bmisset(&J->IFlags,mjifPBSPPNSpecified))
              {
              RQ->DRes.Procs = 1;
              }
            } 
          }
        else if (TA->NodesRequested > 1)
          {
          TA->NCPUs      = 1;
          RQ->DRes.Procs = 1; 

          if (RQ->NodeCount == 1)
           RQ->NodeCount = 0;
          } 
        else if (RQ->TaskCount > 1)
          {
          TA->NCPUs = 1;
          RQ->DRes.Procs = 1;
          }
        }    /* END if (R->U.PBS.PBS5IsEnabled == TRUE) */

      if (RQ->TasksPerNode > RQ->TaskCount)
        RQ->TasksPerNode = 0;

      if ((TA->NCPUs > 1) &&
         ((TA->NodesRequested > 1) || (RQ->TasksPerNode > 1)))
        {
        /* multi-node 'ncpu' specification detected */

        RQ->DRes.Procs   = 1;

        RQ->TasksPerNode = TA->NCPUs;

        RQ->TaskCount    = TA->NCPUs;
        J->Request.TC    = TA->NCPUs;
        }
      }   /* END if (RequestIsProcCentric == TRUE) */

    if (TA->MasterHost[0] != '\0')
      {
      /* If there is only one host, put all tasks on the host.  If more than one,
          upping the task count will mess up the node list distribution for the job */

      if ((RQ->TaskCount > 0) && 
          (MNLGetNodeAtIndex(&J->ReqHList,0,NULL) == SUCCESS) && 
          (MNLGetNodeAtIndex(&J->ReqHList,1,NULL) == FAILURE))
        MNLSetTCAtIndex(&J->ReqHList,0,RQ->TaskCount);
      }
    }     /* END if (TA != NULL) */

  /* adjust req */

  if (RQ->TaskCount == 0)
    {
    /* assume single req */

    RQ->TaskCount = J->Request.TC;
    RQ->NodeCount = J->Request.NC;
    }
 
  /* handle matching policies */

  if (bmisset(&J->NodeMatchBM,mnmpExactTaskMatch))
    {
    /* tasks = nodes, DRes.procs = TasksPerNode */

    /* NYI */
    }
 
  if (bmisset(&J->NodeMatchBM,mnmpExactNodeMatch))
    {
    J->Request.NC = 0;
    }
 
  J->Request.NC = 0;
  J->Request.TC = 0;
 
  J->Alloc.NC = 0;
  J->Alloc.TC = 0;

  for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
    {
    RQ = J->Req[rqindex];

    if ((!MNLIsEmpty(&J->ReqHList)) &&
        (J->HLIndex != -1) &&
        (J->HLIndex == RQ->Index))
      {
      /* tasks per node not utilized with hostlists */

      /* NOTE:  must handle issues with mixed hostlist/task spec jobs */

      RQ->TasksPerNode = 0;
      }
    else
      {
      if (bmisset(&J->NodeMatchBM,mnmpExactProcMatch))
        {
        if (RQ->TasksPerNode >= 1)
          {
          RQ->ReqNR[mrProc]  = RQ->TasksPerNode;
          RQ->ReqNRC[mrProc] = mcmpEQ;
          }
        }
      }

    RQ->TaskCount = MAX(1,RQ->TaskCount);

    /* NOTE:  only apply job defaults to compute reqs */

    if ((TA != NULL) && (RQ->DRes.Procs != 0))
      {
      if (TA->JobMemLimit > 0)
        RQ->DRes.Mem = MAX(RQ->DRes.Mem,TA->JobMemLimit / RQ->TaskCount);

      if (TA->JobSwapLimit > 0)
        RQ->DRes.Swap = MAX(RQ->DRes.Swap,TA->JobSwapLimit / RQ->TaskCount);

      if (TA->JobDiskLimit > 0)
        {
        RQ->DRes.Disk = (MSched.FileRequestIsJobCentric == FALSE) ? 
          TA->JobDiskLimit : 
          MAX(RQ->DRes.Disk,TA->JobDiskLimit / RQ->TaskCount);

        /* NOTE: Disk  requests have moved to DRes
                 ReqNR is available resources (not dedicated)

        RQ->ReqNR[mrDisk] = RQ->DRes.Disk;
        RQ->ReqNRC[mrDisk] = MDEF_RESCMP;  */
        }
      }  /* END if (TA != NULL) */
 
    if (rqindex > 0)
      {
      if ((RQ->DRes.Procs != 0) && (R->Version < 710))
        {
        /* NOTE:  currently only support one task type for proc based reqs */
        /* NOTE:  does not apply to pbspro jobs with this syntax
                  select=2:ncpus=1:mem=1GB+3:ncpus=2:mem=2GB:arch=linux */

        MCResCopy(&RQ->DRes,&J->Req[0]->DRes);
        MCResCopy(RQ->URes,J->Req[0]->URes);

        RQ->Arch = J->Req[0]->Arch;

        memcpy(RQ->ReqNR,J->Req[0]->ReqNR,sizeof(RQ->ReqNR));
        memcpy(RQ->ReqNRC,J->Req[0]->ReqNRC,sizeof(RQ->ReqNRC));
        }
 
      RQ->RMWTime = J->Req[0]->RMWTime;
      }  /* if (rqindex > 0) */

    if (((MNLIsEmpty(&J->ReqHList)) || (RQ->Index != J->HLIndex)) && 
         (bmisset(&J->NodeMatchBM,mnmpExactNodeMatch)))
      {
      RQ->NodeCount = RQ->TaskCount / MAX(1,RQ->TasksPerNode);
      }

    J->Request.NC += RQ->NodeCount;
    J->Request.TC += RQ->TaskCount;
 
    if (MJOBISACTIVE(J) == TRUE)
      {
      J->Alloc.TC += RQ->TaskCount;
      J->Alloc.NC += RQ->NodeCount;
      }

    /* FIXME:  in the future "software" req should point to license RM */

    RQ->RMIndex    = R->Index;
 
    RQ->TaskRequestList[0] = RQ->TaskCount;

    if (RQ->TaskRequestList[1] == 0)
      {
      RQ->TaskRequestList[1] = RQ->TaskCount;
      RQ->TaskRequestList[2] = 0;
      }

    /* adjust resource requirement info */
    /* NOTE:  PBS specifies resource requirement info on a 'per node' basis */

    /* NOTE:  by default, Moab's 'tasks per node' is specified as 'minimum tasks 
              per node', not absolute fixed TPN.  Fixed TPN is enabled if
              JobNodeMatchPolicy is set to ExactNode.
    */

    if ((RQ->TaskCount > 1) &&
        (RQ->TasksPerNode > 1) &&
        (RQ->NodeCount <= 1))
      {
      if (bmisset(&J->NodeMatchBM,mnmpExactNodeMatch))
        RQ->NodeCount = MAX(1,RQ->TaskCount / RQ->TasksPerNode);
      }

    if ((RQ->NodeCount > 1) && (RQ->DRes.Procs > 1) && (R->SubmitPolicy != mrmspNodeCentric) &&
        (!bmisset(&J->IFlags,mjifPBSGPUSSpecified)))

      {
      /* this only applies to ProcCentric Jobs */

      RQ->DRes.Procs = 1;
      }
 
    /* NOTE: this setting overrides per-node settings  */

/*
    if (RQ->NAccessPolicy == mnacNONE)
      RQ->NAccessPolicy = MSched.DefaultNAccessPolicy;
*/
 
    if (((RQ->URes != NULL) &&
         (RQ->LURes != NULL)) &&
        ((J->State == mjsStarting) ||
         (J->State == mjsRunning) ||
         (J->State == mjsCompleted) ||
         (J->State == mjsRemoved)))
      {
      /* obtain 'per task' statistics */

      if (RQ->TaskCount > 0)
        {
        if ((TA != NULL) && (TA->UtlJobCPUTime > 0) && (RQ->RMWTime > 0))
          {
          RQ->LURes->Procs = (int)(TA->UtlJobCPUTime * 100.0 / RQ->TaskCount);
          }
        else
          {
          RQ->LURes->Procs = 100;
          }

        if (RQ->URes->Mem > 0)
          RQ->URes->Mem /= RQ->TaskCount;
        }
 
      /* PBS does not update 'cpupercent' (RQ->URes.Procs) quickly on some systems */
 
      if ((RQ->URes->Procs < 5) && 
          (RQ->LURes->Procs > 0.1) && 
          (RQ->RMWTime > 0))
        {
        RQ->URes->Procs = MAX(RQ->URes->Procs,RQ->LURes->Procs / (long)RQ->RMWTime);
        }
      }    /* END if ((J->State == mjsStarting) || ...) */

    if (MSched.TrackSuspendedJobUsage == TRUE)
      {
      /* to track suspended jobs, map swap requirements to memory requirements */

      RQ->DRes.Swap = MAX(RQ->DRes.Swap,RQ->DRes.Mem);
      }
    }  /* END for (rqindex) */

  /* can this be moved to the beginning of the routine? */

  if ((J->RMXString != NULL) &&
      (MJobProcessExtensionString(J,J->RMXString,mxaNONE,NULL,NULL) == FAILURE))
    {
    char tmpLine[MMAX_LINE];

    MDB(1,fPBS) MLog("ALERT:    cannot process extension line '%s' for job %s (cancelling job)\n",
      (J->RMXString != NULL) ? J->RMXString : "NULL",
      J->Name);

    sprintf(tmpLine,"%s:  cannot process extension line\n",
      MSched.MsgErrHeader);

    MJobCancel(J,tmpLine,FALSE,NULL,NULL);

    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MPBSJobAdjustResources() */



/* NOTE:  see MURSpecToL() for general conversion (report in MB) */

/**
 *
 *
 * @param String (I)
 */

long MPBSGetResKVal(

  char *String)  /* I */

  {
  long  val; 
  char *tail;

  if (String == NULL)
    {
    return(0);
    }

  val = strtol(String,&tail,10);

  if (*tail == ':') 
    {
    /* time resource -> convert to seconds */

    /* FORMAT [[hours:]minutes:]seconds */
    /* currently supports { seconds | hours:minutes[:seconds] } */

    /* currently drop milliseconds */

    char *tail2;

    val *= 3600;                               /* hours   */

    val += (strtol(tail + 1,&tail2,10) * 60);  /* minutes */

    if (*tail2 == ':')
      val += strtol(tail2 + 1,&tail,10);       /* seconds */

    return(val);
    }

  if (val == 0)
    {
    return(val);
    }
 
  if (*tail != '\0' && *(tail + 1) == 'w')
    {
    /* adjust for word */

    val <<= 3;
    }

  /* NOTE:  round up to nearest MB */

  if (*tail == 'k')
    {
    return(MAX(1024,val));
    }
  else if (*tail == 'm')
    {
    return(MAX(1024,val << 10));
    }
  else if (*tail == 'g')
    {
    return(MAX(1024,val << 20));
    }
  else if (*tail == 't')
    {
    return(MAX(1024,val << 30));
    }
  else if (*tail == 'b')
    {
    return(MAX(1024,val >> 10));
    }
  else if (*tail == 'w')
    {
    return(MAX(1024,val >> 7));
    }

  return(MAX(1024,val));
  }  /* END MPBSGetResKVal() */



/**
 * Parse the cray specific mppnodes attribute.
 *
 * mppnodes is the hostlist that the job can run on. The hostlist is not a strict
 * hostlist - meaning that a job can run as long as at least one node in the Required 
 * Hostlist is found in the feasible nodes.
 *
 * Expects Nodes to be only digits - since that is what Cray nodes are. ex. 1,2,5-8.
 *
 * @param J (I)
 * @param Value (I)
 */

int MPBSParseMPPNodesJobAttr(
   
  mjob_t     *J,
  const char *Value)

  {
  MASSERT(J != NULL,"null job when parsing mppnodes.");
  MASSERT(Value != NULL,"null value string when parsing mppnodes.");

  int *NNumberList = (int *)MUCalloc(1,sizeof(int) * MSched.M[mxoNode]);
  MASSERT(NNumberList != NULL, "failed to allocate memory when parsing mppnodes.");

  /* parse and expand string of nodes to array on node id integers. */
  int NCount = 0;
  if (MUParseRangeString(
              Value,
              NULL,
              NNumberList,        /* O */
              MSched.M[mxoNode],
              &NCount,            /* O */
              NULL) == FAILURE)
      {
      MDB(1,fPBS) MLog("ERROR:    failed to parse mppnodes value, '%s', for job '%s'\n",Value,J->Name);
      MUFree((char **)&NNumberList);
      return(FAILURE);
      }

  /* validate that all nodes exist */
  vector<mnode_t *> nodePtrs;
  for (int nindex = 0;nindex < NCount;nindex++)
    {
    mnode_t *tmpNode = NULL;
    string nodeStr = static_cast<ostringstream*>(&(ostringstream() << NNumberList[nindex]))->str();
    if (MNodeFind(nodeStr.c_str(),&tmpNode) == FAILURE)
      {
      MDB(1,fPBS) MLog("ERROR:    failed to find node '%s' in mppnodes '%s' for job '%s'\n",
          nodeStr.c_str(),
          Value,
          J->Name);
      MUFree((char **)&NNumberList);
      return(FAILURE);
      }

    nodePtrs.push_back(tmpNode);
    }

  MUFree((char **)&NNumberList);

  /* nodes exist. we committed to make changes to job's hostlist. */
  int nodePtrsSize = (int)nodePtrs.size();
  MNLClear(&J->ReqHList); /* clear even if no nodes were found -- allows clearing of the hostlist. */
  for (int hindex = 0;hindex < nodePtrsSize;hindex++)
    MNLAddNode(&J->ReqHList,nodePtrs[hindex],1,FALSE,NULL);
      
  J->ReqHLMode = mhlmSubset;
  bmset(&J->IFlags,mjifHostList);

  return(SUCCESS);
  }


/**
 * Interpret PBS mpp* parameters and update the Job accordingly. 
 * 
 * @param J             (I) [modified]
 * @param Resource      (I)
 * @param Value         (I) 
 * @param Request       (I)
 */
int MPBSSetMPPJobAttr(

  mjob_t     *J,
  const char *Resource,
  char       *Value,
  mreq_t     *Request)

  {
  using std::string;
  string attrValue;

  if (!strcasecmp(Resource,"mppmem"))
    {
    char temps[MMAX_LINE];
    snprintf(temps, sizeof(temps), "%ld", MPBSGetResKVal(Value) >> 10);
    attrValue = string(MWikiJobAttr[mwjaDMem]) + "=" + temps;
    }
  else if (!strcasecmp(Resource,"mpphost"))
    {
    attrValue = string(MWikiJobAttr[mwjaPartitionList]) + "=" + Value;
    }
  else if (!strcasecmp(Resource,"mpparch"))
    {
    attrValue = string(MWikiJobAttr[mwjaRArch]) + "=" + Value;
    }
  else if (!strcasecmp(Resource,"mppwidth"))
    {
    attrValue = string(MWikiJobAttr[mwjaTasks]) + "=" + Value;
    }
  else if (!strcasecmp(Resource,"mppnppn"))
    {
    attrValue = string(MWikiJobAttr[mwjaComment]) + "=tpn:" + Value;
    }
  else if (!strcasecmp(Resource,"mppdepth"))
    {
    attrValue = string(MWikiJobAttr[mwjaDProcs]) + "=" + Value;
    }
  else if (!strcasecmp(Resource,"mppnodes"))
    {
    return(MPBSParseMPPNodesJobAttr(J,Value));
    }
  else if (!strcasecmp(Resource,"mpplabels"))
    {
    attrValue = string(MWikiJobAttr[mwjaRFeatures]) + "=" + Value;
    }
  else
    {
    return FAILURE;
    }

  return MWikiJobUpdateAttr((char *)attrValue.c_str(), J, Request, NULL, NULL);

  }


/**
 * Parses the PBS exec_host.
 *  
 * @see MJobGetSelectPBSTaskList() - peer
 * @see MPBSJobUpdate() - parent
 * @see MPBSJobSetAttr() - parent
 *
 * NOTE:  does not set J->Request correctly for multi-req jobs
 * FIXME: the return code is not checked - result: when this routine fails 
 *        there are no constraints on the job
 *
 * @param J (I) [modified]
 * @param RM (I) (optional)
 * @param TaskString (I)
 * @param TaskList (I) [minsize=MSched.JobMaxTaskCount]
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 */

int MJobParsePBSExecHost(

  mjob_t  *J,
  mrm_t   *RM,
  char    *TaskString,
  int     *TaskList,
  char    *EMsg)

  {
  int     index;
  int     tindex;

  char   *ptr;

  char   *TokPtr;
  char   *TokPtr2;

  mnode_t *N;
  char    tmpHostName[MMAX_NAME];
  char    tmpName[MMAX_NAME];

  mreq_t *RQ;
  int     rqindex;

  int     TC;
  int     OldTC;

  int     HLIndex;

  mbool_t RequestIsProcCentric;
  mbool_t ReqIsLoaded;

  mbool_t NodeLocated = FALSE;

  /* FORMAT:  <HOSTNAME>[:{ppn|cpp}=<X>][<HOSTNAME>[:{ppn|cpp}=<X>]][#excl]... */
  /* or       <COUNT>[:{ppn|cpp}=<X>][#excl]                                   */

  const char *FName = "MJobGetPBSTaskList";

  MDB(5,fPBS) MLog("%s(%s,%s,%s,EMsg)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (TaskString != NULL) ? TaskString : "NULL",
    (TaskList != NULL) ? "TaskList" : "NULL");

  if (EMsg != NULL)
    EMsg[0] = '\0';

  MASSERT(J != NULL, "invalid job when parsing pbs exec_host");
  MASSERT(J->Req[0] != NULL, "invalid job req when parsing pbs exec_host");
  MASSERT(TaskString != NULL, "invalid task string when parsing pbs exec_host");
  MASSERT(TaskList != NULL, "invalid task list when parsing pbs exec_host");

  if (bmisset(&J->IFlags,mjifPBSGPUSSpecified | mjifPBSMICSSpecified))
    {
    /* this code sucks reading in DRes.Procs>1 type requests */

    MPBSJobGetAcceleratorExecHList(J,TaskString,TaskList);

    return(SUCCESS);
    }

  tindex = 0;

  RequestIsProcCentric = MJobReqIsProcCentric(J,TaskString);

  /* NOTE:  ppn is 'processes per node' -> number of threads launched per node/virtual node allocated */
  /* NOTE:  cpp is 'cpus per processor' -> task's DRes.Procs */

  /* NOTE:  only handles one 'ppn' distribution per job (same for gpus) */

  rqindex = 0;

  /* NOTE:  initial support enabled for multiple reqs */

  RQ = J->Req[0];

  OldTC = RQ->TaskCount;

  RQ->TaskCount = 0;

  if (RequestIsProcCentric == FALSE)
    {
    RQ->DRes.Procs = 1;
    }

  HLIndex  = -1;

  ReqIsLoaded = FALSE;

  /* NOTE:  collect all host requests into single req */
  /* NOTE:  PPNSet is 'per req' variable */

  /* job is multi-req if
        TCCount > 1

     or

        TCCount == 1 and HLIndex > -1 */

  ptr = MUStrTok(TaskString,"+ \t",&TokPtr);

  while (ptr != NULL)
    {
    mbool_t NoLoad = FALSE;

    /* Format: exec_host = <node_id>/<proccessor>[+<node_id>/<proccessor>...] */

    MUStrCpy(tmpHostName,ptr,sizeof(tmpHostName));

    MUStrTok(tmpHostName,"/",&TokPtr2);

    /* determine hostname/nodecount */

    NodeLocated = FALSE;

    if (MSched.ManageTransparentVMs == TRUE)
      {
      mvm_t *V;

      if (MUHTGet(&MSched.VMTable,tmpHostName,(void **)&V,NULL) == SUCCESS)
        {
        /* virtual machine allocated */

        N = V->N;

        V->ARes.Procs = 0;
        
        NodeLocated = TRUE;
        }
      }

    if ((NodeLocated == FALSE) && (RM != NULL) &&
        (MOMap(RM->OMap,mxoNode,tmpHostName,TRUE,FALSE,tmpName) == SUCCESS))
      {
      MUStrCpy(tmpHostName,tmpName,sizeof(tmpHostName));
      }

    if ((NodeLocated == FALSE) && (MNodeFind(tmpHostName,&N) == FAILURE))
      {
      if (MSched.SideBySide == TRUE)
        {
        /* job was started by another scheduler on unknown nodes */

        bmset(&J->SpecFlags,mjfNoResources);
        bmset(&J->Flags,mjfNoResources);

        if (EMsg != NULL)
          MUStrCpy(EMsg,"job was started by another scheduler on unknown nodes\n",MMAX_LINE);

        return(FAILURE);
        }

      if (TRUE) /* (MSched.VMGResMap == TRUE) */
        {
        mvm_t *V;

        if (MVMFind(tmpHostName,&V) == FAILURE)
          {
          MDB(1,fPBS) MLog("ALERT:    cannot locate host '%s' for job hostlist\n",
            tmpHostName);

          if (EMsg != NULL)
            MUStrCpy(EMsg,"cannot locate host for job hostlist\n",MMAX_LINE);

          return(FAILURE);
          }

        /* use virtual node's physical node */

        N = V->N;

        if (J->ReqVMList == NULL)
          {
          J->ReqVMList = (mvm_t **)MUCalloc(1,sizeof(mvm_t *) * (MSched.JobMaxTaskCount));
          }

        /* NOTE:  only single req VM/job supported (FIXME) */

        J->ReqVMList[0] = V;
        J->ReqVMList[1] = NULL;
        }
      }  /* END if ((NodeLocated == FALSE) && ...) */

    /* load task list info */

    if (RequestIsProcCentric == FALSE)
      {
      for (index = 0;index < tindex;index++)
        {
        if (TaskList[index] == N->Index)
          {
          NoLoad = TRUE;

          break;
          }
        }

      if (NoLoad == FALSE)
        {
        /* FIXME: for RequestIsProcCentric == FALSE same hosts belong in the same task */
        TaskList[tindex] = N->Index;
        tindex++;
        }
      }    /* END if (RequestIsProcCentric == FALSE) */
    else if (N != NULL)
      {
      TaskList[tindex] = N->Index;
      tindex++;
      }    /* END else if (N != NULL) */

    if (NoLoad == FALSE)
      {
      if ((HLIndex == -1) && (ReqIsLoaded == TRUE))
        {
        /* create new HL req */

        rqindex++;

        if (J->Req[rqindex] == NULL)
          {
          MReqCreate(J,NULL,NULL,FALSE);

          MRMReqPreLoad(J->Req[rqindex],&MRM[J->Req[0]->RMIndex]);
          }

        RQ = J->Req[rqindex];

        RQ->TaskCount = 0;
        }  /* END if ((HLIndex == -1) && (ReqIsLoaded == TRUE)) */

      RQ->TaskCount += 1;

      ReqIsLoaded = TRUE;

      HLIndex = rqindex;
      }
    else
      {
      RQ->DRes.Procs += 1;
      }

    ptr = MUStrTok(NULL,"+ \t",&TokPtr);
    }    /* END while (ptr != NULL) */

  TaskList[tindex] = -1;

  MDB(7,fPBS) MLog("INFO:     %d host task(s) located for job\n",
    tindex);

  if (rqindex > 0)
    {
    MDB(7,fPBS) MLog("INFO:     %d task(s) located for job req %d\n",
      J->Req[rqindex]->TaskCount,
      rqindex);
    }

  if ((RQ->TaskCount == 0) && (OldTC != 0))
    RQ->TaskCount = OldTC;

  /* sync Req TaskCounts (NOTE: Req nodecounts not modified) */

  TC = 0;

  for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
    {
    RQ = J->Req[rqindex];

    if (RQ == NULL)
      break;

    RQ->TaskRequestList[0] = RQ->TaskCount;

    /* Why do we overwrite what is at [1]? 
     * For a trl of 4:8:12 and 12 is being used, 
     * the trl will get set to 12:8:12 */
    
    if (!bmisset(&J->IFlags,mjifTRLRequest))
      RQ->TaskRequestList[1] = RQ->TaskCount; 

    TC += RQ->TaskCount;
    }    /* END for (rqindex) */

  J->Request.TC = MAX(J->Request.TC,TC);

  return(SUCCESS);
  }    /* END MJobParsePBSExecHost() */

/* END MJobPBS.c */
