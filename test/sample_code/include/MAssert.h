/* HEADER */

/**
 * @file MAssert.h
 *
 * Declares the APIs for moab's assert operations.
 */

#ifndef __MASSERT_H
#define __MASSERT_H

int MAssert(int,const char *,const char *,const long);

/**
 * Assert macro that will log an error message and cause the calling function 
 * to return or to cause the process to abort if NDEBUG is not defined.
 */

#define MASSERT(BoolVal,MsgString) \
  { if (!(BoolVal)) return (MAssert(BoolVal,MsgString,__func__,__LINE__)); }


#endif /* __MASSERT_H */
