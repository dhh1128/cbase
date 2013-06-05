/* HEADER */

      
/**
 * @file MUMutex.c
 *
 * Contains: Moab Mutex Functions
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  

/**
 * Initializes a Mutex
 *
 */
int MUMutexInit(

  mmutex_t *Mutex)

  {
#ifdef MTHREADSAFE
  pthread_mutex_init(Mutex,NULL);  /* pthread_mutexattr_default instead of NULL? */
#endif /* MTHREADSAFE */

  return(SUCCESS);
  }  /* END MUMutexInit() */


/**
 * Attempts to lock the given mutex. If
 * threading is disabled, then this function
 * is a no-op.
 *
 * @see MUMutexLockSilent() for a MLog safe version
 * of this function.
 *
 * @param Mutex
 */

int MUMutexLock(

  mmutex_t *Mutex)  /* I */

  {
#ifdef MTHREADSAFE
  if (pthread_mutex_lock(Mutex) != 0)
    {
    /* NOTE:  this can result in infinite recursion as MLog() calls MUMutexLock() */

    MDB(0,fCORE) MLog("ALERT:  cannot lock mutex!\n");

    return(FAILURE);
    }
#endif /* MTHREADSAFE */

  return(SUCCESS);
  }  /* END MULockMutex() */


/**
 * Attempts to unlock the given mutex. If
 * threading is disabled, then this function
 * is a no-op.
 *
 * @see MUMutexLockSilent() for a MLog safe version
 * of this function.
 *
 * @param Mutex
 */

int MUMutexUnlock(

  mmutex_t *Mutex)  /* I */

  {
#ifdef MTHREADSAFE
  if (pthread_mutex_unlock(Mutex) != 0)
    {
    MDB(0,fCORE) MLog("ALERT:  cannot unlock mutex!\n");

    return(FAILURE);
    }
#endif /* MTHREADSAFE */

  return(SUCCESS);
  }  /* END MUMutexUnlock() */



/**
 * Same as MUMutexLock(), but does not log anything
 * if there is an error--use in MLog() functions to avoid
 * infinite loops!
 */

int MUMutexLockSilent(

  mmutex_t *Mutex)  /* I */

  {
#ifdef MTHREADSAFE
  if (pthread_mutex_lock(Mutex) != 0)
    {
    return(FAILURE);
    }
#endif /* MTHREADSAFE */

  return(SUCCESS);
  }  /* END MULockMutexSilent() */



/**
 * Same as MUMutexUnlock(), but does not log anything
 * if there is an error--use in MLog() functions to avoid
 * infinite loops!
 */

int MUMutexUnlockSilent(

  mmutex_t *Mutex)  /* I */

  {
#ifdef MTHREADSAFE
  if (pthread_mutex_unlock(Mutex) != 0)
    {
    return(FAILURE);
    }
#endif /* MTHREADSAFE */

  return(SUCCESS);
  }  /* END MUMutexUnlockSilent() */
/* END MUMutex.c */
