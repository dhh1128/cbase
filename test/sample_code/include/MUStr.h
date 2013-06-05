/* HEADER */

/**
 * @file MUStr.h
 *
 * Contains Moab StrXXX APIs Interface
 */

#ifndef __MUSTR_H__
#define __MUSTR_H__


int   MUStrCpy(char *,char const *,int);
int   MUStrCpyQuoted(char *,char const *,int);
int   MUStrLTrim(char *String);
int   MUStrRTrim(char *String);
int   MUStrReplaceStr(char *,const char *,const char *,char *,int);
int   MUStrReplaceChar(char *,char,char,mbool_t);
int   MUStrCat(char *,const char *,int);
int   MUStrToLower(char *);
int   MUStrToUpper(const char *,char *,int);
int   MUStrRemoveFromList(char *,const char *,char,mbool_t);
int   MUStrSplit(char *,const char *,int,char **,int);
int   MUStrRemoveFromEnd(char *,const char *);
int   MUStrCmpReverse(const char *,const char *);
int   MUStrAppend(char **,int *,const char *,char);
int   MUStrAppendStatic(char *,const char *,char,int);

char *MUStrTok(char *,const char *,char **);
char *MUStrTokE(char *,const char *,char **);
char *MUStrTokEArray(char *,const char *,char **,mbool_t);
char *MUStrTokEPlusDoer(char *,const char *,char **,mbool_t);
char *MUStrTokEPlus(char *,const char *,char **);
char *MUStrTokEArrayPlus(char *,const char *,char **,mbool_t);
char *MUStrTokEPlusLeaveQuotes(char *,const char *,char **);
int   MUStrNCmpCI(const char *,const char *,int);
char *MUStrStr(char *,const char *,int,mbool_t,mbool_t);
char *MUStrScan(char const *,char const *,int *,char const **);
char *MUStrChr(const char *,const char);
char *MUStrFormat(const char *Format,...);

int MUBufReplaceString(char **,int *,int,int,const char *,mbool_t);
int MUBufRemoveChar(char *,char,char,char);

mbool_t MUStrHasAlpha(const char *);
mbool_t MUStrHasDigit(const char *);
mbool_t MUStrIsDigit(const char *);
mbool_t MUStrIsEmpty(const char *);

int MUStringChop(char *);

mbool_t MUStrIsInList(const char *,const char *,const char);

#endif /* __MUSTR_H__ */
