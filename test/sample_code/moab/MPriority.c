/* HEADER */

/**
 * @file MPriority.c
 *
 * Moab Priorities
 */
        
/* Contains:                                  *
 *   int MPrioConfigShow()                    *
 *                                            */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-global.h"  
#include "moab-const.h"  


/**
 * Report priority configuration (weights/caps).
 *
 * @see MUIConfigShow()
 *
 * @param ShowVerbose (I)
 * @param P (I) partition
 * @param String (O)
 */

int MPrioConfigShow(

  mbool_t    ShowVerbose,
  mpar_t    *P,
  mstring_t *String)

  {
  int     pindex;  /* priority component/subcomponent index */
  mfsc_t *F;

  char    tmpLine[MMAX_LINE];

  if (String == NULL)
    {
    return(FAILURE);
    }

  MStringSet(String,"\0");

  F = &P->FSC;   

  MStringAppend(String,"\n# Priority Weights\n\n");
 
  for (pindex = 1;pindex < mpcLAST;pindex++)
    {
    if (F->PCW[pindex] != MCONST_PRIO_NOTSET)
      MStringAppend(String,MUShowSLArray(MParam[mcoServWeight + pindex - 1],P->Name,F->PCW[pindex],NULL,0));
    }  /* END for (pindex) */
 
  for (pindex = 1;pindex < mpsLAST;pindex++)
    {
    if (F->PSW[pindex] != MCONST_PRIO_NOTSET)
      MStringAppend(String,MUShowSLArray(MParam[mcoSQTWeight + pindex - 1],P->Name,F->PSW[pindex],NULL,0));
    }  /* END for (pindex) */
 
  if (F->XFMinWCLimit != -1)
    {
    char TString[MMAX_LINE];

    MULToTString(F->XFMinWCLimit,TString);

    MStringAppend(String,MUShowSSArray(MParam[mcoXFMinWCLimit],P->Name,TString,tmpLine,sizeof(tmpLine)));
    }
 
  for (pindex = 1;pindex < mpcLAST;pindex++)
    {
    if (F->PCC[pindex] > 0)
      MStringAppend(String,MUShowSLArray(MParam[mcoServCap + pindex - 1],P->Name,F->PCC[pindex],NULL,0));
    }  /* END for (pindex) */
 
  for (pindex = 1;pindex < mpsLAST;pindex++)
    {
    if (F->PSC[pindex] > 0)
      MStringAppend(String,MUShowSLArray(MParam[mcoSQTCap + pindex - 1],P->Name,F->PSC[pindex],NULL,0));
    }  /* END for (pindex) */
 
  MStringAppend(String,"\n");

  return(SUCCESS);
  }  /* END MPrioConfigShow() */

/* END MPriority.c */

