/* HEADER */

      
/**
 * @file MPrioFToX.c
 *
 * Contains:
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 * Print an individual priorityf item to a mstring.
 * The function prints nothing if Value is 0.
 *
 * @param Name    (I)
 * @param Value   (I) 
 * @param SubName (I)   optional, specifies a sub name to format as <Name>[<SubName>]
 * @param Buf     (I/O)
 */

int MPrioFItemToMString(

  char   const  *Name,    /* I */
  double const   Value,   /* I */
  char   const  *SubName, /* I   optional, specifies a sub name to format as <Name>[<SubName>] */
  mstring_t     *Buf)  /* I/O */

  {
  if (Value == 0.0)
    {
    return(SUCCESS);
    }

  if ((Buf == NULL))
    {
    return(FAILURE);
    }

  if (Value == 1.0)
    {
    MStringAppendF(Buf,"%s%s",
      (!Buf->empty()) ? "+" : "",
      Name);
    }
  else
    {
    MStringAppendF(Buf,"%s%.2f*%s",
      ((Value > 0.0) && (!Buf->empty())) ? "+" : "",
      Value,
      Name);
    }

  if (SubName != NULL)
    MStringAppendF(Buf,"[%s]",SubName);


  return(SUCCESS);
  } /* END MPrioFItemToMString() */

/**
 *
 *
 * @param P (I)
 * @param Buf (O) [optional,minsize=MMAX_LINE]
 */

int MPrioFToMString(

  mnprio_t  *P,   /* I */
  mstring_t *Buf) /* O */

  {
  int aindex;

  if ((P == NULL) || (Buf == NULL))
    {
    return(FAILURE);
    }

 
  for (aindex = 0;MNPComp[aindex] != NULL;aindex++)
    {
    char const *Name;
    char const *SubName;
    double Value;

    if ((aindex == mnpcGMetric) || (aindex == mnpcAGRes) || (aindex == mnpcCGRes)
      || (aindex == mnpcFeature))
      continue; /* process later */

    Name = MNPComp[aindex];
    SubName = NULL;
    Value = P->CW[aindex];

    if ((aindex == mnpcArch) && (P->AIndex > 0))
      {
      SubName = MAList[meArch][P->AIndex];
      }

    MPrioFItemToMString(Name,Value,SubName,Buf);
    }    /* END for (aindex) */ 

  if ((P->CW[mnpcGMetric] != 0.0) &&
      (P->GMValues != NULL))
    {
    int mindex;

    for (mindex = 1;mindex < MSched.M[mxoxGMetric];mindex++)
      {
      MPrioFItemToMString(MNPComp[mnpcGMetric],
        P->GMValues[mindex],
        MGMetric.Name[mindex],
        Buf);
      }
    }

  if (P->CW[mnpcAGRes] != 0.0)
    {
    int rindex;

    for (rindex = 1;rindex < MSched.M[mxoxGMetric];rindex++)
      {
      MPrioFItemToMString(MNPComp[mnpcAGRes],
        P->AGRValues[rindex],
        MGRes.Name[rindex],
        Buf);
      }
    MPrioFItemToMString(MNPComp[mnpcAGRes],
      P->AGRValues[0],
      "ANY",
      Buf);
    }

  if (P->CW[mnpcCGRes] != 0.0)
    {
    int rindex;

    for (rindex = 1;rindex < MSched.M[mxoxGMetric];rindex++)
      {
      MPrioFItemToMString(MNPComp[mnpcCGRes],
        P->CGRValues[rindex],
        MGRes.Name[rindex],
        Buf);
      }

    MPrioFItemToMString(MNPComp[mnpcCGRes],
      P->CGRValues[0],
      "ANY",
      Buf);
    }

  if (P->CW[mnpcFeature] != 0.0)
    {
    int findex;

    for (findex = 1; findex < MMAX_ATTR; findex++)
      {
      MPrioFItemToMString(MNPComp[mnpcFeature],
        P->FValues[findex],
        MAList[meNFeature][findex],
        Buf);
      }
    }

  return(SUCCESS);
  }  /* END MPrioFToString() */
/* END MPrioFToX.c */
