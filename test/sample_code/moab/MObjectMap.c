/* HEADER */

      
/**
 * @file MObjectMap.c
 *
 * Contains: Map Object functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  




/**
 *
 *
 * @param SpecName (I) [map name containing wildcards]
 * @param OID (I) [object id]
 */

int __MOMapIsWildCharMatch(

  char *SpecName,  /* I (map name containing wildcards) */
  char *OID)       /* I (object id) */

  {
  char tmpSpecName[MMAX_LINE];

  char *ptr;

  MUStrCpy(tmpSpecName,SpecName,sizeof(tmpSpecName));

  ptr = strchr(tmpSpecName,'*');

  /* NOTE:  only match [<head>]* with [<head>]X */

  /* must extend matching algo */

  if (ptr != NULL)
    {
    ptr[0] = '\0';

    if (strncmp(OID,tmpSpecName,strlen(tmpSpecName)))
      {
      /* fail if head of match string does not match head of oid */

      return(FAILURE);
      }

    return(SUCCESS);
    }

  /* fail if no wildcard located */

  return(FAILURE);
  }  /* END __MOMapIsWildCharMatch() */




/**
 *
 *
 * @param OMap (I)
 * @param OType (I)
 * @param OID (I)
 * @param DoReverse (I)
 * @param DoLocal (I)
 * @param MappedName (O) [minsize=MMAX_LINE]
 */

int MOMap(

  momap_t             *OMap,        /* I */
  enum MXMLOTypeEnum   OType,       /* I */
  char                *OID,         /* I */
  mbool_t              DoReverse,   /* I */
  mbool_t              DoLocal,     /* I */
  char                *MappedName)  /* O (minsize=MMAX_LINE) */

  {
  int cindex;

  char **SrcList;
  char **DstList;
  mbool_t IsWildCard = FALSE;
  mbool_t IsExact    = FALSE;

  if (MappedName != NULL)
    {
    MappedName[0] = '\0';
    }

  if ((OType == mxoNONE) ||
      (OID == NULL) ||
      (OID[0] == '\0') ||
      (MappedName == NULL))
    {
    return(FAILURE);
    }

  if ((OMap == NULL) || (OMap->IsMapped[OType] == FALSE))
    {
    MUStrCpy(MappedName,OID,MMAX_LINE);

    return(SUCCESS);
    }

  if (DoReverse == FALSE)
    {
    SrcList = OMap->SpecName;
    DstList = OMap->TransName;
    }
  else
    {
    SrcList = OMap->TransName;
    DstList = OMap->SpecName;
    }

  for (cindex = 0;cindex < MMAX_CREDS;cindex++)
    {
    if ((OMap->OType[cindex] != OType) ||
        (DoLocal != OMap->Local[cindex]) ||
        (SrcList[cindex] == NULL))
      {
      continue;
      }

    /* do translation */

    if (!strcmp(SrcList[cindex],OID) )
      {
      IsExact = TRUE;
      }
    else if (__MOMapIsWildCharMatch(SrcList[cindex],OID) == SUCCESS)
      {
      IsWildCard = TRUE; 
      }

    if ((IsExact == TRUE) || 
       ((IsWildCard == TRUE) && (OType != mxoCluster)) )
      {
      if (IsWildCard == TRUE)
        {
        char *BPtr;
        int   BSpace;

        char *Prefix;
        char *Suffix;
        char *TokPtr;
  
        char *ptr;
        char *OPtr;

        char  tmpName[MMAX_LINE];

        mbool_t MapWC = FALSE;

        /* "hide" is currently not documented - I believe we should make it %hide instead! */

        if (!strcasecmp(DstList[cindex],"hide"))
          {
          MappedName[0] = '\0';

          return(SUCCESS);
          }

        if (strchr(DstList[cindex],'*'))
          MapWC = TRUE;

        MUStrCpy(tmpName,DstList[cindex],sizeof(tmpName));

        if (DstList[cindex][0] == '*')
          {
          Prefix = NULL;
          Suffix = MUStrTok(tmpName,"*",&TokPtr);
          }
        else
          {
          Prefix = MUStrTok(tmpName,"*",&TokPtr);
          Suffix = MUStrTok(NULL,"*",&TokPtr);
          }

        MUSNInit(&BPtr,&BSpace,MappedName,MMAX_LINE);

        /* EXAMPLE: class:rti*,ncsa* */

        /* FORMAT:  [<SPECHEAD>]<SPECVAL> -> [<MAPHEAD>][<WILDCARD>] */

        /* mapped name = <MAPHEAD> + <SPECVAL> */

        OPtr = OID;

        if ((ptr = strchr(SrcList[cindex],'*')) != NULL)
          {
          OPtr += ptr - SrcList[cindex];
          }

        if (Prefix != NULL)
          {
          if (MapWC == TRUE)
            {
            MUSNPrintF(&BPtr,&BSpace,"%s%s",
              Prefix,
              OPtr);
            }
          else
            {
            MUSNCat(&BPtr,&BSpace,Prefix);
            } 
          }
        else
          {
          MUSNCat(&BPtr,&BSpace,OPtr);
          }
        
        if (Suffix != NULL)
          {
          MUSNCat(&BPtr,&BSpace,Suffix);
          }
        }
      else
        {
        MUStrCpy(MappedName,DstList[cindex],MMAX_LINE);
        }

      return(SUCCESS);
      }
    else if ((OType == mxoCluster) && 
             (IsWildCard == TRUE))
      {
      if (strchr(DstList[cindex],'*') != NULL)
        {
        char *BPtr;
        int   BSpace;

        char *Head;
        char *TokPtr;

        int   Here;
    
        char  tmpName[MMAX_LINE];

        MUStrCpy(tmpName,DstList[cindex],sizeof(tmpName));

        Head = MUStrTok(tmpName,"*",&TokPtr);

        MUSNInit(&BPtr,&BSpace,MappedName,MMAX_LINE);

        Here = strlen(SrcList[cindex]) - 1;

        if (Head != NULL)
          {
          MUSNPrintF(&BPtr,&BSpace,"%s%s",
            Head,
            &OID[Here]);
          }
        else
          {
          MUSNCat(&BPtr,&BSpace,OID);
          }
        }
      else
        {
        MUStrCpy(MappedName,DstList[cindex],MMAX_LINE);
        }

      return(SUCCESS);
      }
    }    /* END for (cindex) */ 

  /* no rules detected - return original name */

  MUStrCpy(MappedName,OID,MMAX_LINE);

  return(SUCCESS);
  }  /* END MOMap() */





/**
 *
 *
 * @param OMap (I)
 * @param IList (I)
 * @param DoReverse (I)
 * @param DoLocal (I)
 * @param OList (O)
 * @param OListSize (I)
 */

int MOMapClassList(

  momap_t             *OMap,        /* I */
  char                *IList,       /* I */
  mbool_t              DoReverse,   /* I */
  mbool_t              DoLocal,     /* I */
  char                *OList,       /* O */
  int                  OListSize)   /* I */

  {
  char *ptr;
  char *TokPtr;
  char  tmpLine[MMAX_LINE];
  char  tmpClass[MMAX_LINE];

  /* chop up list, map individual pieces, and then put back together again */

  if ((OMap == NULL) ||
      (IList == NULL) ||
      (OList == NULL) ||
      (OListSize < 0))
    {
    return(FAILURE);
    }

  OList[0] = '\0';
  tmpLine[0] = '\0';
  tmpClass[0] = '\0';

  /* FORMAT: [<QNAME> <QCOUNT>] ... */

  ptr = MUStrTok(IList,"[] ",&TokPtr);

  while (ptr != NULL)
    {
    if (MOMap(OMap,mxoClass,ptr,DoReverse,DoLocal,tmpLine) == FAILURE)
      {
      MDB(3,fNAT) MLog("INFO:     cannot map node's available class list");

      return(FAILURE);
      }

    ptr = MUStrTok(NULL,"[] ",&TokPtr);  /* <QCOUNT> */

    if (tmpLine[0] != '\0')
      {
      snprintf(tmpClass,sizeof(tmpClass),"[%s %s]",
        tmpLine,
        ptr);

      MUStrCat(OList,tmpClass,OListSize);
      }

    ptr = MUStrTok(NULL,"[] ",&TokPtr);  /* <QNAME> */
    }  /* END while (ptr != NULL) */
  
  return(SUCCESS);  
  }  /* END MOMapClassList() */




/**
 *
 *
 * @param IName (I)
 * @param SubmitName (O) [optional,minsize=MMAX_NAME]
 * @param OrigName (O) [optional,minsize=MMAX_NAME]
 */

int MOMapSplitNames(

  char *IName,      /* I */
  char *SubmitName, /* O (optional,minsize=MMAX_NAME) */
  char *OrigName)   /* O (optional,minsize=MMAX_NAME) */

  {
  char *ptr;

  if (IName == NULL)
    {
    return(FAILURE);
    }

  /* FORMAT: <SubmitName>/<OrigName> */

  if (SubmitName != NULL)
   SubmitName[0] = '\0'; 

  if (OrigName != NULL)
    OrigName[0] = '\0';

  if ((ptr = strchr(IName,'/')) != NULL)
    {
    if (SubmitName != NULL)
      MUStrCpy(SubmitName,IName,ptr - IName+1);

    if (OrigName != NULL)
      MUStrCpy(OrigName,ptr + 1,MMAX_NAME);
    }
  else
    {
    if (SubmitName != NULL)
      MUStrCpy(SubmitName,IName,MMAX_NAME);

    if (OrigName != NULL)
      MUStrCpy(OrigName,IName,MMAX_NAME);
    }

  return(SUCCESS);
  }  /* END MOMapGetOrig() */



/**
 * Convert list into omapped list.
 *
 * @param OMap (I)
 * @param OType (I)
 * @param IList (I)
 * @param DoReverse (I)
 * @param DoLocal (I)
 * @param OList (O) Expecting OList to be initialized (MStringInit())
 */

int MOMapList(

  momap_t             *OMap,        /* I */
  enum MXMLOTypeEnum   OType,       /* I */
  char                *IList,       /* I */
  mbool_t              DoReverse,   /* I */
  mbool_t              DoLocal,     /* I */
  mstring_t           *OList)       /* O (Expecting OList to be initialized)*/

  {
  char *ptr;
  char *TokPtr;
  char  tmpLine[MMAX_LINE];

  char *ptr2;
  char *TokPtr2;

  /* chop up list, map individual pieces, and then put back together again */

  if ((OMap == NULL) ||
      (IList == NULL) ||
      (OList == NULL)) 
    {
    return(FAILURE);
    }

  tmpLine[0] = '\0';

  /* FORMAT: <ITEM>[,<ITEM>]... */

  ptr = MUStrTok(IList,",",&TokPtr);

  while (ptr != NULL)
    {
    ptr2 = MUStrTok(ptr,":",&TokPtr2);
 
    if (MOMap(OMap,OType,ptr2,DoReverse,DoLocal,tmpLine) == FAILURE)
      {
      return(FAILURE);
      }

    MStringAppendF(OList,"%s%s",
        (!OList->empty()) ? "," : "",
        tmpLine);

    if ((TokPtr2 != NULL) && (TokPtr2[0] != '\0'))
      MStringAppendF(OList,":%s",TokPtr2); 

    ptr = MUStrTok(NULL,",",&TokPtr);
    }  /* END while (ptr != NULL) */

  return(SUCCESS);  
  }  /* END MOMapList() */


/* END MObjectMap.c */
