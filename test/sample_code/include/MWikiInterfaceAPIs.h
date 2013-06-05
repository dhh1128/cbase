/* HEADER */

#ifndef __MWIKIINTERFACEAPIS_H__
#define __MWIKIINTERFACEAPIS_H__


/* Wiki/Slurm APIs */

int MWikiFromAVP(char *,char *,mstring_t *);
int MWikiGetData(mrm_t *,mbool_t,int,mbool_t *,char *,enum MStatusCodeEnum *);
int MWikiJobLoad(const char *,char *,mjob_t *,mreq_t *,int *,mrm_t *,char *);
int MWikiJobSubmit(const char *,mrm_t *,mjob_t **,mbitmap_t *,char *,char *,enum MStatusCodeEnum *);
int MWikiJobUpdate(const char *,mjob_t *J,mreq_t *,mrm_t *); 
int MWikiJobUpdateAttr(const char *,mjob_t *,mreq_t *,mrm_t *,char *);
int MWikiGEventUpdate(char *,char *,char *);
int MWikiLoadModule(mrmfunc_t *);
int MWikiMBToWikiString(mmb_t *,char *,mstring_t *);
int MWikiNodeApplyAttr(mnode_t *,mnode_t *,mrm_t *,enum MWNodeAttrEnum *);
int MWikiNodeLoad(char *,mnode_t *,mrm_t *,enum MWNodeAttrEnum *);
int MWikiNodeUpdate(char *,mnode_t *,mrm_t *,char *);
int MWikiSDUpdate(char *,msdata_t *,mrm_t *);
int MWikiTestCluster(char *);
int MWikiTestJob(char *);
int MWikiTestNode(char *);
int MWikiVMUpdate(mvm_t *,char *,mrm_t *,char *);


int MSLURMLoadParInfo(mrm_t *);
int MSLURMModifyBGJobNodeCount(mjob_t *,char *,char *);
int MSLURMParseWillRunResponse(mjwrs_t *,mrm_t*,char *,mulong,char *);

#endif /* __MWIKIINTERFACEAPIS_H__ */
