/* HEADER */

#ifndef __MWIKII_H__
#define __MWIKII_H__



#define MDEF_SLURMVERSION       10200
#define MDEF_SLURMVERSIONSTRING "1.2.0"

/* SLURM event defines */

#define MDEF_SLURMEVENT_JOB_STATE_CHANGE       1234
#define MDEF_SLURMEVENT_PARTITION_STATE_CHANGE 1235

enum MWikiSlurmEventFlagEnum {
  mwsefNONE = 0,
  mwsefJob,             /* SLURM Job state change event */
  mwsefPartition } ;    /* SLURM Partition state change event */

/* Global flags set when a SLURM event is processed.
 * A flag is set to indicate that configuration information may require a reload.
 * The flags are checked, processed, and cleared each scheduler iteration.
 */
extern mbitmap_t  MWikiSlurmEventFlags[MMAX_RM]; /* Slurm Event flags (bitmap of enum MWikiSlurmEventFlagEnum) */
extern mbool_t    MWikiHasComment;


/* Structure local to Wiki object/interface */

typedef struct {
  char *ClusterInfoBuffer;
  char *WorkloadInfoBuffer;
  } mwikirm_t;


/* wiki interface object */

int MWikiInitialize(mrm_t *,char *,enum MStatusCodeEnum *);

int MWikiDoCommand(mrm_t *,enum MRMFuncEnum,mpsi_t *,mbool_t,enum MRMAuthEnum,char *,char **,long *,mbool_t,char *,int *);
int MWikiSDUpdateAttr(char *,msdata_t *,mrm_t *,char *);
int MWikiParseTimeLimit(char *);

int MWikiClusterQuery(mrm_t *,int *,char *,enum MStatusCodeEnum *);
int MWikiJobStart(mjob_t *,mrm_t *,char *,enum MStatusCodeEnum *);
int MWikiJobCancel(mjob_t *,mrm_t *,char *,char *,enum MStatusCodeEnum *);
int MWikiJobSignal(mjob_t *,mrm_t *,int,char *,char *,int *);
int MWikiJobSuspend(mjob_t *,mrm_t *,char *,int *);
int MWikiJobModify(mjob_t *,mrm_t *,const char *,const char *,const char *,enum MObjectSetModeEnum,char *,enum MStatusCodeEnum *);
int MWikiJobResume(mjob_t *,mrm_t *,char *,int *); 
int MWikiJobRequeue(mjob_t *,mrm_t *,mjob_t **,char *,int *);
int MWikiWorkloadQuery(mrm_t *,int *,int *,char *,enum MStatusCodeEnum *);
int MWikiNodeUpdateAttr(char *,int,mnode_t *,mrm_t *,enum MWNodeAttrEnum *,char *);
int MWikiAttrToTaskList(mjob_t *,int *,char *,int *);
int MWikiProcessEvent(mrm_t *,int *);
int MWikiGetAttrIndex(char *,char **,int *,char *);
int MWikiSystemQuery(mrm_t *,char *,char *,mbool_t,char *,char *,enum MStatusCodeEnum *);

mulong MWillRunAdjustStartTime(mulong,mulong);
int MWillRunParsePreemptees(char *,mjob_t **);

/* SLURM apis */
int MSLURMGetVersion(mrm_t *);
int MSLURMInitialize(mrm_t *);
int MSLURMJobWillRun(mjwrs_t *,mrm_t *,char **,char *,enum MStatusCodeEnum *);

#endif /*  __MWIKII_H__ */
