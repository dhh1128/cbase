/* HEADER */

/**
 * @file MJobArray.h
 *
 * Declares the APIs for MJobArray objects/operations
 *
 */

#ifndef __MJOBARRAY_H__
#define __MJOBARRAY_H__

extern int MJobArrayAlloc(mjob_t *);
extern int MJobArrayAddStep(mjob_t *,mjob_t *,char *,int,int *);
extern int MJobArrayUpdate(mjob_t *);
extern int MJobLoadArray(mjob_t *,char *);
extern int MJobArrayAllocMNL(mjob_t *,mnl_t **,char *,mnl_t **,enum MNodeAllocPolicyEnum,long,int *,char *);
extern void MJobArrayUpdateJobIndicies(mjob_t *);
extern void MJobUpdateJobArrayIndices();
extern int MJobArrayFree(mjob_t *);
extern int MJobArrayUpdateJobInfo(mjob_t *,enum MJobStateEnum);
extern int MJobArrayInsertName(mjarray_t *,char *,int);
extern int MJobArrayCancelIfMeetsPolicy(mjob_t *,mjob_t *,int);
extern int MJobArrayLockToPartition(mjob_t *,mpar_t *);
extern int MJobArrayGetArrayCount(mjarray_t *,int *);
extern int MJobTransitionGetArrayProcCount(mtransjob_t *,marray_t *,int *);
extern int MJobTransitionGetArrayJobCount(mtransjob_t *,marray_t *,int *);
extern int MJobTransitionGetArrayRunningStartTime(mtransjob_t *,marray_t *,mulong *);
extern int MJobGetArrayMasterTransitionID(mjob_t *);
extern int MJobTransitionSetXMLArrayNameCount(mtransjob_t *,mxml_t *,int);
extern mbool_t MJobArrayIsFinished(mjob_t *);
extern mbool_t MJobIsArrayMaster(mjob_t *);

extern int MJobArrayToXML(mjob_t *,mxml_t **); 
extern int MJobArrayFromXML(mjob_t *,mxml_t *);
extern int MJobTransitionArraySubjobsToXML(mtransjob_t *,marray_t *,mxml_t **);

#endif /* __MJOBARRAY_H__ */
