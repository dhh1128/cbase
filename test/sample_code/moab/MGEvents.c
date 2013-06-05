/* HEADER */

/**
 * @file MGEvents.c
 *
 * Contains Functions dealing with GEVENTS
 */


#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 * GEvents Description symbol (hash) table - singleton
 */
static mhash_t   GEventsDescSymbolTable;


/**
 * Free the GEvents Desc Symbol Table
 *
 * @param   FreeObjects   (I)
 * @param   Fn            (I)   [optional]
 * @return  SUCCESS or FAILURE
 */

int MGEventFreeDesc(
    
  mbool_t FreeObjects,
  int     (*Fn)(void **))

  {
  return(MUHTFree(&GEventsDescSymbolTable,FreeObjects,Fn));
  }

/** 
 * Adds a new mgevent_desc_t to the given hash table
 *
 * If an event by that name already exists, just returns SUCCESS.
 * Returns failure if it fails to add the key
 *
 * @param   Name    (I)
 * @return  SUCCESS or FAILURE
 */

int MGEventAddDesc(

  const char *Name)

  {
  mgevent_desc_t *GEventDescP;

  if (Name == NULL)
    {
    return(FAILURE);
    }

  if (MUHTGet(&GEventsDescSymbolTable,Name,(void **)&GEventDescP,NULL) != SUCCESS)
    {
    /* Key doesn't exist, add in */

    if ((GEventDescP = (mgevent_desc_t *)MUCalloc(1,sizeof(mgevent_desc_t))) == NULL)
      {
      return(FAILURE);
      }

    if (MUStrDup(&GEventDescP->Name,Name) != SUCCESS)
      {
      MUFree((char**)&GEventDescP);
      return(FAILURE);
      }

    if (MUHTAdd(&GEventsDescSymbolTable,Name,(void *)GEventDescP,NULL,NULL) != SUCCESS)
      {
      MUFree((char**)&GEventDescP->Name);
      MUFree((char**)&GEventDescP);
      return(FAILURE);
      }
    }

  return(SUCCESS);
  }  /* END MSchedAddDesc() */


/**
 * Returns a pointer to the specified GEvent 'name' if present
 *
 * @param   Name          (I)
 * @param   GEventDescP   (O)
 * @return  SUCCESS or FAILURE
 */

int MGEventGetDescInfo(

  const char       *Name,
  mgevent_desc_t  **GEventDescP)

  {
  return(MUHTGet(&GEventsDescSymbolTable,Name,(void**)GEventDescP,NULL));
  }


/**
 * Returns a count of items in the GEventsDescSymbolTable
 *
 * @return  the count of items in the GEvent Description symbol table
 */

int MGEventGetDescCount(void)

  {
  return(GEventsDescSymbolTable.NumItems);
  } /* END MGEventGetDescCount() */


/**
 * Initializes a GEvent iterator
 *
 * @param   Iter        (I/O)   [modified]
 *
 * @return  SUCCESS or FAILURE
 */

int MGEventIterInit(
    
  mgevent_iter_t *Iter)

  {
  if (NULL == Iter)
    {
    return(FAILURE);
    }

  return(MUHTIterInit(&Iter->htiter));
  } /* MGEventIterInit() */


/**
 * Sets up the iterator for the next item in the desc symbol table
 *
 * @param   Name        (O)
 * @param   GEventDescP (O)
 * @param   Iter        (I/O)   [modified]
 * @return  SUCCESS or FAILURE
 */
int MGEventDescIterate(

  char                **Name,
  mgevent_desc_t      **GEventDescP,
  mgevent_iter_t       *Iter)

  {
  if (NULL == Iter)
    {
    return(FAILURE);
    }

  return(MUHTIterate(&GEventsDescSymbolTable,Name,(void**)GEventDescP,NULL,&Iter->htiter));
  } /* END MGEventDescIterate() */

/**
 * Sets up the iterator for the next item in the object list
 *
 * @param   List    (I)
 * @param   Name    (O)
 * @param   GEventObjP  (O)
 * @param   Iter    (I/O)   [modified]
 * @return  SUCCESS or FAILURE
 */
int MGEventItemIterate(

  mgevent_list_t       *List,
  char                **Name,
  mgevent_obj_t       **GEventObjP,
  mgevent_iter_t       *Iter)

  {
  if ((NULL == List) || (NULL == Iter))
    {
    return(FAILURE);
    }

  return(MUHTIterate(&List->Hash,Name,(void**)GEventObjP,NULL,&Iter->htiter));
  } /* MGEventItemIterate() */

/**
 * Returns a count of items in the List's collection
 *
 * @param   List    (I)
 * @return  the count of items 
 */

int MGEventGetItemCount(
    
  mgevent_list_t    *List)

  {
  if (NULL == List)
    {
    return(0);
    }

  return(List->Hash.NumItems);
  } /* END MGEventGetItemCount() */

/**
 * Remove the specified item (denoted by Name) from the List which may contain it,
 * and return the item 
 *
 * @param   List    (I)
 * @param   Name    (I)
 * @return  SUCCESS or FAILURE
 */

int MGEventRemoveItem(
  
  mgevent_list_t   *List,
  char             *Name)

 {
 if (NULL == List)
   {
   return(FAILURE);
   }

 return(MUHTRemove(&List->Hash,Name,NULL));
 } /* END MGEventRemoveItem() */


/** 
 * Adds a new mgevent_obj_t (named 'Name') to the given hash table.
 * If an event by that name already exists, just returns SUCCESS.
 * Returns failure if it fails to add the key
 *
 * @param   Name    (I)
 * @param   List    (I) modified
 * @return  SUCCESS or FAILURE
 */

int MGEventAddItem (

  const char      *Name,
  mgevent_list_t  *List)

  {
  mgevent_obj_t *GEventItem;

  if ((NULL == Name) || (NULL == List))
    {
    return(FAILURE);
    }

  if (MUHTGet(&List->Hash,Name,(void **)&GEventItem,NULL) != SUCCESS)
    {
    /* Key doesn't exist, add in */

    if ((GEventItem = (mgevent_obj_t *)MUCalloc(1,sizeof(mgevent_obj_t))) == NULL)
      {
      return(FAILURE);
      }

    if (MUStrDup(&GEventItem->Name,Name) != SUCCESS)
      {
      MUFree((char**)&GEventItem);
      return(FAILURE);
      }

    if (MUHTAdd(&List->Hash,Name,(void *)GEventItem,NULL,NULL) != SUCCESS)
      {
      MUFree((char**)&GEventItem->Name);
      MUFree((char**)&GEventItem);
      return(FAILURE);
      }
    }  /* END if (MUHTGet()...) */

  return(SUCCESS);
  }  /* END MGEventAddItem() */



/**
 * Gets the requested gevent from the list.
 *  Returns FAILURE if not found.
 *
 * @param Name   [I] - Name of the GEvent to retrieve
 * @param List   [I] - The List collection to get the gevent from
 * @param GEvent [O] (optional) - The GEvent that was found
 */

int MGEventGetItem(

  const char     *Name,   /* I */
  mgevent_list_t *List,   /* I */
  mgevent_obj_t **GEvent) /* O (optional) */

  {
  mgevent_obj_t *TmpGEvent = NULL;

  if (GEvent != NULL)
    {
    *GEvent = NULL;
    }

  if ((Name == NULL) ||
      (List == NULL))
    {
    return(FAILURE);
    }

  if ((MUHTGet(&List->Hash,Name,(void **)&TmpGEvent,NULL) != SUCCESS) ||
      (TmpGEvent == NULL))
    {
    return(FAILURE);
    }

  if (GEvent != NULL)
    {
    *GEvent = TmpGEvent;
    }

  return(SUCCESS);
  }  /* END MGEventGetItem() */


/**
 * Gets the requested gevent from the hashtable.  If not found, it will
 *  add the gevent to the hash and return the new created one.
 *  Returns FAILURE if not found and couldn't add.
 *
 * @param Name   [I] - Name of the GEvent to retrieve
 * @param List   [I] - The List collection table to get the gevent from
 * @param GEvent [O] (optional) - The GEvent that was found
 */


int MGEventGetOrAddItem(

  const char     *Name,
  mgevent_list_t *List,
  mgevent_obj_t **GEvent)

  {
  if (GEvent != NULL)
    {
    *GEvent = NULL;
    }

  if ((Name == NULL) ||
      (List == NULL))
    {
    return(FAILURE);
    }

  if (MGEventGetItem(Name,List,GEvent) == SUCCESS)
    {
    return(SUCCESS);
    }

  /* Item was not found, add and return */

  if (MGEventAddItem(Name,List) == FAILURE)
    {
    return(FAILURE);
    }

  return(MGEventGetItem(Name,List,GEvent));
  }  /* END MGEventGetOrAddItem () */


/**
 * Free resources on the MGEvent Item List
 *
 * @param List   [I] - The List collection table to get the gevent from
 * @param FreeObjects
 * @param Fn
 */

int MGEventFreeItemList(

  mgevent_list_t     *List,
  mbool_t             FreeObjects,
  int                 (*Fn)(void **))

  {
  if (NULL == List)
    {
    return(FAILURE);
    }

  return(MUHTFree(&List->Hash,FreeObjects,Fn));
  } /* END MGEventFreeItemList() */


/*
 * Returns the effective Severity for the given GEvent.
 *  If GEvent is NULL, or Severity is not set, 0 is returned.
 *
 *  Checks for Severity on instance first, then on configured description.
 *
 * @param GEvent [I] - GEvent to get Severity for
 * @return   COUNT
 */

int MGEventGetSeverity(

  mgevent_obj_t *GEvent) /* I */

  {
  mgevent_desc_t *GEDesc;

  if (GEvent == NULL)
    {
    return(0);
    }

  if ((GEvent->Severity > 0) && (GEvent->Severity <= MMAX_GEVENTSEVERITY))
    {
    return(GEvent->Severity);
    }

  if (MGEventGetDescInfo(GEvent->Name,&GEDesc) == SUCCESS)
    {
    if ((GEDesc->Severity > 0) && (GEDesc->Severity <= MMAX_GEVENTSEVERITY))
      {
      return(GEDesc->Severity);
      }
    }

  return(0);
  } /* END MUGEventGetSeverity */



/**
 * Returns the gevent_list_t container for the given parent object.
 *
 * @param OType      [I] - Object type
 * @param OID        [I] - Object instance name
 * @param GEventList [O] - Return list
 */

int MGEventGetListFromObject(

  enum MXMLOTypeEnum         OType,
  const char                *OID,
  mgevent_list_t           **GEventList)

  {
  if ((GEventList == NULL) ||
      ((OID == NULL) && (OType != mxoSched)))
    {
    return(FAILURE);
    }

  *GEventList = NULL;

  switch (OType)
    {
    case mxoNode:

      {
      mnode_t *N = NULL;

      if (MNodeFind(OID,&N) == FAILURE)
        return(FAILURE);

      *GEventList = &N->GEventsList;
      }

      break;

    case mxoxVM:

      {
      mvm_t *VM = NULL;

      if (MVMFind(OID,&VM) == FAILURE)
        return(FAILURE);

      *GEventList = &VM->GEventsList;
      }

      break;

    case mxoSched:

      *GEventList = &MSched.GEventsList;

      break;

    default:

      /* NOT SUPPORTED */

      return(FAILURE);

      break;
    }  /* END switch(OType) */

  return(SUCCESS);
  }  /* END MGEventGetListFromObject() */
