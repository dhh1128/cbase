#ifndef __MMongo_H__
#define __MMongo_H__

#include "moab.h"
#include "mcom-proto.h"
#include <string>

#ifdef MUSEMONGODB
#include <mongo/client/dbclient.h>
#endif /* MUSEMONGODB */

/* This class is an interface for interfacing with MongoDB */
class MMongoInterface
  {
  public:
  static void MTransOToMongo(void *,enum MXMLOTypeEnum);
  static void MMongoDeleteO(char *,enum MXMLOTypeEnum);

  static void Init(std::string HostAndPortStr);
  static void Teardown();

  static mbool_t ConnectionIsDown();
  static int TestConnection();

  static void DropTables();
  static void CreateIndexes();

  private:

#ifdef MUSEMONGODB
  static mongo::DBClientConnection *Connection;
  static mulong ConnLostTime;         /* Time that the connection was lost */
#endif
  };  /* END class MMongoInterface */

#endif /* __MMongo_H__ */
