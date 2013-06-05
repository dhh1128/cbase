/* HEADER */

/**
 * @file MUEnv.c
 * 
 * Contains various functions for ENVIRONMENT interfacing
 *
 */


#include "moab.h"
#include "moab-proto.h"
#include "moab-const.h"
#include "moab-global.h"




/**
 *
 *
 * @param Name
 * @param Offset
 */

char *MUFindEnv(

  const char *Name,
  int        *Offset)

  {
  extern char **environ;
  int           len;
  const char   *np;
  char        **p;
  char         *c;

  if ((Name == NULL) || (environ == NULL))
    {
    return(NULL);
    }

  /* determine length of variable name */

  for (np = Name;((*np != '\0') && (*np != '='));np++);

  len = np - Name;

  for (p = environ;(c = *p) != NULL;p++)
    {
    if ((strncmp(c,Name,len) == 0) && (c[len] == '='))
      {
      *Offset = p - environ;

      return(c + len + 1);
      }
    }

  return(NULL);
  }  /* END MUFindEnv() */



/**
 * report variable value of env var string
 *
 * Enforce record boundaries at ';' AND optional delim
 */

int MUStringGetEnvValue(

  char       *Env,
  const char *Variable,
  char        SpecDelim,  /* I (optional) */
  char       *OBuf,
  int         OBufSize)

  {
  char *ptr;
  char *tail;
  
  char *dtail;

  if (OBuf != NULL)
    OBuf[0] = '\0';

  if ((Env == NULL) || (Variable == NULL))
    {
    return(FAILURE);
    }

  ptr = Env;

  while (ptr != NULL)
    {
    ptr = strstr(ptr,Variable);

    if (ptr == NULL)
      break;

    if ((ptr != Env) && (*(ptr - 1) != ';'))
      {
      if ((SpecDelim == '\0') ||
         ((SpecDelim != '\0') && (*(ptr - 1) != SpecDelim)))
        {
        /* SpecDelim specified - found variable fragment - continue */

        ptr++;

        continue;
        }
      }

    if (ptr[strlen(Variable)] != '=')
      {
      /* found variable fragment - continue */

      ptr++;

      continue;
      }

    if (OBuf != NULL)
      {
      /* advance past variable and '=' */

      ptr += strlen(Variable) + 1;

      tail = strchr(ptr,';');

      if (SpecDelim != '\0')
        {
        dtail = strchr(ptr,SpecDelim);

        if (dtail != NULL)
          {
          if (tail != NULL)
            tail = MIN(tail,dtail);
          else
            tail = dtail;
          }
        }

      if (tail != NULL)
        {
        /* copy out to variable delimiter */

        MUStrCpy(OBuf,ptr,MIN(OBufSize,tail - ptr + 1));
        }
      else
        {
        /* variable is at end of list */

        MUStrCpy(OBuf,ptr,OBufSize);
        }
      }

    return(SUCCESS);
    }  /* END while (ptr != NULL) */

  return(FAILURE);
  }  /* END MStringGetEnvValue() */





/**
 * Add environment variable name-value pair to muenv_t structure 
 *
 * NOTE:  allocate dynamic memory for var name/value
 *
 * @param ETable (I/O)
 * @param EName (I) [ATTR or ATTR=VAL]
 * @param EValue (I) [optional]
 */

int MUEnvAddValue(

  muenv_t    *ETable,
  const char *EName,
  const char *EValue)

  {
  int     size;

  if (ETable == NULL)
    {
    return(FAILURE);
    }

  if ((ETable == NULL) ||
      (EName == NULL))
    {
    /* table full */

    return(FAILURE);
    }

  /* FORMAT:  { <ENAME> | <ENAME>=<EVAL> } \0 */

  size = strlen(EName) + 1;

  if (EValue != NULL)
    {
    size += strlen(EValue) + 1;
    }

  if (ETable->Count >= (ETable->Size - 1))
    {
    ETable->Size += 16;

    ETable->List = (char **)realloc(ETable->List,ETable->Size * (sizeof(char *)));
    }

  ETable->List[ETable->Count] = (char *)MUMalloc(size);

  /* EName can be AVP */

  if (EValue == NULL)
    {
    strcpy(ETable->List[ETable->Count],EName);
    }
  else
    {
    sprintf(ETable->List[ETable->Count],"%s=%s",
      EName,
      EValue);
    }

  ETable->Count++;

  ETable->List[ETable->Count] = NULL;

  return(SUCCESS);
  }  /* END MUEnvAddValue() */







/**
 * Set default environment values into muent_v structure.
 *
 * @param EP (I) [modified]
 * @param UID (I)
 */

int MUEnvSetDefaults(

  muenv_t *EP,   /* I (modified) */
  int      UID)  /* I */

  {
  struct passwd *pwd;
  struct passwd  pwdStruct;
  char   pwdBuf[MMAX_LINE * 4];

#ifndef MNOREENTRANT
  int    rc = 0;
#endif /* !MNOREENTRANT */

  if (EP == NULL)
    {
    return(FAILURE);
    }

  memset(EP,0,sizeof(muenv_t));

  if (UID == -1)
    {
    return(SUCCESS);
    }

  pwdBuf[0] = '\0';

#ifndef MNOREENTRANT
  /* getpwnam_r() will return positive value on FAILURE */

  rc = getpwuid_r(UID,&pwdStruct,pwdBuf,sizeof(pwdBuf),&pwd);

  if ((rc != 0) || (pwd == NULL))
    {
    return(FAILURE);
    }
#else
  pwd = getpwuid(UID);

  if (pwd == NULL)
    {
    return(FAILURE);
    }
#endif /* !MNOREENTRANT */

  /* set HOME, LOGNAME, SHELL */

  MUEnvAddValue(EP,"HOME",pwd->pw_dir);
  MUEnvAddValue(EP,"LOGNAME",pwd->pw_name);
  MUEnvAddValue(EP,"SHELL",pwd->pw_shell);

  /* set Moab-specific vars */

  MUEnvAddValue(EP,"MOABHOMEDIR",MSched.VarDir);

  return(SUCCESS);
  }  /* END MUEnvSetDefaults() */





/**
 *
 *
 * @param E (I) [modified]
 */

int MUEnvDestroy(

  muenv_t *E)  /* I (modified) */

  {
  int eindex;

  if ((E == NULL) || (E->List == NULL))
    {
    return(SUCCESS);
    }

  for (eindex = 0;eindex < E->Count;eindex++)
    {
    MUFree(&E->List[eindex]);
    }  /* END for (eindex) */

  MUFree((char **)&E->List);

  return(SUCCESS);
  }  /* END MUEnvDestroy() */



/**
 * Search for and load environment variable info from muenv_t structure.
 *
 * see MUEnvAddValue()
 *
 * @param EP (I)
 * @param EName (I)
 */

char *MUEnvGetVal(

  muenv_t    *EP,
  const char *EName)

  {
  int eindex;
  int len;

  int vlen;

  /* FORMAT:  <NAME>=[<VAL>] */

  if ((EP == NULL) || (EP->List == NULL) || (EName == NULL))
    {
    return(NULL);
    }

  len = strlen(EName);

  for (eindex = 0;eindex < EP->Count;eindex++)
    {
    vlen = strlen(EP->List[eindex]);

    if ((vlen < len) || (EP->List[eindex][len] != '='))
      continue;

    if (!strncmp(EName,EP->List[eindex],len))
      {
      return(&EP->List[eindex][len + 1]);
      }
    }    /* END for (eindex) */

  return(NULL);
  }  /* END MUEnvGetVal() */




/**
 * Insert environment variable into muenv_t structure.
 *
 * @see MUEnvGetVal()
 *
 * @param SrcString (I)
 * @param EP (I) [optional]
 * @param DstBuf (O) [minsize=MMAX_LINE]
 *
 * @return If a NULL DstBuf is passed in, this function will return NULL as well!
 */

char *MUInsertEnvVar(

  const char  *SrcString,
  muenv_t     *EP,
  char        *DstBuf)

  {
  char tmpVarName[MMAX_LINE];

  int sindex;
  int dindex;
  int vindex;

  char *head;

  char *eval;

  if (DstBuf == NULL)
    {
    return(NULL);
    }

  head = DstBuf;

  head[0] = '\0';

  if ((SrcString == NULL) || (SrcString[0] == '\0'))
    {
    return(head);
    }

  dindex = 0;

  for (sindex = 0;SrcString[sindex] != '\0';sindex++)
    {
    if (sindex >= (MMAX_LINE - 1))
      break;

    if (SrcString[sindex] == '$')
      {
      sindex++;

      vindex = 0;

      /* extract variable */

      for (;SrcString[sindex] != '\0';sindex++)
        {
        if (!isalnum(SrcString[sindex]))
          break;

        tmpVarName[vindex++] = SrcString[sindex];  
        }  /* END for () */

      if (vindex == 0)
        {
        continue;
        }

      tmpVarName[vindex] = '\0';

      eval = NULL;

      if (EP != NULL)
        {
        eval = MUEnvGetVal(EP,tmpVarName);
        } 
      else
        {
        eval = getenv(tmpVarName);
        }
        
      if (eval != NULL) 
        {
        MUStrCat(head,eval,MMAX_LINE);

        dindex += strlen(eval);
        }
      }    /* END if (SrcString[sindex] == '$') */

    /* copy character */

    head[dindex++] = SrcString[sindex];
    head[dindex] = '\0';
    }      /* END for (sindex) */

  head[dindex] = '\0';
 
  return(head);
  }  /* END MUInsertEnvVar() */



/**
 *
 *
 * @param Name (I)
 */

int MUUnsetEnv(

  const char *Name)  /* I */

  {
  extern char **environ;

  char **ptr;

  int Offset;

  if (MUFindEnv(Name,&Offset) != NULL)
    {
    /* free(environ[Offset]); */

    for (ptr = &environ[Offset];((ptr != NULL) && (*ptr != NULL));ptr++)
      {
      *ptr = *(ptr + 1);
      }
  
    return(SUCCESS);
    }

  return(FAILURE);
  }  /* END MUUnsetEnv() */





/**
 *
 *
 * @param Var (I)
 * @param Value (I)
 */

int MUSetEnv(

  const char *Var,   /* I */
  char       *Value) /* I */

  {
  static char EnvVal[MMAX_ENVVAR][2][MMAX_LINE];
  int         index;

  const char *FName = "MUSetEnv";

  MDB(4,fSTRUCT) MLog("%s(%s,%s)\n",
    FName,
    Var,
    Value);

  if ((Value == NULL) || (Value[0] == '\0'))
    {
    /* unset env */

    MUUnsetEnv(Var);

    return(SUCCESS);
    }

  for (index = 0;EnvVal[index][0][0] != '\0';index++)
    {
    if (!strcmp(Var,EnvVal[index][0]))
      {
      sprintf(EnvVal[index][1],"%s=%s",
        Var,
        Value);

      putenv(EnvVal[index][1]);
      
      return(SUCCESS);
      }
    }    /* END for (index) */

  if (index < MMAX_ENVVAR)
    {
    strcpy(EnvVal[index][0],Var);

    sprintf(EnvVal[index][1],"%s=%s",
      Var,
      Value);

    putenv(EnvVal[index][1]);

    return(SUCCESS);
    }

  return(FAILURE);
  }  /* END MUSetEnv() */

/* END MUEnv.c */
