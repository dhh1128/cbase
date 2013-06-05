/* HEADER */

      
/**
 * @file MSchedGetAttr.c
 *
 * Contains: MSchedGetAttr
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 * Set specified scheduler attributes.
 *
 * @see MSchedAToString() - peer
 * @see MSchedProcessConfig() - parent
 *
 * @param Attr (I)
 * @param DFormat (I)
 * @param Dest (O)
 * @param DestSize (I) [optional]
 */

int MSchedGetAttr(
  
  enum MSchedAttrEnum     Attr,     /* I */
  enum MDataFormatEnum    DFormat,  /* I */
  void                  **Dest,     /* O */
  int                     DestSize) /* I (optional) */

  {
  long tmpL;

  if (Dest == NULL)
    {
    return(FAILURE);
    }

  switch (Attr)
    {
    case msaDefNodeAccessPolicy:

      if (DFormat != mdfInt)
        {
        return(FAILURE);
        }

      *Dest = (void *)MSched.DefaultNAccessPolicy;

      break;

    case msaDBHandle:

      {
      mdb_t *MDBInfo;

      MUMutexLock(&MDBHandleHashLock);
      MUHTGetInt(&MDBHandles,MUGetThreadID(),(void **)&MDBInfo,NULL);
      MUMutexUnlock(&MDBHandleHashLock);

      if (MDBInfo == NULL)
        MDBInfo = &MDBFakeHandle;  /* MDBFakeHandle should be 0'd out */

      *Dest = MDBInfo;
      }

      break;

    case msaIterationCount:

      if (DFormat != mdfInt)
        {
        return(FAILURE);
        }
   
      /* 2 step casting required to eliminate compiler warnings */

      tmpL = (long)MSched.Iteration;

      *Dest = (void *)tmpL;

      break;

    case msaJobMaxNodeCount:

      if (DFormat != mdfInt)
        {
        return(FAILURE);
        }

      tmpL = (long)MSched.JobMaxNodeCount;

      *Dest = (void *)tmpL;

      break;

    case msaMaxJob:

      if (DFormat != mdfInt)
        {
        return(FAILURE);
        }

      tmpL = (long)MSched.M[mxoJob];

      *Dest = (void *)tmpL;

      break;

    case msaMode:

      if (DFormat != mdfInt)
        {
        return(FAILURE);
        }

      *Dest = (void *)MSched.Mode;

      break;

    case msaServer:

      if (DFormat != mdfString)
        {
        return(FAILURE);
        }

      MUStrCpy((char *)Dest,MSched.ServerHost,DestSize);

      break;

    case msaTime:

      if (DFormat != mdfLong)
        {
        return(FAILURE);
        }

      *Dest = (void *)MSched.Time;

      break;

    case msaUID:

      if (DFormat != mdfInt)
        {
        return(FAILURE);
        }

      tmpL = (long)MSched.UID;

      *Dest = (void *)tmpL;

      break;

    case msaVarDir:

      if (DFormat != mdfString)
        {
        return(FAILURE);
        }

      MUStrCpy((char *)Dest,MSched.VarDir,DestSize);

      break;

    default:

      /* NO-OP */

      break;
    }  /* END switch (Attr) */

  return(SUCCESS);
  }  /* END MSchedGetAttr() */


/**
 * Returns the pointer of the "moab home directory" (VarDir) attribute
 * as contained in the MSched.VarDir field
 *
 * Called VarDir because it was initialized by an ENV Variable
 *   $MOABHOMEDIR
 *
 */

const char *MSchedGetHomeDir()
  {
  return MSched.VarDir;   /* return the field pointer of the 'home directory' */
  }

/* END MSchedGetAttr.c */
