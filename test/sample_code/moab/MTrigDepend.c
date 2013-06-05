/* HEADER */

      
/**
 * @file MTrigDepend.c
 *
 * Contains: Trigger Dependency functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



enum TrigDependTreeNodeType {
  tdtntNonLeafNode,
  tdtntNullDepend,
  tdtntCCODE,
  tdtntSSETMatches,
  tdtntLAST };

const char *TrigDependTreeNodeTypeAttr[] = {
  "non leaf node",
  "no dependencies",
  "CCODE dependency",
  "no \'sets on success\' matches",
  NULL };

typedef struct tdtnode_t {
  mtrig_t                    *T;
  enum TrigDependTreeNodeType LeafNodeType;
  char                        TrigDependVariable[MMAX_LINE];
  int                         Depth;
} tdtnode_t;

/**
 * Add a trig dependency record to the linked list used to 
 * create a trig dependency tree. 
 *
 * @param TrigDependTreeNodeList (I) pointer to linked list 
 * @param TRec (I) pointer to the record to be added to the 
 *             linked list
 * @param TreeNodeType (I) type of record to be added (non leaf 
 *                     node, leaf node which has no
 *                     dependencies, etc.)
 * @param MatchedDepend (I) pointer to the depend object that 
 *                      matched this trigger
 * @param Depth (I) depth of recursion when match was found
 */

void MTrigDependTreeAddNode(

  mln_t                     **TrigDependTreeNodeList,
  mtrig_t                    *TRec,
  enum TrigDependTreeNodeType TreeNodeType,
  mdepend_t                  *MatchedDepend,
  int                         Depth) 

  {
  tdtnode_t *TrigDependTreeNode;

  /* malloc a record to add to the link list */

  TrigDependTreeNode = (tdtnode_t *)MUCalloc(1,sizeof(tdtnode_t));
    
  TrigDependTreeNode->T = TRec;
  TrigDependTreeNode->LeafNodeType = TreeNodeType;
  TrigDependTreeNode->Depth = Depth;
          
  /* If a non NULL dependency pointer then save the variable that matched the dependency for this trigger */
            
  if (MatchedDepend != NULL)
    {
    MUStrCpy(TrigDependTreeNode->TrigDependVariable,MatchedDepend->Value,sizeof(TrigDependTreeNode->TrigDependVariable));
    }
  else 
    {
    TrigDependTreeNode->TrigDependVariable[0] = 0;
    }

  /* add the record to the linked list - note that it will be freed by MULLFree() when the linked list is freed */

  MULLAdd(TrigDependTreeNodeList,TRec->Name,TrigDependTreeNode,NULL,MUFREE);

  return;
  } /* END MTrigDependTreeAddNode() */


/**
 * This is a recursive function that takes a trigger and 
 * searches for triggers that this trigger is dependent upon. 
 * For each trigger it finds, it adds a dependency record to a 
 * linked list and then recursively follows that trigger, 
 * building up a linked list from which a trigger dependency 
 * tree can be extracted. 
 *  
 * Note - mdiag -T -D 
 *  
 * Note - a dependency is found if the value of one of the 
 * mdepend_t records in the trigger match one of the SSet values 
 * in another trigger in the global trigger array. 
 *  
 * @param T (I) trigger record 
 * @param TriggersAdded (I) pointer to a boolean array to keep 
 *                      track of triggers we have already added
 *                      to our linked list so that we can prune
 *                      our dependency search paths.
 * @param TrigDependTreeNodeList (I) pointer to linked list 
 * @param MatchedDepend (I) pointer to the depend object that 
 *                      matched this trigger
 * @param Depth (I) recursion depth 
 */

int MTrigBuildDependTree(

  mtrig_t   *T,
  mbool_t   *TriggersAdded,
  mln_t    **TrigDependTreeNodeList,
  mdepend_t *MatchedDepend, 
  int       *Depth)
  
  {
  int dindex = 0;
  int MatchFound = FALSE;
  mdepend_t *TDep;

  if (T == NULL)
    {
    /* this should never happen */
    return(FAILURE);
    }

  TDep = (mdepend_t *)MUArrayListGetPtr(&T->Depend,dindex);

  if (TDep == NULL)
    {
    /* this trigger has no dependencies so add it to the linked list as a leaf node with no dependencies */

    MTrigDependTreeAddNode(TrigDependTreeNodeList,T,tdtntNullDepend,MatchedDepend,*Depth); 

    return(SUCCESS);
    }

  if (!strcmp(TDep->Value,"ccode"))
    {
    /* we are done if a trigger has a ccode dependency so add a record to the linked list as a ccode leaf node */

    MTrigDependTreeAddNode(TrigDependTreeNodeList,T,tdtntCCODE,MatchedDepend,*Depth);
     
    return(SUCCESS);
    }

  /* this trigger has dependencies so add it to the linked list as a non leaf node */

  MTrigDependTreeAddNode(TrigDependTreeNodeList,T,tdtntNonLeafNode,MatchedDepend,*Depth); 

  /* for each dependency for this trigger loop through all the triggers in the global trigger array
     looking for other triggers with the same dependency variable in one of their sset entries. */

  while((TDep = (mdepend_t *)MUArrayListGetPtr(&T->Depend,dindex++)) != NULL)
    {
    int   tindex;
    char *TDependString;

    /* Josh said not to check variables with the "!" attribute */

    if (TDep->Type == mdVarNotSet)
      continue;

    TDependString = TDep->Value;

    /* loop through all the triggers in the global trigger array */

    for (tindex = 0;tindex < MTList.NumItems;tindex++)
      {
      int sindex = 0;
      mtrig_t *tmpT;
      char *tmpSet;

      tmpT = (mtrig_t *)MUArrayListGetPtr(&MTList,tindex);

      if (MTrigIsReal(T) == FALSE)
        continue;

      /* skip over the trigger passed into this routine */

      if (!strcmp(tmpT->TName,T->TName))
        continue;

      /* we found a trigger to check, so compare its sset values with our original trigger's dependency variable */

      if (tmpT->SSets != NULL)
        {     
        while((tmpSet = (char *)MUArrayListGetPtr(tmpT->SSets,sindex++)) != NULL)
          {
          /* if this trigger already matched one of our dependencies then don't check it again */
  
          if (TriggersAdded[tindex] == TRUE)
            break;
  
          if (!strcmp(TDependString,tmpSet))
            {
            /* we have found a trigger with a matching sset variable */
  
            TriggersAdded[tindex] = TRUE;
            MatchFound = TRUE;
  
            /* do a recursive call to get the dependencies for the trigger we just found with a matching sset variable
               and add those dependencies to the linked list */
  
            *Depth = *Depth + 1;
            MTrigBuildDependTree(tmpT,TriggersAdded,TrigDependTreeNodeList,TDep,Depth);
            *Depth = *Depth - 1;
            }
          } /* END for sindex*/
        } /* END if (tmpT->SSets != NULL) */
      } /* END for tindex*/
    } /* END for dindex */

  if (MatchFound == FALSE)
    {
    /* we already added this node to the node list above as a non leaf node, but since we could find
       no other triggers with a matching sset variable then add it to the linked list again (overwriting the old entry)
       as a "no sset matches found" leaf node record. */

    MTrigDependTreeAddNode(TrigDependTreeNodeList,T,tdtntSSETMatches,MatchedDepend,*Depth); 

    return(SUCCESS);
    }

  return(FAILURE);
  } /* END MTrigBuildDependTree() */



/**
 * Report trigger dependencies.
 *
 * NOTE: Assumes that OString has ALREADY been initialized
 *
 * @param Auth
 * @param OString (O)
 */

int MTrigDisplayDependencies(

  char      *Auth,
  mstring_t *OString)

  {
  int tindex;

  int size;

  mtrig_t *T;

  mbool_t *TListChecked;

  if ((OString == NULL) || (OString->capacity() <= 0))
    {
    /* OString was not initialized */

    return(FAILURE);
    }

  size = sizeof(mbool_t) * (MTList.NumItems + 1);

  /* iterate through all of the triggers in the global trigger array */

  TListChecked = (mbool_t *)MUCalloc(1,size);

  for (tindex = 0;tindex < MTList.NumItems;tindex++)
    {
    int depth = 0;
    mln_t *TTree = NULL;
    mln_t *TTreeRecord = NULL;

    memset(TListChecked,0,size);

    T = (mtrig_t *)MUArrayListGetPtr(&MTList,tindex);

    if (MTrigIsReal(T) == FALSE)
      continue;

    /* create a linked list to store trigger dependencies for this trigger */

    MULLCreate(&TTree);

    /* build the trigger dependency tree */

    MTrigBuildDependTree(T,TListChecked,&TTree,NULL,&depth);

    MStringAppend(OString,"\n");

    /* go through the trigger dependency linked list that was just build and display the dependency hierarchy */

    while (MULLIterate(TTree,&TTreeRecord) == SUCCESS)
      {
      tdtnode_t *TNode; 
      mtrig_t   *tmpT;

      TNode = (tdtnode_t *)TTreeRecord->Ptr;
      tmpT = TNode->T;
      
      /* headers TBD */
       
      MStringAppendF(OString,"%c %-20.20s   %-40.40s   %-40.40s   %-40.40s ",
          (TNode->LeafNodeType == tdtntNonLeafNode) ? '-' : '+', 
          tmpT->TName,
          tmpT->Name,
          (TNode->Depth != 0) ? TNode->TrigDependVariable : "",
          ((TNode->Depth != 0) || (TNode->LeafNodeType != tdtntNonLeafNode)) ? TrigDependTreeNodeTypeAttr[TNode->LeafNodeType] : "");

      /* print trigger variables for leaf nodes that have dependencies that were not found in other trigger sset variables */

      if (TNode->LeafNodeType == tdtntSSETMatches)
        {
        int dindex = 0;
        mdepend_t *TDep;
        
        while((TDep = (mdepend_t *)MUArrayListGetPtr(&tmpT->Depend,dindex++)) != NULL)
          {
          if (TDep->Type == mdVarNotSet)
            /* Note - if the requirement is a "not set" then we don't even look at those types of requirements */
            continue;

          MStringAppendF(OString,"%-30.30s ",
            TDep->Value);
          }
        }
        MStringAppendF(OString,"\n");
      } /* END while ... */

    MUFree((char **)&TListChecked);
    MULLFree(&TTree,MUFREE);
    } /* END for tindex */

  MStringAppendF(OString,"End Trigger Leaf Node Data\n");

  return(SUCCESS);
  } /* END MTrigDisplayDependencies */
/* END MTrigDepend.c */
