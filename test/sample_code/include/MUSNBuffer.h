/* HEADER */

#ifndef __MUSNBUFFER_H__
#define __MUSNBUFFER_H__

/**
 * @file MUSNBuffer.h
 *
 * overrun-safe buffer functions
 *
 */

#include "moab.h"

extern int MUSNInit(char **,int *,char*,int);
extern int MUSNUpdate(char **,int *);
extern int MUSNXFlexiblePrint(char **,int *,char **,int *,mbool_t *,mbool_t,char const *,va_list);
extern int MUSNCat(char **,int *,char const *);

#ifndef __MTEST
extern int MUSNPrintF(char **,int *,const char *,...);
#endif 

#endif /* __MUSNBUFFER_H__ */

