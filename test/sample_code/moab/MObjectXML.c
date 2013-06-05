/* HEADER */

      
/**
 * @file MObjectXML.c
 *
 * Contains: Object XML functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 * Populate object subcomponent (stat, limit, fs, etc) from XML.
 *
 * @see MOFromString() - parent 
 * @see MStatFromXML() - child
 * @see MLimitFromXML() - child
 * @see MFSFromXML() - child
 * @see MOToXML() - peer
 *
 * @param O (I) [modified]
 * @param OIndex (I)
 * @param E (I)
 * @param P (I) [optional]
 */

int MOFromXML(
 
  void               *O,
  enum MXMLOTypeEnum  OIndex,
  mxml_t             *E,
  mpar_t             *P)
 
  {
  int aindex;
  int cindex;
 
  int caindex;

  /* components for most recent object queried */

  static mfs_t    *F = NULL;
  static mcredl_t *L = NULL;
  static must_t   *S = NULL; 

  static void *CO = NULL;

  mxml_t *C;

  if ((O == NULL) || (E == NULL))
    {
    return(FAILURE);
    }
 
  /* NOTE:  do not initialize.  may be overlaying data */

  /* set attributes */

  switch (OIndex)
    {
    case mxoSys:

      for (aindex = 0;aindex < E->ACount;aindex++)
        {
        if ((caindex = MUGetIndex(E->AName[aindex],MSysAttr,FALSE,0)) != 0)
          {
          MSysSetAttr(
            (msched_t *)O,
            (enum MSysAttrEnum)caindex,
            (void **)E->AVal[aindex],
            mdfString,
            mSet);
          }
        }  /* END for (aindex) */

      break;

    case mxoTrig:

      if (MTrigFromXML((mtrig_t *)O,E) == FAILURE)
        {
        return(FAILURE);
        }
  
      if (MTrigInitialize((mtrig_t *)O) == FAILURE)
        {
        return(FAILURE);
        }

      break;

    case mxoAcct:
    case mxoUser:
    case mxoGroup:
    case mxoClass:
    case mxoQOS:

      for (aindex = 0;aindex < E->ACount;aindex++)
        {
        caindex = MUGetIndex(E->AName[aindex],MCredAttr,FALSE,0);
 
        if (caindex == 0)
          {
          /* handle cred specific attributes */

          switch (OIndex)
            {
            case mxoClass:

              caindex = MUGetIndex(E->AName[aindex],MClassAttr,FALSE,0);

              if (caindex != 0)
                {
                MClassSetAttr(
                  (mclass_t *)O,
                  P,
                  (enum MClassAttrEnum)caindex,
                  (void **)E->AVal[aindex],
                  mdfString,
                  mSet);
                }

              break;

            default:

              /* NO-OP */

              break;
            }  /* END switch (OIndex) */

          continue;
          }  /* END if (caindex == 0) */

        MCredSetAttr(
          O,
          OIndex,
          (enum MCredAttrEnum)caindex,
          P,
          (void **)E->AVal[aindex],
          NULL,
          mdfString,
          mSet);
        }  /* END for (aindex) */

      break;

    case mxoJob:

      if (MJobFromXML((mjob_t *)O,E,TRUE,mSet) == FAILURE)
        {
        return(FAILURE);
        }

      break;

    case mxoSched:
    default:

      /* not supported */

      break;
    }  /* END switch (OIndex) */

  if ((OIndex != mxoTrig) && (O != CO))
    { 
    /* load/cache child pointers */

    if ((MOGetComponent(O,OIndex,(void **)&F,mxoxFS) == FAILURE) ||
        (MOGetComponent(O,OIndex,(void **)&L,mxoxLimits) == FAILURE) ||
        (MOGetComponent(O,OIndex,(void **)&S,mxoxStats) == FAILURE))
      {
      /* cannot get components */
 
      return(FAILURE);
      }

    CO = O;
    }  /* END if (O != CO) */

  /* load children */

  for (cindex = 0;cindex < E->CCount;cindex++)
    {
    C = E->C[cindex];
 
    caindex = MUGetIndex(C->Name,MXO,FALSE,0);

    if (!strcmp(C->Name,"MESSAGE"))
      {
      mmb_t **MBP;

      switch (OIndex)
        {
        case mxoUser:
        case mxoGroup:
        case mxoAcct:
        default:

          MBP = &((mgcred_t *)O)->MB;

          break;

        case mxoQOS:

          MBP = &((mqos_t *)O)->MB;

          break;

        case mxoClass:

          MBP = &((mclass_t *)O)->MB;

          break;
        }  /* END switch (OIndex) */

      MMBFromXML(MBP,C);

      continue;
      }

    if (caindex == 0)
      continue;
 
    switch (caindex) 
      {
      case mxoxStats:
 
        MStatFromXML((void *)S,msoCred,C);
 
        break;

      case mxoxLimits:

        MLimitFromXML(L,C);

        break;

      case mxoxFS:

        MFSFromXML(F,C);

        break;
 
      default:

        /* not handled */

        return(FAILURE);

        /*NOTREACHED*/
 
        break;
      }  /* END switch (caindex) */
    }    /* END for (cindex) */
 
  return(SUCCESS);
  }  /* END MOFromXML() */





/**
 * Report generic sub-object in XML (stat, fs, limit).
 *
 * @see MCOToXML() - parent
 * @see MFSToXML() - child
 * @see MLimitToXML() - child
 * @see MStatToXML() - child
 *
 * @param O            (I)
 * @param OIndex       (I)
 * @param AList        (I) [optional]
 * @param ClientOutput (I)
 * @param EP           (O)
 */

int MOToXML(

  void     *O,
  enum MXMLOTypeEnum OIndex,
  void     *AList,
  mbool_t   ClientOutput,
  mxml_t  **EP)

  {
  if ((O == NULL) || (EP == NULL))
    {
    return(FAILURE);
    }

  *EP = NULL;

  switch (OIndex)
    {
    case mxoxFS:

      MFSToXML((mfs_t *)O,EP,(enum MFSAttrEnum *)AList);

      break;

    case mxoxLimits:

      MLimitToXML((mcredl_t *)O,EP,(enum MLimitAttrEnum *)AList);

      break;

    case mxoxStats:

      MStatToXML(O,msoCred,EP,ClientOutput,(enum MStatAttrEnum *)AList);

      break;

    default:

      /* NOT HANDLED */

      return(FAILURE);

      /*NOTREACHED*/

      break;
    }  /* END switch (OIndex) */

  if (*EP == NULL)
    {
    return(FAILURE);
    }

  return(SUCCESS);
  }  /* END MOToXML() */



/**
 *
 *
 * @param O (I) [modified]
 * @param OIndex (I)
 * @param EString (I)
 */

int MOFromString(

  void  *O,       /* I (modified) */
  enum MXMLOTypeEnum OIndex,  /* I */
  const char  *EString) /* I */

  {
  mxml_t *E = NULL;

  int rc;

  if ((O == NULL) || (EString == NULL))
    {
    return(FAILURE);
    }

  if (MXMLFromString(&E,EString,NULL,NULL) == FAILURE)
    {
    return(FAILURE);
    }

  rc = MOFromXML(O,OIndex,E,NULL);

  MXMLDestroyE(&E);

  return(rc);
  }  /* END MOFromString() */


/* END MObjectXML.c */
