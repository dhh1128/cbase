/* HEADER */

      
/**
 * @file MRMLoadOMap.c
 *
 * Contains: MRMLoadOMap
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 * Load object translation map.
 *
 * @param R (I) [modified]
 * @param OMapServer (I)
 * @param EMsg (O) [optional,minsize=MMAX_LINE]
 */

int MRMLoadOMap(

  mrm_t *R,           /* I (modified) */
  char  *OMapServer,  /* I */
  char  *EMsg)        /* O (optional,minsize=MMAX_LINE) */

  {
  char tmpProtoString[MMAX_NAME];
  char tmpDir[MMAX_NAME];
  char *ptr;
  char *TokPtr;
  char *ptr2;
  char *TokPtr2;
  char Response[MMAX_BUFFER];

  enum MXMLOTypeEnum OType;

  int index;
  int SC;

  const char *FName = "MRMLoadOMap";

  MDB(2,fRM) MLog("%s(%s,%s)\n",
    FName,
    (R != NULL) ? R->Name : "NULL",
    (OMapServer != NULL) ? OMapServer : "NULL");

  if (EMsg != NULL)
    EMsg[0] = '\0';

  if ((R == NULL) || (OMapServer == NULL) || (OMapServer[0] == '\0'))
    {
    if (EMsg != NULL)
      {
      snprintf(EMsg,MMAX_LINE,"OMap server not specified");
      }

    return(FAILURE);
    }

  if (R->OMap == NULL)
    {
    R->OMap = (momap_t *)MUCalloc(1,sizeof(momap_t));
    }

  /* NOTE:  only supports exec and file protocols */

  if (MUURLParse(
       OMapServer,
       tmpProtoString,
       NULL,
       tmpDir,
       sizeof(tmpDir),
       NULL,
       TRUE,
       TRUE) == FAILURE)
    {
    MDB(2,fRM) MLog("ALERT:    cannot parse object map URL '%s'\n",
      (OMapServer != NULL) ? OMapServer : "NULL");

    if (EMsg != NULL)
      {
      snprintf(EMsg,MMAX_LINE,"cannot parse OMap server URL");
      }

    return(FAILURE);
    }

  R->OMap->Protocol = (enum MBaseProtocolEnum)MUGetIndexCI(
    tmpProtoString,
    MBaseProtocol,
    FALSE,
    mbpNONE);

  if (R->OMap->Protocol == mbpNONE)
    {
    MDB(2,fRM) MLog("ALERT:    invalid object map protocol specified in URL '%s'\n",
      (OMapServer != NULL) ? OMapServer : "NULL");

    if (EMsg != NULL)
      {
      snprintf(EMsg,MMAX_LINE,"invalid protocol specified in OMap server URL");
      }

    return(FAILURE);
    }

  switch (R->OMap->Protocol)
    {
    case mbpFile:

      if ((ptr = MFULoad(tmpDir,1,macmWrite,NULL,NULL,&SC)) == NULL)
        {
        MDB(2,fCONFIG) MLog("WARNING:  cannot load OMap server file '%s'\n",
          tmpDir);

        if (EMsg != NULL)
          {
          snprintf(EMsg,MMAX_LINE,"cannot load OMap server file '%s'",
            tmpDir);
          }

        return(FAILURE);
        }

      MUStrCpy(Response,ptr,sizeof(Response));

      MUFree(&ptr);

      if (Response[0] == '\0')
        {
        MDB(2,fCONFIG) MLog("WARNING:  file '%s' is empty\n",
          tmpDir);

        if (EMsg != NULL)
          {
          snprintf(EMsg,MMAX_LINE,"OMap server file is empty");
          }

        return(FAILURE);
        }

      break;

    case mbpExec:

      /* NOTE:  stderr/stdout not merged */

      if (MUReadPipe(tmpDir,NULL,Response,sizeof(Response),NULL) == FAILURE)
        {
        MDB(1,fNAT) MLog("ALERT:    cannot read output of command '%s'\n",
          tmpDir);

        if (EMsg != NULL)
          {
          snprintf(EMsg,MMAX_LINE,"cannot read output of OMap server URL");
          }

        return(FAILURE);
        }

      if (Response[0] == '\0')
        {
        MDB(2,fCONFIG) MLog("WARNING:  cannot read output of command '%s'\n",
          tmpDir);

        if (EMsg != NULL)
          {
          snprintf(EMsg,MMAX_LINE,"OMap server output is empty");
          }

        return(FAILURE);
        }

      break;

    default:

      /* NOT SUPPORTED */

      MDB(1,fNAT) MLog("ALERT:    protocol not supported\n");

      if (EMsg != NULL)
        {
        snprintf(EMsg,MMAX_LINE,"specified OMap server protocol not supported");
        }

      return(FAILURE);

      /*NOTREACHED*/

      break;
    }  /* END switch (R->OMap->Protocol) */

  /* Response contains output of file or exec */

  /* locate available slot in R->OMap */

  if (Response[0] == '\0')
    {
    /* cred map is empty */

    MDB(1,fNAT) MLog("ALERT:    object map %s' is empty\n",
      tmpDir);

    if (EMsg != NULL)
      {
      snprintf(EMsg,MMAX_LINE,"OMap is empty");
      }

    return(SUCCESS);
    }

  MUStrDup(&R->OMap->Path,tmpDir);

  index = 0;

  while (R->OMap->OType[index] != mxoNONE)
    {
    index++;
    }

  /* FORMAT:  [local]<CREDTYPE>:<SPECCRED>,<TRANSCRED> */

  ptr = MUStrTok(Response,"\n",&TokPtr);

  while (ptr != NULL)
    {
    ptr2 = MUStrTok(ptr,": \t\n",&TokPtr2);

    if ((ptr2 == NULL) || (ptr2[0] == '#'))
      {
      /* empty line or comment - ignore */

      ptr = MUStrTok(NULL,"\n",&TokPtr);

      continue;
      }

    if (!strncasecmp(ptr2,"local",strlen("local")))
      {
      R->OMap->Local[index] = TRUE;

      OType = (enum MXMLOTypeEnum)MUGetIndexCI(
        &ptr2[strlen("local")],
        MXO,
        FALSE,
        mxoNONE);
      }
    else
      {
      R->OMap->Local[index] = FALSE;

      OType = (enum MXMLOTypeEnum)MUGetIndexCI(ptr2,MXO,FALSE,mxoNONE);
      }

    if (OType == mxoNONE)
      {
      /* remap special cases */

      if (!strcasecmp(ptr2,"file") || !strcasecmp(ptr2,"dir"))
        {
        OType = mxoCluster;
        }
      }    /* END if (OType == mxoNONE) */

    if ((OType == mxoNONE) ||
        ((OType != mxoUser) &&
         (OType != mxoGroup) &&
         (OType != mxoClass) &&
         (OType != mxoQOS) &&
         (OType != mxoAcct) &&
         (OType != mxoNode) &&
         (OType != mxoCluster)))  /* HACK: cluster is used to specify file and directory mappings */
      {
      /* bad line */

      if ((EMsg != NULL) && (EMsg[0] == '\0'))
        {
        snprintf(EMsg,MMAX_LINE,"invalid keyword specified in OMap server output '%s'",
          ptr2);
        }

      ptr = MUStrTok(NULL,"\n",&TokPtr);

      continue;
      }

    R->OMap->OType[index] = OType;

    R->OMap->IsMapped[OType] = TRUE;

    ptr2 = MUStrTok(NULL,",",&TokPtr2);

    MUStrDup(&R->OMap->SpecName[index],ptr2);
    MUStrDup(&R->OMap->TransName[index],TokPtr2);               

    index++;

    ptr = MUStrTok(NULL,"\n",&TokPtr);
    }  /* END while (ptr != NULL) */

  return(SUCCESS);
  }  /* END MRMLoadOMap() */
/* END MRMLoadOMap.c */
