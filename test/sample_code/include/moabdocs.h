/* HEADER */

/**
 * @file moabdocs.h
 *
 * Moab Header
 */

/* This file contains design docs for for moab architecture, infrastructure,
   and functionality
 */

/**
 * Table of Contents
 *
 * Periodic Jobs (PeriodicJobsDoc)
 * Dynamic Job Handling (DynamicJobHandlingDoc)
 * General Credential Stats (GeneralCredentialStatsDoc)
 * Generic Resource Structure (GenericResourceStructureDoc)
 * Green Active Power Management (GreenActivePowerManagementDoc)
 * Hash Table (HashTableDoc)
 * Job Structure (JobStructureDoc)
 * VM Management (VMManagementDoc) - virtual machine management
 * VM Scheduling (VMSchedulingDoc) - job scheduling on virtual machines
 * Regressions tests for NODESETPLUS SPANEVENLY (NodeSetPlusSpanEvenlyDoc)
 * Workflow Graphs (WorkflowGraphsDoc)
 * Job Templates (JobTemplatesDoc)
 * Triggers (TriggersDoc)
 * Grid (GridDoc)
 * Utility Computing (UtilityComputingDoc)
 * Job Dependencies (JobDependenciesDoc)
 * Dynamic Job Handling (DynamicJobHandlingDoc)
 * Checkpoint Infrastructure (CheckpointingInfrastructureDoc)
 * Job RM Extensions (JobRMExtensionsDoc)
 * Reservation Nuances (RsvNuances)
 * Forked Moab Child Process User Interface Handler (ForkedMoabUIHandlerDoc)
 * Multithreaded Client Request Handling (MTClientRequestHandlerDoc)
 * Storing Moab objects in the database and/or cache (ThreadSafeObjectStorageDoc)
 * Generic Job Environment Variables (JobEnvVariablesDoc)
 * Bluegene Preemption (BluegenePreemptionDoc)
 * Reservation Search Algorithm (RsvSearchAlgoDoc)
 * Guaranteed Preemption (GuaranteedPreemptionDoc)
 * Job Preemption and per-task/per-node job restraints (PreemptionTPNDoc)
 * Job Preemption (Old Style, MJobSelectPJobList)
 * Job Preemption (New Style, MJobSelectPreemptees)
 * System Jobs (SystemJobDoc) - system job dynamic resource modification
 * Job Scheduling (JobSchedulingDoc) - job scheduling life cycle
 * Allocation Management (AllocationManagementDoc)
 * Active Queue (MAQ) Table (MAQDoc)
 * Dynamic Provisioning (DynProvDoc)
 * Dedicated Resources (DedResDoc)
 * VLAN Management (VLANDoc)
 * SLA Management (SLADoc)
 * Learning (LearnDoc)
 * Calendaring (CalDoc)
 * Moab HA Lock Files (MoabHALockFile)
 * Moab HA Checkpoint/Journaling System (MoabJournalSystem)
 * MultiRM Management (RMDoc)
 * Storage Management (StorageDoc)
 * Two-stage Provisioning (2StageProvision)
 * Sparse Numbered List (SNLDoc)
 * Resource Overcommit Management (OCDoc)
 * Event Reporting (EventReportDoc)
 * Placeholder reservations (PlacerholderRsvDoc)
 * Similation (SimDoc)
 * Generic Events (GEventDoc)
 * Reservation/SystemJob and job templates (RsvSysJobTemplates)
 * Client Failure Reporting (ClientFailReport)
 * Native Accounting Manager Interface (NAMI)
 * ODBC Data Storage
 * Generic System Jobs (GenericSyJob)
 * GPUs (GPUDoc)
 * Job Arrays  (JobArraysDoc)
 * Environment Record Separtor (EnvRSDoc) 
 * Next To Run Jobs (NextToRun)
 * VM Migration (VMMigration) 
 */



/* DESIGN DOC TEMPLATE */

/* First, come up with a name/title for the
 * design doc. Once this is decided on, create a #define
 * directly before the commented body. This #define will be used
 * to quickly jump to the design doc using ctags or other IDE capabilities.
 * When referencing the design doc this #define can then be used for quick
 * "linking". (See JobDependenciesDoc for an example.)
 *
 * Next, start the commented body and create a "@designdoc <TITLE>" Doxygen tag. After
 * that you can follow this rough outline of content: */

/* 1.0 Overview
     1.1 Feature/Concept Overview (what it does, why it matters)
     1.2 Brief Architecture Overview (how it is implemented)
   2.0 Architecture
     2.1 Functional Flow (Review of key functions)
     2.2 Data Structure Overview
     2.3 Functional List (list of all feature functions)
     2.4 Limitations/Warnings
   3.0 Usage
     3.1 Required Parameters
     3.2 End-User Usage
     3.3 Reporting
     3.4 Dynamic Modification
     3.5 Checkpointing
     3.6 Examples
   4.0 Next Steps
   5.0 Related Objects
     5.1 Design Docs
     5.2 Key Developers
     5.3 Interested Parties
     5.4 User Documentation
     5.5 Other
   6.0 Test Plan
   7.0 ChangeLog (optional)
*/


#define PeriodicJobsDoc

/**
 @designdoc Periodic Jobs

 Jobs must be submitted with msub (msub -l period={minute|hour|day|week}[+<timeoffset>])
  - This creates a private template that matches the submitted job
  - Creates a SystemJob of type "Periodic"
   * SystemJob will create reoccurring reservations
   * SystemJob will put a trigger onto the reservation that submits a job

 J->System->SRsv is an marray_t structure that holds the various standing reservations

 MJobProcessExtensionString() 
  - MCalFromString() - modifies J->Cal
  - MTJobAdd() - creates 'private' job template, sets 
     JT->TX->TemplateFlags += mtjfPrivate,mtjfTemplateIsSelectable
  - MSysJobCreatePeriodic() - convert batch job into system job
 MRMWorkloadQuery()
  - MJobUpdateSystemJob()
    - MSysJobUpdatePeriodic()
      - MSRIterationRefresh()
        - MRsvConfigure()
           NOTE:  rsv has trigger to create compute job

 Required moab.cfg
 --------
 #moab.cfg
 ...
 -------
 
 Example:
 > echo "hostname" | msub -l nodes=1,walltime=10,period=minute+10
 
 */



#define DynamicJobHandlingDoc
/**
 * @designdoc Dynamic Job Handling

 1.0 Overview
 1.1 Feature/Concept Overview (what it does, why it matters)

  Moab's dynamic job feature allow jobs to dynamic modify their allocation over
  time based on policies, competing working, and changing resource conditions.
  Policies may cause allocation modification based on per-application SLA's,
  calendars, and allocated resource health and performance.  This allows the
  application to respond to its environment and guarantee performance to the
  embedded service.

 1.2 Brief Architecture Overview (how it is implemented)

  When a dynamic job requires reduction, an MRMJobModify() request is sent with
  the desired nodelist.  This request can either return reporting that the
  modification completed immediately, or that the request was received but is
  pending (indicated with the string 'pending' in the request result.)  If
  the result is immediate, Moab will adjust the jobs allocated nodelist,
  allocation resource stats, and reservation immediately.  If the request is
  pending, the J->TX structure will be updated with the pending hostlist and
  a pending transaction deadline.

  For the pending situation, each iteration when the RM updates the workload
  data, Moab will determine if the pending hostlist matches the reported
  hostlist and if so, will clear the pending hostlist and deadline.  As long
  as the pending hostlist is set, Moab will not allow the dynamic job to
  receive any further changes and will also prevent the preemptor job from
  preempting any further workload.

 2.0 Architecture
 2.1 Functional Flow (Review of key functions)

 MJobScheduleDynamic() -
   all dynamic jobs are managed through this routine.  For all jobs, this
   routine identifies which resources should be alloc'd or dealloc'd.  For real
   (non-pseudo) jobs, this routine also makes the resource change to the job
   and pushes the change out to the resource manager.

 2.2 Data Structure Overview
 2.3 Functional List (list of all feature functions)
 2.4 Limitations/Warnings
   3.0 Usage
     3.1 Required Parameters
     3.2 End-User Usage
     3.3 Reporting
     3.4 Dynamic Modification
     3.5 Checkpointing
     3.6 Examples
   4.0 Next Steps
   5.0 Related Objects
     5.1 Design Docs
     5.2 Key Developers
     5.3 Interested Parties
   6.0 Test Plan
   7.0 ChangeLog (optional)
*/



#define GeneralCredentialStatsDoc
/**
 * @design General Credential Stats
 *
 * @see struct must_t
 *
 * NOTE: general credential stat structure, use for user, group, account,
 *       class, and qos objects, matrix stats, as well as profile stats and
 *       job template stats
 *       see must_t attributes IStat, IStatCount, and IsProfile
 *
 * NOTE: for new attributes, sync with
 * - enum MStatAttrEnum
 * - MSchedStatToString()    - add to AList[] if appropriate for sched/partition stats
 * - MStatValidateNPData()   - possibly add to exception handling
 * - MCredProfileToXML()     - add to AList[] if appropriate for cred stats
 * - MStatAToString()        - add to msoCred switch
 * - MStatSetAttr()          - add to msoCred switch
 * - MStatPToSummaryString() - add data-type specific handling for summarizing values
 *
 *
 * NOTE: for profile support, sync new attributes with
 * - MStatMerge()/MStatMergeXML()
 * - MStatToXML()            - add to DAList[] w/in mxoCred block (enables checkpointing)
 * - MStatPToSummaryString() - add to SType switch
 *
 * Reporting:
 *  @see env var MOABSTATTEST - force stat roll on specific iteration
 */





#define GenericResourceStructureDoc
/**
 * @designdoc Generic Resource Structure
 *
 * Track action type for consumable generic resources
 *
 * NOTE:  ATime updated if running or queued job requests GRes.
 *
 * @see MGRes[] - global gres structure

 * @see MSched.GResChargeRate[]
 * @see MSched.GResType[]
 * @see parameter GRESCFG (mcoGResCfg,MGResAttr,enum MGResAttrEnum)
 *
 * @see MAList[meGRes][] - sync
 *
 * Attribute 'StartDelay' - if specified, when a job requesting/consuming the
 *                          associated gres is started, all other jobs which
 *                          likewise request/consume the gres will be blocked
 *                          from starting for the specified period of time.
 *                          see MJobStart()->MQueueApplyGResStartDelay()
 *                            apply delay to currently queued idle and blocked
 *                            jobs
 *                          see Step 2.7.1 in MRMJobPostLoad()
 *                            apply delay to newly submitted idle and blocked
 *                            jobs
 */



#define GreenActivePowerManagementDoc
/**
 * @designdoc Green Active Power Management
 *
 * The green active power management facility is responsible for intelligently
 * managing HPC/DC power consumption in a manner which minimizes total power
 * consumed while maximizing responsiveness to workload.
 *
 * This facility is composed of the following parts:
 *
 * - power on-demand (powers off nodes not in use and powers on nodes as needed)
 * - greenpool standing reservation (guarantees a minimum number of idle nodes
 *     will be available for immediate usage, configurable through the moab.cfg
 *     parameter "MAXGREENSTANDBYPOOLSIZE")
 * - green diagnostics (verifies correct configuration of green parameters and
 *     execution of facilities
 * - power capping (contrains organizational and application-level total power
 *     consumption on a per-period basis - NYI)
 * - power tracking (tracks and reports on wattage consumption on a per
 *     organizational and per application basis)
 * - power learning (learns where applications can run best from a power
 *     consumption point of view and dynamically grows towards those servers or
 *     starts subsequent applications on best servers)
 * - power-aware task consolidation (migrates tasks to a minimum number of
 *     physical nodes allowing the 'power on-demand' facility to power of
 *     additional servers)
 * - qos-based power management (powers off servers so long as all application
 *     and organizational SLA's are being satisfied)
 * - thermal-aware scheduling (applies server temperature information to task
 *     placement decisions to maximally balance heat distribution throughout
 *     the data center
 * - thermal-aware migration (detects heat surges and migrates or otherwise
 *     preempts applications associated with the violation keeping dc
 *     temperature within prescribed limits)
  
   @designdoc power on-demand
  
    On-Demand power will power off nodes not in use and will power them on
    again as needed.  This facility operates in close conjunction with the
    greenpool reservation.  To enable this facility, the partition power
    template must be specified, ie 'PARCFG[torque] POWERTEMPLATE=green'
    Setting this parameter causes the mpar_t.PowerJ pseudo-job to be created
    and populated activating on-demand power for that partition.

    The power template job can be customized using standard 'JOBCFG'
    specification to control min/max size, flow control (ALLOCSIZE, ALLOCDELAY),
    resource preference, etc.

    If on-demand power is activated, the MSysSchedulePower() routine will
    allocate all powered off nodes within a given partition to the corresponding
    power job at start up.

    NOTE:  Nodes will only be eligible for active green power management if
           Moab detects these nodes in a powered on status and Moab powers
           these nodes down.  If nodes are powered down manually by an admin,
           Moab will not modify the node's power state.  This feature is to
           prevent Moab from interfering with admin and operator maintenance
           activities.  If a node which has been manually or automatically
           powered down is manually powered on, Moab will add it to the list
           of nodes eligible for automatic power management.

    NOTE:  Because of the way the power management green pool operates, Moab
           will only perform power management on the nodes needed to start up
           to <RESERVATIONDEPTH> jobs at a time.  By default, the parameter
           RESERVATIONDEPTH is set to 1 and so even if many jobs are eligible
           to start, Moab will only power-on nodes adequate to start one job
           at a time.  Adjust this parameter as needed to get the desired
           responsiveness.

    After start up, MSysSchedulePower() calls MJobScheduleDynamic() against
    each valid partition power job to determine what adjustments, if any, are
    needed to the power job based on usage, policies, etc.
    MJobScheduleDynamic() will call MJobPowerSelectAllocNL() and
    MJobPowerSelectDeallocNL() to determine which nodes should change power
    state.

    If Moab determines that a node's power state should be changed, the
    routine MRMNodeSetPowerState() is called which calls MUReadPipe2() to take
    the appropriate action via the native RM NODEPOWERURL attribute.

    Upon successful completion of the NODEPOWERURL action,
    MRMNodeSetPowerState() will set N->PowerSelectState and N->PowerIsEnabled.

    NOTE:  With on-demand power, if the node is powered off, N->State is set
           to mnsIdle to allow continued future scheduling of the node.

    @see N->PowerPolicy - node power policy - must be set to 'green' to
                          enable on demand power
    @see N->PowerState - power state reported by RM - if not reported, will
                         be mpowNONE
    @see N->PowerSelectState - power state explictly requested by Moab - if
                               not set, will be ???
    @see N->PowerIsEnabled

    @see N->State - if N->PowerSelectState == mpowOff and node is in green pool,
                    set state to mnsIdle - see
                    MNodePost{Update,Load}()/MNodeSetAdaptiveState()

    @see MNODEISGREEN()
    @see MRMNodeSetPowerState() - modifies node power state
    @see param PARCFG[] POWERTEMPLATE=X
    @see param RMCFG[] NODEPOWERURL=X
    @see param NODECFG[DEFAULT] POWERPOLICY=GREEN

 * @designdoc greenpool standing reservation
 *    @see param GREENPOOLEVALUATIONINTERVAL
 *    @see param MAXGREENSTANDBYPOOLSIZE
 *    @see MSysSchedulePower()
 *    @see MSysUpdateGreenPoolSize()
 *    @see MJobPowerSelectAllocNL()
 *    @see MJobPowerSelectDeallocNL()
 *    @see MSched.MaxGreenStandbyPoolSize
 *    @see MPar.PowerJ
 *    @see #define MMAX_GREENIDLEACTIVATIONDURATION
 *
 * @designdoc green diagnostics
 *    mdiag -G
 *
 * @designdoc power capping
 *    TBD
 *
 * @designdoc power tracking
 *    @see param "MAXWATTS"
 *    @see param "APPLWATTUSAGE"
 *    @see param "DEFAULTNODEACTIVEWATTS"
 *    @see param "DEFAULTNODEIDLEWATTS"
 *    @see param "DEFAULTNODESTANDBYWATTS"
 *    TBD
 *
 * @designdoc power learning
 *
 *    @see MStatUpdateSoftwareWattUsage()
 *    @see mnapp_t.{WattUsageSum,WattUsageSampleCount}
 *    @see MSched.WattsGMetricIndex
 *
 * @designdoc powermanagement
 *
 * uses msjtPowerOn and msjtPowerOff system jobs
 *
 * N->PowerSelectState:
 *   MRMNodeSetPowerState() - if requested state matches PowerSelectState, take
 *                            no action
 *                          - after action request is successfully sent to RM,
 *                            update PowerSelectState to match request
 *   MSysUpdateGreenPool()  - nodes only added to greenpool if PowerSelectState
 *                            and PowerState are off
 *                          - if VM workload detected on node AND poweroff job
 *                            found on node, remove poweroff job and set
 *                            PowerSelectState to on
 *   MJobUpdateSystemJob()  - when power-on/power-off system job completed, sync
 *                            PowerSelectState
 *   MJobPowerSelectDeallocNL() - if no power on/off system jobs exist and node
 *                            is powered on, set PowerSelectState to on
 *
 * @designdoc power learning
 *    @see MStatUpdateSoftwareWattUsage()
 *    @see N->Stat->ApplWattUsage
 *    @see N->{XLoad,XLimit}->MaxWatts
 *    @see mnpcApplWattUsage
 *    @see mnapp_t
 *
 *    For every job that runs and for each node on which it ran, the job's watt usage is stored as a gmetric statistic.
 *    Thus Moab can track how much work is being accomplished per node, per application.
 *    This information is then used to prioritize the nodes for future jobs of the same application.
 *
 *
 * @designdoc power-aware task consolidation
 *    @see MSched.EnableGreenTaskConsolidation
 *    @see param VMMIGRATIONPOLICY
 *
 * @designdoc qos-based power management
 *    @see Q->GreenXFactorThreshold
 *    @see Q->GreenQTThreshold
 *    @see param GREENBACKLOGTHRESHOLD
 *    @see param GREENQUEUETIMETHRESHOLD
 *    @see param GREENXFACTORTHRESHOLD
 *
 * @designdoc thermal-aware scheduling
 *    TBD
 *
 * @designdoc thermal-aware migration
 *    TBD
 *
 * Modification:
 *
 *   Power state can be manually modified using 'mnodectl -m'
 *
 * Reporting:
 *   Node power on/off events can be recorded using the NODEMODIFY event, ie
 *     'RECORDEVENTLIST +NODEMODIY'
 *   Node power state can be determined using 'checknode -v'
 *   Partition powerjob status can be reported using
 *     'checkjob -v power:<POWERTEMPLATE>.<PARID>'
 *   Power job reservations can be viewed using 'mdiag -r' or 'showres'
 */




#define HashTableDoc
/**
 * @designdoc HashTable
 *
 * Provides dynamic auto-sizing hash table.
 *
 * @see mhash_t - structure

 * @see MUHTAdd() - add entry to hash table
 * @see MUHTGet() - find entry in hash table
 *    (also: MUHTGetInt(),MUHTGetCI() )
 * @see MUHTCreate() - create new hash table
 * @see MUHTFree() - free hash table
 * @see MUHTRemove() - remove entry from hash table
 * @see MUHTToString() - report hash table as string
 * @see MUHTIsInitialized()
 *
 * @see mhashiter_t - peer - structure for iterating through hash entries
 *
 * @see MUHTIterInit() - peer - initialize hash iteration
 * @see MUHTIterate() - peer - traverse hash table
 *
 * Example:
 * 
 *  MUHTCreate(&MSched.VMOSList,-1);
 *  MUHTAdd(&MSched.VMOSList,"linux",NULL,NULL,NULL);
 *  MUHTGet(&MSched.VMOSList,"linux,NULL,NULL);
 *  MUHTDestroy(&MSched.VMOSList);
 */




#define JobStructureDoc
/**
 * @designdoc Job Structure
 *
 * This is the master job structure (mjob_t) that holds all relevant
 * information about a job.  A job maintains a single collection of credentials
 * (J->Cred), a single execution timeframe (J->StartTime,J->WCLimit,...), and
 * multiple resource requirement (mreq_t) structures
 *
 * @see struct mreq_t
 * @see struct msdata_t - for data staging
 *
 * NOTE: when adding an attribute to mjob_t, update the following:
 *  - enum MJobAttrEnum
 *  - const char *MJobAttr[]
 *  - MJobAToString()
 *  - MJobSetAttr()
 *  - MS3JobToXML() - modify DJAList[] array and switch (DJAList[]) - for
 *      grid support
 *  - MJobToExportXML() - modify attribute handling for grid translation
 *
 * 2.2 Data Structure Overview:
 * @see mjob_t
 *   mjob_t.ATime      - ???
 *   mjob_t.UIteration - ???
 */




#define VMManagementDoc
/**
 * @designdoc VM Management
 *
 * 1.0 Overview
 *
 * 1.1 Feature/Concept Overview (what it does, why it matters)
 *
 * Transparently monitor and manage virtual machines with ability to do the
 * following:
 *
 * 1) Allow strict logical partitioning of shared compute resources
 * 2) Allow live migration of virtual machines
 * 3) Allow VM checkpointing (phase II)
 * 4) Allow dynamically resized VM's (phase II)
 * 5) Allow creation of fresh per job VM's (phase II)
 * 6) Allow custom per-job images (phase II)
 *
 * 1.2 Brief Architecture Overview (how it is implemented)
 *
 * @see mvm_t
 * @see mjob_t.VMTaskMap - list of VM's currently allocated to job
 * @see mnode_t.VMList - list of VM's currently associated with physical node
 * @see msched_t.VMList - global VM to physical node mapping table
 *
 * @see MVMStringToPNString() - convert VM string to physical node string
 * @see MVMStringToTL() - convert VM string to task list
 * @see MWikiJobXXX() - convert VM WIKI TaskList attribute to PN
 * @see MJobMapVMs() - map allocated physical nodes to available VM's
 
 Phase I Design:

  If VM mode is used, MSched.ComputeResourcesVirtualized will be set to TRUE.
  Phase I requires that all VM's are statically created and distributed
  across compute nodes prior to Moab startup with static IP addresses and
  with TORQUE pbs_mom daemons active within each image.  These distributed
  images must also be 'balanced' across the nodes with the VM/node count
  distribution that is desired for the duration of Moab operation.  Once
  started, Moab will 'swap' VM's from server to server but will maintain the
  initial VM/node count distribution it detected at start-up.

  xCAT is used to query phyiscal node to virtual node mapping and VM (virtual
  machine) to PN (physical node) mapping information is stored in the
  MSched.VMTable table with the linked list object's generic object pointer
  pointing to the corresponding physical node.

  PN to VM mapping information is stored in each node's N->VMList mln_t table
  where mln_t.Ptr points to a dynamically allocated mvm_t structure.

  xCAT is used to migrate VM's - all migrations consist of a VM 'swap'
  operation so each physical node maintains a consistent VM count matching the
  number of cores present.

  When jobs are started, PN entries in job's TaskMap are mapped to
  corresponding VM's and stored in the job's VMTaskMap table -- see
  MJobStart()->MJobMapVMs().  When the job is started via the RM, the
  VMTaskMap info is passed to the RM start interface -- see MNatJobStart() or
  MPBSJobStart()->__MPBSNLToTaskString().

  When nodes are queried via the RM, all reported node id's will be looked up
  in the MSched.VMTable table.  If the node record is virtual, no action will
  be taken unless the VM has failed.  Nodes which have no entry in the
  MSched.VMTable table will be assumed to be physical nodes and will be loaded
  normally.

  When jobs are queried via the RM, all reported job allocated tasklists will
  be looked up in the MSched.VMTable table, mapped back to physical nodes and
  validated against the expected value.  The 'TC' attribute of the N->VMList
  mln_t list is used to indicate if the VM is currently allocated or is free.
  This information is cleared each iteration within MRMUpdate() and is then
  re-populated as jobs are detected within M*JobUpdate()->MVMStringToTL().

  The N->VMList table is dynamically allocated and is freed when the node is
  destroyed.

  When a migration operation is required, a 'swap' will take
  place consisting of 2 migrate operations.  Migration
  requests should be validated to confirm the destination can
  feasible run the target VM.

 NOTE:  no VM mapping info is checkpointed w/in Moab.  All mapping information
        is restored w/in xCAT and loaded by Moab at startup using the xCAT
        native RM ClusterQueryURL interface.  Specifically, the WIKI node
        object's VARATTR attribute is used to report the VM to physical node
        mapping.

 Phase II Design: Phase II Design introduces the ability to schedule workload
                  to both the physical machine (hypervisor) and the virtual
                  machine.  The preference of the application can be
                  specified within the job or inherited from an associated
                  job template.  In each case, the job attribute
                  'mjob_t.VMUsagePolicy' is set.

 NOTE:  adjust node dedicated resources by resources allocated to any private
        VM w/in J->VMTaskMap (In phase II, all VM's are private, though not
        necessarily single use)

 NOTE:  VMs can only run one job at a time effectively enforcing a 'singlejob'
        VM access policy.  This is enforced in MJobMapVMs() which checks if 
        the VM already has an associated jobid and in MRsvJCreate() which 
        reserves the quantity of resources contained within the VM rather the
        the quantity of resources requested by the task.

  OneTimeUse VM's

    VMCreate workload will create VM's with the mvmfOneTimeUse VM flag set.
    If set, the VM will be destroyed once encapsulated workload completes.

  Starting VMCreate jobs - 
    Once Moab has determined when and where and VMCreate compute job should
    start, it will call MJobStart().

    MJobStart() will call MJobDistributeTasks() to identify exactly how 
    tasks will be mapped and will then call MAdaptiveJobStart() which will
    determine what steps, if any, are required to prepare the allocated
    resources to execute the job.  In the case of VMCreate jobs, this routine
    will create one or more VMCreate system jobs, will associate a dependency
    between each system job and the parent compute job, will start each
    vmcreate system job, and will call MJobTransferAllocation() to import the
    compute job's resource allocation into the newly created system job.

    NOTE: prior to 7/22/09, MVMCanMigrate() reported FALSE for all onetimeuse 
          VM's - we don't know why this contraint existed but it has been 
          removed.

    NOTE:  MAdaptiveJobStart() loops through potentially multiple system
           job children but is only designed to support one.  This should 
           be addressed.

    MJobRemove() will check if J->VMUsagePolicy is set to mvmupVMCreate and if
    so, with remove the associated VM.

    After all vmcreate system jobs are started within this routine via 
    MJobStart(), a job reservation is created for the parent vmcreate 
    compute job covering its full wallclock limit and starting immediately 
    at the end of the vmcreate system jobs.

    Question:  will MJobRemove also remove the VM create system job?

    Once successful completed, the vmcreate system job will be removed
    and inside of MJobRemove(), Moab will again call MJobTransferAllocation()
    to restore the system job's resource allocation back to the compute job
    and its associated reservation.  With the dependencies satisfied and the
    job's resource allocation restored, MJobStartRsv() attempts to validate
    that the required resources are available.  If so, it calls MJobStart()
    to start the vmcreate compute job on the newly created VM's.

  Terminating VMCreate jobs

    NOTE:  If the vmcreate compute job is unable to start within its
           reservation, its associated VM's should be removed, and a job
           hold applied. (NYI)

    NOTE:  VMCreate workload will create VM's with the mvmfOneTimeUse VM flag 
           set.  If set, the VM will be destroyed once encapsulated workload 
           completes.  This occurs w/in the routine ???()

    When VM is no longer needed, MJobDestroyVM() is called.  Currently, this
    routine is called when a job completes and is removed via MJobRemove().
    Further, it is also called when resources are deallocated, ie,
    MJobRequeue(), or if a VMCreate compute job fails to start within the
    allocated VM and has a hold applied.

    MJobDestroyVM() will verify that the job has a VM allocated, that it
    has a VMUsagePolicy of VMCreate, and that the IFlag mjifDestroyDynamicVM
    is set.  If these conditions are met, it will create a VMDestroy job
    for each VM it is destroying.

    Question:  will/should MJobRemove() also remove any associated 
               pending/running VM create system job?

    VM Action Table:

    The VM action table contains a list of system job ids for VM-relevant 
    operations such as VMCreate, VMModify, VMMigrate, and VMDestroy.  It is 
    modified using MUAddAction() and MURemoveAction() and checked using
    MUCheckAction(),

    New actons are added within:
      MUVMMigratePostSubmit() - msjtVMMigrate
      MUGenerateSystemJobs() - msjtOSProvision,msjtOSProvision2,msjtVMCreate,
        msjtPowerOn,msjtPowerOff

    Existing actons are removed within:
      MSysJobProcessCompleted()
      MJobUpdateSystemJob()

    @see N->Action
    
    Y.  Action information is used in X and reported externally using Y.

 Phase III Design:

   Add ability to intelligently manage both physical and VM provisioning simultaneously.
   Support 3 simultaneous VM models:
    - Long-Term Container model - VM holds transient batch workload
    - Single Use Container model - VM is used for single batch/service task and is
       destroyed at workload completion
    - Sovereign Workload model - VM contains its own independent workload not directly
        visible or controllable by moab

    Step 1:  enable single use container model

 2.0 Architecture
 2.1 Functional Flow (Review of key functions)

   MJobStart()
   - MJobDistributeTasks()
   - MJobMapVMs()

   When starting jobs, MJobDistributeTasks() is used to populate J->TaskMap[],
   and then MJobMapVMs() identifies available virtual machines located on the
   corresponding allocated physical machine an allocates these VM's to the
   job.

   MPBSJobUpdate()
   - MVMStringToTL()
   MPBSClusterQuery()
   - if (MSched.ManageTransparentVMs == TRUE)
       if (MUHTGet(&MSched.VMTable) == SUCCESS)
         { <UPDATE VM NODE UTILIZATION INFO> }

   MPBSNodeUpdate()
   - process VM usage

  
   When loading a job's allocated host information from the resource manager,
   the MVMStringToTL() will take VM ids and translate them into an associated
   physical machine taskmap.

   The following call stack allows 'live' migration to be used to migrate a
   dynamic job around failing resources, maintenance reservations, and other
   resource-time obstacles.

  MSchedProcessJobs()
  - MVMScheduleMigration()
    - MVMScheduleGreenConsolidation, 
    - MVMScheduleRsvConflictResolutions, 
    - MVMSchedulePolicyBasedMigrations 
      - MSysScheduleVMMigrate()
        - MVMSelectDestination()
        - MUSubmitVMMigrationJob()

  MVMScheduleMigration() - responsible for evaluating all types of vm migration
                            instantiation

  MVMBuildFutureCResHash() - determines current and planned VM consumption for all 
                            servers (see mhash_t NodeVMFutureCRes in MVM.c)

  MVMScheduleGreenConsolidation() - responsible for evaluating power/temperature 
                            status, identifying possible migration-based 
                            actions, and launching these actions

  MVMScheduleRsvConflictResolutions() - responsible for evaluating reservation 
                            conflict status, identifying possible migration-
                            based actions, and launching these actions

  MVMSchedulePolicyBasedMigrations() - responsible for evaluating resource 
                            overcommit and load-balancing status, identifying 
                            possible migration-based actions, and launching 
                            these actions

  MVMSelectDestination()    responsible for locating potential destination 
                            hypervisors for specified VM

  MJobScheduleDynamic()    - determines if obstacle exists, identifies resources
                             which must be vacated and which resources are
                             suitable replacements
     - MRMJobMigrate()     - passes migration request to correct resource
                             manager
       - MNatJobMigrate()
         - MNatDoCommand()

  __MNatClusterProcessData() - looks at new nodes reported via native interface
    - if MNodeFind() fails, attempt to locate string 'containernode='.  If
    found, set HasParenti, find/add needed VM, and call MWikiVMUpdate() which 
    will then process the containernode attribute.

  VMCreate TimeLine:

  MUGenerateSystemJob() 
    - creates system job 
    - creates V @see MVMAdd()
  MJobStartSystemJob() 
    - starts vmcreate system job
    - calls MTrigLaunchInternalAction() which calls MNatNodeModify()
    - sets mvmfInitializing
  MWikiVMUpdate() 
    - detects VM reported via native interface
    - updates VM attributes
    - updates V->SubState through 'noping', 'ping', and 'pbs'
  MPBSVMUpdate()
    - detects VM reported via TORQUE RM (possibly 'down')
    - detects VM in 'idle' state
    - NOTE:  TORQUE may know of V and report it down for a brief period of time
             during bootstrap - this should not occur with TORQUE auto-discover
  MJobUpdateSystemJob()
    - when V reported as 'up' via TORQUE RM, job is removed and
      mvmfInitializing is unset

  <VM CREATION IS COMPLETE>
    
    
  VMMigration TimeLine:

  Node N1, N2
  VM   VM1

  iteration X
  - VM1 on N1
  - VM1 has VMMap job on N1
  - Request issued to migrate VM1 to node N2
  - VMMigrate job for VM1 created on N2
  iteration X+1
  - VMMigrate job starts, issues VM migrate request to RM - see MJobStartSystemJob()
  iteration X+2
  - RM reports VM associated with N2
  - VMMap job removed from N1 - see MWikiUpdateVM()
  - VMMap job created on N2 - see MWikiUpdateVM()
  - dedicated resources updated on N1, N2
  - VMMigrate job completes - see MJobUpdateSystemJob()
  iteration X+3

 * 2.2 Data Structure Overview

  istruct mvm_t

  Flags
    NOTE:  mvmfInitializing - set in MJobStartSystemJob() when vmcreate system
             job launched - unset in MJobUpdateSystemJob() when vmcreate system 
             job completes (job does not complete until V->RM reports node is up)
    NOTE:  mvmfCreationCompleted - ???

  Substate

  
 * 2.3 Functional List (list of all feature functions)
 * 2.4 Limitations/Warnings
 *
 *   In the Phase I design, there are a number of key limitations.  First, the
 *   usage of each VM must be dedicated - no VM sharing.  Second, each physical
 *   machine (aka hypervisor) should start with and maintain a fixed number of
 *   VM's (typically 1 VM per processor or 1 VM per core).  With the phase I
 *   design, Moab will not rebalance VM's across physical machines but will
 *   preserve the original VM distribution.
 *
 *   Further, it is recommended that a TORQUE MOM daemon or other node monitor
 *   by installed and activated within each hypervisor and each VM.  This
 *   provides resource usage, configuration, and health information about each
 *   server.  In this model, Moab will add a physical machine or a virtual
 *   machine for each server it sees via the node monitor (ie, TORQUE)
 *   resource manager.
 *
 * 3.0 Usage
 * 3.1 Required Parameters
 *
 *  Setting up VM management utilizes the following primary parameters:
 *    RMCFG, HIDEVIRTUALNODES
 *
 *  RMCFG - generally used to define at least 3 resource manager interfaces
 *    RM 1 - master RM used to identify/limit the 'hypervisor' nodes Moab will
 *           manage (ie, native RM using 'file' protocol)
 *    RM 2 - provisioning RM with VM to hypervisor mapping (ie, xCAT)
 *    RM 3 - node status RM reporting health, attribute, usage, etc (ie, TORQUE)
 *  HIDEVIRTUALNODES - set to 'transparent' to indicate that VM's should be
 *    hidden and associated as attributes to the appropriate hypervisors

 * 3.2 End-User Usage
 * 3.3 Reporting
 *  Diagnosing Issues - the following commands are useful in diagnosing issues
 *    associated with this environment
 *    - 'mdiag -R' (verify all RM's are active)
 *    - execute xcat clusterquery url command - NOTE: xcat url command reported
 *      via 'mdiag -R xcat' (verify hypervisor nodes report VARATTRS with
 *      correct VM list)
 *    - 'mdiag -n' (verify hypervisor nodes and only hypervisor nodes show up
 *      with the correct processor counts
 *    - 'checknode -v <NODEID>' (verify the 'VMList' attribute is correctly
 *      populated with the corresponding VM's)
 *
 * 3.4 Dynamic Modification
 * 3.5 Checkpointing
 * 3.6 Examples
 * 3.6.1 Example 1 - Basic Setup
 *   NOTE:  for this example, Moab will need to be built using '--with-rm=torque'
 *
 *   ----------------
 *   # moab.cfg
 *
 *   HIDEVIRTUALNODES  transparent
 *   NODEACCESSPOLICY  shared
 *
 *   RMCFG[master] TYPE=native      CLUSTERQUERYURL=file://$HOME/nodes.txt
 *   RMCFG[xCAT]   TYPE=native:xcat RESOURCETYPE=prov FLAGS=nocreateresource
 *   RMCFG[torque] TYPE=torque
 *   ----------------
   
 @see VMSchedulingDoc

 */





#define VMSchedulingDoc
/**
 * @designdoc VM Scheduling
 *
 * 1.0 Overview
 *
 * This document describes Moab's current scheduling of jobs when the parameter
 * HIDEVIRTUALNODES TRANSPARENT is specified. None, some, or even all of this 
 * information may apply when the parameter is not enabled and VM's exist.
 *
 * 1.1 General job scheduling
 *
 * Moab currently can only schedule a job to run on either VM's or nodes.
 * When the HIDEVIRTUALNODES is set to TRANSPARENT, Moab by default schedules
 * jobs to run on VM's only, though this can be overriden by an explicit VM
 * usage policy. In the future, when no VM usage policy is specified
 * Moab needs to be intelligent enough to decide if a job can run on either
 * VM's or nodes and schedule the job accordingly.
 *
 * Currently only three vm usage policies have meaning in Moab.
 *   1. requirevm: The job should only run on VM's
 *   2. requirepm: The job should only run on nodes
 *   3. createvm: The job will be scheduled the same as a require pm job
 *        and then run on VM's created for the job specifically.
 *
 * 1.2 Other Limitations
 *   - Currently VM's can only run one job at a time (ie, per VM access policy
 *      is singlejob)
 *   - VM scheduling only works well for proc-based tasks. Scheduling may not
 *     work correctly for other types of tasks.
 *   - The on-demand VM needs to be identical in size and duration to the job
 *      that will run on it because of the current limitation that only one
        job can run on a VM at a time, and because createvm jobs and vmmap
        system jobs are used to track VM resource consumption in MJobGetSNRange
 *   - We cannot yet support multi-req requirevm jobs because there is not a 
 *     per-req VMTaskMap to allow us to identify per-req VM dedicated resources
 *
 * 1.3 TODOs
 *   - Change VMTaskMap to be orthogonal to TaskMap and make a VMList orthogonal
 *     to NodeList
 *   - Make VMList per-req

 * 1.4 Future Ideas
 *  - Instead of having special rules for processing the three job types outlined
      in 1.1, add two new reservations types, one for requirevm jobs and one for 
      all other jobs. Create a reservation to a configureable time X on each static 
      VM that only requirevm jobs have access to, and put a reservation 
      on the remaining resources of each node of time X that all other jobs
      have access to. This handles the possibility of VM migrations more cleanly and
      localize specialization code to MRsvCheckJAccess and to resource creation/
      deletion functions, but at a potential performance loss.

 *
 * 2.0 Scheduling Implementation Details
 *
 * 2.1 VM Job Scheduling
 *
 *  Most of Moab's schedule functions only know how to schedule on nodes.
 *  Therefore, jobs that want to run on VM's are scheduled on nodes as any 
 *  other job. Different sections of the code have been modified to recognize
 *  a job that wants VM's and provide appropriate scheduling exceptions.
 *  Such jobs can be recognized with the macro MJOBREQUIRESVMS().
 *  There is also code that checks job components such as J->VMUsagePolicy or
 *  J->VMTaskMap to provide exceptions as well. The following functions contain
 *  scheduling exceptions for jobs to run on VMs. This list may not be
 *  exhaustive.
  
     - MJobGetNodeStateDelay()
       - If J->VMUsagePolicy == mvmupVMCreate, ignore OS mismatches

     - MJobGetINL()
       - If job only wants VMs, compute available tasks based on Node VMs and
         inform MReqCheckNRes to use VM job exceptions as well

     - MReqCheckNRes()
       - if job only wants VMs, compute CRes and ARes based on node VMs

     - MReqCheckResourceMatch()
       - if job only wants PMs and server has non-compute hypervisor and 
         two-stage provisioning is not enabled, reject node
       - if job only wants VMs and server has no hypervisor and two-stage
         provisioning is not enabled, reject node
       - if job only wants VMs, compute CRes based on node VMs

     - MJobGetSNRange()
       - requirevm jobs:
          - Remove all non-static vm resources from AvlRes
          - Ignore all non-requirevm reservations
       - requirepm jobs:
          - Remove all static vm resources (not on-demand vm resources) from AvlRes
          - Ignore all requirevm reservations
       - createvm jobs: same as requirepm jobs
          
     - MNodeGetCfgTime()
       - determine if server, VM, or two-stage provisioning is required and if
         so, how much time will be required to set up the needed environment
    
     - MNodeCanProvision()

     - MCResCanOffer() 
       - reports if config/avail CRes can support request CRes

     - MNodeGetVMRes()
       - ???

     - MJobGetFeasibleNodeResources() 
       - reports resources available for VM/non-VM job

     - MJobStart()
       - calls MJobMapVMs() to identify available VM's located on allocated
         physical servers

     - MRsvJCreate()
       - reserves resources equivalent to the configured resources found within
         the allocated VM's located on the physical server.  (ie, enforces
         dedicated VM allocation by reserving entire VM resource set) 

 * 2.2 Allocating VMs to a job
 *
 *  Jobs currently get VMs allocated to them via MJobMapVMs(). This function is
 *  called by MJobStart() when the job is recognized as needing to run on VMs.
 *  This function uses J->NodeList to find VMs on each node the job is to run
 *  on, effectively replacing the node allocation with an equivalent VM
 *  allocation.
 *
 * 2.3 RM Job Start
 *
 *  Every resource manager library is responsible for recognizing if a job
 *  is to run on VMs and send an appropriate task list to the RM. This most
 *  likely currently only happens with native and TORQUE resource managers. 
 *  Since TORQUE is unaware of VMs, TORQUE must see the VMs as nodes or the 
 *  job start request will fail.
 *
 * 2.4 VM resource usage tracking
 * 
 *   Currently jobs running on VMs have their resources tallied to both 
 *   the dedicated resources (DRes) of the nodes they are running on as well
 *   as of the VMs they are running on. In Phase II, the resources should 
 *   only be tallied against the VMs and the dedicated resources of the VMs
 *   should then tally against the nodes the VMs run on. This will provide
 *   for more accurate resource book-keeping.
 *
 * 3.0 On-Demand VM Job Scheduling
 *  
 *  On-Demand VM jobs, jobs that have the mvmupVMCreate usage policy, are jobs 
 *  that create VMs specifically for their use and ensure those VMs are 
 *  destroyed after they complete. These jobs are scheduled as normal jobs with
 *  a couple of scheduling exceptions (see section 2.1).  The sequence of 
 *  events for this type of job are outlined below.
 *
 *     1. In MJobStart(), when the job is ready to start:
 *      - use MVMCreateReqFromJob to build a list of vm creation requests, one
 *        for each node the job is scheduled to run on as specified in
 *        J->NodeList. The VM should have at least the amount of resources
 *        required by its corresponding NodeList entry. The VM can represent
 *        multiple tasks. I.E., an entry of {node1, 4} in the node list would
 *        mean that the VM would have to have enough resources for four tasks on
 *        node1.
 *      - Start VM-creation jobs and make the original job depend on the
 *        successful completion of these jobs.
 *      - Create a "pin" reservation so that no jobs get scheduled to run
 *        right after the VMCreation jobs' WCTime is over.
 *
 *     2. As each vm creation job completes, in MJobRemove the VM destroy job
 *        fills in an entry into the VMTaskMap of its parent job. If the 
 *        VMTaskMap is completely filled, the parent job is then started 
 *        immediately.
 *
 *     3. When the on-demand VM compute job finishes, MJobRemove() creates a 
 *        vm-destroy job for each VM in the job's VMTaskMap and starts these 
 *        jobs immediately.
 
 @see SystemJobDoc
 @see VMManagementDoc
 */

#define NodeSetDoc

/**
 * @designdoc NodeSets
 *
 * Nodesets are a way to allocated nodes in groups, the most obvious use case is
 * assigning nodes within a single switch when possible to optimize network 
 * latency. 
 *
 * The main routine that determines nodesets is MJobSelectResourceSet(). It is
 * called in the following cases:
 * 
 * MJobGetEStartTime (2 times)
 *  one for each range if ValidateFutureNodeSets is TRUE
 *  called on a per-req basis
 *
 * MReqGetFNL (1 time)
 *  called on a per-req basis to validate FNL
 *  this call is optional
 *  disabled for SpanEvenly jobs
 *
 * MJobAllocMNL() (3 times)
 *  called on a per-req basis
 *  called early on if nodesets are not optional
 *  called later but failure is skipped if nodesets are optional
 *
 * There are various other calls in the code but they are related to specific
 * circumstances like SMP, VLAN, or VM  scheduling.
 *
 * For NestedNodeSets the call in MReqGetFNL is skipped
 *
 * Parameters that configure behavior:
 * SCHEDCFG[] FLAGS=ValidateFutureNodeSets
 *
 */





#define NodeSetPlusSpanEvenlyDoc
/**
 * @designdoc NODESETPLUS SPANEVENLY
 *
 * @see MJobGetEStartTime()
 * @see MJobGetRangeValue()
 * @see mreq_t.SetBlock
 * @see mreq_t.SetDelay
 * @see mreq_t.SetSelection
 * @see mreq_t.SetType
 *
 *  PNNL has a large system that are divided in CUs or computational units.  Jobs perform significantly
 *  better when they are contained within a single CU or when they are balanced evently across CUs.  For
 *  instance, a 4 node job would perform well within a single CU, 2 nodes per CU, or 1 node per CU, but the
 *  job performs poorly if it is not balanced, 1 node in one CU and 3 nodes in another CU.  To give them
 *  this behavior the parameter "NODESETPLUS SPANEVENLY" was developed.  When this parameter is set all
 *  jobs that are submitted (by default) will attempt to span.
 *
 *  The test configuration for PNNL is as follows:
 *
 *  SCHEDCFG[]	      FLAGS=AllowPerJobNodeSetIsOptional
 *  NODESETPLUS       SPANEVENLY
 *  NODESETPOLICY     ANYOF
 *  NODESETDELAY      10:00
 *  NODESETATTRIBUTE  FEATURE
 *  NODESETISOPTIONAL FALSE
 *  NODESETLIST       cu1,cu2,cu3,cu4
 *
 *  In this setup jobs will, by default, seek to run within a single CU or they will span evenly, unless
 *  doing so would delay the start of the job beyond 10 minutes. However, users have the ability to customize
 *  their jobs as they see fit.  Users can change the following attributes of a job:
 *
 *  NODESETDELAY
 *  NODESETLIST
 *  NODESETPOLICY
 *
 *  The scheduling of these jobs is handled in a normal way up to a certain point, after which special code
 *  takes over. To start, Moab will see the job as a normal NODESET job and will attempt to put the job
 *  within a single nodeset.  Should this fail we will then call MJobGetEStartTime() to find the earliest
 *  time when the job can start.  MJobGetEStartTime() has been modified to branch if SPANEVENLY is set.
 *  Before branching it will make its first call to MJobGetRange().  This first call will return a set of
 *  ranges for the job.  At that point Moab will call MJobGetRangeValue() with the possible ranges for the job.
 *  MJobGetRangeValue() is the routine that will determine if any of the valid ranges can fit the job
 *  evenly across nodesets or if doing so will delay the job beyond the job's NODESETDELAY. If the job can fit
 *  within 1 CU or can spanevenly then the allocation for the job is made in MJobGetRangeValue(). A flag is
 *  set on the job to tell later routines that they do not need to allocate nodes for the job. If no allocation
 *  is available within the NODESETDELAY then all NODESET requirements for the job are erased and the job
 *  will simply be scheduled normally by the rest of the Moab functions.
 *
 *  For Backfill, Moab will call MJobGetEStartTime() and see if the earliest time the job can start is now. If
 *  it can then the allocation received via MJobGetEStartTime()->MJobGetRangeValue() is used and the job is
 *  started immediately.
 *
 *  Jobs can customize the nodesetdelay as well as the nodesetpoliy using:
 *
 *  msub -l nodesetdelay=<time>
 *  msub -l nodesetpolicy=ONEOF|ANYOF[:FEATURE:<feature_list>]
 * Possible failure points:
 *
 * shutting down and restarting Moab will cause jobs whose nodeset was erased to be revived
 * this has not been tested with multi-req jobs
 * one bug was already found in MJobGetEStartTime() that caused a job to pick the worst partition rather than the best
 *
 * Testing:
 *
 *  PNNL dedicates the entire node to the job and their testing always involved jobs asking for "ppn=8".
 *
 *  The following will need to be added to a regression test to ensure this behavior does not change:
 *
 *  8 nodes, (2 per CU) 8 cores each
 *
 *  msub -l nodes=2:ppn=8 (job should run on within 1 if available otherwise 2 different CUs)
 *  msub -l nodes=3:ppn=8 (job should run on within 3 different CUs)
 *  msub -l nodes=4:ppn=8 (job should run on within 2 if available otherwise 4 different CUs)
 *  msub -l nodes=6:ppn=8 (job should run on within 3 CUs)
 *  msub -l nodes=8:ppn=8 (job should run on within 4 CUs)
 *
 *  8 nodes, (2 per CU) 8 cores each, 1 node busy in 1st CU, 1 node busy in 2nd CU
 *
 *  msub -l nodes=2:ppn=8 (job should run on within 1 if available otherwise 2 different CUs)
 *  msub -l nodes=3:ppn=8 (job should run on within 3 different CUs)
 *  msub -l nodes=3:ppn=8 (job should run on within 3 different CUs)
 *  msub -l nodes=6:ppn=8 (job should run anywhere if the 2 nodes are busy for longer than NODESETDELAY, otherwise it should reserve 3)
 *  msub -l nodes=2:ppn=8 (job should run within 1 or 2 and should backfill if the 6 node job is waiting)
 *
 *
 *  8 nodes, (2 per CU) 8 cores each, 1 node busy in each CU
 *
 *  msub -l nodes=2:ppn=8 (job should run within 2 different CUs)
 *  msub -l nodes=2:ppn=8,nodeset=oneof (job should run within 1 different CUs if within NODESETDELAY, otherwise it should reserve 1 CU)
 */




/**
 * Dynamic flag routines
 *
 * These routines replace the older MUBM routines that were used for partition bitmaps and QOS bitmaps.
 *
 * These new routines operate on either a 0 offset value (like enums) or a 1 offset value (like MMAX_PAR).
 * It can operate on both of these different types of bitmaps because MBMSIZE will create an extra buffer
 * of 1 byte for each array.  
 *
 * MMBMFlagFromString()
 * MUBMFLAGArrayToString()
 * MUBMFLAGUnSet()
 * MUBMFlagSet()
 * MUBMFLAGISSET()
 * MUBMFLAGClear()
 * MUBMFLAGCopy()
 * MUBMFLAGIsClear()
 * MUBMFLAGOR
 * MUBMFLAGAND
 * MUBMFLAGNOT
 * MUBMFLAGXOR
 */


#define WorkflowGraphsDoc
/**
 * @designdoc Workflow Graphs
 *
 * 1. Overview
 *
 * Moab workflow graphs provide a way for customers to visualize dependencies
 * between jobs in order to better synthesize and manage job workflow scenarios. Moab
 * currently allows to request workflow data through a client command and returns
 * the data in XML format. The format of the XML is as follows,
 * where angle brackets-enclosed identifiers denote variable values:
 *
 *<data>
 *  <Error Type="Cycle" Cycle="<node1>,...,<node1>"/>
 *  <job CompletionTime="<timestamp>" JobID="<ID>" StartTime="<timestamp>" State="<state>" >
 *    <depend JobID="<ID>" type="<type>"/>
 *  </job>
 *</data>
 *
 * Job elements describe jobs and their dependencies, while Error elements
 * describe graph processing errors. There is currently only one type of reported error,
 * resulting from a cycle detection in the graph.
 *
 * 2. Code Design
 *
 * The graph generation code, given a job as input, must construct a directed
 * acyclic graph of all jobs connected the input job either as a dependency (ancestor)
 * or a dependent (descendent). The code must also compute an estimated
 * start time or completion time for each node in the graph. Finally, the code
 * must render the resulting graph into XML format. The functions that implement
 * this feature are:
 *
 * -@__MUIJobQueryWorkflow()
 * -@__MUIJobQueryWorkflowRecursive()
 *
 * The higher level function, __MUIJobQueryWorkflow, initializes an array of
 * graph nodes to be used in constructing the graph and passes this array to
 * __MUIJobQueryWorkflowRecursive along with the initial job to search.
 * Once the recursive function has finished finding all nodes and computing
 * the start time for each node in the graph, __MUIJobQueryWorkflow finishes by
 * converting the array-based graph into XML.
 *
 * The recursive portion of the code can be outlined in several steps as follows:
 *
 * 1. Verify that the node hasn't been processed already.
 * 2. Process all parents recursively.
 * 3. Compute a start time based on the node's job and parents.
 * 4. Process all children recursively.
 *
 * The recursive portion of the code currently handles two exceptions in graph processing:
 * shared ancestry and dependency cycles. The first problem, shared ancestry, is
 * caused when a node shares an ancestor with another ancestor, for instance, if
 * a node has a grandparent as a parent so that it shares one parent, its grandparent,
 * with another parent. When this situation occurs, a node can attempt to process
 * itself at the same time as one of its parents. The function cannot process the
 * parent again, or infinite recursion would result, and it cannot process itself
 * completely without first processing all parents. This situation is handled
 * by returning a special value, MWF_PROCESSING_DEFERRED, signifying that the node
 * and its subgraph of children will be handled later.
 *
 * The second exception, cyclic dependencies, is handled by generating an error
 * element for consumption by the user. The element reports in its "Cycle" attribute
 * all nodes in the cycle in a comma-delimited list, witht he first node in
 * the list repeated at the end of the list.
 */



#define JobTemplatesDoc
/**
 * @designdoc Job Templates
 *
 * Job templates allow workload to be matched and be arbitrarily modified.
 * Further, job templates can be used to create collections of related jobs for
 * statistical reporting and automated learning purposes.
 *
 * @see param JOBCFG[] - parameter used to specify job template attributes
 * @see param JOBMATCHCFG[] - parameter used to create relationships between
 *              job templates
 *
 * Job Template Configuration
 *
 * Call Stack:
 *
 * MSysLoadConfig()
 * - MTJobLoadConfig()
 *   - MTJobProcessConfig()
 *     - MTJobAdd()
 *     - MTJobLoad()
 *         (process template extensions)
 *       - MWikiJobUpdateAttr()
 *         - MTJobProcessAttr()
 *
 * Call Stack for dynamic templates:
 *
 * __MUIJobTemplateCtl()
 * - MTJobProcessConfig()
 *
 * Dynamic job templates use the DTJOB identifier in the checkpoint file and are always
 * loaded. They have the IFlag mjifTemplateIsDynamic.
 *
 * Attributes:
 *
 * Attribute processing is handled within the following routines:
 *  - MTJobLoad() - handles all non-wiki extensions
 *  - MWikiJobUpdateAttr() - handles all wiki attributes
 *  - MTJobProcessAttr() - handles wiki profile extensions
 *
 * NOTE:  all the routines below should be located in the MJobTemplate.c module
 *
 * @see MTJobLoadConfig()
 * @see MTJobProcessConfig()
 * @see MTJobAdd()
 * @see MTJobLoad()
 * @see MTJobMatchLoadConfig()
 * @see MTJobMatchProcessConfig()
 * @see MTJobProcessAttr()
 * @see MWikiJobUpdateAttr()
 * @see MTJobStatToXML()
 * @see MJobCheckJMaxLimits()
 * @see MJobCheckJMinLimits()
 * @see MJobGetTemplate()
 * @see MTJobFind()
 * @see MJobCheckRMTemplates()
 * @see MJobApplySetTemplate()
 * @see MJobApplyDefTemplate()
 * @see MJobApplyUnsetTemplate()
 * @see MJobCreateTemplateDependency()
 *
 * @see MJTXEnum/MJTXAttr[],struct mjtx_t
 *
 * Job Template Matching
 *
 * Use JMin and JMax templates to filter/select which jobs should be associated
 * with the JSet, JUnset, JDef, and JStat templates specified within a jobmatch
 * association.
 *
 * Call Stack:
 *
 * MRMJobPostLoad()
 * - MJobSetDefaults()
 *   - MJobGetTemplate()
 *     - MJobCheckJMaxLimits()
 *     - MJobCheckJMinLimits()
 *   - MJobApplyDefTemplate()
 *   - MJobApplySetTemplate()
 *   - MJobApplyUnsetTemplate()
 *
 *
 * Job Template Attribute Application
 *
 *   TBA
 *
 *
 * Job Template Workflows
 *
 * Generate a full workflow based on a single job submission.  Using the
 * TEMPLATEDEPENDS attributes of the JOBCFG parameter, create needed pre and
 * post req jobs and create the needed dependency associations.
 *
 * Call Stack
 *
 * MRMJobPostLoad()
 * - MJobSetDefaults()
 *   - MJobGetTemplate()
 *     - MJobCheckJMaxLimits()
 *     - MJobCheckJMinLimits()
 *   - MJobApplySetTemplate()
 *     - MJobCreateTemplateDependency()
 *       - MS3JobSubmit()
 *     - MJobSetDependency()
 *
 * Structures
 *
 * @see MJob[] - standard jobs
 * @see marray_t MTJob - template jobs
 * @see mjob_t.TX->Depends - linked list of template dependencies
 *  NOTE:  TX->Depends populated w/in MTJobProcessAttr()
 *
 *
 * Job Template Learning
 *   TODO
 *
 * 5.5 User Documentation
 *   When adding support for additional job attributes for JMIN, JMAX, JDEF,
 *   and JSET templates, update section 11.10 of the moab admin guide
 */



#define TriggersDoc
/**
 * @designdoc Triggers
 *
 * TBA
 *
 * @see mxaTrig - resource manager extension
 * @see MTrigLoadString() - process trig spec
 *
 * @see developer Doug
 *
 * Triggers are created by calling MTrigLoadString().  This will create template triggers
 * and store the actual triggers in the global trigger array MTList.  This routine will
 * mark all newly created triggers as templates and add pointers to these template triggers
 * to the individual object's trigger array.
 *
 * At this point the global trigger table has an array of template triggers and the object
 * has an array of template trigger pointers.
 *
 * What happens next depends on the trigger's object.  If the trigger's object is a real
 * object like an actual reservation (not reservation profile and not standing reservation)
 * or a job (not a job template or a class epilog or prolog) then the triggers are 
 * converted into real triggers by calling MTrigInitialize().  This routine will mark the 
 * triggers as real (the actual triggers in the global trigger table) and verify they are 
 * hooked up to the right object.  At this point the template triggers have been converted
 * into real triggers and there should be no more template triggers in the global trigger 
 * list.
 *
 * If the trigger's object is a template (class prolog/epilog/jobtrigger, rsv profile,
 * standing reservation) then the triggers are never marked as real and MTrigInitialize()
 * is not called.  At the point that a real object is created from these templates then
 * MTListCopy() is called to duplicate the triggers in the global trigger table followed 
 * by MTrigInitialize() to convert the copied triggers into real triggers.  The only
 * template triggers left in the global trigger table are those on the template object.
 *
 *
 * NOTE: MTrigCreate() should rarely be called.  for system jobs.
 *
 * NOTE: Do not call MTrigAdd().  This should only be called by MTrigLoadString() and
 *       MTrigInitialize().
 *
 * NOTE: if you don't want the triggers to be on the global table (as is the case with
 *       prologs and epilogs) you can call MTrigInitialize() with FALSE.  This will
 *       create localized triggers with dummy names (you'll want to replace them). You
 *       are then in charge of freeing all the memory for the trigger (see mtifIsAlloc).
 *
 * Sample code:
 * 1) to add trigger templates to a template object then copy them and make them real for 
 *    another object:
 *
 *    MTrigLoadString(template object's array) -> stores template triggers globally
 *    MTListCopy(real object's array) -> copies template triggers to real object
 *    MTrigInitialize() -> makes the copied, template triggers real
 *
 *    end result: template triggers from template object still on the global array
 *                real triggers from real object also on the global array
 *
 * 2) to add real trigger to a real object:
 *
 *    MTrigLoadString(real object's array) -> stores template triggers globally
 *    MTrigInitialize(real object's array) -> makes the template triggers real
 *
 *    end result: real triggrs from real object on the global array
 *                no template triggers on the global array
 *
 */




#define GridDoc
/**
 * @designdoc Grid
 *
 * TBA
 *
 * @see developer Josh
 */




#define UtilityComputingDoc
/**
 * @designdoc Utility Computing
 *
 * TBA
 *
 * @see developer Doug
 *
 * In Utility Computing, a Moab installation (customer) will communicate with
 * another Moab installation (hosting center) to perform various tasks related
 * to dynamic resources.  These tasks include creating a new set of
 * dynamic resources for the customer to use, increasing or decreasing the
 * amount of dynamic resources available to the customer, or removing the
 * dynamic resources from the customer installation.  These tasks can be
 * performed manually (using mrmctl -x) or automatically through a number of
 * different policies and triggers.
 *
 * The relationship between the hosting center and the customer is established
 * using the moab.cfg file and the moab-private.cfg file:
 *
 * Customer moab.cfg:
 *
 *
 * RMCFG[HC]  TYPE=MOAB FLAGS=hostingCenter SERVER=server:port
 *
 * Customer moab-private.cfg:
 *
 * CLIENTCFG[RM:HC] KEY=privatekey
 *
 * Hosting center moab.cfg:
 *
 * VCPROFILE[basic]        DESCRIPTION="dynamic resources"
 *
 * Hosting center moab-private.cfg:
 *
 * CLIENTCFG[RM:customer]  KEY=privatekey AUTH=admin1
 *
 * This configuration will enable the customer to manually request resources
 * from the hosting center using mrmctl -x.  When the customer asks for resources
 * this is translated into an "mshow -a" command that is sent from the customer
 * Moab to the hosting center Moab.  If the hosting center returns a set of
 * resources that meet the time constraint (if a time constraint is specified)
 * then the customer will request the creation of a VPC on the hosting center
 * using an "mschedctl -c vpc" command.  The hosting center will create the VPC
 * and return the information needed for the customer to automatically establish
 * a relationship with the new resources (host and port at a minimum).
 *
 * The call trace for the manual creation of dynamic resources is:
 *
 * MUIRMCtl()
 * 	MUIRMCtlModifyDynamicResources()
 * 	MMModifyDynamicResources()
 * 		__MMCreateDynamicResources()
 * 			__MMDoCommand() (mshow -a)
 * 			__MMDoCommand() (mschedctl -c vpc)
 * 			MMProcessVPCResults()
 * 				MMDynamicResourcesLoad()
 * 				MMDynamicResourcesUpdate()
 * 		__MMIncreaseDynamicResources()
 * 			__MMDoCommand() (mschedctl -m vpc)
 * 		__MMReleaseDynamicResources()
 * 			__MMDoCommand() (mschedctl -m vpc)
 *
 * This process will create a dynamic Moab-type resource manager on the customer
 * Moab that will communicate with the host and port returned by the hosting center.
 * This resource manager can be destroyed using "mrmctl -d".
 *
 * The hosting center Moab will do nothing more than respond to "mshow -a" requests
 * (processed by MClusterShowARes) and create VPCs and Reservations using the normal
 * code.
 *
 * There are various methods to have the customer Moab automatically request
 * resources from the hosting center.  All of these methods involve the use of a
 * trigger with the basic format:
 *
 * TRIGGER=atype=internal,action=rm:<hosting>:modify:nodes+=1
 *
 * One example:
 *
 * CLASSCFG[holland] TRIGGER=atype=internal,etype=threshold,action="rm:holland:modify:nodes+=1:pkg=basic:duration=infinity",threshold=backlog>0,multifire=true,sets=init,requires=!init
 * CLASSCFG[holland] TRIGGER=atype=internal,action=rm:holland-hosting.0:destroy,etype=threshold,threshold="usage=0",multifire=true,unsets=init.!init,requires=init,failoffset=2:00
 *
 * This example can be modified for any credential and for various thresholds.
 *
 */




#define JobDependenciesDoc
/**
 * @design doc Job Dependencies
 *
 * Job dependencies form the basic workload interdendency framework.  Job
 * dependencies can be based on execution or completion of peer workload.
 * Other types of workload dependencies can be based on arbitrary events
 * including trigger completion or the existence, status, or value of
 * metadata.
 *
 * Job dependencies may be managed by Moab or by the resource manager.  If
 * managed by the resource manager, Moab should process, report, and honor
 * these dependencies but the resource manager will enforce them often via
 * indirect job holds or other blocking job states.  Further, in resource
 * manager based dependency management, the resource manager is responsible
 * for handling 'dead' jobs, jobs which can never run because dependencies
 * can never be satisfied (ie, release upon successful completion of job X)
 *
 * Job dependencies enforced by Torque and reported to Moab result in Moab
 * only knowing about the next unmet dependency.  Each iteration, Moab
 * requests the dependency list from torque, and only keeps the first item in
 * the list. This will never represent a problem, as Moab won't consider
 * the dependency it learns from Torque as being completed until Torque says
 * it is. The only effect this has on "normal" use is that to find a
 * complete listing of unmet dependencies, a user will have to rely on qstat
 * -f instead of checkjob or mdiag -j.
 * 
 * Jobs associated with resource managers which operate in the manner are
 * marked with the mjifRMManagedDepend internal job flag.  NOTE:  Currently,
 * Moab does not support a mix of Moab-managed and RM-managed dependencies
 * within the same job.
 *
 * The following routines are important to dependency management:
 *
 * MPBSJobLoad()->MPBSJobSetAttr() - sets mjifRMManagedDepend if RM-reported
 *   dependency is detected
 *
 * MQueueCheckStatus() - clears 'dead' jobs if dependencies are not RM-managed
 *
 * MJobCheckDependency() - report if dependency is blocking job execution
 *
 *   Routine used for job selection in MQueueSelectJobs(), MQueueSelectAllJobs()
 *
 * MJobSetDependency() - apply new dependency to job
 *
 *   Routine used to populate J->Depend sturucture
 *
 * MJobSetSyncDependency() - designed to specifically handle
 *   assigning a slave job to a master job
 *
 * MDependFind() - search for dependency based on type and/or value(job ID, usually)
 * 
 * MJobSynchronizeDependencies() - verify job dependencies reference up-to-date
 *   job ids
 *
 * MJobFindDepend() - XXX
 * MJobRemoveDependency() - XXX
 *
 * Reporting:
 *
 *   Dependency diagnostics are reported via 'mdiag -j' and 'checkjob -v'
 *   Dependency based workflows are reported in XML for visualization via
 *    'mjobctl -q workflow' (see __MUIJobQueryWorkflow())
 *
 * Modification:
 *
 *   Dependencies cannot be dynamically modified at this time
 *
 * Specification:
 *
 *   Dependencies can be specified using RM-specific directives or using RM
 *   extensions (see MJobProcessExtensionString(), mxaDependency)
 *
 * @see Workflow design doc
 *
 * NOTE:  All routines below should be located in the MDepend.c module (NYI)
 *
 * @see MJobCheckDependency()
 * @see MJobFindDependent()
 * @see MJobSetDependency()
 * @see MJobDependDestroy()
 * @see MJobDependCopy()
 * @see MJobAddPBSDependency()
 * @see MJobDependencyToString()
 * @see MJobComputeDependencyName()
 * @see MDependFree()
 * @see MJobRemoveDependency()
 * @see MJobFindDepend()
 * @see MJobSynchronizeDependencies()
 * @see MDependFind()
 * @see MJobSetSyncDependency()
 *
 * @see enum MDependEnum - list of supported dependency types
 *
 * 2.2 Data Structure Overview
 *
 * @see mdepend_t
 *   mdepend_t.Type    - dependency type @see enum MDependEnum
 *                     - set in MJobSetDependency()
 *   mdepend_t.Index   - index of entry in DAG mdepend_t array
 *                     - set in MDAGCreate()
 *   mdepend_t.Value   - object id of depedendee
 *                     - set in MJobSetDependency()
 *   mdepend_t.SValue  - 'raw' specified dependee string
 *                     - set in MJobSetDependency()
 *   mdepend_t.JAttr   - attribute of object to match, see mdpend_t.Value
 *                     - default value (ie, 0) indicates that mdpend_t.Value
 *                       must match a job/trigger id.  Setting JAttr to other
 *                       values allows the dependency to match other object
 *                       attributes (ie, specname)
 *   mdepend_t.ICount  - number of 'sync-slave' instances which must exist
 *                       before sync-master dependency is satisfied
 *                     - set in MJobSetDependency() This value is used for the
 *                       dependency type mdJobSyncMaster and isderived by
 *                       parsing the dependency value as a long. Normally,
 *                       the value represents a job ID, but in this case
 *                       it should be a positive number
 *                     - NOTE:  allows 'syncmaster' to not start until all
 *                              requested sync slaves are submitted and detected.
 *                              SyncMaster HAS to be the first dependency in the
 *                              dependency list J->Depend. All the other code
 *                              in Moab looks for a mdJobSyncMaster dependency
 *                              only in the first link, or J->Depend itself.
 *   mdepend_t.Offset  - time offset for relative event dependencies
 *                       NOTE:  only enabled with mdJobStart and mdJob*Completion
 *                              dependencies
 *                     - set in ???()
 *                     - enforced in MJobCheckDependency()
 *   mdpendt_t.BM      - bitmap of parent object-specific attributes affecting
 *                       dependency behavior - (ie, bitmap of enum
 *                       MTrigVarAttrEnum for trigger-based dependencies
 *                     - set im MJobSetDependency()
 *   mdepend_t.NextAnd - linked list pointer to next logical-AND dependency
 *                     - set in MJobSetDependency()
 *   mdepend_t.NextOr  - linked list pointer to next logical-OR dependency
 *                     - NYI

 * @see mjob_t.Depend - alloc/set/populated in MJobSetDependency()
 *
 * @see mtrig_t.Depend - alloc/set in MTrigSetAttr()
 *                     - populated in MDAGSetAnd()/MDAGSetOr()
 *
 * @see developer Dave,Josh
 *
 * 2.3 syncwith dependencies notes
 *
 *   2.3.1 implementation and msub interface
 *
 * Job co-allocation, the dependency feature that instructs moab
 * to start jobs simultaneously, is implemented using the
 * mdJobSyncMaster and mdJobSyncSlave types. From the user perspective,
 * it is implemented by specifying a "master" job and "slave" jobs.
 *
 * Externally, a user submits a job as a master job by giving it
 * the dependency synccount:<number>, where <number> is the number of
 * slave jobs that the job will have. Then, the user submits <number>
 * slave jobs, giving each of them the dependency syncwith:<job>, where
 * <job> is the master job originally submitted.
 *
 * Internally, moab assigns a dependency of type mdJobSyncMaster to the master
 * job, where the dependency value is the number specified in synccount as
 * a string. For each slave job, moab creates a two-way dependency between the
 * jobs by adding a mdJobSyncSlave dependency to both the master job and the
 * slave job, where the master dependency references the slave and vice versa.
 * It is important to not that the mdJobMasterSync dependency MUST appear
 * first in the J->Depend list on the master job, as all of the Moab scheduling
 * code assumes it is at that location.
 *
 * Moab implements the scheduling of co-allocation jobs in MQueueScheduleIJobs
 * with MJobCombine. MJobCombine locally replaces the master job with a
 * multi-req pseudo-job that has a req for the master job and each slave job.
 * This means that moab can never co-allocate more jobs than the static array
 * size of mjob_t.Req.
 *
 * Mob then distributes tasks for this pseudo-job using MJobDistributeMNL.
 * MJobDistributeMNL currently has the limitation that it cannot schedule more
 * than one req per node, so that each co-allocated job will have to run on its
 * own node. Dave Jackson has done work in version 5.4 to allow co-allocating
 * jobs with identical reqs to share nodes. This feature can be enabled with
 * the moab.cfg boolean parameter ALLOWMULTIREQNODEUSE
 *
 * Finally, when the pseudo-job is about to start in MJobStart, because it is a
 * master job it follows a separate code path whereby MJobStart is called on the
 * real master job and all of its dependencies. If any job start fails, MJobStart
 * is responsible for rolling back any other co-allocated jobs that may have
 * succesfully started first. I don't know if it requeues them or cancels them.
 *
 *   2.3.2 template interface

 * Currently, only horizontal workflow-based job co-allocation is supported.
 * In other words, one can co-allocate using a master template and 1 or more
 * slave templates, but recursive template co-allocation, where one job is
 * both a slave and a master, has not been tested.

 * To co-allocate with templates, a job must merely be applied to a template
 * with syncwith dependencies on other templates. Moab will automatically
 * figure out the synccount number and prepend the mdJobSyncMaster dependency
 * to the beginning of the list. An example job co-allocation workflow is
 * given below.

JOBCFG[SyncTemplate1]   EXEC=/some/script
JOBCFG[SyncTemplate2]   EXEC=/some/script
JOBCFG[SyncTemplate3]   EXEC=/some/script
JOBCFG[syncwith]        TEMPLATEDEPEND=syncwith:SyncTemplate1,syncwith:SyncTemplate2,syncwith:SyncTemplate3
JOBCFG[syncwith]        SELECT=TRUE #one way to apply the template would be to let users select it directly
USEMOABJOBID            TRUE #needed when using torque

 * Finally, it is important to note that for such workflows to work with torque
 * without any bugs, the moab parameter USEMOABJOBID must be specified.
 *
 * 2.4 Job Name Dependencies
 *
 * A user can submit jobs that are dependent on a job's AName. Because slurm
 * doesn't accept jobnames in the sbatch -P option, moab translates jobnames
 * into jobids. Currently this only happens if the job is msub'ed. If moab
 * learns about the job name dependencies from torque then the job names won't
 * be translated into jobids and the job will look at the first job it finds
 * that has the job name. With job name translation, it's possible to have a
 * job that is dependent on multiple jobids of the same job name. When using
 * msub, moab will translate the the job names into jobids and then at 
 * migration moab will inject the dependency list with the jobid's and not
 * jobnames so that on restart the correct list of dependencies will be 
 * preserved.
 *
 * 2.5 Dependency Submission
 *
 * Moab will check all dependencies for existance when the job is submitted
 * via msub. This was added to check against invalid dependency types. If the 
 * dependency list is discovered from a source other than msub then the 
 * dependencies will be not be checked.
 *
 */





#define DynamicJobHandlingDoc
/**
 * @designdoc Dynamic Job Handling
 *
 * @see developer Dave
 */



#define CheckpointingInfrastructureDoc
/**
 * @designdoc Checkpointing Infrastructure
 *
 * Steps to add new checkpointed attribute:
 *  - add to object structure (ie, mjob_t)
 *  - add to object enum/char (ie, enum MJobAttrEnum/MJobAttr[])
 *  - add to M*SetAttr() (ie, MJobSetAttr())
 *  - add to M*AToString() (ie, MJobAToString())
 *  - add to M*StoreCP() (ie, MJobStoreCP())
 *  - customize M*LoadCP() if required (ie, MJobLoadCP())
 *
 * @see developer Doug
 */




#define JobRMExtensionsDoc
/**
 * @designdoc Job RM Extensions
 *
 * @see developer Dave
 */





#define SystemJobDoc
/**
  @design doc System Jobs

  1.0 Overview
  1.1 Feature/Concept Overview (what it does, why it matters)

  System jobs are used to dynamically modify compute resources in a way that
  handles modification delays and potential operational failures.  These jobs
  are used for operations such adjusting node power state, reprovisioning node
  OS, and manipulating VM's.

  1.2 Brief Architecture Overview (how it is implemented)

  When a resource modification operation is required, a high level routine, ie,
  MSysSchedulePower() is called which generates all needed system job meta data
  including job location, job operation, and job details.  The routine
  MUGenerateSystemJobs() is called with these details which is responsible for
  creating the needed system jobs.

  Once created, the system jobs initiate the required low-level operation
  within the correct resource manager and then monitor its progress.  System
  jobs generally track changes and complete when the desired change is
  detected.

  The following routines are important to system job management:

  MUGenerateSystemJobs() - sets mjifRMManagedDepend if RM-reported
    dependency is detected

  MSysJobProcessFailure() - process system job failures
  MJobStartSystemJob() - start system job
  MJobUpdateSystemJob() - update system job
  MUSubmitSysJob() - submit system job
  MJobPopulateSystemJobData()
  MJobCreateSystemData()
  MJobSystemFromXML()

 @see enum MSystemJobTypeEnum
 @see MSysJobType[]

 Types:

  Period:  period jobs are submitted using the 'period' RM extension.  When the
    mxaPeriod extension is processed, the submitted job is copied into a private
    job template and then marked as a perpetual duration, no-resource system job
    with a populated mcal_t structure.  On the next iteration, Moab should start
    the no-resource job.  Once running, each iteration in MJobUpdateSystemJobs()
    the job will be evalutated to determine if a new reservation needs to be
    created.  If so, Moab should search out the needed resources by scheduling the
    job template job at the appropriate time to determine the correct hostlist.
    If successful, a spaceflex reservation with the byname flag should be created
    on the set resources.  This reservation should also have the R->SpecJ field
    set which is used for two purposes.  First, If the reservation needs to be
    space-flexed, this structure should be used to generate the scheduling
    pseudo-job used to search out replacement resources.  Second, when the
    reservation is started, this structure should be used to submit a new batch
    job which which have an rsv-binding to the reservation.

    If Moab cannot locate the needed resources to create the reservation, it
    should optionally trigger an event (Doug, determine trigger type, Doug,
    indicate when this trigger is fired and how we send it once and only once
    for each period which cannot be satisfied)

    Users will be able to see and cancel their parent period system job and
    when so doing, Moab will clean-up all child reservations and the private
    job template.  Running child period jobs will not be terminated
    automatically but must be explicitly killed.

  Event Based Updates: certain system jobs (ie, VM system jobs) can receive 
    updates regarding execution status via WIKI node GEVENTS.  In this case
    MWikiNodeUpdateAttr() will detect the node GEVENT attribute and then call
    MSysProcessGEvent() with the mxoxVM object type.  MSysProcessGEvent() is
    responsible for locating the associated system job and updating the 
    necessary jobs attributes.

 @see developer Dave,Josh
 */




#define JobSchedulingDoc
/**
  @design doc Job Scheduling

  1.0 Overview
  1.1 Feature/Concept Overview (what it does, why it matters)

    This process converts an idle job to an active, running job.

  1.2 Brief Architecture Overview (how it is implemented)

    Jobs are started through the following steps:

  - identify job eligibility according to all policies and limits
  - determine job priority order
  - in job priority order, identify potential resources
  - if adequate resources are available, identify which resources to allocate
  - determine task placement on allocated resources
  - issue 'jobstart' request to resource manager

  2.0 Architecture
  2.1 Functional Flow (Review of key functions)

  - MSchedProcessJobs()
   - MQueueSelectAllJobs()
   - MQueueSelectJobs()
   - MQueueScheduleIJobs()
     - MJobSelectMNL()
       - MReqGetFNL()
       - MJobGetINL() - locate hosts which are matches for immediate execution of job tasks
       - MNodeGetPreemptList()
       - MJobSelectPJobList()
     - MJobAllocMNL()
     - MJobStart()
       - MJobDistributeMNL()
   - MQueueBackfill()

  NoQueue Jobs:

    MJobStart() - defer job immediately if start was unsuccessful and noqueue job flag 
                  specified

    MQueueCheckStatus() - remove noqueue job if idle or if hold/defer in place

    @see mxaQueueJob
    @see mjfNoQueue

    Example:  msub -l nodes=1,walltime=100,queuejob=false

    NOTE:  noqueue does not force an immediate scheduling iteration
    NOTE:  noqueue jobs do not receive special priority

  2.2 Data Structure Overview
  2.3 Functional List (list of all feature functions)
  2.4 Limitations/Warnings
   3.0 Usage
     3.1 Required Parameters
     3.2 End-User Usage
     3.3 Reporting
     3.4 Dynamic Modification
     3.5 Checkpointing
     3.6 Examples
   4.0 Next Steps
   5.0 Related Objects
     5.1 Design Docs
     5.2 Key Developers
     5.3 Interested Parties
     5.4 User Documentation
     5.5 Other
   6.0 Test Plan
   7.0 ChangeLog (optional)
*/





#define AllocationManagementDoc
/**
  @design doc Allocation Management

  1.0 Overview
  1.1 Feature/Concept Overview (what it does, why it matters)

     Coordinates internal resource and service usage with an external allocation
   management service including handling quotes, liens, and charges.

  1.2 Brief Architecture Overview (how it is implemented)

  2.0 Architecture
  2.1 Functional Flow (Review of key functions)

  2.2 Data Structure Overview
  2.3 Functional List (list of all feature functions)
  2.4 Limitations/Warnings
   3.0 Usage
     3.1 Required Parameters
     3.2 End-User Usage
     3.3 Reporting
     3.4 Dynamic Modification
     3.5 Checkpointing
     3.6 Examples
   4.0 Next Steps
   5.0 Related Objects
     5.1 Design Docs
     5.2 Key Developers

       Dave Jackson
       Scott Jackson

     5.3 Interested Parties
     5.4 User Documentation
     5.5 Other
   6.0 Test Plan
   7.0 ChangeLog (optional)
*/



#define MAQDoc
/**
  @design doc Active Queue

  1.0 Overview
  1.1 Feature/Concept Overview (what it does, why it matters)

    The MAQ[] array is the 'moab active queue', a job array containing indices
  to all 'active' jobs stored in the MJob[] table.  This structure provides a
  table of 'active' jobs which are defined as being in state 'starting' or
  state 'running' (not state 'suspended').  The MAQ table is maintained in
  sorted order based on first-to-last completion time.

  1.2 Brief Architecture Overview (how it is implemented)


  2.0 Architecture
  2.1 Functional Flow (Review of key functions)

  Job are added to the MAQ[] table by the routine MQueueAddAJob() and
  are removed using the routine MQueueRemoveAJob().

  The MAQ[] table is destroyed and re-initialized each iteration at the
  start of MRMUpdate().

  MAQ[] is only referenced within the scheduling and UI phases after
  the completion of MRMUpdate().

  MAQ[] is updated as active jobs are discovered w/in
  MRMJobPost{Load,Update}() and as jobs are started by Moab.  If an RM
  workload query fails, Moab will update the MAQ table for these 'stale'
  active jobs at the end of MRMUpdate().

  2.2 Data Structure Overview
  2.3 Functional List (list of all feature functions)
  2.4 Limitations/Warnings
   3.0 Usage
     3.1 Required Parameters
     3.2 End-User Usage
     3.3 Reporting
     3.4 Dynamic Modification
     3.5 Checkpointing
     3.6 Examples
   4.0 Next Steps
   5.0 Related Objects
     5.1 Design Docs
     5.2 Key Developers

       Dave Jackson

     5.3 Interested Parties
     5.4 User Documentation
     5.5 Other
   6.0 Test Plan
   7.0 ChangeLog (optional)
*/

#define ExtendedMetricTrackingDocs

/**
 * @designdoc job metric tracking
 *
 * N->XLoad
 * RQ->XURes->XLoad
 */



#define DynProvDoc
/**
  @design doc Dynamic Provisioning

  1.0 Overview
  1.1 Feature/Concept Overview (what it does, why it matters)

    Dynamic provisioning allows a cluster or data centers OS's mix to be
  adaptive adjusting to specific workload needs or general high-level workload
  performance metrics (ie total backlog for windows jobs)

  1.2 Brief Architecture Overview (how it is implemented)

  OnDemand provisioning:  allow priority jobs which are capable of making reservations to create
                          provisioning system jobs if they are associated with the 'provision'
                          QoS flag - on demand provisioning should prefer to use idle resources
                          which are not inside of a dynamic partition, ie nodes which were previously
                          used by another on-demand job.

  2.0 Architecture
  2.1 Functional Flow (Review of key functions)


  2.2 Data Structure Overview

    @see N->ActiveOS       -- OS currently active on node
    @see N->OSList         -- list of OS's supported by node
    @see J->Req[X]->Opsys  -- required job OS
    @see MSched.ProvRM     -- resource manager responsible for OS changes
    @see mqfProvision      -- QOS flag allowing on-demand provisioning
    @see P->J              -- partition pseudo-job for OS tracking
    @see MSched.ForceJITServiceProvisioning -- do not provision node to needed OS until MJobStart() is called for job

  2.3 Functional List (list of all feature functions)

    @see MReqCheckResourceMatch() - verify node's OSList contains job's
           required operating system
    @see MJobGetSNRange() - delay resource availability is N->ActiveOS does not
           match RQ->Opsys

  2.4 Limitations/Warnings
   3.0 Usage
     3.1 Required Parameters
     3.2 End-User Usage
     3.3 Reporting
     3.4 Dynamic Modification
     3.5 Checkpointing
     3.6 Examples
   4.0 Next Steps
   5.0 Related Objects
     5.1 Design Docs
     5.2 Key Developers

       Dave Jackson

     5.3 Interested Parties
     5.4 User Documentation
     5.5 Other
   6.0 Test Plan
   7.0 ChangeLog (optional)
*/





#define DedResDoc
/**
  @design doc Dedicated Resources

  1.0 Overview
  1.1 Feature/Concept Overview (what it does, why it matters)

    Dedicated resource tracking allows Moab to manage workload resource
  requirement in the active and future time frames.  This information is used
  within nodes, reservations, and jobs in making job placement and optimization
  decisions.

  1.2 Brief Architecture Overview (how it is implemented)


  2.0 Architecture
  2.1 Functional Flow (Review of key functions)


  2.2 Data Structure Overview

    mjob_t.mreq_t.DRes - dedicated resources required by job req

    mrsv_t.DRes - dedicated resources required by rsv

    mnode_t.DRes - dedicated resources protected on node

    mre_t.DRes - dedicated resources blocked by rsv during specified timeframe
                 which are not also consumed by overlapping job/consumer rsv

  2.3 Functional List (list of all feature functions)

    MRMNodePreLoad() - clear node's dedicated resources
    MRMReqPreLoad() - clear req's dedicated resources
    MRMJobPostLoad() - update node's dedicated resources based on workload with
      alloc resources: loop thru all reqs, loop through all nodes in req, use
      MCResAdd() to adjust N->DRes
    MJobUpdateResourceUsage() - update node's dedicated resources (called w/in
      MRMJobPostUpdate())
    MRMWorkloadQuery() - clears node's dedicated resources
    MNodeGetDRes() - determine dedicated resources for node
    MNodeAdjustAvailResources() - update per node available resources
    MPBSNodeUpdate() - update per node dedicated resources as jobs are
      reported
    MNatWorkloadQuery()/__MNatWorkloadProcessData()

  2.4 Limitations/Warnings
   3.0 Usage
     3.1 Required Parameters
     3.2 End-User Usage
     3.3 Reporting
     3.4 Dynamic Modification
     3.5 Checkpointing
     3.6 Examples
   4.0 Next Steps
   5.0 Related Objects
     5.1 Design Docs
     5.2 Key Developers

       Dave Jackson

     5.3 Interested Parties
     5.4 User Documentation
     5.5 Other
   6.0 Test Plan
   7.0 ChangeLog (optional)
*/




#define RsvNuances
/**
 * @designdoc Reservation Nuances
 *
 * There are a lot of nuances when using reservation that make host-expression and taskcount based reservations act
 * differently and make things confusing. This section is used to document them.
 *
 * Host-expression reservations ignore the state of the nodes. So if a node is down, moab is still going to try to put
 * the reservation on the node. This breaks RSVREALLOCPOLICY FAILURE with a host-expression reservation.  See notes in
 * MRsvReplaceNodes and MRsvCreateNL.
 *
 * Reservations by default are besteffort. A system reservation created with -h ALL will try to get all of the nodes,
 * but if it can't it won't fail, and only get some of the nodes. In these cases -F REQFULL should be used.
 *
 * When using host-expressions and taskcounts together, host-expressions specify which nodes will be used to fullfill
 * the taskcount.
 */



#define ForkedMoabUIHandlerDoc
 /**
  *  @designdoc Forked Moab Child Process User Interface Handler 
  *
  * 1.0 Overview
  *   1.1 Feature/Concept Overview (what it does, why it matters)
  *
  *   Moab handles user interface requests (such as showq) during its UI processing phase. This means that users may get slow response
  *   to their user interface commands when moab is busy doing other things besides user interface command processing.
  *
  *   A new experimental feature has been added to allow moab to periodically fork a new child moab process which only processes user interface commands.
  *   This provides quicker response (although with potentially older data) as opposed to potentially much slower response but with current data.
  *
  *   1.2 Brief Architecture Overview (how it is implemented)
  *
  *   The forked moab UI child process gets a copy of the parent moab's data structures, so it can process user interface commands which only
  *   require data from the moab memory structures.
  *
  *   The moab UI child process receives the user interface commands on an alterrnate client user interface port id (specified in a configuration parameter).
  *
  *   A new UI child moab is forked every N iterations (N is specified in a configuration parameter) and the previous child process dies.
  *   The child moab gets a copy of the parent moab data structures at the time it is forked, and this data is not updated for the lifetime of the
  *   child moab process. All user interface requests sent to the child moab UI port will be processed using this "static" data which will become more 
  *   stale as time goes on. For example, all showq requests sent to the moab child process will return the exact same information (until a new child process 
  *   is forked with fresh data). If up to the second information is required, the user interface command should not be sent to the alternate UI port so that the
  *   parent moab will receive and process the request (in due time). 
  *
  *   When a child process is forked it will close all sockets and open files and currently there is no logging from the child moab process.
  *   Child moab process logging is something we will wish to add.
  *
  *   It does open a new socket on the child user interface port to process user interface commands sent to the client UI port.
  *
  *   At present the child moab process does not do any checking of the requested command to make sure that it is a valid command for a moab UI child process to handle.
  *   We currently tell customers that it is just for showq. Other commands may be OK, but which ones those are has not been investigated.
  *   This is another TBD for this feature.
  *
  * 2.0 Architecture
  *   2.1 Functional Flow (Review of key functions)
  *
  *   @see MSysMainLoop()
  *   @see MSysUIChildProcessing()
  *   @see MSysUIChildAcceptRequests()
  *   @see msched_t.ClientUIPort
  *   @see msched_t.ForkIterations
  *   @see msched_t.UIChildPid
  *   
  *   Just before the parent moab calls MUIAcceptRequests() and MSysProcessEvents() it will check to see if the MSched.ForkIterations is greater than 0 and if so do a mod
  *   operation to see if it is time to fork a new child moab UI process.
  *
  *   If so, it kills the old child process (using MSched.UIChildPID) and forks a new child process. The parent moab process continues with its normal processing.
  *
  *   The child moab process calls MSysUIChildProcessing() (which it stays in until it dies). In this routine, the child closes streams, files, sockets, etc inherited
  *   from the parent moab process. It also resets the signal handlers. It also flushes any pending UI transactions (the parent will process those). It opens
  *   the client UI port and accepts any UI commands sent to its client UI port calling MSysUIChildAcceptRequests(). MSysUIChildAcceptRequests() calls MSysProcessRequests()
  *   to handle any requests that come in.
  *
  *   2.2 Limitations/Warnings
  *
  *   As mentioned above, the child UI process does not do any logging at this time. It does not do any checking of what UI command is requested.
  *
  *   Users should be aware that the data is not real time.
  *
  * 3.0 Usage
  *   3.1 Required Parameters
  *
  *   Here are a couple of sample lines for the moab.cfg file in order to enable the feature and cause a new moab child UI process to be forked every "1" iterations and
  *   the child process will process user interface commands sent to port 45666.
  *
  *   FORKITERATIONS 1
  *   CLIENTUIPORT 45666
  *
  *   3.2 End-User Usage
  *
  *   The user can send the showq command to this port by typing the command as follows....
  *
  *   showq --port=45666
  *
  */





#define MTClientRequestHandlerDoc
/**
 * As of Moab 6.0, Moab uses a thread pool to service certain client commands
 * (mdiag -j, mdiag -n, mdiag -r as of 11-28-2010).
 *
 * As of Moab 6.0, Moab uses a memory cache to store job and node transition
 * objects that these threads use to service their client requests.
 *
 * It is intended that the client commands serviced by these threads will
 * return only XML, which will then be translated to human-readable text by
 * the client if necessary. This has been done for mdiag -r and mrsvctl -q
 * diag, but not mdiag -j or mdiag -n.
 *
 */





#define ThreadSafeObjectStorageDoc
/**
 * As of Moab 6.0, Moab is capable of storing certain internal objects in a
 * memory cache and/or an external database. The purpose of the memory cache
 * is to speed up processing of client requests (mdiag -r|n|j, showq, etc) by
 * going directly to the cache on a separate thread instead of waiting for
 * the scheduler thread to finish scheduling and handle client requests. The
 * purpose of the external database is to store moab object state so that an
 * external program or script can access it without interrupting the
 * scheduler (i.e. so a customer could create a web portal (similar to
 * MCM/Viewpoint) to allow their end users a graphical method of viewing moab
 * state and not needing to use the command-line.
 *
 * The following steps are needed to store an object in the cache:
 * 
 * 1. The object needs a transition struct (see mtransjob_t, mtransnode_t, etc)
 *
 *    a.  The transition struct needs to be self-contained, i.e. it needs to
 *        contain sufficient object state to reproduce the client command's
 *        output without requiring intervention from the scheduler thread.
 *
 *    b.  The transition struct needs to be carefully thought out as to
 *        memory requirements, since everything in the transition struct will
 *        be adding directly to Moab's memory footprint.
 *
 *    c.  The cache (see mcache_t) needs to have a hash table for the new
 *        transition object, with an accompanying lock for that particular
 *        hash table.
 * 
 * 2. The transition struct must have the following methods:
 *    
 *    a.  allocation/free (see MJobTransitionAllocate/MJobTransitionFree)
 *
 *    b.  object to transition (see MJobToTransitionStruct) should rely
 *        mainly on object attribute-to-string/mstring functions (i.e.
 *        MJobAToString/MJobAToMString)
 *
 *    c.  attribute to string (see MJobTransitionAToString)
 *
 *    d.  transition to XML (see MJobTransitionToXML) (should rely mainly on
 *        transition attribute-to-string function)
 *
 *    e.  functions for writing to and reading from the cache (see
 *        MCacheWriteJob/MCacheReadJobs)
 *
 *    f.  function for transitioning the object (see MJobTransition)
 * 
 * 3. The transition struct may need the following methods:
 *
 *    a.  transition copy (see MJobTransitionCopy)
 *
 *    b.  transition matches/fits constraint list (see MJobTransitionFitsConstraintList)
 *
 * In order to use the memory cache and the transition objects to service
 * client commands, the following conditions must be met:
 *
 * 1. There must be threadsafe function in MUI.c to handle the request (see
 *    MUIJobCtlThreadSafe)
 * 
 * 2. There must be an enum in MClientCmdEnum to represent that function (in moab.h)
 *
 * 3. There must be a string constant in MClientCmd[] (in MConst.c) that is
 *    the name of the threadsafe function that will be called.
 *
 * 4. There must be a case statement in MSysDelegateRequest which sets the
 *    command index to the apppropriate value of MClientCmdEnum.
 *
 * 5. The MCRequestF list of function pointers/names needs to be updated to
 *    include the name of the function to be called (see items 1 and 3).
 *
 * 6. The client side of the command (mclient.c) will need a function to
 *    translate the data returned from the server.
 *
 * 7. Adding "DISPLAYFLAGS USEBLOCKING" to the moab.cfg file should be tested 
 *    to make sure that the blocking functions are always called and the 
 *    cache is bypassed.
 *
 * In order to store transition objects in the external database, see ODBCDataStorageDoc
 */



#define JobEnvVariablesDoc
 /**
  *  @designdoc Generic Job Environment Variables
  * 
  * 1.0 Overview
  *   1.1 Feature/Concept Overview (what it does, why it matters)
  *
  *   In the move for Tri-Labs to unified across all systems, it was requested that
  *   Moab add environment variables specific to Moab in the job's environment if a 
  *   -E flag was added to msub.
  *
  *   1.2 Brief Architecture Overview (how it is implemented)
  *
  *   The following is the list of environment variables that will be passed to the
  *   job if -E is specified.
  *
  *   MOAB_ACCOUNT      Account Name
  *   MOAB_BATCH        Set if a batch job (non-interactive).
  *   MOAB_CLASS        Class Name 
  *   MOAB_DEPEND       Job dependency string.
  *   MOAB_GROUP        Group Name
  *   MOAB_JOBID        Job ID. If submitted from the grid, grid jobid.
  *   MOAB_JOBINDEX     Index in job array. - Not being used.
  *   MOAB_JOBNAME      Job Name
  *   MOAB_MACHINE      Name of the machine (ie. Destination RM) that the job is running on.
  *   MOAB_NODECOUNT    Number of nodes allocated to job.
  *   MOAB_NODELIST     Comma-separated list of nodes (listed singly with no ppn info).
  *   MOAB_PARTITION    Partition name the job is running in. If grid job, cluster scheduler's name.
  *   MOAB_PROCCOUNT    Number of processors allocated to job.
  *   MOAB_QOS          QOS Name
  *   MOAB_TASKMAP      Node list with procs per node listed. <nodename>.<procs>
  *   MOAB_USER         User Name
  *
  *   Until slurm adds the ability to modify a submitted job's environment variables before
  *   starting, the variables are added at submission. For pbs the environment variables are
  *   added just before the job is started. This allows variables like nodelist to be fully
  *   populated.
  *
  *   MOAB_NODELIST and MOAB_TASKMAP are delimited by '&' because torque doesn't take nested ,'s when 
  *   modifying the variable list.
  *
  *   An rm extension attribute, mxaEnvRequested, was used to pass from the client to the server, rather
  *   than adding a new xml spec because the information needed to be passed to the job for storage/checkpointing
  *   and interactive jobs. It seemed more intuitive to use one attribute rather than two.
  *
  * 2.0 Architecture
  *   2.1 Functional Flow (Review of key functions)
  *
  *   Submission:
  *   MCSubmitProcessArgs()
  *
  *   Slurm:
  *   MWikiJobSubmit()
  *    -->MJobGetEnvVars()
  *
  *   PBS:
  *   MPBSJobStart()
  *    -->MJobGetEnvVars()
  *    -->MPBSJobModify()
  *
  *   2.2 Data Structure Overview
  *
  *   MJEnvVarEnum
  *   MJEnvVarAttr
  *
  *   mxaEnvRequested
  *
  * 3.0 Test Plan
  *
  * Cluster:
  * Submit a job that will have all of the varibles populated. 
  * Look for MOAB_BATCH= for batch jobs.
  * Make sure MOAB_BATCH= is not defined for interactive jobs.
  *
  * Batch Job:
  * brian@keche:~/test/moab-torque$ cat STDIN.o349 | grep MOAB | sort
  * MOAB_ACCOUNT=acctA
  * MOAB_BATCH=
  * MOAB_CLASS=batch
  * MOAB_DEPEND=jobsuccessfulcomplete:346:jobcomplete:347
  * MOAB_GROUP=brian
  * MOAB_JOBID=349
  * MOAB_JOBNAME=STDIN
  * MOAB_MACHINE=torque
  * MOAB_NODECOUNT=0
  * MOAB_NODELIST=keche-vm01&keche
  * MOAB_PARTITION=torque
  * MOAB_PROCCOUNT=8
  * MOAB_QOS=qosA
  * MOAB_TASKMAP=keche-vm01:4&keche:4
  * MOAB_USER=brian
  *
  * Interactive Job:
  * brian@keche-vm01:~$ env | grep MOAB | sort
  * MOAB_ACCOUNT=acctA
  * MOAB_CLASS=batch
  * MOAB_GROUP=brian
  * MOAB_JOBID=351
  * MOAB_JOBNAME=STDIN
  * MOAB_MACHINE=torque
  * MOAB_NODECOUNT=0
  * MOAB_NODELIST=keche-vm01&keche
  * MOAB_PARTITION=torque
  * MOAB_PROCCOUNT=8
  * MOAB_QOS=qosA
  * MOAB_TASKMAP=keche-vm01:4&keche:4
  * MOAB_USER=brian
  *
  * Grid:
  * Submit a job that will have all of the varibles populated. 
  * Check that the jobid and partition names are those of the grid head.
  */

#define MDiagDoc
/**
@designdoc mdiag command

mdiag is a command used to diagnose different aspects of moab.  Different options
are used to diagnose different parts.

mdiag -j: this command is used to display basic information on each job in moab.
A new feature as of version 5.4 is the option to specify just one user's jobs by
This can be constrained with -w attr=val.  Attr can be account, group, user, class,
or QOS.  Jobs that don't have that value for that attribute will be skipped.

*/

#define PreemptionDoc
/**
 * @designdoc preemption
 * 
 * 1.0 Overview
 * 
 *  1.1 Preemption means moving one job out of the way for another.  This can be done by suspending the running job
 *  or requeueing it, or simply cancelling it.  In any case the resources of the active job are freed for the idle 
 *  job and the idle job then begins running.
 *
 *  Moab schedules preemption in MJobSelectMNL().  That routine will first determine feasible resources, then from 
 *  those it will determine idle resources.  If there are not enough idle resources then Moab moves into the preemption
 *  routines.  First, from the feasible node list Moab finds which nodes have preemptible resources (MNodeGetPreemptList)
 *  it then uses the list of feasible preemptees returned by MNodeGetPreemptList() to determine which are the best
 *  jobs to preempt (MJobSelectPJobList).  This happens for each req.  Once we have a list of the best jobs to preempt
 *  we immediately preempt those jobs and assume the preemption succeeded and the nodes are now available to run the
 *  preemptor job.  Scheduling then continues on as normal using whatever idle and preempted resources are available.
 *
 *  With this model (which is the only available model in Moab 5.4 and below) NODEALLOCATIONPOLICY and NODESETS are
 *  ignored.  Moab determines the best resources based on possible preemptees not based on best resources.  The 
 *  advantage of this model is that Moab will preempt the fewest amount of jobs, the best mix of jobs.  A simple 
 *  example is a serial job preempting a 500 way job because the 500 way job has the best resources.  Meanwhile, the
 *  same job could've preempted another serial job and left the 500 way job running.  There are tradeoffs.  
 *
 *  Multi-req preemption is tricky as you have to make sure that the resources are exclusive among reqs.  This is done
 *  by making sure that each req preempts an exclusive list of jobs.  If Req[0] preempts job A then Req[1] cannot
 *  preempt that job A.  This also has problems because it's possible that Req[0] does not use all the resources of job
 *  A and because Req[1] cannot preempt job A some of those resources are wasted and Moab may end up over-preempting.
 *  To solve this we need to add whatever resources are not consumed by Req[0] to the idle list and let subsequent 
 *  Reqs consume those idle resources.  THIS IS NOT YET IMPLEMENTED.  MOAB MAY OVERPREEMPT.
 */

#define BluegenePreemptionDoc 
/** 
 * @designdoc Bluegene Preemption
 *
 * 1.0 Overview
 *
 *   1.1 Feature/Concept Overview (what it does, why it matters)
 *
 *   Because slurm has a hand in where jobs can run in bluegene setups, it doesn't work for moab to make 
 *   decisions where a preemptor can preempt. Thus the help of slurm is needed.
 *
 *   This is implemented in the 5.2_llnl branch. In 5.2_llnl you have to compile with -DMMAX_PREEMPTOPTIMIZATIONSIZE=#
 *   so that moab will consider more than 6 preemptees. In 5.3 you would set PREEMPTJOBCOUNT in moab.cfg.
 *
 *   1.2 Brief Architecture Overview (how it is implemented)
 *
 *   All the bluegene preemption work originates in MJobSelectMNL. If the MJobSelectMNL determines that there are 
 *   enough resources to fullfill the job, moab will ask slurm, through the willrun command, if the job can run 
 *   now on the available nodes. If it can the job will continue on where another willrun will occur in MJobAllocMNL
 *   and the job will be started. If slurm says the job can't run now, moab looks for preemptable jobs. If 
 *   moab determined that there weren't enough resources for the job to begin with, then moab will bypass the first
 *   willrun call and just look for preemptable jobs.
 *
 *   SLURM v2.1+
 *
 *   BLUEGENEPREEMPTION is no longer used. If preemption is needed, moab passes to slurm the idle nodes + the nodes of
 *   preemptible jobs along with the list of the PREEMPTEES. Moab passes the list of preemptees in the order that
 *   slurm should consider the preemptees. Which is: samesizedjobs first then smaller jobs to largers jobs. Slurm then
 *   reports back the nodes and the jobs to preempt.
 *
 *   SLURM 2.0 and below:
 *
 *   If BLUEGENEPREEMPTION is set to FALSE, default, in the moab.cfg then moab combines the idle nodes and the 
 *   preemptable nodes and asks slurm where it can could run the soonest, as if it were to be scheduled after the
 *   current running jobs. Moab then takes the nodelist given by moab and preempts the jobs on those nodes. This was 
 *   the initial implementation, but it doesn't take into account size and placement of jobs.
 *
 *   If BLUEGENEPREEMPTION is set to TRUE, then moab first tries to find a preemptee job with the same size and if that
 *   doesn't work then moab continually builds a list of nodes from the preemptee jobs, starting with the smaller size
 *   jobs first, until slurm says that the job can run on a set of nodes. 
 *
 *   Moab sorts the same size jobs in order of low priority to high priority. Moab will use the first preemptee it finds 
 *   that is of the same size. Asking slurm if the job can run is not necessary since correct size of block (wiring) has 
 *   already been created. One check, which is not implemented yet, is to check if the job has the same connection type 
 *   (torus or mesh) as well as the same size. Moab doesn't currently get this information from slurm.  
 *
 *   If there are no jobs of the same size, Moab builds a list of nodes from a list of jobs sorted by Request.TC. Though 
 *   the list may be in priority order as well, then is no guarantee that all lower priority jobs will be preempted first 
 *   since slurm is determining the nodelist. When building a list of nodes to ask slurm, moab first builds the list of 
 *   nodes until the needed amount of tasks are gathered to prevent unnecessary calls to slurm. When slurm says the job 
 *   can run on a set of nodes, the time returned by slurm is checked against the latest ending preemptee's end time. If 
 *   the start time from slurm is greater than the end time of the latest preemptee, this means that the job won't be able 
 *   to start right away on the given nodes because the needed wiring for the block is being used in another block.
 *
 *   Once the node list is determined, moab then preempts the jobs and tries to start the job. It's possible that the wiring,
 *   won't be available for several seconds. In this case, the willrun will fail in MJobAllocMNL and the job will get a 
 *   reservation one second in the future on the nodes just preempted and will start shortly after.
 *
 * 2.0 Architecture
 *   2.1 Functional Flow (Review of key functions)
 *
 *   v2.1+:
 *
 *   MJobSelectMNL()
 *   -->MJobSelectBGPJobList()
 *     -->MJobSortBGPJobListTest()
 *     -->MJobWillRun()
 *
 *   pre v2.1:
 *
 *   MJobSelectMNL()
 *   -->MJobWillRun()
 *   if (BLUEGENEPREEMPTION)
 *     -->MJobSelectBGPJobList()
 *       -->__MJobSelectBGPFindExactSize()
 *       -->__MJobSelectBGPFindSmallSize()
 *         ->MJobWillRun()
 *       -->__MJobGetLatestPreempeeEndTime()
 *   else
 *     -->MJobWillRun()
 *
 *   -->MJobSelectIdleTasks() - To distinguish between preempt and idle nodes and how many tasks should be preempted.
 *
 * 3.0 Test Plan
 *
 * slurm.conf:
 * NodeName=bgp[000x733] Procs=1024 State=UNKNOWN NodeAddr=keche NodeHostName=keche
 * PartitionName=pdebug Nodes=bgp[000x733] Default=YES MaxTime=INFINITE State=UP Shared=FORCE
 *
 * bluegene.conf:
 * LayoutMode=DYNAMIC
 * BasePartitionNodeCnt=512
 * NodeCardNodeCnt=32
 * Numpsets=32
 *
 * ***Test 1***
 * All jobs but 32k job should be preempted.
 *
 * msub -l nodes=32k cmdl
 * msub -l nodes=6k,qos=standby  cmdl
 * msub -l nodes=2k,qos=standby  cmdl
 * msub -l nodes=2k,qos=standby  cmdl
 * msub -l nodes=512,qos=standby cmdl
 * msub -l nodes=512,qos=standby cmdl
 * msub -l nodes=512,qos=standby cmdl
 * msub -l nodes=512,qos=standby cmdl
 * msub -l nodes=512,qos=standby cmdl
 * 
 * mschedctl -r
 * sleep 2
 * 
 * msub -l nodes=32k cmdl
 *
 * ***Test 2***
 * Should preempt 4k job
 *
 * msub -l nodes=8k,qos=standby  cmdl
 * msub -l nodes=48k,qos=standby cmdl
 * msub -l nodes=4k,qos=standby  cmdl
 * mschedctl -r
 * sleep 3
 * msub -l nodes=4k cmdl
 *
 * ***Test 3***
 * Should preempt 4k job. 
 * The last 2 1k jobs shouldn't get scheduled before preemptor once 4k job has been preempted.
 *
 * msub -l nodes=8k,qos=standby  cmdl
 * msub -l nodes=48k,qos=standby  cmdl
 * msub -l nodes=4k,qos=standby  cmdl
 * msub -l nodes=1k,qos=standby  cmdl
 * msub -l nodes=1k,qos=standby  cmdl
 * msub -l nodes=1k,qos=standby  cmdl
 * msub -l nodes=1k,qos=standby  cmdl
 * 
 * mschedctl -r
 * sleep 2
 * 
 * msub -l nodes=4k cmdl
 *
 * ***Test 4***
 * Should preempt 12 1k jobs
 * Preemptor should start before last two preemptees
 * Sometimes the preemptor will get a reservation for 1 second because
 * the block is being torn down and built.
 * 
 * msub -l nodes=48k,qos=standby  cmdl
 * msub -l nodes=1k,qos=standby  cmdl
 * msub -l nodes=1k,qos=standby  cmdl
 * msub -l nodes=1k,qos=standby  cmdl
 * msub -l nodes=1k,qos=standby  cmdl
 * msub -l nodes=1k,qos=standby  cmdl
 * msub -l nodes=1k,qos=standby  cmdl
 * msub -l nodes=1k,qos=standby  cmdl
 * msub -l nodes=1k,qos=standby  cmdl
 * msub -l nodes=1k,qos=standby  cmdl
 * msub -l nodes=1k,qos=standby  cmdl
 * msub -l nodes=1k,qos=standby  cmdl
 * msub -l nodes=1k,qos=standby  cmdl
 * msub -l nodes=1k,qos=standby  cmdl
 * msub -l nodes=1k,qos=standby  cmdl
 * msub -l nodes=1k,qos=standby  cmdl
 * msub -l nodes=1k,qos=standby  cmdl
 * msub -l nodes=1k,qos=standby  cmdl
 * msub -l nodes=1k,qos=standby  cmdl
 * 
 * mschedctl -r
 * sleep 2
 * 
 * msub -l nodes=12k cmdl
 *
 * ***Test 5***
 * The 26k job gets deferred because it can't ever run. Even
 * if it was submitted alone, without previous jobs. 
 *
 * msub -l nodes=32k cmdl
 * msub -l nodes=2k,qos=standby  cmdl
 * msub -l nodes=2k,qos=standby  cmdl
 * msub -l nodes=512,qos=standby cmdl
 * msub -l nodes=512,qos=standby cmdl
 * msub -l nodes=512,qos=standby cmdl
 * msub -l nodes=512,qos=standby cmdl
 * msub -l nodes=6k cmdl
 * msub -l nodes=512,qos=standby cmdl
 * msub -l nodes=512,qos=standby cmdl
 * 
 * mschedctl -r
 * sleep 2
 * 
 * msub -l nodes=26k cmdl
 *
 * ***Test 6***
 * Should preempt low priority 4k job. 
 * Moab should off the low priority 4k job to be preempted first
 * to slurm but slurm will still have a say.
 * 
 * msub -l nodes=48k,qos=standby  cmdl
 * msub -l nodes=4k,qos=standby  cmdl
 * msub -l nodes=4k,qos=low cmdl
 * msub -l nodes=4k,qos=standby  cmdl
 * msub -l nodes=4k,qos=standby  cmdl
 * 
 * mschedctl -r
 * sleep 2
 * 
 * msub -l nodes=4k cmdl
 * 
 *
 * setup:
 * LayoutMode=OVERLAP
 * BasePartitionNodeCnt=512
 * NodeCardNodeCnt=32
 * Numpsets=32
 *
 * NodeName=bgp[000x003] Procs=512 State=UNKNOWN NodeAddr=keche NodeHostName=keche
 * PartitionName=pdebug Nodes=bgp[000x003] Default=YES MaxTime=INFINITE State=UP Shared=FORCE
 * 
 * BPs=keche[000x000] Type=SMALL 128CNBlocks=4  
 * BPs=keche[000x000] Type=TORUS        
 * BPs=keche[001x001] Type=TORUS        
 * BPs=keche[002x003] Type=TORUS
 * -----------
 * case 1:
 * submit 1 512 preemptee
 * submit 1 512 non-preemptee
 * 
 * submit 1 512 preemptor
 * 
 * result: 512 preemptor preempts preemptee
 * 
 * submit 1 512 preemptor
 * 
 * result: preemptor gets priority reservation.
 * 
 * -----------
 * case 2:
 * submit 1 512 preemptee
 * submit 1 512 non-preemptee
 * submit 1 1k  non-preemptee
 * 
 * submit 1 512 preemptor
 * 
 * result: 512 preemptor preempts preemptee
 * 
 * submit 1 512 preemptor
 * 
 * result: preemptor gets priority reservation.
 * 
 * -----------
 * case 3:
 * submit 1 1k preemptee
 * 
 * submit 1 1k preemptor
 * 
 * restult: preemptor preempts premptee
 * 
 * submit 1 1k preemptee
 * 
 * result: preemptee gets priority reservation
 * 
 * -----------
 * case 4:
 * submit 2 512 preemptees
 * submit 1 1k  preemptee
 * 
 * submit 1 1k preemptor
 * 
 * result: preemptor preempts 1k premptee
 * 
 * submit 1 1k preemptor
 * 
 * result: preemptor gets priority reservation
 * 
 * -----------
 * case 5:
 * submit 1 128 preemptee
 * submit 3 128 preemptors
 * 
 * submit 1 128 preemptor
 * submit 1 128 preemptor
 * 
 * result: 1st preemptor preempts preemptee
 *         2nd preemptor gets priority reservation
 * 
 * -----------
 * case 6:
 * submit 4 128 preemptees
 * submit 1 512 preemptor
 * 
 * submit 1 512 preemptor
 * submit 1 512 preemptor
 * 
 * result: preemptor preempts preemptees
 *         2nd preemptor gets priority reservation 
 * -----------
 * case 7:
 * submit 1 128 preemptee
 * submit 3 128 preemptors
 * 
 * submit 2 512 preemptor
 * 
 * result: 1 preemptor get available 512 block and other gets reservation.
 * 
 * -----------
 * case 8:
 * submit 1 512 preemptee
 * 
 * submit 1 128 preemptor
 * submit 1 128 preemptor
 * 
 * result: 1st preemptor preempts preemptee
 *         2nd runs after 1st starts
 * 
 * -----------
 * case 9:
 * submit 4 128 preemptees
 * submit 1 512 preemptee
 * submit 1 1k   preemptee
 * 
 * submit 2k preemptor
 * submit 512 preemptor
 * 
 * result: all jobs are preempted by preemptor
 *         512 preemptor gets reservation
 * 
 * -----------
 * case 10:
 * submit 2 512 preemptee
 * submit 1 1k   preemptee
 * 
 * submit 2k preemptor
 * 
 * result: all jobs are preempted by preemptor. Job should not get deffered.
 *
 * -----------
 * case 11:
 * submit 1 512 preemptee
 *
 * submit 1 128 preemptor
 * submit 1 512 preemptor
 *
 * result: 128 should preempt 512 preemptee and both preemptors should run.
 *
 * -----------
 * case 12:
 * submit 4 128 preemptees
 * submit 1 512 preemptee
 *
 * submit 1 512 preemptor
 *
 * result: 512 preemptee should be preempted, since same size.
 *
 * -----------
 * case 13: (with GuaranteedPreemption)
 * submit 2 128 preemptees
 * submit 2 128 preemptors
 *
 * submit 1 128 preemptor
 * submit 1 128 preemptor
 *
 * result: both preemptors should preempt one preemptee each
 *
 * -----------
 * case 14:
 * submit 1 512 preemptee
 * submit 1 512 preemptor
 * 
 * submit 1 128 preemptor
 * submit 1 512 preemptor
 * 
 * result: 128 preemptor preempts preemptee
 *         512 preemptor get reservation
 
 *
 * the preemptor should have a reservation on the node and shouldn't preempt
 * anything else.
 */



#define GuaranteedPreemptionDoc
/**
 * @designdoc Guaranteed Preemption
 *
 * 1.0 Overview
 *   1.1 Feature/Concept Overview (what it does, why it matters)
 *
 *   LLNL wanted a way to not let preemptors start until all of the preemptees have reported
 *   completed. With a bluegene setup the following scenario is possible. Slurm reports, through 
 *   willrun, that the node is not available for several minutes because the preempted/canceled 
 *   job is still running, so the preemptor gets a reservation in the future. By the time moab 
 *   looks at the next highest priority job, the job has canceled and slurm reports that the node
 *   is now available in which the job starts ahead of the preemptor.
 *
 *   1.2 Brief Architecture Overview (how it is implemented)
 *
 *   When a job preempts in MJobSelectMNL, the preemptees are stored in a linkedlist (J->Preemptees)
 *   and the job is set with a required hostlist so that preemptor is locked to the nodes it preempted 
 *   on. Then MJobSelectMNL is forced to return failure so that the job immediately gets a reservation.
 *   At the top of the next calls to MJobSelectMNL, all the preemptees are checked for preemption
 *   completion (cancel, requeued, etc. At the time of creation, only cancel was implemented.). If
 *   any of the preemptees weren't finished then MJobSelectMNL will return failure and the job will
 *   get another reservation. This cycle keeps happening until all the jobs are finsihed at which 
 *   time moab will attempt to start the job. If the preemptees don't finish during the job's 
 *   RsvRetryTime the job will be deferred and the required hostlist and preemptee dependecies will
 *   be removed.
 *
 *   In a bluegene setup, another check for preemption completion is done in MJobAllocMNL. If any of 
 *   the jobs are not finished, then willrun will not be called so that the job can get a reservation
 *   on the nodes. Because moab knows where the job should run, moab doesn't need to ask slurm.
 *
 *   In the future if other preemption policies are requested, a check in __MJobPreemptionIsFinished should
 *   be added for requested policy. Also, at the time of preemption the preemption type used should be 
 *   recorded on the job so that the right check can be made, since it's possible to cancel a job even 
 *   though the policy is requeue.
 *
 * 2.0 Architecture
 *   2.1 Functional Flow (Review of key functions)
 *   
 *   ->MJobAllocMNL() or MJobSelectMNL()
 *    ->__MJobPreemptionIsFinished()
 *
 *   ->MJobStartRsv()
 *    ->MJobSetHold() -- RsvRetryTime exceeded
 *    ->__MJobClearPreemptees()
 *
 *   ->MJobStart()
 *    ->__MJobClearPreemptees()
 *
 *   2.2 Data Structure Overview
 *   2.3 Functional List (list of all feature functions)
 *   2.4 Limitations/Warnings
 * 3.0 Usage
 *   3.1 Required Parameters
 *   3.2 End-User Usage
 *   3.3 Reporting
 *   3.4 Dynamic Modification
 *   3.5 Checkpointing 
 *   3.6 Examples
 */



#define RsvSearchAlgoDoc
/** 
 * @designdoc Reservation Search Algorithm
 *
 * 1.0 Overview
 *   1.1 Feature/Concept Overview (what it does, why it matters)
 *
 *   Dave, please answer these questions.
 *
 *   What is RsvSearchAlgo for?
 *
 *   What is the difference between RSVSEARCHALGO WIDE and LONG?
 *
 *   Why is the default NONE/LONG? See the second NOTE: in MRLMergeTime's comment header. It seems
 *   that the wide algorithm would give more realistic ranges.
 *
 *   1.2 Brief Architecture Overview (how it is implemented)
 *
 * 2.0 Architecture
 *   2.1 Functional Flow (Review of key functions)
 *   
 *   MJobGetSNRange()
 *   -->MRLMergeTime()
 *
 *   2.2 Data Structure Overview
 *
 *   MSched.RsvSearchAlgo
 */

#define PreemptionTPNDoc
/**
 * @designdoc Preemption and per-task or per-node requests
 *
 * When considering jobs for preemption, there is no real determination of
 * the possibility of non-homogeneous per-node or per-task resource requests
 * that may have bearing on how many tasks a preempted node can give to
 * a preemptor job.
 *
 * For example, if a job requests 2 tasks, and has a pmem request that will
 * not allow those 2 tasks to reside on the same node, MJobSelectPJobList
 * in MPreempt.c does not know, or try to determine, if the resources
 * requested by the job will prevent the tasks in the job from running on the
 * same node. In most cases, Torque will prevent the job from running if the
 * per-task memory request (to use the same example) is more than can be
 * satisfied by the single node we preempted. However, this is not by any
 * means reliable, nor is it a proper way to solve the problem.
 *
 * This needs to be fixed for any resource request that can create a
 * non-homogeneous tasks-per-node restriction so when we go looking for
 * resources we preempt enough for the job to run.
 */

#define GenericMetricsDoc
/**
 *
 * Generic metrics are reported on nodes via native resource managers.  These are stored in N->XLoad.
 * 
 * These gmetrics are aggregated on each node and on the jobs that run on the nodes and stored within
 * profile statistics.
 *
 * All gmetrics on nodes (N->XLoad) are initialized to MDEF_GMETRIC_VALUE.  This signifies that the gmetric
 * is not currently being reported on the node.  Only by an RM reporting a value does this change away from
 * MDEF_GMETRIC_VALUE.
 *
 * On the other hand, profiling statistics on nodes and jobs start at 0 and are aggregated as long as the 
 * value has been set (value != MDEF_GMETRIC_VALUE).  This means that for statistical reasons if the value 
 * is 0 then it may or may not have been reported, only the user can determine whether that information is 
 * valuable.
 *
 * For triggers and real-time reports, however, a 0 means it is being reported by the RM as 0.
 * If checknode reports the value as 0 then that value was set by the RM.  
 *
 * The limit is MAXGMETRIC in the moab.cfg file (defaults to 10).
 */



#define VLANDoc
/**
 * @designdoc VLAN Management
 *
 * VLAN's are requested using the QOS.SecFlags value of XXX
 * VLAN's are created on demand w/in MJobStart() via a call to MJobCreateVLAN()
 * MJobCreateVLAN() calls MRMSystemModify() with the action 'vlancreate hostlist=X'
 * - this interface returns the VLAN id which is stored within the job's variable
 *   space using the format 'VLANID=X'
 * upon job completion, MJobProcessCompleted() calls MJobDestroyVLAN() which calls
 * MRMSystemModify() with the action 'vlandestroy <VLANID>'
 *
 * -----
 * # moab.cfg
 *
 * RMCFG[network] TYPE=native RESOURCETYPE=network FLAGS=vlan
 * RMCFG[network] CLUSTERQUERYURL=exec://XXX
 * RMCFG[network] SYSTEMMODIFYURL=exec://XXX
 *
 * QOSCFG[vlan]     SECURITY=VLAN
 * USERCFG[DEFAULT] QDEF=vlan
 * ...
 * -----
 */




#define SLADoc
/**
   @designdoc SLA Management

   SLA's can be of two types, application-level SLA's and system-level SLAs.
   There is currently only one system-level SLA, power usage, and it has
   a hard and soft upper bound. There are currently several application SLA's
   that are defined by a soft target range and a hard target range, where the
   soft range is a subrange of the hard range (hard_lower <= soft_lower <=
   soft_upper <= hard_upper )>. Each object that is subject to an SLA also has a current
   value relative to that SLA. For instance, an application job with an APPRESPONSETIME
   SLA has a current application response time value associated with it.

   The basic algorithm for handling jobs based on these two SLA types is as follows:

   1) Determine the overall SLA scheduling mode based on the current system SLAs.
      The possible modes are as follows:

     Soft-satisfied: All current values are below their respective SLA soft upper limits.

     Hard-satisfied: All current values are at most below their hard upper limits.

     None-satisfied: At least one current value is above it's respective SLA hard
     upper limit.

   2) Determine the current resource requirement level (SLA status) of each
      job running an application with an associated SLA. A job can be in five
      possible states:

      Lower-hard-bound-violated: The current value is below it's SLA hard lower bound.

      Lower-soft-bound-violated: The current value is above it's SLA hard lower bound but
      below it's SLA soft lower bound.

      In-soft-range: the current value is inside it's soft SLA target range.

      Upper-soft-bound-violated: The current value is above it's SLA soft upper bound
      but below it's SLA hard upper bound.

      Upper-hard-bound-violated: The current value is above it's SLA


      For scheduling purpose, we are only concerned with first two states. Jobs
      are then divided into one of three categories, where the first two categories
      are the first two states listed above and the third category corresponds to
      the other three states.

      TODO: the above model assumes the job has only one SLA, when it in fact
       can have multiple. How should we resolve conflicting SLA statuses? Pessimistically?

      TODO: how is the scheduling information found in step two used in
        subsequent steps?

   3) Work to satisfy per-job application hard limits. If a job needs to shrink to
      meet hard upper limits, go ahead and shrink it. If a job needs to grow
      to meet a hard lower limit, the action to be taken will depend on the current
      scheduling state determined in step one.

      Soft-satisfied: schedule as many resources as are available.
       TODO: Shrink other jobs if enough resources aren't available?

      Hard-satisfied: do not allocate more resources???

      None-violated: Do not allocate more resources. Shrink resources to lower soft limits maybe??


   4) Work to satisfy per-job application soft limits.
      TODO: flesh out this section more, though it should be similar to step 3.
      TODO: optimize the algorithm to not send two resource manager requests
       for any job.



   The following behavior should be enforced.

   1) If all system-level SLA soft+hard targets are satisfied, Moab should
      attempt to satisfy application-level SLA soft+hard targets.

   2) If only system-level SLA hard targets are satisfied, Moab should attempt
      to satisfy only application-level SLA hard targets.  If Moab determines
      that one or more application-level SLA hard targets cannot be satisfied,
      Moab should attempt to back off one application per iteration to its
      application-level SLA hard target lower bound.

   3) If hard system-level SLA targets are not satisifed, Moab should not
      attempt to handle any application-level SLA upper targets.  Rather, Moab
      should actively attempt to back applications down to their application-
      level hard SLA target lower bound.

   These behaviors should be accomplished through the introduction of new
   global tracking variables on a per-metric basis and a final overarcing
   system-level SLA tracking value where the overarching value is a pessimistic
   summary value indicating the lowest SLA level achieved.

   @see enum MSLATargetTypeEnum { mslattNONE = 0, mslattHard, mslattSoft }
   @see msched_t.SystemPowerSLALevelAchieved
   @see msched_t.SystemSLALevelAchieved

   NOTE:  each application SLA should likewise have a 'level-achieved' status
          variable as well as a single overarching SLA 'level-achieved' status
          variable inidicating the application's most pessimistic SLA status
          to be used as a convenience variable in determing per-application
          and system level actions.

 */





#define LearnDoc
/**
 @designdoc Learning

   Moab can learn per-application attributes relevant to improving subsequent
 scheduling such as average I/O activity, max WC duration, and per-server
 reliability, app performance, and power consumption.

 @see mjtx_t.{LMultiplier,WallTime,Memory,CPULoad,IOLoad,SampleCount}

==============================================
 Application Based Learning (Deprecated?)

 @see mnpcApplFailureRate,mnpcApplWattUsage
 @see struct mnappb_t
 @see mnode_t.ApplStat (linked list of mnappb_t)
 @see #define MMIN_APPLSAMPLES

 @see MNodeApplStatsToXML() - report application learning via XML
 @see MStatResetAllApplStats() - peer
==============================================
 Job Template Based Learning

  Job template based learning of per-server failure, power, and performance information
  is enabled by default.  It is activated by setting the 'X', 'Y', and 'Z' coefficients
  within the PRIORITYF attribute.

 @see #define MMAX_LEARNDEPTH
 @see #define MMAX_LEARNINTERVAL

 @see mnpcFailRate,mnpcPerf

 @see J->TStats
 @see struct mtjobstat_t
   HostPerf[][]
   - updated in MStatUpdateCompletedJobUsage()
   - based on mjob_t.Size if specified, otherwise based on <PSREQUEST>
   - calculated as <SIZE> / <PSRUN>

   HostFail[][]
   - updated in MStatUpdateCompletedJobUsage()
   - incremented if job not cancelled and job state is removed

   HostPower[][]
   - NYI

   HostReject[]
   - updated in MStatUpdateTerminatedJobUsage()
   - NOTE:  only set if job has failed EVERY time on specified server for
     at least MMAX_TEMPLATEFAILURE samples - make configurable (FIXME)
   - only set for master node of job
   - applied in MReqGetFNL() - NOTE:  currently applied to all jobs with
     templates - make configurable (FIXME)

 @see JT->xd (job template extension data) which is cast as mtjobstat_t *
 @see JT->xd->L

 @see MMIN_TEMPLATESAMPLECOUNT - NOTE:  make dynamic and per application - NYI

 @see JT->xd->{HostPerf,HostSample,HostFail,...}
 @see N->Stat.ApplStat - linked list containing pointers to mnapp_t structures
 @see mnapp_t (FailCount,SampleCount,...)

 NOTE: mtjobstat_t.{HostPerf,HostFail,HostSample} are two dimensional arrays
       incorporating 'learn depth' and 'per-server' aspects.  Currently, learn
       depth rolling is NOT enabled so all stats roll into the depth==0 field
       (FIXME)

 NOTE: mtjobstat_t does not yet support HostPower. (FIXME)

 @see MNodeGetPriority() - apply learning statistics to server allocation
                           decisions (mnpcPerf,mnpcFailure)
                           Adjust node allocation priority by CW[mnpcPerf] * HostPerf / HostSample


 @see MStatUpdateActiveJobUsage() - update template active usage stats J->TStats->Ptr->xd->{F,S}
 @see MStatUpdateTerminatedJobUsage() - update template job stats J->TStats->Ptr->xd->S
 @see MReqGetFNL() - reject jobs - mtjobstat_t.HostReject
 @see MJobSetDefaults() - associate 'JSTAT' templates with job
 @see MTJobStatAlloc() - allocates template job stat structures
   NOTE:  structures currently static - FIXME


-----
# moab.cfg

JOBMATCHCFG[X] JMIN=X JSTAT=Y
JOBMATCHCFG[X] JSET=cres
JOBCFG[cres]   MEM=1.1*$LEARN WCLIMIT=1.2*$LEARN

NODECFG[DEFAULT] PRIORITYF='X*FAILRATE+Y*PERF+Z*POWER'
-----

  Note:  learn app failure mtjobstat_t.HostReject (how?)
 */



#define MoabHALockFile
/**
@designdoc Moab HA Lock Files

1.0 Overview
------------

We have added some additional functionality to the existing HA lock file system in order to protect against issues that may occur if the NFS file locking mechanism fails. Common reasons that NFS locking could fail include:

  - Hard reboot of primary Moab host
  - Deleting the lock file
  - General failure of NFS or distributed locking daemons

Moab's "old" HA lock file mechanism consisted of two phases. When a Moab daemon
started up it would enter Phase 1 and proceed to Phase 2, if it could become
the primary Moab. Here is an outline of the old phases:

Phase 1: Wait to become Primary Server

  - Open lock file to ensure we can access it (permissions, etc.) This
    will also create the file if it does not already exist. Close lock file.

  - If lock file is not accessible, abort Moab and print to stderr.

  - If the lock is accessible:
      a) Attempt to lock it via fcntl (block until we can)
      b) Once we acquire lock, move to Phase 2.

Phase 2: Active scheduling as Primary Server

  - The primary server will perform active scheduling. It keeps the primary
lock file FD open to ensure that a file system lock remains.

This model depends strictly on the fcntl() advisory locking feature. If any of
the above problems occur, it is possible that this feature will not work as
expected and two primary Moab's could be running at the same time.

The new model uses an extra "alternate" lock file to more accurately detect
that a primary Moab is running, even if NFS locking becomes flaky. A second
lock file is needed because we cannot use the original lock file to read/write
hostnames to and check modify times. The main reason we can't use the original
lock file is because we MUST never close ANY file descriptor pointing to the
inode of the original lock file or we will lose the fcntl() lock. If the
original lock file is deleted, the inode and open file descriptors stay in
place, but the secondary Moab will get permission from the file system to
create a new lock file (and new inode) and lock it using fcntl(). So, we need
to have another file which can be freely closed and opened to ensure we are not
reading from a file inode that is no longer part of a valid file.

Note that this new lockfile behavior is supplementary to the existing fcntl()
locking method. If NFS locking is working, the fcntl() locking method is
superior and is the best way for Moab to handle failover.

Here is the new flow of operations with the added locking protection:

Phase 1:  Wait to become Primary Server

  - Open lock file to ensure we can access it (permissions, etc.) This
    will also create the file if it does not already exist. Close lock file.

  - If lock file is not accessible, abort Moab and print to stderr.

  - If the lock is accessible:
      a) Attempt to lock it via fcntl (block until we can)
      b) Open up the "alternate" lock file. (This file is the one that is monitored for its update time and contents.)
      c) Read hostname from alternate lock file--does it match ours or is empty? If not, then fall through to next super-step. If yes, then enter Phase 2.
      d) If hostname is empty, write ours in to alternate lock file.
      e) Close alternate lock file. (This step is important to avoid stale NFS descriptors if the lock files are deleted.)

### This step is a "backup" step to help if NFS locking isn't working. ###
  - If the alternate lock has a different hostname than our own:
       a) Wait for some period and check if the alt. lockfile has been updated.
       b) If it hasn't been updated, write our hostname in it, close it, and become the primary server. Enter Phase 2.
       c) Else, loop back to a)

Phase 2:  Active scheduling as Primary Server

  - The primary server will perform active scheduling. It keeps the primary lock file FD open to ensure that a file system lock remains.

### The below steps protect against NFS file locks failing or the lock file being deleted. ###
  - Periodically will:
    a) Open alt. lockfile and checks hostname contents. If the file cannot be opened, go to Phase 3.
    b) If the hostname does not match, go to Phase 3.
    c) Close alt. lock file.
    d) Update the alt. lock file's "modify" time.
    e) Continue scheduling

Phase 3: Immediate Restart
  - Moab will perform a hard restart, avoiding any checkpointing.
    (Checkpointing could corrupt the already running primary Moab's state.)
  - After restart, Moab will be back in Phase 1.

In initial tests, everything seems to work as expected.

There is one case, however, that will result in the primary Moab shutting down
and the secondary Moab remaining in Phase 1 indefinitely. This occurs when the
directory housing the lockfiles is removed, the permissions are changed, the
mount is lost, etc. What will happen is the primary Moab, during Phase 2, will
attempt to check the alt. lock file. This will result in an error and the
primary Moab will go to Phase 3. Phase 3 leads to Phase 1. The first step in
Phase 1 is to check the validity of the lockfile. This will fail, however,
because we cannot recreate the lockfile (bad permissions, directory/mount
removed). Moab will then abort and print an error to stderr.

In this case, the secondary Moab will remain in Phase 1, waiting for the chance
to acquire a fcntl() lock on a file that can no longer be reached. Once the
path to the file is restored, however, the secondary will take over and enter
Phase 2.

2.0 Architecture
----------------

The following functions are key to the HA lock file system:

@see MSysIsHALockFileValid()
@see MSysAcquireHALockFile()
@see MFUAcquireLock()
@see __MSysUpdateHALock()
 */



#define CalDoc
/**
@designdoc Calendaring

Moab can maintain calendars on key objects such as jobs
@see mjob_t.Cal
@see mcal_t
@see enum MCalTypeEnum
@see mxaRange - used to specify time ranges during which job is eligible to
                 start

 @see MCalFromString()
 @see MJobProcessExtensionString() - process mxaRange and populate J->Cal

  NOTE:  is this checkpointed directly or via J->RMX?
*/






#define MoabJournalSystem
/**
@designdoc Moab HA Checkpoint/Journaling System

1.0 Overview (and some Architecture)
------------------------------------
NOTE: This is not to be confused with a parallel journaling system used
to store job submission requests. Please see MoabSubmitJournal for more
information!

There are two different types of checkpointing: file and database. The below
description only covers file checkpointing, but the journaling system would
work just as well with a database model.

Currently, Moab maintains checkpoint files in order to persist information
about jobs to disk. There are two types of checkpoint files:

1) The .moab.ck file located in the Moab home directory. This file contains the
information about compute jobs, reservations, nodes, configured resource
managers, etc. Much of this information is supplemental, in that it is used in
conjunction with information coming back from TORQUE or the nodes themselves.
In other words, the records contained in the .moab.ck file are much more
compact than those found in the second type of checkpoint files.

2) The *.cp files located in the Moab spool directory. Each *.cp file
corresponds with every non-migrated Moab job--in other words, jobs that are
only present in Moab and do not actually have a presence in any resource
manager. In Yahoo's case, this includes the trigger jobs (Moab system jobs).
These checkpoint files contain ALL the info about the jobs, as they exist only
in Moab, and if the data is not persisted out to disk, then Moab will "forget"
about the job when restarted.

Moab will write out to checkpoint files under certain conditions:

* Right after a job is submitted to Moab. For example, submitting a job with
"msub" will result in Moab immediately creating a checkpint file for the job.

* Right after a job is initially discovered by Moab. For example, submitting a
job with qsub will result in Moab discovering it during the next scheduling
iteration and creating an entry in the .moab.ck file for that job.

* If the "CheckPointInterval" timer expires. The CheckPointInterval is a
setting that can be changed via the moab.cfg file or at runtime via
changeparam. It is specified in seconds. After X seconds, Moab will checkpoint
the ENTIRE system--saving everything to disk. This, of course, can be expensive
as the number of jobs, triggers, and variables gets large.

* If Moab is shutdown cleanly. Right before shutting down the Moab daemon, a
full checkpoint will occur (similar to what happens after a CheckPointInterval
timer expires).

The problem with this model is that in order to persist information to disk
Moab must serialize its internal data structures into XML and then write the
entire string out to disk. This can be time consuming, especially with
thousands of jobs and files. In fact, we tried full checkpointing of jobs after
every significant change (like variable changes) and performance was severely
impacted.

Our solution to this problem is the "checkpoint journaling system." Moab will
maintain a single journal file. This is a simple text file. When a job (or
other important system characteristic) changes, and Moab needs to save this
info to disk in case of a crash, Moab will write out only that specific change
to the journal system, instead of saving the entire object. In essence, we only
record what part of the object was changed. Then, if there is a crash, Moab
will load all the info from the checkpoint files that it has and then it will
consult the journal file. It will walk through the journal file and re-perform
all of the recorded changes.

If Moab DOES perform a full checkpoint due to one of the above circumstances,
then the journal file will be cleared, as all of the changes will already be
included in the full checkpoint.

Here is an example:

A trigger job is submitted via msub. Let's call it Moab.5. Moab will create a
Moab.5.cp file for this job and save all of the initial info about it. A few
seconds later, Moab starts the MAIN trigger attached to Moab.5. Jobloader
inserts a compute job--let's call it 1012. Moab discovers the compute job and
an entry is made in the .moab.ck file for it. The compute job finishes and
variables are set within Moab.5 to indicate the completion code of the compute
job. When the variable is set, Moab appends the following to the journal file:

FORMAT: <OBJECT TYPE>:<OBJECT ID>:<MODE>:<ATTR NAME>:<ATTR VALUE>

job:Moab.5:set:var:ccode=0

This basically records the fact that we changed the variable ccode to 0 for
Moab.5. Let's say that a few moments later, an admin purposely sets the ccode
variable on Moab.5 to 7. The following would then be appended to the journal
file:

job:Moab.5:set:var:ccode=7

Then let's say Moab crashes before a full checkpoint of this job can be made.
When Moab boots up, it reads in the Moab.5.cp file and reconstructs the Moab.5
job (it will do this for all other objects as well). After this initial
checkpoint load, it will then go to the journal file and start re-executing
each of the recorded actions in the file. They are executed in sequential
order, from top to bottom. This will therefore cause the ccode to end up being
set to 7, which is accurate.

By keeping all records in a single file, we can keep the file open and the
position pointer at the end of the file. This helps avoid the overhead of
creating hundreds or thousands of new files and having to call open() and
close() system calls each time.

Once again, when a full checkpoint of the system occurs, the journal file will
be cleared.

For now, we only have Moab write records out for actions that we feel are
critical to the proper functioning of the system:
  - variable modifications
  - internal RM counter

The list of journaled actions/data can be expanded as the need arises.

2.0 Architecture
----------------

The following functions are key to the checkpoint journaling system:

@see MCPJournalOpen()
@see MCPJournalClose()
@see MCPJournalClear() - clears out journal
@see MCPJournalAdd() - adds/appends entry to journal
@see MCPJournalLoad() - loads all entries and replays them

2.4 Limitations/Warnings

When loading and replaying entries from the journal file you MUST disable journaling, otherwise
when you replay a 'set variable' action it would create a new journal entry to set the variable again,
and an infinite loop will result.
*/





#define RMDoc
/**
@designdoc Moab MultiRM Management 

1.0 Overview (and some Architecture)
------------------------------------
Both resources (mnode_t, mvm_t) and workload can have multiple resource manager 
information and management sources.  

Resources
  mnode_t.RM - master RM associated w/resource
  mnode_t.RMList - list of RM's associated w/resource
  mrm_t.IFlags - mrmifBecomeMaster - resources reported via this RM will have 
    this RM assigned as master even if the resource was initially discovered/
    reported via an alternate RM

@see MNodeAddRM() - adds RM to node, updates N->RM, N->RMList[]
*/







#define StorageDoc
/**
@designdoc Storage Management

1.0 Overview (and some Architecture)
------------------------------------
The storage management facility allows storage resources to be monitored, 
allocated, tracked, and provisioned.

Compute nodes which can support dynamic provisioning of specific types of dynamic 
storage, should have matching gres's (or features) defined.  A job requesting a
particular storage type would request storage resources with that gres.

If access to all storage types is not universally available, node gres's or 
features could be associated with the job's compute resources and resource 
allocation could be correspondingly constrained

msched_t.DefStorageRM

- MJobStart()/MJobCreateStorage()

@see MJobCreateStorage()

  This routine pushes the following info into the RM request:

    storagenode, hostlist, jobid, user, group, account, gres, ...

  NOTE:  request routes throught the 'createstorage' subcommand of the MRMSystemModifyURL on the
         associated job storage RM.  If the 'systemmodify' call returns any string via stdout,
         this string will be applied to the job as 'MOAB_STORAGEID=<STRING>' within the job's
         variable and environment space.

  NOTE:  if using TORQUE, any MOAB_* variables can be automatically pushed into the job's
         root executed prolog and root level operations such as mounting a specified filesystem
         or importing the specified storage space can be accomplished and/or validated prior to
         job execution

  
@see MJobDestroyStorage()

  NOTE:  request routes throught the 'destroystorage' subcommand of the MRMSystemModifyURL on the
         associated job storage RM.

@see enum MSecFlagEnum msfDynamicStorage

  set via QOSCFG[] security=dynamicstorage
  NOTE:  must be set to have job push dynamic storage request to external storage RM

@see enum MRMFlagEnum mrmfDynamicStorage

  set via RMCFG[] FLAGS=dynamicstorage
  NOTE:  indicates 'storage' RM is capable of handling dynamic storage allocation/
         provisioning requests

  TODO:  enable 'per-job' storage RM specification
  TODO:  enable support for multiple storage RM's

  TODO:  checkpoint mjifDestroyDynamicStorage
  TODO:  execute MJobDestroyDynamicStorage() on a requeue operation

  Model:  1 storage RM, multiple storage nodes, each with gres, and feature.  All containers located
          on given node reported by storage RM have similar storage types (features?)  need to make 
          certain the a job's storage req with gres consumable resources and corresponding feature 
          can have gres's enforced on non-compute resource

  TODO:  need to query storage RM to determine resources available.  For ML, storage RM groups all
         storage resources with similar attributes to single node, and all containers are represented
         as gres instances w/in each node?


          -----------------------------------------------------
          |                     StorageRM                     |
          -----------------------------------------------------
           |                       |                         |
           |                       |                         |
          node1 (featureA)        ...                       nodeN (featureX)
           |   \                                             |   \
           |    \                                            |    \
          gres1  gres2                                      gresX  gresY

*/



#define 2StageProvisionDoc
/**
@designdoc Two Stage Provisioning

In 2 stage provisioning a node must first be either powered on or provisioned to the 
correct OS.  After that stage then virtual machines must be created on the node.  The old
model for 2 stage provisioning had 1 system job handle both tasks.  It would carry out 
the first, then switch and carry out the second (used msjtOSProvision2).  

The new model will use separate system jobs for each task.  When MJobStart() is called
MAdaptiveJobStart() will check to see if the nodes need to be provisioned.  If they do
then a provisioning a provisioning job is created and the real job waits.  When that's done
MJobStart() and MAdaptiveJobStart() are called again and it is determined that virtual
machines need to be created.

MReqCheckResourceMatch will make sure that the node has the correct OS and if the node is
currently being provisioned MJobGetSNRange() will ensure that it is being provisioned to
the right hypervisor.
*/




/**
#define MJobCtl
@designdoc mjobctl command

mjobctl is the primary interface for jobs. 

mjobctl -m: this command allows the user to modify job attributes (if they have
sufficient permissions to do so).  

As of 5.4 the user can clear a job's account information by setting it to NONE, 
i.e.:

mjobctl -m account=NONE <jobname>

This is a new feature and will need to be documented on the release of 5.4

*/

#define SNLDoc

/**
 * @designdoc Sparse Numbered List SNLDoc
 *
 * Sparse Numbered Lists replace Moab's old way of keeping track of generic resources.
 * Moab used to have a static array where the index was the type and the count
 * was the number of resources available.  This is changing to be more
 * dynamic and to also use less space.  Now, using the MSNL routines Moab can easily
 * add and remove resources and the SNL can grow as needed.
 *
 * MSNLGetIndexCount()
 * MSNLIsEmpty()
 * MSNLSetCount()
 * MSNLRefreshTotal()
 *
 * For example, to see how much of a particular gres a node has you can do the following:
 *
 * int gindex = MUMAGetIndex(meGRes,"blue",mVerify);
 *
 * int count  = MSNLGetIndexCount(&N->CRes.GenericRes,gindex);
 *
 * To set a gres value you can do the following:
 * 
 * MSNLSetCount(&N->CRes.GenericRes,gindex,12);
 * 
 */ 



#define OCDoc
/**
@designdoc Resource Overcommit Management 

  With resource overcommit, Moab can allocate more workload to a server than 
can be properly guaranteed.  Ideally, only jobs which can be live-migrated 
should be able to take advantage of overcommit resources.  Overcommit factors
can be specified on a per-node, per-resource type basis using the NODECFG[]
attribute 'OVERCOMMIT'

  @see NODERESOURCEUTILIZATIONPOLICY 
 
  If set on the specific node or the default node, the overcommit factor 
will be applied to the specified node resource at the time the resource is loaded
via the RM (as of 7/30/09, only supported within TORQUE/PBS and WIKI/Native)

  functions:

  @see MNodeGetOCFactor() - return associated scale factor
  @see MNodeGetOCThreshold() - return associated scale threshold
  @see MNodeGetOCValue() - return scaled value
  @see MCResGetBase() - report unscaled resource values
  @see MNodeShowState() - reports original unscaled resource values
  @see MUResFactorArrayFromString() - used to parse 'OVERCOMMIT' parameter 
       values
  @see MUResFactorArrayToString() - used to report 'OVERCOMMIT' parameter
       values

  data:

  @see mnaResOvercommitFactor/mnode_t.ResOvercommitFactor[] - node-specific 
       resource overcommit scaling factors
  @see MSched.ResOvercommitSpecified - set if OVERCOMMIT factor is set for any
       resource or any resource type
  @see mcoVMOvercommitThreshold/MSched.VMResOCMigrationThreshold - indicates 
       proximity to overcommit limits at which Moab takes active migration 
       action - deprecated - now use NODECFG[] VMOCTHRESHOLD=...
  @see mcoNodeCPUOverCommitFactor, mcoNodeMemOverCommitFactor - deprecated -
       maintain support for old-style overcommit functionality

  notes:
  
  NOTE:  VM resources not scaled (@see mnifIsVM) - enforced w/in MNodeGetOCFactor()
         and MNodeGetOCValue()
  
  NOTE:  additional interfaces (mdiag -n, etc) must modify OC scaled node
         resources before reporting these to external consumers.  Currently, 
         only 'checknode -v' reports original values.

  NOTE:  configured resources should be scaled, available resources as reported
         by RM's should not be.

  NOTE:  when comparing workload requirements to available resources, the job's
         resources requirements should be reverse scaled to determine if they
         will fit within the available resource space at the same scale factor 
         (NYI)

  NOTE:  NODEAVAILABILITYPOLICY should be set to 'utilized'

  NOTE:  should not migrate unless migration will allow destination server to 
         still be within its utilization targets

  NOTE:  should not migrate if destination server is identical in configured
         resources and only single job is running

  There are four primary aspects of overcommit management:

  1) monitoring/applying it
  2) allowing it to occur in a controlled manner
  3) taking action when it exceeds certain specified bounds
  4) reporting original base node resource values when queried

  Item one is addressed through standard RM interfaces, namely TORQUE and WIKI.
  see MNodePostUpdate(), MNodeAdjustAvailResources()

  Item two is addressed by specifying two primary factors, a dedicated resource
  overcommit factor (NODECFG[] overcommit=X) and an available resource 
  utilization factor (VMOCTHRESHOLD).  Jobs should be allowed to start so long
  as neither the resource utilization factor nor the dedicated resource 
  overcommit factor is violated.  (see MNodeGetAvailTC() within MJobGetINL()
  NOTE:  MNodeGetAvailTC() is only called w/in MJobGetINL() if 'NODECFG[*]
  OVERCOMMIT=<Y>' is specified.

  If a given job cannot start immediately, it should locate a server/time 
  combination which allows it to start.  If the server currently exceeds its
  available resource utilization threshold, the reservation should not be
  allowed to start any sooner than the completion of the first job currently
  running on the target node.  (see MJobGetSNRange()->MJobGetNodeStateDelay())

  Item three is addressed within MVMSchedulePolicyBasedMigrations().  This
  routine should examine all nodes and sort them in order of resource 
  utilization overcommit factor (see MNodeGetEffectiveLoad()).  Once the 
  sorted list is available, Moab should identify those which are overcommitted
  from either a utilized or dedicated resource, and use MVMSelectDestination()
  to identify the best target.  MVMSelectDestination() should again utilize
  MJobSelectMNL()->MJobGetINL()->MNodeGetAvailTC() to verify that the target
  migration server can support the migrated VM.
 
  Item four is applied to the 'checknode' command only.  It must be applied 
  to mdiag -n (human/xml), and others.  This will be a hairball.
 
  DOUG:  verify 'NODEAVAILABILITYPOLICY utilized' works
         verify overcommit job start works with MNodeGetAvailTC()
         verify migration in MVMSchedulePolicyBasedMigrations()  
         enable additional command to restore original resource values
         - see MCResGetBase(), MNodeGetBaseValue(), MNodeShowState()


------------
#moab.cfg

NODEAVAILABILITYPOLICY utilized
VMMIGRATIONPOLICY      overcommit

# if actual utilization gets within specified range, initiate migration
VMOCTHRESHOLD          procs:0.9,mem:0.85

# specify per-server overcommit targets
NODECFG[DEFAULT]       overcommit=procs:6,mem:2
NODECFG[node13]        overcommit=procs:4,mem:1.5
...
------------
*/





#define ReportEventDoc
/**
@designdoc Report Event Handling

@see MSysRegEvent()
@see MSysLaunchAction()
@see MSysRegExtEvent()

@see enum MEventFlagsEnum
@see enum MSchedActionEnum

@see mcoMailAction ("MAILPROGRAM")
@see mcoAdminEAction ("NOTIFICATIONPROGRAM")
@see mcoAdminEInterval ("NOTIFICATIONINTERVAL")

@see MSched.ActionInterval
@see MSched.Action[]
*/



#define PlacerholderRsvDoc
/**
@design http://cos/wiki/index.php/Daedalus_Request_VPC_Design_Specification


mtrans_t
*/




#define SimDoc
/**

AutoStop - enabled by default in siumlation, takes action when the active queue and idle queue are empty.  Autostop is activated by setting scheduler state to Stopped  NOTE:  If AutoShutdown is enabled (which it is by default) then Moab is shutdown on the same trigger event causing AutoStop to be effectively ignored
*/



/** 
 *Job Preemption (Old Style, MJobSelectPJobList)
 *
 * The old style preemption makes a terrible assumption that each task of the preemptor and the preemptee
 * this means that a job that needs more than one proc per task or memory, disk, swap, or any gres then
 * preemption does not work.
 *
 * MJobSelectMNL() has 3 arrays job array, taskcount array, nodecount array.  For each job, we define how many
 * preemptor tasks and how many nodes are gained by preempting that job.  These arrays are built in MJobSelectPJobList
 * but no conversion is done between preemptor and preemptee task definitions.  Finally, once the arrays are built up
 * MJobSelectMNL() will loop through the array preempting the jobs in order and summing up the taskcounts until it
 * determines enough jobs have been preempted and then it goes to start the job.  
 *
 * The order of the jobs is determined by how many tasks are gained by each job and the priority of the job.  This means
 * that things like nodesets and completely ignored.
 */

/**
 * Job Preemption (New Style, MJobSelectPreemptees)
 *
 * The new style of preemption still goes through MJobSelectMNL() but in MJobSelectPreemptees() the resources gained by 
 * each preemptee are added into a "mcres_t" bucket.  Once the bucket is full we then see how many preemptor tasks are gained
 * by the bucket.  All the jobs in the bucket are then preempted, whether or not they actually contribute any resources
 * to the preemptor.  The result is you may preempt more jobs than necessary.  There is a check to make sure that at least
 * some resources are contributed.  MJobSelectPreemptees() then builds up the 3 arrays needed by MJobSelectMNL().  However,
 * because some jobs will contribute less than 1 task the list of taskcounts is 0 for some jobs and higher for others.  This 
 * is done to ensure that the loop in MJobSelectMNL() keeps preempting until all the tasks have been preempted and the job 
 * can start.
 *
 * TODO: there is still a lot to be done.  Moab should be intelligent in determine which combinations of jobs are needed 
 * in order to gain the most tasks.  This is kind of like a napsack algorithm in deciding the best combination of jobs
 * to get the most resources.
 */

/**
 * RsvSysJobTemplates
 *
 * right now only the nodeaccesspolicy from a job template can be put onto the 
 * systemjob of a reservation.
 */





#define CLIENTFAILREPORT

/**
 * @designdoc Client Failure Reporting 
 *
 * 1.0 Overview
 *
 * 1.1 Feature/Concept Overview
 *
 *   Client Commands must report failure messages in a consistent manner.  
 *   Commands requesting XML output must combine both human readable and 
 *   machine readable information in an actionable format.  In particular, this
 *   output must be usable by Moab Viewpoint to allow automated responses AND
 *   allow users of this system to understand the needed context of the 
 *   failure.
 *
 * 1.2 Brief Architecture Overview
 *
 *   In the event of a service failure, Moab clients should report human 
 *   readable failure messages to stderr.  In the event of a client requesting
 *   XML output, the XML should follow the S3 'Status' element guidelines, ie,
 *
 *   <Status>
 *     <Value>{Success|Failure}</Value>
 *     <Code>INTEGER</Code>
 *     <Message>STRING</Message>
 *   </Status>
 *
 * Code values will follow the S3 failure code guidelines as described in the
 * S3 RMAP Message Format Document at http://www.clusterresources.com/products/gold/docs/SSSRMAP%20Message%20Format%203.0.4.doc
 *   (see sections 3.2.4 for example and section 4.0 for description)
 *
 * enum MSFC
 *
 * @see MS3CheckStatus()
 * @see MS3SetStatus()
 * @see enum MSFC
 *
 * @see MUISSetStatus()
 *
 * 2.0 Architecture
 */

#define NAMI

/**
 * @designdoc Native Accounting Manager Interface
 *
 * This feature introduces allocation management middleware to the Moab architecture 
 * that would permit the removal of allocation management software from Moab itself 
 * and places it into separate middleware and accounting manager software, the former 
 * of which would interface Moab with any accounting manager using a native Wiki interface.
 *
 * This allocation management middleware layer removes the responsibility for allocation 
 * management from Moab and rightly replaces it with a native interface that permits a 
 * separate accounting manager to perform the allocation management functions (e.g., 
 * charging, billing, charge queries, etc). This native allocation management interface 
 * will permit customers to easily interface with their own allocation management 
 * systems and will move maintenance of such functions from the core Moab development 
 * group to the more appropriate Integration group. 
 *
 * Moab currently supports the following URLs:
 *
 * CREATE
 * DELETE
 * QUOTE
 * RESERVE
 * CHARGE
 *
 * The design for NAMI is different from Gold.  It is currently designed to take the
 * bulk of the logic out of Moab and place it in the native software.  Moab acts as the
 * event engine for the native software.  A user runs "mshow" and Moab calls to NAMI
 * to get the QUOTE for the requested resources.  If the TID is committed then Moab calls
 * the CREATE and RESERVE URLS for each object (reservation or job).  Depending on the
 * flush interval Moab will periodically call out to the CHARGE URL.  When the object
 * has reached its end of life Moab calls out to CHARGE and finally DELETE.  Moab keeps
 * track of the last time the object was charged but it does not re-create reservations
 * when restarting nor for intermittent charging.  If Moab is down during a flush interval
 * then Moab will not attempt to catch up, it will simply charge double the next flush
 * interval.
 *
 * Moab sends XML to each of these URLs.  The XML is nothing more than the definition
 * of the object.  
 *
 * Here is some sample XML for the life of a particular object QUOTE->DELETE
 *
 * quote    <Reservation><ObjectID>cost</ObjectID><Processors>1</Processors><WallDuration>12960000</WallDuration><ChargeDuration>12960000</ChargeDuration></Reservation> 
 * create  <Reservation><ObjectID>host.1</ObjectID><User>test</User><Processors>1</Processors><WallDuration>12959999</WallDuration><ChargeDuration>12959999</ChargeDuration></Reservation> 
 * reserve  <Reservation><ObjectID>host.1</ObjectID><User>test</User><Account>blue</Account><Processors>1</Processors><WallDuration>12959999</WallDuration><ChargeDuration>12959999</ChargeDuration><Var name="VPCHOSTLIST">n05</Var><Var name="VPCID">vpc.1</Var></Reservation> 
 * charge  <Reservation><ObjectID>host.2</ObjectID><User>test</User><Account>blue</Account><WallDuration>12959999</WallDuration><ChargeDuration>108</ChargeDuration><Var name="blue">green</Var><Var name="VPCHOSTLIST">n05,GLOBAL</Var><Var name="VPCID">vpc.1</Var><GRes name="storage">100</GRes></Reservation> 
 * delete  <Reservation><ObjectID>host.2</ObjectID><User>test</User><Account>blue</Account><WallDuration>12959999</WallDuration><ChargeDuration>12959999</ChargeDuration><Var name="blue">green</Var><Var name="VPCHOSTLIST">n05,GLOBAL</Var><Var name="VPCID">vpc.1</Var><GRes name="storage">100</GRes></Reservation> 
 *
 * The NAMI calls originate in either MRsvGetAllocCost() or MRsvChargeAllocation().
 * These routines then call out to MAMNativeDoCommand() which calls MAMOToXML() to 
 * convert the object to XML and then place that XML as STDIN to each of the routines.
 * Not parameters are passed into the URLs.  
 * 
 * Currently only the QUOTE should return any information.  It should return nothing more than
 * the cost of the object, no words, just the cost.
 *
 * This model has not been tested with HPC workload.  It has only been tested with 
 * VPC style clouds.
 *
 */

/* END moabdocs.h */



#define GEventDoc
/**
 * @designdoc Generic Events Implementation
 *
 * 1.0 Overview
 * 
 * 1.1 Feature/Concept Overview
 *
 *   Generic events are arbitrary events reported by a resource manager and may
 * have messages attached to them.  These events may be linked with a scheduler
 * action.
 *
 * 1.2 Brief Architecture Overview
 *
 *   Generic events are reported via the Wiki interface and parsed by Moab.
 * See mgevent_desc_t
 * See mgevent_obj_t
 *
 * 2.0 Architecture
 *
 * 2.1 Key Functions
 *
 * MWikiNodeUpdateAttr()
 *   Parses the gevent wiki and places it onto the node.
 *
 * MSchedAddGEvent()
 *   Checks the MSched.GEvents hash to see if it contains the given gevent,
 * adds it if it does not already exist.
 *
 * MObjAddGEvent()
 *   Checks the given hash table (Node->GEvents or VM->GEvents) to see if
 * it contains the given gevent.  If not, it is added into the hash.
 *
 * 2.2 Data Structure Overview
 *
 * mgevent_desc_t
 *   This struct describes a gevent, such as the scheduler action when this
 *   gevent is fired, rearm time, how many times it has been fired, etc.
 *   It is held in the MSched.GEvents hash table (mhash_t).  You should use
 *   MSchedAddGEvent() to add a new event or ensure that the given event exists.
 *
 * mgevent_obj_t
 *   This struct defines a specific reported instance of a gevent.  These are 
 *   stored in the GEvents mhash_t on nodes and virtual machines.  Use
 *   MObjAddGEvent() to add a new event or ensure that it already exists.
 *
 * 2.3 Functional List
 * 2.4 Limitations/Warnings
 *
 *   Generic events use all of the normal mhash_t functions.
 *
 * 3.0 Usage
 *
 *   Report using the Wiki specification.
 *   GEVENT[<type>]=<Message>
 *
 *   See the online docs about how to set up gevent actions and such.
 */




/*
 * JOB EPILOGS
 *
 * job epilogs are tricky in the case of voltaire, we need to make sure that they run whenever the prolog ran
 * this means that if the prolog runs successfully but the job fails to start then we need to run the epilog.
 * there are also problems where because the triggers for epilog are tucked away they are not routinely checked
 * and updated.  this means that epilog triggers can be in "active" state forever as nothing ever checks to see
 * if they are finished.


 */

/*
 * Tracking when a job is blocked by GRes
 *
 * Currently, the internal flag mjifBlockedByGRes is set on a job in MJobSelectMNL() when the req cannot find idle
 * resources in MNodeSelectIdleTasks() and MJobSelectMReqIsGlobalOnly() returns true.  This prints out a message in
 * in checkjob but eventually we'll need to add another line to the events file.  
 *
 * This does not take into account situations where a job can't find idle resources but is able to preempt and get
 * them anyway.  In other words the current implementation will return artificially high values if preemption is
 * configured and used frequently.
 */ 

#define ODBCDataStorageDoc

/**
 * Using an ODBC-MySQL database for storing statistics, events, jobs, and nodes
 *
 * NOTE: When adding a new object type to the database, you will need to
 * ensure that all functions, database relations, and sql statements exist
 * for that object type. For example: if adding reservations to the database,
 * a statement MDBInsertRsvStmtText would be required, parallel to
 * MDBInsertNodeStmtText.
 *
 * To configure and connect Moab to a MySQL-backed ODBC data store, see
 * section 22 of the Moab documentation
 * 
 * As of Moab 6.0, Jobs, Requests, and Nodes are stored in the database. Jobs
 * and nodes have a primary key which is the JobID or Node Name. Requests
 * have a compound primary key consisting of the JobID of its job combined
 * with the Request index.
 *
 * Jobs, Requests, and Nodes are stored in the database by way of transition
 * objects mtransjob_t, mtransreq_t, and mtransnode_t, which are updated
 * whenever a job or node is changed and written to the database by a
 * separate thread from the scheduler.
 *
 * To add a new Job/Request/Node attribute to the database you must do the
 * following:
 *
 *  1.  Add the new attribute to the Moab database schema in: 
 *
 *      contrib/sql/moab-db.sql
 *      
 *  2.  Add a variable to the appropriate struct in moab.h which will
 *      represent that particular attribute
 *
 *  3.  Add the attribute name to the appropriate insert and base select
 *      statements in MDBI.c (i.e. MDBInsertNodeStmtText) BE SURE TO ADD A
 *      QUESTION MARK to the list of question marks at the end of the insert
 *      statement. The number of attributes names MUST match the number of
 *      question marks, and vice versa.
 *  
 *  4.  Add an appropriate statement to the bind function (MDBBindJob,
 *      MDBBindRequest, MDBBindNode) based on the data type of the attribute
 *      you're adding. (MDBBindColText, MDBBindColInteger, etc...)
 *
 *  5.  Add the following to the appropriate write function:
 *
 *      a.  Add the appropriate variable to the top of the write function
 *          (MDBWriteJobs, MDBWriteRequests, MDBWriteNodes). 
 *
 *      b.  Add the appropriate BindParam statement to the if (Initialized ==
 *          FALSE) block (MDBBindParamText, MDBBindParamInteger, etc...).
 *
 *      c.  In the for loop at the bottom, add the appropriate assignment
 *          from the job transition to the variable declared in the write
 *          function.
 *
 *  6.  If necessary, add the new variable/attribute to the appropriate
 *      transition alloc/free functions
 *
 *  7.  Add the appropriate logic to the transition function
 *      (MJobToTransitionStruct, MReqToTransitionStruct,
 *      MNodeToTransitionStruct).
 *  
 *  8.  If necessary, add the appropriate logic to the appropriate AToString
 *      function (MJobTransitionAToString, MReqTransitionAToString,
 *      MNodeTransitionAToString).
 *
 *  9.  If necessary, add any appropriate special handling to the appropriate
 *      TransitionToXML function.
 *
 *  10.  Make sure you can see your selection added by doing the following:
 *
 *      a.  start Moab
 *
 *      b.  verify in Moab that the new attribute shows on a node/job/req
 *
 *      c.  verify that Moab is properly transitioning the new atttribute by
 *          selecting it in a mysql shell:
 *          
 *          select <new_attribute_name> from <Jobs/Nodes/Requests>;
 *
 */




#define GPUDoc

/* Moab GPU support
 *
 * GPUs are supported internally as generic resources.  The generic resource name is MCONST_GPUS.
 *  This generic resource will automatically be added if Torque reports GPUs or if a job requests them.
 *  GPUs are currently only supported with Torque, although they could be added by adding the 
 *  generic resources manually with the same name as MCONST_GPUS, and then jobs could request them.
 *  However, this would only schedule them, and the RM would not be made aware of it.
 *
 * To request GPUs in a job, use "gpus=", similar to ppn.
 *
 *   msub -l nodes=4:gpus=2
 *
 * One important note: gpus are per task, which means that with jobs they are PER PROC.  That means 
 *  that if someone submits:
 *     msub -l nodes=2:ppn=2:gpus=2
 *  there will be 2 nodes * 2 ppn = 4 procs.  Since there are 2 gpus per task(proc), we are requesting
 *  8 gpus, not 4!
 *
 * GPUs are set in torque by putting gpus in the server nodes file (like np):
 *
 * myMachine np=4 gpus=4
 *
 * Torque will assign the specific gpus to the job.  Moab tells Torque how many gpus to assign.
 */



#define JobArraysDoc
/**
  @design doc Job Arrays

  1.0 Overview
  1.1 Feature/Concept Overview (what it does, why it matters)

    Users must have the ability to submit "array" jobs to Moab via msub. 
    Array jobs are an easy way to submit many sub-jobs that perform the same 
    work using the same script, but operate on different data sets. 
    Sub-jobs are simply the jobs created by an array job and are identified 
    by the array job ID and an "index" (e.g., "235-1" where the "-1" 
    represents the sub-job index).

    Like a normal job, an array job submits a job script, but it additionally 
    has start index (sidx), end index (eidx), and increment (incr) values, 
    which Moab uses to create "(eidx - sidx + incr) / incr"¹ sub-jobs, all
    executing the same script. A number of environment variables should be
    supplied for the script to understand what index in the array it is:

    * JOB_ARRAY_INDEX - the index of the job
    * JOB_ARRAY_RANGE - the range in format of <MIN>-<MAX>
  
    To illustrate, suppose an array job has a start index of 1, an end index 
    of 100, and an increment of 1. This is an array job that creates 
    (100 - 1 + 1) / 1 = 100 sub-jobs with indexes of 1, 2, 3, ..., 100. An
    increment of 2 would produce (100 - 1 + 2) / 2 = 50 sub-jobs with indexes 
    of 1, 3, 5, ..., 99. An increment of 2 with a start index of 2 would 
    produce (100 - 2 + 2) / 2 = 50 sub-jobs with indexes of 2, 4, 6, ..., 100.

    Sub-jobs are jobs in their own right that have a slightly different job
    naming convention (jobID-subJobIndex). 
    
    Users can control individual sub-jobs in the same manner as normal jobs. 
    In addition, an array job represents its group of sub-jobs and any user 
    or administrator commands performed on an array job apply to its sub-jobs
    (e.g., canceljob <arrayJobId> cancels all sub-jobs that belong to the 
    array job).

    If a user submits an array job to a grid head node, Moab must schedule 
    the array job's sub-jobs to a single cluster; i.e., its sub-jobs are 
    not permitted to execute across multiple clusters.

    These jobs will run the same job script and will be running
    in an identical environment.


  1.2 Brief Architecture Overview (how it is implemented)

    Jobs are started through the following steps:

  - Process the request for the master job
  - Identify a partition which can accomodate all jobs in the array
  - Clone the number requested of sub-jobs
  - Duplicate Master job parameters to sub-jobs
  - Determine eligibility according to all policies and limits
  - Migrate jobs to correct resource manager and partition
  - Start eligible sub-jobs

  2.0 Architecture
  2.1 Functional Flow (Review of key functions)

  -  MSysProcessRequest()
  -  MUISProcessRequest() 
  -  MUIJobCtl()
  -  MUIJobSubmit()
  -  MRMJobSubmit()
  -  MUThread()
  -  __MUTFunc()
  -  MS3JobSubmit()
  -  MRMJobPostLoad()
  -  MJobLoadArray()
  -  MUIJobSubmit()
  -  MJobDuplicate()
  -  MSchedProcessJobs()
  -  __MSchedScheduleJobs()
  -  MQueueScheduleIJobs()
  -  MJobStart()
  -  MJobMigrate()


  2.2 Data Structure Overview
    The main data structure is the mjarray_t which is attached to the
    mjob_t structure:

    typedef struct mjarray_t {

          char Name[MMAX_NAME];

          int Count; 
          int Size; 

          int Active;
          int Idle;
          int Complete;

          int  Limit;
          long PLimit;

          int  *Members;

          int  *JIndex;
          char **JName;
          enum MJobStateEnum  *JState;

          int     ArrayIndex;

          int     JobIndex;

          mulong  Flags;
     } mjarray_t;


  2.3 Functional List (list of all feature functions)
  2.4 Limitations/Warnings

    No job array usage limits will be supplied at this time. 
    We are not integrating natively with Torque's support for job arrays.

   3.0 Usage
     3.1 Required Parameters

     FORMAT:  msub -t <name>[<indexlist]%<limit>

     3.2 End-User Usage

     Following is the structure of the command:

     FORMAT:  msub -t <name>[<indexlist]%<limit>

     3.3 Reporting
       When reporting a job that is part of an array, the following 
       attributes should be provided:

         * parent array ID
         * array index
         * array range

     3.4 Dynamic Modification
     3.5 Checkpointing
     3.6 Examples

      myarray1.[1-1000]%5  -- only 5 at a time can run
      myarray1.[1,2,3,4]
      myarray1.[1-5,7,10]
      myarray1.[1-10:2] -- job steps are every two ie. 
                            myarray1.2, myarray1.4, myarray1.6

      You can operate on all jobs in the array by using the array name: 

      canceljob myarray1
      releasehold -a myarray1
      checkjob -v myarray1

      you can operate on a specific job in the job array: 

      canceljob myarray1.1
      releasehold -a myarray1.2
      checkjob -v myarray1.3

      You can also operate on ranges of jobs:

      mjobctl -c r:myarray1.[10-99]
      mjobctl -h r:myarray1.[25-53]
      mjobctl -u r:myarray1.[475-500]


   4.0 Next Steps
   5.0 Related Objects
     5.1 Design Docs
     5.2 Key Developers
     5.3 Interested Parties
     5.4 User Documentation
     5.5 Other
   6.0 Test Plan
   7.0 ChangeLog (optional)

*/

#define VMUsageNumbersDoc

/* 
  1.0 Overview.

    Provide an array of doubles that contains the current usage based upon
    cputime, jobproc, proc, memory, disk, swap.

  1.1 Calculations.

    Each usage value is calculated as follows:

      a) mrlCPUTime:  vm cpuload / configured procs
      b) mrlJobProc: available procs / nodes configured procs
      c) mrlProc: configured procs / nodes configured procs
      d) mrlJobMem: vm available memory / nodes configured mem
      e) mrlMem: vm configured memory / nodes configured mem
      f) mrlDisk: vm configured disk / nodes configured disk
      g) mrlSwap: vm configured swap / nodes configured swap.

  1.2 SUPPORT FUNCTIONS:

    1) MVMDoesUsageExceedThresholds:  Returns SUCCESS/FAILURE based upon
       thresholds set by configuration by NODECFG[] VMOCTHRESHOLD=...
    2) MVMGetTotalNodeVMUsagePercentage:  Returns array of usage values based upon the 
       sum of all VM's on a specific node.
    3) MVMGetVMUsagePercentage:  Gets vm usage values for a single VM on a specific node.

  1.2 RETURNS:
    Array of each calculated resource based upon type.

*/
#define MVMSchedulePolicyBasedMigrationDoc

/*
  1.0 Overview
    Evaluates resource utilization identifying hypervisors (nodes) thats
    overcommited and schedule the migration of VM's to other nodes.

  1.1 Feature/Concept Overview 

    1) Search each node and obtain the resource usage value for each VM.
    2) Sort the VM's based on the greatest used resources.
    3) Iterate through each VM, select a destination node, and schedule the 
       migration.
    4) Decrement usuage numbers.
*/

#define MVMSelectDestinationDoc

/*
  1.0 Overview
    Select the best destination node for a VM migration.

  1.1 Feature/Concept Overview
    1) Create a list of eligable nodes.
    2) Search list of nodes
      a) Verify the node accepts migrations.
      b) Check VM usage thresholds.
    3) Sort servers 

*/

#define MVMScheduleGreenConsolidationDoc
/*
  1.0 Overview
    Migrate VM's off nodes and consolidate to a single node so that nodes
    can be powered down.

  1.1 Feature/Concept Overview
    1) Generate node list sorted by least packed servers.
    2) Iterate nodes.
      a) Locate VM associated with node.
      b) Schedule migration.
    
*/



#define EnvRSDoc
/** 
  @designdoc Environment Record Separtor
 
 1.0 Overview
   1.1 Feature/Concept Overview (what it does, why it matters)

   Because it's possible to have nested ','s and ';'s in an environment
   variable ~rs; and \036 are used as the delimter. 
   
   1.2 Brief Architecture Overview (how it is implemented)

   Before migration:

   When msub sends the environment to the server it converts \036 to ~rs; and
   the server converts it back to \036. When the job is checkpointed, \036 is 
   replaced with ~rs; because \036 is a non printable character and will cause
   the checkpoint to fail. When reading back from checkpoint, ~rs; is converted
   back to \036. When the job is being migrated to an rm, MUSpawnChild
   tokenizes on \036 and sticks each environment variable into a an array that
   is passed in to execv.

   After migration:

   When the job is migrated to a rm, the environment is modified by calling
   MRMJobModify. When using torque the, it accepts a comma delimited list of 
   environment variables.

 2.0 Architecture
  
   In moab.h there are threee variables defined that should be used when 
   referencing the record separators.

  ENVRS_SYMBOLIC_STR     "~rs;"
  ENVRS_ENCODED_STR      "\036"
  ENVRS_ENCODED_CHAR     '\036'

 */




#define JobArraysDoc
/**
  @design doc Job Arrays

  1.0 Overview
  1.1 Feature/Concept Overview (what it does, why it matters)

    Users must have the ability to submit "array" jobs to Moab via msub. 
    Array jobs are an easy way to submit many sub-jobs that perform the same 
    work using the same script, but operate on different data sets. 
    Sub-jobs are simply the jobs created by an array job and are identified 
    by the array job ID and an "index" (e.g., "235-1" where the "-1" 
    represents the sub-job index).

    Like a normal job, an array job submits a job script, but it additionally 
    has start index (sidx), end index (eidx), and increment (incr) values, 
    which Moab uses to create "(eidx - sidx + incr) / incr"¹ sub-jobs, all
    executing the same script. A number of environment variables should be
    supplied for the script to understand what index in the array it is:

    * JOB_ARRAY_INDEX - the index of the job
    * JOB_ARRAY_RANGE - the range in format of <MIN>-<MAX>
  
    To illustrate, suppose an array job has a start index of 1, an end index 
    of 100, and an increment of 1. This is an array job that creates 
    (100 - 1 + 1) / 1 = 100 sub-jobs with indexes of 1, 2, 3, ..., 100. An
    increment of 2 would produce (100 - 1 + 2) / 2 = 50 sub-jobs with indexes 
    of 1, 3, 5, ..., 99. An increment of 2 with a start index of 2 would 
    produce (100 - 2 + 2) / 2 = 50 sub-jobs with indexes of 2, 4, 6, ..., 100.

    Sub-jobs are jobs in their own right that have a slightly different job
    naming convention (jobID-subJobIndex). 
    
    Users can control individual sub-jobs in the same manner as normal jobs. 
    In addition, an array job represents its group of sub-jobs and any user 
    or administrator commands performed on an array job apply to its sub-jobs
    (e.g., canceljob <arrayJobId> cancels all sub-jobs that belong to the 
    array job).

    If a user submits an array job to a grid head node, Moab must schedule 
    the array job's sub-jobs to a single cluster; i.e., its sub-jobs are 
    not permitted to execute across multiple clusters.

    These jobs will run the same job script and will be running
    in an identical environment.


  1.2 Brief Architecture Overview (how it is implemented)

    Jobs are started through the following steps:

  - Process the request for the master job
  - Identify a partition which can accomodate all jobs in the array
  - Clone the number requested of sub-jobs
  - Duplicate Master job parameters to sub-jobs
  - Determine eligibility according to all policies and limits
  - Migrate jobs to correct resource manager and partition
  - Start eligible sub-jobs

  2.0 Architecture
  2.1 Functional Flow (Review of key functions)

  -  MSysProcessRequest()
  -  MUISProcessRequest() 
  -  MUIJobCtl()
  -  MUIJobSubmit()
  -  MRMJobSubmit()
  -  MUThread()
  -  __MUTFunc()
  -  MS3JobSubmit()
  -  MRMJobPostLoad()
  -  MJobLoadArray()
  -  MUIJobSubmit()
  -  MJobDuplicate()
  -  MSchedProcessJobs()
  -  __MSchedScheduleJobs()
  -  MQueueScheduleIJobs()
  -  MJobStart()
  -  MJobMigrate()


  2.2 Data Structure Overview
    The main data structure is the mjarray_t which is attached to the
    mjob_t structure:

    typedef struct mjarray_t {

          char Name[MMAX_NAME];

          int Count; 
          int Size; 

          int Active;
          int Idle;
          int Complete;

          int  Limit;
          long PLimit;

          int  *Members;

          int  *JIndex;
          char **JName;
          enum MJobStateEnum  *JState;

          int     ArrayIndex;

          int     JobIndex;

          mulong  Flags;
     } mjarray_t;


  2.3 Functional List (list of all feature functions)
  2.4 Limitations/Warnings

    No job array usage limits will be supplied at this time. 
    We are not integrating natively with Torque's support for job arrays.

   3.0 Usage
     3.1 Required Parameters

     FORMAT:  msub -t <name>[<indexlist]%<limit>

     3.2 End-User Usage

     Following is the structure of the command:

     FORMAT:  msub -t <name>[<indexlist]%<limit>

     3.3 Reporting
       When reporting a job that is part of an array, the following 
       attributes should be provided:

         * parent array ID
         * array index
         * array range

     3.4 Dynamic Modification
     3.5 Checkpointing
     3.6 Examples

      myarray1.[1-1000]%5  -- only 5 at a time can run
      myarray1.[1,2,3,4]
      myarray1.[1-5,7,10]
      myarray1.[1-10:2] -- job steps are every two ie. 
                            myarray1.2, myarray1.4, myarray1.6

      You can operate on all jobs in the array by using the array name: 

      canceljob myarray1
      releasehold -a myarray1
      checkjob -v myarray1

      you can operate on a specific job in the job array: 

      canceljob myarray1.1
      releasehold -a myarray1.2
      checkjob -v myarray1.3

      You can also operate on ranges of jobs:

      mjobctl -c r:myarray1.[10-99]
      mjobctl -h r:myarray1.[25-53]
      mjobctl -u r:myarray1.[475-500]


   4.0 Next Steps
   5.0 Related Objects
     5.1 Design Docs
     5.2 Key Developers
     5.3 Interested Parties
     5.4 User Documentation
     5.5 Other
   6.0 Test Plan
   7.0 ChangeLog (optional)

*/





#define GenericSyJob

/*
 * @designdoc Generic System Jobs implementation
 *
 * 1.0 Overview
 *
 * Generic system jobs are system jobs with a trigger.
 *
 * 2.0 Architecture
 *
 * Generic system jobs must have one and only one trigger.  This trigger must
 *  have AType=exec, EType=start, an Action, and a Timeout.  The job will
 *  receive the trigger's timeout as its wallclock time.  When a job is marked
 *  as a generic system job, it also will have the NoRMStart flag set.
 *
 * Generic system jobs are set up as job templates.  They may be selectable
 *  (which will convert the submitted job) or part of a template dependency
 *  workflow.
 *
 * There is also a new job template boolean, INHERITRES.  When true, if the
 *  job is part of a dependency (i.e., another job declares a dependency on
 *  it), the job becomes a clone of the other job (to get the same resource
 *  request) and then has the template applied.  When the first job finishes, 
 *  it will pass its resources to the other job.  In the case of before 
 *  dependencies, the job that ran first still passes its resources up, but
 *  still the job created by the dependency will be a copy of the job that
 *  declared it.
 *
 * The InheritRes dependencies are kept in the J->ImmediateDepend linked list.
 *  This keeps the name of the following job and the type (if applicable).  If
 *  no type is specified, afterany is assumed.  This is kept on the first job
 *  to run (each job keeps track of which jobs to launch when it completes).
 *  Because of how these may have been specified, the dependency may be listed
 *  as a "before" instead of "after".  However, they are all treated as after
 *  dependencies because they are always kept on the job that will run first.
 *
 * 2.1 Key Functions
 *
 * MJobTemplateIsValidGenSysJob()
 *   Checks that a given job template meets all of the requirements for a 
 *   generic system job.
 *
 * MJobApplySetTemplate()
 *   Applies the trigger, sets job as a system job, etc.
 *
 * 3.0 Usage
 *
 * To set up a generic system job, it needs to have the trigger set up (as 
 *  mentioned above), and have the GENERICSYSJOB attribute set.
 *
 * JOBCFG[gen]   GENERICSYSJOB=TRUE
 * JOBCFG[gen]   TRIGGER=AType=exec,EType=start,Action="myrun.py",Timeout=10:00
 *
 * To set up a workflow, you need to use the TEMPLATEDEPEND parameter.
 *
 * JOBCFG[CreateVMWithSoftware]  TEMPLATEDEPEND=AFTEROK:InstallSoftware  SELECT=TRUE
 *
 * JOBCFG[InstallSoftware]  GENERICSYSJOB=TRUE
 * JOBCFG[InstallSoftware]  TRIGGER=EType=start,AType=exec,Action="$HOME/setupSoftware.py $IPAddr",Timeout=30:00
 * JOBCFG[InstallSoftware]  INHERITRES=TRUE
 * JOBCFG[InstallSoftware]  TEMPLATEDEPEND=AFTEROK:CreateVM
 *
 * JOBCFG[CreateVM]  GENERICSYSJOB=TRUE
 * JOBCFG[CreateVM]  INHERITRES=TRUE
 * JOBCFG[CreateVM]  TRIGGER=EType=start,AType=exec,Action="$HOME/installVM.py $HOSTLIST",Timeout=1:00:00,sets=^IPAddr
 */

#define NextToRun

/* Next to run jobs (NTR) are VIP jobs in moab that have a high priority and which temporarily disable backfill
 * (using MSched.UseBackFill) until they start so they are assured the next spot in line.  Once the NTR job is started, 
 * backfill is reenabled at the bottom of the iteration.  NTR is set on the QOS: 
 *  
 * QOSCFG[test] QFLAGS=NTR 
 *  
 * Then all jobs submitted with that QOS or considered VIP jobs for moab. 
 */ 


#define Simulation

/** Simulation provides the ability to create a virtual environment with both representative workload and resources.
  *
  * Key Parameters:
  *   SIMWORKLOADTRACEFILE
  *   SIMRESOURCETRACEFILE
  *   SIMSTOPITERATION
  *   SCHEDCFG[] MODE=SIMULATION
  *
  * Key Commands:
  *   schedctl -s  // stop/pause scheduling
  *   schedctl -S  // step/advance scheduling
  *   mdiag -S     // diagnose scheduling state
  *
  * Key Routines:
  *   MSimLoadWorkload()
  *   MSimGetResources()
  */

#define VMMigration

/**
 * @designdoc VM Migration
 *
 * VM Migration
 *
 * 1.0 Overview
 *
 *    1.1 Feature/Concept Overview
 *    VM migration is an important feature in cloud by allowing Moab move VMs around.  This makes it possible to 
 *    overcommit nodes but keep the load in check, consolidate machine for green purposes, and maintenance.
 *
 *    1.2 Brief Architecture Overview
 *    There is a common VM migration algorithm, contained mostly in __PlanTypedVMMigrations() and VMFindDestination().
 *    The main algorithm will call out to policy-specific functions, using the interface defined by
 *    mvm_migrate_policy_if_t.  All functions must be present, but are not necessarily required to do anything.  Usage
 *    calculations are the same for all policies.
 *
 * 2.0 Architecture
 *
 *    2.1 Functional Flow
 *    There are several key functions, separated by areas of functionality:
 *
 *      Usage/Load Calculations:
 *        *CalculateNodeAndVMLoads()    - Top-level function that returns hash tables with loads for VMs and hypervisors
 *        *__CalculateVMLoad()          - Calculates load for given VM
 *        *__CalculateNodeLoad()        - Calculates load for given hypervisor
 *        *AddVMMigrationUsage()        - Adds one usage to another (VM usage to hypervisor usage)
 *        *SubtractVMMigrationUsage()   - Subtract one usage from another (VM usage from hypervisor usage)
 *
 *      Main Migration:
 *        *PlanVMMigrations()           - Top level migration controller
 *        *__PlanTypedVMMigrations()    - Plan migrations for a specific type
 *        *CalculateMaxVMMigrations()   - Returns the number of migrations that can be planned (-1 is unlimited)
 *        *PerformVMMigrations()        - Submits the VM migration jobs for migrations that were planned by PlanVMMigrations()
 *        *MVMGetCurrentDestination()   - Returns the destination of a migrating VM (NULL if not migrating)
 *        *NodeAllowsVMMigration()      - Says if a hypervisor allows VMs to be migrated to/from it
 *        *VMIsEligibleForMigration()   - Says if a VM can be migrated
 *        *GetVMsMigratingToNode()      - Returns a list of VMs currently migrating to the given hypervisor
 *
 *      Find Destination for Migration:
 *        *VMFindDestination()          - Searches for a destination for the given VM
 *        *NodeIsOvercommitted()        - Returns TRUE if hypervisor is overcommitted in any way
 *        *VMFindDestinationSingleRun() - Wrapper function that simply finds a destination for a VM (uses overcommit policy)
 *
 *      Policies:
 *        Policy functions are stored in a separate file for each policy.
 *
 *
 *    The basic algorithm is as follows (policy pluggable functions are marked with a *):
 *
 *     *Order nodes (first node is first to be evaluated for migration)
 *      For N in nodes:
 *       *Evaluate if we're done with migration for whole system
 *       *NodeSetup (optional, will continue to next node if NodeSetup fails, can be a pre-check)
 *       *Order VMs (first VM is first to be evaluated)
 *        For VM in VMs:
 *          Check how many migrations are left to plan
 *         *Evaluate if done migrating for this node
 *          Find destination (VMFindDestination)
 *          If destination found:
 *            Queue migration
 *            Add VMusage to destination, subtract from source
 *       *NodeTeardown (optional)
 *
 *    In VMFindDestination, the basic flow is:
 *     *Order destinations
 *       Check destination feasibility (would be overcommitted, features, reservations, policy restrictions, etc.)
 *       If destination is valid, return destination
 *
 *
 *    2.2 Data Structure Overview
 *    Relevant structures are in include/MVMMigration.h.  The three main structures are:
 *
 *      *mvm_migrate_decision_t         - Stores a planned VM migration
 *      *mvm_migrate_usage_t            - Used to store allocation/load information for hypervisors and VMs
 *      *mvm_migrate_policy_if_t        - Defines the function interface for VM policies.
 *
 *    Each policy MUST implement all of the functions in mvm_migrate_policy_if_t, but they are not required to do anything
 *    more than just return SUCCESS (qsort routines need to return an int).  Each policy should be stored in its own file.
 *
 *    There are also two helper structures, mnode_migrate_t and mvm_migrate_t, which both hold a pointer to the node or VM
 *    that they represent, and the associated usage structure for that object.  These are passed into many of the routines.
 *    The usage structure should never be freed when freeing one of these helper functions.  The usage hash tables are the
 *    master source for the usage structures, and they will take care of freeing them.
 *
 *    2.3 Functional List
 *    This VM migration functionality is used for automatic migrations, admin or user-requested migrations, and the evacvms
 *    functionality.
 *
 *    2.4 Limitations/Warnings
 *    Migrations will honor reservations, but not reservation affinities (it is just an access / no access system).
 *
 * 3.0 Usage
 *
 *    3.1 Required Parameters
 *    There are a few things that must be set up for VM migration to work (besides having a VM system):
 *      *Hypervisors must have a declared hypervisor type
 *      *Hypervisors must have a non-zero proc load
 *      *VMs can only be moved to hypervisors of the same type as the hypervisor that it is currently on
 *      *If using workflow VM model, there must be a migrate template defined for the workflow
 *
 *    To turn on automatic migration, do "" in the moab.cfg
 *
 *    3.2 End-User Usage
 *    An admin can force a migration by running "mvmctl -f <type> [--flags=eval].
 *
 *    The --flags=eval option will have Moab do the normal VM migration evaluation, but not actually submit the 
 *    migration jobs.  Moab will return a list of planned migrations, in the order that they were planned.
 *
 *    Also, when using mvmctl -f, there will be no throttle on the number of migrations planned (it will override
 *    VMMIGRATETHROTTLE in moab.cfg).
 *
 *    3.3 Reporting
 *    Moab will log migration decisions and information, using the fMIGRATE logging subject.  Event logs are also
 *    created.
 *
 *    For policy-specific logging, place log statements in the individual policy files.
 *
 *    3.4 Dynamic Modification
 *    None currently supported.
 *
 *    3.5 Checkpointing
 *    Migration jobs are checkpointed normally.  There is no checkpointing needed for the actual migration routines.
 *
 *    3.6 Examples
 *    mvmctl -f overcommit --flags=eval
 *    mvmctl -f green
 *
 * 4.0 Next steps
 *    Some areas of functionality still left to implement:
 *      automatic migration
 *      rename policies
 *      refactor some common policy routines
 *      add event logs
 *
 *    Future functionality:
 *      allow mvmctl -f to pass in a list of nodes to do migration for
 *      allow mvmctl -f to do the evacvms action
 *
 * 5.0 Related Objects
 *
 *    5.1 Design Documentation
 *    See the wiki at https://wiki.adaptivecomputing.com:8443/display/MOAB/VM+Migration+Refactor+Proposal
 *
 *    5.2 Key Developers
 *    Jason Gardner
 *
 *    5.3 Interested Parties
 *    VM migration was originally put in for the Bank of America.  Other interested parties will be nearly any customer
 *    who is using our cloud offering.
 *
 *    5.4 User Documentation
 *    User facing documentation should be in our standard online documentation.
 *
 *    5.5 Other
 *    Main subject matter experts are Mike Bailey, Josh Butikofer, Doug Wightman, Jason Gardner, and David Litster.
 *
 * 6.0 Test Plan
 *   There are unit tests for all functions in the VM migration code.
 *   More heavy testing is needed for the system as a whole.
 *
 * 7.0 Changelog
 */

/* END moabdocs.h */
