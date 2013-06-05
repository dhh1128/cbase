/* HEADER */

/**
 * @file MUIJobSubmitThreadSafe.c
 *
 * Contains MUI Job Sumit function, threadsafe
 */



#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  

/**
 * Threaded routine that accepts a job submission and returns a job id immediately to the client.
 *
 * The job's xml is stored in queue to later be handled by 
 * MS3ProcessSubmitQueue. The xml is also translated to a mtransjob_t struct 
 * and stored in the cache for immediately viewing through showq. 
 *
 * @see MS3ProcessSubmitQueue
 *
 * @param S       (I)
 * @param CFlagBM (I)
 * @param Auth    (I)
 */

int MUIJobSubmitThreadSafe(

  msocket_t *S,
  mbitmap_t *CFlagBM,
  char      *Auth)

  {
  int ID = 0;
  char Name[MMAX_NAME];
  mulong tmpSubmitTime;

  mjob_submit_t *JSubmit = NULL;

  mxml_t *CE = NULL;
	mxml_t *tmpJE = NULL;

  if ((S == NULL) || (S->RDE == NULL))
    {
    return(FAILURE);
    }

  /* extract the XML from the socket, add it to the queue, return a job ID */

  MXMLDupE(S->RDE,&CE);

  MJobSubmitAllocate(&JSubmit);

  JSubmit->JE = CE;

	/* Add submission time to job's xml so that it keeps original time. */

  if (MUGetTime((mulong *)&tmpSubmitTime,mtmNONE,NULL) == FAILURE)
    tmpSubmitTime = MSched.Time;
    
  JSubmit->SubmitTime = tmpSubmitTime;
	
  if (MXMLGetChildCI(CE,(char *)MXO[mxoJob],NULL,&tmpJE) == SUCCESS)
	  {
	  mxml_t *SubmitTimeE = NULL;

		MXMLCreateE(&SubmitTimeE,(char *)MS3JobAttr[ms3jaSubmitTime]);
		MXMLSetVal(SubmitTimeE,(void *)&JSubmit->SubmitTime,mdfLong);
		MXMLAddE(tmpJE,SubmitTimeE);
		}

  MUStrCpy(JSubmit->User,Auth,sizeof(JSubmit->User));

  MSysAddJobSubmitToQueue(JSubmit,&ID,TRUE);

  JSubmit->ID = ID;

  MSysAddJobSubmitToJournal(JSubmit);

  snprintf(Name,sizeof(Name),"%d",ID);

  MJobXMLTransition(CE,Name);

  MUISAddData(S,Name);

  CE = NULL;

  return(SUCCESS);
  }  /* END MUIJobSubmitThreadSafe() */
 /* END MUIJobSubmitThreadSafe.c */ 
