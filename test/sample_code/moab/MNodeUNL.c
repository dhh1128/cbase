/* HEADER */

      
/**
 * @file MNodeUNL.c
 *
 * Contains: MUNL functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 *
 *
 * @param NL (I)
 * @param NAttr (I)
 * @param NPtr (O) [optional]
 * @param Val (O)
 */

int MUNLGetMinAVal(
 
  mnl_t             *NL,     /* I */
  enum MNodeAttrEnum NAttr,  /* I */
  mnode_t          **NPtr,   /* O (optional) */
  void             **Val)    /* O */
 
  {
  mnode_t *N;

  int nindex;

  double ChargeRate;

  const char *FName = "MUNLGetMinAVal";
 
  MDB(6,fSTRUCT) MLog("%s(NL,NAttr,N,Val)\n",
    FName);

  if (Val != NULL)
    {
    /* set defaults */

    switch (NAttr)
      {
      case mnaSpeed:

        *(double *)Val = 1.0;

        break;

      case mnaChargeRate:

        if (MSched.DefaultN.ChargeRate != 0.0)
          *(double *)Val = MSched.DefaultN.ChargeRate;
        else
          *(double *)Val = 1.0;

        break; 

      case mnaProcSpeed:

        *(int *)Val = 1;

        break;

      case mnaNodeType:

        strcpy((char *)Val,MDEF_NODETYPE);

        break;

      default:

        /* NO-OP */

        break;
      }
    }    /* END if (Val != NULL) */
 
  if ((NL == NULL) || (MNLIsEmpty(NL)) || (Val == NULL))
    {
    return(FAILURE);
    }
 
  switch (NAttr)
    {
    case mnaChargeRate:

      {
      double MinRate = MMAX_NODECHARGERATE;

      for (nindex = 0;MNLGetNodeAtIndex(NL,nindex,&N) == SUCCESS;nindex++)
        {
        ChargeRate = (N->ChargeRate != 0.0) ? 
          N->ChargeRate : 
          MPar[MAX(0,N->PtIndex)].DefNodeChargeRate;

        if (ChargeRate < MinRate)
          {
          MinRate = ChargeRate;

          if (NPtr != NULL)
            *NPtr = N;
          }
        }    /* END for (nindex) */

      if ((MinRate < 0.0) || (MinRate > MMAX_NODECHARGERATE))
        {
        if (NPtr != NULL)
          MNLGetNodeAtIndex(NL,0,NPtr);

        /* charge rate not located */

        if (Val != NULL)
          return(SUCCESS);
        else
          return(FAILURE);
        }

      /* return minimum rate */

      *(double *)Val = MinRate;
      }  /* END BLOCK */

      break;

    case mnaSpeed:
 
      {
      double MinSpeed = 999999999999.0;
 
      if (!bmisset(&MPar[0].Flags,mpfUseMachineSpeed))
        {
        if (NPtr != NULL)
          MNLGetNodeAtIndex(NL,0,NPtr);
 
        return(SUCCESS);
        }
 
      for (nindex = 0;MNLGetNodeAtIndex(NL,nindex,&N) == SUCCESS;nindex++)
        {
        if (N->Speed < MinSpeed)
          {
          MinSpeed = N->Speed;
  
          if (NPtr != NULL)
            *NPtr = N;
          }
        }    /* END for (nindex) */
 
      if ((MinSpeed <= 0.0) || (MinSpeed > 99999999.0)) 
        {
        if (NPtr != NULL)
          MNLGetNodeAtIndex(NL,0,NPtr);

        return(FAILURE);
        }
 
      *(double *)Val = MinSpeed;
      }  /* END BLOCK */
 
      break;
 
    case mnaProcSpeed:
 
      {
      int MinSpeed = 99999999;

      for (nindex = 0;MNLGetNodeAtIndex(NL,nindex,&N) == SUCCESS;nindex++)
        {
        if (N->ProcSpeed < MinSpeed)
          {
          MinSpeed = N->ProcSpeed;
 
          if (NPtr != NULL)
            *NPtr = N;
          }
        }    /* END for (nindex) */
 
      if ((MinSpeed <= 0) || (MinSpeed > 99999999))
        {
        if (NPtr != NULL)
          MNLGetNodeAtIndex(NL,0,NPtr);
 
        return(FAILURE);
        }
 
      *(int *)Val = MinSpeed;
      }  /* END BLOCK */
 
      break;

    case mnaNodeType:

      {
      int    MinPSpeed;
      double MinSpeed;

      mnode_t *N1 = NULL;
      mnode_t *N2 = NULL;

      if ((MUNLGetMinAVal(NL,mnaSpeed,&N1,(void **)&MinSpeed) == FAILURE) &&
          (MUNLGetMinAVal(NL,mnaProcSpeed,&N2,(void **)&MinPSpeed) == FAILURE))
        {
        return(FAILURE);
        }

      if ((N1 != NULL) && (N1->NodeType != NULL))
        strcpy((char *)Val,N1->NodeType);      
      else if ((N2 != NULL) && (N2->NodeType != NULL))
        strcpy((char *)Val,N2->NodeType);
      else
        strcpy((char *)Val,MDEF_NODETYPE);

      if (Val[0] == '\0')
        strcpy((char *)Val,MDEF_NODETYPE);
      }    /* END BLOCK */

      break;
 
    default:

      /* NO-OP */
 
      break;
    }  /* END switch (NAttr) */
 
  return(SUCCESS);
  }  /* END MUNLGetMinAVal() */




/**
 *
 *
 * @param NL (I)
 * @param NAttr (I)
 * @param NPtr (O) [optional]
 * @param Val (O)
 */

int MUNLGetMaxAVal(
 
  mnl_t             *NL,     /* I */
  enum MNodeAttrEnum NAttr,  /* I */
  mnode_t          **NPtr,   /* O (optional) */
  void             **Val)    /* O */
 
  {
  mnode_t *N = NULL;
 
  int nindex;

  const char *FName = "MUNLGetMaxAVal";
 
  MDB(6,fSTRUCT) MLog("%s(NL,NAttr,N,Val)\n",
    FName);

  if (Val != NULL)
    {
    /* set defaults */

    switch (NAttr)
      {
      case mnaChargeRate:
      case mnaSpeed:

        *(double *)Val = 1.0;

        break;

      case mnaProcSpeed:

        *(int *)Val = 1;

        break;

      case mnaNodeType:
      default:

        strcpy((char *)Val,MDEF_NODETYPE);

        break;
      }  /* END switch (NAttr) */
    }    /* END if (Val != NULL) */
 
  if ((NL == NULL) || (MNLIsEmpty(NL)) || (Val == NULL))
    {
    return(FAILURE);
    }
 
  switch (NAttr)
    {
    case mnaChargeRate:

      {
      double MaxCharge = -1.0;

      for (nindex = 0;MNLGetNodeAtIndex(NL,nindex,&N) == SUCCESS;nindex++)
        {
        if (N->ChargeRate > MaxCharge)
          {
          MaxCharge = N->ChargeRate;

          if (NPtr != NULL)
            *NPtr = N;
          }
        }    /* END for (nindex) */

      if ((MaxCharge < 0.0) || (MaxCharge > 99999999))
        {
        if (NPtr != NULL)
          MNLGetNodeAtIndex(NL,0,NPtr);

        *(double *)Val = 1.0;

        return(FAILURE);
        }

      *(double *)Val = MaxCharge;
      }  /* END BLOCK */

      break;

    case mnaSpeed:
 
      {
      double MaxSpeed = -1.0;
 
      if (!bmisset(&MPar[0].Flags,mpfUseMachineSpeed))
        {
        *(double *)Val = 1.0;

        if (NPtr != NULL)
          *NPtr = N;
 
        return(SUCCESS);
        }
 
      for (nindex = 0;MNLGetNodeAtIndex(NL,nindex,&N) == SUCCESS;nindex++) 
        {
        if (N->Speed > MaxSpeed)
          {
          MaxSpeed = N->Speed;
 
          if (NPtr != NULL)
            *NPtr = N;
          }
        }    /* END for (nindex) */
 
      if ((MaxSpeed <= 0.0) || (MaxSpeed > 99999999))
        {
        if (NPtr != NULL)
          MNLGetNodeAtIndex(NL,0,NPtr);
 
        *(double *)Val = 1.0;
 
        return(FAILURE);
        }
 
      *(double *)Val = MaxSpeed;
      }  /* END BLOCK */
 
      break;
 
    case mnaProcSpeed:
 
      {
      int MaxSpeed = -1;
 
      for (nindex = 0;MNLGetNodeAtIndex(NL,nindex,&N) == SUCCESS;nindex++) 
        {
        if (N->ProcSpeed > MaxSpeed)
          {
          MaxSpeed = N->ProcSpeed;
 
          if (NPtr != NULL)
            *NPtr = N;
          }
        }    /* END for (nindex) */
 
      if ((MaxSpeed <= 0) || (MaxSpeed > 99999999))
        {
        if (NPtr != NULL)
          MNLGetNodeAtIndex(NL,0,NPtr);
 
        *(int *)Val = 1;
 
        return(FAILURE);
        }
 
      *(int *)Val = MaxSpeed;
      }  /* END BLOCK */
 
      break;
 
    case mnaNodeType:
 
      {
      int MaxPSpeed;
      int MaxSpeed;
 
      if ((MUNLGetMaxAVal(NL,mnaSpeed,&N,(void **)&MaxSpeed) == FAILURE) &&
          (MUNLGetMaxAVal(NL,mnaProcSpeed,&N,(void **)&MaxPSpeed) == FAILURE)) 
        {
        strcpy((char *)Val,MDEF_NODETYPE);
 
        return(FAILURE);
        }

      if ((N != NULL) && (N->NodeType != NULL))
        strcpy((char *)Val,N->NodeType);
      else
        strcpy((char *)Val,MDEF_NODETYPE);
      }  /* END BLOCK */
 
      break;
 
    default:

      /* NO-OP */
 
      break;
    }  /* END switch (NAttr) */
 
  return(SUCCESS);
  }  /* END MUNLGetMaxAVal() */





/**
 * Report average attribute value across all nodes within nodelist.
 *
 * NOTE:  Handles node attributes nodetype, speed, procspeed, and chargerate.
 *
 * NOTE:  If not specified, average value of '1' will be reported.
 *
 * @param NL (I)
 * @param NAttr (I)
 * @param NPtr (O) [optional]
 * @param Val (O)
 */

int MUNLGetAvgAVal(

  mnl_t             *NL,     /* I */
  enum MNodeAttrEnum NAttr,  /* I */
  mnode_t          **NPtr,   /* O (optional) */
  void             **Val)    /* O */

  {
  mnode_t *N;

  int nindex;

  const char *FName = "MUNLGetAvgAVal";

  MDB(6,fSTRUCT) MLog("%s(NL,NAttr,N,Val)\n",
    FName);

  if (Val != NULL)
    {
    /* set defaults */

    switch (NAttr)
      {
      case mnaSpeed:
      case mnaChargeRate:

        *(double *)Val = 1.0;

        break;

      case mnaProcSpeed:

        *(int *)Val = 1;

        break;

      case mnaNodeType:
      default:

        strcpy((char *)Val,MDEF_NODETYPE);

        break;
      }  /* END switch (NAttr) */
    }    /* END if (Val != NULL) */

  if ((NL == NULL) || (MNLIsEmpty(NL)) || (Val == NULL))
    {
    return(FAILURE);
    }

  if (NPtr != NULL)
    MNLGetNodeAtIndex(NL,0,NPtr);

  switch (NAttr)
    {
    case mnaChargeRate:

      {
      double TotCharge = 0.0;
     
      for (nindex = 0;MNLGetNodeAtIndex(NL,nindex,&N) == SUCCESS;nindex++) 
        {
        TotCharge += N->ChargeRate;
        }    /* END for (nindex) */

      if (TotCharge > 0)
        *(double *)Val = TotCharge / nindex;
      }  /* END BLOCK */

      break;

    case mnaSpeed:

      {
      double TotSpeed = 0.0;

      if (!bmisset(&MPar[0].Flags,mpfUseMachineSpeed))
        {
        *(double *)Val = 1.0;

        return(SUCCESS);
        }

      for (nindex = 0;MNLGetNodeAtIndex(NL,nindex,&N) == SUCCESS;nindex++) 
        {
        TotSpeed += N->Speed;
        }    /* END for (nindex) */

      if (TotSpeed > 0)
        *(double *)Val = TotSpeed / nindex;
      }  /* END BLOCK */

      break;

    case mnaProcSpeed:

      {
      int TotSpeed = 0;

      for (nindex = 0;MNLGetNodeAtIndex(NL,nindex,&N) == SUCCESS;nindex++) 
        {
        TotSpeed = N->ProcSpeed;
        }    /* END for (nindex) */

      if (TotSpeed > 0)
        *(int *)Val = TotSpeed / nindex;
      }  /* END BLOCK */

      break;

    case mnaNodeType:

      {
      int MaxPSpeed;
      int MaxSpeed;

      if ((MUNLGetAvgAVal(NL,mnaSpeed,&N,(void **)&MaxSpeed) == FAILURE) &&
          (MUNLGetAvgAVal(NL,mnaProcSpeed,&N,(void **)&MaxPSpeed) == FAILURE))
        {
        strcpy((char *)Val,MDEF_NODETYPE);

        return(FAILURE);
        }

      if ((N != NULL) && (N->NodeType != NULL))
        strcpy((char *)Val,N->NodeType);
      else
        strcpy((char *)Val,MDEF_NODETYPE);
      }  /* END BLOCK */

      break;

    default:

      /* NO-OP */

      break;
    }  /* END switch (NAttr) */

  return(SUCCESS);
  }  /* END MUNLGetAvgAVal() */





/**
 * Report total charge rate for specified nodelist.
 *
 * Incorporate both node charge rate and GRes costing info.
 *
 * NOTE:  assumes each node is fully allocated/dedicated.
 *
 * @param NL (I)
 * @param ChargeRateP (O)
 */

int MUNLGetTotalChargeRate(

  mnl_t     *NL,           /* I */
  double    *ChargeRateP)  /* O */

  {
  mnode_t *N;

  int nindex;
  int gindex;

  const char *FName = "MUNLGetTotalChargeRate";

  MDB(6,fSTRUCT) MLog("%s(NL,ChargeRateP)\n",
    FName);

  if ((NL == NULL) || (MNLIsEmpty(NL)) || (ChargeRateP == NULL))
    {
    if (ChargeRateP != NULL)
      *ChargeRateP = 1.0;

    return(FAILURE);
    }

  *ChargeRateP = 0.0;

  for (nindex = 0;MNLGetNodeAtIndex(NL,nindex,&N) == SUCCESS;nindex++)
    {
    *ChargeRateP += (N->ChargeRate > 0.0) ? N->ChargeRate : 1.0;

    if (!MSNLIsEmpty(&N->CRes.GenericRes))
      {
      for (gindex = 1;gindex < MSched.M[mxoxGRes];gindex++)
        {
        if (MGRes.Name[gindex][0] == '\0')
          break;

        if (MSNLGetIndexCount(&N->CRes.GenericRes,gindex) == 0)
          continue;

        if (MGRes.GRes[gindex]->ChargeRate == 0.0)
          continue;

        *ChargeRateP += MSNLGetIndexCount(&N->CRes.GenericRes,gindex) * MGRes.GRes[gindex]->ChargeRate;
        }  /* END for (gindex) */
      }    /* END if (!MSNLIsEmpty(&N->CRes.GenericRes)) */
    }      /* END for (nindex) */

  return(SUCCESS);
  }  /* END MUNLGetTotalChargeRate() */
/* END MNodeUNL.c */
