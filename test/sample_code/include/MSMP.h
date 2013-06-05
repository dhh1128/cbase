/* HEADER */

/**
 * @file MSMP.h
 *
 * Declares the APIs for job functions that deal with msmp_t structs.
 */

#ifndef __MSMP_H__
#define __MSMP_H__


#define MDEF_MSMPNODECOUNT     10 /* initial number of smp head nodes */

/**
 * @struct msmpnode_t
 *
 * @see mnode_t
 *
 * structure representing smp machine
 */

typedef struct msmpnode_t {
  char    *Name;                  /* Name - ptr to MAList[meNFeature][index] */
  int      Feature;               /* MAttr[meNFeature][index] feature index assigned to smp machine */
  mnode_t *N;                     /* Node representing the smp */
  mcres_t  CRes;                  /* configured resources */
  mcres_t  URes;                  /* utilized resources */
  mcres_t  DRes;                  /* dedicated resources */
  mcres_t  ARes;                  /* available resources */

  int      PtIndex;               /* partition index */
  } msmpnode_t;



msmpnode_t *MSMPNodeFindByFeature(int);
msmpnode_t *MSMPNodeFindByPartition(int);
msmpnode_t *MSMPNodeFindByNode(const mnode_t *N,mbool_t);
msmpnode_t *MSMPNodeCreate(char *,int,const mnode_t *);
int         MSMPNodeInitialize(msmpnode_t *);
int         MSMPNodeResetStats(msmpnode_t *);
int         MSMPNodeResetAllStats();
int         MSMPNodeDestroyAll();
int         MSMPNodeUpdate(mnode_t *);
int         MSMPJobIsFeasible(const mjob_t *,int,mulong);
int         MSMPNodeUpdateWithNL(mjob_t *,mnl_t *);
int         MSMPDiagnose(char *,int);
int         MSMPGetMinLossIndex(mjob_t *,char **,int *,mnl_t *,mulong);
void        MSMPPrintAll();
double      MSMPNodeGetJobUse(const mnode_t *,const mjob_t *,mulong);
mbool_t     MSMPJobIsWithinRLimits(const mjob_t *,msmpnode_t *);


#endif /* __MSMP_H__ */
