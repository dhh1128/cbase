
/* HEADER */

      
/**
 * @file MJobCalcStartPriority.c
 *
 * Contains:
 */


#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


#define MCONST_PRIOCOMPNAME    "pc"
#define MCONST_PRIOSUBCOMPNAME "psc"

#ifndef ABS
#define ABS(a) ((a >= 0) ? (a) : 0-(a))
#endif /* ABS */


/**
 * Calculate job start priority.
 *
 * @see MJobGetRunPriority()
 * @see MJobGetBackfillPriority()
 *
 * @param J (I) [optional]
 * @param P (I)
 * @param Priority (O)
 * @param Mode (I) [mpdJob, mpdFooter, or mpdHeader]
 * @param RE (O)
 * @param String (O)
 * @param DFormat (O)
 * @param ShowJobName (I)
 */

int MJobCalcStartPriority(

  mjob_t                *J,
  const mpar_t          *P,
  double                *Priority,
  enum MPrioDisplayEnum  Mode,
  mxml_t               **RE,
  mstring_t             *String,
  enum MFormatModeEnum   DFormat,
  mbool_t                ShowJobName)

  {
  double        Prio;
  double        APrio;

  double        XFactor;

  double        FSTargetUsage;

  enum MFSTargetEnum FSTargetMode;

  char          CHeader[mpcLAST][MMAX_NAME];    
  char          CWLine[mpcLAST][MMAX_NAME];    
  char          CFooter[mpcLAST][MMAX_NAME];  
  char          CLine[mpcLAST][MMAX_NAME];

  char          tmpHeader[MMAX_NAME];
  char          tmpCWLine[MMAX_NAME];
  char          tmpLine[MMAX_NAME];

  double        CFactor[mpcLAST];  /* component factors */
  double        SFactor[mpsLAST];  /* sub-component factors */

  long          CWeight[mpcLAST];  /* component weights */
  long          SWeight[mpsLAST];  /* sub-component weights */

  static double TotalCFactor[mpcLAST];
  static double TotalSFactor[mpsLAST];

  double        CP[mpcLAST];       /* component percentages */
  double        SP[mpsLAST];       /* sub-component percentages */

  static double TotalPriority;

  static int    SDisplay[mpsLAST];

  const mfsc_t       *F;
  mfsc_t       *GF;

  mpar_t       *GP;

  mreq_t       *RQ;

  int           rqindex;
  int           qindex;
  int           index;

  int           cindex;
  int           sindex;

  int           Target;
  enum MFSTargetEnum TargetMod;

  int           PC;

  long          CCap[mpcLAST];
  long          SCap[mpsLAST];

  long          EffQDate;

  long          tmpL;
  char          tmpS[MMAX_NAME];

  unsigned long MinWCLimit = 0;

  mxml_t      *E = NULL;  /* set to avoid compiler warning */
  mxml_t      *JE;
  mxml_t      *CE;
  mxml_t      *SE;

  /* XML Format:  <data><job name="header"><prioc name="service" value="1000"><prios name="queuetime" value="1"></prios></prioc></job><job name="job13725" priority="93271"><prioc name="service"><priosub name="queuetime" value="100" metric="100 seconds"></priosub></prioc><prioc name="cred" value="10000"><prios name="user" value="100"></prios></prioc></job></data> */

  const char *FName = "MJobCalcStartPriority";

  MDB(6,fSCHED) MLog("%s(%s,%s,%s,%u,RE,String,%s)\n",
    FName,
    (J != NULL) ? J->Name : "NULL",
    (P != NULL) ? P->Name : "NULL",
    (Priority != NULL) ? "Priority" : "NULL",
    Mode,
    MFormatMode[DFormat]);

  if (Priority != NULL)
    *Priority = 0.0;

  /* NOTE: routine used to diagnose priority */

  GF = &MPar[0].FSC;

  F  = (P != NULL) ? &P->FSC : GF;

/*
  F = GF;
*/

  GP = &MPar[0];

  /* NOTE:  do not initialize Buffer */

  if (DFormat == mfmXML)
    {
    if ((RE == NULL) && (Mode != mpdJob))
      {
      return(FAILURE);
      }

    if (RE != NULL)
      {
      if (*RE == NULL)
        {
        MXMLCreateE(RE,(char *)MSON[msonData]);
        }

      E = *RE;
      }
    }
  else
    {
    if ((Mode != mpdJob) && (String == NULL))
      {
      return(FAILURE);
      }

    /* DON'T initialize String */
    }

  if ((Mode == mpdJob) && (J == NULL))
    {
    /* can't calculcate priority on NULL job */

    return(FAILURE);
    }
 
  for (index = 1;index < mpcLAST;index++)
    {
    CWeight[index] = (F->PCW[index] != MCONST_PRIO_NOTSET) ? 
      F->PCW[index] : GF->PCW[index]; 

    MDB(8,fSTRUCT) MLog("INFO:     CWeight[%d] = %ld (F->PCW[]=%ld : GF->PCW[]=%ld)\n",
      index,
      CWeight[index],
      F->PCW[index],
      GF->PCW[index]);

    CCap[index]    = (F->PCC[index] > 0) ? F->PCC[index] : GF->PCC[index];      

    MDB(8,fSTRUCT) MLog("INFO:     CCap[%d]    = %ld (F->PCC[]=%ld : GF->PCC[]=%ld)\n",
      index,
      CCap[index],
      F->PCC[index],
      GF->PCC[index]);

    CFactor[index] = 0.0;
    CP[index]      = 0.0;
    }  /* END for (index) */
 
  for (index = 0;index < mpsLAST;index++)
    {
    SWeight[index] = (F->PSW[index] != MCONST_PRIO_NOTSET) ? 
      F->PSW[index] : GF->PSW[index];     

    MDB(8,fSTRUCT) MLog("INFO:     SWeight[%d] = %ld (F->PSW[]=%ld : GF->PSW[]=%ld)\n",
      index,
      SWeight[index],
      F->PSW[index],
      GF->PSW[index]);

    SCap[index]    = (F->PSC[index] > 0) ? F->PSC[index] : GF->PSC[index];      

    MDB(8,fSTRUCT) MLog("INFO:     SCap[%d]    = %ld (F->PSC[]=%ld : GF->PSC[]=%ld)\n",
      index,
      SCap[index],
      F->PSC[index],
      GF->PSC[index]);

    SFactor[index] = 0.0;
    SP[index]      = 0.0;
    }  /* END for (index) */

  for (index = mpsCU;index <= mpsCC;index++)
    {
    if (MPar[0].FSC.PSCIsActive[index] == FALSE)
      SWeight[index] = 0;
    }

  for (index = mpsFU;index <= mpsFC;index++)
    {
    if (MPar[0].FSC.PSCIsActive[index] == FALSE)
      SWeight[index] = 0;
    }

  /* apply credential based weight adjustments */

  if ((J != NULL) && (J->Credential.Q != NULL))
    {
    SWeight[mpsSQT] += J->Credential.Q->QTSWeight;
    SWeight[mpsSXF] += J->Credential.Q->XFSWeight;
    }

  MinWCLimit = (F->XFMinWCLimit != -1) ? F->XFMinWCLimit : GF->XFMinWCLimit;

  if (J != NULL)
    {
    if (J->Credential.A != NULL)
      {
      CWeight[mpcFS] += J->Credential.A->FSCWeight;
      }

    if (J->Credential.G != NULL)
      {
      CWeight[mpcFS] += J->Credential.G->FSCWeight;
      }

    if (J->Credential.Q != NULL)
      {
      CWeight[mpcFS] += J->Credential.Q->FSCWeight;
      }

    if (J->Credential.G != NULL)
      {
      if (J->Credential.G->ClassSWeight > 0)
        SWeight[mpsCC] = J->Credential.G->ClassSWeight;
      }
    }    /* END if (J != NULL) */

  if (Mode == mpdHeader)
    {
    /* initialize summary values/create header */

    memset(TotalCFactor,0,sizeof(TotalCFactor));
    memset(TotalSFactor,0,sizeof(TotalSFactor));      
    memset(SDisplay,FALSE,sizeof(SDisplay));        

    memset(CHeader,0,sizeof(CHeader));      
    memset(CWLine,0,sizeof(CWLine));  

    TotalPriority = 0.0;

    if (DFormat == mfmXML)
      {
      JE = NULL;

      MXMLCreateE(&JE,"job");
      MXMLSetAttr(JE,"name",(void **)"template",mdfString);

      MXMLAddE(E,JE);

      for (cindex = mpcServ;cindex <= mpcUsage;cindex++)
        {
        CE = NULL;

        MXMLCreateE(&CE,MCONST_PRIOCOMPNAME);
        MXMLSetAttr(CE,"name",(void **)MPrioCName[cindex],mdfString);
        MXMLSetAttr(CE,"value",(void **)&CWeight[cindex],mdfLong);

        MXMLAddE(JE,CE);

        for (sindex = MPSCStart[cindex];sindex <= MPSCEnd[cindex];sindex++)
          {
          SE = NULL;
  
          MXMLCreateE(&SE,MCONST_PRIOSUBCOMPNAME);
          MXMLSetAttr(SE,"name",(void **)MPrioSCName[sindex],mdfString);
          MXMLSetAttr(SE,"value",(void **)&SWeight[sindex],mdfLong);

          MXMLAddE(CE,SE);
          }  /* END for (sindex) */
        }    /* END for (cindex) */
      }
    else 
      {
      if (CWeight[mpcCred] != 0)
        {
        tmpHeader[0] = '\0';
        tmpCWLine[0] = '\0';     

        for (index = mpsCU;index <= mpsCC;index++)
          {
          if (SWeight[index] == 0)
            continue;

          if (tmpHeader[0] != '\0')
            {
            strcat(tmpHeader,":");
            strcat(tmpCWLine,":");
            }

          switch (index)
            {
            case mpsCU: strcat(tmpHeader," User"); break;
            case mpsCG: strcat(tmpHeader,"Group"); break;        
            case mpsCA: strcat(tmpHeader,"Accnt"); break;        
            case mpsCC: strcat(tmpHeader,"Class"); break;        
            case mpsCQ: strcat(tmpHeader,"  QOS"); break;        
            }

          sprintf(tmpCWLine,"%s%5ld",
            tmpCWLine,
            (long)SWeight[index]);
          }  /* END for (index) */

        if (tmpHeader[0] != '\0')
          {
          sprintf(CHeader[mpcCred],"  Cred(%s)",
            tmpHeader);

          sprintf(CWLine[mpcCred]," %5ld(%s)",
            CWeight[mpcCred],
            tmpCWLine);
          }
        }    /* END if (CWeight[mpcCred] != 0) */

    if (CWeight[mpcFS] != 0)
      {
      tmpHeader[0] = '\0';
      tmpCWLine[0] = '\0';
 
      for (index = MPSCStart[mpcFS];index <= MPSCEnd[mpcFS];index++)
        {
        if (SWeight[index] == 0)
          continue;

        if (tmpHeader[0] != '\0')
          {
          strcat(tmpHeader,":");
          strcat(tmpCWLine,":");
          }

        switch (index)
          {
          case mpsFU:    strcat(tmpHeader," User"); break;
          case mpsFG:    strcat(tmpHeader,"Group"); break;
          case mpsFA:    strcat(tmpHeader,"Accnt"); break;
          case mpsFC:    strcat(tmpHeader,"Class"); break;
          case mpsFQ:    strcat(tmpHeader,"  QOS"); break;
          case mpsGFU:   strcat(tmpHeader,"GUser"); break;
          case mpsGFG:   strcat(tmpHeader," GGrp"); break;
          case mpsGFA:   strcat(tmpHeader,"GAcct"); break;
          case mpsFUWCA: strcat(tmpHeader,"  WCA"); break;
          case mpsFJPU:  strcat(tmpHeader,"  JPU"); break;
          case mpsFJRPU: strcat(tmpHeader," JRPU"); break;
          case mpsFPPU:  strcat(tmpHeader,"  PPU"); break;
          case mpsFPSPU: strcat(tmpHeader," PSPU"); break;
          }  /* END switch (index) */
 
        sprintf(tmpCWLine,"%s%5ld",
          tmpCWLine,
          SWeight[index]);
        }  /* END for (index) */
 
      if (tmpHeader[0] != '\0')
        {
        sprintf(CHeader[mpcFS],"    FS(%s)",
          tmpHeader);
 
        sprintf(CWLine[mpcFS]," %5ld(%s)",
          CWeight[mpcFS],
          tmpCWLine);
        }
      }    /* END if (CWeight[mpcFS] != 0) */

    if (CWeight[mpcAttr] != 0)
      {
      tmpHeader[0] = '\0';
      tmpCWLine[0] = '\0';

      for (index = MPSCStart[mpcAttr];index <= MPSCEnd[mpcAttr];index++)
        {
        /* handle job attr based priority weights */

        switch (index)
          {
          case mpsAAttr:
          case mpsAGRes:
          case mpsAJobID:
          case mpsAJobName:
          case mpsAState:
        
            if (SWeight[index] != 0)
              {
              SDisplay[index] = TRUE;
              }

            break;

          default:

            /* NO-OP */

            break;
          }  /* END switch (index) */

        if ((SWeight[index] == 0) && (SDisplay[index] == FALSE))
          continue;

        if (tmpHeader[0] != '\0')
          {
          strcat(tmpHeader,":");
          strcat(tmpCWLine,":");
          }

        switch (index)
          {
          case mpsAAttr:    strcat(tmpHeader," Attr"); break;
          case mpsAGRes:    strcat(tmpHeader," GRes"); break;
          case mpsAJobID:   strcat(tmpHeader,"  JID"); break;
          case mpsAJobName: strcat(tmpHeader," Name"); break;
          case mpsAState:   strcat(tmpHeader,"State"); break;
          }

        sprintf(tmpCWLine,"%s%5ld",
          tmpCWLine,
          SWeight[index]);
        }  /* END for (index) */

      if (tmpHeader[0] != '\0')
        {
        sprintf(CHeader[mpcAttr],"  Attr(%s)",
          tmpHeader);

        sprintf(CWLine[mpcAttr]," %5ld(%s)",
          CWeight[mpcAttr],
          tmpCWLine);
        }    /* END for (index) */
      }      /* END if (CWeight[mpcAttr] != 0) */

    if (CWeight[mpcServ] != 0)
      {
      tmpHeader[0] = '\0';
      tmpCWLine[0] = '\0';

      for (index = MPSCStart[mpcServ];index <= MPSCEnd[mpcServ];index++)
        {
        /* handle QOS based service priority weights */

        switch (index)
          {
          case mpsSQT:

            if (SWeight[mpsSQT] != 0)
              {
              SDisplay[mpsSQT] = TRUE;
              }
            else
              {
              for (qindex = 1;qindex < MMAX_QOS;qindex++)
                {
                if (MQOS[qindex].QTSWeight > 0)
                  {
                  SDisplay[mpsSQT] = TRUE;  
 
                  break;
                  }
                }
              }

            break;

          case mpsSXF:
 
            if (SWeight[mpsSXF] != 0)
              {
              SDisplay[mpsSXF] = TRUE;
              }
            else
              {
              for (qindex = 1;qindex < MMAX_QOS;qindex++)
                {
                if (MQOS[qindex].XFSWeight > 0)
                  {
                  SDisplay[mpsSXF] = TRUE;
 
                  break;
                  }
                }    /* END for (qindex) */
              }
 
            break;
          }  /* END switch (index) */

        if ((SWeight[index] == 0) && (SDisplay[index] == FALSE))
          continue;
 
        if (tmpHeader[0] != '\0')
          {
          strcat(tmpHeader,":");
          strcat(tmpCWLine,":");
          }
 
        switch (index)
          {
          case mpsSQT:         strcat(tmpHeader,"QTime"); break;
          case mpsSXF:         strcat(tmpHeader,"XFctr"); break;
          case mpsSDeadline:   strcat(tmpHeader,"DLine"); break;
          case mpsSSPV:        strcat(tmpHeader,"SPVio"); break;
          case mpsSUPrio:      strcat(tmpHeader,"UPrio"); break;
          case mpsSBP:         strcat(tmpHeader,"Bypas"); break;
          case mpsSStartCount: strcat(tmpHeader,"Start"); break;
          }
 
        sprintf(tmpCWLine,"%s%5ld",
          tmpCWLine,
          SWeight[index]);
        }  /* END for (index) */
 
      if (tmpHeader[0] != '\0')
        {
        sprintf(CHeader[mpcServ],"  Serv(%s)",
          tmpHeader);
 
        sprintf(CWLine[mpcServ]," %5ld(%s)",
          CWeight[mpcServ],
          tmpCWLine);
        }
      }    /* END if (CWeight[mpcServ] != 0) */

    if (CWeight[mpcTarg] != 0)
      {
      tmpHeader[0] = '\0';
      tmpCWLine[0] = '\0';
 
      for (index = mpsTQT;index <= mpsTXF;index++)
        {
        if (SWeight[index] == 0)
          continue;
 
        if (tmpHeader[0] != '\0')
          {
          strcat(tmpHeader,":");
          strcat(tmpCWLine,":");
          }
 
        switch (index)
          {
          case mpsTQT: strcat(tmpHeader,"QTime"); break;
          case mpsTXF: strcat(tmpHeader,"XFctr"); break;
          }
 
        sprintf(tmpCWLine,"%s%5ld",
          tmpCWLine,
          SWeight[index]);
        }  /* END for (index) */
 
      if (tmpHeader[0] != '\0')
        {
        sprintf(CHeader[mpcTarg],"  Targ(%s)",
          tmpHeader);
 
        sprintf(CWLine[mpcTarg]," %5ld(%s)",
          CWeight[mpcTarg],
          tmpCWLine);
        }
      }    /* END if (CWeight[mpcTarg] != 0) */

    if (CWeight[mpcRes] != 0)
      {
      tmpHeader[0] = '\0';
      tmpCWLine[0] = '\0';
 
      for (index = mpsRNode;index <= mpsRWallTime;index++)
        {
        if (SWeight[index] == 0)
          continue;
 
        if (tmpHeader[0] != '\0')
          {
          strcat(tmpHeader,":");
          strcat(tmpCWLine,":");
          }
 
        switch (index)
          {
          case mpsRNode:     strcat(tmpHeader," Node"); break;
          case mpsRProc:     strcat(tmpHeader," Proc"); break;
          case mpsRMem:      strcat(tmpHeader,"  Mem"); break;
          case mpsRSwap:     strcat(tmpHeader," Swap"); break;
          case mpsRDisk:     strcat(tmpHeader," Disk"); break;
          case mpsRPS:       strcat(tmpHeader,"   PS"); break;  
          case mpsRPE:       strcat(tmpHeader,"   PE"); break;  
          case mpsRWallTime: strcat(tmpHeader,"WTime"); break;  
          }
 
        sprintf(tmpCWLine,"%s%5ld",
          tmpCWLine,
          SWeight[index]);
        }  /* END for (index) */
 
      if (tmpHeader[0] != '\0')
        {
        sprintf(CHeader[mpcRes],"   Res(%s)",
          tmpHeader);
 
        sprintf(CWLine[mpcRes]," %5ld(%s)",
          CWeight[mpcRes],
          tmpCWLine);
        }
      }    /* END if (CWeight[mpcRes] != 0) */

    if (CWeight[mpcUsage] != 0)
      {
      tmpHeader[0] = '\0';
      tmpCWLine[0] = '\0';
 
      for (index = mpsUCons;index <= mpsUExeTime;index++)
        {
        if (SWeight[index] == 0)
          continue;
 
        if (tmpHeader[0] != '\0')
          {
          strcat(tmpHeader,":");
          strcat(tmpCWLine,":");
          }
 
        switch (index)
          {
          case mpsUCons:     strcat(tmpHeader,"Cons "); break;
          case mpsURem:      strcat(tmpHeader,"Rem  "); break;
          case mpsUPerC:     strcat(tmpHeader,"PerC "); break;
          case mpsUExeTime:  strcat(tmpHeader,"ExeT "); break;
          }
 
        sprintf(tmpCWLine,"%s%5ld",
          tmpCWLine,
          SWeight[index]);
        }  /* END for (index) */
 
      if (tmpHeader[0] != '\0')
        {
        sprintf(CHeader[mpcUsage],"   Res(%s)",
          tmpHeader);
 
        sprintf(CWLine[mpcUsage]," %5ld(%s)",
          CWeight[mpcUsage],
          tmpCWLine);
        }
      }    /* END if (CWeight[mpcUsage] != 0) */

      MStringAppendF(String,"%-20s %10s%c %*s%*s%*s%*s%*s%*s%*s\n",
        "Job",
        "PRIORITY",
        '*',
        (int)strlen(CHeader[mpcCred]),
        CHeader[mpcCred],
        (int)strlen(CHeader[mpcFS]),
        CHeader[mpcFS],
        (int)strlen(CHeader[mpcAttr]),
        CHeader[mpcAttr],
        (int)strlen(CHeader[mpcServ]),
        CHeader[mpcServ],
        (int)strlen(CHeader[mpcTarg]),
        CHeader[mpcTarg],
        (int)strlen(CHeader[mpcRes]),
        CHeader[mpcRes],
        (int)strlen(CHeader[mpcUsage]),
        CHeader[mpcUsage]);

      MStringAppendF(String,"%20s %10s%c %*s%*s%*s%*s%*s%*s%*s\n",
        "Weights",
        "--------",
        ' ',
        (int)strlen(CWLine[mpcCred]),
        CWLine[mpcCred],
        (int)strlen(CWLine[mpcFS]),
        CWLine[mpcFS],
        (int)strlen(CWLine[mpcAttr]),
        CWLine[mpcAttr],
        (int)strlen(CWLine[mpcServ]),
        CWLine[mpcServ],
        (int)strlen(CWLine[mpcTarg]),
        CWLine[mpcTarg],
        (int)strlen(CWLine[mpcRes]),
        CWLine[mpcRes],
        (int)strlen(CWLine[mpcUsage]),
        CWLine[mpcUsage]);

      MStringAppendF(String,"\n");
      }  /* END else (DFormat == mfmXML) */

    MDB(5,fUI) MLog("INFO:     %s header created\n",
      FName);

    return(SUCCESS);
    }  /* END if (Mode == mpdHeader) */

  if (Mode == mpdFooter)
    {
    /* display priority footer */

    memset(CFooter,'\0',sizeof(CFooter));  

    for (cindex = mpcServ;cindex <= mpcUsage;cindex++)
      {
      if (CWeight[cindex] != 0)
        {
        tmpLine[0] = '\0';
 
        for (sindex = MPSCStart[cindex];sindex <= MPSCEnd[cindex];sindex++)
          {
          if ((SWeight[sindex] == 0) && (SDisplay[sindex] != TRUE))
            continue;

          if (DFormat != mfmXML)
            { 
            if (tmpLine[0] != '\0')
              {
              strcat(tmpLine,":");
              }

            sprintf(tmpS,"%3.1f",
              (TotalPriority != 0.0) ?
                ABS(((double)TotalSFactor[sindex] * SWeight[sindex] * CWeight[cindex] * 100.0 / TotalPriority)) :
                0.0);

            sprintf(tmpLine,"%s%5.5s",
              tmpLine,
              tmpS);
            }  /* END if (DFormat != mfmXML) */
          }    /* END for (sindex) */

        if (DFormat != mfmXML)
          { 
          if (tmpLine[0] != '\0')
            {
            sprintf(tmpS,"%3.1f",
              (TotalPriority != 0.0) ?
                ABS(((double)TotalCFactor[cindex] * CWeight[cindex] * 100.0 / TotalPriority)) :
                0.0);
 
            sprintf(CFooter[cindex]," %5.5s(%s)",
              tmpS,
              tmpLine);
            }
          }  /* END if (DFormat != mfmXML) */
        }    /* END if (CWeight[cindex] != 0) */
      }      /* END for (cindex) */

    if (DFormat != mfmXML)
      {
      MStringAppendF(String,"\n");

      MStringAppendF(String,"%-20s %10s%c %*s%*s%*s%*s%*s%*s%*s\n",
        "Percent Contribution",
        "--------",
        ' ',
        (int)strlen(CFooter[mpcCred]),
        CFooter[mpcCred],
        (int)strlen(CFooter[mpcFS]),
        CFooter[mpcFS],
        (int)strlen(CFooter[mpcAttr]),
        CFooter[mpcAttr],
        (int)strlen(CFooter[mpcServ]),
        CFooter[mpcServ],
        (int)strlen(CFooter[mpcTarg]),
        CFooter[mpcTarg],
        (int)strlen(CFooter[mpcRes]),
        CFooter[mpcRes],
        (int)strlen(CFooter[mpcUsage]),
        CFooter[mpcUsage]);

      MStringAppendF(String,"\n");

      MStringAppendF(String,"* indicates absolute/relative system prio set on job\n");
      }  /* END if (DFormat != mfmXML) */

    return(SUCCESS);
    }  /* END if (Mode == mpdFooter) */

  /* calculate global values */

  if (J->SpecWCLimit[0] == 0)
    {
    MDB(0,fSCHED) MLog("ERROR:    job '%s' has no WCLimit specified\n",
      J->Name);
    }

  if (J->EffQueueDuration >= 0)
    {
    EffQDate = MSched.Time - J->EffQueueDuration;
    
    MDB(7,fSCHED) MLog("INFO:    EffQDate is MSched.Time (%ld) - J->EffQueueDuration (%ld)\n",MSched.Time,J->EffQueueDuration); /* BRIAN - debug (2400) */
    }
  else if (bmisset(&MPar[0].Flags,mpfUseSystemQueueTime))
    {
    EffQDate = J->SystemQueueTime;
    
    MDB(7,fSCHED) MLog("INFO:    EffQDate is J->SystemQueueTime (%ld)\n",J->SystemQueueTime); /* BRIAN - debug (2400) */
    }
  else
    {
    EffQDate = J->SubmitTime;
    
    MDB(7,fSCHED) MLog("INFO:    EffQDate is J->SubmitTime (%ld)\n",J->SubmitTime); /* BRIAN - debug (2400) */
    }

  MDB(7,fSCHED) MLog("INFO:    EffQDate is min of MSched.Time (%ld) and EffQDate (%ld)\n",MSched.Time,EffQDate); /* BRIAN - debug (2400) */
  EffQDate = MIN((long)MSched.Time,EffQDate);

  MDB(7,fSCHED) MLog("INFO:    MSched.Time = %ld\n EffQDate = %ld\n (MSched.Time - EffQDate) = %ld\n J->SpecWCLimit = %ld\n (MSched.Time - EffQDate) + J->SpecWClimit) = %ld\n MinWCLimit = %ld\n Result = %ld\n", /* BRIAN - debug (2400) */
      MSched.Time,
      EffQDate,
      MSched.Time - EffQDate,
      J->SpecWCLimit[0],
      (MSched.Time - EffQDate) + J->SpecWCLimit[0],
      MinWCLimit,
      (((MSched.Time - EffQDate) + J->SpecWCLimit[0])/(MAX(1,MAX(MinWCLimit,J->SpecWCLimit[0])))));

  XFactor = (double)(((unsigned long)(MSched.Time - EffQDate) + J->SpecWCLimit[0])) / 
    MAX(1,MAX(MinWCLimit,J->SpecWCLimit[0]));

  J->EffXFactor = XFactor;

  MDB(7,fSCHED) MLog("INFO:   XFactor = %f\n",XFactor); /* BRIAN - debug (2400) */

  /* calculate cred factor */

  if ((J->Credential.U != NULL) && (J->Credential.U->F.Priority != 0))       
    SFactor[mpsCU] = J->Credential.U->F.Priority;
  else if (MSched.DefaultU != NULL)
    SFactor[mpsCU] = MSched.DefaultU->F.Priority;

  if ((J->Credential.G != NULL) && (J->Credential.G->F.Priority != 0))          
    SFactor[mpsCG] = J->Credential.G->F.Priority;
  else if (MSched.DefaultG != NULL)
    SFactor[mpsCG] = MSched.DefaultG->F.Priority;

  if ((J->Credential.A != NULL) && (J->Credential.A->F.Priority != 0))     
    SFactor[mpsCA] = J->Credential.A->F.Priority; 
  else if (MSched.DefaultA != NULL)
    SFactor[mpsCA] = MSched.DefaultA->F.Priority;         
    
  if ((J->Credential.Q != NULL) && (J->Credential.Q->F.Priority != 0))
    SFactor[mpsCQ] = J->Credential.Q->F.Priority;
  else if (MSched.DefaultQ != NULL)
    SFactor[mpsCQ] = MSched.DefaultQ->F.Priority;

  if ((J->Credential.C != NULL) && (J->Credential.C->F.Priority != 0))
    SFactor[mpsCC] = J->Credential.C->F.Priority;
  else if (MSched.DefaultC != NULL)
    SFactor[mpsCC] = MSched.DefaultC->F.Priority;
 
  CFactor[mpcCred] = 0;

  for (index = MPSCStart[mpcCred];index <= MPSCEnd[mpcCred];index++)
    {
    SFactor[index] = (SCap[index] > 0) ? 
      MIN(SFactor[index],SCap[index]) : 
      SFactor[index];

    SFactor[index] = (SCap[index] > 0) ?
      MAX(SFactor[index],-SCap[index]) :
      SFactor[index];

    CFactor[mpcCred] += SWeight[index] * SFactor[index];
    }  /* END for (index) */
 
  /* calculate FS factor (target utilization delta) */

  CFactor[mpcFS] = 0;

  bmunset(&J->SpecFlags,mjfFSViolation);
  bmunset(&J->Flags,mjfFSViolation);

  if ((GF->FSPolicy != mfspNONE) && 
     ((GP->F.FSUsage[0] + GP->F.FSFactor) > 0.0))
    {
    mfs_t *CFS = NULL;
    mfs_t *DFS;

    double FSFactor;

    DFS = NULL;

    for (index = MPSCStart[mpcFS];index <= MPSCEnd[mpcFS];index++)
      {
      switch (index)
        {
        case mpsFU:
        case mpsGFU:

          CFS = (J->Credential.U != NULL) ? &J->Credential.U->F : NULL;
          DFS = (MSched.DefaultU != NULL) ? &MSched.DefaultU->F : NULL;

          break;

        case mpsFG:
        case mpsGFG:

          CFS = (J->Credential.G != NULL) ? &J->Credential.G->F : NULL;
          DFS = (MSched.DefaultG != NULL) ? &MSched.DefaultG->F : NULL;

          break;

        case mpsFA:
        case mpsGFA:

          CFS = (J->Credential.A != NULL) ? &J->Credential.A->F : NULL;
          DFS = (MSched.DefaultA != NULL) ? &MSched.DefaultA->F : NULL;

          break;

        case mpsFQ:

          CFS = (J->Credential.Q != NULL) ? &J->Credential.Q->F : NULL;
          DFS = (MSched.DefaultQ != NULL) ? &MSched.DefaultQ->F : NULL;

          break;

        case mpsFC:

          CFS = (J->Credential.C != NULL) ? &J->Credential.C->F : NULL;
          DFS = (MSched.DefaultC != NULL) ? &MSched.DefaultC->F : NULL;

          break;

        case mpsFUWCA:

          if ((J->Credential.U != NULL) && (J->Credential.U->Stat.PSRequest > 0))
            {
            SFactor[index] = J->Credential.U->Stat.PSRun / J->Credential.U->Stat.PSRequest;
            }

          break;

        case mpsFJPU:

          if (J->Credential.U != NULL)
            SFactor[index] = J->Credential.U->L.ActivePolicy.Usage[mptMaxJob][0];

          break;

        case mpsFJRPU:

          if (J->Credential.U != NULL)
            {
            if (J->Credential.U->L.ActivePolicy.HLimit[mptMaxJob][0] > 0)
              {
              SFactor[index] = J->Credential.U->L.ActivePolicy.HLimit[mptMaxJob][0] - J->Credential.U->L.ActivePolicy.Usage[mptMaxJob][0];
              }
            else
              {
              SFactor[index] = J->Credential.U->L.ActivePolicy.Usage[mptMaxJob][0];
              }
            }

          break;

        case mpsFPPU:

          if (J->Credential.U != NULL)
            SFactor[index] = J->Credential.U->L.ActivePolicy.Usage[mptMaxProc][0];

          break;

        case mpsFPSPU:

          if (J->Credential.U != NULL)
            SFactor[index] = J->Credential.U->L.ActivePolicy.Usage[mptMaxPS][0];

          break;

        default:

          /* not supported */

          CFS = NULL;

          break;
        }  /* END switch (index) */

      if ((index == mpsFJPU) || 
          (index == mpsFJRPU) ||
          (index == mpsFPPU) || 
          (index == mpsFPSPU) || 
          (index == mpsFUWCA))
        { 
        /* NO-OP */
        }
      else if ((index == mpsGFU) || (index == mpsGFG) || (index == mpsGFA))
        {
        if (CFS == NULL)
          continue;

        if (CFS->GFSTarget > 0.0)
          {
          FSTargetUsage = CFS->GFSTarget;
          }
        else if ((DFS != NULL) && (DFS->GFSTarget > 0.0))
          {
          FSTargetUsage = DFS->GFSTarget;
          }
        else
          {
          FSTargetUsage = 0.0;
          }

        if ((P != NULL) && 
            (P->FSC.ShareTree != NULL) && 
            (P->Index != 0) &&
            (J->FairShareTree != NULL))
          {
          if ((J->FairShareTree[P->Index] != NULL) &&
              (J->FairShareTree[P->Index]->TData != NULL) &&
              (J->FairShareTree[P->Index]->TData->F != NULL))
            {
            FSFactor = J->FairShareTree[P->Index]->TData->F->FSTreeFactor;
            }
          else if (MSched.UseAnyPartitionPrio == TRUE)
            {
            int tindex = 0;
   
            /* use tree information from the first valid partition tree */
            /* another partition's info is better than 0 info */
   
            while (tindex < MMAX_PAR)
              {
              if ((J->FairShareTree[tindex] != NULL) &&
                  (J->FairShareTree[tindex]->TData != NULL))
                {
                /* use J->CPAL if it is set, otherwise use J->PAL */

                if (!bmisclear(&J->CurrentPAL))
                  {
                  if (bmisset(&J->SpecPAL,tindex) == FALSE)
                    {
                     tindex++;
            
                    continue;
                    }

                  FSFactor = J->FairShareTree[tindex]->TData->F->FSTreeFactor;
   
                  break;
                  }
                else if (bmisset(&J->PAL,tindex) == FALSE)
                  {
                  tindex++;

                  continue;
                  }

                FSFactor = J->FairShareTree[tindex]->TData->F->FSTreeFactor;
   
                break;
                }

              tindex++;
              }
   
            if (tindex >= MMAX_PAR)
              FSFactor = CFS->FSFactor + CFS->FSUsage[0];
            }  /* END  else if (MSched.UseAnyPartitionPrio == TRUE) */
          else
            {
            FSFactor = CFS->FSFactor + CFS->FSUsage[0];
            }
          }
        else if ((MPar[0].FSC.ShareTree != NULL) &&
                 (J->FairShareTree != NULL) &&
                 (J->FairShareTree[0] != NULL))
          {
          if ((J->FairShareTree[0]->TData != NULL) &&
              (J->FairShareTree[0]->TData->F != NULL))
            {
            FSFactor = J->FairShareTree[0]->TData->F->FSTreeFactor;
            }
          }
        else if ((J->FairShareTree != NULL) &&
                 (MSched.UseAnyPartitionPrio == TRUE))
          {
          int tindex = 0;
 
          /* use tree information from the first valid partition tree */
          /* another partition's info is better than 0 info */
 
          while (tindex < MMAX_PAR)
            {
            if ((J->FairShareTree[tindex] != NULL) &&
                (J->FairShareTree[tindex]->TData != NULL))
              {
              /* use J->CPAL if it is set, otherwise use J->PAL */

              if (!bmisclear(&J->CurrentPAL))
                {
                if (!bmisset(&J->SpecPAL,tindex))
                  {
                   tindex++;
          
                  continue;
                  }

                FSFactor = J->FairShareTree[tindex]->TData->F->FSTreeFactor;
 
                break;
                }
              else if (!bmisset(&J->PAL,tindex))
                {
                tindex++;

                continue;
                }

              FSFactor = J->FairShareTree[tindex]->TData->F->FSTreeFactor;
 
              break;
              }

            tindex++;
            }
 
          if (tindex >= MMAX_PAR)
            FSFactor = CFS->FSFactor + CFS->FSUsage[0];
          }  /* END  else if (MSched.UseAnyPartitionPrio == TRUE) */
        else
          {
          FSFactor = CFS->GFSUsage;
          }

        if ((FSTargetUsage > 0.0) && (GP->F.GFSUsage > 0.0))
          {
          /* Target 0-100, GFSUsage: DOUBLE */

          /* NOTE: not needed */
/*
          if (MSched.PercentBasedFS == TRUE)
            {
            SFactor[index] = 
              (CFS->GFSUsage * 100.0 / (GP->F.GFSUsage * FSTargetUsage) - 1) * 100.0;
            }
          else
            {
*/
            SFactor[index] = FSTargetUsage -
              CFS->GFSUsage /
              GP->F.GFSUsage * 100.0;
/*
            }
*/
          }
        }    /* END else if ((index == mpsGFU) || ...) */
      else
        {
        /* handle basic fairshare calculation */

        if (CFS == NULL)
          continue;

        FSFactor = 0.0;

        if ((P != NULL) && 
            (P->FSC.ShareTree != NULL) && 
            (P->Index != 0) &&
            (J->FairShareTree != NULL))
          {
          /* NOTE:  change to CFS->PFSTreeFactor[P->Index] when enabled */

          if ((J->FairShareTree[P->Index] != NULL) &&
              (J->FairShareTree[P->Index]->TData != NULL) &&
              (J->FairShareTree[P->Index]->TData->F != NULL))
            {
            FSFactor = J->FairShareTree[P->Index]->TData->F->FSTreeFactor;

            SFactor[index] = J->FairShareTree[P->Index]->TData->F->FSTreeFactor;
            }
          else if (MSched.UseAnyPartitionPrio == TRUE)
            {
            int tindex = 0;
   
            /* use tree information from the first valid partition tree */
            /* another partition's info is better than 0 info */
   
            while (tindex < MMAX_PAR)
              {
              if ((J->FairShareTree[tindex] != NULL) &&
                  (J->FairShareTree[tindex]->TData != NULL))
                {
                /* use J->CPAL if it is set, otherwise use J->PAL */
   
                if (!bmisclear(&J->CurrentPAL))
                  {
                  if (!bmisset(&J->SpecPAL,tindex))
                    {
                    tindex++;
            
                    continue;
                    }

                  FSFactor = J->FairShareTree[tindex]->TData->F->FSTreeFactor;
                  SFactor[index] = J->FairShareTree[tindex]->TData->F->FSTreeFactor;
   
                  break;
                  }
                else if (!bmisset(&J->PAL,tindex))
                  {
                  tindex++;
   
                  continue;
                  }
     
                FSFactor = J->FairShareTree[tindex]->TData->F->FSTreeFactor;
                SFactor[index] = J->FairShareTree[tindex]->TData->F->FSTreeFactor;
   
                break;
                }

              tindex++;
              }
   
            if (tindex >= MMAX_PAR)
              FSFactor = CFS->FSFactor + CFS->FSUsage[0];
            }  /* END  else if (MSched.UseAnyPartitionPrio == TRUE) */
          else
            {
            FSFactor = CFS->FSFactor + CFS->FSUsage[0];
            }
          }
        else if ((MPar[0].FSC.ShareTree != NULL) &&
                 (J->FairShareTree != NULL) &&
                 (J->FairShareTree[0] != NULL))
          {
          if ((J->FairShareTree[0]->TData != NULL) &&
              (J->FairShareTree[0]->TData->F != NULL))
            {
            FSFactor = J->FairShareTree[0]->TData->F->FSTreeFactor;
            SFactor[index] = J->FairShareTree[0]->TData->F->FSTreeFactor;
            }
          }
        else if ((J->FairShareTree != NULL) &&
                 (MSched.UseAnyPartitionPrio == TRUE))
          {
          int tindex = 0;
 
          /* use tree information from the first valid partition tree */
          /* another partition's info is better than 0 info */
 
          while (tindex < MMAX_PAR)
            {
            if ((J->FairShareTree[tindex] != NULL) &&
                (J->FairShareTree[tindex]->TData != NULL))
              {
              /* use J->CPAL if it is set, otherwise use J->PAL */

              if (!bmisset(&J->CurrentPAL,MMAX_PAR))
                {
                if (!bmisset(&J->SpecPAL,tindex))
                  {
                   tindex++;
          
                  continue;
                  }

                FSFactor = J->FairShareTree[tindex]->TData->F->FSTreeFactor;
                SFactor[index] = J->FairShareTree[tindex]->TData->F->FSTreeFactor;
 
                break;
                }
              else if (!bmisset(&J->PAL,tindex))
                {
                tindex++;

                continue;
                }

              FSFactor = J->FairShareTree[tindex]->TData->F->FSTreeFactor;
              SFactor[index] = J->FairShareTree[tindex]->TData->F->FSTreeFactor;
 
              break;
              }

            tindex++;
            }
 
          if (tindex >= MMAX_PAR)
            FSFactor = CFS->FSFactor + CFS->FSUsage[0];
          }  /* END  else if (MSched.UseAnyPartitionPrio == TRUE) */
        else
          {
          FSFactor = CFS->FSFactor + CFS->FSUsage[0];
          }

        if (CFS->FSTarget > 0.0)
          {
          FSTargetUsage = CFS->FSTarget;
          FSTargetMode  = CFS->FSTargetMode;
          }
        else if ((DFS != NULL) && (DFS->FSTarget > 0.0))
          {
          FSTargetUsage = DFS->FSTarget;
          FSTargetMode  = DFS->FSTargetMode;
          }
        else
          {
          FSTargetUsage = 0.0;
          FSTargetMode  = mfstNONE;
          }

        if (FSTargetUsage > 0.0)
          {
          double ActualUsage = FSFactor / (GP->F.FSUsage[0] + GP->F.FSFactor) * 100.0F;

          if (GF->UseExpTarget == TRUE)
            {
            /* NYI */

            SFactor[index] = FSTargetUsage - ActualUsage;
            }
          else if (MSched.FSRelativeUsage == TRUE)
            {
            /* Is this the right calculation? */

            SFactor[index] = 1.0F - ActualUsage / FSTargetUsage;
            }
          else
            {
            SFactor[index] = FSTargetUsage - ActualUsage;
            }

          switch (FSTargetMode)
            {
            case mfstCeiling: SFactor[index] = MIN(SFactor[index],0.0); break;
            case mfstFloor:   SFactor[index] = MAX(SFactor[index],0.0); break;

            case mfstCapAbs: 
            case mfstCapRel:
 
              /* deprecated */

              if (GF->EnableCapPriority == FALSE)
                {
                SFactor[index] = 0.0;
                }

              break;

            case mfstTarget:
            default: 
 
              /* NO-OP */

              break;
            }  /* END switch (FSTargetMode) */

          if (SFactor[index] < 0.0)
            {
            bmset(&J->SpecFlags,mjfFSViolation);
            bmset(&J->Flags,mjfFSViolation);
            }
          }    /* END if (FSTargetUsage > 0.0) */
        }      /* END else ((index == ...) | ...) */

      SFactor[index] = (SCap[index] > 0) ?
        MIN(SFactor[index],SCap[index]) :
        SFactor[index];

      SFactor[index] = (SCap[index] > 0) ?
        MAX(SFactor[index],-SCap[index]) :
        SFactor[index];

      CFactor[mpcFS] += SWeight[index] * SFactor[index];
      }  /* END for (index) */
    }    /* END if ((GF->FSPolicy != mfspNONE) && ...) */

  /* calculate attr component */

  CFactor[mpcAttr] = 0;

  for (index = MPSCStart[mpcAttr];index <= MPSCEnd[mpcAttr];index++)
    {
    switch (index)
      {
      case mpsAAttr:

        MJPrioFGetPrio(MSched.JPrioF,J,mjpatGAttr,&tmpL);

        SFactor[index] = (double)tmpL;

        break;

      case mpsAGRes:

        MJPrioFGetPrio(MSched.JPrioF,J,mjpatGRes,&tmpL);

        SFactor[index] = (double)tmpL;

        break;

      case mpsAJobID:

        SFactor[index] = strtod(J->Name,NULL);

        break;

      case mpsAJobName:

        if (J->AName != NULL)
          {
          char *tmpPtr;

          tmpPtr = J->AName;

          while (!isdigit(*tmpPtr) && (*tmpPtr != '\0'))  /* this should protect against going off end of buffer */
            tmpPtr++;
          
          if (isdigit(*tmpPtr))
            {
#ifdef __M64
            /* on 64-bit systems, a long is 64-bits and we don't run into problems with overflowing J->StartPrio */

            SFactor[index] = strtod(tmpPtr,NULL);
#else
          /* we are getting the remainder here to make sure this SFactor is well below
           * MMAX_PRIO_VAL--this is important for AName's like 200712230940-A! If we don't
           * get the remainder, we overflow the J->StartPrio! */

            SFactor[index] = fmod(strtod(tmpPtr,NULL),MMAX_PRIO_VAL/10);  /* divide by 100,000,000 and use remainder */
#endif /* __M64 */
            }
          else
            {
            SFactor[index] = 0; 
            }
          }

        break;

      case mpsAState:

        MJPrioFGetPrio(MSched.JPrioF,J,mjpatState,&tmpL);

        SFactor[index] = (double)tmpL;

        break;

      default:

        /* NO-OP */

        break;
      }  /* END switch (index) */

    MDB(8,fSTRUCT) MLog("INFO:     SFactor[%d] = %.2f\n",
      index,
      SFactor[index]);
    }    /* END for (index) */

  for (index = MPSCStart[mpcAttr];index <= MPSCEnd[mpcAttr];index++)
    {
    SFactor[index] = (SCap[index] > 0) ?
      MIN(SFactor[index],SCap[index]) :
      SFactor[index];

    MDB(8,fSTRUCT) MLog("INFO:     SFactor[%d] = %.2f (MIN(SFactor[index],SCap[index]:%ld) if SCap[index] > 0)\n",
      index,
      SFactor[index],
      SCap[index]);

    SFactor[index] = (SCap[index] > 0) ?
      MAX(SFactor[index],-SCap[index]) :
      SFactor[index];

    MDB(8,fSTRUCT) MLog("INFO:     SFactor[%d] = %.2f (MAX(SFactor[index],-SCap[index]:%ld) if SCap[index] > 0)\n",
      index,
      SFactor[index],
      -SCap[index]);

    if (SWeight[index] != 0)
      {
      /* there are problems with AName with a name like "3E71486D-AD3E-DF11-AFF8-001617C3B6E2.root",
         This results in SFactor[index] = inf.  "inf" * 0 = nan. (RT7407) */

      CFactor[mpcAttr] += SWeight[index] * SFactor[index];
      }

    MDB(8,fSTRUCT) MLog("INFO:     CFactor[mcpAttr] = %.2f (SWeight[%d]:%ld * SFactor[%d]:%.2f)\n",
      CFactor[mpcAttr],
      index,
      SWeight[index],
      index,
      SFactor[index]);

    }  /* END for (index) */

  /* calculate service component */

  /* queue time factor (in minutes) */
 
  SFactor[mpsSQT] = (double)((MSched.Time - EffQDate) / MCONST_MINUTELEN);

  SFactor[mpsSXF] = (double)XFactor;

  if ((J->CMaxDate > 0) && (J->CMaxDate < MMAX_TIME))
    {
    /* deadline factor value, ranges from 0.0 to 1.0 
         1.0 indicates deadline is reached/surpassed */

    SFactor[mpsSDeadline] = (double)(1.0 / (1 + MAX(0,(long)J->CMaxDate - (long)MSched.Time)));
    }

  SFactor[mpsSSPV] = bmisset(&J->Flags,mjfSPViolation) ? 1.0 : 0.0;

  SFactor[mpsSUPrio] = (double)J->UPriority;

  SFactor[mpsSBP] = (double)J->BypassCount;       

  SFactor[mpsSStartCount] = (double)J->StartCount;

  CFactor[mpcServ] = 0;

  MDB(7,fSCHED) /* BRIAN - debug (2400) */
    {
    MLog("INFO:    SFactor[mpsSQT]    (QueueTime) = %f\n",SFactor[mpsSQT]);
    MLog("INFO:    SFactor[mpsSXF]    (XFactor)   = %f\n",SFactor[mpsSXF]);
    MLog("INFO:    SFactor[mpsSDeadline] (Deadline) = %f\n",SFactor[mpsSDeadline]);
    MLog("INFO:    SFactor[mpsSSPV]   (SPolVio)   = %f\n",SFactor[mpsSSPV]);
    MLog("INFO:    SFactor[mpsSUPrio] (UserPrio)  = %f\n",SFactor[mpsSUPrio]);
    MLog("INFO:    SFactor[mpsSBP]    (BypassCt)  = %f\n",SFactor[mpsSBP]);
    MLog("INFO:    SFactor[mpsSStartCount] (StartCt) = %f\n",SFactor[mpsSStartCount]);
    }

  for (index = MPSCStart[mpcServ];index <= MPSCEnd[mpcServ];index++)
    {
    SFactor[index] = (SCap[index] > 0) ?
      MIN(SFactor[index],SCap[index]) :
      SFactor[index];

    SFactor[index] = (SCap[index] > 0) ?
      MAX(SFactor[index],-SCap[index]) :
      SFactor[index];

    CFactor[mpcServ] += SWeight[index] * SFactor[index];
  
    MDB(7,fSCHED) MLog("INFO:    CFactor[mpcServ] (%f) += SWeight[%d] (%ld) * SFactor[%d] (%f)\n", /* BRIAN - debug (2400) */
        CFactor[mpcServ],
        index,
        SWeight[index],
        index,
        SFactor[index]);

    }  /* END for (index) */

  MDB(7,fSCHED) MLog("INFO:    CFactor = %f\n",CFactor[mpcServ]); /* BRIAN - debug (2400) */

  /* calculate target component */

  /* target QT subcomponent */

  if (J->Credential.Q != NULL)
    { 
    if (J->Credential.Q->QTTarget > 0)
      {
      /* give exponentially increasing priority as we approach target qtime */

      /* Equation:  (1,000)^( QTCurrent / QTTarget )                  */ 

      SFactor[mpsTQT] =
        (double)pow(1000,((double)MIN(1.0,(double)J->EffQueueDuration / (double)J->Credential.Q->QTTarget)));
      }

    /* target XF subcomponent */

    if (J->Credential.Q->XFTarget > 0.0)
      {
      /* give exponentially increasing priority as we approach target xfactor */
      /* Equation:  (XFTarget - XFCurrent)^(-2)                               */

      SFactor[mpsTXF] = 
        (double)pow((MAX(.0001,J->Credential.Q->XFTarget - XFactor)),-2.0);
      }
    }

  SFactor[mpsTQT] = (SCap[mpsTQT] > 0) ? 
    MIN(SFactor[mpsTQT],SCap[mpsTQT]) : 
    SFactor[mpsTQT];

  SFactor[mpsTXF] = (SCap[mpsTXF] > 0) ? 
    MIN(SFactor[mpsTXF],SCap[mpsTXF]) : 
    SFactor[mpsTXF];

  CFactor[mpcTarg] =
    SWeight[mpsTQT] * SFactor[mpsTQT] +
    SWeight[mpsTXF] * SFactor[mpsTXF];

  /* determine resource factor */

  Target = 0;
  TargetMod = mfstNONE;

  if (P != NULL)
    {
    Target = P->PriorityTargetProcCount;
    TargetMod = P->PTProcCountTargetMode;
    }

  if (Target == 0)
    {
    Target = GP->PriorityTargetProcCount;
    TargetMod = GP->PTProcCountTargetMode;
    }
  
  for (rqindex = 0;J->Req[rqindex] != NULL;rqindex++)
    {
    RQ = J->Req[rqindex];

    SFactor[mpsRNode]    += RQ->NodeCount;

    if (Target == 0)
      {
      SFactor[mpsRProc]  += RQ->TaskCount * RQ->DRes.Procs;
      } 

    SFactor[mpsRMem ]    += RQ->TaskCount * RQ->DRes.Mem ;    
    SFactor[mpsRSwap]    += RQ->TaskCount * RQ->DRes.Swap;    
    SFactor[mpsRDisk]    += RQ->TaskCount * RQ->DRes.Disk;  
    SFactor[mpsRPS  ]    += RQ->TaskCount * RQ->DRes.Procs * J->WCLimit; 
    }  /* END for (rqindex) */

  if (Target != 0)
    {
    PC = J->TotalProcCount;

    switch (TargetMod)
      {
      case mfstFloor:

        SFactor[mpsRProc]  += MIN(0,PC - Target);

        break;

      case mfstCeiling:

        SFactor[mpsRProc]  += MIN(0,Target - PC);

        break;

      default:

        SFactor[mpsRProc]  -= abs(Target - PC);

        break;
      }  /* END switch (TargetMod) */
    }

  Target = 0;
  TargetMod = mfstNONE;

  if (P != NULL)
    {
    Target = P->PriorityTargetDuration;
    TargetMod = P->PTDurationTargetMode;
    }

  if (Target == 0)
    {
    Target = GP->PriorityTargetDuration;
    TargetMod = GP->PTDurationTargetMode;
    }

  if (Target != 0)
    {
    switch (TargetMod)
      {
      case mfstFloor:

        SFactor[mpsRWallTime]  += MIN(0,J->WCLimit - Target);

        break;

      case mfstCeiling:

        SFactor[mpsRWallTime]  += MIN(0,Target - J->WCLimit);

        break;

      default:

        SFactor[mpsRWallTime]  -= abs(Target - (long)J->WCLimit);

        break;
      }  /* END switch (TargetMod) */
    }
  else
    {
    SFactor[mpsRWallTime] = J->WCLimit;
    }
      
  MJobGetPE(J,GP,&SFactor[mpsRPE]);
 
  CFactor[mpcRes] = 0;

  for (sindex = MPSCStart[mpcRes];sindex <= MPSCEnd[mpcRes];sindex++)
    { 
    SFactor[sindex] = (SCap[sindex] > 0) ? 
      MIN(SFactor[sindex],SCap[sindex]) : 
      SFactor[sindex];

    SFactor[index] = (SCap[index] > 0) ?
      MAX(SFactor[index],-SCap[index]) :
      SFactor[index];

    CFactor[mpcRes] += SWeight[sindex] * SFactor[sindex];
    }  /* END for (sindex) */

  /* calculate usage factor */

  MJobGetRunPriority(J,(P != NULL) ? P->Index : 0,NULL,&CFactor[mpcUsage],NULL);

  Prio = 0.0;
  APrio = 0.0;

  for (index = mpcServ;index <= mpcUsage;index++)
    {
    CFactor[index] = (CCap[index] > 0) ? 
      MIN(CFactor[index],CCap[index]) : 
      CFactor[index];

    MDB(8,fSTRUCT) MLog("INFO:     CFactor[%d] = %.2f (MIN(CFactor[index],CCap[index]:%ld) if CCap[index] > 0)\n",
      index,
      CFactor[index],
      CCap[index]);

    CFactor[index] = (CCap[index] > 0) ?
      MAX(CFactor[index],-CCap[index]) :
      CFactor[index];
    
    MDB(8,fSTRUCT) MLog("INFO:     CFactor[%d] = %.2f (MAX(CFactor[index],-CCap[index]:%ld) if CCap[index] > 0)\n",
      index,
      CFactor[index],
      -CCap[index]);

    Prio  += (double)CWeight[index] * CFactor[index];

    MDB(7,fSCHED) MLog("INFO:    CWeight[%d] = %ld * CFactor[%d] = %f   += Prio = %f\n", /* BRIAN - debug (2400) */
        index,
        CWeight[index],
        index,
        CFactor[index],
        Prio);

    APrio += ABS((double)CWeight[index] * CFactor[index]);

    MDB(8,fSTRUCT) MLog("INFO:     Prio = %.2f (CWeight[%d]=%ld * CFactor[%d]=%.2f)\n",
      Prio,
      index,
      CWeight[index],
      index,
      CFactor[index]);
    }  /* END for (index) */

  MDB(7,fSCHED) MLog("INFO:    Priority = %f\n",Prio); /* BRIAN - debug (2400) */

  if ((RE != NULL) || (String != NULL))
    {
    TotalPriority += APrio;

    for (index = 1;index < mpcLAST;index++)
      {
      if (APrio != 0.0)
        CP[index] = ABS((double)CWeight[index] * CFactor[index]) / APrio * 100.0;
      else
        CP[index] = 0.0;

      TotalCFactor[index] += ABS(CFactor[index]);
      }  /* END for (index) */

    memset(CLine,0,sizeof(CLine));
 
    for (index = 1;index < mpsLAST;index++)
      {
      SP[index] = (APrio != 0.0) ? 
        ABS((double)SWeight[index] * SFactor[index]) / APrio * 100.0 :
        0.0;        

      if (index >= mpsUCons)
        SP[index] *= CWeight[mpcUsage];
      else if (index >= mpsRNode)
        SP[index] *= CWeight[mpcRes];     
      else if (index >= mpsFU)
        SP[index] *= CWeight[mpcFS];
      else if (index >= mpsCU)
        SP[index] *= CWeight[mpcCred];
      else if (index >= mpsTQT)
        SP[index] *= CWeight[mpcTarg];
      else if (index >= mpsSQT)
        SP[index] *= CWeight[mpcServ];

      TotalSFactor[index] += ABS(SFactor[index]);
      }  /* END for (index) */

    if (DFormat == mfmXML)
      {
      JE = NULL;

      MXMLCreateE(&JE,"job");
      MXMLSetAttr(JE,"name",(void **)J->Name,mdfString);

      MXMLAddE(E,JE);
      }

    for (cindex = mpcServ;cindex <= mpcUsage;cindex++)
      {
      if (DFormat == mfmXML)
        {
        CE = NULL;

        MXMLCreateE(&CE,MCONST_PRIOCOMPNAME);
        MXMLSetAttr(CE,MSAN[msanName],(void **)MPrioCName[cindex],mdfString);

        MXMLAddE(JE,CE);
        }

      if ((CWeight[cindex] != 0) || (DFormat == mfmXML))
        {
        if (DFormat != mfmXML)
          {
          tmpLine[0] = '\0';
          }

        for (sindex = MPSCStart[cindex];sindex <= MPSCEnd[cindex];sindex++)
          {
          if (DFormat == mfmXML)
            {
            SE = NULL;

            MXMLCreateE(&SE,MCONST_PRIOSUBCOMPNAME);
            MXMLSetAttr(SE,MSAN[msanName],(void **)MPrioSCName[sindex],mdfString);
            MXMLSetAttr(SE,MSAN[msanValue],(void **)&SFactor[sindex],mdfDouble);
            /* MXMLSetAttr(SE,"metric","N/A",mdfString); */

            MXMLAddE(CE,SE);
            }

          if (DFormat != mfmXML)
            {
            if (SWeight[sindex] == 0)
              continue;

            if (tmpLine[0] != '\0')
              {
              strcat(tmpLine,":");
              }

            sprintf(tmpS,"%3.1f",
              (SFactor[sindex]));
              /* ABS(SFactor[sindex])); - not sure why this has ABS
               * this causes non-intuitive output in mdiag -p -v (JMB) */
         
            sprintf(tmpLine,"%s%5.5s",
              tmpLine,
              tmpS); 
            }
          }  /* END for (sindex) */

        if (DFormat != mfmXML)
          { 
          if (tmpLine[0] != '\0')
            {
            sprintf(tmpS,"%3.1f",
              CP[cindex]);

            sprintf(CLine[cindex]," %5.5s(%s)",
              tmpS,
              tmpLine);
            }
          }
        }    /* END if ((CWeight[cindex] != 0) || (DFormat == mfmXML)) */
      }      /* END for (sindex) */

    if (DFormat == mfmXML)
      {
      MXMLSetAttr(JE,"priority",(void **)&Prio,mdfDouble);

      if (J->SystemPrio > 0)
        {
        mstring_t tmp(MMAX_LINE);

        MJobAToMString(J,mjaSysPrio,&tmp);

        MXMLSetAttr(JE,"systemPriority",(void **)tmp.c_str(),mdfString);
        }
      }
    else
      {
      double EPrio;
      char tmpJName[MMAX_LINE * 2];  /* to cover large AName's ... */

      if (J->SystemPrio > 0)
        {
        if (J->SystemPrio > MMAX_PRIO_VAL)
          EPrio = Prio + (double)(J->SystemPrio - (MMAX_PRIO_VAL << 1));
        else
          EPrio = (double)(MMAX_PRIO_VAL + J->SystemPrio);
        }
      else
        {
        EPrio = Prio;
        }

      tmpJName[0] = '\0';

      if ((ShowJobName == FALSE) || (J->AName == NULL))
        {
        snprintf(tmpJName,sizeof(tmpJName),"%s",J->Name);
        }
      else
        {
        snprintf(tmpJName,sizeof(tmpJName),"%s(%s)",
          J->AName,
          J->Name);
        }

      MStringAppendF(String,"%-20s %10.0f%c %*s%*s%*s%*s%*s%*s%*s\n",
        tmpJName,
        EPrio,
        (J->SystemPrio > 0) ? '*' : ' ',
        (int)strlen(CLine[mpcCred]),
        CLine[mpcCred],
        (int)strlen(CLine[mpcFS]),
        CLine[mpcFS],
        (int)strlen(CLine[mpcAttr]),
        CLine[mpcAttr],
        (int)strlen(CLine[mpcServ]),
        CLine[mpcServ],
        (int)strlen(CLine[mpcTarg]),
        CLine[mpcTarg],
        (int)strlen(CLine[mpcRes]),
        CLine[mpcRes],
        (int)strlen(CLine[mpcUsage]),
        CLine[mpcUsage]);
      }  /* END else (DFormat == mfmXML) */
    }    /* END if (Buffer != NULL) */

  MDB(4,fSCHED) MLog("INFO:     job '%s' pre-bounded priority: %8.0f\n",
    J->Name,
    Prio);

  MDB(4,fSCHED) MLog("INFO:     Cred: %6.0f(%04.1f)  FS: %6.0f(%04.1f)  Attr: %6.0f(%04.1f)  Serv: %6.0f(%04.1f)  Targ: %6.0f(%04.1f)  Res: %6.0f(%04.1f)  Us: %6.0f(%04.1f)\n",
    (double)CWeight[mpcCred] * CFactor[mpcCred],
    CP[mpcCred],
    (double)CWeight[mpcFS] * CFactor[mpcFS],
    CP[mpcFS],
    (double)CWeight[mpcAttr] * CFactor[mpcAttr],
    CP[mpcAttr],
    (double)CWeight[mpcServ] * CFactor[mpcServ],
    CP[mpcServ],
    (double)CWeight[mpcTarg] * CFactor[mpcTarg],
    CP[mpcTarg],
    (double)CWeight[mpcRes] * CFactor[mpcRes],
    CP[mpcRes],
    (double)CWeight[mpcUsage] * CFactor[mpcUsage],
    CP[mpcUsage]);

  /* clip prio at min value */

  if ((!bmisset(&GP->Flags,mpfRejectNegPrioJobs)) &&
      (!bmisset(&GP->Flags,mpfEnableNegJobPriority)))
    {
    /* clip prio at min value */

    if (Prio < 1.0)
      Prio = 1.0;
    }

  /* cap job start priority */

  if (Prio > (double)MMAX_PRIO_VAL)
    Prio = (double)MMAX_PRIO_VAL;

  /* incorporate system priority value */

  if (J->SystemPrio > 0)
    {
    if (J->SystemPrio > MMAX_PRIO_VAL)
      Prio += (double)(J->SystemPrio - (MMAX_PRIO_VAL << 1));
    else
      Prio = (double)(MMAX_PRIO_VAL + J->SystemPrio);
    }

  MDB(5,fSCHED) MLog("INFO:     job '%s'  partition %s calc start priority: %8.2f\n",
    J->Name,
    (P == NULL) ? MPar[0].Name : P->Name,
    Prio);

  if (Priority != NULL)
    *Priority = Prio;

  return(SUCCESS);
  }  /* END MJobCalcStartPriority() */


