/* HEADER */

/**
 * @file moab-proto.h
 *
 * Moab Prototypes
 *
 * @see mcom-proto.h
 */

#ifndef __MOAB_PROTO_H__
#define __MOAB_PROTO_H__

#ifdef __MINSURE
int _Insure_mem_info(void *pmem);
int _Insure_ptr_info(void **pptr);
long _Insure_list_allocated_memory(int mode);
#endif /* __MINSURE */

#include "moab.h"
#include "mcom-proto.h"

/* XML object declarations */

#include "MXML.h"



struct mdb_t;
struct mstmt_odbc_t;

/* int main(int,char **,char **); */


/* CP object */

int MCPCreate(char *);
int MCPStoreCluster(mckpt_t *,mnode_t **); 
int MCPStoreRMList(mckpt_t *); 
int MCPStoreQueue(mckpt_t *,mbool_t);
int MCPStoreTJobs(mckpt_t *);
int MCPLoadSched(mckpt_t *,const char *,msched_t *);     
int MCPStoreRsvList(mckpt_t *);
mbool_t MCPIsSupported(mckpt_t *,char *);
int MCPStoreSRList(mckpt_t *);
int MCPStoreTransactions(mckpt_t *);
int MCPStoreVPCList(mckpt_t *,marray_t *);
int MCPLoadSys(mckpt_t *,const char *,msched_t *);
int MCPLoadSysStats(char *);
int MCPLoadStats(char *);
int MCPWriteGridStats(mckpt_t *);
int MCPLoadSR(char *);
int MCPLoadPar(char *);
int MCPStoreCredList(mckpt_t *,char *,enum MXMLOTypeEnum);
int MCPStoreObj(FILE *,enum MCkptTypeEnum,char *,const char *);
int MCPLoad(char *,enum MCKPtModeEnum);
int MCPWriteScheduler(FILE *);
int MCPWriteJobs(FILE *,char *);
int MCPWriteStandingReservations(FILE *,char *);
int MNodeToString(mnode_t *,mstring_t *);
int MNodeToWikiString(mnode_t *,char *,mstring_t *);
int MNodeToXML(mnode_t *,mxml_t *,enum MNodeAttrEnum *,mbitmap_t *,mbool_t,mbool_t,mrm_t *,char *);
int MNodeProfileToXML(char *,mnode_t *,char *,enum MNodeAttrEnum *,long,long,long,mxml_t *,mbool_t,mbool_t,mbool_t);
int MNodeFreeOSList(mnodea_t **,int const);
int __MNodeCompareLex(const void *, const void *);
int __MNodeCompareIndex(const void *, const void *);
int MCPWriteSystemStats(mckpt_t *);
int MCPRestore(enum MCkptTypeEnum,const char *,void *,mbool_t *);
int MCPWriteTList(mckpt_t *);
int MCPStoreCJobs(mckpt_t *);
int MCPValidate(char *,mbool_t *,mbool_t *);
int MCPStoreVMs(mckpt_t *,mhash_t *,mbool_t);
mbool_t MCPIsCorrupt();
int MCJobLoadCP(char *);
int MCPLoadAllSpoolCP();
int MCPGetCPFileName(const mjob_t *,char *,int);
int MCPJobCPBufInit();
mstring_t *MCPGetJobCPBuf();
int MCPPopulateJobCPFile(const mjob_t *,const char *,mbool_t);
int MCPStoreVCs(mckpt_t *);
int MCPLoadVC(char *);
int MCPSecondPass();


/* checkpoint journal routines */

int MCPJournalAdd(enum MXMLOTypeEnum,char *,char *,char *,enum MObjectSetModeEnum);
int MCPJournalOpen(char *);
int MCPJournalClose();
int MCPJournalClear();
int MCPJournalLoad();
int MCPSubmitJournalLoad();
int MCPSubmitJournalOpen(char *);
int MCPSubmitJournalAddEntry(mstring_t *,mjob_submit_t *);
int MCPSubmitJournalRemoveEntry(mulong);
int MCPSubmitJournalClear();
int MCPSubmitJournalClose();


/* user object */

int MUserLoadCP(mgcred_t *,char *);
int MUserToXML(mgcred_t *,mxml_t **,int *);
int MUserShow(mgcred_t *,mstring_t *,mbool_t,char *);
int MUserInitialize(mgcred_t *,char *);
int MUserFind(const char *,mgcred_t **);
int MUserAdd(const char *,mgcred_t **);
int MUserCopy(mgcred_t *,mgcred_t *);
int MUserCreate(const char *,mgcred_t **);
int MCredToString(void *,enum MXMLOTypeEnum,mbool_t,mstring_t *);
int MCredFreeTable(mhash_t*);
int MUserProcessConfig(mgcred_t *,char *,mbool_t);
int MUserGetCreds(mgcred_t *,mgcred_t **,mgcred_t **,mgcred_t **,mqos_t **);
int MUserGetUID(mgcred_t *,int *,int *);
mbool_t MUserIsOwner(mgcred_t *,mgcred_t *,mgcred_t *,mclass_t *,void *,mbitmap_t *,enum MXMLOTypeEnum,mfst_t *);
int MCredCreate(enum MXMLOTypeEnum,char *,char *,double,char *);
mbool_t MCredAttrSetByUser(void *,enum MXMLOTypeEnum,enum MCredAttrEnum); 
mbool_t MUserHasPrivilege(mgcred_t *,enum MXMLOTypeEnum,enum MObjectActionEnum);
int MUserAToString(mgcred_t *,enum MUserAttrEnum,mstring_t *,int);
int MPrivilegeParse(mln_t **,const char *,char *);




/* group object */

int MGroupShow(mgcred_t *,mstring_t *,mbool_t,char *);
int MGroupLoadCP(mgcred_t *,const char *);    
int MGroupFind(const char *,mgcred_t **);
int MGroupAdd(const char *,mgcred_t **);
int MGroupProcessConfig(mgcred_t *,char *,mbool_t);

/* acct object */

int MAcctShow(mgcred_t *,mstring_t *,mbool_t,char *);
int MAcctLoadCP(mgcred_t *,const char *);
int MAcctToXML(mgcred_t *,mxml_t **,enum MCredAttrEnum *);  
int MAcctFind(const char *,mgcred_t **);
int MAcctAdd(const char *,mgcred_t **);
int MAcctProcessConfig(mgcred_t *,char *,mbool_t);



/* cred object */

int MCredAToMString(void *,enum MXMLOTypeEnum,enum MCredAttrEnum,mstring_t *,enum MFormatModeEnum,mbitmap_t *);    
int MCredPerLimitsToMString(void *,enum MXMLOTypeEnum,enum MCredAttrEnum,mstring_t *,enum MFormatModeEnum,mulong);    
int MCredSetAttr(void *,enum MXMLOTypeEnum,enum MCredAttrEnum,mpar_t *,void **,char *,enum MDataFormatEnum,enum MObjectSetModeEnum);
char *MCredShowLimit(mpu_t *,long,long,mbool_t);
int MOSetMessage(void *,enum MXMLOTypeEnum,void **,enum MDataFormatEnum,enum MObjectSetModeEnum);
int MOFromXML(void *,enum MXMLOTypeEnum,mxml_t *,mpar_t *);   
int MOFromString(void *,enum MXMLOTypeEnum,const char *);
int MOToXML(void *,enum MXMLOTypeEnum,void *,mbool_t,mxml_t **);
int MOGetComponent(void *,enum MXMLOTypeEnum,void **,enum MXMLOTypeEnum);
int MOLoadCP(void *,enum MXMLOTypeEnum,const char *);
mbool_t MTrigInTList(marray_t *,mtrig_t *);
int MOAddTrigPtr(marray_t **,mtrig_t *);
int MCOToXML(void *,enum MXMLOTypeEnum,mxml_t **,void *,enum MXMLOTypeEnum *,void **,mbool_t,mbitmap_t *);
int MOReportEvent(void *,char *,enum MXMLOTypeEnum,enum MTrigTypeEnum,mulong,mbool_t);
int MODeleteTPtr(mtrig_t *);
int MOCheckLimits(void *,enum MXMLOTypeEnum,enum MPolicyTypeEnum,mpar_t *,mbitmap_t *,char *);
int MCredConfigShow(void *,enum MXMLOTypeEnum,enum MCredAttrEnum *,int,int,mstring_t *);
int MCredConfigAShow(void *,enum MXMLOTypeEnum,enum MCredAttrEnum *,int,int,mstring_t *);
int MCredAdd(int,char *,void **);
int MCredLoadConfig(enum MXMLOTypeEnum,char *,mpar_t *,char *,mbool_t,char *);
int MCredListToAString(char *,char *,char *,char *,char *,char *,char *,int);
int MCredAdjustConfig(enum MXMLOTypeEnum,void *,mbool_t);
int MCredSetDefaults(void);
int MCredProcessConfig(void *,enum MXMLOTypeEnum,mpar_t *,const char *,mcredl_t *,mfs_t *,mbool_t,char *);
char *MCredShowAttrs(mcredl_t *,mpu_t *,mpu_t *,mpu_t *,mpu_t *,mpu_t *,mpu_t *,mpu_t *,mfs_t *,long,mbool_t,mbool_t,char *);
int MCredShow2DLimit(mcredl_t *,enum MLimitTypeEnum,enum MActivePolicyTypeEnum,enum MCredAttrEnum,enum MXMLOTypeEnum,mbool_t,char *,int);
int MCredCheckAcctAccess(mgcred_t *,mgcred_t *,mgcred_t *);
int MCredCheckAccess(mgcred_t *,char *,enum MAttrEnum,macl_t *);
int MCredCheckGroupAccess(mgcred_t *,mgcred_t *);
int MCredShowChildren(enum MXMLOTypeEnum,char *,long,enum MFormatModeEnum,void **,int,mbool_t);
int MCredShowStats(enum MXMLOTypeEnum,char *,enum MStatAttrEnum *,mbool_t,enum MFormatModeEnum,void **,int,mbool_t);
mbool_t MCredIsMatch(const mcred_t *,void *,enum MXMLOTypeEnum);
int MCredDestroy(mgcred_t **);
int MCredPurge(enum MXMLOTypeEnum,void *,char *);
int MCredSyncDefaultConfig(void);
int MCredParseName(char *,enum MXMLOTypeEnum *,void **);
int MCredProfileToXML(enum MXMLOTypeEnum,char *,const char *,mulong,mulong,long,mbitmap_t *,mbool_t,mbool_t,mxml_t *);
int MCredDoDiscovery(mxml_t *,enum MXMLOTypeEnum); 
int __MCredParseLimit(char *,long *,long *); 
int MCredFindManager(char *,mgcred_t **,mgcred_t **);
mbool_t MCredGroupIsVariable(const mjob_t *);
int MCredCheckTotals(const mjob_t *,char *);
void MCredGetCredTypeObject(const mcred_t *,enum MXMLOTypeEnum,void **,void **);


void *MOGetNextObject(void **,enum MXMLOTypeEnum,void *,mhashiter_t *,char **);
char *MOGetName(void *,enum MXMLOTypeEnum,char **);
int MOLoadPvtConfig(void **,enum MXMLOTypeEnum,char *,mpsi_t **,char *,char *);
int MOGetObject(enum MXMLOTypeEnum,const char *,void **,enum MObjectSetModeEnum);
int MOMap(momap_t *,enum MXMLOTypeEnum,char *,mbool_t,mbool_t,char *);
int MOMapList(momap_t *,enum MXMLOTypeEnum,char *,mbool_t,mbool_t,mstring_t *);
int MOMapClassList(momap_t *,char *,mbool_t,mbool_t,char *,int);
int MOMapSplitNames(char *,char *,char *);



/* rsv object */

int MRsvAllocate(mrsv_t *,mnl_t *);
int MRsvAllocateGResType(mrsv_t *,char *,char *);
int MRsvDeallocateResources(mrsv_t *,mnl_t *);
int MResShowHostList(mrsv_t *);
int MRsvLoadCP(mrsv_t *,char *);
int MRsvAdjust(mrsv_t *,long,int,mbool_t);
int MRsvFromXML(mrsv_t *,mxml_t *,char *);
int MRsvSetAttr(mrsv_t *,enum MRsvAttrEnum,void *,enum MDataFormatEnum,enum MObjectSetModeEnum);
int MRsvGroupSetAttr(char *,enum MRsvAttrEnum,void *,enum MDataFormatEnum,enum MObjectSetModeEnum);
int MRsvGroupSendSignal(char *,enum MTrigTypeEnum);
int MRsvToXML(mrsv_t *,mxml_t **,enum MRsvAttrEnum *,int *,mbool_t,enum MCModeEnum);
int MRsvToJob(mrsv_t *,mjob_t *);
int MRsvToVPC(mrsv_t *,mpar_t *);
int MRsvPreempt( mrsv_t *);
int MRsvCreate(enum MRsvTypeEnum,macl_t *,macl_t *,mbitmap_t *,mnl_t *,long,long,int,int,char *,mrsv_t **,char *,mcres_t *,char *,mbool_t,mbool_t);
int MRsvAdjustTime(long);
int MRsvAdjustTimeframe(mrsv_t *);
int MRsvSetTimeframe(mrsv_t *,enum MRsvAttrEnum,enum MObjectSetModeEnum,long,char *);
int MRsvDestroy(mrsv_t **,mbool_t,mbool_t);
int MRsvDestroyGroup(char *,mbool_t);
int MRsvGroupGetList(char *,marray_t *,char *,int);
int MRsvChargeAllocation(mrsv_t *,mbool_t,char *);
int MUIRsvInitialize(mrsv_t **,mrsv_t *,mbool_t);
int MRsvInitialize(mrsv_t **);
int MRsvFind(const char *,mrsv_t **,enum MRsvAttrEnum);
int MRsvFindNext(char *,mrsv_t **,enum MRsvAttrEnum);
int MRsvUpdateStats(void);
int MRsvCheckStatus(mrsv_t *);
int MRsvClearNL(mrsv_t *);
int MRsvCheckAdaptiveState(mrsv_t *,char *);
int MRsvJCreate(mjob_t *,mnl_t **,long,enum MJRsvSubTypeEnum,mrsv_t **,mbool_t);
int MRsvJoin(mrsv_t *,mrsv_t *,char *);
int MRsvMigrate(mrsv_t *,mrsv_t *,char *);
int MRsvSyncJCreate(mjob_t *,mnl_t **,long,enum MJRsvSubTypeEnum,mrsv_t **,mbool_t);
int MRsvFreeTable(void);
mbool_t MRsvCheckJAccess(const mrsv_t *,const mjob_t *,long,maclcon_t *,mbool_t,mbool_t *,mbool_t *,char *,char *);
mbool_t MRsvIsReqRID(const mrsv_t *,char *);
mbool_t MRsvHasOverlap(mrsv_t *,mrsv_t *,mbool_t,mnl_t *,mnode_t *,mbool_t *);
mbool_t MRsvIsValid(mrsv_t *);
int MRsvTableInsert(const char *,mrsv_t *);
int MRsvTableFind(const char *,mrsv_t **);
int MRsvIterInit(rsv_iter *);
int MRsvTableIterate(rsv_iter *,mrsv_t **);
int MRsvCheckRAccess(mrsv_t *,mrsv_t *,long,mbool_t *,char *);
int MRsvAdjustNode(mrsv_t *,mnode_t *,int,int,mbool_t);
int MRsvCheckJobMatch(mjob_t *,mrsv_t *);
int MRsvAdjustGResUsage(mrsv_t *,int);
int MRsvGetCost(mrsv_t *,mvpcp_t *,mulong,mulong,int,int,int,mcres_t *,mnode_t *,mbool_t,const char *,double *);
int MResGetPE(mcres_t *,mpar_t *,double *);
int MREInsert(mre_t **,long,long,mrsv_t *,mcres_t *,int);
int MRERelease(mre_t **,mrsv_t *);
int MREGetNext(mre_t *,mre_t **);
mrsv_t *MREGetRsv(mre_t *);
int MREFree(mre_t **);
int MREAddSingleEvent(mre_t **,mre_t *);
mre_t *MRESort(mre_t *);
int MREGetNextEvent(mulong *);
int MREAlloc(mre_t **,int);
int MRECopy(mre_t *,mre_t *,int,mbool_t);
int MRECopyEvent(mre_t *,mre_t *,mbool_t);
mbool_t MNodeHasPartialUserRsv(mnode_t *);
int MREListToArray(mre_t *,mre_t **,int *);
int MNodeBuildRE(mnode_t *);
int MRsvShowState(mrsv_t *,mbitmap_t *,mxml_t *,mstring_t *,enum MFormatModeEnum);
int MRsvDiagnoseState(mrsv_t *,mbitmap_t *,mxml_t *,mstring_t *,enum MFormatModeEnum);
int MRsvDiagGrid(char *,int,int);
int MRsvToString(mrsv_t *,int *,char *,int,enum MRsvAttrEnum *);
int MRsvToWikiString(mrsv_t *,char *,mstring_t *);
int MNRsvToXML(mnode_t *,mrsv_t *,mxml_t *,int *);
int MNRsvToString(mnode_t *,mrsv_t *,mbool_t,mxml_t **,char *,int);
int MRsvTransitionAToString(mtransrsv_t *,enum MRsvAttrEnum,mstring_t *,int);
int MRsvAToMString(mrsv_t *,enum MRsvAttrEnum,mstring_t *,int);
int MRsvCreateNL(mnl_t *,char *,int,int,char *,mcres_t *,char *);
int MResApplyConstraints(mrsv_t *,mulong,enum MPolicyTypeEnum,mnl_t *,char *);
int MRsvConfigure(msrsv_t *,mrsv_t *,int,int,char *,mrsv_t **,marray_t *,mbool_t);
int MRsvGetUID(char *,int,macl_t *,char *);
int MRsvRecycleUID(char *);
int MRsvProfileAdd(mrsv_t *);
int MRsvProfileFind(char *,mrsv_t **);
int MRsvRefresh(void);
int MRsvToExportXML(mrsv_t *,mxml_t **,enum MRsvAttrEnum *,mrm_t *);
int MRsvCreateFromXML(mrsv_t *,char *,mxml_t *,mrsv_t **,mrm_t *,char *);
int MRsvDumpNL(mrsv_t *); 
int MRsvTableDumpToLog(const char *,int);
int MOAddRemoteRsv(void *,enum MXMLOTypeEnum,mrm_t *,char *);
int MRsvGetAllocCost(mrsv_t *,mvpcp_t *,double *,mbool_t,mbool_t);
int MRsvReplaceNodes(mrsv_t *,char *,char *);
int MRsvModifyTaskCount(mrsv_t *,enum MObjectSetModeEnum,int,int,char *);
int MRsvModifyNodeList(mrsv_t *,enum MObjectSetModeEnum,char *,mbool_t,char *);
int MRsvCheckNodeAccess(mrsv_t *,mnode_t *);
int MRsvReplaceNode(mrsv_t *,mnode_t *,int,mnode_t *);
int MRsvCreateCredLock(mrsv_t *);
int MRsvDestroyCredLock(mrsv_t *);
int MRsvSyncSystemJob(mrsv_t *);
int MRsvSetJobCount(mrsv_t *);
int MRsvCreateDeadlineRsv(mrsv_t *,long,long,char *);
mbool_t MRsvOwnerPreemptExists();
int MRsvPopulateNL(mrsv_t *);
int MRsvReallocNL(mrsv_t *,int);
int MRsvExpandClassExpression(char *,marray_t *,mpar_t *);
mbool_t MNodeHasClassInExpression(char *,mnode_t *);


/* AM object */

int MAMInitialize(mam_t *);
int MAMPing(mam_t *,enum MStatusCodeEnum *);
int MAMClose(mam_t *);
int MAMProcessOConfig(mam_t *,int,int,double,char *,char **);
int MAMSetAttr(mam_t *,int,void **,int,int);
int MAMAToString(mam_t *,enum MAMAttrEnum,char *,int,int);
int MAMAllocJDebit(mam_t *,mjob_t *,enum MS3CodeDecadeEnum *,char *);
int MAMAllocRDebit(mam_t *,mrsv_t *,mbool_t,enum MS3CodeDecadeEnum *,char *);
int MAMAllocJReserve(mam_t *,mjob_t *,mbool_t,enum MS3CodeDecadeEnum *,char *);
int MAMAllocRReserve(mam_t *,mrsv_t *,enum MS3CodeDecadeEnum *,char *);
int MAMAllocResCancel(mam_t *,void *,enum MXMLOTypeEnum,enum MS3CodeDecadeEnum *,char *);
int MAMAccountCreate(mam_t *,char *,char *,double,mrm_t *,const char *,char *,enum MS3CodeDecadeEnum *,char *);
int MAMAccountAddUser(mam_t *,char *,char *,char *,double,mrm_t *,const char *,enum MS3CodeDecadeEnum *,char *);
int MAMAccountGetDefault(char *,char *,enum MS3CodeDecadeEnum *);
int MAMAccountQuery(mam_t *,char *,double *,mrm_t *,char *,enum MS3CodeDecadeEnum *,char *);
int MAMLoadConfig(char *,char *);
int MAMAdd(char *,mam_t **);
int MAMShow(mam_t *,mxml_t *,mstring_t *,enum MFormatModeEnum);
int MAMConfigShow(mam_t *,int,mstring_t *);
int MAMFind(char *,mam_t **);
int MAMDestroy(mam_t **);
int MAMCreate(char *,mam_t **);
int MAMProcessConfig(mam_t *,char *);
int MAMCheckConfig(mam_t *);
int MAMSetDefaults(mam_t *);
int MAMActivate(mam_t *);
int MAMIAccountVerify(char *,char *);
int MAMShutdown(mam_t *);
int MAMUserUpdateAcctAccessList(mam_t *,mgcred_t *,enum MHoldReasonEnum *);
int MAMSetFailure(mam_t *,int,enum MStatusCodeEnum,const char *);
int MAMGoldDoCommand(mxml_t *,mpsi_t *,char  **,enum MS3CodeDecadeEnum *,char *);
int MAMNativeDoCommand(mam_t *,void *,enum MXMLOTypeEnum,enum MAMNativeFuncTypeEnum,char *,mstring_t *,enum MS3CodeDecadeEnum *);
int MAMNativeDoCommandWithString(mam_t *,const char *,enum MAMNativeFuncTypeEnum);
int MAMOToXML(void *,enum MXMLOTypeEnum,enum MAMNativeFuncTypeEnum,mxml_t **);
int MAMRsvToXML(mrsv_t *,enum MAMNativeFuncTypeEnum,mxml_t **);
int MAMJobToXML(mjob_t *,enum MAMNativeFuncTypeEnum,mxml_t **);
int MAMFlushNAMIQueues(mam_t *);
int MAMAddToNAMIQueue(mam_t *,mxml_t *,enum MAMNativeFuncTypeEnum);
int MAMHandleQuote(mam_t *,void *,enum MXMLOTypeEnum,double *,char *,enum MS3CodeDecadeEnum *);
int MAMHandleCreate(mam_t *,void *,enum MXMLOTypeEnum,enum MAMJFActionEnum *,char *,enum MS3CodeDecadeEnum *);
int MAMHandleStart(mam_t *,void *,enum MXMLOTypeEnum,enum MAMJFActionEnum *,char *,enum MS3CodeDecadeEnum *);
int MAMHandleUpdate(mam_t *,void *,enum MXMLOTypeEnum,enum MAMJFActionEnum *,char *,enum MS3CodeDecadeEnum *);
int MAMHandlePause(mam_t *,void *,enum MXMLOTypeEnum,char *,enum MS3CodeDecadeEnum *);
int MAMHandleResume(mam_t *,void *,enum MXMLOTypeEnum,enum MAMJFActionEnum *,char *,enum MS3CodeDecadeEnum *);
int MAMHandleEnd(mam_t *,void *,enum MXMLOTypeEnum,char *,enum MS3CodeDecadeEnum *);
int MAMHandleDelete(mam_t *,void *,enum MXMLOTypeEnum,char *,enum MS3CodeDecadeEnum *);
int MJobDoPeriodicCharges();

/* ID object */

int MIDLoadCred(midm_t *,enum MXMLOTypeEnum,void *);
int MIDLDAPLoadCred(midm_t *,enum MXMLOTypeEnum,void *,char *);
int MIDLoadConfig(char *,char *);
int MIDAdd(char *,midm_t **);
int MIDShow(midm_t *,mxml_t *,mstring_t *,enum MFormatModeEnum,mbitmap_t *);
int MIDConfigShow(midm_t *,int,char *);
int MIDFind(char *,midm_t **);
int MIDDestroy(midm_t **);
int MIDCreate(char *,midm_t **);
int MIDProcessConfig(midm_t *,char *);
int MIDCheckConfig(midm_t *);
int MIDSetDefaults(midm_t *);
int MIDUserCreate(midm_t *,mgcred_t *,mgcred_t *,char *);



/* par object */

int MParSetDefaults(mpar_t *);
int MParSetAttr(mpar_t *,enum MParAttrEnum,void **,enum MDataFormatEnum,enum MObjectSetModeEnum);
int MParAToMString(mpar_t *,enum MParAttrEnum,mstring_t *,int);
int MParFind(const char *,mpar_t **);
int MParProcessOConfig(mpar_t *,enum MCfgParamEnum,int,double,char *,char **);
int MParGetTC(mpar_t *,mcres_t *,mcres_t *,mcres_t *,mcres_t *,long);
int MParGetPriority(mpar_t *,mjob_t *,mulong,double *,long,char *);
int MParPrioritizeList(mpar_t **,mjob_t *);
int MParAdd(const char *,mpar_t **);
int MParAddNode(mpar_t *,mnode_t *);
int MParRemoveNode(mpar_t *,mnode_t *);
int MParDestroy(mpar_t **);
int MParFreeTable(void);
int MParShow(char *,mxml_t *,mstring_t *,enum MFormatModeEnum,mbitmap_t *,mbool_t);
int MParUpdate(mpar_t *,mbool_t,mbool_t);
int MParConfigShow(mpar_t *,int,int,mstring_t *);
int MParToXML(mpar_t *,mxml_t *,enum MParAttrEnum *);
int MPALFromRangeString(char *,mbitmap_t *,int);
char *MParBMToString(unsigned char *,const char *,char *);
int MParFromXML(mpar_t *,mxml_t *);
int MParLoadConfig(mpar_t *,char *);
int MParProcessConfig(mpar_t *,mrm_t *,char *);
int MParProcessNAvailPolicy(mpar_t *,char **,mbool_t,char *);
int MPALGetFirst(mbitmap_t *,mpar_t **);
void MParCheckRepN(int CallingId);
int MParSetupRestrictedCfgBMs(mbitmap_t *,mbitmap_t *,mbitmap_t *,mbitmap_t *,mbitmap_t *,mbitmap_t *,mbitmap_t *);
int MParLoadConfigFile(mpar_t *,mstring_t *,char *); 



int MVPCAdd(char *,mpar_t **);
int MVPCCreate(char *,mpar_t *); 
int MVPCFind(char *,mpar_t **,mbool_t);
int MVPCInitialize(mpar_t *,char *);
int MVPCUpdate(msrsv_t *);
int MVPCUpdateCharge(mpar_t *,char *);
int MVPCDestroy(mpar_t **,const char *);
int MVPCFree(mpar_t **);
int MVPCShow(mpar_t *,mstring_t *);
int MVPCCheckStatus(mpar_t *);
int MVPCModify(mpar_t *,enum MParAttrEnum,void *,enum MDataFormatEnum,enum MObjectSetModeEnum,mbitmap_t *,char *);
int MVPCUpdateResourceUsage(mpar_t *,mcres_t *);
int MVPCAddTID(mpar_t *,char *,char *,char *,marray_t *,char *);
int MVPCUpdateReservations(mpar_t *,marray_t *,mbool_t,char *);
int MVPCUpdateVariables(mpar_t *);
mbool_t MVPCCanPreempt(const mvpcp_t *,const mjob_t *,const mpar_t *,const mrsv_t *,char *); 



/* node object */

int MNodeShow(mnode_t *);
int MNodeMapInit(char **);
int MNodeMapCopy(char *,char *);
int MNodeConfigShow(mnode_t *,int,mpar_t *,mstring_t *);
int MNodeGetPriority(mnode_t *,const mjob_t *,const mreq_t *,mnprio_t *,int,mbool_t,double *,long,char *);
int MNodeGetCat(mnode_t *,mnust_t **S);
double MNodeGetOCFactor(mnode_t *,enum MResLimitTypeEnum);
double MNodeGetOCThreshold(mnode_t *,enum MResLimitTypeEnum);
double MNodeGetGMetricThreshold(mnode_t *,int);
int MNodeGetOCValue(mnode_t *,int,enum MResLimitTypeEnum);
double MNodeGetOCValueD(mnode_t *,double,enum MResLimitTypeEnum);
int MNodeCreateBaseValue(mnode_t *,enum MResLimitTypeEnum,int);
int MNodeGetBaseValue(const mnode_t *,enum MResLimitTypeEnum);
int MNodeUpdateProfileStats(mnode_t *);
int MNodeUpdateMultiComputeStatus(mnode_t *,mrm_t *,char *,mbool_t *);
int MNodeGetJobCount(mnode_t *);
int MNodeGetRsvCount(mnode_t *,mbool_t,mbool_t);
int MREFindRsv(mre_t *,mrsv_t *,int *);
int MNodeGetRsv(const mnode_t *,mbool_t,mulong,mrsv_t **);
double MNodeGetChargeRate(mnode_t *);
int MProcessNAPrioF(mnprio_t **,const char *);
int MNPrioAlloc(mnprio_t **);
int MNPrioFree(mnprio_t **);
int MNodeParseOSList(mnode_t *,mnodea_t **,char *,int,mbool_t);
int MNodeProcessName(mnode_t *,char *);
int MNodeGetTC(const mnode_t *,const mcres_t *,const mcres_t *,const mcres_t *,const mcres_t *,long,int,enum MAllocRejEnum *);
int MNodeGetJob(mnode_t *,mjob_t **,int *);
int MNodeGetNextJob(mnode_t *,mulong,mre_t **,mjob_t **);
int MNodeGetSysJob(const mnode_t *,enum MSystemJobTypeEnum,mbool_t,mjob_t **);
/*int MNodeGetNumVMCreate(const mnode_t *,mbool_t); */
int MNodeGetOSCount(mnode_t *);
int MNodeGetRMState(mnode_t *,enum MRMResourceTypeEnum,enum MNodeStateEnum *); 
int MNodeApplyStatePolicy(mnode_t *,enum MNodeStateEnum *);
int MNodeGroupInitialize(mnode_t *,mnode_t *,void *);
int MNodeGroupAdd(mnode_t *,mnode_t *,void *);
mbool_t MNodeGroupIsMatch(mnode_t *,mnode_t *,void *);
int MNodeGroupTableCreate(mnode_t **,mnl_t **,mln_t ***,int);
int MNodeGroupTableDestroy(mnode_t **,mnl_t **,mln_t ***);
int MNodeInitialize(mnode_t *,const char *);
int MNodeAdjustState(mnode_t *,enum MNodeStateEnum *);
int MNodeAdjustAvailResources(mnode_t *,double,int,int);
int MNodeSetAttr(mnode_t *,enum MNodeAttrEnum,void **,enum MDataFormatEnum,enum MObjectSetModeEnum);
int MNodeSetState(mnode_t *,enum MNodeStateEnum,mbool_t);
int MNodeSetGMetric(mnode_t *,int,double);
int MGMetricCreate(mgmetric_t ***);
int MGMetricFree(mgmetric_t ***);
int MGMetricAdd(mgmetric_t **,mgmetric_t **);
int MGMetricSubtract(mgmetric_t **,mgmetric_t **);
int MGMetricCopy(mgmetric_t **,mgmetric_t **);
int MNodeSetRMMessage(mnode_t *,char *,mrm_t *);
int MNodeLoadConfig(mnode_t *,char *);
int MTNodeLoadConfig();
int MNodeProcessConfig(mnode_t *,mrm_t *,char *);
int MNodeProcessRMMsg(mnode_t *);
int MNodeFind(const char *,mnode_t **);
int MNodeAdd(const char *,mnode_t **);
int MTNodeAdd(char *,mnode_t **);
int MNodeCreate(mnode_t **,mbool_t);
int MNodeRemove(mnode_t *,mbool_t,enum MNodeDestroyPolicyEnum,char *);
int MNodeDestroy(mnode_t **);
int MNodeRMDataDestroy(mnat_t **);
mbool_t MNodeCanProvision(const mnode_t *,const mjob_t *,int,enum MSystemJobTypeEnum *,mnodea_t **);
int MNodeSetPartition(mnode_t *,mpar_t *,const char *);
char *MNodeAdjustName(char *,int,char *);
int MNodeUpdateResExpression(mnode_t *,mbool_t,mbool_t);
int MNodeUpdateOperations(mnode_t *);
int MNodePostLoad(mnode_t *);
int MNodePostUpdate(mnode_t *);
int MNodeApplyTemplates(mnode_t *,mrm_t *);
int MNodeApplyDefaults(mnode_t *);
int MNodeSetDefaultTriggers(mnode_t *);
int MNodeListGetIgnIdleJobRsvJobs(mnl_t *,mjob_t **);
int MNodeApplyTemplates(mnode_t *,mrm_t *);
int MClusterUpdateNodeState(void);
int MClusterLoadConfig(char *,char *);
int MClusterProcessConfig(mvpcp_t *,char *);
int MNodeGetAdaptiveAccessPolicy(const mnode_t *,const mjob_t *,enum MNodeAccessPolicyEnum *);
int MNodeGetAvailTC(const mnode_t *,const mjob_t *,const mreq_t *,mbool_t,int *);
int MNodeGetEffectiveLoad(mnode_t *,enum MAllocRejEnum,mbool_t,mbool_t,double,double *,double *);
int MNodeGetResourceRM(mnode_t *,char *,enum MRMResourceTypeEnum,mrm_t **);
int MXResourceToXML(mxload_t *,mxml_t *);
int MXResourceFromXML(mxload_t *,mxml_t *);

int MVPCProfileFind(char *,mvpcp_t **);
int MVPCProfileAdd(char *,mvpcp_t **);
int MVPCProfileCreate(char *,mvpcp_t *); 
int MVPCProfileToString(mvpcp_t *,char *,mstring_t *,mbool_t);
int MVPCProfileToXML(mvpcp_t *,char *,mxml_t *,mbool_t,int);

int MVPCToString(mpar_t *,char *,int,int);
int MVPCToXML(mpar_t *,mxml_t *,enum MParAttrEnum *);
int MVPCSetDefaults(mpar_t *);

int MRackAdd(char *,int *,mrack_t **);
int MRackAddNode(mrack_t *,mnode_t *,int);
int MNodeGetLocation(mnode_t *);
int MNodeLocationFromName(mnode_t *,int *,int *);
int MNodeCheckPolicies(const mjob_t *,const mnode_t *,long,int *,char *);
mbool_t MNodePolicyAllowsJAccess(const mnode_t *,const mjob_t *,mulong,enum MNodeAccessPolicyEnum,char *);
mbool_t MNodeCheckSuspendedJobs(const mjob_t *,const mnode_t *,char *);
int MNodeCheckGMReq(const mnode_t *,const mln_t *);
int MNodeCheckStatus(mnode_t *);
int MNodeUpdateAvailableGRes(mnode_t *);
int MNodeGetPreemptList(mjob_t *,const mreq_t *,mnl_t *,mnl_t *,mjob_t **,mjob_t **,long,const mpar_t *,mbool_t,mbool_t,int *,int *);
int MNodeSelectIdleTasks(const mjob_t *,const mreq_t *,const mnl_t *,mnl_t **,int *,int *,char *,int R[MMAX_REQ_PER_JOB][marLAST]);
int MNodeUpdateStateFromVMs(mnode_t *);
int MNodeSelectPreemptTasks(mjob_t *,mnl_t *,mnl_t **,int *,int *,char *,int R[MMAX_REQ_PER_JOB][marLAST],long);
int MClusterClearUsage(void);
int MClusterShowARes(char *,enum MWireProtocolEnum,mbitmap_t *,mrange_t *,enum MWireProtocolEnum,void *,void **,int,int,mbool_t *,char *);
int MClusterShowAResWrapper(char *,enum MWireProtocolEnum,mbitmap_t *,mrange_t *,enum MWireProtocolEnum,void *,void **,int,int,mbool_t *,char *);
int MClusterShowUsage(enum MWireProtocolEnum,mbitmap_t *,void *,void **,int);
int MNodeFreeAResInfoAvailTable(void);
int MNodeProcessFeature(mnode_t *,char *);
int MNodeCheckAllocation(mnode_t *);
int MNodeLoadCP(mnode_t *N,const char *Buf);
int MCheckNode(mnode_t *,mbitmap_t *,mstring_t *);
int MNodeSumJobUsage(mnode_t *,mcres_t *); 
int MNodeDiagnoseEligibility(mnode_t *,mjob_t *,mreq_t *,enum MAllocRejEnum *,char *,mulong,int *,int *); 
int MNodeDiagnoseState(mnode_t *,mstring_t *);
int MNodeShowReservations(mnode_t *,mstring_t *);
int MNodeShowRsv(char *,char *,mpar_t *,mbitmap_t *,mxml_t *,char *);
int MNodeComputeFutureVMRes(mnode_t *,mfutureres_t *);
int MNodeCountFutureVMProcs(mnode_t *,int *);
int MNodeDiagnoseRsv(mnode_t *,mbitmap_t *,mstring_t *);
int MNodeAToString(mnode_t *,enum MNodeAttrEnum,mstring_t *,mbitmap_t *);
int MNodeGetDRes(mnode_t *,mrsv_t *,int,mcres_t *);
int MNodeGetCfgTime(const mnode_t *,const mjob_t *,const mreq_t *,mulong *);
int MNLSort(mnl_t *,const mjob_t *,mulong,enum MNodeAllocPolicyEnum,mnprio_t *,mbool_t);
int MNLSelectContiguousBestFit(int,int,mnl_t *,mnl_t *);
int MNLToString(mnl_t *,mbool_t,const char *,char,char *,int);
int MNLToRegExString(mnl_t *,mstring_t *);
int MNLFromString(const char *,mnl_t *,int,mbool_t);
int MNLFromRegExString(const char *,mnl_t *,int,mbool_t);
int MTLFromString(char *,int *,int,mbool_t);
int MNLGetAttr(mnl_t *,enum MResourceTypeEnum,void *,enum MDataFormatEnum);
int MNLGetJobList(mnl_t *,long *,mjob_t **,int *,int);
int MNLPreemptJobs(mnl_t *,const char *,enum MPreemptPolicyEnum,mbool_t);
int MNLMerge(mnl_t *,mnl_t *,mnl_t *,int *,int *);
int MNLMerge2(mnl_t *,mnl_t *,mbool_t );
int MNLRMFilter(mnl_t *,mrm_t *);
int MNLDiff(mnl_t *,mnl_t *,mnl_t *,mbool_t *,mbool_t *);
int MNLAND(mnl_t *,mnl_t *,mnl_t *);
int MNLAdjustResources(mnl_t *,enum MNodeAccessPolicyEnum,mcres_t,int *,int *,double *);
int MNLFind(const mnl_t *,const mnode_t *,int *);
int MNLCount(const mnl_t *);
int MNLRemoveNode(mnl_t *,mnode_t *,int);
int MNLAddNode(mnl_t *,mnode_t *,int,mbool_t,int *);
int MNLFromArray(mnode_t **,mnl_t *,int);
int MNLProvisionAddToBucket(mnl_t *,char *,int);
int MNLProvisionFlushBuckets();
int MNLProvisionFreeBuckets();
mnode_t *MNLReturnNodeAtIndex(mnl_t *,int);
char *MNLGetNodeName(mnl_t *,int,char *);
int MNLGetNodeAtIndex(const mnl_t *,int,mnode_t **);
int MNLSetNodeAtIndex(mnl_t *,int,mnode_t *);
int MNLSetTCAtIndex(mnl_t *,int,int);
int MNLInit(mnl_t *);
int MNLFree(mnl_t *);
int MNLMultiInit(mnl_t **);
int MNLMultiInitCount(mnl_t **,int);
int MNLMultiFree(mnl_t **);
int MNLMultiFreeCount(mnl_t **,int);
int MNLClear(mnl_t *);
int MNLMultiClear(mnl_t **);
int MNLMultiCopy(mnl_t **,mnl_t **);
int MNLCopyIndex(mnl_t *,int,mnl_t *,int);
mbool_t MNLIsEmpty(const mnl_t *);
int MNLSumTC(mnl_t *);
int MNLGetTCAtIndex(const mnl_t *,int);
int MNLAddTCAtIndex(mnl_t *,int,int);
int MNLToOld(mnl_t *,mnalloc_old_t *);
int MNodePreemptJobs(mnode_t *,const char *,enum MPreemptPolicyEnum,mbool_t);
int MNodeSignalJobs(mnode_t *,char *,mbool_t);
mbool_t MTaskMapIsEquivalent(int *,int *);
int MNodeReserve(mnode_t *,mulong,const char *,char *,const char *);
int MNLReserve(mnl_t *,mulong,const char *,char *,const char *); 
int MNLSize(mnl_t *);
int MNodeGetEffNAPolicy(const mnode_t *N,enum MNodeAccessPolicyEnum,enum MNodeAccessPolicyEnum,enum MNodeAccessPolicyEnum,enum MNodeAccessPolicyEnum *);
int MNodeSetEffNAPolicy(mnode_t *);
mbool_t MNodeRangeIsExclusive(mnode_t *,mjob_t *,mulong,mulong);
int MNodeFromXML(mnode_t *,mxml_t *,mbool_t);
mbool_t MNodeIsInGridSandbox(mnode_t *,mrm_t *);
int MXLoadDestroy(mxload_t **);
int MNodeGetAnticipatedLoad(const mnode_t *,mbool_t,double,double *);
int MNodeGetVMLoad(const mnode_t *,double *,int *);
int MNodeCheckKbdActivity(mnode_t *);
int MPrioFToMString(mnprio_t *,mstring_t *);
int MNodeSetAdaptiveState(mnode_t *);
int MNodeGetPowerUsage(mnode_t *,double *,double *);
int MNodeUpdatePowerUsage(mnode_t *);
int MNodeDetermineEState(mnode_t *);
int MNodeInitXLoad(mnode_t *N);
int MNLNodeSetByVLAN(mnl_t *,char *,int);
int MNodeGetVLAN(mnode_t *,char *);
mbool_t MNodeCannotRunJobNow(const mnode_t *,const mjob_t *);
int MNodeGetFreeTime(mnode_t *,mulong,mulong,int *);
int MNodeGetWindowTime(mnode_t *,mulong,mulong,int *);
void MNodeUpdateJobAttrList(mnode_t *);
void MNodeUpdateAttrList(mnode_t *);
int MNodeCheckReqAttr(const mnode_t *,const mreq_t *,char *,char *);




/* Image object */

int MImageLoadConfig(char *,char *);
int MImageProcessConfig(char *,char *);



/* VM object */

#include "MVM.h"


int MVMHashFree(mhash_t *);
int MVMStringToTL(char *,mvm_t **,int *,int,mbool_t,enum MVMUsagePolicyEnum);
int MVMStringToPNString(mstring_t *,mstring_t *);
mbool_t MVMCanRemove(mvm_t *,char  *);
int MVMUpdate(mvm_t *);
int MVMSynchronizeStorageRsvs();
int MVMRemove(mvm_t *,char *,char  *);
int MVMForceVMMigration(char*);
int MVMFindCompleted(const char *VMID,mvm_t **); 
int MVMComplete(mvm_t *);
int MVMUncomplete(mvm_t *);
int MVMDestroy(mvm_t **,char *);
int MVMListDestroy(mln_t *);
int MVMCheckStorage();
int MVMGetStorageNodes(marray_t *,marray_t *);
int MVMGetStorageMounts(marray_t *);
/*int MVMDestroyInTaskMap(mvm_t **);*/
int MVMSelectDestination(mvm_t *,mnprio_t *,mjob_t *,mnode_t **,mpar_t *,mnl_t *,mhash_t *,mhash_t *,mbool_t,enum MMigrationPolicyEnum,char *);
int MVMSetState(mvm_t *,enum MNodeStateEnum, const char *);
mbool_t MVMNodeIsValidDestination(mvm_t *,mnode_t *,mbool_t,char *);
mbool_t MVMCanMigrate(mvm_t *,mbool_t);
int MNodeVMListToXML(mnode_t *,mxml_t *);
int MVMAToString(mvm_t *,mnode_t *,enum MVMAttrEnum,char *,int,mbitmap_t *);
int MVMToXML(mvm_t *,mxml_t *,enum MVMAttrEnum *);
int MVMFromXML(mvm_t *,mxml_t *);
int MVMLToString(mvm_t **,char *,int);
int MVMStorageMountsToXML(mxml_t *,mvm_t *);
/* int MVMCRFromString(char *,mvm_req_create_t *,char *,int); */
int MNodeUpdatePowerUsage(mnode_t *);
mbool_t MNodeHasOSMatchJobRsv(mnl_t *);
mbool_t MNodeIsValidOS(const mnode_t *,int,mnodea_t**);
mbool_t MNodeIsValidVMOS(const mnode_t *,mbool_t,int,mnodea_t**);
mbool_t MNodeCanProvisionVMOS(const mnode_t *,mbool_t,int,mnodea_t **);
int MNodeGetVMRes(const mnode_t *,const mjob_t *,mbool_t,mcres_t *,mcres_t *,mcres_t *);

int MVMChangeParent(mvm_t *,mnode_t *);
int MVMFind(const char *,mvm_t **);
mbool_t MVMIsMigrating(mvm_t *);
mbool_t MVMIsDestroyed(mvm_t *);
mbool_t MVMIsOwned(mvm_t *);
int MUFreeActionTable(mln_t **,mbool_t);
int MVMAdd(const char *,mvm_t **);
int MUAddAction(mln_t **,char *,mjob_t *);
int MURemoveAction(mln_t **,char *);
mbool_t MUCheckAction(mln_t **,enum MSystemJobTypeEnum,mjob_t **);
mbool_t MNodeCheckAction(mnode_t *,enum MSystemJobTypeEnum,mjob_t **);
int MVMGetUniqueID(char *);
int MVMShowState(mstring_t *,mnode_t *);
int MVMShow(mstring_t *,mvm_t *,mbool_t,mbool_t);
int MVMFree(mvm_t **);
int MVMLoadConfig(char *);
int MVMLoadCP(mvm_t *,const char *);
int MVMProcessConfig(mvm_t *VM,char *);
int MVMPostUpdate(mvm_t *);
int MVMSetAttr(mvm_t *,enum MVMAttrEnum,void const *,enum MDataFormatEnum,enum MObjectSetModeEnum);
int MVMSetGMetric(mvm_t *,int,char *);
int MVMSetParent(mvm_t *,mnode_t *);
int MVMToPendingActionXML(mvm_t *,mxml_t **);
int MVMFailuresToXML(mvm_t *,char *,mxml_t **);
mbool_t MVMIncrMigrateCount();
mbool_t MVMDecrMigrateCount();
mbool_t MVMCanMigrateMore(int);
int MVMCheckVLANMigrationBoundaries(mnode_t *,mnode_t *);
/*int MVMCreateStorage(mvm_req_create_t *,mrsv_t **,mjob_t **,char *,int,char *);*/
int MVMVarsFromXML(mvm_t *,mxml_t *);
/*int MVMHarvestStorageVars(mvm_t *,mjob_t *,enum MJobStateEnum *);*/
int MVMReleaseTempStorageRsvs(mjob_t *);
int MVMUpdateVMTracking(mvm_t *);
int MVMUpdateOutOfBandRsv(mvm_t *);
mbool_t MJobIsPartOfVMTrackingWorkflow(mjob_t *);
int MSchedDiagDirFileAccess(char *);


/* VM transition routines */
int MVMTransition(mvm_t *);
int MVMToTransitionStruct(mvm_t *,mtransvm_t *);
int MVMTransitionFree(void **);
int MVMTransitionAllocate(mtransvm_t **);
int MVMTransitionToXML(mtransvm_t *,mxml_t **,enum MVMAttrEnum *);
mbool_t MVMTransitionMatchesConstraints(mtransvm_t *,marray_t *);
int MVMTransitionAToString(mtransvm_t *,enum MVMAttrEnum,char *,int ); 
int MVMGMetricAToString(mgmetric_t **,char *,int,mbitmap_t *);

/* VM */
int MVMAddGEvent(mvm_t *,char *,int,const char *);
int MVMClearGEvents(mvm_t *);
int MVMAddRM(mvm_t *,mrm_t *);
int MVMScheduleMigration(mulong);
mbool_t MVMIsNodeMigrationAllowed(mnode_t *);

int MVMCreateReqInit(mvm_req_create_t *);
int MVMCreateReqFree(mvm_req_create_t *);
int MVMCreateReqFromJob(mjob_t *,marray_t *,mbool_t,char *);
/* int MVMCreateVMFromJob(mjob_t *,char *); */
/*int MVMCreateStorageJob(mnl_t *,mvm_req_create_t *,mjob_t **);*/
int MVMGetStorageNode(mnl_t *,char *,mnode_t **);
/* int MVMCRCheckNodeFeasibility(mnode_t *,mvm_req_create_t *,char *,int); */
int MVMCRToXML(mvm_req_create_t *,mxml_t **);
int MVMCRFromXML(mvm_req_create_t *,mxml_t *);
int MVMChooseVLANFeature(mjob_t *);
int MVMCreateTempStorageRsvs(mjob_t *,mnl_t *,mrsv_t **,char *);
/* int MVMSubmitDestroyJob(const char *,mvm_t *,mjob_t **);*/
mbool_t MVMJobIsVMMigrate(const mjob_t *);


mbool_t MUOSListContains(mnodea_t *,int,mnodea_t **);



/* req object */

#include "MReq.h"



/* job object */


int MJobFind(const char *,mjob_t **,enum MJobSearchEnum);
int MJobFindAName(const char *,mjob_t **);
int MJobFindHTLookup(const char *,mjob_t **,mbool_t);
int MJobCFind(char *,mjob_t **,enum MJobSearchEnum);
int MJobFindDepend(char const *,mjob_t **,char *);
int MJobFindDependent(mjob_t *,mjob_t **,job_iter *,job_iter *);
int MJobFlagsToMString(mjob_t *,mbitmap_t *,mstring_t *);
int MJobFlagsFromString(mjob_t *,mreq_t *,mbitmap_t *,char **,const char *,char *,mbool_t);
int MJobCreate(const char *,mbool_t,mjob_t **,char *);
int MJobCreateAllocPartition(mjob_t *,mrm_t *,char *,int *);
int MJobCreateSystemData(mjob_t *);
int MJobCreateVLAN(mjob_t *,char *);
int MJobDestroyVLAN(mjob_t *,char *);
int MJobCreateStorage(mjob_t *,char *,mbool_t);
int MJobDestroyStorage(mjob_t *,char *);
int MJobRename(mjob_t *,char *);
int MJobDestroyAllocPartition(mjob_t *,mrm_t *,const char *);
int MJobMove(mjob_t *,mjob_t *);
int MJobMigrate(mjob_t **,mrm_t *,mbool_t,char *,char *,enum MStatusCodeEnum *);
int MJobMigrateNode(mjob_t *,mvm_t *,mnode_t *,mnode_t *);
int MJobScheduleDynamic(mjob_t *,mnl_t *,mnl_t *);
mbool_t MJobSecFlagIsSet(mjob_t *,enum MSecFlagEnum);
int MJobPopulateSystemJobData(mjob_t *,enum MSystemJobTypeEnum,void *); 
int MJobShow(mjob_t *,int,char *);
int MJobShowSystemAttrs(mjob_t *,int,mstring_t *);
int MJobReserve(mjob_t **,mpar_t *,mulong,enum MJRsvSubTypeEnum,mbool_t);
int MJobReleaseRsv(mjob_t *,mbool_t,mbool_t);
mbool_t MJobRsvApplies(const mjob_t *,const mnode_t *,const mrsv_t *,enum MNodeAccessPolicyEnum);
int MJobApplyDeadlineFailure(mjob_t *);
int MJobUpdateFlags(mjob_t *);
int MJobUpdateResourceCache(mjob_t *,mreq_t *,int,int *);
int MJobUpdateResourceUsage(mjob_t *);
int MJobPReserve(mjob_t *,mpar_t *,mbool_t,mulong,mbool_t *);
int MJobParseGeometry(mjob_t *,const char *);
int MJobParseBlocking(mjob_t *);
int MJobCheckPolicies(mjob_t *,enum MPolicyTypeEnum,mbitmap_t *,mpar_t *,enum MPolicyRejectionEnum *,char *,long);
mbool_t MJobAllowProxy(mjob_t *,char *);
mbool_t MJobOnlyWantsThisGRes(mjob_t *,int);
int MJobCheckDeadLine(mjob_t *);
int MDynJobCheckTargets(mjob_t *,int *,mbool_t,char    *);
int MJobAdjustRsvStatus(mjob_t *,mpar_t *,mbool_t);
int MJobAllocateGResType(mjob_t *,char *);
int MJobRemoveNonGResReqs(mjob_t *,char *);
int MJobStartRsv(mjob_t *);
int MJobSyncStartRsv(mjob_t *);
int MJobGetFeasibleNodeResources(const mjob_t *,enum MVMUsagePolicyEnum,const mnode_t *,mcres_t *);
int MJobGetQOSRsvBucketIndex(mjob_t *,int *); 
int MJobTranslateNCPUTaskList(mjob_t *,int *);
int MJobIterInit(job_iter *);
int MJobTableInsert(const char *,mjob_t *);
int MJobTableFind(const char *,mjob_t **);
int MJobTableIterate(job_iter *,mjob_t **);
int MCJobIterInit(job_iter *);
int MCJobTableInsert(const char *,mjob_t *);
int MCJobTableFind(const char *,mjob_t **);
int MCJobTableIterate(job_iter *,mjob_t **);
int MCJobFreeTable(void);
int MJobSetAttr(mjob_t *,enum MJobAttrEnum,void **,enum MDataFormatEnum,enum MObjectSetModeEnum);
int MJobSetDynamicAttr(mjob_t *,enum MJobAttrEnum,void **,enum MDataFormatEnum);
int MJobGetDynamicAttr(mjob_t *,enum MJobAttrEnum,void **,enum MDataFormatEnum);
int MJobSetNodeSet(mjob_t *,char *,mbool_t);
int MJobEval(mjob_t *,mrm_t *,mrm_t *,char *,enum MAllocRejEnum *);
int MJobAdjustVirtualTime(mjob_t *);
int MJobAdjustWallTime(mjob_t *,long,long,mbool_t,mbool_t,mbool_t,mbool_t,char *);
int MJobAdjustHold(mjob_t *);
int MJobFromXML(mjob_t *,mxml_t *,mbool_t,enum MObjectSetModeEnum);
int MJobInitialize(mjob_t *);
int MJobToXML(mjob_t *,mxml_t **,enum MJobAttrEnum *,enum MReqAttrEnum *,char *,mbool_t,mbool_t);
int MJobSystemToXML(msysjob_t *,mxml_t **);
int MJobSystemFromXML(msysjob_t *,mxml_t *);
int MJobSystemCopy(msysjob_t *,msysjob_t const *);
int MJobToExportXML(mjob_t  *,mxml_t **,enum MJobAttrEnum *,enum MReqAttrEnum *,char *,mbool_t,mrm_t *);
int MJobBaseToXML(mjob_t *,mxml_t **,enum MJobStateTypeEnum,mbool_t,mbool_t);
int MJobAttrToString(mjob_t *,int,char *,int);
int MJobSetQOS(mjob_t *,mqos_t *,int);
int MJobAddEnvVar(mjob_t *,const char *,const char *,char);
int MJobRemoveEnvVarList(mjob_t *,char *);
int MJobAddDynamicResources(mjob_t *,int,int,mnl_t *,mbool_t,mbool_t,char *,mnl_t *,char *);
int MJobAddGMReq(mjob_t *,mreq_t *,char *);
int MJobRemoveDynamicResources(mjob_t *,int,mnl_t *,mbool_t *,mbool_t,char *,mnl_t *,char *);
int MJobSetState(mjob_t *,enum MJobStateEnum);
mbool_t MJobIsBlocked(mjob_t *);
int MJobSetAllocTaskList(mjob_t *,mreq_t *,int *);
int MJobProvision(mjob_t *,mpar_t *);
int MJobResume(mjob_t *,char *,int *);
int MJobGetPAL(mjob_t *,mbitmap_t *,mbitmap_t *,mbitmap_t *);
int MJobGetQOSDel(mjob_t *,mqos_t **);
int MJobGetCost(mjob_t *,mbool_t,double *);
int MJobGetVarList(mjob_t *,mln_t **,char B[][MMAX_BUFFER3],int *);
int MJobGetVCThrottleCount(const mjob_t *,int *);
int MJobGetNAPolicy(const mjob_t *,const mreq_t *,const mpar_t *,const mnode_t *,enum MNodeAccessPolicyEnum *);
int MJobCleanupForRemoval(mjob_t *);
int MJobRemove(mjob_t **);
int MJobGetAccount(mjob_t *,mgcred_t **);
int MJobGetGroup(mjob_t *,mgcred_t *,mgcred_t **);
int MJobAddDependency(mjob_t *,char *);
int MJobGetEstNC(const mjob_t *,int *,int *,mbool_t);
int MJobSetCreds(mjob_t *,const char *,const char *,const char *,mbool_t,char *);
int MJobSetDefaults(mjob_t *,mbool_t);
int MJobGetDefaultWalltime(mjob_t *,long *);
int MJobAllocBase(mjob_t **,mreq_t **);
int MJobAllocMNL(mjob_t *,mnl_t **,char *,mnl_t **,enum MNodeAllocPolicyEnum,mulong,int *,char *);
enum MNodeAllocPolicyEnum MJobAdjustNAPolicy(enum MNodeAllocPolicyEnum,mjob_t *,mpar_t *,mulong,PluginNodeAllocate **);
int MJobGMetricUpdateAttr(mjob_t *,const char *,const char *);
const mbitmap_t *MJobGetRejectPolicy(const mjob_t  *);
int MJobJoin(mjob_t *,mjob_t *,char *);
int MJobRefreshFromNodeLists(mjob_t *);

/* MJobClass APIs */
#include "MJobClass.h"

/* MJobArray APIs */

#include "MJobArray.h"

/* MPreempt APIs */
#include "MPreempt.h"

int MJobDistributeMNL(mjob_t *,mnl_t **,mnl_t **);
int MJobSelectMNL(mjob_t *,const mpar_t *,mnl_t *,mnl_t **,char *,mbool_t,mbool_t,mbool_t *,char *,int *);
int MJobDistributeTasks(mjob_t *,mrm_t *,mbool_t,mnl_t *,int *,int);
const char *MJobGetName(mjob_t *,const char *,mrm_t *,char *,int,enum MJobNameEnum);
int MJobCalcStartPriority(mjob_t *,const mpar_t *,double *,enum MPrioDisplayEnum,mxml_t **,mstring_t *,enum MFormatModeEnum,mbool_t);
void MJobSetStartPriority(mjob_t *,int,long);
int MJobGetRunPriority(mjob_t *,int,double *,double *,char *);
int MUExpandPBSJobIDToJobName(char *,int,mjob_t *);
int MJobGetBackfillPriority(mjob_t *,unsigned long,int,double *,char *);
int MJobGetAMNodeList(mjob_t *,mnl_t **,mnl_t **,char *N,int *,int *,long);
int MJobProcessCompleted(mjob_t **);
int MJobProcessRemoved(mjob_t **);
int MJobProcessTerminated(mjob_t *,enum MJobStateEnum);
int MJobProcessFailedStart(mjob_t *,mbitmap_t *,enum MHoldReasonEnum,const char *);
int MJobAllocatePriority(const mjob_t *,const mreq_t *,mnl_t *,int,int *,int *,char *,int,int *,mnl_t **,int *,int *,long);
int MJobAllocateFastest(mjob_t *,mreq_t *,mnl_t *,int,int *,int *,char *,int,int *,mnl_t **,int *,int *);
int MJobAllocateBalanced(mjob_t *,mreq_t *,mnl_t *,int,int *,int *,char *,int,int *,mnl_t **,int *,int *);
int MJobAllocateContiguous(mjob_t *,mreq_t *,mnl_t *,int,int *,int *,char *,int,int *,mnl_t **,int *,int *);
int MJobAllocateCPULoad(mjob_t *,mreq_t *,mnl_t *,int,int *,int *,char *,int,int *,mnl_t **,int *,int *);
int MJobAllocateLastAvailable(mjob_t *,mreq_t *,mnl_t *,int,int *,int *,char *,int,int *,mnl_t **,int *,int *,int *);
int MJobAllocateFirstAvailable(mjob_t *,mreq_t *,mnl_t *,int,int *,int *,char *,int,int *,mnl_t **,int *,int *);
int MJobAllocateMinResource(mjob_t *,mreq_t *,mnl_t *,int,int *,int *,char *,int,int *,mnl_t **,int *,int *);
int MJobCheckNStartTime(const mjob_t *,const mreq_t *,const mnode_t *,long,int *,double,enum MAllocRejEnum *,char *,long *,char *);
int MJobGetNRange(const mjob_t *,const mreq_t *,const mnode_t *,long,int *,long *,char *,enum MRsvTypeEnum *,mbool_t,char *);
int MJobGetSNRange(const mjob_t *,const mreq_t *,const mnode_t *,mrange_t *,int,char *,enum MRsvTypeEnum *,mrange_t *,mcres_t *,mbool_t,char *);
int MJobCTimeComp(mjob_t *,mjob_t *);
int MJobStartPrioComp(mjob_t **,mjob_t **);
int MJobRunImmediateDependents(mjob_t *,mln_t *); 
int MJobTemplateIsValidGenSysJob(mjob_t *); 
int MJobApproveFlags(mjob_t *,mbool_t,mbitmap_t *);
int MJobGetIWD(mjob_t *,char *,int,mbool_t);
int MJobGetCmd(mjob_t *,char *,int);
int MJobGetTotalGRes(const mjob_t *,msnl_t *);
int MJobFragment(mjob_t *,mjob_t **,mbool_t);
int MJobRemoveDependency(mjob_t *,enum MDependEnum,char *,char *,mulong);
int MJobWorkflowToHash(mhash_t *,mjob_t *);
int MJobWorkflowCreateTriggers(mjob_t *,mjob_t *);
mbool_t MJobWantsSharedMem(mjob_t *);
int MJobSynchronizeDependencies(mjob_t *);
int MJobExpandPaths(mjob_t *);
int MCredCopy(mcred_t *,mcred_t *);
int MJobCopyCL(mjob_t *,const mjob_t *);
int MRangeApplyLocalDistributionConstraints(mrange_t *,mjob_t *,mnode_t *);
int MRangeApplyGlobalDistributionConstraints(mrange_t *,mjob_t *,int *);
int MRangeGetTC(mrange_t *,mulong);
int MRLFromString(char *,mrange_t *);
int MRLMerge(mrange_t *,mrange_t *,int,mbool_t,long *);
int MRLOffset(mrange_t *,mulong);
int MRLReduce(mrange_t *,mbool_t,int);
int MRLMergeTime(mrange_t *,mrange_t *,mbool_t,long,int);
int MRLLinkTime(mrange_t *,mulong,int);
int MRLAND(mrange_t *,mrange_t *,mrange_t *,mbool_t);
int MRLANDTest(void);
int MRLCheckTime(mrange_t *,mulong,mulong);
int MRLLimitTC(mrange_t *,mrange_t *,mrange_t *,int);
int MRLSFromA(long,mrange_t *,mrange_t *);
char *MRLShow(mrange_t *,mbool_t,mbool_t);
int MRLGetEList(mrange_t A[][MMAX_RANGE],int,long *); 
int MRLGetIndexArray(mrange_t A[][MMAX_RANGE],int,mulong,int *);
int MRLClear(mrange_t *);
int MRLGetIntersection(mrange_t *,mrange_t *,mrange_t *);
int MRLGetRE(mre_t *,mrange_t *,mrange_t *);
int MJobSendFB(mjob_t *);
int MJobExtendStartWallTime(mjob_t *);
int MJobStart(mjob_t *,char *,int *,const char *);
int MJobSyncStart(mjob_t *);
int MJobReject(mjob_t **,enum MHoldTypeEnum,long,enum MHoldReasonEnum,const char *);
int MJobSetHold(mjob_t *,enum MHoldTypeEnum,long,enum MHoldReasonEnum,const char *);
int MJobMakeTemp(mjob_t **);
int MJobFreeTemp(mjob_t **);
int MJobCheckLimits(const mjob_t *,enum MPolicyTypeEnum,const mpar_t *,mbitmap_t *,long *,char *);
int MJobCheckCredentialLimits(const mjob_t *,enum MPolicyTypeEnum,const mpar_t *,mbitmap_t *,long *,char *);
int MJobCheckFairshareTreeLimits(const mjob_t *,enum MPolicyTypeEnum,const mpar_t *,mbitmap_t *,long *,char *);
int MJobCheckVCLimits(const mjob_t *,enum MPolicyTypeEnum,const mpar_t *,mbitmap_t *,long *,char *);
int MJobCheckDataReq(mjob_t *);
int MJobDestroy(mjob_t **);
mbool_t MJobIsInList(mjob_t **,mjob_t *);
int MJobCheckDependency(mjob_t *,enum MDependEnum *,mbool_t,mbool_t *,mstring_t *);
int MJobProcessMissingDependency(enum MDependEnum, mjob_t *, mbool_t, mbool_t *);
int MJobSetDependencyByAName(mjob_t *,char *,enum MDependEnum,mbitmap_t *,mbool_t);
int MJobSetDependency(mjob_t *,enum MDependEnum,char *,char *,mbitmap_t *,mbool_t,char *);
int MJobSetSyncDependency(mjob_t *,mjob_t *);
int MDependFind(mdepend_t *,enum MDependEnum ,char const *,mdepend_t **);
int MJobCheckCircularDependencies(mjob_t *,mjob_t *);
int MJobDependDestroy(mjob_t *);
int MJobDependCopy(mdepend_t *,mdepend_t **);
int MJobSelectResourceSet(const mjob_t *,const mreq_t *,mulong,enum MResourceSetTypeEnum,enum MResourceSetSelectionEnum,char **,mnl_t *,char *,int,char *,int *,mln_t *);
int MJobGetEStartTime(mjob_t *,mpar_t **,int *,int *,mnl_t **,long *,long *,mbool_t,mbool_t,char *);
int MJobGetEstStartTime(char *,mpar_t *,mulong *,mxml_t **,mbool_t,mxml_t *,char *,char *,int); 
int MJobAddToNL(mjob_t *,mnl_t *,mbool_t);
int MJobRemoveFromNL(mjob_t *,mnl_t *);
int MJobFreeTable(void);
int MJobFreeSystemStruct(mjob_t *);
int MJobProcessExtensionString(mjob_t *,char *,enum MXAttrType,mbitmap_t *,char *);
int MRMXFeaturesToString(mreq_t *,char *);
/*int MRMXInit(mrmx_t *);
int MRMXClear(mrmx_t *);*/
int MRMXToMString(mjob_t *,mstring_t *,const mbitmap_t *);
int MRMXAttrToMString(mjob_t *,enum MXAttrType,mstring_t *);
int MStagingDataToString(msdata_t *,mstring_t *);
int MRMXPrefToString(mpref_t *,mstring_t *);
int MJobBuildCL(mjob_t *);
int MJobAddRequiredGRes(mjob_t *,const char *,int,enum MXAttrType,mbool_t,mbool_t);
mbool_t MJobPtrIsValid(const mjob_t *);
mbool_t MReqIsGlobalOnly(const mreq_t *);
mbool_t MJobHasGRes(mjob_t *,const char *,int);
mbool_t MJobHasCreatedEnvironment(mjob_t *);
mbool_t MJobShouldRequeue(mjob_t *,mpar_t *,mbool_t);
mbool_t MJobLangIsSupported(mbitmap_t *,enum MRMTypeEnum);
int MJobListAlloc(mjob_t ***);
int MJobRemoveGRes(mjob_t *,char *,mbool_t);
int MJobRemoveAllGRes(mjob_t *);
int MJobRemoveHash(char *);
int MJobCheckQueuePolicies(mjob_t *,int,mpar_t *,mpar_t *,int *);
int MJobCheckPolicyEvents(mjob_t *,long,int *,char *);
double MJobGetWCAccuracy(mjob_t *,double *);
int MJobGetPE(const mjob_t *,const mpar_t *,double *);
int MOWriteEvent(void *,enum MXMLOTypeEnum,enum MRecordEventTypeEnum,const char *,FILE *,char *);
int MJobGetNL(mjob_t *,mnl_t *);
int MJobDistributeTaskGeometry(mjob_t *,mreq_t *,mnl_t *,int *,int,char *);
int MJobDistributeLLTaskGeometry(mjob_t *,mnl_t *,int *,int,char *);
int MJobCPDestroy(mjob_t *,mjckpt_t **);
int MJobGetRange(mjob_t *,mreq_t *,mnl_t *,mpar_t *,long,mbool_t,mrange_t *,mnl_t **,int *,char *,mbitmap_t *,mbool_t,mrange_t *,char *);
int MJobGetRangeValue(mjob_t *,mpar_t *,mrange_t *,int *,mulong *,int *,mbool_t *,mnl_t **,int *);
int MJobGetRangeWrapper(mjob_t *,mreq_t *,mnl_t *,mpar_t *,long,mrange_t *,mnl_t **,int *,char *,mbitmap_t *,mbool_t,mrange_t *,int *,char *);
int MJobGetMRRange(mjob_t *,mulong *,mpar_t *,long,mrange_t *,mnl_t **,int *,char *,mbitmap_t *,mbool_t,mrange_t *,char *);
int MJobGetINL(const mjob_t *,const mreq_t *,const mnl_t *,mnl_t *,int,int *,int *);
int MJobGetFNL(mjob_t *,mpar_t *,mnl_t *,mnl_t *,int *,int *,long,mbitmap_t *,char *);
int MJobGetNodeStateDelay(const mjob_t *,const mreq_t *,const mnode_t *,long *);
int MJobTestRMExtension(char *);
int MJobTestName(char *);
int MJobTestDist(void);
int MJobAddAccess(mjob_t *,char *,mbool_t);
int MJobAddExclude(mjob_t *,char *,mbool_t);
int MJobSelectFRL(mjob_t *,mreq_t *,mrange_t *,mbitmap_t *,int *);
int MJobToTString(mjob_t *,enum MRecordEventTypeEnum,const char *,mstring_t *);
int MJobToWikiString(mjob_t *,const char *,mstring_t *);
int MJobCheckAccountJLimits(mjob_t *,mgcred_t *,mbool_t,char *,int);
int MJobCheckJMaxLimits(mjob_t *,enum MXMLOTypeEnum,const char *,mjob_t *,mjob_t *,mbool_t,char *,int);
int MJobCheckJMinLimits(mjob_t *,enum MXMLOTypeEnum,const char *,mjob_t *,mjob_t *,mbool_t,char *,int);
int MJobCheckParAccess(mjob_t *,const mpar_t *,mbool_t,enum MAllocRejEnum *,char *);
int MJobCheckRMTemplates(mjob_t *,mrm_t *,char *);
mjtmatch_t *MJobGetTemplate(mjob_t *,mjtmatch_t **,int,int *);
int MJobGetCPName(mjob_t *,char *);
int MJobLoadCPNew(mjob_t *,const char *);
int MJobLoadCPOld(mjob_t *,char *);
int MJobStoreCP(mjob_t *,mstring_t *,mbool_t);
int MJobWriteCPToFile(mjob_t *);
int MCJobStoreCP(mjob_t *,mstring_t *,mbool_t);
int MJobAToString(mjob_t *,enum MJobAttrEnum,char *,int,enum MFormatModeEnum);
int MJobAToMString(mjob_t *,enum MJobAttrEnum,mstring_t *);
int MJobValidate(mjob_t *,char *,int);
int MJobVerifyPolicyStructures(mjob_t *J);
int MJobGetLocalTL(mjob_t *,mbool_t,int *,int);
int MJobDiagnoseState(mjob_t *,mnode_t *,enum MPolicyTypeEnum,mbitmap_t *,mbitmap_t *,mstring_t *);
int MJobDiagnoseEligibility(mjob_t *,mxml_t *,mnode_t *,enum MPolicyTypeEnum,enum MFormatModeEnum,mbitmap_t *,mbitmap_t *,char *,int,mbool_t);
int MJobFeaturesInPAL(mjob_t *,mbitmap_t *,mbitmap_t *,int *); 
int MJobDiagRsvStartFailure(mjob_t *,mpar_t *,char *);
int MOGetCredUsage(void *,enum MXMLOTypeEnum,enum MXMLOTypeEnum,mpu_t **,mpu_t **,mpu_t **);
int MJobApplySetTemplate(mjob_t *,mjob_t *,char *);
int MJobApplyUnsetTemplate(mjob_t *,mjob_t *);
int MJobApplyDefTemplate(mjob_t *,mjob_t *);
int MTJobLoadConfig(char *,char *,char *);
int MTJobProcessConfig(char *,char *,mjob_t **,char *);
int MTJobFind(const char *,mjob_t **);
int MTJobCreateByAction(mjob_t *,enum MTJobActionTemplateEnum,char *,mjob_t **,const char *);
int MTJobFindByAction(mjob_t *,enum MTJobActionTemplateEnum,mjob_t **);
int MTJobAdd(const char *,mjob_t **);
int MTJobProcessAttr(mjob_t *,const char *);
int MTJobLoad(mjob_t *,char *,mrm_t *,char *);
int MTJobMatchLoadConfig(char *,char *);
int MTJobMatchProcessConfig(char *,char *,mjtmatch_t **);
int MTJobCreateFromString(char *,char *,char *,mjob_t **,char *);
int MTJobModify(mjob_t *,char *,char *);
int MTJobCheckCredAccess(mjob_t *,enum MXMLOTypeEnum,char *); 
int MTJobDestroy(mjob_t *,char *,int);
int MTJobFreeXD(mjob_t *);
int MUITJobStatToXML(char *,mxml_t **);
int MTJobStatAlloc(mtjobstat_t *);
int MTJobStatProfileToXML(mjob_t *,char *,char *,mulong,mulong,long,mbool_t,mbool_t,mxml_t *);
int MTJobStoreCP(mjob_t *,char *,int,mbool_t); 
int MTJobToXML(mjob_t *,mxml_t **,char *,mbool_t); 
int MTJobLoadCP(mjob_t *,const char *,mbool_t);
int MTJobStatToXML(mtjobstat_t const *,mbool_t,mxml_t **);
int MTJobStatFromXML(mxml_t const *,mtjobstat_t *);
int MJobTMatchToXML(mjtmatch_t *,mxml_t **,char *); 
int MJobFindSystemJID(char *,mjob_t **,enum MJobSearchEnum);
int MJobFindNodeVM(mjob_t *,mnode_t *,mvm_t **);
int MJobFreeSID(mjob_t *);
int MJobToIdle(mjob_t *,mbool_t,mbool_t);
int MJobAdjustETimes(mjob_t *,mrm_t *);
int MJobDisableParAccess(mjob_t *,mpar_t *);
int MJobGrowTaskMap(mjob_t *,int);
int MJobLLInsertArg(const char *,char *,char *,char *,int);
int MJobProcessStagingReqs(char *);
int MJobInsertEnv(mjob_t *,char *,char *);
int MJobFromJSpec(char *,mjob_t **,mxml_t *,char *,mjob_t **,char *,int,mbool_t *);
int MJobRemoveCP(mjob_t *);
int MJobProcessMigrateFailure(mjob_t *,mrm_t *,char *,enum MStatusCodeEnum);
mbool_t MJobShouldCheckpoint(mjob_t *,mrm_t *);
int MJobCheckExpansion(mjob_t *,mulong,mulong);
mbool_t MJobGResIsRequired(mjob_t *,int);
mbool_t MJobIsMultiReq(mjob_t *);
int MJobInsertHome(mjob_t *,char *,int);
int MJobResolveHome(mjob_t *,char *,char *,int);
int MJobMergeReqs(mjob_t *,const mjob_t *);
int MJobGetDynAttrs(mjob_t *,int *,int *,int *,mulong *);
int MJobConvertNLToVNL(mjob_t *,mnl_t *,char **);
int MJobRunNow(mjob_t *);
int MJobForceStart(mjob_t *);
int MJobForceReserve(mjob_t *,mpar_t *);
int MJobCombine(mjob_t *,mjob_t **);
int MJobGetFSTree(mjob_t *,const mpar_t *,mbool_t);
int MJobCheckFSTreeAccess(mjob_t *,mpar_t *,mgcred_t *,mclass_t *,mgcred_t *,mgcred_t *,mqos_t *);
int MJobProcessRestrictedAttrs(mjob_t *,char *);
int MJobCheckParRes(mjob_t *,mpar_t *);
int MJobCheckParFeasibility(mjob_t *,mpar_t *,mbool_t,char *);
int MJobEvaluateParAccess(mjob_t *,const mpar_t *,enum MAllocRejEnum *,char *); 
int MJobCheckParLimits(const mjob_t *,const mpar_t *,enum MAllocRejEnum *,char *); 
int MJobCheckAllocTX(mjob_t *);
int MJobAllocTX(mjob_t *);
int MJobDestroyTX(mjob_t *);
int MTXFromXML(mjtx_t *,mxml_t *,mjob_t *);
int MJobCopyTrigs(const mjob_t *,mjob_t *,mbool_t,mbool_t,mbool_t);
int MJobAddEffQueueDuration(mjob_t *,mpar_t *);
int MJobCheckPriorityAccrual(mjob_t *,mpar_t *);
int MJobCancel(mjob_t *,const char *,mbool_t,char *,int *);
int MJobCancelWithAuth(const char *,mjob_t *,const char *,mbool_t,char *,int *);
int MJobRequeue(mjob_t *,mjob_t **,const char *,char *,int *);
int MJobCleanupAdaptive(mjob_t *);
int MJobUnMigrate(mjob_t *,mbool_t,char *,int *);
int MJobDestroyList(mjob_t *,int);
int MJobCreateTemplateDependency(mjob_t *,mjob_t *,char *); 
int MJobGetNodeMatchPolicy(mjob_t *,mbitmap_t *);
int MJobFail(mjob_t *,int);
int MJobProcessTVariables(mjob_t *);
int MJobWriteVarStats(mjob_t *,enum MObjectSetModeEnum,const char *,const char *,int *);
int MJobDetermineTTC(mjob_t *);
int MJobUpdateFailRate(mjob_t *);
int MJobGetGroupJob(mjob_t *,mjob_t **);
int MJobCheckConstraintMatch(mjob_t *,mjobconstraint_t *);
mbool_t MJobWillRun(mjob_t *,mnl_t *,mjob_t **,mulong,mulong *,char *);
int MJGroupInit(mjob_t *,char *,int,mjob_t *); 
int MJGroupFree(mjob_t *); 
int MJGroupDup(mjobgroup_t *,mjob_t *);
int MJobDuplicate(mjob_t *,const mjob_t *,mbool_t);
int MJobTransferAllocation(mjob_t *,mjob_t *); 
int MJobGetNBCount(const mjob_t *,const mnode_t *);
int MJobPTransform(mjob_t *,const mpar_t *,int,mbool_t); 
int MJobGetOverlappingSRsvs(mjob_t *,marray_t *);
int MJobCheckProlog(mjob_t *,enum MJobStateEnum *); 
int MJobDoEpilog(mjob_t *,enum MJobStateEnum *); 
int MJobCheckEpilog(mjob_t *); 
int MJobLaunchEpilog(mjob_t *); 
int MJobPopulateReqNodeLists(mjob_t *,mbool_t *); 
int MJobRMNLToString(mjob_t *,mrm_t *,enum MRMResourceTypeEnum,mstring_t *);
int MJobSubmitFromJSpec(mjob_t  *,mjob_t **,char *); 
int MJobSubmitFailureSendMail(char *,char *,char *);
int MJobGetEnvVars(mjob_t *,char,mstring_t *);
int MJobIncrExtLoad(mjob_t *,int,double);
int MJobCountVMCreateJobs(mutime *NextTime);
int MJobParseDependType(char const *String,enum MRMTypeEnum,enum MDependEnum,enum MDependEnum *);
int MJobGetSpecPALCount(mjob_t *);
int MJobPartitionTransformCreds(mjob_t *,mpar_t *);  
int MJobGetBlockInfo(mjob_t *,mpar_t *,mjblockinfo_t **); 
int MJobBlockInfoFree(mjblockinfo_t **);
int MJobAddBlockInfo(mjob_t *,mpar_t *,mjblockinfo_t *);
enum MJobNonEType MJobGetBlockReason(mjob_t *,mpar_t *);
char *MJobGetBlockMsg(mjob_t *,mpar_t *); 
int MJobDupBlockInfo(mjblockinfo_t **,mjblockinfo_t *);
mbool_t MJobCmpBlockInfo(mjblockinfo_t *,enum MJobNonEType,const char *);
mbool_t MJobCanAccessIgnIdleJobRsv(const mjob_t *,const mrsv_t *);

int MDependToString(mstring_t *,mdepend_t *,enum MRMTypeEnum);

/* qsort compare function for Job Completion Time Comp Low to high */

int MJobCompletionTimeCompLToH(mjob_t **,mjob_t **);


/* TORQUE/PBS */
#include "MJobPBS.h"

mbool_t MJobReqIsProcCentric(mjob_t *,char *);
int MJobChargeInternalCompletionCosts(mjob_t *J);
mbool_t MJobIsMatch(mjob_t *,mjob_t *); 
mbool_t MJobIsMultiRM(mjob_t *);
int MJobUpdateGRes(mjob_t *,mreq_t *,msnl_t *,int,char *);



/* queue object */

int MQueueInitialize(mjob_t **,const char *);
int MQueueDestroy(mjob_t **);
int MQueueScheduleIJobs(mjob_t **,mpar_t *);
int MQueuePrioritizeJobs(mjob_t **,mbool_t,const mpar_t *);
int MQueueGetBestRQTime(mjob_t **,long *);
int MQueueScheduleRJobs(mjob_t **,mpar_t *);
int MQueueScheduleSJobs(mjob_t **,int *,char **);
int MUIQueueDiagnose(char *,mjob_t **,enum MPolicyTypeEnum,mpar_t *,mstring_t *);
int MQueueCheckStatus(void);
int MQueueGetRequeueValue(mjob_t **,long,long,double *);
int MQueueSelectAllJobs(enum MPolicyTypeEnum,mpar_t *,mjob_t **,int,int *,mbool_t,mbool_t,mbool_t,mbool_t,mbool_t,char *);
int MQueueSelectJob(mjob_t *,enum MPolicyTypeEnum,mpar_t *,mbool_t,mbool_t,mbool_t,mbool_t,char *);
int MQueueSelectJobs(mjob_t **,mjob_t **,enum MPolicyTypeEnum,int,int,unsigned long,int,int *,mbool_t,mbool_t,mbool_t,mbool_t,mbool_t);
int MQueueAddAJob(mjob_t *);
int MQueueRemoveAJob(mjob_t *,enum MServiceType);
int MAQDump();
int MQueueReplay(void);
int MQueueBackFill(mjob_t **,enum MBFPolicyEnum,enum MPolicyTypeEnum,mbool_t,mpar_t *);
int MQueueApplyGResStartDelay(mulong,int);
int MOQueueInitialize(mjob_t **);
int MOQueueDestroy(mjob_t **,mbool_t);
int MQueueAddCJob(mjob_t *);
int MQueueUpdateCJobs(void);
int MQueueScheduleFloatingDataStage(mrm_t *,int *);
int MQueueCollapse(int *);
void MQueueDiagnoseEligibility();
void MQueueRecordBlock(mjob_t *,mpar_t *,enum MJobNonEType,const char *,mbool_t,mbool_t *);
int MQueueGetNextJobIfSimilar(mjob_t **,int *,mjob_t **,mpar_t *);
mjob_t *MQueueSort(mjob_t *);
int  MQueueMigrateArrays(mjob_t *,mpar_t *,mbool_t);


/* stat object */

int MStatCreate(void **,enum MStatObjectEnum);
int MStatFree(void **,enum MStatObjectEnum);
int MStatIsProfileStat(void *,enum MStatObjectEnum);
mbool_t MStatIsProfileAttr(enum MStatObjectEnum,int);
int MStatPreInitialize(mprofcfg_t *);
int MStatInitialize();
int MStatSetDefaults(void);
int MStatGetFullName(long,char *);
int MStatGetFName(long,char *,int);
int MStatGetFullNameSearch(mulong *,long,char *);
int MStatOpenFile(long,FILE **);
int MStatOpenCompatFile(long);
int MStatWritePeriodPStats(long,enum MCalPeriodEnum);
int MStatShutdown(void);
int MStatToString(must_t *,char *,enum MStatAttrEnum *);
int MStatToXML(void *,enum MStatObjectEnum,mxml_t **,mbool_t,void *);
int MStatFromString(char *,void *,enum MStatObjectEnum);
int MStatFromXML(void *,enum MStatObjectEnum,mxml_t *);
int MStatSetAttr(void *,enum MStatObjectEnum,int,void **,int,enum MDataFormatEnum,int);
int MStatAToString(void *,enum MStatObjectEnum,int,int,char *,enum MFormatModeEnum);
char *MStatsToString(must_t *,char *);
int MStatClearUsage(enum MXMLOTypeEnum,mbool_t,mbool_t,mbool_t,mbool_t,mbool_t);
int MStatClearObjectUsage(mcredl_t *,must_t *S,char *,enum MXMLOTypeEnum,mbool_t,mbool_t,mbool_t,mbool_t);
int MStatClearGCredUsage(enum MXMLOTypeEnum,mbool_t,mbool_t,mbool_t,mbool_t);
int MStatClearOtherUsage(enum MXMLOTypeEnum,mbool_t,mbool_t,mbool_t,mbool_t);
int MStatProfInitialize(mprofcfg_t *);
int MStatUpdateActiveJobUsage(mjob_t *);
int MStatUpdateBacklog(mjob_t **);
int MStatInitializeActiveSysUsage(void);
int MStatUpdateCompletedJobUsage(mjob_t *,enum MSchedModeEnum,mbitmap_t *);
int MStatUpdateRejectedJobUsage(mjob_t *,mbitmap_t *);
int MStatUpdateTerminatedJobUsage(mjob_t *,enum MJobStateEnum,mulong,mulong);
int MStatUpdateStartJobUsage(mjob_t *);
int MStatUpdateSubmitJobUsage(mjob_t *);
double MStatCalcCommunicationCost(mjob_t *);
double MStatGetCom(mnode_t *,mnode_t *);
int MStatAdjustEJob(mjob_t *,mpar_t *,int);
int MStatUpdateBFUsage(mjob_t *);
int MStatBuildMatrix(enum MMStatTypeEnum,char *,int,mbitmap_t *,enum MFormatModeEnum,mxml_t **);
char *MStatGetGrid(enum MMStatTypeEnum,must_t *,must_t *,must_t *,must_t *,int,long,mbitmap_t *);
int MStatPAdjust(void);
int MStatPRoll(enum MXMLOTypeEnum,char *,void **,enum MStatObjectEnum,int *,int);
int MStatPInitialize(void *,mbool_t,enum MStatObjectEnum);
int MStatPInit(void *,enum MStatObjectEnum,int,long);
int MStatPClear(void *,enum MStatObjectEnum);
int MStatPDestroy(void ***,enum MStatObjectEnum,int);
int MStatPToString(void **,enum MStatObjectEnum,int,int,int,char *,int);
int MStatPToSummaryString(void **,enum MStatObjectEnum,int,int,int,char *,int);
int MStatMergeXML(mxml_t *,mxml_t *,char);
int MStatValidatePData(void *,enum MXMLOTypeEnum,char *,must_t *,must_t **,long,long,long,mbool_t,int,mbitmap_t *);
int MStatValidateNPData(mnode_t *,mnust_t  *,mnust_t **,long,long,long,int);
int MStatPCopy(void **,void *,enum MStatObjectEnum);
int MStatPCopy2(void *,void *,enum MStatObjectEnum);
int MStatPExpand(char *,int);
int MStatPStarttimeExpand(char *,int,int);
int MStatGetRange(char *,long *,long *); 
int MStatMerge(void *,void *,void *,enum MStatObjectEnum);
int MStatInsertGenericMetric(double *,char *,double); 
double *MStatExtractXLoad(void *,enum MXMLOTypeEnum); 
int MStatIncrementCredUsage(mjob_t *);
int MStatAddLastFromMemory(enum MXMLOTypeEnum,char *,void *,mulong); 


/* sys object */

int MServerShowCopy(void);
int MServerUpdate(void);

int MSysJobAddQOSCredForProvision(mjob_t *);
int MSysJobStart(mjob_t *,char *,char *);
/*mbool_t MSysJobCheckThrottles(mjob_t *);*/
int MSysJobUpdate(mjob_t *,enum MJobStateEnum *);
/*int MSysJobUpdateRsvFlags(mjob_t *);*/
int MSysJobProcessFailure(mjob_t *);
int MSysJobProcessCompleted(mjob_t *);
int MSysJobAToString(msysjob_t *,enum MJobSysAttrEnum,char *,int,enum MFormatModeEnum,mbool_t);
int MSysJobToWikiString(mjob_t *,msysjob_t *,mstring_t *);
int MSysJobUpdatePeriodic(mjob_t *); 
int MSysJobCreatePeriodic(mjob_t *); 
int MSysJobToPendingActionXML(mjob_t *,mxml_t **);
int MSysJobPassDebugTrigInfo(mjob_t *,mtrig_t *);
int MSysJobGetGenericTrigger(mjob_t *,mtrig_t **); 
int MSysAcquireHALock(void);
int MSysToString(msched_t *,mstring_t *,mbool_t);
int MSysStoreNonBlockingStrings(void);
int MSysToXML(msched_t *,mxml_t **,enum MSysAttrEnum *,enum MXMLOTypeEnum *,mbool_t,int);
int MSysAToString(msched_t *,enum MSysAttrEnum,char *,int,enum MFormatModeEnum);
int MSysSetAttr(msched_t *,enum MSysAttrEnum,void **,enum MDataFormatEnum,enum MObjectSetModeEnum);
int MSysScheduleProvisioning(mbool_t *);
int MSysSchedulePower(void);
int MSysScheduleOnDemandPower(void);
int MSysScheduleVPC(void);
int MSysMemCheck(void);
int MSysSetSignal(int,void *);
int MSysVerifyTextBuf(const char *,int,const char *,char *);
int MSysDaemonize(void);
int MSysInitDB(mdb_t *);
int MSysDoTest(void);
int MSysCheck(void);
int MSysCheckEvents(void); 
int MSysCheckFB(mbool_t);
int MSysCheckAllocPartitions(void); 
int MSysMainLoop(void);
int MSysFBCheckPrimary(mbool_t);
int MSysFBMainLoop(void);
int MSysCheckPeer(char *,int,enum MSvcEnum,char *,mbool_t,long,mccfg_t *,char **,int *);
mbool_t MSysCheckHost(void);
int MSysRefreshCache();
int MSysRegEvent(const char *,enum MSchedActionEnum,int);
int MSysRegExtEvent(const char *,int,long,int);
int MSysLaunchAction(char **,enum MSchedActionEnum);
int MSysSendMail(char *,char *,const char *,char *,const char *);
int MSysLoadConfig(char *,enum MCModeEnum);
int MSysUpdateTime(msched_t *);
int MSysDSRegister(char *,char *,char *,int,char *,char *);
int MSysDSUnregister(char *,char *,char *,int,char *,char *);
int MSysDSQuery(char *,char *,char *,int *,char *,char *);
int MSysEMSubmit(mpsi_t *,char *,char *,char *);
int MSysEMRegister(mpsi_t *,char *,char *,char *,char *);
int MSysSynchronize(void);
int MSysInitialize(mbool_t);
int M64Init(m64_t *);
void MSysShutdown(int,int);
void MSysAbort(int,int);
void MSysInitiateShutdown(int);
int MSysDestroyObjects(void);
int MSysDiagnose(char *,int,enum MFormatModeEnum,long);
int MSysStartServer();
int MSysGetAuth(const char *,enum MSvcEnum,int,mbitmap_t *);
char *MSysGetAdminMailList(int);
int MSysRestart(int);
int MSysReconfigure(char *,mbool_t,mbool_t,mbool_t,int,char *);
mbool_t MParamCanChangeAtRuntime(const char *);
int MSysConfigShow(mstring_t *,int,int);
int MSysRecover(int);
int MSysHandleSignals(void);
int MSysSetCreds(int,int);
int MSysProcessArgs(int,char **,mbool_t);
int MSysShowCopy(void);
int MSysShowEndianness(void);
int MSysShowUsage(char *);
int MSysAuthenticate(void);
int MSysRecordCommandEvent(msocket_t *,enum MSvcEnum,int,char *);


int MSysStartCommThread();
int MCCToMCS(enum MClientCmdEnum,enum MSvcEnum *);


/* License APIs */

#include "MLicense.h"

/* Queue APIs */

#include "MSysSocketQueue.h"
#include "MSysJobSubmitQueue.h"

/* Moab object functions */
#include "MSysObjectQueue.h"


int MSysGetLastEventID(msched_t *);
int MSysUpdateEventCounter(int);
int MSysGetNextEvent(mulong *);

mbool_t MSysUICompressionIsDisabled(void);
int MSysQueryEvents(mulong,mulong,int *,enum MRecordEventTypeEnum *,enum MXMLOTypeEnum *,char A[][MMAX_LINE],mevent_itr_t *);
int MSysQueryEventsToXML(mulong,mulong,int *,enum MRecordEventTypeEnum *,enum MXMLOTypeEnum *,char A[][MMAX_LINE],mxml_t *);
int MSysQueryEventsToArray(mulong,mulong,int *,enum MRecordEventTypeEnum *,enum MXMLOTypeEnum *,char [][MMAX_LINE],mevent_t **,int *,char *);
int MSysQueryPendingActionToXML(mulong,mulong,mxml_t *);
int MSysEItrClear(mevent_itr_t *);
int MSysEItrNext(mevent_itr_t *,mevent_t *);
void *MSysCommThread(void *);
void *MOCacheThread(void *);
void *MOWebServicesThread(void *);
int MSysAddJobSubmitToJournal(mjob_submit_t *); 
int MSysAddJobSubmitToQueue(mjob_submit_t *,int *,mbool_t); 
int MSysDequeueJobSubmit(mjob_submit_t **); 
int MSysGetLastEventIDFromFile(int *);
int MJobSubmitAllocate(mjob_submit_t **);
int MJobSubmitDestroy(mjob_submit_t **);
int MUIJobSubmit(msocket_t *,char *,int,mxml_t *,mjob_t **,char *);
int MJobCmp(mjob_t *,mjob_t *);


enum MJobAttrEnum MS3JAttrToMoabJAttr(enum MS3JobAttrEnum);
enum MReqAttrEnum MS3ReqAttrToMoabReqAttr(enum MS3ReqAttrEnum); 

/* job transition routines */
#include "MJobTransition.h"
#include "MJobTransitionXML.h"

/* reservation transition routines */
int MRsvTransition(mrsv_t *,mbool_t);
int MRsvToTransitionStruct(mrsv_t *,mtransrsv_t *);
int MRsvTransitionFree(void **);
int MRsvTransitionAllocate(mtransrsv_t **);
int MRsvTransitionToXML(mtransrsv_t  *,mxml_t **,enum MRsvAttrEnum *,enum MReqAttrEnum *,long); 
int MRsvTransitionBaseToXML(mtransrsv_t *,mxml_t **,char *);
int MRsvTransitionCopy(mtransrsv_t *,mtransrsv_t *);
mbool_t MRsvTransitionMatchesConstraints(mtransrsv_t *,marray_t *);
int MRsvTransitionFitsConstraintList(mtransrsv_t *,marray_t *);

/* trigger transition routines */
int MTrigTransition(mtrig_t *);
int MTrigToTransitionStruct(mtrig_t *,mtranstrig_t *);
int MTrigTransitionFree(void **);
int MTrigTransitionAllocate(mtranstrig_t **);
int MTrigTransitionToXML(mtranstrig_t  *,mxml_t **,enum MTrigAttrEnum *,enum MReqAttrEnum *,long); 
int MTrigTransitionBaseToXML(mtranstrig_t *,mxml_t **,char *);
int MTrigTransitionCopy(mtranstrig_t *,mtranstrig_t *);
mbool_t MTrigTransitionMatchesConstraints(mtranstrig_t *,marray_t *);
int MTrigTransitionFitsConstraintList(mtranstrig_t *,marray_t *);
int MTrigAToString(mtrig_t *,enum MTrigAttrEnum,char *,int,int);

/* VC transition routines */
int MVCTransition(mvc_t *);
int MVCToTransitionStruct(mvc_t *,mtransvc_t *);
int MVCTransitionFree(void **);
int MVCTransitionAllocate(mtransvc_t **);
int MVCTransitionToXML(mtransvc_t *,mxml_t **,enum MVCAttrEnum *,long,mbool_t);
int MVCTransitionBaseToXML(mtransvc_t *,mxml_t **,char *);
int MVCTransitionCopy(mtransvc_t *,mtransvc_t *);
mbool_t MVCTransitionMatchesConstraints(mtransvc_t *,marray_t *);
int MVCTransitionFitsConstraintList(mtransvc_t *,marray_t *);
mbool_t MVCIsSupportedOType(enum MXMLOTypeEnum);
int MVCShowVCs(mstring_t *,char *,mgcred_t *,enum MFormatModeEnum,mbool_t);
int MVCTransitionAToString(mtransvc_t *,enum MVCAttrEnum,mstring_t  *,int ); 
int MVCAddToVC(mvc_t *,mvc_t *,char *);

/* req transition routines */
/* See MReq.h */

/* node transition routines */
int MNodeTransition(mnode_t *);
int MNodeTransitionFree(void **);
int MNodeTransitionAllocate(mtransnode_t **);
int MNodeTransitionToXML(mtransnode_t  *,enum MNodeAttrEnum *,mxml_t **);
int MNodeTransitionToExportXML(mtransnode_t  *,enum MNodeAttrEnum *,mxml_t **,mrm_t *);
int MNodeTransitionAToString(mtransnode_t *,enum MNodeAttrEnum,mstring_t *);
int MNodeTransitionToMongo(mtransnode_t *);
mbool_t MNodeTransitionFitsConstraintList(mtransnode_t *,marray_t *);
mbool_t MNodeTransitionMatchesConstraints(mtransnode_t *,marray_t *);

int MSysPopSStack(msocket_t **);
int MSysHTTPGet(char const *,int,char const *,long,mstring_t *);
int MPeerFind(char *,mbool_t,mpsi_t **,mbool_t);
int MPeerAdd(char *,mpsi_t **);
int MPeerFree(mpsi_t *);
int MPeerDup(mpsi_t **,mpsi_t *);
char *MSysShowBuildInfo(void);
int MSysAllocJobTable(int);
int MSysAllocNodeTable(int);
int MSysAllocCJobTable(void);
int MSysRecordEvent(enum MXMLOTypeEnum,char *,enum MRecordEventTypeEnum,char *,char *,int *);
int MEventToXML(int,enum MRecordEventTypeEnum,enum MXMLOTypeEnum,int unsigned,char *,char *,char *,mxml_t **);

int MSysCheckLicenseTime(mulong,mbool_t,char *,int);
int MSysRaiseEvent();
void MSysThreadDestructor(void *);
void MSysPostFork(void);
int MSysIsHALockFileValid();
int MSysStoreNonBlockingStrings();
int MSysIncrGlobalReqBuf(void);
int MSysUpdateGreenPoolSize(void);
int MSysUpdateGreenPool(mjob_t *,mpar_t *);


/* transaction object (mtrans_t) */

int MTransFind(int,char *,mtrans_t **);
mbool_t MTransIsValid(int,mbool_t *);
int MTransSetLastCommittedTID();
int MTransAdd(mtrans_t *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,int *);
int MTransSet(int,enum MTransAttrEnum,const char *);
int MTransApplyToRsv(mtrans_t *,mrsv_t *);
int MTransToXML(mtrans_t *,mxml_t **);
int MTransAToString(mtrans_t *,enum MTransAttrEnum,mstring_t *,int);
int MTransFindEmptySlot(int *);
int MTransLoadCP(mtrans_t *,const char *);
int MTransFromXML(mtrans_t *,mxml_t *);
int MTransInvalidate(mtrans_t *);
int MTransFree(mtrans_t *);
int MTransCheckStatus(void);



/* limit object */

int MLimitToXML(mcredl_t *,mxml_t **,enum MLimitAttrEnum *);
int MLimitAToString(mcredl_t *,enum MLimitAttrEnum,char *,int);
int MLimitFromXML(mcredl_t *,mxml_t *);
int MLimitSetAttr(mcredl_t *,enum MLimitAttrEnum,void **,int,int);           
int MLimitEnforceAll(mpar_t *);
int MLimitGetMPU(mcredl_t *,void *,enum MXMLOTypeEnum,enum MLimitTypeEnum,mpu_t **);
int MPUCreate(mpu_t **);
int MPUInitialize(mpu_t *);
int MGPUInitialize(mgpu_t *);
int MPUInitializeOverride(mpu_t *);
int MPUCopy(mpu_t *,mpu_t *);
int MGPUCopy(mgpu_t *,mgpu_t *);
int MPUCopyGLimit(mpu_t *,mpu_t *);
int MPUClearUsage(mpu_t *);
int MGPUClearUsage(mgpu_t *);



/* fs object */

int MFSToXML(mfs_t *,mxml_t **,enum MFSAttrEnum *);
int MFSAToString(mfs_t *,enum MFSAttrEnum,char *,int);
int MFSCAToString(mfsc_t *,mfs_t *,enum MFSCAttrEnum,char *,int);
int MFSTargetFromString(mfs_t *,char *,mbool_t);
int MFSSetAttr(mfs_t *,enum MFSAttrEnum,void **,int,int);
int MFSFromXML(mfs_t *,mxml_t *);
int MFSProcessOConfig(mfsc_t *,enum MCfgParamEnum,int,double,char *,char **,mbool_t,char *);
int MFSLoadConfig(void);
int MFSCheckCap(mfs_t *,mjob_t *,mpar_t *,int *);
int MFSLoadDataFile(char *,int,mbool_t,int);
int MFSUpdateData(mfsc_t *,int,mbool_t,mbool_t,mbool_t);
int MFSInitialize(mfsc_t *);
double MFSCalcFactor(mfsc_t *,double *);
char *MFSTargetToString(double,enum MFSTargetEnum);
int MFSShutdown(mfsc_t *);
int MFSShow(char *,mstring_t *,enum MXMLOTypeEnum,mpar_t *,mbitmap_t *,enum MFormatModeEnum,mbool_t);
int MFSTreeReplicate(mxml_t *,mxml_t **,int);
int MFSTreeFree(mxml_t **);
int MFSTreeLoadConfig(char *);
int MFSTreeProcessConfig(char **,char *,mpar_t *,mbool_t,mbool_t,char *);
int MFSTreeShow(mxml_t *,mxml_t *,mbitmap_t *,int,mstring_t *);
int MFSShowTreeWithFlags(mxml_t *,mbitmap_t *,mstring_t *,mbool_t); 
int MFSTreeGetList(mxml_t *);
int MFSTreeUpdateUsage();
int MFSTreeAssignPriority(mxml_t *,double,int,int,int *);
int MFSTreeStatShow(mxml_t *,mxml_t *,enum MXMLOTypeEnum,long,int,mxml_t **);
int MFSTreeUpdateStats(mxml_t *);
int MFSTreeGetStatXML(mxml_t *,mxml_t *,enum MXMLOTypeEnum,long,int,mxml_t *);
int MFSTreeStatClearUsage(mbool_t,mbool_t,mbool_t);
int MFSTreeValidateConfig(mxml_t *); 
int MFSTreeLoadXML(const char *,mxml_t **,char *);
int MFSTreeGetDepth();
int MFSTreeToXML(mxml_t *,mxml_t **,mbitmap_t *);
int MFSTreeUpdateFSData(mxml_t *,mfsc_t *,mbool_t,mbool_t,mbool_t);
int MFSTreeLoadFSData(mxml_t *,mxml_t *,int,mbool_t);
int MFSInitializeFSTreeData(mfsc_t *);
int MFSCredReset(enum MXMLOTypeEnum OType,char *OName,char *PName);
mxml_t *MFSCredFindElement(mxml_t *,enum MXMLOTypeEnum,char *,int,mbool_t *);
int MFSTreeShareNormalize(mxml_t *,double,double);
int MFSTreeGetQOSBitmap(mxml_t *,char *,char *,mbool_t,mbitmap_t *);
int MFSTreeCheckCap(mxml_t *,mjob_t *,mpar_t *,mstring_t *);
char *MFSTreeReformatXML(char *FSTreeXMLBuf);
mbool_t MFSIsAcctManager(mgcred_t *,mxml_t *,mgcred_t *,mgcred_t *,mbool_t *);
int MFSFindQDef(mjob_t *,mpar_t *,mqos_t **);


/* cfg object */

int MCfgGetIndex(enum MCfgParamEnum,const char *,mbool_t,enum MCfgParamEnum *);
int MCfgTranslateBackLevel(enum MCfgParamEnum *);
int MCfgAdjustBuffer(char **,mbool_t,char **,mbool_t,mbool_t,mbool_t);
int MCfgCompressBuf(char *);
int MCfgProcessLine(int,char *,mpar_t *,char *,mbool_t,char *);
int MCfgEnforceConstraints(mbool_t);
int MCfgGetVal(char **,const char *,char *,int *,char *,int,char **);
int MCfgGetValMString(char **,const char *,char *,int *,mstring_t *,char **);
int MCfgGetIVal(char *,char **,const char *,char *,int *,int *,char **);
int MCfgGetSVal(char *,char **,const char *,char *,int *,char *,int,int,char **);
int MCfgGetSValMString(char *,char **,const char *,char *,int *,mstring_t *,int,char **);
int MCfgProcessBuffer(char *);
int MCfgSetVal(enum MCfgParamEnum,int,double,char *,char **,mpar_t *,char *,mbool_t,char *);
int MCfgGetParameter(char *,const char *,enum MCfgParamEnum *,char *,char *,int,mbool_t,char **);
int MCfgExtractAttr(char *,char **,enum MCfgParamEnum,char *,char *,mbool_t,mbool_t *);
int MCfgEvalOConfig(enum MXMLOTypeEnum,char *,const char *,char **,char *,int);
mbool_t MCfgLineIsEmpty(char *);
int MCfgPurgeBuffer(char *,char **);
int MCfgParsePrioAccrualPolicy(enum MJobPrioAccrualPolicyEnum *,mbitmap_t *,char *);
int MCfgParsePrioExceptions(mbitmap_t *,char *);

/* qos object */

int MQOSFlagsToMString(mqos_t *,mstring_t *,int);
int MQOSFlagsFromString(mqos_t *,char *);
int MQOSSetAttr(mqos_t *,enum MQOSAttrEnum,void **,enum MDataFormatEnum,int);
int MQOSAToMString(mqos_t *,enum MQOSAttrEnum,mstring_t *,int);
int MQOSProcessOConfig(mqos_t *,int,int,double,char *,char **);
int MQOSConfigLShow(mqos_t *,int,int,mstring_t *);
int MQOSInitialize(mqos_t *,const char *);
int MQOSAdd(const char *,mqos_t **);
int MQOSFind(const char *,mqos_t **);
int MQOSGetAccess(mjob_t *,mqos_t *,mpar_t *,mbitmap_t *,mqos_t **);
int MQOSProcessConfig(mqos_t *,char *,mbool_t);
int MQOSListBMFromString(char *,mbitmap_t *,mqos_t **,enum MObjectSetModeEnum);
char *MQOSBMToString(mbitmap_t *,char *);
int MQOSShow(char *,char *,mstring_t *,mbitmap_t *,enum MFormatModeEnum);
int MQOSDestroy(mqos_t **);
int MQOSFreeTable(void);
int MQOSConfigShow(mqos_t *,int,int,mstring_t *);
int MJobCheckQOSJLimits(mjob_t *,mqos_t *,mbool_t,char *,int);
int MQOSReject(mjob_t *,enum MHoldTypeEnum,long,const char *);
int MQOSValidateFairshare(mjob_t *,mqos_t *);



/* class object */

#include "MClass.h"

/* srsv object */

int MSRInitialize(msrsv_t *,char *);
int MSRFind(char *,msrsv_t **,marray_t *);
int MSRAdd(char *,msrsv_t **,marray_t *);
int MSRCreate(char *,msrsv_t *);
int MSRPeriodRefresh(msrsv_t *,marray_t *);
int MSRDestroy(msrsv_t **);
int MSRIterationRefresh(msrsv_t *,marray_t *,char *);
int MSRSetAttr(msrsv_t *,enum MSRAttrEnum,void **,enum MDataFormatEnum,int);
int MSRAToString(msrsv_t *,enum MSRAttrEnum,mstring_t *,mbitmap_t *);
int MSRProcessOConfig(msrsv_t *,int,int,double,char *,char **);
int MResSelectNL(mjob_t *,char *,char *,int,int,mnl_t *,int *,long,mnl_t *,mulong);
int MSRShow(msrsv_t *,mstring_t *,int,enum MCfgParamEnum);
int MSRConfigShow(msrsv_t *,int,enum MCfgParamEnum,mstring_t *);
int MSRGetAttributes(msrsv_t *,int,mulong *,mulong *);
int MSRCheckReservation(msrsv_t *,mrsv_t *);
int MSRLoadConfig(char *,const char *,char *);
int MSRCheckConfig(msrsv_t *);
int MSRProcessConfig(msrsv_t *,char *);
int MSRToXML(msrsv_t *,mxml_t *,enum MSRAttrEnum *);
int MSRToRsvProfile(msrsv_t *,mrsv_t *);
int MSRDiag(msrsv_t *,mstring_t *,enum MFormatModeEnum,marray_t *,mbitmap_t *);
int MSRDiagXML(msocket_t *,mbitmap_t *,char *);
int MSRFromString(msrsv_t *,char *);
int MSRToString(msrsv_t *,char *);
int MSRFromXML(msrsv_t *,mxml_t *);
int MSRFreeTable(void);
int MSRUpdateGridSandboxes(mbool_t *);
int MRsvUpdateGridSandboxes(mbool_t *);
int MRsvProfileConfigShow(msrsv_t *,int,enum MCfgParamEnum,mstring_t *);
int MRsvFromProfile(mrsv_t  *,mrsv_t  *);
int MRsvSumResources(mrsv_t *,mcres_t *);
int MSRUpdateJobCount(mjob_t *,mbool_t);
int MRsvSelectNL(mjob_t *,char *,char *,mbitmap_t *,int,mnl_t *,int *,long,mnl_t *,mbitmap_t *,char *);


/* GEvents Functions */

#include "MGEvents.h"


/* sched object */

int MSchedGetAttr(enum MSchedAttrEnum,enum MDataFormatEnum,void **,int);
int MSchedGetLastCPTime();
int MSchedProcessOConfig(msched_t *,enum MCfgParamEnum,int,double,char *,char **,char *);
int MSchedToString(msched_t *,enum MFormatModeEnum,mbool_t,mstring_t *);
int MSchedStatToString(msched_t *,enum MWireProtocolEnum,void *,int,mbool_t);
int MSchedFromString(msched_t *,const char *);
int MSchedInitialize(msched_t *);
int MSchedSetDefaults(msched_t *);
int MSchedSetAttr(msched_t *,enum MSchedAttrEnum,void **,enum MDataFormatEnum,enum MObjectSetModeEnum);
int MSchedSetState(enum MSchedStateEnum,const char *);
int MSchedSetActivity(enum MActivityEnum,const char *);
int MSchedSetEmulationMode(enum MRMSubTypeEnum);
int MSchedOConfigShow(int,int,mstring_t *);
int MSchedLocateVNode(int,mnode_t **);
int MSchedCheckTriggers(marray_t *,long,long *);
int MPolicyGetEStartTime(void *,enum MXMLOTypeEnum,mpar_t *,enum MPolicyTypeEnum,long *,char *);
int MPolicyAdjustUsage(mjob_t *,mrsv_t *,mpar_t *,enum MLimitTypeEnum,mpu_t *,enum MXMLOTypeEnum,int,char *,mbool_t *);
int MPolicyCheckLimit(const mpc_t *,mbool_t,enum MActivePolicyTypeEnum,enum MPolicyTypeEnum,int,const mpu_t *,const mpu_t *,const mpu_t *,long *,long *,mbool_t *);
int MPCFromJob(const mjob_t *,mpc_t *);
int MPCFromRsv(const mrsv_t *,mpc_t *);
int MSchedProcessJobs(int,mjob_t **,mjob_t **);
int MSchedUpdateStats(void);
int MSchedUpdateNodeCatCredStats(void);
int MSchedTest(void);
int MSchedDiag(const char *,msched_t *,mxml_t *,mstring_t *,enum MFormatModeEnum,mbitmap_t *);
int MSysDiagGreen(char *,mstring_t *,mbitmap_t *);
int MSysDiagHybrid(msched_t *,char *,int,enum MFormatModeEnum,long);
int MSchedProcessConfig(msched_t *,char *,mbool_t);
int MSchedProcessJobRejectPolicy(mbitmap_t *,char **);
int MSchedLoadConfig(char *,mbool_t);
int MSchedPreLoadConfig();
int MSchedConfigShow(msched_t *,mbool_t,mstring_t *);
int MSchedAToString(msched_t *,enum MSchedAttrEnum,mstring_t *,enum MFormatModeEnum);
int MNSFromString(char *,enum MResourceSetSelectionEnum *,enum MResourceSetTypeEnum *,char **); 
int MNSCopy(mreq_t *,const mreq_t *);
int MAdminLoadConfig(char *);
int MAdminProcessConfig(madmin_t *,char *);
int MAdminAToString(madmin_t *,enum MAdminAttrEnum,mstring_t *,enum MFormatModeEnum);
int MSchedAddAdmin(char *,enum MRoleEnum);
int MSchedAddTestOp(enum MRMFuncEnum,enum MXMLOTypeEnum,void *,const char *);
int MSchedUpdateCfgRes(mnode_t *);
int MSchedAddToAddressList(char *,char *,mvaddress_t **,int *);
int MSchedPreSchedInitialize();
int MUGetSchedSpoolDir(char *,int);
int MSchedCreateGMetrics();
void MSchedResetPriorityRsvCounts();
void MSchedIncrementQOSRsvCount(int);
void MSchedIncrementParRsvCount(int);
int MSchedGetQOSRsvCount(int);
int MSchedGetParRsvCount(int);
const char *MSchedGetHomeDir();



/* sim object */

int MSimInitialize(void);
int MSimShow(msim_t *,mbool_t,mstring_t *);
int MSimSummarize(void);
int MSimProcessOConfig(msim_t *,enum MCfgParamEnum,int,double,char *,char **);
int MSimSetDefaults(void);
int MSimRMGetInfo(void);
int MSimMaintainWorkload(void);
int MSimGetWorkload(void);
int MSimJobSubmit(long,mjob_t **,long *);
int MSimJobStart(mjob_t *);
int MSimJobResume(mjob_t *);
int MSimJobModify(mjob_t *,char *,char *,char *,enum MObjectSetModeEnum,char *,int *);
int MSimJobRequeue(mjob_t *);
int MSimJobSuspend(mjob_t *);
int MSimJobCheckpoint(mjob_t *);
int MSimJobTerminate(mjob_t *,enum MJobStateEnum);
int MSimJobCancel(mjob_t *);
int MSimGetResources(char *,char *,char *);
int MSimLoadWorkloadCache(char *,char *,char *,int *);
int MSimJobCreateName(char *,mrm_t *);
int MSimNodeModify(mnode_t *,enum MNodeAttrEnum,char *,void *,enum MObjectSetModeEnum,enum MStatusCodeEnum *);
int MSimQueueModify(mclass_t *,enum MClassAttrEnum,char *,void *,enum MStatusCodeEnum *);
int MSimQueueCreate(mclass_t *,char *,enum MStatusCodeEnum *);



/* RM object */

int MRMInitialize(mrm_t *,char *,enum MStatusCodeEnum *);
int MRMInitializeLocalQueue(void);
int MRMPing(mrm_t *,enum MStatusCodeEnum *);
int MRMPreClusterQuery(void);
int MRMClusterQuery(int *,char *,enum MStatusCodeEnum *);
int MRMCredCtl(mrm_t *,const char *,char *,char *,enum MStatusCodeEnum *);
int MRMWorkloadQuery(int *,int *,char *,enum MStatusCodeEnum *);
int MRMQueueQuery(int *,char *,enum MStatusCodeEnum *);
int MRMInfoQuery(mrm_t *,const char *,char *,char *,char *,enum MStatusCodeEnum *);
int MRMInfoQueryRegister(mrm_t *,char *,char *,enum MStatusCodeEnum *);
int MRMInsertTScript(char *,char *,enum MRMTypeEnum,enum MRMTypeEnum,mstring_t *);
int MRMSystemModify(mrm_t *,const char *,const char *,mbool_t,char *,enum MFormatModeEnum,char *,enum MStatusCodeEnum *);
int MRMSystemQuery(mrm_t *,const char *,const char *,mbool_t,char *,char *,enum MStatusCodeEnum *);
int MRMJobStart(mjob_t *,char *Msg,int *);
int MRMJobCancel(mjob_t *,const char *,mbool_t,char *,int *);
int MRMCreate(const char *,mrm_t *);
int MRMSetDefaults(mrm_t *);
int MRMOConfigShow(mrm_t *,int,int,char *);
int MRMShow(char *,char *,mxml_t *,mstring_t *,enum MFormatModeEnum,mbitmap_t *);
int MRMFind(const char *,mrm_t **);
int MRMFindByPeer(char *,mrm_t **,mbool_t);
int MRMDestroy(mrm_t **,char *);
int MRMFinalizeCycle(void);
int MRMProcessConfig(mrm_t *,char *);
int MRMProcessFailedClusterQuery(mrm_t *,enum MStatusCodeEnum);
int MRMCheckConfig(mrm_t *);
int MRMCheckAllocPartition(mrm_t *,mstring_t *,mbool_t);
int MRMCheckEvents(void);
int MRMJobSubmit(char *,mrm_t *,mjob_t **,mbitmap_t *,char *,char *,enum MStatusCodeEnum *);
int MRMJobSuspend(mjob_t *,const char *,char *,int *);
int MRMJobSignal(mjob_t *,int,char *,char *,int *);
int MRMJobResume(mjob_t *,char *,int *);
int MRMJobCheckpoint(mjob_t *,mbool_t,const char *,char *,int *);
int MRMJobMigrate(mjob_t *,char **,mnl_t *,char *,enum MStatusCodeEnum *);
int MRMJobModify(mjob_t *,const char *,const char *,const char *,enum MObjectSetModeEnum,const char *,char *,int *);
int MRMJobPreLoad(mjob_t *,const char *,mrm_t *);
int MRMReqPreLoad(mreq_t *,mrm_t *);
int MRMVMCreate(char *,mstring_t *,char *,enum MStatusCodeEnum *);
int MRMJobPostLoad(mjob_t *,int *,mrm_t *,char *);
int MRMJobPostUpdate(mjob_t *,int *,enum MJobStateEnum,mrm_t *);
int MRMJobPreUpdate(mjob_t *);
int MRMJobRequeue(mjob_t *,mjob_t **,const char *,char *,int *);
int MRMJobValidate(mjob_t *,mrm_t *,char *,enum MJobCtlCmdEnum *,int *);
int MRMSetFailure(mrm_t *,int,enum MStatusCodeEnum,const char *);
int MRMJobSetAttr(mrm_t *,mjob_t *,void *,char *,mtrma_t *,int *,mbool_t,int *);
int MRMSetState(mrm_t *,enum MRMStateEnum);
int MRMRsvCtl(mrsv_t *,mrm_t *,enum MRsvCtlCmdEnum,void **,char *,enum MStatusCodeEnum *);
int MRMStart(mrm_t *,char *,enum MStatusCodeEnum *);
int MRMStop(mrm_t *,char *,enum MStatusCodeEnum *);
int MRMModuleLoad(void);
int MRMReleaseDynamicResources(mrm_t *,mbool_t,char *);
int MRMNodeMigrate(mnode_t *,mrm_t *,mnode_t *,mrm_t *,char *,char *,enum MStatusCodeEnum *);
int MRMNodeSetPowerState(mnode_t *,mnl_t *,mvm_t *,enum MPowerStateEnum,char *);
int MRMGetComputeRMCount(int *);
int MRMGetData(mrm_t *,mbool_t,char *,enum MStatusCodeEnum *);
mbool_t MRMDataIsLoaded(mrm_t *,enum MStatusCodeEnum *);
int MRMJobCounterIncrement(mrm_t *);
int MRMLoadJobCP(mrm_t *,mjob_t *,char *);
int MRMPurge(mrm_t *);
mbool_t MRMIgnoreNodeStateInfo(const mrm_t *);
int MRMCheckDefaultClass(void);
int MRMUpdateSharedMemJob(mjob_t *);
void MRMUpdateInternalJobCounter(mrm_t *);

int MRMStartFunc(mrm_t *,mpsi_t *,enum MRMFuncEnum);
int MRMEndFunc(mrm_t *,mpsi_t *,enum MRMFuncEnum,long *);
mbool_t MRMIsFuncDisabled(mrm_t *,enum MRMFuncEnum);

#ifdef __cplusplus
extern "C" int MRMIModuleLoad(void);
#else
int MRMIModuleLoad(void);
#endif


int MNatLoadModule(mrmfunc_t *);
int MNatRMInitialize(mrm_t *,char *,char *,enum MStatusCodeEnum *);
int MNatDoCommand(mnat_t *,void *,enum MRMFuncEnum,enum MBaseProtocolEnum,mbool_t,const char *,const char *,mstring_t *,char *,enum MStatusCodeEnum *);
int MNatNodeVirtualize(mnode_t *,char *,char *,mrm_t *,char *,char *,enum MStatusCodeEnum *);
int MNatNodeMigrate(mnode_t *,mrm_t *,mnode_t *,mrm_t *,char *,char *,enum MStatusCodeEnum *);
int MNatRMStart(mrm_t *,char *,char *,enum MStatusCodeEnum *);

int MSQLLoadModule(void);
int MMLoadModule(mrmfunc_t *);
mbool_t MMPeerRequestsArePending(mrm_t *);
int MMRsvDestroy(mrm_t *RM,mrsv_t *,char *);
int MMJobQuery(mrm_t *,mjob_t *,char *,mxml_t **,char *,enum MStatusCodeEnum *);
int MMJobQueryCreateRequest(mjob_t *,mrm_t *,enum MResAvailQueryMethodEnum,mxml_t **,char *);
int MMJobModify(mjob_t *,mrm_t *,const char *,const char *,const char *,enum MObjectSetModeEnum,char *,enum MStatusCodeEnum *);
int MMPeerCopy(char *,mrm_t *,mpsi_t *,char *,int *);
int MMRegisterEvent(enum MPeerEventTypeEnum);
int MRMProcessOConfig(mrm_t *,int,int,double,char *,char **);
int MRMLoadConfig(char *,char *,char *);
void MRMCheckStaleVMs();
int MRMUpdate(void);
int MRMRestore(void);
int MRMGetInternal(mrm_t **);
int MRMAdd(const char *,mrm_t **);
int MRMAddInternal(mrm_t **);
int MRMConfigShow(mrm_t *,int,mstring_t *);
int MRMSetAttr(mrm_t *,enum MRMAttrEnum,void **,enum MDataFormatEnum,enum MObjectSetModeEnum);
int MRMAToString(mrm_t *,enum MRMAttrEnum,char *,int,int);
int MRMNodeModify(mnode_t *,mvm_t *,char *,mrm_t *,enum MNodeAttrEnum,char *,void *,mbool_t,enum MObjectSetModeEnum,char *,enum MStatusCodeEnum *);
int MNodeModify(mnode_t *,char *,enum MNodeAttrEnum,void *,char *,enum MStatusCodeEnum *);
int MNodeReinitialize(mnode_t *,mnl_t *,char *,enum MStatusCodeEnum *);
int MNodeReboot(mnode_t *,mnl_t *,char *,enum MStatusCodeEnum *);
int MNodeProvision(mnode_t *,mvm_t *,char *,char *,mbool_t,char *,int *);
int MNodeRsvFind(const mnode_t *,const char *,mbool_t,mrsv_t **,mjob_t **);
mbool_t MNodeHasVMs(const mnode_t *);
int MRMNodePreLoad(mnode_t *,enum MNodeStateEnum,mrm_t *);
int MRMNodePostLoad(mnode_t *,mrm_t *);
int MRMNodePostUpdate(mnode_t *,mrm_t *);
int MRMNodePreUpdate(mnode_t *,enum MNodeStateEnum,mrm_t *);
int MRMToXML(mrm_t *,mxml_t **,enum MRMAttrEnum *);
int MRMAdjustNodeRes(mnode_t *);
int MPSIToXML(mpsi_t *,mxml_t *);
int MPSIClearStats(mpsi_t *);
int MRMLoadOMap(mrm_t *,char *,char *);
int __MRMNodeAdjustState(mnode_t *);

#ifdef __cplusplus
extern "C" int MRMIModuleList(char *,int,char);
#else
int MRMIModuleList(char *,int,char);
#endif

int MRMQueueModify(mclass_t *,mrm_t *,enum MClassAttrEnum,char *,void *,mbool_t,char *,enum MStatusCodeEnum *);
int MRMQueueCreate(mclass_t *,mrm_t *,char *,mbool_t,char *,enum MStatusCodeEnum *);
int MRMStoreCP(mrm_t *,char *,int);
int MRMLoadCP(mrm_t *,const char *);
int MRMFromXML(mrm_t *,mxml_t *);
int MRMFuncSetAll(mrm_t *);
int MPSIFromXML(mpsi_t *,mxml_t *);
int MRMSetDefaultFnList(mrm_t *);
int MRMAddLangToXML(mxml_t *);
mbool_t MRMIsReal(mrm_t *);



/* RMS interface object */

int MRMSInitialize(void);
int MRMSJobAllocateResources(mjob_t *,mreq_t *,mnl_t *,int,int *,int *,char *,int,int *,mnl_t **,int *,int *);



/* BF object */

int MBFPreempt(mjob_t **,enum MPolicyTypeEnum,mnl_t *,mulong,int,int,mpar_t *);
int MBFFirstFit(mjob_t **,enum MPolicyTypeEnum,mnl_t *,mulong,int,int,mpar_t *);
int MBFBestFit(mjob_t **,enum MPolicyTypeEnum,mnl_t *,mulong,int,int,mpar_t *);
int MBFGreedy(mjob_t **,enum MPolicyTypeEnum,mnl_t *,mulong,int,int,mpar_t *);
int MBFGetWindow(mjob_t *,long,int *,int *,mnl_t *,long *,mbool_t,char *);


/* PBS interface object (only pbs library independent code) */
/*                      (library dependent code found in MPBSI.c */

int MPBSJobSubmit(const char *,mrm_t *,mjob_t **,mbitmap_t *,char *,char *,enum MStatusCodeEnum *);
int __MPBSCmdFileToArgs(mjob_t *,mrm_t *,char *,char *,char *,int,char *);

/* wiki interface object */

#include "MWikiInterfaceAPIs.h"

/* MB class */

int MMBGetTail(mmb_t **,mmb_t ***);
int MMBGetIndex(mmb_t *,const char *,const char *,enum MMBTypeEnum,int *,mmb_t **);
int MMBAdd(mmb_t **,char const *,char const *,enum MMBTypeEnum,long,int,mmb_t **);
int MMBRemove(int,mmb_t **);
int MMBRemoveMessage(mmb_t **,const char *,enum MMBTypeEnum);
char *MMBPrintMessages(mmb_t *,enum MFormatModeEnum,mbool_t,int,char *,int);
int MMBPurge(mmb_t **,char *,int,long);
int MMBFree(mmb_t **,mbool_t);
int MMBFromString(char *,mmb_t **);
int MMBFromXML(mmb_t **,mxml_t *);
int MMBSetAttr(mmb_t *,enum MMBAttrEnum,void *,enum MDataFormatEnum);
int MMBCopyAll(mmb_t **,mmb_t *);



/* util object */

int MUExpandKNumber (char *);
int MUThread(int (*)(),long,int *,int *,mrm_t *,int,...);
char *MUSListToString(char **,char *,int);
int MUMemCCmp(char *Data,char,int);
int *MUIntDup(int);
mbool_t MUIsValidHostName(char *);
int MUNLSortLex(mnl_t *,mulong);
int MUGenRandomStr(char *,int,int);
int MUAddVarsToXML(mxml_t *,mhash_t *);
int MUAddVarsLLToXML(mxml_t *,mln_t *);
int MUAddVarsFromXML(mxml_t *,mhash_t *);
int MUVarsToXML(mxml_t **,mhash_t *); 

/* mem functions */

#include "MMemory.h"
#include "MProfMemory.h"

/* MUStrXXX functions */

#include "MUStr.h"


/* doubly-linked-list utility functions */

#include "MUDLList.h"


int MUCmpFromString(char *,int *);
int MUCmpFromString2(char *);
int MUSignalFromString(char *,int *);
int MUParseComp(char *,char *,enum MCompEnum *,char *,int,mbool_t);
int MUParseNewStyleName(char *,char **,char **);
int MUParseResourceLimitPolicy(mpar_t *,char **);
int MUGetRMTime(char *,enum MRMTypeEnum,long *);
long MUGetOverlap(mulong,mulong,mulong,mulong);
int MTimeToRMString(mulong,enum MRMTypeEnum,char *);
mbool_t MUStringIsMacAddress(char *);
char const *MUBufGetMatchToken(char const *,char *,char const *,char const **,char *,int);
mbool_t MUBoolFromString(char const *,mbool_t);

/* PathList functions */
int MUInitPathList(const char *);
int MUAddToPathList(const char *);
char **MUGetPathList(void);

/* Misc MUGet Operation functions */

#include "MUGetOps.h"

int MUSScanF(const char *,const char *,...);

int MUGetTime(mulong *,enum MTimeModeEnum,msched_t *);
int MUTimeGetDay(long,int *,int *); 
char *MUPrintBuffer(char *,int,char *);
int MUSleep(long,mbool_t);
int MSNLRefreshTotal(msnl_t *);
int MSNLGetCount(long,const msnl_t *,const msnl_t *,int *);
int MSNLGetIndexCount(const msnl_t *,int);
int MSNLSetCount(msnl_t *,int,int);
int MSNLCopy(msnl_t *,const msnl_t *);
mbool_t MSNLIsEmpty(const msnl_t *);
int MUNLFromRangeString(char *,const char *,mnl_t *,char N[][MMAX_NAME],int,int,mbool_t,mbool_t);
mbool_t MUNLNeedsOSProvision(int,mbool_t,mnl_t *,enum MSystemJobTypeEnum *);
mbool_t MNodeNeedsOSProvision(int,mbool_t,mnode_t *,enum MSystemJobTypeEnum *);
mbool_t MNodeVMsAreOverConfiguredRes(mnode_t *);
int MNodeGetVMConfiguredRes(mnode_t *,int *,int *);
mbool_t MUNLNeedsPower(mnl_t *);
int MUNLCreateJobs(const char *,mnl_t *,enum MSystemJobTypeEnum,mulong,char const *,mbitmap_t *,marray_t *,mbool_t);
int MNLCopy(mnl_t *,const mnl_t *);
int MNLPrioritizeList(mjob_t *,mulong,mnl_t *,mnl_t *,mbool_t);
int MUTCCreateJobs(const char *,int,enum MSystemJobTypeEnum,mulong,char const *,mbitmap_t *,marray_t *,mbool_t);
int MUSubmitSysJob(mjob_t **,enum MSystemJobTypeEnum,char *,int,const char *,char const *,mbitmap_t *,mulong,char *);
int MUGenerateSystemJobs(const char *,mjob_t *,mnl_t *,enum MSystemJobTypeEnum,char const *,int,char *,void *,mbool_t,mbool_t,mbitmap_t *,marray_t *);
int MUSubmitVMMigrationJob(mvm_t *,mnode_t *,char *,mjob_t **,char *,char *);
enum MSystemJobTypeEnum MJobGetSystemType(mjob_t const *);
int MJobLinkUpVMTrackingJob(mjob_t *,mbool_t *);
int MAdaptiveJobStart(mjob_t *,mbool_t *,char *);
void MUCollapseHostListRanges(char *);
int MUNLToRangeString(mnl_t *,char *,int,mbool_t,mbool_t,mstring_t *);
int MUNodeNameAdjacent(char *,char *,int *,int *,int *); 
int MUNodeNameNumberOffset(char *NodeName,int  *NodeNumberOffset,int  *NodeNumberLength);
int MNLAdd(mnl_t *,mnode_t *,int);
int MNLRemove(mnl_t *,mnode_t *);
int MNLTerminateAtIndex(mnl_t *,int);

int MGResPlus(msnl_t *,const msnl_t *);
int MGResProcessConfig(int,char *);
int MGMetricInitialize();

int MUBuildPList(mcfg_t *,const char **);

/* CRes Functions */

#include "MCRes.h"

int MUResFactorArrayToString(double *,double *,mstring_t *);
int MUResFactorArrayFromString(char *,double *,double *,mbool_t,mbool_t);

int MUJobNLGetReqIndex(mjob_t *,mnode_t *,int *); 
int MUJobAttributesToString(mbitmap_t *,char *);
char *MPALToString(mbitmap_t *,const char *,char *);
int MPALFromString(char *,enum MObjectSetModeEnum,mbitmap_t *);
int MUShowCopy(void);
int MFUCreate(char *,char *,void *,int,int,int,int,mbool_t,int *);
int MFURemove(char *);
int MFUChmod(char *,int);
int MUSetEnv(const char *,char *);
int MUGetMS(struct timeval *,long *);
int MUGetRC(char *,int,int *,enum MRCListEnum);
long MUTimeFromString(char *);
int MUGetNumDaysInMonth(long); 
int MUGetTimeRangeDuration(long,long,int *,int *);
char *MUTaskMapToString(int *,const char *,char,char *,int);
char *MUTaskMapToMString(int *,const char *,char,mstring_t *);
int MUStringToE(char *,long *,mbool_t);
int MUReadPipe(const char *,char *,char *,int,enum MStatusCodeEnum *);
int MUReadPipe2(const char *,const char *,mstring_t *,mstring_t *,mpsi_t *,int *,char *,enum MStatusCodeEnum *);
int MUClearChild(int *);
int MUCompare(int,int,int);
int MUDCompare(double,enum MCompEnum,double);
char *MULToTStringSeconds(long,char *,int);
int MULToTString(long,char *);
int MULToVTString(long,char **);
int MULToSnapATString(long,char *);
char *MULToATString(mulong,char *);
int MUGetOpt(int *,char **,const char *,char **,int *,char *);
int MUIsIntValueInList(int,char *,const char*);
long MURSpecToL(char *,enum MValModEnum,enum MValModEnum);
char *MULToRSpec(long,enum MValModEnum,char *);
char *MUDToRSpec(double,enum MValModEnum,char *);
char *MUUResToString(mcres_t *,long,enum MFormatModeEnum,char *);
int MUStringPack(char *,char *,int);
int MUMStringPack(const char *,mstring_t *);
char *MUXMLEncode(char *,char *,const char *,mbool_t,int);
mbool_t MUStringIsPacked(char *);
mbool_t MUStringIsRange(char *);
char *MUStringUnpack(char *,char *,int,int *);
int MUStringUnpackToMString(char *,mstring_t *);
unsigned long MUGetHash(char const *);
int MUNLGetMinAVal(mnl_t *,enum MNodeAttrEnum,mnode_t **,void **);
int MUNLGetMaxAVal(mnl_t *,enum MNodeAttrEnum,mnode_t **,void **);
int MUNLGetAvgAVal(mnl_t *,enum MNodeAttrEnum,mnode_t **,void **);
int MUNLGetTotalChargeRate(mnl_t *,double *);

/* overrun-safe buffer functions */

#include "MUSNBuffer.h"

int MUMAToString(enum MExpAttrEnum,char,mbitmap_t *,mstring_t *);
int MUMAGetIndex(enum MExpAttrEnum,char const *,enum MObjectSetModeEnum);
int MUNodeFeaturesFromString(char *,enum MObjectSetModeEnum,mbitmap_t *);
int MUMAFromList(enum MExpAttrEnum,char **,enum MObjectSetModeEnum);
int MUMNLCopy(mnl_t **,mnl_t **);

/* Regex and Range functions */

#include "MURE.h"

char *MUUIDToName(int,char *);
char *MUGIDToName(int,char *);
int MUGIDFromUser(int,char *,char *);
int MUUIDFromName(char *,char *,char *);
int MUGIDFromName(char *);
int MUGNameFromUName(char *,char *);
int MUNameGetGroups(char *,int,int *,char *,int *,int);
int MUSNCTime(mulong *Time,char *);
char *MUFindEnv(const char *,int *);
int MUUnsetEnv(const char *);
int MUPurgeEscape(char *);
int MUGetPeriodRange(long,long,int,enum MCalPeriodEnum,long *,long *,int *);

/* Bit Map declarations */

#include "MBitMaps.h"


char *MUArrayToString(const char **,const char *,char *,int);
char *MUArrayToMString(const char **,const char *,mstring_t *);
int MUTMToHostList(int *,char **,mrm_t *);
int MUGetNodeFeature(const char *,enum MObjectSetModeEnum,mbitmap_t *);
char *MUNodeFeaturesToString(char,const mbitmap_t *,char *);
int MUNodeFeaturesToMString(char,mbitmap_t *,mstring_t *);
char *MUShowIArray(const char *,int,int);
char *MUShowSArray(const char *,int,const char *);
char *MUShowFArray(const char *,int,double);
char *MUShowSSArray(const char *,char *,const char *,char *,int);
char *MUShowSLArray(const char *,char *,long,char *,int);
char *MUShowString(const char *,const char *);
char *MUShowInt(const char *,int Val);
char *MUShowOctal(const char *,int Val);
char *MUShowLong(const char *,long Val);
char *MUShowDouble(const char *,double Val);
int MUBStringTime(long,char *);
int MULToDString(mulong *,char *);
int MUParseRangeString(const char *,mulong *,int *,int,int *,int *);
int MUParseBGRangeString(char *,char **,int,int *);
int MUGetJobIDFromJobName(const char *,int *);
int MAttrSubset(const mbitmap_t *,const mbitmap_t *,enum MCLogicEnum);
int MAVPToXML(const char *,const char *,mamolist_t *,mamolist_t *,mamolist_t *,char *,char *);
int MXMLToAVP(const char *,mamolist_t *,char *);
int MFilterXML(mxml_t **,int,mbool_t,char *);
int MXMLCreateE(mxml_t **,const char *);
int MXMLDestroyE(mxml_t **);
int MXMLExtractE(mxml_t *,mxml_t *,mxml_t **);
int MXMLMergeE(mxml_t *,mxml_t *,char);
int MXMLSetAttr(mxml_t *,const char *,const void *,enum MDataFormatEnum);
int MXMLRemoveAttr(mxml_t *,const char *);
int MXMLToMString(mxml_t *,mstring_t *,char const **,mbool_t); 
int MXMLAppendAttr(mxml_t *,const char *,const char *,char);
int MXMLSetVal(mxml_t *,const void *,enum MDataFormatEnum);
int MXMLAddE(mxml_t *,mxml_t *);
int MXMLSetChild(mxml_t *,char *,mxml_t **);
int MXMLAddChild(mxml_t *,char const *,char const *,mxml_t **);
int MXMLToString(mxml_t *,char *,int,char **,mbool_t);
int MXMLToXString(mxml_t *,char **,int *,int,char const **,mbool_t);
int MXMLGetAttr(mxml_t *,const char *,int *,char *,int);
int MXMLGetAnyAttr(mxml_t *,char *,int *,char *,int);
int MXMLGetAttrMString(mxml_t *,const char *,int *,mstring_t *);
int MXMLDupAttr(mxml_t *,char *,int *,char **);
int MXMLGetAttrF(mxml_t *,const char *,int *,void *,enum MDataFormatEnum,int);
int MXMLGetChild(mxml_t const *,char const *,int *,mxml_t **);
int MXMLGetChildCI(mxml_t *,const char *,int *,mxml_t **);
int MXMLFromString(mxml_t **,const char *,char **,char *);
int MXMLDupE(mxml_t *,mxml_t **);
int MXMLDupENoString(mxml_t  *,mxml_t **);
int MXMLFind(mxml_t *,char *,int,mxml_t **);
int MXMLCompareByName(mxml_t **,mxml_t **);
int MXMLAddCData(mxml_t *,const char *);
int MXMLSort(mxml_t *,int (*)(const void *,const void*));
int MXMLShallowCopyXD(mxml_t *,mxml_t *);


#include "MOSInterface.h"

#ifdef __MIPV6
int MOSGetHostByAddr(struct sockaddr_in6 *,char *);
#else
int MOSGetHostByAddr(struct sockaddr_in *,char *);
#endif
mbool_t MOSHostIsLocal(char *,mbool_t);
mbool_t MOSHostIsSame(char *,char *);
int MOSGetGListFromUName(char *,char *,int *,int,int *);
int MUCheckAuthFile(msched_t *,char *,mbool_t *,mbool_t *,char *,mbool_t);
mbool_t MUPathIsSecure(const char *,int);
int MUCheckString(const char *,char *);
int MUCheckStringList(char *,char *,char);
char *MUInsertEnvVar(const char *,muenv_t *,char *);
uint32_t MUIntReverseEndian(uint32_t Value);
int MURemoveOldFilesFromDir(char *,mulong,mbool_t,char *);
int MURemoveRMDirectives(char *,enum MRMTypeEnum);
int MUBuildSignalString(int,char *,char *);
int MUGetIPAddr(char *,char **);
int MUPackFileNoCache(char *,char **,int *,char *);
int MUCheckUnMungeCreds(char *);
int MUCheckUnMungeSkew(char *);
int MUCheckMungeOutput(char *,char *);
int MUBuildHostListElement(char *,int,int,int,int,int,char *,int,char *);
int MUBuildBGLHostListElement(char *,char *,char *);
int MUStripXDirective(char *,enum MRMTypeEnum);
int MUMksTemp(char *,int *);
char *MUExtractDir(char *,char *,int);
int MURemoveJobListFromArray(mjob_t **,mjob_t **);
int MURemoveJobFromArray(mjob_t **,mjob_t *);


/* hash table functions */

#include "MHash.h"
#include "MHashTable.h"

/* array list functions */

#include "MUArray.h"

int MUFindName(name_t *,char *,int);

/* "Moab string" functions */

#include "MString.h"


/* buffered FD input/output functions */

#include "MUBufferedFD.h"



/* data staging functions */

int MSDCreate(msdata_t **);
int MSDDestroy(msdata_t **);
int MSDGetFileSize(mjob_t *,mrm_t *,char *,mulong *,char *);
int MSDUpdateStageInStatus(mjob_t *,mulong *);
int MSDProcessStageInReqs(mjob_t **);
int MSDGetMinStageInDelay(mjob_t *,mrm_t *,mrm_t **,mulong *,mulong *);
int MSDProcessStageOutReqs(mjob_t *);
int MSDToXML(msdata_t *,mxml_t *,enum MSDataAttrEnum *,char *);
int MSDFromXML(msdata_t *,mxml_t *);
int MSDAddJobHold(char *,enum MHoldTypeEnum);
int MSDGetJobHold(char *,enum MHoldTypeEnum *);
int MSDRemoveJobHold(char *,enum MHoldTypeEnum *);
int MSDSetAttr(msdata_t *,enum MSDataAttrEnum,void **,enum MDataFormatEnum,enum MObjectSetModeEnum);
int MSDUpdateDataReqs(mrm_t *,char *);
int MSDExecuteStageOut(mjob_t *,mrm_t *,char *,enum MJobAttrEnum StageType,char *);
int MDSCreate(mds_t **);
int MDSFree(mds_t **);
int MDSCopy(mds_t **,mds_t *);



int MMNLRemoveNL(mnl_t **,mnl_t *);
int MNLToMString(mnl_t *,mbool_t,const char *,char,int,mstring_t *); 
int MNLGetemReady(mnl_t *,char *,char *);

#include "MUIsStringA.h"

int MUCompressTest();
long MUGetStarttime(int,long);
char *MSLimitToString(char *,char *,char *,int);
char *MSLimitBMToString(const char **,mbitmap_t *,mbitmap_t *,char *,int);
mbool_t MUIsRMDirective(char *,enum MRMTypeEnum);
char *MURMScriptFilter(char *,enum MRMTypeEnum,char *,int);
char *MURMScriptExtractShell(char *,char *);
mbool_t MUCheckList(char *,char **,char **);
mbool_t MUCheckListR(const char *,mlr_t *,mlr_t *);
int MSDParseURL(char *,msdata_t **,enum MDataStageTypeEnum,char *);
int MUFileIsText(char *,char *,mbool_t *);
mbool_t MUFileExists(char const *);
int MArgListAdd(char **,const char *,char *);
int MArgListRemove(char **,const char *,mbool_t);


/* trigger functions */

int MTrigFreeList(marray_t *);
int SetupTrigVarList(mtrig_t *,char *,int);
int MTrigInitiateAction(mtrig_t *);
int MTrigRegisterEvent(mtrig_t *);
int MTrigApplyTemplate(marray_t **,marray_t *,void *,enum MXMLOTypeEnum,char *);
int MTrigFromString(mtrig_t ***,char *,mbool_t,enum MXMLOTypeEnum,char *,mtrig_t **);
int MTrigLoadString(marray_t **,const char *,mbool_t,mbool_t,enum MXMLOTypeEnum,char *,marray_t *,char *);
int MTListToMString(marray_t *,mbool_t ,mstring_t *); 
int MTrigCopy(mtrig_t *,mtrig_t *);
int MTrigCreate(mtrig_t *,enum MTrigTypeEnum,enum MTrigActionTypeEnum,char *);
int MTrigAdd(const char *,mtrig_t *,mtrig_t **);
int MTrigFind(const char *,mtrig_t **);
int MTrigFindByName(const char *,marray_t *,mtrig_t **);
int MTrigRemove(mtrig_t *,char *,mbool_t);
int MTrigFree(mtrig_t **);
int MTListDestroy(marray_t **);
int MTListCopy(marray_t *,marray_t **,mbool_t);
int MTrigFromXML(mtrig_t *,mxml_t *);
int MTrigToXML(mtrig_t *,mxml_t *,enum MTrigAttrEnum *);
int MTrigInitialize(mtrig_t *);
int MTrigSetAttr(mtrig_t *,enum MTrigAttrEnum,void *,enum MDataFormatEnum,enum MObjectSetModeEnum);
int MTListClearObject(marray_t *,mbool_t);
int MTrigCheckState(mtrig_t *);
int MTrigValidate(mtrig_t *);
int MTrigDiagnose(char *,mxml_t *,mxml_t *,mstring_t *,enum MFormatModeEnum,mbitmap_t *,marray_t *,enum MObjectSetModeEnum);
int MTrigDisplayDependencies(char *,mstring_t *);
int MTrigReset(mtrig_t *);
int MTrigAddVariable(mtrig_t *,char *,mbitmap_t *,const char *,mbool_t);
int MTrigDoModifyAction(mtrig_t *,char *,char *);
int MTrigQuery(mtrig_t *,char *,mbool_t);
int MTrigGetOwner(mtrig_t *,mgcred_t **);
int MTrigSetVars(mtrig_t *,mbool_t,char *);
int MTrigLoadExitValue(mtrig_t *,enum MTrigStateEnum *);
int MTrigProbe(mtrig_t *);
int MTrigInitStanding(mtrig_t *);
int MTrigTransitionAToString(mtranstrig_t *,enum MTrigAttrEnum,mstring_t *,int);
int MTrigToTString(mtrig_t *,enum MRecordEventTypeEnum,mstring_t *);
int MTrigSetFlags(mtrig_t *,char *);
int MTrigLaunchInternalAction(mtrig_t *,char *,mbool_t *,char *);
int MTrigParseInternalAction(char *,enum MXMLOTypeEnum *,char *,char *,char *,char *); 
int MTrigCheckInternalAction(mtrig_t *,char *,mbool_t *,char *);
int MTrigLaunchChangeParam(mtrig_t *,char *,char *);
int MTrigRemoveVariable(mtrig_t *,char *,mbool_t);
int MTrigResetETime(mtrig_t *);
int MTrigUnsetVars(mtrig_t *,mbool_t,char *);
int MTrigCheckThresholdUsage(mtrig_t *,double *,char *);
int MTrigCheckDependencies(mtrig_t *,mbitmap_t *,mstring_t *);
int MTrigFlagsToString(mtrig_t *,char *,int);
int MOGetCommonVarList(enum MXMLOTypeEnum,char *,mgcred_t *,mln_t **,char B[][MMAX_BUFFER3],int *);
int MTrigToAVString(mtrig_t *,char,char *,int);
int MTrigDisable(marray_t *);
int MTrigEnable(marray_t *);
int MTrigSetState(mtrig_t *,enum MTrigStateEnum);
int MTrigFailIncomplete();
int MTrigInitSets(marray_t **,mbool_t);
int MTrigIsGenericSysJob(mtrig_t *);
int MTrigTranslateEnv(mstring_t *,mbool_t);
mbool_t MTrigIsEqual(mtrig_t *,mtrig_t *);
mtrig_t *MTListContains(mtrig_t *);
mbool_t MTrigIsReal(mtrig_t *);
mbool_t MTrigIsValid(mtrig_t *);
mbool_t MTrigValidNamespace(const char *,const char *);
mbool_t MTrigVarIsNamespace(const char *,const char **);


/* link list */

#include "MULinkedList.h"


/* general functions */

int MUEnvSetDefaults(muenv_t *,int);
int MUEnvAddValue(muenv_t *,const char *,const char *);
char *MUEnvGetVal(muenv_t *,const char *);
int MUEnvDestroy(muenv_t *);
int MUCheckExpBackoff(mulong,mulong *,mulong *);
int MUSpawnChild(const char *,const char *,const char *,const char *,int,int,const char *,const char *,int *,char *,const char *,char **,char **,char **,int *,int *,long,long,void *,enum MXMLOTypeEnum,mbool_t,mbool_t,char *);
pid_t MUFork();
int MUChildCleanUp(int,int,int,char *,char *,char *);

#include "MUURL.h"

int MSysProcessRequest(msocket_t *,mbool_t);
int MSysLogInitialize(int,const char **,const char *);
int MUStringGetEnvValue(char *,const char *,char,char *,int);
int MJPrioFAdd(mjpriof_t **,enum MJobPrioAttrTypeEnum,char *,long);
int MJPrioFGetPrio(mjpriof_t *,mjob_t *,enum MJobPrioAttrTypeEnum,long *);
int MNodeSetAttrOwner(mnode_t *,mrm_t *,enum MNodeAttrEnum); 
int MNLSetAttr(mnl_t *,enum MNodeAttrEnum,void **,enum MDataFormatEnum,enum MObjectSetModeEnum); 
int MNLSetPower(char *,marray_t *,enum MPowerStateEnum,mbitmap_t *,mstring_t *,msysjob_t**,unsigned int);
int MNodeAddRM(mnode_t *,mrm_t *);


/* Tree management apis */

#include "MTree.h"

int MTLMerge(mtl_t *,mtl_t *,mtl_t *,int);
int MMovePtr(char **,char **);
mbool_t MUIsGridHead();

#include "MVariables.h"


/* ui object */

#include "MUI.h"

/* log object */

int MLogInitialize(const char *,int,int);
int MLogOpen(int);
int MLogRoll(const char *,mbool_t,int,int);
char *MLogGetTime(void);
char *MLogGetTimeNewFormat(void);
int MLogGetName(char **);
void MLogLevelAdjust(int);
int MLogShutdown(void);

#ifndef __MTEST
int MLog(const char *,...);
#endif /* __MTEST */
int MLogNH(const char *,...);

int MLogJob(enum MLogLevelEnum,const char *,int,mjob_t *,char *,...);
int MLogNew(enum MLogLevelEnum,enum MXMLOTypeEnum,char *,const char *,int,char *,va_list);



/* ACL management object */

int MACLFromString(macl_t **,char *,int,enum MAttrEnum,enum MObjectSetModeEnum,momap_t *,mbool_t);
int MACLSet(macl_t **,enum MAttrEnum,void *,int,int,mbitmap_t *,mbool_t);
int MACLSetACL(macl_t **,macl_t *,mbool_t);
int MACLRemove(macl_t **,enum MAttrEnum,void *);
int MACLAddCred(macl_t *,char *,enum MAttrEnum);
int MACLRemoveCredLock(macl_t *,char *);
int MACLLoadConfigLine(macl_t **,char *,enum MObjectSetModeEnum,momap_t *,mbool_t);
int MACLGet(macl_t *,enum MAttrEnum,macl_t **);
int MACLGetCred(macl_t *,enum MAttrEnum,void **);
int MACLCheckAccess(macl_t *,macl_t *,maclcon_t *,char *,mbool_t *);
int MACLListShow(macl_t *,enum MAttrEnum,mbitmap_t *,char *,int);
int MACLListShowMString(macl_t *,enum MAttrEnum,mbitmap_t *,mstring_t *);
char *MACLShow(macl_t *,mbitmap_t *,mbool_t,momap_t *,mbool_t,char *);
int MACLAToString(macl_t *,enum MACLAttrEnum,char *,enum MFormatModeEnum);
int MACLSetAttr(macl_t *,enum MACLAttrEnum,void **,enum MDataFormatEnum,enum MObjectSetModeEnum); 
int MACLToXML(macl_t *,mxml_t **,enum MACLAttrEnum *,mbool_t);
int MACLFromXML(macl_t **,mxml_t *,mbool_t); 
int MACLMergeMaster(macl_t **,macl_t *);
int MACLCheckGridSandbox(macl_t *,mbool_t *,mrm_t *,enum MObjectSetModeEnum);
int MACLCopy(macl_t **,macl_t *);
int MACLMerge(macl_t **,macl_t *);
int MACLInit(macl_t *);
mbool_t MACLIsEmpty(macl_t *);
int MACLSubtract(macl_t **,macl_t *);
int MACLGetNext(macl_t *,macl_t *);
int MACLCount(macl_t *);
int MACLFree(macl_t **);
int MACLFreeList(macl_t **);


/* file util object */

char *MFULoad(const char *,int,enum MAccessModeEnum,int *,char *,int *);
char *MFULoadNoCache(const char *,int,int *,char *,int *,int *);
int MFULoadAllocReadBuffer(char *,char **,char *,int *);
int MFUGetInfo(const char *,mulong *,long *,mbool_t *,mbool_t *);
int MFUCacheInitialize(mulong *);
int MFUGetModifyTime(char *,long *);
int MFUCacheInvalidate(char *);
int MFUGetAttributes(char *,int *,mulong *,long *,int *,int *,mbool_t *,mbool_t *);
int MFUGetFullPath(const char *,char **,char *,char *,mbool_t,char *,int);
int MFUAcquireLock(char *,int *,const char *);
int MFUReleaseLock(char *,int *,char *);
int MFUWriteFile(mjob_t  *,char *,char *,const char *,char *Header,mbool_t,mbool_t,char *);
 



/* trace object */

int MTraceLoadWorkload(char *,int *,mjob_t *,enum MSchedModeEnum);
int MTraceLoadResource(char *,int *,mnode_t *);
int MTraceLoadComputeNode(char *,mnode_t *);
int MTraceGetWorkloadVersion(char *,int *);
int MTraceGetResourceVersion(char *,int *);
int MTraceBuildResource(mnode_t *,mstring_t *);
int MTraceBuildVMResource(mvm_t *,mstring_t *);
int MTraceQueryWorkload(long,long,mjobconstraint_t *,mjob_t *,int);
int MTraceGetWorkloadMatch(mevent_itr_t *,mjobconstraint_t *,mjob_t *,int *,int);
int MTraceCreateJob(char *,int);



/* ui util object */

int MUISClear(msocket_t *);
int MUISAddData(msocket_t *,const char *);
int MUISSetStatus(msocket_t *,int,int,char *);
int MUISInitializeResponse(msocket_t *);
int MUIInitHandlers(void);



/* client object */

int MClientLoadConfig(mccfg_t *,char *);
int MClientProcessConfig(mccfg_t *,char *,mbool_t);
int MCDoCommand(enum MSvcEnum,mccfg_t *,const char *,char **,char *,msocket_t *);
int MCSendRequest(msocket_t *,char *);
int MCInitialize(mccfg_t *,msched_t *,char *,int,mbool_t,mbool_t);
int MCLoadEnvironment(char *,char *,int *);
int MCLoadConfig(mccfg_t *,char *,char *);
int MCProcessGeneralArgs(enum MSvcEnum,char **,int *,mxml_t *);
int MCSchedCtl(void *,int);
int MCRMCtl(void *,int);
int MCSubmit(void *,int);
int MCSubmitInsertArgs(char **,char **,int *);
int MCSubmitFilter(char *,char *,mbool_t,char *); 
int MCSubmitAddRMXString(const char *,const char *,char *,int,mbool_t);

int MCGetEtcDir(char *,int);
int MCGetVarDir(char *,int);
int MCGetBuildHost(char *,int);
int MCGetBuildDate(char *,int);
int MCGetBuildDir(char *,int);
int MCGetBuildArgs(char *,int);



/* local interface */

int MLocalInitialize(void);
int MLocalNodeFilter(mjob_t *,mnode_t *,long);
int MLocalNodeGetPriority(mnode_t *,const mjob_t *,double *);
int MLocalJobAllocateResources(mjob_t *,mreq_t *,mnl_t *,int,int *,int *,char *,int,int *,mnl_t **,int *,int *,mulong);
int MLocalJobCheckNRes(const mjob_t *,const mnode_t *,long);
int MLocalJobDistributeTasks(mjob_t *,mrm_t *,mnl_t *,int *,int);
int MLocalNodeInit(mnode_t *);
int MLocalJobInit(mjob_t *);
int MLocalJobPostLoad(mjob_t *);
int MLocalCheckFairnessPolicy(mjob_t *,long,char *);
int MLocalCheckRequirements(mjob_t *,mnode_t *,long);
int MLocalQueueScheduleIJobs(int *,mpar_t *);
int MLocalReqCheckResourceMatch(const mjob_t *,const mreq_t *,const mnode_t *,enum MAllocRejEnum *);
int MLocalAdjustActiveJobStats(mjob_t *);
int MLocalJobPreStart(mjob_t *);
int MLocalJobPostStart(mjob_t *);
double MLocalApplyCosts(mulong,int,char *);
int MLocalRsvGetAllocCost(mrsv_t *,mvpcp_t *,double *,mbool_t,mbool_t);



/* other */

int MetaCtlInitialize(char *,char *,char *);
int MetaCtlCommit(char *,char *,char *);
int MetaCtlList(char *,char *,char *);
int MetaCtlQuery(char *,char *,char *);
int MetaCtlRegister(char *,char *,char *);
int MetaCtlRemove(char *,char *,char *);
int MetaCtlSet(char *,char *,char *);
int MetaCtlSubmit(char *,char *,char *);
int MetaStoreCompletedJobInfo(mjob_t *);
int MJobNLFromTL(mjob_t *,mnl_t *,int *,int *);
int MUTLFromNL(mnl_t *,int *,int,int *);
int MUTLFromNodeId(char *,int *,int);



#ifdef MXRES_T
 int MGResFind(char *,int,xres_t **);
#endif /* def MXRES_T */
int MGResInitialize();
int MUGenericGetIndex(enum MExpAttrEnum,char const *,enum MObjectSetModeEnum);



/* DAG */

int MDAGSetAnd(marray_t *,enum MDependEnum,char *,char *,mbitmap_t *);
int MDAGSetOr(marray_t *,enum MDependEnum,char *,char *);
int MDAGCopy(marray_t *,marray_t *);
int MDAGDestroy(marray_t *);
int MDAGCheckMulti(mdepend_t *,mln_t **,int *,mstring_t *,mln_t *);
int MDAGCreate(marray_t *,mdepend_t **);
int MDAGToString(marray_t *,char *,int);
int MDAGCheckMultiWithHT(marray_t *,mdepend_t *,mhash_t **,int *,mbitmap_t *,mstring_t *,mln_t *);


/* general object */

int MPrioConfigShow(mbool_t,mpar_t *,mstring_t *);

/* MAssert */
#include "MAssert.h"
void MAbort();

#include "MUDynamicAttr.h"

int MMInfoQuery(mrm_t *,char *,char *,char *,char *,enum MStatusCodeEnum *);
int MMInitialize(mrm_t *,char *,enum MStatusCodeEnum *);
int MMRsvCreate(mrm_t *,mrsv_t *,mnl_t *);
int __MMReleaseDynamicResources(mrm_t *,char *,char *,enum MFormatModeEnum,char *,char *,enum MStatusCodeEnum *);
int MMModifyDynamicResources(mrm_t *,char *,enum MFormatModeEnum,char *,char *,enum MStatusCodeEnum *); 
int MMProcessVPCResults(mrm_t *,mxml_t *,char *,mrm_t **,char *);
int MMDynamicResourcesLoad(mrm_t *,mxml_t *,char *,mrm_t **,char *);
int MMDynamicResourcesUpdate(mrm_t *,mxml_t *,mrm_t *,char *);
mbool_t MPowerGetPowerStateRM(int*);

int MPowerOffReqInit(mpoweroff_req_t *);
int MPowerOffReqFree(mpoweroff_req_t *);
int MPowerOffSubmitMigrationJobs(mnl_t *,mln_t **,mnode_t **,int *);
mbool_t MJobWorkflowHasStarted(mjob_t *);
int MJobGetNodeSet(const mjob_t *,char **,char **);


/* S3 routines */

int MS3JobSubmit(const char *,mrm_t *,mjob_t **,mbitmap_t *,char *,char *,enum MStatusCodeEnum *);
int MS3JobRename(mrm_t *,char *,char *);
int MS3ProcessSubmitQueue(void);

/* DB routines */

#include "mdbi.h"
int MOCheckpoint(enum MXMLOTypeEnum,void *,mbool_t,char *);
int MORemoveCheckpoint(enum MXMLOTypeEnum,char *,long unsigned int, char *);
int MODBCInitialize(struct mdb_t  *);
int MSQLite3Initialize(struct mdb_t *,const char *);
int MDBModuleInitialize();
int MDBInitialize(mdb_t *,MDBTypeEnum);
int MDBBeginTransaction(mdb_t *,char *);
mbool_t MDBInTransaction(mdb_t *);
int MDBConnect(struct mdb_t  *,char *);
int MDBFree(struct mdb_t  *);
int MDBFreeDPtr(mdb_t **);
int MDBDeleteCP(struct mdb_t *,const char *,const char *,mulong,char *);
int MDBPurgeCP(struct mdb_t  *,mulong,char *);
int MDBQueryInternalJobsCP(struct mdb_t  *,int,marray_t *,char *);
int MDBTestGeneralStatsRange();
int MDBReadGeneralStats(struct mdb_t  *,must_t *,enum MXMLOTypeEnum,char *,mulong,char *); 
int MDBGetGeneralStatsRange(struct mdb_t  *,must_t *,enum MXMLOTypeEnum,char *,int,int,int,char *); 
int MDBReadJobs(mdb_t *,marray_t *,char *,marray_t *,char *);
int MDBReadJobRequests(mdb_t *,marray_t *,mtransjob_t *,char *);
int MDBReadNodes(mdb_t *,marray_t *,char *,marray_t *,char *);
int MODBCTestRunAll();
int MSQLite3TestRunAll();
int MDBTestGeneralStats(void);
int MDBReadNodeStats(struct mdb_t  *,mnust_t *,char *,mulong,char *); 
int MDBReadGeneralStatsRange(struct mdb_t  *,must_t *,enum MXMLOTypeEnum,char *,int,int,int,char *); 
int MDBReadNodeStatsRange (struct mdb_t  *,mnust_t *,char *,mulong,mulong,mulong,char *);
int MDBGetLastID(mdb_t *,char const *,char const *,int *,char *);
int MDBClearTables(mdb_t *,char *);
int MDBDeleteJobs(mdb_t *,marray_t *,char *);
int MDBDeleteRsvs(mdb_t *,marray_t *,char *);
int MDBDeleteTriggers(mdb_t *,marray_t *,char *);
int MDBDeleteVCs(mdb_t *,marray_t *,char *);
int MDBDeleteRequests(mdb_t *,marray_t *,char *);

/* CACHE routines */

int MCacheReadNodes(marray_t *,marray_t *);
int MCacheReadJobs(marray_t *,marray_t *);
int MCacheReadRsvs(marray_t *,marray_t *);
int MCacheReadTriggers(marray_t *,marray_t *);
int MCacheReadVCs(marray_t *,marray_t *);
int MCacheReadVMs(marray_t *,marray_t *);
int MCacheReadJobsForShowq(mbitmap_t *,mpar_t *,marray_t *,marray_t *,marray_t *,marray_t *,marray_t *,marray_t *,marray_t *);
int MCachePruneCompletedJobs();
int MCacheDeleteTrigger(mtranstrig_t *);
int MCacheNodeLock(mbool_t,mbool_t);
int MCacheJobLock(mbool_t,mbool_t);
int MCacheRsvLock(mbool_t,mbool_t);
int MCacheTriggerLock(mbool_t,mbool_t);
int MCacheVCLock(mbool_t,mbool_t);
int MCacheVMLock(mbool_t,mbool_t);
int MCacheLockAll();
int MCacheSysInitialize();
int MCacheWriteNode(mtransnode_t *);
int MCacheWriteJob(mtransjob_t *);
int MCacheWriteRsv(mtransrsv_t *);
int MCacheWriteTrigger(mtranstrig_t *);
int MCacheWriteVC(mtransvc_t *);
int MCacheWriteVM(mtransvm_t *);
int MCacheRemoveJob(char *);
int MCacheRemoveNode(char *);
int MCacheRemoveVM(char *);
void MCacheSysShutdown();
void MCacheDumpNodes();

/* Web Services routines */

int MWSWriteObjects(mstring_t **,int,mwsinfo_t *);
int MWSWriteObjectsPOST(mstring_t **,int,mwsinfo_t *);
int MWSWriteObjectsMultiPUT(mstring_t **,int,mwsinfo_t *);

int MWSInfoCreate(mwsinfo_t **);
int MWSInfoFree(mwsinfo_t **);

/* DISA routines */
int MDISATest();
int MDISASetDefaults(mam_t *);
int MLocalRsvGetDISAAllocCost(mrsv_t *,mvpcp_t *,double *,mbool_t,mbool_t);

int MUStringToSQLString(mstring_t *,mbool_t);

int MMRsvQueryPostLoad(mxml_t *,mrm_t *,mbool_t);

/* SMP routines */
#include "MSMP.h"

/* Thread Pool routines */

int MTPAddRequest(int,mthread_handler_t,void *);
int MTPCreateThreadPool(int); 
void MTPDestroyThreadPool();

int MGetMacAddress(char *);

/* History routines */

int MHistoryAddEvent(void *,enum MXMLOTypeEnum);
int MHistoryToString(void *,enum MXMLOTypeEnum,mstring_t *);
int MHistoryToXML(void *,enum MXMLOTypeEnum,mxml_t **);
int MHistoryFromXML(void *,enum MXMLOTypeEnum,mxml_t *);

int MSNLInit(msnl_t *);
int MSNLToMString(msnl_t *,msnl_t *,const char *,mstring_t *,int);
int MSNLFromString(msnl_t *,const char *,const char *,mrm_t *);
int MSNLClear(msnl_t *);
int MSNLFree(msnl_t *);

/* PAL routines */
void MPALClear(unsigned char *);
void MPALAddPartition(unsigned char *,mpar_t *);

/* VC routines */
int MVCAllocate(mvc_t **,const char *,const char *);
int MVCFind(const char *,mvc_t **);
int MVCRemove(mvc_t **,mbool_t,mbool_t,const char *);
int MVCFree(mvc_t **);
mbool_t MVCIsEmpty(mvc_t *);
int MVCAddObjectByName(mvc_t *,const char *,enum MXMLOTypeEnum,mgcred_t *);
int MVCRemoveObjectByName(mvc_t *,const char *,enum MXMLOTypeEnum);
int MVCAddObject(mvc_t *,void *,enum MXMLOTypeEnum);
int MVCRemoveObject(mvc_t *,void *,enum MXMLOTypeEnum);
int MVCRemoveObjectParentVCs(void *,enum MXMLOTypeEnum);
int MVCAToMString(mvc_t *,enum MVCAttrEnum,mstring_t *);
int MVCSetAttr(mvc_t *,enum MVCAttrEnum,void **,enum MDataFormatEnum,enum MObjectSetModeEnum);
int MVCToXML(mvc_t *,mxml_t *,enum MVCAttrEnum *,mbool_t);
int MVCFromXML(mvc_t *,mxml_t *);
int MVCFromString(mvc_t *,char *);
int MVCFreeAll();
mbool_t MVCHasObject(mvc_t *,const char *,enum MXMLOTypeEnum);
mbool_t MVCCredIsOwner(mvc_t *,char *,mgcred_t *);
int MVCAddVarRecursive(mvc_t *,const char *,const char *,mbool_t,mbool_t,mbool_t);
int MVCSearchForVar(mvc_t *,const char *,mln_t **,mbool_t,mbool_t); 
int MVCVarNameFits(const char *,const char *);
int MVCAddVarsToLLRecursive(mvc_t *,mln_t **,const char *);
mbool_t MVCTransitionUserHasAccess(mtransvc_t *,mgcred_t *);
mbool_t MVCUserHasAccess(mvc_t *,mgcred_t *);
mbool_t MVCUserHasAccessToObject(mgcred_t *,void *,enum MXMLOTypeEnum);
mbool_t MVCIsInVCChain(mvc_t *,mvc_t *);
int MVCSetDynamicAttr(mvc_t *,enum MVCAttrEnum,void **,enum MDataFormatEnum);
int MVCGetDynamicAttr(mvc_t *,enum MVCAttrEnum,void **,enum MDataFormatEnum);
int MVCSetNodeSet(mvc_t *,int);
int MVCGetNodeSet(const mjob_t *,char **);
int MVCHarvestVCs();
int MVCExecuteAction(mvc_t *,enum MVCExecuteEnum,mstring_t *,enum MFormatModeEnum);
int MVCJointScheduleJobs(mvc_t *,mjob_t **);
int MVCGetGuaranteedStartTime(mvc_t *,mulong *,mstring_t *);
int MVCGetJobWorkflow(mjob_t *,mvc_t **);
int MVCGrabVarsInHierarchy(mln_t *,mhash_t *,mbool_t);
int MVCGMetricHarvestAverageUsage(mvc_t *,const char *,double *,char *);
int MVCStatClearUsage(mbool_t,mbool_t,mbool_t);
mbool_t MVCIsContainerHold(mvc_t *);

/* VM Migration */
int PlanVMMigrations(enum MVMMigrationPolicyEnum,mbool_t,mln_t **);
int PerformVMMigrations(mln_t **,int *,const char *);
int FreeVMMigrationUsage(mvm_migrate_usage_t **);
int CalculateMaxVMMigrations(mbool_t);
int CalculateNodeAndVMLoads(mhash_t *,mhash_t *);
int MVMGetCurrentDestination(mvm_t *,mln_t **,mnode_t **);
int AddVMMigrationUsage(mvm_migrate_usage_t *,mvm_migrate_usage_t *);
int SubtractVMMigrationUsage(mvm_migrate_usage_t *,mvm_migrate_usage_t *);
int FreeVMMigrationDecisions(mln_t **);
int QueueVMMigrate(mvm_t *,mnode_t *,enum MVMMigrationPolicyEnum,int *,mln_t **);
mbool_t NodeAllowsVMMigration(mnode_t *);
int SetupVMMigrationPolicies();
int NodesHashToList(mhash_t *,mnode_migrate_t ***,int *);
mbool_t NodeIsOvercommitted(mnode_migrate_t *,mbool_t);
int GetVMsMigratingToNode(mnode_t *,mln_t **,mbool_t,mln_t **);
mbool_t VMIsEligibleForMigration(mvm_t *,mbool_t);

int VMFindDestination(mvm_t *,mvm_migrate_policy_if_t *,mhash_t *,mhash_t *,mln_t **,mstring_t *,mnode_t **);
int VMFindDestinationSingleRun(mvm_t *,mstring_t *,mnode_t **);

/* VM Migration - Common policy functions */
int OrderNodesLowToHighLoad(mnode_migrate_t **,mnode_migrate_t **);
int OrderNodesHighToLowLoad(mnode_migrate_t **,mnode_migrate_t **);
int OrderVMsBigToSmall(mvm_migrate_t **,mvm_migrate_t **);
int OrderVMsSmallToBig(mvm_migrate_t **,mvm_migrate_t **);

/* VM Migration - Overcommit */
int VMOvercommitMigrationSetup(mvm_migrate_policy_if_t **);
int OrderNodesOvercommit(mnode_migrate_t **,mnode_migrate_t **);
int OrderVMsOvercommit(mvm_migrate_t **,mvm_migrate_t **);
int OrderDestinationsOvercommit(mnode_migrate_t **,mnode_migrate_t **);
mbool_t MigrationsDoneOvercommit(mhash_t *,mhash_t *,mnode_migrate_t *);
mbool_t MigrationsDoneForNodeOvercommit(mhash_t *,mhash_t *,mnode_migrate_t *,mvm_migrate_t *);
int NodeSetupOvercommit(mhash_t *,mhash_t *,mnode_migrate_t *,mln_t **);
int NodeTearDownOvercommit(mhash_t *,mhash_t *,mnode_migrate_t *,mln_t **);
mbool_t NodeCanBeDestinationOvercommit(mhash_t *,mhash_t *,mln_t **,mnode_migrate_t *,mvm_t *);

/* VM Migration - Consolidation Overcommit */
int VMConsolidationOvercommitMigrationSetup(mvm_migrate_policy_if_t **);
int OrderNodesConsolidationOvercommit(mnode_migrate_t **,mnode_migrate_t **);
int OrderVMsConsolidationOvercommit(mvm_migrate_t **,mvm_migrate_t **);
int OrderDestinationsConsolidationOvercommit(mnode_migrate_t **,mnode_migrate_t **);
mbool_t MigrationsDoneConsolidationOvercommit(mhash_t *,mhash_t *,mnode_migrate_t *);
mbool_t MigrationsDoneForNodeConsolidationOvercommit(mhash_t *,mhash_t *,mnode_migrate_t *,mvm_migrate_t *);
int NodeSetupConsolidationOvercommit(mhash_t *,mhash_t *,mnode_migrate_t *,mln_t **);
int NodeTearDownConsolidationOvercommit(mhash_t *,mhash_t *,mnode_migrate_t *,mln_t **);
mbool_t NodeCanBeDestinationConsolidationOvercommit(mhash_t *,mhash_t *,mln_t **,mnode_migrate_t *,mvm_t *);

/* VM Migration - Consolidation */
int VMConsolidationMigrationSetup(mvm_migrate_policy_if_t **);
int OrderNodesConsolidation(mnode_migrate_t **,mnode_migrate_t **);
int OrderVMsConsolidation(mvm_migrate_t **,mvm_migrate_t **);
int OrderDestinationsConsolidation(mnode_migrate_t **,mnode_migrate_t **);
mbool_t MigrationsDoneConsolidation(mhash_t *,mhash_t *,mnode_migrate_t *);
mbool_t MigrationsDoneForNodeConsolidation(mhash_t *,mhash_t *,mnode_migrate_t *,mvm_migrate_t *);
int NodeSetupConsolidation(mhash_t *,mhash_t *,mnode_migrate_t *,mln_t **);
int NodeTearDownConsolidation(mhash_t *,mhash_t *,mnode_migrate_t *,mln_t **);
mbool_t NodeCanBeDestinationConsolidation(mhash_t *,mhash_t *,mln_t **,mnode_migrate_t *,mvm_t *);

/* Evac VMs */
int MNodeEvacVMs(mnode_t *,mhash_t *,mhash_t *);
int MNodeEvacVMsMultiple(const mnl_t *);

/* MGlobals */
void MGlobalVariablesDestructor();

/* END moab-proto.h */

/* vim: set ts=2 sw=2 expandtab nocindent filetype=c: */

#endif /*  __MOAB_PROTO_H__ */
