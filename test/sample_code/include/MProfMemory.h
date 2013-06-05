/* HEADER */

/**
 * @file MProfMemory.h
 */

#ifndef __MPROFMEMORY_H__
#define __MPROFMEMORY_H__

extern void *MUProfCalloc(char *,int,int,int);
extern void *MUProfMalloc(char *,int,int);
extern void *MUProfRealloc(char *,int,void *,int);
extern int MUProfFree(char *,int,char **);
extern int MUProfFreeTrad(char **);
extern int MUProfStrDup(char *,int,char **,char *);
extern char *MUProfStrDupTrad(char *,int,char *);

#endif /* __MPROFMEMORY_H__ */
/* END MProfMemory.c */
