/* HEADER */

      
/**
 * @file MRMFind.c
 *
 * Contains: RM Find functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 * Find RM by name (case insensitive)
 *
 * @param RMName (I)
 * @param RP (O) [optional]
 */

int MRMFind(
 
  const char  *RMName,
  mrm_t      **RP)
 
  {
  /* if found, return success with RP pointing to RM */
 
  int    rmindex;
  mrm_t *R;
 
  if (RP != NULL)
    *RP = NULL;
 
  if ((RMName == NULL) ||
      (RMName[0] == '\0'))
    {
    return(FAILURE);
    }
 
  for (rmindex = 0;rmindex < MSched.M[mxoRM];rmindex++)
    {
    R = &MRM[rmindex];

    if (R->Name[0] == '\0')
      break;
 
    if ((R->Type == mrmtNONE) || (R->Name[0] == '\1'))
      continue;
 
    if (strcasecmp(R->Name,RMName) != 0)
      continue;
 
    /* RM found */
 
    if (RP != NULL)
      *RP = R;
 
    return(SUCCESS);
    }  /* END for (rmindex) */
 
  /* entire table searched */ 
 
  return(FAILURE);
  }  /* END MRMFind() */





/**
 * Find RM object associated with specified peer
 *
 * @param Auth (I)
 * @param RP (O) [optional]
 * @param IsClient (I) when TRUE, only RM's with the CLIENT flag are valid matches
 */

int MRMFindByPeer(
 
  char     *Auth,     /* I */
  mrm_t   **RP,       /* O (optional) */
  mbool_t   IsClient) /* I when TRUE, only RM's with the CLIENT flag are valid matches */
 
  {
  /* this routine first searches by <AUTH/SCHEDNAME>.INBOUND,
   * then by RM->ClientName. If found, return success with RP pointing to RM */
   
  int    rmindex;
  mrm_t *R;
  char   tmpName[MMAX_NAME];
 
  if (RP != NULL)
    *RP = NULL;
 
  if ((Auth == NULL) ||
      (Auth[0] == '\0'))
    {
    return(FAILURE);
    }

  if (!strncasecmp(Auth,"peer:",strlen("peer:")))
    {
    /* remove PEER: prefix */

    Auth = Auth + strlen("peer:");
    }

  snprintf(tmpName,sizeof(tmpName),"%s.INBOUND",
    Auth);

  /* search RM->Name against <AUTH>.INBOUND */

  for (rmindex = 0;rmindex < MSched.M[mxoRM];rmindex++)
    {
    R = &MRM[rmindex];
 
    if (R->Name[0] == '\0')
      break;

    if (MRMIsReal(R) == FALSE)
      continue;
 
    if (strcasecmp(R->Name,tmpName) != 0)
      continue;

    if (IsClient == TRUE)
      {
      if (!bmisset(&R->Flags,mrmfClient))
        continue;
      }
 
    /* RM found */
 
    if (RP != NULL)
      *RP = R;
 
    return(SUCCESS);
    }  /* END for (rmindex) */

  /* search RM->ClientName against <AUTH/SCHEDNAME> */

  for (rmindex = 0;rmindex < MSched.M[mxoRM];rmindex++)
    {
    R = &MRM[rmindex];
 
    if (R->Name[0] == '\0')
      break;

    if (MRMIsReal(R) == FALSE)
      continue;
 
    if (strcasecmp(R->ClientName,Auth) != 0)
      continue;

    if (IsClient == TRUE)
      {
      if (!bmisset(&R->Flags,mrmfClient))
        continue;
      }
 
    /* RM found */
 
    if (RP != NULL)
      *RP = R;
 
    return(SUCCESS);
    }  /* END for (rmindex) */

  /* entire table searched twice */
 
  return(FAILURE);
  }  /* END MRMFindByPeer() */
/* END MRMFind.c */
