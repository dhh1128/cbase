/* HEADER */

#ifndef __MULINKEDLIST_H__
#define __MULINKEDLIST_H__

/**
 * @file MULinkedList.h
 *
 * moab linked list interface
 *
 */

#include "moab.h"

int MULLCreate(mln_t **);
int MULLGetCount(mln_t *);
int MULLPrepend(mln_t **,char const *,void *);
int MULLAdd(mln_t **,char const *,void *,mln_t **,int (*)(void **));
int MULLDup(mln_t **,mln_t  *);
int MULLRemove(mln_t **,const char  *,int (*)(void **));
int MULLUpdateFromString(mln_t **,char *,const char   *);
int MULLToString(mln_t   *,mbool_t,const char *,char *,int);
int MULLToMString(mln_t  *,mbool_t,const char *,mstring_t *);
int MULLFree(mln_t  **,int (*)(void **));
int MULLCheck(mln_t  *,char const *,mln_t **);
int MULLCheckP(mln_t  *,void *,mln_t **);
int MULLCheckCS(mln_t *,char const *,mln_t **);
int MULLIterate(mln_t  *,mln_t **);
int MULLIteratePayload(mln_t  *,mln_t **,void **);
int MUAddVarLL( mln_t **,const char *,char *);

#endif /* __MULINKEDLIST_H__ */
/* END MULinkedList.h */
