/* HEADER */

/**
 * @file MOSInterface.h  
 */

#ifndef __MOSINTERFACE_H__
#define  __MOSINTERFACE_H__

#include "moab.h"

void *MOSrealloc(void *ptr, size_t size);
char *MOSstrdup(const char *s);

int MOSSyslogInit(msched_t *);
int MOSSyslog(int,const char *,...);
int MOSGetPID(void);
int MOSSetGID(int);
int MOSSetUID(int);
int MOSSetEUID(int);
int MOSGetUID(void);
int MOSGetGID(void);
int MOSGetEUID(void);
int MOSGetHostName(char *,char *,unsigned long *,mbool_t);
int MOMkDir(char *,int);
int MOChangeDir(const char *,muenv_t *,mbool_t); 

int MUGetThreadID();
mbool_t MUFileExists(char const *);
const char *MUGetHomeDir(void);

int MUMutexInit(mmutex_t *);
int MUMutexLock(mmutex_t *);
int MUMutexUnlock(mmutex_t *);
int MUMutexLockSilent(mmutex_t *);
int MUMutexUnlockSilent(mmutex_t *);

#endif  /*   __MOSINTERFACE_H__ */
/* END MOSInterface.h */
