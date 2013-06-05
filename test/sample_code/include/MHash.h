/* HEADER */

/**
 * @file MHash.h
 *
 * declarations for Hash functions
 *
 */

#ifndef __MHASH_H__
#define __MHASH_H__

#include "moab.h"

extern uint32_t hashlittle(const void *,size_t,uint32_t);
extern uint32_t hashbig(const void *,size_t,uint32_t);

#endif /*  __MHASH_H__ */
