/* HEADER */

/**
 * @file MClassExpression.c
 *
 * Contains routines to handle class expressions in 
 * the format CLASS:<CLASSID>[,<CLASSID>] 
 */


#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  

/**
 * Expands the CLASS expression into a list of nodes.
 *
 *          CLASS:<CLASSID>[,<CLASSID>] 
 *  
 * @param Expression (I)
 * @param NodeList   (O)
 * @param P          (I)
 */

int MRsvExpandClassExpression(

  char     *Expression,
  marray_t *NodeList,
  mpar_t   *P)

  {
  char *ptr2;
  char *TokPtr2 = NULL;

  mclass_t *C;
  mnode_t  *N;

  int     nindex;
  mbool_t inlist = FALSE;

  MASSERT(Expression != NULL, "NULL Expression pointer expanding Class Expression");
  MASSERT(P != NULL, "NULL Partition pointer expanding Class Expression");

  ptr2 = MUStrTok(Expression,":",&TokPtr2);
  ptr2 = MUStrTok(NULL,",:",&TokPtr2); /* get class value string */

  while(ptr2 != NULL)
    {
    int cindex;

    if (MClassFind(ptr2,&C) == FAILURE)
      {
      /* requested class not located */

      ptr2 = MUStrTok(NULL,",:",&TokPtr2); /* Get next class name*/
      continue;
      }

    for (nindex = 0;nindex < MSched.M[mxoNode];nindex++)
      {
      N = MNode[nindex];

      if (N == NULL)
        break;

      if (!bmisset(&N->Classes,C->Index))
        {
        /* requested class not on node */

        continue;
        }

      /* If a partition was specified and the node is not on the specified partition then continue */

      if ((P->Index != 0) && (N->PtIndex != P->Index))
        {
        /* requested partition not on node */

        continue;
        }

      /* Make sure the node is not already in the list due to multiple class membership. */
      inlist = FALSE;
      for (cindex = 0;cindex < NodeList->NumItems; cindex++)
        {
        mnode_t *tmpN = (mnode_t *)MUArrayListGetPtr(NodeList,cindex);

        if (tmpN == N)
          {
          /* Node is already in the list */
          inlist = TRUE;
          break;
          }
        }

      if (inlist == FALSE)
        {
        MUArrayListAppendPtr(NodeList,N);
        }
      } /* END for (nindex = 0;nindex < MSched.M[mxoNode];nindex++) */

    ptr2 = MUStrTok(NULL, ",:", &TokPtr2); /* Get next class name*/
    } /* END  while(ptr2 != NULL) */

  return(NodeList->NumItems);
  } /* END int MRsvExpandClassExpression()*/



/**
 * Searches the CLASS expression to determine if the specified 
 * node has one of the classes. 
 *
 *          CLASS:<CLASSID>[,<CLASSID>] 
 *  
 * @param Expression (I)
 * @param N (I) 
 *  
 * @return TRUE if at least one class is found on the Node.
 */

mbool_t MNodeHasClassInExpression(

  char    *Expression,
  mnode_t *N)

  {
  char *ptr;
  char *TokPtr = NULL;

  mclass_t *C;
  int     NC = 0;

  MASSERT(Expression != NULL, "NULL Expression pointer testing for Class in Expression");
  MASSERT(N != NULL, "NULL Node pointer Class in Expression");

  /* Parse off "class:" */
  ptr = MUStrTok(Expression,":",&TokPtr);
  ptr = MUStrTok(NULL,",:",&TokPtr);  /* Point to first class name */

  while(ptr != NULL)
    {
    
    /* If class can be found. */
    if (MClassFind(ptr,&C) != FAILURE)
      {
      /* If Node has it */
      if (bmisset(&N->Classes,C->Index))
        {
        NC++; /* Count it */
        break;  /* We only need to find one class */
        }
      }

    ptr = MUStrTok(NULL,",:",&TokPtr); /* Get next class name */
    }

  return(NC != 0);  /* Return TRUE if class found in node */
  } /* END MNodeHasClassInExpression() */

