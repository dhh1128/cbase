/* HEADER */

      
/**
 * @file MObjectHistory.c
 *
 * Contains: Object History functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/** 
 * Record the current state of the object in its history structure.
 *
 */

int MHistoryAddEvent(

  void               *O,
  enum MXMLOTypeEnum  OType)

  {
  if ((O == NULL) || (OType == mxoNONE))
    {
    return(FAILURE);
    }

  switch (OType)
    {
    case mxoRsv:
  
      {
      marray_t RList;

      mhistory_t NewH;

      mrsv_t *R = (mrsv_t *)O;

      if (R->History == NULL)
        {
        R->History = (marray_t *)MUCalloc(1,sizeof(marray_t));

        MUArrayListCreate(R->History,sizeof(mhistory_t),10);
        }

      memset(&NewH,0,sizeof(mhistory_t));

      MCResInit(&NewH.Res);

      NewH.Time = MSched.Time;

      MRsvSumResources(R,&NewH.Res);

      MUArrayListCreate(&RList,sizeof(mrsv_t *),10);

      if ((!MSNLIsEmpty(&NewH.Res.GenericRes)) &&
          (R->RsvGroup != NULL) &&
          (MRsvGroupGetList(R->RsvGroup,&RList,NULL,0) == SUCCESS))
        {
        NewH.ElementCount = RList.NumItems - 1;
        }

      MUArrayListFree(&RList);

      if ((NewH.ElementCount == 0) && (R->Variables != NULL))
        {
        mln_t *tmpL = NULL;

        if (MULLCheck(R->Variables,"VM_COUNT",&tmpL) == SUCCESS)
          NewH.ElementCount = (int)strtol((char *)tmpL->Ptr,NULL,10);
        } 

      MUArrayListAppend(R->History,&NewH);
      }  /* END case mxoRsv */

      break;

    default:

      /* NYI */

      break;
    }  /* END switch (OType) */

  return(SUCCESS);
  }  /* MHistoryAddEvent() */
  

/**
 * Convert history of object to string.
 */

int MHistoryToString(

  void               *O,
  enum MXMLOTypeEnum  OType,
  mstring_t          *String) /* already initialized */

  {
  if ((O == NULL) || (OType == mxoNONE))
    {
    return(FAILURE);
    }

  switch (OType)
    {
    case mxoRsv:

      {
      char tmpLine[MMAX_LINE];

      int hindex;

      marray_t *History = ((mrsv_t *)O)->History;

      mhistory_t *tmpH;

      if (History == NULL)
        {
        return(SUCCESS);
        }

      for (hindex = 0;hindex < History->NumItems;hindex++)
        {
        tmpH = (mhistory_t *)MUArrayListGet(History,hindex);

        tmpLine[0] = '\0';

        MCResToString(&tmpH->Res,0,mfmAVP,tmpLine);

        MStringAppendF(String,"%ld:%s\n",tmpH->Time,tmpLine);
        }
      }  /* END case mxoRsv */

      break;

    default:

      /* NYI */

      break;
    }

  return(SUCCESS);
  }  /* MHistoryToString() */


int MHistoryFromXML(

  void               *O,
  enum MXMLOTypeEnum  OType,
  mxml_t             *HE)

  {
  switch (OType)
    {
    case mxoRsv:

      {
      char    tmpLine[MMAX_LINE];

      int     CTok = -1;

      mxml_t *CE = NULL;

      mhistory_t NewH;

      mrsv_t *R = (mrsv_t *)O;

      if (R->History == NULL)
        {
        R->History = (marray_t *)MUCalloc(1,sizeof(marray_t));

        MUArrayListCreate(R->History,sizeof(mhistory_t),10);
        }

      while (MXMLGetChild(HE,"event",&CTok,&CE) == SUCCESS)
        {
        memset(&NewH,0,sizeof(mhistory_t));

        MCResInit(&NewH.Res);

        if (MXMLGetAttr(CE,(char *)MHistoryAttr[mhaTime],NULL,tmpLine,sizeof(tmpLine)) == SUCCESS)
          {
          NewH.Time = strtol(tmpLine,NULL,10);
          }

        if (MXMLGetAttr(CE,(char *)MHistoryAttr[mhaState],NULL,tmpLine,sizeof(tmpLine)) == SUCCESS)
          {
          MCResFromString(&NewH.Res,tmpLine);
          }

        if (MXMLGetAttr(CE,(char *)MHistoryAttr[mhaElementCount],NULL,tmpLine,sizeof(tmpLine)) == SUCCESS)
          {
          NewH.ElementCount = (int)strtol(tmpLine,NULL,10);
          }

        MUArrayListAppend(R->History,&NewH);
        }  /* END MXMLGetChild() */
      }  /* END case mxoRsv */
      
      break;
 
    default:

      /* NYI */

      break;
    }   /* END switch (OType) */

  return(SUCCESS);
  }  /* END MHistoryFromXML() */





/**
 * Convert history of object to string.
 */

int MHistoryToXML(

  void               *O,
  enum MXMLOTypeEnum  OType,
  mxml_t            **EP) /* already initialized */

  {
  mxml_t *E;

  if ((O == NULL) || (OType == mxoNONE))
    {
    return(FAILURE);
    }

  if (*EP == NULL)
    {
    if (MXMLCreateE(EP,(char *)MRsvAttr[mraHistory]) == FAILURE)
      {
      return(FAILURE);
      }
    }

  E = *EP;

  switch (OType)
    {
    case mxoRsv:

      {
      mxml_t *CE;

      char tmpLine[MMAX_LINE];

      int hindex;

      marray_t *History = ((mrsv_t *)O)->History;

      mhistory_t *tmpH;

      if (History == NULL)
        {
        return(SUCCESS);
        }

      for (hindex = 0;hindex < History->NumItems;hindex++)
        {
        CE = NULL;

        tmpH = (mhistory_t *)MUArrayListGet(History,hindex);

        tmpLine[0] = '\0';

        MCResToString(&tmpH->Res,0,mfmAVP,tmpLine);

        if (MXMLCreateE(&CE,"event") == FAILURE)
          {
          return(FAILURE);
          }
        
        MXMLSetAttr(CE,(char *)MHistoryAttr[mhaTime],&tmpH->Time,mdfLong);
        MXMLSetAttr(CE,(char *)MHistoryAttr[mhaState],tmpLine,mdfString);

        MXMLAddE(E,CE);
        }
      }  /* END case mxoRsv */

      break;

    default:

      /* NYI */

      break;
    }

  return(SUCCESS);
  }  /* MHistoryToString() */

/* END MObjectHistory.c */
