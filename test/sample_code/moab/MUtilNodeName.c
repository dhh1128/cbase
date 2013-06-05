/* HEADER */

      
/**
 * @file MUtilNodeName.c
 *
 * Contains: Node Name manipulation functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 * Compare two node names and return SUCCESS if they are adjacent (contiguous) 
 * in the specified direction.
 *
 * The following examples of node name comparisons return a SUCCESS
 *  odev14 odev15 (direction >= 0)
 *  14 15         (direction >= 0)
 *  odev15 odev14 (direction <= 0)
 *  odev14abc odev15abc (direction >= 0)
 *
 *  In the case of SUCCESS this function will also load the inode numbers in the NodeNum and AdjNodeNum
 *  pointers passed into this function. In the case of FAILURE these may come back as -1;
 *
 * @param NodeName    (I)   Node Name 
 * @param AdjNodeName (I) Potential Adjacent Node Name to compare against Node Name
 * @param NodeNum     (O) Optional Node Number extracted from Node Name
 * @param AdjNodeNum  (O) Optional Node Number extracted from Adjacent Node Name
 * @param Direction   (I/O)  If 0 then set the direction, otherwise test specified direction (<0 descending, >0 ascending)
 *
 * @return            This function returns SUCCESS or FAILURE
 */

int MUNodeNameAdjacent(
    
  char *NodeName,    /* I */
  char *AdjNodeName, /* I */ 
  int *NodeNum,      /* O */ 
  int *AdjNodeNum,   /* O */
  int *Direction)    /* I/O */

  {
  char *NameSuffix;
  char *AdjNameSuffix;
  int PrefixLen = 0;
  int IndexLen = 0;
  int AdjPrefixLen = 0;
  int AdjIndexLen = 0;

  if ((NodeName == NULL) || (AdjNodeName == NULL) || (NodeNum == NULL) || (AdjNodeNum == NULL) || (Direction == NULL))
    {
    return(FAILURE);
    }

  *NodeNum = -1;
  *AdjNodeNum = -1;

  /* Make sure that the node names have digits */

  if (strpbrk(NodeName,MDEF_DIGITCHARLIST) == NULL)
    {
    return(FAILURE);
    }

  if (strpbrk(AdjNodeName,MDEF_DIGITCHARLIST) == NULL)
    {
    return(FAILURE);
    }

  /* Get the offset in the node names where the node number is located */

  MUNodeNameNumberOffset(NodeName, &PrefixLen, &IndexLen);
  MUNodeNameNumberOffset(AdjNodeName, &AdjPrefixLen, &AdjIndexLen);

  /* Make sure that the portion of the name before the number is the same in both node names */

  if (PrefixLen != AdjPrefixLen)
    {
    return(FAILURE);
    }

  if (strncmp(NodeName,AdjNodeName,PrefixLen) != 0) 
    {
    return(FAILURE);
    }

  /* Get the node number and suffix from both node names */
  /*    (e.g. odev10abc - abc is the suffix) */

  *NodeNum = (int)strtol(NodeName + PrefixLen,&NameSuffix,10);
  *AdjNodeNum  = (int)strtol(AdjNodeName  + PrefixLen,&AdjNameSuffix,10);

  /* Make sure that the node name suffix for each name match */

  if (strcmp(NameSuffix,AdjNameSuffix) != 0)
    {
    return(FAILURE);
    }

  /* If the adjacent node number is one less or one more than the node we are comparing with
   * then they are adjacent. */

  if (*AdjNodeNum == (*NodeNum + 1)) /* ascending range */
    {

    if (*Direction == 0) /* direction not specified, fill in the direction */
      {
      *Direction = 1; /* ascending range */
      }
    else if (*Direction < 0) /* specified descending, but this range is ascending */
      {
      return(FAILURE);
      }

    return(SUCCESS);
    }
  else if (*AdjNodeNum == (*NodeNum - 1)) /* descending range */
    {

    if (*Direction == 0) /* direction not specified,  fill in the direction */
      {
      *Direction = -1; /* descending range */
      }
    else if (*Direction > 0) /* specified ascending, but this range is descending */
      {
      return(FAILURE);
      }

    return(SUCCESS);
    }

  return(FAILURE);
  } /* END MUNodeNameAdjacent() */



/**
 * This function takes a node name, and returns the offset of the node number 
 * and its length (e.g. odev14 returns offset 5 and length 2).  
 *
 * @param NodeName (I) Node Name
 * @param NodeNumberOffset (0) Offset of the node number within the node name 
 *    (e.g. 5 in the case of odev14)
 * @param NodeNumberLength (0) Length of the node number (e.g. 2 in the case of odev14)
 */

int MUNodeNameNumberOffset(

  char *NodeName,
  int  *NodeNumberOffset,
  int  *NodeNumberLength)
  {
  int NodeNameLen;
  int tmpIndex;

  if ((NodeName == NULL) || (NodeNumberOffset == NULL) || (NodeNumberLength == NULL))
    {
    return(FAILURE);
    }

  *NodeNumberOffset = 0;
  *NodeNumberLength = 0;

  NodeNameLen = strlen(NodeName);

  if (NodeNameLen == 0)
    {
    return(SUCCESS);
    }

  for (tmpIndex = NodeNameLen - 1; tmpIndex >= 0; tmpIndex--)
    {
    if (!isdigit(NodeName[tmpIndex]))
      {
      break;
      }
    }

  *NodeNumberOffset = tmpIndex + 1;
  *NodeNumberLength = NodeNameLen - *NodeNumberOffset;

  return(SUCCESS);
  } /* END MUNodeNameNumberOffset() */

/* END MUtilNodeName.c */
