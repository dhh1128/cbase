/* HEADER */

/**
 * @file MUGetOps.c
 *
 * Contains various MUGetXXX functions to extract values from text
 *
 */

#ifndef __MUGETOPS_H__
#define __MUGETOPS_H__

int MUGetIndex(char *,const char **,mbool_t,int);
int MUGetIndex2(char *,const char **,int);
int MUGetIndexCI(char const *,const char **,mbool_t,int);
int MUGetPair(char *,const char **,int *,char *,mbool_t,int *,char *,int);
int MUGetPairCI(char *,const char **,int *,char *,mbool_t,int *,char *,int);
int MUGetWord(char *,char **,char **);

#endif /* __MUGETOPS_H__ */
