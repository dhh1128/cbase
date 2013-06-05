/* HEADER */

/**
 * @file MMemory.h
 *
 * Contains Moab Memory APIs Interface
 */

#ifndef __MMEMORY_H__
#define __MMEMORY_H__


memtracker_t *MUMemoryInitialize(void);
void MUMemoryTeardown(memtracker_t *);
int MUMemTrap(void);
int MUMemReport(mbool_t,char *,int);

#ifdef MPROFMEM

#define MUFree(x) MUProfFree(__FILE__,__LINE__,(x))
/* #define free(x) MUProfFree(__FILE__,__LINE__,(x)) */
#define MUMalloc(x) MUProfMalloc(__FILE__,__LINE__,(x))
#define malloc(x) MUProfMalloc(__FILE__,__LINE__,(x))
#define MUCalloc(x,y) MUProfCalloc(__FILE__,__LINE__,(x),(y))
#define calloc(x,y) MUProfCalloc(__FILE__,__LINE__,(x),(y))
#define realloc(x,y)  MUProfRealloc(__FILE__,__LINE__,(x),(y))
#define MUStrDup(x,y) MUProfStrDup(__FILE__,__LINE__,(x),(y))
#define MUStrNDup(x,y,z) MUProfStrNDup(__FILE__,__LINE__,(x),(y),(z)) /*NYI*/
#define strdup(x) MUProfStrDupTrad(__FILE__,__LINE__,(x))

#else

void __MUMemAddToTrackingTable(void *,int,const char *);
void __MUMemRemoveFromTrackingTable(char **);
int MUFree(char **);
void *MUCalloc(int,int);
void *MUMalloc(int);
int MUStrNDup(char **,char *,int );
int MUStrDup(char **,char const *);

#endif /* MPROFMEM */
#endif /* __MMEMORY_H__ */
