/* HEADER */

#ifndef __MUI_H__
#define __MUI_H__
 
 /* ui object */
 
int MUIXMLSetStatus(mxml_t *,int,const char *,int);
int MUIInitialize(void);
int MUIJobSpecGetNL(char *,mgcred_t *,mpar_t *,mnl_t *);
int MUISProcessRequest(msocket_t *,char *,int *);
int MUIAcceptRequests(msocket_t *,long);
int MUIQueueShow(msocket_t *,mxml_t *,mxml_t **,mpar_t *,char *,char *,mbitmap_t *,mbitmap_t *);
int MUIRsvDiagnose(mbitmap_t *,char *,mrsv_t *,mstring_t *,mpar_t *,char *,enum MFormatModeEnum,mbitmap_t *,enum MRsvTypeEnum);
int MUIRsvDiagnoseXML(msocket_t *,mbitmap_t *,char *);
int MUINodeShow(char *,char *,mbitmap_t *,char *,long *);
int MUINodeShowStats(int,char *,char *,int);
int MUINodeStatsToXML(char *,long,long,char *,mxml_t **);
int MUIShowCStats(mbitmap_t *,mxml_t *,char *,enum MXMLOTypeEnum,mbitmap_t *,char *,mxml_t *,long,long,char *);
int MUIShowCConfig(char *,enum MXMLOTypeEnum,mxml_t *,mxml_t **,enum MLimitAttrEnum *);
int MUIConfigShow(char *,enum MCfgParamEnum,int,char *,int);
int MUIConfigPurge(char *,char *,int);
int MUIJobDiagnose(char *,mxml_t *,mstring_t *,mpar_t *,char *,mxml_t *,mbitmap_t *);
int MUIOClusterShowARes(const char *,char *,mbitmap_t *,char *,long *);
int MUIDiagnoseOld(char *,char *,int,char *,long *);
int MUIParDiagnose(char *,mxml_t **,mstring_t *,char *,mbitmap_t *,enum MFormatModeEnum);
int MUIPrioDiagnose(char *,mstring_t *,mpar_t *,mxml_t *,mbitmap_t *,enum MFormatModeEnum);
int MUINodeDiagnose(char *,mxml_t *,enum MXMLOTypeEnum,char *,mxml_t **,mstring_t *,mpar_t *,char *,char *,mbitmap_t *);
int MUILimitDiagnose(char *,mstring_t *,mbitmap_t *);
int MUIClusterShow(const char *,char *,mbitmap_t *,char *,long *);
int MUIShowState(msocket_t *,mbitmap_t *,char *);
int MUICredDiagnose(char *,mxml_t *,mstring_t *,enum MXMLOTypeEnum,char *,mbool_t);
int MUIStatShowMatrix(char *,char *,mbitmap_t *,char *,int);
int MUIParShowStats(char *,mpar_t *,char *,int);
int MUIRsvList(char *,char *,mpar_t *,mbitmap_t *,mxml_t *,char *);
int MUIShowRes(msocket_t *,mbitmap_t *,char *);
int MUIRsvCtlModify(mrsv_t *,mxml_t *,char *);
int MUIShowEstStartTime(const char *,char *,mbitmap_t *,char *,long *);
int MUICheckJob(mjob_t *,mstring_t *,enum MPolicyTypeEnum,mbitmap_t *,char *,char *,char *,mbitmap_t *);
int MUICheckAuthorization(mgcred_t *,mpsi_t *,void *,enum MXMLOTypeEnum,enum MSvcEnum,int,mbool_t *,char *,int);
int MUICheckRsvAuthorization(mgcred_t **,char *,mrsv_t *,enum MSvcEnum,int,mbool_t *,char *, int);
int MUIRsvCreate(mxml_t *,char *,char *,int,mbool_t,mbool_t,mbool_t,marray_t *);
int MUIVPCCreate(mxml_t *,char *,mbitmap_t *,char *);
int MUIVPCModify(char *,mxml_t *,char *,enum MXMLOTypeEnum,mstring_t *);
int MUIQRemoveJob(mjob_t *);
int MUITransactionAdd(mln_t **,msocket_t *,void *,void *,int (*)(msocket_t *,void *,void *,void *,void *));
int MUITransactionRemove(mln_t **,mln_t *);
int MUIProcessTransactions(mbool_t,char *);
int MUITransactionFindType(int (*)(msocket_t *,void *,void *,void *,void *),mrm_t *,msocket_t **);
int MUIRMCtlModifyDynamicResources(char *,mrm_t *,mxml_t *,char *,char *,char *);
int MUISchedCtlRecvFile(char *,mxml_t *,char *);
int MUInsertGenericResource(char *,msnl_t *,msnl_t *,msnl_t *,int,int,int); 
/*int MUIVMCreate(msocket_t *,char *,mnode_t *,char *,char *);*/
int MUIVMDestroy(mvm_t *,mnode_t *,char *,mbitmap_t *,char *,char *,msysjob_t**);
int MUIJTXToXML(mjob_t *,mxml_t *);
int MUIGEventCreate(mgcred_t *,mbool_t,mxml_t *,char *);

int MDispatchMCRequestFunction(long,msocket_t *,mbitmap_t *,name_t);
int MCheckMCRequestFunction(long) ;
int MUIOShowAccess(void *,enum MXMLOTypeEnum,char *,mbool_t,int,mxtree_t *,mxml_t *);
 
int MUIPeerJobQuery(mrm_t *,char *,mxml_t *,mbool_t,char *);
int MUIPeerJobQueryThreadSafe(mrm_t *,char *,mxml_t *);
int MUIPeerNodeQueryThreadSafe(mrm_t *,mxml_t *);
int MUIPeerSchedQuery(mrm_t *,mxml_t *,mxml_t *,char *,mbitmap_t *);
int MUIPeerSetup(msocket_t *,char *,mbool_t *,mrm_t **);
int MUIPeerQueryLocalResAvail(msocket_t *,mxml_t *,char *,mxml_t *);
mbool_t MUIPeerShowOnlyGridJobs(mrm_t *);

int MUIGenerateSchedStatusXML(mxml_t **,mpsi_t *,char *);

int MUISchedCtlQueryEvents(mxml_t *,mxml_t *);
int MUISchedCtlQueryPendingActions(mxml_t *,mxml_t *);
int MUISchedCtlSendFile(char *,int,char *,mxml_t *,char *);
int MUISchedCtlProcessEvent(mrm_t *,char *,enum MPeerEventTypeEnum,char *);
int MUISchedCtlModify(char *,mxml_t *,char *,enum MXMLOTypeEnum,char *);
int MUISchedCtlCreate(char *,mxml_t *,char *,enum MXMLOTypeEnum,char *);
int MUISchedConfigModify(char *,mxml_t *,char *,enum MXMLOTypeEnum,mstring_t *);
int MUISchedCtlListTID(msocket_t *,mgcred_t *,char *,enum MFormatModeEnum,mxml_t *,char *);


/* Blocking MUI Entry point functions */
int MUIBal(msocket_t *,mbitmap_t *,char *);
int MUIGridCtl(msocket_t *,mbitmap_t *,char *);
int MUIJob(msocket_t *,mbitmap_t *,char *);
int MUINodeCtl(msocket_t *,mbitmap_t *,char *);
int MUIRsvCtl(msocket_t *,mbitmap_t *,char *);
int MUIShow(msocket_t *,mbitmap_t *,char *);
int MUICredCtl(msocket_t *,mbitmap_t *,char *);
int MUIRMCtl(msocket_t *,mbitmap_t *,char *);
int MUISchedCtl(msocket_t *,mbitmap_t *,char *);
int MUIShowStats(msocket_t *,mbitmap_t *,char *);
int MUIDiagnose(msocket_t *,mbitmap_t *,char *);
int MUIVCCtl(msocket_t *,mbitmap_t *,char *);
int MUIVMCtl(msocket_t *,mbitmap_t *,char *);
int MUIVMCtlPlanMigrations(enum MVMMigrationPolicyEnum,mbool_t,mbool_t,msocket_t *);
int MUICheckJobWrapper(msocket_t *,mbitmap_t *,char *);
int MUICheckNode(msocket_t *,mbitmap_t *,char *);

/* Thread Safe MUI ENtry point functions */
int MUIShowQueueThreadSafe(msocket_t *,mbitmap_t *,char *);
int MUIJobCtlThreadSafe(msocket_t *,mbitmap_t *,char *);
int MUINodeCtlThreadSafe(msocket_t *,mbitmap_t *, char *);
int MUIRsvDiagnoseThreadSafe(msocket_t *,mbitmap_t *,char *);
int MUITrigDiagnoseThreadSafe(msocket_t *,mbitmap_t *,char *);
int MUIVCCtlThreadSafe(msocket_t *,mbitmap_t *,char *);
int MUIVMCtlThreadSafe(msocket_t *,mbitmap_t *,char *);
int MUIPeerResourcesQueryThreadSafe(msocket_t *,mbitmap_t *,char *);
int MUIEventQueryThreadSafe(msocket_t *,mbitmap_t *,char *);
int MUIJobSubmitThreadSafe(msocket_t *,mbitmap_t *,char *);

int MUIJobCtl(msocket_t *,mbitmap_t *,char *);

/* MUIJob helper functions */
int MUIJobQuery(char *,char *,mjobconstraint_t *,mpar_t *,msocket_t *,char *,mxml_t *);
int MUIJobModify(mjob_t *,char *,mbitmap_t *,msocket_t *,mxml_t *);

int MUIExportRsvQuery(mrsv_t *,mrm_t *,char *,mxml_t *,mulong);

int MUIMiscShow(msocket_t *,mxml_t *,mxml_t **,char *);
int MUIRsvCompShow(msocket_t *,mxml_t *,mxml_t **,mrsv_t *);

/* Misc helper functions */
int MUIJobQueryTemplates(mxml_t *,mxml_t *);
int MUIGetShowqNotes(mbool_t,mxml_t *);

int __MUIRsvProfShow(msocket_t *,mgcred_t *,char *,mxml_t **,char *);
int __MUIJobSetAttr(mjob_t *,enum MJobAttrEnum,char *,char *,mbitmap_t *,char *,char *,char *,char *);

int __MUIRsvCtlModify(mrsv_t *,mxml_t *,char *);
int __MUIJobGetEStartTime(char *,char *,msocket_t *,char *,char *,mclass_t *,mxml_t **);

int __MTransJobNameCompLToH(mtransjob_t **,mtransjob_t **);
int __MUIJobQueryWorkflowRecursive(mjob_t *,long,mjworkflow_t *,mln_t const *,int,mxml_t *);
int __MUIJobTransitionToXML(mtransjob_t *,mbitmap_t *,mxml_t **);
int __MUIJobMigrate(mjob_t *,char *,msocket_t *,mxml_t *);
int __MUIJobToXML(mjob_t *,mbitmap_t *,mxml_t **);
int __MUIJobRerun(char *,char *,msocket_t *,mxml_t *);
int __MUIJobQueryRsvOrphans(msocket_t *,char *);
int __MUIJobPreempt(mjob_t *,char *,mgcred_t *,enum MPreemptPolicyEnum,mbool_t,char *,char *);
int __MUIJobResume(mjob_t *,char *,char *);

int MCheckMCRequestFunction(long);

int MUIVCCanSetFlags(char *,mstring_t *);

#define ISMASTERSLAVEREQUEST(PeerRM,SchedState) \
  ((((PeerRM) != NULL) && ((SchedState) == msmSlave)) ? TRUE : FALSE)

#endif  /* __MUI_H__ */
