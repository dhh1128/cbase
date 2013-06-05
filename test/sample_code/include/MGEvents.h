/* HEADER */

#ifndef __MGEVENTS_H__
#define __MGEVENTS_H__

/**
 * @file MGEvents.h
 *
 * Contains Function declarations dealing with GEVENTS
 */

/* Init the MGEvent Iterator instance */

extern int MGEventIterInit(mgevent_iter_t *);

/* GEvent Description functions */

extern int MGEventFreeDesc(mbool_t,int (*)(void **));
extern int MGEventAddDesc(const char *);

extern int MGEventGetDescInfo(const char *,mgevent_desc_t **);
extern int MGEventGetDescCount(void);

extern int MGEventDescIterate(char **,mgevent_desc_t **,mgevent_iter_t *);

/* GEvent Instance/Item functions */

extern int MGEventItemIterate(mgevent_list_t *,char **,mgevent_obj_t **,mgevent_iter_t *);

extern int MGEventGetItemCount(mgevent_list_t *);
extern int MGEventAddItem(const char *,mgevent_list_t *);
extern int MGEventRemoveItem(mgevent_list_t *,char *);

extern int MGEventGetItem(const char *,mgevent_list_t *,mgevent_obj_t **);
extern int MGEventGetOrAddItem(const char *,mgevent_list_t *,mgevent_obj_t **);

extern int MGEventFreeItemList(mgevent_list_t *,mbool_t,int (*)(void **));

/* Misc GEvent APIs */

extern int MGEventListToXML(mxml_t *,mgevent_list_t *);
extern int MGEventItemToXML(mxml_t *,char *,mgevent_obj_t *);

extern int MGEventGetSeverity(mgevent_obj_t *);

extern int MGEventGetListFromObject(enum MXMLOTypeEnum,const char *,mgevent_list_t **);

/* System processing Operations APIs */

extern int MGEventClearOldGEvents(void);

extern int MGEventProcessGEvent(int,enum MXMLOTypeEnum,char *,char *,double,enum MRecordEventTypeEnum ,const char *);

#endif /*  __MGEVENTS_H__ */
