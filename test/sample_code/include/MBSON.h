
#ifndef __MBSON_H__
#define __MBSON_H__

#include "moab.h"
#include "mcom-proto.h"

#ifdef MUSEMONGODB
#include <mongo/client/dbclient.h>

using namespace mongo;

int MNodeTransitionToBSON(mnode_t *,BSONObjBuilder **);
int MVMTransitionToBSON(mvm_t *,BSONObjBuilder **);
int MJobTransitionToBSON(mjob_t *,BSONObjBuilder **);

#endif /* MUSEMONGODB */
#endif /* __MBSON_H__ */
