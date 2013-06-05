/* HEADER */

/**
 * @file MGlobals.c
 *
 * This is the file contain all the global stateful structures of moab
 */
#include "moab.h"
#include "moab-global.h"



mln_t                  *MJobsReadyToRun;

marray_t                MTJob;                /* job templates */
mjtmatch_t             *MJTMatch[MMAX_TJOB];  /* job template match info */

mnode_t               **MNode;
int                     MNodeSize = sizeof(MNode);

int                    *MNodeIndex; /* (MDEF_MAXNODE << 2) + MMAX_NHBUF;  node index look up table */

mnlbucket_t            *MNLProvBucket[MMAX_ATTR];
int                     MNLProvBucketSize = sizeof(MNLProvBucket);

marray_t                MSMPNodes;

mhash_t		              MUserHT;
mhash_t                 MGroupHT;         /* MGroup Hash Table. */
mhash_t                 MAcctHT;
RsvTable                MRsvHT;
mrsv_t                 *MRsvProf[MMAX_RSVPROFILE];
marray_t                MSRsv;
JobTable                MJobHT;
JobTable                MCJobHT;
mhash_t                 MJobDRMJIDHT;
mhash_t                 MNodeIdHT;       /* hash table initialized if ??? */
mhash_t                 MNetworkAliasesHT; /* elements are alloc'd mvoid_t, node and vm aliases */
mhash_t                 MVMIDHT;           /* hash table of VMIDs */

mam_t                   MAM[MMAX_AM];
midm_t                  MID[MMAX_ID];

marray_t                MVPCP;

mrange_t                MRange[MMAX_RANGE];
int                     MRangeSize = sizeof(MRange);
mrclass_t               MRClass[MAX_MRCLASS];   /* available resource classes */
int                     MRClassSize = sizeof(MRClass);
mdb_t                   MDBFakeHandle; /* used as a stub handle to avoid NULL checks */
mhash_t                 MDBHandles;
mmutex_t                MDBHandleHashLock;



/**
 * MRE is the master reservation event table containing a list of all 
 * reservations within Moab - it does not contain per-node info as is found
 * in the node's N->RE table
 * 
 * @see MSysInitialize() - initialize MRE[] table at startup
 * @see MSysAllocRsvTable() - dynamically re-alloc rsv tables - NOT TESTED
 *
 * @see MPolicyGetEStartTime() - walks MRE[] events to identify when job could 
 *        satisfy all job policies.
          - used in scheduler phase
 * @see MQueueCheckStatus() - walks MRE[] events to locate overlap 'ghost' 
 *        reservations (supports 'IgnIdleRsv' capability)
          - only used at end of UI phase
 * @see MQueueGetTL() - determines job resource usage over time
 *        - only used in UI phase
 * @see MSysGetNextEvent() - reports next future rsv event 
 *        - only used at start of UI phase
 * NOTE:  MRE[] entry created by MREInsert() w/in MRsvCreate() and MRsvJCreate()
 *        MRE[] entry modified w/in MRsvAdjust()
 *        MRE[] entry removed w/in MRsvDestroy()
 */

mclass_t                MClass[MMAX_CLASS + 1];
msched_t                MSched;
mckpt_t                 MCP;
mpar_t                  MPar[MMAX_PAR + 1];  /* Give one slot more for boundary */
int                     MParSize = sizeof(MPar);
marray_t                MVPC;
mrm_t                   MRM[MMAX_RM];
int                     MRMSize = sizeof(MRM);
mqos_t                  MQOS[MMAX_QOS];
mrack_t                 MRack[MMAX_RACK + 1];
mattrlist_t             MAList;               /* dynamic scheduling attributes */
mstat_t                 MStat;
mpsi_t                  MPeer[MMAX_PEER];
mjob_t                 *MJobTraceBuffer = NULL;
mrmfunc_t               MRMFunc[mrmtLAST];
msim_t                  MSim;
msys_t                  MSys;                   /* cluster layout */
marray_t                MTList;                 /* stores pointers to the trigger objects (ie. elementsize=sizeof(mtrig_t *)) */
mhash_t                 MTPostActions;          /* holds index of trigger names and their corresponding post actions */
mhash_t                 MTSpecNameHash;         /* global trigger index (for faster MTrigFinds--holds user specified (T->Name) names ONLY) */
mhash_t                 MTUniqueNameHash;       /* global trigger index (for faster MTrigFinds--holds unique T->TName's ONLY) */
mhash_t                 MVC;                    /* global hash table of VCs */
mln_t                  *MVCToHarvest;           /* list of VCs to harvest (empty) */

m64_t                   M64;

mgrestable_t            MGRes;       /* generic resource structure */

mgmetrictable_t         MGMetric;       /* generic metrics structure */

mbool_t                 InHardPolicySchedulingCycle = FALSE;
mbool_t                 HardPolicyRsvIsActive       = FALSE;

char                   *MGlobalReqBuf = NULL;   /* holds request response buffers as they are 
                                      being generated */

int                     MGlobalReqBufSize = 0;
mbool_t                 MIncrGlobalReqBuf = FALSE;

/* Strings/structures for holding non-blocking call buffers */
mstring_t               NonBlockMDiagS(MMAX_LINE);  /* call the constructor with a default size */

mxml_t                 *NonBlockShowqN = NULL;


/* allocated by MSysAllocJobTable() */

mhash_t                 MAQ;
mjob_t                **MFQ;          /* terminated by '-1' */
mjob_t                **MUIQ;         /* priority sorted list of eligible idle jobs, 
                           terminated w/-1 */
mjob_t                **MUILastQ;
mjob_t                **MGlobalSQ;
mjob_t                **MGlobalHQ;

/* END allocated by MSysAllocJobTable() */

const char                   *MParam[MMAX_CFG];

long                    MCREndTime = 0;
char                    MCRHostName[MMAX_NAME];

muistat_t               MUIStat[mcsLAST];

enum MRMTypeEnum        MDefRMLanguage = mrmtPBS;

char                    CurrentHostName[MMAX_NAME];

mulong                  PendingJobCounter = 0; /* used by comm. thread to keep track of PENDING job submission names */
                                               /* (the name created by this counter is only a temporary placeholder */

mbool_t                 MSysTestSubmit = FALSE;

enum MDataFormatEnum    MStatDataType[mstaLAST];

/* thread-safety mutexes */

mmutex_t                EUIDMutex;   /* prevent threads from trying operations while we are under a different euid */
mmutex_t                MSUClientCountMutex;


/* UI Globals */

long                    MUIDeadLine;
long                    MUITrigDeadLine;
mln_t                  *MUITransactions;  /* keeps track of non-blocking peer request transactions */



/**
 * cleanup routine of any globals that have allocated memory
 * doing will reduce ValGrind noise
 */

void MGlobalVariablesDestructor()
  {
  int i;

  /* clean up mstrings here */
  NonBlockMDiagS.~mstring_t();

  MCP.OldNodeBuffer.~mstring_t();
  MCP.OldJobBuffer.~mstring_t();

  /* Clean up mstrings in the MRM array */

  for(i = 0; i < MMAX_RM; i++)
    {
      MRM[i].ND.QueueQueryData.~mstring_t();
      MRM[i].ND.ResourceQueryData.~mstring_t();
      MRM[i].ND.WorkloadQueryData.~mstring_t();
    }
  }


/* END UI globals */
