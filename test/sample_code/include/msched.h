/*
*/

/**
 * @file msched.h
 *
 * Moab Scheduler Header
 */
        
#ifndef __M_H__
#define __M_H__

#ifdef __LOCAL
#  include "msched-local.h"
#endif /* __LOCAL */

#ifdef MFORCERMTHREADS
#  include <pthread.h>
#endif  /* MFORCERMTHREADS */

#define DEFAULT_LLBINPATH "/usr/lpp/LoadL/full/bin"

#if defined(__MLL) || defined(__MLL2) || defined(__MLL31) || defined(__MLL35)
# define __LLAPI
# include "llapi.h"
#endif /* __MLL || __MLL2  || __MLL31 */

#if defined(__MLL2) || defined(__MLL31) || defined(__O2K)
#ifndef __SMP
#define __SMP
#endif /* __SMP */
#endif /* __MLL2 || __MLL31 || __O2K */

/* temporary fix to handle LL API bug */

#ifndef LL_NOTQUEUED
# define LL_NOTQUEUED                     10
#endif /* LL_NOTQUEUED */

/* server specific config variables */

#define MSCHED_NAME                              "Moab Workload Manager"
#define MSCHED_SNAME                             "moab"
#define MSCHED_SCNAME                            "Moab"
#define MSCHED_MAILNAME                          "moabadmin"
#define MSCHED_WSFAILOVER                        "wsfailoverdata"
 
#define MSCHED_ENVDBGVAR                         "MOABDEBUG"
#define MSCHED_ENVRECOVERYVAR                    "MOABRECOVERYACTION"
#define MSCHED_ENVHOMEVAR                        "MOABHOMEDIR"
#define MSCHED_ENVLOGSTDERRVAR                   "MOABLOGSTDERR"
#define MSCHED_ENVPARVAR                         "MOABPARTITION"
#define MSCHED_ENVSMPVAR                         "MOABSMP"
#define MSCHED_ENVTESTVAR                        "MOABTEST"
#define MSCHED_ENVCKTESTVAR                      "MOABCKTEST"
#define MSCHED_ENVSIMTESTVAR                     "MOABSIMTEST"
#define MSCHED_ENVDSTESTVAR                      "MOABDSTEST"
#define MSCHED_ENVJSTESTVAR                      "MOABJSTEST"
#define MSCHED_ENVIDTESTVAR                      "MOABIDTEST"
#define MSCHED_ENVISTESTVAR                      "MOABISTEST"

#define MSCHED_ENVDISABLETRIGGER                 "MOABDISABLETRIGGER"
#define MSCHED_ENVDISABLECKPT                    "MOABDISABLECK"

#define MSCHED_MSGINFOHEADER                     "MOAB_INFO"
#define MSCHED_MSGERRHEADER                      "MOAB_ERROR"
      
#define DEFAULT_MRMNETINTERFACE                  mnetEN0

#define DEFAULT_DOMAIN                           ""

/* default stats values */

#ifndef EMPTY
#define EMPTY                0 
#endif  /* EMPTY */

/* list of job mode values */

#define MAX_VAL      2140000000

#define MAX_REQ_TYPE         16

#define MAX_ACCT_ACC          8 

#define MAX_PARMVALS        256
#define MAX_MWINDOW          16   /* maximum BF windows evaluated */
#define MAX_MRCLASS          16   /* maximum resource classes */

#define MDEF_MINSWAP         10   /* in MB */
#define MMIN_OSSWAPOVERHEAD  25   /* in MB */

/* FIXME: these look reversed, but switching them breaks the slurm interface */
#define MIN_OS_CPU_OVERHEAD    0.5
#define MAX_OS_CPU_OVERHEAD    0.0

#define ATTRMARKER             "ATTR"
#define PARTITIONMARKER        "PM_"
#define QOSDEFAULTMARKER       "QDEF="
#define QOSLISTMARKER          "QLIST="
#define PARTDEFAULTMARKER      "PDEF="
#define PARTLISTMARKER         "PLIST="
#define MASTERJOBMARKER        "MASTERJOB="
#define FSFLAGMARKER           "JOBFLAGS="
#define FSTARGETMARKER         "FSTARGET="
#define PRIORITYMARKER         "PRIORITY="


/* status code values */

enum { scFAILURE = 0, scSUCCESS };

/* reasons for policy rejection */
 
enum MPolicyRejectionEnum {
  mprNONE = 0,
  mprMaxJobPerUserPolicy,
  mprMaxProcPerUserPolicy,
  mprMaxNodePerUserPolicy,
  mprMaxPSPerUserPolicy,
  mprMaxJobQueuedPerUserPolicy,
  mprMaxPEPerUserPolicy,
  mprMaxJobPerGroupPolicy,
  mprMaxNodePerGroupPolicy,
  mprMaxPSPerGroupPolicy,
  mprMaxJobQueuedPerGroupPolicy,
  mprMaxJobPerAccountPolicy,
  mprMaxNodePerAccountPolicy,
  mprMaxPSPerAccountPolicy,
  mprMaxJobQueuedPerAccountPolicy,
  mprSystemMaxJobProc,
  mprSystemMaxJobTime,
  mprSystemMaxJobPS,
  mprUnknownUser,
  mprUnknownGroup,
  mprUnknownAccount };


/* node system attributes */

enum MGenericAttrEnum { 
  mattrNONE = 0,
  mattrJMD, 
  mattrBatch, 
  mattrInteractive,
  mattrLogin,
  mattrHPSS,
  mattrPIOFS,
  mattrSystem,
  mattrGateway,
  mattrEPrimary };

#endif /* __M_H__ */

