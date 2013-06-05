/* HEADER */

/**
 * @file MTree.h
 *
 * Tree management functions 
 *
 */

#ifndef __MTREE_H__
#define __MTREE_H__

#include "moab.h"

extern int MSTreeFind(mtree_t **,char *,int,mtree_t *,int,int *,mbool_t,mbool_t *);
extern int MSTreeToString(mtree_t *,int,char *,int);

#endif /*  __MTREE_H__ */
