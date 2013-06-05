/* HEADER */

/**
 * @file MURE.h
 *
 * Contains:
 *     MUREToList()
 *     MURangeToList()
 */

#ifndef __MURE_H__
#define __MURE_H__

int MURangeToList(char *,enum MXMLOTypeEnum,mpar_t *,marray_t *,char *,int);
int MUREToList(char *,enum MXMLOTypeEnum,mpar_t *,marray_t *,mbool_t,char *,int);

#endif  /* __MURE_H__ */
