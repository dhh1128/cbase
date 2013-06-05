/* HEADER */

#ifndef __MSYSMSQUEUE_H__
#define __MSYSMSQUEUE_H__

int MSysSocketQueueCreate(void);
int MSysSocketQueueFree(void);
int MSysSocketQueueMutexInit(void);

int MSysAddSocketToQueue(msocket_t *);
int MSysDequeueSocket(msocket_t **);

#endif  /* __MSYSMSQUEUE_H__ */
