/* HEADER */

/**
 * @file mapi.h
 *
 *
 */

#if !defined(__MAPI_H)
#define __MAPI_H

/* struct declarations */
typedef struct mjob_t mjob_t;
typedef struct mrm_t mrm_t;
typedef struct mnode_t mnode_t;
typedef struct mvm_t mvm_t;
typedef struct mnprio_t mnprio_t;
typedef struct mpar_t mpar_t;
typedef struct mhash_t mhash_t;

#ifndef mbool_t
# define mbool_t unsigned char
#endif /* mbool_t */


/* structs definitions */

/**
 * @struct mnalloc_t
 * Holds a list of nodes, generally used for a task map
 * @param N  A pointer to a mnode_t.
 * @param TC The task count assigned to node N.
 */
/* enums */

enum MJobXAttrEnum {
  mjxaNONE,
  mjxaAllocNodeList,
  mjxaLAST };

enum MNodeXAttrEnum {
  mnxaNONE,
  mnxaIndex,
  mnxaLast };

/* macros */

/* SUCESS can be used to indicate a good operation complete
 * FAILURE can be used to indicate a bad operation complete
 * CONTINUE can be used to indicate all went well, but 'continue' 
 *    in the caller's context without 'returning'
 *    a form of an iterator
 **/

#ifndef CONTINUE
#define CONTINUE             2
#endif

#ifndef SUCCESS
#define SUCCESS              1
#endif

#ifndef FAILURE
#define FAILURE              0
#endif

#ifndef FALSE
#define FALSE 0
#endif /* FALSE */

#ifndef TRUE
#define TRUE 1
#endif /* TRUE */

#ifndef MBNOTSET
#define MBNOTSET 2
#endif /* MBNOTSET */

/* NOTE:  sync MMAX_NAME w/MCMAXNAME in mapi.h */

#ifndef MMAX_NAME
#define MMAX_NAME     64
#endif /* MMAX_NAME */

#ifndef MMAX_LINE
#define MMAX_LINE     1024
#endif /* MMAX_LINE */

#ifndef MMAX_BUFFER
#define MMAX_BUFFER   65536
#endif /* MMAX_BUFFER */

#ifndef MDEF_MAXNODE
# define MDEF_MAXNODE       5120
#endif /* !MDEF_MAXNODE */

#define MCSUCCESS            0    /* sync w/MCCSUCCESS */
#define MCFAILURE           -1    /* sync w/MCCFAILURE */
#define MCMAXLINE            1024 /* sync w/MMAX_LINE */
#define MCMAXNAME            64   /* sync w/MMAX_NAME */

/* prototypes */

/* moab object functions */
int MXJobGetAttr(mjob_t *,enum MJobXAttrEnum,void **);

#if defined(__cplusplus) && !defined(__MOAB_H)
extern "C" int MRMIModuleLoad(void){return(1);};
extern "C" int MRMIModuleList(char * a,int b,char c){return(1);};
#elif !defined(__MOAB_H)
int MRMIModuleLoad(void){return(1);};
int MRMIModuleList(char * a,int b,char c){return(1);};
#endif /* __cplusplus */

/* client functions */
int MCCJobQuery(void **,char *,char **,char *);

int MDBConvertFiles(char *);
#endif /* __MAPI_H */

/* END mapi.h */

