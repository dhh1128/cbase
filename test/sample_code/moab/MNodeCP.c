/* HEADER */

      
/**
 * @file MNodeCP.c
 *
 * Contains: Node Check Point functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 * Load node checkpoint attributes.
 *
 * @see MCPRestore() - parent
 * @see MCPStoreCluster() - peer
 * @see MNodeFromXML() - child
 *
 * @param NP (I) [optional]
 * @param SBuf (I) [optional]
 */

int MNodeLoadCP(

  mnode_t *NP,
  const char    *SBuf)
 
  {
  char    tmpName[MMAX_NAME + 1];
  char    NodeName[MMAX_NAME + 1];

  const char   *ptr;
 
  mnode_t *N;
 
  long    CkTime;

  const char   *Buf;

  mxml_t *E = NULL;

  const char *FName = "MNodeLoadCP";
 
  MDB(4,fCKPT) MLog("%s(NP,%s)\n",
    FName,
    (SBuf != NULL) ? SBuf : "NULL");

  Buf = (SBuf != NULL) ? SBuf : MCP.Buffer;

  if (Buf == NULL)
    {
    return(FAILURE);
    }
 
  /* FORMAT:  <NODEHEADER> <NODEID> <CKTIME> <NODESTRING> */
 
  /* determine job name */
 
  sscanf(Buf,"%64s %64s %ld",
    tmpName,
    NodeName,
    &CkTime);
 
  if (((long)MSched.Time - CkTime) > (long)MCP.CPExpirationTime)
    {
    return(SUCCESS);
    }

  if (NP == NULL)
    {
    if (MNodeFind(NodeName,&N) != SUCCESS)
      {
      MDB(5,fCKPT) MLog("INFO:     node '%s' no longer detected\n",
        NodeName);
 
      return(SUCCESS);
      }
    }
  else
    {
    N = NP;
    }

  if ((ptr = strchr(Buf,'<')) == NULL)
    {
    return(FAILURE);
    }
 
  if (MXMLFromString(&E,ptr,NULL,NULL) == FAILURE)
    {
    MDB(3,fCKPT) MLog("ERROR:    node '%s' has invalid checkpoint data (failure converting XML)\n",
      NodeName);
    }

  if (strstr(ptr,MNodeAttr[mnaCfgGRes]))
    {
    MSched.DynamicNodeGResSpecified = TRUE;
    }
 
  MNodeFromXML(N,E,FALSE);
 
  MXMLDestroyE(&E);

  bmset(&N->IFlags,mnifLoadedFromCP);
 
  return(SUCCESS);
  }  /* END MNodeLoadCP() */

