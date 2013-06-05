/* HEADER */

#ifndef __MOAB_GLOBAL_H__
#define __MOAB_GLOBAL_H__


extern mccfg_t          C;             /* Configuration information container */

/* MAM */
extern mbool_t          MAMTestMode;

extern mlog_t           mlog;
extern mmutex_t         MLogMutex;
extern char             LogFile[];

extern mnode_t        **MNode;
extern int              MNodeSize;
extern mnode_t         *MTNode[];
extern int             *MNodeIndex;  /* one of <NODEINDEX>, MCONST_NHCLEARED, MCONST_NHNEVERSET */

extern JobTable         MJobHT;
extern JobTable         MCJobHT;

extern mln_t           *MJobsReadyToRun;
extern marray_t         MTJob;       /* job templates */
extern mjtmatch_t      *MJTMatch[];  /* job template match info */

extern mnlbucket_t     *MNLProvBucket[];
extern int              MNLProvBucketSize;

extern marray_t         MSMPNodes;

extern mhash_t          MUserHT;
extern mhash_t          MGroupHT;         /* MGroup Hash Table. */
extern mhash_t          MAcctHT;
extern RsvTable         MRsvHT;
extern mrsv_t          *MRsvProf[];
extern marray_t         MSRsv;
extern mhash_t          MJobDRMJIDHT;
extern mhash_t          MNodeIdHT;       /* hash table initialized if ??? */
extern mhash_t          MNetworkAliasesHT; /* elements are alloc'd mvoid_t, node and vm aliases */
extern mhash_t          MVMIDHT;           /* Requested/used VMIDs (used by job template VMs) */

extern mam_t            MAM[];
extern midm_t           MID[];

extern marray_t         MVPCP;

extern mrange_t         MRange[];
extern int              MRangeSize;
extern mrclass_t        MRClass[];   /* available resource classes */
extern int              MRClassSize;
extern mre_t            MRE[];
extern int              MRESize;
extern mdb_t            MDBFakeHandle; /* used as a stub handle to avoid NULL checks */
extern mhash_t          MDBHandles;
extern mmutex_t         MDBHandleHashLock;

extern mqos_t           MQOS[];
extern msched_t         MSched;
extern mrack_t          MRack[];
extern mpar_t           MPar[];
extern int              MParSize;
extern mnlbucket_t     *MNLProvBucket[];
extern mckpt_t          MCP;
extern mclass_t         MClass[];
extern mgrestable_t     MGRes;       /* generic resource structure */

extern marray_t         MVPC;
extern mrack_t          MRack[];
extern mattrlist_t      MAList;               /* dynamic scheduling attributes */
extern mstat_t          MStat;
extern mpsi_t           MPeer[];
extern mjob_t          *MJobTraceBuffer;

extern mrm_t            MRM[];
extern int              MRMSize;
extern mrmfunc_t        MRMFunc[];
extern enum MRMTypeEnum MDefRMLanguage;

extern msim_t           MSim;
extern msys_t           MSys;                   /* cluster layout */
extern marray_t         MTList;                 /* stores pointers to the trigger objects (ie. elementsize=sizeof(mtrig_t *)) */
extern mhash_t          MTPostActions;          /* holds index of trigger names and their corresponding post actions */
extern mhash_t          MTSpecNameHash;         /* global trigger index (for faster MTrigFinds--holds user specified (T->Name) names ONLY) */
extern mhash_t          MTUniqueNameHash;       /* global trigger index (for faster MTrigFinds--holds unique T->TName's ONLY) */
extern mhash_t          MVC;                    /* global hash table of VCs */
extern mln_t           *MVCToHarvest;           /* global list of VCs to be harvested */

extern m64_t            M64;

extern mgmetrictable_t  MGMetric;       /* generic metrics structure */

extern mbool_t          InHardPolicySchedulingCycle;
extern mbool_t          HardPolicyRsvIsActive;

extern char            *MGlobalReqBuf;   /* holds request response buffers as they are being generated */

extern int              MGlobalReqBufSize;
extern mbool_t          MIncrGlobalReqBuf;

extern mhash_t          MAQ;
extern marray_t         MSRsv;

extern const char      *MParam[];

extern char            *MGlobalReqBuf;
extern int              MGlobalReqBufSize;
                    
extern char            *MPalette[];

extern muis_t           MUI[];
extern msocket_t        MClS[];

extern int              MSUClientCount;

extern char            *MSAN[];

extern mhash_t          TrigPIDs;

extern msubjournal_t    MSubJournal;

extern mre_t            MRE[];

/* Strings/structures for holding non-blocking call buffers */
extern mstring_t        NonBlockMDiagS;
extern mxml_t          *NonBlockShowqN;

/* allocated by MSysAllocJobTable() */

extern mjob_t         **MFQ;          /* terminated by '-1' */
extern mjob_t         **MUIQ;         /* priority sorted list of eligible idle jobs, 
                                                                    terminated w/-1 */
extern mjob_t         **MUILastQ;
extern mjob_t         **MGlobalSQ;
extern mjob_t         **MGlobalHQ;

/* END allocated by MSysAllocJobTable() */


extern long             MCREndTime;
extern char             MCRHostName[];

extern muistat_t        MUIStat[];

extern char             CurrentHostName[];

extern mulong           PendingJobCounter; /* used by comm. thread to keep track of PENDING job submission names */
                                           /* (the name created by this counter is only a temporary placeholder */

extern mbool_t          MSysTestSubmit;

extern enum MDataFormatEnum    MStatDataType[];

/* Thread-safety mutexes */
extern mmutex_t         EUIDMutex;
extern mmutex_t         MSUClientCountMutex;

/* UI globals */

extern long             MUIDeadLine;
extern long             MUITrigDeadLine;
extern mln_t           *MUITransactions;  /* keeps track of non-blocking peer request transactions */

extern char             UserDirective[];
extern mbool_t          UserDirectiveSpecified;

/* END UI globals */
#endif  /*  __MOAB_GLOBAL_H__ */
