#include "moab.h"
#include "moab-proto.h"

/**
 * @file MAssert.c
 *
 * Defines MAssert functions that wrap assert.
 */


/**
 * Basic assertion. Call indirectly through macro MASSERT
 *
 * @return Returns failure if test failed.
 */

int MAssert(

  int         TestVal,
  const char *ErrString,
  const char *FuncName,
  const long  LineNo)

  {
  if (!TestVal)
    {
#ifdef NDEBUG
    MLog("ASSERT FAILED:   '%s'\n",
        (ErrString == NULL) ? "" : ErrString);
#else
    MLog("ASSERT FAILED:   %s:%d '%s'\n",
        (FuncName == NULL) ? "UNKNOWN" : FuncName,
        (LineNo <= 0) ? 0 : LineNo,
        (ErrString == NULL) ? "" : ErrString);

    MAbort();
#endif
    }

  return(TestVal != 0);
  } /* END MAssert() */

/* END MAssert.c */
