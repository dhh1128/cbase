/* HEADER */

/**
 * @file MUDLList.h
 *
 * declarations for double link list functions
 *
 */

#ifndef __MUDLLIST_H__
#define __MUDLLIST_H__

#include "moab.h"

mdlnode_t *MUDLListFirst(mdllist_t *);
int MUDLListSize(mdllist_t *);
mdlnode_t *MUDLListLast(mdllist_t *);
void MUDLListFree(mdllist_t *);
void *MUDLListRemoveFirst(mdllist_t *);
int MUDLListAppend(mdllist_t *,void *);
mdllist_t *MUDLListCreate();
mdlnode_t *MUDLListNodeNext(mdlnode_t *);
void *MUDLListNodeData(mdlnode_t *);

#endif /*  __MUDLLIST_H__ */
