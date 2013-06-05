/* HEADER */

/**
 * @file MHashTable.h
 *
 * declarations for HashTable functions
 *
 */

#ifndef __MHASHTABLE_H__
#define __MHASHTABLE_H__

#include "moab.h"

int MUHTAdd(mhash_t *,char const *,void *,mbitmap_t *,int (*)(void **));
int MUHTAddCI(mhash_t *,const char *,void *,mbitmap_t *,int (*)(void **));
int MUHTAddInt(mhash_t *,int,void *,mbitmap_t *,int (*)(void **));
int MUHTCreate(mhash_t *,int);
int MUHTGet(mhash_t *,char const *,void **,mbitmap_t *);
int MUHTGetCI(mhash_t *,char const *,void **,mbitmap_t *);
int MUHTGetInt(mhash_t *,int,void **,mbitmap_t *);
int MUHTFree(mhash_t *,mbool_t,int (*Fn)(void **));
int MUHTClear(mhash_t *,mbool_t,int (*Fn)(void **));
int MUHTMove(mhash_t *,mhash_t *);
int MUHTRemove(mhash_t *,const char *,int (*Fn)(void **));
int MUHTRemoveCI(mhash_t *,const char *,int (*Fn)(void **));
int MUHTRemoveInt(mhash_t *,int,int (*Fn)(void **));
int MUHTIterInit(mhashiter_t *);
int MUHTIterate(const mhash_t *,char **,void **,mbitmap_t *,mhashiter_t *);
int MUHTToMString(mhash_t *,mstring_t *); 
int MUHTToMStringDelim(mhash_t *,mstring_t *,const char *);
int MUHTIsInitialized(mhash_t *); 
int MUHTCopy(mhash_t *,const mhash_t *);
mbool_t MUHTIsEmpty(mhash_t *);

#endif /*  __MHASHTABLE_H__ */
