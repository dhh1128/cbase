/* HEADER */

      
/**
 * @file MNodeSets.c
 *
 * Contains: MNodeSet functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  





/**
 *
 *
 * @param Buf (I)
 * @param SetSelection (O)
 * @param SetType (O)
 * @param SetList (O)
 */

int MNSFromString(

  char                            *Buf,          /* I */
  enum MResourceSetSelectionEnum  *SetSelection, /* O */
  enum MResourceSetTypeEnum       *SetType,      /* O */
  char                           **SetList)      /* O */

  {
  char *ptr;
  char *TokPtr;

  char tmpLine[MMAX_LINE];

  int  sindex;

  if (Buf == NULL)
    {
    return(FAILURE);
    }

  MUStrCpy(tmpLine,Buf,sizeof(tmpLine));

  /* FORMAT:  <SETSELECTION>:<SETTYPE>[:<SETLIST>] */

  if ((ptr = MUStrTok(tmpLine,":",&TokPtr)) == NULL)
    {
    return(FAILURE);
    }

  *SetSelection = (enum MResourceSetSelectionEnum)MUGetIndexCI(
    ptr,
    MResSetSelectionType,
    FALSE,
    mrssNONE);

  if ((ptr = MUStrTok(NULL,":",&TokPtr)) == NULL)
    {
    return(FAILURE);
    }

  *SetType = (enum MResourceSetTypeEnum)MUGetIndexCI(
    ptr,
    MResSetAttrType,
    FALSE,
    mrstNONE);

  sindex = 0;

  while ((ptr = MUStrTok(NULL,":,",&TokPtr)) != NULL)
    {
    MUStrDup(&SetList[sindex],ptr);

    sindex++;

    if (sindex >= MMAX_ATTR)
      break;
    }

  return(SUCCESS);
  }  /* END MNSFromString() */





/**
 *
 *
 * @param DstRQ (O) [modified]
 * @param SrcRQ (I)
 */

int MNSCopy(

  mreq_t       *DstRQ,
  const mreq_t *SrcRQ)

  {
  int lindex;

  if ((SrcRQ == NULL) || (DstRQ == NULL))
    {
    return(FAILURE);
    }

  DstRQ->SetSelection = SrcRQ->SetSelection;

  DstRQ->SetType = SrcRQ->SetType;

  if (SrcRQ->SetList == NULL)
    {
    return(SUCCESS);
    }

  if (DstRQ->SetList == NULL)
    {
    DstRQ->SetList = (char **)MUCalloc(1,sizeof(char *) * MMAX_ATTR);
    }

  for (lindex = 0;lindex < MMAX_ATTR;lindex++)
    {
    if (SrcRQ->SetList[lindex] == NULL)
      {
      if (DstRQ->SetList[lindex] == NULL)
        break;

      MUFree(&DstRQ->SetList[lindex]);
      }
    else
      {
      MUStrDup(&DstRQ->SetList[lindex],SrcRQ->SetList[lindex]);
      }
    }    /* END for (lindex) */

  DstRQ->SetList[lindex] = NULL;

  return(SUCCESS);
  }  /* END MNSCopy() */
/* END MNodeSets.c */
