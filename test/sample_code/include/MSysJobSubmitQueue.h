/* HEADER */

#ifndef __MSYSMJOBSUBMITQUEUE_H__
#define __MSYSMJOBSUBMITQUEUE_H__

int MSysJobSubmitQueueCreate(void);
int MSysJobSubmitQueueFree(void);
int MSysJobSubmitQueueMutexInit(void);

int MSysAddJobSubmitToQueue(mjob_submit_t *,int *,mbool_t);
int MSysDequeueJobSubmit(mjob_submit_t **);

#endif  /* __MSYSMJOBSUBMITQUEUE_H__ */
