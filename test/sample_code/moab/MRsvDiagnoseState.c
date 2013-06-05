/* HEADER */

      
/**
 * @file MRsvDiagnoseState.c
 *
 * Contains: MRsvDiagnoseState
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 * Report reservation state and diagnostic status.
 *
 * @see MSRDiag() - peer
 * @see MUIRsvDiagnose() - parent
 *
 * @param R       (I)
 * @param Flags   (I)
 * @param SRE     (I) [either SRE or String must be initialized]
 * @param String  (I) [either SRE or String must be initialized]
 * @param DFormat (I)
 */

int MRsvDiagnoseState(

  mrsv_t                 *R,
  mbitmap_t              *Flags,
  mxml_t                 *SRE,
  mstring_t              *String,
  enum MFormatModeEnum    DFormat)

  {
  int RsvNC;
  int RsvTC;

  static int *Count = NULL;
  static int *TCount = NULL;

  static int  CountSize;

  mjob_t  *J;

  mxml_t  *RE = NULL;  /* set to avoid compiler warning */

  char tmpLine[MMAX_LINE];

  const char *FName = "MRsvDiagnoseState";

  MDB(5,fSTRUCT) MLog("%s(%s,XML,String,%s)\n",
    FName,
    (R != NULL) ? R->Name : "NULL",
    MFormatMode[DFormat]);

  if (R == NULL)
    {
    return(FAILURE);
    }

  if (Count == NULL)
    {
    int MaxTRsv;

    MaxTRsv = MAX(MMAX_RSV_DEPTH,MMAX_RSV_GDEPTH);

    CountSize = (MaxTRsv + 1) * sizeof(int);

    Count  = (int *)MUMalloc(CountSize);
    TCount = (int *)MUMalloc(CountSize);

    if ((Count == NULL) || (TCount == NULL))
      {
      return(FAILURE);
      }
    }    /* END if (Count == NULL) */

  if (DFormat == mfmXML)
    {
    if (SRE == NULL)
      {
      return(FAILURE);
      }

    RE = SRE;
    }
  else
    {
    if (String == NULL)
      {
      return(FAILURE);
      }
    }

  if ((R->Type == mrtUser) ||
     ((R->Type == mrtJob) && (R->DRes.Procs == -1)))
    {
    /* NO-OP */
    }     /* END if ((R->Type == mrtUser) || ... ) */
  else if (R->Type == mrtJob)
    {
    /* verify associated job exists */

    if ((MJobFind(R->Name,&J,mjsmBasic) == FAILURE) &&
        (MJobFind(R->J->Name,&J,mjsmBasic) == FAILURE))
      {
      sprintf(tmpLine,"WARNING:  cannot find job associated with rsv '%s'\n",
        R->Name);

      if (DFormat == mfmXML)
        {
        MXMLSetAttr(
          RE,
          (char *)MRsvAttr[mraMessages],
          (void *)tmpLine,
          mdfString);
        }
      else
        {
        MStringAppend(String,tmpLine);
        }
      }
    else if (J->Rsv != R)
      {
      int rqindex;

      for (rqindex = 0;rqindex < MMAX_REQ_PER_JOB;rqindex++)
        {
        if (J->Req[rqindex] == NULL)
          break;

        if (J->Req[rqindex]->R == R)
          break;
        }  /* END for (rqindex) */

      if (J->Req[rqindex] == NULL)
        {
        sprintf(tmpLine,"WARNING:  job '%s' does not link to rsv\n",
          J->Name);

        if (DFormat == mfmXML)
          {
          MXMLSetAttr(
            RE,
            (char *)MRsvAttr[mraMessages],
            (void *)tmpLine,
            mdfString);
          }
        else
          {
          MStringAppend(String,tmpLine);
          }
        }
      }  /* END else if (J->R != R) */
    }    /* END else if (R->Type == mrtJob) */

  if (bmisset(&R->Flags,mrfAllowGrid) && bmisset(&R->Flags,mrfIsClosed))
    {
    sprintf(tmpLine,"    WARNING:  grid sandbox reservation configured but no access enabled\n");

    if (DFormat == mfmXML)
      {
      MXMLSetAttr(
        RE,
        (char *)MRsvAttr[mraMessages],
        (void *)tmpLine,
        mdfString);
      }
    else
      {
      MStringAppend(String,tmpLine);
      }
    }

  /* check nodes to validate reservation */

  if (bmisset(Flags,mcmVerbose2))
    {
    RsvNC = 0;
    RsvTC = 0;

    if (RsvTC != R->AllocTC)
      {
      if (DFormat != mfmXML)
        {
        MStringAppendF(String,"WARNING:  rsv '%s' has %d task(s) allocated but %d detected\n",
          R->Name,
          R->AllocTC,
          RsvTC);
        }
      }

    if ((R->ReqTC > 0) && (R->AllocTC != R->ReqTC))
      {
      if (DFormat != mfmXML)
        {
        MStringAppendF(String,"WARNING:  rsv '%s' requires %d task(s) but %d allocated\n",
          R->Name,
          R->ReqTC,
          R->AllocTC);
        }
      }

    if (RsvNC != R->AllocNC)
      {
      if (DFormat != mfmXML)
        {
        MStringAppendF(String,"WARNING:  reservation '%s' has %d node(s) allocated but has %d detected\n",
          R->Name,
          R->AllocNC,
          RsvNC);
        }
      }
    }    /* END if (bmisset(Flags,mcmVerbose2)) */

  if ((R->ReqNC > 0) && (R->AllocNC < R->ReqNC))
    {
    if (DFormat != mfmXML)
      {
      MStringAppendF(String,"WARNING:  rsv '%s' requires %d node(s) but has %d allocated\n",
        R->Name,
        R->ReqNC,
        R->AllocNC);
      }
    }

  return(SUCCESS);
  }  /* END MRsvDiagnoseState() */

/* END MRsvDiagnoseState.c */
