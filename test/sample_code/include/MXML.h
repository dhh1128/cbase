/* HEADER */

#if 0
#ifndef __MXML_H__
#define __MXML_H__

#include "mcom.h"
#include "MString.h"

/* mxml_t structure */


typedef struct mxml_s {
  char *Name;        /* node name (alloc) */
  char *Val;         /* node value (alloc) */

  int   ACount;      /* current number of attributes */
  int   ASize;       /* attribute array size */

  int   CCount;      /* current number of children */
  int   CSize;       /* child array size */

  char **AName;      /* array of attribute names (alloc) */
  char **AVal;       /* array of attribute values (alloc) */

  struct mxml_s **C; /* child node list */

  int    CDataCount; /* current number of CData elements */
  int    CDataSize;  /* size of cdata array */
  char **CData;      /* CData elements */

  int     Type;        /* generic XML object type */
  struct mfst_t *TData;       /* optional extension data (alloc) */
  } mxml_t;





/* XML Node Value encoding of a '<' */
#define XML_VALUE_ANGLE_BRACKET_INT        14
#define XML_VALUE_ANGLE_BRACKET_CHAR      '\016'
#define XML_VALUE_ANGLE_BRACKET_STR       "\016"


int MXMLCreateE(mxml_t **,const char *);
int MXMLDestroyE(mxml_t **);
int MXMLExtractE(mxml_t *,mxml_t *,mxml_t **);
int MXMLMergeE(mxml_t *,mxml_t *,char);
int MXMLSetAttr(mxml_t *,const char *,const void *,enum MDataFormatEnum);
int MXMLRemoveAttr(mxml_t *,const char *);
int MXMLToMString(mxml_t *,mstring_t *,char const **,mbool_t); 
int MXMLAppendAttr(mxml_t *,const char *,const char *,char);
int MXMLSetVal(mxml_t *,const void *,enum MDataFormatEnum);
int MXMLAddE(mxml_t *,mxml_t *);
int MXMLSetChild(mxml_t *,char *,mxml_t **);
int MXMLAddChild(mxml_t *,char const *,char const *,mxml_t **);
int MXMLToString(mxml_t *,char *,int,char **,mbool_t);
int MXMLToXString(mxml_t *,char **,int *,int,char const **,mbool_t);
int MXMLGetAttr(mxml_t *,const char *,int *,char *,int);
int MXMLGetAnyAttr(mxml_t *,char *,int *,char *,int);
int MXMLGetAttrMString(mxml_t *,const char *,int *,mstring_t *);
int MXMLDupAttr(mxml_t *,char *,int *,char **);
int MXMLGetAttrF(mxml_t *,const char *,int *,void *,enum MDataFormatEnum,int);
int MXMLGetChild(mxml_t const *,char const *,int *,mxml_t **);
int MXMLGetChildCI(mxml_t *,const char *,int *,mxml_t **);
int MXMLFromString(mxml_t **,const char *,char **,char *);
int MXMLDupE(mxml_t *,mxml_t **);
int MXMLDupENoString(mxml_t  *,mxml_t **);
int MXMLFind(mxml_t *,char *,int,mxml_t **);
int MXMLCompareByName(mxml_t **,mxml_t **);
int MXMLAddCData(mxml_t *,const char *);
int MXMLSort(mxml_t *,int (*)(const void *,const void*));
int MXMLShallowCopyXD(mxml_t *,mxml_t *);
#endif

#endif  /* __MXML_H__ */
