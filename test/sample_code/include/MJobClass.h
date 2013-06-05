/* HEADER */

/**
 * @file MJobClass.h
 *
 * Declares the APIs for job functions that deal with classes.
 */

#ifndef __MJOBCLASS_H__
#define __MJOBCLASS_H__

int MJobGetClass(mjob_t *);
int MJobApplyClassDefaults(mjob_t *);
int MJobSelectClass(mjob_t *,mbool_t,mbool_t,mclass_t **,char *);
int MJobSetClass(mjob_t *,mclass_t *,mbool_t,char *);
int MJobCheckClassJLimits(mjob_t *,mclass_t *,mbool_t,int,char *,int);
int __ApplyDefaultTaskCounts(mjob_t *,mjob_t *,mjob_t *);

#endif /* __MJOBCLASS_H__ */
