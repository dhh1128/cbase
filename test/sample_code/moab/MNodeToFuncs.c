/* HEADER */

      
/**
 * @file MNodeToFuncs.c
 *
 * Contains: Node to various other representations functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  





/**
 * Report node attributes as string
 * Convert mnode_t to a string for checkpointing.
 *
 * @see MCPStoreCluster() - parent
 *
 * NOTE: used for checkpointing key node attributes.
 * NOTE: only used for checkpointing
 *
 * @param N (I)
 * @param Buf (O)
 */

int MNodeToString(
 
  mnode_t   *N,     /* I */
  mstring_t *Buf)   /* I */
 
  {
  int aindex = 0;

  enum MNodeAttrEnum AList[mnaLAST];

  mxml_t *E = NULL;
 
  if ((N == NULL) || (Buf == NULL))
    {
    return(FAILURE);
    }

  AList[aindex++] = mnaAllocRes;

  if (MSched.DynamicNodeGResSpecified == TRUE)
    {
    AList[aindex++] = mnaCfgGRes;
    }

  if (!bmisset(&N->RMAOBM,mnaPowerIsEnabled))
    {
    AList[aindex++] = mnaPowerState;
    }

  AList[aindex++] =  mnaVariables;
  AList[aindex++] =  mnaOldMessages,
  AList[aindex++] =  mnaPowerSelectState,
  AList[aindex++] =  mnaStatATime,
  AList[aindex++] =  mnaStatTTime,
  AList[aindex++] =  mnaStatUTime,
  AList[aindex++] =  mnaFlags,
  AList[aindex]   = mnaNONE;

  MXMLCreateE(&E,(char *)MXO[mxoNode]);
 
  MNodeToXML(
    N,
    E,
    AList,
    0,
    TRUE,
    FALSE,
    NULL,
    NULL);
 
  MXMLToMString(E,Buf,NULL,TRUE);
 
  MXMLDestroyE(&E);
 
  return(SUCCESS);
  }  /* END MNodeToString() */



/**
 * Converts a node to a WIKI formatted string.
 *
 * @param N      (I) - The node to describe
 * @param Msg    (I) [optional] - Optional message to be appended with node's messages
 * @param String (O) - Resulting string (original contents will be cleared)
 */

int MNodeToWikiString(
  mnode_t *N,
  char    *Msg,
  mstring_t *String)

  {
  int tmpIndex;

  if ((N == NULL) || (String == NULL))
    {
    return(FAILURE);
    }

  mstring_t tmpString(MMAX_LINE);

  /* NAME */

  MStringSet(String,N->Name);

  /* STATE */

  if (N->State != mnsNONE)
    {
    MStringAppendF(String," %s=%s",
      MWikiNodeAttr[mwnaState],
      MNodeState[N->State]);
    }

  /* POWER */

  if (N->PowerState != mpowNONE)
    {
    MStringAppendF(String," %s=%s",
      MWikiNodeAttr[mwnaPowerIsEnabled],
      MPowerState[N->PowerState]);
    }

  /* PARTITION */

  if (N->PtIndex > 0)
    {
    MStringAppendF(String," %s=%s",
      MWikiNodeAttr[mwnaPartition],
      MPar[N->PtIndex].Name);
    }

  /* ARCHITECTURE */

  if (N->Arch > 0)
    {
    MStringAppendF(String," %s=%s",
      MWikiNodeAttr[mwnaArch],
      MAList[meArch][N->Arch]);
    }

  /* ADISK */

  if (N->ARes.Disk > 0)
    {
    MStringAppendF(String," %s=%d",
      MWikiNodeAttr[mwnaADisk],
      N->ARes.Disk);
    }

  /* AMEM */

  if (N->ARes.Mem > 0)
    {
    MStringAppendF(String," %s=%d",
      MWikiNodeAttr[mwnaAMemory],
      N->ARes.Mem);
    }

  /* APROCS */

  if (N->ARes.Procs > 0)
    {
    MStringAppendF(String," %s=%d",
      MWikiNodeAttr[mwnaAProc],
      N->ARes.Procs);
    }

  /* ASWAP */

  if (N->ARes.Swap > 0)
    {
    MStringAppendF(String," %s=%d",
      MWikiNodeAttr[mwnaASwap],
      N->ARes.Swap);
    }

  /* ARES */

  {
  mbool_t AResPrinted = FALSE;

  for (tmpIndex = 0;tmpIndex < MSched.M[mxoxGRes];tmpIndex++)
    {
    if (MGRes.Name[tmpIndex][0] == '\0')
      break;

    if (MSNLGetIndexCount(&N->ARes.GenericRes,tmpIndex) > 0)
      {
      if (AResPrinted == TRUE)
        {
        MStringAppendF(String,",%s:%d",
          MGRes.Name[tmpIndex],
          MSNLGetIndexCount(&N->ARes.GenericRes,tmpIndex));
        }
      else
        {
        MStringAppendF(String," %s=%s:%d",
          MWikiNodeAttr[mwnaARes],
          MGRes.Name[tmpIndex],
          MSNLGetIndexCount(&N->ARes.GenericRes,tmpIndex));

        AResPrinted = TRUE;
        }
      }
    }
  } /* END BLOCK ARES */

  /* CDISK */

  if (N->CRes.Disk > 0)
    {
    MStringAppendF(String," %s=%d",
      MWikiNodeAttr[mwnaCDisk],
      N->CRes.Disk);
    }

  /* CMEM */

  if (N->CRes.Mem > 0)
    {
    MStringAppendF(String," %s=%d",
      MWikiNodeAttr[mwnaCMemory],
      N->CRes.Mem);
    }

  /* CPROCS */

  if (N->CRes.Procs > 0)
    {
    MStringAppendF(String," %s=%d",
      MWikiNodeAttr[mwnaCProc],
      N->CRes.Procs);
    }

  /* CSWAP */

  if (N->CRes.Swap > 0)
    {
    MStringAppendF(String," %s=%d",
      MWikiNodeAttr[mwnaCSwap],
      N->CRes.Swap);
    }

  /* CRES */

  {
  mbool_t CResPrinted = FALSE;

  for (tmpIndex = 0;tmpIndex < MSched.M[mxoxGRes];tmpIndex++)
    {
    if (MGRes.Name[tmpIndex][0] == '\0')
      break;

    if (MSNLGetIndexCount(&N->CRes.GenericRes,tmpIndex) > 0)
      {
      if (CResPrinted == TRUE)
        {
        MStringAppendF(String,",%s:%d",
          MGRes.Name[tmpIndex],
          MSNLGetIndexCount(&N->CRes.GenericRes,tmpIndex));
        }
      else
        {
        MStringAppendF(String," %s=%s:%d",
          MWikiNodeAttr[mwnaCRes],
          MGRes.Name[tmpIndex],
          MSNLGetIndexCount(&N->CRes.GenericRes,tmpIndex));

        CResPrinted = TRUE;
        }
      }
    }
  } /* END BLOCK ARES */

  /* RACK */

  if (N->RackIndex > 0)
    {
    MStringAppendF(String," %s=%d",
      MWikiNodeAttr[mwnaRack],
      N->RackIndex);
    }

  /* SLOT */

  if (N->SlotIndex > 0)
    {
    MStringAppendF(String," %s=%d",
      MWikiNodeAttr[mwnaSlot],
      N->SlotIndex);
    }

  /* OS */

  if (N->ActiveOS > 0)
    {
    MStringAppendF(String," %s=%s",
      MWikiNodeAttr[mwnaOS],
      MAList[meOpsys][N->ActiveOS]);
    }

  /* VARIABLES */

  if (!MUHTIsEmpty(&N->Variables))
    {
    char *VarName = NULL;
    char *VarVal  = NULL;
    int nodeindex;

    mhashiter_t HTI;

    MStringAppendF(String," %s=",
      MWikiNodeAttr[mwnaVariables]);

    MUHTIterInit(&HTI);

    for (nodeindex = 0;MUHTIterate(&N->Variables,&VarName,(void **)&VarVal,NULL,&HTI) == SUCCESS;nodeindex++)
      {
      MStringAppendF(String,"%s%s%s%s",
        (nodeindex > 0) ? "," : "",
        VarName,
        (VarVal != NULL) ? "=" : "",
        (VarVal != NULL) ? VarVal : "");
      }
    } /* END if (N->Variables != NULL) */

  /* MESSAGES */

  if ((N->MB != NULL) || (Msg != NULL))
    {
    char tmpLine[MMAX_LINE << 2];
    mbool_t MMBPrinted = FALSE;

    /* We need a MMBPrintMessagesToMString to not have to use tmpLine */

    MMBPrintMessages(
      N->MB,
      mfmNONE,
      TRUE,        /* verbose */
      -1,
      tmpLine,
      sizeof(tmpLine));

    if (tmpLine[0] != '\0')
      {
      mstring_t packString(MMAX_BUFFER);

      MUMStringPack(tmpLine,&packString);

      tmpString = packString;

      MStringAppendF(String," %s=\"%s",
        MWikiNodeAttr[mwnaMessage],
        tmpString.c_str());

      MMBPrinted = TRUE;
      }

    if (Msg != NULL)
      {
      mstring_t packString(MMAX_BUFFER);

      MUMStringPack(Msg,&packString);

      tmpString = packString;

      if (MMBPrinted == FALSE)
        {
        MStringAppendF(String," %s=\"",
          MWikiNodeAttr[mwnaMessage]);
        }
      else
        {
        MStringAppend(String,",");
        }

      *String += tmpString;
      }

    MStringAppend(String,"\""); 
    } /* END if ((N->MB != NULL) || (Msg != NULL)) */

  /* GMETRICS */

  if ((N->XLoad != NULL) && (N->XLoad->GMetric != NULL))
    {
    for (tmpIndex = 1;tmpIndex < MSched.M[mxoxGMetric];tmpIndex++)
      {
      if ((N->XLoad->GMetric[tmpIndex] == NULL) ||
          (N->XLoad->GMetric[tmpIndex]->IntervalLoad == MDEF_GMETRIC_VALUE))
        {
        /* GMetric not specified for node */

        continue;
        }

      MStringAppendF(String," %s[%s]=%.2f",
        MWikiNodeAttr[mwnaGMetric],
        MGMetric.Name[tmpIndex],
        N->XLoad->GMetric[tmpIndex]->IntervalLoad);
      } /* END for (tmpIndex = 1;...) */
    } /* END if ((N->XLoad != NULL) && (N->XLoad->GMetric != NULL)) */

  /* MASTER RM */

  if (N->RM != NULL)
    {
    MStringAppendF(String," %s=%s",
      MWikiNodeAttr[mwnaRM],
      N->RM->Name);
    }

  /* NODE ACCESS POLICY */

  if (N->EffNAPolicy != mnacNONE)
    {
    MStringAppendF(String," %s=%s",
      MWikiNodeAttr[mwnaEffNAccessPolicy],
      MNAccessPolicy[N->EffNAPolicy]);
    }

  /* VM OS LIST */

  if ((N->VMOSList != NULL) || (MSched.DefaultN.VMOSList != NULL))
    {
    int oindex;
    mnodea_t *OSList;

    MStringAppendF(String," %s=",
      MWikiNodeAttr[mwnaVMOSList]);

    OSList = (N->VMOSList != NULL) ? N->VMOSList : MSched.DefaultN.VMOSList;

    for (oindex = 0;OSList[oindex].AIndex != 0;oindex++)
      {
      if (oindex > 0)
        MStringAppend(String,",");

      MStringAppend(String,MAList[meOpsys][OSList[oindex].AIndex]);
      }
    } /* END if ((N->VMOSList != NULL) || (MSched.DefaultN.VMOSList != NULL)) */

  /* FEATURES */

  if (!bmisclear(&N->FBM))
    {
    char Features[MMAX_LINE];

    MUNodeFeaturesToString('\0',&N->FBM,Features);

    MStringAppendF(String," %s=%s",
      MWikiNodeAttr[mwnaFeature],
      Features);
    }

  /* CLASSES */

  if (!bmisclear(&N->Classes))
    {
    mstring_t Classes(MMAX_LINE);

    MClassListToMString(&N->Classes,&Classes,NULL);

    MStringAppendF(String," %s=%s",
      MWikiNodeAttr[mwnaCClass],
      Classes.c_str());
    }

  /* WIKI string, always finish with \n */

  String += '\n';

  return(SUCCESS);
  } /* END MNodeToWikiString() */
/* END MNodeToFuncs.c */
