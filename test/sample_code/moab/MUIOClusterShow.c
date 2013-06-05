/* HEADER */

      
/**
 * @file MUIOClusterShow.c
 *
 * Contains: MUI Object Cluster Show functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 * Process 'showbf' request - report resources available for immediate use.
 *
 * @see XXX() - peer - process 'mshow -a' request.
 *
 * @param RBuffer (I)
 * @param SBuffer (O)
 * @param DFlags (I)
 * @param Auth (I)
 * @param SBufSize (O)
 */

int MUIOClusterShowARes(

  const char *RBuffer,
  char       *SBuffer,
  mbitmap_t  *DFlags,
  char       *Auth,
  long       *SBufSize)

  {
  mxml_t *E = NULL;

  char    EMsg[MMAX_LINE] = {0};

  mrange_t RRange[2];

  memset(RRange,0,sizeof(RRange));

  MXMLCreateE(&E,(char *)MSON[msonData]);

  if (MClusterShowARes(
        Auth,
        mwpXML,
        DFlags,          /* request data */
        RRange,
        mwpAVP,          
        (void *)RBuffer, /* response data */
        (void **)&E,
        MMAX_BUFFER,
        0,
        NULL,
        EMsg) == FAILURE)
    {
    snprintf(SBuffer,*SBufSize,"%s",
      EMsg); 

    return(FAILURE);
    }

  MXMLToString(E,SBuffer,*SBufSize,NULL,TRUE);

  MXMLDestroyE(&E);

  return(SUCCESS);
  }  /* END MUIOClusterShowARes() */
/* END MUIOClusterShow.c */
