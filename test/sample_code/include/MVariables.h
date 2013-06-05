/* HEADER */

#ifndef __MVARIABLES_H__
#define __MVARIABLES_H__


extern int   MUInsertVarList(char *,mln_t *,mhash_t **,char *,char *,int,mbool_t);
extern int   MVarToMString(mhash_t *,mstring_t *);
extern int   MOGetCommonVarList(enum MXMLOTypeEnum,char *,mgcred_t *,mln_t **,char [][MMAX_BUFFER3],int *);
extern char *MVarLLToString(mln_t *,char *,int);
extern char *MVarToQuotedString(mhash_t *,char *,int);


#endif  /* __MVARIABLES_H__ */
