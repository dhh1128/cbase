/* HEADER */

      
/**
 * @file MGRes.c
 *
 * Contains: MGRes Functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 *
 */

int MGResInitialize()

  {
  int gindex;

  if ((MGRes.Name != NULL) && (MGRes.GRes != NULL))
    {
    /* Already been initialized */

    return(SUCCESS);
    }

  /* Set default GRes value if not previously configured */

  if (MSched.M[mxoxGRes] == 0)
    MSched.M[mxoxGRes] = MDEF_MAXGRES;

  MGRes.Name = (char **)MUCalloc(1,sizeof(char *) * MSched.M[mxoxGRes]);
  MGRes.GRes = (mgres_t **)MUCalloc(1,sizeof(mgres_t *) * MSched.M[mxoxGRes]);

  for (gindex = 0;gindex < MSched.M[mxoxGRes];gindex++)
    {
    MGRes.Name[gindex] = (char *)MUMalloc(sizeof(char) * MMAX_NAME);
    MGRes.GRes[gindex] = (mgres_t *)MUCalloc(1,sizeof(mgres_t));

    MGRes.Name[gindex][0] = '\0';
    }

  return(SUCCESS);
  }  /* END MGResInitialize() */


/**
 * Perform A += B;
 *
 * @param A (O)
 * @param B (I)
 */

int MGResPlus(

  msnl_t *A,
  const msnl_t *B)

  {
  int index;

  for (index = 0;index < MSched.M[mxoxGRes];index++)
    {
    if (MGRes.Name[index][0] == '\0')
      break;

    MSNLSetCount(A,index,MSNLGetIndexCount(A,index) + MSNLGetIndexCount(B,index));
    }

  return(SUCCESS);
  }  /* END MGResPlus() */


/**
 * Process GRESCFG config line
 *
 * @param GIndex (I) gres index
 * @param ValLine (I) attribute-value string [modified]
 */

int MGResProcessConfig(

  int   GIndex,
  char *ValLine)

  {
  int   aindex;
  int   tmpI;

  char  ValBuf[MMAX_LINE];

  char *ptr;
  char *TokPtr;

  mgres_t *G;

  if (GIndex <= 0)
    {
    return(FAILURE);
    }

  if ((ValLine == NULL) || (ValLine[0] == '\0'))
    {
    return(SUCCESS);
    }

  G = MGRes.GRes[GIndex];

  /* FORMAT:  GRESCFG[X] {TYPE,CHARGERATE,PRIVATE}=<A> ... */

  /* process value line */

  ptr = MUStrTokE(ValLine," \t\n",&TokPtr);

  while (ptr != NULL)
    {
    /* parse name-value pairs */

    /* FORMAT:  <ATTR>[INDEXNAME]=<VALUE>[,<VALUE>] */
    /*          <INDEXNAME> -> {[}<OTYPE>[:<OID>]{]} */

    if (MUGetPairCI(
          ptr,
          (const char **)MGResAttr,
          &aindex,
          NULL,       /* O (attribute array index) */
          FALSE,      /* (use relative) */
          &tmpI,      /* O (comparison) */
          ValBuf,     /* O (value) */
          sizeof(ValBuf)) == FAILURE)
      {
      return(FAILURE);
      }

    switch(aindex)
      {
      case mgresaChargeRate:

        G->ChargeRate = strtod(ValBuf,NULL);

        break;

      case mgresaPrivate:

        G->IsPrivate = MUBoolFromString(ValBuf,FALSE);

        break;

      case mgresaInvertTaskCount:

        G->InvertTaskCount = MUBoolFromString(ValBuf,FALSE);

        break;

      case mgresaStartDelay:

        G->StartDelay = MUTimeFromString(ValBuf);

        break;

      case mgresaType:

        MUStrCpy(G->Type,ValBuf,sizeof(G->Type));

        break;

      case mgresaFeatureGRes:

        G->FeatureGRes = MUBoolFromString(ValBuf,FALSE);

        MUGetNodeFeature(MGRes.Name[GIndex],mAdd,NULL);

        break;

      default:

        /* parameter not handled - should we fail here? */

        /* NO-OP */

        break;
      }  /* END switch(aindex) */

    ptr = MUStrTokE(NULL," \t\n",&TokPtr);
    }    /* END while (ptr != NULL) */

  return(SUCCESS);
  }  /* END MGResProcessConfig() */


/* END MGRes.c */
