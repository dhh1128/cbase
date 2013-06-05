/* HEADER */

/**
 * @file MReq.h
 *
 */

#ifndef  __MREQ_H__   
#define  __MREQ_H__   

extern int MReqDestroy(mreq_t **);
extern int MReqOptReqFree(moptreq_t **);
extern int MReqSetAttr(mjob_t *,mreq_t *,enum MReqAttrEnum,void **,enum MDataFormatEnum,enum MObjectSetModeEnum);
extern int MReqSetImage(mjob_t *,mreq_t *,int);
extern int MReqAttrFromString(ReqAttrArray *,const char *);
extern int MReqAttrFromXML(mjob_t *,mreq_t *,mxml_t *);
extern int MReqAttrToString(ReqAttrArray &,mstring_t *);
extern int MReqAttrToXML(ReqAttrArray &,mxml_t *);
extern int MReqCreate(mjob_t *,const mreq_t *,mreq_t **,mbool_t);
extern int MReqRResFromString(mjob_t *,mreq_t *,char *,int,int);
extern int MReqGetPref(mreq_t *,mnode_t *,mbool_t *);
extern int MReqAllocateLocalRes(mjob_t *,mreq_t *);
extern int MReqAToMString(mjob_t *,mreq_t *,enum MReqAttrEnum,mstring_t *,enum MFormatModeEnum);
extern int MReqToXML(mjob_t *,mreq_t *,mxml_t *,enum MReqAttrEnum *,char *);
extern int MReqFromXML(mjob_t *,mreq_t *,mxml_t *);
extern int MReqDuplicate(mreq_t *,const mreq_t *);
extern int MReqCheckFeatures(const mbitmap_t *,const mreq_t *);
extern int MReqGetReqOpsys(const mjob_t *,int*);
extern int MReqInitXLoad(mreq_t *);
extern int MReqFreeNestedNodeSet(mln_t **);
extern int MReqRemoveFromJob(mjob_t *,int);
extern int MReqCheckResourceMatch(const mjob_t *,const mreq_t *,const mnode_t *,enum MAllocRejEnum *,long,mbool_t,const mpar_t *,char *,char *);
extern int MReqCheckNRes(const mjob_t *,const mnode_t *,const mreq_t *,long,int *,double,int,enum MAllocRejEnum *,char *,long *,mbool_t,mbool_t,char *);
extern int MReqGetPC(mjob_t *,mreq_t *);
extern int MReqGetNBCount(const mjob_t *,const mreq_t *,const mnode_t *);
extern int MReqCmp(const mreq_t *,const mreq_t *);
extern void MReqResourceFromString(mreq_t *,enum MResourceTypeEnum,char *);
extern mbool_t MReqIsSizeZero(mjob_t *,mreq_t *);

/* See MReqTransition.c */
extern int MReqTransitionAllocate(mtransreq_t **);
extern int MReqTransitionFree(void **);
extern int MReqToTransitionStruct(mjob_t *,mreq_t *,mtransreq_t *,mbool_t);
extern int MReqTransitionCopy(mtransreq_t *,mtransreq_t *);

extern int MReqTransitionToXML(mtransreq_t *,mxml_t **,enum MReqAttrEnum *);
extern int __MReqTransitionAToMString(mtransreq_t *,enum MReqAttrEnum,mstring_t *);
extern int MReqSetFBM(mjob_t *,mreq_t *,enum MObjectSetModeEnum,const char *);





/* See MReqFNL.c */
extern int MReqGetFNL(const mjob_t *,const mreq_t *,const mpar_t *,const mnl_t *,mnl_t *,int *,int *,long,mbitmap_t *,char *);


#endif /* __MREQ_H__   */
