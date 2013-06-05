/* HEADER */

/**
 * @file MUBitMaps.h
 *
 * Bit Map functions 
 *
 */

#ifndef __MUBITMAPS_H__
#define __MUBITMAPS_H__

#include <string>
#include "moab.h"


void bmunset(mbitmap_t *,unsigned int);
void bmset(mbitmap_t *,unsigned int);
void bmclear(mbitmap_t *);
void bmcopy(mbitmap_t *,const mbitmap_t *);

mbool_t bmisclear(const mbitmap_t *);
mbool_t bmisset(const mbitmap_t *,unsigned int);
mbool_t bmissetall(const mbitmap_t *,unsigned int);

char   *bmtostring(const mbitmap_t *,const char **,char *,const char *SDelim=NULL,const char *EString=NULL);
int     bmtostring(const mbitmap_t *,const char **,mstring_t *,const char *SDelim=NULL,const char *EString=NULL);
std::string  bmtostring(const mbitmap_t *);

void bmor(mbitmap_t *,const mbitmap_t *);
void bmnot(mbitmap_t *,unsigned int);
void bmand(mbitmap_t *,const mbitmap_t *);
void bmsetall(mbitmap_t *,unsigned int);
void bmcopybit(mbitmap_t *,const mbitmap_t *,unsigned int);
void bmsetbool(mbitmap_t *,unsigned int,mbool_t);

int bmfromstring(const char *,const char **,mbitmap_t *,const char *SDelim=NULL);
int bmcompare(const mbitmap_t *,const mbitmap_t *);

#endif /*  __MUBITMAPS_H__ */
