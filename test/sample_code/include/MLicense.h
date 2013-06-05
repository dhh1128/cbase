/* HEADER */

/**
 * @file MLicense.h
 *
 * declarations for APIs in MLicense.c
 *
 */

#ifndef __MLICENSE_H__
#define __MLICENSE_H__

void    MLicenseRecheck(void);
int     MLicenseCheckNodeCPULimit(mnode_t *,int);
int     MLicenseCheckVMUsage(char *);
int     MLicenseGetLicense(mbool_t,char *,int);
int     MLicenseGetExpirationDate(mulong *);
int     MLicenseGetExpirationDateInfo(char *,int);

#endif /*  __MLICENSE_H__ */
