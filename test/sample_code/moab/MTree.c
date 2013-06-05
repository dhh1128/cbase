/* HEADER */


/**
 * @file MTree.c
 *
 * moab Tree data container functions
 *
 */

#include "moab.h"
#include "moab-proto.h"



/* tree management */



/**
 *
 *
 * @param RootP (I) [modified/alloc]
 * @param Key (I)
 * @param KType (I) [optional]
 * @param Buf () tree space
 * @param BufSize (I)
 * @param BIndex (I/O)
 * @param DoInsert (I)
 * @param KeyExists (O)
 */

int MSTreeFind(

  mtree_t  **RootP,     /* I (modified/alloc) */
  char      *Key,       /* I */
  int        KType,     /* I (optional) */
  mtree_t   *Buf,       /* tree space */
  int        BufSize,   /* I */
  int       *BIndex,    /* I/O */
  mbool_t    DoInsert,  /* I */
  mbool_t   *KeyExists) /* O */

  {
  mtree_t *TN;
  mtree_t *T;

  int      rc;

  *KeyExists = FALSE;

  if ((Key == NULL) || (RootP == NULL))
    {
    /* invalid tree address - failure */

    return(FAILURE);
    }

  TN = *((mtree_t **)RootP);

  T  = TN;

  while (TN != NULL)
    {
    rc = strcmp(Key,TN->SBuf);

    if ((rc == 0) && (TN->Type == KType))
      {
      /* key inserted previously */

      *KeyExists = TRUE;

      return(SUCCESS);
      }

    T = TN;

    TN = (rc < 0) ? T->L : T->R;
    }  /* END while (T != NULL) */

  if (DoInsert == FALSE)
    {
    return(FAILURE);
    }

  /* Key not found, create new node */

  if ((Buf == NULL) || (*BIndex >= BufSize))
    {
    return(FAILURE);
    }

  /* link new node to existing node */

  if (T != NULL)
    {
    rc = strcmp(Key,T->SBuf);

    if (rc < 0)
      {
      T->L = &Buf[*BIndex];
      }
    else
      {
      T->R = &Buf[*BIndex];
      }
    }
  else
    {
    /* root is null */

    *RootP = &Buf[*BIndex];
    }

  /* adjust node pointer */

  T = &Buf[*BIndex];

  (*BIndex)++;

  /* initialize node */

  MUStrCpy(T->SBuf,Key,MMAX_NAME);

  T->Type = KType;

  T->L = NULL;
  T->R = NULL;

  return(SUCCESS);
  }  /* END MSTreeFind() */




/**
 *
 *
 * @param Root (I)
 * @param Type (I) [optional]
 * @param Buf (O)
 * @param BufSize (I)
 */

int MSTreeToString(

  mtree_t *Root,     /* I */
  int      Type,     /* I (optional) */
  char    *Buf,      /* O */
  int      BufSize)  /* I */

  {
  mtree_t *T;

  mtree_t *TList[100];  /* search list */
  int  SDepth;

  char *BPtr = NULL;
  int   BSpace = 0;

  if (Buf != NULL)
    Buf[0] = '\0';

  if ((Buf == NULL) || (Root == NULL))
    {
    return(FAILURE);
    }

  MUSNInit(&BPtr,&BSpace,Buf,BufSize);

  memset(TList,0,sizeof(TList));

  /* return tree in order */

  T = Root;

  SDepth = 1;

  while (SDepth > 0)
    {
    if (TList[SDepth + 1] != NULL)
      {
      /* depth previously traversed */

      if (TList[SDepth + 1] == T->R)
        {
        if ((Type == -1) || (Type == T->Type))
          {
          /* subtree traversal complete (right complete) */

          if (Buf[0] != '\0')
            {
            MUSNPrintF(&BPtr,&BSpace,",%s",
              T->SBuf);
            }
          else
            {
            MUSNPrintF(&BPtr,&BSpace,"%s",
              T->SBuf);
            }
          }

        SDepth--;

        T = TList[SDepth];

        continue;
        }

      if ((TList[SDepth + 1] == T->L) && (T->R == NULL))
        {
        if ((Type == -1) || (Type == T->Type))
          {
          /* subtree traversal complete (no right tree) */

          if (Buf[0] != '\0')
            {
            MUSNPrintF(&BPtr,&BSpace,",%s",
              T->SBuf);
            }
          else
            {
            MUSNPrintF(&BPtr,&BSpace,"%s",
              T->SBuf);
            }
          }

        SDepth--;

        T = TList[SDepth];

        continue;
        }

      /* ignore TList (from unrelated sub-tree ) */
      }  /* END if (TList[SDepth + 1] != NULL) */

    if ((T->L != NULL) && (TList[SDepth + 1] != T->L))
      {
      TList[SDepth] = T;

      T = T->L;

      SDepth++;

      continue;
      }

    if (T->R != NULL)
      {
      TList[SDepth] = T;

      T = T->R;

      SDepth++;

      continue;
      }

    if ((Type == -1) || (Type == T->Type))
      {
      /* leaf node located (no left or right tree) */

      if (Buf[0] != '\0')
        {
        MUSNPrintF(&BPtr,&BSpace,",%s",
          T->SBuf);
        }
      else
        {
        MUSNPrintF(&BPtr,&BSpace,"%s",
          T->SBuf);
        }
      }

    TList[SDepth] = T;

    SDepth--;

    T = TList[SDepth];
    }  /* END while (T != NULL) */

  return(SUCCESS);
  }  /* END MSTreeToString() */


