/* HEADER */

/**
 * @file MJobPBS.h
 *
 * Declares the APIs for job functions that deal with pbs jobs.
 */

#ifndef __MJOBPBS_H__
#define __MJOBPBS_H__

int MJobPBSInsertArg(const char *,const char *,char *,char *,int);
int MJobPBSRemoveArg(const char *,const char *,char *,char *,int);
int MJobPBSExtractArg(const char *,const char *,char *,char *,int,mbool_t);
int MJobPBSExtractExtensionArg(const char *,char *,char *,int,mbool_t);
int MJobPBSProcessResources(mjob_t *,mreq_t *,char *,mnode_t **,int *,mbool_t *);
int MJobPBSInsertEnvVarsIntoRM(mjob_t *,mrm_t *);
int MJobRemovePBSDirective(char *,const char *,char *,char *,int);
int MJobGetPBSTaskList(mjob_t *,char *,int *,mbool_t,mbool_t,mbool_t,mbool_t,mrm_t *,char *);
int MJobGetPBSVTaskList(mjob_t *,char *,int *,mbool_t);
int MJobGetSelectPBSTaskList(mjob_t *,const char *,int *,mbool_t,mbool_t,char *);
int MPBSJobAdjustResources(mjob_t *,mtrma_t *,mrm_t *);
int MPBSParseMPPNodesJobAttr(mjob_t *J,const char *Value);
int MPBSSetMPPJobAttr(mjob_t *J,const char * Resource,char * Value,mreq_t  *Request);
long MPBSGetResKVal(char *);
int IsRMDirective(char *);
int MJobParsePBSExecHost(mjob_t *,mrm_t *,char *,int *,char *);
#endif /* __MJOBCLASS_H__ */
