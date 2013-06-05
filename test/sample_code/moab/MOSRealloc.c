/* HEADER */

      
/**
 * @file MOSRealloc.c
 *
 * Contains: MOSrealloc and MOSstrdup
 */

#include "moab.h"
#include "moab-proto.h"  
#include "moab-const.h"  
#include "moab-global.h"  


/*
 * moab OS function to realloc memory
 *
 * @param ptr  old alloc'd memory ptr
 * @param size new size
 */
void *MOSrealloc(
    
  void *ptr, size_t size)

  {
    /* call the OS function and return its return value */
    return(realloc(ptr,size));
  }

/* moab OS function to strdup
 *
 * @param ptr   old alloc'd memory ptr
 */
char *MOSstrdup(
    
    const char *s)

  {
    return(strdup(s));
  }
/* END MOSRealloc.c */
