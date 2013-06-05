/* HEADER */

/**
 * @file MGEventsXML.c
 * 
 * Contains code for generating XML from a MGEvent list
 */

/* Contains:                                 *
 *  int MGEventListToXML                     *
 *                                           */


#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 * Generate XML for one GEVENT item
 *
 * @param OE       (I) [modified]
 * @param GEName   (I)
 * @param GEvent   (I)
 */

int MGEventItemToXML(

  mxml_t         *OE,
  char           *GEName,
  mgevent_obj_t  *GEvent)

  {
  int Sev;

  mxml_t *GEE;

  if ((NULL == OE) || (NULL == GEvent))
    {
    return(FAILURE);
    }

  GEE = NULL;

  MXMLCreateE(&GEE,"gevent");

  MXMLSetAttr(GEE,"name",(void *)GEName,mdfString);
  MXMLSetAttr(GEE,"time",(void *)&GEvent->GEventMTime,mdfLong);

  MXMLSetAttr(GEE,"statuscode",(void *)&GEvent->StatusCode,mdfInt);

  Sev = MGEventGetSeverity(GEvent);

  if (Sev != 0)
    {
    MXMLSetAttr(GEE,"severity",(void *)&Sev,mdfInt);
    }

  /* NOTE: MXMLToString() automatically removes illegal characters from value */

  MXMLSetVal(GEE,(void *)GEvent->GEventMsg,mdfString);

  MXMLAddE(OE,GEE);

  return(SUCCESS);
  } /* MGEventItemToXML() */


/**
 * Report generic events as XML
 
 * @param OE          (I) [modified]
 * @param EventsList  (I)
 */

int MGEventListToXML(

  mxml_t          *OE,     
  mgevent_list_t  *EventsList) 

  {
  char             *GEName;
  mgevent_obj_t    *GEvent;
  mgevent_iter_t    GEIter;

  if ((OE == NULL) || (EventsList == NULL))
    {
    return(FAILURE);
    }

  MGEventIterInit(&GEIter);

  while (MGEventItemIterate(EventsList,&GEName,&GEvent,&GEIter) == SUCCESS)
    {
    MGEventItemToXML(OE,GEName,GEvent);
    } /* END while (MUHTIterate(EventsList,...) == SUCCESS) */

  return(SUCCESS);
  }  /* END MGEventListToXML() */


