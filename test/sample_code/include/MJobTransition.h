/* HEADER */

/**
 * @file MJobTransition.h
 *
 * Declares the APIs for job functions that deal with mtransjob_t structs.
 */

#ifndef __MJOBTRANSITION_H__
#define __MJOBTRANSITION_H__


int     MJobToTransitionStruct(mjob_t *,mtransjob_t *,mbool_t);
int     MJobTransitionAllocate(mtransjob_t **);
int     MJobTransitionFree(void **);
int     MJobTransitionCopy(mtransjob_t *,mtransjob_t *);
int     MJobTransitionAToString(mtransjob_t *,enum MJobAttrEnum,char *,int,enum MFormatModeEnum);
int     MJobTransition(mjob_t *,mbool_t,mbool_t);
int     MJobTransitionFitsConstraintList(mtransjob_t *,marray_t *);
mbool_t MJobTransitionJobIsBlocked(mtransjob_t *,mpar_t *);
mbool_t MJobTransitionMatchesConstraints(mtransjob_t *,marray_t *);
int     MJobTransitionProcessRMXString(mtransjob_t *,char *);

#endif /* __MJOBTRANSITION_H__ */
