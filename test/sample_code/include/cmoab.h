/* HEADER */

/**
 * @file cmoab.h
 *
 *
 */

#if !defined(__CMOAB_H)
#define __CMOAB_H

#include "moab.h"
#include "moab-proto.h"
#include "mclient-proto.h"

#define MAX_MCARGS  128

extern mattrlist_t MAList;

extern mlog_t mlog;

extern msocket_t MClS[];
extern mcfg_t    MCfg[];

extern mclass_t  MClass[];
extern mqos_t    MQOS[];
extern mnode_t  *MNode[];
extern mjob_t   *MJob[];
extern mstat_t   MStat;
extern mpar_t    MPar[];
extern mckpt_t   MCP;
extern mrm_t     MRM[];
extern mrsv_t   *MRes[];         /* terminated with empty name */
extern mrange_t  MRange[];
extern mre_t     MRE[];
extern msrsv_t   SRes[];
extern msrsv_t   OSRes[];
extern mam_t     MAM[];
extern mrack_t   MRack[];
extern msched_t  MSched;

/* globals */

extern mrmfunc_t MRMFunc[mrmtLAST];
extern mjob_t   *MJobTraceBuffer;

/* END globals */

#endif /* __CMOAB_H */

/* END cmoab.h */

