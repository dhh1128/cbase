/* HEADER */

      
/**
 * @file MTrigXML.c
 *
 * Contains: Trigger XML functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 *
 *
 * @param T (I)
 * @param E (O)
 * @param SAList (I) [optional]
 */

int MTrigToXML(

  mtrig_t *T,      /* I */
  mxml_t  *E,      /* O */
  enum MTrigAttrEnum *SAList) /* I (optional) */

  {
  const enum MTrigAttrEnum DAList[] = {
    mtaActionData,
    mtaActionType,
    mtaBlockTime,
    mtaDescription,
    mtaDisabled,
    mtaEBuf,
    mtaEnv,
    mtaEventTime,
    mtaEventType,
    mtaFailOffset,
    mtaFailureDetected,
    mtaFlags,
    mtaIBuf,
    mtaIsComplete,
    mtaIsInterval,
    mtaLaunchTime,
    mtaMessage,
    mtaMultiFire,
    mtaName,
    mtaObjectID,
    mtaObjectType,
    mtaOBuf,
    mtaOffset,
    mtaPeriod,
    mtaPID,
    mtaRearmTime,
    mtaRequires,
    mtaSets,
    mtaState,
    mtaThreshold,
    mtaTimeout,
    mtaTrigID,
    mtaUnsets,
    mtaMaxRetry,
    mtaRetryCount,
    mtaNONE };

  int  aindex;

  enum MTrigAttrEnum *AList;

  char tmpString[MMAX_BUFFER];

  if ((T == NULL) || (E == NULL))
    {
    return(FAILURE);
    }

  if (SAList != NULL)
    AList = SAList;
  else
    AList = (enum MTrigAttrEnum *)DAList;

  for (aindex = 0;AList[aindex] != mtaNONE;aindex++)
    {
    if ((MTrigAToString(
          T,
          AList[aindex],
          tmpString,
          sizeof(tmpString),
          mfmHuman) == FAILURE) || (tmpString[0] == '\0'))
      {
      continue;
      }

    MXMLSetAttr(E,(char *)MTrigAttr[AList[aindex]],tmpString,mdfString);
    }  /* END for (aindex) */

  return(SUCCESS);
  }  /* END MTrigToXML() */




/**
 *
 *
 * @param T (I) [modified]
 * @param E (I)
 */

int MTrigFromXML(

  mtrig_t *T,  /* I (modified) */
  mxml_t  *E)  /* I */

  {
  int aindex;

  enum MTrigAttrEnum taindex;

  if ((T == NULL) || (E == NULL))
    {
    return(FAILURE);
    }

  /* NOTE:  do not initialize.  may be overlaying data */

  for (aindex = 0;aindex < E->ACount;aindex++)
    {
    taindex = (enum MTrigAttrEnum)MUGetIndex(E->AName[aindex],MTrigAttr,FALSE,mtaNONE);

    if (taindex == mtaNONE)
      continue;

    MTrigSetAttr(T,taindex,(void **)E->AVal[aindex],mdfString,mSet);
    }  /* END for (aindex) */

  MOGetObject(T->OType,T->OID,&T->O,mVerify);

  return(SUCCESS);
  }  /* END MTrigFromXML() */
/* END MTrigXML.c */
