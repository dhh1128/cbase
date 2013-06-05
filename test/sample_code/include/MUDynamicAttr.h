/* HEADER */

/** @file MUDynamicAttr.h
 */

#ifndef __MUDYNAMICATTR_H__
#define __MUDYNAMICATTR_H__

extern int MUDynamicAttrGet(int,mdynamic_attr_t *,void **,enum MDataFormatEnum);
extern int MUDynamicAttrSet(int,mdynamic_attr_t **,void **,enum MDataFormatEnum);
extern int MUDynamicAttrFree(mdynamic_attr_t **);
extern int MUDynamicAttrCopy(mdynamic_attr_t *,mdynamic_attr_t **);

#endif /* __MUDYNAMICATTR_H__ */
/* END MUDynamicAttr.h */
