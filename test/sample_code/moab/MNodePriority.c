/* HEADER */

      
/**
 * @file MNodePriority.c
 *
 * Contains:
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 * Alloc a mnprio_t structure and return pointer to same
 *
 * @param Ptr  (I)
 */

int MNPrioAlloc(

  mnprio_t **Ptr)

  {
  mnprio_t *P = (mnprio_t *)MUCalloc(1,sizeof(mnprio_t));

  if (P == NULL)
    {
    return(FAILURE);
    }

  *Ptr = P;

  return(SUCCESS);
  }  /* END MNPrioAlloc() */


/**
 * Deallocate a mnprio_t structure and free the pointer to it.
 *
 * @param Ptr (I) Does a deep free of this structure, frees the structure itself, and sets
 * this pointer to NULL.
 */

int MNPrioFree(

  mnprio_t **Ptr)

  {
  mnprio_t *P = NULL;

  if ((Ptr == NULL) || (*Ptr == NULL))
    return(SUCCESS);

  P = *Ptr;

  MUFree((char **)P->GMValues);
  MUFree((char **)P->AGRValues);
  MUFree((char **)P->CGRValues);

  MUFree((char **)Ptr);

  return(SUCCESS);
  }  /* END MNPrioFree() */




/**
 * Process node allocation priority function.
 *
 * NOTE:  process 'NODECFG[XXX] PRIORITYF='YYY'
 *
 * @see MNodeProcessConfig() - parent
 *
 * @param PP (I/O) [modified,alloc]
 * @param PSpec (I)
 */

int MProcessNAPrioF(

  mnprio_t  **PP,    /* I/O (modified,alloc) */
  const char *PSpec) /* I */

  {
  const char *ptr;
  char *tail;

  double PFactor;

  int   NegCount;

  int   pindex;

  mnprio_t *P = NULL;

  mbool_t IsValid;

  int GIndex = 0;  /* feature, gmetric, or gres index */

  /* NOTE:  should support pure priority value, ie '10*CPROC+500' (NYI) */

  /* FORMAT:  [<WS>][<FACTOR>][<WS>]<COMP>... */
  /*          <FACTOR> -> [+-]<DOUBLE>        */

  /* e.g.  NODEPRIO   6*LOAD + -.01 * CFGMEM - JOBCOUNT */

  if (PP == NULL)
    {
    return(FAILURE);
    }

  if (PSpec == NULL)
    {
    return(FAILURE);
    }

  ptr = PSpec;

  if (*PP == NULL)
    {
    if (MNPrioAlloc(&P) == FAILURE)
      {
      return(FAILURE);
      }

    *PP = P;

    P->CWIsSet = TRUE;
    }
  else
    {
    P = *PP;
    }

  if (MUStrIsEmpty(PSpec))
    {
    /* just return SUCCESS so that the priority structure is essentially reset */

    return(SUCCESS);
    }

  IsValid = FALSE;

  /* FORMAT:  <COEF1> * <FACTOR> [ {+|-} <COEF> * <FACTOR> ] ... */

  while ((ptr != NULL) && (*ptr != '\0'))
    {
    NegCount = 0;

    while (isspace(*ptr) || (*ptr == '-') || (*ptr == '+'))
      {
      if (*ptr == '-')
        NegCount++;

      ptr++;
      }

    if (isalpha(*ptr))
      {
      PFactor = 1.0;
      }
    else
      {
      PFactor = strtod(ptr,&tail);

      ptr = tail;
      }

    while ((isspace(*ptr)) || (*ptr == '*'))
      ptr++;

    pindex = MUGetIndexCI(ptr,MNPComp,TRUE,0);

    if (pindex > 0)
      {
      IsValid = TRUE;

      ptr += strlen(MNPComp[pindex]);

      if ((pindex == mnpcFeature) || 
          (pindex == mnpcGMetric) || 
          (pindex == mnpcAGRes) ||
          (pindex == mnpcCGRes) ||
          (pindex == mnpcArch))
        {
        const char *head;

        char  IName[MMAX_LINE];

        /* FORMAT:  <COMP>[X] */

        if (ptr[0] != '[')
          {
          /* index name required */

          continue;
          }

        head = ptr + 1;

        if ((tail = (char *)strchr(head,']')) == NULL)
          {
          /* ignore, factor is mal-formed */

          continue;
          }

        if (tail - head > (int)sizeof(IName))
          continue;

        MUStrCpy(IName,head,tail - head + 1);

        IName[tail - head + 1] = '\0';

        /* NOTE:  PRIORITYF attribute may be processed before generic metrics,
                  generic resources, or other dynamically discovered attributes
                  are detected.  Consequently, we need to 'add' attributes to
                  the detected attrbute list rather than 'verify' their 
                  presence
        */

        if (pindex == mnpcGMetric)
          {
          /* get gmindex */
          GIndex = MUMAGetIndex(meGMetrics,IName,mAdd);

          if (GIndex == 0)
            continue;
          }
        else if ((pindex == mnpcAGRes) || (pindex == mnpcCGRes))
          {
          if (!strcasecmp(IName,"any"))
            {
            GIndex = 0;
            }
          else
            {
            GIndex = MUMAGetIndex(meGRes,IName,mAdd);

            if (GIndex == 0)
              continue;
            }
          }
        else if (pindex == mnpcArch)
          {
          P->AIndex = MUMAGetIndex(meArch,IName,mAdd);

          if (P->AIndex == 0)
            continue;
          }
        else
          {
          /* extract feature */

          /* race condition: features have not been loaded on the node yet */

          GIndex = MUMAGetIndex(meNFeature,IName,mAdd);

          if (GIndex == 0)
            continue;
          }

        ptr = tail + 1;
        }    /* END if (pindex == X) */
      }      /* END if (pindex > 0) */
    else
      {
      char tmpLine[MMAX_LINE];

      /* NOTE:  invalid characters can also be symbols */

      snprintf(tmpLine,sizeof(tmpLine),"unknown node allocation priority component '%s' specified",
        ptr);

      MMBAdd(&MSched.MB,(char *)tmpLine,NULL,mmbtAnnotation,0,0,NULL);
 
      /* invalid priority factor specified - advance at least one character */
 
      if (*ptr != '\0') /* don't walk off the end. */
        ptr++;
 
      /* walk to end of word */

      while (isalnum(*ptr))
        ptr++;
      }

    /* NOTE: bad factors can override pure priority coefficient */
    /* ie 'SPEED + 12000 + 100*DOG' will be equivalent to 'SPEED + 100' (FIXME) */

    if ((PFactor != 0.0) && (pindex > 0))
      {
      double Value = (NegCount % 2) ? -PFactor : PFactor;

      if (pindex == mnpcGMetric)
        {
        if (P->GMValues == NULL)
          {
          P->GMValues = (double *)MUCalloc(1,sizeof(double) * MSched.M[mxoxGMetric]);
          }

        P->GMValues[GIndex] = Value;
        P->CW[pindex] = 1; /* this is interpreted as a boolean now */
        }
      else if (pindex == mnpcAGRes)
        {
        if (P->AGRValues == NULL)
          {
          P->AGRValues = (double *)MUCalloc(1,sizeof(double) * MSched.M[mxoxGRes]);
          }

        P->AGRValues[GIndex] = Value;
        P->CW[pindex] = 1; /* this is interpreted as a boolean now */
        }
      else if (pindex == mnpcCGRes)
        {
        if (P->CGRValues == NULL)
          {
          P->CGRValues = (double *)MUCalloc(1,sizeof(double) * MSched.M[mxoxGRes]);
          }

        P->CGRValues[GIndex] = Value;
        P->CW[pindex] = 1; /* this is interpreted as a boolean now */
        }
      else if (pindex == mnpcFeature)
        {
        P->FValues[GIndex] = Value;
        P->CW[pindex] = 1; /* this is interpreted as a boolean now */
        }
      else
        {
        P->CW[pindex] = Value;
        }

      }
    }  /* END while (ptr != NULL) */

  if (IsValid == FALSE)
    {
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MProcessNAPrioF() */
/* END MNodePriority.c */
