/* HEADER */

/**
 * @file MPreempt.h
 *
 * Declares the APIs for job functions that deal with preemption.
 */

#ifndef __MPREMPT_H__
#define __MPREMPT_H__

int     MJobPreempt(mjob_t *,mjob_t *,mjob_t **,mnl_t *,const char *,enum MPreemptPolicyEnum,mbool_t,char *,char *,int *);
int     MJobSelectPreemptees(mjob_t *,mreq_t *,int,int,mjob_t **,const mnl_t *,mnl_t **,mjob_t **,int *,int *,mnl_t **);
int     MJobGetNodePreemptList(mjob_t *,mnode_t *,char **);
int     MJobSelectBGPJobList(mjob_t *,mjob_t **,mnl_t *,mjob_t **,int *,int *,mnl_t **);
int     MJobSelectBGPList(mjob_t *J,mnl_t *,mnl_t *,mjob_t **);
int     MPreemptorSelectPreemptees(mjob_t *,mreq_t *,int,int,mjob_t **,const mnl_t *,mnl_t **,mjob_t **,int *,int *,mnl_t **);
int     MPreemptorSelectPreempteesOld(mjob_t *,mreq_t *,int,int,mjob_t **,const mnl_t *,mnl_t **,mjob_t **,int *,int *,mnl_t **);
mbool_t MJobCanPreempt(mjob_t *,const mjob_t *,const long,const mpar_t *,char *);
mbool_t MJobCanPreemptIgnIdleJobRsv(mjob_t *,mjob_t *,long,char *);
mbool_t MJobIsPreempting(mjob_t *);
mbool_t MJobReachedMinPreemptTime(const mjob_t *,char *);
mbool_t MJobReachedMaxPreemptTime(const mjob_t *,char *);
mulong  MJobGetMinPreemptTime(const mjob_t *);
mulong  MJobGetMaxPreemptTime(const mjob_t *);

#endif /* __MPREMPT_H__ */
