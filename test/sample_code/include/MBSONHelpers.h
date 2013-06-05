
#ifndef __MBSON_H__
#define __MBSON_H__

#include "moab.h"
#include "mcom-proto.h"

#ifdef MUSEMONGODB
#include <mongo/client/dbclient.h>

using namespace mongo;

/* Helpers */
void MBSONAppendStrOrNull(string,char *,BSONObjBuilder *);
void MBSONAppendStrOrNull(string,string,BSONObjBuilder *);
void MBSONAppendProxyOrNull(string,char *,BSONObjBuilder *);
void MBSONAppendProxyOrNull(char *,BSONArrayBuilder *);
void MBSONAppendDateOrNull(string,mulong,BSONObjBuilder *);
void MBSONAppendDateOrNull(string,long,BSONObjBuilder *);

#endif /* MUSEMONGODB */
#endif /* __MBSON_H__ */
