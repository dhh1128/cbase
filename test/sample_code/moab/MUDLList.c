/* HEADER */

/**
 * @file MUDLList.c
 *
 * Double Link List functions
 *
 */

#include "moab.h"
#include "moab-proto.h"

#ifndef STATIC
#define STATIC static
#endif



/**
 * Returns the size of List.
 *
 * @param List (I)
 */

int MUDLListSize(

  mdllist_t *List) /* I */

  {
  return(List->Size);
  } /* END MUDLListSize */ 




/**
 * Creates a new list and returns it, or returns NULL on failure.
 */

mdllist_t *MUDLListCreate()

  {
  mdllist_t *List = (mdllist_t *)MUCalloc(1,sizeof(mdllist_t));

  if (List == NULL)
    return(NULL);

  List->Head = NULL;
  List->Tail = NULL;
  List->Size = 0;

  return(List);
  } /* END MUDLListCreate */ 




/**
 * Frees the list and it's internal data structures. 
 * This does NOT free any data stored in the list!
 *
 * @param L (I)
 */

void MUDLListFree(

  mdllist_t *L) /* I */

  {
  mdlnode_t *ToFree;
  mdlnode_t *N;

  N = MUDLListFirst(L);

  while(N != NULL)
    {
    ToFree = N;
    N = MUDLListNodeNext(N);
    MUFree((char **)&ToFree);
    }

  MUFree((char **)&L);
  } /* END MUDLListFree */ 




/**
 * Internal function link Prev and Node together
 *
 * @param Prev (I)
 * @param Node (I)
 */

static void MUDLLinkPrev(

  mdlnode_t *Prev, /* I */
  mdlnode_t *Node) /* I */

  {
  Prev->Next = Node;
  Node->Prev = Prev;
  } /* END MUDLLinkPrev */ 






/**
 * Add data to the end of the list
 *
 * @param List (I)
 * @param Data (I)
 */

int MUDLListAppend(

  mdllist_t *List, /* I */
  void   *Data) /* I */

  {
  mdlnode_t *Node = (mdlnode_t *)MUCalloc(1,sizeof(mdlnode_t));

  MASSERT((Node != NULL),"failed to append to list\n");

  Node->Data = Data;

  if (List->Size == 0)
     List->Head = Node;
  else
    MUDLLinkPrev(List->Tail, Node);

  List->Tail=Node;
  List->Size++;

  return(SUCCESS);
  } /* END MUDLListAppend */ 




/**
 * Return a node to iterate from the start of the list. 
 * (Arbitrary insertion and deletion is not yet supported.)
 *
 * @param List (I)
 */

mdlnode_t * MUDLListFirst(

  mdllist_t *List) /* I */

  {
  return(List->Head);
  } /* END MUDLListFirst */ 




/**
 * Return a node to iterate from the end of the list. 
 * (Arbitrary insertion and deletion is not yet supported.)
 *
 * @param List (I)
 */

mdlnode_t *MUDLListLast(

  mdllist_t *List) /* I */

  {
  return(List->Tail);
  } /* END MUDLListLast */ 




/**
 * Return the data stored in Node
 *
 * @param Node (I)
 */

void *MUDLListNodeData(

  mdlnode_t *Node) /* I */

  {
  return(Node->Data);
  } /* END MUDLListNodeData */ 




/**
 * Get the node following Node
 *
 * @param Node (I)
 */

mdlnode_t *MUDLListNodeNext(

  mdlnode_t *Node) /* I */

  {
  return(Node->Next);
  } /* END MUDLListNodeNext */ 





/**
 * Internal function to remove the last item from List, asssuming that List
 * has only one item.
 *
 * @param List (I)
 */

STATIC void *MUDLListRemoveFinalItem(

  mdllist_t *List)

  {
  mdlnode_t *N;
  void *Data;

  if (List == NULL)
    {
    return(NULL);
    }

  /* List->Size should be 1 and List->Head should == List->Tail */

  N = List->Head;
  Data = N->Data;
  List->Head = NULL;
  List->Tail = NULL;
  MUFree((char **)&N);
  List->Size = 0;

  return(Data);
  }


/**
 * Remove the first item from the list and return its data, or NULL if
 * the list is empty.
 *
 * @param List (I)
 */

void *MUDLListRemoveFirst(

  mdllist_t *List) /* I */

  {
  mdlnode_t *Node;
  void *Result;

  if (List == NULL)
    return(NULL);

  Node = List->Head;

  if (MUDLListSize(List) == 0)
    {
    return(NULL);
    }
  else if(MUDLListSize(List) == 1)
    {
    return(MUDLListRemoveFinalItem(List));
    }
  else 
    {
    Result = Node->Data;
    List->Head = Node->Next;
    List->Size--;
    MUFree((char **)&Node);

    return(Result);
    }

  /*NOTREACHED*/
  return(NULL);
  } /* END MUDLListRemoveFirst */ 

