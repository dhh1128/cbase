/* HEADER */

/**
 * @file MUINodeXML.c
 *
 * Contains MUI Node XML functions
 */


#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 * Report node usage stats and profile info to XML.
 *
 * @param NodeExp (I)
 * @param STime (I)
 * @param ETime (I)
 * @param EMsg (O)
 * @param EP (O)
 */

int MUINodeStatsToXML(

  char    *NodeExp,  /* I */
  long     STime,    /* I */
  long     ETime,    /* I */
  char    *EMsg,     /* O */
  mxml_t **EP)       /* O */

  {
  mnode_t *N;

  int nindex;

  mxml_t *DE = NULL;

  marray_t NodeList;
 
  char tmpMsg[MMAX_LINE];

  enum MNodeAttrEnum AList[] = {
    mnaRCMem,
    mnaRCProc,
    mnaNONE };

  if ((EP == NULL) || (EMsg == NULL))
    {
    return(FAILURE);
    }

  EMsg[0] = '\0';
  tmpMsg[0] = '\0';

  MUArrayListCreate(&NodeList,sizeof(mnode_t *),-1);

  if ((NodeExp[0] == '\0') || (MUREToList(
        NodeExp,
        mxoNode,
        NULL,
        &NodeList,
        FALSE,
        tmpMsg,
        sizeof(tmpMsg)) == FAILURE))
    {
    snprintf(EMsg,MMAX_LINE,"ERROR:    invalid node expression received '%s' : %s\n",
      NodeExp,
      tmpMsg);

    return(FAILURE);
    }

  MXMLCreateE(&DE,(char *)MSON[msonData]);

  if ((STime == -1) || (ETime == -1))
    {
    MUGetPeriodRange(MSched.Time,0,0,mpDay,&STime,NULL,NULL);
    }

  /* never allow query beyond now */

  if (ETime == -1)
    ETime = MIN((mulong)(MStat.P.IStatEnd - 1),MSched.Time);
  else
    ETime = MIN((mulong)ETime,MSched.Time);

  if (ETime < STime)
    {
    return(FAILURE);
    }

  for (nindex = 0;nindex < NodeList.NumItems;nindex++)
    {
    N = (mnode_t *)MUArrayListGetPtr(&NodeList,nindex);

    MNodeProfileToXML(NULL,N,NULL,AList,STime,ETime,-1,DE,TRUE,TRUE,TRUE);
    }

  *EP = DE;

  MUArrayListFree(&NodeList);

  return(SUCCESS);
  }   /* END MUINodeStatsToXML() */
/* END MUINodeXML */
