/* 
 * @file   MStat_Converter.h
 * contains forward declarations for functions used in MStatConverter.c.
 */

#ifndef _MSTAT_CONVERTER_H
#define	_MSTAT_CONVERTER_H


#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <regex.h>
#include <dirent.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "moab.h"
#include "moab-proto.h"

extern msched_t MSched;
extern const char *MDBType[];
extern const char *MStatAttr[];
extern const char *MXO[];
extern const char *MNodeStatType[];
extern const char *MProfAttr[];

typedef struct mstat_union {
  must_t GStat;
  mnust_t NStat;
} mstat_union;

/**
 * represents an array of mstat_union objects
 */

typedef struct must_range_t {
  marray_t Stats;
  mulong StartTime;
  mulong TimeInterval;
} must_range_t;

int MStatReadProfile(mxml_t *,enum MXMLOTypeEnum,must_range_t *,char *);
int MStatFileToDB(mdb_t *,char const *,char *);
int MConfigureDBConn(char *);
int MStatRangeInit(must_range_t *,long unsigned int,long unsigned int);
int MStatTransferToDB(mdb_t *,char *);
int MCPTransferToDB(mdb_t *,char *);
int MDirMatchingFiles(DIR *,regex_t *,marray_t *,char *);
int MStatWriteProfileToDB(mdb_t *,char const *,enum MXMLOTypeEnum,must_range_t *,char *);
int MStatRangeClear(must_range_t *);


#endif



