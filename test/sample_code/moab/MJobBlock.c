/* HEADER */

/**
 * @file MJobBlock.c
 *
 * Contains Job Block functions
 */



#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 * Get the job block record (contains block message and block reason) for the 
 * specified partition from the linked list of block messages. 
 *  
 * @param J (I)
 * @param P (I)   partition [optional] 
 * @param BIPtr (0) (block info pointer) 
 */

int MJobGetBlockInfo(

  mjob_t *J,
  mpar_t *P,
  mjblockinfo_t **BIPtr) 

  {
  int    PIndex = 0;
  mln_t *LinkPtr;
  char   LinkName[MMAX_LINE];

  if ((J == NULL) || (BIPtr == NULL))
    return(FAILURE);

  *BIPtr = NULL;

  if (J->BlockInfo == NULL)
    return(FAILURE);

  /* If no partition specified, use the ALL partition */

  if (P != NULL)
    PIndex = P->Index;

  /* see if the job has a block record for the specified partition */

  sprintf(LinkName,"%s_%d",MPar[PIndex].Name,PIndex);

  if (MULLCheck(J->BlockInfo,LinkName,&LinkPtr) != SUCCESS)
    {
    MDB(8,fSCHED) MLog("INFO:     job %s block info for block record name %s not found \n",
      J->Name,
      LinkName);

    return(FAILURE);
    }

  *BIPtr = (mjblockinfo_t *)LinkPtr->Ptr;

  return(SUCCESS);
  } /* END MJobGetBlockInfo() */
    


/**
 * Free the alloc data in the block record and then free the block record.
 *
 * @param BlockInfoPtr (I) [modified]
 */

int MJobBlockInfoFree(

  mjblockinfo_t **BlockInfoPtr)  /* I (modified) */

  {
  mjblockinfo_t *BI;

  if ((BlockInfoPtr == NULL) || (*BlockInfoPtr == NULL))
    {
    return(SUCCESS);
    }

  BI = *BlockInfoPtr;

  /* free the alloc fields in the block record */

  MUFree(&BI->BlockMsg);

  /* free the block record */

  MUFree((char **)BlockInfoPtr);

  return(SUCCESS);
  }  /* END MJobBlockInfoFree() */
    
    
        
/**
 * Add a block record to the linked list of block records for this job. 
 * If a block record already exists for the requested partition, the 
 * current record is replaced with the new one. 
 *  
 * @param J (I/O)
 * @param P (I)   (optional partition) 
 * @param BIPtr (I) (block info pointer) 
 */

int MJobAddBlockInfo(

  mjob_t *J,
  mpar_t *P,
  mjblockinfo_t *BIPtr)  /* pointer to alloc'ed record */

  {
  int    PIndex = 0;
  mln_t *LinkPtr = NULL;
  char   LinkName[MMAX_LINE];
  mjblockinfo_t *BlockInfoPtr = NULL;

  if ((J == NULL) || (BIPtr == NULL))
    return(FAILURE);

  if (P != NULL)
    PIndex = P->Index;

  sprintf(LinkName,"%s_%d",MPar[PIndex].Name,PIndex);

  /* if no entries in the link list then create the list.
     if an entry already exists for the requested partition
     delete it before we add the new entry. */
 
  if (J->BlockInfo == NULL)
    {
    MULLCreate(&J->BlockInfo);

    MDB(8,fSCHED) MLog("INFO:     job %s (par %s) creating block record list\n",
       J->Name,
       LinkName);
    }
  else if (MULLCheck(J->BlockInfo,LinkName,&LinkPtr) == SUCCESS)
    {
    mjblockinfo_t *BlockInfoPtr = (mjblockinfo_t *)LinkPtr->Ptr;

    MDB(8,fSCHED) MLog("INFO:     job %s (par %s) removing block record (%d) (%s)\n",
       J->Name,
       LinkPtr->Name,
       BlockInfoPtr->BlockReason,
       BlockInfoPtr->BlockMsg);

    MULLRemove(&J->BlockInfo,LinkPtr->Name,(int(*)(void **))MJobBlockInfoFree);
    }
 
  /* add the requested block record.
     note that it is alloc'ed by the calling function and free'ed when
     it is either replaced with another block record or when the job is destroyed */

  if ((BIPtr->BlockReason != mjneNONE) && (BIPtr->BlockMsg != NULL))
    {
    MDB(7,fSCHED) MLog("INFO:     job %s (par %s) adding block record (%d) (%s)\n",
       J->Name,
       LinkName,
       BIPtr->BlockReason,
       BIPtr->BlockMsg);
    }
   
  MULLAdd(&J->BlockInfo,LinkName,(void *)BIPtr,NULL,(int(*)(void **))MJobBlockInfoFree);

  /* if we are adding a block record for a specific partition and no entry has
     been added to the 0 partition (or the current entry does not match) then put this
     block message on the 0 partition so that a block message will still show up for any
     non-per partition operations which may only be checking the 0 partition. */

  if ((PIndex != 0) && 
      ((MJobGetBlockInfo(J,NULL,&BlockInfoPtr) != SUCCESS) ||
      (MJobCmpBlockInfo(BlockInfoPtr,BIPtr->BlockReason,BIPtr->BlockMsg) == FALSE)))
    {
    mjblockinfo_t *copyBIPtr = NULL;

    /* alloc a new block record and populate it from the current block record */

    MJobDupBlockInfo(&copyBIPtr,BIPtr);

    /* recursive call to add the block record to the 0 partition */

    if (MJobAddBlockInfo(J,NULL,copyBIPtr) != SUCCESS)
      MJobBlockInfoFree(&copyBIPtr);

    } /* END if PIndex */

  return(SUCCESS);
  } /* END MJobAddBlockInfo() */
    
    
    
/**
 * alloc a new block message record and populate it using source block record. 
 * free memory pointed to by destination record pointer if it is non-NULL. 
 *  
 * @param DstBIPtr
 * @param SrcBIPtr
 */

int MJobDupBlockInfo(

  mjblockinfo_t **DstBIPtr,
  mjblockinfo_t  *SrcBIPtr)

  {
  mjblockinfo_t  *BlockInfoPtr;

  if ((DstBIPtr == NULL) || (SrcBIPtr == NULL))
    return(FAILURE);

  if (*DstBIPtr != NULL)
    MJobBlockInfoFree(DstBIPtr);

  BlockInfoPtr = (mjblockinfo_t *)MUCalloc(1,sizeof(mjblockinfo_t));

  BlockInfoPtr->PIndex =  SrcBIPtr->PIndex;
  BlockInfoPtr->BlockReason =  SrcBIPtr->BlockReason;
  MUStrDup(&BlockInfoPtr->BlockMsg,SrcBIPtr->BlockMsg);

  *DstBIPtr = BlockInfoPtr;

  return(SUCCESS);
  } /* END MJobAddBlockInfo() */



/**
 * this function returns the job's block reason for the specified partition 
 *  
 * @param J (I)
 * @param P (I)   (optional partition) 
 */

enum MJobNonEType MJobGetBlockReason(

  mjob_t *J,
  mpar_t *P) 

  {
  mjblockinfo_t *BI;

  if (MJobGetBlockInfo(J,P,&BI) != SUCCESS)
    return(mjneNONE);

  return(BI->BlockReason);
  } /* END MJobGetBlockInfo() */
    
    
/**
 * this function returns a pointer to the job's block message for the requested partition 
 *  
 * @param J (I)
 * @param P (I)   (optional partition) 
 */

char *MJobGetBlockMsg(

  mjob_t *J,
  mpar_t *P) 

  {
  mjblockinfo_t *BI;

  if (MJobGetBlockInfo(J,P,&BI) != SUCCESS)
    return(NULL);

  return(BI->BlockMsg);
  } /* END MJobGetBlockMsg() */
    
    
    
    
/**
 * this function compares the block reason and description with the same fields in 
 * a blockinfo structure and returns TRUE if they match. 
 *  
 * @param BI (I)
 * @param BlockReason (I) 
 * @param BlockReasonDesc 
 */

mbool_t MJobCmpBlockInfo(

  mjblockinfo_t      *BI,
  enum MJobNonEType   BlockReason,
  const char         *BlockReasonDesc)

  {
  if (BI == NULL)
    {
    MDB(8,fSCHED) MLog("INFO:     no blockinfo to compare with block record\n");

    return(FALSE);
    }

  if (BI->BlockReason != BlockReason)
    {
    MDB(8,fSCHED) MLog("INFO:     blockinfo block reason (%d) fails compare with (%d) block record\n",
       BI->BlockReason,
       BlockReason);

    return(FALSE);
    }

  if (strcmp(((BlockReasonDesc == NULL) ? "" : BlockReasonDesc),
             ((BI->BlockMsg == NULL) ? "" : BI->BlockMsg)))
    {
    MDB(8,fSCHED) MLog("INFO:     blockinfo block message (%s) fails compare with (%s) block record\n",
       BI->BlockMsg,
       BlockReasonDesc);

    return(FALSE);
    }

  return(TRUE);
  } /* END MJobCmpBlockInfo */
/* END MJobBlock.c */
