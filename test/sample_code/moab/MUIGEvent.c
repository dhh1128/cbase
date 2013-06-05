/* HEADER */

/**
 * @file MUIGEvent.c
 *
 * Takes care of command line requests for GEvents.
 */

#include "moab.h"
#include "moab-proto.h"
#include "mcom-proto.h"
#include "moab-const.h"
#include "moab-global.h"


/**
 * Creates/updates a gevent from command line
 *
 * @param U
 * @param IsAdmin
 * @param RE
 * @param EMsg
 */

int MUIGEventCreate(
  mgcred_t           *U,       /* I */
  mbool_t             IsAdmin, /* I */
  mxml_t             *RE,      /* I */
  char               *EMsg)    /* O */
  {
  char tmpName[MMAX_NAME];
  char tmpVal[MMAX_LINE];

  char *GEventName = NULL;
  char *GEventMessage = NULL;
  mgevent_obj_t *GEvent = NULL;

  /*
  mgevent_desc_t *GEDesc = NULL;
  */

  int CTok = -1;

  enum MXMLOTypeEnum ParentType = mxoSched;

  if ((U == NULL) ||
      (RE == NULL) ||
      (EMsg == NULL))
    return(FAILURE);

  while (MS3GetSet(
      RE,
      NULL,
      &CTok,
      tmpName,
      sizeof(tmpName),
      tmpVal,
      sizeof(tmpVal)) == SUCCESS)
    {
    if (!strcmp(tmpName,"message"))
      {
      MUStrDup(&GEventMessage,tmpVal);
      }
    else if (!strcmp(tmpName,"name"))
      {
      MUStrDup(&GEventName,tmpVal);
      }
    }  /* END while (MS3GetWhere()) */

  if (GEventName == NULL)
    {
    sprintf(EMsg,"ERROR:  No gevent name was specified");

    MUFree(&GEventMessage);
    MUFree(&GEventName);

    return(FAILURE);
    }

  if (MGEventAddDesc(GEventName) == FAILURE)
    {
    sprintf(EMsg,"ERROR:  Could not set GEvent type %s",
      GEventName);

    MUFree(&GEventMessage);
    MUFree(&GEventName);

    return(FAILURE);
    }

  if ((MGEventGetOrAddItem(GEventName,&MSched.GEventsList,&GEvent) != SUCCESS) ||
      (GEvent == NULL))
    {
    sprintf(EMsg,"ERROR:  Could not set GEvent");

    MUFree(&GEventMessage);
    MUFree(&GEventName);

    return(FAILURE);
    }

  if (GEventMessage != NULL)
    MUStrDup(&GEvent->GEventMsg,GEventMessage);

  GEvent->GEventMTime = MSched.Time;

  if (MGEventProcessGEvent(
        -1,
        ParentType,
        NULL,
        GEventName,
        0.0,
        mrelGEvent,
        GEventMessage) == SUCCESS)
    {
    sprintf(EMsg,"INFO:  gevent %s successfully reported",
      GEventName);
    }
  else
    {
    sprintf(EMsg,"ERROR:  failed to report gevent %s",
      GEventName);

    return(FAILURE);
    }

  MUFree(&GEventMessage);
  MUFree(&GEventName);

  return(SUCCESS);
  }  /* END MUIGEventCreate() */
