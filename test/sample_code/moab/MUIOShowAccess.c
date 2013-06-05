/* HEADER */

/**
 * @file MUIOShowAccess.c
 *
 * Contains MUI Object Show Access
 */



#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  



/**
 *
 *
 * @param O (I) [required at parent/optional at child]
 * @param OIndex (I) [required]
 * @param OName (I)
 * @param ShowAccessTo (I)
 * @param Depth (I) [levels of recursion]
 * @param C (I/O) [required]
 * @param E (O) [required by parent]
 */

int MUIOShowAccess(

  void      *O,              /* I (required at parent/optional at child) */
  enum MXMLOTypeEnum OIndex, /* I (required) */
  char      *OName,          /* I */
  mbool_t    ShowAccessTo,   /* I */
  int        Depth,          /* I (levels of recursion) */
  mxtree_t  *C,              /* I/O  list of credentials (required) */
  mxml_t    *E)              /* O    list of credentials (required by parent) */

  {
  mxml_t   *CE;
  mfs_t    *F;
  mfs_t    *tF;

  mjob_t   *J = NULL;

  void     *tO;

  char     *ptr;
  char     *TokPtr;

  int       oindex;

  enum MXMLOTypeEnum OList[] = {
    mxoUser,
    mxoGroup,
    mxoAcct,
    mxoClass,
    mxoQOS,
    mxoNONE };

  mtree_t  NewTBuf[MMAX_CRED];
  int      NewTBufIndex;
  int      NewTBufSize;

  mtree_t *NewTRoot;

  mbool_t Access;
  mbool_t KeyExists;

  char tmpBuf[MMAX_BUFFER];

  char tmpGLine[MMAX_LINE];

  /* parameter C passed in all calls */

  if ((OIndex == mxoNONE) || (C == NULL) || ((OName == NULL) && (O == NULL)))
    {
    return(FAILURE);
    }

  /* initialize */

  NewTBufIndex = 0;
  NewTBufSize  = MMAX_CRED;
  NewTRoot     = NULL;

  /* NOTE:  for each cred added to list, if Depth > 1 and 
            object is not in master list, call recursively */

  if (ShowAccessTo == TRUE)
    {
    int     oindex;

    void   *OE;
    void   *OP;

    char   *NameP;

    mjob_t  tmpJ;

    char   *CName;

    mhashiter_t HTI;

    /* display list of credentials which can access specified object */

    /* initialize structures */

    memset(&tmpJ,0,sizeof(tmpJ));

    if (OName != NULL)
      CName = OName;
    else
      MOGetName(O,OIndex,&CName);

    for (oindex = 0;OList[oindex] != mxoNONE;oindex++)
      {
      if (OList[oindex] == OIndex)
        continue;

      if (OIndex == mxoQOS)
        {
        J = &tmpJ;
  
        memset(J,0,sizeof(mjob_t));

        strcpy(J->Name,"access");
        }  /* END if (OIndex == mxoQOS) */
      else if (OIndex == mxoGroup)
        {
        /* NO-OP */
        }

      MOINITLOOP(&OP,OList[oindex],&OE,&HTI);

      while ((tO = MOGetNextObject(&OP,OList[oindex],OE,&HTI,&NameP)) != NULL)
        {
        if (!strcmp(NameP,NONE) || 
            !strcmp(NameP,ALL) || 
            !strcmp(NameP,"DEFAULT") || 
            !strcmp(NameP,"NOGROUP"))
          continue;

        if (MOGetComponent(tO,OList[oindex],(void **)&tF,mxoxFS) == FAILURE)
          {
          return(FAILURE);
          }

        Access = FALSE;

        switch (OIndex)
          {
          case mxoUser:

            /* no object has access to user */

            break;

          case mxoAcct:

            if ((tF->AAL != NULL) && (MULLCheck(tF->AAL,CName,NULL) == SUCCESS))
              {
              /* cred tO has access to account CName */

              Access = TRUE;
              }
  
            break;

          case mxoGroup:
 
            if ((tF->GAL != NULL) && (MULLCheck(tF->GAL,CName,NULL) == SUCCESS))
              {
              /* cred tO has access to group CName */

              Access = TRUE;
              }
            else if (OList[oindex] == mxoUser)
              {
              if ((O == NULL) && (MOGetObject(OIndex,OName,&O,mVerify) == FAILURE))
                {
                return(FAILURE);
                }

              MOSGetGListFromUName(
                ((mgcred_t *)O)->Name,
                tmpGLine,  /* O */
                NULL,
                sizeof(tmpGLine),
                NULL);

              ptr = MUStrTok(tmpGLine,",",&TokPtr);

              while (ptr != NULL)
                {
                if (!strcmp(ptr,NameP))
                  {
                  /* cred tO has access to group CName (implicit) */

                  Access = TRUE;

                  break;
                  }

                ptr = MUStrTok(NULL,",",&TokPtr);

                continue;
                }  /* END while (ptr != NULL) */
              }

            break;

          case mxoClass:

            if (OList[oindex] == mxoUser)
              {
              if ((O == NULL) && (MOGetObject(OIndex,OName,&O,mVerify) == FAILURE))
                {
                return(FAILURE);
                }

              if (MClassCheckAccess((mclass_t *)O,(mgcred_t *)tO) == SUCCESS)
                {
                /* cred tO has access to class CName */

                Access = TRUE;
                }
              }
            else
              {
              /* the below comment and code doesn't make any sense - all users in a group can then access this class! */
              
              /* by default, all non-user objects can access all classes */
              /* Access = TRUE; */

              Access = FALSE;
              }

            break;

          case mxoQOS:

            switch (OList[oindex])
              {
              case mxoUser:
 
                J->Credential.U = (mgcred_t *)tO;

                break;

              case mxoGroup:

                J->Credential.G = (mgcred_t *)tO;

                break;

              case mxoAcct:
 
                J->Credential.A = (mgcred_t *)tO;

                break;

              case mxoClass:

                J->Credential.C = (mclass_t *)tO;

                break;

              default:

                /* NO-OP */

                break;
              }

            if ((O == NULL) && (MOGetObject(OIndex,OName,&O,mVerify) == FAILURE))
              {
              return(FAILURE);
              }

            if (MQOSGetAccess(&tmpJ,(mqos_t *)O,NULL,NULL,NULL) == SUCCESS)
              {
              /* cred tO has access to qos CName */

              Access = TRUE;
              }

            break;

          default:

            /* NO-OP */

            break;
          }  /* END switch (OIndex) */

        if (Access == FALSE)
          continue;

        /* add object to list */

        MSTreeFind( 
          &C->Root,
          NameP,
          OList[oindex],
          C->Buf,
          C->BufSize,
          &C->BufIndex,
          TRUE,
          &KeyExists);

        if (KeyExists != TRUE)
          {
          MSTreeFind(
            &NewTRoot,
            NameP,
            OList[oindex],
            NewTBuf,
            NewTBufSize,
            &NewTBufIndex,
            TRUE,
            &KeyExists);

          /* NewTBuf[NewTBufIndex - 1].Key = O; (NOTE:  possible future model) */
          }
        }    /* END while ((tO = MOGetNextObject()) != NULL) */
      }      /* END for (oindex) */
    }        /* END if (ShowAccessTo == TRUE) */
  else
    {
    mjob_t    tmpJ;
    char     *CName;

    /* initialize */

    memset(&tmpJ,0,sizeof(tmpJ));

    if (OName != NULL)
      CName = OName;
    else
      MOGetName(O,OIndex,&CName);

    /* show access provided by object to other credentials */

    if ((O == NULL) && (MOGetObject(OIndex,OName,&O,mVerify) == FAILURE))
      {
      return(FAILURE);
      }

    if (MOGetComponent(O,OIndex,(void **)&F,mxoxFS) == FAILURE)
      {
      return(FAILURE);
      }

    /* show users */

    /* NOTE:  non-user credentials do not currently grant access *
     *        to user credentials under any conditions.
     *
     *        THEREFORE, the previous dead-code has been removed in mid Sept 2010,
     *        while converting MUser array to a hash table. */

    /* show groups */

    if ((OIndex == mxoUser) && (MSched.OSCredLookup != mapNever))
      {
      MOSGetGListFromUName(
        ((mgcred_t *)O)->Name,
        tmpBuf,
        NULL,
        sizeof(tmpBuf),
        NULL);

      if (F->GAL != NULL)
        {
        char tmpLine[MMAX_LINE];

        MULLToString(F->GAL,FALSE,NULL,tmpLine,sizeof(tmpLine));

        snprintf(tmpBuf,sizeof(tmpBuf),"%s,%s",
          tmpBuf,
          tmpLine);
        }
      }
    else if (F->GAL != NULL)
      {
      MULLToString(F->GAL,FALSE,NULL,tmpBuf,sizeof(tmpBuf));
      }
    else
      {
      tmpBuf[0] = '\0';
      }

    ptr = MUStrTok(tmpBuf,",",&TokPtr);

    while (ptr != NULL)
      {
      /* only print out groups that exist in Moab */

      if (MGroupFind(ptr,NULL) == SUCCESS)
        {
        MSTreeFind(
          &C->Root,
          ptr,
          mxoGroup,
          C->Buf,
          C->BufSize,
          &C->BufIndex,
          TRUE,
          &KeyExists);

        if (KeyExists != TRUE)
          {
          MSTreeFind(
            &NewTRoot,
            ptr,
            mxoGroup,
            NewTBuf,
            NewTBufSize,
            &NewTBufIndex,
            TRUE,
            &KeyExists);
          }
        }  /* END if (MGroupFind(ptr,NULL) == SUCCESS) */

      ptr = MUStrTok(NULL,",",&TokPtr);
      }  /* END while (ptr != NULL) */    

    /* show accounts */

    MULLToString(F->AAL,FALSE,NULL,tmpBuf,sizeof(tmpBuf));

    ptr = MUStrTok(tmpBuf,",",&TokPtr);

    while (ptr != NULL)
      {
      MSTreeFind(
        &C->Root,
        ptr,
        mxoAcct,
        C->Buf,
        C->BufSize,
        &C->BufIndex,
        TRUE,
        &KeyExists);

      if (KeyExists != TRUE)
        {
        MSTreeFind(
          &NewTRoot,
          ptr,
          mxoAcct,
          NewTBuf,
          NewTBufSize,
          &NewTBufIndex,
          TRUE,
          &KeyExists);
        }

      ptr = MUStrTok(NULL,",",&TokPtr);
      }  /* END while (ptr != NULL) */

    /* show qos's */

    MQOSBMToString(&F->QAL,tmpBuf);

    ptr = MUStrTok(tmpBuf,",",&TokPtr);

    while (ptr != NULL)
      {
      MSTreeFind(
        &C->Root,
        ptr,
        mxoQOS,
        C->Buf,
        C->BufSize,
        &C->BufIndex,
        TRUE,
        &KeyExists);

      if (KeyExists != TRUE)
        {
        MSTreeFind(
          &NewTRoot,
          ptr,
          mxoQOS,
          NewTBuf,
          NewTBufSize,
          &NewTBufIndex,
          TRUE,
          &KeyExists);
        }

      ptr = MUStrTok(NULL,",",&TokPtr);
      }  /* END while (ptr != NULL) */

    /* show classes */

    /* determine if user is in class exclusive list */

    if ((OIndex == mxoUser) && (O != NULL))
      {
      int cindex;

      mclass_t *tmpC;

      /* Using MCLASS_FIRST_INDEX to skip DEFAULT class */
      for (cindex = MCLASS_FIRST_INDEX;cindex < MMAX_CLASS;cindex++)
        {
        tmpC = &MClass[cindex];

        if (tmpC->Name[0] == '\0')
          continue;

        if (MClassCheckAccess(tmpC,(mgcred_t *)O) == FAILURE)
          continue;

        /* add class to list */
 
        MSTreeFind(
          &C->Root,
          tmpC->Name,
          mxoClass,
          C->Buf,
          C->BufSize,
          &C->BufIndex,
          TRUE,
          &KeyExists);

        if (KeyExists != TRUE)
          {
          MSTreeFind(
            &NewTRoot,
            tmpC->Name,
            mxoClass,
            NewTBuf,
            NewTBufSize,
            &NewTBufIndex,
            TRUE,
            &KeyExists);
          }
        }  /* END for (cindex) */
      }    /* END if ((OIndex == mxoUser) && (O != NULL)) */
    }      /* END else (ShowAccessTo == TRUE) */

  /* launch recursive calls if needed */

  if (Depth > 1)
    {
    int tindex;

    /* call recursively with new adds */

    for (tindex = 0;tindex < NewTBufIndex;tindex++)
      {
      MUIOShowAccess(
        NULL,
        (enum MXMLOTypeEnum)NewTBuf[tindex].Type,
        NewTBuf[tindex].SBuf,
        ShowAccessTo,
        Depth - 1,
        C,
        NULL);
      }  /* END for (tindex) */
    }    /* END if (Depth > 1) */

  /* only create/add child at highest level */

  if (E != NULL)
    {
    /* only create/add child at highest level */

    CE = NULL;

    MXMLCreateE(&CE,(char *)MXO[OIndex]);

    if (OName != NULL)
      MXMLSetAttr(CE,(char *)MCredAttr[mcaID],(void *)OName,mdfString);

    MXMLAddE(E,CE);

    /* populate xml w/total tree (sorted) */

    for (oindex = 0;OList[oindex] != mxoNONE;oindex++)
      {
      MSTreeToString(C->Root,OList[oindex],tmpBuf,sizeof(tmpBuf));

      if (tmpBuf[0] == '\0')
        continue;

      switch (OList[oindex])
        {
        case mxoUser:

          MXMLSetAttr(CE,(char *)"UList",(void *)tmpBuf,mdfString);

          break;

        case mxoGroup:

          MXMLSetAttr(CE,(char *)"GList",(void *)tmpBuf,mdfString);

          break;

        case mxoAcct:

          MXMLSetAttr(CE,(char *)"AList",(void *)tmpBuf,mdfString);

          break;

        case mxoQOS:

          MXMLSetAttr(CE,(char *)"QList",(void *)tmpBuf,mdfString);

          break;

        case mxoClass:

          MXMLSetAttr(CE,(char *)"CList",(void *)tmpBuf,mdfString);

          break;

        default:

          /* NO-OP */

          break;
        }  /* END switch (OList[oindex]) */
      }    /* END for (oindex) */
    }      /* END if (E != NULL) */

  return(SUCCESS);
  }  /* END MUIOShowAccess() */
/* END MUIOShowAccess.c */
