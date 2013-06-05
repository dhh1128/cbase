/* HEADER */

/**
 * @file mcom.h
 *
 * Moab Communications
 */

#ifndef __MCOM_H
#define __MCOM_H

/* system includes */

#include <arpa/inet.h>

/* OS defines */

#if defined(__AIX52) || defined(__AIX53) || defined(__AIX54)
# define __AIX 1
#endif /* __AIX */

/* core defines */

#ifndef NULL
#  define NULL (void *)0
#endif /* NULL */

#ifndef MIN
#  define MIN(x,y) (((x) < (y)) ? (x) : (y))
#endif /* MIN */

#ifndef MAX
#  define MAX(x,y) (((x) > (y)) ? (x) : (y))
#endif /* MAX */

#ifndef TRUE
#  define TRUE 1
#endif /* TRUE */

#ifndef FALSE
#  define FALSE 0
#endif /* FALSE */

#ifndef NONE
#  define NONE "[NONE]"
#endif /* NONE */

#ifndef ALL
#  define ALL "[ALL]"
#endif /* ALL */

#ifndef DEFAULT
#  define DEFAULT "[DEFAULT]"
#endif /* DEFAULT */


#include "mapi.h"
/* NOTE:  sync MMAX_LINE w/MCMAXLINE in mapi.h */

#ifndef MMAX_WIKI_BUFFER
#define MMAX_WIKI_BUFFER   MMAX_BUFFER << 4
#endif /* MMAX_BUFFER */

#ifndef MMAX_SCRIPT
#define MMAX_SCRIPT MMAX_BUFFER << 3
#endif

#define MMAX_BUFFER3  524288

#ifndef MCONST_CKEY
#define MCONST_CKEY "hello"
#endif /* MCONST_CKEY */

#ifndef __NOMCOMMTHREAD
#ifndef __MCOMMTHREAD
#define __MCOMMTHREAD
#endif /* !__MCOMMTHREAD */
#endif /* !__NOMCOMMTHREAD */

#if defined(__MCOMMTHREAD) || defined(MFORCERMTHREADS)
#include <pthread.h>
#define MTHREADSAFE  /* this variable MUST be defined if we are going to use threads! */

typedef pthread_mutex_t mmutex_t;
typedef pthread_rwlock_t mrwlock_t;
#else

typedef int mmutex_t;  /* if we don't have a thread lib just use "int" as mutex type */
typedef int mrwlock_t;  /* if we don't have a thread lib just use "int" as lock type */ 
#define PTHREAD_MUTEX_INITIALIZER (mmutex_t)0;
#endif /* __MCOMMTHREAD || MFORCERMTHREADS */




#define  CRYPTHEAD "KGV"

/* enumerations */

/* sync w/MDFormat */

enum MDataFormatEnum {
  mdfNONE = 0,
  mdfString,
  mdfInt,
  mdfLong,
  mdfDouble,
  mdfStringArray,
  mdfIntArray,
  mdfLongArray,
  mdfDoubleArray,
  mdfMulong,
  mdfOther,
  mdfLAST };

enum MSocketProtocolEnum {
  mspNONE = 0,
  mspSingleUseTCP,
  mspHalfSocket,
  mspHTTPClient,
  mspHTTP,
  mspS3Challenge };

enum MWireProtocolEnum {
  mwpNONE = 0,
  mwpAVP,
  mwpXML,
  mwpHTML,
  mwpS32 };


/* sync w/MS3Action[] */

enum MS3ActionEnum {
  msssaNONE,
  msssaCancel,
  msssaCreate,
  msssaDestroy,
  msssaInitialize,
  msssaList,
  msssaModify,
  msssaNotify,
  msssaQuery,
  msssaStart,
  msssaLAST };


/* socket states */

enum { mssNONE = 0, mssClosed, mssOpen, mssBusy, mssLAST };

/* sync w/MCSAlgoType[] */

enum MChecksumAlgoEnum {
  mcsaNONE = 0,
  mcsaDES,
  mcsaHMAC,
  mcsaHMAC64,
  mcsaMD5,
  mcsaMunge,  /* For generating munge signature (sending) */
  mcsaUnMunge,/* For processing munge signature (recieveing), only used as a parameter for MSecGetChecksum */
  mcsaPasswd,
  mcsaRemote,
  mcsaX509,   /* general OpenSSL based X.509 */
  mcsaLAST };


/* sync w/MS3CName[] */

enum MPeerServiceEnum {
  mpstNONE = 0,
  mpstNM,    /* system monitor     */
  mpstQM,    /* queue manager      */
  mpstSC,    /* scheduler          */
  mpstMS,    /* meta scheduler     */
  mpstPM,    /* process manager    */
  mpstAM,    /* accounting manager */
  mpstEM,    /* event manager      */
  mpstSD,    /* service directory  */
  mpstWWW }; /* web                */



/* const defines */

#define MMAX_SBUFFER        65536

#ifndef MMSG_BUFFER
  #define MMSG_BUFFER       (MMAX_SBUFFER << 5)
#endif /* MMSG_BUFFER */

#define MMAX_XBUFFER   (MMAX_SBUFFER << 10)
 
#define MCONST_S3XPATH     3
#define MCONST_S3URI       "SSSRMAP3"

#define MMAX_S3ATTR        256  /* NOTE: must be larger than mjaLAST, mrqaLAST, mnaLAST and mclaLAST */
#define MMAX_S3VERS        4
#define MMAX_S3JACTION     64

/* default defines */

#define MDEF_CSALGO         mcsaHMAC64
#define MDEF_SOCKETWAIT     5000000

#define MMAX_XMLATTR        64
#define MDEF_XMLICCOUNT     16

#define MDEF_SOCKETLINGERVAL 0  /* seconds */



/* failure codes */

/* sync w/MFC[] (there is no MFC!) */

enum MSFC {
  msfENone          = 0,     /* success */
  msfGWarning       = 100,   /* general warning */
  msfPending        = 110,   /* request was successful but has not yet completed */
  msfEGWireProtocol = 200,   /* general wireprotocol/network failure */
  msfEBind          = 218,   /* cannot bind socket */
  msfEGConnect      = 220,   /* general connection failure */
  msfCannotConnect  = 222,   /* cannot connect */
  msfCannotSend     = 224,   /* cannot send data */
  msfCannotRecv     = 226,   /* cannot receive data */
  msfConnRejected   = 230,   /* connection rejected */
  msfETimedOut      = 232,   /* connection timed out */
  msfEFraming       = 240,   /* general framing failure */
  msfEEOF           = 246,   /* unexpected end of file */
  msfEGMessage      = 300,   /* general message format error */
  msfENoObject      = 311,   /* no object specified in request */
  msfEGSecurity     = 400,   /* general security failure */
  msfESecClientSig  = 422,   /* security - signature creation failed at client */
  msfESecServerAuth = 424,   /* security - server auth failure */
  msfESecClientAuth = 442,   /* security - client auth failure */
  msfEGEvent        = 500,   /* general event failure */
  msfEGServer       = 700,   /* general server error */
  msfEGSNoSupport   = 710,   /* request not supported */
  msfEGSInternal    = 720,   /* internal failure on server */
  msfEBadRequestor  = 721,   /* requestor doesn't exist on remote system */
  msfEGSMapping     = 724,   /* cannot map request to local objects on server */
  msfEGSNoResource  = 730,   /* server has insufficient resources to complete request */
  msfEGServerBus    = 740,   /* general server business logic failure */
  msfEGNoFunds      = 782,   /* request has failed due to insufficient allocations */
  msfEGClient       = 800,   /* general client error */
  msfECInternal     = 820,   /* client internal error */
  msfECResUnavail   = 830,   /* client resource unavailable */
  msfECPolicy       = 840,   /* client policy failure */
  msfEGMisc         = 900,   /* general miscellaneous error */
  msfUnknownError   = 999 }; /* unknown failure */


/* sync w/ MS3CodeDecade[] (MConst-com.c) */

enum MS3CodeDecadeEnum {
  ms3cNone                      = 0,    /* Success */
  ms3cUsage                     = 1,    /* Help/Usage Reply */
  ms3cStatus                    = 2,    /* Status Reply */
  ms3cSubsriptionOK             = 3,    /* Subscription Reply */
  ms3cRegistrationOK            = 4,    /* Registration Reply */
  ms3cWarning                   = 10,   /* General Warning */
  ms3cProtocolWarning           = 11,   /* Wire Protocol Warning */
  ms3cFormatWarning             = 12,   /* Message Format Warning */
  ms3cSecurityWarning           = 13,   /* Security Warning */
  ms3cContentWarning            = 14,   /* Content or Action Warning */
  ms3cWireProtocol              = 20,   /* General Wire Protocol or Network Error */
  ms3cNetwork                   = 21,   /* Network Error */
  ms3cConnection                = 22,   /* Connection Failure */
  ms3cTimeOut                   = 23,   /* Connection Timed Out */
  ms3cFraming                   = 24,   /* Framing Failure */
  ms3cMessageFormat             = 30,   /* General Message Format Error */
  ms3cSyntaxRequest             = 31,   /* Syntax Error in Request */
  ms3cSyntaxResponse            = 32,   /* Syntax Error in Response */
  ms3cPipeline                  = 33,   /* Pipelining Failure */
  ms3cSecurity                  = 40,   /* General Security Error */
  ms3cNegotiation               = 41,   /* Negotiation Failure */
  ms3cAuthentication            = 42,   /* Authentication Failure */
  ms3cEncryption                = 43,   /* Encryption Failure */
  ms3cAuthorization             = 44,   /* Authorization Failure */
  ms3cEventManagement           = 50,   /* General Event Management Error */
  ms3cSubscription              = 51,   /* Subscription Failed */
  ms3cNotification              = 52,   /* Notification Failed */
  ms3cServerApplication         = 70,   /* General Server-side Application-specific Error */
  ms3cServerUnsupported         = 71,   /* Not Supported by Server */
  ms3cServerInternalError       = 72,   /* Internal Error at Server */
  ms3cServerResourceUnavailable = 73,   /* Resource Unavailable at Server */
  ms3cServerBusinessLogic       = 74,   /* Business Logic Error at Server */
  ms3cNoFunds                   = 78,   /* Insufficient Funds */
  ms3cClientApplication         = 80,   /* General Client-side Application-specific Error */
  ms3cClientUnsupported         = 81,   /* Not Supported by Client */
  ms3cClientInternalError       = 82,   /* Internal Error at Client */
  ms3cClientResourceUnavailable = 83,   /* Resource Unavailable at Client */
  ms3cClientBusinessLogic       = 84,   /* Business Logic Error at Client */
  ms3cMiscellaneous             = 90,   /* Miscellaneous Failure */
  ms3cUnknown                   = 99,   /* Unknown Failure */
  ms3cLAST                      = 100 };


/* sync w/MSockAttr[] */

enum MSocketAttrEnum {
  msockaNONE = 0,
  msockaLocalHost,
  msockaLocalPort,
  msockaRemoteHost,
  msockaRemotePort,
  msockaLAST };

#endif /* __MCOM_H */

/* END mcom.h */


