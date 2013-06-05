/* HEADER */

/**
 * @file MClass.h 
 *
 * contains: mclass_t related function declarations
 */

#ifndef  __MCLASS_H__
#define  __MCLASS_H__

#include "moab.h"

int MClassSetAttr(mclass_t *,mpar_t *,enum MClassAttrEnum,void **,enum MDataFormatEnum,enum MObjectSetModeEnum Mode);
int MClassAToMString(mclass_t *,enum MClassAttrEnum,mstring_t *);
int MClassProcessConfig(mclass_t *,char *,mbool_t);
int MClassConfigLShow(mclass_t *,int,int,mstring_t *);
int MClassAdd(const char *,mclass_t **);
int MClassFind(const char *,mclass_t **);
int MClassConfigShow(mclass_t *,int,mstring_t *);
int MClassShow(char *,char *,mxml_t *,mstring_t *,mbitmap_t *,mbool_t,enum MFormatModeEnum);
int MClassCheckAccess(mclass_t *,mgcred_t *);
int MClassUpdateHostListRE(mclass_t *,char *);
int MClassGetEffectiveRMLimit(mclass_t *,int,enum MCredAttrEnum,unsigned long *);
int MClassListFromString(mbitmap_t *,char *,mrm_t *);
int MClassListToMString(mbitmap_t *,mstring_t *,mrm_t *); 

#endif /*  __MCLASS_H__ */
