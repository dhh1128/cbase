/* HEADER */

#ifndef __MSYSOBJECTQUEUE_H__
#define __MSYSOBJECTQUEUE_H__


int MObjectQueueInit(void);
int MObjectQueueMutexInit(void);
int MWSQueueInit(void);
int MWSQueueMutexInit();

int MEventLogWSQueueInit();
int MEventLogWSQueueSize();
int MEventLogWSMutexInit();

int MOExportTransition(enum MXMLOTypeEnum,void *);
int MOTransitionToQueue(mdllist_t *,mmutex_t *,enum MXMLOTypeEnum,void *);
int MOTransitionFromQueue(mdllist_t *,mmutex_t *,enum MXMLOTypeEnum *,void **);
int MOExportToWebServices(enum MXMLOTypeEnum,void *,enum MStatObjectEnum);
int MORecordEventToWebServices(mstring_t *);

#endif /* __MSYSOBJECTQUEUE_H__ */
