/* HEADER */

/**
 * @file MVMConfig.c
 * 
 * Contains various functions for VM configuration
 *
 */


#include "moab.h"
#include "moab-proto.h"
#include "moab-const.h"
#include "moab-global.h"


/**
 * Process attributes of VMCFG[] parameter.
 *
 * @see MVMLoadConfig()
 *
 * @param VM    (I) [modified]
 * @param Value (I)
 */

int MVMProcessConfig(

  mvm_t *VM,     /* I (modified) */
  char  *Value) /* I */

  {
  int   aindex;

  int   tmpI;    /* attribute operation */

  char *ptr;
  char *TokPtr;

  char  ValLine[MMAX_LINE];

  if ((VM == NULL) ||
      (Value == NULL) ||
      (Value[0] == '\0'))
    {
    return(FAILURE);
    }

  /* process value line */

  TokPtr = Value;

  while ((ptr = MUStrTok(NULL," \t\n",&TokPtr)) != NULL)
    {
    /* parse name-value pairs */

    /* FORMAT:  <VALUE>[,<VALUE>] */

    if (MUGetPair(
          ptr,
          (const char **)MVMAttr,
          &aindex,  /* O */
          NULL,
          TRUE,
          &tmpI,    /* O (comparison) */
          ValLine,
          sizeof(ValLine)) == FAILURE)
      {
      /* cannot parse value pair */

      MDB(3,fCONFIG) MLog("INFO:     unknown attribute '%s' specified for VM %s\n",
        ptr,
        VM->VMID);

      continue;
      }

    switch (aindex)
      {
      case mvmaSovereign:

      MVMSetAttr(VM,(enum MVMAttrEnum)aindex,ValLine,mdfString,mSet);

      break;

      case mvmaActiveOS:
      case mvmaADisk:
      case mvmaAMem:
      case mvmaAProcs:
      case mvmaCDisk:
      case mvmaCMem:
      case mvmaCProcs:
      case mvmaDescription:
      case mvmaFlags:
      case mvmaID:
      case mvmaNextOS:
      case mvmaNodeTemplate:
      case mvmaOSList:
      case mvmaRackIndex:
      case mvmaSlotIndex:
      case mvmaState:
      case mvmaSubState:
      case mvmaVariables:

        /* for now support only specifying sovereignty */

        break;

      case mvmaNONE:
      case mvmaLAST:
      default:

        assert(FALSE);

        break;
      }  /* END switch (aindex) */
    }    /* END while (ptr != NULL) */

  return(SUCCESS);
  }  /* END MAdminProcessConfig() */





/**
 * Loads ADMINCFG parameters from configuration.
 *
 * @see MAdminProcessConfig()
 * @see MVMProcessConfig() - child
 *
 * @param Buf (I) [optional]
 */

int MVMLoadConfig(

  char *Buf)  /* I (optional) */

  {
  char   IndexName[MMAX_NAME];

  char   Value[MMAX_LINE];

  char  *ptr;
  char  *head;

  mvm_t *VM;

  /* FORMAT:  <KEY>=<VAL>[<WS><KEY>=<VAL>]...         */
  /*          <VAL> -> <ATTR>=<VAL>[:<ATTR>=<VAL>]... */

  /* load all/specified AM config info */

  head = (Buf != NULL) ? Buf : MSched.ConfigBuffer;

  if (head == NULL)
    {
    return(FAILURE);
    }

  /* load all sched config info */

  ptr = head;

  IndexName[0] = '\0';

  while (MCfgGetSVal(
           head,
           &ptr,
           MParam[mcoVMCfg],
           IndexName,
           NULL,
           Value,
           sizeof(Value),
           0,
           NULL) != FAILURE)
    {
    if (IndexName[0] != '\0')
      {
      /* set admin index */

      if (MVMFind(IndexName,&VM) == FAILURE)
        {
        if (MVMAdd(IndexName,&VM) == FAILURE)
          {
          /* TODO: log something useful here */

          IndexName[0] = '\0';

          continue;
          }
        }
      }
    else
      {
      /* TODO: log something useful here */

      continue;
      }

    MVMProcessConfig(VM,Value);

    IndexName[0] = '\0';
    }  /* END while (MCfgGetSVal() != FAILURE) */

  return(SUCCESS);
  }  /* END MVMLoadConfig() */

/* END MVMConfig.c */
