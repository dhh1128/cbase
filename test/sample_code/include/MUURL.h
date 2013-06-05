/* HEADER */

/** 
 * MUURL.h
 *
 * declaractions for the URL functions
 */

#ifndef  __MUURL_H__
#define  __MUURL_H__

extern int MUURLCreate(char *,char *,char *,int,mstring_t *);
extern char *MUURLEncode(const char *,char *,int);
extern int MUURLParse(char *,char *,char *,char *,int,int *,mbool_t,mbool_t);
extern int MUURLQueryParse(char *,char *,char *,int *,char *,int,char *,mbool_t);

#endif /*  __MUURL_H__ */
