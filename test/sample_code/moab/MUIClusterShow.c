/* HEADER */

      
/**
 * @file MUIClusterShow.c
 *
 * Contains: MUI Cluster Show function
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/**
 *
 *
 * @param S (I)
 * @param CFlags (I)
 * @param Auth (I)
 */

int MUIShowState(

  msocket_t *S,      /* I */
  mbitmap_t *CFlags, /* I (not used) */
  char      *Auth)   /* I */

  {
  mhashiter_t HTI;

  int   rindex;
  int   sindex;

  int   nindex;

  mbitmap_t Status;  /* bitmap of enum mnf* */

  mnode_t *N;
  mjob_t  *J;

  mrack_t *R;

  int      MaxSlotPerRack;
  int      MaxSpecRack;

  mbool_t  AutoAssign = TRUE;

  int      ARIndex;
  int      ASIndex;

  int      UANC;

  mhost_t *Hosts;

  mbitmap_t DFlagBM;

  int   *RIndex = NULL;
  int   *SIndex = NULL;

  const char *FName = "MUIClusterShow";

  MDB(2,fUI) MLog("%s(RBuffer,SBuffer,%s,SBufSize)\n",
    FName,
    Auth);

  /* FORMAT */

  /* SYSTEM TIME=<PRESENTTIME> MAXSLOT=<MAXSLOT>
   * RACK <NAME> <RINDEX> <ATTR> ...
   * NODE <NAME> <STATE> <RINDEX> <SINDEX> <SCOUNT>
   * ...
   * JOB <NAME> <STATE> <DURATION> :<NODELIST>:
   */

  switch (S->WireProtocol)
    {
    case mwpXML:
    case mwpS32:

      {
      char FlagLine[MMAX_LINE];

      mxml_t *RE = NULL;

      /* FORMAT:  <Request action="X" object="Y" Flags="Z"></Request> */

      char *ptr;

      if (S->RDE != NULL)
        {
        RE = S->RDE;
        }
      else if ((S->RPtr != NULL) &&
              ((ptr = strchr(S->RPtr,'<')) != NULL) &&
               (MXMLFromString(&RE,ptr,NULL,NULL) == FAILURE))
        {
        MDB(3,fUI) MLog("WARNING:  corrupt command '%100.100s' received\n",
          S->RBuffer);

        MUISAddData(S,"ERROR:    corrupt command received\n");

        return(FAILURE);
        }

      if (MXMLGetAttr(RE,MSAN[msanFlags],NULL,FlagLine,sizeof(FlagLine)) == SUCCESS)
        {
        if (strcasestr(FlagLine,"noauto"))
          {
          AutoAssign = FALSE;

          bmset(&DFlagBM,mcmComplete);
          }

        if (strcasestr(FlagLine,"xml"))
          {
          if ((MXMLCreateE(&S->SDE,(char *)MSON[msonData]) == SUCCESS) &&
              (MClusterShowUsage(
                mwpXML,
                CFlags,
                NULL,
                (void **)&S->SDE,
                0) == SUCCESS))
            {
            return(SUCCESS);
            }
          else
            {
            MUISAddData(S,"ERROR:    cannot create response\n");

            return(FAILURE);
            }
          }
        }
      }  /* END BLOCK (case mwpXML/mwpS3) */

      break;

    default:

      /* not supported */

      MUISAddData(S,"ERROR:    corrupt command received\n");

      return(FAILURE);

      /*NOTREACHED*/

      break;
    }  /* END switch (S->WireProtocol) */

  mstring_t SBuffer(MMAX_LINE);

  /* determine max slot index */

  MaxSlotPerRack = 1;

  MaxSpecRack    = 0;

  UANC           = 0;

  for (rindex = 1;rindex < MMAX_RACK;rindex++)
    {
    for (sindex = 1;sindex <= MMAX_RACKSIZE;sindex++)
      {
      Hosts = &MSys[rindex][sindex];

      if (Hosts->RMName[0] != '\0')
        {
        MaxSlotPerRack = MAX(
          MaxSlotPerRack,
          sindex + MAX(1,Hosts->SlotsUsed) - 1);

        MaxSpecRack = rindex;
        }
      }
    }    /* END for (rindex) */

  /* locate unassigned nodes info via node table */

  for (nindex = 0;nindex < MSched.M[mxoNode];nindex++)
    {
    N = MNode[nindex];

    if ((N == NULL) || (N->Name[0] == '\0'))
      break;

    if (N->Name[0] == '\1')
      continue;

    if ((N->SlotIndex <= 0) || (N->RackIndex <= 0))
      {
      UANC ++;
      }
    }  /* END for (nindex) */

  if ((MaxSlotPerRack <= 1) || (UANC > 0))
    {
    if (AutoAssign == TRUE)
      MaxSlotPerRack = MSched.DefaultRackSize;
    }

  /* create message header */

  MStringAppendF(&SBuffer,"%s %ld %d\n",
    MXO[mxoSys],
    MSched.Time,
    MaxSlotPerRack);

  RIndex = (int *)MUCalloc(1,sizeof(int) * MSched.M[mxoNode]);
  SIndex = (int *)MUCalloc(1,sizeof(int) * MSched.M[mxoNode]);

  /* format rack information */

  for (rindex = 1;rindex < MMAX_RACK;rindex++)
    {
    char tmpName[MMAX_NAME];

    R = &MRack[rindex];

    MDB(5,fUI) MLog("INFO:     displaying status for rack %s\n",
      R->Name);

    if (R->NodeCount > 0)
      {
      strcpy(tmpName,R->Name);
      }
    else if ((AutoAssign == TRUE) && 
             (UANC - (rindex - MaxSpecRack - 1) * MaxSlotPerRack > 0))
      {
      sprintf(tmpName,"%02d",
        rindex);
      }
    else
      {
      continue;
      }

    MStringAppendF(&SBuffer,"%s %s %d %s\n",
      MXO[mxoRack],
      tmpName,
      rindex,
      NONE);
    }

  ARIndex = MaxSpecRack + 1;
  ASIndex = 1;

  for (nindex = 0;nindex < MSched.M[mxoNode];nindex++)
    {
    N = MNode[nindex];

    if ((N == NULL) || (N->Name[0] == '\0'))
      break;

    if (N->Name[0] == '\1')
      continue;

    if (N == MSched.GN)
      continue;

    /* display failure information */

    if ((N->CRes.Disk > 0) && (N->ARes.Disk <= 0))
      bmset(&Status,mnfLocalDisk);

    if ((N->CRes.Mem > 0) && (N->ARes.Mem <= 0))
      bmset(&Status,mnfMemory);

    if ((N->CRes.Swap > 0) && (N->ARes.Swap < MDEF_MINSWAP))
      bmset(&Status,mnfSwap);

    rindex = N->RackIndex;
    sindex = N->SlotIndex;

    if (((rindex <= 0) || (sindex < 1)) && (AutoAssign == TRUE))
      {
      rindex = ARIndex;
      sindex = ASIndex;

      ASIndex++;

      if (ASIndex > MSched.DefaultRackSize)
        {
        ASIndex = 1;
        ARIndex++;

        ARIndex = MIN(ARIndex,MMAX_RACK);
        }
      }

    RIndex[N->Index] = rindex;
    SIndex[N->Index] = sindex;

    /* FORMAT:  NAME STATE RACK SLOT SIZE ERROR */

    MStringAppendF(&SBuffer,"%s %s %s %d %d %d %s\n",
      MXO[mxoNode],
      N->Name,
      MNodeState[N->State],
      rindex,
      sindex,
      MAX(1,N->SlotsUsed),
      NONE);
    }  /* END for (nindex) */

  MDB(5,fUI) MLog("INFO:     node state data collected\n");

  /* collect job status */

  MUHTIterInit(&HTI);

  while (MUHTIterate(&MAQ,NULL,(void **)&J,NULL,&HTI) == SUCCESS)
    {
    if (!MJobPtrIsValid(J))
      continue;

    MStringAppendF(&SBuffer,"%s %s %d %d %ld %s %s %ld %s ",
      MXO[mxoJob],
      J->Name,
      J->TotalProcCount,
      J->Alloc.NC,
      J->WCLimit,
      J->Credential.U->Name,
      J->Credential.G->Name,
      J->StartTime,
      MJobState[J->State]);

    MDB(3,fUI) MLog("INFO:     adding job '%s' info (%d tasks) to buffer\n",
      J->Name,
      J->Alloc.TC);

    for (nindex = 0;MNLGetNodeAtIndex(&J->NodeList,nindex,&N) == SUCCESS;nindex++)
      {
      MStringAppendF(&SBuffer,"%d:%d;",
        (RIndex[N->Index] > 0) ? RIndex[N->Index] : N->RackIndex,
        (SIndex[N->Index] > 0) ? SIndex[N->Index] : N->SlotIndex);

      MDB(4,fUI) MLog("INFO:     adding node '%s' of job '%s' to buffer\n",
        N->Name,
        J->Name);
      }  /* END for (nindex) */

    MStringAppendF(&SBuffer,"\n");
    }  /* END while (MUHTIterate(&MAQ)) */

  MUISAddData(S,SBuffer.c_str());

  MUFree((char **)&RIndex);
  MUFree((char **)&SIndex);

  return(SUCCESS);
  }  /* END MUIClusterShow() */

/* END MUIClusterShow.c */
