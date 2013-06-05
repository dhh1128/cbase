/* HEADER */

/**
 * @file mclient-proto.h
 *
 * Moab Client Prototypes
 */

#ifndef __MCLIENT_PROTO_H__
#define __MCLIENT_PROTO_H__


int Initialize(int,char **,int *);
int OMCShowUsage(enum MSvcEnum);
    
int MCSendRequest(msocket_t *,char *);
int MCShowCStats(mxml_t *,enum MXMLOTypeEnum);
int MCQueueShow(mxml_t *);
int MCShowReservation(void *,int);
int MCShowReservedNodes(mxml_t *);
int MCShowJobReservation(mxml_t *);
int MCDiagnose(void *,int);
int MCShowIdle(char *);
int MCShowRun(char *);
int MCShowCompleted(char *);
int MCShowJobHold(char *);
int MCShowStats(void *,int);
int MCShowSchedulerStatistics(char *);
int MCResetStats(void *,int);
int MCResCreate(char *);

int MCSetJobHold(char *); 
int MCReleaseJobHold(char *); 
int MCSetJobSystemPrio(char *); 
int MCSetJobUserPrio(char *); 
int MCStopScheduling(char *); 
int MCResumeScheduling(char *); 
int MCSetJobDeadline(char *); 
int MCReleaseJobDeadline(char *); 
int MCShowJobDeadline(char *); 
int MCShowEStart(mxml_t *); 
int MCSetJobQOS(char *); 
int MCShowGrid(char *); 
int MCClusterShowARes(void *,int); 
int MCShowConfig(void *,int); 
int MCShowUserStats(char *,int); 
int MCShowGroupStats(char *,int); 
int MCShowAccountStats(char *,int); 
int MCRunJob(void *,int); 
int MCShowNodeStats(mxml_t *); 
int MCShowSchedStats(mxml_t *);
int MCMigrateJob(char *); 
int MCShowEstimatedStartTime(void *,int); 
int MCUPSDedicatedComp(mgcred_t *,mgcred_t *);          
int MShowStatsComp(mshowstatobject_t *,mshowstatobject_t *);        
int MCAPSDedicatedComp(mgcred_t *,mgcred_t *);        


int MCSubmitProcessArgs(mxml_t *,int *,char **,char *,int,mbool_t,mbool_t,char *);
int MCCheckJobFlags(char *,char *);
int MCRestoreQuotes(char *,char *);
int MCProcessRMScriptDirective(char *,mxml_t *,enum MRMTypeEnum *,char *,int,mbool_t,char *);
int MCProcessRMScript(char *,mbool_t,mxml_t *,char *,int,mbool_t,char *);
int MCJobLoadDefaultEnv(mxml_t *);
int MCJobLoadRequiredEnv(mxml_t *);


#endif /* __MCLIENT_PROTO_H__ */
