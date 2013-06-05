/* HEADER */

#ifndef __MCRES_H__
#define __MCRES_H__


int MCResAdd(mcres_t *,const mcres_t *,const mcres_t *,int,mbool_t);
int MCResAddDefTimesTC(mcres_t *,mcres_t *,int);
int MCResAddNodeIdleResources(mnode_t *,mcres_t *);
int MCResAddSpec(mcres_t *,const mcres_t *,const mcres_t *,int,mbool_t,mbitmap_t *);
int MCResClear(mcres_t *);
int MCResCmp(const mcres_t *,const mcres_t *);
int MCResCopy(mcres_t *,const mcres_t *);
int MCResCopyBResAndAdjust(mcres_t *,const mcres_t *,const mcres_t *);
int MCResFree(mcres_t *);
int MCResFromString(mcres_t *,char *);
int MCResGetBase(mnode_t *,mcres_t *,mcres_t *);
int MCResGetMax(mcres_t *,mcres_t *,mcres_t *);
int MCResGetMin(mcres_t *,mcres_t *,mcres_t *);
int MCResGetTC(mcres_t *,int,mcres_t *,int *); 
int MCResInit(mcres_t *);
int MCResLowerBound(mcres_t *,mcres_t *);
int MCResMinus(mcres_t *,mcres_t *);
int MCResNormalize(mcres_t *,int *);
int MCResPlus(mcres_t *,const mcres_t *);
int MCResRemove(mcres_t *,const mcres_t *,const mcres_t *,int,mbool_t);
int MCResRemoveSpec(mcres_t *,const mcres_t *,const mcres_t *,int,mbool_t,mbitmap_t *);
int MCResSumGRes(msnl_t *,int,msnl_t *,int ,msnl_t *); 
int MCResTimes(mcres_t *,int);
int MCResToMString(const mcres_t *,long,enum MFormatModeEnum,mstring_t *); 

char *MCResRatioToString(mcres_t *,mcres_t *,mcres_t *,int);
char *MCResToString(const mcres_t *,long,enum MFormatModeEnum,char *);

mbool_t MCResCanOffer(const mcres_t *,const mcres_t *,mbool_t,char *);
mbool_t MCResHasOverlap(mcres_t *,mcres_t *);
mbool_t MCResIsEmpty(mcres_t *);
mbool_t MCResIsNeg(mcres_t *,enum MAllocRejEnum *,const mcres_t *);

#endif  /* __MCRES_H__ */
